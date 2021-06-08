/*
Copyright 2017-2020 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <assert.h>
#include <stddef.h>
#include <windows.h>
#include <evntcons.h> // must include after windows.h

#include <SpecialK/render/present_mon/TraceSession.hpp>

#include <SpecialK/render/present_mon/Debug.hpp>
#include <SpecialK/render/present_mon/PresentMonTraceConsumer.hpp>
#include <SpecialK/render/present_mon/MixedRealityTraceConsumer.hpp>

#include <SpecialK/render/present_mon/ETW/Microsoft_Windows_D3D9.h>
#include <SpecialK/render/present_mon/ETW/Microsoft_Windows_Dwm_Core.h>
#include <SpecialK/render/present_mon/ETW/Microsoft_Windows_DXGI.h>
#include <SpecialK/render/present_mon/ETW/Microsoft_Windows_DxgKrnl.h>
#include <SpecialK/render/present_mon/ETW/Microsoft_Windows_EventMetadata.h>
#include <SpecialK/render/present_mon/ETW/Microsoft_Windows_Win32k.h>
#include <SpecialK/render/present_mon/ETW/NT_Process.h>

namespace {

struct TraceProperties : public EVENT_TRACE_PROPERTIES {
    wchar_t mSessionName [MAX_PATH];
};

ULONG
EnableFilteredProvider (
               TRACEHANDLE        sessionHandle,
               GUID        const &sessionGuid,
               GUID        const &providerGuid,
               UCHAR              level,
               ULONGLONG          anyKeywordMask,
               ULONGLONG          allKeywordMask,
  std::vector <USHORT>     const &eventIds )
{
  assert (eventIds.size() >= ANYSIZE_ARRAY);
  assert (eventIds.size() <= MAX_EVENT_FILTER_EVENT_ID_COUNT);

  auto memorySize = sizeof (EVENT_FILTER_EVENT_ID) +
                    sizeof (USHORT) * (eventIds.size () - ANYSIZE_ARRAY);
  auto memory     =
    _aligned_malloc (memorySize, alignof (USHORT));

  if (memory == nullptr)
    return ERROR_NOT_ENOUGH_MEMORY;

  auto filterEventIds =
    (EVENT_FILTER_EVENT_ID *) memory;

  filterEventIds->FilterIn = TRUE;
  filterEventIds->Reserved =    0;
  filterEventIds->Count    =    0;

  for ( auto id : eventIds )
    filterEventIds->Events [filterEventIds->Count++] = id;

  EVENT_FILTER_DESCRIPTOR
    filterDesc      = {                        };
    filterDesc.Ptr  = (ULONGLONG) filterEventIds;
    filterDesc.Size = (ULONG)         memorySize;
    filterDesc.Type = EVENT_FILTER_TYPE_EVENT_ID;

  ENABLE_TRACE_PARAMETERS
    params                  = {                                    };
    params.Version          =      ENABLE_TRACE_PARAMETERS_VERSION_2;
    params.EnableProperty   = EVENT_ENABLE_PROPERTY_IGNORE_KEYWORD_0;
    params.SourceId         =                            sessionGuid;
    params.EnableFilterDesc =                            &filterDesc;
    params.FilterDescCount  =                                      1;

  ULONG timeout = 0;

  auto status =
    EnableTraceEx2 ( sessionHandle, &providerGuid,
                       EVENT_CONTROL_CODE_ENABLE_PROVIDER, level,
                         anyKeywordMask, allKeywordMask, timeout,
                           &params );

  _aligned_free (memory);

  return status;
}

ULONG
EnableProviders ( TRACEHANDLE      sessionHandle,
                  GUID const      &sessionGuid,
                  PMTraceConsumer *pmConsumer,
                  MRTraceConsumer *mrConsumer )
{
  uint64_t anyKeywordMask = 0;
  uint64_t allKeywordMask = 0;

  std::vector <USHORT> eventIds;
  ULONG                status = 0;

  // Start backend providers first to reduce Presents being queued up before
  // we can track them.

  // Microsoft_Windows_DxgKrnl
  anyKeywordMask =
      (uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance |
      (uint64_t) Microsoft_Windows_DxgKrnl::Keyword::Base;
  allKeywordMask = anyKeywordMask;

  eventIds = {
      Microsoft_Windows_DxgKrnl::PresentHistory_Start::Id,
  };

  if (pmConsumer->mTrackDisplay)
  {
    eventIds.push_back (Microsoft_Windows_DxgKrnl::Blit_Info::Id);
    eventIds.push_back (Microsoft_Windows_DxgKrnl::Flip_Info::Id);
    eventIds.push_back (Microsoft_Windows_DxgKrnl::FlipMultiPlaneOverlay_Info::Id);
    eventIds.push_back (Microsoft_Windows_DxgKrnl::HSyncDPCMultiPlane_Info::Id);
    eventIds.push_back (Microsoft_Windows_DxgKrnl::VSyncDPCMultiPlane_Info::Id);
    eventIds.push_back (Microsoft_Windows_DxgKrnl::MMIOFlip_Info::Id);
    eventIds.push_back (Microsoft_Windows_DxgKrnl::MMIOFlipMultiPlaneOverlay_Info::Id);
    eventIds.push_back (Microsoft_Windows_DxgKrnl::Present_Info::Id);
    eventIds.push_back (Microsoft_Windows_DxgKrnl::PresentHistory_Info::Id);
    eventIds.push_back (Microsoft_Windows_DxgKrnl::PresentHistoryDetailed_Start::Id);
    eventIds.push_back (Microsoft_Windows_DxgKrnl::QueuePacket_Start::Id);
    eventIds.push_back (Microsoft_Windows_DxgKrnl::QueuePacket_Stop::Id);
    eventIds.push_back (Microsoft_Windows_DxgKrnl::VSyncDPC_Info::Id);
  }

  status =
    EnableFilteredProvider ( sessionHandle, sessionGuid, Microsoft_Windows_DxgKrnl::GUID,
                               TRACE_LEVEL_INFORMATION, anyKeywordMask, allKeywordMask,
                                 eventIds );

  if (status != ERROR_SUCCESS) return status;

  if (pmConsumer->mTrackDisplay)
  {
    // Microsoft_Windows_Win32k
    anyKeywordMask =
        (uint64_t) Microsoft_Windows_Win32k::Keyword::Updates       |
        (uint64_t) Microsoft_Windows_Win32k::Keyword::Visualization |
        (uint64_t) Microsoft_Windows_Win32k::Keyword::Microsoft_Windows_Win32k_Tracing;
    allKeywordMask =
        (uint64_t) Microsoft_Windows_Win32k::Keyword::Updates |
        (uint64_t) Microsoft_Windows_Win32k::Keyword::Microsoft_Windows_Win32k_Tracing;
    eventIds = {
        Microsoft_Windows_Win32k::TokenCompositionSurfaceObject_Info::Id,
        Microsoft_Windows_Win32k::TokenStateChanged_Info::Id,
    };

    status =
      EnableFilteredProvider ( sessionHandle, sessionGuid, Microsoft_Windows_Win32k::GUID,
                                 TRACE_LEVEL_INFORMATION, anyKeywordMask, allKeywordMask,
                                   eventIds );

    if (status != ERROR_SUCCESS) return status;

    // Microsoft_Windows_Dwm_Core
    anyKeywordMask = 0;
    allKeywordMask = anyKeywordMask;
    eventIds = {
        Microsoft_Windows_Dwm_Core::MILEVENT_MEDIA_UCE_PROCESSPRESENTHISTORY_GetPresentHistory_Info::Id,
        Microsoft_Windows_Dwm_Core::SCHEDULE_PRESENT_Start::Id,
        Microsoft_Windows_Dwm_Core::SCHEDULE_SURFACEUPDATE_Info::Id,
        Microsoft_Windows_Dwm_Core::FlipChain_Pending::Id,
        Microsoft_Windows_Dwm_Core::FlipChain_Complete::Id,
        Microsoft_Windows_Dwm_Core::FlipChain_Dirty::Id,
    };

    status =
      EnableFilteredProvider ( sessionHandle, sessionGuid, Microsoft_Windows_Dwm_Core::GUID,
                                 TRACE_LEVEL_VERBOSE, anyKeywordMask, allKeywordMask,
                                   eventIds );

    if (status != ERROR_SUCCESS) return status;
  }

  // Microsoft_Windows_DXGI
  anyKeywordMask =
      (uint64_t) Microsoft_Windows_DXGI::Keyword::Microsoft_Windows_DXGI_Analytic |
      (uint64_t) Microsoft_Windows_DXGI::Keyword::Events;
  allKeywordMask = anyKeywordMask;
  eventIds = {
      Microsoft_Windows_DXGI::Present_Start::Id,
      Microsoft_Windows_DXGI::Present_Stop::Id,
      Microsoft_Windows_DXGI::PresentMultiplaneOverlay_Start::Id,
      Microsoft_Windows_DXGI::PresentMultiplaneOverlay_Stop::Id,
  };

  status =
    EnableFilteredProvider ( sessionHandle, sessionGuid, Microsoft_Windows_DXGI::GUID,
                               TRACE_LEVEL_INFORMATION, anyKeywordMask, allKeywordMask,
                                 eventIds );

  if (status != ERROR_SUCCESS) return status;

  // Microsoft_Windows_D3D9
  anyKeywordMask =
      (uint64_t) Microsoft_Windows_D3D9::Keyword::Microsoft_Windows_Direct3D9_Analytic |
      (uint64_t) Microsoft_Windows_D3D9::Keyword::Events;
  allKeywordMask = anyKeywordMask;
  eventIds = {
      Microsoft_Windows_D3D9::Present_Start::Id,
      Microsoft_Windows_D3D9::Present_Stop::Id,
  };

  status =
    EnableFilteredProvider ( sessionHandle, sessionGuid, Microsoft_Windows_D3D9::GUID,
                               TRACE_LEVEL_INFORMATION, anyKeywordMask, allKeywordMask,
                                 eventIds );

  if (status != ERROR_SUCCESS) return status;

  if (mrConsumer != nullptr)
  {
    // DHD
    status =
      EnableTraceEx2 ( sessionHandle, &DHD_PROVIDER_GUID, EVENT_CONTROL_CODE_ENABLE_PROVIDER,
                         TRACE_LEVEL_VERBOSE, 0x1C00000, 0, 0, nullptr );

    if (status != ERROR_SUCCESS) return status;

    if (! mrConsumer->mSimpleMode)
    {
      // SPECTRUMCONTINUOUS
      status =
        EnableTraceEx2 ( sessionHandle, &SPECTRUMCONTINUOUS_PROVIDER_GUID, EVENT_CONTROL_CODE_ENABLE_PROVIDER,
                           TRACE_LEVEL_VERBOSE, 0x800000, 0, 0, nullptr );

      if (status != ERROR_SUCCESS) return status;
    }
  }

  return
    ERROR_SUCCESS;
}

void
DisableProviders (TRACEHANDLE sessionHandle)
{
  ULONG status = 0;

  status = EnableTraceEx2 (sessionHandle, &Microsoft_Windows_DXGI::GUID,           EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
  status = EnableTraceEx2 (sessionHandle, &Microsoft_Windows_D3D9::GUID,           EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
  status = EnableTraceEx2 (sessionHandle, &Microsoft_Windows_DxgKrnl::GUID,        EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
  status = EnableTraceEx2 (sessionHandle, &Microsoft_Windows_Win32k::GUID,         EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
  status = EnableTraceEx2 (sessionHandle, &Microsoft_Windows_Dwm_Core::GUID,       EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
}

template<
    bool SAVE_FIRST_TIMESTAMP,
    bool TRACK_DISPLAY,
    bool WMR>
void CALLBACK
EventRecordCallback (EVENT_RECORD *pEventRecord)
{
  auto        session = (TraceSession *)pEventRecord->UserContext;
  auto const& hdr     =                 pEventRecord->EventHeader;

#pragma warning(push)
#pragma warning(disable: 4127) // constant conditional expressions

  if (SAVE_FIRST_TIMESTAMP && session->mStartQpc.QuadPart == 0) {
      session->mStartQpc = hdr.TimeStamp;
  }

  // TODO: specialize realtime callback to exclude NT_Process?

       if (                        hdr.ProviderId == Microsoft_Windows_DxgKrnl::GUID)                      session->mPMConsumer->HandleDXGKEvent               (pEventRecord);
  else if (TRACK_DISPLAY &&        hdr.ProviderId == Microsoft_Windows_Win32k::GUID)                       session->mPMConsumer->HandleWin32kEvent             (pEventRecord);
  else if (TRACK_DISPLAY &&        hdr.ProviderId == Microsoft_Windows_Dwm_Core::GUID)                     session->mPMConsumer->HandleDWMEvent                (pEventRecord);
  else if (                        hdr.ProviderId == Microsoft_Windows_DXGI::GUID)                         session->mPMConsumer->HandleDXGIEvent               (pEventRecord);
  else if (                        hdr.ProviderId == Microsoft_Windows_D3D9::GUID)                         session->mPMConsumer->HandleD3D9Event               (pEventRecord);
  else if (                        hdr.ProviderId == NT_Process::GUID)                                     session->mPMConsumer->HandleNTProcessEvent          (pEventRecord);
  else if (                        hdr.ProviderId == Microsoft_Windows_EventMetadata::GUID)                session->mPMConsumer->HandleMetadataEvent           (pEventRecord);

#pragma warning(pop)
}

ULONG CALLBACK
BufferCallback (EVENT_TRACE_LOGFILEA *pLogFile)
{
  auto session =
    (TraceSession *)pLogFile->Context;

  // TRUE = continue processing events, FALSE = return out of ProcessTrace()
  return
    session->mContinueProcessingBuffers;
}

}

ULONG
TraceSession::Start ( PMTraceConsumer *pmConsumer,
                      MRTraceConsumer *mrConsumer,
                      char const       *etlPath,
                      char const       *sessionName )
{
  assert (mSessionHandle == 0);
  assert (mTraceHandle   == INVALID_PROCESSTRACE_HANDLE);

  mStartQpc.QuadPart = 0;

  mPMConsumer = pmConsumer;
  mMRConsumer = mrConsumer;

  mContinueProcessingBuffers = TRUE;

  // -------------------------------------------------------------------------
  // Configure trace properties
  EVENT_TRACE_LOGFILEA
    traceProps                  = {             };
    traceProps.LogFileName      = (char*) etlPath;
    traceProps.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD |
                                  PROCESS_TRACE_MODE_RAW_TIMESTAMP;
    traceProps.Context          = this;
  /* Output members (passed also to BufferCallback):
  traceProps.CurrentTime
  traceProps.BuffersRead
  traceProps.CurrentEvent
  traceProps.LogfileHeader
  traceProps.BufferSize
  traceProps.Filled
  traceProps.IsKernelTrace
  */

  // Redirect to a specialized event handler: <SAVE_FIRST_TIMESTAMP, FULL, WMR>
  auto saveFirstTimestamp = etlPath != nullptr;
  auto trackDisplay       = pmConsumer->mTrackDisplay;
  auto trackWMR           = mrConsumer != nullptr;

  UINT callbackFlags =
     ( saveFirstTimestamp ? 4 : 0 ) |
     ( trackDisplay       ? 2 : 0 ) |
     ( trackWMR           ? 1 : 0 );

  switch (callbackFlags)
  {
    case 0: traceProps.EventRecordCallback = &EventRecordCallback <false, false, false>; break;
    case 1: traceProps.EventRecordCallback = &EventRecordCallback <false, false, true>;  break;
    case 2: traceProps.EventRecordCallback = &EventRecordCallback <false, true,  false>; break;
    case 3: traceProps.EventRecordCallback = &EventRecordCallback <false, true,  true>;  break;
    case 4: traceProps.EventRecordCallback = &EventRecordCallback <true,  false, false>; break;
    case 5: traceProps.EventRecordCallback = &EventRecordCallback <true,  false, true>;  break;
    case 6: traceProps.EventRecordCallback = &EventRecordCallback <true,  true,  false>; break;
    case 7: traceProps.EventRecordCallback = &EventRecordCallback <true,  true,  true>;  break;
  }

  // When processing log files, we need to use the buffer callback in case
  // the user wants to stop processing before the entire log has been parsed.
  if (traceProps.LogFileName != nullptr)
      traceProps.BufferCallback = &BufferCallback;

  // -------------------------------------------------------------------------
  // For realtime collection, start the session with the required providers
  if (traceProps.LogFileName == nullptr)
  {   traceProps.LoggerName        = (char*) sessionName;
      traceProps.ProcessTraceMode |= PROCESS_TRACE_MODE_REAL_TIME;

    TraceProperties
      sessionProps                     = {                             };
      sessionProps.Wnode.BufferSize    = (ULONG) sizeof(TraceProperties);
      sessionProps.Wnode.ClientContext = 1;                                         // Clock resolution to use when logging the timestamp for each event; 1 == query performance counter
      sessionProps.LogFileMode         = EVENT_TRACE_REAL_TIME_MODE;                // We have a realtime consumer, not writing to a log file
      sessionProps.LogFileNameOffset   = 0;                                         // 0 means no output log file
      sessionProps.LoggerNameOffset    = offsetof (TraceProperties, mSessionName);  // Location of session name; will be written by StartTrace()
    /* Not used:
    sessionProps.Wnode.Guid               // Only needed for private or kernel sessions, otherwise it's an output
    sessionProps.FlushTimer               // How often in seconds buffers are flushed; 0=min (1 second)
    sessionProps.EnableFlags              // Which kernel providers to include in trace
    sessionProps.AgeLimit                 // n/a
    sessionProps.BufferSize = 0;          // Size of each tracing buffer in kB (max 1MB)
    sessionProps.MinimumBuffers = 200;    // Min tracing buffer pool size; must be at least 2 per processor
    sessionProps.MaximumBuffers = 0;      // Max tracing buffer pool size; min+20 by default
    sessionProps.MaximumFileSize = 0;     // Max file size in MB
    */
    /* The following members are output variables, set by StartTrace() and/or ControlTrace()
    sessionProps.Wnode.HistoricalContext  // handle to the event tracing session
    sessionProps.Wnode.TimeStamp          // time this structure was updated
    sessionProps.Wnode.Guid               // session Guid
    sessionProps.Wnode.Flags              // e.g., WNODE_FLAG_TRACED_GUID
    sessionProps.NumberOfBuffers          // trace buffer pool size
    sessionProps.FreeBuffers              // trace buffer pool free count
    sessionProps.EventsLost               // count of events not written
    sessionProps.BuffersWritten           // buffers written in total
    sessionProps.LogBuffersLost           // buffers that couldn't be written to the log
    sessionProps.RealTimeBuffersLost      // buffers that couldn't be delivered to the realtime consumer
    sessionProps.LoggerThreadId           // tracing session identifier
    */

    auto status =
      StartTraceA (&mSessionHandle, sessionName, &sessionProps);

    if (status != ERROR_SUCCESS)
    {
      mSessionHandle = 0;
      return status;
    }

    status =
      EnableProviders (mSessionHandle, sessionProps.Wnode.Guid, pmConsumer, mrConsumer);

    if (status != ERROR_SUCCESS)
    {
      Stop ();
      return status;
    }
  }

  // -------------------------------------------------------------------------
  // Open the trace
  mTraceHandle =
    OpenTraceA (&traceProps);

  if (mTraceHandle == INVALID_PROCESSTRACE_HANDLE)
  {
    auto lastError = GetLastError ();
    Stop ();
    return lastError;
  }

  // -------------------------------------------------------------------------
  // Store trace properties
  mQpcFrequency =
    traceProps.LogfileHeader.PerfFreq;

  // Use current time as start for realtime traces (instead of the first event time)
  if (! saveFirstTimestamp)
    QueryPerformanceCounter (&mStartQpc);

  DebugInitialize (&mStartQpc, mQpcFrequency);

  return
    ERROR_SUCCESS;
}

void
TraceSession::Stop (void)
{
  ULONG status = 0;

  // If collecting realtime events, CloseTrace() will cause ProcessTrace() to
  // stop filling buffers and it will return after it finishes processing
  // events already in it's buffers.
  //
  // If collecting from a log file, ProcessTrace() will continue to process
  // the entire file though, which is why we cancel the processing from the
  // BufferCallback in this case.
  mContinueProcessingBuffers = FALSE;

  // Shutdown the trace and session.
  status = CloseTrace (mTraceHandle);
  mTraceHandle = INVALID_PROCESSTRACE_HANDLE;

  if (mSessionHandle != 0)
  {
    DisableProviders (mSessionHandle);

    TraceProperties
      sessionProps                  = {                              };
      sessionProps.Wnode.BufferSize = (ULONG) sizeof (TraceProperties);
      sessionProps.LoggerNameOffset = offsetof (TraceProperties, mSessionName);

    status =
      ControlTraceW (mSessionHandle, nullptr, &sessionProps, EVENT_TRACE_CONTROL_STOP);

    mSessionHandle = 0;
  }
}

ULONG
TraceSession::StopNamedSession (char const *sessionName)
{
  TraceProperties
    sessionProps                  = {                              };
    sessionProps.Wnode.BufferSize = (ULONG) sizeof (TraceProperties);
    sessionProps.LoggerNameOffset = offsetof (TraceProperties, mSessionName);

  return
    ControlTraceA ((TRACEHANDLE) 0, sessionName, &sessionProps, EVENT_TRACE_CONTROL_STOP);
}


ULONG
TraceSession::CheckLostReports (ULONG *eventsLost, ULONG *buffersLost) const
{
  TraceProperties
    sessionProps                  = {                             };
    sessionProps.Wnode.BufferSize = (ULONG) sizeof(TraceProperties);
    sessionProps.LoggerNameOffset = offsetof(TraceProperties, mSessionName);

  auto status =
    ControlTraceW (mSessionHandle, nullptr, &sessionProps, EVENT_TRACE_CONTROL_QUERY);

  *eventsLost  = sessionProps.EventsLost;
  *buffersLost = sessionProps.RealTimeBuffersLost;

  return status;
}