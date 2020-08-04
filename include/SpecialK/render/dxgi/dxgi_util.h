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

extern BOOL __stdcall SK_DXGI_IsFormatCompressed (DXGI_FORMAT fmt);
extern bool           SK_DXGI_IsFormatSRGB       (DXGI_FORMAT fmt);
extern DXGI_FORMAT    SK_DXGI_MakeFormatSRGB     (DXGI_FORMAT fmt);

extern std::wstring __stdcall SK_DXGI_FormatToStr   (DXGI_FORMAT fmt) noexcept;
extern INT          __stdcall SK_DXGI_BytesPerPixel (DXGI_FORMAT fmt);

BOOL __stdcall SK_DXGI_IsFormatFloat      (DXGI_FORMAT fmt) noexcept;
BOOL __stdcall SK_DXGI_IsFormatInteger    (DXGI_FORMAT fmt) noexcept;
BOOL __stdcall SK_DXGI_IsFormatNormalized (DXGI_FORMAT fmt) noexcept;
