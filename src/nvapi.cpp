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

#include <d3d11.h>

#include <SpecialK/nvapi.h>
#include <nvapi/NvApiDriverSettings.h>
#include <nvapi/nvapi_lite_sli.h>

#include <SpecialK/core.h>
#include <SpecialK/config.h>
#include <SpecialK/log.h>

#include <Windows.h>
#include <dxgi.h>
#include <string>

#include <SpecialK/diagnostics/compatibility.h>
#include <SpecialK/utility.h>

//
// Undocumented Functions (unless you sign an NDA)
//
NvAPI_GPU_GetRamType_pfn            NvAPI_GPU_GetRamType;
NvAPI_GPU_GetFBWidthAndLocation_pfn NvAPI_GPU_GetFBWidthAndLocation;
NvAPI_GPU_GetPCIEInfo_pfn           NvAPI_GPU_GetPCIEInfo;
NvAPI_GetPhysicalGPUFromGPUID_pfn   NvAPI_GetPhysicalGPUFromGPUID;
NvAPI_GetGPUIDFromPhysicalGPU_pfn   NvAPI_GetGPUIDFromPhysicalGPU;

using namespace sk;
using namespace sk::NVAPI;

// Not thread-safe; doesn't much matter.
static bool nvapi_silent = false;

#define NVAPI_SILENT()  { nvapi_silent = true;  }
#define NVAPI_VERBOSE() { nvapi_silent = false; }

#if 0
#define NVAPI_CALL(x) { ret = NvAPI_##x;                     \
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
#define NVAPI_CALL(x) { ret = NvAPI_##x; if (nvapi_silent !=                 \
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
{ char szError [256];

  NvAPI_GetErrorMessage (err, szError);

  wchar_t wszFormattedError [1024] = { };

  swprintf ( wszFormattedError, 1024,
              L"Line %u of %hs (in %hs (...)):\n"
              L"------------------------\n\n"
              L"NvAPI_%hs\n\n\t>> %hs <<",
               line_no,
                file_name,
                 function_name,
                  args,
                   szError );

  return wszFormattedError;
}

int
NVAPI::CountPhysicalGPUs (void)
{
  static int nv_gpu_count = -1;

  if (nv_gpu_count == -1)
  {
    if (nv_hardware)
    {
      NvAPI_Status        ret       = NVAPI_ERROR;
      NvPhysicalGpuHandle gpus [64] = {         };
      NvU32               gpu_count =      0;

      NVAPI_CALL (EnumPhysicalGPUs (gpus, &gpu_count));

      nv_gpu_count = gpu_count;
    }

    else
      nv_gpu_count = 0;
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

  DXGI_ADAPTER_DESC* adapters =
    NVAPI::EnumGPUs_DXGI ();

  if (nv_sli_count == -1)
  {
    if (nv_hardware)
    {
      nv_sli_count = 0;

      while (adapters != nullptr)
      {
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
static DXGI_ADAPTER_DESC   _nv_dxgi_adapters [64] = { };
static NvPhysicalGpuHandle _nv_dxgi_gpus     [64] = { };
static NvPhysicalGpuHandle              phys [64] = { };

// This function does much more than it's supposed to -- consider fixing that!
DXGI_ADAPTER_DESC*
sk::NVAPI::EnumGPUs_DXGI (void)
{
  // Only do this once...
  static bool enumerated = false;

  // Early-out if this was already called once before.
  if (enumerated)
    return _nv_dxgi_adapters;

  if (! nv_hardware)
  {
    enumerated = true;
    *_nv_dxgi_adapters [0].Description = L'\0';
    return _nv_dxgi_adapters;
  }

  NvAPI_Status ret       = NVAPI_ERROR;
  NvU32        gpu_count = 0;

  NVAPI_CALL (EnumPhysicalGPUs (_nv_dxgi_gpus, &gpu_count));

  for (int i = 0; i < CountPhysicalGPUs (); i++)
  {
    DXGI_ADAPTER_DESC adapterDesc = { };
    NvAPI_ShortString name        = { };

    int   sli_group = 0;
    int   sli_size  = 0;

    NVAPI_CALL (EnumPhysicalGPUs (_nv_dxgi_gpus,     &gpu_count));

    NvU32              phys_count;
    NvLogicalGpuHandle logical;

    NVAPI_CALL (GetLogicalGPUFromPhysicalGPU  (_nv_dxgi_gpus [i], &logical));
    NVAPI_CALL (GetPhysicalGPUsFromLogicalGPU (logical, phys, &phys_count));

    sli_group = reinterpret_cast <size_t> (logical) & 0xffffffff;
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
      static_cast <size_t> (meminfo.dedicatedVideoMemory) << 10;
    adapterDesc.DedicatedSystemMemory =
      static_cast <size_t> (meminfo.systemVideoMemory)    << 10;
    adapterDesc.SharedSystemMemory    = 
      static_cast <size_t> (meminfo.sharedSystemMemory)   << 10;

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

    if (wcsstr (adapters->Description, wszFixedName) != nullptr)
    {
      free (wszFixedName);
      return adapters;
    }

    ++adapters;
  }

  free (wszFixedName);

  return nullptr;
}

std::wstring
NVAPI::GetDriverVersion (NvU32* pVer)
{
  NvU32             ver           =  0;
  NvAPI_ShortString ver_str       = { }; // ANSI
  wchar_t           ver_wstr [64] = { }; // Unicode

  NvAPI_SYS_GetDriverAndBranchVersion (&ver, ver_str);

  // The driver-branch string's not particularly user frieldy,
  //   let's do this the right way and report a number the end-user
  //     is actually going to recognize...
  _snwprintf (ver_wstr, 63, L"%u.%u", ver / 100, ver % 100);
  ver_wstr [63] = L'\0';

  if (pVer != nullptr)
    *pVer = ver;

  return ver_wstr;
}


BOOL bLibShutdown = FALSE;
BOOL bLibInit     = FALSE;

BOOL
NVAPI::UnloadLibrary (void)
{
  if (bLibInit == TRUE && bLibShutdown == FALSE) {
    // Whine very loudly if this fails, because that's not
    //   supposed to happen!
    NVAPI_VERBOSE ()

      NvAPI_Status ret;

    ret = NvAPI_Unload ();//NVAPI_CALL2 (Unload (), ret);

    if (ret == NVAPI_OK) {
      bLibShutdown = TRUE;
      bLibInit     = FALSE;
    }
  }

  return bLibShutdown;
}

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

  if (! config.apis.NvAPI.enable) {
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
      static HMODULE hLib = nullptr;

#ifdef _WIN64
      GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_PIN, L"nvapi64.dll", &hLib);
#else
      GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_PIN, L"nvapi.dll",   &hLib);
#endif

    if (hLib != nullptr)
    {
      using NvAPI_QueryInterface_pfn = void* (*)(unsigned int ordinal);

      static auto NvAPI_QueryInterface =
        reinterpret_cast <NvAPI_QueryInterface_pfn> (
          GetProcAddress (hLib, "nvapi_QueryInterface")
        );

      NvAPI_GPU_GetRamType =
        (NvAPI_GPU_GetRamType_pfn)NvAPI_QueryInterface            (0x57F7CAACu);
      NvAPI_GPU_GetFBWidthAndLocation =
        (NvAPI_GPU_GetFBWidthAndLocation_pfn)NvAPI_QueryInterface (0x11104158u);
      NvAPI_GPU_GetPCIEInfo =
        (NvAPI_GPU_GetPCIEInfo_pfn)NvAPI_QueryInterface           (0xE3795199u);
      NvAPI_GetPhysicalGPUFromGPUID =
        (NvAPI_GetPhysicalGPUFromGPUID_pfn)NvAPI_QueryInterface   (0x5380AD1Au);
      NvAPI_GetGPUIDFromPhysicalGPU =
        (NvAPI_GetGPUIDFromPhysicalGPU_pfn)NvAPI_QueryInterface   (0x6533EA3Eu);

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

    if (nv_hardware == false) {
      bLibInit = FALSE;
      hLib     = nullptr;
    }
  }

  return (bLibInit = TRUE);
}

NV_GET_CURRENT_SLI_STATE
NVAPI::GetSLIState (IUnknown* pDev)
{
  NV_GET_CURRENT_SLI_STATE state = {                          };
  state.version                  = NV_GET_CURRENT_SLI_STATE_VER;

  NvAPI_D3D_GetCurrentSLIState (pDev, &state);

  return state;
}

// From the NDA version of NvAPI
#define AA_COMPAT_BITS_DX9_ID   0x00D55F7D
#define AA_COMPAT_BITS_DXGI_ID  0x00E32F8A
#define AA_FIX_ID               0x000858F7

BOOL
__stdcall
SK_NvAPI_SetAntiAliasingOverride ( const wchar_t** pwszPropertyList )
{
  // This stuff only works in DirectX.
  if (SK_GetDLLRole () != DXGI && SK_GetDLLRole () != D3D9)
    return FALSE;

  if (! nv_hardware)
    return FALSE;

  static bool             init           = false;
  static CRITICAL_SECTION cs_aa_override = { };

  if (! init)
  {
    InitializeCriticalSection (&cs_aa_override);
    init = true;
  }

  EnterCriticalSection (&cs_aa_override);

  NvAPI_Status       ret       = NVAPI_ERROR;
  NvDRSSessionHandle hSession  = { };

  NVAPI_CALL (DRS_CreateSession (&hSession));
  NVAPI_CALL (DRS_LoadSettings  (hSession));

  NvDRSProfileHandle hProfile;
  NVDRS_APPLICATION  app = { };

  NvU32 compat_bits_enum = 
    (SK_GetDLLRole () == DXGI ? AA_COMPAT_BITS_DXGI_ID :
                                AA_COMPAT_BITS_DX9_ID);

  NvU32 compat_bits = 0xFFFFFFFF;

  struct property_pair_s {
    const wchar_t* wszName  = nullptr;
    const wchar_t* wszValue = nullptr;
  } prop;

  const wchar_t** pwszPropertyListEntry =
    pwszPropertyList;

  prop.wszName  = *(pwszPropertyListEntry++);
  prop.wszValue = *(pwszPropertyListEntry++);


  unsigned int method      = std::numeric_limits <unsigned int>::max ();
  unsigned int replay_mode = std::numeric_limits <unsigned int>::max ();
  unsigned int aa_fix      = std::numeric_limits <unsigned int>::max ();
  unsigned int override    = std::numeric_limits <unsigned int>::max ();
  unsigned int auto_bias   = std::numeric_limits <unsigned int>::max ();

  while (! ( prop.wszName  == nullptr && 
             prop.wszValue == nullptr ) )
  {
    // ...

    if (! _wcsicmp (prop.wszName, L"Override"))
    {
      if ( (! _wcsicmp (prop.wszValue, L"Yes")) ||
           (! _wcsicmp (prop.wszValue, L"On")) )
        override = AA_MODE_SELECTOR_OVERRIDE;

      else if ( (! _wcsicmp (prop.wszValue, L"No")) ||
                (! _wcsicmp (prop.wszValue, L"Off")) )
        override = AA_MODE_SELECTOR_APP_CONTROL;

      else if ( (! _wcsicmp (prop.wszValue, L"Enhance")) )
        override = AA_MODE_SELECTOR_ENHANCE;
    }

    if (! _wcsicmp (prop.wszName, L"Method"))
    {
      if (! _wcsicmp (prop.wszValue, L"2xMSAA"))
        method = AA_MODE_METHOD_MULTISAMPLE_2X_DIAGONAL;

      else if (! _wcsicmp (prop.wszValue, L"4xMSAA"))
        method = AA_MODE_METHOD_MULTISAMPLE_4X;

      else if (! _wcsicmp (prop.wszValue, L"8xMSAA"))
        method = AA_MODE_METHOD_MULTISAMPLE_8X;

      else if (! _wcsicmp (prop.wszValue, L"16xMSAA"))
        method = AA_MODE_METHOD_MULTISAMPLE_16X;

      //else if (! wcsicmp (prop.wszValue, L"32xMSAA"))
        //method = AA_MODE_METHOD_MULTISAMPLE_32;

      // Allow an arbitrary value to be set if it is expressed as a
      //   hexadecimal number.
      else if (wcsstr (prop.wszValue, L"0x") == prop.wszValue)
        method = wcstoul (prop.wszValue, nullptr, 16);
    }

    else if (! _wcsicmp (prop.wszName, L"ReplayMode"))
    {
      if (! _wcsicmp (prop.wszValue, L"2xSGSSAA"))
        replay_mode= AA_MODE_REPLAY_SAMPLES_TWO | AA_MODE_REPLAY_MODE_ALL;

      else if (! _wcsicmp (prop.wszValue, L"4xSGSSAA"))
        replay_mode = AA_MODE_REPLAY_SAMPLES_FOUR| AA_MODE_REPLAY_MODE_ALL;

      else if (! _wcsicmp (prop.wszValue, L"8xSGSSAA"))
        replay_mode = AA_MODE_REPLAY_SAMPLES_EIGHT | AA_MODE_REPLAY_MODE_ALL;

      // Allow an arbitrary value to be set if it is expressed as a
      //   hexadecimal number.
      else if (wcsstr (prop.wszValue, L"0x") == prop.wszValue)
        replay_mode = wcstoul (prop.wszValue, nullptr, 16);
    }

    else if (! _wcsicmp (prop.wszName, L"AntiAliasFix"))
    {
      if ( (! _wcsicmp (prop.wszValue, L"On")) ||
           (! _wcsicmp (prop.wszValue, L"Yes")) )
        aa_fix = 0x00;

      else
        aa_fix = 0x01;
    }

    else if (! _wcsicmp (prop.wszName, L"AutoBiasAdjust"))
    {
      if ( (! _wcsicmp (prop.wszValue, L"On")) ||
           (! _wcsicmp (prop.wszValue, L"Yes")) )
        auto_bias = AUTO_LODBIASADJUST_ON;
      else
        auto_bias = AUTO_LODBIASADJUST_OFF;
    }

    else if (! _wcsicmp (prop.wszName, L"CompatibilityBits"))
    {
      compat_bits = wcstoul (prop.wszValue, nullptr, 16);
    }

    prop.wszName  = *(pwszPropertyListEntry++);
    prop.wszValue = *(pwszPropertyListEntry++);
  }

  NVAPI_SILENT ();

  app.version = NVDRS_APPLICATION_VER;
  ret         = NVAPI_ERROR;

  NVAPI_CALL2 ( DRS_FindApplicationByName ( hSession,
                                              (NvU16 *)app_name.c_str (),
                                                &hProfile,
                                                  &app ),
                ret );

  // If no executable exists anywhere by this name, create a profile for it
  //   and then add the executable to it.
  if (ret == NVAPI_EXECUTABLE_NOT_FOUND)
  {
    NVDRS_PROFILE custom_profile = { };

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

    if (ret == NVAPI_OK)
    {
      app = { };

      lstrcpyW ((wchar_t *)app.appName,          app_name.c_str      ());
      lstrcpyW ((wchar_t *)app.userFriendlyName, friendly_name.c_str ());

      app.version      = NVDRS_APPLICATION_VER;
      app.isPredefined = false;
      app.isMetro      = false;

      NVAPI_CALL2 (DRS_CreateApplication (hSession, hProfile, &app), ret);
      NVAPI_CALL2 (DRS_SaveSettings      (hSession), ret);
    }

    // Driver's not being cooperative, we have no choice but to bail-out
    else {
      dll_log.Log ( L"[  NvAPI   ] Could not find or create application profile for '%s' (%s)",
                      friendly_name.c_str (), app_name.c_str () );

      NVAPI_CALL (DRS_DestroySession (hSession));

      LeaveCriticalSection (&cs_aa_override);

      return FALSE;
    }
  }

  NvU32         method_enum             = AA_MODE_METHOD_ID;
  NVDRS_SETTING method_val              = {               };
                method_val.version      = NVDRS_SETTING_VER;

  NvU32         replay_mode_enum        = AA_MODE_REPLAY_ID;
  NVDRS_SETTING replay_mode_val         = {               };
                replay_mode_val.version = NVDRS_SETTING_VER;

  NvU32         aa_fix_enum             = 0x000858F7;
  NVDRS_SETTING aa_fix_val              = {               };
                aa_fix_val.version      = NVDRS_SETTING_VER;

  NvU32         override_enum           = AA_MODE_SELECTOR_ID;
  NVDRS_SETTING override_val            = {               };
                override_val.version    = NVDRS_SETTING_VER;

  NvU32         autobias_enum           = AUTO_LODBIASADJUST_ID;
  NVDRS_SETTING autobias_val            = {               };
                autobias_val.version    = NVDRS_SETTING_VER;

  NVDRS_SETTING compat_bits_val         = {               };
                compat_bits_val.version = NVDRS_SETTING_VER;

  // These settings may not exist, and getting back a value of 0 is okay...
  NVAPI_SILENT ();
  NVAPI_CALL (DRS_GetSetting (hSession, hProfile, method_enum,      &method_val));
  NVAPI_CALL (DRS_GetSetting (hSession, hProfile, replay_mode_enum, &replay_mode_val));
  NVAPI_CALL (DRS_GetSetting (hSession, hProfile, aa_fix_enum,      &aa_fix_val));
  NVAPI_CALL (DRS_GetSetting (hSession, hProfile, override_enum,    &override_val));
  NVAPI_CALL (DRS_GetSetting (hSession, hProfile, autobias_enum,    &autobias_val));
  NVAPI_CALL (DRS_GetSetting (hSession, hProfile, compat_bits_enum, &compat_bits_val));
  NVAPI_VERBOSE ();

  BOOL already_set = TRUE;

  // Do this first so we don't touch other settings if this fails
  if (compat_bits_val.u32CurrentValue != compat_bits && compat_bits != 0xffffffff)
  {
    compat_bits_val         = {               };
    compat_bits_val.version = NVDRS_SETTING_VER;

    ret = NVAPI_ERROR;

    // This requires admin privs, and we will handle that gracefully...
    NVAPI_SILENT ();
    NVAPI_SET_DWORD (compat_bits_val, compat_bits_enum, compat_bits);
    NVAPI_CALL2     (DRS_SetSetting (hSession, hProfile, &compat_bits_val), ret);
    NVAPI_VERBOSE ();

    // Not running as admin, don't do the override!
    if (ret == NVAPI_INVALID_USER_PRIVILEGE)
    {
      int result = 
        MessageBox ( nullptr,
                       L"Please run this game as Administrator to install Anti-Aliasing "
                       L"compatibility bits\r\n\r\n"
                       L"\t>> Pressing Cancel will disable AA Override",
                         L"Insufficient User Privileges",
                           MB_OKCANCEL | MB_ICONASTERISK | MB_SETFOREGROUND |
                           MB_TOPMOST );

      if (result == IDCANCEL)
      {
        //config.nvidia.aa.override = false;
        NVAPI_CALL (DRS_DestroySession (hSession));

        LeaveCriticalSection (&cs_aa_override);

        return FALSE;
      }

      NVAPI_CALL (DRS_DestroySession (hSession));

      SK_ElevateToAdmin ();

      // Formality, ElevateToAdmin actually kills the process...
      return FALSE;
    }

    already_set = FALSE;
  }

  if (method_val.u32CurrentValue != method && method != -1)
  {
    method_val         = { };
    method_val.version = NVDRS_SETTING_VER;

    NVAPI_SET_DWORD (method_val, method_enum, method);
    NVAPI_CALL      (DRS_SetSetting (hSession, hProfile, &method_val));

    already_set = FALSE;
  }

  if (replay_mode_val.u32CurrentValue != replay_mode && replay_mode != -1)
  {
    replay_mode_val         = { };
    replay_mode_val.version = NVDRS_SETTING_VER;

    NVAPI_SET_DWORD (replay_mode_val, replay_mode_enum, replay_mode);
    NVAPI_CALL      (DRS_SetSetting (hSession, hProfile, &replay_mode_val));

    already_set = FALSE;
  }

  if (SK_IsAdmin ())
  {
    if (aa_fix_val.u32CurrentValue != aa_fix && aa_fix != -1)
    {
      aa_fix_val         = { };
      aa_fix_val.version = NVDRS_SETTING_VER;

      NVAPI_SET_DWORD (aa_fix_val, aa_fix_enum, aa_fix);
      NVAPI_CALL      (DRS_SetSetting (hSession, hProfile, &aa_fix_val));

      already_set = FALSE;
    }
  }

  else
  {
    dll_log.Log ( L"[  NvAPI   ] *** Cannot set NvDRS Profile Setting '%x' as a normal user, skipping...",
                    aa_fix_enum );
  }

  if (autobias_val.u32CurrentValue != auto_bias && auto_bias != -1)
  {
    autobias_val         = { };
    autobias_val.version = NVDRS_SETTING_VER;

    // NVIDIA drivers are stupid, some think this is an invalid enumerant
    NVAPI_SILENT    ();
    NVAPI_SET_DWORD (autobias_val, autobias_enum, auto_bias);
    NVAPI_CALL      (DRS_SetSetting (hSession, hProfile, &autobias_val));
    NVAPI_VERBOSE   ();

    already_set = FALSE;    
  }

  if (override_val.u32CurrentValue != override && override != -1)
  {
    override_val         = { };
    override_val.version = NVDRS_SETTING_VER;

    NVAPI_SET_DWORD (override_val, override_enum, override);
    NVAPI_CALL      (DRS_SetSetting (hSession, hProfile, &override_val));

    already_set = FALSE;
  }

  if (! already_set)
  {
#if 0
#ifndef _WIN64
    HMODULE hLib = LoadLibraryW_Original (L"nvapi.dll");
#else
    HMODULE hLib = LoadLibraryW_Original (L"nvapi64.dll");
#endif
#define __NvAPI_RestartDisplayDriver                      0xB4B26B65
    typedef void* (*NvAPI_QueryInterface_pfn)(unsigned int offset);
    typedef NvAPI_Status(__cdecl *NvAPI_RestartDisplayDriver_pfn)(void);
    NvAPI_QueryInterface_pfn       NvAPI_QueryInterface       =
      (NvAPI_QueryInterface_pfn)GetProcAddress (hLib, "nvapi_QueryInterface");
    NvAPI_RestartDisplayDriver_pfn NvAPI_RestartDisplayDriver =
      (NvAPI_RestartDisplayDriver_pfn)NvAPI_QueryInterface (__NvAPI_RestartDisplayDriver);

    NvAPI_RestartDisplayDriver ();
#endif

    NVAPI_CALL (DRS_SaveSettings (hSession));
  }

  NVAPI_CALL (DRS_DestroySession (hSession));

  LeaveCriticalSection (&cs_aa_override);

  return already_set;
}

// Easier to DLL export this way
BOOL
__stdcall
SK_NvAPI_SetFramerateLimit (uint32_t limit)
{
  UNREFERENCED_PARAMETER (limit);

  return TRUE;

#if 0
  // Allow the end-user to override this using the INI file
  if (config.render.framerate.target_fps != 0)
    limit = (uint32_t)config.render.framerate.target_fps;

  // Don't set anything up.
  if (limit == 0)
    return true;

  NvDRSSessionHandle hSession;
  NVAPI_CALL (DRS_CreateSession (&hSession));
  NVAPI_CALL (DRS_LoadSettings  (hSession));

  NvDRSProfileHandle hProfile;
  static NVDRS_APPLICATION app = { };

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
    NVDRS_PROFILE custom_profile = { };

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

  NVDRS_SETTING fps_limiter = { };
  fps_limiter.version = NVDRS_SETTING_VER;

  NVDRS_SETTING prerendered_frames = { };
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
    HMODULE hLib = LoadLibraryW_Original (L"nvapi.dll");
#else
    HMODULE hLib = LoadLibraryW+_Original (L"nvapi64.dll");
#endif
#define __NvAPI_RestartDisplayDriver                      0xB4B26B65
    typedef void* (*NvAPI_QueryInterface_pfn)(unsigned int offset);
    typedef NvAPI_Status(__cdecl *NvAPI_RestartDisplayDriver_pfn)(void);
    NvAPI_QueryInterface_pfn       NvAPI_QueryInterface       =
      (NvAPI_QueryInterface_pfn)GetProcAddress (hLib, "nvapi_QueryInterface");
    NvAPI_RestartDisplayDriver_pfn NvAPI_RestartDisplayDriver =
      (NvAPI_RestartDisplayDriver_pfn)NvAPI_QueryInterface (__NvAPI_RestartDisplayDriver);

    NvAPI_RestartDisplayDriver ();
  }

  return already_set;
#endif
}

// 0x00000003 AA Behavior (Override App)

BOOL
sk::NVAPI::SetAntiAliasingOverride ( const wchar_t** pwszPropertyList )
{
  return SK_NvAPI_SetAntiAliasingOverride ( pwszPropertyList );
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

  if (! nv_hardware)
    return FALSE;

  NvAPI_Status       ret       = NVAPI_ERROR;
  NvDRSSessionHandle hSession  = { };

  NVAPI_CALL (DRS_CreateSession (&hSession));
  NVAPI_CALL (DRS_LoadSettings  (hSession));

         NvDRSProfileHandle hProfile = { };
  static NVDRS_APPLICATION  app      = { };

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
  ret         = NVAPI_ERROR;

  NVAPI_CALL2 ( DRS_FindApplicationByName ( hSession,
                                              (NvU16 *)app_name.c_str (),
                                                &hProfile,
                                                  &app ),
                ret );

  // If no executable exists anywhere by this name, create a profile for it
  //   and then add the executable to it.
  if (ret == NVAPI_EXECUTABLE_NOT_FOUND)
  {
    NVDRS_PROFILE custom_profile = { };

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

    if (ret == NVAPI_OK)
    {
      app = { };

      lstrcpyW ((wchar_t *)app.appName,          app_name.c_str      ());
      lstrcpyW ((wchar_t *)app.userFriendlyName, friendly_name.c_str ());

      app.version      = NVDRS_APPLICATION_VER;
      app.isPredefined = false;
      app.isMetro      = false;

      NVAPI_CALL2 (DRS_CreateApplication (hSession, hProfile, &app), ret);
      NVAPI_CALL2 (DRS_SaveSettings      (hSession), ret);
    }
  }

  NVDRS_SETTING mode_val                = {               };
                mode_val.version        = NVDRS_SETTING_VER;

  NVDRS_SETTING gpu_count_val           = {               };
                gpu_count_val.version   = NVDRS_SETTING_VER;

  NVDRS_SETTING compat_bits_val         = {               };
                compat_bits_val.version = NVDRS_SETTING_VER;

  // These settings may not exist, and getting back a value of 0 is okay...
  NVAPI_SILENT ();
  NVAPI_CALL (DRS_GetSetting (hSession, hProfile, render_mode_enum, &mode_val));
  NVAPI_CALL (DRS_GetSetting (hSession, hProfile, gpu_count_enum,   &gpu_count_val));
  NVAPI_CALL (DRS_GetSetting (hSession, hProfile, compat_bits_enum, &compat_bits_val));
  NVAPI_VERBOSE ();

  BOOL already_set = TRUE;

  // Do this first so we don't touch other settings if this fails
  if (compat_bits_val.u32CurrentValue != compat_bits)
  {
    compat_bits_val         = {               };
    compat_bits_val.version = NVDRS_SETTING_VER;

    ret = NVAPI_ERROR;

    // This requires admin privs, and we will handle that gracefully...
    NVAPI_SILENT    ();
    NVAPI_SET_DWORD (compat_bits_val, compat_bits_enum, compat_bits);
    NVAPI_CALL2     (DRS_SetSetting (hSession, hProfile, &compat_bits_val), ret);
    NVAPI_VERBOSE   ();

    // Not running as admin, don't do the override!
    if (ret == NVAPI_INVALID_USER_PRIVILEGE)
    {
      int result = 
        MessageBox ( nullptr,
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

  if (mode_val.u32CurrentValue != render_mode)
  {
    mode_val         = { };
    mode_val.version = NVDRS_SETTING_VER;

    NVAPI_SET_DWORD (mode_val, render_mode_enum, render_mode);
    NVAPI_CALL      (DRS_SetSetting (hSession, hProfile, &mode_val));

    already_set = FALSE;
  }

  if (gpu_count_val.u32CurrentValue != gpu_count)
  {
    gpu_count_val         = {               };
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

  NvAPI_Status       ret      = NVAPI_ERROR;
  NvDRSSessionHandle hSession = {         };

  NVAPI_CALL (DRS_CreateSession (&hSession));
  NVAPI_CALL (DRS_LoadSettings  (hSession));

  NvDRSProfileHandle hProfile  = { };
  static NVDRS_APPLICATION app = { };

  //SK_NvAPI_GetAppProfile (hSession, &hProfile, &app);
  NVAPI_SILENT ();

  app.version = NVDRS_APPLICATION_VER;
  ret         = NVAPI_ERROR;

  NVAPI_CALL2 ( DRS_FindApplicationByName ( hSession,
                                              (NvU16 *)app_name.c_str (),
                                                &hProfile,
                                                  &app ),
                ret );

  // If no executable exists anywhere by this name, create a profile for it
  //   and then add the executable to it.
  if (ret == NVAPI_EXECUTABLE_NOT_FOUND)
  {
    NVDRS_PROFILE custom_profile = { };

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

    if (ret == NVAPI_OK)
    {
       app         = {   };
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
     app         = {   };
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
    HMODULE hLib = LoadLibraryW_Original (L"nvapi.dll");
#else
    HMODULE hLib = LoadLibraryW_Original (L"nvapi64.dll");
#endif
#define __NvAPI_RestartDisplayDriver                      0xB4B26B65
    typedef void* (*NvAPI_QueryInterface_pfn)(unsigned int offset);
    typedef NvAPI_Status(__cdecl *NvAPI_RestartDisplayDriver_pfn)(void);
    NvAPI_QueryInterface_pfn       NvAPI_QueryInterface       =
      (NvAPI_QueryInterface_pfn)GetProcAddress (hLib, "nvapi_QueryInterface");
    NvAPI_RestartDisplayDriver_pfn NvAPI_RestartDisplayDriver =
      (NvAPI_RestartDisplayDriver_pfn)NvAPI_QueryInterface (__NvAPI_RestartDisplayDriver);

    NvAPI_RestartDisplayDriver ();
  }
#endif

  return true;
}

bool         NVAPI::nv_hardware   = false;
std::wstring NVAPI::friendly_name = L"Tales of Zestiria";
std::wstring NVAPI::app_name      = L"Tales Of Zestiria.exe";
std::wstring NVAPI::launcher_name = L"";