/** Copyright (c) 2021-2022, NVIDIA CORPORATION.  All rights reserved.*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.*/

#include <windows.h>
#include <TraceLoggingProvider.h>
#include <evntrace.h>
#include <stdlib.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")

typedef enum _PCLSTATS_LATENCY_MARKER_TYPE
{
  PCLSTATS_SIMULATION_START               = 0,
  PCLSTATS_SIMULATION_END                 = 1,
  PCLSTATS_RENDERSUBMIT_START             = 2,
  PCLSTATS_RENDERSUBMIT_END               = 3,
  PCLSTATS_PRESENT_START                  = 4,
  PCLSTATS_PRESENT_END                    = 5,
//PCLSTATS_INPUT_SAMPLE                   = 6, Deprecated
  PCLSTATS_TRIGGER_FLASH                  = 7,
  PCLSTATS_PC_LATENCY_PING                = 8,
  PCLSTATS_OUT_OF_BAND_RENDERSUBMIT_START = 9,
  PCLSTATS_OUT_OF_BAND_RENDERSUBMIT_END   = 10,
  PCLSTATS_OUT_OF_BAND_PRESENT_START      = 11,
  PCLSTATS_OUT_OF_BAND_PRESENT_END        = 12,
  PCLSTATS_CONTROLLER_INPUT_SAMPLE        = 13,
} PCLSTATS_LATENCY_MARKER_TYPE;

typedef enum _PCLSTATS_FLAGS
{
  PCLSTATS_NO_PRESENT_MARKERS = 0x00000001,
} PCLSTATS_FLAGS;

TRACELOGGING_DECLARE_PROVIDER (g_hPCLStatsComponentProvider);

#define PCLSTATS_MARKER(mrk,frid)        TraceLoggingWrite(g_hPCLStatsComponentProvider, "PCLStatsEvent",   TraceLoggingUInt32((mrk), "Marker"), TraceLoggingUInt64((frid), "FrameID"))
#define PCLSTATS_MARKER_V2(mrk,frid)     TraceLoggingWrite(g_hPCLStatsComponentProvider, "PCLStatsEventV2", TraceLoggingUInt32((mrk), "Marker"), TraceLoggingUInt64((frid), "FrameID"), TraceLoggingUInt32(g_PCLStatsFlags, "Flags"))
#define PCLSTATS_IS_PING_MSG_ID(msgId)   ((msgId) == g_PCLStatsWindowMessage)
#define PCLSTATS_SET_ID_THREAD(idThread) (g_PCLStatsIdThread   = (idThread))
#define PCLSTATS_SET_VIRTUAL_KEY(vk)     (g_PCLStatsVirtualKey = (vk))

void PCLSTATS_INIT     (UINT flags);
void PCLSTATS_SHUTDOWN (void);

extern "C" UINT   g_PCLStatsWindowMessage;
extern "C" WORD   g_PCLStatsVirtualKey;
extern "C" HANDLE g_PCLStatsQuitEvent;
extern "C" HANDLE g_PCLStatsPingThread;
extern "C" bool   g_PCLStatsEnable;
extern "C" UINT   g_PCLStatsFlags;
extern "C" DWORD  g_PCLStatsIdThread;
