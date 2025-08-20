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

#ifndef __SK__NVAPI_H__
#define __SK__NVAPI_H__

struct IUnknown;
#include <Unknwnbase.h>

#include <nvapi/nvapi.h>
#include <Windows.h>

#include <string>

#include <SpecialK/core.h>

struct DXGI_ADAPTER_DESC;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NV_PCIE_INFO_UNKNOWN
{
  NvU32 unknown0;
  NvU32 unknown1;
  NvU32 unknown2;
  NvU32 unknown3;
  NvU32 unknown4;
  NvU32 unknown5;
  NvU32 unknown6;
  NvU32 unknown7;
} NV_PCIE_INFO_UNKNOWN;

typedef struct _NV_PCIE_INFO
{
  NvU32                   version;          //!< Structure version
  NV_PCIE_INFO_UNKNOWN    info [5];
} NV_PCIE_INFO;

#define NV_PCIE_INFO_VER   MAKE_NVAPI_VERSION(NV_PCIE_INFO, 2)

#if 0
typedef struct
{
  NvU32                   version;                            //!< Structure version

  struct {
    NvU32                   pciLinkTransferRate;
    NVVIOPCILINKRATE        pciLinkRate;                      //!< specifies the the negotiated PCIE link rate.
    NVVIOPCILINKWIDTH       pciLinkWidth;                     //!< specifies the the negotiated PCIE link width.
    NVVIOPCILINKRATE        pciLinkVersion;                   //!< specifies the the negotiated PCIE link rate.
  } pstates [NVAPI_MAX_GPU_PERF_PSTATES];
} NV_GPU_PCIE_INFO_V2;

typedef NV_GPU_PCIE_INFO_V2 NV_GPU_PCIE_INFO;

#define NV_GPU_PCIE_INFO_VER_2  MAKE_NVAPI_VERSION(NV_GPU_PCIE_INFO_V2,2)
#define NV_GPU_PCIE_INFO_VER    NV_GPU_PCIE_INFO_VER_2
#endif

typedef enum _NV_DITHER_STATE
{
  NV_DITHER_STATE_DEFAULT  = 0,
  NV_DITHER_STATE_ENABLED  = 1,
  NV_DITHER_STATE_DISABLED = 2,
  NV_DITHER_STATE_MAX      = 0xFF
} NV_DITHER_STATE;

typedef enum _NV_DITHER_BITS
{
  NV_DITHER_BITS_6   = 0,
  NV_DITHER_BITS_8   = 1,
  NV_DITHER_BITS_10  = 2,
  NV_DITHER_BITS_MAX = 0xFF
} NV_DITHER_BITS;

typedef enum _NV_DITHER_MODE
{
  NV_DITHER_MODE_SPATIAL_DYNAMIC     = 0, // "Spatial Dynamic"
  NV_DITHER_MODE_SPATIAL_STATIC      = 1, // "Spatial Static""
  NV_DITHER_MODE_SPATIAL_DYNAMIC_2x2 = 2, // "Spatial Dynamic 2x2"
  NV_DITHER_MODE_SPATIAL_STATIC_2x2  = 3, // "Spatial Static 2x2"
  NV_DITHER_MODE_TEMPORAL            = 4, // "Temporal"
  NV_DITHER_MODE_MAX                 = 0xFF
} NV_DITHER_MODE;

typedef struct _NV_GPU_DITHER_CONTROL_V1
{
  NvU32         version;
  NV_DITHER_STATE state;
  NV_DITHER_BITS   bits;
  NV_DITHER_MODE   mode;
  struct {
    NvU32          bits;
    NvU32          mode;
  } caps;
} NV_GPU_DITHER_CONTROL_V1;

#define NV_GPU_DITHER_CONTROL_VER1  MAKE_NVAPI_VERSION(NV_GPU_DITHER_CONTROL_V1, 1)

using NvAPI_Disp_SetDitherControl_pfn =
      NvAPI_Status (__cdecl *)( NvPhysicalGpuHandle hPhysicalGpu,
                                NvU32               output_id,
                                NV_DITHER_STATE     state,
                                NV_DITHER_BITS      bits,
                                NV_DITHER_MODE      mode );

using NvAPI_Disp_GetDitherControl_pfn =
      NvAPI_Status (__cdecl *)( NvU32                     output_id,
                                NV_GPU_DITHER_CONTROL_V1* ditherControl );

#include <Unknwn.h>

typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetPCIEInfo_pfn)
    (NvPhysicalGpuHandle handle, NV_PCIE_INFO* info);
typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetRamType_pfn)
    (NvPhysicalGpuHandle handle, NvU32* memtype);
typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetFBWidthAndLocation_pfn)
    (NvPhysicalGpuHandle handle, NvU32* width, NvU32* loc);
typedef NvAPI_Status (__cdecl *NvAPI_GetPhysicalGPUFromGPUID_pfn)
    (NvU32 gpuid, NvPhysicalGpuHandle* gpu);
typedef NvAPI_Status (__cdecl *NvAPI_GetGPUIDFromPhysicalGPU_pfn)
    (NvPhysicalGpuHandle gpu, NvU32* gpuid);

typedef NvAPI_Status (__cdecl *NvAPI_D3D_IsGSyncCapable_pfn)
    (IUnknown *pDeviceOrContext, NVDX_ObjectHandle primarySurface, BOOL *pIsGsyncCapable);
typedef NvAPI_Status (__cdecl *NvAPI_D3D_IsGSyncActive_pfn)
    (IUnknown *pDeviceOrContext, NVDX_ObjectHandle primarySurface, BOOL *pIsGsyncActive);
typedef NvAPI_Status (__cdecl *NvAPI_D3D_GetObjectHandleForResource_pfn)
    (IUnknown *pDevice, IUnknown *pResource, NVDX_ObjectHandle *pHandle);

#define __NvAPI_D3D_IsGSyncCapable             0x9C1EED78
#define __NvAPI_D3D_IsGSyncActive              0x0E942B0F
#define __NvAPI_D3D_GetObjectHandleForResource 0xFCEAC864

#define __NvAPI_Disp_SetDitherControl          0xDF0DFCDD
#define __NvAPI_Disp_GetDitherControl          0x932AC8FB

extern NvAPI_GPU_GetRamType_pfn                 NvAPI_GPU_GetRamType;
extern NvAPI_GPU_GetFBWidthAndLocation_pfn      NvAPI_GPU_GetFBWidthAndLocation;
extern NvAPI_GPU_GetPCIEInfo_pfn                NvAPI_GPU_GetPCIEInfo;
extern NvAPI_GetGPUIDFromPhysicalGPU_pfn        NvAPI_GetGPUIDFromPhysicalGPU;
extern NvAPI_D3D_IsGSyncCapable_pfn             _NvAPI_D3D_IsGSyncCapable;
extern NvAPI_D3D_IsGSyncActive_pfn              _NvAPI_D3D_IsGSyncActive;
extern NvAPI_D3D_GetObjectHandleForResource_pfn _NvAPI_D3D_GetObjectHandleForResource;
extern NvAPI_Disp_SetDitherControl_pfn          NvAPI_Disp_SetDitherControl;
extern NvAPI_Disp_GetDitherControl_pfn          NvAPI_Disp_GetDitherControl;

#ifdef __cplusplus
}
#endif

namespace sk {
namespace NVAPI {
  BOOL InitializeLibrary (const wchar_t* wszAppName);
  BOOL UnloadLibrary     (void);

  int  CountSLIGPUs      (void);
  int  CountPhysicalGPUs (void);
  DXGI_ADAPTER_DESC*
       EnumGPUs_DXGI     (void);
  DXGI_ADAPTER_DESC*
       EnumSLIGPUs       (void);

  NV_GET_CURRENT_SLI_STATE
       GetSLIState       (IUnknown* pDev);

  DXGI_ADAPTER_DESC*
       FindGPUByDXGIName (const wchar_t* wszName);

  void SetAppFriendlyName(const wchar_t* wszFriendlyName);

  BOOL SetFramerateLimit (uint32_t       limit);

  BOOL SetAntiAliasingOverride ( const wchar_t** pwszPropertyList );

  BOOL SetSLIOverride    (      DLL_ROLE role,
                          const wchar_t* wszModeName,
                          const wchar_t* wszGPUCount,
                          const wchar_t* wszCompatBits);

  std::wstring
       GetDriverVersion  (NvU32* pVer = NULL);

  std::wstring
       ErrorMessage      (_NvAPI_Status err,
                          const char*   args,
                          UINT          line_no,
                          const char*   function_name,
                          const char*   file_name);

  extern std::wstring app_name;
  extern std::wstring friendly_name;
  extern std::wstring launcher_name;

  // Guilty until proven innocent
  extern bool nv_hardware;

}
}

NVAPI_INTERFACE SK_NvAPI_Disp_GetVRRInfo             (__in NvU32 displayId, __inout NV_GET_VRR_INFO         *pVrrInfo);
NVAPI_INTERFACE SK_NvAPI_DISP_GetMonitorCapabilities (__in NvU32 displayId, __inout NV_MONITOR_CAPABILITIES *pMonitorCapabilities);
NVAPI_INTERFACE SK_NvAPI_DISP_GetAdaptiveSyncData    (__in NvU32 displayId, __inout NV_GET_ADAPTIVE_SYNC_DATA *pAdaptiveSyncData);
NVAPI_INTERFACE SK_NvAPI_DISP_SetAdaptiveSyncData    (__in NvU32 displayId, __in    NV_SET_ADAPTIVE_SYNC_DATA *pAdaptiveSyncData);
NVAPI_INTERFACE SK_NvAPI_D3D_IsGSyncCapable          (__in IUnknown          *pDeviceOrContext,
                                                      __in NVDX_ObjectHandle   primarySurface,
                                                     __out BOOL              *pIsGsyncCapable);
NVAPI_INTERFACE SK_NvAPI_D3D_IsGSyncActive           (__in IUnknown          *pDeviceOrContext,
                                                      __in NVDX_ObjectHandle   primarySurface,
                                                     __out BOOL              *pIsGsyncActive);


void           SK_NvAPI_PreInitHDR         (void);
bool           SK_NvAPI_InitializeHDR      (void);

INT            SK_NvAPI_GetAnselEnablement (DLL_ROLE role);
BOOL           SK_NvAPI_EnableAnsel        (DLL_ROLE role);
BOOL           SK_NvAPI_DisableAnsel       (DLL_ROLE role);

BOOL           SK_NvAPI_GetVRREnablement   (void);
BOOL           SK_NvAPI_SetVRREnablement   (BOOL bEnable);

BOOL           SK_NvAPI_GetFastSync        (void);
BOOL           SK_NvAPI_SetFastSync        (BOOL bEnable);

BOOL           SK_NvAPI_IsSmoothingMotion  (void);

BOOL           SK_NvAPI_EnableVulkanBridge (BOOL bEnable);

BOOL           SK_NvAPI_AllowGFEOverlay    (bool bAllow, wchar_t *wszAppName, wchar_t *wszExecutable);

void __stdcall SK_NvAPI_SetAppFriendlyName (const wchar_t* wszFriendlyName);
void __stdcall SK_NvAPI_SetAppName         (const wchar_t* wszAppName);

extern SK_LazyGlobal <NV_GET_CURRENT_SLI_STATE> SK_NV_sli_state;

DWORD SK_NvAPI_DRS_GetDWORD (NvU32 setting_id);
bool  SK_NvAPI_DRS_SetDWORD (NvU32 setting_id, DWORD dwValue);

extern void     SK_NV_AdaptiveSyncControl ();


#endif /* __SK__NVAPI_H__ */
