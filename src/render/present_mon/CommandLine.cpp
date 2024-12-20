// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
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

//#include <generated/version.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <SpecialK/render/present_mon/PresentMon.hpp>
#include <SpecialK/utility.h>
#include <algorithm>

static CommandLineArgs
      gCommandLineArgs;

CommandLineArgs const& GetCommandLineArgs (void)
{
  return gCommandLineArgs;
}

CommandLineArgs* GetCommandLineArgsPtr (void)
{
  return &gCommandLineArgs;
}



bool        SK_Etw_UnregisterSession (const char* szPrefix);
std::string SK_Etw_RegisterSession   (const char* szPrefix, bool bReuse = true);

bool ParseCommandLine (int argc, char** argv)
{
  UNREFERENCED_PARAMETER (argc);
  UNREFERENCED_PARAMETER (argv);

  static std::string session_name =
    SK_Etw_RegisterSession ("SK_PresentMon");

  if (session_name.empty ())
    return false;

  session_name += '\0';

  auto args = &gCommandLineArgs;

  args->mEtlFileName            = nullptr;

  args->mSessionName            = session_name.data   ();
  args->mTargetPid              = GetCurrentProcessId ();

  args->mTerminateExisting      = false;
  args->mTerminateOnProcExit    = false;
  args->mStopExistingSession    =  true;

  args->mTrackDisplay           =  true;
  args->mTrackDebug             = false;
  args->mConsoleOutputType      = ConsoleOutput::Full;
  args->mTryToElevate           = false;

  args->mTrackWMR               = false;

  args->mOutputCsvFileName      = nullptr;
  args->mOutputCsvToFile        = false;
  args->mOutputCsvToStdout      = false;
  args->mOutputQpcTime          = false;
  args->mOutputQpcTimeInSeconds = false;
  args->mScrollLockIndicator    = false;
  args->mExcludeDropped         = false;
  args->mStartTimer             = false;
  args->mTerminateAfterTimer    = false;
  args->mHotkeySupport          = false;
  args->mMultiCsv               = false;

  args->mDelay                  =     0;
  args->mTimer                  =     0;
  args->mHotkeyVirtualKeyCode   =     0;

  // Enable -qpc_time if only -qpc_time_s was provided, since we use that to
  // add the column.
  if (args->mOutputQpcTimeInSeconds) {
      args->mOutputQpcTime = true;
  }

  return true;
}

