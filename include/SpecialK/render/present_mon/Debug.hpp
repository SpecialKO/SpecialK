/*
Copyright 2019 Intel Corporation

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
#pragma once

#define DEBUG_VERBOSE 0
#if DEBUG_VERBOSE

// Time relative to first event
#define DEBUG_START_TIME_NS     0ull
#define DEBUG_STOP_TIME_NS      UINT64_MAX

#include <stdint.h>

struct PresentEvent; // Can't include PresentMonTraceConsumer.hpp because it includes Debug.hpp (before defining PresentEvent)
struct EventMetadata;
struct _EVENT_RECORD;

// Initialize debug system
void DebugInitialize(LARGE_INTEGER* firstTimestamp, LARGE_INTEGER timestampFrequency);

// Check if debug is complete
bool DebugDone();

// Print debug information about the handled event
void DebugEvent(_EVENT_RECORD* eventRecord, EventMetadata* metadata);

// Call when a new present is created
void DebugCreatePresent(PresentEvent const& p);

// Call before modifying any present
void DebugModifyPresent(PresentEvent const& p);

// Call when a present is lost
void DebugLostPresent(PresentEvent const& p);

#else

#define DebugInitialize(firstTimestamp, timestampFrequency) (void) firstTimestamp, timestampFrequency
#define DebugDone()                                         false
#define DebugEvent(eventRecord, metadata)                   (void) eventRecord, metadata
#define DebugCreatePresent(p)                               (void) p
#define DebugModifyPresent(p)                               (void) p
#define DebugLostPresent(p)                                 (void) p

#endif
