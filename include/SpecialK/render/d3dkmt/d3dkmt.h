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

#ifndef __SK__D3DKMT_H__
#define __SK__D3DKMT_H__

#include <Unknwnbase.h>
#include <dxmini.h>

typedef UINT D3DDDI_VIDEO_PRESENT_SOURCE_ID;

typedef UINT32          D3DKMT_HANDLE;
typedef D3DKMT_HANDLE *PD3DKMT_HANDLE;

#define DXGK_MAX_METADATA_NAME_LENGTH 32
typedef enum
{
  DXGK_ENGINE_TYPE_OTHER,
  DXGK_ENGINE_TYPE_3D,
  DXGK_ENGINE_TYPE_VIDEO_DECODE,
  DXGK_ENGINE_TYPE_VIDEO_ENCODE,
  DXGK_ENGINE_TYPE_VIDEO_PROCESSING,
  DXGK_ENGINE_TYPE_SCENE_ASSEMBLY,
  DXGK_ENGINE_TYPE_COPY,
  DXGK_ENGINE_TYPE_OVERLAY,
  DXGK_ENGINE_TYPE_CRYPTO,
  DXGK_ENGINE_TYPE_MAX
} DXGK_ENGINE_TYPE;

typedef struct _DXGK_NODEMETADATA_FLAGS
{
  union
  {
    struct
    {
      UINT ContextSchedulingSupported :  1;
      UINT RingBufferFenceRelease     :  1;
      UINT SupportTrackedWorkload     :  1;
      UINT Reserved                   : 13;
      UINT MaxInFlightHwQueueBuffers  : 16;
    };
    
    UINT32 Value;
  };
} DXGK_NODEMETADATA_FLAGS;

typedef struct _DXGK_NODEMETADATA
{
  DXGK_ENGINE_TYPE        EngineType;
  WCHAR                   FriendlyName [DXGK_MAX_METADATA_NAME_LENGTH];
  DXGK_NODEMETADATA_FLAGS Flags;
  BOOLEAN                 GpuMmuSupported;
  BOOLEAN                 IoMmuSupported;
} DXGK_NODEMETADATA;

typedef struct _D3DKMT_NODEMETADATA
{
  _In_  UINT              NodeOrdinalAndAdapterIndex; // WDDMv2: High word is physical adapter index, low word is node ordinal
  _Out_ DXGK_NODEMETADATA NodeData;
} D3DKMT_NODEMETADATA;

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
  UINT    DisableGpuTimeout        :  1;   // 0x00000004
  UINT    Reserved                 : 29;   // 0xFFFFFFF8
} D3DKMT_CREATEDEVICEFLAGS;

typedef VOID D3DDDI_ALLOCATIONLIST;
typedef VOID D3DDDI_PATCHLOCATIONLIST;

typedef enum _D3DKMDT_VIDEO_SIGNAL_STANDARD
{
  D3DKMDT_VSS_UNINITIALIZED,
  D3DKMDT_VSS_VESA_DMT,
  D3DKMDT_VSS_VESA_GTF,
  D3DKMDT_VSS_VESA_CVT,
  D3DKMDT_VSS_IBM,
  D3DKMDT_VSS_APPLE,
  D3DKMDT_VSS_NTSC_M,
  D3DKMDT_VSS_NTSC_J,
  D3DKMDT_VSS_NTSC_443,
  D3DKMDT_VSS_PAL_B,
  D3DKMDT_VSS_PAL_B1,
  D3DKMDT_VSS_PAL_G,
  D3DKMDT_VSS_PAL_H,
  D3DKMDT_VSS_PAL_I,
  D3DKMDT_VSS_PAL_D,
  D3DKMDT_VSS_PAL_N,
  D3DKMDT_VSS_PAL_NC,
  D3DKMDT_VSS_SECAM_B,
  D3DKMDT_VSS_SECAM_D,
  D3DKMDT_VSS_SECAM_G,
  D3DKMDT_VSS_SECAM_H,
  D3DKMDT_VSS_SECAM_K,
  D3DKMDT_VSS_SECAM_K1,
  D3DKMDT_VSS_SECAM_L,
  D3DKMDT_VSS_SECAM_L1,
  D3DKMDT_VSS_EIA_861,
  D3DKMDT_VSS_EIA_861A,
  D3DKMDT_VSS_EIA_861B,
  D3DKMDT_VSS_PAL_K,
  D3DKMDT_VSS_PAL_K1,
  D3DKMDT_VSS_PAL_L,
  D3DKMDT_VSS_PAL_M,
  D3DKMDT_VSS_OTHER
} D3DKMDT_VIDEO_SIGNAL_STANDARD;

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

typedef struct _D3DKMT_WDDM_2_0_CAPS
{
  union {
    struct {
      UINT Support64BitAtomics       : 1;
      UINT GpuMmuSupported           : 1;
      UINT IoMmuSupported            : 1;

      // Since WDDM2_4
      //
      UINT FlipOverwriteSupported    : 1;
      UINT SupportContextlessPresent : 1;

      // Since WDDM2_5
      //
      UINT SupportSurpriseRemoval    :  1;
      UINT Reserved                  : 26;
    };
    UINT Value;
  };
} D3DKMT_WDDM_2_0_CAPS;

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

// DXGK_FEATURE_SUPPORT constants

// When a driver doesn't support a feature, it doesn't call into QueryFeatureSupport with that feature ID.
// This value is provided for implementation convenience of enumerating possible driver support states
// for a particular feature.
#define DXGK_FEATURE_SUPPORT_ALWAYS_OFF ((UINT)0)

// Driver support for a feature is in the experimental state
#define DXGK_FEATURE_SUPPORT_EXPERIMENTAL ((UINT)1)

// Driver support for a feature is in the stable state
#define DXGK_FEATURE_SUPPORT_STABLE ((UINT)2)

// Driver support for a feature is in the always on state,
// and it doesn't operate without this feature enabled.
#define DXGK_FEATURE_SUPPORT_ALWAYS_ON ((UINT)3)

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

typedef enum _QAI_DRIVERVERSION
{
  KMT_DRIVERVERSION_WDDM_1_0            = 1000,
  KMT_DRIVERVERSION_WDDM_1_1_PRERELEASE = 1102,
  KMT_DRIVERVERSION_WDDM_1_1            = 1105,
  KMT_DRIVERVERSION_WDDM_1_2            = 1200,
  KMT_DRIVERVERSION_WDDM_1_3            = 1300,
  KMT_DRIVERVERSION_WDDM_2_0            = 2000,
  KMT_DRIVERVERSION_WDDM_2_1            = 2100,
  KMT_DRIVERVERSION_WDDM_2_2            = 2200,
  KMT_DRIVERVERSION_WDDM_2_3            = 2300,
  KMT_DRIVERVERSION_WDDM_2_4            = 2400,
  KMT_DRIVERVERSION_WDDM_2_5            = 2500,
  KMT_DRIVERVERSION_WDDM_2_6            = 2600,
  KMT_DRIVERVERSION_WDDM_2_7            = 2700,
  KMT_DRIVERVERSION_WDDM_2_8            = 2800,
  KMT_DRIVERVERSION_WDDM_2_9            = 2900,
  KMT_DRIVERVERSION_WDDM_3_0            = 3000,
  KMT_DRIVERVERSION_WDDM_3_1            = 3100
} D3DKMT_DRIVERVERSION;

struct SK_WDDM_CAPS {
  D3DKMT_DRIVERVERSION version =
     KMT_DRIVERVERSION_WDDM_1_0;

  D3DKMT_WDDM_1_2_CAPS _1_2 = { };
  D3DKMT_WDDM_1_3_CAPS _1_3 = { };
  D3DKMT_WDDM_2_0_CAPS _2_0 = { };
  D3DKMT_WDDM_2_7_CAPS _2_7 = { };
  D3DKMT_WDDM_2_9_CAPS _2_9 = { };
  D3DKMT_WDDM_3_0_CAPS _3_0 = { };

  void init (D3DKMT_HANDLE hAdapter);
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
  // WDDM2_6
  KMTQAITYPE_DRIVER_DESCRIPTION                       = 65, // D3DKMT_DRIVER_DESCRIPTION
  KMTQAITYPE_DRIVER_DESCRIPTION_RENDER                = 66, // D3DKMT_DRIVER_DESCRIPTION
  KMTQAITYPE_SCANOUT_CAPS                             = 67, // D3DKMT_QUERY_SCANOUT_CAPS
  KMTQAITYPE_PARAVIRTUALIZATION_RENDER                = 68, // D3DKMT_PARAVIRTUALIZATION
  // WDDM2_7
  KMTQAITYPE_SERVICENAME                              = 69,
  KMTQAITYPE_WDDM_2_7_CAPS                            = 70, // D3DKMT_WDDM_2_7_CAPS
  KMTQAITYPE_DISPLAY_UMDRIVERNAME                     = 71, // Added in 19H2
  KMTQAITYPE_TRACKEDWORKLOAD_SUPPORT                  = 72,
  // WDDM2_8
  KMTQAITYPE_HYBRID_DLIST_DLL_SUPPORT                 = 73,
  KMTQAITYPE_DISPLAY_CAPS                             = 74,
  // WDDM2_9
  KMTQAITYPE_WDDM_2_9_CAPS                            = 75, // D3DKMT_WDDM_2_9_CAPS
  KMTQAITYPE_CROSSADAPTERRESOURCE_SUPPORT             = 76,
  // WDDM3_0
  KMTQAITYPE_WDDM_3_0_CAPS                            = 77, // D3DKMT_WDDM_3_0_CAPS
  D3DKMT_FORCE_DWORD                                  = 0x7fffffff
} KMTQUERYADAPTERINFOTYPE;

typedef struct _D3DKMT_QUERYADAPTERINFO
{
  _In_                                     D3DKMT_HANDLE           AdapterHandle;
  _In_                                     KMTQUERYADAPTERINFOTYPE Type;
  _Inout_bytecount_(PrivateDriverDataSize) PVOID                   PrivateDriverData;
  _Out_      UINT32 PrivateDriverDataSize;
} D3DKMT_QUERYADAPTERINFO;

typedef struct _D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME {
  WCHAR                          DeviceName [32];
  D3DKMT_HANDLE                  hAdapter;
  LUID                           AdapterLuid;
  D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
} D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME;

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
  D3DKMT_PM_UNINITIALIZED          = 0,
  D3DKMT_PM_REDIRECTED_GDI         = 1,
  D3DKMT_PM_REDIRECTED_FLIP        = 2,
  D3DKMT_PM_REDIRECTED_BLT         = 3,
  D3DKMT_PM_REDIRECTED_VISTABLT    = 4,
  D3DKMT_PM_SCREENCAPTUREFENCE     = 5,
  D3DKMT_PM_REDIRECTED_GDI_SYSMEM  = 6,
  D3DKMT_PM_REDIRECTED_COMPOSITION = 7,
  D3DKMT_PM_SURFACECOMPLETE        = 8,
  D3DKMT_PM_FLIPMANAGER            = 9,
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
      UINT StereoPreferRight             :  1;
      UINT TemporaryMono                 :  1;
      UINT FlipRestart                   :  1;
      UINT ScatterBlt                    :  1;
      UINT AlphaMode                     :  2;
      UINT SignalLimitOnTokenCompletion  :  1;
      UINT Reserved                      : 22;
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
  D3DKMT_DIRTYREGIONS                       DirtyRegions;
} D3DKMT_FLIPMODEL_PRESENTHISTORYTOKEN;

typedef struct _D3DKMT_COMPOSITION_PRESENTHISTORYTOKEN {
  ULONG64 hPrivateData;
} D3DKMT_COMPOSITION_PRESENTHISTORYTOKEN;

typedef ULONGLONG  D3DKMT_VISTABLTMODEL_PRESENTHISTORYTOKEN;

typedef struct _D3DKMT_PRESENTHISTORYTOKEN {
  D3DKMT_PRESENT_MODEL Model;
  UINT                 TokenSize;
  UINT64               CompositionBindingId;
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

HRESULT SK_D3DKMT_QueryAdapterInfo (D3DKMT_QUERYADAPTERINFO *pQueryAdapterInfo);
HRESULT SK_D3DKMT_CloseAdapter     (D3DKMT_CLOSEADAPTER     *pCloseAdapter);

typedef NTSTATUS (NTAPI *PFND3DKMT_QUERYADAPTERINFO)(    D3DKMT_QUERYADAPTERINFO    *pData);
typedef NTSTATUS (NTAPI *PFND3DKMT_OPENADAPTERFROMLUID)( D3DKMT_OPENADAPTERFROMLUID *unnamedParam1);
typedef NTSTATUS (NTAPI *PFND3DKMT_CLOSEADAPTER)(        D3DKMT_CLOSEADAPTER        *unnamedParam1);
typedef NTSTATUS (NTAPI *PFND3DKMT_CREATEDEVICE)(        D3DKMT_CREATEDEVICE        *unnamedParam1);
typedef NTSTATUS (NTAPI *PFND3DKMT_DESTROYDEVICE)(const  D3DKMT_DESTROYDEVICE       *unnamedParam1);

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

using D3DKMTGetDWMVerticalBlankEvent_pfn      = NTSTATUS (WINAPI *)(const D3DKMT_GETVERTICALBLANKEVENT      *unnamedParam1);
using D3DKMTWaitForVerticalBlankEvent2_pfn    = NTSTATUS (WINAPI *)(const D3DKMT_WAITFORVERTICALBLANKEVENT2 *unnamedParam1);
using D3DKMTWaitForVerticalBlankEvent_pfn     = NTSTATUS (WINAPI *)(const D3DKMT_WAITFORVERTICALBLANKEVENT  *unnamedParam1);
using D3DKMTGetScanLine_pfn                   = NTSTATUS (WINAPI *)(D3DKMT_GETSCANLINE                      *unnamedParam1);
using D3DKMTSetStablePowerState_pfn           = NTSTATUS (WINAPI *)(const D3DKMT_SETSTABLEPOWERSTATE        *unnamedParam1);
using D3DKMTGetMultiPlaneOverlayCaps_pfn      = NTSTATUS (WINAPI *)(D3DKMT_GET_MULTIPLANE_OVERLAY_CAPS      *unnamedParam1);
using D3DKMTOpenAdapterFromGdiDisplayName_pfn = NTSTATUS (WINAPI *)(D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME    *unnamedParam1);
using D3DKMTOpenAdapterFromLuid_pfn           = NTSTATUS (WINAPI *)(D3DKMT_OPENADAPTERFROMLUID              *unnamedParam1);

#define SK_D3DKMT_Decl(x) extern "C" __declspec (dllexport) extern \
                                          FARPROC (x)

SK_D3DKMT_Decl (D3DKMTCloseAdapter);
SK_D3DKMT_Decl (D3DKMTDestroyAllocation);
SK_D3DKMT_Decl (D3DKMTDestroyContext);
SK_D3DKMT_Decl (D3DKMTDestroyDevice);
SK_D3DKMT_Decl (D3DKMTDestroySynchronizationObject);
SK_D3DKMT_Decl (D3DKMTQueryAdapterInfo);
SK_D3DKMT_Decl (D3DKMTSetDisplayPrivateDriverFormat);
SK_D3DKMT_Decl (D3DKMTSignalSynchronizationObject);
SK_D3DKMT_Decl (D3DKMTUnlock);
SK_D3DKMT_Decl (D3DKMTWaitForSynchronizationObject);
SK_D3DKMT_Decl (D3DKMTCreateAllocation);
SK_D3DKMT_Decl (D3DKMTCreateContext);
SK_D3DKMT_Decl (D3DKMTCreateDevice);
SK_D3DKMT_Decl (D3DKMTCreateSynchronizationObject);
SK_D3DKMT_Decl (D3DKMTEscape);
SK_D3DKMT_Decl (D3DKMTGetContextSchedulingPriority);
SK_D3DKMT_Decl (D3DKMTGetDeviceState);
SK_D3DKMT_Decl (D3DKMTGetDisplayModeList);
SK_D3DKMT_Decl (D3DKMTGetMultisampleMethodList);
SK_D3DKMT_Decl (D3DKMTGetRuntimeData);
SK_D3DKMT_Decl (D3DKMTGetSharedPrimaryHandle);
SK_D3DKMT_Decl (D3DKMTLock);
SK_D3DKMT_Decl (D3DKMTOpenAdapterFromHdc);
SK_D3DKMT_Decl (D3DKMTOpenAdapterFromLuid);
SK_D3DKMT_Decl (D3DKMTOpenAdapterFromGdiDisplayName);
SK_D3DKMT_Decl (D3DKMTOpenResource);
SK_D3DKMT_Decl (D3DKMTPresent);
SK_D3DKMT_Decl (D3DKMTQueryAllocationResidency);
SK_D3DKMT_Decl (D3DKMTQueryResourceInfo);
SK_D3DKMT_Decl (D3DKMTRender);
SK_D3DKMT_Decl (D3DKMTSetAllocationPriority);
SK_D3DKMT_Decl (D3DKMTSetContextSchedulingPriority);
SK_D3DKMT_Decl (D3DKMTSetDisplayMode);
SK_D3DKMT_Decl (D3DKMTSetGammaRamp);
SK_D3DKMT_Decl (D3DKMTSetVidPnSourceOwner);
SK_D3DKMT_Decl (D3DKMTWaitForVerticalBlankEvent);
SK_D3DKMT_Decl (D3DKMTGetMultiPlaneOverlayCaps);
SK_D3DKMT_Decl (D3DKMTGetScanLine);

#endif /* __SK__D3DKMT__H__ */