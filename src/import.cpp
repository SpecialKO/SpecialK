/**
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
#include <atlbase.h>

#include <SpecialK/import.h>
#include <SpecialK/log.h>
#include <SpecialK/utility.h>
#include <SpecialK/diagnostics/compatibility.h>
#include <SpecialK/config.h>
#include <SpecialK/core.h>

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

using SKPlugIn_Init_pfn     = BOOL (WINAPI *)(HMODULE hSpecialK);
using SKPlugIn_Shutdown_pfn = BOOL (WINAPI *)(LPVOID  user);


// Fix warnings in dbghelp.h
#pragma warning (disable : 4091)

#define _NO_CVCONST_H
#define _IMAGEHLP_SOURCE_
#include <dbghelp.h>

#include <diagnostics/debug_utils.h>

int
SK_Import_GetNumberOfPlugIns (void)
{
  int num = 0;

  const std::wstring& target_arch =
    SK_RunLHIfBitness ( 64, SK_IMPORT_ARCH_X64,
                            SK_IMPORT_ARCH_WIN32 );

  for (int i = 0; i < SK_MAX_IMPORTS; i++)
  {
    if (imports [i].name.empty ())
      continue;

    if (imports [i].architecture->get_value () == target_arch)
    {
      if (imports [i].when->get_value () == SK_IMPORT_PLUGIN)
      {
        ++num;
      }
    }
  }

  return num;
}

bool
SK_Import_GetShimmedLibrary (HMODULE hModShim, HMODULE& hModReal)
{
  //
  // This is the preferred method for doing this; invoking LoadLibraryW (...) from the shim itself
  //   tends to have undefined results due to loader locking.
  //
  using SK_SHIM_GetReShadeFilename_pfn = const wchar_t* (__stdcall *)(void);

  auto SK_SHIM_GetReShadeFilename =
    (SK_SHIM_GetReShadeFilename_pfn)GetProcAddress (hModShim, "SK_SHIM_GetReShadeFilename");

  if (SK_SHIM_GetReShadeFilename != nullptr)
  {
    hModReal = LoadLibraryW_Original (SK_SHIM_GetReShadeFilename ());

    if (hModReal != nullptr)
      return true;
  }


  using SK_SHIM_GetReShade_pfn = HMODULE (__stdcall *)(void);

  auto SK_SHIM_GetReShade =
    (SK_SHIM_GetReShade_pfn)GetProcAddress (hModShim, "SK_SHIM_GetReShade");

  if (SK_SHIM_GetReShade != nullptr)
  {
    hModReal =
      SK_SHIM_GetReShade ();

    if (hModReal != nullptr)
    {
      return true;
    }
  }


  return false;
}

auto
SK_LoadImportModule = [&](import_s& import)
{
  if (config.system.central_repository)
  {
    wchar_t wszProfilePlugIn [MAX_PATH * 2] = { };
    wcsncpy (wszProfilePlugIn, SK_GetConfigPath (), MAX_PATH);
    PathAppendW (wszProfilePlugIn, import.filename->get_value_str ().c_str ());

    import.hLibrary = LoadLibraryW_Original (
      wszProfilePlugIn
    );
  }

  if (import.hLibrary == nullptr)
  {
    import.hLibrary = LoadLibraryW_Original (
      import.filename->get_value_str ().c_str ()
    );
  }
};

HMODULE
SK_InitPlugIn64 (HMODULE hLibrary)
{
  dll_log.Log ( L"[ SpecialK ] [*] Initializing Plug-In: %s...",
                  SK_GetModuleName (hLibrary).c_str () );

  auto SKPlugIn_Init =
    reinterpret_cast <SKPlugIn_Init_pfn> (
      GetProcAddress (
        hLibrary,
          "SKPlugIn_Init"
      )
    );

  if (SKPlugIn_Init != nullptr) 
  {
    if (SKPlugIn_Init (SK_GetDLL ()))
    {
      dll_log.Log ( L"[ SpecialK ] [*] Plug-In Init Success (%s)!",
                      SK_GetModuleName (hLibrary).c_str () );
    }

    else
    {
      dll_log.Log (L"[ SpecialK ] [*] Plug-In Init Failed (Plug-In returned false)!");

      FreeLibrary_Original (hLibrary);
      hLibrary = nullptr;
    }
  }

  else
  {
    dll_log.Log (L"[ SpecialK ] [*] Plug-In Init Failed (Lacks SpecialK PlugIn Entry Point)!");

    FreeLibrary_Original (hLibrary);
    hLibrary = nullptr;
  }

  if (hLibrary != nullptr)
    SK_SymRefreshModuleList ();

  return hLibrary;
}

void
SK_LoadEarlyImports64 (void)
{
  int success = 0;

  for (auto& import : imports)
  {
    // Skip libraries that are already loaded
    if (import.hLibrary != nullptr)
      continue;

    bool         blacklisted = false;
    std::wstring blacklist   =
      import.blacklist != nullptr ?
        import.blacklist->get_value_str () :
        L"";

    if (StrStrIW (blacklist.c_str (), SK_GetHostApp ()))
      blacklisted = true;

    if (import.filename != nullptr)
    {
      if (import.when != nullptr)
      {
        if (import.architecture != nullptr)
        {
          if (import.architecture->get_value () == SK_IMPORT_ARCH_X64 &&
              import.when->get_value         () == SK_IMPORT_EARLY)
          {
            CHeapPtr <wchar_t> file (_wcsdup (import.filename->get_value_str ().c_str ()));

            SK_StripUserNameFromPathW (file);

            dll_log.LogEx (true, L"[ SpecialK ]  * Loading Early Custom Import %s... ",
              file.m_pData);

            if (! blacklisted)
            {
              SK_LoadImportModule (import);

              if (import.hLibrary != nullptr)
              {
                if (SK_Import_GetShimmedLibrary (import.hLibrary, import.hShim))
                  std::swap (import.hLibrary, import.hShim);

                import.product_desc =
                  SK_GetDLLVersionStr (
                    SK_GetModuleFullName ( import.hLibrary ).c_str ()
                  );

                dll_log.LogEx (false, L"success!\n");
                ++success;

                if (import.role->get_value () == SK_IMPORT_ROLE_PLUGIN) {
                  import.hLibrary = SK_InitPlugIn64 (import.hLibrary);

                  if (import.hLibrary == nullptr)
                    --success;
                }
              }

              else
              {
                _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

                import.hLibrary = (HMODULE)-2;
                dll_log.LogEx (false, L"failed: 0x%04X (%s)!\n",
                               err.WCode (), err.ErrorMessage () );
              }
            }

            else
            {
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

  for (auto& import : imports)
  {
    // Skip libraries that are already loaded
    if (import.hLibrary != nullptr)
      continue;

    bool         blacklisted = false;
    std::wstring blacklist   =
      import.blacklist != nullptr ?
        import.blacklist->get_value_str () :
        L"";

    if (StrStrIW (blacklist.c_str (), SK_GetHostApp ()))
      blacklisted = true;

    if (import.filename != nullptr)
    {
      if (import.when != nullptr)
      {
        if (import.architecture != nullptr)
        {
          if (import.architecture->get_value () == SK_IMPORT_ARCH_X64 &&
              import.when->get_value         () == SK_IMPORT_PLUGIN)
          {
            CHeapPtr <wchar_t> file (_wcsdup (import.filename->get_value_str ().c_str ()));

            SK_StripUserNameFromPathW (file);

            dll_log.LogEx (true, L"[ SpecialK ]  * Loading Special K Plug-In %s... ",
              file.m_pData);

            if (! blacklisted)
            {
              SK_LoadImportModule (import);

              if (import.hLibrary != nullptr)
              {
                if (SK_Import_GetShimmedLibrary (import.hLibrary, import.hShim))
                  std::swap (import.hLibrary, import.hShim);

                dll_log.LogEx (false, L"success!\n");
                ++success;

                import.product_desc =
                  SK_GetDLLVersionStr (
                    SK_GetModuleFullName ( import.hLibrary ).c_str ()
                  );

                if (import.role->get_value () == SK_IMPORT_ROLE_PLUGIN)
                {
                  import.hLibrary = SK_InitPlugIn64 (import.hLibrary);

                  if (import.hLibrary == nullptr)
                    --success;
                }
              }

              else
              {
                _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

                import.hLibrary = (HMODULE)-2;
                dll_log.Log (L"[ SpecialK ] [*] Failed: 0x%04X (%s)!",
                               err.WCode (), err.ErrorMessage () );
              }
            }

            else
            {
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

  for (auto& import : imports)
  {
    // Skip libraries that are already loaded
    if (import.hLibrary != nullptr)
      continue;

    bool         blacklisted = false;
    std::wstring blacklist   =
      import.blacklist != nullptr ?
        import.blacklist->get_value_str () :
        L"";

    if (StrStrIW (blacklist.c_str (), SK_GetHostApp ()))
      blacklisted = true;

    if (import.filename != nullptr)
    {
      if (import.when != nullptr)
      {
        if (import.architecture != nullptr)
        {
          if (import.architecture->get_value () == SK_IMPORT_ARCH_X64 &&
              import.when->get_value         () == SK_IMPORT_LATE)
          {
            CHeapPtr <wchar_t> file (_wcsdup (import.filename->get_value_str ().c_str ()));

            SK_StripUserNameFromPathW (file);

            dll_log.LogEx (true, L"[ SpecialK ]  * Loading Late Custom Import %s... ",
              file.m_pData);

            if (! blacklisted)
            {
              SK_LoadImportModule (import);

              if (import.hLibrary != nullptr)
              {
                if (SK_Import_GetShimmedLibrary (import.hLibrary, import.hShim))
                  std::swap (import.hLibrary, import.hShim);

                dll_log.LogEx (false, L"success!\n");
                ++success;

                import.product_desc =
                  SK_GetDLLVersionStr (
                    SK_GetModuleFullName ( import.hLibrary ).c_str ()
                  );
              }

              else
              {
                _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

                import.hLibrary = (HMODULE)-2;
                dll_log.LogEx (false, L"failed: 0x%04X (%s)!\n",
                               err.WCode (), err.ErrorMessage () );
              }
            }

            else
            {
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

  for (auto& import : imports)
  {
    // Skip libraries that are already loaded
    if (import.hLibrary != nullptr)
      continue;

    bool         blacklisted = false;
    std::wstring blacklist   =
      import.blacklist != nullptr ?
        import.blacklist->get_value_str () :
        L"";

    if (StrStrIW (blacklist.c_str (), SK_GetHostApp ()))
      blacklisted = true;

    if (import.filename != nullptr)
    {
      if (import.when != nullptr)
      {
        if (import.architecture != nullptr)
        {
          if (import.architecture->get_value () == SK_IMPORT_ARCH_X64 &&
              import.when->get_value         () == SK_IMPORT_LAZY)
          {
            CHeapPtr <wchar_t> file (_wcsdup (import.filename->get_value_str ().c_str ()));

            SK_StripUserNameFromPathW (file);

            dll_log.LogEx (true, L"[ SpecialK ]  * Loading Lazy Custom Import %s... ",
                file.m_pData);

            if (! blacklisted)
            {
              SK_LoadImportModule (import);

              if (import.hLibrary != nullptr)
              {
                if (SK_Import_GetShimmedLibrary (import.hLibrary, import.hShim))
                  std::swap (import.hLibrary, import.hShim);

                dll_log.LogEx (false, L"success!\n");
                ++success;

                import.product_desc =
                  SK_GetDLLVersionStr (
                    SK_GetModuleFullName ( import.hLibrary ).c_str ()
                  );
              }

              else
              {
                _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

                import.hLibrary = (HMODULE)-3;
                dll_log.LogEx (false, L"failed: 0x%04X (%s)!\n",
                               err.WCode (), err.ErrorMessage () );
              }
            }

            else
            {
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

  auto SKPlugIn_Init =
    reinterpret_cast <SKPlugIn_Init_pfn> (
      GetProcAddress (
        hLibrary,
          "SKPlugIn_Init"
      )
    );

  if (SKPlugIn_Init != nullptr)
  {
    if (SKPlugIn_Init (SK_GetDLL ()))
    {
      dll_log.Log ( L"[ SpecialK ] [*] Plug-In Init Success (%s)!",
                      SK_GetModuleName (hLibrary).c_str () );
    }

    else
    {
      dll_log.Log (L"[ SpecialK ] [*] Plug-In Init Failed (Plug-In returned false)!");

      FreeLibrary_Original (hLibrary);
      hLibrary = nullptr;
    }
  }

  else
  {
    dll_log.Log (L"[ SpecialK ] [*] Plug-In Init Failed (Lacks SpecialK PlugIn Entry Point)!");

    FreeLibrary_Original (hLibrary);
    hLibrary = nullptr;
  }

  if (hLibrary != nullptr)
    SK_SymRefreshModuleList ();

  return hLibrary;
}

void
SK_LoadEarlyImports32 (void)
{
  int success = 0;

  for (auto& import : imports)
  {
    // Skip libraries that are already loaded
    if (import.hLibrary != nullptr)
      continue;

    bool         blacklisted = false;
    std::wstring blacklist   =
      import.blacklist != nullptr ?
        import.blacklist->get_value_str () :
        L"";

    if (StrStrIW (blacklist.c_str (), SK_GetHostApp ()))
      blacklisted = true;

    if (import.filename != nullptr)
    {
      if (import.when != nullptr)
      {
        if (import.architecture != nullptr)
        {
          if (import.architecture->get_value () == SK_IMPORT_ARCH_WIN32 &&
              import.when->get_value         () == SK_IMPORT_EARLY)
          {
            CHeapPtr <wchar_t> file (_wcsdup (import.filename->get_value_str ().c_str ()));

            SK_StripUserNameFromPathW (file);

            dll_log.LogEx (true, L"[ SpecialK ]  * Loading Early Custom Import %s... ",
              file.m_pData);

            if (! blacklisted)
            {
              SK_LoadImportModule (import);

              if (import.hLibrary != nullptr)
              {
                if (SK_Import_GetShimmedLibrary (import.hLibrary, import.hShim))
                  std::swap (import.hLibrary, import.hShim);

                dll_log.LogEx (false, L"success!\n");
                ++success;

                import.product_desc =
                  SK_GetDLLVersionStr (
                    SK_GetModuleFullName ( import.hLibrary ).c_str ()
                  );

                if (import.role->get_value () == SK_IMPORT_ROLE_PLUGIN)
                {
                  import.hLibrary = SK_InitPlugIn32 (import.hLibrary);

                  if (import.hLibrary == nullptr)
                    --success;
                }
              }

              else
              {
                _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

                import.hLibrary = (HMODULE)-2;
                dll_log.LogEx (false, L"failed: 0x%04X (%s)!\n",
                               err.WCode (), err.ErrorMessage () );
              }
            }

            else
            {
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

  for (auto& import : imports)
  {
    // Skip libraries that are already loaded
    if (import.hLibrary != nullptr)
      continue;

    bool         blacklisted = false;
    std::wstring blacklist   =
      import.blacklist != nullptr ?
        import.blacklist->get_value_str () :
        L"";

    if (StrStrIW (blacklist.c_str (), SK_GetHostApp ()))
      blacklisted = true;

    if (import.filename != nullptr)
    {
      if (import.when != nullptr)
      {
        if (import.architecture != nullptr)
        {
          if (import.architecture->get_value () == SK_IMPORT_ARCH_WIN32 &&
              import.when->get_value         () == SK_IMPORT_PLUGIN)
          {
            CHeapPtr <wchar_t> file (_wcsdup (import.filename->get_value_str ().c_str ()));

            SK_StripUserNameFromPathW (file);

            dll_log.LogEx (true, L"[ SpecialK ]  * Loading Special K Plug-In %s... ",
              file.m_pData);

            if (! blacklisted)
            {
              SK_LoadImportModule (import);

              if (import.hLibrary != nullptr)
              {
                if (SK_Import_GetShimmedLibrary (import.hLibrary, import.hShim))
                  std::swap (import.hLibrary, import.hShim);

                dll_log.LogEx (false, L"success!\n");
                ++success;

                import.product_desc =
                  SK_GetDLLVersionStr (
                    SK_GetModuleFullName ( import.hLibrary ).c_str ()
                  );

                if (import.role->get_value () == SK_IMPORT_ROLE_PLUGIN)
                {
                  import.hLibrary = SK_InitPlugIn32 (import.hLibrary);

                  if (import.hLibrary == nullptr)
                    --success;
                }
              }

              else
              {
                _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

                import.hLibrary = (HMODULE)-2;
                dll_log.Log (L"[ SpecialK ] [*] Failed: 0x%04X (%s)!",
                               err.WCode (), err.ErrorMessage () );
              }
            }

            else
            {
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

  for (auto& import : imports)
  {
    // Skip libraries that are already loaded
    if (import.hLibrary != nullptr)
      continue;

    bool         blacklisted = false;
    std::wstring blacklist   =
      import.blacklist != nullptr ?
        import.blacklist->get_value_str () :
        L"";

    if (StrStrIW (blacklist.c_str (), SK_GetHostApp ()))
      blacklisted = true;

    if (import.filename != nullptr)
    {
      if (import.when != nullptr)
      {
        if (import.architecture != nullptr)
        {
          if (import.architecture->get_value () == SK_IMPORT_ARCH_WIN32 &&
              import.when->get_value         () == SK_IMPORT_LATE)
          {
            CHeapPtr <wchar_t> file (_wcsdup (import.filename->get_value_str ().c_str ()));

            SK_StripUserNameFromPathW (file);

            dll_log.LogEx (true, L"[ SpecialK ]  * Loading Late Custom Import %s... ",
              file.m_pData);

            if (! blacklisted)
            {
              SK_LoadImportModule (import);

              if (import.hLibrary != nullptr)
              {
                if (SK_Import_GetShimmedLibrary (import.hLibrary, import.hShim))
                  std::swap (import.hLibrary, import.hShim);

                dll_log.LogEx (false, L"success!\n");
                ++success;

                import.product_desc =
                  SK_GetDLLVersionStr (
                    SK_GetModuleFullName ( import.hLibrary ).c_str ()
                  );
              }

              else
              {
                _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

                import.hLibrary = (HMODULE)-2;
                dll_log.LogEx (false, L"failed: 0x%04X (%s)!\n",
                               err.WCode (), err.ErrorMessage () );
              }
            }

            else
            {
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

  for (auto& import : imports)
  {
    // Skip libraries that are already loaded
    if (import.hLibrary != nullptr)
      continue;

    bool         blacklisted = false;
    std::wstring blacklist   =
      import.blacklist != nullptr ?
        import.blacklist->get_value_str () :
        L"";

    if (StrStrIW (blacklist.c_str (), SK_GetHostApp ()))
      blacklisted = true;

    if (import.filename != nullptr)
    {
      if (import.when != nullptr)
      {
        if (import.architecture != nullptr)
        {
          if (import.architecture->get_value () == SK_IMPORT_ARCH_WIN32 &&
              import.when->get_value         () == SK_IMPORT_LAZY)
          {
            CHeapPtr <wchar_t> file (_wcsdup (import.filename->get_value_str ().c_str ()));

            SK_StripUserNameFromPathW (file);

            dll_log.LogEx (true, L"[ SpecialK ]  * Loading Lazy Custom Import %s... ",
                file.m_pData);

            if (! blacklisted)
            {
              SK_LoadImportModule (import);

              if (import.hLibrary != nullptr)
              {
                if (SK_Import_GetShimmedLibrary (import.hLibrary, import.hShim))
                  std::swap (import.hLibrary, import.hShim);

                dll_log.LogEx (false, L"success!\n");
                ++success;

                import.product_desc =
                  SK_GetDLLVersionStr (
                    SK_GetModuleFullName ( import.hLibrary ).c_str ()
                  );
              }

              else
              {
                _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

                import.hLibrary = (HMODULE)-3;
                dll_log.LogEx (false, L"failed: 0x%04X (%s)!\n",
                               err.WCode (), err.ErrorMessage () );
              }
            }

            else
            {
              dll_log.LogEx (false, L"failed: Host App is Blacklisted!\n");
              import.hLibrary = (HMODULE)-3;
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
    // We use the sign-bit for error codes, so... negative
    //   modules need to be ignored.
    //
    //  ** No module should be loaded at an address that high anyway
    //       (in 32-bit code it's kernel-reserved memory)
    //
    if ((intptr_t)imports [i].hLibrary > 0)
    {
      DWORD dwTime = timeGetTime ();

      if (imports [i].role->get_value () == SK_IMPORT_ROLE_PLUGIN)
      {
        auto SKPlugIn_Shutdown =
          reinterpret_cast <SKPlugIn_Shutdown_pfn> (
            GetProcAddress ( imports [i].hLibrary,
                               "SKPlugIn_Shutdown" )
          );

        if (SKPlugIn_Shutdown != nullptr)
          SKPlugIn_Shutdown (nullptr);
      }

      dll_log.Log ( L"[ SpecialK ] Unloading Custom Import %s...",
                    imports [i].filename->get_value_str ().c_str () );

      // The shim will free the plug-in for us
      if ( (imports [i].hShim != nullptr && FreeLibrary_Original (imports [i].hShim) ) ||
                                            FreeLibrary_Original (imports [i].hLibrary) )
      {
        dll_log.LogEx ( false,
                        L"-------------------------[ Free Lib ]                "
                        L"                           success! (%4u ms)\n",
                          timeGetTime ( ) - dwTime );
      }

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