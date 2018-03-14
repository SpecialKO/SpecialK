// A bunch of stupid "plug-ins," not even worth copyrighting.
//

#include <imgui/imgui.h>

#include <SpecialK/config.h>
#include <SpecialK/control_panel.h>

#include <SpecialK/render/d3d9/d3d9_backend.h>
#include <SpecialK/render/dxgi/dxgi_backend.h>

bool
SK_GalGun_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("Gal*Gun: Double Peace", ImGuiTreeNodeFlags_DefaultOpen))
  {
    static bool emperor_has_no_clothes = false;

    ImGui::TreePush ("");

    if (ImGui::Checkbox ("The emperor of Japan has no clothes", &emperor_has_no_clothes))
    {
      const uint32_t ps_primary = 0x9b826e8a;
      const uint32_t vs_outline = 0x2e1993cf;

      if (emperor_has_no_clothes)
      {
        SK::D3D9::Shaders.vertex.blacklist.emplace (vs_outline);
        SK::D3D9::Shaders.pixel.blacklist.emplace  (ps_primary);
      }

      else
      {
        SK::D3D9::Shaders.vertex.blacklist.erase (vs_outline);
        SK::D3D9::Shaders.pixel.blacklist.erase  (ps_primary);
      }
    }

    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ( emperor_has_no_clothes ? "And neither do the girls in this game!" :
                                                   "But the prudes in this game do." );

    ImGui::TreePop ();

    return true;
  }

  return false;
}

bool
SK_LSBTS_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("Life is Strange: Before the Storm", ImGuiTreeNodeFlags_DefaultOpen))
  {
    static bool evil          = false;
    static bool even_stranger = false;
    static bool wired         = false;

    const uint32_t vs_eyes = 0x223ccf2d;
    const uint32_t ps_face = 0xbde11248;
    const uint32_t ps_skin = 0xa79e425c;

    ImGui::TreePush ("");

    if (ImGui::Checkbox ("Life is Wired", &wired))
    {
      if (wired)
      {
        SK_D3D11_Shaders.pixel.wireframe.emplace (ps_skin);
        SK_D3D11_Shaders.pixel.wireframe.emplace (ps_face);
      }
    
      else
      {
        SK_D3D11_Shaders.pixel.wireframe.erase (ps_skin);
        SK_D3D11_Shaders.pixel.wireframe.erase (ps_face);
      }
    }
    
    if (ImGui::Checkbox ("Life is Evil", &evil))
    {
      if (evil)
      {
        SK_D3D11_Shaders.vertex.blacklist.emplace (vs_eyes);
      }
    
      else
      {
        SK_D3D11_Shaders.vertex.blacklist.erase (vs_eyes);
      }
    }
    
    if (ImGui::Checkbox ("Life is Even Stranger", &even_stranger))
    {
      if (even_stranger)
      {
        SK_D3D11_Shaders.pixel.blacklist.emplace (ps_face);
        SK_D3D11_Shaders.pixel.blacklist.emplace (ps_skin);
      }
    
      else
      {
        SK_D3D11_Shaders.pixel.blacklist.erase (ps_face);
        SK_D3D11_Shaders.pixel.blacklist.erase (ps_skin);
      }
    }

    //bool enable = evil || even_stranger || wired;
    //
    //extern void
    //SK_D3D11_EnableTracking (bool state);
    //SK_D3D11_EnableTracking (enable || show_shader_mod_dlg);

    ImGui::TreePop ();

    return true;
  }

  return false;
}


static const DWORD priority_levels [] =
  { THREAD_PRIORITY_NORMAL,  THREAD_PRIORITY_ABOVE_NORMAL,
    THREAD_PRIORITY_HIGHEST, THREAD_PRIORITY_TIME_CRITICAL };

struct SK_FFXV_Thread
{
  HANDLE hThread = INVALID_HANDLE_VALUE;
  DWORD  dwPrio  = THREAD_PRIORITY_NORMAL;
  BOOL   bBoost  = FALSE;

  void setup (void);
} sk_ffxv_swapchain,
  sk_ffxv_vsync,
  sk_ffxv_async_run;

void
SK_FFXV_Thread::setup (void)
{
  if (hThread == INVALID_HANDLE_VALUE)
  {
    hThread = OpenThread ( THREAD_SET_INFORMATION,
                             FALSE, GetCurrentThreadId () );

    dwPrio = GetThreadPriority      (hThread);
    bBoost = GetThreadPriorityBoost (hThread, &bBoost);
  }
}


bool
SK_FFXV_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("Final Fantasy XV Windows Edition", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    static bool ignis_vision = false;

    if (ImGui::Checkbox (u8"Ignis Vision ™", &ignis_vision))
    {
      if (ignis_vision)
      {
        SK_D3D11_Shaders.vertex.wireframe.emplace (0x89d01dda);
        SK_D3D11_Shaders.vertex.on_top.emplace    (0x89d01dda);
      } else {
        SK_D3D11_Shaders.vertex.wireframe.erase   (0x89d01dda);
        SK_D3D11_Shaders.vertex.on_top.erase      (0x89d01dda);
      }
    }

    auto ConfigThreadPriority = [](const char* name, SK_FFXV_Thread& thread) ->
    void
    {
      ImGui::PushID (name);

      int idx = ( thread.dwPrio == priority_levels [0] ? 0 :
                ( thread.dwPrio == priority_levels [1] ? 1 :
                ( thread.dwPrio == priority_levels [2] ? 2 : 3 ) ) );

      if (thread.hThread != INVALID_HANDLE_VALUE)
      {
        if (ImGui::Combo (name, &idx, "Normal Priority\0Above Normal\0Highest\0Time Critical\0\0"))
        {
          thread.dwPrio = priority_levels [idx];

          SetThreadPriority ( thread.hThread, thread.dwPrio );
        }

        ImGui::SameLine ();

        bool boost = thread.bBoost;

        if (ImGui::Checkbox ("Boost Thread Priority", &boost))
        {
          thread.bBoost = 
            boost ?
              TRUE : FALSE;

          SetThreadPriorityBoost (thread.hThread, thread.bBoost);
        }
      }

      ImGui::PopID ();
    };

    ConfigThreadPriority ("VSYNC Emulation Thread###VSE_Thr", sk_ffxv_vsync);
    ConfigThreadPriority ("SwapChain Flip Thread###SWF_Thr",  sk_ffxv_swapchain);
    ConfigThreadPriority ("Aync. File Run Thread###AFR_Thr",  sk_ffxv_async_run);

    ImGui::TreePop ();

    return true;
  }

  return false;
}


bool
SK_SO4_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("STAR OCEAN - THE LAST HOPE - 4K & Full HD Remaster", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    extern float SK_SO4_MouseScale;
    ImGui::SliderFloat ("Mouselook Deadzone Compensation", &SK_SO4_MouseScale, 2.0f, 33.333f);

    if (ImGui::IsItemHovered ())
    {
      ImGui::SetTooltip ("Reduces mouse deadzone, but may cause Windows to draw the system cursor if set too high.");
    }

    ImGui::TreePop  ();

    return false;
  }

  return true;
}