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

#include <SpecialK/framerate.h>
#include <SpecialK/render/backend.h>

#include <SpecialK/log.h>
#include <SpecialK/config.h>
#include <SpecialK/command.h>
#include <SpecialK/core.h>
#include <SpecialK/hooks.h>
#include <SpecialK/window.h>

#include <d3d9.h>
#include <d3d11.h>
#include <atlbase.h>

#include <SpecialK/tls.h>
#include <SpecialK/thread.h>

SK::Framerate::Stats* frame_history  = nullptr;
SK::Framerate::Stats* frame_history2 = nullptr;

using NTSTATUS = _Return_type_success_(return >= 0) LONG;

using NtQueryTimerResolution_pfn = NTSTATUS (NTAPI *)
(
  OUT PULONG              MinimumResolution,
  OUT PULONG              MaximumResolution,
  OUT PULONG              CurrentResolution
);

using NtSetTimerResolution_pfn = NTSTATUS (NTAPI *)
(
  IN  ULONG               DesiredResolution,
  IN  BOOLEAN             SetResolution,
  OUT PULONG              CurrentResolution
);

HMODULE                    NtDll                  = nullptr;
NtQueryTimerResolution_pfn NtQueryTimerResolution = nullptr;
NtSetTimerResolution_pfn   NtSetTimerResolution   = nullptr;

typedef NTSTATUS (WINAPI *NtDelayExecution_pfn)(
  IN  BOOLEAN        Alertable,
  IN  PLARGE_INTEGER DelayInterval
  );

NtDelayExecution_pfn NtDelayExecution = nullptr;


// Dispatch through the trampoline, rather than hook
//
using WaitForVBlank_pfn = HRESULT (STDMETHODCALLTYPE *)(
  IDXGIOutput *This
);
extern WaitForVBlank_pfn WaitForVBlank_Original;


SK::Framerate::EventCounter* SK::Framerate::events = nullptr;

LPVOID pfnQueryPerformanceCounter = nullptr;
LPVOID pfnSleep                   = nullptr;

Sleep_pfn                   Sleep_Original                     = nullptr;
SleepEx_pfn                 SleepEx_Original                   = nullptr;
QueryPerformanceCounter_pfn QueryPerformanceCounter_Original   = nullptr;
QueryPerformanceCounter_pfn ZwQueryPerformanceCounter          = nullptr;
QueryPerformanceCounter_pfn RtlQueryPerformanceCounter         = nullptr;
QueryPerformanceCounter_pfn QueryPerformanceFrequency_Original = nullptr;
QueryPerformanceCounter_pfn RtlQueryPerformanceFrequency       = nullptr;

LARGE_INTEGER
SK_QueryPerf ()
{
  return
    SK_CurrentPerf ();
}

HANDLE hModSteamAPI = nullptr;

LARGE_INTEGER SK::Framerate::Stats::freq;

#include <SpecialK/utility.h>
#include <SpecialK/steam_api.h>


auto
  long_double_cast =
  [](auto val) ->
    long double
    {
      return
        static_cast <long double> (val);
    };

void
SK_Thread_WaitWhilePumpingMessages (DWORD dwMilliseconds)
{
  if (! SK_Win32_IsGUIThread ())
  {
    while (GetQueueStatus (QS_ALLEVENTS) == 0)
    {
      SK_Sleep (dwMilliseconds /= 2);

      if (dwMilliseconds == 1)
      {
        SleepEx (dwMilliseconds, TRUE);
        return;
      }
    }
  }

  HWND hWndThis = GetActiveWindow ();
  bool bUnicode =
    IsWindowUnicode (hWndThis);

  auto PeekAndDispatch =
  [&]
  {
    MSG msg     = {      };
    msg.hwnd    = hWndThis;
    msg.message = WM_NULL ;

    // Avoid having Windows marshal Unicode messages like a dumb ass
    if (bUnicode)
    {
      if ( PeekMessageW ( &msg, hWndThis, 0, 0,
                                            PM_REMOVE | QS_ALLINPUT)
               &&          msg.message != WM_NULL
         )
      {
        SK_LOG0 ( ( L"Dispatched Message: %x to Unicode HWND: %x while framerate limiting!", msg.message, msg.hwnd ),
                    L"Win32-Pump" );

        DispatchMessageW (&msg);
      }
    }

    else
    {
      if ( PeekMessageA ( &msg, hWndThis, 0, 0,
                                            PM_REMOVE | QS_ALLINPUT)
               &&          msg.message != WM_NULL
         )
      {
        SK_LOG0 ( ( L"Dispatched Message: %x to ANSI HWND: %x while framerate limiting!", msg.message, msg.hwnd ),
                    L"Win32-Pump" );
        DispatchMessageA (&msg);
      }
    }
  };


  if (dwMilliseconds == 0)
  {
    PeekAndDispatch ();

    return;
  }


  LARGE_INTEGER liStart      = SK_CurrentPerf ();
  long long     liTicksPerMS = SK_GetPerfFreq ().QuadPart / 1000LL;
  long long     liEnd        = liStart.QuadPart + ( liTicksPerMS * dwMilliseconds );

  LARGE_INTEGER liNow = liStart;

  while ((liNow = SK_CurrentPerf ()).QuadPart < liEnd)
  {
    DWORD dwMaxWait =
      narrow_cast <DWORD> ((liEnd - liNow.QuadPart) / liTicksPerMS);

    if (dwMaxWait < INT_MAX)
    {
      if (MsgWaitForMultipleObjectsEx (0, nullptr, dwMaxWait, QS_ALLINPUT,
                                                              MWMO_INPUTAVAILABLE) == WAIT_OBJECT_0)
      {
        PeekAndDispatch ();
      }
    }
  }
}

bool
fix_sleep_0 = true;


float
SK_Sched_ThreadContext::most_recent_wait_s::getRate (void)
{
  if (sequence > 0)
  {
    double ms =
      SK_DeltaPerfMS (
        SK_CurrentPerf ().QuadPart - (last_wait.QuadPart - start.QuadPart), 1
      );

    return static_cast <float> ( ms / static_cast <double> (sequence) );
  }

  // Sequence just started
  return -1.0f;
}

typedef
NTSTATUS (NTAPI *NtWaitForSingleObject_pfn)(
  IN HANDLE         Handle,
  IN BOOLEAN        Alertable,
  IN PLARGE_INTEGER Timeout    // Microseconds
);

NtWaitForSingleObject_pfn
NtWaitForSingleObject_Original = nullptr;

#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L) // ntsubauth
#define STATUS_ALERTED                   ((NTSTATUS)0x00000101L)

DWORD
WINAPI
SK_WaitForSingleObject_Micro ( _In_  HANDLE          hHandle,
                               _In_ PLARGE_INTEGER pliMicroseconds )
{
  if (NtWaitForSingleObject_Original != nullptr)
  {
    NTSTATUS NtStatus =
      NtWaitForSingleObject_Original (
        hHandle,
          FALSE,
            pliMicroseconds
      );

    switch (NtStatus)
    {
      case STATUS_SUCCESS:
        return WAIT_OBJECT_0;
      case STATUS_TIMEOUT:
        return WAIT_TIMEOUT;
      case STATUS_ALERTED:
        return WAIT_IO_COMPLETION;
      case STATUS_USER_APC:
        return WAIT_IO_COMPLETION;
    }

    // ???
    return
      WAIT_FAILED;
  }

  DWORD dwMilliseconds =
    ( pliMicroseconds == nullptr ?
              INFINITE : static_cast <DWORD> (
                           pliMicroseconds->QuadPart / 1000ULL
                         )
    );

  return
    WaitForSingleObject ( hHandle,
                            dwMilliseconds );
}

DWORD
WINAPI
SK_WaitForSingleObject (_In_ HANDLE hHandle,
                        _In_ DWORD  dwMilliseconds )
{
  if (dwMilliseconds == INFINITE)
  {
    return
      SK_WaitForSingleObject_Micro ( hHandle, nullptr );
  }

  LARGE_INTEGER usecs;
                usecs.QuadPart =
                  static_cast <LONGLONG> (
                    dwMilliseconds
                  ) * 1000ULL;

  return
    SK_WaitForSingleObject_Micro ( hHandle, &usecs );
}

typedef enum _OBJECT_WAIT_TYPE {
  WaitAllObject,
  WaitAnyObject
} OBJECT_WAIT_TYPE, *POBJECT_WAIT_TYPE;

typedef
NTSTATUS (NTAPI *NtWaitForMultipleObjects_pfn)(
  IN ULONG                ObjectCount,
  IN PHANDLE              ObjectsArray,
  IN OBJECT_WAIT_TYPE     WaitType,
  IN BOOLEAN              Alertable,
  IN PLARGE_INTEGER       TimeOut OPTIONAL );

NtWaitForMultipleObjects_pfn
NtWaitForMultipleObjects_Original = nullptr;

NTSTATUS
NTAPI
NtWaitForMultipleObjects_Detour (
  IN ULONG                ObjectCount,
  IN PHANDLE              ObjectsArray,
  IN OBJECT_WAIT_TYPE     WaitType,
  IN BOOLEAN              Alertable,
  IN PLARGE_INTEGER       TimeOut OPTIONAL )
{
  if (! ( SK_GetFramesDrawn () && SK_MMCS_GetTaskCount () > 1 ) )
  {
    return
      NtWaitForMultipleObjects_Original (
           ObjectCount, ObjectsArray,
             WaitType, Alertable,
               TimeOut                  );
  }

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  if ( pTLS != nullptr &&
       pTLS->scheduler.mmcs_task > (SK_MMCS_TaskEntry *)1 )
  {
    //if (pTLS->scheduler.mmcs_task->hTask > 0)
    //    pTLS->scheduler.mmcs_task->disassociateWithTask ();
  }

  DWORD dwRet =
    NtWaitForMultipleObjects_Original (
         ObjectCount, ObjectsArray,
           WaitType, Alertable,
             TimeOut                  );

  if ( pTLS != nullptr &&
       pTLS->scheduler.mmcs_task > (SK_MMCS_TaskEntry *)1 )
  {
    auto task =
      pTLS->scheduler.mmcs_task;

    //if (pTLS->scheduler.mmcs_task->hTask > 0)
    //  task->reassociateWithTask ();

    if (InterlockedCompareExchange (&task->change.pending, 0, 1))
    {
      task->setPriority           ( task->change.priority );

      ///if (_stricmp (task->change.task0, task->task0) ||
      ///    _stricmp (task->change.task1, task->task1))
      ///{
      ///  task->disassociateWithTask  ();
      ///  task->setMaxCharacteristics (task->change.task0, task->change.task1);
      ///}
    }
  }

  return dwRet;
}

// -------------------
// This code is largely obsolete, but will rate-limit
//   Unity's Asynchronous Procedure Calls if they are
//     ever shown to devestate performance like they
//       were in PoE2.
// -------------------
//

extern volatile LONG SK_POE2_Horses_Held;
extern volatile LONG SK_POE2_SMT_Assists;
extern volatile LONG SK_POE2_ThreadBoostsKilled;
extern          bool SK_POE2_FixUnityEmployment;
extern          bool SK_POE2_Stage2UnityFix;
extern          bool SK_POE2_Stage3UnityFix;

NTSTATUS
WINAPI
NtWaitForSingleObject_Detour (
  IN HANDLE         Handle,
  IN BOOLEAN        Alertable,
  IN PLARGE_INTEGER Timeout  )
{
#ifndef _UNITY_HACK
  if (! ( SK_GetFramesDrawn () && SK_MMCS_GetTaskCount () > 1 ) )
#else
  if (  ! SK_GetFramesDrawn () )
#endif
  {
    return
      NtWaitForSingleObject_Original (
        Handle, Alertable, Timeout
      );
  }

#ifdef _UNITY_HACK
  if (bAlertable)
    InterlockedIncrement (&pTLS->scheduler.alert_waits);

  // Consider double-buffering this since the information
  //   is used almost exclusively by OHTER threads, and
  //     we have to do a synchronous copy the get at this
  //       thing without thread A murdering thread B.
  SK_Sched_ThreadContext::wait_record_s& scheduled_wait =
    (*pTLS->scheduler.objects_waited) [hHandle];

  scheduled_wait.calls++;

  if (dwMilliseconds == INFINITE)
    scheduled_wait.time = 0;//+= dwMilliseconds;

  LARGE_INTEGER liStart =
    SK_QueryPerf ();

  bool same_as_last_time =
    ( pTLS->scheduler.mru_wait.handle == hHandle );

      pTLS->scheduler.mru_wait.handle = hHandle;

  auto ret =
    NtWaitForSingleObject_Original (
      Handle, Alertable, Timeout
    );

  InterlockedAdd`4 ( &scheduled_wait.time_blocked,
                       static_cast <uint64_t> (
                         SK_DeltaPerfMS (liStart.QuadPart, 1)
                       )
                   );

  // We're waiting on the same event as last time on this thread
  if ( same_as_last_time )
  {
    pTLS->scheduler.mru_wait.last_wait = liStart;
    pTLS->scheduler.mru_wait.sequence++;
  }

  // This thread found actual work and has stopped abusing the kernel
  //   waiting on the same always-signaled event; it can have its
  //     normal preemption behavior back  (briefly anyway).
  else
  {
    pTLS->scheduler.mru_wait.start     = liStart;
    pTLS->scheduler.mru_wait.last_wait = liStart;
    pTLS->scheduler.mru_wait.sequence  = 0;
  }

  if ( ret            == WAIT_OBJECT_0 &&   SK_POE2_FixUnityEmployment &&
       dwMilliseconds == INFINITE      && ( bAlertable != FALSE ) )
  {
    // Not to be confused with the other thing
    bool hardly_working =
      (! StrCmpW (pTLS->debug.name, L"Worker Thread"));

    if ( SK_POE2_Stage3UnityFix || hardly_working )
    {
      if (pTLS->scheduler.mru_wait.getRate () >= 0.00666f)
      {
        // This turns preemption of threads in the same priority level off.
        //
        //    * Yes, TRUE means OFF.  Use this wrong and you will hurt
        //                              performance; just sayin'
        //
        if (pTLS->scheduler.mru_wait.preemptive == -1)
        {
          GetThreadPriorityBoost ( GetCurrentThread (),
                                   &pTLS->scheduler.mru_wait.preemptive );
        }

        if (pTLS->scheduler.mru_wait.preemptive == FALSE)
        {
          SetThreadPriorityBoost ( GetCurrentThread (), TRUE );
          InterlockedIncrement   (&SK_POE2_ThreadBoostsKilled);
        }

        //
        // (Everything below applies to the Unity unemployment office only)
        //
        if (hardly_working)
        {
          // Unity Worker Threads have special additional considerations to
          //   make them less of a pain in the ass for the kernel.
          //
          LARGE_INTEGER core_sleep_begin =
            SK_QueryPerf ();

          if (SK_DeltaPerfMS (liStart.QuadPart, 1) < 0.05)
          {
            InterlockedIncrement (&SK_POE2_Horses_Held);
            SwitchToThread       ();

            if (SK_POE2_Stage2UnityFix)
            {
              // Micro-sleep the core this thread is running on to try
              //   and salvage its logical (HyperThreaded) partner's
              //     ability to do work.
              //
              while (SK_DeltaPerfMS (core_sleep_begin.QuadPart, 1) < 0.0000005)
              {
                InterlockedIncrement (&SK_POE2_SMT_Assists);

                // Very brief pause that is good for next to nothing
                //   aside from voluntarily giving up execution resources
                //     on this core's superscalar pipe and hoping the
                //       related Logical Processor can work more
                //         productively if we get out of the way.
                //
                YieldProcessor       (                    );
                //
                // ^^^ Literally does nothing, but an even less useful
                //       nothing if the processor does not support SMT.
                //
              }
            }
          };
        }
      }

      else
      {
        if (pTLS->scheduler.mru_wait.preemptive == -1)
        {
          GetThreadPriorityBoost ( GetCurrentThread (),
                                  &pTLS->scheduler.mru_wait.preemptive );
        }

        if (pTLS->scheduler.mru_wait.preemptive != FALSE)
        {
          SetThreadPriorityBoost (GetCurrentThread (), FALSE);
          InterlockedIncrement   (&SK_POE2_ThreadBoostsKilled);
        }
      }
    }
  }

  // They took our jobs!
  else if (pTLS->scheduler.mru_wait.preemptive != -1)
  {
    SetThreadPriorityBoost (
      GetCurrentThread (),
        pTLS->scheduler.mru_wait.preemptive );

    // Status Quo restored: Jobs nobody wants are back and have
    //   zero future relevance and should be ignored if possible.
    pTLS->scheduler.mru_wait.preemptive = -1;
  }
#endif

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  if (! pTLS)
  {
    return
      NtWaitForSingleObject_Original (
        Handle, Alertable, Timeout
      );
  }


  if (pTLS->scheduler.mmcs_task > (SK_MMCS_TaskEntry *) 1)
  {
    //if (pTLS->scheduler.mmcs_task->hTask > 0)
    //    pTLS->scheduler.mmcs_task->disassociateWithTask ();
  }

  auto ret =
    NtWaitForSingleObject_Original (
      Handle, Alertable, Timeout
    );

  if (pTLS->scheduler.mmcs_task > (SK_MMCS_TaskEntry *) 1)
  {
    auto task =
      pTLS->scheduler.mmcs_task;

    //if (pTLS->scheduler.mmcs_task->hTask > 0)
    //                         task->reassociateWithTask ();

    if (InterlockedCompareExchange (&task->change.pending, 0, 1))
    {
      task->setPriority             ( task->change.priority );

      ///if (_stricmp (task->change.task0, task->task0) ||
      ///    _stricmp (task->change.task1, task->task1))
      ///{
      ///  task->disassociateWithTask  ();
      ///  task->setMaxCharacteristics (task->change.task0, task->change.task1);
      ///}
    }
  }

  return ret;
}


typedef BOOL (WINAPI *SwitchToThread_pfn)(void);
                      SwitchToThread_pfn
                      SwitchToThread_Original = nullptr;

#include <avrt.h>

BOOL
WINAPI
SwitchToThread_Detour (void)
{
  static bool is_mhw =
    ( SK_GetCurrentGameID () == SK_GAME_ID::MonsterHunterWorld     );
  static bool is_aco =
    ( SK_GetCurrentGameID () == SK_GAME_ID::AssassinsCreed_Odyssey );

  if (! (is_mhw || is_aco))
  {
    return
      SwitchToThread_Original ();
  }


  static volatile DWORD dwAntiDebugTid = 0;

  extern bool
  __SK_MHW_KillAntiDebug;

  if (is_mhw && __SK_MHW_KillAntiDebug)
  {
    DWORD dwTid =
      ReadULongAcquire (&dwAntiDebugTid);

    if ( dwTid == 0 ||
         dwTid == GetCurrentThreadId () )
    {
      SK_TLS *pTLS =
        SK_TLS_Bottom ();

      if ( pTLS->win32.getThreadPriority () != 
             THREAD_PRIORITY_HIGHEST )
      {
        return
          SwitchToThread_Original ();
      }

      else if (dwTid == 0)
      {
        dwTid =
          GetCurrentThreadId ();

        InterlockedExchange (&dwAntiDebugTid, dwTid);
      }

      if (pTLS->debug.tid == dwTid)
      {
        ULONG ulFrames =
          SK_GetFramesDrawn ();

        if (pTLS->scheduler.last_frame   < (ulFrames - 3) ||
            pTLS->scheduler.switch_count > 3)
        {
          pTLS->scheduler.switch_count = 0;
        }

        if (WAIT_TIMEOUT != MsgWaitForMultipleObjectsEx (0, nullptr, pTLS->scheduler.switch_count++, QS_ALLEVENTS & ~QS_INPUT, 0x0))
        {
          pTLS->scheduler.last_frame =
            ulFrames;

          return FALSE;
        }

      //SK_Sleep (pTLS->scheduler.switch_count++);

        pTLS->scheduler.last_frame =
          ulFrames;

        return
          TRUE;
      }
    }

    return
      SwitchToThread_Original ();
  }


  //SK_LOG0 ( ( L"Thread: %x (%s) is SwitchingThreads", SK_GetCurrentThreadId (), SK_Thread_GetName (GetCurrentThread ()).c_str () ),
  //         L"AntiTamper");


  BOOL bRet = FALSE;

  if (__SK_MHW_KillAntiDebug && is_aco)
  {
    config.render.framerate.enable_mmcss = true;

    SK_TLS *pTLS =
      SK_TLS_Bottom ();

    if (pTLS->scheduler.last_frame != SK_GetFramesDrawn ())
        pTLS->scheduler.switch_count = 0;


    extern SK_MMCS_TaskEntry*
      SK_MMCS_GetTaskForThreadIDEx ( DWORD dwTid, const char* name,
                                                  const char* class0,
                                                  const char* class1 );

    bRet = TRUE;

    if (pTLS->scheduler.mmcs_task > (SK_MMCS_TaskEntry *) 1)
    {
      //if (pTLS->scheduler.mmcs_task->hTask > 0)
      //    pTLS->scheduler.mmcs_task->disassociateWithTask ();
    }

    if (pTLS->scheduler.switch_count++ < 20)
    {
      if (pTLS->scheduler.switch_count < 15)
        SK_Sleep (0);
      else
        bRet = SwitchToThread_Original ();
    }
    else
      SK_Sleep (1);

    if (pTLS->scheduler.mmcs_task > (SK_MMCS_TaskEntry *)1)
    {
      auto task =
        pTLS->scheduler.mmcs_task;

      //if (pTLS->scheduler.mmcs_task->hTask > 0)
      //                         task->reassociateWithTask ();

      if (InterlockedCompareExchange (&task->change.pending, 0, 1))
      {
        task->setPriority            ( task->change.priority );

        ///if (_stricmp (task->change.task0, task->task0) ||
        ///    _stricmp (task->change.task1, task->task1))
        ///{
        ///  task->disassociateWithTask  ();
        ///  task->setMaxCharacteristics (task->change.task0, task->change.task1);
        ///}
      }
    }

    pTLS->scheduler.last_frame =
      SK_GetFramesDrawn ();
  }

  else
  {
    bRet =
      SwitchToThread_Original ();
  }

  return
    bRet;
}


void
WINAPI
SK_Sleep (DWORD dwMilliseconds)
{
  Sleep_Original != nullptr       ?
  Sleep_Original (dwMilliseconds) :
  Sleep          (dwMilliseconds);
}

void
WINAPI
SK_SleepEx (DWORD dwMilliseconds, BOOL bAlertable)
{
  SleepEx_Original != nullptr       ?
    SleepEx_Original (dwMilliseconds, bAlertable) :
    SleepEx          (dwMilliseconds, bAlertable);
}

void
WINAPI
SleepEx_Detour (DWORD dwMilliseconds, BOOL bAlertable)
{
  if (   ReadAcquire (&__SK_DLL_Ending  ) ||
      (! ReadAcquire (&__SK_DLL_Attached) ) )
  return;

  static auto& rb =
    SK_GetCurrentRenderBackend ();


  bool bWantThreadClassification =
    ( config.render.framerate.sleepless_render ||
      config.render.framerate.sleepless_window );

  bWantThreadClassification |=
    ( ( SK_GetCurrentGameID () == SK_GAME_ID::Tales_of_Vesperia ) &&
                dwMilliseconds == 1 );

  DWORD dwTid =
    bWantThreadClassification ?
        GetCurrentThreadId () : 0;

  //SK_TLS *pTLS =
  //  nullptr;

  //BOOL bGUIThread    = config.render.framerate.sleepless_window ? SK_Win32_IsGUIThread (dwTid, &pTLS)        :
  //                                                                                      false;
  BOOL bRenderThread = config.render.framerate.sleepless_render ? ((DWORD)ReadAcquire (&rb.thread) == dwTid) :
                                                                                        false;

  if (bRenderThread)
  {
    if (SK_GetCurrentGameID () == SK_GAME_ID::Tales_of_Vesperia)
    {
      extern bool __SK_TVFix_NoRenderSleep;

      if (__SK_TVFix_NoRenderSleep)
      {
        //dll_log.Log ( L"Sleep (1) - %s <%p>", SK_SummarizeCaller ().c_str (),
        //                                          _ReturnAddress () );
        return;
      }
    }

    if (config.render.framerate.sleepless_render && dwMilliseconds != INFINITE)
    {
      static bool reported = false;
            if (! reported)
            {
              dll_log.Log (L"[FrameLimit] Sleep called from render thread: %lu ms!", dwMilliseconds);
              reported = true;
            }

      SK::Framerate::events->getRenderThreadStats ().wake (dwMilliseconds);

      //if (bGUIThread)
      //  SK::Framerate::events->getMessagePumpStats ().wake (dwMilliseconds);

      if (dwMilliseconds <= 1)
      {
        SleepEx_Original (0, bAlertable);
      }

      return;
    }

    SK::Framerate::events->getRenderThreadStats ().sleep  (dwMilliseconds);
  }

  //if (bGUIThread)
  //{
  //  if (config.render.framerate.sleepless_window && dwMilliseconds != INFINITE)
  //  {
  //    static bool reported = false;
  //          if (! reported)
  //          {
  //            dll_log.Log (L"[FrameLimit] Sleep called from GUI thread: %lu ms!", dwMilliseconds);
  //            reported = true;
  //          }
  //
  //    SK::Framerate::events->getMessagePumpStats ().wake   (dwMilliseconds);
  //
  //    if (bRenderThread)
  //      SK::Framerate::events->getMessagePumpStats ().wake (dwMilliseconds);
  //
  //    SK_Thread_WaitWhilePumpingMessages (dwMilliseconds);
  //
  //    return;
  //  }
  //
  //  SK::Framerate::events->getMessagePumpStats ().sleep (dwMilliseconds);
  //}

  DWORD max_delta_time =
    narrow_cast <DWORD> (config.render.framerate.max_delta_time);

  extern bool __SK_TVFix_ActiveAntiStutter;

  if ( SK_GetCurrentGameID () == SK_GAME_ID::Tales_of_Vesperia &&
       __SK_TVFix_ActiveAntiStutter )
  {
    SK_TLS* pTLS =
      SK_TLS_Bottom ();

    std::wstring thread_name =
      pTLS->debug.name;

    if (thread_name == L"BusyThread" ||
        thread_name == L"WorkThread")
    {
      ULONG ulFrames =
        SK_GetFramesDrawn ();

      if (pTLS->scheduler.last_frame < ulFrames)
      {
        pTLS->scheduler.switch_count = 0;
      }

      pTLS->scheduler.last_frame =
        ulFrames;

      //SK_Sleep (pTLS->scheduler.switch_count++);

      if (++pTLS->scheduler.switch_count > 256)
      {
        dwMilliseconds               = 0;
        pTLS->scheduler.switch_count = 0;
      }

      else
      {
        if (! (pTLS->scheduler.switch_count % 16))
          YieldProcessor ();

        SwitchToThread ();
        return;
      }

      //max_delta_time = std::max (2UL, max_delta_time);
    }

    else if ( dwMilliseconds == 0 ||
              dwMilliseconds == 1 )
    {
      max_delta_time = std::max (2UL, max_delta_time);
      //dll_log.Log (L"Sleep %lu - %s - %x", dwMilliseconds, thread_name.c_str (), GetCurrentThreadId ());
    }
  }

  // TODO: Stop this nonsense and make an actual parameter for this...
  //         (min sleep?)
  if ( max_delta_time <= dwMilliseconds )
  {
    //dll_log.Log (L"SleepEx (%lu, %s) -- %s", dwMilliseconds, bAlertable ? L"Alertable" : L"Non-Alertable", SK_SummarizeCaller ().c_str ());
    SleepEx_Original (dwMilliseconds, bAlertable);
  }

  else
  {
    static volatile LONG __init = 0;

    double dMinRes = 0.498800;

    if (! InterlockedCompareExchange (&__init, 1, 0))
    {
      NtDll =
        LoadLibrary (L"ntdll.dll");

      NtQueryTimerResolution =
        reinterpret_cast <NtQueryTimerResolution_pfn> (
          GetProcAddress (NtDll, "NtQueryTimerResolution")
        );

      NtSetTimerResolution =
        reinterpret_cast <NtSetTimerResolution_pfn> (
          GetProcAddress (NtDll, "NtSetTimerResolution")
        );

      if (NtQueryTimerResolution != nullptr &&
          NtSetTimerResolution   != nullptr)
      {
        ULONG min, max, cur;
        NtQueryTimerResolution (&min, &max, &cur);
        dll_log.Log ( L"[  Timing  ] Kernel resolution.: %f ms",
                        static_cast <float> (cur * 100)/1000000.0f );
        NtSetTimerResolution   (max, TRUE,  &cur);
        dll_log.Log ( L"[  Timing  ] New resolution....: %f ms",
                        static_cast <float> (cur * 100)/1000000.0f );

        dMinRes =
                       static_cast <double> (cur * 100)/1000000.0;
      }
    }
    SK_TLS *pTLS = SK_TLS_Bottom ();

    //pTLS->scheduler.alert_waits++;

#if 0
    if ( ! (pTLS->scheduler.alert_waits++ % 2) )
    {
        
      LARGE_INTEGER liDelay = { };
                    liDelay.QuadPart =
        (LONGLONG)(-10000.0l * (long double)dwMilliseconds);

      if (pTLS->scheduler.sub_2ms_sleep == INVALID_HANDLE_VALUE)
      {
        // Create an unnamed waitable timer.
        pTLS->scheduler.sub_2ms_sleep =
          CreateWaitableTimer (NULL, FALSE, NULL);
      }

      LARGE_INTEGER uSecs;
      uSecs.QuadPart =
        static_cast <LONGLONG> ((long double)dwMilliseconds * 500.0l);
    
      if (pTLS->scheduler.sub_2ms_sleep != INVALID_HANDLE_VALUE)
      {
        if ( SetWaitableTimer ( pTLS->scheduler.sub_2ms_sleep, &liDelay,
                                  dwMilliseconds, NULL, NULL, 0 ) )
        {
        }

        //SK_WaitForSingleObject (pTLS->scheduler.sub_2ms_sleep, INFINITE);
        SK_WaitForSingleObject_Micro ( &pTLS->scheduler.sub_2ms_sleep,
                                       &uSecs );
      }
      
      else
        NtDelayExecution ((BOOLEAN)bAlertable, &liDelay);
#endif
  }
}

void
WINAPI
Sleep_Detour (DWORD dwMilliseconds)
{
  return
    SleepEx_Detour (dwMilliseconds, FALSE);
}

float __SK_SHENMUE_ClockFuzz = 20.0f;
extern volatile LONG SK_BypassResult;

BOOL
WINAPI
QueryPerformanceFrequency_Detour (_Out_ LARGE_INTEGER *lpPerfFreq)
{
  if (lpPerfFreq)
  {
    *lpPerfFreq =
      SK_GetPerfFreq ();

    return TRUE;
  }

  return
    FALSE;
}

BOOL
WINAPI
SK_QueryPerformanceCounter (_Out_ LARGE_INTEGER *lpPerformanceCount)
{
  if (RtlQueryPerformanceCounter != nullptr)
    return RtlQueryPerformanceCounter (lpPerformanceCount);

  else if (QueryPerformanceCounter_Original != nullptr)
    return QueryPerformanceCounter_Original (lpPerformanceCount);

  else
    return QueryPerformanceCounter (lpPerformanceCount);
}

#include <unordered_set>

extern bool SK_Shenmue_IsLimiterBypassed   (void              );
extern bool SK_Shenmue_InitLimiterOverride (LPVOID pQPCRetAddr);
extern bool SK_Shenmue_UseNtDllQPC;

BOOL
WINAPI
QueryPerformanceCounter_Detour (_Out_ LARGE_INTEGER *lpPerformanceCount)
{
  struct SK_ShenmueLimitBreaker
  {
    bool detected = false;
    bool pacified = false;
  } static
      shenmue_clock {
        SK_GetCurrentGameID () == SK_GAME_ID::Shenmue,
      //SK_GetCurrentGameID () == SK_GAME_ID::DragonQuestXI,
        false
      };


  if ( lpPerformanceCount != nullptr   &&
       shenmue_clock.detected          &&
       shenmue_clock.pacified == false &&
    (! SK_Shenmue_IsLimiterBypassed () ) )
  {
    extern volatile LONG
      __SK_SHENMUE_FinishedButNotPresented;

    static DWORD dwRenderThread = 0;

    if (ReadAcquire (&__SK_SHENMUE_FinishedButNotPresented))
    {
      if (dwRenderThread == 0)
          dwRenderThread = ReadAcquire (&SK_GetCurrentRenderBackend ().thread);

      if ( GetCurrentThreadId () == dwRenderThread )
      {
        if ( SK_GetCallingDLL () == SK_Modules.HostApp () )
        {
          static std::unordered_set <LPCVOID> ret_addrs;

          if (ret_addrs.emplace (_ReturnAddress ()).second)
          {
            SK_LOG1 ( ( L"New QueryPerformanceCounter Return Addr: %ph -- %s",
                        _ReturnAddress (), SK_SummarizeCaller ().c_str () ),
                        L"ShenmueDbg" );

            // The instructions we're looking for are jl ...
            //
            //   Look-ahead up to about 12-bytes and if not found,
            //     this isn't the primary limiter code.
            //
            if ( reinterpret_cast <uintptr_t> (
                   SK_ScanAlignedEx (
                     "\x7C\xEE",          2,
                     nullptr,     (void *)_ReturnAddress ()
                   )
                 ) < (uintptr_t)_ReturnAddress () + 12 )
            {
              shenmue_clock.pacified =
                SK_Shenmue_InitLimiterOverride (_ReturnAddress ());

              SK_LOG0 ( (L"Shenmue Framerate Limiter Located and Pacified"),
                         L"ShenmueDbg" );
            }
          }

          InterlockedExchange (&__SK_SHENMUE_FinishedButNotPresented, 0);

          BOOL bRet =
            RtlQueryPerformanceCounter ?
            RtlQueryPerformanceCounter          (lpPerformanceCount) :
               QueryPerformanceCounter_Original ?
               QueryPerformanceCounter_Original (lpPerformanceCount) :
               QueryPerformanceCounter          (lpPerformanceCount);

          static LARGE_INTEGER last_poll {
            lpPerformanceCount->u.LowPart,
            lpPerformanceCount->u.HighPart
          };

          LARGE_INTEGER pre_fuzz {
            lpPerformanceCount->u.LowPart,
            lpPerformanceCount->u.HighPart
          };

          lpPerformanceCount->QuadPart +=
            static_cast <LONGLONG> (
                                     ( static_cast <long double> (lpPerformanceCount->QuadPart) -
                                       static_cast <long double> (last_poll.QuadPart)           *
                                       static_cast <long double> (__SK_SHENMUE_ClockFuzz)       )
                                   );

          last_poll.QuadPart =
           pre_fuzz.QuadPart;

          return bRet;
        }
      }
    }
  }

  return
    RtlQueryPerformanceCounter          ?
    RtlQueryPerformanceCounter          (lpPerformanceCount) :
       QueryPerformanceCounter_Original ?
       QueryPerformanceCounter_Original (lpPerformanceCount) :
       QueryPerformanceCounter          (lpPerformanceCount);
}

float target_fps = 0.0;

static volatile LONG frames_ahead = 0;

void
SK::Framerate::Init (void)
{
  static SK::Framerate::Stats        _frame_history;
  static SK::Framerate::Stats        _frame_history2;
  static SK::Framerate::EventCounter _events;

  static bool basic_init = false;

  if (! basic_init)
  {
    basic_init = true;

    frame_history         = &_frame_history;
    frame_history2        = &_frame_history2;
    SK::Framerate::events = &_events;

    SK_ICommandProcessor* pCommandProc =
      SK_GetCommandProcessor ();

    // TEMP HACK BECAUSE THIS ISN'T STORED in D3D9.INI
    if (GetModuleHandle (L"AgDrag.dll"))
      config.render.framerate.max_delta_time = 5;

    if (GetModuleHandle (L"tsfix.dll"))
      config.render.framerate.max_delta_time = 0;

    pCommandProc->AddVariable ( "WaitForVBLANK",
            new SK_IVarStub <bool> (&config.render.framerate.wait_for_vblank));
    pCommandProc->AddVariable ( "MaxDeltaTime",
            new SK_IVarStub <int> (&config.render.framerate.max_delta_time));

    pCommandProc->AddVariable ( "LimiterTolerance",
            new SK_IVarStub <float> (&config.render.framerate.limiter_tolerance));
    pCommandProc->AddVariable ( "TargetFPS",
            new SK_IVarStub <float> (&target_fps));

    pCommandProc->AddVariable ( "MaxRenderAhead",
            new SK_IVarStub <int> (&config.render.framerate.max_render_ahead));

    RtlQueryPerformanceFrequency = 
      (QueryPerformanceCounter_pfn)
      SK_GetProcAddress ( L"NtDll",
                           "RtlQueryPerformanceFrequency" );

    RtlQueryPerformanceCounter = 
    (QueryPerformanceCounter_pfn)
      SK_GetProcAddress ( L"NtDll",
                           "RtlQueryPerformanceCounter" );

    ZwQueryPerformanceCounter = 
      (QueryPerformanceCounter_pfn)
      SK_GetProcAddress ( L"NtDll",
                           "ZwQueryPerformanceCounter" );

    NtDelayExecution =
      (NtDelayExecution_pfn)
      SK_GetProcAddress ( L"NtDll",
                           "NtDelayExecution" );

//#define NO_HOOK_QPC
#ifndef NO_HOOK_QPC
  SK_GetPerfFreq ();
  SK_CreateDLLHook2 (      L"kernel32",
                            "QueryPerformanceFrequency",
                             QueryPerformanceFrequency_Detour,
    static_cast_p2p <void> (&QueryPerformanceFrequency_Original) );

  SK_CreateDLLHook2 (      L"kernel32",
                            "QueryPerformanceCounter",
                             QueryPerformanceCounter_Detour,
    static_cast_p2p <void> (&QueryPerformanceCounter_Original),
    static_cast_p2p <void> (&pfnQueryPerformanceCounter) );
#endif

    SK_CreateDLLHook2 (      L"kernel32",
                              "Sleep",
                               Sleep_Detour,
      static_cast_p2p <void> (&Sleep_Original),
      static_cast_p2p <void> (&pfnSleep) );

    SK_CreateDLLHook2 (      L"KernelBase.dll",
                              "SleepEx",
                               SleepEx_Detour,
      static_cast_p2p <void> (&SleepEx_Original) );

        SK_CreateDLLHook2 (  L"KernelBase.dll",
                              "SwitchToThread",
                               SwitchToThread_Detour,
      static_cast_p2p <void> (&SwitchToThread_Original) );

    SK_CreateDLLHook2 (      L"NtDll",
                              "NtWaitForSingleObject",
                               NtWaitForSingleObject_Detour,
      static_cast_p2p <void> (&NtWaitForSingleObject_Original) );

    SK_CreateDLLHook2 (      L"NtDll",
                              "NtWaitForMultipleObjects",
                               NtWaitForMultipleObjects_Detour,
      static_cast_p2p <void> (&NtWaitForMultipleObjects_Original) );

#ifdef NO_HOOK_QPC
      QueryPerformanceCounter_Original =
        reinterpret_cast <QueryPerformanceCounter_pfn> (
          GetProcAddress ( SK_GetModuleHandle (L"kernel32"),
                             "QueryPerformanceCounter" )
        );
#endif

    if (! config.render.framerate.enable_mmcss)
    {
      if (NtDll == nullptr)
      {
        NtDll =
          LoadLibrary (L"ntdll.dll");

        NtQueryTimerResolution =
          reinterpret_cast <NtQueryTimerResolution_pfn> (
            GetProcAddress (NtDll, "NtQueryTimerResolution")
          );

        NtSetTimerResolution =
          reinterpret_cast <NtSetTimerResolution_pfn> (
            GetProcAddress (NtDll, "NtSetTimerResolution")
          );

        if (NtQueryTimerResolution != nullptr &&
            NtSetTimerResolution   != nullptr)
        {
          ULONG min, max, cur;
          NtQueryTimerResolution (&min, &max, &cur);
          dll_log.Log ( L"[  Timing  ] Kernel resolution.: %f ms",
                          static_cast <float> (cur * 100)/1000000.0f );
          NtSetTimerResolution   (max, TRUE,  &cur);
          dll_log.Log ( L"[  Timing  ] New resolution....: %f ms",
                          static_cast <float> (cur * 100)/1000000.0f );
        }
      }
    }
  }
}

void
SK::Framerate::Shutdown (void)
{
  if (NtDll != nullptr)
    FreeLibrary (NtDll);

  SK_DisableHook (pfnSleep);
  //SK_DisableHook (pfnQueryPerformanceCounter);
}

SK::Framerate::Limiter::Limiter (double target)
{
  effective_ms = 0.0;

  init (target);
}


IDirect3DDevice9Ex*
SK_D3D9_GetTimingDevice (void)
{
  static auto* pTimingDevice =
    reinterpret_cast <IDirect3DDevice9Ex *> (-1);

  if (pTimingDevice == reinterpret_cast <IDirect3DDevice9Ex *> (-1))
  {
    CComPtr <IDirect3D9Ex> pD3D9Ex = nullptr;

    using Direct3DCreate9ExPROC = HRESULT (STDMETHODCALLTYPE *)(UINT           SDKVersion,
                                                                IDirect3D9Ex** d3d9ex);

    extern Direct3DCreate9ExPROC Direct3DCreate9Ex_Import;

    // For OpenGL, bootstrap D3D9
    SK_BootD3D9 ();

    HRESULT hr = (config.apis.d3d9ex.hook) ?
      Direct3DCreate9Ex_Import (D3D_SDK_VERSION, &pD3D9Ex)
                                    :
                               E_NOINTERFACE;

    HWND hwnd = nullptr;

    IDirect3DDevice9Ex* pDev9Ex = nullptr;

    if (SUCCEEDED (hr))
    {
      hwnd =
        SK_Win32_CreateDummyWindow ();

      D3DPRESENT_PARAMETERS pparams = { };

      pparams.SwapEffect       = D3DSWAPEFFECT_FLIPEX;
      pparams.BackBufferFormat = D3DFMT_UNKNOWN;
      pparams.hDeviceWindow    = hwnd;
      pparams.Windowed         = TRUE;
      pparams.BackBufferCount  = 2;
      pparams.BackBufferHeight = 2;
      pparams.BackBufferWidth  = 2;

      if ( FAILED ( pD3D9Ex->CreateDeviceEx (
                      D3DADAPTER_DEFAULT,
                        D3DDEVTYPE_HAL,
                          hwnd,
                            D3DCREATE_HARDWARE_VERTEXPROCESSING,
                              &pparams,
                                nullptr,
                                  &pDev9Ex )
                  )
          )
      {
        pTimingDevice = nullptr;
      } else {
        pDev9Ex->AddRef ();
        pTimingDevice = pDev9Ex;
      }
    }
    else {
      pTimingDevice = nullptr;
    }
  }

  return pTimingDevice;
}


void
SK::Framerate::Limiter::init (double target)
{
  QueryPerformanceFrequency (&freq);

  ms  = 1000.0 / target;
  fps = target;

  frames = 0;

  CComPtr <IDirect3DDevice9Ex> d3d9ex      = nullptr;
  CComPtr <IDXGISwapChain>     dxgi_swap   = nullptr;
  CComPtr <IDXGIOutput>        dxgi_output = nullptr;

  static auto& rb =
   SK_GetCurrentRenderBackend ();

  SK_RenderAPI api = rb.api;

  if (                    api ==                    SK_RenderAPI::D3D10  ||
       static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D11) ||
       static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D12) )
  {
    if (rb.swapchain != nullptr)
    {
      HRESULT hr =
        rb.swapchain->QueryInterface <IDXGISwapChain> (&dxgi_swap);

      if (SUCCEEDED (hr))
      {
        if (SUCCEEDED (dxgi_swap->GetContainingOutput (&dxgi_output)))
        {
          //WaitForVBlank_Original (dxgi_output);
          dxgi_output->WaitForVBlank ();
        }
      }
    }
  }

  else if (static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D9))
  {
    if (rb.device != nullptr)
    {
      rb.device->QueryInterface ( IID_PPV_ARGS (&d3d9ex) );

      // Align the start to VBlank for minimum input latency
      if (d3d9ex != nullptr || (d3d9ex = SK_D3D9_GetTimingDevice ()))
      {
        UINT orig_latency = 3;
        d3d9ex->GetMaximumFrameLatency (&orig_latency);

        d3d9ex->SetMaximumFrameLatency (1);
        d3d9ex->WaitForVBlank          (0);
        d3d9ex->SetMaximumFrameLatency (
          config.render.framerate.pre_render_limit == -1 ?
               orig_latency : config.render.framerate.pre_render_limit );
      }
    }
  }

  SK_QueryPerformanceCounter (&start);

  InterlockedExchange (&frames_ahead, 0);

  time.QuadPart = 0ULL;
  last.QuadPart = static_cast <LONGLONG> (start.QuadPart - (ms / 1000.0) * freq.QuadPart);
  next.QuadPart = static_cast <LONGLONG> (start.QuadPart + (ms / 1000.0) * freq.QuadPart);
}

#include <SpecialK/window.h>

bool
SK::Framerate::Limiter::try_wait (void)
{
  if (limit_behavior != LIMIT_APPLY)
    return false;

  if (target_fps <= 0.0f)
    return false;

  LARGE_INTEGER next_;
  next_.QuadPart =
    static_cast <LONGLONG> (
      start.QuadPart                                +
      (  static_cast <long double> (frames) + 1.0 ) *
                                   (ms  / 1000.0L ) *
         static_cast <long double>  (freq.QuadPart)
    );

  SK_QueryPerformanceCounter (&time);

  if (time.QuadPart < next_.QuadPart)
    return true;

  return false;
}

void
SK::Framerate::Limiter::wait (void)
{
  //SK_Win32_AssistStalledMessagePump (100);

  if (limit_behavior != LIMIT_APPLY)
    return;

  // Don't limit under certain circumstances or exiting / alt+tabbing takes
  //   longer than it should.
  if (ReadAcquire (&__SK_DLL_Ending))
    return;

  //SK_RunOnce ( SetThreadPriority ( SK_GetCurrentThread (),
  //                                   THREAD_PRIORITY_ABOVE_NORMAL ) );

  static bool restart      = false;
  static bool full_restart = false;

  if (fps != target_fps)
    init (target_fps);

  if (target_fps <= 0.0f)
    return;

  frames++;

  SK_QueryPerformanceCounter (&time);


  //bool bNeedWait =
  //  time.QuadPart < next.QuadPart;

  ///if (bNeedWait && ReadAcquire (&frames_ahead) < ( config.render.framerate.max_render_ahead + 1 ))
  ///{
  ///  InterlockedIncrement (&frames_ahead);
  ///  return;
  ///}
  ///
  ///else if (bNeedWait && InterlockedCompareExchange (&frames_ahead, 0, 1) > 1)
  ///                      InterlockedDecrement       (&frames_ahead);


  // Actual frametime before we forced a delay
  effective_ms =
    static_cast <double> (
      1000.0L * ( static_cast <double> (time.QuadPart - last.QuadPart) /
                  static_cast <double> (freq.QuadPart)                 )
    );

  if ( static_cast <double> (time.QuadPart - last.QuadPart) /
       static_cast <double> (freq.QuadPart)                 /
                            ( ms / 1000.0L)                 >
      ( config.render.framerate.limiter_tolerance * fps )
     )
  {
    //dll_log.Log ( L" * Frame ran long (%3.01fx expected) - restarting"
                  //L" limiter...",
            //(double)(time.QuadPart - next.QuadPart) /
            //(double)freq.QuadPart / (ms / 1000.0) / fps );
    restart = true;

#if 0
    extern SK::Framerate::Stats frame_history;
    extern SK::Framerate::Stats frame_history2;

    double mean    = frame_history.calcMean     ();
    double sd      = frame_history.calcSqStdDev (mean);

    if (sd > 5.0f)
      full_restart = true;
#endif
  }

  if (restart || full_restart)
  {
    frames         = 0;
    start.QuadPart = static_cast <LONGLONG> (
                       static_cast <double> (time.QuadPart) +
                                            ( ms / 1000.0L) *
                       static_cast <double> (freq.QuadPart)
                     );
    restart        = false;

    time.QuadPart = 0ULL;
    start.QuadPart = static_cast <LONGLONG> (start.QuadPart - (ms / 1000.0) * freq.QuadPart);
     next.QuadPart = static_cast <LONGLONG> (start.QuadPart + (ms / 1000.0) * freq.QuadPart);

    if (full_restart)
    {
      init (target_fps);
      full_restart = false;
    }
    //return;
  }

  next.QuadPart =
    static_cast <LONGLONG> (
      long_double_cast (start.QuadPart) +
      long_double_cast (    frames    ) *
      long_double_cast ( ms / 1000.0L ) *
      long_double_cast ( freq.QuadPart)
    );


  auto
    SK_RecalcTimeToNextFrame =
    [&](void)->
      long double
      {
        long double ldRet =
          ( long_double_cast ( next.QuadPart -
                               SK_QueryPerf ().QuadPart ) /
            long_double_cast ( freq.QuadPart            ) );

        if (ldRet < 0.0L)
          ldRet = 0.0L;

        return ldRet;
      };

  long double
    to_next_in_secs =
      SK_RecalcTimeToNextFrame ();

  LARGE_INTEGER liDelay;
                liDelay.QuadPart =
                  static_cast <LONGLONG> (
                    to_next_in_secs * 1000.0l * 0.9875l
                  );

  //dll_log.Log (L"Wait MS: %f", to_next_in_secs * 1000.0 );

  // Create an unnamed waitable timer.
  static HANDLE hLimitTimer =
    CreateWaitableTimer (NULL, FALSE, NULL);

  if ( liDelay.QuadPart > 0)
  {
    liDelay.QuadPart = -(liDelay.QuadPart * 10000);

    if ( SetWaitableTimer ( hLimitTimer, &liDelay,
                              0, NULL, NULL, 0 ) )
    {
      DWORD dwWait = 0;

      while (dwWait != WAIT_OBJECT_0)
      {
        to_next_in_secs =
          SK_RecalcTimeToNextFrame ();

        if (to_next_in_secs <= 0.0l)
          break;

        LARGE_INTEGER uSecs;
                      uSecs.QuadPart =
          static_cast <LONGLONG> (to_next_in_secs * 1000.0l * 1000.0l);

        dwWait =
          SK_WaitForSingleObject_Micro ( &hLimitTimer,
                                           &uSecs );
      }
    }
  }


  if (next.QuadPart > 0ULL)
  {
    // If available (Windows 7+), wait on the swapchain
    CComPtr <IDirect3DDevice9Ex>  d3d9ex = nullptr;

    // D3D10/11/12
    CComPtr <IDXGISwapChain> dxgi_swap   = nullptr;
    CComPtr <IDXGIOutput>    dxgi_output = nullptr;

    static SK_RenderBackend& rb =
      SK_GetCurrentRenderBackend ();

    if (config.render.framerate.wait_for_vblank)
    {
      SK_RenderAPI api = rb.api;

      if (                    api ==                    SK_RenderAPI::D3D10  ||
           static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D11) ||
           static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D12) )
      {
        if (rb.swapchain != nullptr)
        {
          HRESULT hr =
            rb.swapchain->QueryInterface <IDXGISwapChain> (&dxgi_swap);

          if (SUCCEEDED (hr))
          {
            dxgi_swap->GetContainingOutput (&dxgi_output);
          }
        }
      }

      else if ( static_cast <int> (api)       &
                static_cast <int> (SK_RenderAPI::D3D9) )
      {
        if (rb.device != nullptr)
        {
          if (FAILED (rb.device->QueryInterface <IDirect3DDevice9Ex> (&d3d9ex)))
          {
            d3d9ex =
              SK_D3D9_GetTimingDevice ();
          }
        }
      }
    }

    bool bYielded = false;

    while (time.QuadPart < next.QuadPart)
    {
      // Attempt to use a deeper sleep when possible instead of hammering the
      //   CPU into submission ;)
      if ( ( static_cast <double> (next.QuadPart  - time.QuadPart) >
             static_cast <double> (freq.QuadPart) * 0.001 *
                                   config.render.framerate.busy_wait_limiter) &&
                                  (! (config.render.framerate.yield_once && bYielded))
         )
      {
        if ( config.render.framerate.wait_for_vblank )
        {
          if (d3d9ex != nullptr)
            d3d9ex->WaitForVBlank (0);

          else if (dxgi_output != nullptr)
            dxgi_output->WaitForVBlank ();
        }

        else if (! config.render.framerate.busy_wait_limiter)
        {
          auto dwWaitMS =
            static_cast <DWORD>
              ( (config.render.framerate.max_sleep_percent * 10.0f) / target_fps ); // 10% of full frame

          if ( ( static_cast <long double> (next.QuadPart - time.QuadPart) /
                 static_cast <long double> (freq.QuadPart                ) ) * 1000.0 >
                   dwWaitMS )
          {
            SK_Sleep (dwWaitMS);

            bYielded = true;
          }
        }
      }

      SK_QueryPerformanceCounter (&time);
    }
  }

  else
  {
    dll_log.Log (L"[FrameLimit] Framerate limiter lost time?! (non-monotonic clock)");
    start.QuadPart += -next.QuadPart;
  }

  last.QuadPart = time.QuadPart;
}

void
SK::Framerate::Limiter::set_limit (double target)
{
  init (target);
}

double
SK::Framerate::Limiter::effective_frametime (void)
{
  return effective_ms;
}


SK::Framerate::Limiter*
SK::Framerate::GetLimiter (void)
{
  static          Limiter *limiter = nullptr;
  static volatile LONG     init    = 0;

  if (! InterlockedCompareExchangeAcquire (&init, 1, 0))
  {
    limiter =
      new Limiter (config.render.framerate.target_fps);

    SK_ReleaseAssert (limiter != nullptr)

    if (limiter != nullptr)
      InterlockedIncrementRelease (&init);
    else
      InterlockedDecrementRelease (&init);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&init, 2);

  return limiter;
}

void
SK::Framerate::Tick (double& dt, LARGE_INTEGER& now)
{
  if ( frame_history  == nullptr ||
       frame_history2 == nullptr )
  {
    // Late initialization
    Init ();
  }

  static LARGE_INTEGER last_frame = { };

  now = SK_CurrentPerf ();

  dt = static_cast <double> (
    static_cast <long double> (now.QuadPart -  last_frame.QuadPart) /
    static_cast <long double> (SK::Framerate::Stats::freq.QuadPart)
  );


  // What the bloody hell?! How do we ever get a dt value near 0?
  if (dt > 0.000001)
    frame_history->addSample (1000.0 * dt, now);
  else // Less than single-precision FP epsilon, toss this frame out
    frame_history->addSample (INFINITY, now);



  frame_history2->addSample (
    SK::Framerate::GetLimiter ()->effective_frametime (),
      now
  );


  last_frame = now;
};


double
SK::Framerate::Stats::calcMean (double seconds)
{
  return
    calcMean (SK_DeltaPerf (seconds, freq.QuadPart));
}

double
SK::Framerate::Stats::calcSqStdDev (double mean, double seconds)
{
  return
    calcSqStdDev (mean, SK_DeltaPerf (seconds, freq.QuadPart));
}

double
SK::Framerate::Stats::calcMin (double seconds)
{
  return
    calcMin (SK_DeltaPerf (seconds, freq.QuadPart));
}

double
SK::Framerate::Stats::calcMax (double seconds)
{
  return
    calcMax (SK_DeltaPerf (seconds, freq.QuadPart));
}

int
SK::Framerate::Stats::calcHitches (double tolerance, double mean, double seconds)
{
  return
    calcHitches (tolerance, mean, SK_DeltaPerf (seconds, freq.QuadPart));
}

int
SK::Framerate::Stats::calcNumSamples (double seconds)
{
  return
    calcNumSamples (SK_DeltaPerf (seconds, freq.QuadPart));
}


LARGE_INTEGER
SK_GetPerfFreq (void)
{
  static LARGE_INTEGER freq = { 0UL };
  static volatile LONG init = FALSE;
  
  if (ReadAcquire (&init) < 2)
  {
      RtlQueryPerformanceFrequency = 
        (QueryPerformanceCounter_pfn)
    SK_GetProcAddress ( L"NtDll",
                         "RtlQueryPerformanceFrequency" );

    if (! InterlockedCompareExchange (&init, 1, 0))
    {
      //if (QueryPerformanceFrequency_Original != nullptr)
      //    QueryPerformanceFrequency_Original (&freq);
      //else
        RtlQueryPerformanceFrequency (&freq);
        //QueryPerformanceFrequency (&freq);

      InterlockedIncrement (&init);

      return freq;
    }

    if (ReadAcquire (&init) < 2)
    {
      LARGE_INTEGER freq2 = { };

      RtlQueryPerformanceFrequency (&freq2);
      //if (QueryPerformanceFrequency_Original != nullptr)
      //    QueryPerformanceFrequency_Original (&freq2);

      //else
      //  QueryPerformanceFrequency (&freq2);

      return
        freq2;
    }
  }

  return freq;
}