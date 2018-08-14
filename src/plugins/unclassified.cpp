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
  ~SK_FFXV_Thread (void) {///noexcept {
    if (hThread)
      CloseHandle (hThread);
  }

  HANDLE               hThread;
  volatile LONG        dwPrio = THREAD_PRIORITY_NORMAL;

  sk::ParameterInt* prio_cfg;

  void setup (HANDLE __hThread);
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
SK_FFXV_Thread::setup (HANDLE __hThread)
{
  HANDLE hThreadCopy;

  if (! DuplicateHandle ( GetCurrentProcess (), __hThread, GetCurrentProcess (), &hThreadCopy, THREAD_ALL_ACCESS, FALSE, 0 ))
    return;

  hThread = hThreadCopy;

  prio_cfg =
    dynamic_cast <sk::ParameterInt *> (
      g_ParameterFactory.create_parameter <int> (L"Thread Priority")
    );


  if (this == &sk_ffxv_swapchain) 
  {
#if 0
    SK_CreateDLLHook2 (      L"kernel32",
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

  dwPrio = GetThreadPriority ( hThread );

  int                  prio                       = 0;
  if ( prio_cfg->load (prio) && prio < 4 && prio >= 0 )
  {
    InterlockedExchange ( &dwPrio, 
                            priority_levels [prio] );

    SetThreadPriority ( hThread, ReadAcquire (&dwPrio) );
  }
}

void
SK_FFXV_SetupThreadPriorities (void)
{
  static int iters = 0;

  if (sk_ffxv_swapchain.hThread == 0)
  {
    CHandle hThread (
      OpenThread ( THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId () )
    );

    sk_ffxv_swapchain.setup (hThread.m_h);
  }

  else  if ((iters++ % 120) == 0)
  {
    SetThreadPriority (sk_ffxv_swapchain.hThread, sk_ffxv_swapchain.dwPrio);
    SetThreadPriority (sk_ffxv_vsync.hThread,     sk_ffxv_vsync.dwPrio);
    SetThreadPriority (sk_ffxv_async_run.hThread, sk_ffxv_async_run.dwPrio);
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

      int idx = ( static_cast <int> (thread.dwPrio) == priority_levels [0] ? 0 :
                ( static_cast <int> (thread.dwPrio) == priority_levels [1] ? 1 :
                ( static_cast <int> (thread.dwPrio) == priority_levels [2] ? 2 : 3 ) ) );

      if ( thread.hThread )
      {
        if (ImGui::Combo (name, &idx, "Normal Priority\0Above Normal\0Highest\0Time Critical\0\0"))
        {
          InterlockedExchange ( &thread.dwPrio, priority_levels [idx]);
          SetThreadPriority   ( thread.hThread, ReadAcquire (&thread.dwPrio) );

          thread.prio_cfg->store ( idx );
                  dll_ini->write ( dll_ini->get_filename () );
        }


      const int dwPrio = idx;
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


volatile LONG SK_POE2_Horses_Held        = 0;
volatile LONG SK_POE2_SMT_Assists        = 0;
volatile LONG SK_POE2_ThreadBoostsKilled = 0;
         bool SK_POE2_FixUnityEmployment = false;
         bool SK_POE2_Stage2UnityFix     = false;
         bool SK_POE2_Stage3UnityFix     = false;

bool
SK_POE2_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("Pillars of Eternity II: Deadfire", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    ImGui::Checkbox        ("Supervise Unity Worker Thread Scheduling", &SK_POE2_FixUnityEmployment); ImGui::SameLine ();
    ImGui::TextUnformatted (" (Advanced, not saved)");

    if (SK_POE2_FixUnityEmployment)
    {
      ImGui::BeginGroup ();
      int lvl = SK_POE2_Stage3UnityFix ? 2 :
                SK_POE2_Stage2UnityFix ? 1 : 0;

      if (
        ImGui::Combo ( "Scheduling Supervisory Level",
                       &lvl, "Avoid GPU Thread Starvation\0"
                             "SMT Thread Custody Mediation\0"
                             "Apply Unity Rules on All Threads\0" )
      ) {
        SK_POE2_Stage3UnityFix = (lvl > 1);
        SK_POE2_Stage2UnityFix = (lvl > 0);
      } 
      ImGui::EndGroup   ();

      ImGui::SameLine   ();

      ImGui::BeginGroup ();
      ImGui::Text       ("Events Throttled:");
      ImGui::Text       ("SMT Microsleep Yields:");
      ImGui::Text       ("Pre-Emption Adjustments:");
      ImGui::EndGroup   ();

      ImGui::SameLine   ();

      ImGui::BeginGroup ();
      ImGui::Text       ("%lu", ReadAcquire (&SK_POE2_Horses_Held));
      ImGui::Text       ("%lu", ReadAcquire (&SK_POE2_SMT_Assists));
      ImGui::Text       ("%lu", ReadAcquire (&SK_POE2_ThreadBoostsKilled));
      ImGui::EndGroup   ();
    }

    ImGui::Separator ();

    static int orig =
      config.render.framerate.override_num_cpus;

    bool spoof = (config.render.framerate.override_num_cpus != -1);

    static SYSTEM_INFO             si = { };
    SK_RunOnce (SK_GetSystemInfo (&si));

    if ((! spoof) || static_cast <DWORD> (config.render.framerate.override_num_cpus) > (si.dwNumberOfProcessors / 2))
    {
      ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.14f, .8f, .9f));
      ImGui::BulletText     ("It is strongly suggested that you reduce worker threads to 1/2 max. or lower");
      ImGui::PopStyleColor  ();
    }

    if ( ImGui::Checkbox   ("Reduce Worker Threads", &spoof) )
    {
      config.render.framerate.override_num_cpus =
        ( spoof ? si.dwNumberOfProcessors : -1 );
    }

    if (spoof)
    {
      ImGui::SameLine  (                                             );
      ImGui::SliderInt ( "Number of Worker Threads",
                        &config.render.framerate.override_num_cpus,
                        1, si.dwNumberOfProcessors              );
    }

    if (config.render.framerate.override_num_cpus != orig)
    {
      ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f));
      ImGui::BulletText     ("Game Restart Required");
      ImGui::PopStyleColor  ();
    }

    ImGui::TreePop  ();

    return false;
  }

  return true;
}

volatile LONG __SK_Y0_InitiateHudFreeShot = 0;
volatile LONG __SK_Y0_QueuedShots         = 0;

void
SK_YS0_TriggerHudFreeScreenshot (void) noexcept
{
  InterlockedIncrement (&__SK_Y0_QueuedShots);
}

sk::ParameterBool* _SK_Y0_NoFPBlur;
sk::ParameterBool* _SK_Y0_NoSSAO;
sk::ParameterBool* _SK_Y0_NoDOF;

sk::ParameterBool*  _SK_Y0_LockVolume;
sk::ParameterFloat* _SK_Y0_LockLevel;
sk::ParameterBool*  _SK_Y0_QuietStart;
sk::ParameterFloat* _SK_Y0_QuietLevel;

sk::ParameterBool* _SK_Y0_FixAniso;
sk::ParameterBool* _SK_Y0_ClampLODBias;
sk::ParameterInt*  _SK_Y0_ForceAniso;

sk::ParameterInt*   _SK_Y0_SaveAnywhere;
iSK_INI*            _SK_Y0_Settings;

struct {
  int   save_anywhere =     0;
  bool  no_fp_blur    = false;
  bool  no_ssao       = false;
  bool  no_dof        = false;

  bool  lock_volume   =  true;
  float lock_level    =  1.0f;
  bool  quiet_start   =  true;
  float quiet_level   = 0.10f;
  int   __quiet_mode  = false;
} _SK_Y0_Cfg;

bool __SK_Y0_FixShadowAniso  = false;
bool __SK_Y0_FixAniso        =  true;
bool __SK_Y0_ClampLODBias    =  true;
int  __SK_Y0_ForceAnisoLevel =     0;
bool __SK_Y0_FilterUpdate    = false; 

// The two pixel shaders are for the foreground DepthOfField effect
#define SK_Y0_DOF_PS0_CRC32C 0x10d88ce3
#define SK_Y0_DOF_PS1_CRC32C 0x419dcbfc
#define SK_Y0_DOF_VS_CRC32C  0x0f5fefc2

#include <SpecialK/sound.h>

void
SK_Yakuza0_BeginFrame (void)
{
  if ( ReadAcquire (&__SK_Y0_QueuedShots)          > 0 ||
       ReadAcquire (&__SK_Y0_InitiateHudFreeShot) != 0    )
  {
#define SK_Y0_HUD_PS_CRC32C 0x2e24510d

    if (InterlockedCompareExchange (&__SK_Y0_InitiateHudFreeShot, -1, 1) == 1)
    {
      SK_D3D11_Shaders.pixel.blacklist.emplace (SK_Y0_HUD_PS_CRC32C);

      SK::SteamAPI::TakeScreenshot (SK::ScreenshotStage::BeforeOSD);
    }

    else if (InterlockedCompareExchange (&__SK_Y0_InitiateHudFreeShot, 0, -1) == -1)
    {
      SK_D3D11_Shaders.pixel.blacklist.erase (SK_Y0_HUD_PS_CRC32C);
    }

    else
    {
      InterlockedDecrement (&__SK_Y0_QueuedShots);
      InterlockedExchange  (&__SK_Y0_InitiateHudFreeShot, 1);
      
      return
        SK_Yakuza0_BeginFrame ();
    }
  }


  static bool done = false;

  if (_SK_Y0_Cfg.quiet_start && (! done))
  {
    _SK_Y0_Cfg.__quiet_mode = true;

    static CComPtr <ISimpleAudioVolume> pVolume =
      SK_WASAPI_GetVolumeControl (GetCurrentProcessId ());

    if (pVolume == nullptr)
        pVolume  = SK_WASAPI_GetVolumeControl (GetCurrentProcessId ());

    static float fOrigVol    = 0.0;

    if (pVolume != nullptr && fOrigVol == 0.0f)
        pVolume->GetMasterVolume (&fOrigVol);

    static DWORD dwStartTime = timeGetTime ();

    if (timeGetTime () < (dwStartTime + 20000UL))
    {
      if (pVolume != nullptr)
          pVolume->SetMasterVolume (_SK_Y0_Cfg.quiet_level, nullptr);
    }

    else
    {
      if (pVolume != nullptr)
          pVolume->SetMasterVolume (fOrigVol, nullptr);

      _SK_Y0_Cfg.__quiet_mode = false;
      done = true;
    }
  }

  else if (_SK_Y0_Cfg.lock_volume)
  {
    static CComPtr <ISimpleAudioVolume> pVolume =
      SK_WASAPI_GetVolumeControl (GetCurrentProcessId ());

    if (pVolume != nullptr)
        pVolume->SetMasterVolume (_SK_Y0_Cfg.lock_level, nullptr);
  }
}

void
SK_Yakuza0_PlugInInit (void)
{
  extern std::wstring&
    SK_GetRoamingDir (void);

  std::wstring game_settings =
    SK_GetRoamingDir ();

  game_settings += LR"(\Sega\Yakuza0\settings.ini)";

  _SK_Y0_Settings =
    SK_CreateINI (game_settings.c_str ());

  _SK_Y0_SaveAnywhere =
    dynamic_cast <sk::ParameterInt *> (
      g_ParameterFactory.create_parameter <int> (L"Save Anywhere")
    );

  _SK_Y0_SaveAnywhere->register_to_ini (_SK_Y0_Settings, L"General", L"SaveAnywhere");
  _SK_Y0_SaveAnywhere->load            (_SK_Y0_Cfg.save_anywhere);

  _SK_Y0_NoFPBlur =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory.create_parameter <bool> (L"No First-Person Blur")
    );

  _SK_Y0_NoSSAO =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory.create_parameter <bool> (L"No SSAO")
    );

  _SK_Y0_NoDOF =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory.create_parameter <bool> (L"No Depth of Field")
    );

  _SK_Y0_NoFPBlur->register_to_ini (
    SK_GetDLLConfig (), L"Yakuza0.Shaders", L"DisableFirstPersonBlur"
  );

  _SK_Y0_NoSSAO->register_to_ini (
    SK_GetDLLConfig (), L"Yakuza0.Shaders", L"DisableSSAO"
  );

  _SK_Y0_NoDOF->register_to_ini (
    SK_GetDLLConfig (), L"Yakuza0.Shaders", L"DisableDOF"
  );


  _SK_Y0_FixAniso = 
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory.create_parameter <bool> (L"Fix Anisotropy")
    );
  _SK_Y0_ClampLODBias = 
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory.create_parameter <bool> (L"Clamp Negative LOD Bias")
    );
  _SK_Y0_ForceAniso = 
    dynamic_cast <sk::ParameterInt *> (
      g_ParameterFactory.create_parameter <int> (L"Force Anisotropic Filtering")
    );

  _SK_Y0_FixAniso->register_to_ini (
    SK_GetDLLConfig (), L"Yakuza0.Textures", L"TrilinearToAniso"
  );
  _SK_Y0_ClampLODBias->register_to_ini (
    SK_GetDLLConfig (), L"Yakuza0.Textures", L"ClampLODBias"
  );
  _SK_Y0_ForceAniso->register_to_ini (
    SK_GetDLLConfig (), L"Yakuza0.Textures", L"ForceAnisoLevel"
  );


  _SK_Y0_QuietStart =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory.create_parameter <bool> (L"Quieter Start")
    );
  _SK_Y0_LockVolume =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory.create_parameter <bool> (L"Prevent Volume Changes")
    );

  _SK_Y0_LockLevel =
    dynamic_cast <sk::ParameterFloat *> (
      g_ParameterFactory.create_parameter <float> (L"Volume Lock Level")
    );
  _SK_Y0_QuietLevel =
    dynamic_cast <sk::ParameterFloat *> (
      g_ParameterFactory.create_parameter <float> (L"Volume Start Level")
    );

  _SK_Y0_QuietStart->register_to_ini (
    SK_GetDLLConfig (), L"Yakuza0.Sound", L"QuietStart"
  );
  _SK_Y0_QuietLevel->register_to_ini (
    SK_GetDLLConfig (), L"Yakuza0.Sound", L"QuietLevel"
  );
  _SK_Y0_LockVolume->register_to_ini (
    SK_GetDLLConfig (), L"Yakuza0.Sound", L"LockVolume"
  );
  _SK_Y0_LockLevel->register_to_ini (
    SK_GetDLLConfig (), L"Yakuza0.Sound", L"LockLevel"
  );

  _SK_Y0_NoDOF->load    (_SK_Y0_Cfg.no_dof);
  _SK_Y0_NoSSAO->load   (_SK_Y0_Cfg.no_ssao);
  _SK_Y0_NoFPBlur->load (_SK_Y0_Cfg.no_fp_blur);

  _SK_Y0_QuietStart->load (_SK_Y0_Cfg.quiet_start);
  _SK_Y0_QuietLevel->load (_SK_Y0_Cfg.quiet_level);
  _SK_Y0_LockVolume->load (_SK_Y0_Cfg.lock_volume);
  _SK_Y0_LockLevel->load  (_SK_Y0_Cfg.lock_level);

  _SK_Y0_ForceAniso->load   (__SK_Y0_ForceAnisoLevel);
  _SK_Y0_FixAniso->load     (__SK_Y0_FixAniso);
  _SK_Y0_ClampLODBias->load (__SK_Y0_ClampLODBias);

  extern bool SK_D3D11_EnableTracking;

  if ( _SK_Y0_Cfg.no_fp_blur ||
       _SK_Y0_Cfg.no_dof     ||
       _SK_Y0_Cfg.no_ssao       ) SK_D3D11_EnableTracking = true;

  if (_SK_Y0_Cfg.no_ssao)
  { SK_D3D11_Shaders.vertex.blacklist.emplace (0x97837269);
    SK_D3D11_Shaders.vertex.blacklist.emplace (0x7cc07f78);
    SK_D3D11_Shaders.vertex.blacklist.emplace (0xe5d4a297);
    SK_D3D11_Shaders.pixel.blacklist.emplace  (0x4d2973a3); 
    SK_D3D11_Shaders.pixel.blacklist.emplace  (0x0ed648e1);
    SK_D3D11_Shaders.pixel.blacklist.emplace  (0x170885b9);
    SK_D3D11_Shaders.pixel.blacklist.emplace  (0x4d2973a3);
    SK_D3D11_Shaders.pixel.blacklist.emplace  (0x5256777a);
    SK_D3D11_Shaders.pixel.blacklist.emplace  (0x69b8ef91); }

  if (_SK_Y0_Cfg.no_dof)
  { SK_D3D11_Shaders.vertex.blacklist.emplace (SK_Y0_DOF_VS_CRC32C);
    SK_D3D11_Shaders.pixel.blacklist.emplace  (SK_Y0_DOF_PS0_CRC32C);
    SK_D3D11_Shaders.pixel.blacklist.emplace  (SK_Y0_DOF_PS1_CRC32C); }

  if (_SK_Y0_Cfg.no_fp_blur)
  { SK_D3D11_Shaders.vertex.blacklist.emplace (0xb008686a); 
    SK_D3D11_Shaders.pixel.blacklist.emplace  (0x1c599fa7); }
}

bool __SK_Y0_1024_512 = true;
bool __SK_Y0_1024_768 = true;
bool __SK_Y0_960_540  = true;

bool
SK_Yakuza0_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("Yakuza 0", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    bool changed = false;

    static bool ssao_changed = false;

    ImGui::BeginGroup ();
    changed |= ImGui::Checkbox ("Disable First-Person Blur", &_SK_Y0_Cfg.no_fp_blur);
    changed |= ImGui::Checkbox ("Disable Depth of Field",    &_SK_Y0_Cfg.no_dof);
    changed |= ImGui::Checkbox ("Disable Ambient Occlusion", &_SK_Y0_Cfg.no_ssao);
    ImGui::EndGroup ();

    ImGui::SameLine ();

    ImGui::BeginGroup ();

    static CComPtr <ISimpleAudioVolume> pVolume =
      SK_WASAPI_GetVolumeControl (GetCurrentProcessId ());

    bool sound_changed = false;

    if (! _SK_Y0_Cfg.__quiet_mode)
    {
      sound_changed |=
        ImGui::Checkbox ("Lock Volume", &_SK_Y0_Cfg.lock_volume);

      if (_SK_Y0_Cfg.lock_volume)
      {
        ImGui::SameLine ();

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("The game occasionally fudges with volume, but you lock it down.");

        if (ImGui::SliderFloat ("Master Volume Control", &_SK_Y0_Cfg.lock_level, 0.0, 1.0, ""))
        {
          if (_SK_Y0_Cfg.lock_volume)
          {
            sound_changed = true;
          }

          pVolume->SetMasterVolume (_SK_Y0_Cfg.lock_level, nullptr);
        }
        ImGui::SameLine ();
        ImGui::TextColored ( ImColor::HSV ( 0.15f, 0.9f,
                              0.5f + _SK_Y0_Cfg.lock_level * 0.5f),
                                     "(%03.1f%%)  ",
                                       _SK_Y0_Cfg.lock_level * 100.0f );
      }
    }

    sound_changed |= ImGui::Checkbox ("Quiet Start Mode", &_SK_Y0_Cfg.quiet_start);

    if (_SK_Y0_Cfg.quiet_start)
    {
      ImGui::SameLine ();
      sound_changed |=
        ImGui::SliderFloat ("Intro Volume Level", &_SK_Y0_Cfg.quiet_level, 0.0, 1.0, "");
      ImGui::SameLine ();
      ImGui::TextColored ( ImColor::HSV ( 0.3f, 0.9f,
                             1.0f - _SK_Y0_Cfg.quiet_level * 0.5f),
                               "(%03.1f%%)  ",
                                 _SK_Y0_Cfg.quiet_level * 100.0f );
    }

    if (sound_changed)
    {
      _SK_Y0_QuietStart->store (_SK_Y0_Cfg.quiet_start);
      _SK_Y0_QuietLevel->store (_SK_Y0_Cfg.quiet_level);
      _SK_Y0_LockVolume->store (_SK_Y0_Cfg.lock_volume);
      _SK_Y0_LockLevel->store  (_SK_Y0_Cfg.lock_level);
    }

    ImGui::EndGroup   ();

    if (config.steam.screenshots.enable_hook)
    {
      ImGui::PushID    ("Y0_Screenshots");
      ImGui::Separator ();

      auto Keybinding = [] (SK_Keybind* binding, sk::ParameterStringW* param) ->
      auto
      {
        std::string label  = SK_WideCharToUTF8 (binding->human_readable) + "###";
                    label += binding->bind_name;

        if (ImGui::Selectable (label.c_str (), false))
        {
          ImGui::OpenPopup (binding->bind_name);
        }

        std::wstring original_binding = binding->human_readable;

        SK_ImGui_KeybindDialog (binding);

        if (original_binding != binding->human_readable)
        {
          param->store (binding->human_readable);

          SK_SaveConfig ();

          return true;
        }

        return false;
      };

      static std::set <SK_ConfigSerializedKeybind *>
        keybinds = {
          &config.steam.screenshots.game_hud_free_keybind
        };

      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      for ( auto& keybind : keybinds )
      {
        ImGui::Text          ( "%s:  ",
                                 keybind->bind_name );
      }
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      for ( auto& keybind : keybinds )
      {
        Keybinding ( keybind, keybind->param );
      }
      ImGui::EndGroup   ();

      bool png_changed = false;

      if (config.steam.screenshots.enable_hook)
      {
        png_changed =
          ImGui::Checkbox ( "Keep Lossless .PNG Screenshots",
                              &config.steam.screenshots.png_compress      );
      }

      if ( ( screenshot_manager != nullptr &&
             screenshot_manager->getExternalScreenshotRepository ().files > 0 ) )
      {
        ImGui::SameLine ();

        const SK_Steam_ScreenshotManager::screenshot_repository_s& repo =
          screenshot_manager->getExternalScreenshotRepository (png_changed);

        ImGui::BeginGroup (  );
        ImGui::TreePush   ("");
        ImGui::Text ( "%lu files using %ws",
                        repo.files,
                          SK_File_SizeToString (repo.liSize.QuadPart).c_str  ()
                    );

        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip ( "Steam does not support .png screenshots, so "
                              "SK maintains its own storage for lossless screenshots." );
        }

        ImGui::SameLine ();

        if (ImGui::Button ("Browse"))
        {
          ShellExecuteW ( GetActiveWindow (),
            L"explore",
              screenshot_manager->getExternalScreenshotPath (),
                nullptr, nullptr,
                      SW_NORMAL
          );
        }

        ImGui::TreePop  ();
        ImGui::EndGroup ();
      }
      ImGui::PopID      ();
    }

    ImGui::Separator ();

    if (ImGui::CollapsingHeader ("Texture Settings"))
    {
      static bool tex_changed = false;
    
      ImGui::TreePush ("");
    
      bool new_change = false;
    
      new_change |= ImGui::Checkbox  ("Fix Anisotropic Filtering", &__SK_Y0_FixAniso);
      new_change |= ImGui::Checkbox  ("Clamp LOD Bias",            &__SK_Y0_ClampLODBias);
      new_change |= ImGui::SliderInt ("Force Anisotropic Level",   &__SK_Y0_ForceAnisoLevel, 0, 16);
    
      if (new_change)
      {
        tex_changed |= new_change;
    
        _SK_Y0_FixAniso->store     (__SK_Y0_FixAniso);
        _SK_Y0_ClampLODBias->store (__SK_Y0_ClampLODBias);
        _SK_Y0_ForceAniso->store   (__SK_Y0_ForceAnisoLevel);
        SK_GetDLLConfig ()->write  (SK_GetDLLConfig ()->get_filename ());
      }
    
      if (tex_changed)
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f));
        ImGui::BulletText     ("Restart Game");
        ImGui::PopStyleColor  ();
      }
      ImGui::TreePop ();
    }

    if (changed)
    {
      // SSAO
      if (_SK_Y0_Cfg.no_ssao)
      { SK_D3D11_Shaders.vertex.blacklist.emplace (0x97837269);
        SK_D3D11_Shaders.vertex.blacklist.emplace (0x7cc07f78);
        SK_D3D11_Shaders.vertex.blacklist.emplace (0xe5d4a297);
        SK_D3D11_Shaders.pixel.blacklist.emplace  (0x4d2973a3); 
        SK_D3D11_Shaders.pixel.blacklist.emplace  (0x0ed648e1);
        SK_D3D11_Shaders.pixel.blacklist.emplace  (0x170885b9);
        SK_D3D11_Shaders.pixel.blacklist.emplace  (0x4d2973a3);
        SK_D3D11_Shaders.pixel.blacklist.emplace  (0x5256777a);
        SK_D3D11_Shaders.pixel.blacklist.emplace  (0x69b8ef91); }
      else
      { SK_D3D11_Shaders.vertex.blacklist.erase   (0x97837269);
        SK_D3D11_Shaders.vertex.blacklist.erase   (0x7cc07f78);
        SK_D3D11_Shaders.vertex.blacklist.erase   (0xe5d4a297);
        SK_D3D11_Shaders.pixel.blacklist.erase    (0x4d2973a3); 
        SK_D3D11_Shaders.pixel.blacklist.erase    (0x0ed648e1);
        SK_D3D11_Shaders.pixel.blacklist.erase    (0x170885b9);
        SK_D3D11_Shaders.pixel.blacklist.erase    (0x4d2973a3);
        SK_D3D11_Shaders.pixel.blacklist.erase    (0x5256777a);
        SK_D3D11_Shaders.pixel.blacklist.erase    (0x69b8ef91); }

      // DOF
      if (_SK_Y0_Cfg.no_dof)
      { SK_D3D11_Shaders.vertex.blacklist.emplace (SK_Y0_DOF_VS_CRC32C);
        SK_D3D11_Shaders.pixel.blacklist.emplace  (SK_Y0_DOF_PS0_CRC32C);
        SK_D3D11_Shaders.pixel.blacklist.emplace  (SK_Y0_DOF_PS1_CRC32C);
      }
      else
      { SK_D3D11_Shaders.vertex.blacklist.erase   (SK_Y0_DOF_VS_CRC32C);
        SK_D3D11_Shaders.pixel.blacklist.erase    (SK_Y0_DOF_PS0_CRC32C);
        SK_D3D11_Shaders.pixel.blacklist.erase    (SK_Y0_DOF_PS1_CRC32C);
      }

      // First Person Blur
      if (_SK_Y0_Cfg.no_fp_blur)
      { SK_D3D11_Shaders.vertex.blacklist.emplace (0xb008686a); 
        SK_D3D11_Shaders.pixel.blacklist.emplace  (0x1c599fa7); }
      else
      { SK_D3D11_Shaders.vertex.blacklist.erase   (0xb008686a); 
        SK_D3D11_Shaders.pixel.blacklist.erase    (0x1c599fa7); }

      _SK_Y0_NoDOF->store    (_SK_Y0_Cfg.no_dof);
      _SK_Y0_NoSSAO->store   (_SK_Y0_Cfg.no_ssao);
      _SK_Y0_NoFPBlur->store (_SK_Y0_Cfg.no_fp_blur);

      SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());

      extern bool SK_D3D11_EnableTracking;

      if ( _SK_Y0_Cfg.no_fp_blur ||
           _SK_Y0_Cfg.no_dof     ||
           _SK_Y0_Cfg.no_ssao       ) SK_D3D11_EnableTracking = true;

      return true;
    }

    ImGui::TreePop ();
  }

  return false;
}



volatile LONG __SK_MWH_QueuedShots         = 0;
volatile LONG __SK_MWH_InitiateHudFreeShot = 0;

void
SK_TriggerHudFreeScreenshot (void) noexcept
{
  extern bool SK_D3D11_EnableTracking;
              SK_D3D11_EnableTracking = true;

  InterlockedIncrement (&__SK_MWH_QueuedShots);
}

void
SK_MWH_BeginFrame (void)
{
  if ( ReadAcquire (&__SK_MWH_QueuedShots)          > 0 ||
       ReadAcquire (&__SK_MWH_InitiateHudFreeShot) != 0    )
  {
#define SK_MWH_HUD_VS_CRC32C 0x6f046ebc

    if (InterlockedCompareExchange (&__SK_MWH_InitiateHudFreeShot, -1, 1) == 1)
    {
      SK_D3D11_Shaders.vertex.blacklist.emplace (SK_MWH_HUD_VS_CRC32C);

      SK::SteamAPI::TakeScreenshot (SK::ScreenshotStage::BeforeOSD);
    }

    else if (InterlockedCompareExchange (&__SK_MWH_InitiateHudFreeShot, 0, -1) == -1)
    {
      SK_D3D11_Shaders.vertex.blacklist.erase (SK_MWH_HUD_VS_CRC32C);
    }

    else
    {
      InterlockedDecrement (&__SK_MWH_QueuedShots);
      InterlockedExchange (&__SK_MWH_InitiateHudFreeShot, 1);

      return
        SK_MWH_BeginFrame ();
    }
  }
}

sk::ParameterBool*  _SK_MHW_JobParity;
bool  __SK_MHW_JobParity = true;
sk::ParameterBool*  _SK_MHW_JobParityPhysical;
bool  __SK_MHW_JobParityPhysical = false;

sk::ParameterBool* _SK_MHW_10BitSwapChain;
bool __SK_MHW_10BitSwap = false;

sk::ParameterBool* _SK_MHW_16BitSwapChain;
bool __SK_MHW_16BitSwap = false;

sk::ParameterFloat* _SK_MHW_scRGBLuminance;
float __SK_MHW_HDR_Luma = 172.0_Nits;

sk::ParameterFloat* _SK_MHW_scRGBGamma;
float __SK_MHW_HDR_Exp  = 2.116f;

void
SK_MHW_PlugInInit (void)
{
  _SK_MHW_JobParity =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory.create_parameter <bool> (L"Job Parity")
      );

  _SK_MHW_JobParity->register_to_ini (
    SK_GetDLLConfig (), L"MonsterHuntersWorld.CPU", L"LimitJobThreads");

  _SK_MHW_JobParity->load  (__SK_MHW_JobParity);
  _SK_MHW_JobParity->store (__SK_MHW_JobParity);

  _SK_MHW_JobParityPhysical =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory.create_parameter <bool> (L"Job Parity Rule")
      );

  _SK_MHW_JobParityPhysical->register_to_ini (
    SK_GetDLLConfig (), L"MonsterHuntersWorld.CPU", L"LimitToPhysicalCores");

  _SK_MHW_JobParityPhysical->load  (__SK_MHW_JobParityPhysical);
  _SK_MHW_JobParityPhysical->store (__SK_MHW_JobParityPhysical);

  _SK_MHW_10BitSwapChain =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory.create_parameter <bool> (L"10-bit SwapChain")
    );

  _SK_MHW_10BitSwapChain->register_to_ini (
    SK_GetDLLConfig (), L"MonsterHuntersWorld.HDR", L"Use10BitSwapChain");

  _SK_MHW_10BitSwapChain->load  (__SK_MHW_10BitSwap);
  _SK_MHW_10BitSwapChain->store (__SK_MHW_10BitSwap);

  _SK_MHW_16BitSwapChain =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory.create_parameter <bool> (L"16-bit SwapChain")
    );

  _SK_MHW_16BitSwapChain->register_to_ini (
    SK_GetDLLConfig (), L"MonsterHuntersWorld.HDR", L"Use16BitSwapChain");

  _SK_MHW_16BitSwapChain->load  (__SK_MHW_16BitSwap);
  _SK_MHW_16BitSwapChain->store (__SK_MHW_16BitSwap);


  _SK_MHW_scRGBLuminance =
    dynamic_cast <sk::ParameterFloat *> (
      g_ParameterFactory.create_parameter <float> (L"Luminance")
    );

  _SK_MHW_scRGBLuminance->register_to_ini (
    SK_GetDLLConfig (), L"MonsterHuntersWorld.HDR", L"scRGBLuminance");

  _SK_MHW_scRGBLuminance->load  (__SK_MHW_HDR_Luma);
  _SK_MHW_scRGBLuminance->store (__SK_MHW_HDR_Luma);

  _SK_MHW_scRGBGamma =
    dynamic_cast <sk::ParameterFloat *> (
      g_ParameterFactory.create_parameter <float> (L"Gamma")
    );

  _SK_MHW_scRGBGamma->register_to_ini (
    SK_GetDLLConfig (), L"MonsterHuntersWorld.HDR", L"scRGBGamma");

  _SK_MHW_scRGBGamma->load  (__SK_MHW_HDR_Exp);
  _SK_MHW_scRGBGamma->store (__SK_MHW_HDR_Exp);
}

bool
SK_MHW_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("Monster Hunters World", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    static bool parity_orig = __SK_MHW_JobParity;

    if (ImGui::Checkbox ("Limit Job Threads to number of CPU cores", &__SK_MHW_JobParity))
    {
      _SK_MHW_JobParity->store (__SK_MHW_JobParity);
      SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());
    }

    static bool rule_orig =
      __SK_MHW_JobParityPhysical;

    if (__SK_MHW_JobParity)
    {
      ImGui::SameLine (); ImGui::Spacing ();
      ImGui::SameLine (); ImGui::Spacing ();
      ImGui::SameLine (); ImGui::Text ("Limit: ");
      ImGui::SameLine (); ImGui::Spacing ();
      ImGui::SameLine ();

      int rule = __SK_MHW_JobParityPhysical ? 1 : 0;

      bool changed =
        ImGui::RadioButton ("Logical Cores", &rule, 0);
      ImGui::SameLine ();
      changed |=
        ImGui::RadioButton ("Physical Cores", &rule, 1);

      if (changed)
      {
        __SK_MHW_JobParityPhysical = (rule == 1);
        _SK_MHW_JobParityPhysical->store (__SK_MHW_JobParityPhysical);
        SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());
      }
    }

    if (parity_orig != __SK_MHW_JobParity ||
        rule_orig != __SK_MHW_JobParityPhysical)
    {
      ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f));
      ImGui::BulletText ("Game Restart Required");
      ImGui::PopStyleColor ();
    }

    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("Without this option, the game spawns 32 job threads and nobody can get that many running efficiently.");


    if (ImGui::CollapsingHeader ("HDR Fix", ImGuiTreeNodeFlags_DefaultOpen))
    {
      static bool TenBitSwap_Original     = __SK_MHW_10BitSwap;
      static bool SixteenBitSwap_Original = __SK_MHW_16BitSwap;
      
      static int sel = __SK_MHW_16BitSwap ? 2 :
                       __SK_MHW_10BitSwap ? 1 : 0;

      if (ImGui::RadioButton ("None", &sel, 0))
      {
        __SK_MHW_10BitSwap = false;
        __SK_MHW_16BitSwap = false;

        _SK_MHW_10BitSwapChain->store (__SK_MHW_10BitSwap);
        _SK_MHW_16BitSwapChain->store (__SK_MHW_16BitSwap);

        SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());
      }
      ImGui::SameLine ();


      if (ImGui::RadioButton ("HDR10 (10-bit + Metadata)", &sel, 1))
      {
        __SK_MHW_10BitSwap = true;

        if (__SK_MHW_10BitSwap) __SK_MHW_16BitSwap = false;

        _SK_MHW_10BitSwapChain->store (__SK_MHW_10BitSwap);
        _SK_MHW_16BitSwapChain->store (__SK_MHW_16BitSwap);

        SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());
      }

      auto& rb =
        SK_GetCurrentRenderBackend ();

      if (rb.hdr_capable)
      {
        ImGui::SameLine ();

        if (ImGui::RadioButton ("scRGB HDR (16-bit)", &sel, 2))
        {
          __SK_MHW_16BitSwap = true;

          if (__SK_MHW_16BitSwap) __SK_MHW_10BitSwap = false;

          _SK_MHW_10BitSwapChain->store (__SK_MHW_10BitSwap);
          _SK_MHW_16BitSwapChain->store (__SK_MHW_16BitSwap);

          SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());
        }

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("This is the superior HDR format -- use it ;)");
      }

      if ( (TenBitSwap_Original     != __SK_MHW_10BitSwap ||
            SixteenBitSwap_Original != __SK_MHW_16BitSwap) )
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f));
        ImGui::BulletText     ("Game Restart Required");
        ImGui::PopStyleColor  ();
      }

      if ((__SK_MHW_10BitSwap || __SK_MHW_16BitSwap) && rb.hdr_capable)
      {
        CComQIPtr <IDXGISwapChain4> pSwap4 (rb.swapchain);

        if (pSwap4 != nullptr)
        {
          DXGI_OUTPUT_DESC1     out_desc = { };
          DXGI_SWAP_CHAIN_DESC swap_desc = { };
             pSwap4->GetDesc (&swap_desc);

          if (out_desc.BitsPerColor == 0)
          {
            CComPtr <IDXGIOutput> pOutput = nullptr;

            if (SUCCEEDED ((pSwap4->GetContainingOutput (&pOutput.p))))
            {
              CComQIPtr <IDXGIOutput6> pOutput6 (pOutput);

              pOutput6->GetDesc1 (&out_desc);
            }

            else
            {
              out_desc.BitsPerColor = 8;
            }
          }

          if (out_desc.BitsPerColor >= 10)
          {
            //const DisplayChromacities& Chroma = DisplayChromacityList[selectedChroma];
            DXGI_HDR_METADATA_HDR10 HDR10MetaData = {};

            static int cspace = 1;

            struct DisplayChromacities
            {
              float RedX;
              float RedY;
              float GreenX;
              float GreenY;
              float BlueX;
              float BlueY;
              float WhiteX;
              float WhiteY;
            } const DisplayChromacityList [] =
            {
              { 0.64000f, 0.33000f, 0.30000f, 0.60000f, 0.15000f, 0.06000f, 0.31270f, 0.32900f }, // Display Gamut Rec709 
              { 0.64000f, 0.33000f, 0.30000f, 0.60000f, 0.15000f, 0.06000f, 0.31270f, 0.32900f }, // Display Gamut Rec709 
              ///{ 0.70800f, 0.29200f, 0.17000f, 0.79700f, 0.13100f, 0.04600f, 0.31270f, 0.32900f }, // Display Gamut Rec2020
              //( out_desc.RedPrimary   [0], out_desc.RedPrimary   [1],
              //out_desc.GreenPrimary [0], out_desc.GreenPrimary [1],
              //out_desc.BluePrimary  [0], out_desc.BluePrimary  [1],
              //out_desc.WhitePoint   [0], out_desc.WhitePoint   [1] ),
              //( out_desc.RedPrimary   [0], out_desc.RedPrimary   [1],
              //out_desc.GreenPrimary [0], out_desc.GreenPrimary [1],
              //out_desc.BluePrimary  [0], out_desc.BluePrimary  [1],
              //out_desc.WhitePoint   [0], out_desc.WhitePoint   [1] ),
              { out_desc.RedPrimary   [0], out_desc.RedPrimary   [1],
                out_desc.GreenPrimary [0], out_desc.GreenPrimary [1],
                out_desc.BluePrimary  [0], out_desc.BluePrimary  [1],
                out_desc.WhitePoint   [0], out_desc.WhitePoint   [1] }
            };

            ImGui::TreePush ("");

            bool hdr_gamut_support = false;

            if (swap_desc.BufferDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT)
            {
              hdr_gamut_support = true;
            }

            if ( swap_desc.BufferDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM )
            {
              hdr_gamut_support = true;
              ImGui::RadioButton ("Rec 709",  &cspace, 0); ImGui::SameLine (); 
            }
            else if (cspace == 0) cspace = 1;

            if ( swap_desc.BufferDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM )
            {
              hdr_gamut_support = true;
              ImGui::RadioButton ("Rec 2020", &cspace, 1); ImGui::SameLine ();
            }
            else if (cspace == 1) cspace = 0;
            ////ImGui::RadioButton ("Native",   &cspace, 2); ImGui::SameLine ();

            if (! (config.render.framerate.swapchain_wait != 0 && swap_desc.BufferDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM))// hdr_gamut_support)
            {
              HDR10MetaData.RedPrimary [0] = static_cast <UINT16> (DisplayChromacityList [cspace].RedX * 50000.0f);
              HDR10MetaData.RedPrimary [1] = static_cast <UINT16> (DisplayChromacityList [cspace].RedY * 50000.0f);

              HDR10MetaData.GreenPrimary [0] = static_cast <UINT16> (DisplayChromacityList [cspace].GreenX * 50000.0f);
              HDR10MetaData.GreenPrimary [1] = static_cast <UINT16> (DisplayChromacityList [cspace].GreenY * 50000.0f);

              HDR10MetaData.BluePrimary [0] = static_cast <UINT16> (DisplayChromacityList [cspace].BlueX * 50000.0f);
              HDR10MetaData.BluePrimary [1] = static_cast <UINT16> (DisplayChromacityList [cspace].BlueY * 50000.0f);

              HDR10MetaData.WhitePoint [0] = static_cast <UINT16> (DisplayChromacityList [cspace].WhiteX * 50000.0f);
              HDR10MetaData.WhitePoint [1] = static_cast <UINT16> (DisplayChromacityList [cspace].WhiteY * 50000.0f);

              static float fLuma [4] = { out_desc.MaxLuminance, out_desc.MinLuminance,
                                         2000.0f,               600.0f };

              if (hdr_gamut_support && swap_desc.BufferDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM)
                ImGui::InputFloat4 ("Luminance Coefficients", fLuma, 1);

              HDR10MetaData.MaxMasteringLuminance     = static_cast <UINT>   (fLuma [0] * 10000.0f);
              HDR10MetaData.MinMasteringLuminance     = static_cast <UINT>   (fLuma [1] * 10000.0f);
              HDR10MetaData.MaxContentLightLevel      = static_cast <UINT16> (fLuma [2]);
              HDR10MetaData.MaxFrameAverageLightLevel = static_cast <UINT16> (fLuma [3]);

              if (hdr_gamut_support && swap_desc.BufferDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT)
              {
                float nits =
                  __SK_MHW_HDR_Luma / 1.0_Nits;

                
                if (ImGui::SliderFloat ( "###MHW_LUMINANCE", &nits, 80.0f, rb.display_gamut.maxY,
                                           "Middle-White Luminance: %.1f Nits" ))
                {
                  __SK_MHW_HDR_Luma = nits * 1.0_Nits;

                  _SK_MHW_scRGBLuminance->store (__SK_MHW_HDR_Luma);
                  SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());
                }

                ImGui::SameLine ();

                if (ImGui::SliderFloat ("SDR -> HDR Gamma", &__SK_MHW_HDR_Exp, 1.6f, 2.9f))
                {
                  _SK_MHW_scRGBGamma->store (__SK_MHW_HDR_Exp);
                  SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());
                }

                //ImGui::SameLine ();
                //ImGui::Checkbox ("Explicit LinearRGB -> sRGB###IMGUI_SRGB", &rb.ui_srgb);
              }

              if (swap_desc.BufferDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM && ImGui::Button ("Inject HDR10 Metadata"))
              {
                //if (cspace == 2)
                //  swap_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                //else if (cspace == 1)
                //  swap_desc.BufferDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                //else
                //  swap_desc.BufferDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;

                pSwap4->SetHDRMetaData (DXGI_HDR_METADATA_TYPE_NONE, 0, nullptr);

                if (swap_desc.BufferDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT)
                {
                  pSwap4->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709);
                }

                if      (cspace == 1) pSwap4->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
                else if (cspace == 0) pSwap4->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
                else                  pSwap4->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709);

                if (cspace == 1 || cspace == 0)
                  pSwap4->SetHDRMetaData (DXGI_HDR_METADATA_TYPE_HDR10, sizeof (HDR10MetaData), &HDR10MetaData);
              }
            }

            else
            {
              ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.075f, 1.0f, 1.0f));
              ImGui::BulletText     ("A waitable swapchain is required for HDR10 (D3D11 Settings/SwapChain | {Flip Model + Waitable}");
              ImGui::PopStyleColor  ();
            }

            ImGui::TreePop ();
          }
        }
      }
    }

    ///static int orig =
    ///  config.render.framerate.override_num_cpus;
    ///
    ///bool spoof = (config.render.framerate.override_num_cpus != -1);
    ///
    ///static SYSTEM_INFO             si = { };
    ///SK_RunOnce (SK_GetSystemInfo (&si));
    ///
    ///if ((! spoof) || static_cast <DWORD> (config.render.framerate.override_num_cpus) > (si.dwNumberOfProcessors / 2))
    ///{
    ///  ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.14f, .8f, .9f));
    ///  ImGui::BulletText     ("It is strongly suggested that you reduce threads to 1/2 max. or lower");
    ///  ImGui::PopStyleColor  ();
    ///}
    ///
    ///if ( ImGui::Checkbox   ("Reduce Reported CPU Core Count", &spoof) )
    ///{
    ///  config.render.framerate.override_num_cpus =
    ///    ( spoof ? si.dwNumberOfProcessors : -1 );
    ///}
    ///
    ///if (spoof)
    ///{
    ///  ImGui::SameLine  (                                             );
    ///  ImGui::SliderInt ( "",
    ///                    &config.render.framerate.override_num_cpus,
    ///                    1, si.dwNumberOfProcessors              );
    ///}
    ///
    ///if (config.render.framerate.override_num_cpus != orig)
    ///{
    ///  ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f));
    ///  ImGui::BulletText     ("Game Restart Required");
    ///  ImGui::PopStyleColor  ();
    ///}

    ImGui::TreePop ();

    return true;
  }

  return false;
}

