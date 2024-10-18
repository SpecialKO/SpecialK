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

#ifndef __SK__D3D11_INTERFACES_H__
#define __SK__D3D11_INTERFACES_H__

/*-------------------------------------------------------------------------------------
*
* Copyright (c) Microsoft Corporation
*
*-------------------------------------------------------------------------------------*/


/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 8.00.0613 */
/* @@MIDL_FILE_HEADING(  ) */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

/* verify that the <rpcsal.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCSAL_H_VERSION__
#define __REQUIRED_RPCSAL_H_VERSION__ 100
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/


#define __ID3D11VideoDevice_INTERFACE_DEFINED__
#define __ID3D11VideoDevice1_INTERFACE_DEFINED__
#define __ID3D11VideoDevice2_INTERFACE_DEFINED__

#define __ID3D11VideoContext_INTERFACE_DEFINED__
#define __ID3D11VideoContext1_INTERFACE_DEFINED__
#define __ID3D11VideoContext2_INTERFACE_DEFINED__
#define __ID3D11VideoContext3_INTERFACE_DEFINED__

#define __ID3D11VideoProcessorEnumerator_INTERFACE_DEFINED__
#define __ID3D11VideoProcessorEnumerator1_INTERFACE_DEFINED__

#define __ID3D11CryptoSession_INTERFACE_DEFINED__

//#ifndef __d3d11_h__
//#ifdef __clang__
//interface ID3D11CryptoSession;
//
//typedef struct D3D11_VIDEO_DECODER_DESC
//{
//  GUID Guid;
//  UINT SampleWidth;
//  UINT SampleHeight;
//  DXGI_FORMAT OutputFormat;
//} D3D11_VIDEO_DECODER_DESC;
//
//typedef enum D3D11_VIDEO_DECODER_BUFFER_TYPE
//{
//  D3D11_VIDEO_DECODER_BUFFER_PICTURE_PARAMETERS          = 0,
//  D3D11_VIDEO_DECODER_BUFFER_MACROBLOCK_CONTROL          = 1,
//  D3D11_VIDEO_DECODER_BUFFER_RESIDUAL_DIFFERENCE         = 2,
//  D3D11_VIDEO_DECODER_BUFFER_DEBLOCKING_CONTROL          = 3,
//  D3D11_VIDEO_DECODER_BUFFER_INVERSE_QUANTIZATION_MATRIX = 4,
//  D3D11_VIDEO_DECODER_BUFFER_SLICE_CONTROL               = 5,
//  D3D11_VIDEO_DECODER_BUFFER_BITSTREAM                   = 6,
//  D3D11_VIDEO_DECODER_BUFFER_MOTION_VECTOR               = 7,
//  D3D11_VIDEO_DECODER_BUFFER_FILM_GRAIN                  = 8
//} D3D11_VIDEO_DECODER_BUFFER_TYPE;
//#endif
//#endif

#include "d3dcommon.h"
#include <dxgitype.h>
#include <d3dcompiler.h>

#include <dxgi1_5.h>

#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d11_2.h>
#include <d3d11_3.h>
#include <d3d11_4.h>

#define __D3DX11TEX_H__

//#include <../depends/include/DXSDK/D3DX11.h>
//#include <../depends/include/DXSDK/D3DX11tex.h>

#include <SpecialK/render/dxgi/dxgi_backend.h>


#pragma comment (lib, "dxguid.lib")

// {CD2B947D-85F8-4B8B-9069-E1ADB0C5F0D4}
const GUID IID_SKVertexShaderD3D11   = { 0xcd2b947d, 0x85f8, 0x4b8b,
                         { 0x90, 0x69, 0xe1, 0xad, 0xb0, 0xc5, 0xf0, 0xd4 } };

// {F0ECF4E9-17AC-4159-A0A5-8F0261952FC3}
const GUID IID_SKPixelShaderD3D11    = { 0xf0ecf4e9, 0x17ac, 0x4159,
                         { 0xa0, 0xa5, 0x8f, 0x2, 0x61, 0x95, 0x2f, 0xc3 } };

// {87FC5176-60AF-4BB8-8E0F-5A1027BEECFF}
const GUID IID_SKGeometryShaderD3D11 = { 0x87fc5176, 0x60af, 0x4bb8,
                         { 0x8e, 0xf, 0x5a, 0x10, 0x27, 0xbe, 0xec, 0xff } };

// {579991EC-F558-4216-B0E2-CCEFE518AB4C}
const GUID IID_SKHullShaderD3D11     = { 0x579991ec, 0xf558, 0x4216,
                         { 0xb0, 0xe2, 0xcc, 0xef, 0xe5, 0x18, 0xab, 0x4c } };

// {BA8BB041-3BED-443F-9514-25BEC3EB501D}
const GUID IID_SKDomainShaderD3D11  = { 0xba8bb041, 0x3bed, 0x443f,
                         { 0x95, 0x14, 0x25, 0xbe, 0xc3, 0xeb, 0x50, 0x1d } };

// {B326697D-B458-483F-A8CE-6A4B49B19FE9}
const GUID IID_SKComputeShaderD3D11 = { 0xb326697d, 0xb458, 0x483f,
                         { 0xa8, 0xce, 0x6a, 0x4b, 0x49, 0xb1, 0x9f, 0xe9 } };

struct ID3D11Device;
struct ID3D11ClassLinkage;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11GeometryShader;
struct ID3D11HullShader;
struct ID3D11DomainShader;
struct ID3D11ComputeShader;
struct D3D11_SO_DECLARATION_ENTRY;

typedef HRESULT (STDMETHODCALLTYPE *D3D11Dev_CreateVertexShader_pfn)(ID3D11Device        *This,
                                                     _In_      const void                *pShaderBytecode,
                                                     _In_            SIZE_T               BytecodeLength,
                                                     _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
                                                     _Out_opt_       ID3D11VertexShader **ppVertexShader);

typedef HRESULT (STDMETHODCALLTYPE *D3D11Dev_CreatePixelShader_pfn)(ID3D11Device        *This,
                                                    _In_      const void                *pShaderBytecode,
                                                    _In_            SIZE_T               BytecodeLength,
                                                    _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
                                                    _Out_opt_       ID3D11PixelShader  **ppPixelShader);

typedef HRESULT (STDMETHODCALLTYPE *D3D11Dev_CreateGeometryShader_pfn)(ID3D11Device          *This,
                                                       _In_      const void                  *pShaderBytecode,
                                                       _In_            SIZE_T                 BytecodeLength,
                                                       _In_opt_        ID3D11ClassLinkage    *pClassLinkage,
                                                       _Out_opt_       ID3D11GeometryShader **ppGeometryShader);

typedef HRESULT (STDMETHODCALLTYPE *D3D11Dev_CreateGeometryShaderWithStreamOutput_pfn)(
  _In_            ID3D11Device                *This,
  _In_      const void                        *pShaderBytecode,
  _In_            SIZE_T                       BytecodeLength,
  _In_opt_  const D3D11_SO_DECLARATION_ENTRY  *pSODeclaration,
  _In_            UINT                         NumEntries,
  _In_opt_  const UINT                        *pBufferStrides,
  _In_            UINT                         NumStrides,
  _In_            UINT                         RasterizedStream,
  _In_opt_        ID3D11ClassLinkage          *pClassLinkage,
  _Out_opt_       ID3D11GeometryShader       **ppGeometryShader );

typedef HRESULT (STDMETHODCALLTYPE *D3D11Dev_CreateHullShader_pfn)(ID3D11Device        *This,
                                                   _In_      const void                *pShaderBytecode,
                                                   _In_            SIZE_T               BytecodeLength,
                                                   _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
                                                   _Out_opt_       ID3D11HullShader   **ppHullShader);

typedef HRESULT (STDMETHODCALLTYPE *D3D11Dev_CreateDomainShader_pfn)(ID3D11Device        *This,
                                                     _In_      const void                *pShaderBytecode,
                                                     _In_            SIZE_T               BytecodeLength,
                                                     _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
                                                     _Out_opt_       ID3D11DomainShader **ppDomainShader);

typedef HRESULT (STDMETHODCALLTYPE *D3D11Dev_CreateComputeShader_pfn)(ID3D11Device         *This,
                                                      _In_      const void                 *pShaderBytecode,
                                                      _In_            SIZE_T                BytecodeLength,
                                                      _In_opt_        ID3D11ClassLinkage   *pClassLinkage,
                                                      _Out_opt_       ID3D11ComputeShader **ppComputeShader);

//struct ID3D11DeviceContext;
//struct ID3D11ClassInstance;
//struct ID3D11Asynchronous;

using D3D11_PSSetShader_pfn =
  void (STDMETHODCALLTYPE *)( ID3D11DeviceContext        *This,
                     _In_opt_ ID3D11PixelShader          *pPixelShader,
                     _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
                              UINT                        NumClassInstances );

using D3D11_GSSetShader_pfn =
  void (STDMETHODCALLTYPE *)( ID3D11DeviceContext        *This,
                     _In_opt_ ID3D11GeometryShader       *pGeometryShader,
                     _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
                              UINT                        NumClassInstances );

using D3D11_HSSetShader_pfn =
  void (STDMETHODCALLTYPE *)( ID3D11DeviceContext        *This,
                     _In_opt_ ID3D11HullShader           *pHullShader,
                     _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
                              UINT                        NumClassInstances );

using D3D11_DSSetShader_pfn =
  void (STDMETHODCALLTYPE *)( ID3D11DeviceContext        *This,
                     _In_opt_ ID3D11DomainShader         *pDomainShader,
                     _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
                              UINT                        NumClassInstances );

using D3D11_CSSetShader_pfn =
  void (STDMETHODCALLTYPE *)( ID3D11DeviceContext        *This,
                     _In_opt_ ID3D11ComputeShader        *pComputeShader,
                     _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
                              UINT                        NumClassInstances );

typedef HRESULT (STDMETHODCALLTYPE *D3D11_GetData_pfn)(ID3D11DeviceContext      *This,
                                                       _In_  ID3D11Asynchronous *pAsync,
                                     _Out_writes_bytes_opt_ (DataSize)     void *pData,
                                                       _In_  UINT                DataSize,
                                                       _In_  UINT                GetDataFlags);

typedef void (STDMETHODCALLTYPE *D3D11_VSSetShader_pfn)(ID3D11DeviceContext        *This,
                                               _In_opt_ ID3D11VertexShader         *pVertexShader,
                                               _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
                                                        UINT                        NumClassInstances);

typedef void (STDMETHODCALLTYPE *D3D11_PSSetShader_pfn)(ID3D11DeviceContext        *This,
                                               _In_opt_ ID3D11PixelShader          *pPixelShader,
                                               _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
                                                        UINT                        NumClassInstances);

typedef void (STDMETHODCALLTYPE *D3D11_GSSetShader_pfn)(ID3D11DeviceContext        *This,
                                               _In_opt_ ID3D11GeometryShader       *pGeometryShader,
                                               _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
                                                        UINT                        NumClassInstances);

typedef void (STDMETHODCALLTYPE *D3D11_HSSetShader_pfn)(ID3D11DeviceContext        *This,
                                               _In_opt_ ID3D11HullShader           *pHullShader,
                                               _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
                                                        UINT                        NumClassInstances);

typedef void (STDMETHODCALLTYPE *D3D11_DSSetShader_pfn)(ID3D11DeviceContext        *This,
                                               _In_opt_ ID3D11DomainShader         *pDomainShader,
                                               _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
                                                        UINT                        NumClassInstances);

typedef void (STDMETHODCALLTYPE *D3D11_CSSetShader_pfn)(ID3D11DeviceContext        *This,
                                               _In_opt_ ID3D11ComputeShader        *pComputeShader,
                                               _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
                                                        UINT                        NumClassInstances);


typedef void (STDMETHODCALLTYPE *D3D11_VSGetShader_pfn)(ID3D11DeviceContext         *This,
                                            _Out_       ID3D11VertexShader         **ppVertexShader,
                                            _Out_opt_   ID3D11ClassInstance *const  *ppClassInstances,
                                            _Inout_opt_ UINT                        *pNumClassInstances);

typedef void (STDMETHODCALLTYPE *D3D11_PSGetShader_pfn)(ID3D11DeviceContext         *This,
                                            _Out_       ID3D11PixelShader          **ppPixelShader,
                                            _Out_opt_   ID3D11ClassInstance *const  *ppClassInstances,
                                            _Inout_opt_ UINT                        *pNumClassInstances);

typedef void (STDMETHODCALLTYPE *D3D11_GSGetShader_pfn)(ID3D11DeviceContext         *This,
                                            _Out_       ID3D11GeometryShader       **ppGeometryShader,
                                            _Out_opt_   ID3D11ClassInstance *const  *ppClassInstances,
                                            _Inout_opt_ UINT                        *pNumClassInstances);

typedef void (STDMETHODCALLTYPE *D3D11_HSGetShader_pfn)(ID3D11DeviceContext         *This,
                                            _Out_       ID3D11HullShader           **ppHullShader,
                                            _Out_opt_   ID3D11ClassInstance *const  *ppClassInstances,
                                            _Inout_opt_ UINT                        *pNumClassInstances);

typedef void (STDMETHODCALLTYPE *D3D11_DSGetShader_pfn)(ID3D11DeviceContext         *This,
                                            _Out_       ID3D11DomainShader         **ppDomainShader,
                                            _Out_opt_   ID3D11ClassInstance *const  *ppClassInstances,
                                            _Inout_opt_ UINT                        *pNumClassInstances);

typedef void (STDMETHODCALLTYPE *D3D11_CSGetShader_pfn)(ID3D11DeviceContext         *This,
                                            _Out_       ID3D11ComputeShader        **ppComputeShader,
                                            _Out_opt_   ID3D11ClassInstance *const  *ppClassInstances,
                                            _Inout_opt_ UINT                        *pNumClassInstances);

typedef void (STDMETHODCALLTYPE *D3D11_Dispatch_pfn)(ID3D11DeviceContext *This,
                                                _In_ UINT                 ThreadGroupCountX,
                                                _In_ UINT                 ThreadGroupCountY,
                                                _In_ UINT                 ThreadGroupCountZ);

struct ID3D11Buffer;

typedef void (STDMETHODCALLTYPE *D3D11_DispatchIndirect_pfn)(ID3D11DeviceContext *This,
                                                        _In_ ID3D11Buffer        *pBufferForArgs,
                                                        _In_ UINT                 AlignedByteOffsetForArgs);

typedef void (STDMETHODCALLTYPE *D3D11_DrawAuto_pfn)(ID3D11DeviceContext *This);

//struct ID3D11RenderTargetView;
//struct ID3D11DepthStencilView;
//struct ID3D11UnorderedAccessView;

typedef void (STDMETHODCALLTYPE *D3D11_OMSetRenderTargets_pfn)(ID3D11DeviceContext           *This,
                                                      _In_     UINT                           NumViews,
                                                      _In_opt_ ID3D11RenderTargetView *const *ppRenderTargetViews,
                                                      _In_opt_ ID3D11DepthStencilView        *pDepthStencilView);

typedef void (STDMETHODCALLTYPE *D3D11_OMSetRenderTargetsAndUnorderedAccessViews_pfn)(ID3D11DeviceContext              *This,
                                                                       _In_           UINT                              NumRTVs,
                                                                       _In_opt_       ID3D11RenderTargetView    *const *ppRenderTargetViews,
                                                                       _In_opt_       ID3D11DepthStencilView           *pDepthStencilView,
                                                                       _In_           UINT                              UAVStartSlot,
                                                                       _In_           UINT                              NumUAVs,
                                                                       _In_opt_       ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
                                                                       _In_opt_ const UINT                             *pUAVInitialCounts);

typedef void (STDMETHODCALLTYPE *D3D11_OMGetRenderTargets_pfn)(ID3D11DeviceContext     *This,
                                                          _In_ UINT                     NumViews,
                                                         _Out_ ID3D11RenderTargetView **ppRenderTargetViews,
                                                         _Out_ ID3D11DepthStencilView **ppDepthStencilView);

typedef void (STDMETHODCALLTYPE *D3D11_OMGetRenderTargetsAndUnorderedAccessViews_pfn)(ID3D11DeviceContext         *This,
                                                                                 _In_  UINT                        NumRTVs,
                                                                                 _Out_ ID3D11RenderTargetView    **ppRenderTargetViews,
                                                                                 _Out_ ID3D11DepthStencilView    **ppDepthStencilView,
                                                                                 _In_  UINT                        UAVStartSlot,
                                                                                 _In_  UINT                        NumUAVs,
                                                                                 _Out_ ID3D11UnorderedAccessView **ppUnorderedAccessViews);

typedef void (STDMETHODCALLTYPE *D3D11_ClearRenderTargetView_pfn)(ID3D11DeviceContext    *This,
                                                       _In_       ID3D11RenderTargetView *pRenderTargetView,
                                                       _In_ const FLOAT                   ColorRGBA [4]);

typedef void (STDMETHODCALLTYPE *D3D11_ClearDepthStencilView_pfn)(ID3D11DeviceContext    *This,
                                                             _In_ ID3D11DepthStencilView *pDepthStencilView,
                                                             _In_ UINT                    ClearFlags,
                                                             _In_ FLOAT                   Depth,
                                                             _In_ UINT8                   Stencil);

interface ID3D11DeviceContext2;
interface ID3D11DeviceContext3;

typedef HRESULT (STDMETHODCALLTYPE *D3D11Dev_CreateDeferredContext_pfn)(
  _In_            ID3D11Device         *This,
  _In_            UINT                  ContextFlags,
  _Out_opt_       ID3D11DeviceContext **ppDeferredContext
);
typedef HRESULT (STDMETHODCALLTYPE *D3D11Dev_CreateDeferredContext1_pfn)(
  _In_            ID3D11Device          *This,
  _In_            UINT                   ContextFlags,
  _Out_opt_       ID3D11DeviceContext1 **ppDeferredContext
);
typedef HRESULT (STDMETHODCALLTYPE *D3D11Dev_CreateDeferredContext2_pfn)(
  _In_            ID3D11Device          *This,
  _In_            UINT                   ContextFlags,
  _Out_opt_       ID3D11DeviceContext2 **ppDeferredContext
);
typedef HRESULT (STDMETHODCALLTYPE *D3D11Dev_CreateDeferredContext3_pfn)(
  _In_            ID3D11Device          *This,
  _In_            UINT                   ContextFlags,
  _Out_opt_       ID3D11DeviceContext3 **ppDeferredContext
);
typedef void (STDMETHODCALLTYPE *D3D11Dev_GetImmediateContext_pfn)(
  _In_            ID3D11Device         *This,
  _Out_           ID3D11DeviceContext **ppImmediateContext
);
typedef void (STDMETHODCALLTYPE *D3D11Dev_GetImmediateContext1_pfn)(
  _In_            ID3D11Device          *This,
  _Out_           ID3D11DeviceContext1 **ppImmediateContext
);
typedef void (STDMETHODCALLTYPE *D3D11Dev_GetImmediateContext2_pfn)(
  _In_            ID3D11Device          *This,
  _Out_           ID3D11DeviceContext2 **ppImmediateContext
);
typedef void (STDMETHODCALLTYPE *D3D11Dev_GetImmediateContext3_pfn)(
  _In_            ID3D11Device          *This,
  _Out_           ID3D11DeviceContext3 **ppImmediateContext
);
typedef HRESULT (WINAPI *D3D11Dev_CreateRasterizerState_pfn)(
        ID3D11Device            *This,
  const D3D11_RASTERIZER_DESC   *pRasterizerDesc,
        ID3D11RasterizerState  **ppRasterizerState
);
typedef HRESULT (WINAPI *D3D11Dev_CreateSamplerState_pfn)(
  _In_            ID3D11Device        *This,
  _In_      const D3D11_SAMPLER_DESC  *pSamplerDesc,
  _Out_opt_       ID3D11SamplerState **ppSamplerState
);
typedef HRESULT (WINAPI *D3D11Dev_CreateTexture2D_pfn)(
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D
);
typedef HRESULT (WINAPI *D3D11Dev_CreateTexture2D1_pfn)(
  _In_            ID3D11Device3          *This,
  _In_      const D3D11_TEXTURE2D_DESC1  *pDesc1,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D1       **ppTexture2D
);
typedef HRESULT (WINAPI *D3D11Dev_CreateRenderTargetView_pfn)(
  _In_            ID3D11Device                   *This,
  _In_            ID3D11Resource                 *pResource,
  _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC  *pDesc,
  _Out_opt_       ID3D11RenderTargetView        **ppRTView
);
typedef HRESULT (WINAPI *D3D11Dev_CreateRenderTargetView1_pfn)(
  _In_            ID3D11Device3                  *This,
  _In_            ID3D11Resource                 *pResource,
  _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC1 *pDesc,
  _Out_opt_       ID3D11RenderTargetView1       **ppRTView
);
typedef void (WINAPI *D3D11_PSSetSamplers_pfn)(
  _In_     ID3D11DeviceContext        *This,
  _In_     UINT                        StartSlot,
  _In_     UINT                        NumSamplers,
  _In_opt_ ID3D11SamplerState*/*const*/*ppSamplers
);
typedef void (WINAPI *D3D11_RSSetScissorRects_pfn)(
  _In_           ID3D11DeviceContext *This,
  _In_           UINT                 NumRects,
  _In_opt_ const D3D11_RECT          *pRects
);
typedef void (WINAPI *D3D11_RSSetViewports_pfn)(
  _In_           ID3D11DeviceContext* This,
  _In_           UINT                 NumViewports,
  _In_opt_ const D3D11_VIEWPORT     * pViewports
);
typedef void (WINAPI *D3D11_UpdateSubresource_pfn)(
  _In_           ID3D11DeviceContext *This,
  _In_           ID3D11Resource      *pDstResource,
  _In_           UINT                 DstSubresource,
  _In_opt_ const D3D11_BOX           *pDstBox,
  _In_     const void                *pSrcData,
  _In_           UINT                 SrcRowPitch,
  _In_           UINT                 SrcDepthPitch
);
typedef HRESULT (WINAPI *D3D11_Map_pfn)(
  _In_      ID3D11DeviceContext      *This,
  _In_      ID3D11Resource           *pResource,
  _In_      UINT                      Subresource,
  _In_      D3D11_MAP                 MapType,
  _In_      UINT                      MapFlags,
  _Out_opt_ D3D11_MAPPED_SUBRESOURCE *pMappedResource
);
typedef void (WINAPI *D3D11_Unmap_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Resource      *pResource,
  _In_ UINT                 Subresource
);

typedef void (WINAPI *D3D11_CopyResource_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Resource      *pDstResource,
  _In_ ID3D11Resource      *pSrcResource
);
typedef void (WINAPI *D3D11_CopySubresourceRegion_pfn)(
  _In_           ID3D11DeviceContext *This,
  _In_           ID3D11Resource      *pDstResource,
  _In_           UINT                 DstSubresource,
  _In_           UINT                 DstX,
  _In_           UINT                 DstY,
  _In_           UINT                 DstZ,
  _In_           ID3D11Resource      *pSrcResource,
  _In_           UINT                 SrcSubresource,
  _In_opt_ const D3D11_BOX           *pSrcBox
);

typedef void (WINAPI *D3D11_ResolveSubresource_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Resource      *pDstResource,
  _In_ UINT                 DstSubresource,
  _In_ ID3D11Resource      *pSrcResource,
  _In_ UINT                 SrcSubresource,
  _In_ DXGI_FORMAT          Format
);

typedef void (WINAPI *D3D11_VSSetShaderResources_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews
);
typedef void (WINAPI *D3D11_VSSetConstantBuffers_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumBuffers,
  _In_opt_       ID3D11Buffer *const             *ppConstantBuffers
);
typedef void (WINAPI *D3D11_PSSetShaderResources_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews
);
typedef void (WINAPI *D3D11_PSSetConstantBuffers_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumBuffers,
  _In_opt_       ID3D11Buffer *const             *ppConstantBuffers
);
typedef void (WINAPI *D3D11_GSSetShaderResources_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews
);
typedef void (WINAPI *D3D11_GSSetConstantBuffers_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumBuffers,
  _In_opt_       ID3D11Buffer *const             *ppConstantBuffers
);
typedef void (WINAPI *D3D11_HSSetShaderResources_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews
);
typedef void (WINAPI *D3D11_HSSetConstantBuffers_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumBuffers,
  _In_opt_       ID3D11Buffer *const             *ppConstantBuffers
);
typedef void (WINAPI *D3D11_DSSetShaderResources_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews
);
typedef void (WINAPI *D3D11_DSSetConstantBuffers_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumBuffers,
  _In_opt_       ID3D11Buffer *const             *ppConstantBuffers
);
typedef void (WINAPI *D3D11_CSSetShaderResources_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews
);
typedef void (WINAPI *D3D11_CSSetUnorderedAccessViews_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumUAVs,
  _In_opt_       ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
  _In_opt_ const UINT                             *pUAVInitialCounts
);


typedef HRESULT (WINAPI *D3D11Dev_CreateBuffer_pfn)(
  _In_           ID3D11Device            *This,
  _In_     const D3D11_BUFFER_DESC       *pDesc,
  _In_opt_ const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_      ID3D11Buffer           **ppBuffer
);
typedef HRESULT (WINAPI *D3D11Dev_CreateShaderResourceView_pfn)(
  _In_           ID3D11Device                     *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC  *pDesc,
  _Out_opt_      ID3D11ShaderResourceView        **ppSRView
);
typedef HRESULT (WINAPI *D3D11Dev_CreateShaderResourceView1_pfn)(
  _In_           ID3D11Device3                    *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC1 *pDesc,
  _Out_opt_      ID3D11ShaderResourceView1       **ppSRView
);
typedef HRESULT (WINAPI *D3D11Dev_CreateDepthStencilView_pfn)(
  _In_            ID3D11Device                  *This,
  _In_            ID3D11Resource                *pResource,
  _In_opt_  const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,
  _Out_opt_       ID3D11DepthStencilView        **ppDepthStencilView
);
typedef HRESULT (WINAPI *D3D11Dev_CreateUnorderedAccessView_pfn)(
  _In_            ID3D11Device                     *This,
  _In_            ID3D11Resource                   *pResource,
  _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc,
  _Out_opt_       ID3D11UnorderedAccessView       **ppUAView
);
typedef HRESULT (WINAPI *D3D11Dev_CreateUnorderedAccessView1_pfn)(
  _In_            ID3D11Device3                     *This,
  _In_            ID3D11Resource                    *pResource,
  _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC1 *pDesc,
  _Out_opt_       ID3D11UnorderedAccessView1       **ppUAView
);

typedef HRESULT (WINAPI *D3D11Dev_CheckFeatureSupport_pfn)(
  _In_            ID3D11Device  *This,
  _In_            D3D11_FEATURE  Feature,
  _Out_           void           *pFeatureSupportData,
  _In_            UINT            FeatureSupportDataSize );

typedef void (WINAPI *D3D11_DrawIndexed_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation
);
typedef void (WINAPI *D3D11_Draw_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCount,
  _In_ UINT                 StartVertexLocation
);
typedef void (WINAPI *D3D11_DrawIndexedInstanced_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation,
  _In_ UINT                 StartInstanceLocation
);
typedef void (WINAPI *D3D11_DrawIndexedInstancedIndirect_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs
);
typedef void (WINAPI *D3D11_DrawInstanced_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartVertexLocation,
  _In_ UINT                 StartInstanceLocation
);
typedef void (WINAPI *D3D11_DrawInstancedIndirect_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs
);

typedef void (WINAPI *D3D11_ClearState_pfn)(
  _In_ ID3D11DeviceContext *This
);
typedef void (WINAPI *D3D11_ExecuteCommandList_pfn)(
  _In_  ID3D11DeviceContext *This,
  _In_  ID3D11CommandList   *pCommandList,
        BOOL                 RestoreContextState
);
typedef HRESULT (WINAPI *D3D11_FinishCommandList_pfn)(
      _In_  ID3D11DeviceContext *This,
            BOOL                 RestoreDeferredContextState,
  _Out_opt_ ID3D11CommandList  **ppCommandList
);

extern D3D11Dev_CreateBuffer_pfn                          D3D11Dev_CreateBuffer_Original;
extern D3D11Dev_CreateTexture2D_pfn                       D3D11Dev_CreateTexture2D_Original;
extern D3D11Dev_CreateTexture2D1_pfn                      D3D11Dev_CreateTexture2D1_Original;
extern D3D11Dev_CreateRenderTargetView_pfn                D3D11Dev_CreateRenderTargetView_Original;
extern D3D11Dev_CreateRenderTargetView1_pfn               D3D11Dev_CreateRenderTargetView1_Original;
extern D3D11Dev_CreateShaderResourceView_pfn              D3D11Dev_CreateShaderResourceView_Original;

extern D3D11Dev_CreateVertexShader_pfn                    D3D11Dev_CreateVertexShader_Original;
extern D3D11Dev_CreatePixelShader_pfn                     D3D11Dev_CreatePixelShader_Original;
extern D3D11Dev_CreateGeometryShader_pfn                  D3D11Dev_CreateGeometryShader_Original;
extern D3D11Dev_CreateGeometryShaderWithStreamOutput_pfn  D3D11Dev_CreateGeometryShaderWithStreamOutput_Original;
extern D3D11Dev_CreateHullShader_pfn                      D3D11Dev_CreateHullShader_Original;
extern D3D11Dev_CreateDomainShader_pfn                    D3D11Dev_CreateDomainShader_Original;
extern D3D11Dev_CreateComputeShader_pfn                   D3D11Dev_CreateComputeShader_Original;

extern D3D11Dev_CreateDeferredContext_pfn                 D3D11Dev_CreateDeferredContext_Original;
extern D3D11Dev_CreateDeferredContext1_pfn                D3D11Dev_CreateDeferredContext1_Original;
extern D3D11Dev_CreateDeferredContext2_pfn                D3D11Dev_CreateDeferredContext2_Original;
extern D3D11Dev_CreateDeferredContext3_pfn                D3D11Dev_CreateDeferredContext3_Original;
extern D3D11Dev_GetImmediateContext_pfn                   D3D11Dev_GetImmediateContext_Original;
extern D3D11Dev_GetImmediateContext1_pfn                  D3D11Dev_GetImmediateContext1_Original;
extern D3D11Dev_GetImmediateContext2_pfn                  D3D11Dev_GetImmediateContext2_Original;
extern D3D11Dev_GetImmediateContext3_pfn                  D3D11Dev_GetImmediateContext3_Original;

extern D3D11_RSSetScissorRects_pfn                        D3D11_RSSetScissorRects_Original;
extern D3D11_RSSetViewports_pfn                           D3D11_RSSetViewports_Original;
extern D3D11_VSSetConstantBuffers_pfn                     D3D11_VSSetConstantBuffers_Original;
extern D3D11_PSSetShaderResources_pfn                     D3D11_PSSetShaderResources_Original;
extern D3D11_PSSetConstantBuffers_pfn                     D3D11_PSSetConstantBuffers_Original;
extern D3D11_UpdateSubresource_pfn                        D3D11_UpdateSubresource_Original;
extern D3D11_DrawIndexed_pfn                              D3D11_DrawIndexed_Original;
extern D3D11_Draw_pfn                                     D3D11_Draw_Original;
extern D3D11_DrawIndexedInstanced_pfn                     D3D11_DrawIndexedInstanced_Original;
extern D3D11_DrawIndexedInstancedIndirect_pfn             D3D11_DrawIndexedInstancedIndirect_Original;
extern D3D11_DrawInstanced_pfn                            D3D11_DrawInstanced_Original;
extern D3D11_DrawInstancedIndirect_pfn                    D3D11_DrawInstancedIndirect_Original;
extern D3D11_Map_pfn                                      D3D11_Map_Original;

extern D3D11_VSSetShader_pfn                              D3D11_VSSetShader_Original;
extern D3D11_PSSetShader_pfn                              D3D11_PSSetShader_Original;
extern D3D11_GSSetShader_pfn                              D3D11_GSSetShader_Original;
extern D3D11_HSSetShader_pfn                              D3D11_HSSetShader_Original;
extern D3D11_DSSetShader_pfn                              D3D11_DSSetShader_Original;
extern D3D11_CSSetShader_pfn                              D3D11_CSSetShader_Original;

extern D3D11_CopyResource_pfn                             D3D11_CopyResource_Original;
extern D3D11_CopySubresourceRegion_pfn                    D3D11_CopySubresourceRegion_Original;

extern D3D11_ClearState_pfn                               D3D11_ClearState_Original;
extern D3D11_ExecuteCommandList_pfn                       D3D11_ExecuteCommandList_Original;
extern D3D11_FinishCommandList_pfn                        D3D11_FinishCommandList_Original;

#endif /* __SK__DD11_INTERFAES_H__ */
