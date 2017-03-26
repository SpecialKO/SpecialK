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


sk::ParameterFactory  far_factory;
iSK_INI*              far_prefs                 = nullptr;
wchar_t               far_prefs_file [MAX_PATH] = { L'\0' };
sk::ParameterInt*     far_gi_workgroups         = nullptr;
sk::ParameterBool*    far_limiter_busy          = nullptr;
sk::ParameterBool*    far_rtss_warned           = nullptr;
sk::ParameterBool*    far_osd_disclaimer        = nullptr;


// (Presumable) Size of compute shader workgroup
int __FAR_GlobalIllumWorkGroupSize = 128;

extern void
__stdcall
SK_SetPluginName (std::wstring name);

#define FAR_VERSION_NUM L"0.2.0.2"
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
    SK_FAR_CheckVersion   (nullptr);

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


void
SK_FAR_InitPlugin (void)
{
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

  MH_ApplyQueued ();


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

    far_prefs->write (far_prefs_file);


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
  }

  if (ImGui::IsItemHovered())
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

  if (changed)
    far_prefs->write (far_prefs_file);
}

bool
__stdcall
SK_FAR_IsPlugIn (void)
{
  return far_prefs != nullptr;
}