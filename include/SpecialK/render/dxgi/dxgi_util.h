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

#pragma once

#include <SpecialK/render/dxgi/dxgi_interfaces.h>

extern DXGI_FORMAT SK_DXGI_MakeTypelessFormat (DXGI_FORMAT typeless);
extern DXGI_FORMAT SK_DXGI_MakeTypedFormat    (DXGI_FORMAT typeless);
extern bool        SK_DXGI_IsDataSizeClassOf  (DXGI_FORMAT typeless, DXGI_FORMAT typed, int recursion = 0);

constexpr BOOL SK_DXGI_IsFormatBC6Or7 (DXGI_FORMAT fmt)
{
  return ( fmt >= DXGI_FORMAT_BC6H_TYPELESS &&
          fmt <= DXGI_FORMAT_BC7_UNORM_SRGB   );
}

extern bool __stdcall SK_DXGI_IsFormatCompressed (DXGI_FORMAT fmt) noexcept;
extern bool           SK_DXGI_IsFormatSRGB       (DXGI_FORMAT fmt);
extern DXGI_FORMAT    SK_DXGI_MakeFormatSRGB     (DXGI_FORMAT fmt);

extern std::string_view __stdcall SK_DXGI_FormatToStr   (DXGI_FORMAT fmt) noexcept;
extern INT              __stdcall SK_DXGI_BytesPerPixel (DXGI_FORMAT fmt);

// NOTE: These are very incomplete, D3D12X has a more complete list and it would be a good idea
//         to switch to that in the future...
bool SK_DXGI_IsFormatCastable (DXGI_FORMAT inFormat,
                               DXGI_FORMAT outFormat);
bool SK_DXGI_IsUAVFormatCastable (DXGI_FORMAT from,
                                  DXGI_FORMAT to);

bool __stdcall SK_DXGI_IsFormatFloat      (DXGI_FORMAT fmt) noexcept;
bool __stdcall SK_DXGI_IsFormatInteger    (DXGI_FORMAT fmt) noexcept;
bool __stdcall SK_DXGI_IsFormatNormalized (DXGI_FORMAT fmt) noexcept;

FARPROC
SK_GetProcAddressD3D11 (const char* lpProcName);

// Check if SK is responsible for this resource having a different
//   format than the underlying software initially requested/expects
HRESULT
SK_D3D11_CheckResourceFormatManipulation ( ID3D11Resource* pRes,
                                           DXGI_FORMAT     expected = DXGI_FORMAT_UNKNOWN );

void
SK_D3D11_FlagResourceFormatManipulated ( ID3D11Resource* pRes,
                                         DXGI_FORMAT     original );

bool
SK_D3D11_IsDirectCopyCompatible ( DXGI_FORMAT src,
                                  DXGI_FORMAT dst );

bool SK_D3D11_AreTexturesDirectCopyable ( D3D11_TEXTURE2D_DESC *pSrc,
                                          D3D11_TEXTURE2D_DESC *pDst );

bool SK_D3D11_BltCopySurface ( ID3D11Texture2D *pSrcTex,
                               ID3D11Texture2D *pDstTex,
                         const D3D11_BOX       *pSrcBox        = nullptr,
                               UINT             SrcSubresource = 0,
                               UINT             DstSubresource = 0,
                               UINT             DstX           = 0,
                               UINT             DstY           = 0
                             //UINT             DstZ           = 0 // (Unneeded)
);

bool SK_D3D11_EnsureMatchingDevices ( ID3D11Device *pDevice0,
                                      ID3D11Device *pDevice1 );

bool SK_D3D11_EnsureMatchingDevices ( ID3D11DeviceChild *pDeviceChild,
                                      ID3D11Device      *pDevice );

bool SK_D3D11_EnsureMatchingDevices ( IDXGISwapChain *pSwapChain,
                                      ID3D11Device   *pDevice );

bool SK_DXGI_IsSwapChainReal  (const DXGI_SWAP_CHAIN_DESC   &desc)        noexcept;
bool SK_DXGI_IsSwapChainReal  (      IDXGISwapChain         *pSwapChain)  noexcept;
bool SK_DXGI_IsSwapChainReal1 (const DXGI_SWAP_CHAIN_DESC1  &desc,
                                     HWND                   OutputWindow) noexcept;

DXGI_FORMAT
SK_DXGI_PickHDRFormat ( DXGI_FORMAT fmt_orig, BOOL bWindowed  = FALSE,
                                              BOOL bFlipModel = FALSE );

HRESULT
SK_DXGI_GetPrivateData ( IDXGIObject *pObject,
                   const GUID        &kName,
                         UINT        uiMaxBytes,
                         void        *pPrivateData ) noexcept;

HRESULT
SK_DXGI_SetPrivateData ( IDXGIObject *pObject,
                      const GUID     &kName,
                            UINT     uiNumBytes,
                            void     *pPrivateData ) noexcept;


template <typename _T>
HRESULT
SK_DXGI_GetPrivateData ( IDXGIObject *pObject,
                         _T          *pPrivateData ) noexcept;

template <typename _T>
HRESULT
SK_DXGI_SetPrivateData ( IDXGIObject *pObject,
                            _T       *pPrivateData ) noexcept;

void        SK_DXGI_SignalBudgetThread (void);
bool WINAPI SK_DXGI_IsTrackingBudget   (void) noexcept;

bool SK_DXGI_IsFakeFullscreen (IUnknown *pSwapChain) noexcept;

void SK_HDR_ReleaseResources       (void);
void SK_DXGI_ReleaseSRGBLinearizer (void);

extern volatile LONG lResetD3D12;
extern volatile LONG lResetD3D11;
extern          bool bAlwaysAllowFullscreen;

extern void SK_COMPAT_FixUpFullscreen_DXGI (bool Fullscreen);