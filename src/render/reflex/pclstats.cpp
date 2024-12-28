// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
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

#include <reflex/pclstats.h>

TRACELOGGING_DEFINE_PROVIDER(
    g_hPCLStatsComponentProvider,
    "PCLStatsTraceLoggingProvider",
    (0x0d216f06, 0x82a6, 0x4d49, 0xbc, 0x4f, 0x8f, 0x38, 0xae, 0x56, 0xef, 0xab));

UINT   g_PCLStatsWindowMessage = 0;
WORD   g_PCLStatsVirtualKey    = 0;
HANDLE g_PCLStatsQuitEvent     = nullptr;
HANDLE g_PCLStatsPingThread    = nullptr;
bool   g_PCLStatsEnable        = false;
UINT   g_PCLStatsFlags         = 0;
DWORD  g_PCLStatsIdThread      = 0;

DWORD WINAPI
PCLStatsPingThreadProc (LPVOID)
{
  DWORD minPingInterval = 100 /*ms*/;
  DWORD maxPingInterval = 300 /*ms*/;

  while ( WAIT_TIMEOUT ==
            WaitForSingleObject (g_PCLStatsQuitEvent, minPingInterval + (rand () % (maxPingInterval - minPingInterval))) )
  {
    if (! g_PCLStatsEnable)
    {
      continue;
    }

    if (g_PCLStatsIdThread)
    {
      TraceLoggingWrite  (g_hPCLStatsComponentProvider, "PCLStatsInput", TraceLoggingUInt32 (g_PCLStatsIdThread, "IdThread"));
      PostThreadMessageW (g_PCLStatsIdThread, g_PCLStatsWindowMessage, 0, 0);
      continue;
    }

    HWND hWnd =
      GetForegroundWindow ();

    if (hWnd)
    {
      DWORD                               dwProcessId = 0;
      SK_GetWindowThreadProcessId (hWnd, &dwProcessId);

      if (GetCurrentProcessId () == dwProcessId)
      {
        if ( (g_PCLStatsVirtualKey == VK_F13) ||
             (g_PCLStatsVirtualKey == VK_F14) ||
             (g_PCLStatsVirtualKey == VK_F15) )
        {
          TraceLoggingWrite (      g_hPCLStatsComponentProvider, "PCLStatsInput", TraceLoggingUInt32 (g_PCLStatsVirtualKey, "VirtualKey"));
          PostMessageW      (hWnd, WM_KEYDOWN, g_PCLStatsVirtualKey, 0x00000001);
          PostMessageW      (hWnd, WM_KEYUP,   g_PCLStatsVirtualKey, 0xC0000001);
        }

        else if (g_PCLStatsWindowMessage)
        {
          TraceLoggingWrite (      g_hPCLStatsComponentProvider, "PCLStatsInput", TraceLoggingUInt32 (g_PCLStatsWindowMessage, "MsgId"));
          PostMessageW      (hWnd, g_PCLStatsWindowMessage, 0, 0);
        }

        else
        {
          break;
        }
      }
    }
  }

  return S_OK;
}

void
WINAPI
PCLStatsComponentProviderCb ( LPCGUID,   ULONG ControlCode, UCHAR, ULONGLONG,
                              ULONGLONG, PEVENT_FILTER_DESCRIPTOR, PVOID )
{
  switch (ControlCode)
  {
    case EVENT_CONTROL_CODE_ENABLE_PROVIDER:
      g_PCLStatsEnable = true;
      break;
    case EVENT_CONTROL_CODE_DISABLE_PROVIDER:
      g_PCLStatsEnable = false;
      break;
    case EVENT_CONTROL_CODE_CAPTURE_STATE:
      TraceLoggingWrite (g_hPCLStatsComponentProvider, "PCLStatsFlags", TraceLoggingUInt32 (g_PCLStatsFlags, "Flags"));
      break;
    default:
      break;
  }
}

void
PCLSTATS_INIT (UINT flags)
{
  if (g_PCLStatsWindowMessage == 0)
  {
    g_PCLStatsWindowMessage =
      RegisterWindowMessageW (L"PC_Latency_Stats_Ping");
  }

  g_PCLStatsFlags = flags;

  if (g_PCLStatsQuitEvent == nullptr)
  {
    g_PCLStatsQuitEvent =
      CreateEventW (nullptr, 1, 0, nullptr);
  }

  if (g_PCLStatsQuitEvent != nullptr)
  {
    TraceLoggingRegisterEx (g_hPCLStatsComponentProvider,  PCLStatsComponentProviderCb, nullptr);
    TraceLoggingWrite      (g_hPCLStatsComponentProvider, "PCLStatsInit");

    if (g_PCLStatsPingThread == nullptr)
    {
      g_PCLStatsPingThread =
        SK_Thread_CreateEx (PCLStatsPingThreadProc, L"[SK] PCL Stats Ping");
    }
  }
}

#define PCLSTATS_MARKER(mrk,frid)    TraceLoggingWrite(g_hPCLStatsComponentProvider, "PCLStatsEvent",   TraceLoggingUInt32((mrk), "Marker"), TraceLoggingUInt64((frid), "FrameID"))
#define PCLSTATS_MARKER_V2(mrk,frid) TraceLoggingWrite(g_hPCLStatsComponentProvider, "PCLStatsEventV2", TraceLoggingUInt32((mrk), "Marker"), TraceLoggingUInt64((frid), "FrameID"), TraceLoggingUInt32(g_PCLStatsFlags, "Flags"))

void PCLSTATS_SHUTDOWN (void)
{
  auto& PCLStatsPingThread = g_PCLStatsPingThread;
  auto& PCLStatsQuitEvent  = g_PCLStatsQuitEvent;

  if (std::exchange (g_PCLStatsPingThread, nullptr) != nullptr)
  {
    if ((intptr_t)PCLStatsQuitEvent > 0)
    {
      SetEvent (PCLStatsQuitEvent);
    }

    if ((intptr_t)PCLStatsPingThread > 0)
    {
      WaitForSingleObject (PCLStatsPingThread, 1000);
      CloseHandle         (PCLStatsPingThread);
                           PCLStatsPingThread = 0;
    }
  }

  TraceLoggingWrite      (g_hPCLStatsComponentProvider, "PCLStatsShutdown");
  TraceLoggingUnregister (g_hPCLStatsComponentProvider);

  if (std::exchange (g_PCLStatsQuitEvent, nullptr) != nullptr)
  {
    if ((intptr_t)PCLStatsQuitEvent > 0)
    {
      CloseHandle (PCLStatsQuitEvent);
                   PCLStatsQuitEvent = 0;
    }
  }
}