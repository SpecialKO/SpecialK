/**
 * This file is part of Special K.
 *
 * Special K is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Special K is distributed in the hope that it will be useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Special K.
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/
#include <SpecialK/widgets/widget.h>
#include <SpecialK/utility.h>
#include <SpecialK/thread.h>
#include <SpecialK/diagnostics/debug_utils.h>
#include <SpecialK/diagnostics/crash_handler.h>
#include <SpecialK/performance/gpu_monitor.h>
#include <SpecialK/control_panel.h>

#include  <concurrent_unordered_map.h>

#include <processthreadsapi.h>

typedef LONG NTSTATUS;
typedef      NTSTATUS (WINAPI *NtQueryInformationThread_pfn)(HANDLE,/*THREADINFOCLASS*/LONG,DWORD_PTR*,ULONG,PULONG);

#define ThreadQuerySetWin32StartAddress 9


enum THREAD_INFORMATION_CLASS_EX {
//ThreadMemoryPriority      = 0,
//ThreadAbsoluteCpuPriority = 1,
  ThreadDynamicCodePolicy   = 2,
  ThreadPowerThrottling     = 3,
};

typedef struct _THREAD_POWER_THROTTLING_STATE {
  ULONG Version;
  ULONG ControlMask;
  ULONG StateMask;
} THREAD_POWER_THROTTLING_STATE, *PTHREAD_POWER_THROTTLING_STATE;

#define THREAD_POWER_THROTTLING_CURRENT_VERSION 1
#define THREAD_POWER_THROTTLING_EXECUTION_SPEED 1
#define THREAD_POWER_THROTTLING_VALID_FLAGS 1

typedef BOOL (WINAPI *GetThreadInformation_pfn)(HANDLE, THREAD_INFORMATION_CLASS_EX, LPVOID, DWORD);
typedef BOOL (WINAPI *SetThreadInformation_pfn)(HANDLE, THREAD_INFORMATION_CLASS_EX, LPVOID, DWORD);

// Windows 8+
GetThreadInformation_pfn k32GetThreadInformation = nullptr;
SetThreadInformation_pfn k32SetThreadInformation = nullptr;


extern iSK_INI* osd_ini;

struct SKWG_Thread_Entry
{
  HANDLE hThread;
  DWORD  dwTid;

  struct runtimes_s
  {
    FILETIME created;
    FILETIME exited;
    FILETIME user;
    FILETIME kernel;

    long double percent_user;
    long double percent_kernel;

    struct
    {
      FILETIME user;
      FILETIME kernel;
    } snapshot;
  } runtimes;

  // Last time percentage was non-zero; used to hide inactive threads
  DWORD last_nonzero;
  bool  exited;

  bool  power_throttle;
  DWORD orig_prio;

  bool         self_titled;
  std::wstring name;
};

bool
SK_Thread_HasCustomName (DWORD dwTid);

std::map <DWORD,    SKWG_Thread_Entry*> SKWG_Threads;
std::map <LONGLONG, SKWG_Thread_Entry*> SKWG_Ordered_Threads;
std::map <DWORD,    BOOL>               SKWG_SuspendedThreads;

#include <process.h>
#include <tlhelp32.h>

class SKWG_Thread_Profiler : public SK_Widget
{
public:
  SKWG_Thread_Profiler (void) : SK_Widget ("Thread Profiler")
  {
    SK_ImGui_Widgets.thread_profiler = this;

    setAutoFit (true).setDockingPoint (DockAnchor::West).setClickThrough (true);
  };

  virtual void run (void) override
  {
    SK_RunOnce (
      k32SetThreadInformation =
        (SetThreadInformation_pfn)GetProcAddress (
          GetModuleHandle (L"Kernel32.dll"),
                            "SetThreadInformation"
        )
    );

    SK_RunOnce (
      k32GetThreadInformation =
        (GetThreadInformation_pfn)GetProcAddress (
          GetModuleHandle (L"Kernel32.dll"),
                            "GetThreadInformation"
        )
    );

    // Snapshotting is _slow_, so only do it when a thread has been created...
    extern volatile LONG lLastThreadCreate;
    static          LONG lLastThreadRefresh = 0;

    LONG last = ReadAcquire (&lLastThreadCreate);

    if (last == lLastThreadRefresh)
    {
      return;
    }

    lLastThreadRefresh =
      last;

    CHandle hSnap (
      CreateToolhelp32Snapshot (TH32CS_SNAPTHREAD, 0)
    );

    if (hSnap != INVALID_HANDLE_VALUE)
    {
      THREADENTRY32 tent        = {                  };
                    tent.dwSize = sizeof THREADENTRY32;

      if (Thread32First (hSnap, &tent))
      {
        do
        {
          if ( tent.dwSize >= FIELD_OFFSET (THREADENTRY32, th32OwnerProcessID) +
                                    sizeof (tent.th32OwnerProcessID) )
          {
            if (tent.th32OwnerProcessID == GetCurrentProcessId ())
            {
              if (! SKWG_Threads.count (tent.th32ThreadID))
              {
                SKWG_Thread_Entry::runtimes_s runtimes = { };

                SKWG_Thread_Entry *ptEntry =
                  new SKWG_Thread_Entry {
                                   INVALID_HANDLE_VALUE,
                                      tent.th32ThreadID,
                                      { { 0 , 0 } , { 0 , 0 },
                                        { 0 , 0 } , { 0 , 0 }, 0.0, 0.0,
                                              { 0 , 0 } },    0, false,
                                                false, false, 0,
                     SK_Thread_GetName (tent.th32ThreadID)
                  };

                extern std::wstring
                SK_Thread_GetName (DWORD dwTid);

                SKWG_Threads.insert ( 
                  std::make_pair ( tent.th32ThreadID, ptEntry )
                );
              }
            }
          }

          tent.dwSize = sizeof (tent);
        } while (Thread32Next (hSnap, &tent));
      }

      SKWG_Ordered_Threads.clear ();

      for ( auto& it : SKWG_Threads )
      {
        FILETIME ftCreate, ftExit,
                 ftKernel, ftUser;

        GetThreadTimes (
              OpenThread ( THREAD_QUERY_INFORMATION,
                             FALSE,
                               it.second->dwTid
                         ),   &ftCreate, &ftExit,
          &ftKernel,
          &ftUser
        );

        LARGE_INTEGER liCreate;
                      liCreate.HighPart = ftCreate.dwHighDateTime;
                      liCreate.LowPart  = ftCreate.dwLowDateTime;

        SKWG_Ordered_Threads [liCreate.QuadPart] =
          it.second;
      }
    }
  }


  virtual void draw (void) override
  {
    if (! ImGui::GetFont ()) return;


    bool drew_tooltip = false;

    auto ThreadMemTooltip = [&](DWORD dwSelectedTid) ->
    void
    {
      if (drew_tooltip)
        return;

      drew_tooltip= true;

      SK_TLS* pTLS =
        SK_TLS_BottomEx (dwSelectedTid);

      if (pTLS != nullptr)
      {
        ImGui::BeginTooltip ();

        if (ReadAcquire64 (&pTLS->memory.global_bytes)  ||
            ReadAcquire64 (&pTLS->memory.local_bytes)   ||
            ReadAcquire64 (&pTLS->memory.virtual_bytes) ||
            ReadAcquire64 (&pTLS->memory.heap_bytes)       )
        {
          ImGui::BeginGroup   ();
          if (ReadAcquire64 (&pTLS->memory.local_bytes))   ImGui::Text ("Local Memory:\t");
          if (ReadAcquire64 (&pTLS->memory.global_bytes))  ImGui::Text ("Global Memory:\t");
          if (ReadAcquire64 (&pTLS->memory.heap_bytes))    ImGui::Text ("Heap Memory:\t");
          if (ReadAcquire64 (&pTLS->memory.virtual_bytes)) ImGui::Text ("Virtual Memory:\t");
          ImGui::EndGroup     ();

          ImGui::SameLine     ();

          ImGui::BeginGroup   ();
          if (ReadAcquire64 (&pTLS->memory.local_bytes))   ImGui::Text ("%ws", SK_File_SizeToStringF (ReadAcquire64 (&pTLS->memory.local_bytes),   2, 3).c_str ());
          if (ReadAcquire64 (&pTLS->memory.global_bytes))  ImGui::Text ("%ws", SK_File_SizeToStringF (ReadAcquire64 (&pTLS->memory.global_bytes),  2, 3).c_str ());
          if (ReadAcquire64 (&pTLS->memory.heap_bytes))    ImGui::Text ("%ws", SK_File_SizeToStringF (ReadAcquire64 (&pTLS->memory.heap_bytes),    2, 3).c_str ());
          if (ReadAcquire64 (&pTLS->memory.virtual_bytes)) ImGui::Text ("%ws", SK_File_SizeToStringF (ReadAcquire64 (&pTLS->memory.virtual_bytes), 2, 3).c_str ());
          ImGui::EndGroup     ();

          ImGui::SameLine     ();

          ImGui::BeginGroup   ();
          if (ReadAcquire64 (&pTLS->memory.local_bytes))   ImGui::Text ("\t(Lifetime)");
          if (ReadAcquire64 (&pTLS->memory.global_bytes))  ImGui::Text ("\t(Lifetime)");
          if (ReadAcquire64 (&pTLS->memory.heap_bytes))    ImGui::Text ("\t(Lifetime)");
          if (ReadAcquire64 (&pTLS->memory.virtual_bytes)) ImGui::Text ("\t(In-Use Now)");
          ImGui::EndGroup     ();
        }

        if ( ReadAcquire64 (&pTLS->disk.bytes_read)    > 0 ||
             ReadAcquire64 (&pTLS->disk.bytes_written) > 0    )
        {
          ImGui::Separator ();

          ImGui::BeginGroup ();
          if (ReadAcquire64 (&pTLS->disk.bytes_read))    ImGui::Text ("File Reads:\t\t\t");
          if (ReadAcquire64 (&pTLS->disk.bytes_written)) ImGui::Text ("File Writes:\t\t\t");
          ImGui::EndGroup   ();

          ImGui::SameLine   ();

          ImGui::BeginGroup ();
          if (ReadAcquire64 (&pTLS->disk.bytes_read))    ImGui::Text ("%ws", SK_File_SizeToStringF (ReadAcquire64 (&pTLS->disk.bytes_read),    2, 3).c_str  ());
          if (ReadAcquire64 (&pTLS->disk.bytes_written)) ImGui::Text ("%ws", SK_File_SizeToStringF (ReadAcquire64 (&pTLS->disk.bytes_written), 2, 3).c_str  ());
          ImGui::EndGroup   ();
        }

        if ( ReadAcquire64 (&pTLS->net.bytes_received) > 0 ||
             ReadAcquire64 (&pTLS->net.bytes_sent)     > 0 )
        {
          ImGui::Separator ();

          ImGui::BeginGroup ();
          if (ReadAcquire64 (&pTLS->net.bytes_sent))     ImGui::Text ("Network Sent:\t");
          if (ReadAcquire64 (&pTLS->net.bytes_received)) ImGui::Text ("Network Received:\t");
          ImGui::EndGroup   ();

          ImGui::SameLine   ();

          ImGui::BeginGroup ();
          if (ReadAcquire64 (&pTLS->net.bytes_sent))     ImGui::Text ("%ws", SK_File_SizeToStringF (ReadAcquire64 (&pTLS->net.bytes_sent),     2, 3).c_str  ());
          if (ReadAcquire64 (&pTLS->net.bytes_received)) ImGui::Text ("%ws", SK_File_SizeToStringF (ReadAcquire64 (&pTLS->net.bytes_received), 2, 3).c_str  ());
          ImGui::EndGroup   ();
        }

        ImGui::EndTooltip   ();
      }
    };

           DWORD dwExitCode    = 0;
    static DWORD dwSelectedTid = 0;
    static UINT  uMaxCPU       = 0;

    static bool show_exited_threads = false;
    static bool hide_inactive       = true;
    static bool reset_stats         = true;

    bool clear_counters = false;

    //ImGui::BeginChild ("Thread_List",   ImVec2 (0,0), false, ImGuiWindowFlags_NavFlattened | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::BeginGroup ();

    ImGui::Checkbox ("Show Finished", &show_exited_threads);
    ImGui::SameLine ();
    ImGui::Checkbox ("Hide Inactive", &hide_inactive);
    ImGui::SameLine ();
    ImGui::Checkbox ("Reset Stats Every 30 Seconds", &reset_stats);
    ImGui::SameLine ();
    if (ImGui::Button ("Reset Performance Counters"))
      clear_counters = true;

    ImGui::Separator ();

    ImGui::EndGroup ();

    ImGui::BeginGroup ();
  //ImGui::BeginChildFrame (ImGui::GetID ("Thread_List2"), ImVec2 (0,0), ImGuiWindowFlags_NavFlattened | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_HorizontalScrollbar);

    if (ImGui::BeginPopup ("ThreadInspectPopup"))
    {
      CHandle hSelectedThread;
              hSelectedThread.m_h = INVALID_HANDLE_VALUE;

      if (dwSelectedTid != 0)
      {
        hSelectedThread.m_h =
          OpenThread ( THREAD_QUERY_INFORMATION |
                       THREAD_SET_INFORMATION   |
                       THREAD_SUSPEND_RESUME, FALSE, dwSelectedTid );

        if (GetProcessIdOfThread (hSelectedThread) != GetCurrentProcessId ())
        {
          hSelectedThread.Close ();
          hSelectedThread.m_h = INVALID_HANDLE_VALUE;
        }
      }

      bool active_selection =
        (                     hSelectedThread != INVALID_HANDLE_VALUE &&
          GetExitCodeThread ( hSelectedThread, &dwExitCode )          &&
                                                dwExitCode == STILL_ACTIVE
        );

      if (active_selection)
      {
        ImGui::BeginGroup ();

        bool suspended =
          SKWG_SuspendedThreads [dwSelectedTid];

        if ( ImGui::Button ( suspended ?  "Resume this Thread" :
                                          "Suspend this Thread" ) )
        {
          suspended = (! suspended);

          if (suspended)
          {
            struct suspend_params_s
              { DWORD* pdwTid;
                ULONG  frames; } static params { &dwSelectedTid,
                                                  SK_GetFramesDrawn () };

            SKWG_SuspendedThreads [dwSelectedTid] = true;

            static HANDLE hThreadRecovery = INVALID_HANDLE_VALUE;
            static HANDLE hRecoveryEvent  = INVALID_HANDLE_VALUE;

            if (hThreadRecovery == INVALID_HANDLE_VALUE)
            {
              hRecoveryEvent  =
              CreateEvent  (nullptr, FALSE, TRUE, nullptr);
              hThreadRecovery =
              CreateThread (nullptr, 0, [](LPVOID user) -> DWORD
              {
                SetCurrentThreadDescription  (L"[SK] Thread Profiler Watchdog Timer");
                SK_Thread_SetCurrentPriority (THREAD_PRIORITY_LOWEST);

                while (true)
                {
                  WaitForSingleObjectEx (hRecoveryEvent, INFINITE, TRUE);

                  extern volatile LONG __SK_DLL_Ending;

                  if (ReadAcquire (&__SK_DLL_Ending))
                    break;

                  DWORD dwTid = *((suspend_params_s *)user)->pdwTid;

                  CHandle hThread__ (
                    OpenThread ( THREAD_SUSPEND_RESUME,
                                   FALSE,
                                     dwTid )
                  );

                  if (SuspendThread (hThread__) != (DWORD)-1)
                  {
                    SleepEx (90, TRUE);

                    if (SK_GetFramesDrawn () <= ((suspend_params_s *)user)->frames)
                    {
                      ResumeThread (hThread__);

                      SKWG_SuspendedThreads [dwTid] = false;
                    }
                  }

                  else
                    SKWG_SuspendedThreads [dwTid] = false;
                }

                CloseHandle (hRecoveryEvent);
                             hRecoveryEvent = INVALID_HANDLE_VALUE;

                SK_Thread_CloseSelf ();

                return 0;
              }, (LPVOID)&params, 0x00, nullptr);
            }

            else if (hRecoveryEvent != INVALID_HANDLE_VALUE)
            {
              params.frames = SK_GetFramesDrawn ();
              params.pdwTid = &dwSelectedTid;

              SetEvent (hRecoveryEvent);
            }
          }

          else
          {
            if (ResumeThread (hSelectedThread) != (DWORD)-1)
            {
              SKWG_SuspendedThreads [dwSelectedTid] = false;
            }

            else
              SKWG_SuspendedThreads [dwSelectedTid] = true;
          }
        }

        SYSTEM_INFO     sysinfo = { };
        GetSystemInfo (&sysinfo);

        SK_TLS* pTLS =
          SK_TLS_BottomEx (dwSelectedTid);

        if (sysinfo.dwNumberOfProcessors > 1 &&  pTLS != nullptr)
        {
          ImGui::Separator ();

          for (DWORD_PTR j = 0; j < sysinfo.dwNumberOfProcessors; j++)
          {
            constexpr DWORD_PTR Processor0 = 0x1;

            bool affinity =
              (pTLS->scheduler.affinity_mask & (Processor0 << j)) != 0;

            PROCESSOR_NUMBER pnum = { };
            GetThreadIdealProcessorEx (hSelectedThread, &pnum);

            UINT i = 
              ( pnum.Group + pnum.Number );

            bool ideal = (i == j);

            float c_scale = 
              pTLS->scheduler.lock_affinity ? 0.5f : 1.0f;

            ImGui::TextColored ( ideal    ? ImColor (0.5f   * c_scale, 1.0f   * c_scale,   0.5f * c_scale):
                                 affinity ? ImColor (1.0f   * c_scale, 1.0f   * c_scale,   1.0f * c_scale) :
                                            ImColor (0.375f * c_scale, 0.375f * c_scale, 0.375f * c_scale),
                                   "CPU%lu",
                                     j );

            if (ImGui::IsItemClicked () && (! pTLS->scheduler.lock_affinity))
            {
              affinity = (! affinity);

              DWORD_PTR dwAffinityMask =
                pTLS->scheduler.affinity_mask;

                if (affinity) dwAffinityMask |=  (Processor0 << j);
                else          dwAffinityMask &= ~(Processor0 << j);

              SetThreadAffinityMask ( hSelectedThread, dwAffinityMask );
            }

            if (  j < (sysinfo.dwNumberOfProcessors / 2 - 1) ||
                 (j > (sysinfo.dwNumberOfProcessors / 2 - 1) && j != (sysinfo.dwNumberOfProcessors - 1) ) )
            {
              ImGui::SameLine ();
            }
          }

          ImGui::Checkbox ("Prevent changes to affinity", &pTLS->scheduler.lock_affinity);
        }
        ImGui::EndGroup ();
        ImGui::SameLine ();

        ImGui::BeginGroup ();
        ImGui::Spacing    ();
        ImGui::EndGroup   ();

        ImGui::SameLine   ();
        ImGui::BeginGroup ();
        //if (ImGui::Button ("Terminate this Thread")) TerminateThread (hThread_, 0x0);

        int sel = 0;

        DWORD dwPrio =
          GetThreadPriority (hSelectedThread);

        switch (dwPrio)
        {
          case THREAD_PRIORITY_IDLE:          sel = 0; break;
          case THREAD_PRIORITY_LOWEST:        sel = 1; break;
          case THREAD_PRIORITY_BELOW_NORMAL:  sel = 2; break;
          case THREAD_PRIORITY_NORMAL:        sel = 3; break;
          case THREAD_PRIORITY_ABOVE_NORMAL:  sel = 4; break;
          case THREAD_PRIORITY_HIGHEST:       sel = 5; break;
          case THREAD_PRIORITY_TIME_CRITICAL: sel = 6; break;
          default:                            sel = 7; break;
        }

        if (sel < 7)
        {
          if (ImGui::Combo ( "Thread Priority", &sel,
                             "Idle\0Lowest\0Below Normal\0Normal\0"
                             "Above Normal\0Highest\0Time Critical\0\0" ))
          {
            switch (sel)
            {
              case 0: dwPrio = (DWORD)THREAD_PRIORITY_IDLE;          break;
              case 1: dwPrio = (DWORD)THREAD_PRIORITY_LOWEST;        break;
              case 2: dwPrio = (DWORD)THREAD_PRIORITY_BELOW_NORMAL;  break;
              case 3: dwPrio = (DWORD)THREAD_PRIORITY_NORMAL;        break;
              case 4: dwPrio = (DWORD)THREAD_PRIORITY_ABOVE_NORMAL;  break;
              case 5: dwPrio = (DWORD)THREAD_PRIORITY_HIGHEST;       break;
              case 6: dwPrio = (DWORD)THREAD_PRIORITY_TIME_CRITICAL; break;
              default:                                               break;
            }

            SetThreadPriority (hSelectedThread, dwPrio);
          }
        }

        BOOL bDisableBoost = FALSE;

        if (GetThreadPriorityBoost (hSelectedThread, &bDisableBoost))
        {
          bool boost = (! bDisableBoost);
          if (ImGui::Checkbox ("Enable Dynamic Boost", &boost))
          {
            SetThreadPriorityBoost (hSelectedThread, ! boost);
          }
        }

        //if (k32SetThreadInformation != nullptr)
        //{
        //  THREAD_POWER_THROTTLING_STATE pthrottle = {
        //    0, 0, 0
        //  };
        //
        //  k32GetThreadInformation ( hSelectedThread,
        //    ThreadPowerThrottling, &pthrottle,
        //                    sizeof (pthrottle) );
        //  {
        //    bool throttle = 
        //      ( pthrottle.StateMask == THREAD_POWER_THROTTLING_EXECUTION_SPEED );
        //
        //    if (ImGui::Checkbox ("Enable Power Throttling", &throttle))
        //    {
        //      pthrottle.StateMask = throttle ?
        //        THREAD_POWER_THROTTLING_EXECUTION_SPEED : 0;
        //
        //      pthrottle.Version     = THREAD_POWER_THROTTLING_CURRENT_VERSION;
        //      pthrottle.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
        //
        //      k32SetThreadInformation ( hSelectedThread,
        //        ThreadPowerThrottling, &pthrottle,
        //                        sizeof (pthrottle) );
        //    }
        //  }
        //}

        bool& throttle =
          SKWG_Threads [dwSelectedTid]->power_throttle;

        if (! throttle)
          SKWG_Threads [dwSelectedTid]->orig_prio = dwPrio;

        if (ImGui::Checkbox ("Enable Power Throttling", &throttle))
        {
          SetThreadPriority ( hSelectedThread, throttle ? THREAD_MODE_BACKGROUND_BEGIN | THREAD_PRIORITY_IDLE :
                                                          THREAD_MODE_BACKGROUND_END );

          if (! throttle)
          {
            SetThreadPriority ( hSelectedThread, SKWG_Threads [dwSelectedTid]->orig_prio );
          }
        }

        ImGui::EndGroup   ();
      }

      ImGui::EndPopup ();
    }

    DWORD dwNow = timeGetTime ();

    auto IsThreadNonIdle = [&](SKWG_Thread_Entry& tent) ->
    bool
    {
      const DWORD IDLE_PERIOD = 1500UL;

      if (tent.exited && (! show_exited_threads))
        return false;

      if (! hide_inactive)
        return true;

      if (tent.last_nonzero > dwNow - IDLE_PERIOD)
        return true;

      return false;
    };

    ImGui::BeginGroup ();
    for ( auto& it : SKWG_Ordered_Threads )
    {
      it.second->hThread =
        OpenThread (THREAD_ALL_ACCESS, FALSE, it.second->dwTid);

      if (! IsThreadNonIdle (*it.second)) continue;

      HANDLE hThread = it.second->hThread;

      if (! GetExitCodeThread (hThread, &dwExitCode)) dwExitCode = 0;

      if (SKWG_SuspendedThreads [it.second->dwTid])
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.85f, 0.95f, 0.99f));
      else if (dwExitCode == STILL_ACTIVE)
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.3f, 0.95f, 0.99f));
      else
      {
        it.second->exited = true;
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.15f, 0.95f, 0.99f));
      }

      bool selected = false;
      if (ImGui::Selectable (SK_FormatString ("Thread %x###thr_%x", it.second->dwTid,it.second->dwTid).c_str (), &selected))
      {
        ImGui::PopStyleColor     ();
        ImGui::PushStyleColor    (ImGuiCol_Text, ImColor::HSV (0.5f, 0.99f, 0.999f));

        // We sure as hell cannot suspend the thread that's drawing the UI! :)
        if ( GetCurrentThreadId () != it.second->dwTid && 
             dwExitCode            == STILL_ACTIVE )
        {
          ImGui::OpenPopup ("ThreadInspectPopup");
        }

        dwSelectedTid =
          it.second->dwTid;

        ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
      }

      ImGui::PopStyleColor ();

      if (ImGui::IsItemHovered ()) ThreadMemTooltip (it.second->dwTid);
    }
    ImGui::EndGroup   (); ImGui::SameLine ();


    ImGui::BeginGroup ( );
    for ( auto& it : SKWG_Ordered_Threads )
    {
      if (! IsThreadNonIdle (*it.second)) continue;

      it.second->name        = SK_Thread_GetName       (it.second->dwTid);
      it.second->self_titled = SK_Thread_HasCustomName (it.second->dwTid);

      if (it.second->self_titled)
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.572222, 0.63f, 0.95f));
      else
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.472222, 0.23f, 0.91f));

      if ((! it.second->name.empty ()) && wcslen (it.second->name.c_str ()))
        ImGui::Text (u8R"(“%ws”)", it.second->name.c_str ());
      else
      {
        NTSTATUS  ntStatus;
        HANDLE    hDupHandle;
        DWORD_PTR pdwStartAddress; 

        NtQueryInformationThread_pfn NtQueryInformationThread =
          (NtQueryInformationThread_pfn)GetProcAddress ( GetModuleHandle (L"NtDll.dll"),
                                                           "NtQueryInformationThread" );
        HANDLE hCurrentProc =
          GetCurrentProcess ();

        HANDLE hThread = it.second->hThread;

        if (DuplicateHandle (hCurrentProc, hThread,
                              hCurrentProc, &hDupHandle,
                                THREAD_QUERY_INFORMATION, FALSE, 0 ) )
        {
          ntStatus =
            NtQueryInformationThread (  hDupHandle, ThreadQuerySetWin32StartAddress,
                                       &pdwStartAddress, sizeof (DWORD), NULL );

          char    thread_name [512] = { };
          char    szSymbol    [256] = { };
          ULONG   ulLen             = 191;

          SK::Diagnostics::CrashHandler::InitSyms ();

          ULONG
          SK_GetSymbolNameFromModuleAddr ( HMODULE hMod,   uintptr_t addr,
                                           char*   pszOut, ULONG     ulLen );

          ulLen = SK_GetSymbolNameFromModuleAddr (
                  SK_GetModuleFromAddr ((LPCVOID)pdwStartAddress),
          reinterpret_cast <uintptr_t> ((LPCVOID)pdwStartAddress),
                        szSymbol,
                          ulLen );

          if (ulLen > 0)
          {
            sprintf ( thread_name, "%s+%s",
                     SK_WideCharToUTF8 (SK_GetCallerName ((LPCVOID)pdwStartAddress)).c_str ( ),
                                                             szSymbol );
          }

          else {
            sprintf ( thread_name, "%s",
                        SK_WideCharToUTF8 (SK_GetCallerName ((LPCVOID)pdwStartAddress)).c_str () );
          }

          SK_TLS* pTLS =
            SK_TLS_BottomEx (it.second->dwTid);

          if (pTLS != nullptr)
            wcsncpy (pTLS->debug.name, SK_UTF8ToWideChar (thread_name).c_str ( ), 255);

          {
            extern concurrency::concurrent_unordered_map <DWORD, std::wstring> _SK_ThreadNames;

            if (! _SK_ThreadNames.count (it.second->dwTid))
            {
              _SK_ThreadNames [it.second->dwTid] =
                std::move (SK_UTF8ToWideChar (thread_name));
            }
          }

          CloseHandle (hDupHandle);
        }

        ImGui::Text ("");
      }

      ImGui::PopStyleColor ();

      if (ImGui::IsItemHovered ()) ThreadMemTooltip (it.second->dwTid);
    }
    ImGui::EndGroup   (); ImGui::SameLine ();


    ImGui::BeginGroup ();
    for (auto& it : SKWG_Ordered_Threads)
    {
      if (! IsThreadNonIdle (*it.second)) continue;

      HANDLE hThread = it.second->hThread;

      PROCESSOR_NUMBER pnum = { };
      GetThreadIdealProcessorEx (hThread, &pnum);

      UINT i = 
        ( pnum.Group + pnum.Number );

      ImGui::TextColored ( ImColor::HSV ( (float)( i       + 1 ) /
                                          (float)( uMaxCPU + 1 ), 0.5f, 1.0f ),
                             "CPU %lu", i );

      if (ImGui::IsItemHovered ()) ThreadMemTooltip (it.second->dwTid);

      uMaxCPU  =  std::max (uMaxCPU, i);
    }
    ImGui::EndGroup ();  ImGui::SameLine ();


    ImGui::BeginGroup ();
    for ( auto& it : SKWG_Ordered_Threads )
    {
      if (! IsThreadNonIdle (*it.second)) continue;

      HANDLE hThread = it.second->hThread;

      int dwPrio = GetThreadPriority (hThread);

      if (! it.second->power_throttle)
        it.second->orig_prio = dwPrio;

      std::string prio_txt = "";

      switch (dwPrio)
      {
        case THREAD_PRIORITY_IDLE:
          //prio_txt = std::move ("THREAD_PRIORITY_IDLE");
          prio_txt = std::move ("Idle");
          break;
        case THREAD_PRIORITY_LOWEST:
          //prio_txt = std::move ("THREAD_PRIORITY_LOWEST");
          prio_txt = std::move ("Lowest");
          break;
        case THREAD_PRIORITY_BELOW_NORMAL:
          //prio_txt = std::move ("THREAD_PRIORITY_BELOW_NORMAL");
          prio_txt = std::move ("Below Normal");
          break;
        case THREAD_PRIORITY_NORMAL:
          //prio_txt = std::move ("THREAD_PRIORITY_NORMAL");
          prio_txt = std::move ("Normal");
          break;
        case THREAD_PRIORITY_ABOVE_NORMAL:
          //prio_txt = std::move ("THREAD_PRIORITY_ABOVE_NORMAL");
          prio_txt = std::move ("Above Normal");
          break;
        case THREAD_PRIORITY_HIGHEST:
          //prio_txt = std::move ("THREAD_PRIORITY_HIGHEST");
          prio_txt = std::move ("Highest");
          break;
        case THREAD_PRIORITY_TIME_CRITICAL:
          //prio_txt = std::move ("THREAD_PRIORITY_TIME_CRITICAL");
          prio_txt = std::move ("Time Critical");
          break;
        default:
          if (! it.second->exited)
            prio_txt = SK_FormatString ("%d", dwPrio);
          else
            prio_txt = u8"ⁿ/ₐ";
          break;
      }

      ImGui::Text ("  %s  ", prio_txt.c_str ());

      if (ImGui::IsItemHovered ()) ThreadMemTooltip (it.second->dwTid);
    }
    ImGui::EndGroup (); ImGui::SameLine ();


    static DWORD    dwLastSnap = 0;
    static FILETIME ftSnapKernel, ftSnapUser;

    FILETIME ftCreate, ftExit,
             ftKernel, ftUser;

    const DWORD SNAP_FREQUENCY = 30000UL;

    GetProcessTimes (GetCurrentProcess (), &ftCreate, &ftExit, &ftKernel, &ftUser);

    if ( (reset_stats && (dwLastSnap < dwNow - SNAP_FREQUENCY)) ||
          clear_counters )
    {
      ftSnapKernel = ftKernel;
      ftSnapUser   = ftUser;
    }

    LARGE_INTEGER liKernel; liKernel.LowPart  = ftKernel.dwLowDateTime;
                            liKernel.HighPart = ftKernel.dwHighDateTime;

                            LARGE_INTEGER liSnapDeltaK;
                                          liSnapDeltaK.LowPart  = ftSnapKernel.dwLowDateTime;
                                          liSnapDeltaK.HighPart = ftSnapKernel.dwHighDateTime;

                                          liKernel.QuadPart -= liSnapDeltaK.QuadPart;

    LARGE_INTEGER liUser;   liUser.LowPart    = ftUser.dwLowDateTime;
                            liUser.HighPart   = ftUser.dwHighDateTime;

                            LARGE_INTEGER liSnapDeltaU;
                                          liSnapDeltaU.LowPart  = ftSnapUser.dwLowDateTime;
                                          liSnapDeltaU.HighPart = ftSnapUser.dwHighDateTime;

                                          liUser.QuadPart -= liSnapDeltaU.QuadPart;

    long double p_kmax = 0.0;
    long double p_umax = 0.0;

    for ( auto& it : SKWG_Ordered_Threads )
    {
      HANDLE hThread = it.second->hThread;

      GetThreadTimes ( hThread, &it.second->runtimes.created,
                                &it.second->runtimes.exited,
                                &it.second->runtimes.kernel,
                                &it.second->runtimes.user );

      if ( (reset_stats && (dwLastSnap < dwNow - SNAP_FREQUENCY)) ||
            clear_counters )
      {
        it.second->runtimes.snapshot.kernel = it.second->runtimes.kernel;
        it.second->runtimes.snapshot.user   = it.second->runtimes.user;
      }

      LARGE_INTEGER liThreadKernel, liThreadUser;

      liThreadKernel.HighPart = it.second->runtimes.kernel.dwHighDateTime;
      liThreadKernel.LowPart  = it.second->runtimes.kernel.dwLowDateTime;

      liThreadUser.HighPart   = it.second->runtimes.user.dwHighDateTime;
      liThreadUser.LowPart    = it.second->runtimes.user.dwLowDateTime;

         LARGE_INTEGER liThreadSnapDeltaK;
                       liThreadSnapDeltaK.LowPart  = it.second->runtimes.snapshot.kernel.dwLowDateTime;
                       liThreadSnapDeltaK.HighPart = it.second->runtimes.snapshot.kernel.dwHighDateTime;

                       liThreadKernel.QuadPart -= liThreadSnapDeltaK.QuadPart;

         LARGE_INTEGER liThreadSnapDeltaU;
                       liThreadSnapDeltaU.LowPart  = it.second->runtimes.snapshot.user.dwLowDateTime;
                       liThreadSnapDeltaU.HighPart = it.second->runtimes.snapshot.user.dwHighDateTime;

                       liThreadUser.QuadPart -= liThreadSnapDeltaU.QuadPart;

      it.second->runtimes.percent_kernel =
        100.0 * static_cast <long double> ( liThreadKernel.QuadPart ) /
                      static_cast <long double> ( liKernel.QuadPart );

      it.second->runtimes.percent_user =
        100.0 * static_cast <long double> ( liThreadUser.QuadPart ) /
                static_cast <long double> (       liUser.QuadPart );

      p_kmax = std::fmax (p_kmax, it.second->runtimes.percent_kernel);
      p_umax = std::fmax (p_umax, it.second->runtimes.percent_user);

      if (it.second->runtimes.percent_kernel >= 0.001) it.second->last_nonzero = dwNow;
      if (it.second->runtimes.percent_user   >= 0.001) it.second->last_nonzero = dwNow;
    }


    ImGui::BeginGroup ();
    for ( auto& it : SKWG_Ordered_Threads )
    {
      if (! IsThreadNonIdle (*it.second)) continue;

      if (it.second->runtimes.percent_kernel == p_kmax)
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.0944444, 0.79f, 1.0f));

      if (it.second->runtimes.percent_kernel >= 0.001)
        ImGui::Text ("%6.2f%% Kernel", it.second->runtimes.percent_kernel);
      else
        ImGui::Text ("-");

      if (it.second->runtimes.percent_kernel == p_kmax)
        ImGui::PopStyleColor ();

      if (ImGui::IsItemHovered ()) ThreadMemTooltip (it.second->dwTid);
    }
    ImGui::EndGroup (); ImGui::SameLine ();


    ImGui::BeginGroup ();
    for ( auto& it : SKWG_Ordered_Threads )
    {
      if (! IsThreadNonIdle (*it.second)) continue;

      if (it.second->runtimes.percent_user == p_umax)
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.0944444, 0.79f, 1.0f));

      if (it.second->runtimes.percent_user >= 0.001)
        ImGui::Text ("%6.2f%% User", it.second->runtimes.percent_user);
      else
        ImGui::Text ("-");

      if (it.second->runtimes.percent_user == p_umax)
        ImGui::PopStyleColor ();

      if (ImGui::IsItemHovered ()) ThreadMemTooltip (it.second->dwTid);
    }
    ImGui::EndGroup ();    ImGui::SameLine ();


    ImGui::BeginGroup ();
    for (auto& it : SKWG_Ordered_Threads)
    {
      if (! IsThreadNonIdle (*it.second)) continue;

      HANDLE hThread = it.second->hThread;

      BOOL bDisableBoost = FALSE;

      if (GetThreadPriorityBoost (hThread, &bDisableBoost) && (! bDisableBoost))
      {
        ImGui::TextColored (ImColor::HSV (1.0f, 0.0f, 0.38f), " (Dynamic Boost)");
      }

      else
        ImGui::Text ("");

      if (ImGui::IsItemHovered ()) ThreadMemTooltip (it.second->dwTid);
    }
    ImGui::EndGroup ();    ImGui::SameLine ();


    // Used to determine whether to srhink the dialog box;
    //
    //   Waiting I/O is a status that frequently comes and goes, but we don't
    //     want the widget rapidly resizing itself, so we need a grace period.
    static DWORD dwLastWaiting = 0;
    const  DWORD WAIT_GRACE    = 666UL;

    ImGui::BeginGroup ();
    for (auto& it : SKWG_Ordered_Threads)
    {
      if (! IsThreadNonIdle (*it.second)) continue;

      HANDLE hThread  = it.second->hThread;
      BOOL   bPending = FALSE;

      if (GetThreadIOPendingFlag (hThread, &bPending) && bPending)
      {
        dwLastWaiting = dwNow;
        ImGui::TextColored (ImColor (1.0f, 0.090196f, 0.364706f), "Waiting I/O");
      }
      else if (dwNow > dwLastWaiting + WAIT_GRACE)
        ImGui::Text ("");
      else
        // Alpha: 0  ==>  Hide this text, it's for padding only.
        ImGui::TextColored (ImColor (1.0f, 0.090196f, 0.364706f, 0.0f), "Waiting I/O");

      ImGui::SameLine ();

      if (it.second->power_throttle)
        ImGui::TextColored (ImColor (0.090196f, 1.0f, 0.364706f), "Power-Throttle");
      else
        ImGui::Text ("");

      if (ImGui::IsItemHovered ()) ThreadMemTooltip (it.second->dwTid);
    }
    ImGui::EndGroup   ();

  //ImGui::EndChildFrame ();
    ImGui::EndGroup      ();

    for (auto& it : SKWG_Ordered_Threads)
    {
      if (it.second->hThread != INVALID_HANDLE_VALUE)
      {
        CloseHandle (it.second->hThread);
                     it.second->hThread = INVALID_HANDLE_VALUE;
      }
    }

    if ( (reset_stats && (dwLastSnap < dwNow - SNAP_FREQUENCY)) ||
          clear_counters )
    {
      dwLastSnap = dwNow;
    }
  }


  virtual void OnConfig (ConfigEvent event) override
  {
    switch (event)
    {
      case SK_Widget::ConfigEvent::LoadComplete:
        break;

      case SK_Widget::ConfigEvent::SaveStart:
        break;
    }
  }

protected:
private:
} __thread_profiler__;

