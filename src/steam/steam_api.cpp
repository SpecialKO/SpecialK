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
#include <process.h>
#include <Shlwapi.h>

#include <SpecialK/resource.h>

#include <SpecialK/hooks.h>
#include <SpecialK/core.h>

#include <SpecialK/config.h>
#include <SpecialK/ini.h>
#include <SpecialK/log.h>
#include <SpecialK/crc32.h>
#include <SpecialK/thread.h>
#include <SpecialK/utility.h>
#include <SpecialK/framerate.h>
#include <SpecialK/diagnostics/modules.h>
#include <SpecialK/diagnostics/load_library.h>

#include <SpecialK/osd/popup.h>
#include <SpecialK/osd/text.h>
#include <SpecialK/render/backend.h>

#include <SpecialK/tls.h>

#include <array>
#include <memory>
#include <atlbase.h>

// We're not going to use DLL Import - we will load these function pointers
//  by hand.
#define STEAM_API_NODLL
#include <SpecialK/steam_api.h>
#include <../depends/include/steamapi/steamclientpublic.h>


#include <concurrent_unordered_map.h>
static Concurrency::concurrent_unordered_map <LPVOID, bool> UserStatsReceived_callbacks;

volatile LONGLONG SK_SteamAPI_CallbackRunCount    =  0LL;
volatile LONG     SK_SteamAPI_ManagersInitialized =  0L;
volatile LONG     SK_SteamAPI_CallbackRateLimit   = -1L;

// PlaySound
#pragma comment (lib, "winmm.lib")

#pragma warning (disable: 4706)

         iSK_Logger       steam_log;

volatile LONG             __SK_Steam_init = FALSE;
volatile LONG             __SteamAPI_hook = FALSE;

extern SK_Thread_HybridSpinlock* steam_callback_cs;
extern SK_Thread_HybridSpinlock* steam_popup_cs;
extern SK_Thread_HybridSpinlock* steam_init_cs;

#include <time.h>

// To spoof Overlay Activation (pause the game)
#include <set>
#include <unordered_map>

std::multiset <class CCallbackBase *> overlay_activation_callbacks;

int  SK_HookSteamAPI                   (void);
void SK_Steam_UpdateGlobalAchievements (void);


using SteamClient_pfn = ISteamClient* (S_CALLTYPE *)(
        void
      );
extern                          SteamClient_pfn
                                SteamClient_Original;
extern ISteamClient* S_CALLTYPE SteamClient_Detour (void);

using SteamUser_pfn = ISteamUser* (S_CALLTYPE *)(
        void
      );
extern                        SteamUser_pfn
                              SteamUser_Original;
extern ISteamUser* S_CALLTYPE SteamUser_Detour (void);

using SteamRemoteStorage_pfn = ISteamRemoteStorage* (S_CALLTYPE *)(
        void
      );
extern                                 SteamRemoteStorage_pfn
                                       SteamRemoteStorage_Original;
extern ISteamRemoteStorage* S_CALLTYPE SteamRemoteStorage_Detour (void);

using SteamUGC_pfn = ISteamUGC* (S_CALLTYPE *)(
        void
      );
extern                                 SteamUGC_pfn
                                       SteamUGC_Original;
extern ISteamUGC*           S_CALLTYPE SteamUGC_Detour (void);

using SteamUtils_pfn = ISteamUtils* (S_CALLTYPE *)(
        void
      );
extern                         SteamUtils_pfn
                               SteamUtils_Original;
extern ISteamUtils* S_CALLTYPE SteamUtils_Detour (void);


using SteamInternal_CreateInterface_pfn       = void*       (S_CALLTYPE *)(
        const char *ver
      );
extern                      SteamInternal_CreateInterface_pfn
                            SteamInternal_CreateInterface_Original;
extern     void* S_CALLTYPE SteamInternal_CreateInterface_Detour (const char *ver);

using SteamAPI_ISteamClient_GetISteamUser_pfn = ISteamUser* (S_CALLTYPE *)(
              ISteamClient *This,
              HSteamUser    hSteamUser,
              HSteamPipe    hSteamPipe,
        const char         *pchVersion
      );
extern                        SteamAPI_ISteamClient_GetISteamUser_pfn   SteamAPI_ISteamClient_GetISteamUser_Original;
extern ISteamUser* S_CALLTYPE SteamAPI_ISteamClient_GetISteamUser_Detour ( ISteamClient *This,
                                                                           HSteamUser    hSteamUser,
                                                                           HSteamPipe    hSteamPipe,
                                                                           const char   *pchVersion );

using SteamAPI_ISteamClient_GetISteamUtils_pfn = ISteamUtils* (S_CALLTYPE *)(
  ISteamClient *This,
  HSteamPipe    hSteamPipe,
  const char   *pchVersion
  );
extern                         SteamAPI_ISteamClient_GetISteamUtils_pfn   SteamAPI_ISteamClient_GetISteamUtils_Original;
extern ISteamUtils* S_CALLTYPE SteamAPI_ISteamClient_GetISteamUtils_Detour ( ISteamClient *This,
                                                                             HSteamPipe    hSteamPipe,
                                                                             const char   *pchVersion );

using SteamAPI_ISteamClient_GetISteamController_pfn = ISteamController* (S_CALLTYPE *)(
              ISteamClient *This,
              HSteamUser    hSteamUser,
              HSteamPipe    hSteamPipe,
        const char         *pchVersion
      );
extern                              SteamAPI_ISteamClient_GetISteamController_pfn   SteamAPI_ISteamClient_GetISteamController_Original;
extern ISteamController* S_CALLTYPE SteamAPI_ISteamClient_GetISteamController_Detour ( ISteamClient *This,
                                                                                       HSteamUser    hSteamUser,
                                                                                       HSteamPipe    hSteamPipe,
                                                                                       const char   *pchVersion );

using SteamAPI_ISteamClient_GetISteamRemoteStorage_pfn = ISteamRemoteStorage* (S_CALLTYPE *)(
  ISteamClient *This,
  HSteamUser    hSteamuser,
  HSteamPipe    hSteamPipe,
  const char   *pchVersion
  );
extern                                 SteamAPI_ISteamClient_GetISteamRemoteStorage_pfn   SteamAPI_ISteamClient_GetISteamRemoteStorage_Original;
extern ISteamRemoteStorage* S_CALLTYPE SteamAPI_ISteamClient_GetISteamRemoteStorage_Detour ( ISteamClient *This,
                                                                                             HSteamUser    hSteamuser,
                                                                                             HSteamPipe    hSteamPipe,
                                                                                             const char   *pchVersion );


using SteamAPI_ISteamClient_GetISteamUGC_pfn = ISteamUGC* (S_CALLTYPE *)(
  ISteamClient *This,
  HSteamUser    hSteamuser,
  HSteamPipe    hSteamPipe,
  const char   *pchVersion
  );
extern                                 SteamAPI_ISteamClient_GetISteamUGC_pfn   SteamAPI_ISteamClient_GetISteamUGC_Original;
extern ISteamUGC* S_CALLTYPE SteamAPI_ISteamClient_GetISteamUGC_Detour ( ISteamClient *This,
                                                                         HSteamUser    hSteamuser,
                                                                         HSteamPipe    hSteamPipe,
                                                                         const char   *pchVersion );



extern "C" SteamAPI_InitSafe_pfn              SteamAPI_InitSafe_Original             = nullptr;
extern "C" SteamAPI_Init_pfn                  SteamAPI_Init_Original                 = nullptr;

extern "C" SteamAPI_RunCallbacks_pfn          SteamAPI_RunCallbacks                  = nullptr;
extern "C" SteamAPI_RunCallbacks_pfn          SteamAPI_RunCallbacks_Original         = nullptr;

extern "C" SteamAPI_RegisterCallback_pfn      SteamAPI_RegisterCallback              = nullptr;
extern "C" SteamAPI_RegisterCallback_pfn      SteamAPI_RegisterCallback_Original     = nullptr;

extern "C" SteamAPI_UnregisterCallback_pfn    SteamAPI_UnregisterCallback            = nullptr;
extern "C" SteamAPI_UnregisterCallback_pfn    SteamAPI_UnregisterCallback_Original   = nullptr;

extern "C" SteamAPI_RegisterCallResult_pfn    SteamAPI_RegisterCallResult            = nullptr;
extern "C" SteamAPI_UnregisterCallResult_pfn  SteamAPI_UnregisterCallResult          = nullptr;

extern "C" SteamAPI_Init_pfn                  SteamAPI_Init                          = nullptr;
extern "C" SteamAPI_InitSafe_pfn              SteamAPI_InitSafe                      = nullptr;

extern "C" SteamAPI_RestartAppIfNecessary_pfn SteamAPI_RestartAppIfNecessary         = nullptr;
extern "C" SteamAPI_IsSteamRunning_pfn        SteamAPI_IsSteamRunning                = nullptr;

extern "C" SteamAPI_GetHSteamUser_pfn         SteamAPI_GetHSteamUser                 = nullptr;
extern "C" SteamAPI_GetHSteamPipe_pfn         SteamAPI_GetHSteamPipe                 = nullptr;

extern "C" SteamClient_pfn                    SteamClient                            = nullptr;

extern "C" SteamAPI_Shutdown_pfn              SteamAPI_Shutdown                      = nullptr;
extern "C" SteamAPI_Shutdown_pfn              SteamAPI_Shutdown_Original             = nullptr;

extern "C" SteamAPI_GetSteamInstallPath_pfn   SteamAPI_GetSteamInstallPath           = nullptr;

LPVOID pfnSteamInternal_CreateInterface = nullptr;



auto _ConstructPath =
[&](auto&& path_base, const wchar_t* path_end)
 {
   std::array <wchar_t, MAX_PATH * 2 + 1>
                 path { };
   PathCombineW (path.data (), path_base.data (), path_end);
   return        path;
 };


// Returns the value of files as it was when called
unsigned int
SK_Steam_RecursiveFileScrub ( std::wstring   directory, unsigned int& files,
                              LARGE_INTEGER& liSize,    wchar_t*      wszPattern,
                                                        wchar_t*      wszAntiPattern, // Exact match only
                              bool           erase = false )
{
  unsigned int    files_start = files;
  WIN32_FIND_DATA fd          = {   };
  HANDLE          hFind       =
    FindFirstFileW (_ConstructPath (directory, wszPattern).data (), &fd);

  if (hFind != INVALID_HANDLE_VALUE)
  {
    do
    {
      // Leave installscript.vdf behind
      //
      if (              wszAntiPattern != nullptr &&
           (! _wcsicmp (wszAntiPattern, fd.cFileName)) )
      {
        continue;
      }

      if (          (fd.dwFileAttributes  & FILE_ATTRIBUTE_DIRECTORY) &&
          (_wcsicmp (fd.cFileName, L"." )                != 0)        &&
          (_wcsicmp (fd.cFileName, L"..")                != 0)        &&
          (_wcsicmp (fd.cFileName, L"Steamworks Shared") != 0) )
      {
        // Detect empty directories
        //
        unsigned int before =
          SK_Steam_RecursiveFileScrub ( _ConstructPath (directory, fd.cFileName).data (),
                                          files,
                                            liSize,
                                              wszPattern,
                                                wszAntiPattern,
                                                  erase );

        // Directory is empty; remove it if the user wants
        //
        if (files == before && erase)
        {
          RemoveDirectoryW (_ConstructPath (directory, fd.cFileName).data ());
        }
      }

      else if ( (fd.dwFileAttributes ^ FILE_ATTRIBUTE_DIRECTORY) & 
                                       FILE_ATTRIBUTE_DIRECTORY    )
      {
        if (! erase)
        {
          ++files;
          liSize.QuadPart +=
            LARGE_INTEGER {                     fd.nFileSizeLow,
                            static_cast <LONG> (fd.nFileSizeHigh) }.QuadPart;
        }

        else
        {
          DeleteFileW (_ConstructPath (directory, fd.cFileName).data ());
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
  std::wstring found = L"";

  wchar_t   wszPath [MAX_PATH * 2] = { };
  swprintf (wszPath, LR"(%s\*)", wszDir);

  WIN32_FIND_DATA fd          = {   };
  HANDLE          hFind       =
    FindFirstFileW ( wszPath, &fd);

  if (hFind == INVALID_HANDLE_VALUE) { return found; }

  do
  {
    if ( wcscmp (fd.cFileName, L".")  == 0 ||
         wcscmp (fd.cFileName, L"..") == 0 )
    {
      continue;
    }

    if (! _wcsicmp (fd.cFileName, wszFile))
    {
      found = wszDir;
      found += L"\\"; found += wszFile;

      break;
    }

    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      wchar_t   wszDescend [MAX_PATH * 2] = { };
      swprintf (wszDescend, LR"(%s\%s)", wszDir, fd.cFileName);

      found = _SK_RecursiveFileSearch (wszDescend, wszFile);
    }

  } while ((found.empty ()) && FindNextFile (hFind, &fd));

  FindClose (hFind);

  return found;
}

using steam_library_t = wchar_t* [MAX_PATH * 2];

int
SK_Steam_GetLibraries (steam_library_t** ppLibraries = nullptr);

const wchar_t*
SK_Steam_FindInstallPath (uint32_t appid)
{
  steam_library_t* steam_lib_paths = nullptr;
  int              steam_libs      = SK_Steam_GetLibraries (&steam_lib_paths);

  if (steam_libs != 0)
  {
    for (int i = 0; i < steam_libs; i++)
    {
      wchar_t wszManifest [MAX_PATH * 2 + 1] = { };

      wsprintf ( wszManifest,
                  LR"(%ws\steamapps\appmanifest_%u.acf)",
                    (wchar_t *)steam_lib_paths [i],
                      appid );

      CHandle hManifest (
        CreateFileW ( wszManifest,
                      GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                          nullptr,
                            OPEN_EXISTING,
                              GetFileAttributesW (wszManifest),
                                nullptr )
      );

      if (hManifest != INVALID_HANDLE_VALUE)
      {
        DWORD  dwSize     = 0,
               dwSizeHigh = 0,
               dwRead     = 0;

        dwSize =
          GetFileSize (hManifest, &dwSizeHigh);

        auto szManifestData =
          std::make_unique <char []> (
            std::size_t (dwSize + 1)
          );

        if (! szManifestData.get ())
          continue;

        bool bRead =
          ReadFile ( hManifest,
                       szManifestData.get (),
                         dwSize,
                           &dwRead,
                             nullptr );

        if (! (bRead && dwRead))
        {
          continue;
        }

        char* szAppName =
          StrStrIA (szManifestData.get (), R"("installdir")");

        if (szAppName != nullptr)
        {
          char szGamePath [513] = { };

          // Make sure everything is lowercase
          memcpy ( szAppName, R"("installdir")", 12 );
          sscanf ( szAppName,
                  R"("installdir" "%512[^"]")",
                       szGamePath );

          static wchar_t wszFound [MAX_PATH * 2 + 1] = { };
          wcsncpy_s     (wszFound, MAX_PATH * 2, SK_UTF8ToWideChar (szGamePath).c_str (), _TRUNCATE);
          return         wszFound;
        }
      }
    }
  }

  return L"";
}


//
// Go-go-gadget garbage code compactor!
//
bool
SK_Steam_GetDLLPath ( wchar_t* wszDestBuf,
                      size_t   max_size = MAX_PATH * 2 )
{
  if (max_size == 0)
    return false;

  constexpr wchar_t* wszSteamLib =
    SK_RunLHIfBitness ( 64, L"steam_api64.dll",
                            L"steam_api.dll"    );

  static wchar_t
    dll_file [MAX_PATH * 2 + 1] = { };

  // Already have a working DLL
  if (SK_Modules.LoadLibrary (dll_file))
  {
    wcsncpy_s (wszDestBuf, max_size, dll_file, _TRUNCATE);
    return true;
  }

  std::wstring& cfg_path =
    config.steam.dll_path;

  if (                               (!cfg_path.empty ()) &&
       GetFileAttributesW             (cfg_path.c_str ()) != INVALID_FILE_ATTRIBUTES )
  { if (SK_Modules.LoadLibrary        (cfg_path.c_str ()))
    { wcsncpy_s (wszDestBuf, max_size, cfg_path.c_str (), _TRUNCATE);
      wcsncpy_s (dll_file,   max_size, cfg_path.c_str (), _TRUNCATE);
      return true;
    }
  }

  cfg_path.resize (0);

  wchar_t      wszExecutablePath [MAX_PATH * 2 + 1] = { };
  wcsncpy_s   (wszExecutablePath, MAX_PATH, SK_GetHostPath (), _TRUNCATE);
  PathAppendW (wszExecutablePath, wszSteamLib);

  // The simplest case:  * DLL is in the same directory as the executable.
  //
  if (SK_Modules.LoadLibrary           (wszExecutablePath))
  {   wcsncpy_s (dll_file,   max_size, wszExecutablePath, _TRUNCATE);
      wcsncpy_s (wszDestBuf, max_size, dll_file,          _TRUNCATE);
      cfg_path.assign     (dll_file);

    return true;
  }

  wcsncpy_s (wszExecutablePath, MAX_PATH, SK_GetHostPath (), _TRUNCATE);
  lstrcatW  (wszExecutablePath, L".");

  // Moonwalk to find the game's absolute root directory, rather than
  //   whatever subdirectory the executable might be in.
  while (PathRemoveFileSpecW           (wszExecutablePath))
  { if  (StrStrIW                      (wszExecutablePath, LR"(SteamApps\common\)"))
    {    wcsncpy_s (dll_file, max_size, wszExecutablePath, _TRUNCATE);
                             lstrcatW  (wszExecutablePath, L".");
    } else break;
  }

  // Try the DLL we went back to the root of the install dir-tree for first.
  if (                         *dll_file != L'\0')
  { wcsncpy_s (wszExecutablePath, MAX_PATH * 2, dll_file, _TRUNCATE); }

  // Then get the base install directory from Steam (if we can).
  else if (config.steam.appid != 0)
  { wcsncpy_s ( wszExecutablePath, MAX_PATH * 2, 
                SK_Steam_FindInstallPath (config.steam.appid),
                  _TRUNCATE );
  }

  // Finally, give up and hope the DLL is relative to the executable.
  else
    wcsncpy_s (wszExecutablePath, MAX_PATH * 2, SK_GetHostPath (), _TRUNCATE);

  static wchar_t
    wszFullSteamDllPath [MAX_PATH * 2 - 1] = { };

  std::wstring recursed (
    std::move           (
      _SK_RecursiveFileSearch ( wszExecutablePath,
                                wszSteamLib        )
                        )
                        );

    wcsncpy_s (dll_file,             MAX_PATH * 2, recursed.c_str (), _TRUNCATE);
    wcsncpy_s ( wszFullSteamDllPath, MAX_PATH * 2, dll_file,          _TRUNCATE);


  if (SK_Modules.LoadLibrary              (wszFullSteamDllPath))
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
    dll_file [MAX_PATH * 2 + 1] = { };

  if (*dll_file == L'\0')
  { if (! SK_Steam_GetDLLPath (dll_file)) {
                       *dll_file = L'\0'; }
  }

  return dll_file;
}

BOOL
SK_Steam_PreHookCore (void)
{
  static volatile LONG init = FALSE;

  if (InterlockedCompareExchange (&init, TRUE, FALSE))
    return TRUE;

  const wchar_t* wszSteamLib =
    SK_RunLHIfBitness ( 64, L"steam_api64.dll",
                            L"steam_api.dll"    );

  if (! SK_Modules.LoadLibrary (wszSteamLib))
  {
    const std::wstring& qualified_lib =
      SK_Steam_GetDLLPath ();

    if (! qualified_lib.empty ())
    {
      SK_Modules.LoadLibrary (qualified_lib.c_str ());
      wszSteamLib =           qualified_lib.data  ();
    }
  }

  if (GetProcAddress (GetModuleHandle (wszSteamLib), "SteamClient"))
  {
    SK_CreateDLLHook2 (          wszSteamLib,
                                  "SteamClient",
                                   SteamClient_Detour,
          static_cast_p2p <void> (&SteamClient_Original) );
  }

  if (GetProcAddress (GetModuleHandle (wszSteamLib), "SteamUser"))
  {
    SK_CreateDLLHook2 (          wszSteamLib,
                                  "SteamUser",
                                   SteamUser_Detour,
          static_cast_p2p <void> (&SteamUser_Original) );
  }

  if (GetProcAddress (GetModuleHandle (wszSteamLib), "SteamRemoteStorage"))
  {
    //SK_CreateDLLHook2 (          wszSteamLib,
    //                              "SteamRemoteStorage",
    //                               SteamRemoteStorage_Detour,
    //      static_cast_p2p <void> (&SteamRemoteStorage_Original) );
  }

  if (GetProcAddress (GetModuleHandle (wszSteamLib), "SteamUGC"))
  {
    SK_CreateDLLHook2 (          wszSteamLib,
                                  "SteamUGC",
                                   SteamUGC_Detour,
          static_cast_p2p <void> (&SteamUGC_Original) );
  }

  if (GetProcAddress (GetModuleHandle (wszSteamLib), "SteamUtils"))
  {
    SK_CreateDLLHook2 (          wszSteamLib,
                                  "SteamUtils",
                                   SteamUtils_Detour,
          static_cast_p2p <void> (&SteamUtils_Original) );
  }

  if ( GetProcAddress (
         SK_Modules.LoadLibraryLL(wszSteamLib),
           "SteamInternal_CreateInterface" ) )
  {
    SK_CreateDLLHook2 (          wszSteamLib,
                                  "SteamAPI_ISteamClient_GetISteamUser",
                                   SteamAPI_ISteamClient_GetISteamUser_Detour,
          static_cast_p2p <void> (&SteamAPI_ISteamClient_GetISteamUser_Original) );
    
    SK_CreateDLLHook2 (          wszSteamLib,
                                  "SteamAPI_ISteamClient_GetISteamUtils",
                                   SteamAPI_ISteamClient_GetISteamUtils_Detour,
          static_cast_p2p <void> (&SteamAPI_ISteamClient_GetISteamUtils_Original) );
    
    SK_CreateDLLHook2 (          wszSteamLib,
                                  "SteamAPI_ISteamClient_GetISteamController",
                                   SteamAPI_ISteamClient_GetISteamController_Detour,
          static_cast_p2p <void> (&SteamAPI_ISteamClient_GetISteamController_Original) );
    
    /* 8/1/18 -- Disabled for Yukuza 0
       ------
      
         Will cause SteamAPI problems at cloud-sync start; hook isn't needed since
           SK's cloud features still aren't finished.
    */
    //SK_CreateDLLHook2 (          wszSteamLib,
    //                              "SteamAPI_ISteamClient_GetISteamRemoteStorage",
    //                               SteamAPI_ISteamClient_GetISteamRemoteStorage_Detour,
    //      static_cast_p2p <void> (&SteamAPI_ISteamClient_GetISteamRemoteStorage_Original) );

    SK_CreateDLLHook2 (          wszSteamLib,
                                  "SteamAPI_ISteamClient_GetISteamUGC",
                                   SteamAPI_ISteamClient_GetISteamUGC_Detour,
          static_cast_p2p <void> (&SteamAPI_ISteamClient_GetISteamUGC_Original) );

    SK_CreateDLLHook2 (          wszSteamLib,
                                  "SteamInternal_CreateInterface",
                                   SteamInternal_CreateInterface_Detour,
          static_cast_p2p <void> (&SteamInternal_CreateInterface_Original),
                               &pfnSteamInternal_CreateInterface );
            MH_QueueEnableHook (pfnSteamInternal_CreateInterface);

    return TRUE;
  }

  return FALSE;
}

#include <CEGUI/CEGUI.h>
#include <CEGUI/System.h>

extern bool
SK_D3D11_CaptureSteamScreenshot (SK::ScreenshotStage when);

extern bool
SK_D3D9_CaptureSteamScreenshot  (SK::ScreenshotStage when);

SK_Steam_ScreenshotManager::~SK_Steam_ScreenshotManager (void)
{
  for ( int i = 0 ; i < _StatusTypes ; ++i )
  {
    if (hSigReady [i] != INVALID_HANDLE_VALUE)
    {
      SetEvent (hSigReady [i]);

      CloseHandle (hSigReady [i]);
                   hSigReady [i] = INVALID_HANDLE_VALUE;
    }
  }
}


void
SK_Steam_ScreenshotManager::OnScreenshotRequest ( ScreenshotRequested_t *pParam )
{
  UNREFERENCED_PARAMETER (pParam);

  if ( (int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::D3D11 )
  {
    // Avoid any exotic pixel formats for now -- 8-bit RGB(A) only
    //if (SK_GetCurrentRenderBackend ().framebuffer_flags == 0x00)
    {
      SK_D3D11_CaptureSteamScreenshot ( config.steam.screenshots.show_osd_by_default ?
                                          SK::ScreenshotStage::EndOfFrame : 
                                          SK::ScreenshotStage::BeforeOSD );

      return;
    }
  }

  //else if ( (int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::D3D9 )
  //{
  //  // Avoid any exotic pixel formats for now -- 8-bit RGB(A) only
  //  //if (SK_GetCurrentRenderBackend ().framebuffer_flags == 0x00)
  //  {
  //    SK_D3D9_CaptureSteamScreenshot ( config.steam.screenshots.show_osd_by_default ?
  //                                        SK::ScreenshotStage::EndOfFrame : 
  //                                        SK::ScreenshotStage::BeforeOSD );
  //
  //    return;
  //  }
  //}
  //
  // This feature is not supported,  so go back to normal screenshots.
  request.Unregister ();

  steam_ctx.Screenshots ()->HookScreenshots   (false);
  steam_ctx.Screenshots ()->TriggerScreenshot (     );
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
    steam_log.Log ( L"Screenshot Import/Write Failure for handle '%x'; EResult=%x",
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

  if (   dwStatus >=  WAIT_OBJECT_0                         &&
         dwStatus <  (WAIT_OBJECT_0 + MAXIMUM_WAIT_OBJECTS) &&
       ( dwStatus  -  WAIT_OBJECT_0 ) < 2 )
  {
    UINT type = std::min ( 1UL, dwStatus - WAIT_OBJECT_0 );
    auto iter = screenshots_handled.find (handle);

    if (iter != screenshots_handled.end ())
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
SK_Steam_ScreenshotManager::init (void)
{
  getExternalScreenshotRepository (true);

  if (config.steam.screenshots.enable_hook && steam_ctx.Screenshots ())
  {
    steam_ctx.Screenshots ()->HookScreenshots (config.steam.screenshots.enable_hook);
    
    for ( int i = 0 ; i < _StatusTypes ; ++i )
    {
      if (hSigReady [i] == INVALID_HANDLE_VALUE)
      {
        hSigReady [i] =
          CreateEventW (nullptr, FALSE, FALSE, nullptr);
      }
    }
  }
}

const wchar_t*
SK_Steam_ScreenshotManager::
getExternalScreenshotPath (void)
{
  static wchar_t wszAbsolutePathToScreenshots [ MAX_PATH * 2 + 1 ] = { };

  if (*wszAbsolutePathToScreenshots != L'\0')
    return wszAbsolutePathToScreenshots;

  wcsncpy_s    ( wszAbsolutePathToScreenshots, MAX_PATH * 2, SK_GetConfigPath (), _TRUNCATE);
  PathAppendW  ( wszAbsolutePathToScreenshots, L"Screenshots" );

  return         wszAbsolutePathToScreenshots;
}

SK_Steam_ScreenshotManager::screenshot_repository_s&
SK_Steam_ScreenshotManager::getExternalScreenshotRepository (bool refresh)
{
  static bool init = false;

  if (refresh || (! init))
  {
    external_screenshots.files           = 0;
    external_screenshots.liSize.QuadPart = 0ULL;

    WIN32_FIND_DATA fd        = {   };
    std::wstring    directory = getExternalScreenshotPath ();
    HANDLE          hFind     =
      FindFirstFileW   (_ConstructPath (directory, L"*").data (), &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
      do
      {
        if (          (fd.dwFileAttributes  & FILE_ATTRIBUTE_DIRECTORY) &&
            (_wcsicmp (fd.cFileName, L"." )                != 0)        &&
            (_wcsicmp (fd.cFileName, L"..")                != 0) )
        {
          SK_Steam_RecursiveFileScrub ( _ConstructPath   (
                                          _ConstructPath ( directory, fd.cFileName ),
                                                             L""
                                                         ).data (),
                                          external_screenshots.files,
                                          external_screenshots.liSize,
                                            L"*", L"SK_SteamScreenshotImport.jpg",
                                              false );
        }
      } while (FindNextFile (hFind, &fd));

      FindClose (hFind);

      init = true;
    }
  }

  return external_screenshots;
}

SK_Steam_ScreenshotManager *screenshot_manager = nullptr;


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
#if 0
    if (active_ != (pParam->m_bActive != 0))
    {
      if (pParam->m_bActive)
      {
        static bool first = true;

        if (first)
        {
          cursor_visible_ = ShowCursor (FALSE) >= -1;
                            ShowCursor (TRUE);
          first = false;
        }
      }

      // Restore the original cursor state.
      else
      {
        if (! cursor_visible_)
        {
          ULONG calls = 0;

          steam_log.Log (L"  (Steam Overlay Deactivation:  Hiding Mouse Cursor... )");
          while (ShowCursor (FALSE) >= 0)
            ++calls;

          if (calls == 0)
            steam_log.Log (L"  (Steam Overlay Deactivation:  Overlay is functioning correctly => NOP)");
          else
            steam_log.Log (L"  (Steam Overlay Deactivation:  Overlay leaked state => Took %lu tries)", calls);
        }
      }
    }
#endif

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


    active_ = (pParam->m_bActive != 0);
  }

  bool isActive (void) const { return active_; }

private:
  bool cursor_visible_; // Cursor visible prior to activation?
  bool active_;
} *overlay_manager = nullptr;

bool
WINAPI
SK_IsSteamOverlayActive (void)
{
  if (overlay_manager)
    return overlay_manager->isActive ();

  return false;
}


#include <SpecialK/utility.h>

ISteamUserStats* SK_SteamAPI_UserStats   (void);
void             SK_SteamAPI_ContextInit (HMODULE hSteamAPI);


#define STEAM_VIRTUAL_HOOK(_Base,_Index,_Name,_Override,_Original,_Type) {   \
  void** _vftable = *(void***)*(_Base);                                      \
                                                                             \
  if ((_Original) == nullptr) {                                              \
    SK_CreateVFTableHook ( L##_Name,                                         \
                             _vftable,                                       \
                               (_Index),                                     \
                                 (_Override),                                \
                                   (LPVOID *)&(_Original));                  \
  }                                                                          \
}

using callback_func_t      =
  void (__fastcall *)( CCallbackBase*, LPVOID );
using callback_func_fail_t =
  void (__fastcall *)( CCallbackBase*, LPVOID, bool bIOFailure, SteamAPICall_t hSteamAPICall );

callback_func_t SteamAPI_UserStatsReceived_Original = nullptr;

extern "C" void __fastcall SteamAPI_UserStatsReceived_Detour       (CCallbackBase* This, UserStatsReceived_t* pParam);
//extern "C" void __cdecl SteamAPI_UserStatsReceivedIOFail_Detour (CCallbackBase* This, UserStatsReceived_t* pParam, bool bIOFailure, SteamAPICall_t hSteamAPICall);

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

S_API
void
S_CALLTYPE
SteamAPI_RegisterCallback_Detour (class CCallbackBase *pCallback, int iCallback)
{
  // Don't care about OUR OWN callbacks ;)
  if (SK_GetCallingDLL () == SK_GetDLL ())
  {
    SteamAPI_RegisterCallback_Original (pCallback, iCallback);
    return;
  }
  std::wstring caller =
    SK_GetCallerName ();

  steam_callback_cs->lock ();

  switch (iCallback)
  {
    case GameOverlayActivated_t::k_iCallback:
    {
      steam_log.Log ( L" * (%-28s) Installed Overlay Activation Callback",
                        caller.c_str () );

      if (SK_ValidatePointer (pCallback))
        overlay_activation_callbacks.insert (pCallback);
    } break;

    case ScreenshotRequested_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Screenshot Callback",
                        caller.c_str () );
      break;
    case UserStatsReceived_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed User Stats Receipt Callback",
                        caller.c_str () );
      UserStatsReceived_callbacks [pCallback] = true;
      break;
    case UserStatsStored_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed User Stats Storage Callback",
                        caller.c_str () );
      break;
    case UserAchievementStored_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed User Achievements Storage Callback",
                        caller.c_str () );
      break;
    case UserStatsUnloaded_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed User Stats Unloaded Callback",
                        caller.c_str () );
      break;
    case SteamShutdown_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed SteamAPI Shutdown Callback",
                        caller.c_str () );
      break;
    case DlcInstalled_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed DLC Installation Callback",
                        caller.c_str () );
      break;
    case SteamAPICallCompleted_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Async. SteamAPI Completion Callback",
                        caller.c_str () );
      break;
    case SteamServersConnected_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Steam Server Connected Callback",
                        caller.c_str () );
      break;
    case SteamServerConnectFailure_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Steam Server Connection Failure Callback",
                        caller.c_str () );
      break;
    case SteamServersDisconnected_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Steam Server Disconnect Callback",
                        caller.c_str () );
      break;
    case LobbyEnter_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Lobby Enter Callback",
                        caller.c_str () );
      break;
    case LobbyDataUpdate_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Lobby Data Update Callback",
                        caller.c_str () );
      break;
    case LobbyChatUpdate_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Lobby Chat Update Callback",
                        caller.c_str () );
      break;
    case GameLobbyJoinRequested_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Game Lobby Join Request Callback",
                        caller.c_str () );
      break;
    case GameRichPresenceJoinRequested_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Game Rich Presence Join Request Callback",
                        caller.c_str () );
      break;
    case P2PSessionConnectFail_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Peer to Peer Session Connect Failure Callback",
                        caller.c_str () );
      break;
    case P2PSessionRequest_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Peer to Peer Session Request Callback",
                        caller.c_str () );
      break;
    default:
    {
      steam_log.Log ( L" * (%-28s) Installed Callback (Class=%hs, Id=%li)",
                        caller.c_str (), SK_Steam_ClassifyCallback (iCallback), iCallback % 100 );
    } break;
  }

  // We will dispatch this callback to all registered interfaces ourself.
  if (iCallback != UserStatsReceived_t::k_iCallback)
    SteamAPI_RegisterCallback_Original (pCallback, iCallback);

  steam_callback_cs->unlock ();
}

void
SK_SteamAPI_EraseActivationCallback (class CCallbackBase *pCallback)
{
  __try
  {
    overlay_activation_callbacks.erase (pCallback);
  }

  __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION )  ?
                       EXCEPTION_EXECUTE_HANDLER :
                       EXCEPTION_CONTINUE_SEARCH )
  {
  }
}

S_API
void
S_CALLTYPE
SteamAPI_UnregisterCallback_Detour (class CCallbackBase *pCallback)
{
  // Skip this if we're uninstalling our own callback
  if (SK_GetCallingDLL () == SK_GetDLL ())
  {
    SteamAPI_UnregisterCallback_Original (pCallback);
    return;
  }

  std::wstring caller =
    SK_GetCallerName ();

  steam_callback_cs->lock ();

  int iCallback = pCallback->GetICallback ();

  switch (iCallback)
  {
    case GameOverlayActivated_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Overlay Activation Callback",
                        caller.c_str () );

      SK_SteamAPI_EraseActivationCallback (pCallback);

      break;
    case ScreenshotRequested_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Screenshot Callback",
                        caller.c_str () );
      break;
    case UserStatsReceived_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled User Stats Receipt Callback",
                        caller.c_str () );

      //// May need to block this callback to prevent some games from spuriously shutting
      ////   SteamAPI down if the server's not cooperating.
      ////
      ////  * Fixes issues with missed/backlogged achievements in games like The Witcher 3
      //if (config.steam.block_stat_callback)
      //{
      //  steam_log.Log (L" ### Callback Blacklisted ###");
      //  LeaveCriticalSection (&callback_cs);
      //  return;
      //}
      UserStatsReceived_callbacks [pCallback] = false;
      break;
    case UserStatsStored_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled User Stats Storage Callback",
                        caller.c_str () );
      break;
    case UserAchievementStored_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled User Achievements Storage Callback",
                        caller.c_str () );
      break;
    case UserStatsUnloaded_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled User Stats Unloaded Callback",
                        caller.c_str () );
      break;
    case SteamShutdown_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled SteamAPI Shutdown Callback",
                        caller.c_str () );
      break;
    case DlcInstalled_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled DLC Installation Callback",
                        caller.c_str () );
      break;
    case SteamAPICallCompleted_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Async. SteamAPI Completion Callback",
                        caller.c_str () );
      break;
    case SteamServersConnected_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Steam Server Connected Callback",
                        caller.c_str () );
      break;
    case SteamServerConnectFailure_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Steam Server Connection Failure Callback",
                        caller.c_str () );
      break;
    case SteamServersDisconnected_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Steam Server Disconnect Callback",
                        caller.c_str () );
      break;
    case LobbyEnter_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Lobby Enter Callback",
                        caller.c_str () );
      break;
    case LobbyDataUpdate_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Lobby Data Update Callback",
                        caller.c_str () );
      break;
    case LobbyChatUpdate_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Lobby Chat Update Callback",
                        caller.c_str () );
      break;
    case GameLobbyJoinRequested_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Game Lobby Join Request Callback",
                        caller.c_str () );
      break;
    case GameRichPresenceJoinRequested_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Game Rich Presence Join Request Callback",
                        caller.c_str () );
      break;
    case P2PSessionConnectFail_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Peer to Peer Session Connect Failure Callback",
                        caller.c_str () );
      break;
    case P2PSessionRequest_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Peer to Peer Session Request Callback",
                        caller.c_str () );
      break;
    default:
    {
      steam_log.Log ( L" * (%-28s) Uninstalled Callback (Class=%hs, Id=%li)",
                        caller.c_str (), SK_Steam_ClassifyCallback (iCallback), iCallback % 100 );
    } break;
  }

  SteamAPI_UnregisterCallback_Original (pCallback);

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
  else if (name == L"TopRight")
    return 1;
  else if (name == L"BottomLeft")
    return 2;
  else if (name == L"DontCare")
    return 4;
  else /*if (name == L"TopLeft")*/
    return 3;

  // TODO: TopCenter, BottomCenter, CenterLeft, CenterRight
}

int
SK_Steam_PopupOriginStrToEnum (const char* str)
{
  std::string name (str);

  if (name == "TopLeft")
    return 0;
  else if (name == "TopRight")
    return 1;
  else if (name == "BottomLeft")
    return 2;
  else if (name == "DontCare")
    return 4;
  else /*if (name == L"TopLeft")*/
    return 3;
}

void SK_Steam_SetNotifyCorner (void);


//extern bool
//ISteamController_GetControllerState_Detour (
//  ISteamController*       This,
//  uint32                  unControllerIndex,
//  SteamControllerState_t *pState );

SK_SteamAPIContext steam_ctx;


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
SK_SteamAPIContext::Shutdown (void)
{
  if (InterlockedDecrement (&__SK_Steam_init) == 1)
  { 
    if (client_)
    {
#if 0
      if (hSteamUser != 0)
        client_->ReleaseUser       (hSteamPipe, hSteamUser);
    
      if (hSteamPipe != 0)
        client_->BReleaseSteamPipe (hSteamPipe);
#endif

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

    user_ver_           = 0;
    utils_ver_          = 0;
    remote_storage_ver_ = 0;
    
    if (SteamAPI_Shutdown_Original != nullptr)
    {
      SteamAPI_Shutdown_Original ();
      SteamAPI_Shutdown_Original = nullptr;

      SK_DisableHook (SteamAPI_RunCallbacks);
      SK_DisableHook (SteamAPI_Shutdown);
    }
  }
}


const char*
SK_SteamAPIContext::GetSteamInstallPath (void)
{
  return SteamAPI_GetSteamInstallPath ();
}

bool
SK_SteamAPIContext::OnVarChange (SK_IVariable* var, void* val)
{
  SK_ICommandProcessor*
    pCommandProc = SK_GetCommandProcessor ();

  UNREFERENCED_PARAMETER (pCommandProc);


  bool known = false;

  // ( Dead man's switch for silly pirates that attempt to bypass framerate limit
  //    with a hex edit rather than a proper forked project - "crack" this the right way please )
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

    strcpy (var_strings.notify_corner,
      SK_Steam_PopupOriginToStr (config.steam.notify_corner)
    );

    SK_Steam_SetNotifyCorner ();

    return true;
  }

  else if (var == popup_origin)
  {
    known = true;

    config.steam.achievements.popup.origin =
      SK_Steam_PopupOriginStrToEnum (*reinterpret_cast <char **> (val));

    strcpy (var_strings.popup_origin,
      SK_Steam_PopupOriginToStr (config.steam.achievements.popup.origin)
    );

    return true;
  }

  if (! known)
  {
    dll_log.Log ( L"[SteammAPI] UNKNOWN Variable Changed (%p --> %p)",
                     var,
                       val );
  }

  return false;
}

#if 0
struct BaseStats_t
{
  uint64  m_nGameID;
  EResult status;
  uint64  test;
};

S_API typedef void (S_CALLTYPE *steam_callback_run_t)
                       (CCallbackBase *pThis, void *pvParam);

S_API typedef void (S_CALLTYPE *steam_callback_run_ex_t)
                       (CCallbackBase *pThis, void *pvParam, bool, SteamAPICall_t);

steam_callback_run_t    Steam_Callback_RunStat_Orig   = nullptr;
steam_callback_run_ex_t Steam_Callback_RunStatEx_Orig = nullptr;

S_API
void
S_CALLTYPE
Steam_Callback_RunStat (CCallbackBase *pThis, void *pvParam)
{
  steam_log.Log ( L"CCallback::Run (%04Xh, %04Xh)  <Stat %s>;",
                    pThis, pvParam, pThis->GetICallback () == 1002L ? L"Store" : L"Receive" );

  BaseStats_t* stats = (BaseStats_t *)pvParam;

  steam_log.Log ( L" >> Size: %04i, Event: %04i - %04lu, %04llu, %04llu\n", pThis->GetCallbackSizeBytes (), pThis->GetICallback (), stats->status, stats->m_nGameID, stats->test);

  Steam_Callback_RunStat_Orig (pThis, pvParam);
}

S_API
void
S_CALLTYPE
Steam_Callback_RunStatEx (CCallbackBase *pThis, void           *pvParam,
                          bool           tf,    SteamAPICall_t  call)
{
  steam_log.Log ( L"CCallback::Run (%04Xh, %04Xh, %01i, %04llu)  "
                  L"<Stat %s>;",
                    pThis, pvParam, tf, call, pThis->GetICallback () == 1002L ? L"Store" : L"Receive" );

  BaseStats_t* stats = (BaseStats_t *)pvParam;

  steam_log.Log ( L" >> Size: %04i, Event: %04i - %04lu, %04llu, %04llu\n", pThis->GetCallbackSizeBytes (), pThis->GetICallback (), stats->status, stats->m_nGameID, stats->test);

  Steam_Callback_RunStatEx_Orig (pThis, pvParam, tf, call);
}
#endif

#define FREEBIE     96.0f
#define COMMON      75.0f
#define UNCOMMON    50.0f
#define RARE        25.0f
#define VERY_RARE   15.0f
#define ONE_PERCENT  1.0f

#include <imgui/imgui.h>

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

  return SK_FormatString ("FF%02X%02X%02X", (int)(color.x * 255.0f),
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

      const char* human =
        stats->GetAchievementDisplayAttribute (szName, "name");

      const char* desc =
        stats->GetAchievementDisplayAttribute (szName, "desc");

      human_name_ =
        _strdup (human != nullptr ? human : "<INVALID>");
      desc_ =
        _strdup (desc != nullptr ? desc : "<INVALID>");
    }

    Achievement (Achievement& copy) = delete;

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
                &global_percent_ ) )
      {
        steam_log.Log (L" Global Achievement Read Failure For '%hs'", name_);
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
       unlock_listener ( this, &SK_Steam_AchievementManager::OnUnlock                ),
       stat_receipt    ( this, &SK_Steam_AchievementManager::AckStoreStats           ),
       icon_listener   ( this, &SK_Steam_AchievementManager::OnRecvIcon              ),
       async_complete  ( this, &SK_Steam_AchievementManager::OnAsyncComplete         )
  {
    percent_unlocked = 0.0f;
    lifetime_popups  = 0;

    ISteamUserStats* user_stats = nullptr;
    ISteamFriends*   friends    = nullptr;

    if (! (user_stats = steam_ctx.UserStats ()))
    {
      steam_log.Log ( L" *** Cannot get ISteamUserStats interface, bailing-out of"
                      L" Achievement Manager Init. ***" );
      return;
    }

    if (! (friends = steam_ctx.Friends ()))
    {
      steam_log.Log ( L" *** Cannot get ISteamFriends interface, bailing-out of"
                      L" Achievement Manager Init. ***" );
      return;
    }

    loadSound (wszUnlockSound);

    uint32_t achv_reserve =
      std::max (user_stats->GetNumAchievements (), (uint32_t)128UL);
    friend_count =
      friends->GetFriendCount (k_EFriendFlagImmediate);

    // OFFLINE MODE
    if (friend_count <= 0 || friend_count > 8192)
      friend_count = 0;

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

      steam_log.LogEx (false, L"\n [%c] Achievement %03lu......: '%hs'\n",
                         achievement.unlocked_ ? L'X' : L' ',
                           i, achievement.name_
                      );
      steam_log.LogEx (false,
                              L"  + Human Readable Name...: %hs\n",
                         achievement.human_name_);
      if (achievement.desc_ != nullptr && strlen (achievement.desc_))
        steam_log.LogEx (false,
                                L"  *- Detailed Description.: %hs\n",
                          achievement.desc_);

      if (achievement.global_percent_ > 0.0f)
        steam_log.LogEx (false,
                                L"  #-- Rarity (Global).....: %6.2f%%\n",
                          achievement.global_percent_);

      if (achievement.friends_.possible > 0)
        steam_log.LogEx ( false,
                                 L"  #-- Rarity (Friend).....: %6.2f%%\n",
                           100.0 * (static_cast <double> (achievement.friends_.unlocked) /
                                    static_cast <double> (achievement.friends_.possible)) );

      if (achievement.unlocked_)
      {
        steam_log.LogEx (false,
                                L"  @--- Player Unlocked At.: %s",
                                  _wctime32 (&achievement.time_));
      }
    }

    steam_log.LogEx (false, L"\n");
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
      steam_log.Log ( L" Got User Achievement Stats for Wrong Game (%lu)",
                        app_id );
      return;
    }

    if (pParam->m_eResult == k_EResultOK)
    {
      ISteamUser*      user    = steam_ctx.User      ();
      ISteamUserStats* stats   = steam_ctx.UserStats ();
      ISteamFriends*   friends = steam_ctx.Friends   ();

      if (! (user && stats && friends && pParam))
        return;

      if ( pParam->m_steamIDUser.GetAccountID () ==
           user->GetSteamID ().GetAccountID   () )
      {
        for ( auto it : UserStatsReceived_callbacks )
        {
          if (it.second)
            ((class CCallbackBase *)it.first)->Run (pParam);
        }

        pullStats ();

        if (config.steam.achievements.pull_global_stats && (! has_global_data))
        {
          has_global_data = true;

          global_request =
            stats->RequestGlobalAchievementPercentages ();
        }

        // On first stat reception, trigger an update for all friends so that
        //   we can display friend achievement stats.
        if ( next_friend == 0 && config.steam.achievements.pull_friend_stats )
        {
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
              friend_stats      [i].account_id = sid.ConvertToUint64 ();
              friend_stats      [i].unlocked.resize (achievements.list.capacity ());
            }

            SteamAPICall_t hCall =
              stats->RequestUserStats (CSteamID (friend_stats [next_friend++].account_id));

            friend_query = hCall;

            friend_listener.Set (
              hCall,
                this,
                  &SK_Steam_AchievementManager::OnRecvFriendStats );
          }
        }
      }
    }
  }

  STEAM_CALLRESULT ( SK_Steam_AchievementManager,
                     OnRecvFriendStats,
                     UserStatsReceived_t,
                     friend_listener )
  {
    friend_listener.Cancel ();


    if (ReadAcquire (&__SK_DLL_Ending))
      return;


    uint32 app_id =
      CGameID (pParam->m_nGameID).AppID ();

    if (app_id != SK::SteamAPI::AppID ())
    {
      steam_log.Log ( L" Got User Achievement Stats for Wrong Game (%lu)",
                        app_id );
      return;
    }

    ISteamUser*      user    = steam_ctx.User      ();
    ISteamUserStats* stats   = steam_ctx.UserStats ();
    ISteamFriends*   friends = steam_ctx.Friends   ();

    if (! (user && stats && friends && pParam))
      return;

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
          char* szName = _strdup (stats->GetAchievementName (i));

          stats->GetUserAchievement (
            pParam->m_steamIDUser,
              szName,
                reinterpret_cast <bool *> (
                  &friend_stats [friend_idx].unlocked [i] )
          );

          if (friend_stats [friend_idx].unlocked [i])
          {
            unlocked++;

            // On the first unlocked achievement, make a note...
            if (! can_unlock)
            {
              ///steam_log.Log ( L"Received Achievement Stats for Friend: '%hs'",
                                ///friends->GetFriendPersonaName (pParam->m_steamIDUser) );
            }

            can_unlock = true;

            ++achievements.list [i]->friends_.unlocked;

            ///steam_log.Log (L" >> Has unlocked '%24hs'", szName);
          }

          free (const_cast <char *> (szName));
        }

        friend_stats [friend_idx].percent_unlocked =
          static_cast <float> (unlocked) /
          static_cast <float> (stats->GetNumAchievements ());

        // Second pass over all achievements, incrementing the number of friends who
        //   can potentially unlock them.
        if (can_unlock)
        {
          for (uint32 i = 0; i < stats->GetNumAchievements (); i++)
            ++achievements.list [i]->friends_.possible;
        }
      }
    }

    // TODO: The function that walks friends should be a lambda
    else
    {
      // Friend doesn't own the game, so just skip them...
      if (pParam->m_eResult == k_EResultFail)
      {
      }

      else
      {
        steam_log.Log ( L" >>> User stat receipt unsuccessful! (m_eResult = 0x%04X)",
                        pParam->m_eResult );
      }
    }

    if (friend_count > 0 && next_friend < friend_count)
    {
      CSteamID sid (friend_stats [next_friend++].account_id);

      SteamAPICall_t hCall =
        stats->RequestUserStats (sid);

      friend_query = hCall;

      friend_listener.Set (
        hCall,
          this,
            &SK_Steam_AchievementManager::OnRecvFriendStats );
    }

    else
    {
      friends_done = ReadAcquire64 (&SK_SteamAPI_CallbackRunCount);
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
      steam_ctx.Utils ()->IsAPICallCompleted (pParam->m_hAsyncCall, &failed);

      if (failed)
        eCallFail = steam_ctx.Utils ( )->GetAPICallFailureReason (pParam->m_hAsyncCall);
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
        steam_log.Log ( L" Non-Zero Result (%li) for GlobalAchievementPercentagesReady_t",
                          eCallFail );

        has_global_data = false;
        return;
      }
    }

    else {
      if (failed)
      {
        //steam_log.Log ( L" Asynchronous SteamAPI Call _NOT ISSUED BY Special K_ Failed: "
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

    steam_log.Log ( L" >> Stats Stored for AppID: %lu",
                      app_id );
  }

  STEAM_CALLBACK ( SK_Steam_AchievementManager,
                   OnRecvIcon,
                   UserAchievementIconFetched_t,
                   icon_listener )
  {
    if (pParam->m_nGameID.AppID () != SK::SteamAPI::AppID ())
    {
      steam_log.Log ( L" >>> Received achievement icon for unrelated game (AppId=%lu) <<<",
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
      steam_log.Log ( L" >>> Received achievement unlock for unrelated game (AppId=%lu) <<<",
                        app_id );
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
        steam_ctx.UserStats ()->GetAchievementIcon (pParam->m_rgchAchievementName);

      if ( pParam->m_nMaxProgress == pParam->m_nCurProgress )
      {
        // Do this so that 0/0 does not come back as NaN
        achievement->progress_.current = 1;
        achievement->progress_.max     = 1;

        if (config.steam.achievements.play_sound)
          PlaySound ( (LPCWSTR)unlock_sound, nullptr, SND_ASYNC | SND_MEMORY );

        steam_log.Log (L" Achievement: '%hs' (%hs) - Unlocked!",
          achievement->human_name_, achievement->desc_);

        // If the user wants a screenshot, but no popups (why?!), this is when
        //   the screenshot needs to be taken.
        if ( config.steam.achievements.take_screenshot &&
          (! ( config.steam.achievements.popup.show    && config.cegui.enable ) ) )
        {
          SK::SteamAPI::TakeScreenshot ();
        }

        // Re-calculate percentage unlocked
        pullStats ();
      }

      else
      {
        float progress =
          static_cast <float> (pParam->m_nCurProgress) /
          static_cast <float> (pParam->m_nMaxProgress);

        steam_log.Log (L" Achievement: '%hs' (%hs) - "
                       L"Progress %lu / %lu (%04.01f%%)",
                  achievement->human_name_,
                  achievement->desc_,
                        pParam->m_nCurProgress,
                        pParam->m_nMaxProgress,
                          100.0f * progress );
      }
    }

    else
    {
      steam_log.Log ( L" Got Rogue Achievement Storage: '%hs'",
                        pParam->m_rgchAchievementName );
    }

    if ( config.cegui.enable && config.steam.achievements.popup.show &&
         config.cegui.frames_drawn > 0 &&
         achievement != nullptr )
    {
      CEGUI::System* pSys = CEGUI::System::getDllSingletonPtr ();

      if ( pSys                 != nullptr &&
           pSys->getRenderer () != nullptr )
      {
        steam_popup_cs->lock ();

        try
        {
          SK_AchievementPopup popup;

          popup.window      = nullptr;
          popup.final_pos   = false;
          popup.time        = timeGetTime ();
          popup.achievement = achievement;

          popups.push_back (popup);
        }

        catch (...)
        {
        }

        steam_popup_cs->unlock ();
      }
    }
  }

  void clearPopups (void)
  {
    steam_popup_cs->lock ();

    if (popups.empty ())
    {
      steam_popup_cs->unlock ();
      return;
    }

    popups.clear ();

    steam_popup_cs->unlock ();
  }

  int drawPopups (void)
  {
    if (! (config.cegui.enable && config.cegui.frames_drawn > 0)) return 0;


    int drawn = 0;

    steam_popup_cs->lock ();

    if (popups.empty ())
    {
      steam_popup_cs->unlock ();
      return drawn;
    }

    //
    // We don't need this, we always do this from the render thread.
    //
    //if (SK_PopupManager::getInstance ()->tryLockPopups ())
    {
      try
      {
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
          CEGUI::System::getDllSingleton ().getRenderer ()->
            getDisplaySize ().d_height * (1.0f - inset);

        const float full_wd =
          CEGUI::System::getDllSingleton ().getRenderer ()->
            getDisplaySize ().d_width * (1.0f - inset);

        float x_origin, y_origin,
              x_dir,    y_dir;

        CEGUI::Window* first = it->window;

        const float win_ht0 = first != nullptr ?
                                (it->window->getPixelSize ().d_height) :
                                  0.0f;
        const float win_wd0 = first != nullptr ?
                                (it->window->getPixelSize ().d_width) :
                                  0.0f;

        const float title_wd =
          CEGUI::System::getDllSingleton ().getRenderer ()->
            getDisplaySize ().d_width * (1.0f - 2.0f * inset);

        const float title_ht =
          CEGUI::System::getDllSingleton ().getRenderer ()->
            getDisplaySize ().d_height * (1.0f - 2.0f * inset);

        float fract_x, fract_y;

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

        while (it != popups.end ())
        {
          if (timeGetTime () < (*it).time + POPUP_DURATION_MS)
          {
            float percent_of_lifetime = (static_cast <float> ((*it).time + POPUP_DURATION_MS - timeGetTime ()) /
                                         static_cast <float> (POPUP_DURATION_MS));

            //if (SK_PopupManager::getInstance ()->isPopup ((*it).window)) {
              CEGUI::Window* win = (*it).window;

              if (win == nullptr)
              {
                win = createPopupWindow (&*it);
                created = true;
              }

              const float win_ht =
                win->getPixelSize ().d_height;

              const float win_wd =
                win->getPixelSize ().d_width;

              CEGUI::UVector2 win_pos (x_pos, y_pos);

              float bottom_y = win_pos.d_y.d_scale * full_ht +
                                 win_pos.d_y.d_offset        +
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
                                 win_pos.d_x.d_offset       +
                                   win_wd;

              // Window is off-screen horizontally AND vertically
              if ( inset != 0.0f && (right_x > full_wd || right_x < win_wd0)  )
              {
                // Since we're not going to draw this, treat it as a new
                //   popup until it first becomes visible.
                (*it).time =
                     timeGetTime ();
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
                    it->final_pos   = true;
                    take_screenshot = it->achievement->unlocked_ ? 2 : 0;
                  }
                }

                else if (! it->final_pos)
                {
                  take_screenshot = it->achievement->unlocked_ ? 2 : 0;
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
            SK::SteamAPI::TakeScreenshot (SK::ScreenshotStage::EndOfFrame);
            take_screenshot = -1;
          }
        }

        ++drawn;
      }

      catch (...) {}
    }
    //SK_PopupManager::getInstance ()->unlockPopups ();
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
    if (steam_ctx.UserStats ())
    {
      if (steam_ctx.UserStats ()->RequestCurrentStats ())
      {
        SteamAPICall_t hCall =
          steam_ctx.UserStats ()->RequestUserStats (
            steam_ctx.User ()->GetSteamID ()
          );

        self_listener.Set (
          hCall,
            this,
              &SK_Steam_AchievementManager::OnRecvSelfStats );
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
        //steam_log.Log (L"  >> %hs", stats->GetAchievementName (i));

        if (achievements.list [i] == nullptr)
        {
          char* szName = _strdup (stats->GetAchievementName (i));

          achievements.list [i] =
            new Achievement (
                  i,
                  szName,
                  stats
                );

          achievements.string_map [achievements.list [i]->name_] =
            achievements.list [i];

          achievements.list [i]->update (stats);

          //steam_log.Log (L"  >> (%lu) %hs", i, achievements.list [i]->name_);

          // Start pre-loading images so we do not hitch on achievement unlock...

          //if (! achievements.list [i]->unlocked_)
          //stats->SetAchievement   (szName);

          // After setting the achievement, fetch the icon -- this is
          //   necessary so that the unlock dialog does not display
          //     the locked icon.
          if ( steam_ctx.UserStats () )
            steam_ctx.UserStats ()->GetAchievementIcon (szName);

          //if (! achievements.list [i]->unlocked_)
            //stats->ClearAchievement (szName);

          free (const_cast <char *> (szName));
        }

        else
        {
          achievements.list [i]->update (stats);
        }

        if (achievements.list [i]->unlocked_)
          unlocked++;
      }

      percent_unlocked =
        static_cast <float> (unlocked) /
        static_cast <float> (stats->GetNumAchievements ());
    }
  }

  void
  loadSound (const wchar_t* wszUnlockSound)
  {
    bool xbox = false,
         psn  = false,
         dt   = false;

    wchar_t wszFileName [MAX_PATH * 2 + 1] = { };

    if (*wszUnlockSound == L'\0')
    {
      iSK_INI achievement_ini (
        std::wstring (
          SK_GetDocumentsDir () + LR"(\My Mods\SpecialK\Global\achievements.ini)"
        ).c_str () );

      // If the config file is empty, establish defaults and then write it.
      if (achievement_ini.get_sections ().empty ())
      {
        achievement_ini.import ( L"[Steam.Achievements]\n"
                                 L"SoundFile=psn\n"
                                 L"PlaySound=true\n"
                                 L"TakeScreenshot=false\n"
                                 L"AnimatePopup=true\n"
                                 L"NotifyCorner=0\n" );
        achievement_ini.write (
          std::wstring (
            SK_GetDocumentsDir () + LR"(\My Mods\SpecialK\Global\achievements.ini)"
          ).c_str () );
      }

      if (achievement_ini.contains_section (L"Steam.Achievements"))
      {
        iSK_INISection& sec =
          achievement_ini.get_section (L"Steam.Achievements");

        if (sec.contains_key (L"SoundFile"))
          wcscpy (wszFileName, sec.get_value (L"SoundFile").c_str ());
      }
    }

    else
    {
      wcscpy (wszFileName, wszUnlockSound);
    }

    if ((!     _wcsicmp (wszFileName,  L"psn")))
      psn  = true;
    else if (! _wcsicmp (wszFileName,  L"xbox"))
      xbox = true;
    else if ((! _wcsicmp (wszFileName, L"dream_theater")))
      dt   = true;

    FILE *fWAV = nullptr;

    if ( (!  psn) &&
         (! xbox) && 
         (!   dt) && (fWAV = _wfopen (wszFileName, L"rb")) != nullptr )
    {
      SK_StripUserNameFromPathW (wszFileName);

      steam_log.LogEx (true, L"  >> Loading Achievement Unlock Sound: '%s'...",
              SK_StripUserNameFromPathW (std::wstring (wszFileName).data ()) );

                  fseek  (fWAV, 0, SEEK_END);
      long size = ftell  (fWAV);
                  rewind (fWAV);

      unlock_sound =
        static_cast <uint8_t *> (
          malloc (size)
        );

      if (unlock_sound != nullptr)
        fread  (unlock_sound, size, 1, fWAV);

      fclose (fWAV);

      steam_log.LogEx (false, L" %d bytes\n", size);

      default_loaded = false;
    }

    else
    {
      // Default to PSN if not specified
      if ((! psn) && (! xbox) && (! dt))
        psn = true;

      steam_log.Log (L"  * Loading Built-In Achievement Unlock Sound: '%s'",
                       wszFileName);

      HRSRC   default_sound;

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
    return friend_stats [friend_idx].name;
  }

  float getFriendUnlockPercentage (uint32_t friend_idx) const
  {
    return friend_stats [friend_idx].percent_unlocked;
  }

  bool hasFriendUnlockedAchievement (uint32_t friend_idx, uint32_t achv_idx) const
  {
    return friend_stats [friend_idx].unlocked [achv_idx];
  }

protected:
  CEGUI::Window* createPopupWindow (SK_AchievementPopup* popup)
  {
    if (! (config.cegui.enable && config.cegui.frames_drawn > 0)) return nullptr;

    if (popup->achievement == nullptr)
      return nullptr;

    CEGUI::System* pSys = CEGUI::System::getDllSingletonPtr ();

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
    char szGlobal [32] = { };
    snprintf ( szGlobal, 32,
                 "Global: %6.2f%%",
                   achievement->global_percent_ );
    achv_global->setText (szGlobal);

    CEGUI::Window* achv_friend = achv_popup->getChild ("FriendRarity");
    char szFriend [32] = { };
    snprintf ( szFriend, 32,
                 "Friends: %6.2f%%",
        100.0 * ((double)          achievement->friends_.unlocked /
                 (double)std::max (achievement->friends_.possible, 1)) );
    achv_friend->setText (szFriend);


    // If the server hasn't updated the unlock time, just use the current time
    if (achievement->time_ == 0)
      _time32 (&achievement->time_);

    CEGUI::Window* achv_unlock  = achv_popup->getChild ("UnlockTime");
    CEGUI::Window* achv_percent = achv_popup->getChild ("ProgressBar");

    float progress = achievement->progress_.getPercent ();

    char szUnlockTime [128] = { };
    if (progress == 100.0f)
    {
      snprintf ( szUnlockTime, 128,
                   "Unlocked: %s", _ctime32 (&achievement->time_)
               );

      achv_percent->setProperty ( "CurrentProgress", "1.0" );
    }

    else
    {
      snprintf ( szUnlockTime, 16,
                   "%5.4f",
                     progress / 100.0f
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

    int icon_idx =
      steam_ctx.UserStats () ?
        steam_ctx.UserStats ()->GetAchievementIcon (achievement->name_) :
        0;

    // Icon width and height
    uint32 w = 0,
           h = 0;

    if (icon_idx != 0)
    {
      if (steam_ctx.Utils ())
        steam_ctx.Utils ()->GetImageSize (icon_idx, &w, &h);

      int tries = 1;

      while (achievement->icons_.achieved == nullptr && tries < 8)
      {
        achievement->icons_.achieved =
          (uint8_t *)malloc (4 * w * h);

        if (steam_ctx.Utils ())
        {
          if ( ! steam_ctx.Utils ()->GetImageRGBA (
                   icon_idx,
                     achievement->icons_.achieved,
                       4 * w * h
                 )
             )
          {
            free (achievement->icons_.achieved);
            achievement->icons_.achieved = nullptr;
            ++tries;
          }

          else
          {
            steam_log.Log ( L" * Fetched RGBA Icon (idx=%li) for Achievement: '%hs'  (%lux%lu) "
                              L"{ Took %li try(s) }",
                              icon_idx, achievement->name_, w, h, tries );
          }
        }
      }
    }

    if (achievement->icons_.achieved != nullptr)
    {
      bool exists = CEGUI::ImageManager::getDllSingleton ().isDefined (achievement->name_);

      CEGUI::Image& img =
         exists ?
          CEGUI::ImageManager::getDllSingleton ().get    (              achievement->name_) :
          CEGUI::ImageManager::getDllSingleton ().create ("BasicImage", achievement->name_);

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

      catch (...)
      {
      }

      try
      {
        CEGUI::Window* staticImage = achv_popup->getChild ("Icon");
        staticImage->setProperty ( "Image",
                                     achievement->name_ );
      }

      catch (...)
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
} static *steam_achievements = nullptr;

void
SK_Steam_LoadUnlockSound (const wchar_t* wszUnlockSound)
{
  if (steam_achievements != nullptr)
    steam_achievements->loadSound (wszUnlockSound);
}

#if 0
S_API typedef void (S_CALLTYPE *steam_unregister_callback_t)
                       (class CCallbackBase *pCallback);
S_API typedef void (S_CALLTYPE *steam_register_callback_t)
                       (class CCallbackBase *pCallback, int iCallback);

steam_register_callback_t SteamAPI_RegisterCallbackOrig = nullptr;


S_API bool S_CALLTYPE SteamAPI_Init_Detour (void);
#endif

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
    steam_log.Log ( L" >> Steam Achievement Manager Not Yet Initialized ... "
                    L"Skipping Unlock <<" );
    return;
  }

  steam_log.LogEx (true, L" >> Attempting to Unlock Achievement: %lu... ",
    idx );

  ISteamUserStats* stats = steam_ctx.UserStats ();

  if ( stats                        && idx <
       stats->GetNumAchievements () )
  {
    const char* szName =
      _strdup (stats->GetAchievementName (idx));

    if (szName != nullptr)
    {
      steam_log.LogEx (false, L" (%hs - Found)\n", szName);

      steam_achievements->pullStats ();

      UserAchievementStored_t store;
      store.m_nCurProgress = 0;
      store.m_nMaxProgress = 0;
      store.m_nGameID      = CGameID (SK::SteamAPI::AppID ()).ToUint64 ();
      strncpy (store.m_rgchAchievementName, szName, 128);

      SK_Steam_AchievementManager::Achievement* achievement =
        steam_achievements->getAchievement (
          store.m_rgchAchievementName
        );

      UNREFERENCED_PARAMETER (achievement);

      steam_achievements->OnUnlock (&store);

//      stats->ClearAchievement            (szName);
//      stats->IndicateAchievementProgress (szName, 0, 1);
//      stats->StoreStats                  ();
#if 0
      bool achieved;
      if (stats->GetAchievement (szName, &achieved))
      {
        if (achieved)
        {
          steam_log.LogEx (true, L"Clearing first\n");
          stats->ClearAchievement            (szName);
          stats->StoreStats                  ();

          SteamAPI_RunCallbacks              ();
        } else {
          steam_log.LogEx (true, L"Truly unlocking\n");
          stats->SetAchievement              (szName);

          stats->StoreStats                  ();

          // Dispatch these ASAP, there's a lot of latency apparently...
          SteamAPI_RunCallbacks ();
        }
      }
      else {
        steam_log.LogEx (true, L" >> GetAchievement (...) Failed\n");
      }

#endif
      free (const_cast <char *> (szName));
    }

    else
      steam_log.LogEx (false, L" (None Found)\n");
  }

  else
    steam_log.LogEx   (false, L" (ISteamUserStats is NULL?!)\n");

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
        dll_log.Log ( L" Got Global Data For Unknown Achievement ('%hs')",
                        szName.c_str () );
      }
    }
  }

  else
  {
    steam_log.Log ( L"Got Global Stats from SteamAPI, but no "
                    L"ISteamUserStats interface exists?!" );
  }

  if (steam_achievements)
    SK_Steam_LogAllAchievements ();
}

void
SK_Steam_ClearPopups (void)
{
  if (steam_achievements != nullptr)
  {
    steam_achievements->clearPopups ();
    SteamAPI_RunCallbacks_Original  ();
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
        InterlockedExchange (&num_players_call_,
          user_stats->GetNumberOfCurrentPlayers ());

        current_players_call.Set (
          InterlockedAdd64 ((LONG64 *)&num_players_call_, 0LL),
            this,
              &SK_Steam_UserManager::OnRecvNumCurrentPlayers );
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
} static *user_manager = nullptr;


int
SK_Steam_DrawOSD (void)
{
  if (steam_achievements != nullptr)
  {
    //SteamAPI_RunCallbacks_Original ();
    return steam_achievements->drawPopups ();
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


    static LONG          lastFrame        =
      SK_GetFramesDrawn ();
    static LARGE_INTEGER liLastCallbacked =
      SK_CurrentPerf    ();


    LONG limit = ReadAcquire (&SK_SteamAPI_CallbackRateLimit);
    if  (limit < 0) return false;


    if ( limit == 0 ||
         ( SK_CurrentPerf ().QuadPart - liLastCallbacked.QuadPart  <
           SK_GetPerfFreq ().QuadPart / limit ) )
    {
      InterlockedDecrement64 ( &SK_SteamAPI_CallbackRunCount );
      return true;
    }

    lastFrame        = SK_GetFramesDrawn ();
    liLastCallbacked = SK_CurrentPerf    ();
  }

  return false;
}

extern SK_Thread_HybridSpinlock* steam_mutex;

void
S_CALLTYPE
SteamAPI_RunCallbacks_Detour (void)
{
  if (ReadAcquire (&__SK_DLL_Ending))
    return;


  static bool failure = false;

  SK_TLS* pTLS = SK_TLS_Bottom ();

  if (pTLS != nullptr) InterlockedIncrement64 (&pTLS->steam.callback_count);



  if (! steam_mutex->try_lock ())
  {
    return;
  }


  if (SK_Steam_ShouldThrottleCallbacks ())
  {
    steam_mutex->unlock ();

    return;
  }


  if ((! failure) && (( ReadAcquire64 (&SK_SteamAPI_CallbackRunCount) == 0LL || steam_achievements == nullptr )))
  {
    // Handle situations where Steam was initialized earlier than
    //   expected...
    void SK_SteamAPI_InitManagers (void);
         SK_SteamAPI_InitManagers (    );

    strcpy ( steam_ctx.var_strings.popup_origin,
               SK_Steam_PopupOriginToStr (
                 config.steam.achievements.popup.origin
               )
           );
    
    strcpy ( steam_ctx.var_strings.notify_corner,
               SK_Steam_PopupOriginToStr (
                 config.steam.notify_corner
               )
           );

    if (! steam_ctx.UserStats ())
    {
      if (SteamAPI_InitSafe_Original != nullptr)
        SteamAPI_InitSafe_Detour ();
    }
    
    __try
    {
      SK_Steam_SetNotifyCorner ();

      SteamAPI_RunCallbacks_Original ();

      ISteamUserStats* pStats =
        steam_ctx.UserStats ();

      if (pStats && (steam_achievements != nullptr))
      {
        pStats->RequestGlobalAchievementPercentages ();
        SteamAPI_RunCallbacks_Original ();
      }
    }
    
    __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION )  ?
                         EXCEPTION_EXECUTE_HANDLER :
                         EXCEPTION_CONTINUE_SEARCH )
    {
      failure = true;
      steam_log.Log (L" Caught a Structured Exception while running Steam Callbacks!");
    }
  }

  __try
  {
    if (! ReadAcquire (&__SK_DLL_Ending))
    {
      DWORD dwNow = timeGetTime ();

      static DWORD
          dwLastOnlineStateCheck = 0UL;
      if (dwLastOnlineStateCheck < dwNow - 666UL)
      {
        if ( config.steam.online_status       != -1 && 
             SK::SteamAPI::GetPersonaState () != config.steam.online_status )
        {
          SK::SteamAPI::SetPersonaState ((EPersonaState)config.steam.online_status);
        }

        dwLastOnlineStateCheck = dwNow;
      }

      SteamAPI_RunCallbacks_Original ();
    }
  }

  __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION )  ?
                       EXCEPTION_EXECUTE_HANDLER :
                       EXCEPTION_CONTINUE_SEARCH )
  {
    if (! failure)
    {
      steam_log.Log (L" Caught a Structured Exception while running Steam Callbacks!");
      failure = true;
    }
  }

  steam_mutex->unlock ();
}


#define STEAMAPI_CALL1(x,y,z) ((x) = SteamAPI_##y z)
#define STEAMAPI_CALL0(y,z)   (SteamAPI_##y z)

#include <Windows.h>

#include <ctime>

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

  steam_ctx.Shutdown ();
}

void SK::SteamAPI::Pump (void)
{
  if (steam_ctx.UserStats ())
  {
    if (SteamAPI_RunCallbacks_Original != nullptr)
        SteamAPI_RunCallbacks_Detour ();
  }
}

volatile HANDLE hSteamPump = nullptr;

DWORD
WINAPI
SteamAPI_PumpThread (LPVOID user)
{
  if (SK_GetCurrentGameID () == SK_GAME_ID::FinalFantasyXV)
  {
    SK_Thread_CloseSelf ();

    InterlockedExchangePointer ((void **)&hSteamPump, nullptr);

    return 0;
  }

  SetCurrentThreadDescription (               L"[SK] SteamAPI Callback Pump" );
  SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_IDLE );

  bool   start_immediately = (user != nullptr);
  double callback_freq     =  0.0;

  if (SK_GetCurrentGameID () == SK_GAME_ID::MonsterHunterWorld)
    start_immediately = true;

  if (! start_immediately)
  {
    // Wait 5 seconds, then begin a timing investigation
    SleepEx (5000, FALSE);

    // First, begin a timing probe.
    //
    //   If after 30 seconds the game has not called SteamAPI_RunCallbacks
    //     frequently enough, switch the thread to auto-pump mode.

    const UINT TEST_PERIOD = 30;

    LONGLONG callback_count0 = SK_SteamAPI_CallbackRunCount;

    SleepEx (TEST_PERIOD * 1000UL, FALSE);

    LONGLONG callback_count1 = SK_SteamAPI_CallbackRunCount;

    callback_freq = (double)(callback_count1 - callback_count0) / (double)TEST_PERIOD;

    steam_log.Log ( L"Game runs its callbacks at ~%5.2f Hz ... this is %s the "
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

      steam_log.Log ( L" >> Installing a callback auto-pump at 8 Hz.\n\n");

      while (! ReadAcquire (&__SK_DLL_Ending))
      {
        SK::SteamAPI::Pump ();

        SleepEx (125, FALSE);
      }
    }

    else
    {
      steam_log.Log ( L" >> Tried to install a callback auto-pump, but "
                      L"could not initialize SteamAPI.\n\n" );
    }
  }

  SK_Thread_CloseSelf ();
  
  InterlockedExchangePointer ((void **)&hSteamPump, nullptr);

  return 0;

}

void
SK_Steam_StartPump (bool force)
{
  if (ReadAcquire (&__SK_DLL_Ending)) return;

  if (ReadPointerAcquire ((void **)&hSteamPump) != nullptr)
    return;

  if (config.steam.auto_pump_callbacks || force)
  {
    LPVOID start_params =
      force ? (LPVOID)1 : nullptr;

    InterlockedCompareExchangePointer ( (void **)&hSteamPump, (HANDLE)
                       SK_Thread_CreateEx ( SteamAPI_PumpThread, nullptr,
                                              start_params ), nullptr
                        );
  }
}

void
SK_Steam_KillPump (void)
{
  if (ReadAcquire (&__SK_DLL_Ending)) return;

  CHandle hOriginal (
    reinterpret_cast <HANDLE>
      (InterlockedExchangePointer ((void **)&hSteamPump, nullptr))
  );

  if (hOriginal != nullptr)
  {
    TerminateThread (hOriginal, 0x00);
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

uint32_t
SK::SteamAPI::AppID (void)
{
  ISteamUtils* utils = steam_ctx.Utils ();

  if (utils != nullptr)
  {
    static uint32_t id    = 0;
    static bool     first = true;

    if (first)
    {
      id = utils->GetAppID ();

      if (id != 0)
      {
        first = false;

        if ( config.system.central_repository                        &&
          (! app_cache_mgr.getAppIDFromPath (SK_GetFullyQualifiedApp ())) )
        {
          app_cache_mgr.addAppToCache      (SK_GetFullyQualifiedApp (  ),
                                            SK_GetHostApp           (  ),
                      SK_UTF8ToWideChar (SK_UseManifestToGetAppName (id)).c_str (), id);
          app_cache_mgr.saveAppCache       (true);

          app_cache_mgr.loadAppCacheForExe (SK_GetFullyQualifiedApp ());

          // Trigger profile migration if necessary
          app_cache_mgr.getConfigPathForAppID (id);
        }
      }
    }

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
         DWORD   len         = MAX_PATH;
  static wchar_t wszSteamPath [MAX_PATH + 2];

  LSTATUS status =
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
    return nullptr;
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
      wchar_t wszLibraryFolders [MAX_PATH * 2 + 1] = { };

      lstrcpyW (wszLibraryFolders, wszSteamPath);
      lstrcatW (wszLibraryFolders, LR"(\steamapps\libraryfolders.vdf)");

      CHandle hLibFolders (
        CreateFileW ( wszLibraryFolders,
                        GENERIC_READ,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                            nullptr,
                              OPEN_EXISTING,
                                GetFileAttributesW (wszLibraryFolders),
                                  nullptr )
      );

      if (hLibFolders != INVALID_HANDLE_VALUE)
      {
        DWORD  dwSize     = 0,
               dwSizeHigh = 0,
               dwRead     = 0;

        dwSize =
          GetFileSize (hLibFolders, &dwSizeHigh);

        SK_TLS *pTLS =
          SK_TLS_Bottom ();

        char* data = (char *)
          pTLS->steam.allocScratchText (dwSize + 1);

        if (data == nullptr)
          return steam_libs;

        dwRead = dwSize;

        if (ReadFile (hLibFolders, data, dwSize, &dwRead, nullptr))
        {
          for (DWORD i = 0; i < dwSize; i++)
          {
            if (data [i] == '"' && i < dwSize - 3)
            {
              if (data [i + 2] == '"')
                i += 2;
              else if (data [i + 3] == '"')
                i += 3;
              else
                continue;

              char* lib_start = nullptr;

              for (DWORD j = i; j < dwSize; j++,i++)
              {
                if (data [j] == '"' && lib_start == nullptr && j < dwSize - 1)
                {
                  lib_start =
                    &data [j + 1];
                }

                else if (data [j] == '"' && lib_start != nullptr)
                {
                  data [j] = '\0';

                  wsprintfW (
                    (wchar_t *)steam_lib_paths [steam_libs++],
                      L"%hs",
                        lib_start
                  );

                  lib_start = nullptr;
                }
              }
            }
          }
        }
      }

      // Finally, add the default Steam library
      wcsncpy_s ( (wchar_t *)steam_lib_paths [steam_libs++],
                    MAX_PATH * 2,
                      wszSteamPath,
                        _TRUNCATE );
    }

    scanned_libs = true;
  }

  if (ppLibraries != nullptr)
    *ppLibraries = steam_lib_paths;

  return steam_libs;
}

std::string
SK_UseManifestToGetAppName (uint32_t appid)
{
  steam_library_t* steam_lib_paths = nullptr;
  int              steam_libs      = SK_Steam_GetLibraries (&steam_lib_paths);

  if (steam_libs != 0)
  {
    for (int i = 0; i < steam_libs; i++)
    {
      wchar_t wszManifest [MAX_PATH * 2 + 1] = { };

      wsprintf ( wszManifest,
                  LR"(%s\steamapps\appmanifest_%u.acf)",
                    (wchar_t *)steam_lib_paths [i],
                      appid );

      CHandle hManifest (
        CreateFileW ( wszManifest,
                      GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                          nullptr,
                            OPEN_EXISTING,
                              GetFileAttributesW (wszManifest),
                                nullptr )
      );

      if (hManifest != INVALID_HANDLE_VALUE)
      {
        DWORD  dwSize     = 0,
               dwSizeHigh = 0,
               dwRead     = 0;

        dwSize =
          GetFileSize (hManifest, &dwSizeHigh);

        auto szManifestData =
          std::make_unique <char []> (
            std::size_t (dwSize + 1)
          );
        auto manifest_data =
          szManifestData.get ();

        if (! manifest_data)
          continue;

        bool bRead =
          ReadFile ( hManifest,
                       manifest_data,
                         dwSize,
                           &dwRead,
                             nullptr );

        if (! (bRead && dwRead))
        {
          continue;
        }

        char* szAppName =
          StrStrIA (manifest_data, R"("name")");

        if (szAppName != nullptr)
        {
          char szGameName [513] = { };

          // Make sure everything is lowercase
          memcpy (szAppName, R"("name")", 6);

          sscanf ( szAppName,
                     R"("name" "%512[^"]")",
                       szGameName );

          return szGameName;
        }
      }
    }
  }

  return "";
}

uint64_t
SK_Steam_ScrubRedistributables (int& total_files, bool erase)
{
  static volatile LONGLONG size    = 0LL;
  static volatile HANDLE   hThread =
    INVALID_HANDLE_VALUE;

  if (total_files != -1 && (! erase))
    return size;

  if ( INVALID_HANDLE_VALUE ==
         InterlockedCompareExchangePointer ( &hThread, (LPVOID)1,
                                               INVALID_HANDLE_VALUE ) )
  {
    struct scrub_params_s {
      int   &total_files;
      bool   erase;
    } static params { total_files, erase };

    params.total_files = total_files;
    params.erase       = erase;

    HANDLE hScrub =
    SK_Thread_CreateEx ([](LPVOID) ->
    DWORD
    {
      DWORD dwStart = timeGetTime ();

      SetCurrentThreadDescription (          L"[SK] Steam Redistributable File Cleanup" );
      SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_LOWEST |
                                                            THREAD_MODE_BACKGROUND_BEGIN );
      SetThreadPriorityBoost      ( SK_GetCurrentThread (), TRUE                         );

      unsigned int      files =  0 ;
      LARGE_INTEGER    liSize = { };

      steam_library_t* steam_lib_paths = nullptr;
                  int  steam_libs      = SK_Steam_GetLibraries (&steam_lib_paths);

      // Search custom library paths first
      if (steam_libs != 0)
      {
        for (int i = 0; i < steam_libs; i++)
        {
          WIN32_FIND_DATA fd        = {   };
          std::wstring    directory =
            SK_FormatStringW (LR"(%hs\steamapps\common)", (const char *)steam_lib_paths [i]);
          HANDLE          hFind     =
            FindFirstFileW   (_ConstructPath (directory, L"*").data (), &fd);

          if (hFind != INVALID_HANDLE_VALUE)
          {
            do
            {
              if (          (fd.dwFileAttributes  & FILE_ATTRIBUTE_DIRECTORY) &&
                  (_wcsicmp (fd.cFileName, L"." )                != 0)        &&
                  (_wcsicmp (fd.cFileName, L"..")                != 0)        &&
                  (_wcsicmp (fd.cFileName, L"Steamworks Shared") != 0) )
              {
                SK_Steam_RecursiveFileScrub ( _ConstructPath   (
                                                _ConstructPath ( directory, fd.cFileName ),
                                                                   L"_CommonRedist"
                                                               ).data (),
                                                files, liSize, 
                                                  L"*", L"installscript.vdf",
                                                    params.erase );
              }
            } while (FindNextFile (hFind, &fd));

            FindClose (hFind);
          }
        }
      }

      if (files > 0)
      {
        SK_LOG0 ( ( L"Common Redistributables: %lu files (%7.2f MiB)",
                      files, (float)(liSize.QuadPart / 1024ULL) / 1024.0f ),
                    L"SteamBloat" );
        SK_LOG0 ( ( L" >> Search Completed in %lu ms",
                      timeGetTime () - dwStart ),
                    L"SteamBloat" );
      }

      params.total_files = files;
      InterlockedExchange64 (&size, liSize.QuadPart);

      InterlockedExchangePointer ((void **)&hThread, INVALID_HANDLE_VALUE);

      SK_Thread_CloseSelf ();

      return 0;
    });

    InterlockedExchangePointer ((void **)&hThread, hScrub);
  }

  return ReadAcquire64 (&size);
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
  __try
  {
    GameOverlayActivated_t activated = {
      active
    };

#ifndef _WIN64
    UNREFERENCED_PARAMETER (CallbackFn);
    pClosure->Run (&activated);
#else
    CallbackFn (pClosure, &activated);
#endif

    SK::SteamAPI::overlay_state = active;
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    // Oh well, we literally tried...
  }
}


void
__stdcall
SK::SteamAPI::SetOverlayState (bool active)
{
  if (config.steam.silent)
    return;

  steam_callback_cs->lock ();

  for ( auto& it : overlay_activation_callbacks )
  {
    void** vftable = *(void ***)*(&it);

    SK_Steam_InvokeOverlayActivationCallback (
      reinterpret_cast <SK_Steam_Callback_pfn> ( vftable [0]),
                                                   it,
                                                     active
                                             );
  }

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
SK::SteamAPI::TakeScreenshot (SK::ScreenshotStage when)
{
  ISteamScreenshots* pScreenshots =
    steam_ctx.Screenshots ();

  if (pScreenshots != nullptr)
  {
    steam_log.LogEx (true, L"  >> Triggering Screenshot: ");

    if (config.steam.screenshots.enable_hook)
    {
      if ( (int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::D3D11 )
      {
        // Avoid any exotic pixel formats for now -- 8-bit RGB(A) only
        //if (SK_GetCurrentRenderBackend ().framebuffer_flags == 0x00)
        {
          SK_D3D11_CaptureSteamScreenshot (when);

          steam_log.LogEx ( false, L"Stage=%x (SK_SmartCapture)\n",
                              (int)when );
          return true;
        }
      }

      //else if ( (int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::D3D9 )
      //{
      //  // Avoid any exotic pixel formats for now -- 8-bit RGB(A) only
      //  //if (SK_GetCurrentRenderBackend ().framebuffer_flags == 0x00)
      //  {
      //    SK_D3D9_CaptureSteamScreenshot ( when );
      //
      //
      //    steam_log.LogEx ( false, L"Stage=%x (SK_SmartCapture)\n",
      //                        (int)when );
      //    return true;
      //  }
      //}
    }

    pScreenshots->TriggerScreenshot ();
    steam_log.LogEx (false, L"EndOfFrame (Steam Overlay)\n");

    if ( when != SK::ScreenshotStage::EndOfFrame )
      steam_log.Log (L" >> WARNINNG: Smart Capture disabled or unsupported; screenshot taken at end-of-frame.");

    return true;
  }

  return false;
}




bool
S_CALLTYPE
SteamAPI_InitSafe_Detour (void)
{
  // In case we already initialized stuff...
  if (ReadAcquire (&__SK_Steam_init))
    return SteamAPI_InitSafe_Original ();

  steam_init_cs->lock ();

  static int init_tries = -1;

  if (++init_tries == 0)
  {
    steam_log.Log ( L"Initializing SteamWorks Backend  << %s >>",
                      SK_GetCallerName ().c_str () );
    steam_log.Log (L"----------(InitSafe)-----------\n");
  }

  if ( SteamAPI_InitSafe_Original () )
  {
    if (1 == InterlockedIncrement (&__SK_Steam_init))
    {
      const wchar_t* steam_dll_str =
        SK_RunLHIfBitness ( 64, L"steam_api64.dll",
                                L"steam_api.dll"    );

      HMODULE hSteamAPI = SK_Modules.LoadLibraryLL (steam_dll_str);

      SK_SteamAPI_ContextInit (hSteamAPI);
      if (SK_GetFramesDrawn () > 1)
      SK_ApplyQueuedHooks     (         );

      steam_log.Log ( L"--- Initialization Finished (%d tries [AppId: %lu]) ---\n\n",
                        init_tries + 1,
                          SK::SteamAPI::AppID () );

      SK_Steam_StartPump (config.steam.force_load_steamapi);
    }

    steam_init_cs->unlock ();
    return true;
  }

  steam_init_cs->unlock ();
  return false;
}

void
S_CALLTYPE
SteamAPI_Shutdown_Detour (void)
{
  steam_log.Log (L" *** Game called SteamAPI_Shutdown (...)");

  steam_init_cs->lock   ();
  SK_Steam_KillPump     ();
  steam_init_cs->unlock ();


  if (! ReadAcquire (&__SK_DLL_Ending))
  {
    SK_Thread_Create (
      [](LPVOID) ->
      DWORD
      {
        SetCurrentThreadDescription (L"[SK] SteamAPI Restart Thread");

        for (int i = 0; i < 1500; i++)
        {
          SleepEx (1L, FALSE);

          if (ReadAcquire (&__SK_DLL_Ending))
          {
            steam_ctx.Shutdown  ();

            SK_Thread_CloseSelf ();

            return 0;
          }
        }

        // Start back up again :)
        //
        //  >> Stupid hack for The Witcher 3
        //
        if (SteamAPI_RunCallbacks_Original != nullptr)
        {
          InterlockedExchangePointer ((void **)&hSteamPump, SK_TLS_Bottom ()->debug.handle);

          while (! ReadAcquire (&__SK_DLL_Ending))
          {
            SK::SteamAPI::Pump ();

            MsgWaitForMultipleObjectsEx ( 0, nullptr, 125, QS_ALLEVENTS, MWMO_INPUTAVAILABLE );
          }
        }

        SK_Thread_CloseSelf ();

        return 0;
      }
    );
  };
}

bool
SK_SAFE_SteamAPI_Init (void)
{
  bool bRet = false;
  __try {
    bRet =
     SteamAPI_Init_Original ();
  }

  __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION )  ?
                     EXCEPTION_EXECUTE_HANDLER :
                     EXCEPTION_CONTINUE_SEARCH ) {
  }

  return bRet;
}

bool
S_CALLTYPE
SteamAPI_Init_Detour (void)
{
  bool bRet = false;

  steam_init_cs->lock ();

  // In case we already initialized stuff...
  if (ReadAcquire (&__SK_Steam_init))
  {
    bRet = SK_SAFE_SteamAPI_Init ();
    steam_init_cs->unlock ();
    return bRet;
  }

  static volatile LONG init_tries = -1;

  if (InterlockedIncrement (&init_tries) == 0)
  {
    steam_log.Log ( L"Initializing SteamWorks Backend  << %s >>",
                      SK_GetCallerName ().c_str () );
    steam_log.Log (L"-------------------------------\n");
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
      SetEnvironmentVariableA ( "SteamGameId",
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
        GetModuleHandleW (steam_dll_str);

      SK_SteamAPI_ContextInit (hSteamAPI);
      if (SK_GetFramesDrawn () > 1)
      SK_ApplyQueuedHooks     (         );

      steam_log.Log ( L"--- Initialization Finished (%d tries [AppId: %lu]) ---\n\n",
                        ReadAcquire (&init_tries) + 1,
                          SK::SteamAPI::AppID () );

      SK_Steam_StartPump (config.steam.force_load_steamapi);
    }
  }

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
  steam_log.init (L"logs/steam_api.log", L"wt+,ccs=UTF-8");
  steam_log.silent = config.steam.silent;

  SK_ICommandProcessor* cmd =
    SK_GetCommandProcessor ();

  cmd->AddVariable ("Steam.TakeScreenshot", SK_CreateVar (SK_IVariable::Boolean, (bool *)&config.steam.achievements.take_screenshot));
  cmd->AddVariable ("Steam.ShowPopup",      SK_CreateVar (SK_IVariable::Boolean, (bool *)&config.steam.achievements.popup.show));
  cmd->AddVariable ("Steam.PopupDuration",  SK_CreateVar (SK_IVariable::Int,     (int  *)&config.steam.achievements.popup.duration));
  cmd->AddVariable ("Steam.PopupInset",     SK_CreateVar (SK_IVariable::Float,   (float*)&config.steam.achievements.popup.inset));
  cmd->AddVariable ("Steam.ShowPopupTitle", SK_CreateVar (SK_IVariable::Boolean, (bool *)&config.steam.achievements.popup.show_title));
  cmd->AddVariable ("Steam.PopupAnimate",   SK_CreateVar (SK_IVariable::Boolean, (bool *)&config.steam.achievements.popup.animate));
  cmd->AddVariable ("Steam.PlaySound",      SK_CreateVar (SK_IVariable::Boolean, (bool *)&config.steam.achievements.play_sound));

  steam_ctx.popup_origin = SK_CreateVar (SK_IVariable::String, steam_ctx.var_strings.popup_origin, &steam_ctx);
  cmd->AddVariable ("Steam.PopupOrigin",    steam_ctx.popup_origin);

  steam_ctx.notify_corner = SK_CreateVar (SK_IVariable::String, steam_ctx.var_strings.notify_corner, &steam_ctx);
  cmd->AddVariable ("Steam.NotifyCorner",   steam_ctx.notify_corner);

  steam_ctx.tbf_pirate_fun = SK_CreateVar (SK_IVariable::Float, &steam_ctx.tbf_float, &steam_ctx);
}

DWORD
WINAPI
SteamAPI_Delay_Init (LPVOID)
{
  SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_HIGHEST  );
  SetCurrentThreadDescription (            L"[SK] SteamAPI Delayed Init. Thread" );
  SetThreadPriorityBoost      ( SK_GetCurrentThread (), TRUE                     );


  //if (! SK_IsInjected ())
  //{
  //  SK_Thread_CloseSelf ();
  //
  //  return 0;
  //}

  int tries = 0;

  while ( (! ReadAcquire (&__SK_Steam_init)) &&
            tries < 120 )
  {
    SleepEx (std::max (2, config.steam.init_delay), FALSE);

    if (SK_GetFramesDrawn () < 1)
      continue;

    if (ReadAcquire (&__SK_Steam_init))
    {
      break;
    }

    if (SteamAPI_Init_Original != nullptr)
        SteamAPI_Init_Detour ();

    ++tries;
  }

  SK_Thread_CloseSelf ();

  return 0;
}

std::wstring
SK_RecursiveFileSearch ( const wchar_t* wszDir,
                         const wchar_t* wszFile );

#include <unordered_set>

enum SK_File_SearchStopCondition {
  FirstMatchFound,
  AllMatchesFound,
  LastMatchFound
};

std::unordered_set <std::wstring>
SK_RecursiveFileSearchEx ( const wchar_t* wszDir,
                           const wchar_t* wszExtension,
       std::unordered_set <std::wstring>& cwsFileNames,
       std::vector        <
         std::pair          < std::wstring, bool >
                          >&&             preferred_dirs = { },
              SK_File_SearchStopCondition stop_condition = FirstMatchFound );

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

  if (! GetModuleHandle (wszSteamAPI))
    return hooks;

  if (! InterlockedCompareExchange (&__SteamAPI_hook, TRUE, FALSE))
  {
    steam_log.Log ( L"%s was loaded, hooking...",
      SK_StripUserNameFromPathW ( std::wstring (wszSteamAPI).data () )
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

    std::unordered_set <std::wstring> matches;
    std::unordered_set <std::wstring> pattern = {
#ifdef _WIN64
      L"steam_api64",
#else
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
    //dll_log.Log (L"Found SteamAPI DLL: %s", (matches.begin ())->c_str ());

      SK_Thread_Create (SteamAPI_Delay_Init);
    }

    InterlockedIncrement (&__SteamAPI_hook);
  }

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
        GetProcAddress (hSteamAPI, "SteamAPI_GetSteamInstallPath");
  }

  if (! steam_ctx.UserStats ())
  {
    steam_ctx.InitSteamAPI (hSteamAPI);

    bool
    SK_Steam_HookController (void);
    SK_Steam_HookController ();
  }
}

void
SK_SteamAPI_InitManagers (void)
{
  if (! InterlockedCompareExchange (&SK_SteamAPI_ManagersInitialized, 1, 0))
  {
    if (steam_ctx.Screenshots () && (! screenshot_manager))
      screenshot_manager = new SK_Steam_ScreenshotManager ();

    ISteamUserStats* stats =
      steam_ctx.UserStats ();

    if (stats != nullptr && ((! user_manager) || (! steam_achievements)))
    {
      //for ( auto it : UserStatsReceived_callbacks )
      //{
      //  if (it.second)
      //    SteamAPI_UnregisterCallback_Original ((class CCallbackBase *)it.first);
      //}

      has_global_data = false;
      next_friend     = 0;

      if (stats->GetNumAchievements ())
      {
        steam_log.Log (L" Creating Achievement Manager...");

        steam_achievements = new SK_Steam_AchievementManager (
          config.steam.achievements.sound_file.c_str     ()
        );

        steam_achievements->requestStats ();
      }

      steam_log.LogEx (false, L"\n");

      user_manager         = new SK_Steam_UserManager    ();
    }

    if (steam_ctx.Utils () && (! overlay_manager))
      overlay_manager      = new SK_Steam_OverlayManager ();

    // Failed, try again later...
    if (overlay_manager == nullptr || steam_achievements == nullptr)
      InterlockedExchange (&SK_SteamAPI_ManagersInitialized, 0);
  }
}

void
SK_SteamAPI_DestroyManagers (void)
{
  if (InterlockedCompareExchange (&SK_SteamAPI_ManagersInitialized, 0, 1))
  {
    if (steam_achievements != nullptr)
    {
      delete steam_achievements;
             steam_achievements = nullptr;
    }

    if (overlay_manager != nullptr)
    {
      delete overlay_manager;
             overlay_manager = nullptr;
    }

    if (user_manager != nullptr)
    {
      delete user_manager;
             user_manager = nullptr;
    }

    if (screenshot_manager != nullptr)
    {
      delete screenshot_manager;
             screenshot_manager = nullptr;
    }
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

  for ( auto it : achievements )
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

  for ( auto it : achievements )
  {
    if (! it->unlocked_)
      locked_achievements.push_back (it);
  }

  return locked_achievements;
}

float
SK_SteamAPI_GetUnlockedPercentForFriend (uint32_t friend_idx)
{
  return steam_achievements->getFriendUnlockPercentage (friend_idx);
}

size_t
SK_SteamAPI_GetUnlockedAchievementsForFriend (uint32_t friend_idx, BOOL* pStats)
{
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
  size_t num_achvs = SK_SteamAPI_GetNumPossibleAchievements ();
  size_t locked  = 0;

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
  std::vector <BOOL> friend_stats;
  friend_stats.resize (SK_SteamAPI_GetNumPossibleAchievements ());

//  size_t unlocked =
    SK_SteamAPI_GetUnlockedAchievementsForFriend (friend_idx, friend_stats.data ());

  size_t shared = 0;

  std::vector <SK_SteamAchievement *>&
    unlocked_achvs = SK_SteamAPI_GetUnlockedAchievements ();

  if (pStats != nullptr)
    SecureZeroMemory (pStats, SK_SteamAPI_GetNumPossibleAchievements ());

  for ( auto it : unlocked_achvs )
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

  return static_cast <float> (next_friend) /
         static_cast <float> (friend_count);
}

const char*
__stdcall
SK_SteamAPI_GetFriendName (uint32_t idx, size_t* pLen)
{
  const std::string& name =
    steam_achievements->getFriendName (idx);

  if (pLen != nullptr)
    *pLen = name.length ();

  // Not the most sensible place to implement this, but ... whatever
  return name.c_str ();
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

  return steam_imported || GetModuleHandle (steam_dll_str)      ||
                           GetModuleHandle (L"CSteamworks.dll") ||
                           GetModuleHandle (L"SteamNative.dll");
}

bool
SK_Steam_TestImports (HMODULE hMod)
{
  const wchar_t* steam_dll_str =
    SK_Steam_GetDLLPath ();

  if (*steam_dll_str == L'\0' || StrStrIW (steam_dll_str, L"SpecialK"))
    config.steam.force_load_steamapi = true;

  sk_import_test_s steam_api [] = { { SK_RunLHIfBitness ( 64, "steam_api64.dll",
                                                              "steam_api.dll" ), false } };

  SK_TestImports (hMod, steam_api, sizeof (steam_api) / sizeof sk_import_test_s);

  if (steam_api->used)
  {
    SK_HookSteamAPI ();
    steam_imported = true;
  }

  else
  {
    if (GetModuleHandle (L"CSteamworks.dll"))
    {
      SK_HookCSteamworks ();
      steam_imported = true;
    }

    else if (! SK_IsInjected ())
    {
      if ( SK_Modules.LoadLibrary (steam_dll_str) ||
                  GetModuleHandle (L"SteamNative.dll") )
      {
         SK_HookSteamAPI ();
         steam_imported = true;
      }
    }
  }

  if ( SK_Modules.LoadLibrary (steam_dll_str) ||
              GetModuleHandle (L"SteamNative.dll") )
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
SK_SteamAPI_AddScreenshotToLibrary (const char *pchFilename, const char *pchThumbnailFilename, int nWidth, int nHeight)
{
  if (steam_ctx.Screenshots ())
    steam_ctx.Screenshots ()->AddScreenshotToLibrary (pchFilename, pchThumbnailFilename, nWidth, nHeight);
}

ScreenshotHandle
WINAPI
SK_SteamAPI_AddScreenshotToLibraryEx (const char *pchFilename, const char *pchThumbnailFilename, int nWidth, int nHeight, bool Wait = false)
{
  if (steam_ctx.Screenshots ())
  {
    ScreenshotHandle handle =
      steam_ctx.Screenshots ()->AddScreenshotToLibrary (pchFilename, pchThumbnailFilename, nWidth, nHeight);

    if (Wait)
    {
      screenshot_manager->WaitOnScreenshot ( handle );
    }

    return handle;
  }

  return 0;
}

void
WINAPI
SK_SteamAPI_WriteScreenshot (void *pubRGB, uint32 cubRGB, int nWidth, int nHeight)
{
  if (steam_ctx.Screenshots ())
    steam_ctx.Screenshots ()->WriteScreenshot (pubRGB, cubRGB, nWidth, nHeight);
}

HMODULE hModOverlay;

bool
SK_Steam_LoadOverlayEarly (void)
{
  const wchar_t* wszSteamPath =
      SK_GetSteamDir ();

  if (wszSteamPath == nullptr)
    return false;

  wchar_t    wszOverlayDLL [MAX_PATH + 2] = { };
  lstrcatW ( wszOverlayDLL, wszSteamPath );
  lstrcatW ( wszOverlayDLL, SK_RunLHIfBitness ( 64, LR"(\GameOverlayRenderer64.dll)",
                                                    LR"(\GameOverlayRenderer.dll)"    ) );

  hModOverlay =
    SK_Modules.LoadLibrary (wszOverlayDLL);

  return hModOverlay != nullptr;
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

  if (__SK_SteamUser_BLoggedOn == static_cast <int> (SK_SteamUser_LoggedOn_e::Unknown))
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
//  Please refrain from simply binary-editing one of my distributions, if you have the skill to
//    circumvent DRM it should be obvious that I intend every one of my measures to be plainly
//      visible and should take you thirty-seconds to figure out and remove the correct way.
//
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
//
//  The feature that deletes files is for the benefit of people who "upgrade" from pirated to
//  legit and swear up and down they are continuing to be detected as a pirate. Don't click that
//  button unless you are a legitimate user who upgraded and has remnant illegal files lingering.
//
//
//-----------------------------------------------------------------------------------------------
//    Remove my name from any forked work, and stop making your community look like a bunch of
//      children; it reflects poorly on the whole of PC gaming.
//-----------------------------------------------------------------------------------------------
//
static uint32_t        verdict        = 0x00;
static bool            decided        = false;

alignas (_M128A)
static volatile SteamAPICall_t hAsyncSigCheck = 0; // File Signature Validation
static          std::string    check_file     = "";

enum class SK_Steam_FileSigPass_e
{
  Executable = 0,
  SteamAPI   = 1,
  DLC        = 2,

  Done       = MAXINT
};

static SK_Steam_FileSigPass_e
                       validation_pass (SK_Steam_FileSigPass_e::Executable);

uint32_t
__stdcall
SK_Steam_PiratesAhoy (void)
{
  //   Older versions of SteamAPI may not support the necessary interface version
  if ( steam_ctx.Apps () != nullptr &&
       validation_pass   != SK_Steam_FileSigPass_e::Done  ) 
  {
    DepotId_t depots [16] = { };

    steam_ctx.Apps ()->RequestAllProofOfPurchaseKeys ();
    steam_ctx.Apps ()->GetInstalledDepots (steam_ctx.Utils ()->GetAppID (), depots, 16);

    if ( InterlockedCompareExchange (&hAsyncSigCheck, 1, 0) == 0 &&
         (! config.steam.silent) )
    {
      switch (validation_pass)
      {
        case SK_Steam_FileSigPass_e::Executable:
        {
          check_file =
            SK_WideCharToUTF8 (SK_GetFullyQualifiedApp ());

          InterlockedExchange (&hAsyncSigCheck,
            steam_ctx.Apps ()->GetFileDetails (check_file.c_str ()));
        } break;

        case SK_Steam_FileSigPass_e::SteamAPI:
        {
          char szRelSteamAPI [MAX_PATH * 2] = { };

          // Generally, though not always (i.e. CSteamWorks) we will link against one of these two DLLs
          //
          //   The actual DLL used is pulled from the IAT during init, but I am too lazy to bother doing
          //     this the right way ;)
          snprintf ( szRelSteamAPI, MAX_PATH * 2 - 1, 
                       "%ws", SK_Steam_GetDLLPath () );

          check_file =
            szRelSteamAPI;

          // Handle the case where we manually injected steam_api, because if we do not, tier0_s.dll will
          //   kill the client pipe.
          if (! check_file.empty ())
          {
            InterlockedExchange64 ((volatile LONG64 *)&hAsyncSigCheck,
              steam_ctx.Apps ()->GetFileDetails (check_file.c_str ()));
          }
        } break;
      }

      if (! check_file.empty ())
      {
        steam_ctx.get_file_details.Set (
          hAsyncSigCheck,
            &steam_ctx,
              &SK_SteamAPIContext::OnFileDetailsDone );
      }
    }
  }

  if (decided)
  {
    return verdict;
  }

  static uint32_t crc32_steamapi =
    SK_File_GetCRC32C (SK_Steam_GetDLLPath ());

  if (SK::SteamAPI::steam_size > 0 && SK::SteamAPI::steam_size < (1024 * 92))
  {
    verdict = 0x68992;
  }

  // CPY
  if (steam_ctx.User () != nullptr)
  {
    if ( steam_ctx.User ()->GetSteamID ().ConvertToUint64 () == 18295873490452480 << 4 ||
         steam_ctx.User ()->GetSteamID ().ConvertToUint64 () == 25520399320093309  * 3 )
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

#include <concurrent_priority_queue.h>

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
    bFailed ? k_EResultFileNotFound :
              pParam->m_eResult;


  static hash_job_prio_queue_s
    waiting_hash_jobs;
    waiting_hash_jobs.push ( *pParam );


  if ( InterlockedCompareExchangePointer ( &hSigNewSteamFileDetails, nullptr, INVALID_HANDLE_VALUE) ==
                                                                              INVALID_HANDLE_VALUE )
  {
    InterlockedExchangePointer ( (void **)&hSigNewSteamFileDetails,
                                   CreateEventW ( nullptr, FALSE, true, nullptr ) );

  //
  // Hashing Denuvo games can take a very long time, and we do not need
  //   the result immediately... so do not destroy the game's performance
  //     by blocking at startup!
  //
  SK_Thread_Create ([](LPVOID user) ->
    DWORD
    {
      SetCurrentThreadDescription (L"[SK] Steam File Validation Thread");


      hash_job_prio_queue_s* prio_queue =
        (hash_job_prio_queue_s *)user;


      while ( MsgWaitForMultipleObjectsEx ( 1, &hSigNewSteamFileDetails,
                                              666UL, QS_ALLEVENTS, MWMO_INPUTAVAILABLE
                                          )                == WAIT_OBJECT_0
            )
      {

        if ( ReadAcquire (&__SK_DLL_Ending) ||
               validation_pass == SK_Steam_FileSigPass_e::Done
        ) break;


        FileDetailsResult_t result;
        {
          while ( (! prio_queue->empty   (        ) ) &&
                  (! prio_queue->try_pop ( result ) ) )
          {
            MsgWaitForMultipleObjectsEx ( 1, &hSigNewSteamFileDetails,
                                            133UL, QS_ALLEVENTS, MWMO_INPUTAVAILABLE );
          }
        }
        FileDetailsResult_t* pParam = &result;


        auto HandleResult =
         [&]( const wchar_t* wszFileName )
          {
            SK_SHA1_Hash SHA1;
                 memcpy (SHA1.hash, pParam->m_FileSHA, 20);

            switch (pParam->m_eResult)
            {
              case k_EResultOK:
              {    
                uint64_t size =
                  SK_File_GetSize (wszFileName);
    
                SK_SHA1_Hash file_hash =
                  SK_File_GetSHA1 (wszFileName);
    
                char            szSHA1 [21] = { };
                SHA1.toCString (szSHA1);
    
                if (size == pParam->m_ulFileSize && file_hash == SHA1)
                {
                  steam_log.Log ( L"> SteamAPI Application File Verification:  "
                                  L" Match  ( File: %ws,\n"
    L"                                                                              SHA1: %20hs,\n"
    L"                                                                              Size: %lu bytes )",
                                  SK_StripUserNameFromPathW (std::wstring (wszFileName).data ()),
                                    szSHA1, pParam->m_ulFileSize );
                }
    
                else if (size != 0)
                {
                  if (size != pParam->m_ulFileSize)
                  {
                    steam_log.Log ( L"> SteamAPI SteamAPI File Verification:     "
                                    L" Size Mismatch ( File: %ws,\n"
    L"                                                                              Expected SHA1: %20hs,\n"
    L"                                                                              Expected Size: %lu bytes,\n"
    L"                                                                                Actual Size: %lu bytes )",
                                    SK_StripUserNameFromPathW (std::wstring (wszFileName).data ()),
                                      szSHA1,
                                       pParam->m_ulFileSize, size );
    
                    dll_log.Log ( L"> SteamAPI SteamAPI File Verification:     "
                                  L" Size Mismatch ( File: %ws,\n"
    L"                                                                              Expected SHA1: %20hs,\n"
    L"                                                                              Expected Size: %lu bytes,\n"
    L"                                                                                Actual Size: %lu bytes )",
                                    SK_StripUserNameFromPathW (std::wstring (wszFileName).data ()),
                                      szSHA1,
                                       pParam->m_ulFileSize, size );
                  }
    
                  else if (file_hash != SHA1)
                  {
                    char                 szFileSHA1 [21] = { };
                    file_hash.toCString (szFileSHA1);
    
                    steam_log.Log ( L"> SteamAPI SteamAPI File Verification:     "
                                    L" SHA1 Mismatch ( File: %ws,\n"
    L"                                                                              Expected SHA1: %20hs,\n"
    L"                                                                                Actual SHA1: %20hs,\n"
    L"                                                                                       Size: %lu bytes )",
                                    SK_StripUserNameFromPathW (std::wstring (wszFileName).data ()),
                                      szSHA1, szFileSHA1,
                                       pParam->m_ulFileSize );
    
                    dll_log.Log ( L"> SteamAPI SteamAPI File Verification:     "
                                  L" SHA1 Mismatch ( File: %ws,\n"
    L"                                                                              Expected SHA1: %20hs,\n"
    L"                                                                                Actual SHA1: %20hs,\n"
    L"                                                                                       Size: %lu bytes )",
                                    SK_StripUserNameFromPathW (std::wstring (wszFileName).data ()),
                                      szSHA1, szFileSHA1,
                                       pParam->m_ulFileSize );
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
                steam_log.Log ( L"> SteamAPI File Verification:                "
                                L" UNKNOWN STATUS (%lu) :: '%ws'",
                                  pParam->m_eResult,
                                    SK_StripUserNameFromPathW (std::wstring (wszFileName).data ()) );
              } break;
            }
          };

        // We don't need these results anytime soon, get them when we get them...
        SetThreadPriority      ( GetCurrentThread (), THREAD_MODE_BACKGROUND_BEGIN );
        SetThreadPriority      ( GetCurrentThread (), THREAD_PRIORITY_IDLE         );
        SetThreadPriorityBoost ( GetCurrentThread (), TRUE                         );
        {
          switch (validation_pass)
          {
            case SK_Steam_FileSigPass_e::Executable:
            {
              HandleResult (SK_UTF8ToWideChar (check_file).c_str ());
    
              validation_pass = SK_Steam_FileSigPass_e::SteamAPI;
              InterlockedExchange (&hAsyncSigCheck, 0);
            } break;
    
            case SK_Steam_FileSigPass_e::SteamAPI:
            {
              HandleResult (SK_UTF8ToWideChar (check_file).c_str ());
    
              validation_pass = SK_Steam_FileSigPass_e::Done;
              InterlockedExchange (&hAsyncSigCheck, 0);
            } break;
          }
        }
        SetThreadPriorityBoost ( GetCurrentThread (), FALSE                        );
        SetThreadPriority      ( GetCurrentThread (), THREAD_MODE_BACKGROUND_END   );
        SetThreadPriority      ( GetCurrentThread (), THREAD_PRIORITY_BELOW_NORMAL );
      }


      SK_Thread_CloseSelf ();

      CloseHandle (hSigNewSteamFileDetails);
                   hSigNewSteamFileDetails = INVALID_HANDLE_VALUE;

      return 0;
    }, (void *)&waiting_hash_jobs);
  }

  else SetEvent (hSigNewSteamFileDetails);


  get_file_details.Cancel ();
}


// Fallback to Win32 API if the Steam overlay is not functioning.
#include <SpecialK/window.h>

bool
__stdcall
SK_SteamOverlay_GoToURL (const char* szURL, bool bUseWindowsShellIfOverlayFails)
{
  if (steam_ctx.Utils () != nullptr && steam_ctx.Utils ()->IsOverlayEnabled ())
  {
    if (steam_ctx.Friends () != nullptr)
    {
      steam_ctx.Friends ()->ActivateGameOverlayToWebPage (szURL);
      return true;
    }
  }

  if (bUseWindowsShellIfOverlayFails)
  {
    ShellExecuteA (game_window.hWnd, "open", szURL, nullptr, nullptr, SW_NORMAL);
    return true;
  }

  return false;
}

bool
__stdcall
SK_SteamOverlay_GoToFriendProfile (CSteamID friend_sid)
{
  if (steam_ctx.Utils () != nullptr && steam_ctx.Utils ()->IsOverlayEnabled ())
  {
    if (steam_ctx.Friends () != nullptr)
    {
      steam_ctx.Friends ()->ActivateGameOverlayToUser ("steamid", friend_sid);
      return true;
    }
  }

  return false;
}

bool
__stdcall
SK_SteamOverlay_GoToFriendAchievements (CSteamID friend_sid)
{
  if (steam_ctx.Utils () != nullptr && steam_ctx.Utils ()->IsOverlayEnabled ())
  {
    if (steam_ctx.Friends () != nullptr)
    {
      steam_ctx.Friends ()->ActivateGameOverlayToUser ("achievements", friend_sid);
      return true;
    }
  }

  return false;
}

bool
__stdcall
SK_SteamOverlay_GoToFriendStats (CSteamID friend_sid)
{
  if (steam_ctx.Utils () != nullptr && steam_ctx.Utils ()->IsOverlayEnabled ())
  {
    if (steam_ctx.Friends () != nullptr)
    {
      steam_ctx.Friends ()->ActivateGameOverlayToUser ("stats", friend_sid);
      return true;
    }
  }

  return false;
}



ISteamMusic*
SAFE_GetISteamMusic (ISteamClient* pClient, HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
{
  __try {
    if (SK_IsAddressExecutable ((*(void ***)*&pClient)[24]))
      return pClient->GetISteamMusic (hSteamuser, hSteamPipe, pchVersion);
  }

  __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION )  ?
                       EXCEPTION_EXECUTE_HANDLER :
                       EXCEPTION_CONTINUE_SEARCH ) {
  }

  return nullptr;
}

ISteamController*
SAFE_GetISteamController (ISteamClient* pClient, HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
{
  __try {
    if (SK_IsAddressExecutable ((*(void ***)*&pClient)[21]))
      return pClient->GetISteamController (hSteamuser, hSteamPipe, pchVersion);
  }

  __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION )  ?
                       EXCEPTION_EXECUTE_HANDLER :
                       EXCEPTION_CONTINUE_SEARCH ) {
  }

  return nullptr;
}

ISteamRemoteStorage*
SAFE_GetISteamRemoteStorage (ISteamClient* pClient, HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
{
  __try {
    if (SK_IsAddressExecutable ((*(void ***)*&pClient)[13]))
      return pClient->GetISteamRemoteStorage (hSteamuser, hSteamPipe, pchVersion);
  }
  __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION )  ?
                       EXCEPTION_EXECUTE_HANDLER :
                       EXCEPTION_CONTINUE_SEARCH ) {
  }

  return nullptr;
}

ISteamUGC*
SAFE_GetISteamUGC (ISteamClient* pClient, HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
{
  __try {
    if (SK_IsAddressExecutable ((*(void ***)*&pClient)[23]))
      return pClient->GetISteamUGC (hSteamuser, hSteamPipe, pchVersion);
  }
  __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION )  ?
                       EXCEPTION_EXECUTE_HANDLER :
                       EXCEPTION_CONTINUE_SEARCH ) {
  }

  return nullptr;
}


extern "C" void __cdecl SteamAPIDebugTextHook (int nSeverity, const char *pchDebugText);

extern "C"
void
__cdecl
SK_SteamAPI_DebugText (int nSeverity, const char *pchDebugText)
{
  steam_log.Log ( L" [SteamAPI] Severity: %d - '%hs'",
                    nSeverity, pchDebugText );
}

bool
SK_SteamAPIContext::InitSteamAPI (HMODULE hSteamDLL)
{
  if (SteamAPI_InitSafe == nullptr)
  {
    SteamAPI_InitSafe =
      (SteamAPI_InitSafe_pfn)GetProcAddress (
        hSteamDLL,
          "SteamAPI_InitSafe"
      );
  }

  if (SteamAPI_GetHSteamUser == nullptr)
  {
    SteamAPI_GetHSteamUser =
      (SteamAPI_GetHSteamUser_pfn)GetProcAddress (
         hSteamDLL,
           "SteamAPI_GetHSteamUser"
      );
  }

  if (SteamAPI_GetHSteamPipe == nullptr)
  {
    SteamAPI_GetHSteamPipe =
      (SteamAPI_GetHSteamPipe_pfn)GetProcAddress (
         hSteamDLL,
           "SteamAPI_GetHSteamPipe"
      );
  }

  SteamAPI_IsSteamRunning =
    (SteamAPI_IsSteamRunning_pfn)GetProcAddress (
       hSteamDLL,
         "SteamAPI_IsSteamRunning"
    );

  if (SteamClient_Original == nullptr)
  {
    SteamClient_Original =
      (SteamClient_pfn)GetProcAddress (
         hSteamDLL,
           "SteamClient"
      );
  }

  if (SteamAPI_RegisterCallResult == nullptr)
  {
    SteamAPI_RegisterCallResult =
      (SteamAPI_RegisterCallResult_pfn)GetProcAddress (
        hSteamDLL,
          "SteamAPI_RegisterCallResult"
      );
  }

  if (SteamAPI_UnregisterCallResult == nullptr)
  {
    SteamAPI_UnregisterCallResult =
      (SteamAPI_UnregisterCallResult_pfn)GetProcAddress (
        hSteamDLL,
          "SteamAPI_UnregisterCallResult"
      );
  }


  bool success = true;

  if (SteamAPI_GetHSteamUser == nullptr)
  {
    steam_log.Log (L"Could not load SteamAPI_GetHSteamUser (...)");
    success = false;
  }

  if (SteamAPI_GetHSteamPipe == nullptr)
  {
    steam_log.Log (L"Could not load SteamAPI_GetHSteamPipe (...)");
    success = false;
  }

  if (SteamClient_Original == nullptr)
  {
    steam_log.Log (L"Could not load SteamClient (...)");
    success = false;
  }


  if (SteamAPI_RunCallbacks == nullptr)
  {
    steam_log.Log (L"Could not load SteamAPI_RunCallbacks (...)");
    success = false;
  }

  if (SteamAPI_Shutdown == nullptr)
  {
    steam_log.Log (L"Could not load SteamAPI_Shutdown (...)");
    success = false;
  }

  if (! success)
    return false;

  if (! SteamAPI_InitSafe ())
  {
    steam_log.Log (L" SteamAPI_InitSafe (...) Failed?!");
    return false;
  }

  client_ = SteamClient_Original ();

  if (! client_)
  {
    steam_log.Log (L" SteamClient (...) Failed?!");
    return false;
  } else
    client_->SetWarningMessageHook ( &SK_SteamAPI_DebugText );


  hSteamPipe = SteamAPI_GetHSteamPipe ();
  hSteamUser = SteamAPI_GetHSteamUser ();


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

  for (i = INTERNAL_STEAMUSER_INTERFACE_VERSION+5; i > 0; --i)
  {
    user_ =
      client_->GetISteamUser (
        hSteamUser,
          hSteamPipe,
            SK_FormatString ("SteamUser%03lu", i).c_str ()
      );

    if (user_ != nullptr)
    {
      if (user_ver_ == 0)
      {
        user_ver_ = i;
        steam_log.Log (L"SteamAPI DLL Implements Steam User v%03lu", i);

        if (user_ver_ > INTERNAL_STEAMUSER_INTERFACE_VERSION+1)
          steam_log.Log (L" >> Newer than Special K knows about.");
      }

      if (i == INTERNAL_STEAMUSER_INTERFACE_VERSION)
        break;
    }
  }

  if (i != INTERNAL_STEAMUSER_INTERFACE_VERSION)
  {
    steam_log.Log ( L" >> ISteamUser NOT FOUND for version %hs <<",
                      STEAMUSER_INTERFACE_VERSION );
    user_ = nullptr;
    return false;
  }

  user_ex_ =
    (ISteamUser004_Light *)client_->GetISteamUser (
      hSteamUser,
        hSteamPipe,
          "SteamUser004" );

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
      hSteamUser,
        hSteamPipe,
          STEAMFRIENDS_INTERFACE_VERSION
    );

  if (friends_ == nullptr)
  {
    steam_log.Log ( L" >> ISteamFriends NOT FOUND for version %hs <<",
                      STEAMFRIENDS_INTERFACE_VERSION );
    return false;
  }


  user_stats_ =
    client_->GetISteamUserStats (
      hSteamUser,
        hSteamPipe,
          STEAMUSERSTATS_INTERFACE_VERSION
    );

  if (user_stats_ == nullptr)
  {
    steam_log.Log ( L" >> ISteamUserStats NOT FOUND for version %hs <<",
                      STEAMUSERSTATS_INTERFACE_VERSION );
    return false;
  }

  apps_ =
    client_->GetISteamApps (
      hSteamUser,
        hSteamPipe,
          STEAMAPPS_INTERFACE_VERSION
    );

  if (apps_ == nullptr)
  {
    steam_log.Log ( L" >> ISteamApps NOT FOUND for version %hs <<",
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
        steam_log.Log (L"SteamAPI DLL Implements Utils v%03lu", i);

        if (utils_ver_ > NEWEST_STEAMUTILS_INTERFACE_VERSION)
          steam_log.Log (L" >> Newer than Special K knows about.");
      }

      if (i == INTERNAL_STEAMUTILS_INTERFACE_VERSION)
        break;
    }
  }

  if (i != INTERNAL_STEAMUTILS_INTERFACE_VERSION)
  {
    steam_log.Log ( L" >> ISteamUtils NOT FOUND for version %hs <<",
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
    steam_log.Log ( L" >> ISteamScreenshots NOT FOUND for version %hs <<",
                      STEAMSCREENSHOTS_INTERFACE_VERSION );

    screenshots_ =
      client_->GetISteamScreenshots ( hSteamUser,
                                        hSteamPipe,
                                          "STEAMSCREENSHOTS_INTERFACE_VERSION001" );

    steam_log.Log ( L" >> ISteamScreenshots NOT FOUND for version %hs <<",
                      "STEAMSCREENSHOTS_INTERFACE_VERSION001" );
  }

  // This crashes Dark Souls 3, so... do this
  music_ =
    SAFE_GetISteamMusic ( client_,
                            hSteamUser,
                              hSteamPipe,
                                STEAMMUSIC_INTERFACE_VERSION );

  SK::SteamAPI::player = user_->GetSteamID ();

  controller_ =
    SAFE_GetISteamController ( client_,
                                 hSteamUser,
                                   hSteamPipe,
                                     "SteamController005" );

  for (i = INTERNAL_STEAMREMOTESTORAGE_INTERFACE_VERSION+2; i > 0; --i)
  {
    remote_storage_ =
      SAFE_GetISteamRemoteStorage (
        client_,
          hSteamUser,
            hSteamPipe,
              SK_FormatString ("STEAMREMOTESTORAGE_INTERFACE_VERSION%03u", i).c_str () );

    if (remote_storage_ != nullptr)
    {
      if (remote_storage_ver_ == 0)
      {
        remote_storage_ver_ = i;
        steam_log.Log (L"SteamAPI DLL Implements Remote Storage v%03lu", i);

        if (remote_storage_ver_ > 14)
          steam_log.Log (L" >> Newer than Special K knows about.");
      }

      if (i == INTERNAL_STEAMREMOTESTORAGE_INTERFACE_VERSION)
        break;
    }
  }

  if (i != INTERNAL_STEAMREMOTESTORAGE_INTERFACE_VERSION) remote_storage_ = nullptr;

  for (i = INTERNAL_STEAMUGC_INTERFACE_VERSION+1; i > 0; --i)
  {
    ugc_ =
      SAFE_GetISteamUGC (
        client_,
          hSteamUser,
            hSteamPipe,
              SK_FormatString ("STEAMUGC_INTERFACE_VERSION%03u", i).c_str () );

    if (ugc_ != nullptr)
    {
      if (ugc_ver_ == 0)
      {
        ugc_ver_ = i;
        steam_log.Log (L"SteamAPI DLL Implements UGC v%03lu", i);

        if (ugc_ver_ > INTERNAL_STEAMUGC_INTERFACE_VERSION)
          steam_log.Log (L" >> Newer than Special K knows about.");
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
  return (! overlay_activation_callbacks.empty ());
}

BOOL
SK_Steam_KickStart (const wchar_t* wszLibPath)
{
  UNREFERENCED_PARAMETER (wszLibPath);

  static volatile LONG tried = FALSE;

  if (! InterlockedCompareExchange (&tried, TRUE, FALSE))
  {
    static const wchar_t* wszSteamDLL =
      SK_RunLHIfBitness ( 64, LR"(PlugIns\ThirdParty\Steamworks\steam_api64.dll)",
                              LR"(PlugIns\ThirdParty\Steamworks\steam_api.dll)" );

    if (! GetModuleHandle (wszSteamDLL))
    {
      wchar_t wszDLLPath [MAX_PATH * 2 + 4] = { };

      if (SK_IsInjected ())
        wcsncpy_s (wszDLLPath, MAX_PATH * 2, SK_GetModuleFullName (SK_GetDLL ()).c_str (), _TRUNCATE);
      else
      {
        _swprintf ( wszDLLPath, LR"(%s\My Mods\SpecialK\SpecialK.dll)",
                      SK_GetDocumentsDir ().c_str () );
      }

      if (PathRemoveFileSpec (wszDLLPath))
      {
        PathAppendW (wszDLLPath, wszSteamDLL);

        if (SK_File_GetSize (wszDLLPath) > 0)
        {
          if (SK_Modules.LoadLibraryLL (wszDLLPath))
          {
            dll_log.Log ( L"[DLL Loader]   Manually booted SteamAPI: '%s'",
                            SK_StripUserNameFromPathW (std::wstring (wszDLLPath).data ()) );

            config.steam.dll_path            = wszDLLPath;
            config.steam.force_load_steamapi = true;

            SK_HookSteamAPI ();

            return true;
          }
        }
      }
    }
  }

  return false;
}


#define CLIENTENGINE_INTERFACE_VERSION  "CLIENTENGINE_INTERFACE_VERSION005"
#define CLIENTUSER_INTERFACE_VERSION    "CLIENTUSER_INTERFACE_VERSION001"
#define CLIENTFRIENDS_INTERFACE_VERSION "CLIENTFRIENDS_INTERFACE_VERSION001"


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

class IClientUser_001_Ext : IClientUser_001_Min
{
public:
  virtual CSteamID    GetSteamID                            (void)                                                               = 0;
  virtual CSteamID    GetConsoleSteamID                     (void)                                                               = 0;

  virtual bool IsVACBanned( AppId_t nGameID ) = 0;
  virtual bool RequireShowVACBannedMessage( AppId_t nAppID ) = 0;
  virtual void AcknowledgeVACBanning( AppId_t nAppID ) = 0;

  virtual bool SetEmail( const char *pchEmail ) = 0;

  virtual bool SetConfigString( DWORD eRegistrySubTree, const char *pchKey, const char *pchValue ) = 0;
  virtual bool GetConfigString( DWORD eRegistrySubTree, const char *pchKey, char *pchValue, int32 cbValue ) = 0;
  virtual bool SetConfigInt( DWORD eRegistrySubTree, const char *pchKey, int32 iValue ) = 0;
  virtual bool GetConfigInt( DWORD eRegistrySubTree, const char *pchKey, int32 *pValue ) = 0;

  virtual bool GetConfigStoreKeyName( DWORD eRegistrySubTree, const char *pchKey, char *pchStoreName, int32 cbStoreName ) = 0;

  virtual int32 InitiateGameConnection( void *pOutputBlob, int32 cbBlobMax, CSteamID steamIDGS, CGameID gameID, uint32 unIPServer, uint16 usPortServer, bool bSecure ) = 0;
  virtual int32 InitiateGameConnectionOld( void *pOutputBlob, int32 cbBlobMax, CSteamID steamIDGS, CGameID gameID, uint32 unIPServer, uint16 usPortServer, bool bSecure, void *pvSteam2GetEncryptionKey, int32 cbSteam2GetEncryptionKey ) = 0;

  virtual void TerminateGameConnection( uint32 unIPServer, uint16 usPortServer ) = 0;
  virtual bool TerminateAppMultiStep( uint32, uint32 ) = 0;

  virtual void SetSelfAsPrimaryChatDestination() = 0;
  virtual bool IsPrimaryChatDestination() = 0;

  virtual void RequestLegacyCDKey( AppId_t iAppID ) = 0;

  virtual bool AckGuestPass( const char *pchGuestPassCode ) = 0;
  virtual bool RedeemGuestPass( const char *pchGuestPassCode ) = 0;

  virtual uint32 GetGuestPassToGiveCount() = 0;
  virtual uint32 GetGuestPassToRedeemCount() = 0;

  virtual bool GetGuestPassToGiveInfo( uint32 nPassIndex, GID_t *pgidGuestPassID, PackageId_t* pnPackageID, RTime32* pRTime32Created, RTime32* pRTime32Expiration, RTime32* pRTime32Sent, RTime32* pRTime32Redeemed, char* pchRecipientAddress, int32 cRecipientAddressSize ) = 0;
  virtual bool GetGuestPassToRedeemInfo( uint32 nPassIndex, GID_t *pgidGuestPassID, PackageId_t* pnPackageID, RTime32* pRTime32Created, RTime32* pRTime32Expiration, RTime32* pRTime32Sent, RTime32* pRTime32Redeemed ) = 0;
  virtual bool GetGuestPassToRedeemSenderName( uint32 nPassIndex, char* pchSenderName, int32 cSenderNameSize ) = 0;

  virtual void AcknowledgeMessageByGID( const char *pchMessageGID ) = 0;

  virtual bool SetLanguage( const char *pchLanguage ) = 0;

  virtual void TrackAppUsageEvent( CGameID gameID, int32 eAppUsageEvent, const char *pchExtraInfo = "" ) = 0;

  virtual int32 RaiseConnectionPriority( DWORD eConnectionPriority ) = 0;
  virtual void ResetConnectionPriority( int32 hRaiseConnectionPriorityPrev ) = 0;

  virtual void SetAccountNameFromSteam2( const char *pchAccountName ) = 0;
  virtual bool SetPasswordFromSteam2( const char *pchPassword ) = 0;

  virtual bool BHasCachedCredentials( const char * pchUnk ) = 0;
  virtual bool SetAccountNameForCachedCredentialLogin( const char *pchAccountName, bool bUnk ) = 0;
  virtual void SetLoginInformation( const char *pchAccountName, const char *pchPassword, bool bRememberPassword ) = 0;
  virtual void ClearAllLoginInformation() = 0;

  virtual void SetAccountCreationTime( RTime32 rtime32Time ) = 0;

  virtual SteamAPICall_t RequestWebAuthToken() = 0;
  virtual bool GetCurrentWebAuthToken( char *pchBuffer, int32 cubBuffer ) = 0;

  virtual bool GetLanguage( char* pchLanguage, int32 cbLanguage ) = 0;

  virtual bool BIsCyberCafe() = 0;
  virtual bool BIsAcademicAccount() = 0;

  virtual void CreateAccount( const char *pchAccountName, const char *pchNewPassword, const char *pchNewEmail, int32 iQuestion, const char *pchNewQuestion, const char *pchNewAnswer ) = 0;

  virtual SteamAPICall_t ResetPassword( const char *pchAccountName, const char *pchOldPassword, const char *pchNewPassword, const char *pchValidationCode, const char *pchAnswer ) = 0;

  virtual void TrackNatTraversalStat( const LPVOID pNatStat ) = 0;

  virtual void TrackSteamUsageEvent( DWORD eSteamUsageEvent, const uint8 *pubKV, uint32 cubKV ) = 0;
  virtual void TrackSteamGUIUsage( const char * ) = 0;

  virtual void SetComputerInUse() = 0;

  virtual bool BIsGameRunning( CGameID gameID ) = 0;


  virtual uint64 GetCurrentSessionToken() = 0;

  virtual bool BUpdateAppOwnershipTicket( AppId_t nAppID, bool bOnlyUpdateIfStale, bool bIsDepot ) = 0;

  virtual bool RequestCustomBinary( const char *pszAbsolutePath, AppId_t nAppID, bool bForceUpdate, bool bAppLaunchRequest ) = 0;
  virtual EResult GetCustomBinariesState( AppId_t unAppID, uint32 *punProgress ) = 0;
  virtual EResult RequestCustomBinaries( AppId_t unAppID, bool, bool, uint32 * ) = 0;

  virtual void SetCellID( CellID_t cellID ) = 0;
  virtual void SetWinningPingTimeForCellID( uint32 uPing ) = 0;

  virtual const char *GetUserBaseFolder() = 0;

  virtual bool GetUserDataFolder( CGameID gameID, char* pchBuffer, int32 cubBuffer ) = 0;
  virtual bool GetUserConfigFolder( char *pchBuffer, int32 cubBuffer ) = 0;
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

  // LOTS MORE DONT CARE...
};


void*
SK_SteamAPIContext::ClientEngine (void)
{
  using  CreateInterface_pfn = void* (__cdecl *)(const char *ver, int* pError);
  static CreateInterface_pfn SteamClient_CreateInterface = nullptr;

#ifdef _WIN64
  SteamClient_CreateInterface =
    (CreateInterface_pfn)
      GetProcAddress ( GetModuleHandle (L"steamclient64.dll"),
                                         "CreateInterface" );
#else
  SteamClient_CreateInterface =
    (CreateInterface_pfn)
      GetProcAddress ( GetModuleHandle (L"steamclient.dll"),
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
    SteamAPI_RunCallbacks_Original  (     );
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


//
// Steam may not inject the Steam overlay in some cases without this,
//   it may also pull the steamclient{64}.dll rug out from under our feet
//     at startup without it.
//
bool
SK_Steam_ConnectUserIfNeeded (CSteamID user)
{
  bool bRet = false;

  if (steam_ctx.User () != nullptr && user == steam_ctx.User ()->GetSteamID ())
  {
    SK::SteamAPI::ISteamUser004_Light*
      user_ex =
        steam_ctx.UserEx ();

    if (user_ex != nullptr)
    {
      if (! user_ex->BConnected ())
      {
        user_ex->LogOn                 (user);
        SteamAPI_RunCallbacks_Original (    );

        bRet = true;
      }
    }
  }

  if (config.steam.online_status != -1)
    SK::SteamAPI::SetPersonaState ((EPersonaState)config.steam.online_status);

  return bRet;
}

bool
SK_SteamAPIContext::ReleaseThreadPipe (void)
{
  HSteamPipe& pipe = SK_TLS_Bottom ()->steam.client_pipe;

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
  SK_TLS* pTLS =SK_TLS_Bottom ();

  HSteamUser& user = pTLS->steam.client_user;
  HSteamPipe& pipe = pTLS->steam.client_pipe;

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
  if (steam_ctx.ClientUser () != nullptr)
  {
    char szDir [MAX_PATH * 2] = { };

    if (((IClientUser_001_Ext *)steam_ctx.ClientUser ())->GetUserConfigFolder (szDir, MAX_PATH))
      return szDir;
  }

  return
    std::string ();
}

std::string
SK::SteamAPI::GetDataDir (void)
{
  if (steam_ctx.ClientUser () != nullptr)
  {
    char szDir [MAX_PATH * 2] = { };

    if (((IClientUser_001_Ext *)steam_ctx.ClientUser ())->GetUserDataFolder (CGameID (SK::SteamAPI::AppID ()), szDir, MAX_PATH))
      return szDir;
  }

  return
    std::string ();
}



uint64_t    SK::SteamAPI::steam_size                                    = 0ULL;

// Must be global for x86 ABI problems
CSteamID    SK::SteamAPI::player (0ULL);