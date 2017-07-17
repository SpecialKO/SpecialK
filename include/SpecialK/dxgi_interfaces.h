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

#ifndef __SK__DXGI_INTERFACES_H__
#define __SK__DXGI_INTERFACES_H__

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 8.00.0603 */
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


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
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __dxgi_h__
#define __dxgi_h__

#ifndef D3DCOLORVALUE_DEFINED
typedef struct _D3DCOLORVALUE {
    float r;
    float g;
    float b;
    float a;
} D3DCOLORVALUE;

#define D3DCOLORVALUE_DEFINED
#endif

typedef D3DCOLORVALUE DXGI_RGBA;

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDXGIObject_FWD_DEFINED__
#define __IDXGIObject_FWD_DEFINED__
typedef interface IDXGIObject IDXGIObject;

#endif 	/* __IDXGIObject_FWD_DEFINED__ */


#ifndef __IDXGIDeviceSubObject_FWD_DEFINED__
#define __IDXGIDeviceSubObject_FWD_DEFINED__
typedef interface IDXGIDeviceSubObject IDXGIDeviceSubObject;

#endif 	/* __IDXGIDeviceSubObject_FWD_DEFINED__ */


#ifndef __IDXGIResource_FWD_DEFINED__
#define __IDXGIResource_FWD_DEFINED__
typedef interface IDXGIResource IDXGIResource;

#endif 	/* __IDXGIResource_FWD_DEFINED__ */


#ifndef __IDXGIKeyedMutex_FWD_DEFINED__
#define __IDXGIKeyedMutex_FWD_DEFINED__
typedef interface IDXGIKeyedMutex IDXGIKeyedMutex;

#endif 	/* __IDXGIKeyedMutex_FWD_DEFINED__ */


#ifndef __IDXGISurface_FWD_DEFINED__
#define __IDXGISurface_FWD_DEFINED__
typedef interface IDXGISurface IDXGISurface;

#endif 	/* __IDXGISurface_FWD_DEFINED__ */


#ifndef __IDXGISurface1_FWD_DEFINED__
#define __IDXGISurface1_FWD_DEFINED__
typedef interface IDXGISurface1 IDXGISurface1;

#endif 	/* __IDXGISurface1_FWD_DEFINED__ */


#ifndef __IDXGIAdapter_FWD_DEFINED__
#define __IDXGIAdapter_FWD_DEFINED__
typedef interface IDXGIAdapter IDXGIAdapter;

#endif 	/* __IDXGIAdapter_FWD_DEFINED__ */


#ifndef __IDXGIOutput_FWD_DEFINED__
#define __IDXGIOutput_FWD_DEFINED__
typedef interface IDXGIOutput IDXGIOutput;

#endif 	/* __IDXGIOutput_FWD_DEFINED__ */


#ifndef __IDXGISwapChain_FWD_DEFINED__
#define __IDXGISwapChain_FWD_DEFINED__
typedef interface IDXGISwapChain IDXGISwapChain;

#endif 	/* __IDXGISwapChain_FWD_DEFINED__ */


#ifndef __IDXGIFactory_FWD_DEFINED__
#define __IDXGIFactory_FWD_DEFINED__
typedef interface IDXGIFactory IDXGIFactory;

#endif 	/* __IDXGIFactory_FWD_DEFINED__ */


#ifndef __IDXGIDevice_FWD_DEFINED__
#define __IDXGIDevice_FWD_DEFINED__
typedef interface IDXGIDevice IDXGIDevice;

#endif 	/* __IDXGIDevice_FWD_DEFINED__ */


#ifndef __IDXGIFactory1_FWD_DEFINED__
#define __IDXGIFactory1_FWD_DEFINED__
typedef interface IDXGIFactory1 IDXGIFactory1;

#endif 	/* __IDXGIFactory1_FWD_DEFINED__ */


#ifndef __IDXGIAdapter1_FWD_DEFINED__
#define __IDXGIAdapter1_FWD_DEFINED__
typedef interface IDXGIAdapter1 IDXGIAdapter1;

#endif 	/* __IDXGIAdapter1_FWD_DEFINED__ */


#ifndef __IDXGIDevice1_FWD_DEFINED__
#define __IDXGIDevice1_FWD_DEFINED__
typedef interface IDXGIDevice1 IDXGIDevice1;

#endif 	/* __IDXGIDevice1_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "dxgitype.h"

#ifdef __cplusplus
extern "C"{
#endif 


  /* interface __MIDL_itf_dxgi_0000_0000 */
  /* [local] */ 

//#include <winapifamily.h>
#define DXGI_CPU_ACCESS_NONE    ( 0 )
#define DXGI_CPU_ACCESS_DYNAMIC    ( 1 )
#define DXGI_CPU_ACCESS_READ_WRITE    ( 2 )
#define DXGI_CPU_ACCESS_SCRATCH    ( 3 )
#define DXGI_CPU_ACCESS_FIELD        15
#define DXGI_USAGE_SHADER_INPUT             ( 1L << (0 + 4) )
#define DXGI_USAGE_RENDER_TARGET_OUTPUT     ( 1L << (1 + 4) )
#define DXGI_USAGE_BACK_BUFFER              ( 1L << (2 + 4) )
#define DXGI_USAGE_SHARED                   ( 1L << (3 + 4) )
#define DXGI_USAGE_READ_ONLY                ( 1L << (4 + 4) )
#define DXGI_USAGE_DISCARD_ON_PRESENT       ( 1L << (5 + 4) )
#define DXGI_USAGE_UNORDERED_ACCESS         ( 1L << (6 + 4) )
  typedef UINT DXGI_USAGE;

  typedef struct DXGI_FRAME_STATISTICS
  {
    UINT PresentCount;
    UINT PresentRefreshCount;
    UINT SyncRefreshCount;
    LARGE_INTEGER SyncQPCTime;
    LARGE_INTEGER SyncGPUTime;
  } 	DXGI_FRAME_STATISTICS;

  typedef struct DXGI_MAPPED_RECT
  {
    INT Pitch;
    BYTE *pBits;
  } 	DXGI_MAPPED_RECT;

#ifdef __midl
  typedef struct _LUID
  {
    DWORD LowPart;
    LONG HighPart;
  } 	LUID;

  typedef struct _LUID *PLUID;

#endif
  typedef struct DXGI_ADAPTER_DESC
  {
    WCHAR Description[ 128 ];
    UINT VendorId;
    UINT DeviceId;
    UINT SubSysId;
    UINT Revision;
    SIZE_T DedicatedVideoMemory;
    SIZE_T DedicatedSystemMemory;
    SIZE_T SharedSystemMemory;
    LUID AdapterLuid;
  } 	DXGI_ADAPTER_DESC;

#if !defined(HMONITOR_DECLARED) && !defined(HMONITOR) && (WINVER < 0x0500)
#define HMONITOR_DECLARED
#if 0
  typedef HANDLE HMONITOR;

#endif
  DECLARE_HANDLE(HMONITOR);
#endif
  typedef struct DXGI_OUTPUT_DESC
  {
    WCHAR DeviceName[ 32 ];
    RECT DesktopCoordinates;
    BOOL AttachedToDesktop;
    DXGI_MODE_ROTATION Rotation;
    HMONITOR Monitor;
  } 	DXGI_OUTPUT_DESC;

  typedef struct DXGI_SHARED_RESOURCE
  {
    HANDLE Handle;
  } 	DXGI_SHARED_RESOURCE;

#define	DXGI_RESOURCE_PRIORITY_MINIMUM	( 0x28000000 )

#define	DXGI_RESOURCE_PRIORITY_LOW	( 0x50000000 )

#define	DXGI_RESOURCE_PRIORITY_NORMAL	( 0x78000000 )

#define	DXGI_RESOURCE_PRIORITY_HIGH	( 0xa0000000 )

#define	DXGI_RESOURCE_PRIORITY_MAXIMUM	( 0xc8000000 )

  typedef 
  enum DXGI_RESIDENCY
  {
    DXGI_RESIDENCY_FULLY_RESIDENT	= 1,
    DXGI_RESIDENCY_RESIDENT_IN_SHARED_MEMORY	= 2,
    DXGI_RESIDENCY_EVICTED_TO_DISK	= 3
  } 	DXGI_RESIDENCY;

  typedef struct DXGI_SURFACE_DESC
  {
    UINT Width;
    UINT Height;
    DXGI_FORMAT Format;
    DXGI_SAMPLE_DESC SampleDesc;
  } 	DXGI_SURFACE_DESC;

  typedef 
  enum DXGI_SWAP_EFFECT
  {
    DXGI_SWAP_EFFECT_DISCARD         = 0,
    DXGI_SWAP_EFFECT_SEQUENTIAL      = 1,
    DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL = 3,
    DXGI_SWAP_EFFECT_FLIP_DISCARD    = 4
  }  DXGI_SWAP_EFFECT;

  typedef 
  enum DXGI_SWAP_CHAIN_FLAG
  {
    DXGI_SWAP_CHAIN_FLAG_NONPREROTATED	= 1,
    DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH	= 2,
    DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE	= 4,
    DXGI_SWAP_CHAIN_FLAG_RESTRICTED_CONTENT	= 8,
    DXGI_SWAP_CHAIN_FLAG_RESTRICT_SHARED_RESOURCE_DRIVER	= 16,
    DXGI_SWAP_CHAIN_FLAG_DISPLAY_ONLY	= 32,
    DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT	= 64,
    DXGI_SWAP_CHAIN_FLAG_FOREGROUND_LAYER	= 128,
    DXGI_SWAP_CHAIN_FLAG_FULLSCREEN_VIDEO	= 256,
    DXGI_SWAP_CHAIN_FLAG_YUV_VIDEO	= 512
  } 	DXGI_SWAP_CHAIN_FLAG;

  typedef struct DXGI_SWAP_CHAIN_DESC
  {
    DXGI_MODE_DESC BufferDesc;
    DXGI_SAMPLE_DESC SampleDesc;
    DXGI_USAGE BufferUsage;
    UINT BufferCount;
    HWND OutputWindow;
    BOOL Windowed;
    DXGI_SWAP_EFFECT SwapEffect;
    UINT Flags;
  } 	DXGI_SWAP_CHAIN_DESC;



  extern RPC_IF_HANDLE __MIDL_itf_dxgi_0000_0000_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi_0000_0000_v0_0_s_ifspec;

#ifndef __IDXGIObject_INTERFACE_DEFINED__
#define __IDXGIObject_INTERFACE_DEFINED__

  /* interface IDXGIObject */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIObject;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("aec22fb8-76f3-4639-9be0-28eb43a67a2e")
    IDXGIObject : public IUnknown
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE SetPrivateData( 
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface( 
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetPrivateData( 
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetParent( 
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIObjectVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIObject * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIObject * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIObject * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGIObject * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGIObject * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGIObject * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGIObject * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    END_INTERFACE
  } IDXGIObjectVtbl;

  interface IDXGIObject
  {
    CONST_VTBL struct IDXGIObjectVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIObject_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIObject_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIObject_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIObject_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIObject_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIObject_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIObject_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIObject_INTERFACE_DEFINED__ */


#ifndef __IDXGIDeviceSubObject_INTERFACE_DEFINED__
#define __IDXGIDeviceSubObject_INTERFACE_DEFINED__

  /* interface IDXGIDeviceSubObject */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIDeviceSubObject;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("3d3e0379-f9de-4d58-bb6c-18d62992f1a6")
    IDXGIDeviceSubObject : public IDXGIObject
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetDevice( 
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppDevice) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIDeviceSubObjectVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIDeviceSubObject * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIDeviceSubObject * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIDeviceSubObject * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGIDeviceSubObject * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGIDeviceSubObject * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGIDeviceSubObject * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGIDeviceSubObject * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      IDXGIDeviceSubObject * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppDevice);

    END_INTERFACE
  } IDXGIDeviceSubObjectVtbl;

  interface IDXGIDeviceSubObject
  {
    CONST_VTBL struct IDXGIDeviceSubObjectVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIDeviceSubObject_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIDeviceSubObject_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIDeviceSubObject_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIDeviceSubObject_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIDeviceSubObject_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIDeviceSubObject_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIDeviceSubObject_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIDeviceSubObject_GetDevice(This,riid,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppDevice) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIDeviceSubObject_INTERFACE_DEFINED__ */


#ifndef __IDXGIResource_INTERFACE_DEFINED__
#define __IDXGIResource_INTERFACE_DEFINED__

  /* interface IDXGIResource */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIResource;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("035f3ab4-482e-4e50-b41f-8a7f8bd8960b")
    IDXGIResource : public IDXGIDeviceSubObject
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetSharedHandle( 
      /* [annotation][out] */ 
      _Out_  HANDLE *pSharedHandle) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetUsage( 
      /* [annotation][out] */ 
      _Out_  DXGI_USAGE *pUsage) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetEvictionPriority( 
      /* [in] */ UINT EvictionPriority) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetEvictionPriority( 
      /* [annotation][retval][out] */ 
      _Out_  UINT *pEvictionPriority) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIResourceVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIResource * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIResource * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIResource * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGIResource * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGIResource * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGIResource * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGIResource * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      IDXGIResource * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppDevice);

    HRESULT ( STDMETHODCALLTYPE *GetSharedHandle )( 
      IDXGIResource * This,
      /* [annotation][out] */ 
      _Out_  HANDLE *pSharedHandle);

    HRESULT ( STDMETHODCALLTYPE *GetUsage )( 
      IDXGIResource * This,
      /* [annotation][out] */ 
      _Out_  DXGI_USAGE *pUsage);

    HRESULT ( STDMETHODCALLTYPE *SetEvictionPriority )( 
      IDXGIResource * This,
      /* [in] */ UINT EvictionPriority);

    HRESULT ( STDMETHODCALLTYPE *GetEvictionPriority )( 
      IDXGIResource * This,
      /* [annotation][retval][out] */ 
      _Out_  UINT *pEvictionPriority);

    END_INTERFACE
  } IDXGIResourceVtbl;

  interface IDXGIResource
  {
    CONST_VTBL struct IDXGIResourceVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIResource_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIResource_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIResource_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIResource_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIResource_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIResource_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIResource_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIResource_GetDevice(This,riid,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppDevice) ) 


#define IDXGIResource_GetSharedHandle(This,pSharedHandle)	\
    ( (This)->lpVtbl -> GetSharedHandle(This,pSharedHandle) ) 

#define IDXGIResource_GetUsage(This,pUsage)	\
    ( (This)->lpVtbl -> GetUsage(This,pUsage) ) 

#define IDXGIResource_SetEvictionPriority(This,EvictionPriority)	\
    ( (This)->lpVtbl -> SetEvictionPriority(This,EvictionPriority) ) 

#define IDXGIResource_GetEvictionPriority(This,pEvictionPriority)	\
    ( (This)->lpVtbl -> GetEvictionPriority(This,pEvictionPriority) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIResource_INTERFACE_DEFINED__ */


#ifndef __IDXGIKeyedMutex_INTERFACE_DEFINED__
#define __IDXGIKeyedMutex_INTERFACE_DEFINED__

  /* interface IDXGIKeyedMutex */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIKeyedMutex;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("9d8e1289-d7b3-465f-8126-250e349af85d")
    IDXGIKeyedMutex : public IDXGIDeviceSubObject
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE AcquireSync( 
      /* [in] */ UINT64 Key,
      /* [in] */ DWORD dwMilliseconds) = 0;

    virtual HRESULT STDMETHODCALLTYPE ReleaseSync( 
      /* [in] */ UINT64 Key) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIKeyedMutexVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIKeyedMutex * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIKeyedMutex * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIKeyedMutex * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGIKeyedMutex * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGIKeyedMutex * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGIKeyedMutex * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGIKeyedMutex * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      IDXGIKeyedMutex * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppDevice);

    HRESULT ( STDMETHODCALLTYPE *AcquireSync )( 
      IDXGIKeyedMutex * This,
      /* [in] */ UINT64 Key,
      /* [in] */ DWORD dwMilliseconds);

    HRESULT ( STDMETHODCALLTYPE *ReleaseSync )( 
      IDXGIKeyedMutex * This,
      /* [in] */ UINT64 Key);

    END_INTERFACE
  } IDXGIKeyedMutexVtbl;

  interface IDXGIKeyedMutex
  {
    CONST_VTBL struct IDXGIKeyedMutexVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIKeyedMutex_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIKeyedMutex_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIKeyedMutex_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIKeyedMutex_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIKeyedMutex_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIKeyedMutex_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIKeyedMutex_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIKeyedMutex_GetDevice(This,riid,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppDevice) ) 


#define IDXGIKeyedMutex_AcquireSync(This,Key,dwMilliseconds)	\
    ( (This)->lpVtbl -> AcquireSync(This,Key,dwMilliseconds) ) 

#define IDXGIKeyedMutex_ReleaseSync(This,Key)	\
    ( (This)->lpVtbl -> ReleaseSync(This,Key) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIKeyedMutex_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_dxgi_0000_0004 */
  /* [local] */ 

#define	DXGI_MAP_READ	( 1UL )

#define	DXGI_MAP_WRITE	( 2UL )

#define	DXGI_MAP_DISCARD	( 4UL )



  extern RPC_IF_HANDLE __MIDL_itf_dxgi_0000_0004_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi_0000_0004_v0_0_s_ifspec;

#ifndef __IDXGISurface_INTERFACE_DEFINED__
#define __IDXGISurface_INTERFACE_DEFINED__

  /* interface IDXGISurface */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGISurface;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("cafcb56c-6ac3-4889-bf47-9e23bbd260ec")
    IDXGISurface : public IDXGIDeviceSubObject
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetDesc( 
      /* [annotation][out] */ 
      _Out_  DXGI_SURFACE_DESC *pDesc) = 0;

    virtual HRESULT STDMETHODCALLTYPE Map( 
      /* [annotation][out] */ 
      _Out_  DXGI_MAPPED_RECT *pLockedRect,
      /* [in] */ UINT MapFlags) = 0;

    virtual HRESULT STDMETHODCALLTYPE Unmap( void) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGISurfaceVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGISurface * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGISurface * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGISurface * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGISurface * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGISurface * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGISurface * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGISurface * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      IDXGISurface * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppDevice);

    HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
      IDXGISurface * This,
      /* [annotation][out] */ 
      _Out_  DXGI_SURFACE_DESC *pDesc);

    HRESULT ( STDMETHODCALLTYPE *Map )( 
      IDXGISurface * This,
      /* [annotation][out] */ 
      _Out_  DXGI_MAPPED_RECT *pLockedRect,
      /* [in] */ UINT MapFlags);

    HRESULT ( STDMETHODCALLTYPE *Unmap )( 
      IDXGISurface * This);

    END_INTERFACE
  } IDXGISurfaceVtbl;

  interface IDXGISurface
  {
    CONST_VTBL struct IDXGISurfaceVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGISurface_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGISurface_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGISurface_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGISurface_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGISurface_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGISurface_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGISurface_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGISurface_GetDevice(This,riid,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppDevice) ) 


#define IDXGISurface_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGISurface_Map(This,pLockedRect,MapFlags)	\
    ( (This)->lpVtbl -> Map(This,pLockedRect,MapFlags) ) 

#define IDXGISurface_Unmap(This)	\
    ( (This)->lpVtbl -> Unmap(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGISurface_INTERFACE_DEFINED__ */


#ifndef __IDXGISurface1_INTERFACE_DEFINED__
#define __IDXGISurface1_INTERFACE_DEFINED__

  /* interface IDXGISurface1 */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGISurface1;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("4AE63092-6327-4c1b-80AE-BFE12EA32B86")
    IDXGISurface1 : public IDXGISurface
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetDC( 
      /* [in] */ BOOL Discard,
      /* [annotation][out] */ 
      _Out_  HDC *phdc) = 0;

    virtual HRESULT STDMETHODCALLTYPE ReleaseDC( 
      /* [annotation][in] */ 
      _In_opt_  RECT *pDirtyRect) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGISurface1Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGISurface1 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGISurface1 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGISurface1 * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGISurface1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGISurface1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGISurface1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGISurface1 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      IDXGISurface1 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppDevice);

    HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
      IDXGISurface1 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_SURFACE_DESC *pDesc);

    HRESULT ( STDMETHODCALLTYPE *Map )( 
      IDXGISurface1 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_MAPPED_RECT *pLockedRect,
      /* [in] */ UINT MapFlags);

    HRESULT ( STDMETHODCALLTYPE *Unmap )( 
      IDXGISurface1 * This);

    HRESULT ( STDMETHODCALLTYPE *GetDC )( 
      IDXGISurface1 * This,
      /* [in] */ BOOL Discard,
      /* [annotation][out] */ 
      _Out_  HDC *phdc);

    HRESULT ( STDMETHODCALLTYPE *ReleaseDC )( 
      IDXGISurface1 * This,
      /* [annotation][in] */ 
      _In_opt_  RECT *pDirtyRect);

    END_INTERFACE
  } IDXGISurface1Vtbl;

  interface IDXGISurface1
  {
    CONST_VTBL struct IDXGISurface1Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGISurface1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGISurface1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGISurface1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGISurface1_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGISurface1_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGISurface1_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGISurface1_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGISurface1_GetDevice(This,riid,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppDevice) ) 


#define IDXGISurface1_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGISurface1_Map(This,pLockedRect,MapFlags)	\
    ( (This)->lpVtbl -> Map(This,pLockedRect,MapFlags) ) 

#define IDXGISurface1_Unmap(This)	\
    ( (This)->lpVtbl -> Unmap(This) ) 


#define IDXGISurface1_GetDC(This,Discard,phdc)	\
    ( (This)->lpVtbl -> GetDC(This,Discard,phdc) ) 

#define IDXGISurface1_ReleaseDC(This,pDirtyRect)	\
    ( (This)->lpVtbl -> ReleaseDC(This,pDirtyRect) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGISurface1_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_dxgi_0000_0006 */
  /* [local] */ 




  extern RPC_IF_HANDLE __MIDL_itf_dxgi_0000_0006_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi_0000_0006_v0_0_s_ifspec;

#ifndef __IDXGIAdapter_INTERFACE_DEFINED__
#define __IDXGIAdapter_INTERFACE_DEFINED__

  /* interface IDXGIAdapter */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIAdapter;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("2411e7e1-12ac-4ccf-bd14-9798e8534dc0")
    IDXGIAdapter : public IDXGIObject
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE EnumOutputs( 
      /* [in] */ UINT Output,
      /* [annotation][out][in] */ 
      _Out_  IDXGIOutput **ppOutput) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetDesc( 
      /* [annotation][out] */ 
      _Out_  DXGI_ADAPTER_DESC *pDesc) = 0;

    virtual HRESULT STDMETHODCALLTYPE CheckInterfaceSupport( 
      /* [annotation][in] */ 
      _In_  REFGUID InterfaceName,
      /* [annotation][out] */ 
      _Out_  LARGE_INTEGER *pUMDVersion) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIAdapterVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIAdapter * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIAdapter * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIAdapter * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGIAdapter * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGIAdapter * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGIAdapter * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGIAdapter * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *EnumOutputs )( 
      IDXGIAdapter * This,
      /* [in] */ UINT Output,
      /* [annotation][out][in] */ 
      _Out_  IDXGIOutput **ppOutput);

    HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
      IDXGIAdapter * This,
      /* [annotation][out] */ 
      _Out_  DXGI_ADAPTER_DESC *pDesc);

    HRESULT ( STDMETHODCALLTYPE *CheckInterfaceSupport )( 
      IDXGIAdapter * This,
      /* [annotation][in] */ 
      _In_  REFGUID InterfaceName,
      /* [annotation][out] */ 
      _Out_  LARGE_INTEGER *pUMDVersion);

    END_INTERFACE
  } IDXGIAdapterVtbl;

  interface IDXGIAdapter
  {
    CONST_VTBL struct IDXGIAdapterVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIAdapter_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIAdapter_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIAdapter_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIAdapter_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIAdapter_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIAdapter_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIAdapter_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIAdapter_EnumOutputs(This,Output,ppOutput)	\
    ( (This)->lpVtbl -> EnumOutputs(This,Output,ppOutput) ) 

#define IDXGIAdapter_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGIAdapter_CheckInterfaceSupport(This,InterfaceName,pUMDVersion)	\
    ( (This)->lpVtbl -> CheckInterfaceSupport(This,InterfaceName,pUMDVersion) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIAdapter_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_dxgi_0000_0007 */
  /* [local] */ 

#define	DXGI_ENUM_MODES_INTERLACED	( 1UL )

#define	DXGI_ENUM_MODES_SCALING	( 2UL )



  extern RPC_IF_HANDLE __MIDL_itf_dxgi_0000_0007_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi_0000_0007_v0_0_s_ifspec;

#ifndef __IDXGIOutput_INTERFACE_DEFINED__
#define __IDXGIOutput_INTERFACE_DEFINED__

  /* interface IDXGIOutput */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIOutput;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("ae02eedb-c735-4690-8d52-5a8dc20213aa")
    IDXGIOutput : public IDXGIObject
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetDesc( 
      /* [annotation][out] */ 
      _Out_  DXGI_OUTPUT_DESC *pDesc) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetDisplayModeList( 
      /* [in] */ DXGI_FORMAT EnumFormat,
      /* [in] */ UINT Flags,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pNumModes,
      /* [annotation][out] */ 
      _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC *pDesc) = 0;

    virtual HRESULT STDMETHODCALLTYPE FindClosestMatchingMode( 
      /* [annotation][in] */ 
      _In_  const DXGI_MODE_DESC *pModeToMatch,
      /* [annotation][out] */ 
      _Out_  DXGI_MODE_DESC *pClosestMatch,
      /* [annotation][in] */ 
      _In_opt_  IUnknown *pConcernedDevice) = 0;

    virtual HRESULT STDMETHODCALLTYPE WaitForVBlank( void) = 0;

    virtual HRESULT STDMETHODCALLTYPE TakeOwnership( 
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      BOOL Exclusive) = 0;

    virtual void STDMETHODCALLTYPE ReleaseOwnership( void) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetGammaControlCapabilities( 
      /* [annotation][out] */ 
      _Out_  DXGI_GAMMA_CONTROL_CAPABILITIES *pGammaCaps) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetGammaControl( 
      /* [annotation][in] */ 
      _In_  const DXGI_GAMMA_CONTROL *pArray) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetGammaControl( 
      /* [annotation][out] */ 
      _Out_  DXGI_GAMMA_CONTROL *pArray) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetDisplaySurface( 
      /* [annotation][in] */ 
      _In_  IDXGISurface *pScanoutSurface) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetDisplaySurfaceData( 
      /* [annotation][in] */ 
      _In_  IDXGISurface *pDestination) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics( 
      /* [annotation][out] */ 
      _Out_  DXGI_FRAME_STATISTICS *pStats) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIOutputVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIOutput * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIOutput * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIOutput * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGIOutput * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGIOutput * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGIOutput * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGIOutput * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
      IDXGIOutput * This,
      /* [annotation][out] */ 
      _Out_  DXGI_OUTPUT_DESC *pDesc);

    HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList )( 
      IDXGIOutput * This,
      /* [in] */ DXGI_FORMAT EnumFormat,
      /* [in] */ UINT Flags,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pNumModes,
      /* [annotation][out] */ 
      _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC *pDesc);

    HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode )( 
      IDXGIOutput * This,
      /* [annotation][in] */ 
      _In_  const DXGI_MODE_DESC *pModeToMatch,
      /* [annotation][out] */ 
      _Out_  DXGI_MODE_DESC *pClosestMatch,
      /* [annotation][in] */ 
      _In_opt_  IUnknown *pConcernedDevice);

    HRESULT ( STDMETHODCALLTYPE *WaitForVBlank )( 
      IDXGIOutput * This);

    HRESULT ( STDMETHODCALLTYPE *TakeOwnership )( 
      IDXGIOutput * This,
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      BOOL Exclusive);

    void ( STDMETHODCALLTYPE *ReleaseOwnership )( 
      IDXGIOutput * This);

    HRESULT ( STDMETHODCALLTYPE *GetGammaControlCapabilities )( 
      IDXGIOutput * This,
      /* [annotation][out] */ 
      _Out_  DXGI_GAMMA_CONTROL_CAPABILITIES *pGammaCaps);

    HRESULT ( STDMETHODCALLTYPE *SetGammaControl )( 
      IDXGIOutput * This,
      /* [annotation][in] */ 
      _In_  const DXGI_GAMMA_CONTROL *pArray);

    HRESULT ( STDMETHODCALLTYPE *GetGammaControl )( 
      IDXGIOutput * This,
      /* [annotation][out] */ 
      _Out_  DXGI_GAMMA_CONTROL *pArray);

    HRESULT ( STDMETHODCALLTYPE *SetDisplaySurface )( 
      IDXGIOutput * This,
      /* [annotation][in] */ 
      _In_  IDXGISurface *pScanoutSurface);

    HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData )( 
      IDXGIOutput * This,
      /* [annotation][in] */ 
      _In_  IDXGISurface *pDestination);

    HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
      IDXGIOutput * This,
      /* [annotation][out] */ 
      _Out_  DXGI_FRAME_STATISTICS *pStats);

    END_INTERFACE
  } IDXGIOutputVtbl;

  interface IDXGIOutput
  {
    CONST_VTBL struct IDXGIOutputVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIOutput_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIOutput_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIOutput_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIOutput_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIOutput_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIOutput_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIOutput_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIOutput_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGIOutput_GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput_FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput_WaitForVBlank(This)	\
    ( (This)->lpVtbl -> WaitForVBlank(This) ) 

#define IDXGIOutput_TakeOwnership(This,pDevice,Exclusive)	\
    ( (This)->lpVtbl -> TakeOwnership(This,pDevice,Exclusive) ) 

#define IDXGIOutput_ReleaseOwnership(This)	\
    ( (This)->lpVtbl -> ReleaseOwnership(This) ) 

#define IDXGIOutput_GetGammaControlCapabilities(This,pGammaCaps)	\
    ( (This)->lpVtbl -> GetGammaControlCapabilities(This,pGammaCaps) ) 

#define IDXGIOutput_SetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> SetGammaControl(This,pArray) ) 

#define IDXGIOutput_GetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> GetGammaControl(This,pArray) ) 

#define IDXGIOutput_SetDisplaySurface(This,pScanoutSurface)	\
    ( (This)->lpVtbl -> SetDisplaySurface(This,pScanoutSurface) ) 

#define IDXGIOutput_GetDisplaySurfaceData(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData(This,pDestination) ) 

#define IDXGIOutput_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIOutput_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_dxgi_0000_0008 */
  /* [local] */ 

#define DXGI_MAX_SWAP_CHAIN_BUFFERS        ( 16 )
#define DXGI_PRESENT_TEST                      0x00000001UL
#define DXGI_PRESENT_DO_NOT_SEQUENCE           0x00000002UL
#define DXGI_PRESENT_RESTART                   0x00000004UL
#define DXGI_PRESENT_DO_NOT_WAIT               0x00000008UL
#define DXGI_PRESENT_STEREO_PREFER_RIGHT       0x00000010UL
#define DXGI_PRESENT_STEREO_TEMPORARY_MONO     0x00000020UL
#define DXGI_PRESENT_RESTRICT_TO_OUTPUT        0x00000040UL
#define DXGI_PRESENT_USE_DURATION              0x00000100UL


  extern RPC_IF_HANDLE __MIDL_itf_dxgi_0000_0008_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi_0000_0008_v0_0_s_ifspec;

#ifndef __IDXGISwapChain_INTERFACE_DEFINED__
#define __IDXGISwapChain_INTERFACE_DEFINED__

  /* interface IDXGISwapChain */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGISwapChain;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("310d36a0-d2e7-4c0a-aa04-6a9d23b8886a")
    IDXGISwapChain : public IDXGIDeviceSubObject
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE Present( 
      /* [in] */ UINT SyncInterval,
      /* [in] */ UINT Flags) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetBuffer( 
      /* [in] */ UINT Buffer,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][out][in] */ 
      _Out_  void **ppSurface) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetFullscreenState( 
      /* [in] */ BOOL Fullscreen,
      /* [annotation][in] */ 
      _In_opt_  IDXGIOutput *pTarget) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetFullscreenState( 
      /* [annotation][out] */ 
      _Out_opt_  BOOL *pFullscreen,
      /* [annotation][out] */ 
      _Out_opt_  IDXGIOutput **ppTarget) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetDesc( 
      /* [annotation][out] */ 
      _Out_  DXGI_SWAP_CHAIN_DESC *pDesc) = 0;

    virtual HRESULT STDMETHODCALLTYPE ResizeBuffers( 
      /* [in] */ UINT BufferCount,
      /* [in] */ UINT Width,
      /* [in] */ UINT Height,
      /* [in] */ DXGI_FORMAT NewFormat,
      /* [in] */ UINT SwapChainFlags) = 0;

    virtual HRESULT STDMETHODCALLTYPE ResizeTarget( 
      /* [annotation][in] */ 
      _In_  const DXGI_MODE_DESC *pNewTargetParameters) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetContainingOutput( 
      /* [annotation][out] */ 
      _Out_  IDXGIOutput **ppOutput) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics( 
      /* [annotation][out] */ 
      _Out_  DXGI_FRAME_STATISTICS *pStats) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount( 
      /* [annotation][out] */ 
      _Out_  UINT *pLastPresentCount) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGISwapChainVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGISwapChain * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGISwapChain * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGISwapChain * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGISwapChain * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGISwapChain * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGISwapChain * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGISwapChain * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      IDXGISwapChain * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppDevice);

    HRESULT ( STDMETHODCALLTYPE *Present )( 
      IDXGISwapChain * This,
      /* [in] */ UINT SyncInterval,
      /* [in] */ UINT Flags);

    HRESULT ( STDMETHODCALLTYPE *GetBuffer )( 
      IDXGISwapChain * This,
      /* [in] */ UINT Buffer,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][out][in] */ 
      _Out_  void **ppSurface);

    HRESULT ( STDMETHODCALLTYPE *SetFullscreenState )( 
      IDXGISwapChain * This,
      /* [in] */ BOOL Fullscreen,
      /* [annotation][in] */ 
      _In_opt_  IDXGIOutput *pTarget);

    HRESULT ( STDMETHODCALLTYPE *GetFullscreenState )( 
      IDXGISwapChain * This,
      /* [annotation][out] */ 
      _Out_opt_  BOOL *pFullscreen,
      /* [annotation][out] */ 
      _Out_opt_  IDXGIOutput **ppTarget);

    HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
      IDXGISwapChain * This,
      /* [annotation][out] */ 
      _Out_  DXGI_SWAP_CHAIN_DESC *pDesc);

    HRESULT ( STDMETHODCALLTYPE *ResizeBuffers )( 
      IDXGISwapChain * This,
      /* [in] */ UINT BufferCount,
      /* [in] */ UINT Width,
      /* [in] */ UINT Height,
      /* [in] */ DXGI_FORMAT NewFormat,
      /* [in] */ UINT SwapChainFlags);

    HRESULT ( STDMETHODCALLTYPE *ResizeTarget )( 
      IDXGISwapChain * This,
      /* [annotation][in] */ 
      _In_  const DXGI_MODE_DESC *pNewTargetParameters);

    HRESULT ( STDMETHODCALLTYPE *GetContainingOutput )( 
      IDXGISwapChain * This,
      /* [annotation][out] */ 
      _Out_  IDXGIOutput **ppOutput);

    HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
      IDXGISwapChain * This,
      /* [annotation][out] */ 
      _Out_  DXGI_FRAME_STATISTICS *pStats);

    HRESULT ( STDMETHODCALLTYPE *GetLastPresentCount )( 
      IDXGISwapChain * This,
      /* [annotation][out] */ 
      _Out_  UINT *pLastPresentCount);

    END_INTERFACE
  } IDXGISwapChainVtbl;

  interface IDXGISwapChain
  {
    CONST_VTBL struct IDXGISwapChainVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGISwapChain_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGISwapChain_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGISwapChain_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGISwapChain_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGISwapChain_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGISwapChain_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGISwapChain_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGISwapChain_GetDevice(This,riid,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppDevice) ) 


#define IDXGISwapChain_Present(This,SyncInterval,Flags)	\
    ( (This)->lpVtbl -> Present(This,SyncInterval,Flags) ) 

#define IDXGISwapChain_GetBuffer(This,Buffer,riid,ppSurface)	\
    ( (This)->lpVtbl -> GetBuffer(This,Buffer,riid,ppSurface) ) 

#define IDXGISwapChain_SetFullscreenState(This,Fullscreen,pTarget)	\
    ( (This)->lpVtbl -> SetFullscreenState(This,Fullscreen,pTarget) ) 

#define IDXGISwapChain_GetFullscreenState(This,pFullscreen,ppTarget)	\
    ( (This)->lpVtbl -> GetFullscreenState(This,pFullscreen,ppTarget) ) 

#define IDXGISwapChain_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGISwapChain_ResizeBuffers(This,BufferCount,Width,Height,NewFormat,SwapChainFlags)	\
    ( (This)->lpVtbl -> ResizeBuffers(This,BufferCount,Width,Height,NewFormat,SwapChainFlags) ) 

#define IDXGISwapChain_ResizeTarget(This,pNewTargetParameters)	\
    ( (This)->lpVtbl -> ResizeTarget(This,pNewTargetParameters) ) 

#define IDXGISwapChain_GetContainingOutput(This,ppOutput)	\
    ( (This)->lpVtbl -> GetContainingOutput(This,ppOutput) ) 

#define IDXGISwapChain_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) ) 

#define IDXGISwapChain_GetLastPresentCount(This,pLastPresentCount)	\
    ( (This)->lpVtbl -> GetLastPresentCount(This,pLastPresentCount) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGISwapChain_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_dxgi_0000_0009 */
  /* [local] */ 

#define DXGI_MWA_NO_WINDOW_CHANGES      ( 1 << 0 )
#define DXGI_MWA_NO_ALT_ENTER           ( 1 << 1 )
#define DXGI_MWA_NO_PRINT_SCREEN        ( 1 << 2 )
#define DXGI_MWA_VALID                  ( 0x7 )


  extern RPC_IF_HANDLE __MIDL_itf_dxgi_0000_0009_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi_0000_0009_v0_0_s_ifspec;

#ifndef __IDXGIFactory_INTERFACE_DEFINED__
#define __IDXGIFactory_INTERFACE_DEFINED__

  /* interface IDXGIFactory */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIFactory;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("7b7166ec-21c7-44ae-b21a-c9ae321ae369")
    IDXGIFactory : public IDXGIObject
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE EnumAdapters( 
      /* [in] */ UINT Adapter,
      /* [annotation][out] */ 
      _Out_  IDXGIAdapter **ppAdapter) = 0;

    virtual HRESULT STDMETHODCALLTYPE MakeWindowAssociation( 
      HWND WindowHandle,
      UINT Flags) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetWindowAssociation( 
      /* [annotation][out] */ 
      _Out_  HWND *pWindowHandle) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateSwapChain( 
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      /* [annotation][in] */ 
      _In_  DXGI_SWAP_CHAIN_DESC *pDesc,
      /* [annotation][out] */ 
      _Out_  IDXGISwapChain **ppSwapChain) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter( 
      /* [in] */ HMODULE Module,
      /* [annotation][out] */ 
      _Out_  IDXGIAdapter **ppAdapter) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIFactoryVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIFactory * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIFactory * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIFactory * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGIFactory * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGIFactory * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGIFactory * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGIFactory * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *EnumAdapters )( 
      IDXGIFactory * This,
      /* [in] */ UINT Adapter,
      /* [annotation][out] */ 
      _Out_  IDXGIAdapter **ppAdapter);

    HRESULT ( STDMETHODCALLTYPE *MakeWindowAssociation )( 
      IDXGIFactory * This,
      HWND WindowHandle,
      UINT Flags);

    HRESULT ( STDMETHODCALLTYPE *GetWindowAssociation )( 
      IDXGIFactory * This,
      /* [annotation][out] */ 
      _Out_  HWND *pWindowHandle);

    HRESULT ( STDMETHODCALLTYPE *CreateSwapChain )( 
      IDXGIFactory * This,
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      /* [annotation][in] */ 
      _In_  DXGI_SWAP_CHAIN_DESC *pDesc,
      /* [annotation][out] */ 
      _Out_  IDXGISwapChain **ppSwapChain);

    HRESULT ( STDMETHODCALLTYPE *CreateSoftwareAdapter )( 
      IDXGIFactory * This,
      /* [in] */ HMODULE Module,
      /* [annotation][out] */ 
      _Out_  IDXGIAdapter **ppAdapter);

    END_INTERFACE
  } IDXGIFactoryVtbl;

  interface IDXGIFactory
  {
    CONST_VTBL struct IDXGIFactoryVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIFactory_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIFactory_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIFactory_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIFactory_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIFactory_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIFactory_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIFactory_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIFactory_EnumAdapters(This,Adapter,ppAdapter)	\
    ( (This)->lpVtbl -> EnumAdapters(This,Adapter,ppAdapter) ) 

#define IDXGIFactory_MakeWindowAssociation(This,WindowHandle,Flags)	\
    ( (This)->lpVtbl -> MakeWindowAssociation(This,WindowHandle,Flags) ) 

#define IDXGIFactory_GetWindowAssociation(This,pWindowHandle)	\
    ( (This)->lpVtbl -> GetWindowAssociation(This,pWindowHandle) ) 

#define IDXGIFactory_CreateSwapChain(This,pDevice,pDesc,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChain(This,pDevice,pDesc,ppSwapChain) ) 

#define IDXGIFactory_CreateSoftwareAdapter(This,Module,ppAdapter)	\
    ( (This)->lpVtbl -> CreateSoftwareAdapter(This,Module,ppAdapter) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIFactory_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_dxgi_0000_0010 */
  /* [local] */ 

  extern RPC_IF_HANDLE __MIDL_itf_dxgi_0000_0010_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi_0000_0010_v0_0_s_ifspec;

#ifndef __IDXGIDevice_INTERFACE_DEFINED__
#define __IDXGIDevice_INTERFACE_DEFINED__

  /* interface IDXGIDevice */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIDevice;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("54ec77fa-1377-44e6-8c32-88fd5f44c84c")
    IDXGIDevice : public IDXGIObject
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetAdapter( 
      /* [annotation][out] */ 
      _Out_  IDXGIAdapter **pAdapter) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateSurface( 
      /* [annotation][in] */ 
      _In_  const DXGI_SURFACE_DESC *pDesc,
      /* [in] */ UINT NumSurfaces,
      /* [in] */ DXGI_USAGE Usage,
      /* [annotation][in] */ 
      _In_opt_  const DXGI_SHARED_RESOURCE *pSharedResource,
      /* [annotation][out] */ 
      _Out_  IDXGISurface **ppSurface) = 0;

    virtual HRESULT STDMETHODCALLTYPE QueryResourceResidency( 
      /* [annotation][size_is][in] */ 
      _In_reads_(NumResources)  IUnknown *const *ppResources,
      /* [annotation][size_is][out] */ 
      _Out_writes_(NumResources)  DXGI_RESIDENCY *pResidencyStatus,
      /* [in] */ UINT NumResources) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetGPUThreadPriority( 
      /* [in] */ INT Priority) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetGPUThreadPriority( 
      /* [annotation][retval][out] */ 
      _Out_  INT *pPriority) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIDeviceVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIDevice * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIDevice * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIDevice * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGIDevice * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGIDevice * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGIDevice * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGIDevice * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *GetAdapter )( 
      IDXGIDevice * This,
      /* [annotation][out] */ 
      _Out_  IDXGIAdapter **pAdapter);

    HRESULT ( STDMETHODCALLTYPE *CreateSurface )( 
      IDXGIDevice * This,
      /* [annotation][in] */ 
      _In_  const DXGI_SURFACE_DESC *pDesc,
      /* [in] */ UINT NumSurfaces,
      /* [in] */ DXGI_USAGE Usage,
      /* [annotation][in] */ 
      _In_opt_  const DXGI_SHARED_RESOURCE *pSharedResource,
      /* [annotation][out] */ 
      _Out_  IDXGISurface **ppSurface);

    HRESULT ( STDMETHODCALLTYPE *QueryResourceResidency )( 
      IDXGIDevice * This,
      /* [annotation][size_is][in] */ 
      _In_reads_(NumResources)  IUnknown *const *ppResources,
      /* [annotation][size_is][out] */ 
      _Out_writes_(NumResources)  DXGI_RESIDENCY *pResidencyStatus,
      /* [in] */ UINT NumResources);

    HRESULT ( STDMETHODCALLTYPE *SetGPUThreadPriority )( 
      IDXGIDevice * This,
      /* [in] */ INT Priority);

    HRESULT ( STDMETHODCALLTYPE *GetGPUThreadPriority )( 
      IDXGIDevice * This,
      /* [annotation][retval][out] */ 
      _Out_  INT *pPriority);

    END_INTERFACE
  } IDXGIDeviceVtbl;

  interface IDXGIDevice
  {
    CONST_VTBL struct IDXGIDeviceVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIDevice_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIDevice_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIDevice_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIDevice_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIDevice_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIDevice_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIDevice_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIDevice_GetAdapter(This,pAdapter)	\
    ( (This)->lpVtbl -> GetAdapter(This,pAdapter) ) 

#define IDXGIDevice_CreateSurface(This,pDesc,NumSurfaces,Usage,pSharedResource,ppSurface)	\
    ( (This)->lpVtbl -> CreateSurface(This,pDesc,NumSurfaces,Usage,pSharedResource,ppSurface) ) 

#define IDXGIDevice_QueryResourceResidency(This,ppResources,pResidencyStatus,NumResources)	\
    ( (This)->lpVtbl -> QueryResourceResidency(This,ppResources,pResidencyStatus,NumResources) ) 

#define IDXGIDevice_SetGPUThreadPriority(This,Priority)	\
    ( (This)->lpVtbl -> SetGPUThreadPriority(This,Priority) ) 

#define IDXGIDevice_GetGPUThreadPriority(This,pPriority)	\
    ( (This)->lpVtbl -> GetGPUThreadPriority(This,pPriority) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIDevice_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_dxgi_0000_0011 */
  /* [local] */ 

  typedef 
  enum DXGI_ADAPTER_FLAG
  {
    DXGI_ADAPTER_FLAG_NONE	= 0,
    DXGI_ADAPTER_FLAG_REMOTE	= 1,
    DXGI_ADAPTER_FLAG_SOFTWARE	= 2,
    DXGI_ADAPTER_FLAG_FORCE_DWORD	= 0xffffffff
  } 	DXGI_ADAPTER_FLAG;

  typedef struct DXGI_ADAPTER_DESC1
  {
    WCHAR Description[ 128 ];
    UINT VendorId;
    UINT DeviceId;
    UINT SubSysId;
    UINT Revision;
    SIZE_T DedicatedVideoMemory;
    SIZE_T DedicatedSystemMemory;
    SIZE_T SharedSystemMemory;
    LUID AdapterLuid;
    UINT Flags;
  } 	DXGI_ADAPTER_DESC1;

  typedef struct DXGI_DISPLAY_COLOR_SPACE
  {
    FLOAT PrimaryCoordinates[ 8 ][ 2 ];
    FLOAT WhitePoints[ 16 ][ 2 ];
  } 	DXGI_DISPLAY_COLOR_SPACE;




  extern RPC_IF_HANDLE __MIDL_itf_dxgi_0000_0011_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi_0000_0011_v0_0_s_ifspec;

#ifndef __IDXGIFactory1_INTERFACE_DEFINED__
#define __IDXGIFactory1_INTERFACE_DEFINED__

  /* interface IDXGIFactory1 */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIFactory1;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("770aae78-f26f-4dba-a829-253c83d1b387")
    IDXGIFactory1 : public IDXGIFactory
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE EnumAdapters1( 
      /* [in] */ UINT Adapter,
      /* [annotation][out] */ 
      _Out_  IDXGIAdapter1 **ppAdapter) = 0;

    virtual BOOL STDMETHODCALLTYPE IsCurrent( void) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIFactory1Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIFactory1 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIFactory1 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIFactory1 * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGIFactory1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGIFactory1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGIFactory1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGIFactory1 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *EnumAdapters )( 
      IDXGIFactory1 * This,
      /* [in] */ UINT Adapter,
      /* [annotation][out] */ 
      _Out_  IDXGIAdapter **ppAdapter);

    HRESULT ( STDMETHODCALLTYPE *MakeWindowAssociation )( 
      IDXGIFactory1 * This,
      HWND WindowHandle,
      UINT Flags);

    HRESULT ( STDMETHODCALLTYPE *GetWindowAssociation )( 
      IDXGIFactory1 * This,
      /* [annotation][out] */ 
      _Out_  HWND *pWindowHandle);

    HRESULT ( STDMETHODCALLTYPE *CreateSwapChain )( 
      IDXGIFactory1 * This,
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      /* [annotation][in] */ 
      _In_  DXGI_SWAP_CHAIN_DESC *pDesc,
      /* [annotation][out] */ 
      _Out_  IDXGISwapChain **ppSwapChain);

    HRESULT ( STDMETHODCALLTYPE *CreateSoftwareAdapter )( 
      IDXGIFactory1 * This,
      /* [in] */ HMODULE Module,
      /* [annotation][out] */ 
      _Out_  IDXGIAdapter **ppAdapter);

    HRESULT ( STDMETHODCALLTYPE *EnumAdapters1 )( 
      IDXGIFactory1 * This,
      /* [in] */ UINT Adapter,
      /* [annotation][out] */ 
      _Out_  IDXGIAdapter1 **ppAdapter);

    BOOL ( STDMETHODCALLTYPE *IsCurrent )( 
      IDXGIFactory1 * This);

    END_INTERFACE
  } IDXGIFactory1Vtbl;

  interface IDXGIFactory1
  {
    CONST_VTBL struct IDXGIFactory1Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIFactory1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIFactory1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIFactory1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIFactory1_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIFactory1_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIFactory1_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIFactory1_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIFactory1_EnumAdapters(This,Adapter,ppAdapter)	\
    ( (This)->lpVtbl -> EnumAdapters(This,Adapter,ppAdapter) ) 

#define IDXGIFactory1_MakeWindowAssociation(This,WindowHandle,Flags)	\
    ( (This)->lpVtbl -> MakeWindowAssociation(This,WindowHandle,Flags) ) 

#define IDXGIFactory1_GetWindowAssociation(This,pWindowHandle)	\
    ( (This)->lpVtbl -> GetWindowAssociation(This,pWindowHandle) ) 

#define IDXGIFactory1_CreateSwapChain(This,pDevice,pDesc,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChain(This,pDevice,pDesc,ppSwapChain) ) 

#define IDXGIFactory1_CreateSoftwareAdapter(This,Module,ppAdapter)	\
    ( (This)->lpVtbl -> CreateSoftwareAdapter(This,Module,ppAdapter) ) 


#define IDXGIFactory1_EnumAdapters1(This,Adapter,ppAdapter)	\
    ( (This)->lpVtbl -> EnumAdapters1(This,Adapter,ppAdapter) ) 

#define IDXGIFactory1_IsCurrent(This)	\
    ( (This)->lpVtbl -> IsCurrent(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIFactory1_INTERFACE_DEFINED__ */


#ifndef __IDXGIAdapter1_INTERFACE_DEFINED__
#define __IDXGIAdapter1_INTERFACE_DEFINED__

  /* interface IDXGIAdapter1 */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIAdapter1;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("29038f61-3839-4626-91fd-086879011a05")
    IDXGIAdapter1 : public IDXGIAdapter
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetDesc1( 
      /* [annotation][out] */ 
      _Out_  DXGI_ADAPTER_DESC1 *pDesc) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIAdapter1Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIAdapter1 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIAdapter1 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIAdapter1 * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGIAdapter1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGIAdapter1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGIAdapter1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGIAdapter1 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *EnumOutputs )( 
      IDXGIAdapter1 * This,
      /* [in] */ UINT Output,
      /* [annotation][out][in] */ 
      _Out_  IDXGIOutput **ppOutput);

    HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
      IDXGIAdapter1 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_ADAPTER_DESC *pDesc);

    HRESULT ( STDMETHODCALLTYPE *CheckInterfaceSupport )( 
      IDXGIAdapter1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID InterfaceName,
      /* [annotation][out] */ 
      _Out_  LARGE_INTEGER *pUMDVersion);

    HRESULT ( STDMETHODCALLTYPE *GetDesc1 )( 
      IDXGIAdapter1 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_ADAPTER_DESC1 *pDesc);

    END_INTERFACE
  } IDXGIAdapter1Vtbl;

  interface IDXGIAdapter1
  {
    CONST_VTBL struct IDXGIAdapter1Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIAdapter1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIAdapter1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIAdapter1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIAdapter1_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIAdapter1_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIAdapter1_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIAdapter1_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIAdapter1_EnumOutputs(This,Output,ppOutput)	\
    ( (This)->lpVtbl -> EnumOutputs(This,Output,ppOutput) ) 

#define IDXGIAdapter1_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGIAdapter1_CheckInterfaceSupport(This,InterfaceName,pUMDVersion)	\
    ( (This)->lpVtbl -> CheckInterfaceSupport(This,InterfaceName,pUMDVersion) ) 


#define IDXGIAdapter1_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIAdapter1_INTERFACE_DEFINED__ */


#ifndef __IDXGIDevice1_INTERFACE_DEFINED__
#define __IDXGIDevice1_INTERFACE_DEFINED__

  /* interface IDXGIDevice1 */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIDevice1;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("77db970f-6276-48ba-ba28-070143b4392c")
    IDXGIDevice1 : public IDXGIDevice
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency( 
      /* [in] */ UINT MaxLatency) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency( 
      /* [annotation][out] */ 
      _Out_  UINT *pMaxLatency) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIDevice1Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIDevice1 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIDevice1 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIDevice1 * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGIDevice1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGIDevice1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGIDevice1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGIDevice1 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *GetAdapter )( 
      IDXGIDevice1 * This,
      /* [annotation][out] */ 
      _Out_  IDXGIAdapter **pAdapter);

    HRESULT ( STDMETHODCALLTYPE *CreateSurface )( 
      IDXGIDevice1 * This,
      /* [annotation][in] */ 
      _In_  const DXGI_SURFACE_DESC *pDesc,
      /* [in] */ UINT NumSurfaces,
      /* [in] */ DXGI_USAGE Usage,
      /* [annotation][in] */ 
      _In_opt_  const DXGI_SHARED_RESOURCE *pSharedResource,
      /* [annotation][out] */ 
      _Out_  IDXGISurface **ppSurface);

    HRESULT ( STDMETHODCALLTYPE *QueryResourceResidency )( 
      IDXGIDevice1 * This,
      /* [annotation][size_is][in] */ 
      _In_reads_(NumResources)  IUnknown *const *ppResources,
      /* [annotation][size_is][out] */ 
      _Out_writes_(NumResources)  DXGI_RESIDENCY *pResidencyStatus,
      /* [in] */ UINT NumResources);

    HRESULT ( STDMETHODCALLTYPE *SetGPUThreadPriority )( 
      IDXGIDevice1 * This,
      /* [in] */ INT Priority);

    HRESULT ( STDMETHODCALLTYPE *GetGPUThreadPriority )( 
      IDXGIDevice1 * This,
      /* [annotation][retval][out] */ 
      _Out_  INT *pPriority);

    HRESULT ( STDMETHODCALLTYPE *SetMaximumFrameLatency )( 
      IDXGIDevice1 * This,
      /* [in] */ UINT MaxLatency);

    HRESULT ( STDMETHODCALLTYPE *GetMaximumFrameLatency )( 
      IDXGIDevice1 * This,
      /* [annotation][out] */ 
      _Out_  UINT *pMaxLatency);

    END_INTERFACE
  } IDXGIDevice1Vtbl;

  interface IDXGIDevice1
  {
    CONST_VTBL struct IDXGIDevice1Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIDevice1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIDevice1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIDevice1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIDevice1_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIDevice1_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIDevice1_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIDevice1_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIDevice1_GetAdapter(This,pAdapter)	\
    ( (This)->lpVtbl -> GetAdapter(This,pAdapter) ) 

#define IDXGIDevice1_CreateSurface(This,pDesc,NumSurfaces,Usage,pSharedResource,ppSurface)	\
    ( (This)->lpVtbl -> CreateSurface(This,pDesc,NumSurfaces,Usage,pSharedResource,ppSurface) ) 

#define IDXGIDevice1_QueryResourceResidency(This,ppResources,pResidencyStatus,NumResources)	\
    ( (This)->lpVtbl -> QueryResourceResidency(This,ppResources,pResidencyStatus,NumResources) ) 

#define IDXGIDevice1_SetGPUThreadPriority(This,Priority)	\
    ( (This)->lpVtbl -> SetGPUThreadPriority(This,Priority) ) 

#define IDXGIDevice1_GetGPUThreadPriority(This,pPriority)	\
    ( (This)->lpVtbl -> GetGPUThreadPriority(This,pPriority) ) 


#define IDXGIDevice1_SetMaximumFrameLatency(This,MaxLatency)	\
    ( (This)->lpVtbl -> SetMaximumFrameLatency(This,MaxLatency) ) 

#define IDXGIDevice1_GetMaximumFrameLatency(This,pMaxLatency)	\
    ( (This)->lpVtbl -> GetMaximumFrameLatency(This,pMaxLatency) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIDevice1_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_dxgi_0000_0014 */
  /* [local] */ 

#ifdef __cplusplus
#endif /*__cplusplus*/
  DEFINE_GUID(IID_IDXGIObject,0xaec22fb8,0x76f3,0x4639,0x9b,0xe0,0x28,0xeb,0x43,0xa6,0x7a,0x2e);
  DEFINE_GUID(IID_IDXGIDeviceSubObject,0x3d3e0379,0xf9de,0x4d58,0xbb,0x6c,0x18,0xd6,0x29,0x92,0xf1,0xa6);
  DEFINE_GUID(IID_IDXGIResource,0x035f3ab4,0x482e,0x4e50,0xb4,0x1f,0x8a,0x7f,0x8b,0xd8,0x96,0x0b);
  DEFINE_GUID(IID_IDXGIKeyedMutex,0x9d8e1289,0xd7b3,0x465f,0x81,0x26,0x25,0x0e,0x34,0x9a,0xf8,0x5d);
  DEFINE_GUID(IID_IDXGISurface,0xcafcb56c,0x6ac3,0x4889,0xbf,0x47,0x9e,0x23,0xbb,0xd2,0x60,0xec);
  DEFINE_GUID(IID_IDXGISurface1,0x4AE63092,0x6327,0x4c1b,0x80,0xAE,0xBF,0xE1,0x2E,0xA3,0x2B,0x86);
  DEFINE_GUID(IID_IDXGIAdapter,0x2411e7e1,0x12ac,0x4ccf,0xbd,0x14,0x97,0x98,0xe8,0x53,0x4d,0xc0);
  DEFINE_GUID(IID_IDXGIOutput,0xae02eedb,0xc735,0x4690,0x8d,0x52,0x5a,0x8d,0xc2,0x02,0x13,0xaa);
  DEFINE_GUID(IID_IDXGISwapChain,0x310d36a0,0xd2e7,0x4c0a,0xaa,0x04,0x6a,0x9d,0x23,0xb8,0x88,0x6a);
  DEFINE_GUID(IID_IDXGIFactory,0x7b7166ec,0x21c7,0x44ae,0xb2,0x1a,0xc9,0xae,0x32,0x1a,0xe3,0x69);
  DEFINE_GUID(IID_IDXGIDevice,0x54ec77fa,0x1377,0x44e6,0x8c,0x32,0x88,0xfd,0x5f,0x44,0xc8,0x4c);
  DEFINE_GUID(IID_IDXGIFactory1,0x770aae78,0xf26f,0x4dba,0xa8,0x29,0x25,0x3c,0x83,0xd1,0xb3,0x87);
  DEFINE_GUID(IID_IDXGIAdapter1,0x29038f61,0x3839,0x4626,0x91,0xfd,0x08,0x68,0x79,0x01,0x1a,0x05);
  DEFINE_GUID(IID_IDXGIDevice1,0x77db970f,0x6276,0x48ba,0xba,0x28,0x07,0x01,0x43,0xb4,0x39,0x2c);


  extern RPC_IF_HANDLE __MIDL_itf_dxgi_0000_0014_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi_0000_0014_v0_0_s_ifspec;

  /* Additional Prototypes for ALL interfaces */

  /* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif




/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 8.00.0603 */
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


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
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __dxgi1_2_h__
#define __dxgi1_2_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDXGIDisplayControl_FWD_DEFINED__
#define __IDXGIDisplayControl_FWD_DEFINED__
typedef interface IDXGIDisplayControl IDXGIDisplayControl;

#endif 	/* __IDXGIDisplayControl_FWD_DEFINED__ */


#ifndef __IDXGIOutputDuplication_FWD_DEFINED__
#define __IDXGIOutputDuplication_FWD_DEFINED__
typedef interface IDXGIOutputDuplication IDXGIOutputDuplication;

#endif 	/* __IDXGIOutputDuplication_FWD_DEFINED__ */


#ifndef __IDXGISurface2_FWD_DEFINED__
#define __IDXGISurface2_FWD_DEFINED__
typedef interface IDXGISurface2 IDXGISurface2;

#endif 	/* __IDXGISurface2_FWD_DEFINED__ */


#ifndef __IDXGIResource1_FWD_DEFINED__
#define __IDXGIResource1_FWD_DEFINED__
typedef interface IDXGIResource1 IDXGIResource1;

#endif 	/* __IDXGIResource1_FWD_DEFINED__ */


#ifndef __IDXGIDevice2_FWD_DEFINED__
#define __IDXGIDevice2_FWD_DEFINED__
typedef interface IDXGIDevice2 IDXGIDevice2;

#endif 	/* __IDXGIDevice2_FWD_DEFINED__ */


#ifndef __IDXGISwapChain1_FWD_DEFINED__
#define __IDXGISwapChain1_FWD_DEFINED__
typedef interface IDXGISwapChain1 IDXGISwapChain1;

#endif 	/* __IDXGISwapChain1_FWD_DEFINED__ */


#ifndef __IDXGIFactory2_FWD_DEFINED__
#define __IDXGIFactory2_FWD_DEFINED__
typedef interface IDXGIFactory2 IDXGIFactory2;

#endif 	/* __IDXGIFactory2_FWD_DEFINED__ */


#ifndef __IDXGIAdapter2_FWD_DEFINED__
#define __IDXGIAdapter2_FWD_DEFINED__
typedef interface IDXGIAdapter2 IDXGIAdapter2;

#endif 	/* __IDXGIAdapter2_FWD_DEFINED__ */


#ifndef __IDXGIOutput1_FWD_DEFINED__
#define __IDXGIOutput1_FWD_DEFINED__
typedef interface IDXGIOutput1 IDXGIOutput1;

#endif 	/* __IDXGIOutput1_FWD_DEFINED__ */


///* header files for imported files */
//#include "dxgi.h"

#ifdef __cplusplus
extern "C"{
#endif 


  /* interface __MIDL_itf_dxgi1_2_0000_0000 */
  /* [local] */ 

//#include <winapifamily.h>
#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)


  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0000_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0000_v0_0_s_ifspec;

#ifndef __IDXGIDisplayControl_INTERFACE_DEFINED__
#define __IDXGIDisplayControl_INTERFACE_DEFINED__

  /* interface IDXGIDisplayControl */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIDisplayControl;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("ea9dbf1a-c88e-4486-854a-98aa0138f30c")
    IDXGIDisplayControl : public IUnknown
  {
  public:
    virtual BOOL STDMETHODCALLTYPE IsStereoEnabled( void) = 0;

    virtual void STDMETHODCALLTYPE SetStereoEnabled( 
      BOOL enabled) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIDisplayControlVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIDisplayControl * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIDisplayControl * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIDisplayControl * This);

    BOOL ( STDMETHODCALLTYPE *IsStereoEnabled )( 
      IDXGIDisplayControl * This);

    void ( STDMETHODCALLTYPE *SetStereoEnabled )( 
      IDXGIDisplayControl * This,
      BOOL enabled);

    END_INTERFACE
  } IDXGIDisplayControlVtbl;

  interface IDXGIDisplayControl
  {
    CONST_VTBL struct IDXGIDisplayControlVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIDisplayControl_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIDisplayControl_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIDisplayControl_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIDisplayControl_IsStereoEnabled(This)	\
    ( (This)->lpVtbl -> IsStereoEnabled(This) ) 

#define IDXGIDisplayControl_SetStereoEnabled(This,enabled)	\
    ( (This)->lpVtbl -> SetStereoEnabled(This,enabled) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIDisplayControl_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_dxgi1_2_0000_0001 */
  /* [local] */ 

  typedef struct DXGI_OUTDUPL_MOVE_RECT
  {
    POINT SourcePoint;
    RECT DestinationRect;
  } 	DXGI_OUTDUPL_MOVE_RECT;

  typedef struct DXGI_OUTDUPL_DESC
  {
    DXGI_MODE_DESC ModeDesc;
    DXGI_MODE_ROTATION Rotation;
    BOOL DesktopImageInSystemMemory;
  } 	DXGI_OUTDUPL_DESC;

  typedef struct DXGI_OUTDUPL_POINTER_POSITION
  {
    POINT Position;
    BOOL Visible;
  } 	DXGI_OUTDUPL_POINTER_POSITION;

  typedef 
  enum DXGI_OUTDUPL_POINTER_SHAPE_TYPE
  {
    DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME	= 0x1,
    DXGI_OUTDUPL_POINTER_SHAPE_TYPE_COLOR	= 0x2,
    DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MASKED_COLOR	= 0x4
  } 	DXGI_OUTDUPL_POINTER_SHAPE_TYPE;

  typedef struct DXGI_OUTDUPL_POINTER_SHAPE_INFO
  {
    UINT Type;
    UINT Width;
    UINT Height;
    UINT Pitch;
    POINT HotSpot;
  } 	DXGI_OUTDUPL_POINTER_SHAPE_INFO;

  typedef struct DXGI_OUTDUPL_FRAME_INFO
  {
    LARGE_INTEGER LastPresentTime;
    LARGE_INTEGER LastMouseUpdateTime;
    UINT AccumulatedFrames;
    BOOL RectsCoalesced;
    BOOL ProtectedContentMaskedOut;
    DXGI_OUTDUPL_POINTER_POSITION PointerPosition;
    UINT TotalMetadataBufferSize;
    UINT PointerShapeBufferSize;
  } 	DXGI_OUTDUPL_FRAME_INFO;



  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0001_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0001_v0_0_s_ifspec;

#ifndef __IDXGIOutputDuplication_INTERFACE_DEFINED__
#define __IDXGIOutputDuplication_INTERFACE_DEFINED__

  /* interface IDXGIOutputDuplication */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIOutputDuplication;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("191cfac3-a341-470d-b26e-a864f428319c")
    IDXGIOutputDuplication : public IDXGIObject
  {
  public:
    virtual void STDMETHODCALLTYPE GetDesc( 
      /* [annotation][out] */ 
      _Out_  DXGI_OUTDUPL_DESC *pDesc) = 0;

    virtual HRESULT STDMETHODCALLTYPE AcquireNextFrame( 
      /* [annotation][in] */ 
      _In_  UINT TimeoutInMilliseconds,
      /* [annotation][out] */ 
      _Out_  DXGI_OUTDUPL_FRAME_INFO *pFrameInfo,
      /* [annotation][out] */ 
      _Out_  IDXGIResource **ppDesktopResource) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetFrameDirtyRects( 
      /* [annotation][in] */ 
      _In_  UINT DirtyRectsBufferSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_to_(DirtyRectsBufferSize, *pDirtyRectsBufferSizeRequired)  RECT *pDirtyRectsBuffer,
      /* [annotation][out] */ 
      _Out_  UINT *pDirtyRectsBufferSizeRequired) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetFrameMoveRects( 
      /* [annotation][in] */ 
      _In_  UINT MoveRectsBufferSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_to_(MoveRectsBufferSize, *pMoveRectsBufferSizeRequired)  DXGI_OUTDUPL_MOVE_RECT *pMoveRectBuffer,
      /* [annotation][out] */ 
      _Out_  UINT *pMoveRectsBufferSizeRequired) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetFramePointerShape( 
      /* [annotation][in] */ 
      _In_  UINT PointerShapeBufferSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_to_(PointerShapeBufferSize, *pPointerShapeBufferSizeRequired)  void *pPointerShapeBuffer,
      /* [annotation][out] */ 
      _Out_  UINT *pPointerShapeBufferSizeRequired,
      /* [annotation][out] */ 
      _Out_  DXGI_OUTDUPL_POINTER_SHAPE_INFO *pPointerShapeInfo) = 0;

    virtual HRESULT STDMETHODCALLTYPE MapDesktopSurface( 
      /* [annotation][out] */ 
      _Out_  DXGI_MAPPED_RECT *pLockedRect) = 0;

    virtual HRESULT STDMETHODCALLTYPE UnMapDesktopSurface( void) = 0;

    virtual HRESULT STDMETHODCALLTYPE ReleaseFrame( void) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIOutputDuplicationVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIOutputDuplication * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIOutputDuplication * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIOutputDuplication * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGIOutputDuplication * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGIOutputDuplication * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGIOutputDuplication * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGIOutputDuplication * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    void ( STDMETHODCALLTYPE *GetDesc )( 
      IDXGIOutputDuplication * This,
      /* [annotation][out] */ 
      _Out_  DXGI_OUTDUPL_DESC *pDesc);

    HRESULT ( STDMETHODCALLTYPE *AcquireNextFrame )( 
      IDXGIOutputDuplication * This,
      /* [annotation][in] */ 
      _In_  UINT TimeoutInMilliseconds,
      /* [annotation][out] */ 
      _Out_  DXGI_OUTDUPL_FRAME_INFO *pFrameInfo,
      /* [annotation][out] */ 
      _Out_  IDXGIResource **ppDesktopResource);

    HRESULT ( STDMETHODCALLTYPE *GetFrameDirtyRects )( 
      IDXGIOutputDuplication * This,
      /* [annotation][in] */ 
      _In_  UINT DirtyRectsBufferSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_to_(DirtyRectsBufferSize, *pDirtyRectsBufferSizeRequired)  RECT *pDirtyRectsBuffer,
      /* [annotation][out] */ 
      _Out_  UINT *pDirtyRectsBufferSizeRequired);

    HRESULT ( STDMETHODCALLTYPE *GetFrameMoveRects )( 
      IDXGIOutputDuplication * This,
      /* [annotation][in] */ 
      _In_  UINT MoveRectsBufferSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_to_(MoveRectsBufferSize, *pMoveRectsBufferSizeRequired)  DXGI_OUTDUPL_MOVE_RECT *pMoveRectBuffer,
      /* [annotation][out] */ 
      _Out_  UINT *pMoveRectsBufferSizeRequired);

    HRESULT ( STDMETHODCALLTYPE *GetFramePointerShape )( 
      IDXGIOutputDuplication * This,
      /* [annotation][in] */ 
      _In_  UINT PointerShapeBufferSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_to_(PointerShapeBufferSize, *pPointerShapeBufferSizeRequired)  void *pPointerShapeBuffer,
      /* [annotation][out] */ 
      _Out_  UINT *pPointerShapeBufferSizeRequired,
      /* [annotation][out] */ 
      _Out_  DXGI_OUTDUPL_POINTER_SHAPE_INFO *pPointerShapeInfo);

    HRESULT ( STDMETHODCALLTYPE *MapDesktopSurface )( 
      IDXGIOutputDuplication * This,
      /* [annotation][out] */ 
      _Out_  DXGI_MAPPED_RECT *pLockedRect);

    HRESULT ( STDMETHODCALLTYPE *UnMapDesktopSurface )( 
      IDXGIOutputDuplication * This);

    HRESULT ( STDMETHODCALLTYPE *ReleaseFrame )( 
      IDXGIOutputDuplication * This);

    END_INTERFACE
  } IDXGIOutputDuplicationVtbl;

  interface IDXGIOutputDuplication
  {
    CONST_VTBL struct IDXGIOutputDuplicationVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIOutputDuplication_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIOutputDuplication_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIOutputDuplication_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIOutputDuplication_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIOutputDuplication_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIOutputDuplication_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIOutputDuplication_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIOutputDuplication_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGIOutputDuplication_AcquireNextFrame(This,TimeoutInMilliseconds,pFrameInfo,ppDesktopResource)	\
    ( (This)->lpVtbl -> AcquireNextFrame(This,TimeoutInMilliseconds,pFrameInfo,ppDesktopResource) ) 

#define IDXGIOutputDuplication_GetFrameDirtyRects(This,DirtyRectsBufferSize,pDirtyRectsBuffer,pDirtyRectsBufferSizeRequired)	\
    ( (This)->lpVtbl -> GetFrameDirtyRects(This,DirtyRectsBufferSize,pDirtyRectsBuffer,pDirtyRectsBufferSizeRequired) ) 

#define IDXGIOutputDuplication_GetFrameMoveRects(This,MoveRectsBufferSize,pMoveRectBuffer,pMoveRectsBufferSizeRequired)	\
    ( (This)->lpVtbl -> GetFrameMoveRects(This,MoveRectsBufferSize,pMoveRectBuffer,pMoveRectsBufferSizeRequired) ) 

#define IDXGIOutputDuplication_GetFramePointerShape(This,PointerShapeBufferSize,pPointerShapeBuffer,pPointerShapeBufferSizeRequired,pPointerShapeInfo)	\
    ( (This)->lpVtbl -> GetFramePointerShape(This,PointerShapeBufferSize,pPointerShapeBuffer,pPointerShapeBufferSizeRequired,pPointerShapeInfo) ) 

#define IDXGIOutputDuplication_MapDesktopSurface(This,pLockedRect)	\
    ( (This)->lpVtbl -> MapDesktopSurface(This,pLockedRect) ) 

#define IDXGIOutputDuplication_UnMapDesktopSurface(This)	\
    ( (This)->lpVtbl -> UnMapDesktopSurface(This) ) 

#define IDXGIOutputDuplication_ReleaseFrame(This)	\
    ( (This)->lpVtbl -> ReleaseFrame(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIOutputDuplication_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_dxgi1_2_0000_0002 */
  /* [local] */ 

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion
#pragma region App Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
  typedef 
  enum DXGI_ALPHA_MODE
  {
    DXGI_ALPHA_MODE_UNSPECIFIED	= 0,
    DXGI_ALPHA_MODE_PREMULTIPLIED	= 1,
    DXGI_ALPHA_MODE_STRAIGHT	= 2,
    DXGI_ALPHA_MODE_IGNORE	= 3,
    DXGI_ALPHA_MODE_FORCE_DWORD	= 0xffffffff
  } 	DXGI_ALPHA_MODE;



  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0002_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0002_v0_0_s_ifspec;

#ifndef __IDXGISurface2_INTERFACE_DEFINED__
#define __IDXGISurface2_INTERFACE_DEFINED__

  /* interface IDXGISurface2 */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGISurface2;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("aba496dd-b617-4cb8-a866-bc44d7eb1fa2")
    IDXGISurface2 : public IDXGISurface1
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetResource( 
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][out] */ 
      _Out_  void **ppParentResource,
      /* [annotation][out] */ 
      _Out_  UINT *pSubresourceIndex) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGISurface2Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGISurface2 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGISurface2 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGISurface2 * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGISurface2 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGISurface2 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGISurface2 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGISurface2 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      IDXGISurface2 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppDevice);

    HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
      IDXGISurface2 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_SURFACE_DESC *pDesc);

    HRESULT ( STDMETHODCALLTYPE *Map )( 
      IDXGISurface2 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_MAPPED_RECT *pLockedRect,
      /* [in] */ UINT MapFlags);

    HRESULT ( STDMETHODCALLTYPE *Unmap )( 
      IDXGISurface2 * This);

    HRESULT ( STDMETHODCALLTYPE *GetDC )( 
      IDXGISurface2 * This,
      /* [in] */ BOOL Discard,
      /* [annotation][out] */ 
      _Out_  HDC *phdc);

    HRESULT ( STDMETHODCALLTYPE *ReleaseDC )( 
      IDXGISurface2 * This,
      /* [annotation][in] */ 
      _In_opt_  RECT *pDirtyRect);

    HRESULT ( STDMETHODCALLTYPE *GetResource )( 
      IDXGISurface2 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][out] */ 
      _Out_  void **ppParentResource,
      /* [annotation][out] */ 
      _Out_  UINT *pSubresourceIndex);

    END_INTERFACE
  } IDXGISurface2Vtbl;

  interface IDXGISurface2
  {
    CONST_VTBL struct IDXGISurface2Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGISurface2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGISurface2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGISurface2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGISurface2_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGISurface2_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGISurface2_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGISurface2_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGISurface2_GetDevice(This,riid,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppDevice) ) 


#define IDXGISurface2_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGISurface2_Map(This,pLockedRect,MapFlags)	\
    ( (This)->lpVtbl -> Map(This,pLockedRect,MapFlags) ) 

#define IDXGISurface2_Unmap(This)	\
    ( (This)->lpVtbl -> Unmap(This) ) 


#define IDXGISurface2_GetDC(This,Discard,phdc)	\
    ( (This)->lpVtbl -> GetDC(This,Discard,phdc) ) 

#define IDXGISurface2_ReleaseDC(This,pDirtyRect)	\
    ( (This)->lpVtbl -> ReleaseDC(This,pDirtyRect) ) 


#define IDXGISurface2_GetResource(This,riid,ppParentResource,pSubresourceIndex)	\
    ( (This)->lpVtbl -> GetResource(This,riid,ppParentResource,pSubresourceIndex) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGISurface2_INTERFACE_DEFINED__ */


#ifndef __IDXGIResource1_INTERFACE_DEFINED__
#define __IDXGIResource1_INTERFACE_DEFINED__

  /* interface IDXGIResource1 */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIResource1;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("30961379-4609-4a41-998e-54fe567ee0c1")
    IDXGIResource1 : public IDXGIResource
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE CreateSubresourceSurface( 
      UINT index,
      /* [annotation][out] */ 
      _Out_  IDXGISurface2 **ppSurface) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateSharedHandle( 
      /* [annotation][in] */ 
      _In_opt_  const SECURITY_ATTRIBUTES *pAttributes,
      /* [annotation][in] */ 
      _In_  DWORD dwAccess,
      /* [annotation][in] */ 
      _In_opt_  LPCWSTR lpName,
      /* [annotation][out] */ 
      _Out_  HANDLE *pHandle) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIResource1Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIResource1 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIResource1 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIResource1 * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGIResource1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGIResource1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGIResource1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGIResource1 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      IDXGIResource1 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppDevice);

    HRESULT ( STDMETHODCALLTYPE *GetSharedHandle )( 
      IDXGIResource1 * This,
      /* [annotation][out] */ 
      _Out_  HANDLE *pSharedHandle);

    HRESULT ( STDMETHODCALLTYPE *GetUsage )( 
      IDXGIResource1 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_USAGE *pUsage);

    HRESULT ( STDMETHODCALLTYPE *SetEvictionPriority )( 
      IDXGIResource1 * This,
      /* [in] */ UINT EvictionPriority);

    HRESULT ( STDMETHODCALLTYPE *GetEvictionPriority )( 
      IDXGIResource1 * This,
      /* [annotation][retval][out] */ 
      _Out_  UINT *pEvictionPriority);

    HRESULT ( STDMETHODCALLTYPE *CreateSubresourceSurface )( 
      IDXGIResource1 * This,
      UINT index,
      /* [annotation][out] */ 
      _Out_  IDXGISurface2 **ppSurface);

    HRESULT ( STDMETHODCALLTYPE *CreateSharedHandle )( 
      IDXGIResource1 * This,
      /* [annotation][in] */ 
      _In_opt_  const SECURITY_ATTRIBUTES *pAttributes,
      /* [annotation][in] */ 
      _In_  DWORD dwAccess,
      /* [annotation][in] */ 
      _In_opt_  LPCWSTR lpName,
      /* [annotation][out] */ 
      _Out_  HANDLE *pHandle);

    END_INTERFACE
  } IDXGIResource1Vtbl;

  interface IDXGIResource1
  {
    CONST_VTBL struct IDXGIResource1Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIResource1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIResource1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIResource1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIResource1_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIResource1_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIResource1_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIResource1_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIResource1_GetDevice(This,riid,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppDevice) ) 


#define IDXGIResource1_GetSharedHandle(This,pSharedHandle)	\
    ( (This)->lpVtbl -> GetSharedHandle(This,pSharedHandle) ) 

#define IDXGIResource1_GetUsage(This,pUsage)	\
    ( (This)->lpVtbl -> GetUsage(This,pUsage) ) 

#define IDXGIResource1_SetEvictionPriority(This,EvictionPriority)	\
    ( (This)->lpVtbl -> SetEvictionPriority(This,EvictionPriority) ) 

#define IDXGIResource1_GetEvictionPriority(This,pEvictionPriority)	\
    ( (This)->lpVtbl -> GetEvictionPriority(This,pEvictionPriority) ) 


#define IDXGIResource1_CreateSubresourceSurface(This,index,ppSurface)	\
    ( (This)->lpVtbl -> CreateSubresourceSurface(This,index,ppSurface) ) 

#define IDXGIResource1_CreateSharedHandle(This,pAttributes,dwAccess,lpName,pHandle)	\
    ( (This)->lpVtbl -> CreateSharedHandle(This,pAttributes,dwAccess,lpName,pHandle) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIResource1_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_dxgi1_2_0000_0004 */
  /* [local] */ 

  typedef 
  enum _DXGI_OFFER_RESOURCE_PRIORITY
  {
    DXGI_OFFER_RESOURCE_PRIORITY_LOW	= 1,
    DXGI_OFFER_RESOURCE_PRIORITY_NORMAL	= ( DXGI_OFFER_RESOURCE_PRIORITY_LOW + 1 ) ,
    DXGI_OFFER_RESOURCE_PRIORITY_HIGH	= ( DXGI_OFFER_RESOURCE_PRIORITY_NORMAL + 1 ) 
  } 	DXGI_OFFER_RESOURCE_PRIORITY;



  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0004_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0004_v0_0_s_ifspec;

#ifndef __IDXGIDevice2_INTERFACE_DEFINED__
#define __IDXGIDevice2_INTERFACE_DEFINED__

  /* interface IDXGIDevice2 */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIDevice2;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("05008617-fbfd-4051-a790-144884b4f6a9")
    IDXGIDevice2 : public IDXGIDevice1
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE OfferResources( 
      /* [annotation][in] */ 
      _In_  UINT NumResources,
      /* [annotation][size_is][in] */ 
      _In_reads_(NumResources)  IDXGIResource *const *ppResources,
      /* [annotation][in] */ 
      _In_  DXGI_OFFER_RESOURCE_PRIORITY Priority) = 0;

    virtual HRESULT STDMETHODCALLTYPE ReclaimResources( 
      /* [annotation][in] */ 
      _In_  UINT NumResources,
      /* [annotation][size_is][in] */ 
      _In_reads_(NumResources)  IDXGIResource *const *ppResources,
      /* [annotation][size_is][out] */ 
      _Out_writes_all_opt_(NumResources)  BOOL *pDiscarded) = 0;

    virtual HRESULT STDMETHODCALLTYPE EnqueueSetEvent( 
      /* [annotation][in] */ 
      _In_  HANDLE hEvent) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIDevice2Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIDevice2 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIDevice2 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIDevice2 * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGIDevice2 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGIDevice2 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGIDevice2 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGIDevice2 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *GetAdapter )( 
      IDXGIDevice2 * This,
      /* [annotation][out] */ 
      _Out_  IDXGIAdapter **pAdapter);

    HRESULT ( STDMETHODCALLTYPE *CreateSurface )( 
      IDXGIDevice2 * This,
      /* [annotation][in] */ 
      _In_  const DXGI_SURFACE_DESC *pDesc,
      /* [in] */ UINT NumSurfaces,
      /* [in] */ DXGI_USAGE Usage,
      /* [annotation][in] */ 
      _In_opt_  const DXGI_SHARED_RESOURCE *pSharedResource,
      /* [annotation][out] */ 
      _Out_  IDXGISurface **ppSurface);

    HRESULT ( STDMETHODCALLTYPE *QueryResourceResidency )( 
      IDXGIDevice2 * This,
      /* [annotation][size_is][in] */ 
      _In_reads_(NumResources)  IUnknown *const *ppResources,
      /* [annotation][size_is][out] */ 
      _Out_writes_(NumResources)  DXGI_RESIDENCY *pResidencyStatus,
      /* [in] */ UINT NumResources);

    HRESULT ( STDMETHODCALLTYPE *SetGPUThreadPriority )( 
      IDXGIDevice2 * This,
      /* [in] */ INT Priority);

    HRESULT ( STDMETHODCALLTYPE *GetGPUThreadPriority )( 
      IDXGIDevice2 * This,
      /* [annotation][retval][out] */ 
      _Out_  INT *pPriority);

    HRESULT ( STDMETHODCALLTYPE *SetMaximumFrameLatency )( 
      IDXGIDevice2 * This,
      /* [in] */ UINT MaxLatency);

    HRESULT ( STDMETHODCALLTYPE *GetMaximumFrameLatency )( 
      IDXGIDevice2 * This,
      /* [annotation][out] */ 
      _Out_  UINT *pMaxLatency);

    HRESULT ( STDMETHODCALLTYPE *OfferResources )( 
      IDXGIDevice2 * This,
      /* [annotation][in] */ 
      _In_  UINT NumResources,
      /* [annotation][size_is][in] */ 
      _In_reads_(NumResources)  IDXGIResource *const *ppResources,
      /* [annotation][in] */ 
      _In_  DXGI_OFFER_RESOURCE_PRIORITY Priority);

    HRESULT ( STDMETHODCALLTYPE *ReclaimResources )( 
      IDXGIDevice2 * This,
      /* [annotation][in] */ 
      _In_  UINT NumResources,
      /* [annotation][size_is][in] */ 
      _In_reads_(NumResources)  IDXGIResource *const *ppResources,
      /* [annotation][size_is][out] */ 
      _Out_writes_all_opt_(NumResources)  BOOL *pDiscarded);

    HRESULT ( STDMETHODCALLTYPE *EnqueueSetEvent )( 
      IDXGIDevice2 * This,
      /* [annotation][in] */ 
      _In_  HANDLE hEvent);

    END_INTERFACE
  } IDXGIDevice2Vtbl;

  interface IDXGIDevice2
  {
    CONST_VTBL struct IDXGIDevice2Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIDevice2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIDevice2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIDevice2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIDevice2_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIDevice2_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIDevice2_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIDevice2_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIDevice2_GetAdapter(This,pAdapter)	\
    ( (This)->lpVtbl -> GetAdapter(This,pAdapter) ) 

#define IDXGIDevice2_CreateSurface(This,pDesc,NumSurfaces,Usage,pSharedResource,ppSurface)	\
    ( (This)->lpVtbl -> CreateSurface(This,pDesc,NumSurfaces,Usage,pSharedResource,ppSurface) ) 

#define IDXGIDevice2_QueryResourceResidency(This,ppResources,pResidencyStatus,NumResources)	\
    ( (This)->lpVtbl -> QueryResourceResidency(This,ppResources,pResidencyStatus,NumResources) ) 

#define IDXGIDevice2_SetGPUThreadPriority(This,Priority)	\
    ( (This)->lpVtbl -> SetGPUThreadPriority(This,Priority) ) 

#define IDXGIDevice2_GetGPUThreadPriority(This,pPriority)	\
    ( (This)->lpVtbl -> GetGPUThreadPriority(This,pPriority) ) 


#define IDXGIDevice2_SetMaximumFrameLatency(This,MaxLatency)	\
    ( (This)->lpVtbl -> SetMaximumFrameLatency(This,MaxLatency) ) 

#define IDXGIDevice2_GetMaximumFrameLatency(This,pMaxLatency)	\
    ( (This)->lpVtbl -> GetMaximumFrameLatency(This,pMaxLatency) ) 


#define IDXGIDevice2_OfferResources(This,NumResources,ppResources,Priority)	\
    ( (This)->lpVtbl -> OfferResources(This,NumResources,ppResources,Priority) ) 

#define IDXGIDevice2_ReclaimResources(This,NumResources,ppResources,pDiscarded)	\
    ( (This)->lpVtbl -> ReclaimResources(This,NumResources,ppResources,pDiscarded) ) 

#define IDXGIDevice2_EnqueueSetEvent(This,hEvent)	\
    ( (This)->lpVtbl -> EnqueueSetEvent(This,hEvent) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIDevice2_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_dxgi1_2_0000_0005 */
  /* [local] */ 

#define	DXGI_ENUM_MODES_STEREO	( 4UL )

#define	DXGI_ENUM_MODES_DISABLED_STEREO	( 8UL )

#define	DXGI_SHARED_RESOURCE_READ	( 0x80000000L )

#define	DXGI_SHARED_RESOURCE_WRITE	( 1 )

  typedef struct DXGI_MODE_DESC1
  {
    UINT Width;
    UINT Height;
    DXGI_RATIONAL RefreshRate;
    DXGI_FORMAT Format;
    DXGI_MODE_SCANLINE_ORDER ScanlineOrdering;
    DXGI_MODE_SCALING Scaling;
    BOOL Stereo;
  } 	DXGI_MODE_DESC1;

  typedef 
  enum DXGI_SCALING
  {
    DXGI_SCALING_STRETCH	= 0,
    DXGI_SCALING_NONE	= 1,
    DXGI_SCALING_ASPECT_RATIO_STRETCH	= 2
  } 	DXGI_SCALING;

  typedef struct DXGI_SWAP_CHAIN_DESC1
  {
    UINT Width;
    UINT Height;
    DXGI_FORMAT Format;
    BOOL Stereo;
    DXGI_SAMPLE_DESC SampleDesc;
    DXGI_USAGE BufferUsage;
    UINT BufferCount;
    DXGI_SCALING Scaling;
    DXGI_SWAP_EFFECT SwapEffect;
    DXGI_ALPHA_MODE AlphaMode;
    UINT Flags;
  } 	DXGI_SWAP_CHAIN_DESC1;

  typedef struct DXGI_SWAP_CHAIN_FULLSCREEN_DESC
  {
    DXGI_RATIONAL RefreshRate;
    DXGI_MODE_SCANLINE_ORDER ScanlineOrdering;
    DXGI_MODE_SCALING Scaling;
    BOOL Windowed;
  } 	DXGI_SWAP_CHAIN_FULLSCREEN_DESC;

  typedef struct DXGI_PRESENT_PARAMETERS
  {
    UINT DirtyRectsCount;
    RECT *pDirtyRects;
    RECT *pScrollRect;
    POINT *pScrollOffset;
  } 	DXGI_PRESENT_PARAMETERS;



  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0005_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0005_v0_0_s_ifspec;

#ifndef __IDXGISwapChain1_INTERFACE_DEFINED__
#define __IDXGISwapChain1_INTERFACE_DEFINED__

  /* interface IDXGISwapChain1 */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGISwapChain1;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("790a45f7-0d42-4876-983a-0a55cfe6f4aa")
    IDXGISwapChain1 : public IDXGISwapChain
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetDesc1( 
      /* [annotation][out] */ 
      _Out_  DXGI_SWAP_CHAIN_DESC1 *pDesc) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetFullscreenDesc( 
      /* [annotation][out] */ 
      _Out_  DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetHwnd( 
      /* [annotation][out] */ 
      _Out_  HWND *pHwnd) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetCoreWindow( 
      /* [annotation][in] */ 
      _In_  REFIID refiid,
      /* [annotation][out] */ 
      _Out_  void **ppUnk) = 0;

    virtual HRESULT STDMETHODCALLTYPE Present1( 
      /* [in] */ UINT SyncInterval,
      /* [in] */ UINT PresentFlags,
      /* [annotation][in] */ 
      _In_  const DXGI_PRESENT_PARAMETERS *pPresentParameters) = 0;

    virtual BOOL STDMETHODCALLTYPE IsTemporaryMonoSupported( void) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetRestrictToOutput( 
      /* [annotation][out] */ 
      _Out_  IDXGIOutput **ppRestrictToOutput) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor( 
      /* [annotation][in] */ 
      _In_  const DXGI_RGBA *pColor) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetBackgroundColor( 
      /* [annotation][out] */ 
      _Out_  DXGI_RGBA *pColor) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetRotation( 
      /* [annotation][in] */ 
      _In_  DXGI_MODE_ROTATION Rotation) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetRotation( 
      /* [annotation][out] */ 
      _Out_  DXGI_MODE_ROTATION *pRotation) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGISwapChain1Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGISwapChain1 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGISwapChain1 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGISwapChain1 * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGISwapChain1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGISwapChain1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGISwapChain1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGISwapChain1 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      IDXGISwapChain1 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppDevice);

    HRESULT ( STDMETHODCALLTYPE *Present )( 
      IDXGISwapChain1 * This,
      /* [in] */ UINT SyncInterval,
      /* [in] */ UINT Flags);

    HRESULT ( STDMETHODCALLTYPE *GetBuffer )( 
      IDXGISwapChain1 * This,
      /* [in] */ UINT Buffer,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][out][in] */ 
      _Out_  void **ppSurface);

    HRESULT ( STDMETHODCALLTYPE *SetFullscreenState )( 
      IDXGISwapChain1 * This,
      /* [in] */ BOOL Fullscreen,
      /* [annotation][in] */ 
      _In_opt_  IDXGIOutput *pTarget);

    HRESULT ( STDMETHODCALLTYPE *GetFullscreenState )( 
      IDXGISwapChain1 * This,
      /* [annotation][out] */ 
      _Out_opt_  BOOL *pFullscreen,
      /* [annotation][out] */ 
      _Out_opt_  IDXGIOutput **ppTarget);

    HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
      IDXGISwapChain1 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_SWAP_CHAIN_DESC *pDesc);

    HRESULT ( STDMETHODCALLTYPE *ResizeBuffers )( 
      IDXGISwapChain1 * This,
      /* [in] */ UINT BufferCount,
      /* [in] */ UINT Width,
      /* [in] */ UINT Height,
      /* [in] */ DXGI_FORMAT NewFormat,
      /* [in] */ UINT SwapChainFlags);

    HRESULT ( STDMETHODCALLTYPE *ResizeTarget )( 
      IDXGISwapChain1 * This,
      /* [annotation][in] */ 
      _In_  const DXGI_MODE_DESC *pNewTargetParameters);

    HRESULT ( STDMETHODCALLTYPE *GetContainingOutput )( 
      IDXGISwapChain1 * This,
      /* [annotation][out] */ 
      _Out_  IDXGIOutput **ppOutput);

    HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
      IDXGISwapChain1 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_FRAME_STATISTICS *pStats);

    HRESULT ( STDMETHODCALLTYPE *GetLastPresentCount )( 
      IDXGISwapChain1 * This,
      /* [annotation][out] */ 
      _Out_  UINT *pLastPresentCount);

    HRESULT ( STDMETHODCALLTYPE *GetDesc1 )( 
      IDXGISwapChain1 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_SWAP_CHAIN_DESC1 *pDesc);

    HRESULT ( STDMETHODCALLTYPE *GetFullscreenDesc )( 
      IDXGISwapChain1 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc);

    HRESULT ( STDMETHODCALLTYPE *GetHwnd )( 
      IDXGISwapChain1 * This,
      /* [annotation][out] */ 
      _Out_  HWND *pHwnd);

    HRESULT ( STDMETHODCALLTYPE *GetCoreWindow )( 
      IDXGISwapChain1 * This,
      /* [annotation][in] */ 
      _In_  REFIID refiid,
      /* [annotation][out] */ 
      _Out_  void **ppUnk);

    HRESULT ( STDMETHODCALLTYPE *Present1 )( 
      IDXGISwapChain1 * This,
      /* [in] */ UINT SyncInterval,
      /* [in] */ UINT PresentFlags,
      /* [annotation][in] */ 
      _In_  const DXGI_PRESENT_PARAMETERS *pPresentParameters);

    BOOL ( STDMETHODCALLTYPE *IsTemporaryMonoSupported )( 
      IDXGISwapChain1 * This);

    HRESULT ( STDMETHODCALLTYPE *GetRestrictToOutput )( 
      IDXGISwapChain1 * This,
      /* [annotation][out] */ 
      _Out_  IDXGIOutput **ppRestrictToOutput);

    HRESULT ( STDMETHODCALLTYPE *SetBackgroundColor )( 
      IDXGISwapChain1 * This,
      /* [annotation][in] */ 
      _In_  const DXGI_RGBA *pColor);

    HRESULT ( STDMETHODCALLTYPE *GetBackgroundColor )( 
      IDXGISwapChain1 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_RGBA *pColor);

    HRESULT ( STDMETHODCALLTYPE *SetRotation )( 
      IDXGISwapChain1 * This,
      /* [annotation][in] */ 
      _In_  DXGI_MODE_ROTATION Rotation);

    HRESULT ( STDMETHODCALLTYPE *GetRotation )( 
      IDXGISwapChain1 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_MODE_ROTATION *pRotation);

    END_INTERFACE
  } IDXGISwapChain1Vtbl;

  interface IDXGISwapChain1
  {
    CONST_VTBL struct IDXGISwapChain1Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGISwapChain1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGISwapChain1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGISwapChain1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGISwapChain1_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGISwapChain1_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGISwapChain1_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGISwapChain1_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGISwapChain1_GetDevice(This,riid,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppDevice) ) 


#define IDXGISwapChain1_Present(This,SyncInterval,Flags)	\
    ( (This)->lpVtbl -> Present(This,SyncInterval,Flags) ) 

#define IDXGISwapChain1_GetBuffer(This,Buffer,riid,ppSurface)	\
    ( (This)->lpVtbl -> GetBuffer(This,Buffer,riid,ppSurface) ) 

#define IDXGISwapChain1_SetFullscreenState(This,Fullscreen,pTarget)	\
    ( (This)->lpVtbl -> SetFullscreenState(This,Fullscreen,pTarget) ) 

#define IDXGISwapChain1_GetFullscreenState(This,pFullscreen,ppTarget)	\
    ( (This)->lpVtbl -> GetFullscreenState(This,pFullscreen,ppTarget) ) 

#define IDXGISwapChain1_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGISwapChain1_ResizeBuffers(This,BufferCount,Width,Height,NewFormat,SwapChainFlags)	\
    ( (This)->lpVtbl -> ResizeBuffers(This,BufferCount,Width,Height,NewFormat,SwapChainFlags) ) 

#define IDXGISwapChain1_ResizeTarget(This,pNewTargetParameters)	\
    ( (This)->lpVtbl -> ResizeTarget(This,pNewTargetParameters) ) 

#define IDXGISwapChain1_GetContainingOutput(This,ppOutput)	\
    ( (This)->lpVtbl -> GetContainingOutput(This,ppOutput) ) 

#define IDXGISwapChain1_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) ) 

#define IDXGISwapChain1_GetLastPresentCount(This,pLastPresentCount)	\
    ( (This)->lpVtbl -> GetLastPresentCount(This,pLastPresentCount) ) 


#define IDXGISwapChain1_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 

#define IDXGISwapChain1_GetFullscreenDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetFullscreenDesc(This,pDesc) ) 

#define IDXGISwapChain1_GetHwnd(This,pHwnd)	\
    ( (This)->lpVtbl -> GetHwnd(This,pHwnd) ) 

#define IDXGISwapChain1_GetCoreWindow(This,refiid,ppUnk)	\
    ( (This)->lpVtbl -> GetCoreWindow(This,refiid,ppUnk) ) 

#define IDXGISwapChain1_Present1(This,SyncInterval,PresentFlags,pPresentParameters)	\
    ( (This)->lpVtbl -> Present1(This,SyncInterval,PresentFlags,pPresentParameters) ) 

#define IDXGISwapChain1_IsTemporaryMonoSupported(This)	\
    ( (This)->lpVtbl -> IsTemporaryMonoSupported(This) ) 

#define IDXGISwapChain1_GetRestrictToOutput(This,ppRestrictToOutput)	\
    ( (This)->lpVtbl -> GetRestrictToOutput(This,ppRestrictToOutput) ) 

#define IDXGISwapChain1_SetBackgroundColor(This,pColor)	\
    ( (This)->lpVtbl -> SetBackgroundColor(This,pColor) ) 

#define IDXGISwapChain1_GetBackgroundColor(This,pColor)	\
    ( (This)->lpVtbl -> GetBackgroundColor(This,pColor) ) 

#define IDXGISwapChain1_SetRotation(This,Rotation)	\
    ( (This)->lpVtbl -> SetRotation(This,Rotation) ) 

#define IDXGISwapChain1_GetRotation(This,pRotation)	\
    ( (This)->lpVtbl -> GetRotation(This,pRotation) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGISwapChain1_INTERFACE_DEFINED__ */


#ifndef __IDXGIFactory2_INTERFACE_DEFINED__
#define __IDXGIFactory2_INTERFACE_DEFINED__

  /* interface IDXGIFactory2 */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIFactory2;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("50c83a1c-e072-4c48-87b0-3630fa36a6d0")
    IDXGIFactory2 : public IDXGIFactory1
  {
  public:
    virtual BOOL STDMETHODCALLTYPE IsWindowedStereoEnabled( void) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForHwnd( 
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      /* [annotation][in] */ 
      _In_  HWND hWnd,
      /* [annotation][in] */ 
      _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
      /* [annotation][in] */ 
      _In_opt_  const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc,
      /* [annotation][in] */ 
      _In_opt_  IDXGIOutput *pRestrictToOutput,
      /* [annotation][out] */ 
      _Out_  IDXGISwapChain1 **ppSwapChain) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForCoreWindow( 
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      /* [annotation][in] */ 
      _In_  IUnknown *pWindow,
      /* [annotation][in] */ 
      _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
      /* [annotation][in] */ 
      _In_opt_  IDXGIOutput *pRestrictToOutput,
      /* [annotation][out] */ 
      _Out_  IDXGISwapChain1 **ppSwapChain) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetSharedResourceAdapterLuid( 
      /* [annotation] */ 
      _In_  HANDLE hResource,
      /* [annotation] */ 
      _Out_  LUID *pLuid) = 0;

    virtual HRESULT STDMETHODCALLTYPE RegisterStereoStatusWindow( 
      /* [annotation][in] */ 
      _In_  HWND WindowHandle,
      /* [annotation][in] */ 
      _In_  UINT wMsg,
      /* [annotation][out] */ 
      _Out_  DWORD *pdwCookie) = 0;

    virtual HRESULT STDMETHODCALLTYPE RegisterStereoStatusEvent( 
      /* [annotation][in] */ 
      _In_  HANDLE hEvent,
      /* [annotation][out] */ 
      _Out_  DWORD *pdwCookie) = 0;

    virtual void STDMETHODCALLTYPE UnregisterStereoStatus( 
      /* [annotation][in] */ 
      _In_  DWORD dwCookie) = 0;

    virtual HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusWindow( 
      /* [annotation][in] */ 
      _In_  HWND WindowHandle,
      /* [annotation][in] */ 
      _In_  UINT wMsg,
      /* [annotation][out] */ 
      _Out_  DWORD *pdwCookie) = 0;

    virtual HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusEvent( 
      /* [annotation][in] */ 
      _In_  HANDLE hEvent,
      /* [annotation][out] */ 
      _Out_  DWORD *pdwCookie) = 0;

    virtual void STDMETHODCALLTYPE UnregisterOcclusionStatus( 
      /* [annotation][in] */ 
      _In_  DWORD dwCookie) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForComposition( 
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      /* [annotation][in] */ 
      _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
      /* [annotation][in] */ 
      _In_opt_  IDXGIOutput *pRestrictToOutput,
      /* [annotation][out] */ 
      _Outptr_  IDXGISwapChain1 **ppSwapChain) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIFactory2Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIFactory2 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIFactory2 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIFactory2 * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGIFactory2 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGIFactory2 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGIFactory2 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGIFactory2 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *EnumAdapters )( 
      IDXGIFactory2 * This,
      /* [in] */ UINT Adapter,
      /* [annotation][out] */ 
      _Out_  IDXGIAdapter **ppAdapter);

    HRESULT ( STDMETHODCALLTYPE *MakeWindowAssociation )( 
      IDXGIFactory2 * This,
      HWND WindowHandle,
      UINT Flags);

    HRESULT ( STDMETHODCALLTYPE *GetWindowAssociation )( 
      IDXGIFactory2 * This,
      /* [annotation][out] */ 
      _Out_  HWND *pWindowHandle);

    HRESULT ( STDMETHODCALLTYPE *CreateSwapChain )( 
      IDXGIFactory2 * This,
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      /* [annotation][in] */ 
      _In_  DXGI_SWAP_CHAIN_DESC *pDesc,
      /* [annotation][out] */ 
      _Out_  IDXGISwapChain **ppSwapChain);

    HRESULT ( STDMETHODCALLTYPE *CreateSoftwareAdapter )( 
      IDXGIFactory2 * This,
      /* [in] */ HMODULE Module,
      /* [annotation][out] */ 
      _Out_  IDXGIAdapter **ppAdapter);

    HRESULT ( STDMETHODCALLTYPE *EnumAdapters1 )( 
      IDXGIFactory2 * This,
      /* [in] */ UINT Adapter,
      /* [annotation][out] */ 
      _Out_  IDXGIAdapter1 **ppAdapter);

    BOOL ( STDMETHODCALLTYPE *IsCurrent )( 
      IDXGIFactory2 * This);

    BOOL ( STDMETHODCALLTYPE *IsWindowedStereoEnabled )( 
      IDXGIFactory2 * This);

    HRESULT ( STDMETHODCALLTYPE *CreateSwapChainForHwnd )( 
      IDXGIFactory2 * This,
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      /* [annotation][in] */ 
      _In_  HWND hWnd,
      /* [annotation][in] */ 
      _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
      /* [annotation][in] */ 
      _In_opt_  const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc,
      /* [annotation][in] */ 
      _In_opt_  IDXGIOutput *pRestrictToOutput,
      /* [annotation][out] */ 
      _Out_  IDXGISwapChain1 **ppSwapChain);

    HRESULT ( STDMETHODCALLTYPE *CreateSwapChainForCoreWindow )( 
      IDXGIFactory2 * This,
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      /* [annotation][in] */ 
      _In_  IUnknown *pWindow,
      /* [annotation][in] */ 
      _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
      /* [annotation][in] */ 
      _In_opt_  IDXGIOutput *pRestrictToOutput,
      /* [annotation][out] */ 
      _Out_  IDXGISwapChain1 **ppSwapChain);

    HRESULT ( STDMETHODCALLTYPE *GetSharedResourceAdapterLuid )( 
      IDXGIFactory2 * This,
      /* [annotation] */ 
      _In_  HANDLE hResource,
      /* [annotation] */ 
      _Out_  LUID *pLuid);

    HRESULT ( STDMETHODCALLTYPE *RegisterStereoStatusWindow )( 
      IDXGIFactory2 * This,
      /* [annotation][in] */ 
      _In_  HWND WindowHandle,
      /* [annotation][in] */ 
      _In_  UINT wMsg,
      /* [annotation][out] */ 
      _Out_  DWORD *pdwCookie);

    HRESULT ( STDMETHODCALLTYPE *RegisterStereoStatusEvent )( 
      IDXGIFactory2 * This,
      /* [annotation][in] */ 
      _In_  HANDLE hEvent,
      /* [annotation][out] */ 
      _Out_  DWORD *pdwCookie);

    void ( STDMETHODCALLTYPE *UnregisterStereoStatus )( 
      IDXGIFactory2 * This,
      /* [annotation][in] */ 
      _In_  DWORD dwCookie);

    HRESULT ( STDMETHODCALLTYPE *RegisterOcclusionStatusWindow )( 
      IDXGIFactory2 * This,
      /* [annotation][in] */ 
      _In_  HWND WindowHandle,
      /* [annotation][in] */ 
      _In_  UINT wMsg,
      /* [annotation][out] */ 
      _Out_  DWORD *pdwCookie);

    HRESULT ( STDMETHODCALLTYPE *RegisterOcclusionStatusEvent )( 
      IDXGIFactory2 * This,
      /* [annotation][in] */ 
      _In_  HANDLE hEvent,
      /* [annotation][out] */ 
      _Out_  DWORD *pdwCookie);

    void ( STDMETHODCALLTYPE *UnregisterOcclusionStatus )( 
      IDXGIFactory2 * This,
      /* [annotation][in] */ 
      _In_  DWORD dwCookie);

    HRESULT ( STDMETHODCALLTYPE *CreateSwapChainForComposition )( 
      IDXGIFactory2 * This,
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      /* [annotation][in] */ 
      _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
      /* [annotation][in] */ 
      _In_opt_  IDXGIOutput *pRestrictToOutput,
      /* [annotation][out] */ 
      _Outptr_  IDXGISwapChain1 **ppSwapChain);

    END_INTERFACE
  } IDXGIFactory2Vtbl;

  interface IDXGIFactory2
  {
    CONST_VTBL struct IDXGIFactory2Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIFactory2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIFactory2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIFactory2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIFactory2_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIFactory2_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIFactory2_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIFactory2_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIFactory2_EnumAdapters(This,Adapter,ppAdapter)	\
    ( (This)->lpVtbl -> EnumAdapters(This,Adapter,ppAdapter) ) 

#define IDXGIFactory2_MakeWindowAssociation(This,WindowHandle,Flags)	\
    ( (This)->lpVtbl -> MakeWindowAssociation(This,WindowHandle,Flags) ) 

#define IDXGIFactory2_GetWindowAssociation(This,pWindowHandle)	\
    ( (This)->lpVtbl -> GetWindowAssociation(This,pWindowHandle) ) 

#define IDXGIFactory2_CreateSwapChain(This,pDevice,pDesc,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChain(This,pDevice,pDesc,ppSwapChain) ) 

#define IDXGIFactory2_CreateSoftwareAdapter(This,Module,ppAdapter)	\
    ( (This)->lpVtbl -> CreateSoftwareAdapter(This,Module,ppAdapter) ) 


#define IDXGIFactory2_EnumAdapters1(This,Adapter,ppAdapter)	\
    ( (This)->lpVtbl -> EnumAdapters1(This,Adapter,ppAdapter) ) 

#define IDXGIFactory2_IsCurrent(This)	\
    ( (This)->lpVtbl -> IsCurrent(This) ) 


#define IDXGIFactory2_IsWindowedStereoEnabled(This)	\
    ( (This)->lpVtbl -> IsWindowedStereoEnabled(This) ) 

#define IDXGIFactory2_CreateSwapChainForHwnd(This,pDevice,hWnd,pDesc,pFullscreenDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForHwnd(This,pDevice,hWnd,pDesc,pFullscreenDesc,pRestrictToOutput,ppSwapChain) ) 

#define IDXGIFactory2_CreateSwapChainForCoreWindow(This,pDevice,pWindow,pDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForCoreWindow(This,pDevice,pWindow,pDesc,pRestrictToOutput,ppSwapChain) ) 

#define IDXGIFactory2_GetSharedResourceAdapterLuid(This,hResource,pLuid)	\
    ( (This)->lpVtbl -> GetSharedResourceAdapterLuid(This,hResource,pLuid) ) 

#define IDXGIFactory2_RegisterStereoStatusWindow(This,WindowHandle,wMsg,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterStereoStatusWindow(This,WindowHandle,wMsg,pdwCookie) ) 

#define IDXGIFactory2_RegisterStereoStatusEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterStereoStatusEvent(This,hEvent,pdwCookie) ) 

#define IDXGIFactory2_UnregisterStereoStatus(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterStereoStatus(This,dwCookie) ) 

#define IDXGIFactory2_RegisterOcclusionStatusWindow(This,WindowHandle,wMsg,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterOcclusionStatusWindow(This,WindowHandle,wMsg,pdwCookie) ) 

#define IDXGIFactory2_RegisterOcclusionStatusEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterOcclusionStatusEvent(This,hEvent,pdwCookie) ) 

#define IDXGIFactory2_UnregisterOcclusionStatus(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterOcclusionStatus(This,dwCookie) ) 

#define IDXGIFactory2_CreateSwapChainForComposition(This,pDevice,pDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForComposition(This,pDevice,pDesc,pRestrictToOutput,ppSwapChain) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIFactory2_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_dxgi1_2_0000_0007 */
  /* [local] */ 

  typedef 
  enum DXGI_GRAPHICS_PREEMPTION_GRANULARITY
  {
    DXGI_GRAPHICS_PREEMPTION_DMA_BUFFER_BOUNDARY	= 0,
    DXGI_GRAPHICS_PREEMPTION_PRIMITIVE_BOUNDARY	= 1,
    DXGI_GRAPHICS_PREEMPTION_TRIANGLE_BOUNDARY	= 2,
    DXGI_GRAPHICS_PREEMPTION_PIXEL_BOUNDARY	= 3,
    DXGI_GRAPHICS_PREEMPTION_INSTRUCTION_BOUNDARY	= 4
  } 	DXGI_GRAPHICS_PREEMPTION_GRANULARITY;

  typedef 
  enum DXGI_COMPUTE_PREEMPTION_GRANULARITY
  {
    DXGI_COMPUTE_PREEMPTION_DMA_BUFFER_BOUNDARY	= 0,
    DXGI_COMPUTE_PREEMPTION_DISPATCH_BOUNDARY	= 1,
    DXGI_COMPUTE_PREEMPTION_THREAD_GROUP_BOUNDARY	= 2,
    DXGI_COMPUTE_PREEMPTION_THREAD_BOUNDARY	= 3,
    DXGI_COMPUTE_PREEMPTION_INSTRUCTION_BOUNDARY	= 4
  } 	DXGI_COMPUTE_PREEMPTION_GRANULARITY;

  typedef struct DXGI_ADAPTER_DESC2
  {
    WCHAR Description[ 128 ];
    UINT VendorId;
    UINT DeviceId;
    UINT SubSysId;
    UINT Revision;
    SIZE_T DedicatedVideoMemory;
    SIZE_T DedicatedSystemMemory;
    SIZE_T SharedSystemMemory;
    LUID AdapterLuid;
    UINT Flags;
    DXGI_GRAPHICS_PREEMPTION_GRANULARITY GraphicsPreemptionGranularity;
    DXGI_COMPUTE_PREEMPTION_GRANULARITY ComputePreemptionGranularity;
  } 	DXGI_ADAPTER_DESC2;



  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0007_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0007_v0_0_s_ifspec;

#ifndef __IDXGIAdapter2_INTERFACE_DEFINED__
#define __IDXGIAdapter2_INTERFACE_DEFINED__

  /* interface IDXGIAdapter2 */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIAdapter2;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("0AA1AE0A-FA0E-4B84-8644-E05FF8E5ACB5")
    IDXGIAdapter2 : public IDXGIAdapter1
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetDesc2( 
      /* [annotation][out] */ 
      _Out_  DXGI_ADAPTER_DESC2 *pDesc) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIAdapter2Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIAdapter2 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIAdapter2 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIAdapter2 * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGIAdapter2 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGIAdapter2 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGIAdapter2 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGIAdapter2 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *EnumOutputs )( 
      IDXGIAdapter2 * This,
      /* [in] */ UINT Output,
      /* [annotation][out][in] */ 
      _Out_  IDXGIOutput **ppOutput);

    HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
      IDXGIAdapter2 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_ADAPTER_DESC *pDesc);

    HRESULT ( STDMETHODCALLTYPE *CheckInterfaceSupport )( 
      IDXGIAdapter2 * This,
      /* [annotation][in] */ 
      _In_  REFGUID InterfaceName,
      /* [annotation][out] */ 
      _Out_  LARGE_INTEGER *pUMDVersion);

    HRESULT ( STDMETHODCALLTYPE *GetDesc1 )( 
      IDXGIAdapter2 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_ADAPTER_DESC1 *pDesc);

    HRESULT ( STDMETHODCALLTYPE *GetDesc2 )( 
      IDXGIAdapter2 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_ADAPTER_DESC2 *pDesc);

    END_INTERFACE
  } IDXGIAdapter2Vtbl;

  interface IDXGIAdapter2
  {
    CONST_VTBL struct IDXGIAdapter2Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIAdapter2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIAdapter2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIAdapter2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIAdapter2_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIAdapter2_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIAdapter2_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIAdapter2_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIAdapter2_EnumOutputs(This,Output,ppOutput)	\
    ( (This)->lpVtbl -> EnumOutputs(This,Output,ppOutput) ) 

#define IDXGIAdapter2_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGIAdapter2_CheckInterfaceSupport(This,InterfaceName,pUMDVersion)	\
    ( (This)->lpVtbl -> CheckInterfaceSupport(This,InterfaceName,pUMDVersion) ) 


#define IDXGIAdapter2_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 


#define IDXGIAdapter2_GetDesc2(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc2(This,pDesc) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIAdapter2_INTERFACE_DEFINED__ */


#ifndef __IDXGIOutput1_INTERFACE_DEFINED__
#define __IDXGIOutput1_INTERFACE_DEFINED__

  /* interface IDXGIOutput1 */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIOutput1;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("00cddea8-939b-4b83-a340-a685226666cc")
    IDXGIOutput1 : public IDXGIOutput
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetDisplayModeList1( 
      /* [in] */ DXGI_FORMAT EnumFormat,
      /* [in] */ UINT Flags,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pNumModes,
      /* [annotation][out] */ 
      _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC1 *pDesc) = 0;

    virtual HRESULT STDMETHODCALLTYPE FindClosestMatchingMode1( 
      /* [annotation][in] */ 
      _In_  const DXGI_MODE_DESC1 *pModeToMatch,
      /* [annotation][out] */ 
      _Out_  DXGI_MODE_DESC1 *pClosestMatch,
      /* [annotation][in] */ 
      _In_opt_  IUnknown *pConcernedDevice) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetDisplaySurfaceData1( 
      /* [annotation][in] */ 
      _In_  IDXGIResource *pDestination) = 0;

    virtual HRESULT STDMETHODCALLTYPE DuplicateOutput( 
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      /* [annotation][out] */ 
      _Out_  IDXGIOutputDuplication **ppOutputDuplication) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIOutput1Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIOutput1 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIOutput1 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIOutput1 * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGIOutput1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGIOutput1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGIOutput1 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGIOutput1 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
      IDXGIOutput1 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_OUTPUT_DESC *pDesc);

    HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList )( 
      IDXGIOutput1 * This,
      /* [in] */ DXGI_FORMAT EnumFormat,
      /* [in] */ UINT Flags,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pNumModes,
      /* [annotation][out] */ 
      _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC *pDesc);

    HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode )( 
      IDXGIOutput1 * This,
      /* [annotation][in] */ 
      _In_  const DXGI_MODE_DESC *pModeToMatch,
      /* [annotation][out] */ 
      _Out_  DXGI_MODE_DESC *pClosestMatch,
      /* [annotation][in] */ 
      _In_opt_  IUnknown *pConcernedDevice);

    HRESULT ( STDMETHODCALLTYPE *WaitForVBlank )( 
      IDXGIOutput1 * This);

    HRESULT ( STDMETHODCALLTYPE *TakeOwnership )( 
      IDXGIOutput1 * This,
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      BOOL Exclusive);

    void ( STDMETHODCALLTYPE *ReleaseOwnership )( 
      IDXGIOutput1 * This);

    HRESULT ( STDMETHODCALLTYPE *GetGammaControlCapabilities )( 
      IDXGIOutput1 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_GAMMA_CONTROL_CAPABILITIES *pGammaCaps);

    HRESULT ( STDMETHODCALLTYPE *SetGammaControl )( 
      IDXGIOutput1 * This,
      /* [annotation][in] */ 
      _In_  const DXGI_GAMMA_CONTROL *pArray);

    HRESULT ( STDMETHODCALLTYPE *GetGammaControl )( 
      IDXGIOutput1 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_GAMMA_CONTROL *pArray);

    HRESULT ( STDMETHODCALLTYPE *SetDisplaySurface )( 
      IDXGIOutput1 * This,
      /* [annotation][in] */ 
      _In_  IDXGISurface *pScanoutSurface);

    HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData )( 
      IDXGIOutput1 * This,
      /* [annotation][in] */ 
      _In_  IDXGISurface *pDestination);

    HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
      IDXGIOutput1 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_FRAME_STATISTICS *pStats);

    HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList1 )( 
      IDXGIOutput1 * This,
      /* [in] */ DXGI_FORMAT EnumFormat,
      /* [in] */ UINT Flags,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pNumModes,
      /* [annotation][out] */ 
      _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC1 *pDesc);

    HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode1 )( 
      IDXGIOutput1 * This,
      /* [annotation][in] */ 
      _In_  const DXGI_MODE_DESC1 *pModeToMatch,
      /* [annotation][out] */ 
      _Out_  DXGI_MODE_DESC1 *pClosestMatch,
      /* [annotation][in] */ 
      _In_opt_  IUnknown *pConcernedDevice);

    HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData1 )( 
      IDXGIOutput1 * This,
      /* [annotation][in] */ 
      _In_  IDXGIResource *pDestination);

    HRESULT ( STDMETHODCALLTYPE *DuplicateOutput )( 
      IDXGIOutput1 * This,
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      /* [annotation][out] */ 
      _Out_  IDXGIOutputDuplication **ppOutputDuplication);

    END_INTERFACE
  } IDXGIOutput1Vtbl;

  interface IDXGIOutput1
  {
    CONST_VTBL struct IDXGIOutput1Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIOutput1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIOutput1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIOutput1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIOutput1_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIOutput1_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIOutput1_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIOutput1_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIOutput1_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGIOutput1_GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput1_FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput1_WaitForVBlank(This)	\
    ( (This)->lpVtbl -> WaitForVBlank(This) ) 

#define IDXGIOutput1_TakeOwnership(This,pDevice,Exclusive)	\
    ( (This)->lpVtbl -> TakeOwnership(This,pDevice,Exclusive) ) 

#define IDXGIOutput1_ReleaseOwnership(This)	\
    ( (This)->lpVtbl -> ReleaseOwnership(This) ) 

#define IDXGIOutput1_GetGammaControlCapabilities(This,pGammaCaps)	\
    ( (This)->lpVtbl -> GetGammaControlCapabilities(This,pGammaCaps) ) 

#define IDXGIOutput1_SetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> SetGammaControl(This,pArray) ) 

#define IDXGIOutput1_GetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> GetGammaControl(This,pArray) ) 

#define IDXGIOutput1_SetDisplaySurface(This,pScanoutSurface)	\
    ( (This)->lpVtbl -> SetDisplaySurface(This,pScanoutSurface) ) 

#define IDXGIOutput1_GetDisplaySurfaceData(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData(This,pDestination) ) 

#define IDXGIOutput1_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) ) 


#define IDXGIOutput1_GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput1_FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput1_GetDisplaySurfaceData1(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData1(This,pDestination) ) 

#define IDXGIOutput1_DuplicateOutput(This,pDevice,ppOutputDuplication)	\
    ( (This)->lpVtbl -> DuplicateOutput(This,pDevice,ppOutputDuplication) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIOutput1_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_dxgi1_2_0000_0009 */
  /* [local] */ 

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion
  DEFINE_GUID(IID_IDXGIDisplayControl,0xea9dbf1a,0xc88e,0x4486,0x85,0x4a,0x98,0xaa,0x01,0x38,0xf3,0x0c);
  DEFINE_GUID(IID_IDXGIOutputDuplication,0x191cfac3,0xa341,0x470d,0xb2,0x6e,0xa8,0x64,0xf4,0x28,0x31,0x9c);
  DEFINE_GUID(IID_IDXGISurface2,0xaba496dd,0xb617,0x4cb8,0xa8,0x66,0xbc,0x44,0xd7,0xeb,0x1f,0xa2);
  DEFINE_GUID(IID_IDXGIResource1,0x30961379,0x4609,0x4a41,0x99,0x8e,0x54,0xfe,0x56,0x7e,0xe0,0xc1);
  DEFINE_GUID(IID_IDXGIDevice2,0x05008617,0xfbfd,0x4051,0xa7,0x90,0x14,0x48,0x84,0xb4,0xf6,0xa9);
  DEFINE_GUID(IID_IDXGISwapChain1,0x790a45f7,0x0d42,0x4876,0x98,0x3a,0x0a,0x55,0xcf,0xe6,0xf4,0xaa);
  DEFINE_GUID(IID_IDXGIFactory2,0x50c83a1c,0xe072,0x4c48,0x87,0xb0,0x36,0x30,0xfa,0x36,0xa6,0xd0);
  DEFINE_GUID(IID_IDXGIAdapter2,0x0AA1AE0A,0xFA0E,0x4B84,0x86,0x44,0xE0,0x5F,0xF8,0xE5,0xAC,0xB5);
  DEFINE_GUID(IID_IDXGIOutput1,0x00cddea8,0x939b,0x4b83,0xa3,0x40,0xa6,0x85,0x22,0x66,0x66,0xcc);


  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0009_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_2_0000_0009_v0_0_s_ifspec;

  /* Additional Prototypes for ALL interfaces */

  /* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif




/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 8.00.0603 */
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


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
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __dxgi1_3_h__
#define __dxgi1_3_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDXGIDevice3_FWD_DEFINED__
#define __IDXGIDevice3_FWD_DEFINED__
typedef interface IDXGIDevice3 IDXGIDevice3;

#endif 	/* __IDXGIDevice3_FWD_DEFINED__ */


#ifndef __IDXGISwapChain2_FWD_DEFINED__
#define __IDXGISwapChain2_FWD_DEFINED__
typedef interface IDXGISwapChain2 IDXGISwapChain2;

#endif 	/* __IDXGISwapChain2_FWD_DEFINED__ */


#ifndef __IDXGIOutput2_FWD_DEFINED__
#define __IDXGIOutput2_FWD_DEFINED__
typedef interface IDXGIOutput2 IDXGIOutput2;

#endif 	/* __IDXGIOutput2_FWD_DEFINED__ */


#ifndef __IDXGIFactory3_FWD_DEFINED__
#define __IDXGIFactory3_FWD_DEFINED__
typedef interface IDXGIFactory3 IDXGIFactory3;

#endif 	/* __IDXGIFactory3_FWD_DEFINED__ */


#ifndef __IDXGIDecodeSwapChain_FWD_DEFINED__
#define __IDXGIDecodeSwapChain_FWD_DEFINED__
typedef interface IDXGIDecodeSwapChain IDXGIDecodeSwapChain;

#endif 	/* __IDXGIDecodeSwapChain_FWD_DEFINED__ */


#ifndef __IDXGIFactoryMedia_FWD_DEFINED__
#define __IDXGIFactoryMedia_FWD_DEFINED__
typedef interface IDXGIFactoryMedia IDXGIFactoryMedia;

#endif 	/* __IDXGIFactoryMedia_FWD_DEFINED__ */


#ifndef __IDXGISwapChainMedia_FWD_DEFINED__
#define __IDXGISwapChainMedia_FWD_DEFINED__
typedef interface IDXGISwapChainMedia IDXGISwapChainMedia;

#endif 	/* __IDXGISwapChainMedia_FWD_DEFINED__ */


#ifndef __IDXGIOutput3_FWD_DEFINED__
#define __IDXGIOutput3_FWD_DEFINED__
typedef interface IDXGIOutput3 IDXGIOutput3;

#endif 	/* __IDXGIOutput3_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 


  /* interface __MIDL_itf_dxgi1_3_0000_0000 */
  /* [local] */ 

//#include <winapifamily.h>
#pragma region App Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
#define DXGI_CREATE_FACTORY_DEBUG 0x1
  //HRESULT WINAPI CreateDXGIFactory2(UINT Flags, REFIID riid, _Out_ void **ppFactory);
  HRESULT WINAPI DXGIGetDebugInterface1(UINT Flags, REFIID riid, _COM_Outptr_ void **pDebug);


  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0000_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0000_v0_0_s_ifspec;

#ifndef __IDXGIDevice3_INTERFACE_DEFINED__
#define __IDXGIDevice3_INTERFACE_DEFINED__

  /* interface IDXGIDevice3 */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIDevice3;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("6007896c-3244-4afd-bf18-a6d3beda5023")
    IDXGIDevice3 : public IDXGIDevice2
  {
  public:
    virtual void STDMETHODCALLTYPE Trim( void) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIDevice3Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIDevice3 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIDevice3 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIDevice3 * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGIDevice3 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGIDevice3 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGIDevice3 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGIDevice3 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *GetAdapter )( 
      IDXGIDevice3 * This,
      /* [annotation][out] */ 
      _Out_  IDXGIAdapter **pAdapter);

    HRESULT ( STDMETHODCALLTYPE *CreateSurface )( 
      IDXGIDevice3 * This,
      /* [annotation][in] */ 
      _In_  const DXGI_SURFACE_DESC *pDesc,
      /* [in] */ UINT NumSurfaces,
      /* [in] */ DXGI_USAGE Usage,
      /* [annotation][in] */ 
      _In_opt_  const DXGI_SHARED_RESOURCE *pSharedResource,
      /* [annotation][out] */ 
      _Out_  IDXGISurface **ppSurface);

    HRESULT ( STDMETHODCALLTYPE *QueryResourceResidency )( 
      IDXGIDevice3 * This,
      /* [annotation][size_is][in] */ 
      _In_reads_(NumResources)  IUnknown *const *ppResources,
      /* [annotation][size_is][out] */ 
      _Out_writes_(NumResources)  DXGI_RESIDENCY *pResidencyStatus,
      /* [in] */ UINT NumResources);

    HRESULT ( STDMETHODCALLTYPE *SetGPUThreadPriority )( 
      IDXGIDevice3 * This,
      /* [in] */ INT Priority);

    HRESULT ( STDMETHODCALLTYPE *GetGPUThreadPriority )( 
      IDXGIDevice3 * This,
      /* [annotation][retval][out] */ 
      _Out_  INT *pPriority);

    HRESULT ( STDMETHODCALLTYPE *SetMaximumFrameLatency )( 
      IDXGIDevice3 * This,
      /* [in] */ UINT MaxLatency);

    HRESULT ( STDMETHODCALLTYPE *GetMaximumFrameLatency )( 
      IDXGIDevice3 * This,
      /* [annotation][out] */ 
      _Out_  UINT *pMaxLatency);

    HRESULT ( STDMETHODCALLTYPE *OfferResources )( 
      IDXGIDevice3 * This,
      /* [annotation][in] */ 
      _In_  UINT NumResources,
      /* [annotation][size_is][in] */ 
      _In_reads_(NumResources)  IDXGIResource *const *ppResources,
      /* [annotation][in] */ 
      _In_  DXGI_OFFER_RESOURCE_PRIORITY Priority);

    HRESULT ( STDMETHODCALLTYPE *ReclaimResources )( 
      IDXGIDevice3 * This,
      /* [annotation][in] */ 
      _In_  UINT NumResources,
      /* [annotation][size_is][in] */ 
      _In_reads_(NumResources)  IDXGIResource *const *ppResources,
      /* [annotation][size_is][out] */ 
      _Out_writes_all_opt_(NumResources)  BOOL *pDiscarded);

    HRESULT ( STDMETHODCALLTYPE *EnqueueSetEvent )( 
      IDXGIDevice3 * This,
      /* [annotation][in] */ 
      _In_  HANDLE hEvent);

    void ( STDMETHODCALLTYPE *Trim )( 
      IDXGIDevice3 * This);

    END_INTERFACE
  } IDXGIDevice3Vtbl;

  interface IDXGIDevice3
  {
    CONST_VTBL struct IDXGIDevice3Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIDevice3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIDevice3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIDevice3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIDevice3_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIDevice3_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIDevice3_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIDevice3_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIDevice3_GetAdapter(This,pAdapter)	\
    ( (This)->lpVtbl -> GetAdapter(This,pAdapter) ) 

#define IDXGIDevice3_CreateSurface(This,pDesc,NumSurfaces,Usage,pSharedResource,ppSurface)	\
    ( (This)->lpVtbl -> CreateSurface(This,pDesc,NumSurfaces,Usage,pSharedResource,ppSurface) ) 

#define IDXGIDevice3_QueryResourceResidency(This,ppResources,pResidencyStatus,NumResources)	\
    ( (This)->lpVtbl -> QueryResourceResidency(This,ppResources,pResidencyStatus,NumResources) ) 

#define IDXGIDevice3_SetGPUThreadPriority(This,Priority)	\
    ( (This)->lpVtbl -> SetGPUThreadPriority(This,Priority) ) 

#define IDXGIDevice3_GetGPUThreadPriority(This,pPriority)	\
    ( (This)->lpVtbl -> GetGPUThreadPriority(This,pPriority) ) 


#define IDXGIDevice3_SetMaximumFrameLatency(This,MaxLatency)	\
    ( (This)->lpVtbl -> SetMaximumFrameLatency(This,MaxLatency) ) 

#define IDXGIDevice3_GetMaximumFrameLatency(This,pMaxLatency)	\
    ( (This)->lpVtbl -> GetMaximumFrameLatency(This,pMaxLatency) ) 


#define IDXGIDevice3_OfferResources(This,NumResources,ppResources,Priority)	\
    ( (This)->lpVtbl -> OfferResources(This,NumResources,ppResources,Priority) ) 

#define IDXGIDevice3_ReclaimResources(This,NumResources,ppResources,pDiscarded)	\
    ( (This)->lpVtbl -> ReclaimResources(This,NumResources,ppResources,pDiscarded) ) 

#define IDXGIDevice3_EnqueueSetEvent(This,hEvent)	\
    ( (This)->lpVtbl -> EnqueueSetEvent(This,hEvent) ) 


#define IDXGIDevice3_Trim(This)	\
    ( (This)->lpVtbl -> Trim(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIDevice3_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_dxgi1_3_0000_0001 */
  /* [local] */ 

  typedef struct DXGI_MATRIX_3X2_F
  {
    FLOAT _11;
    FLOAT _12;
    FLOAT _21;
    FLOAT _22;
    FLOAT _31;
    FLOAT _32;
  } 	DXGI_MATRIX_3X2_F;



  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0001_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0001_v0_0_s_ifspec;

#ifndef __IDXGISwapChain2_INTERFACE_DEFINED__
#define __IDXGISwapChain2_INTERFACE_DEFINED__

  /* interface IDXGISwapChain2 */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGISwapChain2;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("a8be2ac4-199f-4946-b331-79599fb98de7")
    IDXGISwapChain2 : public IDXGISwapChain1
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE SetSourceSize( 
      UINT Width,
      UINT Height) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetSourceSize( 
      /* [annotation][out] */ 
      _Out_  UINT *pWidth,
      /* [annotation][out] */ 
      _Out_  UINT *pHeight) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency( 
      UINT MaxLatency) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency( 
      /* [annotation][out] */ 
      _Out_  UINT *pMaxLatency) = 0;

    virtual HANDLE STDMETHODCALLTYPE GetFrameLatencyWaitableObject( void) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetMatrixTransform( 
      const DXGI_MATRIX_3X2_F *pMatrix) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetMatrixTransform( 
      /* [annotation][out] */ 
      _Out_  DXGI_MATRIX_3X2_F *pMatrix) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGISwapChain2Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGISwapChain2 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGISwapChain2 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGISwapChain2 * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGISwapChain2 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGISwapChain2 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGISwapChain2 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGISwapChain2 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      IDXGISwapChain2 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppDevice);

    HRESULT ( STDMETHODCALLTYPE *Present )( 
      IDXGISwapChain2 * This,
      /* [in] */ UINT SyncInterval,
      /* [in] */ UINT Flags);

    HRESULT ( STDMETHODCALLTYPE *GetBuffer )( 
      IDXGISwapChain2 * This,
      /* [in] */ UINT Buffer,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][out][in] */ 
      _Out_  void **ppSurface);

    HRESULT ( STDMETHODCALLTYPE *SetFullscreenState )( 
      IDXGISwapChain2 * This,
      /* [in] */ BOOL Fullscreen,
      /* [annotation][in] */ 
      _In_opt_  IDXGIOutput *pTarget);

    HRESULT ( STDMETHODCALLTYPE *GetFullscreenState )( 
      IDXGISwapChain2 * This,
      /* [annotation][out] */ 
      _Out_opt_  BOOL *pFullscreen,
      /* [annotation][out] */ 
      _Out_opt_  IDXGIOutput **ppTarget);

    HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
      IDXGISwapChain2 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_SWAP_CHAIN_DESC *pDesc);

    HRESULT ( STDMETHODCALLTYPE *ResizeBuffers )( 
      IDXGISwapChain2 * This,
      /* [in] */ UINT BufferCount,
      /* [in] */ UINT Width,
      /* [in] */ UINT Height,
      /* [in] */ DXGI_FORMAT NewFormat,
      /* [in] */ UINT SwapChainFlags);

    HRESULT ( STDMETHODCALLTYPE *ResizeTarget )( 
      IDXGISwapChain2 * This,
      /* [annotation][in] */ 
      _In_  const DXGI_MODE_DESC *pNewTargetParameters);

    HRESULT ( STDMETHODCALLTYPE *GetContainingOutput )( 
      IDXGISwapChain2 * This,
      /* [annotation][out] */ 
      _Out_  IDXGIOutput **ppOutput);

    HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
      IDXGISwapChain2 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_FRAME_STATISTICS *pStats);

    HRESULT ( STDMETHODCALLTYPE *GetLastPresentCount )( 
      IDXGISwapChain2 * This,
      /* [annotation][out] */ 
      _Out_  UINT *pLastPresentCount);

    HRESULT ( STDMETHODCALLTYPE *GetDesc1 )( 
      IDXGISwapChain2 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_SWAP_CHAIN_DESC1 *pDesc);

    HRESULT ( STDMETHODCALLTYPE *GetFullscreenDesc )( 
      IDXGISwapChain2 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc);

    HRESULT ( STDMETHODCALLTYPE *GetHwnd )( 
      IDXGISwapChain2 * This,
      /* [annotation][out] */ 
      _Out_  HWND *pHwnd);

    HRESULT ( STDMETHODCALLTYPE *GetCoreWindow )( 
      IDXGISwapChain2 * This,
      /* [annotation][in] */ 
      _In_  REFIID refiid,
      /* [annotation][out] */ 
      _Out_  void **ppUnk);

    HRESULT ( STDMETHODCALLTYPE *Present1 )( 
      IDXGISwapChain2 * This,
      /* [in] */ UINT SyncInterval,
      /* [in] */ UINT PresentFlags,
      /* [annotation][in] */ 
      _In_  const DXGI_PRESENT_PARAMETERS *pPresentParameters);

    BOOL ( STDMETHODCALLTYPE *IsTemporaryMonoSupported )( 
      IDXGISwapChain2 * This);

    HRESULT ( STDMETHODCALLTYPE *GetRestrictToOutput )( 
      IDXGISwapChain2 * This,
      /* [annotation][out] */ 
      _Out_  IDXGIOutput **ppRestrictToOutput);

    HRESULT ( STDMETHODCALLTYPE *SetBackgroundColor )( 
      IDXGISwapChain2 * This,
      /* [annotation][in] */ 
      _In_  const DXGI_RGBA *pColor);

    HRESULT ( STDMETHODCALLTYPE *GetBackgroundColor )( 
      IDXGISwapChain2 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_RGBA *pColor);

    HRESULT ( STDMETHODCALLTYPE *SetRotation )( 
      IDXGISwapChain2 * This,
      /* [annotation][in] */ 
      _In_  DXGI_MODE_ROTATION Rotation);

    HRESULT ( STDMETHODCALLTYPE *GetRotation )( 
      IDXGISwapChain2 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_MODE_ROTATION *pRotation);

    HRESULT ( STDMETHODCALLTYPE *SetSourceSize )( 
      IDXGISwapChain2 * This,
      UINT Width,
      UINT Height);

    HRESULT ( STDMETHODCALLTYPE *GetSourceSize )( 
      IDXGISwapChain2 * This,
      /* [annotation][out] */ 
      _Out_  UINT *pWidth,
      /* [annotation][out] */ 
      _Out_  UINT *pHeight);

    HRESULT ( STDMETHODCALLTYPE *SetMaximumFrameLatency )( 
      IDXGISwapChain2 * This,
      UINT MaxLatency);

    HRESULT ( STDMETHODCALLTYPE *GetMaximumFrameLatency )( 
      IDXGISwapChain2 * This,
      /* [annotation][out] */ 
      _Out_  UINT *pMaxLatency);

    HANDLE ( STDMETHODCALLTYPE *GetFrameLatencyWaitableObject )( 
      IDXGISwapChain2 * This);

    HRESULT ( STDMETHODCALLTYPE *SetMatrixTransform )( 
      IDXGISwapChain2 * This,
      const DXGI_MATRIX_3X2_F *pMatrix);

    HRESULT ( STDMETHODCALLTYPE *GetMatrixTransform )( 
      IDXGISwapChain2 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_MATRIX_3X2_F *pMatrix);

    END_INTERFACE
  } IDXGISwapChain2Vtbl;

  interface IDXGISwapChain2
  {
    CONST_VTBL struct IDXGISwapChain2Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGISwapChain2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGISwapChain2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGISwapChain2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGISwapChain2_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGISwapChain2_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGISwapChain2_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGISwapChain2_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGISwapChain2_GetDevice(This,riid,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppDevice) ) 


#define IDXGISwapChain2_Present(This,SyncInterval,Flags)	\
    ( (This)->lpVtbl -> Present(This,SyncInterval,Flags) ) 

#define IDXGISwapChain2_GetBuffer(This,Buffer,riid,ppSurface)	\
    ( (This)->lpVtbl -> GetBuffer(This,Buffer,riid,ppSurface) ) 

#define IDXGISwapChain2_SetFullscreenState(This,Fullscreen,pTarget)	\
    ( (This)->lpVtbl -> SetFullscreenState(This,Fullscreen,pTarget) ) 

#define IDXGISwapChain2_GetFullscreenState(This,pFullscreen,ppTarget)	\
    ( (This)->lpVtbl -> GetFullscreenState(This,pFullscreen,ppTarget) ) 

#define IDXGISwapChain2_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGISwapChain2_ResizeBuffers(This,BufferCount,Width,Height,NewFormat,SwapChainFlags)	\
    ( (This)->lpVtbl -> ResizeBuffers(This,BufferCount,Width,Height,NewFormat,SwapChainFlags) ) 

#define IDXGISwapChain2_ResizeTarget(This,pNewTargetParameters)	\
    ( (This)->lpVtbl -> ResizeTarget(This,pNewTargetParameters) ) 

#define IDXGISwapChain2_GetContainingOutput(This,ppOutput)	\
    ( (This)->lpVtbl -> GetContainingOutput(This,ppOutput) ) 

#define IDXGISwapChain2_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) ) 

#define IDXGISwapChain2_GetLastPresentCount(This,pLastPresentCount)	\
    ( (This)->lpVtbl -> GetLastPresentCount(This,pLastPresentCount) ) 


#define IDXGISwapChain2_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 

#define IDXGISwapChain2_GetFullscreenDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetFullscreenDesc(This,pDesc) ) 

#define IDXGISwapChain2_GetHwnd(This,pHwnd)	\
    ( (This)->lpVtbl -> GetHwnd(This,pHwnd) ) 

#define IDXGISwapChain2_GetCoreWindow(This,refiid,ppUnk)	\
    ( (This)->lpVtbl -> GetCoreWindow(This,refiid,ppUnk) ) 

#define IDXGISwapChain2_Present1(This,SyncInterval,PresentFlags,pPresentParameters)	\
    ( (This)->lpVtbl -> Present1(This,SyncInterval,PresentFlags,pPresentParameters) ) 

#define IDXGISwapChain2_IsTemporaryMonoSupported(This)	\
    ( (This)->lpVtbl -> IsTemporaryMonoSupported(This) ) 

#define IDXGISwapChain2_GetRestrictToOutput(This,ppRestrictToOutput)	\
    ( (This)->lpVtbl -> GetRestrictToOutput(This,ppRestrictToOutput) ) 

#define IDXGISwapChain2_SetBackgroundColor(This,pColor)	\
    ( (This)->lpVtbl -> SetBackgroundColor(This,pColor) ) 

#define IDXGISwapChain2_GetBackgroundColor(This,pColor)	\
    ( (This)->lpVtbl -> GetBackgroundColor(This,pColor) ) 

#define IDXGISwapChain2_SetRotation(This,Rotation)	\
    ( (This)->lpVtbl -> SetRotation(This,Rotation) ) 

#define IDXGISwapChain2_GetRotation(This,pRotation)	\
    ( (This)->lpVtbl -> GetRotation(This,pRotation) ) 


#define IDXGISwapChain2_SetSourceSize(This,Width,Height)	\
    ( (This)->lpVtbl -> SetSourceSize(This,Width,Height) ) 

#define IDXGISwapChain2_GetSourceSize(This,pWidth,pHeight)	\
    ( (This)->lpVtbl -> GetSourceSize(This,pWidth,pHeight) ) 

#define IDXGISwapChain2_SetMaximumFrameLatency(This,MaxLatency)	\
    ( (This)->lpVtbl -> SetMaximumFrameLatency(This,MaxLatency) ) 

#define IDXGISwapChain2_GetMaximumFrameLatency(This,pMaxLatency)	\
    ( (This)->lpVtbl -> GetMaximumFrameLatency(This,pMaxLatency) ) 

#define IDXGISwapChain2_GetFrameLatencyWaitableObject(This)	\
    ( (This)->lpVtbl -> GetFrameLatencyWaitableObject(This) ) 

#define IDXGISwapChain2_SetMatrixTransform(This,pMatrix)	\
    ( (This)->lpVtbl -> SetMatrixTransform(This,pMatrix) ) 

#define IDXGISwapChain2_GetMatrixTransform(This,pMatrix)	\
    ( (This)->lpVtbl -> GetMatrixTransform(This,pMatrix) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGISwapChain2_INTERFACE_DEFINED__ */


#ifndef __IDXGIOutput2_INTERFACE_DEFINED__
#define __IDXGIOutput2_INTERFACE_DEFINED__

  /* interface IDXGIOutput2 */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIOutput2;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("595e39d1-2724-4663-99b1-da969de28364")
    IDXGIOutput2 : public IDXGIOutput1
  {
  public:
    virtual BOOL STDMETHODCALLTYPE SupportsOverlays( void) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIOutput2Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIOutput2 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIOutput2 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIOutput2 * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGIOutput2 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGIOutput2 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGIOutput2 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGIOutput2 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
      IDXGIOutput2 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_OUTPUT_DESC *pDesc);

    HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList )( 
      IDXGIOutput2 * This,
      /* [in] */ DXGI_FORMAT EnumFormat,
      /* [in] */ UINT Flags,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pNumModes,
      /* [annotation][out] */ 
      _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC *pDesc);

    HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode )( 
      IDXGIOutput2 * This,
      /* [annotation][in] */ 
      _In_  const DXGI_MODE_DESC *pModeToMatch,
      /* [annotation][out] */ 
      _Out_  DXGI_MODE_DESC *pClosestMatch,
      /* [annotation][in] */ 
      _In_opt_  IUnknown *pConcernedDevice);

    HRESULT ( STDMETHODCALLTYPE *WaitForVBlank )( 
      IDXGIOutput2 * This);

    HRESULT ( STDMETHODCALLTYPE *TakeOwnership )( 
      IDXGIOutput2 * This,
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      BOOL Exclusive);

    void ( STDMETHODCALLTYPE *ReleaseOwnership )( 
      IDXGIOutput2 * This);

    HRESULT ( STDMETHODCALLTYPE *GetGammaControlCapabilities )( 
      IDXGIOutput2 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_GAMMA_CONTROL_CAPABILITIES *pGammaCaps);

    HRESULT ( STDMETHODCALLTYPE *SetGammaControl )( 
      IDXGIOutput2 * This,
      /* [annotation][in] */ 
      _In_  const DXGI_GAMMA_CONTROL *pArray);

    HRESULT ( STDMETHODCALLTYPE *GetGammaControl )( 
      IDXGIOutput2 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_GAMMA_CONTROL *pArray);

    HRESULT ( STDMETHODCALLTYPE *SetDisplaySurface )( 
      IDXGIOutput2 * This,
      /* [annotation][in] */ 
      _In_  IDXGISurface *pScanoutSurface);

    HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData )( 
      IDXGIOutput2 * This,
      /* [annotation][in] */ 
      _In_  IDXGISurface *pDestination);

    HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
      IDXGIOutput2 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_FRAME_STATISTICS *pStats);

    HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList1 )( 
      IDXGIOutput2 * This,
      /* [in] */ DXGI_FORMAT EnumFormat,
      /* [in] */ UINT Flags,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pNumModes,
      /* [annotation][out] */ 
      _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC1 *pDesc);

    HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode1 )( 
      IDXGIOutput2 * This,
      /* [annotation][in] */ 
      _In_  const DXGI_MODE_DESC1 *pModeToMatch,
      /* [annotation][out] */ 
      _Out_  DXGI_MODE_DESC1 *pClosestMatch,
      /* [annotation][in] */ 
      _In_opt_  IUnknown *pConcernedDevice);

    HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData1 )( 
      IDXGIOutput2 * This,
      /* [annotation][in] */ 
      _In_  IDXGIResource *pDestination);

    HRESULT ( STDMETHODCALLTYPE *DuplicateOutput )( 
      IDXGIOutput2 * This,
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      /* [annotation][out] */ 
      _Out_  IDXGIOutputDuplication **ppOutputDuplication);

    BOOL ( STDMETHODCALLTYPE *SupportsOverlays )( 
      IDXGIOutput2 * This);

    END_INTERFACE
  } IDXGIOutput2Vtbl;

  interface IDXGIOutput2
  {
    CONST_VTBL struct IDXGIOutput2Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIOutput2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIOutput2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIOutput2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIOutput2_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIOutput2_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIOutput2_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIOutput2_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIOutput2_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGIOutput2_GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput2_FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput2_WaitForVBlank(This)	\
    ( (This)->lpVtbl -> WaitForVBlank(This) ) 

#define IDXGIOutput2_TakeOwnership(This,pDevice,Exclusive)	\
    ( (This)->lpVtbl -> TakeOwnership(This,pDevice,Exclusive) ) 

#define IDXGIOutput2_ReleaseOwnership(This)	\
    ( (This)->lpVtbl -> ReleaseOwnership(This) ) 

#define IDXGIOutput2_GetGammaControlCapabilities(This,pGammaCaps)	\
    ( (This)->lpVtbl -> GetGammaControlCapabilities(This,pGammaCaps) ) 

#define IDXGIOutput2_SetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> SetGammaControl(This,pArray) ) 

#define IDXGIOutput2_GetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> GetGammaControl(This,pArray) ) 

#define IDXGIOutput2_SetDisplaySurface(This,pScanoutSurface)	\
    ( (This)->lpVtbl -> SetDisplaySurface(This,pScanoutSurface) ) 

#define IDXGIOutput2_GetDisplaySurfaceData(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData(This,pDestination) ) 

#define IDXGIOutput2_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) ) 


#define IDXGIOutput2_GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput2_FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput2_GetDisplaySurfaceData1(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData1(This,pDestination) ) 

#define IDXGIOutput2_DuplicateOutput(This,pDevice,ppOutputDuplication)	\
    ( (This)->lpVtbl -> DuplicateOutput(This,pDevice,ppOutputDuplication) ) 


#define IDXGIOutput2_SupportsOverlays(This)	\
    ( (This)->lpVtbl -> SupportsOverlays(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIOutput2_INTERFACE_DEFINED__ */


#ifndef __IDXGIFactory3_INTERFACE_DEFINED__
#define __IDXGIFactory3_INTERFACE_DEFINED__

  /* interface IDXGIFactory3 */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIFactory3;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("25483823-cd46-4c7d-86ca-47aa95b837bd")
    IDXGIFactory3 : public IDXGIFactory2
  {
  public:
    virtual UINT STDMETHODCALLTYPE GetCreationFlags( void) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIFactory3Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIFactory3 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIFactory3 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIFactory3 * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGIFactory3 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGIFactory3 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGIFactory3 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGIFactory3 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *EnumAdapters )( 
      IDXGIFactory3 * This,
      /* [in] */ UINT Adapter,
      /* [annotation][out] */ 
      _Out_  IDXGIAdapter **ppAdapter);

    HRESULT ( STDMETHODCALLTYPE *MakeWindowAssociation )( 
      IDXGIFactory3 * This,
      HWND WindowHandle,
      UINT Flags);

    HRESULT ( STDMETHODCALLTYPE *GetWindowAssociation )( 
      IDXGIFactory3 * This,
      /* [annotation][out] */ 
      _Out_  HWND *pWindowHandle);

    HRESULT ( STDMETHODCALLTYPE *CreateSwapChain )( 
      IDXGIFactory3 * This,
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      /* [annotation][in] */ 
      _In_  DXGI_SWAP_CHAIN_DESC *pDesc,
      /* [annotation][out] */ 
      _Out_  IDXGISwapChain **ppSwapChain);

    HRESULT ( STDMETHODCALLTYPE *CreateSoftwareAdapter )( 
      IDXGIFactory3 * This,
      /* [in] */ HMODULE Module,
      /* [annotation][out] */ 
      _Out_  IDXGIAdapter **ppAdapter);

    HRESULT ( STDMETHODCALLTYPE *EnumAdapters1 )( 
      IDXGIFactory3 * This,
      /* [in] */ UINT Adapter,
      /* [annotation][out] */ 
      _Out_  IDXGIAdapter1 **ppAdapter);

    BOOL ( STDMETHODCALLTYPE *IsCurrent )( 
      IDXGIFactory3 * This);

    BOOL ( STDMETHODCALLTYPE *IsWindowedStereoEnabled )( 
      IDXGIFactory3 * This);

    HRESULT ( STDMETHODCALLTYPE *CreateSwapChainForHwnd )( 
      IDXGIFactory3 * This,
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      /* [annotation][in] */ 
      _In_  HWND hWnd,
      /* [annotation][in] */ 
      _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
      /* [annotation][in] */ 
      _In_opt_  const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc,
      /* [annotation][in] */ 
      _In_opt_  IDXGIOutput *pRestrictToOutput,
      /* [annotation][out] */ 
      _Out_  IDXGISwapChain1 **ppSwapChain);

    HRESULT ( STDMETHODCALLTYPE *CreateSwapChainForCoreWindow )( 
      IDXGIFactory3 * This,
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      /* [annotation][in] */ 
      _In_  IUnknown *pWindow,
      /* [annotation][in] */ 
      _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
      /* [annotation][in] */ 
      _In_opt_  IDXGIOutput *pRestrictToOutput,
      /* [annotation][out] */ 
      _Out_  IDXGISwapChain1 **ppSwapChain);

    HRESULT ( STDMETHODCALLTYPE *GetSharedResourceAdapterLuid )( 
      IDXGIFactory3 * This,
      /* [annotation] */ 
      _In_  HANDLE hResource,
      /* [annotation] */ 
      _Out_  LUID *pLuid);

    HRESULT ( STDMETHODCALLTYPE *RegisterStereoStatusWindow )( 
      IDXGIFactory3 * This,
      /* [annotation][in] */ 
      _In_  HWND WindowHandle,
      /* [annotation][in] */ 
      _In_  UINT wMsg,
      /* [annotation][out] */ 
      _Out_  DWORD *pdwCookie);

    HRESULT ( STDMETHODCALLTYPE *RegisterStereoStatusEvent )( 
      IDXGIFactory3 * This,
      /* [annotation][in] */ 
      _In_  HANDLE hEvent,
      /* [annotation][out] */ 
      _Out_  DWORD *pdwCookie);

    void ( STDMETHODCALLTYPE *UnregisterStereoStatus )( 
      IDXGIFactory3 * This,
      /* [annotation][in] */ 
      _In_  DWORD dwCookie);

    HRESULT ( STDMETHODCALLTYPE *RegisterOcclusionStatusWindow )( 
      IDXGIFactory3 * This,
      /* [annotation][in] */ 
      _In_  HWND WindowHandle,
      /* [annotation][in] */ 
      _In_  UINT wMsg,
      /* [annotation][out] */ 
      _Out_  DWORD *pdwCookie);

    HRESULT ( STDMETHODCALLTYPE *RegisterOcclusionStatusEvent )( 
      IDXGIFactory3 * This,
      /* [annotation][in] */ 
      _In_  HANDLE hEvent,
      /* [annotation][out] */ 
      _Out_  DWORD *pdwCookie);

    void ( STDMETHODCALLTYPE *UnregisterOcclusionStatus )( 
      IDXGIFactory3 * This,
      /* [annotation][in] */ 
      _In_  DWORD dwCookie);

    HRESULT ( STDMETHODCALLTYPE *CreateSwapChainForComposition )( 
      IDXGIFactory3 * This,
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      /* [annotation][in] */ 
      _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
      /* [annotation][in] */ 
      _In_opt_  IDXGIOutput *pRestrictToOutput,
      /* [annotation][out] */ 
      _Outptr_  IDXGISwapChain1 **ppSwapChain);

    UINT ( STDMETHODCALLTYPE *GetCreationFlags )( 
      IDXGIFactory3 * This);

    END_INTERFACE
  } IDXGIFactory3Vtbl;

  interface IDXGIFactory3
  {
    CONST_VTBL struct IDXGIFactory3Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIFactory3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIFactory3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIFactory3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIFactory3_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIFactory3_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIFactory3_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIFactory3_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIFactory3_EnumAdapters(This,Adapter,ppAdapter)	\
    ( (This)->lpVtbl -> EnumAdapters(This,Adapter,ppAdapter) ) 

#define IDXGIFactory3_MakeWindowAssociation(This,WindowHandle,Flags)	\
    ( (This)->lpVtbl -> MakeWindowAssociation(This,WindowHandle,Flags) ) 

#define IDXGIFactory3_GetWindowAssociation(This,pWindowHandle)	\
    ( (This)->lpVtbl -> GetWindowAssociation(This,pWindowHandle) ) 

#define IDXGIFactory3_CreateSwapChain(This,pDevice,pDesc,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChain(This,pDevice,pDesc,ppSwapChain) ) 

#define IDXGIFactory3_CreateSoftwareAdapter(This,Module,ppAdapter)	\
    ( (This)->lpVtbl -> CreateSoftwareAdapter(This,Module,ppAdapter) ) 


#define IDXGIFactory3_EnumAdapters1(This,Adapter,ppAdapter)	\
    ( (This)->lpVtbl -> EnumAdapters1(This,Adapter,ppAdapter) ) 

#define IDXGIFactory3_IsCurrent(This)	\
    ( (This)->lpVtbl -> IsCurrent(This) ) 


#define IDXGIFactory3_IsWindowedStereoEnabled(This)	\
    ( (This)->lpVtbl -> IsWindowedStereoEnabled(This) ) 

#define IDXGIFactory3_CreateSwapChainForHwnd(This,pDevice,hWnd,pDesc,pFullscreenDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForHwnd(This,pDevice,hWnd,pDesc,pFullscreenDesc,pRestrictToOutput,ppSwapChain) ) 

#define IDXGIFactory3_CreateSwapChainForCoreWindow(This,pDevice,pWindow,pDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForCoreWindow(This,pDevice,pWindow,pDesc,pRestrictToOutput,ppSwapChain) ) 

#define IDXGIFactory3_GetSharedResourceAdapterLuid(This,hResource,pLuid)	\
    ( (This)->lpVtbl -> GetSharedResourceAdapterLuid(This,hResource,pLuid) ) 

#define IDXGIFactory3_RegisterStereoStatusWindow(This,WindowHandle,wMsg,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterStereoStatusWindow(This,WindowHandle,wMsg,pdwCookie) ) 

#define IDXGIFactory3_RegisterStereoStatusEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterStereoStatusEvent(This,hEvent,pdwCookie) ) 

#define IDXGIFactory3_UnregisterStereoStatus(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterStereoStatus(This,dwCookie) ) 

#define IDXGIFactory3_RegisterOcclusionStatusWindow(This,WindowHandle,wMsg,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterOcclusionStatusWindow(This,WindowHandle,wMsg,pdwCookie) ) 

#define IDXGIFactory3_RegisterOcclusionStatusEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterOcclusionStatusEvent(This,hEvent,pdwCookie) ) 

#define IDXGIFactory3_UnregisterOcclusionStatus(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterOcclusionStatus(This,dwCookie) ) 

#define IDXGIFactory3_CreateSwapChainForComposition(This,pDevice,pDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForComposition(This,pDevice,pDesc,pRestrictToOutput,ppSwapChain) ) 


#define IDXGIFactory3_GetCreationFlags(This)	\
    ( (This)->lpVtbl -> GetCreationFlags(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIFactory3_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_dxgi1_3_0000_0004 */
  /* [local] */ 

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion
#pragma region App Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
  typedef struct DXGI_DECODE_SWAP_CHAIN_DESC
  {
    UINT Flags;
  } 	DXGI_DECODE_SWAP_CHAIN_DESC;

  typedef 
  enum DXGI_MULTIPLANE_OVERLAY_YCbCr_FLAGS
  {
    DXGI_MULTIPLANE_OVERLAY_YCbCr_FLAG_NOMINAL_RANGE	= 0x1,
    DXGI_MULTIPLANE_OVERLAY_YCbCr_FLAG_BT709	= 0x2,
    DXGI_MULTIPLANE_OVERLAY_YCbCr_FLAG_xvYCC	= 0x4
  } 	DXGI_MULTIPLANE_OVERLAY_YCbCr_FLAGS;



  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0004_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0004_v0_0_s_ifspec;

#ifndef __IDXGIDecodeSwapChain_INTERFACE_DEFINED__
#define __IDXGIDecodeSwapChain_INTERFACE_DEFINED__

  /* interface IDXGIDecodeSwapChain */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIDecodeSwapChain;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("2633066b-4514-4c7a-8fd8-12ea98059d18")
    IDXGIDecodeSwapChain : public IUnknown
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE PresentBuffer( 
      UINT BufferToPresent,
      UINT SyncInterval,
      UINT Flags) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetSourceRect( 
      const RECT *pRect) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetTargetRect( 
      const RECT *pRect) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetDestSize( 
      UINT Width,
      UINT Height) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetSourceRect( 
      /* [annotation][out] */ 
      _Out_  RECT *pRect) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetTargetRect( 
      /* [annotation][out] */ 
      _Out_  RECT *pRect) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetDestSize( 
      /* [annotation][out] */ 
      _Out_  UINT *pWidth,
      /* [annotation][out] */ 
      _Out_  UINT *pHeight) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetColorSpace( 
      DXGI_MULTIPLANE_OVERLAY_YCbCr_FLAGS ColorSpace) = 0;

    virtual DXGI_MULTIPLANE_OVERLAY_YCbCr_FLAGS STDMETHODCALLTYPE GetColorSpace( void) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIDecodeSwapChainVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIDecodeSwapChain * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIDecodeSwapChain * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIDecodeSwapChain * This);

    HRESULT ( STDMETHODCALLTYPE *PresentBuffer )( 
      IDXGIDecodeSwapChain * This,
      UINT BufferToPresent,
      UINT SyncInterval,
      UINT Flags);

    HRESULT ( STDMETHODCALLTYPE *SetSourceRect )( 
      IDXGIDecodeSwapChain * This,
      const RECT *pRect);

    HRESULT ( STDMETHODCALLTYPE *SetTargetRect )( 
      IDXGIDecodeSwapChain * This,
      const RECT *pRect);

    HRESULT ( STDMETHODCALLTYPE *SetDestSize )( 
      IDXGIDecodeSwapChain * This,
      UINT Width,
      UINT Height);

    HRESULT ( STDMETHODCALLTYPE *GetSourceRect )( 
      IDXGIDecodeSwapChain * This,
      /* [annotation][out] */ 
      _Out_  RECT *pRect);

    HRESULT ( STDMETHODCALLTYPE *GetTargetRect )( 
      IDXGIDecodeSwapChain * This,
      /* [annotation][out] */ 
      _Out_  RECT *pRect);

    HRESULT ( STDMETHODCALLTYPE *GetDestSize )( 
      IDXGIDecodeSwapChain * This,
      /* [annotation][out] */ 
      _Out_  UINT *pWidth,
      /* [annotation][out] */ 
      _Out_  UINT *pHeight);

    HRESULT ( STDMETHODCALLTYPE *SetColorSpace )( 
      IDXGIDecodeSwapChain * This,
      DXGI_MULTIPLANE_OVERLAY_YCbCr_FLAGS ColorSpace);

    DXGI_MULTIPLANE_OVERLAY_YCbCr_FLAGS ( STDMETHODCALLTYPE *GetColorSpace )( 
      IDXGIDecodeSwapChain * This);

    END_INTERFACE
  } IDXGIDecodeSwapChainVtbl;

  interface IDXGIDecodeSwapChain
  {
    CONST_VTBL struct IDXGIDecodeSwapChainVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIDecodeSwapChain_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIDecodeSwapChain_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIDecodeSwapChain_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIDecodeSwapChain_PresentBuffer(This,BufferToPresent,SyncInterval,Flags)	\
    ( (This)->lpVtbl -> PresentBuffer(This,BufferToPresent,SyncInterval,Flags) ) 

#define IDXGIDecodeSwapChain_SetSourceRect(This,pRect)	\
    ( (This)->lpVtbl -> SetSourceRect(This,pRect) ) 

#define IDXGIDecodeSwapChain_SetTargetRect(This,pRect)	\
    ( (This)->lpVtbl -> SetTargetRect(This,pRect) ) 

#define IDXGIDecodeSwapChain_SetDestSize(This,Width,Height)	\
    ( (This)->lpVtbl -> SetDestSize(This,Width,Height) ) 

#define IDXGIDecodeSwapChain_GetSourceRect(This,pRect)	\
    ( (This)->lpVtbl -> GetSourceRect(This,pRect) ) 

#define IDXGIDecodeSwapChain_GetTargetRect(This,pRect)	\
    ( (This)->lpVtbl -> GetTargetRect(This,pRect) ) 

#define IDXGIDecodeSwapChain_GetDestSize(This,pWidth,pHeight)	\
    ( (This)->lpVtbl -> GetDestSize(This,pWidth,pHeight) ) 

#define IDXGIDecodeSwapChain_SetColorSpace(This,ColorSpace)	\
    ( (This)->lpVtbl -> SetColorSpace(This,ColorSpace) ) 

#define IDXGIDecodeSwapChain_GetColorSpace(This)	\
    ( (This)->lpVtbl -> GetColorSpace(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIDecodeSwapChain_INTERFACE_DEFINED__ */


#ifndef __IDXGIFactoryMedia_INTERFACE_DEFINED__
#define __IDXGIFactoryMedia_INTERFACE_DEFINED__

  /* interface IDXGIFactoryMedia */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIFactoryMedia;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("41e7d1f2-a591-4f7b-a2e5-fa9c843e1c12")
    IDXGIFactoryMedia : public IUnknown
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForCompositionSurfaceHandle( 
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      /* [annotation][in] */ 
      _In_opt_  HANDLE hSurface,
      /* [annotation][in] */ 
      _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
      /* [annotation][in] */ 
      _In_opt_  IDXGIOutput *pRestrictToOutput,
      /* [annotation][out] */ 
      _Out_  IDXGISwapChain1 **ppSwapChain) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateDecodeSwapChainForCompositionSurfaceHandle( 
      IUnknown *pDevice,
      HANDLE hSurface,
      DXGI_DECODE_SWAP_CHAIN_DESC *pDesc,
      IDXGIResource *pYuvDecodeBuffers,
      IDXGIOutput *pRestrictToOutput,
      /* [annotation][out] */ 
      _Out_  IDXGIDecodeSwapChain **ppSwapChain) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIFactoryMediaVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIFactoryMedia * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIFactoryMedia * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIFactoryMedia * This);

    HRESULT ( STDMETHODCALLTYPE *CreateSwapChainForCompositionSurfaceHandle )( 
      IDXGIFactoryMedia * This,
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      /* [annotation][in] */ 
      _In_opt_  HANDLE hSurface,
      /* [annotation][in] */ 
      _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
      /* [annotation][in] */ 
      _In_opt_  IDXGIOutput *pRestrictToOutput,
      /* [annotation][out] */ 
      _Out_  IDXGISwapChain1 **ppSwapChain);

    HRESULT ( STDMETHODCALLTYPE *CreateDecodeSwapChainForCompositionSurfaceHandle )( 
      IDXGIFactoryMedia * This,
      IUnknown *pDevice,
      HANDLE hSurface,
      DXGI_DECODE_SWAP_CHAIN_DESC *pDesc,
      IDXGIResource *pYuvDecodeBuffers,
      IDXGIOutput *pRestrictToOutput,
      /* [annotation][out] */ 
      _Out_  IDXGIDecodeSwapChain **ppSwapChain);

    END_INTERFACE
  } IDXGIFactoryMediaVtbl;

  interface IDXGIFactoryMedia
  {
    CONST_VTBL struct IDXGIFactoryMediaVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIFactoryMedia_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIFactoryMedia_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIFactoryMedia_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIFactoryMedia_CreateSwapChainForCompositionSurfaceHandle(This,pDevice,hSurface,pDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForCompositionSurfaceHandle(This,pDevice,hSurface,pDesc,pRestrictToOutput,ppSwapChain) ) 

#define IDXGIFactoryMedia_CreateDecodeSwapChainForCompositionSurfaceHandle(This,pDevice,hSurface,pDesc,pYuvDecodeBuffers,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateDecodeSwapChainForCompositionSurfaceHandle(This,pDevice,hSurface,pDesc,pYuvDecodeBuffers,pRestrictToOutput,ppSwapChain) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIFactoryMedia_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_dxgi1_3_0000_0006 */
  /* [local] */ 

  typedef 
  enum DXGI_FRAME_PRESENTATION_MODE
  {
    DXGI_FRAME_PRESENTATION_MODE_COMPOSED	= 0,
    DXGI_FRAME_PRESENTATION_MODE_OVERLAY	= 1,
    DXGI_FRAME_PRESENTATION_MODE_NONE	= 2
  } 	DXGI_FRAME_PRESENTATION_MODE;

  typedef struct DXGI_FRAME_STATISTICS_MEDIA
  {
    UINT PresentCount;
    UINT PresentRefreshCount;
    UINT SyncRefreshCount;
    LARGE_INTEGER SyncQPCTime;
    LARGE_INTEGER SyncGPUTime;
    DXGI_FRAME_PRESENTATION_MODE CompositionMode;
    UINT ApprovedPresentDuration;
  } 	DXGI_FRAME_STATISTICS_MEDIA;



  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0006_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0006_v0_0_s_ifspec;

#ifndef __IDXGISwapChainMedia_INTERFACE_DEFINED__
#define __IDXGISwapChainMedia_INTERFACE_DEFINED__

  /* interface IDXGISwapChainMedia */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGISwapChainMedia;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("dd95b90b-f05f-4f6a-bd65-25bfb264bd84")
    IDXGISwapChainMedia : public IUnknown
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetFrameStatisticsMedia( 
      /* [annotation][out] */ 
      _Out_  DXGI_FRAME_STATISTICS_MEDIA *pStats) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetPresentDuration( 
      UINT Duration) = 0;

    virtual HRESULT STDMETHODCALLTYPE CheckPresentDurationSupport( 
      UINT DesiredPresentDuration,
      /* [annotation][out] */ 
      _Out_  UINT *pClosestSmallerPresentDuration,
      /* [annotation][out] */ 
      _Out_  UINT *pClosestLargerPresentDuration) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGISwapChainMediaVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGISwapChainMedia * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGISwapChainMedia * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGISwapChainMedia * This);

    HRESULT ( STDMETHODCALLTYPE *GetFrameStatisticsMedia )( 
      IDXGISwapChainMedia * This,
      /* [annotation][out] */ 
      _Out_  DXGI_FRAME_STATISTICS_MEDIA *pStats);

    HRESULT ( STDMETHODCALLTYPE *SetPresentDuration )( 
      IDXGISwapChainMedia * This,
      UINT Duration);

    HRESULT ( STDMETHODCALLTYPE *CheckPresentDurationSupport )( 
      IDXGISwapChainMedia * This,
      UINT DesiredPresentDuration,
      /* [annotation][out] */ 
      _Out_  UINT *pClosestSmallerPresentDuration,
      /* [annotation][out] */ 
      _Out_  UINT *pClosestLargerPresentDuration);

    END_INTERFACE
  } IDXGISwapChainMediaVtbl;

  interface IDXGISwapChainMedia
  {
    CONST_VTBL struct IDXGISwapChainMediaVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGISwapChainMedia_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGISwapChainMedia_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGISwapChainMedia_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGISwapChainMedia_GetFrameStatisticsMedia(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatisticsMedia(This,pStats) ) 

#define IDXGISwapChainMedia_SetPresentDuration(This,Duration)	\
    ( (This)->lpVtbl -> SetPresentDuration(This,Duration) ) 

#define IDXGISwapChainMedia_CheckPresentDurationSupport(This,DesiredPresentDuration,pClosestSmallerPresentDuration,pClosestLargerPresentDuration)	\
    ( (This)->lpVtbl -> CheckPresentDurationSupport(This,DesiredPresentDuration,pClosestSmallerPresentDuration,pClosestLargerPresentDuration) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGISwapChainMedia_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_dxgi1_3_0000_0007 */
  /* [local] */ 

  typedef 
  enum DXGI_OVERLAY_SUPPORT_FLAG
  {
    DXGI_OVERLAY_SUPPORT_FLAG_DIRECT	= 0x1,
    DXGI_OVERLAY_SUPPORT_FLAG_SCALING	= 0x2
  } 	DXGI_OVERLAY_SUPPORT_FLAG;



  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0007_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0007_v0_0_s_ifspec;

#ifndef __IDXGIOutput3_INTERFACE_DEFINED__
#define __IDXGIOutput3_INTERFACE_DEFINED__

  /* interface IDXGIOutput3 */
  /* [unique][local][uuid][object] */ 


  EXTERN_C const IID IID_IDXGIOutput3;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("8a6bb301-7e7e-41F4-a8e0-5b32f7f99b18")
    IDXGIOutput3 : public IDXGIOutput2
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE CheckOverlaySupport( 
      /* [annotation][in] */ 
      _In_  DXGI_FORMAT EnumFormat,
      /* [annotation][out] */ 
      _In_  IUnknown *pConcernedDevice,
      /* [annotation][out] */ 
      _Out_  UINT *pFlags) = 0;

  };


#else 	/* C style interface */

  typedef struct IDXGIOutput3Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIOutput3 * This,
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      IDXGIOutput3 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      IDXGIOutput3 * This);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      IDXGIOutput3 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [in] */ UINT DataSize,
      /* [annotation][in] */ 
      _In_reads_bytes_(DataSize)  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      IDXGIOutput3 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][in] */ 
      _In_  const IUnknown *pUnknown);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      IDXGIOutput3 * This,
      /* [annotation][in] */ 
      _In_  REFGUID Name,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pDataSize,
      /* [annotation][out] */ 
      _Out_writes_bytes_(*pDataSize)  void *pData);

    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
      IDXGIOutput3 * This,
      /* [annotation][in] */ 
      _In_  REFIID riid,
      /* [annotation][retval][out] */ 
      _Out_  void **ppParent);

    HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
      IDXGIOutput3 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_OUTPUT_DESC *pDesc);

    HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList )( 
      IDXGIOutput3 * This,
      /* [in] */ DXGI_FORMAT EnumFormat,
      /* [in] */ UINT Flags,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pNumModes,
      /* [annotation][out] */ 
      _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC *pDesc);

    HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode )( 
      IDXGIOutput3 * This,
      /* [annotation][in] */ 
      _In_  const DXGI_MODE_DESC *pModeToMatch,
      /* [annotation][out] */ 
      _Out_  DXGI_MODE_DESC *pClosestMatch,
      /* [annotation][in] */ 
      _In_opt_  IUnknown *pConcernedDevice);

    HRESULT ( STDMETHODCALLTYPE *WaitForVBlank )( 
      IDXGIOutput3 * This);

    HRESULT ( STDMETHODCALLTYPE *TakeOwnership )( 
      IDXGIOutput3 * This,
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      BOOL Exclusive);

    void ( STDMETHODCALLTYPE *ReleaseOwnership )( 
      IDXGIOutput3 * This);

    HRESULT ( STDMETHODCALLTYPE *GetGammaControlCapabilities )( 
      IDXGIOutput3 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_GAMMA_CONTROL_CAPABILITIES *pGammaCaps);

    HRESULT ( STDMETHODCALLTYPE *SetGammaControl )( 
      IDXGIOutput3 * This,
      /* [annotation][in] */ 
      _In_  const DXGI_GAMMA_CONTROL *pArray);

    HRESULT ( STDMETHODCALLTYPE *GetGammaControl )( 
      IDXGIOutput3 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_GAMMA_CONTROL *pArray);

    HRESULT ( STDMETHODCALLTYPE *SetDisplaySurface )( 
      IDXGIOutput3 * This,
      /* [annotation][in] */ 
      _In_  IDXGISurface *pScanoutSurface);

    HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData )( 
      IDXGIOutput3 * This,
      /* [annotation][in] */ 
      _In_  IDXGISurface *pDestination);

    HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
      IDXGIOutput3 * This,
      /* [annotation][out] */ 
      _Out_  DXGI_FRAME_STATISTICS *pStats);

    HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList1 )( 
      IDXGIOutput3 * This,
      /* [in] */ DXGI_FORMAT EnumFormat,
      /* [in] */ UINT Flags,
      /* [annotation][out][in] */ 
      _Inout_  UINT *pNumModes,
      /* [annotation][out] */ 
      _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC1 *pDesc);

    HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode1 )( 
      IDXGIOutput3 * This,
      /* [annotation][in] */ 
      _In_  const DXGI_MODE_DESC1 *pModeToMatch,
      /* [annotation][out] */ 
      _Out_  DXGI_MODE_DESC1 *pClosestMatch,
      /* [annotation][in] */ 
      _In_opt_  IUnknown *pConcernedDevice);

    HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData1 )( 
      IDXGIOutput3 * This,
      /* [annotation][in] */ 
      _In_  IDXGIResource *pDestination);

    HRESULT ( STDMETHODCALLTYPE *DuplicateOutput )( 
      IDXGIOutput3 * This,
      /* [annotation][in] */ 
      _In_  IUnknown *pDevice,
      /* [annotation][out] */ 
      _Out_  IDXGIOutputDuplication **ppOutputDuplication);

    BOOL ( STDMETHODCALLTYPE *SupportsOverlays )( 
      IDXGIOutput3 * This);

    HRESULT ( STDMETHODCALLTYPE *CheckOverlaySupport )( 
      IDXGIOutput3 * This,
      /* [annotation][in] */ 
      _In_  DXGI_FORMAT EnumFormat,
      /* [annotation][out] */ 
      _In_  IUnknown *pConcernedDevice,
      /* [annotation][out] */ 
      _Out_  UINT *pFlags);

    END_INTERFACE
  } IDXGIOutput3Vtbl;

  interface IDXGIOutput3
  {
    CONST_VTBL struct IDXGIOutput3Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDXGIOutput3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIOutput3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIOutput3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIOutput3_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIOutput3_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIOutput3_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIOutput3_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIOutput3_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGIOutput3_GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput3_FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput3_WaitForVBlank(This)	\
    ( (This)->lpVtbl -> WaitForVBlank(This) ) 

#define IDXGIOutput3_TakeOwnership(This,pDevice,Exclusive)	\
    ( (This)->lpVtbl -> TakeOwnership(This,pDevice,Exclusive) ) 

#define IDXGIOutput3_ReleaseOwnership(This)	\
    ( (This)->lpVtbl -> ReleaseOwnership(This) ) 

#define IDXGIOutput3_GetGammaControlCapabilities(This,pGammaCaps)	\
    ( (This)->lpVtbl -> GetGammaControlCapabilities(This,pGammaCaps) ) 

#define IDXGIOutput3_SetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> SetGammaControl(This,pArray) ) 

#define IDXGIOutput3_GetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> GetGammaControl(This,pArray) ) 

#define IDXGIOutput3_SetDisplaySurface(This,pScanoutSurface)	\
    ( (This)->lpVtbl -> SetDisplaySurface(This,pScanoutSurface) ) 

#define IDXGIOutput3_GetDisplaySurfaceData(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData(This,pDestination) ) 

#define IDXGIOutput3_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) ) 


#define IDXGIOutput3_GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput3_FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput3_GetDisplaySurfaceData1(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData1(This,pDestination) ) 

#define IDXGIOutput3_DuplicateOutput(This,pDevice,ppOutputDuplication)	\
    ( (This)->lpVtbl -> DuplicateOutput(This,pDevice,ppOutputDuplication) ) 


#define IDXGIOutput3_SupportsOverlays(This)	\
    ( (This)->lpVtbl -> SupportsOverlays(This) ) 


#define IDXGIOutput3_CheckOverlaySupport(This,EnumFormat,pConcernedDevice,pFlags)	\
    ( (This)->lpVtbl -> CheckOverlaySupport(This,EnumFormat,pConcernedDevice,pFlags) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIOutput3_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_dxgi1_3_0000_0008 */
  /* [local] */ 

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion
  DEFINE_GUID(IID_IDXGIDevice3,0x6007896c,0x3244,0x4afd,0xbf,0x18,0xa6,0xd3,0xbe,0xda,0x50,0x23);
  DEFINE_GUID(IID_IDXGISwapChain2,0xa8be2ac4,0x199f,0x4946,0xb3,0x31,0x79,0x59,0x9f,0xb9,0x8d,0xe7);
  DEFINE_GUID(IID_IDXGIOutput2,0x595e39d1,0x2724,0x4663,0x99,0xb1,0xda,0x96,0x9d,0xe2,0x83,0x64);
  DEFINE_GUID(IID_IDXGIFactory3,0x25483823,0xcd46,0x4c7d,0x86,0xca,0x47,0xaa,0x95,0xb8,0x37,0xbd);
  DEFINE_GUID(IID_IDXGIDecodeSwapChain,0x2633066b,0x4514,0x4c7a,0x8f,0xd8,0x12,0xea,0x98,0x05,0x9d,0x18);
  DEFINE_GUID(IID_IDXGIFactoryMedia,0x41e7d1f2,0xa591,0x4f7b,0xa2,0xe5,0xfa,0x9c,0x84,0x3e,0x1c,0x12);
  DEFINE_GUID(IID_IDXGISwapChainMedia,0xdd95b90b,0xf05f,0x4f6a,0xbd,0x65,0x25,0xbf,0xb2,0x64,0xbd,0x84);
  DEFINE_GUID(IID_IDXGIOutput3,0x8a6bb301,0x7e7e,0x41F4,0xa8,0xe0,0x5b,0x32,0xf7,0xf9,0x9b,0x18);


  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0008_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_dxgi1_3_0000_0008_v0_0_s_ifspec;

  /* Additional Prototypes for ALL interfaces */

  /* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif

EXTERN_C const IID IID_IDXGIFactory4;

MIDL_INTERFACE("1bc6ea02-ef36-464f-bf0c-21ca39e5168a")
IDXGIFactory4 : public IDXGIFactory3
{
public:
  virtual HRESULT STDMETHODCALLTYPE EnumAdapterByLuid( 
    /* [annotation] */ 
    _In_  LUID AdapterLuid,
    /* [annotation] */ 
    _In_  REFIID riid,
    /* [annotation] */ 
    _COM_Outptr_  void **ppvAdapter) = 0;

  virtual HRESULT STDMETHODCALLTYPE EnumWarpAdapter( 
    /* [annotation] */ 
    _In_  REFIID riid,
    /* [annotation] */ 
    _COM_Outptr_  void **ppvAdapter) = 0;

};

typedef 
enum DXGI_MEMORY_SEGMENT_GROUP
{
  DXGI_MEMORY_SEGMENT_GROUP_LOCAL	= 0,
  DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL	= 1
} 	DXGI_MEMORY_SEGMENT_GROUP;

typedef struct DXGI_QUERY_VIDEO_MEMORY_INFO
{
  UINT64 Budget;
  UINT64 CurrentUsage;
  UINT64 AvailableForReservation;
  UINT64 CurrentReservation;
} 	DXGI_QUERY_VIDEO_MEMORY_INFO;

EXTERN_C const IID IID_IDXGIAdapter3;

MIDL_INTERFACE("645967A4-1392-4310-A798-8053CE3E93FD")
IDXGIAdapter3 : public IDXGIAdapter2
{
public:
  virtual HRESULT STDMETHODCALLTYPE RegisterHardwareContentProtectionTeardownStatusEvent( 
    /* [annotation][in] */ 
    _In_  HANDLE hEvent,
    /* [annotation][out] */ 
    _Out_  DWORD *pdwCookie) = 0;

  virtual void STDMETHODCALLTYPE UnregisterHardwareContentProtectionTeardownStatus( 
    /* [annotation][in] */ 
    _In_  DWORD dwCookie) = 0;

  virtual HRESULT STDMETHODCALLTYPE QueryVideoMemoryInfo( 
    /* [annotation][in] */ 
    _In_  UINT NodeIndex,
    /* [annotation][in] */ 
    _In_  DXGI_MEMORY_SEGMENT_GROUP MemorySegmentGroup,
    /* [annotation][out] */ 
    _Out_  DXGI_QUERY_VIDEO_MEMORY_INFO *pVideoMemoryInfo) = 0;

  virtual HRESULT STDMETHODCALLTYPE SetVideoMemoryReservation( 
    /* [annotation][in] */ 
    _In_  UINT NodeIndex,
    /* [annotation][in] */ 
    _In_  DXGI_MEMORY_SEGMENT_GROUP MemorySegmentGroup,
    /* [annotation][in] */ 
    _In_  UINT64 Reservation) = 0;

  virtual HRESULT STDMETHODCALLTYPE RegisterVideoMemoryBudgetChangeNotificationEvent( 
    /* [annotation][in] */ 
    _In_  HANDLE hEvent,
    /* [annotation][out] */ 
    _Out_  DWORD *pdwCookie) = 0;

  virtual void STDMETHODCALLTYPE UnregisterVideoMemoryBudgetChangeNotification( 
    /* [annotation][in] */ 
    _In_  DWORD dwCookie) = 0;

};

typedef HRESULT (STDMETHODCALLTYPE *PresentSwapChain_pfn)(
                                       IDXGISwapChain *This,
                                       UINT            SyncInterval,
                                       UINT            Flags);

typedef HRESULT (STDMETHODCALLTYPE *Present1SwapChain1_pfn)(
                                       IDXGISwapChain1         *This,
                                       UINT                     SyncInterval,
                                       UINT                     Flags,
                                 const DXGI_PRESENT_PARAMETERS *pPresentParameters);

typedef HRESULT (STDMETHODCALLTYPE *CreateSwapChain_pfn)(
                                       IDXGIFactory          *This,
                           _In_        IUnknown              *pDevice,
                           _In_  const DXGI_SWAP_CHAIN_DESC  *pDesc,
                          _Out_        IDXGISwapChain       **ppSwapChain);

typedef HRESULT (STDMETHODCALLTYPE *CreateSwapChainForHwnd_pfn)(
                                       IDXGIFactory2                    *This,
                            _In_       IUnknown                         *pDevice,
                            _In_       HWND                              hWnd,
                            _In_ const DXGI_SWAP_CHAIN_DESC1            *pDesc,
                        _In_opt_ const DXGI_SWAP_CHAIN_FULLSCREEN_DESC  *pFullscreenDesc,
                        _In_opt_       IDXGIOutput                      *pRestrictToOutput,
                           _Out_       IDXGISwapChain1                 **ppSwapChain);

typedef HRESULT (STDMETHODCALLTYPE *CreateSwapChainForCoreWindow_pfn)(
                                       IDXGIFactory2                   *This,
                            _In_       IUnknown                        *pDevice,
                            _In_       IUnknown                        *pWindow,
                            _In_ const DXGI_SWAP_CHAIN_DESC1           *pDesc,
                        _In_opt_       IDXGIOutput                     *pRestrictToOutput,
                           _Out_       IDXGISwapChain1                **ppSwapChain);

typedef HRESULT (STDMETHODCALLTYPE *CreateSwapChainForComposition_pfn)(
                                       IDXGIFactory2          *This,
                        _In_           IUnknown               *pDevice,
                        _In_     const DXGI_SWAP_CHAIN_DESC1  *pDesc,
                        _In_opt_       IDXGIOutput            *pRestrictToOutput,
                        _Outptr_       IDXGISwapChain1       **ppSwapChain);

typedef HRESULT (STDMETHODCALLTYPE *SetFullscreenState_pfn)(
                                       IDXGISwapChain *This,
                                       BOOL            Fullscreen,
                                       IDXGIOutput    *pTarget);

typedef HRESULT (STDMETHODCALLTYPE *GetFullscreenState_pfn)(
                                       IDXGISwapChain  *This,
                            _Out_opt_  BOOL            *pFullscreen,
                            _Out_opt_  IDXGIOutput    **ppTarget );

typedef HRESULT (STDMETHODCALLTYPE *ResizeBuffers_pfn)(
                                       IDXGISwapChain *This,
                            /* [in] */ UINT            BufferCount,
                            /* [in] */ UINT            Width,
                            /* [in] */ UINT            Height,
                            /* [in] */ DXGI_FORMAT     NewFormat,
                            /* [in] */ UINT            SwapChainFlags);

typedef HRESULT (STDMETHODCALLTYPE *ResizeTarget_pfn)(
                                  _In_ IDXGISwapChain  *This,
                            _In_ const DXGI_MODE_DESC  *pNewTargetParameters );

typedef HRESULT (STDMETHODCALLTYPE *GetDisplayModeList_pfn)(
                                       IDXGIOutput     *This,
                            /* [in] */ DXGI_FORMAT      EnumFormat,
                            /* [in] */ UINT             Flags,
                            /* [annotation][out][in] */
                              _Inout_  UINT            *pNumModes,
                            /* [annotation][out] */
_Out_writes_to_opt_(*pNumModes,*pNumModes)
                                       DXGI_MODE_DESC *pDesc );

typedef HRESULT (STDMETHODCALLTYPE *FindClosestMatchingMode_pfn)(
                                       IDXGIOutput    *This,
                           /* [annotation][in] */
                           _In_  const DXGI_MODE_DESC *pModeToMatch,
                           /* [annotation][out] */
                           _Out_       DXGI_MODE_DESC *pClosestMatch,
                           /* [annotation][in] */
                            _In_opt_  IUnknown *pConcernedDevice );

typedef HRESULT (STDMETHODCALLTYPE *WaitForVBlank_pfn)(
                                       IDXGIOutput    *This );


typedef HRESULT (STDMETHODCALLTYPE *GetDesc1_pfn)(IDXGIAdapter1      *This,
                                           _Out_  DXGI_ADAPTER_DESC1 *pDesc);
typedef HRESULT (STDMETHODCALLTYPE *GetDesc2_pfn)(IDXGIAdapter2      *This,
                                           _Out_  DXGI_ADAPTER_DESC2 *pDesc);
typedef HRESULT (STDMETHODCALLTYPE *GetDesc_pfn) (IDXGIAdapter       *This,
                                           _Out_  DXGI_ADAPTER_DESC  *pDesc);

typedef HRESULT (STDMETHODCALLTYPE *EnumAdapters_pfn)(
                                        IDXGIFactory  *This,
                                        UINT           Adapter,
                                  _Out_ IDXGIAdapter **ppAdapter);

typedef HRESULT (STDMETHODCALLTYPE *EnumAdapters1_pfn)(
                                        IDXGIFactory1  *This,
                                        UINT            Adapter,
                                  _Out_ IDXGIAdapter1 **ppAdapter);

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


struct ID3D11DeviceContext;
struct ID3D11ClassInstance;

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

struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11UnorderedAccessView;

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



#define DXGI_PRESENT_ALLOW_TEARING          0x00000200UL
#define DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING  2048

typedef
enum DXGI_FEATURE
{
    DXGI_FEATURE_PRESENT_ALLOW_TEARING = 0
} DXGI_FEATURE;

MIDL_INTERFACE("7632e1f5-ee65-4dca-87fd-84cd75f8838d")
IDXGIFactory5 : public IDXGIFactory4
{
  public:
      virtual HRESULT STDMETHODCALLTYPE CheckFeatureSupport(
                         DXGI_FEATURE Feature,
                        _Inout_updates_bytes_(FeatureSupportDataSize) void *pFeatureSupportData,
                         UINT FeatureSupportDataSize) = 0;
 };

#endif /* __SK__DXGI_INTERFACES_H__ */