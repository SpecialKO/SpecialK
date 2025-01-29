// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
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
#include <SpecialK/storefront/achievements.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L" SteamAPI "

// We're not going to use DLL Import - we will load these function pointers
//  by hand.
#define STEAM_API_NODLL
#include <SpecialK/steam_api.h>

#ifndef __cpp_lib_format
#define __cpp_lib_format
#endif

#include <format>

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

  SteamAPI_ManualDispatch_Init_pfn
  SteamAPI_ManualDispatch_Init         = nullptr,
  SteamAPI_ManualDispatch_Init_Original= nullptr;

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

  SteamAPI_ISteamInput_Init_pfn
  SteamAPI_ISteamInput_Init_Original   = nullptr;
};

         LPVOID pfnSteamInternal_CreateInterface = nullptr;
volatile LONG __SK_Steam_Downloading             = 0;

static auto constexpr
_ConstructPath =
[](auto&& path_base, const wchar_t* path_end)
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
            sk::narrow_cast <LONG> (fd.nFileSizeHigh) }.QuadPart;
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

using steam_library_t = wchar_t [MAX_PATH + 2];

int
SK_Steam_GetLibraries (steam_library_t** ppLibraries = nullptr);

std::wstring
SK_Steam_GetApplicationManifestPath (AppId64_t appid)
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
                   LR"(%s\steamapps\appmanifest_%llu.acf)",
               (wchar_t *)steam_lib_paths [i],
                            appid );

      SK_AutoHandle hManifest (
        CreateFileW ( wszManifest,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                          nullptr,        OPEN_EXISTING,
                            FILE_FLAG_SEQUENTIAL_SCAN,
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
SK_GetManifestContentsForAppID (AppId64_t appid)
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
        std::move (szManifestData.release ());

      return
        manifest;
    }
  }

  return manifest;
}

std::wstring
SK_Steam_FindInstallPath (AppId64_t appid)
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
        SK_UTF8ToWideChar (install_path_utf8);

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
  if (max_size == 0 || config.platform.silent)
    return false;

  const wchar_t* wszSteamLib =
    SK_RunLHIfBitness ( 64, L"steam_api64.dll",
                            L"steam_api.dll"    );

  static wchar_t
    dll_file [MAX_PATH + 2] = { };

  if (*dll_file != L'\0')
  {
    // Already have a working DLL
    if (SK_IsModuleLoaded (dll_file))
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

  //
  // This will load, hook and initialize SteamAPI... it should
  //   only be done if the user (or Steam client) has assigned
  //     the game an AppID.
  //
  //  Otherwise it will create a lot of idle steamclient{64}.dll
  //    threads for no reason.
  //
  if (SK::SteamAPI::AppID () + config.steam.appid > 0)
  {  // NOTE: non-Steam games are assigned negative AppIDs,
     //         and cannot initialize SteamAPI.
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
    if (                                     *dll_file != L'\0')
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

    if (StrStrIW (wszExecutablePath, LR"(SteamApps\common\)"))
    {
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
    }
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

using  WaitForSingleObject_pfn = DWORD (WINAPI *)(HANDLE,DWORD);
static WaitForSingleObject_pfn
       WaitForSingleObject_Original = nullptr;

DWORD
WINAPI
WaitForSingleObject_Detour (
  _In_ HANDLE  hHandle,
  _In_ DWORD  dwMilliseconds )
{
  // Steam overlay hook mutex wait
  if (dwMilliseconds == 100)
  {
    static HMODULE hModSteam = SK_GetModuleHandleW (
      SK_RunLHIfBitness ( 64, L"GameOverlayRenderer64.dll",
                              L"GameOverlayRenderer.dll" )
    );

    if (SK_GetCallingDLL () == hModSteam)
    {
      if (config.steam.disable_overlay)
      {
        SK_RunOnce (return 0);
        return WAIT_TIMEOUT;
      }
    }
  }

  return
    WaitForSingleObject_Original (hHandle, dwMilliseconds);
}

BOOL
SK_Steam_PreHookCore (const wchar_t* wszTry)
{
  static volatile LONG init = FALSE;

  if (InterlockedCompareExchange (&init, TRUE, FALSE))
    return TRUE;

  //SK_CreateDLLHook2 (     L"kernel32.dll",
  //                          "CreateMutexA",
  //                           CreateMutexA_Detour,
  //  static_cast_p2p <void> (&CreateMutexA_Original) );

  SK_CreateDLLHook2 (     L"kernel32.dll",
                            "WaitForSingleObject",
                             WaitForSingleObject_Detour,
    static_cast_p2p <void> (&WaitForSingleObject_Original) );

  SK_ApplyQueuedHooks ();

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

#if 0
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
#endif

  if ( SK_GetProcAddress (
         SK_Modules->LoadLibraryLL (wszSteamLib),
           "SteamInternal_CreateInterface" )
     )
  {
#if 0
    SK_CreateDLLHook2 (     wszSteamLib,
                              "SteamInternal_CreateInterface",
                               SteamInternal_CreateInterface_Detour,
      static_cast_p2p <void> (&SteamInternal_CreateInterface_Original),
                           &pfnSteamInternal_CreateInterface );
    SK_QueueEnableHook (    pfnSteamInternal_CreateInterface );
#endif

    return TRUE;
  }

  return FALSE;
}


extern bool
SK_D3D11_CaptureScreenshot (SK_ScreenshotStage when, bool allow_sound, std::string title);

extern bool
SK_D3D12_CaptureScreenshot (SK_ScreenshotStage when, bool allow_sound, std::string title);

extern bool
SK_D3D9_CaptureScreenshot  (SK_ScreenshotStage when, bool allow_sound, std::string title);

extern bool
SK_GL_CaptureScreenshot    (SK_ScreenshotStage when, bool allow_sound, std::string title);

SK_Steam_ScreenshotManager::~SK_Steam_ScreenshotManager (void)
{
  for ( int i = 0 ; i < _StatusTypes ; ++i )
  {
    if (           hSigReady [i] != INVALID_HANDLE_VALUE)
    {    SetEvent (hSigReady [i]);
   SK_CloseHandle (hSigReady [i]);
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
              SK_ScreenshotStage::EndOfFrame : SK_ScreenshotStage::BeforeOSD, true, ""
    );

    return;
  }

  else if ( (int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::D3D12 )
  {
    SK_D3D12_CaptureScreenshot (
      config.screenshots.show_osd_by_default ?
              SK_ScreenshotStage::EndOfFrame : SK_ScreenshotStage::BeforeOSD, true, ""
    );

    return;
  }

  else if ( (int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::OpenGL )
  {
    SK_GL_CaptureScreenshot (
      config.screenshots.show_osd_by_default ?
              SK_ScreenshotStage::EndOfFrame : SK_ScreenshotStage::BeforeOSD, true, ""
    );

    return;
  }

  else if ( (int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::D3D9 )
  {
    SK_D3D9_CaptureScreenshot (
            config.screenshots.show_osd_by_default ?
              SK_ScreenshotStage::EndOfFrame : SK_ScreenshotStage::BeforeOSD, true, ""
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
    SK_GetCurrentRenderBackend ().screenshot_mgr->getRepoStats (true);

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


#define CLIENTENGINE_INTERFACE_VERSION     "CLIENTENGINE_INTERFACE_VERSION005"
#define CLIENTENGINE_INTERFACE_VERSION_ALT "CLIENTENGINE_INTERFACE_VERSION006"
#define CLIENTUSER_INTERFACE_VERSION       "CLIENTUSER_INTERFACE_VERSION001"
#define CLIENTFRIENDS_INTERFACE_VERSION    "CLIENTFRIENDS_INTERFACE_VERSION001"
#define CLIENTAPPS_INTERFACE_VERSION       "CLIENTAPPS_INTERFACE_VERSION001"
#define CLIENTUTILS_INTERFACE_VERSION      "CLIENTUTILS_INTERFACE_VERSION001"

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

// Who knows...?
enum EUIMode
{
	k_EUIModeUnknown = 0,
	k_EUIModeMax     = DWORD_MAX
};

enum ELauncherType
{
	k_ELAuncherTypeUnknown = 0,
	k_ELAuncherTypeMax     = DWORD_MAX
};

enum EGameLaunchMethod
{
  k_EGameLaunchMethodUnknown = 0,
  k_EGameLaunchMethodMax     = DWORD_MAX
};

class IClientUtils_001
{
using UNKNOWN_RETURN = void;

public:
  virtual const char*      GetInstallPath                  (void) = 0;
  virtual const char*      GetUserBaseFolderInstallImage   (void) = 0;
  virtual const char*      GetManagedContentRoot           (void) = 0;

  virtual       uint32     GetSecondsSinceAppActive        (void) = 0;
  virtual       uint32     GetSecondsSinceComputerActive   (void) = 0;
  virtual       void       SetComputerActive               (void) = 0;

  virtual       EUniverse  GetConnectedUniverse            (void) = 0;
  virtual       uint32     GetServerRealTime               (void) = 0;
  virtual const char*      GetIPCountry                    (void) = 0;

  virtual       bool       GetImageSize                    (int32   iImage, uint32* pnWidth, uint32* pnHeight)        = 0;
  virtual       bool       GetImageRGBA                    (int32   iImage, uint8*  pubDest, int32   nDestBufferSize) = 0;
  virtual       bool       GetCSERIPPort                   (uint32* unIP,   uint16* usPort)                           = 0;
  virtual       uint32     GetNumRunningApps               (void)          = 0;
  virtual       uint8      GetCurrentBatteryPower          (void)          = 0;
  virtual       void       SetOfflineMode                  (bool bOffline) = 0;
  virtual       bool       GetOfflineMode                  (void)          = 0;

  virtual       AppId_t    SetAppIDForCurrentPipe          (AppId_t nAppID, bool bTrackProcess) = 0; // Disabled in late 2019
  virtual       AppId_t    GetAppID                        (void)                               = 0;

  virtual UNKNOWN_RETURN   SetAPIDebuggingActive           (bool, bool)   = 0;
  virtual UNKNOWN_RETURN   AllocPendingAPICallHandle       (void)         = 0;
  virtual   bool           IsAPICallCompleted              (unsigned long long, bool*) = 0;
  virtual UNKNOWN_RETURN   GetAPICallFailureReason         (unsigned long long)        = 0;
  virtual UNKNOWN_RETURN   GetAPICallResult                (unsigned long long, void*, int, int, bool*) = 0;
  virtual UNKNOWN_RETURN
                  SetAPICallResultWithoutPostingCallback   (unsigned long long, void const*, int, int)  = 0;
  virtual UNKNOWN_RETURN   SignalAppsToShutDown            (void)         = 0;
  virtual UNKNOWN_RETURN   SignalServiceAppsToDisconnect   (void)         = 0;
  virtual UNKNOWN_RETURN   TerminateAllAppsMultiStep       (unsigned int) = 0;
  virtual UNKNOWN_RETURN   GetCellID                       (void)         = 0;
  virtual   bool           BIsGlobalInstance               (void)         = 0;
  virtual UNKNOWN_RETURN   CheckFileSignature              (char const*)  = 0;
  virtual UNKNOWN_RETURN   GetBuildID                      (void)         = 0;

  virtual UNKNOWN_RETURN   SetCurrentUIMode                (EUIMode)       = 0;
  virtual   EUIMode        GetCurrentUIMode                (void)          = 0;
  virtual UNKNOWN_RETURN   ShutdownLauncher                (bool, bool)    = 0;
  virtual UNKNOWN_RETURN   SetLauncherType                 (ELauncherType) = 0;
  virtual   ELauncherType  GetLauncherType                 (void)          = 0;

  virtual UNKNOWN_RETURN   ShowGamepadTextInput            (EGamepadTextInputMode, EGamepadTextInputLineMode,
                                                               char const*, unsigned int, char const*) = 0;
  virtual UNKNOWN_RETURN   GetEnteredGamepadTextLength     (void)                                      = 0;
  virtual UNKNOWN_RETURN   GetEnteredGamepadTextInput      (char*, unsigned int)                       = 0;
  virtual UNKNOWN_RETURN   GamepadTextInputClosed          (int, bool, char const*)                    = 0;
  virtual UNKNOWN_RETURN   SetSpew                         (int, int, int)                             = 0;
  virtual   bool           BDownloadsDisabled              (void)                                      = 0;

  // Use this instead of steam://forceinputappid/... whenever possible
  virtual UNKNOWN_RETURN   SetFocusedWindow                (CGameID, bool)                             = 0;
  
  virtual UNKNOWN_RETURN   GetSteamUILanguage              (void)                                      = 0;
  virtual UNKNOWN_RETURN   CheckSteamReachable             (void)                                      = 0;
  virtual UNKNOWN_RETURN   SetLastGameLaunchMethod         (EGameLaunchMethod)                         = 0;
  virtual UNKNOWN_RETURN   SetVideoAdapterInfo             (int, int, int, int, int, int, char const*) = 0;

  // Steam Input stuff, needs reverse engineering...
  virtual UNKNOWN_RETURN   SetControllerOverrideMode       (CGameID, char const*, unsigned int)        = 0;
  virtual UNKNOWN_RETURN   SetOverlayWindowFocusForPipe    (bool, bool, CGameID)                       = 0;
  virtual UNKNOWN_RETURN
                     GetGameOverlayUIInstanceFocusGameID   (bool*)                                     = 0;
  virtual UNKNOWN_RETURN   SetControllerConfigFileForAppID (unsigned int, char const*)                 = 0;
  virtual UNKNOWN_RETURN   GetControllerConfigFileForAppID (unsigned int, char*, unsigned int)         = 0;

  virtual   bool           IsSteamRunningInVR              (void)                                      = 0;
  virtual   bool           BIsRunningOnAlienwareAlpha      (void)                                      = 0;
  virtual UNKNOWN_RETURN   StartVRDashboard                (void)                                      = 0;
  virtual UNKNOWN_RETURN   IsVRHeadsetStreamingEnabled     (unsigned int)                              = 0;
  virtual UNKNOWN_RETURN   SetVRHeadsetStreamingEnabled    (unsigned int, bool)                        = 0;
  virtual UNKNOWN_RETURN   GenerateSupportSystemReport     (void)                                      = 0;
  virtual UNKNOWN_RETURN   GetSupportSystemReport          (char*, unsigned int, unsigned char*,
                                                                                 unsigned int)         = 0;

  virtual   AppId_t        GetAppIdForPid                  (unsigned int, bool)                        = 0;
  virtual UNKNOWN_RETURN   SetClientUIProcess              (void)                                      = 0;
  virtual   bool           BIsClientUIInForeground         (void)                                      = 0;
};

class SK_Steam_OverlayManager
{
public:
  SK_Steam_OverlayManager (void) :
    activation  ( this, &SK_Steam_OverlayManager::OnActivate              ),
    steam_input ( this, &SK_Steam_OverlayManager::OnSteamInputStateChange )
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
                   OnSteamInputStateChange,
                   SteamInputConfigurationLoaded_t,
                   steam_input )
  {
    if (config.input.gamepad.steam.disabled_to_game)
    {
      steam_ctx.Input ()->Shutdown ();
      SK_Steam_ForceInputAppId (SPECIAL_KILLER_APPID);
      return;
    }

    //// Listen for changes to the desktop appid, we may want to ignore them.
    if (pParam->m_unAppID != config.steam.appid)
    {
      // Background Input is enabled, undo what the Steam client just did...
      if ( config.input.gamepad.disabled_to_game == SK_InputEnablement::Enabled &&
           config.window.background_render )
      {
        if (! SK::SteamAPI::SetWindowFocusState (true))
        {
          // Prevent recursion if two games are running at once,
          //   both with background input mode enabled...
          static HWND        hWndLastForeground = 0;
          if (std::exchange (hWndLastForeground, SK_GetForegroundWindow ())
                          != hWndLastForeground)
          {
            // Special K's AppId is a special case to block input altogether,
            //   allow it to pass through unchallenged.
            if (pParam->m_unAppID != SPECIAL_KILLER_APPID)
            {
              SK_LOGi0 (
                L"Forced AppID Change For Background Render Mode; "
                L"Steam loaded controller profile for AppId=%d",
                  pParam->m_unAppID
              );

              SK_Steam_ForceInputAppId (config.steam.appid);
            }
          }
        }
      }
    }
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
    if (config.platform.reuse_overlay_pause && SK::SteamAPI::IsOverlayAware ())
    {
      // Deactivating, but we might want to hide this event from the game...
      if (pParam->m_bActive == 0)
      {
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
#define SK_STEAM_HIDE(type)  SK_Steam_Backend->markHidden (type);

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
    case IPCFailure_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Installed IPC Failure Callback",
                      caller.c_str () );
      break;
    default:
    {
      steam_log->Log ( L" * (%-28s) Installed Callback (Class=%hs, Id=%li)",
                      caller.c_str (), SK_Steam_ClassifyCallback (iCallback), iCallback % 100 );
    } break;
  }

  if (SK_IsAddressExecutable             (pCallback))
  {
    if (true || //(! config.platform.achievements.pull_friend_stats) //< Temporarily skip while achievement popups are not implemented
                 iCallback != UserStatsReceived_t::k_iCallback)
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
      break;
    case GetAuthSessionTicketResponse_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled Auth Session Ticket Response Callback",
                      caller.c_str () );
      break;
    case IPCFailure_t::k_iCallback:
      steam_log->Log ( L" * (%-28s) Uninstalled IPC Failure Callback",
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


ISteamFriends*
SK_SteamAPI_Friends (void)
{
  return steam_ctx.Friends ();
}

void
SK_Steam_SetNotifyCorner (void)
{
  // 4 == Don't Care
  if (config.platform.notify_corner != 4)
  {
    auto pSteamUtils =
      steam_ctx.Utils ();

    if (pSteamUtils != nullptr)
    {
      // This is a very high overhead API call, and the
      //   chances of the Steam overlay enabling itself
      //     later are almost nil.
      static bool bEnabled =
        pSteamUtils->IsOverlayEnabled ();

      if (bEnabled)
      {
        pSteamUtils->SetOverlayNotificationPosition (
          static_cast <ENotificationPosition> (config.platform.notify_corner)
        );
      }
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
    input_          = nullptr;

    client_ver_         = 0;
    user_ver_           = 0;
    utils_ver_          = 0;
    remote_storage_ver_ = 0;
    input_ver_          = 0;

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

    config.platform.notify_corner =
      SK_Steam_PopupOriginStrToEnum (*reinterpret_cast <char **> (val));

    strncpy_s (                      var_strings.notify_corner,  16,
      SK_Steam_PopupOriginToStr (config.platform.notify_corner), _TRUNCATE
    );

    SK_Steam_SetNotifyCorner ();

    return true;
  }

  else if (var == popup_origin)
  {
    known = true;

    config.platform.achievements.popup.origin =
      SK_Steam_PopupOriginStrToEnum (*reinterpret_cast <char **> (val));

    strncpy_s ( var_strings.popup_origin,
        sizeof (var_strings.popup_origin),
                 SK_Steam_PopupOriginToStr (
                   config.platform.achievements.popup.origin
                ), _TRUNCATE
             );

    return true;
  }

  dll_log->Log ( L"[SteamAPI] UNKNOWN Variable Changed (%p --> %p)",
                 var, val );

  return false;
}

bool           has_global_data  = false;
LONG           next_friend      = 0;
LONG           friend_count     = 0;
SteamAPICall_t friend_query     = 0;
LONGLONG       friends_done     = 0;

class SK_Steam_AchievementManager : public SK_AchievementManager
{
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

    if ((! user_stats) || config.platform.steam_is_b0rked)
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
      std::max (user_stats->GetNumAchievements (), (uint32_t)64UL);
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
    unlock_listener.Unregister ();
    stat_receipt.Unregister    ();
    icon_listener.Unregister   ();
    async_complete.Unregister  ();
  }

  void log_all_achievements (void) const
  {
    ISteamUserStats* stats = steam_ctx.UserStats ();

    if (stats == nullptr || config.platform.steam_is_b0rked)
      return;

    for (uint32 i = 0; i < stats->GetNumAchievements (); i++)
    {
      const Achievement* achievement =
                         achievements.list [i];

      if (achievement == nullptr || achievement->name_.empty ())
        continue;

      steam_log->LogEx (false, L"\n [%c] Achievement %03lu......: '%hs'\n",
                        achievement->unlocked_ ? L'X' : L' ',
                        i, achievement->name_.c_str ());
      steam_log->LogEx (false,
                        L"  + Human Readable Name...: %ws\n",
                        achievement->text_.locked.human_name.c_str ());
      if (! achievement->text_.locked.desc.empty ())
        steam_log->LogEx (false,
                          L"  *- Detailed Description.: %ws\n",
                          achievement->text_.locked.desc.c_str ());

      if (achievement->global_percent_ > 0.0f)
        steam_log->LogEx (false,
                          L"  #-- Rarity (Global).....: %6.2f%%\n",
                          achievement->global_percent_);

      if (achievement->friends_.possible > 0)
        steam_log->LogEx ( false,
                          L"  #-- Rarity (Friend).....: %6.2f%%\n",
          100.0 * (static_cast <double> (achievement->friends_.unlocked) /
                   static_cast <double> (achievement->friends_.possible)) );

      if (achievement->unlocked_)
      {
        steam_log->LogEx (false,
                          L"  @--- Player Unlocked At.: %s",
                          _wctime64 (&achievement->time_));
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

    if (! (user && stats && friends && pParam && config.platform.steam_is_b0rked == false))
      return;

    auto user_id =
      user->GetSteamID ().ConvertToUint64 ();

    if (pParam->m_eResult == k_EResultOK)
    {
      if ( pParam->m_steamIDUser.ConvertToUint64 () ==
                            user_id )
      {
        pullStats ();

        if ( config.platform.achievements.pull_global_stats &&
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
            for ( auto& it : *UserStatsReceived_callbacks )
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
                  stats->GetNumAchievements ()
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
      if (SK_ValidatePointer (pParam, true) && SK_IsAddressExecutable (&pCallback, true))
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

    if (! (user && stats && friends && pParam && config.platform.steam_is_b0rked == false))
      return;

    auto user_id =
      user->GetSteamID ().ConvertToUint64 ();

#if 1
    for (auto& it : *UserStatsReceived_callbacks)
    {
      if (it.second)
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

          bool bAchieved = false;

          bool has_user_stats =
            stats->GetUserAchievement (
              pParam->m_steamIDUser,
                        name.c_str (), &bAchieved
            );

          if (! has_user_stats)
            break;

          if (bAchieved)
          {
            friend_stats [friend_idx].unlocked [i] = TRUE;

            unlocked++;

            // On the first unlocked achievement, make a note...
            if (! can_unlock)
            {
              ///steam_log->Log ( L"Received Achievement Stats for Friend: '%hs'",
              ///friends->GetFriendPersonaName (pParam->m_steamIDUser) );
            }

            can_unlock = true;

            ++(achievements.list [i]->friends_.unlocked);

            ///steam_log->Log (L" >> Has unlocked '%24hs'", szName);
          }

          else
            friend_stats [friend_idx].unlocked [i] = FALSE;
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
            ++(achievements.list [i]->friends_.possible);
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

        for ( auto& callback : plugin_mgr->achievement_unlock_fns )
        {
          callback (achievement);
        }

        if (config.platform.achievements.play_sound && (! unlock_sound.empty ()))
        {
          SK_PlaySound ( (LPCWSTR)unlock_sound.data (),
                                   nullptr, SND_ASYNC |
                                            SND_MEMORY );
        }

        steam_log->Log ( L" Achievement: '%ws' (%ws) - Unlocked!",
                           achievement->text_.unlocked.human_name.c_str (),
                           achievement->text_.unlocked.desc      .c_str ());

        // If the user wants a screenshot, but no popups (why?!), this is when
        //   the screenshot needs to be taken.
        if (       config.platform.achievements.take_screenshot )
        {  if ( (! config.platform.achievements.popup.show) )
           {
             SK::SteamAPI::TakeScreenshot (
               SK_ScreenshotStage::EndOfFrame, false,
                 SK_FormatString ("Achievements\\%ws", achievement->text_.unlocked.human_name.c_str ())
             );
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

        steam_log->Log ( L" Achievement: '%ws' (%ws) - "
                         L"Progress %lu / %lu (%04.01f%%)",
                           achievement->text_.locked.human_name.c_str (),
                           achievement->text_.locked.desc      .c_str (),
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

    if ( config.platform.achievements.popup.show
                      && achievement != nullptr )
    {
      if (platform_popup_cs != nullptr)
          platform_popup_cs->lock ();

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

      if (platform_popup_cs != nullptr)
          platform_popup_cs->unlock ();
    }
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

    if (stats && config.platform.steam_is_b0rked == false)
    {
      //
      // NOTE: Memory allocation works differently in SteamAPI than EOS,
      //         this needs to transition to addAchievement (...)
      //
      int unlocked = 0;

      if (achievements.list.size () < stats->GetNumAchievements ())
          achievements.list.resize (  stats->GetNumAchievements ());

      for (uint32 i = 0; i < stats->GetNumAchievements (); i++)
      {
        //steam_log->Log (L"  >> %hs", stats->GetAchievementName (i));

        auto* achievement =
          achievements.list [i];

        if ( achievement == nullptr   ||
             achievement->name_.empty () )
        {
          achievement = new
            Achievement (i,
              stats->GetAchievementName (i),
              stats
            );

          achievements.list       [i] =
                                   achievement;
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
                 achievement->name_.c_str ()
               );
          }

          //if (! achievements.list [i]->unlocked_)
          //stats->ClearAchievement (szName);
        }

        else
        {
          achievement->update (stats);
        }

        if ( achievement != nullptr &&
             achievement->unlocked_ )
          unlocked++;
      }

      total_unlocked   = unlocked;
      percent_unlocked =
        ( static_cast <float> (total_unlocked) /
          static_cast <float> (stats->GetNumAchievements ()) );
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

private:
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

  SteamAPICall_t global_request = { };
  SteamAPICall_t stats_request  = { };
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
  if (config.platform.silent)
    return;

  if (! steam_achievements)
  {
    steam_log->Log ( L" >> Steam Achievement Manager Not Yet Initialized ... "
                     L"Skipping Unlock <<" );
    return;
  }

  steam_log->LogEx (true, L" >> Attempting to Unlock Achievement: %lu... ",
                    idx );

  ISteamUserStats* stats =
    config.platform.steam_is_b0rked ? nullptr
                                    : steam_ctx.UserStats ();

  if ( stats                        && idx <
       stats->GetNumAchievements () )
  {
    const std::string name =
      stats->GetAchievementName (idx);

    if (! name.empty ())
    {
      steam_log->LogEx (false, L" (%hs - Found)\n", name.c_str ());

      steam_achievements->pullStats ();

      UserAchievementStored_t
               store                = { };
               store.m_nCurProgress =  0 ;
               store.m_nMaxProgress =  0 ;
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
    config.platform.steam_is_b0rked ? nullptr
                                    : steam_ctx.UserStats ();

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
  static bool failed = false;

  if (failed)
    return;

  __try {
    if (SteamAPI_RunCallbacks_Original != nullptr)
        SteamAPI_RunCallbacks_Original ();
  }
  __except ( EXCEPTION_EXECUTE_HANDLER ) {
    failed = true;

    SK_LOGi0 (
      L"Caught a Structured Exception (%x) during SteamAPI_RunCallbacks; stopping callbacks!",
        GetExceptionCode ()
    );
  }
}

void TryRunCallbacks (void)
{
  TryRunCallbacksSEH ();
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
        config.platform.steam_is_b0rked ? nullptr
                                        : steam_ctx.UserStats ();

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
  static const bool
      bOnlyRunOnMainThread = SK_IsCurrentGame (SK_GAME_ID::Starfield);
  if (bOnlyRunOnMainThread)
  {
    // Eliminate this call; main thread will pick up the slack
    if (SK_GetCurrentThreadId () != SK_GetMainThreadID ())
      return true;
  }

  if (ReadAcquire (&__SK_Steam_init))
  {
    InterlockedIncrement64 (&SK_SteamAPI_CallbackRunCount);
    // Might want to skip the callbacks; one call every 1 ms is enough!!

    static LARGE_INTEGER liLastCallbacked = { 0LL, 0LL };

    LONG limit = ReadAcquire (&SK_SteamAPI_CallbackRateLimit);
    if  (limit < 0) return false;

    if ( limit == 0 ||
      ( SK_CurrentPerf ().QuadPart - liLastCallbacked.QuadPart  <
                        SK_PerfFreq / limit ) )
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
    TryRunCallbacks ();

    return;
  }

  if (SK_Steam_ShouldThrottleCallbacks ())
  {
    return;
  }

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  if (pTLS != nullptr)
    InterlockedIncrement64 (&pTLS->steam->callback_count);

  if (! ReadAcquire (&__SK_DLL_Ending))
  {
    static std::atomic_bool
        try_me = true;
    if (try_me)
    { __try
      {
        DWORD dwNow = SK::ControlPanel::current_time;

        static DWORD
            dwLastOnlineStateCheck = 0UL;
        if (dwLastOnlineStateCheck < dwNow - 666UL)
        {
          SK_Steam_SetNotifyCorner ();

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

#if 0
        // TODO: Consider a special state in the control panel to indicate that Steam Input
        //         is enabled by Steam, but being actively disabled by SK using this setting.
        if (! config.input.gamepad.steam.disabled_to_game)
        {
          if (! SK_ImGui_WantGamepadCapture ())
          {
            if (auto pInput = steam_ctx.Input ())
            {
              const auto input_dev_type_mask =
                pInput->GetSessionInputConfigurationSettings ();

              if (input_dev_type_mask != k_ESteamInputConfigurationEnableType_None)
              {
                const bool known_type = 0 !=
                 ( input_dev_type_mask & ( k_ESteamInputConfigurationEnableType_Playstation |
                                           k_ESteamInputConfigurationEnableType_Xbox        |
                                           k_ESteamInputConfigurationEnableType_Generic     |
                                           k_ESteamInputConfigurationEnableType_Switch ) );

                if (known_type) [[likely]]
                {
                  if (input_dev_type_mask & k_ESteamInputConfigurationEnableType_Playstation)
                    SK_STEAM_VIEW (                   sk_input_dev_type::Gamepad_PlayStation);
                  if (input_dev_type_mask & k_ESteamInputConfigurationEnableType_Xbox)
                    SK_STEAM_VIEW (                   sk_input_dev_type::Gamepad_Xbox);
                  if (input_dev_type_mask & k_ESteamInputConfigurationEnableType_Generic)
                    SK_STEAM_VIEW (                   sk_input_dev_type::Gamepad_Generic);
                  if (input_dev_type_mask & k_ESteamInputConfigurationEnableType_Switch)
                    SK_STEAM_VIEW (                   sk_input_dev_type::Gamepad_Nintendo);
                }

                else
                {
                  SK_RunOnce (
                  {
                    wchar_t                               wszMessage [128] = { };
                    std::wstring_view     wszMessageView (wszMessage);
                    SK_FormatStringViewW (wszMessageView,
                      L"Steam Input appears to be broken in this game, consider turning it off."
                      L"\r\n\r\n\t\t Session Input Enable Config:\t%x", input_dev_type_mask);

                    SK_ImGui_Warning (wszMessageView.data ());
                  });
                }
              }
            }
          }
        }
#endif
      }

      __except ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                        EXCEPTION_EXECUTE_HANDLER  :
                                        EXCEPTION_CONTINUE_SEARCH )
      {
        // Log the failure, then stop running any custom SK callback logic
        steam_log->Log (
          L"Swallowed Structured Exception In SteamAPI_RunCallbacks (...) Hook!"
        );

        try_me = false;
      }
    }
  }

  // No matter what happens, always run the -actual- callbacks
  TryRunCallbacks ();
}


#define STEAMAPI_CALL1(x,y,z) ((x) = SteamAPI_##y z)
#define STEAMAPI_CALL0(y,z)   (SteamAPI_##y z)



// TODO: Remove
void
SK::SteamAPI::Init (bool pre_load)
{
  UNREFERENCED_PARAMETER (pre_load);

  if (config.platform.silent)
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
  static auto _Terminate =
  []
  {
    InterlockedExchangePointer ((void **)&hSteamPump, nullptr);

    SK_Thread_CloseSelf ();

    return 0;
  };

  static auto _TerminateOnSignal =
  [](DWORD dwWaitState)
  {
    if (dwWaitState != WAIT_TIMEOUT)
    {
      _Terminate ();

      return true;
    }

    return false;
  };

  static auto game_id =
    SK_GetCurrentGameID ();

  if (game_id == SK_GAME_ID::FinalFantasyXV)
  {
    return
      _Terminate ();
  }

  SetCurrentThreadDescription (               L"[SK] SteamAPI Callback Pump" );
  SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_IDLE );

  bool   start_immediately = (user != nullptr);
  double callback_freq     =  0.0;

  if ( game_id == SK_GAME_ID::JustCause3      ||
       game_id == SK_GAME_ID::YakuzaKiwami2   ||
       game_id == SK_GAME_ID::YakuzaUnderflow ||
       SK_IsModuleLoaded (
         SK_RunLHIfBitness (64, L"kaldaien_api64.dll",
                                L"kaldaien_api.dll" )
       )
     )
  {
    start_immediately = true;
  }

  if (! start_immediately)
  {
    // Since we might initialize this VERY early, wait for the game to start
    //   drawing frames before doing any kind of tests
    while (SK_GetFramesDrawn () < 1)
      SK_Sleep (250UL);

    // Wait 5 seconds, then begin a timing investigation
    DWORD dwWaitState =
      WaitForMultipleObjects ( 2,
           std::array <HANDLE, 2> { hSteamPumpKill,
                            __SK_DLL_TeardownEvent }.data (),
                              FALSE, config.steam.crapcom_mode ? 250 : 5000);
                              // Hurry this test up on CRAPCOM DRM (250 ms)

    if (_TerminateOnSignal (dwWaitState))
      return 0;

    // First, begin a timing probe.
    //
    //   If after 180 seconds the game has not called SteamAPI_RunCallbacks
    //     frequently enough, switch the thread to auto-pump mode.

    const UINT TEST_PERIOD =
      config.steam.crapcom_mode ? 1 : 180;
      // Hurry this test up on CRAPCOM DRM

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
    );SK_CloseHandle (hSteamPumpKill);
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

  if (pUser != nullptr && config.platform.steam_is_b0rked == false)
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

AppId64_t
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
    static          AppId64_t id   = 0;
    static volatile LONG      init = FALSE;

    if (InterlockedCompareExchange (&init, TRUE, FALSE) == FALSE)
    {
      id = ( utils != nullptr     ? static_cast <AppId64_t> (
             utils->GetAppID () ) : atoll (szSteamGameId) );

      if (id != 0 && id != SPECIAL_KILLER_APPID)
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

      if (id == SPECIAL_KILLER_APPID)
          id = 0;  // Special K's AppID is a mistake of some sort, ignore it

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
AppId64_t
__stdcall
SK_SteamAPI_AppID (void)
{
  return SK::SteamAPI::AppID ();
}

const wchar_t*
SK_GetSteamDir (void)
{
  static wchar_t
       wszSteamPath [MAX_PATH + 2] = { };
  if (*wszSteamPath == L'\0')
  {
    // Don't keep querying the registry if Steam is not installed
    wszSteamPath [0] = L'?';

    DWORD     len    =      MAX_PATH;
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
    else
      return L"";
  }

  return wszSteamPath;
}

HMODULE hModOverlay;

bool
SK_Steam_LoadOverlayEarly (void)
{
  // Initialize this early
  std::ignore = SK_Steam_GetLibraries (nullptr);

  const wchar_t* wszSteamPath =
    SK_GetSteamDir ();

  wchar_t           wszOverlayDLL [MAX_PATH + 2] = { };
  SK_PathCombineW ( wszOverlayDLL, wszSteamPath,
    SK_RunLHIfBitness ( 64, L"GameOverlayRenderer64.dll",
                            L"GameOverlayRenderer.dll"    ) );


  //bool bEnableHooks =
  //  SK_EnableApplyQueuedHooks ();
  //
  //SK_ApplyQueuedHooks ();
  //
  //if (! bEnableHooks)
  //  SK_DisableApplyQueuedHooks ();


  hModOverlay =
    SK_Modules->LoadLibrary (wszOverlayDLL);

  return hModOverlay != nullptr;
}

int
SK_Steam_GetLibraries (steam_library_t** ppLibraries)
{
#define MAX_STEAM_LIBRARIES 32

  static volatile LONG   scanned_libs = 0L;
  static int             steam_libs   = 0;
  static steam_library_t steam_lib_paths [MAX_STEAM_LIBRARIES] = { };

  if (! InterlockedCompareExchange (&scanned_libs, 1L, 0L))
  {
    const wchar_t* wszSteamPath = SK_GetSteamDir ();

    wchar_t   wszLibraryFolders [MAX_PATH + 2] = { };
    lstrcpyW (wszLibraryFolders, wszSteamPath);
    lstrcatW (wszLibraryFolders, LR"(\steamapps\libraryfolders.vdf)");

    CHandle hLibFolders (
      CreateFileW ( wszLibraryFolders,
                      GENERIC_READ,
                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                        nullptr,        OPEN_EXISTING,
                          FILE_FLAG_SEQUENTIAL_SCAN,
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
        std::make_unique <char []> (dwSize + static_cast <size_t> (4u));
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

    InterlockedIncrement (&scanned_libs);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&scanned_libs, 2);

  if (ppLibraries != nullptr)
     *ppLibraries  = steam_lib_paths;

  return steam_libs;
}

std::string _StripSpaces (const std::string& input)
{
  // Create a copy of the input string
  std::string result = input;

  // Remove leading spaces
  result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](char ch) {
      return !std::isspace(ch, std::locale::classic());
  }));

  // Remove trailing spaces
  result.erase(std::find_if(result.rbegin(), result.rend(), [](char ch) {
      return !std::isspace(ch, std::locale::classic());
  }).base(), result.end());

  return result;
}

std::string
SK_UseManifestToGetAppName (AppId64_t appid)
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
      return // Strip Spaces because EA keeps adding them to their game names
        _StripSpaces (app_name);
    }
  }

  return "";
}

std::vector <SK_Steam_Depot>
SK_UseManifestToGetDepots (AppId64_t appid)
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
      depots.emplace_back (
          "", sk::narrow_cast <uint32_t> (atoi  (it            .c_str ())),
              sk::narrow_cast <uint64_t> (atoll (values [idx++].c_str ()))
      );
    }
  }

  return depots;
}

ManifestId_t
SK_UseManifestToGetDepotManifest (AppId64_t appid, DepotId_t depot)
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

std::string&
SK::SteamAPI::AppName (void)
{
  if (AppID () != 0x00)
  {
    // Only do this once, the AppID never changes =P
    static std::string app_name =
      SK_UseManifestToGetAppName (AppID ());

    return app_name;
  }

  static std::string
         _empty ("");
  return _empty;
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
  if (config.platform.silent)
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
SK::SteamAPI::TakeScreenshot (SK_ScreenshotStage when, bool allow_sound, std::string title)
{
  steam_log->LogEx (true, L"  >> Triggering Screenshot: ");

  bool captured = false;

  if ( static_cast <int> (SK_GetCurrentRenderBackend ().api) &
       static_cast <int> (SK_RenderAPI::D3D11) )
  {
    captured =
      SK_D3D11_CaptureScreenshot (when, allow_sound, title);
  }

  else if ( static_cast <int> (SK_GetCurrentRenderBackend ().api) &
            static_cast <int> (SK_RenderAPI::D3D12) )
  {
    captured =
      SK_D3D12_CaptureScreenshot (when, allow_sound, title);
  }

  else if ( static_cast <int> (SK_GetCurrentRenderBackend ().api) &
            static_cast <int> (SK_RenderAPI::OpenGL) )
  {
    captured =
      SK_GL_CaptureScreenshot (when, allow_sound, title);
  }

  else if ( static_cast <int> (SK_GetCurrentRenderBackend ().api) &
            static_cast <int> (SK_RenderAPI::D3D9) )
  {
    captured =
      SK_D3D9_CaptureScreenshot (when, allow_sound, title);
  }

  if (captured)
  {
    steam_log->LogEx ( false, L"Stage=%x (SK_SmartCapture)\n",
                         static_cast <int> (when) );

    return true;
  }

  if (when != SK_ScreenshotStage::ClipboardOnly)
  {
    ISteamScreenshots* pScreenshots =
      steam_ctx.Screenshots ();

    if (pScreenshots && (! config.platform.steam_is_b0rked))
    {   pScreenshots->TriggerScreenshot ();
        steam_log->LogEx (false, L"EndOfFrame (Steam Overlay)\n");
    }

    if ( when != SK_ScreenshotStage::EndOfFrame )
    {
      steam_log->Log (L" >> WARNING: Smart Capture disabled or unsupported"
                      L"; screenshot taken at end-of-frame.");
    }
  }

  return true;
}




void
S_CALLTYPE
SteamAPI_ManualDispatch_Init_Detour (void)
{
  if (SteamAPI_ManualDispatch_Init_Original != nullptr)
      SteamAPI_ManualDispatch_Init_Original ();

  if (steam_log.getPtr () != nullptr && (! steam_log->silent))
  {   steam_log->Log (
      L"Disabling Direct SteamAPI Integration  [ Manual Callback Dispatch Req. ]" );
  }

  std::filesystem::path steam_path (
    SK_GetInstallPath ()
  );

  steam_path /= LR"(PlugIns\ThirdParty\Steamworks)";
  steam_path /= SK_RunLHIfBitness (32, L"steam_api_sk.dll",
                                       L"steam_api_sk64.dll");

  config.platform.silent =
    !( PathFileExistsW ( steam_path.c_str () ));

  if (! config.platform.silent) {
        config.steam.auto_inject         = true;
        config.steam.auto_pump_callbacks = true;
        config.steam.force_load_steamapi = true;
        config.steam.init_delay          =    1;
        config.steam.dll_path            =
                          steam_path.wstring ();
  }else
  {
    // Platform integration must be disabled,
    //   Special K does not support this.
    config.steam.dll_path            = L"";
    config.steam.auto_inject         = false;
    config.steam.force_load_steamapi = false;

    SK_GetDLLConfig ()->get_section (L"Steam.Log").
                          get_value (L"Silent").
                             assign (L"true");
  }

  SK_SaveConfig   ();
  SK_GetDLLConfig ()->write ();

  InterlockedExchange (&__SK_DLL_Ending, 1);

  SK_RestartGame ();

  MessageBoxW ( 0,
    L"Special K Does not Support SteamAPI Integration in this game",
    L"Please Restart Game", MB_ICONWARNING | MB_OK | MB_TASKMODAL | MB_SETFOREGROUND | MB_TOPMOST );

  TerminateProcess (
    GetCurrentProcess (), 0x0
  );
}

bool
S_CALLTYPE
SteamAPI_ISteamInput_Init_Detour (ISteamInput* This, bool bExplicitlyCallRunFrame)
{
  SK_LOG_FIRST_CALL

  if (config.input.gamepad.steam.disabled_to_game)
  {
    SK_LOGi0 (L"Blocked game's attempt to initialize Steam Input!");

    return false;
  }

  return
    SteamAPI_ISteamInput_Init_Original (This, bExplicitlyCallRunFrame);
}

bool
S_CALLTYPE
SteamAPI_InitSafe_Detour (void)
{
  bool bRet = false;

  if (steam_init_cs != nullptr)
      steam_init_cs->lock ();

  // In case we already initialized stuff...
  if (ReadAcquire (&__SK_Steam_init))
  {
    bRet = SteamAPI_InitSafe_Original ();

    if (steam_init_cs != nullptr)
        steam_init_cs->unlock ();

    return bRet;
  }

  static volatile LONG init_tries = -1;

  if (InterlockedIncrement (&init_tries) == 0)
  {
    steam_log->Log ( L"Initializing SteamWorks Backend  << %s >>",
                    SK_GetCallerName ().c_str () );
    steam_log->Log (L"----------(InitSafe)-----------\n");
  }
  bool bTempAppIDFile = false;

  bRet = SteamAPI_InitSafe_Original ();

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

    bRet = SteamAPI_InitSafe_Original ();

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
      HMODULE hSteamAPI =
        SK_GetModuleHandleW (SK_Steam_GetDLLPath ());

      SK_SteamAPI_ContextInit (hSteamAPI);

      if (ReadAcquire (&__SK_Init) > 0)
        SK_ApplyQueuedHooks ();

      steam_log->Log ( L"--- Initialization Finished (%d tries [AppId: %lu]) ---\n\n",
                         ReadAcquire (&init_tries) + 1,
                           SK::SteamAPI::AppID () );

      SK_Steam_StartPump (config.steam.force_load_steamapi);

      // Handle situations where Steam was initialized earlier than
      //   expected...
      void SK_SteamAPI_InitManagers (void); SK_RunOnce (
           SK_SteamAPI_InitManagers (    ));
    }
  }

  if (steam_init_cs != nullptr)
      steam_init_cs->unlock ();

  return bRet;
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
      SteamAPI_Init_Original != nullptr ?
      SteamAPI_Init_Original     ()     :
      SteamAPI_InitSafe_Original ();
  }
  catch (const SK_SEH_IgnoredException&) { }
  SK_SEH_RemoveTranslator (orig_se);

  return bRet;
}

bool
S_CALLTYPE
SteamAPI_Init_Detour (void)
{
  //11
  //k_ESteamInputConfigurationEnableType_Switch | k_ESteamInputConfigurationEnableType_Xbox | k_ESteamInputConfigurationEnableType_Playstation

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
      HMODULE hSteamAPI =
        SK_GetModuleHandleW (SK_Steam_GetDLLPath ());

      SK_SteamAPI_ContextInit (hSteamAPI);

      if (ReadAcquire (&__SK_Init) > 0)
        SK_ApplyQueuedHooks ();

      steam_log->Log ( L"--- Initialization Finished (%d tries [AppId: %lu]) ---\n\n",
                         ReadAcquire (&init_tries) + 1,
                           SK::SteamAPI::AppID () );

      SK_Steam_StartPump (config.steam.force_load_steamapi);

      // Handle situations where Steam was initialized earlier than
      //   expected...
      void SK_SteamAPI_InitManagers (void); SK_RunOnce (
           SK_SteamAPI_InitManagers (    ));
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
  if (SK::SteamAPI::AppID () + config.steam.appid > 0)
    steam_log->init (L"logs/steam_api.log", L"wt+,ccs=UTF-8");
  else // Not a Steam game, but stuff may still be printed...
    steam_log->init (L"logs/platform.log",  L"wt+,ccs=UTF-8");

  steam_log->silent = config.platform.silent;

  SK_ICommandProcessor* cmd = nullptr;

  #define cmdAddAliasedVar(name,pVar)                 \
    for ( const char* alias : { "Steam."    #name,    \
                                "Platform." #name } ) \
      cmd->AddVariable (alias, pVar);

  SK_RunOnce (
    cmd =
      SK_Render_InitializeSharedCVars ()
  );
  
  if (cmd != nullptr)
  {
    cmdAddAliasedVar (TakeScreenshot,
        SK_CreateVar (SK_IVariable::Boolean,
                        (bool *)&config.platform.achievements.take_screenshot));
    cmdAddAliasedVar (ShowPopup,
        SK_CreateVar (SK_IVariable::Boolean,
                        (bool *)&config.platform.achievements.popup.show));
    cmdAddAliasedVar (PopupDuration,
        SK_CreateVar (SK_IVariable::Int,
                        (int  *)&config.platform.achievements.popup.duration));
    cmdAddAliasedVar (PopupInset,
        SK_CreateVar (SK_IVariable::Float,
                        (float*)&config.platform.achievements.popup.inset));
    cmdAddAliasedVar (ShowPopupTitle,
        SK_CreateVar (SK_IVariable::Boolean,
                        (bool *)&config.platform.achievements.popup.show_title));
    cmdAddAliasedVar (PopupAnimate,
        SK_CreateVar (SK_IVariable::Boolean,
                        (bool *)&config.platform.achievements.popup.animate));
    cmdAddAliasedVar (PlaySound,
        SK_CreateVar (SK_IVariable::Boolean,
                        (bool *)&config.platform.achievements.play_sound));

    steam_ctx.popup_origin =
      SK_CreateVar ( SK_IVariable::String,
                       steam_ctx.var_strings.popup_origin,
                      &steam_ctx );
    cmdAddAliasedVar ( PopupOrigin,
                       steam_ctx.popup_origin );

    steam_ctx.notify_corner =
      SK_CreateVar ( SK_IVariable::String,
                       steam_ctx.var_strings.notify_corner,
                      &steam_ctx );
    cmdAddAliasedVar ( NotifyCorner,
                       steam_ctx.notify_corner );

    steam_ctx.tbf_pirate_fun =
      SK_CreateVar ( SK_IVariable::Float,
                       &steam_ctx.tbf_float,
                       &steam_ctx );
  }
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
    SK_Sleep (std::max (5, config.steam.init_delay));

    if (SK_GetFramesDrawn () < 1)
    {
      SwitchToThread ();

      if (ReadAcquire (&__SK_Steam_init))
      {
        break;
      }

      continue;
    }

    ++tries;

    if (ReadAcquire (&__SK_Steam_init))
    {
      break;
    }

    if (SteamAPI_Init_Original != nullptr)
    {
      SteamAPI_Init_Detour ();
    }

    else if (SteamAPI_InitSafe_Original != nullptr)
    {
      SteamAPI_InitSafe_Detour ();
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
  static int hooks = 0;

  if (config.platform.silent)
    return hooks;

  static const wchar_t* wszSteamAPI =
    SK_Steam_GetDLLPath ();

  if (! SK_GetModuleHandle (wszSteamAPI))
    return hooks;

  if (! InterlockedCompareExchange (&__SteamAPI_hook, TRUE, FALSE))
  {
    SK_RunOnce ( SK::SteamAPI::steam_size =
                  SK_File_GetSize (wszSteamAPI) );

    steam_log->Log ( L"%s was loaded, hooking...",
                     SK_ConcealUserDir ( std::wstring (wszSteamAPI).data () )
    );

    // New part of SteamAPI, don't try to hook unless the DLL exports it
    if (SK_GetProcAddress (wszSteamAPI, "SteamAPI_ManualDispatch_Init"))
    {
      SK_CreateDLLHook2 ( wszSteamAPI,
                         "SteamAPI_ManualDispatch_Init",
                          SteamAPI_ManualDispatch_Init_Detour,
                          static_cast_p2p <void> (&SteamAPI_ManualDispatch_Init_Original),
                          static_cast_p2p <void> (&SteamAPI_ManualDispatch_Init) );      ++hooks;
    }

    // Newer SteamAPI DLLs may not export this symbol, in which case we fallback to InitSafe
    if (SK_GetProcAddress (wszSteamAPI, "SteamAPI_Init"))
    {
      SK_CreateDLLHook2 ( wszSteamAPI,
                         "SteamAPI_Init",
                          SteamAPI_Init_Detour,
                          static_cast_p2p <void> (&SteamAPI_Init_Original),
                          static_cast_p2p <void> (&SteamAPI_Init) );                     ++hooks;
    }

    // 1.57 aliases SteamAPI_Init to SteamAPI_InitSafe
    if ( SK_GetProcAddress (wszSteamAPI, "SteamAPI_Init") != 
         SK_GetProcAddress (wszSteamAPI, "SteamAPI_InitSafe") )
    {
      SK_CreateDLLHook2 ( wszSteamAPI,
                         "SteamAPI_InitSafe",
                          SteamAPI_InitSafe_Detour,
                          static_cast_p2p <void> (&SteamAPI_InitSafe_Original),
                          static_cast_p2p <void> (&SteamAPI_InitSafe) );                   ++hooks;
    }

    else
      SteamAPI_InitSafe_Original = SteamAPI_Init_Original;

    if (SK_GetProcAddress (wszSteamAPI, "SteamAPI_ISteamInput_Init"))
    {
      SK_CreateDLLHook2 ( wszSteamAPI,
                         "SteamAPI_ISteamInput_Init",
                          SteamAPI_ISteamInput_Init_Detour,
                          static_cast_p2p <void> (&SteamAPI_ISteamInput_Init_Original)); ++hooks;
    }

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


    // Older DLLs will not have this, and we should avoid printing an error in the log
    if (SK_GetProcAddress (wszSteamAPI,
                             "SteamAPI_ISteamController_GetDigitalActionData") != nullptr)
    {
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
    }

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

    if (hooks > 0)
    {
      //bool bEnableHooks =
      //  SK_EnableApplyQueuedHooks ();

      SK_ApplyQueuedHooks ();

      //if (! bEnableHooks)
      //  SK_DisableApplyQueuedHooks ();
    }

    InterlockedIncrementRelease (&__SteamAPI_hook);

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

    if ( stats != nullptr && ( (! user_manager) ||
                               (! steam_achievements) ) )
    {
      // Check if SteamAPI is going to throw exceptions or not, and disable
      //   parts of SteamAPI integration to prevent NVIDIA Streamline crashes
      SK_SteamAPI_GetNumPlayers ();

       // Streamline initiates a memory dump and abort if any exceptions are
       //   raised during Present (...), so various API calls cannot be used
       //     during overlay render.

      if (! steam_achievements)
      {
        has_global_data = false;
        next_friend     = 0;

            stats->RequestCurrentStats ();
                       TryRunCallbacks ();
        if (stats->GetNumAchievements  ())
        {
          steam_log->Log (L" Creating Achievement Manager...");

          steam_achievements =
            std::make_unique <SK_Steam_AchievementManager> (
              config.platform.achievements.sound_file.c_str ()
            );

          steam_achievements->requestStats ();
                           TryRunCallbacks ();
             steam_achievements->pullStats ();
                           TryRunCallbacks ();
        }
      }

      steam_log->LogEx (false, L"\n");

      if (! user_manager)
      {     user_manager = std::make_unique <SK_Steam_UserManager> ();
        strncpy_s ( steam_ctx.var_strings.popup_origin, 16,
              SK_Steam_PopupOriginToStr (
                config.platform.achievements.popup.origin
              ), _TRUNCATE );

        strncpy_s ( steam_ctx.var_strings.notify_corner, 16,
                      SK_Steam_PopupOriginToStr (
                        config.platform.notify_corner
                      ), _TRUNCATE );

        if (! config.platform.steam_is_b0rked)
          SK_Steam_SetNotifyCorner ();
      }
    }

    if (! __SK_Steam_IgnoreOverlayActivation)
    {
      if (steam_ctx.Utils () && (! overlay_manager))
      {
        overlay_manager =
          std::make_unique <SK_Steam_OverlayManager> ();
      }
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

std::vector <SK_Achievement *>&
SK_SteamAPI_GetAllAchievements (void)
{
  static std::vector <SK_Achievement *> achievements;

  if (steam_achievements == nullptr)
  {
    achievements.clear ();
    return achievements;
  }

  size_t num;

  SK_Achievement** ppAchv =
    steam_achievements->getAchievements (&num);

  if (achievements.size () != num)
  {
    achievements.clear ();

    for (size_t i = 0; i < num; i++)
      achievements.push_back (ppAchv [i]);
  }

  return achievements;
}

std::vector <SK_Achievement *>&
SK_SteamAPI_GetUnlockedAchievements (void)
{
  static std::vector <SK_Achievement *> unlocked_achievements;

  unlocked_achievements.clear ();

  std::vector <SK_Achievement *>& achievements =
    SK_SteamAPI_GetAllAchievements ();

  for ( auto& it : achievements )
  {
    if (it->unlocked_)
      unlocked_achievements.push_back (it);
  }

  return unlocked_achievements;
}

std::vector <SK_Achievement *>&
SK_SteamAPI_GetLockedAchievements (void)
{
  static std::vector <SK_Achievement *> locked_achievements;

  locked_achievements.clear ();

  std::vector <SK_Achievement *>& achievements =
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

  std::vector <SK_Achievement *>&
    unlocked_achvs = SK_SteamAPI_GetUnlockedAchievements ();

  if (pStats != nullptr)
    RtlZeroMemory (pStats, SK_SteamAPI_GetNumPossibleAchievements ());

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
    sk::narrow_cast <uint32_t> (
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
    steam_imported = true;
    SK_HookSteamAPI ();
  }

  else
  {
    if (! SK_IsInjected ())
    {
      if ( SK_Modules->LoadLibrary (steam_dll_str) ||
           SK_GetModuleHandle (L"SteamNative.dll") )
      {
        steam_imported = true;
        SK_HookSteamAPI ();
      }
    }
  }

  // Once is enough, dammit!
  if ( steam_imported == false && 
       ( SK_Modules->LoadLibrary (steam_dll_str) ||
         SK_GetModuleHandle (L"SteamNative.dll") )
     )
  {
    steam_imported = true;
    SK_HookSteamAPI ();
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
  // Thanks to NVIDIA Streamline's bullshit, and the fact that
  //   SteamAPI raises an exception if this API is used while
  //     offline or the player is invisible... it is not safe
  //       to call certain SteamAPI functions inside of the
  //         overlay's render code.
  // 
  //  * We MUST swallow exceptions and then stop calling this API
  //
  if (! config.platform.steam_is_b0rked)
  {
    __try
    {
      if (user_manager != nullptr)
        user_manager->UpdateNumPlayers ();

      else
      {
        ISteamUserStats* user_stats =
          steam_ctx.UserStats ();

        if (user_stats != nullptr)
            user_stats->GetNumberOfCurrentPlayers ();
      }
    }

    __except (EXCEPTION_EXECUTE_HANDLER)
    {
      SK_LOGi0 (L"SteamAPI is not working correctly, ignoring player count.");

      config.platform.steam_is_b0rked = true;

      if (SK_IsModuleLoaded (L"sl.interposer.dll"))
      {
        SK_ImGui_Warning (
          L"SteamAPI is not working correctly and NVIDIA Streamline is used by the game."
          L"\r\n\r\n\t>> You may experience crashes unless SteamAPI integration is disabled."
          L"\r\n\r\nSet [Steam.Log] Silent=true"
        );
      }
    }
  }
}

int32_t
__stdcall
SK_SteamAPI_GetNumPlayers (void)
{
  // Thanks to NVIDIA Streamline's bullshit, and the fact that
  //   SteamAPI raises an exception if this API is used while
  //     offline or the player is invisible... it is not safe
  //       to call certain SteamAPI functions inside of the
  //         overlay's render code.
  // 
  //  * We MUST swallow exceptions and then stop calling this API
  //
  if (! config.platform.steam_is_b0rked)
  {
    __try
    {
      if (user_manager != nullptr)
      {
        int32 num =
          user_manager->GetNumPlayers ();

        if (num <= 1)
          user_manager->UpdateNumPlayers ();

        return
          std::max (1, user_manager->GetNumPlayers ());
      }

      else
      {
        ISteamUserStats* user_stats =
          steam_ctx.UserStats ();

        if (user_stats != nullptr)
            user_stats->GetNumberOfCurrentPlayers ();
      }
    }

    __except (EXCEPTION_EXECUTE_HANDLER)
    {
      SK_LOGi0 (L"SteamAPI is not working correctly, ignoring player count.");

      config.platform.steam_is_b0rked = true;

      if (SK_IsModuleLoaded (L"sl.interposer.dll"))
      {
        SK_ImGui_Warning (
          L"SteamAPI is not working correctly and NVIDIA Streamline is used by the game."
          L"\r\n\r\n\t>> You may experience crashes unless SteamAPI integration is disabled."
          L"\r\n\r\nSet [Steam.Log] Silent=true"
        );
      }
    }
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

int
__stdcall
SK_SteamAPI_NumberOfAchievementsUnlocked (void)
{
  if (steam_achievements != nullptr)
    return steam_achievements->getNumberOfAchievementsUnlocked ();

  return 0;
}

int
__stdcall
SK::SteamAPI::NumberOfAchievementsUnlocked (void)
{
  return SK_SteamAPI_NumberOfAchievementsUnlocked ();
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

    if (pUser && !config.platform.steam_is_b0rked)
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
       validation_pass   != SK_Steam_FileSigPass_e::Done && !SK_IsCurrentGame (SK_GAME_ID::Fallout4) )
  {
    static DepotId_t
        depots [16] = { };
    if (depots [0] != 0 ||
        ! SK_Steam_GetInstalledDepots (depots))
    {
      validation_pass = SK_Steam_FileSigPass_e::Done;
      return verdict;
    }

    if ( hAsyncSigCheck == 0 &&
      (! config.platform.silent) )
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
  if ( SK::SteamAPI::steam_size > 0LL &&
       SK::SteamAPI::steam_size < (1024LL * 85LL) )
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
  if (config.platform.silent && verdict)
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

          SK_SHA1_Hash SHA1 = { };
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
                  verdict |= ~k_ECheckFileSignatureInvalidSignature;
                  decided  =  true;
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
SK_Steam_SignalEmulatedXInputActivity (DWORD dwSlot, bool blocked)
{
  static iSK_INI *controller_ini =
    SK_CreateINI (
      SK_FormatStringW (
        L"%ws/config/virtualgamepadinfo.txt", SK_GetSteamDir ()
      ).c_str ()
    );

  static constexpr wchar_t* slot_names [] = {
    L"slot 0", L"slot 1", L"slot 2", L"slot 3",
    L"slot 4", L"slot 5", L"slot 6", L"slot 7"
  };

  static sk_input_dev_type device_types [8] = {
    sk_input_dev_type::Gamepad, sk_input_dev_type::Gamepad, sk_input_dev_type::Gamepad,
    sk_input_dev_type::Gamepad, sk_input_dev_type::Gamepad, sk_input_dev_type::Gamepad,
    sk_input_dev_type::Gamepad, sk_input_dev_type::Gamepad
  };

  if (dwSlot > 7)
    return;

  auto *pInput = steam_ctx.Input ();

  if (pInput == nullptr)
    return;

  // Figure out the real type one time by parsing the INI file
  if (device_types [dwSlot] == sk_input_dev_type::Gamepad)
  {
#if 0
    SK_RunOnce (pInput->Init (false));

    auto type =
      pInput->GetInputTypeForHandle (
        pInput->GetControllerForGamepadIndex (dwSlot)
      );

    switch (type)
    {
      case k_ESteamInputType_XBox360Controller:
      case k_ESteamInputType_XBoxOneController:
        device_types [dwSlot] = sk_input_dev_type::Gamepad_Xbox;
        break;
      case k_ESteamInputType_GenericGamepad:
      case k_ESteamInputType_SteamController:
      case k_ESteamInputType_SteamDeckController:
      case k_ESteamInputType_AppleMFiController:
      case k_ESteamInputType_AndroidController:
      case k_ESteamInputType_MobileTouch:
        device_types [dwSlot] = sk_input_dev_type::Gamepad_Generic;
        break;
      case k_ESteamInputType_SwitchJoyConPair:
      case k_ESteamInputType_SwitchJoyConSingle:
      case k_ESteamInputType_SwitchProController:
        device_types [dwSlot] = sk_input_dev_type::Gamepad_Nintendo;
        break;
      case k_ESteamInputType_PS3Controller:
      case k_ESteamInputType_PS4Controller:
      case k_ESteamInputType_PS5Controller:
        device_types [dwSlot] = sk_input_dev_type::Gamepad_PlayStation;
        break;
      default:
        device_types [dwSlot] = sk_input_dev_type::Other;
        break;
    }
#else
    if (controller_ini->contains_section (slot_names [dwSlot]))
    {
      auto& type =
        controller_ini->get_section (slot_names [dwSlot]).get_cvalue (L"type");

      if (StrStrIW (type.c_str (), L"ps") == type.c_str ())
      {
        device_types [dwSlot] = sk_input_dev_type::Gamepad_PlayStation;
      }

      else if (StrStrIW (type.c_str (), L"xbox") == type.c_str ())
      {
        device_types [dwSlot] = sk_input_dev_type::Gamepad_Xbox;
      }

      else
      {
        device_types [dwSlot] = sk_input_dev_type::Gamepad_Generic;
      }
    }

    else
    {
      device_types [dwSlot] = sk_input_dev_type::Other;
    }
#endif
  }

  if (device_types [dwSlot] != sk_input_dev_type::Other)
  {
    if (! blocked)
      SK_STEAM_VIEW (device_types [dwSlot])
    else
      SK_STEAM_HIDE (device_types [dwSlot])
  }

#if 0
  const auto input_dev_type_mask =
    pInput->GetSessionInputConfigurationSettings ();

  if (input_dev_type_mask != k_ESteamInputConfigurationEnableType_None)
  {
    const bool known_type = 0 !=
     ( input_dev_type_mask & ( k_ESteamInputConfigurationEnableType_Playstation |
                               k_ESteamInputConfigurationEnableType_Xbox        |
                               k_ESteamInputConfigurationEnableType_Generic     |
                               k_ESteamInputConfigurationEnableType_Switch ) );
  
    if (known_type) [[likely]]
    {
      if (input_dev_type_mask & k_ESteamInputConfigurationEnableType_Playstation)
        SK_STEAM_VIEW (                   sk_input_dev_type::Gamepad_PlayStation);
      if (input_dev_type_mask & k_ESteamInputConfigurationEnableType_Xbox)
        SK_STEAM_VIEW (                   sk_input_dev_type::Gamepad_Xbox);
      if (input_dev_type_mask & k_ESteamInputConfigurationEnableType_Generic)
        SK_STEAM_VIEW (                   sk_input_dev_type::Gamepad_Generic);
      if (input_dev_type_mask & k_ESteamInputConfigurationEnableType_Switch)
        SK_STEAM_VIEW (                   sk_input_dev_type::Gamepad_Nintendo);
    }
  }
#endif
}

bool
SK_Steam_IsClientRunning (void)
{
  return
    SK_IsProcessRunning (L"steam.exe");
}

bool
SK_Steam_RunClientCommand (const wchar_t *wszCommand)
{
  wchar_t      wszSteamCmd [MAX_PATH * 3] = { };
  swprintf_s ( wszSteamCmd, MAX_PATH * 3 - 1,
                 LR"(%ws\steam.exe -- steam://%ws)",
                   SK_GetSteamDir (),
                     wszCommand );

  STARTUPINFOW        sinfo = { };
  PROCESS_INFORMATION pinfo = { };

  sinfo.cb          = sizeof (STARTUPINFOW);
  sinfo.wShowWindow = SW_HIDE;
  sinfo.dwFlags     = STARTF_USESHOWWINDOW;

  CreateProcess ( nullptr, wszSteamCmd,      nullptr, nullptr,
                  FALSE,   CREATE_SUSPENDED, nullptr, SK_GetSteamDir (),
                  &sinfo,  &pinfo );

  if (pinfo.hProcess != 0)
  {
    ResumeThread   (pinfo.hThread);
    SK_CloseHandle (pinfo.hThread);
    SK_CloseHandle (pinfo.hProcess);

    return true;
  }

  return false;
}

void
SK_Steam_ForceInputAppId (AppId64_t appid)
{
  if (config.platform.silent)
    return;

  char                                                  configurator [8] = { };
  GetEnvironmentVariableA ("EnableConfiguratorSupport", configurator, 7);

  char                                                                       xinput_emulation [8] = { };
  GetEnvironmentVariableA ("SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD", xinput_emulation, 7);

  if (*configurator != '\0' && atoi (configurator) == 0)
    return;

  if (*xinput_emulation != '\0' && atoi (xinput_emulation) == 0)
    return;

  //
  // SteamOS has a ton of input problems to begin with, let's just let the platform
  //   stay broken.
  // 
  //   Using control panel in SteamInput-native games will cause undefined
  //     behavior on Linux, but the Steam client will be less broken.
  //
  if (config.compatibility.using_wine)
    return;

  static volatile LONG changes       = 0;
  static bool          steam_running = false;

  static auto
    _CleanupForcedAppId = [&](int minimum_change_count = 0)
  {
    if (steam_running && ReadAcquire (&changes) > minimum_change_count)
    {
      SK_RunOnce (
      {
        SK_LOGi1 (L"Resetting Steam Input AppId...");

        // Cleanup on unexpected application termination
        //
        SK_Steam_RunClientCommand (L"forceinputappid/");
      });
    }
  };

  if (config.steam.appid > 0)
  {
    if ( ReadAcquire (&__SK_DLL_Ending) &&
         ReadAcquire (&changes) > 1 ) // First change is always to 0
    {
      _CleanupForcedAppId (1);

      return;
    }

    struct {
      concurrency::concurrent_queue <AppId64_t> app_ids;
      SK_AutoHandle                             signal =
         SK_AutoHandle (
           SK_CreateEvent ( nullptr, FALSE, FALSE, nullptr )
                       );

      void push (AppId64_t appid)
      {
        app_ids.push (appid);

        if (appid != 0)
          InterlockedIncrement (&changes);

        SetEvent (signal.m_h);
      }
    } static override_ctx;

    SK_RunOnce (
      SK_Thread_CreateEx ([](LPVOID) -> DWORD
      {
        SK_Thread_SetCurrentPriority (THREAD_PRIORITY_LOWEST);

        HANDLE hWaitObjects [] = {
          __SK_DLL_TeardownEvent,
          override_ctx.signal.m_h
        };

        DWORD dwWaitState = WAIT_OBJECT_0;

        if (! steam_running)
        {
          do
          {
            steam_running =
              SK_Steam_IsClientRunning ();

            if (steam_running)
              break;
          } while (WaitForSingleObject (__SK_DLL_TeardownEvent, 3333UL) == WAIT_TIMEOUT);
        }

        if (steam_running)
        {
          do
          {
            dwWaitState =
              WaitForMultipleObjects (2, hWaitObjects, FALSE, INFINITE);

            if (dwWaitState == WAIT_OBJECT_0 + 1)
            {
              while (SK_GetFramesDrawn () == 0)
              {
                SK_SleepEx (25, FALSE);

                if (ReadAcquire (&__SK_DLL_Ending))
                  break;
              }

              if (SK_GetFramesDrawn () > 0)
              {
                while (! override_ctx.app_ids.empty ())
                {
                  static AppId64_t                  last_override = UINT64_MAX;
                  AppId64_t                         appid         = UINT64_MAX;
                  if (override_ctx.app_ids.try_pop (appid))
                  {
                    if (! ReadAcquire (&__SK_DLL_Ending))
                    {
                      // We have other stuff pending, no reason to send this stale command
                      if (! override_ctx.app_ids.empty ())
                        continue;

                      // Still send this override, but delay it in case something else comes in...
                      if (std::exchange (last_override, appid) == appid)
                        SK_SleepEx (25UL, FALSE);

                      if (! override_ctx.app_ids.empty ())
                        continue;

                      if (appid == UINT64_MAX)
                        continue;

                      const bool appid_set =
                        SK_Steam_RunClientCommand ( appid != 0 ?
                          SK_FormatStringW (LR"(forceinputappid/%llu)", appid).c_str ()
                                                               :
                                            LR"(forceinputappid/)"
                        );

                      if (appid_set)
                      {
                        if (appid != 0)
                        {
                          static AppId64_t   last_appid = static_cast <AppId64_t> (-1);
                          if (std::exchange (last_appid, appid) != appid)
                          {
                            SK_LOGi1 (L"Forced Steam Input AppID: %llu", appid);
                          }
                        }
                        else
                          SK_LOGi1 (L"Forced Steam Input AppID: Default");
                      }

                      else
                      {
                        SK_LOGi0 (L"steam://forceinputappid failed!");
                        break; // Fail-once and we're done...
                      }

                      appid = UINT64_MAX;

                      SK_SleepEx (200UL, FALSE);
                    }
                  }
                }
              }
            }
          } while (dwWaitState != WAIT_OBJECT_0);
        }

        if (dwWaitState == WAIT_OBJECT_0)
        {
          _CleanupForcedAppId (1);
        }

        SK_Thread_CloseSelf ();

        return 0;
      }, L"[SK] SteamInput Obedience Trainer")
    );

    override_ctx.push (appid);
  }
}

void
SK_SteamInput_Unfux0r (void)
{
  if (ReadAcquire (&__SK_DLL_Ending))
    return;

  static SK_AutoHandle hSignal (
    SK_CreateEvent (nullptr, FALSE, FALSE, nullptr)
  );

  static HANDLE hThread =
    SK_Thread_CreateEx ([](LPVOID)->DWORD
    {
      const HANDLE events [] =
      { __SK_DLL_TeardownEvent, hSignal };

      while ( WAIT_OBJECT_0 !=
                WaitForMultipleObjects (
                  _countof (events),
                            events, FALSE, INFINITE ) )
      {
        SK_SleepEx               (500UL, FALSE);
        SK_Steam_ForceInputAppId (0);
      }

      SK_Thread_CloseSelf ();

      return 0;
    }, L"[SK] SteamInput Unfux0r");

  SetEvent (hSignal);
}

void
SK_Steam_ProcessWindowActivation (bool active)
{
  if (config.steam.appid <= 0)
    return;

  // Hacky code for Steam Input background input
  if (config.window.background_render && config.input.gamepad.disabled_to_game == SK_InputEnablement::Enabled)
  {
    if (! SK::SteamAPI::SetWindowFocusState (true))
    {
      SK_Steam_ForceInputAppId (config.steam.appid);

      // So Valve's stupid overlay "works"
      if (SK_GetForegroundWindow () == game_window.hWnd)
      {
        SK_SteamInput_Unfux0r ();
      }
    }
  }

  else if (config.input.gamepad.disabled_to_game != SK_InputEnablement::Disabled)
  {
    if (! active)
    {
      if (! SK::SteamAPI::SetWindowFocusState (false))
      {
        SK_Steam_ForceInputAppId (SPECIAL_KILLER_APPID);
      }
    }

    else
    {
      if (! SK::SteamAPI::SetWindowFocusState (true))
      {
        SK_Steam_ForceInputAppId (config.steam.appid);

        SK_SteamInput_Unfux0r ();
      }
    }
  }

  else// (config.input.gamepad.disabled_to_game == SK_InputEnablement::Disabled)
  {
    if (! SK::SteamAPI::SetWindowFocusState (false))
    {
      SK_Steam_ForceInputAppId (SPECIAL_KILLER_APPID);
    }
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

ISteamInput*
SAFE_GetISteamInput (       ISteamClient *pClient,
                            HSteamUser    hSteamuser,
                            HSteamPipe    hSteamPipe,
                      const char         *pchVersion )
{
  ISteamInput* pInput = nullptr;

  if (pClient != nullptr)
  {
    auto orig_se =
    SK_SEH_ApplyTranslator (
      SK_FilteringStructuredExceptionTranslator(EXCEPTION_ACCESS_VIOLATION)
    );
    try
    {
      pInput = (ISteamInput *)
        pClient->GetISteamGenericInterface (hSteamuser, hSteamPipe, pchVersion);
    }

    catch (const SK_SEH_IgnoredException&) { };
    SK_SEH_RemoveTranslator (orig_se);
  }

  return pInput;
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
  } //else
    //client_->SetWarningMessageHook ( &SK_SteamAPI_DebugText );


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
#define INTERNAL_STEAMINPUT_INTERFACE_VERSION          6

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
        steam_log->Log (L"SteamAPI DLL Implements User v%03lu", i);

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


  using  SteamInternal_FindOrCreateUserInterface_pfn = void* (S_CALLTYPE *)(HSteamUser hSteamUser, const char *pszVersion);
  static SteamInternal_FindOrCreateUserInterface_pfn
         SteamInternal_FindOrCreateUserInterface =
        (SteamInternal_FindOrCreateUserInterface_pfn)SK_GetProcAddress (hSteamDLL,
        "SteamInternal_FindOrCreateUserInterface");

  if (SteamInternal_FindOrCreateUserInterface != nullptr)
  {
    input_ = (ISteamInput *)
      SteamInternal_FindOrCreateUserInterface (hSteamUser, SK_FormatString ("SteamInput%03lu", INTERNAL_STEAMINPUT_INTERFACE_VERSION).c_str ());
  }

  if (input_ != nullptr)
  {
    if (config.input.gamepad.steam.disabled_to_game)
    {
      input_->Shutdown ();
    }
  }

  for (i = INTERNAL_STEAMINPUT_INTERFACE_VERSION+1; i > 0; --i)
  {
    input_ = (ISteamInput *)
      client_->GetISteamGenericInterface (
        hSteamUser, hSteamPipe,
          SK_FormatString ("SteamInput%03lu", i).c_str ()
      );

    if (input_ != nullptr)
    {
      if (input_ver_ == 0)
      {
        input_ver_ = i;
        steam_log->Log (L"SteamAPI DLL Implements Input v%03lu", i);

        if (input_ver_ > INTERNAL_STEAMINPUT_INTERFACE_VERSION)
          steam_log->Log (L" >> Newer than Special K knows about.");
      }

      if (i == INTERNAL_STEAMINPUT_INTERFACE_VERSION)
      {
        break;
      }
    }
  }

  if (i != INTERNAL_STEAMINPUT_INTERFACE_VERSION)
  {
    steam_log->Log ( L" >> ISteamInput NOT FOUND for version %hs <<",
                   STEAMINPUT_INTERFACE_VERSION );
    input_ = nullptr;
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
  if (config.steam.online_status != SK_NoPreference)
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

    steam_log->Log ( L" ** DLL is too old, disabling SteamAPI integration!");

    config.platform.silent = true;

    SK_SaveConfig  ();
    SK_RestartGame ();

    return false;
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

  SK_QueueEnableHook (SteamAPI_Shutdown);
  SK_QueueEnableHook (SteamAPI_RunCallbacks);

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
        SK_RunLHIfBitness (                             64,
          LR"(PlugIns\ThirdParty\Steamworks\steam_api_sk64.dll)",
          LR"(PlugIns\ThirdParty\Steamworks\steam_api_sk.dll)"
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
          PathAppendW (wszDLLPath, SK_GetInstallPath ());
          PathAppendW (wszDLLPath, LR"(\SpecialK.dll)");
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
  virtual IClientUtils_001       *GetIClientUtils       (HSteamUser hSteamUser, HSteamPipe hSteamPipe, char const* pchVersion) = 0;

  // LOTS MORE DONT CARE...
};


void*
SK_SteamAPIContext::ClientEngine (void)
{
  using  CreateInterface_pfn = void* (__cdecl *)(const char *ver, int* pError);
  static CreateInterface_pfn SteamClient_CreateInterface = nullptr;

#ifdef _M_AMD64
  if (SteamClient_CreateInterface == nullptr)
      SteamClient_CreateInterface =
    (CreateInterface_pfn)
    SK_GetProcAddress ( SK_GetModuleHandle (L"steamclient64.dll"),
                       "CreateInterface" );
#else /* _M_IX86 */
  if (SteamClient_CreateInterface == nullptr)
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

    // Try upgrading the interface version if Steam wouldn't give us 005
    if (client_engine == nullptr)
    {
      client_engine =
        (IClientEngine_005_Min *)SteamClient_CreateInterface (
          CLIENTENGINE_INTERFACE_VERSION_ALT, &err
        );
    }
      
    if (client_engine == nullptr) // We're boned
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

bool
SK_SteamAPIContext::SetWindowFocusState (bool focused)
{
  // SteamOS has even buggier Steam Input than Windows; ignore the problem.
  if (config.compatibility.using_wine)
    return false;

  //if (config.input.gamepad.steam.is_native)
    return false;

  auto* client_engine =
    (IClientEngine_005_Min *)ClientEngine ();

  assert (client_engine != nullptr);

  if (client_engine != nullptr)
  {
    IClientUtils_001* utils = nullptr;

#define MANUAL_PIPE_CREATION
#ifdef MANUAL_PIPE_CREATION
    static thread_local auto hPipe = client_engine->CreateSteamPipe     ();
    static thread_local auto hUser = client_engine->ConnectToGlobalUser (hPipe);

    if (hUser == 0)
    {
      hUser =
        client_engine->CreateGlobalUser (&hPipe);
    }
#else
    auto hPipe = hSteamPipe;
    auto hUser = hSteamUser;
#endif

    if (hPipe != 0)
    {
      utils =
        client_engine->GetIClientUtils (hUser, hPipe,
         CLIENTUTILS_INTERFACE_VERSION);

      assert (utils != nullptr);

      if (utils != nullptr)
      {
        utils->SetFocusedWindow (
          CGameID (SK::SteamAPI::AppID ()), focused
        );
      }

#ifdef MANUAL_PIPE_CREATION
#ifdef RELEASE_THREAD_PIPES
      client_engine->ReleaseUser       (hPipe, hUser);
      client_engine->BReleaseSteamPipe (hPipe);
#endif
#endif

      if (utils)
        return true;
    }
  }

  return false;
}

bool
SK::SteamAPI::SetWindowFocusState (bool focused)
{
  return
    pSteamCtx->SetWindowFocusState (focused);
}

AppId64_t
SK_SteamAPIContext::ReassignAppIDForPipe ( HSteamPipe hPipe,
                                           AppId64_t  nAppID,
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
        client_engine->GetIClientUtils (hSteamUser, hPipe,
          CLIENTUTILS_INTERFACE_VERSION);

      assert (utils != nullptr);

      if (utils != nullptr)
        return
          utils->SetAppIDForCurrentPipe (sk::narrow_cast <uint32_t> (nAppID), bTrackProcess);
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
        client_engine->GetIClientUtils (hSteamUser, hSteamPipe,
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

  if (config.steam.online_status != SK_NoPreference)
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

#include <SpecialK/storefront/epic.h>

uint64_t
SK_Steam_GetAppID_NoAPI (void)
{
  static constexpr int MAX_APPID_LEN = 32;

  DWORD    dwSteamGameIdLen                  =  0 ;
  uint64_t AppID                             =  0 ;
  char     szSteamGameId [MAX_APPID_LEN + 1] = { };

  // First, look for steam_appid.txt
  if (GetFileAttributesW (L"steam_appid.txt") != INVALID_FILE_ATTRIBUTES)
  { FILE* fAppID = fopen ( "steam_appid.txt", "r" );
    if (  fAppID != nullptr  )
    {
      fgets (szSteamGameId, MAX_APPID_LEN, fAppID);
                                   fclose (fAppID);

      dwSteamGameIdLen = (DWORD)strlen  (szSteamGameId);
      AppID            =        strtoll (szSteamGameId, nullptr, 0);
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
      AppID = strtoll (szSteamGameId, nullptr, 0);
  }

  // Special K's AppID is a mistake of some sort, ignore it
  if (AppID == SPECIAL_KILLER_APPID)
  {   AppID = 0; config.steam.appid = 0; }

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

  //
  // Alternative platforms init now
  //
  else if ( StrStrIA (GetCommandLineA (), "-epicapp") ||
                        PathFileExistsW (L".egstore") ||
                     PathFileExistsW (L"../.egstore") )
  {
    SK::EOS::Init (false); // Hook EOS SDK; game was launched by Epic
    SK::EOS::AppName (  ); // Use manifest to get app name

    // Trigger profile migration if necessary
    app_cache_mgr->getConfigPathFromAppPath (SK_GetFullyQualifiedApp ());

    SK_GetConfigPathEx (true);
  }

  return 0;
}

uint64_t    SK::SteamAPI::steam_size = 0ULL;

// Must be global for x86 ABI problems
CSteamID    SK::SteamAPI::player      (0ULL);