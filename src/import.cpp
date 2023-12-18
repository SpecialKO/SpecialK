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

#include <SpecialK/stdafx.h>

const wchar_t* SK_IMPORT_EARLY         = L"Early";
const wchar_t* SK_IMPORT_PLUGIN        = L"PlugIn";
const wchar_t* SK_IMPORT_LATE          = L"Late";
const wchar_t* SK_IMPORT_LAZY          = L"Lazy";
const wchar_t* SK_IMPORT_PROXY         = L"Proxy";

const wchar_t* SK_IMPORT_ROLE_DXGI     = L"dxgi";
const wchar_t* SK_IMPORT_ROLE_D3D11    = L"d3d11";
const wchar_t* SK_IMPORT_ROLE_D3D9     = L"d3d9";
const wchar_t* SK_IMPORT_ROLE_OPENGL   = L"OpenGL32";
const wchar_t* SK_IMPORT_ROLE_PLUGIN   = L"PlugIn";
const wchar_t* SK_IMPORT_ROLE_3RDPARTY = L"ThirdParty";

const wchar_t* SK_IMPORT_ARCH_X64      = L"x64";
const wchar_t* SK_IMPORT_ARCH_WIN32    = L"Win32";

SK_LazyGlobal <SK_Import_Datastore> imports;

using SKPlugIn_Init_pfn     = BOOL (WINAPI *)(HMODULE hSpecialK);
using SKPlugIn_Shutdown_pfn = BOOL (WINAPI *)(LPVOID  user);

// Fix warnings in dbghelp.h
#pragma warning (disable : 4091)

#define _IMAGEHLP_SOURCE_
#include <dbghelp.h>

#include <diagnostics/debug_utils.h>

int
SK_Import_GetNumberOfPlugIns (void)
{
  int num = 0;

  const std::wstring target_arch =
    SK_RunLHIfBitness ( 64, SK_IMPORT_ARCH_X64,
                            SK_IMPORT_ARCH_WIN32 );

  for (int i = 0; i < SK_MAX_IMPORTS; i++)
  {
    auto& import =
      imports->imports [i];

    if (import.name.empty ())
      continue;

    if (import.architecture->get_value ()._Equal (target_arch))
    {
      if (import.when->get_value ()._Equal (SK_IMPORT_PLUGIN))
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
  auto  SK_SHIM_GetReShadeFilename =
       (SK_SHIM_GetReShadeFilename_pfn)SK_GetProcAddress (
       hModShim,
       "SK_SHIM_GetReShadeFilename"
    );

  if (SK_SHIM_GetReShadeFilename != nullptr)
  {
    hModReal =
      SK_Modules->LoadLibrary (SK_SHIM_GetReShadeFilename ());

    if (hModReal != nullptr)
      return true;
  }


  using SK_SHIM_GetReShade_pfn = HMODULE (__stdcall *)(void);
  auto  SK_SHIM_GetReShade =
       (SK_SHIM_GetReShade_pfn)SK_GetProcAddress (
       hModShim,
       "SK_SHIM_GetReShade"
    );

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

HMODULE
SK_ReShade_LoadDLL (const wchar_t *wszDllFile, const wchar_t *wszMode)
{
  // Allow ReShade 5.2+ to be loaded globally, and rebase its config
  if (StrStrIW (wszDllFile, L"ReShade") != nullptr)
  {
    SK_RunOnce (
    {
      if (0 != _wcsicmp (wszMode, L"Normal"))
      {
        SetEnvironmentVariableW (L"RESHADE_DISABLE_GRAPHICS_HOOK", L"1");
      }

      SetEnvironmentVariableW (L"RESHADE_DISABLE_LOADING_CHECK", L"1");

      config.reshade.has_local_ini =
        PathFileExistsW (L"ReShade.ini");

      // If user already has a local ReShade.ini file, prefer the default ReShade behavior
      if (! config.reshade.has_local_ini)
      {
        std::filesystem::path
          reshade_path (SK_GetConfigPath ());
          reshade_path /= L"ReShade";

        // Otherwise, use SK's per-game config path
        SetEnvironmentVariableW (L"RESHADE_BASE_PATH_OVERRIDE", reshade_path.c_str ());

        std::filesystem::path
          reshade_cfg_path    = reshade_path / L"ReShade.ini";
        std::filesystem::path
          reshade_preset_path = reshade_path / L"ReShadePreset.ini";

        std::wstring shared_base_path =
          SK_FormatStringW (LR"(%ws\Global\ReShade\)", SK_GetInstallPath ());
        std::wstring shared_default_config =
          shared_base_path + L"default_ReShade.ini";
        std::wstring shared_master_config =
          shared_base_path + L"master_ReShade.ini";
        std::wstring shared_default_preset =
          shared_base_path + L"default_ReShadePreset.ini";

        bool bHasDefaultPreset = PathFileExistsW (shared_default_preset.c_str ());
        bool bHasMasterConfig  = PathFileExistsW (shared_master_config.c_str  ());
        bool bHasBasePreset    =
             PathFileExistsW (reshade_preset_path.c_str ());
        bool bHasBaseConfig    =
             PathFileExistsW (reshade_cfg_path.c_str ());
        SK_CreateDirectories (reshade_cfg_path.c_str ());

        if ((! bHasBasePreset) && bHasDefaultPreset)
        {
          CopyFile ( shared_default_preset.c_str (),
                            reshade_preset_path.c_str (), FALSE );
        }

        if ((! bHasBaseConfig) || bHasMasterConfig)
        {
          auto pINI =
            SK_CreateINI (reshade_cfg_path.c_str ());

          if (pINI != nullptr)
          {
            if (! bHasBaseConfig)
            {
              if (PathFileExistsW (shared_default_config.c_str ()))
                pINI->import_file (shared_default_config.c_str ());

              auto& general_section =
                pINI->get_section (L"GENERAL");

              if (! general_section.contains_key  (L"EffectSearchPaths"))
                    general_section.add_key_value (L"EffectSearchPaths", L"");

              if (! general_section.contains_key  (L"TextureSearchPaths"))
                    general_section.add_key_value (L"TextureSearchPaths", L"");

              auto& effect_search_paths =
                general_section.get_value (L"EffectSearchPaths");
              auto& texture_search_paths =
                general_section.get_value (L"TextureSearchPaths");

              effect_search_paths = effect_search_paths.empty () ?
                SK_FormatStringW (LR"(%wsShaders\**,.\reshade-shaders\Shaders\**)",     shared_base_path.c_str ()) :
                SK_FormatStringW (LR"(%wsShaders\**,.\reshade-shaders\Shaders\**,%ws)", shared_base_path.c_str (),
                                                                                     effect_search_paths.c_str ());

              texture_search_paths = texture_search_paths.empty () ?
                SK_FormatStringW (LR"(%wsTextures\**,.\reshade-shaders\Textures\**)",     shared_base_path.c_str ()) :
                SK_FormatStringW (LR"(%wsTextures\**,.\reshade-shaders\Textures\**,%ws)", shared_base_path.c_str (),
                                                                                      texture_search_paths.c_str ());

              pINI->get_section (L"OVERLAY").add_key_value (L"TutorialProgress", L"4");
            }

            if (bHasMasterConfig)
            {
              pINI->import_file (shared_master_config.c_str ());
            }

            pINI->write ();
          }
        }
      }

      if (SK_GetCurrentRenderBackend ().api == SK_RenderAPI::D3D12)
      {
        // Teardown and re-create the D3D12 Render Backend, so that
        //   we initialize various resources needed for ReShade using
        //     hot-injection.
        WriteULongRelease (&_d3d12_rbk->reset_needed, 1);
      }

      return
        SK_LoadLibraryW (wszDllFile);
    });
  }

  return nullptr;
}

void
SK_LoadImportModule (import_s& import)
{
  if (StrStrIW (import.filename->get_value_ref ().c_str (), L"ReShade") != nullptr)
  {
    SK_ReShade_LoadDLL ( import.filename->get_value_ref ().c_str (),
                             import.mode->get_value_str ().c_str () );
  }

  if (config.system.central_repository)
  {
    wchar_t      wszProfilePlugIn [MAX_PATH + 2] = { };
    wcsncpy_s   (wszProfilePlugIn, MAX_PATH, SK_GetConfigPath (), _TRUNCATE);
    PathAppendW (wszProfilePlugIn, import.filename->get_value_str ().c_str ());

    import.hLibrary = SK_Modules->LoadLibrary (
      wszProfilePlugIn
    );
  }

  if (import.hLibrary == nullptr)
  {
    import.hLibrary = SK_Modules->LoadLibrary (
      import.filename->get_value_str ().c_str ()
    );
  }

  // If we still can't find the library, try expanding environment variables
  // in the user-supplied path.
  if (import.hLibrary == nullptr)
  {
          wchar_t wszExpandedPath [MAX_PATH + 2] = { };
    const DWORD     nExpandedPathSize            =
      ExpandEnvironmentStrings (
        import.filename->get_value_str ().c_str (),
                  wszExpandedPath, MAX_PATH );

    if (nExpandedPathSize != 0)
    {
      if (nExpandedPathSize <= MAX_PATH)
      {
        import.hLibrary = SK_Modules->LoadLibrary(
          wszExpandedPath
        );
      }

      else
      {
        // There's no guarantee that the length of the fully-expanded path will fit within
        // the array defined above. If the path is long, we'll need to dynamically allocate
        // memory on the heap for the long path and then call `ExpandEnvironmentStrings` again.
        // The use of `std::make_unique` ensures that this heap-allocated memory will be freed
        // when the `wszLongExpandedPath` object goes out of scope.
        auto wszLongExpandedPath {
          std::make_unique <wchar_t []> (
            static_cast <size_t> (nExpandedPathSize) + 2
          )
        };

        const DWORD nLongPathSize =
          ExpandEnvironmentStrings (
            import.filename->get_value_str ().c_str (),
              wszLongExpandedPath.get (),
                    nExpandedPathSize );

        if (nLongPathSize != 0)
        {
          import.hLibrary =
            SK_Modules->LoadLibrary (
              wszLongExpandedPath.get ()
            );
        }
      }
    }
  }

  if (import.hLibrary != nullptr)
  {
    // Scans all loaded DLLs, and initializes SK as an Add-On to ReShade
    //   for the first ReShade DLL found.
    //
    //  This is done here to catch implicit loads of ReShade by some
    //    .asi-based mods.
    //
    SK_ReShadeAddOn_Init ();
  }
};

HMODULE
SK_InitPlugIn64 (HMODULE hLibrary)
{
  dll_log->Log ( L"[ SpecialK ] [*] Initializing Plug-In: %s...",
                   SK_GetModuleName (hLibrary).c_str () );

  auto SKPlugIn_Init =
    reinterpret_cast <SKPlugIn_Init_pfn> (
      SK_GetProcAddress (
        hLibrary,
          "SKPlugIn_Init"
      )
    );

  if (SKPlugIn_Init != nullptr)
  {
    if (SKPlugIn_Init (SK_GetDLL ()))
    {
      dll_log->Log ( L"[ SpecialK ] [*] Plug-In Init Success (%s)!",
                       SK_GetModuleName (hLibrary).c_str () );
    }

    else
    {
      dll_log->Log (L"[ SpecialK ] [*] Plug-In Init Failed (Plug-In returned false)!");

      SK_FreeLibrary (hLibrary);
      hLibrary = nullptr;
    }
  }

  else
  {
    dll_log->Log (L"[ SpecialK ] [*] Plug-In Init Failed (Lacks SpecialK PlugIn Entry Point)!");

    SK_FreeLibrary (hLibrary);
    hLibrary = nullptr;
  }

  return hLibrary;
}

void
SK_LoadEarlyImports64 (void)
{
  int success = 0;

  for (auto& import : imports->imports)
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
          if (import.architecture->get_value ()._Equal (SK_IMPORT_ARCH_X64) &&
              import.when->get_value         ()._Equal (SK_IMPORT_EARLY))
          {
            CHeapPtr <wchar_t> file (
              _wcsdup (import.filename->get_value_str ().c_str ())
            );

            SK_StripUserNameFromPathW (file);

            dll_log->LogEx (true, L"[ SpecialK ]  * Loading Early Custom Import %s... ",
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

                dll_log->LogEx (false, L"success!\n");
                ++success;

                if (import.role->get_value ()._Equal (SK_IMPORT_ROLE_PLUGIN)) {
                  import.hLibrary = SK_InitPlugIn64 (import.hLibrary);

                  if (import.hLibrary == nullptr)
                    --success;
                }
              }

              else
              {
                _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

                import.hLibrary = (HMODULE)-2;
                dll_log->LogEx (false, L"failed: 0x%04X (%s)!\n",
                                err.WCode (), err.ErrorMessage () );
              }
            }

            else
            {
              dll_log->LogEx (false, L"failed: Host App is Blacklisted!\n");
            }
          }
        }
      }
    }
  }
}

void
SK_LoadPlugIns64 (void)
{
  int success = 0;

  for (auto& import : imports->imports)
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
          if (import.architecture->get_value ()._Equal (SK_IMPORT_ARCH_X64) &&
              import.when->get_value         ()._Equal (SK_IMPORT_PLUGIN))
          {
            CHeapPtr <wchar_t> file (
              _wcsdup (import.filename->get_value_str ().c_str ())
            );

            SK_StripUserNameFromPathW (file);

            dll_log->LogEx (true, L"[ SpecialK ]  * Loading Special K Plug-In %s... ",
              file.m_pData);

            if (! blacklisted)
            {
              SK_LoadImportModule (import);

              if (import.hLibrary != nullptr)
              {
                if (SK_Import_GetShimmedLibrary (import.hLibrary, import.hShim))
                  std::swap (import.hLibrary, import.hShim);

                dll_log->LogEx (false, L"success!\n");
                ++success;

                import.product_desc =
                  SK_GetDLLVersionStr (
                    SK_GetModuleFullName ( import.hLibrary ).c_str ()
                  );

                if (import.role->get_value ()._Equal (SK_IMPORT_ROLE_PLUGIN))
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
                dll_log->Log (L"[ SpecialK ] [*] Failed: 0x%04X (%s)!",
                                err.WCode (), err.ErrorMessage () );
              }
            }

            else
            {
              dll_log->Log (L"[ SpecialK ] [*] Failed: Host App is Blacklisted!");
            }
          }
        }
      }
    }
  }
}

void
SK_LoadLateImports64 (void)
{
  int success = 0;

  for (auto& import : imports->imports)
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
          if (import.architecture->get_value ()._Equal (SK_IMPORT_ARCH_X64) &&
              import.when->get_value         ()._Equal (SK_IMPORT_LATE))
          {
            CHeapPtr <wchar_t> file (
              _wcsdup (import.filename->get_value_str ().c_str ())
            );

            SK_StripUserNameFromPathW (file);

            dll_log->LogEx (true, L"[ SpecialK ]  * Loading Late Custom Import %s... ",
              file.m_pData);

            if (! blacklisted)
            {
              SK_LoadImportModule (import);

              if (import.hLibrary != nullptr)
              {
                if (SK_Import_GetShimmedLibrary (import.hLibrary, import.hShim))
                  std::swap (import.hLibrary, import.hShim);

                dll_log->LogEx (false, L"success!\n");
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
                dll_log->LogEx (false, L"failed: 0x%04X (%s)!\n",
                               err.WCode (), err.ErrorMessage () );
              }
            }

            else
            {
              dll_log->LogEx (false, L"failed: Host App is Blacklisted!\n");
            }
          }
        }
      }
    }
  }
}

void
SK_LoadLazyImports64 (void)
{
  int success = 0;

  for (auto& import : imports->imports)
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
          if (import.architecture->get_value ()._Equal (SK_IMPORT_ARCH_X64) &&
              import.when->get_value         ()._Equal (SK_IMPORT_LAZY))
          {
            CHeapPtr <wchar_t> file (
              _wcsdup (import.filename->get_value_str ().c_str ())
            );

            SK_StripUserNameFromPathW (file);

            dll_log->LogEx (true, L"[ SpecialK ]  * Loading Lazy Custom Import %s... ",
                file.m_pData);

            if (! blacklisted)
            {
              SK_LoadImportModule (import);

              if (import.hLibrary != nullptr)
              {
                if (SK_Import_GetShimmedLibrary (import.hLibrary, import.hShim))
                  std::swap (import.hLibrary, import.hShim);

                dll_log->LogEx (false, L"success!\n");
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
                dll_log->LogEx (false, L"failed: 0x%04X (%s)!\n",
                                err.WCode (), err.ErrorMessage () );
              }
            }

            else
            {
              dll_log->LogEx (false, L"failed: Host App is Blacklisted!\n");
            }
          }
        }
      }
    }
  }
}


HMODULE
SK_InitPlugIn32 (HMODULE hLibrary)
{
  dll_log->Log ( L"[ SpecialK ] [*] Initializing Plug-In: %s...",
                   SK_GetModuleName (hLibrary).c_str () );

  auto SKPlugIn_Init =
    reinterpret_cast <SKPlugIn_Init_pfn> (
      SK_GetProcAddress (
        hLibrary,
          "SKPlugIn_Init"
      )
    );

  if (SKPlugIn_Init != nullptr)
  {
    if (SKPlugIn_Init (SK_GetDLL ()))
    {
      dll_log->Log ( L"[ SpecialK ] [*] Plug-In Init Success (%s)!",
                       SK_GetModuleName (hLibrary).c_str () );
    }

    else
    {
      dll_log->Log (L"[ SpecialK ] [*] Plug-In Init Failed (Plug-In returned false)!");

      SK_FreeLibrary (hLibrary);
      hLibrary = nullptr;
    }
  }

  else
  {
    dll_log->Log (L"[ SpecialK ] [*] Plug-In Init Failed (Lacks SpecialK PlugIn Entry Point)!");

    SK_FreeLibrary (hLibrary);
    hLibrary = nullptr;
  }

  return hLibrary;
}

void
SK_LoadEarlyImports32 (void)
{
  int success = 0;

  for (auto& import : imports->imports)
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
          if (import.architecture->get_value ()._Equal (SK_IMPORT_ARCH_WIN32) &&
              import.when->get_value         ()._Equal (SK_IMPORT_EARLY))
          {
            CHeapPtr <wchar_t> file (
              _wcsdup (import.filename->get_value_str ().c_str ())
            );

            SK_StripUserNameFromPathW (file);

            dll_log->LogEx (true, L"[ SpecialK ]  * Loading Early Custom Import %s... ",
              file.m_pData);

            if (! blacklisted)
            {
              SK_LoadImportModule (import);

              if (import.hLibrary != nullptr)
              {
                if (SK_Import_GetShimmedLibrary (import.hLibrary, import.hShim))
                  std::swap (import.hLibrary, import.hShim);

                dll_log->LogEx (false, L"success!\n");
                ++success;

                import.product_desc =
                  SK_GetDLLVersionStr (
                    SK_GetModuleFullName ( import.hLibrary ).c_str ()
                  );

                if (import.role->get_value ()._Equal (SK_IMPORT_ROLE_PLUGIN))
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
                dll_log->LogEx (false, L"failed: 0x%04X (%s)!\n",
                                err.WCode (), err.ErrorMessage () );
              }
            }

            else
            {
              dll_log->LogEx (false, L"failed: Host App is Blacklisted!\n");
            }
          }
        }
      }
    }
  }
}

void
SK_LoadPlugIns32 (void)
{
  int success = 0;

  for (auto& import : imports->imports)
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
          if (import.architecture->get_value ()._Equal (SK_IMPORT_ARCH_WIN32) &&
              import.when->get_value         ()._Equal (SK_IMPORT_PLUGIN))
          {
            CHeapPtr <wchar_t> file (
              _wcsdup (import.filename->get_value_str ().c_str ())
            );

            SK_StripUserNameFromPathW (file);

            dll_log->LogEx (true, L"[ SpecialK ]  * Loading Special K Plug-In %s... ",
              file.m_pData);

            if (! blacklisted)
            {
              SK_LoadImportModule (import);

              if (import.hLibrary != nullptr)
              {
                if (SK_Import_GetShimmedLibrary (import.hLibrary, import.hShim))
                  std::swap (import.hLibrary, import.hShim);

                dll_log->LogEx (false, L"success!\n");
                ++success;

                import.product_desc =
                  SK_GetDLLVersionStr (
                    SK_GetModuleFullName ( import.hLibrary ).c_str ()
                  );

                if (import.role->get_value ()._Equal (SK_IMPORT_ROLE_PLUGIN))
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
                dll_log->Log (L"[ SpecialK ] [*] Failed: 0x%04X (%s)!",
                                err.WCode (), err.ErrorMessage () );
              }
            }

            else
            {
              dll_log->Log (L"[ SpecialK ] [*] Failed: Host App is Blacklisted!");
            }
          }
        }
      }
    }
  }
}

void
SK_LoadLateImports32 (void)
{
  int success = 0;

  for (auto& import : imports->imports)
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
          if (import.architecture->get_value ()._Equal (SK_IMPORT_ARCH_WIN32) &&
              import.when->get_value         ()._Equal (SK_IMPORT_LATE))
          {
            CHeapPtr <wchar_t> file (
              _wcsdup (import.filename->get_value_str ().c_str ())
            );

            SK_StripUserNameFromPathW (file);

            dll_log->LogEx (true, L"[ SpecialK ]  * Loading Late Custom Import %s... ",
              file.m_pData);

            if (! blacklisted)
            {
              SK_LoadImportModule (import);

              if (import.hLibrary != nullptr)
              {
                if (SK_Import_GetShimmedLibrary (import.hLibrary, import.hShim))
                  std::swap (import.hLibrary, import.hShim);

                dll_log->LogEx (false, L"success!\n");
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
                dll_log->LogEx (false, L"failed: 0x%04X (%s)!\n",
                                err.WCode (), err.ErrorMessage () );
              }
            }

            else
            {
              dll_log->LogEx (false, L"failed: Host App is Blacklisted!\n");
            }
          }
        }
      }
    }
  }
}

void
SK_LoadLazyImports32 (void)
{
  int success = 0;

  for (auto& import : imports->imports)
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
          if (import.architecture->get_value ()._Equal (SK_IMPORT_ARCH_WIN32) &&
              import.when->get_value         ()._Equal (SK_IMPORT_LAZY))
          {
            CHeapPtr <wchar_t> file (
              _wcsdup (import.filename->get_value_str ().c_str ())
            );

            SK_StripUserNameFromPathW (file);

            dll_log->LogEx (true, L"[ SpecialK ]  * Loading Lazy Custom Import %s... ",
                file.m_pData);

            if (! blacklisted)
            {
              SK_LoadImportModule (import);

              if (import.hLibrary != nullptr)
              {
                if (SK_Import_GetShimmedLibrary (import.hLibrary, import.hShim))
                  std::swap (import.hLibrary, import.hShim);

                dll_log->LogEx (false, L"success!\n");
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
                dll_log->LogEx (false, L"failed: 0x%04X (%s)!\n",
                                err.WCode (), err.ErrorMessage () );
              }
            }

            else
            {
              dll_log->LogEx (false, L"failed: Host App is Blacklisted!\n");
              import.hLibrary = (HMODULE)-3;
            }
          }
        }
      }
    }
  }
}

void
SK_LogLastErr (void)
{
  _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

  dll_log->LogEx ( false,
                     L"failed: 0x%04X (%s)!\n",
                       err.WCode (),
                         err.ErrorMessage ()
                 );
}

bool
_IsRoleSame ( const std::wstring& role,
              const std::wstring& test )
{
  return
    role._Equal (test);
}

void
SK_UnloadImports (void)
{
  auto orig_se =
  SK_SEH_ApplyTranslator (
    SK_FilteringStructuredExceptionTranslator (
      EXCEPTION_ACCESS_VIOLATION
    )
  );
  try {
    // Unload in reverse order, because that's safer :)
    for (int i = SK_MAX_IMPORTS - 1; i >= 0; i--)
    {
      auto& import =
        imports->imports [i];

      // We use the sign-bit for error codes, so... negative
      //   modules need to be ignored.
      //
      //  ** No module should be loaded at an address that high anyway
      //       (in 32-bit code it's kernel-reserved memory)
      //
      if ((intptr_t)import.hLibrary > 0)
      {
        DWORD dwTime =
          SK_timeGetTime ();

        if (_IsRoleSame (import.role->get_value_ref (), SK_IMPORT_ROLE_PLUGIN))
        {
          auto SKPlugIn_Shutdown =
            reinterpret_cast <SKPlugIn_Shutdown_pfn> (
              SK_GetProcAddress ( import.hLibrary,
                                 "SKPlugIn_Shutdown" )
            );

          if (SKPlugIn_Shutdown != nullptr)
              SKPlugIn_Shutdown   (nullptr);
        }

        ///dll_log.Log ( L"[ SpecialK ] Unloading Custom Import %s...",
        ///              import.filename->get_value_str ().c_str () );

        // The shim will free the plug-in for us
        if ( (import.hShim != nullptr && SK_FreeLibrary (import.hShim) ) ||
                                         SK_FreeLibrary (import.hLibrary) )
        {
          dll_log->LogEx ( false,
                           L"-------------------------[ Free Lib ]                "
                           L"                           success! (%4u ms)\n",
                             SK_timeGetTime ( ) - dwTime );
        }

        else
        {
          SK_LogLastErr ();
        }
      }
    }
  }

  catch (const SK_SEH_IgnoredException&)
  { }
  SK_SEH_RemoveTranslator (orig_se);
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





static SK_LazyGlobal <SK_Thread_HybridSpinlock> static_loader;
static                SK_Thread_HybridSpinlock* loader_lock     = nullptr;
static volatile LONG                            _LoaderLockInit = 0;

SK_Thread_HybridSpinlock*
SK_DLL_LoaderLockGuard (void) noexcept
{
  if (0 == InterlockedCompareExchange (&_LoaderLockInit, 1, 0))
  {
    if (loader_lock == nullptr)
        loader_lock  = static_loader.getPtr ();

    InterlockedIncrement (&_LoaderLockInit);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&_LoaderLockInit, 2);

  return
    loader_lock;
}