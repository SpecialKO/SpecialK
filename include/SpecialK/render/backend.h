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

#include <SpecialK/render/d3dkmt/d3dkmt.h>
#include <SpecialK/render/d3d12/d3d12_interfaces.h>
#include <SpecialK/render/screenshot.h>
#include <SpecialK/utility.h>

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

void __stdcall
SK_D3D11_ResetTexCache (void);

enum class PresentMode
{
  Unknown,
  Hardware_Legacy_Flip,
  Hardware_Legacy_Copy_To_Front_Buffer,
  /* Not detected:
  Hardware_Direct_Flip,
  */
  Hardware_Independent_Flip,
  Composed_Flip,
  Composed_Copy_GPU_GDI,
  Composed_Copy_CPU_GDI,
  Composed_Composition_Atlas,
  Hardware_Composed_Independent_Flip,
};

using SK_PresentMode =
         PresentMode;

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
      D3DKMT_NODE_PERFDATA    engine_3d        = { };
      ULONG64                 sampled_frame    =  0;
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

  struct {
    SK_PresentMode        mode                 = SK_PresentMode::Unknown;

    struct stats_s {
      double              latency              = 0.0L;
      double              display              = 0.0L;
      double              idle                 = 0.0L;
      double              cpu                  = 0.0L;
    } avg_stats;

    struct {
      SK_PresentMode      mode                 = SK_PresentMode::Unknown;

      struct {
        LONG64            qpcPresented         = 0LL;
        LONG64            qpcGPUFinished       = 0LL;
        LONG64            qpcScannedOut        = 0LL;
        LONG64            qpcPresentOverhead   = 0LL;
      } timestamps;

      stats_s             stats                = { 0.0L };
    } history [360];

    ULONG64               frames_presented     = 0ULL;
  } presentation;

  struct output_s {
    UINT                  idx                  =   0;
    RECT                  rect                 = { 0, 0,
                                                   0, 0 };
    int                   bpc                  =   8;
    SK_ColorSpace         gamut;
    DXGI_COLOR_SPACE_TYPE colorspace           = DXGI_COLOR_SPACE_CUSTOM;
    bool                  primary              = false;
    uint32_t              mpo_planes           =     0;
    SK_WDDM_CAPS          wddm_caps            = { };
                                               
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
      NV_MONITOR_CAPABILITIES
                          monitor_caps         = { };
      BOOL                vrr_enabled          =  -1;

      static output_s*    getDisplayFromId     (NvU32           display_id)     noexcept;
      static output_s*    getDisplayFromHandle (NvDisplayHandle display_handle) noexcept;
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

    struct audio_s {
      wchar_t             paired_device [128]  = L"{*}##No Preference";
    } audio;
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
    void reset             (void);
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
      // We may have cached textures preventing the destruction of the original
      //   D3D11 device associated with this SwapChain, so clear those now.
      SK_D3D11_ResetTexCache ();

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

    BOOL   capable      = FALSE;
    BOOL   active       = FALSE;
    struct {
      bool for_app      = false;
    } disabled;
    BOOL   maybe_active = FALSE; // If PresentMon isn't working...
    DWORD  last_checked = 0;
  } gsync_state;


  SK_LazyGlobal <SK_ScreenshotManager>
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
  void            updateWDDMCaps       (output_s *pOutput);
  bool            assignOutputFromHWND (HWND hWndContainer);
  bool            routeAudioForDisplay (output_s *pOutput, bool force_update = false);

  bool isReflexSupported  (void);
  bool setLatencyMarkerNV (NV_LATENCY_MARKER_TYPE    marker);
  bool getLatencyReportNV (NV_LATENCY_RESULT_PARAMS *pGetLatencyParams);
  void driverSleepNV      (int site);

  std::string parseEDIDForName      (uint8_t* edid, size_t length);
  POINT       parseEDIDForNativeRes (uint8_t* edid, size_t length);

  bool resetTemporaryDisplayChanges (void);

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

BOOL SK_DXGI_SupportsTearing    (void);
void SK_DXGI_SignalBudgetThread (void);


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

bool SK_Display_ApplyDesktopResolution (MONITORINFOEX& mi);

uint32_t
SK_Render_GetVulkanInteropSwapChainType (IUnknown *swapchain);

// Disables Vulkan layers (i.e. if using DXGI interop, prefer software hook the D3D11 SwapChain and not Vulkan)
void SK_Vulkan_DisableThirdPartyLayers (void); // Can only be called during application startup

// Returns true if d3d9.dll or d3d11.dll are DXVK, because some workarounds are required
bool __stdcall SK_DXVK_CheckForInterop (void);

// Move this to a more formal presentation manager
extern bool SK_GL_OnD3D11;
extern bool SK_GL_OnD3D11_Reset; // This one especially, this has a signal

#endif /* __SK__RENDER_BACKEND__H__ */