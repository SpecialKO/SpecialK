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
#define __SK_SUBSYSTEM__ L"DXGI Util."

#include <SpecialK/render/dxgi/dxgi_interfaces.h>
#include <SpecialK/render/d3d11/d3d11_tex_mgr.h>
#include <SpecialK/render/d3d11/d3d11_core.h>
#include <SpecialK/render/d3d11/d3d11_state_tracker.h>

#include <concurrent_unordered_map.h>

//
// Helpers for typeless DXGI format class view compatibility
//
//   Returns true if this is a valid way of (re)interpreting a sized datatype
bool
SK_DXGI_IsDataSizeClassOf ( DXGI_FORMAT typeless, DXGI_FORMAT typed,
                            int         recursion )
{
  switch (typeless)
  {
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
      return true;

    case DXGI_FORMAT_D24_UNORM_S8_UINT:
      return ( typed == DXGI_FORMAT_D32_FLOAT_S8X24_UINT );
  }

  if (recursion == 0)
    return SK_DXGI_IsDataSizeClassOf (typed, typeless, 1);

  return false;
}

DXGI_FORMAT
SK_DXGI_MakeTypedFormat (DXGI_FORMAT typeless)
{
#if 1
  return
    DirectX::MakeTypelessUNORM   (
      DirectX::MakeTypelessFLOAT (typeless)
                                 );
#else
  switch (typeless)
  {
    // Non-renderable formats
    case DXGI_FORMAT_BC1_TYPELESS:
      return DXGI_FORMAT_BC1_UNORM;
    case DXGI_FORMAT_BC2_TYPELESS:
      return DXGI_FORMAT_BC2_UNORM;
    case DXGI_FORMAT_BC3_TYPELESS:
      return DXGI_FORMAT_BC3_UNORM;
    case DXGI_FORMAT_BC4_TYPELESS:
      return DXGI_FORMAT_BC4_UNORM;
    case DXGI_FORMAT_BC5_TYPELESS:
      return DXGI_FORMAT_BC5_UNORM;
    case DXGI_FORMAT_BC6H_TYPELESS:
      return DXGI_FORMAT_BC6H_SF16;
    case DXGI_FORMAT_BC7_TYPELESS:
      return DXGI_FORMAT_BC7_UNORM;

    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
      return DXGI_FORMAT_B8G8R8A8_UNORM;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
      return DXGI_FORMAT_R8G8B8A8_UNORM;

    case DXGI_FORMAT_R8_TYPELESS:
      return DXGI_FORMAT_R8_UNORM;
    case DXGI_FORMAT_R8G8_TYPELESS:
      return DXGI_FORMAT_R8G8_UNORM;

    case DXGI_FORMAT_R16_TYPELESS:
      return DXGI_FORMAT_R16_FLOAT;
    case DXGI_FORMAT_R16G16_TYPELESS:
      return DXGI_FORMAT_R16G16_FLOAT;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
      return DXGI_FORMAT_R16G16B16A16_FLOAT;

    case DXGI_FORMAT_R32_TYPELESS:
      return DXGI_FORMAT_R32_FLOAT;
    case DXGI_FORMAT_R32G32_TYPELESS:
      return DXGI_FORMAT_R32G32_FLOAT;
    case DXGI_FORMAT_R32G32B32_TYPELESS:
      return DXGI_FORMAT_R32G32B32_FLOAT;
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
      return DXGI_FORMAT_R32G32B32A32_FLOAT;


    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
      return DXGI_FORMAT_R10G10B10A2_UNORM;

    // Generally a depth-stencil texture, so treat as UNORM : UINT
    case DXGI_FORMAT_R24G8_TYPELESS:
      return DXGI_FORMAT_D24_UNORM_S8_UINT;

    // Depth view, stencil is hard to get at
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
      return DXGI_FORMAT_R32G32_FLOAT;


    default:
      return typeless;
  };
#endif
}

DXGI_FORMAT
SK_DXGI_MakeTypelessFormat (DXGI_FORMAT typed)
{
#if 1
  return
    DirectX::MakeTypeless (typed);
#else
    switch (typed)
  {
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
      return DXGI_FORMAT_BC1_TYPELESS;
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
      return DXGI_FORMAT_BC2_TYPELESS;
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
      return DXGI_FORMAT_BC3_TYPELESS;
    case DXGI_FORMAT_BC4_UNORM:
      return DXGI_FORMAT_BC4_TYPELESS;
    case DXGI_FORMAT_BC5_UNORM:
      return DXGI_FORMAT_BC5_TYPELESS;
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC6H_UF16:
      return DXGI_FORMAT_BC6H_TYPELESS;
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
      return DXGI_FORMAT_BC7_TYPELESS;

    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
      return DXGI_FORMAT_B8G8R8A8_TYPELESS;

    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R8G8B8A8_UINT:
      return DXGI_FORMAT_R8G8B8A8_TYPELESS;

    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      return DXGI_FORMAT_B8G8R8A8_TYPELESS;

    case DXGI_FORMAT_R10G10B10A2_UNORM:
      return DXGI_FORMAT_R10G10B10A2_TYPELESS;

    case DXGI_FORMAT_D24_UNORM_S8_UINT:
      return DXGI_FORMAT_R24G8_TYPELESS;

    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_R8_UINT:
      return DXGI_FORMAT_R8_TYPELESS;

    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R8G8_UINT:
      return DXGI_FORMAT_R8G8_TYPELESS;

    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_R16_UINT:
      return DXGI_FORMAT_R16_TYPELESS;

    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R16G16_UINT:
      return DXGI_FORMAT_R16G16_TYPELESS;

    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
      return DXGI_FORMAT_R16G16B16A16_TYPELESS;


    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R32_UINT:
      return DXGI_FORMAT_R32_TYPELESS;

    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G32_UINT:
      return DXGI_FORMAT_R32G32_TYPELESS;

    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_SINT:
    case DXGI_FORMAT_R32G32B32_UINT:
      return DXGI_FORMAT_R32G32B32_TYPELESS;

    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
      return DXGI_FORMAT_R32G32B32A32_TYPELESS;

    default:
      return typed;
  };
#endif
}

bool
__stdcall
SK_DXGI_IsFormatCompressed (DXGI_FORMAT fmt) noexcept
{
  switch (fmt)
  {
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
      return true;

    default:
      return false;
  }
}

bool
__stdcall
SK_DXGI_IsFormatFloat (DXGI_FORMAT fmt) noexcept
{
  switch (fmt)
  {
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
      return true;

    default:
     return false;
  };
}

bool
__stdcall
SK_DXGI_IsFormatInteger (DXGI_FORMAT fmt) noexcept
{
  switch (fmt)
  {
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
      return true;

    default:
      return false;
  };
}

bool
__stdcall
SK_DXGI_IsFormatNormalized (DXGI_FORMAT fmt) noexcept
{
  switch (fmt)
  {
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_A8_UNORM:
    case DXGI_FORMAT_R1_UNORM:
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
    case DXGI_FORMAT_B4G4R4A4_UNORM:
      return true;

    default:
      return false;
  }
}

INT
__stdcall
SK_DXGI_BytesPerPixel (DXGI_FORMAT fmt)
{
  static constexpr INT SixteenBytes   = 16;
  static constexpr INT TwelveBytes    = 12;
  static constexpr INT EightBytes     =  8;
  static constexpr INT FourBytes      =  4;
  static constexpr INT TwoBytes       =  2;
  static constexpr INT OneByte        =  1;
  static constexpr INT BlockComp_Half = -1;
  static constexpr INT BlockComp_One  = -2;
  static constexpr INT UNSUPPORTED    =  0;


  switch (fmt)
  {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:      return SixteenBytes;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:         return SixteenBytes;
    case DXGI_FORMAT_R32G32B32A32_UINT:          return SixteenBytes;
    case DXGI_FORMAT_R32G32B32A32_SINT:          return SixteenBytes;

    case DXGI_FORMAT_R32G32B32_TYPELESS:         return TwelveBytes;
    case DXGI_FORMAT_R32G32B32_FLOAT:            return TwelveBytes;
    case DXGI_FORMAT_R32G32B32_UINT:             return TwelveBytes;
    case DXGI_FORMAT_R32G32B32_SINT:             return TwelveBytes;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:      return EightBytes;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:         return EightBytes;
    case DXGI_FORMAT_R16G16B16A16_UNORM:         return EightBytes;
    case DXGI_FORMAT_R16G16B16A16_UINT:          return EightBytes;
    case DXGI_FORMAT_R16G16B16A16_SNORM:         return EightBytes;
    case DXGI_FORMAT_R16G16B16A16_SINT:          return EightBytes;

    case DXGI_FORMAT_R32G32_TYPELESS:            return EightBytes;
    case DXGI_FORMAT_R32G32_FLOAT:               return EightBytes;
    case DXGI_FORMAT_R32G32_UINT:                return EightBytes;
    case DXGI_FORMAT_R32G32_SINT:                return EightBytes;
    case DXGI_FORMAT_R32G8X24_TYPELESS:          return EightBytes;

    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:       return EightBytes;
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:   return EightBytes;
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:    return EightBytes;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:       return FourBytes;
    case DXGI_FORMAT_R10G10B10A2_UNORM:          return FourBytes;
    case DXGI_FORMAT_R10G10B10A2_UINT:           return FourBytes;
    case DXGI_FORMAT_R11G11B10_FLOAT:            return FourBytes;

    case DXGI_FORMAT_R8G8B8A8_TYPELESS:          return FourBytes;
    case DXGI_FORMAT_R8G8B8A8_UNORM:             return FourBytes;
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:        return FourBytes;
    case DXGI_FORMAT_R8G8B8A8_UINT:              return FourBytes;
    case DXGI_FORMAT_R8G8B8A8_SNORM:             return FourBytes;
    case DXGI_FORMAT_R8G8B8A8_SINT:              return FourBytes;

    case DXGI_FORMAT_R16G16_TYPELESS:            return FourBytes;
    case DXGI_FORMAT_R16G16_FLOAT:               return FourBytes;
    case DXGI_FORMAT_R16G16_UNORM:               return FourBytes;
    case DXGI_FORMAT_R16G16_UINT:                return FourBytes;
    case DXGI_FORMAT_R16G16_SNORM:               return FourBytes;
    case DXGI_FORMAT_R16G16_SINT:                return FourBytes;

    case DXGI_FORMAT_R32_TYPELESS:               return FourBytes;
    case DXGI_FORMAT_D32_FLOAT:                  return FourBytes;
    case DXGI_FORMAT_R32_FLOAT:                  return FourBytes;
    case DXGI_FORMAT_R32_UINT:                   return FourBytes;
    case DXGI_FORMAT_R32_SINT:                   return FourBytes;
    case DXGI_FORMAT_R24G8_TYPELESS:             return FourBytes;

    case DXGI_FORMAT_D24_UNORM_S8_UINT:          return FourBytes;
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:      return FourBytes;
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:       return FourBytes;

    case DXGI_FORMAT_R8G8_TYPELESS:              return TwoBytes;
    case DXGI_FORMAT_R8G8_UNORM:                 return TwoBytes;
    case DXGI_FORMAT_R8G8_UINT:                  return TwoBytes;
    case DXGI_FORMAT_R8G8_SNORM:                 return TwoBytes;
    case DXGI_FORMAT_R8G8_SINT:                  return TwoBytes;

    case DXGI_FORMAT_R16_TYPELESS:               return TwoBytes;
    case DXGI_FORMAT_R16_FLOAT:                  return TwoBytes;
    case DXGI_FORMAT_D16_UNORM:                  return TwoBytes;
    case DXGI_FORMAT_R16_UNORM:                  return TwoBytes;
    case DXGI_FORMAT_R16_UINT:                   return TwoBytes;
    case DXGI_FORMAT_R16_SNORM:                  return TwoBytes;
    case DXGI_FORMAT_R16_SINT:                   return TwoBytes;

    case DXGI_FORMAT_R8_TYPELESS:                return OneByte;
    case DXGI_FORMAT_R8_UNORM:                   return OneByte;
    case DXGI_FORMAT_R8_UINT:                    return OneByte;
    case DXGI_FORMAT_R8_SNORM:                   return OneByte;
    case DXGI_FORMAT_R8_SINT:                    return OneByte;
    case DXGI_FORMAT_A8_UNORM:                   return OneByte;
    case DXGI_FORMAT_R1_UNORM:                   return OneByte;

    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:         return FourBytes;
    case DXGI_FORMAT_R8G8_B8G8_UNORM:            return FourBytes;
    case DXGI_FORMAT_G8R8_G8B8_UNORM:            return FourBytes;

    case DXGI_FORMAT_BC1_TYPELESS:               return BlockComp_Half;
    case DXGI_FORMAT_BC1_UNORM:                  return BlockComp_Half;
    case DXGI_FORMAT_BC1_UNORM_SRGB:             return BlockComp_Half;
    case DXGI_FORMAT_BC2_TYPELESS:               return BlockComp_One;
    case DXGI_FORMAT_BC2_UNORM:                  return BlockComp_One;
    case DXGI_FORMAT_BC2_UNORM_SRGB:             return BlockComp_One;
    case DXGI_FORMAT_BC3_TYPELESS:               return BlockComp_One;
    case DXGI_FORMAT_BC3_UNORM:                  return BlockComp_One;
    case DXGI_FORMAT_BC3_UNORM_SRGB:             return BlockComp_One;
    case DXGI_FORMAT_BC4_TYPELESS:               return BlockComp_Half;
    case DXGI_FORMAT_BC4_UNORM:                  return BlockComp_Half;
    case DXGI_FORMAT_BC4_SNORM:                  return BlockComp_Half;
    case DXGI_FORMAT_BC5_TYPELESS:               return BlockComp_One;
    case DXGI_FORMAT_BC5_UNORM:                  return BlockComp_One;
    case DXGI_FORMAT_BC5_SNORM:                  return BlockComp_One;

    case DXGI_FORMAT_B5G6R5_UNORM:               return TwoBytes;
    case DXGI_FORMAT_B5G5R5A1_UNORM:             return TwoBytes;
    case DXGI_FORMAT_B8G8R8X8_UNORM:             return FourBytes;
    case DXGI_FORMAT_B8G8R8A8_UNORM:             return FourBytes;
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM: return FourBytes;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:          return FourBytes;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:        return FourBytes;
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:          return FourBytes;
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:        return FourBytes;

    case DXGI_FORMAT_BC6H_TYPELESS:              return BlockComp_One;
    case DXGI_FORMAT_BC6H_UF16:                  return BlockComp_One;
    case DXGI_FORMAT_BC6H_SF16:                  return BlockComp_One;
    case DXGI_FORMAT_BC7_TYPELESS:               return BlockComp_One;
    case DXGI_FORMAT_BC7_UNORM:                  return BlockComp_One;
    case DXGI_FORMAT_BC7_UNORM_SRGB:             return BlockComp_One;

    case DXGI_FORMAT_AYUV:                       return (INT)DirectX::BitsPerPixel (DXGI_FORMAT_AYUV      ) / 8;
    case DXGI_FORMAT_Y410:                       return (INT)DirectX::BitsPerPixel (DXGI_FORMAT_Y410      ) / 8;
    case DXGI_FORMAT_Y416:                       return (INT)DirectX::BitsPerPixel (DXGI_FORMAT_Y416      ) / 8;
    case DXGI_FORMAT_NV12:                       return (INT)DirectX::BitsPerPixel (DXGI_FORMAT_NV12      ) / 8;
    case DXGI_FORMAT_P010:                       return (INT)DirectX::BitsPerPixel (DXGI_FORMAT_P010      ) / 8;
    case DXGI_FORMAT_P016:                       return (INT)DirectX::BitsPerPixel (DXGI_FORMAT_P016      ) / 8;
    case DXGI_FORMAT_420_OPAQUE:                 return (INT)DirectX::BitsPerPixel (DXGI_FORMAT_420_OPAQUE) / 8;
    case DXGI_FORMAT_YUY2:                       return (INT)DirectX::BitsPerPixel (DXGI_FORMAT_YUY2      ) / 8;
    case DXGI_FORMAT_Y210:                       return (INT)DirectX::BitsPerPixel (DXGI_FORMAT_Y210      ) / 8;
    case DXGI_FORMAT_Y216:                       return (INT)DirectX::BitsPerPixel (DXGI_FORMAT_Y216      ) / 8;
    case DXGI_FORMAT_NV11:                       return (INT)DirectX::BitsPerPixel (DXGI_FORMAT_NV11      ) / 8;
    case DXGI_FORMAT_AI44:                       return (INT)DirectX::BitsPerPixel (DXGI_FORMAT_AI44      ) / 8;
    case DXGI_FORMAT_IA44:                       return (INT)DirectX::BitsPerPixel (DXGI_FORMAT_IA44      ) / 8;
    case DXGI_FORMAT_P8:                         return OneByte;
    case DXGI_FORMAT_A8P8:                       return TwoBytes;
    case DXGI_FORMAT_B4G4R4A4_UNORM:             return TwoBytes;

    default: SK_ReleaseAssert (!"Unknown Format");
                                                 return UNSUPPORTED;
  }
}

std::string_view
__stdcall
SK_DXGI_FormatToStr (DXGI_FORMAT fmt) noexcept
{
  switch (fmt)
  {
    case DXGI_FORMAT_UNKNOWN:                    return "DXGI_FORMAT_UNKNOWN";

    case DXGI_FORMAT_R32G32B32A32_TYPELESS:      return "DXGI_FORMAT_R32G32B32A32_TYPELESS";
    case DXGI_FORMAT_R32G32B32A32_FLOAT:         return "DXGI_FORMAT_R32G32B32A32_FLOAT";
    case DXGI_FORMAT_R32G32B32A32_UINT:          return "DXGI_FORMAT_R32G32B32A32_UINT";
    case DXGI_FORMAT_R32G32B32A32_SINT:          return "DXGI_FORMAT_R32G32B32A32_SINT";

    case DXGI_FORMAT_R32G32B32_TYPELESS:         return "DXGI_FORMAT_R32G32B32_TYPELESS";
    case DXGI_FORMAT_R32G32B32_FLOAT:            return "DXGI_FORMAT_R32G32B32_FLOAT";
    case DXGI_FORMAT_R32G32B32_UINT:             return "DXGI_FORMAT_R32G32B32_UINT";
    case DXGI_FORMAT_R32G32B32_SINT:             return "DXGI_FORMAT_R32G32B32_SINT";

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:      return "DXGI_FORMAT_R16G16B16A16_TYPELESS";
    case DXGI_FORMAT_R16G16B16A16_FLOAT:         return "DXGI_FORMAT_R16G16B16A16_FLOAT";
    case DXGI_FORMAT_R16G16B16A16_UNORM:         return "DXGI_FORMAT_R16G16B16A16_UNORM";
    case DXGI_FORMAT_R16G16B16A16_UINT:          return "DXGI_FORMAT_R16G16B16A16_UINT";
    case DXGI_FORMAT_R16G16B16A16_SNORM:         return "DXGI_FORMAT_R16G16B16A16_SNORM";
    case DXGI_FORMAT_R16G16B16A16_SINT:          return "DXGI_FORMAT_R16G16B16A16_SINT";

    case DXGI_FORMAT_R32G32_TYPELESS:            return "DXGI_FORMAT_R32G32_TYPELESS";
    case DXGI_FORMAT_R32G32_FLOAT:               return "DXGI_FORMAT_R32G32_FLOAT";
    case DXGI_FORMAT_R32G32_UINT:                return "DXGI_FORMAT_R32G32_UINT";
    case DXGI_FORMAT_R32G32_SINT:                return "DXGI_FORMAT_R32G32_SINT";
    case DXGI_FORMAT_R32G8X24_TYPELESS:          return "DXGI_FORMAT_R32G8X24_TYPELESS";

    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:       return "DXGI_FORMAT_D32_FLOAT_S8X24_UINT";
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:   return "DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS";
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:    return "DXGI_FORMAT_X32_TYPELESS_G8X24_UINT";

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:       return "DXGI_FORMAT_R10G10B10A2_TYPELESS";
    case DXGI_FORMAT_R10G10B10A2_UNORM:          return "DXGI_FORMAT_R10G10B10A2_UNORM";
    case DXGI_FORMAT_R10G10B10A2_UINT:           return "DXGI_FORMAT_R10G10B10A2_UINT";
    case DXGI_FORMAT_R11G11B10_FLOAT:            return "DXGI_FORMAT_R11G11B10_FLOAT";

    case DXGI_FORMAT_R8G8B8A8_TYPELESS:          return "DXGI_FORMAT_R8G8B8A8_TYPELESS";
    case DXGI_FORMAT_R8G8B8A8_UNORM:             return "DXGI_FORMAT_R8G8B8A8_UNORM";
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:        return "DXGI_FORMAT_R8G8B8A8_UNORM_SRGB";
    case DXGI_FORMAT_R8G8B8A8_UINT:              return "DXGI_FORMAT_R8G8B8A8_UINT";
    case DXGI_FORMAT_R8G8B8A8_SNORM:             return "DXGI_FORMAT_R8G8B8A8_SNORM";
    case DXGI_FORMAT_R8G8B8A8_SINT:              return "DXGI_FORMAT_R8G8B8A8_SINT";

    case DXGI_FORMAT_R16G16_TYPELESS:            return "DXGI_FORMAT_R16G16_TYPELESS";
    case DXGI_FORMAT_R16G16_FLOAT:               return "DXGI_FORMAT_R16G16_FLOAT";
    case DXGI_FORMAT_R16G16_UNORM:               return "DXGI_FORMAT_R16G16_UNORM";
    case DXGI_FORMAT_R16G16_UINT:                return "DXGI_FORMAT_R16G16_UINT";
    case DXGI_FORMAT_R16G16_SNORM:               return "DXGI_FORMAT_R16G16_SNORM";
    case DXGI_FORMAT_R16G16_SINT:                return "DXGI_FORMAT_R16G16_SINT";

    case DXGI_FORMAT_R32_TYPELESS:               return "DXGI_FORMAT_R32_TYPELESS";
    case DXGI_FORMAT_D32_FLOAT:                  return "DXGI_FORMAT_D32_FLOAT";
    case DXGI_FORMAT_R32_FLOAT:                  return "DXGI_FORMAT_R32_FLOAT";
    case DXGI_FORMAT_R32_UINT:                   return "DXGI_FORMAT_R32_UINT";
    case DXGI_FORMAT_R32_SINT:                   return "DXGI_FORMAT_R32_SINT";
    case DXGI_FORMAT_R24G8_TYPELESS:             return "DXGI_FORMAT_R24G8_TYPELESS";

    case DXGI_FORMAT_D24_UNORM_S8_UINT:          return "DXGI_FORMAT_D24_UNORM_S8_UINT";
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:      return "DXGI_FORMAT_R24_UNORM_X8_TYPELESS";
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:       return "DXGI_FORMAT_X24_TYPELESS_G8_UINT";

    case DXGI_FORMAT_R8G8_TYPELESS:              return "DXGI_FORMAT_R8G8_TYPELESS";
    case DXGI_FORMAT_R8G8_UNORM:                 return "DXGI_FORMAT_R8G8_UNORM";
    case DXGI_FORMAT_R8G8_UINT:                  return "DXGI_FORMAT_R8G8_UINT";
    case DXGI_FORMAT_R8G8_SNORM:                 return "DXGI_FORMAT_R8G8_SNORM";
    case DXGI_FORMAT_R8G8_SINT:                  return "DXGI_FORMAT_R8G8_SINT";

    case DXGI_FORMAT_R16_TYPELESS:               return "DXGI_FORMAT_R16_TYPELESS";
    case DXGI_FORMAT_R16_FLOAT:                  return "DXGI_FORMAT_R16_FLOAT";
    case DXGI_FORMAT_D16_UNORM:                  return "DXGI_FORMAT_D16_UNORM";
    case DXGI_FORMAT_R16_UNORM:                  return "DXGI_FORMAT_R16_UNORM";
    case DXGI_FORMAT_R16_UINT:                   return "DXGI_FORMAT_R16_UINT";
    case DXGI_FORMAT_R16_SNORM:                  return "DXGI_FORMAT_R16_SNORM";
    case DXGI_FORMAT_R16_SINT:                   return "DXGI_FORMAT_R16_SINT";

    case DXGI_FORMAT_R8_TYPELESS:                return "DXGI_FORMAT_R8_TYPELESS";
    case DXGI_FORMAT_R8_UNORM:                   return "DXGI_FORMAT_R8_UNORM";
    case DXGI_FORMAT_R8_UINT:                    return "DXGI_FORMAT_R8_UINT";
    case DXGI_FORMAT_R8_SNORM:                   return "DXGI_FORMAT_R8_SNORM";
    case DXGI_FORMAT_R8_SINT:                    return "DXGI_FORMAT_R8_SINT";
    case DXGI_FORMAT_A8_UNORM:                   return "DXGI_FORMAT_A8_UNORM";
    case DXGI_FORMAT_R1_UNORM:                   return "DXGI_FORMAT_R1_UNORM";

    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:         return "DXGI_FORMAT_R9G9B9E5_SHAREDEXP";
    case DXGI_FORMAT_R8G8_B8G8_UNORM:            return "DXGI_FORMAT_R8G8_B8G8_UNORM";
    case DXGI_FORMAT_G8R8_G8B8_UNORM:            return "DXGI_FORMAT_G8R8_G8B8_UNORM";

    case DXGI_FORMAT_BC1_TYPELESS:               return "DXGI_FORMAT_BC1_TYPELESS";
    case DXGI_FORMAT_BC1_UNORM:                  return "DXGI_FORMAT_BC1_UNORM";
    case DXGI_FORMAT_BC1_UNORM_SRGB:             return "DXGI_FORMAT_BC1_UNORM_SRGB";
    case DXGI_FORMAT_BC2_TYPELESS:               return "DXGI_FORMAT_BC2_TYPELESS";
    case DXGI_FORMAT_BC2_UNORM:                  return "DXGI_FORMAT_BC2_UNORM";
    case DXGI_FORMAT_BC2_UNORM_SRGB:             return "DXGI_FORMAT_BC2_UNORM_SRGB";
    case DXGI_FORMAT_BC3_TYPELESS:               return "DXGI_FORMAT_BC3_TYPELESS";
    case DXGI_FORMAT_BC3_UNORM:                  return "DXGI_FORMAT_BC3_UNORM";
    case DXGI_FORMAT_BC3_UNORM_SRGB:             return "DXGI_FORMAT_BC3_UNORM_SRGB";
    case DXGI_FORMAT_BC4_TYPELESS:               return "DXGI_FORMAT_BC4_TYPELESS";
    case DXGI_FORMAT_BC4_UNORM:                  return "DXGI_FORMAT_BC4_UNORM";
    case DXGI_FORMAT_BC4_SNORM:                  return "DXGI_FORMAT_BC4_SNORM";
    case DXGI_FORMAT_BC5_TYPELESS:               return "DXGI_FORMAT_BC5_TYPELESS";
    case DXGI_FORMAT_BC5_UNORM:                  return "DXGI_FORMAT_BC5_UNORM";
    case DXGI_FORMAT_BC5_SNORM:                  return "DXGI_FORMAT_BC5_SNORM";

    case DXGI_FORMAT_B5G6R5_UNORM:               return "DXGI_FORMAT_B5G6R5_UNORM";
    case DXGI_FORMAT_B5G5R5A1_UNORM:             return "DXGI_FORMAT_B5G5R5A1_UNORM";
    case DXGI_FORMAT_B8G8R8X8_UNORM:             return "DXGI_FORMAT_B8G8R8X8_UNORM";
    case DXGI_FORMAT_B8G8R8A8_UNORM:             return "DXGI_FORMAT_B8G8R8A8_UNORM";
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM: return "DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM";
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:          return "DXGI_FORMAT_B8G8R8A8_TYPELESS";
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:        return "DXGI_FORMAT_B8G8R8A8_UNORM_SRGB";
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:          return "DXGI_FORMAT_B8G8R8X8_TYPELESS";
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:        return "DXGI_FORMAT_B8G8R8X8_UNORM_SRGB";

    case DXGI_FORMAT_BC6H_TYPELESS:              return "DXGI_FORMAT_BC6H_TYPELESS";
    case DXGI_FORMAT_BC6H_UF16:                  return "DXGI_FORMAT_BC6H_UF16";
    case DXGI_FORMAT_BC6H_SF16:                  return "DXGI_FORMAT_BC6H_SF16";
    case DXGI_FORMAT_BC7_TYPELESS:               return "DXGI_FORMAT_BC7_TYPELESS";
    case DXGI_FORMAT_BC7_UNORM:                  return "DXGI_FORMAT_BC7_UNORM";
    case DXGI_FORMAT_BC7_UNORM_SRGB:             return "DXGI_FORMAT_BC7_UNORM_SRGB";

    case DXGI_FORMAT_AYUV:                       return "DXGI_FORMAT_AYUV";
    case DXGI_FORMAT_Y410:                       return "DXGI_FORMAT_Y410";
    case DXGI_FORMAT_Y416:                       return "DXGI_FORMAT_Y416";
    case DXGI_FORMAT_NV12:                       return "DXGI_FORMAT_NV12";
    case DXGI_FORMAT_P010:                       return "DXGI_FORMAT_P010";
    case DXGI_FORMAT_P016:                       return "DXGI_FORMAT_P016";
    case DXGI_FORMAT_420_OPAQUE:                 return "DXGI_FORMAT_420_OPAQUE";
    case DXGI_FORMAT_YUY2:                       return "DXGI_FORMAT_YUY2";
    case DXGI_FORMAT_Y210:                       return "DXGI_FORMAT_Y210";
    case DXGI_FORMAT_Y216:                       return "DXGI_FORMAT_Y216";
    case DXGI_FORMAT_NV11:                       return "DXGI_FORMAT_NV11";
    case DXGI_FORMAT_AI44:                       return "DXGI_FORMAT_AI44";
    case DXGI_FORMAT_IA44:                       return "DXGI_FORMAT_IA44";
    case DXGI_FORMAT_P8:                         return "DXGI_FORMAT_P8";
    case DXGI_FORMAT_A8P8:                       return "DXGI_FORMAT_A8P8";
    case DXGI_FORMAT_B4G4R4A4_UNORM:             return "DXGI_FORMAT_B4G4R4A4_UNORM";

    default:                                     return "UNKNOWN";
  }
}

#include <shaders/sRGB_codec_ps.h>
#include <shaders/vs_colorutil.h>
#include <SpecialK/render/dxgi/dxgi_hdr.h>

struct SK_DXGI_sRGBCoDec {
  struct params_cbuffer_s {
    BOOL passthrough = FALSE;
    BOOL apply       = FALSE;
    BOOL strip       = FALSE;
    BOOL no_alpha    = FALSE;
  };

  struct device_instance_s
  {
    ID3D11InputLayout*            pInputLayout = nullptr;
    ID3D11Buffer*                  pSRGBParams = nullptr;
    ID3D11Buffer*                  pVSUtilLuma = nullptr;

    ID3D11SamplerState*               pSampler = nullptr;

    ID3D11ShaderResourceView*   pBackbufferSrv = nullptr;
    ID3D11RenderTargetView*     pBackbufferRtv = nullptr;

    ID3D11RasterizerState*        pRasterState = nullptr;
    ID3D11DepthStencilState*          pDSState = nullptr;

    ID3D11BlendState*              pBlendState = nullptr;

    D3D11_FEATURE_DATA_D3D11_OPTIONS FeatureOpts
                                               = { };
    D3DX11_STATE_BLOCK                      sb = { };

    static constexpr FLOAT    fBlendFactor [4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    SK::DXGI::ShaderBase <ID3D11PixelShader>  PixelShader_sRGB_NoMore;
    SK::DXGI::ShaderBase <ID3D11VertexShader> VertexShader_Util;

    void release (void)
    {
      if (pSampler       != nullptr) { pSampler->Release       (); pSampler       = nullptr; }

      if (pBackbufferSrv != nullptr) { pBackbufferSrv->Release (); pBackbufferSrv = nullptr; }
      if (pBackbufferRtv != nullptr) { pBackbufferRtv->Release (); pBackbufferRtv = nullptr; }

      if (pRasterState   != nullptr) { pRasterState->Release   (); pRasterState   = nullptr; }
      if (pDSState       != nullptr) { pDSState->Release       ();     pDSState   = nullptr; }

      if (pVSUtilLuma    != nullptr) { pVSUtilLuma->Release    (); pVSUtilLuma    = nullptr; }
      if (pSRGBParams    != nullptr) { pSRGBParams->Release    (); pSRGBParams    = nullptr; }
      if (pBlendState    != nullptr) { pBlendState->Release    (); pBlendState    = nullptr; }
      if (pInputLayout   != nullptr) { pInputLayout->Release   (); pInputLayout   = nullptr; }

      PixelShader_sRGB_NoMore.releaseResources ();
      VertexShader_Util.releaseResources       ();
    }
  };

  Concurrency::concurrent_unordered_map <SK_ComPtr <ID3D11Device>, device_instance_s> devices_;

  void releaseResources (ID3D11Device *pDev)
  {
    if (pDev == nullptr)
      return;

    auto device =
      devices_.find (pDev);

    if (device == devices_.end ())
      return;

    device->second.release ();
  }

  bool
  recompileShaders (ID3D11Device *pDev)
  {
    if (pDev == nullptr)
      return false;

    auto& device =
      devices_ [pDev];

    bool ret =
      pDev->CreatePixelShader ( sRGB_codec_ps_bytecode,
                        sizeof (sRGB_codec_ps_bytecode),
             nullptr, &device.PixelShader_sRGB_NoMore.shader ) == S_OK;
    ret &=
      pDev->CreateVertexShader ( colorutil_vs_bytecode,
                         sizeof (colorutil_vs_bytecode),
            nullptr, &device.VertexShader_Util.shader ) == S_OK;

    return ret;
  }

  void
  reloadResources (ID3D11Device *pDev, IDXGISwapChain *pSwapChain = nullptr)
  {
    releaseResources (pDev);

    if (pDev != nullptr)
    {
      auto& device =
        devices_ [pDev];

      if (! recompileShaders (pDev))
        return;

      if ( device.pBackbufferSrv == nullptr &&
                      pSwapChain != nullptr )
      {
        if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_11_1)
        {
          pDev->CheckFeatureSupport (
             D3D11_FEATURE_D3D11_OPTIONS, &device.FeatureOpts,
               sizeof (D3D11_FEATURE_DATA_D3D11_OPTIONS)
          );
        }

        SK_ScopedBool decl_tex_scope (
          SK_D3D11_DeclareTexInjectScope ()
        );

        DXGI_SWAP_CHAIN_DESC  swap_desc = { };
        pSwapChain->GetDesc (&swap_desc);

        D3D11_TEXTURE2D_DESC
          tex_desc                    = { };

          tex_desc.Width              = swap_desc.BufferDesc.Width;
          tex_desc.Height             = swap_desc.BufferDesc.Height;
          tex_desc.MipLevels          = 1;
          tex_desc.ArraySize          = 1;
          tex_desc.Format             = swap_desc.BufferDesc.Format;
          tex_desc.SampleDesc.Count   = 1;
          tex_desc.SampleDesc.Quality = 0;
          tex_desc.Usage              = D3D11_USAGE_DEFAULT;
          tex_desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
          tex_desc.CPUAccessFlags     = 0x0;
          tex_desc.MiscFlags          = 0x0;

        SK_ComPtr <ID3D11Texture2D> pSRGBTexture;

        pDev->CreateTexture2D          (&tex_desc,    nullptr, &pSRGBTexture);
        pDev->CreateShaderResourceView (pSRGBTexture, nullptr, &device.pBackbufferSrv);
      }

      D3D11_INPUT_ELEMENT_DESC local_layout [] = {
        { "", 0, DXGI_FORMAT_R32_UINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
      };

      pDev->CreateInputLayout ( local_layout, 0,
                             (const void *)(colorutil_vs_bytecode),
                                    sizeof (colorutil_vs_bytecode) /
                                    sizeof (colorutil_vs_bytecode [0]),
                                      &device.pInputLayout );

      D3D11_SAMPLER_DESC
        sampler_desc                    = { };

        sampler_desc.Filter             = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampler_desc.AddressU           = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressV           = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressW           = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.MipLODBias         = 0.f;
        sampler_desc.MaxAnisotropy      =   1;
        sampler_desc.ComparisonFunc     =  D3D11_COMPARISON_NEVER;
        sampler_desc.MinLOD             = -D3D11_FLOAT32_MAX;
        sampler_desc.MaxLOD             =  D3D11_FLOAT32_MAX;

      pDev->CreateSamplerState ( &sampler_desc,
                                            &device.pSampler );

      SK_D3D11_SetDebugName (device.pSampler, L"sRGBLinearizer SamplerState");

      D3D11_BLEND_DESC
        blend_desc                                        = { };

        blend_desc.AlphaToCoverageEnable                  = false;
        blend_desc.RenderTarget [0].BlendEnable           = false;
        blend_desc.RenderTarget [0].SrcBlend              = D3D11_BLEND_ONE;
        blend_desc.RenderTarget [0].DestBlend             = D3D11_BLEND_ZERO;
        blend_desc.RenderTarget [0].BlendOp               = D3D11_BLEND_OP_ADD;
        blend_desc.RenderTarget [0].SrcBlendAlpha         = D3D11_BLEND_ONE;
        blend_desc.RenderTarget [0].DestBlendAlpha        = D3D11_BLEND_ZERO;
        blend_desc.RenderTarget [0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
        blend_desc.RenderTarget [0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

      pDev->CreateBlendState ( &blend_desc,
                                 &device.pBlendState );

      SK_D3D11_SetDebugName (device.pBlendState, L"sRGBLinearizer BlendState");

      D3D11_DEPTH_STENCIL_DESC
        depth                          = {  };

        depth.DepthEnable              = FALSE;
        depth.StencilEnable            = FALSE;
        depth.DepthWriteMask           = D3D11_DEPTH_WRITE_MASK_ZERO;
        depth.DepthFunc                = D3D11_COMPARISON_ALWAYS;
        depth.FrontFace.StencilFailOp  = depth.FrontFace.StencilDepthFailOp =
                                         depth.FrontFace.StencilPassOp      =
                                         D3D11_STENCIL_OP_KEEP;
        depth.FrontFace.StencilFunc    = D3D11_COMPARISON_ALWAYS;
        depth.BackFace                 = depth.FrontFace;

      pDev->CreateDepthStencilState   ( &depth, &device.pDSState );

      SK_D3D11_SetDebugName (device.pDSState, L"sRGBLinearizer DepthStencilState");

      D3D11_BUFFER_DESC
        buffer_desc                   = { };

        buffer_desc.ByteWidth         = sizeof (params_cbuffer_s);
        buffer_desc.Usage             = D3D11_USAGE_DYNAMIC;
        buffer_desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
        buffer_desc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;
        buffer_desc.MiscFlags         = 0;

      pDev->CreateBuffer (&buffer_desc, nullptr, &device.pSRGBParams);

      SK_D3D11_SetDebugName (device.pSRGBParams, L"sRGBLinearizer CBuffer");

        buffer_desc                   = { };

        buffer_desc.ByteWidth         = sizeof (HDR_LUMINANCE);
        buffer_desc.Usage             = D3D11_USAGE_DYNAMIC;
        buffer_desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
        buffer_desc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;
        buffer_desc.MiscFlags         = 0;

      pDev->CreateBuffer (&buffer_desc, nullptr, &device.pVSUtilLuma);

      SK_D3D11_SetDebugName (device.pSRGBParams, L"VS Colorutil CBuffer");
    }
  }
};

SK_LazyGlobal <SK_DXGI_sRGBCoDec> srgb_codec;

void
SK_DXGI_ReleaseSRGBLinearizer (void)
{
  const SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if (rb.device != nullptr)
  {
    auto pDev =
      rb.getDevice <ID3D11Device> ();

    if (pDev.p != nullptr)
    {
      srgb_codec->releaseResources (
        pDev.p
      );
    }
  }
}

bool
SK_DXGI_LinearizeSRGB (IDXGISwapChain* pChainThatUsedToBeSRGB)
{
  if (pChainThatUsedToBeSRGB == nullptr)
    return false;

  SK_ComPtr <IDXGISwapChain> pSwapChain (pChainThatUsedToBeSRGB);
  SK_ComPtr <ID3D11DeviceContext>                                pDevCtx;
  SK_ComPtr <ID3D11Device>                                       pDev;
  pChainThatUsedToBeSRGB->GetDevice (IID_ID3D11Device, (void **)&pDev.p);

  if (! pDev)
    return false;

  pDev->GetImmediateContext (&pDevCtx.p);

  if (! pDevCtx)
    return false;

  auto& codec        = srgb_codec->devices_ [pDev];
  auto& vs_hdr_util  = codec.VertexShader_Util;
  auto& ps_hdr_scrgb = codec.PixelShader_sRGB_NoMore;

  if ( vs_hdr_util.shader   == nullptr ||
       ps_hdr_scrgb.shader  == nullptr ||
       codec.pBackbufferSrv == nullptr )
  {
    srgb_codec->reloadResources (pDev, pChainThatUsedToBeSRGB);

    if ( vs_hdr_util.shader   == nullptr ||
         ps_hdr_scrgb.shader  == nullptr ||
         codec.pBackbufferSrv == nullptr )
      return false;
  }

  SK_ComPtr <ID3D11Resource> pSrc = nullptr;
  SK_ComPtr <ID3D11Resource> pDst = nullptr;
  DXGI_SWAP_CHAIN_DESC  swap_desc = { };

  if (pSwapChain != nullptr)
  {
    pSwapChain->GetDesc (&swap_desc);

    if ( SUCCEEDED (
           pSwapChain->GetBuffer    (
             0,
               IID_ID3D11Texture2D,
                 (void **)&pSrc.p  )
                   )
       )
    {
      codec.pBackbufferSrv->GetResource ( &pDst.p );
                  pDevCtx->CopyResource (  pDst, pSrc );
    }

    else
      return false;
  }

  D3D11_MAPPED_SUBRESOURCE
        mapped_resource = { };

  if ( SUCCEEDED (
         pDevCtx->Map ( codec.pVSUtilLuma,
                          0, D3D11_MAP_WRITE_DISCARD, 0,
                             &mapped_resource )
                 )
     )
  {
    HDR_LUMINANCE luma = { };

    _ReadWriteBarrier ();

    memcpy ( static_cast <HDR_LUMINANCE *> (mapped_resource.pData),
           &luma, sizeof (HDR_LUMINANCE) );

    pDevCtx->Unmap (codec.pVSUtilLuma, 0);
  }

  else
    return false;

  if ( SUCCEEDED (
         pDevCtx->Map ( codec.pSRGBParams,
                          0, D3D11_MAP_WRITE_DISCARD, 0,
                             &mapped_resource )
                 )
     )
  {
    SK_DXGI_sRGBCoDec::params_cbuffer_s
      codec_params = { .passthrough = -1,
                       .apply       = FALSE,
                       .strip       = FALSE };

    codec_params.apply       = (config.render.dxgi.srgb_behavior ==  1 ||
                                config.render.dxgi.srgb_behavior == -2);
    codec_params.strip       = (config.render.dxgi.srgb_behavior ==  0);
    codec_params.passthrough = (config.render.dxgi.srgb_behavior == -1);

    _ReadWriteBarrier ();

    memcpy (    static_cast <SK_DXGI_sRGBCoDec::params_cbuffer_s *> (mapped_resource.pData),
       &codec_params, sizeof(SK_DXGI_sRGBCoDec::params_cbuffer_s));

    pDevCtx->Unmap (codec.pSRGBParams, 0);
  }

  else
    return false;

  SK_ComPtr <ID3D11RenderTargetView> pRenderTargetView = nullptr;

  D3D11_RENDER_TARGET_VIEW_DESC rtdesc               = {                           };
                                rtdesc.ViewDimension = swap_desc.SampleDesc.Count == 1 ?
                                                         D3D11_RTV_DIMENSION_TEXTURE2D :
                                                         D3D11_RTV_DIMENSION_TEXTURE2DMS;
                                rtdesc.Format        = DirectX::MakeSRGB (swap_desc.BufferDesc.Format);

  if ( SUCCEEDED (
         pDev->CreateRenderTargetView ( pSrc, &rtdesc,
                                  &pRenderTargetView.p )
       )
     )
  {
    SK_D3D11_SetDebugName (pRenderTargetView, L"sRGBLinearizer Single-Use RTV");

#if 0
    SK_IMGUI_D3D11StateBlock
      sb;
      sb.Capture (pDevCtx);
#else
    D3DX11_STATE_BLOCK sblock = { };
    auto *sb =        &sblock;

    CreateStateblock (pDevCtx, sb);
#endif

    const D3D11_VIEWPORT vp = {                    0.0f, 0.0f,
      static_cast <float> (swap_desc.BufferDesc.Width ),
      static_cast <float> (swap_desc.BufferDesc.Height),
                                                   0.0f, 1.0f
    };

    pDevCtx->IASetPrimitiveTopology (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    pDevCtx->IASetVertexBuffers     (0, 1, std::array <ID3D11Buffer *, 1> { nullptr }.data (),
                                           std::array <UINT,           1> { 0       }.data (),
                                           std::array <UINT,           1> { 0       }.data ());
    pDevCtx->IASetInputLayout       (        codec.pInputLayout     );
    pDevCtx->IASetIndexBuffer       (nullptr, DXGI_FORMAT_UNKNOWN, 0);

    pDevCtx->OMSetBlendState        (        codec.pBlendState,
                                             codec.fBlendFactor,   0xFFFFFFFFU);
    pDevCtx->OMSetDepthStencilState (        codec.pDSState,       0);
    pDevCtx->OMSetRenderTargets     (1,
                                       &pRenderTargetView.p,
                                         nullptr);

    pDevCtx->VSSetShader            (        codec.VertexShader_Util.shader,       nullptr, 0);
    pDevCtx->VSSetConstantBuffers   (0, 1,  &codec.pVSUtilLuma   );
    pDevCtx->PSSetShader            (        codec.PixelShader_sRGB_NoMore.shader, nullptr, 0);
    pDevCtx->PSSetConstantBuffers   (0, 1,  &codec.pSRGBParams   );
    pDevCtx->PSSetShaderResources   (0, 1,  &codec.pBackbufferSrv);
    pDevCtx->PSSetSamplers          (0, 1,  &codec.pSampler      );

    pDevCtx->RSSetState             (codec.pRasterState);
    pDevCtx->RSSetScissorRects      (0,         nullptr);
    pDevCtx->RSSetViewports         (1,             &vp);

    if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_10_0)
    {
      if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_11_0)
      {
        pDevCtx->HSSetShader        (nullptr, nullptr,       0);
        pDevCtx->DSSetShader        (nullptr, nullptr,       0);
      }
      pDevCtx->GSSetShader          (nullptr, nullptr,       0);
      pDevCtx->SOSetTargets         (0,       nullptr, nullptr);
    }

    pDevCtx->Draw                   (3, 0);

    ApplyStateblock (pDevCtx, sb);
    //sb.Apply (pDevCtx.p);

    return true;
  }

  return false;
}



bool
SK_DXGI_IsFormatCastable ( DXGI_FORMAT inFormat,
                           DXGI_FORMAT outFormat )
{
  if ( DirectX::MakeTypeless (inFormat) ==
       DirectX::MakeTypeless (outFormat) )
    return true;

  if ( DirectX::MakeTypeless (inFormat)  == DXGI_FORMAT_B8G8R8A8_TYPELESS &&
       DirectX::MakeTypeless (outFormat) == DXGI_FORMAT_R8G8B8A8_TYPELESS )
    return true;

  if ( DirectX::MakeTypeless (inFormat)  == DXGI_FORMAT_B8G8R8A8_TYPELESS &&
       DirectX::MakeTypeless (outFormat) == DXGI_FORMAT_R8G8B8A8_TYPELESS )
    return true;

  // This function was written in a weird way, should be re-written in the future, but for now, do this...
  DXGI_FORMAT to   = DirectX::IsTypeless (inFormat) ? outFormat : inFormat;
  DXGI_FORMAT from = DirectX::IsTypeless (inFormat) ? inFormat  : outFormat;

  if (! (DirectX::IsTypeless (inFormat) || DirectX::IsTypeless (outFormat)))
    return false;

  switch (from)
  {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
      switch (to)
      {
        default:
          return false;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
          return true;
      }
      break;
  
    case DXGI_FORMAT_R32G32B32_TYPELESS:
      switch (to)
      {
        default:
          return false;
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
          return true;
      }
      break;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
      switch (to)
      {
        default:
          return false;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
          return true;
      }
      break;

    case DXGI_FORMAT_R32G32_TYPELESS:
      switch (to)
      {
        default:
          return false;
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
          return true;
      }
      break;
    
    case DXGI_FORMAT_R32G8X24_TYPELESS:
      switch (to)
      {
        default:
          return false;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
          return true;
      }
      break;
    
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
      switch (to)
      {
        default:
          return false;
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
          return true;
      }
      break;

    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
      switch (to)
      {
        default:
          return false;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
          return true;
      }
      break;

    case DXGI_FORMAT_R16G16_TYPELESS:
      switch (to)
      {
        default:
          return false;
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:              
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:              
        case DXGI_FORMAT_R16G16_SINT:
          return true;
      }
      break;

    case DXGI_FORMAT_R32_TYPELESS:
      switch (to)
      {
        default:
          return false;
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
          return true;
      }
      break;

    case DXGI_FORMAT_R24G8_TYPELESS:
      switch (to)
      {
        default:
          return false;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
          return true;
      }
      break;

    case DXGI_FORMAT_R8G8_TYPELESS:
      switch (to)
      {
        default:
          return false;
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
          return true;
      }
      break;

    case DXGI_FORMAT_R16_TYPELESS:
      switch (to)
      {
        default:
          return false;
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
          return true;
      }
      break;

    case DXGI_FORMAT_R8_TYPELESS:
      switch (to)
      {
        default:
          return false;
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
          return true;
      }
      break;

    case DXGI_FORMAT_BC1_TYPELESS:
      switch (to)
      {
        default:
          return false;
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
          return true;
      }
      break;

    case DXGI_FORMAT_BC2_TYPELESS:
      switch (to)
      {
        default:
          return false;
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
          return true;
      }
      break;

    case DXGI_FORMAT_BC3_TYPELESS:
      switch (to)
      {
        default:
          return false;
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
          return true;
      }
      break;

    case DXGI_FORMAT_BC4_TYPELESS:
      switch (to)
      {
        default:
          return false;
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
          return true;
      }
      break;

    case DXGI_FORMAT_BC5_TYPELESS:
      switch (to)
      {    
        default:
          return false;
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
          return true;
      }
      break;

    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
      switch (to)
      {
        default:
          return false;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
          return true;
      }
      break;

    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
      switch (to)
      {    
        default:
          return false;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
          return true;
      }
      break;

    case DXGI_FORMAT_BC6H_TYPELESS:
      switch (to)
      {
        default:
          return false;
        case DXGI_FORMAT_BC6H_UF16:                 
        case DXGI_FORMAT_BC6H_SF16:
          return true;
      }
      break;

    case DXGI_FORMAT_BC7_TYPELESS:
      switch (to)
      {    
        default:
          return false;
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
          return true;
      }
      break;

    // nb: Table is incomplete, it lacks various planar and video formats
  }

  return false;
}

// Derived from d3dx12_property_format_table.cpp
// https://github.com/microsoft/DirectX-Headers/blob/48a762973271c5a75869946bf1fdbc489a628a5c/src/d3dx12_property_format_table.cpp#L2315
bool
SK_DXGI_IsUAVFormatCastable (DXGI_FORMAT from,
                             DXGI_FORMAT to)
{
  // Allow casting of 32 bit formats to R32_*
  if(
      ((to == DXGI_FORMAT_R32_UINT)||(to == DXGI_FORMAT_R32_SINT)||(to == DXGI_FORMAT_R32_FLOAT))
      && 
      (
          (from == DXGI_FORMAT_R10G10B10A2_TYPELESS) ||
          (from == DXGI_FORMAT_R8G8B8A8_TYPELESS) ||
          (from == DXGI_FORMAT_B8G8R8A8_TYPELESS) ||
          (from == DXGI_FORMAT_B8G8R8X8_TYPELESS) ||
          (from == DXGI_FORMAT_R16G16_TYPELESS) ||
          (from == DXGI_FORMAT_R32_TYPELESS) 
      )
  )
  {
    return true;
  }
  return false;
}

bool
SK_D3D11_BltCopySurface ( ID3D11Texture2D *pSrcTex,
                          ID3D11Texture2D *pDstTex,
            _In_opt_ const D3D11_BOX      *pSrcBox,
            _In_opt_       UINT             SrcSubresource,
            _In_opt_       UINT             DstSubresource,
            _In_opt_       UINT             DstX,
            _In_opt_       UINT             DstY )
{
  SK_ComPtr <ID3D11DeviceContext> pDevCtx;
  SK_ComPtr <ID3D11Device>        pDev;
  pSrcTex->GetDevice            (&pDev.p);
  pDev->GetImmediateContext     (&pDevCtx.p);

  if (! pDevCtx)
    return false;

  SK_ComPtr <ID3D11DeviceContext>             pUnwrapped;
  if (SUCCEEDED (pDevCtx->QueryInterface (IID_IUnwrappedD3D11DeviceContext,
                                    (void **)&pUnwrapped.p)))
  {
    pDevCtx = pUnwrapped.p;
  }

  auto& codec        = srgb_codec->devices_ [pDev];
  auto& vs_hdr_util  = codec.VertexShader_Util;
  auto& ps_hdr_scrgb = codec.PixelShader_sRGB_NoMore;

  if ( vs_hdr_util.shader  == nullptr ||
       ps_hdr_scrgb.shader == nullptr    )
  {
    srgb_codec->reloadResources (pDev);

    if ( vs_hdr_util.shader  == nullptr ||
         ps_hdr_scrgb.shader == nullptr    ) return false;
  }

  D3D11_TEXTURE2D_DESC dstTexDesc = { };
  D3D11_TEXTURE2D_DESC srcTexDesc = { };
    pSrcTex->GetDesc (&srcTexDesc);
    pDstTex->GetDesc (&dstTexDesc);

  auto _Return = [&](bool ret)->bool
  {
    if (! ret)
    {
      // Give user a warning if we are trying to HDR remaster
      //  (input = 8/10/11-bit, output = 16-bpc) but fail, so
      //    they can turn off HDR remastering.
      if ( ((DirectX::BitsPerColor (srcTexDesc.Format) ==  8  ||
             DirectX::BitsPerColor (srcTexDesc.Format) ==  10 ||
             DirectX::BitsPerColor (srcTexDesc.Format) == 11) &&
             DirectX::BitsPerColor (dstTexDesc.Format) == 16) ||
           ((DirectX::BitsPerColor (dstTexDesc.Format) ==  8  ||
             DirectX::BitsPerColor (dstTexDesc.Format) ==  10 ||
             DirectX::BitsPerColor (dstTexDesc.Format) == 11) &&
             DirectX::BitsPerColor (srcTexDesc.Format) == 16) )
      {
        SK_RunOnce (
          SK_LOGi0 (L"Failed to BltCopy Surface: src fmt=%hs, dst fmt=%hs",
                    SK_DXGI_FormatToStr (srcTexDesc.Format).data (),
                    SK_DXGI_FormatToStr (dstTexDesc.Format).data ()
          );
          SK_ImGui_Warning (
            SK_FormatStringW (
              L"Failed HDR Remaster [Type=%hs]\t\tSrc=%d-bit, Dst=%d-bit",
              (dstTexDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) ? "Compute"
                                                                   : "Render",
                DirectX::BitsPerColor (srcTexDesc.Format),
                DirectX::BitsPerColor (dstTexDesc.Format)
            ).c_str ()
          )
        );
      }
    }

    return ret;
  };

  if ((srcTexDesc.ArraySize != 1 && srcTexDesc.MipLevels != 1) ||
      (dstTexDesc.ArraySize != 1 && dstTexDesc.MipLevels != 1))
  {
    //SK_ReleaseAssert (srcTexDesc.ArraySize == 1 || srcTexDesc.MipLevels == 1);
    //SK_ReleaseAssert (dstTexDesc.ArraySize == 1 || dstTexDesc.MipLevels == 1);
    return
      _Return (false);
  }

  SK_RunOnce (SK_ReleaseAssert (DstX == 0 && DstY == 0));

  if ( DirectX::IsCompressed (srcTexDesc.Format)           ||
       DirectX::IsCompressed (dstTexDesc.Format)           ||
       SK_ComPtr <ID3D11Texture2D> (pSrcTex).IsEqualObject (pDstTex)              ||
     //srcTexDesc.ArraySize        != dstTexDesc.ArraySize ||
     //srcTexDesc.MipLevels        != dstTexDesc.MipLevels ||
      (srcTexDesc.SampleDesc.Count != dstTexDesc.SampleDesc.Count &&
       srcTexDesc.SampleDesc.Count != 1) )
  {
    // This isn't so much a fix as it is masking a different problem...
    //
    //  * Figure out why the software is trying to copy a mipmapped resource that
    //     requires format conversion DirectX cannot provide.
    //
    //SK_ReleaseAssert (
    //     srcTexDesc.MipLevels == dstTexDesc.MipLevels
    //);

    if (srcTexDesc.MipLevels != dstTexDesc.MipLevels)
    {
      SK_LOGi0 (
        L"Mipmap Mismatch on Blt Copy, src=%hs, dst=%hs", SK_DXGI_FormatToStr (srcTexDesc.Format).data (),
                                                          SK_DXGI_FormatToStr (dstTexDesc.Format).data ()
      );
    }

    //// SK_ReleaseAssert (! L"Impossible BltCopy Requested");

    return
      _Return (false);
  }

  UINT dev_idx =
    SK_D3D11_GetDeviceContextHandle (pDevCtx);

  auto flag_result =
    SK_ImGui_FlagDrawing_OnD3D11Ctx (dev_idx);

  SK_ScopedBool auto_bool (flag_result.first);
                          *flag_result.first = flag_result.second;

  SK_ScopedBool decl_tex_scope (
    SK_D3D11_DeclareTexInjectScope ()
  );

#if 0
  // Must be zero or one, or the input/output surface is incompatible
  if (dstTexDesc.MipLevels > 1 || srcTexDesc.MipLevels > 1)
  {
    ///SK_ReleaseAssert ( dstTexDesc.MipLevels <= 1 &&
    ///                   srcTexDesc.MipLevels <= 1 );
    SK_RunOnce (
      SK_LOGi0 (
        L" *** SK_D3D11_BltCopySurface (...) from src w/ %d mip levels to dst w/ %d",
           srcTexDesc.MipLevels, dstTexDesc.MipLevels )
    );
  }
#endif

  struct {
    struct {
      SK_ComPtr <ID3D11Texture2D>          tex  = nullptr;
      SK_ComPtr <ID3D11Texture2D>          tex2 = nullptr;
      SK_ComPtr <ID3D11RenderTargetView>   rtv  = nullptr;
    } render;

    struct {
      SK_ComPtr <ID3D11ShaderResourceView> srv  = nullptr;
    } source;

    struct {
      D3D11_TEXTURE2D_DESC                 tex  = { };
      D3D11_TEXTURE2D_DESC                 tex2 = { };
      D3D11_RENDER_TARGET_VIEW_DESC        rtv  = { };
      D3D11_SHADER_RESOURCE_VIEW_DESC      srv  = { };
    } desc;
  } surface;

  bool bArraySRV =
    srcTexDesc.ArraySize != 1;

  bool bArrayRTV =
    dstTexDesc.ArraySize != 1;

  // Handle subresource (mipmap) selection
  if (! bArraySRV)
  {
    srcTexDesc.Width  = std::max (1U, srcTexDesc.Width  >> SrcSubresource);
    srcTexDesc.Height = std::max (1U, srcTexDesc.Height >> SrcSubresource);
  }

  if (! bArrayRTV)
  {
    dstTexDesc.Width  = std::max (1U, dstTexDesc.Width  >> DstSubresource);
    dstTexDesc.Height = std::max (1U, dstTexDesc.Height >> DstSubresource);
  }

  surface.desc.tex.Width                     = dstTexDesc.Width;
	surface.desc.tex.Height                    = dstTexDesc.Height;
	surface.desc.tex.MipLevels                 = dstTexDesc.MipLevels;//1;
	surface.desc.tex.ArraySize                 = 1;
	surface.desc.tex.Format                    = dstTexDesc.Format;
	surface.desc.tex.SampleDesc.Count          = dstTexDesc.SampleDesc.Count;
  surface.desc.tex.SampleDesc.Quality        = dstTexDesc.SampleDesc.Quality;
	surface.desc.tex.Usage                     = D3D11_USAGE_DEFAULT;
	surface.desc.tex.BindFlags                 = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	surface.desc.tex.CPUAccessFlags            = 0;
	surface.desc.tex.MiscFlags                 = 0;

  surface.desc.tex2.Width                    = srcTexDesc.Width;
	surface.desc.tex2.Height                   = srcTexDesc.Height;
	surface.desc.tex2.MipLevels                = srcTexDesc.MipLevels;//1;
	surface.desc.tex2.ArraySize                = 1;
	surface.desc.tex2.Format                   = srcTexDesc.Format;
	surface.desc.tex2.SampleDesc.Count         = srcTexDesc.SampleDesc.Count;
  surface.desc.tex2.SampleDesc.Quality       = srcTexDesc.SampleDesc.Quality;
	surface.desc.tex2.Usage                    = D3D11_USAGE_DEFAULT;
	surface.desc.tex2.BindFlags                = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	surface.desc.tex2.CPUAccessFlags           = 0;
	surface.desc.tex2.MiscFlags                = 0;

  // Cast any typeless views to UNORM
  srcTexDesc.Format =
    SK_DXGI_MakeTypedFormat (srcTexDesc.Format);
  dstTexDesc.Format =
    SK_DXGI_MakeTypedFormat (dstTexDesc.Format);

  bool bMultisampleRtv =
    surface.desc.tex.SampleDesc.Count > 1;

  D3D11_RTV_DIMENSION rtvDim =
    bMultisampleRtv ? bArrayRTV ? D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY
                                : D3D11_RTV_DIMENSION_TEXTURE2DMS
                    : bArrayRTV ? D3D11_RTV_DIMENSION_TEXTURE2DARRAY
                                : D3D11_RTV_DIMENSION_TEXTURE2D;

  bool bMultisampleSrv =
    surface.desc.tex2.SampleDesc.Count > 1;

  D3D11_SRV_DIMENSION srvDim =
    bMultisampleSrv ? bArraySRV ? D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY
                                : D3D11_SRV_DIMENSION_TEXTURE2DMS
                    : bArraySRV ? D3D11_SRV_DIMENSION_TEXTURE2DARRAY
                                : D3D11_SRV_DIMENSION_TEXTURE2D;

  surface.desc.rtv.Format                    = dstTexDesc.Format;
	surface.desc.rtv.ViewDimension             = rtvDim;

  if (! bArrayRTV)
  {
	  surface.desc.rtv.Texture2D.MipSlice      = DstSubresource;
  }
  else
  {
    surface.desc.rtv.Texture2DArray.FirstArraySlice = DstSubresource;
    surface.desc.rtv.Texture2DArray.ArraySize       = 1;
  }

  surface.desc.srv.Format                    = srcTexDesc.Format;
  surface.desc.srv.ViewDimension             = srvDim;

  if (! bArraySRV)
  {
	  surface.desc.srv.Texture2D.MostDetailedMip = SrcSubresource;
	  surface.desc.srv.Texture2D.MipLevels       = 1;
  }

  else
  {
    surface.desc.srv.Texture2DArray.ArraySize       = 1;
    surface.desc.srv.Texture2DArray.FirstArraySlice = SrcSubresource;
  }

  UINT size =
     sizeof (void *);

  if ( srcTexDesc.Width  == dstTexDesc.Width &&
       srcTexDesc.Height == dstTexDesc.Height )
  {
    // TODO: Use another GUID for src/dst Blt Copy
    if ( SUCCEEDED (
           pSrcTex->GetPrivateData ( SKID_D3D11_CachedBltCopySrc,
                                       &size, &surface.render.tex.p )
         )
       )
    {
      D3D11_TEXTURE2D_DESC          surrogateDesc = { };
      surface.render.tex->GetDesc (&surrogateDesc);

      if (!(surrogateDesc.BindFlags & D3D11_BIND_RENDER_TARGET) ||
            SK_ComPtr <ID3D11Texture2D> (surface.render.tex.p).IsEqualObject (pSrcTex)                     ||
            SK_ComPtr <ID3D11Texture2D> (surface.render.tex.p).IsEqualObject (pDstTex)                     ||
            surrogateDesc.Width            != dstTexDesc.Width            ||
            surrogateDesc.Height           != dstTexDesc.Height           ||
            surrogateDesc.SampleDesc.Count != dstTexDesc.SampleDesc.Count ||
          (!SK_DXGI_IsFormatCastable (surrogateDesc.Format,
                                      surface.desc.tex.Format)))
      {
        SK_LOGi0 (
          L"SK_D3D11_BltCopySurface Released Existing Proxy Surface Because Format Or Resolution Was Incompatible (%hs <--> %hs, (%dx%d) <--> (%dx%d))",
            SK_DXGI_FormatToStr (surrogateDesc.Format).data (), SK_DXGI_FormatToStr (surface.desc.tex.Format).data (),
                                 surrogateDesc.Width,    surrogateDesc.Height,
                              surface.desc.tex.Width, surface.desc.tex.Height
        );

        surface.render.tex.Release ();
      }
    }
  }

  // A temporary texture is needed (but we'll stash it for potential reuse)
  if (surface.render.tex.p == nullptr)
  {
    surface.desc.tex.Format =
      DirectX::MakeTypeless (surface.desc.tex.Format);

    surface.desc.tex.BindFlags |=
      D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    if ( SUCCEEDED (
           pDev->CreateTexture2D ( &surface.desc.tex,
                                     nullptr, &surface.render.tex.p )
         )
       )
    {
      surface.render.tex->SetEvictionPriority (DXGI_RESOURCE_PRIORITY_LOW);

      SK_D3D11_SetDebugName ( surface.render.tex,
        L"[SK] D3D11 BltCopy I/O Stage" );

      pSrcTex->SetPrivateDataInterface (
        SKID_D3D11_CachedBltCopySrc, surface.render.tex.p );
    }
  }

  if ( surface.render.tex.p == nullptr
    || FAILED (pDev->CreateRenderTargetView (surface.render.tex, &surface.desc.rtv, &surface.render.rtv.p)) )
  {
    if (surface.render.tex.p == nullptr) SK_LOGi0 ( L"SK_D3D11_BltCopySurface Failed: surface.render.tex.p == nullptr" );
    else                                 SK_LOGi0 ( L"SK_D3D11_BltCopySurface Failed: CreateRTV Failed" );

    return
      _Return (false);
  }

  SK_ComPtr <ID3D11Texture2D>
    pNewSrcTex =
       pSrcTex;


  // A Temporary Copy May Be Needed (as above)
  if (! (srcTexDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE))
  {
    size = sizeof (void *);

    if ( SUCCEEDED (
           pDstTex->GetPrivateData ( SKID_D3D11_CachedBltCopyDst,
                                       &size, &surface.render.tex2.p )
         )
       )
    {
      D3D11_TEXTURE2D_DESC           surrogateDesc = { };
      surface.render.tex2->GetDesc (&surrogateDesc);

      if (!(surrogateDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) ||
            SK_ComPtr <ID3D11Texture2D> (surface.render.tex2.p).IsEqualObject (pSrcTex) ||
            SK_ComPtr <ID3D11Texture2D> (surface.render.tex2.p).IsEqualObject (pDstTex) ||
            surrogateDesc.Width   != surface.desc.tex2.Width      ||
            surrogateDesc.Height  != surface.desc.tex2.Height     ||
            (! SK_DXGI_IsFormatCastable (surrogateDesc.Format,
                                         surface.desc.tex2.Format)))
      {
        surface.render.tex2.Release ();

        SK_LOGi0 (
          L"SK_D3D11_BltCopySurface Released Existing Proxy Surface Because Format Or Resolution Was Incompatible (%hs <--> %hs, (%dx%d) <--> (%dx%d))",
              SK_DXGI_FormatToStr (surrogateDesc.Format).data (), SK_DXGI_FormatToStr (surface.desc.tex2.Format).data (),
                                   surrogateDesc.Width,     surrogateDesc.Height,
                               surface.desc.tex2.Width, surface.desc.tex2.Height
        );
      }
    }

    surface.desc.tex2.BindFlags |=
       D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

    surface.desc.tex2.Format =
      DirectX::MakeTypeless (surface.desc.tex2.Format);

    if ( surface.render.tex2.p == nullptr &&
          SUCCEEDED (
            pDev->CreateTexture2D ( &surface.desc.tex2,
                                       nullptr, &surface.render.tex2.p )
          )
       )
    {
      surface.render.tex2->SetEvictionPriority (DXGI_RESOURCE_PRIORITY_LOW);

      SK_D3D11_SetDebugName ( surface.render.tex2,
        L"[SK] D3D11 BltCopy SrcTex (SRV-COPY)" );

      pDstTex->SetPrivateDataInterface (
        SKID_D3D11_CachedBltCopyDst, surface.render.tex2.p);
    }

    pNewSrcTex =
      surface.render.tex2.p;

    if (pNewSrcTex == nullptr) {
      SK_LOGi0 ( L"SK_D3D11_BltCopySurface Failed: pNewSrcTex == nullptr" );
      return
        _Return (false);
    }

    pDevCtx->CopyResource (pNewSrcTex, pSrcTex);
  }

  if ( FAILED (
         pDev->CreateShaderResourceView (
             pNewSrcTex, &surface.desc.srv,
                         &surface.source.srv.p ) )
     )
  {
    SK_LOGi0 ( L"SK_D3D11_BltCopySurface Failed: CreateSRV Failed" );

    return
      _Return (false);
  }

  if (config.system.log_level > 0)
  {
    SK_D3D11_SetDebugName (surface.render.rtv, L"[SK] D3D11 BltCopy RTV (Dst)");
    SK_D3D11_SetDebugName (surface.source.srv, L"[SK] D3D11 BltCopy SRV (Src)");
  }

  D3D11_MAPPED_SUBRESOURCE
        mapped_resource = { };

  if ( SUCCEEDED (
         pDevCtx->Map ( codec.pVSUtilLuma,
                          0, D3D11_MAP_WRITE_DISCARD, 0,
                             &mapped_resource )
                 )
     )
  {
    HDR_LUMINANCE luma = { };

    _ReadWriteBarrier ();

    memcpy ( static_cast <HDR_LUMINANCE *> (mapped_resource.pData),
           &luma, sizeof (HDR_LUMINANCE) );

    pDevCtx->Unmap (codec.pVSUtilLuma, 0);
  }

  else
  {
    return
      _Return (false);
  }

  if ( SUCCEEDED (
         pDevCtx->Map ( codec.pSRGBParams,
                          0, D3D11_MAP_WRITE_DISCARD, 0,
                             &mapped_resource )
                 )
     )
  {
    SK_DXGI_sRGBCoDec::params_cbuffer_s
      codec_params = { .passthrough = 1,
                       .apply       = FALSE,
                       .strip       = FALSE,
                       .no_alpha    =
                         (! DirectX::HasAlpha (srcTexDesc.Format)) };

    _ReadWriteBarrier ();

    memcpy (    static_cast <SK_DXGI_sRGBCoDec::params_cbuffer_s *> (mapped_resource.pData),
       &codec_params, sizeof(SK_DXGI_sRGBCoDec::params_cbuffer_s));

    pDevCtx->Unmap (codec.pSRGBParams, 0);
  }

  else
  {
    return
      _Return (false);
  }

#if 0
  SK_IMGUI_D3D11StateBlock
    sb;
    sb.Capture (pDevCtx);
#else
  D3DX11_STATE_BLOCK sblock = { };
  auto *sb =        &sblock;

  CreateStateblock (pDevCtx, sb);
#endif

  const D3D11_VIEWPORT vp = {         0.0f, 0.0f,
    static_cast <float> (dstTexDesc.Width),
    static_cast <float> (dstTexDesc.Height),
                                       0.0f, 1.0f
  };

  pDevCtx->IASetPrimitiveTopology (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  pDevCtx->IASetVertexBuffers     (0, 1, std::array <ID3D11Buffer *, 1> { nullptr }.data (),
                                         std::array <UINT,           1> { 0       }.data (),
                                         std::array <UINT,           1> { 0       }.data ());
  pDevCtx->IASetInputLayout       (        codec.pInputLayout     );
  pDevCtx->IASetIndexBuffer       (nullptr, DXGI_FORMAT_UNKNOWN, 0);

  pDevCtx->OMSetBlendState        (        codec.pBlendState,
                                           codec.fBlendFactor,   0xFFFFFFFFU);
  pDevCtx->OMSetDepthStencilState (        codec.pDSState,       0);
  pDevCtx->OMSetRenderTargets     (1,
                                     &surface.render.rtv.p,
                                       nullptr);

  pDevCtx->VSSetShader            (        codec.VertexShader_Util.shader,       nullptr, 0);
  pDevCtx->VSSetConstantBuffers   (0, 1,  &codec.pVSUtilLuma   );
  pDevCtx->PSSetShader            (        codec.PixelShader_sRGB_NoMore.shader, nullptr, 0);
  pDevCtx->PSSetConstantBuffers   (0, 1,  &codec.pSRGBParams   );
  pDevCtx->PSSetShaderResources   (0, 1,  &surface.source.srv.p);
  pDevCtx->PSSetSamplers          (0, 1,  &codec.pSampler      );

  pDevCtx->RSSetState             (codec.pRasterState);
  pDevCtx->RSSetScissorRects      (0,         nullptr);
  pDevCtx->RSSetViewports         (1,             &vp);

  if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_10_0)
  {
    if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_11_0)
    {
      pDevCtx->HSSetShader        (nullptr, nullptr,       0);
      pDevCtx->DSSetShader        (nullptr, nullptr,       0);
    }
    pDevCtx->GSSetShader          (nullptr, nullptr,       0);
    pDevCtx->SOSetTargets         (0,       nullptr, nullptr);
  }

  // Implement partial copies using scissor
  //
  if (pSrcBox != nullptr)
  {
    D3D11_RECT scissor_rect = {};
    scissor_rect.left   = pSrcBox->left;
    scissor_rect.right  = pSrcBox->right;
    scissor_rect.top    = pSrcBox->top;
    scissor_rect.bottom = pSrcBox->bottom;

    pDevCtx->RSSetScissorRects (1, &scissor_rect);
  }

  pDevCtx->Draw                   (3, 0);

  SK_ComQIPtr <ID3D11DeviceContext1> pDevCtx1 (pDevCtx);

  bool bUseDiscard =
    (pDevCtx1.p != nullptr && codec.FeatureOpts.DiscardAPIsSeenByDriver);

  // If there are any mipmaps, copy them first...
  if (dstTexDesc.MipLevels > 1)
  {
    for ( UINT i = 1 ; i < dstTexDesc.MipLevels ; ++i )
    {
      const D3D11_VIEWPORT vp_sub = {         0.0f, 0.0f,
        std::max (
           static_cast <float> (dstTexDesc.Width  >> i), 1.0f),
        std::max (
           static_cast <float> (dstTexDesc.Height >> i), 1.0f),
                                              0.0f, 1.0f
      };

      D3D11_RENDER_TARGET_VIEW_DESC   rtv_dst = {};
      D3D11_SHADER_RESOURCE_VIEW_DESC srv_src = {};

      surface.source.srv->GetDesc   (&srv_src);
      surface.render.rtv->GetDesc   (&rtv_dst);

      srv_src.Texture2D.MostDetailedMip = i;
      srv_src.Texture2D.MipLevels       = 1;
      rtv_dst.Texture2D.MipSlice        = i;

      SK_ComPtr <ID3D11ShaderResourceView> pSrvMipLod;
      SK_ComPtr <ID3D11RenderTargetView>   pRtvMipLod;

      SK_ComPtr <ID3D11Resource> pSrcRes;
      SK_ComPtr <ID3D11Resource> pDstRes;

      surface.source.srv->GetResource (&pSrcRes.p);
      surface.render.rtv->GetResource (&pDstRes.p);

      pDev->CreateShaderResourceView ( pSrcRes.p,
                                         &srv_src,
                                           &pSrvMipLod );

      pDev->CreateRenderTargetView ( pDstRes.p,
                                       &rtv_dst,
                                         &pRtvMipLod );

      if ( pSrvMipLod != nullptr &&
           pRtvMipLod != nullptr )
      {
        pDevCtx->OMSetRenderTargets ( 1,
                                        &pRtvMipLod.p,
                                          nullptr );

        pDevCtx->PSSetShaderResources ( 0, 1,
                                          &pSrvMipLod.p );

        pDevCtx->RSSetViewports (1, &vp_sub);
        pDevCtx->Draw           (3, 0);

        if (pDstTex != nullptr && surface.render.tex != nullptr)
        {
          pDevCtx->CopySubresourceRegion ( pDstTex, i, 0, 0, 0,
                                            surface.render.tex, i,
                                              nullptr );
        }
      }
    }
  }

  if (pDstTex != nullptr && surface.render.tex != nullptr)
  {
    // Copy the top-level
    pDevCtx->CopySubresourceRegion (pDstTex, 0, 0, 0, 0, surface.render.tex, 0, nullptr);

    if (pDevCtx1 != nullptr && bUseDiscard)
        pDevCtx1->DiscardResource (surface.render.tex);
  }

  if (pDevCtx1 != nullptr && bUseDiscard)
      pDevCtx1->DiscardResource (pNewSrcTex);

  ApplyStateblock (pDevCtx, sb);
  //sb.Apply (pDevCtx.p);

  return
    _Return (true);
}

// Check if SK is responsible for this resource having a different
//   format than the underlying software initially requested/expects
HRESULT
SK_D3D11_CheckResourceFormatManipulation ( ID3D11Resource* pRes,
                                           DXGI_FORMAT     expected )
{
  if (pRes == nullptr)
    return E_POINTER;

  DXGI_FORMAT overrideFormat = DXGI_FORMAT_UNKNOWN;
  UINT        formatSize     = sizeof (DXGI_FORMAT);

  if ( SUCCEEDED (
         pRes->GetPrivateData ( SKID_D3D11_ResourceFormatOverride,
                                  &formatSize, &overrideFormat ) ) )
  {
    return
      ( overrideFormat != expected ) ?
                  E_UNSUPPORTED_TYPE : S_OK;
  }

  return
    S_OK;
}

void
SK_D3D11_FlagResourceFormatManipulated ( ID3D11Resource* pRes,
                                         DXGI_FORMAT     original )
{
  SK_ReleaseAssert (original != DXGI_FORMAT_UNKNOWN);

  if (pRes == nullptr)
    return;

  UINT formatSize =
    sizeof (DXGI_FORMAT);

  pRes->SetPrivateData ( SKID_D3D11_ResourceFormatOverride,
                           formatSize, (const void *)&original );
}

bool
SK_D3D11_AreTexturesDirectCopyable (D3D11_TEXTURE2D_DESC* pSrcDesc, D3D11_TEXTURE2D_DESC* pDstDesc)
{
  if (pSrcDesc == nullptr || pDstDesc == nullptr)
    return false;

  if (pSrcDesc->Width  != pDstDesc->Width ||
      pSrcDesc->Height != pDstDesc->Height)
  {
    return false;
  }

  if (! SK_D3D11_IsDirectCopyCompatible (pSrcDesc->Format, pDstDesc->Format))
    return false;

  if (pSrcDesc->SampleDesc.Count != pDstDesc->SampleDesc.Count)
    return false;

  return true;
}

bool
SK_D3D11_CheckForMatchingDevicesUsingPrivateData ( ID3D11Device *pDevice0,
                                                   ID3D11Device *pDevice1 )
{
  if (pDevice0 == nullptr ||
      pDevice1 == nullptr)
  {
    return false;
  }

  bool matching = false;

  uintptr_t  ptr0 = 0,
             ptr1 = 0;
  UINT      size0 = sizeof (uintptr_t),
            size1 = sizeof (uintptr_t);

  pDevice0->GetPrivateData (SKID_D3D11DeviceBasePtr, &size0, &ptr0);
  pDevice1->GetPrivateData (SKID_D3D11DeviceBasePtr, &size1, &ptr1);

  matching =
    ( ptr0 == ptr1 ) && ptr0 != 0;

  return matching;
}

bool
SK_D3D11_EnsureMatchingDevices (ID3D11Device *pDevice0, ID3D11Device *pDevice1)
{
  if (pDevice0 == nullptr || pDevice1 == nullptr)
    return false;

  return
    ( pDevice0 == pDevice1 ) ||
      SK_D3D11_CheckForMatchingDevicesUsingPrivateData (pDevice0, pDevice1);
}

bool
SK_D3D11_EnsureMatchingDevices (ID3D11DeviceChild *pDeviceChild, ID3D11Device *pDevice)
{
  if (pDeviceChild == nullptr || pDevice == nullptr)
    return false;

  ID3D11Device* dev_ptr = 0;
  UINT          size    = sizeof (uintptr_t);

  if (FAILED (pDeviceChild->GetPrivateData (SKID_D3D11DeviceBasePtr, &size, &dev_ptr)))
  {
    SK_ComPtr <ID3D11Device>                                                    pParentDevice;
    pDeviceChild->GetDevice                                                   (&pParentDevice.p);
    pDeviceChild->SetPrivateData (SKID_D3D11DeviceBasePtr, sizeof (uintptr_t), &pParentDevice.p);

    dev_ptr = pParentDevice.p;
  }

  return
    dev_ptr ==           pDevice ||
      SK_D3D11_CheckForMatchingDevicesUsingPrivateData (
                dev_ptr, pDevice );
}

bool
SK_D3D11_EnsureMatchingDevices (IDXGISwapChain *pSwapChain, ID3D11Device *pDevice)
{
  if (pSwapChain == nullptr || pDevice == nullptr)
    return false;

  SK_ComPtr <ID3D11Device>                             pSwapChainDevice;
  pSwapChain->GetDevice   (IID_ID3D11Device, (void **)&pSwapChainDevice);

  return
    pSwapChainDevice.p      ==      pDevice  ||
    pSwapChainDevice.IsEqualObject (pDevice) ||
      SK_D3D11_CheckForMatchingDevicesUsingPrivateData (
                pSwapChainDevice.p, pDevice );
}


// Classifies a swapchain as dummy (used by some libraries during init) or
//   real (potentially used to do actual rendering).
bool
SK_DXGI_IsSwapChainReal (const DXGI_SWAP_CHAIN_DESC& desc) noexcept
{
  // 0x0 is implicitly sized to match its HWND's client rect,
  //
  //   => Anything ( 1x1 - 31x31 ) has no practical application.
  //
#if 0
  if (desc.BufferDesc.Height > 0 && desc.BufferDesc.Height < 32)
    return false;
  if (desc.BufferDesc.Width > 0  && desc.BufferDesc.Width  < 32)
    return false;
#endif

  bool dummy_window =
    SK_Win32_IsDummyWindowClass (desc.OutputWindow);

  wchar_t   wszClass [64] = { };
  RealGetWindowClassW
    ( desc.OutputWindow, wszClass, 63 );

  if (dummy_window)
    SK_LOGi0 ( L"Ignoring SwapChain associated with Window Class: %ws",
                 wszClass );
  else
    SK_LOGi1 ( L"Typical SwapChain Associated with Window Class: %ws",
                 wszClass );

  return
    (! dummy_window);
}

bool
SK_DXGI_IsSwapChainReal1 (const DXGI_SWAP_CHAIN_DESC1& desc, HWND OutputWindow) noexcept
{
  (void)desc;

  wchar_t              wszClass [64] = { };
  RealGetWindowClassW (
         OutputWindow, wszClass, 63);

  bool dummy_window =
    SK_Win32_IsDummyWindowClass (OutputWindow);

  if (dummy_window)
    SK_LOGi0 ( L"Ignoring SwapChain associated with Window Class: %ws",
                 wszClass );
  else
    SK_LOGi1 ( L"Typical SwapChain Associated with Window Class: %ws",
                 wszClass );

  return
    (! dummy_window);
}

bool
SK_DXGI_IsSwapChainReal (IDXGISwapChain *pSwapChain) noexcept
{
  if (! pSwapChain)
    return false;

  DXGI_SWAP_CHAIN_DESC       desc = { };
       pSwapChain->GetDesc (&desc);
  return
    SK_DXGI_IsSwapChainReal (desc);
}

#include <SpecialK/render/dxgi/dxgi_swapchain.h>

HRESULT
SK_DXGI_GetPrivateData ( IDXGIObject *pObject,
                   const GUID        &kName,
                         UINT        uiMaxBytes,
                         void        *pPrivateData ) noexcept
{
  if (pObject == nullptr)
    return E_POINTER;

  UINT size = 0;

  if (SUCCEEDED (pObject->GetPrivateData (kName, &size, nullptr)))
  {
    if (size <= uiMaxBytes)
    {
      return
        pObject->GetPrivateData (kName, &size, pPrivateData);
    }

    return
      DXGI_ERROR_MORE_DATA;
  }

  return
    DXGI_ERROR_NOT_FOUND;
}

HRESULT
SK_DXGI_SetPrivateData ( IDXGIObject *pObject,
                      const GUID     &kName,
                            UINT     uiNumBytes,
                            void     *pPrivateData ) noexcept
{
  if (pObject == nullptr)
    return E_POINTER;

  return
    pObject->SetPrivateData (kName, uiNumBytes, pPrivateData);
}

template <>
HRESULT
SK_DXGI_GetPrivateData ( IDXGIObject *pObject,
   IWrapDXGISwapChain::state_cache_s *pPrivateData ) noexcept
{
  if (pObject == nullptr)
    return E_POINTER;

  return
    SK_DXGI_GetPrivateData ( pObject, SKID_DXGI_SwapChain_StateCache,
      sizeof (IWrapDXGISwapChain::state_cache_s),
                             pPrivateData );
}

template <>
HRESULT
SK_DXGI_SetPrivateData ( IDXGIObject *pObject,
   IWrapDXGISwapChain::state_cache_s *pPrivateData ) noexcept
{
  if (pObject == nullptr)
    return E_POINTER;

  return
    SK_DXGI_SetPrivateData ( pObject, SKID_DXGI_SwapChain_StateCache,
      sizeof (IWrapDXGISwapChain::state_cache_s),
                             pPrivateData );
}