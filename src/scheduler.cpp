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

#include <SpecialK/stdafx.h>

#ifndef __SK_SUBSYSTEM__
#define __SK_SUBSYSTEM__ L"Scheduling"
#endif

#define ALLOW_UNDOCUMENTED


using NTSTATUS = _Return_type_success_(return >= 0) LONG;


NtQueryTimerResolution_pfn NtQueryTimerResolution        = nullptr;
NtSetTimerResolution_pfn   NtSetTimerResolution          = nullptr;
NtSetTimerResolution_pfn   NtSetTimerResolution_Original = nullptr;

NtDelayExecution_pfn       NtDelayExecution              = nullptr;


LPVOID pfnQueryPerformanceCounter = nullptr;
LPVOID pfnSleep                   = nullptr;

double                      dTicksPerMS                        =     0.0;
Sleep_pfn                   Sleep_Original                     = nullptr;
SleepEx_pfn                 SleepEx_Original                   = nullptr;
QueryPerformanceCounter_pfn QueryPerformanceCounter_Original   = nullptr;
QueryPerformanceCounter_pfn ZwQueryPerformanceCounter          = nullptr;
QueryPerformanceCounter_pfn RtlQueryPerformanceCounter         = nullptr;
QueryPerformanceCounter_pfn QueryPerformanceFrequency_Original = nullptr;
QueryPerformanceCounter_pfn RtlQueryPerformanceFrequency       = nullptr;

typedef BOOL (WINAPI *SwitchToThread_pfn)(void);
                      SwitchToThread_pfn
                      SwitchToThread_Original = nullptr;

extern HWND WINAPI SK_GetActiveWindow (           SK_TLS *pTLS);
extern BOOL WINAPI SK_IsWindowUnicode (HWND hWnd, SK_TLS *pTLS);

DWORD
WINAPI
SK_DelayExecution (double dMilliseconds, BOOL bAlertable) noexcept;

void
SK_MMCS_ApplyPendingTaskPriority (SK_TLS **ppTLS = nullptr)
{
  if (ReadAcquire (&__SK_MMCS_PendingChanges) > 0)
  {
    // Stash and Cache
    //
    SK_TLS*
      pTLS =
     ppTLS != nullptr ?
    *ppTLS :  nullptr;

    if (pTLS == nullptr)
        pTLS =
      SK_TLS_Bottom ();

    if (ppTLS != nullptr)
       *ppTLS =
         pTLS;

    if (! pTLS) return;

    if (pTLS->scheduler->mmcs_task > (SK_MMCS_TaskEntry *)1)
    {
      auto task =
          pTLS->scheduler->mmcs_task;

      if (InterlockedCompareExchange (&task->change.pending, 0, 1))
      {
        // In an active task?
        if ((intptr_t)pTLS->scheduler->mmcs_task->hTask > 0)
          task->setPriority (task->change.priority);
        else // No, we'll apply it the next time we re-associate the task
          task->priority  =  task->change.priority;

        InterlockedDecrement (&__SK_MMCS_PendingChanges);
      }
    }
  }
}

void
SK_Thread_WaitWhilePumpingMessages (DWORD dwMilliseconds, BOOL bAlertable, SK_TLS **ppTLS)
{
  UNREFERENCED_PARAMETER (ppTLS);

  if (dwMilliseconds == INFINITE)
  {
    SK_LOG0 ( ( L"Infinite Sleep On Window Thread (!!) - SK will not force thread awake..." ),
                L"Win32-Pump" );

    SK_SleepEx (
      INFINITE,
        bAlertable
    );

    return;
  }

//#define SK_DISPATCH_SLEEP
#ifdef  SK_DISPATCH_SLEEP
  auto PeekAndDispatch = [&]
  {
    constexpr HWND hWndThis =    0;
    constexpr bool bUnicode = true;

    // Avoid having Windows marshal Unicode messages like a dumb ass
              MSG        msg (                           { }                 );
    constexpr auto      Peek (bUnicode ? PeekMessageW     :      PeekMessageA);
    constexpr auto  Dispatch (bUnicode ? DispatchMessageW :  DispatchMessageA);
    constexpr auto Translate (bUnicode ? TranslateMessage : TranslateMessage );
    constexpr auto   wszDesc (bUnicode ? L"Unicode"       :           L"ANSI");

    if (config.system.log_level > 0)
    {
      DWORD dwQueueStatus =
        GetQueueStatus (QS_ALLINPUT);

      DWORD dwTypes =
        HIWORD (dwQueueStatus);

      DWORD dwTypesAdded =
        LOWORD (dwQueueStatus);

      std::wstring types = L"";

      if (dwTypes      & QS_HOTKEY)      types += L"          [Hotkey] ";
      if (dwTypes      & QS_MOUSE)       types += L"    [Legacy Mouse] ";
      if (dwTypes      & QS_KEY)         types += L" [Legacy Keyboard] ";
      if (dwTypes      & QS_RAWINPUT)    types += L"       [Raw Input] ";
      if (dwTypes      & QS_TIMER)       types += L"           [Timer] ";
      if (dwTypes      & QS_POSTMESSAGE) types += L"    [Post Message] ";
      if (dwTypes      & QS_SENDMESSAGE) types += L"    [Send Message] ";

      if (dwTypesAdded & QS_HOTKEY)      types += L"          [Hotkey]+";
      if (dwTypesAdded & QS_MOUSE)       types += L"    [Legacy Mouse]+";
      if (dwTypesAdded & QS_KEY)         types += L" [Legacy Keyboard]+";
      if (dwTypesAdded & QS_RAWINPUT)    types += L"       [Raw Input]+";
      if (dwTypesAdded & QS_TIMER)       types += L"           [Timer]+";
      if (dwTypesAdded & QS_POSTMESSAGE) types += L"    [Post Message]+";
      if (dwTypesAdded & QS_SENDMESSAGE) types += L"    [Send Message]+";

      if (! types.empty ())
        dll_log->Log (L"HWND %x, %ws", msg.hwnd, types.c_str ());
    }

    while ( Peek (&msg, hWndThis, 0U, 0U, PM_REMOVE | PM_NOYIELD) )
    {  Translate (&msg);
        Dispatch (&msg);

      SK_LOG1 ( ( L"Dispatched Message: %x to %ws HWND: %x while "
                  L"framerate limiting!",
                   msg.message, wszDesc,
                   msg.hwnd ),                       L"Win32-Pump" );
    }
  };
#endif

#ifdef _M_AMD64
  static bool bNierSqrt1_5 =
    SK_GetCurrentGameID () == SK_GAME_ID::NieR_Sqrt_1_5;

  if (bNierSqrt1_5)
  {
    static DWORD InputThread = 0;

    static volatile LONG                                          s_Once = FALSE;
    if ((! InputThread) && (FALSE == InterlockedCompareExchange (&s_Once, TRUE, FALSE)))
    {
      SK_TLS*
        pTLS =
       ppTLS != nullptr ?
      *ppTLS :  nullptr;

      if (pTLS == nullptr)
          pTLS =
        SK_TLS_Bottom ();

      if (ppTLS != nullptr)
         *ppTLS =
           pTLS;


      if (*pTLS->debug.name == L'I' && (0 == _wcsicmp (pTLS->debug.name, L"InputThread")))
      {
        auto thread =
          SK_GetCurrentThread ();

        SetThreadPriority      (thread, THREAD_PRIORITY_HIGHEST);
        SetThreadPriorityBoost (thread, FALSE);

        SK_MMCS_TaskEntry* task_me =
          (config.render.framerate.enable_mmcss ?
           SK_MMCS_GetTaskForThreadID (SK_GetCurrentThreadId (),
                                       "InputThread") : nullptr);

        if (task_me != nullptr)
        {
          task_me->setPriority (AVRT_PRIORITY_NORMAL);
        }

        InputThread = SK_GetCurrentThreadId ();
      }

      else
        InterlockedExchange (&s_Once, FALSE);
    }

    if (SK_GetCurrentThreadId () == InputThread)
    {
      extern volatile LONG _SK_NIER_RAD_InputPollingPeriod;

      if (dwMilliseconds == 8)
        dwMilliseconds = ReadAcquire (&_SK_NIER_RAD_InputPollingPeriod);

      SK_SleepEx (dwMilliseconds, bAlertable);
      return;
    }
  }
#endif

  // Not good
  if (dwMilliseconds == 0)
  {
    static DWORD dwLastTid = 0;

    if (SK_GetCurrentThreadId () != dwLastTid)
    {
        // Stash and Cache
        //
        SK_TLS*
          pTLS =
         ppTLS != nullptr ?
        *ppTLS :  nullptr;

        if (pTLS == nullptr)
            pTLS =
          SK_TLS_Bottom ();

        if (ppTLS != nullptr)
           *ppTLS =
             pTLS;


      if (! pTLS->win32->mmcs_task)
      {     pTLS->win32->mmcs_task = true;
        auto thread =
          SK_GetCurrentThread ();

        SetThreadPriority  (thread,
         GetThreadPriority (thread) + 1);

        SK_MMCS_TaskEntry* task_me =
          ( config.render.framerate.enable_mmcss ?
            SK_MMCS_GetTaskForThreadID ( SK_GetCurrentThreadId (),
                      "Dubious Sleeper (0 ms)" ) : nullptr );

        if (task_me != nullptr)
        {
          task_me->setPriority (AVRT_PRIORITY_NORMAL);
        }
      }

      if (pTLS->win32->mmcs_task)
        dwLastTid = SK_GetCurrentThreadId ();
    }
  }



  auto now =
         SK_CurrentPerf ().QuadPart;
  auto end =
       now + static_cast <LONGLONG> (
             static_cast < double > (                         dTicksPerMS) *
                                    (static_cast <double> (dwMilliseconds)));

  do
  {
    DWORD dwMaxWait =
      sk::narrow_cast <DWORD> (
        std::max ( 0.0,
          static_cast <double> ( end - now ) / dTicksPerMS )
                          );

    //GUITHREADINFO gti        = {                    };
    //              gti.cbSize = sizeof (GUITHREADINFO);

    BOOL bGUIThread =
      SK_Thread_GetTEB_FAST ()->Win32ThreadInfo != nullptr;

    //BOOL bGUIThread    =
    //  GetGUIThreadInfo (GetCurrentThreadId (), &gti) && IsWindow (gti.hwndActive);

    if (dwMaxWait < 5000UL)
    {
      if (! bGUIThread)
        return;

      if (dwMaxWait <= (DWORD)config.render.framerate.max_delta_time)
        return;

      DWORD dwWaitState =
        MsgWaitForMultipleObjectsEx (
          1, &__SK_DLL_TeardownEvent,
               dwMaxWait, QS_ALLINPUT,
                           MWMO_INPUTAVAILABLE |
          (   bAlertable ? MWMO_ALERTABLE
                         : 0x0 )    );

      // APC popping up in strange places?
      if (     dwWaitState == WAIT_IO_COMPLETION)
      {  ////SK_ReleaseAssert (  "WAIT_IO_COMPLETION"
         ////                         && bAlertable );
        return;
      }

      // Waiting messages
      else if (dwWaitState == WAIT_OBJECT_0 + 1)
      {
#ifdef SK_DISPATCH_SLEEP
        PeekAndDispatch ();
        // ...
#endif
        return;
      }

      // DLL Shutdown
      else if (dwWaitState == WAIT_OBJECT_0)
      {
        return;
      }

      // Embarassing
      else if (dwWaitState == WAIT_TIMEOUT)
      {
        return;
      }

      // ???
      else
      {
        SK_ReleaseAssert (!
                          L"Unexpected Wait State in call to "
                          L"MsgWaitForMultipleObjectsEx (...)");

        return;
      }

      if (dwMilliseconds == 0)
        return;
    }

    else
    {
      SK_SleepEx (dwMaxWait, bAlertable);
      return;
    }

    now =
      SK_CurrentPerf ().QuadPart;
  }
  while (now < end);
}

float
SK_Sched_ThreadContext::most_recent_wait_s::getRate (void)
{
  if (sequence > 0)
  {
    double ms =
      SK_DeltaPerfMS (
        SK_CurrentPerf ().QuadPart - (last_wait.QuadPart - start.QuadPart), 1
      );

    return
      static_cast <float> ( ms / static_cast <double> (sequence) );
  }

  // Sequence just started
  return -1.0f;
}


NtWaitForSingleObject_pfn
NtWaitForSingleObject_Original    = nullptr;

NtWaitForMultipleObjects_pfn
NtWaitForMultipleObjects_Original = nullptr;

DWORD
WINAPI
SK_WaitForSingleObject_Micro ( _In_  HANDLE          hHandle,
                               _In_ PLARGE_INTEGER pliMicroseconds )
{
#ifdef ALLOW_UNDOCUMENTED
  if (NtWaitForSingleObject_Original != nullptr)
  {
    NTSTATUS NtStatus =
      NtWaitForSingleObject_Original (
        hHandle,
          TRUE,
            pliMicroseconds
      );

    if (NT_SUCCESS (NtStatus))
    {
      switch (NtStatus)
      {
        case STATUS_SUCCESS:
          return WAIT_OBJECT_0;
        case STATUS_TIMEOUT:
          return WAIT_TIMEOUT;
        case STATUS_ALERTED:
        case STATUS_USER_APC:
          return WAIT_IO_COMPLETION;
      }
    }

    dll_log->Log (L"Unexpected SK_WaitForSingleObject_Micro Status: '%x'", (DWORD)NtStatus);

    // ???
    return
      WAIT_FAILED;
  }
#endif

  DWORD dwMilliseconds =
    ( pliMicroseconds == nullptr ?
              INFINITE : static_cast <DWORD> (
                           -pliMicroseconds->QuadPart / 10000LL
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
#if 1
  return
    WaitForSingleObject (hHandle, dwMilliseconds);
#else
  if (dwMilliseconds == INFINITE)
  {
    return
      SK_WaitForSingleObject_Micro ( hHandle, nullptr );
  }

  LARGE_INTEGER usecs;
                usecs.QuadPart =
                  -static_cast <LONGLONG> (
                    dwMilliseconds
                  ) * 10000LL;

  return
    SK_WaitForSingleObject_Micro ( hHandle, &usecs );
#endif
}

NTSTATUS
NTAPI
NtWaitForMultipleObjects_Detour (
  IN ULONG                ObjectCount,
  IN PHANDLE              ObjectsArray,
  IN OBJECT_WAIT_TYPE     WaitType,
  IN BOOLEAN              Alertable,
  IN PLARGE_INTEGER       TimeOut OPTIONAL )
{
  NTSTATUS ntStatus =
    NtWaitForMultipleObjects_Original (
         ObjectCount, ObjectsArray,
           WaitType, Alertable,
             TimeOut                  );

  if (ntStatus != STATUS_TIMEOUT)
    SK_MMCS_ApplyPendingTaskPriority ();

  return
    ntStatus;
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

#if 0
DWORD
WINAPI
SK_WaitForSingleObject ( HANDLE hWaitObj,
                         DWORD  dwTimeout )
{
  if (   NtWaitForSingleObject_Original == nullptr)
    return WaitForSingleObject (hWaitObj, dwTimeout);

  LARGE_INTEGER
    liTimeout;
    liTimeout.QuadPart =
  static_cast <LONGLONG> (dwTimeout) * 1000LL;

  NTSTATUS ntStatus =
    NtWaitForSingleObject_Original ( hWaitObj, FALSE, &liTimeout );

  switch (ntStatus)
  {
    case STATUS_ACCESS_DENIED:
      SetLastError (ERROR_ACCESS_DENIED);
      return WAIT_FAILED;

    // Alerted states should not happen in WaitForSingleObject
    case STATUS_ALERTED:
    case STATUS_USER_APC:
      return WAIT_FAILED;

    case STATUS_SUCCESS:
      return WAIT_OBJECT_0;

    case STATUS_TIMEOUT:
      return WAIT_TIMEOUT;

    case STATUS_INVALID_HANDLE:
      SetLastError (ERROR_INVALID_HANDLE);
      return WAIT_FAILED;

    default:
      return WAIT_FAILED;
  }
}
#endif

NTSTATUS
WINAPI
NtWaitForSingleObject_Detour (
  IN HANDLE         Handle,
  IN BOOLEAN        Alertable,
  IN PLARGE_INTEGER Timeout  )
{
#pragma region UnityHack
#ifdef _UNITY_HACK
  if (bAlertable)
    InterlockedIncrement (&pTLS->scheduler->alert_waits);

  // Consider double-buffering this since the information
  //   is used almost exclusively by OHTER threads, and
  //     we have to do a synchronous copy the get at this
  //       thing without thread A murdering thread B.
  SK_Sched_ThreadContext::wait_record_s& scheduled_wait =
    (*pTLS->scheduler->objects_waited) [hHandle];

  scheduled_wait.calls++;

  if (dwMilliseconds == INFINITE)
    scheduled_wait.time = 0;//+= dwMilliseconds;

  LARGE_INTEGER liStart =
    SK_QueryPerf ();

  bool same_as_last_time =
    ( pTLS->scheduler->mru_wait.handle == hHandle );

      pTLS->scheduler->mru_wait.handle = hHandle;

  auto ret =
    NtWaitForSingleObject_Original (
      Handle, Alertable, Timeout
    );

  InterlockedAdd ( &scheduled_wait.time_blocked,
                       static_cast <uint64_t> (
                         SK_DeltaPerfMS (liStart.QuadPart, 1)
                       )
                   );

  // We're waiting on the same event as last time on this thread
  if ( same_as_last_time )
  {
    pTLS->scheduler->mru_wait.last_wait = liStart;
    pTLS->scheduler->mru_wait.sequence++;
  }

  // This thread found actual work and has stopped abusing the kernel
  //   waiting on the same always-signaled event; it can have its
  //     normal preemption behavior back  (briefly anyway).
  else
  {
    pTLS->scheduler->mru_wait.start     = liStart;
    pTLS->scheduler->mru_wait.last_wait = liStart;
    pTLS->scheduler->mru_wait.sequence  = 0;
  }

  if ( ret            == WAIT_OBJECT_0 &&   SK_POE2_FixUnityEmployment &&
       dwMilliseconds == INFINITE      && ( bAlertable != FALSE ) )
  {
    // Not to be confused with the other thing
    bool hardly_working =
      (! StrCmpW (pTLS->debug.name, L"Worker Thread"));

    if ( SK_POE2_Stage3UnityFix || hardly_working )
    {
      if (pTLS->scheduler->mru_wait.getRate () >= 0.00666f)
      {
        // This turns preemption of threads in the same priority level off.
        //
        //    * Yes, TRUE means OFF.  Use this wrong and you will hurt
        //                              performance; just sayin'
        //
        if (pTLS->scheduler->mru_wait.preemptive == -1)
        {
          GetThreadPriorityBoost ( GetCurrentThread (),
                                   &pTLS->scheduler->mru_wait.preemptive );
        }

        if (pTLS->scheduler->mru_wait.preemptive == FALSE)
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
        if (pTLS->scheduler->mru_wait.preemptive == -1)
        {
          GetThreadPriorityBoost ( GetCurrentThread (),
                                  &pTLS->scheduler->mru_wait.preemptive );
        }

        if (pTLS->scheduler->mru_wait.preemptive != FALSE)
        {
          SetThreadPriorityBoost (GetCurrentThread (), FALSE);
          InterlockedIncrement   (&SK_POE2_ThreadBoostsKilled);
        }
      }
    }
  }

  // They took our jobs!
  else if (pTLS->scheduler->mru_wait.preemptive != -1)
  {
    SetThreadPriorityBoost (
      GetCurrentThread (),
        pTLS->scheduler->mru_wait.preemptive );

    // Status Quo restored: Jobs nobody wants are back and have
    //   zero future relevance and should be ignored if possible.
    pTLS->scheduler->mru_wait.preemptive = -1;
  }
#endif

  //if (config.system.log_level > 0)
  //{
  //  dll_log.Log ( L"tid=%lu (\"%s\") WaitForSingleObject [Alertable: %lu] Timeout: %lli",
  //                  pTLS->debug.tid,
  //                    pTLS->debug.mapped ? pTLS->debug.name : L"Unnamed",
  //                      Alertable,
  //                        Timeout != nullptr ? Timeout->QuadPart : -1
  //              );
  //}
  //
  //if (Timeout != nullptr)
  //  Timeout = nullptr;
#pragma endregion

  auto ret =
    NtWaitForSingleObject_Original (
      Handle, Alertable, Timeout
    );

  if (ret != STATUS_TIMEOUT)
    SK_MMCS_ApplyPendingTaskPriority ();

  return
    ret;
}



BOOL
WINAPI
SwitchToThread_Detour (void)
{
#ifdef _M_AMD64
  struct game_s {
    bool is_mhw   = SK_GetCurrentGameID () == SK_GAME_ID::MonsterHunterWorld;
    bool is_aco   = SK_GetCurrentGameID () == SK_GAME_ID::AssassinsCreed_Odyssey;
    bool is_elex2 = SK_GetCurrentGameID () == SK_GAME_ID::Elex2;
    bool is_generic = !(is_mhw || is_aco || is_elex2);
  } static game;

  if (game.is_generic) [[likely]]
  {
#endif
    BOOL bRet =
      SwitchToThread_Original ();

    if (bRet)
      SK_MMCS_ApplyPendingTaskPriority ();

    return bRet;
#ifdef _M_AMD64
  }


  static volatile DWORD dwAntiDebugTid = 0;

  extern bool        __SK_MHW_KillAntiDebug;
  if (game.is_mhw && __SK_MHW_KillAntiDebug)
  {
    DWORD dwTid =
      ReadULongAcquire (&dwAntiDebugTid);

    if ( dwTid == 0 ||
         dwTid == SK_Thread_GetCurrentId () )
    {
      SK_TLS *pTLS =
        SK_TLS_Bottom ();

      if ( GetActiveWindow () == 0 )
      {
        return
          SwitchToThread_Original ();
      }

      else if (dwTid == 0)
      {
        dwTid =
          SK_Thread_GetCurrentId ();

        InterlockedExchange (&dwAntiDebugTid, dwTid);
      }

      if (pTLS->debug.tid == dwTid)
      {
        ULONG64 ulFrames =
          SK_GetFramesDrawn ();

        if (pTLS->scheduler->last_frame   < (ulFrames - 3) ||
            pTLS->scheduler->switch_count > 3)
        {
          pTLS->scheduler->switch_count = 0;
        }

        if ( WAIT_TIMEOUT !=
               MsgWaitForMultipleObjectsEx (
                 0, nullptr,
                    pTLS->scheduler->switch_count++,
                      QS_ALLEVENTS & ~QS_INPUT,
                        0x0                )
           )
        {
          pTLS->scheduler->last_frame =
            ulFrames;

          return FALSE;
        }

      //SK_Sleep (pTLS->scheduler->switch_count++);

        pTLS->scheduler->last_frame =
          ulFrames;

        return
          TRUE;
      }
    }

    return
      SwitchToThread_Original ();
  }


  //SK_LOG0 ( ( L"Thread: %x (%s) is SwitchingThreads",
  //            SK_GetCurrentThreadId (), SK_Thread_GetName (
  //              GetCurrentThread ()).c_str ()
  //            ),
  //         L"AntiTamper");


  BOOL bRet = FALSE;

  if (__SK_MHW_KillAntiDebug && game.is_aco)
  {
    config.render.framerate.enable_mmcss = true;

    SK_TLS *pTLS =
      SK_TLS_Bottom ();

    if (pTLS->scheduler->last_frame != SK_GetFramesDrawn ())
        pTLS->scheduler->switch_count = 0;

    bRet = TRUE;

    if (pTLS->scheduler->mmcs_task > (SK_MMCS_TaskEntry *) 1)
    {
      //if (pTLS->scheduler->mmcs_task->hTask > 0)
      //    pTLS->scheduler->mmcs_task->disassociateWithTask ();
    }

    if (pTLS->scheduler->switch_count++ < 20)
    {
      if (pTLS->scheduler->switch_count < 15)
        SK_Sleep (0);
      else
        bRet = SwitchToThread_Original ();
    }
    else
      SK_Sleep (1);

    if (pTLS->scheduler->mmcs_task > (SK_MMCS_TaskEntry *)1)
    {
      auto task =
        pTLS->scheduler->mmcs_task;

      //if (pTLS->scheduler->mmcs_task->hTask > 0)
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

        InterlockedDecrement (&__SK_MMCS_PendingChanges);
      }
    }

    pTLS->scheduler->last_frame =
      SK_GetFramesDrawn ();
  }

  else if (game.is_elex2)
  {
    static volatile LONG iters = 0L;

    if ((InterlockedIncrement (&iters) % 13L) == 0L)
      SK_DelayExecution (0.5, TRUE);

    else if ((ReadAcquire (&iters) % 9L) != 0L)
      SwitchToThread_Original ();
  }

  else
  {
    bRet =
      SwitchToThread_Original ();
  }

  return
    bRet;
#endif
}


#include <SpecialK/framerate.h>

DWORD
WINAPI
SK_DelayExecution (double dMilliseconds, BOOL bAlertable) noexcept
{
  LARGE_INTEGER
    liSleep = { };

  if (dMilliseconds - SK::Framerate::Limiter::timer_res_ms <
                      SK::Framerate::Limiter::timer_res_ms)
  {
    liSleep.QuadPart =
      static_cast <LONGLONG> (-10000.0 * SK::Framerate::Limiter::timer_res_ms);
  }

  else
  {
    liSleep.QuadPart  =
      static_cast <LONGLONG> (
        (-10000.0 * dMilliseconds) -
        (-10000.0 * SK::Framerate::Limiter::timer_res_ms)
      );
  }

  NTSTATUS status =
    NtDelayExecution ( (BOOLEAN)(bAlertable != FALSE),
                         &liSleep );

  if (status == STATUS_ALERTED || status == STATUS_USER_APC)
    return WAIT_IO_COMPLETION;

  if (status == STATUS_SUCCESS || status == STATUS_TIMEOUT)
    return 0;

  return 1;
}

volatile LONG __sleep_init = FALSE;

DWORD
WINAPI
SK_SleepEx (DWORD dwMilliseconds, BOOL bAlertable) noexcept
{
  if (ReadAcquire (&__SK_DLL_Ending))
    return 0;

  if (ReadAcquire (&__sleep_init) == FALSE)
    return SleepEx (dwMilliseconds, bAlertable);

  return
    SleepEx_Original != nullptr                   ?
    SleepEx_Original (dwMilliseconds, bAlertable) :
    SleepEx          (dwMilliseconds, bAlertable);
}

void
WINAPI
SK_Sleep (DWORD dwMilliseconds) noexcept
{
  SK_SleepEx (dwMilliseconds, FALSE);
}


#define _TVFIX


//
// Metaphor fix
//
using timeBeginPeriod_pfn = MMRESULT (WINAPI *)(UINT);
using timeEndPeriod_pfn   = MMRESULT (WINAPI *)(UINT);

static timeBeginPeriod_pfn timeBeginPeriod_Original = nullptr;
static timeEndPeriod_pfn   timeEndPeriod_Original   = nullptr;

MMRESULT
WINAPI
timeBeginPeriod_Detour(_In_ UINT uPeriod)
{
  std::ignore = uPeriod;

  return TIMERR_NOERROR;
}

MMRESULT
WINAPI
timeEndPeriod_Detour(_In_ UINT uPeriod)
{
  std::ignore = uPeriod;

  return TIMERR_NOERROR;
}


DWORD
WINAPI
SleepEx_Detour (DWORD dwMilliseconds, BOOL bAlertable)
{
  if (   ReadAcquire (&__SK_DLL_Ending  ) ||
      (! ReadAcquire (&__SK_DLL_Attached) ) )
  {
    return 0;
  }

  // For sleeps longer than 1 second, let's do some consistency checks
  const bool bLongSleep = 
    (dwMilliseconds > 1000UL);

  static auto game_id =
    SK_GetCurrentGameID ();

  SK_TLS*                            pTLS = nullptr;
  SK_MMCS_ApplyPendingTaskPriority (&pTLS);

  // TODO: Move into actual plug-in
  if (game_id == SK_GAME_ID::ChronoCross)
  {
    if (dwMilliseconds == 0)
    {
      return
        SwitchToThread ();
    }
  }

  if (game_id == SK_GAME_ID::Starfield)
  {
    if (dwMilliseconds == 0)
    {
      return
        SwitchToThread ();
    }
  }

  const bool sleepless_render = config.render.framerate.sleepless_render;
  const bool sleepless_window = config.render.framerate.sleepless_window;

  bool bWantThreadClassification =
    ( sleepless_render ||
      sleepless_window ||
      bLongSleep );

#ifdef _TVFIX
  bWantThreadClassification |=
    ( game_id == SK_GAME_ID::Tales_of_Vesperia &&
            1 == dwMilliseconds );
#endif

  // Time wasted classifying the thread would be undesirable, just replace
  //   with an immediate return to caller :)
  if (dwMilliseconds == 0 && bWantThreadClassification)
    return 1;

  DWORD dwTid =
    ( bWantThreadClassification ) ?
        SK_Thread_GetCurrentId () : 0x0;

#if 1
  BOOL bGUIThread    =
    sleepless_window ? (SK_Thread_GetTEB_FAST ()->Win32ThreadInfo                != nullptr)
                     : FALSE;
#else
  GUITHREADINFO gti        = {                    };
                gti.cbSize = sizeof (GUITHREADINFO);

  BOOL bGUIThread    =
    sleepless_window ? GetGUIThreadInfo (dwTid, &gti) && IsWindow (gti.hwndActive)
                     : FALSE;
#endif

  if (sleepless_render && pTLS == nullptr && (! bGUIThread))
                          pTLS = SK_TLS_Bottom ();

  BOOL bRenderThread =
    sleepless_render ?
      (ReadULongAcquire   (&SK_GetCurrentRenderBackend ().last_thread) == dwTid ||
       ReadULongAcquire   (&SK_GetCurrentRenderBackend ().thread)      == dwTid ||
     (!bGUIThread &&
       ReadULong64Acquire (&pTLS->render->frames_presented)))
                     : FALSE;

  // Steam doesn't init correctly without sleeping for 25 ms
#define STEAM_THRESHOLD 25
#define MIN_FRAMES_DRAWN 5

  if (bRenderThread && SK_GetFramesDrawn () > MIN_FRAMES_DRAWN)
  {
    if (bLongSleep)
    { // This warning might happen repeatedly
      static int times_warned = 0;
      if (     ++times_warned < 5 || config.system.log_level > 0)
      {
        SK_LOGs0 ( L"Scheduler ",
          L"Excessive sleep duration (%d-ms) detected on render thread, limiting to 500 ms",
            dwMilliseconds );
      }     dwMilliseconds = 500;
    }

#ifdef _TVFIX
#pragma region Tales of Vesperia Render NoSleep
#ifdef _M_AMD64
    if (game_id == SK_GAME_ID::Tales_of_Vesperia)
    {
      extern bool SK_TVFix_NoRenderSleep (void);

      if (SK_TVFix_NoRenderSleep ())
      {
        YieldProcessor ( );
        return 0;
      }
    }
#endif
#pragma endregion
#endif

    if ( sleepless_render && dwMilliseconds != INFINITE &&
                             dwMilliseconds != STEAM_THRESHOLD )
    {
      if (config.system.log_level > 0)
      {
        static bool reported = false;
             if ((! reported) || config.system.log_level > 2)
              {
                dll_log->Log ( L"[FrameLimit] Sleep called from render "
                               L"thread: %lu ms!", dwMilliseconds );
                reported = true;
              }
      }

      if (SK_ImGui_Visible)
        SK::Framerate::events.getRenderThreadStats ().wake (std::max (1UL, dwMilliseconds));

      if (bGUIThread)
        SK::Framerate::events.getMessagePumpStats ().wake  (std::max (1UL, dwMilliseconds));


      // Allow 1-ms sleeps
      if (false)//! config.render.framerate.sleepless_window)
      {
        if (dwMilliseconds <= 1)
        {
          return
            SleepEx_Original (0, bAlertable);
        }
      }

      YieldProcessor ( );

      // Check for I/O Wait Completion Before Going Sleepless
      if (bAlertable)
      {
        if (SK_SleepEx (0, TRUE) == WAIT_IO_COMPLETION)
                             return WAIT_IO_COMPLETION;
      }

      return 0;
    }

    if (SK_ImGui_Visible)
      SK::Framerate::events.getRenderThreadStats ().sleep  (std::max (1UL, dwMilliseconds));
  }

  if ((bGUIThread || (game_id == SK_GAME_ID::NieR_Sqrt_1_5 && dwMilliseconds < 3)) && SK_GetFramesDrawn () > MIN_FRAMES_DRAWN)
  {
    if (bLongSleep)
    { // This warning might happen repeatedly
      static int times_warned = 0;
      if (     ++times_warned < 5 || config.system.log_level > 0)
      {
        SK_LOGs0 ( L"Scheduler ",
          L"Excessive sleep duration (%d-ms) detected on window thread, limiting to 250 ms",
            dwMilliseconds );
      }     dwMilliseconds = 250;
    }

    if ( sleepless_window && dwMilliseconds != INFINITE &&
                             dwMilliseconds != STEAM_THRESHOLD )
    {
      if (config.system.log_level > 0)
      {
        static bool reported = false;
              if (! reported)
              {
                dll_log->Log ( L"[FrameLimit] Sleep called from GUI thread: "
                               L"%lu ms!", dwMilliseconds );
                reported = true;
              }
      }

      if (SK_ImGui_Visible)
        SK::Framerate::events.getMessagePumpStats ().wake (std::max (1UL, dwMilliseconds));

      if (bRenderThread && SK_ImGui_Visible)
        SK::Framerate::events.getMessagePumpStats ().wake (std::max (1UL, dwMilliseconds));

      SK_Thread_WaitWhilePumpingMessages (dwMilliseconds, bAlertable, &pTLS);

      // Check for I/O Wait Completion Before Going Sleepless
      if (bAlertable)
      {
        if (SK_SleepEx (0, TRUE) == WAIT_IO_COMPLETION)
                             return WAIT_IO_COMPLETION;
      }

      return 0;
    }

    if (SK_ImGui_Visible)
      SK::Framerate::events.getMessagePumpStats ().sleep (std::max (1UL, dwMilliseconds));
  }

  DWORD max_delta_time =
    sk::narrow_cast <DWORD> (config.render.framerate.max_delta_time);

#ifdef _TVFIX
#pragma region Tales of Vesperia Anti-Stutter Code
#ifdef _M_AMD64
  extern bool SK_TVFix_ActiveAntiStutter (void);

  if ( game_id == SK_GAME_ID::Tales_of_Vesperia &&
                   SK_TVFix_ActiveAntiStutter () )
  {
    if (dwTid == 0)
        dwTid = SK_Thread_GetCurrentId ();

    static DWORD   dwTidBusy = 0;
    static DWORD   dwTidWork = 0;
    static SK_TLS *pTLSBusy  = nullptr;
    static SK_TLS *pTLSWork  = nullptr;

    if (dwTid != 0 && ( dwTidBusy == 0 || dwTidWork == 0 ))
    {
      if (pTLS == nullptr)
          pTLS = SK_TLS_Bottom ();

           if (! _wcsicmp (pTLS->debug.name, L"BusyThread")) { dwTidBusy = dwTid; pTLSBusy = pTLS; }
      else if (! _wcsicmp (pTLS->debug.name, L"WorkThread")) { dwTidWork = dwTid; pTLSWork = pTLS; }
    }

    if ( dwTidBusy == dwTid || dwTidWork == dwTid )
    {
      pTLS = ( dwTidBusy == dwTid ? pTLSBusy :
                                    pTLSWork );

      ULONG64 ulFrames =
        SK_GetFramesDrawn ();


      if (pTLS->scheduler->last_frame < ulFrames)
          pTLS->scheduler->switch_count = 0;

      pTLS->scheduler->last_frame =
        ulFrames;

      YieldProcessor ();

      return 0;
    }

    else if ( dwMilliseconds == 0 ||
              dwMilliseconds == 1 )
    {
      max_delta_time = std::max (2UL, max_delta_time);
      //dll_log.Log ( L"Sleep %lu - %s - %x", dwMilliseconds,
      //               thread_name.c_str (), SK_Thread_GetCurrentId () );
    }
  }
#endif
#pragma endregion
#endif

  // TODO: Stop this nonsense and make an actual parameter for this...
  //         (min sleep?)
  if ( max_delta_time <= dwMilliseconds || SK_GetFramesDrawn () < MIN_FRAMES_DRAWN )
  {
    //dll_log.Log (L"SleepEx (%lu, %s) -- %s", dwMilliseconds, bAlertable ?
    //             L"Alertable" : L"Non-Alertable",
    //             SK_SummarizeCaller ().c_str ());
    return
      SK_SleepEx (dwMilliseconds, bAlertable);
  }

  // Check for I/O Wait Completion Before Going Sleepless
  else if (bAlertable)
  {
    if (SK_SleepEx (0, TRUE) == WAIT_IO_COMPLETION)
                         return WAIT_IO_COMPLETION;
  }

  static const bool bFFXVI =
    (SK_GetCurrentGameID () == SK_GAME_ID::FinalFantasyXVI);

  if (bFFXVI)
  {
    static thread_local uint64_t sleeps_skipped = 0;
    if (                         sleeps_skipped++ % 3 == 0 )
    {
      return
        SK_SleepEx (dwMilliseconds, bAlertable);
    }

    if (SK_CPU_HasMWAITX)
    {
      static alignas(64) uint64_t monitor = 0ULL;

      _mm_monitorx (&monitor, 0, 0);
      _mm_mwaitx   (0x2, 0, ((DWORD)(0.015f * SK_QpcTicksPerMs) * SK_PerfFreqInTsc + 1));
    }
  }

  return 0;
}

using  Thrd_sleep_pfn = void (*)(const xtime*);
static Thrd_sleep_pfn
       Thrd_sleep_Original = nullptr;

// This simple function in msvcp140 may call into
//   kernelbase.dll directly and avoid the hook on
//     kernel32.dll!SleepEx (...).
void
__cdecl
Thrd_sleep_Detour (const xtime* x)
{
  DWORD dwMilliseconds =
    static_cast <DWORD> (
      (x->sec * 1000UL) +
      (x->nsec / 1000000)
    );

  SleepEx_Detour (dwMilliseconds, FALSE);
}

void
WINAPI
Sleep_Detour (DWORD dwMilliseconds)
{
  SleepEx_Detour (dwMilliseconds, FALSE);
}

using SleepConditionVariableSRW_pfn = BOOL (WINAPI *)(
  _Inout_ PCONDITION_VARIABLE ConditionVariable,
  _Inout_ PSRWLOCK            SRWLock,
  _In_    DWORD               dwMilliseconds,
  _In_    ULONG               Flags
);

static SleepConditionVariableSRW_pfn
       SleepConditionVariableSRW_Original = nullptr;

BOOL
WINAPI
SleepConditionVariableSRW_Detour (
  _Inout_ PCONDITION_VARIABLE ConditionVariable,
  _Inout_ PSRWLOCK            SRWLock,
  _In_    DWORD               dwMilliseconds,
  _In_    ULONG               Flags )
{
  //SK_LOG_FIRST_CALL

#if 0
  if (SK_IsCurrentGame (SK_GAME_ID::Starfield))
  {
    SK_LOGs0 ( L"Scheduler ",
                 L"SleepConditionVariableSRW (..., ..., %d, %x) - %ws",
              dwMilliseconds, Flags,
              SK_Thread_GetName (SK_GetCurrentThreadId ()).c_str () );
  }
#endif

  return
    SleepConditionVariableSRW_Original (
         ConditionVariable, SRWLock, dwMilliseconds, Flags
    );
}

BOOL
WINAPI
QueryPerformanceFrequency_Detour (_Out_ LARGE_INTEGER *lpPerfFreq) noexcept
{
  if (lpPerfFreq)
  {
    lpPerfFreq->QuadPart =
      SK_QpcFreq;

    return TRUE;
  }

  return
    FALSE;
}

BOOL
WINAPI
SK_QueryPerformanceCounter (_Out_ LARGE_INTEGER *lpPerformanceCount) noexcept
{
  if (RtlQueryPerformanceCounter != nullptr)
    return RtlQueryPerformanceCounter (lpPerformanceCount);

  else if (QueryPerformanceCounter_Original != nullptr)
    return QueryPerformanceCounter_Original (lpPerformanceCount);

  else
    return QueryPerformanceCounter (lpPerformanceCount);
}

BOOL
WINAPI
QueryPerformanceCounter_Detour (_Out_ LARGE_INTEGER* lpPerformanceCount) noexcept
{
  return
    RtlQueryPerformanceCounter          ?
    RtlQueryPerformanceCounter          (lpPerformanceCount) :
       QueryPerformanceCounter_Original ?
       QueryPerformanceCounter_Original (lpPerformanceCount) :
       QueryPerformanceCounter          (lpPerformanceCount);
}


#ifdef _M_AMD64
extern bool SK_Shenmue_IsLimiterBypassed   (void              );
extern bool SK_Shenmue_InitLimiterOverride (LPVOID pQPCRetAddr);
extern bool SK_Shenmue_UseNtDllQPC;
#endif

#ifdef _M_AMD64
float __SK_SHENMUE_ClockFuzz = 20.0f;
extern volatile LONG SK_BypassResult;
#endif

BOOL
WINAPI
QueryPerformanceCounter_Shenmue_Detour (_Out_ LARGE_INTEGER* lpPerformanceCount)
{
#pragma region Shenmue Clock Fuzzing
#ifdef _M_AMD64
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

    if (ReadAcquire (&__SK_SHENMUE_FinishedButNotPresented))
    {
      if ( SK_Thread_GetCurrentId () == SK_GetRenderThreadID () )
      {
        if ( SK_GetCallingDLL () == SK_Modules->HostApp () )
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
                                     ( static_cast <double> (lpPerformanceCount->QuadPart) -
                                       static_cast <double> (last_poll.QuadPart)           *
                                       static_cast <double> (__SK_SHENMUE_ClockFuzz)       )
                                   );

          last_poll.QuadPart =
           pre_fuzz.QuadPart;

          return bRet;
        }
      }
    }
  }
#endif
#pragma endregion

  return
    QueryPerformanceCounter_Detour (lpPerformanceCount);
}

LARGE_INTEGER _SK_PerfFreq     = { 0UL };
volatile LONG _SK_PerfFreqInit =  FALSE;

LARGE_INTEGER
SK_GetPerfFreq (void)
{
  if (     _SK_PerfFreq.QuadPart != 0)
    return _SK_PerfFreq;

  if (ReadAcquire (&_SK_PerfFreqInit) < 2)
  {
    RtlQueryPerformanceFrequency =
      (QueryPerformanceCounter_pfn)
    SK_GetProcAddress ( L"NtDll",
                         "RtlQueryPerformanceFrequency" );

    if (0 == InterlockedCompareExchange (&_SK_PerfFreqInit, 1, 0))
    {
      if (RtlQueryPerformanceFrequency != nullptr)
          RtlQueryPerformanceFrequency            (&_SK_PerfFreq);
      else if (QueryPerformanceFrequency_Original != nullptr)
               QueryPerformanceFrequency_Original (&_SK_PerfFreq);
      else
        QueryPerformanceFrequency                 (&_SK_PerfFreq);

      InterlockedIncrement (&_SK_PerfFreqInit);

      return _SK_PerfFreq;
    }

    if (ReadAcquire (&_SK_PerfFreqInit) < 2)
    {
      LARGE_INTEGER freq2 = { };

      if (RtlQueryPerformanceFrequency != nullptr)
          RtlQueryPerformanceFrequency            (&freq2);
      else if (QueryPerformanceFrequency_Original != nullptr)
               QueryPerformanceFrequency_Original (&freq2);
      else
        QueryPerformanceFrequency                 (&freq2);

      return
        freq2;
    }
  }

  return
    _SK_PerfFreq;
}

NTSTATUS
NTAPI
NtSetTimerResolution_Detour
(
  IN   ULONG    DesiredResolution,
  IN   BOOLEAN      SetResolution,
  OUT  PULONG   CurrentResolution
)
{
  NTSTATUS ret = 0;

  if (NtQueryTimerResolution == nullptr)
  {   NtQueryTimerResolution =
     (NtQueryTimerResolution_pfn)::SK_GetProcAddress ( L"NtDll",
     "NtQueryTimerResolution" );
  }

  static Concurrency::concurrent_unordered_map
    < std::wstring, int > setters_;

  int *pSetCount = nullptr;

  if (SetResolution)
  {
    pSetCount =
      &setters_ [SK_GetCallerName ()];

    (*pSetCount)++;
  }

  bool bPrint =
    (config.system.log_level > 0);

  // RTSS is going to spam us to death with this, just make note of it once
  if (bPrint && SK_GetCallerName ().find (L"RTSSHooks") != std::wstring::npos)
  {
                bPrint = false;
    SK_RunOnce (bPrint = true);
  }

  if (NtQueryTimerResolution != nullptr)
  {
    ULONG                    min,  max,  cur;
    NtQueryTimerResolution (&min, &max, &cur);

    if ( bPrint               &&
         pSetCount != nullptr && (*(pSetCount) % 100) == 1 )
    {
      SK_LOGs0 ( L"  Timing  ",
                 L"Kernel resolution.: %f ms", static_cast <float>
                                   (cur * 100)/1000000.0f
      );
    }

    // TODO: Make configurable for power saving mode
    //
    if (config.render.framerate.target_fps <= 0.0f)
    {
      if (! SetResolution)
        ret = NtSetTimerResolution_Original (DesiredResolution, SetResolution, CurrentResolution);
      else
        ret = NtSetTimerResolution_Original (max,               TRUE,          CurrentResolution);
    }
  }

  if (bPrint && ((! pSetCount) || (*(pSetCount) % 100) == 1))
  {
    SK_LOGs0 ( L"Scheduler ",
               L"NtSetTimerResolution (%f ms : %s) issued by %s ... %lu times",
                (double)(DesiredResolution * 100.0) / 1000000.0,
                            SetResolution ? L"Set" : L"Get",
                                  SK_GetCallerName ().c_str (),
                                  pSetCount != nullptr ? *pSetCount
                                                       :  0 );
  }

  return ret;
}

#pragma warning(push, 1)
#pragma warning(disable : 4244)
#include <intel/HybridDetect.h>

using  SetProcessAffinityMask_pfn = BOOL (WINAPI *)(HANDLE,DWORD_PTR);
static SetProcessAffinityMask_pfn
       SetProcessAffinityMask_Original = nullptr;

BOOL
WINAPI
SetProcessAffinityMask_Detour (
  _In_ HANDLE    hProcess,
  _In_ DWORD_PTR dwProcessAffinityMask )
{
  SK_LOG_FIRST_CALL

  // Check virtual handle first, generally that's what games would use
  if (              hProcess  == SK_GetCurrentProcess () ||
      GetProcessId (hProcess) == GetCurrentProcessId  ())
  {
    if (config.priority.cpu_affinity_mask != UINT64_MAX)
    {
      static bool        logged_once = false;
      if (std::exchange (logged_once, true) == false ||
          config.system.log_level > 0)
      {
        SK_LOGi0 (
          L"Preventing attempted change (%x) to process affinity mask.",
            dwProcessAffinityMask
        );
      }

      dwProcessAffinityMask =
        config.priority.cpu_affinity_mask;
    }
  }

  return
    SetProcessAffinityMask_Original (hProcess, dwProcessAffinityMask);
}

BOOL
WINAPI
SK_SetProcessAffinityMask (
  _In_ HANDLE    hProcess,
  _In_ DWORD_PTR dwProcessAffinityMask )
{
  DWORD_PTR dwDontCare,
            dwSystemMask;

  GetProcessAffinityMask (
   hProcess, &dwDontCare, &dwSystemMask);
  dwProcessAffinityMask &= dwSystemMask;

  if (     SetProcessAffinityMask_Original != nullptr)
    return SetProcessAffinityMask_Original (hProcess, dwProcessAffinityMask);

  return
    SetProcessAffinityMask (hProcess, dwProcessAffinityMask);
}

void SK_Scheduler_Init (void)
{
  SK_ICommandProcessor
    *pCommandProc = nullptr;

  SK_RunOnce (
     pCommandProc =
       SK_Render_InitializeSharedCVars ()
  );

  if (pCommandProc != nullptr)
  {
    dTicksPerMS =
      static_cast <double> (SK_GetPerfFreq ().QuadPart) / 1000.0;

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


    if (SK_GetCurrentGameID () == SK_GAME_ID::Shenmue)
    {
      SK_CreateDLLHook2 (      L"kernel32",
                                "QueryPerformanceCounter",
                                 QueryPerformanceCounter_Shenmue_Detour,
        static_cast_p2p <void> (&QueryPerformanceCounter_Original),
        static_cast_p2p <void> (&pfnQueryPerformanceCounter) );
    }
    else
    {
#if 0
      SK_CreateDLLHook2 (      L"kernel32",
                                "QueryPerformanceCounter",
                                 QueryPerformanceCounter_Detour,
        static_cast_p2p <void> (&QueryPerformanceCounter_Original),
        static_cast_p2p <void> (&pfnQueryPerformanceCounter) );
#endif
    }
#endif

    SK_CreateDLLHook2 (      L"kernel32",
                              "Sleep",
                               Sleep_Detour,
      static_cast_p2p <void> (&Sleep_Original),
      static_cast_p2p <void> (&pfnSleep) );

    SK_CreateDLLHook2 (      L"Kernel32",
                              "SleepEx",
                               SleepEx_Detour,
      static_cast_p2p <void> (&SleepEx_Original) );

    if (GetModuleHandleW (L"msvcp140"))
    {
      SK_CreateDLLHook2 (      L"msvcp140",
                                "_Thrd_sleep",
                                 Thrd_sleep_Detour,
        static_cast_p2p <void> (&Thrd_sleep_Original) );
    }

    SK_CreateDLLHook2 (      L"Kernel32",
                              "SleepConditionVariableSRW",
                               SleepConditionVariableSRW_Detour,
      static_cast_p2p <void> (&SleepConditionVariableSRW_Original) );

    SK_CreateDLLHook2 (      L"Kernel32",
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

    SK_CreateDLLHook2 (      L"Kernel32",
                              "SetProcessAffinityMask",
                               SetProcessAffinityMask_Detour,
      static_cast_p2p <void> (&SetProcessAffinityMask_Original) );

    SK_CreateDLLHook2 (      L"NtDll",
                              "NtSetTimerResolution",
                               NtSetTimerResolution_Detour,
      static_cast_p2p <void> (&NtSetTimerResolution_Original) );

    //
    // Turn these into nops because they do nothing useful,
    //   there is more overhead calling them than there is benefit.
    //
    SK_CreateDLLHook2 (      L"Kernel32",
                              "timeBeginPeriod",
                               timeBeginPeriod_Detour,
      static_cast_p2p <void> (&timeBeginPeriod_Original) );

    SK_CreateDLLHook2 (      L"Kernel32",
                              "timeEndPeriod",
                               timeEndPeriod_Detour,
      static_cast_p2p <void> (&timeEndPeriod_Original) );

    if (config.priority.cpu_affinity_mask != -1)
    {
      SK_SetProcessAffinityMask ( GetCurrentProcess (),
        (DWORD_PTR)config.priority.cpu_affinity_mask
      );
    }

    HybridDetect::PROCESSOR_INFO    pinfo;
    HybridDetect::GetProcessorInfo (pinfo);

    if (pinfo.IsIntel () && pinfo.hybrid && config.priority.perf_cores_only)
    {      
      DWORD_PTR orig_affinity    = ULONG_PTR_MAX,
                process_affinity = 0,
                system_affinity  = 0;

      GetProcessAffinityMask ( GetCurrentProcess (),
                                &orig_affinity,
                              &system_affinity );

      process_affinity &=
        pinfo.coreMasks [HybridDetect::INTEL_CORE];

      SK_LOGs0 (L"Scheduler",
        L"Intel Hybrid CPU Detected:  Performance Core Mask=%x",
          process_affinity
      );

      SK_SetProcessAffinityMask ( GetCurrentProcess (),
        process_affinity
      );

      // Determine number of CPU cores total, and then the subset of those
      //   cores that the process is allowed to run threads on.
      SYSTEM_INFO        si = { };
      SK_GetSystemInfo (&si);

      DWORD cpu_pop    = std::max (1UL, si.dwNumberOfProcessors);
      process_affinity = 0;
      system_affinity  = 0;

      if (GetProcessAffinityMask (GetCurrentProcess (), &process_affinity,
                                                         &system_affinity))
      {
        cpu_pop = 0;

        for ( auto i = 0 ; i < 64 ; ++i )
        {
          if ((process_affinity >> i) & 0x1)
            ++cpu_pop;
        }
      }

      config.priority.available_cpu_cores =
        std::max (1UL, std::min (cpu_pop, si.dwNumberOfProcessors));
    }

    SK_ApplyQueuedHooks ();

    NtSetTimerResolution     = NtSetTimerResolution_Original;
    NtQueryTimerResolution   =
      reinterpret_cast       <NtQueryTimerResolution_pfn> (
        SK_GetProcAddress ( L"NtDll",
                             "NtQueryTimerResolution" )   );

    if (0 == InterlockedCompareExchange (&__sleep_init, 1, 0))
    {
      pCommandProc->AddVariable ( "Render.FrameRate.SleeplessRenderThread",
              new SK_IVarStub <bool> (&config.render.framerate.sleepless_render));

      pCommandProc->AddVariable ( "Render.FrameRate.SleeplessWindowThread",
              new SK_IVarStub <bool> (&config.render.framerate.sleepless_window));
    }

#ifdef NO_HOOK_QPC
    QueryPerformanceCounter_Original =
      reinterpret_cast <QueryPerformanceCounter_pfn> (
        SK_GetProcAddress ( SK_GetModuleHandle (L"kernel32"),
                           "QueryPerformanceCounter" )
      );
#endif
  }
}

void
SK_Scheduler_Shutdown (void)
{
  //SK_DisableHook (pfnSleep);
  //SK_DisableHook (pfnQueryPerformanceCounter);
}
#pragma warning(pop)