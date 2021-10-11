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

#include <SpecialK/render/present_mon/PresentMon.hpp>
#include <SpecialK/render/present_mon/TraceSession.hpp>
#include <SpecialK/thread.h>

extern CommandLineArgs* GetCommandLineArgsPtr  (void);
extern std::string      SK_Etw_RegisterSession (const char* szPrefix, bool bReuse = true);
extern HANDLE         __SK_DLL_TeardownEvent;

int
SK_PresentMon_Main (int argc, char **argv)
{
  // Parse command line arguments.
  if (! ParseCommandLine (argc, argv))
    return 1;

  auto args =
      GetCommandLineArgsPtr ();

//EnableDebugPrivilege ();

  static
    std::string
        session_name;

  // Start the ETW trace session (including consumer and output threads).
  while (! StartTraceSession ())
  {
    if ( WAIT_OBJECT_0 ==
           SK_WaitForSingleObject (__SK_DLL_TeardownEvent, 2000UL) )
    {
      return 6;
    }

        session_name = SK_Etw_RegisterSession ("SK_PresentMon",  true);
    if (session_name.empty ())
        session_name = SK_Etw_RegisterSession ("SK_PresentMon", false);
    if (session_name.empty ()) continue; else
        session_name += '\0';

    args->mSessionName =
        session_name.data ();
  }

  SK_WaitForSingleObject (
    __SK_DLL_TeardownEvent, INFINITE
  );

  return 0;
}
