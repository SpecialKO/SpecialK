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

extern bool __stdcall SK_DXGI_IsFormatCompressed (DXGI_FORMAT fmt);
extern bool           SK_DXGI_IsFormatSRGB       (DXGI_FORMAT fmt);
extern DXGI_FORMAT    SK_DXGI_MakeFormatSRGB     (DXGI_FORMAT fmt);

extern std::string_view __stdcall SK_DXGI_FormatToStr   (DXGI_FORMAT fmt) noexcept;
extern INT              __stdcall SK_DXGI_BytesPerPixel (DXGI_FORMAT fmt);

bool __stdcall SK_DXGI_IsFormatFloat      (DXGI_FORMAT fmt) noexcept;
bool __stdcall SK_DXGI_IsFormatInteger    (DXGI_FORMAT fmt) noexcept;
bool __stdcall SK_DXGI_IsFormatNormalized (DXGI_FORMAT fmt) noexcept;

// Check if SK is responsible for this resource having a different
//   format than the underlying software initially requested/expects
HRESULT
SK_D3D11_CheckResourceFormatManipulation ( ID3D11Resource* pRes,
                                           DXGI_FORMAT     expected = DXGI_FORMAT_UNKNOWN );

void
SK_D3D11_FlagResourceFormatManipulated ( ID3D11Resource* pRes,
                                         DXGI_FORMAT     original );

bool
SK_D3D11_IsDirectCopyCompatible (DXGI_FORMAT src, DXGI_FORMAT dst);

bool SK_D3D11_AreTexturesDirectCopyable (D3D11_TEXTURE2D_DESC *pSrc, D3D11_TEXTURE2D_DESC *pDst);

bool SK_D3D11_BltCopySurface (ID3D11Texture2D* pSrcTex, ID3D11Texture2D* pDstTex, const D3D11_BOX *pSrcBox = nullptr);

bool SK_D3D11_EnsureMatchingDevices (ID3D11DeviceChild *pDeviceChild, ID3D11Device *pDevice);
bool SK_D3D11_EnsureMatchingDevices (IDXGISwapChain      *pSwapChain, ID3D11Device *pDevice);