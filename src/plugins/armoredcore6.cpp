//
// Copyright 2022 Andon "Kaldaien" Coleman
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

#include <SpecialK/stdafx.h>
#include <SpecialK/utility.h>

// 32-bit Launcher Bypass Code
void
SK_SEH_LaunchArmoredCoreVI (void)
{
  __try {
    STARTUPINFOW        sinfo = { };
    PROCESS_INFORMATION pinfo = { };

    sinfo.cb          = sizeof (STARTUPINFOW);
    sinfo.wShowWindow = SW_SHOWNORMAL;
    sinfo.dwFlags     = STARTF_USESHOWWINDOW | STARTF_FORCEOFFFEEDBACK;

    // EAC Launcher has SteamNoOverlayUIDrawing set to 1, we don't want
    //   to inherit that (!!)
    SetEnvironmentVariable (L"SteamNoOverlayUIDrawing", L"0");

    CreateProcess ( L"armoredcore6.exe", nullptr, nullptr, nullptr,
                    TRUE,    CREATE_SUSPENDED, nullptr, SK_GetHostPath (),
                    &sinfo,  &pinfo );

    if (pinfo.hProcess != 0)
    {
      // Save config prior to comitting suicide
      SK_SelfDestruct ();

      ResumeThread     (pinfo.hThread);
      SK_CloseHandle   (pinfo.hThread);
      SK_CloseHandle   (pinfo.hProcess);

      SK_TerminateProcess (0x00);
    }
  }

  __except (EXCEPTION_EXECUTE_HANDLER) {
    // Swallow _all_ exceptions, EAC deserves a swift death
  }
}