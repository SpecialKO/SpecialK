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
    PCLSTATS_SIMULATION_START = 0,
    PCLSTATS_SIMULATION_END = 1,
    PCLSTATS_RENDERSUBMIT_START = 2,
    PCLSTATS_RENDERSUBMIT_END = 3,
    PCLSTATS_PRESENT_START = 4,
    PCLSTATS_PRESENT_END = 5,
    //PCLSTATS_INPUT_SAMPLE = 6, Deprecated
    PCLSTATS_TRIGGER_FLASH = 7,
    PCLSTATS_PC_LATENCY_PING = 8,
    PCLSTATS_OUT_OF_BAND_RENDERSUBMIT_START = 9,
    PCLSTATS_OUT_OF_BAND_RENDERSUBMIT_END = 10,
    PCLSTATS_OUT_OF_BAND_PRESENT_START = 11,
    PCLSTATS_OUT_OF_BAND_PRESENT_END = 12,
    PCLSTATS_CONTROLLER_INPUT_SAMPLE = 13,
} PCLSTATS_LATENCY_MARKER_TYPE;

typedef enum _PCLSTATS_FLAGS
{
    PCLSTATS_NO_PRESENT_MARKERS = 0x00000001,
} PCLSTATS_FLAGS;

TRACELOGGING_DECLARE_PROVIDER(g_hPCLStatsComponentProvider);

#define PCLSTATS_DEFINE() \
    TRACELOGGING_DEFINE_PROVIDER( \
        g_hPCLStatsComponentProvider, \
        "PCLStatsTraceLoggingProvider", \
        (0x0d216f06, 0x82a6, 0x4d49, 0xbc, 0x4f, 0x8f, 0x38, 0xae, 0x56, 0xef, 0xab)); \
    UINT   g_PCLStatsWindowMessage = 0; \
    WORD   g_PCLStatsVirtualKey = 0; \
    HANDLE g_PCLStatsQuitEvent = NULL; \
    HANDLE g_PCLStatsPingThread = NULL; \
    bool   g_PCLStatsEnable = false; \
    UINT   g_PCLStatsFlags = 0; \
    DWORD  g_PCLStatsIdThread = 0; \
    DWORD PCLStatsPingThreadProc(LPVOID lpThreadParameter) \
    { \
        DWORD minPingInterval = 100 /*ms*/; \
        DWORD maxPingInterval = 300 /*ms*/; \
        while (WAIT_TIMEOUT == WaitForSingleObject(g_PCLStatsQuitEvent, minPingInterval + (rand() % (maxPingInterval - minPingInterval)))) \
        { \
            if (!g_PCLStatsEnable) \
            { \
                continue; \
            } \
            if (g_PCLStatsIdThread) \
            { \
                TraceLoggingWrite(g_hPCLStatsComponentProvider, "PCLStatsInput", TraceLoggingUInt32(g_PCLStatsIdThread, "IdThread")); \
                PostThreadMessageW(g_PCLStatsIdThread, g_PCLStatsWindowMessage, 0, 0); \
                continue; \
            } \
            HWND hWnd = GetForegroundWindow(); \
            if (hWnd) \
            { \
                DWORD dwProcessId = 0; \
                (void)GetWindowThreadProcessId(hWnd, &dwProcessId); \
                if (GetCurrentProcessId() == dwProcessId) \
                { \
                    if ((g_PCLStatsVirtualKey == VK_F13) || \
                        (g_PCLStatsVirtualKey == VK_F14) || \
                        (g_PCLStatsVirtualKey == VK_F15)) \
                    { \
                        TraceLoggingWrite(g_hPCLStatsComponentProvider, "PCLStatsInput", TraceLoggingUInt32(g_PCLStatsVirtualKey, "VirtualKey")); \
                        PostMessageW(hWnd, WM_KEYDOWN, g_PCLStatsVirtualKey, 0x00000001); \
                        PostMessageW(hWnd, WM_KEYUP,   g_PCLStatsVirtualKey, 0xC0000001); \
                    } \
                    else if (g_PCLStatsWindowMessage) \
                    { \
                        TraceLoggingWrite(g_hPCLStatsComponentProvider, "PCLStatsInput", TraceLoggingUInt32(g_PCLStatsWindowMessage, "MsgId")); \
                        PostMessageW(hWnd, g_PCLStatsWindowMessage, 0, 0); \
                    } \
                    else \
                    { \
                        break; \
                    } \
                } \
            } \
        } \
        return S_OK; \
    } \
    void WINAPI PCLStatsComponentProviderCb(LPCGUID, ULONG ControlCode, UCHAR, ULONGLONG, ULONGLONG, PEVENT_FILTER_DESCRIPTOR, PVOID) \
    { \
        switch (ControlCode) \
        { \
        case EVENT_CONTROL_CODE_ENABLE_PROVIDER: \
            g_PCLStatsEnable = true; \
            break; \
        case EVENT_CONTROL_CODE_DISABLE_PROVIDER: \
            g_PCLStatsEnable = false; \
            break; \
        case EVENT_CONTROL_CODE_CAPTURE_STATE: \
            TraceLoggingWrite(g_hPCLStatsComponentProvider, "PCLStatsFlags", TraceLoggingUInt32(g_PCLStatsFlags, "Flags")); \
            break; \
        default: \
            break; \
        } \
    }

#define PCLSTATS_INIT(flags) \
    if (g_PCLStatsWindowMessage == 0) \
    { \
        g_PCLStatsWindowMessage = RegisterWindowMessageW(L"PC_Latency_Stats_Ping"); \
    } \
    g_PCLStatsFlags = (flags); \
    if (!g_PCLStatsQuitEvent) \
    { \
        g_PCLStatsQuitEvent = CreateEventW(NULL, 1, 0, NULL); \
    } \
    if (g_PCLStatsQuitEvent) \
    { \
        TraceLoggingRegisterEx(g_hPCLStatsComponentProvider, PCLStatsComponentProviderCb, NULL); \
	    TraceLoggingWrite(g_hPCLStatsComponentProvider, "PCLStatsInit"); \
        if (!g_PCLStatsPingThread) \
        { \
            g_PCLStatsPingThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PCLStatsPingThreadProc, NULL, 0, NULL); \
        } \
    }

#define PCLSTATS_MARKER(mrk,frid) TraceLoggingWrite(g_hPCLStatsComponentProvider, "PCLStatsEvent", TraceLoggingUInt32((mrk), "Marker"), TraceLoggingUInt64((frid), "FrameID"))
#define PCLSTATS_MARKER_V2(mrk,frid) TraceLoggingWrite(g_hPCLStatsComponentProvider, "PCLStatsEventV2", TraceLoggingUInt32((mrk), "Marker"), TraceLoggingUInt64((frid), "FrameID"), TraceLoggingUInt32(g_PCLStatsFlags, "Flags"))

#define PCLSTATS_SHUTDOWN() \
    if (g_PCLStatsPingThread) \
    { \
        if (g_PCLStatsQuitEvent) \
        { \
            SetEvent(g_PCLStatsQuitEvent); \
        } \
        (void)WaitForSingleObject(g_PCLStatsPingThread, 1000); \
        CloseHandle(g_PCLStatsPingThread); \
        g_PCLStatsPingThread = NULL; \
    } \
    TraceLoggingWrite(g_hPCLStatsComponentProvider, "PCLStatsShutdown"); \
    TraceLoggingUnregister(g_hPCLStatsComponentProvider); \
    if (g_PCLStatsQuitEvent) \
    { \
        CloseHandle(g_PCLStatsQuitEvent); \
        g_PCLStatsQuitEvent = NULL; \
    }

#define PCLSTATS_IS_PING_MSG_ID(msgId) ((msgId) == g_PCLStatsWindowMessage)
#define PCLSTATS_SET_ID_THREAD(idThread) (g_PCLStatsIdThread = (idThread))
#define PCLSTATS_SET_VIRTUAL_KEY(vk) (g_PCLStatsVirtualKey = (vk))

extern "C" UINT   g_PCLStatsWindowMessage;
extern "C" WORD   g_PCLStatsVirtualKey;
extern "C" HANDLE g_PCLStatsQuitEvent;
extern "C" HANDLE g_PCLStatsPingThread;
extern "C" bool   g_PCLStatsEnable;
extern "C" UINT   g_PCLStatsFlags;
extern "C" DWORD  g_PCLStatsIdThread;

DWORD PCLStatsPingThreadProc(LPVOID lpThreadParameter);
void WINAPI PCLStatsComponentProviderCb(LPCGUID, ULONG ControlCode, UCHAR, ULONGLONG, ULONGLONG, PEVENT_FILTER_DESCRIPTOR, PVOID);
