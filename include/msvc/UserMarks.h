// -----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// -----------------------------------------------------------------------------

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef WIN32_LEAN_AND_MEAN
#else
#include <Windows.h>
#endif
#include <objbase.h>
#include <winerror.h>
#include <Evntprov.h>
#include <assert.h>
#include <intsafe.h>
#include <assert.h>


#ifndef __DIAGHUBTRACE_H__
#define __DIAGHUBTRACE_H__

inline int USER_ID_START = INT32_MIN;

extern "C" __declspec(dllexport) inline int GetId()
{
    if (USER_ID_START >= INT32_MAX)
    {
        return INT32_MAX; // assert should catch it 
    }

    if (USER_ID_START == -1)
    {
        // We reserve 0 for the add user marks ID, set to 0 so it returns 1.
        USER_ID_START = 0;
    }

    return USER_ID_START++;
}

#ifdef DIAGHUB_ENABLE_TRACE_SYSTEM
typedef struct ____DiagHubTraceRegistration
{
    REGHANDLE RegHandle;
    int Flags;
} ___DiagHubTraceRegistration;

#define DIAGHUB_DECLARE_TRACE \
/* 41B0D44F-F34D-458A-8297-59362184834E */ \
GUID ____userMarksProviderId = { 0x41b0d44f, 0xf34d, 0x458a, { 0x82, 0x97, 0x59, 0x36, 0x21, 0x84, 0x83, 0x4e } }; \
___DiagHubTraceRegistration ____diagHubTraceReg; \
int ____userId; \
int ____startId; \
int ____endId; \

extern ___DiagHubTraceRegistration ____diagHubTraceReg;

#define DIAGHUB_TRACE_FLAG_STARTED 0x1
#define DIAGHUB_TRACE_STARTED ((____diagHubTraceReg.Flags & DIAGHUB_TRACE_FLAG_STARTED) != 0)

#define DIAGHUB_START_TRACE_SYSTEM() \
    if (ERROR_SUCCESS == EventRegister(&____userMarksProviderId, NULL, NULL, &____diagHubTraceReg.RegHandle)) \
    { ____diagHubTraceReg.Flags |= DIAGHUB_TRACE_FLAG_STARTED; }

#define USERMARKS_INITIALIZE(name) \
    if (DIAGHUB_TRACE_STARTED) \
    { ____userId = GetId(); \
      DiagHubFireUserEventIdNameMap(____userId, name); \
      assert(____userId < INT32_MAX && "Cannot make more usermarks, hit limit"); }

#define USERMARKRANGE_INITIALIZE(name) \
    if (DIAGHUB_TRACE_STARTED) \
    { ____startId = GetId(); \
    ____endId = GetId(); \
    assert(____startId < INT32_MAX && ____endId < INT32_MAX && "Cannot make more usermarks, hit limit"); \
    GUID rangeGuid; \
    OLECHAR* guidString; \
    (void)CoCreateGuid(&rangeGuid); \
    (void)StringFromCLSID(rangeGuid, &guidString); \
    DiagHubFireUserRangeEvent(guidString, ____startId, ____endId); \
    DiagHubFireUserRangeIdNameMap(guidString, name); \
    CoTaskMemFree(guidString); }

#define DIAGHUB_STOP_TRACE_SYSTEM() \
    if (DIAGHUB_TRACE_STARTED) \
    { EventUnregister(____diagHubTraceReg.RegHandle); \
      ____diagHubTraceReg.Flags &= ~(DIAGHUB_TRACE_FLAG_STARTED); \
    }

#ifndef DIAGHUB_PFORCEINLINE
#if defined(PFORCEINLINE)
#define DIAGHUB_PFORCEINLINE PFORCEINLINE
#elif defined(FORCEINLINE)
#define DIAGHUB_PFORCEINLINE FORCEINLINE
#else
#define DIAGHUB_PFORCEINLINE __forceinline
#endif
#endif

/* The max size limit of an ETW payload is 64k, but that includes the header.
   We take off a bit for ourselves. */
#define MAX_MSG_SIZE_IN_BYTES (63 * 1024)

DIAGHUB_PFORCEINLINE
void
DiagHubFireUserEventIdNameMap(
    int id,
    _In_z_ LPCWSTR nameWideString
)
{
    assert(id <= INT32_MAX);
    assert(NULL != nameWideString && (2 * wcslen(nameWideString)) < MAX_MSG_SIZE_IN_BYTES);

    EVENT_DESCRIPTOR evtDesc;
    EVENT_DATA_DESCRIPTOR evtDataDesc[2];
    /* Include the null in the length */
    size_t nameLenInBytes = 2 * (wcslen(nameWideString) + 1);

    /* ID to name mapping event is 1 */
    EventDescCreate(&evtDesc, 1, 2, 0, 4, 0, 0, 0x1); /* 4 = TRACE_LEVEL_INFORMATION */

    EventDataDescCreate(&evtDataDesc[0], &id, sizeof(id));
    EventDataDescCreate(&evtDataDesc[1], nameWideString, (ULONG)nameLenInBytes);

    EventWrite(____diagHubTraceReg.RegHandle, &evtDesc, ARRAYSIZE(evtDataDesc), evtDataDesc);
}

DIAGHUB_PFORCEINLINE
void
DiagHubFireUserEventWithString(
    int id,
    _In_opt_z_ LPCWSTR markWideString
)
{
    assert(INT32_MIN <= id && id <= INT32_MAX && "Invalid user ID");

    EVENT_DESCRIPTOR evtDesc;
    EVENT_DATA_DESCRIPTOR evtDataDesc[2];

    /* Include the null in the length if the string is not null */
    size_t strLenInBytes = NULL == markWideString ? 0 : (2 * (wcslen(markWideString) + 1));
    assert(strLenInBytes < MAX_MSG_SIZE_IN_BYTES && "Invalid string argument");

    /* Firing write event is 2 */
    EventDescCreate(&evtDesc, 2, 2, 0, 4, 0, 0, 0x1); /* 4 = TRACE_LEVEL_INFORMATION */
    EventDataDescCreate(&evtDataDesc[0], &id, sizeof(id));
    EventDataDescCreate(&evtDataDesc[1], markWideString, (ULONG)strLenInBytes);
    EventWrite(____diagHubTraceReg.RegHandle, &evtDesc, ARRAYSIZE(evtDataDesc), evtDataDesc);
}

DIAGHUB_PFORCEINLINE
void
DiagHubFireUserRangeEvent(
    _In_z_ LPCWSTR rangeGuid,
    int startId,
    int endId
)
{
    assert(INT32_MIN <= startId && startId <= INT32_MAX && INT32_MIN <= endId && endId <= INT32_MAX && "Invalid user ID(s)");
    assert(NULL != rangeGuid && (2 * wcslen(rangeGuid)) < MAX_MSG_SIZE_IN_BYTES && "Invalid string argument");

    EVENT_DESCRIPTOR evtDesc;
    EVENT_DATA_DESCRIPTOR evtDataDesc[3];

    /* Include the null in the length if the string is not null */
    size_t strLenInBytes = 2 * (wcslen(rangeGuid) + 1);

    /* Firing range event is 3 */
    EventDescCreate(&evtDesc, 3, 2, 0, 4, 0, 0, 0x1); /* 4 = TRACE_LEVEL_INFORMATION */
    EventDataDescCreate(&evtDataDesc[0], rangeGuid, (ULONG)strLenInBytes);
    EventDataDescCreate(&evtDataDesc[1], &startId, sizeof(startId));
    EventDataDescCreate(&evtDataDesc[2], &endId, sizeof(endId));

    EventWrite(____diagHubTraceReg.RegHandle, &evtDesc, ARRAYSIZE(evtDataDesc), evtDataDesc);
}

DIAGHUB_PFORCEINLINE
void
DiagHubFireUserRangeIdNameMap(
    _In_z_ LPCWSTR rangeGuid,
    _In_z_ LPCWSTR nameWideString
)
{
    assert(NULL != rangeGuid && (2 * wcslen(rangeGuid)) < MAX_MSG_SIZE_IN_BYTES && "Invalid string argument");
    assert(NULL != nameWideString && (2 * wcslen(nameWideString)) < MAX_MSG_SIZE_IN_BYTES && "Invalid string argument");

    EVENT_DESCRIPTOR evtDesc;
    EVENT_DATA_DESCRIPTOR evtDataDesc[2];

    /* Include the null in the length */
    size_t nameLenInBytes = 2 * (wcslen(nameWideString) + 1);
    size_t strLenInBytes = 2 * (wcslen(rangeGuid) + 1);

    /* Range guid to name mapping event is 4 */
    EventDescCreate(&evtDesc, 4, 2, 0, 4, 0, 0, 0x1); /* 4 = TRACE_LEVEL_INFORMATION */

    EventDataDescCreate(&evtDataDesc[0], rangeGuid, (ULONG)strLenInBytes);
    EventDataDescCreate(&evtDataDesc[1], nameWideString, (ULONG)nameLenInBytes);

    EventWrite(____diagHubTraceReg.RegHandle, &evtDesc, ARRAYSIZE(evtDataDesc), evtDataDesc);
}

#define USERMARKS_EMIT_MESSAGE(markWideString) \
    if (DIAGHUB_TRACE_STARTED) \
    { assert(INT32_MIN <= ____userId && ____userId <= INT32_MAX && "Invalid ID"); \
      DiagHubFireUserEventWithString(____userId, markWideString); }

#define USERMARKS_EMIT(markWideString) \
    if (DIAGHUB_TRACE_STARTED) \
    { DiagHubFireUserEventWithString(INT32_MIN, markWideString); }


#define USERMARKRANGE_START(markWideString) \
    if (DIAGHUB_TRACE_STARTED) \
    { assert(INT32_MIN <= ____startId && ____startId <= INT32_MAX && "Invalid ID"); \
      assert(INT32_MIN <= ____endId && ____endId <= INT32_MAX && "Invalid ID"); \
      DiagHubFireUserEventWithString(____startId, markWideString); }


#define USERMARKRANGE_END() \
    if (DIAGHUB_TRACE_STARTED) \
    { assert(INT32_MIN <= ____startId && ____startId <= INT32_MAX && "Invalid ID"); \
      assert(INT32_MIN <= ____endId && ____endId <= INT32_MAX && "Invalid ID"); \
      DiagHubFireUserEventWithString(____endId, L""); }

#else

#define DIAGHUB_DECLARE_TRACE ;
#define DIAGHUB_TRACE_STARTED (0)

#define DIAGHUB_START_TRACE_SYSTEM() do {} while(0);
#define DIAGHUB_STOP_TRACE_SYSTEM() do {} while(0);
#define USERMARKS_INITIALIZE(name) do {} while(0);
#define USERMARKRANGE_INITIALIZE(name) do {} while(0);

#define USERMARKS_EMIT_MESSAGE(m) return S_OK;
#define USERMARKS_EMIT(m) return S_OK;

#define USERMARKRANGE_START(m) return S_OK;
#define USERMARKRANGE_END(m) return S_OK;

#endif

#endif /* __DIAGHUBTRACE_H__ */
