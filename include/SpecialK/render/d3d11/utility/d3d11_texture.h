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

#include <SpecialK/render/d3d11/d3d11_core.h>

static void
SK_D3D11_DescribeTexFailure (const D3D11_TEXTURE2D_DESC   *__restrict pDesc)
{
  SK_LOG0 ( (L"Texture Hash Fail: Access Violation [Tex: %lux%lu : %lu LODs :: Array Size %lu"
             L"Format: %s, Usage: %s, CPUAccess: %x, BindFlags: %s, Misc Flags: %s]",
                           pDesc->Width,  pDesc->Height,
                       pDesc->MipLevels,  pDesc->ArraySize,
              SK_DXGI_FormatToStr        (pDesc->Format).c_str (),
              SK_D3D11_DescribeUsage     (pDesc->Usage),
                                          pDesc->CPUAccessFlags,
              SK_D3D11_DescribeBindFlags (pDesc->BindFlags).c_str (),
              SK_D3D11_DescribeMiscFlags (
  static_cast <D3D11_RESOURCE_MISC_FLAG> (pDesc->MiscFlags)).c_str ()),
              L"DX11TexMgr" );
}

uint32_t
safe_crc32c_ex (uint32_t seed, const void* pData, size_t size, bool* failed);

uint32_t
__cdecl
crc32_tex (_In_      const D3D11_TEXTURE2D_DESC   *__restrict pDesc,
           _In_      const D3D11_SUBRESOURCE_DATA *__restrict pInitialData,
           _Out_opt_       size_t                 *__restrict pSize,
           _Out_opt_       uint32_t               *__restrict pLOD0_CRC32,
           _In_opt_        bool                               bAllLODs = true );

uint32_t
__cdecl
crc32_ffx (  _In_      const D3D11_TEXTURE2D_DESC   *__restrict pDesc,
             _In_      const D3D11_SUBRESOURCE_DATA *__restrict pInitialData,
             _Out_opt_       size_t                 *__restrict pSize );

size_t __stdcall SK_D3D11_ComputeTextureSize (const D3D11_TEXTURE2D_DESC* pDesc);

constexpr auto CalcMipmapLODs = [](UINT width, UINT height) ->
UINT
{
  UINT lods = 1U;

  while ((width > 1U) || (height > 1U))
  {
    if (width  > 1U) width  >>= 1UL;
    if (height > 1U) height >>= 1UL;

    ++lods;
  }

  return lods;
};