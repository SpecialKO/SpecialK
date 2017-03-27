#define _CRT_SECURE_NO_WARNINGS

#include <SpecialK/dxgi_backend.h>
#include <SpecialK/config.h>
#include <SpecialK/command.h>
#include <SpecialK/ini.h>
#include <SpecialK/parameter.h>
#include <SpecialK/utility.h>
#include <SpecialK/log.h>

#include <SpecialK/hooks.h>
#include <SpecialK/core.h>
#include <process.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_d3d11.h>

#include <atlbase.h>


#define SK_LOG0 if (config.system.log_level >= 1) dll_log.Log
#define SK_LOG1 if (config.system.log_level >= 2) dll_log.Log
#define SK_LOG2 if (config.system.log_level >= 3) dll_log.Log
#define SK_LOG3 if (config.system.log_level >= 4) dll_log.Log


sk::ParameterFactory  far_factory;
iSK_INI*              far_prefs                 = nullptr;
wchar_t               far_prefs_file [MAX_PATH] = { L'\0' };
sk::ParameterInt*     far_gi_workgroups         = nullptr;
sk::ParameterInt*     far_bloom_width           = nullptr;
sk::ParameterBool*    far_limiter_busy          = nullptr;
sk::ParameterBool*    far_rtss_warned           = nullptr;
sk::ParameterBool*    far_osd_disclaimer        = nullptr;


// (Presumable) Size of compute shader workgroup
int __FAR_GlobalIllumWorkGroupSize = 128;
int __FAR_BloomWidth               =  -1; // Set at startup from user prefs, never changed

extern void
__stdcall
SK_SetPluginName (std::wstring name);

#define FAR_VERSION_NUM L"0.3.0.1"
#define FAR_VERSION_STR L"FAR v " FAR_VERSION_NUM


typedef HRESULT (WINAPI *D3D11Dev_CreateBuffer_pfn)(
  _In_           ID3D11Device            *This,
  _In_     const D3D11_BUFFER_DESC       *pDesc,
  _In_opt_ const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_      ID3D11Buffer           **ppBuffer
);
typedef HRESULT (WINAPI *D3D11Dev_CreateShaderResourceView_pfn)(
  _In_           ID3D11Device                     *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC  *pDesc,
  _Out_opt_      ID3D11ShaderResourceView        **ppSRView
);

static D3D11Dev_CreateBuffer_pfn             D3D11Dev_CreateBuffer_Original;
static D3D11Dev_CreateShaderResourceView_pfn D3D11Dev_CreateShaderResourceView_Original;

extern
HRESULT
WINAPI
D3D11Dev_CreateBuffer_Override (
  _In_           ID3D11Device            *This,
  _In_     const D3D11_BUFFER_DESC       *pDesc,
  _In_opt_ const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_      ID3D11Buffer           **ppBuffer );

extern
HRESULT
WINAPI
D3D11Dev_CreateShaderResourceView_Override (
  _In_           ID3D11Device                     *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC  *pDesc,
  _Out_opt_      ID3D11ShaderResourceView        **ppSRView );


// Was threaded originally, but it is important to block until
//   the update check completes.
unsigned int
__stdcall
SK_FAR_CheckVersion (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  extern bool
  __stdcall
  SK_FetchVersionInfo (const wchar_t* wszProduct);

  if (SK_FetchVersionInfo (L"FAR")) {
    extern HRESULT
      __stdcall
      SK_UpdateSoftware (const wchar_t* wszProduct);

    SK_UpdateSoftware (L"FAR");
  }

  return 0;
}

HRESULT
WINAPI
SK_FAR_CreateBuffer (
  _In_           ID3D11Device            *This,
  _In_     const D3D11_BUFFER_DESC       *pDesc,
  _In_opt_ const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_      ID3D11Buffer           **ppBuffer )
{
  if ( pDesc != nullptr && pDesc->StructureByteStride == 96 &&
                           pDesc->ByteWidth           == 96 * 128 )
  {
    D3D11_BUFFER_DESC new_desc = *pDesc;

    new_desc.ByteWidth = 96 * __FAR_GlobalIllumWorkGroupSize;

    return D3D11Dev_CreateBuffer_Original (This, &new_desc, pInitialData, ppBuffer);
  }

  return D3D11Dev_CreateBuffer_Original (This, pDesc, pInitialData, ppBuffer);
}

HRESULT
WINAPI
SK_FAR_CreateShaderResourceView (
  _In_           ID3D11Device                     *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC  *pDesc,
  _Out_opt_      ID3D11ShaderResourceView        **ppSRView )
{
  if ( pDesc != nullptr && pDesc->ViewDimension        == D3D_SRV_DIMENSION_BUFFEREX &&
                           pDesc->BufferEx.NumElements == 128 )
  {
    CComPtr <ID3D11Buffer> pBuf;

    if ( SUCCEEDED (
           pResource->QueryInterface (__uuidof (ID3D11Buffer), (void **)&pBuf)
         )
       )
    {
      D3D11_SHADER_RESOURCE_VIEW_DESC new_desc = *pDesc;
      D3D11_BUFFER_DESC               buf_desc;

      pBuf->GetDesc (&buf_desc);

      if (buf_desc.ByteWidth == 96 * __FAR_GlobalIllumWorkGroupSize)
        new_desc.BufferEx.NumElements = __FAR_GlobalIllumWorkGroupSize;

      return D3D11Dev_CreateShaderResourceView_Original (This, pResource, &new_desc, ppSRView);
    }
  }

  return D3D11Dev_CreateShaderResourceView_Original (This, pResource, pDesc, ppSRView);
}


enum class SK_FAR_WaitBehavior
{
  Sleep = 0x1,
  Busy  = 0x2
};

SK_FAR_WaitBehavior wait_behavior (SK_FAR_WaitBehavior::Sleep);

extern LPVOID __SK_base_img_addr;
extern LPVOID __SK_end_img_addr;

extern void* __stdcall SK_Scan (const uint8_t* pattern, size_t len, const uint8_t* mask);

void
SK_FAR_SetLimiterWait (SK_FAR_WaitBehavior behavior)
{
  const uint8_t sleep_wait [] = { 0xFF, 0x15, 0xD3, 0x4B, 0x2C, 0x06 };
  const uint8_t busy_wait  [] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

  static bool   init      = false;
  static LPVOID wait_addr = 0x0;

  // Square-Enix rarely patches the games they publish, so just search for this pattern and
  //   don't bother to adjust memory addresses... if it's not found using the hard-coded address,
  //     bail-out.
  if (! init)
  {
    init = true;

    if ( (wait_addr = SK_Scan ( sleep_wait, 6, nullptr )) == nullptr )
    {
      dll_log.Log (L"[ FARLimit ]  Could not locate Framerate Limiter Sleep Addr.");
    }
    else {
      dll_log.Log (L"[ FARLimit ]  Scanned Framerate Limiter Sleep Addr.: 0x%p", wait_addr);
    }
  }

  if (wait_addr == nullptr)
    return;

  wait_behavior = behavior;

  DWORD dwProtect;
  VirtualProtect (wait_addr, 6, PAGE_EXECUTE_READWRITE, &dwProtect);

  // Hard coded for now, 
  switch (behavior)
  {
    case SK_FAR_WaitBehavior::Busy:
      memcpy (wait_addr, busy_wait, 6);
      break;

    case SK_FAR_WaitBehavior::Sleep:
      memcpy (wait_addr, sleep_wait, 6);
      break;
  }

  VirtualProtect (wait_addr, 6, dwProtect, &dwProtect);
}


extern void
STDMETHODCALLTYPE
SK_BeginBufferSwap (void);

extern BOOL
__stdcall
SK_DrawExternalOSD (std::string app_name, std::string text);

typedef void (STDMETHODCALLTYPE *SK_BeginFrame_pfn)(void);
SK_BeginFrame_pfn SK_BeginFrame_Original = nullptr;

void
STDMETHODCALLTYPE
SK_FAR_BeginFrame (void)
{
  SK_BeginFrame_Original ();

  SK_DrawExternalOSD ( "FAR", "  Press Ctrl + Shift + O         to toggle In-Game OSD\n"
                              "  Press Ctrl + Shift + Backspace to access In-Game Config Menu\n\n"
                              "   * This message will go away the first time you actually read it and successfully toggle the OSD.\n" );
}


// Sit and spin until the user figures out what an OSD is
//
DWORD
WINAPI
SK_FAR_OSD_Disclaimer (LPVOID user)
{
  SK_CreateFuncHook ( L"SK_BeginBufferSwap", SK_BeginBufferSwap,
                                             SK_FAR_BeginFrame,
                                  (LPVOID *)&SK_BeginFrame_Original );

  SK_EnableHook (SK_BeginBufferSwap);

  while (config.osd.show)
    Sleep (66);

  SK_DisableHook (SK_BeginBufferSwap);
  SK_RemoveHook  (SK_BeginBufferSwap);

  SK_DrawExternalOSD ( "FAR", "" );

  far_osd_disclaimer->set_value (false);
  far_osd_disclaimer->store     ();

  far_prefs->write              (far_prefs_file);

  CloseHandle (GetCurrentThread ());

  return 0;
}


void
SK_FAR_FirstFrame (void)
{
  if (! SK_IsInjected ())
  {
    bool busy_wait = far_limiter_busy->get_value ();

    SK_FAR_SetLimiterWait ( busy_wait ? SK_FAR_WaitBehavior::Busy :
                                        SK_FAR_WaitBehavior::Sleep );
  }

  if (GetModuleHandle (L"RTSSHooks64.dll"))
  {
    bool warned = far_rtss_warned->get_value ();

    if (! warned)
    {
      warned = true;
      
      SK_MessageBox ( L"RivaTuner Statistics Server Detected\r\n\r\n\t"
                      L"If FAR does not work correctly, this is probably why.",
                        L"Incompatible Third-Party Software", MB_OK | MB_ICONWARNING );

      far_rtss_warned->set_value (true);
      far_rtss_warned->store     ();
      far_prefs->write           (far_prefs_file);
    }
  }

  // Since people don't read guides, nag them to death...
  if (far_osd_disclaimer->get_value () && config.osd.show)
  {
    CreateThread ( nullptr,                 0,
                     SK_FAR_OSD_Disclaimer, nullptr,
                       0x00,                nullptr );
  }
}



typedef HRESULT (WINAPI *D3D11Dev_CreateTexture2D_pfn)(
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D
);
typedef void (WINAPI *D3D11_DrawIndexed_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation
);
typedef void (WINAPI *D3D11_Draw_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCount,
  _In_ UINT                 StartVertexLocation
);


static D3D11Dev_CreateTexture2D_pfn  D3D11Dev_CreateTexture2D_Original = nullptr;
static D3D11_DrawIndexed_pfn         D3D11_DrawIndexed_Original        = nullptr;
static D3D11_Draw_pfn                D3D11_Draw_Original               = nullptr;


extern HRESULT
WINAPI
D3D11Dev_CreateTexture2D_Override (
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D );

extern void
WINAPI
D3D11_DrawIndexed_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation );

extern void
WINAPI
D3D11_Draw_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCount,
  _In_ UINT                 StartVertexLocation );


// Overview:
//  The bloom pyramid in Nier:A is built up of 5 buffers, which are sized
//  800x450, 400x225, 200x112, 100x56 and 50x28, regardless of resolution
//  the mismatch between the largest buffer size and the screen resolution (in e.g. 2560x1440 or even 1920x1080)
//  leads to some really ugly artifacts.
//
//  To change this, we need to
//    1) Replace the rendertarget textures in question at their creation point
//    2) Adjust the viewport and some constant shader parameter each time they are rendered to
//
//  Examples here:
//    http://abload.de/img/bloom_defaultjhuq9.jpg 
//    http://abload.de/img/bloom_fixedp7uef.jpg
//
//  Note that there are more low-res 800x450 buffers not yet handled by this, 
//  but which could probably be handled similarly. Primarily, SSAO.

HRESULT
WINAPI
SK_FAR_CreateTexture2D (
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D )
{
  if (ppTexture2D == nullptr)
    return D3D11Dev_CreateTexture2D_Original ( This, pDesc, pInitialData, nullptr );

  static UINT  resW      = __FAR_BloomWidth; // horizontal resolution, must be set at application start
  static float resFactor = resW / 1600.0f;   // the factor required to scale to the largest part of the pyramid

  // R11G11B10 float textures of these sizes are part of the BLOOM PYRAMID
  // Note: we do not manipulate the 50x28 buffer
  //    -- it's read by a compute shader and the whole screen white level can be off if it is the wrong size
  if (pDesc->Format == DXGI_FORMAT_R11G11B10_FLOAT)
  {
    if (   (pDesc->Width == 800 && pDesc->Height == 450)
        || (pDesc->Width == 400 && pDesc->Height == 225)
        || (pDesc->Width == 200 && pDesc->Height == 112)
        || (pDesc->Width == 100 && pDesc->Height == 56) 
        /*|| (pDesc->Width == 50 && pDesc->Height == 28)*/ )
    {
      SK_LOG1 (L"Bloom Tex (%lux%lu)", pDesc->Width, pDesc->Height);

      D3D11_TEXTURE2D_DESC copy = *pDesc;

      // Scale the upper parts of the pyramid fully
      // and lower levels progressively less
      float pyramidLevelFactor  = (pDesc->Width - 50) / 750.0f;
      float scalingFactor       = 1.0f + (resFactor - 1.0f) * pyramidLevelFactor;

      copy.Width  = (UINT)(copy.Width  * scalingFactor);
      copy.Height = (UINT)(copy.Height * scalingFactor);

      pDesc       = &copy;
    }
  }

  return D3D11Dev_CreateTexture2D_Original ( This,
                                               pDesc, pInitialData,
                                                 ppTexture2D );
}


// High level description:
//  IF we have 
//   - 1 viewport
//   - with the size of one of the 4 elements of the pyramid we changed
//   - and a primary rendertarget of type R11G11B10
//   - which is associated with a texture of a size different from the viewport
//  THEN
//   - set the viewport to the texture size
//   - adjust the pixel shader constant buffer in slot #12 to this format (4 floats):
//     [ 0.5f / W, 0.5f / H, W, H ] (half-pixel size and total dimensions)
void
SK_FAR_PreDraw (ID3D11DeviceContext* pDevCtx)
{
  UINT numViewports = 0;

  pDevCtx->RSGetViewports (&numViewports, nullptr);

  if (numViewports == 1)
  {
    D3D11_VIEWPORT vp;

    pDevCtx->RSGetViewports (&numViewports, &vp);

    if (   (vp.Width == 800 && vp.Height == 450)
        || (vp.Width == 400 && vp.Height == 225)
        || (vp.Width == 200 && vp.Height == 112)
        || (vp.Width == 100 && vp.Height == 56) )
    {
      CComPtr <ID3D11RenderTargetView> rtView = nullptr;

      pDevCtx->OMGetRenderTargets (1, &rtView, nullptr);

      if (rtView)
      {
        D3D11_RENDER_TARGET_VIEW_DESC desc;

        rtView->GetDesc (&desc);

        if (desc.Format == DXGI_FORMAT_R11G11B10_FLOAT)
        {
          CComPtr <ID3D11Resource> rt = nullptr;

          rtView->GetResource (&rt);

          if (rt != nullptr)
          {
            CComPtr <ID3D11Texture2D> rttex = nullptr;

            rt->QueryInterface <ID3D11Texture2D> (&rttex);

            if (rttex != nullptr)
            {
              D3D11_TEXTURE2D_DESC texdesc;
              rttex->GetDesc (&texdesc);

              if (texdesc.Width != vp.Width)
              {
                // Here we go!
                // Viewport is the easy part

                vp.Width  = (float)texdesc.Width;
                vp.Height = (float)texdesc.Height;

                pDevCtx->RSSetViewports (1, &vp);

                // The constant buffer is a bit more difficult

                // We don't want to create a new buffer every frame,
                // but we also can't use the game's because they are read-only
                // this just-in-time initialized map is a rather ugly solution,
                // but it works as long as the game only renders from 1 thread (which it does)
                // NOTE: rather than storing them statically here (basically a global) the lifetime should probably be managed

                static std::map <UINT, ID3D11Buffer*> buffers;

                auto iter = buffers.find (texdesc.Width);
                if (iter == buffers.cend ())
                {
                  float constants [4] = {
                    0.5f / vp.Width, 0.5f / vp.Height,
                    (float)vp.Width, (float)vp.Height
                  };

                  CComPtr <ID3D11Device> dev;
                  pDevCtx->GetDevice (&dev);

                  D3D11_BUFFER_DESC buffdesc;
                  buffdesc.ByteWidth           = 16;
                  buffdesc.Usage               = D3D11_USAGE_IMMUTABLE;
                  buffdesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
                  buffdesc.CPUAccessFlags      = 0;
                  buffdesc.MiscFlags           = 0;
                  buffdesc.StructureByteStride = 16;

                  D3D11_SUBRESOURCE_DATA initialdata;
                  initialdata.pSysMem = constants;

                  ID3D11Buffer                                *replacementbuffer = nullptr;
                  dev->CreateBuffer (&buffdesc, &initialdata, &replacementbuffer);

                  buffers [texdesc.Width] = replacementbuffer;
                }

                pDevCtx->PSSetConstantBuffers (12, 1, &buffers [texdesc.Width]);
              }
            }
          }
        }
      }
    }
  }
}


void
WINAPI
SK_FAR_DrawIndexed (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation )
{
  if (IndexCount == 4 && StartIndexLocation == 0 && BaseVertexLocation == 0)
    SK_FAR_PreDraw (This);

  return D3D11_DrawIndexed_Original ( This, IndexCount,
                                        StartIndexLocation, BaseVertexLocation );
}

void
WINAPI
SK_FAR_Draw (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCount,
  _In_ UINT                 StartVertexLocation )
{
  if (VertexCount == 4 && StartVertexLocation == 0)
    SK_FAR_PreDraw (This);

  return D3D11_Draw_Original ( This, VertexCount,
                                 StartVertexLocation );
}




void
SK_FAR_InitPlugin (void)
{
  if (! SK_IsInjected ())
    SK_FAR_CheckVersion (nullptr);

  SK_SetPluginName (FAR_VERSION_STR);

  SK_CreateFuncHook ( L"ID3D11Device::CreateBuffer",
                        D3D11Dev_CreateBuffer_Override,
                          SK_FAR_CreateBuffer,
                            (LPVOID *)&D3D11Dev_CreateBuffer_Original );
  MH_QueueEnableHook (D3D11Dev_CreateBuffer_Override);

  SK_CreateFuncHook ( L"ID3D11Device::CreateShaderResourceView",
                        D3D11Dev_CreateShaderResourceView_Override,
                          SK_FAR_CreateShaderResourceView,
                            (LPVOID *)&D3D11Dev_CreateShaderResourceView_Original );
  MH_QueueEnableHook (D3D11Dev_CreateShaderResourceView_Override);


  if (far_prefs == nullptr)
  {
    lstrcatW (far_prefs_file, SK_GetConfigPath ());
    lstrcatW (far_prefs_file, L"FAR.ini");

    far_prefs = new iSK_INI (far_prefs_file);
    far_prefs->parse ();

    far_gi_workgroups = 
        static_cast <sk::ParameterInt *>
          (far_factory.create_parameter <int> (L"Global Illumination Compute Shader Workgroups"));

    far_gi_workgroups->register_to_ini ( far_prefs,
                                      L"FAR.Lighting",
                                        L"GlobalIlluminationWorkgroups" );

    if (far_gi_workgroups->load ())
      __FAR_GlobalIllumWorkGroupSize = far_gi_workgroups->get_value ();

    far_gi_workgroups->set_value (__FAR_GlobalIllumWorkGroupSize);
    far_gi_workgroups->store     ();


    far_limiter_busy = 
        static_cast <sk::ParameterBool *>
          (far_factory.create_parameter <bool> (L"Favor Busy-Wait For Better Timing"));

    far_limiter_busy->register_to_ini ( far_prefs,
                                      L"FAR.FrameRate",
                                        L"UseBusyWait" );

    if (! far_limiter_busy->load ())
    {
      // Enable by default, most people should have enough CPU cores for this
      //   policy to be practical.
      far_limiter_busy->set_value (true);
      far_limiter_busy->store     ();
    }

    far_rtss_warned = 
        static_cast <sk::ParameterBool *>
          (far_factory.create_parameter <bool> (L"RTSS Warning Issued"));

    far_rtss_warned->register_to_ini ( far_prefs,
                                         L"FAR.Compatibility",
                                           L"WarnedAboutRTSS" );

    if (! far_rtss_warned->load ())
    {
      far_rtss_warned->set_value (false);
      far_rtss_warned->store     ();
    }

    far_osd_disclaimer = 
        static_cast <sk::ParameterBool *>
          (far_factory.create_parameter <bool> (L"OSD Disclaimer Dismissed"));

    far_osd_disclaimer->register_to_ini ( far_prefs,
                                            L"FAR.OSD",
                                              L"ShowDisclaimer" );

    if (! far_osd_disclaimer->load ())
    {
      far_osd_disclaimer->set_value (true);
      far_osd_disclaimer->store     ();
    }


    far_bloom_width =
      static_cast <sk::ParameterInt *>
        (far_factory.create_parameter <int> (L"Width of Bloom Post-Process"));

    far_bloom_width->register_to_ini ( far_prefs,
                                         L"FAR.Lighting",
                                           L"BloomWidth" );

    if (! far_bloom_width->load ())
    {
      far_bloom_width->set_value (-1);
      far_bloom_width->store     (  );
    }

    __FAR_BloomWidth = far_bloom_width->get_value ();

    // Bloom Width must be > 0 or -1, never 0!
    if (__FAR_BloomWidth <= 0) {
      __FAR_BloomWidth =                -1;
      far_bloom_width->set_value (__FAR_BloomWidth);
      far_bloom_width->store     (                );
    }



    far_prefs->write (far_prefs_file);


    // If overriding bloom resolution, add these additional hooks
    //
    if (__FAR_BloomWidth != -1)
    {
      SK_CreateFuncHook ( L"ID3D11Device::CreateTexture2D",
                            D3D11Dev_CreateTexture2D_Override,
                              SK_FAR_CreateTexture2D,
                                (LPVOID *)&D3D11Dev_CreateTexture2D_Original );
      MH_QueueEnableHook (D3D11Dev_CreateTexture2D_Override);

      SK_CreateFuncHook ( L"ID3D11DeviceContext::Draw",
                            D3D11_Draw_Override,
                              SK_FAR_Draw,
                                (LPVOID *)&D3D11_Draw_Original );
      MH_QueueEnableHook (D3D11_Draw_Override);

      SK_CreateFuncHook ( L"ID3D11DeviceContext::DrawIndexed",
                            D3D11_DrawIndexed_Override,
                              SK_FAR_DrawIndexed,
                                (LPVOID *)&D3D11_DrawIndexed_Original );
      MH_QueueEnableHook (D3D11_DrawIndexed_Override);
    }


    MH_ApplyQueued ();

    SK_GetCommandProcessor ()->AddVariable ("FAR.GIWorkgroups", SK_CreateVar (SK_IVariable::Int,     &__FAR_GlobalIllumWorkGroupSize));
    //SK_GetCommandProcessor ()->AddVariable ("FAR.BusyWait",     SK_CreateVar (SK_IVariable::Boolean, &__FAR_BusyWait));
  }
}

// Not currently used
bool
WINAPI
SK_FAR_ShutdownPlugin (const wchar_t* backend)
{
  UNREFERENCED_PARAMETER (backend);

  return true;
}


void
__stdcall
SK_FAR_ControlPanel (void)
{
  bool changed = false;

  if (ImGui::CollapsingHeader("NieR: Automata", ImGuiTreeNodeFlags_DefaultOpen))
  {
    int quality = 0;

    if (__FAR_GlobalIllumWorkGroupSize < 16)
      quality = 0;
    else if (__FAR_GlobalIllumWorkGroupSize < 32)
      quality = 1;
    else if (__FAR_GlobalIllumWorkGroupSize < 64)
      quality = 2;
    else if (__FAR_GlobalIllumWorkGroupSize < 128)
      quality = 3;
    else
      quality = 4;

    if ( ImGui::Combo ( "Global Illumination Quality", &quality, "Off (High Performance)\0"
                                                                 "Low\0"
                                                                 "Medium\0"
                                                                 "High\0"
                                                                 "Ultra (Game Default)\0\0", 5 ) )
    {
      changed = true;

      switch (quality)
      {
        case 0:
          __FAR_GlobalIllumWorkGroupSize = 0;
          break;

        case 1:
          __FAR_GlobalIllumWorkGroupSize = 16;
          break;

        case 2:
          __FAR_GlobalIllumWorkGroupSize = 32;
          break;

        case 3:
          __FAR_GlobalIllumWorkGroupSize = 64;
          break;

        default:
        case 4:
          __FAR_GlobalIllumWorkGroupSize = 128;
          break;
      }
    }

    far_gi_workgroups->set_value (__FAR_GlobalIllumWorkGroupSize);
    far_gi_workgroups->store     ();

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip ();
      ImGui::Text         ("Global Illumination is indirect lighting bouncing off of surfaces");
      ImGui::Separator    ();
      ImGui::BulletText   ("Lower the quality for better performance but less natural looking lighting in shadows");
      ImGui::BulletText   ("Please direct thanks for this feature to DrDaxxy ;)");
      ImGui::EndTooltip   ();
    }

    bool busy_wait = (wait_behavior == SK_FAR_WaitBehavior::Busy);

    if (ImGui::Checkbox ("Use Busy-Wait Framerate Limiter", &busy_wait))
    {
      changed = true;

      if (busy_wait)
        SK_FAR_SetLimiterWait (SK_FAR_WaitBehavior::Busy);
      else
        SK_FAR_SetLimiterWait (SK_FAR_WaitBehavior::Sleep);

      far_limiter_busy->set_value (busy_wait);
      far_limiter_busy->store     ();
    }

    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("Increase CPU load on render thread in exchange for less hitching");

    bool expanded_bloom = ImGui::TreeNode ("Bloom");

    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("Changes to this setting require a full application restart");

    if (expanded_bloom)
    {
      int bloom_behavior = (far_bloom_width->get_value () != -1);

      if (ImGui::RadioButton ("Default Resolution (800x450)", &bloom_behavior, 0))
      {
        changed = true;

        far_bloom_width->set_value (-1);
        far_bloom_width->store     ();
      }

      ImGui::SameLine ();

      if (ImGui::RadioButton ("Custom Resolution",        &bloom_behavior, 1))
      {
        far_bloom_width->set_value ((int)ImGui::GetIO ().DisplaySize.x);
        far_bloom_width->store     ();

        changed = true;
      }

      int width = far_bloom_width->get_value ();

      if (bloom_behavior == 1)
      {
        ImGui::SameLine ();

        if (ImGui::InputInt ("Width", &width))
        {
          // Clamp values, 0 will crash!
          if (width <= 0)
            width = -1;

          far_bloom_width->set_value (width);
          far_bloom_width->store     ();

          changed = true;
        }
      }
      ImGui::TreePop ();
    }
  }

  if (changed)
    far_prefs->write (far_prefs_file);
}

bool
__stdcall
SK_FAR_IsPlugIn (void)
{
  return far_prefs != nullptr;
}