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

#define _CRT_NON_CONFORMING_SWPRINTFS
#define _CRT_SECURE_NO_WARNINGS 

#include <d3d11.h>

#include "nvapi.h"
#include <nvapi/NvApiDriverSettings.h>
#include <nvapi/nvapi_lite_sli.h>

#include "core.h"

#include <Windows.h>
#include <dxgi.h>
#include <string>

//
// Undocumented Functions
//
//  ** (I am not breaking any NDA; I found these the hard way!)
//
NvAPI_GPU_GetRamType_t            NvAPI_GPU_GetRamType;
NvAPI_GPU_GetFBWidthAndLocation_t NvAPI_GPU_GetFBWidthAndLocation;
NvAPI_GPU_GetPCIEInfo_t           NvAPI_GPU_GetPCIEInfo;
NvAPI_GetPhysicalGPUFromGPUID_t   NvAPI_GetPhysicalGPUFromGPUID;
NvAPI_GetGPUIDFromPhysicalGPU_t   NvAPI_GetGPUIDFromPhysicalGPU;

#ifdef _WIN64
#pragma comment (lib, "nvapi/amd64/nvapi64.lib")
#else
#pragma comment (lib, "nvapi/x86/nvapi.lib")
#endif

using namespace sk;
using namespace sk::NVAPI;

static bool nvapi_silent = false;

#define NVAPI_SILENT()  { nvapi_silent = true;  }
#define NVAPI_VERBOSE() { nvapi_silent = false; }

#if 0
#define NVAPI_CALL(x) { NvAPI_Status ret = NvAPI_##x;        \
                        if (nvapi_silent != true &&          \
                                     ret != NVAPI_OK)        \
                          MessageBox (                       \
                            NVAPI_ErrorMessage (             \
                              ret, #x, __LINE__,             \
                              __FUNCTION__, __FILE__),       \
                            L"Error Calling NVAPI Function", \
                            MB_OK | MB_ICONASTERISK );       \
                      }
#else
#define NVAPI_CALL(x) { NvAPI_Status ret = NvAPI_##x; if (nvapi_silent !=    \
 true && ret != NVAPI_OK) MessageBox (NULL, ErrorMessage (ret, #x, __LINE__, \
__FUNCTION__, __FILE__).c_str (), L"Error Calling NVAPI Function", MB_OK     \
 | MB_ICONASTERISK | MB_SETFOREGROUND | MB_TOPMOST ); }

#define NVAPI_CALL2(x,y) { ##y = NvAPI_##x; if (nvapi_silent != true &&     \
##y != NVAPI_OK) MessageBox (                                               \
  NULL, ErrorMessage (##y, #x, __LINE__, __FUNCTION__, __FILE__).c_str (),  \
L"Error Calling NVAPI Function", MB_OK | MB_ICONASTERISK | MB_SETFOREGROUND \
 | MB_TOPMOST ); }

#endif

#define NVAPI_SET_DWORD(x,y,z) (x).version = NVDRS_SETTING_VER;       \
                               (x).settingId = (y); (x).settingType = \
                                 NVDRS_DWORD_TYPE;                    \
                               (x).u32CurrentValue = (z);             \
                               (x).isCurrentPredefined = 0;


std::wstring
NVAPI::ErrorMessage (_NvAPI_Status err,
                     const char*   args,
                     UINT          line_no,
                     const char*   function_name,
                     const char*   file_name)
{
  char szError [64];

  NvAPI_GetErrorMessage (err, szError);

  wchar_t wszError          [64];
  wchar_t wszFile           [256];
  wchar_t wszFunction       [256];
  wchar_t wszArgs           [256];
  wchar_t wszFormattedError [1024];

  MultiByteToWideChar (CP_OEMCP, 0, szError,       -1, wszError,     64);
  MultiByteToWideChar (CP_OEMCP, 0, file_name,     -1, wszFile,     256);
  MultiByteToWideChar (CP_OEMCP, 0, function_name, -1, wszFunction, 256);
  MultiByteToWideChar (CP_OEMCP, 0, args,          -1, wszArgs,     256);
  *wszFormattedError = L'\0';

  swprintf ( wszFormattedError, 1024,
              L"Line %u of %s (in %s (...)):\n"
              L"------------------------\n\n"
              L"NvAPI_%s\n\n\t>> %s <<",
               line_no,
                wszFile,
                 wszFunction,
                  wszArgs,
                   wszError );

  return wszFormattedError;
}

int
NVAPI::CountPhysicalGPUs (void)
{
  static int nv_gpu_count = -1;

  if (nv_gpu_count == -1) {
    if (nv_hardware) {
      NvPhysicalGpuHandle gpus [64];
      NvU32               gpu_count = 0;

      NVAPI_CALL (EnumPhysicalGPUs (gpus, &gpu_count));

      nv_gpu_count = gpu_count;
    }
    else {
      nv_gpu_count = 0;
    }
  }

  return nv_gpu_count;
}

int
NVAPI::CountSLIGPUs (void)
{
  static int nv_sli_count = -1;

  DXGI_ADAPTER_DESC* adapters = NVAPI::EnumGPUs_DXGI ();

  if (nv_sli_count == -1) {
    if (nv_hardware) {
      nv_sli_count = 0;

      while (adapters != nullptr) {
        if (adapters->AdapterLuid.LowPart > 1)
          nv_sli_count++;

        ++adapters;

        if (*adapters->Description == '\0')
          break;
      }
    }
  }

  return nv_sli_count;
}

static DXGI_ADAPTER_DESC   _nv_sli_adapters [64];

DXGI_ADAPTER_DESC*
NVAPI::EnumSLIGPUs (void)
{
  static int nv_sli_count = -1;

  *_nv_sli_adapters [0].Description = L'\0';

  DXGI_ADAPTER_DESC* adapters = NVAPI::EnumGPUs_DXGI ();

  if (nv_sli_count == -1) {
    if (nv_hardware) {
      nv_sli_count = 0;

      while (adapters != nullptr) {
        if (adapters->AdapterLuid.LowPart > 1)
          memcpy (&_nv_sli_adapters [nv_sli_count++], adapters,
                  sizeof (DXGI_ADAPTER_DESC));

        ++adapters;

        if (*adapters->Description == '\0')
          break;
      }

      *_nv_sli_adapters [nv_sli_count].Description = L'\0';
    }
  }

  return _nv_sli_adapters;
}

/**
 * These were hoisted out of EnumGPUs_DXGI (...) to reduce stack size.
 *
 *  ... but, it very likely has caused thread safety issues.
 **/
static DXGI_ADAPTER_DESC   _nv_dxgi_adapters [64];
static NvPhysicalGpuHandle _nv_dxgi_gpus     [64];
static NvPhysicalGpuHandle phys [64];

// This function does much more than it's supposed to -- consider fixing that!
DXGI_ADAPTER_DESC*
sk::NVAPI::EnumGPUs_DXGI (void)
{
  // Only do this once...
  static bool enumerated = false;

  // Early-out if this was already called once before.
  if (enumerated)
    return _nv_dxgi_adapters;

  if (! nv_hardware) {
    enumerated = true;
    *_nv_dxgi_adapters [0].Description = L'\0';
    return _nv_dxgi_adapters;
  }

  NvU32 gpu_count     = 0;

  NVAPI_CALL (EnumPhysicalGPUs (_nv_dxgi_gpus, &gpu_count));

  for (int i = 0; i < CountPhysicalGPUs (); i++) {
    DXGI_ADAPTER_DESC adapterDesc;

    NvAPI_ShortString name;

    int   sli_group = 0;
    int   sli_size  = 0;

    NVAPI_CALL (EnumPhysicalGPUs (_nv_dxgi_gpus,     &gpu_count));

    NvU32              phys_count;
    NvLogicalGpuHandle logical;

    NVAPI_CALL (GetLogicalGPUFromPhysicalGPU  (_nv_dxgi_gpus [i], &logical));
    NVAPI_CALL (GetPhysicalGPUsFromLogicalGPU (logical, phys, &phys_count));

    sli_group = (size_t)logical & 0xffffffff;
    sli_size  = phys_count;

    NVAPI_CALL (GPU_GetFullName (_nv_dxgi_gpus [i], name));

    NV_DISPLAY_DRIVER_MEMORY_INFO meminfo;
    meminfo.version = NV_DISPLAY_DRIVER_MEMORY_INFO_VER_2;
    // ^^^ V3 is for Windows 10+ only, we don't even care about eviction stats.

    NVAPI_CALL (GPU_GetMemoryInfo (_nv_dxgi_gpus [i], &meminfo));

    MultiByteToWideChar (CP_OEMCP, 0, name, -1, adapterDesc.Description, 64);

    adapterDesc.VendorId = 0x10de;

    adapterDesc.AdapterLuid.HighPart = sli_group;
    adapterDesc.AdapterLuid.LowPart  = sli_size;

    // NVIDIA's driver measures these numbers in KiB (to store as a 32-bit int)
    //  * We want the numbers in bytes (64-bit)
    adapterDesc.DedicatedVideoMemory  =
      (size_t)meminfo.dedicatedVideoMemory << 10;
    adapterDesc.DedicatedSystemMemory =
      (size_t)meminfo.systemVideoMemory    << 10;
    adapterDesc.SharedSystemMemory    = 
      (size_t)meminfo.sharedSystemMemory   << 10;

    _nv_dxgi_adapters [i] = adapterDesc;
  }

  *_nv_dxgi_adapters [gpu_count].Description = L'\0';

  enumerated = true;

  return _nv_dxgi_adapters;
}

DXGI_ADAPTER_DESC*
NVAPI::FindGPUByDXGIName (const wchar_t* wszName)
{
  DXGI_ADAPTER_DESC* adapters = EnumGPUs_DXGI ();

  //"NVIDIA "
  // 01234567

  wchar_t* wszFixedName = _wcsdup (wszName + 7);
  int      fixed_len    = lstrlenW (wszFixedName);

  // Remove trailing whitespace some NV GPUs inexplicably have in their names
  for (int i = fixed_len; i > 0; i--) {
    if (wszFixedName [i - 1] == L' ')
      wszFixedName [i - 1] = L'\0';
    else
      break;
  }

  while (*adapters->Description != L'\0') {
    //strstrw (lstrcmpiW () == 0) { // More accurate, but some GPUs have
                                    //   trailing spaces in their names.
                                    // -----
                                    // What the heck?!

    if (wcsstr (adapters->Description, wszFixedName) != NULL) {
      delete [] wszFixedName;
      return adapters;
    }

    ++adapters;
  }

  delete [] wszFixedName;

  return NULL;
}

std::wstring
NVAPI::GetDriverVersion (NvU32* pVer)
{
  NvU32             ver;
  NvAPI_ShortString ver_str;       // ANSI
  wchar_t           ver_wstr [64]; // Unicode

  NvAPI_SYS_GetDriverAndBranchVersion (&ver, ver_str);

  // The driver-branch string's not particularly user frieldy,
  //   let's do this the right way and report a number the end-user
  //     is actually going to recognize...
  swprintf (ver_wstr, 64, L"%u.%u", ver / 100, ver % 100);

  if (pVer != NULL)
    *pVer = ver;

  return ver_wstr;
}


BOOL bLibShutdown = FALSE;
BOOL bLibInit = FALSE;

BOOL
NVAPI::UnloadLibrary (void)
{
  if (bLibInit == TRUE && bLibShutdown == FALSE) {
    // Whine very loudly if this fails, because that's not
    //   supposed to happen!
    NVAPI_VERBOSE ()

      NvAPI_Status ret;

    NVAPI_CALL2 (Unload (), ret);

    if (ret == NVAPI_OK) {
      bLibShutdown = TRUE;
      bLibInit     = FALSE;
    }
  }

  return bLibShutdown;
}

#include "log.h"
#include "config.h"

BOOL
NVAPI::InitializeLibrary (const wchar_t* wszAppName)
{
  // It's silly to call this more than once, but not necessarily
  //  an error... just ignore repeated calls.
  if (bLibInit == TRUE)
    return TRUE;

  // If init is not false and not true, it's because we failed to
  //   initialize the API once before. Just return the failure status
  //     again.
  if (bLibInit != FALSE)
    return FALSE;

  app_name      = wszAppName;
  friendly_name = wszAppName; // Not so friendly, but whatever...

  NvAPI_Status ret;

  if (config.nvidia.api.disable) {
    nv_hardware = false;
    bLibInit    = TRUE + 1; // Clearly this isn't a boolean; just for looks
    return FALSE;
  }

  // We want this error to be silent, because this tool works on AMD GPUs too!
  NVAPI_SILENT ()
  {
    NVAPI_CALL2 (Initialize (), ret);
  }
  NVAPI_VERBOSE ()

    if (ret != NVAPI_OK) {
      nv_hardware = false;
      bLibInit    = TRUE + 1; // Clearly this isn't a boolean; just for looks
      return FALSE;
    }
    else {
      // True unless we fail the stuff below...
      nv_hardware = true;

      //
      // Time to initialize a few undocumented (if you do not sign an NDA)
      //   parts of NvAPI, hurray!
      //
#ifdef _WIN64
      static HMODULE hLib = LoadLibrary (L"nvapi64.dll");
      GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_PIN, L"nvapi64.dll", &hLib);
#else
      static HMODULE hLib = LoadLibrary (L"nvapi.dll");
      GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_PIN, L"nvapi.dll", &hLib);
#endif

    if (hLib != nullptr) {
      typedef void* (*NvAPI_QueryInterface_t)(unsigned int ordinal);

      static NvAPI_QueryInterface_t NvAPI_QueryInterface =
        (NvAPI_QueryInterface_t)GetProcAddress (hLib, "nvapi_QueryInterface");

      NvAPI_GPU_GetRamType =
        (NvAPI_GPU_GetRamType_t)NvAPI_QueryInterface (0x57F7CAAC);
      NvAPI_GPU_GetFBWidthAndLocation =
        (NvAPI_GPU_GetFBWidthAndLocation_t)NvAPI_QueryInterface (0x11104158);
      NvAPI_GPU_GetPCIEInfo =
        (NvAPI_GPU_GetPCIEInfo_t)NvAPI_QueryInterface (0xE3795199UL);
      NvAPI_GetPhysicalGPUFromGPUID =
        (NvAPI_GetPhysicalGPUFromGPUID_t)NvAPI_QueryInterface (0x5380AD1A);
      NvAPI_GetGPUIDFromPhysicalGPU =
        (NvAPI_GetGPUIDFromPhysicalGPU_t)NvAPI_QueryInterface (0x6533EA3E);

      if (NvAPI_GPU_GetRamType == nullptr) {
        dll_log.LogEx (false, L"missing NvAPI_GPU_GetRamType ");
        nv_hardware = false;
      }

      if (NvAPI_GPU_GetFBWidthAndLocation == nullptr) {
        dll_log.LogEx (false, L"missing NvAPI_GPU_GetFBWidthAndLocation ");
        nv_hardware = false;
      }

      if (NvAPI_GPU_GetPCIEInfo == nullptr) {
        dll_log.LogEx (false, L"missing NvAPI_GPU_GetPCIEInfo ");
        nv_hardware = false;
      }

      if (NvAPI_GetPhysicalGPUFromGPUID == nullptr) {
        dll_log.LogEx (false, L"missing NvAPI_GetPhysicalGPUFromGPUID ");
        nv_hardware = false;
      }

      if (NvAPI_GetGPUIDFromPhysicalGPU == nullptr) {
        dll_log.LogEx (false, L"missing NvAPI_GetGPUIDFromPhysicalGPU ");
        nv_hardware = false;
      }
    }

    else {
      dll_log.Log (L"unable to complete LoadLibrary (...) ");
      nv_hardware = false;
    }

    if (nv_hardware == false)
      bLibInit = FALSE;
  }

  //if (! CheckDriverVersion ()) {
    //MessageBox (NULL,
                //L"WARNING:  Your display drivers are too old to play this game!\n",
                //L"Please update your display drivers (Minimum Version = 355.82)",
                //MB_OK | MB_ICONEXCLAMATION);
  //}

  return (bLibInit = TRUE);
}

bool
NVAPI::CheckDriverVersion (void)
{
  NvU32 ver;
  GetDriverVersion (&ver);

  return ver >= 35582;
  if (ver < 35582) {
    return false;
  }

  return true;
}

NV_GET_CURRENT_SLI_STATE
NVAPI::GetSLIState (IUnknown* pDev)
{
  NV_GET_CURRENT_SLI_STATE state;
  state.version = NV_GET_CURRENT_SLI_STATE_VER;

  NvAPI_D3D_GetCurrentSLIState (pDev, &state);

  return state;
}


#include "config.h"

// Easier to DLL export this way
BOOL
__stdcall
SK_NvAPI_SetFramerateLimit (uint32_t limit)
{
  return TRUE;


  // Allow the end-user to override this using the INI file
  if (config.render.framerate.target_fps != 0)
    limit = config.render.framerate.target_fps;

  // Don't set anything up.
  if (limit == 0)
    return true;

  NvDRSSessionHandle hSession;
  NVAPI_CALL (DRS_CreateSession (&hSession));
  NVAPI_CALL (DRS_LoadSettings  (hSession));

  NvDRSProfileHandle hProfile;
  static NVDRS_APPLICATION app = { 0 };

  //SK_NvAPI_GetAppProfile (hSession, &hProfile, &app);
  NVAPI_SILENT ();

  app.version = NVDRS_APPLICATION_VER;

  NvAPI_Status ret;
  NVAPI_CALL2 ( DRS_FindApplicationByName ( hSession,
                                              (NvU16 *)app_name.c_str (),
                                                &hProfile,
                                                  &app ),
                ret );

  // If no executable exists anywhere by this name, create a profile for it
  //   and then add the executable to it.
  if (ret == NVAPI_EXECUTABLE_NOT_FOUND) {
    NVDRS_PROFILE custom_profile = { 0 };

    custom_profile.isPredefined = false;
    lstrcpyW ((wchar_t *)custom_profile.profileName, friendly_name.c_str ());
    custom_profile.version = NVDRS_PROFILE_VER;

    // It's not necessarily wrong if this does not return NVAPI_OK, so don't
    //   raise a fuss if it happens.
    NVAPI_SILENT ()
    {
      NVAPI_CALL2 (DRS_CreateProfile (hSession, &custom_profile, &hProfile), ret);
    }
    NVAPI_VERBOSE ()

    // Add the application name to the profile, if a profile already exists
    if (ret == NVAPI_PROFILE_NAME_IN_USE)
      NVAPI_CALL2 ( DRS_FindProfileByName ( hSession,
                                              (NvU16 *)friendly_name.c_str (),
                                                &hProfile),
                      ret );

    if (ret == NVAPI_OK) {
      memset (&app, 0, sizeof (NVDRS_APPLICATION));

      lstrcpyW ((wchar_t *)app.appName,          app_name.c_str      ());
      lstrcpyW ((wchar_t *)app.userFriendlyName, friendly_name.c_str ());
      app.version      = NVDRS_APPLICATION_VER;
      app.isPredefined = false;
      app.isMetro      = false;

      NVAPI_CALL2 (DRS_CreateApplication (hSession, hProfile, &app), ret);
      NVAPI_CALL2 (DRS_SaveSettings      (hSession), ret);
    }
  }

  NVDRS_SETTING fps_limiter = { 0 };
  fps_limiter.version = NVDRS_SETTING_VER;

  NVDRS_SETTING prerendered_frames = { 0 };
  prerendered_frames.version = NVDRS_SETTING_VER;

  // These settings may not exist, and getting back a value of 0 is okay...
  NVAPI_SILENT  ();
  NVAPI_CALL    (DRS_GetSetting (hSession, hProfile, PS_FRAMERATE_LIMITER_ID, &fps_limiter));
  NVAPI_CALL    (DRS_GetSetting (hSession, hProfile, PRERENDERLIMIT_ID,       &prerendered_frames));
  NVAPI_VERBOSE ();

  NvU32 limit_mask = ( PS_FRAMERATE_LIMITER_ENABLED        |
                       PS_FRAMERATE_LIMITER_FORCEON        |
                       PS_FRAMERATE_LIMITER_ALLOW_WINDOWED |
                       PS_FRAMERATE_LIMITER_ACCURATE );

  limit_mask |= (limit & PS_FRAMERATE_LIMITER_FPSMASK);

  // We need a way to forcefully disable this
  if (limit == 255)
    limit_mask = 0;

  // Default to application preference
  uint32_t target_prerender = config.render.framerate.pre_render_limit;

  BOOL already_set = TRUE;

  if (fps_limiter.u32CurrentValue != limit_mask) {
    ZeroMemory (&fps_limiter, sizeof NVDRS_SETTING);
    fps_limiter.version = NVDRS_SETTING_VER;

    already_set = FALSE;

    NVAPI_SET_DWORD (fps_limiter, PS_FRAMERATE_LIMITER_ID, limit_mask);
    NVAPI_CALL      (DRS_SetSetting (hSession, hProfile, &fps_limiter));
  }

  // If this == -1, then don't make any changes...
  if (target_prerender != -1 && prerendered_frames.u32CurrentValue != target_prerender) {
    ZeroMemory (&prerendered_frames, sizeof NVDRS_SETTING);
    prerendered_frames.version = NVDRS_SETTING_VER;

    already_set = FALSE;

    NVAPI_SET_DWORD (prerendered_frames, PRERENDERLIMIT_ID, target_prerender);
    NVAPI_CALL      (DRS_SetSetting (hSession, hProfile, &prerendered_frames));
  }

  NVAPI_VERBOSE ();

  NVAPI_CALL (DRS_SaveSettings   (hSession));
  NVAPI_CALL (DRS_DestroySession (hSession));

  if (! already_set) {
#ifdef WIN32
    HMODULE hLib = LoadLibrary (L"nvapi.dll");
#else
    HMODULE hLib = LoadLibrary (L"nvapi64.dll");
#endif
#define __NvAPI_RestartDisplayDriver                      0xB4B26B65
    typedef void* (*NvAPI_QueryInterface_t)(unsigned int offset);
    typedef NvAPI_Status(__cdecl *NvAPI_RestartDisplayDriver_t)(void);
    NvAPI_QueryInterface_t       NvAPI_QueryInterface       =
      (NvAPI_QueryInterface_t)GetProcAddress (hLib, "nvapi_QueryInterface");
    NvAPI_RestartDisplayDriver_t NvAPI_RestartDisplayDriver =
      (NvAPI_RestartDisplayDriver_t)NvAPI_QueryInterface (__NvAPI_RestartDisplayDriver);

    NvAPI_RestartDisplayDriver ();
  }

  return already_set;
}

BOOL
sk::NVAPI::SetFramerateLimit (uint32_t limit)
{
  return SK_NvAPI_SetFramerateLimit (limit);
}

// From the NDA version of NvAPI
#define SLI_COMPAT_BITS_DXGI_ID 0x00A06946
#define SLI_COMPAT_BITS_DX9_ID  0x1095DEF8

BOOL
sk::NVAPI::SetSLIOverride    (       DLL_ROLE role,
                                const wchar_t* wszModeName,
                                const wchar_t* wszGPUCount,
                                const wchar_t* wszCompatBits )
{
  // SLI only works in DirectX.
  if (role != DXGI && role != D3D9)
    return TRUE;

  NvDRSSessionHandle hSession;
  NVAPI_CALL (DRS_CreateSession (&hSession));
  NVAPI_CALL (DRS_LoadSettings  (hSession));

  NvDRSProfileHandle hProfile;
  static NVDRS_APPLICATION  app = { 0 };

  NvU32 compat_bits_enum = 
    (role == DXGI ? SLI_COMPAT_BITS_DXGI_ID :
                    SLI_COMPAT_BITS_DX9_ID);

  NvU32 gpu_count_enum =
    SLI_GPU_COUNT_ID;

  NvU32 render_mode_enum =
    SLI_RENDERING_MODE_ID;

  NvU32 render_mode = SLI_RENDERING_MODE_DEFAULT; //0x00000007;
  NvU32 gpu_count   = SLI_GPU_COUNT_DEFAULT;
  NvU32 compat_bits = 0x00000000;

  wchar_t* wszModeNameLower = _wcsdup (wszModeName);
  std::wstring mode_str (_wcslwr (wszModeNameLower));
  free (wszModeNameLower);

  if (mode_str == L"default")
    render_mode = SLI_RENDERING_MODE_DEFAULT;
  else if (mode_str == L"auto")
    render_mode = SLI_RENDERING_MODE_AUTOSELECT;
  else if (mode_str == L"single")
    render_mode = SLI_RENDERING_MODE_FORCE_SINGLE;
  else if (mode_str == L"afr")
    render_mode = SLI_RENDERING_MODE_FORCE_AFR;
  else if (mode_str == L"afr2")
    render_mode = SLI_RENDERING_MODE_FORCE_AFR2;
  else if (mode_str == L"sfr")
    render_mode = SLI_RENDERING_MODE_FORCE_SFR;
  else if (mode_str == L"3afr"       ||
           mode_str == L"afr of sfr" ||
           mode_str == L"afr3"       ||
           mode_str == L"afr of sfr  fallback 3afr")
    render_mode = SLI_RENDERING_MODE_FORCE_AFR_OF_SFR__FALLBACK_3AFR;
  else
    dll_log.Log ( L"[  NvAPI   ] >> Unknown SLI Mode: '%s', falling back to 'Auto'",
                    wszModeName );

  wchar_t* wszGPUCountLower = _wcsdup (wszGPUCount);
  std::wstring gpu_count_str (_wcslwr (wszGPUCountLower));
  free (wszGPUCountLower);

  if (gpu_count_str == L"default")
    gpu_count = SLI_GPU_COUNT_DEFAULT;
  else if (gpu_count_str == L"auto")
    gpu_count = SLI_GPU_COUNT_AUTOSELECT;
  else if (gpu_count_str == L"one"   || gpu_count_str == L"1")
    gpu_count = SLI_GPU_COUNT_ONE;
  else if (gpu_count_str == L"two"   || gpu_count_str == L"2")
    gpu_count = SLI_GPU_COUNT_TWO;
  else if (gpu_count_str == L"three" || gpu_count_str == L"3")
    gpu_count = SLI_GPU_COUNT_THREE;
  else if (gpu_count_str == L"four"  || gpu_count_str == L"4")
    gpu_count = SLI_GPU_COUNT_FOUR;
  else
    dll_log.Log ( L"[  NvAPI   ] >> Unknown SLI GPU Count: '%s', falling back to 'Auto'",
                    wszGPUCount );

  compat_bits = wcstoul (wszCompatBits, nullptr, 16);

  NVAPI_SILENT ();

  app.version = NVDRS_APPLICATION_VER;

  NvAPI_Status ret;
  NVAPI_CALL2 ( DRS_FindApplicationByName ( hSession,
                                              (NvU16 *)app_name.c_str (),
                                                &hProfile,
                                                  &app ),
                ret );

  // If no executable exists anywhere by this name, create a profile for it
  //   and then add the executable to it.
  if (ret == NVAPI_EXECUTABLE_NOT_FOUND) {
    NVDRS_PROFILE custom_profile = { 0 };

    custom_profile.isPredefined = false;
    lstrcpyW ((wchar_t *)custom_profile.profileName, friendly_name.c_str ());
    custom_profile.version = NVDRS_PROFILE_VER;

    // It's not necessarily wrong if this does not return NVAPI_OK, so don't
    //   raise a fuss if it happens.
    NVAPI_SILENT ()
    {
      NVAPI_CALL2 (DRS_CreateProfile (hSession, &custom_profile, &hProfile), ret);
    }
    NVAPI_VERBOSE ()

    // Add the application name to the profile, if a profile already exists
    if (ret == NVAPI_PROFILE_NAME_IN_USE)
      NVAPI_CALL2 ( DRS_FindProfileByName ( hSession,
                                              (NvU16 *)friendly_name.c_str (),
                                                &hProfile),
                      ret );

    if (ret == NVAPI_OK) {
      memset (&app, 0, sizeof (NVDRS_APPLICATION));

      lstrcpyW ((wchar_t *)app.appName,          app_name.c_str      ());
      lstrcpyW ((wchar_t *)app.userFriendlyName, friendly_name.c_str ());
      app.version      = NVDRS_APPLICATION_VER;
      app.isPredefined = false;
      app.isMetro      = false;

      NVAPI_CALL2 (DRS_CreateApplication (hSession, hProfile, &app), ret);
      NVAPI_CALL2 (DRS_SaveSettings      (hSession), ret);
    }
  }

  NVDRS_SETTING mode_val = { 0 };
  mode_val.version = NVDRS_SETTING_VER;

  NVDRS_SETTING gpu_count_val = { 0 };
  gpu_count_val.version = NVDRS_SETTING_VER;

  NVDRS_SETTING compat_bits_val = { 0 };
  compat_bits_val.version = NVDRS_SETTING_VER;

  // These settings may not exist, and getting back a value of 0 is okay...
  NVAPI_SILENT ();
  NVAPI_CALL (DRS_GetSetting (hSession, hProfile, render_mode_enum, &mode_val));
  NVAPI_CALL (DRS_GetSetting (hSession, hProfile, gpu_count_enum,   &gpu_count_val));
  NVAPI_CALL (DRS_GetSetting (hSession, hProfile, compat_bits_enum, &compat_bits_val));
  NVAPI_VERBOSE ();

  BOOL already_set = TRUE;

  // Do this first so we don't touch other settings if this fails
  if (compat_bits_val.u32CurrentValue != compat_bits) {
    ZeroMemory (&compat_bits_val, sizeof NVDRS_SETTING);
    compat_bits_val.version = NVDRS_SETTING_VER;

    NvAPI_Status ret;

    // This requires admin privs, and we will handle that gracefully...
    NVAPI_SILENT ();
    NVAPI_SET_DWORD (compat_bits_val, compat_bits_enum, compat_bits);
    NVAPI_CALL2     (DRS_SetSetting (hSession, hProfile, &compat_bits_val), ret);
    NVAPI_VERBOSE ();

    // Not running as admin, don't do the override!
    if (ret == NVAPI_INVALID_USER_PRIVILEGE) {
      int result = 
        MessageBox ( NULL,
                       L"Please run this game as Administrator to install SLI "
                       L"compatibility bits\r\n\r\n"
                       L"\t>> Pressing Cancel will disable SLI Override",
                         L"Insufficient User Privileges",
                           MB_OKCANCEL | MB_ICONASTERISK | MB_SETFOREGROUND |
                           MB_TOPMOST );

      if (result == IDCANCEL) {
        config.nvidia.sli.override = false;
        NVAPI_CALL (DRS_DestroySession (hSession));
        return FALSE;
      }
      exit (0);
    }

    already_set = FALSE;
  }

  if (mode_val.u32CurrentValue != render_mode) {
    ZeroMemory (&mode_val, sizeof NVDRS_SETTING);
    mode_val.version = NVDRS_SETTING_VER;

    NVAPI_SET_DWORD (mode_val, render_mode_enum, render_mode);
    NVAPI_CALL      (DRS_SetSetting (hSession, hProfile, &mode_val));

    already_set = FALSE;
  }

  if (gpu_count_val.u32CurrentValue != gpu_count) {
    ZeroMemory (&gpu_count_val, sizeof NVDRS_SETTING);
    gpu_count_val.version = NVDRS_SETTING_VER;

    NVAPI_SET_DWORD (gpu_count_val, gpu_count_enum, gpu_count);
    NVAPI_CALL      (DRS_SetSetting (hSession, hProfile, &gpu_count_val));

    already_set = FALSE;
  }

  if (! already_set)
    NVAPI_CALL (DRS_SaveSettings (hSession));

  NVAPI_CALL (DRS_DestroySession (hSession));

  return already_set;
}

void
__stdcall
SK_NvAPI_SetAppFriendlyName (const wchar_t* wszFriendlyName)
{
  friendly_name = wszFriendlyName;
}

void
sk::NVAPI::SetAppFriendlyName (const wchar_t* wszFriendlyName)
{
  SK_NvAPI_SetAppFriendlyName (wszFriendlyName);
}

BOOL
__stdcall
SK_NvAPI_IsInit (void)
{
  return NVAPI::nv_hardware;
}

void
__stdcall
SK_NvAPI_SetAppName (const wchar_t* wszAppName)
{
  app_name = wszAppName;
}

void
__stdcall
SK_NvAPI_SetLauncher (const wchar_t* wszLauncherName)
{
  launcher_name = wszLauncherName;
}

BOOL
__stdcall
SK_NvAPI_AddLauncherToProf (void)
{
  if (! nv_hardware)
    return FALSE;

  NvDRSSessionHandle hSession;
  NVAPI_CALL (DRS_CreateSession (&hSession));
  NVAPI_CALL (DRS_LoadSettings  (hSession));

  NvDRSProfileHandle hProfile;
  static NVDRS_APPLICATION app = { 0 };

  //SK_NvAPI_GetAppProfile (hSession, &hProfile, &app);
  NVAPI_SILENT ();

  app.version = NVDRS_APPLICATION_VER;

  NvAPI_Status ret;
  NVAPI_CALL2 ( DRS_FindApplicationByName ( hSession,
                                              (NvU16 *)app_name.c_str (),
                                                &hProfile,
                                                  &app ),
                ret );

  // If no executable exists anywhere by this name, create a profile for it
  //   and then add the executable to it.
  if (ret == NVAPI_EXECUTABLE_NOT_FOUND) {
    NVDRS_PROFILE custom_profile = { 0 };

    custom_profile.isPredefined = false;
    lstrcpyW ((wchar_t *)custom_profile.profileName, friendly_name.c_str ());
    custom_profile.version = NVDRS_PROFILE_VER;

    // It's not necessarily wrong if this does not return NVAPI_OK, so don't
    //   raise a fuss if it happens.
    NVAPI_SILENT ()
    {
      NVAPI_CALL2 (DRS_CreateProfile (hSession, &custom_profile, &hProfile), ret);
    }
    NVAPI_VERBOSE ()

    // Add the application name to the profile, if a profile already exists
    if (ret == NVAPI_PROFILE_NAME_IN_USE)
      NVAPI_CALL2 ( DRS_FindProfileByName ( hSession,
                                              (NvU16 *)friendly_name.c_str (),
                                                &hProfile),
                      ret );

    if (ret == NVAPI_OK) {
      memset (&app, 0, sizeof (NVDRS_APPLICATION));

      *app.appName = L'\0';
      //lstrcpyW ((wchar_t *)app.appName,          app_name.c_str      ());
      lstrcpyW ((wchar_t *)app.userFriendlyName, friendly_name.c_str ());
      lstrcpyW ((wchar_t *)app.launcher,         launcher_name.c_str ());
      app.version      = NVDRS_APPLICATION_VER;
      app.isPredefined = false;
      app.isMetro      = false;

      NVAPI_CALL2 (DRS_CreateApplication (hSession, hProfile, &app), ret);
      NVAPI_CALL2 (DRS_SaveSettings      (hSession), ret);
    }
  } else {
    memset (&app, 0, sizeof (NVDRS_APPLICATION));

    *app.appName = L'\0';
    //lstrcpyW ((wchar_t *)app.appName,          app_name.c_str      ());
    lstrcpyW ((wchar_t *)app.userFriendlyName, friendly_name.c_str ());
    lstrcpyW ((wchar_t *)app.launcher,         launcher_name.c_str ());
    app.version      = NVDRS_APPLICATION_VER;
    app.isPredefined = false;
    app.isMetro      = false;

    NVAPI_CALL2 (DRS_CreateApplication (hSession, hProfile, &app), ret);
    NVAPI_CALL2 (DRS_SaveSettings      (hSession), ret);
  }

  NVAPI_CALL (DRS_SaveSettings   (hSession));
  NVAPI_CALL (DRS_DestroySession (hSession));

#if 0
  if (! already_set) {
#ifdef WIN32
    HMODULE hLib = LoadLibrary (L"nvapi.dll");
#else
    HMODULE hLib = LoadLibrary (L"nvapi64.dll");
#endif
#define __NvAPI_RestartDisplayDriver                      0xB4B26B65
    typedef void* (*NvAPI_QueryInterface_t)(unsigned int offset);
    typedef NvAPI_Status(__cdecl *NvAPI_RestartDisplayDriver_t)(void);
    NvAPI_QueryInterface_t       NvAPI_QueryInterface       =
      (NvAPI_QueryInterface_t)GetProcAddress (hLib, "nvapi_QueryInterface");
    NvAPI_RestartDisplayDriver_t NvAPI_RestartDisplayDriver =
      (NvAPI_RestartDisplayDriver_t)NvAPI_QueryInterface (__NvAPI_RestartDisplayDriver);

    NvAPI_RestartDisplayDriver ();
  }
#endif

  return true;
}

bool         NVAPI::nv_hardware   = false;
std::wstring NVAPI::friendly_name = L"Tales of Zestiria";
std::wstring NVAPI::app_name      = L"Tales Of Zestiria.exe";
std::wstring NVAPI::launcher_name = L"";