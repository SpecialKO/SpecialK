// A bunch of stupid "plug-ins," not even worth copyrighting.
//

#include <SpecialK/stdafx.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"BasicPlugN"


#include <SpecialK/render/d3d9/d3d9_backend.h>
#include <SpecialK/render/d3d11/d3d11_core.h>
#include <SpecialK/render/d3d11/d3d11_state_tracker.h>
#include <SpecialK/plugin/plugin_mgr.h>

struct SK_MemScan_Params__v0
{
  enum Privilege
  {
    Allowed    = true,
    Disallowed = false,
    DontCare   = (DWORD_PTR)-1
  };

  struct
  {
    Privilege execute = DontCare;
    Privilege read    = Allowed;
    Privilege write   = DontCare;
  } privileges;

  enum MemType
  {
    ImageCode  = SEC_IMAGE,
    FileData   = SEC_FILE,
    HeapMemory = SEC_COMMIT
  } mem_type;

  bool testPrivs (const MEMORY_BASIC_INFORMATION& mi)
  {
    if (mi.AllocationProtect == 0)
      return false;

    bool valid = true;


    if (privileges.execute != DontCare)
    {
      bool exec_matches = true;

      switch (mi.Protect)
      {
        case PAGE_EXECUTE:
        case PAGE_EXECUTE_READ:
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
          if (privileges.execute != Allowed)
            exec_matches = false;
          break;

        default:
          if (privileges.execute == Disallowed)
            exec_matches = false;
          break;
      }

      valid &= exec_matches;
    }


    if (privileges.read != DontCare)
    {
      bool read_matches = true;

      switch (mi.Protect)
      {
        case PAGE_READONLY:
        case PAGE_READWRITE:
        case PAGE_EXECUTE_READ:
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
          if (privileges.read != Allowed)
            read_matches = false;
          break;

        default:
          if (privileges.read == Disallowed)
            read_matches = false;
          break;
      }

      valid &= read_matches;
    }


    if (privileges.write != DontCare)
    {
      bool write_matches = true;

      switch (mi.Protect)
      {
        case PAGE_READWRITE:
        case PAGE_WRITECOPY:
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
          if (privileges.write != Allowed)
            write_matches = false;
          break;

        default:
          if (privileges.write == Disallowed)
            write_matches = false;
          break;
      }

      valid &= write_matches;
    }

    return valid;
  }
};


void*
__stdcall
SKX_ScanAlignedEx ( const void* pattern, size_t len,   const void* mask,
                          void* after,   int    align,    uint8_t* base_addr,

                          SK_MemScan_Params__v0 params =
                          SK_MemScan_Params__v0 ()       );

bool
SK_FarCry6_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("Far Cry 6", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    static void* pLimitSet =
      (void *)((uintptr_t)
       ( SK_GetProcAddress ( L"FC_m64d3d12.dll",
             "?Count@ReadBytesSkip@AK@@QEAAJXZ" )
       ) + 0x2F342 );
    // FC_m64d3d12.dll.reloc+3A0E02 - 8B 89 BC000000        - mov ecx,[rcx+000000BC]

    static bool patchable = false;
    static bool init      = false;

    if (! init)
    {
      patchable =
        SK_IsAddressExecutable (pLimitSet);

      init = true;

      if (patchable)
      {
        DWORD                                                      dwOrigProt = 0;
        if (VirtualProtect (pLimitSet, 6, PAGE_EXECUTE_READWRITE, &dwOrigProt))
        {
          patchable =
            (! memcmp (pLimitSet, "\x8B\x89\xBC\x00\x00\x00",
                                       6));
            VirtualProtect (pLimitSet, 6, dwOrigProt,             &dwOrigProt);
        }
      }
    }

    if (patchable)
    {
      static bool patched = false;

      if (ImGui::Checkbox ("Disable Framerate Limit", &patched))
      {
        DWORD                                                      dwOrigProt = 0;
        if (VirtualProtect (pLimitSet, 6, PAGE_EXECUTE_READWRITE, &dwOrigProt))
        {
          if (patched)
          {
            memcpy (pLimitSet, "\xB9\x00\x00\x00\x00\x90", 6);
          }

          else
          {
            memcpy (pLimitSet, "\x8B\x89\xBC\x00\x00\x00", 6);
          }
          VirtualProtect (pLimitSet, 6, dwOrigProt, &dwOrigProt);
        }

        else
          patched = (! patched);
      }
    }
    ImGui::TreePop  (  );

    return true;
  }

  return false;
}

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

      static auto& _Shaders = SK::D3D9::Shaders.get ();

      if (emperor_has_no_clothes)
      {
        _Shaders.vertex.blacklist.emplace (vs_outline);
        _Shaders.pixel.blacklist.emplace  (ps_primary);
      }

      else
      {
        _Shaders.vertex.blacklist.erase (vs_outline);
        _Shaders.pixel.blacklist.erase  (ps_primary);
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

    static auto& shaders =
      SK_D3D11_Shaders.get ();

    ImGui::TreePush ("");

    if (ImGui::Checkbox ("Life is Wired", &wired))
    {
      if (wired)
      {
        shaders.pixel.addTrackingRef (shaders.pixel.wireframe, ps_skin);
        shaders.pixel.addTrackingRef (shaders.pixel.wireframe, ps_face);
      }

      else
      {
        shaders.pixel.releaseTrackingRef (shaders.pixel.wireframe, ps_skin);
        shaders.pixel.releaseTrackingRef (shaders.pixel.wireframe, ps_face);
      }
    }

    if (ImGui::Checkbox ("Life is Evil", &evil))
    {
      if (evil)
      {
        shaders.vertex.addTrackingRef (shaders.vertex.blacklist, vs_eyes);
      }

      else
      {
        shaders.vertex.releaseTrackingRef (shaders.vertex.blacklist, vs_eyes);
      }
    }

    if (ImGui::Checkbox ("Life is Even Stranger", &even_stranger))
    {
      if (even_stranger)
      {
        shaders.pixel.addTrackingRef (shaders.pixel.blacklist, ps_face);
        shaders.pixel.addTrackingRef (shaders.pixel.blacklist, ps_skin);
      }

      else
      {
        shaders.pixel.releaseTrackingRef (shaders.pixel.blacklist, ps_face);
        shaders.pixel.releaseTrackingRef (shaders.pixel.blacklist, ps_skin);
      }
    }

    //bool enable = evil || even_stranger || wired;
    //
    //SK_D3D11_EnableTracking (enable || show_shader_mod_dlg);

    ImGui::TreePop ();

    return true;
  }

  return false;
}


static const int priority_levels [] =
  { THREAD_PRIORITY_NORMAL,  THREAD_PRIORITY_ABOVE_NORMAL,
    THREAD_PRIORITY_HIGHEST, THREAD_PRIORITY_TIME_CRITICAL };

#ifdef _M_AMD64
struct SK_FFXV_Thread
{
  ~SK_FFXV_Thread (void) {///noexcept {
    if (hThread)
      SK_CloseHandle (hThread);
  }

  HANDLE               hThread = 0;
  volatile LONG        dwPrio  = THREAD_PRIORITY_NORMAL;

  sk::ParameterInt* prio_cfg   = nullptr;

  void setup (HANDLE __hThread);
};

SK_LazyGlobal <SK_FFXV_Thread> sk_ffxv_swapchain,
                               sk_ffxv_vsync,
                               sk_ffxv_async_run;

typedef DWORD (WINAPI *GetEnvironmentVariableA_pfn)(
  LPCSTR lpName,
  LPCSTR lpBuffer,
  DWORD  nSize
);

GetEnvironmentVariableA_pfn
GetEnvironmentVariableA_Original = nullptr;

DWORD
WINAPI
GetEnvironmentVariableA_Detour ( LPCSTR lpName,
                                 LPSTR  lpBuffer,
                                 DWORD  nSize )
{
  if (_stricmp (lpName, "USERPROFILE") == 0)
  {
    char     szDocs [MAX_PATH + 2] = { };
    strcpy ( szDocs,
               SK_WideCharToUTF8 (SK_GetDocumentsDir ()).c_str () );

    PathRemoveFileSpecA (szDocs);

    if (lpBuffer != nullptr)
    {
      strncpy (lpBuffer, szDocs, nSize);

      //dll_log.Log ( L"GetEnvornmentVariableA (%hs) = %hs",
      //                lpName, lpBuffer );
    }

    return
      (DWORD)strlen (szDocs);
  }

  return
    GetEnvironmentVariableA_Original (lpName, lpBuffer, nSize);
}

void
SK_FFXV_InitPlugin (void)
{
  plugin_mgr->config_fns.emplace (SK_FFXV_PlugInCfg);

  std::unordered_set <uint32_t>
  __SK_FFXV_UI_Pix_Shaders =
  {
    0x224cc7df, 0x7182460b,
    0xe9716459, 0xe7015770,
    0xf15a90ab
  };

  for (                 auto    it : __SK_FFXV_UI_Pix_Shaders)
    SK_D3D11_DeclHUDShader_Pix (it);

  //SK_CreateDLLHook2 (      L"kernel32",
  //                          "GetEnvironmentVariableA",
  //                           GetEnvironmentVariableA_Detour,
  //  static_cast_p2p <void> (&GetEnvironmentVariableA_Original) );
}

bool fix_sleep_0 = false;

extern DWORD WINAPI SleepEx_Detour (DWORD, BOOL);
             static SleepEx_pfn
                    SleepEx_Override = nullptr;

DWORD
WINAPI
SK_FFXV_SleepEx (DWORD dwMilliseconds, BOOL bAlertable)
{
  if ( dwMilliseconds == 0 && fix_sleep_0 )
  {
    SwitchToThread ();
    return 0;
  }

  return
    SleepEx_Override (dwMilliseconds, bAlertable);
}

void
SK_FFXV_Thread::setup (HANDLE __hThread)
{
  HANDLE hThreadCopy;

  if (! DuplicateHandle ( GetCurrentProcess (), __hThread,
                          GetCurrentProcess (), &hThreadCopy, THREAD_ALL_ACCESS, FALSE, 0 ))
    return;

  auto dll_ini =
    SK_GetDLLConfig ();

  hThread = hThreadCopy;

  prio_cfg =
    dynamic_cast <sk::ParameterInt *> (
      g_ParameterFactory->create_parameter <int> (L"Thread Priority")
    );

  if (! prio_cfg)
    return;

  if (this == &*sk_ffxv_swapchain)
  {
    SK_CreateFuncHook (      L"SleepEx_Detour",
                               SleepEx_Detour,
                               SK_FFXV_SleepEx,
      static_cast_p2p <void> (&SleepEx_Override) );
    SK_EnableHook        (     SleepEx_Detour );
#if 0
    SK_CreateDLLHook2 (      L"kernel32",
                              "SleepConditionVariableCS",
                               SleepConditionVariableCS_Detour,
      static_cast_p2p <void> (&SleepConditionVariableCS_Original) );

    SK_ApplyQueuedHooks ();
#endif

    prio_cfg->register_to_ini ( dll_ini, L"FFXV.CPUFix",
                                         L"SwapChainPriority" );
  }

  else if (this == &*sk_ffxv_vsync)
  {
    prio_cfg->register_to_ini ( dll_ini, L"FFXV.CPUFix", L"VSyncPriority" );
  }

  else if (this == &*sk_ffxv_async_run)
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

  if (sk_ffxv_swapchain->hThread == 0)
  {
    SK_AutoHandle hThread (
      OpenThread ( THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId () )
    );

    sk_ffxv_swapchain->setup (hThread.m_h);
  }

  else  if ((iters++ % 120) == 0)
  {
    SetThreadPriority (sk_ffxv_swapchain->hThread, sk_ffxv_swapchain->dwPrio);
    SetThreadPriority (sk_ffxv_vsync->hThread,     sk_ffxv_vsync->dwPrio);
    SetThreadPriority (sk_ffxv_async_run->hThread, sk_ffxv_async_run->dwPrio);
  }
}

bool
SK_FFXV_PlugInCfg (void)
{
  auto dll_ini =
    SK_GetDLLConfig ();

  if (ImGui::CollapsingHeader ("Final Fantasy XV Windows Edition", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    static bool ignis_vision = false;
    static bool hair_club    = false;

    if (ignis_vision || hair_club)
      SK_D3D11_EnableTracking = true;

    static auto& shaders =
      SK_D3D11_Shaders.get ();

    if (ImGui::Checkbox ((const char *)u8R"(Ignis Vision ™)", &ignis_vision))
    {
      if (ignis_vision)
      {
        shaders.vertex.addTrackingRef (shaders.vertex.wireframe, 0x89d01dda);
        shaders.vertex.addTrackingRef (shaders.vertex.on_top,    0x89d01dda);
      } else {
        shaders.vertex.releaseTrackingRef (shaders.vertex.wireframe, 0x89d01dda);
        shaders.vertex.releaseTrackingRef (shaders.vertex.on_top,    0x89d01dda);
      }
    }

    ImGui::SameLine ();

    if (ImGui::Checkbox ((const char *)u8R"((No)Hair Club for Men™)", &hair_club))
    {
      if (hair_club)
      {
        // Normal Hair
        shaders.pixel.addTrackingRef (shaders.pixel.blacklist, 0x1a77046d);
        shaders.pixel.addTrackingRef (shaders.pixel.blacklist, 0x132b907a);
        shaders.pixel.addTrackingRef (shaders.pixel.blacklist, 0x8a0dbca1);
        shaders.pixel.addTrackingRef (shaders.pixel.blacklist, 0xc9bb3e7f);

        // Wet Hair
        //shaders.pixel.blacklist.emplace (0x41c6add3);
        //shaders.pixel.blacklist.emplace (0x4524bf4f);
        //shaders.pixel.blacklist.emplace (0x62f9cfe8);
        //shaders.pixel.blacklist.emplace (0x95f7de71);

        // HairWorks
        shaders.pixel.addTrackingRef (shaders.pixel.blacklist, 0x2d6f6ee8);
      } else {
        shaders.pixel.releaseTrackingRef (shaders.pixel.blacklist, 0x1a77046d);
        shaders.pixel.releaseTrackingRef (shaders.pixel.blacklist, 0x132b907a);
        shaders.pixel.releaseTrackingRef (shaders.pixel.blacklist, 0x8a0dbca1);
        shaders.pixel.releaseTrackingRef (shaders.pixel.blacklist, 0xc9bb3e7f);
        shaders.pixel.releaseTrackingRef (shaders.pixel.blacklist, 0x2d6f6ee8);
        //shaders.pixel.blacklist.erase (0x41c6add3);
        //shaders.pixel.blacklist.erase (0x4524bf4f);
        //shaders.pixel.blacklist.erase (0x62f9cfe8);
        //shaders.pixel.blacklist.erase (0x95f7de71);
      }
    }


    auto ConfigThreadPriority = [&](const char* name, SK_FFXV_Thread& thread) ->
    int
    {
      ImGui::PushID (name);

      int idx = ( sk::narrow_cast <int> (thread.dwPrio) == priority_levels [0] ? 0 :
                ( sk::narrow_cast <int> (thread.dwPrio) == priority_levels [1] ? 1 :
                ( sk::narrow_cast <int> (thread.dwPrio) == priority_levels [2] ? 2 : 3 ) ) );

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
          ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.075f, 0.8f, 0.9f).Value);
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
          ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.25f, 0.8f, 0.9f).Value);
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
        ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.12f, 0.9f, 0.95f));
        ImGui::BulletText     ("Change this for better performance!");
        ImGui::PopStyleColor  ();
      }

      else
        ImGui::Text ("");
    }
    ImGui::EndGroup (  );
    //ImGui::SameLine (  );
    //
    //extern bool fix_sleep_0;
    //ImGui::Checkbox ("Sleep (0) --> SwitchToThread ()", &fix_sleep_0);

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

    bool spoof = (config.render.framerate.override_num_cpus != SK_NoPreference);

    static SYSTEM_INFO             si = { };
    SK_RunOnce (SK_GetSystemInfo (&si));

    if ((! spoof) || sk::narrow_cast <DWORD> (config.render.framerate.override_num_cpus) > (si.dwNumberOfProcessors / 2))
    {
      ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (.14f, .8f, .9f));
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
      ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (.3f, .8f, .9f));
      ImGui::BulletText     ("Game Restart Required");
      ImGui::PopStyleColor  ();
    }

    ImGui::TreePop  ();

    return false;
  }

  return true;
}

volatile LONG __SK_SHENMUE_FinishedButNotPresented = 0;
volatile LONG __SK_SHENMUE_FullAspectCutscenes     = 1;
         bool  bSK_SHENMUE_FullAspectCutscenes     = true;

sk::ParameterBool*    _SK_SM_FullAspectCutscenes = nullptr;
sk::ParameterStringW* _SK_SM_FullAspectToggle    = nullptr;

sk::ParameterFloat*   _SK_SM_ClockFuzz           = nullptr;
sk::ParameterBool*    _SK_SM_BypassLimiter       = nullptr;

bool SK_Shenmue_UseNtDllQPC = true;

struct shenmue_limit_ctrl_s {
public:
  bool
    initialize (LPVOID lpQueryAddr)
  {
    if (lpQueryAddr != nullptr)
    {
      if ( SK_IsAddressExecutable (lpQueryAddr) &&
           SK_GetModuleFromAddr   (lpQueryAddr) ==
           SK_Modules->HostApp    (           )    )
      {
        qpc_loop_addr =
           lpQueryAddr;

        if ( branch_addr == nullptr )
        {
          branch_addr =
            SK_ScanAlignedEx ( "\x7C\xEE", 2, nullptr, (void *)qpc_loop_addr );

          // Damn, we couldn't find the loop control code
          if (branch_addr == nullptr)
              branch_addr = (LPVOID)-1;
        }

        if (want_bypass)
          toggle ();

        return true;
      }
    }

    return false;
  }

  bool
    toggle (void)
  {
    if (qpc_loop_addr != nullptr)
    {
      if (reinterpret_cast <intptr_t> (branch_addr) > 0)
      {
        DWORD dwOldProt = (DWORD)-1;

        VirtualProtect ( branch_addr,                     2,
                         PAGE_EXECUTE_READWRITE, &dwOldProt );

        if (enabled)
        {
          memcpy ( branch_addr,
                     "\x90\x90", 2 );
        }

        else
        {
          memcpy ( branch_addr,
                     orig_instns, 2 );
        }

        enabled = (! enabled);

        VirtualProtect ( branch_addr,          2,
                         dwOldProt,   &dwOldProt );
      }
    }

    return enabled;
  }

//protected:
  // ---------------------------------------
  // The first return addr. we encounter in our
  //   QPC hook for a call originating on the render
  //     thread is within jumping distance of the
  //       evil busy-loop causing framerate problems.
  LPCVOID qpc_loop_addr    =        nullptr;
  LPVOID  branch_addr      =        nullptr;
  uint8_t orig_instns  [2] = { 0x7C, 0xEE };

  // It takes a few frames to locate the limiter's
  //   inner-loop -- but the config file's preference
  //     needs to be respected.
  //
  //  * So ... once we find the addr. -> Install Bypass?
  //
  bool    want_bypass      = true;
  bool    enabled          = true;
} SK_Shenmue_Limiter;

bool
SK_Shenmue_IsLimiterBypassed (void)
{
  return
    ( SK_Shenmue_Limiter.enabled == false );
}

bool
SK_Shenmue_InitLimiterOverride (LPVOID pQPCRetAddr)
{
  return
    SK_Shenmue_Limiter.initialize (pQPCRetAddr);
}

SK_IVariable *pVarWideCutscenes;
SK_IVariable *pVarBypassLimiter;

void
SK_SM_PlugInInit (void)
{
  plugin_mgr->config_fns.emplace (SK_SM_PlugInCfg);

  auto cp =
    SK_GetCommandProcessor ();

       if (0 == _wcsicmp (SK_GetHostApp (), L"Shenmue.exe"))
    __SK_SHENMUE_ClockFuzz = 20.0f;
  else if (0 == _wcsicmp (SK_GetHostApp (), L"Shenmue2.exe"))
    __SK_SHENMUE_ClockFuzz = 166.0f;

  _SK_SM_FullAspectCutscenes =
    _CreateConfigParameterBool  ( L"Shenmue.Misc",
                                  L"FullAspectCutscenes", bSK_SHENMUE_FullAspectCutscenes,
                                  L"Enable Full Aspect Ratio Cutscenes" );

  _SK_SM_ClockFuzz =
    _CreateConfigParameterFloat ( L"Shenmue.Misc",
                                  L"ClockFuzz", __SK_SHENMUE_ClockFuzz,
                                  L"Framerate Limiter Variance" );

  _SK_SM_BypassLimiter =
    _CreateConfigParameterBool  ( L"Shenmue.Misc",
                                  L"BypassFrameLimiter", SK_Shenmue_Limiter.want_bypass,
                                  L"Die you evil hellspawn!" );

  if (bSK_SHENMUE_FullAspectCutscenes)
  {
    InterlockedExchange  (&__SK_SHENMUE_FullAspectCutscenes, 1);
    InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
  }

  else
  {
    InterlockedExchange (&__SK_SHENMUE_FullAspectCutscenes, 0);
  }


  cp->AddVariable ( "Shenmue.ClockFuzz",
                      new SK_IVarStub <float> ((float *)&__SK_SHENMUE_ClockFuzz));

  class listen : public SK_IVariableListener
  {
  public:
    virtual ~listen (void) = default;

    bool OnVarChange (SK_IVariable* var, void* val = nullptr) override
    {
      if (var == pVarWideCutscenes)
      {
        if (val != nullptr)
        {
          InterlockedExchange (&__SK_SHENMUE_FullAspectCutscenes, *(bool *) val != 0 ? 1 : 0);

          if (ReadAcquire (&__SK_SHENMUE_FullAspectCutscenes))
          {
            InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
          }
          else
            InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
        }
      }

      if (var == pVarBypassLimiter)
      {
        if (val != nullptr)
        {
          SK_Shenmue_Limiter.want_bypass = *(bool *)val;

          if (SK_Shenmue_Limiter.enabled ==
              SK_Shenmue_Limiter.want_bypass)
          {
            SK_Shenmue_Limiter.toggle ();
          }
        }
      }

      return true;
    }
  } static stay_a_while_and_listen;


  pVarWideCutscenes =
    new SK_IVarStub <bool> (
      (bool *)&__SK_SHENMUE_FullAspectCutscenes,
              &stay_a_while_and_listen
    );

  pVarBypassLimiter =
    new SK_IVarStub <bool> (
      (bool *)&SK_Shenmue_Limiter.want_bypass,
              &stay_a_while_and_listen
    );

  cp->AddVariable ("Shenmue.NoCrops",       pVarWideCutscenes);
  cp->AddVariable ("Shenmue.BypassLimiter", pVarBypassLimiter);


  if (SK_Shenmue_Limiter.want_bypass)
  {
    if (config.render.framerate.target_fps == 0.0f)
      cp->ProcessCommandLine ("TargetFPS 30.0");

    else
    {
      cp->ProcessCommandFormatted (
        "TargetFPS %f", config.render.framerate.target_fps
      );
    }
  }
}

bool
SKX_Keybinding (SK_Keybind* binding, sk::ParameterStringW* param)
{
  if (param == nullptr)
    return false;

  std::string label =
    SK_WideCharToUTF8 (binding->human_readable) + "###";
              label += binding->bind_name;

  if (SK_ImGui_KeybindSelect (binding, label.c_str ()))
    ImGui::OpenPopup (        binding->bind_name);

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

bool
SK_SM_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("Shenmue I & II", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    if (SK_GetCurrentGameID () == SK_GAME_ID::Shenmue)
    {
      bSK_SHENMUE_FullAspectCutscenes =
        ( ReadAcquire (&__SK_SHENMUE_FullAspectCutscenes) != 0 );

      bool changed =
        ImGui::Checkbox ( "Enable 16:9 Aspect Ratio Cutscenes",
                          &bSK_SHENMUE_FullAspectCutscenes );

      //ImGui::SameLine ();

      //static std::set <SK_ConfigSerializedKeybind *>
      //  keybinds = {
      //    &config.steam.screenshots.game_hud_free_keybind
      //  };

      ////ImGui::SameLine   ();
      ////ImGui::BeginGroup ();
      ////for ( auto& keybind : keybinds )
      ////{
      ////  ImGui::Text          ( "%s:  ",
      ////                        keybind->bind_name );
      ////}
      ////ImGui::EndGroup   ();
      ////ImGui::SameLine   ();
      ////ImGui::BeginGroup ();
      ////for ( auto& keybind : keybinds )
      ////{
      ////  Keybinding ( keybind, keybind->param );
      ////}
      ////ImGui::EndGroup   ();

      if (SK_Shenmue_Limiter.branch_addr != nullptr)
      {
        bool bypass =
          (! SK_Shenmue_Limiter.enabled);

        bool want_change =
          ImGui::Checkbox ( "Complete Framerate Limit Bypass###SHENMUE_FPS_BYPASS",
                           &bypass );

        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip ( "Limiter Branch Addr. location %ph",
                                SK_Shenmue_Limiter.branch_addr );
        }

        if (bypass)
        {
          ImGui::SameLine ();
          ImGui::Checkbox ("Use  -mode timer", &SK_Shenmue_UseNtDllQPC);
        }

        if (want_change)
        {
          SK_Shenmue_Limiter.toggle ();

          SK_Shenmue_Limiter.want_bypass =
            (! SK_Shenmue_Limiter.enabled);

          changed = true;
        }
      }

      if (SK_Shenmue_Limiter.enabled)
      {
        changed |=
          ImGui::SliderFloat ( "30 FPS Clock Fuzzing",
                                 &__SK_SHENMUE_ClockFuzz, -20.f, 200.f );

        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip ( "Ancient Chinese Secret Technique for Pacifying "
                              "Uncooperative Framerate Limiters" );
        }
      }

      if (changed)
      {
        _SK_SM_BypassLimiter->store      ((! SK_Shenmue_Limiter.enabled));
        _SK_SM_FullAspectCutscenes->store ( bSK_SHENMUE_FullAspectCutscenes);
        _SK_SM_ClockFuzz->store          ( __SK_SHENMUE_ClockFuzz);

        SK_GetDLLConfig   ()->write (
          SK_GetDLLConfig ()->get_filename ()
        );

        if (bSK_SHENMUE_FullAspectCutscenes)
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
        else
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);

        InterlockedExchange ( &__SK_SHENMUE_FullAspectCutscenes,
                                bSK_SHENMUE_FullAspectCutscenes ? 1L : 0L );
      }
    }

    ImGui::TreePop  (  );
  }

  return true;


}

sk::ParameterBool*  _SK_ACO_AlternateTaskScheduling;
sk::ParameterFloat* _SK_ACO_AutoRebalanceInterval;

void
SK_ACO_PlugInInit (void)
{
  plugin_mgr->config_fns.emplace (SK_ACO_PlugInCfg);

  __SK_MHW_KillAntiDebug = false;

  _SK_ACO_AlternateTaskScheduling =
    _CreateConfigParameterBool  ( L"AssassinsCreed.Threads",
                                  L"AltTaskSchedule", __SK_MHW_KillAntiDebug,
                                  L"Make Task Threads More Cooperative" );

  _SK_ACO_AutoRebalanceInterval =
    _CreateConfigParameterFloat ( L"AssassinsCreed.Threads",
                                  L"AutoRebalanceInterval", __SK_Thread_RebalanceEveryNSeconds,
                                  L"Periodically Rebalance Task Threads" );

  SK_SaveConfig ();
}

bool
SK_ACO_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("Assassin's Creed Odyssey / Origins", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    bool changed = false;

    if (ImGui::Checkbox ("Alternate Task Scheduling", &__SK_MHW_KillAntiDebug))
    {
      changed = true;
      _SK_ACO_AlternateTaskScheduling->store (__SK_MHW_KillAntiDebug);
    }

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip    ();
      ImGui::TextUnformatted ("Causes task threads to yield CPU time to other threads more often");
      ImGui::Separator       ();
      ImGui::BulletText      ("The task threads are not switching to other threads cooperatively and that produces high CPU load on lower-end systems.");
      ImGui::BulletText      ("Try both on/off, I cannot give one-answer-fits-all advice about this setting.");
      ImGui::EndTooltip      ();
    }

    ImGui::PushStyleColor  (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.18f, 0.85f, 0.95f));
    ImGui::TextUnformatted ("Thread Rebalancing is CRITICAL for AMD Ryzen CPUs");
    ImGui::PopStyleColor   ();

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip    ();
      ImGui::TextUnformatted ("Ryzen CPUs need this because of a scheduler bug in Windows that tries to run all threads on a single CPU core.");
      ImGui::Separator       ();
      ImGui::BulletText      ("Other CPU brands and models may see marginal improvements by enabling auto-rebalance.");
      ImGui::EndTooltip      ();
    }

    SK_ImGui_RebalanceThreadButton ();

    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("Redistribute CPU core assignment based on CPU usage history.");

    ImGui::SameLine ();


    if (ImGui::SliderFloat ("Re-Balance Interval", &__SK_Thread_RebalanceEveryNSeconds, 0.0f, 60.0f, "%.3f Seconds"))
    {
      changed = true;
      _SK_ACO_AutoRebalanceInterval->store (__SK_Thread_RebalanceEveryNSeconds);
    }

    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("0=Disabled, anything else periodically redistributes threads for optimal CPU utilization.");

    if (changed)
      SK_SaveConfig ();

    ImGui::TreePop  (  );
  }

  return true;
}
#endif


#ifdef _M_AMD64
void __stdcall
SK_HatsuneMiku_BeginFrame (void)
{
  // Skip a few frames before doing the pointer scan
  //
  if (SK_GetFramesDrawn () < 333)
    return;

  // 1.00: 0x14B2A78
  // 1.01: 0x14ACA68
  // 1.02, 1.03: 0x14ABBB8
  static uint32_t* puiGameLimit =
        (uint32_t *)((uintptr_t)SK_Debug_GetImageBaseAddr () + 0x14ABBB8);

  // Pointer at DivaMegaMix.exe, Offset=780h
  // 1.00: 0x114EFF8
  // 1.01: 0x1148FC8
  // 1.02, 1.03: 0x11481E8
  static auto menu_flag_addr =
      *(uintptr_t *)((uintptr_t)SK_Debug_GetImageBaseAddr () + 0x11481E8) + 0x780;

  // Menu is Active, 60 FPS (or lower) framerate cap is required
  if ((*(uint8_t *)menu_flag_addr) & 0x1)
  {
    __target_fps =
      std::min ( 60.0f,  config.render.framerate.target_fps <= 0.0f ?
                 60.0f : config.render.framerate.target_fps );

    *puiGameLimit = 60;
  }

  // Gameplay: Game needs no cap
  else
  {
    __target_fps =
      config.render.framerate.target_fps;

    *puiGameLimit = 0;
  }
}
#endif


#include <SpecialK/render/dxgi/dxgi_util.h>

bool
SK_D3D11_SanitizeFP16RenderTargets ( ID3D11DeviceContext *pDevCtx,
                                     UINT                 dev_idx )
{
  if (dev_idx == UINT_MAX)
  {
    dev_idx =
      SK_D3D11_GetDeviceContextHandle (pDevCtx);
  }

  // This Pixel Shader is the source of all artifacts
  if ( 0xeef0923 ==
         SK_D3D11_Shaders->pixel.current.shader [dev_idx] )
  {
    SK_ComPtr <ID3D11RenderTargetView> pRTV;
    pDevCtx->OMGetRenderTargets  ( 1, &pRTV.p, nullptr ); 
    
    if (pRTV.p != nullptr)
    {
      SK_ComPtr <ID3D11Texture2D>     pTexCopy;
      SK_ComPtr <ID3D11Device>            pDev;
      pDevCtx->GetDevice                (&pDev.p);
      SK_ComPtr <ID3D11Resource>          pRes;
      pRTV->GetResource                 (&pRes.p);
      SK_ComQIPtr <ID3D11Texture2D> pTex (pRes.p);

      if ( pRes.p != nullptr &&
           pTex.p != nullptr &&
           pDev.p != nullptr )
      {
        D3D11_TEXTURE2D_DESC    texDesc = { };
        pTex->GetDesc         (&texDesc);
        pDev->CreateTexture2D (&texDesc, nullptr,
           &pTexCopy.p);
        if (pTexCopy.p != nullptr)
        {
          pDevCtx->CopyResource (
            pTexCopy.p, pTex.p
          );
  
          return
            SK_D3D11_BltCopySurface (
              pTexCopy, pTex
            );
        }
      }
    }
  }

  return false;
}


#ifdef _M_AMD64
bool SK_SO2R_UltraWidescreen = false;

void SK_SO2R_InitPlugin (void)
{
  auto dll_ini =
    SK_GetDLLConfig ();

  SK_SO2R_UltraWidescreen =
    SK_IsTrue (
      dll_ini->get_section (L"SO2R.PlugIn").
               get_cvalue  (L"UnlockAspectRatio").c_str ()
    );

  if (SK_SO2R_UltraWidescreen)
    InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);

  plugin_mgr->config_fns.emplace (SK_SO2R_PlugInCfg);
}

bool SK_SO2R_PlugInCfg (void)
{
  auto dll_ini =
    SK_GetDLLConfig ();

  if (ImGui::CollapsingHeader ("STAR OCEAN THE SECOND STORY R", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    if (ImGui::Checkbox ("Enable Wide Aspect Ratios", &SK_SO2R_UltraWidescreen))
    {
      dll_ini->get_section (L"SO2R.PlugIn").
             add_key_value (L"UnlockAspectRatio",
      SK_SO2R_UltraWidescreen ? L"true"
                              : L"false");

      if (SK_SO2R_UltraWidescreen) InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
      else                         InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);

      dll_ini->write ();
    }

    if (ImGui::IsItemHovered ())
    {
      ImGui::SetTooltip ("Some visual glitches may occur.");
    }

    ImGui::TreePop  ();

    return false;
  }

  return true;
}

bool
SK_SO2R_DrawHandler (ID3D11DeviceContext *pDevCtx, uint32_t current_ps, int num_verts)
{
  if (pDevCtx != nullptr && SK_SO2R_UltraWidescreen && current_ps == 0x7ee4636e)
  {
    SK_ComPtr <ID3D11ShaderResourceView>  pSRV;
    pDevCtx->PSGetShaderResources (0, 1, &pSRV.p);
  
    if (pSRV.p != nullptr)
    {
      SK_ComPtr <ID3D11Resource>
                          pRes;
      pSRV->GetResource (&pRes.p);
  
      SK_ComQIPtr <ID3D11Texture2D>
          pTex (pRes);
      if (pTex != nullptr)
      {
        D3D11_TEXTURE2D_DESC
                        texDesc = { };
        pTex->GetDesc (&texDesc);
  
        if (texDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM && (num_verts == 24 || num_verts == 6))
        {
          UINT       num_vp =   1;
          D3D11_VIEWPORT vp = { };
          pDevCtx->RSGetViewports (&num_vp, &vp);

          if ( vp.TopLeftX != 0.0f ||
               vp.TopLeftY != 0.0  )
          {
          }

          else
          {
            return true;
          }
          //SK_LOGi0 (L"DrawType=%d, Verts=%d", draw_type, num_verts);
        }
      }
    }
  }

  return false;
}
#endif

WNDPROC SK_Metaphor_OrigWndProc = nullptr;
LRESULT
CALLBACK
SK_Metaphor_WindowProc ( _In_  HWND   hWnd,
                         _In_  UINT   uMsg,
                         _In_  WPARAM wParam,
                         _In_  LPARAM lParam )
{
  switch (uMsg)
  {
    case WM_CLOSE:
    {
      if (config.input.keyboard.override_alt_f4 && hWnd == game_window.hWnd)
      {
        SK_ImGui_WantExit = true;
      
        if (! config.input.keyboard.catch_alt_f4)
        {
          return IsWindowUnicode (hWnd)                ?
           DefWindowProcW (hWnd, uMsg, wParam, lParam) :
           DefWindowProcA (hWnd, uMsg, wParam, lParam);
        }
      
        return 0;
      }
    } break;
    default:
      break;
  }

  return SK_Metaphor_OrigWndProc (hWnd, uMsg, wParam, lParam);
}


__forceinline
static void
SK_AMD_MWAITX (INT64 qpcTarget = 0)
{
  if (! config.render.framerate.use_amd_mwaitx)
    return YieldProcessor ();

  static alignas(64) uint64_t monitor = 0ULL;

  _mm_monitorx (&monitor, 0, 0);
  _mm_mwaitx   (0x2, 0, (qpcTarget > 0) ? ((DWORD)qpcTarget * SK_PerfFreqInTsc + 1) : SK_PerfFreqInTsc);
}

bool
SK_CPU_IsZen (void);

__forceinline
static void
SK_YieldProcessor (INT64 qpcTarget = 0)
{
  static const bool bIsCompatibleZen =
    SK_CPU_IsZen () && SK_CPU_HasMWAITX;

  if (bIsCompatibleZen)
    SK_AMD_MWAITX (qpcTarget);
  else YieldProcessor ();
}

SleepEx_pfn SK_Metaphor_SleepEx_Original = nullptr;

DWORD
WINAPI
SK_Metaphor_SleepEx (DWORD dwMilliseconds, BOOL bAlertable)
{
  if (dwMilliseconds <= 1)
  {
    static thread_local DWORD  sleep0Count      = 0;
    static thread_local DWORD  sleep1Count      = 0;
    static thread_local UINT64 lastSkippedFrame = 0;

    if (dwMilliseconds == 0)
    {
      if (sleep0Count++ > 65536 && lastSkippedFrame != SK_GetFramesDrawn ())
      {
        sleep0Count = 0;
        SwitchToThread ();
      }

      YieldProcessor ();
      return 0;
    }

    dwMilliseconds = 0;

    // Prefer to keep this thread busy, but prevent a complete runaway...
    if (sleep1Count++ > 0 && lastSkippedFrame == SK_GetFramesDrawn ())
    {
      sleep1Count = 0;

      return
        SleepEx_Original (dwMilliseconds, bAlertable);
    }

    lastSkippedFrame = SK_GetFramesDrawn ();

    // Micro sleep on AMD CPUs for power efficiency on handhelds...
    SK_YieldProcessor (SK_QpcTicksPerMs/1000);

    return 0;
  }

  return
    SK_Metaphor_SleepEx_Original (dwMilliseconds, bAlertable);
}

void
SK_Metaphor_InitPlugin (void)
{
  extern LRESULT
  CALLBACK
  SK_DetourWindowProc ( _In_  HWND   hWnd,
                        _In_  UINT   uMsg,
                        _In_  WPARAM wParam,
                        _In_  LPARAM lParam );

  extern DWORD WINAPI SleepEx_Detour (DWORD, BOOL);

  SK_CreateFuncHook (        L"SK_DetourWindowProc",
                               SK_DetourWindowProc,
                               SK_Metaphor_WindowProc,
      static_cast_p2p <void> (&SK_Metaphor_OrigWndProc) );
  MH_QueueEnableHook   (       SK_DetourWindowProc      );

  SK_CreateFuncHook (        L"SleepEx_Detour",
                               SleepEx_Detour,
                               SK_Metaphor_SleepEx,
      static_cast_p2p <void> (&SK_Metaphor_SleepEx_Original) );
  MH_QueueEnableHook   (       SleepEx_Detour                );
  SK_ApplyQueuedHooks  ();
}