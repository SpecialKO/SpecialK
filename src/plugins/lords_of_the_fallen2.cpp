//
// Copyright 2023 Andon "Kaldaien" Coleman
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

void
SK_LOF2_EnableEAC (bool enable)
{
  auto& local_app_data_dir =
    SK_GetLocalAppDataDir ();
  
  std::filesystem::path
    ini_file_name (local_app_data_dir);
  
  auto ini =
    SK_CreateINI ((ini_file_name / L"LOTF2/Saved/Config/Windows/Engine.ini").c_str ());

  if (ini != nullptr)
  {
    ini->parse ();

    ini->get_section (L"EpicOnlineServices").
      add_key_value (L"EnableAntiCheat", enable ? L"True" : L"False");

    ini->write ();

    delete ini;
  }
}

// 32-bit Launcher Bypass Code
void
SK_SEH_LaunchLordsOfTheFallen2 (void)
{
  __try {
    SK_LOF2_EnableEAC (false);

    STARTUPINFOW        sinfo = { };
    PROCESS_INFORMATION pinfo = { };

    sinfo.cb          = sizeof (STARTUPINFOW);
    sinfo.wShowWindow = SW_SHOWNORMAL;
    sinfo.dwFlags     = STARTF_USESHOWWINDOW | STARTF_FORCEOFFFEEDBACK;

    // EAC Launcher has SteamNoOverlayUIDrawing set to 1, we don't want
    //   to inherit that (!!)
    SetEnvironmentVariable (L"SteamNoOverlayUIDrawing", L"0");

    wchar_t     wszEACLauncher [MAX_PATH * 2 + 1] = { };
    _snwprintf (wszEACLauncher, MAX_PATH * 2, LR"(%ws\*%ws\)",
                          SK_GetHostPath (),
                          SK_GetHostPath ());

    SetEnvironmentVariable (L"EAC_LAUNCHERDIR", wszEACLauncher);

    wchar_t                         wszArgs [MAX_PATH * 2] = { };
    GetCurrentDirectoryW (MAX_PATH, wszArgs);
    wcscat (                        wszArgs,
            LR"(\LOTF2\Binaries\Win64\LOTF2-Win64-Shipping.exe -DLSSFG)"
    );

    CreateProcess ( LR"(LOTF2\Binaries\Win64\LOTF2-Win64-Shipping.exe)", wszArgs,
                    nullptr, nullptr, TRUE, CREATE_SUSPENDED, nullptr, SK_GetHostPath (),
                    &sinfo,  &pinfo );

    if (pinfo.hProcess != 0)
    {
      if ( PathFileExistsW (LR"(LOTF2\Binaries\Win64\LOTF2-Win64-Shipping.pdb)") &&
          !PathFileExistsW (LR"(LOTF2\Binaries\Win64\LOTF2-Win64-Shipping.dbg)"))
      {
        if ( IDYES == MessageBox ( GetForegroundWindow (),
               L"Delete Debug Symbols (for Lords of the Fallen 2)?\r\n\r\n"
                  L"\tThis will improve performance when using SK.\r\n\r\n"
                  L"Saves ~1.5 GiB of disk space and memory on each launch.",
               L"Debug Symbols Detected", MB_ICONQUESTION | MB_YESNO )
           )
        {
          DeleteFileW (LR"(LOTF2\Binaries\Win64\LOTF2-Win64-Shipping.pdb)");
        }

        else
        {
          FILE *fIgnore =
            _wfopen (LR"(LOTF2\Binaries\Win64\LOTF2-Win64-Shipping.dbg)", L"w+");

          if (fIgnore != nullptr)
          {
            fputws (L" ", fIgnore);
            fclose (      fIgnore);
          }
        }
      }
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

void
__stdcall
SK_LOTF2_ExitGame (void)
{
  // Restore EAC on exit
  SK_LOF2_EnableEAC (true);
}

void
SK_LOTF2_InitPlugin (void)
{
  plugin_mgr->exit_game_fns.emplace (SK_LOTF2_ExitGame);
}