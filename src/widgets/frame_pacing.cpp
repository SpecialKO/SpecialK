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

extern iSK_INI* osd_ini;

extern void SK_ImGui_DrawGraph_FramePacing (void);

#include <SpecialK/render/d3d11/d3d11_core.h>

namespace
SK_ImGui
{
  bool BatteryMeter (void);
};

static bool has_battery   = false;
static bool debug_limiter = false;

struct SK_ImGui_FramePercentiles
{
  bool  display             =  true;
  bool  display_above       =  true;
  bool  display_most_recent =  true;
  int   seconds_to_sample   =     0; // Unlimited

  struct
  {
    sk::ParameterBool*  display             = nullptr;
    sk::ParameterBool*  display_above       = nullptr;
    sk::ParameterBool*  display_most_recent = nullptr;
    sk::ParameterInt*   seconds_to_sample   = nullptr;
    sk::ParameterFloat* percentile0_cutoff  = nullptr;
    sk::ParameterFloat* percentile1_cutoff  = nullptr;
  } percentile_cfg;

  void load_percentile_cfg (void)
  {
    assert (osd_ini != nullptr);

    percentile_cfg.display =
      dynamic_cast <sk::ParameterBool *> (
        SK_Widget_ParameterFactory->create_parameter <bool> (
          L"Show Statistics"
        )
      );

    percentile_cfg.display_above =
      dynamic_cast <sk::ParameterBool *> (
        SK_Widget_ParameterFactory->create_parameter <bool> (
          L"Draw Statistics Above Line Graph"
        )
      );

    percentile_cfg.display_most_recent =
      dynamic_cast <sk::ParameterBool *> (
        SK_Widget_ParameterFactory->create_parameter <bool> (
          L"Limit sample dataset to the past ~30 seconds at most."
        )
      );

    percentile_cfg.percentile0_cutoff =
      dynamic_cast <sk::ParameterFloat *> (
        SK_Widget_ParameterFactory->create_parameter <float> (
          L"Boundary that defines the percentile being profiled."
        )
      );

    percentile_cfg.percentile1_cutoff =
      dynamic_cast <sk::ParameterFloat *> (
        SK_Widget_ParameterFactory->create_parameter <float> (
          L"Boundary that defines the percentile being profiled."
        )
      );

    percentile_cfg.display->register_to_ini ( osd_ini,
      L"Widget.FramePacing", L"DisplayPercentiles"
    );

    percentile_cfg.display_above->register_to_ini ( osd_ini,
      L"Widget.FramePacing", L"PercentilesAboveGraph"
    );
    percentile_cfg.display_most_recent->register_to_ini ( osd_ini,
      L"Widget.FramePacing", L"ShortTermPercentiles"
    );
    percentile_cfg.percentile0_cutoff->register_to_ini ( osd_ini,
      L"Widget.FramePacing", L"Percentile[0].Cutoff"
    );
    percentile_cfg.percentile1_cutoff->register_to_ini ( osd_ini,
      L"Widget.FramePacing", L"Percentile[1].Cutoff"
    );

    percentile_cfg.display->load             (display);
    percentile_cfg.display_above->load       (display_above);
    percentile_cfg.display_most_recent->load (display_most_recent);

    if (! percentile_cfg.percentile0_cutoff->load  (percentile0.cutoff))
          percentile0.cutoff = 1.0f; // 1.0%

    if (! percentile_cfg.percentile1_cutoff->load  (percentile1.cutoff))
          percentile1.cutoff = 0.1f; // 0.1%

    static auto* cp =
      SK_GetCommandProcessor ();

    SK_RunOnce (cp->AddVariable ("FramePacing.DisplayPercentiles",    SK_CreateVar (SK_IVariable::Boolean, &display)));
    SK_RunOnce (cp->AddVariable ("FramePacing.PercentilesAboveGraph", SK_CreateVar (SK_IVariable::Boolean, &display_above)));
    SK_RunOnce (cp->AddVariable ("FramePacing.ShortTermPercentiles",  SK_CreateVar (SK_IVariable::Boolean, &display_most_recent)));
    SK_RunOnce (cp->AddVariable ("FramePacing.Percentile[0].Cutoff",  SK_CreateVar (SK_IVariable::Float,   &percentile0.cutoff)));
    SK_RunOnce (cp->AddVariable ("FramePacing.Percentile[1].Cutoff",  SK_CreateVar (SK_IVariable::Float,   &percentile1.cutoff)));
  }

  void store_percentile_cfg (void)
  {
    percentile_cfg.display->store             (display);
    percentile_cfg.display_above->store       (display_above);
    percentile_cfg.display_most_recent->store (display_most_recent);
    percentile_cfg.percentile0_cutoff->store  (percentile0.cutoff);
    percentile_cfg.percentile1_cutoff->store  (percentile1.cutoff);
  }

  struct sample_subset_s
  {
    float   cutoff          =  0.0f;
    bool    has_data        = false;
    float   computed_fps    =  0.0f;
    ULONG64 last_calculated =   0UL;

    void computeFPS (long double dt)
    {
      computed_fps =
        (float)(1000.0L / dt);

      last_calculated =
        SK_GetFramesDrawn ();
    }
  } percentile0,
    percentile1, mean;

  void toggleDisplay (void)
  {
    display = (! display);

    store_percentile_cfg ();
  }
};

SK_LazyGlobal <SK_ImGui_FramePercentiles> SK_FramePercentiles;

float SK_Framerate_GetPercentileByIdx (int idx)
{
  if (idx == 0)
    return SK_FramePercentiles->percentile0.cutoff;

  if (idx == 1)
    return SK_FramePercentiles->percentile1.cutoff;

  return 0.0f;
}

extern float __target_fps;
extern float __target_fps_bg;

class SK_ImGui_FrameHistory : public SK_Stat_DataHistory <float, 120>
{
public:
  void timeFrame       (double seconds)
  {
    addValue ((float)(1000.0 * seconds));
  }
};

SK_LazyGlobal <SK_ImGui_FrameHistory> SK_ImGui_Frames;


#include <dwmapi.h>

HRESULT
WINAPI
SK_DWM_GetCompositionTimingInfo (DWM_TIMING_INFO *pTimingInfo)
{
  static HMODULE hModDwmApi =
    SK_LoadLibraryW (L"dwmapi.dll");

  typedef HRESULT (WINAPI *DwmGetCompositionTimingInfo_pfn)(
                   HWND             hwnd,
                   DWM_TIMING_INFO *pTimingInfo);

  static                   DwmGetCompositionTimingInfo_pfn
                           DwmGetCompositionTimingInfo =
         reinterpret_cast <DwmGetCompositionTimingInfo_pfn> (
    SK_GetProcAddress ( hModDwmApi,
                          "DwmGetCompositionTimingInfo" )   );

  return
    DwmGetCompositionTimingInfo ( 0, pTimingInfo );
}



typedef UINT D3DDDI_VIDEO_PRESENT_SOURCE_ID;
typedef UINT D3DKMT_HANDLE;

typedef struct _D3DKMT_OPENADAPTERFROMHDC
{
  HDC                             hDc;            // in:  DC that maps to a single display
  D3DKMT_HANDLE                   hAdapter;       // out: adapter handle
  LUID                            AdapterLuid;    // out: adapter LUID
  D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;  // out: VidPN source ID for that particular display
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

typedef UINT32 D3DKMT_HANDLE;

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
  KMTQAITYPE_DRIVER_DESCRIPTION                       = 65, // D3DKMT_DRIVER_DESCRIPTION // since WDDM2_6
  KMTQAITYPE_DRIVER_DESCRIPTION_RENDER                = 66, // D3DKMT_DRIVER_DESCRIPTION
  KMTQAITYPE_SCANOUT_CAPS                             = 67, // D3DKMT_QUERY_SCANOUT_CAPS
} KMTQUERYADAPTERINFOTYPE;

typedef struct _D3DKMT_QUERYADAPTERINFO
{
  _In_                                     D3DKMT_HANDLE           AdapterHandle;
  _In_                                     KMTQUERYADAPTERINFOTYPE Type;
  _Inout_bytecount_(PrivateDriverDataSize) PVOID                   PrivateDriverData;
  _Out_      UINT32 PrivateDriverDataSize;
} D3DKMT_QUERYADAPTERINFO;

typedef NTSTATUS (NTAPI *PFND3DKMT_QUERYADAPTERINFO)(D3DKMT_QUERYADAPTERINFO *pData);

HRESULT
SK_D3DKMT_QueryAdapterInfo (D3DKMT_QUERYADAPTERINFO *pQueryAdapterInfo)
{
  if (! D3DKMTQueryAdapterInfo)
        D3DKMTQueryAdapterInfo =
        SK_GetProcAddress (
          SK_LoadLibraryW ( L"gdi32.dll" ),
            "D3DKMTQueryAdapterInfo"
                          );

  if (D3DKMTQueryAdapterInfo != nullptr)
  {
    return
       reinterpret_cast <
         PFND3DKMT_QUERYADAPTERINFO                    > (
             D3DKMTQueryAdapterInfo) (pQueryAdapterInfo);
  }

  return E_FAIL;
}

HRESULT
SK_D3DKMT_QueryAdapterPerfData ( HDC                      hDC,
                                 D3DKMT_ADAPTER_PERFDATA *pAdapterPerfData )
{
  D3DKMT_OPENADAPTERFROMHDC oa     = { };
                            oa.hDc = hDC;

  if (! D3DKMTOpenAdapterFromHdc)
  {     D3DKMTOpenAdapterFromHdc =
        SK_GetProcAddress (
          SK_LoadLibraryW ( L"gdi32.dll" ),
            "D3DKMTOpenAdapterFromHdc"
                          );
  }

  NTSTATUS result =
        reinterpret_cast <PFND3DKMT_OPENADAPTERFROMHDC> (D3DKMTOpenAdapterFromHdc)(&oa);

  if (SUCCEEDED (result))
  {
    D3DKMT_QUERYADAPTERINFO
      queryAdapterInfo                       = { };
      queryAdapterInfo.AdapterHandle         = oa.hAdapter;
      queryAdapterInfo.Type                  = KMTQAITYPE_ADAPTERPERFDATA;
      queryAdapterInfo.PrivateDriverData     =          pAdapterPerfData;
      queryAdapterInfo.PrivateDriverDataSize = sizeof (*pAdapterPerfData);

    result =
      SK_D3DKMT_QueryAdapterInfo (&queryAdapterInfo);
  }

  return result;
}

void
SK_ImGui_DrawGraph_FramePacing (void)
{
  static const auto& io =
    ImGui::GetIO ();

  const  float font_size           =
    ( ImGui::GetFont  ()->FontSize * io.FontGlobalScale );

  const  float font_size_multiline =
    ( ImGui::GetStyle ().ItemSpacing.y      +
      ImGui::GetStyle ().ItemInnerSpacing.y + font_size );


  float sum = 0.0f;

  float min = FLT_MAX;
  float max = 0.0f;

  for ( const auto& val : SK_ImGui_Frames->getValues () )
  {
    sum += val;

    if (val > max)
      max = val;

    if (val < min)
      min = val;
  }

  static       char szAvg [512] = { };
  static const bool ffx = SK_GetModuleHandle (L"UnX.dll") != nullptr;

  float& target =
    ( SK_IsGameWindowActive () || __target_fps_bg == 0.0f ) ?
                  __target_fps  : __target_fps_bg;

  float target_frametime = ( target == 0.0f ) ?
                           ( 1000.0f   / (ffx ? 30.0f : 60.0f) ) :
                             ( 1000.0f / fabs (target) );

  if (target == 0.0f && (! ffx))
  {
    DWM_TIMING_INFO dwmTiming        = {                      };
                    dwmTiming.cbSize = sizeof (DWM_TIMING_INFO);

    if ( SUCCEEDED ( SK_DWM_GetCompositionTimingInfo (&dwmTiming) ) )
    {
      target_frametime =
        static_cast <float> (
        static_cast <double> (dwmTiming.qpcRefreshPeriod) /
        static_cast <double> (SK_GetPerfFreq ().QuadPart)
                            ) * 1000.0f;
    }
  }

  float frames =
    std::min ( (float)SK_ImGui_Frames->getUpdates  (),
               (float)SK_ImGui_Frames->getCapacity () );


  if (ffx)
  {
    // Normal Gameplay: 30 FPS
    if (sum / frames > 30.0)
      target_frametime = 33.333333f;

    // Menus: 60 FPS
    else
      target_frametime = 16.666667f;
  }

  extern void SK_ImGui_DrawFramePercentiles (void);

  if (SK_FramePercentiles->display_above)
      SK_ImGui_DrawFramePercentiles ();


  SK_ComQIPtr <IDXGISwapChain1> pSwap1 (
    SK_GetCurrentRenderBackend ().swapchain.p
  );

  struct {
    struct {
        DXGI_FRAME_STATISTICS frameStats0   = {   };
        DXGI_FRAME_STATISTICS frameStats1   = {   };
        UINT                  lastPresent   =     0;
        ULONGLONG             lastFrame     =  0ULL;
    } counters;
    struct {
        UINT                 PresentQueue   =     0;
        UINT                    SyncDelay   =     0;
        float                     TotalMs   =  0.0f;
    } delays;
    struct {
        float                 AverageMs     =  0.0f;
        float                     MaxMs     =  0.0f;
        float                   ScaleMs     = 99.0f;
        float                 History [120] = {   };
    } stats;
  } static latency;

  latency.stats.MaxMs =
    *std::max_element ( std::begin (latency.stats.History),
                        std::end   (latency.stats.History) );
  latency.stats.ScaleMs =
    std::max ( 99.0f, latency.stats.MaxMs );

  static bool has_latency = false;

  if (pSwap1.p != nullptr)
  {
    // Queue the insanity of trying to track render latency using static
    //   variables, when there can be 2 copies of this widget open at once (!!)
    if (              latency.counters.lastFrame !=
       std::exchange (latency.counters.lastFrame, SK_GetFramesDrawn ()) )
    {
      if (SUCCEEDED (pSwap1->GetFrameStatistics  (&latency.counters.frameStats1)))
      {              pSwap1->GetLastPresentCount (&latency.counters.lastPresent);

        SK_RunOnce (latency.counters.frameStats0 = latency.counters.frameStats1);

        LARGE_INTEGER time;
                      time.QuadPart =
          ( latency.counters.frameStats1.SyncQPCTime.QuadPart -
            latency.counters.frameStats0.SyncQPCTime.QuadPart );

        latency.delays.PresentQueue =
          ( latency.counters.lastPresent -
            latency.counters.frameStats1.PresentCount );

        latency.delays.SyncDelay    =
          ( latency.counters.frameStats1.SyncRefreshCount -
            latency.counters.frameStats0.SyncRefreshCount );

        latency.delays .TotalMs     =
                    static_cast <float>  (
           1000.0 * static_cast <double> (             time.QuadPart) /
                    static_cast <double> (SK_GetPerfFreq ().QuadPart) ) -
             SK_ImGui_Frames->getLastValue ();

        latency.counters.frameStats0 = latency.counters.frameStats1;

        has_latency = true;
      }

      else has_latency = false;

      latency.stats.History [latency.counters.lastFrame % std::size (latency.stats.History)] =
        fabs (latency.delays.TotalMs);
    }

    latency.stats.AverageMs =
      std::accumulate (
        std::begin ( latency.stats.History ),
        std::end   ( latency.stats.History ), 0.0f )
      / std::size  ( latency.stats.History );

  }

  if (has_latency)
  {
    snprintf
      ( szAvg,
          511, (const char *)
          u8"Avg milliseconds per-frame: %6.3f  (Target: %6.3f)\n"
          u8"         Render latency:           %lu Frame%s | %3.1f / %3.1f ms |  %lu Hz\n\n\n\n"
          u8"Variation:  %8.5f ms        %.1f FPS  ±  %3.1f frames",
              sum / frames,
                target_frametime,
                    latency.delays.PresentQueue,
                    latency.delays.PresentQueue != 1 ?
                                                "s " : "  ",
                      latency.stats.AverageMs,
                      latency.stats.MaxMs,
                        latency.delays.SyncDelay,
            (double)max - (double)min,
                    1000.0f / (sum / frames),
                      ((double)max-(double)min)/(1000.0f/(sum/frames)) );
  }

  else
  {
    snprintf
      ( szAvg,
          511, (const char *)
          u8"Avg milliseconds per-frame: %6.3f  (Target: %6.3f)\n"
          u8"    Extreme frame times:     %6.3f min, %6.3f max\n\n\n\n"
          u8"Variation:  %8.5f ms        %.1f FPS  ±  %3.1f frames",
              sum / frames,
                target_frametime,
                  min, max,
            (double)max - (double)min,
                    1000.0f / (sum / frames),
                      ((double)max-(double)min)/(1000.0f/(sum/frames)) );
  }

  ImGui::PushStyleColor ( ImGuiCol_PlotLines,
                             ImColor::HSV ( 0.31f - 0.31f *
                     std::min ( 1.0f, (max - min) / (2.0f * target_frametime) ),
                                             1.0f,   1.0f ) );

  const ImVec2 border_dims (
    std::max (500.0f, ImGui::GetContentRegionAvailWidth ()),
      font_size * 7.0f
  );

  float fX = ImGui::GetCursorPosX ();

  ///float fMax = std::max ( 99.0f,
  ///  *std::max_element ( std::begin (fLatencyHistory),
  ///                      std::end   (fLatencyHistory) )
  ///                      );

  ImGui::PushStyleColor (ImGuiCol_PlotHistogram, ImVec4 (.66f, .66f, .66f, .75f));
  ImGui::PlotHistogram ( SK_ImGui_Visible ? "###ControlPanel_LatencyHistogram" :
                                            "###Floating_LatencyHistogram",
                           latency.stats.History,
                             IM_ARRAYSIZE (latency.stats.History),
                               (SK_GetFramesDrawn () + 1) % 120,
                                 "",
                                   0,
                                     latency.stats.ScaleMs,
                                      border_dims );
  ImGui::PopStyleColor  ();

  ImGui::SameLine ();
  ImGui::SetCursorPosX (fX);

  ImGui::PlotLines ( SK_ImGui_Visible ? "###ControlPanel_FramePacing" :
                                        "###Floating_FramePacing",
                       SK_ImGui_Frames->getValues     ().data (),
      static_cast <int> (frames),
                           SK_ImGui_Frames->getOffset (),
                             szAvg,
                               -.1f,
                                 2.0f * target_frametime + 0.1f,
                                   border_dims );

  ImGui::PopStyleColor ();


  // Only toggle when clicking the graph and percentiles are off,
  //   to turn them back off, click the progress bars.
  if ((! SK_FramePercentiles->display) && ImGui::IsItemClicked ())
  {
    SK_FramePercentiles->toggleDisplay ();
  }


  //SK_RenderBackend& rb =
  //  SK_GetCurrentRenderBackend ();
  //
  //if (sk::NVAPI::nv_hardware && config.apis.NvAPI.gsync_status)
  //{
  //  if (rb.gsync_state.capable)
  //  {
  //    ImGui::SameLine ();
  //    ImGui::TextColored (ImColor::HSV (0.226537f, 1.0f, 0.36f), "G-Sync: ");
  //    ImGui::SameLine ();
  //
  //    if (rb.gsync_state.active)
  //    {
  //      ImGui::TextColored (ImColor::HSV (0.226537f, 1.0f, 0.45f), "Active");
  //    }
  //
  //
  //        else
  //    {
  //      ImGui::TextColored (ImColor::HSV (0.226537f, 0.75f, 0.27f), "Inactive");
  //    }
  //  }
  //}


  if (! SK_FramePercentiles->display_above)
        SK_ImGui_DrawFramePercentiles ();


#if 0
  DWM_TIMING_INFO dwmTiming        = {                      };
                  dwmTiming.cbSize = sizeof (DWM_TIMING_INFO);

  HRESULT hr =
    SK_DWM_GetCompositionTimingInfo (&dwmTiming);

  if ( SUCCEEDED (hr) )
  {
    ImGui::Text ( "Refresh Rate: %6.2f Hz",
                    static_cast <double> (dwmTiming.rateRefresh.uiNumerator) /
                    static_cast <double> (dwmTiming.rateRefresh.uiDenominator)
                );

    LONGLONG llPerfFreq =
      SK_GetPerfFreq ().QuadPart;

    LONGLONG qpcAtNextVBlank =
         dwmTiming.qpcVBlank;
    double   dMsToNextVBlank =
      1000.0 *
        ( static_cast <double> ( qpcAtNextVBlank - SK_QueryPerf ().QuadPart ) /
          static_cast <double> ( llPerfFreq ) );

    extern LONG64 __SK_VBlankLatency_QPCycles;

    static double uS =
      static_cast <double> ( llPerfFreq ) / ( 1000.0 * 1000.0 );

    ImGui::Text ( "ToNextVBlank:  %f ms", dMsToNextVBlank );
    ImGui::Text ( "VBlankLatency: %f us", __SK_VBlankLatency_QPCycles * uS );

#if 1
    static D3DKMT_ADAPTER_PERFDATA
                  adapterPerfData = { };

    static bool _thread = false;
    if (      ! _thread )
    {           _thread = true;
      SK_Thread_CreateEx ([] (LPVOID) -> DWORD
      {
        auto hDC =
           GetDC (NULL);

        while (! ReadAcquire (&__SK_DLL_Ending))
        {
          static DWORD
              dwLastUpdate = 0;
          if (dwLastUpdate < (timeGetTime () - 250))
          {   dwLastUpdate =  timeGetTime ();
            SK_D3DKMT_QueryAdapterPerfData (hDC,
                          &adapterPerfData );
          }
        }

        ReleaseDC (NULL, hDC);

        SK_Thread_CloseSelf ();

        return 0;
      }, L"[SK] D3DKMT HwInfo Thread" );
    }

    ImGui::Text ( "GPUTemp: %3.1fC (%u RPM)",
                                   static_cast <double> (adapterPerfData.Temperature) / 10.0,
                                                         adapterPerfData.FanRPM              );
    ImGui::Text ( "VRAM:    %7.2f MHz",
                                   static_cast <double> (adapterPerfData.MemoryFrequency) / 1000000.0
                                                                                             );
    ImGui::Text ( "Power:   %3.1f%%",
                                   static_cast <double> (adapterPerfData.Power)       / 10.0 );
#endif
  }
#endif
  //
  //  ImGui::Text ( "Composition Rate: %f",
  //                  static_cast <double> (dwmTiming.rateCompose.uiNumerator) /
  //                  static_cast <double> (dwmTiming.rateCompose.uiDenominator)
  //              );
  //
  //  ImGui::Text ( "DWM Refreshes (%llu), D3D Refreshes (%lu)", dwmTiming.cRefresh, dwmTiming.cDXRefresh );
  //  ImGui::Text ( "D3D Presents (%lu)",                                            dwmTiming.cDXPresent );
  //
  //  ImGui::Text ( "DWM Glitches (%llu)",                                           dwmTiming.cFramesLate );
  //  ImGui::Text ( "DWM Queue Length (%lu)",                                        dwmTiming.cFramesOutstanding );
  //  ImGui::Text ( "D3D Queue Length (%lu)", dwmTiming.cDXPresentSubmitted -
  //                                          dwmTiming.cDXPresentConfirmed );
  //}
}

void
SK_ImGui_DrawFramePercentiles (void)
{
  if (! SK_FramePercentiles->display)
    return;

  auto pLimiter =
    SK::Framerate::GetLimiter (
      SK_GetCurrentRenderBackend ().swapchain.p,
      false
    );

  if (! pLimiter)
    return;

  auto& snapshots =
    pLimiter->frame_history_snapshots;

  auto& percentile0 = SK_FramePercentiles->percentile0;
  auto& percentile1 = SK_FramePercentiles->percentile1;
  auto&        mean = SK_FramePercentiles->mean;

  bool& show_immediate =
    SK_FramePercentiles->display_most_recent;

  static constexpr LARGE_INTEGER all_samples = { 0ULL };

  auto frame_history =
    pLimiter->frame_history.getPtr ();

  long double data_timespan = ( show_immediate ?
            frame_history->calcDataTimespan () :
           snapshots.mean->calcDataTimespan () );

  ImGui::PushStyleColor (ImGuiCol_Text,           (unsigned int)ImColor (255, 255, 255));
  ImGui::PushStyleColor (ImGuiCol_FrameBg,        (unsigned int)ImColor ( 0.3f,  0.3f,  0.3f, 0.7f));
  ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, (unsigned int)ImColor ( 0.6f,  0.6f,  0.6f, 0.8f));
  ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  (unsigned int)ImColor ( 0.9f,  0.9f,  0.9f, 0.9f));

  ////if ( SK_FramePercentiles->percentile0.last_calculated !=
  ////     SK_GetFramesDrawn ()
  ////   )
  {
   //const long double   SAMPLE_SECONDS = 5.0L;

    percentile0.computeFPS (                             show_immediate ?
      frame_history->calcPercentile   (percentile0.cutoff, all_samples) :
        snapshots.percentile0->calcMean                   (all_samples) );

    percentile1.computeFPS (                             show_immediate ?
      frame_history->calcPercentile   (percentile1.cutoff, all_samples) :
        snapshots.percentile1->calcMean                   (all_samples) );

    mean.computeFPS ( show_immediate          ?
        frame_history->calcMean (all_samples) :
       snapshots.mean->calcMean (all_samples) );
  }

  float p0_ratio =
        percentile0.computed_fps / mean.computed_fps;
  float p1_ratio =
        percentile1.computed_fps / mean.computed_fps;

  if ( std::isnormal (percentile0.computed_fps) ||
        (! show_immediate)
     )
  {
    if (! std::isnormal (percentile0.computed_fps))
    {
      mean.computeFPS (
        frame_history->calcMean (all_samples)
      );
      percentile0.computeFPS (
        frame_history->calcPercentile (percentile0.cutoff, all_samples)
      );

      p0_ratio      =
        percentile0.computed_fps / mean.computed_fps;

      data_timespan =
        frame_history->calcDataTimespan ();
    }

    percentile0.has_data = true;

    static constexpr float  luminance  =   0.5f;
    static constexpr ImVec4 white_color (  1.0f,   1.0f,   1.0f, 1.0f);
    static constexpr ImVec4 label_color (0.595f, 0.595f, 0.595f, 1.0f);
    static constexpr ImVec4 click_color (0.685f, 0.685f, 0.685f, 1.0f);

    ImGui::BeginGroup ();

    if (data_timespan > 0)
    {
    //ImGui::Text ("Sample Size");

      ImGui::PushStyleColor  (ImGuiCol_Text, white_color);
           if (data_timespan > 60.0 * 60.0)  ImGui::Text ("%7.3f", data_timespan / (60.0 * 60.0));
      else if (data_timespan > 60.0)         ImGui::Text ("%6.2f", data_timespan / (60.0       ));
      else                                   ImGui::Text ("%5.1f", data_timespan);

      ImGui::SameLine ();

      ImGui::PushStyleColor  (ImGuiCol_Text, label_color);

           if (data_timespan > 60.0 * 60.0)  ImGui::Text ("Hours");
      else if (data_timespan > 60.0)         ImGui::Text ("Minutes");
      else                                   ImGui::Text ("Seconds");

      ImGui::PopStyleColor (2);
    }

    if (percentile1.has_data && data_timespan > 0)
    {
      ImGui::Spacing          ();
      ImGui::Spacing          ();
      ImGui::PushStyleColor  (ImGuiCol_Text, label_color);
      ImGui::TextUnformatted ("AvgFPS: "); ImGui::SameLine ();
      ImGui::PushStyleColor  (ImGuiCol_Text, white_color);
      ImGui::Text            ("%4.1f", mean.computed_fps);
      ImGui::PopStyleColor   (2);
    }

    ImGui::EndGroup   ();

    if (ImGui::IsItemHovered ( ))
    {
      ImGui::BeginTooltip    ( );
      ImGui::BeginGroup      ( );
      ImGui::PushStyleColor  (ImGuiCol_Text, click_color);
      ImGui::BulletText      ("Left-Click");
      ImGui::BulletText      ("Ctrl+Click");
      ImGui::PopStyleColor   ( );
      ImGui::EndGroup        ( );
      ImGui::SameLine        ( ); ImGui::Spacing ();
      ImGui::SameLine        ( ); ImGui::Spacing ();
      ImGui::SameLine        ( );
      ImGui::BeginGroup      ( );
      ImGui::PushStyleColor  (ImGuiCol_Text, white_color);
      ImGui::TextUnformatted ("Change Datasets (Long / Short)");
      ImGui::TextUnformatted ("Reset Long-term Statistics");
      ImGui::PopStyleColor   ( );
      ImGui::EndGroup        ( );
      ImGui::EndTooltip      ( );
    }

    static auto& io =
      ImGui::GetIO ();

    if (ImGui::IsItemClicked ())
    {
      if (! io.KeyCtrl)
      {
        show_immediate = (! show_immediate);
        SK_FramePercentiles->store_percentile_cfg ();
      }

      else snapshots.reset ();
    }

    unsigned int     p0_color  (
      ImColor::HSV ( p0_ratio * 0.278f,
              0.88f, luminance )
                   ),p1_color  (
      ImColor::HSV ( p1_ratio * 0.278f,
              0.88f, luminance )
                   );

    ImGui::SameLine       ( );
    ImGui::BeginGroup     ( );
    ImGui::PushStyleColor ( ImGuiCol_PlotHistogram,
                            p0_color                 );
    ImGui::ProgressBar    ( p0_ratio, ImVec2 ( -1.0f,
                                                0.0f ),
                            SK_FormatString (
                              "%3.1f%% Low FPS: %5.2f",
                                percentile0.cutoff,
                                percentile0.computed_fps
                                            ).c_str ()
                          );
    if ( data_timespan > 0                     &&
         std::isnormal ( percentile1.computed_fps )
       )
    { percentile1.has_data = true;

      ImGui::PushStyleColor ( ImGuiCol_PlotHistogram,
                              p1_color                 );
      ImGui::ProgressBar    ( p1_ratio, ImVec2 ( -1.0f,
                                                  0.0f ),
                              SK_FormatString (
                                "%3.1f%% Low FPS: %5.2f",
                                  percentile1.cutoff,
                                  percentile1.computed_fps
                                              ).c_str ()
                              );
      ImGui::PopStyleColor  (2);
    }

    else
    {
      percentile1.has_data = false;
      ImGui::PopStyleColor  (1);
    }

    ImGui::EndGroup         ( );

    if (SK_FramePercentiles->display)
    {
      if (ImGui::IsItemClicked ())
        SK_FramePercentiles->toggleDisplay ();
    }
  }

  else
  {
    percentile0.has_data = false;
    percentile1.has_data = false;
  }

  ImGui::PopStyleColor      (4);
}

float fExtraData = 0.0f;

class SKWG_FramePacing : public SK_Widget
{
public:
  SKWG_FramePacing (void) noexcept : SK_Widget ("FramePacing")
  {
    SK_ImGui_Widgets->frame_pacing = this;

    setResizable    (                false).setAutoFit      (true).setMovable (false).
    setDockingPoint (DockAnchor::SouthEast).setClickThrough (true).setVisible (false);

    SK_FramePercentiles->load_percentile_cfg ();
  };

  void load (iSK_INI* cfg) noexcept override
  {
    SK_Widget::load (cfg);

    SK_FramePercentiles->load_percentile_cfg ();
  }

  void save (iSK_INI* cfg) noexcept override
  {
    if (cfg == nullptr)
      return;

    SK_Widget::save (cfg);

    SK_FramePercentiles->store_percentile_cfg ();

    cfg->write (
      cfg->get_filename ()
    );
  }

  void run (void) noexcept override
  {
    static auto* cp =
      SK_GetCommandProcessor ();

    static volatile LONG init = 0;

    if (! InterlockedCompareExchange (&init, 1, 0))
    {
      //auto *display_framerate_percentiles_above =
      //  SK_CreateVar (
      //    SK_IVariable::Boolean,
      //      &SK_FramePercentiles->display_above,
      //        nullptr );
      //
      //SK_RunOnce (
      //  cp->AddVariable (
      //    "Framepacing.DisplayLessDynamicPercentiles",
      //      display_framerate_percentiles
      //  )
      //);


      auto *display_framerate_percentiles =
        SK_CreateVar (
          SK_IVariable::Boolean,
            &SK_FramePercentiles->display,
              nullptr );

      SK_RunOnce (
        cp->AddVariable (
          "Framepacing.DisplayPercentiles",
            display_framerate_percentiles
        ) );
    }

    if (ImGui::GetFont () == nullptr)
      return;

    const float font_size           =             ImGui::GetFont  ()->FontSize                        ;//* scale;
    const float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

    float extra_line_space = 0.0f;

    auto& percentile0 = SK_FramePercentiles->percentile0;
    auto& percentile1 = SK_FramePercentiles->percentile1;

    if (has_battery)            extra_line_space += 1.16f;
    if (SK_FramePercentiles->display)
    {
      if (percentile0.has_data) extra_line_space += 1.16f;
      if (percentile1.has_data) extra_line_space += 1.16f;
    }

    // If configuring ...
    if (state__ != 0) extra_line_space += (1.16f * 5.5f);

    ImVec2   new_size (font_size * 35, font_size_multiline * (5.44f + extra_line_space));
             new_size.y += fExtraData;
    setSize (new_size);

    if (isVisible ())// && state__ == 0)
    {
      ImGui::SetNextWindowSize (new_size, ImGuiCond_Always);
    }
  }

  void draw (void) noexcept override
  {
    if (ImGui::GetFont () == nullptr)
      return;

    static const auto& io =
      ImGui::GetIO ();

    static bool move = true;

    if (move)
    {
      ImGui::SetWindowPos (
        ImVec2 ( io.DisplaySize.x - getSize ().x,
                 io.DisplaySize.y - getSize ().y )
      );

      move = false;
    }

    ImGui::BeginGroup ();

    SK_ImGui_DrawGraph_FramePacing ();

    has_battery =
      SK_ImGui::BatteryMeter ();
    ImGui::EndGroup   ();

    auto* pLimiter = debug_limiter ?
      SK::Framerate::GetLimiter (
       SK_GetCurrentRenderBackend ( ).swapchain.p,
        false                   )  : nullptr;

    if (pLimiter != nullptr)
    {
      ImGui::BeginGroup ();

      SK::Framerate::Limiter::snapshot_s snapshot =
                            pLimiter->getSnapshot ();

      struct {
        DWORD  dwLastSnap =   0;
        double dLastMS    = 0.0;
        double dLastFPS   = 0.0;

        void update (SK::Framerate::Limiter::snapshot_s& snapshot)
        {
          static constexpr DWORD UPDATE_INTERVAL = 225UL;

          DWORD dwNow =
            SK::ControlPanel::current_time;

          if (dwLastSnap < dwNow - UPDATE_INTERVAL)
          {
            dLastMS    =          snapshot.effective_ms;
            dLastFPS   = 1000.0 / snapshot.effective_ms;
            dwLastSnap = dwNow;
          }
        }
      } static effective_snapshot;

      effective_snapshot.update (snapshot);

      ImGui::BeginGroup ();
      ImGui::Text ("MS:");
      ImGui::Text ("FPS:");
      ImGui::Text ("EffectiveMS:");
      ImGui::Text ("Clock Ticks per-Frame:");
      ImGui::Separator ();
      ImGui::Text ("Limiter Time=");
      ImGui::Text ("Limiter Start=");
      ImGui::Text ("Limiter Next=");
      ImGui::Text ("Limiter Last=");
      ImGui::Text ("Limiter Freq=");
      ImGui::Separator ();
      ImGui::Text ("Limited Frames:");
      ImGui::Separator  ();
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      ImGui::Text ("%f ms",  snapshot.ms);
      ImGui::Text ("%f fps", snapshot.fps);
      ImGui::Text ("%f ms        (%#06.1f fps)",
                   effective_snapshot.dLastMS,
                   effective_snapshot.dLastFPS);
      ImGui::Text ("%llu",   snapshot.ticks_per_frame);
      ImGui::Separator ();
      ImGui::Text ("%llu", ReadAcquire64 (&snapshot.time));
      ImGui::Text ("%llu", ReadAcquire64 (&snapshot.start));
      ImGui::Text ("%llu", ReadAcquire64 (&snapshot.next));
      ImGui::Text ("%llu", ReadAcquire64 (&snapshot.last));
      ImGui::Text ("%llu", ReadAcquire64 (&snapshot.freq));
      ImGui::Separator ();
      ImGui::Text ("%llu", ReadAcquire64 (&snapshot.frames));
      //ImGui::SameLine  ();
      //ImGui::ProgressBar (
      //  static_cast <float> (
      //    static_cast <double> (pLimiter->frames_of_fame.frames_measured.count ()) /
      //    static_cast <double> (ReadAcquire64 (&snapshot.frames))
      //  )
      //);
      ImGui::EndGroup  ();

      if (ImGui::Button ("Reset"))        *snapshot.pRestart     = true;
      ImGui::SameLine ();
      if (ImGui::Button ("Full Restart")) *snapshot.pFullRestart = true;
      ImGui::EndGroup ();

      SK_ComQIPtr <IDXGISwapChain1> pSwap1 (
          SK_GetCurrentRenderBackend ().swapchain.p
        );

      static UINT                  uiLastPresent = 0;
      static DXGI_FRAME_STATISTICS frameStats    = { };

      if (pLimiter->get_limit () > 0.0f)
      {
        if ( SUCCEEDED (
               pSwap1->GetLastPresentCount (&uiLastPresent)
                       )
           )
        {
          if ( SUCCEEDED (
                 pSwap1->GetFrameStatistics (&frameStats)
                         )
             )
          {
          }
        }
      }

      UINT uiLatency = ( uiLastPresent - frameStats.PresentCount );

      ImGui::Text ( "Present Latency: %i Frames", uiLatency );

      ImGui::Separator ();

      ImGui::Text ( "LastPresent: %i, PresentCount: %i", uiLastPresent, frameStats.PresentCount     );
      ImGui::Text ( "PresentRefreshCount: %i, SyncRefreshCount: %i",    frameStats.PresentRefreshCount,
                                                                        frameStats.SyncRefreshCount );

      float fUndershoot =
        pLimiter->get_undershoot ();

      if (ImGui::InputFloat ("Undershoot %", &fUndershoot, 0.1f, 0.1f))
      {
        pLimiter->set_undershoot (fUndershoot);
        pLimiter->reset          (true);
      }

      fExtraData = ImGui::GetItemRectSize ().y;
    } else
      fExtraData = 0.0f;
  }


  void config_base (void) noexcept override
  {
    SK_Widget::config_base ();

    auto *pLimiter =
      SK::Framerate::GetLimiter (
        SK_GetCurrentRenderBackend ().swapchain.p,
        false
      );

    if (pLimiter == nullptr)
      return;

    auto& snapshots =
      pLimiter->frame_history_snapshots;

    ImGui::Separator ();

    bool changed = false;
    bool display = SK_FramePercentiles->display;

        changed |= ImGui::Checkbox  ("Show Percentile Analysis", &display);
    if (changed)
    {
      SK_FramePercentiles->toggleDisplay ();
    }

    if (SK_FramePercentiles->display)
    {
      changed |= ImGui::Checkbox ("Draw Percentiles Above Graph",         &SK_FramePercentiles->display_above);
      changed |= ImGui::Checkbox ("Use Short-Term (~15-30 seconds) Data", &SK_FramePercentiles->display_most_recent);

      ImGui::Separator ();
      ImGui::TreePush  ();

      if ( ImGui::SliderFloat (
             "Percentile Class 0 Cutoff",
               &SK_FramePercentiles->percentile0.cutoff,
                 0.1f, 99.99f, "%3.1f%%" )
         )
      {
        snapshots.reset (); changed = true;
      }

      if ( ImGui::SliderFloat (
             "Percentile Class 1 Cutoff",
               &SK_FramePercentiles->percentile1.cutoff,
                 0.1f, 99.99f, "%3.1f%%" )
         )
      {
        snapshots.reset (); changed = true;
      }

      ImGui::TreePop ();
    }

    if (changed)
      SK_FramePercentiles->store_percentile_cfg ();

    ImGui::Checkbox ("Framerate Limiter Debug", &debug_limiter);
  }

  void OnConfig (ConfigEvent event) noexcept override
  {
    switch (event)
    {
      case SK_Widget::ConfigEvent::LoadComplete:
        break;

      case SK_Widget::ConfigEvent::SaveStart:
        break;
    }
  }

  //sk::ParameterInt* samples_max;
};

SK_LazyGlobal <SKWG_FramePacing> __frame_pacing__;

void SK_Widget_InitFramePacing (void)
{
  SK_RunOnce (__frame_pacing__.get ());
}