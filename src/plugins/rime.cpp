#define _CRT_SECURE_NO_WARNINGS

#include <SpecialK/dxgi_backend.h>
#include <SpecialK/config.h>
#include <SpecialK/command.h>
#include <SpecialK/framerate.h>
#include <SpecialK/ini.h>
#include <SpecialK/parameter.h>
#include <SpecialK/utility.h>
#include <SpecialK/log.h>
#include <SpecialK/steam_api.h>

#include <SpecialK/input/input.h>
#include <SpecialK/input/xinput.h>

#include <SpecialK/hooks.h>
#include <SpecialK/core.h>
#include <process.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_d3d11.h>

#include <atlbase.h>

#define REASON_VERSION_NUM L"0.0.2.1"
#define REASON_VERSION_STR L"ReASON v " REASON_VERSION_NUM

// Block until update finishes, otherwise the update dialog
//   will be dismissed as the game crashes when it tries to
//     draw the first frame.
volatile LONG __REASON_init = FALSE;

sk::ParameterFactory  reason_factory;
iSK_INI*              reason_prefs                 = nullptr;
wchar_t               reason_prefs_file [MAX_PATH] = { L'\0' };
sk::ParameterBool*    reason_simple_interior_water = nullptr;
sk::ParameterBool*    reason_simple_ocean_water    = nullptr;

bool __REASON_SimpleInterior = false;
bool __REASON_SimpleOcean    = false;

extern void
__stdcall
SK_SetPluginName (std::wstring name);


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

typedef void (__stdcall *SK_PlugIn_ControlPanelWidget_pfn)(void);
        void  __stdcall SK_REASON_ControlPanel            (void);

static SK_PlugIn_ControlPanelWidget_pfn SK_PlugIn_ControlPanelWidget_Original = nullptr;

HRESULT
WINAPI
SK_REASON_CreateBuffer (
  _In_           ID3D11Device            *This,
  _In_     const D3D11_BUFFER_DESC       *pDesc,
  _In_opt_ const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_      ID3D11Buffer           **ppBuffer )
{
  return D3D11Dev_CreateBuffer_Original (This, pDesc, pInitialData, ppBuffer);
}

HRESULT
WINAPI
SK_REASON_CreateShaderResourceView (
  _In_           ID3D11Device                     *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC  *pDesc,
  _Out_opt_      ID3D11ShaderResourceView        **ppSRView )
{
  HRESULT hr =
    D3D11Dev_CreateShaderResourceView_Original (This, pResource, pDesc, ppSRView);

  return hr;
}

typedef void (CALLBACK *SK_PluginKeyPress_pfn)(
  BOOL Control, BOOL Shift, BOOL Alt,
  BYTE vkCode
);
static SK_PluginKeyPress_pfn SK_PluginKeyPress_Original;

#define SK_MakeKeyMask(vKey,ctrl,shift,alt) \
  (UINT)((vKey) | (((ctrl) != 0) <<  9) |   \
                  (((shift)!= 0) << 10) |   \
                  (((alt)  != 0) << 11))

#define SK_ControlShiftKey(vKey) SK_MakeKeyMask ((vKey), true, true, false)

void
CALLBACK
SK_REASON_PluginKeyPress (BOOL Control, BOOL Shift, BOOL Alt, BYTE vkCode)
{
  UINT uiMaskedKeyCode =
    SK_MakeKeyMask (vkCode, Control, Shift, Alt);
}

typedef LONG NTSTATUS;

typedef NTSTATUS (NTAPI *NtQueryTimerResolution_pfn)
(
  OUT PULONG              MinimumResolution,
  OUT PULONG              MaximumResolution,
  OUT PULONG              CurrentResolution
);

typedef NTSTATUS (NTAPI *NtSetTimerResolution_pfn)
(
  IN  ULONG               DesiredResolution,
  IN  BOOLEAN             SetResolution,
  OUT PULONG              CurrentResolution
);

static HMODULE                    NtDll                  = 0;

static NtQueryTimerResolution_pfn NtQueryTimerResolution = nullptr;
static NtSetTimerResolution_pfn   NtSetTimerResolution   = nullptr;

void
SK_REASON_InitPlugin (void)
{
  SK_SetPluginName (REASON_VERSION_STR);

//  if (! SK_IsInjected ())
//    SK_FAR_CheckVersion (nullptr);

  if (NtDll == 0) {
    NtDll = LoadLibrary (L"ntdll.dll");

    NtQueryTimerResolution =
      (NtQueryTimerResolution_pfn)
        GetProcAddress (NtDll, "NtQueryTimerResolution");

    NtSetTimerResolution =
      (NtSetTimerResolution_pfn)
        GetProcAddress (NtDll, "NtSetTimerResolution");

    if (NtQueryTimerResolution != nullptr &&
        NtSetTimerResolution   != nullptr) {
      ULONG min, max, cur;
      NtQueryTimerResolution (&min, &max, &cur);
      dll_log.Log ( L"[  Timing  ] Kernel resolution.: %f ms",
                      (float)(cur * 100)/1000000.0f );
      NtSetTimerResolution   (max, TRUE,  &cur);
      dll_log.Log ( L"[  Timing  ] New resolution....: %f ms",
                      (float)(cur * 100)/1000000.0f );

    }
  }

  SK_CreateFuncHook ( L"ID3D11Device::CreateBuffer",
                        D3D11Dev_CreateBuffer_Override,
                          SK_REASON_CreateBuffer,
                            (LPVOID *)&D3D11Dev_CreateBuffer_Original );
  MH_QueueEnableHook (D3D11Dev_CreateBuffer_Override);

  SK_CreateFuncHook ( L"ID3D11Device::CreateShaderResourceView",
                        D3D11Dev_CreateShaderResourceView_Override,
                          SK_REASON_CreateShaderResourceView,
                            (LPVOID *)&D3D11Dev_CreateShaderResourceView_Original );
  MH_QueueEnableHook (D3D11Dev_CreateShaderResourceView_Override);

  SK_CreateFuncHook ( L"SK_PlugIn_ControlPanelWidget",
                        SK_PlugIn_ControlPanelWidget,
                          SK_REASON_ControlPanel,
               (LPVOID *)&SK_PlugIn_ControlPanelWidget_Original );

  MH_QueueEnableHook (SK_PlugIn_ControlPanelWidget);

#if 0
  LPVOID dontcare = nullptr;

  SK_CreateFuncHook ( L"SK_ImGUI_DrawEULA_PlugIn",
                      SK_ImGui_DrawEULA_PlugIn,
                      SK_FAR_EULA_Insert,
                     &dontcare );

  MH_QueueEnableHook (SK_ImGui_DrawEULA_PlugIn);
#endif

  if (reason_prefs == nullptr)
  {
    lstrcatW (reason_prefs_file, SK_GetConfigPath ());
    lstrcatW (reason_prefs_file, L"ReASON.ini");

    reason_prefs = new iSK_INI (reason_prefs_file);
    reason_prefs->parse ();

    reason_simple_interior_water = 
        static_cast <sk::ParameterBool *>
          (reason_factory.create_parameter <bool> (L"Simplified Interior Water"));

    reason_simple_interior_water->register_to_ini ( reason_prefs,
                                                      L"ReASON.Water",
                                                        L"SimpleInterior" );

    if (reason_simple_interior_water->load ())
      __REASON_SimpleInterior = reason_simple_interior_water->get_value ();

    if (__REASON_SimpleInterior)
      SK_D3D11_Shaders.pixel.blacklist.insert (0x93a52f8d);

    reason_simple_interior_water->set_value (__REASON_SimpleInterior);
    reason_simple_interior_water->store     ();

    reason_simple_ocean_water = 
        static_cast <sk::ParameterBool *>
          (reason_factory.create_parameter <bool> (L"Simplified Ocean Water"));

    reason_simple_ocean_water->register_to_ini ( reason_prefs,
                                                   L"ReASON.Water",
                                                     L"SimpleOcean" );

    if (reason_simple_ocean_water->load ())
      __REASON_SimpleOcean = reason_simple_ocean_water->get_value ();

    if (__REASON_SimpleOcean)
      SK_D3D11_Shaders.pixel.blacklist.insert (0x750c5215);

    reason_simple_ocean_water->set_value (__REASON_SimpleInterior);
    reason_simple_ocean_water->store     ();

    reason_prefs->write (reason_prefs_file);


    MH_ApplyQueued ();
  }

  InterlockedExchange (&__REASON_init, 1);
}

// Not currently used
bool
WINAPI
SK_REASON_ShutdownPlugin (const wchar_t* backend)
{
  UNREFERENCED_PARAMETER (backend);

  return true;
}


void
__stdcall
SK_REASON_ControlPanel (void)
{
  bool changed = false;

  if (ImGui::CollapsingHeader("RiME", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));
    ImGui::TreePush       ("");

    if (ImGui::CollapsingHeader ("Water", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush ("");

      if (ImGui::Checkbox ("Simple Interior Water", &__REASON_SimpleInterior))
      {
        changed = true;

        if (__REASON_SimpleInterior)
          SK_D3D11_Shaders.pixel.blacklist.insert (0x93a52f8d);
        else
          SK_D3D11_Shaders.pixel.blacklist.erase  (0x93a52f8d);

        reason_simple_interior_water->set_value (__REASON_SimpleInterior);
        reason_simple_interior_water->store     ();
      }

      ImGui::SameLine ();

      if (ImGui::Checkbox ("Simple Ocean Water", &__REASON_SimpleOcean))
      {
        changed = true;

        if (__REASON_SimpleOcean)
          SK_D3D11_Shaders.pixel.blacklist.insert (0x750c5215);
        else
          SK_D3D11_Shaders.pixel.blacklist.erase  (0x750c5215);

        reason_simple_ocean_water->set_value (__REASON_SimpleOcean);
        reason_simple_ocean_water->store     ();
      }

      ImGui::TreePop     ( );
    }

    ImGui::TreePop       ( );
    ImGui::PopStyleColor (3);
  }

  if (changed)
    reason_prefs->write (reason_prefs_file);
}