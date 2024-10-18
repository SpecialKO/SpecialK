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

#include <SpecialK/render/dxgi/dxgi_util.h>
#include <SpecialK/render/d3d11/d3d11_tex_mgr.h>
#include <SpecialK/render/d3d11/d3d11_state_tracker.h>

//using D3D11On12CreateDevice_pfn =
//  HRESULT (WINAPI *)(              _In_ IUnknown*             pDevice,
//                                        UINT                  Flags,
//  _In_reads_opt_( FeatureLevels ) CONST D3D_FEATURE_LEVEL*    pFeatureLevels,
//                                        UINT                  FeatureLevels,
//            _In_reads_opt_( NumQueues ) IUnknown* CONST*      ppCommandQueues,
//                                        UINT                  NumQueues,
//                                        UINT                  NodeMask,
//                       _COM_Outptr_opt_ ID3D11Device**        ppDevice,
//                       _COM_Outptr_opt_ ID3D11DeviceContext** ppImmediateContext,
//                       _Out_opt_        D3D_FEATURE_LEVEL*    pChosenFeatureLevel );


extern "C" FARPROC D3D11CreateDeviceForD3D12              = nullptr;
extern "C" FARPROC CreateDirect3D11DeviceFromDXGIDevice   = nullptr;
extern "C" FARPROC CreateDirect3D11SurfaceFromDXGISurface = nullptr;
extern "C" D3D11On12CreateDevice_pfn
                   D3D11On12CreateDevice                  = nullptr;
extern "C" FARPROC EnableFeatureLevelUpgrade              = nullptr;
extern "C" FARPROC OpenAdapter10                          = nullptr;
extern "C" FARPROC OpenAdapter10_2                        = nullptr;
extern "C" FARPROC D3D11CoreCreateLayeredDevice           = nullptr;
extern "C" FARPROC D3D11CoreGetLayeredDeviceSize          = nullptr;
extern "C" FARPROC D3D11CoreRegisterLayers                = nullptr;
extern "C" FARPROC D3DPerformance_BeginEvent              = nullptr;
extern "C" FARPROC D3DPerformance_EndEvent                = nullptr;
extern "C" FARPROC D3DPerformance_GetStatus               = nullptr;
extern "C" FARPROC D3DPerformance_SetMarker               = nullptr;

FARPROC
SK_GetProcAddressD3D11 (const char* lpProcName)
{
  extern HMODULE SK::DXGI::hModD3D11;

  if (StrStrA (lpProcName, "D3DKMT") == lpProcName)
  {
    return SK_GetProcAddress (SK::DXGI::hModD3D11, lpProcName);
  }

  if (! StrCmpA (lpProcName, "D3D11CreateDeviceForD3D12"))
    return SK_GetProcAddress (SK::DXGI::hModD3D11, lpProcName);
  if (! StrCmpA (lpProcName, "CreateDirect3D11DeviceFromDXGIDevice"))
    return SK_GetProcAddress (SK::DXGI::hModD3D11, lpProcName);
  if (! StrCmpA (lpProcName, "CreateDirect3D11SurfaceFromDXGISurface"))
    return SK_GetProcAddress (SK::DXGI::hModD3D11, lpProcName);
  if (! StrCmpA (lpProcName, "D3D11On12CreateDevice"))
    return SK_GetProcAddress (SK::DXGI::hModD3D11, lpProcName);
  if (! StrCmpA (lpProcName, "EnableFeatureLevelUpgrade"))
    return SK_GetProcAddress (SK::DXGI::hModD3D11, lpProcName);
  if (! StrCmpA (lpProcName, "OpenAdapter10"))
    return SK_GetProcAddress (SK::DXGI::hModD3D11, lpProcName);
  if (! StrCmpA (lpProcName, "OpenAdapter10_2"))
    return SK_GetProcAddress (SK::DXGI::hModD3D11, lpProcName);
  if (! StrCmpA (lpProcName, "D3D11CoreCreateLayeredDevice"))
    return SK_GetProcAddress (SK::DXGI::hModD3D11, lpProcName);
  if (! StrCmpA (lpProcName, "D3D11CoreGetLayeredDeviceSize"))
    return SK_GetProcAddress (SK::DXGI::hModD3D11, lpProcName);
  if (! StrCmpA (lpProcName, "D3D11CoreRegisterLayers"))
    return SK_GetProcAddress (SK::DXGI::hModD3D11, lpProcName);
  if (! StrCmpA (lpProcName, "D3DPerformance_BeginEvent"))
    return SK_GetProcAddress (SK::DXGI::hModD3D11, lpProcName);
  if (! StrCmpA (lpProcName, "D3DPerformance_EndEvent"))
    return SK_GetProcAddress (SK::DXGI::hModD3D11, lpProcName);
  if (! StrCmpA (lpProcName, "D3DPerformance_GetStatus"))
    return SK_GetProcAddress (SK::DXGI::hModD3D11, lpProcName);
  if (! StrCmpA (lpProcName, "D3DPerformance_SetMarker"))
    return SK_GetProcAddress (SK::DXGI::hModD3D11, lpProcName);

  return (FARPROC)-1;
}


bool
SK_D3D11_OverrideDepthStencil (DXGI_FORMAT& fmt)
{
  if (! config.render.dxgi.enhanced_depth)
    return false;

  switch (fmt)
  {
    case DXGI_FORMAT_R24G8_TYPELESS:
      fmt = DXGI_FORMAT_R32G8X24_TYPELESS;
      return true;

    case DXGI_FORMAT_D24_UNORM_S8_UINT:
      fmt = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
      return true;

    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
      fmt = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
      return true;

    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
      fmt = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
      return true;
  }

  return false;
}



__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateBuffer_Override (
  _In_           ID3D11Device            *This,
  _In_     const D3D11_BUFFER_DESC       *pDesc,
  _In_opt_ const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_      ID3D11Buffer           **ppBuffer )
{
  if (pDesc == nullptr)
    return E_INVALIDARG;

  return
    D3D11Dev_CreateBuffer_Original ( This, pDesc,
                                       pInitialData, ppBuffer );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateShaderResourceView_Override (
  _In_           ID3D11Device                     *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC  *pDesc,
  _Out_opt_      ID3D11ShaderResourceView        **ppSRView )
{
  if (pResource == nullptr)
    return E_INVALIDARG;

#ifdef _SK_D3D11_VALIDATE_DEVICE_RESOURCES
  if (pResource != nullptr)
  {
    SK_ComPtr <ID3D11Device> pActualDevice;
    pResource->GetDevice   (&pActualDevice.p);

    if (! pActualDevice.IsEqualObject (This))
    {
      SK_LOGi0 (L"D3D11 Device Hook Trying to Create SRV for a Resource"
                L"Belonging to a Different Device");
      return DXGI_ERROR_DEVICE_RESET;
    }
  }
#endif

#if 0
  if (! SK_D3D11_EnsureMatchingDevices (pResource, This))
  {
    SK_ComPtr <ID3D11Device> pResourceDev;
    pResource->GetDevice   (&pResourceDev);
  
    This = pResourceDev;
  }
#endif

  D3D11_RESOURCE_DIMENSION   dim;
  pResource->GetType       (&dim);

  if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
  {
    D3D11_SHADER_RESOURCE_VIEW_DESC desc =
    {                .Format          = DXGI_FORMAT_UNKNOWN,
                     .ViewDimension   = D3D11_SRV_DIMENSION_TEXTURE2D,
      .Texture2D = { .MostDetailedMip = 0, .MipLevels = (UINT)-1 }
    };

    if (pDesc != nullptr)
      desc = *pDesc;

    SK_ComQIPtr <ID3D11Texture2D>
        pTex (pResource);
    if (pTex != nullptr)
    {
      D3D11_TEXTURE2D_DESC texDesc = { };
      pTex->GetDesc      (&texDesc);

      // Ys 8 crashes if this nonsense isn't here
      bool bInvalidType =
        (pDesc != nullptr && DirectX::IsTypeless (pDesc->Format, false));

      // Fix-up SRV's created using NULL desc's on Typeless SwapChain Backbuffers, or using DXGI_FORMAT_UNKNOWN
      if (DirectX::IsTypeless (texDesc.Format) && (pDesc == nullptr || pDesc->Format == DXGI_FORMAT_UNKNOWN || DirectX::IsTypeless (pDesc->Format)) &&
                              (texDesc.BindFlags & D3D11_BIND_RENDER_TARGET))
      {
        if (pDesc == nullptr)
            pDesc  = &desc;

        desc.Format = DirectX::MakeTypelessUNORM (
                      DirectX::MakeTypelessFLOAT (texDesc.Format));

        bInvalidType = true;
      }

      // SK only overrides the format of RenderTargets, anything else is not our fault.
      if ( bInvalidType || FAILED (SK_D3D11_CheckResourceFormatManipulation (pTex, desc.Format)) )
      {
        if (texDesc.SampleDesc.Count > 1)
        {
          if (desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2DARRAY) desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
          else                                                          desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
        }

        else
        {
          if (desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY) desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
          else                                                            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        }

        if (                                                            bInvalidType ||
             (                                   (desc.Format != DXGI_FORMAT_UNKNOWN || DirectX::IsTypeless (texDesc.Format, false)) /*&&
              (! SK_D3D11_IsDirectCopyCompatible (desc.Format,                                               texDesc.Format))*/)
           )
        {
          DXGI_FORMAT swapChainFormat = DXGI_FORMAT_UNKNOWN;
          UINT        sizeOfFormat    = sizeof (DXGI_FORMAT);

          pTex->GetPrivateData (
            SKID_DXGI_SwapChainBackbufferFormat,
                                  &sizeOfFormat,
                               &swapChainFormat );

          if (swapChainFormat != DXGI_FORMAT_UNKNOWN)
          {
            // Unsure how to handle arrays of SwapChain backbuffers... I don't think D3D11 supports that
            SK_ReleaseAssert ( desc.ViewDimension != D3D11_SRV_DIMENSION_TEXTURE2DARRAY &&
                               desc.ViewDimension != D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY );

            SK_LOGi1 (
              L"SRV Format Override (Requested=%hs), (Returned=%hs) - [SwapChain]",
                SK_DXGI_FormatToStr (    desc.Format).data (),
                SK_DXGI_FormatToStr (swapChainFormat).data () );

            desc.Format =
              swapChainFormat;
          }

          // Not a SwapChain Backbuffer, so probably had its format changed for remastering
          else
          {
            SK_LOGi1 (
              L"SRV Format Override (Requested=%hs), (Returned=%hs) - Non-SwapChain Override",
                SK_DXGI_FormatToStr (   desc.Format).data (),
                SK_DXGI_FormatToStr (texDesc.Format).data () );

            desc.Format =
              texDesc.Format;
          }

          bool bErrorCorrection = false;

          // If we got this far, the problem wasn't created by Special K, however...
          //   Special K can still try to fix it.
          if (DirectX::IsTypeless (desc.Format, false))
          {
            auto typedFormat =
              DirectX::MakeTypelessUNORM   (
                DirectX::MakeTypelessFLOAT (desc.Format));

            SK_LOGi0 (
              L"-!- Game tried to create a Typeless SRV (%hs) of a"
                L" surface ('%hs') not directly managed by Special K",
                        SK_DXGI_FormatToStr (desc.Format).data (),
                        SK_D3D11_GetDebugNameUTF8 (pTex).c_str () );
            SK_LOGi0 (
              L"<?> Attempting to fix game's mistake by converting to %hs",
                        SK_DXGI_FormatToStr (typedFormat).data () );

            desc.Format      = typedFormat;
            bErrorCorrection = true;
          }

          const HRESULT hr =
            D3D11Dev_CreateShaderResourceView_Original (
              This, pResource,
                &desc, ppSRView );

          if (SUCCEEDED (hr))
          {
            if (bErrorCorrection)
              SK_LOGi0 (L"==> [ Success ]");

            return hr;
          }
        }
      }
    }
  }

  if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D && pDesc != nullptr)
  {
    DXGI_FORMAT newFormat    = pDesc->Format;
    UINT        newMipLevels = pDesc->Texture2D.MipLevels;

    SK_ComQIPtr <ID3D11Texture2D>
        pTex2D (pResource);
    D3D11_TEXTURE2D_DESC  tex_desc = { };

    if (pTex2D != nullptr)
    {
      pTex2D->GetDesc (&tex_desc);

      bool override = false;

      if (! DirectX::IsDepthStencil (pDesc->Format))
      {
        if (                         pDesc->Format  != DXGI_FORMAT_UNKNOWN && (
            ( DirectX::BitsPerPixel (pDesc->Format) !=
              DirectX::BitsPerPixel (tex_desc.Format) ) ||
            ( DirectX::MakeTypeless (pDesc->Format) != // Handle cases such as BC3 -> BC7: Size = Same, Fmt != Same
              DirectX::MakeTypeless (tex_desc.Format) ) ||
              DirectX::IsTypeless   (pDesc->Format,
                                               false) ) && (! DirectX::IsTypeless (tex_desc.Format, false) &&
                                                           (! DirectX::IsVideo    (tex_desc.Format)))
           ) // Does not handle sRGB vs. non-sRGB, but generally the game
        {    //   won't render stuff correctly if injected textures change that.
          override  = true;
          newFormat = tex_desc.Format;
        }
      }

      if ( SK_D3D11_OverrideDepthStencil (newFormat) )
        override = true;

      if ( SK_D3D11_TextureIsCached ((ID3D11Texture2D *)pResource) )
      {
        auto& textures =
          SK_D3D11_Textures;

        auto& cache_desc =
          textures->Textures_2D [(ID3D11Texture2D *)pResource];

        // Texture may have been removed (i.e. dynamic update in Witcher 3)
        if (cache_desc.texture != nullptr)
        {
          SK_ComPtr <ID3D11Device>        pCacheDevice;
          cache_desc.texture->GetDevice (&pCacheDevice.p);

          if (pCacheDevice.IsEqualObject (This))
          {
            newFormat =
              cache_desc.desc.Format;

            newMipLevels =
              pDesc->Texture2D.MipLevels;

            if (pDesc->Format != DXGI_FORMAT_UNKNOWN &&
                DirectX::MakeTypeless (pDesc->Format) !=
                DirectX::MakeTypeless (newFormat))
            {
              if (DirectX::IsSRGB (pDesc->Format))
                newFormat = DirectX::MakeSRGB (newFormat);

              override = true;

              SK_LOG1 ((L"Overriding Resource View Format for Cached Texture '%08x'  { Was: '%hs', Now: '%hs' }",
                        cache_desc.crc32c,
                        SK_DXGI_FormatToStr (pDesc->Format).data (),
                        SK_DXGI_FormatToStr (newFormat).data ()),
                       L"DX11TexMgr");
            }

            if (config.textures.d3d11.generate_mips &&
                cache_desc.desc.MipLevels != pDesc->Texture2D.MipLevels)
            {
              override = true;
              newMipLevels = cache_desc.desc.MipLevels;

              SK_LOG1 ((L"Overriding Resource View Mip Levels for Cached Texture '%08x'  { Was: %lu, Now: %lu }",
                        cache_desc.crc32c,
                        pDesc->Texture2D.MipLevels,
                        newMipLevels),
                       L"DX11TexMgr");
            }
          }

          else if (config.system.log_level > 0)
            // TODO: Texture cache needs to be per-device
            SK_ReleaseAssert (!"Attempted to use a cached texture on the wrong device!");
        }
      }

      auto descCopy =
        *pDesc;

      // SRVs and RTVs cannot be typeless
      if (DirectX::IsTypeless (newFormat,       false) ||
          DirectX::IsTypeless (descCopy.Format, false))
      {
        if (! DirectX::IsTypeless (tex_desc.Format))
        {
          override  = true;
          newFormat = DXGI_FORMAT_UNKNOWN;
        }
      }

      if (override)
      {
        descCopy.Format = newFormat;

        if (newMipLevels != pDesc->Texture2D.MipLevels)
        {
          descCopy.Texture2D.MipLevels = sk::narrow_cast <UINT>( -1 );
          descCopy.Texture2D.MostDetailedMip = 0;
        }

        HRESULT hr =
          D3D11Dev_CreateShaderResourceView_Original ( This, pResource,
                                                         &descCopy, ppSRView );

        if (SUCCEEDED (hr))
        {
          return hr;
        }
      }
    }
  }

  return
    D3D11Dev_CreateShaderResourceView_Original ( This, pResource,
                                                   pDesc, ppSRView );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateShaderResourceView1_Override (
  _In_           ID3D11Device3                    *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC1 *pDesc,
  _Out_opt_      ID3D11ShaderResourceView1       **ppSRView )
{
  if (pResource == nullptr)
    return E_INVALIDARG;

#ifdef _SK_D3D11_VALIDATE_DEVICE_RESOURCES
  if (pResource != nullptr)
  {
    SK_ComPtr <ID3D11Device> pActualDevice;
    pResource->GetDevice   (&pActualDevice.p);

    if (! pActualDevice.IsEqualObject (This))
    {
      SK_LOGi0 (L"D3D11 Device Hook Trying to Create SRV for a Resource"
                L"Belonging to a Different Device");
      return DXGI_ERROR_DEVICE_RESET;
    }
  }
#endif

#if 0
  if (! SK_D3D11_EnsureMatchingDevices (pResource, This))
  {
    SK_ComPtr <ID3D11Device> pResourceDev;
    pResource->GetDevice   (&pResourceDev);
  
    This = pResourceDev;
  }
#endif

  D3D11_RESOURCE_DIMENSION   dim;
  pResource->GetType       (&dim);

  if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
  {
    D3D11_SHADER_RESOURCE_VIEW_DESC1 desc =
    {                .Format          = DXGI_FORMAT_UNKNOWN,
                     .ViewDimension   = D3D11_SRV_DIMENSION_TEXTURE2D,
      .Texture2D = { .MostDetailedMip = 0, .MipLevels = (UINT)-1 }
    };

    if (pDesc != nullptr)
      desc = *pDesc;

    SK_ComQIPtr <ID3D11Texture2D>
        pTex (pResource);
    if (pTex != nullptr)
    {
      D3D11_TEXTURE2D_DESC texDesc = { };
      pTex->GetDesc      (&texDesc);

      // Ys 8 crashes if this nonsense isn't here
      bool bInvalidType =
        (pDesc != nullptr && DirectX::IsTypeless (pDesc->Format, false));

      // Fix-up SRV's created using NULL desc's on Typeless SwapChain Backbuffers, or using DXGI_FORMAT_UNKNOWN
      if (DirectX::IsTypeless (texDesc.Format) && (pDesc == nullptr || pDesc->Format == DXGI_FORMAT_UNKNOWN || DirectX::IsTypeless (pDesc->Format)) &&
                              (texDesc.BindFlags & D3D11_BIND_RENDER_TARGET))
      {
        if (pDesc == nullptr)
            pDesc  = &desc;

        desc.Format = DirectX::MakeTypelessUNORM (
                      DirectX::MakeTypelessFLOAT (texDesc.Format));

        bInvalidType = true;
      }

      // SK only overrides the format of RenderTargets, anything else is not our fault.
      if ( bInvalidType || FAILED (SK_D3D11_CheckResourceFormatManipulation (pTex, desc.Format)) )
      {
        if (texDesc.SampleDesc.Count > 1)
        {
          if (desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2DARRAY) desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
          else                                                          desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
        }

        else
        {
          if (desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY) desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
          else                                                            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        }

        if (                                                            bInvalidType ||
             (                                   (desc.Format != DXGI_FORMAT_UNKNOWN || DirectX::IsTypeless (texDesc.Format, false)) /*&&
              (! SK_D3D11_IsDirectCopyCompatible (desc.Format,                                               texDesc.Format))*/)
           )
        {
          DXGI_FORMAT swapChainFormat = DXGI_FORMAT_UNKNOWN;
          UINT        sizeOfFormat    = sizeof (DXGI_FORMAT);

          pTex->GetPrivateData (
            SKID_DXGI_SwapChainBackbufferFormat,
                                  &sizeOfFormat,
                               &swapChainFormat );

          if (swapChainFormat != DXGI_FORMAT_UNKNOWN)
          {
            // Unsure how to handle arrays of SwapChain backbuffers... I don't think D3D11 supports that
            SK_ReleaseAssert ( desc.ViewDimension != D3D11_SRV_DIMENSION_TEXTURE2DARRAY &&
                               desc.ViewDimension != D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY );

            SK_LOGi1 (
              L"SRV Format Override (Requested=%hs), (Returned=%hs) - [SwapChain]",
                SK_DXGI_FormatToStr (    desc.Format).data (),
                SK_DXGI_FormatToStr (swapChainFormat).data () );

            desc.Format =
              swapChainFormat;
          }

          // Not a SwapChain Backbuffer, so probably had its format changed for remastering
          else
          {
            SK_LOGi1 (
              L"SRV Format Override (Requested=%hs), (Returned=%hs) - Non-SwapChain Override",
                SK_DXGI_FormatToStr (   desc.Format).data (),
                SK_DXGI_FormatToStr (texDesc.Format).data () );

            desc.Format =
              texDesc.Format;
          }

          bool bErrorCorrection = false;

          // If we got this far, the problem wasn't created by Special K, however...
          //   Special K can still try to fix it.
          if (DirectX::IsTypeless (desc.Format, false))
          {
            auto typedFormat =
              DirectX::MakeTypelessUNORM   (
                DirectX::MakeTypelessFLOAT (desc.Format));

            SK_LOGi0 (
              L"-!- Game tried to create a Typeless SRV (%hs) of a"
                L" surface ('%hs') not directly managed by Special K",
                        SK_DXGI_FormatToStr (desc.Format).data (),
                        SK_D3D11_GetDebugNameUTF8 (pTex).c_str () );
            SK_LOGi0 (
              L"<?> Attempting to fix game's mistake by converting to %hs",
                        SK_DXGI_FormatToStr (typedFormat).data () );

            desc.Format      = typedFormat;
            bErrorCorrection = true;
          }

          const HRESULT hr =
            D3D11Dev_CreateShaderResourceView1_Original (
              This, pResource,
                &desc, ppSRView );

          if (SUCCEEDED (hr))
          {
            if (bErrorCorrection)
              SK_LOGi0 (L"==> [ Success ]");

            return hr;
          }
        }
      }
    }
  }

  if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D && pDesc != nullptr)
  {
    DXGI_FORMAT newFormat    = pDesc->Format;
    UINT        newMipLevels = pDesc->Texture2D.MipLevels;

    SK_ComQIPtr <ID3D11Texture2D>
        pTex2D (pResource);
    D3D11_TEXTURE2D_DESC  tex_desc = { };

    if (pTex2D != nullptr)
    {
      pTex2D->GetDesc (&tex_desc);

      bool override = false;

      if (! DirectX::IsDepthStencil (pDesc->Format))
      {
        if (                         pDesc->Format  != DXGI_FORMAT_UNKNOWN && (
            ( DirectX::BitsPerPixel (pDesc->Format) !=
              DirectX::BitsPerPixel (tex_desc.Format) ) ||
            ( DirectX::MakeTypeless (pDesc->Format) != // Handle cases such as BC3 -> BC7: Size = Same, Fmt != Same
              DirectX::MakeTypeless (tex_desc.Format) ) ||
              DirectX::IsTypeless   (pDesc->Format,
                                               false) ) && (! DirectX::IsTypeless (tex_desc.Format, false) &&
                                                           (! DirectX::IsVideo    (tex_desc.Format)))
           ) // Does not handle sRGB vs. non-sRGB, but generally the game
        {    //   won't render stuff correctly if injected textures change that.
          override  = true;
          newFormat = tex_desc.Format;
        }
      }

      if ( SK_D3D11_OverrideDepthStencil (newFormat) )
        override = true;

      if ( SK_D3D11_TextureIsCached ((ID3D11Texture2D *)pResource) )
      {
        auto& textures =
          SK_D3D11_Textures;

        auto& cache_desc =
          textures->Textures_2D [(ID3D11Texture2D *)pResource];

        // Texture may have been removed (i.e. dynamic update in Witcher 3)
        if (cache_desc.texture != nullptr)
        {
          SK_ComPtr <ID3D11Device>        pCacheDevice;
          cache_desc.texture->GetDevice (&pCacheDevice.p);

          if (pCacheDevice.IsEqualObject (This))
          {
            newFormat =
              cache_desc.desc.Format;

            newMipLevels =
              pDesc->Texture2D.MipLevels;

            if (pDesc->Format != DXGI_FORMAT_UNKNOWN &&
                DirectX::MakeTypeless (pDesc->Format) !=
                DirectX::MakeTypeless (newFormat))
            {
              if (DirectX::IsSRGB (pDesc->Format))
                newFormat = DirectX::MakeSRGB (newFormat);

              override = true;

              SK_LOG1 ((L"Overriding Resource View Format for Cached Texture '%08x'  { Was: '%hs', Now: '%hs' }",
                        cache_desc.crc32c,
                        SK_DXGI_FormatToStr (pDesc->Format).data (),
                        SK_DXGI_FormatToStr (newFormat).data ()),
                       L"DX11TexMgr");
            }

            if (config.textures.d3d11.generate_mips &&
                cache_desc.desc.MipLevels != pDesc->Texture2D.MipLevels)
            {
              override = true;
              newMipLevels = cache_desc.desc.MipLevels;

              SK_LOG1 ((L"Overriding Resource View Mip Levels for Cached Texture '%08x'  { Was: %lu, Now: %lu }",
                        cache_desc.crc32c,
                        pDesc->Texture2D.MipLevels,
                        newMipLevels),
                       L"DX11TexMgr");
            }
          }

          else if (config.system.log_level > 0)
            // TODO: Texture cache needs to be per-device
            SK_ReleaseAssert (!"Attempted to use a cached texture on the wrong device!");
        }
      }

      auto descCopy =
        *pDesc;

      // SRVs and RTVs cannot be typeless
      if (DirectX::IsTypeless (newFormat,       false) ||
          DirectX::IsTypeless (descCopy.Format, false))
      {
        if (! DirectX::IsTypeless (tex_desc.Format))
        {
          override  = true;
          newFormat = DXGI_FORMAT_UNKNOWN;
        }
      }

      if (override)
      {
        descCopy.Format = newFormat;

        if (newMipLevels != pDesc->Texture2D.MipLevels)
        {
          descCopy.Texture2D.MipLevels = sk::narrow_cast <UINT>( -1 );
          descCopy.Texture2D.MostDetailedMip = 0;
        }

        HRESULT hr =
          D3D11Dev_CreateShaderResourceView1_Original ( This, pResource,
                                                          &descCopy, ppSRView );

        if (SUCCEEDED (hr))
        {
          return hr;
        }
      }
    }
  }

  return
    D3D11Dev_CreateShaderResourceView1_Original ( This, pResource,
                                                    pDesc, ppSRView );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateDepthStencilView_Override (
  _In_            ID3D11Device                  *This,
  _In_            ID3D11Resource                *pResource,
  _In_opt_  const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,
  _Out_opt_       ID3D11DepthStencilView        **ppDepthStencilView )
{
  if (pResource == nullptr)
    return E_INVALIDARG;

#ifdef _SK_D3D11_VALIDATE_DEVICE_RESOURCES
  if (pResource != nullptr)
  {
    SK_ComPtr <ID3D11Device> pActualDevice;
    pResource->GetDevice   (&pActualDevice.p);

    if (! pActualDevice.IsEqualObject (This))
    {
      SK_LOGi0 (L"D3D11 Device Hook Trying to Create DSV for a Resource"
                L"Belonging to a Different Device");
      return DXGI_ERROR_DEVICE_RESET;
    }
  }
#endif

#if 0
  if (! SK_D3D11_EnsureMatchingDevices (pResource, This))
  {
    SK_ComPtr <ID3D11Device> pResourceDev;
    pResource->GetDevice   (&pResourceDev);
  
    This = pResourceDev;
  }
#endif

  HRESULT hr =
    E_UNEXPECTED;

  if ( pDesc     != nullptr &&
       pResource != nullptr )
  {
    D3D11_RESOURCE_DIMENSION dim;
    pResource->GetType     (&dim);

    if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      DXGI_FORMAT                   newFormat (pDesc->Format);
      SK_ComQIPtr <ID3D11Texture2D> pTex      (pResource);

      if (pTex != nullptr)
      {
        D3D11_TEXTURE2D_DESC tex_desc;
             pTex->GetDesc (&tex_desc);

        auto descCopy =
          *pDesc;

        if ( SK_D3D11_OverrideDepthStencil (newFormat) || DirectX::BitsPerPixel (newFormat) !=
                                                          DirectX::BitsPerPixel (tex_desc.Format)  )
        {
          if (                        newFormat  != DXGI_FORMAT_UNKNOWN &&
               DirectX::BitsPerPixel (newFormat) !=
               DirectX::BitsPerPixel (tex_desc.Format) )
          {
            newFormat = tex_desc.Format;
          }

          descCopy.Format = newFormat;

          hr =
            D3D11Dev_CreateDepthStencilView_Original (
              This, pResource,
                &descCopy,
                  ppDepthStencilView
            );
        }

        if (SUCCEEDED (hr))
          return hr;
      }
    }
  }

  hr =
    D3D11Dev_CreateDepthStencilView_Original ( This, pResource,
                                                 pDesc, ppDepthStencilView );
  return hr;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateUnorderedAccessView_Override (
  _In_            ID3D11Device                     *This,
  _In_            ID3D11Resource                   *pResource,
  _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc,
  _Out_opt_       ID3D11UnorderedAccessView       **ppUAView )
{
  if (pResource == nullptr)
    return E_INVALIDARG;

#if 0
  if (pResource != nullptr)
  {
    SK_ComPtr <ID3D11Device> pActualDevice;
    pResource->GetDevice   (&pActualDevice.p);

    if (! pActualDevice.IsEqualObject (This))
    {
      SK_LOGi0 (L"D3D11 Device Hook Trying to Create UAV for a Resource"
                L"Belonging to a Different Device");
      return DXGI_ERROR_DEVICE_RESET;
    }
  }
#endif

#if 0
  if (! SK_D3D11_EnsureMatchingDevices (pResource, This))
  {
    SK_ComPtr <ID3D11Device> pResourceDev;
    pResource->GetDevice   (&pResourceDev);
  
    This = pResourceDev;
  }
#endif

  if (SK_ComQIPtr <ID3D11Texture2D> pTex (pResource);
                                    pTex != nullptr)
  {
    D3D11_RESOURCE_DIMENSION dim;
    pResource->GetType     (&dim);

    if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      D3D11_UNORDERED_ACCESS_VIEW_DESC desc =
      { .Format         = DXGI_FORMAT_UNKNOWN,
        .ViewDimension  = D3D11_UAV_DIMENSION_TEXTURE2D,
        .Texture2D      = { .MipSlice = 0 }
      };

      if (pDesc != nullptr)
        desc = *pDesc;

      D3D11_TEXTURE2D_DESC tex_desc = { };
      pTex->GetDesc      (&tex_desc);

      DXGI_FORMAT newFormat = desc.Format;
      bool         override = false;

      // Fix-up UAV's created using NULL desc's on Typeless SwapChain Backbuffers, or using DXGI_FORMAT_UNKNOWN
      if (DirectX::IsTypeless (tex_desc.Format) && (pDesc == nullptr || pDesc->Format == DXGI_FORMAT_UNKNOWN || DirectX::IsTypeless (pDesc->Format)) &&
                              (tex_desc.BindFlags & D3D11_BIND_RENDER_TARGET))
      {
        if (pDesc == nullptr)
          pDesc    = &desc;

        desc.Format = DirectX::MakeTypelessUNORM (
                      DirectX::MakeTypelessFLOAT (tex_desc.Format));

        newFormat = desc.Format;
        override  = true;
      }

      if ( SK_D3D11_OverrideDepthStencil (newFormat) )
        override = true;

      if ( SK_D3D11_TextureIsCached ((ID3D11Texture2D *)pResource) )
      {
        auto& textures =
          SK_D3D11_Textures;

        auto& cache_desc =
          textures->Textures_2D [(ID3D11Texture2D *)pResource];

        newFormat =
          cache_desc.desc.Format;

        if (                                             pDesc != nullptr &&
                                    pDesc->Format  != DXGI_FORMAT_UNKNOWN &&
             DirectX::MakeTypeless (pDesc->Format) !=
             DirectX::MakeTypeless (newFormat    )  )
        {
          if (DirectX::IsSRGB (pDesc->Format))
            newFormat = DirectX::MakeSRGB (newFormat);

          override = true;

          if (config.system.log_level > 0)
            SK_ReleaseAssert (override == false && L"UAV Format Override Needed");

          SK_LOG1 ( ( L"Overriding Unordered Access View Format for Cached Texture '%08x'  { Was: '%hs', Now: '%hs' }",
                        cache_desc.crc32c,
                   SK_DXGI_FormatToStr (pDesc->Format).data      (),
                            SK_DXGI_FormatToStr (newFormat).data () ),
                      L"DX11TexMgr" );
        }
      }

      if (                        pDesc == nullptr ||
                                 (pDesc->Format  != DXGI_FORMAT_UNKNOWN &&
           DirectX::MakeTypeless (pDesc->Format) !=
           DirectX::MakeTypeless (tex_desc.Format)) )
      {
        override  = true;

        if (! DirectX::IsTypeless (tex_desc.Format))
          newFormat = DXGI_FORMAT_UNKNOWN; // Inherit resource's format
        else // Guess the appropriate format
          newFormat = DirectX::MakeTypelessUNORM (
                      DirectX::MakeTypelessFLOAT (tex_desc.Format));
      }

      if (override)
      {
        auto descCopy =
          (pDesc != nullptr) ?
          *pDesc             : desc;

        descCopy.Format = newFormat;

        const HRESULT hr =
          D3D11Dev_CreateUnorderedAccessView_Original ( This, pResource,
                                                          &descCopy, ppUAView );

        if (SUCCEEDED (hr))
          return hr;
      }
    }
  }

  const HRESULT hr =
    D3D11Dev_CreateUnorderedAccessView_Original ( This, pResource,
                                                    pDesc, ppUAView );
  return hr;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateUnorderedAccessView1_Override (
  _In_            ID3D11Device3                     *This,
  _In_            ID3D11Resource                    *pResource,
  _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC1 *pDesc,
  _Out_opt_       ID3D11UnorderedAccessView1       **ppUAView )
{
  if (pResource == nullptr)
    return E_INVALIDARG;

#if 0
  if (pResource != nullptr)
  {
    SK_ComPtr <ID3D11Device> pActualDevice;
    pResource->GetDevice   (&pActualDevice.p);

    if (! pActualDevice.IsEqualObject (This))
    {
      SK_LOGi0 (L"D3D11 Device Hook Trying to Create UAV for a Resource"
                L"Belonging to a Different Device");
      return DXGI_ERROR_DEVICE_RESET;
    }
  }
#endif

#if 0
  if (! SK_D3D11_EnsureMatchingDevices (pResource, This))
  {
    SK_ComPtr <ID3D11Device> pResourceDev;
    pResource->GetDevice   (&pResourceDev);
  
    This = pResourceDev;
  }
#endif

  if (SK_ComQIPtr <ID3D11Texture2D> pTex (pResource);
                                    pTex != nullptr)
  {
    D3D11_RESOURCE_DIMENSION dim;
    pResource->GetType     (&dim);

    if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      D3D11_UNORDERED_ACCESS_VIEW_DESC1 desc =
      { .Format         = DXGI_FORMAT_UNKNOWN,
        .ViewDimension  = D3D11_UAV_DIMENSION_TEXTURE2D,
        .Texture2D      = { .MipSlice = 0 }
      };

      if (pDesc != nullptr)
        desc = *pDesc;

      D3D11_TEXTURE2D_DESC tex_desc = { };
      pTex->GetDesc      (&tex_desc);

      DXGI_FORMAT newFormat = desc.Format;
      bool         override = false;

      // Fix-up UAV's created using NULL desc's on Typeless SwapChain Backbuffers, or using DXGI_FORMAT_UNKNOWN
      if (DirectX::IsTypeless (tex_desc.Format) && (pDesc == nullptr || pDesc->Format == DXGI_FORMAT_UNKNOWN || DirectX::IsTypeless (pDesc->Format)) &&
                              (tex_desc.BindFlags & D3D11_BIND_RENDER_TARGET))
      {
        if (pDesc == nullptr)
          pDesc    = &desc;

        desc.Format = DirectX::MakeTypelessUNORM (
                      DirectX::MakeTypelessFLOAT (tex_desc.Format));

        newFormat = desc.Format;
        override  = true;
      }

      if ( SK_D3D11_OverrideDepthStencil (newFormat) )
        override = true;

      if ( SK_D3D11_TextureIsCached ((ID3D11Texture2D *)pResource) )
      {
        auto& textures =
          SK_D3D11_Textures;

        auto& cache_desc =
          textures->Textures_2D [(ID3D11Texture2D *)pResource];

        newFormat =
          cache_desc.desc.Format;

        if (                                             pDesc != nullptr &&
                                    pDesc->Format  != DXGI_FORMAT_UNKNOWN &&
             DirectX::MakeTypeless (pDesc->Format) !=
             DirectX::MakeTypeless (newFormat    )  )
        {
          if (DirectX::IsSRGB (pDesc->Format))
            newFormat = DirectX::MakeSRGB (newFormat);

          override = true;

          if (config.system.log_level > 0)
            SK_ReleaseAssert (override == false && L"UAV Format Override Needed");

          SK_LOG1 ( ( L"Overriding Unordered Access View Format for Cached Texture '%08x'  { Was: '%hs', Now: '%hs' }",
                        cache_desc.crc32c,
                   SK_DXGI_FormatToStr (pDesc->Format).data      (),
                            SK_DXGI_FormatToStr (newFormat).data () ),
                      L"DX11TexMgr" );
        }
      }

      if (                        pDesc == nullptr ||
                                 (pDesc->Format  != DXGI_FORMAT_UNKNOWN &&
           DirectX::MakeTypeless (pDesc->Format) !=
           DirectX::MakeTypeless (tex_desc.Format)) )
      {
        override  = true;

        if (! DirectX::IsTypeless (tex_desc.Format))
          newFormat = DXGI_FORMAT_UNKNOWN; // Inherit resource's format
        else // Guess the appropriate format
          newFormat = DirectX::MakeTypelessUNORM (
                      DirectX::MakeTypelessFLOAT (tex_desc.Format));
      }

      if (override)
      {
        auto descCopy =
          (pDesc != nullptr) ?
          *pDesc             : desc;

        descCopy.Format = newFormat;

        const HRESULT hr =
          D3D11Dev_CreateUnorderedAccessView1_Original ( This, pResource,
                                                           &descCopy, ppUAView );

        if (SUCCEEDED (hr))
          return hr;
      }
    }
  }

  const HRESULT hr =
    D3D11Dev_CreateUnorderedAccessView1_Original ( This, pResource,
                                                     pDesc, ppUAView );
  return hr;
}

HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateRasterizerState_Override (
                  ID3D11Device            *This,
  _In_      const D3D11_RASTERIZER_DESC   *pRasterizerDesc,
  _Out_opt_       ID3D11RasterizerState  **ppRasterizerState )
{
  if (pRasterizerDesc == nullptr)
    return E_INVALIDARG;

  return
    D3D11Dev_CreateRasterizerState_Original ( This,
                                                pRasterizerDesc,
                                                  ppRasterizerState );
}

concurrency::concurrent_unordered_set <ID3D11SamplerState *>
  _SK_D3D11_OverrideSamplers__UserDefinedLODBias,
  _SK_D3D11_OverrideSamplers__UserDefinedAnisotropy,
  _SK_D3D11_OverrideSamplers__UserForcedAnisotropic;

HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateSamplerState_Override
(
  _In_            ID3D11Device        *This,
  _In_      const D3D11_SAMPLER_DESC  *pSamplerDesc,
  _Out_opt_       ID3D11SamplerState **ppSamplerState )
{
  if (pSamplerDesc == nullptr)
    return E_INVALIDARG;

  D3D11_SAMPLER_DESC new_desc = *pSamplerDesc;

#pragma region "UglyGameHacksThatShouldNotBeHere"
  static const bool bShenmue =
    SK_GetCurrentGameID () == SK_GAME_ID::Shenmue;

  if (bShenmue)
  {
    config.textures.d3d11.uncompressed_mips = true;
    config.textures.d3d11.cache_gen_mips    = true;
    config.textures.d3d11.generate_mips     = true;

    ///dll_log.Log ( L"CreateSamplerState - Filter: %s, MaxAniso: %lu, MipLODBias: %f, MinLOD: %f, MaxLOD: %f, Comparison: %x, U:%x,V:%x,W:%x - %ws",
    ///             SK_D3D11_FilterToStr (new_desc.Filter), new_desc.MaxAnisotropy, new_desc.MipLODBias, new_desc.MinLOD, new_desc.MaxLOD,
    ///             new_desc.ComparisonFunc, new_desc.AddressU, new_desc.AddressV, new_desc.AddressW, SK_SummarizeCaller ().c_str () );

    if (new_desc.Filter != D3D11_FILTER_MIN_MAG_MIP_POINT)
    {
      //if ( new_desc.ComparisonFunc == D3D11_COMPARISON_ALWAYS /*&&
           //new_desc.MaxLOD         == D3D11_FLOAT32_MAX        */)
      //{
        new_desc.MaxAnisotropy =  16;
        new_desc.Filter        =  D3D11_FILTER_ANISOTROPIC;
        new_desc.MaxLOD        =  D3D11_FLOAT32_MAX;
        new_desc.MinLOD        = -D3D11_FLOAT32_MAX;
      //}
    }

    return
      D3D11Dev_CreateSamplerState_Original (This, &new_desc, ppSamplerState);
  }

#ifdef _M_AMD64
  static bool __yakuza =
    ( SK_GetCurrentGameID () == SK_GAME_ID::Yakuza0 ||
      SK_GetCurrentGameID () == SK_GAME_ID::YakuzaKiwami2 ||
      SK_GetCurrentGameID () == SK_GAME_ID::YakuzaUnderflow );

  if (__yakuza)
  {
    if (__SK_Y0_ClampLODBias)
    {
      new_desc.MipLODBias = std::max (0.0f, new_desc.MipLODBias);
    }

    if (__SK_Y0_ForceAnisoLevel != 0)
    {
      new_desc.MinLOD        = -D3D11_FLOAT32_MAX;
      new_desc.MaxLOD        =  D3D11_FLOAT32_MAX;

      new_desc.MaxAnisotropy = __SK_Y0_ForceAnisoLevel;
    }

    if (__SK_Y0_FixAniso)
    {
      if (new_desc.Filter <= D3D11_FILTER_ANISOTROPIC)
      {
        if (new_desc.MaxAnisotropy > 1)
        {
          new_desc.Filter = D3D11_FILTER_ANISOTROPIC;
        }
      }

      if (new_desc.Filter > D3D11_FILTER_ANISOTROPIC && new_desc.ComparisonFunc == 4)
      {
        if (new_desc.MaxAnisotropy > 1)
        {
          new_desc.Filter = D3D11_FILTER_COMPARISON_ANISOTROPIC;
        }
      }
    }

    const HRESULT hr =
      D3D11Dev_CreateSamplerState_Original (This, &new_desc, ppSamplerState);

    if (SUCCEEDED (hr))
      return hr;
  }
#endif

  static const bool bLegoMarvel2 =
    ( SK_GetCurrentGameID () == SK_GAME_ID::LEGOMarvelSuperheroes2 );

  if (bLegoMarvel2)
  {
    if (new_desc.Filter <= D3D11_FILTER_ANISOTROPIC)
    {
      new_desc.Filter        = D3D11_FILTER_ANISOTROPIC;
      new_desc.MaxAnisotropy = 16;

      new_desc.MipLODBias    = 0.0f;
      new_desc.MinLOD        = 0.0f;
      new_desc.MaxLOD        = D3D11_FLOAT32_MAX;

      const HRESULT hr =
        D3D11Dev_CreateSamplerState_Original (This, &new_desc, ppSamplerState);

      if (SUCCEEDED (hr))
        return hr;
    }
  }

  static const bool bYs8 =
    (SK_GetCurrentGameID () == SK_GAME_ID::Ys_Eight);

  if (bYs8)
  {
    if (config.textures.d3d11.generate_mips && new_desc.Filter <= D3D11_FILTER_ANISOTROPIC)
    {
      //if (new_desc.Filter != D3D11_FILTER_MIN_MAG_MIP_POINT)
      {
        new_desc.Filter        = D3D11_FILTER_ANISOTROPIC;
        new_desc.MaxAnisotropy = 16;

        if (new_desc.MipLODBias < 0.0f)
          new_desc.MipLODBias   = 0.0f;

        new_desc.MinLOD        = 0.0f;
        new_desc.MaxLOD        = D3D11_FLOAT32_MAX;
      }

      const HRESULT hr =
        D3D11Dev_CreateSamplerState_Original (This, &new_desc, ppSamplerState);

      if (SUCCEEDED (hr))
        return hr;
    }

    if ( config.textures.d3d11.generate_mips                          &&
          ( ( new_desc.Filter >  D3D11_FILTER_ANISOTROPIC &&
              new_desc.Filter <= D3D11_FILTER_COMPARISON_ANISOTROPIC) ||
            new_desc.ComparisonFunc != D3D11_COMPARISON_NEVER ) )
    {
      new_desc.Filter        = D3D11_FILTER_COMPARISON_ANISOTROPIC;
      new_desc.MaxAnisotropy = 16;

      if (pSamplerDesc->Filter != new_desc.Filter)
      {
        SK_LOG0 ( ( L"Changing Shadow Filter from '%hs' to '%hs'",
                      SK_D3D11_DescribeFilter (pSamplerDesc->Filter),
                           SK_D3D11_DescribeFilter (new_desc.Filter) ),
                    L" TexCache " );
      }

      const HRESULT hr =
        D3D11Dev_CreateSamplerState_Original (This, &new_desc, ppSamplerState);

      if (SUCCEEDED (hr))
        return hr;
    }
  }

#ifndef _M_AMD64
  static const bool bChronoCross =
    (SK_GetCurrentGameID () == SK_GAME_ID::ChronoCross);

  if (bChronoCross)
  {
    if (SK_GetCallingDLL () == GetModuleHandle (nullptr))
    {
      const char*
      SK_D3D11_FilterToStr (D3D11_FILTER filter) noexcept;

      SK_LOGi0 (
        L"CreateSamplerState - Filter: %hs, MaxAniso: %lu, MipLODBias: %f, MinLOD: %f, MaxLOD: %f, Comparison: %x, U:%x,V:%x,W:%x - %ws",
               SK_D3D11_FilterToStr (new_desc.Filter), new_desc.MaxAnisotropy, new_desc.MipLODBias, new_desc.MinLOD, new_desc.MaxLOD,
               new_desc.ComparisonFunc, new_desc.AddressU, new_desc.AddressV, new_desc.AddressW, SK_SummarizeCaller ().c_str () );
       
      new_desc.AddressU       = D3D11_TEXTURE_ADDRESS_MIRROR;
      new_desc.AddressV       = D3D11_TEXTURE_ADDRESS_MIRROR;
      new_desc.AddressW       = D3D11_TEXTURE_ADDRESS_MIRROR;
      new_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

      HRESULT hr =
        D3D11Dev_CreateSamplerState_Original (This, &new_desc, ppSamplerState);

      if (SUCCEEDED (hr) && new_desc.Filter == D3D11_FILTER_MIN_MAG_MIP_POINT)
      {
        SK_CC_NearestSampler = *ppSamplerState;
        SK_CC_NearestSampler->AddRef ();
      }

      return hr;
    }
  }
#endif
#pragma endregion

  //
  // Modern codepath for generic configurable sampler overrides
  //   (as opposed to the myriad of game-specific hacks above)
  //
  bool bCustomLODBias     = false;
  bool bCustomAnisotropy  = false;
  bool bForcedAnisotropic = false;

  if (config.render.d3d12.force_lod_bias != 0.0f)
  {
    if ( pSamplerDesc->MinLOD !=
         pSamplerDesc->MaxLOD && ( pSamplerDesc->ComparisonFunc == D3D11_COMPARISON_ALWAYS ||
                                   pSamplerDesc->ComparisonFunc == 0 /*WTF does 0 imply?*/ ||
                                   pSamplerDesc->ComparisonFunc == D3D11_COMPARISON_NEVER ) )
    {
      new_desc.MipLODBias =
        config.render.d3d12.force_lod_bias;

      bCustomLODBias = true;
    }
  }

  if (config.render.d3d12.force_anisotropic)
  {
    bForcedAnisotropic = true;

    switch (new_desc.Filter)
    {
      case D3D11_FILTER_MIN_MAG_MIP_LINEAR:                  new_desc.Filter =
           D3D11_FILTER_ANISOTROPIC;                         break;
      case D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR:       new_desc.Filter =
           D3D11_FILTER_COMPARISON_ANISOTROPIC;              break;
      case D3D11_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR:          new_desc.Filter =
           D3D11_FILTER_MINIMUM_ANISOTROPIC;                 break;
      case D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR:          new_desc.Filter =
           D3D11_FILTER_MAXIMUM_ANISOTROPIC;                 break;

      // Upgrade to trilinear Anisotropic...
      //   * Only D3D12 supports bilinear Anisotropic + Mip Nearest
      case D3D11_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT:    new_desc.Filter =
           D3D11_FILTER_MINIMUM_ANISOTROPIC;                 break;
      case D3D11_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:    new_desc.Filter =
           D3D11_FILTER_MAXIMUM_ANISOTROPIC;                 break;

      // XXX: Is this a sensible thing to do?
      case D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT: new_desc.Filter =
           D3D11_FILTER_COMPARISON_ANISOTROPIC;              break;

      default: bForcedAnisotropic = false;                   break;
    }
  }

  switch (new_desc.Filter)
  {
    case D3D11_FILTER_ANISOTROPIC:
    case D3D11_FILTER_COMPARISON_ANISOTROPIC:
    case D3D11_FILTER_MINIMUM_ANISOTROPIC:
    case D3D11_FILTER_MAXIMUM_ANISOTROPIC:
      if (config.render.d3d12.max_anisotropy > 0)
                      new_desc.MaxAnisotropy =
    (UINT)config.render.d3d12.max_anisotropy;
                           bCustomAnisotropy = true; break;
    default:                                         break;
  }

  HRESULT hr =
    D3D11Dev_CreateSamplerState_Original (This, &new_desc, ppSamplerState);

  if (SUCCEEDED (hr))
  {
    if (bCustomLODBias)
      _SK_D3D11_OverrideSamplers__UserDefinedLODBias.insert    (*ppSamplerState);
    if (bCustomAnisotropy)
      _SK_D3D11_OverrideSamplers__UserDefinedAnisotropy.insert (*ppSamplerState);
    if (bForcedAnisotropic)
      _SK_D3D11_OverrideSamplers__UserForcedAnisotropic.insert (*ppSamplerState);
  }

  else
  {
    SK_LOGi0 (L"Sampler State Override(s) Invalid; trying original params...");
    new_desc = *pSamplerDesc;

    hr =
      D3D11Dev_CreateSamplerState_Original (This, &new_desc, ppSamplerState);
  }

  return hr;
}

HMODULE SK_KnownModule_MSMPEG2VDEC = 0;
HMODULE SK_KnownModule_MFPLAT      = 0;

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateTexture2D1_Override (
  _In_            ID3D11Device3          *This,
  _In_      const D3D11_TEXTURE2D_DESC1  *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D1       **ppTexture2D )
{
  SK_LOG_FIRST_CALL

  if (pDesc == nullptr)
    return E_INVALIDARG;

  auto hCallingMod =
    SK_GetCallingDLL ();

  // Give Media Foundation video surfaces a fast-path to bypass our hooks
  if ( hCallingMod == SK_KnownModule_MSMPEG2VDEC ||
       hCallingMod == SK_KnownModule_MFPLAT      ||
         (pDesc->BindFlags & (D3D11_BIND_DECODER | D3D11_BIND_VIDEO_ENCODER)) != 0x0 )
  {
    return
      D3D11Dev_CreateTexture2D1_Original (This, pDesc, pInitialData, ppTexture2D);
  }

  const D3D11_TEXTURE2D_DESC1* pDescOrig =  pDesc;
                          auto descCopy  = *pDescOrig;

  const HRESULT hr =
    D3D11Dev_CreateTexture2DCore_Impl (
      This, nullptr, &descCopy, pInitialData,
            nullptr, ppTexture2D, _ReturnAddress ()
    );

  return hr;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateTexture2D_Override (
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D )
{
  SK_LOG_FIRST_CALL

  if (pDesc == nullptr)
    return E_INVALIDARG;

  auto hCallingMod =
    SK_GetCallingDLL ();

  // Give Media Foundation video surfaces a fast-path to bypass our hooks
  if ( hCallingMod == SK_KnownModule_MSMPEG2VDEC ||
       hCallingMod == SK_KnownModule_MFPLAT      ||
         (pDesc->BindFlags & (D3D11_BIND_DECODER | D3D11_BIND_VIDEO_ENCODER)) != 0x0 )
  {
    return
      D3D11Dev_CreateTexture2D_Original (This, pDesc, pInitialData, ppTexture2D);
  }

  const D3D11_TEXTURE2D_DESC* pDescOrig =  pDesc;
                         auto descCopy  = *pDescOrig;

  const HRESULT hr =
    D3D11Dev_CreateTexture2DCore_Impl (
      This, &descCopy, nullptr, pInitialData,
            ppTexture2D, nullptr, _ReturnAddress ()
    );

  return hr;
}


__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateRenderTargetView_Override (
  _In_            ID3D11Device                   *This,
  _In_            ID3D11Resource                 *pResource,
  _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC  *pDesc,
  _Out_opt_       ID3D11RenderTargetView        **ppRTView )
{
  if (pResource == nullptr)
    return E_INVALIDARG;

#if 0
  SK_ComPtr <ID3D11Device> pActualDevice;
  pResource->GetDevice   (&pActualDevice.p);

  if (! pActualDevice.IsEqualObject (This))
  {
    SK_LOGi0 (L"D3D11 Device Hook Trying to Create RTV for a Resource"
              L"Belonging to a Different Device");
    return DXGI_ERROR_DEVICE_RESET;
  }
#endif

#if 0
  if (! SK_D3D11_EnsureMatchingDevices (pResource, This))
  {
    SK_ComPtr <ID3D11Device> pResourceDev;
    pResource->GetDevice   (&pResourceDev);
  
    This = pResourceDev;
  }
#endif

  return
    SK_D3D11Dev_CreateRenderTargetView_Impl ( This,
                                                pResource, pDesc,
                                                  ppRTView, FALSE );
}

//
// TODO: This is stupid, and code duplication will cause problems in the future...
//
//         Rewrite this so that CreateRenderTargetView forwards its execution
//           through to this function.
//
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateRenderTargetView1_Override (
  _In_            ID3D11Device3                  *This,
  _In_            ID3D11Resource                 *pResource,
  _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC1 *pDesc,
  _Out_opt_       ID3D11RenderTargetView1       **ppRTView )
{
  if (pResource == nullptr)
    return E_INVALIDARG;

#if 0
  SK_ComPtr <ID3D11Device> pActualDevice;
  pResource->GetDevice   (&pActualDevice.p);

  if (! pActualDevice.IsEqualObject (This))
  {
    SK_LOGi0 (L"D3D11 Device Hook Trying to Create RTV for a Resource"
              L"Belonging to a Different Device");
    return DXGI_ERROR_DEVICE_RESET;
  }
#endif

#if 0
  if (! SK_D3D11_EnsureMatchingDevices (pResource, This))
  {
    SK_ComPtr <ID3D11Device> pResourceDev;
    pResource->GetDevice   (&pResourceDev);
  
    This = pResourceDev;
  }
#endif

  return
    SK_D3D11Dev_CreateRenderTargetView1_Impl ( This,
                                                pResource, pDesc,
                                                  ppRTView, FALSE );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateVertexShader_Override (
  _In_            ID3D11Device        *This,
  _In_      const void                *pShaderBytecode,
  _In_            SIZE_T               BytecodeLength,
  _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
  _Out_opt_       ID3D11VertexShader **ppVertexShader )
{
  if (pShaderBytecode == nullptr)
    return E_INVALIDARG;

  return
    SK_D3D11_CreateShader_Impl ( This,
                                   pShaderBytecode, BytecodeLength,
                                     pClassLinkage,
                         (IUnknown **)(ppVertexShader),
                                         sk_shader_class::Vertex );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreatePixelShader_Override (
  _In_            ID3D11Device        *This,
  _In_      const void                *pShaderBytecode,
  _In_            SIZE_T               BytecodeLength,
  _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
  _Out_opt_       ID3D11PixelShader  **ppPixelShader )
{
  if (pShaderBytecode == nullptr)
    return E_INVALIDARG;

  return
    SK_D3D11_CreateShader_Impl ( This,
                                   pShaderBytecode, BytecodeLength,
                                     pClassLinkage,
                         (IUnknown **)(ppPixelShader),
                                         sk_shader_class::Pixel );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateGeometryShader_Override (
  _In_            ID3D11Device          *This,
  _In_      const void                  *pShaderBytecode,
  _In_            SIZE_T                 BytecodeLength,
  _In_opt_        ID3D11ClassLinkage    *pClassLinkage,
  _Out_opt_       ID3D11GeometryShader **ppGeometryShader )
{
  if (pShaderBytecode == nullptr)
    return E_INVALIDARG;

  return
    SK_D3D11_CreateShader_Impl ( This,
                                   pShaderBytecode, BytecodeLength,
                                     pClassLinkage,
                         (IUnknown **)(ppGeometryShader),
                                         sk_shader_class::Geometry );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateGeometryShaderWithStreamOutput_Override (
  _In_            ID3D11Device               *This,
  _In_      const void                       *pShaderBytecode,
  _In_            SIZE_T                     BytecodeLength,
  _In_opt_  const D3D11_SO_DECLARATION_ENTRY *pSODeclaration,
  _In_            UINT                       NumEntries,
  _In_opt_  const UINT                       *pBufferStrides,
  _In_            UINT                       NumStrides,
  _In_            UINT                       RasterizedStream,
  _In_opt_        ID3D11ClassLinkage         *pClassLinkage,
  _Out_opt_       ID3D11GeometryShader      **ppGeometryShader )
{
  if (pShaderBytecode == nullptr)
    return E_INVALIDARG;

  const HRESULT hr =
    D3D11Dev_CreateGeometryShaderWithStreamOutput_Original ( This, pShaderBytecode,
                                                               BytecodeLength,
                                                                 pSODeclaration, NumEntries,
                                                                   pBufferStrides, NumStrides,
                                                                     RasterizedStream, pClassLinkage,
                                                                       ppGeometryShader );

  if (SUCCEEDED (hr) && ppGeometryShader)
  {
    auto& geo_shaders =
      SK_D3D11_Shaders->geometry;

    uint32_t checksum =
      SK_D3D11_ChecksumShaderBytecode (pShaderBytecode, BytecodeLength);

    if (checksum == 0x00)
      checksum = (uint32_t)BytecodeLength;

    cs_shader_gs->lock ();

    if (! geo_shaders.descs [This].count (checksum))
    {
      SK_D3D11_ShaderDesc desc;

      desc.type   = SK_D3D11_ShaderType::Geometry;
      desc.crc32c = checksum;

      desc.bytecode.insert (  desc.bytecode.cend  (),
        &((const uint8_t *) pShaderBytecode) [0],
        &((const uint8_t *) pShaderBytecode) [BytecodeLength]
      );

      geo_shaders.descs [This].emplace (std::make_pair (checksum, desc));
    }

    SK_D3D11_ShaderDesc* pDesc =
      &geo_shaders.descs [This][checksum];

    if ( geo_shaders.rev [This].count (*ppGeometryShader) &&
               geo_shaders.rev [This][*ppGeometryShader]->crc32c != checksum )
         geo_shaders.rev [This].erase (*ppGeometryShader);

    geo_shaders.rev [This].emplace (std::make_pair (*ppGeometryShader, pDesc));

    cs_shader_gs->unlock ();

    InterlockedExchange64 ((volatile LONG64 *)&pDesc->usage.last_frame, SK_GetFramesDrawn ());
              //_time64 (&desc.usage.last_time);
  }

  return hr;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateHullShader_Override (
  _In_            ID3D11Device        *This,
  _In_      const void                *pShaderBytecode,
  _In_            SIZE_T               BytecodeLength,
  _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
  _Out_opt_       ID3D11HullShader   **ppHullShader )
{
  if (pShaderBytecode == nullptr)
    return E_INVALIDARG;

  return
    SK_D3D11_CreateShader_Impl ( This,
                                   pShaderBytecode, BytecodeLength,
                                     pClassLinkage,
                         (IUnknown **)(ppHullShader),
                                         sk_shader_class::Hull );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateDomainShader_Override (
  _In_            ID3D11Device        *This,
  _In_      const void                *pShaderBytecode,
  _In_            SIZE_T               BytecodeLength,
  _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
  _Out_opt_       ID3D11DomainShader **ppDomainShader )
{
  if (pShaderBytecode == nullptr)
    return E_INVALIDARG;

  return
    SK_D3D11_CreateShader_Impl ( This,
                                   pShaderBytecode, BytecodeLength,
                                     pClassLinkage,
                         (IUnknown **)(ppDomainShader),
                                         sk_shader_class::Domain );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateComputeShader_Override (
  _In_            ID3D11Device         *This,
  _In_      const void                 *pShaderBytecode,
  _In_            SIZE_T                BytecodeLength,
  _In_opt_        ID3D11ClassLinkage   *pClassLinkage,
  _Out_opt_       ID3D11ComputeShader **ppComputeShader )
{
  if (pShaderBytecode == nullptr)
    return E_INVALIDARG;

  return
    SK_D3D11_CreateShader_Impl ( This,
                                   pShaderBytecode, BytecodeLength,
                                     pClassLinkage,
                         (IUnknown **)(ppComputeShader),
                                         sk_shader_class::Compute );
}