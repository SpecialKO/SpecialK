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
#include <SpecialK/resource.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L" SteamAPI "

// We're not going to use DLL Import - we will load these function pointers
//  by hand.
#define STEAM_API_NODLL
#include <SpecialK/steam_api.h>

using            callback_set = std::multiset <class CCallbackBase*>;
using threadsafe_callback_map = Concurrency::concurrent_unordered_map
                                                      <LPVOID, bool>;

SK_LazyGlobal <threadsafe_callback_map> UserStatsReceived_callbacks;
SK_LazyGlobal <threadsafe_callback_map> ScreenshotRequested_callbacks;
SK_LazyGlobal <threadsafe_callback_map> UserStatsStored_callbacks;
SK_LazyGlobal <threadsafe_callback_map> UserAchievementStored_callbacks;
SK_LazyGlobal <threadsafe_callback_map> UserStatsUnloaded_callbacks;
SK_LazyGlobal <callback_set>            overlay_activation_callbacks;
SK_LazyGlobal <SK_SteamAPIContext>      pSteamCtx;

volatile LONGLONG SK_SteamAPI_CallbackRunCount    =  0LL;
volatile LONG     SK_SteamAPI_ManagersInitialized =  0L;
volatile LONG     SK_SteamAPI_CallbackRateLimit   = -1L;

volatile LONG   __SK_Steam_init = FALSE;
volatile LONG   __SteamAPI_hook = FALSE;

// To spoof Overlay Activation (pause the game)

void SK_Steam_UpdateGlobalAchievements (void);


using   SteamClient_pfn =
       ISteamClient* (S_CALLTYPE *)(void);
extern  SteamClient_pfn
        SteamClient_Original;                                              extern ISteamClient* S_CALLTYPE
        SteamClient_Detour (void);

using   SteamUser_pfn =
       ISteamUser* (S_CALLTYPE *)(void);
extern  SteamUser_pfn
        SteamUser_Original;                                                extern ISteamUser* S_CALLTYPE
        SteamUser_Detour (void);

using  SteamRemoteStorage_pfn =
      ISteamRemoteStorage* (S_CALLTYPE *)(void);
extern SteamRemoteStorage_pfn
       SteamRemoteStorage_Original;                                        extern ISteamRemoteStorage* S_CALLTYPE
       SteamRemoteStorage_Detour (void);

using   SteamUGC_pfn =
       ISteamUGC* (S_CALLTYPE *)(void);
extern  SteamUGC_pfn
        SteamUGC_Original;                                                 extern ISteamUGC* S_CALLTYPE
        SteamUGC_Detour (void);

using   SteamUtils_pfn =
       ISteamUtils* (S_CALLTYPE *)(void);
extern  SteamUtils_pfn
        SteamUtils_Original;                                               extern ISteamUtils* S_CALLTYPE
        SteamUtils_Detour (void);


typedef ISteamUserStats* (S_CALLTYPE* SteamUserStats_pfn)(void);
                                      SteamUserStats_pfn
                                      SteamUserStats_Original = nullptr;
extern  ISteamUserStats*  S_CALLTYPE  SteamUserStats_Detour (void);


using  SteamInternal_CreateInterface_pfn =
                        void* (S_CALLTYPE *)(const char *ver);
extern SteamInternal_CreateInterface_pfn
       SteamInternal_CreateInterface_Original;                             extern void* S_CALLTYPE
       SteamInternal_CreateInterface_Detour (const char *ver);


using  SteamAPI_ISteamClient_GetISteamUser_pfn =
                                ISteamUser* (S_CALLTYPE *)
        (       ISteamClient *This,
                HSteamUser    hSteamUser,
                HSteamPipe    hSteamPipe,
          const char         *pchVersion );
extern SteamAPI_ISteamClient_GetISteamUser_pfn
       SteamAPI_ISteamClient_GetISteamUser_Original;                       extern ISteamUser* S_CALLTYPE
       SteamAPI_ISteamClient_GetISteamUser_Detour
        (       ISteamClient *This,
                HSteamUser    hSteamUser,
                HSteamPipe    hSteamPipe,
          const char         *pchVersion );

using  SteamAPI_ISteamClient_GetISteamUtils_pfn =
                                ISteamUtils* (S_CALLTYPE *)
        (       ISteamClient *This,
                HSteamPipe    hSteamPipe,
          const char         *pchVersion );
extern SteamAPI_ISteamClient_GetISteamUtils_pfn
       SteamAPI_ISteamClient_GetISteamUtils_Original;                      extern ISteamUtils* S_CALLTYPE
       SteamAPI_ISteamClient_GetISteamUtils_Detour
        (       ISteamClient *This,
                HSteamPipe    hSteamPipe,
          const char         *pchVersion );

using  SteamAPI_ISteamClient_GetISteamController_pfn =
                                ISteamController* (S_CALLTYPE *)
        (       ISteamClient *This,
                HSteamUser    hSteamUser,
                HSteamPipe    hSteamPipe,
          const char         *pchVersion );
extern SteamAPI_ISteamClient_GetISteamController_pfn
       SteamAPI_ISteamClient_GetISteamController_Original;                 extern ISteamController* S_CALLTYPE
       SteamAPI_ISteamClient_GetISteamController_Detour
        (       ISteamClient *This,
                HSteamUser    hSteamUser,
                HSteamPipe    hSteamPipe,
          const char         *pchVersion );

using  SteamAPI_ISteamClient_GetISteamRemoteStorage_pfn =
                                ISteamRemoteStorage* (S_CALLTYPE *)
        (       ISteamClient *This,
                HSteamUser    hSteamuser,
                HSteamPipe    hSteamPipe,
          const char         *pchVersion );
extern SteamAPI_ISteamClient_GetISteamRemoteStorage_pfn
       SteamAPI_ISteamClient_GetISteamRemoteStorage_Original;              extern ISteamRemoteStorage* S_CALLTYPE
       SteamAPI_ISteamClient_GetISteamRemoteStorage_Detour
        (       ISteamClient *This,
                HSteamUser    hSteamuser,
                HSteamPipe    hSteamPipe,
          const char         *pchVersion );


using  SteamAPI_ISteamClient_GetISteamUGC_pfn =
                                ISteamUGC* (S_CALLTYPE *)
        (       ISteamClient *This,
                HSteamUser    hSteamuser,
                HSteamPipe    hSteamPipe,
          const char         *pchVersion );
extern SteamAPI_ISteamClient_GetISteamUGC_pfn
       SteamAPI_ISteamClient_GetISteamUGC_Original;                        extern ISteamUGC* S_CALLTYPE
       SteamAPI_ISteamClient_GetISteamUGC_Detour
        (       ISteamClient *This,
                HSteamUser    hSteamuser,
                HSteamPipe    hSteamPipe,
          const char         *pchVersion );

extern "C" {
  SteamAPI_Init_pfn
  SteamAPI_Init                        = nullptr,
  SteamAPI_Init_Original               = nullptr;

  SteamAPI_InitSafe_pfn
  SteamAPI_InitSafe                    = nullptr,
  SteamAPI_InitSafe_Original           = nullptr;

  SteamAPI_RunCallbacks_pfn
  SteamAPI_RunCallbacks                = nullptr,
  SteamAPI_RunCallbacks_Original       = nullptr;

  SteamAPI_RegisterCallback_pfn
  SteamAPI_RegisterCallback            = nullptr,
  SteamAPI_RegisterCallback_Original   = nullptr;

  SteamAPI_UnregisterCallback_pfn
  SteamAPI_UnregisterCallback          = nullptr,
  SteamAPI_UnregisterCallback_Original = nullptr;

  SteamAPI_RegisterCallResult_pfn
  SteamAPI_RegisterCallResult          = nullptr;
  SteamAPI_UnregisterCallResult_pfn
  SteamAPI_UnregisterCallResult        = nullptr;

  SteamAPI_RestartAppIfNecessary_pfn
  SteamAPI_RestartAppIfNecessary       = nullptr;
  SteamAPI_IsSteamRunning_pfn
  SteamAPI_IsSteamRunning              = nullptr;

  SteamAPI_GetHSteamUser_pfn
  SteamAPI_GetHSteamUser               = nullptr;
  SteamAPI_GetHSteamPipe_pfn
  SteamAPI_GetHSteamPipe               = nullptr;

  SteamClient_pfn
  SteamClient                          = nullptr;

  SteamAPI_Shutdown_pfn
  SteamAPI_Shutdown                    = nullptr,
  SteamAPI_Shutdown_Original           = nullptr;

  SteamAPI_GetSteamInstallPath_pfn
  SteamAPI_GetSteamInstallPath         = nullptr;
};

         LPVOID pfnSteamInternal_CreateInterface = nullptr;
volatile LONG __SK_Steam_Downloading             = 0;

auto constexpr _ConstructPath =
[&](auto&& path_base, const wchar_t* path_end)
{
  std::array <wchar_t, MAX_PATH + 1>
    path { };
  SK_PathCombineW (path.data (), path_base.data (), path_end);
  return           path;
};


// Returns the value of files as it was when called
unsigned int
SK_Steam_RecursiveFileScrub (
        std::wstring   directory,    unsigned int &files,
        LARGE_INTEGER &liSize, const wchar_t      *wszPattern,
  const wchar_t       *wszAntiPattern,     // Exact match only
        bool           erase = false )
{
  unsigned int  const files_start = files;
  WIN32_FIND_DATA     fd          = {   };
  HANDLE              hFind       =
    FindFirstFileW (_ConstructPath (directory, wszPattern).data (), &fd);

  if (hFind != INVALID_HANDLE_VALUE)
  {
    do
    {
      // Leave installscript.vdf behind
      //
      if (            wszAntiPattern != nullptr &&
        (! _wcsnicmp (wszAntiPattern, fd.cFileName, MAX_PATH)) )
      {
        continue;
      }

      if (          (fd.dwFileAttributes  & FILE_ATTRIBUTE_DIRECTORY)     &&
        (  _wcsnicmp (fd.cFileName, L"." ,                MAX_PATH) != 0) &&
          (_wcsnicmp (fd.cFileName, L"..",                MAX_PATH) != 0) &&
          (_wcsnicmp (fd.cFileName, L"Steamworks Shared", MAX_PATH) != 0) )
      {
        // Detect empty directories
        //
        unsigned int const before =
          SK_Steam_RecursiveFileScrub (
            _ConstructPath (directory, fd.cFileName).data (),
                                files,
                                liSize,
                                wszPattern,
                                wszAntiPattern,
                                erase );

        // Directory is empty; remove it if the user wants
        //
        if (files == before && erase)
        {
          RemoveDirectoryW (
            _ConstructPath ( directory, fd.cFileName ).data ()
          );
        }
      }

      else if ( (fd.dwFileAttributes ^ FILE_ATTRIBUTE_DIRECTORY) &
                                       FILE_ATTRIBUTE_DIRECTORY    )
      {
        if (! erase)
        {
          ++files;
          liSize.QuadPart +=
            LARGE_INTEGER {          fd.nFileSizeLow,
            gsl::narrow_cast <LONG> (fd.nFileSizeHigh) }.QuadPart;
        }

        else
        {
          DeleteFileW (
            _ConstructPath ( directory, fd.cFileName ).data ()
          );
        }
      }
    } while (FindNextFile (hFind, &fd));

    FindClose (hFind);
  }

  return files_start;
}

std::wstring
_SK_RecursiveFileSearch ( const wchar_t* wszDir,
                          const wchar_t* wszFile )
{
  std::wstring found;

  wchar_t   wszPath [MAX_PATH + 2] = { };
  swprintf (wszPath, LR"(%s\*)", wszDir);

  WIN32_FIND_DATA fd          = {   };
  HANDLE          hFind       =
    FindFirstFileW ( wszPath, &fd);

  if (hFind == INVALID_HANDLE_VALUE) { return found; }

  do
  {
    if ( wcsncmp (fd.cFileName, L".",  MAX_PATH) == 0 ||
         wcsncmp (fd.cFileName, L"..", MAX_PATH) == 0 )
    {
      continue;
    }

    if (! _wcsnicmp (fd.cFileName, wszFile, MAX_PATH))
    {
      found = wszDir;
      found += L"\\"; found += wszFile;

      break;
    }

    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      wchar_t   wszDescend [MAX_PATH + 2] = { };
      swprintf (wszDescend, LR"(%s\%s)", wszDir, fd.cFileName);

      found =
        _SK_RecursiveFileSearch (wszDescend, wszFile);
    }

  } while ((found.empty ()) && FindNextFile (hFind, &fd));

  FindClose (hFind);

  return found;
}

using steam_library_t = wchar_t* [MAX_PATH + 2];

int
SK_Steam_GetLibraries (steam_library_t** ppLibraries = nullptr);

std::wstring
SK_Steam_GetApplicationManifestPath (AppId_t appid)
{
  if (appid == 0)
    appid = SK::SteamAPI::AppID ();

  steam_library_t* steam_lib_paths = nullptr;
  const int        steam_libs      = SK_Steam_GetLibraries (&steam_lib_paths);

  if (! steam_lib_paths)
    return L"";

  if (steam_libs != 0)
  {
    for (int i = 0; i < steam_libs; i++)
    {
      wchar_t wszManifest [MAX_PATH + 2] = { };

      swprintf ( wszManifest,
                   LR"(%s\steamapps\appmanifest_%u.acf)",
               (wchar_t *)steam_lib_paths [i],
                            appid );

      SK_AutoHandle hManifest (
        CreateFileW ( wszManifest,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                          nullptr,        OPEN_EXISTING,
                            GetFileAttributesW (wszManifest),
                              nullptr
                    )
      );

      if (hManifest != INVALID_HANDLE_VALUE)
        return wszManifest;
    }
  }

  return L"";
}

std::string
SK_GetManifestContentsForAppID (AppId_t appid)
{
  static std::string manifest;

  if (! manifest.empty ())
    return manifest;

  std::wstring wszManifest =
    SK_Steam_GetApplicationManifestPath (appid);

  if (wszManifest.empty ())
    return manifest;

  SK_AutoHandle hManifest (
    CreateFileW ( wszManifest.c_str (),
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                      nullptr,        OPEN_EXISTING,
                        GetFileAttributesW (wszManifest.c_str ()),
                          nullptr
                )
  );

  if (hManifest != INVALID_HANDLE_VALUE)
  {
    DWORD dwSizeHigh = 0,
          dwRead     = 0,
          dwSize     =
     GetFileSize (hManifest, &dwSizeHigh);

    auto szManifestData =
        std::make_unique <char []> (
          static_cast <std::size_t> (dwSize) +
          static_cast <std::size_t> (1)
                                   );
    auto manifest_data =
      szManifestData.get ();

    if (! manifest_data)
      return "";

    const bool bRead =
      ReadFile ( hManifest,
                   manifest_data,
                     dwSize,
                    &dwRead,
                       nullptr );

    if (bRead && dwRead)
    {
      manifest =
        std::move (manifest_data);

      return
        manifest;
    }
  }

  return manifest;
}

std::wstring
SK_Steam_FindInstallPath (uint32_t appid)
{
  std::string manifest =
    SK_GetManifestContentsForAppID (appid);

  std::string install_path_utf8 =
    SK_Steam_KeyValues::getValue (
      manifest, { "AppState" }, "installdir"
    );

  if (! install_path_utf8.empty ())
  {
    std::wstring manifest_path (
      SK_Steam_GetApplicationManifestPath (appid)
    );

    // This is a relative path, we need to build a full path
    std::wstring install_path =
      std::move (
        SK_UTF8ToWideChar (install_path_utf8)
      );

    if (manifest_path.length () < MAX_PATH)
    {
      wchar_t     wszManifestDir [MAX_PATH + 2] = { };
      wcsncpy_s ( wszManifestDir,
                     manifest_path.length (),
                     manifest_path.c_str  (), _TRUNCATE );

      // Path builder
      if
      ( PathRemoveFileSpecW
                        ( wszManifestDir ) )
      { PathAppendW     ( wszManifestDir, L"common");
        PathAppendW     ( wszManifestDir, install_path.c_str ());
        SK_StripTrailingSlashesW
                        ( wszManifestDir );
        SK_FixSlashesW  ( wszManifestDir );

        SK_ReleaseAssert (
          PathFileExistsW ( wszManifestDir )
                         );

        return wszManifestDir;
      }
    }

    // Well damn, just give the relative path
    return install_path;
  }

  return L"";
}


//
// Go-go-gadget garbage code compactor!
//
bool
SK_Steam_GetDLLPath ( wchar_t* wszDestBuf,
                      size_t   max_size = MAX_PATH )
{
  if (max_size == 0)
    return false;

  const wchar_t* wszSteamLib =
    SK_RunLHIfBitness ( 64, L"steam_api64.dll",
                            L"steam_api.dll"    );

  static wchar_t
    dll_file [MAX_PATH + 2] = { };

  if (*dll_file != L'\0')
  {
    // Already have a working DLL
    if (SK_Modules->LoadLibrary (dll_file))
    {
      wcsncpy_s (
        wszDestBuf,
          std::min (max_size, (size_t)MAX_PATH),
                            dll_file, _TRUNCATE
      );

      return true;
    }
  }

  std::wstring& cfg_path =
    config.steam.dll_path;

  if (                               (!cfg_path.empty ()) &&
      GetFileAttributesW             ( cfg_path.c_str ()) != INVALID_FILE_ATTRIBUTES )
  { if (SK_Modules->LoadLibrary       (cfg_path.c_str ()))
    { wcsncpy_s (wszDestBuf, max_size, cfg_path.c_str (), _TRUNCATE);
      wcsncpy_s (dll_file,   max_size, cfg_path.c_str (), _TRUNCATE);
      return true;
    }
  }

  cfg_path.resize (0);

  wchar_t      wszExecutablePath [MAX_PATH + 2] = { };
  wcsncpy_s   (wszExecutablePath, MAX_PATH, SK_GetHostPath (), _TRUNCATE);
  PathAppendW (wszExecutablePath, wszSteamLib);

  // The simplest case:  * DLL is in the same directory as the executable.
  //
  if (SK_Modules->LoadLibrary         (wszExecutablePath))
  {   wcsncpy_s (dll_file,   max_size, wszExecutablePath, _TRUNCATE);
      wcsncpy_s (wszDestBuf, max_size, dll_file,          _TRUNCATE);
      cfg_path.assign                 (dll_file);

    return true;
  }

  wcsncpy_s (wszExecutablePath, MAX_PATH, SK_GetHostPath (), _TRUNCATE);
  lstrcatW  (wszExecutablePath, L".");

  // Moonwalk to find the game's absolute root directory, rather than
  //   whatever subdirectory the executable might be in.
  while (PathRemoveFileSpecW           (wszExecutablePath))
  { if  (StrStrIW                      (wszExecutablePath, LR"(SteamApps\common\)"))
  {      wcsncpy_s (dll_file, max_size, wszExecutablePath, _TRUNCATE);
                             lstrcatW  (wszExecutablePath, L".");
  } else break;
  }

  // Try the DLL we went back to the root of the install dir-tree for first.
  if (                         *dll_file != L'\0')
  { wcsncpy_s (wszExecutablePath, MAX_PATH, dll_file, _TRUNCATE); }

  // Then get the base install directory from Steam (if we can).
  else if (config.steam.appid != 0)
  { wcsncpy_s ( wszExecutablePath, MAX_PATH,
               SK_Steam_FindInstallPath (config.steam.appid).c_str (),
               _TRUNCATE );
  }

  // Finally, give up and hope the DLL is relative to the executable.
  else
    wcsncpy_s (wszExecutablePath, MAX_PATH, SK_GetHostPath (), _TRUNCATE);

  static wchar_t
    wszFullSteamDllPath [MAX_PATH + 2] = { };

  std::wstring recursed (
    _SK_RecursiveFileSearch ( wszExecutablePath,
                              wszSteamLib        )
  );

  wcsncpy_s (dll_file,             MAX_PATH, recursed.c_str (), _TRUNCATE);
  wcsncpy_s ( wszFullSteamDllPath, MAX_PATH, dll_file,          _TRUNCATE);


  if (SK_Modules->LoadLibrary             (wszFullSteamDllPath))
  { wcsncpy_s       (wszDestBuf, max_size, wszFullSteamDllPath, _TRUNCATE);
    cfg_path.assign (wszDestBuf);
    return true;
  }

  return false;
}


const wchar_t*
SK_Steam_GetDLLPath (void)
{
  static wchar_t
    dll_file [MAX_PATH + 2] = { };

  if (*dll_file == L'\0')
  { if (! SK_Steam_GetDLLPath (dll_file)) {
      *dll_file  = L'?'; }
  }

  return dll_file;
}

BOOL
SK_Steam_PreHookCore (const wchar_t* wszTry)
{
  static volatile LONG init = FALSE;

  if (InterlockedCompareExchange (&init, TRUE, FALSE))
    return TRUE;

  const wchar_t*
    wszSteamLib = wszTry;

  if (wszTry == nullptr)
  {
    static wchar_t       resolved_dll [MAX_PATH + 2];
    SK_Steam_GetDLLPath (resolved_dll, MAX_PATH);
    wszSteamLib     =    resolved_dll;
  }

  const std::wstring& qualified_lib =
    SK_Steam_GetDLLPath ();

  if (! SK_Modules->LoadLibrary (wszSteamLib))
  {
    if (! qualified_lib.empty ())
    {
      SK_Modules->LoadLibrary (qualified_lib.c_str ());
      wszSteamLib =            qualified_lib.data  ();
    }
  }

  if (SK_GetProcAddress (SK_GetModuleHandle (wszSteamLib), "SteamUser"))
  {
    SK_CreateDLLHook2 (     wszSteamLib,
                              "SteamUser",
                               SteamUser_Detour,
      static_cast_p2p <void> (&SteamUser_Original) );
  }

  if (SK_GetProcAddress (SK_GetModuleHandle (wszSteamLib), "SteamUtils"))
  {
    SK_CreateDLLHook2 (     wszSteamLib,
                              "SteamUtils",
                               SteamUtils_Detour,
      static_cast_p2p <void> (&SteamUtils_Original) );
  }

  if ( SK_GetProcAddress (
         SK_Modules->LoadLibraryLL (wszSteamLib),
           "SteamInternal_CreateInterface" )
     )
  {
    SK_CreateDLLHook2 (     wszSteamLib,
                              "SteamInternal_CreateInterface",
                               SteamInternal_CreateInterface_Detour,
      static_cast_p2p <void> (&SteamInternal_CreateInterface_Original),
                           &pfnSteamInternal_CreateInterface );
    MH_QueueEnableHook (    pfnSteamInternal_CreateInterface );

    return TRUE;
  }

  return FALSE;
}


extern bool
SK_D3D11_CaptureScreenshot (SK_ScreenshotStage when);

extern bool
SK_D3D12_CaptureScreenshot (SK_ScreenshotStage when);

extern bool
SK_D3D9_CaptureScreenshot (SK_ScreenshotStage when);

extern bool
SK_GL_CaptureScreenshot (SK_ScreenshotStage when);

SK_Steam_ScreenshotManager::~SK_Steam_ScreenshotManager (void)
{
  for ( int i = 0 ; i < _StatusTypes ; ++i )
  {
    if (           hSigReady [i] != INVALID_HANDLE_VALUE)
    {    SetEvent (hSigReady [i]);
      CloseHandle (hSigReady [i]);
                   hSigReady [i]  = INVALID_HANDLE_VALUE;
    }
  }
}


void
SK_Steam_ScreenshotManager::OnScreenshotRequest ( ScreenshotRequested_t *pParam )
{
  UNREFERENCED_PARAMETER (pParam);

  if ( (int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::D3D11 )
  {
    SK_D3D11_CaptureScreenshot (
      config.screenshots.show_osd_by_default ?
              SK_ScreenshotStage::EndOfFrame : SK_ScreenshotStage::BeforeOSD
    );

    return;
  }

  else if ( (int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::D3D12 )
  {
    SK_D3D12_CaptureScreenshot (
      config.screenshots.show_osd_by_default ?
              SK_ScreenshotStage::EndOfFrame : SK_ScreenshotStage::BeforeOSD
    );

    return;
  }

  else if ( (int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::OpenGL )
  {
    SK_GL_CaptureScreenshot (
      config.screenshots.show_osd_by_default ?
              SK_ScreenshotStage::EndOfFrame : SK_ScreenshotStage::BeforeOSD
    );

    return;
  }

  else if ( (int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::D3D9 )
  {
    SK_D3D9_CaptureScreenshot (
            config.screenshots.show_osd_by_default ?
              SK_ScreenshotStage::EndOfFrame : SK_ScreenshotStage::BeforeOSD
    );

    return;
  }

  //
  // This feature is not supported,  so go back to normal screenshots.
  request.Unregister ();

  auto pScreenshots =
    steam_ctx.Screenshots ();

  if (pScreenshots != nullptr)
  {
    pScreenshots->HookScreenshots   (false);
    pScreenshots->TriggerScreenshot (     );
  }
}

void
SK_Steam_CatastropicScreenshotFail (void)
{
  SK_RunOnce (
    SK_ImGui_Warning ( L"Something went catastrophically wrong attempting to "
                       L"take a Smart Steam Screenshot -- DISABLING FEATURE." )
  );

  auto pScreenshots =
    steam_ctx.Screenshots ();

  if (pScreenshots != nullptr)
  {
    pScreenshots->HookScreenshots   (false);
    pScreenshots->TriggerScreenshot (     );
  }
}

void
SK_Steam_ScreenshotManager::OnScreenshotReady ( ScreenshotReady_t *pParam )
{
  screenshots_handled.insert (
    std::make_pair ( pParam->m_hLocal,
                     pParam->m_eResult )
  );

  if (pParam->m_eResult == k_EResultOK)
    SetEvent (hSigReady [(UINT)ScreenshotStatus::Success]);

  else
  {
    steam_log->Log ( L"Screenshot Import/Write Failure for handle '%x'; EResult=%x",
                       pParam->m_hLocal, pParam->m_eResult );

    SetEvent (hSigReady [(UINT)ScreenshotStatus::Fail]);
  }
}


SK_Steam_ScreenshotManager::ScreenshotStatus
SK_Steam_ScreenshotManager::WaitOnScreenshot ( ScreenshotHandle handle,
                                               DWORD            dwTimeoutMs )
{
  DWORD dwStatus =
    MsgWaitForMultipleObjects ( _StatusTypes, hSigReady,
                                  FALSE, dwTimeoutMs,
                                    0x0 );

  if (  dwStatus >=  WAIT_OBJECT_0                         &&
        dwStatus <  (WAIT_OBJECT_0 + MAXIMUM_WAIT_OBJECTS) &&
      ( dwStatus  -  WAIT_OBJECT_0 ) < 2 )
  {
    UINT type = std::min ( 1UL, dwStatus - WAIT_OBJECT_0 );
    auto iter = screenshots_handled.find (handle);

    if (iter != screenshots_handled.cend ())
    {
      if (iter->second == k_EResultOK)
        return ScreenshotStatus::Success;
      else
        return ScreenshotStatus::Fail;
    }

    // This signal wasn't for us, so restore it...
    else
      SetEvent (hSigReady [type]);
  }

  // Unknown?!
  assert (false);

  return ScreenshotStatus::Fail;
}

void
SK_Steam_ScreenshotManager::init (void) noexcept
{
  __try {
    SK_GetCurrentRenderBackend ().screenshot_mgr.getRepoStats (true);

    auto pScreenshots =
      steam_ctx.Screenshots ();

    if (config.steam.screenshots.enable_hook && pScreenshots)
    {
      pScreenshots->HookScreenshots (
        config.steam.screenshots.enable_hook
      );

      for ( auto&& hSig : hSigReady )
      {
        if (hSig == INVALID_HANDLE_VALUE)
            hSig  = SK_CreateEvent (nullptr, FALSE, FALSE, nullptr);
      }
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) { };
}

bool  __SK_Steam_IgnoreOverlayActivation = false;
DWORD __SK_Steam_MagicThread             =     0;

class SK_Steam_OverlayManager
{
public:
  SK_Steam_OverlayManager (void) :
    activation ( this, &SK_Steam_OverlayManager::OnActivate      )
  {
    cursor_visible_ = false;
    active_         = false;
  }

  ~SK_Steam_OverlayManager (void)
  {
    cursor_visible_ = false;
    active_         = false;
  }

  STEAM_CALLBACK ( SK_Steam_OverlayManager,
                   OnActivate,
                   GameOverlayActivated_t,
                   activation )
  {
    if (__SK_Steam_IgnoreOverlayActivation)
      return;

    // If the game has an activation callback installed, then
    //   it's also going to see this event... make a note of that when
    //     tracking its believed overlay state.
    if (SK::SteamAPI::IsOverlayAware ())
        SK::SteamAPI::overlay_state = (pParam->m_bActive != 0);


    // If we want to use this as our own, then don't let the Steam overlay
    //   unpause the game on deactivation unless the control panel is closed.
    if (config.steam.reuse_overlay_pause && SK::SteamAPI::IsOverlayAware ())
    {
      // Deactivating, but we might want to hide this event from the game...
      if (pParam->m_bActive == 0)
      {
        extern bool SK_ImGui_Visible;

        // Undo the event the game is about to receive.
        if (SK_ImGui_Visible) SK::SteamAPI::SetOverlayState (true);
      }
    }


    const bool wasActive =
                  active_;

    active_ = (pParam->m_bActive != 0);

    if (wasActive != active_)
    {
      auto& io =
        ImGui::GetIO ();

      static bool capture_keys  = io.WantCaptureKeyboard;
      static bool capture_text  = io.WantTextInput;
      static bool capture_mouse = io.WantCaptureMouse;
      static bool nav_active    = io.NavActive;

      // When the overlay activates, stop blocking
      //   input !!
      if (! wasActive)
      {
        capture_keys  =
          io.WantCaptureKeyboard;
          io.WantCaptureKeyboard = false;

        capture_text  =
          io.WantTextInput;
          io.WantTextInput       = false;

        capture_mouse =
          io.WantCaptureMouse;
          io.WantCaptureMouse    = false;

        nav_active    =
          io.NavActive;
          io.NavActive           = false;

         ImGui::SetWindowFocus (nullptr);
      }

      else
      {
        io.WantCaptureKeyboard = SK_ImGui_Visible ? capture_keys  : false;
        io.WantCaptureMouse    = SK_ImGui_Visible ? capture_mouse : false;
        io.NavActive           = SK_ImGui_Visible ? nav_active    : false;

        ImGui::SetWindowFocus (nullptr);
        io.WantTextInput       = false;//capture_text;
      }

      ///if (active_) SK::Framerate::GetLimiter ()->suspend ();
      ///else         SK::Framerate::GetLimiter ()->resume  ();
    }
  }

  bool isActive (void) const {
    return active_;
  }

private:
  bool cursor_visible_; // Cursor visible prior to activation?
  bool active_;
};

static std::unique_ptr <SK_Steam_OverlayManager> overlay_manager = nullptr;

bool
WINAPI
SK_IsSteamOverlayActive (void)
{
  if (overlay_manager)
    return overlay_manager->isActive ();

  return false;
}



#include <steamapi/isteamcontroller.h>
#include <SpecialK/input/input.h>

#define SK_STEAM_READ(type)  SK_Steam_Backend->markRead   (type);
#define SK_STEAM_WRITE(type) SK_Steam_Backend->markWrite  (type);
#define SK_STEAM_VIEW(type)  SK_Steam_Backend->markViewed (type);

using SteamAPI_ISteamController_GetDigitalActionData_pfn = ControllerDigitalActionData_t (S_CALLTYPE *)(ISteamController *, ControllerHandle_t, ControllerDigitalActionHandle_t);
using SteamAPI_ISteamController_GetAnalogActionData_pfn  = ControllerAnalogActionData_t  (S_CALLTYPE *)(ISteamController *, ControllerHandle_t, ControllerAnalogActionHandle_t);

SteamAPI_ISteamController_GetDigitalActionData_pfn SteamAPI_ISteamController_GetDigitalActionData_Original = nullptr;
SteamAPI_ISteamController_GetAnalogActionData_pfn  SteamAPI_ISteamController_GetAnalogActionData_Original  = nullptr;

ControllerDigitalActionData_t
SteamAPI_ISteamController_GetDigitalActionData_Detour (ISteamController *This, ControllerHandle_t controllerHandle, ControllerDigitalActionHandle_t digitalActionHandle)
{
  static ControllerDigitalActionData_t
    neutralized = { false, false };

  if (SK_ImGui_WantGamepadCapture ())
    return neutralized;

  SK_STEAM_READ (sk_input_dev_type::Gamepad);

  return
    SteamAPI_ISteamController_GetDigitalActionData_Original (This, controllerHandle, digitalActionHandle);
}

ControllerAnalogActionData_t
SteamAPI_ISteamController_GetAnalogActionData_Detour (ISteamController *This, ControllerHandle_t controllerHandle, ControllerAnalogActionHandle_t analogActionHandle)
{
  static ControllerAnalogActionData_t
    neutralized = { k_EControllerSourceMode_None, 0.0f, 0.0f, false };

  if (SK_ImGui_WantGamepadCapture ())
    return neutralized;

  SK_STEAM_READ (sk_input_dev_type::Gamepad);

  return
    SteamAPI_ISteamController_GetAnalogActionData_Original (This, controllerHandle, analogActionHandle);
}



ISteamUserStats* SK_SteamAPI_UserStats   (void);
void             SK_SteamAPI_ContextInit (HMODULE hSteamAPI);


#define STEAM_VIRTUAL_HOOK(_Base,_Index,_Name,_Override,_Original,_Type) {\
  void** _vftable = *(void***)*(_Base);                                   \
                                                                          \
  if ((_Original) == nullptr) {                                           \
    SK_CreateVFTableHook ( L##_Name,                                      \
                             _vftable,                                    \
                               (_Index),                                  \
                                 (_Override),                             \
                                   (LPVOID *)&(_Original));               \
  }                                                                       \
}

using callback_func_t      =
void (__fastcall *)( CCallbackBase*, LPVOID );
using callback_func_fail_t =
void (__fastcall *)( CCallbackBase*, LPVOID, bool bIOFailure,
                                   SteamAPICall_t hSteamAPICall );

callback_func_t SteamAPI_UserStatsReceived_Original = nullptr;

extern "C"
{
  void __fastcall SteamAPI_UserStatsReceived_Detour (
                    CCallbackBase       *This,
                    UserStatsReceived_t *pParam
                  );
};

const char*
__stdcall
SK_Steam_ClassifyCallback (int iCallback)
{
  switch ((iCallback / 100) * 100)
  {
    case k_iSteamUserCallbacks:                  return "k_iSteamUserCallbacks";
    case k_iSteamGameServerCallbacks:            return "k_iSteamGameServerCallbacks";
    case k_iSteamFriendsCallbacks:               return "k_iSteamFriendsCallbacks";
    case k_iSteamBillingCallbacks:               return "k_iSteamBillingCallbacks";
    case k_iSteamMatchmakingCallbacks:           return "k_iSteamMatchmakingCallbacks";
    case k_iSteamContentServerCallbacks:         return "k_iSteamContentServerCallbacks";
    case k_iSteamUtilsCallbacks:                 return "k_iSteamUtilsCallbacks";
    case k_iClientFriendsCallbacks:              return "k_iClientFriendsCallbacks";
    case k_iClientUserCallbacks:                 return "k_iClientUserCallbacks";
    case k_iSteamAppsCallbacks:                  return "k_iSteamAppsCallbacks";
    case k_iSteamUserStatsCallbacks:             return "k_iSteamUserStatsCallbacks";
    case k_iSteamNetworkingCallbacks:            return "k_iSteamNetworkingCallbacks";
    case k_iClientRemoteStorageCallbacks:        return "k_iClientRemoteStorageCallbacks";
    case k_iClientDepotBuilderCallbacks:         return "k_iClientDepotBuilderCallbacks";
    case k_iSteamGameServerItemsCallbacks:       return "k_iSteamGameServerItemsCallbacks";
    case k_iClientUtilsCallbacks:                return "k_iClientUtilsCallbacks";
    case k_iSteamGameCoordinatorCallbacks:       return "k_iSteamGameCoordinatorCallbacks";
    case k_iSteamGameServerStatsCallbacks:       return "k_iSteamGameServerStatsCallbacks";
    case k_iSteam2AsyncCallbacks:                return "k_iSteam2AsyncCallbacks";
    case k_iSteamGameStatsCallbacks:             return "k_iSteamGameStatsCallbacks";
    case k_iClientHTTPCallbacks:                 return "k_iClientHTTPCallbacks";
    case k_iClientScreenshotsCallbacks:          return "k_iClientScreenshotsCallbacks";
    case k_iSteamScreenshotsCallbacks:           return "k_iSteamScreenshotsCallbacks";
    case k_iClientAudioCallbacks:                return "k_iClientAudioCallbacks";
    case k_iClientUnifiedMessagesCallbacks:      return "k_iClientUnifiedMessagesCallbacks";
    case k_iSteamStreamLauncherCallbacks:        return "k_iSteamStreamLauncherCallbacks";
    case k_iClientControllerCallbacks:           return "k_iClientControllerCallbacks";
    case k_iSteamControllerCallbacks:            return "k_iSteamControllerCallbacks";
    case k_iClientParentalSettingsCallbacks:     return "k_iClientParentalSettingsCallbacks";
    case k_iClientDeviceAuthCallbacks:           return "k_iClientDeviceAuthCallbacks";
    case k_iClientNetworkDeviceManagerCallbacks: return "k_iClientNetworkDeviceManagerCallbacks";
    case k_iClientMusicCallbacks:                return "k_iClientMusicCallbacks";
    case k_iClientRemoteClientManagerCallbacks:  return "k_iClientRemoteClientManagerCallbacks";
    case k_iClientUGCCallbacks:                  return "k_iClientUGCCallbacks";
    case k_iSteamStreamClientCallbacks:          return "k_iSteamStreamClientCallbacks";
    case k_IClientProductBuilderCallbacks:       return "k_IClientProductBuilderCallbacks";
    case k_iClientShortcutsCallbacks:            return "k_iClientShortcutsCallbacks";
    case k_iClientRemoteControlManagerCallbacks: return "k_iClientRemoteControlManagerCallbacks";
    case k_iSteamAppListCallbacks:               return "k_iSteamAppListCallbacks";
    case k_iSteamMusicCallbacks:                 return "k_iSteamMusicCallbacks";
    case k_iSteamMusicRemoteCallbacks:           return "k_iSteamMusicRemoteCallbacks";
    case k_iClientVRCallbacks:                   return "k_iClientVRCallbacks";
      //case k_iClientReservedCallbacks:             return "k_iClientReservedCallbacks";
      //case k_iSteamReservedCallbacks:              return "k_iSteamReservedCallbacks";
    case k_iSteamHTMLSurfaceCallbacks:           return "k_iSteamHTMLSurfaceCallbacks";
    case k_iClientVideoCallbacks:                return "k_iClientVideoCallbacks";
    case k_iClientInventoryCallbacks:            return "k_iClientInventoryCallbacks";
    default:                                     return "UNKNOWN";
  }
}

extern "C"
void
S_CALLTYPE
SteamAPI_RegisterCallback_Detour (class CCallbackBase *pCallback, int iCallback)
{
  std::wstring caller =
    SK_GetCallerName ();

  if (steam_callback_cs != nullptr)
      steam_callback_cs->lock ();

  switch (iCallback)
  {
    case GameOverlayActivated_t::k_iCallback:
    {
      if (!__SK_Steam_IgnoreOverlayActivation)
      {
        steam_log->Log (L" * (%-28s) Installed Overlay Activation Callback",
                        caller.c_str ());

        if (SK_ValidatePointer (pCallback))
          overlay_activation_callbacks->insert (pCallback);
      }

      else
        iCallback = UserStatsReceived_t::k_iCallback;
    } break;

    case ScreenshotRequested_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Installed Screenshot Callback",
                      caller.c_str () );
      (*ScreenshotRequested_callbacks)[pCallback] = true;
      break;
    case UserStatsReceived_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Installed User Stats Receipt Callback",
                      caller.c_str () );
      (*UserStatsReceived_callbacks)[pCallback] = true;
      break;
    case UserStatsStored_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Installed User Stats Storage Callback",
                      caller.c_str () );
      (*UserStatsStored_callbacks)[pCallback] = true;
      break;
    case UserAchievementStored_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Installed User Achievements Storage Callback",
                      caller.c_str () );
      (*UserAchievementStored_callbacks)[pCallback] = true;
      break;
    case UserStatsUnloaded_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Installed User Stats Unloaded Callback",
                      caller.c_str () );
      (*UserStatsUnloaded_callbacks)[pCallback] = true;
      break;
    case SteamShutdown_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Installed SteamAPI Shutdown Callback",
                      caller.c_str () );
      break;
    case DlcInstalled_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Installed DLC Installation Callback",
                      caller.c_str () );
      break;
    case SteamAPICallCompleted_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Installed Async. SteamAPI Completion Callback",
                      caller.c_str () );
      break;
    case SteamServersConnected_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Installed Steam Server Connected Callback",
                      caller.c_str () );
      break;
    case SteamServerConnectFailure_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Installed Steam Server Connection Failure Callback",
                      caller.c_str () );
      break;
    case SteamServersDisconnected_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Installed Steam Server Disconnect Callback",
                      caller.c_str () );
      break;
    case LobbyEnter_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Installed Lobby Enter Callback",
                      caller.c_str () );
      break;
    case LobbyDataUpdate_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Installed Lobby Data Update Callback",
                      caller.c_str () );
      break;
    case LobbyChatUpdate_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Installed Lobby Chat Update Callback",
                      caller.c_str () );
      break;
    case GameLobbyJoinRequested_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Installed Game Lobby Join Request Callback",
                      caller.c_str () );
      break;
    case GameRichPresenceJoinRequested_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Installed Game Rich Presence Join Request Callback",
                      caller.c_str () );
      break;
    case P2PSessionConnectFail_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Installed Peer to Peer Session Connect Failure Callback",
                      caller.c_str () );
      break;
    case P2PSessionRequest_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Installed Peer to Peer Session Request Callback",
                      caller.c_str () );
      break;
    case GetAuthSessionTicketResponse_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Installed Auth Session Ticket Response Callback",
                      caller.c_str () );
      break;
    default:
    {
      steam_log->Log ( L" * (%-28s) Installed Callback (Class=%hs, Id=%li)",
                      caller.c_str (), SK_Steam_ClassifyCallback (iCallback), iCallback % 100 );
    } break;
  }

  if (SK_IsAddressExecutable           (pCallback))
  {
    // We will dispatch this callback to all registered interfaces ourself.
    if (iCallback != UserStatsReceived_t::k_iCallback)
      SteamAPI_RegisterCallback_Original (pCallback, iCallback);
  }

  if (steam_callback_cs != nullptr)
      steam_callback_cs->unlock ();
}

void
SK_SteamAPI_EraseActivationCallback (class CCallbackBase *pCallback)
{
  auto orig_se =
  SK_SEH_ApplyTranslator (
    SK_FilteringStructuredExceptionTranslator (EXCEPTION_ACCESS_VIOLATION)
  );
  try
  {
    overlay_activation_callbacks->erase (pCallback);
  }

  catch (const SK_SEH_IgnoredException&)
  {
  }
  SK_SEH_RemoveTranslator (orig_se);
}

extern "C"
void
S_CALLTYPE
SteamAPI_UnregisterCallback_Detour (class CCallbackBase *pCallback)
{
  std::wstring caller =
    SK_GetCallerName ();

  if (steam_callback_cs != nullptr)
      steam_callback_cs->lock ();

  int iCallback =
    pCallback->GetICallback ();

  switch (iCallback)
  {
    case GameOverlayActivated_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled Overlay Activation Callback",
                      caller.c_str () );

      SK_SteamAPI_EraseActivationCallback (pCallback);

      break;
    case ScreenshotRequested_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled Screenshot Callback",
                      caller.c_str () );
      (*ScreenshotRequested_callbacks)[pCallback] = false;
      break;
    case UserStatsReceived_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled User Stats Receipt Callback",
                      caller.c_str () );
      (*UserStatsReceived_callbacks)[pCallback] = false;
      break;
    case UserStatsStored_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled User Stats Storage Callback",
                      caller.c_str () );
      (*UserStatsStored_callbacks)[pCallback] = false;
      break;
    case UserAchievementStored_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled User Achievements Storage Callback",
                      caller.c_str () );
      (*UserAchievementStored_callbacks)[pCallback] = false;
      break;
    case UserStatsUnloaded_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled User Stats Unloaded Callback",
                      caller.c_str () );
      (*UserStatsUnloaded_callbacks)[pCallback] = false;
      break;
    case SteamShutdown_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled SteamAPI Shutdown Callback",
                      caller.c_str () );
      break;
    case DlcInstalled_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled DLC Installation Callback",
                      caller.c_str () );
      break;
    case SteamAPICallCompleted_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled Async. SteamAPI Completion Callback",
                      caller.c_str () );
      break;
    case SteamServersConnected_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled Steam Server Connected Callback",
                      caller.c_str () );
      break;
    case SteamServerConnectFailure_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled Steam Server Connection Failure Callback",
                      caller.c_str () );
      break;
    case SteamServersDisconnected_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled Steam Server Disconnect Callback",
                      caller.c_str () );
      break;
    case LobbyEnter_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled Lobby Enter Callback",
                      caller.c_str () );
      break;
    case LobbyDataUpdate_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled Lobby Data Update Callback",
                      caller.c_str () );
      break;
    case LobbyChatUpdate_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled Lobby Chat Update Callback",
                      caller.c_str () );
      break;
    case GameLobbyJoinRequested_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled Game Lobby Join Request Callback",
                      caller.c_str () );
      break;
    case GameRichPresenceJoinRequested_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled Game Rich Presence Join Request Callback",
                      caller.c_str () );
      break;
    case P2PSessionConnectFail_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled Peer to Peer Session Connect Failure Callback",
                      caller.c_str () );
      break;
    case P2PSessionRequest_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled Peer to Peer Session Request Callback",
                      caller.c_str () );
    case GetAuthSessionTicketResponse_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled Auth Session Ticket Response Callback",
                      caller.c_str () );
      break;
    default:
    {
      steam_log->Log ( L" * (%-28s) Uninstalled Callback (Class=%hs, Id=%li)",
                      caller.c_str (), SK_Steam_ClassifyCallback (iCallback), iCallback % 100 );
    } break;
  }

  SteamAPI_UnregisterCallback_Original (pCallback);

  if (steam_callback_cs != nullptr)
      steam_callback_cs->unlock ();
}

const char*
SK_Steam_PopupOriginToStr (int origin)
{
  switch (origin)
  {
    case 0:
      return "TopLeft";
    case 1:
      return "TopRight";
    case 2:
      return "BottomLeft";
    case 3:
    default:
      return "BottomRight";
    case 4:
      return "DontCare";
  }
}

const wchar_t*
SK_Steam_PopupOriginToWStr (int origin)
{
  switch (origin)
  {
    case 0:
      return L"TopLeft";
    case 1:
      return L"TopRight";
    case 2:
      return L"BottomLeft";
    case 3:
    default:
      return L"BottomRight";
    case 4:
      return L"DontCare";
  }
}

int
SK_Steam_PopupOriginWStrToEnum (const wchar_t* str)
{
  std::wstring name (str);

  if (name == L"TopLeft")
    return 0;
  if (name == L"TopRight")
    return 1;
  if (name == L"BottomLeft")
    return 2;
  if (name == L"DontCare")
    return 4;
  /*if (name == L"TopLeft")*/
    return 3;

  // TODO: TopCenter, BottomCenter, CenterLeft, CenterRight
}

int
SK_Steam_PopupOriginStrToEnum (const char* str)
{
  std::string name (str);

  if (name == "TopLeft")
    return 0;
  if (name == "TopRight")
    return 1;
  if (name == "BottomLeft")
    return 2;
  if (name == "DontCare")
    return 4;
  /*if (name == L"TopLeft")*/
    return 3;
}

void SK_Steam_SetNotifyCorner (void);


//extern bool
//ISteamController_GetControllerState_Detour (
//  ISteamController*       This,
//  uint32                  unControllerIndex,
//  SteamControllerState_t *pState );



ISteamFriends*
SK_SteamAPI_Friends (void)
{
  return steam_ctx.Friends ();
}

void
SK_Steam_SetNotifyCorner (void)
{
  // 4 == Don't Care
  if (config.steam.notify_corner != 4)
  {
    if (steam_ctx.Utils ())
    {
      steam_ctx.Utils ()->SetOverlayNotificationPosition (
        static_cast <ENotificationPosition> (config.steam.notify_corner)
      );
    }
  }
}

void
SK_SteamAPIContext::Shutdown (bool bGameRequested)
{
  static volatile LONG _SimpleMutex = 0;

  if (InterlockedCompareExchange (&_SimpleMutex, 1, 0) == 0)
  {
    SK_Steam_KillPump  ();

    if (client_)
    {
      SK_SteamAPI_DestroyManagers  ();

      ReleaseThreadUser ();
      ReleaseThreadPipe ();
    }

    hSteamPipe      = 0;
    hSteamUser      = 0;

    client_         = nullptr;
    user_           = nullptr;
    user_stats_     = nullptr;
    apps_           = nullptr;
    friends_        = nullptr;
    utils_          = nullptr;
    screenshots_    = nullptr;
    controller_     = nullptr;
    music_          = nullptr;
    remote_storage_ = nullptr;

    client_ver_         = 0;
    user_ver_           = 0;
    utils_ver_          = 0;
    remote_storage_ver_ = 0;

    // Calls to SK_DisableHook do nothing while DLL is shutting down,
    //   just kinda ignore this whole thing :)
    if (! ReadAcquire (&__SK_DLL_Ending))
    {
      if (bGameRequested)
      {
        SK_ReleaseAssert (SteamAPI_Shutdown_Original != nullptr);

        if (SteamAPI_Shutdown_Original != nullptr)
        {   SteamAPI_Shutdown_Original ();

          SK_DisableHook (SteamAPI_RunCallbacks);
          SK_DisableHook (SteamAPI_Shutdown);
        }
      }
    }

    InterlockedExchange (&_SimpleMutex, 0);
  }
}


const char*
SK_SteamAPIContext::GetSteamInstallPath (void)
{
  return
    SteamAPI_GetSteamInstallPath ();
}

bool
SK_SteamAPIContext::OnVarChange (SK_IVariable* var, void* val)
{
  if (val == nullptr)
    return false;

  const SK_ICommandProcessor*
    pCommandProc = SK_GetCommandProcessor ();

  UNREFERENCED_PARAMETER (pCommandProc);


  bool known = false;

  // ( Dead man's switch for silly pirates that attempt to bypass framerate
  //    limit with a hex edit rather than a proper forked project - "crack"
  //      this the right way please )
  if (var == tbf_pirate_fun)
  {
    known = true;

    static float history [4] = { 45.0f, 45.0f, 45.0f, 45.0f };
    static int   idx         = 0;

    history [idx] = *static_cast <float *> (val);

    if (*static_cast <float *> (val) != 45.0f)
    {
      if (idx > 0 && history [idx-1] != 45.0f)
      {
        const SK_IVariable* _var =
          SK_GetCommandProcessor ()->ProcessCommandLine (
            "Textures.LODBias"
          ).getVariable ();

        if (_var != nullptr)
        {
          *static_cast <float *> (_var->getValuePointer ()) =
            *static_cast <float *> (val);
        }
      }
    }

    idx++;

    if (idx > 3)
      idx = 0;

    tbf_float = 0.0001f;

    return true;
  }


  if (var == notify_corner)
  {
    known = true;

    config.steam.notify_corner =
      SK_Steam_PopupOriginStrToEnum (*reinterpret_cast <char **> (val));

    strncpy_s (                   var_strings.notify_corner,  16,
      SK_Steam_PopupOriginToStr (config.steam.notify_corner), _TRUNCATE
    );

    SK_Steam_SetNotifyCorner ();

    return true;
  }

  else if (var == popup_origin)
  {
    known = true;

    config.steam.achievements.popup.origin =
      SK_Steam_PopupOriginStrToEnum (*reinterpret_cast <char **> (val));

    strncpy_s ( var_strings.popup_origin,
        sizeof (var_strings.popup_origin),
                 SK_Steam_PopupOriginToStr (
                   config.steam.achievements.popup.origin
                ), _TRUNCATE
             );

    return true;
  }

  dll_log->Log ( L"[SteamAPI] UNKNOWN Variable Changed (%p --> %p)",
                 var, val );

  return false;
}


#define FREEBIE     96.0f
#define COMMON      75.0f
#define UNCOMMON    50.0f
#define RARE        25.0f
#define VERY_RARE   15.0f
#define ONE_PERCENT  1.0f


std::string
SK_Steam_RarityToColor (float percent)
{
#ifdef SK_USE_OLD_ACHIEVEMENT_COLORS
  if (percent <= ONE_PERCENT)
    return "FFFF1111";

  if (percent <= VERY_RARE)
    return "FFFFC711";

  if (percent <= RARE)
    return "FFFFFACD";

  if (percent <= UNCOMMON)
    return "FF0084FF";

  if (percent <= COMMON)
    return "FFFFFFFF";

  if (percent >= FREEBIE)
    return "FFBBBBBB";

  return "FFFFFFFF";
#else
  ImVec4 color =
    ImColor::HSV (0.4f * (percent / 100.0f), 1.0f, 1.0f);

  return
    SK_FormatString ("FF%02X%02X%02X", (int)(color.x * 255.0f),
                                       (int)(color.y * 255.0f),
                                       (int)(color.z * 255.0f) );
#endif
}

const char*
SK_Steam_RarityToName (float percent)
{
  if (percent <= ONE_PERCENT)
    return "The Other 1%";

  if (percent <= VERY_RARE)
    return "Very Rare";

  if (percent <= RARE)
    return "Rare";

  if (percent <= UNCOMMON)
    return "Uncommon";

  if (percent <= COMMON)
    return "Common";

  if (percent >= FREEBIE)
    return "Freebie";

  return "Common";
}

bool           has_global_data  = false;
LONG           next_friend      = 0;
LONG           friend_count     = 0;
SteamAPICall_t friend_query     = 0;
LONGLONG       friends_done     = 0;

class SK_Steam_AchievementManager
{
public:
  class Achievement : public SK_SteamAchievement
  {
  public:
    Achievement (int idx, const char* szName, ISteamUserStats* stats)
    {
      idx_            = idx;

      global_percent_ = 0.0f;
      unlocked_       = false;
      time_           = 0;

      friends_.possible = 0;
      friends_.unlocked = 0;

      icons_.achieved   = nullptr;
      icons_.unachieved = nullptr;

      progress_.max     = 0;
      progress_.current = 0;

      name_ =
        _strdup (szName);

      if (stats != nullptr)
      {
        const char* human =
          stats->GetAchievementDisplayAttribute (szName, "name");

        const char* desc =
          stats->GetAchievementDisplayAttribute (szName, "desc");

        human_name_ =
          _strdup (human != nullptr ? human : "<INVALID>");
        desc_ =
          _strdup ( desc != nullptr ? desc  : "<INVALID>");
      }
    }

     Achievement (const Achievement& copy) = delete;

    ~Achievement (void)
    {
      if (name_ != nullptr)
      {
        free ((void *)name_);
        name_ = nullptr;
      }

      if (human_name_ != nullptr)
      {
        free ((void *)human_name_);
        human_name_ = nullptr;
      }

      if (desc_ != nullptr)
      {
        free ((void *)desc_);
        desc_ = nullptr;
      }

      if (icons_.unachieved != nullptr)
      {
        free (icons_.unachieved);
        icons_.unachieved = nullptr;
      }

      if (icons_.achieved != nullptr)
      {
        free (icons_.achieved);
        icons_.achieved = nullptr;
      }
    }

    void update (ISteamUserStats* stats)
    {
      stats->GetAchievementAndUnlockTime ( name_,
                                          &unlocked_,
                              (uint32_t *)&time_ );
    }

    void update_global (ISteamUserStats* stats)
    {
      // Reset to 0.0 on read failure
      if (! stats->GetAchievementAchievedPercent (
              name_,
                &global_percent_                 )
         )
      {
        steam_log->Log (
          L" Global Achievement Read Failure For '%hs'", name_
        );
        global_percent_ = 0.0f;
      }
    }
  };

private:
  struct SK_AchievementPopup
  {
    CEGUI::Window* window;
    DWORD          time;
    bool           final_pos; // When the animation is finished, this will be set.
    Achievement*   achievement;
  };

public:
  SK_Steam_AchievementManager (const wchar_t* wszUnlockSound) :
    unlock_listener ( this, &SK_Steam_AchievementManager::OnUnlock       ),
    stat_receipt    ( this, &SK_Steam_AchievementManager::AckStoreStats  ),
    icon_listener   ( this, &SK_Steam_AchievementManager::OnRecvIcon     ),
    async_complete  ( this, &SK_Steam_AchievementManager::OnAsyncComplete)
  {
    percent_unlocked = 0.0f;
    lifetime_popups  = 0;

    ISteamUserStats* user_stats = steam_ctx.UserStats ();
    ISteamFriends*   friends    = steam_ctx.Friends   ();

    if (! user_stats)
    {
      steam_log->Log ( L" *** Cannot get ISteamUserStats interface, bailing"
                       L"-out of Achievement Manager Init. ***" );
      return;
    }

    if (! friends)
    {
      steam_log->Log ( L" *** Cannot get ISteamFriends interface, bailing"
                       L"-out of Achievement Manager Init. ***" );
      return;
    }

    loadSound (wszUnlockSound);

    uint32_t achv_reserve =
      std::max (user_stats->GetNumAchievements (), (uint32_t)128UL);
    friend_count =
      friends->GetFriendCount (k_EFriendFlagImmediate);

    // OFFLINE MODE
    if ( friend_count <= 0 ||
         friend_count >  8192 )
         friend_count =  0;

    else {
      friend_sid_to_idx.reserve (friend_count);
      friend_stats.resize       (friend_count);
    }

    achievements.list.resize        (achv_reserve);
    achievements.string_map.reserve (achv_reserve);

    for (uint32_t i = 0; i < achv_reserve; i++)
      achievements.list [i] = nullptr;
  }

  ~SK_Steam_AchievementManager (void)
  {
    if ((! default_loaded) && (unlock_sound != nullptr))
    {
      delete [] unlock_sound;
      unlock_sound = nullptr;
    }

    unlock_listener.Unregister ();
    stat_receipt.Unregister    ();
    icon_listener.Unregister   ();
    async_complete.Unregister  ();
  }

  void log_all_achievements (void) const
  {
    ISteamUserStats* stats = steam_ctx.UserStats ();

    if (stats == nullptr)
      return;

    for (uint32 i = 0; i < stats->GetNumAchievements (); i++)
    {
      Achievement* pAchievement = achievements.list [i];

      if (! pAchievement)
        continue;

      Achievement& achievement = *pAchievement;

      steam_log->LogEx (false, L"\n [%c] Achievement %03lu......: '%hs'\n",
                        achievement.unlocked_ ? L'X' : L' ',
                        i, achievement.name_
      );
      steam_log->LogEx (false,
                        L"  + Human Readable Name...: %hs\n",
                        achievement.human_name_);
      if (achievement.desc_ != nullptr && strlen (achievement.desc_))
        steam_log->LogEx (false,
                          L"  *- Detailed Description.: %hs\n",
                          achievement.desc_);

      if (achievement.global_percent_ > 0.0f)
        steam_log->LogEx (false,
                          L"  #-- Rarity (Global).....: %6.2f%%\n",
                          achievement.global_percent_);

      if (achievement.friends_.possible > 0)
        steam_log->LogEx ( false,
                          L"  #-- Rarity (Friend).....: %6.2f%%\n",
          100.0 * (static_cast <double> (achievement.friends_.unlocked) /
                   static_cast <double> (achievement.friends_.possible)) );

      if (achievement.unlocked_)
      {
        steam_log->LogEx (false,
                          L"  @--- Player Unlocked At.: %s",
                          _wctime32 (&achievement.time_));
      }
    }

    steam_log->LogEx (false, L"\n");
  }

  STEAM_CALLRESULT ( SK_Steam_AchievementManager,
                     OnRecvSelfStats,
                     UserStatsReceived_t,
                     self_listener )
  {
    self_listener.Cancel ();

    uint32 app_id =
      CGameID (pParam->m_nGameID).AppID ();

    if (app_id != SK::SteamAPI::AppID ())
    {
      steam_log->Log ( L" Got User Achievement Stats for Wrong Game (%lu)",
                       app_id );
      return;
    }

    ISteamUser*      user    = steam_ctx.User      ();
    ISteamUserStats* stats   = steam_ctx.UserStats ();
    ISteamFriends*   friends = steam_ctx.Friends   ();

    if (! (user && stats && friends && pParam))
      return;

    auto user_id =
      user->GetSteamID ().ConvertToUint64 ();


#if 1
    //if (has_global_data)
    {
      for (auto& it : *UserStatsReceived_callbacks)
      {
        if (it.second && SK_IsAddressExecutable (it.first, true))
        {
          if (pParam->m_eResult != k_EResultOK)
          {
            if ( user_id ==
                 pParam->m_steamIDUser.ConvertToUint64 () )
            {
              steam_log->Log (
                L"Got UserStatsReceived Failure for Current Player, m_eResult=%x",
                  pParam->m_eResult
              );
            }
          }

          if (pParam->m_eResult                        == k_EResultOK &&
              pParam->m_steamIDUser.ConvertToUint64 () == user_id)
          {
            static bool          once = false;
            if (! std::exchange (once, true))
            {
              TryCallback ((class CCallbackBase*)it.first, pParam);
            }
          }
        }
      }
    }
#endif


    if (pParam->m_eResult == k_EResultOK)
    {
      if ( pParam->m_steamIDUser.ConvertToUint64 () ==
                            user_id )
      {
        pullStats ();

        if ( config.steam.achievements.pull_global_stats &&
                                     (! has_global_data) )
        {
          has_global_data = true;

          global_request =
            stats->RequestGlobalAchievementPercentages ();
        }

        // On first stat reception, trigger an update for all friends so
        //   that we can display friend achievement stats.
        if ( next_friend == 0 && app_cache_mgr->wantFriendStats () )
        {
          static bool run_once = false;


          if (run_once)
          {
            // Uninstall the game's callback while we're fetching friend stats,
            //   some games panic when they get data from other users.
            for ( auto it : *UserStatsReceived_callbacks )
            {
              if (it.second && SK_IsAddressExecutable (it.first))
                SteamAPI_UnregisterCallback_Original ((class CCallbackBase *)it.first);
            }

            friend_count =
              friends->GetFriendCount (k_EFriendFlagImmediate);

            // May be -1 in offline mode
            if (friend_count > 0)
            {
              friend_stats.resize       (friend_count);
              friend_sid_to_idx.reserve (friend_count);

              // Enumerate all known friends immediately
              for (int i = 0 ; i < friend_count; i++)
              {
                CSteamID sid ( friends->GetFriendByIndex ( i,
                                k_EFriendFlagImmediate
                                                         )
                             );

                friend_sid_to_idx [sid.ConvertToUint64 ()] = i;
                friend_stats      [i].name       =
                  friends->GetFriendPersonaName (sid);
                friend_stats      [i].account_id =
                                         sid.ConvertToUint64 ();
                friend_stats      [i].unlocked.resize (
                  achievements.list.capacity ()
                );
              }

              SteamAPICall_t hCall =
                stats->RequestUserStats (
                  CSteamID (friend_stats [next_friend].account_id)
                );

              friend_query = hCall;

              friend_listener.Set ( hCall,
                                     this,
                &SK_Steam_AchievementManager::OnRecvFriendStats );
            }
          }

          else
          {
            run_once = true;

                                           requestStats ();
            steam_ctx.UserStats ()->RequestCurrentStats ();
          }
        }
      }
    }
  }

  void TryCallback (CCallbackBase* pCallback, UserStatsReceived_t* pParam)
  {
    __try {
      if (SK_ValidatePointer (pParam, true))
              pCallback->Run (pParam);
    }
    __except ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                  EXCEPTION_EXECUTE_HANDLER  :
                  EXCEPTION_CONTINUE_SEARCH ) {
    }
  }

  STEAM_CALLRESULT ( SK_Steam_AchievementManager,
                     OnRecvFriendStats,
                     UserStatsReceived_t,
                     friend_listener )
  {
    friend_listener.Cancel ();

    if ( ReadAcquire (&__SK_DLL_Ending) ||
         ReadAcquire (&__SK_Steam_Downloading) )
      return;

    uint32 app_id =
      CGameID (pParam->m_nGameID).AppID ();

    if (app_id != SK::SteamAPI::AppID ())
    {
      steam_log->Log ( L" Got User Achievement Stats for Wrong Game (%lu)",
                       app_id );
      return;
    }

    ISteamUser*      user    = steam_ctx.User      ();
    ISteamUserStats* stats   = steam_ctx.UserStats ();
    ISteamFriends*   friends = steam_ctx.Friends   ();

    if (! (user && stats && friends && pParam))
      return;

    auto user_id =
      user->GetSteamID ().ConvertToUint64 ();

#if 1
    for (auto& it : *UserStatsReceived_callbacks)
    {
      if (it.second && SK_IsAddressExecutable (it.first, true))
      {
        auto override_params =
          *pParam;

        if (override_params.m_eResult != k_EResultOK)
        {
          if (                          user_id !=
               override_params.m_steamIDUser.ConvertToUint64 () )
          {
            // This is normal, so ignore in low log levels
            if (config.system.log_level > 0)
            {
              steam_log->Log (
                L"Got UserStatsReceived Failure for Friend, m_eResult=%x",
                  pParam->m_eResult
              );
            }
          }
        }

        if ( pParam->m_eResult == k_EResultOK &&
             pParam->m_steamIDUser.ConvertToUint64 () == user_id )
        {
          // Control freaks the @#$% out if achievement status isn't
          //   successful at all times.
          TryCallback (((class CCallbackBase*)it.first), pParam);
        }
      }
    }
#endif

    // A user may return kEResultFail if they don't own this game, so
    //   do this anyway and continue enumerating remaining friends.
    if (pParam->m_eResult == k_EResultOK)
    {
      // If the stats are not for the player, they are probably for one of
      //   the player's friends.
      if ( k_EFriendRelationshipFriend ==
             friends->GetFriendRelationship (pParam->m_steamIDUser) )
      {
        bool     can_unlock  = false;
        int      unlocked    = 0;

        uint32_t friend_idx  =
          friend_sid_to_idx [pParam->m_steamIDUser.ConvertToUint64 ()];

        for (uint32 i = 0; i < stats->GetNumAchievements (); i++)
        {
          std::string name = stats->GetAchievementName (i);

          stats->GetUserAchievement (
            pParam->m_steamIDUser,
                      name.c_str (),
                    reinterpret_cast <bool *> (
                      &friend_stats [friend_idx].unlocked [i]
                    )
          );

          if (friend_stats [friend_idx].unlocked [i])
          {
            unlocked++;

            // On the first unlocked achievement, make a note...
            if (! can_unlock)
            {
              ///steam_log->Log ( L"Received Achievement Stats for Friend: '%hs'",
              ///friends->GetFriendPersonaName (pParam->m_steamIDUser) );
            }

            can_unlock = true;

            ++achievements.list [i]->friends_.unlocked;

            ///steam_log->Log (L" >> Has unlocked '%24hs'", szName);
          }
        }

        friend_stats [friend_idx].percent_unlocked =
          static_cast <float> (unlocked) /
          static_cast <float> (stats->GetNumAchievements ());

        app_cache_mgr->setFriendAchievPct (
          friend_stats   [friend_idx].account_id,
            friend_stats [friend_idx].percent_unlocked
        );

        // Second pass over all achievements, incrementing the number of friends who
        //   can potentially unlock them.
        if (can_unlock)
        {
          app_cache_mgr->setFriendOwnership (
            pParam->m_steamIDUser.ConvertToUint64 (),
              SK_AppCache_Manager::OwnsGame
          );

          for (uint32 i = 0; i < stats->GetNumAchievements (); i++)
            ++achievements.list [i]->friends_.possible;
        }

        else
        {
          app_cache_mgr->setFriendOwnership (
            pParam->m_steamIDUser.ConvertToUint64 (),
              SK_AppCache_Manager::DoesNotOwn
          );
        }
      }
    }

    // TODO: The function that walks friends should be a lambda
    else
    {
      // Friend doesn't own the game, so just skip them...
      if (pParam->m_eResult == k_EResultFail)
      {
        app_cache_mgr->setFriendOwnership (
          pParam->m_steamIDUser.ConvertToUint64 (),
            SK_AppCache_Manager::DoesNotOwn
        );
      }

      else
      {
        steam_log->Log ( L" >>> User stat receipt unsuccessful! "
                         L"(m_eResult = 0x%04X)",
                           pParam->m_eResult );
      }
    }

    if (               friend_count > 0 &&
         next_friend < friend_count )
    {
      SK_AppCache_Manager::Ownership ownership =
        SK_AppCache_Manager::Unknown;

      time_t time_now = 0;
      time (&time_now);

      //float percent = 0.0f;

      do {
        time_t last_updated = 0;

        ownership =
          app_cache_mgr->getFriendOwnership (
            friend_stats [next_friend++].account_id,
              &last_updated
          );

        const time_t two_days = 172800;

        if ( ( next_friend == friend_count ) ||
             ( ( time_now - last_updated )   > two_days ) )
        {
          break;
        }

        //// TODO: We need to serialize and stash all unlocked achievements
        ////         so that friend % unlock stats are correct.
        ///const time_t two_hours = 7200;
        ///
        ///percent =
        ///  app_cache_mgr->getFriendAchievPct (
        ///    friend_stats [friend_idx].account_id,
        ///      &last_updated
        ///  );
        ///
        ///if ( percent != 0.0f && ( time_now - last_updated ) < two_hours )
        ///{
        ///  friend_stats [friend_idx].percent_unlocked =
        ///    percent;
        ///}
      } while ( ownership == SK_AppCache_Manager::DoesNotOwn );

      if (next_friend < friend_count)
      {
        CSteamID sid (
          friend_stats [next_friend].account_id
        );

        SteamAPICall_t hCall =
          stats->RequestUserStats (sid);

        friend_query        = hCall;
        friend_listener.Set ( hCall,
          this, &SK_Steam_AchievementManager::OnRecvFriendStats
        );
      }
    }

    if ( ! ( friend_count > 0 &&
              next_friend < friend_count ) )
    {
      // Reinstate stat receipt callback
      for (auto& it : *UserStatsReceived_callbacks)
      {
        if (it.second && SK_IsAddressExecutable (it.first, true))
          SteamAPI_RegisterCallback_Original ((CCallbackBase *)it.first, UserStatsReceived_t::k_iCallback);
      }

#if 0
      // Trigger the game's callback in case it's stupidly waiting synchronously for
      //   this to happen (i.e. Yakuza Kiwami 2).
      steam_ctx.UserStats ()->RequestCurrentStats ();
#endif

      friends_done =
        ReadAcquire64 (&SK_SteamAPI_CallbackRunCount);

      app_cache_mgr->saveAppCache ();
    }
  }

  STEAM_CALLBACK ( SK_Steam_AchievementManager,
                   OnAsyncComplete,
                   SteamAPICallCompleted_t,
                   async_complete )
  {
    bool                 failed    = true;
    ESteamAPICallFailure eCallFail = k_ESteamAPICallFailureNone;

    if (steam_ctx.Utils ())
    {
      steam_ctx.Utils ()->IsAPICallCompleted (
        pParam->m_hAsyncCall, &failed
      );

      if (failed)
      {
        eCallFail =
          steam_ctx.Utils ()->GetAPICallFailureReason (
            pParam->m_hAsyncCall
          );
      }
    }

    if (pParam->m_hAsyncCall == global_request)
    {
      if (! failed)
      {
        has_global_data = true;

        SK_Steam_UpdateGlobalAchievements ();
      }

      else
      {
        steam_log->Log (
          L" Non-Zero Result (%li) * GlobalAchievementPercentagesReady_t",
            eCallFail
        );

        has_global_data = false;
        return;
      }
    }

    else {
      if (failed)
      {
        //steam_log->Log ( L" Asynchronous SteamAPI Call _NOT ISSUED BY Special K_ Failed: "
        //L"(reason=%lu)",
        //eCallFail );
      }
    }
  }

  STEAM_CALLBACK ( SK_Steam_AchievementManager,
                   AckStoreStats,
                   UserStatsStored_t,
                   stat_receipt )
  {
    uint32 app_id =
      CGameID (pParam->m_nGameID).AppID ();

    // Sometimes we receive event callbacks for games that aren't this one...
    //   ignore those!
    if (app_id != SK::SteamAPI::AppID ())
      return;

    steam_log->Log ( L" >> Stats Stored for AppID: %lu",
                       app_id );
  }

  STEAM_CALLBACK ( SK_Steam_AchievementManager,
                   OnRecvIcon,
                   UserAchievementIconFetched_t,
                   icon_listener )
  {
    if (pParam->m_nGameID.AppID () != SK::SteamAPI::AppID ())
    {
      steam_log->Log ( L" >>> Received achievement icon for unrelated game (AppId=%lu) <<<",
                         pParam->m_nGameID.AppID () );
      return;
    }

    if (pParam->m_nIconHandle == 0)
      return;
  }

  STEAM_CALLBACK ( SK_Steam_AchievementManager,
                   OnUnlock,
                   UserAchievementStored_t,
                   unlock_listener )
  {
    uint32 app_id =
      CGameID (pParam->m_nGameID).AppID ();

    // Sometimes we receive event callbacks for games that aren't this one...
    //   ignore those!
    if (app_id != SK::SteamAPI::AppID ())
    {
      steam_log->Log ( L" >>> Received achievement unlock for unrelated "
                       L"game (AppId=%lu) <<<", app_id );
      return;
    }

    Achievement* achievement =
      getAchievement (
        pParam->m_rgchAchievementName
      );

    if (achievement != nullptr)
    {
      if (steam_ctx.UserStats ())
        achievement->update (steam_ctx.UserStats ());

      achievement->progress_.current = pParam->m_nCurProgress;
      achievement->progress_.max     = pParam->m_nMaxProgress;

      // Pre-Fetch
      if (steam_ctx.UserStats ())
      {   steam_ctx.UserStats ()->GetAchievementIcon (
            pParam->m_rgchAchievementName            );
      }

      if ( pParam->m_nMaxProgress == pParam->m_nCurProgress )
      {
        // Do this so that 0/0 does not come back as NaN
        achievement->progress_.current = 1;
        achievement->progress_.max     = 1;

        achievement->unlocked_ = true;

        if (config.steam.achievements.play_sound)
        {
          SK_PlaySound ( (LPCWSTR)unlock_sound, nullptr, SND_ASYNC |
                                                         SND_MEMORY );
        }

        steam_log->Log ( L" Achievement: '%hs' (%hs) - Unlocked!",
                           achievement->human_name_, achievement->desc_ );

        // If the user wants a screenshot, but no popups (why?!), this is when
        //   the screenshot needs to be taken.
        if (       config.steam.achievements.take_screenshot )
        {  if ( (! config.steam.achievements.popup.show) ||
                (! config.cegui.enable)                  ||
                (  SK_GetCurrentRenderBackend ().api == SK_RenderAPI::D3D12 ) )
           {
             SK::SteamAPI::TakeScreenshot ();
           }
        }

        // Re-calculate percentage unlocked
        pullStats ();
      }

      else
      {
        const float progress =
          static_cast <float> (pParam->m_nCurProgress) /
          static_cast <float> (pParam->m_nMaxProgress);

        steam_log->Log ( L" Achievement: '%hs' (%hs) - "
                         L"Progress %lu / %lu (%04.01f%%)",
                           achievement->human_name_,
                           achievement->desc_,
                             pParam->m_nCurProgress,
                             pParam->m_nMaxProgress,
                               100.0 * progress );
      }
    }

    else
    {
      steam_log->Log ( L" Got Rogue Achievement Storage: '%hs'",
                      pParam->m_rgchAchievementName );
    }

    if ( config.cegui.enable                  &&
         config.steam.achievements.popup.show &&
         config.cegui.frames_drawn > 0        &&
                       achievement != nullptr    )
    {
      CEGUI::System* pSys =
        CEGUI::System::getDllSingletonPtr ();

      if ( pSys                 != nullptr &&
           pSys->getRenderer () != nullptr )
      {
        if (steam_popup_cs != nullptr)
            steam_popup_cs->lock ();

        try
        {
          SK_AchievementPopup popup = { };

          // Little bit of sanity goes a long way
          if (achievement->progress_.current ==
              achievement->progress_.max)
          {
            // It's implicit
            achievement->unlocked_ = true;
          }

          popup.window      = nullptr;
          popup.final_pos   = false;
          popup.time        = SK_timeGetTime ();
          popup.achievement = achievement;

          popups.push_back (popup);
        }

        catch (const CEGUI::GenericException&)
        {
        }

        if (steam_popup_cs != nullptr)
            steam_popup_cs->unlock ();
      }
    }
  }

  void clearPopups (void)
  {
    if (steam_popup_cs != nullptr)
        steam_popup_cs->lock ();

    if (popups.empty ())
    {
      if (steam_popup_cs != nullptr)
          steam_popup_cs->unlock ();
      return;
    }

    popups.clear ();

    if (steam_popup_cs != nullptr)
        steam_popup_cs->unlock ();
  }

  int drawPopups (void)
  {
    if (! ( config.cegui.enable &&
            config.cegui.frames_drawn > 0))
    {
      return 0;
    }


    int drawn = 0;

    if (steam_popup_cs != nullptr)
        steam_popup_cs->lock ();

    if (popups.empty ())
    {
      if (steam_popup_cs != nullptr)
          steam_popup_cs->unlock ();
      return drawn;
    }

    //
    // We don't need this, we always do this from the render thread.
    //
    {
      try
      {
        auto& DisplaySize =
          CEGUI::System::getDllSingleton ().getRenderer ()->
                          getDisplaySize ();

        // If true, we need to redraw all text overlays to prevent flickering
        bool removed = false;
        bool created = false;

#define POPUP_DURATION_MS config.steam.achievements.popup.duration

        auto it =
          popups.begin ();

        float inset =
          config.steam.achievements.popup.inset;

        if (inset < 0.0001f)  inset = 0.0f;

        const float full_ht =
          DisplaySize.d_height * (1.0f - inset);

        const float full_wd = DisplaySize.d_width * (1.0f - inset);

        float x_origin, y_origin,
          x_dir,    y_dir;

        CEGUI::Window* first = it->window;

        const float win_ht0 = ( first != nullptr ?
          (it->window->getPixelSize ().d_height) : 0.0f );
        const float win_wd0 = ( first != nullptr ?
           (it->window->getPixelSize ().d_width) : 0.0f );

        const float title_wd =
          DisplaySize.d_width * (1.0f - 2.0f * inset);

        const float title_ht =
          DisplaySize.d_height * (1.0f - 2.0f * inset);

        float fract_x = 0.0f,
              fract_y = 0.0f;

        modf (title_wd / win_wd0, &fract_x);
        modf (title_ht / win_ht0, &fract_y);

        float x_off = full_wd / (4.0f * fract_x);
        float y_off = full_ht / (4.0f * fract_y);

        // 0.0 == Special Case: Flush With Anchor Point
        if (inset == 0.0f)
        {
          x_off = 0.000001f;
          y_off = 0.000001f;
        }

        switch (config.steam.achievements.popup.origin)
        {
          default:
          case 0:
            x_origin =        inset;                          x_dir = 1.0f;
            y_origin =        inset;                          y_dir = 1.0f;
            break;
          case 1:
            x_origin = 1.0f - inset - (win_wd0 / full_wd);    x_dir = -1.0f;
            y_origin =        inset;                          y_dir =  1.0f;
            break;
          case 2:
            x_origin =        inset;                          x_dir =  1.0f;
            y_origin = 1.0f - inset - (win_ht0 / full_ht);    y_dir = -1.0f;
            break;
          case 3:
            x_origin = 1.0f - inset - (win_wd0 / full_wd);    x_dir = -1.0f;
            y_origin = 1.0f - inset - (win_ht0 / full_ht);    y_dir = -1.0f;
            break;
        }

        CEGUI::UDim x_pos (x_origin, x_off * x_dir);
        CEGUI::UDim y_pos (y_origin, y_off * y_dir);

        static int take_screenshot = 0;

        while (it != popups.cend ())
        {
          if (SK_timeGetTime () < (*it).time + POPUP_DURATION_MS)
          {
            float percent_of_lifetime =
              ( static_cast <float> ((*it).time + POPUP_DURATION_MS - SK_timeGetTime ()) /
                static_cast <float> (             POPUP_DURATION_MS)                     );

            //if (SK_PopupManager::getInstance ()->isPopup ((*it).window)) {
            CEGUI::Window* win = (*it).window;

            if (win == nullptr)
            {
              win     = createPopupWindow (&*it);
              created = true;
            }

            const float win_ht =
              win->getPixelSize ().d_height;

            const float win_wd =
              win->getPixelSize ().d_width;

            CEGUI::UVector2 win_pos (x_pos, y_pos);

            float bottom_y = win_pos.d_y.d_scale * full_ht +
                             win_pos.d_y.d_offset          +
                             win_ht;

            // The bottom of the window would be off-screen, so
            //   move it to the top and offset by the width of
            //     each popup.
            if ( bottom_y > full_ht || bottom_y < win_ht0 )
            {
              y_pos  = CEGUI::UDim (y_origin, y_off * y_dir);
              x_pos += x_dir * win->getSize ().d_width;

              win_pos   = (CEGUI::UVector2 (x_pos, y_pos));
            }

            float right_x = win_pos.d_x.d_scale * full_wd +
                            win_pos.d_x.d_offset          +
                            win_wd;

            // Window is off-screen horizontally AND vertically
            if ( inset != 0.0f && (right_x > full_wd || right_x < win_wd0)  )
            {
              // Since we're not going to draw this, treat it as a new
              //   popup until it first becomes visible.
              (*it).time =
                SK_timeGetTime ();
              win->hide        ();
            }

            else
            {
              if (config.steam.achievements.popup.animate)
              {
                CEGUI::UDim percent (
                  CEGUI::UDim (y_origin, y_off).percent ()
                );

                if (percent_of_lifetime <= 0.1f)
                {
                  win_pos.d_y /= (percent * 100.0f *
                                  CEGUI::UDim (percent_of_lifetime / 0.1f, 0.0f));
                }

                else if (percent_of_lifetime >= 0.9f)
                {
                  win_pos.d_y /= (percent * 100.0f *
                                  CEGUI::UDim ( (1.0f -
                                  (percent_of_lifetime - 0.9f) / 0.1f),
                                  0.0f ));
                }

                else if (! it->final_pos)
                {
                  take_screenshot = it->achievement->unlocked_ ? 2 : take_screenshot;
                  it->final_pos   = true;
                }
              }

              else if (! it->final_pos)
              {
                take_screenshot = it->achievement->unlocked_ ? 2 : take_screenshot;
                it->final_pos   = true;
              }

              win->show        ();
              win->setPosition (win_pos);
            }

            y_pos += y_dir * win->getSize ().d_height;

            ++it;
            //} else {
            //it = popups.erase (it);
            //}
          }

          else
          {
            //if (SK_PopupManager::getInstance ()->isPopup ((*it).window)) {
            CEGUI::Window* win = (*it).window;

            CEGUI::WindowManager::getDllSingleton ().destroyWindow (win);

            removed = true;
            //SK_PopupManager::getInstance ()->destroyPopup ((*it).window);
            //}

            it = popups.erase (it);
          }
        }

        // Invalidate text overlays any time a window is removed,
        //   this prevents flicker.
        if (removed || created || take_screenshot > 0)
        {
          SK_TextOverlayManager::getInstance ()->drawAllOverlays     (0.0f, 0.0f, true);
            CEGUI::System::getDllSingleton   ().renderAllGUIContexts ();
        }

        // Popup is in the final location, so now is when screenshots
        //   need to be taken.
        if (config.steam.achievements.take_screenshot && take_screenshot > 0)
        {
          // Delay the screenshot so it doesn't show up twice
          --take_screenshot;

          if (! take_screenshot)
          {
            SK::SteamAPI::TakeScreenshot (SK_ScreenshotStage::EndOfFrame);
            take_screenshot = -1;
          }
        }

        ++drawn;
      }

      catch (const CEGUI::GenericException&) {}
    }
    if (steam_popup_cs != nullptr)
        steam_popup_cs->unlock ();

    return drawn;
  }

  float getPercentOfAchievementsUnlocked (void) const
  {
    return percent_unlocked;
  }

  Achievement* getAchievement (const char* szName) const
  {
    if (achievements.string_map.count (szName))
      return achievements.string_map.at (szName);

    return nullptr;
  }

  SK_SteamAchievement** getAchievements (size_t* pnAchievements = nullptr)
  {
    ISteamUserStats* stats = steam_ctx.UserStats       ();
    size_t           count = stats->GetNumAchievements ();

    if (pnAchievements != nullptr)
      *pnAchievements = count;

    static std::set    <SK_SteamAchievement *> achievement_set;
    static std::vector <SK_SteamAchievement *> achievement_data;

    if (achievement_set.size () != count)
    {
      for (size_t i = 0; i < count; i++)
      {
        if (! achievement_set.count (achievements.list [i]))
        {
          achievement_set.emplace    (achievements.list [i]);
          achievement_data.push_back (achievements.list [i]);
        }
      }
    }

    return achievement_data.data ();
  }

  void requestStats (void)
  {
    auto pUserStats =
      steam_ctx.UserStats ();

    if (pUserStats != nullptr)
    {
      if (pUserStats->RequestCurrentStats ())
      {
        SteamAPICall_t hCall =
          pUserStats->RequestUserStats (
            steam_ctx.User ()->GetSteamID ()
          );

        self_listener.Set ( hCall,
          this,
            &SK_Steam_AchievementManager::OnRecvSelfStats
        );
      }
    }
  }

  // FIXME: Poorly named given the existence of requestStats
  //
  //   (nb: triggered when the async Steam call in requestStats returns)
  void pullStats (void)
  {
    ISteamUserStats* stats = steam_ctx.UserStats ();

    if (stats)
    {
      int unlocked = 0;

      for (uint32 i = 0; i < stats->GetNumAchievements (); i++)
      {
        //steam_log->Log (L"  >> %hs", stats->GetAchievementName (i));

        auto& achievement =
          achievements.list [i];

        if (achievement == nullptr)
        {
          achievement =
            new Achievement (i,
              stats->GetAchievementName (i),
              stats
            );

          achievements.string_map [achievement->name_] =
                                   achievement;
                                   achievement->update (stats);

          //steam_log->Log (L"  >> (%lu) %hs", i, achievements.list [i]->name_);

          // Start pre-loading images so we do not hitch on achievement unlock...

          //if (! achievements.list [i]->unlocked_)
          //stats->SetAchievement   (szName);

          // After setting the achievement, fetch the icon -- this is
          //   necessary so that the unlock dialog does not display
          //     the locked icon.
          if ( steam_ctx.UserStats () )
          {    steam_ctx.UserStats ()->GetAchievementIcon (
                 achievement->name_
               );
          }

          //if (! achievements.list [i]->unlocked_)
          //stats->ClearAchievement (szName);
        }

        else
        {
          achievement->update (stats);
        }

        if (achievement->unlocked_)
          unlocked++;
      }

      percent_unlocked =
        ( static_cast <float> (unlocked) /
          static_cast <float> (stats->GetNumAchievements ()) );
    }
  }

  void
  loadSound (const wchar_t* wszUnlockSound)
  {
    bool xbox = false,
         psn  = false,
         dt   = false;

    wchar_t wszFileName [MAX_PATH + 2] = { };

    extern iSK_INI* steam_ini;

    if (*wszUnlockSound == L'\0' && steam_ini != nullptr)
    {
      // If the config file is empty, establish defaults and then write it.
      if (steam_ini->get_sections ().empty ())
      {
        steam_ini->import ( L"[Steam.Achievements]\n"
                            L"SoundFile=psn\n"
                            L"PlaySound=true\n"
                            L"TakeScreenshot=false\n"
                            L"AnimatePopup=true\n"
                            L"NotifyCorner=0\n" );

        steam_ini->write ();
      }

      if (steam_ini->contains_section (L"Steam.Achievements"))
      {
        iSK_INISection& sec =
          steam_ini->get_section (L"Steam.Achievements");

        if (sec.contains_key (L"SoundFile"))
        {
          wcsncpy_s ( wszFileName,   MAX_PATH,
                      sec.get_value (L"SoundFile").c_str (),
                                     _TRUNCATE );
        }
      }
    }

    else
    {
      wcsncpy_s ( wszFileName,    MAX_PATH,
                  wszUnlockSound, _TRUNCATE );
    }

    if ((!      _wcsnicmp (wszFileName, L"psn", MAX_PATH)))
      psn  = true;
    else if  (! _wcsnicmp (wszFileName, L"xbox", MAX_PATH))
      xbox = true;
    else if ((! _wcsnicmp (wszFileName, L"dream_theater", MAX_PATH)))
      dt   = true;

    FILE *fWAV = nullptr;

    if ( (!  psn) &&
         (! xbox) &&
         (!   dt) && (fWAV = _wfopen (wszFileName, L"rb")) != nullptr )
    {
      SK_ConcealUserDir (wszFileName);

      steam_log->LogEx ( true,
                           L"  >> Loading Achievement Unlock Sound: '%s'...",
                             wszFileName );

                  fseek (fWAV, 0, SEEK_END);
      long size = ftell (fWAV);
                 rewind (fWAV);

      unlock_sound =
        static_cast <uint8_t *> (
          malloc (size)
        );

      if (unlock_sound != nullptr)
        fread  (unlock_sound, size, 1, fWAV);

      fclose (fWAV);

      steam_log->LogEx (false, L" %d bytes\n", size);

      default_loaded = false;
    }

    else
    {
      // Default to PSN if not specified
      if ((! psn) && (! xbox) && (! dt))
        psn = true;

      steam_log->Log ( L"  * Loading Built-In Achievement Unlock Sound: '%s'",
                         wszFileName );

      HRSRC default_sound = nullptr;

      if (psn)
        default_sound =
        FindResource (SK_GetDLL (), MAKEINTRESOURCE (IDR_TROPHY),        L"WAVE");
      else if (xbox)
        default_sound =
        FindResource (SK_GetDLL (), MAKEINTRESOURCE (IDR_XBOX),          L"WAVE");
      else
        default_sound =
        FindResource (SK_GetDLL (), MAKEINTRESOURCE (IDR_DREAM_THEATER), L"WAVE");

      if (default_sound != nullptr)
      {
        HGLOBAL sound_ref     =
          LoadResource (SK_GetDLL (), default_sound);

        if (sound_ref != nullptr)
        {
          unlock_sound        =
            static_cast <uint8_t *> (LockResource (sound_ref));

          default_loaded = true;
        }
      }
    }
  }

  const std::string& getFriendName (uint32_t friend_idx) const
  {
    static const std::string dummy_ = "";

    if ( (int32_t)friend_idx >= 0 &&
                  friend_idx < friend_stats.size () )
    {
      return
        friend_stats [friend_idx].name;
    }

    return dummy_;
  }

  float getFriendUnlockPercentage (uint32_t friend_idx) const
  {
    if ( (int32_t)friend_idx >= 0 &&
                  friend_idx < friend_stats.size () )
    {
      return
        friend_stats [friend_idx].percent_unlocked;
    }

    return 0.0f;
  }

  bool hasFriendUnlockedAchievement (uint32_t friend_idx, uint32_t achv_idx) const
  {
    if ( (int32_t)friend_idx >= 0 &&
                  friend_idx < friend_stats.size () &&
         static_cast <int32_t> (achv_idx) >= 0      &&
                friend_stats [friend_idx].unlocked.size () > achv_idx )
    {
      return
        friend_stats [friend_idx].unlocked [achv_idx];
    }

    return false;
  }

protected:
  CEGUI::Window* createPopupWindow (SK_AchievementPopup* popup)
  {
    if (! (config.cegui.enable && config.cegui.frames_drawn > 0)) return nullptr;

    if (popup->achievement == nullptr)
      return nullptr;

    CEGUI::System* pSys =
      CEGUI::System::getDllSingletonPtr ();

    extern CEGUI::Window* SK_achv_popup;

    char     szPopupName [32] = { };
    sprintf (szPopupName, "Achievement_%i", lifetime_popups++);

    popup->window              = SK_achv_popup->clone (true);
    Achievement*   achievement = popup->achievement;
    CEGUI::Window* achv_popup  = popup->window;

    assert (achievement != nullptr);

    achv_popup->setName (szPopupName);

    CEGUI::Window* achv_title  = achv_popup->getChild ("Title");
    achv_title->setText ((const CEGUI::utf8 *)achievement->human_name_);

    CEGUI::Window* achv_desc = achv_popup->getChild ("Description");
    achv_desc->setText ((const CEGUI::utf8 *)achievement->desc_);

    CEGUI::Window* achv_rank = achv_popup->getChild ("Rank");
    achv_rank->setProperty ( "NormalTextColour",
                            SK_Steam_RarityToColor (achievement->global_percent_).c_str ()
    );
    achv_rank->setText ( SK_Steam_RarityToName (achievement->global_percent_) );

    CEGUI::Window* achv_global = achv_popup->getChild ("GlobalRarity");

    char       szGlobal [32] = { };
    snprintf ( szGlobal, 32,
                 "Global: %6.2f%%",
                   achievement->global_percent_ );
    achv_global->setText (szGlobal);

    CEGUI::Window* achv_friend =
      achv_popup->getChild ("FriendRarity");

    char       szFriend [32] = { };
    snprintf ( szFriend, 32,
                 "Friends: %6.2f%%",
                   100.0 * ( (double)          achievement->friends_.unlocked /
                             (double)std::max (achievement->friends_.possible, 1) )
             );
    achv_friend->setText (szFriend);


    // If the server hasn't updated the unlock time, just use the current time
    if (achievement->time_ == 0)
      _time32 (&achievement->time_);

    CEGUI::Window* achv_unlock  = achv_popup->getChild ("UnlockTime");
    CEGUI::Window* achv_percent = achv_popup->getChild ("ProgressBar");

    float progress =
      achievement->progress_.getPercent ();

    char szUnlockTime [128] = { };
    if (progress == 100.0f)
    {
      snprintf ( szUnlockTime, 128,
                   "Unlocked: %s", _ctime32 (&achievement->time_) );

      achv_percent->setProperty ( "CurrentProgress", "1.0" );
    }

    else
    {
      snprintf ( szUnlockTime, 16,
                   "%5.4f",
                     progress / 100.0
      );

      achv_percent->setProperty ( "CurrentProgress", szUnlockTime );

      snprintf ( szUnlockTime, 128,
                   "Current Progress: %li/%li (%6.2f%%)",
                     achievement->progress_.current,
                     achievement->progress_.max,
                                  progress
      );
    }
    achv_unlock->setText (szUnlockTime);

    auto pUserStats = steam_ctx.UserStats ();
    auto pUtils = steam_ctx.Utils ();

    int icon_idx =
      ( pUserStats != nullptr ?
        pUserStats->GetAchievementIcon (achievement->name_) :
                              0 );

    // Icon width and height
    uint32 w = 0,
      h = 0;

    if (icon_idx != 0)
    {
      if (pUtils != nullptr)
          pUtils->GetImageSize (icon_idx, &w, &h);

      int tries = 1;

      while (achievement->icons_.achieved == nullptr && tries < 8)
      {
        achievement->icons_.achieved =
          (uint8_t *)_aligned_malloc (static_cast <size_t> (4) * w * h, 16);

        if (pUtils != nullptr)
        {
          if ( ! pUtils->GetImageRGBA (
                   icon_idx,
                     achievement->icons_.achieved,
                       ( 4 * w * h )
                 )
             )
          {
            _aligned_free (achievement->icons_.achieved);
                           achievement->icons_.achieved = nullptr;

            ++tries;
          }

          else
          {
            steam_log->Log ( L" * Fetched RGBA Icon (idx=%li) for Achievement: '%hs'  (%lux%lu) "
                             L"{ Took %li try(s) }",
                               icon_idx, achievement->name_, w, h, tries
            );
          }
        }
      }
    }

    if (achievement->icons_.achieved != nullptr)
    {
      bool exists =
        CEGUI::ImageManager::getDllSingleton ().isDefined (achievement->name_);

      CEGUI::Image& img =
        ( exists ?
            CEGUI::ImageManager::getDllSingleton ().get    (              achievement->name_) :
            CEGUI::ImageManager::getDllSingleton ().create ("BasicImage", achievement->name_) );

      if (! exists) try
      {
        /* StaticImage */
        CEGUI::Texture& Tex =
          pSys->getRenderer ()->createTexture (achievement->name_);

        Tex.loadFromMemory ( achievement->icons_.achieved,
                               CEGUI::Sizef ( static_cast <float> (w),
                                              static_cast <float> (h) ),
                               CEGUI::Texture::PF_RGBA );

        ((CEGUI::BasicImage &)img).setTexture (&Tex);

        const CEGUI::Rectf rect (CEGUI::Vector2f (0.0f, 0.0f), Tex.getOriginalDataSize ());

        ((CEGUI::BasicImage &)img).setArea       (rect);
        ((CEGUI::BasicImage &)img).setAutoScaled (CEGUI::ASM_Both);
      }

      catch (const CEGUI::GenericException&)
      {
      }

      try
      {
        CEGUI::Window* staticImage =
          achv_popup->getChild ("Icon");

        staticImage->setProperty ( "Image",
                                     achievement->name_ );
      }

      catch (const CEGUI::GenericException&)
      {
      }
    }

    if (config.steam.achievements.popup.show_title)
    {
      std::string app_name = SK::SteamAPI::AppName ();

      if (! app_name.empty ())
      {
        achv_popup->setText (
          (const CEGUI::utf8 *)app_name.c_str ()
        );
      }
    }

    CEGUI::System::getDllSingleton ().
      getDefaultGUIContext ().
             getRootWindow ()->
                  addChild (popup->window);

    return achv_popup;
  }

private:
  struct SK_AchievementStorage
  {
    std::vector        <             Achievement*> list;
    std::unordered_map <std::string, Achievement*> string_map;
  } achievements; // SELF

  struct friend_s {
    std::string        name             =   "";
    float              percent_unlocked = 0.0f;
    std::vector <BOOL> unlocked         = {  };
    uint64_t           account_id       =    0;
  };

  std::vector <friend_s>
    friend_stats;

  std::unordered_map <uint64_t, uint32_t>
    friend_sid_to_idx;

  float                             percent_unlocked;

  std::vector <SK_AchievementPopup> popups;
  int                               lifetime_popups;

  SteamAPICall_t global_request = { };
  SteamAPICall_t stats_request  = { };

  bool           default_loaded = false;
  uint8_t*       unlock_sound   = nullptr;   // A .WAV (PCM) file
};

static std::unique_ptr <SK_Steam_AchievementManager> steam_achievements = nullptr;
static std::unique_ptr <SK_Steam_ScreenshotManager>  steam_screenshots  = nullptr;

void
SK_Steam_LoadUnlockSound (const wchar_t* wszUnlockSound)
{
  if (steam_achievements != nullptr)
      steam_achievements->loadSound (wszUnlockSound);
}

void
SK_Steam_LogAllAchievements (void)
{
  if (steam_achievements != nullptr)
      steam_achievements->log_all_achievements ();
}

void
SK_Steam_UnlockAchievement (uint32_t idx)
{
  if (config.steam.silent)
    return;

  if (! steam_achievements)
  {
    steam_log->Log ( L" >> Steam Achievement Manager Not Yet Initialized ... "
                     L"Skipping Unlock <<" );
    return;
  }

  steam_log->LogEx (true, L" >> Attempting to Unlock Achievement: %lu... ",
                    idx );

  ISteamUserStats* stats = steam_ctx.UserStats ();

  if ( stats                        && idx <
       stats->GetNumAchievements () )
  {
    const std::string name =
      stats->GetAchievementName (idx);

    if (! name.empty ())
    {
      steam_log->LogEx (false, L" (%hs - Found)\n", name.c_str ());

      steam_achievements->pullStats ();

      UserAchievementStored_t store;
      store.m_nCurProgress = 0;
      store.m_nMaxProgress = 0;
      store.m_nGameID      = CGameID (SK::SteamAPI::AppID ()).ToUint64 ();
      strncpy (store.m_rgchAchievementName, name.c_str (), 128);

      SK_Steam_AchievementManager::Achievement* achievement =
        steam_achievements->getAchievement (
          store.m_rgchAchievementName
        );

      UNREFERENCED_PARAMETER (achievement);

      steam_achievements->OnUnlock (&store);

      //      stats->ClearAchievement            (szName);
      //      stats->IndicateAchievementProgress (szName, 0, 1);
      //      stats->StoreStats                  ();
    }

    else
      steam_log->LogEx (false, L" (None Found)\n");
  }

  else
    steam_log->LogEx   (false, L" (ISteamUserStats is NULL?!)\n");

  SK_Steam_LogAllAchievements ();
}

void
SK_Steam_UpdateGlobalAchievements (void)
{
  ISteamUserStats* stats =
    steam_ctx.UserStats ();

  if (stats != nullptr && steam_achievements != nullptr)
  {
    const int num_achievements = stats->GetNumAchievements ();

    for (int i = 0; i < num_achievements; i++)
    {
      std::string szName (stats->GetAchievementName (i));

      SK_Steam_AchievementManager::Achievement* achievement =
        steam_achievements->getAchievement (szName.c_str ());

      if (achievement != nullptr)
        achievement->update_global (stats);

      else
      {
        dll_log->Log ( L" Got Global Data For Unknown Achievement ('%hs')",
                      szName.c_str () );
      }
    }
  }

  else
  {
    steam_log->Log ( L"Got Global Stats from SteamAPI, but no "
                     L"ISteamUserStats interface exists?!" );
  }

  if (steam_achievements)
    SK_Steam_LogAllAchievements ();
}


void TryRunCallbacksSEH (void)
{
  __try {
    if (SteamAPI_RunCallbacks_Original != nullptr)
        SteamAPI_RunCallbacks_Original ();
  }
  __finally {
  }
}

void TryRunCallbacks (void)
{
  auto orig_se =
  SK_SEH_ApplyTranslator (
    SK_BasicStructuredExceptionTranslator
  );
  try {
    TryRunCallbacksSEH ();
  }
  catch (const SK_SEH_IgnoredException&)
  {
  }
  SK_SEH_RemoveTranslator (orig_se);
}


void
SK_Steam_ClearPopups (void)
{
  if (steam_achievements != nullptr)
  {
    steam_achievements->clearPopups ();
                    TryRunCallbacks ();
  }
}

class SK_Steam_UserManager
{
public:
   SK_Steam_UserManager (void) = default;
  ~SK_Steam_UserManager (void) = default;

  int32_t  GetNumPlayers    (void) { return ReadAcquire (&num_players_); }
  void     UpdateNumPlayers (void)
  {
    // Service Only One Update
    if (! InterlockedCompareExchange (&num_players_call_, 1, 0))
    {
      ISteamUserStats* user_stats =
        steam_ctx.UserStats ();

      if (user_stats != nullptr)
      {
        InterlockedExchange ( &num_players_call_,
                               user_stats->GetNumberOfCurrentPlayers () );

        current_players_call.Set (
          InterlockedAdd64 ((LONG64 *)&num_players_call_, 0LL),
            this,
              &SK_Steam_UserManager::OnRecvNumCurrentPlayers
        );
      }
    }
  }

protected:
  STEAM_CALLRESULT ( SK_Steam_UserManager,
                     OnRecvNumCurrentPlayers,
                     NumberOfCurrentPlayers_t,
                     current_players_call )
  {
    current_players_call.Cancel ();

    if (pParam->m_bSuccess)
      InterlockedExchange (&num_players_, pParam->m_cPlayers);

    InterlockedExchange (&num_players_call_, 0);
  }

private:
  // Can be updated on-request
  volatile LONG           num_players_      = 1;
  volatile SteamAPICall_t num_players_call_ = 0;
};

static std::unique_ptr <SK_Steam_UserManager> user_manager = nullptr;


int
SK_Steam_DrawOSD (void)
{
  if (steam_achievements != nullptr)
  {
    return
      steam_achievements->drawPopups ();
  }

  return 0;
}

bool
SK_Steam_ShouldThrottleCallbacks (void)
{
  if (ReadAcquire (&__SK_Steam_init))
  {
    InterlockedIncrement64 (&SK_SteamAPI_CallbackRunCount);
    // Might want to skip the callbacks; one call every 1 ms is enough!!

    static LARGE_INTEGER liLastCallbacked = { 0LL, 0LL };

    LONG limit = ReadAcquire (&SK_SteamAPI_CallbackRateLimit);
    if  (limit < 0) return false;

    if ( limit == 0 ||
      ( SK_CurrentPerf ().QuadPart - liLastCallbacked.QuadPart  <
                        SK_QpcFreq / limit ) )
    {
      InterlockedDecrement64 ( &SK_SteamAPI_CallbackRunCount );
      return true;
    }

    liLastCallbacked = SK_CurrentPerf ();
  }

  return false;
}

void
S_CALLTYPE
SteamAPI_RunCallbacks_Detour (void)
{
  if (ReadAcquire (&__SK_DLL_Ending))
  {
    if (SteamAPI_RunCallbacks_Original != nullptr)
        TryRunCallbacks ();
    return;
  }

  static bool first   =  true;
  static bool failure = false;

  if (SK_Steam_ShouldThrottleCallbacks ())
  {
    return;
  }

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  if (pTLS != nullptr)
    InterlockedIncrement64 (&pTLS->steam->callback_count);

  if ( (! failure) && steam_achievements == nullptr )
  {
    // Handle situations where Steam was initialized earlier than
    //   expected...
    void SK_SteamAPI_InitManagers (void);
         SK_SteamAPI_InitManagers (    );

    strncpy_s ( steam_ctx.var_strings.popup_origin, 16,
                  SK_Steam_PopupOriginToStr (
                    config.steam.achievements.popup.origin
                  ), _TRUNCATE
              );

    strncpy_s ( steam_ctx.var_strings.notify_corner, 16,
                  SK_Steam_PopupOriginToStr (
                    config.steam.notify_corner
                  ), _TRUNCATE
              );

    if (! steam_ctx.UserStats ())
    {
      if (SteamAPI_InitSafe_Original != nullptr)
          SteamAPI_InitSafe_Detour ();
    }
  }

  static bool fetch_stats = true;

  if (fetch_stats)
  {
    auto orig_se =
    SK_SEH_ApplyTranslator (
      SK_FilteringStructuredExceptionTranslator (
        EXCEPTION_ACCESS_VIOLATION
      )
    );
    try
    {
      if (ReadAcquire64 (&SK_SteamAPI_CallbackRunCount) > 24)
      {
        SK_Steam_SetNotifyCorner ();

        ISteamUserStats* pStats =
          steam_ctx.UserStats ();

        if (! pStats)
        {
          failure = true;
        }

        if (pStats && (steam_achievements != nullptr))
        {
          steam_achievements->requestStats            ();
          pStats->RequestGlobalAchievementPercentages ();
        }

        TryRunCallbacks ();

        fetch_stats = false;
      }

      else
        TryRunCallbacks ();
    }

    catch (const SK_SEH_IgnoredException&)
    {
      fetch_stats = false;
      failure     = true;
      steam_log->Log (L" Caught a Structured Exception while running Steam"
                      L" Callbacks!");
    }
    SK_SEH_RemoveTranslator (orig_se);

    return;
  }

  if (! ReadAcquire (&__SK_DLL_Ending))
  {
    static bool try_me = true;

    auto orig_se =
    SK_SEH_ApplyTranslator (
      SK_FilteringStructuredExceptionTranslator(EXCEPTION_ACCESS_VIOLATION)
    );
    try
    {
      if (try_me)
      {
        DWORD dwNow = SK_timeGetTime ();

        static DWORD
            dwLastOnlineStateCheck = 0UL;
        if (dwLastOnlineStateCheck < dwNow - 666UL)
        {
          const auto cfg_online_status =
            config.steam.online_status;

          if ( cfg_online_status != -1 &&
               cfg_online_status != SK::SteamAPI::GetPersonaState () )
          {
            SK::SteamAPI::SetPersonaState (
              (EPersonaState)config.steam.online_status
            );
          }

          dwLastOnlineStateCheck = dwNow;
        }
      }
    }

    catch (const SK_SEH_IgnoredException&)
    {
      try_me = false;
    }
    SK_SEH_RemoveTranslator (orig_se);


    TryRunCallbacks ();
  }
}


#define STEAMAPI_CALL1(x,y,z) ((x) = SteamAPI_##y z)
#define STEAMAPI_CALL0(y,z)   (SteamAPI_##y z)



// TODO: Remove
void
SK::SteamAPI::Init (bool pre_load)
{
  UNREFERENCED_PARAMETER (pre_load);

  if (config.steam.silent)
    return;
}

void
SK::SteamAPI::Shutdown (void)
{
  // Since we might restart SteamAPI, don't do this.
  //SK_AutoClose_Log (steam_log);

  // This sometimes snags in weird ways, so
  //   ignore any exceptions it causes...
  __try
  {
    steam_ctx.Shutdown ();
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
  }
}

void SK::SteamAPI::Pump (void)
{
    if (steam_ctx.UserStats ())
  {
    if (SteamAPI_RunCallbacks_Original != nullptr)
        SteamAPI_RunCallbacks_Detour ();
  }
}

volatile HANDLE hSteamPump     = nullptr;
static   HANDLE hSteamPumpKill = INVALID_HANDLE_VALUE;

DWORD
WINAPI
SteamAPI_PumpThread (LPVOID user)
{
  auto _Terminate = [&]
  {
    InterlockedExchangePointer ((void **)&hSteamPump, nullptr);

    SK_Thread_CloseSelf ();

    return 0;
  };

  auto _TerminateOnSignal = [&](DWORD dwWaitState)
  {
    if (dwWaitState != WAIT_TIMEOUT)
    {
      _Terminate ();

      return true;
    }

    return false;
  };

  if (SK_GetCurrentGameID () == SK_GAME_ID::FinalFantasyXV)
  {
    return
      _Terminate ();
  }

  SetCurrentThreadDescription (               L"[SK] SteamAPI Callback Pump" );
  SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_IDLE );

  bool   start_immediately = (user != nullptr);
  double callback_freq     =  0.0;

  if ( SK_GetCurrentGameID () == SK_GAME_ID::MonsterHunterWorld ||
       SK_GetCurrentGameID () == SK_GAME_ID::JustCause3         ||
       SK_GetCurrentGameID () == SK_GAME_ID::YakuzaKiwami2      ||
       SK_GetCurrentGameID () == SK_GAME_ID::YakuzaUnderflow    ||
       GetModuleHandleW (
         SK_RunLHIfBitness (64, L"kaldaien_api64.dll",
                                L"kaldaien_api.dll" )
       ) != nullptr
     )
  {
    start_immediately = true;
  }

  if (! start_immediately)
  {
    // Wait 5 seconds, then begin a timing investigation
    DWORD dwWaitState =
      WaitForMultipleObjects ( 2,
           std::array <HANDLE, 2> { hSteamPumpKill,
                            __SK_DLL_TeardownEvent }.data (),
                              FALSE, 5000 );

    if (_TerminateOnSignal (dwWaitState))
      return 0;

    // First, begin a timing probe.
    //
    //   If after 30 seconds the game has not called SteamAPI_RunCallbacks
    //     frequently enough, switch the thread to auto-pump mode.

    const UINT TEST_PERIOD = 180;

    LONGLONG callback_count0 = SK_SteamAPI_CallbackRunCount;

    dwWaitState =
      WaitForMultipleObjects ( 2,
           std::array <HANDLE, 2> { hSteamPumpKill,
                            __SK_DLL_TeardownEvent }.data (),
                              FALSE,
              TEST_PERIOD * 1000UL );

    if (_TerminateOnSignal (dwWaitState))
      return 0;

    LONGLONG callback_count1 = SK_SteamAPI_CallbackRunCount;

    callback_freq = (double)(callback_count1 - callback_count0) / (double)TEST_PERIOD;

    steam_log->Log ( L"Game runs its callbacks at ~%5.2f Hz ... this is %s the "
                     L"minimum threshold for achievement unlock sound.\n",
                     callback_freq, callback_freq > 3.0 ? L"above" : L"below" );
  }

  // If the timing period failed, then start auto-pumping.
  //
  if (callback_freq < 3.0)
  {
    if ( SteamAPI_InitSafe_Original  != nullptr &&
         SteamAPI_InitSafe_Original ()             )
    {
      SetThreadPriority (SK_GetCurrentThread (), THREAD_PRIORITY_LOWEST);

      steam_log->Log ( L" >> Installing a callback auto-pump at 8 Hz.\n\n");

      do
      {
        HANDLE hSignals [] = {
          hSteamPumpKill, __SK_DLL_TeardownEvent
        };

        DWORD dwWait =
          WaitForMultipleObjects (2, hSignals, FALSE, 75);

        if ( dwWait ==  WAIT_OBJECT_0
                ||
             dwWait == (WAIT_OBJECT_0 + 1)
                ||
          ( hSteamPumpKill == INVALID_HANDLE_VALUE ||
            hSteamPumpKill == 0 )
           )
        {
          break;
        }

        SK::SteamAPI::Pump ();
      } while (! ReadAcquire (&__SK_DLL_Ending));
    }

    else
    {
      steam_log->Log ( L" >> Tried to install a callback auto-pump, but "
                      L"could not initialize SteamAPI.\n\n" );
    }
  }

  return
    _Terminate ();
}

void
SK_Steam_StartPump (bool force)
{
  SK_Steam_ForceInputAppId (0);

  if (            ReadAcquire        (&__SK_DLL_Ending)  ||
       nullptr != ReadPointerAcquire ((void **)&hSteamPump) )
  {
    return;
  }

  if (config.steam.auto_pump_callbacks || force)
  {
    if (hSteamPumpKill == INVALID_HANDLE_VALUE)
    {
      hSteamPumpKill =
        SK_CreateEvent (nullptr, FALSE, FALSE, nullptr);
    }

    LPVOID
      start_params = force ?
                 (LPVOID)1 : nullptr;

    InterlockedCompareExchangePointer (
      (void **)&hSteamPump, (HANDLE)
        SK_Thread_CreateEx ( SteamAPI_PumpThread, nullptr,
                             start_params ),
          nullptr
    );
  }
}

void
SK_Steam_KillPump (void)
{
  HANDLE hPumpThread =
    InterlockedExchangePointer (
      (void **)&hSteamPump, nullptr
    );

                                      DWORD dwFlags = 0x0;
  if ((intptr_t)hPumpThread <= 0 ||
     (! GetHandleInformation (hPumpThread, &dwFlags)))
    return;

  if (hSteamPumpKill != INVALID_HANDLE_VALUE)
  {
    SignalObjectAndWait (
      hSteamPumpKill, hPumpThread, 20UL, FALSE
    ); CloseHandle (hSteamPumpKill);
                    hSteamPumpKill = INVALID_HANDLE_VALUE;
  }
}

CSteamID
SK::SteamAPI::UserSteamID (void)
{
  static CSteamID usr_steam_id (0ui64);

  if (usr_steam_id.ConvertToUint64 () != 0ui64)
    return usr_steam_id.ConvertToUint64 ();

  ISteamUser *pUser =
    steam_ctx.User ();

  if (pUser != nullptr)
  {
    usr_steam_id =
      pUser->GetSteamID ();
  }

  return usr_steam_id;
}

LONGLONG
SK::SteamAPI::GetCallbacksRun (void)
{
  return
    ReadAcquire64 (&SK_SteamAPI_CallbackRunCount);
}

uint32_t
SK::SteamAPI::AppID (void)
{
  ISteamUtils* utils =
    steam_ctx.Utils ();

  static char  szSteamGameId [32] = { };
  static DWORD dwSteamGameIdLen   =
    GetEnvironmentVariableA ( "SteamGameId",
                                szSteamGameId,
                                  0x20 );

  if (utils != nullptr || dwSteamGameIdLen > 3)
  {
    static          uint32_t id   = 0;
    static volatile LONG     init = FALSE;

    if (InterlockedCompareExchange (&init, TRUE, FALSE) == FALSE)
    {
      id = ( utils != nullptr   ?
             utils->GetAppID () : atoi (szSteamGameId) );

      if (id != 0)
      {
        if (config.system.central_repository &&
          (! app_cache_mgr->getAppIDFromPath (SK_GetFullyQualifiedApp ())))
        {
          app_cache_mgr->addAppToCache (
            SK_GetFullyQualifiedApp (),
                      SK_GetHostApp (),
                  SK_UTF8ToWideChar (SK_UseManifestToGetAppName (id)).c_str (),
                                                                 id
          );
          app_cache_mgr->saveAppCache       ();//true);
          app_cache_mgr->loadAppCacheForExe (SK_GetFullyQualifiedApp ());

          // Trigger profile migration if necessary
          app_cache_mgr->getConfigPathForAppID (id);
        }
      }

      InterlockedIncrement (&init);
    }

    else
      SK_Thread_SpinUntilAtomicMin (&init, 2);

    // If no AppID was manually set, let's assign one now.
    if (config.steam.appid == 0)
    {
      config.steam.appid = id;
    }

    return id;
  }

  return 0;
}

// Easier to DLL export a flat interface
uint32_t
__stdcall
SK_SteamAPI_AppID (void)
{
  return SK::SteamAPI::AppID ();
}

const wchar_t*
SK_GetSteamDir (void)
{
  static wchar_t wszSteamPath [MAX_PATH + 2] = { };
  DWORD     len    =           MAX_PATH;

  // Return the cached value
  if (*wszSteamPath != L'\0')
    return wszSteamPath;

  LSTATUS   status =
    RegGetValueW ( HKEY_CURRENT_USER,
                     LR"(SOFTWARE\Valve\Steam\)",
                                      L"SteamPath",
                       RRF_RT_REG_SZ,
                         nullptr,
                           wszSteamPath,
                             (LPDWORD)&len );

  if (status == ERROR_SUCCESS)
    return wszSteamPath;

  *wszSteamPath = L'\0';

  return nullptr;
}

HMODULE hModOverlay;

bool
SK_Steam_LoadOverlayEarly (void)
{
  const wchar_t* wszSteamPath =
    SK_GetSteamDir ();

  if (wszSteamPath == nullptr)
    return false;

  wchar_t           wszOverlayDLL [MAX_PATH + 2] = { };
  SK_PathCombineW ( wszOverlayDLL, wszSteamPath,
    SK_RunLHIfBitness ( 64, L"GameOverlayRenderer64.dll",
                            L"GameOverlayRenderer.dll"    ) );


  bool bEnableHooks =
    SK_EnableApplyQueuedHooks ();

  SK_ApplyQueuedHooks ();

  if (! bEnableHooks)
    SK_DisableApplyQueuedHooks ();


  hModOverlay =
    SK_Modules->LoadLibrary (wszOverlayDLL);

  return hModOverlay != nullptr;
}

int
SK_Steam_GetLibraries (steam_library_t** ppLibraries)
{
#define MAX_STEAM_LIBRARIES 16

  static bool            scanned_libs = false;
  static int             steam_libs   = 0;
  static steam_library_t steam_lib_paths [MAX_STEAM_LIBRARIES] = { };

  static const wchar_t* wszSteamPath;

  if (! scanned_libs)
  {
    wszSteamPath =
      SK_GetSteamDir ();

    if (wszSteamPath != nullptr)
    {
      wchar_t wszLibraryFolders [MAX_PATH + 2] = { };

      lstrcpyW (wszLibraryFolders, wszSteamPath);
      lstrcatW (wszLibraryFolders, LR"(\steamapps\libraryfolders.vdf)");

      CHandle hLibFolders (
        CreateFileW ( wszLibraryFolders,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                          nullptr,        OPEN_EXISTING,
                            GetFileAttributesW (wszLibraryFolders),
                              nullptr
                    )
      );

      if (hLibFolders != INVALID_HANDLE_VALUE)
      {
        DWORD dwSizeHigh = 0,
              dwRead     = 0,
              dwSize     =
         GetFileSize (hLibFolders, &dwSizeHigh);

        std::unique_ptr <char []>
          local_data;
        char*   data = nullptr;

        local_data =
          std::make_unique <char []> (dwSize + 4u);
              data = local_data.get ();

        if (data == nullptr)
          return steam_libs;

        dwRead = dwSize;

        if (ReadFile (hLibFolders, data, dwSize, &dwRead, nullptr))
        {
          data [dwSize] = '\0';

          for (int i = 1; i < MAX_STEAM_LIBRARIES - 1; i++)
          {
            // Old libraryfolders.vdf format
            std::wstring lib_path =
              SK_Steam_KeyValues::getValueAsUTF16 (
                data, { "LibraryFolders" }, std::to_string (i)
              );

            if (lib_path.empty ())
            {
              // New (July 2021) libraryfolders.vdf format
              lib_path =
                SK_Steam_KeyValues::getValueAsUTF16 (
                  data, { "LibraryFolders", std::to_string (i) }, "path"
                );
            }

            if (! lib_path.empty ())
            {
              wcsncpy_s (
                (wchar_t *)steam_lib_paths [steam_libs++], MAX_PATH,
                                 lib_path.c_str (),       _TRUNCATE );
            }

            else
              break;
          }
        }
      }

      // Finally, add the default Steam library
      wcsncpy_s ( (wchar_t *)steam_lib_paths [steam_libs++],
                                               MAX_PATH,
                          wszSteamPath,       _TRUNCATE );
    }

    scanned_libs = true;
  }

  if (ppLibraries != nullptr)
    *ppLibraries = steam_lib_paths;

  return steam_libs;
}

std::string
SK_UseManifestToGetAppName (AppId_t appid)
{
  std::string manifest_data =
    SK_GetManifestContentsForAppID (appid);

  if (! manifest_data.empty ())
  {
    std::string app_name =
      SK_Steam_KeyValues::getValue (
        manifest_data, { "AppState" }, "name"
      );

    if (! app_name.empty ())
    {
      return app_name;
    }
  }

  return "";
}

std::vector <SK_Steam_Depot>
SK_UseManifestToGetDepots (AppId_t appid)
{
  std::vector <SK_Steam_Depot> depots;

  std::string manifest_data =
    SK_GetManifestContentsForAppID (appid);

  if (! manifest_data.empty ())
  {
    std::vector <std::string> values;
    auto                      mounted_depots =
      SK_Steam_KeyValues::getKeys (
        manifest_data, { "AppState", "MountedDepots" }, &values
      );

    int idx = 0;

    for ( auto& it : mounted_depots )
    {
      depots.push_back (
        SK_Steam_Depot {
          "", gsl::narrow_cast <uint32_t> (atoi  (it            .c_str ())),
              gsl::narrow_cast <uint64_t> (atoll (values [idx++].c_str ()))
        }
      );
    }
  }

  return depots;
}

ManifestId_t
SK_UseManifestToGetDepotManifest (AppId_t appid, DepotId_t depot)
{
  std::string manifest_data =
    SK_GetManifestContentsForAppID (appid);

  if (! manifest_data.empty ())
  {
    return
      atoll (
        SK_Steam_KeyValues::getValue (
          manifest_data, {
            "AppState", "InstalledDepots", std::to_string (depot)
          },
          "manifest"
        ).c_str ()
      );
  }

  return 0;
}

std::string
SK::SteamAPI::AppName (void)
{
  if (AppID () != 0x00)
  {
    // Only do this once, the AppID never changes =P
    static std::string app_name =
      SK_UseManifestToGetAppName (AppID ());

    return app_name;
  }

  return "";
}


// What the hell is wrong with Steam's 32-bit ABI?!
//
//   Some games need __fastcall, others __cdecl.
//
using SK_Steam_Callback_pfn = void (__cdecl *)(CCallbackBase* This, void* pvParam);

//
// This bypasses SteamAPI and attempts to invoke a registered callback
//   manually.
//
//   In very rare cases there are ABI-related issues. Or... more commonly,
//     a game has thrown around an invalid pointer for the callback or we
//       missed an unregister call.
//
// For all of those reasons, we run this through a Structured Exception Handler
//   and ignore access violations caused by this hackjob :)
//
void
SK_Steam_InvokeOverlayActivationCallback ( SK_Steam_Callback_pfn CallbackFn,
                                           CCallbackBase*        pClosure,
                                           bool                  active )
{
  if (__SK_Steam_IgnoreOverlayActivation || pClosure == nullptr)
    return;

  auto orig_se =
  SK_SEH_ApplyTranslator (
    SK_FilteringStructuredExceptionTranslator (EXCEPTION_ACCESS_VIOLATION)
  );
  try
  {
    GameOverlayActivated_t activated = {
      active
    };

#ifndef _M_AMD64
    UNREFERENCED_PARAMETER (CallbackFn);
    pClosure->Run (&activated);
    {
#else /* _M_IX86 */
    if (CallbackFn != nullptr)
    {
      CallbackFn (pClosure, &activated);
#endif

      SK::SteamAPI::overlay_state = active;
    }
  }

  catch (const SK_SEH_IgnoredException&)
  {
    // Oh well, we literally tried...
  }
  SK_SEH_RemoveTranslator (orig_se);
}


void
__stdcall
SK::SteamAPI::SetOverlayState (bool active)
{
  if (config.steam.silent)
    return;

  if (__SK_Steam_IgnoreOverlayActivation)
    return;

  if (steam_callback_cs != nullptr)
      steam_callback_cs->lock ();

  for ( auto& it : *overlay_activation_callbacks )
  {
    if (it == nullptr)
      continue;

    void** vftable =
      *(void ***)*(&it);

    if (vftable == nullptr)
      continue;

    SK_Steam_InvokeOverlayActivationCallback (
      reinterpret_cast <SK_Steam_Callback_pfn> (vftable [0]),
        it,
          active
    );
  }

  if (steam_callback_cs != nullptr)
      steam_callback_cs->unlock ();
}

bool
__stdcall
SK::SteamAPI::GetOverlayState (bool real)
{
  return real ? SK_IsSteamOverlayActive () :
    overlay_state;
}

bool
__stdcall
SK::SteamAPI::TakeScreenshot (SK_ScreenshotStage when)
{
  steam_log->LogEx (true, L"  >> Triggering Screenshot: ");

  bool captured = false;

  if ( static_cast <int> (SK_GetCurrentRenderBackend ().api) &
       static_cast <int> (SK_RenderAPI::D3D11) )
  {
    captured =
      SK_D3D11_CaptureScreenshot (when);
  }

  else if ( static_cast <int> (SK_GetCurrentRenderBackend ().api) &
            static_cast <int> (SK_RenderAPI::D3D12) )
  {
    captured =
      SK_D3D12_CaptureScreenshot (when);
  }

  else if ( static_cast <int> (SK_GetCurrentRenderBackend ().api) &
            static_cast <int> (SK_RenderAPI::OpenGL) )
  {
    captured =
      SK_GL_CaptureScreenshot (when);
  }

  else if ( static_cast <int> (SK_GetCurrentRenderBackend ().api) &
            static_cast <int> (SK_RenderAPI::D3D9) )
  {
    captured =
      SK_D3D9_CaptureScreenshot (when);
  }

  if (captured)
  {
    steam_log->LogEx ( false, L"Stage=%x (SK_SmartCapture)\n",
                         static_cast <int> (when) );

    return true;
  }

  ISteamScreenshots* pScreenshots =
    steam_ctx.Screenshots ();

  if (pScreenshots)
  {   pScreenshots->TriggerScreenshot ();
      steam_log->LogEx (false, L"EndOfFrame (Steam Overlay)\n");
  }

  if ( when != SK_ScreenshotStage::EndOfFrame )
  {
    steam_log->Log (L" >> WARNING: Smart Capture disabled or unsupported"
                    L"; screenshot taken at end-of-frame.");
  }

  return true;
}




bool
S_CALLTYPE
SteamAPI_InitSafe_Detour (void)
{
  // In case we already initialized stuff...
  if (ReadAcquire (&__SK_Steam_init))
    return SteamAPI_InitSafe_Original ();

  if (steam_init_cs != nullptr)
      steam_init_cs->lock ();

  static int init_tries = -1;

  if (++init_tries == 0)
  {
    steam_log->Log ( L"Initializing SteamWorks Backend  << %s >>",
                   SK_GetCallerName ().c_str () );
    steam_log->Log (L"----------(InitSafe)-----------\n");
  }

  bool bRet =
      SteamAPI_InitSafe_Original ();

  if (bRet)
  {
    if ( 1 ==
             InterlockedIncrement (&__SK_Steam_init) )
    {
      const wchar_t* steam_dll_str =
        SK_RunLHIfBitness ( 64, L"steam_api64.dll",
                                L"steam_api.dll"    );

      HMODULE hSteamAPI =
        SK_Modules->LoadLibraryLL (steam_dll_str);

      SK_SteamAPI_ContextInit (hSteamAPI);

      if (ReadAcquire (&__SK_Init) > 0)
        SK_ApplyQueuedHooks ();

      steam_log->Log (
        L"--- Initialization Finished (%d tries [AppId: %lu]) ---\n\n",
          init_tries + 1,
            SK::SteamAPI::AppID () );

      SK_Steam_StartPump (config.steam.force_load_steamapi);
    }

    if (steam_init_cs != nullptr)
        steam_init_cs->unlock ();

    return true;
  }

  if (steam_init_cs != nullptr)
      steam_init_cs->unlock ();

  return false;
}

class SK_SEH_ScopedTranslator
{
private:
  const _se_translator_function
     old_SE_translator;
public:
  SK_SEH_ScopedTranslator     ( _se_translator_function new_SE_translator ) noexcept
      : old_SE_translator { _set_se_translator        ( new_SE_translator ) } {}
  ~SK_SEH_ScopedTranslator (void) noexcept
                          { _set_se_translator        ( old_SE_translator ); }
};

void
S_CALLTYPE
SteamAPI_Shutdown_Detour (void)
{
  bool locked =
    ( steam_init_cs != nullptr   ?
      steam_init_cs->try_lock () : false );

  __try
  {
    steam_log->Log (
      L" *** Game called SteamAPI_Shutdown (...)"
    );

    SK_GetDLLConfig    (    )->write ();
    steam_ctx.Shutdown (true);
  } __except (EXCEPTION_EXECUTE_HANDLER) {}

  if (locked)
    steam_init_cs->unlock ();

  return;
}

// Toss out a few rogue exceptions generated during init in 32-bit builds
bool
SK_SAFE_SteamAPI_Init (void)
{
  bool bRet = false;

  auto orig_se =
  SK_SEH_ApplyTranslator (
    SK_FilteringStructuredExceptionTranslator (
      EXCEPTION_ACCESS_VIOLATION
    )
  );
  try {
    bRet =
      SteamAPI_Init_Original ();
  }
  catch (const SK_SEH_IgnoredException&) { }
  SK_SEH_RemoveTranslator (orig_se);

  return bRet;
}

bool
S_CALLTYPE
SteamAPI_Init_Detour (void)
{
  bool bRet = false;

  if (steam_init_cs != nullptr)
      steam_init_cs->lock ();

  // In case we already initialized stuff...
  if (ReadAcquire (&__SK_Steam_init))
  {
    bRet = SK_SAFE_SteamAPI_Init ();

    if (steam_init_cs != nullptr)
        steam_init_cs->unlock ();

    return bRet;
  }

  static volatile LONG init_tries = -1;

  if (InterlockedIncrement (&init_tries) == 0)
  {
    steam_log->Log ( L"Initializing SteamWorks Backend  << %s >>",
                    SK_GetCallerName ().c_str () );
    steam_log->Log (L"-------------------------------\n");
  }
  bool bTempAppIDFile = false;

  bRet = SK_SAFE_SteamAPI_Init ();

  if (config.steam.appid != 0 && (! bRet))
  {
    char szSteamGameId [32] = { };

    DWORD dwSteamGameIdLen =
      GetEnvironmentVariableA ( "SteamGameId",
                                  szSteamGameId,
                                    0x20 );

    FILE* fAppID = nullptr;

    if (GetFileAttributesW (L"steam_appid.txt") == INVALID_FILE_ATTRIBUTES)
    {
      fAppID =
        _wfopen (L"steam_appid.txt", L"w");

      if (fAppID != nullptr)
      {
        bTempAppIDFile = true;
        fputws ( SK_FormatStringW (L"%lu", config.steam.appid).c_str (),
                fAppID );
      }
    }

    if (dwSteamGameIdLen < 3)
    {
      SetEnvironmentVariableA (
        "SteamGameId",
          std::to_string (config.steam.appid).c_str ()
      );
    }

    bRet = SK_SAFE_SteamAPI_Init ();

    if (bTempAppIDFile && fAppID != nullptr)
    {
      fclose        (fAppID);

      if (bRet)
        DeleteFileW (L"steam_appid.txt");
    }
  }

  if (bRet)
  {
    if (1 == InterlockedIncrement (&__SK_Steam_init))
    {
      static const wchar_t* steam_dll_str =
        SK_RunLHIfBitness ( 64, L"steam_api64.dll",
                                L"steam_api.dll" );

      HMODULE hSteamAPI =
        SK_GetModuleHandleW (steam_dll_str);

      SK_SteamAPI_ContextInit (hSteamAPI);

      if (ReadAcquire (&__SK_Init) > 0)
        SK_ApplyQueuedHooks ();

      steam_log->Log ( L"--- Initialization Finished (%d tries [AppId: %lu]) ---\n\n",
                         ReadAcquire (&init_tries) + 1,
                           SK::SteamAPI::AppID () );

      SK_Steam_StartPump (config.steam.force_load_steamapi);
    }
  }

  if (steam_init_cs != nullptr)
      steam_init_cs->unlock ();

  return bRet;
}


void
__stdcall
SK_SteamAPI_SetOverlayState (bool active)
{
  SK::SteamAPI::SetOverlayState (active);
}

bool
__stdcall
SK_SteamAPI_GetOverlayState (bool real)
{
  return SK::SteamAPI::GetOverlayState (real);
}

bool
__stdcall
SK_SteamAPI_TakeScreenshot (void)
{
  return SK::SteamAPI::TakeScreenshot ();
}

void
SK_Steam_InitCommandConsoleVariables (void)
{
  steam_log->init (L"logs/steam_api.log", L"wt+,ccs=UTF-8");
  steam_log->silent = config.steam.silent;

  SK_ICommandProcessor* cmd =
    SK_GetCommandProcessor ();

  cmd->AddVariable ("Steam.TakeScreenshot",
      SK_CreateVar (SK_IVariable::Boolean,
                      (bool *)&config.steam.achievements.take_screenshot));
  cmd->AddVariable ("Steam.ShowPopup",
      SK_CreateVar (SK_IVariable::Boolean,
                      (bool *)&config.steam.achievements.popup.show));
  cmd->AddVariable ("Steam.PopupDuration",
      SK_CreateVar (SK_IVariable::Int,
                      (int  *)&config.steam.achievements.popup.duration));
  cmd->AddVariable ("Steam.PopupInset",
      SK_CreateVar (SK_IVariable::Float,
                      (float*)&config.steam.achievements.popup.inset));
  cmd->AddVariable ("Steam.ShowPopupTitle",
      SK_CreateVar (SK_IVariable::Boolean,
                      (bool *)&config.steam.achievements.popup.show_title));
  cmd->AddVariable ("Steam.PopupAnimate",
      SK_CreateVar (SK_IVariable::Boolean,
                      (bool *)&config.steam.achievements.popup.animate));
  cmd->AddVariable ("Steam.PlaySound",
      SK_CreateVar (SK_IVariable::Boolean,
                      (bool *)&config.steam.achievements.play_sound));

  steam_ctx.popup_origin =
    SK_CreateVar ( SK_IVariable::String,
                     steam_ctx.var_strings.popup_origin,
                    &steam_ctx );
  cmd->AddVariable ( "Steam.PopupOrigin",
                     steam_ctx.popup_origin );

  steam_ctx.notify_corner =
    SK_CreateVar ( SK_IVariable::String,
                     steam_ctx.var_strings.notify_corner,
                    &steam_ctx );
  cmd->AddVariable ( "Steam.NotifyCorner",
                     steam_ctx.notify_corner );

  steam_ctx.tbf_pirate_fun =
    SK_CreateVar ( SK_IVariable::Float,
                     &steam_ctx.tbf_float,
                     &steam_ctx );
}

DWORD
WINAPI
SteamAPI_Delay_Init (LPVOID)
{
  const auto thread =
    SK_GetCurrentThread ();

  SetThreadPriority           ( thread, THREAD_PRIORITY_BELOW_NORMAL  );
  SetCurrentThreadDescription ( L"[SK] SteamAPI Delayed Init. Thread" );
  SetThreadPriorityBoost      ( thread, TRUE                          );

  int tries = 0;

  while ( (! ReadAcquire (&__SK_Steam_init)) &&
             tries < 120 )
  {
    SK_Sleep (std::max (1, config.steam.init_delay));

    if (SK_GetFramesDrawn () < 1)
      continue;

    ++tries;

    if (ReadAcquire (&__SK_Steam_init))
    {
      break;
    }

    if (SteamAPI_Init_Original != nullptr)
    {
      SK_ApplyQueuedHooks  ();
      SteamAPI_Init_Detour ();
    }
  }

  SK_Thread_CloseSelf ();

  return 0;
}

std::wstring
SK_RecursiveFileSearch ( const wchar_t* wszDir,
                         const wchar_t* wszFile );


enum SK_File_SearchStopCondition {
  FirstMatchFound,
  AllMatchesFound,
  LastMatchFound
};

std::unordered_set <std::wstring>
SK_RecursiveFileSearchEx (
     const wchar_t                           *wszDir,
     const wchar_t                           *wszExtension,
     std::unordered_set <std::wstring_view>& cwsFileNames,
     std::vector        <
     std::pair          < std::wstring, bool >
     >&&                                     preferred_dirs = { },
     SK_File_SearchStopCondition             stop_condition = FirstMatchFound );

int
SK_HookSteamAPI (void)
{
  int hooks = 0;

  const wchar_t* wszSteamAPI =
    SK_Steam_GetDLLPath ();

  SK_RunOnce ( SK::SteamAPI::steam_size =
               SK_File_GetSize (wszSteamAPI) );

  if (config.steam.silent)
    return hooks;

  if (! SK_GetModuleHandle (wszSteamAPI))
    return hooks;

  if (! InterlockedCompareExchange (&__SteamAPI_hook, TRUE, FALSE))
  {
    steam_log->Log ( L"%s was loaded, hooking...",
                     SK_ConcealUserDir ( std::wstring (wszSteamAPI).data () )
    );

    SK_CreateDLLHook2 ( wszSteamAPI,
                       "SteamAPI_InitSafe",
                        SteamAPI_InitSafe_Detour,
                        static_cast_p2p <void> (&SteamAPI_InitSafe_Original),
                        static_cast_p2p <void> (&SteamAPI_InitSafe) );                   ++hooks;

    SK_CreateDLLHook2 ( wszSteamAPI,
                       "SteamAPI_Init",
                        SteamAPI_Init_Detour,
                        static_cast_p2p <void> (&SteamAPI_Init_Original),
                        static_cast_p2p <void> (&SteamAPI_Init) );                       ++hooks;

    SK_CreateDLLHook2 ( wszSteamAPI,
                       "SteamAPI_RegisterCallback",
                        SteamAPI_RegisterCallback_Detour,
                        static_cast_p2p <void> (&SteamAPI_RegisterCallback_Original),
                        static_cast_p2p <void> (&SteamAPI_RegisterCallback) );           ++hooks;

    SK_CreateDLLHook2 ( wszSteamAPI,
                       "SteamAPI_UnregisterCallback",
                        SteamAPI_UnregisterCallback_Detour,
                        static_cast_p2p <void> (&SteamAPI_UnregisterCallback_Original),
                        static_cast_p2p <void> (&SteamAPI_UnregisterCallback) );         ++hooks;

    SK_CreateDLLHook2 ( wszSteamAPI,
                       "SteamAPI_RunCallbacks",
                        SteamAPI_RunCallbacks_Detour,
                        static_cast_p2p <void> (&SteamAPI_RunCallbacks_Original),
                        static_cast_p2p <void> (&SteamAPI_RunCallbacks) );               ++hooks;


    SK_CreateDLLHook2 ( wszSteamAPI,
                       "SteamAPI_ISteamController_GetDigitalActionData",
                        SteamAPI_ISteamController_GetDigitalActionData_Detour,
                        static_cast_p2p <void> (&SteamAPI_ISteamController_GetDigitalActionData_Original) );
                                                                                         ++hooks;

    SK_CreateDLLHook2 ( wszSteamAPI,
                       "SteamAPI_ISteamController_GetAnalogActionData",
                        SteamAPI_ISteamController_GetAnalogActionData_Detour,
                        static_cast_p2p <void> (&SteamAPI_ISteamController_GetAnalogActionData_Original) );
                                                                                         ++hooks;

    //
    // Do not queue these up (by calling CreateDLLHook2),
    //   they will be installed only upon the game successfully
    //     calling one of the SteamAPI initialization functions.
    //
    SK_CreateDLLHook  ( wszSteamAPI,
                       "SteamAPI_Shutdown",
                        SteamAPI_Shutdown_Detour,
                        static_cast_p2p <void> (&SteamAPI_Shutdown_Original),
                        static_cast_p2p <void> (&SteamAPI_Shutdown) );                   ++hooks;

    std::unordered_set <std::wstring>      matches;
    std::unordered_set <std::wstring_view> pattern = {
#ifdef _M_AMD64
      L"steam_api64",
#else /* _M_IX86 */
      L"steam_api",
#endif
      L"csteamworks",
      L"steamwrapper",
      L"steamnative"
    };

    if ( config.steam.force_load_steamapi ||
         config.steam.init_delay      > 0 ||
           (! ( matches =
                  SK_RecursiveFileSearchEx (
                    SK_GetHostPath (), L".dll", pattern,
                       { {wszSteamAPI,           false},
                         {config.steam.dll_path, false} }
                  )
              ).empty ()
           )
        )
    {
      SK_Thread_Create (SteamAPI_Delay_Init);
    }

    InterlockedIncrementRelease (&__SteamAPI_hook);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&__SteamAPI_hook, 2);

  return hooks;
}

ISteamUserStats*
SK_SteamAPI_UserStats (void)
{
  return steam_ctx.UserStats ();
}

void
SK_SteamAPI_ContextInit (HMODULE hSteamAPI)
{
  if (! SteamAPI_GetSteamInstallPath)
  {
    SteamAPI_GetSteamInstallPath =
      (SteamAPI_GetSteamInstallPath_pfn)
      SK_GetProcAddress (hSteamAPI, "SteamAPI_GetSteamInstallPath");
  }

  if (! steam_ctx.UserStats ())
  {
    steam_ctx.InitSteamAPI (hSteamAPI);

#ifdef SK_STEAM_CONTROLLER_SUPPORT
    bool
    SK_Steam_HookController (void);
    SK_Steam_HookController (    );
#endif
  }
}

void
SK_SteamAPI_InitManagers (void)
{
  if (! InterlockedCompareExchange (&SK_SteamAPI_ManagersInitialized, 1, 0))
  {
    if (steam_ctx.Screenshots () && (! steam_screenshots))
       steam_screenshots.reset (new SK_Steam_ScreenshotManager ());

    ISteamUserStats* stats =
      steam_ctx.UserStats ();

    if (stats != nullptr && ((! user_manager) || (! steam_achievements)))
    {
      has_global_data = false;
      next_friend     = 0;

      if (stats->GetNumAchievements ())
      {
        steam_log->Log (L" Creating Achievement Manager...");

        steam_achievements = std::make_unique <SK_Steam_AchievementManager> (
          config.steam.achievements.sound_file.c_str     ()
        );
      }

      steam_log->LogEx (false, L"\n");

      user_manager = std::make_unique <SK_Steam_UserManager> ();
    }

    if (! __SK_Steam_IgnoreOverlayActivation)
    {
      if (steam_ctx.Utils () && (! overlay_manager))
        overlay_manager      = std::make_unique <SK_Steam_OverlayManager> ();
    }

    // Failed, try again later...
    if (((! __SK_Steam_IgnoreOverlayActivation) && overlay_manager    == nullptr) ||
                                                   steam_achievements == nullptr)
      InterlockedExchange (&SK_SteamAPI_ManagersInitialized, 0);
  }
}

void
SK_SteamAPI_DestroyManagers (void)
{
  if (InterlockedCompareExchange (&SK_SteamAPI_ManagersInitialized, 0, 1) == 1)
  {
    steam_achievements.reset ();
    steam_screenshots.reset  ();
    overlay_manager.reset    ();
    user_manager.reset       ();
  }
}


// Cache this instead of getting it from the Steam client constantly;
//   doing that is far more expensive than you would think.
size_t
SK_SteamAPI_GetNumPossibleAchievements (void)
{
  static std::pair <size_t,bool> possible =
  { 0, false };

  if ( possible.second    == false   &&
      steam_achievements != nullptr    )
  {
    steam_achievements->getAchievements (&possible.first);

    possible = { possible.first, true };
  }

  return possible.first;
}

std::vector <SK_SteamAchievement *>&
SK_SteamAPI_GetAllAchievements (void)
{
  static std::vector <SK_SteamAchievement *> achievements;

  if (steam_achievements == nullptr)
  {
    achievements.clear ();
    return achievements;
  }

  size_t num;

  SK_SteamAchievement** ppAchv =
    steam_achievements->getAchievements (&num);

  if (achievements.size () != num)
  {
    achievements.clear ();

    for (size_t i = 0; i < num; i++)
      achievements.push_back (ppAchv [i]);
  }

  return achievements;
}

std::vector <SK_SteamAchievement *>&
SK_SteamAPI_GetUnlockedAchievements (void)
{
  static std::vector <SK_SteamAchievement *> unlocked_achievements;

  unlocked_achievements.clear ();

  std::vector <SK_SteamAchievement *>& achievements =
    SK_SteamAPI_GetAllAchievements ();

  for ( auto& it : achievements )
  {
    if (it->unlocked_)
      unlocked_achievements.push_back (it);
  }

  return unlocked_achievements;
}

std::vector <SK_SteamAchievement *>&
SK_SteamAPI_GetLockedAchievements (void)
{
  static std::vector <SK_SteamAchievement *> locked_achievements;

  locked_achievements.clear ();

  std::vector <SK_SteamAchievement *>& achievements =
    SK_SteamAPI_GetAllAchievements ();

  for ( auto& it : achievements )
  {
    if (! it->unlocked_)
      locked_achievements.push_back (it);
  }

  return locked_achievements;
}

float
SK_SteamAPI_GetUnlockedPercentForFriend (uint32_t friend_idx)
{
  if (steam_achievements != nullptr)
  {
    return
      steam_achievements->getFriendUnlockPercentage (friend_idx);
  }

  return 0.0f;
}

size_t
SK_SteamAPI_GetUnlockedAchievementsForFriend (uint32_t friend_idx, BOOL* pStats)
{
  if (steam_achievements == nullptr)
    return 0;

  size_t num_achvs = SK_SteamAPI_GetNumPossibleAchievements ();
  size_t unlocked  = 0;

  for (size_t i = 0; i < num_achvs; i++)
  {
    if (steam_achievements->hasFriendUnlockedAchievement (friend_idx, (uint32_t)i))
    {
      if (pStats != nullptr)
        pStats [i] = TRUE;

      unlocked++;
    }

    else if (pStats != nullptr)
      pStats [i] = FALSE;
  }

  return unlocked;
}

size_t
SK_SteamAPI_GetLockedAchievementsForFriend (uint32_t friend_idx, BOOL* pStats)
{
  if (steam_achievements == nullptr)
    return 0;

  size_t num_achvs = SK_SteamAPI_GetNumPossibleAchievements ();
  size_t locked    = 0;

  for (size_t i = 0; i < num_achvs; i++)
  {
    if (! steam_achievements->hasFriendUnlockedAchievement (friend_idx, (uint32_t)i))
    {
      if (pStats != nullptr)
        pStats [i] = TRUE;

      locked++;
    }

    else if (pStats != nullptr)
      pStats [i] = FALSE;
  }

  return locked;
}

size_t
SK_SteamAPI_GetSharedAchievementsForFriend (uint32_t friend_idx, BOOL* pStats)
{
  if (steam_achievements == nullptr)
    return 0;

  std::vector <BOOL> friend_stats;
  friend_stats.resize (SK_SteamAPI_GetNumPossibleAchievements ());

  //  size_t unlocked =
  SK_SteamAPI_GetUnlockedAchievementsForFriend (friend_idx, friend_stats.data ());

  size_t shared = 0;

  std::vector <SK_SteamAchievement *>&
    unlocked_achvs = SK_SteamAPI_GetUnlockedAchievements ();

  if (pStats != nullptr)
    RtlSecureZeroMemory (pStats, SK_SteamAPI_GetNumPossibleAchievements ());

  for ( auto& it : unlocked_achvs )
  {
    if (friend_stats [it->idx_])
    {
      if (pStats != nullptr)
        pStats [it->idx_] = TRUE;

      shared++;
    }
  }

  return shared;
}


// Returns true if all friend stats have been pulled from the server
bool
__stdcall
SK_SteamAPI_FriendStatsFinished (void)
{
  return (next_friend >= friend_count);
}

// Percent (0.0 - 1.0) of friend achievement info fetched
float
__stdcall
SK_SteamAPI_FriendStatPercentage (void)
{
  if (SK_SteamAPI_FriendStatsFinished ())
    return 1.0f;

  return
    ( static_cast <float> ( next_friend) /
      static_cast <float> (friend_count) );
}

const std::string&
__stdcall
SK_SteamAPI_GetFriendName (uint32_t idx, size_t* pLen)
{
  static const std::string dummy_ = "";

  uint32_t friend_quick_count =
    gsl::narrow_cast <uint32_t> (
      std::max (0i32, SK_SteamAPI_GetNumFriends ())
    );

  if ( steam_achievements == nullptr ||
       friend_quick_count <= idx )
  {
    return dummy_;
  }

  const std::string& name =
    steam_achievements->getFriendName (idx);

  if (pLen != nullptr)
    *pLen = name.length ();

  // Not the most sensible place to implement this, but ... whatever
  return name;
}

int
__stdcall
SK_SteamAPI_GetNumFriends (void)
{
  return friend_count;
}


bool steam_imported = false;

bool
SK_Steam_Imported (void)
{
  static const wchar_t* steam_dll_str =
    SK_RunLHIfBitness ( 64, L"steam_api64.dll",
                            L"steam_api.dll" );

  return
    ( steam_imported || SK_GetModuleHandle (steam_dll_str)      ||
                        SK_GetModuleHandle (L"CSteamworks.dll") ||
                        SK_GetModuleHandle (L"SteamNative.dll") );
}

bool
SK_Steam_TestImports (HMODULE hMod)
{
  const wchar_t* steam_dll_str =
    SK_Steam_GetDLLPath ();

  if (*steam_dll_str == L'\0' || StrStrIW (steam_dll_str, L"SpecialK"))
    config.steam.force_load_steamapi = true;

  sk_import_test_s steam_api [] = {
      { SK_RunLHIfBitness ( 64, "steam_api64.dll",
                                "steam_api.dll" ), false }
  };

  SK_TestImports (hMod, steam_api, sizeof (steam_api) / sizeof sk_import_test_s);

  if (steam_api->used)
  {
    SK_HookSteamAPI ();
    steam_imported = true;
  }

  else
  {
    if (! SK_IsInjected ())
    {
      if ( SK_Modules->LoadLibrary (steam_dll_str) ||
           SK_GetModuleHandle (L"SteamNative.dll") )
      {
        SK_HookSteamAPI ();
        steam_imported = true;
      }
    }
  }

  if ( SK_Modules->LoadLibrary (steam_dll_str) ||
       SK_GetModuleHandle (L"SteamNative.dll") )
  {
    SK_HookSteamAPI ();
    steam_imported = true;
  }

  // Special case (kick-start)
  if (config.steam.force_load_steamapi)
  {
    return false;
  }

  return steam_imported;
}

void
WINAPI
SK_SteamAPI_AddScreenshotToLibrary ( const char *pchFilename,
                                     const char *pchThumbnailFilename,
                                           int   nWidth,
                                           int   nHeight )
{
  if (steam_ctx.Screenshots ())
  {
    steam_ctx.Screenshots ()->AddScreenshotToLibrary (
      pchFilename,
      pchThumbnailFilename,
           nWidth,  nHeight
    );
  }
}

ScreenshotHandle
WINAPI
SK_SteamAPI_AddScreenshotToLibraryEx ( const char *pchFilename,
                                       const char *pchThumbnailFilename,
                                             int   nWidth,
                                             int   nHeight,
                                             bool  Wait )
{
  if (steam_ctx.Screenshots ())
  {
    ScreenshotHandle handle =
      steam_ctx.Screenshots ()->AddScreenshotToLibrary (
        pchFilename,
        pchThumbnailFilename,
             nWidth,  nHeight
      );

    if (Wait)
    {
      steam_screenshots->WaitOnScreenshot ( handle );
    }

    return handle;
  }

  return 0;
}

void
WINAPI
SK_SteamAPI_WriteScreenshot ( void   *pubRGB,
                              uint32  cubRGB,
                              int     nWidth,
                              int     nHeight )
{
  if (steam_ctx.Screenshots ())
  {
    steam_ctx.Screenshots ()->WriteScreenshot (
       pubRGB,
       cubRGB,  nWidth,
                nHeight
    );
  }
}

bool SK::SteamAPI::overlay_state = false;


void
__stdcall
SK_SteamAPI_UpdateNumPlayers (void)
{
  if (user_manager != nullptr)
    user_manager->UpdateNumPlayers ();
}

int32_t
__stdcall
SK_SteamAPI_GetNumPlayers (void)
{
  if (user_manager != nullptr)
  {
    int32 num =
      user_manager->GetNumPlayers ();

    if (num <= 1)
      user_manager->UpdateNumPlayers ();

    return std::max (1, user_manager->GetNumPlayers ());
  }

  return 1; // You, and only you, apparently ;)
}

void
__stdcall
SK::SteamAPI::UpdateNumPlayers (void)
{
  SK_SteamAPI_UpdateNumPlayers ();
}

int32_t
__stdcall
SK::SteamAPI::GetNumPlayers (void)
{
  return SK_SteamAPI_GetNumPlayers ();
}

float
__stdcall
SK_SteamAPI_PercentOfAchievementsUnlocked (void)
{
  if (steam_achievements != nullptr)
    return steam_achievements->getPercentOfAchievementsUnlocked ();

  return 0.0f;
}

float
__stdcall
SK::SteamAPI::PercentOfAchievementsUnlocked (void)
{
  return SK_SteamAPI_PercentOfAchievementsUnlocked ();
}

ISteamUtils*
SK_SteamAPI_Utils (void)
{
  return steam_ctx.Utils ();
}

ISteamMusic*
SK_SteamAPI_Music (void)
{
  return steam_ctx.Music ();
}

ISteamRemoteStorage*
SK_SteamAPI_RemoteStorage (void)
{
  return steam_ctx.RemoteStorage ();
}

ISteamUGC*
SK_SteamAPI_UGC (void)
{
  return steam_ctx.UGC ();
}

// Returns the REAL value of this
SK_SteamUser_LoggedOn_e
SK_SteamUser_BLoggedOn (void)
{
  extern int __SK_SteamUser_BLoggedOn;

  if ( __SK_SteamUser_BLoggedOn ==
        static_cast <int> (SK_SteamUser_LoggedOn_e::Unknown) )
  {
    ISteamUser* pUser =
      steam_ctx.User ();

    if (pUser)
      return pUser->BLoggedOn () ? SK_SteamUser_LoggedOn_e::Online :
                                   SK_SteamUser_LoggedOn_e::Offline;

    else
      return SK_SteamUser_LoggedOn_e::Unknown;
  }

  return
    static_cast <SK_SteamUser_LoggedOn_e> (
      __SK_SteamUser_BLoggedOn
      );
}


//
// ATTENTION PIRATES:  Forking Special K and circumventing this is 99% of the work you need
// ------------------    to do to "crack" my mods. The remaining 1% involves removing my name.
//
//
//  You will find various measures scattered throughout the code that took me all of five
//    minutes to devise whose intention is to make any attempt to do something other than
//      modify the source code fail.
//
//  >> TBFix, for example, will throw up a HUGE positive LOD bias causing blurry textures if
//       the framerate limit restriction is bypassed with a simple hex edit.
//
//  Please refrain from simply binary-editing (because it violates license terms) my
//    distributions, if you have the skill to circumvent DRM it should be obvious that I
//      intend every one of my measures to be plainly visible and should take you
//        thirty-seconds to figure out and remove the correct way.
//
//     You have various other responsibilities if you want to distribute this for cracked
//       software such as replacing any hard-coded memory addresses, etc. and I will not
//         offer any assistance, so do not come to me for support in any form.
//
//
//  "Proper" (lol) Redistribution for Illegal Purposes (which I do not condone) follows:
//  ------------------------------------------------------------------------------------
//
//    1.  Fork the C++ code, make your changes public for the decency of your community
//    2.  Remove my name if you are going to distribute your modification in anything illegal
//    3.  Stop accusing me of distributing malware, you rarely even honor bullet-point #1
//
//
//
//  Common Sense Practices it Frequently Astounds me that you Ignore:
//  -----------------------------------------------------------------
//
//   (*) Disable SteamAPI enhancement, most of the stuff you complain about isn't even a problem
//        if you would simply turn SteamAPI features off.
//
//    >> Your fake SteamAPI implementations are fragile, and I sure as hell have no intention to
//         test against them or fix them.
//
//    >> Opt-out of Steam-related enhancement if you care about stability.
//
//
//   (*) Do not grab the latest version of my mod and shoe-horn it into your cracked binary
//
//    >> I maintain forward-compatibility for the benefit of the paying Steam community,
//         you need the opposite since your executable never receives updates.
//
//    >> I remove fixes for stuff that the publisher has already fixed, you get nothing from
//         following my update schedule when your executable has its own set of problems that
//           are not addressed by new versions of the software.
//
//-----------------------------------------------------------------------------------------------
//    Remove my name from any forked work, and stop making your community look like a bunch of
//      children; it reflects poorly on the whole of PC gaming.
//-----------------------------------------------------------------------------------------------
//
static uint32_t       verdict        = 0x00;
static bool           decided        = false;

static SteamAPICall_t hAsyncSigCheck = 0; // File Signature Validation
static std::string    check_file;

enum class SK_Steam_FileSigPass_e
{
  Executable = 0,
  SteamAPI   = 1,
  DLC        = 2,

  Done       = MAXINT
};

static SK_Steam_FileSigPass_e
  validation_pass (
       SK_Steam_FileSigPass_e::Executable
  );

bool
__stdcall
SK_Steam_GetInstalledDepots (DepotId_t *depots)
{
  __try
  {
    const auto pApps  = steam_ctx.Apps  ();
    const auto pUtils = steam_ctx.Utils ();

    if (pApps != nullptr)
    {   pApps->GetInstalledDepots (pUtils->GetAppID (), depots, 16);
        return true;
    }
  }

  __except ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER  :
                EXCEPTION_CONTINUE_SEARCH )
  {
  }

  return false;
}
uint32_t
__stdcall
SK_Steam_PiratesAhoy (void)
{
  const auto pApps  = steam_ctx.Apps  ();
  const auto pUtils = steam_ctx.Utils ();
  const auto pUser  = steam_ctx.User  ();

  //   Older versions of SteamAPI may not support the necessary interface version
  if ( pUtils != nullptr && pApps != nullptr &&
       validation_pass   != SK_Steam_FileSigPass_e::Done )
  {
    DepotId_t depots [16] = { };

    if (! SK_Steam_GetInstalledDepots (depots))
      return verdict;

    if ( hAsyncSigCheck == 0 &&
      (! config.steam.silent) )
    {
      switch (validation_pass)
      {
        // For information purposes only
        case SK_Steam_FileSigPass_e::Executable:
        {
          check_file =
            SK_WideCharToUTF8 (SK_GetFullyQualifiedApp ());
        } break;

        // It is critically important to verify this DLL or pirates will waste my time
        //   complaining all about how their cracked game crashes; I have better stuff to do.
        case SK_Steam_FileSigPass_e::SteamAPI:
        {
          char szRelSteamAPI [MAX_PATH + 2] = { };

          // Generally, though not always (i.e. CSteamWorks) we will link against one of these two DLLs
          //
          //   The actual DLL used is pulled from the IAT during init, but I am too lazy to bother doing
          //     this the right way ;)
          snprintf ( szRelSteamAPI, MAX_PATH,
                       "%ws", SK_Steam_GetDLLPath () );

          check_file =
            szRelSteamAPI;
        } break;
      }

      if (! check_file.empty ())
      {
        hAsyncSigCheck =
          pApps->GetFileDetails (check_file.c_str ());

        steam_ctx.get_file_details.Set (
          hAsyncSigCheck,
            &steam_ctx,
              &SK_SteamAPIContext::OnFileDetailsDone
        );
      }
    }
  }

  if (decided)
  {
    return verdict;
  }

  static uint32_t crc32_steamapi =
    SK_File_GetCRC32C (SK_Steam_GetDLLPath ());

  // DLL is too small to be legit, don't enable SteamAPI features
  if ( SK::SteamAPI::steam_size > 0 &&
       SK::SteamAPI::steam_size < (1024 * 92) )
  {
    verdict = 0x68992;
  }

  // CPY's favorite fake DLL
  if (pUser != nullptr)
  {
    if ( pUser->GetSteamID ().ConvertToUint64 () == 18295873490452480 << 4 ||
         pUser->GetSteamID ().ConvertToUint64 () == 25520399320093309  * 3 )
      verdict = 0x1;
  }

  if (crc32_steamapi == 0x28140083 << 1)
    verdict = crc32_steamapi;

  decided = true;

  // User opted out of Steam enhancement, no further action necessary
  if (config.steam.silent && verdict)
  {
    validation_pass = SK_Steam_FileSigPass_e::Done;
    verdict         = 0x00;
  }

  return verdict;
}

uint32_t
__stdcall
SK_Steam_PiratesAhoy2 (void)
{
  return SK_Steam_PiratesAhoy ();
}



static HANDLE hSigNewSteamFileDetails {
  INVALID_HANDLE_VALUE
};


void
SK_SteamAPIContext::OnFileDetailsDone ( FileDetailsResult_t* pParam,
                                        bool                 bFailed )
{
  struct _HashWorkComparitor {
    bool operator () ( const FileDetailsResult_t& lh,
                       const FileDetailsResult_t& rh ) const {
      return ( rh.m_ulFileSize > lh.m_ulFileSize );
    }
  };

  typedef
    concurrency::concurrent_priority_queue <
      FileDetailsResult_t,
      _HashWorkComparitor
    > hash_job_prio_queue_s;



  pParam->m_eResult =
    ( bFailed ? k_EResultFileNotFound :
                pParam->m_eResult       );


  static hash_job_prio_queue_s
    waiting_hash_jobs;
    waiting_hash_jobs.push ( *pParam );


  if ( INVALID_HANDLE_VALUE == hSigNewSteamFileDetails )
  {
    hSigNewSteamFileDetails =
      SK_CreateEvent ( nullptr, FALSE, true, nullptr );

    //
    // Hashing Denuvo games can take a very long time, and we do not need
    //   the result immediately... so do not destroy the game's performance
    //     by blocking at startup!
    //
    SK_Thread_Create ([](LPVOID user) ->
    DWORD
    {
      // We don't need these results anytime soon, get them when we get them...
      SetCurrentThreadDescription (L"[SK] Steam File Validator");
      SetThreadPriority           ( GetCurrentThread (), THREAD_PRIORITY_IDLE         );
      SetThreadPriorityBoost      ( GetCurrentThread (), TRUE                         );

      hash_job_prio_queue_s* prio_queue =
        (hash_job_prio_queue_s *)user;


      while ( WAIT_OBJECT_0 ==
                MsgWaitForMultipleObjectsEx (
                  1, &hSigNewSteamFileDetails,
                    666UL, QS_ALLEVENTS, MWMO_INPUTAVAILABLE
                )
            )
      {

        if ( ReadAcquire (&__SK_DLL_Ending) ||
               validation_pass == SK_Steam_FileSigPass_e::Done
           ) break;


        FileDetailsResult_t result = { };
        {
          while ( (! prio_queue->empty   (        ) ) &&
                  (! prio_queue->try_pop ( result ) ) )
          {
            MsgWaitForMultipleObjectsEx (
              1, &hSigNewSteamFileDetails,
                133UL, QS_ALLEVENTS, MWMO_INPUTAVAILABLE
            );
          }
        }
        FileDetailsResult_t* pParam = &result;

        auto HandleResult =
        [&]( const wchar_t* wszFileName )
        {
          // If SteamAPI shuts down, stop doing this.
          if (! steam_ctx.Utils ())
            return;

          SK_SHA1_Hash SHA1;
          memcpy (SHA1.hash, pParam->m_FileSHA, 20);

          std::wstring wszStrippedName  = wszFileName;
                       wszStrippedName += L'\0';

          SK_StripUserNameFromPathW (wszStrippedName.data ());

          switch (pParam->m_eResult)
          {
            case k_EResultOK:
            {
              uint64_t size =
                SK_File_GetSize (wszFileName);

              SK_SHA1_Hash file_hash =
                SK_File_GetSHA1 (wszFileName);

              char            szSHA1 [22] = { };
              SHA1.toCString (szSHA1);

              if (size == pParam->m_ulFileSize && file_hash == SHA1)
              {
                steam_log->Log ( L"> SteamAPI Application File Verification:  "
                                 L" Match  ( File: %ws,\n"
                                 L"                                                                              SHA1: %20hs,\n"
                                 L"                                                                              Size: %lu bytes )",
                                 wszStrippedName.c_str (),
                                 szSHA1, pParam->m_ulFileSize );
              }

              else if (size != 0)
              {
                iSK_Logger* const logs [] = {
                  steam_log.getPtr (),
                    dll_log.getPtr ()
                };

                if (size != pParam->m_ulFileSize)
                {
                  for ( auto log : logs )
                  {
                    log->Log ( L"> SteamAPI DLL File Verification:     "
                               L" Size Mismatch ( File: %ws,\n"
                               L"                                                                              Expected SHA1: %20hs,\n"
                               L"                                                                              Expected Size: %lu bytes,\n"
                               L"                                                                                Actual Size: %lu bytes )",
                               wszStrippedName.c_str (),
                               szSHA1,
                               pParam->m_ulFileSize, size );
                  }
                }

                else if (file_hash != SHA1)
                {
                  char                 szFileSHA1 [22] = { };
                  file_hash.toCString (szFileSHA1);

                  for ( auto log : logs )
                  {
                    log->Log ( L"> SteamAPI File Verification:     "
                               L" SHA1 Mismatch ( File: %ws,\n"
                               L"                                                                              Expected SHA1: %20hs,\n"
                               L"                                                                                Actual SHA1: %20hs,\n"
                               L"                                                                                       Size: %lu bytes )",
                               wszStrippedName.c_str (),
                               szSHA1, szFileSHA1,
                               pParam->m_ulFileSize );
                  }
                }

                if (validation_pass == SK_Steam_FileSigPass_e::SteamAPI)
                {
                  verdict |= ~( k_ECheckFileSignatureInvalidSignature );
                  decided  =    true;
                }
              }
            } break;

            default:
            {
              steam_log->Log ( L"> SteamAPI File Verification:                "
                               L" UNKNOWN STATUS (%lu) :: '%ws'",
                               pParam->m_eResult,
                               wszStrippedName.c_str () );
            } break;
          }
        };

        HandleResult (SK_UTF8ToWideChar (check_file).c_str ());

        switch (validation_pass)
        {
          case SK_Steam_FileSigPass_e::Executable:
          {
            validation_pass = SK_Steam_FileSigPass_e::SteamAPI;
          } break;

          case SK_Steam_FileSigPass_e::SteamAPI:
          {
            validation_pass = SK_Steam_FileSigPass_e::Done;
          } break;

          default: break;
        }

        hAsyncSigCheck = 0;
      }

      SK_Thread_CloseSelf ();

      SK_AutoHandle hCloseMe (
        hSigNewSteamFileDetails
      );hSigNewSteamFileDetails = INVALID_HANDLE_VALUE;

      return 0;
    }, (void *)&waiting_hash_jobs);
  }

  else {
    SetEvent (hSigNewSteamFileDetails);
  }


  get_file_details.Cancel ();
}

void
SK_Steam_ForceInputAppId (AppId_t appid)
{
  if (config.steam.appid != 0)
  {
    struct {
      concurrency::concurrent_queue <AppId_t> app_ids;
      SK_AutoHandle                           signal =
        SK_CreateEvent ( nullptr, FALSE, FALSE, nullptr );

      void push (AppId_t appid)
      {
        app_ids.push ( appid == 0 ? config.steam.appid
                                  :              appid );
        SetEvent (signal.m_h);
      }
    } static override_ctx;

    SK_RunOnce (
      SK_Thread_CreateEx ([](LPVOID) -> DWORD
      {
        auto hModules = {
          SK_LoadLibraryW (L"ieframe.dll"),
          SK_LoadLibraryW (L"WINHTTP.dll")
        };

        HANDLE hWaitObjects [] = {
          __SK_DLL_TeardownEvent,
          override_ctx.signal.m_h
        };

        DWORD dwWaitState = WAIT_OBJECT_0;

        do
        {
          dwWaitState =
            WaitForMultipleObjects (2, hWaitObjects, FALSE, INFINITE);

          if (dwWaitState == WAIT_OBJECT_0 + 1)
          {
            AppId_t                              appid;
            while (override_ctx.app_ids.try_pop (appid))
            {
              ShellExecuteW ( 0, L"OPEN",
                  SK_FormatStringW ( LR"(steam://forceinputappid/%d)",
                                                           appid ).c_str (),
                    nullptr, nullptr, SW_HIDE );
            }
          }
        } while (dwWaitState != WAIT_OBJECT_0);

        // Cleanup on unexpected application termination
        //
        ShellExecuteW ( 0, L"OPEN",
                  LR"(steam://forceinputappid/0)", nullptr, nullptr,
                    SW_HIDE );

        for (auto module : hModules)
        {
          if (              module != 0)
            SK_FreeLibrary (module);
        }

        SK_Thread_CloseSelf ();

        return 0;
      }, L"[SK] SteamInput Suppressor")
    );

    override_ctx.push (appid);
  }
}

// Fallback to Win32 API if the Steam overlay is not functioning.

bool
__stdcall
SK_SteamOverlay_GoToURL ( const char* szURL,
                                bool  bUseWindowsShellIfOverlayFails )
{
  if ( steam_ctx.Utils () != nullptr        &&
       steam_ctx.Utils ()->IsOverlayEnabled () )
  {
    if (steam_ctx.Friends () != nullptr)
    {
      steam_ctx.Friends ()->ActivateGameOverlayToWebPage (szURL);
      return true;
    }
  }

  if (bUseWindowsShellIfOverlayFails)
  {
    SK_ShellExecuteA ( nullptr, "open",
                         szURL, nullptr, nullptr, SW_NORMAL );
    return true;
  }

  return false;
}

bool
__stdcall
SK_SteamOverlay_GoToFriendProfile (CSteamID friend_sid)
{
  if ( steam_ctx.Utils () != nullptr        &&
       steam_ctx.Utils ()->IsOverlayEnabled () )
  {
    if (steam_ctx.Friends () != nullptr)
    {
      steam_ctx.Friends ()->ActivateGameOverlayToUser (
        "steamid", friend_sid
      );

      return true;
    }
  }

  return false;
}

bool
__stdcall
SK_SteamOverlay_GoToFriendAchievements (CSteamID friend_sid)
{
  if ( steam_ctx.Utils () != nullptr        &&
       steam_ctx.Utils ()->IsOverlayEnabled () )
  {
    if (steam_ctx.Friends () != nullptr)
    {
      steam_ctx.Friends ()->ActivateGameOverlayToUser (
        "achievements", friend_sid
      );

      return true;
    }
  }

  return false;
}

bool
__stdcall
SK_SteamOverlay_GoToFriendStats (CSteamID friend_sid)
{
  if ( steam_ctx.Utils () != nullptr        &&
       steam_ctx.Utils ()->IsOverlayEnabled () )
  {
    if (steam_ctx.Friends () != nullptr)
    {
      steam_ctx.Friends ()->ActivateGameOverlayToUser (
        "stats", friend_sid
      );

      return true;
    }
  }

  return false;
}



ISteamMusic*
SAFE_GetISteamMusic (       ISteamClient *pClient,
                            HSteamUser    hSteamuser,
                            HSteamPipe    hSteamPipe,
                      const char         *pchVersion )
{
  ISteamMusic* pMusic = nullptr;

  if (pClient != nullptr)
  {
    using  SteamInternal_CreateInterface_pfn =
     void* (S_CALLTYPE*)(const char* ver);
    extern SteamInternal_CreateInterface_pfn
           SteamInternal_CreateInterface_Original;

    if (SteamInternal_CreateInterface_Original != nullptr)
    {
      if ( nullptr !=
             SteamInternal_CreateInterface_Original (
               STEAMMUSIC_INTERFACE_VERSION
             )
         )
      {
        auto orig_se =
        SK_SEH_ApplyTranslator (
          SK_FilteringStructuredExceptionTranslator (
            EXCEPTION_ACCESS_VIOLATION
          )
        );
        try
        {
          if (SK_IsAddressExecutable ((*(void ***)*&pClient)[24]))
          {
            pMusic =
              pClient->GetISteamMusic (hSteamuser, hSteamPipe, pchVersion);
          }
        }

        catch (const SK_SEH_IgnoredException&) { };
        SK_SEH_RemoveTranslator (orig_se);
      }
    }
  }

  return pMusic;
}

ISteamController*
SAFE_GetISteamController (       ISteamClient *pClient,
                                 HSteamUser    hSteamuser,
                                 HSteamPipe    hSteamPipe,
                           const char         *pchVersion )
{
  ISteamController* pController = nullptr;

  if (pClient != nullptr)
  {
    auto orig_se =
    SK_SEH_ApplyTranslator (
      SK_FilteringStructuredExceptionTranslator(EXCEPTION_ACCESS_VIOLATION)
    );
    try
    {
      if (SK_IsAddressExecutable ((*(void ***)*&pClient)[21]))
      {
        pController =
          pClient->GetISteamController (hSteamuser, hSteamPipe, pchVersion);
      }
    }

    catch (const SK_SEH_IgnoredException&) { };
    SK_SEH_RemoveTranslator (orig_se);
  }

  return pController;
}

ISteamRemoteStorage*
SAFE_GetISteamRemoteStorage (       ISteamClient *pClient,
                                    HSteamUser    hSteamuser,
                                    HSteamPipe    hSteamPipe,
                              const char         *pchVersion )
{
  ISteamRemoteStorage* pStorage = nullptr;

  if (pClient != nullptr)
  {
    auto orig_se =
    SK_SEH_ApplyTranslator (
      SK_FilteringStructuredExceptionTranslator(EXCEPTION_ACCESS_VIOLATION)
    );
    try
    {
      if (SK_IsAddressExecutable ((*(void ***)*&pClient)[13]))
      {
        pStorage =
          pClient->GetISteamRemoteStorage (
            hSteamuser, hSteamPipe, pchVersion
          );
      }
    }
    catch (const SK_SEH_IgnoredException&) { };
    SK_SEH_RemoveTranslator (orig_se);
  }

  return pStorage;
}

ISteamUGC*
SAFE_GetISteamUGC (ISteamClient* pClient, HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
{
  ISteamUGC* pUGC = nullptr;

  if (pClient != nullptr)
  {
    auto orig_se =
    SK_SEH_ApplyTranslator (
      SK_FilteringStructuredExceptionTranslator(EXCEPTION_ACCESS_VIOLATION)
    );
    try
    {
      if (SK_IsAddressExecutable ((*(void ***)*&pClient)[23]))
      {
        pUGC =
          pClient->GetISteamUGC (hSteamuser, hSteamPipe, pchVersion);
      }
    }
    catch (const SK_SEH_IgnoredException&) { };
    SK_SEH_RemoveTranslator (orig_se);
  }

  return pUGC;
}


extern "C"
  void __cdecl SteamAPIDebugTextHook (       int   nSeverity,
                                       const char *pchDebugText );

extern "C"
void
__cdecl
SK_SteamAPI_DebugText (int nSeverity, const char *pchDebugText)
{
  steam_log->Log ( L" [SteamAPI] Severity: %d - '%hs'",
                   nSeverity, pchDebugText );
}

bool
SK_SteamAPIContext::InitSteamAPI (HMODULE hSteamDLL)
{
  if (SteamAPI_InitSafe == nullptr)
  {
    SteamAPI_InitSafe =
      (SteamAPI_InitSafe_pfn)SK_GetProcAddress (
        hSteamDLL,
        "SteamAPI_InitSafe"
      );
  }

  if (SteamAPI_GetHSteamUser == nullptr)
  {
    SteamAPI_GetHSteamUser =
      (SteamAPI_GetHSteamUser_pfn)SK_GetProcAddress (
        hSteamDLL,
        "SteamAPI_GetHSteamUser"
      );
  }

  if (SteamAPI_GetHSteamPipe == nullptr)
  {
    SteamAPI_GetHSteamPipe =
      (SteamAPI_GetHSteamPipe_pfn)SK_GetProcAddress (
        hSteamDLL,
        "SteamAPI_GetHSteamPipe"
      );
  }

  SteamAPI_IsSteamRunning =
    (SteamAPI_IsSteamRunning_pfn)SK_GetProcAddress (
      hSteamDLL,
      "SteamAPI_IsSteamRunning"
    );

  if (SteamClient_Original == nullptr)
  {
    SteamClient_Original =
      (SteamClient_pfn)SK_GetProcAddress (
        hSteamDLL,
        "SteamClient"
      );
  }

  if (SteamAPI_RegisterCallResult == nullptr)
  {
    SteamAPI_RegisterCallResult =
      (SteamAPI_RegisterCallResult_pfn)SK_GetProcAddress (
        hSteamDLL,
        "SteamAPI_RegisterCallResult"
      );
  }

  if (SteamAPI_UnregisterCallResult == nullptr)
  {
    SteamAPI_UnregisterCallResult =
      (SteamAPI_UnregisterCallResult_pfn)SK_GetProcAddress (
        hSteamDLL,
        "SteamAPI_UnregisterCallResult"
      );
  }


  bool success = true;

  if (SteamAPI_GetHSteamUser == nullptr)
  {
    steam_log->Log (L"Could not load SteamAPI_GetHSteamUser (...)");
    success = false;
  }

  if (SteamAPI_GetHSteamPipe == nullptr)
  {
    steam_log->Log (L"Could not load SteamAPI_GetHSteamPipe (...)");
    success = false;
  }

  if (SteamClient_Original == nullptr)
  {
    steam_log->Log (L"Could not load SteamClient (...)");
    success = false;
  }


  if (SteamAPI_RunCallbacks == nullptr)
  {
    steam_log->Log (L"Could not load SteamAPI_RunCallbacks (...)");
    success = false;
  }

  if (SteamAPI_Shutdown == nullptr)
  {
    steam_log->Log (L"Could not load SteamAPI_Shutdown (...)");
    success = false;
  }

  if (! success)
    return false;

  if (! SteamAPI_InitSafe ())
  {
    steam_log->Log (L" SteamAPI_InitSafe (...) Failed?!");
    return false;
  }

  client_ = SteamClient_Original ();

  if (! client_)
  {
    steam_log->Log (L" SteamClient (...) Failed?!");
    return false;
  } else
    client_->SetWarningMessageHook ( &SK_SteamAPI_DebugText );


  hSteamPipe = SteamAPI_GetHSteamPipe ();
  hSteamUser = SteamAPI_GetHSteamUser ();


#define INTERNAL_STEAMCLIENT_INTERFACE_VERSION        17
#define INTERNAL_STEAMUSER_INTERFACE_VERSION          18
#define INTERNAL_STEAMFRIENDS_INTERFACE_VERSION       15
#define INTERNAL_STEAMUSERSTATS_INTERFACE_VERSION     11
#define INTERNAL_STEAMAPPS_INTERFACE_VERSION           8
#define INTERNAL_STEAMUTILS_INTERFACE_VERSION          7
#define NEWEST_STEAMUTILS_INTERFACE_VERSION            9
#define INTERNAL_STEAMSCREENSHOTS_INTERFACE_VERSION    2
#define INTERNAL_STEAMMUSIC_INTERFACE_VERSION          1
#define INTERNAL_STEAMREMOTESTORAGE_INTERFACE_VERSION 12
#define INTERNAL_STEAMUGC_INTERFACE_VERSION           10

  int i = 0;

  if (SteamInternal_CreateInterface_Original != nullptr)
  {
    for (i = INTERNAL_STEAMCLIENT_INTERFACE_VERSION+5; i > 0 ; --i)
    {
      auto client =
        SteamInternal_CreateInterface_Original (
          SK_FormatString ("SteamClient%03lu", i).c_str ()
        );

      if (client != nullptr)
      {
        if (client_ver_ == 0)
        {
          client_ver_ = i;
          steam_log->Log (L"SteamAPI DLL Implements Steam Client v%03lu", i);

          if (client_ver_ > INTERNAL_STEAMCLIENT_INTERFACE_VERSION + 1)
            steam_log->Log (L" >> Newer than Special K knows about.");
        }

        if (i == INTERNAL_STEAMCLIENT_INTERFACE_VERSION)
          break;
      }
    }
  }

  for (i = INTERNAL_STEAMUSER_INTERFACE_VERSION+5; i > 0; --i)
  {
    user_ =
      client_->GetISteamUser (
        hSteamUser, hSteamPipe,
          SK_FormatString ("SteamUser%03lu", i).c_str ()
      );

    if (user_ != nullptr)
    {
      if (user_ver_ == 0)
      {
        user_ver_ = i;
        steam_log->Log (L"SteamAPI DLL Implements Steam User v%03lu", i);

        if (user_ver_ > INTERNAL_STEAMUSER_INTERFACE_VERSION+1)
          steam_log->Log (L" >> Newer than Special K knows about.");
      }

      if (i == INTERNAL_STEAMUSER_INTERFACE_VERSION)
        break;
    }
  }

  if (i != INTERNAL_STEAMUSER_INTERFACE_VERSION)
  {
    steam_log->Log ( L" >> ISteamUser NOT FOUND for version %hs <<",
                   STEAMUSER_INTERFACE_VERSION );
    user_ = nullptr;
    return false;
  }

  user_ex_ =
    (ISteamUser004_Light *)client_->GetISteamUser (
      hSteamUser, hSteamPipe,
        "SteamUser004"
    );

  // Ensure the Steam overlay injects and behaves in a sane way.
  //
  //  * Refer to the non-standard interface definition above for more.
  //
  if (config.steam.online_status != -1)
    SK_Steam_ConnectUserIfNeeded (user_->GetSteamID ());


  // Blacklist of people not allowed to use my software (for being disruptive to other users)
  //uint32_t aid = user_->GetSteamID ().GetAccountID    ();
  //uint64_t s64 = user_->GetSteamID ().ConvertToUint64 ();

  //
  // Obsolete now, but will remain here for historical purposes.
  //
  //   I've nothing to hide, these users were not people I held personal grudges against, but rather
  //     that hijacked my support thread before I was able to moderate them.
  //
  //
  //if ( aid ==  64655118 || aid == 183437803 )
  //{
  //  SK_MessageBox ( L"You are not authorized to use this software",
  //                    L"Unauthorized User", MB_ICONWARNING | MB_OK );
  //  ExitProcess (0x00);
  //}

  friends_ =
    client_->GetISteamFriends (
      hSteamUser, hSteamPipe,
        STEAMFRIENDS_INTERFACE_VERSION
    );

  if (friends_ == nullptr)
  {
    steam_log->Log ( L" >> ISteamFriends NOT FOUND for version %hs <<",
                    STEAMFRIENDS_INTERFACE_VERSION );
    return false;
  }


  user_stats_ =
    client_->GetISteamUserStats (
      hSteamUser, hSteamPipe,
        STEAMUSERSTATS_INTERFACE_VERSION
    );

  if (user_stats_ == nullptr)
  {
    steam_log->Log ( L" >> ISteamUserStats NOT FOUND for version %hs <<",
                    STEAMUSERSTATS_INTERFACE_VERSION );
    return false;
  }

  apps_ =
    client_->GetISteamApps (
      hSteamUser, hSteamPipe,
        STEAMAPPS_INTERFACE_VERSION
    );

  if (apps_ == nullptr)
  {
    steam_log->Log ( L" >> ISteamApps NOT FOUND for version %hs <<",
                    STEAMAPPS_INTERFACE_VERSION );
    return false;
  }

  for (i = INTERNAL_STEAMUTILS_INTERFACE_VERSION+5; i > 0; --i)
  {
    utils_ =
      client_->GetISteamUtils (
        hSteamUser,
          SK_FormatString ("SteamUtils%03lu", i).c_str ()
      );

    if (utils_ != nullptr)
    {
      if (utils_ver_ == 0)
      {
        utils_ver_ = i;
        steam_log->Log (L"SteamAPI DLL Implements Utils v%03lu", i);

        if (utils_ver_ > NEWEST_STEAMUTILS_INTERFACE_VERSION)
          steam_log->Log (L" >> Newer than Special K knows about.");
      }

      if (i == INTERNAL_STEAMUTILS_INTERFACE_VERSION)
        break;
    }
  }

  if (i != INTERNAL_STEAMUTILS_INTERFACE_VERSION)
  {
    steam_log->Log ( L" >> ISteamUtils NOT FOUND for version %hs <<",
                   STEAMUTILS_INTERFACE_VERSION );
    utils_ = nullptr;
    return false;
  } else utils_->SetWarningMessageHook ( &SK_SteamAPI_DebugText );

  screenshots_ =
    client_->GetISteamScreenshots ( hSteamUser,
                                    hSteamPipe,
                                     STEAMSCREENSHOTS_INTERFACE_VERSION );

  // We can live without this...
  if (screenshots_ == nullptr)
  {
    steam_log->Log ( L" >> ISteamScreenshots NOT FOUND for version %hs <<",
                            STEAMSCREENSHOTS_INTERFACE_VERSION );

    screenshots_ =
      client_->GetISteamScreenshots (
        hSteamUser, hSteamPipe,
          "STEAMSCREENSHOTS_INTERFACE_VERSION001"
      );

    steam_log->Log ( L" >> ISteamScreenshots NOT FOUND for version %hs <<",
                      "STEAMSCREENSHOTS_INTERFACE_VERSION001" );
  }

  // This crashes Dark Souls 3, so... do this
  music_ =
    SAFE_GetISteamMusic ( client_,
        hSteamUser, hSteamPipe,
          STEAMMUSIC_INTERFACE_VERSION
    );

  SK::SteamAPI::player = user_->GetSteamID ();

  controller_ =
    SAFE_GetISteamController ( client_,
      hSteamUser, hSteamPipe,
        "SteamController005"
    );

  for (i = INTERNAL_STEAMREMOTESTORAGE_INTERFACE_VERSION+2; i > 0; --i)
  {
    remote_storage_ =
      SAFE_GetISteamRemoteStorage ( client_,
        hSteamUser, hSteamPipe,
          SK_FormatString ("STEAMREMOTESTORAGE_INTERFACE_VERSION%03u", i).c_str ()
      );

    if (remote_storage_ != nullptr)
    {
      if (remote_storage_ver_ == 0)
      {
        remote_storage_ver_ = i;
        steam_log->Log (L"SteamAPI DLL Implements Remote Storage v%03lu", i);

        if (remote_storage_ver_ > 14)
          steam_log->Log (L" >> Newer than Special K knows about.");
      }

      if (i == INTERNAL_STEAMREMOTESTORAGE_INTERFACE_VERSION)
        break;
    }
  }

  if (i != INTERNAL_STEAMREMOTESTORAGE_INTERFACE_VERSION) remote_storage_ = nullptr;

  for (i = INTERNAL_STEAMUGC_INTERFACE_VERSION+1; i > 0; --i)
  {
    ugc_ =
      SAFE_GetISteamUGC ( client_,
        hSteamUser, hSteamPipe,
          SK_FormatString ("STEAMUGC_INTERFACE_VERSION%03u", i).c_str ()
      );

    if (ugc_ != nullptr)
    {
      if (ugc_ver_ == 0)
      {
        ugc_ver_ = i;
        steam_log->Log (L"SteamAPI DLL Implements UGC v%03lu", i);

        if (ugc_ver_ > INTERNAL_STEAMUGC_INTERFACE_VERSION)
          steam_log->Log (L" >> Newer than Special K knows about.");
      }

      if (i == INTERNAL_STEAMUGC_INTERFACE_VERSION)
        break;
    }
  }

  if (i != INTERNAL_STEAMUGC_INTERFACE_VERSION) ugc_ = nullptr;

#if 0
  void** vftable = *(void***)*(&controller_);
  SK_CreateVFTableHook ( L"ISteamController::GetControllerState",
                        vftable, 3,
                        ISteamController_GetControllerState_Detour,
                        (LPVOID *)&GetControllerState_Original );
#endif

  MH_QueueEnableHook (SteamAPI_Shutdown);
  MH_QueueEnableHook (SteamAPI_RunCallbacks);

  return true;
}

bool
__stdcall
SK::SteamAPI::IsOverlayAware (void)
{
  return (! overlay_activation_callbacks->empty ());
}

BOOL
SK_Steam_KickStart (const wchar_t* wszLibPath)
{
  static volatile LONG tried = FALSE;

  if (! InterlockedCompareExchange (&tried, TRUE, FALSE))
  {
    wchar_t wszDLLPath [MAX_PATH + 2] = { };

    auto _TryLoadModule =
     [&](const wchar_t* wszPath) ->
      HMODULE
      {
        HMODULE hModRet =
          skModuleRegistry::INVALID_MODULE;

        if (SK_File_GetSize (wszPath) > 0)
        {
           hModRet =
            SK_Modules->LoadLibraryLL (wszPath);

          if (hModRet != skModuleRegistry::INVALID_MODULE)
          {
            wcsncpy_s ( wszDLLPath, MAX_PATH,
                        wszPath,    _TRUNCATE );
          }
        }

        return hModRet;
      };

    HMODULE hModExplicit =
                nullptr != wszLibPath  ?
           _TryLoadModule (wszLibPath) : skModuleRegistry::INVALID_MODULE;

    if (hModExplicit == skModuleRegistry::INVALID_MODULE)
    {
      static const wchar_t* wszSteamDLL =
        SK_RunLHIfBitness (                          64,
          LR"(PlugIns\ThirdParty\Steamworks\steam_api64.dll)",
          LR"(PlugIns\ThirdParty\Steamworks\steam_api.dll)"
        );

      if (! SK_GetModuleHandle (wszSteamDLL))
      {
        if (SK_IsInjected ())
        {
          wcsncpy_s (                         wszDLLPath,  MAX_PATH,
                     SK_GetModuleFullName (SK_GetDLL ()).c_str (),
                                                              _TRUNCATE );
        }

        else
        {
          PathAppendW (wszDLLPath,      SK_GetDocumentsDir ().c_str ()   );
          PathAppendW (wszDLLPath, LR"(%s\My Mods\SpecialK\SpecialK.dll)");
        }

        if (PathRemoveFileSpec (wszDLLPath)            )
        {   PathAppendW        (wszDLLPath, wszSteamDLL);
          if (SK_File_GetSize  (wszDLLPath)      > 0ull) { hModExplicit =
                _TryLoadModule (wszDLLPath);
          }
        }
      }
    }

    if (hModExplicit != skModuleRegistry::INVALID_MODULE)
    {
      std::wstring privacy_aware_path (wszDLLPath);

      dll_log->Log ( L"[DLL Loader]   Manually booted SteamAPI: '%s'",
                     SK_StripUserNameFromPathW  (
                       privacy_aware_path.data ( )
                                                ) );

      config.steam.dll_path            = wszDLLPath;
      config.steam.force_load_steamapi = true;

      SK_HookSteamAPI ();

      return TRUE;
    }
  }

  return FALSE;
}


#define CLIENTENGINE_INTERFACE_VERSION  "CLIENTENGINE_INTERFACE_VERSION005"
#define CLIENTUSER_INTERFACE_VERSION    "CLIENTUSER_INTERFACE_VERSION001"
#define CLIENTFRIENDS_INTERFACE_VERSION "CLIENTFRIENDS_INTERFACE_VERSION001"
#define CLIENTAPPS_INTERFACE_VERSION    "CLIENTAPPS_INTERFACE_VERSION001"
#define CLIENTUTILS_INTERFACE_VERSION   "CLIENTUTILS_INTERFACE_VERSION001"

enum EAppState
{
  k_EAppStateInvalid        = 0,
  k_EAppStateUninstalled    = 1,
  k_EAppStateUpdateRequired = 2,
  k_EAppStateFullyInstalled = 4,
  k_EAppStateDataEncrypted  = 8,
  k_EAppStateSharedOnly     = 64,
  k_EAppStateDataLocked     = 16,
  k_EAppStateFilesMissing   = 32,
  k_EAppStateFilesCorrupt   = 128,
  k_EAppStateAppRunning     = 8192,
  k_EAppStateBackupRunning  = 4096,
  k_EAppStateUpdateRunning  = 256,
  k_EAppStateUpdateStopping = 8388608,
  k_EAppStateUpdatePaused   = 512,
  k_EAppStateUpdateStarted  = 1024,
  k_EAppStateReconfiguring  = 65536,
  k_EAppStateAddingFiles    = 262144,
  k_EAppStateDownloading    = 1048576,
  k_EAppStateStaging        = 2097152,
  k_EAppStateCommitting     = 4194304,
  k_EAppStateUninstalling   = 2048,
  k_EAppStatePreallocating  = 524288,
  k_EAppStateValidating     = 131072,
};

enum EAppEvent
{
  k_EAppEventDownloadComplete = 2,
};

enum EAppInfoSection
{
  k_EAppInfoSectionUnknown = 0,
  k_EAppInfoSectionAll,
  k_EAppInfoSectionCommon,
  k_EAppInfoSectionExtended,
  k_EAppInfoSectionConfig,
  k_EAppInfoSectionStats,
  k_EAppInfoSectionInstall,
  k_EAppInfoSectionDepots,
  k_EAppInfoSectionVac,
  k_EAppInfoSectionDrm,
  k_EAppInfoSectionUfs,
  k_EAppInfoSectionOgg,
  k_EAppInfoSectionItems,
  k_EAppInfoSectionPolicies,
  k_EAppInfoSectionSysreqs,
  k_EAppInfoSectionCommunity
};

class IClientUtils_001
{
public:
  virtual const char*      GetInstallPath                (void) = 0;
  virtual const char*      GetUserBaseFolderInstallImage (void) = 0;
  virtual const char*      GetManagedContentRoot         (void) = 0;

  virtual       uint32     GetSecondsSinceAppActive      (void) = 0;
  virtual       uint32     GetSecondsSinceComputerActive (void) = 0;
  virtual       void       SetComputerActive             (void) = 0;

  virtual       EUniverse  GetConnectedUniverse (void) = 0;
  virtual       uint32     GetServerRealTime    (void) = 0;
  virtual const char*      GetIPCountry         (void) = 0;

  virtual bool             GetImageSize           (int32   iImage, uint32* pnWidth, uint32* pnHeight)        = 0;
  virtual bool             GetImageRGBA           (int32   iImage, uint8*  pubDest, int32   nDestBufferSize) = 0;
  virtual bool             GetCSERIPPort          (uint32* unIP,   uint16* usPort)                           = 0;
  virtual uint32           GetNumRunningApps      (void)          = 0;
  virtual uint8            GetCurrentBatteryPower (void)          = 0;
  virtual void             SetOfflineMode         (bool bOffline) = 0;
  virtual bool             GetOfflineMode         (void)          = 0;

  virtual AppId_t SetAppIDForCurrentPipe (AppId_t nAppID, bool bTrackProcess) = 0;
};

class IClientApps_001
{
public:
  virtual int32   GetAppData                  (      AppId_t  unAppID, const char*           pchKey,    char*  pchValue,
                                                     int32    cchValueMax                                                    ) = 0;
  virtual bool    SetLocalAppConfig           (      AppId_t  unAppID,       uint8*          pchBuffer, int32  cbBuffer      ) = 0;
  virtual AppId_t GetInternalAppIDFromGameID  (      CGameID  nGameID                                                        ) = 0;
  virtual int32   GetAllOwnedMultiplayerApps  (      uint32*  punAppIDs,     int32           cAppIDsMax                      ) = 0;
  virtual int32   GetAppDataSection           (      AppId_t  unAppID,       EAppInfoSection eSection,  uint8* pchBuffer,
                                                     int32    cbBufferMax,   bool            bSharedKVSymbols                ) = 0;
  virtual bool    RequestAppInfoUpdate        (const AppId_t* pAppIDs,       int32           nNumAppIDs                      ) = 0;
  virtual void    NotifyAppEventTriggered     (      AppId_t  unAppID,       EAppEvent       eAppEvent                       ) = 0;
  virtual int32   GetDLCCount                 (      AppId_t  unAppID                                                        ) = 0;
  virtual bool    BGetDLCDataByIndex          (      AppId_t  unAppID,       int32           iDLC,
                                                     AppId_t* pDlcAppID,     bool*           pbAvailable,
                                                     char*    pchName,       int32           cchNameBufferSize               ) = 0;
};

class IClientUser_001_Min
{
public:
  virtual HSteamUser  GetHSteamUser                         (void)                                                               = 0;

  virtual void        LogOn                                 (bool bInteractive, CSteamID steamID)                                = 0;
  virtual void        LogOnWithPassword                     (bool bInteractive, const char * pchLogin, const char * pchPassword) = 0;
  virtual void        LogOnAndCreateNewSteamAccountIfNeeded (bool bInteractive)                                                  = 0;

  virtual EResult     LogOnConnectionless                   (void)                                                               = 0;
  virtual void        LogOff                                (void)                                                               = 0;
  virtual bool        BLoggedOn                             (void)                                                               = 0;
  virtual SK::SteamAPI::ELogonState
                      GetLogonState                         (void)                                                               = 0;
  virtual bool        BConnected                            (void)                                                               = 0;
  virtual bool        BTryingToLogin                        (void)                                                               = 0;
};

#define vf        virtual
#define SteamCall SteamAPICall_t

class IClientUser_001_Ext : IClientUser_001_Min
{
public:
  vf CSteamID    GetSteamID                       (void)                                                                            = 0;
  vf CSteamID    GetConsoleSteamID                (void)                                                                            = 0;

  vf bool        IsVACBanned                      (AppId_t nGameID)                                                                 = 0;
  vf bool        RequireShowVACBannedMessage      (AppId_t nAppID )                                                                 = 0;
  vf void        AcknowledgeVACBanning            (AppId_t nAppID )                                                                 = 0;

  vf bool        SetEmail                         (const char *pchEmail) = 0;

  vf bool        SetConfigString                  (DWORD eRegistrySubTree, const char *pchKey, const char *pchValue)                = 0;
  vf bool        GetConfigString                  (DWORD eRegistrySubTree, const char *pchKey,       char *pchValue, int32 cbValue) = 0;
  vf bool        SetConfigInt                     (DWORD eRegistrySubTree, const char *pchKey, int32  iValu)                        = 0;
  vf bool        GetConfigInt                     (DWORD eRegistrySubTree, const char *pchKey, int32 *pValue)                       = 0;

  vf bool        GetConfigStoreKeyName            (DWORD eRegistrySubTree, const char *pchKey, char *pchStoreName,
                                                   int32 cbStoreName)                                                               = 0;

  vf int32       InitiateGameConnection           (void  *pOutputBlob, int32  cbBlobMax,    CSteamID steamIDGS, CGameID gameID,
                                                   uint32 unIPServer,  uint16 usPortServer, bool     bSecure)                       = 0;
  vf int32       InitiateGameConnectionOld        (void  *pOutputBlob, int32  cbBlobMax,    CSteamID steamIDGS, CGameID gameID,
                                                   uint32 unIPServer,  uint16 usPortServer, bool     bSecure,
                                                   void  *pvSteam2GetEncryptionKey,         int32    cbSteam2GetEncryptionKey)      = 0;


  vf void        TerminateGameConnection          (uint32 unIPServer, uint16 usPortServer)                                          = 0;
  vf bool        TerminateAppMultiStep            (uint32, uint32)                                                                  = 0;

  vf void        SetSelfAsPrimaryChatDestination  (void)                                                                            = 0;
  vf bool        IsPrimaryChatDestination         (void)                                                                            = 0;

  vf void        RequestLegacyCDKey               (AppId_t iAppID)                                                                  = 0;

  vf bool        AckGuestPass                     (const char *pchGuestPassCode )                                                   = 0;
  vf bool        RedeemGuestPass                  (const char *pchGuestPassCode )                                                   = 0;

  vf uint32      GetGuestPassToGiveCount          (void)                                                                            = 0;
  vf uint32      GetGuestPassToRedeemCount        (void)                                                                            = 0;

  vf bool        GetGuestPassToGiveInfo           (uint32       nPassIndex,          GID_t   *pgidGuestPassID,
                                                   PackageId_t *pnPackageID,
                                                   RTime32     *pRTime32Created,     RTime32 *pRTime32Expiration,
                                                   RTime32     *pRTime32Sent,        RTime32 *pRTime32Redeemed,
                                                   char        *pchRecipientAddress, int32    cRecipientAddressSize)                = 0;
  vf bool        GetGuestPassToRedeemInfo         (uint32       nPassIndex,          GID_t   *pgidGuestPassID,
                                                   PackageId_t *pnPackageID,         RTime32 *pRTime32Created,
                                                   RTime32     *pRTime32Expiration,  RTime32 *pRTime32Sent,
                                                   RTime32     *pRTime32Redeemed)                                                   = 0;
  vf bool        GetGuestPassToRedeemSenderName   (uint32       nPassIndex,          char    *pchSenderName,
                                                    int32       cSenderNameSize)                                                    = 0;

  vf void        AcknowledgeMessageByGID          (const char *pchMessageGID)                                                       = 0;

  vf bool        SetLanguage                      (const char *pchLanguage)                                                         = 0;

  vf void        TrackAppUsageEvent               (      CGameID  gameID,      int32 eAppUsageEvent,
                                                   const char    *pchExtraInfo = "")                                                = 0;

  vf int32       RaiseConnectionPriority          (DWORD eConnectionPriority)                                                       = 0;
  vf void        ResetConnectionPriority          (int32 hRaiseConnectionPriorityPrev)                                              = 0;

  vf void        SetAccountNameFromSteam2         (const char *pchAccountName)                                                      = 0;
  vf bool        SetPasswordFromSteam2            (const char *pchPassword)                                                         = 0;

  vf bool        BHasCachedCredentials            (const char  pchUnk)                                                              = 0;
  vf bool        SetAccountName                   (const char *pchAccountName,       bool  bUnk)                                    = 0;
  vf void        SetLoginInformation              (const char *pchAccountName, const char *pchPassword,
                                                         bool  bRememberPassword)                                                   = 0;
  vf void        ClearAllLoginInformation         (void)                                                                            = 0;

  vf void        SetAccountCreationTime           (RTime32 rtime32Time)                                                             = 0;

  vf SteamCall   RequestWebAuthToken              (void)                                                                            = 0;
  vf bool        GetCurrentWebAuthToken           (char *pchBuffer,   int32 cubBuffer)                                              = 0;

  vf bool        GetLanguage                      (char *pchLanguage, int32 cbLanguage)                                             = 0;

  vf bool        BIsCyberCafe                     (void)                                                                            = 0;
  vf bool        BIsAcademicAccount               (void)                                                                            = 0;

  vf void        CreateAccount                    (const char *pchAccountName, const char *pchNewPassword,
                                                   const char *pchNewEmail,          int32 iQuestion,
                                                   const char *pchNewQuestion, const char *pchNewAnswer)                            = 0;
  vf SteamCall   ResetPassword                    (const char *pchAccountName, const char *pchOldPassword,
                                                   const char *pchNewPassword, const char *pchValidationCode,
                                                   const char *pchAnswer)                                                           = 0;

  vf void        TrackNatTraversalStat            (PVOID const pNatStat)                                                            = 0;

  vf void        TrackSteamUsageEvent             (DWORD eSteamUsageEvent, const uint8 *pubKV, uint32 cubKV)                        = 0;
  vf void        TrackSteamGUIUsage               (const char *)                                                                    = 0;

  vf void        SetComputerInUse                 (void)                                                                            = 0;

  vf bool        BIsGameRunning                   (CGameID gameID)                                                                  = 0;

  vf uint64      GetCurrentSessionToken           (void)                                                                            = 0;

  vf bool        BUpdateAppOwnershipTicket        (AppId_t nAppID, bool bOnlyUpdateIfStale, bool bIsDepot)                          = 0;

  vf bool        RequestCustomBinary              (const char *pszAbsolutePath, AppId_t nAppID,
                                                         bool  bForceUpdate,    bool    bAppLaunchRequest)                          = 0;
  vf EResult     GetCustomBinariesState           (AppId_t     unAppID,         uint32 *punProgress)                                = 0;
  vf EResult     RequestCustomBinaries            (AppId_t     unAppID,         bool, bool, uint32*)                                = 0;

  vf void        SetCellID                        (CellID_t cellID)                                                                 = 0;
  vf void        SetWinningPingTimeForCellID      (uint32    uPing)                                                                 = 0;

  vf const char *GetUserBaseFolder                 (void)                                                                           = 0;

  vf bool        GetUserDataFolder                 (CGameID  gameID,    char  *pchBuffer, int32 cubBuffer)                          = 0;
  vf bool        GetUserConfigFolder               (char    *pchBuffer, int32  cubBuffer)                                           = 0;
};

class IClientFriends_001_Min
{
public:
  // returns the local players name - guaranteed to not be NULL.
  virtual const char    *GetPersonaName   (void)                        = 0;

  // sets the player name, stores it on the server and publishes the changes
  // to all friends who are online
  virtual void           SetPersonaName   (const char *pchPersonaName)  = 0;
  virtual SteamAPICall_t SetPersonaNameEx (const char *pchPersonaName,
                                                 bool  bSendCallback)   = 0;

  virtual bool           IsPersonaNameSet (void)                        = 0;

  // gets the friend status of the current user
  virtual EPersonaState  GetPersonaState  (void)                        = 0;
  // sets the status, communicates to server, tells all friends
  virtual void           SetPersonaState  (EPersonaState ePersonaState) = 0;
};

class IClientEngine_005_Min
{
public:
  virtual HSteamPipe              CreateSteamPipe       (void)                                                                 = 0;
  virtual bool                    BReleaseSteamPipe     (HSteamPipe   hSteamPipe)                                              = 0;
  virtual HSteamUser              CreateGlobalUser      (HSteamPipe* phSteamPipe)                                              = 0;
  virtual HSteamUser              ConnectToGlobalUser   (HSteamPipe   hSteamPipe)                                              = 0;
  virtual HSteamUser              CreateLocalUser       (HSteamPipe* phSteamPipe, EAccountType eAccountType)                   = 0;
  virtual void                    CreatePipeToLocalUser (HSteamUser   hSteamUser, HSteamPipe* phSteamPipe  )                   = 0;
  virtual void                    ReleaseUser           (HSteamPipe   hSteamPipe, HSteamUser   hUser       )                   = 0;
  virtual bool                    IsValidHSteamUserPipe (HSteamPipe   hSteamPipe, HSteamUser   hUser       )                   = 0;

  virtual IClientUser_001_Min    *GetIClientUser        (HSteamUser hSteamUser, HSteamPipe hSteamPipe, char const* pchVersion) = 0;
  virtual LPVOID /*DONTCARE*/     GetIClientGameServer  (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) = 0;

  virtual void                    SetLocalIPBinding     (uint32    unIP, uint16 usPort)                                        = 0;
  virtual char const             *GetUniverseName       (EUniverse eUniverse          )                                        = 0;

  virtual IClientFriends_001_Min *GetIClientFriends     (HSteamUser hSteamUser, HSteamPipe hSteamPipe, char const* pchVersion) = 0;
  virtual IClientUtils_001       *GetIClientUtils       (HSteamPipe hSteamPipe,                        char const* pchVersion) = 0;

  // LOTS MORE DONT CARE...
};


void*
SK_SteamAPIContext::ClientEngine (void)
{
  using  CreateInterface_pfn = void* (__cdecl *)(const char *ver, int* pError);
  static CreateInterface_pfn SteamClient_CreateInterface = nullptr;

#ifdef _M_AMD64
  SteamClient_CreateInterface =
    (CreateInterface_pfn)
    SK_GetProcAddress ( SK_GetModuleHandle (L"steamclient64.dll"),
                       "CreateInterface" );
#else /* _M_IX86 */
  SteamClient_CreateInterface =
    (CreateInterface_pfn)
    SK_GetProcAddress ( SK_GetModuleHandle (L"steamclient.dll"),
                       "CreateInterface" );
#endif

  static IClientEngine_005_Min* client_engine = nullptr;

  if (SteamClient_CreateInterface != nullptr && client_engine == nullptr)
  {
    int err = 0;

    client_engine =
      (IClientEngine_005_Min *)SteamClient_CreateInterface (
        CLIENTENGINE_INTERFACE_VERSION, &err
      );

    if (client_engine == nullptr)
      client_engine = (IClientEngine_005_Min *)(uintptr_t)1;
  }

  return (uintptr_t)client_engine > 1 ? client_engine : nullptr;
}

void*
SK_SteamAPIContext::ClientUser (void)
{
  static auto* client_engine =
    (IClientEngine_005_Min *)ClientEngine ();

  if (client_engine != nullptr && client_ != nullptr)
  {
    static void* client_user = nullptr;

    if (client_user == nullptr && hSteamPipe != 0)
    {
      client_user =
        client_engine->GetIClientUser ( hSteamUser,
                                        hSteamPipe,
                                        CLIENTUSER_INTERFACE_VERSION );

      if (client_user == nullptr) client_user = (LPVOID)(uintptr_t)1;
    }

    return (uintptr_t)client_user > 1 ? client_user : nullptr;
  }

  return nullptr;
}

void*
SK_SteamAPIContext::ClientFriends (void)
{
  static auto* client_engine =
    (IClientEngine_005_Min *)ClientEngine ();

  assert (client_engine != nullptr);

  if (client_engine != nullptr && client_ != nullptr)
  {
    static void* friends = nullptr;

    if (friends == nullptr && hSteamPipe != 0)
    {
      friends =
        client_engine->GetIClientFriends ( hSteamUser, hSteamPipe,
                                           CLIENTFRIENDS_INTERFACE_VERSION );

      assert (friends != nullptr);

      if (friends == nullptr) friends = (LPVOID)(uintptr_t)1;
    }

    return (uintptr_t)friends > 1 ? friends : nullptr;
  }

  return nullptr;
}


AppId_t
SK_SteamAPIContext::ReassignAppIDForPipe ( HSteamPipe hPipe,
                                           AppId_t    nAppID,
                                           bool       bTrackProcess )
{
  static auto* client_engine =
    (IClientEngine_005_Min*)ClientEngine ();

  assert (client_engine != nullptr);

  if (client_engine != nullptr && client_ != nullptr)
  {
    IClientUtils_001* utils = nullptr;

    if (hPipe != 0)
    {
      utils =
        client_engine->GetIClientUtils (hPipe,
          CLIENTUTILS_INTERFACE_VERSION);

      assert (utils != nullptr);

      if (utils != nullptr)
        return
          utils->SetAppIDForCurrentPipe (nAppID, bTrackProcess);
    }
  }

  return SK_Steam_GetAppID_NoAPI ();
}

void*
SK_SteamAPIContext::ClientUtils (void)
{
  static auto* client_engine =
    (IClientEngine_005_Min*)ClientEngine ();

  assert (client_engine != nullptr);

  if (client_engine != nullptr && client_ != nullptr)
  {
    static void* utils = nullptr;

    if (utils == nullptr && hSteamPipe != 0)
    {
      utils =
        client_engine->GetIClientUtils (hSteamPipe,
          CLIENTUTILS_INTERFACE_VERSION);

      assert (utils != nullptr);

      if (utils == nullptr) utils = (LPVOID)(uintptr_t)1;
    }

    return (uintptr_t)utils > 1 ? utils : nullptr;
  }

  return nullptr;
}

void
SK::SteamAPI::SetPersonaState (EPersonaState state)
{
  auto* client_friends =
    static_cast <IClientFriends_001_Min *> (
        steam_ctx.ClientFriends ()
      );

  if (client_friends)
  {
    client_friends->SetPersonaState (state);
                         TryRunCallbacks ();
  }
}

EPersonaState
SK::SteamAPI::GetPersonaState (void)
{
  auto* client_friends =
    static_cast <IClientFriends_001_Min *> (
        steam_ctx.ClientFriends ()
      );

  if (client_friends)
  {
    return
      client_friends->GetPersonaState ();
  }

  return
    steam_ctx.Friends () ? steam_ctx.Friends ()->GetPersonaState () :
    k_EPersonaStateOffline;
}


bool
SK_Steam_IsClientUser (CSteamID user)
{
  return
    ( steam_ctx.User () != nullptr     &&
      steam_ctx.User ()->GetSteamID () == user );
}

SK::SteamAPI::ISteamUser004_Light*
SK_Steam_GetUser004 (CSteamID user)
{
  return
    ( SK_Steam_IsClientUser   (user) ?
             steam_ctx.UserEx (    ) : nullptr );
}

bool
SK_Steam_IsUserConnected (CSteamID user)
{
  SK::SteamAPI::ISteamUser004_Light*
    user_ex = SK_Steam_GetUser004 (user);

  return
    ( user_ex != nullptr     ?
      user_ex->BConnected () : false );
}

bool
SK_Steam_LogOn (CSteamID user)
{
  SK::SteamAPI::ISteamUser004_Light*
    user_ex = SK_Steam_GetUser004 (user);

  if (user_ex != nullptr)
  {
      user_ex->LogOn (user);

      if (SteamAPI_RunCallbacks_Original != nullptr)
                TryRunCallbacks ();
  }

  return
    ( user_ex != nullptr );
}

//
// Steam may not inject the Steam overlay in some cases without this,
//   it may also pull the steamclient{64}.dll rug out from under our feet
//     at startup without it.
//
bool
SK_Steam_ConnectUserIfNeeded (CSteamID user)
{
  bool bRet = false;

  if (! SK_Steam_IsUserConnected (user))
  {
    bRet =
      SK_Steam_LogOn (user);
  }

  if (config.steam.online_status != -1)
    SK::SteamAPI::SetPersonaState ((EPersonaState)config.steam.online_status);

  return bRet;
}

bool
SK_SteamAPIContext::ReleaseThreadPipe (void)
{
  HSteamPipe& pipe = SK_TLS_Bottom ()->steam->client_pipe;

  if (pipe != 0 && client_ != nullptr)
  {
    if (client_->BReleaseSteamPipe (pipe))
    {
      pipe = 0;

      return true;
    }
  }

  return false;
}

bool
SK_SteamAPIContext::ReleaseThreadUser (void)
{
  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  HSteamUser& user = pTLS->steam->client_user;
  HSteamPipe& pipe = pTLS->steam->client_pipe;

  if (user != 0 && client_ != nullptr)
  {
    client_->ReleaseUser (pipe, user);
    user = 0;

    return true;
  }

  return false;
}


std::string
SK::SteamAPI::GetConfigDir (void)
{
  auto* user =
    steam_ctx.ClientUser ();

  if (user != nullptr)
  {
    char szDir [MAX_PATH + 2] = { };

    if (((IClientUser_001_Ext *)user)->GetUserConfigFolder (szDir, MAX_PATH))
      return szDir;
  }

  return
    std::string ();
}

std::string
SK::SteamAPI::GetDataDir (void)
{
  auto* user =
    steam_ctx.ClientUser ();

  if (user != nullptr)
  {
    char szDir [MAX_PATH + 2] = { };

    if (((IClientUser_001_Ext *)user)->GetUserDataFolder (CGameID (SK::SteamAPI::AppID ()), szDir, MAX_PATH))
      return szDir;
  }

  return
    std::string ();
}

uint32_t
SK_Steam_GetAppID_NoAPI (void)
{
  static constexpr int MAX_APPID_LEN = 32;

  DWORD    dwSteamGameIdLen                  =  0 ;
  uint32_t AppID                             =  0 ;
  char     szSteamGameId [MAX_APPID_LEN + 1] = { };

  // First, look for steam_appid.txt
  if (GetFileAttributesW (L"steam_appid.txt") != INVALID_FILE_ATTRIBUTES)
  { FILE* fAppID = fopen ( "steam_appid.txt", "r" );
    if (  fAppID != nullptr  )
    {
      fgets (szSteamGameId, MAX_APPID_LEN, fAppID);
                                   fclose (fAppID);

      dwSteamGameIdLen = (DWORD)strlen (szSteamGameId);
      AppID            =        strtol (szSteamGameId, nullptr, 0);
    }
  }

  // If no steam_appid.txt file exists, the Steam client exports the
  //   running game's AppID as an environment variable when launched.
  if (AppID == 0)
  {
    dwSteamGameIdLen =
      GetEnvironmentVariableA ( "SteamGameId",
                                  szSteamGameId,
                                    MAX_APPID_LEN );


    if (dwSteamGameIdLen > 1)
      AppID = strtol (szSteamGameId, nullptr, 0);
  }

  // If we have an AppID, stash it in the config file for future use
  if (AppID != 0)
    config.steam.appid = AppID;

  if (dwSteamGameIdLen > 1 && config.steam.appid != 0)
  {
    SK_GetConfigPathEx (true);

    app_cache_mgr->loadAppCacheForExe (SK_GetFullyQualifiedApp ());
    app_cache_mgr->addAppToCache
    (
      SK_GetFullyQualifiedApp (),
                SK_GetHostApp (),
            SK_UTF8ToWideChar (
             SK_UseManifestToGetAppName   (AppID)
                               ).c_str (), AppID
    );

    app_cache_mgr->saveAppCache       ();//true);
    app_cache_mgr->loadAppCacheForExe (SK_GetFullyQualifiedApp ());

    // Trigger profile migration if necessary
    app_cache_mgr->getConfigPathForAppID (AppID);

    return
      config.steam.appid;
  }

  return 0;
}

uint64_t    SK::SteamAPI::steam_size = 0ULL;

// Must be global for x86 ABI problems
CSteamID    SK::SteamAPI::player      (0ULL);