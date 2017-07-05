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

#include <nvapi/nvapi.h>
#include <Windows.h>

#include <string>

enum   DLL_ROLE;
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

extern NvAPI_GPU_GetRamType_pfn                 NvAPI_GPU_GetRamType;
extern NvAPI_GPU_GetFBWidthAndLocation_pfn      NvAPI_GPU_GetFBWidthAndLocation;
extern NvAPI_GPU_GetPCIEInfo_pfn                NvAPI_GPU_GetPCIEInfo;
extern NvAPI_GetPhysicalGPUFromGPUID_pfn        NvAPI_GetPhysicalGPUFromGPUID;
extern NvAPI_GetGPUIDFromPhysicalGPU_pfn        NvAPI_GetGPUIDFromPhysicalGPU;
extern NvAPI_D3D_IsGSyncCapable_pfn             _NvAPI_D3D_IsGSyncCapable;
extern NvAPI_D3D_IsGSyncActive_pfn              _NvAPI_D3D_IsGSyncActive;
extern NvAPI_D3D_GetObjectHandleForResource_pfn _NvAPI_D3D_GetObjectHandleForResource;

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

#endif /* __SK__NVAPI_H__ */
