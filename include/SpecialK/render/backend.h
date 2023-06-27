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

#ifndef __SK__RENDER_BACKEND_H__
#define __SK__RENDER_BACKEND_H__

#include <Unknwnbase.h>


#include <SpecialK/com_util.h>
#include <SpecialK/config.h>
#include <SpecialK/core.h>

#include <Windows.h>

// For NVDX_ObjectHandle to emerge as a definition, include this
#include <d3d9.h>

#include <../depends/include/nvapi/nvapi_lite_common.h>
#include <../depends/include/nvapi/nvapi.h>

#include <comdef.h>
#include <atlbase.h>
#include <cstdint>

#include <dxmini.h>

typedef UINT32          D3DKMT_HANDLE;
typedef D3DKMT_HANDLE *PD3DKMT_HANDLE;

#include <SpecialK/render/d3d12/d3d12_interfaces.h>
#include <SpecialK/render/screenshot.h>
#include <SpecialK/utility.h>

#pragma pack (push,1)
typedef UINT D3DDDI_VIDEO_PRESENT_SOURCE_ID;

typedef UINT32          D3DKMT_HANDLE;
typedef D3DKMT_HANDLE *PD3DKMT_HANDLE;

typedef struct _D3DKMT_OPENADAPTERFROMHDC
{
  HDC                            hDc;            // in:  DC that maps to a single display
  D3DKMT_HANDLE                  hAdapter;       // out: adapter handle
  LUID                           AdapterLuid;    // out: adapter LUID
  D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;  // out: VidPN source ID for that particular display
} D3DKMT_OPENADAPTERFROMHDC;

typedef _Check_return_ NTSTATUS(APIENTRY *PFND3DKMT_OPENADAPTERFROMHDC)(_Inout_ D3DKMT_OPENADAPTERFROMHDC*);

// Represents performance data collected per engine from an adapter on an interval basis.
typedef struct _D3DKMT_NODE_PERFDATA
{
  _In_  UINT32    NodeOrdinal;          // Node ordinal of the requested engine.
  _In_  UINT32    PhysicalAdapterIndex; // The physical adapter index in a LDA chain.
  _Out_ ULONGLONG Frequency;            // Clock frequency of the requested engine, represented in hertz.
  _Out_ ULONGLONG MaxFrequency;         // The max frequency the engine can normally reach in hertz while not overclocked.
  _Out_ ULONGLONG MaxFrequencyOC;       // The max frequency the engine can reach with it�s current overclock in hertz.
  _Out_ ULONG     Voltage;              // Voltage of the engine in milli volts mV
  _Out_ ULONG     VoltageMax;           // The max voltage of the engine in milli volts while not overclocked.
  _Out_ ULONG     VoltageMaxOC;         // The max voltage of the engine while overclocked in milli volts.
  _Out_ ULONGLONG MaxTransitionLatency; // Max transition latency to change the frequency in 100 nanoseconds // REDSTONE5
} D3DKMT_NODE_PERFDATA;

// Represents performance data collected per adapter on an interval basis.
typedef struct _D3DKMT_ADAPTER_PERFDATA
{
  _In_  UINT32    PhysicalAdapterIndex; // The physical adapter index in a LDA chain.
  _Out_ ULONGLONG MemoryFrequency;      // Clock frequency of the memory in hertz
  _Out_ ULONGLONG MaxMemoryFrequency;   // Max clock frequency of the memory while not overclocked, represented in hertz.
  _Out_ ULONGLONG MaxMemoryFrequencyOC; // Clock frequency of the memory while overclocked in hertz.
  _Out_ ULONGLONG MemoryBandwidth;      // Amount of memory transferred in bytes
  _Out_ ULONGLONG PCIEBandwidth;        // Amount of memory transferred over PCI-E in bytes
  _Out_ ULONG     FanRPM;               // Fan rpm
  _Out_ ULONG     Power;                // Power draw of the adapter in tenths of a percentage
  _Out_ ULONG     Temperature;          // Temperature in deci-Celsius 1 = 0.1C
  _Out_ UCHAR     PowerStateOverride;   // Overrides dxgkrnls power view of linked adapters.
} D3DKMT_ADAPTER_PERFDATA;

// Represents data capabilities that are static and queried once per GPU during initialization.
typedef struct _D3DKMT_ADAPTER_PERFDATACAPS
{
  _In_  UINT32    PhysicalAdapterIndex; // The physical adapter index in a LDA chain.
  _Out_ ULONGLONG MaxMemoryBandwidth;   // Max memory bandwidth in bytes for 1 second
  _Out_ ULONGLONG MaxPCIEBandwidth;     // Max pcie bandwidth in bytes for 1 second
  _Out_ ULONG     MaxFanRPM;            // Max fan rpm
  _Out_ ULONG     TemperatureMax;       // Max temperature before damage levels
  _Out_ ULONG     TemperatureWarning;   // The temperature level where throttling begins.
} D3DKMT_ADAPTER_PERFDATACAPS;

typedef struct _D3DKMT_CREATEDEVICEFLAGS
{
  UINT    LegacyMode               :  1;   // 0x00000001
  UINT    RequestVSync             :  1;   // 0x00000002
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
  UINT    DisableGpuTimeout        :  1;   // 0x00000004
  UINT    Reserved                 : 29;   // 0xFFFFFFF8
#else
  UINT    Reserved                 : 30;   // 0xFFFFFFFC
#endif
} D3DKMT_CREATEDEVICEFLAGS;

typedef VOID D3DDDI_ALLOCATIONLIST;
typedef VOID D3DDDI_PATCHLOCATIONLIST;

typedef struct _D3DKMT_CREATEDEVICE
{
  union
  {
    D3DKMT_HANDLE           hAdapter;           // in: identifies the adapter for user-mode creation
    VOID*                   pAdapter;           // in: identifies the adapter for kernel-mode creation
  };

  D3DKMT_CREATEDEVICEFLAGS  Flags;

  D3DKMT_HANDLE             hDevice;                // out: Identifies the device
  VOID*                     pCommandBuffer;         // out: D3D10 compatibility.
  UINT                      CommandBufferSize;      // out: D3D10 compatibility.
  D3DDDI_ALLOCATIONLIST*    pAllocationList;        // out: D3D10 compatibility.
  UINT                      AllocationListSize;     // out: D3D10 compatibility.
  D3DDDI_PATCHLOCATIONLIST* pPatchLocationList;     // out: D3D10 compatibility.
  UINT                      PatchLocationListSize;  // out: D3D10 compatibility.
} D3DKMT_CREATEDEVICE;

typedef struct _D3DKMT_DESTROYDEVICE
{
  D3DKMT_HANDLE             hDevice;                // in: Indentifies the device
}D3DKMT_DESTROYDEVICE;

typedef UINT32 D3DKMT_HANDLE;

typedef enum _D3DKMDT_GRAPHICS_PREEMPTION_GRANULARITY
{
  D3DKMDT_GRAPHICS_PREEMPTION_NONE                = 0,
  D3DKMDT_GRAPHICS_PREEMPTION_DMA_BUFFER_BOUNDARY = 100,
  D3DKMDT_GRAPHICS_PREEMPTION_PRIMITIVE_BOUNDARY  = 200,
  D3DKMDT_GRAPHICS_PREEMPTION_TRIANGLE_BOUNDARY   = 300,
  D3DKMDT_GRAPHICS_PREEMPTION_PIXEL_BOUNDARY      = 400,
  D3DKMDT_GRAPHICS_PREEMPTION_SHADER_BOUNDARY     = 500,
} D3DKMDT_GRAPHICS_PREEMPTION_GRANULARITY;

typedef enum _D3DKMDT_COMPUTE_PREEMPTION_GRANULARITY
{
  D3DKMDT_COMPUTE_PREEMPTION_NONE                  = 0,
  D3DKMDT_COMPUTE_PREEMPTION_DMA_BUFFER_BOUNDARY   = 100,
  D3DKMDT_COMPUTE_PREEMPTION_DISPATCH_BOUNDARY     = 200,
  D3DKMDT_COMPUTE_PREEMPTION_THREAD_GROUP_BOUNDARY = 300,
  D3DKMDT_COMPUTE_PREEMPTION_THREAD_BOUNDARY       = 400,
  D3DKMDT_COMPUTE_PREEMPTION_SHADER_BOUNDARY       = 500,
} D3DKMDT_COMPUTE_PREEMPTION_GRANULARITY;

typedef struct _D3DKMDT_PREEMPTION_CAPS
{
  D3DKMDT_GRAPHICS_PREEMPTION_GRANULARITY GraphicsPreemptionGranularity;
  D3DKMDT_COMPUTE_PREEMPTION_GRANULARITY  ComputePreemptionGranularity;
} D3DKMDT_PREEMPTION_CAPS;

typedef struct _D3DKMT_WDDM_1_2_CAPS
{
  D3DKMDT_PREEMPTION_CAPS PreemptionCaps;
  union {
    struct {
      UINT SupportNonVGA                       :  1;
      UINT SupportSmoothRotation               :  1;
      UINT SupportPerEngineTDR                 :  1;
      UINT SupportKernelModeCommandBuffer      :  1;
      UINT SupportCCD                          :  1;
      UINT SupportSoftwareDeviceBitmaps        :  1;
      UINT SupportGammaRamp                    :  1;
      UINT SupportHWCursor                     :  1;
      UINT SupportHWVSync                      :  1;
      UINT SupportSurpriseRemovalInHibernation :  1;
      UINT Reserved                            : 22;
    };
    UINT Value;
  };
} D3DKMT_WDDM_1_2_CAPS;

typedef struct _D3DKMT_WDDM_1_3_CAPS
{
  union {
    struct {
      UINT SupportMiracast               :  1;
      UINT IsHybridIntegratedGPU         :  1;
      UINT IsHybridDiscreteGPU           :  1;
      UINT SupportPowerManagementPStates :  1;
      UINT SupportVirtualModes           :  1;
      UINT SupportCrossAdapterResource   :  1;
      UINT Reserved                      : 26;
    };
    UINT Value;
  };
} D3DKMT_WDDM_1_3_CAPS;

#ifndef DXGKDDI_INTERFACE_VERSION_WDDM2_0
typedef struct _D3DKMT_WDDM_2_0_CAPS
{
  union {
    struct {
      UINT Support64BitAtomics       : 1;
      UINT GpuMmuSupported           : 1;
      UINT IoMmuSupported            : 1;

#ifndef DXGKDDI_INTERFACE_VERSION_WDDM2_4
      // Since WDDM2_4
      //
      UINT FlipOverwriteSupported    : 1;
      UINT SupportContextlessPresent : 1;

#ifndef DXGKDDI_INTERFACE_VERSION_WDDM2_5
      // Since WDDM2_5
      //
      UINT SupportSurpriseRemoval    :  1;
      UINT Reserved                  : 26;
#else
      UINT Reserved                  : 27;
#endif // !defined (DXGKDDI_INTERFACE_VERSION_WDDM2_5)
#else
      UINT Reserved                  : 29;
#endif // !defined (DXGKDDI_INTERFACE_VERSION_WDDM2_4)
    };
    UINT Value;
  };
} D3DKMT_WDDM_2_0_CAPS;
#endif // !defined (DXGKDDI_INTERFACE_VERSION_WDDM2_0)

#ifndef DXGKDDI_INTERFACE_VERSION_WDDM2_7
typedef struct _D3DKMT_WDDM_2_7_CAPS
{
  union {
    struct {
      UINT HwSchSupported               :  1;    // Specifies whether the GPU supports hardware scheduling
      UINT HwSchEnabled                 :  1;    // Specifies whether the hardware scheduling is currently enabled for this GPU
      UINT HwSchEnabledByDefault        :  1;    // Set to 1 if the OS default policy is to enable hardware scheduling for this GPU
      UINT IndependentVidPnVSyncControl :  1;
      UINT Reserved                     : 28;
    };
    UINT Value;
  };
} D3DKMT_WDDM_2_7_CAPS;
#endif

// Accomodate older build environments that do not define 2_9 and 3_0
//
//   -> When redistributables mature, remove this :)
//
#ifndef DXGKDDI_INTERFACE_VERSION_WDDM2_9
typedef struct _D3DKMT_WDDM_2_9_CAPS {
  union {
    struct {
      UINT HwSchSupportState          :  2;
      UINT HwSchEnabled               :  1;
      UINT SelfRefreshMemorySupported :  1;
      UINT Reserved                   : 28;
    };
    UINT Value;
  };
} D3DKMT_WDDM_2_9_CAPS;

typedef struct _D3DKMT_WDDM_3_0_CAPS {
  union {
    struct {
      UINT HwFlipQueueSupportState :  2;
      UINT HwFlipQueueEnabled      :  1;
      UINT DisplayableSupported    :  1;
      UINT Reserved                : 28;
    };
    UINT Value;
  };
} D3DKMT_WDDM_3_0_CAPS;
#endif

struct SK_WDDM_CAPS {
  D3DKMT_WDDM_1_2_CAPS _1_2 = { };
  D3DKMT_WDDM_1_3_CAPS _1_3 = { };
  D3DKMT_WDDM_2_0_CAPS _2_0 = { };
  D3DKMT_WDDM_2_7_CAPS _2_7 = { };
  D3DKMT_WDDM_2_9_CAPS _2_9 = { };
  D3DKMT_WDDM_3_0_CAPS _3_0 = { };

  void init (void);
};

typedef struct _D3DKMT_MULTIPLANE_OVERLAY_CAPS
{
  union
  {
    struct
    {
      UINT Rotation                        : 1;    // Full rotation
      UINT RotationWithoutIndependentFlip  : 1;    // Rotation, but without simultaneous IndependentFlip support
      UINT VerticalFlip                    : 1;    // Can flip the data vertically
      UINT HorizontalFlip                  : 1;    // Can flip the data horizontally
      UINT StretchRGB                      : 1;    // Supports stretching RGB formats
      UINT StretchYUV                      : 1;    // Supports stretching YUV formats
      UINT BilinearFilter                  : 1;    // Blinear filtering
      UINT HighFilter                      : 1;    // Better than bilinear filtering
      UINT Shared                          : 1;    // MPO resources are shared across VidPnSources
      UINT Immediate                       : 1;    // Immediate flip support
      UINT Plane0ForVirtualModeOnly        : 1;    // Stretching plane 0 will also stretch the HW cursor and should only be used for virtual mode support
      UINT Version3DDISupport              : 1;    // Driver supports the 2.2 MPO DDIs
      UINT Reserved                        : 20;
    };
    UINT Value;
  };
} D3DKMT_MULTIPLANE_OVERLAY_CAPS;

typedef struct _D3DKMT_GET_MULTIPLANE_OVERLAY_CAPS
{
  D3DKMT_HANDLE                          hAdapter;             // in: adapter handle
  D3DDDI_VIDEO_PRESENT_SOURCE_ID         VidPnSourceId;        // in
  UINT                                   MaxPlanes;            // out: Total number of planes currently supported
  UINT                                   MaxRGBPlanes;         // out: Number of RGB planes currently supported
  UINT                                   MaxYUVPlanes;         // out: Number of YUV planes currently supported
  D3DKMT_MULTIPLANE_OVERLAY_CAPS         OverlayCaps;          // out: Overlay capabilities
  float                                  MaxStretchFactor;     // out
  float                                  MaxShrinkFactor;      // out
} D3DKMT_GET_MULTIPLANE_OVERLAY_CAPS;


typedef enum _KMTQUERYADAPTERINFOTYPE
{
  KMTQAITYPE_UMDRIVERPRIVATE                          =  0,
  KMTQAITYPE_UMDRIVERNAME                             =  1, // D3DKMT_UMDFILENAMEINFO
  KMTQAITYPE_UMOPENGLINFO                             =  2, // D3DKMT_OPENGLINFO
  KMTQAITYPE_GETSEGMENTSIZE                           =  3, // D3DKMT_SEGMENTSIZEINFO
  KMTQAITYPE_ADAPTERGUID                              =  4, // GUID
  KMTQAITYPE_FLIPQUEUEINFO                            =  5, // D3DKMT_FLIPQUEUEINFO
  KMTQAITYPE_ADAPTERADDRESS                           =  6, // D3DKMT_ADAPTERADDRESS
  KMTQAITYPE_SETWORKINGSETINFO                        =  7, // D3DKMT_WORKINGSETINFO
  KMTQAITYPE_ADAPTERREGISTRYINFO                      =  8, // D3DKMT_ADAPTERREGISTRYINFO
  KMTQAITYPE_CURRENTDISPLAYMODE                       =  9, // D3DKMT_CURRENTDISPLAYMODE
  KMTQAITYPE_MODELIST                                 = 10, // D3DKMT_DISPLAYMODE (array)
  KMTQAITYPE_CHECKDRIVERUPDATESTATUS                  = 11,
  KMTQAITYPE_VIRTUALADDRESSINFO                       = 12, // D3DKMT_VIRTUALADDRESSINFO
  KMTQAITYPE_DRIVERVERSION                            = 13, // D3DKMT_DRIVERVERSION
  KMTQAITYPE_ADAPTERTYPE                              = 15, // D3DKMT_ADAPTERTYPE // since WIN8
  KMTQAITYPE_OUTPUTDUPLCONTEXTSCOUNT                  = 16, // D3DKMT_OUTPUTDUPLCONTEXTSCOUNT
  KMTQAITYPE_WDDM_1_2_CAPS                            = 17, // D3DKMT_WDDM_1_2_CAPS
  KMTQAITYPE_UMD_DRIVER_VERSION                       = 18, // D3DKMT_UMD_DRIVER_VERSION
  KMTQAITYPE_DIRECTFLIP_SUPPORT                       = 19, // D3DKMT_DIRECTFLIP_SUPPORT
  KMTQAITYPE_MULTIPLANEOVERLAY_SUPPORT                = 20, // D3DKMT_MULTIPLANEOVERLAY_SUPPORT // since WDDM1_3
  KMTQAITYPE_DLIST_DRIVER_NAME                        = 21, // D3DKMT_DLIST_DRIVER_NAME
  KMTQAITYPE_WDDM_1_3_CAPS                            = 22, // D3DKMT_WDDM_1_3_CAPS
  KMTQAITYPE_MULTIPLANEOVERLAY_HUD_SUPPORT            = 23, // D3DKMT_MULTIPLANEOVERLAY_HUD_SUPPORT
  KMTQAITYPE_WDDM_2_0_CAPS                            = 24, // D3DKMT_WDDM_2_0_CAPS // since WDDM2_0
  KMTQAITYPE_NODEMETADATA                             = 25, // D3DKMT_NODEMETADATA
  KMTQAITYPE_CPDRIVERNAME                             = 26, // D3DKMT_CPDRIVERNAME
  KMTQAITYPE_XBOX                                     = 27, // D3DKMT_XBOX
  KMTQAITYPE_INDEPENDENTFLIP_SUPPORT                  = 28, // D3DKMT_INDEPENDENTFLIP_SUPPORT
  KMTQAITYPE_MIRACASTCOMPANIONDRIVERNAME              = 29, // D3DKMT_MIRACASTCOMPANIONDRIVERNAME
  KMTQAITYPE_PHYSICALADAPTERCOUNT                     = 30, // D3DKMT_PHYSICAL_ADAPTER_COUNT
  KMTQAITYPE_PHYSICALADAPTERDEVICEIDS                 = 31, // D3DKMT_QUERY_DEVICE_IDS
  KMTQAITYPE_DRIVERCAPS_EXT                           = 32, // D3DKMT_DRIVERCAPS_EXT
  KMTQAITYPE_QUERY_MIRACAST_DRIVER_TYPE               = 33, // D3DKMT_QUERY_MIRACAST_DRIVER_TYPE
  KMTQAITYPE_QUERY_GPUMMU_CAPS                        = 34, // D3DKMT_QUERY_GPUMMU_CAPS
  KMTQAITYPE_QUERY_MULTIPLANEOVERLAY_DECODE_SUPPORT   = 35, // D3DKMT_MULTIPLANEOVERLAY_DECODE_SUPPORT
  KMTQAITYPE_QUERY_HW_PROTECTION_TEARDOWN_COUNT       = 36, // UINT32
  KMTQAITYPE_QUERY_ISBADDRIVERFORHWPROTECTIONDISABLED = 37, // D3DKMT_ISBADDRIVERFORHWPROTECTIONDISABLED
  KMTQAITYPE_MULTIPLANEOVERLAY_SECONDARY_SUPPORT      = 38, // D3DKMT_MULTIPLANEOVERLAY_SECONDARY_SUPPORT
  KMTQAITYPE_INDEPENDENTFLIP_SECONDARY_SUPPORT        = 39, // D3DKMT_INDEPENDENTFLIP_SECONDARY_SUPPORT
  KMTQAITYPE_PANELFITTER_SUPPORT                      = 40, // D3DKMT_PANELFITTER_SUPPORT // since WDDM2_1
  KMTQAITYPE_PHYSICALADAPTERPNPKEY                    = 41, // D3DKMT_QUERY_PHYSICAL_ADAPTER_PNP_KEY // since WDDM2_2
  KMTQAITYPE_GETSEGMENTGROUPSIZE                      = 42, // D3DKMT_SEGMENTGROUPSIZEINFO
  KMTQAITYPE_MPO3DDI_SUPPORT                          = 43, // D3DKMT_MPO3DDI_SUPPORT
  KMTQAITYPE_HWDRM_SUPPORT                            = 44, // D3DKMT_HWDRM_SUPPORT
  KMTQAITYPE_MPOKERNELCAPS_SUPPORT                    = 45, // D3DKMT_MPOKERNELCAPS_SUPPORT
  KMTQAITYPE_MULTIPLANEOVERLAY_STRETCH_SUPPORT        = 46, // D3DKMT_MULTIPLANEOVERLAY_STRETCH_SUPPORT
  KMTQAITYPE_GET_DEVICE_VIDPN_OWNERSHIP_INFO          = 47, // D3DKMT_GET_DEVICE_VIDPN_OWNERSHIP_INFO
  KMTQAITYPE_QUERYREGISTRY                            = 48, // D3DDDI_QUERYREGISTRY_INFO // since WDDM2_4
  KMTQAITYPE_KMD_DRIVER_VERSION                       = 49, // D3DKMT_KMD_DRIVER_VERSION
  KMTQAITYPE_BLOCKLIST_KERNEL                         = 50, // D3DKMT_BLOCKLIST_INFO ??
  KMTQAITYPE_BLOCKLIST_RUNTIME                        = 51, // D3DKMT_BLOCKLIST_INFO ??
  KMTQAITYPE_ADAPTERGUID_RENDER                       = 52, // GUID
  KMTQAITYPE_ADAPTERADDRESS_RENDER                    = 53, // D3DKMT_ADAPTERADDRESS
  KMTQAITYPE_ADAPTERREGISTRYINFO_RENDER               = 54, // D3DKMT_ADAPTERREGISTRYINFO
  KMTQAITYPE_CHECKDRIVERUPDATESTATUS_RENDER           = 55,
  KMTQAITYPE_DRIVERVERSION_RENDER                     = 56, // D3DKMT_DRIVERVERSION
  KMTQAITYPE_ADAPTERTYPE_RENDER                       = 57, // D3DKMT_ADAPTERTYPE
  KMTQAITYPE_WDDM_1_2_CAPS_RENDER                     = 58, // D3DKMT_WDDM_1_2_CAPS
  KMTQAITYPE_WDDM_1_3_CAPS_RENDER                     = 59, // D3DKMT_WDDM_1_3_CAPS
  KMTQAITYPE_QUERY_ADAPTER_UNIQUE_GUID                = 60, // D3DKMT_QUERY_ADAPTER_UNIQUE_GUID
  KMTQAITYPE_NODEPERFDATA                             = 61, // D3DKMT_NODE_PERFDATA
  KMTQAITYPE_ADAPTERPERFDATA                          = 62, // D3DKMT_ADAPTER_PERFDATA
  KMTQAITYPE_ADAPTERPERFDATA_CAPS                     = 63, // D3DKMT_ADAPTER_PERFDATACAPS
  KMTQUITYPE_GPUVERSION                               = 64, // D3DKMT_GPUVERSION
#ifndef DXGKDDI_INTERFACE_VERSION_WDDM2_6
  // WDDM2_6
  KMTQAITYPE_DRIVER_DESCRIPTION                       = 65, // D3DKMT_DRIVER_DESCRIPTION
  KMTQAITYPE_DRIVER_DESCRIPTION_RENDER                = 66, // D3DKMT_DRIVER_DESCRIPTION
  KMTQAITYPE_SCANOUT_CAPS                             = 67, // D3DKMT_QUERY_SCANOUT_CAPS
  KMTQAITYPE_PARAVIRTUALIZATION_RENDER                = 68, // D3DKMT_PARAVIRTUALIZATION
#ifndef DXGKDDI_INTERFACE_VERSION_WDDM2_7
  // WDDM2_7
  KMTQAITYPE_SERVICENAME                              = 69,
  KMTQAITYPE_WDDM_2_7_CAPS                            = 70, // D3DKMT_WDDM_2_7_CAPS
  KMTQAITYPE_DISPLAY_UMDRIVERNAME                     = 71, // Added in 19H2
  KMTQAITYPE_TRACKEDWORKLOAD_SUPPORT                  = 72,
#ifndef DXGKDDI_INTERFACE_VERSION_WDDM2_8
  // WDDM2_8
  KMTQAITYPE_HYBRID_DLIST_DLL_SUPPORT                 = 73,
  KMTQAITYPE_DISPLAY_CAPS                             = 74,
#ifndef DXGKDDI_INTERFACE_VERSION_WDDM2_9
  // WDDM2_9
  KMTQAITYPE_WDDM_2_9_CAPS                            = 75, // D3DKMT_WDDM_2_9_CAPS
  KMTQAITYPE_CROSSADAPTERRESOURCE_SUPPORT             = 76,
#ifndef DXGKDDI_INTERFACE_VERSION_WDDM3_0
  // WDDM3_0
  KMTQAITYPE_WDDM_3_0_CAPS                            = 77, // D3DKMT_WDDM_3_0_CAPS
#endif // DXGKDDI_INTERFACE_VERSION_WDDM3_0
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_9
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_8
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_7
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_6
  D3DKMT_FORCE_DWORD                                  = 0x7fffffff
} KMTQUERYADAPTERINFOTYPE;

typedef struct _D3DKMT_QUERYADAPTERINFO
{
  _In_                                     D3DKMT_HANDLE           AdapterHandle;
  _In_                                     KMTQUERYADAPTERINFOTYPE Type;
  _Inout_bytecount_(PrivateDriverDataSize) PVOID                   PrivateDriverData;
  _Out_      UINT32 PrivateDriverDataSize;
} D3DKMT_QUERYADAPTERINFO;

typedef struct _D3DKMT_OPENADAPTERFROMLUID {
  LUID          AdapterLuid;
  D3DKMT_HANDLE hAdapter;
} D3DKMT_OPENADAPTERFROMLUID;

typedef struct _D3DKMT_CLOSEADAPTER {
  D3DKMT_HANDLE hAdapter;
} D3DKMT_CLOSEADAPTER;

#define D3DKMT_MAX_PRESENT_HISTORY_RECTS       16
#define D3DKMT_MAX_PRESENT_HISTORY_SCATTERBLTS 12

typedef enum _D3DKMT_PRESENT_MODEL {
  D3DKMT_PM_UNINITIALIZED           = 0,
  D3DKMT_PM_REDIRECTED_GDI          = 1,
  D3DKMT_PM_REDIRECTED_FLIP         = 2,
  D3DKMT_PM_REDIRECTED_BLT          = 3,
  D3DKMT_PM_REDIRECTED_VISTABLT     = 4,
  D3DKMT_PM_SCREENCAPTUREFENCE      = 5,
  D3DKMT_PM_REDIRECTED_GDI_SYSMEM   = 6,
  D3DKMT_PM_REDIRECTED_COMPOSITION  = 7
} D3DKMT_PRESENT_MODEL;

typedef enum D3DDDI_FLIPINTERVAL_TYPE {
  D3DDDI_FLIPINTERVAL_IMMEDIATE,
  D3DDDI_FLIPINTERVAL_ONE,
  D3DDDI_FLIPINTERVAL_TWO,
  D3DDDI_FLIPINTERVAL_THREE,
  D3DDDI_FLIPINTERVAL_FOUR,
  D3DDDI_FLIPINTERVAL_IMMEDIATE_ALLOW_TEARING
} ;

typedef struct _D3DKMT_FENCE_PRESENTHISTORYTOKEN {
  UINT64 Key;
} D3DKMT_FENCE_PRESENTHISTORYTOKEN;

typedef struct _D3DKMT_GDIMODEL_SYSMEM_PRESENTHISTORYTOKEN {
  ULONG64 hlsurf;
  DWORD   dwDirtyFlags;
  UINT64  uiCookie;
} D3DKMT_GDIMODEL_SYSMEM_PRESENTHISTORYTOKEN;

typedef struct _D3DKMT_DIRTYREGIONS {
  UINT NumRects;
  RECT Rects [D3DKMT_MAX_PRESENT_HISTORY_RECTS];
} D3DKMT_DIRTYREGIONS;

typedef struct _D3DKMT_GDIMODEL_PRESENTHISTORYTOKEN {
  ULONG64             hLogicalSurface;
  ULONG64             hPhysicalSurface;
  RECT                ScrollRect;
  POINT               ScrollOffset;
  D3DKMT_DIRTYREGIONS DirtyRegions;
} D3DKMT_GDIMODEL_PRESENTHISTORYTOKEN;

typedef struct _D3DKMT_FLIPMODEL_PRESENTHISTORYTOKENFLAGS {
  union {
    struct {
      UINT Video                         :  1;
      UINT RestrictedContent             :  1;
      UINT ClipToView                    :  1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
      UINT StereoPreferRight             :  1;
      UINT TemporaryMono                 :  1;
      UINT FlipRestart                   :  1;
      UINT ScatterBlt                    :  1;
      UINT AlphaMode                     :  2;
      UINT SignalLimitOnTokenCompletion  :  1;
      UINT Reserved                      : 22;
#else
      UINT Reserved  :29;
#endif
    };
    UINT   Value;
  };
} D3DKMT_FLIPMODEL_PRESENTHISTORYTOKENFLAGS;

typedef struct _D3DKMT_BLTMODEL_PRESENTHISTORYTOKEN {
  ULONG64             hLogicalSurface;
  ULONG64             hPhysicalSurface;
  ULONG64             EventId;
  D3DKMT_DIRTYREGIONS DirtyRegions;
} D3DKMT_BLTMODEL_PRESENTHISTORYTOKEN;

typedef enum _D3DDDI_ROTATION {
  D3DDDI_ROTATION_IDENTITY,
  D3DDDI_ROTATION_90,
  D3DDDI_ROTATION_180,
  D3DDDI_ROTATION_270
} D3DDDI_ROTATION;

typedef struct _D3DKMT_SCATTERBLT
{
  ULONG64 hLogicalSurfaceDestination;
  LONG64  hDestinationCompSurfDWM;
  UINT64  DestinationCompositionBindingId;
  RECT    SourceRect;
  POINT   DestinationOffset;
} D3DKMT_SCATTERBLT;

typedef struct _D3DKMT_SCATTERBLTS {
  UINT              NumBlts;
  D3DKMT_SCATTERBLT Blts [D3DKMT_MAX_PRESENT_HISTORY_SCATTERBLTS];
} D3DKMT_SCATTERBLTS;

typedef struct _D3DKMT_FLIPMODEL_PRESENTHISTORYTOKEN {
  UINT64                                    FenceValue;
  ULONG64                                   hLogicalSurface;
  UINT                                      SwapChainIndex;
  UINT64                                    PresentLimitSemaphoreId;
  D3DDDI_FLIPINTERVAL_TYPE                  FlipInterval;
  D3DKMT_FLIPMODEL_PRESENTHISTORYTOKENFLAGS Flags;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
  LONG64                                    hCompSurf;
  UINT64                                    CompositionSyncKey;
  UINT                                      RemainingTokens;
  RECT                                      ScrollRect;
  POINT                                     ScrollOffset;
  UINT                                      PresentCount;
  FLOAT                                     RevealColor[4];
  D3DDDI_ROTATION                           Rotation;
  D3DKMT_SCATTERBLTS                        ScatterBlts;
  D3DKMT_HANDLE                             hSyncObject;
#endif
  D3DKMT_DIRTYREGIONS                       DirtyRegions;
} D3DKMT_FLIPMODEL_PRESENTHISTORYTOKEN;

typedef struct _D3DKMT_COMPOSITION_PRESENTHISTORYTOKEN {
  ULONG64 hPrivateData;
} D3DKMT_COMPOSITION_PRESENTHISTORYTOKEN;

typedef ULONGLONG  D3DKMT_VISTABLTMODEL_PRESENTHISTORYTOKEN;

typedef struct _D3DKMT_PRESENTHISTORYTOKEN {
  D3DKMT_PRESENT_MODEL Model;
  UINT                 TokenSize;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
  UINT64               CompositionBindingId;
#endif
  union {
    D3DKMT_FLIPMODEL_PRESENTHISTORYTOKEN       Flip;
    D3DKMT_BLTMODEL_PRESENTHISTORYTOKEN        Blt;
    D3DKMT_VISTABLTMODEL_PRESENTHISTORYTOKEN   VistaBlt;
    D3DKMT_GDIMODEL_PRESENTHISTORYTOKEN        Gdi;
    D3DKMT_FENCE_PRESENTHISTORYTOKEN           Fence;
    D3DKMT_GDIMODEL_SYSMEM_PRESENTHISTORYTOKEN GdiSysMem;
    D3DKMT_COMPOSITION_PRESENTHISTORYTOKEN     Composition;
  } Token;
} D3DKMT_PRESENTHISTORYTOKEN;

typedef struct _D3DKMT_GETPRESENTHISTORY {
  D3DKMT_HANDLE              hAdapter;
  UINT                       ProvidedSize;
  UINT                       WrittenSize;
  D3DKMT_PRESENTHISTORYTOKEN *pTokens;
  UINT                       NumTokens;
} D3DKMT_GETPRESENTHISTORY;

typedef struct _D3DKMT_DEVICEPRESENT_QUEUE_STATE {
  D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
  BOOLEAN                        bQueuedPresentLimitReached;
} D3DKMT_DEVICEPRESENT_QUEUE_STATE;
#pragma pack (pop)

typedef NTSTATUS (NTAPI *PFND3DKMT_QUERYADAPTERINFO)(    D3DKMT_QUERYADAPTERINFO    *pData);
typedef NTSTATUS (NTAPI *PFND3DKMT_OPENADAPTERFROMLUID)( D3DKMT_OPENADAPTERFROMLUID *unnamedParam1);
typedef NTSTATUS (NTAPI *PFND3DKMT_CLOSEADAPTER)(        D3DKMT_CLOSEADAPTER        *unnamedParam1);
typedef NTSTATUS (NTAPI *PFND3DKMT_CREATEDEVICE)(        D3DKMT_CREATEDEVICE        *unnamedParam1);
typedef NTSTATUS (NTAPI *PFND3DKMT_DESTROYDEVICE)(const  D3DKMT_DESTROYDEVICE       *unnamedParam1);

class SK_RenderBackend_V1
{
public:
  enum SK_RenderAPI api       = SK_RenderAPI::Reserved;
       wchar_t      name [16] = { };
};

enum
{
  SK_FRAMEBUFFER_FLAG_SRGB    = 0x01U,
  SK_FRAMEBUFFER_FLAG_FLOAT   = 0x02U,
  SK_FRAMEBUFFER_FLAG_RGB10A2 = 0x04U,
  SK_FRAMEBUFFER_FLAG_MSAA    = 0x08U,
  SK_FRAMEBUFFER_FLAG_HDR     = 0x10U,
};

enum reset_stage_e
{
  Initiate = 0x0u, // Fake device loss
  Respond  = 0x1u, // Fake device not reset
  Clear    = 0x2u  // Return status to normal
};

enum mode_change_request_e
{
  Windowed   =    0U, // Switch from Fullscreen to Windowed
  Fullscreen =    1U, // Switch from Windowed to Fullscreen
  None       = 0xFFU
};

extern reset_stage_e         trigger_reset;
extern mode_change_request_e request_mode_change;

struct sk_hwnd_cache_s
{      sk_hwnd_cache_s  (HWND window);
  HWND    hwnd         = nullptr;
  HWND    parent       = nullptr;
  struct
  { DWORD pid          = 0x0,
          tid          = 0x0;
  }       owner;
  bool    unicode      = false;
  ULONG64 last_changed = 0ULL;

  struct devcaps_s
  { struct
    { int    x         =   0;
      int    y         =   0;
      double refresh   = 0.0;
    }        res;

    DWORD last_checked = 0UL;
  } devcaps;

  wchar_t class_name [128] = { };
  wchar_t title      [128] = { };

  operator const HWND& (void) const noexcept {
    return hwnd;
  };

  devcaps_s& getDevCaps (void);
  bool       update     (HWND window);
};

constexpr
float
operator"" _Nits ( long double whitepoint_scalar ) noexcept
{
  return
    static_cast <float> ( whitepoint_scalar / 80.0F );
}

#pragma pack(push,8)
struct SK_ColorSpace {
  float xr = 0.0f, yr = 0.0f,
        xg = 0.0f, yg = 0.0f,
        xb = 0.0f, yb = 0.0f,
        Xw = 0.0f, Yw = 0.0f, Zw = 0.0f;

  float minY        = 0.0f,
        maxLocalY   = 0.0f,
        maxAverageY = 0.0f,
        maxY        = 0.0f;
};

const char*
HDRModeToStr (NV_HDR_MODE mode);


class SK_RenderBackend_V2 : public SK_RenderBackend_V1
{
public:
   SK_RenderBackend_V2 (void);
  ~SK_RenderBackend_V2 (void);

  wchar_t                 display_name [128]   = {     };
  SK_ComPtr <IUnknown>    device               = nullptr;
  SK_ComPtr <IUnknown>    swapchain            = nullptr;
  SK_ComPtr <IDXGIFactory1>
                          factory              = nullptr; // Used to enumerate display modes, the rb doesn't create any resources
  HMONITOR                monitor              = nullptr;
  HMONITOR                next_monitor         = nullptr; // monitor != next_monitor during relocation

  // Different views of the same resource (API interop)
  struct {
    SK_ComPtr <IDirect3DSurface9>
                          d3d9                 = nullptr;
    SK_ComPtr <IDXGISurface>
                          dxgi                 = nullptr;
    NVDX_ObjectHandle     nvapi                = nullptr;
  } surface;

  struct {
    D3DKMT_HANDLE         d3dkmt               =      0;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID
                          VidPnSourceId        =      0;
    LUID                  luid                 = { 0, 0 };

    HANDLE                hWait                = nullptr;

    struct {
      D3DKMT_ADAPTER_PERFDATA data             = { };
      LONG64                  sampled_frame    =  0;
    } perf;

    struct {
      D3DKMT_HANDLE       hDevice              =  0;
    } device;
    std::recursive_mutex  lock;
  } adapter;

  static auto constexpr
    _MAX_DISPLAYS = 16;

  bool                    fullscreen_exclusive = false;
  uint64_t                framebuffer_flags    = 0x00;
  int                     present_interval     = 0; // Present interval on last call to present
  float                   ui_luminance         = 325.0_Nits;
  bool                    ui_srgb              = true;
  bool                    srgb_stripped        = false; // sRGB may be stripped from swapchains for advanced features to work
  bool                    hdr_capable          = false;
  bool                    driver_based_hdr     = false;
  SK_ColorSpace           display_gamut;       // EDID
  SK_ColorSpace           working_gamut;       // Metadata range

  struct output_s {
    UINT                  idx                  =   0;
    RECT                  rect                 = { 0, 0,
                                                   0, 0 };
    int                   bpc                  =   8;
    SK_ColorSpace         gamut;
    DXGI_COLOR_SPACE_TYPE colorspace           = DXGI_COLOR_SPACE_CUSTOM;
    bool                  primary              = false;
    uint32_t              mpo_planes           =     0;
                                               
    struct {                                   
      bool                enabled              = false;
      bool                supported            = false;
      float               white_level          = 80.0f;
      DISPLAYCONFIG_COLOR_ENCODING             
                          encoding             = DISPLAYCONFIG_COLOR_ENCODING_RGB;
    } hdr;                                     
    bool                  attached             = false;
    wchar_t               name      [64]       =  { };
    wchar_t               gdi_name  [32]       =  { };
    wchar_t               path_name [128]      =  { };
    char                  full_name [128]      =  { };
    HMONITOR              monitor              = nullptr;
    DISPLAYCONFIG_PATH_INFO                    
                          vidpn                =  { };
    struct native_res_s {                      
      uint32_t            width                =   0;
      uint32_t            height               =   0;
      DXGI_RATIONAL       refresh              = { 0, 0 };
    } native;

    struct nvapi_ctx_s {
      NvPhysicalGpuHandle gpu_handle           =   nullptr;
      NvDisplayHandle     display_handle       =   nullptr;
      NvU32               display_id           =   0;
      NvU32               output_id            =   0;
    } nvapi;

    struct signal_info_s {
      char                type [32]            = { };
      UINT                connector_idx        =  0;

      struct timing_s {
        UINT64            pixel_clock          =   0ULL;
        DXGI_RATIONAL     hsync_freq           = { 0, 0 };
        DXGI_RATIONAL     vsync_freq           = { 0, 0 };
        SIZEL             active_size          = { 0, 0 };
        SIZEL             total_size           = { 0, 0 };

        union video_standard_s {
          struct additional_info_s {
            UINT32        videoStandard    :16 = 0;
            UINT32        vSyncFreqDivider : 6 = 0;
            UINT32        reserved         :10 = 0;
          } _AdditionalSignalInfo;

          UINT32          videoStandard        = 0;

          const char*     toStr (void);
        }                 videoStandard;

        struct custom_wait_s {
          HANDLE          hVBlankFront         = nullptr;
          HANDLE          hVBlankBack          = nullptr;
        } events;
      } timing;
    } signal;
  } displays [_MAX_DISPLAYS];

  int                     active_display       =  0;
  uint32_t                display_crc
                           [_MAX_DISPLAYS]     = { };  // Quick detect for changing displays
  bool                    stale_display_info   = true; // Output topology is stale, update it during getContainingOutput (...)

  struct scan_out_s {
    int                        bpc                  = 8;
    DXGI_COLOR_SPACE_TYPE      colorspace_override  = DXGI_COLOR_SPACE_CUSTOM;
    DXGI_COLOR_SPACE_TYPE      dxgi_colorspace      = DXGI_COLOR_SPACE_CUSTOM;
    DXGI_COLOR_SPACE_TYPE      dwm_colorspace       = DXGI_COLOR_SPACE_CUSTOM;

    struct nvapi_desc_s {
      bool                     active               = false;

      struct {
        NvU32                  supports_YUV422_12bit : 1;
        NvU32                  supports_10b_12b_444  : 2;
      } color_support_hdr                           = { };

      struct {
        NvU32                  display_id           = std::numeric_limits <NvU32>::max ();
        NV_HDR_CAPABILITIES_V2 hdr_caps             = { };
        NV_HDR_COLOR_DATA_V2   hdr_data             = { };

        // TODO
        //std::vector  <
        //  std::tuple < NV_BPC,
        //               NV_COLOR_FORMAT,
        //               NV_DYNAMIC_RANGE >
        //                                >   color_modes =
        //                     { std::make_tuple ( NV_BPC_DEFAULT,
        //                                         NV_COLOR_FORMAT_DEFAULT,
        //                                         NV_DYNAMIC_RANGE_AUTO ); }
      } raw;

      NV_HDR_MODE              mode                 = NV_HDR_MODE_OFF;
      NV_COLOR_FORMAT          color_format         = NV_COLOR_FORMAT_DEFAULT;
      NV_DYNAMIC_RANGE         dynamic_range        = NV_DYNAMIC_RANGE_AUTO;
      NV_BPC                   bpc                  = NV_BPC_DEFAULT;

      bool isHDR10 (void) const noexcept
                                { return ( mode == NV_HDR_MODE_UHDA ||
                                           mode == NV_HDR_MODE_UHDA_PASSTHROUGH ); }

      bool setColorEncoding_HDR ( NV_COLOR_FORMAT fmt,
                                  NV_BPC          bpc,
                                  NvU32           display_id = std::numeric_limits <NvU32>::max () );

      bool setColorEncoding_SDR ( NV_COLOR_FORMAT fmt,
                                  NV_BPC          bpc,
                                  NvU32           display_id = std::numeric_limits <NvU32>::max () );

      const char* getBpcStr (void) const
      {
        static std::unordered_map <NV_BPC, const char*>
          bpc_map =
            { { NV_BPC_DEFAULT, "Default?" },
              { NV_BPC_6,       "6 bpc"    },
              { NV_BPC_8,       "8 bpc"    },
              { NV_BPC_10,      "10 bpc"   },
              { NV_BPC_12,      "12 bpc"   },
              { NV_BPC_16,      "16 bpc"   } };

        return bpc_map [bpc];
      }

      const char* getFormatStr (void) const
      {
        static std::unordered_map <NV_COLOR_FORMAT, const char*>
          color_fmt_map =
              { { NV_COLOR_FORMAT_RGB,    "RGB 4:4:4"  },
                { NV_COLOR_FORMAT_YUV422, "YUV 4:2:2"  },
                { NV_COLOR_FORMAT_YUV444, "YUV 4:4:4", },
                { NV_COLOR_FORMAT_YUV420, "YUV 4:2:0", },
                { NV_COLOR_FORMAT_DEFAULT,"Default?",  },
                { NV_COLOR_FORMAT_AUTO,   "Auto",      } };

        return color_fmt_map [color_format];
      }

      const char* getRangeStr (void) const
      {
        static std::unordered_map <_NV_DYNAMIC_RANGE, const char*>
          dynamic_range_map =
            { { NV_DYNAMIC_RANGE_VESA, "Limited"    },
              { NV_DYNAMIC_RANGE_CEA,  "Full Range" },
              { NV_DYNAMIC_RANGE_AUTO, "Auto"     } };

        return dynamic_range_map [dynamic_range];
      }
    } nvapi_hdr;

    enum SK_HDR_TRANSFER_FUNC
    {
      sRGB,        // ~= Power-Law: 2.2

      scRGB,       // Linear, but source material is generally sRGB
                   //  >> And you have to undo that transformation!

      G19,         //   Power-Law: 1.9
      G20,         //   Power-Law: 2.0
      G22,         //   Power-Law: 2.2
      G24,         //   Power-Law: 2.4

      Linear=scRGB,// Alias for scRGB  (Power-Law: 1.0 = Linear!)

      SMPTE_2084,  // Perceptual Quantization
      HYBRID_LOG,  // TODO
      NONE
    };

    SK_HDR_TRANSFER_FUNC
    getEOTF (void);
  } scanout;

  // Set of displays that SK has enabled HDR on, so we can turn it back
  //   off if user wants
  std::set <output_s *> hdr_enabled_displays;

           bool checkHDRState (void); ///< Call in response to WM_DISPLAYCHANGE
  __inline void setHDRCapable (bool set) noexcept { hdr_capable = set; }
  __inline bool isHDRCapable  (void)     noexcept
  {
    ////if (framebuffer_flags & SK_FRAMEBUFFER_FLAG_HDR)
    ////  hdr_capable = true;

    if (driver_based_hdr)
      hdr_capable = true;

    return
      hdr_capable;
  }
  __inline bool isHDRActive (void) noexcept
  {
    return
      framebuffer_flags & SK_FRAMEBUFFER_FLAG_HDR;
  }

  struct                 {
    struct sub_event     {
      LARGE_INTEGER time = LARGE_INTEGER { 0LL, 0LL };
    } submit,
      begin_overlays,
      begin_imgui,   end_imgui,
      begin_texmgmt, end_texmgmt;
  } present_staging;


  struct latency_monitor_s {
    bool stale = true;

    struct {
        DXGI_FRAME_STATISTICS frameStats0   = {   };
        DXGI_FRAME_STATISTICS frameStats1   = {   };
        UINT                  lastPresent   =     0;
        ULONGLONG             lastFrame     =  0ULL;
    } counters;

    struct {
        UINT                 PresentQueue   =     0;
        UINT                    SyncDelay   =     0;
        float                     TotalMs   =  0.0F;
    } delays;

    struct {
        float                 AverageMs     =  0.0F;
        float                     MaxMs     =  0.0F;
        float                   ScaleMs     = 99.0F;
        float                 History [120] = {   };
    } stats;
    
    void submitQueuedFrame (IDXGISwapChain1 *pSwapChain);
  } static latency;


  struct frame_delta_s {
    LONGLONG lastFrame =  0LL;
    ULONG64  lastDelta = 0ULL;

    ULONG64 getDeltaTime (void) noexcept
    {
      return
        lastDelta;
    }

    void    markFrame    (void) noexcept
    {
      LONGLONG thisFrame =
        SK_QueryPerf ().QuadPart;

      lastDelta =
        ( thisFrame - lastFrame );
      lastFrame =
          thisFrame;
    }
  } frame_delta;

  static volatile ULONG64 frames_drawn;
  static volatile LONG    flip_skip; // DXGI flip queue glitch reduction

  struct window_registry_s
  {
    // Most input processing happens in this HWND's message pump
    sk_hwnd_cache_s       focus      { HWND_DESKTOP };

    // Defines the client rectangle and not much else
    sk_hwnd_cache_s       device     { HWND_DESKTOP };

    // This Unity engine is so terrible at window management that we need
    //   to flag the game and start disabling features!
    bool                  unity                = false;
    bool                  unreal               = false;
    bool                  capcom               = false;

    void setFocus  (HWND hWndFocus);
    void setDevice (HWND hWndRender);

    HWND getFocus  (void);
    HWND getDevice (void);
  } windows;

  // Pass Reserved to detect the API, pass an actual API
  //   to indicate a frame was drawn using this API
  void updateActiveAPI ( SK_RenderAPI _api =
                         SK_RenderAPI::Reserved );

  struct d3d10_s
  {
    SK_ComPtr <ID3D10Device>        device        = nullptr;
  } d3d10;

  // TODO: Proper abstraction
  struct d3d11_s
  {
    SK_ComPtr <ID3D11Device>        device        = nullptr;
               ID3D11DeviceContext* immediate_ctx = nullptr;
    SK_ComPtr <ID3D11DeviceContext> deferred_ctx  = nullptr;

    // Call to forcefully unbind Flip Model resources, as required
    // during SwapChain cleanup.
    void clearState (void)
    {
      if (immediate_ctx != nullptr)
      {
        immediate_ctx->Flush      ();
        immediate_ctx->ClearState ();
      }
    }
  } d3d11;

  struct d3d12_s
  {
    SK_ComPtr <ID3D12Device>       device        = nullptr;
    SK_ComPtr <ID3D12CommandQueue> command_queue = nullptr;
  } d3d12;


          HRESULT       setDevice (IUnknown* pDevice);
  template <typename Q>
          SK_ComPtr <Q> getDevice (void)
          {
            REFIID riid =
              __uuidof (Q);

            if ( riid == IID_IDirect3DDevice9
            ||   riid == IID_IDirect3DDevice9Ex
            ||   riid == IID_ID3D10Device
            ||   riid == IID_ID3D11Device
            ||   riid == IID_ID3D12Device      )
            {
              Q* pRet = nullptr;

              if (device.p != nullptr)
              {
                if (SUCCEEDED (SK_SafeQueryInterface (device, riid, (void **)&pRet)))
                {
                  return pRet;
                }
              }

              return nullptr;
            }

            else MessageBeep (0xFFFFFFFF);

            ///static_assert ( riid == __uuidof (IDirect3DDevice9)   ||
            ///                riid == __uuidof (IDirect3DDevice9Ex) ||
            ///                riid == __uuidof (ID3D11Device)       ||
            ///                riid == __uuidof (ID3D12Device),
            ///  "Unknown Render Device Class Requested" );

            return nullptr;
          }

  struct gsync_s
  { void update (bool force = false);

    BOOL  capable      = FALSE;
    BOOL  active       = FALSE;
    BOOL  disabled     = FALSE;
    DWORD last_checked = 0;
  } gsync_state;


  SK_ScreenshotManager
     screenshot_mgr;


  //
  // Somewhat meaningless, given how most engines manage
  //   memory, textures and shaders...
  //
  //   This is the thread that handles SwapChain Presentation;
  //     nothing else can safely be inferred about this thread.
  //
  volatile DWORD    /*primary_*/thread  =  0;
  volatile DWORD           last_thread  =  0;
  volatile ULONG64         most_frames  =  0;
  SK_Thread_HybridSpinlock res_lock;

  bool canEnterFullscreen    (void);

  void requestFullscreenMode (bool override = false);
  void requestWindowedMode   (bool override = false);

  double getActiveRefreshRate (HMONITOR hMonitor = 0 /*Default to HWND's nearest*/);

  HANDLE getSwapWaitHandle   (void);
  void releaseOwnedResources (void);

  void            queueUpdateOutputs   (void);
  void            updateOutputTopology (void);
  const output_s* getContainingOutput  (const RECT& rkRect);
  bool            assignOutputFromHWND (HWND hWndContainer);

  bool setLatencyMarkerNV (NV_LATENCY_MARKER_TYPE    marker);
  bool getLatencyReportNV (NV_LATENCY_RESULT_PARAMS *pGetLatencyParams);
  void driverSleepNV      (int site);

  std::string parseEDIDForName      (uint8_t* edid, size_t length);
  POINT       parseEDIDForNativeRes (uint8_t* edid, size_t length);

  bool update_outputs = false;
};
#pragma pack(pop)

LONG
__stdcall
SK_ChangeDisplaySettings (DEVMODEW *lpDevMode, DWORD dwFlags);

LONG
__stdcall
SK_ChangeDisplaySettingsEx ( _In_ LPCWSTR   lpszDeviceName,
                             _In_ DEVMODEW *lpDevMode,
                                  HWND      hWnd,
                             _In_ DWORD     dwFlags,
                             _In_ LPVOID    lParam );

using SK_RenderBackend = SK_RenderBackend_V2;

SK_RenderBackend&
__stdcall
SK_GetCurrentRenderBackend (void) noexcept;

void
__stdcall
SK_InitRenderBackends (void);

SK_API
IUnknown* __stdcall SK_Render_GetDevice (void);

SK_API
IUnknown* __stdcall SK_Render_GetSwapChain (void);

void SK_BootD3D8   (void);
void SK_BootDDraw  (void);
void SK_BootGlide  (void);

void SK_BootD3D9   (void);
void SK_BootDXGI   (void);
void SK_BootOpenGL (void);
void SK_BootVulkan (void);

BOOL SK_DXGI_SupportsTearing (void);


_Return_type_success_ (nullptr)
IUnknown*
SK_COM_ValidateRelease (IUnknown** ppObj);

const wchar_t*
SK_Render_GetAPIName (SK_RenderAPI api);

extern volatile ULONG64 SK_Reflex_LastFrameMarked;

__forceinline
ULONG64
__stdcall
SK_GetFramesDrawn (void) noexcept
{
  return
    ReadULong64Acquire (&SK_RenderBackend::frames_drawn);
}


// Compute the overlay area of two rectangles, A and B.
// (ax1, ay1) = left-top coordinates of A; (ax2, ay2) = right-bottom coordinates of A
// (bx1, by1) = left-top coordinates of B; (bx2, by2) = right-bottom coordinates of B
inline int
ComputeIntersectionArea ( int ax1, int ay1, int ax2, int ay2,
                          int bx1, int by1, int bx2, int by2 ) noexcept
{
  return std::max (0, std::min (ax2, bx2) -
                      std::max (ax1, bx1) ) *
         std::max (0, std::min (ay2, by2) -
                      std::max (ay1, by1) );
}


DPI_AWARENESS
SK_GetThreadDpiAwareness (void);

bool SK_RenderBackendUtil_IsFullscreen (void);
void SK_D3D_SetupShaderCompiler        (void);
void SK_Display_DisableDPIScaling      (void);

interface
SK_ICommandProcessor;
SK_ICommandProcessor*
     SK_Render_InitializeSharedCVars   (void);
void SK_Display_HookModeChangeAPIs     (void);

HMODULE
SK_D3D_GetShaderCompiler (void);

HRESULT
WINAPI
SK_D3D_Disassemble (_In_reads_bytes_(SrcDataSize) LPCVOID    pSrcData,
                    _In_                          SIZE_T     SrcDataSize,
                    _In_                          UINT       Flags,
                    _In_opt_                      LPCSTR     szComments,
                    _Out_                         ID3DBlob** ppDisassembly);

HRESULT
WINAPI
SK_D3D_Reflect (_In_reads_bytes_(SrcDataSize) LPCVOID pSrcData,
                _In_                          SIZE_T  SrcDataSize,
                _In_                          REFIID  pInterface,
                _Out_                         void**  ppReflector);

HRESULT
WINAPI
SK_D3D_Compile (
  _In_reads_bytes_(SrcDataSize)           LPCVOID           pSrcData,
  _In_                                    SIZE_T            SrcDataSize,
  _In_opt_                                LPCSTR            pSourceName,
  _In_reads_opt_(_Inexpressible_(pDefines->Name != NULL))
                                    CONST D3D_SHADER_MACRO* pDefines,
  _In_opt_                                ID3DInclude*      pInclude,
  _In_opt_                                LPCSTR            pEntrypoint,
  _In_                                    LPCSTR            pTarget,
  _In_                                    UINT              Flags1,
  _In_                                    UINT              Flags2,
  _Out_                                   ID3DBlob**        ppCode,
  _Always_(_Outptr_opt_result_maybenull_) ID3DBlob**        ppErrorMsgs);


#define D3DKMT_MAX_WAITFORVERTICALBLANK_OBJECTS 8

typedef struct _D3DKMT_GETVERTICALBLANKEVENT {
  D3DKMT_HANDLE                  hAdapter;
  D3DKMT_HANDLE                  hDevice;
  D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
  HANDLE                         *phEvent;
} D3DKMT_GETVERTICALBLANKEVENT;

typedef struct _D3DKMT_WAITFORVERTICALBLANKEVENT2 {
  D3DKMT_HANDLE                  hAdapter;
  D3DKMT_HANDLE                  hDevice;
  D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
  UINT                           NumObjects;
  HANDLE                         ObjectHandleArray [D3DKMT_MAX_WAITFORVERTICALBLANK_OBJECTS];
} D3DKMT_WAITFORVERTICALBLANKEVENT2;

typedef struct _D3DKMT_WAITFORVERTICALBLANKEVENT {
  D3DKMT_HANDLE                  hAdapter;
  D3DKMT_HANDLE                  hDevice;
  D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
} D3DKMT_WAITFORVERTICALBLANKEVENT;

typedef struct _D3DKMT_GETSCANLINE {
  D3DKMT_HANDLE                  hAdapter;
  D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
  BOOLEAN                        InVerticalBlank;
  UINT                           ScanLine;
} D3DKMT_GETSCANLINE;

typedef struct _D3DKMT_SETSTABLEPOWERSTATE {
  D3DKMT_HANDLE hAdapter;
  BOOL          Enabled;
} D3DKMT_SETSTABLEPOWERSTATE;

using D3DKMTGetDWMVerticalBlankEvent_pfn   = NTSTATUS (WINAPI *)(const D3DKMT_GETVERTICALBLANKEVENT      *unnamedParam1);
using D3DKMTWaitForVerticalBlankEvent2_pfn = NTSTATUS (WINAPI *)(const D3DKMT_WAITFORVERTICALBLANKEVENT2 *unnamedParam1);
using D3DKMTWaitForVerticalBlankEvent_pfn  = NTSTATUS (WINAPI *)(const D3DKMT_WAITFORVERTICALBLANKEVENT  *unnamedParam1);
using D3DKMTGetScanLine_pfn                = NTSTATUS (WINAPI *)(D3DKMT_GETSCANLINE                      *unnamedParam1);
using D3DKMTSetStablePowerState_pfn        = NTSTATUS (WINAPI *)(const D3DKMT_SETSTABLEPOWERSTATE        *unnamedParam1);

void SK_Display_ApplyDesktopResolution (MONITORINFOEX& mi);

// Disables Vulkan layers (i.e. if using DXGI interop, prefer software hook the D3D11 SwapChain and not Vulkan)
void SK_Vulkan_DisableThirdPartyLayers (void); // Can only be called during application startup

// Move this to a more formal presentation manager
extern bool SK_GL_OnD3D11;
extern bool SK_GL_OnD3D11_Reset; // This one especially, this has a signal

#endif /* __SK__RENDER_BACKEND__H__ */