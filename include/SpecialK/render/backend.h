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


#include <SpecialK/com_util.h>
#include <SpecialK/core.h>

#include <Windows.h>

// For NVDX_ObjectHandle to emerge as a definition, include this
#include <d3d9.h>

#include <../depends/include/nvapi/nvapi_lite_common.h>
#include <../depends/include/nvapi/nvapi.h>

#include <comdef.h>
#include <atlbase.h>
#include <cstdint>

class SK_RenderBackend_V1
{
public:
  enum SK_RenderAPI api       = SK_RenderAPI::Reserved;
       wchar_t      name [16] = { };
};

enum
{
  SK_FRAMEBUFFER_FLAG_SRGB    = 0x01u,
  SK_FRAMEBUFFER_FLAG_FLOAT   = 0x02u,
  SK_FRAMEBUFFER_FLAG_RGB10A2 = 0x04u,
  SK_FRAMEBUFFER_FLAG_MSAA    = 0x08u,
  SK_FRAMEBUFFER_FLAG_HDR     = 0x10u,
};

enum reset_stage_e
{
  Initiate = 0x0u, // Fake device loss
  Respond  = 0x1u, // Fake device not reset
  Clear    = 0x2u  // Return status to normal
} extern trigger_reset;

enum mode_change_request_e
{
  Windowed   =    0U, // Switch from Fullscreen to Windowed
  Fullscreen =    1U, // Switch from Windowed to Fullscreen
  None       = 0xFFU
} extern request_mode_change;


struct sk_hwnd_cache_s
{
  HWND    hwnd             = HWND_DESKTOP;
  HWND    parent           = HWND_DESKTOP;

  struct devcaps_s
  {
    struct
    {
      int x, y, refresh;
    } res { 0, 0, 0 };

    DWORD last_checked     = 0UL;
  } devcaps;

  devcaps_s&
  getDevCaps (void);

  wchar_t class_name [128] = { };
  wchar_t title      [128] = { };

  struct {
    DWORD pid, tid;
  }       owner            = { 0, 0 };

  bool    unicode          = false;

  ULONG   last_changed     = 0UL;


  sk_hwnd_cache_s (HWND wnd);

  operator const HWND& (void) const noexcept { return hwnd; };
};

constexpr
float
operator"" _Nits ( long double whitepoint_scalar ) noexcept
{
  return
    static_cast <float> ( whitepoint_scalar / 80.0f );
}

#pragma pack(push)
#pragma pack(8)
struct SK_ColorSpace {
  float xr, yr,
        xg, yg,
        xb, yb,
        Xw, Yw, Zw;

  float minY, maxLocalY, maxY;
};


const wchar_t*
HDRModeToStr (NV_HDR_MODE mode);


interface IDirect3DSurface9;
interface IDXGISurface;

interface ID3D11DeviceContext;
interface ID3D11Texture2D;
interface ID3D11RenderTargetView;


class SK_RenderBackend_V2 : public SK_RenderBackend_V1
{
public:
   SK_RenderBackend_V2 (void);
  ~SK_RenderBackend_V2 (void);

  SK_ComPtr <IUnknown>    device               = nullptr;
  SK_ComPtr <IUnknown>    swapchain            = nullptr;
  // Different views of the same resource (API interop)
  struct {
    SK_ComPtr <IDirect3DSurface9>
                          d3d9                 = nullptr;
    SK_ComPtr <IDXGISurface>
                          dxgi                 = nullptr;
    NVDX_ObjectHandle     nvapi                = nullptr;
  } surface;
  bool                    fullscreen_exclusive = false;
  uint64_t                framebuffer_flags    = 0x00;
  int                     present_interval     = 0; // Present interval on last call to present
  float                   ui_luminance         = 325.0_Nits;
  bool                    ui_srgb              = true;
  bool                    hdr_capable          = false;
  bool                    driver_based_hdr     = false;
  SK_ColorSpace           display_gamut        = { 0.0f }; // EDID
  SK_ColorSpace           working_gamut        = { 0.0f }; // Metadata range

  struct scan_out_s {
    int                   bpc                  = 8;
    DXGI_COLOR_SPACE_TYPE colorspace_override  = DXGI_COLOR_SPACE_CUSTOM;
    DXGI_COLOR_SPACE_TYPE dxgi_colorspace      = DXGI_COLOR_SPACE_CUSTOM;
    DXGI_COLOR_SPACE_TYPE dwm_colorspace       = DXGI_COLOR_SPACE_CUSTOM;

    struct nvapi_desc_s {
      bool                     active               = false;

      struct {
        NvU32                  supports_YUV422_12bit : 1;
        NvU32                  supports_10b_12b_444  : 2;
      } color_support_hdr = { };

      struct {
        NvU32                  display_id      = std::numeric_limits <NvU32>::max ();
        NV_HDR_CAPABILITIES_V2 hdr_caps        = { };
        NV_HDR_COLOR_DATA_V2   hdr_data        = { };

        // TODO
        //std::vector  <
        //  std::tuple < NV_BPC,
        //               NV_COLOR_FORMAT,
        //               NV_DYNAMIC_RANGE >
        //                                >   color_modes =
        //                     { std::make_tuple ( NV_BPC_DEFAULT,
        //                                         NV_COLOR_FORMAT_DEFAULT,
        //                                         NV_DYNAMIC_RANGE_AUTO ); }
      } raw = { };

      NV_HDR_MODE         mode                 = NV_HDR_MODE_OFF;
      NV_COLOR_FORMAT     color_format         = NV_COLOR_FORMAT_DEFAULT;
      NV_DYNAMIC_RANGE    dynamic_range        = NV_DYNAMIC_RANGE_AUTO;
      NV_BPC              bpc                  = NV_BPC_DEFAULT;

      bool                isHDR10 (void) const { return ( mode == NV_HDR_MODE_UHDA ||
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
    } nvapi_hdr = { };

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

  wchar_t                 display_name [128]   = { };

  __inline void setHDRCapable (bool set) { hdr_capable = set; }
  __inline bool isHDRCapable  (void)
  {
    if (framebuffer_flags& SK_FRAMEBUFFER_FLAG_HDR)
      hdr_capable = true;

    if (driver_based_hdr)
      hdr_capable = true;

    return
      hdr_capable;
  }

  struct                 {
    struct sub_event     {
      LARGE_INTEGER time = LARGE_INTEGER { 0LL, 0LL };
    } submit,
      begin_overlays,
      begin_cegui,   end_cegui,
      begin_imgui,   end_imgui,
      begin_texmgmt, end_texmgmt;
  } present_staging;

  static volatile LONG    frames_drawn;

  struct window_registry_s
  {
    // Most input processing happens in this HWND's message pump
    sk_hwnd_cache_s       focus                = { HWND_DESKTOP };

    // Defines the client rectangle and not much else
    sk_hwnd_cache_s       device               = { HWND_DESKTOP };

    // This Unity engine is so terrible at window management that we need
    //   to flag the game and start disabling features!
    bool                  unity                = false;

    void setFocus  (HWND hWndFocus);
    void setDevice (HWND hWndRender);

    HWND getFocus  (void);
    HWND getDevice (void);
  } windows;

  // TODO: Proper abstraction
  struct d3d11_s
  {
    //MIDL_INTERFACE ("c0bfa96c-e089-44fb-8eaf-26f8796190da")
               ID3D11DeviceContext* immediate_ctx = nullptr;
  //SK_ComPtr <ID3D11DeviceContext> immediate_ctx = nullptr;
    SK_ComPtr <ID3D11DeviceContext> deferred_ctx  = nullptr;
 //SK_ComPtr <ID3D11On12Device>    wrapper_dev   = nullptr;

    //struct {
    //  UINT                               buffer_idx       = UINT_MAX;
    //  SK_ComPtr <ID3D11Texture2D>        backbuffer_tex2D = nullptr;
    //  SK_ComPtr <ID3D11RenderTargetView> backbuffer_rtv   = nullptr;
    //} interop;

  } d3d11;


  struct gsync_s
  {
    void update (void);

    BOOL                  capable      = FALSE;
    BOOL                  active       = FALSE;
    DWORD                 last_checked = 0;
  } gsync_state;


  //
  // Somewhat meaningless, given how most engines manage
  //   memory, textures and shaders...
  //
  //   This is the thread that handles SwapChain Presentation;
  //     nothing else can safely be inferred about this thread.
  //
  volatile DWORD           thread       =  0;
  SK_Thread_HybridSpinlock res_lock;


  bool canEnterFullscreen    (void);

  void requestFullscreenMode (bool override = false);
  void requestWindowedMode   (bool override = false);

  float getActiveRefreshRate (void);


  void releaseOwnedResources (void);
};
#pragma pack(pop)

using SK_RenderBackend = SK_RenderBackend_V2;

SK_RenderBackend&
__stdcall
SK_GetCurrentRenderBackend (void);

void
__stdcall
SK_InitRenderBackends (void);

__declspec (dllexport)
IUnknown* __stdcall SK_Render_GetDevice (void);

__declspec (dllexport)
IUnknown* __stdcall SK_Render_GetSwapChain (void);

void SK_BootD3D8   (void);
void SK_BootDDraw  (void);
void SK_BootGlide  (void);

void SK_BootD3D9   (void);
void SK_BootDXGI   (void);
void SK_BootOpenGL (void);
void SK_BootVulkan (void);


_Return_type_success_ (nullptr)
IUnknown*
SK_COM_ValidateRelease (IUnknown** ppObj);

const wchar_t*
SK_Render_GetAPIName (SK_RenderAPI api);


__forceinline
ULONG
__stdcall
SK_GetFramesDrawn (void)
{
  return
    ULONG { gsl::narrow_cast <ULONG> (ReadAcquire (&SK_RenderBackend::frames_drawn)) };
}

typedef enum
  _SK_SCANOUT_COLOR_FORMAT
  { SK_COLOR_FORMAT_RGB        = 0x1,
    SK_COLOR_FORMAT_YCbCr420   = 0x2,
    SK_COLOR_FORMAT_YCbCr422   = 0x4,
    SK_COLOR_FORMAT_YCbCr424   = 0x8,
  } SK_SCANOUT_COLOR_FORMAT;

typedef enum
  _SK_SCANOUT_COLORIMETRY
  { SK_COLORIMETRY_RGB         = 0x001, // PC-RGB (Duh)
    SK_COLORIMETRY_YCC601      = 0x002, // ITU601
    SK_COLORIMETRY_YCC709      = 0x004, // ITU709
    SK_COLORIMETRY_XVYCC601    = 0x008,
    SK_COLORIMETRY_XVYCC709    = 0x010,
    SK_COLORIMETRY_SYCC601     = 0x020,
    SK_COLORIMETRY_ADOBEYCC601 = 0x030,
    SK_COLORIMETRY_ADOBERGB    = 0x040,
    SK_COLORIMETRY_BT2020RGB   = 0x080,
    SK_COLORIMETRY_BT2020YCC   = 0x100,
    SK_COLORIMETRY_BT2020cYCC  = 0x200,

    SK_COLORIMETRY_DEFAULT     = 0x0FE,
    SK_COLORIMETRY_AUTO        = 0x0FF
  } SK_SCANOUT_COLORIMETRY;

typedef enum
  _SK_SCANOUT_DYNAMIC_RANGE
  { SK_DYNAMIC_RANGE_VESA      = 0x0,
    SK_DYNAMIC_RANGE_CEA       = 0x1,

    SK_DYNAMIC_RANGE_AUTO      = 0xFF
  } SK_SCANOUT_DYNAMIC_RANGE;

typedef enum
  _SK_SCANOUT_BPC
  { SK_BPC_DEFAULT             = 0x00,
    SK_BPC_6                   = 0x01,
    SK_BPC_8                   = 0x02,
    SK_BPC_10                  = 0x04,
    SK_BPC_12                  = 0x08,
    SK_BPC_16                  = 0x10,
  } SK_SCANOUT_BPC;

typedef enum
  _SK_SCANOUT_QUALITY_POLICY
  { SK_QUALITY_POLICY_USER         = 0x0,
    SK_QUALITY_POLICY_BEST_QUALITY = 0x1,
    SK_QUALITY_POLICY_DEFAULT      = SK_QUALITY_POLICY_BEST_QUALITY,
    SK_QUALITY_POLICY_UNKNOWN      = 0xFF,
  } SK_SCANOUT_QUALITY_POLICY;

class SK_SignalTransport
{
public:
  uint32_t                      version;

  SK_SCANOUT_COLOR_FORMAT   colorFormat;
  SK_SCANOUT_DYNAMIC_RANGE dynamicRange;
  SK_SCANOUT_COLORIMETRY    colorimetry;
  SK_SCANOUT_BPC                    bpc;

  uint32_t isDp                             : 1;
  uint32_t isInternalDp                     : 1;
  uint32_t isColorCtrlSupported             : 1;
  uint32_t is6BPCSupported                  : 1;
  uint32_t is8BPCSupported                  : 1;
  uint32_t is10BPCSupported                 : 1;
  uint32_t is12BPCSupported                 : 1;
  uint32_t is16BPCSupported                 : 1;
  uint32_t isYCrCb420Supported              : 1;
  uint32_t isYCrCb422Supported              : 1;
  uint32_t isYCrCb444Supported              : 1;
  uint32_t isRgb444SupportedOnCurrentMode   : 1;
  uint32_t isYCbCr444SupportedOnCurrentMode : 1;
  uint32_t isYCbCr422SupportedOnCurrentMode : 1;
  uint32_t isYCbCr420SupportedOnCurrentMode : 1;
  uint32_t is6BPCSupportedOnCurrentMode     : 1;
  uint32_t is8BPCSupportedOnCurrentMode     : 1;
  uint32_t is10BPCSupportedOnCurrentMode    : 1;
  uint32_t is12BPCSupportedOnCurrentMode    : 1;
  uint32_t is16BPCSupportedOnCurrentMode    : 1;
  uint32_t isMonxvYCC601Capable             : 1;
  uint32_t isMonxvYCC709Capable             : 1;
  uint32_t isMonsYCC601Capable              : 1;
  uint32_t isMonAdobeYCC601Capable          : 1;
  uint32_t isMonAdobeRGBCapable             : 1;
  uint32_t isMonBT2020RGBCapable            : 1;
  uint32_t isMonBT2020YCCCapable            : 1;
  uint32_t isMonBT2020cYCCCapable           : 1;
  uint32_t isUInt32_tA28ByteTypeYet         : 4;
};

typedef enum
  _SK_DP_LINK_RATE
  {
    SK_DP_LINK_1_62GBPS =    6,
    SK_DP_LINK_2_70GBPS = 0x0A,
    SK_DP_LINK_5_40GBPS = 0x14,
    SK_DP_LINK_8_10GBPS = 0x1E
  } SK_DP_LINK_RATE;

typedef enum
  _SK_DP_LANE_COUNT
  {
    SK_DP_1_LANE = 1,
    SK_DP_2_LANE = 2,
    SK_DP_4_LANE = 4,
  } SK_DP_LANE_COUNT;

class SK_Signal_DisplayPort : public SK_SignalTransport
{
public:
  // DisplayPort Configuration Data v. (?)
  uint32_t cfg_data_ver;

  struct {
    NV_DP_LINK_RATE max;
    NV_DP_LINK_RATE cur;
  } LinkRate;

  struct {
    NV_DP_LANE_COUNT max;
    NV_DP_LANE_COUNT cur;
  } LaneCount;
};

class SK_Signal_HDMI : public SK_SignalTransport
{
public:
  uint32_t isGpuHDMICapable        :  1;
  uint32_t isMonUnderscanCapable   :  1;
  uint32_t isMonBasicAudioCapable  :  1;
  uint32_t isMonYCbCr444Capable    :  1;
  uint32_t isMonYCbCr422Capable    :  1;
  uint32_t isMonxvYCC601Capable    :  1;
  uint32_t isMonxvYCC709Capable    :  1;
  uint32_t isMonHDMI               :  1;
  uint32_t isMonsYCC601Capable     :  1;
  uint32_t isMonAdobeYCC601Capable :  1;
  uint32_t isMonAdobeRGBCapable    :  1;
  uint32_t isAnyoneMissing21Bits   : 21; //!< Padding, because HDMI is uninteresting.

  uint32_t EDID_861_Extn_rev;
 };


typedef struct
  _SK_MONITOR_CAPS_VCDB
  {
    uint8_t quantizationRangeYcc         : 1;
    uint8_t quantizationRangeRgb         : 1;
    uint8_t scanInfoPreferredVideoFormat : 2;
    uint8_t scanInfoITVideoFormats       : 2;
    uint8_t scanInfoCEVideoFormats       : 2;
  } SK_MONITOR_CAPS_VCDB;


///! See NvAPI_DISP_GetMonitorCapabilities().
typedef struct
  _SK_MONITOR_CAPS_VSDB
  {
    // byte 1
    uint8_t sourcePhysicalAddressB         : 4; //!< Byte  1
    uint8_t sourcePhysicalAddressA         : 4; //!< Byte  1
    // byte 2
    uint8_t sourcePhysicalAddressD         : 4; //!< Byte  2
    uint8_t sourcePhysicalAddressC         : 4; //!< Byte  2
    // byte 3
    uint8_t supportDualDviOperation        : 1; //!< Byte  3
    uint8_t reserved6                      : 2; //!< Byte  3
    uint8_t supportDeepColorYCbCr444       : 1; //!< Byte  3
    uint8_t supportDeepColor30bits         : 1; //!< Byte  3
    uint8_t supportDeepColor36bits         : 1; //!< Byte  3
    uint8_t supportDeepColor48bits         : 1; //!< Byte  3
    uint8_t supportAI                      : 1; //!< Byte  3
    // byte 4
    uint8_t maxTmdsClock;                       //!< Byte  4
    // byte 5
    uint8_t cnc0SupportGraphicsTextContent : 1; //!< Byte  5
    uint8_t cnc1SupportPhotoContent        : 1; //!< Byte  5
    uint8_t cnc2SupportCinemaContent       : 1; //!< Byte  5
    uint8_t cnc3SupportGameContent         : 1; //!< Byte  5
    uint8_t reserved8                      : 1; //!< Byte  5
    uint8_t hasVicEntries                  : 1; //!< Byte  5
    uint8_t hasInterlacedLatencyField      : 1; //!< Byte  5
    uint8_t hasLatencyField                : 1; //!< Byte  5
    // byte 6
    uint8_t videoLatency;                       //!< Byte  6
    // byte 7
    uint8_t audioLatency;                       //!< Byte  7
    // byte 8
    uint8_t interlacedVideoLatency;             //!< Byte  8
    // byte 9
    uint8_t interlacedAudioLatency;             //!< Byte  9
    // byte 10
    uint8_t reserved13                     : 7; //!< Byte 10
    uint8_t has3dEntries                   : 1; //!< Byte 10
    // byte 11
    uint8_t hdmi3dLength                   : 5; //!< Byte 11
    uint8_t hdmiVicLength                  : 3; //!< Byte 11

    // Remaining bytes
    uint8_t hdmi_vic [ 7];  ///!< Keeping maximum length for 3 bits
    uint8_t hdmi_3d  [31];  ///!< Keeping maximum length for 5 bits
  } SK_MONITOR_CAPS_VSDB;


//! See NvAPI_DISP_GetMonitorCapabilities().
typedef struct
  _SK_MONITOR_CAPABILITIES
  {
  //uint32_t version;
  //uint16_t size;
  //
    uint32_t infoType;
    uint32_t connectorType;    //!< Out:      [ VGA, TV, DVI, HDMI, DP ]
    uint8_t  bIsValidInfo : 1; //!< Boolean: Returns invalid if requested
                               //!>            info is not present such as
                               //!>               VCDB not present
    union
    {
      SK_MONITOR_CAPS_VSDB vsdb;
      SK_MONITOR_CAPS_VCDB vcdb;
    } data;
  } SK_MONITOR_CAPABILITIES;


void SK_D3D_SetupShaderCompiler   (void);
void SK_Display_DisableDPIScaling (void);

#endif /* __SK__RENDER_BACKEND__H__ */