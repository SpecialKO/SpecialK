// dear imgui: Renderer for DirectX12
// This needs to be used along with a Platform Binding (e.g. Win32)

#include <SpecialK/stdafx.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_d3d12.h>
#include <SpecialK/render/backend.h>
#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/render/dxgi/dxgi_swapchain.h>
#include <SpecialK/render/d3d12/d3d12_device.h>
#include <SpecialK/render/d3d12/d3d12_dxil_shader.h>

#include <shaders/imgui_d3d11_vs.h>
#include <shaders/imgui_d3d11_ps.h>

#include <SpecialK/render/dxgi/dxgi_hdr.h>

#include <shaders/vs_colorutil.h>
#include <shaders/uber_hdr_shader_ps.h>
#include <shaders/blt_copy_cs.h>

#include <render/ngx/ngx.h>
#include <render/ngx/ngx_dlss.h>

#include <DirectXTex/d3dx12.h>

#include <pix.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"D3D12BkEnd"

#define SK_D3D12_PERSISTENT_IMGUI_DEV_OBJECTS

D3D12GraphicsCommandList_ResourceBarrier_pfn
D3D12GraphicsCommandList_ResourceBarrier_Original     = nullptr;

D3D12GraphicsCommandList_SetPipelineState_pfn
D3D12GraphicsCommandList_SetPipelineState_Original     = nullptr;

D3D12GraphicsCommandList_DrawInstanced_pfn
D3D12GraphicsCommandList_DrawInstanced_Original        = nullptr;

D3D12GraphicsCommandList_DrawIndexedInstanced_pfn
D3D12GraphicsCommandList_DrawIndexedInstanced_Original = nullptr;

D3D12GraphicsCommandList_ExecuteIndirect_pfn
D3D12GraphicsCommandList_ExecuteIndirect_Original      = nullptr;

D3D12GraphicsCommandList_OMSetRenderTargets_pfn
D3D12GraphicsCommandList_OMSetRenderTargets_Original   = nullptr;

static inline constexpr GUID SKID_D3D12ParentCmdQueue = { 0x33ede230, 0xbb84, 0x4585, { 0xa6, 0xd2, 0x9a, 0x47, 0x3,  0x66, 0xc5, 0xdb } };
static inline constexpr GUID SKID_D3D12BackbufferPtr  = { 0xa1d3a46b, 0x1ac1, 0x4e83, { 0x99, 0x83, 0x12, 0x35, 0x18, 0x34, 0x3d, 0x80 } };
static inline constexpr GUID SKID_D3D12BackbufferIdx  = { 0x2d99b3c7, 0x169,  0x48e5, { 0x95, 0xbd, 0x3d, 0x1,  0xcf, 0x55, 0xb3, 0xe0 } };

struct SK_ImGui_D3D12Ctx
{
  SK_ComPtr <ID3D12Device>        pDevice;
  SK_ComPtr <ID3D12Device>        pLastDevice;
  SK_ComPtr <ID3D12RootSignature> pRootSignature;
  SK_ComPtr <ID3D12PipelineState> pPipelineState;

  DXGI_FORMAT                     RTVFormat             = DXGI_FORMAT_UNKNOWN;

  SK_ComPtr <ID3D12Resource>      pFontTexture;
  D3D12_CPU_DESCRIPTOR_HANDLE     hFontSrvCpuDescHandle = { };
  D3D12_GPU_DESCRIPTOR_HANDLE     hFontSrvGpuDescHandle = { };

  HWND                            hWndSwapChain         = SK_HWND_DESKTOP;

  struct FrameHeap
  {
    struct buffer_s : SK_ComPtr <ID3D12Resource>
    { INT size = 0; void* data = nullptr;
    } Vb, Ib;
  } frame_heaps [DXGI_MAX_SWAP_CHAIN_BUFFERS];
} static _imgui_d3d12;

// A few ImGui pipeline states must never be disabled during render mod
extern concurrency::concurrent_unordered_set <ID3D12PipelineState *> _criticalVertexShaders;

HRESULT
WINAPI
D3D12SerializeRootSignature ( const D3D12_ROOT_SIGNATURE_DESC* pRootSignature,
                                    D3D_ROOT_SIGNATURE_VERSION Version,
                                    ID3DBlob**                 ppBlob,
                                    ID3DBlob**                 ppErrorBlob )
{
  static D3D12SerializeRootSignature_pfn
        _D3D12SerializeRootSignature =
        (D3D12SerializeRootSignature_pfn)
           SK_GetProcAddress ( L"d3d12.dll",
                                "D3D12SerializeRootSignature" );

  if (! _D3D12SerializeRootSignature)
    return E_NOTIMPL;

  return
    _D3D12SerializeRootSignature (pRootSignature, Version, ppBlob, ppErrorBlob);
}

struct VERTEX_CONSTANT_BUFFER
{
  float   mvp[4][4];

  // scRGB allows values > 1.0, sRGB (SDR) simply clamps them
  float luminance_scale [4]; // For HDR displays,    1.0 = 80 Nits
                             // For SDR displays, >= 1.0 = 80 Nits
  float steam_luminance [4];
};

void
ImGui_ImplDX12_RenderDrawData ( ImDrawData* draw_data,
              SK_D3D12_RenderCtx::FrameCtx* pFrame )
{
  // Avoid rendering when minimized
  if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
    return;

  if (! ( _d3d12_rbk->_pSwapChain.p && _d3d12_rbk->_pDevice.p ) )
    return;

  //ImGuiIO& io =
  //  ImGui::GetIO ();

  // The cmd list is either closed and needs resetting, or just wants pipeline state.
  if (! pFrame->begin_cmd_list (/*g_PipelineState*/))
  {
    SK_ReleaseAssert (!"ImGui Frame Command List Could not be Reset");
    return;
  }

  auto _frame =
    _d3d12_rbk->getCurrentBackBufferIndex ();

  SK_ReleaseAssert (_frame == pFrame->iBufferIdx);

  SK_ImGui_D3D12Ctx::FrameHeap* pHeap =
    &_imgui_d3d12.frame_heaps [pFrame->iBufferIdx];

  auto& descriptorHeaps =
    pFrame->pRoot->descriptorHeaps;

  SK_ComPtr <ID3D12GraphicsCommandList> ctx =
    pFrame->pCmdList;

  bool sync_cmd_list =
    ( (pHeap->Vb.p == nullptr || pHeap->Vb.data == nullptr || pHeap->Vb.size < draw_data->TotalVtxCount) ||
      (pHeap->Ib.p == nullptr || pHeap->Ib.data == nullptr || pHeap->Ib.size < draw_data->TotalIdxCount) );

  // Creation, or reallocation of vtx / idx buffers required...
  if (sync_cmd_list)
  {
    D3D12_HEAP_TYPE                    heap_type = D3D12_HEAP_TYPE_UPLOAD;
#ifdef __ID3D12GraphicsCommandList10_FWD_DEFINED__
    D3D12_FEATURE_DATA_D3D12_OPTIONS16 opt16     = { };

    pFrame->pRoot->_pDevice->
      CheckFeatureSupport (D3D12_FEATURE_D3D12_OPTIONS16, &opt16,
                                                   sizeof (opt16));

    if (opt16.GPUUploadHeapSupported)
      heap_type = D3D12_HEAP_TYPE_GPU_UPLOAD;
#endif

    if (! pFrame->wait_for_gpu ())
      return;

    try
    {
      if (pHeap->Vb.p == nullptr || pHeap->Vb.size < draw_data->TotalVtxCount)
      {
        if (pHeap->Vb.data != nullptr &&
            pHeap->Vb.p    != nullptr)
            pHeap->Vb->Unmap (0, nullptr);

        auto overAlloc =
          (std::max (draw_data->TotalVtxCount, pHeap->Vb.size) * 2) +
          (std::max (draw_data->TotalVtxCount, pHeap->Vb.size) * 2) % 4096;

        SK_LOGi0 (L"Growing ImGui Vertex Buffer from %d to %d", pHeap->Vb.size, overAlloc);

        pHeap->Vb.Release ();
        pHeap->Vb.size = 0;
        pHeap->Vb.data = nullptr;

        D3D12_HEAP_PROPERTIES
          props                 = { };
          props.Type            = heap_type;

        D3D12_RESOURCE_DESC
          desc                  = { };
          desc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
          desc.Width            = overAlloc * sizeof (ImDrawVert);
          desc.Height           = 1;
          desc.DepthOrArraySize = 1;
          desc.MipLevels        = 1;
          desc.SampleDesc.Count = 1;
          desc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        ThrowIfFailed (
          _imgui_d3d12.pDevice->CreateCommittedResource (
            &props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
              nullptr, IID_PPV_ARGS (&pHeap->Vb.p)));
        SK_D3D12_SetDebugName (       pHeap->Vb.p,
          L"ImGui D3D12 VertexBuffer" + std::to_wstring (_frame));

        pHeap->Vb.size = overAlloc;
      }

      if (pHeap->Ib.p == nullptr || pHeap->Ib.size < draw_data->TotalIdxCount)
      {
        if (pHeap->Ib.data != nullptr &&
            pHeap->Ib.p    != nullptr)
            pHeap->Ib->Unmap (0, nullptr);

        auto overAlloc =
          (std::max (draw_data->TotalIdxCount, pHeap->Ib.size) * 2) +
          (std::max (draw_data->TotalIdxCount, pHeap->Ib.size) * 2) % 8192;

        SK_LOGi0 (L"Growing ImGui Index Buffer from %d to %d", pHeap->Ib.size, overAlloc);

        pHeap->Ib.Release ();
        pHeap->Ib.size = 0;
        pHeap->Ib.data = nullptr;

        D3D12_HEAP_PROPERTIES
          props                 = { };
          props.Type            = heap_type;

        D3D12_RESOURCE_DESC
          desc                  = { };
          desc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
          desc.Width            = overAlloc * sizeof (ImDrawIdx);
          desc.Height           = 1;
          desc.DepthOrArraySize = 1;
          desc.MipLevels        = 1;
          desc.SampleDesc.Count = 1;
          desc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        ThrowIfFailed (
          _imgui_d3d12.pDevice->CreateCommittedResource (
            &props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
              nullptr, IID_PPV_ARGS (&pHeap->Ib.p)));
        SK_D3D12_SetDebugName (       pHeap->Ib.p,
          L"ImGui D3D12 IndexBuffer" + std::to_wstring (_frame));

        pHeap->Ib.size = overAlloc;
      }
    }

    catch (const SK_ComException& e)
    {
      SK_LOG0 ( ( L" Exception: %hs [%ws]", e.what (), __FUNCTIONW__ ),
                  L"ImGuiD3D12" );

      _d3d12_rbk->release (_d3d12_rbk->_pSwapChain);

      return;
    }
  }

  try
  {
    // Write-only
    D3D12_RANGE range = { };

    if (pHeap->Vb.data == nullptr && pHeap->Vb != nullptr)
      ThrowIfFailed (pHeap->Vb->Map (0, &range, (void **)&pHeap->Vb.data));

    if (pHeap->Ib.data == nullptr && pHeap->Ib != nullptr)
      ThrowIfFailed (pHeap->Ib->Map (0, &range, (void **)&pHeap->Ib.data));
  }

  catch (const SK_ComException& e)
  {
    if (pHeap->Vb.data != nullptr && pHeap->Vb.p != nullptr) { pHeap->Vb->Unmap (0, nullptr); pHeap->Vb.data = nullptr; }
    if (pHeap->Ib.data != nullptr && pHeap->Ib.p != nullptr) { pHeap->Ib->Unmap (0, nullptr); pHeap->Ib.data = nullptr; }

    SK_LOG0 ( ( L" Exception: %hs [%ws]", e.what (), __FUNCTIONW__ ),
                L"ImGuiD3D12" );

    _d3d12_rbk->release (_d3d12_rbk->_pSwapChain);

    return;
  }

  // Copy and convert all vertices into a single contiguous buffer
  ImDrawVert* vtx_heap = (ImDrawVert *)pHeap->Vb.data;
  ImDrawIdx*  idx_heap = (ImDrawIdx  *)pHeap->Ib.data;

  ImDrawVert* vtx_ptr = vtx_heap;
  ImDrawIdx*  idx_ptr = idx_heap;

  if (vtx_ptr == nullptr || idx_ptr == nullptr)
    return;

  for (int n = 0; n < draw_data->CmdListsCount; n++)
  {
    const ImDrawList*
      cmd_list = draw_data->CmdLists [n];

    if (config.imgui.render.strip_alpha)
    {
      for (INT i = 0; i < cmd_list->VtxBuffer.Size; i++)
      {
        ImU32 color =
          ImColor (cmd_list->VtxBuffer.Data [i].col);

        uint8_t alpha = (((color & 0xFF000000U) >> 24U) & 0xFFU);

        // Boost alpha for visibility
        if (alpha < 93 && alpha != 0)
          alpha += (93  - alpha) / 2;

        float a = ((float)                       alpha / 255.0f);
        float r = ((float)((color & 0xFF0000U) >> 16U) / 255.0f);
        float g = ((float)((color & 0x00FF00U) >>  8U) / 255.0f);
        float b = ((float)((color & 0x0000FFU)       ) / 255.0f);

        color =                    0xFF000000U  |
                ((UINT)((r * a) * 255U) << 16U) |
                ((UINT)((g * a) * 255U) <<  8U) |
                ((UINT)((b * a) * 255U)       );

#if 0
        cmd_list->VtxBuffer.Data[i].col =
          (ImVec4)ImColor (color);
#else
        cmd_list->VtxBuffer.Data[i].col =
          ImColor (color);
        /// XXX: FIXME
#endif
      }
    }

    if (vtx_ptr != nullptr)
        vtx_ptr = std::copy_n (cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size, vtx_ptr);

    if (idx_ptr != nullptr)
        idx_ptr = std::copy_n (cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size, idx_ptr);
  }

  if (! pFrame->begin_cmd_list ())
    return;

  ctx->SetGraphicsRootSignature (_imgui_d3d12.pRootSignature);
  ctx->SetPipelineState         (_imgui_d3d12.pPipelineState);

  ctx->OMSetRenderTargets       (1, &pFrame->hBackBufferRTV,  FALSE,  nullptr);
  ctx->SetDescriptorHeaps       (1, &descriptorHeaps.pImGui.p);



  // Don't let the user disable ImGui's pipeline state (!!)
  bool                                                                                         _enable = false;
  _imgui_d3d12.pPipelineState->SetPrivateData (SKID_D3D12DisablePipelineState, sizeof (bool), &_enable);
  SK_RunOnce (_criticalVertexShaders.insert (_imgui_d3d12.pPipelineState));


  //
  // HDR STUFF
  //
  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  bool hdr_display =
    (rb.isHDRCapable () &&
     rb.isHDRActive  ());


  // Setup orthographic projection matrix into our constant buffer
  // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right).
  VERTEX_CONSTANT_BUFFER vertex_constant_buffer = { };

  VERTEX_CONSTANT_BUFFER* constant_buffer =
                  &vertex_constant_buffer;

  float L = draw_data->DisplayPos.x;
  float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
  float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
  float T = draw_data->DisplayPos.y;
  float mvp [4][4] =
  {
    { 2.0f/(R-L),     0.0f,         0.0f,       0.0f },
    {  0.0f,          2.0f/(T-B),   0.0f,       0.0f },
    {  0.0f,          0.0f,         0.5f,       0.0f },
    { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
  };
  memcpy (&constant_buffer->mvp, mvp, sizeof (mvp));

  if (! hdr_display)
  {
    constant_buffer->luminance_scale [0] = 1.0f; constant_buffer->luminance_scale [1] = 1.0f;
    constant_buffer->luminance_scale [2] = 0.0f; constant_buffer->luminance_scale [3] = 0.0f;
    constant_buffer->steam_luminance [0] = 1.0f; constant_buffer->steam_luminance [1] = 1.0f;
    constant_buffer->steam_luminance [2] = 1.0f; constant_buffer->steam_luminance [3] = 1.0f;
  }

  else
  {
    SK_RenderBackend::scan_out_s::SK_HDR_TRANSFER_FUNC eotf =
      rb.scanout.getEOTF ();

    bool bEOTF_is_PQ =
     ( eotf == SK_RenderBackend::scan_out_s::SMPTE_2084 );

    constant_buffer->luminance_scale [0] = ( bEOTF_is_PQ ? -80.0f * rb.ui_luminance :
                                                                    rb.ui_luminance );
    constant_buffer->luminance_scale [1] = 2.2f;
    constant_buffer->luminance_scale [2] = rb.display_gamut.minY * 1.0_Nits;
    constant_buffer->steam_luminance [0] = ( bEOTF_is_PQ ? -80.0f * config.platform.overlay_hdr_luminance :
                                                                    config.platform.overlay_hdr_luminance );
    constant_buffer->steam_luminance [1] = 2.2f;
    constant_buffer->steam_luminance [2] = ( bEOTF_is_PQ ? -80.0f * config.uplay.overlay_luminance :
                                                                    config.uplay.overlay_luminance );
    constant_buffer->steam_luminance [3] = 2.2f;
  }

  // Setup viewport
  ctx->RSSetViewports (         1,
    std::array <D3D12_VIEWPORT, 1> (
    {                     0.0f, 0.0f,
      draw_data->DisplaySize.x, draw_data->DisplaySize.y,
                          0.0f, 1.0f
    }                              ).data ()
  );

  // Bind shader and vertex buffers
  UINT                      stride = sizeof (ImDrawVert);
  D3D12_GPU_VIRTUAL_ADDRESS offset = 0;

  ctx->IASetVertexBuffers ( 0,            1,
    std::array <D3D12_VERTEX_BUFFER_VIEW, 1> (
    {       pHeap->Vb->GetGPUVirtualAddress () + offset,
      (UINT)pHeap->Vb.size * stride,
                             stride
    }                                        ).data ()
  );

  ctx->IASetIndexBuffer (
    std::array <D3D12_INDEX_BUFFER_VIEW, 1> (
    { pHeap->Ib->GetGPUVirtualAddress (), sk::narrow_cast <UINT> (
      pHeap->Ib.size * sizeof (ImDrawIdx)),
                       sizeof (ImDrawIdx) == 2 ?
                          DXGI_FORMAT_R16_UINT :
                          DXGI_FORMAT_R32_UINT
    }                                       ).data ()
  );

  ctx->IASetPrimitiveTopology        (D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  ctx->SetGraphicsRoot32BitConstants (0, 24, &vertex_constant_buffer, 0);
  ctx->SetGraphicsRoot32BitConstants ( 2, 8,
                       std::array <float, 8> (
      {                draw_data->DisplayPos.x,  draw_data->DisplayPos.y,
        hdr_display ? draw_data->DisplaySize.x - draw_data->DisplayPos.x : 0.0f,
        hdr_display ? draw_data->DisplaySize.y - draw_data->DisplayPos.y : 0.0f,
                                                                           0.0f, 0.0f, 0.0f, 0.0f }
                                             ).data (),
                                          0
  );

  // Render command lists
  int    vtx_offset = 0;
  int    idx_offset = 0;
  ImVec2 pos        = ImVec2 (0.0f, 0.0f);
  ImVec2 clip_off   = draw_data->DisplayPos;

  for ( int n = 0;
            n < draw_data->CmdListsCount;
          ++n )
  {
    const ImDrawList* cmd_list =
      draw_data->CmdLists [n];

    for ( int cmd_i = 0;
              cmd_i < cmd_list->CmdBuffer.Size;
            ++cmd_i )                     
    {
      const ImDrawCmd* pcmd =
        &cmd_list->CmdBuffer [cmd_i];

      if (pcmd->UserCallback)
      {
        // User callback, registered via ImDrawList::AddCallback()
        // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
        if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
        {
          SK_ReleaseAssert (false && "ImGui_ImplDX12_SetupRenderState (...) not implemented");
                                  // "ImGui_ImplDX12_SetupRenderState (draw_data, ctx, fr);                           
        }
        else
          pcmd->UserCallback (cmd_list, pcmd);
      }

      else
      {
        // Project scissor/clipping rectangles into framebuffer space
        ImVec2 clip_min (pcmd->ClipRect.x - clip_off.x, pcmd->ClipRect.y - clip_off.y);
        ImVec2 clip_max (pcmd->ClipRect.z - clip_off.x, pcmd->ClipRect.w - clip_off.y);
        
        if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
          continue;

        const D3D12_RECT r =
        {
          static_cast <LONG> (clip_min.x), static_cast <LONG> (clip_min.y),
          static_cast <LONG> (clip_max.x), static_cast <LONG> (clip_max.y)
        };

        SK_ReleaseAssert (r.left <= r.right && r.top <= r.bottom);

        const D3D12_GPU_DESCRIPTOR_HANDLE
          texture_handle ((UINT64)(pcmd->GetTexID ()));

        ctx->SetGraphicsRootDescriptorTable ( 1, texture_handle );
        ctx->RSSetScissorRects              ( 1,             &r );
        ctx->DrawIndexedInstanced           ( pcmd->ElemCount, 1,
                                              pcmd->IdxOffset + idx_offset,
                                              pcmd->VtxOffset + vtx_offset, 0 );
      }
    }

    idx_offset += cmd_list->IdxBuffer.Size;
    vtx_offset += cmd_list->VtxBuffer.Size;
  }

  extern bool SK_D3D12_ShouldSkipHUD (void);
              SK_D3D12_ShouldSkipHUD ();
}

static void
ImGui_ImplDX12_CreateFontsTexture (void)
{
  // Build texture atlas
  ImGuiIO& io =
    ImGui::GetIO ();

  if (! _imgui_d3d12.pDevice)
    return;

  if (_imgui_d3d12.pFontTexture)
    return;

  unsigned char* pixels = nullptr;
  int           _width  = 0,
                _height = 0;

  io.Fonts->GetTexDataAsRGBA32 ( &pixels,
                                 &_width, &_height );

  uintptr_t width  = _width;
  uintptr_t height = _height;

  try {
    SK_ComPtr <ID3D12Resource>            pTexture;
    SK_ComPtr <ID3D12Resource>            uploadBuffer;

    SK_ComPtr <ID3D12Fence>               pFence;
    SK_AutoHandle                         hEvent (
                                  SK_CreateEvent ( nullptr, FALSE,
                                                            FALSE, nullptr )
                                                 );

    SK_ComPtr <ID3D12CommandQueue>        cmdQueue;
    SK_ComPtr <ID3D12CommandAllocator>    cmdAlloc;
    SK_ComPtr <ID3D12GraphicsCommandList> cmdList;


    ThrowIfFailed ( hEvent.m_h != 0 ?
                               S_OK : E_UNEXPECTED );

    // Upload texture to graphics system
    D3D12_HEAP_PROPERTIES
      props                 = { };
      props.Type            = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC
      desc                  = { };
      desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
      desc.Width            = (UINT)width;
      desc.Height           = (UINT)height;
      desc.DepthOrArraySize = 1;
      desc.MipLevels        = 1;
      desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
      desc.SampleDesc.Count = 1;

    ThrowIfFailed (
      _imgui_d3d12.pDevice->CreateCommittedResource (
        &props, D3D12_HEAP_FLAG_NONE,
        &desc,  D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr, IID_PPV_ARGS (&pTexture.p))
    );SK_D3D12_SetDebugName (   pTexture.p,
              L"ImGui D3D12 Font Texture");

    size_t uploadPitch = ((size_t)width * (size_t)4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - (size_t)1u)
                                                  & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - (size_t)1u);
    size_t uploadSize  =  (size_t)height * uploadPitch;

    props.Type            = D3D12_HEAP_TYPE_UPLOAD;
    desc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width            = uploadSize;
    desc.Height           = 1;
    desc.Format           = DXGI_FORMAT_UNKNOWN;
    desc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ThrowIfFailed (
      _imgui_d3d12.pDevice->CreateCommittedResource (
        &props, D3D12_HEAP_FLAG_NONE,
        &desc,  D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS (&uploadBuffer.p))
    ); SK_D3D12_SetDebugName (  uploadBuffer.p,
     L"ImGui D3D12 Font Texture UploadBuffer" );

    void        *mapped = nullptr;
    D3D12_RANGE  range  = { 0, uploadSize };

    ThrowIfFailed (uploadBuffer->Map (0, &range, &mapped));

    for ( uintptr_t y = 0; y < height; y++ )
    {
      memcpy ( (void*) ((uintptr_t) mapped + y * uploadPitch),
                                    pixels + y * width * 4,
                                                 width * 4 );
    }

    uploadBuffer->Unmap (0, &range);

    D3D12_TEXTURE_COPY_LOCATION
      dstLocation                                    = { .pResource = pTexture     },
      srcLocation                                    = { .pResource = uploadBuffer };
      srcLocation.Type                               = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
      srcLocation.PlacedFootprint.Footprint.Format   = DXGI_FORMAT_R8G8B8A8_UNORM;
      srcLocation.PlacedFootprint.Footprint.Width    = (UINT)width;
      srcLocation.PlacedFootprint.Footprint.Height   = (UINT)height;
      srcLocation.PlacedFootprint.Footprint.Depth    = 1;
      srcLocation.PlacedFootprint.Footprint.RowPitch = (UINT)uploadPitch;

    D3D12_RESOURCE_BARRIER
      barrier                        = { };
      barrier.Transition.pResource   = pTexture;
      barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
      barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                                       D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    ThrowIfFailed (
      _imgui_d3d12.pDevice->CreateFence (
        0, D3D12_FENCE_FLAG_NONE,
                  IID_PPV_ARGS (&pFence.p))
     ); SK_D3D12_SetDebugName  ( pFence.p,
     L"ImGui D3D12 Texture Upload Fence");

    D3D12_COMMAND_QUEUE_DESC queueDesc { .NodeMask = 1 };

    ThrowIfFailed (
      _imgui_d3d12.pDevice->CreateCommandQueue (
           &queueDesc, IID_PPV_ARGS (&cmdQueue.p))
      ); SK_D3D12_SetDebugName (      cmdQueue.p,
         L"ImGui D3D12 Texture Upload CmdQueue");

    ThrowIfFailed (
      _imgui_d3d12.pDevice->CreateCommandAllocator (
        D3D12_COMMAND_LIST_TYPE_DIRECT,
                    IID_PPV_ARGS (&cmdAlloc.p))
    ); SK_D3D12_SetDebugName (     cmdAlloc.p,
      L"ImGui D3D12 Texture Upload CmdAllocator");

    ThrowIfFailed (
      _imgui_d3d12.pDevice->CreateCommandList (
        0, D3D12_COMMAND_LIST_TYPE_DIRECT,
           cmdAlloc, nullptr,
                  IID_PPV_ARGS (&cmdList.p))
    ); SK_D3D12_SetDebugName (   cmdList.p,
    L"ImGui D3D12 Texture Upload CmdList");

    cmdList->CopyTextureRegion ( &dstLocation, 0, 0, 0,
                                 &srcLocation, nullptr );
    cmdList->ResourceBarrier   ( 1, &barrier           );

    ThrowIfFailed (                     cmdList->Close ());

                   cmdQueue->ExecuteCommandLists (1,
                             (ID3D12CommandList* const*)
                                       &cmdList);

    ThrowIfFailed (
      cmdQueue->Signal (pFence,                       1));
                        pFence->SetEventOnCompletion (1,
                                  hEvent.m_h);
    SK_WaitForSingleObject       (hEvent.m_h, INFINITE);

    // Create texture view
    D3D12_SHADER_RESOURCE_VIEW_DESC
      srvDesc                           = { };
      srvDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
      srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
      srvDesc.Texture2D.MipLevels       = desc.MipLevels;
      srvDesc.Texture2D.MostDetailedMip = 0;
      srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    _imgui_d3d12.pDevice->CreateShaderResourceView (
      pTexture, &srvDesc, _imgui_d3d12.hFontSrvCpuDescHandle
    );

    _imgui_d3d12.pFontTexture.Attach (
      pTexture.Detach ()
    );

    SK_D3D12_SetDebugName (
      _imgui_d3d12.pFontTexture.p,
      L"ImGui D3D12 FontTexture");

    //static_assert(sizeof(ImTextureID) >= sizeof(bd->hFontSrvGpuDescHandle.ptr), "Can't pack descriptor handle into TexID, 32-bit not supported yet.");
    io.Fonts->SetTexID (
      (ImTextureID)_imgui_d3d12.hFontSrvGpuDescHandle.ptr
    );
  }

  catch (const SK_ComException& e) {
    SK_LOG0 ( ( L" Exception: %hs [%ws]", e.what (), __FUNCTIONW__ ),
                L"ImGuiD3D12" );
  };
}

size_t sk_d3d12_texture_s::num_textures = 0;

sk_d3d12_texture_s
SK_D3D12_CreateDXTex ( DirectX::TexMetadata&  metadata,
                       DirectX::ScratchImage& image )
{
  sk_d3d12_texture_s texture = { };

  if (! _imgui_d3d12.pDevice)
    return texture;

  unsigned char* pixels = image.GetImage (0, 0, 0)->pixels;
  intptr_t       width  = static_cast <intptr_t> (metadata.width),
                 height = static_cast <intptr_t> (metadata.height);

  try {
    SK_ComPtr <ID3D12Resource>            pTexture;
    SK_ComPtr <ID3D12Resource>            uploadBuffer;

    SK_ComPtr <ID3D12Fence>               pFence;
    SK_AutoHandle                         hEvent (
                                  SK_CreateEvent ( nullptr, FALSE,
                                                            FALSE, nullptr )
                                                 );

    SK_ComPtr <ID3D12CommandQueue>        cmdQueue;
    SK_ComPtr <ID3D12CommandAllocator>    cmdAlloc;
    SK_ComPtr <ID3D12GraphicsCommandList> cmdList;


    ThrowIfFailed ( hEvent.m_h != 0 ?
                               S_OK : E_UNEXPECTED );

    // Upload texture to graphics system
    D3D12_HEAP_PROPERTIES
      props                 = { .Type = D3D12_HEAP_TYPE_DEFAULT };
    D3D12_RESOURCE_DESC
      desc                  = { };
      desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
      desc.Width            = (UINT)width;
      desc.Height           = (UINT)height;
      desc.DepthOrArraySize = 1;
      desc.MipLevels        = 1;
      desc.Format           = metadata.format;
      desc.SampleDesc.Count = 1;

    static UINT uiTexNum = 0;

    ThrowIfFailed (
      _imgui_d3d12.pDevice->CreateCommittedResource (
        &props, D3D12_HEAP_FLAG_NONE,
        &desc,  D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr, IID_PPV_ARGS (&pTexture.p))
    );SK_D3D12_SetDebugName (   pTexture.p, SK_FormatStringW (
              L"SK D3D12 Generic Texture%d", uiTexNum ) );

    uintptr_t uploadPitch = (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u)
                                     & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
    uintptr_t uploadSize  = height * uploadPitch;

    props.Type     = D3D12_HEAP_TYPE_UPLOAD;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width     = uploadSize;
    desc.Height    = 1;
    desc.Format    = DXGI_FORMAT_UNKNOWN;
    desc.Layout    = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ThrowIfFailed (
      _imgui_d3d12.pDevice->CreateCommittedResource (
        &props, D3D12_HEAP_FLAG_NONE,
        &desc,  D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS (&uploadBuffer.p))
    );  SK_D3D12_SetDebugName ( uploadBuffer.p, SK_FormatStringW (
             L"SK D3D12 Texture UploadBuffer%d", uiTexNum ) );

    void        *mapped = nullptr;
    D3D12_RANGE  range  = { 0, uploadSize };

    ThrowIfFailed (uploadBuffer->Map (0, &range, &mapped));

    for ( int y = 0; y < height; y++ )
    {
      memcpy ( (void*) ((uintptr_t) mapped + y * uploadPitch),
                                    pixels + y * width * 4,
                                                 width * 4 );
    }

    uploadBuffer->Unmap (0, &range);

    D3D12_TEXTURE_COPY_LOCATION
      dstLocation                                    = { .pResource = pTexture     },
      srcLocation                                    = { .pResource = uploadBuffer };
      srcLocation.Type                               = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
      srcLocation.PlacedFootprint.Footprint.Format   = metadata.format;
      srcLocation.PlacedFootprint.Footprint.Width    = (UINT)width;
      srcLocation.PlacedFootprint.Footprint.Height   = (UINT)height;
      srcLocation.PlacedFootprint.Footprint.Depth    = 1;
      srcLocation.PlacedFootprint.Footprint.RowPitch = (UINT)uploadPitch;

    D3D12_RESOURCE_BARRIER
      barrier                        = { };
      barrier.Transition.pResource   = pTexture;
      barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
      barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                                       D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    ThrowIfFailed (
      _imgui_d3d12.pDevice->CreateFence (
        0, D3D12_FENCE_FLAG_NONE,
                  IID_PPV_ARGS (&pFence.p))
     ); SK_D3D12_SetDebugName  ( pFence.p, SK_FormatStringW (
        L"SK D3D12 Texture Upload Fence%d", uiTexNum ) );

    D3D12_COMMAND_QUEUE_DESC queueDesc { .NodeMask = 1 };

    ThrowIfFailed (
      _imgui_d3d12.pDevice->CreateCommandQueue (
           &queueDesc, IID_PPV_ARGS (&cmdQueue.p))
      ); SK_D3D12_SetDebugName (      cmdQueue.p, SK_FormatStringW (
            L"SK D3D12 Texture Upload CmdQueue%d", uiTexNum ) );

    ThrowIfFailed (
      _imgui_d3d12.pDevice->CreateCommandAllocator (
        D3D12_COMMAND_LIST_TYPE_DIRECT,
                    IID_PPV_ARGS (&cmdAlloc.p))
    ); SK_D3D12_SetDebugName (     cmdAlloc.p, SK_FormatStringW (
         L"SK D3D12 Texture Upload CmdAllocator%d", uiTexNum ) );

    ThrowIfFailed (
      _imgui_d3d12.pDevice->CreateCommandList (
        0, D3D12_COMMAND_LIST_TYPE_DIRECT,
           cmdAlloc, nullptr,
                  IID_PPV_ARGS (&cmdList.p))
    ); SK_D3D12_SetDebugName (   cmdList.p, SK_FormatStringW (
       L"SK D3D12 Texture Upload CmdList%d", uiTexNum ) );

    cmdList->CopyTextureRegion ( &dstLocation, 0, 0, 0,
                                 &srcLocation, nullptr );
    cmdList->ResourceBarrier   ( 1, &barrier           );

    ThrowIfFailed (                     cmdList->Close ());

                   cmdQueue->ExecuteCommandLists (1,
                             (ID3D12CommandList* const*)
                                       &cmdList);

    ThrowIfFailed (
      cmdQueue->Signal (pFence,                       1));
                        pFence->SetEventOnCompletion (1,
                                  hEvent.m_h);
    SK_WaitForSingleObject       (hEvent.m_h, INFINITE);

    // Create texture view
    D3D12_SHADER_RESOURCE_VIEW_DESC
      srvDesc                           = { };
      srvDesc.Format                    = metadata.format;
      srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
      srvDesc.Texture2D.MipLevels       = desc.MipLevels;
      srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    const size_t srvDescriptorSize =
      _imgui_d3d12.pDevice->GetDescriptorHandleIncrementSize (D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    auto& descriptorHeaps =
      _d3d12_rbk->frames_ [0].pRoot->descriptorHeaps;

    auto      srvHandleCPU     =
      descriptorHeaps.pImGui->GetCPUDescriptorHandleForHeapStart ();
    auto      srvHandleGPU     =
      descriptorHeaps.pImGui->GetGPUDescriptorHandleForHeapStart ();

    size_t tex_id = ++texture.num_textures;

    texture.hTextureSrvCpuDescHandle.ptr =
              srvHandleCPU.ptr + tex_id * srvDescriptorSize;

    texture.hTextureSrvGpuDescHandle.ptr =
              srvHandleGPU.ptr + tex_id * srvDescriptorSize;

    _imgui_d3d12.pDevice->CreateShaderResourceView (
      pTexture, &srvDesc, texture.hTextureSrvCpuDescHandle
    );

    texture.pTexture =
      pTexture.Detach ();

    SK_D3D12_SetDebugName (
      texture.pTexture, SK_FormatStringW (
    L"SK D3D12 Texture%d", uiTexNum++ ) );
  }

  catch (const SK_ComException& e) {
    SK_LOG0 ( ( L" Exception: %hs [%ws]", e.what (), __FUNCTIONW__ ),
                L"ImGuiD3D12" );
  };

  return texture;
}

bool
ImGui_ImplDX12_CreateDeviceObjects (void)
{
  if (! _imgui_d3d12.pDevice)
    return false;

  try
  {
    if (_imgui_d3d12.pPipelineState)
      ImGui_ImplDX12_InvalidateDeviceObjects ();

    // Create the root signature
    D3D12_DESCRIPTOR_RANGE
      descRange                                     = { };
      descRange.RangeType                           = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      descRange.NumDescriptors                      = 1;

    D3D12_ROOT_PARAMETER
      param [3]                                     = { };

      param [0].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
      param [0].Constants.Num32BitValues            = 24;
      param [0].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_VERTEX;

      param [1].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      param [1].DescriptorTable.NumDescriptorRanges = 1;
      param [1].DescriptorTable.pDescriptorRanges   = &descRange;
      param [1].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;

      param [2].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
      param [2].Constants.ShaderRegister            = 0; // b0
      param [2].Constants.Num32BitValues            = 8;
      param [2].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC
      staticSampler                                 = { };
      staticSampler.Filter                          = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
      staticSampler.AddressU                        = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
      staticSampler.AddressV                        = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
      staticSampler.AddressW                        = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
      staticSampler.ComparisonFunc                  = D3D12_COMPARISON_FUNC_NEVER;
      staticSampler.ShaderVisibility                = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC
      desc                   = { };
      desc.NumParameters     = _countof (param);
      desc.pParameters       =           param;
      desc.NumStaticSamplers =  1;
      desc.pStaticSamplers   = &staticSampler;
      desc.Flags             =
      ( D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS       |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS     |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS );

    SK_ComPtr <ID3DBlob>
                  pBlob;

    ThrowIfFailed (
      D3D12SerializeRootSignature ( &desc,
               D3D_ROOT_SIGNATURE_VERSION_1,
                 &pBlob, nullptr)
    );

    ThrowIfFailed (
      _imgui_d3d12.pDevice->CreateRootSignature (
        0, pBlob->GetBufferPointer (),
           pBlob->GetBufferSize    (),
                  IID_PPV_ARGS (&_imgui_d3d12.pRootSignature.p))
    ); SK_D3D12_SetDebugName (   _imgui_d3d12.pRootSignature.p,
                                L"ImGui D3D12 Root Signature");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC
      psoDesc                       = { };
      psoDesc.NodeMask              = 1;
      psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
      psoDesc.pRootSignature        = _imgui_d3d12.pRootSignature;
      psoDesc.SampleMask            = UINT_MAX;
      psoDesc.NumRenderTargets      = 1;
      psoDesc.RTVFormats [0]        = _imgui_d3d12.RTVFormat;
      psoDesc.SampleDesc.Count      = 1;

      psoDesc.VS = {
                imgui_d3d11_vs_bytecode,
        sizeof (imgui_d3d11_vs_bytecode) /
        sizeof (imgui_d3d11_vs_bytecode) [0]
                   };

    // Create the input layout
    static D3D12_INPUT_ELEMENT_DESC local_layout [] = {
      { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0, sk::narrow_cast<UINT>((size_t)(&((ImDrawVert*)0)->pos)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
      { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, sk::narrow_cast<UINT>((size_t)(&((ImDrawVert*)0)->uv)),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
#if 0 // NO HDR ImGui Yet
      { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sk::narrow_cast<UINT>((size_t)(&((ImDrawVert*)0)->col)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
#else
      { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,     0, sk::narrow_cast<UINT>((size_t)(&((ImDrawVert*)0)->col)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
#endif
    };

      psoDesc.InputLayout = {       local_layout, 3   };

      psoDesc.PS = {
                imgui_d3d11_ps_bytecode,
        sizeof (imgui_d3d11_ps_bytecode) /
        sizeof (imgui_d3d11_ps_bytecode) [0]
                   };

    // Create the blending setup
    D3D12_RENDER_TARGET_BLEND_DESC&
      rt0_blendDesc                            = psoDesc.BlendState.RenderTarget [0];
      rt0_blendDesc.BlendEnable                = TRUE;
      rt0_blendDesc.SrcBlend                   = D3D12_BLEND_ONE;
      rt0_blendDesc.DestBlend                  = D3D12_BLEND_INV_SRC_ALPHA;
      rt0_blendDesc.BlendOp                    = D3D12_BLEND_OP_ADD;
      rt0_blendDesc.SrcBlendAlpha              = D3D12_BLEND_ONE;
      rt0_blendDesc.DestBlendAlpha             = D3D12_BLEND_ZERO;
      rt0_blendDesc.BlendOpAlpha               = D3D12_BLEND_OP_ADD;
      rt0_blendDesc.RenderTargetWriteMask      = D3D12_COLOR_WRITE_ENABLE_ALL;

    // Create the rasterizer state
    D3D12_RASTERIZER_DESC&
      rasterDesc                               = psoDesc.RasterizerState;
      rasterDesc.FillMode                      = D3D12_FILL_MODE_SOLID;
      rasterDesc.CullMode                      = D3D12_CULL_MODE_NONE;

    // Create depth-stencil State
    D3D12_DEPTH_STENCIL_DESC&
      stencilDesc                              = psoDesc.DepthStencilState;
      stencilDesc.DepthFunc                    = D3D12_COMPARISON_FUNC_NEVER;
      stencilDesc.FrontFace.StencilFailOp      =
      stencilDesc.FrontFace.StencilDepthFailOp =
      stencilDesc.FrontFace.StencilPassOp      = D3D12_STENCIL_OP_KEEP;
      stencilDesc.FrontFace.StencilFunc        = D3D12_COMPARISON_FUNC_NEVER;
      stencilDesc.BackFace                     =
      stencilDesc.FrontFace;

    ThrowIfFailed (
      _imgui_d3d12.pDevice->CreateGraphicsPipelineState (
        &psoDesc, IID_PPV_ARGS (&_imgui_d3d12.pPipelineState.p))
    ); SK_D3D12_SetDebugName (   _imgui_d3d12.pPipelineState.p,
                                L"ImGui D3D12 Pipeline State");

    ImGui_ImplDX12_CreateFontsTexture ();

    return true;
  }

  catch (const SK_ComException& e)
  {
    SK_LOG0 ( ( L" Exception: %hs [%ws]", e.what (), __FUNCTIONW__ ),
                L"ImGuiD3D12" );

    return false;
  }
}

void
ImGui_ImplDX12_InvalidateDeviceObjects (void)
{
  if (! _imgui_d3d12.pDevice)
    return;

#ifndef SK_D3D12_PERSISTENT_IMGUI_DEV_OBJECTS
  _imgui_d3d12.pRootSignature.Release ();
  _imgui_d3d12.pPipelineState.Release ();
  
  // We copied g_pFontTextureView to io.Fonts->TexID so let's clear that as well.
  _imgui_d3d12.pFontTexture.Release ();
  
  ImGui::GetIO ().Fonts->SetTexID (0);
  
  for ( auto& frame : _imgui_d3d12.frame_heaps )
  {
    frame.Ib.Release (); frame.Ib.size = 0;
    frame.Vb.Release (); frame.Vb.size = 0;
  }
#endif
}

bool
ImGui_ImplDX12_Init ( ID3D12Device*               device,
                      int                         num_frames_in_flight,
                      DXGI_FORMAT                 rtv_format,
                      D3D12_CPU_DESCRIPTOR_HANDLE font_srv_cpu_desc_handle,
                      D3D12_GPU_DESCRIPTOR_HANDLE font_srv_gpu_desc_handle,
                      HWND                        hwnd )
{
  ImGuiIO& io =
    ImGui::GetIO ();

  IM_ASSERT (io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

  // Setup backend capabilities flags
  //ImGui_ImplDX12_Data* bd = IM_NEW(ImGui_ImplDX12_Data)();
  io.BackendRendererUserData = nullptr;/// XXX FIXME (void *)bd;
  io.BackendRendererName     = "imgui_impl_dx12";

  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;


  SK_LOG0 ( ( L"(+) Acquiring D3D12 Render Context: Device=%08xh, SwapChain: {%lu x %hs, HWND=%08xh}",
                device, num_frames_in_flight,
                                      SK_DXGI_FormatToStr (rtv_format).data (),
                                      hwnd ),
              L"D3D12BkEnd" );

  _imgui_d3d12.pDevice               = device;
  _imgui_d3d12.RTVFormat             = rtv_format;
  _imgui_d3d12.hFontSrvCpuDescHandle = font_srv_cpu_desc_handle;
  _imgui_d3d12.hFontSrvGpuDescHandle = font_srv_gpu_desc_handle;
  _imgui_d3d12.hWndSwapChain         = hwnd;

  auto& rb =
    SK_GetCurrentRenderBackend ();

  rb.displays [rb.active_display].statistics.vblank_counter.resetStats ();

#ifdef SK_D3D12_PERSISTENT_IMGUI_DEV_OBJECTS
  if (_imgui_d3d12.pDevice.p != _imgui_d3d12.pLastDevice.p)
  {
    // We copied g_pFontTextureView to io.Fonts->TexID so let's clear that as well.
    _imgui_d3d12.pFontTexture.Release ();
    
    ImGui::GetIO ().Fonts->SetTexID (0);

    _d3d12_rbk->descriptorHeaps.pImGui.Release ();

    // Create buffers with a default size (they will later be grown as needed)
    for ( auto& frame : _imgui_d3d12.frame_heaps )
    {
      if (frame.Ib.data != nullptr)
          frame.Ib->Unmap (0, nullptr);

      if (frame.Vb.data != nullptr)
          frame.Vb->Unmap (0, nullptr);

      frame.Ib.Release ();
      frame.Vb.Release ();

      frame.Vb.size = 25000;
      frame.Ib.size = 50000;

      frame.Vb.data = nullptr;
      frame.Ib.data = nullptr;
    }

    _imgui_d3d12.pLastDevice = _imgui_d3d12.pDevice;

    device->CreateDescriptorHeap (
      std::array < D3D12_DESCRIPTOR_HEAP_DESC,                1 >
        {          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,    1024,
                   D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 }.data (),
                                   IID_PPV_ARGS (&_d3d12_rbk->descriptorHeaps.pImGui.p));
                          SK_D3D12_SetDebugName ( _d3d12_rbk->descriptorHeaps.pImGui.p,
                                    L"ImGui D3D12 DescriptorHeap" );

    _imgui_d3d12.hFontSrvCpuDescHandle = _d3d12_rbk->descriptorHeaps.pImGui->GetCPUDescriptorHandleForHeapStart ();
    _imgui_d3d12.hFontSrvGpuDescHandle = _d3d12_rbk->descriptorHeaps.pImGui->GetGPUDescriptorHandleForHeapStart ();

    _imgui_d3d12.pRootSignature.Release ();
    _imgui_d3d12.pPipelineState.Release ();
  }
#endif

  return true;
}

void
ImGui_ImplDX12_Shutdown (void)
{
  ///ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();
  ///IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");

  ImGuiIO& io =
    ImGui::GetIO ();

  ImGui_ImplDX12_InvalidateDeviceObjects ();

  if (_imgui_d3d12.pDevice.p != nullptr)
  {
    SK_LOG0 ( ( L"(-) Releasing D3D12 Render Context: Device=%08xh, SwapChain: {%lu x %hs, HWND=%08xh}",
                                            _imgui_d3d12.pDevice.p,
                                            _d3d12_rbk->frames_.size (),
                       SK_DXGI_FormatToStr (_imgui_d3d12.RTVFormat).data (),
                                            _imgui_d3d12.hWndSwapChain),
                L"D3D12BkEnd" );

    _d3d12_rbk->drain_queue ();
  }

  _imgui_d3d12.pDevice.Release ();
  _imgui_d3d12.hFontSrvCpuDescHandle.ptr = 0;
  _imgui_d3d12.hFontSrvGpuDescHandle.ptr = 0;

  io.BackendRendererName     = nullptr;
  io.BackendRendererUserData = nullptr;

  io.BackendFlags &= ~ImGuiBackendFlags_RendererHasVtxOffset;
  
//IM_DELETE(bd);
}

void
SK_ImGui_User_NewFrame (void);

#include <SpecialK/window.h>

void
ImGui_ImplDX12_NewFrame (void)
{
  if ( _imgui_d3d12.pDevice                   == nullptr ||
       _imgui_d3d12.hFontSrvCpuDescHandle.ptr == 0       ||
       _imgui_d3d12.hFontSrvGpuDescHandle.ptr == 0 ) return;

  if (! _imgui_d3d12.pPipelineState)
    ImGui_ImplDX12_CreateDeviceObjects ();

  const SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if (! rb.device)
    return;

  SK_ReleaseAssert (rb.device == _imgui_d3d12.pDevice);

  //// Start the frame
  SK_ImGui_User_NewFrame ();
}


SK_LazyGlobal <SK_ImGui_ResourcesD3D12> SK_ImGui_D3D12;

extern void
SK_D3D12_AddMissingPipelineState ( ID3D12Device        *pDevice,
                                   ID3D12PipelineState *pPipelineState );

void
STDMETHODCALLTYPE
D3D12GraphicsCommandList_ResourceBarrier_Detour ( 
      ID3D12GraphicsCommandList *This,
      UINT                       NumBarriers,
const D3D12_RESOURCE_BARRIER    *pBarriers )
{
  SK_LOG_FIRST_CALL

  if (NumBarriers == 0 || pBarriers == nullptr)
  {
    if (pBarriers == nullptr)
      SK_RunOnce (SK_LOGi0 (L"Game tried to pass a null array of barriers to ID3D12GraphicsCommandList::ResourceBarrier (...)!"));

    return;
  }

  return
    D3D12GraphicsCommandList_ResourceBarrier_Original (This, NumBarriers, pBarriers);
}

void
STDMETHODCALLTYPE
D3D12GraphicsCommandList_SetPipelineState_Detour (
  ID3D12GraphicsCommandList *This,
  ID3D12PipelineState       *pPipelineState )
{
  SK_LOG_FIRST_CALL

  if (pPipelineState != nullptr)
  {
    // We do not actually care what this is, only that it exists.
    UINT size_ = DxilContainerHashSize;

    if ( FAILED ( pPipelineState->GetPrivateData (
                    SKID_D3D12KnownVtxShaderDigest,
                                            &size_, nullptr ) ) )
    {
      SK_ComPtr <ID3D12Device>               pDevice;
      if ( SUCCEEDED (
             This->GetDevice (IID_PPV_ARGS (&pDevice.p))) )
      { SK_D3D12_AddMissingPipelineState (   pDevice.p, pPipelineState ); }
    }

    UINT64 current_frame =
      SK_GetFramesDrawn ();

    pPipelineState->SetPrivateData (
      SKID_D3D12LastFrameUsed, sizeof (current_frame),
                                      &current_frame );

    UINT size    = sizeof (bool);
    bool disable = false;

    pPipelineState->GetPrivateData (
      SKID_D3D12DisablePipelineState, &size, &disable );
              This->SetPrivateData (
      SKID_D3D12DisablePipelineState,  sizeof (bool),
                                             &disable );
  }

  D3D12GraphicsCommandList_SetPipelineState_Original ( This,
                                                        pPipelineState );
}

void
STDMETHODCALLTYPE
D3D12GraphicsCommandList_OMSetRenderTargets_Detour (
        ID3D12GraphicsCommandList   *This,
        UINT                         NumRenderTargetDescriptors,
  const D3D12_CPU_DESCRIPTOR_HANDLE *pRenderTargetDescriptors,
        BOOL                         RTsSingleHandleToDescriptorRange,
  const D3D12_CPU_DESCRIPTOR_HANDLE *pDepthStencilDescriptor )
{
  SK_LOG_FIRST_CALL

  if (config.reshade.is_addon)
  {
    UINT                        size       = sizeof (D3D12_CPU_DESCRIPTOR_HANDLE);
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = { 0 };

    if ( NumRenderTargetDescriptors > 0 &&
           pRenderTargetDescriptors != nullptr )
    {
      rtv_handle.ptr =
        pRenderTargetDescriptors [0].ptr;
    }

    This->SetPrivateData (
      SKID_D3D12RenderTarget0, size, &rtv_handle );
  }

  return
    D3D12GraphicsCommandList_OMSetRenderTargets_Original (
      This, NumRenderTargetDescriptors, pRenderTargetDescriptors,
      RTsSingleHandleToDescriptorRange, pDepthStencilDescriptor
    );
}

void
STDMETHODCALLTYPE
D3D12GraphicsCommandList_DrawInstanced_Detour (
  ID3D12GraphicsCommandList *This,
  UINT                       VertexCountPerInstance,
  UINT                       InstanceCount,
  UINT                       StartVertexLocation,
  UINT                       StartInstanceLocation )
{
  SK_LOG_FIRST_CALL

  UINT size    = sizeof (bool);
  bool disable = false;

  if ( SUCCEEDED ( This->GetPrivateData ( SKID_D3D12DisablePipelineState, &size, &disable ) ) )
  {
    if (disable)
    {
                                  size       = sizeof (D3D12_CPU_DESCRIPTOR_HANDLE);
      D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = { 0 };

      if (SUCCEEDED (This->GetPrivateData (SKID_D3D12RenderTarget0, &size, &rtv_handle)) && rtv_handle.ptr != 0)
      {
        // This currently doesn't work with sRGB
        SK_ReShadeAddOn_RenderEffectsD3D12 ((IDXGISwapChain1 *)SK_GetCurrentRenderBackend ().swapchain.p, nullptr, nullptr, rtv_handle, { 0 });
      }

      //return;
    }
  }

  //     size            = sizeof (bool);
  //bool trigger_reshade = false;
  //
  //This->GetPrivateData (
  //  SKID_D3D12TriggerReShadeOnDraw, &size, &trigger_reshade );
  //          This->SetPrivateData (
  //  SKID_D3D12TriggerReShadeOnDraw,  sizeof (bool),
  //                                         &trigger_reshade );
  //
  //if (trigger_reshade)
  //{
  //}

  return
    D3D12GraphicsCommandList_DrawInstanced_Original (
      This, VertexCountPerInstance, InstanceCount,
               StartVertexLocation, StartInstanceLocation );
}

void
STDMETHODCALLTYPE
D3D12GraphicsCommandList_DrawIndexedInstanced_Detour (
  ID3D12GraphicsCommandList *This,
  UINT                       IndexCountPerInstance,
  UINT                       InstanceCount,
  UINT                       StartIndexLocation,
  INT                        BaseVertexLocation,
  UINT                       StartInstanceLocation )
{
  SK_LOG_FIRST_CALL

  UINT size    = sizeof (bool);
  bool disable = false;

  if ( SUCCEEDED ( This->GetPrivateData ( SKID_D3D12DisablePipelineState, &size, &disable ) ) )
  {
    if (disable)
    {
                                  size       = sizeof (D3D12_CPU_DESCRIPTOR_HANDLE);
      D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = { 0 };

      if (SUCCEEDED (This->GetPrivateData (SKID_D3D12RenderTarget0, &size, &rtv_handle)) && rtv_handle.ptr != 0)
      {
        // This currently doesn't work with sRGB
        SK_ReShadeAddOn_RenderEffectsD3D12 ((IDXGISwapChain1 *)SK_GetCurrentRenderBackend ().swapchain.p, nullptr, nullptr, rtv_handle, { 0 });
      }

      //return;
    }
  }

  return
    D3D12GraphicsCommandList_DrawIndexedInstanced_Original (
      This, IndexCountPerInstance, InstanceCount, StartIndexLocation,
                           BaseVertexLocation, StartInstanceLocation );
}

using                       D3D12GraphicsCommandList_ClearRenderTargetView_pfn =
void (STDMETHODCALLTYPE *)(ID3D12GraphicsCommandList*,D3D12_CPU_DESCRIPTOR_HANDLE,
                           const FLOAT [4],UINT,const D3D12_RECT*);

static D3D12GraphicsCommandList_ClearRenderTargetView_pfn
       D3D12GraphicsCommandList_ClearRenderTargetView_Original = nullptr;

void
STDMETHODCALLTYPE
D3D12GraphicsCommandList_ClearRenderTargetView_Detour (
                             ID3D12GraphicsCommandList   *This,
                             D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView,
                       const FLOAT                          ColorRGBA [4],
                             UINT                                NumRects,
  _In_reads_(NumRects) const D3D12_RECT                           *pRects)
{
  SK_LOG_FIRST_CALL

  return
    D3D12GraphicsCommandList_ClearRenderTargetView_Original (
      This, RenderTargetView, ColorRGBA, NumRects, pRects );
}

void
STDMETHODCALLTYPE
D3D12GraphicsCommandList_ExecuteIndirect_Detour (
  ID3D12GraphicsCommandList *This,
  ID3D12CommandSignature    *pCommandSignature,
  UINT                       MaxCommandCount,
  ID3D12Resource            *pArgumentBuffer,
  UINT64                     ArgumentBufferOffset,
  ID3D12Resource            *pCountBuffer,
  UINT64                     CountBufferOffset )
{
  SK_LOG_FIRST_CALL

  UINT size    = sizeof (bool);
  bool disable = false;

  if ( SUCCEEDED ( This->GetPrivateData ( SKID_D3D12DisablePipelineState, &size, &disable ) ) )
  {
    if (disable)
    {
                                  size       = sizeof (D3D12_CPU_DESCRIPTOR_HANDLE);
      D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = { 0 };

      if (SUCCEEDED (This->GetPrivateData (SKID_D3D12RenderTarget0, &size, &rtv_handle)) && rtv_handle.ptr != 0)
      {
        // This currently doesn't work with sRGB
        SK_ReShadeAddOn_RenderEffectsD3D12 ((IDXGISwapChain1 *)SK_GetCurrentRenderBackend ().swapchain.p, nullptr, nullptr, rtv_handle, { 0 });
      }

      //return;
    }
  }

  return
    D3D12GraphicsCommandList_ExecuteIndirect_Original (
      This, pCommandSignature, MaxCommandCount, pArgumentBuffer,
        ArgumentBufferOffset, pCountBuffer, CountBufferOffset );
}

void
_InitDrawCommandHooks (ID3D12GraphicsCommandList* pCmdList)
{
  if (! pCmdList)
    return;

  if (D3D12GraphicsCommandList_DrawInstanced_Original == nullptr)
  {
    SK_CreateVFTableHook2 ( L"ID3D12GraphicsCommandList::DrawInstanced",
                            *(void***)*(&pCmdList), 12,
                              D3D12GraphicsCommandList_DrawInstanced_Detour,
                    (void **)&D3D12GraphicsCommandList_DrawInstanced_Original );
  }

  if (D3D12GraphicsCommandList_DrawIndexedInstanced_Original == nullptr)
  {
    SK_CreateVFTableHook2 ( L"ID3D12GraphicsCommandList::DrawIndexedInstanced",
                            *(void***)*(&pCmdList), 13,
                              D3D12GraphicsCommandList_DrawIndexedInstanced_Detour,
                    (void **)&D3D12GraphicsCommandList_DrawIndexedInstanced_Original );
  }

  if (D3D12GraphicsCommandList_SetPipelineState_Original == nullptr)
  {
    SK_CreateVFTableHook2 ( L"ID3D12GraphicsCommandList::SetPipelineState",
                            *(void***)*(&pCmdList), 25,
                               D3D12GraphicsCommandList_SetPipelineState_Detour,
                     (void **)&D3D12GraphicsCommandList_SetPipelineState_Original );
  }

  if (D3D12GraphicsCommandList_ResourceBarrier_Original == nullptr)
  {
    SK_CreateVFTableHook2 ( L"ID3D12GraphicsCommandList::ResourceBarrier",
                            *(void***)*(&pCmdList), 26,
                               D3D12GraphicsCommandList_ResourceBarrier_Detour,
                     (void **)&D3D12GraphicsCommandList_ResourceBarrier_Original );
  }

  // 26 ResourceBarrier
  // 27 ExecuteBundle
  // 28 SetDescriptorHeaps
  // 29 SetComputeRootSignature
  // 30 SetGraphicsRootSignature
  // 31 SetComputeRootDescriptorTable
  // 32 SetGraphicsRootDescriptorTable
  // 33 SetComputeRoot32BitConstant
  // 34 SetGraphicsRoot32BitConstant
  // 35 SetComputeRoot32BitConstants
  // 36 SetGraphicsRoot32BitConstants
  // 37 SetComputeRootConstantBufferView
  // 38 SetGraphicsRootConstantBufferView
  // 39 SetComputeRootShaderResourceView
  // 40 SetGraphicsRootShaderResourceView
  // 41 SetComputeRootUnorderedAccessView
  // 42 SetGraphicsRootUnorderedAccessView
  // 43 IASetIndexBuffer
  // 44 IASetVertexBuffers
  // 45 SOSetTargets
  // 46 OMSetRenderTargets
  // 47 ClearDepthStencilView
  // 48 ClearRenderTargetView
  // 49 ClearUnorderedAccessViewUint
  // 50 ClearUnorderedAccessViewFloat
  // 51 DiscardResource
  // 52 BeginQuery
  // 53 EndQuery
  // 54 ResolveQueryData
  // 55 SetPredication
  // 56 SetMarker
  // 57 BeginEvent
  // 58 EndEvent
  // 59 ExecuteIndirect

  if (D3D12GraphicsCommandList_OMSetRenderTargets_Original == nullptr)
  {
    SK_CreateVFTableHook2 ( L"ID3D12GraphicsCommandList::OMSetRenderTargets",
                            *(void***)*(&pCmdList), 46,
                               D3D12GraphicsCommandList_OMSetRenderTargets_Detour,
                     (void **)&D3D12GraphicsCommandList_OMSetRenderTargets_Original );
  }

  if (D3D12GraphicsCommandList_ExecuteIndirect_Original == nullptr)
  {
    SK_CreateVFTableHook2 ( L"ID3D12GraphicsCommandList::ExecuteIndirect",
                            *(void***)*(&pCmdList), 59,
                               D3D12GraphicsCommandList_ExecuteIndirect_Detour,
                     (void **)&D3D12GraphicsCommandList_ExecuteIndirect_Original );
  }

  if (D3D12GraphicsCommandList_ClearRenderTargetView_Original == nullptr)
  {
    SK_CreateVFTableHook2 ( L"ID3D12GraphicsCommandList::ClearRenderTargetView",
                        *(void***)*(&pCmdList), 48,
                           D3D12GraphicsCommandList_ClearRenderTargetView_Detour,
                 (void **)&D3D12GraphicsCommandList_ClearRenderTargetView_Original );
  }

  SK_ApplyQueuedHooks ();
}


/// --------------- UGLY COMPAT HACK ----------------------
D3D12GraphicsCommandList_CopyTextureRegion_pfn
D3D12GraphicsCommandList_CopyTextureRegion_Original = nullptr;

D3D12GraphicsCommandList_CopyResource_pfn
D3D12GraphicsCommandList_CopyResource_Original      = nullptr;

extern bool SK_D3D12_IsTextureInjectionNeeded (void);

ID3D12GraphicsCommandList *SK_DLSSG_CopyCommandList;

// Workaround for Guardians of the Galaxy in HDR
//
void
STDMETHODCALLTYPE
D3D12GraphicsCommandList_CopyResource_Detour (
        ID3D12GraphicsCommandList *This,
        ID3D12Resource            *pDstResource,
        ID3D12Resource            *pSrcResource )
{
  const auto src_desc = pSrcResource->GetDesc (),
             dst_desc = pDstResource->GetDesc ();

  // Thanks NVIDIA Streamline, very helpful.
  if (src_desc.Width  != dst_desc.Width ||
      src_desc.Height != dst_desc.Height) return;

  // Handle scenarios where user changes HDR override mid-game
  static bool had_hdr_override = false;

  const bool bMayNeedCopyAssist =
    dst_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
      ( __SK_HDR_16BitSwap ||
        __SK_HDR_10BitSwap ||
          had_hdr_override ) && pDstResource != nullptr &&
                                pSrcResource != nullptr;

  if (! bMayNeedCopyAssist)
  {
    return
      D3D12GraphicsCommandList_CopyResource_Original (
        This, pDstResource, pSrcResource
      );
  }

  had_hdr_override = true;

  DXGI_SWAP_CHAIN_DESC                   swapDesc = { };
  if (_d3d12_rbk->_pSwapChain != nullptr)
      _d3d12_rbk->_pSwapChain->GetDesc (&swapDesc);

  const auto
    typelessBackbuffer = DirectX::MakeTypeless (swapDesc.BufferDesc.Format),
    typelessSrc        = DirectX::MakeTypeless (src_desc.Format),
    typelessDst        = DirectX::MakeTypeless (dst_desc.Format);

  if ( (typelessDst == typelessBackbuffer || typelessBackbuffer == DXGI_FORMAT_UNKNOWN)  &&
        typelessSrc != typelessDst        &&
         DirectX::BitsPerColor (src_desc.Format) !=
         DirectX::BitsPerColor (dst_desc.Format) )
  {
    const bool bBasicInitComplete =
      (! _d3d12_rbk->frames_.empty ())              &&
         _d3d12_rbk->frames_ [0].fence.p != nullptr &&
         _d3d12_rbk->_pDevice            != nullptr &&
         _d3d12_rbk->_pCommandQueue      != nullptr &&
         _d3d12_rbk->_pSwapChain         != nullptr;

    if (! bBasicInitComplete)
    {
      SK_RunOnce (SK_LOGi0 (L"Cannot perform HDR CopyBuffer because D3D12 Render Backend is not initialized yet..."));
      return;
    }

    // Permit size mismatches, otherwise resizing the SwapChain asynchronously
    //   to the queue this command is running on might break stuff...
    if (This->GetType () == D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
      //// We're copying to the SwapChain, so we can use SK's Blitter to copy an incompatible format
      SK_D3D12_HDR_CopyBuffer (This, pSrcResource, pDstResource);
    }

    if (This->GetType () == D3D12_COMMAND_LIST_TYPE_COMPUTE)
    {
      if ( (dst_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0 ||
           (src_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0 )
      {
        SK_ReleaseAssert ( (dst_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0 &&
                           (src_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0 );

        SK_RunOnce (SK_LOGi0 (L"Cannot perform HDR CopyBuffer because Src or Dst lacks UAV Access..."));
        return;
      }

      const UINT swapIdx =
        _d3d12_rbk->getCurrentBackBufferIndex ();

      auto& cmdList     = This;
      auto& queries     = _d3d12_rbk->queries;
      auto& pDevice     = _d3d12_rbk->_pDevice;
      auto& descriptors = _d3d12_rbk->descriptorHeaps;
      auto& computeCopy = _d3d12_rbk->computeCopy;
      const auto& dlssg = queries.dlssg;

      if (! SK_D3D12_HasDebugName (cmdList))
        cmdList->SetName (L"[DLSSG] Frame Generation Dispatch");

#ifdef SK_PROFILE_WITH_PIX
      PIXBeginEvent (cmdList, 0, "DLSSG Format Conversion");
#endif

      const int copy_stage =
        (computeCopy.current_stage++ % 4);

      const int descriptor_base_idx =
        (swapIdx * 4) + (copy_stage * 4);

      if (copy_stage == 0)
        cmdList->EndQuery (queries.dlssg.pHeap.p, D3D12_QUERY_TYPE_TIMESTAMP, swapIdx * 2);

      D3D12_CPU_DESCRIPTOR_HANDLE dstUAVHandle_CPU = { };
      D3D12_GPU_DESCRIPTOR_HANDLE dstUAVHandle_GPU = { };
      D3D12_CPU_DESCRIPTOR_HANDLE srcUAVHandle_CPU = { };

      const SIZE_T srvDescriptorSize =
        pDevice->GetDescriptorHandleIncrementSize (D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

      const SIZE_T srvCPUHeapStart =
        descriptors.pComputeCopy->GetCPUDescriptorHandleForHeapStart ().ptr;
      const UINT64 srvGPUHeapStart =
        descriptors.pComputeCopy->GetGPUDescriptorHandleForHeapStart ().ptr;

      dstUAVHandle_GPU.ptr = srvGPUHeapStart + static_cast <UINT64> (descriptor_base_idx    ) * srvDescriptorSize;
      dstUAVHandle_CPU.ptr = srvCPUHeapStart + static_cast <SIZE_T> (descriptor_base_idx    ) * srvDescriptorSize;
      srcUAVHandle_CPU.ptr = srvCPUHeapStart + static_cast <SIZE_T> (descriptor_base_idx + 1) * srvDescriptorSize;

      pDevice->CreateUnorderedAccessView (pDstResource, nullptr, nullptr, dstUAVHandle_CPU);
      pDevice->CreateUnorderedAccessView (pSrcResource, nullptr, nullptr, srcUAVHandle_CPU);

      cmdList->SetComputeRootSignature       (     computeCopy.pSignature.p   );
      cmdList->SetPipelineState              (     computeCopy.pPipeline.p    );
      cmdList->SetDescriptorHeaps            ( 1, &descriptors.pComputeCopy.p );
      cmdList->SetComputeRootDescriptorTable ( 0, dstUAVHandle_GPU            );

      SK_D3D12_StateTransition SrcBarrier [] =
        { { D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, pSrcResource },
          { D3D12_RESOURCE_STATE_COPY_DEST,   D3D12_RESOURCE_STATE_UNORDERED_ACCESS, pDstResource } };

      D3D12_RESOURCE_BARRIER
        DstBarrier [3]                        = { };

        DstBarrier [0].Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        DstBarrier [0].Transition.pResource   = pSrcResource;
        DstBarrier [0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        DstBarrier [0].Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;

        DstBarrier [1].Type                   = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        DstBarrier [1].UAV.pResource          = pDstResource;

        DstBarrier [2].Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        DstBarrier [2].Transition.pResource   = pDstResource;
        DstBarrier [2].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        DstBarrier [2].Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;

      static constexpr UINT                                            _BltTileSizeX = 16U;
      static constexpr UINT                                            _BltTileSizeY = 64U;
                 const UINT
        ThreadGroupCountX = (sk::narrow_cast <UINT> (src_desc.Width) + _BltTileSizeX - 1) / _BltTileSizeX,
        ThreadGroupCountY = (                        src_desc.Height + _BltTileSizeY - 1) / _BltTileSizeY,
        ThreadGroupCountZ = 1;

#ifdef SK_DEBUG_DLSSG_COMPUTE_COPY
      SK_LOGi0 (
        L"Compute Queue Copy: %hs [%p (%p)] -> %hs [%p (%p)] -- CmdList: %p",
          SK_D3D12_GetDebugNameUTF8 (pSrcResource).c_str (), pSrcResource, pSrcResource->GetGPUVirtualAddress (),
          SK_D3D12_GetDebugNameUTF8 (pDstResource).c_str (), pDstResource, pDstResource->GetGPUVirtualAddress (),
            cmdList
      );
      
      SK_LOGi0 (
        L"Source: Align=%d, Dims=(%dx%d)[%d], Mips=%d, Fmt=%hs, Layout=%d, Flags=%x",
          src_desc.Alignment,   src_desc.Width,            src_desc.Height,
                                src_desc.DepthOrArraySize, src_desc.MipLevels,
           SK_DXGI_FormatToStr (src_desc.Format).data (),  src_desc.Layout,
                                src_desc.Flags
      );
      
      SK_LOGi0 (
        L"Dest: Align=%d, Dims=(%dx%d)[%d], Mips=%d, Fmt=%hs, Layout=%d, Flags=%x",
          dst_desc.Alignment,   dst_desc.Width,            dst_desc.Height,
                                dst_desc.DepthOrArraySize, dst_desc.MipLevels,
           SK_DXGI_FormatToStr (dst_desc.Format).data (),  dst_desc.Layout,
                                dst_desc.Flags
      );
#endif

      constexpr auto type =
         D3D12_QUERY_TYPE_TIMESTAMP;

      cmdList->ResourceBarrier               ( 2, SrcBarrier                         );
      cmdList->Dispatch                      ( ThreadGroupCountX, ThreadGroupCountY,
                                                                  ThreadGroupCountZ  );
      cmdList->ResourceBarrier               ( 3, DstBarrier                         );

      if (copy_stage == 0)
      {
        cmdList->EndQuery                    ( dlssg.pHeap.p, type, swapIdx * 2 + 1  );
        cmdList->ResolveQueryData            ( dlssg.pHeap.p, type, swapIdx * 2,  2,
                                               dlssg.pReadBack.p,                  0 );
      }

#ifdef SK_PROFILE_WITH_PIX
      PIXEndEvent (cmdList);
#endif

      _d3d12_rbk->frames_ [swapIdx].hdr.format_conversions++;

      computeCopy.lastFrameActive =
                SK_GetFramesDrawn ();

      SK_DLSSG_CopyCommandList = cmdList;
    }

    return;
  }

  D3D12GraphicsCommandList_CopyResource_Original (
    This, pDstResource, pSrcResource
  );
}

//extern void
//SK_D3D12_CopyTexRegion_Dump (ID3D12GraphicsCommandList* This, ID3D12Resource* pResource, const wchar_t *wszName = nullptr);

// Workaround for Control in HDR
void
STDMETHODCALLTYPE
D3D12GraphicsCommandList_CopyTextureRegion_Detour (
         ID3D12GraphicsCommandList   *This,
  const  D3D12_TEXTURE_COPY_LOCATION *pDst,
         UINT                         DstX,
         UINT                         DstY,
         UINT                         DstZ,
  const  D3D12_TEXTURE_COPY_LOCATION *pSrc,
  const  D3D12_BOX                   *pSrcBox )
{
  // Let D3D12 figure out what to do here, this scenario is meaningless to SK...
  if ( pSrc == nullptr || pSrc->pResource == nullptr ||
       pDst == nullptr || pDst->pResource == nullptr )
  {
    D3D12GraphicsCommandList_CopyTextureRegion_Original (
      This, pDst, DstX, DstY, DstZ, pSrc, pSrcBox
    );

    return;
  }

#if 0
  D3D12_RESOURCE_DESC
    src_desc = pSrc->pResource->GetDesc (),
    dst_desc = pDst->pResource->GetDesc ();

  static bool
    bUseInjection =
      SK_D3D12_IsTextureInjectionNeeded ();

  if (                                           bUseInjection &&
      D3D12_RESOURCE_DIMENSION_BUFFER    == src_desc.Dimension &&
      D3D12_RESOURCE_DIMENSION_TEXTURE2D == dst_desc.Dimension &&
      pDst->SubresourceIndex             == 0 &&
      pSrc->SubresourceIndex             == 0 && pSrcBox == nullptr &&
                                    DstX == 0 &&
                                    DstY == 0 && DstZ == 0 )
  {
    UINT size   = 1U;
    bool ignore = false;

    pSrc->pResource->GetPrivateData (SKID_D3D12IgnoredTextureCopy, &size, &ignore);

    if (! ignore)
    {
      size = 1U;
      pDst->pResource->GetPrivateData (SKID_D3D12IgnoredTextureCopy, &size, &ignore);
    }

    if (! ignore)
    {
      //SK_D3D12_CopyTexRegion_Dump (This, pDst->pResource);
    }
  }
#endif

  if ( __SK_HDR_16BitSwap ||
       __SK_HDR_10BitSwap )
  {
    D3D12_RESOURCE_DESC
      src_desc = pSrc->pResource->GetDesc ();

    // Format override siliness in D3D12
    static volatile LONG lSizeSkips   = 0;
    static volatile LONG lFormatSkips = 0;

    if (D3D12_RESOURCE_DIMENSION_TEXTURE2D == src_desc.Dimension &&
        1                                  == src_desc.MipLevels)
    {
      const SK_RenderBackend& rb =
        SK_GetCurrentRenderBackend ();

      SK_ComQIPtr <IDXGISwapChain>
          pSwap (  rb.swapchain  );
      DXGI_SWAP_CHAIN_DESC swapDesc = { };
      if (pSwap)
          pSwap->GetDesc (&swapDesc);

      auto expectedTyplessSrc =
           __SK_HDR_16BitSwap ? DXGI_FORMAT_R16G16B16A16_TYPELESS
                              : DXGI_FORMAT_R10G10B10A2_TYPELESS;

      D3D12_RESOURCE_DESC
        dst_desc = pDst->pResource->GetDesc ();

      //
      // SwapChain Copies:   Potentially fixable using shader-based copy
      //
      if ( pSwap.p == nullptr ||
           ( src_desc.Width  == swapDesc.BufferDesc.Width  &&
             src_desc.Height == swapDesc.BufferDesc.Height &&
            (src_desc.Format == swapDesc.BufferDesc.Format ||
             dst_desc.Format == swapDesc.BufferDesc.Format) )
         )
      {
        if ( dst_desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
             dst_desc.Height    == 1                               && DstX == 0 && DstY    == 0
                                                                   && DstZ == 0 && pSrcBox == nullptr &&
             // Is the destination buffer too small?
             dst_desc.Width     <  src_desc.Width   *
                                   src_desc.Height  *
            DirectX::BitsPerPixel (src_desc.Format) / 8 )
        {
          InterlockedIncrement (&lSizeSkips);

          return;
        }

        else if ( dst_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D )
        {
          auto typelessSrc = DirectX::MakeTypeless (src_desc.Format),
               typelessDst = DirectX::MakeTypeless (dst_desc.Format);

          if ( typelessSrc != typelessDst     &&
               DirectX::BitsPerColor (src_desc.Format) !=
               DirectX::BitsPerColor (dst_desc.Format) )
          {
            // We're copying -to- the SwapChain, so we can use SK's Blitter to copy an incompatible format
            if (typelessSrc != expectedTyplessSrc)
            {
              if (This->GetType () == D3D12_COMMAND_LIST_TYPE_DIRECT)
              {
                SK_D3D12_HDR_CopyBuffer (This, pSrc->pResource, pDst->pResource);

                return;
              }

              SK_RunOnce (
                SK_ImGui_WarningWithTitle (
                  L"If you are using DLSS Frame Generation, try SK's HDR10 mode or run the game using its native HDR.",
                  L"Uncorrectable Format Mismatch on D3D12 CopyTextureRegion"
                );
              );
            }

            else
            {
              //
              // Either some unrelated copy, or the engine is copying back -from- the SwapChain
              //
              //  * This case is not currently implemented (rarely used)
              //

              InterlockedIncrement (&lFormatSkips);

              return;
            }
          }         
        }
      }

      auto typelessFootprintSrc = DirectX::MakeTypeless (pSrc->PlacedFootprint.Footprint.Format),
           typelessFootprintDst = DirectX::MakeTypeless (pDst->PlacedFootprint.Footprint.Format);

      //
      // Handle situations where engine uses some, but not all, modified swapchain properties
      // 
      //  (e.g. knows the format is different, but computes size using the original format)
      //
      if ( typelessFootprintSrc == expectedTyplessSrc ||
           typelessFootprintDst == expectedTyplessSrc )
      {
        const UINT src_row_pitch =      pSrc->PlacedFootprint.Footprint.Width *
          (sk::narrow_cast <UINT> (DirectX::BitsPerPixel (typelessFootprintSrc) / 8));
        const UINT dst_row_pitch =      pDst->PlacedFootprint.Footprint.Width *
          (sk::narrow_cast <UINT> (DirectX::BitsPerPixel (typelessFootprintDst) / 8));

        if ( pSrc->PlacedFootprint.Footprint.RowPitch < src_row_pitch ||
             pDst->PlacedFootprint.Footprint.RowPitch < dst_row_pitch )
        {
          SK_LOGi0 (
            L"Skipping invalid CopyTextureRegion:"
            L" (SrcPitch: Requested = %lu, Valid >= %lu),"
            L" (DstPitch: Requested = %lu, Valid >= %lu) - SrcFmt: (%hs | %hs) /"
            L" DstFmt: (% hs | % hs)",
              pSrc->PlacedFootprint.Footprint.RowPitch, src_row_pitch,
              pDst->PlacedFootprint.Footprint.RowPitch, dst_row_pitch,
                SK_DXGI_FormatToStr (pSrc->PlacedFootprint.Footprint.Format).data (),
                SK_DXGI_FormatToStr (                       src_desc.Format).data (),
                SK_DXGI_FormatToStr (pDst->PlacedFootprint.Footprint.Format).data (),
                SK_DXGI_FormatToStr (                       dst_desc.Format).data ()
          );

          InterlockedIncrement (&lSizeSkips);

          //
          // TODO: Implement a copy-from-swapchain to temporary surface w/ format conversion
          //         and allow this mismatched format subregion copy to read from it.
          //

          return;
        }
      }
    }
  }

  D3D12GraphicsCommandList_CopyTextureRegion_Original (
    This, pDst, DstX, DstY, DstZ, pSrc, pSrcBox
  );
}

void
_InitCopyTextureRegionHook (ID3D12GraphicsCommandList* pCmdList)
{
  if (! pCmdList)
    return;

  if (D3D12GraphicsCommandList_CopyTextureRegion_Original == nullptr)
  {
    SK_CreateVFTableHook2 ( L"ID3D12GraphicsCommandList::CopyTextureRegion",
                            *(void***)*(&pCmdList), 16,
                               D3D12GraphicsCommandList_CopyTextureRegion_Detour,
                     (void **)&D3D12GraphicsCommandList_CopyTextureRegion_Original );
  }

  if (D3D12GraphicsCommandList_CopyResource_Original == nullptr)
  {
    SK_CreateVFTableHook2 ( L"ID3D12GraphicsCommandList::CopyResource",
                            *(void***)*(&pCmdList), 17,
                               D3D12GraphicsCommandList_CopyResource_Detour,
                     (void **)&D3D12GraphicsCommandList_CopyResource_Original );
  }
}
/// --------------- UGLY COMPAT HACK ----------------------


void
SK_D3D12_HDR_CopyBuffer ( ID3D12GraphicsCommandList *pCommandList,
                          ID3D12Resource            *pSrcResource,
                          ID3D12Resource            *pDstResource )
{
  if (pCommandList == nullptr || pSrcResource == nullptr || pDstResource == nullptr)
  {
    SK_RunOnce (SK_LOGi0 (L"Cannot perform HDR CopyBuffer because one or more parameters are nullptr..."));
    return;
  }

  if ( _d3d12_rbk->frames_.empty ()            ||
       _d3d12_rbk->_pCommandQueue.p == nullptr ||
       _d3d12_rbk->_pSwapChain.p    == nullptr )
  {
    SK_RunOnce (SK_LOGi0 (L"Cannot perform HDR CopyBuffer because D3D12 Render Backend is not initialized yet..."));
    return;
  }

  SK_ComPtr <ID3D12Device>
             pD3D12Device;

  if (FAILED (_d3d12_rbk->_pSwapChain->GetDevice (IID_PPV_ARGS (&pD3D12Device.p))))
    return;

  UINT swapIdx =
    _d3d12_rbk->getCurrentBackBufferIndex ();

  if ( swapIdx >= _d3d12_rbk->frames_.size () )
  {
    return;
  }

  SK_D3D12_RenderCtx::FrameCtx& stagingFrame =
    _d3d12_rbk->frames_ [swapIdx];

  SK_ReleaseAssert (stagingFrame.fence.p != nullptr);

  if (stagingFrame.fence == nullptr)
    return;

  // Passthrough mode to reuse HDR shader to blt incompatible image formats
  static auto constexpr TONEMAP_CopyResource = 255;

  static const HDR_LUMINANCE         cbuffer_luma   = { 1.0f, 1.0f, 1.0f, 1.0f };
  static const HDR_COLORSPACE_PARAMS cbuffer_cspace =
    { .uiToneMapper = TONEMAP_CopyResource };

#define SK_D3D12_ALLOW_SKIP_HDR_COPY

  auto& hBufferCopy = stagingFrame.hdr.hBufferCopy;
  auto& pCopyHeap   = _d3d12_rbk->descriptorHeaps.pHDR_CopyAssist_SRV;

  // Disable zero-copy fast-path if ReShade is in use
  const bool skippable = 
#ifdef SK_D3D12_ALLOW_SKIP_HDR_COPY
    (! config.reshade.is_addon) && (config.render.dxgi.allow_d3d12_footguns);
#else
    false;
#endif

  auto& pDstRes     = skippable ?
    stagingFrame.hdr.pSwapChainCopy.p
                    : pDstResource;
  auto& pSrcRes     = pSrcResource;

  if (skippable)
  {
    // This optimization is potentially wrong, so log it the first time we do it.
    SK_RunOnce (
      SK_LOGi0 (L"Using Zero-Copy Fast-Path for HDR SwapChain Processing")
    );
  }

  pD3D12Device->CreateShaderResourceView (pSrcRes, nullptr, hBufferCopy.SRV.CPU);
  pD3D12Device->CreateRenderTargetView   (pDstRes, nullptr, hBufferCopy.RTV.CPU);

  SK_ReleaseAssert ((pDstResource->GetDesc ().Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)  != 0);
  SK_ReleaseAssert ((pSrcResource->GetDesc ().Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0);

  SK_D3D12_StateTransition barriers [] =
   { { D3D12_RESOURCE_STATE_COPY_DEST,
       D3D12_RESOURCE_STATE_RENDER_TARGET,         pDstRes },
     { D3D12_RESOURCE_STATE_COPY_SOURCE,
       D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, pSrcRes },
     { D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
       D3D12_RESOURCE_STATE_COPY_SOURCE,           pSrcRes },
     { D3D12_RESOURCE_STATE_RENDER_TARGET,
       D3D12_RESOURCE_STATE_COPY_DEST,             pDstRes } };

  pCommandList->ResourceBarrier                ( 2,                        &barriers [0] );
  pCommandList->SetGraphicsRootSignature       ( _d3d12_rbk->pHDRSignature               );
  pCommandList->SetPipelineState               ( _d3d12_rbk->pHDRPipeline                );
  pCommandList->SetDescriptorHeaps             ( 1, &pCopyHeap.p                         );
  pCommandList->SetGraphicsRoot32BitConstants  ( 0, 4,  &cbuffer_luma,   0               );
  pCommandList->SetGraphicsRoot32BitConstants  ( 1, 20, &cbuffer_cspace, 0               );
  pCommandList->SetGraphicsRootDescriptorTable ( 2,  hBufferCopy.SRV.GPU                 );
  pCommandList->OMSetRenderTargets             ( 1, &hBufferCopy.RTV.CPU, FALSE, nullptr );
  pCommandList->IASetPrimitiveTopology         ( D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP    );
  pCommandList->RSSetViewports                 ( 1, &stagingFrame.hdr.vp                 );
  pCommandList->RSSetScissorRects              ( 1, &stagingFrame.hdr.scissor            );
  pCommandList->DrawInstanced                  ( 3, 1, 0, 0                              );
  pCommandList->ResourceBarrier                ( 2,                        &barriers [2] );

    // We copied directly to the working buffer for HDR post-processing...
    stagingFrame.hdr.skip_copy = skippable;
  ++stagingFrame.hdr.format_conversions;
}

void
SK_D3D12_RenderCtx::present (IDXGISwapChain3 *pSwapChain)
{
  if (! pSwapChain)
    return;

  if ((! _pDevice.p || frames_.empty ()) || _pCommandQueue.p == nullptr)
  {
    if (! init (pSwapChain, _pCommandQueue.p))
      return;
  }

  if (! _pCommandQueue.p)
    return;

  if (! SK_D3D12_HasDebugName (_pCommandQueue.p))
  {
    static UINT unique_d3d12_qid = 0UL;
  
    SK_D3D12_SetDebugName (    _pCommandQueue.p,
         SK_FormatStringW (
            L"[Game] D3D12 SwapChain CmdQueue%d",
                             unique_d3d12_qid++ )
                          );
  }

  SK_ComPtr <ID3D12Device>
             pD3D12Device;

  if (FAILED (pSwapChain->GetDevice (IID_PPV_ARGS (&pD3D12Device.p))))
    return;

  // This test for device equality will fail if there is a Streamline interposer; ignore it.
  if ((! pD3D12Device.IsEqualObject (_pDevice.p)) && (! SK_IsModuleLoaded (L"sl.interposer.dll")))
    return;

  //std::scoped_lock lock (_d3d12_rbk->_ctx_lock);

  UINT swapIdx =
    getCurrentBackBufferIndex ();

  DXGI_SWAP_CHAIN_DESC1          swapDesc = { };
  pSwapChain->GetDesc1         (&swapDesc);
  if ((_imgui_d3d12.RTVFormat != swapDesc.Format &&
       _imgui_d3d12.RTVFormat != DXGI_FORMAT_UNKNOWN) || swapIdx >= frames_.size () )
  {
    static bool          once = false;
    if (! std::exchange (once, true))
    {
      SK_LOG0 ( ( L"ImGui Expects SwapChain Format %hs, but Got %hs... "
                  L"no attempt to draw will be made.",
                    SK_DXGI_FormatToStr (_imgui_d3d12.RTVFormat).data (),
                    SK_DXGI_FormatToStr (       swapDesc.Format).data () ),
                  __SK_SUBSYSTEM__ );
    }

    return;
  }

  FrameCtx& stagingFrame =
    frames_ [swapIdx];

  SK_ReleaseAssert (stagingFrame.fence.p != nullptr);

  auto pCommandList =
      stagingFrame.pCmdList.p;
  if (stagingFrame.fence == nullptr || pCommandList == nullptr)
    return;

  // Make sure all commands for this command allocator have finished executing before reseting it
  if (stagingFrame.fence->GetCompletedValue () < stagingFrame.fence.value)
  {
    if ( SUCCEEDED ( stagingFrame.fence->SetEventOnCompletion (
                     stagingFrame.fence.value,
                     stagingFrame.fence.event                 )
                   )
       )
    {
      // Event is automatically reset after this wait is released
      SK_WaitForSingleObject (stagingFrame.fence.event, INFINITE);
    }
  }

  SK_ComPtr <ID3D12Resource>                                                                     pBackBuffer;
  pSwapChain->GetBuffer (pSwapChain->GetCurrentBackBufferIndex (), IID_ID3D12Resource, (void **)&pBackBuffer.p);

  bool  SK_D3D12_IsBackBufferOnActiveQueue (ID3D12Resource *pResource, ID3D12CommandQueue *pCmdQueue, UINT iBackBufferIdx);
  if (! SK_D3D12_IsBackBufferOnActiveQueue (pBackBuffer.p, _pCommandQueue, swapIdx))
  {
    SK_LOGi0 (L"Attempting to Execute D3D12 Present (...) on wrong Command Queue, shutting down overlay!");

    _d3d12_rbk->release (pSwapChain);
    return;
  }

  // Screenshot may have left this in a recording state
  if (! stagingFrame.bCmdListRecording)
  {
    stagingFrame.pCmdAllocator->Reset ();

    if (! stagingFrame.begin_cmd_list ())
    {
      SK_ReleaseAssert (!"Command List Cannot Begin");
      return;
    }
  }

  const SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  ///bool hdr_display =
  ///  (rb.isHDRCapable () && (rb.framebuffer_flags & SK_FRAMEBUFFER_FLAG_HDR));

  SK_RunOnce (
  {
    // This level of state tracking is unnecessary normally
    if (config.render.dxgi.allow_d3d12_footguns)// || config.reshade.is_addon)
    {
      _InitDrawCommandHooks (pCommandList);
    }
  });

  static std::error_code ec;
  static bool inject_textures =
    std::filesystem::exists   (SK_Resource_GetRoot () / LR"(inject/textures/)", ec) &&
  (!std::filesystem::is_empty (SK_Resource_GetRoot () / LR"(inject/textures/)", ec));

  if (config.textures.dump_on_load || inject_textures)
  {
    SK_RunOnce (
      _InitCopyTextureRegionHook (pCommandList)
    );
  }

  bool bHDR =
    ( __SK_HDR_16BitSwap &&
       stagingFrame.hdr.pSwapChainCopy.p != nullptr &&
       stagingFrame.hdr.pSwapChainCopy.p->GetDesc ().Format == DXGI_FORMAT_R16G16B16A16_FLOAT &&
                        pHDRPipeline.p   != nullptr &&
                        pHDRSignature.p  != nullptr )  ||
    ( __SK_HDR_10BitSwap &&
       stagingFrame.hdr.pSwapChainCopy.p != nullptr &&
       stagingFrame.hdr.pSwapChainCopy.p->GetDesc ().Format == DXGI_FORMAT_R10G10B10A2_UNORM &&
                        pHDRPipeline.p   != nullptr &&
                        pHDRSignature.p  != nullptr )  ||
    // Implicit HDR processing if we redirected a call to CopyResource (...) during this frame
    //
    stagingFrame.hdr.skip_copy;

  auto _DrawAllReShadeEffects = [&](bool draw_first)
  {
    if ( config.reshade.is_addon                 &&
         config.reshade.draw_first == draw_first && _pReShadeRuntime != nullptr )
    {
      if (! draw_first)
      {
        // Split the command list (flush existing, begin new, draw ReShade in-between)
        if ( stagingFrame.flush_cmd_list () == false ||
             stagingFrame.begin_cmd_list () == false )
        {
          // Uh oh... skip ReShade.
          return;
        }
      }

      // Lazy final-initialization of ReShade for AddOn support
      //
      if (stagingFrame.hReShadeRTV.ptr == 0)
      {
        using namespace reshade::api;

        auto pBackBuffer =
          stagingFrame.pBackBuffer.p;

        reshade_format backbuffer_fmt =
          pBackBuffer->GetDesc ().Format, backbuffer_fmt_as_srgb =
                       DirectX::MakeSRGB (backbuffer_fmt);

        struct {
          device            *pDevice;
          resource           backbuffer;
          resource_view      rtv;
          resource_view      rtv_srgb;
          resource_view_desc rtv_desc;
        } reshade = { };
        
        reshade.pDevice    = _pReShadeRuntime->get_device ();
        reshade.backbuffer = RESHADE_MAKE_HANDLE (pBackBuffer);
        reshade.rtv_desc   =  resource_view_desc (backbuffer_fmt);

        reshade.pDevice->create_resource_view ( reshade.backbuffer,
                resource_usage::render_target,  reshade.rtv_desc,
                                               &reshade.rtv );

        D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle =
          (D3D12_CPU_DESCRIPTOR_HANDLE)(uintptr_t)reshade.rtv.handle;

        _pDevice->CopyDescriptorsSimple (
          1, stagingFrame.hBackBufferRTV, rtv_handle,
          D3D12_DESCRIPTOR_HEAP_TYPE_RTV
        );   stagingFrame.hReShadeRTV  =  rtv_handle;

        if (DirectX::IsSRGB (backbuffer_fmt))
        {
          SK_ReleaseAssert (!L"BUG: sRGB SwapChain RTVs Impossible in D3D12!");
        }

        else
        {
          if (DirectX::IsSRGB (backbuffer_fmt_as_srgb))
          {
            reshade.rtv_desc.format = backbuffer_fmt_as_srgb;

            reshade.pDevice->create_resource_view ( reshade.backbuffer,
                    resource_usage::render_target,  reshade.rtv_desc,
                                                   &reshade.rtv_srgb );

            D3D12_CPU_DESCRIPTOR_HANDLE rtv_srgb_handle =
              (D3D12_CPU_DESCRIPTOR_HANDLE)(uintptr_t)reshade.rtv_srgb.handle;

            _pDevice->CopyDescriptorsSimple (
              1, stagingFrame.hBackBufferRTV_sRGB, rtv_srgb_handle,
              D3D12_DESCRIPTOR_HEAP_TYPE_RTV
            );   stagingFrame.hReShadeRTV_sRGB  =  rtv_srgb_handle;
          }
        }

        //
        // Pulling the rug out from underneath things is a bad idea in DLSS3
        //   games, so stall all queued frames before proceeding.
        // 
        // Streamline has a second queue on its fake SwapChain, that SK cannot
        //   properly synchronize other than draining the entire queue.
        //  
        drain_queue ();
      }

      SK_ComQIPtr <IDXGISwapChain1>
                       pSwapChain1 (_pSwapChain);
      SK_ReleaseAssert(pSwapChain1.p != nullptr);

      UINT64 uiFenceVal =
        SK_ReShadeAddOn_RenderEffectsD3D12 (
          pSwapChain1.p, stagingFrame.pBackBuffer,
                         stagingFrame.reshade_fence,
                         stagingFrame.hReShadeRTV,
                         stagingFrame.hReShadeRTV_sRGB );

      if (uiFenceVal != 0)
      {
        _pCommandQueue->Wait ( stagingFrame.reshade_fence, uiFenceVal );
      }
    }
  };

  if (bHDR)
  {
    SK_RunOnce (
      _InitCopyTextureRegionHook (pCommandList)
    );

    // Don't let user disable HDR re-processing
    bool                                                                          _enable = false;
    pHDRPipeline->SetPrivateData (SKID_D3D12DisablePipelineState, sizeof (bool), &_enable);
    SK_RunOnce (_criticalVertexShaders.insert (pHDRPipeline));


    HDR_LUMINANCE
      cbuffer_luma = {
        {  __SK_HDR_Luma,
           __SK_HDR_Exp,
          (__SK_HDR_HorizCoverage / 100.0f) * 2.0f - 1.0f,
          (__SK_HDR_VertCoverage  / 100.0f) * 2.0f - 1.0f
        }
      };

    static HDR_COLORSPACE_PARAMS
      cbuffer_cspace                       = { };

      cbuffer_cspace.uiToneMapper          =   __SK_HDR_tonemap;
      cbuffer_cspace.hdrSaturation         =   __SK_HDR_Saturation;
      cbuffer_cspace.hdrGamutExpansion     =   __SK_HDR_Gamut;
      cbuffer_cspace.sdrLuminance_NonStd   =   __SK_HDR_user_sdr_Y * 1.0_Nits;
      cbuffer_cspace.sdrContentEOTF        =   __SK_HDR_Content_EOTF;
      cbuffer_cspace.visualFunc [0]        = (uint32_t)__SK_HDR_visualization;
      cbuffer_cspace.visualFunc [1]        = (uint32_t)__SK_HDR_10BitSwap ? 1 : 0;

      cbuffer_cspace.sdrLuminance_White    =
        std::max (1.0f, rb.displays [rb.active_display].hdr.white_level * 1.0_Nits);

      cbuffer_cspace.hdrLuminance_MaxAvg   = __SK_HDR_tonemap == 2 ?
                                      rb.working_gamut.maxAverageY != 0.0f ?
                                      rb.working_gamut.maxAverageY         : rb.display_gamut.maxAverageY
                                                                   :         rb.display_gamut.maxAverageY;
      cbuffer_cspace.hdrLuminance_MaxLocal = __SK_HDR_tonemap == 2 ?
                                      rb.working_gamut.maxLocalY != 0.0f ?
                                      rb.working_gamut.maxLocalY         : rb.display_gamut.maxLocalY
                                                                   :       rb.display_gamut.maxLocalY;
      cbuffer_cspace.hdrLuminance_Min      = rb.display_gamut.minY * 1.0_Nits;
      cbuffer_cspace.currentTime           = (float)SK_timeGetTime ();

      extern float                       __SK_HDR_ColorBoost;
      extern float                       __SK_HDR_PQBoost0;
      extern float                       __SK_HDR_PQBoost1;
      extern float                       __SK_HDR_PQBoost2;
      extern float                       __SK_HDR_PQBoost3;
      extern bool                        __SK_HDR_TonemapOverbright;

      // Pass-through, don't screw with overbright
      if (abs (__SK_HDR_Luma) == 1.0f)
      {
        __SK_HDR_TonemapOverbright = false;
      }

      cbuffer_cspace.pqBoostParams [0] = __SK_HDR_PQBoost0;
      cbuffer_cspace.pqBoostParams [1] = __SK_HDR_PQBoost1;
      cbuffer_cspace.pqBoostParams [2] = __SK_HDR_PQBoost2;
      cbuffer_cspace.pqBoostParams [3] = __SK_HDR_PQBoost3;
      cbuffer_cspace.colorBoost        = __SK_HDR_ColorBoost;
      cbuffer_cspace.tonemapOverbright = __SK_HDR_TonemapOverbright;


    if ( config.reshade.is_addon   &&
         config.reshade.draw_first == true )
    {
      // Draw ReShade before HDR image processing
      _DrawAllReShadeEffects (true);
    }

    // Optional (if not already performed by Streamline) SwapChain Backbuffer Copy
    //
    auto *copy_barrier =
      stagingFrame.hdr.barriers.zero_copy;

    pCommandList->EndQuery                          (queries.hdr.pHeap.p, D3D12_QUERY_TYPE_TIMESTAMP,
                                                          stagingFrame.iBufferIdx * 4                );
    if (! std::exchange (stagingFrame.hdr.skip_copy, false))
    {
      copy_barrier =                                      stagingFrame.hdr.barriers.process;
      // pBackBuffer is expected to promote from STATE_PRESENT to STATE_COPY_SOURCE without a barrier.
      pCommandList->CopyResource                    (     stagingFrame.hdr.pSwapChainCopy.p,
                                                          stagingFrame.       pBackBuffer.p          );
    }
    pCommandList->ResourceBarrier                   ( 2, copy_barrier                                );
    pCommandList->EndQuery                          (queries.hdr.pHeap.p, D3D12_QUERY_TYPE_TIMESTAMP,
                                                          stagingFrame.iBufferIdx * 4 + 1            );

    // Actual HDR Image Processing Begins Now
    pCommandList->EndQuery                          (queries.hdr.pHeap.p, D3D12_QUERY_TYPE_TIMESTAMP,
                                                          stagingFrame.iBufferIdx * 4 + 2            );
    pCommandList->SetGraphicsRootSignature          ( pHDRSignature                                  );
    pCommandList->SetPipelineState                  ( pHDRPipeline                                   );
    pCommandList->SetGraphicsRoot32BitConstants     ( 0, 4,  &cbuffer_luma,   0                      );
    pCommandList->SetGraphicsRoot32BitConstants     ( 1, 20, &cbuffer_cspace, 0                      );
    pCommandList->IASetPrimitiveTopology            ( D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP           );
    pCommandList->SetDescriptorHeaps                ( 1, &descriptorHeaps.pHDR.p                     );
    pCommandList->SetGraphicsRootDescriptorTable    ( 2,  stagingFrame.hdr.hSwapChainCopySRV.GPU     );
    pCommandList->OMSetRenderTargets                ( 1, &stagingFrame.hBackBufferRTV, FALSE,nullptr );
    pCommandList->RSSetViewports                    ( 1, &stagingFrame.hdr.vp                        );
    pCommandList->RSSetScissorRects                 ( 1, &stagingFrame.hdr.scissor                   );
    pCommandList->DrawInstanced                     ( 3, 1, 0, 0                                     );
    pCommandList->ResourceBarrier                   ( 1, stagingFrame.hdr.barriers.copy_end          );
    pCommandList->EndQuery                          (queries.hdr.pHeap.p, D3D12_QUERY_TYPE_TIMESTAMP,
                                                          stagingFrame.iBufferIdx * 4 + 3            );
    pCommandList->ResolveQueryData                  (queries.hdr.pHeap.p, D3D12_QUERY_TYPE_TIMESTAMP,
                                                          stagingFrame.iBufferIdx * 4, 4,
                                                     queries.hdr.pReadBack.p,
                                        sizeof (UINT64) * stagingFrame.iBufferIdx * 4                );

    if (stagingFrame.timer_fence->GetCompletedValue () >= stagingFrame.timer_fence.value)
    {
      UINT64      *timestamps = nullptr;
      D3D12_RANGE       range = { stagingFrame.iBufferIdx * sizeof (UINT64) * 4,
                                  stagingFrame.iBufferIdx * sizeof (UINT64) * 4 + sizeof (UINT64) * 3 };

      if (SUCCEEDED (queries.hdr.pReadBack->Map (0, &range, (void **)&timestamps)))
      {
        stagingFrame.hdr.timestamps.Start = timestamps [0];
        stagingFrame.hdr.timestamps.End   = timestamps [1] +
                          (timestamps [3] - timestamps [2]);

                                          range = { 0, 0 };
        queries.hdr.pReadBack->Unmap (0, &range);

        if ( UINT64      sync_value      =      stagingFrame.timer_fence.value + 1;
             SUCCEEDED (_pCommandQueue->Signal (stagingFrame.timer_fence,
                         sync_value))
           )
        {
          stagingFrame.timer_fence.value =
                         sync_value;
        }

        double dTotalRuntimeMs = 0.0;
        UINT   i               =   0;

        for ( i = 0 ; i < frames_.size () ; ++i )
        {
          dTotalRuntimeMs +=
            frames_ [i].hdr.timestamps.GetMilliseconds
              (GPUTimestampFreq);
        }

        extern double SK_D3D11_HDR_RuntimeMs;
                      SK_D3D11_HDR_RuntimeMs = dTotalRuntimeMs / static_cast <double> (i);
      }
    }

    if ( config.reshade.is_addon   &&
         config.reshade.draw_first == false )
    {
      // Draw ReShade after HDR image processing, but before SK's UI
      _DrawAllReShadeEffects (false);
    }
  }

  // Regular SDR Render Setup
  else
  {
    transition_state (pCommandList, stagingFrame.pBackBuffer, D3D12_RESOURCE_STATE_PRESENT,
                                                              D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                              D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

    // No HDR, so trigger ReShade now (draw_first is meaningless in this context)
    _DrawAllReShadeEffects (config.reshade.draw_first);
  }

  // Queue-up Pre-SK OSD Screenshots
  SK_Screenshot_ProcessQueue  (SK_ScreenshotStage::BeforeGameHUD, rb); // Before Game HUD (meaningless in D3D12)
  SK_Screenshot_ProcessQueue  (SK_ScreenshotStage::BeforeOSD,     rb);

#if 0
  if (config.nvidia.dlss.dump_buffers)
    SK_NGX12_DumpBuffers_DLSSG (pCommandList);
#endif

  extern void SK_D3D12_WriteResources (void);
              SK_D3D12_WriteResources ();

  SK_D3D12_CommitUploadQueue (pCommandList);
  SK_ImGui_DrawFrame         (0x00,nullptr);

  // Queue-up Post-SK OSD Screenshots  (If done here, will not include ReShade)
  SK_Screenshot_ProcessQueue  (SK_ScreenshotStage::PrePresent, rb);

  transition_state (pCommandList, stagingFrame.pBackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                            D3D12_RESOURCE_STATE_PRESENT,
                                                            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

  if (stagingFrame.flush_cmd_list ())
  {
    if (_pReShadeRuntime != nullptr)
    {
      _pReShadeRuntime->get_command_queue ()->wait (
        reshade::api::fence { (uint64_t)stagingFrame.fence.p },
                                        stagingFrame.fence.value
      );

      SK_ReShadeAddOn_UpdateAndPresentEffectRuntime (_pReShadeRuntime);
    }

    else if (config.reshade.is_addon)
    {
      // Lazy initialize the runtime so that we can hot-inject ReShade
      _pReShadeRuntime =
        SK_ReShadeAddOn_CreateEffectRuntime_D3D12 (_pDevice, _pCommandQueue, _pSwapChain);
    }

    stagingFrame.hdr.format_conversions = 0;
  }

  if (InterlockedCompareExchange (&reset_needed, 0, 1) == 1)
  {
    if (stagingFrame.wait_for_gpu ())
    {
      release (pSwapChain);

      if (nullptr != _pReShadeRuntime)
      {
        SK_ReShadeAddOn_DestroyEffectRuntime (
          std::exchange (_pReShadeRuntime, nullptr)
        );
      }
    }

    else
      WriteULongRelease (&reset_needed, 1);
  }

  SK_RunOnce (SK_ApplyQueuedHooks ());
}

bool
SK_D3D12_RenderCtx::FrameCtx::begin_cmd_list (const SK_ComPtr <ID3D12PipelineState>& state)
{
  if (pCmdList == nullptr)
    return false;

  if (bCmdListRecording)
  {
    if (state != nullptr) // Update pipeline state if requested
    {
      pCmdList->SetPipelineState (state.p);
    }

    return true;
  }

  // Reset command list using current command allocator and put it into the recording state
  bCmdListRecording =
    SUCCEEDED (pCmdList->Reset (pCmdAllocator.p, state.p));

  if (bCmdListRecording)
      pCmdList->SetPredication (nullptr, 0, D3D12_PREDICATION_OP_EQUAL_ZERO);

  return
    bCmdListRecording;
}

bool
SK_D3D12_RenderCtx::FrameCtx::exec_cmd_list (void)
{
  assert (bCmdListRecording);

  if (pCmdList == nullptr)
    return false;

  ID3D12CommandList* const cmd_lists [] = {
    pCmdList.p
  };

  // If we are doing this in the wrong order (i.e. during teardown),
  //   do not attempt to execute the command list, just close it.
  //
  //  Failure to skip execution will result in device removal

  UINT                                                 uiSize                 = sizeof (uintptr_t);
  ID3D12CommandQueue*                                           pParentQueue  = nullptr;
  pCmdList->GetPrivateData (SKID_D3D12ParentCmdQueue, &uiSize, &pParentQueue);
                                                       uiSize                 = sizeof (uintptr_t);  
  ID3D12Resource*                                               pParentBuffer = nullptr;
  pCmdList->GetPrivateData (SKID_D3D12BackbufferPtr,  &uiSize, &pParentBuffer);
                                                       uiSize                 = sizeof (UINT);
  UINT                                                          BufferIdx     = UINT_MAX-1;
  pCmdList->GetPrivateData (SKID_D3D12BackbufferIdx,  &uiSize, &BufferIdx);

  SK_ComPtr <ID3D12Resource>                                 pBackbuffer;
  SK_ComPtr <IDXGISwapChain>                                 pRealSwapChain;
  if (SK_slGetNativeInterface (pRoot->_pSwapChain, (void **)&pRealSwapChain.p) == sl::Result::eOk)
  {   _ExchangeProxyForNative (pRoot->_pSwapChain,           pRealSwapChain);
    pRealSwapChain->GetBuffer (BufferIdx, IID_ID3D12Resource, (void **)&pBackbuffer.p);
  }

  else if (pRoot->_pSwapChain != nullptr)
    pRoot->_pSwapChain->GetBuffer (BufferIdx, IID_ID3D12Resource, (void **)&pBackbuffer.p);

  if (pRoot->getCurrentBackBufferIndex () == BufferIdx   &&
                            pParentBuffer == pBackbuffer &&
      pRoot->frames_ [BufferIdx].pCmdList == pCmdList)
  {
    SK_LOGi4 (L"Drew (BufferIdx=%d)...", BufferIdx);

    if (FAILED (pCmdList->Close ()))
      return false;

    bCmdListRecording = false;

    pParentQueue->ExecuteCommandLists (ARRAYSIZE (cmd_lists), cmd_lists);

    return true;
  }

  SK_LOGi0 (L"SwapChain Backbuffer changed while command lists were recording!");

  _d3d12_rbk->release (pRoot->_pSwapChain.p);
  
  return false;
}

bool
SK_D3D12_RenderCtx::FrameCtx::flush_cmd_list (void)
{
  if (exec_cmd_list ()) // Implicit sync
  {
    if ( const UINT64 sync_value = fence.value + 1;
           SUCCEEDED ( pRoot->_pCommandQueue->Signal (
                                   fence.p,
                      sync_value              )
                     )
       )
    {
      fence.value =
       sync_value;
    }

    return true;
  }

  return false;
}

bool
SK_D3D12_RenderCtx::drain_queue (void) noexcept
{
  if (frames_.empty ())
    return true;

  bool success { true };

  for (auto& frame : frames_)
  {
    if (! frame.wait_for_gpu ())
    {
      success = false;
    }
  }

  return
    success;
}

bool
SK_D3D12_RenderCtx::FrameCtx::wait_for_gpu (void) noexcept
{
  // Flush command list, to avoid it still referencing resources that may be destroyed after this call
  if (bCmdListRecording)
  {
    if (! exec_cmd_list ())
      return false;
  }

  // Increment fence value to ensure it has not been signaled before
  const UINT64 sync_value =
    fence.value + 1;

  if (! fence.event)
    return false;

  if ( FAILED (
         pRoot->_pCommandQueue->Signal (
           fence.p, sync_value         )
              )
     )
  {
    return false; // Cannot wait on fence if signaling was not successful
  }

  if ( SUCCEEDED (
         fence->SetEventOnCompletion (
           sync_value, fence.event   )
                 )
     )
  {
    SK_WaitForSingleObject (fence.event, INFINITE);
  }

  // Update CPU side fence value now that it is guaranteed to have come through
  fence.value =
    sync_value;


  //
  // ReShade Fencing
  //

  // Increment fence value to ensure it has not been signaled before
  const UINT64 sync_value0 =
    reshade_fence.value + 1;

  if (! reshade_fence.event)
    return false;

  if ( FAILED (
         pRoot->_pCommandQueue->Signal (
           reshade_fence , sync_value0 )
              )
     )
  {
    return false; // Cannot wait on fence if signaling was not successful
  }

  if ( SUCCEEDED (
         reshade_fence->SetEventOnCompletion (
           sync_value0, reshade_fence.event  )
                 )
     )
  {
    SK_WaitForSingleObject (reshade_fence.event, INFINITE);
  }

  // Update CPU side fence value now that it is guaranteed to have come through
  reshade_fence.value =
    sync_value0;


  return true;
}

void
SK_SEH_FrameCtxDtor (SK_D3D12_RenderCtx::FrameCtx *pFrameCtx) noexcept
{
  __try {
    pFrameCtx->pCmdList.Release           ();
    pFrameCtx->pCmdAllocator.Release      ();
    pFrameCtx->bCmdListRecording     = false;

    pFrameCtx->pBackBuffer.Release        ();
    pFrameCtx->hdr.pSwapChainCopy.Release ();

    pFrameCtx->reshade_fence.Release ();
    pFrameCtx->reshade_fence.value = 0;

    pFrameCtx->fence.Release ();
    pFrameCtx->fence.value = 0;
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    SK_LOGi0 (
      L"COM Exception Raised during SK_D3D12_RenderCtx::FrameCtx dtor"
    );
  }
}

SK_D3D12_RenderCtx::FrameCtx::~FrameCtx (void)
{
  // Execute and wait for any cmds on the current pending swap,
  //   everything else can be destroyed with no sync.
  if (                            this->pRoot          == nullptr ||
       ! SK_SAFE_ValidatePointer (this->pRoot->_pSwapChain, true) ||
                                  this->pRoot->getCurrentBackBufferIndex () != iBufferIdx )
  {
    bCmdListRecording = false;
  }

  wait_for_gpu        (    );
  SK_SEH_FrameCtxDtor (this);

  if (fence.event != nullptr)
  {
    SK_CloseHandle (fence.event);
                    fence.event = nullptr;
  }

  if (reshade_fence.event != nullptr)
  {
    SK_CloseHandle (reshade_fence.event);
                    reshade_fence.event = nullptr;
  }

  if (timer_fence.event != nullptr)
  {
    SK_CloseHandle (timer_fence.event);
                    timer_fence.event = nullptr;
  }
}

#include <d3d12sdklayers.h>
#include <SpecialK/render/dxgi/dxgi_swapchain.h>
#include <SpecialK/plugin/reshade.h>

//std::recursive_mutex SK_D3D12_RenderCtx::_ctx_lock;

void
SK_D3D12_RenderCtx::release (IDXGISwapChain *pSwapChain)
{
  //std::scoped_lock lock (_ctx_lock);

  drain_queue ();

  if (! ((_pSwapChain.p != nullptr && pSwapChain == nullptr) ||
         (_pSwapChain.p == nullptr && pSwapChain != nullptr) ||
          _pSwapChain.IsEqualObject  (pSwapChain)            ))
  {
    SK_LOGi0 (
      L"Unexpected SwapChain (%p) encountered in SK_D3D12_RenderCtx::release (...); expected %p!",
                  pSwapChain,
                 _pSwapChain.p
    );

    SK_ReShadeAddOn_CleanupRTVs (SK_ReShadeAddOn_GetRuntimeForSwapChain (_pSwapChain.p), true);
  }

  if (_pSwapChain.p == nullptr && _pDevice.p == nullptr)
  {
    SK_LOGi1 (L"Releasing D3D12 Render Context that was never initialized!");
  }

  SK_ReShadeAddOn_CleanupRTVs (SK_ReShadeAddOn_GetRuntimeForSwapChain (pSwapChain), true);

  if (SK_IsDebuggerPresent ())
  {
    SK_ComQIPtr <ID3D12DebugDevice>
                      pDebugDevice (_pDevice.p);

    if (pDebugDevice)
        pDebugDevice->ReportLiveDeviceObjects ( D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL|
                                                D3D12_RLDO_IGNORE_INTERNAL );
  }

  SK_D3D12_EndFrame (SK_TLS_Bottom ());


  if (_pReShadeRuntime != nullptr)
  {
    SK_ReShadeAddOn_DestroyEffectRuntime (_pReShadeRuntime);
                                          _pReShadeRuntime = nullptr;
  }

  ImGui_ImplDX12_Shutdown ();

  ///// 1 frame delay for re-init
  ///frame_delay.fetch_add (1);

  // Steam overlay is releasing references to the SwapChain it did not acquire (!!)
  if (! SK_ValidatePointer (_pSwapChain.p, true))
                            _pSwapChain.p = nullptr;

  if (! SK_ValidatePointer (_pDevice.p, true))
                            _pDevice.p = nullptr;

  frames_.clear ();

  // Do this after closing the command lists (frames_.clear ())
  pHDRPipeline.Release                        ();
  pHDRSignature.Release                       ();

  computeCopy.pPipeline.Release               ();
  computeCopy.pSignature.Release              ();
  computeCopy.dlssg_fence.Release             ();
  computeCopy.dlssg_fence.value = 0;

  descriptorHeaps.pBackBuffers.Release        ();
//descriptorHeaps.pImGui.Release              ();
  descriptorHeaps.pHDR.Release                ();
  descriptorHeaps.pHDR_CopyAssist_SRV.Release ();
  descriptorHeaps.pHDR_CopyAssist_RTV.Release ();
  descriptorHeaps.pComputeCopy.Release        ();

//_pCommandQueue.Release ();
  _pSwapChain.Release ();
  _pDevice.Release    ();
}

bool
SK_D3D12_RenderCtx::init (IDXGISwapChain3 *pSwapChain, ID3D12CommandQueue *pCommandQueue)
{
  //std::scoped_lock lock (_ctx_lock);

  SK_ComPtr <IDXGISwapChain3>                        pNativeSwapChain;
  if (SK_slGetNativeInterface (pSwapChain, (void **)&pNativeSwapChain.p) == sl::Result::eOk)
                               pSwapChain =          pNativeSwapChain.p;

  if (pSwapChain != nullptr)
  {
    UINT  uiSize    = sizeof (void *);
    void *pCmdQueue = nullptr;

    if (SUCCEEDED (pSwapChain->GetPrivateData (SKID_D3D12_SwapChainCommandQueue, &uiSize, pCmdQueue)))
    {
      if (pCmdQueue != nullptr)
      {
        pCommandQueue = (ID3D12CommandQueue *)pCmdQueue;
      }
    }
  }

  SK_ComPtr <ID3D12CommandQueue>                        pNativeQueue;
  if (SK_slGetNativeInterface (pCommandQueue, (void **)&pNativeQueue.p) == sl::Result::eOk)
  {
    if (                       _pCommandQueue != nullptr)
      _ExchangeProxyForNative (_pCommandQueue,          pNativeQueue)
    else                       _pCommandQueue         = pNativeQueue;
  }

  if (pNativeSwapChain != nullptr &&
      pNativeQueue.p   != nullptr)
  {
    UINT uiSize = sizeof (void *);

    pSwapChain->SetPrivateData (SKID_D3D12_SwapChainCommandQueue, uiSize, _pCommandQueue);
  }

  // Turn HDR off in dgVoodoo2 so it does not crash
#ifdef _M_IX86
  if ( (! config.render.dxgi.allow_d3d12_footguns ) &&
       ( __SK_HDR_16BitSwap || __SK_HDR_10BitSwap ) )
  {
    SK_HDR_DisableOverridesForGame ();
    SK_RestartGame                 ();
  }
#endif

  // This is the first time we've seen this device (unless something really funky's going on)
  if ( _pCommandQueue.p == nullptr &&
        pCommandQueue   != nullptr )
  {
    _pCommandQueue =
     pCommandQueue;
  }


  // Delay (re-)init for issues with Ubisoft games
  if ( frame_delay.fetch_sub (1) > 0 ) return false;
  else frame_delay.exchange  (0);


  if (_pDevice.p == nullptr || pNativeQueue.p != nullptr)
  {
    // TODO: Figure out why 32-bit D3D12 crashes when
    // pNativeDev12 is placed below _pCommandQueue...
    SK_ComPtr <ID3D12Device> pNativeDev12;

    if ( _pCommandQueue.p != nullptr && FAILED (
           _pCommandQueue->GetDevice ( IID_PPV_ARGS (&_pDevice.p) )
                                               )
       )
    {
      return false;
    }

    if (SK_slGetNativeInterface (_pDevice.p, (void **)&pNativeDev12.p) == sl::Result::eOk)
        _ExchangeProxyForNative (_pDevice,             pNativeDev12);
  }

  if (_pDevice.p != nullptr)
  {
    _pSwapChain =
     pSwapChain;

    SK_RenderBackend& rb =
      SK_GetCurrentRenderBackend ();

    if (rb.swapchain == nullptr || rb.swapchain != pSwapChain || rb.d3d12.command_queue != _pCommandQueue || rb.d3d12.device != _pDevice)
    {
      // This is unexpected, but may happen if a game destroys its original window
      //   and then creates a new SwapChain
      if (rb.swapchain != nullptr)
      {
        HWND hWnd = (HWND)-1;

        SK_ComQIPtr <IDXGISwapChain1>
                         pSwapChain1 (rb.swapchain.p);
        if (             pSwapChain1.p != nullptr )
                         pSwapChain1->GetHwnd (&hWnd);

#if 1   // Likley the device is always the same due to D3D12 adapters being singletons
        if (! IsWindow (hWnd))
        {
          SK_LOGi0 (
            L"# Transitioning active ImGui SwapChain from %p to %p because game window was destroyed",
              rb.swapchain.p, pSwapChain );

          _pDevice       = nullptr;
          _pCommandQueue = pCommandQueue;

          if (_pCommandQueue != nullptr)
          {   _pCommandQueue->GetDevice (IID_PPV_ARGS (&_pDevice.p));

            SK_ComPtr <ID3D12Device>                         pNativeDev12;
            if (SK_slGetNativeInterface (_pDevice, (void **)&pNativeDev12.p) == sl::Result::eOk)
                _ExchangeProxyForNative (_pDevice,           pNativeDev12);
          }
        }
#endif
      }

      rb.swapchain           = pSwapChain;
      rb.device              = _pDevice.p;
      rb.d3d12.command_queue = _pCommandQueue.p;
      rb.api                 = SK_RenderAPI::D3D12;
    }
  }

  if (! _pSwapChain.p)
  {
    return false;
  }

  else if (_pDevice != nullptr && _pSwapChain != nullptr)
  {
    frames_.clear ();

    DXGI_SWAP_CHAIN_DESC1  swapDesc1 = { };
    pSwapChain->GetDesc1 (&swapDesc1);

    SK_ReleaseAssert (swapDesc1.BufferCount > 0);
    frames_.resize   (swapDesc1.BufferCount); // Resize does not clear, if the size is the same

    try
    {
#ifdef SK_D3D12_PERSISTENT_IMGUI_DEV_OBJECTS
      if (descriptorHeaps.pImGui.p == nullptr)
#endif
      {
        ThrowIfFailed (
          _pDevice->CreateDescriptorHeap (
            std::array < D3D12_DESCRIPTOR_HEAP_DESC,                1 >
              {          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,    1024,
                         D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 }.data (),
                                         IID_PPV_ARGS (&descriptorHeaps.pImGui.p)));
                                SK_D3D12_SetDebugName ( descriptorHeaps.pImGui.p,
                                          L"ImGui D3D12 DescriptorHeap" );

        sk_d3d12_texture_s::num_textures = 0;
      }
      ThrowIfFailed (
        _pDevice->CreateDescriptorHeap (
          std::array < D3D12_DESCRIPTOR_HEAP_DESC,                1 >
            {          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,    swapDesc1.BufferCount * 2,
                       D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 }.data (),
                                       IID_PPV_ARGS (&descriptorHeaps.pHDR.p)));
                              SK_D3D12_SetDebugName ( descriptorHeaps.pHDR.p,
                                        L"SK D3D12 HDR DescriptorHeap" );

      ThrowIfFailed (
        _pDevice->CreateDescriptorHeap (
          std::array < D3D12_DESCRIPTOR_HEAP_DESC,                1 >
            {          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,    swapDesc1.BufferCount * 2,
                       D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 }.data (),
                                       IID_PPV_ARGS (&descriptorHeaps.pHDR_CopyAssist_SRV.p)));
                              SK_D3D12_SetDebugName ( descriptorHeaps.pHDR_CopyAssist_SRV.p,
                              L"SK D3D12 HDR Copy SRV DescriptorHeap" );
                              
      ThrowIfFailed (
        _pDevice->CreateDescriptorHeap (
          std::array < D3D12_DESCRIPTOR_HEAP_DESC,                1 >
            {          D3D12_DESCRIPTOR_HEAP_TYPE_RTV,            swapDesc1.BufferCount * 2,
                       D3D12_DESCRIPTOR_HEAP_FLAG_NONE,           0 }.data (),
                                       IID_PPV_ARGS (&descriptorHeaps.pHDR_CopyAssist_RTV.p)));
                              SK_D3D12_SetDebugName ( descriptorHeaps.pHDR_CopyAssist_RTV.p,
                              L"SK D3D12 HDR Copy RTV DescriptorHeap" );

      ThrowIfFailed (
        _pDevice->CreateDescriptorHeap (
          std::array < D3D12_DESCRIPTOR_HEAP_DESC,                1 >
            {          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,    std::max (swapDesc1.BufferCount * 32, 64U),
                       D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 }.data (),
                                       IID_PPV_ARGS (&descriptorHeaps.pComputeCopy.p)));
                              SK_D3D12_SetDebugName ( descriptorHeaps.pComputeCopy.p,
                              L"SK D3D12 Compute Copy DescriptorHeap" );

      // This heap will store Linear and sRGB views, it needs 2x SwapChain backbuffer
      ThrowIfFailed (
        _pDevice->CreateDescriptorHeap (
          std::array < D3D12_DESCRIPTOR_HEAP_DESC,                1 >
            {          D3D12_DESCRIPTOR_HEAP_TYPE_RTV,            swapDesc1.BufferCount * 2,
                       D3D12_DESCRIPTOR_HEAP_FLAG_NONE,           0 }.data (),
                                       IID_PPV_ARGS (&descriptorHeaps.pBackBuffers.p)));
                              SK_D3D12_SetDebugName ( descriptorHeaps.pBackBuffers.p,
                                L"SK D3D12 Backbuffer DescriptorHeap" );

      ThrowIfFailed (
        _pDevice->CreateQueryHeap (
          std::array < D3D12_QUERY_HEAP_DESC,                     1 >
            {          D3D12_QUERY_HEAP_TYPE_TIMESTAMP,           swapDesc1.BufferCount * 4,
                                                                  0 }.data (),
                                       IID_PPV_ARGS (&queries.hdr.pHeap.p)));
                              SK_D3D12_SetDebugName ( queries.hdr.pHeap.p,
                                        L"SK D3D12 HDR Timer Query Heap" );

      ThrowIfFailed (
        _pDevice->CreateQueryHeap (
          std::array < D3D12_QUERY_HEAP_DESC,                     1 >
            {          D3D12_QUERY_HEAP_TYPE_TIMESTAMP,           swapDesc1.BufferCount * 2,
                                                                  0 }.data (),
                                    IID_PPV_ARGS (&queries.reshade.pHeap.p)));
                           SK_D3D12_SetDebugName ( queries.reshade.pHeap.p,
                                     L"SK D3D12 ReShade Timer Query Heap" );

      ThrowIfFailed (
        _pDevice->CreateQueryHeap (
          std::array < D3D12_QUERY_HEAP_DESC,                     1 >
            {          D3D12_QUERY_HEAP_TYPE_TIMESTAMP,           swapDesc1.BufferCount * 2,
                                                                  0 }.data (),
                                    IID_PPV_ARGS (&queries.dlssg.pHeap.p)));
                           SK_D3D12_SetDebugName ( queries.dlssg.pHeap.p,
                                     L"SK D3D12 DLSSG Timer Query Heap" );

      ThrowIfFailed (
        _pDevice->CreateCommittedResource (
          std::array < D3D12_HEAP_PROPERTIES, 1 >  {
                       D3D12_HEAP_TYPE_READBACK, D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                                                 D3D12_MEMORY_POOL_UNKNOWN,
                                           0, 1    }.data (),
                       D3D12_HEAP_FLAG_NONE,
          std::array < D3D12_RESOURCE_DESC,   1 > {
                       D3D12_RESOURCE_DIMENSION_BUFFER,
                                       0,
            sizeof (UINT64) * swapDesc1.BufferCount * 4, 1,
                                       1,                1,
                      DXGI_FORMAT_UNKNOWN, { 1, 0 }, D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
                                                     D3D12_RESOURCE_FLAG_NONE }.data (),
                         D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                              IID_PPV_ARGS (&queries.hdr.pReadBack.p)));
                                     SK_D3D12_SetDebugName ( queries.hdr.pReadBack.p,
                                               L"SK D3D12 HDR Timer Query Readback Buffer" );

      ThrowIfFailed (
        _pDevice->CreateCommittedResource (
          std::array < D3D12_HEAP_PROPERTIES, 1 >  {
                       D3D12_HEAP_TYPE_READBACK, D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                                                 D3D12_MEMORY_POOL_UNKNOWN,
                                           0, 1    }.data (),
                       D3D12_HEAP_FLAG_NONE,
          std::array < D3D12_RESOURCE_DESC,   1 > {
                       D3D12_RESOURCE_DIMENSION_BUFFER,
                                       0,
            sizeof (UINT64) * swapDesc1.BufferCount * 2, 1,
                                       1,                1,
                      DXGI_FORMAT_UNKNOWN, { 1, 0 }, D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
                                                     D3D12_RESOURCE_FLAG_NONE }.data (),
                         D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                              IID_PPV_ARGS (&queries.reshade.pReadBack.p)));
                                     SK_D3D12_SetDebugName ( queries.reshade.pReadBack.p,
                                               L"SK D3D12 ReShade Timer Query Readback Buffer" );

      ThrowIfFailed (
        _pDevice->CreateCommittedResource (
          std::array < D3D12_HEAP_PROPERTIES, 1 >  {
                       D3D12_HEAP_TYPE_READBACK, D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                                                 D3D12_MEMORY_POOL_UNKNOWN,
                                           0, 1    }.data (),
                       D3D12_HEAP_FLAG_NONE,
          std::array < D3D12_RESOURCE_DESC,   1 > {
                       D3D12_RESOURCE_DIMENSION_BUFFER,
                                       0,
            sizeof (UINT64) * swapDesc1.BufferCount * 2, 1,
                                       1,                1,
                      DXGI_FORMAT_UNKNOWN, { 1, 0 }, D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
                                                     D3D12_RESOURCE_FLAG_NONE }.data (),
                         D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                              IID_PPV_ARGS (&queries.dlssg.pReadBack.p)));
                                     SK_D3D12_SetDebugName ( queries.dlssg.pReadBack.p,
                                               L"SK D3D12 DLSSG Timer Query Readback Buffer" );

      const size_t rtvDescriptorSize =
        _pDevice->GetDescriptorHandleIncrementSize (D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
      const size_t srvDescriptorSize =
        _pDevice->GetDescriptorHandleIncrementSize (D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

      auto         rtvHandle         =
        descriptorHeaps.pBackBuffers->GetCPUDescriptorHandleForHeapStart ();

      int bufferIdx = 0;

      _pCommandQueue->GetTimestampFrequency (&GPUTimestampFreq);

      ThrowIfFailed (
        _pDevice->CreateFence (0, D3D12_FENCE_FLAG_NONE,            IID_PPV_ARGS (&computeCopy.dlssg_fence.p)));

      for ( auto& frame : frames_ )
      {
        frame.iBufferIdx =
               bufferIdx++;

        ThrowIfFailed (
          _pDevice->CreateFence (0, D3D12_FENCE_FLAG_NONE,          IID_PPV_ARGS (&frame.fence.p)));
        ThrowIfFailed (
          _pDevice->CreateFence (0, D3D12_FENCE_FLAG_NONE,          IID_PPV_ARGS (&frame.reshade_fence.p)));
        ThrowIfFailed (
          _pDevice->CreateFence (0, D3D12_FENCE_FLAG_NONE,          IID_PPV_ARGS (&frame.timer_fence.p)));
        ThrowIfFailed (
          _pDevice->CreateCommandAllocator (
                                    D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS (&frame.pCmdAllocator.p)));

        SK_ComQIPtr <ID3D12Device4>
                          pDevice4 (_pDevice);

        if (! pDevice4) {
          ThrowIfFailed (
            _pDevice->CreateCommandList ( 0,
                                    D3D12_COMMAND_LIST_TYPE_DIRECT,                frame.pCmdAllocator.p, nullptr,
                                                                    IID_PPV_ARGS (&frame.pCmdList.p)));
          ThrowIfFailed (                                                          frame.pCmdList->Close ());
        } else {
          ThrowIfFailed ( pDevice4->CreateCommandList1 ( 0,
                                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                                    D3D12_COMMAND_LIST_FLAG_NONE,   IID_PPV_ARGS (&frame.pCmdList.p)));
        }

        ThrowIfFailed (
          _pSwapChain->GetBuffer (frame.iBufferIdx,                 IID_PPV_ARGS (&frame.pBackBuffer.p)));

        void SK_D3D12_TrackResource (ID3D12Device *pDevice, ID3D12Resource *pResource, UINT iBufferIdx);
             SK_D3D12_TrackResource (_pDevice, frame.pBackBuffer.p, frame.iBufferIdx);

        frame.pCmdList->SetPrivateData (SKID_D3D12ParentCmdQueue, sizeof (uintptr_t), &_pCommandQueue.p);
        frame.pCmdList->SetPrivateData (SKID_D3D12BackbufferPtr,  sizeof (uintptr_t), &frame.pBackBuffer.p);
        frame.pCmdList->SetPrivateData (SKID_D3D12BackbufferIdx,  sizeof (UINT),      &frame.iBufferIdx);

        frame.hBackBufferRTV.ptr =
                  rtvHandle.ptr + ((size_t)frame.iBufferIdx * (size_t)2)             * rtvDescriptorSize;
        frame.hBackBufferRTV_sRGB.ptr =
                  rtvHandle.ptr + ((size_t)frame.iBufferIdx * (size_t)2 + (size_t)1) * rtvDescriptorSize;

        //
        // Wrap this using ReShade's internal device, so that it can map the CPU descriptor back to a resource
        //   and we can use this Render Target in non-Render Hook versions of ReShade.
        //
        if (config.reshade.is_addon || _pReShadeRuntime != nullptr)
        {
          if (config.reshade.is_addon && _pReShadeRuntime == nullptr)
          {
            // Lazy initialize the runtime so that we can hot-inject ReShade
            _pReShadeRuntime =
              SK_ReShadeAddOn_CreateEffectRuntime_D3D12 (_pDevice, _pCommandQueue, _pSwapChain);
          }
        }

        // Hookless ReShade is not in use, do the normal thing.
        else
        {
          _pDevice->CreateRenderTargetView ( frame.pBackBuffer, nullptr,
                                             frame.hBackBufferRTV );
        }

        ThrowIfFailed (
          _pDevice->CreateCommittedResource (
            std::array < D3D12_HEAP_PROPERTIES, 1 >  {
                         D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                                                  D3D12_MEMORY_POOL_UNKNOWN,
                                              0, 1   }.data (),
                         D3D12_HEAP_FLAG_NONE,
            std::array < D3D12_RESOURCE_DESC,   1 > {
                         D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                                         0,
                           swapDesc1.Width, swapDesc1.Height,
                                         1,                1,
                           swapDesc1.Format, { 1, 0 }, D3D12_TEXTURE_LAYOUT_UNKNOWN,
                                                       D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET |
                                                       D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS }.data (),
                           D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                IID_PPV_ARGS (&frame.hdr.pSwapChainCopy.p)));

        frame.hdr.vp.Width       = static_cast <FLOAT> (swapDesc1.Width);
        frame.hdr.vp.Height      = static_cast <FLOAT> (swapDesc1.Height);
        frame.hdr.scissor.right  =                      swapDesc1.Width;
        frame.hdr.scissor.bottom =                      swapDesc1.Height;

        frame.hdr.hSwapChainCopySRV.CPU.ptr =
          descriptorHeaps.pHDR->GetCPUDescriptorHandleForHeapStart ().ptr +
                                   srvDescriptorSize * frame.iBufferIdx;

        frame.hdr.hSwapChainCopySRV.GPU.ptr =
          descriptorHeaps.pHDR->GetGPUDescriptorHandleForHeapStart ().ptr +
                                   srvDescriptorSize * frame.iBufferIdx;

        frame.hdr.hBufferCopy.SRV.CPU.ptr =
          descriptorHeaps.pHDR_CopyAssist_SRV->GetCPUDescriptorHandleForHeapStart ().ptr +
                                                  srvDescriptorSize * frame.iBufferIdx;

        frame.hdr.hBufferCopy.SRV.GPU.ptr =
          descriptorHeaps.pHDR_CopyAssist_SRV->GetGPUDescriptorHandleForHeapStart ().ptr +
                                                  srvDescriptorSize * frame.iBufferIdx;

        frame.hdr.hBufferCopy.RTV.CPU.ptr =
          descriptorHeaps.pHDR_CopyAssist_RTV->GetCPUDescriptorHandleForHeapStart ().ptr +
                                                  rtvDescriptorSize * frame.iBufferIdx;

        // RTVs don't have GPU descriptor handles
        frame.hdr.hBufferCopy.RTV.GPU.ptr = 0;

        _pDevice->CreateShaderResourceView ( frame.hdr.pSwapChainCopy.p, nullptr,
                                             frame.hdr.hSwapChainCopySRV.CPU
        );

        // As long as HDR processing comes first, we can implicitly transition the swapchain from
        //   STATE_PRESENT to D3D12_RESOURCE_STATE_COPY_SOURCE
        frame.hdr.barriers.copy_end  [0].Transition.pResource = frame.hdr.pSwapChainCopy.p;
        frame.hdr.barriers.process   [0].Transition.pResource = frame.hdr.pSwapChainCopy.p;
        frame.hdr.barriers.process   [1].Transition.pResource =        frame.pBackBuffer.p;
        frame.hdr.barriers.zero_copy [1].Transition.pResource =        frame.pBackBuffer.p;
        frame.hdr.barriers.zero_copy [0].Transition.pResource = frame.hdr.pSwapChainCopy.p;
        frame.hdr.skip_copy = false;

        frame.pRoot         = this;

        frame.fence.value   = 0;
        frame.fence.event   =
          SK_CreateEvent ( nullptr, FALSE, FALSE, nullptr );

        frame.reshade_fence.value = 0;
        frame.reshade_fence.event =
          SK_CreateEvent ( nullptr, FALSE, FALSE, nullptr );

        struct {        ID3D12Object *pObj;
                 const std::wstring   kName;
        } _debugObjects [] =
          {
            { frame.pBackBuffer.p,        L"SK D3D12 SwapChain Buffer" },
            { frame.hdr.pSwapChainCopy.p, L"SK D3D12 HDR Copy Buffer"  },
            { frame.pCmdAllocator.p,      L"SK D3D12 CmdAllocator"     },
            { frame.pCmdList.p,           L"SK D3D12 CmdList"          },
            { frame.fence.p,              L"SK D3D12 Fence"            },
            { frame.reshade_fence.p,      L"SK D3D12 ReShade Fence"    },
          };

        for ( auto& _obj : _debugObjects )
        {
          SK_D3D12_SetDebugName (
            _obj.pObj,
            _obj.kName + std::to_wstring (frame.iBufferIdx)
          );
        }

        frame.bCmdListRecording = false;
      }

      // Create the root signature
      D3D12_ROOT_PARAMETER
        param [3]                                     = { };

        param [0].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        param [0].Constants.Num32BitValues            = 4;// cbuffer vertexBuffer : register (b0)
        param [0].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_VERTEX;

        param [1].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        param [1].Constants.Num32BitValues            = 20;// cbuffer colorSpaceTransform : register (b0)
        param [1].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;

      D3D12_DESCRIPTOR_RANGE
        srv_range                                     = { };
        srv_range.RangeType                           = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srv_range.NumDescriptors                      = 2;
        srv_range.BaseShaderRegister                  = 0; // t0, t1 (texLastFrame0)

        param [2].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param [2].DescriptorTable.NumDescriptorRanges = 1;
        param [2].DescriptorTable.pDescriptorRanges   = &srv_range;
        param [2].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;

      D3D12_STATIC_SAMPLER_DESC
        staticSampler                                 = { };
        staticSampler.AddressU                        = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSampler.AddressV                        = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSampler.AddressW                        = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSampler.ComparisonFunc                  = D3D12_COMPARISON_FUNC_NEVER;
        staticSampler.ShaderVisibility                = D3D12_SHADER_VISIBILITY_PIXEL;

      D3D12_ROOT_SIGNATURE_DESC
        desc                                          = { };
        desc.NumParameters                            = _countof (param);
        desc.pParameters                              =           param;
        desc.NumStaticSamplers                        =  1;
        desc.pStaticSamplers                          = &staticSampler;

        desc.Flags                                    =
        ( D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS       |
          D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS     |
          D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS );

      SK_ComPtr <ID3DBlob>
                    pBlob;

      ThrowIfFailed (
        D3D12SerializeRootSignature (
          &desc, D3D_ROOT_SIGNATURE_VERSION_1,
            &pBlob.p, nullptr )     );

      ThrowIfFailed (
        _pDevice->CreateRootSignature (
          0, pBlob->GetBufferPointer (),
             pBlob->GetBufferSize    (),
                 IID_PPV_ARGS (&pHDRSignature.p))
      ); SK_D3D12_SetDebugName (pHDRSignature.p, L"SK HDR Root Signature");

      D3D12_GRAPHICS_PIPELINE_STATE_DESC
        psoDesc                                  = { };
        psoDesc.NodeMask                         = 1;
        psoDesc.PrimitiveTopologyType            = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.pRootSignature                   = pHDRSignature;
        psoDesc.SampleMask                       = UINT_MAX;
        psoDesc.NumRenderTargets                 = 1;
        psoDesc.RTVFormats [0]                   = swapDesc1.Format;
        psoDesc.SampleDesc.Count                 = 1;

        psoDesc.VS = { colorutil_vs_bytecode,
               sizeof (colorutil_vs_bytecode) /
               sizeof (colorutil_vs_bytecode) [0]
                     };

        psoDesc.PS = { uber_hdr_shader_ps_bytecode,
               sizeof (uber_hdr_shader_ps_bytecode) /
               sizeof (uber_hdr_shader_ps_bytecode) [0]
                     };

      // Create the blending setup
      D3D12_BLEND_DESC&
        blendDesc                                = psoDesc.BlendState;
        blendDesc.
          RenderTarget [0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

      // Create the rasterizer state
      D3D12_RASTERIZER_DESC&
        rasterDesc                               = psoDesc.RasterizerState;
        rasterDesc.FillMode                      = D3D12_FILL_MODE_SOLID;
        rasterDesc.CullMode                      = D3D12_CULL_MODE_NONE;

      // Create depth-stencil State
      D3D12_DEPTH_STENCIL_DESC&
        depthDesc                                = psoDesc.DepthStencilState;
        depthDesc.DepthFunc                      = D3D12_COMPARISON_FUNC_NEVER;
        depthDesc.FrontFace.StencilFailOp        =
        depthDesc.FrontFace.StencilDepthFailOp   =
        depthDesc.FrontFace.StencilPassOp        = D3D12_STENCIL_OP_KEEP;
        depthDesc.FrontFace.StencilFunc          = D3D12_COMPARISON_FUNC_NEVER;
        depthDesc.BackFace                       =
        depthDesc.FrontFace;

      ThrowIfFailed (
        _pDevice->CreateGraphicsPipelineState (
          &psoDesc, IID_PPV_ARGS (&pHDRPipeline.p)
       ));SK_D3D12_SetDebugName  ( pHDRPipeline.p, L"SK HDR Pipeline State" );

        // This is optional, and not compatible with 32-bit software.
#ifndef _M_IX86
      D3D12_DESCRIPTOR_RANGE
        uav_range                                     = { };
        uav_range.RangeType                           = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        uav_range.NumDescriptors                      = 2;
        uav_range.BaseShaderRegister                  = 0; // u0, u1

        param [0]                                     = { };
        param [0].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param [0].DescriptorTable.NumDescriptorRanges = 1;
        param [0].DescriptorTable.pDescriptorRanges   = &uav_range;
        param [0].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;

        desc                                          = { };
        desc.NumParameters                            = 1;
        desc.pParameters                              = param;

      ThrowIfFailed (
        D3D12SerializeRootSignature (
          &desc, D3D_ROOT_SIGNATURE_VERSION_1,
            &pBlob.p, nullptr )     );

      ThrowIfFailed (
        _pDevice->CreateRootSignature (
          0, pBlob->GetBufferPointer (),
             pBlob->GetBufferSize    (),
                 IID_PPV_ARGS (&computeCopy.pSignature.p))
      ); SK_D3D12_SetDebugName (computeCopy.pSignature.p, L"SK Compute Copy Root Signature");

      D3D12_COMPUTE_PIPELINE_STATE_DESC
        csoDesc                = { };
        csoDesc.NodeMask       =  1 ;
        csoDesc.CS             = { blt_copy_cs_bytecode,
                           sizeof (blt_copy_cs_bytecode) /
                           sizeof (blt_copy_cs_bytecode) [0]
                                 };
        csoDesc.pRootSignature = computeCopy.pSignature;

      ThrowIfFailed (
        _pDevice->CreateComputePipelineState (
          &csoDesc, IID_PPV_ARGS (&computeCopy.pPipeline.p)
       ));SK_D3D12_SetDebugName  ( computeCopy.pPipeline.p, L"SK Compute Copy Pipeline State" );
#endif

      HWND                   hWnd = HWND_BROADCAST;
      _pSwapChain->GetHwnd (&hWnd);

      // Re-apply colorspace if necessary

      // {018B57E4-1493-4953-ADF2-DE6D99CC05E5}
      static constexpr GUID SKID_SwapChainColorSpace =
      { 0x18b57e4, 0x1493, 0x4953, { 0xad, 0xf2, 0xde, 0x6d, 0x99, 0xcc, 0x5, 0xe5 } };

      UINT                  uiColorSpaceSize = sizeof (DXGI_COLOR_SPACE_TYPE);
      DXGI_COLOR_SPACE_TYPE csp              = DXGI_COLOR_SPACE_RESERVED;

      // Since SwapChains don't have a Get method, we'll just have the SwapChain remember the last one
      //   set and check it for consistency... set a colorspace override if necessary.
      if (SUCCEEDED (pSwapChain->GetPrivateData (SKID_SwapChainColorSpace, &uiColorSpaceSize, &csp)))
      {
        SK_ComQIPtr <IDXGISwapChain3>
                         pSwapChain3
                        (pSwapChain);

        // Invoke our own hook using the private data, in case an external application has set this,
        //  that way we can handle HDR correctly even if the hook on SetColorSpace1 isn't working.
        if (pSwapChain3 != nullptr)
            pSwapChain3->SetColorSpace1 (csp);
      }

      if (
        ImGui_ImplDX12_Init ( _pDevice,
                              swapDesc1.BufferCount,
                              swapDesc1.Format,
          descriptorHeaps.pImGui->GetCPUDescriptorHandleForHeapStart (),
          descriptorHeaps.pImGui->GetGPUDescriptorHandleForHeapStart (), hWnd)
         )
      {
        ImGui_ImplDX12_CreateDeviceObjects ();

        return true;
      }
    }

    catch (const SK_ComException& e)
    {
      SK_LOG0 ( ( L"SK D3D12 Init Failed: %hs", e.what () ),
                  __SK_SUBSYSTEM__ );
    }
  }

  return false;
}

SK_LazyGlobal <SK_D3D12_RenderCtx> _d3d12_rbk;
