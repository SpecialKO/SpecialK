/**s
 * This file is part of Special K.
 *
 * Special K is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Special K is distributed in the hope that it will be useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Special K.
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/

#include <Windows.h>
#include <comdef.h>
#include <shlwapi.h>

#include <SpecialK/import.h>
#include <SpecialK/log.h>
#include <SpecialK/utility.h>
#include <SpecialK/diagnostics/compatibility.h>

const std::wstring SK_IMPORT_EARLY         = L"Early";
const std::wstring SK_IMPORT_PLUGIN        = L"PlugIn";
const std::wstring SK_IMPORT_LATE          = L"Late";
const std::wstring SK_IMPORT_LAZY          = L"Lazy";
const std::wstring SK_IMPORT_PROXY         = L"Proxy";

const std::wstring SK_IMPORT_ROLE_DXGI     = L"dxgi";
const std::wstring SK_IMPORT_ROLE_D3D11    = L"d3d11";
const std::wstring SK_IMPORT_ROLE_D3D9     = L"d3d9";
const std::wstring SK_IMPORT_ROLE_OPENGL   = L"OpenGL32";
const std::wstring SK_IMPORT_ROLE_PLUGIN   = L"PlugIn";
const std::wstring SK_IMPORT_ROLE_3RDPARTY = L"ThirdParty";

const std::wstring SK_IMPORT_ARCH_X64      = L"x64";
const std::wstring SK_IMPORT_ARCH_WIN32    = L"Win32";

import_s imports [SK_MAX_IMPORTS] = { };
import_s host_executable          = { };

extern
HMODULE
__stdcall
SK_GetDLL (void);

typedef BOOL (WINAPI *SKPlugIn_Init_pfn)     (HMODULE hSpecialK);
typedef BOOL (WINAPI *SKPlugIn_Shutdown_pfn) (LPVOID  user);


// Fix warnings in dbghelp.h
#pragma warning (disable : 4091)

#define _NO_CVCONST_H
#include <dbghelp.h>

#include <diagnostics/debug_utils.h>

#pragma comment( lib, "dbghelp.lib" )

HMODULE
SK_InitPlugIn64 (HMODULE hLibrary)
{
  dll_log.Log ( L"[ SpecialK ] [*] Initializing Plug-In: %s...",
                  SK_GetModuleName (hLibrary).c_str () );

  SKPlugIn_Init_pfn SKPlugIn_Init =
    (SKPlugIn_Init_pfn)GetProcAddress (
      hLibrary,
        "SKPlugIn_Init"
    );

  if (SKPlugIn_Init != nullptr) {
    if (SKPlugIn_Init (SK_GetDLL ())) {
      dll_log.Log ( L"[ SpecialK ] [*] Plug-In Init Success (%s)!",
                      SK_GetModuleName (hLibrary).c_str () );
    }

    else {
      dll_log.Log (L"[ SpecialK ] [*] Plug-In Init Failed (Plug-In returned false)!");

      FreeLibrary_Original (hLibrary);
      hLibrary = NULL;
    }
  } else {
    dll_log.Log (L"[ SpecialK ] [*] Plug-In Init Failed (Lacks SpecialK PlugIn Entry Point)!");

    FreeLibrary_Original (hLibrary);
    hLibrary = NULL;
  }

  if (hLibrary != NULL)
    SK_SymRefreshModuleList ();

  return hLibrary;
}

void
SK_LoadEarlyImports64 (void)
{
  int success = 0;

  for (int i = 0; i < SK_MAX_IMPORTS; i++) {

    // Skip libraries that are already loaded
    if (imports [i].hLibrary != NULL)
      continue;

    bool         blacklisted = false;
    std::wstring blacklist   =
      imports [i].blacklist != nullptr ?
        imports [i].blacklist->get_value_str () :
        L"";

    if (StrStrIW (blacklist.c_str (), SK_GetHostApp ()))
      blacklisted = true;

    if (imports [i].filename != nullptr) {
      if (imports [i].when != nullptr) {
        if (imports [i].architecture != nullptr) {
          if (imports [i].architecture->get_value () == SK_IMPORT_ARCH_X64 &&
              imports [i].when->get_value         () == SK_IMPORT_EARLY) {

            dll_log.LogEx (true, L"[ SpecialK ]  * Loading Early Custom Import %s... ",
              imports [i].filename->get_value_str ().c_str ());

            if (! blacklisted) {
              imports [i].hLibrary = LoadLibraryW_Original (
                imports [i].filename->get_value_str ().c_str ()
              );

              if (imports [i].hLibrary != NULL)
              {
                imports [i].product_desc =
                  SK_GetDLLVersionStr (
                    SK_GetModuleFullName ( imports [i].hLibrary ).c_str ()
                  );

                dll_log.LogEx (false, L"success!\n");
                ++success;

                if (imports [i].role->get_value () == SK_IMPORT_ROLE_PLUGIN) {
                  imports [i].hLibrary = SK_InitPlugIn64 (imports [i].hLibrary);

                  if (imports [i].hLibrary == NULL)
                    --success;
                }
              } else  {
                _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

                imports [i].hLibrary = (HMODULE)-2;
                dll_log.LogEx (false, L"failed: 0x%04X (%s)!\n",
                               err.WCode (), err.ErrorMessage () );
              }
            } else {
              dll_log.LogEx (false, L"failed: Host App is Blacklisted!\n");
            }
          }
        }
      }
    }
  }

  if (success > 0)
    SK_SymRefreshModuleList ();
}

void
SK_LoadPlugIns64 (void)
{
  int success = 0;

  for (int i = 0; i < SK_MAX_IMPORTS; i++) {

    // Skip libraries that are already loaded
    if (imports [i].hLibrary != NULL)
      continue;

    bool         blacklisted = false;
    std::wstring blacklist   =
      imports [i].blacklist != nullptr ?
        imports [i].blacklist->get_value_str () :
        L"";

    if (StrStrIW (blacklist.c_str (), SK_GetHostApp ()))
      blacklisted = true;

    if (imports [i].filename != nullptr) {
      if (imports [i].when != nullptr) {
        if (imports [i].architecture != nullptr) {
          if (imports [i].architecture->get_value () == SK_IMPORT_ARCH_X64 &&
              imports [i].when->get_value         () == SK_IMPORT_PLUGIN) {

            dll_log.LogEx (true, L"[ SpecialK ]  * Loading Special K Plug-In %s... ",
              imports [i].filename->get_value_str ().c_str ());

            if (! blacklisted) {
              imports [i].hLibrary = LoadLibraryW_Original (
                imports [i].filename->get_value_str ().c_str ()
              );

              if (imports [i].hLibrary != NULL)
              {
                dll_log.LogEx (false, L"success!\n");
                ++success;

                imports [i].product_desc =
                  SK_GetDLLVersionStr (
                    SK_GetModuleFullName ( imports [i].hLibrary ).c_str ()
                  );

                if (imports [i].role->get_value () == SK_IMPORT_ROLE_PLUGIN) {
                  imports [i].hLibrary = SK_InitPlugIn64 (imports [i].hLibrary);

                  if (imports [i].hLibrary == NULL)
                    --success;
                }
              } else  {
                _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

                imports [i].hLibrary = (HMODULE)-2;
                dll_log.Log (L"[ SpecialK ] [*] Failed: 0x%04X (%s)!",
                               err.WCode (), err.ErrorMessage () );
              }
            } else {
              dll_log.Log (L"[ SpecialK ] [*] Failed: Host App is Blacklisted!");
            }
          }
        }
      }
    }
  }

  if (success > 0)
    SK_SymRefreshModuleList ();
}

void
SK_LoadLateImports64 (void)
{
  int success = 0;

  for (int i = 0; i < SK_MAX_IMPORTS; i++) {

    // Skip libraries that are already loaded
    if (imports [i].hLibrary != NULL)
      continue;

    bool         blacklisted = false;
    std::wstring blacklist   =
      imports [i].blacklist != nullptr ?
        imports [i].blacklist->get_value_str () :
        L"";

    if (StrStrIW (blacklist.c_str (), SK_GetHostApp ()))
      blacklisted = true;

    if (imports [i].filename != nullptr) {
      if (imports [i].when != nullptr) {
        if (imports [i].architecture != nullptr) {
          if (imports [i].architecture->get_value () == SK_IMPORT_ARCH_X64 &&
              imports [i].when->get_value         () == SK_IMPORT_LATE) {

            dll_log.LogEx (true, L"[ SpecialK ]  * Loading Late Custom Import %s... ",
              imports [i].filename->get_value_str ().c_str ());

            if (! blacklisted) {
              imports [i].hLibrary = LoadLibraryW_Original (
                imports [i].filename->get_value_str ().c_str ()
              );

              if (imports [i].hLibrary != NULL)
              {
                dll_log.LogEx (false, L"success!\n");
                ++success;

                imports [i].product_desc =
                  SK_GetDLLVersionStr (
                    SK_GetModuleFullName ( imports [i].hLibrary ).c_str ()
                  );
              }

              else
              {
                _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

                imports [i].hLibrary = (HMODULE)-2;
                dll_log.LogEx (false, L"failed: 0x%04X (%s)!\n",
                               err.WCode (), err.ErrorMessage () );
              }
            } else {
              dll_log.LogEx (false, L"failed: Host App is Blacklisted!\n");
            }
          }
        }
      }
    }
  }

  if (success > 0)
    SK_SymRefreshModuleList ();
}

void
SK_LoadLazyImports64 (void)
{
  int success = 0;

  for (int i = 0; i < SK_MAX_IMPORTS; i++) {

    // Skip libraries that are already loaded
    if (imports [i].hLibrary != NULL)
      continue;

    bool         blacklisted = false;
    std::wstring blacklist   =
      imports [i].blacklist != nullptr ?
        imports [i].blacklist->get_value_str () :
        L"";

    if (StrStrIW (blacklist.c_str (), SK_GetHostApp ()))
      blacklisted = true;

    if (imports [i].filename != nullptr) {
      if (imports [i].when != nullptr) {
        if (imports [i].architecture != nullptr) {
          if (imports [i].architecture->get_value () == SK_IMPORT_ARCH_X64 &&
              imports [i].when->get_value         () == SK_IMPORT_LAZY) {

            dll_log.LogEx (true, L"[ SpecialK ]  * Loading Lazy Custom Import %s... ",
                imports [i].filename->get_value_str ().c_str ());

            if (! blacklisted) {
              imports [i].hLibrary = LoadLibraryW_Original (
                imports [i].filename->get_value_str ().c_str ()
              );

              if (imports [i].hLibrary != NULL)
              {
                dll_log.LogEx (false, L"success!\n");
                ++success;

                imports [i].product_desc =
                  SK_GetDLLVersionStr (
                    SK_GetModuleFullName ( imports [i].hLibrary ).c_str ()
                  );
              }

              else
              {
                _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

                imports [i].hLibrary = (HMODULE)-3;
                dll_log.LogEx (false, L"failed: 0x%04X (%s)!\n",
                               err.WCode (), err.ErrorMessage () );
              }
            } else {
              dll_log.LogEx (false, L"failed: Host App is Blacklisted!\n");
            }
          }
        }
      }
    }
  }

  if (success > 0)
    SK_SymRefreshModuleList ();
}


HMODULE
SK_InitPlugIn32 (HMODULE hLibrary)
{
  dll_log.Log ( L"[ SpecialK ] [*] Initializing Plug-In: %s...",
                  SK_GetModuleName (hLibrary).c_str () );

  SKPlugIn_Init_pfn SKPlugIn_Init =
    (SKPlugIn_Init_pfn)GetProcAddress (
      hLibrary,
        "SKPlugIn_Init"
    );

  if (SKPlugIn_Init != nullptr) {
    if (SKPlugIn_Init (SK_GetDLL ())) {
      dll_log.Log ( L"[ SpecialK ] [*] Plug-In Init Success (%s)!",
                      SK_GetModuleName (hLibrary).c_str () );
    }

    else {
      dll_log.Log (L"[ SpecialK ] [*] Plug-In Init Failed (Plug-In returned false)!");

      FreeLibrary_Original (hLibrary);
      hLibrary = NULL;
    }
  } else {
    dll_log.Log (L"[ SpecialK ] [*] Plug-In Init Failed (Lacks SpecialK PlugIn Entry Point)!");

    FreeLibrary_Original (hLibrary);
    hLibrary = NULL;
  }

  if (hLibrary != NULL)
    SK_SymRefreshModuleList ();

  return hLibrary;
}

void
SK_LoadEarlyImports32 (void)
{
  int success = 0;

  for (int i = 0; i < SK_MAX_IMPORTS; i++) {

    // Skip libraries that are already loaded
    if (imports [i].hLibrary != NULL)
      continue;

    bool         blacklisted = false;
    std::wstring blacklist   =
      imports [i].blacklist != nullptr ?
        imports [i].blacklist->get_value_str () :
        L"";

    if (StrStrIW (blacklist.c_str (), SK_GetHostApp ()))
      blacklisted = true;

    if (imports [i].filename != nullptr) {
      if (imports [i].when != nullptr) {
        if (imports [i].architecture != nullptr) {
          if (imports [i].architecture->get_value () == SK_IMPORT_ARCH_WIN32 &&
              imports [i].when->get_value         () == SK_IMPORT_EARLY) {

            dll_log.LogEx (true, L"[ SpecialK ]  * Loading Early Custom Import %s... ",
              imports [i].filename->get_value_str ().c_str ());

            if (! blacklisted) {
              imports [i].hLibrary = LoadLibraryW_Original (
                imports [i].filename->get_value_str ().c_str ()
              );

              if (imports [i].hLibrary != NULL)
              {
                dll_log.LogEx (false, L"success!\n");
                ++success;

                imports [i].product_desc =
                  SK_GetDLLVersionStr (
                    SK_GetModuleFullName ( imports [i].hLibrary ).c_str ()
                  );

                if (imports [i].role->get_value () == SK_IMPORT_ROLE_PLUGIN) {
                  imports [i].hLibrary = SK_InitPlugIn32 (imports [i].hLibrary);

                  if (imports [i].hLibrary == NULL)
                    --success;
                }
              } else  {
                _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

                imports [i].hLibrary = (HMODULE)-2;
                dll_log.LogEx (false, L"failed: 0x%04X (%s)!\n",
                               err.WCode (), err.ErrorMessage () );
              }
            } else {
              dll_log.LogEx (false, L"failed: Host App is Blacklisted!\n");
            }
          }
        }
      }
    }
  }

  if (success > 0)
    SK_SymRefreshModuleList ();
}

void
SK_LoadPlugIns32 (void)
{
  int success = 0;

  for (int i = 0; i < SK_MAX_IMPORTS; i++) {

    // Skip libraries that are already loaded
    if (imports [i].hLibrary != NULL)
      continue;

    bool         blacklisted = false;
    std::wstring blacklist   =
      imports [i].blacklist != nullptr ?
        imports [i].blacklist->get_value_str () :
        L"";

    if (StrStrIW (blacklist.c_str (), SK_GetHostApp ()))
      blacklisted = true;

    if (imports [i].filename != nullptr) {
      if (imports [i].when != nullptr) {
        if (imports [i].architecture != nullptr) {
          if (imports [i].architecture->get_value () == SK_IMPORT_ARCH_WIN32 &&
              imports [i].when->get_value         () == SK_IMPORT_PLUGIN) {

            dll_log.LogEx (true, L"[ SpecialK ]  * Loading Special K Plug-In %s... ",
              imports [i].filename->get_value_str ().c_str ());

            if (! blacklisted) {
              imports [i].hLibrary = LoadLibraryW_Original (
                imports [i].filename->get_value_str ().c_str ()
              );

              if (imports [i].hLibrary != NULL)
              {
                dll_log.LogEx (false, L"success!\n");
                ++success;

                imports [i].product_desc =
                  SK_GetDLLVersionStr (
                    SK_GetModuleFullName ( imports [i].hLibrary ).c_str ()
                  );

                if (imports [i].role->get_value () == SK_IMPORT_ROLE_PLUGIN) {
                  imports [i].hLibrary = SK_InitPlugIn32 (imports [i].hLibrary);

                  if (imports [i].hLibrary == NULL)
                    --success;
                }
              } else  {
                _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

                imports [i].hLibrary = (HMODULE)-2;
                dll_log.Log (L"[ SpecialK ] [*] Failed: 0x%04X (%s)!",
                               err.WCode (), err.ErrorMessage () );
              }
            } else {
              dll_log.Log (L"[ SpecialK ] [*] Failed: Host App is Blacklisted!");
            }
          }
        }
      }
    }
  }

  if (success > 0)
    SK_SymRefreshModuleList ();
}

void
SK_LoadLateImports32 (void)
{
  int success = 0;

  for (int i = 0; i < SK_MAX_IMPORTS; i++) {

    // Skip libraries that are already loaded
    if (imports [i].hLibrary != NULL)
      continue;

    bool         blacklisted = false;
    std::wstring blacklist   =
      imports [i].blacklist != nullptr ?
        imports [i].blacklist->get_value_str () :
        L"";

    if (StrStrIW (blacklist.c_str (), SK_GetHostApp ()))
      blacklisted = true;

    if (imports [i].filename != nullptr) {
      if (imports [i].when != nullptr) {
        if (imports [i].architecture != nullptr) {
          if (imports [i].architecture->get_value () == SK_IMPORT_ARCH_WIN32 &&
              imports [i].when->get_value         () == SK_IMPORT_LATE) {

            dll_log.LogEx (true, L"[ SpecialK ]  * Loading Late Custom Import %s... ",
              imports [i].filename->get_value_str ().c_str ());

            if (! blacklisted) {
              imports [i].hLibrary = LoadLibraryW_Original (
                imports [i].filename->get_value_str ().c_str ()
                );

              if (imports [i].hLibrary != NULL)
              {
                dll_log.LogEx (false, L"success!\n");
                ++success;

                imports [i].product_desc =
                  SK_GetDLLVersionStr (
                    SK_GetModuleFullName ( imports [i].hLibrary ).c_str ()
                  );
              }

              else
              {
                _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

                imports [i].hLibrary = (HMODULE)-2;
                dll_log.LogEx (false, L"failed: 0x%04X (%s)!\n",
                               err.WCode (), err.ErrorMessage () );
              }
            } else {
              dll_log.LogEx (false, L"failed: Host App is Blacklisted!\n");
            }
          }
        }
      }
    }
  }

  if (success > 0)
    SK_SymRefreshModuleList ();
}

void
SK_LoadLazyImports32 (void)
{
  int success = 0;

  for (int i = 0; i < SK_MAX_IMPORTS; i++) {

    // Skip libraries that are already loaded
    if (imports [i].hLibrary != NULL)
      continue;

    bool         blacklisted = false;
    std::wstring blacklist   =
      imports [i].blacklist != nullptr ?
        imports [i].blacklist->get_value_str () :
        L"";

    if (StrStrIW (blacklist.c_str (), SK_GetHostApp ()))
      blacklisted = true;

    if (imports [i].filename != nullptr) {
      if (imports [i].when != nullptr) {
        if (imports [i].architecture != nullptr) {
          if (imports [i].architecture->get_value () == SK_IMPORT_ARCH_WIN32 &&
              imports [i].when->get_value         () == SK_IMPORT_LAZY) {

            dll_log.LogEx (true, L"[ SpecialK ]  * Loading Lazy Custom Import %s... ",
                imports [i].filename->get_value_str ().c_str ());

            if (! blacklisted) {
              imports [i].hLibrary = LoadLibraryW_Original (
                imports [i].filename->get_value_str ().c_str ()
                );

              if (imports [i].hLibrary != NULL)
              {
                dll_log.LogEx (false, L"success!\n");
                ++success;

                imports [i].product_desc =
                  SK_GetDLLVersionStr (
                    SK_GetModuleFullName ( imports [i].hLibrary ).c_str ()
                  );
              }

              else
              {
                _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

                imports [i].hLibrary = (HMODULE)-3;
                dll_log.LogEx (false, L"failed: 0x%04X (%s)!\n",
                               err.WCode (), err.ErrorMessage () );
              }
            } else {
              dll_log.LogEx (false, L"failed: Host App is Blacklisted!\n");
              imports [i].hLibrary = (HMODULE)-3;
            }
          }
        }
      }
    }
  }

  if (success > 0)
    SK_SymRefreshModuleList ();
}

void
SK_UnloadImports (void)
{
  // Unload in reverse order, because that's safer :)
  for (int i = SK_MAX_IMPORTS - 1; i >= 0; i--)
  {
    if (imports [i].hLibrary > 0)
    {
      if (imports [i].role->get_value () == SK_IMPORT_ROLE_PLUGIN)
      {
        SKPlugIn_Shutdown_pfn SKPlugIn_Shutdown =
          (SKPlugIn_Shutdown_pfn)
            GetProcAddress ( imports [i].hLibrary,
                               "SKPlugIn_Shutdown" );

        if (SKPlugIn_Shutdown != nullptr)
          SKPlugIn_Shutdown (nullptr);
      }

      dll_log.LogEx ( true,
                        L"[ SpecialK ] Unloading Custom Import %s... ",
                          imports [i].filename->get_value_str ().c_str ()
      );

      if (FreeLibrary_Original (imports [i].hLibrary))
        dll_log.LogEx (false, L"success!\n");

      else
      {
        _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

        dll_log.LogEx ( false,
                          L"failed: 0x%04X (%s)!\n",
                            err.WCode (),
                              err.ErrorMessage ()
                      );
      }
    }
  }

  SK_SymRefreshModuleList ();
}


/*
[Executable.<FileName>]
Architecture={Any|Win32|x64}                            ; Win32 = 32-bit, x64 = 64-bit
Aliases={none|"filename0,filename1,..."}                ; Other executables that share these settings
BlacklistDLLs={none|"filename0,filename1,..."}          ; DLLs that need to be blocked
RenderAPI={none|auto|OpenGL|D3D9|[dxgi/d3d11/d3d12]}    ; Graphics API used
InputAPIs={none,auto,Win32,Joystick,DirectInput,XInput} ; Input API(s) used
InjectionDelay=[0.0-60.0]                               ; Seconds before initializing mod
SpecialK_Version={any|...}                              ; Version of DLL the game requires


On upgrade:

Move old DLL(s) to versions/{version}/
*/