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

#include <d3d9.h>
#include <d3d11.h>

#include <SpecialK/nvapi.h>
#include <nvapi/NvApiDriverSettings.h>
#include <nvapi/nvapi_lite_sli.h>

typedef NvAPI_Status (__cdecl *NvAPI_Disp_GetHdrCapabilities_pfn)(NvU32, NV_HDR_CAPABILITIES*);
typedef NvAPI_Status (__cdecl *NvAPI_Disp_HdrColorControl_pfn)   (NvU32, NV_HDR_COLOR_DATA*);

NvAPI_Disp_GetHdrCapabilities_pfn NvAPI_Disp_GetHdrCapabilities_Original = nullptr;
NvAPI_Disp_HdrColorControl_pfn    NvAPI_Disp_HdrColorControl_Original    = nullptr;

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


#include <SpecialK/render/dxgi/dxgi_hdr.h>

#define __SK_SUBSYSTEM__ L"  Nv API  "

NvAPI_Status
__cdecl
NvAPI_Disp_GetHdrCapabilities_Override ( NvU32                displayId,
                                         NV_HDR_CAPABILITIES *pHdrCapabilities )
{
  if (config.apis.NvAPI.disable_hdr)
    return NVAPI_LIBRARY_NOT_FOUND;

  SK_LOG_FIRST_CALL

  SK_LOG0 ( ( L"NV_HDR_CAPABILITIES Version: %lu", pHdrCapabilities->version ),
              __SK_SUBSYSTEM__ );
  SK_LOG0 ( ( L" >> Wants Driver to Expand Default HDR Params: %s",
                pHdrCapabilities->driverExpandDefaultHdrParameters ? L"Yes" :
                                                                     L"No" ),
              __SK_SUBSYSTEM__ );

  NvAPI_Status ret =
    NvAPI_Disp_GetHdrCapabilities_Original ( displayId, pHdrCapabilities );

  ////SK_DXGI_HDRControl* pHDRCtl =
  ////  SK_HDR_GetControl ();


    if ( pHdrCapabilities->isST2084EotfSupported ||
         pHdrCapabilities->isTraditionalHdrGammaSupported )
    {
      auto& rb =
        SK_GetCurrentRenderBackend ();

      rb.driver_based_hdr = true;
          rb.setHDRCapable (true);
    }

    dll_log->LogEx ( true,
      L"[ HDR Caps ]\n"
      L"  +-----------------+---------------------\n"
      L"  | Red Primary.... |  %f, %f\n"
      L"  | Green Primary.. |  %f, %f\n"
      L"  | Blue Primary... |  %f, %f\n"
      L"  | White Point.... |  %f, %f\n"
      L"  | Min Luminance.. |  %f\n"
      L"  | Max Luminance.. |  %f\n"
      L"  |  \"  FullFrame.. |  %f\n"
      L"  | EDR Support.... |  %s\n"
      L"  | ST2084 Gamma... |  %s\n"
      L"  | HDR Gamma...... |  %s\n"
      L"  | SDR Gamma...... |  %s\n"
      L"  +-----------------+---------------------\n",
        pHdrCapabilities->display_data.displayPrimary_x0,   pHdrCapabilities->display_data.displayPrimary_y0,
        pHdrCapabilities->display_data.displayPrimary_x1,   pHdrCapabilities->display_data.displayPrimary_y1,
        pHdrCapabilities->display_data.displayPrimary_x2,   pHdrCapabilities->display_data.displayPrimary_y2,
        pHdrCapabilities->display_data.displayWhitePoint_x, pHdrCapabilities->display_data.displayWhitePoint_y,
        (float)pHdrCapabilities->display_data.desired_content_min_luminance,
        (float)pHdrCapabilities->display_data.desired_content_max_luminance,
        (float)pHdrCapabilities->display_data.desired_content_max_frame_average_luminance,
               pHdrCapabilities->isEdrSupported                 ? L"Yes" : L"No",
               pHdrCapabilities->isST2084EotfSupported          ? L"Yes" : L"No",
               pHdrCapabilities->isTraditionalHdrGammaSupported ? L"Yes" : L"No",
               pHdrCapabilities->isTraditionalSdrGammaSupported ? L"Yes" : L"No" );


  if (ret == NVAPI_OK)
  {
  //pHDRCtl->devcaps.BitsPerColor          = 10;
    //pHDRCtl->devcaps.RedPrimary   [0]      = ((float)pHdrCapabilities->display_data.displayPrimary_x0) / (float)0xC350;
    //pHDRCtl->devcaps.RedPrimary   [1]      = ((float)pHdrCapabilities->display_data.displayPrimary_y0) / (float)0xC350;
    //
    //pHDRCtl->devcaps.GreenPrimary [0]      = ((float)pHdrCapabilities->display_data.displayPrimary_x1) / (float)0xC350;
    //pHDRCtl->devcaps.GreenPrimary [1]      = ((float)pHdrCapabilities->display_data.displayPrimary_y1) / (float)0xC350;
    //
    //pHDRCtl->devcaps.BluePrimary  [0]      = ((float)pHdrCapabilities->display_data.displayPrimary_x2) / (float)0xC350;
    //pHDRCtl->devcaps.BluePrimary  [1]      = ((float)pHdrCapabilities->display_data.displayPrimary_y2) / (float)0xC350;
    //
    //pHDRCtl->devcaps.WhitePoint   [0]      = ((float)pHdrCapabilities->display_data.displayWhitePoint_x) / (float)0xC350;
    //pHDRCtl->devcaps.WhitePoint   [1]      = ((float)pHdrCapabilities->display_data.displayWhitePoint_y) / (float)0xC350;
    //
  ////pHDRCtl->devcaps.ColorSpace            = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
    //pHDRCtl->devcaps.MinLuminance          = (float)pHdrCapabilities->display_data.desired_content_min_luminance;
    //pHDRCtl->devcaps.MaxLuminance          = (float)pHdrCapabilities->display_data.desired_content_max_luminance;
    //pHDRCtl->devcaps.MaxFullFrameLuminance = (float)pHdrCapabilities->display_data.desired_content_max_frame_average_luminance;
  }

  return ret;
}

static
std::pair < NvAPI_Status,
            std::pair < NvU32,
                        NV_HDR_COLOR_DATA >
          >
  SK_NvAPI_LastHdrColorControl (
    std::make_pair   ( (NvAPI_Status)NVAPI_ERROR,
      std::make_pair ( (NvU32)0, NV_HDR_COLOR_DATA { } )
    )
);

void
SK_NvAPI_ReIssueLastHdrColorControl (void)
{
  if (SK_NvAPI_LastHdrColorControl.first == NVAPI_OK)
  {
    auto& reissue =
       SK_NvAPI_LastHdrColorControl.second;
    //auto off_copy         = reissue.second;
    //     off_copy.cmd     = NV_HDR_CMD_SET;
    //     off_copy.hdrMode = NV_HDR_MODE_OFF;
    //NvAPI_Disp_HdrColorControl_Original (reissue.first, &off_copy);
    NvAPI_Disp_HdrColorControl (reissue.first, &reissue.second);
  }
}

#include <SpecialK/render/dxgi/dxgi_swapchain.h>

NvAPI_Status
__cdecl
NvAPI_Disp_HdrColorControl_Override ( NvU32              displayId,
                                      NV_HDR_COLOR_DATA *pHdrColorData )
{
  if (config.apis.NvAPI.disable_hdr)
    return NVAPI_LIBRARY_NOT_FOUND;

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  SK_LOG_FIRST_CALL

  static NV_HDR_COLOR_DATA_V2  expandedData;
         NV_HDR_COLOR_DATA_V1 *inputData = (NV_HDR_COLOR_DATA_V1 *)pHdrColorData;

  if (pHdrColorData->version == NV_HDR_COLOR_DATA_VER1)
  {
    NV_HDR_COLOR_DATA_V1    origData =     *((NV_HDR_COLOR_DATA_V1 *)pHdrColorData);
    memcpy (&expandedData, &origData, sizeof (NV_HDR_COLOR_DATA_V1));

    expandedData.version   = NV_HDR_COLOR_DATA_VER2;
    pHdrColorData          = &expandedData;

    pHdrColorData->cmd                           = inputData->cmd;
    pHdrColorData->hdrMode                       = inputData->hdrMode;
    pHdrColorData->static_metadata_descriptor_id = inputData->static_metadata_descriptor_id;

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
  SK_LOG0 ( ( L"<%s> HDR Mode:    %s", pHdrColorData->cmd == NV_HDR_CMD_GET    ?
                                                        L"Get"                 :
                                       pHdrColorData->cmd == NV_HDR_CMD_SET    ?
                                                        L"Set"                 :
                                       pHdrColorData->cmd == NV_COLOR_CMD_IS_SUPPORTED_COLOR ?
                                                        L"Check Color Support"               :
                                       pHdrColorData->cmd == NV_COLOR_CMD_GET_DEFAULT        ?
                                                        L"Get Default"                       :
                                                        L"Unknown",
                                       HDRModeToStr (pHdrColorData->hdrMode) ),
              __SK_SUBSYSTEM__ );

  //SK_DXGI_HDRControl* pHDRCtl =
  //  SK_HDR_GetControl ();

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
        { { L"No Metadata",             NV_DYNAMIC_RANGE_VESA  },
          { L"CEA-861.3, SMPTE ST2086", NV_DYNAMIC_RANGE_CEA   },
          { L"[AUTO]",                  NV_DYNAMIC_RANGE_AUTO  } };

    SK_LOG0 ( ( L"HDR:  Max Master Luma: %7.1f, Min Master Luma: %7.5f",
      static_cast <double> (pHdrColorData->mastering_display_data.max_display_mastering_luminance),
      static_cast <double> (
        static_cast <double>
                          (pHdrColorData->mastering_display_data.min_display_mastering_luminance) * 0.0001
                          )
              ), __SK_SUBSYSTEM__ );

    SK_LOG0 ( ( L"HDR:  Max Avg. Luma: %7.1f, Max Luma: %7.1f",
      static_cast <double> (pHdrColorData->mastering_display_data.max_frame_average_light_level),
      static_cast <double> (pHdrColorData->mastering_display_data.max_content_light_level)
              ), __SK_SUBSYSTEM__ );

    SK_LOG0 ( ( L"HDR:  Color ( Bit-Depth: %s, Sampling: %s ), Dynamic Range: %s",
                bpc_map           [pHdrColorData->hdrBpc].         c_str (),
                color_fmt_map     [pHdrColorData->hdrColorFormat]. c_str (),
                dynamic_range_map [pHdrColorData->hdrDynamicRange].c_str ()
              ), __SK_SUBSYSTEM__ );
  };

  auto _Push_NvAPI_HDR_Metadata_to_DXGI_Backend = [&](void) ->
  void
  {
    rb.working_gamut.maxY =
      pHdrColorData->mastering_display_data.max_display_mastering_luminance * 0.00001f;
    rb.working_gamut.minY =
     pHdrColorData->mastering_display_data.min_display_mastering_luminance  *  0.0001f;

    rb.working_gamut.maxLocalY =
      pHdrColorData->mastering_display_data.max_frame_average_light_level   * 0.00001f;

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

    //bool passthrough = (pHdrColorData->hdrMode == NV_HDR_MODE_UHDA_PASSTHROUGH);

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
      NvAPI_Disp_HdrColorControl_Original ( displayId, pHdrColorData );

    if (NVAPI_OK == ret)
    {
      if (pHdrColorData->hdrMode != NV_HDR_MODE_OFF)
      {
        rb.setHDRCapable       (true);
        rb.driver_based_hdr   = true;
        rb.framebuffer_flags |=  SK_FRAMEBUFFER_FLAG_HDR;

        SK_NvAPI_LastHdrColorControl =
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
      }

      else
      {
        rb.driver_based_hdr         = false;
        rb.scanout.nvapi_hdr.active = false;

        extern bool __SK_HDR_10BitSwap;
        extern bool __SK_HDR_16BitSwap;

        if ( __SK_HDR_10BitSwap == false &&
             __SK_HDR_16BitSwap == false )
        {
          rb.framebuffer_flags &= ~SK_FRAMEBUFFER_FLAG_HDR;
        }
      }

      _Push_NvAPI_HDR_Metadata_to_DXGI_Backend ();
    }

    return ret;
  }

  ////pHDRCtl->meta._AdjustmentCount++;

  NvAPI_Status ret =
    NvAPI_Disp_HdrColorControl_Original ( displayId, pHdrColorData );

  if (pHdrColorData->cmd == NV_HDR_CMD_GET)
  {
    _LogGameRequestedValues ();

    if (ret == NVAPI_OK)
    {
      rb.scanout.nvapi_hdr.mode           = pHdrColorData->hdrMode;
      rb.scanout.nvapi_hdr.color_format   = pHdrColorData->hdrColorFormat;
      rb.scanout.nvapi_hdr.dynamic_range  = pHdrColorData->hdrDynamicRange;
      rb.scanout.nvapi_hdr.bpc            = pHdrColorData->hdrBpc;
      rb.scanout.nvapi_hdr.active         =(pHdrColorData->hdrMode != NV_HDR_MODE_OFF);

      _Push_NvAPI_HDR_Metadata_to_DXGI_Backend ();
    }
  }

  if (inputData->version == NV_HDR_COLOR_DATA_VER1)
  {
    inputData->hdrMode                       = pHdrColorData->hdrMode;

    memcpy (     &inputData->mastering_display_data,
             &pHdrColorData->mastering_display_data,
          sizeof (inputData->mastering_display_data) );

    inputData->static_metadata_descriptor_id = pHdrColorData->static_metadata_descriptor_id;
  }

  return ret;
}


using NvAPI_QueryInterface_pfn = void* (*)(unsigned int ordinal);
      NvAPI_QueryInterface_pfn
      NvAPI_QueryInterface_Original = nullptr;

#include <concurrent_unordered_set.h>

void*
NvAPI_QueryInterface_Detour (unsigned int ordinal)
{
  static Concurrency::concurrent_unordered_set <unsigned int> logged_ordinals;

  if (logged_ordinals.count (ordinal) == 0)
  {
    logged_ordinals.insert (ordinal);

    dll_log->Log (L"NvAPI Ordinal: %lu  --  %s", ordinal, SK_SummarizeCaller ().c_str ());
  }

  return NvAPI_QueryInterface_Original (ordinal);
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
      GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_PIN, L"nvapi64.dll", &hLib);
#else
    HMODULE hLib =
      SK_Modules->LoadLibraryLL (L"nvapi.dll");

    if (hLib)
    {
      GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_PIN, L"nvapi.dll",   &hLib);
#endif

      static auto NvAPI_QueryInterface =
        reinterpret_cast <NvAPI_QueryInterface_pfn> (
          GetProcAddress (hLib, "nvapi_QueryInterface")
        );

      if (NvAPI_QueryInterface != nullptr)
      {
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
      }
    }
  }
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


      if (NvAPI_Disp_HdrColorControl_Original == nullptr)
      {
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
      }


      if (nv_hardware && NvAPI_Disp_GetHdrCapabilities_Original != nullptr)
      {
        NV_HDR_CAPABILITIES hdr_caps         = { };
        hdr_caps.version = NV_HDR_CAPABILITIES_VER;

        NvU32 DefaultOutputId = 0;

        if ( NVAPI_OK ==
               NvAPI_GetAssociatedDisplayOutputId (NVAPI_DEFAULT_HANDLE, &DefaultOutputId) )
        {
          NvAPI_Disp_GetHdrCapabilities_Override ( DefaultOutputId, &hdr_caps );
        }
      }

  //    SK_CreateDLLHook2 ( L"nvapi64.dll",
  //                         "nvapi_QueryInterface",
  //                          NvAPI_QueryInterface_Detour,
  // static_cast_p2p <void> (&NvAPI_QueryInterface_Original) );

//#ifdef SK_AGGRESSIVE_HOOKS
//      SK_ApplyQueuedHooks ();
//#endif
    }

    else {
      dll_log->Log (L"unable to complete LoadLibrary (...) ");
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
      RtlSecureZeroMemory (app_ptr.get (), sizeof NVDRS_APPLICATION);

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
      dll_log->LogEx ( L"[  NvAPI   ] Could not find or create application profile for '%s' (%s)",
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
    HMODULE hLib = SK_Modules->LoadLibraryLL (L"nvapi.dll");
#else
    HMODULE hLib = SK_Modules->LoadLibraryLL (L"nvapi64.dll");
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
      RtlSecureZeroMemory (app_ptr.get (), sizeof NVDRS_APPLICATION);

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
        config.nvidia.sli.override = FALSE;
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
      RtlSecureZeroMemory (app_ptr.get (), sizeof (NVDRS_APPLICATION));
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
    RtlSecureZeroMemory (app_ptr.get (), sizeof (NVDRS_APPLICATION));
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