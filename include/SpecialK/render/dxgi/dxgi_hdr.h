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

#ifndef __SK__DXGI_HDR_H__
#define __SK__DXGI_HDR_H__

#include <SpecialK/render/dxgi/dxgi_backend.h>

struct SK_DXGI_HDRControl
{
  struct
  {
    UINT                  BitsPerColor     = 0;
    DXGI_COLOR_SPACE_TYPE ColorSpace       = DXGI_COLOR_SPACE_RESERVED;

    FLOAT                 RedPrimary   [2] = { 0.0f, 0.0f };
    FLOAT                 GreenPrimary [2] = { 0.0f, 0.0f };
    FLOAT                 BluePrimary  [2] = { 0.0f, 0.0f };
    FLOAT                 WhitePoint   [2] = { 0.0f, 0.0f };

    FLOAT                 MinLuminance          = 0.0f,
                          MaxLuminance          = 0.0f,
                          MaxFullFrameLuminance = 0.0f;
  } devcaps;

  struct
  {
    UINT   MaxMasteringLuminance     = 0;
    UINT   MinMasteringLuminance     = 0;

    UINT16 MaxContentLightLevel      = 0;
    UINT16 MaxFrameAverageLightLevel = 0;

    LONG   _AdjustmentCount          = 0;
  } meta;

  struct
  {
    bool MaxMaster                 = false;
    bool MinMaster                 = false;
    bool MaxContentLightLevel      = false;
    bool MaxFrameAverageLightLevel = false;
  } overrides;
};


#pragma warning (push)
#pragma warning (disable: 4324)
#pragma pack    (push)
#pragma pack    (16)
// Why the hell did I used to think cbuffers were 4-byte
// aligned? They're 16-byte aligned (!!)
  struct HDR_COLORSPACE_PARAMS
  {
    uint32_t visualFunc       [2]  = { 0, 0 };

    float    hdrSaturation         =       1.0f;
    float    hdrLuminance_MaxAvg   = 300.0_Nits;
    float    hdrLuminance_MaxLocal = 750.0_Nits;
    float    hdrLuminance_Min      =   0.0_Nits; // lol
    float    sdrLuminance_White    =  80.0_Nits;
    float    hdrGamutExpansion     =     0.015f;
    float    currentTime           =       0.0f;
    float    sdrLuminance_NonStd   = 100.0_Nits;
    float    sdrContentEOTF        =      -2.2f;
    uint32_t uiToneMapper          =          0;
    float    pqBoostParams [4]     =        { };
  };

  //const int x = sizeof (HDR_COLORSPACE_PARAMS);

  struct HDR_LUMINANCE
  {
    // scRGB allows values > 1.0, sRGB (SDR) simply clamps them
    float luminance_scale [4]; // For HDR displays,    1.0 = 80 Nits
                               // For SDR displays, >= 1.0 = 80 Nits

    // (x,y): Main Scene  <Linear Scale, Gamma Exponent>
    //  z   : HUD          Linear Scale {Gamma is Implicitly 1/2.2}
    //  w   : Always 1.0
  };
#pragma pack    (pop)
#pragma warning (pop)


struct SK_HDR_RenderTargetManager
{
           bool   PromoteTo16Bit  = false;
  volatile LONG64 BytesAllocated  =   0LL;
  volatile ULONG  TargetsUpgraded =   0UL;
  volatile ULONG  CandidatesSeen  =   0UL;
};

extern SK_LazyGlobal <SK_HDR_RenderTargetManager> SK_HDR_RenderTargets_8bpc;
extern SK_LazyGlobal <SK_HDR_RenderTargetManager> SK_HDR_RenderTargets_10bpc;
extern SK_LazyGlobal <SK_HDR_RenderTargetManager> SK_HDR_RenderTargets_11bpc;

extern SK_LazyGlobal <SK_HDR_RenderTargetManager> SK_HDR_UnorderedViews_8bpc;
extern SK_LazyGlobal <SK_HDR_RenderTargetManager> SK_HDR_UnorderedViews_10bpc;
extern SK_LazyGlobal <SK_HDR_RenderTargetManager> SK_HDR_UnorderedViews_11bpc;

extern bool __SK_HDR_10BitSwap;
extern bool __SK_HDR_16BitSwap;
extern bool __SK_HDR_UserForced;

extern float __SK_HDR_Luma;
extern float __SK_HDR_Exp;
extern float __SK_HDR_Saturation;
extern float __SK_HDR_Gamut;
extern float __SK_HDR_Content_EOTF;
extern float __SK_HDR_user_sdr_Y;
extern float __SK_HDR_MiddleLuma;
extern int   __SK_HDR_Preset;
extern bool  __SK_HDR_FullRange;
extern bool  __SK_HDR_AdaptiveToneMap;

extern float __SK_HDR_UI_Luma;
extern float __SK_HDR_HorizCoverage;
extern float __SK_HDR_VertCoverage;

extern int   __SK_HDR_tonemap;
extern int   __SK_HDR_visualization;
extern float __SK_HDR_PaperWhite;
extern float __SK_HDR_user_sdr_Y;

enum {
  SK_HDR_TONEMAP_NONE              = 0,
  SK_HDR_TONEMAP_FILMIC            = 1, // N/A
  SK_HDR_TONEMAP_HDR10_PASSTHROUGH = 2,
  SK_HDR_TONEMAP_HDR10_FILMIC      = 3, // N/A
  SK_HDR_TONEMAP_RAW_IMAGE         = 255
};



SK_DXGI_HDRControl*
SK_HDR_GetControl (void);

bool
SK_D3D11_SanitizeFP16RenderTargets ( ID3D11DeviceContext *pDevCtx,
                                     UINT                 dev_idx );

void
SK_D3D11_EndFrameHDR (void);

void SK_HDR_DisableOverridesForGame (void);
void SK_HDR_SetOverridesForGame     (bool bScRGB, bool bHDR10);

extern double SK_D3D11_HDR_RuntimeMs;
extern bool   SK_D3D11_HDR_ZeroCopy;

#endif /*__SK__DXGI_HDR_H__*/