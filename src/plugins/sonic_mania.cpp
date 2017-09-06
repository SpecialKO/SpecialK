#include <SpecialK/config.h>
#include <SpecialK/ini.h>
#include <SpecialK/parameter.h>
#include <SpecialK/core.h>
#include <SpecialK/utility.h>
#include <SpecialK/steam_api.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_d3d11.h>

#include <SpecialK/diagnostics/compatibility.h>


#define SMOKE_VERSION_NUM L"0.0.2"
#define SMOKE_VERSION_STR L"SMOKE v " SMOKE_VERSION_NUM

extern void
__stdcall
SK_SetPluginName (std::wstring name);

struct {
  struct {
    sk::ParameterBool* enable;
  } offline;
} SMOKE_drm;


using  SK_PlugIn_ControlPanelWidget_pfn = void (__stdcall         *)(void);
static SK_PlugIn_ControlPanelWidget_pfn SK_PlugIn_ControlPanelWidget_Original = nullptr;


void
__stdcall
SK_SMOKE_ControlPanel (void)
{
  if (ImGui::CollapsingHeader ("Sonic Mania", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));
    ImGui::TreePush       ("");

    if (ImGui::CollapsingHeader ("DRM Nonsense", ImGuiTreeNodeFlags_DefaultOpen))
    {
      bool val =
        SMOKE_drm.offline.enable->get_value ();

      ImGui::TreePush ("");

      if (ImGui::Checkbox ("Enable Offline Play", &val))
      {
        SMOKE_drm.offline.enable->set_value (val);
        SMOKE_drm.offline.enable->store     (   );
        SK_GetDLLConfig ()->write           ( SK_GetDLLConfig ()->get_filename () );
      }

      if (ImGui::IsItemHovered ( ))
      {
        if (SK_IsInjected ( ))
        {
          ImGui::SetTooltip ("Please use File | Install Local Wrapper DLL before enabling this.");
        }
      }

      ImGui::TreePop     ( );
    }

    ImGui::PopStyleColor (3);
    ImGui::TreePop       ( );
  }
}

void
SK_SMOKE_InitPlugin (void)
{
  static sk::ParameterFactory smoke_factory;

  SMOKE_drm.offline.enable = 
    dynamic_cast <sk::ParameterBool *> (
      smoke_factory.create_parameter <bool> (L"Enable Offline Play")
  );

  SMOKE_drm.offline.enable->register_to_ini ( SK_GetDLLConfig (),
                                                L"SMOKE.PlugIn",
                                                  L"EnableOfflinePlay" );

  if (! SMOKE_drm.offline.enable->load ())
  {
    SMOKE_drm.offline.enable->set_value (true);
    SMOKE_drm.offline.enable->store     (    );
  }

  if (SMOKE_drm.offline.enable->get_value ())
    config.steam.spoof_BLoggedOn = true;


  // SMOKE only works when locally wrapped
  if (! SK_IsInjected ())
    SK_SetPluginName (SMOKE_VERSION_STR);


  SK_CreateFuncHook (      L"SK_PlugIn_ControlPanelWidget",
                             SK_PlugIn_ControlPanelWidget,
                              SK_SMOKE_ControlPanel,
    static_cast_p2p <void> (&SK_PlugIn_ControlPanelWidget_Original) );
  MH_QueueEnableHook (       SK_PlugIn_ControlPanelWidget);
};