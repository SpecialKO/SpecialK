/*
Copyright 2017 Intel Corporation

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

#include <SpecialK/render/present_mon/MixedRealityTraceConsumer.hpp>

#include <deque>
#include <stdint.h>
#include <unordered_map>

struct LateStageReprojectionRuntimeStats {
    template <typename T>
    class RuntimeStat {
    private:
        T mAvg;
        T mMax;
        size_t mCount;

    public:
        RuntimeStat()
            : mAvg(0)
            , mMax(0)
            , mCount(0)
        {}

        void AddValue(const T& value)
        {
            mAvg += value;
            mMax = std::max<T>(mMax, value);
            mCount++;
        }

        inline T GetAverage() const
        {
            return mAvg / mCount;
        }

        inline T GetMax() const
        {
            return mMax;
        }
    };

    RuntimeStat<double> mGpuPreemptionInMs;
    RuntimeStat<double> mGpuExecutionInMs;
    RuntimeStat<double> mCopyPreemptionInMs;
    RuntimeStat<double> mCopyExecutionInMs;
    RuntimeStat<double> mLsrInputLatchToVsyncInMs;
    double mGpuEndToVsyncInMs = 0.0;
    double mVsyncToPhotonsMiddleInMs = 0.0;
    double mLsrPoseLatencyInMs = 0.0;
    double mAppPoseLatencyInMs = 0.0;
    double mAppSourceReleaseToLsrAcquireInMs = 0.0;
    double mAppSourceCpuRenderTimeInMs = 0.0;
    double mLsrCpuRenderTimeInMs = 0.0;
    size_t mAppMissedFrames = 0;
    size_t mLsrMissedFrames = 0;
    size_t mLsrConsecutiveMissedFrames = 0;
    uint32_t mAppProcessId = 0;
    uint32_t mLsrProcessId = 0;
};

struct LateStageReprojectionData {
    size_t mLifetimeLsrMissedFrames = 0;
    size_t mLifetimeAppMissedFrames = 0;
    std::deque<LateStageReprojectionEvent> mLSRHistory;
    std::deque<LateStageReprojectionEvent> mDisplayedLSRHistory;
    std::deque<LateStageReprojectionEvent> mSourceHistory;

    void PruneDeque(std::deque<LateStageReprojectionEvent>& lsrHistory, uint32_t msTimeDiff, uint32_t maxHistLen);
    void AddLateStageReprojection(LateStageReprojectionEvent& p);
    void UpdateLateStageReprojectionInfo();
    double ComputeHistoryTime() const;
    double ComputeSourceFps() const;
    double ComputeDisplayedFps() const;
    double ComputeFps() const;
    size_t ComputeHistorySize() const;
    LateStageReprojectionRuntimeStats ComputeRuntimeStats() const;

    bool HasData() const { return !mLSRHistory.empty(); }

private:
    double ComputeFps(const std::deque<LateStageReprojectionEvent>& lsrHistory) const;
    double ComputeHistoryTime(const std::deque<LateStageReprojectionEvent>& lsrHistory) const;
};

FILE* CreateLsrCsvFile(char const* path);
void UpdateLsrCsv(LateStageReprojectionData& lsr, ProcessInfo* proc, LateStageReprojectionEvent& p);
void UpdateConsole(std::unordered_map<uint32_t, ProcessInfo> const& activeProcesses, LateStageReprojectionData& lsr);
