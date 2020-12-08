// dear imgui: Renderer for DirectX12
// This needs to be used along with a Platform Binding (e.g. Win32)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'D3D12_GPU_DESCRIPTOR_HANDLE' as ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.
// Issues:
//  [ ] 64-bit only for now! (Because sizeof(ImTextureId) == sizeof(void*)). See github.com/ocornut/imgui/pull/301

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2018-06-12: DirectX12: Moved the ID3D12GraphicsCommandList* parameter from NewFrame() to RenderDrawData().
//  2018-06-08: Misc: Extracted imgui_impl_dx12.cpp/.h away from the old combined DX12+Win32 example.
//  2018-06-08: DirectX12: Use draw_data->DisplayPos and draw_data->DisplaySize to setup projection matrix and clipping rectangle (to ease support for future multi-viewport).
//  2018-02-22: Merged into master with all Win32 code synchronized to other examples.

#include <SpecialK/stdafx.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_d3d12.h>
#include <SpecialK/render/backend.h>
#include <SpecialK/render/dxgi/dxgi_backend.h>



#include <shaders/imgui_d3d11_vs.h>
#include <shaders/imgui_d3d11_ps.h>

#include <SpecialK/render/dxgi/dxgi_hdr.h>

#include <shaders/vs_colorutil.h>
#include <shaders/uber_hdr_shader_ps.h>

#include <DirectXTex/d3dx12.h>

#define D3D12_RESOURCE_STATE_SHADER_RESOURCE \
	(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)

// DirectX
//#include <dxgi1_4.h>

// DirectX data
static ID3D12Device*                   g_pd3dDevice            = nullptr;
static SK_ComPtr <ID3D12RootSignature> g_pRootSignature        = nullptr;
static SK_ComPtr <ID3D12PipelineState> g_pPipelineState        = nullptr;
static DXGI_FORMAT                     g_RTVFormat             = DXGI_FORMAT_UNKNOWN;
static SK_ComPtr <ID3D12Resource>      g_pFontTextureResource  = nullptr;
static D3D12_CPU_DESCRIPTOR_HANDLE     g_hFontSrvCpuDescHandle = {};
static D3D12_GPU_DESCRIPTOR_HANDLE     g_hFontSrvGpuDescHandle = {};


    extern float __SK_HDR_Luma;
    extern float __SK_HDR_Exp;
    extern float __SK_HDR_Saturation;
    extern float __SK_HDR_HorizCoverage;
    extern float __SK_HDR_VertCoverage;
    extern int   __SK_HDR_tonemap;
    extern int   __SK_HDR_visualization;
    extern int   __SK_HDR_Bypass_sRGB;
    extern float __SK_HDR_PaperWhite;
    extern float __SK_HDR_user_sdr_Y;

struct FrameResources
{
  SK_ComPtr <ID3D12Resource> IB;
  SK_ComPtr <ID3D12Resource> VB;
  int VertexBufferSize;
  int IndexBufferSize;
};
std::vector <FrameResources> g_FrameResources;

struct VERTEX_CONSTANT_BUFFER
{
  float   mvp[4][4];

  // scRGB allows values > 1.0, sRGB (SDR) simply clamps them
  float luminance_scale [4]; // For HDR displays,    1.0 = 80 Nits
                             // For SDR displays, >= 1.0 = 80 Nits
  float steam_luminance [4];
};

static UINT64 _frame = 0;

void ImGui_ImplDX12_RenderDrawData (ImDrawData* draw_data, SK_D3D12_RenderCtx::FrameCtx* pFrame)
{
  FrameResources* frameResources =
    &g_FrameResources [_frame % g_FrameResources.size ()];

  SK_ComPtr <ID3D12GraphicsCommandList> ctx =
    pFrame->pCmdList;

  SK_ComPtr <ID3D12Resource> g_pVB = frameResources->VB;
  SK_ComPtr <ID3D12Resource> g_pIB = frameResources->IB;

  int g_VertexBufferSize = frameResources->VertexBufferSize;
  int g_IndexBufferSize  = frameResources->IndexBufferSize;

  // Create and grow vertex/index buffers if needed
  if (! g_pVB.p || g_VertexBufferSize < draw_data->TotalVtxCount)
  {
    pFrame->wait_for_gpu ();

    if (g_pVB.p)
        g_pVB.Release ();

    g_VertexBufferSize = draw_data->TotalVtxCount + 5000;

    D3D12_HEAP_PROPERTIES
      props                      = { };
      props.Type                 = D3D12_HEAP_TYPE_UPLOAD;
      props.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
      props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC
      desc                  = { };
      desc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
      desc.Width            = g_VertexBufferSize * sizeof (ImDrawVert);
      desc.Height           = 1;
      desc.DepthOrArraySize = 1;
      desc.MipLevels        = 1;
      desc.Format           = DXGI_FORMAT_UNKNOWN;
      desc.SampleDesc.Count = 1;
      desc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      desc.Flags            = D3D12_RESOURCE_FLAG_NONE;

    if (g_pd3dDevice->CreateCommittedResource (&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&g_pVB)) < 0)
      return;

    frameResources->VB               = g_pVB;
    frameResources->VertexBufferSize = g_VertexBufferSize;
  }

  if (! g_pIB.p || g_IndexBufferSize < draw_data->TotalIdxCount)
  {
    pFrame->wait_for_gpu ();

    if (g_pIB.p)
        g_pIB.Release ();

    g_IndexBufferSize = draw_data->TotalIdxCount + 10000;

    D3D12_HEAP_PROPERTIES
      props                      = { };
      props.Type                 = D3D12_HEAP_TYPE_UPLOAD;
      props.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
      props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC
      desc                  = { };
      desc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
      desc.Width            = g_IndexBufferSize * sizeof (ImDrawIdx);
      desc.Height           = 1;
      desc.DepthOrArraySize = 1;
      desc.MipLevels        = 1;
      desc.Format           = DXGI_FORMAT_UNKNOWN;
      desc.SampleDesc.Count = 1;
      desc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      desc.Flags            = D3D12_RESOURCE_FLAG_NONE;

      if (g_pd3dDevice->CreateCommittedResource (&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&g_pIB)) < 0)
        return;

    frameResources->IB              = g_pIB;
    frameResources->IndexBufferSize = g_IndexBufferSize;
  }

  // Copy and convert all vertices into a single contiguous buffer
  void *vtx_resource,
       *idx_resource;

  D3D12_RANGE range = { };

  if (g_pVB->Map (0, &range, &vtx_resource) != S_OK)
    return;

  if (g_pIB->Map (0, &range, &idx_resource) != S_OK)
    return;

  ImDrawVert* vtx_dst = (ImDrawVert *)vtx_resource;
  ImDrawIdx*  idx_dst = (ImDrawIdx  *)idx_resource;

  for (int n = 0; n < draw_data->CmdListsCount; n++)
  {
    const ImDrawList*
      cmd_list = draw_data->CmdLists [n];

    memcpy (vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof (ImDrawVert));
    memcpy (idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof (ImDrawIdx));

    vtx_dst += cmd_list->VtxBuffer.Size;
    idx_dst += cmd_list->IdxBuffer.Size;
  }

  g_pVB->Unmap (0, &range);
  g_pIB->Unmap (0, &range);

  ImGuiIO& io =
    ImGui::GetIO ();


  if (! pFrame->begin_cmd_list ())
    return;

  ctx->SetGraphicsRootSignature     (g_pRootSignature);
  ctx->SetPipelineState             (g_pPipelineState);

  ctx->OMSetRenderTargets (1, &pFrame->hRenderOutput,  FALSE,  nullptr);
  ctx->SetDescriptorHeaps (1, &pFrame->pRoot->descriptorHeaps.pImGui.p);


  //
  // HDR STUFF
  //
  auto& rb =
    SK_GetCurrentRenderBackend ();

  bool hdr_display =
    (rb.isHDRCapable () && (rb.framebuffer_flags & SK_FRAMEBUFFER_FLAG_HDR));


  // Setup orthographic projection matrix into our constant buffer
  // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right).
  VERTEX_CONSTANT_BUFFER vertex_constant_buffer = { };
  {
    VERTEX_CONSTANT_BUFFER* constant_buffer =
      &vertex_constant_buffer;

    float L = 0.0f;
    float R = 0.0f + io.DisplaySize.x;
    float T = 0.0f;
    float B = 0.0f + io.DisplaySize.y;
    float mvp[4][4] =
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
        (eotf == SK_RenderBackend::scan_out_s::SMPTE_2084);

      constant_buffer->luminance_scale [0] = ( bEOTF_is_PQ ? -80.0f * rb.ui_luminance :
                                                                      rb.ui_luminance );
      constant_buffer->luminance_scale [1] = 2.2f;
      constant_buffer->luminance_scale [2] = rb.display_gamut.minY * 1.0_Nits;
      constant_buffer->steam_luminance [0] = ( bEOTF_is_PQ ? -80.0f * config.steam.overlay_hdr_luminance :
                                                                      config.steam.overlay_hdr_luminance );
      constant_buffer->steam_luminance [1] = 2.2f;//( bEOTF_is_PQ ? 1.0f : (rb.ui_srgb ? 2.2f :
                                                  //                                     1.0f));
      constant_buffer->steam_luminance [2] = ( bEOTF_is_PQ ? -80.0f * config.uplay.overlay_luminance :
                                                                      config.uplay.overlay_luminance );
      constant_buffer->steam_luminance [3] = 2.2f;//( bEOTF_is_PQ ? 1.0f : (rb.ui_srgb ? 2.2f :
                                                  //                1.0f));
    }
  }

  // Setup viewport
  ctx->RSSetViewports (         1,
    std::array <D3D12_VIEWPORT, 1> (
    {             0.0f, 0.0f,
      io.DisplaySize.x, io.DisplaySize.y,
                  0.0f, 1.0f
    }                              ).data ()
  );

  // Bind shader and vertex buffers
  UINT                      stride = sizeof (ImDrawVert);
  D3D12_GPU_VIRTUAL_ADDRESS offset = 0;

  ctx->IASetVertexBuffers ( 0,            1,
    std::array <D3D12_VERTEX_BUFFER_VIEW, 1> (
    {   g_pVB->GetGPUVirtualAddress () + offset,
              (UINT)g_VertexBufferSize * stride,
                                         stride
    }                                        ).data ()
  );

  ctx->IASetIndexBuffer (
    std::array <D3D12_INDEX_BUFFER_VIEW, 1> (
    {         g_pIB->GetGPUVirtualAddress (),
      g_IndexBufferSize * sizeof (ImDrawIdx),
                          sizeof (ImDrawIdx) == 2 ?
                             DXGI_FORMAT_R16_UINT :
                             DXGI_FORMAT_R32_UINT
    }                                       ).data ()
  );

  ctx->IASetPrimitiveTopology        (D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  ctx->SetGraphicsRoot32BitConstants (0, 24, &vertex_constant_buffer, 0);
  ctx->SetGraphicsRoot32BitConstants ( 2, 4,
                       std::array <float, 4> (
      { 0.0f, 0.0f,
        hdr_display ? (float)io.DisplaySize.x : 0.0f,
        hdr_display ? (float)io.DisplaySize.y : 0.0f }
                                             ).data (),
                                          0
  );

  // Setup render state
  static const float     blend_factor [4] = { 0.f, 0.f, 0.f, 0.f };
  ctx->OMSetBlendFactor (blend_factor);

  // Render command lists
  int    vtx_offset = 0;
  int    idx_offset = 0;
  ImVec2 pos        = ImVec2 (0.0f, 0.0f);

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
        pcmd->UserCallback (cmd_list, pcmd);
      }

      else
      {
        const D3D12_RECT r =
        {
          (LONG)(pcmd->ClipRect.x - pos.x), (LONG)(pcmd->ClipRect.y - pos.y),
          (LONG)(pcmd->ClipRect.z - pos.x), (LONG)(pcmd->ClipRect.w - pos.y)
        };

        ctx->SetGraphicsRootDescriptorTable ( 1,
              *(D3D12_GPU_DESCRIPTOR_HANDLE*)&pcmd->TextureId    );
        ctx->RSSetScissorRects              ( 1,              &r );
        ctx->DrawIndexedInstanced           ( pcmd->ElemCount, 1,
                                              idx_offset,
                                              vtx_offset,      0 );
      }

      idx_offset +=
        pcmd->ElemCount;
    }

    vtx_offset +=
      cmd_list->VtxBuffer.Size;
  }
}

static void
ImGui_ImplDX12_CreateFontsTexture (void)
{
  // Build texture atlas
  ImGuiIO& io =
    ImGui::GetIO ();

  static bool           init   = false;
  static unsigned char* pixels = nullptr;
  static int            width  = 0,
                        height = 0;

  // Only needs to be done once, the raw pixels are API agnostic
  if (! init)
  {
    SK_ImGui_LoadFonts ();

    io.Fonts->GetTexDataAsRGBA32 ( &pixels,
                                   &width, &height );
  }


  SK_ComPtr <ID3D12Resource>            pTexture;
  SK_ComPtr <ID3D12Resource>            uploadBuffer;

  SK_ComPtr <ID3D12Fence>               pFence;

  SK_ComPtr <ID3D12CommandQueue>        cmdQueue;
  SK_ComPtr <ID3D12CommandAllocator>    cmdAlloc;
  SK_ComPtr <ID3D12GraphicsCommandList> cmdList;


  // Upload texture to graphics system
  {
    D3D12_HEAP_PROPERTIES
      props                      = { };
      props.Type                 = D3D12_HEAP_TYPE_DEFAULT;
      props.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
      props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC
      desc                       = { };
      desc.Dimension             = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
      desc.Alignment             = 0;
      desc.Width                 = width;
      desc.Height                = height;
      desc.DepthOrArraySize      = 1;
      desc.MipLevels             = 1;
      desc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
      desc.SampleDesc.Count      = 1;
      desc.SampleDesc.Quality    = 0;
      desc.Layout                = D3D12_TEXTURE_LAYOUT_UNKNOWN;
      desc.Flags                 = D3D12_RESOURCE_FLAG_NONE;

    g_pd3dDevice->CreateCommittedResource(
      &props, D3D12_HEAP_FLAG_NONE,
      &desc,  D3D12_RESOURCE_STATE_COPY_DEST,
       nullptr, IID_PPV_ARGS (&pTexture.p)
    );

    UINT uploadPitch = (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u)
                                & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
    UINT uploadSize  = height * uploadPitch;

    desc.Dimension             = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment             = 0;
    desc.Width                 = uploadSize;
    desc.Height                = 1;
    desc.DepthOrArraySize      = 1;
    desc.MipLevels             = 1;
    desc.Format                = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count      = 1;
    desc.SampleDesc.Quality    = 0;
    desc.Layout                = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags                 = D3D12_RESOURCE_FLAG_NONE;

    props.Type                 = D3D12_HEAP_TYPE_UPLOAD;
    props.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    HRESULT hr =
      g_pd3dDevice->CreateCommittedResource( &props, D3D12_HEAP_FLAG_NONE, &desc,
                                                     D3D12_RESOURCE_STATE_GENERIC_READ,
                                             nullptr, IID_PPV_ARGS (&uploadBuffer.p) );
    IM_ASSERT (SUCCEEDED (hr));

    void *mapped = nullptr;

    D3D12_RANGE range =
     { 0, uploadSize };

    hr =
      uploadBuffer->Map (0, &range, &mapped);

    IM_ASSERT (SUCCEEDED (hr));

    for ( int y = 0; y < height; y++ )
    {
      memcpy ( (void*) ((uintptr_t) mapped + y * uploadPitch),
                                    pixels + y * width * 4,
                                                 width * 4 );
    }

    uploadBuffer->Unmap (0, &range);

    D3D12_TEXTURE_COPY_LOCATION
      srcLocation                                    = { };
      srcLocation.pResource                          = uploadBuffer;
      srcLocation.Type                               = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
      srcLocation.PlacedFootprint.Footprint.Format   = DXGI_FORMAT_R8G8B8A8_UNORM;
      srcLocation.PlacedFootprint.Footprint.Width    = width;
      srcLocation.PlacedFootprint.Footprint.Height   = height;
      srcLocation.PlacedFootprint.Footprint.Depth    = 1;
      srcLocation.PlacedFootprint.Footprint.RowPitch = uploadPitch;

    D3D12_TEXTURE_COPY_LOCATION
      dstLocation                  = { };
      dstLocation.pResource        = pTexture;
      dstLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
      dstLocation.SubresourceIndex = 0;

    D3D12_RESOURCE_BARRIER
      barrier                        = { };
      barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
      barrier.Transition.pResource   = pTexture;
      barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
      barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    hr =
      g_pd3dDevice->CreateFence (0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS (&pFence.p));

    IM_ASSERT (SUCCEEDED (hr));

    HANDLE hEvent =
      SK_CreateEvent (nullptr, FALSE, FALSE, nullptr);

    IM_ASSERT (hEvent != nullptr);

    D3D12_COMMAND_QUEUE_DESC
      queueDesc          = { };
      queueDesc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
      queueDesc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
      queueDesc.NodeMask = 1;

    hr =
      g_pd3dDevice->CreateCommandQueue (&queueDesc, IID_PPV_ARGS (&cmdQueue.p));

    IM_ASSERT (SUCCEEDED (hr));

    hr =
      g_pd3dDevice->CreateCommandAllocator (D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS (&cmdAlloc.p));

    IM_ASSERT (SUCCEEDED (hr));

    hr =
      g_pd3dDevice->CreateCommandList (0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc, NULL, IID_PPV_ARGS (&cmdList.p));

    IM_ASSERT (SUCCEEDED (hr));

    cmdList->CopyTextureRegion (&dstLocation, 0, 0, 0, &srcLocation, NULL);
    cmdList->ResourceBarrier   (1, &barrier);

    hr =
      cmdList->Close ();

    IM_ASSERT (SUCCEEDED (hr));

    cmdQueue->ExecuteCommandLists (1, (ID3D12CommandList* const*) &cmdList);

    hr =
      cmdQueue->Signal (pFence, 1);

    IM_ASSERT (SUCCEEDED (hr));

    pFence->SetEventOnCompletion (1, hEvent);
    WaitForSingleObject          (   hEvent, INFINITE);

    cmdList.Release      ();
    cmdAlloc.Release     ();
    cmdQueue.Release     ();
    CloseHandle          (hEvent);
    pFence.Release       ();
    uploadBuffer.Release ();

    // Create texture view
    D3D12_SHADER_RESOURCE_VIEW_DESC
      srvDesc                           = { };
      srvDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
      srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
      srvDesc.Texture2D.MipLevels       = desc.MipLevels;
      srvDesc.Texture2D.MostDetailedMip = 0;
      srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    g_pd3dDevice->CreateShaderResourceView (
      pTexture, &srvDesc,
        g_hFontSrvCpuDescHandle
    );

    g_pFontTextureResource =
      pTexture;

    init = true;
  }

#if 0
  // Store our identifier
  static_assert (sizeof(ImTextureID) >= sizeof(g_hFontSrvGpuDescHandle.ptr), \
                 "Can't pack descriptor handle into TexID, 32-bit not supported yet.");
#endif

  io.Fonts->TexID =
    (ImTextureID)g_hFontSrvGpuDescHandle.ptr;
}

bool
ImGui_ImplDX12_CreateDeviceObjects (void)
{
  if (! g_pd3dDevice)
    return false;

  if (g_pPipelineState)
    ImGui_ImplDX12_InvalidateDeviceObjects ();

  // Create the root signature
  {
    D3D12_DESCRIPTOR_RANGE
      descRange                                   = { };
      descRange.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      descRange.NumDescriptors                    = 1;
      descRange.BaseShaderRegister                = 0;
      descRange.RegisterSpace                     = 0;
      descRange.OffsetInDescriptorsFromTableStart = 0;

    D3D12_ROOT_PARAMETER
      param [3]                                     = { };

      param [0].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
      param [0].Constants.ShaderRegister            = 0;
      param [0].Constants.RegisterSpace             = 0;
      param [0].Constants.Num32BitValues            = 24;//16;
      param [0].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_VERTEX;

      param [1].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      param [1].DescriptorTable.NumDescriptorRanges = 1;
      param [1].DescriptorTable.pDescriptorRanges   = &descRange;
      param [1].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;

      param [2].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	    param [2].Constants.ShaderRegister            = 0; // b0
	    param [2].Constants.Num32BitValues            = 4;
	    param [2].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC
      staticSampler = { };

      staticSampler.Filter           = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
      staticSampler.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
      staticSampler.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
      staticSampler.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
      staticSampler.MipLODBias       = 0.f;
      staticSampler.MaxAnisotropy    = 0;
      staticSampler.ComparisonFunc   = D3D12_COMPARISON_FUNC_ALWAYS;
      staticSampler.BorderColor      = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
      staticSampler.MinLOD           = 0.f;
      staticSampler.MaxLOD           = 0.f;
      staticSampler.ShaderRegister   = 0;
      staticSampler.RegisterSpace    = 0;
      staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

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

    ID3DBlob*
      pBlob = nullptr;

    typedef
      HRESULT (*D3D12SerializeRootSignature_pfn)(
                 const D3D12_ROOT_SIGNATURE_DESC   *pRootSignature,
                       D3D_ROOT_SIGNATURE_VERSION   Version,
                       ID3DBlob                   **ppBlob,
                       ID3DBlob                   **ppErrorBlob);

    static D3D12SerializeRootSignature_pfn
           D3D12SerializeRootSignature =
          (D3D12SerializeRootSignature_pfn)
             SK_GetProcAddress ( L"d3d12.dll",
                                  "D3D12SerializeRootSignature" );

    if ( FAILED ( D3D12SerializeRootSignature ( &desc,
                                                  D3D_ROOT_SIGNATURE_VERSION_1,
                                                    &pBlob, nullptr ) ) )
    {
      return false;
    }

    g_pd3dDevice->CreateRootSignature (
      0, pBlob->GetBufferPointer (),
         pBlob->GetBufferSize    (),
           IID_PPV_ARGS (&g_pRootSignature)
    );
    pBlob->Release ();
  }

  D3D12_GRAPHICS_PIPELINE_STATE_DESC
    psoDesc                       = { };
    psoDesc.NodeMask              = 1;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.pRootSignature        = g_pRootSignature;
    psoDesc.SampleMask            = UINT_MAX;
    psoDesc.NumRenderTargets      = 1;
    psoDesc.RTVFormats [0]        = g_RTVFormat;
    psoDesc.SampleDesc.Count      = 1;
    psoDesc.Flags                 = D3D12_PIPELINE_STATE_FLAG_NONE;

    psoDesc.VS = {
                    imgui_d3d11_vs_bytecode,
            sizeof (imgui_d3d11_vs_bytecode) /
            sizeof (imgui_d3d11_vs_bytecode) [0]
                  };

    // Create the input layout
    static D3D12_INPUT_ELEMENT_DESC local_layout[] = {
      { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0, static_cast<UINT>((size_t)(&((ImDrawVert*)0)->pos)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
      { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, static_cast<UINT>((size_t)(&((ImDrawVert*)0)->uv)),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
      { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, static_cast<UINT>((size_t)(&((ImDrawVert*)0)->col)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    psoDesc.InputLayout =
    { local_layout, 3 };

    psoDesc.PS = { imgui_d3d11_ps_bytecode,
           sizeof (imgui_d3d11_ps_bytecode) /
           sizeof (imgui_d3d11_ps_bytecode) [0]
    };

  // Create the blending setup
  {
    D3D12_BLEND_DESC&
      desc                                        = psoDesc.BlendState;
      desc.AlphaToCoverageEnable                  = false;
      desc.RenderTarget [0].BlendEnable           = true;
      desc.RenderTarget [0].SrcBlend              = D3D12_BLEND_SRC_ALPHA;
      desc.RenderTarget [0].DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
      desc.RenderTarget [0].BlendOp               = D3D12_BLEND_OP_ADD;
      desc.RenderTarget [0].SrcBlendAlpha         = D3D12_BLEND_ONE;//INV_SRC_ALPHA;
      desc.RenderTarget [0].DestBlendAlpha        = D3D12_BLEND_ZERO;
      desc.RenderTarget [0].BlendOpAlpha          = D3D12_BLEND_OP_ADD;
      desc.RenderTarget [0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
  }

  // Create the rasterizer state
  {
    D3D12_RASTERIZER_DESC&
      desc                       = psoDesc.RasterizerState;
      desc.FillMode              = D3D12_FILL_MODE_SOLID;
      desc.CullMode              = D3D12_CULL_MODE_NONE;
      desc.FrontCounterClockwise = FALSE;
      desc.DepthBias             = D3D12_DEFAULT_DEPTH_BIAS;
      desc.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
      desc.SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
      desc.DepthClipEnable       = true;
      desc.MultisampleEnable     = FALSE;
      desc.AntialiasedLineEnable = FALSE;
      desc.ForcedSampleCount     = 0;
      desc.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
  }

  // Create depth-stencil State
  {
    D3D12_DEPTH_STENCIL_DESC&
      desc                              = psoDesc.DepthStencilState;
      desc.DepthEnable                  = false;
      desc.DepthWriteMask               = D3D12_DEPTH_WRITE_MASK_ALL;
      desc.DepthFunc                    = D3D12_COMPARISON_FUNC_ALWAYS;
      desc.StencilEnable                = false;
      desc.FrontFace.StencilFailOp      =
      desc.FrontFace.StencilDepthFailOp =
      desc.FrontFace.StencilPassOp      = D3D12_STENCIL_OP_KEEP;
      desc.FrontFace.StencilFunc        = D3D12_COMPARISON_FUNC_ALWAYS;
      desc.BackFace                     =
      desc.FrontFace;
  }

  if ( FAILED ( g_pd3dDevice->CreateGraphicsPipelineState    (
                  &psoDesc, IID_PPV_ARGS (&g_pPipelineState) ) )
     )
  {
    return false;
  }

  ImGui_ImplDX12_CreateFontsTexture ();

  return true;
}

void
ImGui_ImplDX12_InvalidateDeviceObjects (void)
{
  if (! g_pd3dDevice)
    return;

  if (g_pRootSignature)
      g_pRootSignature.Release ();
  if (g_pPipelineState)
      g_pPipelineState.Release ();

  // We copied g_pFontTextureView to io.Fonts->TexID so let's clear that as well.
  if (g_pFontTextureResource) {
      g_pFontTextureResource.Release ();
      ImGui::GetIO ().Fonts->TexID = NULL;
  }

  for ( auto& frame : g_FrameResources )
  {
    if (frame.IB)
        frame.IB.Release ();
    if (frame.VB)
        frame.VB.Release ();
  }
}

bool
ImGui_ImplDX12_Init ( ID3D12Device*               device,
                      int                         num_frames_in_flight,
                      DXGI_FORMAT                 rtv_format,
                      D3D12_CPU_DESCRIPTOR_HANDLE font_srv_cpu_desc_handle,
                      D3D12_GPU_DESCRIPTOR_HANDLE font_srv_gpu_desc_handle )
{
#ifndef NDEBUG
  dll_log->Log (L"[DX12Init] Device: %p, Frames in Flight: %lu, Format: %lu", device, num_frames_in_flight, rtv_format);
#endif

  g_pd3dDevice            = device;
  g_RTVFormat             = rtv_format;
  g_hFontSrvCpuDescHandle = font_srv_cpu_desc_handle;
  g_hFontSrvGpuDescHandle = font_srv_gpu_desc_handle;
  g_FrameResources.resize (num_frames_in_flight * 2);

  // Create buffers with a default size (they will later be grown as needed)
  for ( auto& frame : g_FrameResources )
  {
    frame.IB               = nullptr;
    frame.VB               = nullptr;
    frame.VertexBufferSize = 5000;
    frame.IndexBufferSize  = 10000;
  }

  return true;
}

void
ImGui_ImplDX12_Shutdown (void)
{
#ifndef NDEBUG
  dll_log->Log (L"[DX12Shutdown] Device: %p, Frames in Flight: %lu, Format: %lu", g_pd3dDevice, g_numFramesInFlight, g_RTVFormat);
#endif

  ImGui_ImplDX12_InvalidateDeviceObjects ();

  g_FrameResources.clear ();

  g_pd3dDevice                = NULL;
  g_hFontSrvCpuDescHandle.ptr = 0;
  g_hFontSrvGpuDescHandle.ptr = 0;
}

void
SK_ImGui_User_NewFrame (void);

// Data
static INT64                    g_Time                  = 0;
static INT64                    g_TicksPerSecond        = 0;

#include <SpecialK/window.h>

void
SK_ImGui_PollGamepad (void);


void
ImGui_ImplDX12_NewFrame (void)
{
  ///if (! g_pPipelineState)
  ///  ImGui_ImplDX12_CreateDeviceObjects ();
  ///
  ///
  ///if (g_pd3dDevice == nullptr || g_hFontSrvCpuDescHandle.ptr == 0 || g_hFontSrvGpuDescHandle.ptr == 0)
  ///  return;


  static bool first = true;

  if (first)
  {
    g_TicksPerSecond  =
      SK_GetPerfFreq ( ).QuadPart;
    g_Time            =
      SK_QueryPerf   ( ).QuadPart;

    first = false;

    ImGuiIO& io =
      ImGui::GetIO ();

    io.KeyMap [ImGuiKey_Tab]        = VK_TAB;
    io.KeyMap [ImGuiKey_LeftArrow]  = VK_LEFT;
    io.KeyMap [ImGuiKey_RightArrow] = VK_RIGHT;
    io.KeyMap [ImGuiKey_UpArrow]    = VK_UP;
    io.KeyMap [ImGuiKey_DownArrow]  = VK_DOWN;
    io.KeyMap [ImGuiKey_PageUp]     = VK_PRIOR;
    io.KeyMap [ImGuiKey_PageDown]   = VK_NEXT;
    io.KeyMap [ImGuiKey_Home]       = VK_HOME;
    io.KeyMap [ImGuiKey_End]        = VK_END;
    io.KeyMap [ImGuiKey_Insert]     = VK_INSERT;
    io.KeyMap [ImGuiKey_Delete]     = VK_DELETE;
    io.KeyMap [ImGuiKey_Backspace]  = VK_BACK;
    io.KeyMap [ImGuiKey_Space]      = VK_SPACE;
    io.KeyMap [ImGuiKey_Enter]      = VK_RETURN;
    io.KeyMap [ImGuiKey_Escape]     = VK_ESCAPE;
    io.KeyMap [ImGuiKey_A]          = 'A';
    io.KeyMap [ImGuiKey_C]          = 'C';
    io.KeyMap [ImGuiKey_V]          = 'V';
    io.KeyMap [ImGuiKey_X]          = 'X';
    io.KeyMap [ImGuiKey_Y]          = 'Y';
    io.KeyMap [ImGuiKey_Z]          = 'Z';
  }


  // Setup time step
  INT64 current_time;

  SK_QueryPerformanceCounter (
    reinterpret_cast <LARGE_INTEGER *> (&current_time)
  );

  auto& io =
    ImGui::GetIO ();

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if (! rb.device)
    return;

  io.DeltaTime =
    std::min ( 1.0f,
    std::max ( 0.0f, static_cast <float> (
                    (static_cast <long double> (                       current_time) -
                     static_cast <long double> (std::exchange (g_Time, current_time))) /
                     static_cast <long double> (               g_TicksPerSecond      ) ) )
    );

  // Read keyboard modifiers inputs
  io.KeyCtrl   = (io.KeysDown [VK_CONTROL]) != 0;
  io.KeyShift  = (io.KeysDown [VK_SHIFT])   != 0;
  io.KeyAlt    = (io.KeysDown [VK_MENU])    != 0;

  io.KeySuper  = false;

  SK_ImGui_PollGamepad ();

  //// Start the frame
  SK_ImGui_User_NewFrame ();
}


SK_LazyGlobal <SK_ImGui_ResourcesD3D12> SK_ImGui_D3D12;


/// --------------- UGLY COMPAT HACK ----------------------
using D3D12GraphicsCommandList_CopyTextureRegion_pfn = void
(STDMETHODCALLTYPE *)( ID3D12GraphicsCommandList*,
                const  D3D12_TEXTURE_COPY_LOCATION*,
                       UINT, UINT, UINT,
                const  D3D12_TEXTURE_COPY_LOCATION*,
                const  D3D12_BOX* );
      D3D12GraphicsCommandList_CopyTextureRegion_pfn
      D3D12GraphicsCommandList_CopyTextureRegion_Original = nullptr;

//void
//STDMETHODCALLTYPE
//D3D12GraphicsCommandList_CopyResource_Detour (
//        ID3D12GraphicsCommandList *This,
//        ID3D12Resource            *pDstResource,
//        ID3D12Resource            *pSrcResource )
//{
//}

// Workaround for Control in HDR
void
STDMETHODCALLTYPE
D3D12GraphicsCommandList_CopyTextureRegion_Detour (
        ID3D12GraphicsCommandList    *This,
  const  D3D12_TEXTURE_COPY_LOCATION *pDst,
         UINT                         DstX,
         UINT                         DstY,
         UINT                         DstZ,
  const  D3D12_TEXTURE_COPY_LOCATION *pSrc,
  const  D3D12_BOX                   *pSrcBox )
{
  D3D12_RESOURCE_DESC src_desc =
    pSrc->pResource->GetDesc ();

  if ( D3D12_RESOURCE_DIMENSION_TEXTURE2D == src_desc.Dimension &&
       DXGI_FORMAT_R16G16B16A16_TYPELESS  ==
                     DirectX::MakeTypeless ( src_desc.Format )
     )
  {
    D3D12_RESOURCE_DESC dst_desc =
      pDst->pResource->GetDesc ();

    if ( DirectX::BitsPerColor (dst_desc.Format) !=
         DirectX::BitsPerColor (src_desc.Format) )
    {
      return;
    }
  }

  D3D12GraphicsCommandList_CopyTextureRegion_Original (
    This, pDst, DstX, DstY, DstZ, pSrc, pSrcBox
  );
}

void
_InitCopyTextureRegionHook (ID3D12GraphicsCommandList* pCmdList)
{
  if (D3D12GraphicsCommandList_CopyTextureRegion_Original == nullptr)
  {
    SK_CreateVFTableHook ( L"ID3D12GraphicsCommandList::CopyTextureRegion",
                           *(void***)*(&pCmdList), 16,
                            D3D12GraphicsCommandList_CopyTextureRegion_Detour,
                  (void **)&D3D12GraphicsCommandList_CopyTextureRegion_Original );
  }
}
/// --------------- UGLY COMPAT HACK ----------------------



void
SK_D3D12_RenderCtx::present (IDXGISwapChain3 *pSwapChain_)
{
  pSwapChain = pSwapChain_;

  if (pDevice.p == nullptr)
  {
    init (pSwapChain, pCommandQueue);
  }

  UINT swapIdx =
    pSwapChain->GetCurrentBackBufferIndex ();

#ifdef DEBUG
  SK_ReleaseAssert (swapIdx < frames_.size ());
#endif

  if (frames_.empty () || swapIdx >= frames_.size ())
    return;

  FrameCtx& currentFrame =
    frames_ [swapIdx];

  auto pCommandList =
    currentFrame.pCmdList.p;

	// Make sure all commands for this command allocator have finished executing before reseting it
	if (currentFrame.fence->GetCompletedValue () < currentFrame.fence.value)
	{
		if ( SUCCEEDED ( currentFrame.fence->SetEventOnCompletion (
                     currentFrame.fence.value,
                     currentFrame.fence.event                 )
                   )
       )
    {
      // Event is automatically reset after this wait is released
      WaitForSingleObject (currentFrame.fence.event, INFINITE);
    }
	}

  D3D12_RESOURCE_BARRIER hdr_copy_barriers [] =
  {
    CD3DX12_RESOURCE_BARRIER::Transition (
      currentFrame.pRenderOutput,
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_COPY_SOURCE ),
    CD3DX12_RESOURCE_BARRIER::Transition (
      currentFrame.hdr.pSwapChainCopy,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_COPY_DEST   )
  };

  D3D12_RESOURCE_BARRIER hdr_process_barriers [] =
  {
    CD3DX12_RESOURCE_BARRIER::Transition (
      currentFrame.hdr.pSwapChainCopy,
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE ),
    CD3DX12_RESOURCE_BARRIER::Transition (
      currentFrame.pRenderOutput,
        D3D12_RESOURCE_STATE_COPY_SOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET )
  };

  currentFrame.pCmdAllocator->Reset ();

  if (! currentFrame.begin_cmd_list ())
    return;

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  bool hdr_display =
    (rb.isHDRCapable () && (rb.framebuffer_flags & SK_FRAMEBUFFER_FLAG_HDR));

  extern bool __SK_HDR_16BitSwap;

  if ( hdr_display &&            __SK_HDR_16BitSwap &&
       currentFrame.hdr.pSwapChainCopy.p != nullptr &&
                        pHDRPipeline.p   != nullptr &&
                        pHDRSignature.p  != nullptr )
  {
    SK_RunOnce (
      _InitCopyTextureRegionHook (pCommandList)
    );

    auto& io =
      ImGui::GetIO ();

    HDR_LUMINANCE
      cbuffer_luma = {
        {  __SK_HDR_Luma,
           __SK_HDR_Exp,
          (__SK_HDR_HorizCoverage / 100.0f) * 2.0f - 1.0f,
          (__SK_HDR_VertCoverage  / 100.0f) * 2.0f - 1.0f
        }
      };

    HDR_COLORSPACE_PARAMS
      cbuffer_cspace                       = { };

      cbuffer_cspace.uiToneMapper          =   __SK_HDR_tonemap;
      cbuffer_cspace.hdrSaturation         =   __SK_HDR_Saturation;
      cbuffer_cspace.hdrPaperWhite         =   __SK_HDR_PaperWhite;
      cbuffer_cspace.sdrLuminance_NonStd   =   __SK_HDR_user_sdr_Y * 1.0_Nits;
      cbuffer_cspace.sdrIsImplicitlysRGB   =   __SK_HDR_Bypass_sRGB != 1;
      cbuffer_cspace.visualFunc [0]        = (uint32_t)__SK_HDR_visualization;
      cbuffer_cspace.visualFunc [1]        = (uint32_t)__SK_HDR_visualization;
      cbuffer_cspace.visualFunc [2]        = (uint32_t)__SK_HDR_visualization;

      cbuffer_cspace.hdrLuminance_MaxAvg   = __SK_HDR_tonemap == 2 ?
                                      rb.working_gamut.maxAverageY != 0.0f ?
                                      rb.working_gamut.maxAverageY         : rb.display_gamut.maxAverageY
                                                                   :         rb.display_gamut.maxAverageY;
      cbuffer_cspace.hdrLuminance_MaxLocal = __SK_HDR_tonemap == 2 ?
                                      rb.working_gamut.maxLocalY != 0.0f ?
                                      rb.working_gamut.maxLocalY         : rb.display_gamut.maxLocalY
                                                                   :       rb.display_gamut.maxLocalY;
      cbuffer_cspace.hdrLuminance_Min      = rb.display_gamut.minY * 1.0_Nits;
      cbuffer_cspace.currentTime           = (float)timeGetTime ();

    pCommandList->SetGraphicsRootSignature      (pHDRSignature);
    pCommandList->SetPipelineState              (pHDRPipeline);
    pCommandList->SetGraphicsRoot32BitConstants (0, 4,  &cbuffer_luma,   0);
    pCommandList->SetGraphicsRoot32BitConstants (1, 12, &cbuffer_cspace, 0);

    // Setup render state
    pCommandList->RSSetViewports ( 1,
       std::array <D3D12_VIEWPORT, 1> (
       {             0.0f, 0.0f,
         io.DisplaySize.x, io.DisplaySize.y,
                     0.0f, 1.0f
       }                              ).data ()
    );
    pCommandList->RSSetScissorRects ( 1,
              std::array <D3D12_RECT, 1> (
              {                      0,                      0,
                (LONG)io.DisplaySize.x, (LONG)io.DisplaySize.y
              }                          ).data ()
    );

    pCommandList->ResourceBarrier                   ( 2, hdr_copy_barriers                           );
    pCommandList->OMSetBlendFactor                  ( std::initializer_list <float> (
                                                         { 0.0f, 0.0f, 0.0f, 0.0f } ).begin ()       );
    pCommandList->IASetPrimitiveTopology            ( D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP           );

    pCommandList->CopyResource                      (     currentFrame.hdr.pSwapChainCopy.p,
                                                          currentFrame.     pRenderOutput.p          );
    pCommandList->ResourceBarrier                   ( 2,               hdr_process_barriers          );
    pCommandList->SetDescriptorHeaps                ( 1, &descriptorHeaps.pHDR.p                     );
    pCommandList->SetGraphicsRootDescriptorTable    ( 2,  currentFrame.hdr.hSwapChainCopy_GPU        );
    //pCommandList->SetGraphicsRootShaderResourceView ( 2,  currentFrame.hdr.hSwapChainCopy_GPU.ptr );
    pCommandList->OMSetRenderTargets                ( 1, &currentFrame.hRenderOutput, FALSE, nullptr );
    pCommandList->DrawInstanced                     ( 4, 1, 0, 0                                     );
  }

  else
    transition_state (pCommandList, currentFrame.pRenderOutput, D3D12_RESOURCE_STATE_PRESENT,
                                                                D3D12_RESOURCE_STATE_RENDER_TARGET);

  extern DWORD SK_ImGui_DrawFrame ( DWORD dwFlags, void* user    );
               SK_ImGui_DrawFrame (       0x00,          nullptr );

  // Potentially have to restart command list here because a screenshot was taken
	if (! currentFrame.begin_cmd_list ())
		return;

  transition_state   (pCommandList, currentFrame.pRenderOutput, D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                D3D12_RESOURCE_STATE_PRESENT);

  currentFrame.exec_cmd_list ();

  if ( const UINT64 sync_value = currentFrame.fence.value + 1;
		     SUCCEEDED ( pCommandQueue->Signal (
                                 currentFrame.fence.p,
                    sync_value             )
                   )
     )
  {
    _frame++;

	  currentFrame.fence.value =
      sync_value;
  }
}

bool
SK_D3D12_RenderCtx::FrameCtx::begin_cmd_list (const SK_ComPtr <ID3D12PipelineState>& state)
{
  if (bCmdListRecording)
  {
    if (state != nullptr) // Update pipeline state if requested
      pCmdList->SetPipelineState (state.p);

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

void
SK_D3D12_RenderCtx::FrameCtx::exec_cmd_list (void)
{
	assert (bCmdListRecording);

	bCmdListRecording = false;

	if (FAILED (pCmdList->Close ()))
		return;

	ID3D12CommandList* const cmd_lists [] = {
    pCmdList.p
  };

  // If we are doing this in the wrong order (i.e. during teardown),
  //   do not attempt to execute the command list, just close it.
  //
  //  Failure to skip execution will result in device removal
  if (pRoot->pSwapChain->GetCurrentBackBufferIndex () == iBufferIdx)
  {
	  pRoot->pCommandQueue->ExecuteCommandLists (
      ARRAYSIZE (cmd_lists),
                 cmd_lists
    );
  }
}

bool
SK_D3D12_RenderCtx::FrameCtx::wait_for_gpu (void)
{
  // Flush command list, to avoid it still referencing resources that may be destroyed after this call
  if (bCmdListRecording)
  {
    exec_cmd_list ();
  }

	// Increment fence value to ensure it has not been signaled before
	const UINT64 sync_value =
    fence.value + 1;

  if (! fence.event)
    return false;

  if ( FAILED (
         pRoot->pCommandQueue->Signal (
           fence.p, sync_value       )
              )
     )
  {
    return false; // Cannot wait on fence if signaling was not successful
  }

  if ( SUCCEEDED (
         fence->SetEventOnCompletion  (
           sync_value, fence.event    )
                 )
     )
  {
		WaitForSingleObject (fence.event, INFINITE);
  }

	// Update CPU side fence value now that it is guaranteed to have come through
	fence.value =
    sync_value;

  return true;
}

SK_D3D12_RenderCtx::FrameCtx::~FrameCtx (void)
{
  wait_for_gpu               ();

  pCmdList.Release           ();
  pCmdAllocator.Release      ();
  bCmdListRecording     = false;

  pRenderOutput.Release      ();
  hdr.pSwapChainCopy.Release ();

  fence.Release ();
  fence.value  = 0;

  if (fence.event != 0)
  {
    CloseHandle (fence.event);
                 fence.event = 0;
  }
}

void
SK_D3D12_RenderCtx::release (void)
{
  if (pCommandQueue != nullptr)
  {
	//pCommandQueue.Release                ();

    frames_.clear ();

    ImGui_ImplDX12_Shutdown ();

    pHDRPipeline.Release                 ();
    pHDRSignature.Release                ();

    descriptorHeaps.pBackBuffers.Release ();
	  descriptorHeaps.pImGui.Release       ();
    descriptorHeaps.pHDR.Release         ();

    pSwapChain.Release                   ();
    pDevice.Release                      ();
  }
}

static constexpr int _MinimumTriesToSkip = 1;// DXGI_MAX_SWAP_CHAIN_BUFFERS;

bool
SK_D3D12_RenderCtx::init (IDXGISwapChain3* pSwapChain_, ID3D12CommandQueue* pCommandQueue_)
{
  if (pCommandQueue_ && (! pCommandQueue))
    pCommandQueue = pCommandQueue_;

  static int tries = 0;

  DXGI_SWAP_CHAIN_DESC1   swapDesc1 = { };
  pSwapChain_->GetDesc1 (&swapDesc1);

  ///// Delay-init to avoid all kinds of hook wackiness
  if (tries++ <= swapDesc1.BufferCount)
    return false;

  tries = 0;

  pSwapChain =
    pSwapChain_;

  if ( pCommandQueue != nullptr && SUCCEEDED (
         pCommandQueue->GetDevice ( __uuidof (ID3D12Device),
                                    (void **)&pDevice.p )
                 )
     )
  {
    SK_ReleaseAssert (swapDesc1.BufferCount > 0);
    frames_.resize   (swapDesc1.BufferCount);

    for ( auto& frameCtx : frames_ )
    {
      SK_ReleaseAssert (
        SUCCEEDED (
             pDevice->CreateFence (
                   0, D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS (&frameCtx.fence.p)
                                  )
                  )
                       );

      SK_ComPtr <ID3D12CommandAllocator>    pAllocator;
      SK_ComPtr <ID3D12GraphicsCommandList> pList;

      if ( FAILED (
             pDevice->CreateCommandAllocator ( D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                 IID_PPV_ARGS (&pAllocator.p)
                                             )
                  )
         )
      {
        return false;
      }

      if ( FAILED (
           pDevice->CreateCommandList ( 0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                          pAllocator, nullptr,
                                            IID_PPV_ARGS (&pList.p)
                                      )
                )
           ||
         FAILED (
           pList->Close ()
                )
       )
      {
        return false;
      }

      frameCtx.pCmdAllocator = pAllocator;
      frameCtx.pCmdList      = pList;
    }

    D3D12_DESCRIPTOR_HEAP_DESC
      heapDesc_ImGui                = { };
	    heapDesc_ImGui.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	    heapDesc_ImGui.NumDescriptors = static_cast <UINT> (frames_.size ());
	    heapDesc_ImGui.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    if ( FAILED (
           pDevice->CreateDescriptorHeap (         &heapDesc_ImGui,
                             IID_PPV_ARGS (&descriptorHeaps.pImGui.p)
                                         )
                )
       )
    {
      return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC
      heapDesc_BackBuffers                = {                            };
			heapDesc_BackBuffers.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			heapDesc_BackBuffers.NumDescriptors = static_cast <UINT> (frames_.size ());
			heapDesc_BackBuffers.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    if ( FAILED (
           pDevice->CreateDescriptorHeap ( &heapDesc_BackBuffers,
                     IID_PPV_ARGS (&descriptorHeaps.pBackBuffers.p)
                                         )
                )
       )
    {
      return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC
      heapDesc_HDR                = {                            };
			heapDesc_HDR.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
      heapDesc_HDR.NumDescriptors = static_cast <UINT> (frames_.size ());
			heapDesc_HDR.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    if ( FAILED (
           pDevice->CreateDescriptorHeap ( &heapDesc_HDR,
                     IID_PPV_ARGS (&descriptorHeaps.pHDR.p)
                                         )
                )
       )
    {
      descriptorHeaps.pHDR = nullptr;
    }

    const auto rtvDescriptorSize =
      pDevice->GetDescriptorHandleIncrementSize (
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV
      );

    const auto srvDescriptorSize =
      pDevice->GetDescriptorHandleIncrementSize (
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
      );

	  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
      descriptorHeaps.pBackBuffers->GetCPUDescriptorHandleForHeapStart ();

    D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU =
      descriptorHeaps.pHDR->GetCPUDescriptorHandleForHeapStart ();
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU =
      descriptorHeaps.pHDR->GetGPUDescriptorHandleForHeapStart ();

    int i = 0;

	  for ( auto& frameCtx : frames_ )
    {
      SK_ComPtr <ID3D12Resource>
                       pBackBuffer;

		  if ( FAILED (
             pSwapChain->GetBuffer ( i++,
               IID_PPV_ARGS (&pBackBuffer.p)
                                   )
                  )
         ) continue;

      frameCtx.hRenderOutput =
        rtvHandle;

		  pDevice->CreateRenderTargetView (
         pBackBuffer, nullptr, rtvHandle
      );

      frameCtx.pRenderOutput = pBackBuffer;

      rtvHandle.ptr +=
        rtvDescriptorSize;

      D3D12_RESOURCE_DESC
        desc                  = { D3D12_RESOURCE_DIMENSION_TEXTURE2D };
	  	  desc.Width            = swapDesc1.Width;
	  	  desc.Height           = swapDesc1.Height;
	  	  desc.DepthOrArraySize = 1;
	  	  desc.MipLevels        = 1;
	  	  desc.Format           = swapDesc1.Format;
	  	  desc.SampleDesc       = { 1, 0 };

      D3D12_HEAP_PROPERTIES
        props                      = { D3D12_HEAP_TYPE_DEFAULT };
        props.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        props.VisibleNodeMask      = 1;

	  	if ( SUCCEEDED ( pDevice->CreateCommittedResource (
                      &props, D3D12_HEAP_FLAG_NONE, &desc,
                              D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr,
                                IID_PPV_ARGS (&frameCtx.hdr.pSwapChainCopy.p)
                                                        )
                     )
         )
      {
        pDevice->CreateShaderResourceView (
          frameCtx.hdr.pSwapChainCopy.p,
            nullptr,
              srvHandleCPU
        );

        frameCtx.hdr.hSwapChainCopy_CPU = srvHandleCPU;
        frameCtx.hdr.hSwapChainCopy_GPU = srvHandleGPU;

        srvHandleCPU.ptr += srvDescriptorSize;
        srvHandleGPU.ptr += srvDescriptorSize;
      }

      frameCtx.fence.event       =
        SK_CreateEvent (nullptr, FALSE, FALSE, nullptr);
      frameCtx.fence.value       = 0;
      frameCtx.bCmdListRecording = false;

      frameCtx.pRoot      =  this;
      frameCtx.iBufferIdx = i - 1;
	  }

    // Create the root signature
    {
      D3D12_DESCRIPTOR_RANGE
        srv_range                    = {};
		    srv_range.RangeType          = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		    srv_range.NumDescriptors     = 1;//3; // t1, t2
		    srv_range.BaseShaderRegister = 0; // t0

      D3D12_ROOT_PARAMETER
        param [3]                                     = { };

        param [0].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        param [0].Constants.ShaderRegister            = 0;
        param [0].Constants.RegisterSpace             = 0;
        param [0].Constants.Num32BitValues            = 4;// cbuffer vertexBuffer : register (b0)
        param [0].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_VERTEX;

        param [1].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        param [1].Constants.ShaderRegister            = 0;
        param [1].Constants.RegisterSpace             = 0;
        param [1].Constants.Num32BitValues            = 12;// cbuffer colorSpaceTransform : register (b0)
        param [1].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;

        param [2].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param [2].DescriptorTable.NumDescriptorRanges = 1;
        param [2].DescriptorTable.pDescriptorRanges   = &srv_range;
        param [2].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;

      D3D12_STATIC_SAMPLER_DESC
        staticSampler = { };

        staticSampler.Filter           = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        staticSampler.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSampler.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSampler.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSampler.MipLODBias       = 0.f;
        staticSampler.MaxAnisotropy    = 0;
        staticSampler.ComparisonFunc   = D3D12_COMPARISON_FUNC_ALWAYS;
        staticSampler.BorderColor      = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        staticSampler.MinLOD           = 0.f;
        staticSampler.MaxLOD           = 0.f;
        staticSampler.ShaderRegister   = 0;
        staticSampler.RegisterSpace    = 0;
        staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

      D3D12_ROOT_SIGNATURE_DESC
        desc                   = { };

        desc.NumParameters     = _countof (param);
        desc.pParameters       =           param;
        desc.NumStaticSamplers =  1;
        desc.pStaticSamplers   = &staticSampler;

        desc.Flags             =
        ( D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS       |
          D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS     |
          D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS );

      SK_ComPtr <ID3DBlob>
                    pBlob;

      typedef
        HRESULT (*D3D12SerializeRootSignature_pfn)(
                   const D3D12_ROOT_SIGNATURE_DESC   *pRootSignature,
                         D3D_ROOT_SIGNATURE_VERSION   Version,
                         ID3DBlob                   **ppBlob,
                         ID3DBlob                   **ppErrorBlob);

      static D3D12SerializeRootSignature_pfn
             D3D12SerializeRootSignature =
            (D3D12SerializeRootSignature_pfn)
               SK_GetProcAddress ( L"d3d12.dll",
                                    "D3D12SerializeRootSignature" );

      if ( SUCCEEDED ( D3D12SerializeRootSignature ( &desc,
                                                       D3D_ROOT_SIGNATURE_VERSION_1,
                                                         &pBlob.p, nullptr ) ) )
      {
        pDevice->CreateRootSignature (
          0, pBlob->GetBufferPointer (),
             pBlob->GetBufferSize    (),
               IID_PPV_ARGS (&pHDRSignature)
        );
      }
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC
      psoDesc                       = { };
      psoDesc.NodeMask              = 1;
      psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
      psoDesc.pRootSignature        = pHDRSignature;
      psoDesc.SampleMask            = UINT_MAX;
      psoDesc.NumRenderTargets      = 1;
      psoDesc.RTVFormats [0]        = swapDesc1.Format;
      psoDesc.SampleDesc.Count      = 1;
      psoDesc.Flags                 = D3D12_PIPELINE_STATE_FLAG_NONE;

      psoDesc.VS = { colorutil_vs_bytecode,
             sizeof (colorutil_vs_bytecode) /
             sizeof (colorutil_vs_bytecode) [0]
                   };

      psoDesc.PS = { uber_hdr_shader_ps_bytecode,
             sizeof (uber_hdr_shader_ps_bytecode) /
             sizeof (uber_hdr_shader_ps_bytecode) [0]
                   };

    // Create the blending setup
    {
      D3D12_BLEND_DESC&
        desc                                        = psoDesc.BlendState;
        desc.AlphaToCoverageEnable                  = false;
        desc.RenderTarget [0].BlendEnable           = true;
        desc.RenderTarget [0].SrcBlend              = D3D12_BLEND_SRC_ALPHA;
        desc.RenderTarget [0].DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget [0].BlendOp               = D3D12_BLEND_OP_ADD;
        desc.RenderTarget [0].SrcBlendAlpha         = D3D12_BLEND_ONE;//INV_SRC_ALPHA;
        desc.RenderTarget [0].DestBlendAlpha        = D3D12_BLEND_ZERO;
        desc.RenderTarget [0].BlendOpAlpha          = D3D12_BLEND_OP_ADD;
        desc.RenderTarget [0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }

    // Create the rasterizer state
    {
      D3D12_RASTERIZER_DESC&
        desc                       = psoDesc.RasterizerState;
        desc.FillMode              = D3D12_FILL_MODE_SOLID;
        desc.CullMode              = D3D12_CULL_MODE_NONE;
        desc.FrontCounterClockwise = FALSE;
        desc.DepthBias             = D3D12_DEFAULT_DEPTH_BIAS;
        desc.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        desc.SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        desc.DepthClipEnable       = true;
        desc.MultisampleEnable     = FALSE;
        desc.AntialiasedLineEnable = FALSE;
        desc.ForcedSampleCount     = 0;
        desc.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    }

    // Create depth-stencil State
    {
      D3D12_DEPTH_STENCIL_DESC&
        desc                              = psoDesc.DepthStencilState;
        desc.DepthEnable                  = false;
        desc.DepthWriteMask               = D3D12_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc                    = D3D12_COMPARISON_FUNC_ALWAYS;
        desc.StencilEnable                = false;
        desc.FrontFace.StencilFailOp      =
        desc.FrontFace.StencilDepthFailOp =
        desc.FrontFace.StencilPassOp      = D3D12_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc        = D3D12_COMPARISON_FUNC_ALWAYS;
        desc.BackFace                     =
        desc.FrontFace;
    }

    if ( FAILED ( pDevice->CreateGraphicsPipelineState    (
                    &psoDesc, IID_PPV_ARGS (&pHDRPipeline) ) )
       )
    {
      pHDRSignature.Release ();
    }

	  ImGui_ImplDX12_Init ( pDevice, static_cast <int> (frames_.size ()),
                            swapDesc1.Format,
				descriptorHeaps.pImGui->GetCPUDescriptorHandleForHeapStart (),
				descriptorHeaps.pImGui->GetGPUDescriptorHandleForHeapStart ()
    );

	  ImGui_ImplDX12_CreateDeviceObjects ();

    return true;
  }

  return false;
}

SK_LazyGlobal <SK_D3D12_RenderCtx> _d3d12_rbk2;