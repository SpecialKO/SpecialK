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

#define __SK_SUBSYSTEM__ L"  D3D 11  "

#include <Windows.h>

#include <SpecialK/diagnostics/cpu.h>
#include <SpecialK/diagnostics/modules.h>
#include <SpecialK/diagnostics/load_library.h>
#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/render/backend.h>

#include <SpecialK/diagnostics/debug_utils.h>
#include <SpecialK/diagnostics/compatibility.h>

#include <SpecialK/core.h>
#include <SpecialK/hooks.h>
#include <SpecialK/command.h>
#include <SpecialK/config.h>
#include <SpecialK/log.h>
#include <SpecialK/crc32.h>
#include <SpecialK/utility.h>
#include <SpecialK/thread.h>

#include <DirectXTex/DirectXTex.h>

#include <SpecialK/widgets/widget.h>
#include <SpecialK/framerate.h>
#include <SpecialK/tls.h>

#include <SpecialK/control_panel.h>

#define _SK_WITHOUT_D3DX11

extern LARGE_INTEGER SK_QueryPerf (void);
static DWORD         dwFrameTime = SK::ControlPanel::current_time; // For effects that blink; updated once per-frame.

#include <cinttypes>
#include <algorithm>
#include <memory>
#include <atomic>
#include <mutex>
#include <stack>
#include <concurrent_unordered_set.h>
#include <concurrent_queue.h>
#include <atlbase.h>

#include <d3d11.h>
#include <d3d11_1.h>

                                                                   /*-------------------------------------------------------------------------------------
                                                                   *
                                                                   * Copyright (c) Microsoft Corporation
                                                                   *
                                                                   *-------------------------------------------------------------------------------------*/


                                                                   /* this ALWAYS GENERATED file contains the definitions for the interfaces */


                                                                   /* File created by MIDL compiler version 8.01.0622 */
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

#ifndef __d3d11_3_h__
#define __d3d11_3_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

                                                                   /* Forward Declarations */ 

#ifndef __ID3D11Texture2D1_FWD_DEFINED__
#define __ID3D11Texture2D1_FWD_DEFINED__
typedef interface ID3D11Texture2D1 ID3D11Texture2D1;

#endif 	/* __ID3D11Texture2D1_FWD_DEFINED__ */


#ifndef __ID3D11Texture3D1_FWD_DEFINED__
#define __ID3D11Texture3D1_FWD_DEFINED__
typedef interface ID3D11Texture3D1 ID3D11Texture3D1;

#endif 	/* __ID3D11Texture3D1_FWD_DEFINED__ */


#ifndef __ID3D11RasterizerState2_FWD_DEFINED__
#define __ID3D11RasterizerState2_FWD_DEFINED__
typedef interface ID3D11RasterizerState2 ID3D11RasterizerState2;

#endif 	/* __ID3D11RasterizerState2_FWD_DEFINED__ */


#ifndef __ID3D11ShaderResourceView1_FWD_DEFINED__
#define __ID3D11ShaderResourceView1_FWD_DEFINED__
typedef interface ID3D11ShaderResourceView1 ID3D11ShaderResourceView1;

#endif 	/* __ID3D11ShaderResourceView1_FWD_DEFINED__ */


#ifndef __ID3D11RenderTargetView1_FWD_DEFINED__
#define __ID3D11RenderTargetView1_FWD_DEFINED__
typedef interface ID3D11RenderTargetView1 ID3D11RenderTargetView1;

#endif 	/* __ID3D11RenderTargetView1_FWD_DEFINED__ */


#ifndef __ID3D11UnorderedAccessView1_FWD_DEFINED__
#define __ID3D11UnorderedAccessView1_FWD_DEFINED__
typedef interface ID3D11UnorderedAccessView1 ID3D11UnorderedAccessView1;

#endif 	/* __ID3D11UnorderedAccessView1_FWD_DEFINED__ */


#ifndef __ID3D11Query1_FWD_DEFINED__
#define __ID3D11Query1_FWD_DEFINED__
typedef interface ID3D11Query1 ID3D11Query1;

#endif 	/* __ID3D11Query1_FWD_DEFINED__ */


#ifndef __ID3D11DeviceContext3_FWD_DEFINED__
#define __ID3D11DeviceContext3_FWD_DEFINED__
typedef interface ID3D11DeviceContext3 ID3D11DeviceContext3;

#endif 	/* __ID3D11DeviceContext3_FWD_DEFINED__ */


#ifndef __ID3D11Fence_FWD_DEFINED__
#define __ID3D11Fence_FWD_DEFINED__
typedef interface ID3D11Fence ID3D11Fence;

#endif 	/* __ID3D11Fence_FWD_DEFINED__ */


#ifndef __ID3D11DeviceContext4_FWD_DEFINED__
#define __ID3D11DeviceContext4_FWD_DEFINED__
typedef interface ID3D11DeviceContext4 ID3D11DeviceContext4;

#endif 	/* __ID3D11DeviceContext4_FWD_DEFINED__ */


#ifndef __ID3D11Device3_FWD_DEFINED__
#define __ID3D11Device3_FWD_DEFINED__
typedef interface ID3D11Device3 ID3D11Device3;

#endif 	/* __ID3D11Device3_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "dxgi1_3.h"
#include "d3dcommon.h"
#include "d3d11_2.h"

#ifdef __cplusplus
extern "C"{
#endif 


  /* interface __MIDL_itf_d3d11_3_0000_0000 */
  /* [local] */ 

#ifdef __cplusplus
}
#endif
#include "d3d11_2.h" //
#ifdef __cplusplus
extern "C"{
#endif
  typedef 
    enum D3D11_CONTEXT_TYPE
  {
    D3D11_CONTEXT_TYPE_ALL	= 0,
    D3D11_CONTEXT_TYPE_3D	= 1,
    D3D11_CONTEXT_TYPE_COMPUTE	= 2,
    D3D11_CONTEXT_TYPE_COPY	= 3,
    D3D11_CONTEXT_TYPE_VIDEO	= 4
  } 	D3D11_CONTEXT_TYPE;

  typedef 
    enum D3D11_TEXTURE_LAYOUT
  {
    D3D11_TEXTURE_LAYOUT_UNDEFINED	= 0,
    D3D11_TEXTURE_LAYOUT_ROW_MAJOR	= 1,
    D3D11_TEXTURE_LAYOUT_64K_STANDARD_SWIZZLE	= 2
  } 	D3D11_TEXTURE_LAYOUT;

  typedef struct D3D11_TEXTURE2D_DESC1
  {
    UINT Width;
    UINT Height;
    UINT MipLevels;
    UINT ArraySize;
    DXGI_FORMAT Format;
    DXGI_SAMPLE_DESC SampleDesc;
    D3D11_USAGE Usage;
    UINT BindFlags;
    UINT CPUAccessFlags;
    UINT MiscFlags;
    D3D11_TEXTURE_LAYOUT TextureLayout;
  } 	D3D11_TEXTURE2D_DESC1;

#if !defined( D3D11_NO_HELPERS ) && defined( __cplusplus )
}
struct CD3D11_TEXTURE2D_DESC1 : public D3D11_TEXTURE2D_DESC1
{
  CD3D11_TEXTURE2D_DESC1()
  {}
  explicit CD3D11_TEXTURE2D_DESC1( const D3D11_TEXTURE2D_DESC1& o ) :
    D3D11_TEXTURE2D_DESC1( o )
  {}
  explicit CD3D11_TEXTURE2D_DESC1(
    DXGI_FORMAT format,
    UINT width,
    UINT height,
    UINT arraySize = 1,
    UINT mipLevels = 0,
    UINT bindFlags = D3D11_BIND_SHADER_RESOURCE,
    D3D11_USAGE usage = D3D11_USAGE_DEFAULT,
    UINT cpuaccessFlags = 0,
    UINT sampleCount = 1,
    UINT sampleQuality = 0,
    UINT miscFlags = 0,
    D3D11_TEXTURE_LAYOUT textureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED)
  {
    Width = width;
    Height = height;
    MipLevels = mipLevels;
    ArraySize = arraySize;
    Format = format;
    SampleDesc.Count = sampleCount;
    SampleDesc.Quality = sampleQuality;
    Usage = usage;
    BindFlags = bindFlags;
    CPUAccessFlags = cpuaccessFlags;
    MiscFlags = miscFlags;
    TextureLayout = textureLayout;
  }
  explicit CD3D11_TEXTURE2D_DESC1(
    const D3D11_TEXTURE2D_DESC &desc,
    D3D11_TEXTURE_LAYOUT textureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED)
  {
    Width = desc.Width;
    Height = desc.Height;
    MipLevels = desc.MipLevels;
    ArraySize = desc.ArraySize;
    Format = desc.Format;
    SampleDesc.Count = desc.SampleDesc.Count;
    SampleDesc.Quality = desc. SampleDesc.Quality;
    Usage = desc.Usage;
    BindFlags = desc.BindFlags;
    CPUAccessFlags = desc.CPUAccessFlags;
    MiscFlags = desc.MiscFlags;
    TextureLayout = textureLayout;
  }
  ~CD3D11_TEXTURE2D_DESC1() {}
  operator const D3D11_TEXTURE2D_DESC1&() const { return *this; }
};
extern "C"{
#endif


  extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0000_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0000_v0_0_s_ifspec;

#ifndef __ID3D11Texture2D1_INTERFACE_DEFINED__
#define __ID3D11Texture2D1_INTERFACE_DEFINED__

  /* interface ID3D11Texture2D1 */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D11Texture2D1;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("51218251-1E33-4617-9CCB-4D3A4367E7BB")
    ID3D11Texture2D1 : public ID3D11Texture2D
  {
  public:
    virtual void STDMETHODCALLTYPE GetDesc1( 
      /* [annotation] */ 
      _Out_  D3D11_TEXTURE2D_DESC1 *pDesc) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D11Texture2D1Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D11Texture2D1 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D11Texture2D1 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D11Texture2D1 * This);

    void ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D11Texture2D1 * This,
      /* [annotation] */ 
      _Outptr_  ID3D11Device **ppDevice);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D11Texture2D1 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation] */ 
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D11Texture2D1 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_  UINT DataSize,
      /* [annotation] */ 
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D11Texture2D1 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_opt_  const IUnknown *pData);

    void ( STDMETHODCALLTYPE *GetType )( 
      ID3D11Texture2D1 * This,
      /* [annotation] */ 
      _Out_  D3D11_RESOURCE_DIMENSION *pResourceDimension);

    void ( STDMETHODCALLTYPE *SetEvictionPriority )( 
      ID3D11Texture2D1 * This,
      /* [annotation] */ 
      _In_  UINT EvictionPriority);

    UINT ( STDMETHODCALLTYPE *GetEvictionPriority )( 
      ID3D11Texture2D1 * This);

    void ( STDMETHODCALLTYPE *GetDesc )( 
      ID3D11Texture2D1 * This,
      /* [annotation] */ 
      _Out_  D3D11_TEXTURE2D_DESC *pDesc);

    void ( STDMETHODCALLTYPE *GetDesc1 )( 
      ID3D11Texture2D1 * This,
      /* [annotation] */ 
      _Out_  D3D11_TEXTURE2D_DESC1 *pDesc);

    END_INTERFACE
  } ID3D11Texture2D1Vtbl;

  interface ID3D11Texture2D1
  {
    CONST_VTBL struct ID3D11Texture2D1Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D11Texture2D1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11Texture2D1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11Texture2D1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11Texture2D1_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11Texture2D1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11Texture2D1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11Texture2D1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11Texture2D1_GetType(This,pResourceDimension)	\
    ( (This)->lpVtbl -> GetType(This,pResourceDimension) ) 

#define ID3D11Texture2D1_SetEvictionPriority(This,EvictionPriority)	\
    ( (This)->lpVtbl -> SetEvictionPriority(This,EvictionPriority) ) 

#define ID3D11Texture2D1_GetEvictionPriority(This)	\
    ( (This)->lpVtbl -> GetEvictionPriority(This) ) 


#define ID3D11Texture2D1_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 


#define ID3D11Texture2D1_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11Texture2D1_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_d3d11_3_0000_0001 */
  /* [local] */ 

  typedef struct D3D11_TEXTURE3D_DESC1
  {
    UINT Width;
    UINT Height;
    UINT Depth;
    UINT MipLevels;
    DXGI_FORMAT Format;
    D3D11_USAGE Usage;
    UINT BindFlags;
    UINT CPUAccessFlags;
    UINT MiscFlags;
    D3D11_TEXTURE_LAYOUT TextureLayout;
  } 	D3D11_TEXTURE3D_DESC1;

#if !defined( D3D11_NO_HELPERS ) && defined( __cplusplus )
}
struct CD3D11_TEXTURE3D_DESC1 : public D3D11_TEXTURE3D_DESC1
{
  CD3D11_TEXTURE3D_DESC1()
  {}
  explicit CD3D11_TEXTURE3D_DESC1( const D3D11_TEXTURE3D_DESC1& o ) :
    D3D11_TEXTURE3D_DESC1( o )
  {}
  explicit CD3D11_TEXTURE3D_DESC1(
    DXGI_FORMAT format,
    UINT width,
    UINT height,
    UINT depth,
    UINT mipLevels = 0,
    UINT bindFlags = D3D11_BIND_SHADER_RESOURCE,
    D3D11_USAGE usage = D3D11_USAGE_DEFAULT,
    UINT cpuaccessFlags = 0,
    UINT miscFlags = 0,
    D3D11_TEXTURE_LAYOUT textureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED)
  {
    Width = width;
    Height = height;
    Depth = depth;
    MipLevels = mipLevels;
    Format = format;
    Usage = usage;
    BindFlags = bindFlags;
    CPUAccessFlags = cpuaccessFlags;
    MiscFlags = miscFlags;
    TextureLayout = textureLayout;
  }
  explicit CD3D11_TEXTURE3D_DESC1(
    const D3D11_TEXTURE3D_DESC &desc,
    D3D11_TEXTURE_LAYOUT textureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED)
  {
    Width = desc.Width;
    Height = desc.Height;
    Depth = desc.Depth;
    MipLevels = desc.MipLevels;
    Format = desc.Format;
    Usage = desc.Usage;
    BindFlags = desc.BindFlags;
    CPUAccessFlags = desc.CPUAccessFlags;
    MiscFlags = desc.MiscFlags;
    TextureLayout = textureLayout;
  }
  ~CD3D11_TEXTURE3D_DESC1() {}
  operator const D3D11_TEXTURE3D_DESC1&() const { return *this; }
};
extern "C"{
#endif


  extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0001_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0001_v0_0_s_ifspec;

#ifndef __ID3D11Texture3D1_INTERFACE_DEFINED__
#define __ID3D11Texture3D1_INTERFACE_DEFINED__

  /* interface ID3D11Texture3D1 */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D11Texture3D1;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("0C711683-2853-4846-9BB0-F3E60639E46A")
    ID3D11Texture3D1 : public ID3D11Texture3D
  {
  public:
    virtual void STDMETHODCALLTYPE GetDesc1( 
      /* [annotation] */ 
      _Out_  D3D11_TEXTURE3D_DESC1 *pDesc) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D11Texture3D1Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D11Texture3D1 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D11Texture3D1 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D11Texture3D1 * This);

    void ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D11Texture3D1 * This,
      /* [annotation] */ 
      _Outptr_  ID3D11Device **ppDevice);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D11Texture3D1 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation] */ 
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D11Texture3D1 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_  UINT DataSize,
      /* [annotation] */ 
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D11Texture3D1 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_opt_  const IUnknown *pData);

    void ( STDMETHODCALLTYPE *GetType )( 
      ID3D11Texture3D1 * This,
      /* [annotation] */ 
      _Out_  D3D11_RESOURCE_DIMENSION *pResourceDimension);

    void ( STDMETHODCALLTYPE *SetEvictionPriority )( 
      ID3D11Texture3D1 * This,
      /* [annotation] */ 
      _In_  UINT EvictionPriority);

    UINT ( STDMETHODCALLTYPE *GetEvictionPriority )( 
      ID3D11Texture3D1 * This);

    void ( STDMETHODCALLTYPE *GetDesc )( 
      ID3D11Texture3D1 * This,
      /* [annotation] */ 
      _Out_  D3D11_TEXTURE3D_DESC *pDesc);

    void ( STDMETHODCALLTYPE *GetDesc1 )( 
      ID3D11Texture3D1 * This,
      /* [annotation] */ 
      _Out_  D3D11_TEXTURE3D_DESC1 *pDesc);

    END_INTERFACE
  } ID3D11Texture3D1Vtbl;

  interface ID3D11Texture3D1
  {
    CONST_VTBL struct ID3D11Texture3D1Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D11Texture3D1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11Texture3D1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11Texture3D1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11Texture3D1_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11Texture3D1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11Texture3D1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11Texture3D1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11Texture3D1_GetType(This,pResourceDimension)	\
    ( (This)->lpVtbl -> GetType(This,pResourceDimension) ) 

#define ID3D11Texture3D1_SetEvictionPriority(This,EvictionPriority)	\
    ( (This)->lpVtbl -> SetEvictionPriority(This,EvictionPriority) ) 

#define ID3D11Texture3D1_GetEvictionPriority(This)	\
    ( (This)->lpVtbl -> GetEvictionPriority(This) ) 


#define ID3D11Texture3D1_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 


#define ID3D11Texture3D1_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11Texture3D1_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_d3d11_3_0000_0002 */
  /* [local] */ 

  typedef 
    enum D3D11_CONSERVATIVE_RASTERIZATION_MODE
  {
    D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF	= 0,
    D3D11_CONSERVATIVE_RASTERIZATION_MODE_ON	= 1
  } 	D3D11_CONSERVATIVE_RASTERIZATION_MODE;

  typedef struct D3D11_RASTERIZER_DESC2
  {
    D3D11_FILL_MODE FillMode;
    D3D11_CULL_MODE CullMode;
    BOOL FrontCounterClockwise;
    INT DepthBias;
    FLOAT DepthBiasClamp;
    FLOAT SlopeScaledDepthBias;
    BOOL DepthClipEnable;
    BOOL ScissorEnable;
    BOOL MultisampleEnable;
    BOOL AntialiasedLineEnable;
    UINT ForcedSampleCount;
    D3D11_CONSERVATIVE_RASTERIZATION_MODE ConservativeRaster;
  } 	D3D11_RASTERIZER_DESC2;

#if !defined( D3D11_NO_HELPERS ) && defined( __cplusplus )
}
struct CD3D11_RASTERIZER_DESC2 : public D3D11_RASTERIZER_DESC2
{
  CD3D11_RASTERIZER_DESC2()
  {}
  explicit CD3D11_RASTERIZER_DESC2( const D3D11_RASTERIZER_DESC2& o ) :
    D3D11_RASTERIZER_DESC2( o )
  {}
  explicit CD3D11_RASTERIZER_DESC2( CD3D11_DEFAULT )
  {
    FillMode = D3D11_FILL_SOLID;
    CullMode = D3D11_CULL_BACK;
    FrontCounterClockwise = FALSE;
    DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
    DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
    SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    DepthClipEnable = TRUE;
    ScissorEnable = FALSE;
    MultisampleEnable = FALSE;
    AntialiasedLineEnable = FALSE;
    ForcedSampleCount = 0;
    ConservativeRaster = D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF;
  }
  explicit CD3D11_RASTERIZER_DESC2(
    D3D11_FILL_MODE fillMode,
    D3D11_CULL_MODE cullMode,
    BOOL frontCounterClockwise,
    INT depthBias,
    FLOAT depthBiasClamp,
    FLOAT slopeScaledDepthBias,
    BOOL depthClipEnable,
    BOOL scissorEnable,
    BOOL multisampleEnable,
    BOOL antialiasedLineEnable, 
    UINT forcedSampleCount, 
    D3D11_CONSERVATIVE_RASTERIZATION_MODE conservativeRaster )
  {
    FillMode = fillMode;
    CullMode = cullMode;
    FrontCounterClockwise = frontCounterClockwise;
    DepthBias = depthBias;
    DepthBiasClamp = depthBiasClamp;
    SlopeScaledDepthBias = slopeScaledDepthBias;
    DepthClipEnable = depthClipEnable;
    ScissorEnable = scissorEnable;
    MultisampleEnable = multisampleEnable;
    AntialiasedLineEnable = antialiasedLineEnable;
    ForcedSampleCount = forcedSampleCount;
    ConservativeRaster = conservativeRaster;
  }
  ~CD3D11_RASTERIZER_DESC2() {}
  operator const D3D11_RASTERIZER_DESC2&() const { return *this; }
};
extern "C"{
#endif


  extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0002_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0002_v0_0_s_ifspec;

#ifndef __ID3D11RasterizerState2_INTERFACE_DEFINED__
#define __ID3D11RasterizerState2_INTERFACE_DEFINED__

  /* interface ID3D11RasterizerState2 */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D11RasterizerState2;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("6fbd02fb-209f-46c4-b059-2ed15586a6ac")
    ID3D11RasterizerState2 : public ID3D11RasterizerState1
  {
  public:
    virtual void STDMETHODCALLTYPE GetDesc2( 
      /* [annotation] */ 
      _Out_  D3D11_RASTERIZER_DESC2 *pDesc) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D11RasterizerState2Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D11RasterizerState2 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D11RasterizerState2 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D11RasterizerState2 * This);

    void ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D11RasterizerState2 * This,
      /* [annotation] */ 
      _Outptr_  ID3D11Device **ppDevice);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D11RasterizerState2 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation] */ 
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D11RasterizerState2 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_  UINT DataSize,
      /* [annotation] */ 
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D11RasterizerState2 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_opt_  const IUnknown *pData);

    void ( STDMETHODCALLTYPE *GetDesc )( 
      ID3D11RasterizerState2 * This,
      /* [annotation] */ 
      _Out_  D3D11_RASTERIZER_DESC *pDesc);

    void ( STDMETHODCALLTYPE *GetDesc1 )( 
      ID3D11RasterizerState2 * This,
      /* [annotation] */ 
      _Out_  D3D11_RASTERIZER_DESC1 *pDesc);

    void ( STDMETHODCALLTYPE *GetDesc2 )( 
      ID3D11RasterizerState2 * This,
      /* [annotation] */ 
      _Out_  D3D11_RASTERIZER_DESC2 *pDesc);

    END_INTERFACE
  } ID3D11RasterizerState2Vtbl;

  interface ID3D11RasterizerState2
  {
    CONST_VTBL struct ID3D11RasterizerState2Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D11RasterizerState2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11RasterizerState2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11RasterizerState2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11RasterizerState2_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11RasterizerState2_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11RasterizerState2_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11RasterizerState2_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11RasterizerState2_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 


#define ID3D11RasterizerState2_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 


#define ID3D11RasterizerState2_GetDesc2(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc2(This,pDesc) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11RasterizerState2_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_d3d11_3_0000_0003 */
  /* [local] */ 

  typedef struct D3D11_TEX2D_SRV1
  {
    UINT MostDetailedMip;
    UINT MipLevels;
    UINT PlaneSlice;
  } 	D3D11_TEX2D_SRV1;

  typedef struct D3D11_TEX2D_ARRAY_SRV1
  {
    UINT MostDetailedMip;
    UINT MipLevels;
    UINT FirstArraySlice;
    UINT ArraySize;
    UINT PlaneSlice;
  } 	D3D11_TEX2D_ARRAY_SRV1;

  typedef struct D3D11_SHADER_RESOURCE_VIEW_DESC1
  {
    DXGI_FORMAT Format;
    D3D11_SRV_DIMENSION ViewDimension;
    union 
    {
      D3D11_BUFFER_SRV Buffer;
      D3D11_TEX1D_SRV Texture1D;
      D3D11_TEX1D_ARRAY_SRV Texture1DArray;
      D3D11_TEX2D_SRV1 Texture2D;
      D3D11_TEX2D_ARRAY_SRV1 Texture2DArray;
      D3D11_TEX2DMS_SRV Texture2DMS;
      D3D11_TEX2DMS_ARRAY_SRV Texture2DMSArray;
      D3D11_TEX3D_SRV Texture3D;
      D3D11_TEXCUBE_SRV TextureCube;
      D3D11_TEXCUBE_ARRAY_SRV TextureCubeArray;
      D3D11_BUFFEREX_SRV BufferEx;
    } 	;
  } 	D3D11_SHADER_RESOURCE_VIEW_DESC1;

#if !defined( D3D11_NO_HELPERS ) && defined( __cplusplus )
}
struct CD3D11_SHADER_RESOURCE_VIEW_DESC1 : public D3D11_SHADER_RESOURCE_VIEW_DESC1
{
  CD3D11_SHADER_RESOURCE_VIEW_DESC1()
  {}
  explicit CD3D11_SHADER_RESOURCE_VIEW_DESC1( const D3D11_SHADER_RESOURCE_VIEW_DESC1& o ) :
    D3D11_SHADER_RESOURCE_VIEW_DESC1( o )
  {}
  explicit CD3D11_SHADER_RESOURCE_VIEW_DESC1(
    D3D11_SRV_DIMENSION viewDimension,
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
    UINT mostDetailedMip = 0, // FirstElement for BUFFER
    UINT mipLevels = -1, // NumElements for BUFFER
    UINT firstArraySlice = 0, // First2DArrayFace for TEXTURECUBEARRAY
    UINT arraySize = -1, // NumCubes for TEXTURECUBEARRAY
    UINT flags = 0, // BUFFEREX only
    UINT planeSlice = 0 ) // Texture2D and Texture2DArray only
  {
    Format = format;
    ViewDimension = viewDimension;
    switch (viewDimension)
    {
      case D3D11_SRV_DIMENSION_BUFFER:
        Buffer.FirstElement = mostDetailedMip;
        Buffer.NumElements = mipLevels;
        break;
      case D3D11_SRV_DIMENSION_TEXTURE1D:
        Texture1D.MostDetailedMip = mostDetailedMip;
        Texture1D.MipLevels = mipLevels;
        break;
      case D3D11_SRV_DIMENSION_TEXTURE1DARRAY:
        Texture1DArray.MostDetailedMip = mostDetailedMip;
        Texture1DArray.MipLevels = mipLevels;
        Texture1DArray.FirstArraySlice = firstArraySlice;
        Texture1DArray.ArraySize = arraySize;
        break;
      case D3D11_SRV_DIMENSION_TEXTURE2D:
        Texture2D.MostDetailedMip = mostDetailedMip;
        Texture2D.MipLevels = mipLevels;
        Texture2D.PlaneSlice = planeSlice;
        break;
      case D3D11_SRV_DIMENSION_TEXTURE2DARRAY:
        Texture2DArray.MostDetailedMip = mostDetailedMip;
        Texture2DArray.MipLevels = mipLevels;
        Texture2DArray.FirstArraySlice = firstArraySlice;
        Texture2DArray.ArraySize = arraySize;
        Texture2DArray.PlaneSlice = planeSlice;
        break;
      case D3D11_SRV_DIMENSION_TEXTURE2DMS:
        break;
      case D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY:
        Texture2DMSArray.FirstArraySlice = firstArraySlice;
        Texture2DMSArray.ArraySize = arraySize;
        break;
      case D3D11_SRV_DIMENSION_TEXTURE3D:
        Texture3D.MostDetailedMip = mostDetailedMip;
        Texture3D.MipLevels = mipLevels;
        break;
      case D3D11_SRV_DIMENSION_TEXTURECUBE:
        TextureCube.MostDetailedMip = mostDetailedMip;
        TextureCube.MipLevels = mipLevels;
        break;
      case D3D11_SRV_DIMENSION_TEXTURECUBEARRAY:
        TextureCubeArray.MostDetailedMip = mostDetailedMip;
        TextureCubeArray.MipLevels = mipLevels;
        TextureCubeArray.First2DArrayFace = firstArraySlice;
        TextureCubeArray.NumCubes = arraySize;
        break;
      case D3D11_SRV_DIMENSION_BUFFEREX:
        BufferEx.FirstElement = mostDetailedMip;
        BufferEx.NumElements = mipLevels;
        BufferEx.Flags = flags;
        break;
      default: break;
    }
  }
  explicit CD3D11_SHADER_RESOURCE_VIEW_DESC1(
    _In_ ID3D11Buffer*,
    DXGI_FORMAT format,
    UINT firstElement,
    UINT numElements,
    UINT flags = 0 )
  {
    Format = format;
    ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
    BufferEx.FirstElement = firstElement;
    BufferEx.NumElements = numElements;
    BufferEx.Flags = flags;
  }
  explicit CD3D11_SHADER_RESOURCE_VIEW_DESC1(
    _In_ ID3D11Texture1D* pTex1D,
    D3D11_SRV_DIMENSION viewDimension,
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
    UINT mostDetailedMip = 0,
    UINT mipLevels = -1,
    UINT firstArraySlice = 0,
    UINT arraySize = -1 )
  {
    ViewDimension = viewDimension;
    if (DXGI_FORMAT_UNKNOWN == format || -1 == mipLevels ||
      (-1 == arraySize && D3D11_SRV_DIMENSION_TEXTURE1DARRAY == viewDimension))
    {
      D3D11_TEXTURE1D_DESC TexDesc;
      pTex1D->GetDesc( &TexDesc );
      if (DXGI_FORMAT_UNKNOWN == format) format = TexDesc.Format;
      if (-1 == mipLevels) mipLevels = TexDesc.MipLevels - mostDetailedMip;
      if (-1 == arraySize) arraySize = TexDesc.ArraySize - firstArraySlice;
    }
    Format = format;
    switch (viewDimension)
    {
      case D3D11_SRV_DIMENSION_TEXTURE1D:
        Texture1D.MostDetailedMip = mostDetailedMip;
        Texture1D.MipLevels = mipLevels;
        break;
      case D3D11_SRV_DIMENSION_TEXTURE1DARRAY:
        Texture1DArray.MostDetailedMip = mostDetailedMip;
        Texture1DArray.MipLevels = mipLevels;
        Texture1DArray.FirstArraySlice = firstArraySlice;
        Texture1DArray.ArraySize = arraySize;
        break;
      default: break;
    }
  }
  explicit CD3D11_SHADER_RESOURCE_VIEW_DESC1(
    _In_ ID3D11Texture2D* pTex2D,
    D3D11_SRV_DIMENSION viewDimension,
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
    UINT mostDetailedMip = 0,
    UINT mipLevels = -1,
    UINT firstArraySlice = 0, // First2DArrayFace for TEXTURECUBEARRAY
    UINT arraySize = -1,  // NumCubes for TEXTURECUBEARRAY
    UINT planeSlice = 0 ) // PlaneSlice for TEXTURE2D or TEXTURE2DARRAY
  {
    ViewDimension = viewDimension;
    if (DXGI_FORMAT_UNKNOWN == format || 
      (-1 == mipLevels &&
        D3D11_SRV_DIMENSION_TEXTURE2DMS != viewDimension &&
        D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY != viewDimension) ||
        (-1 == arraySize &&
        (D3D11_SRV_DIMENSION_TEXTURE2DARRAY == viewDimension ||
        D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY == viewDimension ||
        D3D11_SRV_DIMENSION_TEXTURECUBEARRAY == viewDimension)))
    {
      D3D11_TEXTURE2D_DESC TexDesc;
      pTex2D->GetDesc( &TexDesc );
      if (DXGI_FORMAT_UNKNOWN == format) format = TexDesc.Format;
      if (-1 == mipLevels) mipLevels = TexDesc.MipLevels - mostDetailedMip;
      if (-1 == arraySize)
      {
        arraySize = TexDesc.ArraySize - firstArraySlice;
        if (D3D11_SRV_DIMENSION_TEXTURECUBEARRAY == viewDimension) arraySize /= 6;
      }
    }
    Format = format;
    switch (viewDimension)
    {
      case D3D11_SRV_DIMENSION_TEXTURE2D:
        Texture2D.MostDetailedMip = mostDetailedMip;
        Texture2D.MipLevels = mipLevels;
        Texture2D.PlaneSlice = planeSlice;
        break;
      case D3D11_SRV_DIMENSION_TEXTURE2DARRAY:
        Texture2DArray.MostDetailedMip = mostDetailedMip;
        Texture2DArray.MipLevels = mipLevels;
        Texture2DArray.FirstArraySlice = firstArraySlice;
        Texture2DArray.ArraySize = arraySize;
        Texture2DArray.PlaneSlice = planeSlice;
        break;
      case D3D11_SRV_DIMENSION_TEXTURE2DMS:
        break;
      case D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY:
        Texture2DMSArray.FirstArraySlice = firstArraySlice;
        Texture2DMSArray.ArraySize = arraySize;
        break;
      case D3D11_SRV_DIMENSION_TEXTURECUBE:
        TextureCube.MostDetailedMip = mostDetailedMip;
        TextureCube.MipLevels = mipLevels;
        break;
      case D3D11_SRV_DIMENSION_TEXTURECUBEARRAY:
        TextureCubeArray.MostDetailedMip = mostDetailedMip;
        TextureCubeArray.MipLevels = mipLevels;
        TextureCubeArray.First2DArrayFace = firstArraySlice;
        TextureCubeArray.NumCubes = arraySize;
        break;
      default: break;
    }
  }
  explicit CD3D11_SHADER_RESOURCE_VIEW_DESC1(
    _In_ ID3D11Texture3D* pTex3D,
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
    UINT mostDetailedMip = 0,
    UINT mipLevels = -1 )
  {
    ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
    if (DXGI_FORMAT_UNKNOWN == format || -1 == mipLevels)
    {
      D3D11_TEXTURE3D_DESC TexDesc;
      pTex3D->GetDesc( &TexDesc );
      if (DXGI_FORMAT_UNKNOWN == format) format = TexDesc.Format;
      if (-1 == mipLevels) mipLevels = TexDesc.MipLevels - mostDetailedMip;
    }
    Format = format;
    Texture3D.MostDetailedMip = mostDetailedMip;
    Texture3D.MipLevels = mipLevels;
  }
  ~CD3D11_SHADER_RESOURCE_VIEW_DESC1() {}
  operator const D3D11_SHADER_RESOURCE_VIEW_DESC1&() const { return *this; }
};
extern "C"{
#endif


  extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0003_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0003_v0_0_s_ifspec;

#ifndef __ID3D11ShaderResourceView1_INTERFACE_DEFINED__
#define __ID3D11ShaderResourceView1_INTERFACE_DEFINED__

  /* interface ID3D11ShaderResourceView1 */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D11ShaderResourceView1;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("91308b87-9040-411d-8c67-c39253ce3802")
    ID3D11ShaderResourceView1 : public ID3D11ShaderResourceView
  {
  public:
    virtual void STDMETHODCALLTYPE GetDesc1( 
      /* [annotation] */ 
      _Out_  D3D11_SHADER_RESOURCE_VIEW_DESC1 *pDesc1) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D11ShaderResourceView1Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D11ShaderResourceView1 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D11ShaderResourceView1 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D11ShaderResourceView1 * This);

    void ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D11ShaderResourceView1 * This,
      /* [annotation] */ 
      _Outptr_  ID3D11Device **ppDevice);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D11ShaderResourceView1 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation] */ 
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D11ShaderResourceView1 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_  UINT DataSize,
      /* [annotation] */ 
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D11ShaderResourceView1 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_opt_  const IUnknown *pData);

    void ( STDMETHODCALLTYPE *GetResource )( 
      ID3D11ShaderResourceView1 * This,
      /* [annotation] */ 
      _Outptr_  ID3D11Resource **ppResource);

    void ( STDMETHODCALLTYPE *GetDesc )( 
      ID3D11ShaderResourceView1 * This,
      /* [annotation] */ 
      _Out_  D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc);

    void ( STDMETHODCALLTYPE *GetDesc1 )( 
      ID3D11ShaderResourceView1 * This,
      /* [annotation] */ 
      _Out_  D3D11_SHADER_RESOURCE_VIEW_DESC1 *pDesc1);

    END_INTERFACE
  } ID3D11ShaderResourceView1Vtbl;

  interface ID3D11ShaderResourceView1
  {
    CONST_VTBL struct ID3D11ShaderResourceView1Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D11ShaderResourceView1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11ShaderResourceView1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11ShaderResourceView1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11ShaderResourceView1_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11ShaderResourceView1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11ShaderResourceView1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11ShaderResourceView1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11ShaderResourceView1_GetResource(This,ppResource)	\
    ( (This)->lpVtbl -> GetResource(This,ppResource) ) 


#define ID3D11ShaderResourceView1_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 


#define ID3D11ShaderResourceView1_GetDesc1(This,pDesc1)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc1) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11ShaderResourceView1_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_d3d11_3_0000_0004 */
  /* [local] */ 

  typedef struct D3D11_TEX2D_RTV1
  {
    UINT MipSlice;
    UINT PlaneSlice;
  } 	D3D11_TEX2D_RTV1;

  typedef struct D3D11_TEX2D_ARRAY_RTV1
  {
    UINT MipSlice;
    UINT FirstArraySlice;
    UINT ArraySize;
    UINT PlaneSlice;
  } 	D3D11_TEX2D_ARRAY_RTV1;

  typedef struct D3D11_RENDER_TARGET_VIEW_DESC1
  {
    DXGI_FORMAT Format;
    D3D11_RTV_DIMENSION ViewDimension;
    union 
    {
      D3D11_BUFFER_RTV Buffer;
      D3D11_TEX1D_RTV Texture1D;
      D3D11_TEX1D_ARRAY_RTV Texture1DArray;
      D3D11_TEX2D_RTV1 Texture2D;
      D3D11_TEX2D_ARRAY_RTV1 Texture2DArray;
      D3D11_TEX2DMS_RTV Texture2DMS;
      D3D11_TEX2DMS_ARRAY_RTV Texture2DMSArray;
      D3D11_TEX3D_RTV Texture3D;
    } 	;
  } 	D3D11_RENDER_TARGET_VIEW_DESC1;

#if !defined( D3D11_NO_HELPERS ) && defined( __cplusplus )
}
struct CD3D11_RENDER_TARGET_VIEW_DESC1 : public D3D11_RENDER_TARGET_VIEW_DESC1
{
  CD3D11_RENDER_TARGET_VIEW_DESC1()
  {}
  explicit CD3D11_RENDER_TARGET_VIEW_DESC1( const D3D11_RENDER_TARGET_VIEW_DESC1& o ) :
    D3D11_RENDER_TARGET_VIEW_DESC1( o )
  {}
  explicit CD3D11_RENDER_TARGET_VIEW_DESC1(
    D3D11_RTV_DIMENSION viewDimension,
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
    UINT mipSlice = 0, // FirstElement for BUFFER
    UINT firstArraySlice = 0, // NumElements for BUFFER, FirstWSlice for TEXTURE3D
    UINT arraySize = -1, // WSize for TEXTURE3D
    UINT planeSlice = 0 ) // PlaneSlice for TEXTURE2D and TEXTURE2DARRAY
  {
    Format = format;
    ViewDimension = viewDimension;
    switch (viewDimension)
    {
      case D3D11_RTV_DIMENSION_BUFFER:
        Buffer.FirstElement = mipSlice;
        Buffer.NumElements = firstArraySlice;
        break;
      case D3D11_RTV_DIMENSION_TEXTURE1D:
        Texture1D.MipSlice = mipSlice;
        break;
      case D3D11_RTV_DIMENSION_TEXTURE1DARRAY:
        Texture1DArray.MipSlice = mipSlice;
        Texture1DArray.FirstArraySlice = firstArraySlice;
        Texture1DArray.ArraySize = arraySize;
        break;
      case D3D11_RTV_DIMENSION_TEXTURE2D:
        Texture2D.MipSlice = mipSlice;
        Texture2D.PlaneSlice = planeSlice;
        break;
      case D3D11_RTV_DIMENSION_TEXTURE2DARRAY:
        Texture2DArray.MipSlice = mipSlice;
        Texture2DArray.FirstArraySlice = firstArraySlice;
        Texture2DArray.ArraySize = arraySize;
        Texture2DArray.PlaneSlice = planeSlice;
        break;
      case D3D11_RTV_DIMENSION_TEXTURE2DMS:
        break;
      case D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY:
        Texture2DMSArray.FirstArraySlice = firstArraySlice;
        Texture2DMSArray.ArraySize = arraySize;
        break;
      case D3D11_RTV_DIMENSION_TEXTURE3D:
        Texture3D.MipSlice = mipSlice;
        Texture3D.FirstWSlice = firstArraySlice;
        Texture3D.WSize = arraySize;
        break;
      default: break;
    }
  }
  explicit CD3D11_RENDER_TARGET_VIEW_DESC1(
    _In_ ID3D11Buffer*,
    DXGI_FORMAT format,
    UINT firstElement,
    UINT numElements )
  {
    Format = format;
    ViewDimension = D3D11_RTV_DIMENSION_BUFFER;
    Buffer.FirstElement = firstElement;
    Buffer.NumElements = numElements;
  }
  explicit CD3D11_RENDER_TARGET_VIEW_DESC1(
    _In_ ID3D11Texture1D* pTex1D,
    D3D11_RTV_DIMENSION viewDimension,
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
    UINT mipSlice = 0,
    UINT firstArraySlice = 0,
    UINT arraySize = -1 )
  {
    ViewDimension = viewDimension;
    if (DXGI_FORMAT_UNKNOWN == format ||
      (-1 == arraySize && D3D11_RTV_DIMENSION_TEXTURE1DARRAY == viewDimension))
    {
      D3D11_TEXTURE1D_DESC TexDesc;
      pTex1D->GetDesc( &TexDesc );
      if (DXGI_FORMAT_UNKNOWN == format) format = TexDesc.Format;
      if (-1 == arraySize) arraySize = TexDesc.ArraySize - firstArraySlice;
    }
    Format = format;
    switch (viewDimension)
    {
      case D3D11_RTV_DIMENSION_TEXTURE1D:
        Texture1D.MipSlice = mipSlice;
        break;
      case D3D11_RTV_DIMENSION_TEXTURE1DARRAY:
        Texture1DArray.MipSlice = mipSlice;
        Texture1DArray.FirstArraySlice = firstArraySlice;
        Texture1DArray.ArraySize = arraySize;
        break;
      default: break;
    }
  }
  explicit CD3D11_RENDER_TARGET_VIEW_DESC1(
    _In_ ID3D11Texture2D* pTex2D,
    D3D11_RTV_DIMENSION viewDimension,
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
    UINT mipSlice = 0,
    UINT firstArraySlice = 0,
    UINT arraySize = -1,
    UINT planeSlice = 0 )
  {
    ViewDimension = viewDimension;
    if (DXGI_FORMAT_UNKNOWN == format || 
      (-1 == arraySize &&
        (D3D11_RTV_DIMENSION_TEXTURE2DARRAY == viewDimension ||
        D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY == viewDimension)))
    {
      D3D11_TEXTURE2D_DESC TexDesc;
      pTex2D->GetDesc( &TexDesc );
      if (DXGI_FORMAT_UNKNOWN == format) format = TexDesc.Format;
      if (-1 == arraySize) arraySize = TexDesc.ArraySize - firstArraySlice;
    }
    Format = format;
    switch (viewDimension)
    {
      case D3D11_RTV_DIMENSION_TEXTURE2D:
        Texture2D.MipSlice = mipSlice;
        Texture2D.PlaneSlice = planeSlice;
        break;
      case D3D11_RTV_DIMENSION_TEXTURE2DARRAY:
        Texture2DArray.MipSlice = mipSlice;
        Texture2DArray.FirstArraySlice = firstArraySlice;
        Texture2DArray.ArraySize = arraySize;
        Texture2DArray.PlaneSlice = planeSlice;
        break;
      case D3D11_RTV_DIMENSION_TEXTURE2DMS:
        break;
      case D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY:
        Texture2DMSArray.FirstArraySlice = firstArraySlice;
        Texture2DMSArray.ArraySize = arraySize;
        break;
      default: break;
    }
  }
  explicit CD3D11_RENDER_TARGET_VIEW_DESC1(
    _In_ ID3D11Texture3D* pTex3D,
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
    UINT mipSlice = 0,
    UINT firstWSlice = 0,
    UINT wSize = -1 )
  {
    ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
    if (DXGI_FORMAT_UNKNOWN == format || -1 == wSize)
    {
      D3D11_TEXTURE3D_DESC TexDesc;
      pTex3D->GetDesc( &TexDesc );
      if (DXGI_FORMAT_UNKNOWN == format) format = TexDesc.Format;
      if (-1 == wSize) wSize = TexDesc.Depth - firstWSlice;
    }
    Format = format;
    Texture3D.MipSlice = mipSlice;
    Texture3D.FirstWSlice = firstWSlice;
    Texture3D.WSize = wSize;
  }
  ~CD3D11_RENDER_TARGET_VIEW_DESC1() {}
  operator const D3D11_RENDER_TARGET_VIEW_DESC1&() const { return *this; }
};
extern "C"{
#endif


  extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0004_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0004_v0_0_s_ifspec;

#ifndef __ID3D11RenderTargetView1_INTERFACE_DEFINED__
#define __ID3D11RenderTargetView1_INTERFACE_DEFINED__

  /* interface ID3D11RenderTargetView1 */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D11RenderTargetView1;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("ffbe2e23-f011-418a-ac56-5ceed7c5b94b")
    ID3D11RenderTargetView1 : public ID3D11RenderTargetView
  {
  public:
    virtual void STDMETHODCALLTYPE GetDesc1( 
      /* [annotation] */ 
      _Out_  D3D11_RENDER_TARGET_VIEW_DESC1 *pDesc1) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D11RenderTargetView1Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D11RenderTargetView1 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D11RenderTargetView1 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D11RenderTargetView1 * This);

    void ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D11RenderTargetView1 * This,
      /* [annotation] */ 
      _Outptr_  ID3D11Device **ppDevice);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D11RenderTargetView1 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation] */ 
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D11RenderTargetView1 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_  UINT DataSize,
      /* [annotation] */ 
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D11RenderTargetView1 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_opt_  const IUnknown *pData);

    void ( STDMETHODCALLTYPE *GetResource )( 
      ID3D11RenderTargetView1 * This,
      /* [annotation] */ 
      _Outptr_  ID3D11Resource **ppResource);

    void ( STDMETHODCALLTYPE *GetDesc )( 
      ID3D11RenderTargetView1 * This,
      /* [annotation] */ 
      _Out_  D3D11_RENDER_TARGET_VIEW_DESC *pDesc);

    void ( STDMETHODCALLTYPE *GetDesc1 )( 
      ID3D11RenderTargetView1 * This,
      /* [annotation] */ 
      _Out_  D3D11_RENDER_TARGET_VIEW_DESC1 *pDesc1);

    END_INTERFACE
  } ID3D11RenderTargetView1Vtbl;

  interface ID3D11RenderTargetView1
  {
    CONST_VTBL struct ID3D11RenderTargetView1Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D11RenderTargetView1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11RenderTargetView1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11RenderTargetView1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11RenderTargetView1_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11RenderTargetView1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11RenderTargetView1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11RenderTargetView1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11RenderTargetView1_GetResource(This,ppResource)	\
    ( (This)->lpVtbl -> GetResource(This,ppResource) ) 


#define ID3D11RenderTargetView1_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 


#define ID3D11RenderTargetView1_GetDesc1(This,pDesc1)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc1) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11RenderTargetView1_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_d3d11_3_0000_0005 */
  /* [local] */ 

  typedef struct D3D11_TEX2D_UAV1
  {
    UINT MipSlice;
    UINT PlaneSlice;
  } 	D3D11_TEX2D_UAV1;

  typedef struct D3D11_TEX2D_ARRAY_UAV1
  {
    UINT MipSlice;
    UINT FirstArraySlice;
    UINT ArraySize;
    UINT PlaneSlice;
  } 	D3D11_TEX2D_ARRAY_UAV1;

  typedef struct D3D11_UNORDERED_ACCESS_VIEW_DESC1
  {
    DXGI_FORMAT Format;
    D3D11_UAV_DIMENSION ViewDimension;
    union 
    {
      D3D11_BUFFER_UAV Buffer;
      D3D11_TEX1D_UAV Texture1D;
      D3D11_TEX1D_ARRAY_UAV Texture1DArray;
      D3D11_TEX2D_UAV1 Texture2D;
      D3D11_TEX2D_ARRAY_UAV1 Texture2DArray;
      D3D11_TEX3D_UAV Texture3D;
    } 	;
  } 	D3D11_UNORDERED_ACCESS_VIEW_DESC1;

#if !defined( D3D11_NO_HELPERS ) && defined( __cplusplus )
}
struct CD3D11_UNORDERED_ACCESS_VIEW_DESC1 : public D3D11_UNORDERED_ACCESS_VIEW_DESC1
{
  CD3D11_UNORDERED_ACCESS_VIEW_DESC1()
  {}
  explicit CD3D11_UNORDERED_ACCESS_VIEW_DESC1( const D3D11_UNORDERED_ACCESS_VIEW_DESC1& o ) :
    D3D11_UNORDERED_ACCESS_VIEW_DESC1( o )
  {}
  explicit CD3D11_UNORDERED_ACCESS_VIEW_DESC1(
    D3D11_UAV_DIMENSION viewDimension,
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
    UINT mipSlice = 0, // FirstElement for BUFFER
    UINT firstArraySlice = 0, // NumElements for BUFFER, FirstWSlice for TEXTURE3D
    UINT arraySize = -1, // WSize for TEXTURE3D
    UINT flags = 0, // BUFFER only
    UINT planeSlice = 0 ) // PlaneSlice for TEXTURE2D and TEXTURE2DARRAY
  {
    Format = format;
    ViewDimension = viewDimension;
    switch (viewDimension)
    {
      case D3D11_UAV_DIMENSION_BUFFER:
        Buffer.FirstElement = mipSlice;
        Buffer.NumElements = firstArraySlice;
        Buffer.Flags = flags;
        break;
      case D3D11_UAV_DIMENSION_TEXTURE1D:
        Texture1D.MipSlice = mipSlice;
        break;
      case D3D11_UAV_DIMENSION_TEXTURE1DARRAY:
        Texture1DArray.MipSlice = mipSlice;
        Texture1DArray.FirstArraySlice = firstArraySlice;
        Texture1DArray.ArraySize = arraySize;
        break;
      case D3D11_UAV_DIMENSION_TEXTURE2D:
        Texture2D.MipSlice = mipSlice;
        Texture2D.PlaneSlice = planeSlice;
        break;
      case D3D11_UAV_DIMENSION_TEXTURE2DARRAY:
        Texture2DArray.MipSlice = mipSlice;
        Texture2DArray.FirstArraySlice = firstArraySlice;
        Texture2DArray.ArraySize = arraySize;
        Texture2DArray.PlaneSlice = planeSlice;
        break;
      case D3D11_UAV_DIMENSION_TEXTURE3D:
        Texture3D.MipSlice = mipSlice;
        Texture3D.FirstWSlice = firstArraySlice;
        Texture3D.WSize = arraySize;
        break;
      default: break;
    }
  }
  explicit CD3D11_UNORDERED_ACCESS_VIEW_DESC1(
    _In_ ID3D11Buffer*,
    DXGI_FORMAT format,
    UINT firstElement,
    UINT numElements,
    UINT flags = 0 )
  {
    Format = format;
    ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    Buffer.FirstElement = firstElement;
    Buffer.NumElements = numElements;
    Buffer.Flags = flags;
  }
  explicit CD3D11_UNORDERED_ACCESS_VIEW_DESC1(
    _In_ ID3D11Texture1D* pTex1D,
    D3D11_UAV_DIMENSION viewDimension,
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
    UINT mipSlice = 0,
    UINT firstArraySlice = 0,
    UINT arraySize = -1 )
  {
    ViewDimension = viewDimension;
    if (DXGI_FORMAT_UNKNOWN == format ||
      (-1 == arraySize && D3D11_UAV_DIMENSION_TEXTURE1DARRAY == viewDimension))
    {
      D3D11_TEXTURE1D_DESC TexDesc;
      pTex1D->GetDesc( &TexDesc );
      if (DXGI_FORMAT_UNKNOWN == format) format = TexDesc.Format;
      if (-1 == arraySize) arraySize = TexDesc.ArraySize - firstArraySlice;
    }
    Format = format;
    switch (viewDimension)
    {
      case D3D11_UAV_DIMENSION_TEXTURE1D:
        Texture1D.MipSlice = mipSlice;
        break;
      case D3D11_UAV_DIMENSION_TEXTURE1DARRAY:
        Texture1DArray.MipSlice = mipSlice;
        Texture1DArray.FirstArraySlice = firstArraySlice;
        Texture1DArray.ArraySize = arraySize;
        break;
      default: break;
    }
  }
  explicit CD3D11_UNORDERED_ACCESS_VIEW_DESC1(
    _In_ ID3D11Texture2D* pTex2D,
    D3D11_UAV_DIMENSION viewDimension,
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
    UINT mipSlice = 0,
    UINT firstArraySlice = 0,
    UINT arraySize = -1, 
    UINT planeSlice = 0 ) 
  {
    ViewDimension = viewDimension;
    if (DXGI_FORMAT_UNKNOWN == format || 
      (-1 == arraySize && D3D11_UAV_DIMENSION_TEXTURE2DARRAY == viewDimension))
    {
      D3D11_TEXTURE2D_DESC TexDesc;
      pTex2D->GetDesc( &TexDesc );
      if (DXGI_FORMAT_UNKNOWN == format) format = TexDesc.Format;
      if (-1 == arraySize) arraySize = TexDesc.ArraySize - firstArraySlice;
    }
    Format = format;
    switch (viewDimension)
    {
      case D3D11_UAV_DIMENSION_TEXTURE2D:
        Texture2D.MipSlice = mipSlice;
        Texture2D.PlaneSlice = planeSlice;
        break;
      case D3D11_UAV_DIMENSION_TEXTURE2DARRAY:
        Texture2DArray.MipSlice = mipSlice;
        Texture2DArray.FirstArraySlice = firstArraySlice;
        Texture2DArray.ArraySize = arraySize;
        Texture2DArray.PlaneSlice = planeSlice;
        break;
      default: break;
    }
  }
  explicit CD3D11_UNORDERED_ACCESS_VIEW_DESC1(
    _In_ ID3D11Texture3D* pTex3D,
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
    UINT mipSlice = 0,
    UINT firstWSlice = 0,
    UINT wSize = -1 )
  {
    ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
    if (DXGI_FORMAT_UNKNOWN == format || -1 == wSize)
    {
      D3D11_TEXTURE3D_DESC TexDesc;
      pTex3D->GetDesc( &TexDesc );
      if (DXGI_FORMAT_UNKNOWN == format) format = TexDesc.Format;
      if (-1 == wSize) wSize = TexDesc.Depth - firstWSlice;
    }
    Format = format;
    Texture3D.MipSlice = mipSlice;
    Texture3D.FirstWSlice = firstWSlice;
    Texture3D.WSize = wSize;
  }
  ~CD3D11_UNORDERED_ACCESS_VIEW_DESC1() {}
  operator const D3D11_UNORDERED_ACCESS_VIEW_DESC1&() const { return *this; }
};
extern "C"{
#endif


  extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0005_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0005_v0_0_s_ifspec;

#ifndef __ID3D11UnorderedAccessView1_INTERFACE_DEFINED__
#define __ID3D11UnorderedAccessView1_INTERFACE_DEFINED__

  /* interface ID3D11UnorderedAccessView1 */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D11UnorderedAccessView1;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("7b3b6153-a886-4544-ab37-6537c8500403")
    ID3D11UnorderedAccessView1 : public ID3D11UnorderedAccessView
  {
  public:
    virtual void STDMETHODCALLTYPE GetDesc1( 
      /* [annotation] */ 
      _Out_  D3D11_UNORDERED_ACCESS_VIEW_DESC1 *pDesc1) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D11UnorderedAccessView1Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D11UnorderedAccessView1 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D11UnorderedAccessView1 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D11UnorderedAccessView1 * This);

    void ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D11UnorderedAccessView1 * This,
      /* [annotation] */ 
      _Outptr_  ID3D11Device **ppDevice);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D11UnorderedAccessView1 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation] */ 
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D11UnorderedAccessView1 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_  UINT DataSize,
      /* [annotation] */ 
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D11UnorderedAccessView1 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_opt_  const IUnknown *pData);

    void ( STDMETHODCALLTYPE *GetResource )( 
      ID3D11UnorderedAccessView1 * This,
      /* [annotation] */ 
      _Outptr_  ID3D11Resource **ppResource);

    void ( STDMETHODCALLTYPE *GetDesc )( 
      ID3D11UnorderedAccessView1 * This,
      /* [annotation] */ 
      _Out_  D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc);

    void ( STDMETHODCALLTYPE *GetDesc1 )( 
      ID3D11UnorderedAccessView1 * This,
      /* [annotation] */ 
      _Out_  D3D11_UNORDERED_ACCESS_VIEW_DESC1 *pDesc1);

    END_INTERFACE
  } ID3D11UnorderedAccessView1Vtbl;

  interface ID3D11UnorderedAccessView1
  {
    CONST_VTBL struct ID3D11UnorderedAccessView1Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D11UnorderedAccessView1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11UnorderedAccessView1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11UnorderedAccessView1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11UnorderedAccessView1_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11UnorderedAccessView1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11UnorderedAccessView1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11UnorderedAccessView1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11UnorderedAccessView1_GetResource(This,ppResource)	\
    ( (This)->lpVtbl -> GetResource(This,ppResource) ) 


#define ID3D11UnorderedAccessView1_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 


#define ID3D11UnorderedAccessView1_GetDesc1(This,pDesc1)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc1) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11UnorderedAccessView1_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_d3d11_3_0000_0006 */
  /* [local] */ 

  typedef struct D3D11_QUERY_DESC1
  {
    D3D11_QUERY Query;
    UINT MiscFlags;
    D3D11_CONTEXT_TYPE ContextType;
  } 	D3D11_QUERY_DESC1;

#if !defined( D3D11_NO_HELPERS ) && defined( __cplusplus )
}
struct CD3D11_QUERY_DESC1 : public D3D11_QUERY_DESC1
{
  CD3D11_QUERY_DESC1()
  {}
  explicit CD3D11_QUERY_DESC1( const D3D11_QUERY_DESC1& o ) :
    D3D11_QUERY_DESC1( o )
  {}
  explicit CD3D11_QUERY_DESC1(
    D3D11_QUERY query,
    UINT miscFlags = 0,
    D3D11_CONTEXT_TYPE contextType = D3D11_CONTEXT_TYPE_ALL )
  {
    Query = query;
    MiscFlags = miscFlags;
    ContextType = contextType;
  }
  ~CD3D11_QUERY_DESC1() {}
  operator const D3D11_QUERY_DESC1&() const { return *this; }
};
extern "C"{
#endif


  extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0006_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0006_v0_0_s_ifspec;

#ifndef __ID3D11Query1_INTERFACE_DEFINED__
#define __ID3D11Query1_INTERFACE_DEFINED__

  /* interface ID3D11Query1 */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D11Query1;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("631b4766-36dc-461d-8db6-c47e13e60916")
    ID3D11Query1 : public ID3D11Query
  {
  public:
    virtual void STDMETHODCALLTYPE GetDesc1( 
      /* [annotation] */ 
      _Out_  D3D11_QUERY_DESC1 *pDesc1) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D11Query1Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D11Query1 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D11Query1 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D11Query1 * This);

    void ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D11Query1 * This,
      /* [annotation] */ 
      _Outptr_  ID3D11Device **ppDevice);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D11Query1 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation] */ 
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D11Query1 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_  UINT DataSize,
      /* [annotation] */ 
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D11Query1 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_opt_  const IUnknown *pData);

    UINT ( STDMETHODCALLTYPE *GetDataSize )( 
      ID3D11Query1 * This);

    void ( STDMETHODCALLTYPE *GetDesc )( 
      ID3D11Query1 * This,
      /* [annotation] */ 
      _Out_  D3D11_QUERY_DESC *pDesc);

    void ( STDMETHODCALLTYPE *GetDesc1 )( 
      ID3D11Query1 * This,
      /* [annotation] */ 
      _Out_  D3D11_QUERY_DESC1 *pDesc1);

    END_INTERFACE
  } ID3D11Query1Vtbl;

  interface ID3D11Query1
  {
    CONST_VTBL struct ID3D11Query1Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D11Query1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11Query1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11Query1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11Query1_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11Query1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11Query1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11Query1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11Query1_GetDataSize(This)	\
    ( (This)->lpVtbl -> GetDataSize(This) ) 


#define ID3D11Query1_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 


#define ID3D11Query1_GetDesc1(This,pDesc1)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc1) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11Query1_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_d3d11_3_0000_0007 */
  /* [local] */ 

  typedef 
    enum D3D11_FENCE_FLAG
  {
    D3D11_FENCE_FLAG_NONE	= 0x1,
    D3D11_FENCE_FLAG_SHARED	= 0x2,
    D3D11_FENCE_FLAG_SHARED_CROSS_ADAPTER	= 0x4
  } 	D3D11_FENCE_FLAG;

  DEFINE_ENUM_FLAG_OPERATORS(D3D11_FENCE_FLAG);


  extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0007_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0007_v0_0_s_ifspec;

#ifndef __ID3D11DeviceContext3_INTERFACE_DEFINED__
#define __ID3D11DeviceContext3_INTERFACE_DEFINED__

  /* interface ID3D11DeviceContext3 */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D11DeviceContext3;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("b4e3c01d-e79e-4637-91b2-510e9f4c9b8f")
    ID3D11DeviceContext3 : public ID3D11DeviceContext2
  {
  public:
    virtual void STDMETHODCALLTYPE Flush1( 
      D3D11_CONTEXT_TYPE ContextType,
      /* [annotation] */ 
      _In_opt_  HANDLE hEvent) = 0;

    virtual void STDMETHODCALLTYPE SetHardwareProtectionState( 
      /* [annotation] */ 
      _In_  BOOL HwProtectionEnable) = 0;

    virtual void STDMETHODCALLTYPE GetHardwareProtectionState( 
      /* [annotation] */ 
      _Out_  BOOL *pHwProtectionEnable) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D11DeviceContext3Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D11DeviceContext3 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D11DeviceContext3 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D11DeviceContext3 * This);

    void ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _Outptr_  ID3D11Device **ppDevice);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation] */ 
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_  UINT DataSize,
      /* [annotation] */ 
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_opt_  const IUnknown *pData);

    void ( STDMETHODCALLTYPE *VSSetConstantBuffers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);

    void ( STDMETHODCALLTYPE *PSSetShaderResources )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *PSSetShader )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11PixelShader *pPixelShader,
      /* [annotation] */ 
      _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
      UINT NumClassInstances);

    void ( STDMETHODCALLTYPE *PSSetSamplers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);

    void ( STDMETHODCALLTYPE *VSSetShader )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11VertexShader *pVertexShader,
      /* [annotation] */ 
      _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
      UINT NumClassInstances);

    void ( STDMETHODCALLTYPE *DrawIndexed )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  UINT IndexCount,
      /* [annotation] */ 
      _In_  UINT StartIndexLocation,
      /* [annotation] */ 
      _In_  INT BaseVertexLocation);

    void ( STDMETHODCALLTYPE *Draw )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  UINT VertexCount,
      /* [annotation] */ 
      _In_  UINT StartVertexLocation);

    HRESULT ( STDMETHODCALLTYPE *Map )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pResource,
      /* [annotation] */ 
      _In_  UINT Subresource,
      /* [annotation] */ 
      _In_  D3D11_MAP MapType,
      /* [annotation] */ 
      _In_  UINT MapFlags,
      /* [annotation] */ 
      _Out_opt_  D3D11_MAPPED_SUBRESOURCE *pMappedResource);

    void ( STDMETHODCALLTYPE *Unmap )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pResource,
      /* [annotation] */ 
      _In_  UINT Subresource);

    void ( STDMETHODCALLTYPE *PSSetConstantBuffers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);

    void ( STDMETHODCALLTYPE *IASetInputLayout )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11InputLayout *pInputLayout);

    void ( STDMETHODCALLTYPE *IASetVertexBuffers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppVertexBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pStrides,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pOffsets);

    void ( STDMETHODCALLTYPE *IASetIndexBuffer )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11Buffer *pIndexBuffer,
      /* [annotation] */ 
      _In_  DXGI_FORMAT Format,
      /* [annotation] */ 
      _In_  UINT Offset);

    void ( STDMETHODCALLTYPE *DrawIndexedInstanced )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  UINT IndexCountPerInstance,
      /* [annotation] */ 
      _In_  UINT InstanceCount,
      /* [annotation] */ 
      _In_  UINT StartIndexLocation,
      /* [annotation] */ 
      _In_  INT BaseVertexLocation,
      /* [annotation] */ 
      _In_  UINT StartInstanceLocation);

    void ( STDMETHODCALLTYPE *DrawInstanced )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  UINT VertexCountPerInstance,
      /* [annotation] */ 
      _In_  UINT InstanceCount,
      /* [annotation] */ 
      _In_  UINT StartVertexLocation,
      /* [annotation] */ 
      _In_  UINT StartInstanceLocation);

    void ( STDMETHODCALLTYPE *GSSetConstantBuffers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);

    void ( STDMETHODCALLTYPE *GSSetShader )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11GeometryShader *pShader,
      /* [annotation] */ 
      _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
      UINT NumClassInstances);

    void ( STDMETHODCALLTYPE *IASetPrimitiveTopology )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  D3D11_PRIMITIVE_TOPOLOGY Topology);

    void ( STDMETHODCALLTYPE *VSSetShaderResources )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *VSSetSamplers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);

    void ( STDMETHODCALLTYPE *Begin )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Asynchronous *pAsync);

    void ( STDMETHODCALLTYPE *End )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Asynchronous *pAsync);

    HRESULT ( STDMETHODCALLTYPE *GetData )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Asynchronous *pAsync,
      /* [annotation] */ 
      _Out_writes_bytes_opt_( DataSize )  void *pData,
      /* [annotation] */ 
      _In_  UINT DataSize,
      /* [annotation] */ 
      _In_  UINT GetDataFlags);

    void ( STDMETHODCALLTYPE *SetPredication )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11Predicate *pPredicate,
      /* [annotation] */ 
      _In_  BOOL PredicateValue);

    void ( STDMETHODCALLTYPE *GSSetShaderResources )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *GSSetSamplers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);

    void ( STDMETHODCALLTYPE *OMSetRenderTargets )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT )  UINT NumViews,
      /* [annotation] */ 
      _In_reads_opt_(NumViews)  ID3D11RenderTargetView *const *ppRenderTargetViews,
      /* [annotation] */ 
      _In_opt_  ID3D11DepthStencilView *pDepthStencilView);

    void ( STDMETHODCALLTYPE *OMSetRenderTargetsAndUnorderedAccessViews )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  UINT NumRTVs,
      /* [annotation] */ 
      _In_reads_opt_(NumRTVs)  ID3D11RenderTargetView *const *ppRenderTargetViews,
      /* [annotation] */ 
      _In_opt_  ID3D11DepthStencilView *pDepthStencilView,
      /* [annotation] */ 
      _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - 1 )  UINT UAVStartSlot,
      /* [annotation] */ 
      _In_  UINT NumUAVs,
      /* [annotation] */ 
      _In_reads_opt_(NumUAVs)  ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
      /* [annotation] */ 
      _In_reads_opt_(NumUAVs)  const UINT *pUAVInitialCounts);

    void ( STDMETHODCALLTYPE *OMSetBlendState )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11BlendState *pBlendState,
      /* [annotation] */ 
      _In_opt_  const FLOAT BlendFactor[ 4 ],
      /* [annotation] */ 
      _In_  UINT SampleMask);

    void ( STDMETHODCALLTYPE *OMSetDepthStencilState )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11DepthStencilState *pDepthStencilState,
      /* [annotation] */ 
      _In_  UINT StencilRef);

    void ( STDMETHODCALLTYPE *SOSetTargets )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_SO_BUFFER_SLOT_COUNT)  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppSOTargets,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pOffsets);

    void ( STDMETHODCALLTYPE *DrawAuto )( 
      ID3D11DeviceContext3 * This);

    void ( STDMETHODCALLTYPE *DrawIndexedInstancedIndirect )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Buffer *pBufferForArgs,
      /* [annotation] */ 
      _In_  UINT AlignedByteOffsetForArgs);

    void ( STDMETHODCALLTYPE *DrawInstancedIndirect )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Buffer *pBufferForArgs,
      /* [annotation] */ 
      _In_  UINT AlignedByteOffsetForArgs);

    void ( STDMETHODCALLTYPE *Dispatch )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  UINT ThreadGroupCountX,
      /* [annotation] */ 
      _In_  UINT ThreadGroupCountY,
      /* [annotation] */ 
      _In_  UINT ThreadGroupCountZ);

    void ( STDMETHODCALLTYPE *DispatchIndirect )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Buffer *pBufferForArgs,
      /* [annotation] */ 
      _In_  UINT AlignedByteOffsetForArgs);

    void ( STDMETHODCALLTYPE *RSSetState )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11RasterizerState *pRasterizerState);

    void ( STDMETHODCALLTYPE *RSSetViewports )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumViewports,
      /* [annotation] */ 
      _In_reads_opt_(NumViewports)  const D3D11_VIEWPORT *pViewports);

    void ( STDMETHODCALLTYPE *RSSetScissorRects )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumRects,
      /* [annotation] */ 
      _In_reads_opt_(NumRects)  const D3D11_RECT *pRects);

    void ( STDMETHODCALLTYPE *CopySubresourceRegion )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pDstResource,
      /* [annotation] */ 
      _In_  UINT DstSubresource,
      /* [annotation] */ 
      _In_  UINT DstX,
      /* [annotation] */ 
      _In_  UINT DstY,
      /* [annotation] */ 
      _In_  UINT DstZ,
      /* [annotation] */ 
      _In_  ID3D11Resource *pSrcResource,
      /* [annotation] */ 
      _In_  UINT SrcSubresource,
      /* [annotation] */ 
      _In_opt_  const D3D11_BOX *pSrcBox);

    void ( STDMETHODCALLTYPE *CopyResource )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pDstResource,
      /* [annotation] */ 
      _In_  ID3D11Resource *pSrcResource);

    void ( STDMETHODCALLTYPE *UpdateSubresource )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pDstResource,
      /* [annotation] */ 
      _In_  UINT DstSubresource,
      /* [annotation] */ 
      _In_opt_  const D3D11_BOX *pDstBox,
      /* [annotation] */ 
      _In_  const void *pSrcData,
      /* [annotation] */ 
      _In_  UINT SrcRowPitch,
      /* [annotation] */ 
      _In_  UINT SrcDepthPitch);

    void ( STDMETHODCALLTYPE *CopyStructureCount )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Buffer *pDstBuffer,
      /* [annotation] */ 
      _In_  UINT DstAlignedByteOffset,
      /* [annotation] */ 
      _In_  ID3D11UnorderedAccessView *pSrcView);

    void ( STDMETHODCALLTYPE *ClearRenderTargetView )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11RenderTargetView *pRenderTargetView,
      /* [annotation] */ 
      _In_  const FLOAT ColorRGBA[ 4 ]);

    void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewUint )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11UnorderedAccessView *pUnorderedAccessView,
      /* [annotation] */ 
      _In_  const UINT Values[ 4 ]);

    void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewFloat )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11UnorderedAccessView *pUnorderedAccessView,
      /* [annotation] */ 
      _In_  const FLOAT Values[ 4 ]);

    void ( STDMETHODCALLTYPE *ClearDepthStencilView )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11DepthStencilView *pDepthStencilView,
      /* [annotation] */ 
      _In_  UINT ClearFlags,
      /* [annotation] */ 
      _In_  FLOAT Depth,
      /* [annotation] */ 
      _In_  UINT8 Stencil);

    void ( STDMETHODCALLTYPE *GenerateMips )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11ShaderResourceView *pShaderResourceView);

    void ( STDMETHODCALLTYPE *SetResourceMinLOD )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pResource,
      FLOAT MinLOD);

    FLOAT ( STDMETHODCALLTYPE *GetResourceMinLOD )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pResource);

    void ( STDMETHODCALLTYPE *ResolveSubresource )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pDstResource,
      /* [annotation] */ 
      _In_  UINT DstSubresource,
      /* [annotation] */ 
      _In_  ID3D11Resource *pSrcResource,
      /* [annotation] */ 
      _In_  UINT SrcSubresource,
      /* [annotation] */ 
      _In_  DXGI_FORMAT Format);

    void ( STDMETHODCALLTYPE *ExecuteCommandList )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11CommandList *pCommandList,
      BOOL RestoreContextState);

    void ( STDMETHODCALLTYPE *HSSetShaderResources )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *HSSetShader )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11HullShader *pHullShader,
      /* [annotation] */ 
      _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
      UINT NumClassInstances);

    void ( STDMETHODCALLTYPE *HSSetSamplers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);

    void ( STDMETHODCALLTYPE *HSSetConstantBuffers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);

    void ( STDMETHODCALLTYPE *DSSetShaderResources )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *DSSetShader )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11DomainShader *pDomainShader,
      /* [annotation] */ 
      _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
      UINT NumClassInstances);

    void ( STDMETHODCALLTYPE *DSSetSamplers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);

    void ( STDMETHODCALLTYPE *DSSetConstantBuffers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);

    void ( STDMETHODCALLTYPE *CSSetShaderResources )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *CSSetUnorderedAccessViews )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - StartSlot )  UINT NumUAVs,
      /* [annotation] */ 
      _In_reads_opt_(NumUAVs)  ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
      /* [annotation] */ 
      _In_reads_opt_(NumUAVs)  const UINT *pUAVInitialCounts);

    void ( STDMETHODCALLTYPE *CSSetShader )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11ComputeShader *pComputeShader,
      /* [annotation] */ 
      _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
      UINT NumClassInstances);

    void ( STDMETHODCALLTYPE *CSSetSamplers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);

    void ( STDMETHODCALLTYPE *CSSetConstantBuffers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);

    void ( STDMETHODCALLTYPE *VSGetConstantBuffers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);

    void ( STDMETHODCALLTYPE *PSGetShaderResources )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *PSGetShader )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _Outptr_result_maybenull_  ID3D11PixelShader **ppPixelShader,
      /* [annotation] */ 
      _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
      /* [annotation] */ 
      _Inout_opt_  UINT *pNumClassInstances);

    void ( STDMETHODCALLTYPE *PSGetSamplers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);

    void ( STDMETHODCALLTYPE *VSGetShader )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _Outptr_result_maybenull_  ID3D11VertexShader **ppVertexShader,
      /* [annotation] */ 
      _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
      /* [annotation] */ 
      _Inout_opt_  UINT *pNumClassInstances);

    void ( STDMETHODCALLTYPE *PSGetConstantBuffers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);

    void ( STDMETHODCALLTYPE *IAGetInputLayout )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _Outptr_result_maybenull_  ID3D11InputLayout **ppInputLayout);

    void ( STDMETHODCALLTYPE *IAGetVertexBuffers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppVertexBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pStrides,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pOffsets);

    void ( STDMETHODCALLTYPE *IAGetIndexBuffer )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _Outptr_opt_result_maybenull_  ID3D11Buffer **pIndexBuffer,
      /* [annotation] */ 
      _Out_opt_  DXGI_FORMAT *Format,
      /* [annotation] */ 
      _Out_opt_  UINT *Offset);

    void ( STDMETHODCALLTYPE *GSGetConstantBuffers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);

    void ( STDMETHODCALLTYPE *GSGetShader )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _Outptr_result_maybenull_  ID3D11GeometryShader **ppGeometryShader,
      /* [annotation] */ 
      _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
      /* [annotation] */ 
      _Inout_opt_  UINT *pNumClassInstances);

    void ( STDMETHODCALLTYPE *IAGetPrimitiveTopology )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _Out_  D3D11_PRIMITIVE_TOPOLOGY *pTopology);

    void ( STDMETHODCALLTYPE *VSGetShaderResources )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *VSGetSamplers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);

    void ( STDMETHODCALLTYPE *GetPredication )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _Outptr_opt_result_maybenull_  ID3D11Predicate **ppPredicate,
      /* [annotation] */ 
      _Out_opt_  BOOL *pPredicateValue);

    void ( STDMETHODCALLTYPE *GSGetShaderResources )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *GSGetSamplers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);

    void ( STDMETHODCALLTYPE *OMGetRenderTargets )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT )  UINT NumViews,
      /* [annotation] */ 
      _Out_writes_opt_(NumViews)  ID3D11RenderTargetView **ppRenderTargetViews,
      /* [annotation] */ 
      _Outptr_opt_result_maybenull_  ID3D11DepthStencilView **ppDepthStencilView);

    void ( STDMETHODCALLTYPE *OMGetRenderTargetsAndUnorderedAccessViews )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT )  UINT NumRTVs,
      /* [annotation] */ 
      _Out_writes_opt_(NumRTVs)  ID3D11RenderTargetView **ppRenderTargetViews,
      /* [annotation] */ 
      _Outptr_opt_result_maybenull_  ID3D11DepthStencilView **ppDepthStencilView,
      /* [annotation] */ 
      _In_range_( 0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1 )  UINT UAVStartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_PS_CS_UAV_REGISTER_COUNT - UAVStartSlot )  UINT NumUAVs,
      /* [annotation] */ 
      _Out_writes_opt_(NumUAVs)  ID3D11UnorderedAccessView **ppUnorderedAccessViews);

    void ( STDMETHODCALLTYPE *OMGetBlendState )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _Outptr_opt_result_maybenull_  ID3D11BlendState **ppBlendState,
      /* [annotation] */ 
      _Out_opt_  FLOAT BlendFactor[ 4 ],
      /* [annotation] */ 
      _Out_opt_  UINT *pSampleMask);

    void ( STDMETHODCALLTYPE *OMGetDepthStencilState )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _Outptr_opt_result_maybenull_  ID3D11DepthStencilState **ppDepthStencilState,
      /* [annotation] */ 
      _Out_opt_  UINT *pStencilRef);

    void ( STDMETHODCALLTYPE *SOGetTargets )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_SO_BUFFER_SLOT_COUNT )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppSOTargets);

    void ( STDMETHODCALLTYPE *RSGetState )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _Outptr_result_maybenull_  ID3D11RasterizerState **ppRasterizerState);

    void ( STDMETHODCALLTYPE *RSGetViewports )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumViewports,
      /* [annotation] */ 
      _Out_writes_opt_(*pNumViewports)  D3D11_VIEWPORT *pViewports);

    void ( STDMETHODCALLTYPE *RSGetScissorRects )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumRects,
      /* [annotation] */ 
      _Out_writes_opt_(*pNumRects)  D3D11_RECT *pRects);

    void ( STDMETHODCALLTYPE *HSGetShaderResources )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *HSGetShader )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _Outptr_result_maybenull_  ID3D11HullShader **ppHullShader,
      /* [annotation] */ 
      _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
      /* [annotation] */ 
      _Inout_opt_  UINT *pNumClassInstances);

    void ( STDMETHODCALLTYPE *HSGetSamplers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);

    void ( STDMETHODCALLTYPE *HSGetConstantBuffers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);

    void ( STDMETHODCALLTYPE *DSGetShaderResources )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *DSGetShader )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _Outptr_result_maybenull_  ID3D11DomainShader **ppDomainShader,
      /* [annotation] */ 
      _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
      /* [annotation] */ 
      _Inout_opt_  UINT *pNumClassInstances);

    void ( STDMETHODCALLTYPE *DSGetSamplers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);

    void ( STDMETHODCALLTYPE *DSGetConstantBuffers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);

    void ( STDMETHODCALLTYPE *CSGetShaderResources )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *CSGetUnorderedAccessViews )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - StartSlot )  UINT NumUAVs,
      /* [annotation] */ 
      _Out_writes_opt_(NumUAVs)  ID3D11UnorderedAccessView **ppUnorderedAccessViews);

    void ( STDMETHODCALLTYPE *CSGetShader )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _Outptr_result_maybenull_  ID3D11ComputeShader **ppComputeShader,
      /* [annotation] */ 
      _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
      /* [annotation] */ 
      _Inout_opt_  UINT *pNumClassInstances);

    void ( STDMETHODCALLTYPE *CSGetSamplers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);

    void ( STDMETHODCALLTYPE *CSGetConstantBuffers )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);

    void ( STDMETHODCALLTYPE *ClearState )( 
      ID3D11DeviceContext3 * This);

    void ( STDMETHODCALLTYPE *Flush )( 
      ID3D11DeviceContext3 * This);

    D3D11_DEVICE_CONTEXT_TYPE ( STDMETHODCALLTYPE *GetType )( 
      ID3D11DeviceContext3 * This);

    UINT ( STDMETHODCALLTYPE *GetContextFlags )( 
      ID3D11DeviceContext3 * This);

    HRESULT ( STDMETHODCALLTYPE *FinishCommandList )( 
      ID3D11DeviceContext3 * This,
      BOOL RestoreDeferredContextState,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11CommandList **ppCommandList);

    void ( STDMETHODCALLTYPE *CopySubresourceRegion1 )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pDstResource,
      /* [annotation] */ 
      _In_  UINT DstSubresource,
      /* [annotation] */ 
      _In_  UINT DstX,
      /* [annotation] */ 
      _In_  UINT DstY,
      /* [annotation] */ 
      _In_  UINT DstZ,
      /* [annotation] */ 
      _In_  ID3D11Resource *pSrcResource,
      /* [annotation] */ 
      _In_  UINT SrcSubresource,
      /* [annotation] */ 
      _In_opt_  const D3D11_BOX *pSrcBox,
      /* [annotation] */ 
      _In_  UINT CopyFlags);

    void ( STDMETHODCALLTYPE *UpdateSubresource1 )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pDstResource,
      /* [annotation] */ 
      _In_  UINT DstSubresource,
      /* [annotation] */ 
      _In_opt_  const D3D11_BOX *pDstBox,
      /* [annotation] */ 
      _In_  const void *pSrcData,
      /* [annotation] */ 
      _In_  UINT SrcRowPitch,
      /* [annotation] */ 
      _In_  UINT SrcDepthPitch,
      /* [annotation] */ 
      _In_  UINT CopyFlags);

    void ( STDMETHODCALLTYPE *DiscardResource )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pResource);

    void ( STDMETHODCALLTYPE *DiscardView )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11View *pResourceView);

    void ( STDMETHODCALLTYPE *VSSetConstantBuffers1 )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *HSSetConstantBuffers1 )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *DSSetConstantBuffers1 )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *GSSetConstantBuffers1 )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *PSSetConstantBuffers1 )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *CSSetConstantBuffers1 )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *VSGetConstantBuffers1 )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *HSGetConstantBuffers1 )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *DSGetConstantBuffers1 )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *GSGetConstantBuffers1 )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *PSGetConstantBuffers1 )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *CSGetConstantBuffers1 )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *SwapDeviceContextState )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3DDeviceContextState *pState,
      /* [annotation] */ 
      _Outptr_opt_  ID3DDeviceContextState **ppPreviousState);

    void ( STDMETHODCALLTYPE *ClearView )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11View *pView,
      /* [annotation] */ 
      _In_  const FLOAT Color[ 4 ],
      /* [annotation] */ 
      _In_reads_opt_(NumRects)  const D3D11_RECT *pRect,
      UINT NumRects);

    void ( STDMETHODCALLTYPE *DiscardView1 )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11View *pResourceView,
      /* [annotation] */ 
      _In_reads_opt_(NumRects)  const D3D11_RECT *pRects,
      UINT NumRects);

    HRESULT ( STDMETHODCALLTYPE *UpdateTileMappings )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pTiledResource,
      /* [annotation] */ 
      _In_  UINT NumTiledResourceRegions,
      /* [annotation] */ 
      _In_reads_opt_(NumTiledResourceRegions)  const D3D11_TILED_RESOURCE_COORDINATE *pTiledResourceRegionStartCoordinates,
      /* [annotation] */ 
      _In_reads_opt_(NumTiledResourceRegions)  const D3D11_TILE_REGION_SIZE *pTiledResourceRegionSizes,
      /* [annotation] */ 
      _In_opt_  ID3D11Buffer *pTilePool,
      /* [annotation] */ 
      _In_  UINT NumRanges,
      /* [annotation] */ 
      _In_reads_opt_(NumRanges)  const UINT *pRangeFlags,
      /* [annotation] */ 
      _In_reads_opt_(NumRanges)  const UINT *pTilePoolStartOffsets,
      /* [annotation] */ 
      _In_reads_opt_(NumRanges)  const UINT *pRangeTileCounts,
      /* [annotation] */ 
      _In_  UINT Flags);

    HRESULT ( STDMETHODCALLTYPE *CopyTileMappings )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pDestTiledResource,
      /* [annotation] */ 
      _In_  const D3D11_TILED_RESOURCE_COORDINATE *pDestRegionStartCoordinate,
      /* [annotation] */ 
      _In_  ID3D11Resource *pSourceTiledResource,
      /* [annotation] */ 
      _In_  const D3D11_TILED_RESOURCE_COORDINATE *pSourceRegionStartCoordinate,
      /* [annotation] */ 
      _In_  const D3D11_TILE_REGION_SIZE *pTileRegionSize,
      /* [annotation] */ 
      _In_  UINT Flags);

    void ( STDMETHODCALLTYPE *CopyTiles )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pTiledResource,
      /* [annotation] */ 
      _In_  const D3D11_TILED_RESOURCE_COORDINATE *pTileRegionStartCoordinate,
      /* [annotation] */ 
      _In_  const D3D11_TILE_REGION_SIZE *pTileRegionSize,
      /* [annotation] */ 
      _In_  ID3D11Buffer *pBuffer,
      /* [annotation] */ 
      _In_  UINT64 BufferStartOffsetInBytes,
      /* [annotation] */ 
      _In_  UINT Flags);

    void ( STDMETHODCALLTYPE *UpdateTiles )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pDestTiledResource,
      /* [annotation] */ 
      _In_  const D3D11_TILED_RESOURCE_COORDINATE *pDestTileRegionStartCoordinate,
      /* [annotation] */ 
      _In_  const D3D11_TILE_REGION_SIZE *pDestTileRegionSize,
      /* [annotation] */ 
      _In_  const void *pSourceTileData,
      /* [annotation] */ 
      _In_  UINT Flags);

    HRESULT ( STDMETHODCALLTYPE *ResizeTilePool )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  ID3D11Buffer *pTilePool,
      /* [annotation] */ 
      _In_  UINT64 NewSizeInBytes);

    void ( STDMETHODCALLTYPE *TiledResourceBarrier )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11DeviceChild *pTiledResourceOrViewAccessBeforeBarrier,
      /* [annotation] */ 
      _In_opt_  ID3D11DeviceChild *pTiledResourceOrViewAccessAfterBarrier);

    BOOL ( STDMETHODCALLTYPE *IsAnnotationEnabled )( 
      ID3D11DeviceContext3 * This);

    void ( STDMETHODCALLTYPE *SetMarkerInt )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  LPCWSTR pLabel,
      INT Data);

    void ( STDMETHODCALLTYPE *BeginEventInt )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  LPCWSTR pLabel,
      INT Data);

    void ( STDMETHODCALLTYPE *EndEvent )( 
      ID3D11DeviceContext3 * This);

    void ( STDMETHODCALLTYPE *Flush1 )( 
      ID3D11DeviceContext3 * This,
      D3D11_CONTEXT_TYPE ContextType,
      /* [annotation] */ 
      _In_opt_  HANDLE hEvent);

    void ( STDMETHODCALLTYPE *SetHardwareProtectionState )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _In_  BOOL HwProtectionEnable);

    void ( STDMETHODCALLTYPE *GetHardwareProtectionState )( 
      ID3D11DeviceContext3 * This,
      /* [annotation] */ 
      _Out_  BOOL *pHwProtectionEnable);

    END_INTERFACE
  } ID3D11DeviceContext3Vtbl;

  interface ID3D11DeviceContext3
  {
    CONST_VTBL struct ID3D11DeviceContext3Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D11DeviceContext3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11DeviceContext3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11DeviceContext3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11DeviceContext3_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11DeviceContext3_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11DeviceContext3_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11DeviceContext3_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11DeviceContext3_VSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> VSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_PSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> PSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_PSSetShader(This,pPixelShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> PSSetShader(This,pPixelShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext3_PSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> PSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_VSSetShader(This,pVertexShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> VSSetShader(This,pVertexShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext3_DrawIndexed(This,IndexCount,StartIndexLocation,BaseVertexLocation)	\
    ( (This)->lpVtbl -> DrawIndexed(This,IndexCount,StartIndexLocation,BaseVertexLocation) ) 

#define ID3D11DeviceContext3_Draw(This,VertexCount,StartVertexLocation)	\
    ( (This)->lpVtbl -> Draw(This,VertexCount,StartVertexLocation) ) 

#define ID3D11DeviceContext3_Map(This,pResource,Subresource,MapType,MapFlags,pMappedResource)	\
    ( (This)->lpVtbl -> Map(This,pResource,Subresource,MapType,MapFlags,pMappedResource) ) 

#define ID3D11DeviceContext3_Unmap(This,pResource,Subresource)	\
    ( (This)->lpVtbl -> Unmap(This,pResource,Subresource) ) 

#define ID3D11DeviceContext3_PSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> PSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_IASetInputLayout(This,pInputLayout)	\
    ( (This)->lpVtbl -> IASetInputLayout(This,pInputLayout) ) 

#define ID3D11DeviceContext3_IASetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets)	\
    ( (This)->lpVtbl -> IASetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets) ) 

#define ID3D11DeviceContext3_IASetIndexBuffer(This,pIndexBuffer,Format,Offset)	\
    ( (This)->lpVtbl -> IASetIndexBuffer(This,pIndexBuffer,Format,Offset) ) 

#define ID3D11DeviceContext3_DrawIndexedInstanced(This,IndexCountPerInstance,InstanceCount,StartIndexLocation,BaseVertexLocation,StartInstanceLocation)	\
    ( (This)->lpVtbl -> DrawIndexedInstanced(This,IndexCountPerInstance,InstanceCount,StartIndexLocation,BaseVertexLocation,StartInstanceLocation) ) 

#define ID3D11DeviceContext3_DrawInstanced(This,VertexCountPerInstance,InstanceCount,StartVertexLocation,StartInstanceLocation)	\
    ( (This)->lpVtbl -> DrawInstanced(This,VertexCountPerInstance,InstanceCount,StartVertexLocation,StartInstanceLocation) ) 

#define ID3D11DeviceContext3_GSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> GSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_GSSetShader(This,pShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> GSSetShader(This,pShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext3_IASetPrimitiveTopology(This,Topology)	\
    ( (This)->lpVtbl -> IASetPrimitiveTopology(This,Topology) ) 

#define ID3D11DeviceContext3_VSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> VSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_VSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> VSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_Begin(This,pAsync)	\
    ( (This)->lpVtbl -> Begin(This,pAsync) ) 

#define ID3D11DeviceContext3_End(This,pAsync)	\
    ( (This)->lpVtbl -> End(This,pAsync) ) 

#define ID3D11DeviceContext3_GetData(This,pAsync,pData,DataSize,GetDataFlags)	\
    ( (This)->lpVtbl -> GetData(This,pAsync,pData,DataSize,GetDataFlags) ) 

#define ID3D11DeviceContext3_SetPredication(This,pPredicate,PredicateValue)	\
    ( (This)->lpVtbl -> SetPredication(This,pPredicate,PredicateValue) ) 

#define ID3D11DeviceContext3_GSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> GSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_GSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> GSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_OMSetRenderTargets(This,NumViews,ppRenderTargetViews,pDepthStencilView)	\
    ( (This)->lpVtbl -> OMSetRenderTargets(This,NumViews,ppRenderTargetViews,pDepthStencilView) ) 

#define ID3D11DeviceContext3_OMSetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,pDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts)	\
    ( (This)->lpVtbl -> OMSetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,pDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts) ) 

#define ID3D11DeviceContext3_OMSetBlendState(This,pBlendState,BlendFactor,SampleMask)	\
    ( (This)->lpVtbl -> OMSetBlendState(This,pBlendState,BlendFactor,SampleMask) ) 

#define ID3D11DeviceContext3_OMSetDepthStencilState(This,pDepthStencilState,StencilRef)	\
    ( (This)->lpVtbl -> OMSetDepthStencilState(This,pDepthStencilState,StencilRef) ) 

#define ID3D11DeviceContext3_SOSetTargets(This,NumBuffers,ppSOTargets,pOffsets)	\
    ( (This)->lpVtbl -> SOSetTargets(This,NumBuffers,ppSOTargets,pOffsets) ) 

#define ID3D11DeviceContext3_DrawAuto(This)	\
    ( (This)->lpVtbl -> DrawAuto(This) ) 

#define ID3D11DeviceContext3_DrawIndexedInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs)	\
    ( (This)->lpVtbl -> DrawIndexedInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs) ) 

#define ID3D11DeviceContext3_DrawInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs)	\
    ( (This)->lpVtbl -> DrawInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs) ) 

#define ID3D11DeviceContext3_Dispatch(This,ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ)	\
    ( (This)->lpVtbl -> Dispatch(This,ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ) ) 

#define ID3D11DeviceContext3_DispatchIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs)	\
    ( (This)->lpVtbl -> DispatchIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs) ) 

#define ID3D11DeviceContext3_RSSetState(This,pRasterizerState)	\
    ( (This)->lpVtbl -> RSSetState(This,pRasterizerState) ) 

#define ID3D11DeviceContext3_RSSetViewports(This,NumViewports,pViewports)	\
    ( (This)->lpVtbl -> RSSetViewports(This,NumViewports,pViewports) ) 

#define ID3D11DeviceContext3_RSSetScissorRects(This,NumRects,pRects)	\
    ( (This)->lpVtbl -> RSSetScissorRects(This,NumRects,pRects) ) 

#define ID3D11DeviceContext3_CopySubresourceRegion(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox)	\
    ( (This)->lpVtbl -> CopySubresourceRegion(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox) ) 

#define ID3D11DeviceContext3_CopyResource(This,pDstResource,pSrcResource)	\
    ( (This)->lpVtbl -> CopyResource(This,pDstResource,pSrcResource) ) 

#define ID3D11DeviceContext3_UpdateSubresource(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch)	\
    ( (This)->lpVtbl -> UpdateSubresource(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch) ) 

#define ID3D11DeviceContext3_CopyStructureCount(This,pDstBuffer,DstAlignedByteOffset,pSrcView)	\
    ( (This)->lpVtbl -> CopyStructureCount(This,pDstBuffer,DstAlignedByteOffset,pSrcView) ) 

#define ID3D11DeviceContext3_ClearRenderTargetView(This,pRenderTargetView,ColorRGBA)	\
    ( (This)->lpVtbl -> ClearRenderTargetView(This,pRenderTargetView,ColorRGBA) ) 

#define ID3D11DeviceContext3_ClearUnorderedAccessViewUint(This,pUnorderedAccessView,Values)	\
    ( (This)->lpVtbl -> ClearUnorderedAccessViewUint(This,pUnorderedAccessView,Values) ) 

#define ID3D11DeviceContext3_ClearUnorderedAccessViewFloat(This,pUnorderedAccessView,Values)	\
    ( (This)->lpVtbl -> ClearUnorderedAccessViewFloat(This,pUnorderedAccessView,Values) ) 

#define ID3D11DeviceContext3_ClearDepthStencilView(This,pDepthStencilView,ClearFlags,Depth,Stencil)	\
    ( (This)->lpVtbl -> ClearDepthStencilView(This,pDepthStencilView,ClearFlags,Depth,Stencil) ) 

#define ID3D11DeviceContext3_GenerateMips(This,pShaderResourceView)	\
    ( (This)->lpVtbl -> GenerateMips(This,pShaderResourceView) ) 

#define ID3D11DeviceContext3_SetResourceMinLOD(This,pResource,MinLOD)	\
    ( (This)->lpVtbl -> SetResourceMinLOD(This,pResource,MinLOD) ) 

#define ID3D11DeviceContext3_GetResourceMinLOD(This,pResource)	\
    ( (This)->lpVtbl -> GetResourceMinLOD(This,pResource) ) 

#define ID3D11DeviceContext3_ResolveSubresource(This,pDstResource,DstSubresource,pSrcResource,SrcSubresource,Format)	\
    ( (This)->lpVtbl -> ResolveSubresource(This,pDstResource,DstSubresource,pSrcResource,SrcSubresource,Format) ) 

#define ID3D11DeviceContext3_ExecuteCommandList(This,pCommandList,RestoreContextState)	\
    ( (This)->lpVtbl -> ExecuteCommandList(This,pCommandList,RestoreContextState) ) 

#define ID3D11DeviceContext3_HSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> HSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_HSSetShader(This,pHullShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> HSSetShader(This,pHullShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext3_HSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> HSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_HSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> HSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_DSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> DSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_DSSetShader(This,pDomainShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> DSSetShader(This,pDomainShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext3_DSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> DSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_DSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> DSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_CSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> CSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_CSSetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts)	\
    ( (This)->lpVtbl -> CSSetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts) ) 

#define ID3D11DeviceContext3_CSSetShader(This,pComputeShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> CSSetShader(This,pComputeShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext3_CSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> CSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_CSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> CSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_VSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> VSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_PSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> PSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_PSGetShader(This,ppPixelShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> PSGetShader(This,ppPixelShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext3_PSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> PSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_VSGetShader(This,ppVertexShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> VSGetShader(This,ppVertexShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext3_PSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> PSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_IAGetInputLayout(This,ppInputLayout)	\
    ( (This)->lpVtbl -> IAGetInputLayout(This,ppInputLayout) ) 

#define ID3D11DeviceContext3_IAGetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets)	\
    ( (This)->lpVtbl -> IAGetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets) ) 

#define ID3D11DeviceContext3_IAGetIndexBuffer(This,pIndexBuffer,Format,Offset)	\
    ( (This)->lpVtbl -> IAGetIndexBuffer(This,pIndexBuffer,Format,Offset) ) 

#define ID3D11DeviceContext3_GSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> GSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_GSGetShader(This,ppGeometryShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> GSGetShader(This,ppGeometryShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext3_IAGetPrimitiveTopology(This,pTopology)	\
    ( (This)->lpVtbl -> IAGetPrimitiveTopology(This,pTopology) ) 

#define ID3D11DeviceContext3_VSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> VSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_VSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> VSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_GetPredication(This,ppPredicate,pPredicateValue)	\
    ( (This)->lpVtbl -> GetPredication(This,ppPredicate,pPredicateValue) ) 

#define ID3D11DeviceContext3_GSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> GSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_GSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> GSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_OMGetRenderTargets(This,NumViews,ppRenderTargetViews,ppDepthStencilView)	\
    ( (This)->lpVtbl -> OMGetRenderTargets(This,NumViews,ppRenderTargetViews,ppDepthStencilView) ) 

#define ID3D11DeviceContext3_OMGetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,ppDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews)	\
    ( (This)->lpVtbl -> OMGetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,ppDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews) ) 

#define ID3D11DeviceContext3_OMGetBlendState(This,ppBlendState,BlendFactor,pSampleMask)	\
    ( (This)->lpVtbl -> OMGetBlendState(This,ppBlendState,BlendFactor,pSampleMask) ) 

#define ID3D11DeviceContext3_OMGetDepthStencilState(This,ppDepthStencilState,pStencilRef)	\
    ( (This)->lpVtbl -> OMGetDepthStencilState(This,ppDepthStencilState,pStencilRef) ) 

#define ID3D11DeviceContext3_SOGetTargets(This,NumBuffers,ppSOTargets)	\
    ( (This)->lpVtbl -> SOGetTargets(This,NumBuffers,ppSOTargets) ) 

#define ID3D11DeviceContext3_RSGetState(This,ppRasterizerState)	\
    ( (This)->lpVtbl -> RSGetState(This,ppRasterizerState) ) 

#define ID3D11DeviceContext3_RSGetViewports(This,pNumViewports,pViewports)	\
    ( (This)->lpVtbl -> RSGetViewports(This,pNumViewports,pViewports) ) 

#define ID3D11DeviceContext3_RSGetScissorRects(This,pNumRects,pRects)	\
    ( (This)->lpVtbl -> RSGetScissorRects(This,pNumRects,pRects) ) 

#define ID3D11DeviceContext3_HSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> HSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_HSGetShader(This,ppHullShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> HSGetShader(This,ppHullShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext3_HSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> HSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_HSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> HSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_DSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> DSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_DSGetShader(This,ppDomainShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> DSGetShader(This,ppDomainShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext3_DSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> DSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_DSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> DSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_CSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> CSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext3_CSGetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews)	\
    ( (This)->lpVtbl -> CSGetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews) ) 

#define ID3D11DeviceContext3_CSGetShader(This,ppComputeShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> CSGetShader(This,ppComputeShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext3_CSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> CSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext3_CSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> CSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext3_ClearState(This)	\
    ( (This)->lpVtbl -> ClearState(This) ) 

#define ID3D11DeviceContext3_Flush(This)	\
    ( (This)->lpVtbl -> Flush(This) ) 

#define ID3D11DeviceContext3_GetType(This)	\
    ( (This)->lpVtbl -> GetType(This) ) 

#define ID3D11DeviceContext3_GetContextFlags(This)	\
    ( (This)->lpVtbl -> GetContextFlags(This) ) 

#define ID3D11DeviceContext3_FinishCommandList(This,RestoreDeferredContextState,ppCommandList)	\
    ( (This)->lpVtbl -> FinishCommandList(This,RestoreDeferredContextState,ppCommandList) ) 


#define ID3D11DeviceContext3_CopySubresourceRegion1(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox,CopyFlags)	\
    ( (This)->lpVtbl -> CopySubresourceRegion1(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox,CopyFlags) ) 

#define ID3D11DeviceContext3_UpdateSubresource1(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch,CopyFlags)	\
    ( (This)->lpVtbl -> UpdateSubresource1(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch,CopyFlags) ) 

#define ID3D11DeviceContext3_DiscardResource(This,pResource)	\
    ( (This)->lpVtbl -> DiscardResource(This,pResource) ) 

#define ID3D11DeviceContext3_DiscardView(This,pResourceView)	\
    ( (This)->lpVtbl -> DiscardView(This,pResourceView) ) 

#define ID3D11DeviceContext3_VSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> VSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_HSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> HSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_DSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> DSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_GSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> GSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_PSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> PSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_CSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> CSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_VSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> VSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_HSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> HSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_DSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> DSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_GSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> GSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_PSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> PSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_CSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> CSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext3_SwapDeviceContextState(This,pState,ppPreviousState)	\
    ( (This)->lpVtbl -> SwapDeviceContextState(This,pState,ppPreviousState) ) 

#define ID3D11DeviceContext3_ClearView(This,pView,Color,pRect,NumRects)	\
    ( (This)->lpVtbl -> ClearView(This,pView,Color,pRect,NumRects) ) 

#define ID3D11DeviceContext3_DiscardView1(This,pResourceView,pRects,NumRects)	\
    ( (This)->lpVtbl -> DiscardView1(This,pResourceView,pRects,NumRects) ) 


#define ID3D11DeviceContext3_UpdateTileMappings(This,pTiledResource,NumTiledResourceRegions,pTiledResourceRegionStartCoordinates,pTiledResourceRegionSizes,pTilePool,NumRanges,pRangeFlags,pTilePoolStartOffsets,pRangeTileCounts,Flags)	\
    ( (This)->lpVtbl -> UpdateTileMappings(This,pTiledResource,NumTiledResourceRegions,pTiledResourceRegionStartCoordinates,pTiledResourceRegionSizes,pTilePool,NumRanges,pRangeFlags,pTilePoolStartOffsets,pRangeTileCounts,Flags) ) 

#define ID3D11DeviceContext3_CopyTileMappings(This,pDestTiledResource,pDestRegionStartCoordinate,pSourceTiledResource,pSourceRegionStartCoordinate,pTileRegionSize,Flags)	\
    ( (This)->lpVtbl -> CopyTileMappings(This,pDestTiledResource,pDestRegionStartCoordinate,pSourceTiledResource,pSourceRegionStartCoordinate,pTileRegionSize,Flags) ) 

#define ID3D11DeviceContext3_CopyTiles(This,pTiledResource,pTileRegionStartCoordinate,pTileRegionSize,pBuffer,BufferStartOffsetInBytes,Flags)	\
    ( (This)->lpVtbl -> CopyTiles(This,pTiledResource,pTileRegionStartCoordinate,pTileRegionSize,pBuffer,BufferStartOffsetInBytes,Flags) ) 

#define ID3D11DeviceContext3_UpdateTiles(This,pDestTiledResource,pDestTileRegionStartCoordinate,pDestTileRegionSize,pSourceTileData,Flags)	\
    ( (This)->lpVtbl -> UpdateTiles(This,pDestTiledResource,pDestTileRegionStartCoordinate,pDestTileRegionSize,pSourceTileData,Flags) ) 

#define ID3D11DeviceContext3_ResizeTilePool(This,pTilePool,NewSizeInBytes)	\
    ( (This)->lpVtbl -> ResizeTilePool(This,pTilePool,NewSizeInBytes) ) 

#define ID3D11DeviceContext3_TiledResourceBarrier(This,pTiledResourceOrViewAccessBeforeBarrier,pTiledResourceOrViewAccessAfterBarrier)	\
    ( (This)->lpVtbl -> TiledResourceBarrier(This,pTiledResourceOrViewAccessBeforeBarrier,pTiledResourceOrViewAccessAfterBarrier) ) 

#define ID3D11DeviceContext3_IsAnnotationEnabled(This)	\
    ( (This)->lpVtbl -> IsAnnotationEnabled(This) ) 

#define ID3D11DeviceContext3_SetMarkerInt(This,pLabel,Data)	\
    ( (This)->lpVtbl -> SetMarkerInt(This,pLabel,Data) ) 

#define ID3D11DeviceContext3_BeginEventInt(This,pLabel,Data)	\
    ( (This)->lpVtbl -> BeginEventInt(This,pLabel,Data) ) 

#define ID3D11DeviceContext3_EndEvent(This)	\
    ( (This)->lpVtbl -> EndEvent(This) ) 


#define ID3D11DeviceContext3_Flush1(This,ContextType,hEvent)	\
    ( (This)->lpVtbl -> Flush1(This,ContextType,hEvent) ) 

#define ID3D11DeviceContext3_SetHardwareProtectionState(This,HwProtectionEnable)	\
    ( (This)->lpVtbl -> SetHardwareProtectionState(This,HwProtectionEnable) ) 

#define ID3D11DeviceContext3_GetHardwareProtectionState(This,pHwProtectionEnable)	\
    ( (This)->lpVtbl -> GetHardwareProtectionState(This,pHwProtectionEnable) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11DeviceContext3_INTERFACE_DEFINED__ */


#ifndef __ID3D11Fence_INTERFACE_DEFINED__
#define __ID3D11Fence_INTERFACE_DEFINED__

  /* interface ID3D11Fence */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D11Fence;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("affde9d1-1df7-4bb7-8a34-0f46251dab80")
    ID3D11Fence : public ID3D11DeviceChild
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE CreateSharedHandle( 
      /* [annotation] */ 
      _In_opt_  const SECURITY_ATTRIBUTES *pAttributes,
      /* [annotation] */ 
      _In_  DWORD dwAccess,
      /* [annotation] */ 
      _In_opt_  LPCWSTR lpName,
      /* [annotation] */ 
      _Out_  HANDLE *pHandle) = 0;

    virtual UINT64 STDMETHODCALLTYPE GetCompletedValue( void) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetEventOnCompletion( 
      /* [annotation] */ 
      _In_  UINT64 Value,
      /* [annotation] */ 
      _In_  HANDLE hEvent) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D11FenceVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D11Fence * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D11Fence * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D11Fence * This);

    void ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D11Fence * This,
      /* [annotation] */ 
      _Outptr_  ID3D11Device **ppDevice);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D11Fence * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation] */ 
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D11Fence * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_  UINT DataSize,
      /* [annotation] */ 
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D11Fence * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *CreateSharedHandle )( 
      ID3D11Fence * This,
      /* [annotation] */ 
      _In_opt_  const SECURITY_ATTRIBUTES *pAttributes,
      /* [annotation] */ 
      _In_  DWORD dwAccess,
      /* [annotation] */ 
      _In_opt_  LPCWSTR lpName,
      /* [annotation] */ 
      _Out_  HANDLE *pHandle);

    UINT64 ( STDMETHODCALLTYPE *GetCompletedValue )( 
      ID3D11Fence * This);

    HRESULT ( STDMETHODCALLTYPE *SetEventOnCompletion )( 
      ID3D11Fence * This,
      /* [annotation] */ 
      _In_  UINT64 Value,
      /* [annotation] */ 
      _In_  HANDLE hEvent);

    END_INTERFACE
  } ID3D11FenceVtbl;

  interface ID3D11Fence
  {
    CONST_VTBL struct ID3D11FenceVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D11Fence_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11Fence_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11Fence_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11Fence_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11Fence_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11Fence_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11Fence_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11Fence_CreateSharedHandle(This,pAttributes,dwAccess,lpName,pHandle)	\
    ( (This)->lpVtbl -> CreateSharedHandle(This,pAttributes,dwAccess,lpName,pHandle) ) 

#define ID3D11Fence_GetCompletedValue(This)	\
    ( (This)->lpVtbl -> GetCompletedValue(This) ) 

#define ID3D11Fence_SetEventOnCompletion(This,Value,hEvent)	\
    ( (This)->lpVtbl -> SetEventOnCompletion(This,Value,hEvent) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11Fence_INTERFACE_DEFINED__ */


#ifndef __ID3D11DeviceContext4_INTERFACE_DEFINED__
#define __ID3D11DeviceContext4_INTERFACE_DEFINED__

  /* interface ID3D11DeviceContext4 */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D11DeviceContext4;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("917600da-f58c-4c33-98d8-3e15b390fa24")
    ID3D11DeviceContext4 : public ID3D11DeviceContext3
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE Signal( 
      /* [annotation] */ 
      _In_  ID3D11Fence *pFence,
      /* [annotation] */ 
      _In_  UINT64 Value) = 0;

    virtual HRESULT STDMETHODCALLTYPE Wait( 
      /* [annotation] */ 
      _In_  ID3D11Fence *pFence,
      /* [annotation] */ 
      _In_  UINT64 Value) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D11DeviceContext4Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D11DeviceContext4 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D11DeviceContext4 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D11DeviceContext4 * This);

    void ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _Outptr_  ID3D11Device **ppDevice);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation] */ 
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_  UINT DataSize,
      /* [annotation] */ 
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_opt_  const IUnknown *pData);

    void ( STDMETHODCALLTYPE *VSSetConstantBuffers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);

    void ( STDMETHODCALLTYPE *PSSetShaderResources )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *PSSetShader )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11PixelShader *pPixelShader,
      /* [annotation] */ 
      _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
      UINT NumClassInstances);

    void ( STDMETHODCALLTYPE *PSSetSamplers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);

    void ( STDMETHODCALLTYPE *VSSetShader )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11VertexShader *pVertexShader,
      /* [annotation] */ 
      _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
      UINT NumClassInstances);

    void ( STDMETHODCALLTYPE *DrawIndexed )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  UINT IndexCount,
      /* [annotation] */ 
      _In_  UINT StartIndexLocation,
      /* [annotation] */ 
      _In_  INT BaseVertexLocation);

    void ( STDMETHODCALLTYPE *Draw )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  UINT VertexCount,
      /* [annotation] */ 
      _In_  UINT StartVertexLocation);

    HRESULT ( STDMETHODCALLTYPE *Map )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pResource,
      /* [annotation] */ 
      _In_  UINT Subresource,
      /* [annotation] */ 
      _In_  D3D11_MAP MapType,
      /* [annotation] */ 
      _In_  UINT MapFlags,
      /* [annotation] */ 
      _Out_opt_  D3D11_MAPPED_SUBRESOURCE *pMappedResource);

    void ( STDMETHODCALLTYPE *Unmap )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pResource,
      /* [annotation] */ 
      _In_  UINT Subresource);

    void ( STDMETHODCALLTYPE *PSSetConstantBuffers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);

    void ( STDMETHODCALLTYPE *IASetInputLayout )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11InputLayout *pInputLayout);

    void ( STDMETHODCALLTYPE *IASetVertexBuffers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppVertexBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pStrides,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pOffsets);

    void ( STDMETHODCALLTYPE *IASetIndexBuffer )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11Buffer *pIndexBuffer,
      /* [annotation] */ 
      _In_  DXGI_FORMAT Format,
      /* [annotation] */ 
      _In_  UINT Offset);

    void ( STDMETHODCALLTYPE *DrawIndexedInstanced )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  UINT IndexCountPerInstance,
      /* [annotation] */ 
      _In_  UINT InstanceCount,
      /* [annotation] */ 
      _In_  UINT StartIndexLocation,
      /* [annotation] */ 
      _In_  INT BaseVertexLocation,
      /* [annotation] */ 
      _In_  UINT StartInstanceLocation);

    void ( STDMETHODCALLTYPE *DrawInstanced )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  UINT VertexCountPerInstance,
      /* [annotation] */ 
      _In_  UINT InstanceCount,
      /* [annotation] */ 
      _In_  UINT StartVertexLocation,
      /* [annotation] */ 
      _In_  UINT StartInstanceLocation);

    void ( STDMETHODCALLTYPE *GSSetConstantBuffers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);

    void ( STDMETHODCALLTYPE *GSSetShader )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11GeometryShader *pShader,
      /* [annotation] */ 
      _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
      UINT NumClassInstances);

    void ( STDMETHODCALLTYPE *IASetPrimitiveTopology )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  D3D11_PRIMITIVE_TOPOLOGY Topology);

    void ( STDMETHODCALLTYPE *VSSetShaderResources )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *VSSetSamplers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);

    void ( STDMETHODCALLTYPE *Begin )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Asynchronous *pAsync);

    void ( STDMETHODCALLTYPE *End )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Asynchronous *pAsync);

    HRESULT ( STDMETHODCALLTYPE *GetData )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Asynchronous *pAsync,
      /* [annotation] */ 
      _Out_writes_bytes_opt_( DataSize )  void *pData,
      /* [annotation] */ 
      _In_  UINT DataSize,
      /* [annotation] */ 
      _In_  UINT GetDataFlags);

    void ( STDMETHODCALLTYPE *SetPredication )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11Predicate *pPredicate,
      /* [annotation] */ 
      _In_  BOOL PredicateValue);

    void ( STDMETHODCALLTYPE *GSSetShaderResources )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *GSSetSamplers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);

    void ( STDMETHODCALLTYPE *OMSetRenderTargets )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT )  UINT NumViews,
      /* [annotation] */ 
      _In_reads_opt_(NumViews)  ID3D11RenderTargetView *const *ppRenderTargetViews,
      /* [annotation] */ 
      _In_opt_  ID3D11DepthStencilView *pDepthStencilView);

    void ( STDMETHODCALLTYPE *OMSetRenderTargetsAndUnorderedAccessViews )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  UINT NumRTVs,
      /* [annotation] */ 
      _In_reads_opt_(NumRTVs)  ID3D11RenderTargetView *const *ppRenderTargetViews,
      /* [annotation] */ 
      _In_opt_  ID3D11DepthStencilView *pDepthStencilView,
      /* [annotation] */ 
      _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - 1 )  UINT UAVStartSlot,
      /* [annotation] */ 
      _In_  UINT NumUAVs,
      /* [annotation] */ 
      _In_reads_opt_(NumUAVs)  ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
      /* [annotation] */ 
      _In_reads_opt_(NumUAVs)  const UINT *pUAVInitialCounts);

    void ( STDMETHODCALLTYPE *OMSetBlendState )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11BlendState *pBlendState,
      /* [annotation] */ 
      _In_opt_  const FLOAT BlendFactor[ 4 ],
      /* [annotation] */ 
      _In_  UINT SampleMask);

    void ( STDMETHODCALLTYPE *OMSetDepthStencilState )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11DepthStencilState *pDepthStencilState,
      /* [annotation] */ 
      _In_  UINT StencilRef);

    void ( STDMETHODCALLTYPE *SOSetTargets )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_SO_BUFFER_SLOT_COUNT)  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppSOTargets,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pOffsets);

    void ( STDMETHODCALLTYPE *DrawAuto )( 
      ID3D11DeviceContext4 * This);

    void ( STDMETHODCALLTYPE *DrawIndexedInstancedIndirect )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Buffer *pBufferForArgs,
      /* [annotation] */ 
      _In_  UINT AlignedByteOffsetForArgs);

    void ( STDMETHODCALLTYPE *DrawInstancedIndirect )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Buffer *pBufferForArgs,
      /* [annotation] */ 
      _In_  UINT AlignedByteOffsetForArgs);

    void ( STDMETHODCALLTYPE *Dispatch )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  UINT ThreadGroupCountX,
      /* [annotation] */ 
      _In_  UINT ThreadGroupCountY,
      /* [annotation] */ 
      _In_  UINT ThreadGroupCountZ);

    void ( STDMETHODCALLTYPE *DispatchIndirect )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Buffer *pBufferForArgs,
      /* [annotation] */ 
      _In_  UINT AlignedByteOffsetForArgs);

    void ( STDMETHODCALLTYPE *RSSetState )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11RasterizerState *pRasterizerState);

    void ( STDMETHODCALLTYPE *RSSetViewports )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumViewports,
      /* [annotation] */ 
      _In_reads_opt_(NumViewports)  const D3D11_VIEWPORT *pViewports);

    void ( STDMETHODCALLTYPE *RSSetScissorRects )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumRects,
      /* [annotation] */ 
      _In_reads_opt_(NumRects)  const D3D11_RECT *pRects);

    void ( STDMETHODCALLTYPE *CopySubresourceRegion )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pDstResource,
      /* [annotation] */ 
      _In_  UINT DstSubresource,
      /* [annotation] */ 
      _In_  UINT DstX,
      /* [annotation] */ 
      _In_  UINT DstY,
      /* [annotation] */ 
      _In_  UINT DstZ,
      /* [annotation] */ 
      _In_  ID3D11Resource *pSrcResource,
      /* [annotation] */ 
      _In_  UINT SrcSubresource,
      /* [annotation] */ 
      _In_opt_  const D3D11_BOX *pSrcBox);

    void ( STDMETHODCALLTYPE *CopyResource )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pDstResource,
      /* [annotation] */ 
      _In_  ID3D11Resource *pSrcResource);

    void ( STDMETHODCALLTYPE *UpdateSubresource )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pDstResource,
      /* [annotation] */ 
      _In_  UINT DstSubresource,
      /* [annotation] */ 
      _In_opt_  const D3D11_BOX *pDstBox,
      /* [annotation] */ 
      _In_  const void *pSrcData,
      /* [annotation] */ 
      _In_  UINT SrcRowPitch,
      /* [annotation] */ 
      _In_  UINT SrcDepthPitch);

    void ( STDMETHODCALLTYPE *CopyStructureCount )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Buffer *pDstBuffer,
      /* [annotation] */ 
      _In_  UINT DstAlignedByteOffset,
      /* [annotation] */ 
      _In_  ID3D11UnorderedAccessView *pSrcView);

    void ( STDMETHODCALLTYPE *ClearRenderTargetView )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11RenderTargetView *pRenderTargetView,
      /* [annotation] */ 
      _In_  const FLOAT ColorRGBA[ 4 ]);

    void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewUint )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11UnorderedAccessView *pUnorderedAccessView,
      /* [annotation] */ 
      _In_  const UINT Values[ 4 ]);

    void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewFloat )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11UnorderedAccessView *pUnorderedAccessView,
      /* [annotation] */ 
      _In_  const FLOAT Values[ 4 ]);

    void ( STDMETHODCALLTYPE *ClearDepthStencilView )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11DepthStencilView *pDepthStencilView,
      /* [annotation] */ 
      _In_  UINT ClearFlags,
      /* [annotation] */ 
      _In_  FLOAT Depth,
      /* [annotation] */ 
      _In_  UINT8 Stencil);

    void ( STDMETHODCALLTYPE *GenerateMips )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11ShaderResourceView *pShaderResourceView);

    void ( STDMETHODCALLTYPE *SetResourceMinLOD )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pResource,
      FLOAT MinLOD);

    FLOAT ( STDMETHODCALLTYPE *GetResourceMinLOD )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pResource);

    void ( STDMETHODCALLTYPE *ResolveSubresource )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pDstResource,
      /* [annotation] */ 
      _In_  UINT DstSubresource,
      /* [annotation] */ 
      _In_  ID3D11Resource *pSrcResource,
      /* [annotation] */ 
      _In_  UINT SrcSubresource,
      /* [annotation] */ 
      _In_  DXGI_FORMAT Format);

    void ( STDMETHODCALLTYPE *ExecuteCommandList )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11CommandList *pCommandList,
      BOOL RestoreContextState);

    void ( STDMETHODCALLTYPE *HSSetShaderResources )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *HSSetShader )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11HullShader *pHullShader,
      /* [annotation] */ 
      _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
      UINT NumClassInstances);

    void ( STDMETHODCALLTYPE *HSSetSamplers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);

    void ( STDMETHODCALLTYPE *HSSetConstantBuffers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);

    void ( STDMETHODCALLTYPE *DSSetShaderResources )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *DSSetShader )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11DomainShader *pDomainShader,
      /* [annotation] */ 
      _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
      UINT NumClassInstances);

    void ( STDMETHODCALLTYPE *DSSetSamplers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);

    void ( STDMETHODCALLTYPE *DSSetConstantBuffers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);

    void ( STDMETHODCALLTYPE *CSSetShaderResources )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *CSSetUnorderedAccessViews )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - StartSlot )  UINT NumUAVs,
      /* [annotation] */ 
      _In_reads_opt_(NumUAVs)  ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
      /* [annotation] */ 
      _In_reads_opt_(NumUAVs)  const UINT *pUAVInitialCounts);

    void ( STDMETHODCALLTYPE *CSSetShader )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11ComputeShader *pComputeShader,
      /* [annotation] */ 
      _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
      UINT NumClassInstances);

    void ( STDMETHODCALLTYPE *CSSetSamplers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);

    void ( STDMETHODCALLTYPE *CSSetConstantBuffers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);

    void ( STDMETHODCALLTYPE *VSGetConstantBuffers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);

    void ( STDMETHODCALLTYPE *PSGetShaderResources )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *PSGetShader )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _Outptr_result_maybenull_  ID3D11PixelShader **ppPixelShader,
      /* [annotation] */ 
      _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
      /* [annotation] */ 
      _Inout_opt_  UINT *pNumClassInstances);

    void ( STDMETHODCALLTYPE *PSGetSamplers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);

    void ( STDMETHODCALLTYPE *VSGetShader )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _Outptr_result_maybenull_  ID3D11VertexShader **ppVertexShader,
      /* [annotation] */ 
      _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
      /* [annotation] */ 
      _Inout_opt_  UINT *pNumClassInstances);

    void ( STDMETHODCALLTYPE *PSGetConstantBuffers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);

    void ( STDMETHODCALLTYPE *IAGetInputLayout )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _Outptr_result_maybenull_  ID3D11InputLayout **ppInputLayout);

    void ( STDMETHODCALLTYPE *IAGetVertexBuffers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppVertexBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pStrides,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pOffsets);

    void ( STDMETHODCALLTYPE *IAGetIndexBuffer )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _Outptr_opt_result_maybenull_  ID3D11Buffer **pIndexBuffer,
      /* [annotation] */ 
      _Out_opt_  DXGI_FORMAT *Format,
      /* [annotation] */ 
      _Out_opt_  UINT *Offset);

    void ( STDMETHODCALLTYPE *GSGetConstantBuffers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);

    void ( STDMETHODCALLTYPE *GSGetShader )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _Outptr_result_maybenull_  ID3D11GeometryShader **ppGeometryShader,
      /* [annotation] */ 
      _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
      /* [annotation] */ 
      _Inout_opt_  UINT *pNumClassInstances);

    void ( STDMETHODCALLTYPE *IAGetPrimitiveTopology )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _Out_  D3D11_PRIMITIVE_TOPOLOGY *pTopology);

    void ( STDMETHODCALLTYPE *VSGetShaderResources )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *VSGetSamplers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);

    void ( STDMETHODCALLTYPE *GetPredication )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _Outptr_opt_result_maybenull_  ID3D11Predicate **ppPredicate,
      /* [annotation] */ 
      _Out_opt_  BOOL *pPredicateValue);

    void ( STDMETHODCALLTYPE *GSGetShaderResources )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *GSGetSamplers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);

    void ( STDMETHODCALLTYPE *OMGetRenderTargets )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT )  UINT NumViews,
      /* [annotation] */ 
      _Out_writes_opt_(NumViews)  ID3D11RenderTargetView **ppRenderTargetViews,
      /* [annotation] */ 
      _Outptr_opt_result_maybenull_  ID3D11DepthStencilView **ppDepthStencilView);

    void ( STDMETHODCALLTYPE *OMGetRenderTargetsAndUnorderedAccessViews )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT )  UINT NumRTVs,
      /* [annotation] */ 
      _Out_writes_opt_(NumRTVs)  ID3D11RenderTargetView **ppRenderTargetViews,
      /* [annotation] */ 
      _Outptr_opt_result_maybenull_  ID3D11DepthStencilView **ppDepthStencilView,
      /* [annotation] */ 
      _In_range_( 0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1 )  UINT UAVStartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_PS_CS_UAV_REGISTER_COUNT - UAVStartSlot )  UINT NumUAVs,
      /* [annotation] */ 
      _Out_writes_opt_(NumUAVs)  ID3D11UnorderedAccessView **ppUnorderedAccessViews);

    void ( STDMETHODCALLTYPE *OMGetBlendState )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _Outptr_opt_result_maybenull_  ID3D11BlendState **ppBlendState,
      /* [annotation] */ 
      _Out_opt_  FLOAT BlendFactor[ 4 ],
      /* [annotation] */ 
      _Out_opt_  UINT *pSampleMask);

    void ( STDMETHODCALLTYPE *OMGetDepthStencilState )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _Outptr_opt_result_maybenull_  ID3D11DepthStencilState **ppDepthStencilState,
      /* [annotation] */ 
      _Out_opt_  UINT *pStencilRef);

    void ( STDMETHODCALLTYPE *SOGetTargets )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_SO_BUFFER_SLOT_COUNT )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppSOTargets);

    void ( STDMETHODCALLTYPE *RSGetState )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _Outptr_result_maybenull_  ID3D11RasterizerState **ppRasterizerState);

    void ( STDMETHODCALLTYPE *RSGetViewports )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumViewports,
      /* [annotation] */ 
      _Out_writes_opt_(*pNumViewports)  D3D11_VIEWPORT *pViewports);

    void ( STDMETHODCALLTYPE *RSGetScissorRects )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumRects,
      /* [annotation] */ 
      _Out_writes_opt_(*pNumRects)  D3D11_RECT *pRects);

    void ( STDMETHODCALLTYPE *HSGetShaderResources )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *HSGetShader )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _Outptr_result_maybenull_  ID3D11HullShader **ppHullShader,
      /* [annotation] */ 
      _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
      /* [annotation] */ 
      _Inout_opt_  UINT *pNumClassInstances);

    void ( STDMETHODCALLTYPE *HSGetSamplers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);

    void ( STDMETHODCALLTYPE *HSGetConstantBuffers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);

    void ( STDMETHODCALLTYPE *DSGetShaderResources )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *DSGetShader )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _Outptr_result_maybenull_  ID3D11DomainShader **ppDomainShader,
      /* [annotation] */ 
      _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
      /* [annotation] */ 
      _Inout_opt_  UINT *pNumClassInstances);

    void ( STDMETHODCALLTYPE *DSGetSamplers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);

    void ( STDMETHODCALLTYPE *DSGetConstantBuffers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);

    void ( STDMETHODCALLTYPE *CSGetShaderResources )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      /* [annotation] */ 
      _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);

    void ( STDMETHODCALLTYPE *CSGetUnorderedAccessViews )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - StartSlot )  UINT NumUAVs,
      /* [annotation] */ 
      _Out_writes_opt_(NumUAVs)  ID3D11UnorderedAccessView **ppUnorderedAccessViews);

    void ( STDMETHODCALLTYPE *CSGetShader )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _Outptr_result_maybenull_  ID3D11ComputeShader **ppComputeShader,
      /* [annotation] */ 
      _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
      /* [annotation] */ 
      _Inout_opt_  UINT *pNumClassInstances);

    void ( STDMETHODCALLTYPE *CSGetSamplers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
      /* [annotation] */ 
      _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);

    void ( STDMETHODCALLTYPE *CSGetConstantBuffers )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);

    void ( STDMETHODCALLTYPE *ClearState )( 
      ID3D11DeviceContext4 * This);

    void ( STDMETHODCALLTYPE *Flush )( 
      ID3D11DeviceContext4 * This);

    D3D11_DEVICE_CONTEXT_TYPE ( STDMETHODCALLTYPE *GetType )( 
      ID3D11DeviceContext4 * This);

    UINT ( STDMETHODCALLTYPE *GetContextFlags )( 
      ID3D11DeviceContext4 * This);

    HRESULT ( STDMETHODCALLTYPE *FinishCommandList )( 
      ID3D11DeviceContext4 * This,
      BOOL RestoreDeferredContextState,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11CommandList **ppCommandList);

    void ( STDMETHODCALLTYPE *CopySubresourceRegion1 )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pDstResource,
      /* [annotation] */ 
      _In_  UINT DstSubresource,
      /* [annotation] */ 
      _In_  UINT DstX,
      /* [annotation] */ 
      _In_  UINT DstY,
      /* [annotation] */ 
      _In_  UINT DstZ,
      /* [annotation] */ 
      _In_  ID3D11Resource *pSrcResource,
      /* [annotation] */ 
      _In_  UINT SrcSubresource,
      /* [annotation] */ 
      _In_opt_  const D3D11_BOX *pSrcBox,
      /* [annotation] */ 
      _In_  UINT CopyFlags);

    void ( STDMETHODCALLTYPE *UpdateSubresource1 )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pDstResource,
      /* [annotation] */ 
      _In_  UINT DstSubresource,
      /* [annotation] */ 
      _In_opt_  const D3D11_BOX *pDstBox,
      /* [annotation] */ 
      _In_  const void *pSrcData,
      /* [annotation] */ 
      _In_  UINT SrcRowPitch,
      /* [annotation] */ 
      _In_  UINT SrcDepthPitch,
      /* [annotation] */ 
      _In_  UINT CopyFlags);

    void ( STDMETHODCALLTYPE *DiscardResource )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pResource);

    void ( STDMETHODCALLTYPE *DiscardView )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11View *pResourceView);

    void ( STDMETHODCALLTYPE *VSSetConstantBuffers1 )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *HSSetConstantBuffers1 )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *DSSetConstantBuffers1 )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *GSSetConstantBuffers1 )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *PSSetConstantBuffers1 )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *CSSetConstantBuffers1 )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pFirstConstant,
      /* [annotation] */ 
      _In_reads_opt_(NumBuffers)  const UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *VSGetConstantBuffers1 )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *HSGetConstantBuffers1 )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *DSGetConstantBuffers1 )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *GSGetConstantBuffers1 )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *PSGetConstantBuffers1 )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *CSGetConstantBuffers1 )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      /* [annotation] */ 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
      /* [annotation] */ 
      _Out_writes_opt_(NumBuffers)  UINT *pNumConstants);

    void ( STDMETHODCALLTYPE *SwapDeviceContextState )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3DDeviceContextState *pState,
      /* [annotation] */ 
      _Outptr_opt_  ID3DDeviceContextState **ppPreviousState);

    void ( STDMETHODCALLTYPE *ClearView )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11View *pView,
      /* [annotation] */ 
      _In_  const FLOAT Color[ 4 ],
      /* [annotation] */ 
      _In_reads_opt_(NumRects)  const D3D11_RECT *pRect,
      UINT NumRects);

    void ( STDMETHODCALLTYPE *DiscardView1 )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11View *pResourceView,
      /* [annotation] */ 
      _In_reads_opt_(NumRects)  const D3D11_RECT *pRects,
      UINT NumRects);

    HRESULT ( STDMETHODCALLTYPE *UpdateTileMappings )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pTiledResource,
      /* [annotation] */ 
      _In_  UINT NumTiledResourceRegions,
      /* [annotation] */ 
      _In_reads_opt_(NumTiledResourceRegions)  const D3D11_TILED_RESOURCE_COORDINATE *pTiledResourceRegionStartCoordinates,
      /* [annotation] */ 
      _In_reads_opt_(NumTiledResourceRegions)  const D3D11_TILE_REGION_SIZE *pTiledResourceRegionSizes,
      /* [annotation] */ 
      _In_opt_  ID3D11Buffer *pTilePool,
      /* [annotation] */ 
      _In_  UINT NumRanges,
      /* [annotation] */ 
      _In_reads_opt_(NumRanges)  const UINT *pRangeFlags,
      /* [annotation] */ 
      _In_reads_opt_(NumRanges)  const UINT *pTilePoolStartOffsets,
      /* [annotation] */ 
      _In_reads_opt_(NumRanges)  const UINT *pRangeTileCounts,
      /* [annotation] */ 
      _In_  UINT Flags);

    HRESULT ( STDMETHODCALLTYPE *CopyTileMappings )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pDestTiledResource,
      /* [annotation] */ 
      _In_  const D3D11_TILED_RESOURCE_COORDINATE *pDestRegionStartCoordinate,
      /* [annotation] */ 
      _In_  ID3D11Resource *pSourceTiledResource,
      /* [annotation] */ 
      _In_  const D3D11_TILED_RESOURCE_COORDINATE *pSourceRegionStartCoordinate,
      /* [annotation] */ 
      _In_  const D3D11_TILE_REGION_SIZE *pTileRegionSize,
      /* [annotation] */ 
      _In_  UINT Flags);

    void ( STDMETHODCALLTYPE *CopyTiles )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pTiledResource,
      /* [annotation] */ 
      _In_  const D3D11_TILED_RESOURCE_COORDINATE *pTileRegionStartCoordinate,
      /* [annotation] */ 
      _In_  const D3D11_TILE_REGION_SIZE *pTileRegionSize,
      /* [annotation] */ 
      _In_  ID3D11Buffer *pBuffer,
      /* [annotation] */ 
      _In_  UINT64 BufferStartOffsetInBytes,
      /* [annotation] */ 
      _In_  UINT Flags);

    void ( STDMETHODCALLTYPE *UpdateTiles )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pDestTiledResource,
      /* [annotation] */ 
      _In_  const D3D11_TILED_RESOURCE_COORDINATE *pDestTileRegionStartCoordinate,
      /* [annotation] */ 
      _In_  const D3D11_TILE_REGION_SIZE *pDestTileRegionSize,
      /* [annotation] */ 
      _In_  const void *pSourceTileData,
      /* [annotation] */ 
      _In_  UINT Flags);

    HRESULT ( STDMETHODCALLTYPE *ResizeTilePool )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Buffer *pTilePool,
      /* [annotation] */ 
      _In_  UINT64 NewSizeInBytes);

    void ( STDMETHODCALLTYPE *TiledResourceBarrier )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_opt_  ID3D11DeviceChild *pTiledResourceOrViewAccessBeforeBarrier,
      /* [annotation] */ 
      _In_opt_  ID3D11DeviceChild *pTiledResourceOrViewAccessAfterBarrier);

    BOOL ( STDMETHODCALLTYPE *IsAnnotationEnabled )( 
      ID3D11DeviceContext4 * This);

    void ( STDMETHODCALLTYPE *SetMarkerInt )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  LPCWSTR pLabel,
      INT Data);

    void ( STDMETHODCALLTYPE *BeginEventInt )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  LPCWSTR pLabel,
      INT Data);

    void ( STDMETHODCALLTYPE *EndEvent )( 
      ID3D11DeviceContext4 * This);

    void ( STDMETHODCALLTYPE *Flush1 )( 
      ID3D11DeviceContext4 * This,
      D3D11_CONTEXT_TYPE ContextType,
      /* [annotation] */ 
      _In_opt_  HANDLE hEvent);

    void ( STDMETHODCALLTYPE *SetHardwareProtectionState )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  BOOL HwProtectionEnable);

    void ( STDMETHODCALLTYPE *GetHardwareProtectionState )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _Out_  BOOL *pHwProtectionEnable);

    HRESULT ( STDMETHODCALLTYPE *Signal )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Fence *pFence,
      /* [annotation] */ 
      _In_  UINT64 Value);

    HRESULT ( STDMETHODCALLTYPE *Wait )( 
      ID3D11DeviceContext4 * This,
      /* [annotation] */ 
      _In_  ID3D11Fence *pFence,
      /* [annotation] */ 
      _In_  UINT64 Value);

    END_INTERFACE
  } ID3D11DeviceContext4Vtbl;

  interface ID3D11DeviceContext4
  {
    CONST_VTBL struct ID3D11DeviceContext4Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D11DeviceContext4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11DeviceContext4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11DeviceContext4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11DeviceContext4_GetDevice(This,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,ppDevice) ) 

#define ID3D11DeviceContext4_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11DeviceContext4_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11DeviceContext4_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 


#define ID3D11DeviceContext4_VSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> VSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_PSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> PSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_PSSetShader(This,pPixelShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> PSSetShader(This,pPixelShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext4_PSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> PSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_VSSetShader(This,pVertexShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> VSSetShader(This,pVertexShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext4_DrawIndexed(This,IndexCount,StartIndexLocation,BaseVertexLocation)	\
    ( (This)->lpVtbl -> DrawIndexed(This,IndexCount,StartIndexLocation,BaseVertexLocation) ) 

#define ID3D11DeviceContext4_Draw(This,VertexCount,StartVertexLocation)	\
    ( (This)->lpVtbl -> Draw(This,VertexCount,StartVertexLocation) ) 

#define ID3D11DeviceContext4_Map(This,pResource,Subresource,MapType,MapFlags,pMappedResource)	\
    ( (This)->lpVtbl -> Map(This,pResource,Subresource,MapType,MapFlags,pMappedResource) ) 

#define ID3D11DeviceContext4_Unmap(This,pResource,Subresource)	\
    ( (This)->lpVtbl -> Unmap(This,pResource,Subresource) ) 

#define ID3D11DeviceContext4_PSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> PSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_IASetInputLayout(This,pInputLayout)	\
    ( (This)->lpVtbl -> IASetInputLayout(This,pInputLayout) ) 

#define ID3D11DeviceContext4_IASetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets)	\
    ( (This)->lpVtbl -> IASetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets) ) 

#define ID3D11DeviceContext4_IASetIndexBuffer(This,pIndexBuffer,Format,Offset)	\
    ( (This)->lpVtbl -> IASetIndexBuffer(This,pIndexBuffer,Format,Offset) ) 

#define ID3D11DeviceContext4_DrawIndexedInstanced(This,IndexCountPerInstance,InstanceCount,StartIndexLocation,BaseVertexLocation,StartInstanceLocation)	\
    ( (This)->lpVtbl -> DrawIndexedInstanced(This,IndexCountPerInstance,InstanceCount,StartIndexLocation,BaseVertexLocation,StartInstanceLocation) ) 

#define ID3D11DeviceContext4_DrawInstanced(This,VertexCountPerInstance,InstanceCount,StartVertexLocation,StartInstanceLocation)	\
    ( (This)->lpVtbl -> DrawInstanced(This,VertexCountPerInstance,InstanceCount,StartVertexLocation,StartInstanceLocation) ) 

#define ID3D11DeviceContext4_GSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> GSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_GSSetShader(This,pShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> GSSetShader(This,pShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext4_IASetPrimitiveTopology(This,Topology)	\
    ( (This)->lpVtbl -> IASetPrimitiveTopology(This,Topology) ) 

#define ID3D11DeviceContext4_VSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> VSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_VSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> VSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_Begin(This,pAsync)	\
    ( (This)->lpVtbl -> Begin(This,pAsync) ) 

#define ID3D11DeviceContext4_End(This,pAsync)	\
    ( (This)->lpVtbl -> End(This,pAsync) ) 

#define ID3D11DeviceContext4_GetData(This,pAsync,pData,DataSize,GetDataFlags)	\
    ( (This)->lpVtbl -> GetData(This,pAsync,pData,DataSize,GetDataFlags) ) 

#define ID3D11DeviceContext4_SetPredication(This,pPredicate,PredicateValue)	\
    ( (This)->lpVtbl -> SetPredication(This,pPredicate,PredicateValue) ) 

#define ID3D11DeviceContext4_GSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> GSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_GSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> GSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_OMSetRenderTargets(This,NumViews,ppRenderTargetViews,pDepthStencilView)	\
    ( (This)->lpVtbl -> OMSetRenderTargets(This,NumViews,ppRenderTargetViews,pDepthStencilView) ) 

#define ID3D11DeviceContext4_OMSetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,pDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts)	\
    ( (This)->lpVtbl -> OMSetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,pDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts) ) 

#define ID3D11DeviceContext4_OMSetBlendState(This,pBlendState,BlendFactor,SampleMask)	\
    ( (This)->lpVtbl -> OMSetBlendState(This,pBlendState,BlendFactor,SampleMask) ) 

#define ID3D11DeviceContext4_OMSetDepthStencilState(This,pDepthStencilState,StencilRef)	\
    ( (This)->lpVtbl -> OMSetDepthStencilState(This,pDepthStencilState,StencilRef) ) 

#define ID3D11DeviceContext4_SOSetTargets(This,NumBuffers,ppSOTargets,pOffsets)	\
    ( (This)->lpVtbl -> SOSetTargets(This,NumBuffers,ppSOTargets,pOffsets) ) 

#define ID3D11DeviceContext4_DrawAuto(This)	\
    ( (This)->lpVtbl -> DrawAuto(This) ) 

#define ID3D11DeviceContext4_DrawIndexedInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs)	\
    ( (This)->lpVtbl -> DrawIndexedInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs) ) 

#define ID3D11DeviceContext4_DrawInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs)	\
    ( (This)->lpVtbl -> DrawInstancedIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs) ) 

#define ID3D11DeviceContext4_Dispatch(This,ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ)	\
    ( (This)->lpVtbl -> Dispatch(This,ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ) ) 

#define ID3D11DeviceContext4_DispatchIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs)	\
    ( (This)->lpVtbl -> DispatchIndirect(This,pBufferForArgs,AlignedByteOffsetForArgs) ) 

#define ID3D11DeviceContext4_RSSetState(This,pRasterizerState)	\
    ( (This)->lpVtbl -> RSSetState(This,pRasterizerState) ) 

#define ID3D11DeviceContext4_RSSetViewports(This,NumViewports,pViewports)	\
    ( (This)->lpVtbl -> RSSetViewports(This,NumViewports,pViewports) ) 

#define ID3D11DeviceContext4_RSSetScissorRects(This,NumRects,pRects)	\
    ( (This)->lpVtbl -> RSSetScissorRects(This,NumRects,pRects) ) 

#define ID3D11DeviceContext4_CopySubresourceRegion(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox)	\
    ( (This)->lpVtbl -> CopySubresourceRegion(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox) ) 

#define ID3D11DeviceContext4_CopyResource(This,pDstResource,pSrcResource)	\
    ( (This)->lpVtbl -> CopyResource(This,pDstResource,pSrcResource) ) 

#define ID3D11DeviceContext4_UpdateSubresource(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch)	\
    ( (This)->lpVtbl -> UpdateSubresource(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch) ) 

#define ID3D11DeviceContext4_CopyStructureCount(This,pDstBuffer,DstAlignedByteOffset,pSrcView)	\
    ( (This)->lpVtbl -> CopyStructureCount(This,pDstBuffer,DstAlignedByteOffset,pSrcView) ) 

#define ID3D11DeviceContext4_ClearRenderTargetView(This,pRenderTargetView,ColorRGBA)	\
    ( (This)->lpVtbl -> ClearRenderTargetView(This,pRenderTargetView,ColorRGBA) ) 

#define ID3D11DeviceContext4_ClearUnorderedAccessViewUint(This,pUnorderedAccessView,Values)	\
    ( (This)->lpVtbl -> ClearUnorderedAccessViewUint(This,pUnorderedAccessView,Values) ) 

#define ID3D11DeviceContext4_ClearUnorderedAccessViewFloat(This,pUnorderedAccessView,Values)	\
    ( (This)->lpVtbl -> ClearUnorderedAccessViewFloat(This,pUnorderedAccessView,Values) ) 

#define ID3D11DeviceContext4_ClearDepthStencilView(This,pDepthStencilView,ClearFlags,Depth,Stencil)	\
    ( (This)->lpVtbl -> ClearDepthStencilView(This,pDepthStencilView,ClearFlags,Depth,Stencil) ) 

#define ID3D11DeviceContext4_GenerateMips(This,pShaderResourceView)	\
    ( (This)->lpVtbl -> GenerateMips(This,pShaderResourceView) ) 

#define ID3D11DeviceContext4_SetResourceMinLOD(This,pResource,MinLOD)	\
    ( (This)->lpVtbl -> SetResourceMinLOD(This,pResource,MinLOD) ) 

#define ID3D11DeviceContext4_GetResourceMinLOD(This,pResource)	\
    ( (This)->lpVtbl -> GetResourceMinLOD(This,pResource) ) 

#define ID3D11DeviceContext4_ResolveSubresource(This,pDstResource,DstSubresource,pSrcResource,SrcSubresource,Format)	\
    ( (This)->lpVtbl -> ResolveSubresource(This,pDstResource,DstSubresource,pSrcResource,SrcSubresource,Format) ) 

#define ID3D11DeviceContext4_ExecuteCommandList(This,pCommandList,RestoreContextState)	\
    ( (This)->lpVtbl -> ExecuteCommandList(This,pCommandList,RestoreContextState) ) 

#define ID3D11DeviceContext4_HSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> HSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_HSSetShader(This,pHullShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> HSSetShader(This,pHullShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext4_HSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> HSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_HSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> HSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_DSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> DSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_DSSetShader(This,pDomainShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> DSSetShader(This,pDomainShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext4_DSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> DSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_DSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> DSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_CSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> CSSetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_CSSetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts)	\
    ( (This)->lpVtbl -> CSSetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews,pUAVInitialCounts) ) 

#define ID3D11DeviceContext4_CSSetShader(This,pComputeShader,ppClassInstances,NumClassInstances)	\
    ( (This)->lpVtbl -> CSSetShader(This,pComputeShader,ppClassInstances,NumClassInstances) ) 

#define ID3D11DeviceContext4_CSSetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> CSSetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_CSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> CSSetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_VSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> VSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_PSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> PSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_PSGetShader(This,ppPixelShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> PSGetShader(This,ppPixelShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext4_PSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> PSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_VSGetShader(This,ppVertexShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> VSGetShader(This,ppVertexShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext4_PSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> PSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_IAGetInputLayout(This,ppInputLayout)	\
    ( (This)->lpVtbl -> IAGetInputLayout(This,ppInputLayout) ) 

#define ID3D11DeviceContext4_IAGetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets)	\
    ( (This)->lpVtbl -> IAGetVertexBuffers(This,StartSlot,NumBuffers,ppVertexBuffers,pStrides,pOffsets) ) 

#define ID3D11DeviceContext4_IAGetIndexBuffer(This,pIndexBuffer,Format,Offset)	\
    ( (This)->lpVtbl -> IAGetIndexBuffer(This,pIndexBuffer,Format,Offset) ) 

#define ID3D11DeviceContext4_GSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> GSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_GSGetShader(This,ppGeometryShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> GSGetShader(This,ppGeometryShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext4_IAGetPrimitiveTopology(This,pTopology)	\
    ( (This)->lpVtbl -> IAGetPrimitiveTopology(This,pTopology) ) 

#define ID3D11DeviceContext4_VSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> VSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_VSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> VSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_GetPredication(This,ppPredicate,pPredicateValue)	\
    ( (This)->lpVtbl -> GetPredication(This,ppPredicate,pPredicateValue) ) 

#define ID3D11DeviceContext4_GSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> GSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_GSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> GSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_OMGetRenderTargets(This,NumViews,ppRenderTargetViews,ppDepthStencilView)	\
    ( (This)->lpVtbl -> OMGetRenderTargets(This,NumViews,ppRenderTargetViews,ppDepthStencilView) ) 

#define ID3D11DeviceContext4_OMGetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,ppDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews)	\
    ( (This)->lpVtbl -> OMGetRenderTargetsAndUnorderedAccessViews(This,NumRTVs,ppRenderTargetViews,ppDepthStencilView,UAVStartSlot,NumUAVs,ppUnorderedAccessViews) ) 

#define ID3D11DeviceContext4_OMGetBlendState(This,ppBlendState,BlendFactor,pSampleMask)	\
    ( (This)->lpVtbl -> OMGetBlendState(This,ppBlendState,BlendFactor,pSampleMask) ) 

#define ID3D11DeviceContext4_OMGetDepthStencilState(This,ppDepthStencilState,pStencilRef)	\
    ( (This)->lpVtbl -> OMGetDepthStencilState(This,ppDepthStencilState,pStencilRef) ) 

#define ID3D11DeviceContext4_SOGetTargets(This,NumBuffers,ppSOTargets)	\
    ( (This)->lpVtbl -> SOGetTargets(This,NumBuffers,ppSOTargets) ) 

#define ID3D11DeviceContext4_RSGetState(This,ppRasterizerState)	\
    ( (This)->lpVtbl -> RSGetState(This,ppRasterizerState) ) 

#define ID3D11DeviceContext4_RSGetViewports(This,pNumViewports,pViewports)	\
    ( (This)->lpVtbl -> RSGetViewports(This,pNumViewports,pViewports) ) 

#define ID3D11DeviceContext4_RSGetScissorRects(This,pNumRects,pRects)	\
    ( (This)->lpVtbl -> RSGetScissorRects(This,pNumRects,pRects) ) 

#define ID3D11DeviceContext4_HSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> HSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_HSGetShader(This,ppHullShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> HSGetShader(This,ppHullShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext4_HSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> HSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_HSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> HSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_DSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> DSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_DSGetShader(This,ppDomainShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> DSGetShader(This,ppDomainShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext4_DSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> DSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_DSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> DSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_CSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews)	\
    ( (This)->lpVtbl -> CSGetShaderResources(This,StartSlot,NumViews,ppShaderResourceViews) ) 

#define ID3D11DeviceContext4_CSGetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews)	\
    ( (This)->lpVtbl -> CSGetUnorderedAccessViews(This,StartSlot,NumUAVs,ppUnorderedAccessViews) ) 

#define ID3D11DeviceContext4_CSGetShader(This,ppComputeShader,ppClassInstances,pNumClassInstances)	\
    ( (This)->lpVtbl -> CSGetShader(This,ppComputeShader,ppClassInstances,pNumClassInstances) ) 

#define ID3D11DeviceContext4_CSGetSamplers(This,StartSlot,NumSamplers,ppSamplers)	\
    ( (This)->lpVtbl -> CSGetSamplers(This,StartSlot,NumSamplers,ppSamplers) ) 

#define ID3D11DeviceContext4_CSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers)	\
    ( (This)->lpVtbl -> CSGetConstantBuffers(This,StartSlot,NumBuffers,ppConstantBuffers) ) 

#define ID3D11DeviceContext4_ClearState(This)	\
    ( (This)->lpVtbl -> ClearState(This) ) 

#define ID3D11DeviceContext4_Flush(This)	\
    ( (This)->lpVtbl -> Flush(This) ) 

#define ID3D11DeviceContext4_GetType(This)	\
    ( (This)->lpVtbl -> GetType(This) ) 

#define ID3D11DeviceContext4_GetContextFlags(This)	\
    ( (This)->lpVtbl -> GetContextFlags(This) ) 

#define ID3D11DeviceContext4_FinishCommandList(This,RestoreDeferredContextState,ppCommandList)	\
    ( (This)->lpVtbl -> FinishCommandList(This,RestoreDeferredContextState,ppCommandList) ) 


#define ID3D11DeviceContext4_CopySubresourceRegion1(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox,CopyFlags)	\
    ( (This)->lpVtbl -> CopySubresourceRegion1(This,pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox,CopyFlags) ) 

#define ID3D11DeviceContext4_UpdateSubresource1(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch,CopyFlags)	\
    ( (This)->lpVtbl -> UpdateSubresource1(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch,CopyFlags) ) 

#define ID3D11DeviceContext4_DiscardResource(This,pResource)	\
    ( (This)->lpVtbl -> DiscardResource(This,pResource) ) 

#define ID3D11DeviceContext4_DiscardView(This,pResourceView)	\
    ( (This)->lpVtbl -> DiscardView(This,pResourceView) ) 

#define ID3D11DeviceContext4_VSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> VSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_HSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> HSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_DSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> DSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_GSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> GSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_PSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> PSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_CSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> CSSetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_VSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> VSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_HSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> HSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_DSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> DSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_GSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> GSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_PSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> PSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_CSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants)	\
    ( (This)->lpVtbl -> CSGetConstantBuffers1(This,StartSlot,NumBuffers,ppConstantBuffers,pFirstConstant,pNumConstants) ) 

#define ID3D11DeviceContext4_SwapDeviceContextState(This,pState,ppPreviousState)	\
    ( (This)->lpVtbl -> SwapDeviceContextState(This,pState,ppPreviousState) ) 

#define ID3D11DeviceContext4_ClearView(This,pView,Color,pRect,NumRects)	\
    ( (This)->lpVtbl -> ClearView(This,pView,Color,pRect,NumRects) ) 

#define ID3D11DeviceContext4_DiscardView1(This,pResourceView,pRects,NumRects)	\
    ( (This)->lpVtbl -> DiscardView1(This,pResourceView,pRects,NumRects) ) 


#define ID3D11DeviceContext4_UpdateTileMappings(This,pTiledResource,NumTiledResourceRegions,pTiledResourceRegionStartCoordinates,pTiledResourceRegionSizes,pTilePool,NumRanges,pRangeFlags,pTilePoolStartOffsets,pRangeTileCounts,Flags)	\
    ( (This)->lpVtbl -> UpdateTileMappings(This,pTiledResource,NumTiledResourceRegions,pTiledResourceRegionStartCoordinates,pTiledResourceRegionSizes,pTilePool,NumRanges,pRangeFlags,pTilePoolStartOffsets,pRangeTileCounts,Flags) ) 

#define ID3D11DeviceContext4_CopyTileMappings(This,pDestTiledResource,pDestRegionStartCoordinate,pSourceTiledResource,pSourceRegionStartCoordinate,pTileRegionSize,Flags)	\
    ( (This)->lpVtbl -> CopyTileMappings(This,pDestTiledResource,pDestRegionStartCoordinate,pSourceTiledResource,pSourceRegionStartCoordinate,pTileRegionSize,Flags) ) 

#define ID3D11DeviceContext4_CopyTiles(This,pTiledResource,pTileRegionStartCoordinate,pTileRegionSize,pBuffer,BufferStartOffsetInBytes,Flags)	\
    ( (This)->lpVtbl -> CopyTiles(This,pTiledResource,pTileRegionStartCoordinate,pTileRegionSize,pBuffer,BufferStartOffsetInBytes,Flags) ) 

#define ID3D11DeviceContext4_UpdateTiles(This,pDestTiledResource,pDestTileRegionStartCoordinate,pDestTileRegionSize,pSourceTileData,Flags)	\
    ( (This)->lpVtbl -> UpdateTiles(This,pDestTiledResource,pDestTileRegionStartCoordinate,pDestTileRegionSize,pSourceTileData,Flags) ) 

#define ID3D11DeviceContext4_ResizeTilePool(This,pTilePool,NewSizeInBytes)	\
    ( (This)->lpVtbl -> ResizeTilePool(This,pTilePool,NewSizeInBytes) ) 

#define ID3D11DeviceContext4_TiledResourceBarrier(This,pTiledResourceOrViewAccessBeforeBarrier,pTiledResourceOrViewAccessAfterBarrier)	\
    ( (This)->lpVtbl -> TiledResourceBarrier(This,pTiledResourceOrViewAccessBeforeBarrier,pTiledResourceOrViewAccessAfterBarrier) ) 

#define ID3D11DeviceContext4_IsAnnotationEnabled(This)	\
    ( (This)->lpVtbl -> IsAnnotationEnabled(This) ) 

#define ID3D11DeviceContext4_SetMarkerInt(This,pLabel,Data)	\
    ( (This)->lpVtbl -> SetMarkerInt(This,pLabel,Data) ) 

#define ID3D11DeviceContext4_BeginEventInt(This,pLabel,Data)	\
    ( (This)->lpVtbl -> BeginEventInt(This,pLabel,Data) ) 

#define ID3D11DeviceContext4_EndEvent(This)	\
    ( (This)->lpVtbl -> EndEvent(This) ) 


#define ID3D11DeviceContext4_Flush1(This,ContextType,hEvent)	\
    ( (This)->lpVtbl -> Flush1(This,ContextType,hEvent) ) 

#define ID3D11DeviceContext4_SetHardwareProtectionState(This,HwProtectionEnable)	\
    ( (This)->lpVtbl -> SetHardwareProtectionState(This,HwProtectionEnable) ) 

#define ID3D11DeviceContext4_GetHardwareProtectionState(This,pHwProtectionEnable)	\
    ( (This)->lpVtbl -> GetHardwareProtectionState(This,pHwProtectionEnable) ) 


#define ID3D11DeviceContext4_Signal(This,pFence,Value)	\
    ( (This)->lpVtbl -> Signal(This,pFence,Value) ) 

#define ID3D11DeviceContext4_Wait(This,pFence,Value)	\
    ( (This)->lpVtbl -> Wait(This,pFence,Value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11DeviceContext4_INTERFACE_DEFINED__ */


#ifndef __ID3D11Device3_INTERFACE_DEFINED__
#define __ID3D11Device3_INTERFACE_DEFINED__

  /* interface ID3D11Device3 */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D11Device3;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("A05C8C37-D2C6-4732-B3A0-9CE0B0DC9AE6")
    ID3D11Device3 : public ID3D11Device2
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE CreateTexture2D1( 
      /* [annotation] */ 
      _In_  const D3D11_TEXTURE2D_DESC1 *pDesc1,
      /* [annotation] */ 
      _In_reads_opt_(_Inexpressible_(pDesc1->MipLevels * pDesc1->ArraySize))  const D3D11_SUBRESOURCE_DATA *pInitialData,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11Texture2D1 **ppTexture2D) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateTexture3D1( 
      /* [annotation] */ 
      _In_  const D3D11_TEXTURE3D_DESC1 *pDesc1,
      /* [annotation] */ 
      _In_reads_opt_(_Inexpressible_(pDesc1->MipLevels))  const D3D11_SUBRESOURCE_DATA *pInitialData,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11Texture3D1 **ppTexture3D) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateRasterizerState2( 
      /* [annotation] */ 
      _In_  const D3D11_RASTERIZER_DESC2 *pRasterizerDesc,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11RasterizerState2 **ppRasterizerState) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateShaderResourceView1( 
      /* [annotation] */ 
      _In_  ID3D11Resource *pResource,
      /* [annotation] */ 
      _In_opt_  const D3D11_SHADER_RESOURCE_VIEW_DESC1 *pDesc1,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11ShaderResourceView1 **ppSRView1) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateUnorderedAccessView1( 
      /* [annotation] */ 
      _In_  ID3D11Resource *pResource,
      /* [annotation] */ 
      _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC1 *pDesc1,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11UnorderedAccessView1 **ppUAView1) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateRenderTargetView1( 
      /* [annotation] */ 
      _In_  ID3D11Resource *pResource,
      /* [annotation] */ 
      _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC1 *pDesc1,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11RenderTargetView1 **ppRTView1) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateQuery1( 
      /* [annotation] */ 
      _In_  const D3D11_QUERY_DESC1 *pQueryDesc1,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11Query1 **ppQuery1) = 0;

    virtual void STDMETHODCALLTYPE GetImmediateContext3( 
      /* [annotation] */ 
      _Outptr_  ID3D11DeviceContext3 **ppImmediateContext) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateDeferredContext3( 
      UINT ContextFlags,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11DeviceContext3 **ppDeferredContext) = 0;

    virtual void STDMETHODCALLTYPE WriteToSubresource( 
      /* [annotation] */ 
      _In_  ID3D11Resource *pDstResource,
      /* [annotation] */ 
      _In_  UINT DstSubresource,
      /* [annotation] */ 
      _In_opt_  const D3D11_BOX *pDstBox,
      /* [annotation] */ 
      _In_  const void *pSrcData,
      /* [annotation] */ 
      _In_  UINT SrcRowPitch,
      /* [annotation] */ 
      _In_  UINT SrcDepthPitch) = 0;

    virtual void STDMETHODCALLTYPE ReadFromSubresource( 
      /* [annotation] */ 
      _Out_  void *pDstData,
      /* [annotation] */ 
      _In_  UINT DstRowPitch,
      /* [annotation] */ 
      _In_  UINT DstDepthPitch,
      /* [annotation] */ 
      _In_  ID3D11Resource *pSrcResource,
      /* [annotation] */ 
      _In_  UINT SrcSubresource,
      /* [annotation] */ 
      _In_opt_  const D3D11_BOX *pSrcBox) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D11Device3Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D11Device3 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D11Device3 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D11Device3 * This);

    HRESULT ( STDMETHODCALLTYPE *CreateBuffer )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  const D3D11_BUFFER_DESC *pDesc,
      /* [annotation] */ 
      _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11Buffer **ppBuffer);

    HRESULT ( STDMETHODCALLTYPE *CreateTexture1D )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  const D3D11_TEXTURE1D_DESC *pDesc,
      /* [annotation] */ 
      _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))  const D3D11_SUBRESOURCE_DATA *pInitialData,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11Texture1D **ppTexture1D);

    HRESULT ( STDMETHODCALLTYPE *CreateTexture2D )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  const D3D11_TEXTURE2D_DESC *pDesc,
      /* [annotation] */ 
      _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))  const D3D11_SUBRESOURCE_DATA *pInitialData,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11Texture2D **ppTexture2D);

    HRESULT ( STDMETHODCALLTYPE *CreateTexture3D )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  const D3D11_TEXTURE3D_DESC *pDesc,
      /* [annotation] */ 
      _In_reads_opt_(_Inexpressible_(pDesc->MipLevels))  const D3D11_SUBRESOURCE_DATA *pInitialData,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11Texture3D **ppTexture3D);

    HRESULT ( STDMETHODCALLTYPE *CreateShaderResourceView )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pResource,
      /* [annotation] */ 
      _In_opt_  const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11ShaderResourceView **ppSRView);

    HRESULT ( STDMETHODCALLTYPE *CreateUnorderedAccessView )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pResource,
      /* [annotation] */ 
      _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11UnorderedAccessView **ppUAView);

    HRESULT ( STDMETHODCALLTYPE *CreateRenderTargetView )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pResource,
      /* [annotation] */ 
      _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC *pDesc,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11RenderTargetView **ppRTView);

    HRESULT ( STDMETHODCALLTYPE *CreateDepthStencilView )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pResource,
      /* [annotation] */ 
      _In_opt_  const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11DepthStencilView **ppDepthStencilView);

    HRESULT ( STDMETHODCALLTYPE *CreateInputLayout )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_reads_(NumElements)  const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs,
      /* [annotation] */ 
      _In_range_( 0, D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT )  UINT NumElements,
      /* [annotation] */ 
      _In_reads_(BytecodeLength)  const void *pShaderBytecodeWithInputSignature,
      /* [annotation] */ 
      _In_  SIZE_T BytecodeLength,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11InputLayout **ppInputLayout);

    HRESULT ( STDMETHODCALLTYPE *CreateVertexShader )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_reads_(BytecodeLength)  const void *pShaderBytecode,
      /* [annotation] */ 
      _In_  SIZE_T BytecodeLength,
      /* [annotation] */ 
      _In_opt_  ID3D11ClassLinkage *pClassLinkage,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11VertexShader **ppVertexShader);

    HRESULT ( STDMETHODCALLTYPE *CreateGeometryShader )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_reads_(BytecodeLength)  const void *pShaderBytecode,
      /* [annotation] */ 
      _In_  SIZE_T BytecodeLength,
      /* [annotation] */ 
      _In_opt_  ID3D11ClassLinkage *pClassLinkage,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11GeometryShader **ppGeometryShader);

    HRESULT ( STDMETHODCALLTYPE *CreateGeometryShaderWithStreamOutput )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_reads_(BytecodeLength)  const void *pShaderBytecode,
      /* [annotation] */ 
      _In_  SIZE_T BytecodeLength,
      /* [annotation] */ 
      _In_reads_opt_(NumEntries)  const D3D11_SO_DECLARATION_ENTRY *pSODeclaration,
      /* [annotation] */ 
      _In_range_( 0, D3D11_SO_STREAM_COUNT * D3D11_SO_OUTPUT_COMPONENT_COUNT )  UINT NumEntries,
      /* [annotation] */ 
      _In_reads_opt_(NumStrides)  const UINT *pBufferStrides,
      /* [annotation] */ 
      _In_range_( 0, D3D11_SO_BUFFER_SLOT_COUNT )  UINT NumStrides,
      /* [annotation] */ 
      _In_  UINT RasterizedStream,
      /* [annotation] */ 
      _In_opt_  ID3D11ClassLinkage *pClassLinkage,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11GeometryShader **ppGeometryShader);

    HRESULT ( STDMETHODCALLTYPE *CreatePixelShader )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_reads_(BytecodeLength)  const void *pShaderBytecode,
      /* [annotation] */ 
      _In_  SIZE_T BytecodeLength,
      /* [annotation] */ 
      _In_opt_  ID3D11ClassLinkage *pClassLinkage,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11PixelShader **ppPixelShader);

    HRESULT ( STDMETHODCALLTYPE *CreateHullShader )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_reads_(BytecodeLength)  const void *pShaderBytecode,
      /* [annotation] */ 
      _In_  SIZE_T BytecodeLength,
      /* [annotation] */ 
      _In_opt_  ID3D11ClassLinkage *pClassLinkage,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11HullShader **ppHullShader);

    HRESULT ( STDMETHODCALLTYPE *CreateDomainShader )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_reads_(BytecodeLength)  const void *pShaderBytecode,
      /* [annotation] */ 
      _In_  SIZE_T BytecodeLength,
      /* [annotation] */ 
      _In_opt_  ID3D11ClassLinkage *pClassLinkage,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11DomainShader **ppDomainShader);

    HRESULT ( STDMETHODCALLTYPE *CreateComputeShader )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_reads_(BytecodeLength)  const void *pShaderBytecode,
      /* [annotation] */ 
      _In_  SIZE_T BytecodeLength,
      /* [annotation] */ 
      _In_opt_  ID3D11ClassLinkage *pClassLinkage,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11ComputeShader **ppComputeShader);

    HRESULT ( STDMETHODCALLTYPE *CreateClassLinkage )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _COM_Outptr_  ID3D11ClassLinkage **ppLinkage);

    HRESULT ( STDMETHODCALLTYPE *CreateBlendState )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  const D3D11_BLEND_DESC *pBlendStateDesc,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11BlendState **ppBlendState);

    HRESULT ( STDMETHODCALLTYPE *CreateDepthStencilState )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11DepthStencilState **ppDepthStencilState);

    HRESULT ( STDMETHODCALLTYPE *CreateRasterizerState )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  const D3D11_RASTERIZER_DESC *pRasterizerDesc,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11RasterizerState **ppRasterizerState);

    HRESULT ( STDMETHODCALLTYPE *CreateSamplerState )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  const D3D11_SAMPLER_DESC *pSamplerDesc,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11SamplerState **ppSamplerState);

    HRESULT ( STDMETHODCALLTYPE *CreateQuery )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  const D3D11_QUERY_DESC *pQueryDesc,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11Query **ppQuery);

    HRESULT ( STDMETHODCALLTYPE *CreatePredicate )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  const D3D11_QUERY_DESC *pPredicateDesc,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11Predicate **ppPredicate);

    HRESULT ( STDMETHODCALLTYPE *CreateCounter )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  const D3D11_COUNTER_DESC *pCounterDesc,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11Counter **ppCounter);

    HRESULT ( STDMETHODCALLTYPE *CreateDeferredContext )( 
      ID3D11Device3 * This,
      UINT ContextFlags,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11DeviceContext **ppDeferredContext);

    HRESULT ( STDMETHODCALLTYPE *OpenSharedResource )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  HANDLE hResource,
      /* [annotation] */ 
      _In_  REFIID ReturnedInterface,
      /* [annotation] */ 
      _COM_Outptr_opt_  void **ppResource);

    HRESULT ( STDMETHODCALLTYPE *CheckFormatSupport )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  DXGI_FORMAT Format,
      /* [annotation] */ 
      _Out_  UINT *pFormatSupport);

    HRESULT ( STDMETHODCALLTYPE *CheckMultisampleQualityLevels )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  DXGI_FORMAT Format,
      /* [annotation] */ 
      _In_  UINT SampleCount,
      /* [annotation] */ 
      _Out_  UINT *pNumQualityLevels);

    void ( STDMETHODCALLTYPE *CheckCounterInfo )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _Out_  D3D11_COUNTER_INFO *pCounterInfo);

    HRESULT ( STDMETHODCALLTYPE *CheckCounter )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  const D3D11_COUNTER_DESC *pDesc,
      /* [annotation] */ 
      _Out_  D3D11_COUNTER_TYPE *pType,
      /* [annotation] */ 
      _Out_  UINT *pActiveCounters,
      /* [annotation] */ 
      _Out_writes_opt_(*pNameLength)  LPSTR szName,
      /* [annotation] */ 
      _Inout_opt_  UINT *pNameLength,
      /* [annotation] */ 
      _Out_writes_opt_(*pUnitsLength)  LPSTR szUnits,
      /* [annotation] */ 
      _Inout_opt_  UINT *pUnitsLength,
      /* [annotation] */ 
      _Out_writes_opt_(*pDescriptionLength)  LPSTR szDescription,
      /* [annotation] */ 
      _Inout_opt_  UINT *pDescriptionLength);

    HRESULT ( STDMETHODCALLTYPE *CheckFeatureSupport )( 
      ID3D11Device3 * This,
      D3D11_FEATURE Feature,
      /* [annotation] */ 
      _Out_writes_bytes_(FeatureSupportDataSize)  void *pFeatureSupportData,
      UINT FeatureSupportDataSize);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation] */ 
      _Out_writes_bytes_opt_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_  UINT DataSize,
      /* [annotation] */ 
      _In_reads_bytes_opt_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  REFGUID guid,
      /* [annotation] */ 
      _In_opt_  const IUnknown *pData);

    D3D_FEATURE_LEVEL ( STDMETHODCALLTYPE *GetFeatureLevel )( 
      ID3D11Device3 * This);

    UINT ( STDMETHODCALLTYPE *GetCreationFlags )( 
      ID3D11Device3 * This);

    HRESULT ( STDMETHODCALLTYPE *GetDeviceRemovedReason )( 
      ID3D11Device3 * This);

    void ( STDMETHODCALLTYPE *GetImmediateContext )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _Outptr_  ID3D11DeviceContext **ppImmediateContext);

    HRESULT ( STDMETHODCALLTYPE *SetExceptionMode )( 
      ID3D11Device3 * This,
      UINT RaiseFlags);

    UINT ( STDMETHODCALLTYPE *GetExceptionMode )( 
      ID3D11Device3 * This);

    void ( STDMETHODCALLTYPE *GetImmediateContext1 )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _Outptr_  ID3D11DeviceContext1 **ppImmediateContext);

    HRESULT ( STDMETHODCALLTYPE *CreateDeferredContext1 )( 
      ID3D11Device3 * This,
      UINT ContextFlags,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11DeviceContext1 **ppDeferredContext);

    HRESULT ( STDMETHODCALLTYPE *CreateBlendState1 )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  const D3D11_BLEND_DESC1 *pBlendStateDesc,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11BlendState1 **ppBlendState);

    HRESULT ( STDMETHODCALLTYPE *CreateRasterizerState1 )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  const D3D11_RASTERIZER_DESC1 *pRasterizerDesc,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11RasterizerState1 **ppRasterizerState);

    HRESULT ( STDMETHODCALLTYPE *CreateDeviceContextState )( 
      ID3D11Device3 * This,
      UINT Flags,
      /* [annotation] */ 
      _In_reads_( FeatureLevels )  const D3D_FEATURE_LEVEL *pFeatureLevels,
      UINT FeatureLevels,
      UINT SDKVersion,
      REFIID EmulatedInterface,
      /* [annotation] */ 
      _Out_opt_  D3D_FEATURE_LEVEL *pChosenFeatureLevel,
      /* [annotation] */ 
      _Out_opt_  ID3DDeviceContextState **ppContextState);

    HRESULT ( STDMETHODCALLTYPE *OpenSharedResource1 )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  HANDLE hResource,
      /* [annotation] */ 
      _In_  REFIID returnedInterface,
      /* [annotation] */ 
      _COM_Outptr_  void **ppResource);

    HRESULT ( STDMETHODCALLTYPE *OpenSharedResourceByName )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  LPCWSTR lpName,
      /* [annotation] */ 
      _In_  DWORD dwDesiredAccess,
      /* [annotation] */ 
      _In_  REFIID returnedInterface,
      /* [annotation] */ 
      _COM_Outptr_  void **ppResource);

    void ( STDMETHODCALLTYPE *GetImmediateContext2 )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _Outptr_  ID3D11DeviceContext2 **ppImmediateContext);

    HRESULT ( STDMETHODCALLTYPE *CreateDeferredContext2 )( 
      ID3D11Device3 * This,
      UINT ContextFlags,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11DeviceContext2 **ppDeferredContext);

    void ( STDMETHODCALLTYPE *GetResourceTiling )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pTiledResource,
      /* [annotation] */ 
      _Out_opt_  UINT *pNumTilesForEntireResource,
      /* [annotation] */ 
      _Out_opt_  D3D11_PACKED_MIP_DESC *pPackedMipDesc,
      /* [annotation] */ 
      _Out_opt_  D3D11_TILE_SHAPE *pStandardTileShapeForNonPackedMips,
      /* [annotation] */ 
      _Inout_opt_  UINT *pNumSubresourceTilings,
      /* [annotation] */ 
      _In_  UINT FirstSubresourceTilingToGet,
      /* [annotation] */ 
      _Out_writes_(*pNumSubresourceTilings)  D3D11_SUBRESOURCE_TILING *pSubresourceTilingsForNonPackedMips);

    HRESULT ( STDMETHODCALLTYPE *CheckMultisampleQualityLevels1 )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  DXGI_FORMAT Format,
      /* [annotation] */ 
      _In_  UINT SampleCount,
      /* [annotation] */ 
      _In_  UINT Flags,
      /* [annotation] */ 
      _Out_  UINT *pNumQualityLevels);

    HRESULT ( STDMETHODCALLTYPE *CreateTexture2D1 )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  const D3D11_TEXTURE2D_DESC1 *pDesc1,
      /* [annotation] */ 
      _In_reads_opt_(_Inexpressible_(pDesc1->MipLevels * pDesc1->ArraySize))  const D3D11_SUBRESOURCE_DATA *pInitialData,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11Texture2D1 **ppTexture2D);

    HRESULT ( STDMETHODCALLTYPE *CreateTexture3D1 )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  const D3D11_TEXTURE3D_DESC1 *pDesc1,
      /* [annotation] */ 
      _In_reads_opt_(_Inexpressible_(pDesc1->MipLevels))  const D3D11_SUBRESOURCE_DATA *pInitialData,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11Texture3D1 **ppTexture3D);

    HRESULT ( STDMETHODCALLTYPE *CreateRasterizerState2 )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  const D3D11_RASTERIZER_DESC2 *pRasterizerDesc,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11RasterizerState2 **ppRasterizerState);

    HRESULT ( STDMETHODCALLTYPE *CreateShaderResourceView1 )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pResource,
      /* [annotation] */ 
      _In_opt_  const D3D11_SHADER_RESOURCE_VIEW_DESC1 *pDesc1,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11ShaderResourceView1 **ppSRView1);

    HRESULT ( STDMETHODCALLTYPE *CreateUnorderedAccessView1 )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pResource,
      /* [annotation] */ 
      _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC1 *pDesc1,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11UnorderedAccessView1 **ppUAView1);

    HRESULT ( STDMETHODCALLTYPE *CreateRenderTargetView1 )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pResource,
      /* [annotation] */ 
      _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC1 *pDesc1,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11RenderTargetView1 **ppRTView1);

    HRESULT ( STDMETHODCALLTYPE *CreateQuery1 )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  const D3D11_QUERY_DESC1 *pQueryDesc1,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11Query1 **ppQuery1);

    void ( STDMETHODCALLTYPE *GetImmediateContext3 )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _Outptr_  ID3D11DeviceContext3 **ppImmediateContext);

    HRESULT ( STDMETHODCALLTYPE *CreateDeferredContext3 )( 
      ID3D11Device3 * This,
      UINT ContextFlags,
      /* [annotation] */ 
      _COM_Outptr_opt_  ID3D11DeviceContext3 **ppDeferredContext);

    void ( STDMETHODCALLTYPE *WriteToSubresource )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _In_  ID3D11Resource *pDstResource,
      /* [annotation] */ 
      _In_  UINT DstSubresource,
      /* [annotation] */ 
      _In_opt_  const D3D11_BOX *pDstBox,
      /* [annotation] */ 
      _In_  const void *pSrcData,
      /* [annotation] */ 
      _In_  UINT SrcRowPitch,
      /* [annotation] */ 
      _In_  UINT SrcDepthPitch);

    void ( STDMETHODCALLTYPE *ReadFromSubresource )( 
      ID3D11Device3 * This,
      /* [annotation] */ 
      _Out_  void *pDstData,
      /* [annotation] */ 
      _In_  UINT DstRowPitch,
      /* [annotation] */ 
      _In_  UINT DstDepthPitch,
      /* [annotation] */ 
      _In_  ID3D11Resource *pSrcResource,
      /* [annotation] */ 
      _In_  UINT SrcSubresource,
      /* [annotation] */ 
      _In_opt_  const D3D11_BOX *pSrcBox);

    END_INTERFACE
  } ID3D11Device3Vtbl;

  interface ID3D11Device3
  {
    CONST_VTBL struct ID3D11Device3Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D11Device3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11Device3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11Device3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11Device3_CreateBuffer(This,pDesc,pInitialData,ppBuffer)	\
    ( (This)->lpVtbl -> CreateBuffer(This,pDesc,pInitialData,ppBuffer) ) 

#define ID3D11Device3_CreateTexture1D(This,pDesc,pInitialData,ppTexture1D)	\
    ( (This)->lpVtbl -> CreateTexture1D(This,pDesc,pInitialData,ppTexture1D) ) 

#define ID3D11Device3_CreateTexture2D(This,pDesc,pInitialData,ppTexture2D)	\
    ( (This)->lpVtbl -> CreateTexture2D(This,pDesc,pInitialData,ppTexture2D) ) 

#define ID3D11Device3_CreateTexture3D(This,pDesc,pInitialData,ppTexture3D)	\
    ( (This)->lpVtbl -> CreateTexture3D(This,pDesc,pInitialData,ppTexture3D) ) 

#define ID3D11Device3_CreateShaderResourceView(This,pResource,pDesc,ppSRView)	\
    ( (This)->lpVtbl -> CreateShaderResourceView(This,pResource,pDesc,ppSRView) ) 

#define ID3D11Device3_CreateUnorderedAccessView(This,pResource,pDesc,ppUAView)	\
    ( (This)->lpVtbl -> CreateUnorderedAccessView(This,pResource,pDesc,ppUAView) ) 

#define ID3D11Device3_CreateRenderTargetView(This,pResource,pDesc,ppRTView)	\
    ( (This)->lpVtbl -> CreateRenderTargetView(This,pResource,pDesc,ppRTView) ) 

#define ID3D11Device3_CreateDepthStencilView(This,pResource,pDesc,ppDepthStencilView)	\
    ( (This)->lpVtbl -> CreateDepthStencilView(This,pResource,pDesc,ppDepthStencilView) ) 

#define ID3D11Device3_CreateInputLayout(This,pInputElementDescs,NumElements,pShaderBytecodeWithInputSignature,BytecodeLength,ppInputLayout)	\
    ( (This)->lpVtbl -> CreateInputLayout(This,pInputElementDescs,NumElements,pShaderBytecodeWithInputSignature,BytecodeLength,ppInputLayout) ) 

#define ID3D11Device3_CreateVertexShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppVertexShader)	\
    ( (This)->lpVtbl -> CreateVertexShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppVertexShader) ) 

#define ID3D11Device3_CreateGeometryShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppGeometryShader)	\
    ( (This)->lpVtbl -> CreateGeometryShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppGeometryShader) ) 

#define ID3D11Device3_CreateGeometryShaderWithStreamOutput(This,pShaderBytecode,BytecodeLength,pSODeclaration,NumEntries,pBufferStrides,NumStrides,RasterizedStream,pClassLinkage,ppGeometryShader)	\
    ( (This)->lpVtbl -> CreateGeometryShaderWithStreamOutput(This,pShaderBytecode,BytecodeLength,pSODeclaration,NumEntries,pBufferStrides,NumStrides,RasterizedStream,pClassLinkage,ppGeometryShader) ) 

#define ID3D11Device3_CreatePixelShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppPixelShader)	\
    ( (This)->lpVtbl -> CreatePixelShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppPixelShader) ) 

#define ID3D11Device3_CreateHullShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppHullShader)	\
    ( (This)->lpVtbl -> CreateHullShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppHullShader) ) 

#define ID3D11Device3_CreateDomainShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppDomainShader)	\
    ( (This)->lpVtbl -> CreateDomainShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppDomainShader) ) 

#define ID3D11Device3_CreateComputeShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppComputeShader)	\
    ( (This)->lpVtbl -> CreateComputeShader(This,pShaderBytecode,BytecodeLength,pClassLinkage,ppComputeShader) ) 

#define ID3D11Device3_CreateClassLinkage(This,ppLinkage)	\
    ( (This)->lpVtbl -> CreateClassLinkage(This,ppLinkage) ) 

#define ID3D11Device3_CreateBlendState(This,pBlendStateDesc,ppBlendState)	\
    ( (This)->lpVtbl -> CreateBlendState(This,pBlendStateDesc,ppBlendState) ) 

#define ID3D11Device3_CreateDepthStencilState(This,pDepthStencilDesc,ppDepthStencilState)	\
    ( (This)->lpVtbl -> CreateDepthStencilState(This,pDepthStencilDesc,ppDepthStencilState) ) 

#define ID3D11Device3_CreateRasterizerState(This,pRasterizerDesc,ppRasterizerState)	\
    ( (This)->lpVtbl -> CreateRasterizerState(This,pRasterizerDesc,ppRasterizerState) ) 

#define ID3D11Device3_CreateSamplerState(This,pSamplerDesc,ppSamplerState)	\
    ( (This)->lpVtbl -> CreateSamplerState(This,pSamplerDesc,ppSamplerState) ) 

#define ID3D11Device3_CreateQuery(This,pQueryDesc,ppQuery)	\
    ( (This)->lpVtbl -> CreateQuery(This,pQueryDesc,ppQuery) ) 

#define ID3D11Device3_CreatePredicate(This,pPredicateDesc,ppPredicate)	\
    ( (This)->lpVtbl -> CreatePredicate(This,pPredicateDesc,ppPredicate) ) 

#define ID3D11Device3_CreateCounter(This,pCounterDesc,ppCounter)	\
    ( (This)->lpVtbl -> CreateCounter(This,pCounterDesc,ppCounter) ) 

#define ID3D11Device3_CreateDeferredContext(This,ContextFlags,ppDeferredContext)	\
    ( (This)->lpVtbl -> CreateDeferredContext(This,ContextFlags,ppDeferredContext) ) 

#define ID3D11Device3_OpenSharedResource(This,hResource,ReturnedInterface,ppResource)	\
    ( (This)->lpVtbl -> OpenSharedResource(This,hResource,ReturnedInterface,ppResource) ) 

#define ID3D11Device3_CheckFormatSupport(This,Format,pFormatSupport)	\
    ( (This)->lpVtbl -> CheckFormatSupport(This,Format,pFormatSupport) ) 

#define ID3D11Device3_CheckMultisampleQualityLevels(This,Format,SampleCount,pNumQualityLevels)	\
    ( (This)->lpVtbl -> CheckMultisampleQualityLevels(This,Format,SampleCount,pNumQualityLevels) ) 

#define ID3D11Device3_CheckCounterInfo(This,pCounterInfo)	\
    ( (This)->lpVtbl -> CheckCounterInfo(This,pCounterInfo) ) 

#define ID3D11Device3_CheckCounter(This,pDesc,pType,pActiveCounters,szName,pNameLength,szUnits,pUnitsLength,szDescription,pDescriptionLength)	\
    ( (This)->lpVtbl -> CheckCounter(This,pDesc,pType,pActiveCounters,szName,pNameLength,szUnits,pUnitsLength,szDescription,pDescriptionLength) ) 

#define ID3D11Device3_CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize)	\
    ( (This)->lpVtbl -> CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize) ) 

#define ID3D11Device3_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D11Device3_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D11Device3_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D11Device3_GetFeatureLevel(This)	\
    ( (This)->lpVtbl -> GetFeatureLevel(This) ) 

#define ID3D11Device3_GetCreationFlags(This)	\
    ( (This)->lpVtbl -> GetCreationFlags(This) ) 

#define ID3D11Device3_GetDeviceRemovedReason(This)	\
    ( (This)->lpVtbl -> GetDeviceRemovedReason(This) ) 

#define ID3D11Device3_GetImmediateContext(This,ppImmediateContext)	\
    ( (This)->lpVtbl -> GetImmediateContext(This,ppImmediateContext) ) 

#define ID3D11Device3_SetExceptionMode(This,RaiseFlags)	\
    ( (This)->lpVtbl -> SetExceptionMode(This,RaiseFlags) ) 

#define ID3D11Device3_GetExceptionMode(This)	\
    ( (This)->lpVtbl -> GetExceptionMode(This) ) 


#define ID3D11Device3_GetImmediateContext1(This,ppImmediateContext)	\
    ( (This)->lpVtbl -> GetImmediateContext1(This,ppImmediateContext) ) 

#define ID3D11Device3_CreateDeferredContext1(This,ContextFlags,ppDeferredContext)	\
    ( (This)->lpVtbl -> CreateDeferredContext1(This,ContextFlags,ppDeferredContext) ) 

#define ID3D11Device3_CreateBlendState1(This,pBlendStateDesc,ppBlendState)	\
    ( (This)->lpVtbl -> CreateBlendState1(This,pBlendStateDesc,ppBlendState) ) 

#define ID3D11Device3_CreateRasterizerState1(This,pRasterizerDesc,ppRasterizerState)	\
    ( (This)->lpVtbl -> CreateRasterizerState1(This,pRasterizerDesc,ppRasterizerState) ) 

#define ID3D11Device3_CreateDeviceContextState(This,Flags,pFeatureLevels,FeatureLevels,SDKVersion,EmulatedInterface,pChosenFeatureLevel,ppContextState)	\
    ( (This)->lpVtbl -> CreateDeviceContextState(This,Flags,pFeatureLevels,FeatureLevels,SDKVersion,EmulatedInterface,pChosenFeatureLevel,ppContextState) ) 

#define ID3D11Device3_OpenSharedResource1(This,hResource,returnedInterface,ppResource)	\
    ( (This)->lpVtbl -> OpenSharedResource1(This,hResource,returnedInterface,ppResource) ) 

#define ID3D11Device3_OpenSharedResourceByName(This,lpName,dwDesiredAccess,returnedInterface,ppResource)	\
    ( (This)->lpVtbl -> OpenSharedResourceByName(This,lpName,dwDesiredAccess,returnedInterface,ppResource) ) 


#define ID3D11Device3_GetImmediateContext2(This,ppImmediateContext)	\
    ( (This)->lpVtbl -> GetImmediateContext2(This,ppImmediateContext) ) 

#define ID3D11Device3_CreateDeferredContext2(This,ContextFlags,ppDeferredContext)	\
    ( (This)->lpVtbl -> CreateDeferredContext2(This,ContextFlags,ppDeferredContext) ) 

#define ID3D11Device3_GetResourceTiling(This,pTiledResource,pNumTilesForEntireResource,pPackedMipDesc,pStandardTileShapeForNonPackedMips,pNumSubresourceTilings,FirstSubresourceTilingToGet,pSubresourceTilingsForNonPackedMips)	\
    ( (This)->lpVtbl -> GetResourceTiling(This,pTiledResource,pNumTilesForEntireResource,pPackedMipDesc,pStandardTileShapeForNonPackedMips,pNumSubresourceTilings,FirstSubresourceTilingToGet,pSubresourceTilingsForNonPackedMips) ) 

#define ID3D11Device3_CheckMultisampleQualityLevels1(This,Format,SampleCount,Flags,pNumQualityLevels)	\
    ( (This)->lpVtbl -> CheckMultisampleQualityLevels1(This,Format,SampleCount,Flags,pNumQualityLevels) ) 


#define ID3D11Device3_CreateTexture2D1(This,pDesc1,pInitialData,ppTexture2D)	\
    ( (This)->lpVtbl -> CreateTexture2D1(This,pDesc1,pInitialData,ppTexture2D) ) 

#define ID3D11Device3_CreateTexture3D1(This,pDesc1,pInitialData,ppTexture3D)	\
    ( (This)->lpVtbl -> CreateTexture3D1(This,pDesc1,pInitialData,ppTexture3D) ) 

#define ID3D11Device3_CreateRasterizerState2(This,pRasterizerDesc,ppRasterizerState)	\
    ( (This)->lpVtbl -> CreateRasterizerState2(This,pRasterizerDesc,ppRasterizerState) ) 

#define ID3D11Device3_CreateShaderResourceView1(This,pResource,pDesc1,ppSRView1)	\
    ( (This)->lpVtbl -> CreateShaderResourceView1(This,pResource,pDesc1,ppSRView1) ) 

#define ID3D11Device3_CreateUnorderedAccessView1(This,pResource,pDesc1,ppUAView1)	\
    ( (This)->lpVtbl -> CreateUnorderedAccessView1(This,pResource,pDesc1,ppUAView1) ) 

#define ID3D11Device3_CreateRenderTargetView1(This,pResource,pDesc1,ppRTView1)	\
    ( (This)->lpVtbl -> CreateRenderTargetView1(This,pResource,pDesc1,ppRTView1) ) 

#define ID3D11Device3_CreateQuery1(This,pQueryDesc1,ppQuery1)	\
    ( (This)->lpVtbl -> CreateQuery1(This,pQueryDesc1,ppQuery1) ) 

#define ID3D11Device3_GetImmediateContext3(This,ppImmediateContext)	\
    ( (This)->lpVtbl -> GetImmediateContext3(This,ppImmediateContext) ) 

#define ID3D11Device3_CreateDeferredContext3(This,ContextFlags,ppDeferredContext)	\
    ( (This)->lpVtbl -> CreateDeferredContext3(This,ContextFlags,ppDeferredContext) ) 

#define ID3D11Device3_WriteToSubresource(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch)	\
    ( (This)->lpVtbl -> WriteToSubresource(This,pDstResource,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch) ) 

#define ID3D11Device3_ReadFromSubresource(This,pDstData,DstRowPitch,DstDepthPitch,pSrcResource,SrcSubresource,pSrcBox)	\
    ( (This)->lpVtbl -> ReadFromSubresource(This,pDstData,DstRowPitch,DstDepthPitch,pSrcResource,SrcSubresource,pSrcBox) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11Device3_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_d3d11_3_0000_0011 */
  /* [local] */ 

  DEFINE_GUID(IID_ID3D11Texture2D1,0x51218251,0x1E33,0x4617,0x9C,0xCB,0x4D,0x3A,0x43,0x67,0xE7,0xBB);
  DEFINE_GUID(IID_ID3D11Texture3D1,0x0C711683,0x2853,0x4846,0x9B,0xB0,0xF3,0xE6,0x06,0x39,0xE4,0x6A);
  DEFINE_GUID(IID_ID3D11RasterizerState2,0x6fbd02fb,0x209f,0x46c4,0xb0,0x59,0x2e,0xd1,0x55,0x86,0xa6,0xac);
  DEFINE_GUID(IID_ID3D11ShaderResourceView1,0x91308b87,0x9040,0x411d,0x8c,0x67,0xc3,0x92,0x53,0xce,0x38,0x02);
  DEFINE_GUID(IID_ID3D11RenderTargetView1,0xffbe2e23,0xf011,0x418a,0xac,0x56,0x5c,0xee,0xd7,0xc5,0xb9,0x4b);
  DEFINE_GUID(IID_ID3D11UnorderedAccessView1,0x7b3b6153,0xa886,0x4544,0xab,0x37,0x65,0x37,0xc8,0x50,0x04,0x03);
  DEFINE_GUID(IID_ID3D11Query1,0x631b4766,0x36dc,0x461d,0x8d,0xb6,0xc4,0x7e,0x13,0xe6,0x09,0x16);
  DEFINE_GUID(IID_ID3D11DeviceContext3,0xb4e3c01d,0xe79e,0x4637,0x91,0xb2,0x51,0x0e,0x9f,0x4c,0x9b,0x8f);
  DEFINE_GUID(IID_ID3D11Fence,0xaffde9d1,0x1df7,0x4bb7,0x8a,0x34,0x0f,0x46,0x25,0x1d,0xab,0x80);
  DEFINE_GUID(IID_ID3D11DeviceContext4,0x917600da,0xf58c,0x4c33,0x98,0xd8,0x3e,0x15,0xb3,0x90,0xfa,0x24);
  DEFINE_GUID(IID_ID3D11Device3,0xA05C8C37,0xD2C6,0x4732,0xB3,0xA0,0x9C,0xE0,0xB0,0xDC,0x9A,0xE6);


  extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0011_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_d3d11_3_0000_0011_v0_0_s_ifspec;

  /* Additional Prototypes for ALL interfaces */

  /* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif



#include <d3dcompiler.h>

#include <../depends/include/nvapi/nvapi.h>


// Only accessed by the swapchain thread and only to clear any outstanding
//   references prior to a buffer resize
std::vector <IUnknown *> temp_resources;

SK_Thread_HybridSpinlock cs_shader      (0xc00);
SK_Thread_HybridSpinlock cs_shader_vs   (0x1000);
SK_Thread_HybridSpinlock cs_shader_ps   (0x800);
SK_Thread_HybridSpinlock cs_shader_gs   (0x400);
SK_Thread_HybridSpinlock cs_shader_hs   (0x300);
SK_Thread_HybridSpinlock cs_shader_ds   (0x300);
SK_Thread_HybridSpinlock cs_shader_cs   (0x900);
SK_Thread_HybridSpinlock cs_mmio        (0xe00);
SK_Thread_HybridSpinlock cs_render_view (0xb00) ;

CRITICAL_SECTION tex_cs     = { };
CRITICAL_SECTION hash_cs    = { };
CRITICAL_SECTION dump_cs    = { };
CRITICAL_SECTION cache_cs   = { };
CRITICAL_SECTION inject_cs  = { };
CRITICAL_SECTION preload_cs = { };

LPVOID pfnD3D11CreateDevice             = nullptr;
LPVOID pfnD3D11CreateDeviceAndSwapChain = nullptr;
LPVOID pfnD3D11CoreCreateDevice         = nullptr;

HMODULE SK::DXGI::hModD3D11 = nullptr;

SK::DXGI::PipelineStatsD3D11 SK::DXGI::pipeline_stats_d3d11 = { };

volatile HANDLE hResampleThread = nullptr;

__declspec (noinline)
HRESULT
WINAPI
D3D11CreateDevice_Detour (
  _In_opt_                            IDXGIAdapter         *pAdapter,
                                      D3D_DRIVER_TYPE       DriverType,
                                      HMODULE               Software,
                                      UINT                  Flags,
  _In_opt_                      const D3D_FEATURE_LEVEL    *pFeatureLevels,
                                      UINT                  FeatureLevels,
                                      UINT                  SDKVersion,
  _Out_opt_                           ID3D11Device        **ppDevice,
  _Out_opt_                           D3D_FEATURE_LEVEL    *pFeatureLevel,
  _Out_opt_                           ID3D11DeviceContext **ppImmediateContext);

__declspec (noinline)
HRESULT
WINAPI
D3D11CreateDeviceAndSwapChain_Detour (IDXGIAdapter          *pAdapter,
                                      D3D_DRIVER_TYPE        DriverType,
                                      HMODULE                Software,
                                      UINT                   Flags,
 _In_reads_opt_ (FeatureLevels) CONST D3D_FEATURE_LEVEL     *pFeatureLevels,
                                      UINT                   FeatureLevels,
                                      UINT                   SDKVersion,
 _In_opt_                       CONST DXGI_SWAP_CHAIN_DESC  *pSwapChainDesc,
 _Out_opt_                            IDXGISwapChain       **ppSwapChain,
 _Out_opt_                            ID3D11Device         **ppDevice,
 _Out_opt_                            D3D_FEATURE_LEVEL     *pFeatureLevel,
 _Out_opt_                            ID3D11DeviceContext  **ppImmediateContext);


#pragma data_seg (".SK_D3D11_Hooks")
extern "C"
{
  // Global DLL's cache
__declspec (dllexport) SK_HookCacheEntryGlobal (D3D11CreateDevice)
__declspec (dllexport) SK_HookCacheEntryGlobal (D3D11CreateDeviceAndSwapChain)
};
#pragma data_seg ()
#pragma comment  (linker, "/SECTION:.SK_D3D11_Hooks,RWS")

// Local DLL's cached addresses
SK_HookCacheEntryLocal (D3D11CreateDevice,             L"d3d11.dll", D3D11CreateDevice_Detour,             &D3D11CreateDevice_Import)
SK_HookCacheEntryLocal (D3D11CreateDeviceAndSwapChain, L"d3d11.dll", D3D11CreateDeviceAndSwapChain_Detour, &D3D11CreateDeviceAndSwapChain_Import)

static
std::vector <sk_hook_cache_record_s *> global_d3d11_records =
  { &GlobalHook_D3D11CreateDevice,
    &GlobalHook_D3D11CreateDeviceAndSwapChain };

static
std::vector <sk_hook_cache_record_s *> local_d3d11_records =
  { &LocalHook_D3D11CreateDevice,
    &LocalHook_D3D11CreateDeviceAndSwapChain };



extern "C" __declspec (dllexport) FARPROC D3D11CreateDeviceForD3D12              = nullptr;
extern "C" __declspec (dllexport) FARPROC CreateDirect3D11DeviceFromDXGIDevice   = nullptr;
extern "C" __declspec (dllexport) FARPROC CreateDirect3D11SurfaceFromDXGISurface = nullptr;
extern "C" __declspec (dllexport) FARPROC D3D11On12CreateDevice                  = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTCloseAdapter                     = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTDestroyAllocation                = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTDestroyContext                   = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTDestroyDevice                    = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTDestroySynchronizationObject     = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTQueryAdapterInfo                 = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTSetDisplayPrivateDriverFormat    = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTSignalSynchronizationObject      = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTUnlock                           = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTWaitForSynchronizationObject     = nullptr;
extern "C" __declspec (dllexport) FARPROC EnableFeatureLevelUpgrade              = nullptr;
extern "C" __declspec (dllexport) FARPROC OpenAdapter10                          = nullptr;
extern "C" __declspec (dllexport) FARPROC OpenAdapter10_2                        = nullptr;
extern "C" __declspec (dllexport) FARPROC D3D11CoreCreateLayeredDevice           = nullptr;
extern "C" __declspec (dllexport) FARPROC D3D11CoreGetLayeredDeviceSize          = nullptr;
extern "C" __declspec (dllexport) FARPROC D3D11CoreRegisterLayers                = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTCreateAllocation                 = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTCreateContext                    = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTCreateDevice                     = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTCreateSynchronizationObject      = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTEscape                           = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTGetContextSchedulingPriority     = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTGetDeviceState                   = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTGetDisplayModeList               = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTGetMultisampleMethodList         = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTGetRuntimeData                   = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTGetSharedPrimaryHandle           = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTLock                             = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTOpenAdapterFromHdc               = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTOpenResource                     = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTPresent                          = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTQueryAllocationResidency         = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTQueryResourceInfo                = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTRender                           = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTSetAllocationPriority            = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTSetContextSchedulingPriority     = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTSetDisplayMode                   = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTSetGammaRamp                     = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTSetVidPnSourceOwner              = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTWaitForVerticalBlankEvent        = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DPerformance_BeginEvent              = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DPerformance_EndEvent                = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DPerformance_GetStatus               = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DPerformance_SetMarker               = nullptr;




volatile LONG __SKX_ComputeAntiStall = 1;


INT
__stdcall
SK_D3D11_BytesPerPixel  (DXGI_FORMAT fmt);

BOOL
SK_D3D11_IsFormatBC6Or7 (DXGI_FORMAT fmt)
{
  return ( fmt >= DXGI_FORMAT_BC6H_TYPELESS &&
           fmt <= DXGI_FORMAT_BC7_UNORM_SRGB   );
}

BOOL
__stdcall
SK_D3D11_IsFormatCompressed (DXGI_FORMAT fmt)
{
  if ( (fmt >= DXGI_FORMAT_BC1_TYPELESS  &&
        fmt <= DXGI_FORMAT_BC5_SNORM)    ||
       (fmt >= DXGI_FORMAT_BC6H_TYPELESS &&
        fmt <= DXGI_FORMAT_BC7_UNORM_SRGB) )
  {
    return true;
  }

  return false;
}

bool SK_D3D11_EnableTracking     = false;
bool SK_D3D11_EnableMMIOTracking = false;

__inline
bool
SK_D3D11_ShouldTrackRenderOp ( ID3D11DeviceContext* pDevCtx,
                               SK_TLS**             ppTLS = nullptr )
{
  if (! SK_D3D11_EnableTracking)
    return false;

  if ((! config.render.dxgi.deferred_isolation) && pDevCtx->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED)
    return false;

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if ( rb.d3d11.immediate_ctx == nullptr ||
       rb.device              == nullptr ||
       rb.swapchain           == nullptr )
  {
    return false;
  }

  SK_TLS *pTLS = nullptr;

  if (ppTLS != nullptr)
  {
    if (*ppTLS != nullptr)
    {
      pTLS = *ppTLS;
    }

    else
    {
       pTLS  = SK_TLS_Bottom ();
      *ppTLS = pTLS;
    }
  }

  if (pTLS != nullptr && pTLS->imgui.drawing)
    return false;

  return true;
}

// ID3D11DeviceContext* private data used for indexing various per-ctx lookuos
const GUID SKID_D3D11DeviceContextHandle =
// {5C5298CA-0F9C-5932-A19D-A2E69792AE03}
  { 0x5c5298ca, 0xf9c,  0x5932, { 0xa1, 0x9d, 0xa2, 0xe6, 0x97, 0x92, 0xae, 0x3 } };

volatile LONG SK_D3D11_NumberOfSeenContexts = 0;

enum class sk_shader_class {
  Unknown  = 0x00,
  Vertex   = 0x01,
  Pixel    = 0x02,
  Geometry = 0x04,
  Hull     = 0x08,
  Domain   = 0x10,
  Compute  = 0x20
};

void
SK_D3D11_MergeCommandLists ( ID3D11DeviceContext *pSurrogate,
                             ID3D11DeviceContext *pMerge )
{
  auto _GetRegistry =
  [&]( SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>** ppShaderDomain ,
       sk_shader_class                                    type )
  {
    switch (type)
    {
      default:
      case sk_shader_class::Vertex:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.vertex
                          );
         break;
      case sk_shader_class::Pixel:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.pixel
                          );
         break;
      case sk_shader_class::Geometry:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.geometry
                          );
         break;
      case sk_shader_class::Domain:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.domain
                          );
         break;
      case sk_shader_class::Hull:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.hull
                          );
         break;
      case sk_shader_class::Compute:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.compute
                          );
         break;
    }

    return *ppShaderDomain;
  };

  SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* pShaderRepoIn  = nullptr;
  SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* pShaderRepoOut = nullptr;

  UINT dev_ctx_in  =
    SK_D3D11_GetDeviceContextHandle (pSurrogate),
       dev_ctx_out =
    SK_D3D11_GetDeviceContextHandle (pMerge);

  _GetRegistry ( &pShaderRepoIn,  sk_shader_class::Vertex   );
  _GetRegistry ( &pShaderRepoOut, sk_shader_class::Vertex   )->current.shader [dev_ctx_out] = 
                                                pShaderRepoIn->current.shader [dev_ctx_in];
                                                pShaderRepoIn->current.shader [dev_ctx_in]  = 0x0;
    memcpy     (  pShaderRepoIn->current.views      [dev_ctx_in].data  (),
                  pShaderRepoOut->current.views     [dev_ctx_out].data (),
                    128 * sizeof (ptrdiff_t) );
                  pShaderRepoIn->tracked.deactivate (pSurrogate);
    ZeroMemory (  pShaderRepoIn->current.views      [dev_ctx_in].data  (),
                    128 * sizeof (ptrdiff_t) );

  _GetRegistry ( &pShaderRepoIn,  sk_shader_class::Pixel    );
  _GetRegistry ( &pShaderRepoOut, sk_shader_class::Pixel    )->current.shader [dev_ctx_out] = 
                                                pShaderRepoIn->current.shader [dev_ctx_in];
                                                pShaderRepoIn->current.shader [dev_ctx_in]  = 0x0;
    memcpy     (  pShaderRepoIn->current.views      [dev_ctx_in].data  (),
                  pShaderRepoOut->current.views     [dev_ctx_out].data (),
                    128 * sizeof (ptrdiff_t) );
                  pShaderRepoIn->tracked.deactivate (pSurrogate);
    ZeroMemory (  pShaderRepoIn->current.views      [dev_ctx_in].data  (),
                    128 * sizeof (ptrdiff_t) );

  _GetRegistry ( &pShaderRepoIn,  sk_shader_class::Geometry   );
  _GetRegistry ( &pShaderRepoOut, sk_shader_class::Geometry   )->current.shader [dev_ctx_out] = 
                                                  pShaderRepoIn->current.shader [dev_ctx_in];
                                                  pShaderRepoIn->current.shader [dev_ctx_in]  = 0x0;
    memcpy     (  pShaderRepoIn->current.views      [dev_ctx_in].data  (),
                  pShaderRepoOut->current.views     [dev_ctx_out].data (),
                    128 * sizeof (ptrdiff_t) );
                  pShaderRepoIn->tracked.deactivate (pSurrogate);
    ZeroMemory (  pShaderRepoIn->current.views      [dev_ctx_in].data  () ,
                    128 * sizeof (ptrdiff_t) );


  _GetRegistry ( &pShaderRepoIn,  sk_shader_class::Hull   );
  _GetRegistry ( &pShaderRepoOut, sk_shader_class::Hull   )->current.shader [dev_ctx_out] = 
                                               pShaderRepoIn->current.shader [dev_ctx_in];
                                               pShaderRepoIn->current.shader [dev_ctx_in] = 0x0;
    memcpy     (  pShaderRepoIn->current.views      [dev_ctx_in].data  (),
                  pShaderRepoOut->current.views     [dev_ctx_out].data (),
                    128 * sizeof (ptrdiff_t) );
                  pShaderRepoIn->tracked.deactivate (pSurrogate);
    ZeroMemory (  pShaderRepoIn->current.views      [dev_ctx_in].data  (),
                    128 * sizeof (ptrdiff_t) );


  _GetRegistry ( &pShaderRepoIn,  sk_shader_class::Domain   );
  _GetRegistry ( &pShaderRepoOut, sk_shader_class::Domain   )->current.shader [dev_ctx_out] = 
                                                pShaderRepoIn->current.shader [dev_ctx_in];
                                                pShaderRepoIn->current.shader [dev_ctx_in] = 0x0;
    memcpy     (  pShaderRepoIn->current.views      [dev_ctx_in].data  (),
                  pShaderRepoOut->current.views     [dev_ctx_out].data (),
                    128 * sizeof (ptrdiff_t) );
                  pShaderRepoIn->tracked.deactivate (pSurrogate);
    ZeroMemory (  pShaderRepoIn->current.views      [dev_ctx_in].data  (),
                    128 * sizeof (ptrdiff_t) );

  _GetRegistry ( &pShaderRepoIn,  sk_shader_class::Compute   );
  _GetRegistry ( &pShaderRepoOut, sk_shader_class::Compute   )->current.shader [dev_ctx_out] = 
                                                 pShaderRepoIn->current.shader [dev_ctx_in];
                                                 pShaderRepoIn->current.shader [dev_ctx_in]  = 0x0;
    memcpy     (  pShaderRepoIn->current.views      [dev_ctx_in].data  (),
                  pShaderRepoOut->current.views     [dev_ctx_out].data (),
                    128 * sizeof (ptrdiff_t) );
                  pShaderRepoIn->tracked.deactivate (pSurrogate);
    ZeroMemory (  pShaderRepoIn->current.views      [dev_ctx_in].data  (),
                    128 * sizeof (ptrdiff_t) );
}

void
SK_D3D11_ResetContextState (ID3D11DeviceContext* pDevCtx)
{
  auto _GetRegistry =
  [&]( SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>** ppShaderDomain ,
       sk_shader_class                                    type )
  {
    switch (type)
    {
      default:
      case sk_shader_class::Vertex:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.vertex
                          );
         break;
      case sk_shader_class::Pixel:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.pixel
                          );
         break;
      case sk_shader_class::Geometry:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.geometry
                          );
         break;
      case sk_shader_class::Domain:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.domain
                          );
         break;
      case sk_shader_class::Hull:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.hull
                          );
         break;
      case sk_shader_class::Compute:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.compute
                          );
         break;
    }

    return *ppShaderDomain;
  };

  SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* pShaderRepo = nullptr;

  UINT dev_ctx =
    SK_D3D11_GetDeviceContextHandle (pDevCtx);

  _GetRegistry ( &pShaderRepo, sk_shader_class::Vertex   )->current.shader [dev_ctx] = 0x0;
                  pShaderRepo->tracked.deactivate (pDevCtx);
    ZeroMemory (  pShaderRepo->current.views      [dev_ctx].data (), 128 * sizeof (ptrdiff_t) );
  _GetRegistry ( &pShaderRepo, sk_shader_class::Pixel    )->current.shader [dev_ctx] = 0x0;
                  pShaderRepo->tracked.deactivate (pDevCtx);
    ZeroMemory (  pShaderRepo->current.views      [dev_ctx].data (), 128 * sizeof (ptrdiff_t) );
  _GetRegistry ( &pShaderRepo, sk_shader_class::Geometry )->current.shader [dev_ctx] = 0x0;
                  pShaderRepo->tracked.deactivate (pDevCtx);
    ZeroMemory (  pShaderRepo->current.views      [dev_ctx].data (), 128 * sizeof (ptrdiff_t) );
  _GetRegistry ( &pShaderRepo, sk_shader_class::Domain   )->current.shader [dev_ctx] = 0x0;
                  pShaderRepo->tracked.deactivate (pDevCtx);
    ZeroMemory (  pShaderRepo->current.views      [dev_ctx].data (), 128 * sizeof (ptrdiff_t) );
  _GetRegistry ( &pShaderRepo, sk_shader_class::Hull     )->current.shader [dev_ctx] = 0x0;
                  pShaderRepo->tracked.deactivate (pDevCtx);
    ZeroMemory (  pShaderRepo->current.views      [dev_ctx].data (), 128 * sizeof (ptrdiff_t) );
  _GetRegistry ( &pShaderRepo, sk_shader_class::Compute  )->current.shader [dev_ctx] = 0x0;
                  pShaderRepo->tracked.deactivate (pDevCtx);
    ZeroMemory (  pShaderRepo->current.views      [dev_ctx].data (), 128 * sizeof (ptrdiff_t) );
}

LONG
SK_D3D11_GetDeviceContextHandle ( ID3D11DeviceContext *pDevCtx )
{
  if (pDevCtx == nullptr) return SK_D3D11_MAX_DEV_CONTEXTS;

  // Polymorphic weirdness - thank you IUnknown promoting to anything it wants
  ////CComQIPtr <ID3D11DeviceContext> pTestCtx (pDevCtx);

  if (pDevCtx == nullptr) return SK_D3D11_MAX_DEV_CONTEXTS;


  UINT size   = sizeof (LONG);
  LONG handle = 0;

  if ( SUCCEEDED ( pDevCtx->GetPrivateData ( SKID_D3D11DeviceContextHandle, &size,
                                             &handle ) ) )
  {
    return handle;
  }

  std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (cs_shader);

  size   = sizeof (LONG);
  handle = ReadAcquire (&SK_D3D11_NumberOfSeenContexts);

  LONG* new_handle =
    new LONG (handle);

  if ( SUCCEEDED (
         pDevCtx->SetPrivateData ( SKID_D3D11DeviceContextHandle, size, new_handle )
       )
     )
  {
    InterlockedIncrement (&SK_D3D11_NumberOfSeenContexts);

    return *new_handle;
  }

  else delete new_handle;

  assert (handle < SK_D3D11_MAX_DEV_CONTEXTS);

  return handle;
}


size_t
__stdcall
SK_D3D11_ComputeTextureSize (D3D11_TEXTURE2D_DESC* pDesc)
{
  size_t size       = 0;
  bool   compressed =
    SK_D3D11_IsFormatCompressed (pDesc->Format);

  if (! compressed)
  {
    for (UINT i = 0; i < pDesc->MipLevels; i++)
    {
      size += static_cast <size_t> (SK_D3D11_BytesPerPixel (pDesc->Format)) *
              static_cast <size_t> (std::max (1U, pDesc->Width  >> i))      *
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
      size_t stride = (bpp == 0) ?
               static_cast <size_t> (std::max (1UL, (std::max (1U, (pDesc->Width >> i)) + 3UL) / 4UL)) * 8UL :
               static_cast <size_t> (std::max (1UL, (std::max (1U, (pDesc->Width >> i)) + 3UL) / 4UL)) * 16UL;

      size_t lod_size =
        static_cast <size_t> (stride) * ((pDesc->Height >> i) / 4 +
                                         (pDesc->Height >> i) % 4);

      size += lod_size;
    }
  }

  return size;
}


std::wstring
DescribeResource (ID3D11Resource* pRes)
{
  std::wstring desc_out = L"";

  D3D11_RESOURCE_DIMENSION rDim;
  pRes->GetType          (&rDim);

  if (rDim == D3D11_RESOURCE_DIMENSION_BUFFER)
    desc_out += L"Buffer";
  else if (rDim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
  {
    desc_out += L"Tex2D: ";
    CComQIPtr <ID3D11Texture2D> pTex2D (pRes);

    D3D11_TEXTURE2D_DESC desc = {};
    pTex2D->GetDesc (&desc);

    desc_out += SK_FormatStringW (L"(%lux%lu): %s { %s/%s }",
                  desc.Width, desc.Height, SK_DXGI_FormatToStr (desc.Format).c_str (),
                                        SK_D3D11_DescribeUsage (desc.Usage),
                   SK_D3D11_DescribeBindFlags ((D3D11_BIND_FLAG)desc.BindFlags).c_str ());
                  
  }
  else
    desc_out += L"Other";

  return desc_out;
};


HRESULT
__stdcall
SK_D3D11_MipmapCacheTexture2DEx ( const DirectX::ScratchImage&   img,
                                        uint32_t                 crc32c, 
                                        ID3D11Texture2D*         pOutTex,
                                        DirectX::ScratchImage** ppOutImg,
                                        SK_TLS*                  pTLS = SK_TLS_Bottom () );

struct resample_job_s {
  DirectX::ScratchImage *data;
  ID3D11Texture2D       *texture;

  uint32_t               crc32c;

  struct {
    int64_t preprocess; // Time performing various work required for submission
                        //   (i.e. decompression)

    int64_t received;
    int64_t started;
    int64_t finished;
  } time;
};

static       HANDLE hResampleWork       = INVALID_HANDLE_VALUE;
static unsigned int dispatch_thread_idx = 0;

struct resample_dispatch_s
{
  void delayInit (void)
  {
    hResampleWork =
      CreateEventW ( nullptr, FALSE,
                              FALSE,
        SK_FormatStringW ( LR"(Local\SK_DX11TextureResample_Dispatch_pid%x)",
                             GetCurrentProcessId () ).c_str ()
      );

    SYSTEM_INFO        sysinfo = { };
    SK_GetSystemInfo (&sysinfo);

    max_workers = std::max (1UL, sysinfo.dwNumberOfProcessors - 1);

    PROCESSOR_NUMBER                                 pnum = { };
    GetThreadIdealProcessorEx (GetCurrentThread (), &pnum);

    dispatch_thread_idx = pnum.Group + pnum.Number;
  }

  bool postJob (resample_job_s& job)
  {
    job.time.received = SK_QueryPerf ().QuadPart;
    waiting_textures.push (job);

    InterlockedIncrement  (&SK_D3D11_TextureResampler.stats.textures_waiting);

    SK_RunOnce (delayInit ());

    bool ret = false;

    if (InterlockedIncrement (&active_workers) <= max_workers)
    {
      SK_Thread_Create ( [](LPVOID) ->
      DWORD
      {
        SetThreadPriority ( SK_GetCurrentThread (),
                            THREAD_MODE_BACKGROUND_BEGIN );

        static volatile LONG thread_num = 0;

        uintptr_t thread_idx = ReadAcquire (&thread_num);
        uintptr_t logic_idx  = 0;

        // There will be an odd-man-out in any SMT implementation since the
        //   SwapChain thread has an ideal _logical_ CPU core.
        bool         disjoint     = false;


        InterlockedIncrement (&thread_num);


        if (thread_idx == dispatch_thread_idx)
        {
          disjoint = true;
          thread_idx++;
          InterlockedIncrement (&thread_num);
        }


        for ( auto& it : SK_CPU_GetLogicalCorePairs () )
        {
          if ( it & ( (uintptr_t)1 << thread_idx ) ) break;
          else                       ++logic_idx;
        }

        ULONG_PTR logic_mask =
          SK_CPU_GetLogicalCorePairs ()[logic_idx];


        SK_TLS *pTLS = SK_TLS_Bottom ();

        SetThreadIdealProcessor ( SK_GetCurrentThread (), (DWORD)thread_idx );
        if (! disjoint)
          SetThreadAffinityMask ( SK_GetCurrentThread (),        logic_mask );

        std::wstring desc = L"[SK] D3D11 Texture Resampling ";


        auto CountSetBits = [](ULONG_PTR bitMask) -> DWORD
        {
          DWORD     LSHIFT      = sizeof (ULONG_PTR) * 8 - 1;
          DWORD     bitSetCount = 0;
          ULONG_PTR bitTest     = (ULONG_PTR)1 << LSHIFT;
          DWORD     i;
        
          for (i = 0; i <= LSHIFT; ++i)
          {
            bitSetCount += ((bitMask & bitTest) ? 1 : 0);
            bitTest     /= 2;
          }
        
          return bitSetCount;
        };

        int logical_siblings = CountSetBits (logic_mask);
        if (logical_siblings > 1 && (! disjoint))
        {
          desc += L"HyperThread < ";
          for ( int i = 0,
                    j = 0;
                    i < SK_GetBitness ();
                  ++i )
          {
            if ( (logic_mask & ( (uintptr_t)1 << i ) ) )
            {
              desc += std::to_wstring (i);

              if ( ++j < logical_siblings ) desc += L",";
            }
          }
          desc += L" >";
        }

        else
          desc += SK_FormatStringW (L"Thread %lu", thread_idx);


        SetCurrentThreadDescription (desc.c_str ());

        pTLS->texture_management.injection_thread = true;
        pTLS->imgui.drawing                       = true;

        SetThreadPriorityBoost ( SK_GetCurrentThread (),
                                 TRUE );
        SetThreadPriority (      SK_GetCurrentThread (),
                                 THREAD_PRIORITY_TIME_CRITICAL );

        do
        {
          resample_job_s job;

          while (SK_D3D11_TextureResampler.waiting_textures.try_pop (job))
          {
            job.time.started = SK_QueryPerf ().QuadPart;

            InterlockedDecrement (&SK_D3D11_TextureResampler.stats.textures_waiting);
            InterlockedIncrement (&SK_D3D11_TextureResampler.stats.textures_resampling);

            DirectX::ScratchImage* pNewImg = nullptr;

            HRESULT hr =
              SK_D3D11_MipmapCacheTexture2DEx ( *job.data,
                                                 job.crc32c,
                                                 job.texture,
                                                &pNewImg,
                                                 pTLS );

            delete job.data;

            if (FAILED (hr))
            {
              dll_log.Log (L"Resampler failure (crc32c=%x)", job.crc32c);
              InterlockedIncrement (&SK_D3D11_TextureResampler.stats.error_count);
            }

            else
            {
              job.time.finished = SK_QueryPerf ().QuadPart;
              job.data          = pNewImg;

              SK_D3D11_TextureResampler.finished_textures.push (job);
            }

            InterlockedDecrement (&SK_D3D11_TextureResampler.stats.textures_resampling);
          }

          if (WaitForSingleObject (hResampleWork, 666UL) != WAIT_OBJECT_0)
          {
            SK_Thread_ScopedPriority
              hold_your_horses    (THREAD_PRIORITY_IDLE);
            WaitForSingleObjectEx (hResampleWork, INFINITE, FALSE);
          }
        } while (! ReadAcquire (&__SK_DLL_Ending));

        InterlockedDecrement (&SK_D3D11_TextureResampler.active_workers);

        SK_Thread_CloseSelf ();

        return 0;
      } );

      ret = true;
    }

    SetEvent (hResampleWork);

    return ret;
  };


  bool processFinished ( ID3D11Device        *pDev,
                         ID3D11DeviceContext *pDevCtx,
                         SK_TLS              *pTLS = SK_TLS_Bottom () )
  {
    size_t MAX_TEXTURE_UPLOADS_PER_FRAME;
    int    MAX_UPLOAD_TIME_PER_FRAME_IN_MS;

    extern int SK_D3D11_TexStreamPriority;
    switch    (SK_D3D11_TexStreamPriority)
    {
      case 0:
        MAX_UPLOAD_TIME_PER_FRAME_IN_MS = 3UL;
        MAX_TEXTURE_UPLOADS_PER_FRAME   = ReadAcquire (&SK_D3D11_TextureResampler.stats.textures_waiting) / 7 + 1;
        break;

      case 1:
      default:
        MAX_UPLOAD_TIME_PER_FRAME_IN_MS = 13UL;
        MAX_TEXTURE_UPLOADS_PER_FRAME   = ReadAcquire (&SK_D3D11_TextureResampler.stats.textures_waiting) / 4 + 1;
        break;

      case 2:
        MAX_UPLOAD_TIME_PER_FRAME_IN_MS = 27UL;
        MAX_TEXTURE_UPLOADS_PER_FRAME   = ReadAcquire (&SK_D3D11_TextureResampler.stats.textures_waiting) / 2 + 1;
    }


    static const uint64_t _TicksPerMsec =
      ( SK_GetPerfFreq ().QuadPart / 1000ULL );


    size_t   uploaded   = 0;
    uint64_t start_tick = SK::ControlPanel::current_tick;//SK_QueryPerf ().QuadPart;
    uint64_t deadline   = start_tick + MAX_UPLOAD_TIME_PER_FRAME_IN_MS * _TicksPerMsec;

    SK_ScopedBool auto_bool_tex (&pTLS->texture_management.injection_thread);
    SK_ScopedBool auto_bool_mem (&pTLS->imgui.drawing);

    pTLS->texture_management.injection_thread = true;
    pTLS->imgui.drawing                       = true;

    bool processed = false;

    //
    // Finish Resampled Texture Uploads (discard if texture is no longer live)
    //
    while (! finished_textures.empty ())
    {
      resample_job_s finished = { };

      if ( pDev    != nullptr &&
           pDevCtx != nullptr    )
      {
        if (finished_textures.try_pop (finished))
        {
          // Due to cache purging behavior and the fact that some crazy games issue back-to-back loads of the same resource,
          //   we need to test the cache health prior to trying to service this request.
          if ( ( finished.time.started > SK_D3D11_Textures.LastPurge_2D ) || ( SK_D3D11_TextureIsCached (finished.texture) )

            && finished.data         != nullptr
            && finished.texture      != nullptr )
          {
            CComPtr <ID3D11Texture2D> pTempTex = nullptr;

            HRESULT ret =
              DirectX::CreateTexture (pDev, finished.data->GetImages   (), finished.data->GetImageCount (),
                                            finished.data->GetMetadata (), (ID3D11Resource **)&pTempTex);

            if (SUCCEEDED (ret))
            {
              pDevCtx->CopyResource (finished.texture, pTempTex);

              uploaded++;

              InterlockedIncrement (&stats.textures_finished);

              // Various wait-time statistics;  the job queue is HyperThreading aware and helps reduce contention on
              //                                  the list of finished textures ( Which we need to service from the
              //                                                                   game's original calling thread ).
              uint64_t wait_in_queue = ( finished.time.started      - finished.time.received );
              uint64_t work_time     = ( finished.time.finished     - finished.time.started  );
              uint64_t wait_finished = ( SK_CurrentPerf ().QuadPart - finished.time.finished );

              SK_LOG1 ( (L"ReSample Job %4lu (hash=%08x {%4lux%#4lu}) => %9.3f ms TOTAL :: ( %9.3f ms pre-proc, "
                                                                                           L"%9.3f ms work queue, "
                                                                                           L"%9.3f ms resampling, "
                                                                                           L"%9.3f ms finish queue )",
                           ReadAcquire (&stats.textures_finished), finished.crc32c,
                           finished.data->GetMetadata ().width,    finished.data->GetMetadata ().height,
                         ( (long double)SK_CurrentPerf ().QuadPart - (long double)finished.time.received +
                           (long double)finished.time.preprocess ) / (long double)_TicksPerMsec,
                           (long double)finished.time.preprocess   / (long double)_TicksPerMsec,
                           (long double)wait_in_queue              / (long double)_TicksPerMsec,
                           (long double)work_time                  / (long double)_TicksPerMsec,
                           (long double)wait_finished              / (long double)_TicksPerMsec ),
                       L"DX11TexMgr"  );
            }

            else
              InterlockedIncrement (&stats.error_count);
          }

          else
          {
            SK_LOG0 ( (L"Texture (%x) was loaded too late, discarding...", finished.crc32c),
                       L"DX11TexMgr" );

            InterlockedIncrement (&stats.textures_too_late);
          }
          

          if (finished.data != nullptr)
          {
            delete finished.data;
                   finished.data = nullptr;
          }

          processed = true;


          if ( uploaded >= MAX_TEXTURE_UPLOADS_PER_FRAME ||
               deadline <  (uint64_t)SK_QueryPerf ().QuadPart )  break;
        }

        else
          break;
      }

      else
        break;
    }

    return processed;
  };


  struct stats_s {
    ~stats_s (void)
    {
      if (hResampleWork != INVALID_HANDLE_VALUE)
      {
        CloseHandle (hResampleWork);
                     hResampleWork = INVALID_HANDLE_VALUE;
      }
    }

    volatile LONG textures_resampled    = 0L;
    volatile LONG textures_compressed   = 0L;
    volatile LONG textures_decompressed = 0L;

    volatile LONG textures_waiting      = 0L;
    volatile LONG textures_resampling   = 0L;
    volatile LONG textures_too_late     = 0L;
    volatile LONG textures_finished     = 0L;

    volatile LONG error_count           = 0L;
  } stats;

           LONG max_workers    = 0;
  volatile LONG active_workers = 0;

  concurrency::concurrent_queue <resample_job_s> waiting_textures;
  concurrency::concurrent_queue <resample_job_s> finished_textures;
} SK_D3D11_TextureResampler;


LONG
SK_D3D11_Resampler_GetActiveJobCount (void)
{
  return ReadAcquire (&SK_D3D11_TextureResampler.stats.textures_resampling);
}

LONG
SK_D3D11_Resampler_GetWaitingJobCount (void)
{
  return ReadAcquire (&SK_D3D11_TextureResampler.stats.textures_waiting);
}

LONG
SK_D3D11_Resampler_GetRetiredCount (void)
{
  return ReadAcquire (&SK_D3D11_TextureResampler.stats.textures_resampled);
}

LONG
SK_D3D11_Resampler_GetErrorCount (void)
{
  return ReadAcquire (&SK_D3D11_TextureResampler.stats.error_count);
}

volatile LONG  __d3d11_ready    = FALSE;

void WaitForInitD3D11 (void)
{
  if (CreateDXGIFactory_Import == nullptr)
  {
    SK_RunOnce (SK_BootDXGI ());
  }

  if (SK_TLS_Bottom ()->d3d11.ctx_init_thread)
    return;

  if (! ReadAcquire (&__d3d11_ready))
    SK_Thread_SpinUntilFlagged (&__d3d11_ready);
}


template <class _T>
static
__forceinline
UINT
calc_count (_T** arr, UINT max_count)
{
  for ( int i = (int)max_count - 1 ;
            i >= 0 ;
          --i )
  {
    if (arr [i] != 0)
      return i;
  }

  return max_count;
}


__declspec (noinline)
uint32_t
__cdecl
crc32_tex (  _In_      const D3D11_TEXTURE2D_DESC   *__restrict pDesc,
             _In_      const D3D11_SUBRESOURCE_DATA *__restrict pInitialData,
             _Out_opt_       size_t                 *__restrict pSize,
             _Out_opt_       uint32_t               *__restrict pLOD0_CRC32 );



using SK_ReShade_PresentCallback_pfn              = bool (__stdcall *)(void *user);
using SK_ReShade_OnCopyResourceD3D11_pfn          = void (__stdcall *)(void *user, ID3D11Resource         *&dest,    ID3D11Resource *&source);
using SK_ReShade_OnClearDepthStencilViewD3D11_pfn = void (__stdcall *)(void *user, ID3D11DepthStencilView *&depthstencil);
using SK_ReShade_OnGetDepthStencilViewD3D11_pfn   = void (__stdcall *)(void *user, ID3D11DepthStencilView *&depthstencil);
using SK_ReShade_OnSetDepthStencilViewD3D11_pfn   = void (__stdcall *)(void *user, ID3D11DepthStencilView *&depthstencil);
using SK_ReShade_OnDrawD3D11_pfn                  = void (__stdcall *)(void *user, ID3D11DeviceContext     *context, unsigned int   vertices);

struct reshade_coeffs_s {
  int indexed                    = 1;
  int draw                       = 1;
  int auto_draw                  = 0;
  int indexed_instanced          = 1;
  int indexed_instanced_indirect = 4096;
  int instanced                  = 1;
  int instanced_indirect         = 4096;
  int dispatch                   = 1;
  int dispatch_indirect          = 1;
} SK_D3D11_ReshadeDrawFactors;

struct
{
  SK_ReShade_OnDrawD3D11_pfn fn   = nullptr;
  void*                      data = nullptr;
  void call (ID3D11DeviceContext *context, unsigned int vertices, SK_TLS* pTLS = SK_TLS_Bottom ())
    { if (data != nullptr && fn != nullptr && (! pTLS->imgui.drawing)) fn (data, context, vertices); }
} SK_ReShade_DrawCallback;

struct
{
  SK_ReShade_OnSetDepthStencilViewD3D11_pfn fn   = nullptr;
  void*                                     data = nullptr;

  void try_call (ID3D11DepthStencilView *&depthstencil)
  {
    if ((uintptr_t)data + (uintptr_t)fn != 0)
      return call (depthstencil);
  }

  void call (ID3D11DepthStencilView *&depthstencil, SK_TLS* pTLS = SK_TLS_Bottom ())
    { if (data != nullptr && fn != nullptr && (! pTLS->imgui.drawing)) fn (data, depthstencil); }
} SK_ReShade_SetDepthStencilViewCallback;

struct
{
  SK_ReShade_OnGetDepthStencilViewD3D11_pfn fn   = nullptr;
  void*                                     data = nullptr;
  
  void try_call (ID3D11DepthStencilView *&depthstencil)
  {
    if ((uintptr_t)data + (uintptr_t)fn != 0)
      return call (depthstencil);
  }

  void call (ID3D11DepthStencilView *&depthstencil, SK_TLS* pTLS = SK_TLS_Bottom ())
    { if (data != nullptr && fn != nullptr && (!pTLS->imgui.drawing)) fn (data, depthstencil); }
} SK_ReShade_GetDepthStencilViewCallback;

struct
{
  SK_ReShade_OnClearDepthStencilViewD3D11_pfn fn   = nullptr;
  void*                                       data = nullptr;

  void try_call (ID3D11DepthStencilView *&depthstencil)
  {
    if ((uintptr_t)data + (uintptr_t)fn != 0)
      return call (depthstencil);
  }

  void call (ID3D11DepthStencilView *&depthstencil, SK_TLS* pTLS = SK_TLS_Bottom ())
    { if (data != nullptr && fn != nullptr && (! pTLS->imgui.drawing)) fn (data, depthstencil); }
} SK_ReShade_ClearDepthStencilViewCallback;

struct
{
  SK_ReShade_OnCopyResourceD3D11_pfn fn   = nullptr;
  void*                              data = nullptr;
  void call (ID3D11Resource *&dest, ID3D11Resource *&source, SK_TLS* pTLS = SK_TLS_Bottom ())
    { if (data != nullptr && fn != nullptr && (! pTLS->imgui.drawing)) fn (data, dest, source); }
} SK_ReShade_CopyResourceCallback;



#ifndef _SK_WITHOUT_D3DX11
D3DX11CreateTextureFromMemory_pfn D3DX11CreateTextureFromMemory = nullptr;
D3DX11CreateTextureFromFileW_pfn  D3DX11CreateTextureFromFileW  = nullptr;
D3DX11GetImageInfoFromFileW_pfn   D3DX11GetImageInfoFromFileW   = nullptr;
HMODULE                           hModD3DX11_43                 = nullptr;
#endif

bool SK_D3D11_IsTexInjectThread (SK_TLS *pTLS = SK_TLS_Bottom ())
{
  return pTLS->texture_management.injection_thread;
}

void
SK_D3D11_ClearTexInjectThread (SK_TLS *pTLS = SK_TLS_Bottom ())
{
  pTLS->texture_management.injection_thread = false;
}

void
SK_D3D11_SetTexInjectThread (SK_TLS* pTLS = SK_TLS_Bottom ())
{
  pTLS->texture_management.injection_thread = true;
}

#include <atlbase.h>
struct IWrapD3D11Texture2D : ID3D11Texture2D1
{
  IWrapD3D11Texture2D ( ID3D11Device    *pDevice,
                        ID3D11Texture2D *pTexture2D ) :
    pReal (pTexture2D),
    pDev  (pDevice),
    ver_  (0)
  {
                                  pTexture2D->AddRef  (),
    InterlockedExchange  (&refs_, pTexture2D->Release ());

    //// Immediately try to upgrade
    CComQIPtr <ID3D11Texture2D1> pTex1 (this);
  }

    IWrapD3D11Texture2D ( ID3D11Device   *pDevice,
                        ID3D11Texture2D1 *pTexture2D1 ) :
    pReal (pTexture2D1),
    pDev  (pDevice),
    ver_  (1)
  {
                                  pTexture2D1->AddRef  (),
    InterlockedExchange  (&refs_, pTexture2D1->Release ());
  }

  IWrapD3D11Texture2D            (const IWrapD3D11Texture2D &) = delete;
  IWrapD3D11Texture2D &operator= (const IWrapD3D11Texture2D &) = delete;

#pragma region IUnknown
  virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, void **ppvObj) override;                        // 0
  virtual ULONG   STDMETHODCALLTYPE AddRef         (void)                       override;                        // 1
  virtual ULONG   STDMETHODCALLTYPE Release        (void)                       override;                        // 2
#pragma endregion

#pragma region ID3D11DeviceChild
  virtual void    STDMETHODCALLTYPE GetDevice               ( ID3D11Device **ppDevice) override;                 // 3
  virtual HRESULT STDMETHODCALLTYPE GetPrivateData          ( REFGUID       guid,
                                                              UINT         *pDataSize,
                                                              void         *pData)     override;                 // 4
  virtual HRESULT STDMETHODCALLTYPE SetPrivateData          ( REFGUID       guid,
                                                              UINT          DataSize,
                                                        const void         *pData)     override;                 // 5
  virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface ( REFGUID       guid,
                                                        const IUnknown     *pData)     override;                 // 6
#pragma endregion

#pragma region ID3D11Resource
  virtual void    STDMETHODCALLTYPE GetType             (D3D11_RESOURCE_DIMENSION *pResourceDimension) override; // 7
  virtual void    STDMETHODCALLTYPE SetEvictionPriority (UINT                      EvictionPriority)   override; // 8
  virtual UINT    STDMETHODCALLTYPE GetEvictionPriority (void)                                         override; // 9
#pragma endregion

#pragma region ID3D11Texture2D
  virtual void    STDMETHODCALLTYPE GetDesc (D3D11_TEXTURE2D_DESC   *pDesc)  override;                           // 10
#pragma endregion

#pragma region ID3D11Texture2D1
  virtual void    STDMETHODCALLTYPE GetDesc1 (D3D11_TEXTURE2D_DESC1 *pDesc1) override;                           // 11
#pragma endregion

  volatile LONG    refs_ = 1;
  ID3D11Texture2D *pReal;
  ID3D11Device    *pDev;
  unsigned int     ver_;
};

// ID3D11Texture2D
HRESULT STDMETHODCALLTYPE
IWrapD3D11Texture2D::QueryInterface (REFIID riid, void **ppvObj)
{
  if (ppvObj == nullptr)
  {
    return E_POINTER;
  }

  else if (
  //riid == __uuidof (this)                 ||
    riid == __uuidof (IUnknown)             ||
    riid == __uuidof (ID3D11DeviceChild)    ||
    riid == __uuidof (ID3D11Resource)       ||
    riid == __uuidof (ID3D11Texture2D)      ||
    riid == __uuidof (ID3D11Texture2D1) )
  {
    #pragma region Update to ID3D11Texture2D1 interface
    if (riid == __uuidof (ID3D11Texture2D1) && ver_ < 1)
    {
      ID3D11Texture2D1 *texture1 = nullptr;

      if (FAILED (pReal->QueryInterface (&texture1)))
      {
        return E_NOINTERFACE;
      }

      pReal->Release ();

      pReal = texture1;
      ver_ = 1;
    }
    #pragma endregion

    pReal->AddRef ();
    InterlockedExchange (&refs_, pReal->Release ());
    AddRef ();

    *ppvObj = this;

    return S_OK;
  }

  return pReal->QueryInterface (riid, ppvObj);
}

ULONG
STDMETHODCALLTYPE
IWrapD3D11Texture2D::AddRef (void)
{
  InterlockedIncrement (&refs_);

  if (SK_D3D11_TextureIsCached (this))
    SK_D3D11_UseTexture (this);

  return pReal->AddRef ();
}

ULONG
STDMETHODCALLTYPE
IWrapD3D11Texture2D::Release (void)
{
    if (InterlockedDecrement (&refs_) == 0)
    {
      //assert(_runtime != nullptr);
    }

	  ULONG refs = pReal->Release ();

    if (ReadAcquire (&refs_) == 0 && refs != 0)
    {
      //SK_LOG0 ( (L"Reference count for 'IDXGISwapChain" << (ver_ > 0 ? std::to_string(ver_) : "") << "' object " << this << " is inconsistent: " << ref << ", but expected 0.";

      refs = 0;
    }

    if (refs == 0)
    {
      assert (ReadAcquire (&refs_) <= 0);

      if (ReadAcquire (&refs_) == 0)
      {
        SK_D3D11_RemoveTexFromCache (this);

        //if (ver_ > 0)
        //  InterlockedDecrement (&SK_DXGI_LiveWrappedSwapChain1s);
        //else
        //  InterlockedDecrement (&SK_DXGI_LiveWrappedSwapChains);

        delete this;
      }
    }

    return refs;
}

HRESULT STDMETHODCALLTYPE IWrapD3D11Texture2D::SetPrivateData(REFGUID Name, UINT DataSize, const void *pData)
{
	return pReal->SetPrivateData(Name, DataSize, pData);
}
HRESULT STDMETHODCALLTYPE IWrapD3D11Texture2D::SetPrivateDataInterface(REFGUID Name, const IUnknown *pUnknown)
{
	return pReal->SetPrivateDataInterface(Name, pUnknown);
}
void    STDMETHODCALLTYPE IWrapD3D11Texture2D::GetDevice     (ID3D11Device **ppDevice)
{
  return pReal->GetDevice(ppDevice);
}
HRESULT STDMETHODCALLTYPE IWrapD3D11Texture2D::GetPrivateData(REFGUID Name, UINT *pDataSize, void *pData)
{
	return pReal->GetPrivateData(Name, pDataSize, pData);
}

void     STDMETHODCALLTYPE IWrapD3D11Texture2D::GetType             (D3D11_RESOURCE_DIMENSION *pResourceDimension)
{
  return pReal->GetType(pResourceDimension);
}
void    STDMETHODCALLTYPE IWrapD3D11Texture2D::SetEvictionPriority (UINT                      EvictionPriority)
{
  return pReal->SetEvictionPriority(EvictionPriority);
}
UINT    STDMETHODCALLTYPE IWrapD3D11Texture2D::GetEvictionPriority (void)
{
  return pReal->GetEvictionPriority();
}

void    STDMETHODCALLTYPE IWrapD3D11Texture2D::GetDesc (D3D11_TEXTURE2D_DESC *pDesc)
{
  return pReal->GetDesc(pDesc);
}

void    STDMETHODCALLTYPE IWrapD3D11Texture2D::GetDesc1 (D3D11_TEXTURE2D_DESC1 *pDesc1)
{
  assert(ver_ >= 1);

  return static_cast <ID3D11Texture2D1 *>(pReal)->GetDesc1(pDesc1);
}

uint32_t
__stdcall
SK_D3D11_TextureHashFromCache (ID3D11Texture2D* pTex);

using IUnknown_Release_pfn =
  ULONG (WINAPI *)(IUnknown* This);
using IUnknown_AddRef_pfn  =
  ULONG (WINAPI *)(IUnknown* This);

IUnknown_Release_pfn IUnknown_Release_Original = nullptr;
IUnknown_AddRef_pfn  IUnknown_AddRef_Original  = nullptr;

static volatile LONG __SK_D3D11_TexRefCount_Failures = 0;
//
// If reference counting is broken by some weird COM wrapper
//   misbehaving, then for the love of ... don't cache textures
//     using SK *==> it hooks rather than wraps.
//
BOOL
SK_D3D11_TestRefCountHooks (ID3D11Texture2D* pInputTex, SK_TLS* pTLS = nullptr)
{
  // This is unsafe, but ... the variable name already told you that!
  if (config.textures.cache.allow_unsafe_refs)
    return TRUE;

  if (pTLS == nullptr) pTLS = SK_TLS_Bottom ();

  auto SanityFail =
  [&](void) ->
  BOOL
  {
    InterlockedIncrement (&__SK_D3D11_TexRefCount_Failures);
    return FALSE;
  };

  pTLS->texture_management.refcount_obj = pInputTex;

  LONG initial = pTLS->texture_management.refcount_test;

  pInputTex->AddRef ();

  LONG initial_plus_one =
    pTLS->texture_management.refcount_test;

  pInputTex->Release ();

  LONG initial_again =
    pTLS->texture_management.refcount_test;

  pTLS->texture_management.refcount_obj = nullptr;

  if ( initial != initial_plus_one - 1 ||
       initial != initial_again )	
  {
    SK_LOG1 ( (L"Expected %li after AddRef (); got %li.",
                 initial + 1, initial_plus_one ),
               L"DX11TexMgr" );
    SK_LOG1 ( (L"Expected %li after Release (); got %li.",
                 initial, initial_again ),
               L"DX11TexMgr" );

    return SanityFail ();
  }


  // Important note: The D3D11 runtime implements QueryInterface on an
  //                   ID3D11Texture2D by returning a SEPARATE object with
  //                     an initial reference count of 1.
  //
  //     Effectively, QueryInterface creates a view with its own lifetime,
  //       never expect to get the original object by making this call!
  //

            pTLS->texture_management.refcount_test = 0;

  // Also validate that the wrapper's QueryInterface method is
  //   invoking our hooks
  //
  ID3D11Texture2D* pReferenced = nullptr;
  pInputTex->QueryInterface <ID3D11Texture2D> (&pReferenced);

  if (! pReferenced)
    return SanityFail ();

  pTLS->texture_management.refcount_obj = pReferenced;

  initial =
    pTLS->texture_management.refcount_test;

  pReferenced->Release ();

  LONG initial_after_release =
    pTLS->texture_management.refcount_test;
    pTLS->texture_management.refcount_obj = nullptr;

  if ( initial != initial_after_release + 1 )
  {
    SK_LOG1 ( (L"Expected %lu after QueryInterface (...); got %lu.",
                 initial + 1, initial_plus_one ),
               L"DX11TexMgr" );
    SK_LOG1 ( (L"Expected %lu after Release (); got %lu.",
                 initial, initial_again ),
               L"DX11TexMgr" );

    return SanityFail ();
  }


  return TRUE;
}


__declspec (noinline)
ULONG
WINAPI
IUnknown_Release (IUnknown* This)
{
  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (! SK_D3D11_IsTexInjectThread (pTLS))
  {
	  if (This == pTLS->texture_management.refcount_obj)
	    pTLS->texture_management.refcount_test--;

    ID3D11Texture2D* pTex = (ID3D11Texture2D *)This;

    LONG count =
      IUnknown_AddRef_Original  (pTex);
      IUnknown_Release_Original (pTex);

    // If count is == 0, something's screwy
    if (pTex != nullptr && count <= 2)
    {
      if (SK_D3D11_RemoveTexFromCache (pTex))
      {
        if (count < 2)
          SK_LOG0 ( ( L"Unexpected reference count: %li", count ), L"DX11TexMgr" );

        if (count == 3)
        {
          return pTex->Release ();
        }
      }
    }
  }

  return IUnknown_Release_Original (This);
}


__declspec (noinline)
ULONG
WINAPI
IUnknown_AddRef (IUnknown* This)
{
 SK_TLS* pTLS =
    SK_TLS_Bottom ();

  bool refcount_test = ( This == pTLS->texture_management.refcount_obj );

  if (! SK_D3D11_IsTexInjectThread (pTLS))
  {
    // Flag this thread so we don't infinitely recurse when querying the interface
    SK_ScopedBool auto_bool (&pTLS->texture_management.injection_thread);
    pTLS->texture_management.injection_thread = true;

	if (refcount_test)
      pTLS->texture_management.refcount_test++;

    ID3D11Texture2D* pTex = nullptr;

    if (SUCCEEDED (This->QueryInterface <ID3D11Texture2D> (&pTex)))
    {
      pTLS->texture_management.injection_thread = false;

      if (SK_D3D11_TextureIsCached (pTex))
        SK_D3D11_UseTexture (pTex);

      IUnknown_Release_Original (pTex);
    }
  }

  return IUnknown_AddRef_Original (This);
}


const wchar_t*
SK_D3D11_DescribeUsage (D3D11_USAGE usage)
{
  switch (usage)
  {
    case D3D11_USAGE_DEFAULT:
      return L"Default";
    case D3D11_USAGE_DYNAMIC:
      return L"Dynamic";
    case D3D11_USAGE_IMMUTABLE:
      return L"Immutable";
    case D3D11_USAGE_STAGING:
      return L"Staging";
    default:
      return L"UNKNOWN";
  }
}


const wchar_t*
SK_D3D11_DescribeFilter (D3D11_FILTER filter)
{
  switch (filter)
  {
    case D3D11_FILTER_MIN_MAG_MIP_POINT:                            return L"D3D11_FILTER_MIN_MAG_MIP_POINT";
    case D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR:                     return L"D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT:               return L"D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR:                     return L"D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR";
    case D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT:                     return L"D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT";
    case D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR:              return L"D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT:                     return L"D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_MIN_MAG_MIP_LINEAR:                           return L"D3D11_FILTER_MIN_MAG_MIP_LINEAR";

    case D3D11_FILTER_ANISOTROPIC:                                  return L"D3D11_FILTER_ANISOTROPIC";

    case D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT:                 return L"D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT";
    case D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR:          return L"D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:    return L"D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR:          return L"D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR";
    case D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT:          return L"D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT";
    case D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:   return L"D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT:          return L"D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR:                return L"D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR";

    case D3D11_FILTER_COMPARISON_ANISOTROPIC:                       return L"D3D11_FILTER_COMPARISON_ANISOTROPIC";

    case D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT:                    return L"D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT";
    case D3D11_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR:             return L"D3D11_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:       return L"D3D11_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR:             return L"D3D11_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR";
    case D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT:             return L"D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT";
    case D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:      return L"D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT:             return L"D3D11_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR:                   return L"D3D11_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR";

    case D3D11_FILTER_MINIMUM_ANISOTROPIC:                          return L"D3D11_FILTER_MINIMUM_ANISOTROPIC";

    case D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT:                    return L"D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT";
    case D3D11_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:             return L"D3D11_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:       return L"D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:             return L"D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR";
    case D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:             return L"D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT";
    case D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:      return L"D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:             return L"D3D11_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR:                   return L"D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR";

    case D3D11_FILTER_MAXIMUM_ANISOTROPIC:                          return L"D3D11_FILTER_MAXIMUM_ANISOTROPIC";

    default: return L"Unknown";
  };
}

std::wstring
SK_D3D11_DescribeMiscFlags (D3D11_RESOURCE_MISC_FLAG flags)
{
  std::wstring                 out = L"";
  std::stack <const wchar_t *> flag_text;

  if (flags & D3D11_RESOURCE_MISC_GENERATE_MIPS)
    flag_text.push (L"Generates Mipmaps");

  if (flags & D3D11_RESOURCE_MISC_SHARED)
    flag_text.push (L"Shared Resource");

  if (flags & D3D11_RESOURCE_MISC_TEXTURECUBE)
    flag_text.push (L"Cubemap");

  if (flags & D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS)
    flag_text.push (L"Draw Indirect Arguments");

  if (flags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
    flag_text.push (L"Allows Raw Buffer Views");

  if (flags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
    flag_text.push (L"Structured Buffer");

  if (flags & D3D11_RESOURCE_MISC_RESOURCE_CLAMP)
    flag_text.push (L"Resource Clamp");

  if (flags & D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX)
    flag_text.push (L"Shared Key Mutex");

  if (flags & D3D11_RESOURCE_MISC_GDI_COMPATIBLE)
    flag_text.push (L"GDI Compatible");

  if (flags & D3D11_RESOURCE_MISC_SHARED_NTHANDLE)
    flag_text.push (L"Shared Through NT Handles");

  if (flags & D3D11_RESOURCE_MISC_RESTRICTED_CONTENT)
    flag_text.push (L"Content is Encrypted");

  if (flags & D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE)
    flag_text.push (L"Shared Encrypted Content");

  if (flags & D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE_DRIVER)
    flag_text.push (L"Driver-level Encrypted Resource Sharing");

  if (flags & D3D11_RESOURCE_MISC_GUARDED)
    flag_text.push (L"Guarded");

  if (flags & D3D11_RESOURCE_MISC_TILE_POOL)
    flag_text.push (L"Stored in Tiled Resource Memory Pool");

  if (flags & D3D11_RESOURCE_MISC_TILED)
    flag_text.push (L"Tiled Resource");

  while (! flag_text.empty ())
  {
    out += flag_text.top ();
           flag_text.pop ();

    if (! flag_text.empty ())
      out += L", ";
  }

  return out;
}

std::wstring
SK_D3D11_DescribeBindFlags (D3D11_BIND_FLAG flags)
{
  std::wstring                 out = L"";
  std::stack <const wchar_t *> flag_text;

  if (flags & D3D11_BIND_CONSTANT_BUFFER)
    flag_text.push (L"Constant Buffer");

  if (flags & D3D11_BIND_DECODER)
    flag_text.push (L"Video Decoder");

  if (flags & D3D11_BIND_DEPTH_STENCIL)
    flag_text.push (L"Depth/Stencil");

  if (flags & D3D11_BIND_INDEX_BUFFER)
    flag_text.push (L"Index Buffer");

  if (flags & D3D11_BIND_RENDER_TARGET)
    flag_text.push (L"Render Target");

  if (flags & D3D11_BIND_SHADER_RESOURCE)
    flag_text.push (L"Shader Resource");

  if (flags & D3D11_BIND_STREAM_OUTPUT)
    flag_text.push (L"Stream Output");

  if (flags & D3D11_BIND_UNORDERED_ACCESS)
    flag_text.push (L"Unordered Access");

  if (flags & D3D11_BIND_VERTEX_BUFFER)
    flag_text.push (L"Vertex Buffer");

  if (flags & D3D11_BIND_VIDEO_ENCODER)
    flag_text.push (L"Video Encoder");

  while (! flag_text.empty ())
  {
    out += flag_text.top ();
           flag_text.pop ();

    if (! flag_text.empty ())
      out += L", ";
  }

  return out;
}


void
WINAPI
SK_D3D11_AddInjectable (uint32_t top_crc32, uint32_t checksum);

void
__stdcall
SK_D3D11_RemoveInjectable (uint32_t top_crc32, uint32_t checksum);

void
WINAPI
SK_D3D11_AddTexHash ( const wchar_t* name, uint32_t top_crc32, uint32_t hash );



// NEVER, under any circumstances, call any functions using this!
ID3D11Device* g_pD3D11Dev = nullptr;

struct d3d11_caps_t {
  struct {
    bool d3d11_1         = false;
  } feature_level;

  bool MapNoOverwriteOnDynamicConstantBuffer = false;
} d3d11_caps;

D3D11CreateDeviceAndSwapChain_pfn D3D11CreateDeviceAndSwapChain_Import = nullptr;
D3D11CreateDevice_pfn             D3D11CreateDevice_Import             = nullptr;

void
SK_D3D11_SetDevice ( ID3D11Device           **ppDevice,
                     D3D_FEATURE_LEVEL        FeatureLevel )
{
  if ( ppDevice != nullptr )
  {
    if ( *ppDevice != g_pD3D11Dev )
    {
      SK_LOG0 ( (L" >> Device = %08" PRIxPTR L"h (Feature Level:%s)",
                      (uintptr_t)*ppDevice,
                        SK_DXGI_FeatureLevelsToStr ( 1,
                                                      (DWORD *)&FeatureLevel
                                                   ).c_str ()
                  ), __SK_SUBSYSTEM__ );

      // We ARE technically holding a reference, but we never make calls to this
      //   interface - it's just for tracking purposes.
      g_pD3D11Dev = *ppDevice;

      SK_GetCurrentRenderBackend ().api = SK_RenderAPI::D3D11;
    }

    if (config.render.dxgi.exception_mode != -1)
      (*ppDevice)->SetExceptionMode (config.render.dxgi.exception_mode);

    CComQIPtr <IDXGIDevice>  pDXGIDev (*ppDevice);
    CComPtr   <IDXGIAdapter> pAdapter = nullptr;

    if ( pDXGIDev != nullptr )
    {
      HRESULT hr =
        pDXGIDev->GetParent ( IID_PPV_ARGS (&pAdapter.p) );

      if ( SUCCEEDED ( hr ) )
      {
        if ( pAdapter == nullptr )
          return;

        const int iver =
          SK_GetDXGIAdapterInterfaceVer ( pAdapter );

        // IDXGIAdapter3 = DXGI 1.4 (Windows 10+)
        if ( iver >= 3 )
        {
          SK::DXGI::StartBudgetThread ( &pAdapter.p );
        }
      }
    }
  }
}

UINT
SK_D3D11_RemoveUndesirableFlags (UINT& Flags)
{
  UINT original = Flags;

  // The Steam overlay behaves strangely when this is present
  Flags &= ~D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY;

  return original;
}

#include <SpecialK/import.h>

volatile LONG __dxgi_plugins_loaded = FALSE;

//   0 QueryInterface
//   1 AddRef
//   2 Release

//   3 GetDevice
//   4 GetPrivateData
//   5 SetPrivateData
//   6 SetPrivateDataInterface

//   7 VSSetConstantBuffers 
//   8 PSSetShaderResources 
//   9 PSSetShader 
//  10 PSSetSamplers 
//  11 VSSetShader 
//  12 DrawIndexed 
//  13 Draw 
//  14 Map 
//  15 Unmap 
//  16 PSSetConstantBuffers 
//  17 IASetInputLayout 
//  18 IASetVertexBuffers 
//  19 IASetIndexBuffer 
//  20 DrawIndexedInstanced 
//  21 DrawInstanced 
//  22 GSSetConstantBuffers 
//  23 GSSetShader 
//  24 IASetPrimitiveTopology 
//  25 VSSetShaderResources 
//  26 VSSetSamplers 
//  27 Begin
//  28 End
//  29 GetData 
//  30 SetPredication 
//  31 GSSetShaderResources 
//  32 GSSetSamplers 
//  33 OMSetRenderTargets 
//  34 OMSetRenderTargetsAndUnorderedAccessViews 
//  35 OMSetBlendState 
//  36 OMSetDepthStencilState 
//  37 SOSetTargets 
//  38 DrawAuto
//  39 DrawIndexedInstancedIndirect 
//  40 DrawInstancedIndirect 
//  41 Dispatch 
//  42 DispatchIndirect 
//  43 RSSetState 
//  44 RSSetViewports 
//  45 RSSetScissorRects 
//  46 CopySubresourceRegion 
//  47 CopyResource 
//  48 UpdateSubresource 
//  49 CopyStructureCount 
//  50 ClearRenderTargetView 
//  51 ClearUnorderedAccessViewUint 
//  52 ClearUnorderedAccessViewFloat 
//  53 ClearDepthStencilView 
//  54 GenerateMips 
//  55 SetResourceMinLOD 
//  56 GetResourceMinLOD 
//  57 ResolveSubresource 
//  58 ExecuteCommandList 
//  59 HSSetShaderResources 
//  60 HSSetShader 
//  61 HSSetSamplers 
//  62 HSSetConstantBuffers 
//  63 DSSetShaderResources 
//  64 DSSetShader 
//  65 DSSetSamplers 
//  66 DSSetConstantBuffers 
//  67 CSSetShaderResources 
//  68 CSSetUnorderedAccessViews 
//  69 CSSetShader 
//  70 CSSetSamplers 
//  71 CSSetConstantBuffers 
//  72 VSGetConstantBuffers 
//  73 PSGetShaderResources 
//  74 PSGetShader 
//  75 PSGetSamplers 
//  76 VSGetShader 
//  77 PSGetConstantBuffers 
//  78 IAGetInputLayout 
//  79 IAGetVertexBuffers 
//  80 IAGetIndexBuffer 
//  81 GSGetConstantBuffers 
//  82 GSGetShader 
//  83 IAGetPrimitiveTopology 
//  84 VSGetShaderResources
//  85 VSGetSamplers 
//  86 GetPredication
//  87 GSGetShaderResources
//  88 GSGetSamplers
//  89 OMGetRenderTargets 
//  90 OMGetRenderTargetsAndUnorderedAccessViews 
//  91 OMGetBlendState 
//  92 OMGetDepthStencilState 
//  93 SOGetTargets 
//  94 RSGetState 
//  95 RSGetViewports 
//  96 RSGetScissorRects 
//  97 HSGetShaderResources 
//  98 HSGetShader 
//  99 HSGetSamplers 
// 100 HSGetConstantBuffers 
// 101 DSGetShaderResources 
// 102 DSGetShader 
// 103 DSGetSamplers 
// 104 DSGetConstantBuffers 
// 105 CSGetShaderResources 
// 106 CSGetUnorderedAccessViews 
// 107 CSGetShader 
// 108 CSGetSamplers 
// 109 CSGetConstantBuffers        
// 110 ClearState
// 111 Flush
// 112 GetType
// 113 GetContextFlags
// 114 FinishCommandList




__declspec (noinline)
HRESULT
WINAPI
D3D11CreateDeviceAndSwapChain_Detour (IDXGIAdapter          *pAdapter,
                                      D3D_DRIVER_TYPE        DriverType,
                                      HMODULE                Software,
                                      UINT                   Flags,
 _In_reads_opt_ (FeatureLevels) CONST D3D_FEATURE_LEVEL     *pFeatureLevels,
                                      UINT                   FeatureLevels,
                                      UINT                   SDKVersion,
 _In_opt_                       CONST DXGI_SWAP_CHAIN_DESC  *pSwapChainDesc,
 _Out_opt_                            IDXGISwapChain       **ppSwapChain,
 _Out_opt_                            ID3D11Device         **ppDevice,
 _Out_opt_                            D3D_FEATURE_LEVEL     *pFeatureLevel,
 _Out_opt_                            ID3D11DeviceContext  **ppImmediateContext)
{
  // Even if the game doesn't care about the feature level, we do.
  D3D_FEATURE_LEVEL ret_level  = D3D_FEATURE_LEVEL_11_1;
  ID3D11Device*     ret_device = nullptr;

  // Allow override of swapchain parameters
  DXGI_SWAP_CHAIN_DESC  swap_chain_override = {   };
  auto*                 swap_chain_desc     =
    const_cast <DXGI_SWAP_CHAIN_DESC *> (pSwapChainDesc);

  DXGI_LOG_CALL_1 (L"D3D11CreateDeviceAndSwapChain", L"Flags=0x%x", Flags );

  SK_D3D11_Init ();

  dll_log.LogEx ( true,
                    L"[  D3D 11  ]  <~> Preferred Feature Level(s): <%u> - %s\n",
                      FeatureLevels,
                        SK_DXGI_FeatureLevelsToStr (
                          FeatureLevels,
                            (DWORD *)pFeatureLevels
                        ).c_str ()
                );

  // Optionally Enable Debug Layer
  if (ReadAcquire (&__d3d11_ready))
  {
    if (config.render.dxgi.debug_layer && (! (Flags & D3D11_CREATE_DEVICE_DEBUG)))
    {
      SK_LOG0 ( ( L" ==> Enabling D3D11 Debug layer" ),
                  __SK_SUBSYSTEM__ );
      Flags |= D3D11_CREATE_DEVICE_DEBUG;
    }
  }


  SK_D3D11_RemoveUndesirableFlags (Flags);


  //
  // DXGI Adapter Override (for performance)
  //

  SK_DXGI_AdapterOverride ( &pAdapter, &DriverType );

  if (swap_chain_desc != nullptr)
  {
    wchar_t wszMSAA [128] = { };

    _swprintf ( wszMSAA, swap_chain_desc->SampleDesc.Count > 1 ?
                           L"%u Samples" :
                           L"Not Used (or Offscreen)",
                  swap_chain_desc->SampleDesc.Count );

    dll_log.LogEx ( true,
      L"[Swap Chain]\n"
      L"  +-------------+-------------------------------------------------------------------------+\n"
      L"  | Resolution. |  %4lux%4lu @ %6.2f Hz%-50ws|\n"
      L"  | Format..... |  %-71ws|\n"
      L"  | Buffers.... |  %-2lu%-69ws|\n"
      L"  | MSAA....... |  %-71ws|\n"
      L"  | Mode....... |  %-71ws|\n"
      L"  | Scaling.... |  %-71ws|\n"
      L"  | Scanlines.. |  %-71ws|\n"
      L"  | Flags...... |  0x%04x%-65ws|\n"
      L"  | SwapEffect. |  %-71ws|\n"
      L"  +-------------+-------------------------------------------------------------------------+\n",
          swap_chain_desc->BufferDesc.Width,
          swap_chain_desc->BufferDesc.Height,
          swap_chain_desc->BufferDesc.RefreshRate.Denominator != 0 ?
            static_cast <float> (swap_chain_desc->BufferDesc.RefreshRate.Numerator) /
            static_cast <float> (swap_chain_desc->BufferDesc.RefreshRate.Denominator) :
              std::numeric_limits <float>::quiet_NaN (), L" ",
    SK_DXGI_FormatToStr (swap_chain_desc->BufferDesc.Format).c_str (),
          swap_chain_desc->BufferCount, L" ",
          wszMSAA,
          swap_chain_desc->Windowed ? L"Windowed" : L"Fullscreen",
          swap_chain_desc->BufferDesc.Scaling == DXGI_MODE_SCALING_UNSPECIFIED ?
            L"Unspecified" :
            swap_chain_desc->BufferDesc.Scaling == DXGI_MODE_SCALING_CENTERED ?
              L"Centered" :
              L"Stretched",
          swap_chain_desc->BufferDesc.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED ?
            L"Unspecified" :
            swap_chain_desc->BufferDesc.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE ?
              L"Progressive" :
              swap_chain_desc->BufferDesc.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST ?
                L"Interlaced Even" :
                L"Interlaced Odd",
          swap_chain_desc->Flags, L" ",
          swap_chain_desc->SwapEffect         == 0 ?
            L"Discard" :
            swap_chain_desc->SwapEffect       == 1 ?
              L"Sequential" :
              swap_chain_desc->SwapEffect     == 2 ?
                L"<Unknown>" :
                swap_chain_desc->SwapEffect   == 3 ?
                  L"Flip Sequential" :
                  swap_chain_desc->SwapEffect == 4 ?
                    L"Flip Discard" :
                    L"<Unknown>" );

    swap_chain_override = *swap_chain_desc;
    swap_chain_desc     = &swap_chain_override;

    if ( config.render.dxgi.scaling_mode      != -1 &&
          swap_chain_desc->BufferDesc.Scaling !=
            (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode )
    {
      SK_LOG0 ( ( L" >> Scaling Override "
                  L"(Requested: %s, Using: %s)",
                      SK_DXGI_DescribeScalingMode (
                        swap_chain_desc->BufferDesc.Scaling
                      ),
                        SK_DXGI_DescribeScalingMode (
                          (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode
                        )
                  ), __SK_SUBSYSTEM__ );

      swap_chain_desc->BufferDesc.Scaling =
        (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode;
    }

    if (! config.window.res.override.isZero ())
    {
      swap_chain_desc->BufferDesc.Width  = config.window.res.override.x;
      swap_chain_desc->BufferDesc.Height = config.window.res.override.y;
    }

    else
    {
      SK_DXGI_BorderCompensation (
        swap_chain_desc->BufferDesc.Width,
          swap_chain_desc->BufferDesc.Height
      );
    }
  }


  HRESULT res = E_UNEXPECTED;

  DXGI_CALL (res, 
    D3D11CreateDeviceAndSwapChain_Import ( pAdapter,
                                             DriverType,
                                               Software,
                                                 Flags,
                                                   pFeatureLevels,
                                                     FeatureLevels,
                                                       SDKVersion,
                                                         swap_chain_desc,
                                                           ppSwapChain,
                                                             &ret_device,
                                                               &ret_level,
                                                                 ppImmediateContext )
            );
  

  if (SUCCEEDED (res))
  {
    if (swap_chain_desc != nullptr)
    {
      wchar_t wszClass [MAX_PATH * 2] = { };

      if (swap_chain_desc != nullptr)
        RealGetWindowClassW (swap_chain_desc->OutputWindow, wszClass, MAX_PATH);

      bool dummy_window = 
        StrStrIW (wszClass, L"Special K Dummy Window Class (Ex)") ||
        StrStrIW (wszClass, L"RTSSWndClass");

      if (! dummy_window)
      {
        SK_GetCurrentRenderBackend ().windows.setDevice (swap_chain_desc->OutputWindow);

        void
        SK_InstallWindowHook (HWND hWnd);
        SK_InstallWindowHook (swap_chain_desc->OutputWindow);

        if ( dwRenderThread == 0x00 ||
             dwRenderThread == GetCurrentThreadId () )
        {
          if ( SK_GetCurrentRenderBackend ().windows.device != nullptr &&
               swap_chain_desc->OutputWindow                != nullptr &&
               swap_chain_desc->OutputWindow                != SK_GetCurrentRenderBackend ().windows.device )
            SK_LOG0 ( (L"Game created a new window?!"), __SK_SUBSYSTEM__ );
        }

        extern void SK_DXGI_HookSwapChain (IDXGISwapChain* pSwapChain);

        if (ppSwapChain != nullptr)
          SK_DXGI_HookSwapChain (*ppSwapChain);

        // Assume the first thing to create a D3D11 render device is
        //   the game and that devices never migrate threads; for most games
        //     this assumption holds.
        if ( dwRenderThread == 0x00 ||
             dwRenderThread == GetCurrentThreadId () )
        {
          dwRenderThread = GetCurrentThreadId ();
        }

        SK_D3D11_SetDevice ( &ret_device, ret_level );
      }
    }
  }

  if (ppDevice != nullptr)
    *ppDevice   = ret_device;

  if (pFeatureLevel != nullptr)
    *pFeatureLevel   = ret_level;


  if (ppDevice != nullptr && SUCCEEDED (res))
  {
    D3D11_FEATURE_DATA_D3D11_OPTIONS options;
    (*ppDevice)->CheckFeatureSupport ( D3D11_FEATURE_D3D11_OPTIONS,
                                         &options, sizeof (options) );

    d3d11_caps.MapNoOverwriteOnDynamicConstantBuffer =
       options.MapNoOverwriteOnDynamicConstantBuffer;
  }

  return res;
}

__declspec (noinline)
HRESULT
WINAPI
D3D11CreateDevice_Detour (
  _In_opt_                            IDXGIAdapter         *pAdapter,
                                      D3D_DRIVER_TYPE       DriverType,
                                      HMODULE               Software,
                                      UINT                  Flags,
  _In_opt_                      const D3D_FEATURE_LEVEL    *pFeatureLevels,
                                      UINT                  FeatureLevels,
                                      UINT                  SDKVersion,
  _Out_opt_                           ID3D11Device        **ppDevice,
  _Out_opt_                           D3D_FEATURE_LEVEL    *pFeatureLevel,
  _Out_opt_                           ID3D11DeviceContext **ppImmediateContext)
{
  DXGI_LOG_CALL_1 (L"D3D11CreateDevice            ", L"Flags=0x%x", Flags);

  SK_D3D11_Init ();

  return
    D3D11CreateDeviceAndSwapChain_Detour ( pAdapter, DriverType, Software, Flags,
                                             pFeatureLevels, FeatureLevels, SDKVersion,
                                               nullptr, nullptr, ppDevice, pFeatureLevel,
                                                 ppImmediateContext );
}




__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateTexture2D_Override (
_In_            ID3D11Device           *This,
_In_      const D3D11_TEXTURE2D_DESC   *pDesc,
_In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
_Out_opt_       ID3D11Texture2D        **ppTexture2D );

D3D11Dev_CreateSamplerState_pfn                     D3D11Dev_CreateSamplerState_Original                     = nullptr;
D3D11Dev_CreateBuffer_pfn                           D3D11Dev_CreateBuffer_Original                           = nullptr;
D3D11Dev_CreateTexture2D_pfn                        D3D11Dev_CreateTexture2D_Original                        = nullptr;
D3D11Dev_CreateRenderTargetView_pfn                 D3D11Dev_CreateRenderTargetView_Original                 = nullptr;
D3D11Dev_CreateShaderResourceView_pfn               D3D11Dev_CreateShaderResourceView_Original               = nullptr;
D3D11Dev_CreateDepthStencilView_pfn                 D3D11Dev_CreateDepthStencilView_Original                 = nullptr;
D3D11Dev_CreateUnorderedAccessView_pfn              D3D11Dev_CreateUnorderedAccessView_Original              = nullptr;

NvAPI_D3D11_CreateVertexShaderEx_pfn                NvAPI_D3D11_CreateVertexShaderEx_Original                = nullptr;
NvAPI_D3D11_CreateGeometryShaderEx_2_pfn            NvAPI_D3D11_CreateGeometryShaderEx_2_Original            = nullptr;
NvAPI_D3D11_CreateFastGeometryShader_pfn            NvAPI_D3D11_CreateFastGeometryShader_Original            = nullptr;
NvAPI_D3D11_CreateFastGeometryShaderExplicit_pfn    NvAPI_D3D11_CreateFastGeometryShaderExplicit_Original    = nullptr;
NvAPI_D3D11_CreateHullShaderEx_pfn                  NvAPI_D3D11_CreateHullShaderEx_Original                  = nullptr;
NvAPI_D3D11_CreateDomainShaderEx_pfn                NvAPI_D3D11_CreateDomainShaderEx_Original                = nullptr;

D3D11Dev_CreateVertexShader_pfn                     D3D11Dev_CreateVertexShader_Original                     = nullptr;
D3D11Dev_CreatePixelShader_pfn                      D3D11Dev_CreatePixelShader_Original                      = nullptr;
D3D11Dev_CreateGeometryShader_pfn                   D3D11Dev_CreateGeometryShader_Original                   = nullptr;
D3D11Dev_CreateGeometryShaderWithStreamOutput_pfn   D3D11Dev_CreateGeometryShaderWithStreamOutput_Original   = nullptr;
D3D11Dev_CreateHullShader_pfn                       D3D11Dev_CreateHullShader_Original                       = nullptr;
D3D11Dev_CreateDomainShader_pfn                     D3D11Dev_CreateDomainShader_Original                     = nullptr;
D3D11Dev_CreateComputeShader_pfn                    D3D11Dev_CreateComputeShader_Original                    = nullptr;

D3D11Dev_CreateDeferredContext_pfn                  D3D11Dev_CreateDeferredContext_Original                  = nullptr;
D3D11Dev_GetImmediateContext_pfn                    D3D11Dev_GetImmediateContext_Original                    = nullptr;

D3D11_RSSetScissorRects_pfn                         D3D11_RSSetScissorRects_Original                         = nullptr;
D3D11_RSSetViewports_pfn                            D3D11_RSSetViewports_Original                            = nullptr;
D3D11_VSSetConstantBuffers_pfn                      D3D11_VSSetConstantBuffers_Original                      = nullptr;
D3D11_VSSetShaderResources_pfn                      D3D11_VSSetShaderResources_Original                      = nullptr;
D3D11_PSSetShaderResources_pfn                      D3D11_PSSetShaderResources_Original                      = nullptr;
D3D11_PSSetConstantBuffers_pfn                      D3D11_PSSetConstantBuffers_Original                      = nullptr;
D3D11_GSSetShaderResources_pfn                      D3D11_GSSetShaderResources_Original                      = nullptr;
D3D11_HSSetShaderResources_pfn                      D3D11_HSSetShaderResources_Original                      = nullptr;
D3D11_DSSetShaderResources_pfn                      D3D11_DSSetShaderResources_Original                      = nullptr;
D3D11_CSSetShaderResources_pfn                      D3D11_CSSetShaderResources_Original                      = nullptr;
D3D11_CSSetUnorderedAccessViews_pfn                 D3D11_CSSetUnorderedAccessViews_Original                 = nullptr;
D3D11_UpdateSubresource_pfn                         D3D11_UpdateSubresource_Original                         = nullptr;
D3D11_DrawIndexed_pfn                               D3D11_DrawIndexed_Original                               = nullptr;
D3D11_Draw_pfn                                      D3D11_Draw_Original                                      = nullptr;
D3D11_DrawAuto_pfn                                  D3D11_DrawAuto_Original                                  = nullptr;
D3D11_DrawIndexedInstanced_pfn                      D3D11_DrawIndexedInstanced_Original                      = nullptr;
D3D11_DrawIndexedInstancedIndirect_pfn              D3D11_DrawIndexedInstancedIndirect_Original              = nullptr;
D3D11_DrawInstanced_pfn                             D3D11_DrawInstanced_Original                             = nullptr;
D3D11_DrawInstancedIndirect_pfn                     D3D11_DrawInstancedIndirect_Original                     = nullptr;
D3D11_Dispatch_pfn                                  D3D11_Dispatch_Original                                  = nullptr;
D3D11_DispatchIndirect_pfn                          D3D11_DispatchIndirect_Original                          = nullptr;
D3D11_Map_pfn                                       D3D11_Map_Original                                       = nullptr;
D3D11_Unmap_pfn                                     D3D11_Unmap_Original                                     = nullptr;

D3D11_OMSetRenderTargets_pfn                        D3D11_OMSetRenderTargets_Original                        = nullptr;
D3D11_OMSetRenderTargetsAndUnorderedAccessViews_pfn D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Original = nullptr;
D3D11_OMGetRenderTargets_pfn                        D3D11_OMGetRenderTargets_Original                        = nullptr;
D3D11_OMGetRenderTargetsAndUnorderedAccessViews_pfn D3D11_OMGetRenderTargetsAndUnorderedAccessViews_Original = nullptr;
D3D11_ClearDepthStencilView_pfn                     D3D11_ClearDepthStencilView_Original                     = nullptr;

D3D11_PSSetSamplers_pfn                             D3D11_PSSetSamplers_Original                             = nullptr;

D3D11_VSSetShader_pfn                               D3D11_VSSetShader_Original                               = nullptr;
D3D11_PSSetShader_pfn                               D3D11_PSSetShader_Original                               = nullptr;
D3D11_GSSetShader_pfn                               D3D11_GSSetShader_Original                               = nullptr;
D3D11_HSSetShader_pfn                               D3D11_HSSetShader_Original                               = nullptr;
D3D11_DSSetShader_pfn                               D3D11_DSSetShader_Original                               = nullptr;
D3D11_CSSetShader_pfn                               D3D11_CSSetShader_Original                               = nullptr;

D3D11_VSGetShader_pfn                               D3D11_VSGetShader_Original                               = nullptr;
D3D11_PSGetShader_pfn                               D3D11_PSGetShader_Original                               = nullptr;
D3D11_GSGetShader_pfn                               D3D11_GSGetShader_Original                               = nullptr;
D3D11_HSGetShader_pfn                               D3D11_HSGetShader_Original                               = nullptr;
D3D11_DSGetShader_pfn                               D3D11_DSGetShader_Original                               = nullptr;
D3D11_CSGetShader_pfn                               D3D11_CSGetShader_Original                               = nullptr;

D3D11_CopyResource_pfn                              D3D11_CopyResource_Original                              = nullptr;
D3D11_CopySubresourceRegion_pfn                     D3D11_CopySubresourceRegion_Original                     = nullptr;
D3D11_UpdateSubresource1_pfn                        D3D11_UpdateSubresource1_Original                        = nullptr;


struct cache_params_s {
  uint32_t max_entries       = 4096UL;
  uint32_t min_entries       = 1024UL;
  uint32_t max_size          = 2048UL; // Measured in MiB
  uint32_t min_size          = 512UL;
  uint32_t min_evict         = 16;
  uint32_t max_evict         = 64;
      bool ignore_non_mipped = false;
} cache_opts;


//
// Helpers for typeless DXGI format class view compatibility
//
//   Returns true if this is a valid way of (re)interpreting a sized datatype
bool
SK_D3D11_IsDataSizeClassOf ( DXGI_FORMAT typeless, DXGI_FORMAT typed,
                             int         recursion = 0 )
{
  switch (typeless)
  {
    case DXGI_FORMAT_R24G8_TYPELESS:
      return true;

    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
      return true;

    case DXGI_FORMAT_D24_UNORM_S8_UINT:
      return typed == DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
      return true;
  }

  if (recursion == 0)
    return SK_D3D11_IsDataSizeClassOf (typed, typeless, 1);

  return false;
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
D3D11Dev_CreateRenderTargetView_Override (
  _In_            ID3D11Device                   *This,
  _In_            ID3D11Resource                 *pResource,
  _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC  *pDesc,
  _Out_opt_       ID3D11RenderTargetView        **ppRTView )
{
  // Unity throws around NULL for pResource
  if (pDesc != nullptr && pResource != nullptr)
  {
    D3D11_RENDER_TARGET_VIEW_DESC desc = { };
    D3D11_RESOURCE_DIMENSION      dim  = { };

    pResource->GetType (&dim);

    if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      desc = *pDesc;

      DXGI_FORMAT newFormat =
        pDesc->Format;

      CComQIPtr <ID3D11Texture2D> pTex (pResource);

      if (pTex != nullptr)
      {
        D3D11_TEXTURE2D_DESC  tex_desc = { };
        pTex->GetDesc       (&tex_desc);

        if ( SK_D3D11_OverrideDepthStencil (newFormat) )
          desc.Format = newFormat;

        if (SK_GetCurrentGameID () == SK_GAME_ID::DotHackGU)
        {
          if ((pDesc->Format == DXGI_FORMAT_B8G8R8A8_UNORM || pDesc->Format == DXGI_FORMAT_B8G8R8A8_TYPELESS))
          {
            newFormat   = pDesc->Format == DXGI_FORMAT_B8G8R8A8_UNORM ? DXGI_FORMAT_R8G8B8A8_UNORM :
                                                                        DXGI_FORMAT_R8G8B8A8_TYPELESS;
            desc.Format = newFormat;
          }
        }

        HRESULT hr =
          D3D11Dev_CreateRenderTargetView_Original ( This, pResource,
                                                       &desc, ppRTView );

        if (SUCCEEDED (hr))
          return hr;
      }
    }
  }

  return
    D3D11Dev_CreateRenderTargetView_Original ( This,  pResource,
                                                 pDesc, ppRTView );
}

__forceinline
SK_D3D11_KnownShaders_Singleton&
__SK_Singleton_D3D11_Shaders (void)
{
  static SK_D3D11_KnownShaders _SK_D3D11_Shaders;
  return                       _SK_D3D11_Shaders;
}


struct SK_D3D11_KnownTargets
{
  std::unordered_set <ID3D11RenderTargetView *> rt_views;
  std::unordered_set <ID3D11DepthStencilView *> ds_views;

  SK_D3D11_KnownTargets (void)
  {
    rt_views.reserve (128);
    ds_views.reserve ( 64);
  }

  void clear (void)
  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_render_view);

    for (auto it : rt_views)
      it->Release ();

    for (auto it : ds_views)
      it->Release ();

    rt_views.clear ();
    ds_views.clear ();
  }
};

std::array <SK_D3D11_KnownTargets, SK_D3D11_MAX_DEV_CONTEXTS + 1> SK_D3D11_RenderTargets;


struct SK_D3D11_KnownThreads
{
  SK_D3D11_KnownThreads (void)
  {
    InitializeCriticalSectionAndSpinCount (&cs, 0x400);

    //ids.reserve    (16);
    //active.reserve (16);
  }

  void   clear_all    (void);
  size_t count_all    (void);

  void   clear_active (void)
  {
    if (use_lock)
    {
      SK_AutoCriticalSection auto_cs (&cs);
      active.clear ();
      return;
    }

    active.clear ();
  }

  size_t count_active (void)
  {
    if (use_lock)
    {
      SK_AutoCriticalSection auto_cs (&cs);
      return active.size ();
    }

    return active.size ();
  }

  float  active_ratio (void)
  {
    if (use_lock)
    {
      SK_AutoCriticalSection auto_cs (&cs);
      return (float)active.size () / (float)ids.size ();
    }

    return (float)active.size () / (float)ids.size ();
  }

  static void mark (void);

private:
  std::unordered_set <DWORD> ids;
  std::unordered_set <DWORD> active;
  CRITICAL_SECTION           cs;
  bool             use_lock = true;
} SK_D3D11_MemoryThreads,
  SK_D3D11_DrawThreads,
  SK_D3D11_DispatchThreads,
  SK_D3D11_ShaderThreads;


void
SK_D3D11_KnownThreads::clear_all (void)
{
  if (use_lock)
  {
    SK_AutoCriticalSection auto_cs (&cs);
    ids.clear ();
    return;
  }

  ids.clear ();
}

size_t
SK_D3D11_KnownThreads::count_all (void)
{
  if (use_lock)
  {
    SK_AutoCriticalSection auto_cs (&cs);
    return ids.size ();
  }

  return ids.size ();
}

void
SK_D3D11_KnownThreads::mark (void)
{
//#ifndef _DEBUG
#if 1
  return;
#else
  if (! SK_D3D11_EnableTracking)
    return;

  if (use_lock)
  {
    SK_AutoCriticalSection auto_cs (&cs);
    ids.emplace    (GetCurrentThreadId ());
    active.emplace (GetCurrentThreadId ());
    return;
  }

  ids.emplace    (GetCurrentThreadId ());
  active.emplace (GetCurrentThreadId ());
#endif
}

#include <array>

struct memory_tracking_s
{
  static const int __types = 5;

  memory_tracking_s (void)
  {
    cs = &cs_mmio;
  }

  struct history_s
  {
    history_s (void) = default;
    //{
      //vertex_buffers.reserve   ( 128);
      //index_buffers.reserve    ( 256);
      //constant_buffers.reserve (2048);
    //}

    void clear (SK_Thread_CriticalSection* /*cs*/)
    {
      ///std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs);

      for (int i = 0; i < __types; i++)
      {
        map_types      [i] = 0;
        resource_types [i] = 0;
      }

      bytes_written = 0ULL;
      bytes_read    = 0ULL;
      bytes_copied  = 0ULL;

      active_set.clear_using (empty_set);
    }

    int pinned_frames = 0;

    // Clearing these containers performs dynamic heap allocation irrespective of
    //   prior state (i.e. clearing a set whose size is zero is the same as 100),
    //     so we want to swap an already cleared set whenever possible to avoid
    //       disrupting SK's thread-optimized memory placement.
    struct datastore_s {
      concurrency::concurrent_unordered_set <ID3D11Resource *> mapped_resources;
      concurrency::concurrent_unordered_set <ID3D11Buffer *>   index_buffers;
      concurrency::concurrent_unordered_set <ID3D11Buffer *>   vertex_buffers;
      concurrency::concurrent_unordered_set <ID3D11Buffer *>   constant_buffers;

      // It is only safe to call this at the end of a frame
      void clear_using ( datastore_s& fresh )
      {
        if (mapped_resources.size ())
        {
          std::swap (mapped_resources, fresh.mapped_resources);
                                       fresh.mapped_resources.clear ();
        }

        if (index_buffers.size ())
        {
          std::swap (index_buffers, fresh.index_buffers);
                                    fresh.index_buffers.clear ();
        }

        if (vertex_buffers.size ())
        {
          std::swap (vertex_buffers, fresh.vertex_buffers);
                                     fresh.vertex_buffers.clear ();
        }

        if (constant_buffers.size ())
        {
          std::swap (constant_buffers, fresh.constant_buffers);
                                       fresh.constant_buffers.clear ();
        }
      }
    } empty_set,
      active_set;

    concurrency::concurrent_unordered_set <ID3D11Buffer *>&
      index_buffers    = active_set.index_buffers;
    concurrency::concurrent_unordered_set <ID3D11Buffer *>&
      vertex_buffers   = active_set.vertex_buffers;
    concurrency::concurrent_unordered_set <ID3D11Buffer *>&
      constant_buffers = active_set.constant_buffers;
    concurrency::concurrent_unordered_set <ID3D11Resource *>&
      mapped_resources = active_set.mapped_resources;


    std::atomic <uint32_t> map_types      [__types] = { };
    std::atomic <uint32_t> resource_types [__types] = { };

    std::atomic <uint64_t> bytes_read               = 0ULL;
    std::atomic <uint64_t> bytes_written            = 0ULL;
    std::atomic <uint64_t> bytes_copied             = 0ULL;
  } lifetime, last_frame;


  std::atomic <uint32_t>   num_maps                 = 0UL;
  std::atomic <uint32_t>   num_unmaps               = 0UL; // If does not match, something is pinned.


  void clear (void)
  {
    if (num_maps != num_unmaps)
      ++lifetime.pinned_frames;

    num_maps   = 0UL;
    num_unmaps = 0UL;

    for (int i = 0; i < __types; i++)
    {
      lifetime.map_types      [i] += last_frame.map_types      [i];
      lifetime.resource_types [i] += last_frame.resource_types [i];
    }

    lifetime.bytes_read    += last_frame.bytes_read;
    lifetime.bytes_written += last_frame.bytes_written;
    lifetime.bytes_copied  += last_frame.bytes_copied;

    last_frame.clear (cs);
  }

  SK_Thread_CriticalSection* cs;
} mem_map_stats;

struct target_tracking_s
{
  target_tracking_s (void)
  {
    for ( int i = 0; i < SK_D3D11_MAX_DEV_CONTEXTS + 1; ++i )
    {
      for (auto& it : active [i])
      {
        it = false;
      }

      active_count [i] = 0;
    }

    //ref_vs.reserve (16); ref_ps.reserve (16);
    //ref_gs.reserve (8);
    //ref_hs.reserve (4);  ref_ds.reserve (4);
    //ref_cs.reserve (2);
  };


  struct refs_s {
    concurrency::concurrent_unordered_set <uint32_t> ref_vs;
    concurrency::concurrent_unordered_set <uint32_t> ref_ps;
    concurrency::concurrent_unordered_set <uint32_t> ref_gs;
    concurrency::concurrent_unordered_set <uint32_t> ref_hs;
    concurrency::concurrent_unordered_set <uint32_t> ref_ds;
    concurrency::concurrent_unordered_set <uint32_t> ref_cs;

    // It is only safe to call this at the end of a frame
    void clear_using ( refs_s& fresh )
    {
      //
      // Swap-and-Discard a pre-allocated concurrent set, because
      //   clearing any of the concurrency runtime containers causes heap
      //     allocation even if the container was already empty!
      //
      if (ref_vs.size ())
      {
        std::swap (ref_vs, fresh.ref_vs);
                           fresh.ref_vs.clear ();
      }

      if (ref_ps.size ())
      {
        std::swap (ref_ps, fresh.ref_ps);
                           fresh.ref_ps.clear ();
      }

      if (ref_gs.size ())
      {
        std::swap (ref_gs, fresh.ref_gs);
                           fresh.ref_gs.clear ();
      }

      if (ref_ds.size ())
      {
        std::swap (ref_ds, fresh.ref_ds);
                           fresh.ref_ds.clear ();
      }

      if (ref_hs.size ())
      {
        std::swap (ref_hs, fresh.ref_hs);
                           fresh.ref_hs.clear ();
      }

      if (ref_cs.size ())
      {
        std::swap (ref_cs, fresh.ref_cs);
                           fresh.ref_cs.clear ();
      }
    }
  } empty_set,
    active_set;


  void clear (void)
  {
    for ( int i = 0; i < SK_D3D11_MAX_DEV_CONTEXTS + 1; ++i )
    {
      for (auto& it : active [i]) it = false;

      active_count [i] = 0;
    }

    num_draws = 0;

    active_set.clear_using (empty_set);
  }

  volatile ID3D11RenderTargetView*       resource     =  (ID3D11RenderTargetView *)INTPTR_MAX;

  std::atomic_bool                       active [SK_D3D11_MAX_DEV_CONTEXTS+1][D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]
                                                      = { };
  std::atomic <uint32_t>                 active_count [SK_D3D11_MAX_DEV_CONTEXTS+1]
                                                      = { };

  std::atomic <uint32_t>                 num_draws    =     0;

  concurrency::concurrent_unordered_set <uint32_t>& ref_vs =
    active_set.ref_vs;
  concurrency::concurrent_unordered_set <uint32_t>& ref_ps =
    active_set.ref_ps;
  concurrency::concurrent_unordered_set <uint32_t>& ref_gs =
    active_set.ref_gs;
  concurrency::concurrent_unordered_set <uint32_t>& ref_hs =
    active_set.ref_hs;
  concurrency::concurrent_unordered_set <uint32_t>& ref_ds =
    active_set.ref_ds;
  concurrency::concurrent_unordered_set <uint32_t>& ref_cs =
    active_set.ref_cs;
} tracked_rtv;

ID3D11Texture2D* tracked_texture               = nullptr;
DWORD            tracked_tex_blink_duration    = 666UL;
DWORD            tracked_shader_blink_duration = 666UL;

struct SK_DisjointTimerQueryD3D11 d3d11_shader_tracking_s::disjoint_query;



void
d3d11_shader_tracking_s::activate ( ID3D11DeviceContext        *pDevContext,
                                    ID3D11ClassInstance *const *ppClassInstances,
                                    UINT                        NumClassInstances )
{
  for ( UINT i = 0 ; i < NumClassInstances ; i++ )
  {
    if (ppClassInstances && ppClassInstances [i])
        addClassInstance   (ppClassInstances [i]);
  }

  UINT dev_idx =
    SK_D3D11_GetDeviceContextHandle (pDevContext);

  bool is_active = active.get (dev_idx);

  if ((! is_active))
  {
    active.set (dev_idx, true);

    switch (type_)
    {
      case SK_D3D11_ShaderType::Vertex:
        SK_D3D11_Shaders.vertex.current.shader   [dev_idx] = crc32c;
        break;
      case SK_D3D11_ShaderType::Pixel:
        SK_D3D11_Shaders.pixel.current.shader    [dev_idx] = crc32c;
        break;
      case SK_D3D11_ShaderType::Geometry:
        SK_D3D11_Shaders.geometry.current.shader [dev_idx] = crc32c;
        break;
      case SK_D3D11_ShaderType::Domain:
        SK_D3D11_Shaders.domain.current.shader   [dev_idx] = crc32c;
        break;
      case SK_D3D11_ShaderType::Hull:
        SK_D3D11_Shaders.hull.current.shader     [dev_idx] = crc32c;
        break;
      case SK_D3D11_ShaderType::Compute:
        SK_D3D11_Shaders.compute.current.shader  [dev_idx] = crc32c;
        break;
    }
  }

  else
    return;


  // Timing is very difficult on deferred contexts; will finish later (years?)
  if ( pDevContext != nullptr && pDevContext->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED )
    return;


  CComPtr <ID3D11Device> dev = nullptr;

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if ( pDevContext          != nullptr                             &&
                  rb.device != nullptr                             &&
       SUCCEEDED (rb.device->QueryInterface <ID3D11Device> (&dev)) &&
                  rb.device.IsEqualObject                  ( dev) )
  {
    if (ReadPointerAcquire ((void **)&disjoint_query.async) == nullptr && timers.empty ())
    {
      D3D11_QUERY_DESC query_desc {
        D3D11_QUERY_TIMESTAMP_DISJOINT, 0x00
      };

      ID3D11Query* pQuery = nullptr;
      if (SUCCEEDED (dev->CreateQuery (&query_desc, &pQuery)))
      {
        CComPtr <ID3D11DeviceContext> pImmediateContext;
        dev->GetImmediateContext    (&pImmediateContext);

        InterlockedExchangePointer ((void **)&disjoint_query.async, pQuery);
        pImmediateContext->Begin                                   (pQuery);
        InterlockedExchange (&disjoint_query.active, TRUE);
      }
    }

    if (ReadAcquire (&disjoint_query.active))
    {
      // Start a new query
      D3D11_QUERY_DESC query_desc {
        D3D11_QUERY_TIMESTAMP, 0x00
      };

      duration_s duration;

      ID3D11Query* pQuery = nullptr;
      if (SUCCEEDED (dev->CreateQuery (&query_desc, &pQuery)))
      {
        InterlockedExchangePointer ((void **)&duration.start.dev_ctx, pDevContext);
        InterlockedExchangePointer ((void **)&duration.start.async,   pQuery);
        pDevContext->End                                             (pQuery);
        timers.emplace_back (duration);
      }
    }
  }
}

void
d3d11_shader_tracking_s::deactivate (ID3D11DeviceContext* pDevCtx)
{
  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  UINT dev_idx =
    SK_D3D11_GetDeviceContextHandle (pDevCtx);

  bool is_active = active.get (dev_idx);

  if (is_active)
  {
    active.set (dev_idx, false);

    bool end_of_frame = false;

    if (pDevCtx == nullptr)
    {
      end_of_frame = true;
      pDevCtx      = (ID3D11DeviceContext *)rb.d3d11.immediate_ctx.p;
    }

    switch (type_)
    {
      case SK_D3D11_ShaderType::Vertex:
        SK_D3D11_Shaders.vertex.current.shader [dev_idx]   = 0x0;
        break;
      case SK_D3D11_ShaderType::Pixel:
        SK_D3D11_Shaders.pixel.current.shader  [dev_idx]    = 0x0;
        break;
      case SK_D3D11_ShaderType::Geometry:
        SK_D3D11_Shaders.geometry.current.shader [dev_idx] = 0x0;
        break;
      case SK_D3D11_ShaderType::Domain:
        SK_D3D11_Shaders.domain.current.shader [dev_idx]   = 0x0;
        break;
      case SK_D3D11_ShaderType::Hull:
        SK_D3D11_Shaders.hull.current.shader [dev_idx]     = 0x0;
        break;
      case SK_D3D11_ShaderType::Compute:
        SK_D3D11_Shaders.compute.current.shader [dev_idx]  = 0x0;
        break;
    }

    if (end_of_frame) return;
  }

  else
    return;


  // Timing is very difficult on deferred contexts; will finish later (years?)
  if ( pDevCtx != nullptr && pDevCtx->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED )
    return;


  CComPtr <ID3D11Device> dev = nullptr;

  if ( pDevCtx              != nullptr                             &&
                  rb.device != nullptr                             &&
       SUCCEEDED (rb.device->QueryInterface <ID3D11Device> (&dev)) &&
                  rb.device.IsEqualObject                  ( dev)  &&

         ReadAcquire (&disjoint_query.active) )
  {
    D3D11_QUERY_DESC query_desc {
      D3D11_QUERY_TIMESTAMP, 0x00
    };

    duration_s& duration =
      timers.back ();

                                      ID3D11Query* pQuery = nullptr;
    if (SUCCEEDED (dev->CreateQuery (&query_desc, &pQuery)))
    {
      InterlockedExchangePointer ((void **)&duration.end.dev_ctx, pDevCtx);
      InterlockedExchangePointer ((void **)&duration.end.async, pQuery);
                                                  pDevCtx->End (pQuery);
    }
  }
}

__forceinline
void
d3d11_shader_tracking_s::use (IUnknown* pShader)
{
  UNREFERENCED_PARAMETER (pShader);

  num_draws++;
}


bool drawing_cpl = false;


NvAPI_Status
__cdecl
NvAPI_D3D11_CreateVertexShaderEx_Override ( __in        ID3D11Device *pDevice,        __in     const void                *pShaderBytecode, 
                                            __in        SIZE_T        BytecodeLength, __in_opt       ID3D11ClassLinkage  *pClassLinkage, 
                                            __in  const LPVOID                                                            pCreateVertexShaderExArgs,
                                            __out       ID3D11VertexShader                                              **ppVertexShader )
{
  SK_D3D11_ShaderThreads.mark ();


  NvAPI_Status ret =
    NvAPI_D3D11_CreateVertexShaderEx_Original ( pDevice,
                                                  pShaderBytecode, BytecodeLength,
                                                    pClassLinkage, pCreateVertexShaderExArgs,
                                                      ppVertexShader );

  if (ret == NVAPI_OK)
  {
    uint32_t checksum =
      crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);

    cs_shader_vs.lock ();

    if (! SK_D3D11_Shaders.vertex.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;

      desc.type   = SK_D3D11_ShaderType::Vertex;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
      {
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);
      }

      SK_D3D11_Shaders.vertex.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.vertex.rev.count (*ppVertexShader) &&
               SK_D3D11_Shaders.vertex.rev [*ppVertexShader] != checksum )
      SK_D3D11_Shaders.vertex.rev.erase (*ppVertexShader);

    SK_D3D11_Shaders.vertex.rev.emplace (std::make_pair (*ppVertexShader, checksum));

    SK_D3D11_ShaderDesc& desc =
      SK_D3D11_Shaders.vertex.descs [checksum];

    cs_shader_vs.unlock ();

    InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
                _time64 (&desc.usage.last_time);
  }

  return ret;
}

NvAPI_Status
__cdecl
NvAPI_D3D11_CreateHullShaderEx_Override ( __in        ID3D11Device *pDevice,        __in const void               *pShaderBytecode, 
                                          __in        SIZE_T        BytecodeLength, __in_opt   ID3D11ClassLinkage *pClassLinkage, 
                                          __in  const LPVOID                                                       pCreateHullShaderExArgs,
                                          __out       ID3D11HullShader                                           **ppHullShader )
{
  SK_D3D11_ShaderThreads.mark ();


  NvAPI_Status ret =
    NvAPI_D3D11_CreateHullShaderEx_Original ( pDevice,
                                                pShaderBytecode, BytecodeLength,
                                                  pClassLinkage, pCreateHullShaderExArgs,
                                                    ppHullShader );

  if (ret == NVAPI_OK)
  {
    uint32_t checksum =
      crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);

    cs_shader_hs.lock ();

    if (! SK_D3D11_Shaders.hull.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Hull;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
      {
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);
      }

      SK_D3D11_Shaders.hull.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.hull.rev.count (*ppHullShader) &&
               SK_D3D11_Shaders.hull.rev [*ppHullShader] != checksum )
      SK_D3D11_Shaders.hull.rev.erase (*ppHullShader);

    SK_D3D11_Shaders.hull.rev.emplace (std::make_pair (*ppHullShader, checksum));

    SK_D3D11_ShaderDesc& desc =
      SK_D3D11_Shaders.hull.descs [checksum];

    cs_shader_hs.unlock ();

    InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
                _time64 (&desc.usage.last_time);
  }

  return ret;
}

NvAPI_Status
__cdecl
NvAPI_D3D11_CreateDomainShaderEx_Override ( __in        ID3D11Device *pDevice,        __in     const void               *pShaderBytecode, 
                                            __in        SIZE_T        BytecodeLength, __in_opt       ID3D11ClassLinkage *pClassLinkage, 
                                            __in  const LPVOID                                                           pCreateDomainShaderExArgs,
                                            __out       ID3D11DomainShader                                             **ppDomainShader )
{
  SK_D3D11_ShaderThreads.mark ();


  NvAPI_Status ret =
    NvAPI_D3D11_CreateDomainShaderEx_Original ( pDevice,
                                                  pShaderBytecode, BytecodeLength,
                                                    pClassLinkage, pCreateDomainShaderExArgs,
                                                      ppDomainShader );

  if (ret == NVAPI_OK)
  {
    uint32_t checksum =
      crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);

    cs_shader_ds.lock ();

    if (! SK_D3D11_Shaders.domain.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Domain;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
      {
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);
      }

      SK_D3D11_Shaders.domain.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.domain.rev.count (*ppDomainShader) &&
               SK_D3D11_Shaders.domain.rev [*ppDomainShader] != checksum )
      SK_D3D11_Shaders.domain.rev.erase (*ppDomainShader);

    SK_D3D11_Shaders.domain.rev.emplace (std::make_pair (*ppDomainShader, checksum));

    SK_D3D11_ShaderDesc& desc =
      SK_D3D11_Shaders.domain.descs [checksum];

    cs_shader_ds.unlock ();

    InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
                _time64 (&desc.usage.last_time);
  }

  return ret;
}

NvAPI_Status
__cdecl
NvAPI_D3D11_CreateGeometryShaderEx_2_Override ( __in        ID3D11Device *pDevice,        __in     const void               *pShaderBytecode, 
                                                __in        SIZE_T        BytecodeLength, __in_opt       ID3D11ClassLinkage *pClassLinkage, 
                                                __in  const LPVOID                                                           pCreateGeometryShaderExArgs,
                                                __out       ID3D11GeometryShader                                           **ppGeometryShader )
{
  SK_D3D11_ShaderThreads.mark ();


  NvAPI_Status ret =
    NvAPI_D3D11_CreateGeometryShaderEx_2_Original ( pDevice,
                                                      pShaderBytecode, BytecodeLength,
                                                        pClassLinkage, pCreateGeometryShaderExArgs,
                                                          ppGeometryShader );

  if (ret == NVAPI_OK)
  {
    uint32_t checksum =
      crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);

    cs_shader_gs.lock ();

    if (! SK_D3D11_Shaders.geometry.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Geometry;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
      {
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);
      }

      SK_D3D11_Shaders.geometry.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.geometry.rev.count (*ppGeometryShader) &&
               SK_D3D11_Shaders.geometry.rev [*ppGeometryShader] != checksum )
      SK_D3D11_Shaders.geometry.rev.erase (*ppGeometryShader);

    SK_D3D11_Shaders.geometry.rev.emplace (std::make_pair (*ppGeometryShader, checksum));

    SK_D3D11_ShaderDesc& desc =
      SK_D3D11_Shaders.geometry.descs [checksum];

    cs_shader_gs.unlock ();

    InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
                _time64 (&desc.usage.last_time);
  }

  return ret;
}

NvAPI_Status
__cdecl
NvAPI_D3D11_CreateFastGeometryShaderExplicit_Override ( __in        ID3D11Device *pDevice,        __in     const void               *pShaderBytecode,
                                                        __in        SIZE_T        BytecodeLength, __in_opt       ID3D11ClassLinkage *pClassLinkage,
                                                        __in  const LPVOID                                                           pCreateFastGSArgs,
                                                        __out       ID3D11GeometryShader                                           **ppGeometryShader )
{
  SK_D3D11_ShaderThreads.mark ();


  NvAPI_Status ret =
    NvAPI_D3D11_CreateFastGeometryShaderExplicit_Original ( pDevice,
                                                              pShaderBytecode, BytecodeLength,
                                                                pClassLinkage, pCreateFastGSArgs,
                                                                  ppGeometryShader );

  if (ret == NVAPI_OK)
  {
    uint32_t checksum =
      crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);

    cs_shader_gs.lock ();

    if (! SK_D3D11_Shaders.geometry.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Geometry;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
      {
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);
      }

      SK_D3D11_Shaders.geometry.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.geometry.rev.count (*ppGeometryShader) &&
               SK_D3D11_Shaders.geometry.rev [*ppGeometryShader] != checksum )
      SK_D3D11_Shaders.geometry.rev.erase (*ppGeometryShader);

    SK_D3D11_Shaders.geometry.rev.emplace (std::make_pair (*ppGeometryShader, checksum));

    SK_D3D11_ShaderDesc& desc =
      SK_D3D11_Shaders.geometry.descs [checksum];

    cs_shader_gs.unlock ();

    InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
                _time64 (&desc.usage.last_time);
  }

  return ret;
}

NvAPI_Status
__cdecl
NvAPI_D3D11_CreateFastGeometryShader_Override ( __in  ID3D11Device *pDevice,        __in     const void                *pShaderBytecode, 
                                                __in  SIZE_T        BytecodeLength, __in_opt       ID3D11ClassLinkage  *pClassLinkage,
                                                __out ID3D11GeometryShader                                            **ppGeometryShader )
{
  SK_D3D11_ShaderThreads.mark ();


  NvAPI_Status ret =
    NvAPI_D3D11_CreateFastGeometryShader_Original ( pDevice,
                                                      pShaderBytecode, BytecodeLength,
                                                        pClassLinkage, ppGeometryShader );

  if (ret == NVAPI_OK)
  {
    uint32_t checksum =
      crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);

    cs_shader_gs.lock ();

    if (! SK_D3D11_Shaders.geometry.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Geometry;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
      {
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);
      }

      SK_D3D11_Shaders.geometry.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.geometry.rev.count (*ppGeometryShader) &&
               SK_D3D11_Shaders.geometry.rev [*ppGeometryShader] != checksum )
      SK_D3D11_Shaders.geometry.rev.erase (*ppGeometryShader);

    SK_D3D11_Shaders.geometry.rev.emplace (std::make_pair (*ppGeometryShader, checksum));

    SK_D3D11_ShaderDesc& desc =
      SK_D3D11_Shaders.geometry.descs [checksum];

    cs_shader_gs.unlock ();

    InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
                _time64 (&desc.usage.last_time);
  }

  return ret;
}

uint32_t
__cdecl
SK_D3D11_ChecksumShaderBytecode ( _In_      const void                *pShaderBytecode,
                                  _In_            SIZE_T               BytecodeLength  )
{
  __try
  {
    return crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);
  }

  __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ) ? 
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
  {
    SK_LOG0 ( (L" >> Threw out disassembled shader due to access violation during hash."),
               L"   DXGI   ");
    return 0x00;
  }
}

__declspec (noinline)
HRESULT
SK_D3D11_CreateShader_Impl (
  _In_            ID3D11Device         *This,
  _In_      const void                 *pShaderBytecode,
  _In_            SIZE_T                BytecodeLength,
  _In_opt_        ID3D11ClassLinkage   *pClassLinkage,
  _Out_opt_       IUnknown            **ppShader,
                  sk_shader_class       type )
{
  // In debug builds, keep a tally of threads involved in shader management.
  //
  //   (Per-frame and lifetime statistics are tabulated)
  //
  //  ---------------------------------------------------------------------
  //
  //  * D3D11 devices are free-threaded and will perform resource creation
  //      from concurrent threads.
  //
  //   >> Ideally, we must keep our own locking to a minimum otherwise we
  //        can derail game's performance! <<
  //
  SK_D3D11_ShaderThreads.mark ();

  HRESULT hr =
    S_OK;

  auto GetResources =
  [&]( SK_Thread_CriticalSection**                        ppCritical,
       SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>** ppShaderDomain )
  {
    switch (type)
    {
      default:
      case sk_shader_class::Vertex:
        *ppCritical     = &cs_shader_vs;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.vertex
                          );
         break;
      case sk_shader_class::Pixel:
        *ppCritical     = &cs_shader_ps;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.pixel
                          );
         break;
      case sk_shader_class::Geometry:
        *ppCritical     = &cs_shader_gs;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.geometry
                          );
         break;
      case sk_shader_class::Domain:
        *ppCritical     = &cs_shader_ds;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.domain
                          );
         break;
      case sk_shader_class::Hull:
        *ppCritical     = &cs_shader_hs;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.hull
                          );
         break;
      case sk_shader_class::Compute:
        *ppCritical     = &cs_shader_cs;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.compute
                          );
         break;
    }
  };

  auto InvokeCreateRoutine =
  [&]
  {
    switch (type)
    {
      default:
      case sk_shader_class::Vertex:
        return
          D3D11Dev_CreateVertexShader_Original ( This, pShaderBytecode,
                                                   BytecodeLength, pClassLinkage,
                                                     (ID3D11VertexShader **)ppShader );
      case sk_shader_class::Pixel:
        return
          D3D11Dev_CreatePixelShader_Original ( This, pShaderBytecode,
                                                  BytecodeLength, pClassLinkage,
                                                    (ID3D11PixelShader **)ppShader );
      case sk_shader_class::Geometry:
        return
          D3D11Dev_CreateGeometryShader_Original ( This, pShaderBytecode,
                                                     BytecodeLength, pClassLinkage,
                                                       (ID3D11GeometryShader **)ppShader );
      case sk_shader_class::Domain:
        return
          D3D11Dev_CreateDomainShader_Original ( This, pShaderBytecode,
                                                   BytecodeLength, pClassLinkage,
                                                     (ID3D11DomainShader **)ppShader );
      case sk_shader_class::Hull:
        return
          D3D11Dev_CreateHullShader_Original ( This, pShaderBytecode,
                                                 BytecodeLength, pClassLinkage,
                                                   (ID3D11HullShader **)ppShader );
      case sk_shader_class::Compute:
        return
          D3D11Dev_CreateComputeShader_Original ( This, pShaderBytecode,
                                                    BytecodeLength, pClassLinkage,
                                                      (ID3D11ComputeShader **)ppShader );
    }
  };


  SK_Thread_CriticalSection*                        pCritical   = nullptr;
  SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* pShaderRepo = nullptr;

  GetResources (&pCritical, &pShaderRepo);


  if (SUCCEEDED (hr) && ppShader)
  {
    uint32_t checksum =
      SK_D3D11_ChecksumShaderBytecode (pShaderBytecode, BytecodeLength);

    // Checksum failure, just give the data to D3D11 and hope for the best
    if (checksum == 0x00)
    {
      hr = InvokeCreateRoutine ();
    }

    else
    {
      pCritical->lock ();     // Lock during cache check

      bool cached =
        pShaderRepo->descs.count (checksum) != 0;

      SK_D3D11_ShaderDesc* pCachedDesc = nullptr;

      if (! cached)
      {
        pCritical->unlock (); // Release lock during resource creation

        hr = InvokeCreateRoutine ();

        if (SUCCEEDED (hr))
        {
          SK_D3D11_ShaderDesc desc;

          switch (type)
          {
            default:
            case sk_shader_class::Vertex:   desc.type = SK_D3D11_ShaderType::Vertex;   break;
            case sk_shader_class::Pixel:    desc.type = SK_D3D11_ShaderType::Pixel;    break;
            case sk_shader_class::Geometry: desc.type = SK_D3D11_ShaderType::Geometry; break;
            case sk_shader_class::Domain:   desc.type = SK_D3D11_ShaderType::Domain;   break;
            case sk_shader_class::Hull:     desc.type = SK_D3D11_ShaderType::Hull;     break;
            case sk_shader_class::Compute:  desc.type = SK_D3D11_ShaderType::Compute;  break;
          }

          desc.crc32c  =  checksum;
          desc.pShader = *ppShader;

          desc.bytecode.reserve (BytecodeLength);

          for (UINT i = 0; i < BytecodeLength; i++)
          {
            desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);
          }

          pCritical->lock (); // Re-acquire before cache manipulation

          // Concurrent shader creation resulted in the same shader twice,
          //   release this one and use the one currently in cache.
          //
          //  (nb: DOES NOT ACCOUNT FOR ALT. SUBROUTINES [Class Linkage])
          //
          if (pShaderRepo->descs.count (checksum) != 0)
          {
            ((IUnknown *)*ppShader)->Release ();

            SK_LOG0 ( (L"Discarding Concurrent Shader Cache Collision for %x",
                         checksum ), L"DX11Shader" );
          }

          else
            pShaderRepo->descs.emplace (std::make_pair (checksum, desc));

          pCachedDesc = &pShaderRepo->descs [checksum];
        }
      }

      // Cache hit
      else
      {
        //
        // NOTE:
        //
        //   Because of this caching system, we alias some render passes that
        //     could ordinarily be distinguished because they use two different
        //       instances of the same shader.
        //
        //  * Consider a tagging system to prevent aliasing in the future.
        //
        pCachedDesc = &pShaderRepo->descs [checksum];

        SK_LOG3 ( ( L"Shader Class %lu Cache Hit for Checksum %08x", type, checksum ),
                    L"DX11Shader" );
      }

      if (pCachedDesc != nullptr)
      {
         *ppShader = (IUnknown *)pCachedDesc->pShader;
        (*ppShader)->AddRef ();

        // XXX: Creation does not imply usage
        //
        //InterlockedExchange (&pCachedDesc->usage.last_frame, SK_GetFramesDrawn ());
        //            _time64 (&pCachedDesc->usage.last_time);

        // Update cache mapping (see note about aliasing above)
        ///if ( pShaderRepo->rev.count (*ppShader) &&
        ///           pShaderRepo->rev [*ppShader] != checksum )
        ///  pShaderRepo->rev.erase (*ppShader);

        pShaderRepo->rev.emplace (std::make_pair (*ppShader, checksum));

        pCritical->unlock ();
      }
    }
  }

  return hr;
}


__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateVertexShader_Override (
  _In_            ID3D11Device        *This,
  _In_      const void                *pShaderBytecode,
  _In_            SIZE_T               BytecodeLength,
  _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
  _Out_opt_       ID3D11VertexShader **ppVertexShader )
{
  return
    SK_D3D11_CreateShader_Impl ( This,
                                   pShaderBytecode, BytecodeLength,
                                     pClassLinkage,
                                       (IUnknown **)ppVertexShader,
                                         sk_shader_class::Vertex );
}

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreatePixelShader_Override (
  _In_            ID3D11Device        *This,
  _In_      const void                *pShaderBytecode,
  _In_            SIZE_T               BytecodeLength,
  _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
  _Out_opt_       ID3D11PixelShader  **ppPixelShader )
{
  return
    SK_D3D11_CreateShader_Impl ( This,
                                   pShaderBytecode, BytecodeLength,
                                     pClassLinkage,
                                       (IUnknown **)ppPixelShader,
                                         sk_shader_class::Pixel );
}

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateGeometryShader_Override (
  _In_            ID3D11Device          *This,
  _In_      const void                  *pShaderBytecode,
  _In_            SIZE_T                 BytecodeLength,
  _In_opt_        ID3D11ClassLinkage    *pClassLinkage,
  _Out_opt_       ID3D11GeometryShader **ppGeometryShader )
{
  return
    SK_D3D11_CreateShader_Impl ( This,
                                   pShaderBytecode, BytecodeLength,
                                     pClassLinkage,
                                       (IUnknown **)ppGeometryShader,
                                         sk_shader_class::Geometry );
}

__declspec (noinline)
HRESULT
WINAPI
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
  _Out_opt_       ID3D11GeometryShader       **ppGeometryShader )
{
  HRESULT hr =
    D3D11Dev_CreateGeometryShaderWithStreamOutput_Original ( This, pShaderBytecode,
                                                               BytecodeLength,
                                                                 pSODeclaration, NumEntries,
                                                                   pBufferStrides, NumStrides,
                                                                     RasterizedStream, pClassLinkage,
                                                                       ppGeometryShader );

  if (SUCCEEDED (hr) && ppGeometryShader)
  {
    uint32_t checksum =
      SK_D3D11_ChecksumShaderBytecode (pShaderBytecode, BytecodeLength);

    if (checksum == 0x00)
      checksum = (uint32_t)BytecodeLength;

    cs_shader_gs.lock ();

    if (! SK_D3D11_Shaders.geometry.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Geometry;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
      {
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);
      }

      SK_D3D11_Shaders.geometry.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.geometry.rev.count (*ppGeometryShader) &&
               SK_D3D11_Shaders.geometry.rev [*ppGeometryShader] != checksum )
      SK_D3D11_Shaders.geometry.rev.erase (*ppGeometryShader);

    SK_D3D11_Shaders.geometry.rev.emplace (std::make_pair (*ppGeometryShader, checksum));

    SK_D3D11_ShaderDesc& desc =
      SK_D3D11_Shaders.geometry.descs [checksum];

    cs_shader_gs.unlock ();

    InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
                _time64 (&desc.usage.last_time);
  }

  return hr;
}

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateHullShader_Override (
  _In_            ID3D11Device        *This,
  _In_      const void                *pShaderBytecode,
  _In_            SIZE_T               BytecodeLength,
  _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
  _Out_opt_       ID3D11HullShader   **ppHullShader )
{
  return
    SK_D3D11_CreateShader_Impl ( This,
                                   pShaderBytecode, BytecodeLength,
                                     pClassLinkage,
                                       (IUnknown **)ppHullShader,
                                         sk_shader_class::Hull );
}

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateDomainShader_Override (
  _In_            ID3D11Device        *This,
  _In_      const void                *pShaderBytecode,
  _In_            SIZE_T               BytecodeLength,
  _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
  _Out_opt_       ID3D11DomainShader **ppDomainShader )
{
  return
    SK_D3D11_CreateShader_Impl ( This,
                                   pShaderBytecode, BytecodeLength,
                                     pClassLinkage,
                                       (IUnknown **)ppDomainShader,
                                         sk_shader_class::Domain );
}

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateComputeShader_Override (
  _In_            ID3D11Device         *This,
  _In_      const void                 *pShaderBytecode,
  _In_            SIZE_T                BytecodeLength,
  _In_opt_        ID3D11ClassLinkage   *pClassLinkage,
  _Out_opt_       ID3D11ComputeShader **ppComputeShader )
{
  return
    SK_D3D11_CreateShader_Impl ( This,
                                   pShaderBytecode, BytecodeLength,
                                     pClassLinkage,
                                       (IUnknown **)ppComputeShader,
                                         sk_shader_class::Compute );
}


__forceinline
void
SK_D3D11_SetShader_Impl ( ID3D11DeviceContext* pDevCtx, IUnknown* pShader, sk_shader_class type, ID3D11ClassInstance *const *ppClassInstances,
                                                                                                 UINT                        NumClassInstances )
{
  auto Finish =
  [&]
  {
    switch (type)
    {
      case sk_shader_class::Vertex:
        return D3D11_VSSetShader_Original ( pDevCtx, (ID3D11VertexShader *)pShader,
                                              ppClassInstances, NumClassInstances );
      case sk_shader_class::Pixel:
        return D3D11_PSSetShader_Original ( pDevCtx, (ID3D11PixelShader *)pShader,
                                              ppClassInstances, NumClassInstances );
      case sk_shader_class::Geometry:
        return D3D11_GSSetShader_Original ( pDevCtx, (ID3D11GeometryShader *)pShader,
                                              ppClassInstances, NumClassInstances );
      case sk_shader_class::Domain:
        return D3D11_DSSetShader_Original ( pDevCtx, (ID3D11DomainShader *)pShader,
                                              ppClassInstances, NumClassInstances );
      case sk_shader_class::Hull:
        return D3D11_HSSetShader_Original ( pDevCtx, (ID3D11HullShader *)pShader,
                                              ppClassInstances, NumClassInstances );
      case sk_shader_class::Compute:
        return D3D11_CSSetShader_Original ( pDevCtx, (ID3D11ComputeShader *)pShader,
                                              ppClassInstances, NumClassInstances );
    }
  };


  ///if (! SK_D3D11_ShouldTrackRenderOp (pDevCtx))
  ///  return Finish ();


  auto GetResources =
  [&]( SK_Thread_CriticalSection**                        ppCritical,
       SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>** ppShaderDomain )
  {
    switch (type)
    {
      default:
      case sk_shader_class::Vertex:
        *ppCritical     = &cs_shader_vs;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.vertex
                          );
         break;
      case sk_shader_class::Pixel:
        *ppCritical     = &cs_shader_ps;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.pixel
                          );
         break;
      case sk_shader_class::Geometry:
        *ppCritical     = &cs_shader_gs;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.geometry
                          );
         break;
      case sk_shader_class::Domain:
        *ppCritical     = &cs_shader_ds;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.domain
                          );
         break;
      case sk_shader_class::Hull:
        *ppCritical     = &cs_shader_hs;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.hull
                          );
         break;
      case sk_shader_class::Compute:
        *ppCritical     = &cs_shader_cs;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.compute
                          );
         break;
    }
  };


  SK_Thread_CriticalSection*                        pCritical   = nullptr;
  SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* pShaderRepo = nullptr;

  GetResources (&pCritical, &pShaderRepo);

  // ImGui gets to pass-through without invoking the hook
  if (! SK_D3D11_ShouldTrackRenderOp (pDevCtx))
  {
    pShaderRepo->tracked.deactivate (pDevCtx);
  
    return Finish ();
  }


  SK_D3D11_ShaderDesc *pDesc = nullptr;
  if (pShader != nullptr)
  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock (*pCritical);

    if (pShaderRepo->rev.count (pShader))
      pDesc = &pShaderRepo->descs [pShaderRepo->rev [pShader]];
  }

  UINT dev_idx =
    SK_D3D11_GetDeviceContextHandle (pDevCtx);

  if (pDesc != nullptr)
  {
    InterlockedIncrement (&pShaderRepo->changes_last_frame);

    uint32_t checksum =
      pDesc->crc32c;

    pShaderRepo->current.shader [dev_idx] = checksum;

    InterlockedExchange (&pDesc->usage.last_frame, SK_GetFramesDrawn ());
    _time64             (&pDesc->usage.last_time);

    if (checksum == pShaderRepo->tracked.crc32c)
    {
      pShaderRepo->tracked.activate (pDevCtx, ppClassInstances, NumClassInstances);

      return Finish ();
    }
  }

  else
  {
    pShaderRepo->current.shader [dev_idx] = 0x0;
  }

  pShaderRepo->tracked.deactivate (pDevCtx);

  return Finish ();
}


__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_VSSetShader_Override (
 _In_     ID3D11DeviceContext        *This,
 _In_opt_ ID3D11VertexShader         *pVertexShader,
 _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
          UINT                        NumClassInstances )
{
  return
    SK_D3D11_SetShader_Impl ( This, pVertexShader,
                                sk_shader_class::Vertex,
                                  ppClassInstances, NumClassInstances );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_VSGetShader_Override (
 _In_        ID3D11DeviceContext  *This,
 _Out_       ID3D11VertexShader  **ppVertexShader,
 _Out_opt_   ID3D11ClassInstance **ppClassInstances,
 _Inout_opt_ UINT                 *pNumClassInstances )
{
  return D3D11_VSGetShader_Original ( This, ppVertexShader,
                                        ppClassInstances, pNumClassInstances );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_PSSetShader_Override (
 _In_     ID3D11DeviceContext        *This,
 _In_opt_ ID3D11PixelShader          *pPixelShader,
 _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
          UINT                        NumClassInstances )
{
  return
    SK_D3D11_SetShader_Impl ( This, pPixelShader,
                                sk_shader_class::Pixel,
                                  ppClassInstances, NumClassInstances );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_PSGetShader_Override (
 _In_        ID3D11DeviceContext  *This,
 _Out_       ID3D11PixelShader   **ppPixelShader,
 _Out_opt_   ID3D11ClassInstance **ppClassInstances,
 _Inout_opt_ UINT                 *pNumClassInstances )
{
  return D3D11_PSGetShader_Original ( This, ppPixelShader,
                                        ppClassInstances, pNumClassInstances );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_GSSetShader_Override (
 _In_     ID3D11DeviceContext        *This,
 _In_opt_ ID3D11GeometryShader       *pGeometryShader,
 _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
          UINT                        NumClassInstances )
{
  return
    SK_D3D11_SetShader_Impl ( This, pGeometryShader,
                                sk_shader_class::Geometry,
                                  ppClassInstances, NumClassInstances );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_GSGetShader_Override (
 _In_        ID3D11DeviceContext   *This,
 _Out_       ID3D11GeometryShader **ppGeometryShader,
 _Out_opt_   ID3D11ClassInstance  **ppClassInstances,
 _Inout_opt_ UINT                  *pNumClassInstances )
{
  return D3D11_GSGetShader_Original (This, ppGeometryShader, ppClassInstances, pNumClassInstances);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_HSSetShader_Override (
 _In_     ID3D11DeviceContext        *This,
 _In_opt_ ID3D11HullShader           *pHullShader,
 _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
          UINT                        NumClassInstances )
{
  return
    SK_D3D11_SetShader_Impl ( This, pHullShader,
                                sk_shader_class::Hull,
                                  ppClassInstances, NumClassInstances );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_HSGetShader_Override (
 _In_        ID3D11DeviceContext  *This,
 _Out_       ID3D11HullShader    **ppHullShader,
 _Out_opt_   ID3D11ClassInstance **ppClassInstances,
 _Inout_opt_ UINT                 *pNumClassInstances )
{
  return D3D11_HSGetShader_Original ( This, ppHullShader,
                                        ppClassInstances, pNumClassInstances );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DSSetShader_Override (
 _In_     ID3D11DeviceContext        *This,
 _In_opt_ ID3D11DomainShader         *pDomainShader,
 _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
          UINT                        NumClassInstances )
{
  return
    SK_D3D11_SetShader_Impl ( This, pDomainShader,
                                sk_shader_class::Domain,
                                  ppClassInstances, NumClassInstances );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DSGetShader_Override (
 _In_        ID3D11DeviceContext  *This,
 _Out_       ID3D11DomainShader  **ppDomainShader,
 _Out_opt_   ID3D11ClassInstance **ppClassInstances,
 _Inout_opt_ UINT                 *pNumClassInstances )
{
  return D3D11_DSGetShader_Original ( This, ppDomainShader,
                                        ppClassInstances, pNumClassInstances );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CSSetShader_Override (
 _In_     ID3D11DeviceContext        *This,
 _In_opt_ ID3D11ComputeShader        *pComputeShader,
 _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
          UINT                        NumClassInstances )
{
  return
    SK_D3D11_SetShader_Impl ( This, pComputeShader,
                                sk_shader_class::Compute,
                                  ppClassInstances, NumClassInstances );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CSGetShader_Override (
 _In_        ID3D11DeviceContext  *This,
 _Out_       ID3D11ComputeShader **ppComputeShader,
 _Out_opt_   ID3D11ClassInstance **ppClassInstances,
 _Inout_opt_ UINT                 *pNumClassInstances )
{
  return
    D3D11_CSGetShader_Original ( This, ppComputeShader,
                                   ppClassInstances, pNumClassInstances );
}

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


__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_RSSetScissorRects_Override (
        ID3D11DeviceContext *This,
        UINT                 NumRects,
  const D3D11_RECT          *pRects )
{
  return D3D11_RSSetScissorRects_Original (This, NumRects, pRects);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_VSSetConstantBuffers_Override (
  ID3D11DeviceContext*  This,
  UINT                  StartSlot,
  UINT                  NumBuffers,
  ID3D11Buffer *const  *ppConstantBuffers )
{
  //dll_log.Log (L"[   DXGI   ] [!]D3D11_VSSetConstantBuffers (%lu, %lu, ...)", StartSlot, NumBuffers);
  return D3D11_VSSetConstantBuffers_Original (This, StartSlot, NumBuffers, ppConstantBuffers );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_PSSetConstantBuffers_Override (
  ID3D11DeviceContext*  This,
  UINT                  StartSlot,
  UINT                  NumBuffers,
  ID3D11Buffer *const  *ppConstantBuffers )
{
  //dll_log.Log (L"[   DXGI   ] [!]D3D11_VSSetConstantBuffers (%lu, %lu, ...)", StartSlot, NumBuffers);
  return D3D11_PSSetConstantBuffers_Original (This, StartSlot, NumBuffers, ppConstantBuffers );
}

std::unordered_set <ID3D11Texture2D *> used_textures;

struct shader_stage_s {
  void nulBind (int dev_ctx_handle, int slot, ID3D11ShaderResourceView* pView)
  {
    if (skipped_bindings [dev_ctx_handle][slot] != nullptr)
    {
      skipped_bindings [dev_ctx_handle][slot]->Release ();
      skipped_bindings [dev_ctx_handle][slot] = nullptr;
    }

    skipped_bindings [dev_ctx_handle][slot] = pView;

    if (pView != nullptr)
    {
      pView->AddRef ();
    }
  };

  void Bind (int dev_ctx_handle, int slot, ID3D11ShaderResourceView* pView)
  {
    if (skipped_bindings [dev_ctx_handle][slot] != nullptr)
    {
      skipped_bindings [dev_ctx_handle][slot]->Release ();
      skipped_bindings [dev_ctx_handle][slot] = nullptr;
    }

    skipped_bindings [dev_ctx_handle][slot] = nullptr;

    // The D3D11 Runtime is holding a reference if this is non-null.
    real_bindings    [dev_ctx_handle][slot] = pView;
  };

  // We have to hold references to these things, because the D3D11 runtime would have.
  std::array <std::array <ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT>, SK_D3D11_MAX_DEV_CONTEXTS+1> skipped_bindings = { };
  std::array <std::array <ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT>, SK_D3D11_MAX_DEV_CONTEXTS+1> real_bindings    = { };
};

std::array <shader_stage_s, SK_D3D11_MAX_DEV_CONTEXTS+1>*
SK_D3D11_GetShaderStages (void)
{
  static std::array <shader_stage_s, SK_D3D11_MAX_DEV_CONTEXTS+1> _stages [6];
  return                                                          _stages;
}


struct SK_D3D11_ContextResources
{
  std::set <ID3D11Texture2D *> used_textures;
  std::set <IUnknown        *> temp_resources;

  volatile LONG                writing_ = 0;
  UINT                         ctx_id_  = 0;
};

std::array < SK_D3D11_ContextResources, SK_D3D11_MAX_DEV_CONTEXTS+1 >
                     SK_D3D11_PerCtxResources;

#define d3d11_shader_stages SK_D3D11_GetShaderStages ()


__forceinline
bool
SK_D3D11_ActivateSRVOnSlot ( UINT                      dev_ctx_handle,
                             shader_stage_s&           stage,
                             ID3D11ShaderResourceView* pSRV,
                             int                       SLOT )
{
  if (! pSRV)
  {
    stage.Bind (dev_ctx_handle, SLOT, pSRV);

    return true;
  }

  D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = { };

  pSRV->GetDesc (&srv_desc);

  CComPtr <ID3D11Resource>  pRes = nullptr;
  ID3D11Texture2D*          pTex = nullptr;

  if (srv_desc.ViewDimension == D3D_SRV_DIMENSION_TEXTURE2D)
  {
    pSRV->GetResource (&pRes);

    if (pRes.p != nullptr && SUCCEEDED (pRes->QueryInterface <ID3D11Texture2D> (&pTex)))
    {
      int spins = 0;

      while (InterlockedCompareExchange (&SK_D3D11_PerCtxResources [dev_ctx_handle].writing_, 1, 0) != 0)
      {
        if ( ++spins > 0x1000 )
        {
          SleepEx (1, FALSE);
          spins = 0;
        }
      }

      if (SK_D3D11_PerCtxResources [dev_ctx_handle].used_textures.emplace  (pTex).second)
          SK_D3D11_PerCtxResources [dev_ctx_handle].temp_resources.emplace (pTex);

      InterlockedExchange (&SK_D3D11_PerCtxResources [dev_ctx_handle].writing_, 0);

      if (tracked_texture == pTex && config.textures.d3d11.highlight_debug)
      {
        if (dwFrameTime % tracked_tex_blink_duration > tracked_tex_blink_duration / 2)
        {
          stage.nulBind (dev_ctx_handle, SLOT, pSRV);
          return false;
        }
      }
    }
  }

  stage.Bind (dev_ctx_handle, SLOT, pSRV);

  return true;
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_VSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews )
{
#if 0
  // ImGui gets to pass-through without invoking the hook
  if (((! config.render.dxgi.deferred_isolation) && This->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED) || SK_TLS_Bottom ()->imgui.drawing || (! SK_D3D11_EnableTracking))
    return D3D11_VSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  auto&& newResourceViews = SK_TLS_Bottom ()->d3d11.newResourceViews;

  RtlZeroMemory (newResourceViews, sizeof (ID3D11ShaderResourceView*) * NumViews);

  if (ppShaderResourceViews && NumViews > 0)
  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_shader_vs);

    auto&&          views = SK_D3D11_Shaders.vertex.current.views [This];
    shader_stage_s& stage = d3d11_shader_stages [0][This];

    d3d11_shader_tracking_s& tracked =
      SK_D3D11_Shaders.vertex.tracked;

    uint32_t shader_crc32c = tracked.crc32c.load ();
    bool     active        = tracked.active.load ();

    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRVOnSlot (stage, ppShaderResourceViews [i], StartSlot + i))
        newResourceViews [i] =               ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;

      if (shader_crc32c != 0 && active && ppShaderResourceViews [i])            {
        if (! tracked.set_of_views.count (ppShaderResourceViews [i]))           {
            tracked.set_of_views.emplace (ppShaderResourceViews [i]);
                                          ppShaderResourceViews [i]->AddRef (); }
            tracked.used_views.push_back (ppShaderResourceViews [i]);           }
                  views [StartSlot + i] = ppShaderResourceViews [i];
    }
  }

  else if (NumViews == 0 && ppShaderResourceViews == nullptr)
  {
    auto&&          views = SK_D3D11_Shaders.vertex.current.views [This];
    shader_stage_s& stage = d3d11_shader_stages [0][This];

    for (UINT i = 0; i < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; i++)
    {
      SK_D3D11_ActivateSRVOnSlot (stage, nullptr, i);

      newResourceViews [i] = nullptr;
      views            [i] = nullptr;
    }
  }

  D3D11_VSSetShaderResources_Original (This, StartSlot, NumViews, newResourceViews);
#else
  constexpr int VERTEX_SHADER_STAGE = 1;

  SK_TLS* pTLS = nullptr;

  // ImGui gets to pass-through without invoking the hook
  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
    return D3D11_VSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  auto&& newResourceViews = pTLS->d3d11.newResourceViews;

  RtlZeroMemory (newResourceViews, sizeof (ID3D11ShaderResourceView*) * NumViews);

  if (ppShaderResourceViews && NumViews > 0)
  {
    UINT dev_idx =
      SK_D3D11_GetDeviceContextHandle (This);

    auto&& views =
          SK_D3D11_Shaders.vertex.current.views [dev_idx];
    auto&& stage =
      d3d11_shader_stages [VERTEX_SHADER_STAGE] [dev_idx];

    d3d11_shader_tracking_s& tracked =
      SK_D3D11_Shaders.vertex.tracked;

    uint32_t shader_crc32c = tracked.crc32c.load ();
    bool     active        = tracked.active.get  (dev_idx);

    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRVOnSlot (dev_idx, stage,
                                               ppShaderResourceViews [i], StartSlot + i))
        newResourceViews [i] =                 ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;
    }

    if (active && shader_crc32c != 0)
    {
      for (UINT i = 0; i < NumViews; i++)
      {
        if (ppShaderResourceViews [i])                                            {
          std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_shader_vs);
          if (tracked.set_of_views.emplace (ppShaderResourceViews [i]).second)    {
                                            ppShaderResourceViews [i]->AddRef (); }
           tracked.used_views.emplace_back (ppShaderResourceViews [i]);           }
                    views [StartSlot + i] = ppShaderResourceViews [i];
      }
    }
  }

  D3D11_VSSetShaderResources_Original (This, StartSlot, NumViews, newResourceViews);
#endif
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_PSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews )
{
  SK_TLS* pTLS = nullptr;

  constexpr int PIXEL_SHADER_STAGE = 1;

  // ImGui gets to pass-through without invoking the hook
  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
    return D3D11_PSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  auto&& newResourceViews = pTLS->d3d11.newResourceViews;

  RtlZeroMemory (newResourceViews, sizeof (ID3D11ShaderResourceView*) * NumViews);

  if (ppShaderResourceViews && NumViews > 0)
  {
    UINT dev_idx =
      SK_D3D11_GetDeviceContextHandle (This);

    auto&& views =
          SK_D3D11_Shaders.pixel.current.views [dev_idx];
    auto&& stage =
      d3d11_shader_stages [PIXEL_SHADER_STAGE] [dev_idx];

    d3d11_shader_tracking_s& tracked =
      SK_D3D11_Shaders.pixel.tracked;

    uint32_t shader_crc32c = tracked.crc32c.load ();
    bool     active        = tracked.active.get  (dev_idx);

  if (! ReadAcquire (&__SKX_ComputeAntiStall))
  {
    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRVOnSlot ( dev_idx, stage,
                                           ppShaderResourceViews [i], StartSlot + i))
        newResourceViews [i] =             ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;
    }

    if (active && shader_crc32c != 0)
    {
      for (UINT i = 0; i < NumViews; i++)
      {
        if (ppShaderResourceViews [i])                                            {
          std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_shader_ps);
          if (tracked.set_of_views.emplace (ppShaderResourceViews [i]).second)    {
                                            ppShaderResourceViews [i]->AddRef (); }
           tracked.used_views.emplace_back (ppShaderResourceViews [i]);           }
                    views [StartSlot + i] = ppShaderResourceViews [i];
      }
    }
  } else {
    for ( UINT i = StartSlot; i < StartSlot + NumViews; ++i )
    {
      if (SK_D3D11_ActivateSRVOnSlot (dev_idx, stage,
                                           ppShaderResourceViews [i - StartSlot], i))
        newResourceViews [i - StartSlot] = ppShaderResourceViews [i - StartSlot];
      else
        newResourceViews [i - StartSlot] = nullptr;
    }

    if (active && shader_crc32c != 0)
    {
      for ( UINT i = StartSlot; i < StartSlot + NumViews; ++i )
      {
        if (ppShaderResourceViews [i - StartSlot])                                            {
          std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_shader_ps);
          if (tracked.set_of_views.emplace (ppShaderResourceViews [i - StartSlot]).second)    {
                                            ppShaderResourceViews [i - StartSlot]->AddRef (); }
           tracked.used_views.emplace_back (ppShaderResourceViews [i - StartSlot]);           }
                                views [i] = ppShaderResourceViews [i - StartSlot];
      }
    }
  }
  }

  D3D11_PSSetShaderResources_Original (This, StartSlot, NumViews, newResourceViews);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_GSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews )
{
  constexpr int GEOMETRY_SHADER_STAGE = 2;

  SK_TLS *pTLS = nullptr;

  // ImGui gets to pass-through without invoking the hook
  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
    return D3D11_GSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  auto&& newResourceViews = pTLS->d3d11.newResourceViews;

  RtlZeroMemory (newResourceViews, sizeof (ID3D11ShaderResourceView*) * NumViews);

  if (ppShaderResourceViews && NumViews > 0)
  {
    UINT dev_idx =
      SK_D3D11_GetDeviceContextHandle (This);

    auto&& views =
          SK_D3D11_Shaders.geometry.current.views [dev_idx];
    auto&& stage =
      d3d11_shader_stages [GEOMETRY_SHADER_STAGE] [dev_idx];

    d3d11_shader_tracking_s& tracked =
      SK_D3D11_Shaders.geometry.tracked;

    uint32_t shader_crc32c = tracked.crc32c.load ();
    bool     active        = tracked.active.get  (dev_idx);

    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRVOnSlot (dev_idx, stage,
                                          ppShaderResourceViews [i], StartSlot + i))
        newResourceViews [i] =            ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;
    }

    if (active && shader_crc32c != 0)
    {
      for (UINT i = 0; i < NumViews; i++)
      {
        if (ppShaderResourceViews [i])                                            {
          std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_shader_gs);
          if (tracked.set_of_views.emplace (ppShaderResourceViews [i]).second)    {
                                            ppShaderResourceViews [i]->AddRef (); }
           tracked.used_views.emplace_back (ppShaderResourceViews [i]);           }
                    views [StartSlot + i] = ppShaderResourceViews [i];
      }
    }
  }

  D3D11_GSSetShaderResources_Original (This, StartSlot, NumViews, newResourceViews);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_HSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews )
{
  constexpr int HULL_SHADER_STAGE = 3;

  SK_TLS *pTLS = nullptr;

  // ImGui gets to pass-through without invoking the hook
  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
    return D3D11_HSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  auto&& newResourceViews = pTLS->d3d11.newResourceViews;

  RtlZeroMemory (newResourceViews, sizeof (ID3D11ShaderResourceView*) * NumViews);

  if (ppShaderResourceViews && NumViews > 0)
  {
    UINT dev_idx =
      SK_D3D11_GetDeviceContextHandle (This);

    auto&& views =
          SK_D3D11_Shaders.hull.current.views[dev_idx];
    auto&& stage =
      d3d11_shader_stages [HULL_SHADER_STAGE][dev_idx];

    d3d11_shader_tracking_s& tracked =
      SK_D3D11_Shaders.hull.tracked;

    uint32_t shader_crc32c = tracked.crc32c.load ();
    bool     active        = tracked.active.get  (dev_idx);

    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRVOnSlot (dev_idx, stage,
                                          ppShaderResourceViews [i], StartSlot + i))
        newResourceViews [i] =            ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;
    }

    if (active && shader_crc32c != 0)
    {
      for (UINT i = 0; i < NumViews; i++)
      {
        if (ppShaderResourceViews [i])                                            {
          std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_shader_hs);
          if (tracked.set_of_views.emplace (ppShaderResourceViews [i]).second)    {
                                            ppShaderResourceViews [i]->AddRef (); }
           tracked.used_views.emplace_back (ppShaderResourceViews [i]);           }
                    views [StartSlot + i] = ppShaderResourceViews [i];
      }
    }
  }

  D3D11_HSSetShaderResources_Original (This, StartSlot, NumViews, newResourceViews);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews )
{
  constexpr int DOMAIN_SHADER_STAGE = 4;

  SK_TLS *pTLS = nullptr;

  // ImGui gets to pass-through without invoking the hook
  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
    return D3D11_DSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  auto&& newResourceViews = pTLS->d3d11.newResourceViews;

  RtlZeroMemory (newResourceViews, sizeof (ID3D11ShaderResourceView*) * NumViews);

  if (ppShaderResourceViews && NumViews > 0)
  {
    UINT dev_idx =
      SK_D3D11_GetDeviceContextHandle (This);

    auto&& views =
          SK_D3D11_Shaders.domain.current.views[dev_idx];
    auto&& stage =
      d3d11_shader_stages [DOMAIN_SHADER_STAGE][dev_idx];

    d3d11_shader_tracking_s& tracked =
      SK_D3D11_Shaders.domain.tracked;

    uint32_t shader_crc32c = tracked.crc32c.load ();
    bool     active        = tracked.active.get  (dev_idx);

    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRVOnSlot (dev_idx, stage,
                                          ppShaderResourceViews [i], StartSlot + i))
        newResourceViews [i] =            ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;
    }

    if (active && shader_crc32c != 0)
    {
      for (UINT i = 0; i < NumViews; i++)
      {
        if (ppShaderResourceViews [i])                                            {
          std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_shader_ds);
          if (tracked.set_of_views.emplace (ppShaderResourceViews [i]).second)    {
                                            ppShaderResourceViews [i]->AddRef (); }
              tracked.used_views.push_back (ppShaderResourceViews [i]);           }
                    views [StartSlot + i] = ppShaderResourceViews [i];
      }
    }
  }

  D3D11_DSSetShaderResources_Original (This, StartSlot, NumViews, newResourceViews);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews )
{
  constexpr int COMPUTE_SHADER_STAGE = 5;

  SK_TLS *pTLS = nullptr;

  // ImGui gets to pass-through without invoking the hook
  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
    return D3D11_CSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  auto&& newResourceViews = pTLS->d3d11.newResourceViews;

  RtlZeroMemory (newResourceViews, sizeof (ID3D11ShaderResourceView*) * NumViews);

  if (ppShaderResourceViews && NumViews > 0)
  {
    UINT dev_idx =
      SK_D3D11_GetDeviceContextHandle (This);

    auto&& views =
          SK_D3D11_Shaders.compute.current.views[dev_idx];
    auto&& stage =
      d3d11_shader_stages [COMPUTE_SHADER_STAGE][dev_idx];

    d3d11_shader_tracking_s& tracked =
      SK_D3D11_Shaders.compute.tracked;

    uint32_t shader_crc32c = tracked.crc32c.load ();
    bool     active        = tracked.active.get  (dev_idx);

    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRVOnSlot (dev_idx, stage,
                                          ppShaderResourceViews [i], StartSlot + i))
        newResourceViews [i] =            ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;
    }

    if (active && shader_crc32c != 0)
    {
      for (UINT i = 0; i < NumViews; i++)
      {
        if (ppShaderResourceViews [i])                                            {
          std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_shader_cs);
          if (tracked.set_of_views.emplace (ppShaderResourceViews [i]).second)    {
                                            ppShaderResourceViews [i]->AddRef (); }
           tracked.used_views.emplace_back (ppShaderResourceViews [i]);           }
                    views [StartSlot + i] = ppShaderResourceViews [i];
      }
    }
  }

  D3D11_CSSetShaderResources_Original (This, StartSlot, NumViews, newResourceViews);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CSSetUnorderedAccessViews_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumUAVs,
  _In_opt_       ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
  _In_opt_ const UINT                             *pUAVInitialCounts )
{
  return D3D11_CSSetUnorderedAccessViews_Original (This, StartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
}



struct SK_D3D11_TEXTURE2D_DESC
{
  UINT                  Width;
  UINT                  Height;
  UINT                  MipLevels;
  UINT                  ArraySize;
  DXGI_FORMAT           Format;
  DXGI_SAMPLE_DESC      SampleDesc;
  D3D11_USAGE           Usage;
  D3D11_BIND_FLAG       BindFlags;
  D3D11_CPU_ACCESS_FLAG CPUAccessFlags;
  UINT                  MiscFlags;

  explicit SK_D3D11_TEXTURE2D_DESC (D3D11_TEXTURE2D_DESC& descFrom)
  {
    Width          = descFrom.Width;
    Height         = descFrom.Height;
    MipLevels      = descFrom.MipLevels;
    ArraySize      = descFrom.ArraySize;
    Format         = descFrom.Format;
    SampleDesc     = descFrom.SampleDesc;
    Usage          = descFrom.Usage;
    BindFlags      = (D3D11_BIND_FLAG      )descFrom.BindFlags;
    CPUAccessFlags = (D3D11_CPU_ACCESS_FLAG)descFrom.CPUAccessFlags;
    MiscFlags      = descFrom.MiscFlags;
  }
};

bool
SK_D3D11_IsStagingCacheable ( D3D11_RESOURCE_DIMENSION  rdim,
                              ID3D11Resource           *pRes,
                              SK_TLS                   *pTLS = nullptr
 )
{
  if ( config.textures.cache.allow_staging && pRes != nullptr &&
                                              rdim == D3D11_RESOURCE_DIMENSION_TEXTURE2D )
  {
    CComQIPtr <ID3D11Texture2D> pTex (pRes);

    if (pTex)
    {
      D3D11_TEXTURE2D_DESC tex_desc = { };
           pTex->GetDesc (&tex_desc);

      SK_D3D11_TEXTURE2D_DESC desc (tex_desc);

      if (desc.Usage != D3D11_USAGE_STAGING || desc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE)//desc.Width != 1920 && desc.Height != 1080)
      {
        if (pTLS == nullptr)
            pTLS = SK_TLS_Bottom ();

        if (! (pTLS->imgui.drawing || pTLS->texture_management.injection_thread))
          return true;
      }
    }
  }

  return false;
}

// Temporary staging for memory-mapped texture uploads
//
struct mapped_resources_s {
  std::unordered_map <ID3D11Resource*,  D3D11_MAPPED_SUBRESOURCE> textures;
  std::unordered_map <ID3D11Resource*,  uint64_t>                 texture_times;

  std::unordered_map <ID3D11Resource*,  uint32_t>                 dynamic_textures;
  std::unordered_map <ID3D11Resource*,  uint32_t>                 dynamic_texturesx;
  std::map           <uint32_t,         uint64_t>                 dynamic_times2;
  std::map           <uint32_t,         size_t>                   dynamic_sizes2;
};

std::unordered_map <ID3D11DeviceContext *, mapped_resources_s> mapped_resources;


__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_UpdateSubresource1_Override (
  _In_           ID3D11DeviceContext1 *This,
  _In_           ID3D11Resource       *pDstResource,
  _In_           UINT                  DstSubresource,
  _In_opt_ const D3D11_BOX            *pDstBox,
  _In_     const void                 *pSrcData,
  _In_           UINT                  SrcRowPitch,
  _In_           UINT                  SrcDepthPitch,
  _In_           UINT                  CopyFlags)
{
  bool early_out = false;

  SK_TLS *pTLS = nullptr;

  // UB: If it's happening, pretend we never saw this...
  if (pDstResource == nullptr || pSrcData == nullptr)
  {
    early_out = true;
  }

  else
    early_out = (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS));


  if (early_out)
  {
    return D3D11_UpdateSubresource1_Original ( This, pDstResource, DstSubresource,
                                                 pDstBox, pSrcData, SrcRowPitch,
                                                   SrcDepthPitch, CopyFlags );
  }

  dll_log.Log (L"UpdateSubresource1 ({%s}", DescribeResource (pDstResource).c_str ());


  //dll_log.Log (L"[   DXGI   ] [!]D3D11_UpdateSubresource1 (%ph, %lu, %ph, %ph, %lu, %lu, %x)",
  //          pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch, CopyFlags);

  D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
  pDstResource->GetType  (&rdim);

  if (SK_D3D11_IsStagingCacheable (rdim, pDstResource) && DstSubresource == 0)
  {
    CComQIPtr <ID3D11Texture2D> pTex (pDstResource);

    if (pTex != nullptr)
    {
      D3D11_TEXTURE2D_DESC desc = { };
           pTex->GetDesc (&desc);

      D3D11_SUBRESOURCE_DATA srd = { };

      srd.pSysMem           = pSrcData;
      srd.SysMemPitch       = SrcRowPitch;
      srd.SysMemSlicePitch  = 0;

      size_t   size         = 0;
      uint32_t top_crc32c   = 0x0;

      uint32_t checksum     =
        crc32_tex   (&desc, &srd, &size, &top_crc32c);

      uint32_t cache_tag    =
        safe_crc32c (top_crc32c, (uint8_t *)&desc, sizeof D3D11_TEXTURE2D_DESC);

      auto start            = SK_QueryPerf ().QuadPart;

      CComPtr <ID3D11Texture2D> pCachedTex =
        SK_D3D11_Textures.getTexture2D (cache_tag, &desc, nullptr, nullptr, pTLS);

      if (pCachedTex != nullptr)
      {
        CComQIPtr <ID3D11Resource> pCachedResource = pCachedTex;

        D3D11_CopyResource_Original (This, pDstResource, pCachedResource);

        SK_LOG1 ( ( L"Texture Cache Hit (Slow Path): (%lux%lu) -- %x",
                      desc.Width, desc.Height, top_crc32c ),
                    L"DX11TexMgr" );

        return;
      }

      else
      {
        if (SK_D3D11_TextureIsCached (pTex))
        {
          SK_LOG0 ( (L"Cached texture was updated (UpdateSubresource)... removing from cache! - <%s>",
                         SK_GetCallerName ().c_str ()), L"DX11TexMgr" );
          SK_D3D11_RemoveTexFromCache (pTex, true);
        }

        D3D11_UpdateSubresource_Original ( This, pDstResource, DstSubresource,
                                             pDstBox, pSrcData, SrcRowPitch,
                                               SrcDepthPitch );
        auto end     = SK_QueryPerf ().QuadPart;
        auto elapsed = end - start;

        if (desc.Usage == D3D11_USAGE_STAGING)
        {
          auto&& map_ctx = mapped_resources [This];

          map_ctx.dynamic_textures  [pDstResource] = checksum;
          map_ctx.dynamic_texturesx [pDstResource] = top_crc32c;

          SK_LOG1 ( ( L"New Staged Texture: (%lux%lu) -- %x",
                        desc.Width, desc.Height, top_crc32c ),
                      L"DX11TexMgr" );

          map_ctx.dynamic_times2    [checksum]  = elapsed;
          map_ctx.dynamic_sizes2    [checksum]  = size;

          return;
        }

        else
        {
          bool cacheable = ( desc.MiscFlags <= 4 &&
                             desc.Width      > 0 && 
                             desc.Height     > 0 &&
                             desc.ArraySize == 1 //||
                           //((desc.ArraySize  % 6 == 0) && (desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE))
                           );

          bool compressed = false;

          if ( (desc.Format >= DXGI_FORMAT_BC1_TYPELESS  &&
                desc.Format <= DXGI_FORMAT_BC5_SNORM)    ||
               (desc.Format >= DXGI_FORMAT_BC6H_TYPELESS &&
                desc.Format <= DXGI_FORMAT_BC7_UNORM_SRGB) )
          {
            compressed = true;
          }

          // If this isn't an injectable texture, then filter out non-mipmapped
          //   textures.
          if (/*(! injectable) && */cache_opts.ignore_non_mipped)
            cacheable &= (desc.MipLevels > 1 || compressed);

          if (cacheable)
          {
            SK_LOG1 ( ( L"New Cacheable Texture: (%lux%lu) -- %x",
                          desc.Width, desc.Height, top_crc32c ),
                        L"DX11TexMgr" );

            SK_D3D11_Textures.CacheMisses_2D++;
            SK_D3D11_Textures.refTexture2D ( pTex, &desc, cache_tag, size, elapsed, top_crc32c,
                                               L"", nullptr, (HMODULE)(intptr_t)-1/*SK_GetCallingDLL ()*/, pTLS );

            return;
          }
        }
      }
    }
  }

  return D3D11_UpdateSubresource1_Original ( This, pDstResource, DstSubresource,
                                               pDstBox, pSrcData, SrcRowPitch,
                                                 SrcDepthPitch, CopyFlags );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_UpdateSubresource_Override (
  _In_           ID3D11DeviceContext* This,
  _In_           ID3D11Resource      *pDstResource,
  _In_           UINT                 DstSubresource,
  _In_opt_ const D3D11_BOX           *pDstBox,
  _In_     const void                *pSrcData,
  _In_           UINT                 SrcRowPitch,
  _In_           UINT                 SrcDepthPitch)
{
  bool early_out = false;

  SK_TLS *pTLS = nullptr;

  // UB: If it's happening, pretend we never saw this...
  if (pDstResource == nullptr || pSrcData == nullptr)
  {
    early_out = true;
  }

  else
    early_out = (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS));

  if (early_out)
  {
    return D3D11_UpdateSubresource_Original ( This, pDstResource, DstSubresource,
                                                pDstBox, pSrcData, SrcRowPitch,
                                                  SrcDepthPitch );
  }

  //if (SK_GetCurrentGameID() == SK_GAME_ID::Ys_Eight)
  //{
  //  dll_log.Log ( L"UpdateSubresource:  { [%p] <RowPitch: %5lu> } -> { %s [%lu] }",
  //                pSrcData, SrcRowPitch, DescribeResource (pDstResource).c_str  (), DstSubresource );
  //}

  //dll_log.Log (L"[   DXGI   ] [!]D3D11_UpdateSubresource (%ph, %lu, %ph, %ph, %lu, %lu)",
  //          pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);

  D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
  pDstResource->GetType  (&rdim);

  if (SK_D3D11_IsStagingCacheable (rdim, pDstResource) && DstSubresource == 0)
  {
    CComQIPtr <ID3D11Texture2D> pTex (pDstResource);

    if (pTex != nullptr)
    {
      D3D11_TEXTURE2D_DESC desc = { };
           pTex->GetDesc (&desc);

      D3D11_SUBRESOURCE_DATA srd = { };

      srd.pSysMem           = pSrcData;
      srd.SysMemPitch       = SrcRowPitch;
      srd.SysMemSlicePitch  = 0;

      size_t   size         = 0;
      uint32_t top_crc32c   = 0x0;

      uint32_t checksum     =
        crc32_tex   (&desc, &srd, &size, &top_crc32c);

      uint32_t cache_tag    =
        safe_crc32c (top_crc32c, (uint8_t *)&desc, sizeof D3D11_TEXTURE2D_DESC);

      auto start            = SK_QueryPerf ().QuadPart;

      CComPtr <ID3D11Texture2D> pCachedTex =
        SK_D3D11_Textures.getTexture2D (cache_tag, &desc);

      if (pCachedTex != nullptr)
      {
        CComQIPtr <ID3D11Resource> pCachedResource = pCachedTex;

        D3D11_CopyResource_Original (This, pDstResource, pCachedResource);

        SK_LOG1 ( ( L"Texture Cache Hit (Slow Path): (%lux%lu) -- %x",
                      desc.Width, desc.Height, top_crc32c ),
                    L"DX11TexMgr" );

        return;
      }

      else
      {
        if (SK_D3D11_TextureIsCached (pTex))
        {
          SK_LOG0 ( (L"Cached texture was updated (UpdateSubresource)... removing from cache! - <%s>",
                         SK_GetCallerName ().c_str ()), L"DX11TexMgr" );
          SK_D3D11_RemoveTexFromCache (pTex, true);
        }

        D3D11_UpdateSubresource_Original ( This, pDstResource, DstSubresource,
                                             pDstBox, pSrcData, SrcRowPitch,
                                               SrcDepthPitch );
        auto end     = SK_QueryPerf ().QuadPart;
        auto elapsed = end - start;

        if (desc.Usage == D3D11_USAGE_STAGING)
        {
          auto&& map_ctx = mapped_resources [This];

          map_ctx.dynamic_textures  [pDstResource] = checksum;
          map_ctx.dynamic_texturesx [pDstResource] = top_crc32c;

          SK_LOG1 ( ( L"New Staged Texture: (%lux%lu) -- %x",
                        desc.Width, desc.Height, top_crc32c ),
                      L"DX11TexMgr" );

          map_ctx.dynamic_times2    [checksum]  = elapsed;
          map_ctx.dynamic_sizes2    [checksum]  = size;

          return;
        }

        else
        {
          bool cacheable = ( desc.MiscFlags <= 4 &&
                             desc.Width      > 0 && 
                             desc.Height     > 0 &&
                             desc.ArraySize == 1 //||
                           //((desc.ArraySize  % 6 == 0) && (desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE))
                           );

          bool compressed = false;

          if ( (desc.Format >= DXGI_FORMAT_BC1_TYPELESS  &&
                desc.Format <= DXGI_FORMAT_BC5_SNORM)    ||
               (desc.Format >= DXGI_FORMAT_BC6H_TYPELESS &&
                desc.Format <= DXGI_FORMAT_BC7_UNORM_SRGB) )
          {
            compressed = true;
          }

          // If this isn't an injectable texture, then filter out non-mipmapped
          //   textures.
          if (/*(! injectable) && */cache_opts.ignore_non_mipped)
            cacheable &= (desc.MipLevels > 1 || compressed);

          if (cacheable)
          {
            SK_LOG1 ( ( L"New Cacheable Texture: (%lux%lu) -- %x",
                          desc.Width, desc.Height, top_crc32c ),
                        L"DX11TexMgr" );

            SK_D3D11_Textures.CacheMisses_2D++;
            SK_D3D11_Textures.refTexture2D (pTex, &desc, cache_tag, size, elapsed, top_crc32c,
                                            L"", nullptr, (HMODULE)(intptr_t)-1/*SK_GetCallingDLL ()*/, pTLS);

            return;
          }
        }
      }
    }
  }

  return D3D11_UpdateSubresource_Original ( This, pDstResource, DstSubresource,
                                              pDstBox, pSrcData, SrcRowPitch,
                                                SrcDepthPitch );
}

const GUID SKID_D3D11Texture2D_DISCARD =
// {5C5298CA-0F9C-4931-A19D-A2E69792AE02}
  { 0x5c5298ca, 0xf9c,  0x4931, { 0xa1, 0x9d, 0xa2, 0xe6, 0x97, 0x92, 0xae, 0x2 } };

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_Map_Override (
   _In_ ID3D11DeviceContext      *This,
   _In_ ID3D11Resource           *pResource,
   _In_ UINT                      Subresource,
   _In_ D3D11_MAP                 MapType,
   _In_ UINT                      MapFlags,
_Out_opt_ D3D11_MAPPED_SUBRESOURCE *pMappedResource )
{
  D3D11_MAPPED_SUBRESOURCE local_map = { };

  if (pMappedResource == nullptr)
    pMappedResource = &local_map;


  HRESULT hr =
    D3D11_Map_Original ( This, pResource, Subresource,
                           MapType, MapFlags, pMappedResource );


  // UB: If it's happening, pretend we never saw this...
  if (pResource == nullptr)
  {
    assert (pResource != nullptr);

    return hr;
  }

  D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
      pResource->GetType (&rdim);

  // ImGui gets to pass-through without invoking the hook
  if ((! config.textures.cache.allow_staging) && (! SK_D3D11_ShouldTrackRenderOp (This)))
  {
    return hr;
  }

  if (SUCCEEDED (hr))
  {
    SK_D3D11_MemoryThreads.mark ();

    if (SK_D3D11_IsStagingCacheable (rdim, pResource))
    {
      // Reference will be released, but we expect the game's going to unmap this at some point anyway...
      CComQIPtr <ID3D11Texture2D> pTex = pResource;

      if (pTex != nullptr)
      {
                        D3D11_TEXTURE2D_DESC    d3d11_desc = { };
        pTex->GetDesc ((D3D11_TEXTURE2D_DESC *)&d3d11_desc);

        SK_D3D11_TEXTURE2D_DESC desc (d3d11_desc);

        //dll_log.Log ( L"Staging Map: Type=%x, BindFlags: %s, Res: %lux%lu",
        //                MapType, SK_D3D11_DescribeBindFlags (desc.BindFlags).c_str (),
        //       desc.Width, desc.Height, SK_DXGI_FormatToStr (desc.Format).c_str    () );

        if (MapType == D3D11_MAP_WRITE_DISCARD)
        {
          auto&& it = SK_D3D11_Textures.Textures_2D.find (pTex);
          if (  it != SK_D3D11_Textures.Textures_2D.end  (    ) && it->second.crc32c != 0x00 )
          {
                                                                               it->second.discard = true;
            pTex->SetPrivateData (SKID_D3D11Texture2D_DISCARD, sizeof (bool), &it->second.discard);

            SK_D3D11_RemoveTexFromCache (pTex, true);
            SK_D3D11_Textures.HashMap_2D [it->second.orig_desc.MipLevels].erase (it->second.tag);

            SK_LOG4 ( ( L"Removing discarded texture from cache (it has been memory-mapped as discard)." ),
                        L"DX11TexMgr" );
          } 
        }

        UINT  private_size = sizeof (bool);
        bool  private_data = false;

        bool discard = false;

        if (SUCCEEDED (pTex->GetPrivateData (SKID_D3D11Texture2D_DISCARD, &private_size, &private_data)) && private_size == sizeof (bool))
        {
          discard = private_data;
        }

        if ((desc.BindFlags & D3D11_BIND_SHADER_RESOURCE ) || desc.Usage == D3D11_USAGE_STAGING)
        {
          if (! discard)
          {
            // Keep cached, but don't update -- it is now discarded and we should ignore it
            if (SK_D3D11_TextureIsCached (pTex))
            {
              auto& texDesc =
                SK_D3D11_Textures.Textures_2D [pTex];
                                                                                 texDesc.discard = true;
              pTex->SetPrivateData (SKID_D3D11Texture2D_DISCARD, sizeof (bool), &texDesc.discard);

              SK_LOG1 ( ( L"Removing discarded texture from cache (it has been memory-mapped multiple times)." ),
                          L"DX11TexMgr" );
              SK_D3D11_RemoveTexFromCache (pTex, true);
            }

            else
            {
              auto&& map_ctx = mapped_resources [This];

              map_ctx.textures.emplace      (std::make_pair (pResource, *pMappedResource));
              map_ctx.texture_times.emplace (std::make_pair (pResource, SK_QueryPerf ().QuadPart));

              //dll_log.Log (L"[DX11TexMgr] Mapped 2D texture...");
            }
          }

          else
          {
            if (SK_D3D11_TextureIsCached (pTex))
            {
              SK_LOG1 ( ( L"Removing discarded texture from cache." ),
                          L"DX11TexMgr" );
              SK_D3D11_RemoveTexFromCache (pTex, true);
            }
            //dll_log.Log (L"[DX11TexMgr] Skipped 2D texture...");
          }
        }
      }
    }

    if (! SK_D3D11_EnableMMIOTracking)
      return hr;

    bool read =  ( MapType == D3D11_MAP_READ      ||
                   MapType == D3D11_MAP_READ_WRITE );

    bool write = ( MapType == D3D11_MAP_WRITE             ||
                   MapType == D3D11_MAP_WRITE_DISCARD     ||
                   MapType == D3D11_MAP_READ_WRITE        ||
                   MapType == D3D11_MAP_WRITE_NO_OVERWRITE );

    mem_map_stats.last_frame.map_types [MapType-1]++;

    switch (rdim)
    {
      case D3D11_RESOURCE_DIMENSION_UNKNOWN:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_UNKNOWN]++;
        break;
      case D3D11_RESOURCE_DIMENSION_BUFFER:
      {
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_BUFFER]++;

        ID3D11Buffer* pBuffer = nullptr;

        if (SUCCEEDED (pResource->QueryInterface <ID3D11Buffer> (&pBuffer)))
        {
          D3D11_BUFFER_DESC  buf_desc = { };
          pBuffer->GetDesc (&buf_desc);
          {
            ///std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_mmio);

            // Extra memory allocation pressure for no good reason -- kill it.
            //  
            ////if (buf_desc.BindFlags & D3D11_BIND_INDEX_BUFFER)
            ////  mem_map_stats.last_frame.index_buffers.insert (pBuffer);
            ////
            ////if (buf_desc.BindFlags & D3D11_BIND_VERTEX_BUFFER)
            ////  mem_map_stats.last_frame.vertex_buffers.insert (pBuffer);
            ////
            ////if (buf_desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)
            ////  mem_map_stats.last_frame.constant_buffers.insert (pBuffer);
          }

          if (read)
            mem_map_stats.last_frame.bytes_read    += buf_desc.ByteWidth;

          if (write)
            mem_map_stats.last_frame.bytes_written += buf_desc.ByteWidth;

          pBuffer->Release ();
        }
      } break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE1D]++;
        break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE2D]++;
        break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE3D]++;
        break;
    }
  }

  return hr;
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_Unmap_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Resource      *pResource,
  _In_ UINT                 Subresource )
{ 
  // UB: If it's happening, pretend we never saw this...
  if (pResource == nullptr)
  {
    assert (pResource != nullptr);

    return
      D3D11_Unmap_Original (This, pResource, Subresource);
  }

  SK_TLS *pTLS = nullptr;

  // ImGui gets to pass-through without invoking the hook
  if ((! config.textures.cache.allow_staging) && (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS)))
  {
    D3D11_Unmap_Original (This, pResource, Subresource);
    return;
  }

  pTLS = ( pTLS != nullptr       ?
           pTLS : SK_TLS_Bottom ( ) );

  SK_D3D11_MemoryThreads.mark ();

  D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
      pResource->GetType (&rdim);

  if (SK_D3D11_IsStagingCacheable (rdim, pResource) && Subresource == 0)
  {
    if (rdim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      auto&& map_ctx = mapped_resources [This];

      // More of an assertion, if this fails something's screwy!
      if (map_ctx.textures.count (pResource))
      {
        std::lock_guard <SK_Thread_CriticalSection> auto_lock  (cs_mmio);

        uint64_t time_elapsed =
          SK_QueryPerf ().QuadPart - map_ctx.texture_times [pResource];

        CComQIPtr <ID3D11Texture2D> pTex = pResource;

        if (pTex != nullptr)
        {
          uint32_t checksum  = 0;
          size_t   size      = 0;
          uint32_t top_crc32 = 0x00;

          D3D11_TEXTURE2D_DESC desc = { };
               pTex->GetDesc (&desc);

          if ((desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) || desc.Usage == D3D11_USAGE_STAGING)
          {
            checksum =
              crc32_tex (&desc, (D3D11_SUBRESOURCE_DATA *)&map_ctx.textures [pResource], &size, &top_crc32);

            //dll_log.Log (L"[DX11TexMgr] Mapped 2D texture... (%x -- %lu bytes)", checksum, size);

            if (checksum != 0x0)
            {
              if (desc.Usage != D3D11_USAGE_STAGING)
              {
                uint32_t cache_tag    =
                  safe_crc32c (top_crc32, (uint8_t *)&desc, sizeof D3D11_TEXTURE2D_DESC);

                SK_D3D11_Textures.CacheMisses_2D++;

                SK_D3D11_Textures.refTexture2D ( pTex,
                                                   &desc,
                                                     cache_tag,
                                                       size,
                                                         time_elapsed,
                                                           top_crc32,
                                                      L"", nullptr, (HMODULE)(intptr_t)-1/*SK_GetCallingDLL ()*/,
                                                             pTLS );
              }

              else
              {
                map_ctx.dynamic_textures  [pResource] = checksum;
                map_ctx.dynamic_texturesx [pResource] = top_crc32;

                map_ctx.dynamic_times2    [checksum]  = time_elapsed;
                map_ctx.dynamic_sizes2    [checksum]  = size;
              }
            }
          }
        }

        map_ctx.textures.erase      (pResource);
        map_ctx.texture_times.erase (pResource);
      }
    }
  }

  D3D11_Unmap_Original (This, pResource, Subresource);
}


__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CopyResource_Override (
       ID3D11DeviceContext *This,
  _In_ ID3D11Resource      *pDstResource,
  _In_ ID3D11Resource      *pSrcResource )
{
  // UB: If it's happening, pretend we never saw this...
  if (pDstResource == nullptr || pSrcResource == nullptr)
  {
    return;
  }

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  if (! pTLS->imgui.drawing)
    SK_ReShade_CopyResourceCallback.call (pDstResource, pSrcResource, pTLS);

  CComQIPtr <ID3D11Texture2D> pDstTex = pDstResource;

  if (pDstTex != nullptr)
  {
    if (! SK_D3D11_IsTexInjectThread (pTLS))
    {
      if (SK_D3D11_TextureIsCached (pDstTex))
      {
        //SK_LOG0 ( (L"Cached texture was modified (CopyResource)... removing from cache! - <%s>",
        //               SK_GetCallerName ().c_str ()), L"DX11TexMgr" );
        //SK_D3D11_RemoveTexFromCache (pDstTex, true);
      }
    }
  }


  // ImGui gets to pass-through without invoking the hook
  if ((! config.textures.cache.allow_staging) && (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS)))
  {
    D3D11_CopyResource_Original (This, pDstResource, pSrcResource);
  
    return;
  }


  D3D11_RESOURCE_DIMENSION res_dim = { };
   pSrcResource->GetType (&res_dim);


  if (SK_D3D11_EnableMMIOTracking)
  {
    SK_D3D11_MemoryThreads.mark ();

    switch (res_dim)
    {
      case D3D11_RESOURCE_DIMENSION_UNKNOWN:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_UNKNOWN]++;
        break;
      case D3D11_RESOURCE_DIMENSION_BUFFER:
      {
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_BUFFER]++;
    
        ID3D11Buffer* pBuffer = nullptr;
    
        if (SUCCEEDED (pSrcResource->QueryInterface <ID3D11Buffer> (&pBuffer)))
        {
          D3D11_BUFFER_DESC  buf_desc = { };
          pBuffer->GetDesc (&buf_desc);
          {
            if (buf_desc.BindFlags & D3D11_BIND_INDEX_BUFFER)
              mem_map_stats.last_frame.index_buffers.insert (pBuffer);

            if (buf_desc.BindFlags & D3D11_BIND_VERTEX_BUFFER)
              mem_map_stats.last_frame.vertex_buffers.insert (pBuffer);

            if (buf_desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)
              mem_map_stats.last_frame.constant_buffers.insert (pBuffer);
          }

          mem_map_stats.last_frame.bytes_copied += buf_desc.ByteWidth;

          pBuffer->Release ();
        }
      } break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE1D]++;
        break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE2D]++;
        break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE3D]++;
        break;
    }
  }

  D3D11_CopyResource_Original (This, pDstResource, pSrcResource);


  if ( SK_D3D11_IsStagingCacheable (res_dim, pSrcResource) ||
       SK_D3D11_IsStagingCacheable (res_dim, pDstResource) )
  {
    auto&& map_ctx = mapped_resources [This];

    CComQIPtr <ID3D11Texture2D> pSrcTex (pSrcResource);

    if (pSrcTex != nullptr)
    {
      if (SK_D3D11_TextureIsCached (pSrcTex))
      {
        dll_log.Log (L"Copying from cached source with checksum: %x", SK_D3D11_TextureHashFromCache (pSrcTex));
      }
    }

    if (pDstTex != nullptr && map_ctx.dynamic_textures.count (pSrcResource))
    {
      uint32_t top_crc32 = map_ctx.dynamic_texturesx [pSrcResource];
      uint32_t checksum  = map_ctx.dynamic_textures  [pSrcResource];

      D3D11_TEXTURE2D_DESC dst_desc = { };
        pDstTex->GetDesc (&dst_desc);

      uint32_t cache_tag =
        safe_crc32c (top_crc32, (uint8_t *)&dst_desc, sizeof D3D11_TEXTURE2D_DESC);

      if (checksum != 0x00 && dst_desc.Usage != D3D11_USAGE_STAGING)
      {
        SK_D3D11_Textures.CacheMisses_2D++;

        SK_D3D11_Textures.refTexture2D ( pDstTex,
                                           &dst_desc,
                                             cache_tag,
                                               map_ctx.dynamic_sizes2   [checksum],
                                                 map_ctx.dynamic_times2 [checksum],
                                                   top_crc32,
                                      L"", nullptr, (HMODULE)(intptr_t)-1/*SK_GetCallingDLL ()*/,
                                                     pTLS );
        map_ctx.dynamic_textures.erase  (pSrcResource);
        map_ctx.dynamic_texturesx.erase (pSrcResource);

        map_ctx.dynamic_sizes2.erase    (checksum);
        map_ctx.dynamic_times2.erase    (checksum);
      }
    }
  }
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CopySubresourceRegion_Override (
  _In_           ID3D11DeviceContext *This,
  _In_           ID3D11Resource      *pDstResource,
  _In_           UINT                 DstSubresource,
  _In_           UINT                 DstX,
  _In_           UINT                 DstY,
  _In_           UINT                 DstZ,
  _In_           ID3D11Resource      *pSrcResource,
  _In_           UINT                 SrcSubresource,
  _In_opt_ const D3D11_BOX           *pSrcBox )
{
  // UB: If it's happening, pretend we never saw this...
  if (pDstResource == nullptr || pSrcResource == nullptr)
  {
    return;
  }


  ///if (SK_GetCurrentGameID() == SK_GAME_ID::Ys_Eight)
  ///{
  ///  CComQIPtr <ID3D11Texture2D> pTex (pSrcResource);
  ///
  ///  if (pTex)
  ///  {
  ///    D3D11_BOX box = { };
  ///    
  ///    if (pSrcBox != nullptr)
  ///        box = *pSrcBox;
  ///
  ///    else
  ///    {
  ///      D3D11_TEXTURE2D_DESC tex_desc = {};
  ///           pTex->GetDesc (&tex_desc);
  ///
  ///      box.left  = 0; box.right  = tex_desc.Width; 
  ///      box.top   = 0; box.bottom = tex_desc.Height;
  ///      box.front = 0; box.back   = 1;
  ///    }
  ///    
  ///    dll_log.Log ( L"CopySubresourceRegion:  { %s <%lu> [ %lu/%lu, %lu/%lu, %lu/%lu ] } -> { %s <%lu> (%lu,%lu,%lu) }",
  ///                    DescribeResource (pSrcResource).c_str (), SrcSubresource, box.left,box.right, box.top,box.bottom, box.front,box.back,
  ///                    DescribeResource (pDstResource).c_str (), DstSubresource, DstX, DstY, DstZ );
  ///  }
  ///}


  if ( (! config.render.dxgi.deferred_isolation)    &&
            This->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED )
  {
    return
      D3D11_CopySubresourceRegion_Original ( This, pDstResource, DstSubresource,
                                                     DstX, DstY, DstZ,
                                                       pSrcResource, SrcSubresource,
                                                         pSrcBox );
  }


  SK_TLS* pTLS = SK_TLS_Bottom ();

  CComQIPtr <ID3D11Texture2D> pDstTex (pDstResource);

  if (pDstTex != nullptr)
  {
    if (! SK_D3D11_IsTexInjectThread (pTLS))
    {
      //if (SK_GetCurrentGameID () == SK_GAME_ID::PillarsOfEternity2)
      //{
      //  extern          bool SK_POE2_NopSubresourceCopy;
      //  extern volatile LONG SK_POE2_SkippedCopies;
      //
      //  if (SK_POE2_NopSubresourceCopy)
      //  {
      //    D3D11_TEXTURE2D_DESC desc_out = { };
      //      pDstTex->GetDesc (&desc_out);
      //
      //    if (pSrcBox != nullptr)
      //    {
      //      dll_log.Log (L"Copy (%lu-%lu,%lu-%lu : %lu,%lu,%lu : %s : {%p,%p})",
      //        pSrcBox->left, pSrcBox->right, pSrcBox->top, pSrcBox->bottom,
      //          DstX, DstY, DstZ,
      //            SK_D3D11_DescribeUsage (desc_out.Usage),
      //              pSrcResource, pDstResource );
      //    }
      //
      //    else
      //    {
      //      dll_log.Log (L"Copy (%lu,%lu,%lu : %s)",
      //                   DstX, DstY, DstZ,
      //                   SK_D3D11_DescribeUsage (desc_out.Usage) );
      //    }
      //
      //    if (pSrcBox == nullptr || ( pSrcBox->right != 3840 || pSrcBox->bottom != 2160 ))
      //    {
      //      if (desc_out.Usage == D3D11_USAGE_STAGING || pSrcBox == nullptr)
      //      {
      //        InterlockedIncrement (&SK_POE2_SkippedCopies);
      //
      //        return;
      //      }
      //    }
      //  }
      //}

      if (DstSubresource == 0 && SK_D3D11_TextureIsCached (pDstTex))
      {
        SK_LOG0 ( (L"Cached texture was modified (CopySubresourceRegion)... removing from cache! - <%s>",
                       SK_GetCallerName ().c_str ()), L"DX11TexMgr" );
        SK_D3D11_RemoveTexFromCache (pDstTex, true);
      }
    }
  }


  // ImGui gets to pass-through without invoking the hook
  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
  {
    D3D11_CopySubresourceRegion_Original (This, pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox);

    return;
  }


  D3D11_RESOURCE_DIMENSION res_dim = { };
  pSrcResource->GetType  (&res_dim);


  if (SK_D3D11_EnableMMIOTracking)
  {
    SK_D3D11_MemoryThreads.mark ();

    switch (res_dim)
    {
      case D3D11_RESOURCE_DIMENSION_UNKNOWN:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_UNKNOWN]++;
        break;
      case D3D11_RESOURCE_DIMENSION_BUFFER:
      {
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_BUFFER]++;

        ID3D11Buffer* pBuffer = nullptr;

        if (SUCCEEDED (pSrcResource->QueryInterface <ID3D11Buffer> (&pBuffer)))
        {
          D3D11_BUFFER_DESC  buf_desc = { };
          pBuffer->GetDesc (&buf_desc);
          {
            ////std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_mmio);

            if (buf_desc.BindFlags & D3D11_BIND_INDEX_BUFFER)
              mem_map_stats.last_frame.index_buffers.insert (pBuffer);

            if (buf_desc.BindFlags & D3D11_BIND_VERTEX_BUFFER)
              mem_map_stats.last_frame.vertex_buffers.insert (pBuffer);

            if (buf_desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)
              mem_map_stats.last_frame.constant_buffers.insert (pBuffer);
          }

          mem_map_stats.last_frame.bytes_copied += buf_desc.ByteWidth;

          pBuffer->Release ();
        }
      } break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE1D]++;
        break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE2D]++;
        break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE3D]++;
        break;
    }
  }


  D3D11_CopySubresourceRegion_Original (This, pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox);

  if ( ( SK_D3D11_IsStagingCacheable (res_dim, pSrcResource) ||
         SK_D3D11_IsStagingCacheable (res_dim, pDstResource) ) && SrcSubresource == 0 && DstSubresource == 0)
  {
    auto&& map_ctx = mapped_resources [This];

    if (pDstTex != nullptr && map_ctx.dynamic_textures.count (pSrcResource))
    {
      uint32_t top_crc32 = map_ctx.dynamic_texturesx [pSrcResource];
      uint32_t checksum  = map_ctx.dynamic_textures  [pSrcResource];

      D3D11_TEXTURE2D_DESC dst_desc = { };
        pDstTex->GetDesc (&dst_desc);

      uint32_t cache_tag =
        safe_crc32c (top_crc32, (uint8_t *)&dst_desc, sizeof D3D11_TEXTURE2D_DESC);

      if (checksum != 0x00 && dst_desc.Usage != D3D11_USAGE_STAGING)
      {
        SK_D3D11_Textures.refTexture2D ( pDstTex,
                                           &dst_desc,
                                             cache_tag,
                                               map_ctx.dynamic_sizes2   [checksum],
                                                 map_ctx.dynamic_times2 [checksum],
                                                   top_crc32,
                                     L"", nullptr, (HMODULE)(intptr_t)-1/*SK_GetCallingDLL ()*/,
                                                     pTLS );

        map_ctx.dynamic_textures.erase  (pSrcResource);
        map_ctx.dynamic_texturesx.erase (pSrcResource);

        map_ctx.dynamic_sizes2.erase    (checksum);
        map_ctx.dynamic_times2.erase    (checksum);
      }
    }
  }
}



void
SK_D3D11_ClearResidualDrawState (SK_TLS* pTLS = SK_TLS_Bottom ())
{
  if (pTLS->d3d11.pDSVOrig != nullptr)
  {
    CComQIPtr <ID3D11DeviceContext> pDevCtx (
      pTLS->d3d11.pDevCtx
    );

    if (pDevCtx != nullptr)
    {
      ID3D11RenderTargetView* pRTV [8] = { };
      CComPtr <ID3D11DepthStencilView> pDSV;
      
      pDevCtx->OMGetRenderTargets (8, &pRTV [0], &pDSV);

      UINT OMRenderTargetCount =
        calc_count (&pRTV [0], D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);

      pDevCtx->OMSetRenderTargets (OMRenderTargetCount, &pRTV [0], pTLS->d3d11.pDSVOrig);

      for (UINT i = 0; i < OMRenderTargetCount; i++)
      {
        if (pRTV [i] != 0) pRTV [i]->Release ();
      }

      pTLS->d3d11.pDSVOrig = nullptr;
    }
  }

  if (pTLS->d3d11.pDepthStencilStateOrig != nullptr)
  {
    CComQIPtr <ID3D11DeviceContext> pDevCtx (
      pTLS->d3d11.pDevCtx
    );

    if (pDevCtx != nullptr)
    {
      pDevCtx->OMSetDepthStencilState (pTLS->d3d11.pDepthStencilStateOrig, pTLS->d3d11.StencilRefOrig);

      pTLS->d3d11.pDepthStencilStateOrig->Release ();
      pTLS->d3d11.pDepthStencilStateOrig = nullptr;

      if (pTLS->d3d11.pDepthStencilStateNew != nullptr)
      {
        pTLS->d3d11.pDepthStencilStateNew->Release ();
        pTLS->d3d11.pDepthStencilStateNew = nullptr;
      }
    }
  }


  if (pTLS->d3d11.pRasterStateOrig != nullptr)
  {
    CComQIPtr <ID3D11DeviceContext> pDevCtx (
      pTLS->d3d11.pDevCtx
    );

    if (pDevCtx != nullptr)
    {
      pDevCtx->RSSetState (pTLS->d3d11.pRasterStateOrig);

      pTLS->d3d11.pRasterStateOrig->Release ();
      pTLS->d3d11.pRasterStateOrig = nullptr;

      if (pTLS->d3d11.pRasterStateNew != nullptr)
      {
        pTLS->d3d11.pRasterStateNew->Release ();
        pTLS->d3d11.pRasterStateNew = nullptr;
      }
    }
  }


  for (int i = 0; i < 5; i++)
  {
    if (pTLS->d3d11.pOriginalCBuffers [i][0] != nullptr)
    {
      if ((intptr_t)pTLS->d3d11.pOriginalCBuffers [i][0] == -1)
        pTLS->d3d11.pOriginalCBuffers [i][0] = nullptr;

      CComQIPtr <ID3D11DeviceContext> pDevCtx (
        pTLS->d3d11.pDevCtx
      );

      switch (i)
      {
        case 0:
          pDevCtx->VSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, &pTLS->d3d11.pOriginalCBuffers [i][0]);
          break;
        case 1:
          pDevCtx->PSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, &pTLS->d3d11.pOriginalCBuffers [i][0]);
          break;
        case 2:
          pDevCtx->GSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, &pTLS->d3d11.pOriginalCBuffers [i][0]);
          break;
        case 3:
          pDevCtx->HSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, &pTLS->d3d11.pOriginalCBuffers [i][0]);
          break;
        case 4:
          pDevCtx->DSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, &pTLS->d3d11.pOriginalCBuffers [i][0]);
          break;
      }

      for (int j = 0; j < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; j++)
      {
        if (pTLS->d3d11.pOriginalCBuffers [i][j] != nullptr)
        {
          pTLS->d3d11.pOriginalCBuffers [i][j]->Release ();
          pTLS->d3d11.pOriginalCBuffers [i][j] = nullptr;
        }
      }
    }
  }
}


void
SK_D3D11_PostDraw (SK_TLS *pTLS)
{
  //if ( SK_GetCurrentRenderBackend ().device              == nullptr ||
  //     SK_GetCurrentRenderBackend ().swapchain           == nullptr ) return;

  SK_D3D11_ClearResidualDrawState (pTLS);
}

struct
{
  SK_ReShade_PresentCallback_pfn fn   = nullptr;
  void*                          data = nullptr;

  struct explict_draw_s
  {
    void*                   ptr     = nullptr;
    ID3D11RenderTargetView* pRTV    = nullptr;
    bool                    pass    = false;
    int                     calls   = 0;
    ID3D11DeviceContext*    src_ctx = nullptr;
  } explicit_draw;
} SK_ReShade_PresentCallback;

__declspec (noinline)
IMGUI_API
void
SK_ReShade_InstallPresentCallback (SK_ReShade_PresentCallback_pfn fn, void* user)
{
  SK_ReShade_PresentCallback.fn                = fn;
  SK_ReShade_PresentCallback.explicit_draw.ptr = user;
}

__declspec (noinline)
IMGUI_API
void
SK_ReShade_InstallDrawCallback (SK_ReShade_OnDrawD3D11_pfn fn, void* user)
{
  SK_ReShade_DrawCallback.fn   = fn;
  SK_ReShade_DrawCallback.data = user;
}

__declspec (noinline)
IMGUI_API
void
SK_ReShade_InstallSetDepthStencilViewCallback (SK_ReShade_OnSetDepthStencilViewD3D11_pfn fn, void* user)
{
  SK_ReShade_SetDepthStencilViewCallback.fn   = fn;
  SK_ReShade_SetDepthStencilViewCallback.data = user;
}

__declspec (noinline)
IMGUI_API
void
SK_ReShade_InstallGetDepthStencilViewCallback (SK_ReShade_OnGetDepthStencilViewD3D11_pfn fn, void* user)
{
  SK_ReShade_GetDepthStencilViewCallback.fn   = fn;
  SK_ReShade_GetDepthStencilViewCallback.data = user;
}

__declspec (noinline)
IMGUI_API
void
SK_ReShade_InstallClearDepthStencilViewCallback (SK_ReShade_OnClearDepthStencilViewD3D11_pfn fn, void* user)
{
  SK_ReShade_ClearDepthStencilViewCallback.fn   = fn;
  SK_ReShade_ClearDepthStencilViewCallback.data = user;
}

__declspec (noinline)
IMGUI_API
void
SK_ReShade_InstallCopyResourceCallback (SK_ReShade_OnCopyResourceD3D11_pfn fn, void* user)
{
  SK_ReShade_CopyResourceCallback.fn   = fn;
  SK_ReShade_CopyResourceCallback.data = user;
}



class SK_ExecuteReShadeOnReturn
{
public:
   explicit
   SK_ExecuteReShadeOnReturn ( ID3D11DeviceContext* pCtx,
                               UINT                 uiDevCtxHandle,
                               SK_TLS*              pTLS ) : _ctx        (pCtx),
                                                             _ctx_handle (uiDevCtxHandle),
                                                             _tls        (pTLS) { };
  ~SK_ExecuteReShadeOnReturn (void)
  {
    auto TriggerReShade_After = [&]()
    {
      SK_ScopedBool auto_bool (&_tls->imgui.drawing);
      _tls->imgui.drawing = true;
      
      UINT dev_idx = _ctx_handle;

      if (SK_ReShade_PresentCallback.fn && (! SK_D3D11_Shaders.reshade_triggered [dev_idx]))
      {
        //CComPtr <ID3D11DepthStencilView>  pDSV = nullptr;
        //CComPtr <ID3D11DepthStencilState> pDSS = nullptr;
        //CComPtr <ID3D11RenderTargetView>  pRTV = nullptr;
        //
        //                                    UINT ref;
        //   _ctx->OMGetDepthStencilState (&pDSS, &ref);
        //   _ctx->OMGetRenderTargets     (1, &pRTV, &pDSV);

        //if (pDSS)
        //{
        //  D3D11_DEPTH_STENCIL_DESC ds_desc;
        //           pDSS->GetDesc (&ds_desc);
        //
        //  if ((! pDSV) || (! ds_desc.StencilEnable))
        //  {
            for (int i = 0 ; i < 5; i++)
            {
              SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *>* pShaderReg;

              switch (i)
              {
                default:
                case 0:
                  pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.vertex;
                  break;
                case 1:
                  pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.pixel;
                  break;
                case 2:
                  pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.geometry;
                  break;
                case 3:
                  pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.hull;
                  break;
                case 4:
                  pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.domain;
                  break;
              };

              if ( (! pShaderReg->trigger_reshade.after.empty ()) &&
                      pShaderReg->current.shader [dev_idx] != 0x0    &&
                      pShaderReg->trigger_reshade.after.count (pShaderReg->current.shader [dev_idx]) )
              {
                SK_D3D11_Shaders.reshade_triggered [dev_idx] = true;

                SK_ReShade_PresentCallback.explicit_draw.calls++;
                SK_ReShade_PresentCallback.explicit_draw.src_ctx = _ctx;
                SK_ReShade_PresentCallback.fn (&SK_ReShade_PresentCallback.explicit_draw);
                break;
              }
            }
          //}
        }
      //}
    };

    TriggerReShade_After ();
  }

protected:
  ID3D11DeviceContext* _ctx;
  UINT                 _ctx_handle;
  SK_TLS*              _tls;
};


void*
__cdecl
SK_SEH_Guarded_memcpy (
    _Out_writes_bytes_all_ (_Size) void       *_Dst,
    _In_reads_bytes_       (_Size) void const *_Src,
    _In_                           size_t      _Size )
{
  __try {
    return memcpy (_Dst, _Src, _Size);
  } __except ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                            EXCEPTION_EXECUTE_HANDLER :
                            EXCEPTION_CONTINUE_SEARCH   )
  {
    return _Dst;
  }
}

#include <concurrent_vector.h>
#include <array>
concurrency::concurrent_vector <d3d11_shader_tracking_s::cbuffer_override_s> __SK_D3D11_PixelShader_CBuffer_Overrides;

std::array <bool, SK_D3D11_MAX_DEV_CONTEXTS+1> SK_D3D11_KnownShaders::reshade_triggered;

#include <SpecialK/control_panel/d3d11.h>

// Indicates whether the shader mod window is tracking render target refs
static bool live_rt_view = true;

bool
SK_D3D11_DrawHandler (ID3D11DeviceContext* pDevCtx, SK_TLS* pTLS = nullptr)
{
  if ( SK_GetCurrentRenderBackend ().d3d11.immediate_ctx == nullptr ||
       SK_GetCurrentRenderBackend ().device              == nullptr ||
       SK_GetCurrentRenderBackend ().swapchain           == nullptr )
  {
    return false;
  }

  if (! pTLS)
        pTLS = SK_TLS_Bottom ();

  // ImGui gets to pass-through without invoking the hook
  if (pTLS->imgui.drawing)
    return false;

  SK_ScopedBool auto_bool (&pTLS->imgui.drawing);
                            pTLS->imgui.drawing = true;


  // Make sure state cleanup happens on the same context, or deferred rendering
  //   will make life miserable!
  pTLS->d3d11.pDevCtx = pDevCtx;

  ///auto HashFromCtx =
  ///  [] ( std::array <uint32_t, SK_D3D11_MAX_DEV_CONTEXTS>& registry,
  ///       UINT                                              dev_idx  ) ->
  ///uint32_t
  ///{
  ///  return registry [dev_idx];
  ///};

  UINT dev_idx =
    SK_D3D11_GetDeviceContextHandle (pDevCtx);

  uint32_t current_vs = SK_D3D11_Shaders.vertex.current.shader   [dev_idx];
  uint32_t current_ps = SK_D3D11_Shaders.pixel.current.shader    [dev_idx];
  uint32_t current_gs = SK_D3D11_Shaders.geometry.current.shader [dev_idx];
  uint32_t current_hs = SK_D3D11_Shaders.hull.current.shader     [dev_idx];
  uint32_t current_ds = SK_D3D11_Shaders.domain.current.shader   [dev_idx];


  auto TriggerReShade_Before = [&]
  {
    if (SK_ReShade_PresentCallback.fn && (! SK_D3D11_Shaders.reshade_triggered [dev_idx]))
    {
      CComPtr <ID3D11DepthStencilState> pDSS = nullptr;
      CComPtr <ID3D11DepthStencilView>  pDSV = nullptr;
      CComPtr <ID3D11RenderTargetView>  pRTV = nullptr;
                                          UINT ref;
      
      pDevCtx->OMGetDepthStencilState (&pDSS, &ref);
      pDevCtx->OMGetRenderTargets     (1, &pRTV, &pDSV);
      
      if (pDSS)
      {
        D3D11_DEPTH_STENCIL_DESC ds_desc;
                 pDSS->GetDesc (&ds_desc);
      
        if ((! pDSV) || (! ds_desc.StencilEnable))
        {
          for (int i = 0 ; i < 5; i++)
          {
            SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *>* pShaderReg;
            uint32_t*                                           current_shader = nullptr;

            switch (i)
            {
              default:
              case 0:
                pShaderReg     = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.vertex;
                current_shader = &current_vs;
                break;
              case 1:
                pShaderReg     = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.pixel;
                current_shader = &current_ps;
                break;
              case 2:
                pShaderReg     = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.geometry;
                current_shader = &current_gs;
                break;
              case 3:
                pShaderReg     = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.hull;
                current_shader = &current_hs;
                break;
              case 4:
                pShaderReg     = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.domain;
                current_shader = &current_ds;
                break;
            };


            if ( *current_shader != 0x0                        &&
               (! pShaderReg->trigger_reshade.before.empty ()) &&
                  pShaderReg->trigger_reshade.before.count (*current_shader) )
            {
              SK_D3D11_Shaders.reshade_triggered [dev_idx] = true;

              SK_ReShade_PresentCallback.explicit_draw.calls++;
              SK_ReShade_PresentCallback.explicit_draw.src_ctx = pDevCtx;
              SK_ReShade_PresentCallback.fn (&SK_ReShade_PresentCallback.explicit_draw);

              break;
            }
          }
        }
      }
    }
  };


  TriggerReShade_Before ();


  if (SK_D3D11_EnableTracking)
  {
    SK_D3D11_DrawThreads.mark ();

    bool rtv_active = false;

    if (tracked_rtv.active_count [dev_idx] > 0)
    {
      rtv_active = true;

      // Reference tracking is only used when the mod tool window is open,
      //   so skip lengthy work that would otherwise be necessary.
      if ( live_rt_view && SK::ControlPanel::D3D11::show_shader_mod_dlg &&
           SK_ImGui_Visible )
      {
        if (current_vs != 0x00) tracked_rtv.ref_vs.insert (current_vs);
        if (current_ps != 0x00) tracked_rtv.ref_ps.insert (current_ps);
        if (current_gs != 0x00) tracked_rtv.ref_gs.insert (current_gs);
        if (current_hs != 0x00) tracked_rtv.ref_hs.insert (current_hs);
        if (current_ds != 0x00) tracked_rtv.ref_ds.insert (current_ds);
      }
    }
  }

  bool on_top           = false;
  bool wireframe        = false;
  bool highlight_shader = (dwFrameTime % tracked_shader_blink_duration > tracked_shader_blink_duration / 2);

  auto GetTracker = [&](int i)
  {
    switch (i)
    {
      default: return &((SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.vertex)->tracked;
      case 1:  return &((SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.pixel)->tracked;
      case 2:  return &((SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.geometry)->tracked;
      case 3:  return &((SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.hull)->tracked;
      case 4:  return &((SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.domain)->tracked;
    }
  };
  
  d3d11_shader_tracking_s* trackers [5] = {
    GetTracker (0), GetTracker (1),
    GetTracker (2),
    GetTracker (3), GetTracker (4)
  };
  
  for (int i = 0; i < 5; i++)
  {
    auto* tracker = trackers [i];

    bool active =
      tracker->active.get (dev_idx);

    if (active)
    {
      tracker->use (nullptr);
 
      if (tracker->cancel_draws) return true;

      if (tracker->wireframe) wireframe = tracker->highlight_draws ? highlight_shader : true;
      if (tracker->on_top)    on_top    = tracker->highlight_draws ? highlight_shader : true;

      if (! (wireframe || on_top))
      {
        if (tracker->highlight_draws && highlight_shader) return highlight_shader;
      }
    }
  }


  if (current_vs != 0x00 && (! SK_D3D11_Shaders.vertex.blacklist.empty   ()) && SK_D3D11_Shaders.vertex.blacklist.count   (current_vs)) return true;
  if (current_ps != 0x00 && (! SK_D3D11_Shaders.pixel.blacklist.empty    ()) && SK_D3D11_Shaders.pixel.blacklist.count    (current_ps)) return true;
  if (current_gs != 0x00 && (! SK_D3D11_Shaders.geometry.blacklist.empty ()) && SK_D3D11_Shaders.geometry.blacklist.count (current_gs)) return true;
  if (current_hs != 0x00 && (! SK_D3D11_Shaders.hull.blacklist.empty     ()) && SK_D3D11_Shaders.hull.blacklist.count     (current_hs)) return true;
  if (current_ds != 0x00 && (! SK_D3D11_Shaders.domain.blacklist.empty   ()) && SK_D3D11_Shaders.domain.blacklist.count   (current_ds)) return true;


  /*if ( SK_D3D11_Shaders.vertex.blacklist_if_texture.size   () ||
       SK_D3D11_Shaders.pixel.blacklist_if_texture.size    () ||
       SK_D3D11_Shaders.geometry.blacklist_if_texture.size () )*/

  if (current_vs != 0x00 && (! SK_D3D11_Shaders.vertex.blacklist_if_texture.empty ()) && SK_D3D11_Shaders.vertex.blacklist_if_texture.count (current_vs))
  {
    auto& views = SK_D3D11_Shaders.vertex.current.views [dev_idx];

    for (auto& it2 : views)
    {
      if (it2 == nullptr)
        continue;

      CComPtr <ID3D11Resource> pRes = nullptr;
            it2->GetResource (&pRes);

      D3D11_RESOURCE_DIMENSION rdv  = D3D11_RESOURCE_DIMENSION_UNKNOWN;
               pRes->GetType (&rdv);

      if (rdv == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
      {
        CComQIPtr <ID3D11Texture2D> pTex (pRes);

        if (SK_D3D11_Textures.Textures_2D.count (pTex) && SK_D3D11_Textures.Textures_2D [pTex].crc32c != 0x0)
        {
          if (SK_D3D11_Shaders.vertex.blacklist_if_texture [current_vs].count (SK_D3D11_Textures.Textures_2D [pTex].crc32c))
            return true;
        }
      }
    }
  }

  if (current_ps != 0x00 && (! SK_D3D11_Shaders.pixel.blacklist_if_texture.empty ()) && SK_D3D11_Shaders.pixel.blacklist_if_texture.count (current_ps))
  {
    auto& views = SK_D3D11_Shaders.pixel.current.views [dev_idx];

    for (auto& it2 : views)
    {
      if (it2 == nullptr)
        continue;

      CComPtr <ID3D11Resource> pRes = nullptr;
            it2->GetResource (&pRes);

      D3D11_RESOURCE_DIMENSION rdv  = D3D11_RESOURCE_DIMENSION_UNKNOWN;
               pRes->GetType (&rdv);

      if (rdv == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
      {
        CComQIPtr <ID3D11Texture2D> pTex (pRes);

        if (SK_D3D11_Textures.Textures_2D.count (pTex) && SK_D3D11_Textures.Textures_2D [pTex].crc32c != 0x0)
        {
          if (SK_D3D11_Shaders.pixel.blacklist_if_texture [current_ps].count (SK_D3D11_Textures.Textures_2D [pTex].crc32c))
            return true;
        }
      }
    }
  }

  if (current_gs != 0x00 && (! SK_D3D11_Shaders.geometry.blacklist_if_texture.empty ()) && SK_D3D11_Shaders.geometry.blacklist_if_texture.count (current_gs))
  {
    auto& views = SK_D3D11_Shaders.geometry.current.views [dev_idx];

    for (auto& it2 : views)
    {
      if (it2 == nullptr)
        continue;

      CComPtr <ID3D11Resource> pRes = nullptr;
            it2->GetResource (&pRes);

      D3D11_RESOURCE_DIMENSION rdv  = D3D11_RESOURCE_DIMENSION_UNKNOWN;
               pRes->GetType (&rdv);

      if (rdv == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
      {
        CComQIPtr <ID3D11Texture2D> pTex (pRes);

        if (SK_D3D11_Textures.Textures_2D.count (pTex) && SK_D3D11_Textures.Textures_2D [pTex].crc32c != 0x00)
        {
          if (SK_D3D11_Shaders.geometry.blacklist_if_texture [current_gs].count (SK_D3D11_Textures.Textures_2D [pTex].crc32c))
            return true;
        }
      }
    }
  }


  if (current_vs != 0 && (! on_top) && (! SK_D3D11_Shaders.vertex.on_top.empty ()) && SK_D3D11_Shaders.vertex.on_top.count(current_vs)) on_top = true;
  if (current_ps != 0 && (! on_top) && (! SK_D3D11_Shaders.pixel.on_top.empty  ()) && SK_D3D11_Shaders.pixel.on_top.count (current_ps)) on_top = true;

  //
  // Hack to track combinations of VS + PS OnTop, not important.
  //
  /////if (on_top)
  /////{
  /////  if (SK_D3D11_Shaders.vertex.tracked.on_top)
  /////  {
  /////    if (SK_D3D11_Shaders.vertex.tracked.crc32c != current_vs) on_top = false;
  /////  }
  /////
  /////  if (SK_D3D11_Shaders.pixel.tracked.on_top)
  /////  {
  /////    if (SK_D3D11_Shaders.pixel.tracked.crc32c != current_ps) on_top = false;
  /////  }
  /////}

  if (current_gs != 0 && (! on_top) && (! SK_D3D11_Shaders.geometry.on_top.empty ()) && SK_D3D11_Shaders.geometry.on_top.count (current_gs)) on_top = true;
  if (current_hs != 0 && (! on_top) && (! SK_D3D11_Shaders.hull.on_top.empty     ()) && SK_D3D11_Shaders.hull.on_top.count     (current_hs)) on_top = true;
  if (current_ds != 0 && (! on_top) && (! SK_D3D11_Shaders.domain.on_top.empty   ()) && SK_D3D11_Shaders.domain.on_top.count   (current_ds)) on_top = true;

  if (on_top)
  {
    CComPtr <ID3D11Device> pDev = nullptr;
    pDevCtx->GetDevice   (&pDev);

    if (pDev != nullptr)
    {
      D3D11_DEPTH_STENCIL_DESC desc = { };

      pDevCtx->OMGetDepthStencilState (&pTLS->d3d11.pDepthStencilStateOrig, &pTLS->d3d11.StencilRefOrig);

      if (pTLS->d3d11.pDepthStencilStateOrig != nullptr)
      {
        pTLS->d3d11.pDepthStencilStateOrig->GetDesc (&desc);

        desc.DepthEnable    = TRUE;
        desc.DepthFunc      = D3D11_COMPARISON_ALWAYS;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        desc.StencilEnable  = FALSE;

        if (SUCCEEDED (pDev->CreateDepthStencilState (&desc, &pTLS->d3d11.pDepthStencilStateNew)))
        {
          pDevCtx->OMSetDepthStencilState (pTLS->d3d11.pDepthStencilStateNew, 0);
        }
      }
    }
  }


  if (current_vs != 0 && (! wireframe) && (! SK_D3D11_Shaders.vertex.wireframe.empty   ()) && SK_D3D11_Shaders.vertex.wireframe.count   (current_vs)) wireframe = true;
  if (current_ps != 0 && (! wireframe) && (! SK_D3D11_Shaders.pixel.wireframe.empty    ()) && SK_D3D11_Shaders.pixel.wireframe.count    (current_ps)) wireframe = true;
  if (current_gs != 0 && (! wireframe) && (! SK_D3D11_Shaders.geometry.wireframe.empty ()) && SK_D3D11_Shaders.geometry.wireframe.count (current_gs)) wireframe = true;
  if (current_hs != 0 && (! wireframe) && (! SK_D3D11_Shaders.hull.wireframe.empty     ()) && SK_D3D11_Shaders.hull.wireframe.count     (current_hs)) wireframe = true;
  if (current_ds != 0 && (! wireframe) && (! SK_D3D11_Shaders.domain.wireframe.empty   ()) && SK_D3D11_Shaders.domain.wireframe.count   (current_ds)) wireframe = true;

  if (wireframe)
  {
    CComPtr <ID3D11Device> pDev = nullptr;
    pDevCtx->GetDevice   (&pDev);

    if (pDev != nullptr)
    {
      pDevCtx->RSGetState         (&pTLS->d3d11.pRasterStateOrig);

      D3D11_RASTERIZER_DESC desc = { };

      if (pTLS->d3d11.pRasterStateOrig != nullptr)
      {
        pTLS->d3d11.pRasterStateOrig->GetDesc (&desc);

        desc.FillMode = D3D11_FILL_WIREFRAME;
        desc.CullMode = D3D11_CULL_NONE;
        //desc.FrontCounterClockwise = TRUE;
        //desc.DepthClipEnable       = FALSE;

        if (SUCCEEDED (pDev->CreateRasterizerState (&desc, &pTLS->d3d11.pRasterStateNew)))
        {
          pDevCtx->RSSetState (pTLS->d3d11.pRasterStateNew);
        }
      }
    }
  }

  uint32_t current_shaders [5] = {
    current_vs, current_ps,
    current_gs,
    current_hs, current_ds
  };

  for (int i = 0; i < 5; i++)
  {
    if (current_shaders [i] == 0x00)
      continue;

    auto* tracker = trackers [i];

    std::vector <d3d11_shader_tracking_s::cbuffer_override_s> overrides;

    if (tracker->crc32c == current_shaders [i])
    {
      for ( auto& ovr : tracker->overrides )
      {
        if (ovr.Enable && ovr.parent == tracker->crc32c)
        {
          if (ovr.Slot >= 0 && ovr.Slot < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT)
          {
            overrides.push_back (ovr);
          }
        }
      }
    }

    if ((1 << i) == (int)sk_shader_class::Pixel && (! __SK_D3D11_PixelShader_CBuffer_Overrides.empty ()) )
    {
      for ( auto& ovr : __SK_D3D11_PixelShader_CBuffer_Overrides )
      {
        if (ovr.parent == current_ps && ovr.Enable)
        {
          if (ovr.Slot >= 0 && ovr.Slot < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT)
          {
            overrides.push_back (ovr);
          }
        }
      }
    }

    if (! overrides.empty ())
    {
      ID3D11Buffer* pConstantBuffers [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };
      UINT          pFirstConstant   [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { },
                    pNumConstants    [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };
      ID3D11Buffer* pConstantCopies  [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };

      auto _GetConstantBuffers = [&](int i, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants) ->
      void
      {
        CComQIPtr <ID3D11DeviceContext1> pDevCtx1 (pDevCtx);

        if (pDevCtx1)
        {
          switch (i)
          {
            case 0:
              pDevCtx1->VSGetConstantBuffers1 (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers, pFirstConstant, pNumConstants);
              break;
            case 1:
              pDevCtx1->PSGetConstantBuffers1 (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers, pFirstConstant, pNumConstants);
              break;
            case 2:
              pDevCtx1->GSGetConstantBuffers1 (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers, pFirstConstant, pNumConstants);
              break;
            case 3:
              pDevCtx1->HSGetConstantBuffers1 (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers, pFirstConstant, pNumConstants);
              break;
            case 4:
              pDevCtx1->DSGetConstantBuffers1 (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers, pFirstConstant, pNumConstants);
              break;
          }

          for ( int j = 0 ; j < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT ; ++j )
          {
            if (ppConstantBuffers [j] != nullptr)
            {
              // Expected non-D3D11.1+ behavior ( Any game that supplies a different value is using code that REQUIRES the D3D11.1 runtime
              //                                    and will require additional state tracking in future versions of Special K )
              if (pFirstConstant [j] != 0)
              {
                dll_log.Log ( L"Detected non-zero first constant offset: %lu on CBuffer slot %lu for shader type %lu",
                                pFirstConstant [j], j, i );
              }

              // Expected non-D3D11.1+ behavior
              if (pNumConstants [j] != 4096)
              {
                dll_log.Log ( L"Detected non-4096 num constants: %lu on CBuffer slot %lu for shader type %lu",
                                pNumConstants [j], j, i );
              }
            }
          }
        }

        else
        {
          switch (i)
          { case 0:
              pDevCtx->VSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers);
              break;
            case 1:
              pDevCtx->PSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers);
              break;
            case 2:
              pDevCtx->GSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers);
              break;
            case 3:
              pDevCtx->HSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers);
              break;
            case 4:
              pDevCtx->DSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers);
              break;
          }
        }
      };

      auto _SetConstantBuffers = [&](int i, ID3D11Buffer** ppConstantBuffers) ->
      void
      {
        switch (i)
        { case 0:
            pDevCtx->VSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers);
            break;
          case 1:
            pDevCtx->PSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers);
            break;
          case 2:
            pDevCtx->GSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers);
            break;
          case 3:
            pDevCtx->HSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers);
            break;
          case 4:
            pDevCtx->DSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers);
            break;
        }
      };

      _GetConstantBuffers (i, pConstantBuffers, pFirstConstant, pNumConstants);

      for ( UINT j = 0 ; j < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT ; ++j )
      {
        if (pConstantBuffers [j] == nullptr) continue;// || (! used_slots [j])) continue;

        D3D11_BUFFER_DESC                   buff_desc  = { };
            pConstantBuffers [j]->GetDesc (&buff_desc);

        buff_desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
        buff_desc.Usage               = D3D11_USAGE_DYNAMIC;
        buff_desc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
        buff_desc.MiscFlags           = 0x0;
        buff_desc.StructureByteStride = 0;

        CComPtr <ID3D11Device> pDev;
        pDevCtx->GetDevice   (&pDev);

        D3D11_MAP                map_type   = D3D11_MAP_WRITE_DISCARD;
        D3D11_MAPPED_SUBRESOURCE mapped_sub = { };
        HRESULT                  hrMap      = E_FAIL;

        bool used       = false;
        UINT start_addr = buff_desc.ByteWidth-1;
        UINT end_addr   = 0;

        for ( auto& ovr : overrides )
        {
          if ( ovr.Slot == j && ovr.Enable )
          {
            if (ovr.StartAddr < start_addr)
              start_addr = ovr.StartAddr;

            if (ovr.Size + ovr.StartAddr > end_addr)
              end_addr   = ovr.Size + ovr.StartAddr;

            used = true;
          }
        }

        if (used)
        {
          pTLS->d3d11.pOriginalCBuffers [i][j] = pConstantBuffers [j];

          if (SUCCEEDED (pDev->CreateBuffer
                                         (&buff_desc, nullptr, &pConstantCopies [j])))
          {
            if (pConstantBuffers [j] != nullptr)
            {
              if (ReadAcquire (&__SKX_ComputeAntiStall))
              {
                D3D11_BOX src = { };

                src.left   = 0;
                src.right  = buff_desc.ByteWidth;
                src.top    = 0;
                src.bottom = 1;
                src.front  = 0;
                src.back   = 1;

                pDevCtx->CopySubresourceRegion ( pConstantCopies  [j], 0, 0, 0, 0,
                                                 pConstantBuffers [j], 0, &src );
              }

              else
              {
                pDevCtx->CopyResource ( pConstantCopies [j], pConstantBuffers [j] );
              }
            }

            hrMap = pDevCtx->Map ( pConstantCopies [j], 0,
                                     map_type, 0x0,
                                       &mapped_sub );
          }

          if (SUCCEEDED (hrMap))
          {
            for ( auto& ovr : overrides )
            {
              if ( ovr.Slot == j && mapped_sub.pData != nullptr )
              {
                void*   pBase  =
                  ((uint8_t *)mapped_sub.pData   +   ovr.StartAddr);
                SK_SEH_Guarded_memcpy (pBase, ovr.Values, std::min (ovr.Size, 64ui32));
              }
            }

            pDevCtx->Unmap (pConstantCopies [j], 0);
          }
        }
      }

      _SetConstantBuffers (i, pConstantCopies);

      for (int k = 0; k < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; k++)
      {
        if (pConstantCopies [k] != nullptr)
        {
          pConstantCopies [k]->Release ();
          pConstantCopies [k] = nullptr;
        }
      }
    }
  }

  SK_ExecuteReShadeOnReturn easy_reshade (pDevCtx, dev_idx, pTLS);

  return false;
}

void
SK_D3D11_PostDispatch (ID3D11DeviceContext* pDevCtx, SK_TLS* pTLS = SK_TLS_Bottom ())
{
  UINT dev_idx =
    SK_D3D11_GetDeviceContextHandle (pDevCtx);

  if (SK_D3D11_Shaders.compute.tracked.active.get (dev_idx))
  {
    if (pTLS->d3d11.pOriginalCBuffers [5][0] != nullptr)
    {
      if ((intptr_t)pTLS->d3d11.pOriginalCBuffers [5][0] == -1)
      {
        pTLS->d3d11.pOriginalCBuffers [5][0] = nullptr;
      }

      pDevCtx->CSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, &pTLS->d3d11.pOriginalCBuffers [5][0]);

      for (int j = 0; j < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++j)
      {
        if (pTLS->d3d11.pOriginalCBuffers [5][j] != nullptr)
        {
          pTLS->d3d11.pOriginalCBuffers [5][j]->Release ();
          pTLS->d3d11.pOriginalCBuffers [5][j] = nullptr;
        }
      }
    }
  }
}

bool
SK_D3D11_DispatchHandler ( ID3D11DeviceContext* pDevCtx,
                           SK_TLS*              pTLS = SK_TLS_Bottom () )
{
  SK_D3D11_DispatchThreads.mark ();

  UINT dev_idx =
    SK_D3D11_GetDeviceContextHandle (pDevCtx);

  if (SK_D3D11_EnableTracking)
  {
    bool rtv_active = false;

    if (tracked_rtv.active_count [dev_idx] > 0)
    {
      rtv_active = true;

      if (SK_D3D11_Shaders.compute.current.shader [dev_idx] != 0x00)
        tracked_rtv.ref_cs.insert (SK_D3D11_Shaders.compute.current.shader [dev_idx]);
    }

    if (SK_D3D11_Shaders.compute.tracked.active.get (dev_idx)) { SK_D3D11_Shaders.compute.tracked.use (nullptr); }
  }

  SK_ScopedBool auto_bool (&pTLS->imgui.drawing);
                            pTLS->imgui.drawing = true;

  bool highlight_shader =
    (dwFrameTime % tracked_shader_blink_duration > tracked_shader_blink_duration / 2);


  uint32_t current_cs = SK_D3D11_Shaders.compute.current.shader [dev_idx];

  if (SK_D3D11_Shaders.compute.blacklist.count (current_cs)) return true;

  d3d11_shader_tracking_s*
    tracker = &SK_D3D11_Shaders.compute.tracked;

  if (tracker->crc32c == current_cs)
  {
    if (SK_D3D11_Shaders.compute.tracked.highlight_draws && highlight_shader) return true;
    if (SK_D3D11_Shaders.compute.tracked.cancel_draws)                        return true;

    std::vector <d3d11_shader_tracking_s::cbuffer_override_s> overrides;
    size_t used_slots [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };

    for ( auto& ovr : tracker->overrides )
    {
      if (ovr.Enable && ovr.parent == tracker->crc32c)
      {
        if (ovr.Slot >= 0 && ovr.Slot < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT)
        {
                   used_slots [ovr.Slot] = ovr.BufferSize;
          overrides.push_back (ovr);
        }
      }
    }

    if (! overrides.empty ())
    {
      ID3D11Buffer* pConstantBuffers [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };
      ID3D11Buffer* pConstantCopies  [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };

      pDevCtx->CSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pConstantBuffers);
      pDevCtx->CSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pConstantCopies);

      for ( UINT j = 0 ; j < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT ; j++ )
      {
        pTLS->d3d11.pOriginalCBuffers [5][j] = pConstantBuffers [j];

        if (j == 0 && pConstantBuffers [j] == nullptr) pTLS->d3d11.pOriginalCBuffers [5][j] = (ID3D11Buffer *)(intptr_t)-1;

            pConstantCopies  [j]  = nullptr;
        if (pConstantBuffers [j] == nullptr && (! used_slots [j])) continue;

        D3D11_BUFFER_DESC                   buff_desc  = { };

        if (pConstantBuffers [j])
            pConstantBuffers [j]->GetDesc (&buff_desc);

        buff_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        buff_desc.Usage          = D3D11_USAGE_DYNAMIC;
        buff_desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;

        CComPtr <ID3D11Device> pDev;
        pDevCtx->GetDevice   (&pDev);

        D3D11_MAP                map_type   = D3D11_MAP_WRITE_DISCARD;
        D3D11_MAPPED_SUBRESOURCE mapped_sub = { };
        HRESULT                  hrMap      = E_FAIL;

        bool used       = false;
        UINT start_addr = buff_desc.ByteWidth-1;
        UINT end_addr   = 0;

        for ( auto& ovr : overrides )
        {
          if ( ovr.Slot == j && ovr.Enable )
          {
            if (ovr.StartAddr < start_addr)
              start_addr = ovr.StartAddr;

            if (ovr.Size + ovr.StartAddr > end_addr)
              end_addr   = ovr.Size + ovr.StartAddr;

            used = true;
          }
        }

        if (used)
        {
          if (SUCCEEDED (pDev->CreateBuffer
                                         (&buff_desc, nullptr, &pConstantCopies [j])))
          {
            if (pConstantBuffers [j] != nullptr)
            {
              if (ReadAcquire (&__SKX_ComputeAntiStall))
              {
                D3D11_BOX src = { };

                src.left   = 0;
                src.right  = buff_desc.ByteWidth;
                src.top    = 0;
                src.bottom = 1;
                src.front  = 0;
                src.back   = 1;

                pDevCtx->CopySubresourceRegion ( pConstantCopies  [j], 0, 0, 0, 0,
                                                 pConstantBuffers [j], 0, &src );
              }

              else
              {
                pDevCtx->CopyResource ( pConstantCopies [j], pConstantBuffers [j] );
              }
            }

            hrMap = pDevCtx->Map ( pConstantCopies [j], 0,
                                     map_type, 0x0,
                                       &mapped_sub );
          }

          if (SUCCEEDED (hrMap))
          {
            for ( auto& ovr : overrides )
            {
              if ( ovr.Slot == j && mapped_sub.pData != nullptr )
              {
                void*   pBase  = ((uint8_t *)mapped_sub.pData + ovr.StartAddr);

                SK_SEH_Guarded_memcpy (pBase, ovr.Values, ovr.Size);
              }
            }

            pDevCtx->Unmap (pConstantCopies [j], 0);
          }

          else if (pConstantCopies [j] != nullptr)
          {
            dll_log.Log (L"Failure To Copy Resource");
            pConstantCopies [j]->Release ();
            pConstantCopies [j] = nullptr;
          }
        }
      }

      pDevCtx->CSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pConstantCopies);

      for (int k = 0; k < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; k++)
      {
        if (pConstantCopies [k] != nullptr)
        {
          pConstantCopies [k]->Release ();
          pConstantCopies [k] = nullptr;
        }
      }
    }
  }

  return false;
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawAuto_Override (_In_ ID3D11DeviceContext *This)
{
  SK_LOG_FIRST_CALL

  SK_TLS *pTLS = nullptr;

  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
  {
    return
      D3D11_DrawAuto_Original (This);
  }

  if (SK_D3D11_DrawHandler (This, pTLS))
    return;

  SK_ReShade_DrawCallback.call (This, SK_D3D11_ReshadeDrawFactors.auto_draw, pTLS);

  D3D11_DrawAuto_Original (This);

  SK_D3D11_PostDraw (pTLS);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawIndexed_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation )
{
  SK_LOG_FIRST_CALL

  SK_TLS *pTLS = nullptr;

  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
  {
    return
      D3D11_DrawIndexed_Original ( This, IndexCount,
                                     StartIndexLocation,
                                       BaseVertexLocation );
  }

  if (SK_D3D11_DrawHandler (This, pTLS))
    return;

  SK_ReShade_DrawCallback.call (This, IndexCount * SK_D3D11_ReshadeDrawFactors.indexed, pTLS);

  D3D11_DrawIndexed_Original ( This, IndexCount,
                                       StartIndexLocation,
                                         BaseVertexLocation );

  SK_D3D11_PostDraw (pTLS);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_Draw_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCount,
  _In_ UINT                 StartVertexLocation )
{
  SK_LOG_FIRST_CALL

  SK_TLS *pTLS = nullptr;

  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
  {
    return
      D3D11_Draw_Original ( This,
                              VertexCount,
                                StartVertexLocation );
  }

  if (SK_D3D11_DrawHandler (This, pTLS))
    return;

  SK_ReShade_DrawCallback.call (This, VertexCount * SK_D3D11_ReshadeDrawFactors.draw, pTLS);

  D3D11_Draw_Original ( This, VertexCount, StartVertexLocation );

  SK_D3D11_PostDraw (pTLS);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawIndexedInstanced_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation,
  _In_ UINT                 StartInstanceLocation )
{
  SK_LOG_FIRST_CALL

  SK_TLS *pTLS = nullptr;

  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
  {
    return
      D3D11_DrawIndexedInstanced_Original ( This, IndexCountPerInstance,
                                              InstanceCount, StartIndexLocation,
                                                BaseVertexLocation, StartInstanceLocation );
  }

  if (SK_D3D11_DrawHandler (This, pTLS))
    return;

  SK_ReShade_DrawCallback.call (This, IndexCountPerInstance * InstanceCount * SK_D3D11_ReshadeDrawFactors.indexed_instanced, pTLS);

  D3D11_DrawIndexedInstanced_Original ( This, IndexCountPerInstance,
                                          InstanceCount, StartIndexLocation,
                                            BaseVertexLocation, StartInstanceLocation );

  SK_D3D11_PostDraw (pTLS);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawIndexedInstancedIndirect_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs )
{
  SK_LOG_FIRST_CALL

  SK_TLS *pTLS = nullptr;

  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
  {
    return
      D3D11_DrawIndexedInstancedIndirect_Original ( This, pBufferForArgs,
                                                      AlignedByteOffsetForArgs );
  }

  if (SK_D3D11_DrawHandler (This, pTLS))
    return;

  SK_ReShade_DrawCallback.call (This, SK_D3D11_ReshadeDrawFactors.indexed_instanced_indirect, pTLS);

  D3D11_DrawIndexedInstancedIndirect_Original ( This, pBufferForArgs,
                                                  AlignedByteOffsetForArgs );

  SK_D3D11_PostDraw (pTLS);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawInstanced_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartVertexLocation,
  _In_ UINT                 StartInstanceLocation )
{
  SK_LOG_FIRST_CALL

  SK_TLS *pTLS = nullptr;

  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
  {
    return
      D3D11_DrawInstanced_Original ( This, VertexCountPerInstance,
                                       InstanceCount, StartVertexLocation,
                                         StartInstanceLocation );
  }

  if (SK_D3D11_DrawHandler (This, pTLS))
    return;

  SK_ReShade_DrawCallback.call (This, VertexCountPerInstance * InstanceCount * SK_D3D11_ReshadeDrawFactors.instanced, pTLS);

  D3D11_DrawInstanced_Original ( This, VertexCountPerInstance,
                                   InstanceCount, StartVertexLocation,
                                     StartInstanceLocation );

  SK_D3D11_PostDraw (pTLS);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawInstancedIndirect_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs )
{
  SK_LOG_FIRST_CALL

  SK_TLS *pTLS = nullptr;

  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
  {
    return
      D3D11_DrawInstancedIndirect_Original ( This, pBufferForArgs,
                                               AlignedByteOffsetForArgs );
  }

  if (SK_D3D11_DrawHandler (This, pTLS))
    return;

  SK_ReShade_DrawCallback.call (This, SK_D3D11_ReshadeDrawFactors.instanced_indirect, pTLS);

  D3D11_DrawInstancedIndirect_Original ( This, pBufferForArgs,
                                           AlignedByteOffsetForArgs );

  SK_D3D11_PostDraw (pTLS);
}


__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_Dispatch_Override ( _In_ ID3D11DeviceContext *This,
                          _In_ UINT                 ThreadGroupCountX,
                          _In_ UINT                 ThreadGroupCountY,
                          _In_ UINT                 ThreadGroupCountZ )
{
  SK_LOG_FIRST_CALL

  SK_TLS *pTLS = nullptr;

  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
  {
    return
      D3D11_Dispatch_Original ( This,
                                  ThreadGroupCountX,
                                    ThreadGroupCountY,
                                      ThreadGroupCountZ );
  }

  if (SK_D3D11_DispatchHandler (This, pTLS))
    return;

  SK_ReShade_DrawCallback.call (This, 64, pTLS);//std::max (1ui32, ThreadGroupCountX) * std::max (1ui32, ThreadGroupCountY) * std::max (1ui32, ThreadGroupCountZ) );

  D3D11_Dispatch_Original ( This,
                              ThreadGroupCountX,
                                ThreadGroupCountY,
                                  ThreadGroupCountZ );

  SK_D3D11_PostDispatch (This, pTLS);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DispatchIndirect_Override ( _In_ ID3D11DeviceContext *This,
                                  _In_ ID3D11Buffer        *pBufferForArgs,
                                  _In_ UINT                 AlignedByteOffsetForArgs )
{
  SK_LOG_FIRST_CALL

  SK_TLS *pTLS = nullptr;

  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
  {
    return
      D3D11_DispatchIndirect_Original ( This,
                                          pBufferForArgs,
                                            AlignedByteOffsetForArgs );
  }

  if (SK_D3D11_DispatchHandler (This, pTLS))
    return;

  SK_ReShade_DrawCallback.call (This, 64, pTLS);

  D3D11_DispatchIndirect_Original ( This,
                                      pBufferForArgs,
                                        AlignedByteOffsetForArgs );

  SK_D3D11_PostDispatch (This, pTLS);
}



__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_OMSetRenderTargets_Override (
         ID3D11DeviceContext           *This,
_In_     UINT                           NumViews,
_In_opt_ ID3D11RenderTargetView *const *ppRenderTargetViews,
_In_opt_ ID3D11DepthStencilView        *pDepthStencilView )
{
  ID3D11DepthStencilView *pDSV =
    pDepthStencilView;

  SK_TLS *pTLS = nullptr;

  UINT dev_idx =
    SK_D3D11_GetDeviceContextHandle (This);

  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
  {
    if (tracked_rtv.active_count [dev_idx] > 0)
    {
      for (auto&& rtv : tracked_rtv.active [dev_idx]) rtv = false;

      tracked_rtv.active_count [dev_idx] = 0;
    }

    D3D11_OMSetRenderTargets_Original (
      This, NumViews, ppRenderTargetViews,
        pDSV );

    return;
  }

  if (pDepthStencilView != nullptr)
    SK_ReShade_SetDepthStencilViewCallback.call (pDSV, pTLS);

  D3D11_OMSetRenderTargets_Original (
    This, NumViews, ppRenderTargetViews,
      pDSV );

  if (NumViews > 0)
  {
    if (ppRenderTargetViews)
    {
      auto&                              rt_views =
        SK_D3D11_RenderTargets [dev_idx].rt_views;

      auto* tracked_rtv_res  = 
        static_cast <ID3D11RenderTargetView *> (
          ReadPointerAcquire ((volatile PVOID *)&tracked_rtv.resource)
        );

      for (UINT i = 0; i < NumViews; i++)
      {
        if (ppRenderTargetViews [i] && rt_views.emplace (ppRenderTargetViews [i]).second)
            ppRenderTargetViews [i]->AddRef ();

        bool active_before = tracked_rtv.active_count [dev_idx] > 0 ? 
                             tracked_rtv.active       [dev_idx][i].load ()
                                                                    : false;

        bool active = 
          ( tracked_rtv_res == ppRenderTargetViews [i] ) ?
                       true :
                              false;

        if (active_before != active)
        {
          tracked_rtv.active [dev_idx][i] = active;

          if      (            active                    ) tracked_rtv.active_count [dev_idx]++;
          else if (tracked_rtv.active_count [dev_idx] > 0) tracked_rtv.active_count [dev_idx]--;
        }
      }

      for ( UINT j = 0; j < 5 ; j++ )
      {
        switch (j)
        { case 0:
          {
            INT  pre_hud_slot  = SK_D3D11_Shaders.vertex.tracked.pre_hud_rt_slot;
            if ( pre_hud_slot >= 0 && pre_hud_slot < (INT)NumViews )
            {
              if (SK_D3D11_Shaders.vertex.tracked.pre_hud_rtv == nullptr &&
                  SK_D3D11_Shaders.vertex.current.shader [dev_idx] == 
                  SK_D3D11_Shaders.vertex.tracked.crc32c.load () )
              {
                if (ppRenderTargetViews [pre_hud_slot] != nullptr)
                {
                  SK_D3D11_Shaders.vertex.tracked.pre_hud_rtv = ppRenderTargetViews [pre_hud_slot];
                  SK_D3D11_Shaders.vertex.tracked.pre_hud_rtv->AddRef ();
                }
              }
            }

          } break;

          case 1:
          {
            INT  pre_hud_slot  = SK_D3D11_Shaders.pixel.tracked.pre_hud_rt_slot;
            if ( pre_hud_slot >= 0 && pre_hud_slot < (INT)NumViews )
            {
              if (SK_D3D11_Shaders.pixel.tracked.pre_hud_rtv == nullptr &&
                  SK_D3D11_Shaders.pixel.current.shader [dev_idx] == 
                  SK_D3D11_Shaders.pixel.tracked.crc32c.load () )
              {
                if (ppRenderTargetViews [pre_hud_slot] != nullptr)
                {
                  SK_D3D11_Shaders.pixel.tracked.pre_hud_rtv = ppRenderTargetViews [pre_hud_slot];
                  SK_D3D11_Shaders.pixel.tracked.pre_hud_rtv->AddRef ();
                }
              }
            }
          } break;

          default:
           break;
        }
      }
    }

    if (pDepthStencilView)
    {
      auto& ds_views =
        SK_D3D11_RenderTargets [dev_idx].ds_views;

      if (ds_views.emplace (pDepthStencilView).second)
                            pDepthStencilView->AddRef ();
    }
  }
}

//std::array <bool, SK_D3D11_MAX_DEV_CONTEXTS> SK_D3D11_KnownShaders::reshade_triggered;

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Override ( ID3D11DeviceContext              *This,
                                            _In_           UINT                              NumRTVs,
                                            _In_opt_       ID3D11RenderTargetView    *const *ppRenderTargetViews,
                                            _In_opt_       ID3D11DepthStencilView           *pDepthStencilView,
                                            _In_           UINT                              UAVStartSlot,
                                            _In_           UINT                              NumUAVs,
                                            _In_opt_       ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
                                            _In_opt_ const UINT                             *pUAVInitialCounts )
{
  ID3D11DepthStencilView *pDSV = pDepthStencilView;

  SK_TLS *pTLS = nullptr;

  UINT dev_idx =
    SK_D3D11_GetDeviceContextHandle (This);

  // ImGui gets to pass-through without invoking the hook
  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
  {
    for (auto&& i : tracked_rtv.active [dev_idx]) i = false;

    tracked_rtv.active_count [dev_idx] = 0;

    
    D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Original (
      This, NumRTVs, ppRenderTargetViews,
        pDSV, UAVStartSlot, NumUAVs,
          ppUnorderedAccessViews, pUAVInitialCounts
    );

    return;
  }

  if (pDepthStencilView != nullptr)
    SK_ReShade_SetDepthStencilViewCallback.call (pDSV, pTLS);

  D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Original (
    This, NumRTVs, ppRenderTargetViews,
      pDSV, UAVStartSlot, NumUAVs,
        ppUnorderedAccessViews, pUAVInitialCounts
  );

  if (NumRTVs > 0)
  {
    if (ppRenderTargetViews)
    {
      auto&                              rt_views =
        SK_D3D11_RenderTargets [dev_idx].rt_views;

      auto* tracked_rtv_res = 
        static_cast <ID3D11RenderTargetView *> (
          ReadPointerAcquire ((volatile PVOID *)&tracked_rtv.resource)
        );

      for (UINT i = 0; i < NumRTVs; i++)
      {
        if (ppRenderTargetViews [i] && rt_views.emplace (ppRenderTargetViews [i]).second)
            ppRenderTargetViews [i]->AddRef ();

        bool active_before = tracked_rtv.active_count [dev_idx] > 0 ? 
                             tracked_rtv.active       [dev_idx][i].load ()
                                                                    : false;

        bool active = 
          ( tracked_rtv_res == ppRenderTargetViews [i] ) ?
                       true :
                              false;

        if (active_before != active)
        {
          tracked_rtv.active [dev_idx][i] = active;

          if      (            active                    ) tracked_rtv.active_count [dev_idx]++;
          else if (tracked_rtv.active_count [dev_idx] > 0) tracked_rtv.active_count [dev_idx]--;
        }
      }
    }

    if (pDepthStencilView)
    {
      auto& ds_views =
        SK_D3D11_RenderTargets [dev_idx].ds_views;

      if (! ds_views.count (pDepthStencilView))
      {
                         pDepthStencilView->AddRef ();
        ds_views.insert (pDepthStencilView);
      }
    }
  }
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_OMGetRenderTargets_Override (ID3D11DeviceContext     *This,
                              _In_ UINT                     NumViews,
                             _Out_ ID3D11RenderTargetView **ppRenderTargetViews,
                             _Out_ ID3D11DepthStencilView **ppDepthStencilView)
{
  D3D11_OMGetRenderTargets_Original (This, NumViews, ppRenderTargetViews, ppDepthStencilView);

  if (ppDepthStencilView != nullptr)
    SK_ReShade_GetDepthStencilViewCallback.try_call (*ppDepthStencilView);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_OMGetRenderTargetsAndUnorderedAccessViews_Override (ID3D11DeviceContext        *This,
                                                    _In_  UINT                        NumRTVs,
                                                    _Out_ ID3D11RenderTargetView    **ppRenderTargetViews,
                                                    _Out_ ID3D11DepthStencilView    **ppDepthStencilView,
                                                    _In_  UINT                        UAVStartSlot,
                                                    _In_  UINT                        NumUAVs,
                                                    _Out_ ID3D11UnorderedAccessView **ppUnorderedAccessViews)
{
  D3D11_OMGetRenderTargetsAndUnorderedAccessViews_Original (This, NumRTVs, ppRenderTargetViews, ppDepthStencilView, UAVStartSlot, NumUAVs, ppUnorderedAccessViews);

  if (ppDepthStencilView != nullptr)
    SK_ReShade_GetDepthStencilViewCallback.try_call (*ppDepthStencilView);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_ClearDepthStencilView_Override (ID3D11DeviceContext    *This,
                                 _In_ ID3D11DepthStencilView *pDepthStencilView,
                                 _In_ UINT                    ClearFlags,
                                 _In_ FLOAT                   Depth,
                                 _In_ UINT8                   Stencil)
{
  SK_ReShade_ClearDepthStencilViewCallback.try_call (pDepthStencilView);

  D3D11_ClearDepthStencilView_Original (This, pDepthStencilView, ClearFlags, Depth, Stencil);
}

extern bool __SK_Y0_1024_512;
extern bool __SK_Y0_1024_768;
extern bool __SK_Y0_960_540;

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_RSSetViewports_Override (
        ID3D11DeviceContext* This,
        UINT                 NumViewports,
  const D3D11_VIEWPORT*      pViewports )
{
  if (SK_GetCurrentGameID () == SK_GAME_ID::Yakuza0)
  {
    if (NumViewports > 1)
    {
      dll_log.Log (L" *** More than one viewport set at a time! (%lu)", NumViewports);

      for (int i = 0; i < NumViewports; i++)
      {
        dll_log.Log ( L"\t)>~ [%i] (%f x %f) | <%f, %f> | [%f - %f]",
                      i,  pViewports [i].Width,    pViewports [i].Height,
                          pViewports [i].TopLeftX, pViewports [i].TopLeftY,
                          pViewports [i].MinDepth, pViewports [i].MaxDepth );
      }
    }

    //if (NumViewports == 1)
    //{
    //  dll_log.Log ( L"  VP_XXX -- (%8.3f x %8.3f) | <%5.2f, %5.2f> | [%3.1f - %3.1f]",
    //               pViewports [0].Width,    pViewports [0].Height,
    //               pViewports [0].TopLeftX, pViewports [0].TopLeftY,
    //               pViewports [0].MinDepth, pViewports [0].MaxDepth );
    //}
  }
#if 0
  if (NumViewports > 0)
  {
    D3D11_VIEWPORT* vps = new D3D11_VIEWPORT [NumViewports];
    for (UINT i = 0; i < NumViewports; i++)
    {
      vps [i] = pViewports [i];

      if (! (config.render.dxgi.res.max.isZero () || config.render.dxgi.res.min.isZero ()))
      {
        vps [i].Height /= ((float)config.render.dxgi.res.min.y/(float)config.render.dxgi.res.max.y);
        vps [i].Width  /= ((float)config.render.dxgi.res.min.x/(float)config.render.dxgi.res.max.x);
      }
    }

    D3D11_RSSetViewports_Original (This, NumViewports, vps);

    delete [] vps;

    return;
  }

  else
#endif
  D3D11_RSSetViewports_Original (This, NumViewports, pViewports);
}

void WINAPI SK_D3D11_SetResourceRoot      (const wchar_t* root);
void WINAPI SK_D3D11_EnableTexDump        (bool enable);
void WINAPI SK_D3D11_EnableTexInject      (bool enable);
void WINAPI SK_D3D11_EnableTexCache       (bool enable);
void WINAPI SK_D3D11_PopulateResourceList (bool refresh = false);

#include <unordered_set>
#include <unordered_map>
#include <map>

__forceinline
SK_D3D11_TexMgr_Singleton&
__SK_Singleton_D3D11_Textures (void)
{
  static SK_D3D11_TexMgr _SK_D3D11_Textures;
  return                 _SK_D3D11_Textures;
}


class SK_D3D11_TexCacheMgr {
public:
};


std::string
SK_D3D11_SummarizeTexCache (void)
{
  char szOut [512] { };

  snprintf ( szOut, 511, "  Tex Cache: %#5llu MiB   Entries:   %#7li\n"
                         "  Hits:      %#5li       Time Saved: %#7.01lf ms\n"
                         "  Evictions: %#5lu",
               SK_D3D11_Textures.AggregateSize_2D >> 20ULL,
               SK_D3D11_Textures.Entries_2D.load        (),
               SK_D3D11_Textures.RedundantLoads_2D.load (),
               SK_D3D11_Textures.RedundantTime_2D,
               SK_D3D11_Textures.Evicted_2D.load        () );

  return std::move (szOut);
}

#include <SpecialK/utility.h>

bool         SK_D3D11_need_tex_reset      = false;
bool         SK_D3D11_try_tex_reset       = false; // Don't need, but would be beneficial to try
int32_t      SK_D3D11_amount_to_purge     = 0L;

bool         SK_D3D11_dump_textures       = false;// true;
bool         SK_D3D11_inject_textures_ffx = false;
bool         SK_D3D11_inject_textures     = false;
bool         SK_D3D11_cache_textures      = false;
bool         SK_D3D11_mark_textures       = false;
std::wstring SK_D3D11_res_root            = L"SK_Res";

bool
SK_D3D11_TexMgr::isTexture2D (uint32_t crc32, const D3D11_TEXTURE2D_DESC *pDesc)
{
  if (! (SK_D3D11_cache_textures && crc32))
    return false;

  return HashMap_2D [pDesc->MipLevels].contains (crc32);
}

void
__stdcall
SK_D3D11_ResetTexCache (void)
{
//  SK_D3D11_need_tex_reset = true;
  SK_D3D11_try_tex_reset = true;
  SK_D3D11_Textures.reset ();
}


static volatile ULONG live_textures_dirty = FALSE;

void
__stdcall
SK_D3D11_TexCacheCheckpoint (void)
{
  IDXGIDevice  *pDXGIDev = nullptr;
  ID3D11Device *pDevice  = nullptr;

  if (SK_GetCurrentRenderBackend ().device != nullptr)
      SK_GetCurrentRenderBackend ().device->QueryInterface <ID3D11Device> (&pDevice);
  if ( config.textures.cache.residency_managemnt &&
      pDevice &&        SUCCEEDED (pDevice->QueryInterface <IDXGIDevice>  (&pDXGIDev)) && pDXGIDev != nullptr)
  {
    static DWORD dwLastEvict = 0;
    static DWORD dwLastTest  = 0;
    static DWORD dwLastSize  = SK_D3D11_Textures.Entries_2D.load ();

    static DWORD dwInitiateEvict = 0;
    static DWORD dwInitiateSize  = 0;

    DWORD dwNow = timeGetTime ();

    const int MAX_TEXTURES_PER_PASS = 32UL;

    static size_t cur_tex = 0;

    if ( ( SK_D3D11_Textures.Evicted_2D.load () != dwLastEvict || 
           SK_D3D11_Textures.Entries_2D.load () != dwLastSize     ) &&
                                            dwLastTest < dwNow - 666L )
    {
      if (cur_tex == 0)
      {
        dwInitiateEvict = SK_D3D11_Textures.Evicted_2D.load ();
        dwInitiateSize  = SK_D3D11_Textures.Entries_2D.load ();
      }

      static LONG fully_resident = 0;
      static LONG shared_memory  = 0;
      static LONG on_disk        = 0;

      static LONG64 size_vram   = 0ULL;
      static LONG64 size_shared = 0ULL;
      static LONG64 size_disk   = 0ULL;

      static std::array <IUnknown *,                           MAX_TEXTURES_PER_PASS + 2> residency_tests;
      static std::array <SK_D3D11_TexMgr::tex2D_descriptor_s*, MAX_TEXTURES_PER_PASS + 2> residency_descs;
      static std::array <DXGI_RESIDENCY, MAX_TEXTURES_PER_PASS + 2> residency_results;

      size_t record_count = 0;

      auto record =
        residency_tests.begin ();
      auto desc =
        residency_descs.begin ();

      size_t start_idx = cur_tex;
      size_t idx       = 0;
      size_t max_size  = SK_D3D11_Textures.Textures_2D.size ();

      bool done = false;

      for ( auto& tex : SK_D3D11_Textures.Textures_2D )
      {
        if (++idx < cur_tex)
          continue;

        if (idx > start_idx + MAX_TEXTURES_PER_PASS)
        {
          cur_tex = idx;
          break;
        }

        if (tex.second.crc32c != 0x0 && tex.first != nullptr)
        {
          *(record++) =  tex.first;
          *(desc++)   = &tex.second;
                      ++record_count;
        }
      }

      if (idx >= max_size)
        done = true;

      pDXGIDev->QueryResourceResidency (
        residency_tests.data   (),
        residency_results.data (),
                                   (UINT)record_count
      );

      idx = 0;

      desc = residency_descs.begin ();

      for ( auto it : residency_results )
      {
        // Handle uninit. and corrupted texture entries
        if (it < DXGI_RESIDENCY_FULLY_RESIDENT || it > DXGI_RESIDENCY_EVICTED_TO_DISK)
          continue;

        if (it != DXGI_RESIDENCY_FULLY_RESIDENT)
        {
          SK_LOG1 ( (L"Texture %x is non-resident, last use: %lu <Residence: %lu>",  (*desc)->crc32c, (*desc)->last_used, it),
                     L"DXGI Cache" );

          D3D11_TEXTURE2D_DESC* tex_desc = &(*desc)->desc;

          if ((*desc)->texture != nullptr)
          {
            UINT refs_plus_1 = (*desc)->texture->AddRef  ();
            UINT refs        = (*desc)->texture->Release ();

            SK_LOG1 ( ( L"(%lux%lu@%lu [%s] - %s, %s, %s : CPU Usage=%x -- refs+1=%lu, refs=%lu",
                        tex_desc->Width, tex_desc->Height, tex_desc->MipLevels,
                        SK_DXGI_FormatToStr (tex_desc->Format).c_str (),
                        SK_D3D11_DescribeBindFlags ((D3D11_BIND_FLAG)tex_desc->BindFlags).c_str (), 
                        SK_D3D11_DescribeMiscFlags ((D3D11_RESOURCE_MISC_FLAG)tex_desc->MiscFlags).c_str (),
                        SK_D3D11_DescribeUsage     (tex_desc->Usage),
                        (UINT)tex_desc->CPUAccessFlags,
                        refs_plus_1, refs ),
                       L"DXGI Cache" );

            // If this texture is _NOT_ injected and also not resident in VRAM, then
            //   remove it from cache.
            //
            //  If it is injected, leave it loaded because this cache's purpose is to prevent
            //    re-loading injected textures.
            if (refs == 1 && (! (*desc)->injected))
              (*desc)->texture->Release ();
          }
        }

        if (it == DXGI_RESIDENCY_FULLY_RESIDENT)            { ++fully_resident; size_vram   += (*(desc))->mem_size; }
        if (it == DXGI_RESIDENCY_RESIDENT_IN_SHARED_MEMORY) { ++shared_memory;  size_shared += (*(desc))->mem_size; }
        if (it == DXGI_RESIDENCY_EVICTED_TO_DISK)           { ++on_disk;        size_disk   += (*(desc))->mem_size; }

        ++desc;

        if (++idx >= record_count)
          break;
      }

      dwLastTest = dwNow;

      if (done)
      {
        InterlockedExchange   (&SK_D3D11_TexCacheResidency.count.InVRAM,   fully_resident);
        InterlockedExchange   (&SK_D3D11_TexCacheResidency.count.Shared,   shared_memory);
        InterlockedExchange   (&SK_D3D11_TexCacheResidency.count.PagedOut, on_disk);

        InterlockedExchange64 (&SK_D3D11_TexCacheResidency.size.InVRAM,   size_vram);
        InterlockedExchange64 (&SK_D3D11_TexCacheResidency.size.Shared,   size_shared);
        InterlockedExchange64 (&SK_D3D11_TexCacheResidency.size.PagedOut, size_disk);

        SK_D3D11_Textures.AggregateSize_2D = size_vram      + size_shared + size_disk;
        SK_D3D11_Textures.Entries_2D       = fully_resident + shared_memory + on_disk;

        fully_resident = 0;
        shared_memory  = 0;
        on_disk        = 0;

        size_vram   = 0ULL;
        size_shared = 0ULL;
        size_disk   = 0ULL;

        dwLastEvict = dwInitiateEvict;
        dwLastSize  = dwInitiateSize;

        cur_tex = 0;
      }
    }
  }

  if (pDXGIDev) pDXGIDev->Release ();
  if (pDevice)  pDevice->Release  ();



  static int       iter               = 0;

  static bool      init               = false;
  static ULONGLONG ullMemoryTotal_KiB = 0;
  static HANDLE    hProc              = nullptr;

  if (! init)
  {
    init  = true;
    hProc = SK_GetCurrentProcess ();

    GetPhysicallyInstalledSystemMemory (&ullMemoryTotal_KiB);
  }

  ++iter;

  bool reset =
    SK_D3D11_need_tex_reset || SK_D3D11_try_tex_reset;

  static PROCESS_MEMORY_COUNTERS pmc = {   };

  bool has_non_zero_reserve = config.mem.reserve > 0.0f;

  if (! (iter % 5))
  {
                                  pmc.cb = sizeof pmc;
    GetProcessMemoryInfo (hProc, &pmc,     sizeof pmc);

    reset |=
      ( (SK_D3D11_Textures.AggregateSize_2D >> 20ULL) > (uint64_t)cache_opts.max_size    ||
         SK_D3D11_Textures.Entries_2D                 >           cache_opts.max_entries ||
        ( has_non_zero_reserve && config.mem.reserve / 100.0f) * ullMemoryTotal_KiB 
                                                        <
                              (pmc.PagefileUsage >> 10UL)
      );
  }

  if (reset)
  {
    //dll_log.Log (L"[DX11TexMgr] DXGI 1.4 Budget Change: Triggered a texture manager purge...");

    SK_D3D11_amount_to_purge =
      has_non_zero_reserve   ? 
      static_cast <int32_t> (
        (pmc.PagefileUsage >> 10UL) - (float)(ullMemoryTotal_KiB) *
                       (config.mem.reserve / 100.0f)
      )                      : 0;

    SK_D3D11_Textures.reset ();
  }

  else
  {
    //SK_RenderBackend& rb =
    //  SK_GetCurrentRenderBackend ();

    //if (rb.d3d11.immediate_ctx != nullptr)
    //{
    //  SK_D3D11_PreLoadTextures ();
    //}
  }
}

void
SK_D3D11_TexMgr::reset (void)
{
  ////if (IUnknown_AddRef_Original == nullptr || IUnknown_Release_Original == nullptr)
  ////  return;

  if (SK_GetFramesDrawn () < 1)
    return;


  uint32_t count  = 0;
  int64_t  purged = 0;


  bool must_free =
    SK_D3D11_need_tex_reset;

  SK_D3D11_need_tex_reset = false;


  LONGLONG time_now = SK_QueryPerf ().QuadPart;

  // Additional conditions that may trigger a purge
  if (! must_free)
  {
    // Throttle to at most one potentially unnecessary purge attempt per-ten seconds
    if ( LastModified_2D <=  LastPurge_2D &&
         time_now        < ( LastPurge_2D + ( PerfFreq.QuadPart * 10LL ) ) )
    {
      SK_D3D11_try_tex_reset = true;
      return;
    }


    const float overfill_factor = 1.05f;

    bool no_work = true;


    if ( (uint32_t)AggregateSize_2D >> 20ULL > ( (uint32_t)       cache_opts.max_size +
                                                ((uint32_t)(float)cache_opts.max_size * overfill_factor)) )
      no_work = false;

    if ( SK_D3D11_Textures.Entries_2D > (LONG)       cache_opts.max_entries +
                                        (LONG)(float)cache_opts.max_entries * overfill_factor )
      no_work = false;

    if ( SK_D3D11_amount_to_purge > 0 )
      no_work = false;

    if (no_work) return;
  }


  must_free = false;


  std::set    <ID3D11Texture2D *>                     cleared;// (cache_opts.max_evict);
  std::vector <SK_D3D11_TexMgr::tex2D_descriptor_s *> textures;

  int potential = 0;
  {
    SK_AutoCriticalSection critical (&cache_cs);
    {
      textures.reserve  (Textures_2D.size ());

      for ( auto& desc : Textures_2D )
      {
        if (desc.second.texture == nullptr || desc.second.crc32c == 0x00)
          continue;

        bool can_free = must_free;

        if (! must_free)
          can_free = (desc.second.texture->AddRef () <= 2);

        if (can_free)
        {
          ++potential;
            textures.emplace_back (&desc.second);
        }

        if (! must_free)
          desc.second.texture->Release ();
      }
    }

    if (potential > 0)
    {
      std::sort ( textures.begin (),
                    textures.end (),
         [&]( const SK_D3D11_TexMgr::tex2D_descriptor_s* const a,
              const SK_D3D11_TexMgr::tex2D_descriptor_s* const b )
        {
          return (b->load_time * 10 + b->last_used / 10 + b->hits * 100) <
                 (a->load_time * 10 + a->last_used / 10 + a->hits * 100);
        }
      );
    }
  }

  if (potential == 0)
    return;

  dll_log.Log (L"Reset potential = %lu / %lu", potential, textures.size ());

  const uint32_t max_count =
   cache_opts.max_evict;
 
  SK_AutoCriticalSection critical (&cache_cs);

  for ( const auto& desc : textures )
  {
    auto mem_size =
     static_cast <int64_t> (desc->mem_size) >> 10ULL;
 
   if (desc->texture != nullptr && cleared.count (desc->texture) == 0)
   {
     int refs =
       desc->texture->AddRef  () - 1;
       desc->texture->Release ();
 
     if (refs <= 1 || must_free)
     {
       // Avoid double-freeing if the hash map somehow contains the same texture multiple times
       cleared.emplace (desc->texture);

       if (must_free && refs != 1 && refs != 2)
       {
         SK_LOG0 ( ( L"Unexpected reference count for texture with crc32=%08x; refs=%lu, expected=1 -- removing from cache and praying...",
                       desc->crc32c, refs ),
                    L"DX11TexMgr" );
       }

       SK_D3D11_RemoveTexFromCache (desc->texture);
                                    desc->texture->Release ();
         //IUnknown_Release_Original (desc->texture);
 
       count++;
       purged += mem_size;
 
       if ( ! must_free )
       {
         if ((// Have we over-freed? If so, stop when the minimum number of evicted textures is reached
              (AggregateSize_2D >> 20ULL) <= (uint32_t)cache_opts.min_size &&
                                    count >=           cache_opts.min_evict )
         
              // An arbitrary purge request was issued
                                                                                               ||
             (SK_D3D11_amount_to_purge     >            0                   &&
              SK_D3D11_amount_to_purge     <=           purged              &&
                                     count >=           cache_opts.min_evict ) ||
         
              // Have we evicted as many textures as we can in one pass?
                                     count >=           max_count )
         {
           SK_D3D11_amount_to_purge =
             std::max (0, SK_D3D11_amount_to_purge);
         
           break;
         }
       }
     }
   }
 }
 
 LastPurge_2D.store (time_now);

 if (count > 0)
 {
   SK_D3D11_try_tex_reset = false;

   SK_LOG0 ( ( L"Texture Cache Purge:  Eliminated %.2f MiB of texture data across %lu textures (max. evict per-pass: %lu).",
                 static_cast <float> (purged) / 1024.0f,
                                      count,
                                      cache_opts.max_evict ),
               L"DX11TexMgr" );
 
   if ((AggregateSize_2D >> 20ULL) >= static_cast <uint64_t> (cache_opts.max_size))
   {
     SK_LOG0 ( ( L" >> Texture cache remains %.2f MiB over-budget; will schedule future passes until resolved.",
                   static_cast <float> ( AggregateSize_2D ) / (1024.0f * 1024.0f) -
                   static_cast <float> (cache_opts.max_size) ),
                 L"DX11TexMgr" );

     SK_D3D11_try_tex_reset = true;
   }
 
   if (Entries_2D >= cache_opts.max_entries)
   {
     SK_LOG0 ( ( L" >> Texture cache remains %lu entries over-filled; will schedule future passes until resolved.",
                   Entries_2D - cache_opts.max_entries ),
                 L"DX11TexMgr" );

     SK_D3D11_try_tex_reset = true;
   }
 
   InterlockedExchange (&live_textures_dirty, TRUE);
 }
}

ID3D11Texture2D*
SK_D3D11_TexMgr::getTexture2D ( uint32_t              tag,
                          const D3D11_TEXTURE2D_DESC* pDesc,
                                size_t*               pMemSize,
                                float*                pTimeSaved,
                                SK_TLS*               pTLS )
{
  ID3D11Texture2D* pTex2D =
    nullptr;

  if (isTexture2D (tag, pDesc))
  {
    ID3D11Texture2D*    it =     HashMap_2D [pDesc->MipLevels][tag];
    tex2D_descriptor_s& desc2d (Textures_2D [it]);

    // We use a lockless concurrent hashmap, which makes removal
    //   of key/values an unsafe operation.
    //
    //   Thus, crc32c == 0x0 signals a key that exists in the
    //     map, but has no live association with any real data.
    //
    //     Zombie, in other words.
    //
    bool zombie = desc2d.crc32c == 0x00;

    if ( desc2d.crc32c != 0x00 &&
         desc2d.tag    == tag  &&
      (! desc2d.discard) )
    {
      pTex2D = desc2d.texture;
      pTex2D->AddRef ();

      const size_t  size = desc2d.mem_size;
      const float  fTime = static_cast <float> (desc2d.load_time ) * 1000.0f /
                           static_cast <float> (PerfFreq.QuadPart);

      if (pMemSize)   *pMemSize   = size;
      if (pTimeSaved) *pTimeSaved = fTime;

      desc2d.last_used =
        SK_QueryPerf ().QuadPart;

      if (pTLS == nullptr)
          pTLS = SK_TLS_Bottom ();

      // Don't record cache hits caused by the shader mod interface
      if (! pTLS->imgui.drawing)
      {
        InterlockedIncrement (&desc2d.hits);

        RedundantData_2D += static_cast <LONG64> (size);
        RedundantLoads_2D++;
        RedundantTime_2D += fTime;
      }
    }

    else
    {
      if (! zombie)
      {
        SK_LOG0 ( ( L"Cached texture (tag=%x) found in hash table, but not in texture map", tag),
                    L"DX11TexMgr" );
      }

      HashMap_2D  [pDesc->MipLevels].erase (tag);
      Textures_2D [it].crc32c = 0x00;
    }
  }

  return pTex2D;
}

bool
__stdcall
SK_D3D11_TextureIsCached (ID3D11Texture2D* pTex)
{
  if (! SK_D3D11_cache_textures)
    return false;

  const auto& it =
    SK_D3D11_Textures.Textures_2D.find (pTex);

  return ( it                != SK_D3D11_Textures.Textures_2D.cend () &&
           it->second.crc32c != 0x00 );
}

uint32_t
__stdcall
SK_D3D11_TextureHashFromCache (ID3D11Texture2D* pTex)
{
  if (! SK_D3D11_cache_textures)
    return 0x00;

  const auto& it =
    SK_D3D11_Textures.Textures_2D.find (pTex);

  if ( it != SK_D3D11_Textures.Textures_2D.cend () )
    return it->second.crc32c;

  return 0x00;
}

void
__stdcall
SK_D3D11_UseTexture (ID3D11Texture2D* pTex)
{
  if (! SK_D3D11_cache_textures)
    return;

  if (SK_D3D11_TextureIsCached (pTex))
  {
    SK_D3D11_Textures.Textures_2D [pTex].last_used =
      SK_QueryPerf ().QuadPart;
  }
}

bool
__stdcall
SK_D3D11_RemoveTexFromCache (ID3D11Texture2D* pTex, bool blacklist)
{
  SK_AutoCriticalSection critical (&cache_cs);

  if (! SK_D3D11_TextureIsCached (pTex))
    return false;

  if (pTex != nullptr)
  {
    SK_D3D11_TexMgr::tex2D_descriptor_s& it =
      SK_D3D11_Textures.Textures_2D [pTex];

          uint32_t              tag  = it.tag;
    const D3D11_TEXTURE2D_DESC& desc = it.orig_desc;

    SK_D3D11_Textures.AggregateSize_2D -=
      it.mem_size;
      it.crc32c = 0x00;

  //SK_D3D11_Textures.Textures_2D.erase (pTex);
    SK_D3D11_Textures.Evicted_2D++;
  //SK_D3D11_Textures.TexRefs_2D.erase (pTex);

    SK_D3D11_Textures.Entries_2D--;

    if (blacklist)
    {
      SK_D3D11_Textures.Blacklist_2D [desc.MipLevels].emplace (tag);
      pTex->Release ();
    }
    else
      SK_D3D11_Textures.HashMap_2D   [desc.MipLevels].erase   (tag);

    live_textures_dirty = true;
  }
   
  // Lightweight signal that something changed and a purge may be needed
  SK_D3D11_Textures.LastModified_2D = SK_QueryPerf ().QuadPart;

  return true;
}

void
SK_D3D11_TexMgr::refTexture2D ( ID3D11Texture2D*      pTex,
                          const D3D11_TEXTURE2D_DESC *pDesc,
                                uint32_t              tag,
                                size_t                mem_size,
                                uint64_t              load_time,
                                uint32_t              crc32c,
                                std::wstring          fileName,
                          const D3D11_TEXTURE2D_DESC *pOrigDesc,
                                     _In_opt_ HMODULE/*hModCaller*/,
                                SK_TLS               *pTLS )
{
  if (! SK_D3D11_cache_textures)
    return;

  ///// static volatile LONG init = FALSE;
  ///// 
  ///// if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  ///// {
  /////   DXGI_VIRTUAL_HOOK ( &pTex, 2, "IUnknown::Release",
  /////                       IUnknown_Release,
  /////                       IUnknown_Release_Original,
  /////                       IUnknown_Release_pfn );
  /////   DXGI_VIRTUAL_HOOK ( &pTex, 1, "IUnknown::AddRef",
  /////                       IUnknown_AddRef,
  /////                       IUnknown_AddRef_Original,
  /////                       IUnknown_AddRef_pfn );
  ///// 
  /////   SK_ApplyQueuedHooks ();
  ///// 
  /////   InterlockedIncrement (&init);
  ///// }
  ///// 
  ///// SK_Thread_SpinUntilAtomicMin (&init, 2);


  if (pTex == nullptr || tag == 0x00)
	  return;



  ///////if (SK_D3D11_TestRefCountHooks (pTex, pTLS))
  ///////{
  ///////  SK_LOG2 ( (L"Cached texture (%x)",
  ///////                crc32c ),
  ///////             L"DX11TexMgr" );
  ///////}
  ///////
  ///////else
  ///////{
  ///////  SK_LOG0 ( (L"Potentially cacheable texture (%x) is not correctly reference counted; opting out!",
  ///////                crc32c ),
  ///////             L"DX11TexMgr" );
  ///////  return;
  ///////}


  ///if (pDesc->Usage >= D3D11_USAGE_DYNAMIC)
  ///{
  ///  dll_log.Log ( L"[DX11TexMgr] Texture '%08X' Is Not Cacheable "
  ///                L"Due To Usage: %lu",
  ///                crc32c, pDesc->Usage );
  ///  return;
  ///}

  ///if (pDesc->CPUAccessFlags & D3D11_CPU_ACCESS_WRITE)
  ///{
  ///  dll_log.Log ( L"[DX11TexMgr] Texture '%08X' Is Not Cacheable "
  ///                L"Due To CPUAccessFlags: 0x%X",
  ///                crc32c, pDesc->CPUAccessFlags );
  ///  return;
  ///}

  tex2D_descriptor_s  null_desc = { };
  tex2D_descriptor_s&   texDesc = null_desc;

  if (SK_D3D11_TextureIsCached (pTex) && (! (texDesc = Textures_2D [pTex]).discard))
  {
    // If we are updating once per-frame, then remove the freaking texture :)
    if (HashMap_2D [texDesc.orig_desc.MipLevels].contains (texDesc.tag))
    {
      if (texDesc.last_frame > SK_GetFramesDrawn () - 3)
      {
                                                                           texDesc.discard = true;
        pTex->SetPrivateData (SKID_D3D11Texture2D_DISCARD, sizeof (bool), &texDesc.discard);
      }

      //// This texture is too dynamic, it's best to just cut all ties to it,
      ////   don't try to keep updating the hash because that may kill performance.
      ////////HashMap_2D [texDesc.orig_desc.MipLevels].erase (texDesc.tag);
    }

    texDesc.crc32c     = crc32c;
    texDesc.tag        = tag;
    texDesc.last_used  = SK_QueryPerf      ().QuadPart;
    texDesc.last_frame = SK_GetFramesDrawn ();

    ///InterlockedIncrement (&texDesc.hits);

    dll_log.Log (L"[DX11TexMgr] Texture is already cached?!  { Original: %x, New: %x }",
                   Textures_2D [pTex].crc32c, crc32c );
    return;
  }

  if (texDesc.discard)
  {
    dll_log.Log (L"[DX11TexMgr] Texture was cached, but marked as discard... ignoring" );
    return;
  }


  AggregateSize_2D += mem_size;

  tex2D_descriptor_s desc2d 
                    = { };

  desc2d.desc       = *pDesc;
  desc2d.orig_desc  =  pOrigDesc != nullptr ?
                      *pOrigDesc : *pDesc;
  desc2d.texture    =  pTex;
  desc2d.load_time  =  load_time;
  desc2d.mem_size   =  mem_size;
  desc2d.tag        =  tag;
  desc2d.crc32c     =  crc32c;
  desc2d.last_used  =  SK_QueryPerf      ().QuadPart;
  desc2d.last_frame =  SK_GetFramesDrawn ();
  desc2d.file_name  =  fileName;


  if (desc2d.orig_desc.MipLevels >= 18)
  {
    SK_LOG0 ( ( L"Too many mipmap LODs to cache (%lu), will not cache '%x'",
                  desc2d.orig_desc.MipLevels,
                    desc2d.crc32c ),
                L"DX11TexMgr" );
  }

  SK_LOG4 ( ( L"Referencing Texture '%x' with %lu mipmap levels :: (%08" PRIxPTR L"h)",
                desc2d.crc32c,
                  desc2d.orig_desc.MipLevels,
                    (uintptr_t)pTex ),
              L"DX11TexMgr" );

  SK_LOG4 ( ( L"  >> (%ux%u:%u) [CPU Access: %x], Misc Flags: %x, Usage: %u, Bind Flags: %x",
                desc2d.orig_desc.Width,     desc2d.orig_desc.Height,
                desc2d.orig_desc.MipLevels, desc2d.orig_desc.CPUAccessFlags,
                desc2d.orig_desc.MiscFlags, desc2d.orig_desc.Usage,
                desc2d.orig_desc.BindFlags ),
              L"DX11TexMgr" );

  HashMap_2D [desc2d.orig_desc.MipLevels][tag] = pTex;
  Textures_2D.insert            (std::make_pair (pTex, desc2d));
  TexRefs_2D.insert             (                pTex);

  Entries_2D++;

  // Hold a reference ourselves so that the game cannot free it
  pTex->AddRef ();


  // Lightweight signal that something changed and a purge may be needed
  LastModified_2D = SK_QueryPerf ().QuadPart;
}

#include <Shlwapi.h>

std::unordered_map < std::wstring, uint32_t > SK_D3D11_EnumeratedMipmapCache;
                                     uint64_t SK_D3D11_MipmapCacheSize;
void
SK_D3D11_RecursiveEnumAndAddTex ( std::wstring   directory, unsigned int& files,
                                  LARGE_INTEGER& liSize,    wchar_t*      wszPattern = L"*" );

void
WINAPI
SK_D3D11_PopulateResourceList (bool refresh)
{
  static bool init = false;

  if (((! refresh) && init) || SK_D3D11_res_root.empty ())
    return;

  SK_AutoCriticalSection critical0 (&tex_cs);
  SK_AutoCriticalSection critical1 (&inject_cs);

  if (refresh)
  {
    SK_D3D11_Textures.dumped_textures.clear ();
    SK_D3D11_Textures.dumped_texture_bytes = 0;

    SK_D3D11_Textures.injectable_textures.clear ();
    SK_D3D11_Textures.injectable_texture_bytes = 0;

    SK_D3D11_EnumeratedMipmapCache.clear ();
               SK_D3D11_MipmapCacheSize = 0;
  }

  init = true;

  wchar_t wszTexDumpDir_RAW [ MAX_PATH     ] = { };
  wchar_t wszTexDumpDir     [ MAX_PATH + 2 ] = { };

  lstrcatW (wszTexDumpDir_RAW, SK_D3D11_res_root.c_str ());
  lstrcatW (wszTexDumpDir_RAW, LR"(\dump\textures\)");
  lstrcatW (wszTexDumpDir_RAW, SK_GetHostApp ());

  wcscpy ( wszTexDumpDir,
             SK_EvalEnvironmentVars (wszTexDumpDir_RAW).c_str () );

  //
  // Walk custom textures so we don't have to query the filesystem on every
  //   texture load to check if a custom one exists.
  //
  if ( GetFileAttributesW (wszTexDumpDir) !=
         INVALID_FILE_ATTRIBUTES )
  {
    WIN32_FIND_DATA fd     = {  };
    HANDLE          hFind  = INVALID_HANDLE_VALUE;
    unsigned int    files  =  0UL;
    LARGE_INTEGER   liSize = {  };

    LARGE_INTEGER   liCompressed   = {   };
    LARGE_INTEGER   liUncompressed = {   };

    dll_log.LogEx ( true, L"[DX11TexMgr] Enumerating dumped...    " );

    lstrcatW (wszTexDumpDir, LR"(\*)");

    hFind = FindFirstFileW (wszTexDumpDir, &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
      do
      {
        if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
        {
          // Dumped Metadata has the extension .dds.txt, do not
          //   include these while scanning for textures.
          if (    StrStrIW (fd.cFileName, L".dds")    &&
               (! StrStrIW (fd.cFileName, L".dds.txt") ) )
          {
            uint32_t top_crc32 = 0x00;
            uint32_t checksum  = 0x00;

            bool compressed = false;

            if (StrStrIW (fd.cFileName, L"Uncompressed_"))
            {
              if (StrStrIW (StrStrIW (fd.cFileName, L"_") + 1, L"_"))
              {
                swscanf ( fd.cFileName,
                            L"Uncompressed_%08X_%08X.dds",
                              &top_crc32,
                                &checksum );
              }

              else
              {
                swscanf ( fd.cFileName,
                            L"Uncompressed_%08X.dds",
                              &top_crc32 );
                checksum = 0x00;
              }
            }

            else
            {
              if (StrStrIW (StrStrIW (fd.cFileName, L"_") + 1, L"_"))
              {
                swscanf ( fd.cFileName,
                            L"Compressed_%08X_%08X.dds",
                              &top_crc32,
                                &checksum );
              }

              else
              {
                swscanf ( fd.cFileName,
                            L"Compressed_%08X.dds",
                              &top_crc32 );
                checksum = 0x00;
              }

              compressed = true;
            }

            ++files;

            LARGE_INTEGER fsize;

            fsize.HighPart = fd.nFileSizeHigh;
            fsize.LowPart  = fd.nFileSizeLow;

            liSize.QuadPart += fsize.QuadPart;

            if (compressed)
              liCompressed.QuadPart += fsize.QuadPart;
            else
              liUncompressed.QuadPart += fsize.QuadPart;

            if (SK_D3D11_Textures.dumped_textures.count (top_crc32) >= 1)
              SK_LOG0 ( ( L" >> WARNING: Multiple textures have "
                            L"the same top-level LOD hash (%08X) <<",
                              top_crc32 ), L"DX11TexDmp" );

            if (checksum == 0x00)
              SK_D3D11_Textures.dumped_textures.insert (top_crc32);
            else
              SK_D3D11_Textures.dumped_collisions.insert (crc32c (top_crc32, (uint8_t *)&checksum, 4));
          }
        }
      } while (FindNextFileW (hFind, &fd) != 0);

      FindClose (hFind);
    }

    SK_D3D11_Textures.dumped_texture_bytes = liSize.QuadPart;

    dll_log.LogEx ( false, L" %lu files (%3.1f MiB -- %3.1f:%3.1f MiB Un:Compressed)\n",
                      files, (double)liSize.QuadPart / (1024.0 * 1024.0),
                               (double)liUncompressed.QuadPart / (1024.0 * 1024.0),
                                 (double)liCompressed.QuadPart /  (1024.0 * 1024.0) );
  }

  wchar_t wszTexInjectDir_RAW [ MAX_PATH + 2 ] = { };
  wchar_t wszTexInjectDir     [ MAX_PATH + 2 ] = { };

  lstrcatW (wszTexInjectDir_RAW, SK_D3D11_res_root.c_str ());
  lstrcatW (wszTexInjectDir_RAW, LR"(\inject\textures)");

  wcscpy ( wszTexInjectDir,
             SK_EvalEnvironmentVars (wszTexInjectDir_RAW).c_str () );

  if ( GetFileAttributesW (wszTexInjectDir) !=
         INVALID_FILE_ATTRIBUTES )
  {
    dll_log.LogEx ( true, L"[DX11TexMgr] Enumerating injectable..." );

    unsigned int    files  =   0;
    LARGE_INTEGER   liSize = {   };

    SK_D3D11_RecursiveEnumAndAddTex (wszTexInjectDir, files, liSize);

    SK_D3D11_Textures.injectable_texture_bytes = liSize.QuadPart;

    dll_log.LogEx ( false, L" %lu files (%3.1f MiB)\n",
                      files, (double)liSize.QuadPart / (1024.0 * 1024.0) );
  }

  wchar_t wszTexInjectDir_FFX_RAW [ MAX_PATH     ] = { };
  wchar_t wszTexInjectDir_FFX     [ MAX_PATH + 2 ] = { };

  lstrcatW (wszTexInjectDir_FFX_RAW, SK_D3D11_res_root.c_str ());
  lstrcatW (wszTexInjectDir_FFX_RAW, LR"(\inject\textures\UnX_Old)");

  wcscpy ( wszTexInjectDir_FFX,
             SK_EvalEnvironmentVars (wszTexInjectDir_FFX_RAW).c_str () );

  if ( GetFileAttributesW (wszTexInjectDir_FFX) !=
         INVALID_FILE_ATTRIBUTES )
  {
    WIN32_FIND_DATA fd     = {   };
    HANDLE          hFind  = INVALID_HANDLE_VALUE;
    int             files  =   0;
    LARGE_INTEGER   liSize = {   };

    dll_log.LogEx ( true, L"[DX11TexMgr] Enumerating FFX inject..." );

    lstrcatW (wszTexInjectDir_FFX, LR"(\*)");

    hFind = FindFirstFileW (wszTexInjectDir_FFX, &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
      do
      {
        if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
        {
          if (StrStrIW (fd.cFileName, L".dds"))
          {
            uint32_t ffx_crc32;

            swscanf (fd.cFileName, L"%08X.dds", &ffx_crc32);

            ++files;

            LARGE_INTEGER fsize;

            fsize.HighPart = fd.nFileSizeHigh;
            fsize.LowPart  = fd.nFileSizeLow;

            liSize.QuadPart += fsize.QuadPart;

            SK_D3D11_Textures.injectable_ffx.insert           (ffx_crc32);
          }
        }
      } while (FindNextFileW (hFind, &fd) != 0);

      FindClose (hFind);
    }

    dll_log.LogEx ( false, L" %li files (%3.1f MiB)\n",
                      files, (double)liSize.QuadPart / (1024.0 * 1024.0) );
  }

  for (auto& it : SK_D3D11_EnumeratedMipmapCache)
  {
    SK_D3D11_AddTexHash (it.first.c_str (), it.second, 0);
  }
}

void
WINAPI
SK_D3D11_SetResourceRoot (const wchar_t* root)
{
  // Non-absolute path (e.g. NOT C:\...\...")
  if (! wcsstr (root, L":"))
  {
         wchar_t wszPath [MAX_PATH * 2] = { };
    wcsncpy     (wszPath, SK_IsInjected () ? SK_GetConfigPath () :
                                             SK_GetRootPath   (),  MAX_PATH);
    PathAppendW (wszPath, root);

    SK_D3D11_res_root = wszPath;
  }

  else
    SK_D3D11_res_root = root;
}

void
WINAPI
SK_D3D11_EnableTexDump (bool enable)
{
  SK_D3D11_dump_textures = enable;
}

void
WINAPI
SK_D3D11_EnableTexInject (bool enable)
{
  SK_D3D11_inject_textures = enable;
}

void
WINAPI
SK_D3D11_EnableTexInject_FFX (bool enable)
{
  SK_D3D11_inject_textures     = enable;
  SK_D3D11_inject_textures_ffx = enable;
}

void
WINAPI
SK_D3D11_EnableTexCache (bool enable)
{
  SK_D3D11_cache_textures = enable;
}

void
WINAPI
SKX_D3D11_MarkTextures (bool x, bool y, bool z)
{
  UNREFERENCED_PARAMETER (x);
  UNREFERENCED_PARAMETER (y);
  UNREFERENCED_PARAMETER (z);
}

bool
WINAPI
SK_D3D11_IsTexHashed (uint32_t top_crc32, uint32_t hash)
{
  if (SK_D3D11_Textures.tex_hashes_ex.empty () && SK_D3D11_Textures.tex_hashes.empty ())
    return false;

  SK_AutoCriticalSection critical (&hash_cs);

  if (SK_D3D11_Textures.tex_hashes.count (crc32c (top_crc32, (const uint8_t *)&hash, 4)) != 0)
    return true;

  return SK_D3D11_Textures.tex_hashes.count (top_crc32) != 0;
}

void
WINAPI
SK_D3D11_AddInjectable (uint32_t top_crc32, uint32_t checksum);

void
WINAPI
SK_D3D11_AddTexHash ( const wchar_t* name, uint32_t top_crc32, uint32_t hash )
{
  // For early loading UnX
  SK_D3D11_InitTextures ();

  if (hash != 0x00)
  {
    if (! SK_D3D11_IsTexHashed (top_crc32, hash))
    {
      {
        SK_AutoCriticalSection critical (&hash_cs);
        SK_D3D11_Textures.tex_hashes_ex.emplace  (crc32c (top_crc32, (const uint8_t *)&hash, 4), name);
      }

      SK_D3D11_AddInjectable (top_crc32, hash);
    }
  }

  if (! SK_D3D11_IsTexHashed (top_crc32, 0x00))
  {
    {
      SK_AutoCriticalSection critical (&hash_cs);
      SK_D3D11_Textures.tex_hashes.emplace (top_crc32, name);
    }

    if (! SK_D3D11_inject_textures_ffx)
      SK_D3D11_AddInjectable (top_crc32, 0x00);
    else
    {
      SK_AutoCriticalSection critical (&inject_cs);
      SK_D3D11_Textures.injectable_ffx.emplace (top_crc32);
    }
  }
}

void
WINAPI
SK_D3D11_RemoveTexHash (uint32_t top_crc32, uint32_t hash)
{
  if (SK_D3D11_Textures.tex_hashes_ex.empty () && SK_D3D11_Textures.tex_hashes.empty ())
    return;

  SK_AutoCriticalSection critical (&hash_cs);

  if (hash != 0x00 && SK_D3D11_IsTexHashed (top_crc32, hash))
  {
    SK_D3D11_Textures.tex_hashes_ex.erase (crc32c (top_crc32, (const uint8_t *)&hash, 4));

    SK_D3D11_RemoveInjectable (top_crc32, hash);
  }

  else if (hash == 0x00 && SK_D3D11_IsTexHashed (top_crc32, 0x00)) {
    SK_D3D11_Textures.tex_hashes.erase (top_crc32);

    SK_D3D11_RemoveInjectable (top_crc32, 0x00);
  }
}

std::wstring
__stdcall
SK_D3D11_TexHashToName (uint32_t top_crc32, uint32_t hash)
{
  if (SK_D3D11_Textures.tex_hashes_ex.empty () && SK_D3D11_Textures.tex_hashes.empty ())
    return L"";

  SK_AutoCriticalSection critical (&hash_cs);

  std::wstring ret = L"";

  if (hash != 0x00 && SK_D3D11_IsTexHashed (top_crc32, hash))
  {
    ret = SK_D3D11_Textures.tex_hashes_ex [crc32c (top_crc32, (const uint8_t *)&hash, 4)];
  }

  else if (hash == 0x00 && SK_D3D11_IsTexHashed (top_crc32, 0x00))
  {
    ret = SK_D3D11_Textures.tex_hashes [top_crc32];
  }

  return ret;
}

void
SK_D3D11_RecursiveEnumAndAddTex ( std::wstring   directory, unsigned int& files,
                                  LARGE_INTEGER& liSize,    wchar_t*      wszPattern )
{
  WIN32_FIND_DATA fd            = {   };
  HANDLE          hFind         = INVALID_HANDLE_VALUE;
  wchar_t         wszPath        [MAX_PATH * 2] = { };
  wchar_t         wszFullPattern [MAX_PATH * 2] = { };

  PathCombineW (wszFullPattern, directory.c_str (), L"*");

  hFind =
    FindFirstFileW (wszFullPattern, &fd);

  if (hFind != INVALID_HANDLE_VALUE)
  {
    do
    {
      if (          (fd.dwFileAttributes       & FILE_ATTRIBUTE_DIRECTORY) &&
          (_wcsicmp (fd.cFileName, L"." )      != 0) &&
          (_wcsicmp (fd.cFileName, L"..")      != 0) &&
          (_wcsicmp (fd.cFileName, L"UnX_Old") != 0)   )
      {
        PathCombineW                    (wszPath, directory.c_str (), fd.cFileName);
        SK_D3D11_RecursiveEnumAndAddTex (wszPath, files, liSize, wszPattern);
      }
    } while (FindNextFile (hFind, &fd));

    FindClose (hFind);
  }

  PathCombineW (wszFullPattern, directory.c_str (), wszPattern);

  hFind =
    FindFirstFileW (wszFullPattern, &fd);

  //bool preload =
  //  StrStrIW (directory.c_str (), LR"(\Preload\)") != nullptr;

  if (hFind != INVALID_HANDLE_VALUE)
  {
    do
    {
      if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
      {
        if (StrStrIW (fd.cFileName, L".dds"))
        {
        //bool     preloaded = preload;
          uint32_t top_crc32 = 0x00;
          uint32_t checksum  = 0x00;

          wchar_t* wszFileName = fd.cFileName;

          //if (StrStrIW (wszFileName, L"Preload") == fd.cFileName)
          //{
          //  const size_t skip =
          //    wcslen (L"Preload");
          //
          //  for ( size_t i = 0; i < skip; i++ )
          //    wszFileName = CharNextW (wszFileName);
          //
          //  preloaded = true;
          //}

          if (StrStrIW (wszFileName, L"_"))
          {
            swscanf (wszFileName, L"%08X_%08X.dds", &top_crc32, &checksum);
          }

          else
          {
            swscanf (wszFileName, L"%08X.dds",    &top_crc32);
          }

          ++files;

          LARGE_INTEGER fsize;

          fsize.HighPart = fd.nFileSizeHigh;
          fsize.LowPart  = fd.nFileSizeLow;

          liSize.QuadPart += fsize.QuadPart;

          PathCombineW        (wszPath, directory.c_str (), wszFileName);

          if (! StrStrIW (wszPath, L"MipmapCache"))
          {
            SK_D3D11_AddTexHash (wszPath, top_crc32, 0);

            if (checksum != 0x00)
              SK_D3D11_AddTexHash (wszPath, top_crc32, checksum);

            //if (preloaded)
            //  SK_D3D11_AddTexPreLoad (top_crc32);

          }

          else
          {
            SK_D3D11_MipmapCacheSize += fsize.QuadPart;
            SK_D3D11_EnumeratedMipmapCache.emplace (wszPath, top_crc32);
          }
        }
      }
    } while (FindNextFileW (hFind, &fd) != 0);

    FindClose (hFind);
  }
}

INT
__stdcall
SK_D3D11_BytesPerPixel (DXGI_FORMAT fmt)
{
  switch (fmt)
  {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:      return 16;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:         return 16;
    case DXGI_FORMAT_R32G32B32A32_UINT:          return 16;
    case DXGI_FORMAT_R32G32B32A32_SINT:          return 16;

    case DXGI_FORMAT_R32G32B32_TYPELESS:         return 12;
    case DXGI_FORMAT_R32G32B32_FLOAT:            return 12;
    case DXGI_FORMAT_R32G32B32_UINT:             return 12;
    case DXGI_FORMAT_R32G32B32_SINT:             return 12;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:      return 8;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:         return 8;
    case DXGI_FORMAT_R16G16B16A16_UNORM:         return 8;
    case DXGI_FORMAT_R16G16B16A16_UINT:          return 8;
    case DXGI_FORMAT_R16G16B16A16_SNORM:         return 8;
    case DXGI_FORMAT_R16G16B16A16_SINT:          return 8;

    case DXGI_FORMAT_R32G32_TYPELESS:            return 8;
    case DXGI_FORMAT_R32G32_FLOAT:               return 8;
    case DXGI_FORMAT_R32G32_UINT:                return 8;
    case DXGI_FORMAT_R32G32_SINT:                return 8;
    case DXGI_FORMAT_R32G8X24_TYPELESS:          return 8;

    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:       return 8;
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:   return 8;
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:    return 8;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:       return 4;
    case DXGI_FORMAT_R10G10B10A2_UNORM:          return 4;
    case DXGI_FORMAT_R10G10B10A2_UINT:           return 4;
    case DXGI_FORMAT_R11G11B10_FLOAT:            return 4;

    case DXGI_FORMAT_R8G8B8A8_TYPELESS:          return 4;
    case DXGI_FORMAT_R8G8B8A8_UNORM:             return 4;
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:        return 4;
    case DXGI_FORMAT_R8G8B8A8_UINT:              return 4;
    case DXGI_FORMAT_R8G8B8A8_SNORM:             return 4;
    case DXGI_FORMAT_R8G8B8A8_SINT:              return 4;

    case DXGI_FORMAT_R16G16_TYPELESS:            return 4;
    case DXGI_FORMAT_R16G16_FLOAT:               return 4;
    case DXGI_FORMAT_R16G16_UNORM:               return 4;
    case DXGI_FORMAT_R16G16_UINT:                return 4;
    case DXGI_FORMAT_R16G16_SNORM:               return 4;
    case DXGI_FORMAT_R16G16_SINT:                return 4;

    case DXGI_FORMAT_R32_TYPELESS:               return 4;
    case DXGI_FORMAT_D32_FLOAT:                  return 4;
    case DXGI_FORMAT_R32_FLOAT:                  return 4;
    case DXGI_FORMAT_R32_UINT:                   return 4;
    case DXGI_FORMAT_R32_SINT:                   return 4;
    case DXGI_FORMAT_R24G8_TYPELESS:             return 4;

    case DXGI_FORMAT_D24_UNORM_S8_UINT:          return 4;
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:      return 4;
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:       return 4;

    case DXGI_FORMAT_R8G8_TYPELESS:              return 2;
    case DXGI_FORMAT_R8G8_UNORM:                 return 2;
    case DXGI_FORMAT_R8G8_UINT:                  return 2;
    case DXGI_FORMAT_R8G8_SNORM:                 return 2;
    case DXGI_FORMAT_R8G8_SINT:                  return 2;

    case DXGI_FORMAT_R16_TYPELESS:               return 2;
    case DXGI_FORMAT_R16_FLOAT:                  return 2;
    case DXGI_FORMAT_D16_UNORM:                  return 2;
    case DXGI_FORMAT_R16_UNORM:                  return 2;
    case DXGI_FORMAT_R16_UINT:                   return 2;
    case DXGI_FORMAT_R16_SNORM:                  return 2;
    case DXGI_FORMAT_R16_SINT:                   return 2;

    case DXGI_FORMAT_R8_TYPELESS:                return 1;
    case DXGI_FORMAT_R8_UNORM:                   return 1;
    case DXGI_FORMAT_R8_UINT:                    return 1;
    case DXGI_FORMAT_R8_SNORM:                   return 1;
    case DXGI_FORMAT_R8_SINT:                    return 1;
    case DXGI_FORMAT_A8_UNORM:                   return 1;
    case DXGI_FORMAT_R1_UNORM:                   return 1;

    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:         return 4;
    case DXGI_FORMAT_R8G8_B8G8_UNORM:            return 4;
    case DXGI_FORMAT_G8R8_G8B8_UNORM:            return 4;

    case DXGI_FORMAT_BC1_TYPELESS:               return -1;
    case DXGI_FORMAT_BC1_UNORM:                  return -1;
    case DXGI_FORMAT_BC1_UNORM_SRGB:             return -1;
    case DXGI_FORMAT_BC2_TYPELESS:               return -2;
    case DXGI_FORMAT_BC2_UNORM:                  return -2;
    case DXGI_FORMAT_BC2_UNORM_SRGB:             return -2;
    case DXGI_FORMAT_BC3_TYPELESS:               return -2;
    case DXGI_FORMAT_BC3_UNORM:                  return -2;
    case DXGI_FORMAT_BC3_UNORM_SRGB:             return -2;
    case DXGI_FORMAT_BC4_TYPELESS:               return -1;
    case DXGI_FORMAT_BC4_UNORM:                  return -1;
    case DXGI_FORMAT_BC4_SNORM:                  return -1;
    case DXGI_FORMAT_BC5_TYPELESS:               return -2;
    case DXGI_FORMAT_BC5_UNORM:                  return -2;
    case DXGI_FORMAT_BC5_SNORM:                  return -2;

    case DXGI_FORMAT_B5G6R5_UNORM:               return 2;
    case DXGI_FORMAT_B5G5R5A1_UNORM:             return 2;
    case DXGI_FORMAT_B8G8R8X8_UNORM:             return 4;
    case DXGI_FORMAT_B8G8R8A8_UNORM:             return 4;
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM: return 4;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:          return 4;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:        return 4;
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:          return 4;
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:        return 4;

    case DXGI_FORMAT_BC6H_TYPELESS:              return -2;
    case DXGI_FORMAT_BC6H_UF16:                  return -2;
    case DXGI_FORMAT_BC6H_SF16:                  return -2;
    case DXGI_FORMAT_BC7_TYPELESS:               return -2;
    case DXGI_FORMAT_BC7_UNORM:                  return -2;
    case DXGI_FORMAT_BC7_UNORM_SRGB:             return -2;

    case DXGI_FORMAT_AYUV:                       return 0;
    case DXGI_FORMAT_Y410:                       return 0;
    case DXGI_FORMAT_Y416:                       return 0;
    case DXGI_FORMAT_NV12:                       return 0;
    case DXGI_FORMAT_P010:                       return 0;
    case DXGI_FORMAT_P016:                       return 0;
    case DXGI_FORMAT_420_OPAQUE:                 return 0;
    case DXGI_FORMAT_YUY2:                       return 0;
    case DXGI_FORMAT_Y210:                       return 0;
    case DXGI_FORMAT_Y216:                       return 0;
    case DXGI_FORMAT_NV11:                       return 0;
    case DXGI_FORMAT_AI44:                       return 0;
    case DXGI_FORMAT_IA44:                       return 0;
    case DXGI_FORMAT_P8:                         return 1;
    case DXGI_FORMAT_A8P8:                       return 2;
    case DXGI_FORMAT_B4G4R4A4_UNORM:             return 2;

    default: return 0;
  }
}

std::wstring
__stdcall
SK_DXGI_FormatToStr (DXGI_FORMAT fmt)
{
  switch (fmt)
  {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:      return L"DXGI_FORMAT_R32G32B32A32_TYPELESS";
    case DXGI_FORMAT_R32G32B32A32_FLOAT:         return L"DXGI_FORMAT_R32G32B32A32_FLOAT";
    case DXGI_FORMAT_R32G32B32A32_UINT:          return L"DXGI_FORMAT_R32G32B32A32_UINT";
    case DXGI_FORMAT_R32G32B32A32_SINT:          return L"DXGI_FORMAT_R32G32B32A32_SINT";

    case DXGI_FORMAT_R32G32B32_TYPELESS:         return L"DXGI_FORMAT_R32G32B32_TYPELESS";
    case DXGI_FORMAT_R32G32B32_FLOAT:            return L"DXGI_FORMAT_R32G32B32_FLOAT";
    case DXGI_FORMAT_R32G32B32_UINT:             return L"DXGI_FORMAT_R32G32B32_UINT";
    case DXGI_FORMAT_R32G32B32_SINT:             return L"DXGI_FORMAT_R32G32B32_SINT";

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:      return L"DXGI_FORMAT_R16G16B16A16_TYPELESS";
    case DXGI_FORMAT_R16G16B16A16_FLOAT:         return L"DXGI_FORMAT_R16G16B16A16_FLOAT";
    case DXGI_FORMAT_R16G16B16A16_UNORM:         return L"DXGI_FORMAT_R16G16B16A16_UNORM";
    case DXGI_FORMAT_R16G16B16A16_UINT:          return L"DXGI_FORMAT_R16G16B16A16_UINT";
    case DXGI_FORMAT_R16G16B16A16_SNORM:         return L"DXGI_FORMAT_R16G16B16A16_SNORM";
    case DXGI_FORMAT_R16G16B16A16_SINT:          return L"DXGI_FORMAT_R16G16B16A16_SINT";

    case DXGI_FORMAT_R32G32_TYPELESS:            return L"DXGI_FORMAT_R32G32_TYPELESS";
    case DXGI_FORMAT_R32G32_FLOAT:               return L"DXGI_FORMAT_R32G32_FLOAT";
    case DXGI_FORMAT_R32G32_UINT:                return L"DXGI_FORMAT_R32G32_UINT";
    case DXGI_FORMAT_R32G32_SINT:                return L"DXGI_FORMAT_R32G32_SINT";
    case DXGI_FORMAT_R32G8X24_TYPELESS:          return L"DXGI_FORMAT_R32G8X24_TYPELESS";

    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:       return L"DXGI_FORMAT_D32_FLOAT_S8X24_UINT";
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:   return L"DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS";
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:    return L"DXGI_FORMAT_X32_TYPELESS_G8X24_UINT";

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:       return L"DXGI_FORMAT_R10G10B10A2_TYPELESS";
    case DXGI_FORMAT_R10G10B10A2_UNORM:          return L"DXGI_FORMAT_R10G10B10A2_UNORM";
    case DXGI_FORMAT_R10G10B10A2_UINT:           return L"DXGI_FORMAT_R10G10B10A2_UINT";
    case DXGI_FORMAT_R11G11B10_FLOAT:            return L"DXGI_FORMAT_R11G11B10_FLOAT";

    case DXGI_FORMAT_R8G8B8A8_TYPELESS:          return L"DXGI_FORMAT_R8G8B8A8_TYPELESS";
    case DXGI_FORMAT_R8G8B8A8_UNORM:             return L"DXGI_FORMAT_R8G8B8A8_UNORM";
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:        return L"DXGI_FORMAT_R8G8B8A8_UNORM_SRGB";
    case DXGI_FORMAT_R8G8B8A8_UINT:              return L"DXGI_FORMAT_R8G8B8A8_UINT";
    case DXGI_FORMAT_R8G8B8A8_SNORM:             return L"DXGI_FORMAT_R8G8B8A8_SNORM";
    case DXGI_FORMAT_R8G8B8A8_SINT:              return L"DXGI_FORMAT_R8G8B8A8_SINT";

    case DXGI_FORMAT_R16G16_TYPELESS:            return L"DXGI_FORMAT_R16G16_TYPELESS";
    case DXGI_FORMAT_R16G16_FLOAT:               return L"DXGI_FORMAT_R16G16_FLOAT";
    case DXGI_FORMAT_R16G16_UNORM:               return L"DXGI_FORMAT_R16G16_UNORM";
    case DXGI_FORMAT_R16G16_UINT:                return L"DXGI_FORMAT_R16G16_UINT";
    case DXGI_FORMAT_R16G16_SNORM:               return L"DXGI_FORMAT_R16G16_SNORM";
    case DXGI_FORMAT_R16G16_SINT:                return L"DXGI_FORMAT_R16G16_SINT";

    case DXGI_FORMAT_R32_TYPELESS:               return L"DXGI_FORMAT_R32_TYPELESS";
    case DXGI_FORMAT_D32_FLOAT:                  return L"DXGI_FORMAT_D32_FLOAT";
    case DXGI_FORMAT_R32_FLOAT:                  return L"DXGI_FORMAT_R32_FLOAT";
    case DXGI_FORMAT_R32_UINT:                   return L"DXGI_FORMAT_R32_UINT";
    case DXGI_FORMAT_R32_SINT:                   return L"DXGI_FORMAT_R32_SINT";
    case DXGI_FORMAT_R24G8_TYPELESS:             return L"DXGI_FORMAT_R24G8_TYPELESS";

    case DXGI_FORMAT_D24_UNORM_S8_UINT:          return L"DXGI_FORMAT_D24_UNORM_S8_UINT";
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:      return L"DXGI_FORMAT_R24_UNORM_X8_TYPELESS";
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:       return L"DXGI_FORMAT_X24_TYPELESS_G8_UINT";

    case DXGI_FORMAT_R8G8_TYPELESS:              return L"DXGI_FORMAT_R8G8_TYPELESS";
    case DXGI_FORMAT_R8G8_UNORM:                 return L"DXGI_FORMAT_R8G8_UNORM";
    case DXGI_FORMAT_R8G8_UINT:                  return L"DXGI_FORMAT_R8G8_UINT";
    case DXGI_FORMAT_R8G8_SNORM:                 return L"DXGI_FORMAT_R8G8_SNORM";
    case DXGI_FORMAT_R8G8_SINT:                  return L"DXGI_FORMAT_R8G8_SINT";

    case DXGI_FORMAT_R16_TYPELESS:               return L"DXGI_FORMAT_R16_TYPELESS";
    case DXGI_FORMAT_R16_FLOAT:                  return L"DXGI_FORMAT_R16_FLOAT";
    case DXGI_FORMAT_D16_UNORM:                  return L"DXGI_FORMAT_D16_UNORM";
    case DXGI_FORMAT_R16_UNORM:                  return L"DXGI_FORMAT_R16_UNORM";
    case DXGI_FORMAT_R16_UINT:                   return L"DXGI_FORMAT_R16_UINT";
    case DXGI_FORMAT_R16_SNORM:                  return L"DXGI_FORMAT_R16_SNORM";
    case DXGI_FORMAT_R16_SINT:                   return L"DXGI_FORMAT_R16_SINT";

    case DXGI_FORMAT_R8_TYPELESS:                return L"DXGI_FORMAT_R8_TYPELESS";
    case DXGI_FORMAT_R8_UNORM:                   return L"DXGI_FORMAT_R8_UNORM";
    case DXGI_FORMAT_R8_UINT:                    return L"DXGI_FORMAT_R8_UINT";
    case DXGI_FORMAT_R8_SNORM:                   return L"DXGI_FORMAT_R8_SNORM";
    case DXGI_FORMAT_R8_SINT:                    return L"DXGI_FORMAT_R8_SINT";
    case DXGI_FORMAT_A8_UNORM:                   return L"DXGI_FORMAT_A8_UNORM";
    case DXGI_FORMAT_R1_UNORM:                   return L"DXGI_FORMAT_R1_UNORM";

    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:         return L"DXGI_FORMAT_R9G9B9E5_SHAREDEXP";
    case DXGI_FORMAT_R8G8_B8G8_UNORM:            return L"DXGI_FORMAT_R8G8_B8G8_UNORM";
    case DXGI_FORMAT_G8R8_G8B8_UNORM:            return L"DXGI_FORMAT_G8R8_G8B8_UNORM";

    case DXGI_FORMAT_BC1_TYPELESS:               return L"DXGI_FORMAT_BC1_TYPELESS";
    case DXGI_FORMAT_BC1_UNORM:                  return L"DXGI_FORMAT_BC1_UNORM";
    case DXGI_FORMAT_BC1_UNORM_SRGB:             return L"DXGI_FORMAT_BC1_UNORM_SRGB";
    case DXGI_FORMAT_BC2_TYPELESS:               return L"DXGI_FORMAT_BC2_TYPELESS";
    case DXGI_FORMAT_BC2_UNORM:                  return L"DXGI_FORMAT_BC2_UNORM";
    case DXGI_FORMAT_BC2_UNORM_SRGB:             return L"DXGI_FORMAT_BC2_UNORM_SRGB";
    case DXGI_FORMAT_BC3_TYPELESS:               return L"DXGI_FORMAT_BC3_TYPELESS";
    case DXGI_FORMAT_BC3_UNORM:                  return L"DXGI_FORMAT_BC3_UNORM";
    case DXGI_FORMAT_BC3_UNORM_SRGB:             return L"DXGI_FORMAT_BC3_UNORM_SRGB";
    case DXGI_FORMAT_BC4_TYPELESS:               return L"DXGI_FORMAT_BC4_TYPELESS";
    case DXGI_FORMAT_BC4_UNORM:                  return L"DXGI_FORMAT_BC4_UNORM";
    case DXGI_FORMAT_BC4_SNORM:                  return L"DXGI_FORMAT_BC4_SNORM";
    case DXGI_FORMAT_BC5_TYPELESS:               return L"DXGI_FORMAT_BC5_TYPELESS";
    case DXGI_FORMAT_BC5_UNORM:                  return L"DXGI_FORMAT_BC5_UNORM";
    case DXGI_FORMAT_BC5_SNORM:                  return L"DXGI_FORMAT_BC5_SNORM";

    case DXGI_FORMAT_B5G6R5_UNORM:               return L"DXGI_FORMAT_B5G6R5_UNORM";
    case DXGI_FORMAT_B5G5R5A1_UNORM:             return L"DXGI_FORMAT_B5G5R5A1_UNORM";
    case DXGI_FORMAT_B8G8R8X8_UNORM:             return L"DXGI_FORMAT_B8G8R8X8_UNORM";
    case DXGI_FORMAT_B8G8R8A8_UNORM:             return L"DXGI_FORMAT_B8G8R8A8_UNORM";
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM: return L"DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM";
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:          return L"DXGI_FORMAT_B8G8R8A8_TYPELESS";
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:        return L"DXGI_FORMAT_B8G8R8A8_UNORM_SRGB";
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:          return L"DXGI_FORMAT_B8G8R8X8_TYPELESS";
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:        return L"DXGI_FORMAT_B8G8R8X8_UNORM_SRGB";

    case DXGI_FORMAT_BC6H_TYPELESS:              return L"DXGI_FORMAT_BC6H_TYPELESS";
    case DXGI_FORMAT_BC6H_UF16:                  return L"DXGI_FORMAT_BC6H_UF16";
    case DXGI_FORMAT_BC6H_SF16:                  return L"DXGI_FORMAT_BC6H_SF16";
    case DXGI_FORMAT_BC7_TYPELESS:               return L"DXGI_FORMAT_BC7_TYPELESS";
    case DXGI_FORMAT_BC7_UNORM:                  return L"DXGI_FORMAT_BC7_UNORM";
    case DXGI_FORMAT_BC7_UNORM_SRGB:             return L"DXGI_FORMAT_BC7_UNORM_SRGB";

    case DXGI_FORMAT_AYUV:                       return L"DXGI_FORMAT_AYUV";
    case DXGI_FORMAT_Y410:                       return L"DXGI_FORMAT_Y410";
    case DXGI_FORMAT_Y416:                       return L"DXGI_FORMAT_Y416";
    case DXGI_FORMAT_NV12:                       return L"DXGI_FORMAT_NV12";
    case DXGI_FORMAT_P010:                       return L"DXGI_FORMAT_P010";
    case DXGI_FORMAT_P016:                       return L"DXGI_FORMAT_P016";
    case DXGI_FORMAT_420_OPAQUE:                 return L"DXGI_FORMAT_420_OPAQUE";
    case DXGI_FORMAT_YUY2:                       return L"DXGI_FORMAT_YUY2";
    case DXGI_FORMAT_Y210:                       return L"DXGI_FORMAT_Y210";
    case DXGI_FORMAT_Y216:                       return L"DXGI_FORMAT_Y216";
    case DXGI_FORMAT_NV11:                       return L"DXGI_FORMAT_NV11";
    case DXGI_FORMAT_AI44:                       return L"DXGI_FORMAT_AI44";
    case DXGI_FORMAT_IA44:                       return L"DXGI_FORMAT_IA44";
    case DXGI_FORMAT_P8:                         return L"DXGI_FORMAT_P8";
    case DXGI_FORMAT_A8P8:                       return L"DXGI_FORMAT_A8P8";
    case DXGI_FORMAT_B4G4R4A4_UNORM:             return L"DXGI_FORMAT_B4G4R4A4_UNORM";

    default:                                     return L"UNKNOWN";
  }
}

__declspec (noinline)
uint32_t
__cdecl
crc32_tex (  _In_      const D3D11_TEXTURE2D_DESC   *__restrict pDesc,
             _In_      const D3D11_SUBRESOURCE_DATA *__restrict pInitialData,
             _Out_opt_       size_t                 *__restrict pSize,
             _Out_opt_       uint32_t               *__restrict pLOD0_CRC32 )
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

  if (pDesc->MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE)
  {
    //SK_LOG0 ( ( L">>Neat! A cubemap!  [%lux%lu - Array: %lu :: pInitialData: %ph ]",
    //              pDesc->Width, pDesc->Height, pDesc->ArraySize, pInitialData ),
    //           L"DX11TexMgr" );
    return 0;
  }

        uint32_t checksum   = 0;
  const bool     compressed =
    SK_D3D11_IsFormatCompressed (pDesc->Format);

  const int bpp = ( (pDesc->Format >= DXGI_FORMAT_BC1_TYPELESS &&
                     pDesc->Format <= DXGI_FORMAT_BC1_UNORM_SRGB) ||
                    (pDesc->Format >= DXGI_FORMAT_BC4_TYPELESS &&
                     pDesc->Format <= DXGI_FORMAT_BC4_SNORM) ) ? 0 : 1;

  unsigned int width  = pDesc->Width;
  unsigned int height = pDesc->Height;

        size_t size   = 0UL;

  if (compressed)
  {
    for (unsigned int i = 0; i < pDesc->MipLevels; i++)
    {
      auto* pData    =
        static_cast  <char *> (
          const_cast <void *> (pInitialData [i].pSysMem)
        );

      UINT stride = bpp == 0 ?
             std::max (1UL, ((width + 3UL) / 4UL) ) *  8UL:
             std::max (1UL, ((width + 3UL) / 4UL) ) * 16UL;

      // Fast path:  Data is tightly packed and alignment agrees with
      //               convention...
      if (stride == pInitialData [i].SysMemPitch)
      {
        unsigned int lod_size = stride * (height / 4 +
                                          height % 4);

        __try {
          checksum  = crc32c (checksum, (const uint8_t *)pData, lod_size);
          size     += lod_size;
        }

        // Triggered by a certain JRPG that shall remain nameless (not so Marvelous!)
        __except ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                     EXCEPTION_EXECUTE_HANDLER :
                     EXCEPTION_CONTINUE_SEARCH )
        {
          size += stride * (height / 4) + (height % 4);
          SK_LOG0 ( ( L"Access Violation while Hashing Texture: %x", checksum ),
                      L" Tex Hash " );
        }
      }

      else
      {
        // We are running through the compressed image block-by-block,
        //  the lines we are "scanning" actually represent 4 rows of image data.
        for (unsigned int j = 0; j < height; j += 4)
        {
          uint32_t row_checksum =
            safe_crc32c (checksum, (const uint8_t *)pData, stride);

          if (row_checksum != 0x00)
            checksum = row_checksum;

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
    for (unsigned int i = 0; i < pDesc->MipLevels; i++)
    {
      auto* pData      =
        static_const_cast <char *, void *> (pInitialData [i].pSysMem);

      UINT  scanlength =
        SK_D3D11_BytesPerPixel (pDesc->Format) * width;

      // Fast path:  Data is tightly packed and alignment agrees with
      //               convention...
      if (scanlength == pInitialData [i].SysMemPitch) 
      {
        unsigned int lod_size =
          (scanlength * height);

        checksum = safe_crc32c ( checksum,
                                   static_cast <const uint8_t *> (
                                     static_const_cast <const void *, const char *> (pData)
                                   ),
                                  lod_size );
        size    += lod_size;
      }

      else if (pDesc->Usage != D3D11_USAGE_STAGING)
      {
        for (unsigned int j = 0; j < height; j++)
        {
          uint32_t row_checksum =
            safe_crc32c (checksum, (const uint8_t *)pData, scanlength);

          if (row_checksum != 0x00)
            checksum = row_checksum;

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

  bool compressed =
    SK_D3D11_IsFormatCompressed (pDesc->Format);

  int block_size = pDesc->Format == DXGI_FORMAT_BC1_UNORM ? 8 : 16;
  int height     = pDesc->Height;

  size_t size = 0;

  for (unsigned int i = 0; i < pDesc->MipLevels; i++)
  {
    if (compressed)
    {
      size += (pInitialData [i].SysMemPitch / block_size) * (height >> i);

      checksum =
        crc32 ( checksum, (const char *)pInitialData [i].pSysMem,
                  (pInitialData [i].SysMemPitch / block_size) * (height >> i) );
    }

    else
    {
      size += (pInitialData [i].SysMemPitch) * (height >> i);

      checksum =
        crc32 ( checksum, (const char *)pInitialData [i].pSysMem,
                  (pInitialData [i].SysMemPitch) * (height >> i) );
    }
  }

  if (pSize != nullptr)
    *pSize = size;

  return checksum;
}


bool
__stdcall
SK_D3D11_IsDumped (uint32_t top_crc32, uint32_t checksum)
{
  if (SK_D3D11_Textures.dumped_textures.empty ())
    return false;

  SK_AutoCriticalSection critical (&dump_cs);

  if (config.textures.d3d11.precise_hash && SK_D3D11_Textures.dumped_collisions.count (crc32c (top_crc32, (uint8_t *)&checksum, 4)))
    return true;
  if (! config.textures.d3d11.precise_hash)
    return SK_D3D11_Textures.dumped_textures.count (top_crc32) != 0;

  return false;
}

void
__stdcall
SK_D3D11_AddDumped (uint32_t top_crc32, uint32_t checksum)
{
  SK_AutoCriticalSection critical (&dump_cs);

  if (! config.textures.d3d11.precise_hash)
    SK_D3D11_Textures.dumped_textures.insert (top_crc32);

  SK_D3D11_Textures.dumped_collisions.insert (crc32c (top_crc32, (uint8_t *)&checksum, 4));
}

void
__stdcall
SK_D3D11_RemoveDumped (uint32_t top_crc32, uint32_t checksum)
{
  if (SK_D3D11_Textures.dumped_textures.empty ())
    return;

  SK_AutoCriticalSection critical (&dump_cs);

  if (! config.textures.d3d11.precise_hash)
    SK_D3D11_Textures.dumped_textures.erase (top_crc32);

  SK_D3D11_Textures.dumped_collisions.erase (crc32c (top_crc32, (uint8_t *)&checksum, 4));
}

bool
__stdcall
SK_D3D11_IsInjectable (uint32_t top_crc32, uint32_t checksum)
{
  if (SK_D3D11_Textures.injectable_textures.empty ())
    return false;

  SK_AutoCriticalSection critical (&inject_cs);

  if (checksum != 0x00)
  {
    if (SK_D3D11_Textures.injected_collisions.count (crc32c (top_crc32, (uint8_t *)&checksum, 4)))
      return true;

    return false;
  }

  return SK_D3D11_Textures.injectable_textures.count (top_crc32) != 0;
}

bool
__stdcall
SK_D3D11_IsInjectable_FFX (uint32_t top_crc32)
{
  if (SK_D3D11_Textures.injectable_ffx.empty ())
    return false;

  SK_AutoCriticalSection critical (&inject_cs);

  return SK_D3D11_Textures.injectable_ffx.count (top_crc32) != 0;
}


void
__stdcall
SK_D3D11_AddInjectable (uint32_t top_crc32, uint32_t checksum)
{
  SK_AutoCriticalSection critical (&inject_cs);

  if (checksum != 0x00)
    SK_D3D11_Textures.injected_collisions.insert (crc32c (top_crc32, (uint8_t *)&checksum, 4));

  SK_D3D11_Textures.injectable_textures.insert (top_crc32);
}


void
__stdcall
SK_D3D11_RemoveInjectable (uint32_t top_crc32, uint32_t checksum)
{
  SK_AutoCriticalSection critical (&inject_cs);

  if (checksum != 0x00)
    SK_D3D11_Textures.injected_collisions.erase (crc32c (top_crc32, (uint8_t *)&checksum, 4));

  SK_D3D11_Textures.injectable_textures.erase (top_crc32);
}

HRESULT
__stdcall
SK_D3D11_DumpTexture2D ( _In_ ID3D11Texture2D* pTex, uint32_t crc32c )
{
  CComPtr <ID3D11Device>        pDev    = nullptr;
  CComPtr <ID3D11DeviceContext> pDevCtx = nullptr;

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if ( SUCCEEDED (rb.device->QueryInterface              <ID3D11Device>        (&pDev))  &&
       SUCCEEDED (rb.d3d11.immediate_ctx->QueryInterface <ID3D11DeviceContext> (&pDevCtx)) )
  {
    DirectX::ScratchImage img;

    if (SUCCEEDED (DirectX::CaptureTexture (pDev, pDevCtx, pTex, img)))
    {
      wchar_t wszPath [ MAX_PATH + 2 ] = { };

      wcscpy ( wszPath,
                 SK_EvalEnvironmentVars (SK_D3D11_res_root.c_str ()).c_str () );

      lstrcatW (wszPath, L"/dump/textures/");
      lstrcatW (wszPath, SK_GetHostApp ());
      lstrcatW (wszPath, L"/");

      const DXGI_FORMAT fmt =
        img.GetMetadata ().format;

      bool compressed =
        SK_D3D11_IsFormatCompressed (fmt);

      wchar_t wszOutName [MAX_PATH + 2] = { };

      if (compressed)
      {
        _swprintf ( wszOutName, LR"(%s\Compressed_%08X.dds)",
                      wszPath, crc32c );
      }

      else
      {
        _swprintf ( wszOutName, LR"(%s\Uncompressed_%08X.dds)",
                      wszPath, crc32c );
      }

      SK_CreateDirectories (wszPath);

      HRESULT hr =
        DirectX::SaveToDDSFile ( img.GetImages   (), img.GetImageCount (),
                                 img.GetMetadata (), 0x00, wszOutName );

      if (SUCCEEDED (hr))
      {
        SK_D3D11_Textures.dumped_texture_bytes += SK_File_GetSize (wszOutName);

        SK_D3D11_AddDumped (crc32c, crc32c);

        return hr;
      }
    }
  }

  return E_FAIL;
}


bool
SK_D3D11_IsFormatSRGB (DXGI_FORMAT format)
{
  switch (format)
  {
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      return true;
    default:
      return false;
  }
}

DXGI_FORMAT
SK_D3D11_MakeFormatSRGB (DXGI_FORMAT format)
{
  switch (format)
  {
    //case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_TYPELESS:
      return DXGI_FORMAT_BC1_UNORM_SRGB;
    //case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_TYPELESS:
      return DXGI_FORMAT_BC2_UNORM_SRGB;
    //case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_TYPELESS:
      return DXGI_FORMAT_BC3_UNORM_SRGB;
    //case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_TYPELESS:
      return DXGI_FORMAT_BC7_UNORM_SRGB;
    //case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
      return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    //case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
      return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    //case DXGI_FORMAT_B8G8R8X8_UNORM:
      return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
    default:
      return format;
  }
}

DXGI_FORMAT
SK_D3D11_MakeTypedFormat (DXGI_FORMAT typeless)
{
  switch (typeless)
  {
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

    default:
      return typeless;
  };
}

DXGI_FORMAT
SK_D3D11_MakeTypelessFormat (DXGI_FORMAT typeless)
{
  switch (typeless)
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
      return DXGI_FORMAT_R8G8B8A8_TYPELESS;

    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      return DXGI_FORMAT_B8G8R8A8_TYPELESS;

    default:
      return typeless;
  };
}

HRESULT
__stdcall
SK_D3D11_MipmapCacheTexture2DEx ( const DirectX::ScratchImage&   img,
                                        uint32_t                 crc32c, 
                                        ID3D11Texture2D*       /*pOutTex*/,
                                        DirectX::ScratchImage** ppOutImg,
                                        SK_TLS*                  pTLS )
{
  SK_ScopedBool auto_bool  (&pTLS->texture_management.injection_thread);
  SK_ScopedBool auto_bool2 (&pTLS->imgui.drawing);

  pTLS->texture_management.injection_thread = true;
  pTLS->imgui.drawing                       = true;


  wchar_t wszPath [ MAX_PATH + 2 ] = { };

  wcscpy ( wszPath,
             SK_EvalEnvironmentVars (SK_D3D11_res_root.c_str ()).c_str () );

  lstrcatW (wszPath, L"/inject/textures/MipmapCache/");
  lstrcatW (wszPath, SK_GetHostApp ());
  lstrcatW (wszPath, L"/");

  wchar_t wszOutName [MAX_PATH + 2] = { };

  _swprintf ( wszOutName, LR"(%s\%08X.dds)",
                wszPath, crc32c );


  SK_CreateDirectories (wszPath);

  bool compressed =
    SK_D3D11_IsFormatCompressed (img.GetMetadata ().format);

  auto* mipmaps =
    new DirectX::ScratchImage;

  HRESULT ret  = E_FAIL;
  size_t  size = 0;

  if (compressed)
  {
          DirectX::ScratchImage decompressed;
    const DirectX::Image*       orig_img =
      img.GetImage (0, 0, 0);

    DirectX::TexMetadata meta =
      img.GetMetadata ();

    meta.format    = SK_D3D11_MakeTypedFormat (meta.format);
    meta.mipLevels = 1;

    ret =
      DirectX::Decompress (orig_img, 1, meta, DXGI_FORMAT_UNKNOWN, decompressed);

    if (SUCCEEDED (ret))
    {
      ret =
        DirectX::GenerateMipMaps ( decompressed.GetImage (0,0,0),
                                   1,
                                   decompressed.GetMetadata   (), DirectX::TEX_FILTER_FANT,
                                   0, *mipmaps );

      if (SUCCEEDED (ret))
      {
        if ((! config.textures.d3d11.uncompressed_mips))// && (! SK_D3D11_IsFormatBC6Or7 (meta.format)))
        {
          auto* compressed_mips =
            new DirectX::ScratchImage;

          DirectX::TexMetadata mipmap_meta =
            mipmaps->GetMetadata ();

          //
          // Required speedup since BC7 is _slow_
          //
          if (mipmap_meta.format == DXGI_FORMAT_BC7_UNORM)
              mipmap_meta.format = DXGI_FORMAT_BC5_UNORM;

          mipmap_meta.format           =
            SK_D3D11_MakeTypedFormat (mipmap_meta.format);

          DXGI_FORMAT newFormat =
            SK_D3D11_MakeTypedFormat (img.GetMetadata ().format);

          ret =
            DirectX::Compress ( //This,
                                  mipmaps->GetImages       (),
                                    mipmaps->GetImageCount (),
                                      mipmap_meta,
                                        newFormat,//DXGI_FORMAT_BC7_UNORM,
                                          DirectX::TEX_COMPRESS_DITHER //|
                                          ,//DirectX::TEX_COMPRESS_PARALLEL,
                                            DirectX::TEX_THRESHOLD_DEFAULT, *compressed_mips );

          if (SUCCEEDED (ret))
          {
            delete mipmaps;
                   mipmaps = compressed_mips;
          }
        }
      }
    }
  }

  else
  {
    DirectX::TexMetadata meta =
      img.GetMetadata ();

    meta.format = SK_D3D11_MakeTypedFormat (meta.format);

    ret =
      DirectX::GenerateMipMaps ( img.GetImages     (),
                                 img.GetImageCount (),
                                 meta,                 DirectX::TEX_FILTER_FANT,
                                   0, *mipmaps );
  }

  if (SUCCEEDED (ret) && mipmaps != nullptr)
  {
    if (config.textures.d3d11.cache_gen_mips)
    {
      DirectX::TexMetadata meta =
        mipmaps->GetMetadata ();

      meta.format = SK_D3D11_MakeTypedFormat (meta.format);

      if (SUCCEEDED (DirectX::SaveToDDSFile ( mipmaps->GetImages (), mipmaps->GetImageCount (),
                                              meta,                  0x00, wszOutName ) ) )
      {
        size = mipmaps->GetPixelsSize ();
      }
    }
  }
  

  if (SUCCEEDED (ret))
  {
    if (ppOutImg != nullptr)
      *ppOutImg = mipmaps;
    else if (mipmaps != nullptr)
      delete mipmaps;

    if (config.textures.d3d11.cache_gen_mips)// || SK_D3D11_IsTexInjectThread ())
    {
      extern uint64_t SK_D3D11_MipmapCacheSize;
                      SK_D3D11_MipmapCacheSize += size;

      SK_D3D11_AddDumped  (crc32c, crc32c);
      SK_D3D11_AddTexHash (wszOutName, crc32c, 0);
    }

    return ret;
  }

  if (mipmaps != nullptr)
    delete mipmaps;

  return E_FAIL;
}

HRESULT
__stdcall
SK_D3D11_MipmapCacheTexture2D ( _In_ ID3D11Texture2D* pTex, uint32_t crc32c, SK_TLS* pTLS = SK_TLS_Bottom () )
{
  CComPtr <ID3D11Device>        pDev    = nullptr;
  CComPtr <ID3D11DeviceContext> pDevCtx = nullptr;

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if ( SUCCEEDED (rb.device->QueryInterface              <ID3D11Device>        (&pDev))  &&
       SUCCEEDED (rb.d3d11.immediate_ctx->QueryInterface <ID3D11DeviceContext> (&pDevCtx)) )
  {
    DirectX::ScratchImage img;

    if (SUCCEEDED (DirectX::CaptureTexture (pDev, pDevCtx, pTex, img)))
    {
      return SK_D3D11_MipmapCacheTexture2DEx (img, crc32c, nullptr, nullptr, pTLS);
    }
  }

  return E_FAIL;
}


BOOL
SK_D3D11_DeleteDumpedTexture (uint32_t crc32c)
{
  wchar_t wszPath [ MAX_PATH + 2 ] = { };

  wcscpy ( wszPath,
             SK_EvalEnvironmentVars (SK_D3D11_res_root.c_str ()).c_str () );

  lstrcatW (wszPath, L"/dump/textures/");
  lstrcatW (wszPath, SK_GetHostApp ());
  lstrcatW (wszPath, L"/");

  wchar_t wszOutName [MAX_PATH + 2] = { };
  _swprintf ( wszOutName, LR"(%s\Compressed_%08X.dds)",
                wszPath, crc32c );

  uint64_t size = SK_File_GetSize (wszOutName);

  if (DeleteFileW (wszOutName))
  {
    SK_D3D11_RemoveDumped (crc32c, crc32c);

    SK_D3D11_Textures.dumped_texture_bytes -= size;

    return TRUE;
  }

  *wszOutName = L'\0';

  _swprintf ( wszOutName, LR"(%s\Uncompressed_%08X.dds)",
                wszPath, crc32c );

  size = SK_File_GetSize (wszOutName);

  if (DeleteFileW (wszOutName))
  {
    SK_D3D11_RemoveDumped (crc32c, crc32c);

    SK_D3D11_Textures.dumped_texture_bytes -= size;

    return TRUE;
  }

  return FALSE;
}

HRESULT
__stdcall
SK_D3D11_DumpTexture2D (  _In_ const D3D11_TEXTURE2D_DESC   *pDesc,
                          _In_ const D3D11_SUBRESOURCE_DATA *pInitialData,
                          _In_       uint32_t                top_crc32,
                          _In_       uint32_t                checksum )
{
  SK_LOG0 ( (L"Dumping Texture: %08x::%08x... (fmt=%03lu, "
                    L"BindFlags=0x%04x, Usage=0x%04x, CPUAccessFlags"
                    L"=0x%02x, Misc=0x%02x, MipLODs=%02lu, ArraySize=%02lu)",
                  top_crc32,
                    checksum,
  static_cast <UINT> (pDesc->Format),
                        pDesc->BindFlags,
                          pDesc->Usage,
                            pDesc->CPUAccessFlags,
                              pDesc->MiscFlags,
                                pDesc->MipLevels,
                                  pDesc->ArraySize), L"DX11TexDmp" );

  SK_D3D11_AddDumped (top_crc32, checksum);

  DirectX::TexMetadata mdata;

  mdata.width      = pDesc->Width;
  mdata.height     = pDesc->Height;
  mdata.depth      = 1;
  mdata.arraySize  = pDesc->ArraySize;
  mdata.mipLevels  = pDesc->MipLevels;
  mdata.miscFlags  = (pDesc->MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) ? 
                                            DirectX::TEX_MISC_TEXTURECUBE : 0;
  mdata.miscFlags2 = 0;
  mdata.format     = pDesc->Format;
  mdata.dimension  = DirectX::TEX_DIMENSION_TEXTURE2D;

  DirectX::ScratchImage image;
  image.Initialize (mdata);

  bool error = false;

  for (size_t slice = 0; slice < mdata.arraySize; ++slice)
  {
    size_t height = mdata.height;

    for (size_t lod = 0; lod < mdata.mipLevels; ++lod)
    {
      const DirectX::Image* img =
        image.GetImage (lod, slice, 0);

      if (! (img && img->pixels))
      {
        error = true;
        break;
      }

      const size_t lines =
        DirectX::ComputeScanlines (mdata.format, height);

      if  (! lines)
      {
        error = true;
        break;
      }

      auto sptr =
        static_cast <const uint8_t *>(
          pInitialData [lod].pSysMem
        );

      uint8_t* dptr =
        img->pixels;

      for (size_t h = 0; h < lines; ++h)
      {
        size_t msize =
          std::min <size_t> ( img->rowPitch,
                                pInitialData [lod].SysMemPitch );

        memcpy_s (dptr, img->rowPitch, sptr, msize);

        sptr += pInitialData [lod].SysMemPitch;
        dptr += img->rowPitch;
      }

      if (height > 1) height >>= 1;
    }

    if (error)
      break;
  }

  wchar_t wszPath [ MAX_PATH + 2 ] = { };

  wcscpy ( wszPath,
             SK_EvalEnvironmentVars (SK_D3D11_res_root.c_str ()).c_str () );

  lstrcatW (wszPath, L"/dump/textures/");
  lstrcatW (wszPath, SK_GetHostApp ());
  lstrcatW (wszPath, L"/");

  SK_CreateDirectories (wszPath);

  bool compressed =
    SK_D3D11_IsFormatCompressed (pDesc->Format);

  wchar_t wszOutPath [MAX_PATH + 2] = { };
  wchar_t wszOutName [MAX_PATH + 2] = { };

  wcscpy ( wszOutPath,
             SK_EvalEnvironmentVars (SK_D3D11_res_root.c_str ()).c_str () );

  lstrcatW (wszOutPath, LR"(\dump\textures\)");
  lstrcatW (wszOutPath, SK_GetHostApp ());

  if (compressed && config.textures.d3d11.precise_hash) {
    _swprintf ( wszOutName, LR"(%s\Compressed_%08X_%08X.dds)",
                  wszOutPath, top_crc32, checksum );
  } else if (compressed) {
    _swprintf ( wszOutName, LR"(%s\Compressed_%08X.dds)",
                  wszOutPath, top_crc32 );
  } else if (config.textures.d3d11.precise_hash) {
    _swprintf ( wszOutName, LR"(%s\Uncompressed_%08X_%08X.dds)",
                  wszOutPath, top_crc32, checksum );
  } else {
    _swprintf ( wszOutName, LR"(%s\Uncompressed_%08X.dds)",
                  wszOutPath, top_crc32 );
  }

  if ((! error) && wcslen (wszOutName))
  {
    if (GetFileAttributes (wszOutName) == INVALID_FILE_ATTRIBUTES)
    {
      SK_LOG0 ( (L" >> File: '%s' (top: %x, full: %x)",
                      wszOutName,
                        top_crc32,
                          checksum), L"DX11TexDmp" );

#if 0
      wchar_t wszMetaFilename [ MAX_PATH + 2 ] = { };

      _swprintf (wszMetaFilename, L"%s.txt", wszOutName);

      FILE* fMetaData = _wfopen (wszMetaFilename, L"w+");

      if (fMetaData != nullptr) {
        fprintf ( fMetaData,
                  "Dumped Name:    %ws\n"
                  "Texture:        %08x::%08x\n"
                  "Dimensions:     (%lux%lu)\n"
                  "Format:         %03lu\n"
                  "BindFlags:      0x%04x\n"
                  "Usage:          0x%04x\n"
                  "CPUAccessFlags: 0x%02x\n"
                  "Misc:           0x%02x\n"
                  "MipLODs:        %02lu\n"
                  "ArraySize:      %02lu",
                  wszOutName,
                    top_crc32,
                      checksum,
                        pDesc->Width, pDesc->Height,
                        pDesc->Format,
                          pDesc->BindFlags,
                            pDesc->Usage,
                              pDesc->CPUAccessFlags,
                                pDesc->MiscFlags,
                                  pDesc->MipLevels,
                                    pDesc->ArraySize );

        fclose (fMetaData);
      }
#endif

      HRESULT hr =  SaveToDDSFile ( image.GetImages (), image.GetImageCount (),
                                      image.GetMetadata (), DirectX::DDS_FLAGS_NONE,
                                        wszOutName );

      if (SUCCEEDED (hr))
      {
        SK_D3D11_Textures.dumped_texture_bytes += SK_File_GetSize (wszOutName);
      }
    }
  }

  return E_FAIL;
}

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateBuffer_Override (
  _In_           ID3D11Device            *This,
  _In_     const D3D11_BUFFER_DESC       *pDesc,
  _In_opt_ const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_      ID3D11Buffer           **ppBuffer )
{
  return
    D3D11Dev_CreateBuffer_Original ( This, pDesc,
                                       pInitialData, ppBuffer );
}



#if 0
DEFINE_GUID(IID_ID3D11ShaderResourceView,0xb0e06fe0,0x8192,0x4e1a,0xb1,0xca,0x36,0xd7,0x41,0x47,0x10,0xb2);
typedef interface ID3D11ShaderResourceView ID3D11ShaderResourceView;

EXTERN_C const IID IID_ID3D11ShaderResourceView1;

typedef struct D3D11_TEX2D_SRV1
{
  UINT MostDetailedMip;
  UINT MipLevels;
  UINT PlaneSlice;
} D3D11_TEX2D_SRV1;

typedef struct D3D11_TEX2D_ARRAY_SRV1
{
  UINT MostDetailedMip;
  UINT MipLevels;
  UINT FirstArraySlice;
  UINT ArraySize;
  UINT PlaneSlice;
} D3D11_TEX2D_ARRAY_SRV1;

typedef struct D3D11_SHADER_RESOURCE_VIEW_DESC1
{
  DXGI_FORMAT         Format;
  D3D11_SRV_DIMENSION ViewDimension;

  unionP
  {
    D3D11_BUFFER_SRV        Buffer;
    D3D11_TEX1D_SRV         Texture1D;
    D3D11_TEX1D_ARRAY_SRV   Texture1DArray;
    D3D11_TEX2D_SRV1        Texture2D;
    D3D11_TEX2D_ARRAY_SRV1  Texture2DArray;
    D3D11_TEX2DMS_SRV       Texture2DMS;
    D3D11_TEX2DMS_ARRAY_SRV Texture2DMSArray;
    D3D11_TEX3D_SRV         Texture3D;
    D3D11_TEXCUBE_SRV       TextureCube;
    D3D11_TEXCUBE_ARRAY_SRV TextureCubeArray;
    D3D11_BUFFEREX_SRV      BufferEx;
  };
} D3D11_SHADER_RESOURCE_VIEW_DESC1;

MIDL_INTERFACE("91308b87-9040-411d-8c67-c39253ce3802")
ID3D11ShaderResourceView1 : public ID3D11ShaderResourceView
{
public:
    virtual void STDMETHODCALLTYPE GetDesc1( 
        /* [annotation] */ 
        _Out_  D3D11_SHADER_RESOURCE_VIEW_DESC1 *pDesc1) = 0;
    
};

interface SK_ID3D11_ShaderResourceView : public ID3D11ShaderResourceView1
{
public:
  explicit SK_ID3D11_ShaderResourceView (ID3D11ShaderResourceView *pView) : pRealSRV (pView)
  {
    iver_ = 0;
    InterlockedExchange (&refs_, 1);
  }

  explicit SK_ID3D11_ShaderResourceView (ID3D11ShaderResourceView1 *pView) : pRealSRV (pView)
  {
    iver_ = 1;
    InterlockedExchange (&refs_, 1);
  }

  SK_ID3D11_ShaderResourceView            (const SK_ID3D11_ShaderResourceView &) = delete;
  SK_ID3D11_ShaderResourceView &operator= (const SK_ID3D11_ShaderResourceView &) = delete;

  __declspec (noinline)
  virtual HRESULT STDMETHODCALLTYPE QueryInterface (
    _In_         REFIID                     riid,
    _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject )    override
  {
    if (ppvObject == nullptr)
    {
      return E_POINTER;
    }

    else if ( riid == __uuidof (IUnknown)                 ||
              riid == __uuidof (ID3D11DeviceChild)        ||
              riid == __uuidof (ID3D11View)               ||
              riid == __uuidof (ID3D11ShaderResourceView) ||
              riid == __uuidof (ID3D11ShaderResourceView1) )
    {
#pragma region Update to ID3D11ShaderResourceView1 interface
      if (riid == __uuidof (ID3D11ShaderResourceView1) && iver_ < 1)
      {
        ID3D11ShaderResourceView1* pSRV1 = nullptr;

        if (FAILED (pRealSRV->QueryInterface (&pSRV1)))
        {
          return E_NOINTERFACE;
        }

        pRealSRV->Release ();

        SK_LOG0 ( ( L"Adding 'ID3D11ShaderResourceView1' (v1) interfaces to %ph, whose original interface was limited to 'ID3D11ShaderResourceView' (v0).",
                      this ),
                    L"DX11ResMgr" );

        pRealSRV = pSRV1;

        iver_ = 1;
      }
#pragma endregion

      AddRef ();
      
      *ppvObject = this;

      return S_OK;
    }

    else
      return pRealSRV->QueryInterface (riid, ppvObject);
  }

  __declspec (noinline)
  virtual ULONG STDMETHODCALLTYPE AddRef (void)            override
  {
    InterlockedIncrement (&refs_);

    assert (pRealSRV != nullptr);

    return pRealSRV->AddRef ();
  }

  __declspec (noinline)
  virtual ULONG STDMETHODCALLTYPE Release (void)           override
  {
    assert (pRealSRV != nullptr);


    if (InterlockedAdd (&refs_, 0) > 0)
    {
      ULONG local_refs =
        pRealSRV->Release ();

      if (InterlockedDecrement (&refs_) == 0 && local_refs != 0)
      {
        SK_LOG0 ( ( L"WARNING: 'ID3D11ShaderResourceView%s (%ph) has %lu references; expected 0.",
                    iver_ > 0 ? (std::to_wstring (iver_) + L"'").c_str () :
                                                           L"'",
                      this,
                        local_refs ),
                    L"DX11ResMgr"
                );

        local_refs = 0;
      }

      if (local_refs == 0)
      {
        assert (InterlockedAdd (refs_, 0) <= 0);

        SK_LOG1 ( ( L"Destroying Custom Shader Resource View" ),
                    L"DX11TexMgr" );

        delete this;
      }

      return local_refs;
    }

    return 0;
  }

  template <class Q>
  HRESULT
#ifdef _M_CEE_PURE
  __clrcall
#else
  STDMETHODCALLTYPE
#endif
  QueryInterface (_COM_Outptr_ Q** pp)
  {
    return QueryInterface (__uuidof (Q), (void **)pp);
  }

  __declspec (noinline)
  virtual void STDMETHODCALLTYPE GetDevice ( 
      /* [annotation] */ 
      _Out_  ID3D11Device **ppDevice)                        override
  {
    assert (pRealSRV != nullptr);

    pRealSRV->GetDevice (ppDevice);
  }
  
  __declspec (noinline)
  virtual HRESULT STDMETHODCALLTYPE GetPrivateData ( 
      _In_                                  REFGUID  guid,
      _Inout_                               UINT    *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void    *pData)  override
  {
    return pRealSRV->GetPrivateData (guid, pDataSize, pData);
  }
  
  __declspec (noinline)
  virtual HRESULT STDMETHODCALLTYPE SetPrivateData ( 
      _In_                                    REFGUID guid,
      _In_                                    UINT    DataSize,
      _In_reads_bytes_opt_( DataSize )  const void   *pData) override
  {
    return pRealSRV->SetPrivateData (guid, DataSize, pData);
  }
  
  __declspec (noinline)
  virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface ( 
      _In_            REFGUID   guid,
      _In_opt_  const IUnknown *pData)                       override
  {
    return pRealSRV->SetPrivateDataInterface (guid, pData);
  }

  __declspec (noinline)
  virtual void STDMETHODCALLTYPE GetResource ( 
      _Out_  ID3D11Resource **ppResource )                   override
  {
    pRealSRV->GetResource (ppResource);
  }

  __declspec (noinline)
  virtual void STDMETHODCALLTYPE GetDesc (
      _Out_  D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc )        override
  {
    pRealSRV->GetDesc (pDesc);
  }

#pragma region ID3D11ShaderResourceView1
  __declspec (noinline)
  virtual void STDMETHODCALLTYPE GetDesc1 (
      _Out_  D3D11_SHADER_RESOURCE_VIEW_DESC1 *pDesc )       override
  {
    reinterpret_cast <ID3D11ShaderResourceView1 *> (pRealSRV)->GetDesc1 (pDesc);
  }
#pragma endregion


           ID3D11ShaderResourceView* pRealSRV = nullptr;
           unsigned int                 iver_ = 0;
  volatile LONG                         refs_ = 1;
};
#endif

#include <unordered_set>

HRESULT
SEH_CreateSRV (
  _In_           ID3D11Device                     *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC  *pDesc,
  _Out_opt_      ID3D11ShaderResourceView        **ppSRView )
{
  HRESULT hr = E_OUTOFMEMORY;

  __try {
    hr =
      D3D11Dev_CreateShaderResourceView_Original ( This, pResource,
                                                  pDesc, ppSRView );
  }
  __except (EXCEPTION_EXECUTE_HANDLER) {
  }

  return hr;
}

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateShaderResourceView_Override (
  _In_           ID3D11Device                     *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC  *pDesc,
  _Out_opt_      ID3D11ShaderResourceView        **ppSRView )
{
  if (pDesc != nullptr && pResource != nullptr)
  {
    D3D11_RESOURCE_DIMENSION   dim;
    pResource->GetType       (&dim);
  
    if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      ////if (SK_GetCurrentGameID () == SK_GAME_ID::DotHackGU)
      ////{
      ////  if (pDesc != nullptr && pDesc->Format == DXGI_FORMAT_B8G8R8A8_UNORM)
      ////    ((D3D11_SHADER_RESOURCE_VIEW_DESC *)pDesc)->Format = DXGI_FORMAT_R8G8B8A8_UNORM;
      ////}
  
      DXGI_FORMAT newFormat    = pDesc->Format;
      UINT        newMipLevels = pDesc->Texture2D.MipLevels;
  
      CComQIPtr <ID3D11Texture2D> pTex (pResource);
  
      D3D11_TEXTURE2D_DESC        tex_desc = { };
      if (pTex) pTex->GetDesc   (&tex_desc);
      if (pTex != nullptr)// && (!((tex_desc.BindFlags & D3D11_BIND_RENDER_TARGET)||
                          //      (tex_desc.BindFlags & D3D11_BIND_DEPTH_STENCIL))))
      {
        bool override = false;
  
        if ( pDesc->Format      == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB &&
             tex_desc.Format    == DXGI_FORMAT_R8G8B8A8_UNORM      &&
            (tex_desc.BindFlags &  D3D11_BIND_RENDER_TARGET) )
        {
          override  = true;
          newFormat = tex_desc.Format;
        }
  
        if ( SK_D3D11_OverrideDepthStencil (newFormat) )
          override = true;
  
        if ( SK_D3D11_TextureIsCached (pTex) )
        {
          auto& cache_desc =
            SK_D3D11_Textures.Textures_2D [pTex];
  
          newFormat =
            cache_desc.desc.Format;
  
          newMipLevels =
            pDesc->Texture2D.MipLevels;
  
          if (pDesc->Format != newFormat)// && SK_DXGI_FormatToStr (pDesc->Format) != L"UNKNOWN")
          {
            if (SK_D3D11_IsFormatSRGB (pDesc->Format))
              newFormat = SK_D3D11_MakeFormatSRGB (newFormat);
  
            override = true;
  
            SK_LOG1 ( ( L"Overriding Resource View Format for Cached Texture '%08x'  { Was: '%s', Now: '%s' }",
                          cache_desc.crc32c,
                     SK_DXGI_FormatToStr (pDesc->Format).c_str      (),
                              SK_DXGI_FormatToStr (newFormat).c_str () ),
                        L"DX11TexMgr" );
          }
  
          if (config.textures.d3d11.generate_mips && cache_desc.desc.MipLevels != pDesc->Texture2D.MipLevels)
          {
            override     = true;
            newMipLevels = cache_desc.desc.MipLevels;
  
            SK_LOG1 ( ( L"Overriding Resource View Mip Levels for Cached Texture '%08x'  { Was: %lu, Now: %lu }",
                          cache_desc.crc32c,
                            pDesc->Texture2D.MipLevels,
                               newMipLevels ),
                        L"DX11TexMgr" );
          }
        }
  
        if (override)
        {
          auto descCopy = *pDesc;
  
          descCopy.Format                    = newFormat;
  
          if (newMipLevels != pDesc->Texture2D.MipLevels)
          {
            descCopy.Texture2D.MipLevels       = (UINT)-1;
            descCopy.Texture2D.MostDetailedMip =        0;
          }
  
          HRESULT hr =
            SEH_CreateSRV (This, pResource, &descCopy, ppSRView);
  
          if (SUCCEEDED (hr))
          {
            return hr;
          }
        }
      }
    }
  }

  dll_log.Log (L"CreateShaderResourceView (%x, %x, %x, %x)", This, pResource, pDesc, ppSRView);

  return
    SEH_CreateSRV (This, pResource, pDesc, ppSRView);
}

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateDepthStencilView_Override (
  _In_            ID3D11Device                  *This,
  _In_            ID3D11Resource                *pResource,
  _In_opt_  const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,
  _Out_opt_       ID3D11DepthStencilView        **ppDepthStencilView )
{
  const D3D11_DEPTH_STENCIL_VIEW_DESC* pDescOrig = pDesc;

  HRESULT hr =
    E_UNEXPECTED;

  if (pDesc != nullptr && pResource != nullptr)
  {
    D3D11_RESOURCE_DIMENSION dim;
    pResource->GetType (&dim);

    if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      DXGI_FORMAT newFormat = pDesc->Format;

      CComQIPtr <ID3D11Texture2D> pTex (pResource);

      if (pTex != nullptr)
      {
        D3D11_TEXTURE2D_DESC tex_desc;
             pTex->GetDesc (&tex_desc);

        if ( SK_D3D11_OverrideDepthStencil (newFormat) )
        {
          auto descCopy = *pDescOrig;

          descCopy.Format = newFormat;
          pDesc           = &descCopy;

          hr = 
            D3D11Dev_CreateDepthStencilView_Original ( This, pResource,
                                                         pDesc, ppDepthStencilView );
        }

        if (SUCCEEDED (hr))
          return hr;
      }
    }

    pDesc = pDescOrig;
  }

  hr =
    D3D11Dev_CreateDepthStencilView_Original ( This, pResource,
                                                 pDesc, ppDepthStencilView );
  return hr;
}

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateUnorderedAccessView_Override (
  _In_            ID3D11Device                     *This,
  _In_            ID3D11Resource                   *pResource,
  _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc,
  _Out_opt_       ID3D11UnorderedAccessView       **ppUAView )
{
  const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDescOrig = pDesc;

  if (pDesc != nullptr && pResource != nullptr)
  {
    D3D11_RESOURCE_DIMENSION dim;

    pResource->GetType (&dim);

    if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      DXGI_FORMAT newFormat = pDesc->Format;

      CComQIPtr <ID3D11Texture2D> pTex (pResource);

      if (pTex != nullptr)
      {
        bool                 override = false;

        D3D11_TEXTURE2D_DESC tex_desc = { };
             pTex->GetDesc (&tex_desc);

        if ( SK_D3D11_OverrideDepthStencil (newFormat) )
          override = true;

        if (override)
        {
          auto descCopy = *pDescOrig;

          descCopy.Format = newFormat;
          pDesc           = &descCopy;

          HRESULT hr = 
            D3D11Dev_CreateUnorderedAccessView_Original ( This, pResource,
                                                            pDesc, ppUAView );

          if (SUCCEEDED (hr))
            return hr;
        }
      }
    }

    pDesc = pDescOrig;
  }

  HRESULT hr =
    D3D11Dev_CreateUnorderedAccessView_Original ( This, pResource,
                                                    pDesc, ppUAView );
  return hr;
}


std::unordered_set <ID3D11Texture2D *> render_tex;

bool b_66b35959 = false;
bool b_9d665ae2 = false;
bool b_b21c8ab9 = false;
bool b_6bb0972d = false;
bool b_05da09bd = true;

void
WINAPI
D3D11_PSSetSamplers_Override
(
  _In_     ID3D11DeviceContext       *This,
  _In_     UINT                       StartSlot,
  _In_     UINT                       NumSamplers,
  _In_opt_ ID3D11SamplerState *const *ppSamplers )
{
#if 0
  if ( ppSamplers != nullptr )
  {
    //if (SK_GetCurrentRenderBackend ().d3d11.immediate_ctx.IsEqualObject (This))
    if (SK_GetCurrentRenderBackend ().device.p != nullptr)
    {
      SK_TLS *pTLS =
        SK_TLS_Bottom ();

      ID3D11SamplerState** pSamplerCopy =
        (ID3D11SamplerState **)pTLS->scratch_memory.cmd.alloc (
           sizeof (ID3D11SamplerState  *) * 4096
        );

      bool ys8_wrap_ui  = false,
           ys8_clamp_ui = false;

      if (SK_GetCurrentGameID () == SK_GAME_ID::Ys_Eight)
      {
        SK_D3D11_EnableTracking = true;

        auto HashFromCtx =
          [] ( std::array <uint32_t, SK_D3D11_MAX_DEV_CONTEXTS+1>& registry,
               UINT                                                dev_idx ) ->
        uint32_t
        {
          return registry [dev_idx];
        };

        UINT dev_idx = SK_D3D11_GetDeviceContextHandle (This);

        uint32_t current_ps = HashFromCtx (SK_D3D11_Shaders.pixel.current.shader,  dev_idx);
        uint32_t current_vs = HashFromCtx (SK_D3D11_Shaders.vertex.current.shader, dev_idx);

        switch (current_ps)
        {
          case 0x66b35959:
          case 0x9d665ae2:
          case 0xb21c8ab9:
          case 0x05da09bd:
          {
            if (current_ps == 0x66b35959 && b_66b35959)                             ys8_clamp_ui = true;
            if (current_ps == 0x9d665ae2 && b_9d665ae2)                             ys8_clamp_ui = true;
            if (current_ps == 0xb21c8ab9 && b_b21c8ab9)                             ys8_clamp_ui = true;
            if (current_ps == 0x05da09bd && b_05da09bd && current_vs == 0x7759c300) ys8_clamp_ui = true;
          }break;
          case 0x6bb0972d:
          {
            if (current_ps == 0x6bb0972d && b_6bb0972d) ys8_wrap_ui = true;
          } break;
        }
      }

      if (! (pTLS->imgui.drawing || ys8_clamp_ui || ys8_wrap_ui))
      {
        for ( UINT i = 0 ; i < NumSamplers ; i++ )
        {
          pSamplerCopy [i] = ppSamplers [i];

          if (ppSamplers [i] != nullptr)
          {
            D3D11_SAMPLER_DESC        new_desc = { };
            ppSamplers [i]->GetDesc (&new_desc);

            ((ID3D11Device *)SK_GetCurrentRenderBackend ().device.p)->CreateSamplerState (
                                                     &new_desc,
                                                       &pSamplerCopy [i] );
          }
        }
      }

      else
      {
        for ( UINT i = 0 ; i < NumSamplers ; i++ )
        {
          if (! ys8_wrap_ui)
            pSamplerCopy [i] = pTLS->d3d11.uiSampler_clamp;
          else
            pSamplerCopy [i] = pTLS->d3d11.uiSampler_wrap;
        }
      }

      return
        D3D11_PSSetSamplers_Original (This, StartSlot, NumSamplers, pSamplerCopy);
    }
  }
#endif

  D3D11_PSSetSamplers_Original (This, StartSlot, NumSamplers, ppSamplers);
}

HRESULT
WINAPI
D3D11Dev_CreateSamplerState_Override
(
  _In_            ID3D11Device        *This,
  _In_      const D3D11_SAMPLER_DESC  *pSamplerDesc,
  _Out_opt_       ID3D11SamplerState **ppSamplerState )
{
  D3D11_SAMPLER_DESC new_desc = *pSamplerDesc;


  if (SK_GetCurrentGameID () == SK_GAME_ID::LEGOMarvelSuperheroes2)
  {
    if (new_desc.Filter <= D3D11_FILTER_ANISOTROPIC)
    {
      new_desc.Filter        = D3D11_FILTER_ANISOTROPIC;
      new_desc.MaxAnisotropy = 16;

      new_desc.MipLODBias    = 0.0f;
      new_desc.MinLOD        = 0.0f;
      new_desc.MaxLOD        = D3D11_FLOAT32_MAX;

      HRESULT hr =
        D3D11Dev_CreateSamplerState_Original (This, &new_desc, ppSamplerState);

      if (SUCCEEDED (hr))
        return hr;
    }
  }

  if (SK_GetCurrentGameID () != SK_GAME_ID::Ys_Eight)
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
    
      HRESULT hr =
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
        SK_LOG0 ( ( L"Changing Shadow Filter from '%s' to '%s'",
                      SK_D3D11_DescribeFilter (pSamplerDesc->Filter),
                           SK_D3D11_DescribeFilter (new_desc.Filter) ),
                    L" TexCache " );
      }
    
      HRESULT hr =
        D3D11Dev_CreateSamplerState_Original (This, &new_desc, ppSamplerState);
    
      if (SUCCEEDED (hr))
        return hr;
    }
  }

  return
    D3D11Dev_CreateSamplerState_Original (This, pSamplerDesc, ppSamplerState);
}


#ifdef _SK_WITHOUT_D3DX11
[[deprecated]]
#endif
HRESULT
SK_D3DX11_SAFE_GetImageInfoFromFileW (const wchar_t* wszTex, D3DX11_IMAGE_INFO* pInfo)
{
#ifndef _SK_WITHOUT_D3DX11
  __try {
    return D3DX11GetImageInfoFromFileW (wszTex, nullptr, pInfo, nullptr);
  }

  __except ( /* GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ? */
               EXCEPTION_EXECUTE_HANDLER /* :
               EXCEPTION_CONTINUE_SEARCH */ )
  {
    SK_LOG0 ( ( L"Texture '%s' is corrupt, please delete it.",
                  wszTex ),
                L" TexCache " );

    return E_FAIL;
  }
#else
  return E_NOTIMPL;
#endif
}

#ifdef _SK_WITHOUT_D3DX11
[[deprecated]]
#endif
HRESULT
SK_D3DX11_SAFE_CreateTextureFromFileW ( ID3D11Device*           pDevice,   LPCWSTR           pSrcFile,
                                        D3DX11_IMAGE_LOAD_INFO* pLoadInfo, ID3D11Resource** ppTexture )
{
#ifndef _SK_WITHOUT_D3DX11
  __try {
    return D3DX11CreateTextureFromFileW (pDevice, pSrcFile, pLoadInfo, nullptr, ppTexture, nullptr);
  }

  __except ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
               EXCEPTION_EXECUTE_HANDLER :
               EXCEPTION_CONTINUE_SEARCH )
  {
    return E_FAIL;
  }
#else
  return E_NOTIMPL;
#endif
}


std::wstring
SK_D3D11_TexNameFromChecksum (uint32_t top_crc32, uint32_t checksum, uint32_t ffx_crc32 = 0x00);

HRESULT
SK_D3D11_ReloadTexture ( ID3D11Texture2D* pTex,
                         SK_TLS*          pTLS = SK_TLS_Bottom () )
{
  HRESULT hr =
    E_UNEXPECTED;

  SK_ScopedBool auto_bool  (&pTLS->texture_management.injection_thread);
  SK_ScopedBool auto_bool2 (&pTLS->imgui.drawing);

  pTLS->texture_management.injection_thread = true;
  pTLS->imgui.drawing                       = true;
  {
    SK_D3D11_TexMgr::tex2D_descriptor_s texDesc2D =
      SK_D3D11_Textures.Textures_2D [pTex];

    std::wstring fname = 
      SK_D3D11_TexNameFromChecksum (texDesc2D.crc32c, 0x00);

    if (fname.empty ()) fname = texDesc2D.file_name;

    else
      texDesc2D.file_name = fname;

    if (GetFileAttributes (fname.c_str ()) != INVALID_FILE_ATTRIBUTES )
    {
#define D3DX11_DEFAULT static_cast <UINT> (-1)

      D3DX11_IMAGE_INFO      img_info   = {   };
      D3DX11_IMAGE_LOAD_INFO load_info  = {   };

      LARGE_INTEGER load_start =
        SK_QueryPerf ();

	  DirectX::TexMetadata mdata;

	  
      if ( SUCCEEDED (
		    ( hr = DirectX::GetMetadataFromDDSFile ( fname.c_str (),
				                                       0x0,
				                                         mdata )
	        )        )
		 )
      {
        load_info.BindFlags      = texDesc2D.desc.BindFlags;
        load_info.CpuAccessFlags = texDesc2D.desc.CPUAccessFlags;
        load_info.Depth          = texDesc2D.desc.ArraySize;
        load_info.Filter         = D3DX11_DEFAULT;
        load_info.FirstMipLevel  = 0;
      
        if (config.textures.d3d11.injection_keeps_fmt)
          load_info.Format       = texDesc2D.desc.Format;
        else
          load_info.Format       = mdata.format;

        load_info.Height         = texDesc2D.desc.Height;
        load_info.MipFilter      = D3DX11_DEFAULT;
        load_info.MipLevels      = texDesc2D.desc.MipLevels;
        load_info.MiscFlags      = texDesc2D.desc.MiscFlags;
        load_info.Usage          = D3D11_USAGE_DEFAULT;
        load_info.Width          = texDesc2D.desc.Width;

		DirectX::ScratchImage scratch;

        CComPtr   <ID3D11Texture2D> pInjTex = nullptr;
        CComQIPtr <ID3D11Device>    pDev (SK_GetCurrentRenderBackend ().device);

        hr =
		  DirectX::LoadFromDDSFile (fname.c_str (), 0x0, &mdata, scratch);

        if (SUCCEEDED (hr))
        {
		  if ( SUCCEEDED (
			     DirectX::CreateTexture (pDev, scratch.GetImages (), scratch.GetImageCount (), mdata, (ID3D11Resource **)&pInjTex.p)
		       )
			 )
		  {
			CComQIPtr <ID3D11DeviceContext> pDevCtx (
			  SK_GetCurrentRenderBackend ().d3d11.immediate_ctx
			);

			pDevCtx->CopyResource (pTex, pInjTex);

			LARGE_INTEGER load_end =
			  SK_QueryPerf ();

			SK_D3D11_Textures.Textures_2D [pTex].load_time = (load_end.QuadPart - load_start.QuadPart);

			return S_OK;
		  }
        }
      }
    }
  }

  SK_LOG0 ( ( L" >> Texture Reload Failure (HRESULT: %x)", hr),
              L"DX11TexMgr" );

  return E_FAIL;
}


int
SK_D3D11_ReloadAllTextures (void)
{
  SK_D3D11_PopulateResourceList (true);

  int count = 0;

  for ( auto& it : SK_D3D11_Textures.Textures_2D )
  {
    if (SK_D3D11_TextureIsCached (it.first))
    {
      if (! (it.second.injected || it.second.discard))
        continue;

      if (SUCCEEDED (SK_D3D11_ReloadTexture (it.first)))
        ++count;
    }
  }

  return count;
}


std::wstring
SK_D3D11_TexNameFromChecksum (uint32_t top_crc32, uint32_t checksum, uint32_t ffx_crc32)
{
  wchar_t wszTex [MAX_PATH + 2] = { };
  
  wcscpy ( wszTex,
            SK_EvalEnvironmentVars (SK_D3D11_res_root.c_str ()).c_str () );

  static std::wstring  null_ref;
         std::wstring& hash_name = null_ref;

  static bool ffx = GetModuleHandle (L"unx.dll") != nullptr;

  if (SK_D3D11_inject_textures_ffx && (! (hash_name = SK_D3D11_TexHashToName (ffx_crc32, 0x00)).empty ()))
  {
    SK_LOG4 ( ( L"Caching texture with crc32: %x", ffx_crc32 ),
                L" Tex Hash " );

    PathAppendW (wszTex, hash_name.c_str ());
  }

  else if (! ( (hash_name = SK_D3D11_TexHashToName (top_crc32, checksum)).empty () &&
               (hash_name = SK_D3D11_TexHashToName (top_crc32, 0x00)    ).empty () ) )
  {
    SK_LOG4 ( ( L"Caching texture with crc32c: %x  (%s) [%s]", top_crc32, hash_name.c_str (), wszTex ),
                L" Tex Hash " );

    PathAppendW (wszTex, hash_name.c_str ());
  }

  else if ( SK_D3D11_inject_textures )
  {
    if ( /*config.textures.d3d11.precise_hash &&*/
         SK_D3D11_IsInjectable (top_crc32, checksum) )
    {
      _swprintf ( wszTex,
                    LR"(%s\inject\textures\%08X_%08X.dds)",
                      wszTex,
                        top_crc32, checksum );
    }

    else if ( SK_D3D11_IsInjectable (top_crc32, 0x00) )
    {
      SK_LOG4 ( ( L"Caching texture with crc32c: %08X", top_crc32 ),
                  L" Tex Hash " );
      _swprintf ( wszTex,
                    LR"(%s\inject\textures\%08X.dds)",
                      wszTex,
                        top_crc32 );
    }

    else if ( ffx &&
              SK_D3D11_IsInjectable_FFX (ffx_crc32) )
    {
      SK_LOG4 ( ( L"Caching texture with crc32: %08X", ffx_crc32 ),
                  L" Tex Hash " );
      _swprintf ( wszTex,
                    LR"(%s\inject\textures\Unx_Old\%08X.dds)",
                      wszTex,
                        ffx_crc32 );
    }

    else *wszTex = L'\0';
  }

  // Not a hashed texture, not an injectable texture, skip it...
  else *wszTex = L'\0';

  SK_FixSlashesW (wszTex);

  return wszTex;
}

//__forceinline
HRESULT
WINAPI
D3D11Dev_CreateTexture2D_Impl (
  _In_              ID3D11Device            *This,
  _Inout_opt_       D3D11_TEXTURE2D_DESC    *pDesc,
  _In_opt_    const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_         ID3D11Texture2D        **ppTexture2D,
                    LPVOID                   lpCallerAddr,
                    SK_TLS                  *pTLS = SK_TLS_Bottom ())
{
  BOOL  bIgnoreThisUpload = FALSE;
  if (! bIgnoreThisUpload) bIgnoreThisUpload = SK_D3D11_IsTexInjectThread (pTLS);
  if (! bIgnoreThisUpload) bIgnoreThisUpload = (! (SK_D3D11_cache_textures || SK_D3D11_dump_textures || SK_D3D11_inject_textures));
  if (  bIgnoreThisUpload)
  {
    return
      D3D11Dev_CreateTexture2D_Original ( This,            pDesc,
                                            pInitialData, ppTexture2D );
  }


  // ------------
  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  CComPtr   <ID3D11Device>        pTestDev (This);
  CComQIPtr <ID3D11DeviceContext> pDevCtx  (rb.d3d11.immediate_ctx);

  if (pDevCtx != nullptr)
  {
    pDevCtx->GetDevice (&pTestDev);
    if (! pTestDev.IsEqualObject (This))
    {
      This->GetImmediateContext (&pDevCtx);
    }
  }

  else
  {
    return
      D3D11Dev_CreateTexture2D_Original ( This,            pDesc,
                                         pInitialData, ppTexture2D );
  }
  // -----------


  if (SK_GetCurrentGameID () == SK_GAME_ID::Yakuza0)
  {
    if (pDesc != nullptr && pDesc->Usage != D3D11_USAGE_IMMUTABLE && ( (pDesc->BindFlags & D3D11_BIND_RENDER_TARGET) ||
                                                                       (pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL) ) )
    {
      //dll_log.Log (L"Texture: (%lu x %lu) { Usage: %s, Format: %s }", pDesc->Width, pDesc->Height, SK_D3D11_DescribeUsage (pDesc->Usage), SK_DXGI_FormatToStr (pDesc->Format).c_str ());
      //if (__SK_Y0_1024_512 && pDesc->Width == 1024 && pDesc->Height == 512)
      //{
      ////
      //  pDesc->Width  = static_cast <UINT> (3840.0 / 1.5);
      //  pDesc->Height = static_cast <UINT> (2160.0 / 1.5);
      //}
      //
      ////if (__SK_Y0_1024_768 && pDesc->Width == 1024 && pDesc->Height == 768)
      ////{
      //////dll_log.Log (L"Texture: 1024x768 { Usage: %s, Format: %s )", SK_D3D11_DescribeUsage (pDesc->Usage), SK_DXGI_FormatToStr (pDesc->Format).c_str ());
      ////  pDesc->Width  = static_cast <UINT> (3840.0 / 1.5);
      ////  pDesc->Height = static_cast <UINT> (2160.0 / 1.5);
      ////}
      //
      //if (__SK_Y0_960_540 && pDesc->Width == 960 && pDesc->Height == 540)
      //{
      ////dll_log.Log (L"Texture: 960x540 { Usage: %s, Format: %s )", SK_D3D11_DescribeUsage (pDesc->Usage), SK_DXGI_FormatToStr (pDesc->Format).c_str ());
      //  pDesc->Width  = 3840 / 2;
      //  pDesc->Height = 2160 / 2;
      //}
    }
  }


  SK_D3D11_TextureResampler.processFinished (This, pDevCtx, pTLS);

  SK_D3D11_MemoryThreads.mark ();


  if (pDesc != nullptr)
  {
    DXGI_FORMAT newFormat =
      pDesc->Format;

    if (pInitialData == nullptr || pInitialData->pSysMem == nullptr)
    {
      if (SK_D3D11_OverrideDepthStencil (newFormat))
      {
        pDesc->Format = newFormat;
        pInitialData  = nullptr;
      }
    }
  }


  uint32_t checksum   = 0;
  uint32_t cache_tag  = 0;
  size_t   size       = 0;

  CComPtr <ID3D11Texture2D> pCachedTex = nullptr;

  bool cacheable = ( ppTexture2D             != nullptr &&
                     pInitialData            != nullptr &&
                     pInitialData->pSysMem   != nullptr &&
                     pDesc                   != nullptr &&
                     pDesc->SampleDesc.Count == 1       &&
                      pDesc->MiscFlags       == 0x00    &&
                     //pDesc->MiscFlags         < 0x04    &&
                     //pDesc->MiscFlags        != 0x01    &&
                     pDesc->CPUAccessFlags   == 0x0     &&
                     pDesc->Width             > 0       &&
                     pDesc->Height            > 0       &&
                     pDesc->MipLevels         > 0       &&
                     pDesc->ArraySize        == 1 //||
                   //((pDesc->ArraySize  % 6 == 0) && ( pDesc->MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE ))
                   );

  if ( cacheable && pDesc->MipLevels == 0 &&
                    pDesc->MiscFlags == D3D11_RESOURCE_MISC_GENERATE_MIPS )
  {
    SK_LOG0 ( ( L"Skipping Texture because of Mipmap Autogen" ),
                L" TexCache " );
  }

  bool injectable = false;

  cacheable = cacheable &&
    (! (pDesc->BindFlags & ( D3D11_BIND_DEPTH_STENCIL |
                             D3D11_BIND_RENDER_TARGET   )))  &&
       (pDesc->BindFlags & ( D3D11_BIND_SHADER_RESOURCE |
                             D3D11_BIND_UNORDERED_ACCESS  )) &&
        (pDesc->Usage    <   D3D11_USAGE_DYNAMIC); // Cancel out Staging
                                                   //   They will be handled through a
                                                   //     different codepath.

  if (SK_GetCurrentGameID () == SK_GAME_ID::Yakuza0)
  {
    cacheable = false;

    if (pDesc->Usage != D3D11_USAGE_IMMUTABLE)
      cacheable = false;

    else if (pDesc->MipLevels == 1)
    {
      cacheable = false;
      //if (SK_D3D11_IsFormatCompressed (pDesc->Format))
      //{
      //  //cacheable = false;
      //  if (pDesc->Width  > 128 ||
      //      pDesc->Height > 128   )
      //  //if (pDesc->Width == 1024 && pDesc->Height == 512)
      //  {
      //    cacheable = false;
      //
      //    dll_log.Log ( L"Reject (Compressed, Width || Height > 128) : (%lu x %lu) : MiscFlags: %x, CPUAccessFlags: %x, SampleDesc.Count: %lu, ArraySize: %lu",
      //                 pDesc->Width, pDesc->Height, pDesc->MiscFlags, pDesc->CPUAccessFlags, pDesc->SampleDesc.Count, pDesc->ArraySize );
      //  }
      //}
      //else
      //{
      //  if (pDesc->Width < 32 || pDesc->Height < 32)
      //  {
      //    dll_log.Log ( L"Reject (Uncompressed, Width || Height < 32) : (%lu x %lu) : MiscFlags: %x, CPUAccessFlags: %x, SampleDesc.Count: %lu, ArraySize: %lu",
      //                 pDesc->Width, pDesc->Height, pDesc->MiscFlags, pDesc->CPUAccessFlags, pDesc->SampleDesc.Count, pDesc->ArraySize );
      //
      //    cacheable = false;
      //  }
      //}
    }

    //dll_log.Log ( L"MipLevels: %lu, MiscFlags: %x, CPUAccessFlags: %x, SampleDesc.Count: %lu, ArraySize: %lu",
    //                pDesc->MipLevels, pDesc->MiscFlags, pDesc->CPUAccessFlags, pDesc->SampleDesc.Count, pDesc->ArraySize );
  }

  CComQIPtr <ID3D11Device> pDev (SK_GetCurrentRenderBackend ().device);

  //
  // Filter out any noise coming from overlays / video capture software
  //
  ///if (SK_GetFramesDrawn () > 0)
  ///  cacheable &= ( pDev.IsEqualObject (This) );

  // XXX: Good idea in theory, but in practice these software packages wrap
  //        the D3D11 device in incompatible ways.


  if (cacheable)
  {
    //dll_log.Log (L"Misc Flags: %x, Bind: %x", pDesc->MiscFlags, pDesc->BindFlags);
  }

  bool gen_mips = false;

  auto CalcMipmapLODs = [](UINT width, UINT height)
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

  if ( config.textures.d3d11.generate_mips && cacheable && pDesc != nullptr && ( pDesc->MipLevels != CalcMipmapLODs (pDesc->Width, pDesc->Height) ) )
  {
    gen_mips = true;
  }


  bool dynamic = false;

  if (config.textures.d3d11.cache && (! cacheable) && pDesc != nullptr)
  {
    SK_LOG1 ( ( L"Impossible to cache texture (Code Origin: '%s') -- Misc Flags: %x, MipLevels: %lu, "
                L"ArraySize: %lu, CPUAccess: %x, BindFlags: %x, Usage: %x, pInitialData: %08"
                PRIxPTR L" (%08" PRIxPTR L")",
                  SK_GetModuleName (SK_GetCallingDLL (lpCallerAddr)).c_str (), pDesc->MiscFlags, pDesc->MipLevels, pDesc->ArraySize,
                    pDesc->CPUAccessFlags, pDesc->BindFlags, pDesc->Usage, (uintptr_t)pInitialData,
                      pInitialData ? (uintptr_t)pInitialData->pSysMem : (uintptr_t)nullptr
              ),
              L"DX11TexMgr" );

    dynamic = true;
  }

  const bool dumpable = 
              cacheable;

  cacheable =
    cacheable && (! (pDesc->CPUAccessFlags & D3D11_CPU_ACCESS_WRITE));


  uint32_t top_crc32 = 0x00;
  uint32_t ffx_crc32 = 0x00;

  if (cacheable)
  {
    checksum =
      crc32_tex (pDesc, pInitialData, &size, &top_crc32);

    if (SK_D3D11_inject_textures_ffx) 
    {
      ffx_crc32 =
        crc32_ffx (pDesc, pInitialData, &size);
    }

    injectable = (
      checksum != 0x00 &&
       ( SK_D3D11_IsInjectable     (top_crc32, checksum) ||
         SK_D3D11_IsInjectable     (top_crc32, 0x00)     ||
         SK_D3D11_IsInjectable_FFX (ffx_crc32)
       )
    );

    if ( checksum != 0x00 &&
         ( SK_D3D11_cache_textures ||
           injectable
         )
       )
    {
      bool compressed =
        SK_D3D11_IsFormatCompressed (pDesc->Format);

      // If this isn't an injectable texture, then filter out non-mipmapped
      //   textures.
      if ((! injectable) && cache_opts.ignore_non_mipped)
        cacheable &= (pDesc->MipLevels > 1 || compressed);

      if (cacheable)
      {
        cache_tag  =
          safe_crc32c (top_crc32, (uint8_t *)pDesc, sizeof D3D11_TEXTURE2D_DESC);

        pCachedTex =
          SK_D3D11_Textures.getTexture2D ( cache_tag, pDesc, nullptr, nullptr,
                                           pTLS );
      }
    }

    else
    {
      cacheable = false;
    }
  }

  if (pCachedTex != nullptr)
  {
    //dll_log.Log ( L"[DX11TexMgr] >> Redundant 2D Texture Load "
                  //L" (Hash=0x%08X [%05.03f MiB]) <<",
                  //checksum, (float)size / (1024.0f * 1024.0f) );

    (*ppTexture2D = pCachedTex)->AddRef ();

    return S_OK;
  }

  // The concept of a cache-miss only applies if the texture had data at the time
  //   of creation...
  if ( cacheable )
  {
    bool
    WINAPI
    SK_XInput_PulseController ( INT   iJoyID,
                                float fStrengthLeft,
                                float fStrengthRight );

    if (config.textures.cache.vibrate_on_miss)
      SK_XInput_PulseController (0, 1.0f, 0.0f);

    SK_D3D11_Textures.CacheMisses_2D++;
  }


  LARGE_INTEGER load_start =
    SK_QueryPerf ();

  if (cacheable)
  {
    if (SK_D3D11_res_root.length ())
    {
      wchar_t   wszTex [MAX_PATH * 2] = { };
      wcsncpy ( wszTex,
                  SK_D3D11_TexNameFromChecksum (top_crc32, checksum, ffx_crc32).c_str (),
                    MAX_PATH );

      if (                   *wszTex  != L'\0' &&
           GetFileAttributes (wszTex) != INVALID_FILE_ATTRIBUTES )
      {
#define D3DX11_DEFAULT static_cast <UINT> (-1)

        SK_ScopedBool auto_tex_inject (&pTLS->texture_management.injection_thread);
        SK_ScopedBool auto_draw       (&pTLS->imgui.drawing);

        pTLS->texture_management.injection_thread = true;
        pTLS->imgui.drawing                       = true;

        HRESULT hr = E_UNEXPECTED;

        // To allow texture reloads, we cannot allow immutable usage on these textures.
        //
        if (pDesc->Usage == D3D11_USAGE_IMMUTABLE)
          pDesc->Usage    = D3D11_USAGE_DEFAULT;

        DirectX::TexMetadata mdata;

        if (SUCCEEDED ((hr = DirectX::GetMetadataFromDDSFile (wszTex, 0, mdata))))
        {
          DirectX::ScratchImage img;

          if (SUCCEEDED ((hr = DirectX::LoadFromDDSFile (wszTex, 0, &mdata, img))))
          {
            if (SUCCEEDED ((hr = DirectX::CreateTexture (This,
                                      img.GetImages     (),
                                      img.GetImageCount (), mdata, 
                                        reinterpret_cast <ID3D11Resource **> (ppTexture2D))))
               )
            {
              LARGE_INTEGER load_end =
                SK_QueryPerf ();

              D3D11_TEXTURE2D_DESC orig_desc = *pDesc;
              D3D11_TEXTURE2D_DESC new_desc  = {    };

              (*ppTexture2D)->GetDesc (&new_desc);

              pDesc->BindFlags      = new_desc.BindFlags;
              pDesc->CPUAccessFlags = new_desc.CPUAccessFlags;
              pDesc->ArraySize      = new_desc.ArraySize;
              pDesc->Format         = new_desc.Format;
              pDesc->Height         = new_desc.Height;
              pDesc->MipLevels      = new_desc.MipLevels;
              pDesc->MiscFlags      = new_desc.MiscFlags;
              pDesc->Usage          = new_desc.Usage;
              pDesc->Width          = new_desc.Width;

              if (pDesc->Usage == D3D11_USAGE_IMMUTABLE)
              {
                pDesc->Usage    = D3D11_USAGE_DEFAULT;
                orig_desc.Usage = D3D11_USAGE_DEFAULT;
              }

              size =
                SK_D3D11_ComputeTextureSize (pDesc);

              SK_AutoCriticalSection critical (&cache_cs);

              SK_D3D11_Textures.refTexture2D (
                *ppTexture2D,
                  pDesc,
                    cache_tag,
                      size,
                        load_end.QuadPart - load_start.QuadPart,
                          top_crc32,
                            wszTex,
                              &orig_desc,  (HMODULE)lpCallerAddr,
                                pTLS );


              SK_D3D11_Textures.Textures_2D [*ppTexture2D].injected = true;

              return ( ( hr = S_OK ) );
            }

            else
            {
              SK_LOG0 ( (L"*** Texture '%s' failed DirectX::CreateTexture (...) -- (HRESULT=%s), skipping!", 
                          SK_StripUserNameFromPathW (wszTex), _com_error (hr).ErrorMessage () ),
                         L"DX11TexMgr" );
            }
          }

          else
          {
            SK_LOG0 ( (L"*** Texture '%s' failed DirectX::LoadFromDDSFile (...) -- (HRESULT=%s), skipping!", 
                         SK_StripUserNameFromPathW (wszTex), _com_error (hr).ErrorMessage () ),
                       L"DX11TexMgr" );
          }
        }

        else
        {
          SK_LOG0 ( (L"*** Texture '%s' failed DirectX::GetMetadataFromDDSFile (...) -- (HRESULT=%s), skipping!", 
                       SK_StripUserNameFromPathW (wszTex), _com_error (hr).ErrorMessage () ),
                     L"DX11TexMgr" );
        }
      }
    }
  }


  HRESULT              ret       = E_NOT_VALID_STATE;
  D3D11_TEXTURE2D_DESC orig_desc = *pDesc;

  //
  // Texture has one mipmap, but we want a full mipmap chain
  //
  //   Be smart about this, stream the other mipmap LODs in over time
  //     and adjust the min/max LOD levels while the texture is incomplete.
  //
  if (gen_mips && SK_GetCurrentGameID () == SK_GAME_ID::Ys_Eight)
  {
    if (pDesc->MipLevels == 1)
    {
      // Various UI textures that only contribute additional load-time
      //   and no benefits if we were to generate mipmaps for them.
      if ( (pDesc->Width == 2048 && pDesc->Height == 1024)
         ||(pDesc->Width == 2048 && pDesc->Height == 2048)
         ||(pDesc->Width == 2048 && pDesc->Height == 4096)
         ||(pDesc->Width == 4096 && pDesc->Height == 4096))
      {
        gen_mips = false;
      }
    }
  }

  if (gen_mips)
  {
    SK_LOG4 ( ( L"Generating mipmaps for texture with crc32c: %x", top_crc32 ),
                L" Tex Hash " );

    SK_ScopedBool auto_tex_inject (&pTLS->texture_management.injection_thread);
    SK_ScopedBool auto_draw       (&pTLS->imgui.drawing);

    pTLS->texture_management.injection_thread = true;
    pTLS->imgui.drawing                       = true;

    CComPtr <ID3D11Texture2D>       pTempTex  = nullptr;

    // We will return this, but when it is returned, it will be missing mipmaps
    //   until the resample job (scheduled onto a worker thread) finishes.
    //
    //   Minimum latency is 1 frame before the texture is complete.
    //
    CComPtr <ID3D11Texture2D>     pFinalTex = nullptr;

    D3D11_TEXTURE2D_DESC original_desc =
      *pDesc;
       pDesc->MipLevels = CalcMipmapLODs (pDesc->Width, pDesc->Height);

    if (pDesc->Usage == D3D11_USAGE_IMMUTABLE)
      pDesc->Usage    = D3D11_USAGE_DEFAULT;

    DirectX::TexMetadata mdata;

    mdata.width      = pDesc->Width;
    mdata.height     = pDesc->Height;
    mdata.depth      = (pDesc->MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) ? 6 : 1;
    mdata.arraySize  = 1;
    mdata.mipLevels  = 1;
    mdata.miscFlags  = (pDesc->MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) ? 
                                              DirectX::TEX_MISC_TEXTURECUBE : 0;
    mdata.miscFlags2 = 0;
    mdata.format     = pDesc->Format;
    mdata.dimension  = DirectX::TEX_DIMENSION_TEXTURE2D;


    resample_job_s resample = { };
                   resample.time.preprocess = SK_QueryPerf ().QuadPart;

    auto* image = new DirectX::ScratchImage;
          image->Initialize (mdata);

    bool error = false;

    for (size_t slice = 0; slice < mdata.arraySize; ++slice)
    {
      size_t height = mdata.height;

      for (size_t lod = 0; lod < mdata.mipLevels; ++lod)
      {
        const DirectX::Image* img =
          image->GetImage (lod, slice, 0);

        if (! (img && img->pixels))
        {
          error = true;
          break;
        }

        const size_t lines =
          DirectX::ComputeScanlines (mdata.format, height);

        if (! lines)
        {
          error = true;
          break;
        }

        auto sptr =
          static_cast <const uint8_t *>(
            pInitialData [lod].pSysMem
          );

        uint8_t* dptr =
          img->pixels;

        for (size_t h = 0; h < lines; ++h)
        {
          size_t msize =
            std::min <size_t> ( img->rowPitch,
                                  pInitialData [lod].SysMemPitch );

          memcpy_s (dptr, img->rowPitch, sptr, msize);

          sptr += pInitialData [lod].SysMemPitch;
          dptr += img->rowPitch;
        }

        if (height > 1) height >>= 1;
      }

      if (error)
        break;
    }

    const DirectX::Image* orig_img =
      image->GetImage (0, 0, 0);

    bool compressed =
      SK_D3D11_IsFormatCompressed (pDesc->Format);

    if (config.textures.d3d11.uncompressed_mips && compressed)
    {
      auto* decompressed =
        new DirectX::ScratchImage;

      ret =
        DirectX::Decompress (orig_img, 1, image->GetMetadata (), DXGI_FORMAT_UNKNOWN, *decompressed);

      if (SUCCEEDED (ret))
      {
        ret =
          DirectX::CreateTexture ( This,
                                     decompressed->GetImages   (), decompressed->GetImageCount (),
                                     decompressed->GetMetadata (), reinterpret_cast <ID3D11Resource **> (&pTempTex) );
        if (SUCCEEDED (ret))
        {
          pDesc->Format =
            decompressed->GetMetadata ().format;

          delete image;
                 image = decompressed;
        }
      }

      if (FAILED (ret))
        delete decompressed;
    }

    else
    {
      ret =
        D3D11Dev_CreateTexture2D_Original (This, &original_desc, pInitialData, &pTempTex);
    }

    if (SUCCEEDED (ret))
    {
      ret =
        D3D11Dev_CreateTexture2D_Original (This, pDesc, nullptr, &pFinalTex);

      if (SUCCEEDED (ret))
      {
        D3D11_CopySubresourceRegion_Original (
          pDevCtx, pFinalTex,
            0, 0, 0, 0,
              pTempTex, 0, nullptr
        );

        size =
          SK_D3D11_ComputeTextureSize (pDesc);
      }
    }

    if (FAILED (ret))
    {
      SK_LOG0 ( (L"Mipmap Generation Failed [%s]",
                  _com_error (ret).ErrorMessage () ), L"DX11TexMgr");
    }

    else
    {
      resample.time.preprocess = 
        ( SK_QueryPerf ().QuadPart - resample.time.preprocess );

      (*ppTexture2D)   = pFinalTex;
      (*ppTexture2D)->AddRef ();

      resample.crc32c  = top_crc32;
      resample.data    = image;
      resample.texture = pFinalTex;

      if (resample.data->GetMetadata ().IsCubemap ())
        SK_LOG0 ( (L"Neat, a Cubemap!"), L"DirectXTex" );

      SK_D3D11_TextureResampler.postJob (resample);

      // It's the thread pool's problem now, don't free this.
      image = nullptr;
    }

    if (image != nullptr) delete image;
  }


  // Auto-gen or some other process failed, fallback to normal texture upload
  if (FAILED (ret))
  {
    assert (ret == S_OK || ret == E_NOT_VALID_STATE);

    ID3D11Texture2D* pIntermediateTex;

    ret =
      D3D11Dev_CreateTexture2D_Original (This, &orig_desc, pInitialData, &pIntermediateTex);

    if (SUCCEEDED (ret))
      *ppTexture2D =
      new IWrapD3D11Texture2D (This, pIntermediateTex);
  }


  LARGE_INTEGER load_end =
    SK_QueryPerf ();

  if ( SUCCEEDED (ret) &&
          dumpable     &&
      checksum != 0x00 &&
      SK_D3D11_dump_textures )
  {
    if (! SK_D3D11_IsDumped (top_crc32, checksum))
    {
      SK_ScopedBool auto_bool (&pTLS->texture_management.injection_thread);
                                pTLS->texture_management.injection_thread = true;

      SK_D3D11_DumpTexture2D (&orig_desc, pInitialData, top_crc32, checksum);
    }
  }

  cacheable &=
    (SK_D3D11_cache_textures || injectable);

  if ( SUCCEEDED (ret) && cacheable )
  {
    SK_AutoCriticalSection critical (&cache_cs);

    if (! SK_D3D11_Textures.Blacklist_2D [orig_desc.MipLevels].count (checksum))
    {
      SK_D3D11_Textures.refTexture2D (
        *ppTexture2D,
          pDesc,
            cache_tag,
              size,
                load_end.QuadPart - load_start.QuadPart,
                  top_crc32,
                    L"",
                      &orig_desc,  (HMODULE)(intptr_t)lpCallerAddr,
                        pTLS
      );
    }
  }

  return ret;
}

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateTexture2D_Override (
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D )
{
  const D3D11_TEXTURE2D_DESC* pDescOrig = pDesc;

  auto descCopy = *pDescOrig;

  HRESULT hr =
    D3D11Dev_CreateTexture2D_Impl (
      This, &descCopy, pInitialData,
            ppTexture2D, _ReturnAddress ()
    );

  *const_cast <D3D11_TEXTURE2D_DESC *> ( pDesc ) = descCopy;

  //if (pDesc && pDesc->Usage == D3D11_USAGE_STAGING)
  //{
  //  dll_log.Log ( L"Code Origin ('%s') - Staging: %lux%lu - Format: %s, CPU Access: %x, Misc Flags: %x",
  //                  SK_GetCallerName ().c_str (), pDesc->Width, pDesc->Height,
  //                  SK_DXGI_FormatToStr          (pDesc->Format).c_str (),
  //                         pDesc->CPUAccessFlags, pDesc->MiscFlags );
  //}

  return hr;
}


void
__stdcall
SK_D3D11_UpdateRenderStatsEx (const IDXGISwapChain* pSwapChain)
{
  if (! (pSwapChain))
    return;

  CComPtr <ID3D11Device>        pDev    = nullptr;
  CComPtr <ID3D11DeviceContext> pDevCtx = nullptr;

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if (rb.device == nullptr || rb.d3d11.immediate_ctx == nullptr || rb.swapchain == nullptr)
    return;

  if ( SUCCEEDED (rb.device->QueryInterface              <ID3D11Device>        (&pDev))  &&
       SUCCEEDED (rb.d3d11.immediate_ctx->QueryInterface <ID3D11DeviceContext> (&pDevCtx)) )
  {
    SK::DXGI::PipelineStatsD3D11& pipeline_stats =
      SK::DXGI::pipeline_stats_d3d11;

    if (ReadPointerAcquire ((volatile PVOID *)&pipeline_stats.query.async) != nullptr)
    {
      if (ReadAcquire (&pipeline_stats.query.active))
      {
        pDevCtx->End ((ID3D11Asynchronous *)ReadPointerAcquire ((volatile PVOID *)&pipeline_stats.query.async));
        InterlockedExchange (&pipeline_stats.query.active, FALSE);
      }

      else
      {
        const HRESULT hr =
          pDevCtx->GetData ( (ID3D11Asynchronous *)ReadPointerAcquire ((volatile PVOID *)&pipeline_stats.query.async),
                              &pipeline_stats.last_results,
                                sizeof D3D11_QUERY_DATA_PIPELINE_STATISTICS,
                                  0x0 );
        if (hr == S_OK)
        {
          ((ID3D11Asynchronous *)ReadPointerAcquire ((volatile PVOID *)&pipeline_stats.query.async))->Release ();
                         InterlockedExchangePointer ((void **)         &pipeline_stats.query.async, nullptr);
        }
      }
    }

    else
    {
      D3D11_QUERY_DESC query_desc
        {  D3D11_QUERY_PIPELINE_STATISTICS,
           0x00                             };

      ID3D11Query* pQuery = nullptr;

      if (SUCCEEDED (pDev->CreateQuery (&query_desc, &pQuery)))
      {
        InterlockedExchangePointer ((void **)&pipeline_stats.query.async,  pQuery);
        pDevCtx->Begin             (                                       pQuery);
        InterlockedExchange        (         &pipeline_stats.query.active, TRUE);
      }
    }
  }
}

void
__stdcall
SK_D3D11_UpdateRenderStats (IDXGISwapChain* pSwapChain)
{
  if (! (pSwapChain && ( config.render.show ||
                          ( SK_ImGui_Widgets.d3d11_pipeline != nullptr &&
                          ( SK_ImGui_Widgets.d3d11_pipeline->isActive () ) ) ) ) )
    return;

  SK_D3D11_UpdateRenderStatsEx (pSwapChain);
}

std::wstring
SK_CountToString (uint64_t count)
{
  wchar_t str [64] = { };

  unsigned int unit = 0;

  if      (count > 1000000000UL) unit = 1000000000UL;
  else if (count > 1000000)      unit = 1000000UL;
  else if (count > 1000)         unit = 1000UL;
  else                           unit = 1UL;

  switch (unit)
  {
    case 1000000000UL:
      _swprintf (str, L"%6.2f Billion ", (float)count / (float)unit);
      break;
    case 1000000UL:
      _swprintf (str, L"%6.2f Million ", (float)count / (float)unit);
      break;
    case 1000UL:
      _swprintf (str, L"%6.2f Thousand", (float)count / (float)unit);
      break;
    case 1UL:
    default:
      _swprintf (str, L"%15llu", count);
      break;
  }

  return str;
}

void
SK_D3D11_SetPipelineStats (void* pData)
{
  memcpy ( &SK::DXGI::pipeline_stats_d3d11.last_results,
             pData,
               sizeof D3D11_QUERY_DATA_PIPELINE_STATISTICS );
}

void
SK_D3D11_GetVertexPipelineDesc (wchar_t* wszDesc)
{
  D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
     SK::DXGI::pipeline_stats_d3d11.last_results;

  if (stats.VSInvocations > 0)
  {
    _swprintf ( wszDesc,
                 L"  VERTEX : %s   (%s Verts ==> %s Triangles)",
                   SK_CountToString (stats.VSInvocations).c_str (),
                     SK_CountToString (stats.IAVertices).c_str (),
                       SK_CountToString (stats.IAPrimitives).c_str () );
  }

  else
  {
    _swprintf ( wszDesc,
                 L"  VERTEX : <Unused>" );
  }
}

void
SK_D3D11_GetGeometryPipelineDesc (wchar_t* wszDesc)
{
  D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
     SK::DXGI::pipeline_stats_d3d11.last_results;

  if (stats.GSInvocations > 0)
  {
    _swprintf ( wszDesc,
                 L"%s  GEOM   : %s   (%s Prims)",
                   wszDesc,
                     SK_CountToString (stats.GSInvocations).c_str (),
                       SK_CountToString (stats.GSPrimitives).c_str () );
  }

  else
  {
    _swprintf ( wszDesc,
                 L"%s  GEOM   : <Unused>",
                   wszDesc );
  }
}

void
SK_D3D11_GetTessellationPipelineDesc (wchar_t* wszDesc)
{
  D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
     SK::DXGI::pipeline_stats_d3d11.last_results;

  if (stats.HSInvocations > 0 || stats.DSInvocations > 0)
  {
    _swprintf ( wszDesc,
                 L"%s  TESS   : %s Hull ==> %s Domain",
                   wszDesc,
                     SK_CountToString (stats.HSInvocations).c_str (),
                       SK_CountToString (stats.DSInvocations).c_str () ) ;
  }

  else
  {
    _swprintf ( wszDesc,
                 L"%s  TESS   : <Unused>",
                   wszDesc );
  }
}

void
SK_D3D11_GetRasterPipelineDesc (wchar_t* wszDesc)
{
  D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
     SK::DXGI::pipeline_stats_d3d11.last_results;

  if (stats.CInvocations > 0)
  {
    _swprintf ( wszDesc,
                 L"%s  RASTER : %5.1f%% Filled     (%s Triangles IN )",
                   wszDesc, 100.0f *
                       ( static_cast <float> (stats.CPrimitives) /
                         static_cast <float> (stats.CInvocations) ),
                     SK_CountToString (stats.CInvocations).c_str () );
  }

  else
  {
    _swprintf ( wszDesc,
                 L"%s  RASTER : <Unused>",
                   wszDesc );
  }
}

void
SK_D3D11_GetPixelPipelineDesc (wchar_t* wszDesc)
{
  D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
     SK::DXGI::pipeline_stats_d3d11.last_results;

  if (stats.PSInvocations > 0)
  {
    _swprintf ( wszDesc,
                 L"%s  PIXEL  : %s   (%s Triangles OUT)",
                   wszDesc,
                     SK_CountToString (stats.PSInvocations).c_str (),
                       SK_CountToString (stats.CPrimitives).c_str () );
  }

  else
  {
    _swprintf ( wszDesc,
                 L"%s  PIXEL  : <Unused>",
                   wszDesc );
  }
}

void
SK_D3D11_GetComputePipelineDesc (wchar_t* wszDesc)
{
  D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
     SK::DXGI::pipeline_stats_d3d11.last_results;

  if (stats.CSInvocations > 0)
  {
    _swprintf ( wszDesc,
                 L"%s  COMPUTE: %s",
                   wszDesc, SK_CountToString (stats.CSInvocations).c_str () );
  }

  else 
  {
    _swprintf ( wszDesc,
                 L"%s  COMPUTE: <Unused>",
                   wszDesc );
  }
}

std::wstring
SK::DXGI::getPipelineStatsDesc (void)
{
  wchar_t wszDesc [1024] = { };

  //
  // VERTEX SHADING
  //
  SK_D3D11_GetVertexPipelineDesc (wszDesc);       lstrcatW (wszDesc, L"\n");

  //
  // GEOMETRY SHADING
  //
  SK_D3D11_GetGeometryPipelineDesc (wszDesc);     lstrcatW (wszDesc, L"\n");

  //
  // TESSELLATION
  //
  SK_D3D11_GetTessellationPipelineDesc (wszDesc); lstrcatW (wszDesc, L"\n");

  //
  // RASTERIZATION
  //
  SK_D3D11_GetRasterPipelineDesc (wszDesc);       lstrcatW (wszDesc, L"\n");

  //
  // PIXEL SHADING
  //
  SK_D3D11_GetPixelPipelineDesc (wszDesc);        lstrcatW (wszDesc, L"\n");

  //
  // COMPUTE
  //
  SK_D3D11_GetComputePipelineDesc (wszDesc);      lstrcatW (wszDesc, L"\n");

  return wszDesc;
}


#include <SpecialK/resource.h>

auto SK_UnpackD3DX11 =
[](void) -> void
{
#ifndef _SK_WITHOUT_D3DX11
  HMODULE hModSelf = 
    SK_GetDLL ();

  HRSRC res =
    FindResource ( hModSelf, MAKEINTRESOURCE (IDR_D3DX11_PACKAGE), L"7ZIP" );

  if (res)
  {
    SK_LOG0 ( ( L"Unpacking D3DX11_43.dll because user does not have June 2010 DirectX Redistributables installed." ),
                L"D3DCompile" );

    DWORD   res_size     =
      SizeofResource ( hModSelf, res );

    HGLOBAL packed_d3dx11 =
      LoadResource   ( hModSelf, res );

    if (! packed_d3dx11) return;


    const void* const locked =
      (void *)LockResource (packed_d3dx11);


    if (locked != nullptr)
    {
      wchar_t      wszArchive     [MAX_PATH * 2 + 1] = { };
      wchar_t      wszDestination [MAX_PATH * 2 + 1] = { };

      wcscpy (wszDestination, SK_GetHostPath ());

      if (GetFileAttributesW (wszDestination) == INVALID_FILE_ATTRIBUTES)
        SK_CreateDirectories (wszDestination);

      wcscpy      (wszArchive, wszDestination);
      PathAppendW (wszArchive, L"D3DX11_43.7z");

      FILE* fPackedD3DX11 =
        _wfopen   (wszArchive, L"wb");

      fwrite      (locked, 1, res_size, fPackedD3DX11);
      fclose      (fPackedD3DX11);

      using SK_7Z_DECOMP_PROGRESS_PFN = int (__stdcall *)(int current, int total);

      extern
      HRESULT
      SK_Decompress7zEx ( const wchar_t*            wszArchive,
                          const wchar_t*            wszDestination,
                          SK_7Z_DECOMP_PROGRESS_PFN callback );

      SK_Decompress7zEx (wszArchive, wszDestination, nullptr);
      DeleteFileW       (wszArchive);
    }

    UnlockResource (packed_d3dx11);
  }
#endif
};

void
SK_D3D11_InitTextures (void)
{
  static volatile LONG SK_D3D11_tex_init = FALSE;

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (! InterlockedCompareExchange (&SK_D3D11_tex_init, TRUE, FALSE))
  {
    pTLS->d3d11.ctx_init_thread = true;

    if (SK_GetCurrentGameID () == SK_GAME_ID::FinalFantasyX_X2)
      SK_D3D11_inject_textures_ffx = true;

    InitializeCriticalSectionAndSpinCount (&preload_cs, 0x01);
    InitializeCriticalSectionAndSpinCount (&dump_cs,    0x02);
    InitializeCriticalSectionAndSpinCount (&inject_cs,  0x10);
    InitializeCriticalSectionAndSpinCount (&hash_cs,    0x40);
    InitializeCriticalSectionAndSpinCount (&cache_cs,   0x80);
    InitializeCriticalSectionAndSpinCount (&tex_cs,     0xFF);

    cache_opts.max_entries       = config.textures.cache.max_entries;
    cache_opts.min_entries       = config.textures.cache.min_entries;
    cache_opts.max_evict         = config.textures.cache.max_evict;
    cache_opts.min_evict         = config.textures.cache.min_evict;
    cache_opts.max_size          = config.textures.cache.max_size;
    cache_opts.min_size          = config.textures.cache.min_size;
    cache_opts.ignore_non_mipped = config.textures.cache.ignore_nonmipped;

    //
    // Legacy Hack for Untitled Project X (FFX/FFX-2)
    //
    extern bool SK_D3D11_inject_textures_ffx;
    if       (! SK_D3D11_inject_textures_ffx)
    {
      SK_D3D11_EnableTexCache  (config.textures.d3d11.cache);
      SK_D3D11_EnableTexDump   (config.textures.d3d11.dump);
      SK_D3D11_EnableTexInject (config.textures.d3d11.inject);
      SK_D3D11_SetResourceRoot (config.textures.d3d11.res_root.c_str ());
    }

    SK_GetCommandProcessor ()->AddVariable ("TexCache.Enable",
         new SK_IVarStub <bool> ((bool *)&config.textures.d3d11.cache));
    SK_GetCommandProcessor ()->AddVariable ("TexCache.MaxEntries",
         new SK_IVarStub <int> ((int *)&cache_opts.max_entries));
    SK_GetCommandProcessor ()->AddVariable ("TexC ache.MinEntries",
         new SK_IVarStub <int> ((int *)&cache_opts.min_entries));
    SK_GetCommandProcessor ()->AddVariable ("TexCache.MaxSize",
         new SK_IVarStub <int> ((int *)&cache_opts.max_size));
    SK_GetCommandProcessor ()->AddVariable ("TexCache.MinSize",
         new SK_IVarStub <int> ((int *)&cache_opts.min_size));
    SK_GetCommandProcessor ()->AddVariable ("TexCache.MinEvict",
         new SK_IVarStub <int> ((int *)&cache_opts.min_evict));
    SK_GetCommandProcessor ()->AddVariable ("TexCache.MaxEvict",
         new SK_IVarStub <int> ((int *)&cache_opts.max_evict));
    SK_GetCommandProcessor ()->AddVariable ("TexCache.IgnoreNonMipped",
         new SK_IVarStub <bool> ((bool *)&cache_opts.ignore_non_mipped));

    if ((! SK_D3D11_inject_textures_ffx) && config.textures.d3d11.inject)
      SK_D3D11_PopulateResourceList ();


#ifndef _SK_WITHOUT_D3DX11
    if (hModD3DX11_43 == nullptr)
    {
      hModD3DX11_43 =
        SK_Modules.LoadLibrary (L"d3dx11_43.dll");

      if (hModD3DX11_43 == nullptr)
      {
        SK_UnpackD3DX11 ();

        hModD3DX11_43 =
          SK_Modules.LoadLibrary (L"d3dx11_43.dll");

        if (hModD3DX11_43 == nullptr)
            hModD3DX11_43 = (HMODULE)1;
      }
    }

    if ((uintptr_t)hModD3DX11_43 > 1)
    {
      if (D3DX11CreateTextureFromFileW == nullptr)
      {
        D3DX11CreateTextureFromFileW =
          (D3DX11CreateTextureFromFileW_pfn)
            GetProcAddress (hModD3DX11_43, "D3DX11CreateTextureFromFileW");
      }

      if (D3DX11GetImageInfoFromFileW == nullptr)
      {
        D3DX11GetImageInfoFromFileW =
          (D3DX11GetImageInfoFromFileW_pfn)
            GetProcAddress (hModD3DX11_43, "D3DX11GetImageInfoFromFileW");
      }

      if (D3DX11CreateTextureFromMemory == nullptr)
      {
        D3DX11CreateTextureFromMemory =
          (D3DX11CreateTextureFromMemory_pfn)
            GetProcAddress (hModD3DX11_43, "D3DX11CreateTextureFromMemory");
      }
    }
#endif


    if (SK_GetCurrentGameID () == SK_GAME_ID::Okami)
    {
      extern void SK_Okami_LoadConfig (void);
                  SK_Okami_LoadConfig ();
    }

    InterlockedIncrement (&SK_D3D11_tex_init);
  }

  if (pTLS->d3d11.ctx_init_thread)
    return;

  SK_Thread_SpinUntilAtomicMin (&SK_D3D11_tex_init, 2);
}


volatile LONG SK_D3D11_initialized = FALSE;

#define D3D11_STUB(_Return, _Name, _Proto, _Args)                           \
  extern "C"                                                                \
  __declspec (dllexport)                                                    \
  _Return STDMETHODCALLTYPE                                                 \
  _Name _Proto {                                                            \
    WaitForInit ();                                                         \
                                                                            \
    typedef _Return (STDMETHODCALLTYPE *passthrough_pfn) _Proto;            \
    static passthrough_pfn _default_impl = nullptr;                         \
                                                                            \
    if (_default_impl == nullptr) {                                         \
      static const char* szName = #_Name;                                   \
      _default_impl = (passthrough_pfn)GetProcAddress (backend_dll, szName);\
                                                                            \
      if (_default_impl == nullptr) {                                       \
        dll_log.Log (                                                       \
          L"Unable to locate symbol  %s in d3d11.dll",                      \
          L#_Name);                                                         \
        return (_Return)E_NOTIMPL;                                          \
      }                                                                     \
    }                                                                       \
                                                                            \
    SK_LOG0 ( (L"[!] %s %s - "                                              \
               L"[Calling Thread: 0x%04x]",                                 \
      L#_Name, L#_Proto, GetCurrentThreadId ()),                            \
                 __SK_SUBSYSTEM__ );                                        \
                                                                            \
    return _default_impl _Args;                                             \
}

#define D3D11_STUB_(_Name, _Proto, _Args)                                   \
  extern "C"                                                                \
  __declspec (dllexport)                                                    \
  void STDMETHODCALLTYPE                                                    \
  _Name _Proto {                                                            \
    WaitForInit ();                                                         \
                                                                            \
    typedef void (STDMETHODCALLTYPE *passthrough_pfn) _Proto;               \
    static passthrough_pfn _default_impl = nullptr;                         \
                                                                            \
    if (_default_impl == nullptr) {                                         \
      static const char* szName = #_Name;                                   \
      _default_impl = (passthrough_pfn)GetProcAddress (backend_dll, szName);\
                                                                            \
      if (_default_impl == nullptr) {                                       \
        dll_log.Log (                                                       \
          L"Unable to locate symbol  %s in d3d11.dll",                      \
          L#_Name );                                                        \
        return;                                                             \
      }                                                                     \
    }                                                                       \
                                                                            \
    SK_LOG0 ( (L"[!] %s %s - "                                              \
               L"[Calling Thread: 0x%04x]",                                 \
      L#_Name, L#_Proto, GetCurrentThreadId ()),                            \
                 __SK_SUBSYSTEM__ );                                        \
                                                                            \
    _default_impl _Args;                                                    \
}

bool
SK_D3D11_Init (void)
{
  BOOL success = FALSE;

  if (! InterlockedCompareExchange (&SK_D3D11_initialized, TRUE, FALSE))
  {
    HMODULE hBackend = 
      ( (SK_GetDLLRole () & DLL_ROLE::D3D11) ) ? backend_dll :
                                                  SK_Modules.LoadLibraryLL (L"d3d11.dll");

    SK::DXGI::hModD3D11 = hBackend;

    D3D11CreateDeviceForD3D12              = GetProcAddress (SK::DXGI::hModD3D11, "D3D11CreateDeviceForD3D12");
    CreateDirect3D11DeviceFromDXGIDevice   = GetProcAddress (SK::DXGI::hModD3D11, "CreateDirect3D11DeviceFromDXGIDevice");
    CreateDirect3D11SurfaceFromDXGISurface = GetProcAddress (SK::DXGI::hModD3D11, "CreateDirect3D11SurfaceFromDXGISurface");
    D3D11On12CreateDevice                  = GetProcAddress (SK::DXGI::hModD3D11, "D3D11On12CreateDevice");
    D3DKMTCloseAdapter                     = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTCloseAdapter");
    D3DKMTDestroyAllocation                = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTDestroyAllocation");
    D3DKMTDestroyContext                   = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTDestroyContext");
    D3DKMTDestroyDevice                    = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTDestroyDevice ");
    D3DKMTDestroySynchronizationObject     = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTDestroySynchronizationObject");
    D3DKMTQueryAdapterInfo                 = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTQueryAdapterInfo");
    D3DKMTSetDisplayPrivateDriverFormat    = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTSetDisplayPrivateDriverFormat");
    D3DKMTSignalSynchronizationObject      = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTSignalSynchronizationObject");
    D3DKMTUnlock                           = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTUnlock");
    D3DKMTWaitForSynchronizationObject     = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTWaitForSynchronizationObject");
    EnableFeatureLevelUpgrade              = GetProcAddress (SK::DXGI::hModD3D11, "EnableFeatureLevelUpgrade");
    OpenAdapter10                          = GetProcAddress (SK::DXGI::hModD3D11, "OpenAdapter10");
    OpenAdapter10_2                        = GetProcAddress (SK::DXGI::hModD3D11, "OpenAdapter10_2");
    D3D11CoreCreateLayeredDevice           = GetProcAddress (SK::DXGI::hModD3D11, "D3D11CoreCreateLayeredDevice");
    D3D11CoreGetLayeredDeviceSize          = GetProcAddress (SK::DXGI::hModD3D11, "D3D11CoreGetLayeredDeviceSize");
    D3D11CoreRegisterLayers                = GetProcAddress (SK::DXGI::hModD3D11, "D3D11CoreRegisterLayers");
    D3DKMTCreateAllocation                 = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTCreateAllocation");
    D3DKMTCreateContext                    = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTCreateContext");
    D3DKMTCreateDevice                     = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTCreateDevice");
    D3DKMTCreateSynchronizationObject      = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTCreateSynchronizationObject");
    D3DKMTEscape                           = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTEscape");
    D3DKMTGetContextSchedulingPriority     = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTGetContextSchedulingPriority");
    D3DKMTGetDeviceState                   = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTGetDeviceState");
    D3DKMTGetDisplayModeList               = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTGetDisplayModeList");
    D3DKMTGetMultisampleMethodList         = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTGetMultisampleMethodList");
    D3DKMTGetRuntimeData                   = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTGetRuntimeData");
    D3DKMTGetSharedPrimaryHandle           = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTGetSharedPrimaryHandle");
    D3DKMTLock                             = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTLock");
    D3DKMTOpenAdapterFromHdc               = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTOpenAdapterFromHdc");
    D3DKMTOpenResource                     = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTOpenResource");
    D3DKMTPresent                          = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTPresent");
    D3DKMTQueryAllocationResidency         = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTQueryAllocationResidency");
    D3DKMTQueryResourceInfo                = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTQueryResourceInfo");
    D3DKMTRender                           = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTRender");
    D3DKMTSetAllocationPriority            = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTSetAllocationPriority");
    D3DKMTSetContextSchedulingPriority     = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTSetContextSchedulingPriority");
    D3DKMTSetDisplayMode                   = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTSetDisplayMode");
    D3DKMTSetGammaRamp                     = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTSetGammaRamp");
    D3DKMTSetVidPnSourceOwner              = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTSetVidPnSourceOwner");
    D3DKMTWaitForVerticalBlankEvent        = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTWaitForVerticalBlankEvent");
    D3DPerformance_BeginEvent              = GetProcAddress (SK::DXGI::hModD3D11, "D3DPerformance_BeginEvent");
    D3DPerformance_EndEvent                = GetProcAddress (SK::DXGI::hModD3D11, "D3DPerformance_EndEvent");
    D3DPerformance_GetStatus               = GetProcAddress (SK::DXGI::hModD3D11, "D3DPerformance_GetStatus");
    D3DPerformance_SetMarker               = GetProcAddress (SK::DXGI::hModD3D11, "D3DPerformance_SetMarker");


    SK_LOG0 ( (L"Importing D3D11CreateDevice[AndSwapChain]"), __SK_SUBSYSTEM__ );
    SK_LOG0 ( (L"========================================="), __SK_SUBSYSTEM__ );

    if (! _wcsicmp (SK_GetModuleName (SK_GetDLL ()).c_str (), L"d3d11.dll"))
    {
      if (! LocalHook_D3D11CreateDevice.active)
      {
        D3D11CreateDevice_Import            =  \
         (D3D11CreateDevice_pfn)               \
           GetProcAddress (hBackend, "D3D11CreateDevice");
      }

      if (! LocalHook_D3D11CreateDeviceAndSwapChain.active)
      {
        D3D11CreateDeviceAndSwapChain_Import            =  \
         (D3D11CreateDeviceAndSwapChain_pfn)               \
           GetProcAddress (hBackend, "D3D11CreateDeviceAndSwapChain");
      }

      SK_LOG0 ( ( L"  D3D11CreateDevice:             %s",
                    SK_MakePrettyAddress (D3D11CreateDevice_Import).c_str () ),
                  __SK_SUBSYSTEM__ );
      SK_LogSymbolName                    (D3D11CreateDevice_Import);

      SK_LOG0 ( ( L"  D3D11CreateDeviceAndSwapChain: %s",
                    SK_MakePrettyAddress (D3D11CreateDeviceAndSwapChain_Import).c_str () ),
                  __SK_SUBSYSTEM__ );
      SK_LogSymbolName                   (D3D11CreateDeviceAndSwapChain_Import);

      pfnD3D11CreateDeviceAndSwapChain = D3D11CreateDeviceAndSwapChain_Import;
      pfnD3D11CreateDevice             = D3D11CreateDevice_Import;

      InterlockedIncrement (&SK_D3D11_initialized);
    }

    else
    {
      if ( LocalHook_D3D11CreateDevice.active ||
          ( MH_OK ==
             SK_CreateDLLHook2 (      L"d3d11.dll",
                                       "D3D11CreateDevice",
                                        D3D11CreateDevice_Detour,
               static_cast_p2p <void> (&D3D11CreateDevice_Import),
                                    &pfnD3D11CreateDevice )
          )
         )
      {
              SK_LOG0 ( ( L"  D3D11CreateDevice:              %s  %s",
        SK_MakePrettyAddress (pfnD3D11CreateDevice ? pfnD3D11CreateDevice :
                                                        D3D11CreateDevice_Import).c_str (),
                              pfnD3D11CreateDevice ? L"{ Hooked }" :
                                                     L"{ Cached }" ),
                        __SK_SUBSYSTEM__ );
      }

      if ( LocalHook_D3D11CreateDeviceAndSwapChain.active ||
          ( MH_OK ==
             SK_CreateDLLHook2 (    L"d3d11.dll",
                                     "D3D11CreateDeviceAndSwapChain",
                                      D3D11CreateDeviceAndSwapChain_Detour,
             static_cast_p2p <void> (&D3D11CreateDeviceAndSwapChain_Import),
                                  &pfnD3D11CreateDeviceAndSwapChain )
          )
         )
      {
            SK_LOG0 ( ( L"  D3D11CreateDeviceAndSwapChain:  %s  %s",
        SK_MakePrettyAddress (pfnD3D11CreateDevice ? pfnD3D11CreateDeviceAndSwapChain :
                                                        D3D11CreateDeviceAndSwapChain_Import).c_str (),
                            pfnD3D11CreateDeviceAndSwapChain ? L"{ Hooked }" :
                                                               L"{ Cached }" ),
                        __SK_SUBSYSTEM__ );
        SK_LogSymbolName     (pfnD3D11CreateDeviceAndSwapChain);

        if ((SK_GetDLLRole () & DLL_ROLE::D3D11) || (SK_GetDLLRole () & DLL_ROLE::DInput8))
        {
          SK_RunLHIfBitness ( 64, SK_LoadPlugIns64 (),
                                  SK_LoadPlugIns32 () );
        }

        if ( ( LocalHook_D3D11CreateDevice.active ||
               MH_OK == MH_QueueEnableHook (pfnD3D11CreateDevice) ) &&
             ( LocalHook_D3D11CreateDeviceAndSwapChain.active ||
               MH_OK == MH_QueueEnableHook (pfnD3D11CreateDeviceAndSwapChain) ) )
        {
          InterlockedIncrement (&SK_D3D11_initialized);
          success = TRUE;//(MH_OK == SK_ApplyQueuedHooks ());
        }
      }

      if (! success)
      {
        SK_LOG0 ( (L"Something went wrong hooking D3D11 -- need better errors."), __SK_SUBSYSTEM__ );
      }
    }

    LocalHook_D3D11CreateDeviceAndSwapChain.target.addr =
      pfnD3D11CreateDeviceAndSwapChain ? pfnD3D11CreateDeviceAndSwapChain :
                                            D3D11CreateDeviceAndSwapChain_Import;
    LocalHook_D3D11CreateDeviceAndSwapChain.active = true;

    LocalHook_D3D11CreateDevice.target.addr             =
      pfnD3D11CreateDevice            ? pfnD3D11CreateDevice :
                                           D3D11CreateDevice_Import;
    LocalHook_D3D11CreateDevice.active = true;
  }

  SK_Thread_SpinUntilAtomicMin (&SK_D3D11_initialized, 2);

  return success;
}

void
SK_D3D11_Shutdown (void)
{
  if (! InterlockedCompareExchange (&SK_D3D11_initialized, FALSE, TRUE))
    return;

  if (SK_D3D11_Textures.RedundantLoads_2D > 0)
  {
    SK_LOG0 ( (L"At shutdown: %7.2f seconds and %7.2f MiB of"
                  L" CPU->GPU I/O avoided by %lu texture cache hits.",
                    SK_D3D11_Textures.RedundantTime_2D / 1000.0f,
                      (float)SK_D3D11_Textures.RedundantData_2D.load () /
                                 (1024.0f * 1024.0f),
                             SK_D3D11_Textures.RedundantLoads_2D.load ()),
               L"Perf Stats" );
  }

  SK_D3D11_Textures.reset ();

  // Stop caching while we shutdown
  SK_D3D11_cache_textures = false;

  if (FreeLibrary_Original (SK::DXGI::hModD3D11))
  {
    //DeleteCriticalSection (&tex_cs);
    //DeleteCriticalSection (&hash_cs);
    //DeleteCriticalSection (&dump_cs);
    //DeleteCriticalSection (&cache_cs);
    //DeleteCriticalSection (&inject_cs);
    //DeleteCriticalSection (&preload_cs);
  }
}

void
SK_D3D11_EnableHooks (void)
{
  InterlockedExchange (&__d3d11_ready, TRUE);
}


void
SK_D3D11_HookDevCtx (sk_hook_d3d11_t *pHooks)
{
  static bool hooked = false;
  
  if (hooked)
    return;

  hooked = true;
#if 0
    DXGI_VIRTUAL_OVERRIDE ( pHooks->ppImmediateContext, 7, "ID3D11DeviceContext::VSSetConstantBuffers",
                             D3D11_VSSetConstantBuffers_Override, D3D11_VSSetConstantBuffers_Original,
                             D3D11_VSSetConstantBuffers_pfn);
#else
    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,    7,
                          "ID3D11DeviceContext::VSSetConstantBuffers",
                                          D3D11_VSSetConstantBuffers_Override,
                                          D3D11_VSSetConstantBuffers_Original,
                                          D3D11_VSSetConstantBuffers_pfn );
#endif

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,    8,
                          "ID3D11DeviceContext::PSSetShaderResources",
                                          D3D11_PSSetShaderResources_Override,
                                          D3D11_PSSetShaderResources_Original,
                                          D3D11_PSSetShaderResources_pfn );

#if 0
    DXGI_VIRTUAL_OVERRIDE (pHooks->ppImmediateContext, 9, "ID3D11DeviceContext::PSSetShader",
                             D3D11_PSSetShader_Override, D3D11_PSSetShader_Original,
                             D3D11_PSSetShader_pfn);
#else
    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,    9,
                          "ID3D11DeviceContext::PSSetShader",
                                          D3D11_PSSetShader_Override,
                                          D3D11_PSSetShader_Original,
                                          D3D11_PSSetShader_pfn );
#endif

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   10,
                          "ID3D11DeviceContext::PSSetSamplers",
                                          D3D11_PSSetSamplers_Override,
                                          D3D11_PSSetSamplers_Original,
                                          D3D11_PSSetSamplers_pfn );

#if 0
    DXGI_VIRTUAL_OVERRIDE ( pHooks->ppImmediateContext, 11, "ID3D11DeviceContext::VSSetShader",
                             D3D11_VSSetShader_Override, D3D11_VSSetShader_Original,
                             D3D11_VSSetShader_pfn);
#else
    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   11,
                          "ID3D11DeviceContext::VSSetShader",
                                          D3D11_VSSetShader_Override,
                                          D3D11_VSSetShader_Original,
                                          D3D11_VSSetShader_pfn );
#endif

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   12,
                          "ID3D11DeviceContext::DrawIndexed",
                                          D3D11_DrawIndexed_Override,
                                          D3D11_DrawIndexed_Original,
                                          D3D11_DrawIndexed_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   13,
                          "ID3D11DeviceContext::Draw",
                                          D3D11_Draw_Override,
                                          D3D11_Draw_Original,
                                          D3D11_Draw_pfn );

    //
    // Third-party software frequently causes these hooks to become corrupted, try installing a new
    //   vFtable pointer instead of hooking the function.
    //
#if 0
    DXGI_VIRTUAL_OVERRIDE ( pHooks->ppImmediateContext, 14, "ID3D11DeviceContext::Map",
                             D3D11_Map_Override, D3D11_Map_Original,
                             D3D11_Map_pfn);
#else
    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   14,
                          "ID3D11DeviceContext::Map",
                                          D3D11Dev_Map_Override,
                                             D3D11_Map_Original,
                                          D3D11Dev_Map_pfn );

      DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   15,
                            "ID3D11DeviceContext::Unmap",
                                            D3D11_Unmap_Override,
                                            D3D11_Unmap_Original,
                                            D3D11_Unmap_pfn );
#endif

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   16,
                          "ID3D11DeviceContext::PSSetConstantBuffers",
                                          D3D11_PSSetConstantBuffers_Override,
                                          D3D11_PSSetConstantBuffers_Original,
                                          D3D11_PSSetConstantBuffers_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   20,
                          "ID3D11DeviceContext::DrawIndexedInstanced",
                                          D3D11_DrawIndexedInstanced_Override,
                                          D3D11_DrawIndexedInstanced_Original,
                                          D3D11_DrawIndexedInstanced_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   21,
                          "ID3D11DeviceContext::DrawInstanced",
                                          D3D11_DrawInstanced_Override,
                                          D3D11_DrawInstanced_Original,
                                          D3D11_DrawInstanced_pfn);

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   23,
                          "ID3D11DeviceContext::GSSetShader",
                                          D3D11_GSSetShader_Override,
                                          D3D11_GSSetShader_Original,
                                          D3D11_GSSetShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   25,
                          "ID3D11DeviceContext::VSSetShaderResources",
                                          D3D11_VSSetShaderResources_Override,
                                          D3D11_VSSetShaderResources_Original,
                                          D3D11_VSSetShaderResources_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   31,
                          "ID3D11DeviceContext::GSSetShaderResources",
                                          D3D11_GSSetShaderResources_Override,
                                          D3D11_GSSetShaderResources_Original,
                                          D3D11_GSSetShaderResources_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   33,
                          "ID3D11DeviceContext::OMSetRenderTargets",
                                          D3D11_OMSetRenderTargets_Override,
                                          D3D11_OMSetRenderTargets_Original,
                                          D3D11_OMSetRenderTargets_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   34,
                          "ID3D11DeviceContext::OMSetRenderTargetsAndUnorderedAccessViews",
                                          D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Override,
                                          D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Original,
                                          D3D11_OMSetRenderTargetsAndUnorderedAccessViews_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   38,
                          "ID3D11DeviceContext::DrawAuto",
                                          D3D11_DrawAuto_Override,
                                          D3D11_DrawAuto_Original,
                                          D3D11_DrawAuto_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   39,
                          "ID3D11DeviceContext::DrawIndexedInstancedIndirect",
                                          D3D11_DrawIndexedInstancedIndirect_Override,
                                          D3D11_DrawIndexedInstancedIndirect_Original,
                                          D3D11_DrawIndexedInstancedIndirect_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   40,
                          "ID3D11DeviceContext::DrawInstancedIndirect",
                                          D3D11_DrawInstancedIndirect_Override,
                                          D3D11_DrawInstancedIndirect_Original,
                                          D3D11_DrawInstancedIndirect_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   41,
                          "ID3D11DeviceContext::Dispatch",
                                          D3D11_Dispatch_Override,
                                          D3D11_Dispatch_Original,
                                          D3D11_Dispatch_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   42,
                          "ID3D11DeviceContext::DispatchIndirect",
                                          D3D11_DispatchIndirect_Override,
                                          D3D11_DispatchIndirect_Original,
                                          D3D11_DispatchIndirect_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   44,
                          "ID3D11DeviceContext::RSSetViewports",
                                          D3D11_RSSetViewports_Override,
                                          D3D11_RSSetViewports_Original,
                                          D3D11_RSSetViewports_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   45,
                          "ID3D11DeviceContext::RSSetScissorRects",
                                          D3D11_RSSetScissorRects_Override,
                                          D3D11_RSSetScissorRects_Original,
                                          D3D11_RSSetScissorRects_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   46,
                          "ID3D11DeviceContext::CopySubresourceRegion",
                                          D3D11_CopySubresourceRegion_Override,
                                          D3D11_CopySubresourceRegion_Original,
                                          D3D11_CopySubresourceRegion_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   47,
                          "ID3D11DeviceContext::CopyResource",
                                          D3D11_CopyResource_Override,
                                          D3D11_CopyResource_Original,
                                          D3D11_CopyResource_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   48,
                          "ID3D11DeviceContext::UpdateSubresource",
                                          D3D11_UpdateSubresource_Override,
                                          D3D11_UpdateSubresource_Original,
                                          D3D11_UpdateSubresource_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   53,
                          "ID3D11DeviceContext::ClearDepthStencilView",
                                          D3D11_ClearDepthStencilView_Override,
                                          D3D11_ClearDepthStencilView_Original,
                                          D3D11_ClearDepthStencilView_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   59,
                          "ID3D11DeviceContext::HSSetShaderResources",
                                          D3D11_HSSetShaderResources_Override, 
                                          D3D11_HSSetShaderResources_Original,
                                          D3D11_HSSetShaderResources_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   60,
                          "ID3D11DeviceContext::HSSetShader",
                                          D3D11_HSSetShader_Override,
                                          D3D11_HSSetShader_Original,
                                          D3D11_HSSetShader_pfn);

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   63,
                          "ID3D11DeviceContext::DSSetShaderResources",
                                          D3D11_DSSetShaderResources_Override,
                                          D3D11_DSSetShaderResources_Original,
                                          D3D11_DSSetShaderResources_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   64,
                          "ID3D11DeviceContext::DSSetShader",
                                          D3D11_DSSetShader_Override,
                                          D3D11_DSSetShader_Original,
                                          D3D11_DSSetShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   67,
                          "ID3D11DeviceContext::CSSetShaderResources",
                                          D3D11_CSSetShaderResources_Override,
                                          D3D11_CSSetShaderResources_Original,
                                          D3D11_CSSetShaderResources_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   68,
                          "ID3D11DeviceContext::CSSetUnorderedAccessViews",
                                          D3D11_CSSetUnorderedAccessViews_Override,
                                          D3D11_CSSetUnorderedAccessViews_Original,
                                          D3D11_CSSetUnorderedAccessViews_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   69,
                          "ID3D11DeviceContext::CSSetShader",
                                          D3D11_CSSetShader_Override,
                                          D3D11_CSSetShader_Original,
                                          D3D11_CSSetShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   74,
                          "ID3D11DeviceContext::PSGetShader",
                                          D3D11_PSGetShader_Override,
                                          D3D11_PSGetShader_Original,
                                          D3D11_PSGetShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   76,
                          "ID3D11DeviceContext::VSGetShader",
                                          D3D11_VSGetShader_Override,
                                          D3D11_VSGetShader_Original,
                                          D3D11_VSGetShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   82,
                          "ID3D11DeviceContext::GSGetShader",
                                          D3D11_GSGetShader_Override,
                                          D3D11_GSGetShader_Original,
                                          D3D11_GSGetShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   89,
                          "ID3D11DeviceContext::OMGetRenderTargets",
                                          D3D11_OMGetRenderTargets_Override,
                                          D3D11_OMGetRenderTargets_Original,
                                          D3D11_OMGetRenderTargets_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   90,
                          "ID3D11DeviceContext::OMGetRenderTargetsAndUnorderedAccessViews",
                                          D3D11_OMGetRenderTargetsAndUnorderedAccessViews_Override,
                                          D3D11_OMGetRenderTargetsAndUnorderedAccessViews_Original,
                                          D3D11_OMGetRenderTargetsAndUnorderedAccessViews_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   98,
                           "ID3D11DeviceContext::HSGetShader",
                                           D3D11_HSGetShader_Override,
                                           D3D11_HSGetShader_Original,
                                           D3D11_HSGetShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,  102,
                          "ID3D11DeviceContext::DSGetShader",
                                          D3D11_DSGetShader_Override,
                                          D3D11_DSGetShader_Original,
                                          D3D11_DSGetShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,  107,
                          "ID3D11DeviceContext::CSGetShader",
                                          D3D11_CSGetShader_Override,
                                          D3D11_CSGetShader_Original,
                                          D3D11_CSGetShader_pfn );
}


#include <d3d11.h>
#include <../src/render/d3d11/d3d11_dev_ctx.cpp>

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateDeferredContext_Override (
  _In_            ID3D11Device         *This,
  _In_            UINT                  ContextFlags,
  _Out_opt_       ID3D11DeviceContext **ppDeferredContext )
{
  DXGI_LOG_CALL_1 (L"ID3D11Device::CreateDeferredContext", L"ContextFlags=0x%x", ContextFlags );

  if (ppDeferredContext != nullptr)
  {
    static Concurrency::concurrent_unordered_map
      <        ID3D11DeviceContext *,
        SK_IWrapD3D11DeviceContext * > wrapped_contexts;

    ID3D11DeviceContext* pTemp = nullptr;
    HRESULT              hr    =
      D3D11Dev_CreateDeferredContext_Original (This, ContextFlags, &pTemp);

    if (SUCCEEDED (hr))
    {
      if (wrapped_contexts.count (pTemp) == 0)
        wrapped_contexts [pTemp] = new SK_IWrapD3D11DeviceContext (pTemp);
      
      *ppDeferredContext =
        dynamic_cast <ID3D11DeviceContext *> (wrapped_contexts [pTemp]);


      //sk_hook_d3d11_t       hook_ctx { nullptr, &pTemp };
      //SK_D3D11_HookDevCtx (&hook_ctx);

      return hr;
    }

    *ppDeferredContext = pTemp;

    return hr;
  }

  else
    return D3D11Dev_CreateDeferredContext_Original (This, ContextFlags, nullptr);
}

#include <concurrent_unordered_map.h>

_declspec (noinline)
void
STDMETHODCALLTYPE
D3D11Dev_GetImmediateContext_Override ( 
  _In_            ID3D11Device         *This,
  _Out_           ID3D11DeviceContext **ppImmediateContext )
{
  if (config.system.log_level > 1)
  {
    DXGI_LOG_CALL_0 (L"ID3D11Device::GetImmediateContext");
  }

  static Concurrency::concurrent_unordered_map
    <        ID3D11DeviceContext *,
      SK_IWrapD3D11DeviceContext * > wrapped_contexts;

  ID3D11DeviceContext* pCtx = nullptr;

  D3D11Dev_GetImmediateContext_Original (This, &pCtx);

  //sk_hook_d3d11_t       hook_ctx { nullptr, &pCtx };
  //SK_D3D11_HookDevCtx (&hook_ctx);

  if (wrapped_contexts.count (pCtx) == 0)
    wrapped_contexts [pCtx] = new SK_IWrapD3D11DeviceContext (pCtx);
  
  *ppImmediateContext =
    dynamic_cast <ID3D11DeviceContext *> (wrapped_contexts [pCtx]);
//*ppImmediateContext = pCtx;
}


extern
unsigned int __stdcall HookD3D12                   (LPVOID user);

DWORD
__stdcall
HookD3D11 (LPVOID user)
{
  if (! config.apis.dxgi.d3d11.hook)
    return 0;

  // Wait for DXGI to boot
  if (CreateDXGIFactory_Import == nullptr)
  {
    static volatile ULONG implicit_init = FALSE;

    // If something called a D3D11 function before DXGI was initialized,
    //   begin the process, but ... only do this once.
    if (! InterlockedCompareExchange (&implicit_init, TRUE, FALSE))
    {
      SK_LOG0 ( (L" >> Implicit Initialization Triggered <<"), __SK_SUBSYSTEM__ );
      SK_BootDXGI ();
    }

    while (CreateDXGIFactory_Import == nullptr)
      MsgWaitForMultipleObjectsEx (0, nullptr, 16UL, QS_ALLINPUT, MWMO_INPUTAVAILABLE);

    // TODO: Handle situation where CreateDXGIFactory is unloadable
  }

  static volatile LONG __d3d11_hooked = FALSE;

  // This only needs to be done once
  if (! InterlockedCompareExchange (&__d3d11_hooked, TRUE, FALSE))
  {
  SK_LOG0 ( (L"  Hooking D3D11"), __SK_SUBSYSTEM__ );

  auto* pHooks = 
    static_cast <sk_hook_d3d11_t *> (user);

  //  3 CreateBuffer
  //  4 CreateTexture1D
  //  5 CreateTexture2D
  //  6 CreateTexture3D
  //  7 CreateShaderResourceView
  //  8 CreateUnorderedAccessView
  //  9 CreateRenderTargetView
  // 10 CreateDepthStencilView
  // 11 CreateInputLayout
  // 12 CreateVertexShader
  // 13 CreateGeometryShader
  // 14 CreateGeometryShaderWithStreamOutput
  // 15 CreatePixelShader
  // 16 CreateHullShader
  // 17 CreateDomainShader
  // 18 CreateComputeShader
  // 19 CreateClassLinkage
  // 20 CreateBlendState
  // 21 CreateDepthStencilState
  // 22 CreateRasterizerState
  // 23 CreateSamplerState
  // 24 CreateQuery
  // 25 CreatePredicate
  // 26 CreateCounter
  // 27 CreateDeferredContext
  // 28 OpenSharedResource
  // 29 CheckFormatSupport
  // 30 CheckMultisampleQualityLevels
  // 31 CheckCounterInfo
  // 32 CheckCounter
  // 33 CheckFeatureSupport
  // 34 GetPrivateData
  // 35 SetPrivateData
  // 36 SetPrivateDataInterface
  // 37 GetFeatureLevel
  // 38 GetCreationFlags
  // 39 GetDeviceRemovedReason
  // 40 GetImmediateContext
  // 41 SetExceptionMode
  // 42 GetExceptionMode

  if (pHooks->ppDevice && pHooks->ppImmediateContext)
  {
    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,    3,
                          "ID3D11Device::CreateBuffer",
                                D3D11Dev_CreateBuffer_Override,
                                D3D11Dev_CreateBuffer_Original,
                                D3D11Dev_CreateBuffer_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,    5,
                          "ID3D11Device::CreateTexture2D",
                                D3D11Dev_CreateTexture2D_Override,
                                D3D11Dev_CreateTexture2D_Original,
                                D3D11Dev_CreateTexture2D_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,    7,
                          "ID3D11Device::CreateShaderResourceView",
                                D3D11Dev_CreateShaderResourceView_Override,
                                D3D11Dev_CreateShaderResourceView_Original,
                                D3D11Dev_CreateShaderResourceView_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,    8,
                          "ID3D11Device::CreateUnorderedAccessView",
                                D3D11Dev_CreateUnorderedAccessView_Override,
                                D3D11Dev_CreateUnorderedAccessView_Original,
                                D3D11Dev_CreateUnorderedAccessView_pfn );
    
    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,    9,
                          "ID3D11Device::CreateRenderTargetView",
                                D3D11Dev_CreateRenderTargetView_Override,
                                D3D11Dev_CreateRenderTargetView_Original,
                                D3D11Dev_CreateRenderTargetView_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,   10,
                          "ID3D11Device::CreateDepthStencilView",
                                D3D11Dev_CreateDepthStencilView_Override,
                                D3D11Dev_CreateDepthStencilView_Original,
                                D3D11Dev_CreateDepthStencilView_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,   12,
                          "ID3D11Device::CreateVertexShader",
                                D3D11Dev_CreateVertexShader_Override,
                                D3D11Dev_CreateVertexShader_Original,
                                D3D11Dev_CreateVertexShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,   13,
                          "ID3D11Device::CreateGeometryShader",
                                D3D11Dev_CreateGeometryShader_Override,
                                D3D11Dev_CreateGeometryShader_Original,
                                D3D11Dev_CreateGeometryShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,   14,
                          "ID3D11Device::CreateGeometryShaderWithStreamOutput",
                                D3D11Dev_CreateGeometryShaderWithStreamOutput_Override,
                                D3D11Dev_CreateGeometryShaderWithStreamOutput_Original,
                                D3D11Dev_CreateGeometryShaderWithStreamOutput_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,   15,
                          "ID3D11Device::CreatePixelShader",
                                D3D11Dev_CreatePixelShader_Override,
                                D3D11Dev_CreatePixelShader_Original,
                                D3D11Dev_CreatePixelShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,   16,
                          "ID3D11Device::CreateHullShader",
                                D3D11Dev_CreateHullShader_Override,
                                D3D11Dev_CreateHullShader_Original,
                                D3D11Dev_CreateHullShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,   17,
                          "ID3D11Device::CreateDomainShader",
                                D3D11Dev_CreateDomainShader_Override,
                                D3D11Dev_CreateDomainShader_Original,
                                D3D11Dev_CreateDomainShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,   18,
                          "ID3D11Device::CreateComputeShader",
                                D3D11Dev_CreateComputeShader_Override,
                                D3D11Dev_CreateComputeShader_Original,
                                D3D11Dev_CreateComputeShader_pfn );

    //DXGI_VIRTUAL_HOOK (pHooks->ppDevice, 19, "ID3D11Device::CreateClassLinkage",
    //                       D3D11Dev_CreateClassLinkage_Override, D3D11Dev_CreateClassLinkage_Original,
    //                       D3D11Dev_CreateClassLinkage_pfn);

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,    23,
                          "ID3D11Device::CreateSamplerState",
                                D3D11Dev_CreateSamplerState_Override,
                                D3D11Dev_CreateSamplerState_Original,
                                D3D11Dev_CreateSamplerState_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,     27,
                          "ID3D11Device::CreateDeferredContext",
                                D3D11Dev_CreateDeferredContext_Override,
                                D3D11Dev_CreateDeferredContext_Original,
                                D3D11Dev_CreateDeferredContext_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,     40,
                          "ID3D11Device::GetImmediateContext",
                                D3D11Dev_GetImmediateContext_Override,
                                D3D11Dev_GetImmediateContext_Original,
                                D3D11Dev_GetImmediateContext_pfn );

#if 1
    SK_D3D11_HookDevCtx (pHooks);

    //CComQIPtr <ID3D11DeviceContext1> pDevCtx1 (*pHooks->ppImmediateContext);
    //
    //if (pDevCtx1 != nullptr)
    //{
    //  DXGI_VIRTUAL_HOOK ( &pDevCtx1,  116,
    //                        "ID3D11DeviceContext1::UpdateSubresource1",
    //                                         D3D11_UpdateSubresource1_Override,
    //                                         D3D11_UpdateSubresource1_Original,
    //                                         D3D11_UpdateSubresource1_pfn );
    //}
#endif
  }

  InterlockedIncrement (&__d3d11_hooked);
  }

  SK_Thread_SpinUntilAtomicMin (&__d3d11_hooked, 2);

  if (config.apis.dxgi.d3d12.hook)
    HookD3D12 (nullptr);

  return 0;
}


bool convert_typeless = false;

auto IsWireframe = [&](sk_shader_class shader_class, uint32_t crc32c)
{
  d3d11_shader_tracking_s* tracker   = nullptr;
  bool                     wireframe = false;

  switch (shader_class)
  {
    case sk_shader_class::Vertex:
    {
      tracker   = &SK_D3D11_Shaders.vertex.tracked;
      wireframe =  SK_D3D11_Shaders.vertex.wireframe.count (crc32c) != 0;
    } break;

    case sk_shader_class::Pixel:
    {
      tracker   = &SK_D3D11_Shaders.pixel.tracked;
      wireframe =  SK_D3D11_Shaders.pixel.wireframe.count (crc32c) != 0;
    } break;

    case sk_shader_class::Geometry:
    {
      tracker   = &SK_D3D11_Shaders.geometry.tracked;
      wireframe =  SK_D3D11_Shaders.geometry.wireframe.count (crc32c) != 0;
    } break;

    case sk_shader_class::Hull:
    {
      tracker   = &SK_D3D11_Shaders.hull.tracked;
      wireframe =  SK_D3D11_Shaders.hull.wireframe.count (crc32c) != 0;
    } break;

    case sk_shader_class::Domain:
    {
      tracker   = &SK_D3D11_Shaders.domain.tracked;
      wireframe =  SK_D3D11_Shaders.domain.wireframe.count (crc32c) != 0;
    } break;

    default:
    case sk_shader_class::Compute:
    {
      return false;
    } break;
  }

  if (tracker->crc32c == crc32c && tracker->wireframe)
    return true;

  return wireframe;
};

auto IsOnTop = [&](sk_shader_class shader_class, uint32_t crc32c)
{
  d3d11_shader_tracking_s* tracker = nullptr;
  bool                     on_top  = false;

  switch (shader_class)
  {
    case sk_shader_class::Vertex:
    {
      tracker = &SK_D3D11_Shaders.vertex.tracked;
      on_top  =  SK_D3D11_Shaders.vertex.on_top.count (crc32c) != 0;
    } break;

    case sk_shader_class::Pixel:
    {
      tracker = &SK_D3D11_Shaders.pixel.tracked;
      on_top  =  SK_D3D11_Shaders.pixel.on_top.count (crc32c) != 0;
    } break;

    case sk_shader_class::Geometry:
    {
      tracker = &SK_D3D11_Shaders.geometry.tracked;
      on_top  =  SK_D3D11_Shaders.geometry.on_top.count (crc32c) != 0;
    } break;

    case sk_shader_class::Hull:
    {
      tracker = &SK_D3D11_Shaders.hull.tracked;
      on_top  =  SK_D3D11_Shaders.hull.on_top.count (crc32c) != 0;
    } break;

    case sk_shader_class::Domain:
    {
      tracker = &SK_D3D11_Shaders.domain.tracked;
      on_top  =  SK_D3D11_Shaders.domain.on_top.count (crc32c) != 0;
    } break;

    default:
    case sk_shader_class::Compute:
    {
      return false;
    } break;
  }

  if (tracker->crc32c == crc32c && tracker->on_top)
    return true;

  return on_top;
};

auto IsSkipped = [&](sk_shader_class shader_class, uint32_t crc32c)
{
  d3d11_shader_tracking_s* tracker     = nullptr;
  bool                     blacklisted = false;

  switch (shader_class)
  {
    case sk_shader_class::Vertex:
    {
      tracker     = &SK_D3D11_Shaders.vertex.tracked;
      blacklisted =  SK_D3D11_Shaders.vertex.blacklist.count (crc32c) != 0;
    } break;

    case sk_shader_class::Pixel:
    {
      tracker     = &SK_D3D11_Shaders.pixel.tracked;
      blacklisted =  SK_D3D11_Shaders.pixel.blacklist.count (crc32c) != 0;
    } break;

    case sk_shader_class::Geometry:
    {
      tracker     = &SK_D3D11_Shaders.geometry.tracked;
      blacklisted =  SK_D3D11_Shaders.geometry.blacklist.count (crc32c) != 0;
    } break;

    case sk_shader_class::Hull:
    {
      tracker     = &SK_D3D11_Shaders.hull.tracked;
      blacklisted =  SK_D3D11_Shaders.hull.blacklist.count (crc32c) != 0;
    } break;

    case sk_shader_class::Domain:
    {
      tracker     = &SK_D3D11_Shaders.domain.tracked;
      blacklisted =  SK_D3D11_Shaders.domain.blacklist.count (crc32c) != 0;
    } break;

    case sk_shader_class::Compute:
    {
      tracker     = &SK_D3D11_Shaders.compute.tracked;
      blacklisted =  SK_D3D11_Shaders.compute.blacklist.count (crc32c) != 0;
    } break;

    default:
      return false;
  }

  if (tracker->crc32c == crc32c && tracker->cancel_draws)
    return true;

  return blacklisted;
};

struct shader_disasm_s {
  std::string       header = "";
  std::string       code   = "";
  std::string       footer = "";

  struct constant_buffer
  {
    std::string      name  = "";
    UINT             Slot  =  0;

    struct variable
    {
      std::string                name = "";
      D3D11_SHADER_VARIABLE_DESC var_desc;
      D3D11_SHADER_TYPE_DESC     type_desc;
      float                      default_value [16];
    };

    typedef std::pair <std::vector <variable>, std::string> struct_ent;

    std::vector <struct_ent> structs;

    size_t           size  = 0UL;
  };

  std::vector <constant_buffer> cbuffers;
};

using ID3DXBuffer  = interface ID3DXBuffer;
using LPD3DXBUFFER = interface ID3DXBuffer*;

// {8BA5FB08-5195-40e2-AC58-0D989C3A0102}
DEFINE_GUID(IID_ID3DXBuffer, 
0x8ba5fb08, 0x5195, 0x40e2, 0xac, 0x58, 0xd, 0x98, 0x9c, 0x3a, 0x1, 0x2);

#undef INTERFACE
#define INTERFACE ID3DXBuffer

DECLARE_INTERFACE_(ID3DXBuffer, IUnknown)
{
    // IUnknown
    STDMETHOD  (        QueryInterface)   (THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_ (ULONG,  AddRef)           (THIS) PURE;
    STDMETHOD_ (ULONG,  Release)          (THIS) PURE;

    // ID3DXBuffer
    STDMETHOD_ (LPVOID, GetBufferPointer) (THIS) PURE;
    STDMETHOD_ (DWORD,  GetBufferSize)    (THIS) PURE;
};

#include <imgui/imgui.h>
#include <imgui/backends/imgui_d3d11.h>

std::unordered_map <uint32_t, shader_disasm_s> vs_disassembly;
std::unordered_map <uint32_t, shader_disasm_s> ps_disassembly;
std::unordered_map <uint32_t, shader_disasm_s> gs_disassembly;
std::unordered_map <uint32_t, shader_disasm_s> hs_disassembly;
std::unordered_map <uint32_t, shader_disasm_s> ds_disassembly;
std::unordered_map <uint32_t, shader_disasm_s> cs_disassembly;

uint32_t change_sel_vs = 0x00;
uint32_t change_sel_ps = 0x00;
uint32_t change_sel_gs = 0x00;
uint32_t change_sel_hs = 0x00;
uint32_t change_sel_ds = 0x00;
uint32_t change_sel_cs = 0x00;


auto ShaderMenu =
[&] ( std::unordered_set <uint32_t>&                          blacklist,
      SK_D3D11_KnownShaders::conditional_blacklist_t&    cond_blacklist,
      std::vector       <ID3D11ShaderResourceView *>&   used_resources,
const std::set          <ID3D11ShaderResourceView *>& set_of_resources,
      uint32_t                                                   shader )
{
  if (blacklist.count (shader))
  {
    if (ImGui::MenuItem ("Enable Shader"))
    {
      blacklist.erase (shader);
    }
  }

  else
  {
    if (ImGui::MenuItem ("Disable Shader"))
    {
      blacklist.emplace (shader);
    }
  }

  auto DrawSRV = [&](ID3D11ShaderResourceView* pSRV)
  {
    DXGI_FORMAT                     fmt = DXGI_FORMAT_UNKNOWN;
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;

    pSRV->GetDesc (&srv_desc);

    if (srv_desc.ViewDimension == D3D_SRV_DIMENSION_TEXTURE2D)
    {
      CComPtr <ID3D11Resource>  pRes = nullptr;
      CComPtr <ID3D11Texture2D> pTex = nullptr;

      pSRV->GetResource (&pRes);

      if (pRes && SUCCEEDED (pRes->QueryInterface <ID3D11Texture2D> (&pTex)) && pTex)
      {
        D3D11_TEXTURE2D_DESC desc = { };
        pTex->GetDesc      (&desc);
                       fmt = desc.Format;

        if (desc.Height > 0 && desc.Width > 0)
        {
          ImGui::TreePush ("");

          std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_render_view);

          ID3D11ShaderResourceView* pSRV2 = nullptr;

          // Typeless compressed types need to assume a type, or they won't render :P
          switch (srv_desc.Format)
          {
            case DXGI_FORMAT_BC1_TYPELESS:
              srv_desc.Format = DXGI_FORMAT_BC1_UNORM;
              break;
            case DXGI_FORMAT_BC2_TYPELESS:
              srv_desc.Format = DXGI_FORMAT_BC2_UNORM;
              break;
            case DXGI_FORMAT_BC3_TYPELESS:
              srv_desc.Format = DXGI_FORMAT_BC3_UNORM;
              break;
            case DXGI_FORMAT_BC4_TYPELESS:
              srv_desc.Format = DXGI_FORMAT_BC4_UNORM;
              break;
            case DXGI_FORMAT_BC5_TYPELESS:
              srv_desc.Format = DXGI_FORMAT_BC5_UNORM;
              break;
            case DXGI_FORMAT_BC6H_TYPELESS:
              srv_desc.Format = DXGI_FORMAT_BC6H_SF16;
              break;
            case DXGI_FORMAT_BC7_TYPELESS:
              srv_desc.Format = DXGI_FORMAT_BC7_UNORM;
              break;

            case DXGI_FORMAT_B8G8R8A8_TYPELESS:
              srv_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
              break;
            case DXGI_FORMAT_R8G8B8A8_TYPELESS:
              srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;


            case DXGI_FORMAT_R8_TYPELESS:
              srv_desc.Format = DXGI_FORMAT_R8_UNORM;
              break;
            case DXGI_FORMAT_R8G8_TYPELESS:
              srv_desc.Format = DXGI_FORMAT_R8G8_UNORM;
              break;

            case DXGI_FORMAT_R16_TYPELESS:
              srv_desc.Format = DXGI_FORMAT_R16_UNORM;
              break;
            case DXGI_FORMAT_R16G16_TYPELESS:
              srv_desc.Format = DXGI_FORMAT_R16G16_UNORM;
              break;
            case DXGI_FORMAT_R16G16B16A16_TYPELESS:
              srv_desc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
              break;

            case DXGI_FORMAT_R32_TYPELESS:
              srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
              break;
            case DXGI_FORMAT_R32G32_TYPELESS:
              srv_desc.Format = DXGI_FORMAT_R32G32_FLOAT;
              break;
            case DXGI_FORMAT_R32G32B32_TYPELESS:
              srv_desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
              break;
            case DXGI_FORMAT_R32G32B32A32_TYPELESS:
              srv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
              break;
          };

          srv_desc.Texture2D.MipLevels       = (UINT)-1;
          srv_desc.Texture2D.MostDetailedMip =        desc.MipLevels;

          if (convert_typeless && SUCCEEDED (((ID3D11Device *)(SK_GetCurrentRenderBackend ().device.p))->CreateShaderResourceView (pRes, &srv_desc, &pSRV2)))
          {
            temp_resources.emplace_back (pSRV2);

            ImGui::Image ( pSRV2,      ImVec2  ( std::max (64.0f, (float)desc.Width / 16.0f),
      ((float)desc.Height / (float)desc.Width) * std::max (64.0f, (float)desc.Width / 16.0f) ),
                                       ImVec2  (0,0),             ImVec2  (1,1),
                                       ImColor (255,255,255,255), ImColor (242,242,13,255) );
          }

          else
          {
            temp_resources.emplace_back (pSRV);
                                         pSRV->AddRef ();

            ImGui::Image ( pSRV,       ImVec2  ( std::max (64.0f, (float)desc.Width / 16.0f),
      ((float)desc.Height / (float)desc.Width) * std::max (64.0f, (float)desc.Width / 16.0f) ),
                                       ImVec2  (0,0),             ImVec2  (1,1),
                                       ImColor (255,255,255,255), ImColor (242,242,13,255) );
          }
          ImGui::TreePop  ();
        }
      }
    }
  };

  std::set <uint32_t> listed;

  for (auto it : used_resources)
  {
    bool selected = false;

    uint32_t crc32c = 0x0;

    ID3D11Resource*   pRes = nullptr;
    it->GetResource (&pRes);

    D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
             pRes->GetType (&rdim);

    if (rdim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      CComQIPtr <ID3D11Texture2D> pTex2D (pRes);

      if (pTex2D != nullptr)
      {
        if (SK_D3D11_Textures.Textures_2D.count  (pTex2D) &&  SK_D3D11_Textures.Textures_2D [pTex2D].crc32c != 0x0)
        {
          crc32c = SK_D3D11_Textures.Textures_2D [pTex2D].crc32c;
        }
      }
    }

    if (listed.count (crc32c))
      continue;

    if (cond_blacklist [shader].count (crc32c))
    {
      ImGui::BeginGroup ();

      if (crc32c != 0x00)
        ImGui::MenuItem ( SK_FormatString ("Enable Shader for Texture %x", crc32c).c_str (), nullptr, &selected);

      DrawSRV (it);

      ImGui::EndGroup ();
    }

    if (crc32c != 0x0)
      listed.emplace (crc32c);

    if (selected)
    {
      cond_blacklist [shader].erase (crc32c);
    }
  }

  listed.clear ();

  for (auto it : used_resources )
  {
    if (it == nullptr)
      continue;

    bool selected = false;

    uint32_t crc32c = 0x0;

    ID3D11Resource*   pRes = nullptr;
    it->GetResource (&pRes);

    D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    pRes->GetType          (&rdim);

    if (rdim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      CComQIPtr <ID3D11Texture2D> pTex2D (pRes);

      if (pTex2D != nullptr)
      {
        if (SK_D3D11_Textures.Textures_2D.count  (pTex2D) && SK_D3D11_Textures.Textures_2D  [pTex2D].crc32c != 0x0)
        {
          crc32c = SK_D3D11_Textures.Textures_2D [pTex2D].crc32c;
        }
      }
    }

    if (listed.count (crc32c))
      continue;

    if (! cond_blacklist [shader].count (crc32c))
    {
      ImGui::BeginGroup ();

      if (crc32c != 0x00)
      {
        ImGui::MenuItem ( SK_FormatString ("Disable Shader for Texture %x", crc32c).c_str (), nullptr, &selected);

        if (set_of_resources.count (it))
          DrawSRV (it);
      }

      ImGui::EndGroup ();

      if (selected)
      {
        cond_blacklist [shader].emplace (crc32c);
      }
    }

    if (crc32c != 0x0)
      listed.emplace (crc32c);
  }
};

static size_t debug_tex_id = 0x0;
static size_t tex_dbg_idx  = 0;

#include <SpecialK/steam_api.h>
#include <unordered_map>

extern std::unordered_map <BYTE, std::wstring> virtKeyCodeToHumanKeyName;


void
SK_LiveTextureView (bool& can_scroll, SK_TLS* pTLS = SK_TLS_Bottom ())
{
  ImGuiIO& io (ImGui::GetIO ());

  std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_render_view);

  ImGui::PushID ("Texture2D_D3D11");

  const float font_size           = ImGui::GetFont ()->FontSize * io.FontGlobalScale;
  const float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

  static float last_ht    = 256.0f;
  static float last_width = 256.0f;

  struct list_entry_s {
    std::string          name      = "I need an adult!";
    uint32_t             tag       = 0UL;
    uint32_t             crc32c    = 0UL;
    bool                 injected  = false;
    D3D11_TEXTURE2D_DESC desc      = { };
    D3D11_TEXTURE2D_DESC orig_desc = { };
    ID3D11Texture2D*     pTex      = nullptr;
    size_t               size      = 0;
  };                              

  static std::vector <list_entry_s> list_contents;
  static std::map
           <uint32_t, list_entry_s> texture_map;
  static              bool          list_dirty      = true;
  static              bool          lod_list_dirty  = true;
  static              size_t        sel             =    0;
  static              int           tex_set         =    1;
  static              int           lod             =    0;
  static              char          lod_list [1024]   {  };

  extern              size_t        tex_dbg_idx;
  extern              size_t        debug_tex_id;

  static              int           refresh_interval     = 0UL; // > 0 = Periodic Refresh
  static              int           last_frame           = 0UL;
  static              size_t        total_texture_memory = 0ULL;

  
  ImGui::Text      ("Current list represents %5.2f MiB of texture memory", (double)total_texture_memory / (double)(1024 * 1024));
  ImGui::SameLine  ( );

  ImGui::PushItemWidth (ImGui::GetContentRegionAvailWidth () * 0.33f);
  ImGui::SliderInt     ("Frames Between Texture Refreshes", &refresh_interval, 0, 120);
  ImGui::PopItemWidth  ();

  ImGui::BeginChild ( ImGui::GetID ("ToolHeadings"), ImVec2 (font_size * 66.0f, font_size * 2.5f), false,
                        ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NavFlattened );

  if (InterlockedCompareExchange (&live_textures_dirty, FALSE, TRUE))
  {
    texture_map.clear   ();
    list_contents.clear ();

    last_ht             =  0;
    last_width          =  0;
    lod                 =  0;

    list_dirty          = true;
  }

  if (ImGui::Button ("  Refresh Textures  "))
  {
    list_dirty = true;
  }

  if (ImGui::IsItemHovered ())
  {
    if (tex_set == 1) ImGui::SetTooltip ("Refresh the list using textures drawn during the last frame.");
    else              ImGui::SetTooltip ("Refresh the list using ALL cached textures.");
  }

  ImGui::SameLine      ();
  ImGui::PushItemWidth (font_size * strlen ("Used Textures   ") / 2);

  ImGui::Combo ("###TexturesD3D11_TextureSet", &tex_set, "All Textures\0Used Textures\0\0", 2);

  ImGui::PopItemWidth ();
  ImGui::SameLine     ();

  if (ImGui::Button (" Clear Debug "))
  {
    sel                 = std::numeric_limits <size_t>::max ();
    debug_tex_id        =  0;
    list_contents.clear ();
    last_ht             =  0;
    last_width          =  0;
    lod                 =  0;
    tracked_texture     =  nullptr;
  }

  if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("Exits texture debug mode.");

  ImGui::SameLine ();

  if (ImGui::Button ("  Reload All Injected Textures  "))
  {
    SK_D3D11_ReloadAllTextures ();
  }

  ImGui::SameLine ();
  ImGui::Checkbox ("Highlight Selected Texture in Game##D3D11_HighlightSelectedTexture", &config.textures.d3d11.highlight_debug);
  ImGui::SameLine ();

  static bool hide_inactive = true;
  
  ImGui::Checkbox  ("Hide Inactive Textures##D3D11_HideInactiveTextures",                 &hide_inactive);
  ImGui::Separator ();
  ImGui::EndChild  ();


  for (auto& it_ctx : SK_D3D11_PerCtxResources )
  {
    int spins = 0;

    while (InterlockedCompareExchange (&it_ctx.writing_, 1, 0) != 0)
    {
      if ( ++spins > 0x1000 )
      {
        SleepEx (1, FALSE);
        spins = 0;
      }
    }

    for ( auto& it_res : it_ctx.used_textures )
    {
      used_textures.emplace (it_res);
    }

    it_ctx.used_textures.clear ();

    InterlockedExchange (&it_ctx.writing_, 0);
  }


  if (refresh_interval > 0 || list_dirty)
  {
    if ((LONG)SK_GetFramesDrawn () > last_frame + refresh_interval || list_dirty)
    {
      list_dirty           = true;
      last_frame           = SK_GetFramesDrawn ();
      total_texture_memory = 0ULL;
    }
  }

  if (list_dirty)
  {
    list_contents.clear ();

    if (debug_tex_id == 0)
      last_ht = 0;

    {
      // Relatively immutable textures
      for (auto& it : SK_D3D11_Textures.HashMap_2D)
      {
        SK_AutoCriticalSection critical1 (&it.mutex);

        for (auto& it2 : it.entries)
        {
          const auto& tex_ref =
            SK_D3D11_Textures.TexRefs_2D.find (it2.second);

          if ( tex_ref != SK_D3D11_Textures.TexRefs_2D.cend () )
          {
            list_entry_s entry = { };

            entry.crc32c = 0;
            entry.tag    = it2.first;
            entry.name   = "DontCare";
            entry.pTex   = it2.second;

            if (SK_D3D11_TextureIsCached (it2.second))
            {
              const SK_D3D11_TexMgr::tex2D_descriptor_s& desc =
                SK_D3D11_Textures.Textures_2D [it2.second];

              entry.desc      = desc.desc;
              entry.orig_desc = desc.orig_desc;
              entry.size      = desc.mem_size;
              entry.crc32c    = desc.crc32c;
              entry.injected  = desc.injected;
            }

            else
              continue;

            if (entry.size > 0 && entry.crc32c != 0x00)
              texture_map [entry.crc32c] = entry;
          }
        }
      }

      // Self-sorted list, yay :)
      for (auto& it : texture_map)
      {
        bool active =
          used_textures.count (it.second.pTex) != 0;

        if (active || tex_set == 0)
        {
          char szDesc [48] = { };

          _itoa (it.second.crc32c, szDesc, 16);

          list_entry_s entry = { };

          entry.crc32c    = it.second.crc32c;
          entry.tag       = it.second.tag;
          entry.desc      = it.second.desc;
          entry.orig_desc = it.second.orig_desc;
          entry.name      = szDesc;
          entry.pTex      = it.second.pTex;
          entry.size      = it.second.size;

          list_contents.emplace_back (entry);
          total_texture_memory += entry.size;
        }
      }
    }

    list_dirty = false;

    if ((! SK_D3D11_Textures.TexRefs_2D.count (tracked_texture)) ||
           SK_D3D11_Textures.Textures_2D      [tracked_texture].crc32c == 0x0 )
      tracked_texture = nullptr;
  }

  ImGui::BeginGroup ();

  ImGui::PushStyleVar   (ImGuiStyleVar_ChildWindowRounding, 0.0f);
  ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.9f, 0.7f, 0.5f, 1.0f));

  ImGui::BeginChild ( ImGui::GetID ("D3D11_TexHashes_CRC32C"),
                      ImVec2 ( font_size * 6.0f, std::max (font_size * 15.0f, last_ht)),
                        true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

  if (ImGui::IsWindowHovered ())
    can_scroll = false;

  static int draws = 0;

  if (! list_contents.empty ())
  {
    static size_t last_sel     = std::numeric_limits <size_t>::max ();
    static bool   sel_changed  = false;

    // Don't select a texture immediately
    if (sel != last_sel && draws++ != 0)
      sel_changed = true;
    
    last_sel = sel;
    
    for ( size_t line = 0; line < list_contents.size (); line++)
    {
      bool active = used_textures.count (list_contents [line].pTex) != 0;

      if (active)
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.95f, 0.95f, 0.95f, 1.0f));
      else
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.425f, 0.425f, 0.425f, 0.9f));

      if ((! hide_inactive) || active)
      {
        if (line == sel)
        {
          bool selected = true;
          ImGui::Selectable (list_contents [line].name.c_str (), &selected);
    
          if (sel_changed)
          {
            ImGui::SetScrollHere        (0.5f);
            ImGui::SetKeyboardFocusHere (    );

            sel_changed     = false;
            tex_dbg_idx     = line;
            sel             = line;
            debug_tex_id    = list_contents [line].crc32c;
            tracked_texture = list_contents [line].pTex;
            lod             = 0;
            lod_list_dirty  = true;
            *lod_list       = '\0';
          }
        }
    
        else
        {
          bool selected = false;
    
          if (ImGui::Selectable (list_contents [line].name.c_str (), &selected))
          {
            sel_changed     = true;
            tex_dbg_idx     = line;
            sel             = line;
            debug_tex_id    = list_contents [line].crc32c;
            tracked_texture = list_contents [line].pTex;
            lod             = 0;
            lod_list_dirty  = true;
            *lod_list       = '\0';
          }
        }
      }

      ImGui::PopStyleColor ();
    }
  }

  ImGui::EndChild ();

  if (ImGui::IsItemHovered () || ImGui::IsItemFocused ())
  {
    int dir = 0;

    if (ImGui::IsItemFocused ())
    {
      ImGui::BeginTooltip ();
      ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "");
      ImGui::Separator    ();
      ImGui::BulletText   ("Press LB to select the previous texture from this list");
      ImGui::BulletText   ("Press RB to select the next texture from this list");
      ImGui::EndTooltip   ();
    }

    else
    {
      ImGui::BeginTooltip ();
      ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "");
      ImGui::Separator    ();
      ImGui::BulletText   ("Press %ws to select the previous texture from this list", virtKeyCodeToHumanKeyName [VK_OEM_4].c_str ());
      ImGui::BulletText   ("Press %ws to select the next texture from this list",     virtKeyCodeToHumanKeyName [VK_OEM_6].c_str ());
      ImGui::EndTooltip   ();
    }

         if (io.NavInputs [ImGuiNavInput_PadFocusPrev] && io.NavInputsDownDuration [ImGuiNavInput_PadFocusPrev] == 0.0f) { dir = -1; }
    else if (io.NavInputs [ImGuiNavInput_PadFocusNext] && io.NavInputsDownDuration [ImGuiNavInput_PadFocusNext] == 0.0f) { dir =  1; }

    else
    {
           if (io.KeysDown [VK_OEM_4] && io.KeysDownDuration [VK_OEM_4] == 0.0f) { dir = -1;  io.WantCaptureKeyboard = true; }
      else if (io.KeysDown [VK_OEM_6] && io.KeysDownDuration [VK_OEM_6] == 0.0f) { dir =  1;  io.WantCaptureKeyboard = true; }
    }

    if (dir != 0)
    {
      if ((SSIZE_T)sel <  0)                     sel = 0;
      if (         sel >= list_contents.size ()) sel = list_contents.size () - 1;
      if ((SSIZE_T)sel <  0)                     sel = 0;

      while ((SSIZE_T)sel >= 0 && sel < list_contents.size ())
      {
        if ( (dir < 0 && sel == 0                        ) ||
             (dir > 0 && sel == list_contents.size () - 1)    )
        {
          break;
        }

        sel += dir;

        if (hide_inactive)
        {
          bool active = used_textures.count (list_contents [sel].pTex) != 0;

          if (active)
            break;
        }

        else
          break;
      }

      if ((SSIZE_T)sel <  0)                     sel = 0;
      if (         sel >= list_contents.size ()) sel = list_contents.size () - 1;
      if ((SSIZE_T)sel <  0)                     sel = 0;
    }
  }

  ImGui::SameLine     ();
  ImGui::PushStyleVar (ImGuiStyleVar_ChildWindowRounding, 20.0f);

  last_ht    = std::max (last_ht,    16.0f);
  last_width = std::max (last_width, 16.0f);

  if (debug_tex_id != 0x00 && texture_map.count ((uint32_t)debug_tex_id))
  {
    list_entry_s& entry =
      texture_map [(uint32_t)debug_tex_id];

    D3D11_TEXTURE2D_DESC tex_desc  = entry.orig_desc;
    size_t               tex_size  = 0UL;
    float                load_time = 0.0f;

    CComPtr <ID3D11Texture2D> pTex =
      SK_D3D11_Textures.getTexture2D ((uint32_t)entry.tag, &tex_desc, &tex_size, &load_time, pTLS);

    bool staged = false;

    if (pTex != nullptr)
    {
      // Get the REAL format, not the one the engine knows about through texture cache
      pTex->GetDesc (&tex_desc);

      if (lod_list_dirty)
      {
        UINT w = tex_desc.Width;
        UINT h = tex_desc.Height;

        char* pszLODList = lod_list;

        for ( UINT i = 0 ; i < tex_desc.MipLevels ; i++ )
        {
          int len =
            sprintf (pszLODList, "LOD%u: (%ux%u)", i, std::max (1U, w >> i), std::max (1U, h >> i));

          pszLODList += (len + 1);
        }

        *pszLODList = '\0';

        lod_list_dirty = false;
      }


      ID3D11ShaderResourceView*       pSRV     = nullptr;
      D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {     };

      srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
      srv_desc.Format                    = tex_desc.Format;

      // Typeless compressed types need to assume a type, or they won't render :P
      switch (srv_desc.Format)
      {
        case DXGI_FORMAT_BC1_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_BC1_UNORM;
          break;
        case DXGI_FORMAT_BC2_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_BC2_UNORM;
          break;
        case DXGI_FORMAT_BC3_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_BC3_UNORM;
          break;
        case DXGI_FORMAT_BC4_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_BC4_UNORM;
          break;
        case DXGI_FORMAT_BC5_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_BC5_UNORM;
          break;
        case DXGI_FORMAT_BC6H_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_BC6H_SF16;
          break;
        case DXGI_FORMAT_BC7_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_BC7_UNORM;
          break;

        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
          break;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
          break;

        case DXGI_FORMAT_R8_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_R8_UNORM;
          break;
        case DXGI_FORMAT_R8G8_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_R8G8_UNORM;
          break;

        case DXGI_FORMAT_R16_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_R16_UNORM;
          break;
        case DXGI_FORMAT_R16G16_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_R16G16_UNORM;
          break;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
          break;

        case DXGI_FORMAT_R32_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
          break;
        case DXGI_FORMAT_R32G32_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_R32G32_FLOAT;
          break;
        case DXGI_FORMAT_R32G32B32_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
          break;
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
          break;
      };

      srv_desc.Texture2D.MipLevels       = (UINT)-1;
      srv_desc.Texture2D.MostDetailedMip =        tex_desc.MipLevels;

      CComQIPtr <ID3D11Device> pDev (SK_GetCurrentRenderBackend ().device);

      if (pDev != nullptr)
      {
#if 0
        ImVec4 border_color = config.textures.highlight_debug_tex ? 
                                ImVec4 (0.3f, 0.3f, 0.3f, 1.0f) :
                                  (__remap_textures && has_alternate) ? ImVec4 (0.5f,  0.5f,  0.5f, 1.0f) :
                                                                        ImVec4 (0.3f,  1.0f,  0.3f, 1.0f);
#else
        ImVec4 border_color = entry.injected ? ImVec4 (0.3f,  0.3f,  1.0f, 1.0f) :
                                               ImVec4 (0.3f,  1.0f,  0.3f, 1.0f);
#endif

        ImGui::PushStyleColor (ImGuiCol_Border, border_color);

        float scale_factor = 1.0f;

        float content_avail_y = (ImGui::GetWindowContentRegionMax ().y - ImGui::GetWindowContentRegionMin ().y) / scale_factor;
        float content_avail_x = (ImGui::GetWindowContentRegionMax ().x - ImGui::GetWindowContentRegionMin ().x) / scale_factor;
        float effective_width, effective_height;

        effective_height = std::max (std::min ((float)(tex_desc.Height >> lod), 256.0f), std::min ((float)(tex_desc.Height >> lod), (content_avail_y - font_size_multiline * 11.0f - 24.0f)));
        effective_width  = std::min ((float)(tex_desc.Width  >> lod), (effective_height * (std::max (1.0f, (float)(tex_desc.Width >> lod)) / std::max (1.0f, (float)(tex_desc.Height >> lod)))));

        if (effective_width > (content_avail_x - font_size * 28.0f))
        {
          effective_width   = std::max (std::min ((float)(tex_desc.Width >> lod), 256.0f), (content_avail_x - font_size * 28.0f));
          effective_height  =  effective_width * (std::max (1.0f, (float)(tex_desc.Height >> lod)) / std::max (1.0f, (float)(tex_desc.Width >> lod)) );
        }

        ImGui::BeginGroup     ();
        ImGui::BeginChild     ( ImGui::GetID ("Texture_Select_D3D11"),
                                ImVec2 ( -1.0f, -1.0f ),
                                  true,
                                    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar |
                                    ImGuiWindowFlags_NavFlattened );

        //if ((! config.textures.highlight_debug_tex) && has_alternate)
        //{
        //  if (ImGui::IsItemHovered ())
        //    ImGui::SetTooltip ("Click me to make this texture the visible version.");
        //  
        //  // Allow the user to toggle texture override by clicking the frame
        //  if (ImGui::IsItemClicked ())
        //    __remap_textures = false;
        //}

        last_width  = effective_width  + font_size * 28.0f;

        ImGui::BeginGroup      (                  );
        ImGui::TextUnformatted ( "Dimensions:   " );
        ImGui::EndGroup        (                  );
        ImGui::SameLine        (                  );
        ImGui::BeginGroup      (                  );
        ImGui::PushItemWidth   (                -1);
        ImGui::Combo           ("###Texture_LOD_D3D11", &lod, lod_list, tex_desc.MipLevels);
        ImGui::PopItemWidth    (                  );
        ImGui::EndGroup        (                  );

        ImGui::BeginGroup      (                  );
        ImGui::TextUnformatted ( "Format:       " );
        ImGui::TextUnformatted ( "Data Size:    " );
        ImGui::TextUnformatted ( "Load Time:    " );
        ImGui::TextUnformatted ( "Cache Hits:   " );
        ImGui::EndGroup        (                  );
        ImGui::SameLine        (                  );
        ImGui::BeginGroup      (                  );
        ImGui::Text            ( "%ws",
                                   SK_DXGI_FormatToStr (tex_desc.Format).c_str () );
        ImGui::Text            ( "%.3f MiB",
                                   tex_size / (1024.0f * 1024.0f) );
        ImGui::Text            ( "%.3f ms",
                                   load_time );
        ImGui::Text            ( "%li",
                                   ReadAcquire (&SK_D3D11_Textures.Textures_2D [pTex].hits) );
        ImGui::EndGroup        (                  );
        ImGui::Separator       (                  );

        static bool flip_vertical   = false;
        static bool flip_horizontal = false;

        ImGui::Checkbox ("Flip Vertically##D3D11_FlipVertical",     &flip_vertical);   ImGui::SameLine ();
        ImGui::Checkbox ("Flip Horizontally##D3D11_FlipHorizontal", &flip_horizontal);

        if (! entry.injected)
        {
          if (! SK_D3D11_IsDumped (entry.crc32c, 0x0))
          {
            if ( ImGui::Button ("  Dump Texture to Disk  ###DumpTexture") )
            {
              SK_ScopedBool auto_bool (&pTLS->texture_management.injection_thread);
                                        pTLS->texture_management.injection_thread = true;

              SK_D3D11_DumpTexture2D (pTex, entry.crc32c);
            }
          }

          else
          {
            if ( ImGui::Button ("  Delete Dumped Texture from Disk  ###DumpTexture") )
            {
              SK_D3D11_DeleteDumpedTexture (entry.crc32c);
            }
          }

          ImGui::SameLine ();

          if (ImGui::Button ("  Generate Mipmaps  ###GenerateMipmaps"))
          {
            SK_ScopedBool auto_bool (&pTLS->texture_management.injection_thread);
                                      pTLS->texture_management.injection_thread = true;

            HRESULT
            __stdcall
            SK_D3D11_MipmapCacheTexture2D ( _In_ ID3D11Texture2D* pTex, uint32_t crc32c, SK_TLS* pTLS );
            SK_D3D11_MipmapCacheTexture2D (pTex, entry.crc32c, pTLS);
          }
        }

        if (staged)
        {
          ImGui::SameLine ();
          ImGui::TextColored (ImColor::HSV (0.25f, 1.0f, 1.0f), "Staged textures cannot be re-injected yet.");
        }

        if (entry.injected)
        {
          if ( ImGui::Button ("  Reload Texture  ###ReloadTexture") )
          {
            SK_D3D11_ReloadTexture (pTex);
          }

          ImGui::SameLine    ();
          ImGui::TextColored (ImVec4 (0.05f, 0.95f, 0.95f, 1.0f), "This texture has been injected over the original.");
        }

        if ( effective_height != (float)(tex_desc.Height >> lod) ||
             effective_width  != (float)(tex_desc.Width  >> lod) )
        {
          if (! entry.injected)
            ImGui::SameLine ();

          ImGui::TextColored (ImColor::HSV (0.5f, 1.0f, 1.0f), "Texture was rescaled to fit.");
        }

        if (! entry.injected)
          ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));
        else
           ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.05f, 0.95f, 0.95f, 1.0f));

        srv_desc.Texture2D.MipLevels       = 1;
        srv_desc.Texture2D.MostDetailedMip = lod;

        if (SUCCEEDED (pDev->CreateShaderResourceView (pTex, &srv_desc, &pSRV)))
        {
          ImVec2 uv0 (flip_horizontal ? 1.0f : 0.0f, flip_vertical ? 1.0f : 0.0f);
          ImVec2 uv1 (flip_horizontal ? 0.0f : 1.0f, flip_vertical ? 0.0f : 1.0f);

          ImGui::BeginChildFrame (ImGui::GetID ("TextureView_Frame"), ImVec2 (effective_width + 8.0f, effective_height + 8.0f),
                                  ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders |
                                  ImGuiWindowFlags_NoInputs         | ImGuiWindowFlags_NoScrollbar |
                                  ImGuiWindowFlags_NoNavInputs      | ImGuiWindowFlags_NoNavFocus );

          temp_resources.push_back (pSRV);
          ImGui::Image            ( pSRV,
                                     ImVec2 (effective_width, effective_height),
                                       uv0,                       uv1,
                                       ImColor (255,255,255,255), ImColor (255,255,255,128)
                               );
          ImGui::EndChildFrame ();

          static DWORD dwLastUnhovered = 0;

          if (ImGui::IsItemHovered ())
          {
            if (SK::ControlPanel::current_time - dwLastUnhovered > 666UL)
            {
              ImGui::BeginTooltip    ();
              ImGui::BeginGroup      ();
              ImGui::PushStyleColor  (ImGuiCol_Text, ImColor (0.785f, 0.785f, 0.785f));
              ImGui::TextUnformatted ("Usage:");
              ImGui::TextUnformatted ("Bind Flags:");
              if (tex_desc.MiscFlags != 0)
              ImGui::TextUnformatted ("Misc Flags:");
              ImGui::PopStyleColor   ();
              ImGui::EndGroup        ();
              ImGui::SameLine        ();
              ImGui::BeginGroup      ();
              ImGui::PushStyleColor  (ImGuiCol_Text, ImColor (1.0f, 1.0f, 1.0f));
              ImGui::Text            ("%ws", SK_D3D11_DescribeUsage     (
                                               tex_desc.Usage )             );
              ImGui::Text            ("%ws", SK_D3D11_DescribeBindFlags (
                              (D3D11_BIND_FLAG)tex_desc.BindFlags).c_str () );
              if (tex_desc.MiscFlags != 0)
              {
                ImGui::Text          ("%ws", SK_D3D11_DescribeMiscFlags (
                     (D3D11_RESOURCE_MISC_FLAG)tex_desc.MiscFlags).c_str () );
              }
              ImGui::PopStyleColor   ();
              ImGui::EndGroup        ();
              ImGui::EndTooltip      ();
            }
          }

          else
            dwLastUnhovered = SK::ControlPanel::current_time;
        }
        ImGui::PopStyleColor   ();
        ImGui::EndChild        ();
        ImGui::EndGroup        ();
        last_ht =
        ImGui::GetItemRectSize ().y;
        ImGui::PopStyleColor   ();
      }
    }

#if 0
    if (has_alternate)
    {
      ImGui::SameLine ();

      D3DSURFACE_DESC desc;

      if (SUCCEEDED (pTex->d3d9_tex->pTexOverride->GetLevelDesc (0, &desc)))
      {
        ImVec4 border_color = config.textures.highlight_debug_tex ? 
                                ImVec4 (0.3f, 0.3f, 0.3f, 1.0f) :
                                  (__remap_textures) ? ImVec4 (0.3f,  1.0f,  0.3f, 1.0f) :
                                                       ImVec4 (0.5f,  0.5f,  0.5f, 1.0f);

        ImGui::PushStyleColor  (ImGuiCol_Border, border_color);

        ImGui::BeginGroup ();
        ImGui::BeginChild ( "Item Selection2",
                            ImVec2 ( std::max (font_size * 19.0f, (float)desc.Width  + 24.0f),
                                                                  (float)desc.Height + font_size * 10.0f),
                              true,
                                ImGuiWindowFlags_AlwaysAutoResize );

        //if (! config.textures.highlight_debug_tex)
        //{
        //  if (ImGui::IsItemHovered ())
        //    ImGui::SetTooltip ("Click me to make this texture the visible version.");
        //
        //  // Allow the user to toggle texture override by clicking the frame
        //  if (ImGui::IsItemClicked ())
        //    __remap_textures = true;
        //}


        last_width  = std::max (last_width, (float)desc.Width);
        last_ht     = std::max (last_ht,    (float)desc.Height + font_size * 10.0f);


        extern std::wstring
        SK_D3D9_FormatToStr (D3DFORMAT Format, bool include_ordinal = true);


        SK_D3D11_IsInjectable ()
        bool injected  =
          (TBF_GetInjectableTexture (debug_tex_id) != nullptr),
             reloading = false;

        int num_lods = pTex->d3d9_tex->pTexOverride->GetLevelCount ();

        ImGui::Text ( "Dimensions:   %lux%lu  (%lu %s)",
                        desc.Width, desc.Height,
                           num_lods, num_lods > 1 ? "LODs" : "LOD" );
        ImGui::Text ( "Format:       %ws",
                        SK_D3D9_FormatToStr (desc.Format).c_str () );
        ImGui::Text ( "Data Size:    %.2f MiB",
                        (double)pTex->d3d9_tex->override_size / (1024.0f * 1024.0f) );
        ImGui::TextColored (ImVec4 (1.0f, 1.0f, 1.0f, 1.0f), injected ? "Injected Texture" : "Resampled Texture" );

        ImGui::Separator     ();


        if (injected)
        {
          if ( ImGui::Button ("  Reload This Texture  ") && tbf::RenderFix::tex_mgr.reloadTexture (debug_tex_id) )
          {
            reloading    = true;

            tbf::RenderFix::tex_mgr.updateOSD ();
          }
        }

        else {
          ImGui::Button ("  Resample This Texture  "); // NO-OP, but preserves alignment :P
        }

        if (! reloading)
        {
          ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));
          ImGui::BeginChildFrame (0, ImVec2 ((float)desc.Width + 8, (float)desc.Height + 8), ImGuiWindowFlags_ShowBorders);
          ImGui::Image           ( pTex->d3d9_tex->pTexOverride,
                                     ImVec2 ((float)desc.Width, (float)desc.Height),
                                       ImVec2  (0,0),             ImVec2  (1,1),
                                       ImColor (255,255,255,255), ImColor (255,255,255,128)
                                 );
          ImGui::EndChildFrame   ();
          ImGui::PopStyleColor   (1);
        }

        ImGui::EndChild        ();
        ImGui::EndGroup        ();
        ImGui::PopStyleColor   (1);
      }
    }
#endif
  }
  ImGui::EndGroup      ( );
  ImGui::PopStyleColor (1);
  ImGui::PopStyleVar   (2);
  ImGui::PopID         ( );
}


auto OnTopColorCycle =
[ ]
{
  return ImColor::HSV ( 0.491666f, 
                            std::min ( static_cast <float>(0.161616f +  (dwFrameTime % 250) / 250.0f -
                                                                floor ( (dwFrameTime % 250) / 250.0f) ), 1.0f),
                                std::min ( static_cast <float>(0.333 +  (dwFrameTime % 500) / 500.0f -
                                                                floor ( (dwFrameTime % 500) / 500.0f) ), 1.0f) );
};

auto WireframeColorCycle =
[ ]
{
  return ImColor::HSV ( 0.133333f, 
                            std::min ( static_cast <float>(0.161616f +  (dwFrameTime % 250) / 250.0f -
                                                                floor ( (dwFrameTime % 250) / 250.0f) ), 1.0f),
                                std::min ( static_cast <float>(0.333 +  (dwFrameTime % 500) / 500.0f -
                                                                floor ( (dwFrameTime % 500) / 500.0f) ), 1.0f) );
};

auto SkipColorCycle =
[ ]
{
  return ImColor::HSV ( 0.0f, 
                            std::min ( static_cast <float>(0.161616f +  (dwFrameTime % 250) / 250.0f -
                                                                floor ( (dwFrameTime % 250) / 250.0f) ), 1.0f),
                                std::min ( static_cast <float>(0.333 +  (dwFrameTime % 500) / 500.0f -
                                                                floor ( (dwFrameTime % 500) / 500.0f) ), 1.0f) );
};

void
SK_D3D11_ClearShaderState (void)
{
  for (int i = 0; i < 6; i++)
  {
    auto shader_record =
    [&]
    {
      switch (i)
      {
        default:
        case 0:
          return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.vertex;
        case 1:
          return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.pixel;
        case 2:
          return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.geometry;
        case 3:
          return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.hull;
        case 4:
          return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.domain;
        case 5:
          return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.compute;
      }
    };

    shader_record ()->blacklist.clear              ();
    shader_record ()->blacklist_if_texture.clear   ();
    shader_record ()->wireframe.clear              ();
    shader_record ()->on_top.clear                 ();
    shader_record ()->trigger_reshade.before.clear ();
    shader_record ()->trigger_reshade.after.clear  ();
  }
};

void
SK_D3D11_LoadShaderStateEx (std::wstring name, bool clear)
{
  auto wszShaderConfigFile =
    SK_FormatStringW ( LR"(%s\%s)",
                         SK_GetConfigPath (), name.c_str () );

  if (GetFileAttributesW (wszShaderConfigFile.c_str ()) == INVALID_FILE_ATTRIBUTES)
  {
    // No shader config file, nothing to do here...
    return;
  }

  std::unique_ptr <iSK_INI> d3d11_shaders_ini (
    SK_CreateINI (wszShaderConfigFile.c_str ())
  );

  //d3d11_shaders_ini->parse ();

  int shader_class = 0;

  iSK_INI::_TSectionMap& sections =
    d3d11_shaders_ini->get_sections ();

  auto sec =
    sections.begin ();

  struct draw_state_s {
    std::set <uint32_t>                       wireframe;
    std::set <uint32_t>                       on_top;
    std::set <uint32_t>                       disable;
    std::map <uint32_t, std::set <uint32_t> > disable_if_texture;
    std::set <uint32_t>                       trigger_reshade_before;
    std::set <uint32_t>                       trigger_reshade_after;
  } draw_states [7];

  auto shader_class_idx = [](const std::wstring& name)
  {
         if (name == L"Vertex")   return 0;
    else if (name == L"Pixel")    return 1;
    else if (name == L"Geometry") return 2;
    else if (name == L"Hull")     return 3;
    else if (name == L"Domain")   return 4;
    else if (name == L"Compute")  return 5;
    else                          return 6;
  };

  while (sec != sections.end ())
  {
    if (std::wcsstr ((*sec).first.c_str (), L"DrawState."))
    {
      wchar_t wszClass [32] = { };

      _snwscanf ((*sec).first.c_str (), 31, L"DrawState.%s", wszClass);

      shader_class =
        shader_class_idx (wszClass);

      for ( auto it : (*sec).second.keys )
      {
        unsigned int                        shader = 0U;
        swscanf (it.first.c_str (), L"%x", &shader);

        CHeapPtr <wchar_t> wszState (
          _wcsdup (it.second.c_str ())
        );

        wchar_t* wszBuf = nullptr;
        wchar_t* wszTok =
          std::wcstok (wszState, L",", &wszBuf);

        while (wszTok)
        {
          if (! _wcsicmp (wszTok, L"Wireframe"))
            draw_states [shader_class].wireframe.emplace (shader);
          else if (! _wcsicmp (wszTok, L"OnTop"))
            draw_states [shader_class].on_top.emplace (shader);
          else if (! _wcsicmp (wszTok, L"Disable"))
            draw_states [shader_class].disable.emplace (shader);
          else if (! _wcsicmp (wszTok, L"TriggerReShade"))
          {
            draw_states [shader_class].trigger_reshade_before.emplace (shader);
          }
          else if (! _wcsicmp (wszTok, L"TriggerReShadeAfter"))
          {
            draw_states [shader_class].trigger_reshade_after.emplace (shader);
          }
          else if ( StrStrIW (wszTok, L"DisableIfTexture=") == wszTok &&
                 std::wcslen (wszTok) >
                         std::wcslen (L"DisableIfTexture=") ) // Skip the degenerate case
          {
            uint32_t                                  crc32c;
            swscanf (wszTok, L"DisableIfTexture=%x", &crc32c);

            draw_states [shader_class].disable_if_texture [shader].insert (crc32c);
          }

          wszTok =
            std::wcstok (nullptr, L",", &wszBuf);
        }
      }
    }

    ++sec;
  }

  if (clear)
  {
    SK_D3D11_ClearShaderState ();
  }

  for (int i = 0; i < 6; i++)
  {
    auto shader_record =
      [&]
      {
        switch (i)
        {
          default:
          case 0:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.vertex;
          case 1:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.pixel;
          case 2:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.geometry;
          case 3:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.hull;
          case 4:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.domain;
          case 5:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.compute;
        }
      };

    shader_record ()->blacklist.insert ( draw_states [i].disable.begin   (), draw_states [i].disable.end   () );
    shader_record ()->wireframe.insert ( draw_states [i].wireframe.begin (), draw_states [i].wireframe.end () );
    shader_record ()->on_top.insert    ( draw_states [i].on_top.begin    (), draw_states [i].on_top.end    () );

    for (auto it : draw_states [i].trigger_reshade_before)
    {
      shader_record ()->trigger_reshade.before.insert (it);
    }

    for (auto it : draw_states [i].trigger_reshade_after)
    {
      shader_record ()->trigger_reshade.after.insert (it);
    }

    for ( auto it : draw_states [i].disable_if_texture )
    {
      shader_record ()->blacklist_if_texture [it.first].insert (
        it.second.begin (),
          it.second.end ()
      );
    }
  }

  SK_D3D11_EnableTracking = true;
}

void
SK_D3D11_LoadShaderState (bool clear = true)
{
  SK_D3D11_LoadShaderStateEx (L"d3d11_shaders.ini", clear);
}

void
SK_D3D11_UnloadShaderState (std::wstring name)
{
  auto wszShaderConfigFile =
    SK_FormatStringW ( LR"(%s\%s)",
                         SK_GetConfigPath (), name.c_str () );

  if (GetFileAttributesW (wszShaderConfigFile.c_str ()) == INVALID_FILE_ATTRIBUTES)
  {
    // No shader config file, nothing to do here...
    return;
  }

  std::unique_ptr <iSK_INI> d3d11_shaders_ini (
    SK_CreateINI (wszShaderConfigFile.c_str ())
  );

  int shader_class = 0;

  const iSK_INI::_TSectionMap& sections =
    d3d11_shaders_ini->get_sections ();

  auto sec =
    sections.begin ();

  struct draw_state_s {
    std::set <uint32_t>                       wireframe;
    std::set <uint32_t>                       on_top;
    std::set <uint32_t>                       disable;
    std::map <uint32_t, std::set <uint32_t> > disable_if_texture;
    std::set <uint32_t>                       trigger_reshade_before;
    std::set <uint32_t>                       trigger_reshade_after;
  } draw_states [7];

  auto shader_class_idx = [](const std::wstring& name)
  {
         if (name == L"Vertex")   return 0;
    else if (name == L"Pixel")    return 1;
    else if (name == L"Geometry") return 2;
    else if (name == L"Hull")     return 3;
    else if (name == L"Domain")   return 4;
    else if (name == L"Compute")  return 5;
    else                          return 6;
  };

  while (sec != sections.end ())
  {
    if (std::wcsstr ((*sec).first.c_str (), L"DrawState."))
    {
      wchar_t wszClass [32] = { };

      _snwscanf ((*sec).first.c_str (), 31, L"DrawState.%s", wszClass);

      shader_class =
        shader_class_idx (wszClass);

      for ( auto it : (*sec).second.keys )
      {
        unsigned int                        shader = 0U;
        swscanf (it.first.c_str (), L"%x", &shader);

        CHeapPtr <wchar_t> wszState (
          _wcsdup (it.second.c_str ())
        );

        wchar_t* wszBuf = nullptr;
        wchar_t* wszTok =
          std::wcstok (wszState, L",", &wszBuf);

        while (wszTok)
        {
          if (! _wcsicmp (wszTok, L"Wireframe"))
            draw_states [shader_class].wireframe.emplace (shader);
          else if (! _wcsicmp (wszTok, L"OnTop"))
            draw_states [shader_class].on_top.emplace (shader);
          else if (! _wcsicmp (wszTok, L"Disable"))
            draw_states [shader_class].disable.emplace (shader);
          else if (! _wcsicmp (wszTok, L"TriggerReShade"))
          {
            draw_states [shader_class].trigger_reshade_before.emplace (shader);
          }
          else if (! _wcsicmp (wszTok, L"TriggerReShadeAfter"))
          {
            draw_states [shader_class].trigger_reshade_after.emplace (shader);
          }
          else if ( StrStrIW (wszTok, L"DisableIfTexture=") == wszTok &&
                 std::wcslen (wszTok) >
                         std::wcslen (L"DisableIfTexture=") ) // Skip the degenerate case
          {
            uint32_t                                  crc32c;
            swscanf (wszTok, L"DisableIfTexture=%x", &crc32c);

            draw_states [shader_class].disable_if_texture [shader].emplace (crc32c);
          }

          wszTok =
            std::wcstok (nullptr, L",", &wszBuf);
        }
      }
    }

    ++sec;
  }

  for (int i = 0; i < 6; i++)
  {
    auto shader_record =
      [&]
      {
        switch (i)
        {
          default:
          case 0:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.vertex;
          case 1:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.pixel;
          case 2:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.geometry;
          case 3:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.hull;
          case 4:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.domain;
          case 5:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.compute;
        }
      };

    for (auto it : draw_states [i].disable)
      shader_record ()->blacklist.erase (it);

    for (auto it : draw_states [i].wireframe)
      shader_record ()->wireframe.erase (it);

    for (auto it : draw_states [i].on_top)
      shader_record ()->on_top.erase (it);

    for (auto it : draw_states [i].trigger_reshade_before)
    {
      shader_record ()->trigger_reshade.before.erase (it);
    }

    for (auto it : draw_states [i].trigger_reshade_after)
    {
      shader_record ()->trigger_reshade.after.erase (it);
    }

    for ( auto it : draw_states [i].disable_if_texture )
    {
      for (auto it2 : it.second)
      {
        shader_record ()->blacklist_if_texture [it.first].erase (it2);
      }
    }
  }

  SK_D3D11_EnableTracking = true;
}



void
EnumConstantBuffer ( ID3D11ShaderReflectionConstantBuffer* pConstantBuffer,
                     shader_disasm_s::constant_buffer&     cbuffer )
{
  if (! pConstantBuffer)
    return;

  D3D11_SHADER_BUFFER_DESC cbuffer_desc = { };

  if (SUCCEEDED (pConstantBuffer->GetDesc (&cbuffer_desc)))
  {
    //if (constant_desc.DefaultValue != nullptr)
    //  memcpy (constant.Data, constant_desc.DefaultValue, std::min (static_cast <size_t> (constant_desc.Bytes), sizeof (float) * 4));

    cbuffer.structs.push_back ( { } );

    shader_disasm_s::constant_buffer::struct_ent& unnamed_struct =
      cbuffer.structs.back ();

    for ( UINT j = 0; j < cbuffer_desc.Variables; j++ )
    {
      ID3D11ShaderReflectionVariable* pVar =
        pConstantBuffer->GetVariableByIndex (j);

      shader_disasm_s::constant_buffer::variable var = { };

      if (            pVar != nullptr && 
           SUCCEEDED (pVar->GetType ()->GetDesc (&var.type_desc)) )
      {
        if (SUCCEEDED (pVar->GetDesc (&var.var_desc)) && (var.var_desc.uFlags & D3D_SVF_USED) &&
                                                       !( var.var_desc.uFlags & D3D_SVF_USERPACKED ))
        {
          var.name = var.var_desc.Name;

          if (var.type_desc.Class == D3D_SVC_STRUCT)
          {
            shader_disasm_s::constant_buffer::struct_ent this_struct;
            this_struct.second = var.name;

            this_struct.first.emplace_back (var);

            // Stupid convoluted CBuffer struct design --
            //
            //   It is far simpler to do this ( with proper recursion ) in D3D9.
            for ( UINT k = 0 ; k < var.type_desc.Members ; k++ )
            {
              ID3D11ShaderReflectionType* mem_type = pVar->GetType ()->GetMemberTypeByIndex (k);
              D3D11_SHADER_TYPE_DESC      mem_type_desc = { };

              shader_disasm_s::constant_buffer::variable mem_var = { };

              mem_type->GetDesc (&mem_var.type_desc);

              if ( k == var.type_desc.Members - 1 )
                mem_var.var_desc.Size = ( var.var_desc.Size / std::max (1ui32, var.type_desc.Elements) ) - mem_var.type_desc.Offset;

              else
              {
                D3D11_SHADER_TYPE_DESC next_type_desc = { };

                pVar->GetType ()->GetMemberTypeByIndex (k + 1)->GetDesc (&next_type_desc);

                mem_var.var_desc.Size = next_type_desc.Offset - mem_var.type_desc.Offset;
              }

              mem_var.var_desc.StartOffset = var.var_desc.StartOffset + mem_var.type_desc.Offset;
              mem_var.name                 = pVar->GetType ()->GetMemberTypeName (k);

              dll_log.Log  (L"%hs.%hs <%lu> {%lu}", this_struct.second.c_str (), mem_var.name.c_str (), mem_var.var_desc.StartOffset, mem_var.var_desc.Size);

              this_struct.first.emplace_back (mem_var);
            }

            cbuffer.structs.emplace_back (this_struct);
          }

          else
          {
            if (var.var_desc.DefaultValue != nullptr)
            {
              memcpy ( (void *)var.default_value,
                               var.var_desc.DefaultValue, 
                       std::min (16 * sizeof (float), (size_t)var.var_desc.Size) );
            }

            unnamed_struct.first.emplace_back (var);
          }
        }
      }
    }
  }
};


void
SK_D3D11_StoreShaderState (void)
{
  auto wszShaderConfigFile =
    SK_FormatStringW ( LR"(%s\d3d11_shaders.ini)",
                         SK_GetConfigPath () );

  std::unique_ptr <iSK_INI> d3d11_shaders_ini (
    SK_CreateINI (wszShaderConfigFile.c_str ())
  );

  if (! d3d11_shaders_ini->get_sections ().empty ())
  {
    auto secs =
      d3d11_shaders_ini->get_sections ();

    for ( auto& it : secs )
    {
      d3d11_shaders_ini->remove_section (it.first.c_str ());
    }
  }

  auto shader_class_name = [](int i)
  {
    switch (i)
    {
      default:
      case 0: return L"Vertex";
      case 1: return L"Pixel";
      case 2: return L"Geometry";
      case 3: return L"Hull";
      case 4: return L"Domain";
      case 5: return L"Compute";
    };
  };

  for (int i = 0; i < 6; i++)
  {
    auto shader_record =
      [&]
      {
        switch (i)
        {
          default:
          case 0:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.vertex;
          case 1:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.pixel;
          case 2:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.geometry;
          case 3:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.hull;
          case 4:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.domain;
          case 5:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.compute;
        }
      };

    iSK_INISection& sec =
      d3d11_shaders_ini->get_section_f (L"DrawState.%s", shader_class_name (i));

    std::set <uint32_t> shaders;

    shaders.insert ( shader_record ()->blacklist.begin              (), shader_record ()->blacklist.end              () );
    shaders.insert ( shader_record ()->wireframe.begin              (), shader_record ()->wireframe.end              () );
    shaders.insert ( shader_record ()->on_top.begin                 (), shader_record ()->on_top.end                 () );
    shaders.insert ( shader_record ()->trigger_reshade.before.begin (), shader_record ()->trigger_reshade.before.end () );
    shaders.insert ( shader_record ()->trigger_reshade.after.begin  (), shader_record ()->trigger_reshade.after.end  () );

    for (auto it : shader_record ()->blacklist_if_texture)
    {
      shaders.insert ( it.first );
    }

    for ( auto it : shaders )
    {
      std::queue <std::wstring> states;

      if (shader_record ()->blacklist.count (it))
        states.emplace (L"Disable");

      if (shader_record ()->wireframe.count (it))
        states.emplace (L"Wireframe");

      if (shader_record ()->on_top.count (it))
        states.emplace (L"OnTop");

      if (shader_record ()->blacklist_if_texture.count (it))
      {
        for ( auto it2 : shader_record ()->blacklist_if_texture [it] )
        {
          states.emplace (SK_FormatStringW (L"DisableIfTexture=%x", it2));
        }
      }

      if (shader_record ()->trigger_reshade.before.count (it))
      {
        states.emplace (L"TriggerReShade");
      }

      if (shader_record ()->trigger_reshade.after.count (it))
      {
        states.emplace (L"TriggerReShadeAfter");
      }

      if (! states.empty ())
      {
        std::wstring state = L"";

        while (! states.empty ())
        {
          state += states.front ();
                   states.pop   ();

          if (! states.empty ()) state += L",";
        }

        sec.add_key_value ( SK_FormatStringW (L"%08x", it).c_str (), state.c_str ());
      }
    }
  }

  d3d11_shaders_ini->write (d3d11_shaders_ini->get_filename ());
}

void
SK_LiveShaderClassView (sk_shader_class shader_type, bool& can_scroll)
{
  std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_shader);

  ImGuiIO& io (ImGui::GetIO ());

  ImGui::PushID (static_cast <int> (shader_type));

  static float last_width = 256.0f;
  const  float font_size  = ImGui::GetFont ()->FontSize * io.FontGlobalScale;

  struct shader_class_imp_s
  {
    std::vector <
      std::pair <uint32_t, bool>
    >                         contents;
    bool                      dirty      = true;
    uint32_t                  last_sel   =    std::numeric_limits <uint32_t>::max ();
    unsigned int                   sel   =    0;
    float                     last_ht    = 256.0f;
    ImVec2                    last_min   = ImVec2 (0.0f, 0.0f);
    ImVec2                    last_max   = ImVec2 (0.0f, 0.0f);
  };

  struct {
    shader_class_imp_s vs;
    shader_class_imp_s ps;
    shader_class_imp_s gs;
    shader_class_imp_s hs;
    shader_class_imp_s ds;
    shader_class_imp_s cs;
  } static list_base;

  auto GetShaderList =
    [](const sk_shader_class& type) ->
      shader_class_imp_s*
      {
        switch (type)
        {
          case sk_shader_class::Vertex:   return &list_base.vs;
          case sk_shader_class::Pixel:    return &list_base.ps;
          case sk_shader_class::Geometry: return &list_base.gs;
          case sk_shader_class::Hull:     return &list_base.hs;
          case sk_shader_class::Domain:   return &list_base.ds;
          case sk_shader_class::Compute:  return &list_base.cs;
        }

        assert (false);

        return nullptr;
      };

  shader_class_imp_s*
    list = GetShaderList (shader_type);

  auto GetShaderTracker =
    [](const sk_shader_class& type) ->
      d3d11_shader_tracking_s*
      {
        switch (type)
        {
          case sk_shader_class::Vertex:   return &SK_D3D11_Shaders.vertex.tracked;
          case sk_shader_class::Pixel:    return &SK_D3D11_Shaders.pixel.tracked;
          case sk_shader_class::Geometry: return &SK_D3D11_Shaders.geometry.tracked;
          case sk_shader_class::Hull:     return &SK_D3D11_Shaders.hull.tracked;
          case sk_shader_class::Domain:   return &SK_D3D11_Shaders.domain.tracked;
          case sk_shader_class::Compute:  return &SK_D3D11_Shaders.compute.tracked;
        }

        assert (false);

        return nullptr;
      };

  d3d11_shader_tracking_s*
    tracker = GetShaderTracker (shader_type);

  auto GetShaderSet =
    [](const sk_shader_class& type) ->
      std::set <uint32_t>&
      {
        static std::set <uint32_t> set  [6];
        static size_t              size [6];

        switch (type)
        {
          case sk_shader_class::Vertex:
          {
            if (size [0] == SK_D3D11_Shaders.vertex.descs.size ())
              return set [0];

            for (auto const& vertex_shader : SK_D3D11_Shaders.vertex.descs)
            {
              // Ignore ImGui / CEGUI shaders
              if ( vertex_shader.first != 0xb42ede74 &&
                   vertex_shader.first != 0x1f8c62dc )
              {
                if (vertex_shader.first > 0x00000000) set [0].emplace (vertex_shader.first);
              }
            }

                   size [0] = SK_D3D11_Shaders.vertex.descs.size ();
            return set  [0];
          } break;

          case sk_shader_class::Pixel:
          {
            if (size [1] == SK_D3D11_Shaders.pixel.descs.size ())
              return set [1];

            for (auto const& pixel_shader : SK_D3D11_Shaders.pixel.descs)
            {
              // Ignore ImGui / CEGUI shaders
              if ( pixel_shader.first != 0xd3af3aa0 &&
                   pixel_shader.first != 0xb04a90ba )
              {
                if (pixel_shader.first > 0x00000000) set [1].emplace (pixel_shader.first);
              }
            }

                   size [1] = SK_D3D11_Shaders.pixel.descs.size ();
            return set  [1];
          } break;

          case sk_shader_class::Geometry:
          {
            if (size [2] == SK_D3D11_Shaders.geometry.descs.size ())
              return set [2];

            for (auto const& geometry_shader : SK_D3D11_Shaders.geometry.descs)
            {
              if (geometry_shader.first > 0x00000000) set [2].emplace (geometry_shader.first);
            }

                   size [2] = SK_D3D11_Shaders.geometry.descs.size ();
            return set  [2];
          } break;

          case sk_shader_class::Hull:
          {
            if (size [3] == SK_D3D11_Shaders.hull.descs.size ())
              return set [3];

            for (auto const& hull_shader : SK_D3D11_Shaders.hull.descs)
            {
              if (hull_shader.first > 0x00000000) set [3].emplace (hull_shader.first);
            }

                   size [3] = SK_D3D11_Shaders.hull.descs.size ();
            return set  [3];
          } break;

          case sk_shader_class::Domain:
          {
            if (size [4] == SK_D3D11_Shaders.domain.descs.size ())
              return set [4];

            for (auto const& domain_shader : SK_D3D11_Shaders.domain.descs)
            {
              if (domain_shader.first > 0x00000000) set [4].emplace (domain_shader.first);
            }

                   size [4] = SK_D3D11_Shaders.domain.descs.size ();
            return set  [4];
          } break;

          case sk_shader_class::Compute:
          {
            if (size [5] == SK_D3D11_Shaders.compute.descs.size ())
              return set [5];

            for (auto const& compute_shader : SK_D3D11_Shaders.compute.descs)
            {
              if (compute_shader.first > 0x00000000) set [5].emplace (compute_shader.first);
            }

                   size [5] = SK_D3D11_Shaders.compute.descs.size ();
            return set  [5];
          } break;
        }

        return set [0];
      };

  std::set <uint32_t>& set =
    GetShaderSet (shader_type);

  std::vector <uint32_t>
    shaders ( set.begin (), set.end () );

  auto GetShaderDisasm =
    [](const sk_shader_class& type) ->
      std::unordered_map <uint32_t, shader_disasm_s>*
      {
        switch (type)
        {
          case sk_shader_class::Vertex:   return &vs_disassembly;
          default:
          case sk_shader_class::Pixel:    return &ps_disassembly;
          case sk_shader_class::Geometry: return &gs_disassembly;
          case sk_shader_class::Hull:     return &hs_disassembly;
          case sk_shader_class::Domain:   return &ds_disassembly;
          case sk_shader_class::Compute:  return &cs_disassembly;
        }
      };

  std::unordered_map <uint32_t, shader_disasm_s>*
    disassembly = GetShaderDisasm (shader_type);

  auto GetShaderWord =
    [](const sk_shader_class& type) ->
      const char*
      {
        switch (type)
        {
          case sk_shader_class::Vertex:   return "Vertex";
          case sk_shader_class::Pixel:    return "Pixel";
          case sk_shader_class::Geometry: return "Geometry";
          case sk_shader_class::Hull:     return "Hull";
          case sk_shader_class::Domain:   return "Domain";
          case sk_shader_class::Compute:  return "Compute";
          default:                        return "Unknown";
        }
      };

  const char*
    szShaderWord = GetShaderWord (shader_type);

  uint32_t invalid;

  auto GetShaderChange =
    [&](const sk_shader_class& type) ->
      uint32_t&
      {
        switch (type)
        {
          case sk_shader_class::Vertex:   return change_sel_vs;
          case sk_shader_class::Pixel:    return change_sel_ps;
          case sk_shader_class::Geometry: return change_sel_gs;
          case sk_shader_class::Hull:     return change_sel_hs;
          case sk_shader_class::Domain:   return change_sel_ds;
          case sk_shader_class::Compute:  return change_sel_cs;
          default:                        return invalid;
        }
      };

  auto GetShaderBlacklist =
    [&](const sk_shader_class& type)->
      std::unordered_set <uint32_t>&
      {
        static std::unordered_set <uint32_t> invalid;

        switch (type)
        {
          case sk_shader_class::Vertex:   return SK_D3D11_Shaders.vertex.blacklist;
          case sk_shader_class::Pixel:    return SK_D3D11_Shaders.pixel.blacklist;
          case sk_shader_class::Geometry: return SK_D3D11_Shaders.geometry.blacklist;
          case sk_shader_class::Hull:     return SK_D3D11_Shaders.hull.blacklist;
          case sk_shader_class::Domain:   return SK_D3D11_Shaders.domain.blacklist;
          case sk_shader_class::Compute:  return SK_D3D11_Shaders.compute.blacklist;
          default:                        return invalid;
        }
      };

  auto GetShaderBlacklistEx =
    [&](const sk_shader_class& type)->
      SK_D3D11_KnownShaders::conditional_blacklist_t&
      {
        static SK_D3D11_KnownShaders::conditional_blacklist_t invalid;

        switch (type)
        {
          case sk_shader_class::Vertex:   return SK_D3D11_Shaders.vertex.blacklist_if_texture;
          case sk_shader_class::Pixel:    return SK_D3D11_Shaders.pixel.blacklist_if_texture;
          case sk_shader_class::Geometry: return SK_D3D11_Shaders.geometry.blacklist_if_texture;
          case sk_shader_class::Hull:     return SK_D3D11_Shaders.hull.blacklist_if_texture;
          case sk_shader_class::Domain:   return SK_D3D11_Shaders.domain.blacklist_if_texture;
          case sk_shader_class::Compute:  return SK_D3D11_Shaders.compute.blacklist_if_texture;
          default:                        return invalid;
        }
      };

  auto GetShaderUsedResourceViews =
    [&](const sk_shader_class& type)->
      std::vector <ID3D11ShaderResourceView *>&
      {
        static std::vector <ID3D11ShaderResourceView *> invalid;

        switch (type)
        {
          case sk_shader_class::Vertex:   return SK_D3D11_Shaders.vertex.tracked.used_views;
          case sk_shader_class::Pixel:    return SK_D3D11_Shaders.pixel.tracked.used_views;
          case sk_shader_class::Geometry: return SK_D3D11_Shaders.geometry.tracked.used_views;
          case sk_shader_class::Hull:     return SK_D3D11_Shaders.hull.tracked.used_views;
          case sk_shader_class::Domain:   return SK_D3D11_Shaders.domain.tracked.used_views;
          case sk_shader_class::Compute:  return SK_D3D11_Shaders.compute.tracked.used_views;
          default:                        return invalid;
        }
      };

  auto GetShaderResourceSet =
    [&](const sk_shader_class& type)->
      std::set <ID3D11ShaderResourceView *>&
      {
        static std::set <ID3D11ShaderResourceView *> invalid;

        switch (type)
        {
          case sk_shader_class::Vertex:   return SK_D3D11_Shaders.vertex.tracked.set_of_views;
          case sk_shader_class::Pixel:    return SK_D3D11_Shaders.pixel.tracked.set_of_views;
          case sk_shader_class::Geometry: return SK_D3D11_Shaders.geometry.tracked.set_of_views;
          case sk_shader_class::Hull:     return SK_D3D11_Shaders.hull.tracked.set_of_views;
          case sk_shader_class::Domain:   return SK_D3D11_Shaders.domain.tracked.set_of_views;
          case sk_shader_class::Compute:  return SK_D3D11_Shaders.compute.tracked.set_of_views;
          default:                        return invalid;
        }
      };

  struct sk_shader_state_s {
    unsigned int last_sel      = 0;
            bool sel_changed   = false;
            bool hide_inactive = true;
    unsigned int active_frames = 3;
            bool hovering      = false;
            bool focused       = false;

    static int ClassToIdx (sk_shader_class& shader_class)
    {
      // nb: shader_class is a bitmask, we need indices
      switch (shader_class)
      {
        case sk_shader_class::Vertex:   return 0;
        default:
        case sk_shader_class::Pixel:    return 1;
        case sk_shader_class::Geometry: return 2;
        case sk_shader_class::Hull:     return 3;
        case sk_shader_class::Domain:   return 4;
        case sk_shader_class::Compute:  return 5;

        // Masked combinations are, of course, invalid :)
      }
    }
  } static shader_state [6];

  unsigned int& last_sel      =  shader_state [sk_shader_state_s::ClassToIdx (shader_type)].last_sel;
  bool&         sel_changed   =  shader_state [sk_shader_state_s::ClassToIdx (shader_type)].sel_changed;
  bool*         hide_inactive = &shader_state [sk_shader_state_s::ClassToIdx (shader_type)].hide_inactive;
  unsigned int& active_frames =  shader_state [sk_shader_state_s::ClassToIdx (shader_type)].active_frames;
  bool scrolled = false;

  int dir = 0;

  if (ImGui::IsMouseHoveringRect (list->last_min, list->last_max))
  {
         if (io.KeysDown [VK_OEM_4] && io.KeysDownDuration [VK_OEM_4] == 0.0f) { dir = -1;  io.WantCaptureKeyboard = true; scrolled = true; }
    else if (io.KeysDown [VK_OEM_6] && io.KeysDownDuration [VK_OEM_6] == 0.0f) { dir = +1;  io.WantCaptureKeyboard = true; scrolled = true; }
  }

  ImGui::BeginGroup   ();

  if (ImGui::Checkbox ("Hide Inactive", hide_inactive))
  {
    list->dirty = true;
  }

  ImGui::SameLine     ();

  ImGui::TextDisabled ("   Tracked Shader: ");
  ImGui::SameLine     ();
  ImGui::BeginGroup   ();

  bool cancel =
    tracker->cancel_draws;

  if ( ImGui::Checkbox     ( shader_type != sk_shader_class::Compute ? "Skip Draws" :
                                                                       "Skip Dispatches",
                               &cancel )
     )
  {
    tracker->cancel_draws = cancel;
  }

  if (! cancel)
  {
    ImGui::SameLine   ();

    bool blink =
      tracker->highlight_draws;

    if (ImGui::Checkbox   ( "Blink", &blink ))
      tracker->highlight_draws = blink;

    if (shader_type != sk_shader_class::Compute)
    {
      bool wireframe =
        tracker->wireframe;

      ImGui::SameLine ();

      if ( ImGui::Checkbox ( "Draw in Wireframe", &wireframe ) )
        tracker->wireframe = wireframe;

      bool on_top =
        tracker->on_top;

      ImGui::SameLine ();

      if (ImGui::Checkbox ( "Draw on Top", &on_top))
        tracker->on_top = on_top;

      ImGui::SameLine ();

      bool probe_target =
        tracker->pre_hud_source;

      if (ImGui::Checkbox ("Probe Outputs", &probe_target))
      {
        tracker->pre_hud_source = probe_target;
        tracker->pre_hud_rtv    =      nullptr;
      }

      if (probe_target)
      {
        int rt_slot = tracker->pre_hud_rt_slot + 1;

        if ( ImGui::Combo ( "Examine RenderTarget", &rt_slot,
                               "N/A\0"
                               "Slot 0\0Slot 1\0Slot 2\0"
                               "Slot 3\0Slot 4\0Slot 5\0"
                               "Slot 6\0Slot 7\0\0" )
           )
        {
          tracker->pre_hud_rt_slot = ( rt_slot - 1 );
          tracker->pre_hud_rtv     =   nullptr;
        }


        if (ImGui::IsItemHovered () && tracker->pre_hud_rtv != nullptr)
        {
          ID3D11RenderTargetView* rt_view = tracker->pre_hud_rtv;
  
          D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = { };
          rt_view->GetDesc            (&rtv_desc);
  
          if (rtv_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D)
          {
            CComPtr <ID3D11Resource>          pRes = nullptr;
            rt_view->GetResource            (&pRes.p);
            CComQIPtr <ID3D11Texture2D> pTex (pRes);

            if (pRes && pTex)
            {
              D3D11_TEXTURE2D_DESC desc = { };
              pTex->GetDesc      (&desc);

              ID3D11ShaderResourceView*       pSRV     = nullptr;
              D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = { };
  
              srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
              srv_desc.Format                    = rtv_desc.Format;
              srv_desc.Texture2D.MipLevels       = (UINT)-1;
              srv_desc.Texture2D.MostDetailedMip =  0;

              CComQIPtr <ID3D11Device> pDev (SK_GetCurrentRenderBackend ().device);

              if (pDev != nullptr)
              {
                bool success =
                  SUCCEEDED (pDev->CreateShaderResourceView (pTex, &srv_desc, &pSRV));
  
                if (success)
                {
                  ImGui::BeginTooltip    ();
                  ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.5f, 0.5f, 0.5f, 1.0f));

                  ImGui::BeginGroup (                  );
                  ImGui::Text       ( "Dimensions:   " );
                  ImGui::Text       ( "Format:       " );
                  ImGui::EndGroup   (                  );
  
                  ImGui::SameLine   ( );
  
                  ImGui::BeginGroup (                                              );
                  ImGui::Text       ( "%lux%lu",
                                        desc.Width, desc.Height/*, effective_width, effective_height, 0.9875f * content_avail_y - ((float)(bottom_list + 3) * font_size * 1.125f), content_avail_y*//*,
                                          pTex->d3d9_tex->GetLevelCount ()*/       );
                  ImGui::Text       ( "%ws",
                                        SK_DXGI_FormatToStr (desc.Format).c_str () );
                  ImGui::EndGroup   (                                              );
    
                  if (pSRV != nullptr)
                  {
                    float effective_width  = 512.0f;
                    float effective_height = effective_width / ((float)desc.Width / (float)desc.Height);

                    ImGui::Separator  ( );

                    ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));
                    ImGui::BeginChildFrame   (ImGui::GetID ("ShaderResourceView_RT_Frame"),
                                              ImVec2 (effective_width + 8.0f, effective_height + 8.0f),
                                              ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders );

                    temp_resources.push_back (rt_view);
                                              rt_view->AddRef ();

                    temp_resources.push_back (pSRV);
                    ImGui::Image             ( pSRV,
                                                 ImVec2 (effective_width, effective_height),
                                                   ImVec2  (0,0),             ImVec2  (1,1),
                                                   ImColor (255,255,255,255), ImColor (255,255,255,128)
                                             );

                    ImGui::EndChildFrame     (    );
                    ImGui::PopStyleColor     (    );
                  }

                  ImGui::PopStyleColor       (    );
                  ImGui::EndTooltip          (    );
                }
              }
            }
          }
        }
      }
    }
  }
  ImGui::EndGroup ();

  if (shaders.size () != list->contents.size ())
  {
    list->dirty = true;
  }

  auto ShaderBase = [](sk_shader_class& shader_class) ->
    void*
    {
      switch (shader_class)
      {
        case sk_shader_class::Vertex:   return &SK_D3D11_Shaders.vertex;
        case sk_shader_class::Pixel:    return &SK_D3D11_Shaders.pixel;
        case sk_shader_class::Geometry: return &SK_D3D11_Shaders.geometry;
        case sk_shader_class::Hull:     return &SK_D3D11_Shaders.hull;
        case sk_shader_class::Domain:   return &SK_D3D11_Shaders.domain;
        case sk_shader_class::Compute:  return &SK_D3D11_Shaders.compute;
        default:
        return nullptr;
      }
    };


  auto GetShaderDesc = [&](sk_shader_class type, uint32_t crc32c) ->
    SK_D3D11_ShaderDesc&
      {
        return
          ((SK_D3D11_KnownShaders::ShaderRegistry <ID3D11VertexShader>*)ShaderBase (type))->descs [
            crc32c
          ];
      };

  auto ShaderIsActive = [&](sk_shader_class type, uint32_t crc32c) ->
    bool
      {
        return GetShaderDesc (type, crc32c).usage.last_frame > SK_GetFramesDrawn () - active_frames;
      };

  if (list->dirty || GetShaderChange (shader_type) != 0)
  {
        list->sel = 0;
    int idx       = 0;
        list->contents.clear ();

    for ( auto it : shaders )
    {
      const bool disabled =
        ( GetShaderBlacklist (shader_type).count (it) != 0 );

      list->contents.emplace_back (std::make_pair (it, (! disabled)));

      if ( ((! GetShaderChange (shader_type)) && list->last_sel == (uint32_t)it) ||
               GetShaderChange (shader_type)                    == (uint32_t)it )
      {
        list->sel = idx;

        // Allow other parts of the mod UI to change the selected shader
        //
        if (GetShaderChange (shader_type))
        {
          if (list->last_sel != GetShaderChange (shader_type))
            list->last_sel = std::numeric_limits <uint32_t>::max ();

          GetShaderChange (shader_type) = 0x00;
        }

        tracker->crc32c = it;
      }

      ++idx;
    }

    list->dirty = false;
  }

  ImGui::PushStyleVar   (ImGuiStyleVar_ChildWindowRounding, 0.0f);
  ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.9f, 0.7f, 0.5f, 1.0f));

  bool& hovering = shader_state [sk_shader_state_s::ClassToIdx (shader_type)].hovering;
  bool& focused  = shader_state [sk_shader_state_s::ClassToIdx (shader_type)].focused;

  ImGui::BeginChild ( ImGui::GetID (GetShaderWord (shader_type)),
                      ImVec2 ( font_size * 7.0f, std::max (font_size * 15.0f, list->last_ht)),
                        true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

  if (hovering || focused)
  {
    can_scroll = false;

    if (! focused)
    {
      ImGui::BeginTooltip ();
      ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "You can cancel all render passes using the selected %s shader to disable an effect", szShaderWord);
      ImGui::Separator    ();
      ImGui::BulletText   ("Press %ws while the mouse is hovering this list to select the previous shader", virtKeyCodeToHumanKeyName [VK_OEM_4].c_str ());
      ImGui::BulletText   ("Press %ws while the mouse is hovering this list to select the next shader",     virtKeyCodeToHumanKeyName [VK_OEM_6].c_str ());
      ImGui::EndTooltip   ();
    }

    else
    {
      ImGui::BeginTooltip ();
      ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "You can cancel all render passes using the selected %s shader to disable an effect", szShaderWord);
      ImGui::Separator    ();
      ImGui::BulletText   ("Press LB to select the previous shader");
      ImGui::BulletText   ("Press RB to select the next shader");
      ImGui::EndTooltip   ();
    }

    if (! scrolled)
    {
          if  (io.NavInputs [ImGuiNavInput_PadFocusPrev] && io.NavInputsDownDuration [ImGuiNavInput_PadFocusPrev] == 0.0f) { dir = -1; }
      else if (io.NavInputs [ImGuiNavInput_PadFocusNext] && io.NavInputsDownDuration [ImGuiNavInput_PadFocusNext] == 0.0f) { dir =  1; }

      else
      {
             if (io.KeysDown [VK_OEM_4] && io.KeysDownDuration [VK_OEM_4] == 0.0f) { dir = -1;  io.WantCaptureKeyboard = true; scrolled = true; }
        else if (io.KeysDown [VK_OEM_6] && io.KeysDownDuration [VK_OEM_6] == 0.0f) { dir = +1;  io.WantCaptureKeyboard = true; scrolled = true; }
      }
    }
  }

  if (! shaders.empty ())
  {
    // User wants to cycle list elements, we know only the direction, not how many indices in the list
    //   we need to increment to get an unfiltered list item.
    if (dir != 0)
    {
      if (list->sel == 0)                   // Can't go lower
        dir = std::max (0, dir);
      if (list->sel >= shaders.size () - 1) // Can't go higher
        dir = std::min (0, dir);

      int last_active = list->sel;

      do
      {
        list->sel += dir;

        if (*hide_inactive && list->sel <= shaders.size ())
        {
          if (! ShaderIsActive (shader_type, (uint32_t)shaders [list->sel]))
          {
            continue;
          }
        }

        last_active = list->sel;

        break;
      } while (list->sel > 0 && list->sel < shaders.size () - 1);

      // Don't go higher/lower if we're already at the limit
      list->sel = last_active;
    }


    if (list->sel != last_sel)
      sel_changed = true;

    last_sel = list->sel;

    auto ChangeSelectedShader = [](       shader_class_imp_s*  list,
                                    d3d11_shader_tracking_s*   tracker,
                                    SK_D3D11_ShaderDesc&       rDesc )
    {
      list->last_sel              = rDesc.crc32c;
      tracker->crc32c             = rDesc.crc32c;
      tracker->runtime_ms         = 0.0;
      tracker->last_runtime_ms    = 0.0;
      tracker->runtime_ticks.store (0LL);
    };

    // Line counting; assume the list sort order is unchanged
    unsigned int line = 0;

    for ( auto& it : list->contents )
    {
      SK_D3D11_ShaderDesc& rDesc =
        GetShaderDesc (shader_type, it.first);

      bool active = ShaderIsActive (shader_type, it.first);

      if ((! active) && (*hide_inactive))
      {
        line++;
        continue;
      }


      if (IsSkipped        (shader_type, it.first))
      {
        ImGui::PushStyleColor (ImGuiCol_Text, SkipColorCycle ());
      }

      else if (IsOnTop     (shader_type, it.first))
      {
       ImGui::PushStyleColor (ImGuiCol_Text, OnTopColorCycle ());
      }

      else if (IsWireframe (shader_type, it.first))
      {
        ImGui::PushStyleColor (ImGuiCol_Text, WireframeColorCycle ());
      }

      else
      {
        if (active)
          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.95f, 0.95f, 0.95f, 1.0f));
        else
          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.425f, 0.425f, 0.425f, 0.9f));
      }

      std::string line_text =
        SK_FormatString ("%c%08x", it.second ? ' ' : '*', it.first);


      if (line == list->sel)
      {
        bool selected = true;

        ImGui::Selectable (line_text.c_str (), &selected);

        if (sel_changed)
        {
          ImGui::SetScrollHere        (0.5f);
          ImGui::SetKeyboardFocusHere (    );

          sel_changed = false;

          ChangeSelectedShader (list, tracker, rDesc);
        }
      }

      else
      {
        bool selected    = false;

        if (ImGui::Selectable (line_text.c_str (), &selected))
        {
          sel_changed = true;
          list->sel   = line;

          ChangeSelectedShader (list, tracker, rDesc);
        }
      }

      ImGui::PushID (it.first);

      if (SK_ImGui_IsItemRightClicked ())
      {
        ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
        ImGui::OpenPopup         ("ShaderSubMenu");
      }

      if (ImGui::BeginPopup      ("ShaderSubMenu"))
      {
        static std::vector <ID3D11ShaderResourceView *> empty_list;
        static std::set    <ID3D11ShaderResourceView *> empty_set;

        ShaderMenu ( GetShaderBlacklist         (shader_type),
                     GetShaderBlacklistEx       (shader_type),
                     line == list->sel ? GetShaderUsedResourceViews (shader_type) :
                                         empty_list,
                     line == list->sel ? GetShaderResourceSet       (shader_type) :
                                         empty_set,
                     it.first );
        list->dirty = true;

        ImGui::EndPopup  ();
      }
      ImGui::PopID       ();

      line++;

      ImGui::PopStyleColor ();
    }

    CComPtr <ID3DBlob>               pDisasm  = nullptr;
    CComPtr <ID3D11ShaderReflection> pReflect = nullptr;

    HRESULT hr = E_FAIL;

    if (ReadAcquire ((volatile LONG *)&tracker->crc32c) != 0 && (! (*disassembly).count (ReadAcquire ((volatile LONG *)&tracker->crc32c))))
    {
      switch (shader_type)
      {
        case sk_shader_class::Vertex:
        {
          //if ( SK_D3D11_Shaders.vertex.descs [tracker->crc32c].bytecode.empty () )
          //{
            std::lock_guard <SK_Thread_CriticalSection> auto_lock_vs (cs_shader_vs);

            hr = D3DDisassemble ( SK_D3D11_Shaders.vertex.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.vertex.descs [tracker->crc32c].bytecode.size (),
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm.p);
            if (SUCCEEDED (hr))
                 D3DReflect     ( SK_D3D11_Shaders.vertex.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.vertex.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect.p);
          //}
        } break;

        case sk_shader_class::Pixel:
        {
          //if ( SK_D3D11_Shaders.pixel.descs [tracker->crc32c].bytecode.empty () )
          //{
            std::lock_guard <SK_Thread_CriticalSection> auto_lock_ps (cs_shader_ps);

            hr = D3DDisassemble ( SK_D3D11_Shaders.pixel.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.pixel.descs [tracker->crc32c].bytecode.size (),
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm.p);
            if (SUCCEEDED (hr))
                 D3DReflect     ( SK_D3D11_Shaders.pixel.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.pixel.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect.p);
          //}
        } break;

        case sk_shader_class::Geometry:
        {
          //if ( SK_D3D11_Shaders.geometry.descs [tracker->crc32c].bytecode.empty () )
          //{
            std::lock_guard <SK_Thread_CriticalSection> auto_lock_gs (cs_shader_gs);

            hr = D3DDisassemble ( SK_D3D11_Shaders.geometry.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.geometry.descs [tracker->crc32c].bytecode.size (), 
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm.p);
            if (SUCCEEDED (hr))
                 D3DReflect     ( SK_D3D11_Shaders.geometry.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.geometry.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect.p);
          //}
        } break;

        case sk_shader_class::Hull:
        {
          //if ( SK_D3D11_Shaders.hull.descs [tracker->crc32c].bytecode.empty () )
          //{
            std::lock_guard <SK_Thread_CriticalSection> auto_lock_hs (cs_shader_hs);

            hr = D3DDisassemble ( SK_D3D11_Shaders.hull.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.hull.descs [tracker->crc32c].bytecode.size (), 
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm.p);
            if (SUCCEEDED (hr))
                 D3DReflect     ( SK_D3D11_Shaders.hull.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.hull.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect.p);
          //}
        } break;

        case sk_shader_class::Domain:
        {
          //if ( SK_D3D11_Shaders.domain.descs [tracker->crc32c].bytecode.empty () )
          //{
            std::lock_guard <SK_Thread_CriticalSection> auto_lock_ds (cs_shader_ds);

            hr = D3DDisassemble ( SK_D3D11_Shaders.domain.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.domain.descs [tracker->crc32c].bytecode.size (), 
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm.p);
            if (SUCCEEDED (hr))
                 D3DReflect     ( SK_D3D11_Shaders.domain.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.domain.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect.p);
          //}
        } break;

        case sk_shader_class::Compute:
        {
          //if ( SK_D3D11_Shaders.compute.descs [tracker->crc32c].bytecode.empty () )
          //{
            std::lock_guard <SK_Thread_CriticalSection> auto_lock_cs (cs_shader_cs);

            hr = D3DDisassemble ( SK_D3D11_Shaders.compute.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.compute.descs [tracker->crc32c].bytecode.size (), 
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm.p);
            if (SUCCEEDED (hr))
                 D3DReflect     ( SK_D3D11_Shaders.compute.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.compute.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect.p);
          //}
        } break;
      }

      if (SUCCEEDED (hr)    && strlen ((const char *)pDisasm->GetBufferPointer ()))
      {
        char* szDisasm      = _strdup ((const char *)pDisasm->GetBufferPointer ());
        char* comments_end  = nullptr;

        if (szDisasm && strlen (szDisasm))
        {
          comments_end = strstr (szDisasm,          "\nvs_");
          if (! comments_end)
            comments_end      =                strstr (szDisasm,          "\nps_");
          if (! comments_end)
            comments_end      =                strstr (szDisasm,          "\ngs_");
          if (! comments_end)
            comments_end      =                strstr (szDisasm,          "\nhs_");
          if (! comments_end)
            comments_end      =                strstr (szDisasm,          "\nds_");
          if (! comments_end)
            comments_end      =                strstr (szDisasm,          "\ncs_");
          char* footer_begins = comments_end ? strstr (comments_end + 1, "\n//") : nullptr;

          if (comments_end)  *comments_end  = '\0'; else (comments_end  = "  ");
          if (footer_begins) *footer_begins = '\0'; else (footer_begins = "  ");

          disassembly->emplace ( ReadAcquire ((volatile LONG *)&tracker->crc32c),
                                   shader_disasm_s { szDisasm,
                                                       comments_end + 1,
                                                         footer_begins + 1
                                                   }
                               );

          if (pReflect)
          {
            D3D11_SHADER_DESC desc = { };

            if (SUCCEEDED (pReflect->GetDesc (&desc)))
            {
              for (UINT i = 0; i < desc.BoundResources; i++)
              {
                D3D11_SHADER_INPUT_BIND_DESC bind_desc = { };

                if (SUCCEEDED (pReflect->GetResourceBindingDesc (i, &bind_desc)))
                {
                  if (bind_desc.Type == D3D_SIT_CBUFFER)
                  {
                    ID3D11ShaderReflectionConstantBuffer* pReflectedCBuffer =
                      pReflect->GetConstantBufferByName (bind_desc.Name);

                    if (pReflectedCBuffer != nullptr)
                    {
                      D3D11_SHADER_BUFFER_DESC buffer_desc = { };
          
                      if (SUCCEEDED (pReflectedCBuffer->GetDesc (&buffer_desc)))
                      {
                        shader_disasm_s::constant_buffer cbuffer = { };

                        cbuffer.name = buffer_desc.Name;
                        cbuffer.size = buffer_desc.Size;
                        cbuffer.Slot = bind_desc.BindPoint;

                        EnumConstantBuffer (pReflectedCBuffer, cbuffer);

                        (*disassembly) [ReadAcquire ((volatile LONG *)&tracker->crc32c)].cbuffers.emplace_back (cbuffer);
                      }
                    }
                  }
                }
              }
            }
          }

          free (szDisasm);
        }
      }
    }
  }

  ImGui::EndChild      ();

  if (ImGui::IsItemHovered ()) hovering = true; else hovering = false;
  if (ImGui::IsItemFocused ()) focused  = true; else focused  = false;

  ImGui::PopStyleVar   ();
  ImGui::PopStyleColor ();

  ImGui::SameLine      ();
  ImGui::BeginGroup    ();

  if (ImGui::IsItemHoveredRect ())
  {
    if (! scrolled)
    {
           if (io.KeysDownDuration [VK_OEM_4] == 0.0f) list->sel--;
      else if (io.KeysDownDuration [VK_OEM_6] == 0.0f) list->sel++;
    }
  }

  static bool wireframe_dummy = false;
  bool&       wireframe       = wireframe_dummy;

  static bool on_top_dummy = false;
  bool&       on_top       = on_top_dummy;


  if (ReadAcquire ((volatile LONG *)&tracker->crc32c)    != 0x00 &&
                                      (SSIZE_T)list->sel >= 0    &&
                                               list->sel < (int)list->contents.size ())
  {
    ImGui::BeginGroup ();

    int num_used_textures = 0;

    std::set <ID3D11ShaderResourceView *> unique_views;

    if (! tracker->used_views.empty ())
    {
      for ( auto it : tracker->used_views )
      {
        if (! unique_views.emplace (it).second)
          continue;

        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;

        it->GetDesc (&srv_desc);

        if (srv_desc.ViewDimension == D3D_SRV_DIMENSION_TEXTURE2D)
        {
          ++num_used_textures;
        }
      }
    }

    static char szDraws      [128] = { };
    static char szTextures   [ 64] = { };
    static char szRuntime    [ 32] = { };
    static char szAvgRuntime [ 32] = { };

    if (shader_type != sk_shader_class::Compute)
    {
      if (tracker->cancel_draws)
        snprintf (szDraws, 128, "%3lu Skipped Draw%sLast Frame",  tracker->num_draws.load (), tracker->num_draws != 1 ? "s " : " ");
      else
        snprintf (szDraws, 128, "%3lu Draw%sLast Frame        " , tracker->num_draws.load (), tracker->num_draws != 1 ? "s " : " ");
    }

    else
    {
      if (tracker->cancel_draws)
        snprintf (szDraws, 128, "%3lu Skipped Dispatch%sLast Frame", tracker->num_draws.load (), tracker->num_draws != 1 ? "es " : " ");
      else
        snprintf (szDraws, 128, "%3lu Dispatch%sLast Frame        ", tracker->num_draws.load (), tracker->num_draws != 1 ? "es " : " ");
    }

    snprintf (szTextures,   64, "(%#i %s)",  num_used_textures, num_used_textures != 1 ? "textures" : "texture");
    snprintf (szRuntime,    32,  "%0.4f ms", tracker->runtime_ms);
    snprintf (szAvgRuntime, 32,  "%0.4f ms", tracker->runtime_ms / tracker->num_draws);

    SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* pShader = nullptr;

    switch (shader_type)
    {
      case sk_shader_class::Vertex:   pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.vertex;   break;
      case sk_shader_class::Pixel:    pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.pixel;    break;
      case sk_shader_class::Geometry: pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.geometry; break;
      case sk_shader_class::Hull:     pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.hull;     break;
      case sk_shader_class::Domain:   pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.domain;   break;
      case sk_shader_class::Compute:  pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.compute;  break;
    }

    ImGui::BeginGroup   ();
    ImGui::Separator    ();

    ImGui::Columns      (2, nullptr, false);
    ImGui::BeginGroup   ( );

    bool skip =
      pShader->blacklist.count (tracker->crc32c);

    ImGui::PushID (tracker->crc32c);

    if (skip)
    {
      ImGui::PushStyleColor (ImGuiCol_Text, SkipColorCycle ());
    }

    bool cancel_ = skip;

    if (ImGui::Checkbox ( shader_type == sk_shader_class::Compute ? "Never Dispatch" :
                                                                    "Never Draw", &cancel_))
    {
      if (cancel_)
        pShader->blacklist.emplace (tracker->crc32c);
      else
        pShader->blacklist.erase   (tracker->crc32c);
    }

    if (skip)
      ImGui::PopStyleColor ();

    if (shader_type != sk_shader_class::Compute)
    {
      wireframe = pShader->wireframe.count (tracker->crc32c);
      on_top    = pShader->on_top.count    (tracker->crc32c);

      bool wire = wireframe;

      if (wire)
        ImGui::PushStyleColor (ImGuiCol_Text, WireframeColorCycle ());

      if (ImGui::Checkbox ("Always Draw in Wireframe", &wireframe))
      {
        if (wireframe)
          pShader->wireframe.emplace (tracker->crc32c);
        else
          pShader->wireframe.erase   (tracker->crc32c);
      }

      if (wire)
        ImGui::PopStyleColor ();

      bool top = on_top;

      if (top)
        ImGui::PushStyleColor (ImGuiCol_Text, OnTopColorCycle ());

      if (ImGui::Checkbox ("Always Draw on Top", &on_top))
      {
        if (on_top)
          pShader->on_top.emplace (tracker->crc32c);
        else
          pShader->on_top.erase   (tracker->crc32c);
      }

      if (top)
        ImGui::PopStyleColor ();
    }
    else
    {
      ImGui::Columns  (1);
    }

    if (SK_ReShade_PresentCallback.fn != nullptr && shader_type != sk_shader_class::Compute)
    {
      bool reshade_before = pShader->trigger_reshade.before.count (tracker->crc32c);
      bool reshade_after  = pShader->trigger_reshade.after.count  (tracker->crc32c);

      if (ImGui::Checkbox ("Trigger ReShade On First Draw", &reshade_before))
      {
        if (reshade_before)
          pShader->trigger_reshade.before.emplace (tracker->crc32c);
        else
          pShader->trigger_reshade.before.erase   (tracker->crc32c);
      }

      if (ImGui::Checkbox ("Trigger ReShade After First Draw", &reshade_after))
      {
        if (reshade_after)
          pShader->trigger_reshade.after.emplace (tracker->crc32c);
        else
          pShader->trigger_reshade.after.erase   (tracker->crc32c);
      }
    }

    else
    {
      bool antistall = (ReadAcquire (&__SKX_ComputeAntiStall) != 0);

      if (ImGui::Checkbox ("Use ComputeShader Anti-Stall For Buffer Injection", &antistall))
      {
        InterlockedExchange (&__SKX_ComputeAntiStall, antistall ? 1 : 0);
      }
    }


    ImGui::PopID      ();
    ImGui::EndGroup   ();
    ImGui::EndGroup   ();

    ImGui::NextColumn ();

    ImGui::BeginGroup ();
    ImGui::BeginGroup ();

    ImGui::TextDisabled (szDraws);
    ImGui::TextDisabled ("Total GPU Runtime:");

    if (shader_type != sk_shader_class::Compute)
      ImGui::TextDisabled ("Avg Time per-Draw:");
    else
      ImGui::TextDisabled ("Avg Time per-Dispatch:");
    ImGui::EndGroup   ();

    ImGui::SameLine   ();

    ImGui::BeginGroup   ( );
    ImGui::TextDisabled (szTextures);
    ImGui::TextDisabled (tracker->num_draws > 0 ? szRuntime    : "N/A");
    ImGui::TextDisabled (tracker->num_draws > 0 ? szAvgRuntime : "N/A");
    ImGui::EndGroup     ( );
    ImGui::EndGroup     ( );

    ImGui::Columns      (1);
    ImGui::Separator    ( );

    ImGui::PushFont (io.Fonts->Fonts [1]); // Fixed-width font

    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.80f, 0.80f, 1.0f, 1.0f));
    ImGui::TextWrapped    ((*disassembly) [tracker->crc32c].header.c_str ());
    ImGui::PopStyleColor  ();
    
    ImGui::SameLine       ();
    ImGui::BeginGroup     ();

    ImGui::TreePush       ("");
    ImGui::Spacing        (); ImGui::Spacing ();


    ImGui::PushID (tracker->crc32c);

    size_t min_size = 0;

    for (auto&& it : (*disassembly) [tracker->crc32c].cbuffers)
    {
      ImGui::PushID (it.Slot);

      if (! tracker->overrides.empty ())
      {
        if (tracker->overrides [0].parent != tracker->crc32c)
        {
          tracker->overrides.clear ();
        }
      }

      for (auto&& itS : it.structs)
      {
        if (! itS.second.empty ())
        {
          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.3f, 0.6f, 0.9f, 1.0f));
          ImGui::Text           (itS.second.c_str ());
          ImGui::PopStyleColor  (  );
          ImGui::TreePush       ("");
        }
      for (auto&& it2 : itS.first)
      {        
        ImGui::PushID (it2.var_desc.StartOffset);

        if (tracker->overrides.size () < ++min_size)
        {
          tracker->overrides.push_back ( { tracker->crc32c, it.size,
                                             false, it.Slot,
                                                   it2.var_desc.StartOffset,
                                                   it2.var_desc.Size, { 0.0f, 0.0f, 0.0f, 0.0f,
                                                                        0.0f, 0.0f, 0.0f, 0.0f,
                                                                        0.0f, 0.0f, 0.0f, 0.0f, 
                                                                        0.0f, 0.0f, 0.0f, 0.0f } } );

          memcpy ( (void *)(tracker->overrides.back ().Values),
                                 (void *)it2.default_value,
                                   std::min ((size_t)it2.var_desc.Size, sizeof (float) * 16) );
        }

        auto& override_inst = tracker->overrides [min_size-1];

        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.9f, 0.9f, 0.9f, 1.0f));


        auto EnableButton = [&](void) -> void
        {
          ImGui::Checkbox ("##Enable", &override_inst.Enable);

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
            ImGui::Separator    ();
            ImGui::TextColored  ( ImVec4 (1.f, 1.f, 1.f, 1.f),
                                  "CBuffer [%lu] => { Offset: %lu, Value Size: %lu }",
                                  override_inst.Slot,
                                                    override_inst.StartAddr,
                                                    override_inst.Size );
            ImGui::EndTooltip   ();
          }
        };


        if ( it2.type_desc.Class == D3D_SVC_SCALAR ||
             it2.type_desc.Class == D3D_SVC_VECTOR )
        {
          EnableButton ();

          if (it2.type_desc.Type == D3D_SVT_FLOAT)
          {
            ImGui::SameLine    ();
            ImGui::InputFloatN (it2.name.c_str (), &override_inst.Values [0], (int)it2.type_desc.Columns, 4, 0x0);
          }

          else if ( it2.type_desc.Type == D3D_SVT_INT  ||
                    it2.type_desc.Type == D3D_SVT_UINT ||
                    it2.type_desc.Type == D3D_SVT_UINT8 )
          {
            if (it2.type_desc.Type == D3D_SVT_INT)
            {
              ImGui::SameLine  ();
              ImGui::InputIntN (it2.name.c_str (), (int *)&override_inst.Values [0], (int)it2.type_desc.Columns, 0x0);
            }

            else
            {
              for ( UINT k = 0 ; k < it2.type_desc.Columns ; k++ )
              {
                ImGui::SameLine  ( );
                ImGui::PushID    (k);

                int val = it2.type_desc.Type == D3D_SVT_UINT8 ? (int)((uint8_t *)override_inst.Values) [k] :
                                                                    ((uint32_t *)override_inst.Values) [k];

                if (ImGui::InputIntN (it2.name.c_str (), &val, 1, 0x0))
                {
                  if (val < 0) val = 0;

                  if (it2.type_desc.Type == D3D_SVT_UINT8)
                  {
                    if (val > 255) val = 255;

                    (( uint8_t *)override_inst.Values) [k] = (uint8_t)val;
                  }

                  else
                    ((uint32_t *)override_inst.Values) [k] = (uint32_t)val;

                }
                ImGui::PopID     ( );
              }
            }
          }

          else if (it2.type_desc.Type == D3D_SVT_BOOL)
          {
            for ( UINT k = 0 ; k < it2.type_desc.Columns ; k++ )
            {
              ImGui::SameLine ( );
              ImGui::PushID   (k);
              ImGui::Checkbox (it2.name.c_str (), (bool *)&((BOOL *)override_inst.Values) [k]);
              ImGui::PopID    ( );
            }
          }
        }

        else if ( it2.type_desc.Class == D3D_SVC_MATRIX_ROWS ||
                  it2.type_desc.Class == D3D_SVC_MATRIX_COLUMNS )
        {
          EnableButton ();

          ImGui::SameLine ();

          ImGui::BeginGroup ();
          float* fMatrixPtr = override_inst.Values;

          for ( UINT k = 0 ; k < it2.type_desc.Rows; k++ )
          {
            ImGui::Text     (SK_FormatString ("%s <m%lu>", it2.name.c_str (), k).c_str ());
            ImGui::SameLine ( );
            ImGui::PushID   (k);
            ImGui::InputFloatN ("##Matrix_Row",
                                 fMatrixPtr, it2.type_desc.Columns, 4, 0x0 );
            ImGui::PopID    ( );

            fMatrixPtr += it2.type_desc.Columns;
          }
          ImGui::EndGroup   ( );
        }

        else if (it2.type_desc.Class == D3D_SVC_STRUCT)
        {
          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.3f, 0.6f, 0.9f, 1.0f));
          ImGui::Text           (it2.name.c_str ());
          ImGui::PopStyleColor  ();
        }

        else
        {
          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.9f, 0.6f, 0.3f, 1.0f));
          ImGui::Text           (it2.name.c_str ());
          ImGui::PopStyleColor  ();
        }

        ImGui::PopStyleColor  ();
        ImGui::PopID          ();
      }

      if (! itS.second.empty ())
        ImGui::TreePop ();
    }

      ImGui::PopID ();
    }

    ImGui::PopID ();

    ImGui::TreePop    ();
    ImGui::EndGroup   ();
    ImGui::EndGroup   ();

    if (ImGui::IsItemHoveredRect () && (! tracker->used_views.empty ()))
    {
      ImGui::BeginTooltip ();

      DXGI_FORMAT fmt = DXGI_FORMAT_UNKNOWN;

      unique_views.clear ();

      for ( auto it : tracker->used_views )
      {
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;

        it->GetDesc (&srv_desc);

        if (srv_desc.ViewDimension == D3D_SRV_DIMENSION_TEXTURE2D)
        {
          CComPtr   <ID3D11Resource>        pRes = nullptr;
                          it->GetResource (&pRes.p);
          CComQIPtr <ID3D11Texture2D> pTex (pRes);

          if (pRes && pTex)
          {
            D3D11_TEXTURE2D_DESC desc = { };
            pTex->GetDesc      (&desc);
                           fmt = desc.Format;

            if (desc.Height > 0 && desc.Width > 0)
            {
              if (! unique_views.emplace (it).second)
                continue;

                                           it->AddRef ();
              temp_resources.emplace_back (it);

              ImGui::Image ( it,         ImVec2  ( std::max (64.0f, (float)desc.Width / 16.0f),
        ((float)desc.Height / (float)desc.Width) * std::max (64.0f, (float)desc.Width / 16.0f) ),
                                         ImVec2  (0,0),             ImVec2  (1,1),
                              desc.Format == DXGI_FORMAT_R8_UNORM ? ImColor (0, 255, 255, 255) :
                                         ImColor (255,255,255,255), ImColor (242,242,13,255) );
            }

            ImGui::SameLine   ();

            ImGui::BeginGroup ();
            ImGui::Text       ("Texture: ");
            ImGui::Text       ("Format:  ");
            ImGui::EndGroup   ();

            ImGui::SameLine   ();

            ImGui::BeginGroup ();
            ImGui::Text       ("%08lx", pTex.p);
            ImGui::Text       ("%ws",   SK_DXGI_FormatToStr (fmt).c_str ());
            ImGui::EndGroup   ();
          }
        }
      }
    
      ImGui::EndTooltip ();
    }
#if 0
    ImGui::SameLine       ();
    ImGui::BeginGroup     ();
    ImGui::TreePush       ("");
    ImGui::Spacing        (); ImGui::Spacing ();
    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.666f, 0.666f, 0.666f, 1.0f));
    
    char szName    [192] = { };
    char szOrdinal [64 ] = { };
    char szOrdEl   [ 96] = { };
    
    int  el = 0;
    
    ImGui::PushItemWidth (font_size * 25);
    
    for ( auto&& it : tracker->constants )
    {
      if (it.struct_members.size ())
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.9f, 0.1f, 0.7f, 1.0f));
        ImGui::Text           (it.Name);
        ImGui::PopStyleColor  ();
    
        for ( auto&& it2 : it.struct_members )
        {
          snprintf ( szOrdinal, 64, " (%c%-3lu) ",
                        it2.RegisterSet != D3DXRS_SAMPLER ? 'c' : 's',
                          it2.RegisterIndex );
          snprintf ( szOrdEl,  96,  "%s::%lu %c", // Uniquely identify parameters that share registers
                       szOrdinal, el++, shader_type == tbf_shader_class::Pixel ? 'p' : 'v' );
          snprintf ( szName, 192, "[%s] %-24s :%s",
                       shader_type == tbf_shader_class::Pixel ? "ps" :
                                                                "vs",
                         it2.Name, szOrdinal );
    
          if (it2.Type == D3DXPT_FLOAT && it2.Class == D3DXPC_VECTOR)
          {
            ImGui::Checkbox    (szName,  &it2.Override); ImGui::SameLine ();
            ImGui::InputFloat4 (szOrdEl,  it2.Data);
          }
          else {
            ImGui::TreePush (""); ImGui::TextColored (ImVec4 (0.45f, 0.75f, 0.45f, 1.0f), szName); ImGui::TreePop ();
          }
        }
    
        ImGui::Separator ();
      }
    
      else
      {
        snprintf ( szOrdinal, 64, " (%c%-3lu) ",
                     it.RegisterSet != D3DXRS_SAMPLER ? 'c' : 's',
                        it.RegisterIndex );
        snprintf ( szOrdEl,  96,  "%s::%lu %c", // Uniquely identify parameters that share registers
                       szOrdinal, el++, shader_type == tbf_shader_class::Pixel ? 'p' : 'v' );
        snprintf ( szName, 192, "[%s] %-24s :%s",
                     shader_type == tbf_shader_class::Pixel ? "ps" :
                                                              "vs",
                         it.Name, szOrdinal );
    
        if (it.Type == D3DXPT_FLOAT && it.Class == D3DXPC_VECTOR)
        {
          ImGui::Checkbox    (szName,  &it.Override); ImGui::SameLine ();
          ImGui::InputFloat4 (szOrdEl,  it.Data);
        } else {
          ImGui::TreePush (""); ImGui::TextColored (ImVec4 (0.45f, 0.75f, 0.45f, 1.0f), szName); ImGui::TreePop ();
        }
      }
    }
    ImGui::PopItemWidth ();
    ImGui::TreePop      ();
    ImGui::EndGroup     ();
#endif
    
    ImGui::Separator      ();
    
    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.99f, 0.99f, 0.01f, 1.0f));
    ImGui::TextWrapped    ((*disassembly) [ReadAcquire ((volatile LONG *)&tracker->crc32c)].code.c_str ());
    
    ImGui::Separator      ();
    
    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.5f, 0.95f, 0.5f, 1.0f));
    ImGui::TextWrapped    ((*disassembly) [ReadAcquire ((volatile LONG *)&tracker->crc32c)].footer.c_str ());

    ImGui::PopFont        ();

    ImGui::PopStyleColor (2);
  }
  else
    tracker->cancel_draws = false;

  ImGui::EndGroup ();

  list->last_ht    = ImGui::GetItemRectSize ().y;

  ImGui::EndGroup ();

  list->last_min   = ImGui::GetItemRectMin ();
  list->last_max   = ImGui::GetItemRectMax ();

  ImGui::PopID    ();
}

void
SK_D3D11_EndFrame (SK_TLS* pTLS = SK_TLS_Bottom ())
{
  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_render_view);

  dwFrameTime = SK::ControlPanel::current_time;

  static ULONG frames = 0;

  if (frames++ == 0)
  {
    SK_D3D11_LoadShaderState ();
  }


  SK_D3D11_MemoryThreads.clear_active   ();
  SK_D3D11_ShaderThreads.clear_active   ();
  SK_D3D11_DrawThreads.clear_active     ();
  SK_D3D11_DispatchThreads.clear_active ();


  for ( auto& it : SK_D3D11_Shaders.reshade_triggered )
    it = false;


  SK_D3D11_Shaders.vertex.tracked.deactivate   ( nullptr );
  SK_D3D11_Shaders.pixel.tracked.deactivate    ( nullptr );
  SK_D3D11_Shaders.geometry.tracked.deactivate ( nullptr );
  SK_D3D11_Shaders.hull.tracked.deactivate     ( nullptr );
  SK_D3D11_Shaders.domain.tracked.deactivate   ( nullptr );
  SK_D3D11_Shaders.compute.tracked.deactivate  ( nullptr );

  for ( auto&& it : SK_D3D11_Shaders.vertex.current.views   ) { for (auto&& it2 : it ) it2 = nullptr; }
  for ( auto&& it : SK_D3D11_Shaders.pixel.current.views    ) { for (auto&& it2 : it ) it2 = nullptr; }
  for ( auto&& it : SK_D3D11_Shaders.geometry.current.views ) { for (auto&& it2 : it ) it2 = nullptr; }
  for ( auto&& it : SK_D3D11_Shaders.domain.current.views   ) { for (auto&& it2 : it ) it2 = nullptr; }
  for ( auto&& it : SK_D3D11_Shaders.hull.current.views     ) { for (auto&& it2 : it ) it2 = nullptr; }
  for ( auto&& it : SK_D3D11_Shaders.compute.current.views  ) { for (auto&& it2 : it ) it2 = nullptr; }



  tracked_rtv.clear   ();
  used_textures.clear ();

  mem_map_stats.clear ();

  // True if the disjoint query is complete and we can get the results of
  //   each tracked shader's timing
  static bool disjoint_done = false;

  CComPtr <ID3D11Device>        pDev    = nullptr;
  CComPtr <ID3D11DeviceContext> pDevCtx = nullptr;

  if (            rb.device                                         &&
       SUCCEEDED (rb.device->QueryInterface <ID3D11Device> (&pDev)) &&
                  rb.d3d11.immediate_ctx != nullptr )
  {
    rb.d3d11.immediate_ctx->QueryInterface <ID3D11DeviceContext> (&pDevCtx);
  }

  else
    return;


  if (! pDevCtx)
    disjoint_done = true;

  // End the Query and probe results (when the pipeline has drained)
  if (pDevCtx && (! disjoint_done) && ReadPointerAcquire ((volatile PVOID *)&d3d11_shader_tracking_s::disjoint_query.async))
  {
    if (pDevCtx != nullptr)
    {
      if (ReadAcquire (&d3d11_shader_tracking_s::disjoint_query.active))
      {
        pDevCtx->End ((ID3D11Asynchronous *)ReadPointerAcquire ((volatile PVOID*)&d3d11_shader_tracking_s::disjoint_query.async));
        InterlockedExchange (&d3d11_shader_tracking_s::disjoint_query.active, FALSE);
      }

      else
      {
        HRESULT const hr =
            pDevCtx->GetData ( (ID3D11Asynchronous *)ReadPointerAcquire ((volatile PVOID*)&d3d11_shader_tracking_s::disjoint_query.async),
                                &d3d11_shader_tracking_s::disjoint_query.last_results,
                                  sizeof D3D11_QUERY_DATA_TIMESTAMP_DISJOINT,
                                    0x0 );

        if (hr == S_OK)
        {
          ((ID3D11Asynchronous *)ReadPointerAcquire ((volatile PVOID*)&d3d11_shader_tracking_s::disjoint_query.async))->Release ();
          InterlockedExchangePointer ((void **)&d3d11_shader_tracking_s::disjoint_query.async, nullptr);

          // Check for failure, if so, toss out the results.
          if (! d3d11_shader_tracking_s::disjoint_query.last_results.Disjoint)
            disjoint_done = true;

          else
          {
            auto ClearTimers = [](d3d11_shader_tracking_s* tracker)
            {
              for (auto& it : tracker->timers)
              {
                SK_COM_ValidateRelease ((IUnknown **)&it.start.async);
                SK_COM_ValidateRelease ((IUnknown **)&it.end.async);

                SK_COM_ValidateRelease ((IUnknown **)&it.start.dev_ctx);
                SK_COM_ValidateRelease ((IUnknown **)&it.end.dev_ctx);
              }

              tracker->timers.clear ();
            };

            ClearTimers (&SK_D3D11_Shaders.vertex.tracked);
            ClearTimers (&SK_D3D11_Shaders.pixel.tracked);
            ClearTimers (&SK_D3D11_Shaders.geometry.tracked);
            ClearTimers (&SK_D3D11_Shaders.hull.tracked);
            ClearTimers (&SK_D3D11_Shaders.domain.tracked);
            ClearTimers (&SK_D3D11_Shaders.compute.tracked);

            disjoint_done = true;
          }
        }
      }
    }
  }

  if (pDevCtx && disjoint_done)
  {
    auto GetTimerDataStart = [](d3d11_shader_tracking_s::duration_s* duration, bool& success) ->
      UINT64
      {
        ID3D11DeviceContext* dev_ctx =
          (ID3D11DeviceContext *)ReadPointerAcquire ((volatile PVOID *)&duration->start.dev_ctx);

        if (            dev_ctx != nullptr &&
             SUCCEEDED (dev_ctx->GetData ( (ID3D11Query *)ReadPointerAcquire ((volatile PVOID *)&duration->start.async), &duration->start.last_results, sizeof UINT64, 0x00 )))
        {
          SK_COM_ValidateRelease ((IUnknown **)&duration->start.async);
          SK_COM_ValidateRelease ((IUnknown **)&duration->start.dev_ctx);

          success = true;
          
          return duration->start.last_results;
        }

        success = false;

        return 0;
      };

    auto GetTimerDataEnd = [](d3d11_shader_tracking_s::duration_s* duration, bool& success) ->
      UINT64
      {
        if ((ID3D11Query *)ReadPointerAcquire ((volatile PVOID *)&duration->end.async) == nullptr)
          return duration->start.last_results;

        ID3D11DeviceContext* dev_ctx =
          (ID3D11DeviceContext *)ReadPointerAcquire ((volatile PVOID *)&duration->end.dev_ctx);

        if (            dev_ctx != nullptr &&
             SUCCEEDED (dev_ctx->GetData ( (ID3D11Query *)ReadPointerAcquire ((volatile PVOID *)&duration->end.async), &duration->end.last_results, sizeof UINT64, 0x00 )))
        {
          SK_COM_ValidateRelease ((IUnknown **)&duration->end.async);
          SK_COM_ValidateRelease ((IUnknown **)&duration->end.dev_ctx);

          success = true;

          return duration->end.last_results;
        }

        success = false;

        return 0;
      };

    auto CalcRuntimeMS = [](d3d11_shader_tracking_s* tracker)
    {
      if (tracker->runtime_ticks != 0ULL)
      {
        tracker->runtime_ms =
          1000.0 * (((double)tracker->runtime_ticks.load ()) / (double)tracker->disjoint_query.last_results.Frequency);

        // Filter out queries that spanned multiple frames
        //
        if (tracker->runtime_ms > 0.0 && tracker->last_runtime_ms > 0.0)
        {
          if (tracker->runtime_ms > tracker->last_runtime_ms * 100.0 || tracker->runtime_ms > 12.0)
            tracker->runtime_ms = tracker->last_runtime_ms;
        }

        tracker->last_runtime_ms = tracker->runtime_ms;
      }
    };

    auto AccumulateRuntimeTicks = [&]( d3d11_shader_tracking_s*       tracker,
                                 const std::unordered_set <uint32_t>& blacklist )
      {
        std::vector <d3d11_shader_tracking_s::duration_s> rejects;

        tracker->runtime_ticks = 0ULL;

        for (auto& it : tracker->timers)
        {
          bool   success0 = false, success1 = false;

          UINT64 time0    = GetTimerDataEnd   (&it, success0),
                 time1    = GetTimerDataStart (&it, success1);

          if (success0 && success1)
            tracker->runtime_ticks += (time0 - time1);
          else
            rejects.emplace_back (it);
        }


        if (tracker->cancel_draws || tracker->num_draws == 0 || blacklist.count (tracker->crc32c))
        {
          tracker->runtime_ticks   = 0ULL;
          tracker->runtime_ms      = 0.0;
          tracker->last_runtime_ms = 0.0;
        }

        tracker->timers.clear ();

        // Anything that fails goes back on the list and we will try again next frame
        tracker->timers = rejects;
      };

    AccumulateRuntimeTicks (&SK_D3D11_Shaders.vertex.tracked,   SK_D3D11_Shaders.vertex.blacklist);
    CalcRuntimeMS          (&SK_D3D11_Shaders.vertex.tracked);

    AccumulateRuntimeTicks (&SK_D3D11_Shaders.pixel.tracked,    SK_D3D11_Shaders.pixel.blacklist);
    CalcRuntimeMS          (&SK_D3D11_Shaders.pixel.tracked);

    AccumulateRuntimeTicks (&SK_D3D11_Shaders.geometry.tracked, SK_D3D11_Shaders.geometry.blacklist);
    CalcRuntimeMS          (&SK_D3D11_Shaders.geometry.tracked);

    AccumulateRuntimeTicks (&SK_D3D11_Shaders.hull.tracked,     SK_D3D11_Shaders.hull.blacklist);
    CalcRuntimeMS          (&SK_D3D11_Shaders.hull.tracked);

    AccumulateRuntimeTicks (&SK_D3D11_Shaders.domain.tracked,   SK_D3D11_Shaders.domain.blacklist);
    CalcRuntimeMS          (&SK_D3D11_Shaders.domain.tracked);

    AccumulateRuntimeTicks (&SK_D3D11_Shaders.compute.tracked,  SK_D3D11_Shaders.compute.blacklist);
    CalcRuntimeMS          (&SK_D3D11_Shaders.compute.tracked);

    disjoint_done = false;
  }
 
  SK_D3D11_Shaders.vertex.tracked.clear   ();
  SK_D3D11_Shaders.pixel.tracked.clear    ();
  SK_D3D11_Shaders.geometry.tracked.clear ();
  SK_D3D11_Shaders.hull.tracked.clear     ();
  SK_D3D11_Shaders.domain.tracked.clear   ();
  SK_D3D11_Shaders.compute.tracked.clear  ();

  SK_D3D11_Shaders.vertex.changes_last_frame   = 0;
  SK_D3D11_Shaders.pixel.changes_last_frame    = 0;
  SK_D3D11_Shaders.geometry.changes_last_frame = 0;
  SK_D3D11_Shaders.hull.changes_last_frame     = 0;
  SK_D3D11_Shaders.domain.changes_last_frame   = 0;
  SK_D3D11_Shaders.compute.changes_last_frame  = 0;


  for (auto& it_ctx : SK_D3D11_PerCtxResources )
  {
    int spins = 0;

    while (InterlockedCompareExchange (&it_ctx.writing_, 1, 0) != 0)
    {
      if ( ++spins > 0x1000 )
      {
        SleepEx (1, FALSE);
        spins = 0;
      }
    }

    for ( auto& it_res : it_ctx.temp_resources )
    {
      it_res->Release ();
    }

    it_ctx.temp_resources.clear ();
    it_ctx.used_textures.clear  ();

    InterlockedExchange (&it_ctx.writing_, 0);
  }


  for (auto it : temp_resources)
    it->Release ();

  temp_resources.clear ();

  for (auto& it : SK_D3D11_RenderTargets)
    it.clear ();


  extern bool SK_D3D11_ShowShaderModDlg (void);

  if (! SK_D3D11_ShowShaderModDlg ())
    SK_D3D11_EnableMMIOTracking = false;

  SK_D3D11_TextureResampler.processFinished (pDev, pDevCtx, pTLS);
}


#define ShaderColorDecl(idx) {                                                \
  { ImGuiCol_Header,        ImColor::HSV ( (idx + 1) / 6.0f, 0.5f,  0.45f) }, \
  { ImGuiCol_HeaderHovered, ImColor::HSV ( (idx + 1) / 6.0f, 0.55f, 0.6f ) }, \
  { ImGuiCol_HeaderActive,  ImColor::HSV ( (idx + 1) / 6.0f, 0.6f,  0.6f ) } }


bool
SK_D3D11_ShaderModDlg (SK_TLS* pTLS = SK_TLS_Bottom ())
{
  static
  std::unordered_map < sk_shader_class, std::tuple < std::pair <ImGuiCol, ImColor>,
                                                     std::pair <ImGuiCol, ImColor>,
                                                     std::pair <ImGuiCol, ImColor> > >
    SK_D3D11_ShaderColors =
      { { sk_shader_class::Unknown,  ShaderColorDecl (-1) },
        { sk_shader_class::Vertex,   ShaderColorDecl ( 0) },
        { sk_shader_class::Pixel,    ShaderColorDecl ( 1) },
        { sk_shader_class::Geometry, ShaderColorDecl ( 2) },
        { sk_shader_class::Hull,     ShaderColorDecl ( 3) },
        { sk_shader_class::Domain,   ShaderColorDecl ( 4) },
        { sk_shader_class::Compute,  ShaderColorDecl ( 5) } };


  std::lock_guard <SK_Thread_CriticalSection> auto_lock_gp (cs_shader);
  std::lock_guard <SK_Thread_CriticalSection> auto_lock_vs (cs_shader_vs);
  std::lock_guard <SK_Thread_CriticalSection> auto_lock_ps (cs_shader_ps);
  std::lock_guard <SK_Thread_CriticalSection> auto_lock_gs (cs_shader_gs);
  std::lock_guard <SK_Thread_CriticalSection> auto_lock_ds (cs_shader_ds);
  std::lock_guard <SK_Thread_CriticalSection> auto_lock_hs (cs_shader_hs);
  std::lock_guard <SK_Thread_CriticalSection> auto_lock_cs (cs_shader_cs);


  ImGuiIO& io (ImGui::GetIO ());

  const float font_size           = ImGui::GetFont ()->FontSize * io.FontGlobalScale;
  const float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

  bool show_dlg = true;

  ImGui::SetNextWindowSize ( ImVec2 ( io.DisplaySize.x * 0.66f, io.DisplaySize.y * 0.42f ), ImGuiSetCond_Appearing);

  ImGui::SetNextWindowSizeConstraints ( /*ImVec2 (768.0f, 384.0f),*/
                                        ImVec2 ( io.DisplaySize.x * 0.16f, io.DisplaySize.y * 0.16f ),
                                        ImVec2 ( io.DisplaySize.x * 0.96f, io.DisplaySize.y * 0.96f ) );

  if ( ImGui::Begin ( SK_FormatString ( "D3D11 Render Mod Toolkit     (%lu/%lu Memory Mgmt Threads,  %lu/%lu Shader Mgmt Threads,  %lu/%lu Draw Threads,  %lu/%lu Compute Threads)###D3D11_RenderDebug",
                                          SK_D3D11_MemoryThreads.count_active         (), SK_D3D11_MemoryThreads.count_all   (),
                                            SK_D3D11_ShaderThreads.count_active       (), SK_D3D11_ShaderThreads.count_all   (),
                                              SK_D3D11_DrawThreads.count_active       (), SK_D3D11_DrawThreads.count_all     (),
                                                SK_D3D11_DispatchThreads.count_active (), SK_D3D11_DispatchThreads.count_all () ).c_str (),
                        &show_dlg,
                          ImGuiWindowFlags_ShowBorders ) )
  {
    SK_D3D11_EnableTracking = true;

    bool can_scroll = ImGui::IsWindowFocused () && ImGui::IsMouseHoveringRect ( ImVec2 (ImGui::GetWindowPos ().x,                             ImGui::GetWindowPos ().y),
                                                                                ImVec2 (ImGui::GetWindowPos ().x + ImGui::GetWindowSize ().x, ImGui::GetWindowPos ().y + ImGui::GetWindowSize ().y) );

    ImGui::PushItemWidth (ImGui::GetWindowWidth () * 0.666f);

    ImGui::Columns (2);

    ImGui::BeginChild ( ImGui::GetID ("Render_Left_Side"), ImVec2 (0,0), false,
                          ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

    if (ImGui::CollapsingHeader ("Live Shader View", ImGuiTreeNodeFlags_DefaultOpen))
    {
      // This causes the stats below to update
      SK_ImGui_Widgets.d3d11_pipeline->setActive (true);

      ImGui::TreePush ("");

      auto ShaderClassMenu = [&](sk_shader_class shader_type) ->
        void
        {
          bool        used_last_frame       = false;
          bool        ui_link_activated     = false;
          char        label           [512] = { };
          wchar_t     wszPipelineDesc [512] = { };

          switch (shader_type)
          {
            case sk_shader_class::Vertex:   ui_link_activated = change_sel_vs != 0x00;
                                            used_last_frame   = SK_D3D11_Shaders.vertex.changes_last_frame > 0;
                                            //SK_D3D11_GetVertexPipelineDesc (wszPipelineDesc);
                                            sprintf (label,     "Vertex Shaders\t\t%ws###LiveVertexShaderTree", wszPipelineDesc);
              break;
            case sk_shader_class::Pixel:    ui_link_activated = change_sel_ps != 0x00;
                                            used_last_frame   = SK_D3D11_Shaders.pixel.changes_last_frame > 0;
                                            //SK_D3D11_GetRasterPipelineDesc (wszPipelineDesc);
                                            //lstrcatW                       (wszPipelineDesc, L"\t\t");
                                            //SK_D3D11_GetPixelPipelineDesc  (wszPipelineDesc);
                                            sprintf (label,     "Pixel Shaders\t\t%ws###LivePixelShaderTree", wszPipelineDesc);
              break;
            case sk_shader_class::Geometry: ui_link_activated = change_sel_gs != 0x00;
                                            used_last_frame   = SK_D3D11_Shaders.geometry.changes_last_frame > 0;
                                            //SK_D3D11_GetGeometryPipelineDesc (wszPipelineDesc);
                                            sprintf (label,     "Geometry Shaders\t\t%ws###LiveGeometryShaderTree", wszPipelineDesc);
              break;
            case sk_shader_class::Hull:     ui_link_activated = change_sel_hs != 0x00;
                                            used_last_frame   = SK_D3D11_Shaders.hull.changes_last_frame > 0;
                                            //SK_D3D11_GetTessellationPipelineDesc (wszPipelineDesc);
                                            sprintf (label,     "Hull Shaders\t\t%ws###LiveHullShaderTree", wszPipelineDesc);
              break;
            case sk_shader_class::Domain:   ui_link_activated = change_sel_ds != 0x00;
                                            used_last_frame   = SK_D3D11_Shaders.domain.changes_last_frame > 0;
                                            //SK_D3D11_GetTessellationPipelineDesc (wszPipelineDesc);
                                            sprintf (label,     "Domain Shaders\t\t%ws###LiveDomainShaderTree", wszPipelineDesc);
              break;
            case sk_shader_class::Compute:  ui_link_activated = change_sel_cs != 0x00;
                                            used_last_frame   = SK_D3D11_Shaders.compute.changes_last_frame > 0;
                                            //SK_D3D11_GetComputePipelineDesc (wszPipelineDesc);
                                            sprintf (label,     "Compute Shaders\t\t%ws###LiveComputeShaderTree", wszPipelineDesc);
              break;
            default:
              break;
          }

          if (used_last_frame)
          {
            if (ui_link_activated)
              ImGui::SetNextTreeNodeOpen (true, ImGuiSetCond_Always);

            ImGui::PushStyleColor ( std::get <0> (SK_D3D11_ShaderColors [shader_type]).first,
                                    std::get <0> (SK_D3D11_ShaderColors [shader_type]).second );
            ImGui::PushStyleColor ( std::get <1> (SK_D3D11_ShaderColors [shader_type]).first,
                                    std::get <1> (SK_D3D11_ShaderColors [shader_type]).second );
            ImGui::PushStyleColor ( std::get <2> (SK_D3D11_ShaderColors [shader_type]).first,
                                    std::get <2> (SK_D3D11_ShaderColors [shader_type]).second );

            if (ImGui::CollapsingHeader (label))
            {
              SK_LiveShaderClassView (shader_type, can_scroll);
            }

            ImGui::PopStyleColor (3);
          }
        };

        ImGui::TreePush    ("");
        ImGui::PushFont    (io.Fonts->Fonts [1]); // Fixed-width font
        ImGui::TextColored (ImColor (238, 250, 5), "%ws", SK::DXGI::getPipelineStatsDesc ().c_str ());
        ImGui::PopFont     ();

        ImGui::Separator   ();

        if (ImGui::Button (" Clear Shader State "))
        {
          SK_D3D11_ClearShaderState ();
        }

        ImGui::SameLine ();

        if (ImGui::Button (" Store Shader State "))
        {
          SK_D3D11_StoreShaderState ();
        }

        ImGui::SameLine ();

        if (ImGui::Button (" Add to Shader State "))
        {
          SK_D3D11_LoadShaderState (false);
        }

        ImGui::SameLine ();

        if (ImGui::Button (" Restore FULL Shader State "))
        {
          SK_D3D11_LoadShaderState ();
        }

        ImGui::SameLine ();
        ImGui::Checkbox ("Convert typeless resource views", &convert_typeless);

        ImGui::TreePop     ();

        ShaderClassMenu (sk_shader_class::Vertex);
        ShaderClassMenu (sk_shader_class::Pixel);
        ShaderClassMenu (sk_shader_class::Geometry);
        ShaderClassMenu (sk_shader_class::Hull);
        ShaderClassMenu (sk_shader_class::Domain);
        ShaderClassMenu (sk_shader_class::Compute);

        ImGui::TreePop ();
      }

      auto FormatNumber = [](int num) ->
        const char*
        {
          static char szNumber       [16] = { };
          static char szPrettyNumber [32] = { };

          const NUMBERFMTA fmt = { 0, 0, 3, ".", ",", 0 };

          snprintf (szNumber, 15, "%li", num);

          GetNumberFormatA ( MAKELCID (LOCALE_USER_DEFAULT, SORT_DEFAULT),
                               0x00,
                                 szNumber, &fmt,
                                   szPrettyNumber, 32 );

          return szPrettyNumber;
        };

      if (ImGui::CollapsingHeader ("Live Memory View", ImGuiTreeNodeFlags_DefaultOpen))
      {
        SK_D3D11_EnableMMIOTracking = true;
        ////std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_mmio);

        ImGui::BeginChild ( ImGui::GetID ("Render_MemStats_D3D11"), ImVec2 (0, 0), false, ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus |  ImGuiWindowFlags_AlwaysAutoResize );

        ImGui::TreePush   (""                      );
        ImGui::BeginGroup (                        );
        ImGui::BeginGroup (                        );
        ImGui::TextColored(ImColor (0.9f, 1.0f, 0.15f, 1.0f), "Mapped Memory"  );
        ImGui::TreePush   (""                      );
        ImGui::Text       ("Read-Only:            ");
        ImGui::Text       ("Write-Only:           ");
        ImGui::Text       ("Read-Write:           ");
        ImGui::Text       ("Write (Discard):      ");
        ImGui::Text       ("Write (No Overwrite): ");
        ImGui::Text       (""               );
        ImGui::TreePop    (                        );
        ImGui::TextColored(ImColor (0.9f, 1.0f, 0.15f, 1.0f), "Resource Types"  );
        ImGui::TreePush   (""               );
        ImGui::Text       ("Unknown:       ");
        ImGui::Text       ("Buffers:       ");
        ImGui::TreePush   (""               );
        ImGui::Text       ("Index:         ");
        ImGui::Text       ("Vertex:        ");
        ImGui::Text       ("Constant:      ");
        ImGui::TreePop    (                 );
        ImGui::Text       ("Textures:      ");
        ImGui::TreePush   (""               );
        ImGui::Text       ("Textures (1D): ");
        ImGui::Text       ("Textures (2D): ");
        ImGui::Text       ("Textures (3D): ");
        ImGui::TreePop    (                 );
        ImGui::Text       (""               );
        ImGui::TreePop    (                 );
        ImGui::TextColored(ImColor (0.9f, 1.0f, 0.15f, 1.0f), "Memory Totals"  );
        ImGui::TreePush   (""               );
        ImGui::Text       ("Bytes Read:    ");
        ImGui::Text       ("Bytes Written: ");
        ImGui::Text       ("Bytes Copied:  ");
        ImGui::TreePop    (                 );
        ImGui::EndGroup   (                 );

        ImGui::SameLine   (                        );

        ImGui::BeginGroup (                        );
        ImGui::Text       (""                      );
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.map_types [0]));
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.map_types [1]));
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.map_types [2]));
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.map_types [3]));
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.map_types [4]));
        ImGui::Text       (""                      );
        ImGui::Text       (""                      );
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.resource_types [0]));
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.resource_types [1]));
        ImGui::TreePush   (""                      );
        ImGui::Text       ("%s",     FormatNumber ((int)mem_map_stats.last_frame.index_buffers.size    ()));
        ImGui::Text       ("%s",     FormatNumber ((int)mem_map_stats.last_frame.vertex_buffers.size   ()));
        ImGui::Text       ("%s",     FormatNumber ((int)mem_map_stats.last_frame.constant_buffers.size ()));
        ImGui::TreePop    (                        );
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.resource_types [2] +
                                                   mem_map_stats.last_frame.resource_types [3] +
                                                   mem_map_stats.last_frame.resource_types [4]));
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.resource_types [2]));
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.resource_types [3]));
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.resource_types [4]));
        ImGui::Text       (""                      );
        ImGui::Text       (""                      );

        if ((double)mem_map_stats.last_frame.bytes_read < (0.75f * 1024.0 * 1024.0))
          ImGui::Text     ("( %06.2f KiB )", (double)mem_map_stats.last_frame.bytes_read    / (1024.0));
        else
          ImGui::Text     ("( %06.2f MiB )", (double)mem_map_stats.last_frame.bytes_read    / (1024.0 * 1024.0));

        if ((double)mem_map_stats.last_frame.bytes_written < (0.75f * 1024.0 * 1024.0))
          ImGui::Text     ("( %06.2f KiB )", (double)mem_map_stats.last_frame.bytes_written / (1024.0));
        else
          ImGui::Text     ("( %06.2f MiB )", (double)mem_map_stats.last_frame.bytes_written / (1024.0 * 1024.0));

        if ((double)mem_map_stats.last_frame.bytes_copied < (0.75f * 1024.0 * 1024.0))
          ImGui::Text     ("( %06.2f KiB )", (double)mem_map_stats.last_frame.bytes_copied / (1024.0));
        else
          ImGui::Text     ("( %06.2f MiB )", (double)mem_map_stats.last_frame.bytes_copied / (1024.0 * 1024.0));

        ImGui::EndGroup   (                        );
        
        ImGui::SameLine   (                        );

        ImGui::BeginGroup (                        );
        ImGui::Text       (""                      );
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.map_types [0]));
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.map_types [1]));
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.map_types [2]));
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.map_types [3]));
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.map_types [4]));
        ImGui::Text       (""                      );
        ImGui::Text       (""                      );
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.resource_types [0]));
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.resource_types [1]));
        ImGui::Text       ("");
        ImGui::Text       ("");
        ImGui::Text       ("");
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.resource_types [2] +
                                                  mem_map_stats.lifetime.resource_types [3] +
                                                  mem_map_stats.lifetime.resource_types [4]));
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.resource_types [2]));
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.resource_types [3]));
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.resource_types [4]));
        ImGui::Text       (""                      );
        ImGui::Text       (""                      );

        if ((double)mem_map_stats.lifetime.bytes_read < (0.75f * 1024.0 * 1024.0 * 1024.0))
          ImGui::Text     (" / %06.2f MiB", (double)mem_map_stats.lifetime.bytes_read    / (1024.0 * 1024.0));
        else
          ImGui::Text     (" / %06.2f GiB", (double)mem_map_stats.lifetime.bytes_read    / (1024.0 * 1024.0 * 1024.0));

        if ((double)mem_map_stats.lifetime.bytes_written < (0.75f * 1024.0 * 1024.0 * 1024.0))
          ImGui::Text     (" / %06.2f MiB", (double)mem_map_stats.lifetime.bytes_written / (1024.0 * 1024.0));
        else
          ImGui::Text     (" / %06.2f GiB", (double)mem_map_stats.lifetime.bytes_written / (1024.0 * 1024.0 * 1024.0));

        if ((double)mem_map_stats.lifetime.bytes_copied < (0.75f * 1024.0 * 1024.0 * 1024.0))
          ImGui::Text     (" / %06.2f MiB", (double)mem_map_stats.lifetime.bytes_copied / (1024.0 * 1024.0));
        else
          ImGui::Text     (" / %06.2f GiB", (double)mem_map_stats.lifetime.bytes_copied / (1024.0 * 1024.0 * 1024.0));

        ImGui::EndGroup   (                        );
        ImGui::EndGroup   (                        );
        ImGui::TreePop    (                        );
        ImGui::EndChild   ();
      }

      else
        SK_D3D11_EnableMMIOTracking = false;

      ImGui::EndChild   ();

      ImGui::NextColumn ();

      ImGui::BeginChild ( ImGui::GetID ("Render_Right_Side"), ImVec2 (0, 0), false, 
                            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened | ImGuiWindowFlags_NoScrollbar );

      static bool uncollapsed_tex = true;
      static bool uncollapsed_rtv = true;

      float scale = (uncollapsed_tex ? 1.0f * (uncollapsed_rtv ? 0.5f : 1.0f) : -1.0f);

      ImGui::BeginChild     ( ImGui::GetID ("Live_Texture_View_Panel"),
                              ImVec2 ( -1.0f, scale == -1.0f ? font_size_multiline * 1.666f : ( ImGui::GetWindowContentRegionMax ().y - ImGui::GetWindowContentRegionMin ().y ) * scale - (scale == 1.0f ? font_size_multiline * 1.666f : 0.0f) ),
                                true,
                                  ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

      uncollapsed_tex = 
        ImGui::CollapsingHeader ( "Live Texture View",
                                  config.textures.d3d11.cache ? ImGuiTreeNodeFlags_DefaultOpen :
                                                                0x0 );

      if (! config.textures.d3d11.cache)
      {
        ImGui::SameLine    ();
        ImGui::TextColored (ImColor::HSV (0.15f, 1.0f, 1.0f), "\t(Unavailable because Texture Caching is not enabled!)");
      }

      uncollapsed_tex = uncollapsed_tex && config.textures.d3d11.cache;

      if (uncollapsed_tex)
      {
        static bool warned_invalid_ref_count = false;

        if ((! warned_invalid_ref_count) && ReadAcquire (&__SK_D3D11_TexRefCount_Failures) > 0)
        {
          SK_ImGui_Warning ( L"The game's graphics engine is not correctly tracking texture memory.\n\n"
                             L"\t\t\t\t>> Texture mod support has been partially disabled to prevent memory leaks.\n\n"
                             L"\t\tYou may force support for texture mods by setting AllowUnsafeRefCounting=true" );

          warned_invalid_ref_count = true;
        }

        SK_LiveTextureView (can_scroll, pTLS);
      }

      ImGui::EndChild ();

      scale = (live_rt_view ? (1.0f * (uncollapsed_tex ? 0.5f : 1.0f)) : -1.0f);

      ImGui::BeginChild     ( ImGui::GetID ("Live_RenderTarget_View_Panel"),
                              ImVec2 ( -1.0f, scale == -1.0f ? font_size_multiline * 1.666f : ( ImGui::GetWindowContentRegionMax ().y - ImGui::GetWindowContentRegionMin ().y ) * scale - (scale == 1.0f ? font_size_multiline * 1.666f : 0.0f) ),
                                true,
                                  ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

      live_rt_view =
          ImGui::CollapsingHeader ("Live RenderTarget View", ImGuiTreeNodeFlags_DefaultOpen);

      if (live_rt_view)
      {
        //SK_AutoCriticalSection auto_cs_rv (&cs_render_view, true);

        //if (auto_cs2.try_result ())
        {
        static float last_ht    = 256.0f;
        static float last_width = 256.0f;

        static std::vector <std::string> list_contents;
        static bool                      list_dirty     = true;
        static uintptr_t                 last_sel_ptr   =    0;
        static size_t                    sel            = std::numeric_limits <size_t>::max ();
        static bool                      first_frame    = true;

        std::set <ID3D11RenderTargetView *> live_textures;

        struct lifetime
        {
          ULONG last_frame;
          ULONG frames_active;
        };

        static std::unordered_map <ID3D11RenderTargetView *, lifetime> render_lifetime;
        static std::vector        <ID3D11RenderTargetView *>           render_textures;

        //render_textures.reserve (128);
        //render_textures.clear   ();

        for (auto&& rtl : SK_D3D11_RenderTargets )
        for (auto&& it  : rtl.rt_views           )
        {
          if (it == nullptr)
            continue;

          D3D11_RENDER_TARGET_VIEW_DESC desc;
          it->GetDesc (&desc);

          if (desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D)
          {
            CComPtr   <ID3D11Resource>        pRes = nullptr;
                            it->GetResource (&pRes.p);
            CComQIPtr <ID3D11Texture2D> pTex (pRes);

            if (pRes && pTex)
            {
              D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;

              srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
              srv_desc.Format                    = desc.Format;
              srv_desc.Texture2D.MipLevels       = desc.Texture2D.MipSlice + 1;
              srv_desc.Texture2D.MostDetailedMip = desc.Texture2D.MipSlice;

              CComQIPtr <ID3D11Device> pDev (SK_GetCurrentRenderBackend ().device);

              if (pDev != nullptr)
              {
                if (! render_lifetime.count (it))
                {
                  lifetime life;

                  life.frames_active = 1;
                  life.last_frame    = SK_GetFramesDrawn ();

                  render_textures.push_back (it);
                  render_lifetime.emplace   (std::make_pair (it, life));
                }

                else {
                  render_lifetime [it].frames_active++;
                  render_lifetime [it].last_frame = SK_GetFramesDrawn ();
                }

                live_textures.emplace (it);
              }
            }
          }
        }

        const ULONG      zombie_threshold = 120;
        static ULONG last_zombie_pass      = SK_GetFramesDrawn ();

        if (last_zombie_pass < SK_GetFramesDrawn () - zombie_threshold / 2)
        {
          bool newly_dead = false;

          for (auto it : render_textures)
          {
            if (render_lifetime [it].last_frame < SK_GetFramesDrawn () - zombie_threshold)
            {
              render_lifetime.erase (it);
              newly_dead = true;
            }
          }

          if (newly_dead)
          {
            render_textures.clear ();

            for (auto& it : render_lifetime)
              render_textures.push_back (it.first);
          }

          last_zombie_pass = SK_GetFramesDrawn ();
        }


        if (list_dirty)
        {
              sel =  std::numeric_limits <size_t>::max ();
          int idx =  0;
              list_contents.clear ();

          // The underlying list is unsorted for speed, but that's not at all
          //   intuitive to humans, so sort the thing when we have the RT view open.
          std::sort ( render_textures.begin (),
                      render_textures.end   (),
            []( ID3D11RenderTargetView *a,
                ID3D11RenderTargetView *b )
            {
              return (uintptr_t)a < (uintptr_t)b;
            }
          );


          for ( auto it : render_textures )
          {
            static char szDesc [48] = { };

            sprintf (szDesc, "%07" PRIxPTR "###rtv_%p", (uintptr_t)it, it);

            list_contents.emplace_back (szDesc);
  
            if ((uintptr_t)it == last_sel_ptr)
            {
              sel = idx;
              //tbf::RenderFix::tracked_rt.tracking_tex = render_textures [sel];
            }
  
            ++idx;
          }
        }
  
        static bool hovered = false;
        static bool focused = false;
  
        if (hovered || focused)
        {
          can_scroll = false;
  
          if (!render_textures.empty ())
          {
            if (! focused)//hovered)
            {
              ImGui::BeginTooltip ();
              ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "You can view the output of individual render passes");
              ImGui::Separator    ();
              ImGui::BulletText   ("Press %ws while the mouse is hovering this list to select the previous output", virtKeyCodeToHumanKeyName [VK_OEM_4].c_str ());
              ImGui::BulletText   ("Press %ws while the mouse is hovering this list to select the next output",     virtKeyCodeToHumanKeyName [VK_OEM_6].c_str ());
              ImGui::EndTooltip   ();
            }

            else
            {
              ImGui::BeginTooltip ();
              ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "You can view the output of individual render passes");
              ImGui::Separator    ();
              ImGui::BulletText   ("Press LB to select the previous output");
              ImGui::BulletText   ("Press RB to select the next output");
              ImGui::EndTooltip   ();
            }
  
            int direction = 0;
  
                 if (io.KeysDown [VK_OEM_4] && io.KeysDownDuration [VK_OEM_4] == 0.0f) { direction--;  io.WantCaptureKeyboard = true; }
            else if (io.KeysDown [VK_OEM_6] && io.KeysDownDuration [VK_OEM_6] == 0.0f) { direction++;  io.WantCaptureKeyboard = true; }
  
            else {
                  if  (io.NavInputs [ImGuiNavInput_PadFocusPrev] && io.NavInputsDownDuration [ImGuiNavInput_PadFocusPrev] == 0.0f) { direction--; }
              else if (io.NavInputs [ImGuiNavInput_PadFocusNext] && io.NavInputsDownDuration [ImGuiNavInput_PadFocusNext] == 0.0f) { direction++; }
            }
  
            int neutral_idx = 0;
  
            for (UINT i = 0; i < render_textures.size (); i++)
            {
              if ((uintptr_t)render_textures [i] >= last_sel_ptr)
              {
                neutral_idx = i;
                break;
              }
            }
  
            sel = neutral_idx + direction;

            if ((SSIZE_T)sel <  0) sel = 0;
  
            if ((ULONG)sel >= (ULONG)render_textures.size ())
            {
              sel = render_textures.size () - 1;
            }

            if ((SSIZE_T)sel <  0) sel = 0;
          }
        }
  
        ImGui::BeginGroup     ();
        ImGui::PushStyleVar   (ImGuiStyleVar_ChildWindowRounding, 0.0f);
        ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.9f, 0.7f, 0.5f, 1.0f));
  
        ImGui::BeginChild ( ImGui::GetID ("RenderTargetViewList"),
                            ImVec2 ( font_size * 7.0f, -1.0f),
                              true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );
  
        if (! render_textures.empty ())
        {
          ImGui::BeginGroup ();
  
          if (first_frame)
          {
            sel         = 0;
            first_frame = false;
          }
  
          static bool sel_changed  = false;
  
          if ((SSIZE_T)sel >= 0 && sel < (int)render_textures.size ())
          {
            if (last_sel_ptr != (uintptr_t)render_textures [sel])
              sel_changed = true;
          }
  
          for ( UINT line = 0; line < render_textures.size (); line++ )
          {      
            if (line == sel)
            {
              bool selected = true;
              ImGui::Selectable (list_contents [line].c_str (), &selected);
  
              if (sel_changed)
              {
                ImGui::SetScrollHere        (0.5f);
                ImGui::SetKeyboardFocusHere (    );

                sel_changed  = false;
                last_sel_ptr = (uintptr_t)render_textures [sel];
                tracked_rtv.resource =    render_textures [sel];
              }
            }
  
            else
            {
              bool selected = false;
  
              if (ImGui::Selectable (list_contents [line].c_str (), &selected))
              {
                if (selected)
                {
                  sel_changed          = true;
                  sel                  =  line;
                  last_sel_ptr         = (uintptr_t)render_textures [sel];
                  tracked_rtv.resource =            render_textures [sel];
                }
              }
            }
          }
  
          ImGui::EndGroup ();
        }
  
        ImGui::EndChild      ();
        ImGui::PopStyleColor ();
        ImGui::PopStyleVar   ();
        ImGui::EndGroup      ();


        if (ImGui::IsItemHoveredRect ())
        {
          if (ImGui::IsItemHovered ()) hovered = true; else hovered = false;
          if (ImGui::IsItemFocused ()) focused = true; else focused = false;
        }

        else
        {
          hovered = false; focused = false;
        }
 
  
        if (render_textures.size () > (size_t)sel && live_textures.count (render_textures [sel]))
        {
          ID3D11RenderTargetView* rt_view = render_textures [sel];
  
          D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = { };
          rt_view->GetDesc            (&rtv_desc);
  
          if (rtv_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D)
          {
            CComPtr <ID3D11Resource>          pRes = nullptr;
            rt_view->GetResource            (&pRes.p);
            CComQIPtr <ID3D11Texture2D> pTex (pRes);

            if (pRes && pTex)
            {
              D3D11_TEXTURE2D_DESC desc = { };
              pTex->GetDesc      (&desc);

              ID3D11ShaderResourceView*       pSRV     = nullptr;
              D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = { };
  
              srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
              srv_desc.Format                    = rtv_desc.Format;
              srv_desc.Texture2D.MipLevels       = (UINT)-1;
              srv_desc.Texture2D.MostDetailedMip =  0;

              CComQIPtr <ID3D11Device> pDev (SK_GetCurrentRenderBackend ().device);

              if (pDev != nullptr)
              {
                size_t row0  = std::max (tracked_rtv.ref_vs.size (), tracked_rtv.ref_ps.size ());
                size_t row1  =           tracked_rtv.ref_gs.size ();
                size_t row2  = std::max (tracked_rtv.ref_hs.size (), tracked_rtv.ref_ds.size ());
                size_t row3  =           tracked_rtv.ref_cs.size ();

                size_t bottom_list = row0 + row1 + row2 + row3;

                bool success =
                  SUCCEEDED (pDev->CreateShaderResourceView (pTex, &srv_desc, &pSRV));
  
                float content_avail_y = ImGui::GetWindowContentRegionMax ().y - ImGui::GetWindowContentRegionMin ().y;
                float content_avail_x = ImGui::GetWindowContentRegionMax ().x - ImGui::GetWindowContentRegionMin ().x;
                float effective_width, effective_height;

                if (! success)
                {
                  effective_width  = 0;
                  effective_height = 0;
                }

                else
                {
                  // Some Render Targets are MASSIVE, let's try to keep the damn things on the screen ;)
                  if (bottom_list > 0)
                    effective_height =           std::max (256.0f, content_avail_y - ((float)(bottom_list + 4) * font_size_multiline * 1.125f));
                  else
                    effective_height = std::max (256.0f, std::max (content_avail_y, (float)desc.Height));

                  effective_width    = effective_height  * ((float)desc.Width / (float)desc.Height );

                  if (effective_width > content_avail_x)
                  {
                    effective_width  = std::max (content_avail_x, 256.0f);
                    effective_height = effective_width * ((float)desc.Height / (float)desc.Width);
                  }
                }
  
                ImGui::SameLine ();
 
                ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.5f, 0.5f, 0.5f, 1.0f));
                ImGui::BeginChild      ( ImGui::GetID ("RenderTargetPreview"),
                                         ImVec2 ( -1.0f, -1.0f ),
                                           true,
                                             ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );
  
                last_width  = content_avail_x;//effective_width;
                last_ht     = content_avail_y;//effective_height + ( font_size_multiline * (bottom_list + 4) * 1.125f );
  
                ImGui::BeginGroup (                  );
                ImGui::Text       ( "Dimensions:   " );
                ImGui::Text       ( "Format:       " );
                ImGui::EndGroup   (                  );
  
                ImGui::SameLine   ( );
  
                ImGui::BeginGroup (                                              );
                ImGui::Text       ( "%lux%lu",
                                      desc.Width, desc.Height/*, effective_width, effective_height, 0.9875f * content_avail_y - ((float)(bottom_list + 3) * font_size * 1.125f), content_avail_y*//*,
                                        pTex->d3d9_tex->GetLevelCount ()*/       );
                ImGui::Text       ( "%ws",
                                      SK_DXGI_FormatToStr (desc.Format).c_str () );
                ImGui::EndGroup   (                                              );
    
                if (success && pSRV != nullptr)
                {
                  ImGui::Separator  ( );

                  ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));
                  ImGui::BeginChildFrame   (ImGui::GetID ("ShaderResourceView_Frame"),
                                              ImVec2 (effective_width + 8.0f, effective_height + 8.0f),
                                              ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders );

                  temp_resources.push_back (rt_view);
                                            rt_view->AddRef ();
                  temp_resources.push_back (pSRV);
                  ImGui::Image             ( pSRV,
                                               ImVec2 (effective_width, effective_height),
                                                 ImVec2  (0,0),             ImVec2  (1,1),
                                                 ImColor (255,255,255,255), ImColor (255,255,255,128)
                                           );

#if 0
                  if (ImGui::IsItemHovered ())
                  {
                    ImGui::BeginTooltip ();
                    ImGui::BeginGroup   ();
                    ImGui::TextUnformatted ("Mip Levels:   ");
                    if (desc.SampleDesc.Count > 1)
                    {
                      ImGui::TextUnformatted ("Sample Count: ");
                      ImGui::TextUnformatted ("MSAA Quality: ");
                    }
                    ImGui::TextUnformatted ("Usage:        ");
                    ImGui::TextUnformatted ("Bind Flags:   ");
                    ImGui::TextUnformatted ("CPU Access:   ");
                    ImGui::TextUnformatted ("Misc Flags:   ");
                    ImGui::EndGroup     ();

                    ImGui::SameLine     ();

                    ImGui::BeginGroup   ();
                    ImGui::Text ("%u", desc.MipLevels);
                    if (desc.SampleDesc.Count > 1)
                    {
                      ImGui::Text ("%u", desc.SampleDesc.Count);
                      ImGui::Text ("%u", desc.SampleDesc.Quality);
                    }
                    ImGui::Text (      "%ws", SK_D3D11_DescribeUsage (desc.Usage));
                    ImGui::Text ("%u (  %ws)", desc.BindFlags,
                                            SK_D3D11_DescribeBindFlags (
                      (D3D11_BIND_FLAG)desc.BindFlags).c_str ());
                    ImGui::Text ("%x", desc.CPUAccessFlags);
                    ImGui::Text ("%x", desc.MiscFlags);
                    ImGui::EndGroup   ();
                    ImGui::EndTooltip ();
                  }
#endif

                  ImGui::EndChildFrame     (    );
                  ImGui::PopStyleColor     (    );
                }


                // XXX: When you're done being stupid and writing the same code for every type of shader,
                //        go and do some actual design work.
                bool selected = false;
  
                if (bottom_list)
                {
                  ImGui::Separator  ( );

                  ImGui::BeginChild ( ImGui::GetID ("RenderTargetContributors"),
                                    ImVec2 ( -1.0f ,//std::max (font_size * 30.0f, effective_width + 24.0f),
                                             -1.0f ),
                                      true,
                                        ImGuiWindowFlags_AlwaysAutoResize |
                                        ImGuiWindowFlags_NavFlattened );

                  auto _SetupShaderHeaderColors =
                  [&](sk_shader_class type)
                  {
                    ImGui::PushStyleColor ( std::get <0> (SK_D3D11_ShaderColors [type]).first,
                                            std::get <0> (SK_D3D11_ShaderColors [type]).second );
                    ImGui::PushStyleColor ( std::get <1> (SK_D3D11_ShaderColors [type]).first,
                                            std::get <1> (SK_D3D11_ShaderColors [type]).second );
                    ImGui::PushStyleColor ( std::get <2> (SK_D3D11_ShaderColors [type]).first,
                                            std::get <2> (SK_D3D11_ShaderColors [type]).second );
                  };
  
                  if ((! tracked_rtv.ref_vs.empty ()) || (! tracked_rtv.ref_ps.empty ()))
                  {
                    ImGui::Columns (2);
  

                    _SetupShaderHeaderColors (sk_shader_class::Vertex);

                    if (ImGui::CollapsingHeader ("Vertex Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");
  
                      for ( auto it : tracked_rtv.ref_vs )
                      {
                        if (IsSkipped (sk_shader_class::Vertex, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, SkipColorCycle ());
                        }

                        else if (IsOnTop (sk_shader_class::Vertex, it))
                        {
                         ImGui::PushStyleColor (ImGuiCol_Text, OnTopColorCycle ());
                        }

                        else if (IsWireframe (sk_shader_class::Vertex, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, WireframeColorCycle ());
                        }

                        else
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.95f, 0.95f, 0.95f, 1.0f));
                        }

                        bool disabled = SK_D3D11_Shaders.vertex.blacklist.count (it) != 0;
  
                        if (ImGui::Selectable (SK_FormatString ("%s%08lx##vs", disabled ? "*" : " ", it).c_str (), &selected))
                        {
                          change_sel_vs = it;
                        }

                        ImGui::PopStyleColor ();
  
                        if (SK_ImGui_IsItemRightClicked ())
                        {
                          ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
                          ImGui::OpenPopup         (SK_FormatString ("ShaderSubMenu_VS%08lx", it).c_str ());
                        }
  
                        if (ImGui::BeginPopup (SK_FormatString ("ShaderSubMenu_VS%08lx", it).c_str ()))
                        {
                          ShaderMenu (SK_D3D11_Shaders.vertex.blacklist, SK_D3D11_Shaders.vertex.blacklist_if_texture, SK_D3D11_Shaders.vertex.tracked.used_views, SK_D3D11_Shaders.vertex.tracked.set_of_views, it);
                          ImGui::EndPopup ();
                        }
                      }
  
                      ImGui::TreePop ();
                    }

                    ImGui::PopStyleColor (3);
  
                    ImGui::NextColumn ();
  
                    _SetupShaderHeaderColors (sk_shader_class::Pixel);

                    if (ImGui::CollapsingHeader ("Pixel Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");
  
                      for ( auto it : tracked_rtv.ref_ps )
                      {
                        if (IsSkipped (sk_shader_class::Pixel, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, SkipColorCycle ());
                        }

                        else if (IsOnTop (sk_shader_class::Pixel, it))
                        {
                         ImGui::PushStyleColor (ImGuiCol_Text, OnTopColorCycle ());
                        }

                        else if (IsWireframe (sk_shader_class::Pixel, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, WireframeColorCycle ());
                        }

                        else
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.95f, 0.95f, 0.95f, 1.0f));
                        }

                        bool disabled = SK_D3D11_Shaders.pixel.blacklist.count (it) != 0;
  
                        if (ImGui::Selectable (SK_FormatString ("%s%08lx##ps", disabled ? "*" : " ", it).c_str (), &selected))
                        {
                          change_sel_ps = it;
                        }

                        ImGui::PopStyleColor ();
  
                        if (SK_ImGui_IsItemRightClicked ())
                        {
                          ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
                          ImGui::OpenPopup         (SK_FormatString ("ShaderSubMenu_PS%08lx", it).c_str ());
                        }
  
                        if (ImGui::BeginPopup (SK_FormatString ("ShaderSubMenu_PS%08lx", it).c_str ()))
                        {
                          ShaderMenu (SK_D3D11_Shaders.pixel.blacklist, SK_D3D11_Shaders.pixel.blacklist_if_texture, SK_D3D11_Shaders.pixel.tracked.used_views, SK_D3D11_Shaders.pixel.tracked.set_of_views, it);
                          ImGui::EndPopup ();
                        }
                      }
  
                      ImGui::TreePop ();
                    }

                    ImGui::PopStyleColor (3);  
                    ImGui::Columns       (1);
                  }
  
                  if (! tracked_rtv.ref_gs.empty ())
                  {
                    ImGui::Separator ( );
  
                    _SetupShaderHeaderColors (sk_shader_class::Geometry);

                    if (ImGui::CollapsingHeader ("Geometry Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");
  
                      for ( auto it : tracked_rtv.ref_gs )
                      {
                        if (IsSkipped (sk_shader_class::Geometry, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, SkipColorCycle ());
                        }

                        else if (IsOnTop (sk_shader_class::Geometry, it))
                        {
                         ImGui::PushStyleColor (ImGuiCol_Text, OnTopColorCycle ());
                        }

                        else if (IsWireframe (sk_shader_class::Geometry, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, WireframeColorCycle ());
                        }

                        else
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.95f, 0.95f, 0.95f, 1.0f));
                        }

                        bool disabled = SK_D3D11_Shaders.geometry.blacklist.count (it) != 0;
  
                        if (ImGui::Selectable (SK_FormatString ("%s%08lx##gs", disabled ? "*" : " ", it).c_str (), &selected))
                        {
                          change_sel_gs = it;
                        }

                        ImGui::PopStyleColor ();
  
                        if (SK_ImGui_IsItemRightClicked ())
                        {
                          ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
                          ImGui::OpenPopup         (SK_FormatString ("ShaderSubMenu_GS%08lx", it).c_str ());
                        }
  
                        if (ImGui::BeginPopup (SK_FormatString ("ShaderSubMenu_GS%08lx", it).c_str ()))
                        {
                          ShaderMenu (SK_D3D11_Shaders.geometry.blacklist, SK_D3D11_Shaders.geometry.blacklist_if_texture, SK_D3D11_Shaders.geometry.tracked.used_views, SK_D3D11_Shaders.geometry.tracked.set_of_views, it);
                          ImGui::EndPopup ();
                        }
                      }
  
                      ImGui::TreePop ();
                    }

                    ImGui::PopStyleColor (3);
                  }
  
                  if ((! tracked_rtv.ref_hs.empty ()) || (! tracked_rtv.ref_ds.empty ()))
                  {
                    ImGui::Separator ( );
  
                    ImGui::Columns   (2);
  
                    _SetupShaderHeaderColors (sk_shader_class::Hull);

                    if (ImGui::CollapsingHeader ("Hull Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");
  
                      for ( auto it : tracked_rtv.ref_hs )
                      {
                        bool disabled = SK_D3D11_Shaders.hull.blacklist.count (it) != 0;

                        if (IsSkipped (sk_shader_class::Hull, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, SkipColorCycle ());
                        }

                        else if (IsOnTop (sk_shader_class::Hull, it))
                        {
                         ImGui::PushStyleColor (ImGuiCol_Text, OnTopColorCycle ());
                        }

                        else if (IsWireframe (sk_shader_class::Hull, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, WireframeColorCycle ());
                        }

                        else
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.95f, 0.95f, 0.95f, 1.0f));
                        }
  
                        if (ImGui::Selectable (SK_FormatString ("%s%08lx##hs", disabled ? "*" : " ", it).c_str (), &selected))
                        {
                          change_sel_hs = it;
                        }

                        ImGui::PopStyleColor ();
  
                        if (SK_ImGui_IsItemRightClicked ())
                        {
                          ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
                          ImGui::OpenPopup         (SK_FormatString ("ShaderSubMenu_HS%08lx", it).c_str ());
                        }
  
                        if (ImGui::BeginPopup (SK_FormatString ("ShaderSubMenu_HS%08lx", it).c_str ()))
                        {
                          ShaderMenu (SK_D3D11_Shaders.hull.blacklist, SK_D3D11_Shaders.hull.blacklist_if_texture, SK_D3D11_Shaders.hull.tracked.used_views, SK_D3D11_Shaders.hull.tracked.set_of_views, it);
                          ImGui::EndPopup ();
                        }
                      }
  
                      ImGui::TreePop ();
                    }

                    ImGui::PopStyleColor (3);
  
                    ImGui::NextColumn ();
  
                    _SetupShaderHeaderColors (sk_shader_class::Domain);

                    if (ImGui::CollapsingHeader ("Domain Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");
  
                      for ( auto it : tracked_rtv.ref_ds )
                      {
                        bool disabled = SK_D3D11_Shaders.domain.blacklist.count (it) != 0;

                        if (IsSkipped (sk_shader_class::Domain, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, SkipColorCycle ());
                        }

                        else if (IsOnTop (sk_shader_class::Domain, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, OnTopColorCycle ());
                        }

                        else if (IsWireframe (sk_shader_class::Domain, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, WireframeColorCycle ());
                        }

                        else
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.95f, 0.95f, 0.95f, 1.0f));
                        }
  
                        if (ImGui::Selectable (SK_FormatString ("%s%08lx##ds", disabled ? "*" : " ", it).c_str (), &selected))
                        {
                          change_sel_ds = it;
                        }

                        ImGui::PopStyleColor ();
  
                        if (SK_ImGui_IsItemRightClicked ())
                        {
                          ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
                          ImGui::OpenPopup         (SK_FormatString ("ShaderSubMenu_DS%08lx", it).c_str ());
                        }
  
                        if (ImGui::BeginPopup (SK_FormatString ("ShaderSubMenu_DS%08lx", it).c_str ()))
                        {
                          ShaderMenu (SK_D3D11_Shaders.domain.blacklist, SK_D3D11_Shaders.domain.blacklist_if_texture, SK_D3D11_Shaders.domain.tracked.used_views, SK_D3D11_Shaders.domain.tracked.set_of_views, it);
                          ImGui::EndPopup ();
                        }
                      }
  
                      ImGui::TreePop ();
                    }

                    ImGui::PopStyleColor (3); 
                    ImGui::Columns       (1);
                  }
  
                  if (! tracked_rtv.ref_cs.empty ())
                  {
                    ImGui::Separator ( );
  
                    _SetupShaderHeaderColors (sk_shader_class::Compute);

                    if (ImGui::CollapsingHeader ("Compute Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");
  
                      for ( auto it : tracked_rtv.ref_cs )
                      {
                        bool disabled =
                          (SK_D3D11_Shaders.compute.blacklist.count (it) != 0);

                        if (IsSkipped (sk_shader_class::Compute, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, SkipColorCycle ());
                        }

                        else
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.95f, 0.95f, 0.95f, 1.0f));
                        }
  
                        if (ImGui::Selectable (SK_FormatString ("%s%08lx##cs", disabled ? "*" : " ", it).c_str (), &selected))
                        {
                          change_sel_cs = it;
                        }

                        ImGui::PopStyleColor ();
  
                        if (SK_ImGui_IsItemRightClicked ())
                        {
                          ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
                          ImGui::OpenPopup         (SK_FormatString ("ShaderSubMenu_CS%08lx", it).c_str ());
                        }
  
                        if (ImGui::BeginPopup (SK_FormatString ("ShaderSubMenu_CS%08lx", it).c_str ()))
                        {
                          ShaderMenu (SK_D3D11_Shaders.compute.blacklist, SK_D3D11_Shaders.compute.blacklist_if_texture, SK_D3D11_Shaders.compute.tracked.used_views, SK_D3D11_Shaders.compute.tracked.set_of_views, it);
                          ImGui::EndPopup     (                                      );
                        }
                      }
  
                      ImGui::TreePop ( );
                    }

                    ImGui::PopStyleColor (3);
                  }
  
                  ImGui::EndChild    ( );
                }
  
                ImGui::EndChild      ( );
                ImGui::PopStyleColor (1);
              }
            }
          }
        }
        }
      }

      ImGui::EndChild     ( );
      ImGui::EndChild     ( );
      ImGui::Columns      (1);

      ImGui::PopItemWidth ( );
    }

  ImGui::End            ( );

  SK_D3D11_EnableTracking = show_dlg;

  return show_dlg;
}






// Not thread-safe, I mean this! Don't let the stupid critical section fool you;
//   if you import this and try to call it, your software will explode.
__declspec (dllexport)
void
__stdcall
SKX_ImGui_RegisterDiscardableResource (IUnknown* pRes)
{
  std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_render_view);
  temp_resources.push_back (pRes);
}



//std::array <bool, SK_D3D11_MAX_DEV_CONTEXTS+1A SK_D3D11_KnownShaders::reshade_triggered;







void
SK_D3D11_ResetShaders (void)
{
  for (auto& it : SK_D3D11_Shaders.vertex.descs)
  {
    if (it.second.pShader->Release () == 0)
    {
      SK_D3D11_Shaders.vertex.rev.erase   ((ID3D11VertexShader *)it.second.pShader);
      SK_D3D11_Shaders.vertex.descs.erase (it.first);
    }
  }

  for (auto& it : SK_D3D11_Shaders.pixel.descs)
  {
    if (it.second.pShader->Release () == 0)
    {
      SK_D3D11_Shaders.pixel.rev.erase   ((ID3D11PixelShader *)it.second.pShader);
      SK_D3D11_Shaders.pixel.descs.erase (it.first);
    }
  }

  for (auto& it : SK_D3D11_Shaders.geometry.descs)
  {
    if (it.second.pShader->Release () == 0)
    {
      SK_D3D11_Shaders.geometry.rev.erase   ((ID3D11GeometryShader *)it.second.pShader);
      SK_D3D11_Shaders.geometry.descs.erase (it.first);
    }
  }

  for (auto& it : SK_D3D11_Shaders.hull.descs)
  {
    if (it.second.pShader->Release () == 0)
    {
      SK_D3D11_Shaders.hull.rev.erase   ((ID3D11HullShader *)it.second.pShader);
      SK_D3D11_Shaders.hull.descs.erase (it.first);
    }
  }

  for (auto& it : SK_D3D11_Shaders.domain.descs)
  {
    if (it.second.pShader->Release () == 0)
    {
      SK_D3D11_Shaders.domain.rev.erase   ((ID3D11DomainShader *)it.second.pShader);
      SK_D3D11_Shaders.domain.descs.erase (it.first);
    }
  }

  for (auto& it : SK_D3D11_Shaders.compute.descs)
  {
    if (it.second.pShader->Release () == 0)
    {
      SK_D3D11_Shaders.compute.rev.erase   ((ID3D11ComputeShader *)it.second.pShader);
      SK_D3D11_Shaders.compute.descs.erase (it.first);
    }
  }
}


#if 0
class SK_ExecuteReShadeOnReturn
{
public:
   SK_ExecuteReShadeOnReturn (ID3D11DeviceContext* pCtx) : _ctx (pCtx) { };
  ~SK_ExecuteReShadeOnReturn (void)
  {
    auto TriggerReShade_After = [&]
    {
      SK_ScopedBool auto_bool (&SK_TLS_Bottom ()->imgui.drawing);
      SK_TLS_Bottom ()->imgui.drawing = true;

      if (SK_ReShade_PresentCallback.fn && (! SK_D3D11_Shaders.reshade_triggered))
      {
        CComPtr <ID3D11DepthStencilView>  pDSV = nullptr;
        CComPtr <ID3D11DepthStencilState> pDSS = nullptr;
        CComPtr <ID3D11RenderTargetView>  pRTV = nullptr;

        _ctx->OMGetRenderTargets (1, &pRTV, &pDSV);

        if (pRTV != nullptr)
        {
          D3D11_RENDER_TARGET_VIEW_DESC rt_desc = { };
                        pRTV->GetDesc (&rt_desc);

          if (rt_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D && rt_desc.Texture2D.MipSlice == 0)
          {
            CComPtr <ID3D11Resource> pRes = nullptr;
                 pRTV->GetResource (&pRes);

            CComQIPtr <ID3D11Texture2D> pTex (pRes);

            if (pTex)
            {
              D3D11_TEXTURE2D_DESC tex_desc = { };

              if ( ImGui::GetIO ().DisplaySize.x == tex_desc.Width &&
                   ImGui::GetIO ().DisplaySize.y == tex_desc.Height )
              {
                for (int i = 0 ; i < 5; i++)
                {
                  SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *>* pShaderReg;

                  switch (i)
                  {
                    default:
                    case 0:
                      pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.vertex;
                      break;
                    case 1:
                      pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.pixel;
                      break;
                    case 2:
                      pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.geometry;
                      break;
                    case 3:
                      pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.hull;
                      break;
                    case 4:
                      pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.domain;
                      break;
                  };

                  if ( pShaderReg->current.shader [_ctx] != 0x0    &&
                    (! pShaderReg->trigger_reshade.after.empty ()) &&
                       pShaderReg->trigger_reshade.after.count (pShaderReg->current.shader [_ctx]) )
                  {
                    SK_D3D11_Shaders.reshade_triggered = true;

                    SK_ReShade_PresentCallback.explicit_draw.calls++;
                    SK_ReShade_PresentCallback.fn (&SK_ReShade_PresentCallback.explicit_draw);
                    break;
                  }
                }
              }
            }
          }
        }
      }
    };

    TriggerReShade_After ();
  }

protected:
  ID3D11DeviceContext* _ctx;
};
#endif




__forceinline
const std::unordered_map <std::wstring, SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*>&
__SK_Singleton_D3D11_ShaderClassMap (void)
{
  static const
  std::unordered_map <std::wstring, SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*>
    SK_D3D11_ShaderClassMap_ =
    {
      std::make_pair (L"Vertex",   (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.vertex),
      std::make_pair (L"VS",       (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.vertex),
    
      std::make_pair (L"Pixel",    (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.pixel),
      std::make_pair (L"PS",       (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.pixel),
    
      std::make_pair (L"Geometry", (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.geometry),
      std::make_pair (L"GS",       (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.geometry),
    
      std::make_pair (L"Hull",     (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.hull),
      std::make_pair (L"HS",       (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.hull),
    
      std::make_pair (L"Domain",   (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.domain),
      std::make_pair (L"DS",       (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.domain),
    
      std::make_pair (L"Compute",  (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.compute),
      std::make_pair (L"CS",       (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.compute)
    };

  return SK_D3D11_ShaderClassMap_;
}

__forceinline
std::unordered_set <std::wstring>&
__SK_Singleton_D3D11_loaded_configs (void)
{
  static std::unordered_set <std::wstring> loaded_configs_;
  return                                   loaded_configs_;
}

#define loaded_configs          __SK_Singleton_D3D11_loaded_configs()
#define SK_D3D11_ShaderClassMap __SK_Singleton_D3D11_ShaderClassMap()



struct SK_D3D11_CommandBase
{
  struct ShaderMods
  {
    class Load : public SK_ICommand
    {
    public:
      SK_ICommandResult execute (const char* szArgs) override 
      {
        if (strlen (szArgs) > 0)
        {
          std::wstring args =
            SK_UTF8ToWideChar (szArgs);

          SK_D3D11_LoadShaderStateEx (args, false);

          if (loaded_configs.count (args))
            loaded_configs.emplace (args);
        }

        else
          SK_D3D11_LoadShaderState (true);

        return SK_ICommandResult ("D3D11.ShaderMods.Load", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)               override
      {
        return "(Re)Load d3d11_shaders.ini";
      }
    };

    class Unload : public SK_ICommand
    {
    public:
      SK_ICommandResult execute (const char* szArgs) override 
      {
        std::wstring args =
          SK_UTF8ToWideChar (szArgs);

        SK_D3D11_UnloadShaderState (args);

        if (loaded_configs.count (args))
          loaded_configs.erase (args);

        return SK_ICommandResult ("D3D11.ShaderMods.Unload", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)               override
      {
        return "Unload <shader.ini>";
      }
    };

    class ToggleConfig : public SK_ICommand
    {
    public:
      SK_ICommandResult execute (const char* szArgs) override 
      {
        std::wstring args =
          SK_UTF8ToWideChar (szArgs);

        if (loaded_configs.count (args))
        {
          loaded_configs.erase (args);
          SK_D3D11_UnloadShaderState (args);
        }
        else
        {
          loaded_configs.emplace (args);
          SK_D3D11_LoadShaderStateEx (args, false);
        }

        return SK_ICommandResult ("D3D11.ShaderMods.ToggleConfig", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)               override
      {
        return "Load or Unload <shader.ini>";
      }
    };

    class Store : public SK_ICommand
    {
    public:
      SK_ICommandResult execute (const char* szArgs) override
      {
        SK_D3D11_StoreShaderState ();

        return SK_ICommandResult ("D3D11.ShaderMods.Store", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)               override
      {
        return "Store d3d11_shaders.ini";
      }
    };

    class Clear : public SK_ICommand
    {
    public:
      SK_ICommandResult execute (const char* szArgs) override
      {
        SK_D3D11_ClearShaderState ();

        return SK_ICommandResult ("D3D11.ShaderMods.Clear", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)               override
      {
        return "Disable all Shader Mods";
      }
    };

    class Set : public SK_ICommand
    {
    public:
      SK_ICommandResult execute (const char* szArgs) override
      {
        std::wstring wstr_cpy (SK_UTF8ToWideChar (szArgs));

        wchar_t* wszBuf = nullptr;
        wchar_t* wszTok =
          std::wcstok (wstr_cpy.data (), L" ", &wszBuf);

        int arg = 0;

        SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* shader_registry = nullptr;
        uint32_t                                          shader_hash     = 0x0;

        while (wszTok)
        {
          switch (arg++)
          {
            case 0:
            {
              if (! SK_D3D11_ShaderClassMap.count (wszTok))
                return SK_ICommandResult ("D3D11.ShaderMod.Set", szArgs, "Invalid Shader Type", 0, nullptr, this);
              else
                shader_registry = SK_D3D11_ShaderClassMap.at (std::wstring (wszTok));
            } break;

            case 1:
            {
              shader_hash = std::wcstoul (wszTok, nullptr, 16);
            } break;

            case 2:
            {
              if (! _wcsicmp (wszTok, L"Wireframe"))
                shader_registry->wireframe.emplace (shader_hash);
              else if (! _wcsicmp (wszTok, L"OnTop"))
                shader_registry->on_top.emplace (shader_hash);
              else if (! _wcsicmp (wszTok, L"Disable"))
                shader_registry->blacklist.emplace (shader_hash);
              else if (! _wcsicmp (wszTok, L"TriggerReShade"))
                shader_registry->trigger_reshade.before.emplace (shader_hash);
              else
                return SK_ICommandResult ("D3D11.ShaderMod.Set", szArgs, "Invalid Shader State", 0, nullptr, this);
            } break;
          }

          wszTok =
            std::wcstok (nullptr, L" ", &wszBuf);
        }

        return SK_ICommandResult ("D3D11.ShaderMods.Set", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)               override
      {
        return "(Vertex,VS)|(Pixel,PS)|(Geometry,GS)|(Hull,HS)|(Domain,DS)|(Compute,CS)  <Hash>  {Disable|Wireframe|OnTop|TriggerReShade}";
      }
    };

    class Unset : public SK_ICommand
    {
    public:
      SK_ICommandResult execute (const char* szArgs) override
      {
        std::wstring wstr_cpy (SK_UTF8ToWideChar (szArgs));

        wchar_t* wszBuf = nullptr;
        wchar_t* wszTok =
          std::wcstok (wstr_cpy.data (), L" ", &wszBuf);

        int arg = 0;

        SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* shader_registry = nullptr;
        uint32_t                                          shader_hash     = 0x0;

        while (wszTok)
        {
          switch (arg++)
          {
            case 0:
            {
              if (! SK_D3D11_ShaderClassMap.count (wszTok))
                return SK_ICommandResult ("D3D11.ShaderMod.Unset", szArgs, "Invalid Shader Type", 0, nullptr, this);
              else
                shader_registry = SK_D3D11_ShaderClassMap.at (wszTok);
            } break;

            case 1:
            {
              shader_hash = std::wcstoul (wszTok, nullptr, 16);
            } break;

            case 2:
            {
              if (! _wcsicmp (wszTok, L"Wireframe"))
                shader_registry->wireframe.erase (shader_hash);
              else if (! _wcsicmp (wszTok, L"OnTop"))
                shader_registry->on_top.erase (shader_hash);
              else if (! _wcsicmp (wszTok, L"Disable"))
                shader_registry->blacklist.erase (shader_hash);
              else if (! _wcsicmp (wszTok, L"TriggerReShade"))
                shader_registry->trigger_reshade.before.erase (shader_hash);
              else
                return SK_ICommandResult ("D3D11.ShaderMod.Unset", szArgs, "Invalid Shader State", 0, nullptr, this);
            } break;
          }

          wszTok =
            std::wcstok (nullptr, L" ", &wszBuf);
        }

        return SK_ICommandResult ("D3D11.ShaderMods.Unset", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)               override
      {
        return "(Vertex,VS)|(Pixel,PS)|(Geometry,GS)|(Hull,HS)|(Domain,DS)|(Compute,CS)  <Hash>  {Disable|Wireframe|OnTop|TriggerReShade}";
      }
    };

    class Toggle : public SK_ICommand
    {
    public:
      SK_ICommandResult execute (const char* szArgs) override
      {
        std::wstring wstr_cpy (SK_UTF8ToWideChar (szArgs));

        wchar_t* wszBuf = nullptr;
        wchar_t* wszTok =
          std::wcstok (wstr_cpy.data (), L" ", &wszBuf);

        int arg = 0;

        SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* shader_registry = nullptr;
        uint32_t                                          shader_hash     = 0x0;

        while (wszTok)
        {
          switch (arg++)
          {
            case 0:
            {
              if (! SK_D3D11_ShaderClassMap.count (wszTok))
                return SK_ICommandResult ("D3D11.ShaderMod.Toggle", szArgs, "Invalid Shader Type", 1, nullptr, this);
              else
                shader_registry = SK_D3D11_ShaderClassMap.at (wszTok);
            } break;

            case 1:
            {
              shader_hash = std::wcstoul (wszTok, nullptr, 16);
            } break;

            case 2:
            {
              if (! _wcsicmp (wszTok, L"Wireframe"))
              {
                if (! shader_registry->wireframe.count   (shader_hash))
                      shader_registry->wireframe.emplace (shader_hash);
                else
                      shader_registry->wireframe.erase   (shader_hash);
              }
              else if (! _wcsicmp (wszTok, L"OnTop"))
              {
                if (! shader_registry->on_top.count   (shader_hash))
                      shader_registry->on_top.emplace (shader_hash);
                else
                      shader_registry->on_top.erase   (shader_hash);
              }
              else if (! _wcsicmp (wszTok, L"Disable"))
              {
                if (! shader_registry->blacklist.count   (shader_hash))
                      shader_registry->blacklist.emplace (shader_hash);
                else
                      shader_registry->blacklist.erase   (shader_hash);
              }
              else if (! _wcsicmp (wszTok, L"TriggerReShade"))
              {
                if (! shader_registry->trigger_reshade.before.count   (shader_hash))
                      shader_registry->trigger_reshade.before.emplace (shader_hash);
                else
                      shader_registry->trigger_reshade.before.erase   (shader_hash);
              }
              else
                return SK_ICommandResult ("D3D11.ShaderMod.Toggle", szArgs, "Invalid Shader State", 1, nullptr, this);
            } break;
          }

          wszTok =
            std::wcstok (nullptr, L" ", &wszBuf);
        }

        return SK_ICommandResult ("D3D11.ShaderMods.Toggle", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)               override
      {
        return "(Vertex,VS)|(Pixel,PS)|(Geometry,GS)|(Hull,HS)|(Domain,DS)|(Compute,CS)  <Hash>  {Disable|Wireframe|OnTop|TriggerReShade}";
      }
    };
  };

  SK_D3D11_CommandBase (void)
  {
    SK_GetCommandProcessor ()->AddCommand ("D3D11.ShaderMods.Load",         new ShaderMods::Load         ());
    SK_GetCommandProcessor ()->AddCommand ("D3D11.ShaderMods.Unload",       new ShaderMods::Unload       ());
    SK_GetCommandProcessor ()->AddCommand ("D3D11.ShaderMods.ToggleConfig", new ShaderMods::ToggleConfig ());
    SK_GetCommandProcessor ()->AddCommand ("D3D11.ShaderMods.Store",        new ShaderMods::Store        ());
    SK_GetCommandProcessor ()->AddCommand ("D3D11.ShaderMods.Clear",        new ShaderMods::Clear        ());
    SK_GetCommandProcessor ()->AddCommand ("D3D11.ShaderMods.Set",          new ShaderMods::Set          ());
    SK_GetCommandProcessor ()->AddCommand ("D3D11.ShaderMods.Unset",        new ShaderMods::Unset        ());
    SK_GetCommandProcessor ()->AddCommand ("D3D11.ShaderMods.Toggle",       new ShaderMods::Toggle       ());
  }
};


SK_D3D11_CommandBase&
SK_D3D11_GetCommands (void)
{
  static SK_D3D11_CommandBase _commands;
  return                      _commands;
};

#define SK_D3D11_Commands SK_D3D11_GetCommands ()


#include <SpecialK/render/d3d11/d3d11_3.h>

//SK_ICommand
//{
//  virtual SK_ICommandResult execute (const char* szArgs) = 0;
//
//  virtual const char* getHelp            (void) { return "No Help Available"; }
//
//  virtual int         getNumArgs         (void) { return 0; }
//  virtual int         getNumOptionalArgs (void) { return 0; }
//  virtual int         getNumRequiredArgs (void) {
//    return getNumArgs () - getNumOptionalArgs ();
//  }
//};

#include <tuple>

class SK_D3D11_Screenshot
{
public:
  explicit SK_D3D11_Screenshot (const SK_D3D11_Screenshot& rkMove)
  {
    pDev                     = rkMove.pDev;
    pImmediateCtx            = rkMove.pImmediateCtx;
   
    pSwapChain               = rkMove.pSwapChain;   
    pBackbufferSurface       = rkMove.pBackbufferSurface;
    pStagingBackbufferCopy   = rkMove.pStagingBackbufferCopy;

    pPixelBufferFence        = rkMove.pPixelBufferFence;
    ulCommandIssuedOnFrame   = rkMove.ulCommandIssuedOnFrame;


    framebuffer.Width        = rkMove.framebuffer.Width;
    framebuffer.Height       = rkMove.framebuffer.Height;
    framebuffer.NativeFormat = rkMove.framebuffer.NativeFormat;

    framebuffer.PixelBuffer.Attach (rkMove.framebuffer.PixelBuffer.m_pData);
  }


  explicit SK_D3D11_Screenshot (const CComQIPtr <ID3D11Device>& pDevice)
  {
    pDev = pDevice;

    if (pDev.p != nullptr)
    {
      SK_RenderBackend& rb =
        SK_GetCurrentRenderBackend ();

      if (! ( pDev.IsEqualObject (rb.device) &&
                                  rb.d3d11.immediate_ctx != nullptr )
         )
      {
        pDev->GetImmediateContext (&pImmediateCtx);
      }
      else
        pImmediateCtx = rb.d3d11.immediate_ctx;

      CComQIPtr <ID3D11DeviceContext3>
          pImmediateCtx3 (pImmediateCtx);
      if (pImmediateCtx3 != nullptr)
      {
        pImmediateCtx = pImmediateCtx3;
      }

      D3D11_QUERY_DESC fence_query_desc =
      {
        D3D11_QUERY_EVENT,
        0x00
      };
 
      if ( SUCCEEDED ( pDev->CreateQuery ( &fence_query_desc,
                                             &pPixelBufferFence
                                         )
                     )
         )
      {
        if ( SUCCEEDED ( SK_GetCurrentRenderBackend ().swapchain.QueryInterface (
                           &pSwapChain                                          )
                       )
           )
        {
          ulCommandIssuedOnFrame = SK_GetFramesDrawn ();

          if ( SUCCEEDED ( pSwapChain->GetBuffer ( 0,
                                                     __uuidof (ID3D11Texture2D),
                                                       (void **)&pBackbufferSurface.p
                                                 )
                         )
             )
          {
            D3D11_TEXTURE2D_DESC          backbuffer_desc = { };
            pBackbufferSurface->GetDesc (&backbuffer_desc);

            framebuffer.Width        = backbuffer_desc.Width;
            framebuffer.Height       = backbuffer_desc.Height;
            framebuffer.NativeFormat = backbuffer_desc.Format;

            D3D11_TEXTURE2D_DESC staging_desc =
            {
              framebuffer.Width, framebuffer.Height,
              1,                 1,

              framebuffer.NativeFormat,//DXGI_FORMAT_R8G8B8A8_UNORM,

              { 1, 0 },//D3D11_STANDARD_MULTISAMPLE_PATTERN }, 

                D3D11_USAGE_STAGING,                              0x00,
              ( D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE ), 0x00
            };

            if ( SUCCEEDED (
              D3D11Dev_CreateTexture2D_Original ( pDev,
                   &staging_desc, nullptr,
                     &pStagingBackbufferCopy )
                           )
               )
            {
              if (backbuffer_desc.SampleDesc.Count == 1)
              {
                pImmediateCtx->CopyResource ( pStagingBackbufferCopy,
                                              pBackbufferSurface );
              }

              else
              {
                pImmediateCtx->ResolveSubresource ( pStagingBackbufferCopy, 0,
                                                    pBackbufferSurface,     0,
                                                    framebuffer.NativeFormat );
              }

              if (pImmediateCtx3 != nullptr)
              {
                pImmediateCtx3->Flush1 (D3D11_CONTEXT_TYPE_COPY, nullptr);
              }

              pImmediateCtx->End          (pPixelBufferFence);

              return;
            }
          }
        }
      }
    }

    dispose ();
  }

  ~SK_D3D11_Screenshot (void) { dispose (); }


  bool isValid (void) { return pPixelBufferFence != nullptr; }
  bool isReady (void)
  {
    if (! isValid ())
      return false;

    if (ulCommandIssuedOnFrame == SK_GetFramesDrawn ())
      return false;

    if (pPixelBufferFence != nullptr)
    {
      return ( S_FALSE != pImmediateCtx->GetData ( pPixelBufferFence, nullptr,
                                                   0,                 0x0 ) );
    }

    return false;
  }

  void dispose (void)
  {
    pPixelBufferFence      = nullptr;

    pStagingBackbufferCopy = nullptr;
    pBackbufferSurface     = nullptr;

    pImmediateCtx          = nullptr;
    pSwapChain             = nullptr;
    pDev                   = nullptr;

    if (framebuffer.PixelBuffer.m_pData != nullptr)
        framebuffer.PixelBuffer.Free ();
  };


  bool getData ( UINT     *pWidth,
                 UINT     *pHeight,
                 uint8_t **ppData,
                 bool      Wait = false )
  {
    auto ReadBack = [&](void) -> bool
    {
      const size_t BytesPerPel = DirectX::BitsPerPixel (framebuffer.NativeFormat) / 8UL;
      const UINT   Subresource =
        D3D11CalcSubresource ( 0, 0, 1 );

      D3D11_MAPPED_SUBRESOURCE finished_copy = { };

      if ( SUCCEEDED ( pImmediateCtx->Map (
                         pStagingBackbufferCopy, Subresource,
                           D3D11_MAP_READ,       0x0,
                             &finished_copy )
                     )
         )
      {
        size_t PackedDstPitch,
               PackedDstSlicePitch;

        DirectX::ComputePitch (
          framebuffer.NativeFormat,
            framebuffer.Width, framebuffer.Height,
              PackedDstPitch, PackedDstSlicePitch
         );

        if ( framebuffer.PixelBuffer.AllocateBytes ( framebuffer.Height *
                                                       PackedDstPitch
                                                   )
           )
        {
          *pWidth  = framebuffer.Width;
          *pHeight = framebuffer.Height;

          uint8_t* pSrc =  (uint8_t *)finished_copy.pData;
          uint8_t* pDst = framebuffer.PixelBuffer.m_pData;

          for ( UINT i = 0; i < framebuffer.Height; ++i )
          {
            memcpy ( pDst, pSrc, finished_copy.RowPitch );
            
            // Eliminate pre-multiplied alpha problems (the stupid way)
            for ( UINT j = 3 ; j < PackedDstPitch ; j += 4 )
            {
              pDst [j] = 255UL;
            }

            pSrc += finished_copy.RowPitch;
            pDst +=         PackedDstPitch;
          }
        }

        SK_LOG0 ( ( L"Screenshot Readback Complete after %li frames",
                      SK_GetFramesDrawn () - ulCommandIssuedOnFrame ),
                    L"D3D11SShot" );

        pImmediateCtx->Unmap (pStagingBackbufferCopy, Subresource);

        *ppData = framebuffer.PixelBuffer.m_pData;

        return true;
      }

      return false;
    };


    bool ready_to_read = false;


    if (! Wait)
    {
      if (isReady ())
      {
        ready_to_read = true;
      }
    }

    else if (isValid ())
    {
      ready_to_read = true;
    }


    return ( ready_to_read ? ReadBack () :
                             false         );
  }

  DXGI_FORMAT
  getInternalFormat (void)
  {
    return framebuffer.NativeFormat;
  }


protected:
  CComPtr <ID3D11Device>        pDev                   = nullptr;
  CComPtr <ID3D11DeviceContext> pImmediateCtx          = nullptr;

  CComPtr <IDXGISwapChain>      pSwapChain             = nullptr;
  CComPtr <ID3D11Texture2D>     pBackbufferSurface     = nullptr;
  CComPtr <ID3D11Texture2D>     pStagingBackbufferCopy = nullptr;

  CComPtr <ID3D11Query>         pPixelBufferFence      = nullptr;
  ULONG                         ulCommandIssuedOnFrame = 0;

  struct framebuffer_s
  {
    UINT               Width        = 0,
                       Height       = 0;
    DXGI_FORMAT        NativeFormat = DXGI_FORMAT_UNKNOWN;

    CHeapPtr <uint8_t> PixelBuffer;
  } framebuffer;
};


struct
{
  union
  {
    volatile LONG stages [3];

    struct
    {
      volatile LONG pre_game_hud;

      volatile LONG without_sk_osd;
      volatile LONG with_sk_osd;
    };
  };
} enqueued_screenshots { 0, 0, 0 };

concurrency::concurrent_queue <SK_D3D11_Screenshot> screenshot_queue;


bool
SK_D3D11_CaptureSteamScreenshot  ( SK::ScreenshotStage when =
                                   SK::ScreenshotStage::EndOfFrame )
{
  if ( (int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::D3D11 )
  {
    int stage = 0;

    static const std::map <SK::ScreenshotStage, int> __stage_map = {
      { SK::ScreenshotStage::BeforeGameHUD, 0 },
      { SK::ScreenshotStage::BeforeOSD,     1 },
      { SK::ScreenshotStage::EndOfFrame,    2 }
    };

    const auto it = __stage_map.find (when);

    if (it != __stage_map.cend ())
    {
      stage = it->second;

      InterlockedIncrement (&enqueued_screenshots.stages [stage]);

      return true;
    }
  }

  return false;
}


ScreenshotHandle
WINAPI
SK_SteamAPI_AddScreenshotToLibraryEx (const char *pchFilename, const char *pchThumbnailFilename, int nWidth, int nHeight, bool Wait = false);


void
SK_D3D11_ProcessScreenshotQueue (int stage = 2)
{
  const int __MaxStage = 2;

  assert (stage >= 0 && stage <= __MaxStage);

  if (ReadAcquire (&enqueued_screenshots.stages [stage]) > 0)
  {
    if (InterlockedDecrement (&enqueued_screenshots.stages [stage]) >= 0)
    {
      screenshot_queue.push (
        SK_D3D11_Screenshot (SK_GetCurrentRenderBackend ().device.p)
      );
    }

    else InterlockedIncrement (&enqueued_screenshots.stages [stage]);
  }


  if (! screenshot_queue.empty ())
  {
    static volatile HANDLE hSignalScreenshot = INVALID_HANDLE_VALUE;

    if (InterlockedCompareExchangePointer (&hSignalScreenshot, 0, INVALID_HANDLE_VALUE) == INVALID_HANDLE_VALUE)
    {
      InterlockedExchangePointer ( (void **)&hSignalScreenshot,
                                     CreateEventW (nullptr, FALSE, TRUE, nullptr) );

      SK_Thread_Create ([](LPVOID) -> DWORD
      {
        SetCurrentThreadDescription (          L"[SK] D3D11 Screenshot Capture Thread" );
        SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_BELOW_NORMAL |
                                                              THREAD_MODE_BACKGROUND_BEGIN );


        HANDLE hSignal =
          ReadPointerAcquire (&hSignalScreenshot);

        static const SK_D3D11_Screenshot _invalid (nullptr);


        // Any incomplete captures are pushed onto this queue, and then the pending
        //   queue (once drained) is re-built.
        //
        //  This is faster than iterating a synchronized list in highly multi-threaded engines.
        static concurrency::concurrent_queue <SK_D3D11_Screenshot> rejected_screenshots;


        while (! ReadAcquire (&__SK_DLL_Ending))
        {
          MsgWaitForMultipleObjectsEx ( 1, &hSignal, INFINITE, 0x0, 0x0 );

          while (! screenshot_queue.empty ())
          {
            SK_D3D11_Screenshot pop_off (_invalid);

            if (screenshot_queue.try_pop (pop_off))
            {
              UINT     Width, Height;
              uint8_t* pData;

              if (pop_off.getData (&Width, &Height, &pData))
              {
                using namespace DirectX;

                Image raw_img = { };

                ComputePitch (
                  pop_off.getInternalFormat (),
                    Width, Height,
                      raw_img.rowPitch, raw_img.slicePitch
                );

                raw_img.format = pop_off.getInternalFormat ();
                raw_img.width  = Width;
                raw_img.height = Height;
                raw_img.pixels = pData;


                wchar_t       wszAbsolutePathToScreenshot [ MAX_PATH * 2 + 1 ] = { };
                wcsncpy     ( wszAbsolutePathToScreenshot,
                  screenshot_manager->getExternalScreenshotPath (), MAX_PATH * 2 );

                PathAppendW (wszAbsolutePathToScreenshot, L"SK_SteamScreenshotImport.jpg");
                SK_CreateDirectories (wszAbsolutePathToScreenshot);
                
                time_t screenshot_time;

                wchar_t       wszAbsolutePathToLossless [ MAX_PATH * 2 + 1 ] = { };
                wcsncpy     ( wszAbsolutePathToLossless,
                  screenshot_manager->getExternalScreenshotPath (), MAX_PATH * 2 );

                PathAppendW ( wszAbsolutePathToLossless,
                  SK_FormatStringW ( L"Lossless\\%lu.png",
                              time (&screenshot_time) ).c_str () );

                if ( SUCCEEDED (
                  SaveToWICFile ( raw_img, WIC_FLAGS_NONE,
                                     GetWICCodec (WIC_CODEC_JPEG),
                                      wszAbsolutePathToScreenshot )
                               )
                   )
                {
                  if (config.steam.screenshots.png_compress)
                  {
                    SK_CreateDirectories (wszAbsolutePathToLossless);

                    if ( SUCCEEDED (
                          SaveToWICFile ( raw_img, WIC_FLAGS_NONE,
                                            GetWICCodec (WIC_CODEC_PNG),
                                              wszAbsolutePathToLossless )
                         )
                       )
                    {
                      // Refresh
                      screenshot_manager->getExternalScreenshotRepository (true);
                    }
                  }

                  wchar_t      wszAbsolutePathToThumbnail [ MAX_PATH * 2 + 1 ] = { };
                  wcsncpy     ( wszAbsolutePathToLossless,
                    screenshot_manager->getExternalScreenshotPath (), MAX_PATH * 2 );

                  PathAppendW (wszAbsolutePathToThumbnail, L"SK_SteamThumbnailImport.jpg");

                  float aspect = (float)Height /
                                 (float)Width;

                  ScratchImage thumbnailImage;

                  Resize ( raw_img, 200, static_cast <size_t> (200 * aspect), TEX_FILTER_TRIANGLE, thumbnailImage );

                  SaveToWICFile ( *thumbnailImage.GetImages (), WIC_FLAGS_NONE,
                                    GetWICCodec (WIC_CODEC_JPEG),
                                      wszAbsolutePathToThumbnail );

                  ScreenshotHandle screenshot =
                    SK_SteamAPI_AddScreenshotToLibraryEx ( SK_WideCharToUTF8 (wszAbsolutePathToScreenshot).c_str  (),
                                                             SK_WideCharToUTF8 (wszAbsolutePathToThumbnail).c_str (),
                                                               Width, Height, true );

                  SK_LOG0 ( ( L"Finished Steam Screenshot Import for Handle: '%x'", 
                              screenshot ), L"SteamSShot" );

                  // Remove the temporary files...
                  DeleteFileW (wszAbsolutePathToScreenshot);
                  DeleteFileW (wszAbsolutePathToThumbnail);
                }
              }

              else
              {
                rejected_screenshots.push (pop_off);
              }
            }
          }

          while (! rejected_screenshots.empty ())
          {
            SK_D3D11_Screenshot push_back (_invalid);

            if (rejected_screenshots.try_pop (push_back))
            {
              screenshot_queue.push (push_back);
            }
          }
        }

        SK_Thread_CloseSelf ();

        CloseHandle (hSignal);

        return 0;
      });
    }

    else
      SetEvent (hSignalScreenshot);
  }
}



#include <SpecialK/ini.h>

void
__stdcall
SK_D3D11_PresentFirstFrame (IDXGISwapChain* pSwapChain)
{
  UNREFERENCED_PARAMETER (pSwapChain);

  auto cmds = SK_D3D11_GetCommands ();

  LocalHook_D3D11CreateDevice.active             = true;
  LocalHook_D3D11CreateDeviceAndSwapChain.active = true;

  for ( auto& it : local_d3d11_records )
  {
    if (it->active)
    {
      SK_Hook_ResolveTarget (*it);
  
      // Don't cache addresses that were screwed with by other injectors
      const wchar_t* wszSection =
        StrStrIW (it->target.module_path, LR"(d3d11.dll)") ?
                                        L"D3D11.Hooks" : nullptr;
  
      if (! wszSection)
      {
        SK_LOG0 ( ( L"Hook for '%hs' resides in '%s', will not cache!",
                      it->target.symbol_name,
          SK_StripUserNameFromPathW (
            std::wstring (
                      it->target.module_path
                         ).data ()
          )                                                             ),
                    L"Hook Cache" );
      }
  
      else
        SK_Hook_CacheTarget ( *it, wszSection );
    }
  }
  
  if (SK_IsInjected ())
  {
    auto it_local  = std::begin (local_d3d11_records);
    auto it_global = std::begin (global_d3d11_records);
  
    while ( it_local != std::end (local_d3d11_records) )
    {
      if (( *it_local )->hits && (
StrStrIW (( *it_local )->target.module_path, LR"(d3d11.dll)") ) &&
          ( *it_local )->active)
        SK_Hook_PushLocalCacheOntoGlobal ( **it_local,
                                             **it_global );
      else
      {
        ( *it_global )->target.addr = nullptr;
        ( *it_global )->hits        = 0;
        ( *it_global )->active      = false;
      }
  
      it_global++, it_local++;
    }
  }
}

static bool quick_hooked = false;

void
SK_D3D11_QuickHook (void)
{
  if (config.steam.preload_overlay)
    return;

  static volatile LONG hooked = FALSE;

  if (! InterlockedCompareExchange (&hooked, TRUE, FALSE))
  {
    sk_hook_cache_enablement_s state =
      SK_Hook_PreCacheModule ( L"D3D11",
                                 local_d3d11_records,
                                   global_d3d11_records );

    if ( state.hooks_loaded.from_shared_dll > 0 ||
         state.hooks_loaded.from_game_ini   > 0 )
    {
      // For early loading UnX
      ///SK_D3D11_InitTextures ();

      quick_hooked = true;
    }

    else 
    {
      for ( auto& it : local_d3d11_records )
      {
        it->active = false;
      }
    }

    InterlockedIncrement (&hooked);
  }

  SK_Thread_SpinUntilAtomicMin (&hooked, 2);
}


bool
SK_D3D11_QuickHooked (void)
{
  return quick_hooked;
}

int
SK_D3D11_PurgeHookAddressCache (void)
{
  int i = 0;

  for ( auto& it : local_d3d11_records )
  {
    SK_Hook_RemoveTarget ( *it, L"D3D11.Hooks" );

    ++i;
  }

  return i;
}

void
SK_D3D11_UpdateHookAddressCache (void)
{
  for ( auto& it : local_d3d11_records )
  {
    if (it->active)
    {
      SK_Hook_ResolveTarget (*it);

      // Don't cache addresses that were screwed with by other injectors
      const wchar_t* wszSection =
        StrStrIW (it->target.module_path, LR"(\sys)") ?
                                          L"D3D11.Hooks" : nullptr;

      if (! wszSection)
      {
        SK_LOG0 ( ( L"Hook for '%hs' resides in '%s', will not cache!",
                      it->target.symbol_name,
          SK_StripUserNameFromPathW (
            std::wstring (
                      it->target.module_path
                         ).data ()
          )                                                             ),
                    L"Hook Cache" );
      }
      SK_Hook_CacheTarget ( *it, wszSection );
    }
  }

  auto it_local  = std::begin (local_d3d11_records);
  auto it_global = std::begin (global_d3d11_records);

  while ( it_local != std::end (local_d3d11_records) )
  {
    if (   ( *it_local )->hits &&
 StrStrIW (( *it_local )->target.module_path, LR"(\sys)") &&
           ( *it_local )->active)
      SK_Hook_PushLocalCacheOntoGlobal ( **it_local,
                                           **it_global );
    else
    {
      ( *it_global )->target.addr = nullptr;
      ( *it_global )->hits        = 0;
      ( *it_global )->active      = false;
    }

    it_global++, it_local++;
  }
}

#ifdef _WIN64
#pragma comment (linker, "/export:DirectX::ScratchImage::Release=?Release@ScratchImage@DirectX@@QEAAXXZ")
#else
#pragma comment (linker, "/export:DirectX::ScratchImage::Release=?Release@ScratchImage@DirectX@@QAAXXZ")
#endif

HRESULT
__cdecl
SK_DXTex_CreateTexture ( _In_reads_(nimages) const DirectX::Image*       srcImages,
                         _In_                      size_t                nimages,
                         _In_                const DirectX::TexMetadata& metadata,
                         _Outptr_                  ID3D11Resource**      ppResource )
{
  return
    DirectX::CreateTexture ( (ID3D11Device *)SK_Render_GetDevice (),
                               srcImages, nimages, metadata, ppResource );
}