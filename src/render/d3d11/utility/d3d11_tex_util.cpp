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

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"  D3D 11  "

#include <SpecialK/render/d3d11/d3d11_core.h>
#include <SpecialK/render/dxgi/dxgi_util.h>
#include <SpecialK/render/d3d11/utility/d3d11_texture.h>

uint32_t
safe_crc32c_ex (uint32_t seed, const void* pData, size_t size, bool* failed)
{
  if (failed != nullptr)
     *failed = false;

  // Current limit == 2 GiB
  if (size > (1024ULL * 1024ULL * 1024ULL * 2) || pData == nullptr)
  {
    SK_LOG0 ( ( L"Hash Fail: Data is too large (%zu) or invalid pointer (%p).",
                  size, pData ),
                L"DX11TexMgr");

    if (failed != nullptr)
       *failed = true;

    return seed;
  }

  uint32_t ret = 0x0;

  __try {
    ret =
      crc32c (seed, pData, size);
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    if (failed != nullptr)
       *failed = true;
  }

  return ret;
}



__declspec (noinline)
uint32_t
__cdecl
crc32_tex (  _In_      const D3D11_TEXTURE2D_DESC   *__restrict pDesc,
             _In_      const D3D11_SUBRESOURCE_DATA *__restrict pInitialData,
             _Out_opt_       size_t                 *__restrict pSize,
             _Out_opt_       uint32_t               *__restrict pLOD0_CRC32,
             _In_opt_        bool                               bAllLODs /*= true */)
{
  if (pLOD0_CRC32 != nullptr)
    *pLOD0_CRC32 = 0ui32;

  // Eh?  Why did we even call this if there's no freaking data?!
  if (pInitialData == nullptr)
  {
    assert (pInitialData != nullptr && "FIXME: Dumbass");
    return 0ui32;
  }

  if (pDesc->MiscFlags > D3D11_RESOURCE_MISC_TEXTURECUBE)
  {
    SK_LOG0 ( (L">> Hashing texture with unexpected MiscFlags: "
                   L"0x%04X",
                     pDesc->MiscFlags), L" Tex Hash " );
    return 0;
  }

  if ((pDesc->MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE)
                       == D3D11_RESOURCE_MISC_TEXTURECUBE)
  {
    //SK_LOG0 ( ( L">>Neat! A cubemap!  [%lux%lu - Array: %lu :: pInitialData: %ph ]",
    //              pDesc->Width, pDesc->Height, pDesc->ArraySize, pInitialData ),
    //           L"DX11TexMgr" );
    return 0;
  }

        uint32_t checksum   = 0;
  const bool     compressed =
    SK_DXGI_IsFormatCompressed (pDesc->Format);

  int bpp = 0;

  switch (pDesc->Format)
  {
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
      bpp = 0;
      break;

    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
      bpp = 1;
      break;
  }

  unsigned int width      = pDesc->Width;
  unsigned int height     = pDesc->Height;
  unsigned int mip_levels = pDesc->MipLevels;

  if (mip_levels == 0)
    mip_levels = CalcMipmapLODs (width, height);

        size_t size   = 0UL;

  if (compressed)
  {
    for (unsigned int i = 0; i < mip_levels; i++)
    {
      if (i > 0 && (!bAllLODs))
        break;

      auto* pData    =
        static_cast  <char *> (
          const_cast <void *> (pInitialData [i].pSysMem)
        );

      const UINT stride = bpp == 0 ?
             std::max (1UL, ((width + 3UL) / 4UL) ) *  8UL:
             std::max (1UL, ((width + 3UL) / 4UL) ) * 16UL;

      // Fast path:  Data is tightly packed and alignment agrees with
      //               convention...
      if (stride == pInitialData [i].SysMemPitch)
      {
        const unsigned int lod_size =
          ( stride * ( height / 4 +
                       height % 4 ) );

        bool failed = false;

        checksum =
          safe_crc32c_ex (checksum, (const uint8_t *)pData, lod_size, &failed);

        if (failed)
        {
          size +=
            ( static_cast <size_t> (stride) * ( static_cast <size_t> (height) /
                                                static_cast <size_t> (4)    ) +
                                              ( static_cast <size_t> (height) %
                                                static_cast <size_t> (4)      ) );

          SK_LOG0 ( ( L"Access Violation while Hashing Texture: %x", checksum ),
                      L" Tex Hash " );

          return 0;
        }

        size += lod_size;
      }

      else
      {
        // We are running through the compressed image block-by-block,
        //  the lines we are "scanning" actually represent 4 rows of image data.
        for (unsigned int j = 0; j < height; j += 4)
        {
          bool failed = false;

          const uint32_t row_checksum =
            safe_crc32c_ex (checksum, (const uint8_t *)(pData), stride, &failed);

          if (row_checksum != 0x00)
            checksum = row_checksum;

          if (failed)
          {
            SK_D3D11_DescribeTexFailure (pDesc);

            return 0;
          }


          // Respect the engine's reported stride, while making sure to
          //   only read the 4x4 blocks that have legal data. Any padding
          //     the engine uses will not be included in our hash since the
          //       values are undefined.
          pData += pInitialData [i].SysMemPitch;
          size  += stride;
        }
      }

      if (i == 0 && pLOD0_CRC32 != nullptr)
        *pLOD0_CRC32 = checksum;

      if (width  > 1) width  >>= 1UL;
      if (height > 1) height >>= 1UL;
    }
  }


  else
  {
    for (unsigned int i = 0; i < mip_levels; i++)
    {
      if (i > 0 && (! bAllLODs))
        break;

      auto* pData      =
        static_const_cast <char *, void *> (pInitialData [i].pSysMem);

      const UINT  scanlength =
        SK_DXGI_BytesPerPixel (pDesc->Format) * width;

      // Fast path:  Data is tightly packed and alignment agrees with
      //               convention...
      if (scanlength == pInitialData [i].SysMemPitch)
      {
        const unsigned int lod_size =
          (scanlength * height);

        bool failed = false;

        checksum = safe_crc32c_ex ( checksum,
                                     static_cast <const uint8_t *> (
                                       static_const_cast <const void *, const char *> (pData)
                                     ),
                                    lod_size, &failed );
        size    += lod_size;

        if (failed)
          return 0;
      }

      else// if (pDesc->Usage != D3D11_USAGE_STAGING)
      {
        for (unsigned int j = 0; j < height; j++)
        {
          bool failed = false;

          const uint32_t row_checksum =
            safe_crc32c_ex (checksum, (const uint8_t *)(pData), scanlength, &failed);

          if (row_checksum != 0x00)
            checksum = row_checksum;

          if (failed)
            return 0;

          pData += pInitialData [i].SysMemPitch;
          size  += scanlength;
        }
      }

      if (i == 0 && pLOD0_CRC32 != nullptr)
        *pLOD0_CRC32 = checksum;

      if (width  > 1) width  >>= 1UL;
      if (height > 1) height >>= 1UL;
    }
  }

  if (pSize != nullptr)
     *pSize  = size;

  return checksum;
}

//
// OLD, BUGGY Algorithm... must remain here for compatibility with UnX :(
//
__declspec (noinline)
uint32_t
__cdecl
crc32_ffx (  _In_      const D3D11_TEXTURE2D_DESC   *__restrict pDesc,
             _In_      const D3D11_SUBRESOURCE_DATA *__restrict pInitialData,
             _Out_opt_       size_t                 *__restrict pSize )
{
  uint32_t checksum = 0;

  if (pInitialData == nullptr)
    return checksum;

  const bool compressed =
    SK_DXGI_IsFormatCompressed (pDesc->Format) == TRUE;

  const int block_size = pDesc->Format == DXGI_FORMAT_BC1_UNORM ? 8 : 16;
  const int height     = pDesc->Height;

  size_t size = 0;

  for (unsigned int i = 0; i < pDesc->MipLevels; i++)
  {
    if (compressed)
    {
      size += (pInitialData [i].SysMemPitch / block_size) * static_cast <size_t> (height >> i);

      checksum =
        crc32 ( checksum, static_cast <const char *>(pInitialData [i].pSysMem),
                  (pInitialData [i].SysMemPitch / block_size) * static_cast <size_t> (height >> i) );
    }

    else
    {
      size += (pInitialData [i].SysMemPitch) * static_cast <size_t> (height >> i);

      checksum =
        crc32 ( checksum, static_cast <const char *>(pInitialData [i].pSysMem),
                  (pInitialData [i].SysMemPitch) * static_cast <size_t> (height >> i) );
    }
  }

  if (pSize != nullptr)
    *pSize = size;

  return checksum;
}

size_t
__stdcall
SK_D3D11_ComputeTextureSize (const D3D11_TEXTURE2D_DESC* pDesc)
{
  size_t       size       = 0;
  const bool   compressed =
    SK_DXGI_IsFormatCompressed (pDesc->Format) == TRUE;

  if (! compressed)
  {
    for (UINT i = 0; i < pDesc->MipLevels; i++)
    {
      size += static_cast <size_t> (SK_DXGI_BytesPerPixel (pDesc->Format)) *
              static_cast <size_t> (std::max (1U, pDesc->Width  >> i))     *
              static_cast <size_t> (std::max (1U, pDesc->Height >> i));
    }
  }

  else
  {
    const int bpp = ( (pDesc->Format >= DXGI_FORMAT_BC1_TYPELESS &&
                       pDesc->Format <= DXGI_FORMAT_BC1_UNORM_SRGB) ||
                      (pDesc->Format >= DXGI_FORMAT_BC4_TYPELESS &&
                       pDesc->Format <= DXGI_FORMAT_BC4_SNORM) ) ? 0 : 1;

    // Block-Compressed Formats have minimum 4x4 pixel alignment, so
    //   computing size is non-trivial.
    for (UINT i = 0; i < pDesc->MipLevels; i++)
    {
      const size_t stride = (bpp == 0) ?
               static_cast <size_t> (std::max (1UL, (std::max (1U, (pDesc->Width >> i)) + 3UL) / 4UL)) * 8UL :
               static_cast <size_t> (std::max (1UL, (std::max (1U, (pDesc->Width >> i)) + 3UL) / 4UL)) * 16UL;

      const size_t lod_size =
        gsl::narrow_cast <size_t> (stride) * (gsl::narrow_cast <size_t> (pDesc->Height >> i) / 4 +
                                              gsl::narrow_cast <size_t> (pDesc->Height >> i) % 4);

      size += lod_size;
    }
  }

  return size;
}