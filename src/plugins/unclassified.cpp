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


static const int priority_levels [] =
  { THREAD_PRIORITY_NORMAL,  THREAD_PRIORITY_ABOVE_NORMAL,
    THREAD_PRIORITY_HIGHEST, THREAD_PRIORITY_TIME_CRITICAL };

#include <SpecialK/parameter.h>
#include <unordered_set>

struct SK_FFXV_Thread
{
  ~SK_FFXV_Thread (void) { for ( auto && h : hThreads )
                            CloseHandle (h); }

  std::vector <HANDLE> hThreads;
  volatile LONG        dwPrio = THREAD_PRIORITY_NORMAL;

  sk::ParameterInt* prio_cfg;

  void setup (void);
} sk_ffxv_swapchain,
  sk_ffxv_vsync,
  sk_ffxv_async_run;

extern iSK_INI*             dll_ini;
extern sk::ParameterFactory g_ParameterFactory;

#if 0
using SleepConditionVariableCS_pfn = BOOL (WINAPI *)(PCONDITION_VARIABLE, PCRITICAL_SECTION, DWORD);
SleepConditionVariableCS_pfn SleepConditionVariableCS_Original = nullptr;

#include <SpecialK/hooks.h>
#include <SpecialK/tls.h>
#include <SpecialK/log.h>

#define __SK_SUBSYSTEM__ L"FFXV_Fix"

BOOL
WINAPI
SleepConditionVariableCS_Detour (
  _Inout_ PCONDITION_VARIABLE ConditionVariable,
  _Inout_ PCRITICAL_SECTION   CriticalSection,
  _In_    DWORD               dwMilliseconds )
{
  extern float target_fps;
  if (target_fps != 0.0f)
  {
    extern DWORD dwRenderThread;

    if (GetCurrentThreadId () == dwRenderThread)
    {
      SK_LOG_FIRST_CALL

      LeaveCriticalSection (CriticalSection);

      //SleepConditionVariableCS_Original ( ConditionVariable, CriticalSection, 0 );

      EnterCriticalSection (CriticalSection);

      SetLastError (ERROR_TIMEOUT);

      return 0;
    }
  }

  return
    SleepConditionVariableCS_Original ( ConditionVariable, CriticalSection, dwMilliseconds );
}
#endif


void
SK_FFXV_Thread::setup (void)
{
  HANDLE hThread;

  if ( DuplicateHandle ( (HANDLE)-1, (HANDLE)-2,
                         (HANDLE)-1, &hThread,
                         0, FALSE, DUPLICATE_SAME_ACCESS ) )
  {
    hThreads.push_back (hThread);

    prio_cfg =
      dynamic_cast <sk::ParameterInt *> (
        g_ParameterFactory.create_parameter <int> (L"Thread Priority")
      );


    if (this == &sk_ffxv_swapchain) 
    {
#if 0
      SK_CreateDLLHook2 (      L"kernel32.dll",
                                "SleepConditionVariableCS",
                                 SleepConditionVariableCS_Detour,
        static_cast_p2p <void> (&SleepConditionVariableCS_Original) );

      SK_ApplyQueuedHooks ();
#endif

      prio_cfg->register_to_ini ( dll_ini, L"FFXV.CPUFix", L"SwapChainPriority" );
    }

    else if (this == &sk_ffxv_vsync)
    {
      prio_cfg->register_to_ini ( dll_ini, L"FFXV.CPUFix", L"VSyncPriority" );
    }

    else if (this == &sk_ffxv_async_run)
    {
      prio_cfg->register_to_ini ( dll_ini, L"FFXV.DiskFix", L"AsyncFileRun" );
    }

    else
    {
      return;
    }

    dwPrio = GetThreadPriority ( &hThread );

    int                  prio                       = 0;
    if ( prio_cfg->load (prio) && prio < 4 && prio >= 0 )
    {
      InterlockedExchange ( &dwPrio, 
                              priority_levels [prio] );

      for ( auto && h : hThreads )
      {
        SetThreadPriority ( h, ReadAcquire (&dwPrio) );
      }
    }
  }
}

void
SK_FFXV_SetupThreadPriorities (void)
{
  static int iters = 0;

  if (sk_ffxv_swapchain.hThreads.empty ())
      sk_ffxv_swapchain.setup ();

  else  if ((iters++ % 120) == 0)
  {
    for (auto hThread : sk_ffxv_swapchain.hThreads)
    {
      SetThreadPriority (hThread, sk_ffxv_swapchain.dwPrio);
    }

    for (auto hThread : sk_ffxv_vsync.hThreads)
    {
      SetThreadPriority (hThread, sk_ffxv_vsync.dwPrio);
    }
  }
}

bool
SK_FFXV_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("Final Fantasy XV Windows Edition", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    static bool ignis_vision = false;
    static bool hair_club    = false;

    extern bool SK_D3D11_EnableTracking;

    if (ignis_vision || hair_club)
      SK_D3D11_EnableTracking = true;

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

    ImGui::SameLine ();

    if (ImGui::Checkbox (u8"(No)Hair Club for Men™", &hair_club))
    {
      if (hair_club)
      {
        // Normal Hair
        SK_D3D11_Shaders.pixel.blacklist.emplace (0x1a77046d);
        SK_D3D11_Shaders.pixel.blacklist.emplace (0x132b907a);
        SK_D3D11_Shaders.pixel.blacklist.emplace (0x8a0dbca1);
        SK_D3D11_Shaders.pixel.blacklist.emplace (0xc9bb3e7f);

        // Wet Hair
        //SK_D3D11_Shaders.pixel.blacklist.emplace (0x41c6add3);
        //SK_D3D11_Shaders.pixel.blacklist.emplace (0x4524bf4f);
        //SK_D3D11_Shaders.pixel.blacklist.emplace (0x62f9cfe8);
        //SK_D3D11_Shaders.pixel.blacklist.emplace (0x95f7de71);

        // HairWorks
        SK_D3D11_Shaders.pixel.blacklist.emplace (0x2d6f6ee8);
      } else {
        SK_D3D11_Shaders.pixel.blacklist.erase (0x1a77046d);
        SK_D3D11_Shaders.pixel.blacklist.erase (0x132b907a);
        SK_D3D11_Shaders.pixel.blacklist.erase (0x8a0dbca1);
        SK_D3D11_Shaders.pixel.blacklist.erase (0xc9bb3e7f);
        SK_D3D11_Shaders.pixel.blacklist.erase (0x2d6f6ee8);
        //SK_D3D11_Shaders.pixel.blacklist.erase (0x41c6add3);
        //SK_D3D11_Shaders.pixel.blacklist.erase (0x4524bf4f);
        //SK_D3D11_Shaders.pixel.blacklist.erase (0x62f9cfe8);
        //SK_D3D11_Shaders.pixel.blacklist.erase (0x95f7de71);
      }
    }


    auto ConfigThreadPriority = [](const char* name, SK_FFXV_Thread& thread) ->
    int
    {
      ImGui::PushID (name);

      int idx = ( (int)thread.dwPrio == priority_levels [0] ? 0 :
                ( (int)thread.dwPrio == priority_levels [1] ? 1 :
                ( (int)thread.dwPrio == priority_levels [2] ? 2 : 3 ) ) );

      if ( ! thread.hThreads.empty () )
      {
        if (ImGui::Combo (name, &idx, "Normal Priority\0Above Normal\0Highest\0Time Critical\0\0"))
        {
          for ( auto && h : thread.hThreads )
          {
            InterlockedExchange ( &thread.dwPrio, priority_levels [idx]);
            SetThreadPriority   ( h, ReadAcquire (&thread.dwPrio) );
          }

          thread.prio_cfg->store ( idx );
                  dll_ini->write ( dll_ini->get_filename () );
        }


        int dwPrio = idx;
        idx = ( dwPrio == priority_levels [0] ? 0 :
              ( dwPrio == priority_levels [1] ? 1 :
              ( dwPrio == priority_levels [2] ? 2 : 3 ) ) );

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ( );
          ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.075, 0.8, 0.9));
          ImGui::Text         ( "The graphics engine has bass-acwkwards scheduling priorities." );
          ImGui::PopStyleColor ();
          ImGui::Separator    ( );

          ImGui::BulletText  ("Time Critical Scheduling is for simple threads that write data constantly and would break if ever interrupted.");
          ImGui::TreePush    ("");
          ImGui::BulletText  ("Audio, for example.");
          ImGui::TreePop     (  );

          ImGui::Text        ("");

          ImGui::BulletText  ("--- Rendering is completely different ---");
          ImGui::TreePush    ("");
          ImGui::BulletText  ("The engine starves threads with more important work to do because it assigned them the wrong priority too.");
          ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.25, 0.8, 0.9));
          ImGui::BulletText  ("LOWER the priority of all render-related threads for best results.");
          ImGui::PopStyleColor ();
          ImGui::TreePop     (  );
          ImGui::EndTooltip  (  );
        }
      }

      ImGui::PopID ();

      return idx;
    };

    ImGui::BeginGroup ();
    int x =
    ConfigThreadPriority ("VSYNC Emulation Thread###VSE_Thr", sk_ffxv_vsync);
    int y =
    ConfigThreadPriority ("SwapChain Flip Thread###SWF_Thr",  sk_ffxv_swapchain);
    int z =
    ConfigThreadPriority ("Aync. File Run Thread###AFR_Thr",  sk_ffxv_async_run);

    ImGui::EndGroup   ();
    ImGui::SameLine   ();
    ImGui::BeginGroup ();

    for ( auto* label_me : { &x, &y, &z } )
    {
      if ( *label_me == 3 &&
            label_me != &z   )
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.12f, 0.9f, 0.95f));
        ImGui::BulletText     ("Change this for better performance!"); 
        ImGui::PopStyleColor  ();
      }

      else
        ImGui::Text ("");
    }
    ImGui::EndGroup (  );
    ImGui::SameLine (  );

    extern bool fix_sleep_0;
    ImGui::Checkbox ("Sleep (0) --> SwitchToThread ()", &fix_sleep_0);

    ImGui::TreePop  (  );

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