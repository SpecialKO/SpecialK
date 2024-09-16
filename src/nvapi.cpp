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

#include <SpecialK/nvapi.h>
#include <nvapi/NvApiDriverSettings.h>

#include <SpecialK/render/dxgi/dxgi_hdr.h>

using NvAPI_Disp_GetHdrCapabilities_pfn = NvAPI_Status (__cdecl *)(NvU32, NV_HDR_CAPABILITIES*);
using NvAPI_Disp_HdrColorControl_pfn    = NvAPI_Status (__cdecl *)(NvU32, NV_HDR_COLOR_DATA*);
using NvAPI_Disp_ColorControl_pfn       = NvAPI_Status (__cdecl *)(NvU32, NV_COLOR_DATA*);

NvAPI_Disp_GetHdrCapabilities_pfn NvAPI_Disp_GetHdrCapabilities_Original = nullptr;
NvAPI_Disp_HdrColorControl_pfn    NvAPI_Disp_HdrColorControl_Original    = nullptr;
NvAPI_Disp_ColorControl_pfn       NvAPI_Disp_ColorControl_Original       = nullptr;

using NvAPI_Mosaic_GetDisplayViewportsByResolution_pfn = NvAPI_Status (__cdecl *)(NvU32 displayId, NvU32 srcWidth, NvU32 srcHeight, NV_RECT viewports[NV_MOSAIC_MAX_DISPLAYS], NvU8* bezelCorrected);

NvAPI_Mosaic_GetDisplayViewportsByResolution_pfn NvAPI_Mosaic_GetDisplayViewportsByResolution_Original = nullptr; 

//
// Undocumented Functions (unless you sign an NDA)
//
NvAPI_GPU_GetRamType_pfn            NvAPI_GPU_GetRamType            = nullptr;
NvAPI_GPU_GetFBWidthAndLocation_pfn NvAPI_GPU_GetFBWidthAndLocation = nullptr;
NvAPI_GPU_GetPCIEInfo_pfn           NvAPI_GPU_GetPCIEInfo           = nullptr;
NvAPI_GetGPUIDFromPhysicalGPU_pfn   NvAPI_GetGPUIDFromPhysicalGPU   = nullptr;
NvAPI_Disp_SetDitherControl_pfn     NvAPI_Disp_SetDitherControl     = nullptr;
NvAPI_Disp_GetDitherControl_pfn     NvAPI_Disp_GetDitherControl     = nullptr;

struct NVAPI_ThreadSafety {
  struct {
    SK_Thread_HybridSpinlock Init;
    SK_Thread_HybridSpinlock QueryInterface;
    SK_Thread_HybridSpinlock DISP_GetMonitorCapabilities;
    SK_Thread_HybridSpinlock Disp_ColorControl;
    SK_Thread_HybridSpinlock Disp_HdrColorControl;
    SK_Thread_HybridSpinlock Disp_GetHdrCapabilities;
    SK_Thread_HybridSpinlock Disp_GetVRRInfo;
    SK_Thread_HybridSpinlock DISP_GetAdaptiveSyncData;
    SK_Thread_HybridSpinlock DISP_SetAdaptiveSyncData;
    SK_Thread_HybridSpinlock D3D_IsGSyncCapable;
    SK_Thread_HybridSpinlock D3D_IsGSyncActive;
  } locks;
};

SK_LazyGlobal <NVAPI_ThreadSafety> SK_NvAPI_Threading;


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
                               (x).settingLocation =                  \
                                 NVDRS_DEFAULT_PROFILE_LOCATION;      \
                               (x).settingId = (y); (x).settingType = \
                                 NVDRS_DWORD_TYPE;                    \
                               (x).u32CurrentValue = (z);             \
                               (x).isCurrentPredefined = 0;           \
                               (x).isPredefinedValid = 0;


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

SK_LazyGlobal <std::array <DXGI_ADAPTER_DESC, 64>> _nv_sli_adapters;

DXGI_ADAPTER_DESC*
NVAPI::EnumSLIGPUs (void)
{
  static int nv_sli_count = -1;

  *_nv_sli_adapters.get ()[0].Description = L'\0';

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
          memcpy (&_nv_sli_adapters.get ()[nv_sli_count++], adapters,
                 sizeof (DXGI_ADAPTER_DESC));

        ++adapters;

        if (*adapters->Description == '\0')
          break;
      }

      *_nv_sli_adapters.get ()[nv_sli_count].Description = L'\0';
    }
  }

  return
    &_nv_sli_adapters.get ()[0];
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

    NV_GPU_MEMORY_INFO_EX
      meminfo         = { };
      meminfo.version = NV_GPU_MEMORY_INFO_EX_VER_1;

    NvAPI_GPU_GetMemoryInfoEx (_nv_dxgi_gpus [i], &meminfo);

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

  if (wszFixedName != nullptr)
  {
    // Remove trailing whitespace some NV GPUs inexplicably have in their names
    for (int i = fixed_len; i > 0; i--) {
      if (wszFixedName [i - 1] == L' ')
        wszFixedName [i - 1] = L'\0';
      else
        break;
    }
  }

  while (*adapters->Description != L'\0')
  {
    //strstrw (lstrcmpiW () == 0) { // More accurate, but some GPUs have
                                    //   trailing spaces in their names.
                                    // -----
                                    // What the heck?!

    if (                               wszFixedName  != nullptr &&
        wcsstr (adapters->Description, wszFixedName) != nullptr)
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

  // The driver-branch string's not particularly user friendly,
  //   let's do this the right way and report a number the end-user
  //     is actually going to recognize...
  _snwprintf (ver_wstr, 63, L"%03u.%02u", ver / 100, ver % 100);
              ver_wstr [63] = L'\0';

  if (pVer != nullptr)
    *pVer = ver;

  return ver_wstr;
}

NvU32
SK_NvAPI_GetDefaultDisplayId (void)
{
  static NvU32 default_display_id =
    std::numeric_limits <NvU32>::max ();

  if (default_display_id != std::numeric_limits <NvU32>::max ())
    return default_display_id;

  NvU32               gpuCount                        =  0 ;
  NvPhysicalGpuHandle ahGPU [NVAPI_MAX_PHYSICAL_GPUS] = { };

  if (NVAPI_OK == NvAPI_EnumPhysicalGPUs (ahGPU, &gpuCount))
  {
    for (NvU32 i = 0; i < gpuCount; ++i)
    {
      NvU32             displayIdCount      =  16;
      NvU32             flags               =  0 ;
      NV_GPU_DISPLAYIDS
        displayIdArray [16]         = {                   };
        displayIdArray [ 0].version = NV_GPU_DISPLAYIDS_VER;

      // Query list of displays connected to this GPU
      if (NVAPI_OK == NvAPI_GPU_GetConnectedDisplayIds (
           (const NvPhysicalGpuHandle) ahGPU [i],
                                  displayIdArray,
                                 &displayIdCount, flags))
      {
        if (displayIdArray [0].isActive)
        {
          default_display_id = displayIdArray [0].displayId;
          break;
        }
      }

      // Iterate over displays to test for HDR capabilities
    }
  }

  return 0;
}



#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"  Nv API  "

NvAPI_Status
__cdecl
NvAPI_Mosaic_GetDisplayViewportsByResolution_Override ( NvU32   displayId,
                                                        NvU32   srcWidth,
                                                        NvU32   srcHeight,
                                                        NV_RECT viewports [NV_MOSAIC_MAX_DISPLAYS],
                                                        NvU8*   bezelCorrected )
{
  SK_LOG_FIRST_CALL

#ifndef ALLOW_NVAPI_RESOLUTION_QUERY
  return NVAPI_NOT_SUPPORTED;
#endif

  return
    NvAPI_Mosaic_GetDisplayViewportsByResolution_Original (displayId, srcWidth, srcHeight, viewports, bezelCorrected);
}

NvAPI_Status
__cdecl
NvAPI_Disp_GetHdrCapabilities_Override ( NvU32                displayId,
                                         NV_HDR_CAPABILITIES *pHdrCapabilities )
{
  SK_LOG_FIRST_CALL

  if (config.apis.NvAPI.disable_hdr)
    return NVAPI_NO_IMPLEMENTATION;

  // We don't want it, pass it straight to NvAPI
  if (pHdrCapabilities == nullptr)
    return NvAPI_Disp_GetHdrCapabilities_Original ( displayId, pHdrCapabilities );

  SK_ReleaseAssert (
    pHdrCapabilities->version <= NV_HDR_CAPABILITIES_VER3
  );

  std::lock_guard
       lock (SK_NvAPI_Threading->locks.Disp_GetHdrCapabilities);

  SK_LOG0 ( ( L"NV_HDR_CAPABILITIES Version: %lu", pHdrCapabilities->version ),
              __SK_SUBSYSTEM__ );
  SK_LOG0 ( ( L" >> Wants Driver to Expand Default HDR Params: %s",
                pHdrCapabilities->driverExpandDefaultHdrParameters ? L"Yes" :
                                                                     L"No" ),
              __SK_SUBSYSTEM__ );

  // We don't care what the game wants, we're filling-in default values dammit!
  pHdrCapabilities->driverExpandDefaultHdrParameters = true;

  NvAPI_Status ret =
    NvAPI_Disp_GetHdrCapabilities_Original ( displayId, pHdrCapabilities );

  const SK_RenderBackend_V2::output_s* const pBackendDisplay =
    SK_GetCurrentRenderBackend ().displays->nvapi.getDisplayFromId (displayId);

  dll_log->LogEx ( true,
      L"[ HDR Caps ]\n"
      L"  +-----------------+---------------------\n"
      L"  | Red Primary.... |  %f, %f\n"
      L"  | Green Primary.. |  %f, %f\n"
      L"  | Blue Primary... |  %f, %f\n"
      L"  | White Point.... |  %f, %f\n"
      L"  | =============== |\n"
      L"  | Min Luminance.. |  %10.5f cd/m²\n"
      L"  | Max Luminance.. |  %10.5f cd/m²\n"
      L"  |  |- FullFrame.. |  %10.5f cd/m²\n"
      L"  | SDR Luminance.. |  %10.5f cd/m²\n"
      L"  | =============== |\n"
      L"  | ST2084 EOTF.... |  %s\n"
      L"  | CTA-861.3 HDR.. |  %s\n"
      L"  | CTA-861.3 SDR.. |  %s\n"
      L"  | =============== |\n"
      L"  | HDR10+......... |  %s\n"
      L"  | HDR10+ Gaming.. |  %s\n"
      L"  | =============== |\n"
      L"  | Dolby Vision... |  %s\n"
      L"  | YUV 4:2:2 12bpc |  %s\n"
      L"  | Low Latency.... |  %s\n"
      L"  |  |- 4:4:4 10bpc |  %s\n"
      L"  |  |- 4:4:4 12bpc |  %s\n"
      L"  +-----------------+---------------------\n",
        (float)pHdrCapabilities->display_data.displayPrimary_x0   / (float)0xC350, (float)pHdrCapabilities->display_data.displayPrimary_y0   / (float)0xC350,
        (float)pHdrCapabilities->display_data.displayPrimary_x1   / (float)0xC350, (float)pHdrCapabilities->display_data.displayPrimary_y1   / (float)0xC350,
        (float)pHdrCapabilities->display_data.displayPrimary_x2   / (float)0xC350, (float)pHdrCapabilities->display_data.displayPrimary_y2   / (float)0xC350,
        (float)pHdrCapabilities->display_data.displayWhitePoint_x / (float)0xC350, (float)pHdrCapabilities->display_data.displayWhitePoint_y / (float)0xC350,
        (float)pHdrCapabilities->display_data.desired_content_min_luminance * 0.0001f,
        (float)pHdrCapabilities->display_data.desired_content_max_luminance,
        (float)pHdrCapabilities->display_data.desired_content_max_frame_average_luminance,
               pBackendDisplay != nullptr ?
               pBackendDisplay->hdr.white_level : 
        (float)pHdrCapabilities->display_data.desired_content_max_frame_average_luminance,
               pHdrCapabilities->isST2084EotfSupported                          ? L"Yes" : L"No",
               pHdrCapabilities->isTraditionalHdrGammaSupported                 ? L"Yes" : L"No",
               pHdrCapabilities->isTraditionalSdrGammaSupported                 ? L"Yes" : L"No",
               pHdrCapabilities->isHdr10PlusSupported                           ? L"Yes" : L"No",
               pHdrCapabilities->isHdr10PlusGamingSupported                     ? L"Yes" : L"No",
               pHdrCapabilities->isDolbyVisionSupported                         ? L"Yes" : L"No",
               pHdrCapabilities->dv_static_metadata.supports_YUV422_12bit       ? L"Yes" : L"No",
              (pHdrCapabilities->dv_static_metadata.interface_supported_by_sink
                                                                         & 0x2) ? L"Yes" : L"No",
              (pHdrCapabilities->dv_static_metadata.supports_10b_12b_444 & 0x1) ? L"Yes" : L"No",
              (pHdrCapabilities->dv_static_metadata.supports_10b_12b_444 & 0x2) ? L"Yes" : L"No");

  return ret;
}

SK_LazyGlobal < std::pair < NvAPI_Status,
                std::pair < NvU32,
                            NV_HDR_COLOR_DATA > > >
SK_NvAPI_LastHdrColorControl;

void
SK_NvAPI_ReIssueLastHdrColorControl (void)
{
  if (SK_NvAPI_LastHdrColorControl->first == NVAPI_OK)
  {
    auto& reissue =
       SK_NvAPI_LastHdrColorControl->second;
    //auto off_copy         = reissue.second;
    //     off_copy.cmd     = NV_HDR_CMD_SET;
    //     off_copy.hdrMode = NV_HDR_MODE_OFF;
    //NvAPI_Disp_HdrColorControl_Original (reissue.first, &off_copy);
    NvAPI_Disp_HdrColorControl (reissue.first, &reissue.second);
  }
}


NvAPI_Status
__cdecl
NvAPI_Disp_ColorControl_Override ( NvU32          displayId,
                                   NV_COLOR_DATA *pColorData )
{
  std::lock_guard
       lock (SK_NvAPI_Threading->locks.Disp_ColorControl);

  SK_LOG_CALL (SK_FormatStringW (L"NvAPI_Disp_ColorControl (%lu, ...)", displayId).c_str ());

  return
    NvAPI_Disp_ColorControl_Original (displayId, pColorData);
}

NvAPI_Status
SK_NvAPI_CheckColorSupport_SDR (
  NvU32                displayId,
  NV_BPC               bpcTest,
  NV_COLOR_FORMAT      formatTest      = NV_COLOR_FORMAT_AUTO,
  NV_DYNAMIC_RANGE     rangeTest       = NV_DYNAMIC_RANGE_AUTO,
  NV_COLOR_COLORIMETRY colorimetryTest = NV_COLOR_COLORIMETRY_AUTO )
{
  if (displayId == 0)
  {
    displayId =
      SK_NvAPI_GetDefaultDisplayId ();
  }

  NV_COLOR_DATA
    colorCheck                   = { };
    colorCheck.version           = NV_COLOR_DATA_VER;
    colorCheck.size              = sizeof (NV_COLOR_DATA);
    colorCheck.cmd               = NV_COLOR_CMD_IS_SUPPORTED_COLOR;
    colorCheck.data.colorimetry  = (NvU8)colorimetryTest;
    colorCheck.data.dynamicRange = (NvU8)rangeTest;
    colorCheck.data.colorFormat  = (NvU8)formatTest;
    colorCheck.data.bpc          = bpcTest;

  return
    NvAPI_Disp_ColorControl_Original ( displayId, &colorCheck );
}

NvAPI_Status
SK_NvAPI_CheckColorSupport_HDR (
  NvU32                displayId,
  NV_BPC               bpcTest,
  NV_COLOR_FORMAT      formatTest      = NV_COLOR_FORMAT_AUTO,
  NV_DYNAMIC_RANGE     rangeTest       = NV_DYNAMIC_RANGE_AUTO,
  NV_COLOR_COLORIMETRY colorimetryTest = NV_COLOR_COLORIMETRY_AUTO )
{
  if (displayId == 0)
  {
    displayId =
      SK_NvAPI_GetDefaultDisplayId ();
  }

  NV_COLOR_DATA
    colorCheck                   = { };
    colorCheck.version           = NV_COLOR_DATA_VER;
    colorCheck.size              = sizeof (NV_COLOR_DATA);
    colorCheck.cmd               = NV_COLOR_CMD_IS_SUPPORTED_COLOR;
    colorCheck.data.colorimetry  = (NvU8)colorimetryTest;
    colorCheck.data.dynamicRange = (NvU8)rangeTest;
    colorCheck.data.colorFormat  = (NvU8)formatTest;
    colorCheck.data.bpc          =       bpcTest;

  return
    NvAPI_Disp_ColorControl_Original ( displayId, &colorCheck );
}


NvAPI_Status
__cdecl
NvAPI_Disp_HdrColorControl_Override ( NvU32              displayId,
                                      NV_HDR_COLOR_DATA *pHdrColorData )
{
  SK_LOG_FIRST_CALL

  if (config.apis.NvAPI.disable_hdr)
    return NVAPI_NO_IMPLEMENTATION;

  // We don't want it, pass it straight to NvAPI
  if (pHdrColorData == nullptr)
    return NvAPI_Disp_HdrColorControl_Original (displayId, pHdrColorData);

  std::lock_guard
       lock (SK_NvAPI_Threading->locks.Disp_HdrColorControl);

  if (pHdrColorData->version != NV_HDR_COLOR_DATA_VER2 &&
      pHdrColorData->version != NV_HDR_COLOR_DATA_VER1)
  {
    SK_LOGi0 (
      L"HDR: NvAPI_Disp_HdrColorControl (...) called using a struct version (%d) SK does not understand",
                pHdrColorData->version );

    if (__SK_HDR_10BitSwap || __SK_HDR_16BitSwap)
    {
      SK_LOG0 ( ( L"HDR:  << Ignoring NvAPI HDR because user is forcing DXGI HDR (which is better!)"
                ), __SK_SUBSYSTEM__ );

      return NVAPI_OK;
    }

    if (SK_IsModuleLoaded (L"vulkan-1.dll"))
    {
      SK_LOGi0 (L"Disabling D3D12/Vulkan Interop in favor of D3D11/Vulkan Interop.");

      config.compatibility.disable_dx12_vk_interop = true;

      return NVAPI_OK;
    }

    return
      NvAPI_Disp_HdrColorControl_Original (displayId, pHdrColorData);
  }

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  static NV_HDR_COLOR_DATA_V2  expandedData = { };
         NV_HDR_COLOR_DATA_V1   *inputData    =
        (NV_HDR_COLOR_DATA_V1   *)pHdrColorData;

  if (pHdrColorData->version == NV_HDR_COLOR_DATA_VER1)
  {
    NV_HDR_COLOR_DATA_V1    origData =     *((NV_HDR_COLOR_DATA_V1 *)pHdrColorData);
    memcpy (&expandedData, &origData, sizeof (NV_HDR_COLOR_DATA_V1));

    expandedData.version   = NV_HDR_COLOR_DATA_VER2;
    pHdrColorData          = &expandedData;

    pHdrColorData->cmd                           = inputData->cmd;
    pHdrColorData->hdrMode                       = inputData->hdrMode;
    pHdrColorData->static_metadata_descriptor_id = inputData->static_metadata_descriptor_id;

    //// std::bit_cast ?
    memcpy ( &pHdrColorData->mastering_display_data,
                 &inputData->mastering_display_data,
          sizeof (inputData->mastering_display_data) );
  }

  if (pHdrColorData->version == NV_HDR_COLOR_DATA_VER2)
  {
    auto& hdr_sec =
      SK_GetDLLConfig ()->get_section (L"NvAPI.HDR");

    auto& hdrBpc =
      hdr_sec.get_value (L"hdrBpc");

    auto& hdrColorFormat =
      hdr_sec.get_value (L"hdrColorFormat");

    auto& hdrDynamicRange =
      hdr_sec.get_value (L"hdrDynamicRange");

    static std::unordered_map <std::wstring, _NV_BPC>
      bpc_map =
        { { L"NV_BPC_DEFAULT", NV_BPC_DEFAULT },
          { L"NV_BPC_6",       NV_BPC_6       },
          { L"NV_BPC_8",       NV_BPC_8       },
          { L"NV_BPC_10",      NV_BPC_10      },
          { L"NV_BPC_12",      NV_BPC_12      },
          { L"NV_BPC_16",      NV_BPC_16      } };

    static std::unordered_map <std::wstring, NV_COLOR_FORMAT>
      color_fmt_map =
        { { L"NV_COLOR_FORMAT_RGB",     NV_COLOR_FORMAT_RGB     },
          { L"NV_COLOR_FORMAT_YUV422",  NV_COLOR_FORMAT_YUV422  },
          { L"NV_COLOR_FORMAT_YUV444",  NV_COLOR_FORMAT_YUV444  },
          { L"NV_COLOR_FORMAT_YUV420",  NV_COLOR_FORMAT_YUV420  },
          { L"NV_COLOR_FORMAT_DEFAULT", NV_COLOR_FORMAT_DEFAULT },
          { L"NV_COLOR_FORMAT_AUTO",    NV_COLOR_FORMAT_AUTO    } };

    static std::unordered_map <std::wstring, _NV_DYNAMIC_RANGE>
      dynamic_range_map =
        { { L"NV_DYNAMIC_RANGE_VESA", NV_DYNAMIC_RANGE_VESA  },
          { L"NV_DYNAMIC_RANGE_CEA",  NV_DYNAMIC_RANGE_CEA   },
          { L"NV_DYNAMIC_RANGE_AUTO", NV_DYNAMIC_RANGE_AUTO  } };

    if (! hdrBpc.empty ())
      pHdrColorData->hdrBpc = bpc_map [hdrBpc];

    if (! hdrColorFormat.empty ())
      pHdrColorData->hdrColorFormat = color_fmt_map [hdrColorFormat];

    if (! hdrDynamicRange.empty ())
      pHdrColorData->hdrDynamicRange = dynamic_range_map [hdrDynamicRange];

    //if (pHdrColorData->hdrMode == NV_HDR_MODE_UHDA_PASSTHROUGH)
    //    pHdrColorData->hdrMode =  NV_HDR_MODE_UHDA;
  }

  //SK_LOG0 ( ( L"NV_HDR_COLOR_DATA Version: %lu", pHdrColorData->version ),
  //            __SK_SUBSYSTEM__ );
  SK_LOG0 ( ( L"<%s> %hs%hs",pHdrColorData->cmd == NV_HDR_CMD_GET    ?
                                                        L"Get"                 :
                                       pHdrColorData->cmd == NV_HDR_CMD_SET    ?
                                                        L"Set"                 : L"Unknown",
  pHdrColorData->cmd == NV_HDR_CMD_SET ? "HDR Mode:    "                       : " ",
  pHdrColorData->cmd == NV_HDR_CMD_SET ? HDRModeToStr (pHdrColorData->hdrMode) : " " ),
              __SK_SUBSYSTEM__ );

  void SK_HDR_RunWidgetOnce (void);
       SK_HDR_RunWidgetOnce ();

  const bool sk_is_overriding_hdr =
      (__SK_HDR_UserForced);
  bool game_is_engaging_native_hdr = false;

  if ( pHdrColorData->cmd     == NV_HDR_CMD_SET &&
       pHdrColorData->hdrMode == NV_HDR_MODE_UHDA_PASSTHROUGH )
  {
    if (rb.isHDRCapable ())
    {
      game_is_engaging_native_hdr = true;

      if (! sk_is_overriding_hdr)
      {
        __SK_HDR_16BitSwap = false;
        __SK_HDR_10BitSwap =  true;

        __SK_HDR_Preset  = 3;
        __SK_HDR_tonemap = SK_HDR_TONEMAP_RAW_IMAGE;

        if (SK_ComQIPtr <IDXGISwapChain3> pSwap3 (rb.swapchain);
                                          pSwap3 != nullptr)
        {
          pSwap3->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
        }
      }
    }
  }

  else if ( pHdrColorData->cmd     == NV_HDR_CMD_SET &&
            pHdrColorData->hdrMode == NV_HDR_MODE_UHDA )
  {
    if (rb.isHDRCapable ())
    {
      game_is_engaging_native_hdr = true;

      if (! sk_is_overriding_hdr)
      {
        __SK_HDR_10BitSwap = false;
        __SK_HDR_16BitSwap = true;

        __SK_HDR_Preset       = 2;
        __SK_HDR_Content_EOTF = 1.0f;

        if (SK_ComQIPtr <IDXGISwapChain3> pSwap3 (rb.swapchain);
                                          pSwap3 != nullptr)
        {
          pSwap3->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709);
        }
      }
    }
  }

  else if (pHdrColorData->cmd == NV_HDR_CMD_SET)
  {
    if (! sk_is_overriding_hdr)
    {
      __SK_HDR_16BitSwap = false;
      __SK_HDR_10BitSwap = false;

      if (SK_ComQIPtr <IDXGISwapChain3> pSwap3 (rb.swapchain);
                                        pSwap3 != nullptr)
      {
        pSwap3->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
      }
    }
  }

  if (game_is_engaging_native_hdr)
  {
    SK_HDR_RunWidgetOnce ();

    if (! sk_is_overriding_hdr)
      __SK_HDR_tonemap = SK_HDR_TONEMAP_RAW_IMAGE;
  }


  NV_COLOR_DATA
  colorCheck                   = { };
  colorCheck.version           = NV_COLOR_DATA_VER;
  colorCheck.size              = sizeof (NV_COLOR_DATA);
  colorCheck.cmd               = NV_COLOR_CMD_IS_SUPPORTED_COLOR;
  colorCheck.data.colorimetry  = NV_COLOR_COLORIMETRY_AUTO;
  colorCheck.data.dynamicRange = NV_DYNAMIC_RANGE_AUTO;
  colorCheck.data.colorFormat  = NV_COLOR_FORMAT_RGB;
  colorCheck.data.bpc          = NV_BPC_10;


  if ( NVAPI_OK ==
        SK_NvAPI_CheckColorSupport_HDR (displayId, NV_BPC_10, NV_COLOR_FORMAT_YUV444) )
    rb.scanout.nvapi_hdr.color_support_hdr.supports_10b_12b_444 |=  0x1;
  else
    rb.scanout.nvapi_hdr.color_support_hdr.supports_10b_12b_444 &= ~0x1;

  if ( NVAPI_OK ==
        SK_NvAPI_CheckColorSupport_HDR (displayId, NV_BPC_12, NV_COLOR_FORMAT_YUV444) )
    rb.scanout.nvapi_hdr.color_support_hdr.supports_10b_12b_444 |=   0x2;
  else
    rb.scanout.nvapi_hdr.color_support_hdr.supports_10b_12b_444 &=  ~0x2;

  if ( NVAPI_OK ==
         SK_NvAPI_CheckColorSupport_HDR (displayId, NV_BPC_12, NV_COLOR_FORMAT_YUV422) )
    rb.scanout.nvapi_hdr.color_support_hdr.supports_YUV422_12bit = 1;
  else
    rb.scanout.nvapi_hdr.color_support_hdr.supports_YUV422_12bit = 0;


  auto _LogGameRequestedValues = [&](void) ->
  void
  {
    static SKTL_BidirectionalHashMap <std::wstring, _NV_BPC>
      bpc_map =
        { { L"Default bpc", NV_BPC_DEFAULT },
          { L"6 bpc",       NV_BPC_6       },
          { L"8 bpc",       NV_BPC_8       },
          { L"10 bpc",      NV_BPC_10      },
          { L"12 bpc",      NV_BPC_12      },
          { L"16 bpc",      NV_BPC_16      } };

    static SKTL_BidirectionalHashMap <std::wstring, NV_COLOR_FORMAT>
      color_fmt_map =
        { { L"RGB 4:4:4",  NV_COLOR_FORMAT_RGB     },
          { L"YUV 4:2:2",  NV_COLOR_FORMAT_YUV422  },
          { L"YUV 4:4:4",  NV_COLOR_FORMAT_YUV444  },
          { L"YUV 4:2:0",  NV_COLOR_FORMAT_YUV420  },
          { L"(Default?)", NV_COLOR_FORMAT_DEFAULT },
          { L"[AUTO]",     NV_COLOR_FORMAT_AUTO    } };

    static SKTL_BidirectionalHashMap <std::wstring, _NV_DYNAMIC_RANGE>
      dynamic_range_map =
        { { L"Full Range", NV_DYNAMIC_RANGE_VESA  },
          { L"Limited",    NV_DYNAMIC_RANGE_CEA   },
          { L"Don't Care", NV_DYNAMIC_RANGE_AUTO  } };

    SK_LOGi1 ( L"HDR:  Mode: %hs", HDRModeToStr (pHdrColorData->hdrMode) );

    SK_LOG1 ( ( L"HDR:  Max Master Luma: %7.1f, Min Master Luma: %7.5f",
      static_cast <double> (pHdrColorData->mastering_display_data.max_display_mastering_luminance),
      static_cast <double> (
        static_cast <double>
                          (pHdrColorData->mastering_display_data.min_display_mastering_luminance) * 0.0001
                          )
              ), __SK_SUBSYSTEM__ );

    SK_LOG1 ( ( L"HDR:  Max Avg. Luma: %7.1f, Max Luma: %7.1f",
      static_cast <double> (pHdrColorData->mastering_display_data.max_frame_average_light_level),
      static_cast <double> (pHdrColorData->mastering_display_data.max_content_light_level)
              ), __SK_SUBSYSTEM__ );

    SK_LOG1 ( ( L"HDR:  Color ( Bit-Depth: %s, Sampling: %s ), Dynamic Range: %s",
                bpc_map           [pHdrColorData->hdrBpc].         c_str (),
                color_fmt_map     [pHdrColorData->hdrColorFormat]. c_str (),
                dynamic_range_map [pHdrColorData->hdrDynamicRange].c_str ()
              ), __SK_SUBSYSTEM__ );
  };

  auto _Push_NvAPI_HDR_Metadata_to_DXGI_Backend = [&](void) ->
  void
  {
    rb.working_gamut.maxY =
     (float)pHdrColorData->mastering_display_data.max_display_mastering_luminance;
    rb.working_gamut.minY =
     (float)pHdrColorData->mastering_display_data.min_display_mastering_luminance / 1000.0f;

    rb.working_gamut.maxLocalY   =
     (float)pHdrColorData->mastering_display_data.max_content_light_level;
    rb.working_gamut.maxAverageY =
     (float)pHdrColorData->mastering_display_data.max_frame_average_light_level;

    rb.working_gamut.xr = (float)pHdrColorData->mastering_display_data.displayPrimary_x0 /
                          (float)50000.0f;
    rb.working_gamut.xg = (float)pHdrColorData->mastering_display_data.displayPrimary_x1 /
                          (float)50000.0f;
    rb.working_gamut.xb = (float)pHdrColorData->mastering_display_data.displayPrimary_x2 /
                          (float)50000.0f;

    rb.working_gamut.yr = (float)pHdrColorData->mastering_display_data.displayPrimary_y0 /
                          (float)50000.0f;
    rb.working_gamut.yg = (float)pHdrColorData->mastering_display_data.displayPrimary_y1 /
                          (float)50000.0f;
    rb.working_gamut.yb = (float)pHdrColorData->mastering_display_data.displayPrimary_y2 /
                          (float)50000.0f;

    rb.working_gamut.Xw = (float)pHdrColorData->mastering_display_data.displayWhitePoint_x /
                          (float)50000.0f;
    rb.working_gamut.Yw = (float)pHdrColorData->mastering_display_data.displayWhitePoint_y /
                          (float)50000.0f;
    rb.working_gamut.Zw = 1.0f;
  };

  if (pHdrColorData->cmd == NV_HDR_CMD_SET)
  {
    _LogGameRequestedValues ();

    bool passthrough = (pHdrColorData->hdrMode == NV_HDR_MODE_UHDA_PASSTHROUGH);

    if (__SK_HDR_16BitSwap && passthrough)
    {
      //pHdrColorData->mastering_display_data.max_content_light_level = 1499;
      //pHdrColorData->mastering_display_data.max_frame_average_light_level = 750;

      pHdrColorData->hdrMode = NV_HDR_MODE_UHDA;
    }

    //struct DisplayChromacities
    //{
    //  float RedX;
    //  float RedY;
    //  float GreenX;
    //  float GreenY;
    //  float BlueX;
    //  float BlueY;
    //  float WhiteX;
    //  float WhiteY;
    //} rec2020 = { 0.70800f, 0.29200f,
    //              0.17000f, 0.79700f,
    //              0.13100f, 0.04600f,
    //              0.31270f, 0.32900f };
    //
    //DXGI_HDR_METADATA_HDR10 HDR10MetaData = { };
    //
    //HDR10MetaData.RedPrimary   [0] = static_cast <uint16_t>(rec2020.RedX   * 50000.0f);
    //HDR10MetaData.RedPrimary   [1] = static_cast <uint16_t>(rec2020.RedY   * 50000.0f);
    //HDR10MetaData.GreenPrimary [0] = static_cast <uint16_t>(rec2020.GreenX * 50000.0f);
    //HDR10MetaData.GreenPrimary [1] = static_cast <uint16_t>(rec2020.GreenY * 50000.0f);
    //HDR10MetaData.BluePrimary  [0] = static_cast <uint16_t>(rec2020.BlueX  * 50000.0f);
    //HDR10MetaData.BluePrimary  [1] = static_cast <uint16_t>(rec2020.BlueY  * 50000.0f);
    //HDR10MetaData.WhitePoint   [0] = static_cast <uint16_t>(rec2020.WhiteX * 50000.0f);
    //HDR10MetaData.WhitePoint   [1] = static_cast <uint16_t>(rec2020.WhiteY * 50000.0f);
    //HDR10MetaData.MaxMasteringLuminance     = static_cast <uint32_t>(100000.0f * 10000.0f);
    //HDR10MetaData.MinMasteringLuminance     = static_cast <uint32_t>(0.001f    * 10000.0f);
    //HDR10MetaData.MaxContentLightLevel      = static_cast <uint16_t>(2000);
    //HDR10MetaData.MaxFrameAverageLightLevel = static_cast <uint16_t>(500);
    //
    //if (rb.swapchain.p != nullptr && passthrough)
    //{
    //  reinterpret_cast <IDXGISwapChain4*> (rb.swapchain.p)->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
    //  reinterpret_cast <IDXGISwapChain4*> (rb.swapchain.p)->SetHDRMetaData (DXGI_HDR_METADATA_TYPE_HDR10, sizeof (HDR10MetaData), &HDR10MetaData);
    //}
    //
    //else
    //  passthrough = false;


    //pHdrColorData->mastering_display_data.max_display_mastering_luminance =
    //  rb.display_gamut.maxY > 65535.0f ? 0xFFFF :
    //  rb.display_gamut.maxY <     1.0f ? 0x0001 :
    //            static_cast <uint16_t> (rb.display_gamut.maxY);
    //pHdrColorData->mastering_display_data.min_display_mastering_luminance =
    //  (rb.display_gamut.minY <     1.0f) ? 0x0001 :
    //  (rb.display_gamut.minY > 6.55350f) ? 0xFFFF :
    //              static_cast <uint16_t> (rb.display_gamut.minY / 0.0001f);
    //
    //pHdrColorData->mastering_display_data.max_frame_average_light_level =
    //  rb.display_gamut.maxLocalY > 65535.0f ? 0xFFFF :
    //  rb.display_gamut.maxLocalY <     1.0f ? 0x0001 :
    //                 static_cast <uint16_t> (rb.display_gamut.maxLocalY);
    //pHdrColorData->mastering_display_data.max_content_light_level =
    //  rb.display_gamut.maxY > 65535.0f ? 0xFFFF :
    //  rb.display_gamut.maxY <     1.0f ? 0x0001 :
    //            static_cast <uint16_t> (rb.display_gamut.maxY);

    //pHdrColorData->hdrMode = NV_HDR_MODE_EDR;
    //pHdrColorData->static_metadata_descriptor_id =
    //                         NV_STATIC_METADATA_TYPE_1;
    //pHdrColorData->version = NV_HDR_COLOR_DATA_VER;

    NvAPI_Status ret =
      NVAPI_OK;

    if (__SK_HDR_10BitSwap || __SK_HDR_16BitSwap)
    {
      SK_LOG0 ( ( L"HDR:  << Ignoring NvAPI HDR because DXGI HDR is better"
                ), __SK_SUBSYSTEM__ );
    }

    else
      ret =
        NvAPI_Disp_HdrColorControl_Original ( displayId, pHdrColorData );

    if (NVAPI_OK == ret)
    {
      rb.scanout.nvapi_hdr.raw.display_id = displayId;

      if (pHdrColorData->hdrMode != NV_HDR_MODE_OFF)
      {
        rb.setHDRCapable       (true);
        rb.driver_based_hdr   = true;
        rb.framebuffer_flags |=  SK_FRAMEBUFFER_FLAG_HDR;

        *SK_NvAPI_LastHdrColorControl =
          std::make_pair (ret, std::make_pair (displayId, *pHdrColorData));

        rb.scanout.nvapi_hdr.mode = pHdrColorData->hdrMode;

        //if (rb.swapchain.p != nullptr)
        //{
        //  reinterpret_cast <IDXGISwapChain*> (rb.swapchain.p)->ResizeBuffers (0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        //}

        NV_HDR_COLOR_DATA query_data         = { };
                          query_data.version = NV_HDR_COLOR_DATA_VER;
                          query_data.cmd     = NV_HDR_CMD_GET;

        NvAPI_Disp_HdrColorControl_Original (displayId, &query_data);

        rb.scanout.nvapi_hdr.color_format   = query_data.hdrColorFormat;
        rb.scanout.nvapi_hdr.dynamic_range  = query_data.hdrDynamicRange;
        rb.scanout.nvapi_hdr.bpc            = query_data.hdrBpc;

        rb.scanout.nvapi_hdr.active         =
       (rb.scanout.nvapi_hdr.mode != NV_HDR_MODE_OFF);

        memcpy ( &rb.scanout.nvapi_hdr.raw.hdr_data,
                   &query_data, sizeof (NV_HDR_COLOR_DATA) );
      }

      else
      {
        rb.driver_based_hdr         = false;
        rb.scanout.nvapi_hdr.active = false;

        if (__SK_HDR_10BitSwap == false &&
            __SK_HDR_16BitSwap == false)
        {
          rb.framebuffer_flags &= ~SK_FRAMEBUFFER_FLAG_HDR;
        }

        else
        {
          pHdrColorData->hdrMode = NV_HDR_MODE_UHDA;
        }
      }

      ///_Push_NvAPI_HDR_Metadata_to_DXGI_Backend ();
    }

    return ret;
  }

  NvAPI_Status ret =
    NvAPI_Disp_HdrColorControl_Original ( displayId, pHdrColorData );

  if (pHdrColorData->cmd == NV_HDR_CMD_GET)
  {
    colorCheck         = { };
    colorCheck.version = NV_COLOR_DATA_VER;
    colorCheck.size    = sizeof (NV_COLOR_DATA);
    colorCheck.cmd     = NV_COLOR_CMD_GET_DEFAULT;

    // Fill-in default values if needed...
    if ( NVAPI_OK == NvAPI_Disp_ColorControl_Original (displayId, &colorCheck) )
    {
      NV_HDR_CAPABILITIES
          hdrCapabilities                                  = { };
      {
          hdrCapabilities.version                          = NV_HDR_CAPABILITIES_VER3;
          hdrCapabilities.driverExpandDefaultHdrParameters = true;

        std::lock_guard
             lock2 (SK_NvAPI_Threading->locks.Disp_GetHdrCapabilities);

        NvAPI_Disp_GetHdrCapabilities_Original (displayId, &hdrCapabilities);
      }

      if (pHdrColorData->hdrBpc == NV_BPC_DEFAULT)
          pHdrColorData->hdrBpc  = colorCheck.data.bpc;

      if (pHdrColorData->hdrColorFormat == NV_COLOR_FORMAT_DEFAULT)
          pHdrColorData->hdrColorFormat  = (NV_COLOR_FORMAT)colorCheck.data.colorFormat;

      if (pHdrColorData->hdrDynamicRange == NV_DYNAMIC_RANGE_AUTO)
          pHdrColorData->hdrDynamicRange  = (NV_DYNAMIC_RANGE)colorCheck.data.dynamicRange;

      if (pHdrColorData->mastering_display_data.displayPrimary_x0 == 0)
      {
        pHdrColorData->mastering_display_data.displayPrimary_x0               = hdrCapabilities.display_data.displayPrimary_x0;
        pHdrColorData->mastering_display_data.displayPrimary_x1               = hdrCapabilities.display_data.displayPrimary_x1;
        pHdrColorData->mastering_display_data.displayPrimary_x2               = hdrCapabilities.display_data.displayPrimary_x2;
        pHdrColorData->mastering_display_data.displayPrimary_y0               = hdrCapabilities.display_data.displayPrimary_y0;
        pHdrColorData->mastering_display_data.displayPrimary_y1               = hdrCapabilities.display_data.displayPrimary_y1;
        pHdrColorData->mastering_display_data.displayPrimary_y2               = hdrCapabilities.display_data.displayPrimary_y2;
        pHdrColorData->mastering_display_data.displayWhitePoint_x             = hdrCapabilities.display_data.displayWhitePoint_x;
        pHdrColorData->mastering_display_data.displayWhitePoint_y             = hdrCapabilities.display_data.displayWhitePoint_y;
        pHdrColorData->mastering_display_data.max_content_light_level         = hdrCapabilities.display_data.desired_content_max_luminance;
        pHdrColorData->mastering_display_data.max_display_mastering_luminance = hdrCapabilities.display_data.desired_content_max_luminance;
        pHdrColorData->mastering_display_data.min_display_mastering_luminance = hdrCapabilities.display_data.desired_content_min_luminance;
        pHdrColorData->mastering_display_data.max_frame_average_light_level   = hdrCapabilities.display_data.desired_content_max_frame_average_luminance;
      }
    }

    // If we are overriding, then we need to lie to the game... NVAPI would report OFF
    //   even though we've got HDR turned on using DXGI.
    pHdrColorData->hdrMode = (__SK_HDR_16BitSwap ? NV_HDR_MODE_UHDA :
                              __SK_HDR_10BitSwap ? NV_HDR_MODE_UHDA_PASSTHROUGH
                                                 : pHdrColorData->hdrMode);
  }

  _LogGameRequestedValues ();

  if (ret == NVAPI_OK && pHdrColorData->cmd == NV_HDR_CMD_SET)
  {
    rb.scanout.nvapi_hdr.mode           = (__SK_HDR_16BitSwap ? NV_HDR_MODE_UHDA :
                                           __SK_HDR_10BitSwap ? NV_HDR_MODE_UHDA_PASSTHROUGH
                                                              : pHdrColorData->hdrMode);
    rb.scanout.nvapi_hdr.color_format   = pHdrColorData->hdrColorFormat;
    rb.scanout.nvapi_hdr.dynamic_range  = pHdrColorData->hdrDynamicRange;
    rb.scanout.nvapi_hdr.bpc            = pHdrColorData->hdrBpc;
    rb.scanout.nvapi_hdr.active         =(pHdrColorData->hdrMode != NV_HDR_MODE_OFF);

    ///_Push_NvAPI_HDR_Metadata_to_DXGI_Backend ();
  }

  if (inputData->version == NV_HDR_COLOR_DATA_VER1 ||
      inputData->version == NV_HDR_COLOR_DATA_VER2)
  {
    inputData->hdrMode = NV_HDR_MODE_OFF;// pHdrColorData->hdrMode;

    memcpy (  &inputData->mastering_display_data,
          &pHdrColorData->mastering_display_data,
       sizeof (inputData->mastering_display_data) );

    inputData->static_metadata_descriptor_id =
      pHdrColorData->static_metadata_descriptor_id;

    if (inputData->version == NV_HDR_COLOR_DATA_VER2)
    {
      ((NV_HDR_COLOR_DATA_V2 *)inputData)->hdrBpc          = pHdrColorData->hdrBpc;
      ((NV_HDR_COLOR_DATA_V2 *)inputData)->hdrColorFormat  = pHdrColorData->hdrColorFormat;
      ((NV_HDR_COLOR_DATA_V2 *)inputData)->hdrDynamicRange = pHdrColorData->hdrDynamicRange;
    }
  }

  return ret;
}

bool
SK_RenderBackend_V2::output_s::nvapi_ctx_s::vblank_history_s::addRecord (NvDisplayHandle nv_disp, DXGI_FRAME_STATISTICS* pFrameStats, NvU64 tNow) noexcept
{
  const SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();
  
  NvU32   vblank_count =     0;
  bool bHasVBlankCount = false;

  if (! pFrameStats)
  {
    //
    // In case it was not patently obvious, NvAPI is not thread-safe :)
    //
    //   Without these locks -- rather than a crash -- framerate would plummet
    //     following any display topology / capability changes.
    //
    if (rb.stale_display_info || (! rb.gsync_state.active))
      return false;

    static constexpr auto                                   _PollingFreqInMs = 3;
    if (last_polled_time <= SK::ControlPanel::current_time -_PollingFreqInMs)
    {
      std::scoped_lock
        lock (SK_NvAPI_Threading->locks.Disp_GetVRRInfo,
              SK_NvAPI_Threading->locks.D3D_IsGSyncActive);

      bHasVBlankCount =
        (NVAPI_OK == NvAPI_GetVBlankCounter (nv_disp, &vblank_count));
    }
  }

  else
  {
    bHasVBlankCount = pFrameStats->SyncRefreshCount != 0;
    vblank_count    = pFrameStats->SyncRefreshCount;
  }

  if (bHasVBlankCount)
  {
    last_polled_time = SK::ControlPanel::current_time;

    head = std::min (head, (NvU32)MaxVBlankRecords-1);

    if (vblank_count > records [head].vblank_count)
    {
      if ( head == MaxVBlankRecords-1 )
           head  = 0;
      else head++;

      records [head] = { tNow, vblank_count };
    }

    return true;
  }

  return false;
}

void
SK_RenderBackend_V2::output_s::nvapi_ctx_s::vblank_history_s::resetStats (void) noexcept
{
  for (auto& record : records)
  {
    record.timestamp_qpc = 0;
    record.vblank_count  = 0;
  }
  head                   = 0;
  last_qpc_refreshed     = 0;
  last_frame_sampled     = 0;
  last_polled_time       = 0;
}

float
SK_RenderBackend_V2::output_s::nvapi_ctx_s::vblank_history_s::getVBlankHz (NvU64 tNow) noexcept
{
  NvU32 num_vblanks_in_period = 0;

  NvU64 vblank_count_min = UINT64_MAX,
        vblank_count_max = 0;

  NvU64 vblank_t0        = UINT64_MAX,
        vblank_n         = 0;

  static constexpr auto
    _MaxWindowSizeInMs = 750UL;

  for (const auto& record : records)
  {
    if (vblank_t0 > record.timestamp_qpc &&
                    record.timestamp_qpc >= tNow - SK_QpcTicksPerMs * _MaxWindowSizeInMs)
        vblank_t0 = record.timestamp_qpc;
  }

  if (vblank_t0 > tNow)
      vblank_t0 = tNow;

  for (const auto& record : records)
  {
    if (record.timestamp_qpc >= vblank_t0)
    {
      ++num_vblanks_in_period;

      if (vblank_n < record.timestamp_qpc)
          vblank_n = record.timestamp_qpc;

      if (vblank_count_min > record.vblank_count)
          vblank_count_min = record.vblank_count;

      if (vblank_count_max < record.vblank_count)
          vblank_count_max = record.vblank_count;
    }
  }

  // ... how?
  if (num_vblanks_in_period == 0)
    return 0.0f;

  // Apply smoothing because these numbers are hyperactive
  float new_average =
    static_cast <float> (
    static_cast <double> (vblank_count_max - vblank_count_min) /
   (static_cast <double> (vblank_n         - vblank_t0)        /
    static_cast <double> (SK_QpcFreq)));

  // Keep imaginary numbers out of the data set...
  if (vblank_n - vblank_t0 == 0)
    new_average = 0.0f;

  const auto& rb =
    SK_GetCurrentRenderBackend ();

  const auto& signal_timing =
    rb.displays [rb.active_display].signal.timing;

  const double _MaxExpectedRefreshDbl =
      static_cast <double> (signal_timing.vsync_freq.Numerator) /
      static_cast <double> (signal_timing.vsync_freq.Denominator);

  const float _MaxExpectedRefresh =
    static_cast <float> (_MaxExpectedRefreshDbl);

       last_average = std::min (     last_average, _MaxExpectedRefresh);
  last_last_average = std::min (last_last_average, _MaxExpectedRefresh);
        new_average = std::min (      new_average, _MaxExpectedRefresh);

  if (last_average != 0.0f)
  {
    // Weighted rolling-average because this is really jittery
    new_average =
      (2.0f * last_average + 7.5f * new_average + 0.5f * last_last_average) * 0.1f;
  }

  last_last_average = last_average;
  last_average      = new_average;

  static DWORD dwLastUpdate = SK::ControlPanel::current_time;
  static float fLastAverage = last_average;
  
  if (dwLastUpdate < SK::ControlPanel::current_time - 150)
  {   dwLastUpdate = SK::ControlPanel::current_time;

    float fNewAverage =
      (3.0f * fLastAverage + 7.0f * last_average) * 0.1f;

    if (fNewAverage > _MaxExpectedRefresh)
        fNewAverage = _MaxExpectedRefresh;

    fLastAverage = fNewAverage;
  }

  return fLastAverage;
}

bool
SK_RenderBackend_V2::scan_out_s::
       nvapi_desc_s::setColorEncoding_HDR ( NV_COLOR_FORMAT fmt_,
                                            NV_BPC          bpc_,
                                            NvU32           display_id )
{
  if (display_id == std::numeric_limits <NvU32>::max ())
      display_id = raw.display_id;

  NV_HDR_COLOR_DATA
    dataGetSet         = { };
    dataGetSet.version = NV_HDR_COLOR_DATA_VER;
    dataGetSet.cmd     = NV_HDR_CMD_GET;

  if ( NVAPI_OK ==
         NvAPI_Disp_HdrColorControl_Original ( display_id, &dataGetSet ) )
  {
    dataGetSet.cmd = NV_HDR_CMD_SET;

    if (bpc_ != NV_BPC_DEFAULT)
      dataGetSet.hdrBpc = bpc_;

    if (fmt_ != NV_COLOR_FORMAT_DEFAULT)
      dataGetSet.hdrColorFormat = fmt_;

    dataGetSet.hdrDynamicRange = NV_DYNAMIC_RANGE_AUTO;
    dataGetSet.hdrMode         = mode;

    NV_COLOR_DATA
      colorSet       = { };
    colorSet.version = NV_COLOR_DATA_VER;
    colorSet.cmd     = NV_COLOR_CMD_GET;
    colorSet.size    = sizeof (NV_COLOR_DATA);

    NvAPI_Disp_ColorControl_Original (display_id, &colorSet);

    if ( NVAPI_OK ==
           NvAPI_Disp_HdrColorControl_Original ( display_id, &dataGetSet ) )
    {
      colorSet.cmd                       = NV_COLOR_CMD_SET;
      colorSet.data.colorSelectionPolicy = NV_COLOR_SELECTION_POLICY_USER;
      colorSet.data.colorFormat          = (NvU8)dataGetSet.hdrColorFormat;
      colorSet.data.bpc                  =       dataGetSet.hdrBpc;
      colorSet.data.dynamicRange         = NV_DYNAMIC_RANGE_AUTO;
      colorSet.data.colorimetry          = NV_COLOR_COLORIMETRY_AUTO;
      colorSet.data.depth                = NV_DESKTOP_COLOR_DEPTH_16BPC_FLOAT_HDR;

      NvAPI_Disp_ColorControl_Original (display_id, &colorSet);

      SK_GetCurrentRenderBackend ().requestFullscreenMode ();

      raw.hdr_data  = dataGetSet;
      mode          = dataGetSet.hdrMode;
      bpc           = dataGetSet.hdrBpc;
      color_format  = dataGetSet.hdrColorFormat;
      dynamic_range = dataGetSet.hdrDynamicRange;

      NV_COLOR_DATA
        colorCheck                   = { };
        colorCheck.version           = NV_COLOR_DATA_VER;
        colorCheck.size              = sizeof (NV_COLOR_DATA);
        colorCheck.cmd               = NV_COLOR_CMD_IS_SUPPORTED_COLOR;
        colorCheck.data.colorimetry  = NV_COLOR_COLORIMETRY_AUTO;
        colorCheck.data.dynamicRange = NV_DYNAMIC_RANGE_AUTO;
        colorCheck.data.colorFormat  = NV_COLOR_FORMAT_RGB;
        colorCheck.data.bpc          = NV_BPC_10;

      if ( NVAPI_OK == NvAPI_Disp_ColorControl_Original (display_id, &colorCheck) )
        color_support_hdr.supports_10b_12b_444 |=  0x1;
      else
        color_support_hdr.supports_10b_12b_444 &= ~0x1;

      colorCheck.data.colorFormat = NV_COLOR_FORMAT_RGB;
      colorCheck.data.bpc         = NV_BPC_12;

      if ( NVAPI_OK == NvAPI_Disp_ColorControl_Original (display_id, &colorCheck) )
        color_support_hdr.supports_10b_12b_444 |=  0x2;
      else
        color_support_hdr.supports_10b_12b_444 &= ~0x2;

      colorCheck.data.colorFormat = NV_COLOR_FORMAT_YUV422;
      colorCheck.data.bpc         = NV_BPC_12;

      if ( NVAPI_OK == NvAPI_Disp_ColorControl_Original (display_id, &colorCheck) )
        color_support_hdr.supports_YUV422_12bit = 1;
      else
        color_support_hdr.supports_YUV422_12bit = 0;

      return true;
    }
  }

  return false;
}

bool
SK_RenderBackend_V2::scan_out_s::
       nvapi_desc_s::setColorEncoding_SDR ( NV_COLOR_FORMAT fmt_,
                                            NV_BPC          bpc_,
                                            NvU32           display_id )
{
  if (display_id == std::numeric_limits <NvU32>::max ())
  {
    display_id =
      SK_NvAPI_GetDefaultDisplayId ();
  }

  NV_COLOR_DATA
    colorSet       = { };
  colorSet.version = NV_COLOR_DATA_VER;
  colorSet.cmd     = NV_COLOR_CMD_GET;
  colorSet.size    = sizeof (NV_COLOR_DATA);

  NvAPI_Disp_ColorControl_Original (display_id, &colorSet);

  colorSet.cmd                       = NV_COLOR_CMD_SET;
  colorSet.data.colorSelectionPolicy = NV_COLOR_SELECTION_POLICY_USER;
  colorSet.data.colorFormat          = (NvU8)fmt_;
  colorSet.data.bpc                  =       bpc_;
  colorSet.data.dynamicRange         = NV_DYNAMIC_RANGE_AUTO;
  colorSet.data.colorimetry          = NV_COLOR_COLORIMETRY_AUTO;
  colorSet.data.depth                = NV_DESKTOP_COLOR_DEPTH_16BPC_FLOAT_HDR;

  if ( NVAPI_OK ==
         NvAPI_Disp_ColorControl_Original ( display_id, &colorSet ) )
  {
    SK_GetCurrentRenderBackend ().requestFullscreenMode ();

    mode          = NV_HDR_MODE_OFF;
    bpc           = colorSet.data.bpc;
    color_format  = (NV_COLOR_FORMAT)colorSet.data.colorFormat;
    dynamic_range = (NV_DYNAMIC_RANGE)colorSet.data.dynamicRange;

    return true;
  }

  return false;
}


using NvAPI_QueryInterface_pfn = void* (*)(unsigned int ordinal);
      NvAPI_QueryInterface_pfn
      NvAPI_QueryInterface_Original = nullptr;

#define threadsafe_unordered_set Concurrency::concurrent_unordered_set

void*
NvAPI_QueryInterface_Detour (unsigned int ordinal)
{
  std::lock_guard
       lock (SK_NvAPI_Threading->locks.QueryInterface);

//#define NVAPI_ORDINAL_TEST
#ifdef NVAPI_ORDINAL_TEST
  static
    threadsafe_unordered_set <unsigned int>
      logged_ordinals;
  if (logged_ordinals.count  (ordinal) == 0)
  {   logged_ordinals.insert (ordinal);

    void* pAddr =
      NvAPI_QueryInterface_Original (ordinal);

    dll_log->Log ( L"[  NvAPI   ] NvAPI Ordinal: %lu [%p]  --  %s", ordinal,
                     pAddr, SK_SummarizeCaller ().c_str ()    );

    return
      pAddr;
  }
#endif

  return
    NvAPI_QueryInterface_Original (ordinal);
}

NVAPI_INTERFACE
SK_NvAPI_Disp_GetVRRInfo (__in NvU32 displayId, __inout NV_GET_VRR_INFO *pVrrInfo)
{
  std::lock_guard
       lock (SK_NvAPI_Threading->locks.Disp_GetVRRInfo);

  return
    NvAPI_Disp_GetVRRInfo (displayId, pVrrInfo);
}

NVAPI_INTERFACE
SK_NvAPI_DISP_GetAdaptiveSyncData (__in NvU32 displayId, __inout NV_GET_ADAPTIVE_SYNC_DATA *pAdaptiveSyncData)
{
  std::lock_guard
       lock (SK_NvAPI_Threading->locks.DISP_GetAdaptiveSyncData);

  return
    NvAPI_DISP_GetAdaptiveSyncData (displayId, pAdaptiveSyncData);
}

NVAPI_INTERFACE
SK_NvAPI_DISP_SetAdaptiveSyncData    (__in NvU32 displayId, __in NV_SET_ADAPTIVE_SYNC_DATA *pAdaptiveSyncData)
{
  std::lock_guard
       lock (SK_NvAPI_Threading->locks.DISP_SetAdaptiveSyncData);

  return
    NvAPI_DISP_SetAdaptiveSyncData (displayId, pAdaptiveSyncData);
}

NVAPI_INTERFACE
SK_NvAPI_DISP_GetMonitorCapabilities (__in NvU32 displayId, __inout NV_MONITOR_CAPABILITIES *pMonitorCapabilities)
{
  std::lock_guard
       lock (SK_NvAPI_Threading->locks.DISP_GetMonitorCapabilities);

  return
    NvAPI_DISP_GetMonitorCapabilities (displayId, pMonitorCapabilities);
}

NVAPI_INTERFACE
SK_NvAPI_D3D_IsGSyncCapable (__in IUnknown          *pDeviceOrContext,
                             __in NVDX_ObjectHandle   primarySurface,
                             __out BOOL             *pIsGsyncCapable)
{
  std::lock_guard
       lock (SK_NvAPI_Threading->locks.D3D_IsGSyncCapable);

  return
    NvAPI_D3D_IsGSyncCapable (pDeviceOrContext, primarySurface, pIsGsyncCapable);
}

NVAPI_INTERFACE
SK_NvAPI_D3D_IsGSyncActive (__in IUnknown          *pDeviceOrContext,
                            __in NVDX_ObjectHandle   primarySurface,
                           __out BOOL              *pIsGsyncActive)
{
  std::lock_guard
       lock (SK_NvAPI_Threading->locks.D3D_IsGSyncActive);

  return
    NvAPI_D3D_IsGSyncActive (pDeviceOrContext, primarySurface, pIsGsyncActive);
}


BOOL bLibShutdown = FALSE;
BOOL bLibInit     = FALSE;

BOOL
NVAPI::UnloadLibrary (void)
{
  if ( bLibInit     == TRUE &&
       bLibShutdown == FALSE  )
  {
    // Whine very loudly if this fails, because that's not
    //   supposed to happen!
    NVAPI_VERBOSE ()

    NvAPI_Status ret =
      NvAPI_Unload ();//NVAPI_CALL2 (Unload (), ret);

    if (ret == NVAPI_OK)
    {
      bLibShutdown = TRUE;
      bLibInit     = FALSE;
    }
  }

  return
    bLibShutdown;
}

void
SK_NvAPI_PreInitHDR (void)
{
  if (NvAPI_Disp_HdrColorControl_Original == nullptr)
  {
#ifdef _WIN64
    HMODULE hLib =
      SK_Modules->LoadLibraryLL (L"nvapi64.dll");

    if (hLib)
    {
      GetModuleHandleEx (
        GET_MODULE_HANDLE_EX_FLAG_PIN, L"nvapi64.dll", &hLib
      );
#else
    HMODULE hLib =
      SK_Modules->LoadLibraryLL (L"nvapi.dll");

    if (hLib)
    {
      GetModuleHandleEx (
        GET_MODULE_HANDLE_EX_FLAG_PIN, L"nvapi.dll",   &hLib
      );
#endif

      static auto NvAPI_QueryInterface =
        reinterpret_cast <NvAPI_QueryInterface_pfn> (
          SK_GetProcAddress (hLib, "nvapi_QueryInterface")
        );

      if (NvAPI_QueryInterface != nullptr)
      {
        SK_CreateFuncHook ( L"NvAPI_Disp_ColorControl",
                              NvAPI_QueryInterface (2465847309),
                              NvAPI_Disp_ColorControl_Override,
     static_cast_p2p <void> (&NvAPI_Disp_ColorControl_Original) );
        SK_CreateFuncHook ( L"NvAPI_Disp_HdrColorControl",
                              NvAPI_QueryInterface (891134500),
                              NvAPI_Disp_HdrColorControl_Override,
    static_cast_p2p <void> (&NvAPI_Disp_HdrColorControl_Original) );

        SK_CreateFuncHook ( L"NvAPI_Disp_GetHdrCapabilities",
                              NvAPI_QueryInterface (2230495455),
                              NvAPI_Disp_GetHdrCapabilities_Override,
     static_cast_p2p <void> (&NvAPI_Disp_GetHdrCapabilities_Original) );

        MH_QueueEnableHook (NvAPI_QueryInterface (891134500));
        MH_QueueEnableHook (NvAPI_QueryInterface (2230495455));
        MH_QueueEnableHook (NvAPI_QueryInterface (2465847309));
      }
    }
  }
}

BOOL
NVAPI::InitializeLibrary (const wchar_t* wszAppName)
{
    std::lock_guard
         lock (SK_NvAPI_Threading->locks.Init);

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

  if (              app_cache_mgr->loadAppCacheForExe (SK_GetFullyQualifiedApp ()/*wszAppName*/))
    friendly_name = app_cache_mgr->getAppNameFromPath (SK_GetFullyQualifiedApp ()/*wszAppName*/);
  else
    friendly_name = wszAppName; // Not so friendly, but whatever...

  if (! config.apis.NvAPI.enable) {
    nv_hardware = false;
    bLibInit    = TRUE + 1; // Clearly this isn't a boolean; just for looks
    return FALSE;
  }

  NvAPI_Status ret       = NVAPI_ERROR;

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
      static auto NvAPI_QueryInterface =
        reinterpret_cast <NvAPI_QueryInterface_pfn> (
          SK_GetProcAddress (hLib, "nvapi_QueryInterface")
        );

      NvAPI_GPU_GetRamType =
        (NvAPI_GPU_GetRamType_pfn)NvAPI_QueryInterface            (0x57F7CAACu);
      NvAPI_GPU_GetFBWidthAndLocation =
        (NvAPI_GPU_GetFBWidthAndLocation_pfn)NvAPI_QueryInterface (0x11104158u);
      NvAPI_GPU_GetPCIEInfo =
        (NvAPI_GPU_GetPCIEInfo_pfn)NvAPI_QueryInterface           (0xE3795199u);
      NvAPI_GetGPUIDFromPhysicalGPU =
        (NvAPI_GetGPUIDFromPhysicalGPU_pfn)NvAPI_QueryInterface   (0x6533EA3Eu);

      NvAPI_Disp_SetDitherControl =
        (NvAPI_Disp_SetDitherControl_pfn)NvAPI_QueryInterface (__NvAPI_Disp_SetDitherControl);
      NvAPI_Disp_GetDitherControl =
        (NvAPI_Disp_GetDitherControl_pfn)NvAPI_QueryInterface (__NvAPI_Disp_GetDitherControl);

      if (NvAPI_GPU_GetRamType == nullptr) {
        dll_log->LogEx (false, L"missing NvAPI_GPU_GetRamType ");
        nv_hardware = false;
      }

      if (NvAPI_GPU_GetFBWidthAndLocation == nullptr) {
        dll_log->LogEx (false, L"missing NvAPI_GPU_GetFBWidthAndLocation ");
        nv_hardware = false;
      }

      if (NvAPI_GPU_GetPCIEInfo == nullptr) {
        dll_log->LogEx (false, L"missing NvAPI_GPU_GetPCIEInfo ");
        nv_hardware = false;
      }

      if (NvAPI_GetPhysicalGPUFromGPUID == nullptr) {
        dll_log->LogEx (false, L"missing NvAPI_GetPhysicalGPUFromGPUID ");
        nv_hardware = false;
      }

      if (NvAPI_GetGPUIDFromPhysicalGPU == nullptr) {
        dll_log->LogEx (false, L"missing NvAPI_GetGPUIDFromPhysicalGPU ");
        nv_hardware = false;
      }

      if ( NvU32                                     gpu_count = 0;
           NVAPI_NO_IMPLEMENTATION ==
             NvAPI_EnumPhysicalGPUs (_nv_dxgi_gpus, &gpu_count) )
      {
        dll_log->LogEx (false, L"no implementation for NvAPI_EnumPhysicalGPUs ");
        nv_hardware = false;
      }

      if (NvAPI_Disp_HdrColorControl_Original == nullptr)
      {
        SK_CreateFuncHook (      L"NvAPI_Disp_HdrColorControl",
                                   NvAPI_QueryInterface (891134500),
                                   NvAPI_Disp_HdrColorControl_Override,
          static_cast_p2p <void> (&NvAPI_Disp_HdrColorControl_Original) );

             SK_CreateFuncHook ( L"NvAPI_Disp_GetHdrCapabilities",
                                   NvAPI_QueryInterface (2230495455),
                                   NvAPI_Disp_GetHdrCapabilities_Override,
          static_cast_p2p <void> (&NvAPI_Disp_GetHdrCapabilities_Original) );

        MH_QueueEnableHook (NvAPI_QueryInterface (891134500));
        MH_QueueEnableHook (NvAPI_QueryInterface (2230495455));
      }

      if (NvAPI_Mosaic_GetDisplayViewportsByResolution_Original == nullptr)
      {
        SK_CreateFuncHook (      L"NvAPI_Mosaic_GetDisplayViewportsByResolution",
                                   NvAPI_QueryInterface (0xDC6DC8D3),
                                   NvAPI_Mosaic_GetDisplayViewportsByResolution_Override,
          static_cast_p2p <void> (&NvAPI_Mosaic_GetDisplayViewportsByResolution_Original) );

        MH_QueueEnableHook (NvAPI_QueryInterface (0xDC6DC8D3));
      }

      // Admin privileges are required to do this...
      if (SK_IsAdmin ())
        SK_NvAPI_AllowGFEOverlay (false, L"SKIF", L"SKIF.exe");

      SK_CreateDLLHook2 ( SK_RunLHIfBitness (64, L"nvapi64.dll",
                                                 L"nvapi.dll"),
                                "nvapi_QueryInterface",
                                 NvAPI_QueryInterface_Detour,
        static_cast_p2p <void> (&NvAPI_QueryInterface_Original) );

      SK_ApplyQueuedHooks ();

      if (! SK_IsRunDLLInvocation ())
      {
        SK_GetCurrentRenderBackend ().nvapi.rebar =
          SK_NvAPI_DRS_GetDWORD (0x000F00BA) != 0x0;
      }

//#ifdef SK_AGGRESSIVE_HOOKS
//      SK_ApplyQueuedHooks ();
//#endif
    }

    else
    {
      dll_log->Log (L"unable to complete LoadLibrary (...) ");
      nv_hardware = false;
    }

    if (nv_hardware == false)
    {
      bLibInit = FALSE;
      hLib     = nullptr;
    }
  }

  return
    ( bLibInit = TRUE );
}



bool SK_NvAPI_InitializeHDR (void)
{
  std::lock_guard
       lock (SK_NvAPI_Threading->locks.Disp_GetHdrCapabilities);

  if (nv_hardware && NvAPI_Disp_GetHdrCapabilities_Original != nullptr)
  {
    NV_HDR_CAPABILITIES
      hdr_caps       = { };
    hdr_caps.version = NV_HDR_CAPABILITIES_VER;

    NvAPI_Disp_GetHdrCapabilities_Override (
      SK_NvAPI_GetDefaultDisplayId (), &hdr_caps
    );
  }

  // Not yet meaningful
  return true;
}

SK_LazyGlobal <NV_GET_CURRENT_SLI_STATE> SK_NV_sli_state;

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
    InitializeCriticalSectionEx (&cs_aa_override, 0, 0x0);
    init = true;
  }

  EnterCriticalSection (&cs_aa_override);

  NvAPI_Status       ret       = NVAPI_ERROR;
  NvDRSSessionHandle hSession  = { };

  NVAPI_CALL (DRS_CreateSession (&hSession));
  NVAPI_CALL (DRS_LoadSettings  (hSession));

  NvDRSProfileHandle                     hProfile;
  std::unique_ptr    <NVDRS_APPLICATION> app_ptr =
    std::make_unique <NVDRS_APPLICATION> ();
  NVDRS_APPLICATION&                     app     = *app_ptr;

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

  while ( prop.wszName  != nullptr &&
          prop.wszValue != nullptr )
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

    if (friendly_name.empty ()) // Avoid NVAPI failure: NVAPI_PROFILE_NAME_EMPTY
        friendly_name = app_name;

    custom_profile.isPredefined  = FALSE;
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
      RtlZeroMemory (app_ptr.get (), sizeof (NVDRS_APPLICATION));

      lstrcpyW ((wchar_t *)app.appName,          app_name.c_str      ());
      lstrcpyW ((wchar_t *)app.userFriendlyName, friendly_name.c_str ());

      app.version      = NVDRS_APPLICATION_VER;
      app.isPredefined = FALSE;
      app.isMetro      = FALSE;

      NVAPI_CALL2 (DRS_CreateApplication (hSession, hProfile, &app), ret);
      NVAPI_CALL2 (DRS_SaveSettings      (hSession), ret);
    }

    // Driver's not being cooperative, we have no choice but to bail-out
    else {
      dll_log->LogEx ( true, L"[  NvAPI   ] Could not find or create application profile for '%s' (%s)",
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

      LeaveCriticalSection (&cs_aa_override);

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
    dll_log->Log ( L"[  NvAPI   ] *** Cannot set NvDRS Profile Setting '%x' as a normal user, skipping...",
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
    HMODULE hLib = SK_Modules->LoadLibraryLL (L"nvapi.dll");
#else
    HMODULE hLib = SK_Modules->LoadLibraryLL (L"nvapi64.dll");
#endif
#define __NvAPI_RestartDisplayDriver                      0xB4B26B65
    typedef void* (*NvAPI_QueryInterface_pfn)(unsigned int offset);
    typedef NvAPI_Status(__cdecl *NvAPI_RestartDisplayDriver_pfn)(void);
    NvAPI_QueryInterface_pfn          NvAPI_QueryInterface       =
      (NvAPI_QueryInterface_pfn)SK_GetProcAddress (hLib, "nvapi_QueryInterface");
    NvAPI_RestartDisplayDriver_pfn NvAPI_RestartDisplayDriver =
      (NvAPI_RestartDisplayDriver_pfn)NvAPI_QueryInterface (__NvAPI_RestartDisplayDriver);

    NvAPI_RestartDisplayDriver ();
#endif

    NVAPI_CALL (DRS_SaveSettings   (hSession));
  } NVAPI_CALL (DRS_DestroySession (hSession));

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
    RtlZeroMemory (&fps_limiter, sizeof NVDRS_SETTING);
    fps_limiter.version = NVDRS_SETTING_VER;

    already_set = FALSE;

    NVAPI_SET_DWORD (fps_limiter, PS_FRAMERATE_LIMITER_ID, limit_mask);
    NVAPI_CALL      (DRS_SetSetting (hSession, hProfile, &fps_limiter));
  }

  // If this == -1, then don't make any changes...
  if (target_prerender != -1 && prerendered_frames.u32CurrentValue != target_prerender) {
    RtlZeroMemory (&prerendered_frames, sizeof NVDRS_SETTING);
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
    HMODULE hLib = SK_Modules->LoadLibraryLL (L"nvapi.dll");
#else
    HMODULE hLib = SK_Modules->LoadLibraryLL (L"nvapi64.dll");
#endif
#define __NvAPI_RestartDisplayDriver                      0xB4B26B65
    typedef void* (*NvAPI_QueryInterface_pfn)(unsigned int offset);
    typedef NvAPI_Status(__cdecl *NvAPI_RestartDisplayDriver_pfn)(void);
    NvAPI_QueryInterface_pfn       NvAPI_QueryInterface       =
      (NvAPI_QueryInterface_pfn)SK_GetProcAddress (hLib, "nvapi_QueryInterface");
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


BOOL SK_NvAPI_GetVRREnablement (void)
{
  if (! nv_hardware)
    return -2;

  NvAPI_Status       ret       = NVAPI_ERROR;
  NvDRSSessionHandle hSession  = { };

  NVAPI_CALL (DRS_CreateSession (&hSession));
  NVAPI_CALL (DRS_LoadSettings  ( hSession));

               NvDRSProfileHandle hProfile       = { };
  std::unique_ptr    <NVDRS_APPLICATION> app_ptr =
    std::make_unique <NVDRS_APPLICATION> ();
  NVDRS_APPLICATION&                     app     =
                                        *app_ptr;

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
    NVDRS_PROFILE custom_profile = {   };

    if (friendly_name.empty ()) // Avoid NVAPI failure: NVAPI_PROFILE_NAME_EMPTY
        friendly_name = app_name;

    custom_profile.isPredefined  = FALSE;
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
    {
      NVAPI_CALL2 ( DRS_FindProfileByName ( hSession,
                                              (NvU16 *)friendly_name.c_str (),
                                                &hProfile),
                      ret );
    }

    if (ret == NVAPI_OK)
    {
      RtlZeroMemory (app_ptr.get (), sizeof NVDRS_APPLICATION);

      lstrcpyW ((wchar_t *)app.appName,          app_name.c_str      ());
      lstrcpyW ((wchar_t *)app.userFriendlyName, friendly_name.c_str ());

      app.version      = NVDRS_APPLICATION_VER;
      app.isPredefined = FALSE;
      app.isMetro      = FALSE;

      NVAPI_CALL2 (DRS_CreateApplication (hSession, hProfile, &app), ret);
      NVAPI_CALL2 (DRS_SaveSettings      (hSession), ret);
      NVAPI_CALL  (DRS_LoadSettings      (hSession));
    }
  }

  NVDRS_SETTING vrr_control_val               = {               };
                vrr_control_val.version       = NVDRS_SETTING_VER;

  // These settings may not exist, and getting back a value of 0 is okay...
  NVAPI_SILENT  ();
  NVAPI_CALL    (DRS_GetSetting (hSession, hProfile, VRR_APP_OVERRIDE_ID, &vrr_control_val));
  NVAPI_VERBOSE ();

  BOOL bRet =
   ( vrr_control_val.u32CurrentValue != VRR_APP_OVERRIDE_FORCE_OFF )
                                      ? TRUE
                                      : FALSE;

  //SK_ReleaseAssert (vrr_control_val.u32CurrentValue <= VRRREQUESTSTATE_FULLSCREEN_AND_WINDOWED);

  NVAPI_CALL (DRS_DestroySession (hSession));

  return bRet;
}

BOOL SK_NvAPI_SetVRREnablement (BOOL bEnable)
{
  if (! nv_hardware)
    return -2;

  NvAPI_Status       ret       = NVAPI_ERROR;
  NvDRSSessionHandle hSession  = { };

  NVAPI_CALL (DRS_CreateSession (&hSession));
  NVAPI_CALL (DRS_LoadSettings  ( hSession));

               NvDRSProfileHandle hProfile       = { };
  std::unique_ptr    <NVDRS_APPLICATION> app_ptr =
    std::make_unique <NVDRS_APPLICATION> ();
  NVDRS_APPLICATION&                     app     =
                                        *app_ptr;

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
    NVDRS_PROFILE custom_profile = {   };

    if (friendly_name.empty ()) // Avoid NVAPI failure: NVAPI_PROFILE_NAME_EMPTY
        friendly_name = app_name;

    custom_profile.isPredefined  = FALSE;
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
    {
      NVAPI_CALL2 ( DRS_FindProfileByName ( hSession,
                                              (NvU16 *)friendly_name.c_str (),
                                                &hProfile),
                      ret );
    }

    if (ret == NVAPI_OK)
    {
      RtlZeroMemory (app_ptr.get (), sizeof NVDRS_APPLICATION);

      lstrcpyW ((wchar_t *)app.appName,          app_name.c_str      ());
      lstrcpyW ((wchar_t *)app.userFriendlyName, friendly_name.c_str ());

      app.version      = NVDRS_APPLICATION_VER;
      app.isPredefined = FALSE;
      app.isMetro      = FALSE;

      NVAPI_CALL2 (DRS_CreateApplication (hSession, hProfile, &app), ret);
      NVAPI_CALL2 (DRS_SaveSettings      (hSession),                 ret);
    }
  }

  NVDRS_SETTING vrr_control_val               = {               };
                vrr_control_val.version       = NVDRS_SETTING_VER;

  // These settings may not exist, and getting back a value of 0 is okay...
  NVAPI_SILENT  ();
  NVAPI_CALL    (DRS_GetSetting (hSession, hProfile, VRR_APP_OVERRIDE_ID, &vrr_control_val));
  NVAPI_SET_DWORD (vrr_control_val,                  VRR_APP_OVERRIDE_ID,
                                           bEnable ? VRR_APP_OVERRIDE_ALLOW
                                                   : VRR_APP_OVERRIDE_FORCE_OFF);
  NVAPI_CALL    (DRS_SetSetting (hSession, hProfile,                     &vrr_control_val));
  NVAPI_VERBOSE ();

  BOOL bRet =
   ( vrr_control_val.u32CurrentValue != VRR_APP_OVERRIDE_FORCE_OFF )
                                      ? TRUE
                                      : FALSE;

  NVAPI_CALL (DRS_SaveSettings   (hSession));
  NVAPI_CALL (DRS_DestroySession (hSession));

  return bRet;
}

BOOL SK_NvAPI_EnableVulkanBridge (BOOL bEnable)
{
#define OGL_DX_PRESENT_DEBUG_ID       0x20324987
#define DISABLE_FULLSCREEN_OPT        0x00000001
#define ENABLE_DFLIP_ALWAYS           0x00000004
#define FORCE_PRESENT_RESTARTS        0x00000010
#define SIGNAL_PRESENT_END_FROM_CPU   0x00000020
#define DISABLE_MSHYBRID_SEQUENTIAL   0x00000040
#define ENABLE_DX_SYNC_INTERVAL       0x00000080
#define DISABLE_INTEROP_GPU_SYNC      0x00000100
#define FORCE_INTEROP_GPU_SYNC        0x00000200
#define ENABLE_WITH_NON_NV            0x00020000
#define ENABLE_DXVK                   0x00080000

#define OGL_DX_LAYERED_PRESENT_ID     0x20D690F8
#define OGL_DX_LAYERED_PRESENT_AUTO   0x00000000
#define OGL_DX_LAYERED_PRESENT_DXGI   0x00000001
#define OGL_DX_LAYERED_PRESENT_NATIVE 0x00000002

  static constexpr int uiOptimalInteropFlags =
    ( DISABLE_FULLSCREEN_OPT      | ENABLE_DFLIP_ALWAYS     |
      SIGNAL_PRESENT_END_FROM_CPU | ENABLE_DX_SYNC_INTERVAL |
      FORCE_INTEROP_GPU_SYNC      | ENABLE_DXVK );

  if (! nv_hardware)
    return -2;

  NvAPI_Status       ret       = NVAPI_ERROR;
  NvDRSSessionHandle hSession  = { };

  NVAPI_CALL (DRS_CreateSession (&hSession));
  NVAPI_CALL (DRS_LoadSettings  ( hSession));

               NvDRSProfileHandle hProfile       = { };
  std::unique_ptr    <NVDRS_APPLICATION> app_ptr =
    std::make_unique <NVDRS_APPLICATION> ();
  NVDRS_APPLICATION&                     app     =
                                        *app_ptr;

  NVAPI_SILENT ();

  app.version = NVDRS_APPLICATION_VER;
  ret         = NVAPI_ERROR;

  NVAPI_CALL2 ( DRS_FindApplicationByName ( hSession,
                                              (NvU16 *)app_name.c_str (),
                                                &hProfile,
                                                  &app ),
                ret );

  if (ret != NVAPI_OK && ret != NVAPI_EXECUTABLE_NOT_FOUND)
    SK_MessageBox (SK_FormatStringW (L"FindApplicationByName Returned %d", ret), L"NVAPI Debug", MB_OK);

  // If no executable exists anywhere by this name, create a profile for it
  //   and then add the executable to it.
  if (ret == NVAPI_EXECUTABLE_NOT_FOUND)
  {
    NVDRS_PROFILE custom_profile = {   };

    if (friendly_name.empty ()) // Avoid NVAPI failure: NVAPI_PROFILE_NAME_EMPTY
        friendly_name = app_name;

    custom_profile.isPredefined  = FALSE;
    lstrcpyW ((wchar_t *)custom_profile.profileName, friendly_name.c_str ());
    custom_profile.version = NVDRS_PROFILE_VER;

    // It's not necessarily wrong if this does not return NVAPI_OK, so don't
    //   raise a fuss if it happens.
    NVAPI_SILENT ()
    {
      NVAPI_CALL2 (DRS_CreateProfile (hSession, &custom_profile, &hProfile), ret);

      if (ret != NVAPI_OK && ret != NVAPI_PROFILE_NAME_IN_USE)
        SK_MessageBox (SK_FormatStringW (L"CreateProfile Returned %d", ret), L"NVAPI Debug", MB_OK);
    }
    NVAPI_VERBOSE ()

    // Add the application name to the profile, if a profile already exists
    if (ret == NVAPI_PROFILE_NAME_IN_USE)
    {
      NVAPI_CALL2 ( DRS_FindProfileByName ( hSession,
                                              (NvU16 *)friendly_name.c_str (),
                                                &hProfile),
                      ret );
    }

    if (ret != NVAPI_OK)
        SK_MessageBox (SK_FormatStringW (L"DRS_FindProfileByName (2) Returned %d", ret), L"NVAPI Debug", MB_OK);

    if (ret == NVAPI_OK)
    {
      RtlZeroMemory (app_ptr.get (), sizeof NVDRS_APPLICATION);

      lstrcpyW ((wchar_t *)app.appName,          app_name.c_str      ());
      lstrcpyW ((wchar_t *)app.userFriendlyName, friendly_name.c_str ());

      app.version      = NVDRS_APPLICATION_VER;
      app.isPredefined = FALSE;
      app.isMetro      = FALSE;

      NVAPI_CALL2 (DRS_CreateApplication (hSession, hProfile, &app), ret);
      NVAPI_CALL2 (DRS_SaveSettings      (hSession),                 ret);
    }
  }

  bool bRestartRequired = false;

  if (config.apis.d3d9.hook != !bEnable)
  {
    // Turn these off if using DXGI layered present
    config.apis.d3d9.hook   = !bEnable;
    config.apis.d3d9ex.hook = !bEnable;

    SK_SaveConfig ();

    bRestartRequired = true;
  }

  NVDRS_SETTING ogl_dx_present_debug_val         = {               };
                ogl_dx_present_debug_val.version = NVDRS_SETTING_VER;

  NVDRS_SETTING ogl_dx_present_layer_val         = {               };
                ogl_dx_present_layer_val.version = NVDRS_SETTING_VER;

  // These settings may not exist, and getting back a value of 0 is okay...
  NVAPI_SILENT  ();
  NVAPI_CALL    (DRS_GetSetting (hSession, hProfile, OGL_DX_PRESENT_DEBUG_ID,   &ogl_dx_present_debug_val));
  NVAPI_CALL    (DRS_GetSetting (hSession, hProfile, OGL_DX_LAYERED_PRESENT_ID, &ogl_dx_present_layer_val));

  DWORD dwLayeredPresent =
    bEnable ? OGL_DX_LAYERED_PRESENT_DXGI
            : OGL_DX_LAYERED_PRESENT_NATIVE;

  if (ogl_dx_present_layer_val.u32CurrentValue != dwLayeredPresent)
  {
    NVAPI_SET_DWORD (ogl_dx_present_layer_val,         OGL_DX_LAYERED_PRESENT_ID, dwLayeredPresent);
    NVAPI_CALL    (DRS_SetSetting (hSession, hProfile, &ogl_dx_present_layer_val));

    bRestartRequired = true;
  }

  if (! SK_IsAdmin ())
  {
    if (bEnable)
    {
      if ((ogl_dx_present_debug_val.u32CurrentValue & (uiOptimalInteropFlags))
                                                   != (uiOptimalInteropFlags))
      {
        NVAPI_CALL (DRS_SaveSettings   (hSession));
        NVAPI_CALL (DRS_DestroySession (hSession));

        std::wstring wszCommand =
          SK_FormatStringW (
            L"rundll32.exe \"%ws\", RunDLL_NvAPI_SetDWORD %x %x %ws",
              SK_GetModuleFullName (SK_GetDLL ()).c_str (),
                OGL_DX_PRESENT_DEBUG_ID, ogl_dx_present_debug_val.u32CurrentValue | uiOptimalInteropFlags,
                  app_name.c_str ()
                  
          );

        if ( IDOK ==
               SK_MessageBox ( L"Special K's Vulkan Bridge Will Be Enabled\r\n\r\tA game restart is required.",
                               L"NVIDIA Vulkan/DXGI Layer Setup", MB_OKCANCEL | MB_ICONINFORMATION )
           )
        {
          SK_ElevateToAdmin (wszCommand.c_str ());
          bRestartRequired = true;
        }

        else
          return FALSE;
      }
    }

    else
    {
      if ((ogl_dx_present_debug_val.u32CurrentValue & ENABLE_DXVK)
                                                   == ENABLE_DXVK)
      {
        NVAPI_CALL (DRS_SaveSettings   (hSession));
        NVAPI_CALL (DRS_DestroySession (hSession));

        std::wstring wszCommand =
          SK_FormatStringW (
            L"rundll32.exe \"%ws\", RunDLL_NvAPI_SetDWORD %x %x %ws",
              SK_GetModuleFullName (SK_GetDLL ()).c_str (),
                OGL_DX_PRESENT_DEBUG_ID, ogl_dx_present_debug_val.u32CurrentValue & ~ENABLE_DXVK,
                  app_name.c_str ()
                  
          );

        if ( IDOK ==
               SK_MessageBox ( L"Special K's Vulkan Bridge Will Be Disabled\r\n\r\tA game restart is required.",
                               L"NVIDIA Vulkan/DXGI Layer Setup", MB_OKCANCEL | MB_ICONINFORMATION )
           )
        {
          SK_ElevateToAdmin (wszCommand.c_str ());
          bRestartRequired = true;
        }

        else
          return TRUE;
      }
    }
  }

  // Highly unlikely that we'll ever reach this point... don't run games as admin! :P
  if (SK_IsAdmin ())
  {
    NVAPI_SET_DWORD (ogl_dx_present_debug_val,         OGL_DX_PRESENT_DEBUG_ID,
                                             bEnable ? ogl_dx_present_debug_val.u32CurrentValue |
                                                        uiOptimalInteropFlags
                                                     : ogl_dx_present_debug_val.u32CurrentValue &
                                                      (~ENABLE_DXVK));
    NVAPI_CALL   (DRS_SetSetting (hSession, hProfile, &ogl_dx_present_debug_val));
  }
  NVAPI_VERBOSE ();

  NVAPI_CALL (DRS_SaveSettings   (hSession));
  NVAPI_CALL (DRS_DestroySession (hSession));

  if (bRestartRequired)
    SK_RestartGame ();

  return true;
}

BOOL SK_NvAPI_GetFastSync (void)
{
  if (! nv_hardware)
    return -2;

  NvAPI_Status       ret       = NVAPI_ERROR;
  NvDRSSessionHandle hSession  = { };

  NVAPI_CALL (DRS_CreateSession (&hSession));
  NVAPI_CALL (DRS_LoadSettings  ( hSession));

               NvDRSProfileHandle hProfile       = { };
  std::unique_ptr    <NVDRS_APPLICATION> app_ptr =
    std::make_unique <NVDRS_APPLICATION> ();
  NVDRS_APPLICATION&                     app     =
                                        *app_ptr;

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
    NVDRS_PROFILE custom_profile = {   };

    if (friendly_name.empty ()) // Avoid NVAPI failure: NVAPI_PROFILE_NAME_EMPTY
        friendly_name = app_name;

    custom_profile.isPredefined  = FALSE;
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
    {
      NVAPI_CALL2 ( DRS_FindProfileByName ( hSession,
                                              (NvU16 *)friendly_name.c_str (),
                                                &hProfile),
                      ret );
    }

    if (ret == NVAPI_OK)
    {
      RtlZeroMemory (app_ptr.get (), sizeof NVDRS_APPLICATION);

      lstrcpyW ((wchar_t *)app.appName,          app_name.c_str      ());
      lstrcpyW ((wchar_t *)app.userFriendlyName, friendly_name.c_str ());

      app.version      = NVDRS_APPLICATION_VER;
      app.isPredefined = FALSE;
      app.isMetro      = FALSE;

      NVAPI_CALL2 (DRS_CreateApplication (hSession, hProfile, &app), ret);
      NVAPI_CALL2 (DRS_SaveSettings      (hSession), ret);
      NVAPI_CALL  (DRS_LoadSettings      (hSession));
    }
  }

  NVDRS_SETTING vsync_mode_val               = {               };
                vsync_mode_val.version       = NVDRS_SETTING_VER;

  // These settings may not exist, and getting back a value of 0 is okay...
  NVAPI_SILENT  ();
  NVAPI_CALL    (DRS_GetSetting (hSession, hProfile, VSYNCMODE_ID, &vsync_mode_val));
  NVAPI_VERBOSE ();

  BOOL bRet =
   ( vsync_mode_val.u32CurrentValue == VSYNCMODE_VIRTUAL )
                                     ? TRUE
                                     : FALSE;

  NVAPI_CALL (DRS_DestroySession (hSession));

  return bRet;
}

BOOL SK_NvAPI_SetFastSync (BOOL bEnable)
{
  if (! nv_hardware)
    return -2;

  NvAPI_Status       ret       = NVAPI_ERROR;
  NvDRSSessionHandle hSession  = { };

  NVAPI_CALL (DRS_CreateSession (&hSession));
  NVAPI_CALL (DRS_LoadSettings  ( hSession));

               NvDRSProfileHandle hProfile       = { };
  std::unique_ptr    <NVDRS_APPLICATION> app_ptr =
    std::make_unique <NVDRS_APPLICATION> ();
  NVDRS_APPLICATION&                     app     =
                                        *app_ptr;

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
    NVDRS_PROFILE custom_profile = {   };

    if (friendly_name.empty ()) // Avoid NVAPI failure: NVAPI_PROFILE_NAME_EMPTY
        friendly_name = app_name;

    custom_profile.isPredefined  = FALSE;
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
    {
      NVAPI_CALL2 ( DRS_FindProfileByName ( hSession,
                                              (NvU16 *)friendly_name.c_str (),
                                                &hProfile),
                      ret );
    }

    if (ret == NVAPI_OK)
    {
      RtlZeroMemory (app_ptr.get (), sizeof NVDRS_APPLICATION);

      lstrcpyW ((wchar_t *)app.appName,          app_name.c_str      ());
      lstrcpyW ((wchar_t *)app.userFriendlyName, friendly_name.c_str ());

      app.version      = NVDRS_APPLICATION_VER;
      app.isPredefined = FALSE;
      app.isMetro      = FALSE;

      NVAPI_CALL2 (DRS_CreateApplication (hSession, hProfile, &app), ret);
      NVAPI_CALL2 (DRS_SaveSettings      (hSession),                 ret);
    }
  }

  NVDRS_SETTING vsync_mode_val               = {               };
                vsync_mode_val.version       = NVDRS_SETTING_VER;

  // These settings may not exist, and getting back a value of 0 is okay...
  NVAPI_SILENT    ();
  NVAPI_CALL      (DRS_GetSetting (hSession, hProfile, VSYNCMODE_ID, &vsync_mode_val));
  NVAPI_SET_DWORD (vsync_mode_val,                     VSYNCMODE_ID,
                                             bEnable ? VSYNCMODE_VIRTUAL
                                                     : VSYNCMODE_DEFAULT);
  NVAPI_CALL    (DRS_SetSetting (hSession, hProfile,                 &vsync_mode_val));
  NVAPI_VERBOSE ();

  BOOL bRet =
   ( vsync_mode_val.u32CurrentValue == VSYNCMODE_VIRTUAL )
                                     ? TRUE
                                     : FALSE;

  NVAPI_CALL (DRS_SaveSettings   (hSession));
  NVAPI_CALL (DRS_DestroySession (hSession));

  return bRet;
}


BOOL SK_NvAPI_AllowGFEOverlay (bool bAllow, wchar_t *wszAppName, wchar_t *wszExecutable)
{
  if (! nv_hardware)
    return -2;

  NvAPI_Status       ret       = NVAPI_ERROR;
  NvDRSSessionHandle hSession  = { };

  NVAPI_CALL (DRS_CreateSession (&hSession));
  NVAPI_CALL (DRS_LoadSettings  ( hSession));

               NvDRSProfileHandle hProfile       = { };
  std::unique_ptr    <NVDRS_APPLICATION> app_ptr =
    std::make_unique <NVDRS_APPLICATION> ();
  NVDRS_APPLICATION&                     app     =
                                        *app_ptr;

  NVAPI_SILENT ();

  app.version = NVDRS_APPLICATION_VER;
  ret         = NVAPI_ERROR;

  NVAPI_CALL2 ( DRS_FindApplicationByName ( hSession,
                                              (NvU16 *)wszExecutable,
                                                &hProfile,
                                                  &app ),
                ret );

  // If no executable exists anywhere by this name, create a profile for it
  //   and then add the executable to it.
  if (ret == NVAPI_EXECUTABLE_NOT_FOUND)
  {
    NVDRS_PROFILE custom_profile = {   };

    if (friendly_name.empty ()) // Avoid NVAPI failure: NVAPI_PROFILE_NAME_EMPTY
        friendly_name = app_name;

    custom_profile.isPredefined  = FALSE;
    lstrcpyW ((wchar_t *)custom_profile.profileName, wszAppName);
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
    {
      NVAPI_CALL2 ( DRS_FindProfileByName ( hSession,
                                              (NvU16 *)wszAppName,
                                                &hProfile),
                      ret );
    }

    if (ret == NVAPI_OK)
    {
      RtlZeroMemory (app_ptr.get (), sizeof NVDRS_APPLICATION);

      lstrcpyW ((wchar_t *)app.appName,          wszExecutable);
      lstrcpyW ((wchar_t *)app.userFriendlyName, wszAppName);

      app.version      = NVDRS_APPLICATION_VER;
      app.isPredefined = FALSE;
      app.isMetro      = FALSE;

      NVAPI_CALL2 (DRS_CreateApplication (hSession, hProfile, &app), ret);
      NVAPI_CALL2 (DRS_SaveSettings      (hSession),                 ret);
    }
  }

  NVDRS_SETTING gfe_overlay_val                 = {               };
                gfe_overlay_val.version         = NVDRS_SETTING_VER;

#define GFE_OVERLAY_ID       0x809D5F60
#define GFE_OVERLAY_ALLOW    0x0
#define GFE_OVERLAY_DISALLOW 0x10000000

  // These settings may not exist, and getting back a value of 0 is okay...
  NVAPI_SILENT  ();
  NVAPI_CALL    (DRS_GetSetting (hSession, hProfile, GFE_OVERLAY_ID, &gfe_overlay_val));
  NVAPI_SET_DWORD (gfe_overlay_val,                  GFE_OVERLAY_ID,
                                            bAllow ? GFE_OVERLAY_ALLOW
                                                   : GFE_OVERLAY_DISALLOW);
  NVAPI_CALL    (DRS_SetSetting (hSession, hProfile,                 &gfe_overlay_val));
  NVAPI_VERBOSE ();

  BOOL bRet =
   ( gfe_overlay_val.u32CurrentValue == GFE_OVERLAY_ALLOW )
                                      ? TRUE
                                      : FALSE;

  NVAPI_CALL (DRS_SaveSettings   (hSession));
  NVAPI_CALL (DRS_DestroySession (hSession));

  return bRet;
}

void
CALLBACK
RunDLL_RestartNVIDIADriver ( HWND   hwnd,        HINSTANCE hInst,
                             LPCSTR lpszCmdLine, int       nCmdShow )
{
  UNREFERENCED_PARAMETER (hInst);
  UNREFERENCED_PARAMETER (hwnd);
  UNREFERENCED_PARAMETER (nCmdShow);

  if (SK_IsAdmin ())
  {
#ifndef _WIN64
    HMODULE hLib = SK_Modules->LoadLibraryLL (L"nvapi.dll");
#else
    HMODULE hLib = SK_Modules->LoadLibraryLL (L"nvapi64.dll");
#endif

#define __NvAPI_RestartDisplayDriver                      0xB4B26B65
    typedef void* (*NvAPI_QueryInterface_pfn)(unsigned int offset);
    typedef NvAPI_Status(__cdecl *NvAPI_RestartDisplayDriver_pfn)(void);
    NvAPI_QueryInterface_pfn          NvAPI_QueryInterface       =
      (NvAPI_QueryInterface_pfn)SK_GetProcAddress (hLib, "nvapi_QueryInterface");

    if (NvAPI_QueryInterface != nullptr)
    {
      NvAPI_RestartDisplayDriver_pfn NvAPI_RestartDisplayDriver =
        (NvAPI_RestartDisplayDriver_pfn)NvAPI_QueryInterface (__NvAPI_RestartDisplayDriver);

      if (NvAPI_RestartDisplayDriver != nullptr)
          NvAPI_RestartDisplayDriver ();
    }
  }
  
  else
  {
    if (! StrStrIA (lpszCmdLine, "silent"))
    {
      MessageBox (
        NULL, L"This command must be run as admin.",
           L"Restart NVIDIA Driver Failed", MB_OK
      );
    }
  }
}

DWORD
SK_NvAPI_DRS_GetDWORD (NvU32 setting_id)
{
  if (! nv_hardware)
    return (DWORD)-1;

  NvAPI_Status       ret       = NVAPI_ERROR;
  NvDRSSessionHandle hSession  = { };

  NVAPI_CALL (DRS_CreateSession (&hSession));
  NVAPI_CALL (DRS_LoadSettings  ( hSession));

               NvDRSProfileHandle hProfile       = { };
  std::unique_ptr    <NVDRS_APPLICATION> app_ptr =
    std::make_unique <NVDRS_APPLICATION> ();
  NVDRS_APPLICATION&                     app     =
                                        *app_ptr;

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
    NVDRS_PROFILE custom_profile = {   };

    if (friendly_name.empty ()) // Avoid NVAPI failure: NVAPI_PROFILE_NAME_EMPTY
        friendly_name = app_name;

    custom_profile.isPredefined  = FALSE;
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
    {
      NVAPI_CALL2 ( DRS_FindProfileByName ( hSession,
                                              (NvU16 *)friendly_name.c_str (),
                                                &hProfile),
                      ret );
    }

    if (ret == NVAPI_OK)
    {
      RtlZeroMemory (app_ptr.get (), sizeof NVDRS_APPLICATION);

      lstrcpyW ((wchar_t *)app.appName,          app_name.c_str      ());
      lstrcpyW ((wchar_t *)app.userFriendlyName, friendly_name.c_str ());

      app.version      = NVDRS_APPLICATION_VER;
      app.isPredefined = FALSE;
      app.isMetro      = FALSE;

      NVAPI_CALL2 (DRS_CreateApplication (hSession, hProfile, &app), ret);
      NVAPI_CALL2 (DRS_SaveSettings      (hSession), ret);
    }
  }

  NVDRS_SETTING get_val               = {               };
                get_val.version       = NVDRS_SETTING_VER;

  // These settings may not exist, and getting back a value of 0 is okay...
  NVAPI_SILENT  ();
  NVAPI_CALL    (DRS_GetSetting (hSession, hProfile, setting_id, &get_val));
  NVAPI_VERBOSE ();

  NVAPI_CALL (DRS_DestroySession (hSession));

  return get_val.u32CurrentValue;
}

bool
SK_NvAPI_DRS_SetDWORD (NvU32 setting_id, DWORD dwValue)
{
  if (! nv_hardware)
    return FALSE;


  NvAPI_Status       ret       = NVAPI_ERROR;
  NvDRSSessionHandle hSession  = { };

  NVAPI_CALL (DRS_CreateSession (&hSession));
  NVAPI_CALL (DRS_LoadSettings  ( hSession));

               NvDRSProfileHandle hProfile       = { };
  std::unique_ptr    <NVDRS_APPLICATION> app_ptr =
    std::make_unique <NVDRS_APPLICATION> ();
  NVDRS_APPLICATION&                     app     =
                                        *app_ptr;

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
    NVDRS_PROFILE custom_profile = {   };

    if (friendly_name.empty ()) // Avoid NVAPI failure: NVAPI_PROFILE_NAME_EMPTY
        friendly_name = app_name;

    custom_profile.isPredefined  = FALSE;
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
    {
      NVAPI_CALL2 ( DRS_FindProfileByName ( hSession,
                                              (NvU16 *)friendly_name.c_str (),
                                                &hProfile),
                      ret );
    }

    if (ret == NVAPI_OK)
    {
      RtlZeroMemory (app_ptr.get (), sizeof NVDRS_APPLICATION);

      lstrcpyW ((wchar_t *)app.appName,          app_name.c_str      ());
      lstrcpyW ((wchar_t *)app.userFriendlyName, friendly_name.c_str ());

      app.version      = NVDRS_APPLICATION_VER;
      app.isPredefined = FALSE;
      app.isMetro      = FALSE;

      NVAPI_CALL2 (DRS_CreateApplication (hSession, hProfile, &app), ret);
      NVAPI_CALL2 (DRS_SaveSettings      (hSession), ret);
    }
  }

  NVDRS_SETTING set_val               = {               };
                set_val.version       = NVDRS_SETTING_VER;

  // These settings may not exist, and getting back a value of 0 is okay...
  NVAPI_SILENT  ();
  NVAPI_CALL    (DRS_GetSetting (hSession, hProfile, setting_id, &set_val));
  NVAPI_VERBOSE ();


  BOOL already_set = TRUE;

  // Do this first so we don't touch other settings if this fails
  if (set_val.u32CurrentValue != dwValue)
  {
    ret = NVAPI_ERROR;

    // This requires admin privs, and we will handle that gracefully...
    NVAPI_SILENT    ();
    NVAPI_SET_DWORD (set_val, setting_id, dwValue);
    NVAPI_CALL2     (DRS_SetSetting (hSession, hProfile, &set_val), ret);
    NVAPI_VERBOSE   ();

    already_set = FALSE;
  }

  return
    (! already_set);
}

void
CALLBACK
RunDLL_NvAPI_SetDWORD ( HWND   hwnd,        HINSTANCE hInst,
                        LPCSTR lpszCmdLine, int       nCmdShow )
{
  UNREFERENCED_PARAMETER (hInst);
  UNREFERENCED_PARAMETER (hwnd);
  UNREFERENCED_PARAMETER (nCmdShow);

  char  szExecutable [MAX_PATH + 2] = { };
  DWORD dwSettingID                 = 0x0,
        dwSettingVal                = 0x0;
  BOOL  bClearSetting               = FALSE;

  int vals =
    sscanf (
      lpszCmdLine, "%x %x ",
        &dwSettingID, &dwSettingVal
    );

  if (vals != 2)
  {
    vals =
      sscanf (
        lpszCmdLine, "%x ~ ", // Clear any user-defined value
          &dwSettingID
      );

    if (vals != 1)
    {
      printf ("Arguments: <HexID> <HexValue|~> <ExecutableName.exe>");
      return;
    }

    else
      bClearSetting = TRUE;
  }

  // sscanf doesn't like strings with spaces, so we'll do it ourself :)
  strncpy ( szExecutable,
    StrStrIA (StrStrIA (lpszCmdLine, " ") + 1,
                                     " ") + 1, MAX_PATH );

  std::wstring executable_name =
    SK_UTF8ToWideChar (szExecutable);

  if (NVAPI::InitializeLibrary (executable_name.c_str ()))
  {
    app_name      = executable_name;
    friendly_name = executable_name;

    NvAPI_Status       ret       = NVAPI_ERROR;
    NvDRSSessionHandle hSession  = { };

    NVAPI_CALL (DRS_CreateSession (&hSession));
    NVAPI_CALL (DRS_LoadSettings  ( hSession));

                 NvDRSProfileHandle hProfile       = { };
    std::unique_ptr    <NVDRS_APPLICATION> app_ptr =
      std::make_unique <NVDRS_APPLICATION> ();
    NVDRS_APPLICATION&                     app     =
                                          *app_ptr;

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
      if ( IDYES == SK_MessageBox (
                  SK_FormatStringW ( L"Create New Driver Profile for \"%ws\"?",
                                       app_name.c_str ()
                                   ).c_str (),
                                     L"NVIDIA Driver Profile Does Not Exist",
                         MB_YESNO | MB_ICONQUESTION )
         )
      {
        NVDRS_PROFILE custom_profile = {   };

        if (friendly_name.empty ()) // Avoid NVAPI failure: NVAPI_PROFILE_NAME_EMPTY
            friendly_name = app_name;

        custom_profile.isPredefined  = FALSE;
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
        {
          NVAPI_CALL2 ( DRS_FindProfileByName ( hSession,
                                                  (NvU16 *)friendly_name.c_str (),
                                                    &hProfile),
                          ret );
        }

        if (ret == NVAPI_OK)
        {
          RtlZeroMemory (app_ptr.get (), sizeof NVDRS_APPLICATION);

          lstrcpyW ((wchar_t *)app.appName,          app_name.c_str      ());
          lstrcpyW ((wchar_t *)app.userFriendlyName, friendly_name.c_str ());

          app.version      = NVDRS_APPLICATION_VER;
          app.isPredefined = FALSE;
          app.isMetro      = FALSE;

          NVAPI_CALL2 (DRS_CreateApplication (hSession, hProfile, &app), ret);
          NVAPI_CALL2 (DRS_SaveSettings      (hSession), ret);
        }
      }

      else
        return;
    }

    if (ret == NVAPI_OK)
    {
      NVDRS_SETTING setting         = {               };
                    setting.version = NVDRS_SETTING_VER;

      if (! bClearSetting)
      {
        NVAPI_CALL    (DRS_GetSetting   (hSession, hProfile, dwSettingID, &setting));
        NVAPI_SET_DWORD (setting,                            dwSettingID, dwSettingVal);
        NVAPI_CALL    (DRS_SetSetting   (hSession, hProfile,              &setting));
      }

      else
      {
        NVAPI_CALL (DRS_DeleteProfileSetting (hSession, hProfile, dwSettingID));
      }

      if (ret != NVAPI_OK)
      {
        printf ("NvAPI_DRS_SetSetting (...) Failed: %x", ret);
      }

      NVAPI_CALL    (DRS_SaveSettings (hSession));
    }

    else
    {
      printf ("Unable to find Application Profile for %ws", app_name.c_str ());
    }

    NVAPI_CALL (DRS_DestroySession (hSession));
  }

  else
    printf ("NVAPI Failed to Initialize!");
}

void
CALLBACK
RunDLL_DisableGFEForSKIF ( HWND   hwnd,        HINSTANCE hInst,
                           LPCSTR lpszCmdLine, int       nCmdShow )
{
  UNREFERENCED_PARAMETER (hInst);
  UNREFERENCED_PARAMETER (hwnd);
  UNREFERENCED_PARAMETER (nCmdShow);

  if (NVAPI::InitializeLibrary (L"SKIF"))
  {
    app_name      = L"SKIF.exe";
    friendly_name = L"SKIF";

    SK_NvAPI_SetVRREnablement (FALSE);

    if (SK_IsAdmin ())
    {
      SK_NvAPI_AllowGFEOverlay  (false, L"SKIF", L"SKIF.exe");
    }

    else
    {
      if (! StrStrIA (lpszCmdLine, "silent"))
      {
        MessageBox (
          NULL, L"This command must be run as admin.",
             L"Disable GFE For SKIF Failed", MB_OK
        );
      }
    }
  }
}


INT
SK_NvAPI_GetAnselEnablement (DLL_ROLE role)
{
  if (role == OpenGL || role == Vulkan)
    return -1;

  if (! nv_hardware)
    return -2;

  NvAPI_Status       ret       = NVAPI_ERROR;
  NvDRSSessionHandle hSession  = { };

  NVAPI_CALL (DRS_CreateSession (&hSession));
  NVAPI_CALL (DRS_LoadSettings  ( hSession));

               NvDRSProfileHandle hProfile       = { };
  std::unique_ptr    <NVDRS_APPLICATION> app_ptr =
    std::make_unique <NVDRS_APPLICATION> ();
  NVDRS_APPLICATION&                     app     =
                                        *app_ptr;


  NvU32 ansel_allow_enum       = ANSEL_ALLOW_ID;
  NvU32 ansel_enable_enum      = ANSEL_ENABLE_ID;
  NvU32 ansel_allowlisted_enum = ANSEL_ALLOWLISTED_ID;

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
    NVDRS_PROFILE custom_profile = {   };

    if (friendly_name.empty ()) // Avoid NVAPI failure: NVAPI_PROFILE_NAME_EMPTY
        friendly_name = app_name;

    custom_profile.isPredefined  = FALSE;
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
    {
      NVAPI_CALL2 ( DRS_FindProfileByName ( hSession,
                                              (NvU16 *)friendly_name.c_str (),
                                                &hProfile),
                      ret );
    }

    if (ret == NVAPI_OK)
    {
      RtlZeroMemory (app_ptr.get (), sizeof NVDRS_APPLICATION);

      lstrcpyW ((wchar_t *)app.appName,          app_name.c_str      ());
      lstrcpyW ((wchar_t *)app.userFriendlyName, friendly_name.c_str ());

      app.version      = NVDRS_APPLICATION_VER;
      app.isPredefined = FALSE;
      app.isMetro      = FALSE;

      NVAPI_CALL2 (DRS_CreateApplication (hSession, hProfile, &app), ret);
      NVAPI_CALL2 (DRS_SaveSettings      (hSession), ret);
    }
  }

  NVDRS_SETTING ansel_allow_val               = {               };
                ansel_allow_val.version       = NVDRS_SETTING_VER;

  NVDRS_SETTING ansel_enable_val              = {               };
                ansel_enable_val.version      = NVDRS_SETTING_VER;

  NVDRS_SETTING ansel_allowlisted_val         = {               };
                ansel_allowlisted_val.version = NVDRS_SETTING_VER;

  // These settings may not exist, and getting back a value of 0 is okay...
  NVAPI_SILENT  ();
  NVAPI_CALL    (DRS_GetSetting (hSession, hProfile, ansel_allow_enum,       &ansel_allow_val));
  NVAPI_CALL    (DRS_GetSetting (hSession, hProfile, ansel_enable_enum,      &ansel_enable_val));
  NVAPI_CALL    (DRS_GetSetting (hSession, hProfile, ansel_allowlisted_enum, &ansel_allowlisted_val));
  NVAPI_VERBOSE ();

  INT iRet =
   ( ansel_allow_val.u32CurrentValue       == ANSEL_ALLOW_ALLOWED    &&
     ansel_enable_val.u32CurrentValue      == ANSEL_ENABLE_ON        &&
     ansel_allowlisted_val.u32CurrentValue == ANSEL_ALLOWLISTED_ALLOWED )
                                            ? 1
                                            : 0;

  NVAPI_CALL (DRS_DestroySession (hSession));

  return iRet;
}

BOOL
SK_NvAPI_SetAnselEnablement (DLL_ROLE role, bool enabled)
{
  if (role == OpenGL || role == Vulkan)
    return FALSE;

  if (! nv_hardware)
    return FALSE;


  // Forcefully block the Ansel DLL
  config.nvidia.bugs.bypass_ansel = (! enabled);



  NvAPI_Status       ret       = NVAPI_ERROR;
  NvDRSSessionHandle hSession  = { };

  NVAPI_CALL (DRS_CreateSession (&hSession));
  NVAPI_CALL (DRS_LoadSettings  ( hSession));

               NvDRSProfileHandle hProfile       = { };
  std::unique_ptr    <NVDRS_APPLICATION> app_ptr =
    std::make_unique <NVDRS_APPLICATION> ();
  NVDRS_APPLICATION&                     app     =
                                        *app_ptr;


  NvU32 ansel_allow_enum       = ANSEL_ALLOW_ID;
  NvU32 ansel_enable_enum      = ANSEL_ENABLE_ID;
  NvU32 ansel_allowlisted_enum = ANSEL_ALLOWLISTED_ID;

  NvU32 ansel_allow_to_set =
    enabled ? ANSEL_ALLOW_ALLOWED
            : ANSEL_ALLOW_DISALLOWED;

  NvU32 ansel_enable_to_set =
    enabled ? ANSEL_ENABLE_ON
            : ANSEL_ENABLE_OFF;

  NvU32 ansel_allowlisted_to_set =
    enabled ? ANSEL_ALLOWLISTED_ALLOWED
            : ANSEL_ALLOWLISTED_DISALLOWED;

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
    NVDRS_PROFILE custom_profile = {   };

    if (friendly_name.empty ()) // Avoid NVAPI failure: NVAPI_PROFILE_NAME_EMPTY
        friendly_name = app_name;

    custom_profile.isPredefined  = FALSE;
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
    {
      NVAPI_CALL2 ( DRS_FindProfileByName ( hSession,
                                              (NvU16 *)friendly_name.c_str (),
                                                &hProfile),
                      ret );
    }

    if (ret == NVAPI_OK)
    {
      RtlZeroMemory (app_ptr.get (), sizeof NVDRS_APPLICATION);

      lstrcpyW ((wchar_t *)app.appName,          app_name.c_str      ());
      lstrcpyW ((wchar_t *)app.userFriendlyName, friendly_name.c_str ());

      app.version      = NVDRS_APPLICATION_VER;
      app.isPredefined = FALSE;
      app.isMetro      = FALSE;

      NVAPI_CALL2 (DRS_CreateApplication (hSession, hProfile, &app), ret);
      NVAPI_CALL2 (DRS_SaveSettings      (hSession), ret);
    }
  }

  NVDRS_SETTING ansel_allow_val               = {               };
                ansel_allow_val.version       = NVDRS_SETTING_VER;

  NVDRS_SETTING ansel_enable_val              = {               };
                ansel_enable_val.version      = NVDRS_SETTING_VER;

  NVDRS_SETTING ansel_allowlisted_val         = {               };
                ansel_allowlisted_val.version = NVDRS_SETTING_VER;

  // These settings may not exist, and getting back a value of 0 is okay...
  NVAPI_SILENT ();
  NVAPI_CALL (DRS_GetSetting (hSession, hProfile, ansel_allow_enum,       &ansel_allow_val));
  NVAPI_CALL (DRS_GetSetting (hSession, hProfile, ansel_enable_enum,      &ansel_enable_val));
  NVAPI_CALL (DRS_GetSetting (hSession, hProfile, ansel_allowlisted_enum, &ansel_allowlisted_val));
  NVAPI_VERBOSE ();


  BOOL already_set = TRUE;

  // Do this first so we don't touch other settings if this fails
  if (ansel_allow_val.u32CurrentValue != ansel_allow_to_set)
  {
    ansel_allow_val         = {               };
    ansel_allow_val.version = NVDRS_SETTING_VER;

    ret = NVAPI_ERROR;

    // This requires admin privs, and we will handle that gracefully...
    NVAPI_SILENT    ();
    NVAPI_SET_DWORD (ansel_allow_val, ansel_allow_enum, ansel_allow_to_set);
    NVAPI_CALL2     (DRS_SetSetting (hSession, hProfile, &ansel_allow_val), ret);
    NVAPI_VERBOSE   ();

    already_set = FALSE;
  }

  if (ansel_enable_val.u32CurrentValue != ansel_enable_to_set)
  {
    ansel_enable_val         = {               };
    ansel_enable_val.version = NVDRS_SETTING_VER;

    NVAPI_SET_DWORD (ansel_enable_val, ansel_enable_enum, ansel_enable_to_set);
    NVAPI_CALL      (DRS_SetSetting (hSession, hProfile, &ansel_enable_val));

    already_set = FALSE;
  }

  bool not_allowlisted = false;

  if (ansel_allowlisted_val.u32CurrentValue != ansel_allowlisted_to_set)
  {
    ansel_allowlisted_val         = {               };
    ansel_allowlisted_val.version = NVDRS_SETTING_VER;

    NVAPI_SET_DWORD (ansel_allowlisted_val, ansel_allowlisted_enum, ansel_allowlisted_to_set);
    NVAPI_CALL      (DRS_SetSetting (hSession, hProfile, &ansel_allowlisted_val));

    already_set = FALSE;
  }

  else
    not_allowlisted = true;

  if (! already_set)
  {
    NVAPI_CALL (DRS_SaveSettings   (hSession));
  } NVAPI_CALL (DRS_DestroySession (hSession));


  // We turned off the remaining Ansel profile flags, but it was already not allowlisted.
  if ((! enabled) && not_allowlisted)
    return false;


  return
    (! already_set);
}

BOOL
SK_NvAPI_EnableAnsel (DLL_ROLE role)
{
  return
    SK_NvAPI_SetAnselEnablement (role, true);
}

BOOL
SK_NvAPI_DisableAnsel (DLL_ROLE role)
{
  return
    SK_NvAPI_SetAnselEnablement (role, false);
}

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
  std::unique_ptr    <NVDRS_APPLICATION> app_ptr =
    std::make_unique <NVDRS_APPLICATION> ();
  NVDRS_APPLICATION&                     app     = *app_ptr;

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
    dll_log->Log ( L"[  NvAPI   ] >> Unknown SLI Mode: '%s', falling back to 'Auto'",
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
    dll_log->Log ( L"[  NvAPI   ] >> Unknown SLI GPU Count: '%s', falling back to 'Auto'",
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

    if (friendly_name.empty ()) // Avoid NVAPI failure: NVAPI_PROFILE_NAME_EMPTY
        friendly_name = app_name;

    custom_profile.isPredefined = FALSE;
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
      RtlZeroMemory (app_ptr.get (), sizeof NVDRS_APPLICATION);

      lstrcpyW ((wchar_t *)app.appName,          app_name.c_str      ());
      lstrcpyW ((wchar_t *)app.userFriendlyName, friendly_name.c_str ());

      app.version      = NVDRS_APPLICATION_VER;
      app.isPredefined = FALSE;
      app.isMetro      = FALSE;

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

      if (result == IDCANCEL)
      {
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

                     NvDRSProfileHandle hProfile = { };
  std::unique_ptr    <NVDRS_APPLICATION> app_ptr =
    std::make_unique <NVDRS_APPLICATION> ();
  NVDRS_APPLICATION&                     app     = *app_ptr;

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
    custom_profile.isPredefined  = FALSE;

    if (friendly_name.empty ()) // Avoid NVAPI failure: NVAPI_PROFILE_NAME_EMPTY
        friendly_name = app_name;

    lstrcpyW (
      (wchar_t *)custom_profile.profileName,
                              friendly_name.c_str ()
    );
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
      RtlZeroMemory (app_ptr.get (), sizeof (NVDRS_APPLICATION));
      *app.appName = L'\0';

      //lstrcpyW ((wchar_t *)app.appName,          app_name.c_str      ());
      lstrcpyW ((wchar_t *)app.userFriendlyName, friendly_name.c_str ());
      lstrcpyW ((wchar_t *)app.launcher,         launcher_name.c_str ());
      app.version      = NVDRS_APPLICATION_VER;
      app.isPredefined = FALSE;
      app.isMetro      = FALSE;

      NVAPI_CALL2 (DRS_CreateApplication (hSession, hProfile, &app), ret);
      NVAPI_CALL2 (DRS_SaveSettings      (hSession), ret);
    }
  } else {
    RtlZeroMemory (app_ptr.get (), sizeof (NVDRS_APPLICATION));
    *app.appName = L'\0';

    //lstrcpyW ((wchar_t *)app.appName,          app_name.c_str      ());
    lstrcpyW ((wchar_t *)app.userFriendlyName, friendly_name.c_str ());
    lstrcpyW ((wchar_t *)app.launcher,         launcher_name.c_str ());
    app.version      = NVDRS_APPLICATION_VER;
    app.isPredefined = FALSE;
    app.isMetro      = FALSE;

    NVAPI_CALL2 (DRS_CreateApplication (hSession, hProfile, &app), ret);
    NVAPI_CALL2 (DRS_SaveSettings      (hSession), ret);
  }

  NVAPI_CALL (DRS_SaveSettings   (hSession));
  NVAPI_CALL (DRS_DestroySession (hSession));

#if 0
  if (! already_set) {
#ifdef WIN32
    HMODULE hLib = SK_LoadLibraryW (L"nvapi.dll");
#else
    HMODULE hLib = SK_LoadLibraryW (L"nvapi64.dll");
#endif
#define __NvAPI_RestartDisplayDriver                      0xB4B26B65
    typedef void* (*NvAPI_QueryInterface_pfn)(unsigned int offset);
    typedef NvAPI_Status(__cdecl *NvAPI_RestartDisplayDriver_pfn)(void);
    NvAPI_QueryInterface_pfn       NvAPI_QueryInterface       =
      (NvAPI_QueryInterface_pfn)SK_GetProcAddress (hLib, "nvapi_QueryInterface");
    NvAPI_RestartDisplayDriver_pfn NvAPI_RestartDisplayDriver =
      (NvAPI_RestartDisplayDriver_pfn)NvAPI_QueryInterface (__NvAPI_RestartDisplayDriver);

    NvAPI_RestartDisplayDriver ();
  }
#endif

  return true;
}

bool         NVAPI::nv_hardware   = false;
std::wstring NVAPI::friendly_name;
std::wstring NVAPI::app_name;
std::wstring NVAPI::launcher_name;

//std::wstring NVAPI::friendly_name; = L"Tales of Zestiria";
//std::wstring NVAPI::app_name; = L"Tales Of Zestiria.exe";
//std::wstring NVAPI::launcher_name; = L"";