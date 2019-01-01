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

#ifndef __SK__DXGI_SWAPCHAIN_H__
#define __SK__DXGI_SWAPCHAIN_H__

#pragma once

#include <SpecialK/render/dxgi/dxgi_interfaces.h>

extern volatile LONG SK_DXGI_LiveWrappedSwapChains;
extern volatile LONG SK_DXGI_LiveWrappedSwapChain1s;

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


/* header files for imported files */
#include "dxgi.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_dxgi1_2_0000_0000 */
/* [local] */ 



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
            _COM_Outptr_  IDXGIResource **ppDesktopResource) = 0;
        
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
            _COM_Outptr_  void **ppParent);
        
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
            _COM_Outptr_  IDXGIResource **ppDesktopResource);
        
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
            _COM_Outptr_  void **ppParentResource,
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
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
            IDXGISurface2 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppDevice);
        
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
            _COM_Outptr_  void **ppParentResource,
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
            _COM_Outptr_  IDXGISurface2 **ppSurface) = 0;
        
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
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
            IDXGIResource1 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetSharedHandle )( 
            IDXGIResource1 * This,
            /* [annotation][out] */ 
            _Out_  HANDLE *pSharedHandle);
        
        HRESULT ( STDMETHODCALLTYPE *GetUsage )( 
            IDXGIResource1 * This,
            /* [out] */ DXGI_USAGE *pUsage);
        
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
            _COM_Outptr_  IDXGISurface2 **ppSurface);
        
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
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetAdapter )( 
            IDXGIDevice2 * This,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter **pAdapter);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSurface )( 
            IDXGIDevice2 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_SURFACE_DESC *pDesc,
            /* [in] */ UINT NumSurfaces,
            /* [in] */ DXGI_USAGE Usage,
            /* [annotation][in] */ 
            _In_opt_  const DXGI_SHARED_RESOURCE *pSharedResource,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISurface **ppSurface);
        
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
            _COM_Outptr_  void **ppUnk) = 0;
        
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
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
            IDXGISwapChain1 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppDevice);
        
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
            _COM_Outptr_  void **ppSurface);
        
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
            _COM_Outptr_opt_result_maybenull_  IDXGIOutput **ppTarget);
        
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
            _COM_Outptr_  IDXGIOutput **ppOutput);
        
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
            _COM_Outptr_  void **ppUnk);
        
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
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain) = 0;
        
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
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain) = 0;
        
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
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain) = 0;
        
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
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *EnumAdapters )( 
            IDXGIFactory2 * This,
            /* [in] */ UINT Adapter,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter **ppAdapter);
        
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
            _COM_Outptr_  IDXGISwapChain **ppSwapChain);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSoftwareAdapter )( 
            IDXGIFactory2 * This,
            /* [in] */ HMODULE Module,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter **ppAdapter);
        
        HRESULT ( STDMETHODCALLTYPE *EnumAdapters1 )( 
            IDXGIFactory2 * This,
            /* [in] */ UINT Adapter,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter1 **ppAdapter);
        
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
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain);
        
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
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain);
        
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
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain);
        
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
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *EnumOutputs )( 
            IDXGIAdapter2 * This,
            /* [in] */ UINT Output,
            /* [annotation][out][in] */ 
            _COM_Outptr_  IDXGIOutput **ppOutput);
        
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
            _COM_Outptr_  IDXGIOutputDuplication **ppOutputDuplication) = 0;
        
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
            _COM_Outptr_  void **ppParent);
        
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
            _COM_Outptr_  IDXGIOutputDuplication **ppOutputDuplication);
        
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


/* header files for imported files */
#include "dxgi1_2.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_dxgi1_3_0000_0000 */
/* [local] */ 

#define DXGI_CREATE_FACTORY_DEBUG 0x1
HRESULT WINAPI CreateDXGIFactory2(UINT Flags, REFIID riid, _COM_Outptr_ void **ppFactory);
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
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetAdapter )( 
            IDXGIDevice3 * This,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter **pAdapter);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSurface )( 
            IDXGIDevice3 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_SURFACE_DESC *pDesc,
            /* [in] */ UINT NumSurfaces,
            /* [in] */ DXGI_USAGE Usage,
            /* [annotation][in] */ 
            _In_opt_  const DXGI_SHARED_RESOURCE *pSharedResource,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISurface **ppSurface);
        
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
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
            IDXGISwapChain2 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppDevice);
        
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
            _COM_Outptr_  void **ppSurface);
        
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
            _COM_Outptr_opt_result_maybenull_  IDXGIOutput **ppTarget);
        
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
            _COM_Outptr_  IDXGIOutput **ppOutput);
        
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
            _COM_Outptr_  void **ppUnk);
        
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
            _COM_Outptr_  void **ppParent);
        
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
            _COM_Outptr_  IDXGIOutputDuplication **ppOutputDuplication);
        
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
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *EnumAdapters )( 
            IDXGIFactory3 * This,
            /* [in] */ UINT Adapter,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter **ppAdapter);
        
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
            _COM_Outptr_  IDXGISwapChain **ppSwapChain);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSoftwareAdapter )( 
            IDXGIFactory3 * This,
            /* [in] */ HMODULE Module,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter **ppAdapter);
        
        HRESULT ( STDMETHODCALLTYPE *EnumAdapters1 )( 
            IDXGIFactory3 * This,
            /* [in] */ UINT Adapter,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter1 **ppAdapter);
        
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
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain);
        
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
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain);
        
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
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain);
        
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
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDecodeSwapChainForCompositionSurfaceHandle( 
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][in] */ 
            _In_opt_  HANDLE hSurface,
            /* [annotation][in] */ 
            _In_  DXGI_DECODE_SWAP_CHAIN_DESC *pDesc,
            /* [annotation][in] */ 
            _In_  IDXGIResource *pYuvDecodeBuffers,
            /* [annotation][in] */ 
            _In_opt_  IDXGIOutput *pRestrictToOutput,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIDecodeSwapChain **ppSwapChain) = 0;
        
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
            _COM_Outptr_  IDXGISwapChain1 **ppSwapChain);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDecodeSwapChainForCompositionSurfaceHandle )( 
            IDXGIFactoryMedia * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][in] */ 
            _In_opt_  HANDLE hSurface,
            /* [annotation][in] */ 
            _In_  DXGI_DECODE_SWAP_CHAIN_DESC *pDesc,
            /* [annotation][in] */ 
            _In_  IDXGIResource *pYuvDecodeBuffers,
            /* [annotation][in] */ 
            _In_opt_  IDXGIOutput *pRestrictToOutput,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIDecodeSwapChain **ppSwapChain);
        
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
        DXGI_FRAME_PRESENTATION_MODE_NONE	= 2,
        DXGI_FRAME_PRESENTATION_MODE_COMPOSITION_FAILURE	= 3
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
            _COM_Outptr_  void **ppParent);
        
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
            _COM_Outptr_  IDXGIOutputDuplication **ppOutputDuplication);
        
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

#ifndef __dxgi1_4_h__
#define __dxgi1_4_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDXGISwapChain3_FWD_DEFINED__
#define __IDXGISwapChain3_FWD_DEFINED__
typedef interface IDXGISwapChain3 IDXGISwapChain3;

#endif 	/* __IDXGISwapChain3_FWD_DEFINED__ */


#ifndef __IDXGIOutput4_FWD_DEFINED__
#define __IDXGIOutput4_FWD_DEFINED__
typedef interface IDXGIOutput4 IDXGIOutput4;

#endif 	/* __IDXGIOutput4_FWD_DEFINED__ */


#ifndef __IDXGIFactory4_FWD_DEFINED__
#define __IDXGIFactory4_FWD_DEFINED__
//typedef interface IDXGIFactory4 IDXGIFactory4;

#endif 	/* __IDXGIFactory4_FWD_DEFINED__ */


#ifndef __IDXGIAdapter3_FWD_DEFINED__
#define __IDXGIAdapter3_FWD_DEFINED__
typedef interface IDXGIAdapter3 IDXGIAdapter3;

#endif 	/* __IDXGIAdapter3_FWD_DEFINED__ */


/* header files for imported files */
#include "dxgi1_3.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_dxgi1_4_0000_0000 */
/* [local] */ 

typedef 
enum DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG
    {
        DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT	= 0x1,
        DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_OVERLAY_PRESENT	= 0x2
    } 	DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_4_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_4_0000_0000_v0_0_s_ifspec;

#ifndef __IDXGISwapChain3_INTERFACE_DEFINED__
#define __IDXGISwapChain3_INTERFACE_DEFINED__

/* interface IDXGISwapChain3 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGISwapChain3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("94d99bdb-f1f8-4ab0-b236-7da0170edab1")
    IDXGISwapChain3 : public IDXGISwapChain2
    {
    public:
        virtual UINT STDMETHODCALLTYPE GetCurrentBackBufferIndex( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CheckColorSpaceSupport( 
            /* [annotation][in] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace,
            /* [annotation][out] */ 
            _Out_  UINT *pColorSpaceSupport) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetColorSpace1( 
            /* [annotation][in] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResizeBuffers1( 
            /* [annotation][in] */ 
            _In_  UINT BufferCount,
            /* [annotation][in] */ 
            _In_  UINT Width,
            /* [annotation][in] */ 
            _In_  UINT Height,
            /* [annotation][in] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation][in] */ 
            _In_  UINT SwapChainFlags,
            /* [annotation][in] */ 
            _In_reads_(BufferCount)  const UINT *pCreationNodeMask,
            /* [annotation][in] */ 
            _In_reads_(BufferCount)  IUnknown *const *ppPresentQueue) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGISwapChain3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGISwapChain3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGISwapChain3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGISwapChain3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *Present )( 
            IDXGISwapChain3 * This,
            /* [in] */ UINT SyncInterval,
            /* [in] */ UINT Flags);
        
        HRESULT ( STDMETHODCALLTYPE *GetBuffer )( 
            IDXGISwapChain3 * This,
            /* [in] */ UINT Buffer,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][out][in] */ 
            _COM_Outptr_  void **ppSurface);
        
        HRESULT ( STDMETHODCALLTYPE *SetFullscreenState )( 
            IDXGISwapChain3 * This,
            /* [in] */ BOOL Fullscreen,
            /* [annotation][in] */ 
            _In_opt_  IDXGIOutput *pTarget);
        
        HRESULT ( STDMETHODCALLTYPE *GetFullscreenState )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_opt_  BOOL *pFullscreen,
            /* [annotation][out] */ 
            _COM_Outptr_opt_result_maybenull_  IDXGIOutput **ppTarget);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_SWAP_CHAIN_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeBuffers )( 
            IDXGISwapChain3 * This,
            /* [in] */ UINT BufferCount,
            /* [in] */ UINT Width,
            /* [in] */ UINT Height,
            /* [in] */ DXGI_FORMAT NewFormat,
            /* [in] */ UINT SwapChainFlags);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeTarget )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC *pNewTargetParameters);
        
        HRESULT ( STDMETHODCALLTYPE *GetContainingOutput )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIOutput **ppOutput);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_FRAME_STATISTICS *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *GetLastPresentCount )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  UINT *pLastPresentCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc1 )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_SWAP_CHAIN_DESC1 *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetFullscreenDesc )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetHwnd )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  HWND *pHwnd);
        
        HRESULT ( STDMETHODCALLTYPE *GetCoreWindow )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  REFIID refiid,
            /* [annotation][out] */ 
            _COM_Outptr_  void **ppUnk);
        
        HRESULT ( STDMETHODCALLTYPE *Present1 )( 
            IDXGISwapChain3 * This,
            /* [in] */ UINT SyncInterval,
            /* [in] */ UINT PresentFlags,
            /* [annotation][in] */ 
            _In_  const DXGI_PRESENT_PARAMETERS *pPresentParameters);
        
        BOOL ( STDMETHODCALLTYPE *IsTemporaryMonoSupported )( 
            IDXGISwapChain3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetRestrictToOutput )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  IDXGIOutput **ppRestrictToOutput);
        
        HRESULT ( STDMETHODCALLTYPE *SetBackgroundColor )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_RGBA *pColor);
        
        HRESULT ( STDMETHODCALLTYPE *GetBackgroundColor )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_RGBA *pColor);
        
        HRESULT ( STDMETHODCALLTYPE *SetRotation )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  DXGI_MODE_ROTATION Rotation);
        
        HRESULT ( STDMETHODCALLTYPE *GetRotation )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_ROTATION *pRotation);
        
        HRESULT ( STDMETHODCALLTYPE *SetSourceSize )( 
            IDXGISwapChain3 * This,
            UINT Width,
            UINT Height);
        
        HRESULT ( STDMETHODCALLTYPE *GetSourceSize )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  UINT *pWidth,
            /* [annotation][out] */ 
            _Out_  UINT *pHeight);
        
        HRESULT ( STDMETHODCALLTYPE *SetMaximumFrameLatency )( 
            IDXGISwapChain3 * This,
            UINT MaxLatency);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaximumFrameLatency )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  UINT *pMaxLatency);
        
        HANDLE ( STDMETHODCALLTYPE *GetFrameLatencyWaitableObject )( 
            IDXGISwapChain3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetMatrixTransform )( 
            IDXGISwapChain3 * This,
            const DXGI_MATRIX_3X2_F *pMatrix);
        
        HRESULT ( STDMETHODCALLTYPE *GetMatrixTransform )( 
            IDXGISwapChain3 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_MATRIX_3X2_F *pMatrix);
        
        UINT ( STDMETHODCALLTYPE *GetCurrentBackBufferIndex )( 
            IDXGISwapChain3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CheckColorSpaceSupport )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace,
            /* [annotation][out] */ 
            _Out_  UINT *pColorSpaceSupport);
        
        HRESULT ( STDMETHODCALLTYPE *SetColorSpace1 )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeBuffers1 )( 
            IDXGISwapChain3 * This,
            /* [annotation][in] */ 
            _In_  UINT BufferCount,
            /* [annotation][in] */ 
            _In_  UINT Width,
            /* [annotation][in] */ 
            _In_  UINT Height,
            /* [annotation][in] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation][in] */ 
            _In_  UINT SwapChainFlags,
            /* [annotation][in] */ 
            _In_reads_(BufferCount)  const UINT *pCreationNodeMask,
            /* [annotation][in] */ 
            _In_reads_(BufferCount)  IUnknown *const *ppPresentQueue);
        
        END_INTERFACE
    } IDXGISwapChain3Vtbl;

    interface IDXGISwapChain3
    {
        CONST_VTBL struct IDXGISwapChain3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGISwapChain3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGISwapChain3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGISwapChain3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGISwapChain3_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGISwapChain3_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGISwapChain3_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGISwapChain3_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGISwapChain3_GetDevice(This,riid,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppDevice) ) 


#define IDXGISwapChain3_Present(This,SyncInterval,Flags)	\
    ( (This)->lpVtbl -> Present(This,SyncInterval,Flags) ) 

#define IDXGISwapChain3_GetBuffer(This,Buffer,riid,ppSurface)	\
    ( (This)->lpVtbl -> GetBuffer(This,Buffer,riid,ppSurface) ) 

#define IDXGISwapChain3_SetFullscreenState(This,Fullscreen,pTarget)	\
    ( (This)->lpVtbl -> SetFullscreenState(This,Fullscreen,pTarget) ) 

#define IDXGISwapChain3_GetFullscreenState(This,pFullscreen,ppTarget)	\
    ( (This)->lpVtbl -> GetFullscreenState(This,pFullscreen,ppTarget) ) 

#define IDXGISwapChain3_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGISwapChain3_ResizeBuffers(This,BufferCount,Width,Height,NewFormat,SwapChainFlags)	\
    ( (This)->lpVtbl -> ResizeBuffers(This,BufferCount,Width,Height,NewFormat,SwapChainFlags) ) 

#define IDXGISwapChain3_ResizeTarget(This,pNewTargetParameters)	\
    ( (This)->lpVtbl -> ResizeTarget(This,pNewTargetParameters) ) 

#define IDXGISwapChain3_GetContainingOutput(This,ppOutput)	\
    ( (This)->lpVtbl -> GetContainingOutput(This,ppOutput) ) 

#define IDXGISwapChain3_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) ) 

#define IDXGISwapChain3_GetLastPresentCount(This,pLastPresentCount)	\
    ( (This)->lpVtbl -> GetLastPresentCount(This,pLastPresentCount) ) 


#define IDXGISwapChain3_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 

#define IDXGISwapChain3_GetFullscreenDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetFullscreenDesc(This,pDesc) ) 

#define IDXGISwapChain3_GetHwnd(This,pHwnd)	\
    ( (This)->lpVtbl -> GetHwnd(This,pHwnd) ) 

#define IDXGISwapChain3_GetCoreWindow(This,refiid,ppUnk)	\
    ( (This)->lpVtbl -> GetCoreWindow(This,refiid,ppUnk) ) 

#define IDXGISwapChain3_Present1(This,SyncInterval,PresentFlags,pPresentParameters)	\
    ( (This)->lpVtbl -> Present1(This,SyncInterval,PresentFlags,pPresentParameters) ) 

#define IDXGISwapChain3_IsTemporaryMonoSupported(This)	\
    ( (This)->lpVtbl -> IsTemporaryMonoSupported(This) ) 

#define IDXGISwapChain3_GetRestrictToOutput(This,ppRestrictToOutput)	\
    ( (This)->lpVtbl -> GetRestrictToOutput(This,ppRestrictToOutput) ) 

#define IDXGISwapChain3_SetBackgroundColor(This,pColor)	\
    ( (This)->lpVtbl -> SetBackgroundColor(This,pColor) ) 

#define IDXGISwapChain3_GetBackgroundColor(This,pColor)	\
    ( (This)->lpVtbl -> GetBackgroundColor(This,pColor) ) 

#define IDXGISwapChain3_SetRotation(This,Rotation)	\
    ( (This)->lpVtbl -> SetRotation(This,Rotation) ) 

#define IDXGISwapChain3_GetRotation(This,pRotation)	\
    ( (This)->lpVtbl -> GetRotation(This,pRotation) ) 


#define IDXGISwapChain3_SetSourceSize(This,Width,Height)	\
    ( (This)->lpVtbl -> SetSourceSize(This,Width,Height) ) 

#define IDXGISwapChain3_GetSourceSize(This,pWidth,pHeight)	\
    ( (This)->lpVtbl -> GetSourceSize(This,pWidth,pHeight) ) 

#define IDXGISwapChain3_SetMaximumFrameLatency(This,MaxLatency)	\
    ( (This)->lpVtbl -> SetMaximumFrameLatency(This,MaxLatency) ) 

#define IDXGISwapChain3_GetMaximumFrameLatency(This,pMaxLatency)	\
    ( (This)->lpVtbl -> GetMaximumFrameLatency(This,pMaxLatency) ) 

#define IDXGISwapChain3_GetFrameLatencyWaitableObject(This)	\
    ( (This)->lpVtbl -> GetFrameLatencyWaitableObject(This) ) 

#define IDXGISwapChain3_SetMatrixTransform(This,pMatrix)	\
    ( (This)->lpVtbl -> SetMatrixTransform(This,pMatrix) ) 

#define IDXGISwapChain3_GetMatrixTransform(This,pMatrix)	\
    ( (This)->lpVtbl -> GetMatrixTransform(This,pMatrix) ) 


#define IDXGISwapChain3_GetCurrentBackBufferIndex(This)	\
    ( (This)->lpVtbl -> GetCurrentBackBufferIndex(This) ) 

#define IDXGISwapChain3_CheckColorSpaceSupport(This,ColorSpace,pColorSpaceSupport)	\
    ( (This)->lpVtbl -> CheckColorSpaceSupport(This,ColorSpace,pColorSpaceSupport) ) 

#define IDXGISwapChain3_SetColorSpace1(This,ColorSpace)	\
    ( (This)->lpVtbl -> SetColorSpace1(This,ColorSpace) ) 

#define IDXGISwapChain3_ResizeBuffers1(This,BufferCount,Width,Height,Format,SwapChainFlags,pCreationNodeMask,ppPresentQueue)	\
    ( (This)->lpVtbl -> ResizeBuffers1(This,BufferCount,Width,Height,Format,SwapChainFlags,pCreationNodeMask,ppPresentQueue) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGISwapChain3_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_4_0000_0001 */
/* [local] */ 

typedef 
enum DXGI_OVERLAY_COLOR_SPACE_SUPPORT_FLAG
    {
        DXGI_OVERLAY_COLOR_SPACE_SUPPORT_FLAG_PRESENT	= 0x1
    } 	DXGI_OVERLAY_COLOR_SPACE_SUPPORT_FLAG;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_4_0000_0001_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_4_0000_0001_v0_0_s_ifspec;

#ifndef __IDXGIOutput4_INTERFACE_DEFINED__
#define __IDXGIOutput4_INTERFACE_DEFINED__

/* interface IDXGIOutput4 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIOutput4;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("dc7dca35-2196-414d-9F53-617884032a60")
    IDXGIOutput4 : public IDXGIOutput3
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CheckOverlayColorSpaceSupport( 
            /* [annotation][in] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation][in] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace,
            /* [annotation][in] */ 
            _In_  IUnknown *pConcernedDevice,
            /* [annotation][out] */ 
            _Out_  UINT *pFlags) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIOutput4Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIOutput4 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIOutput4 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIOutput4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
            IDXGIOutput4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_OUTPUT_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList )( 
            IDXGIOutput4 * This,
            /* [in] */ DXGI_FORMAT EnumFormat,
            /* [in] */ UINT Flags,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pNumModes,
            /* [annotation][out] */ 
            _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC *pModeToMatch,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_DESC *pClosestMatch,
            /* [annotation][in] */ 
            _In_opt_  IUnknown *pConcernedDevice);
        
        HRESULT ( STDMETHODCALLTYPE *WaitForVBlank )( 
            IDXGIOutput4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *TakeOwnership )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            BOOL Exclusive);
        
        void ( STDMETHODCALLTYPE *ReleaseOwnership )( 
            IDXGIOutput4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetGammaControlCapabilities )( 
            IDXGIOutput4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_GAMMA_CONTROL_CAPABILITIES *pGammaCaps);
        
        HRESULT ( STDMETHODCALLTYPE *SetGammaControl )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_GAMMA_CONTROL *pArray);
        
        HRESULT ( STDMETHODCALLTYPE *GetGammaControl )( 
            IDXGIOutput4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_GAMMA_CONTROL *pArray);
        
        HRESULT ( STDMETHODCALLTYPE *SetDisplaySurface )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  IDXGISurface *pScanoutSurface);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  IDXGISurface *pDestination);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
            IDXGIOutput4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_FRAME_STATISTICS *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList1 )( 
            IDXGIOutput4 * This,
            /* [in] */ DXGI_FORMAT EnumFormat,
            /* [in] */ UINT Flags,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pNumModes,
            /* [annotation][out] */ 
            _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC1 *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode1 )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC1 *pModeToMatch,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_DESC1 *pClosestMatch,
            /* [annotation][in] */ 
            _In_opt_  IUnknown *pConcernedDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData1 )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  IDXGIResource *pDestination);
        
        HRESULT ( STDMETHODCALLTYPE *DuplicateOutput )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIOutputDuplication **ppOutputDuplication);
        
        BOOL ( STDMETHODCALLTYPE *SupportsOverlays )( 
            IDXGIOutput4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CheckOverlaySupport )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  DXGI_FORMAT EnumFormat,
            /* [annotation][out] */ 
            _In_  IUnknown *pConcernedDevice,
            /* [annotation][out] */ 
            _Out_  UINT *pFlags);
        
        HRESULT ( STDMETHODCALLTYPE *CheckOverlayColorSpaceSupport )( 
            IDXGIOutput4 * This,
            /* [annotation][in] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation][in] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace,
            /* [annotation][in] */ 
            _In_  IUnknown *pConcernedDevice,
            /* [annotation][out] */ 
            _Out_  UINT *pFlags);
        
        END_INTERFACE
    } IDXGIOutput4Vtbl;

    interface IDXGIOutput4
    {
        CONST_VTBL struct IDXGIOutput4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIOutput4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIOutput4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIOutput4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIOutput4_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIOutput4_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIOutput4_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIOutput4_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIOutput4_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGIOutput4_GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput4_FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput4_WaitForVBlank(This)	\
    ( (This)->lpVtbl -> WaitForVBlank(This) ) 

#define IDXGIOutput4_TakeOwnership(This,pDevice,Exclusive)	\
    ( (This)->lpVtbl -> TakeOwnership(This,pDevice,Exclusive) ) 

#define IDXGIOutput4_ReleaseOwnership(This)	\
    ( (This)->lpVtbl -> ReleaseOwnership(This) ) 

#define IDXGIOutput4_GetGammaControlCapabilities(This,pGammaCaps)	\
    ( (This)->lpVtbl -> GetGammaControlCapabilities(This,pGammaCaps) ) 

#define IDXGIOutput4_SetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> SetGammaControl(This,pArray) ) 

#define IDXGIOutput4_GetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> GetGammaControl(This,pArray) ) 

#define IDXGIOutput4_SetDisplaySurface(This,pScanoutSurface)	\
    ( (This)->lpVtbl -> SetDisplaySurface(This,pScanoutSurface) ) 

#define IDXGIOutput4_GetDisplaySurfaceData(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData(This,pDestination) ) 

#define IDXGIOutput4_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) ) 


#define IDXGIOutput4_GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput4_FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput4_GetDisplaySurfaceData1(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData1(This,pDestination) ) 

#define IDXGIOutput4_DuplicateOutput(This,pDevice,ppOutputDuplication)	\
    ( (This)->lpVtbl -> DuplicateOutput(This,pDevice,ppOutputDuplication) ) 


#define IDXGIOutput4_SupportsOverlays(This)	\
    ( (This)->lpVtbl -> SupportsOverlays(This) ) 


#define IDXGIOutput4_CheckOverlaySupport(This,EnumFormat,pConcernedDevice,pFlags)	\
    ( (This)->lpVtbl -> CheckOverlaySupport(This,EnumFormat,pConcernedDevice,pFlags) ) 


#define IDXGIOutput4_CheckOverlayColorSpaceSupport(This,Format,ColorSpace,pConcernedDevice,pFlags)	\
    ( (This)->lpVtbl -> CheckOverlayColorSpaceSupport(This,Format,ColorSpace,pConcernedDevice,pFlags) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIOutput4_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_4_0000_0004 */
/* [local] */ 

DEFINE_GUID(IID_IDXGISwapChain3,0x94d99bdb,0xf1f8,0x4ab0,0xb2,0x36,0x7d,0xa0,0x17,0x0e,0xda,0xb1);
DEFINE_GUID(IID_IDXGIOutput4,0xdc7dca35,0x2196,0x414d,0x9F,0x53,0x61,0x78,0x84,0x03,0x2a,0x60);
DEFINE_GUID(IID_IDXGIFactory4,0x1bc6ea02,0xef36,0x464f,0xbf,0x0c,0x21,0xca,0x39,0xe5,0x16,0x8a);
DEFINE_GUID(IID_IDXGIAdapter3,0x645967A4,0x1392,0x4310,0xA7,0x98,0x80,0x53,0xCE,0x3E,0x93,0xFD);


extern RPC_IF_HANDLE __MIDL_itf_dxgi1_4_0000_0004_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_4_0000_0004_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif





struct __declspec(uuid("CB285C3B-3677-4332-98C7-D6339B9782B1")) DXGIDevice;
struct __declspec(uuid("1F445F9F-9887-4C4C-9055-4E3BADAFCCA8")) DXGISwapChain;





/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0613 */
/* @@MIDL_FILE_HEADING(  ) */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
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

#ifndef __dxgi1_5_h__
#define __dxgi1_5_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDXGIOutput5_FWD_DEFINED__
#define __IDXGIOutput5_FWD_DEFINED__
typedef interface IDXGIOutput5 IDXGIOutput5;

#endif 	/* __IDXGIOutput5_FWD_DEFINED__ */


#ifndef __IDXGISwapChain4_FWD_DEFINED__
#define __IDXGISwapChain4_FWD_DEFINED__
typedef interface IDXGISwapChain4 IDXGISwapChain4;

#endif 	/* __IDXGISwapChain4_FWD_DEFINED__ */


#ifndef __IDXGIDevice4_FWD_DEFINED__
#define __IDXGIDevice4_FWD_DEFINED__
typedef interface IDXGIDevice4 IDXGIDevice4;

#endif 	/* __IDXGIDevice4_FWD_DEFINED__ */


#ifndef __IDXGIFactory5_FWD_DEFINED__
#define __IDXGIFactory5_FWD_DEFINED__
typedef interface IDXGIFactory5 IDXGIFactory5;

#endif 	/* __IDXGIFactory5_FWD_DEFINED__ */


/* header files for imported files */
//#include "dxgi1_4.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_dxgi1_5_0000_0000 */
/* [local] */ 

/*#include <winapifamily.h>*/
/*#pragma region App Family*/
/*#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)*/
typedef 
enum DXGI_OUTDUPL_FLAG
    {
        DXGI_OUTDUPL_COMPOSITED_UI_CAPTURE_ONLY	= 1
    } 	DXGI_OUTDUPL_FLAG;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_5_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_5_0000_0000_v0_0_s_ifspec;

#ifndef __IDXGIOutput5_INTERFACE_DEFINED__
#define __IDXGIOutput5_INTERFACE_DEFINED__

/* interface IDXGIOutput5 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIOutput5;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("80A07424-AB52-42EB-833C-0C42FD282D98")
    IDXGIOutput5 : public IDXGIOutput4
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DuplicateOutput1( 
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [in] */ UINT Flags,
            /* [annotation][in] */ 
            _In_  UINT SupportedFormatsCount,
            /* [annotation][in] */ 
            _In_reads_(SupportedFormatsCount)  const DXGI_FORMAT *pSupportedFormats,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIOutputDuplication **ppOutputDuplication) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIOutput5Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIOutput5 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIOutput5 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIOutput5 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
            IDXGIOutput5 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_OUTPUT_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList )( 
            IDXGIOutput5 * This,
            /* [in] */ DXGI_FORMAT EnumFormat,
            /* [in] */ UINT Flags,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pNumModes,
            /* [annotation][out] */ 
            _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC *pModeToMatch,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_DESC *pClosestMatch,
            /* [annotation][in] */ 
            _In_opt_  IUnknown *pConcernedDevice);
        
        HRESULT ( STDMETHODCALLTYPE *WaitForVBlank )( 
            IDXGIOutput5 * This);
        
        HRESULT ( STDMETHODCALLTYPE *TakeOwnership )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            BOOL Exclusive);
        
        void ( STDMETHODCALLTYPE *ReleaseOwnership )( 
            IDXGIOutput5 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetGammaControlCapabilities )( 
            IDXGIOutput5 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_GAMMA_CONTROL_CAPABILITIES *pGammaCaps);
        
        HRESULT ( STDMETHODCALLTYPE *SetGammaControl )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_GAMMA_CONTROL *pArray);
        
        HRESULT ( STDMETHODCALLTYPE *GetGammaControl )( 
            IDXGIOutput5 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_GAMMA_CONTROL *pArray);
        
        HRESULT ( STDMETHODCALLTYPE *SetDisplaySurface )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  IDXGISurface *pScanoutSurface);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  IDXGISurface *pDestination);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
            IDXGIOutput5 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_FRAME_STATISTICS *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList1 )( 
            IDXGIOutput5 * This,
            /* [in] */ DXGI_FORMAT EnumFormat,
            /* [in] */ UINT Flags,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pNumModes,
            /* [annotation][out] */ 
            _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC1 *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode1 )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC1 *pModeToMatch,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_DESC1 *pClosestMatch,
            /* [annotation][in] */ 
            _In_opt_  IUnknown *pConcernedDevice);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData1 )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  IDXGIResource *pDestination);
        
        HRESULT ( STDMETHODCALLTYPE *DuplicateOutput )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIOutputDuplication **ppOutputDuplication);
        
        BOOL ( STDMETHODCALLTYPE *SupportsOverlays )( 
            IDXGIOutput5 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CheckOverlaySupport )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  DXGI_FORMAT EnumFormat,
            /* [annotation][out] */ 
            _In_  IUnknown *pConcernedDevice,
            /* [annotation][out] */ 
            _Out_  UINT *pFlags);
        
        HRESULT ( STDMETHODCALLTYPE *CheckOverlayColorSpaceSupport )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation][in] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace,
            /* [annotation][in] */ 
            _In_  IUnknown *pConcernedDevice,
            /* [annotation][out] */ 
            _Out_  UINT *pFlags);
        
        HRESULT ( STDMETHODCALLTYPE *DuplicateOutput1 )( 
            IDXGIOutput5 * This,
            /* [annotation][in] */ 
            _In_  IUnknown *pDevice,
            /* [in] */ UINT Flags,
            /* [annotation][in] */ 
            _In_  UINT SupportedFormatsCount,
            /* [annotation][in] */ 
            _In_reads_(SupportedFormatsCount)  const DXGI_FORMAT *pSupportedFormats,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIOutputDuplication **ppOutputDuplication);
        
        END_INTERFACE
    } IDXGIOutput5Vtbl;

    interface IDXGIOutput5
    {
        CONST_VTBL struct IDXGIOutput5Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIOutput5_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIOutput5_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIOutput5_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIOutput5_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIOutput5_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIOutput5_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIOutput5_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIOutput5_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGIOutput5_GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput5_FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput5_WaitForVBlank(This)	\
    ( (This)->lpVtbl -> WaitForVBlank(This) ) 

#define IDXGIOutput5_TakeOwnership(This,pDevice,Exclusive)	\
    ( (This)->lpVtbl -> TakeOwnership(This,pDevice,Exclusive) ) 

#define IDXGIOutput5_ReleaseOwnership(This)	\
    ( (This)->lpVtbl -> ReleaseOwnership(This) ) 

#define IDXGIOutput5_GetGammaControlCapabilities(This,pGammaCaps)	\
    ( (This)->lpVtbl -> GetGammaControlCapabilities(This,pGammaCaps) ) 

#define IDXGIOutput5_SetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> SetGammaControl(This,pArray) ) 

#define IDXGIOutput5_GetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> GetGammaControl(This,pArray) ) 

#define IDXGIOutput5_SetDisplaySurface(This,pScanoutSurface)	\
    ( (This)->lpVtbl -> SetDisplaySurface(This,pScanoutSurface) ) 

#define IDXGIOutput5_GetDisplaySurfaceData(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData(This,pDestination) ) 

#define IDXGIOutput5_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) ) 


#define IDXGIOutput5_GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc) ) 

#define IDXGIOutput5_FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice) ) 

#define IDXGIOutput5_GetDisplaySurfaceData1(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData1(This,pDestination) ) 

#define IDXGIOutput5_DuplicateOutput(This,pDevice,ppOutputDuplication)	\
    ( (This)->lpVtbl -> DuplicateOutput(This,pDevice,ppOutputDuplication) ) 


#define IDXGIOutput5_SupportsOverlays(This)	\
    ( (This)->lpVtbl -> SupportsOverlays(This) ) 


#define IDXGIOutput5_CheckOverlaySupport(This,EnumFormat,pConcernedDevice,pFlags)	\
    ( (This)->lpVtbl -> CheckOverlaySupport(This,EnumFormat,pConcernedDevice,pFlags) ) 


#define IDXGIOutput5_CheckOverlayColorSpaceSupport(This,Format,ColorSpace,pConcernedDevice,pFlags)	\
    ( (This)->lpVtbl -> CheckOverlayColorSpaceSupport(This,Format,ColorSpace,pConcernedDevice,pFlags) ) 


#define IDXGIOutput5_DuplicateOutput1(This,pDevice,Flags,SupportedFormatsCount,pSupportedFormats,ppOutputDuplication)	\
    ( (This)->lpVtbl -> DuplicateOutput1(This,pDevice,Flags,SupportedFormatsCount,pSupportedFormats,ppOutputDuplication) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIOutput5_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_5_0000_0001 */
/* [local] */ 

typedef 
enum DXGI_HDR_METADATA_TYPE
    {
        DXGI_HDR_METADATA_TYPE_NONE	= 0,
        DXGI_HDR_METADATA_TYPE_HDR10	= 1
    } 	DXGI_HDR_METADATA_TYPE;

typedef struct DXGI_HDR_METADATA_HDR10
    {
    UINT16 RedPrimary[ 2 ];
    UINT16 GreenPrimary[ 2 ];
    UINT16 BluePrimary[ 2 ];
    UINT16 WhitePoint[ 2 ];
    UINT MaxMasteringLuminance;
    UINT MinMasteringLuminance;
    UINT16 MaxContentLightLevel;
    UINT16 MaxFrameAverageLightLevel;
    } 	DXGI_HDR_METADATA_HDR10;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_5_0000_0001_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_5_0000_0001_v0_0_s_ifspec;

#ifndef __IDXGISwapChain4_INTERFACE_DEFINED__
#define __IDXGISwapChain4_INTERFACE_DEFINED__

/* interface IDXGISwapChain4 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGISwapChain4;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3D585D5A-BD4A-489E-B1F4-3DBCB6452FFB")
    IDXGISwapChain4 : public IDXGISwapChain3
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetHDRMetaData( 
            /* [annotation][in] */ 
            _In_  DXGI_HDR_METADATA_TYPE Type,
            /* [annotation][in] */ 
            _In_  UINT Size,
            /* [annotation][size_is][in] */ 
            _In_reads_opt_(Size)  void *pMetaData) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGISwapChain4Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGISwapChain4 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGISwapChain4 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGISwapChain4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE *Present )( 
            IDXGISwapChain4 * This,
            /* [in] */ UINT SyncInterval,
            /* [in] */ UINT Flags);
        
        HRESULT ( STDMETHODCALLTYPE *GetBuffer )( 
            IDXGISwapChain4 * This,
            /* [in] */ UINT Buffer,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][out][in] */ 
            _COM_Outptr_  void **ppSurface);
        
        HRESULT ( STDMETHODCALLTYPE *SetFullscreenState )( 
            IDXGISwapChain4 * This,
            /* [in] */ BOOL Fullscreen,
            /* [annotation][in] */ 
            _In_opt_  IDXGIOutput *pTarget);
        
        HRESULT ( STDMETHODCALLTYPE *GetFullscreenState )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_opt_  BOOL *pFullscreen,
            /* [annotation][out] */ 
            _COM_Outptr_opt_result_maybenull_  IDXGIOutput **ppTarget);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_SWAP_CHAIN_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeBuffers )( 
            IDXGISwapChain4 * This,
            /* [in] */ UINT BufferCount,
            /* [in] */ UINT Width,
            /* [in] */ UINT Height,
            /* [in] */ DXGI_FORMAT NewFormat,
            /* [in] */ UINT SwapChainFlags);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeTarget )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_MODE_DESC *pNewTargetParameters);
        
        HRESULT ( STDMETHODCALLTYPE *GetContainingOutput )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIOutput **ppOutput);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_FRAME_STATISTICS *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *GetLastPresentCount )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  UINT *pLastPresentCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetDesc1 )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_SWAP_CHAIN_DESC1 *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetFullscreenDesc )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetHwnd )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  HWND *pHwnd);
        
        HRESULT ( STDMETHODCALLTYPE *GetCoreWindow )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  REFIID refiid,
            /* [annotation][out] */ 
            _COM_Outptr_  void **ppUnk);
        
        HRESULT ( STDMETHODCALLTYPE *Present1 )( 
            IDXGISwapChain4 * This,
            /* [in] */ UINT SyncInterval,
            /* [in] */ UINT PresentFlags,
            /* [annotation][in] */ 
            _In_  const DXGI_PRESENT_PARAMETERS *pPresentParameters);
        
        BOOL ( STDMETHODCALLTYPE *IsTemporaryMonoSupported )( 
            IDXGISwapChain4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetRestrictToOutput )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  IDXGIOutput **ppRestrictToOutput);
        
        HRESULT ( STDMETHODCALLTYPE *SetBackgroundColor )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_RGBA *pColor);
        
        HRESULT ( STDMETHODCALLTYPE *GetBackgroundColor )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_RGBA *pColor);
        
        HRESULT ( STDMETHODCALLTYPE *SetRotation )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  DXGI_MODE_ROTATION Rotation);
        
        HRESULT ( STDMETHODCALLTYPE *GetRotation )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_MODE_ROTATION *pRotation);
        
        HRESULT ( STDMETHODCALLTYPE *SetSourceSize )( 
            IDXGISwapChain4 * This,
            UINT Width,
            UINT Height);
        
        HRESULT ( STDMETHODCALLTYPE *GetSourceSize )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  UINT *pWidth,
            /* [annotation][out] */ 
            _Out_  UINT *pHeight);
        
        HRESULT ( STDMETHODCALLTYPE *SetMaximumFrameLatency )( 
            IDXGISwapChain4 * This,
            UINT MaxLatency);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaximumFrameLatency )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  UINT *pMaxLatency);
        
        HANDLE ( STDMETHODCALLTYPE *GetFrameLatencyWaitableObject )( 
            IDXGISwapChain4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetMatrixTransform )( 
            IDXGISwapChain4 * This,
            const DXGI_MATRIX_3X2_F *pMatrix);
        
        HRESULT ( STDMETHODCALLTYPE *GetMatrixTransform )( 
            IDXGISwapChain4 * This,
            /* [annotation][out] */ 
            _Out_  DXGI_MATRIX_3X2_F *pMatrix);
        
        UINT ( STDMETHODCALLTYPE *GetCurrentBackBufferIndex )( 
            IDXGISwapChain4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CheckColorSpaceSupport )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace,
            /* [annotation][out] */ 
            _Out_  UINT *pColorSpaceSupport);
        
        HRESULT ( STDMETHODCALLTYPE *SetColorSpace1 )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeBuffers1 )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  UINT BufferCount,
            /* [annotation][in] */ 
            _In_  UINT Width,
            /* [annotation][in] */ 
            _In_  UINT Height,
            /* [annotation][in] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation][in] */ 
            _In_  UINT SwapChainFlags,
            /* [annotation][in] */ 
            _In_reads_(BufferCount)  const UINT *pCreationNodeMask,
            /* [annotation][in] */ 
            _In_reads_(BufferCount)  IUnknown *const *ppPresentQueue);
        
        HRESULT ( STDMETHODCALLTYPE *SetHDRMetaData )( 
            IDXGISwapChain4 * This,
            /* [annotation][in] */ 
            _In_  DXGI_HDR_METADATA_TYPE Type,
            /* [annotation][in] */ 
            _In_  UINT Size,
            /* [annotation][size_is][in] */ 
            _In_reads_opt_(Size)  void *pMetaData);
        
        END_INTERFACE
    } IDXGISwapChain4Vtbl;

    interface IDXGISwapChain4
    {
        CONST_VTBL struct IDXGISwapChain4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGISwapChain4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGISwapChain4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGISwapChain4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGISwapChain4_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGISwapChain4_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGISwapChain4_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGISwapChain4_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGISwapChain4_GetDevice(This,riid,ppDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppDevice) ) 


#define IDXGISwapChain4_Present(This,SyncInterval,Flags)	\
    ( (This)->lpVtbl -> Present(This,SyncInterval,Flags) ) 

#define IDXGISwapChain4_GetBuffer(This,Buffer,riid,ppSurface)	\
    ( (This)->lpVtbl -> GetBuffer(This,Buffer,riid,ppSurface) ) 

#define IDXGISwapChain4_SetFullscreenState(This,Fullscreen,pTarget)	\
    ( (This)->lpVtbl -> SetFullscreenState(This,Fullscreen,pTarget) ) 

#define IDXGISwapChain4_GetFullscreenState(This,pFullscreen,ppTarget)	\
    ( (This)->lpVtbl -> GetFullscreenState(This,pFullscreen,ppTarget) ) 

#define IDXGISwapChain4_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) ) 

#define IDXGISwapChain4_ResizeBuffers(This,BufferCount,Width,Height,NewFormat,SwapChainFlags)	\
    ( (This)->lpVtbl -> ResizeBuffers(This,BufferCount,Width,Height,NewFormat,SwapChainFlags) ) 

#define IDXGISwapChain4_ResizeTarget(This,pNewTargetParameters)	\
    ( (This)->lpVtbl -> ResizeTarget(This,pNewTargetParameters) ) 

#define IDXGISwapChain4_GetContainingOutput(This,ppOutput)	\
    ( (This)->lpVtbl -> GetContainingOutput(This,ppOutput) ) 

#define IDXGISwapChain4_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) ) 

#define IDXGISwapChain4_GetLastPresentCount(This,pLastPresentCount)	\
    ( (This)->lpVtbl -> GetLastPresentCount(This,pLastPresentCount) ) 


#define IDXGISwapChain4_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) ) 

#define IDXGISwapChain4_GetFullscreenDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetFullscreenDesc(This,pDesc) ) 

#define IDXGISwapChain4_GetHwnd(This,pHwnd)	\
    ( (This)->lpVtbl -> GetHwnd(This,pHwnd) ) 

#define IDXGISwapChain4_GetCoreWindow(This,refiid,ppUnk)	\
    ( (This)->lpVtbl -> GetCoreWindow(This,refiid,ppUnk) ) 

#define IDXGISwapChain4_Present1(This,SyncInterval,PresentFlags,pPresentParameters)	\
    ( (This)->lpVtbl -> Present1(This,SyncInterval,PresentFlags,pPresentParameters) ) 

#define IDXGISwapChain4_IsTemporaryMonoSupported(This)	\
    ( (This)->lpVtbl -> IsTemporaryMonoSupported(This) ) 

#define IDXGISwapChain4_GetRestrictToOutput(This,ppRestrictToOutput)	\
    ( (This)->lpVtbl -> GetRestrictToOutput(This,ppRestrictToOutput) ) 

#define IDXGISwapChain4_SetBackgroundColor(This,pColor)	\
    ( (This)->lpVtbl -> SetBackgroundColor(This,pColor) ) 

#define IDXGISwapChain4_GetBackgroundColor(This,pColor)	\
    ( (This)->lpVtbl -> GetBackgroundColor(This,pColor) ) 

#define IDXGISwapChain4_SetRotation(This,Rotation)	\
    ( (This)->lpVtbl -> SetRotation(This,Rotation) ) 

#define IDXGISwapChain4_GetRotation(This,pRotation)	\
    ( (This)->lpVtbl -> GetRotation(This,pRotation) ) 


#define IDXGISwapChain4_SetSourceSize(This,Width,Height)	\
    ( (This)->lpVtbl -> SetSourceSize(This,Width,Height) ) 

#define IDXGISwapChain4_GetSourceSize(This,pWidth,pHeight)	\
    ( (This)->lpVtbl -> GetSourceSize(This,pWidth,pHeight) ) 

#define IDXGISwapChain4_SetMaximumFrameLatency(This,MaxLatency)	\
    ( (This)->lpVtbl -> SetMaximumFrameLatency(This,MaxLatency) ) 

#define IDXGISwapChain4_GetMaximumFrameLatency(This,pMaxLatency)	\
    ( (This)->lpVtbl -> GetMaximumFrameLatency(This,pMaxLatency) ) 

#define IDXGISwapChain4_GetFrameLatencyWaitableObject(This)	\
    ( (This)->lpVtbl -> GetFrameLatencyWaitableObject(This) ) 

#define IDXGISwapChain4_SetMatrixTransform(This,pMatrix)	\
    ( (This)->lpVtbl -> SetMatrixTransform(This,pMatrix) ) 

#define IDXGISwapChain4_GetMatrixTransform(This,pMatrix)	\
    ( (This)->lpVtbl -> GetMatrixTransform(This,pMatrix) ) 


#define IDXGISwapChain4_GetCurrentBackBufferIndex(This)	\
    ( (This)->lpVtbl -> GetCurrentBackBufferIndex(This) ) 

#define IDXGISwapChain4_CheckColorSpaceSupport(This,ColorSpace,pColorSpaceSupport)	\
    ( (This)->lpVtbl -> CheckColorSpaceSupport(This,ColorSpace,pColorSpaceSupport) ) 

#define IDXGISwapChain4_SetColorSpace1(This,ColorSpace)	\
    ( (This)->lpVtbl -> SetColorSpace1(This,ColorSpace) ) 

#define IDXGISwapChain4_ResizeBuffers1(This,BufferCount,Width,Height,Format,SwapChainFlags,pCreationNodeMask,ppPresentQueue)	\
    ( (This)->lpVtbl -> ResizeBuffers1(This,BufferCount,Width,Height,Format,SwapChainFlags,pCreationNodeMask,ppPresentQueue) ) 


#define IDXGISwapChain4_SetHDRMetaData(This,Type,Size,pMetaData)	\
    ( (This)->lpVtbl -> SetHDRMetaData(This,Type,Size,pMetaData) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGISwapChain4_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_5_0000_0002 */
/* [local] */ 

typedef 
enum _DXGI_OFFER_RESOURCE_FLAGS
    {
        DXGI_OFFER_RESOURCE_FLAG_ALLOW_DECOMMIT	= 0x1
    } 	DXGI_OFFER_RESOURCE_FLAGS;

typedef 
enum _DXGI_RECLAIM_RESOURCE_RESULTS
    {
        DXGI_RECLAIM_RESOURCE_RESULT_OK	= 0,
        DXGI_RECLAIM_RESOURCE_RESULT_DISCARDED	= 1,
        DXGI_RECLAIM_RESOURCE_RESULT_NOT_COMMITTED	= 2
    } 	DXGI_RECLAIM_RESOURCE_RESULTS;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_5_0000_0002_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_5_0000_0002_v0_0_s_ifspec;

#ifndef __IDXGIDevice4_INTERFACE_DEFINED__
#define __IDXGIDevice4_INTERFACE_DEFINED__

/* interface IDXGIDevice4 */
/* [unique][local][uuid][object] */ 


EXTERN_C const IID IID_IDXGIDevice4;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("95B4F95F-D8DA-4CA4-9EE6-3B76D5968A10")
    IDXGIDevice4 : public IDXGIDevice3
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OfferResources1( 
            /* [annotation][in] */ 
            _In_  UINT NumResources,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IDXGIResource *const *ppResources,
            /* [annotation][in] */ 
            _In_  DXGI_OFFER_RESOURCE_PRIORITY Priority,
            /* [annotation][in] */ 
            _In_  UINT Flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReclaimResources1( 
            /* [annotation][in] */ 
            _In_  UINT NumResources,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IDXGIResource *const *ppResources,
            /* [annotation][size_is][out] */ 
            _Out_writes_all_(NumResources)  DXGI_RECLAIM_RESOURCE_RESULTS *pResults) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDXGIDevice4Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGIDevice4 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGIDevice4 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGIDevice4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            IDXGIDevice4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */ 
            _In_reads_bytes_(DataSize)  const void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            IDXGIDevice4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][in] */ 
            _In_opt_  const IUnknown *pUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            IDXGIDevice4 * This,
            /* [annotation][in] */ 
            _In_  REFGUID Name,
            /* [annotation][out][in] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */ 
            _Out_writes_bytes_(*pDataSize)  void *pData);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IDXGIDevice4 * This,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][retval][out] */ 
            _COM_Outptr_  void **ppParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetAdapter )( 
            IDXGIDevice4 * This,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGIAdapter **pAdapter);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSurface )( 
            IDXGIDevice4 * This,
            /* [annotation][in] */ 
            _In_  const DXGI_SURFACE_DESC *pDesc,
            /* [in] */ UINT NumSurfaces,
            /* [in] */ DXGI_USAGE Usage,
            /* [annotation][in] */ 
            _In_opt_  const DXGI_SHARED_RESOURCE *pSharedResource,
            /* [annotation][out] */ 
            _COM_Outptr_  IDXGISurface **ppSurface);
        
        HRESULT ( STDMETHODCALLTYPE *QueryResourceResidency )( 
            IDXGIDevice4 * This,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IUnknown *const *ppResources,
            /* [annotation][size_is][out] */ 
            _Out_writes_(NumResources)  DXGI_RESIDENCY *pResidencyStatus,
            /* [in] */ UINT NumResources);
        
        HRESULT ( STDMETHODCALLTYPE *SetGPUThreadPriority )( 
            IDXGIDevice4 * This,
            /* [in] */ INT Priority);
        
        HRESULT ( STDMETHODCALLTYPE *GetGPUThreadPriority )( 
            IDXGIDevice4 * This,
            /* [annotation][retval][out] */ 
            _Out_  INT *pPriority);
        
        HRESULT ( STDMETHODCALLTYPE *SetMaximumFrameLatency )( 
            IDXGIDevice4 * This,
            /* [in] */ UINT MaxLatency);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaximumFrameLatency )( 
            IDXGIDevice4 * This,
            /* [annotation][out] */ 
            _Out_  UINT *pMaxLatency);
        
        HRESULT ( STDMETHODCALLTYPE *OfferResources )( 
            IDXGIDevice4 * This,
            /* [annotation][in] */ 
            _In_  UINT NumResources,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IDXGIResource *const *ppResources,
            /* [annotation][in] */ 
            _In_  DXGI_OFFER_RESOURCE_PRIORITY Priority);
        
        HRESULT ( STDMETHODCALLTYPE *ReclaimResources )( 
            IDXGIDevice4 * This,
            /* [annotation][in] */ 
            _In_  UINT NumResources,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IDXGIResource *const *ppResources,
            /* [annotation][size_is][out] */ 
            _Out_writes_all_opt_(NumResources)  BOOL *pDiscarded);
        
        HRESULT ( STDMETHODCALLTYPE *EnqueueSetEvent )( 
            IDXGIDevice4 * This,
            /* [annotation][in] */ 
            _In_  HANDLE hEvent);
        
        void ( STDMETHODCALLTYPE *Trim )( 
            IDXGIDevice4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *OfferResources1 )( 
            IDXGIDevice4 * This,
            /* [annotation][in] */ 
            _In_  UINT NumResources,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IDXGIResource *const *ppResources,
            /* [annotation][in] */ 
            _In_  DXGI_OFFER_RESOURCE_PRIORITY Priority,
            /* [annotation][in] */ 
            _In_  UINT Flags);
        
        HRESULT ( STDMETHODCALLTYPE *ReclaimResources1 )( 
            IDXGIDevice4 * This,
            /* [annotation][in] */ 
            _In_  UINT NumResources,
            /* [annotation][size_is][in] */ 
            _In_reads_(NumResources)  IDXGIResource *const *ppResources,
            /* [annotation][size_is][out] */ 
            _Out_writes_all_(NumResources)  DXGI_RECLAIM_RESOURCE_RESULTS *pResults);
        
        END_INTERFACE
    } IDXGIDevice4Vtbl;

    interface IDXGIDevice4
    {
        CONST_VTBL struct IDXGIDevice4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGIDevice4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIDevice4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIDevice4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIDevice4_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIDevice4_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIDevice4_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIDevice4_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIDevice4_GetAdapter(This,pAdapter)	\
    ( (This)->lpVtbl -> GetAdapter(This,pAdapter) ) 

#define IDXGIDevice4_CreateSurface(This,pDesc,NumSurfaces,Usage,pSharedResource,ppSurface)	\
    ( (This)->lpVtbl -> CreateSurface(This,pDesc,NumSurfaces,Usage,pSharedResource,ppSurface) ) 

#define IDXGIDevice4_QueryResourceResidency(This,ppResources,pResidencyStatus,NumResources)	\
    ( (This)->lpVtbl -> QueryResourceResidency(This,ppResources,pResidencyStatus,NumResources) ) 

#define IDXGIDevice4_SetGPUThreadPriority(This,Priority)	\
    ( (This)->lpVtbl -> SetGPUThreadPriority(This,Priority) ) 

#define IDXGIDevice4_GetGPUThreadPriority(This,pPriority)	\
    ( (This)->lpVtbl -> GetGPUThreadPriority(This,pPriority) ) 


#define IDXGIDevice4_SetMaximumFrameLatency(This,MaxLatency)	\
    ( (This)->lpVtbl -> SetMaximumFrameLatency(This,MaxLatency) ) 

#define IDXGIDevice4_GetMaximumFrameLatency(This,pMaxLatency)	\
    ( (This)->lpVtbl -> GetMaximumFrameLatency(This,pMaxLatency) ) 


#define IDXGIDevice4_OfferResources(This,NumResources,ppResources,Priority)	\
    ( (This)->lpVtbl -> OfferResources(This,NumResources,ppResources,Priority) ) 

#define IDXGIDevice4_ReclaimResources(This,NumResources,ppResources,pDiscarded)	\
    ( (This)->lpVtbl -> ReclaimResources(This,NumResources,ppResources,pDiscarded) ) 

#define IDXGIDevice4_EnqueueSetEvent(This,hEvent)	\
    ( (This)->lpVtbl -> EnqueueSetEvent(This,hEvent) ) 


#define IDXGIDevice4_Trim(This)	\
    ( (This)->lpVtbl -> Trim(This) ) 


#define IDXGIDevice4_OfferResources1(This,NumResources,ppResources,Priority,Flags)	\
    ( (This)->lpVtbl -> OfferResources1(This,NumResources,ppResources,Priority,Flags) ) 

#define IDXGIDevice4_ReclaimResources1(This,NumResources,ppResources,pResults)	\
    ( (This)->lpVtbl -> ReclaimResources1(This,NumResources,ppResources,pResults) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */
#endif 	/* __IDXGIDevice4_INTERFACE_DEFINED__ */


extern RPC_IF_HANDLE __MIDL_itf_dxgi1_5_0000_0003_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_5_0000_0003_v0_0_s_ifspec;


/* interface __MIDL_itf_dxgi1_5_0000_0004 */
/* [local] */ 

/*#endif*/ /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
/*#pragma endregion*/
DEFINE_GUID(IID_IDXGIOutput5,0x80A07424,0xAB52,0x42EB,0x83,0x3C,0x0C,0x42,0xFD,0x28,0x2D,0x98);
DEFINE_GUID(IID_IDXGISwapChain4,0x3D585D5A,0xBD4A,0x489E,0xB1,0xF4,0x3D,0xBC,0xB6,0x45,0x2F,0xFB);
DEFINE_GUID(IID_IDXGIDevice4,0x95B4F95F,0xD8DA,0x4CA4,0x9E,0xE6,0x3B,0x76,0xD5,0x96,0x8A,0x10);
DEFINE_GUID(IID_IDXGIFactory5,0x7632e1f5,0xee65,0x4dca,0x87,0xfd,0x84,0xcd,0x75,0xf8,0x83,0x8d);


extern RPC_IF_HANDLE __MIDL_itf_dxgi1_5_0000_0004_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_5_0000_0004_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


//#include <d3d12.h>







/*-------------------------------------------------------------------------------------
*
* Copyright (c) Microsoft Corporation
*
*-------------------------------------------------------------------------------------*/


/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 8.01.0622 */



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

#ifndef __d3d12_h__
#define __d3d12_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ID3D12Object_FWD_DEFINED__
#define __ID3D12Object_FWD_DEFINED__
typedef interface ID3D12Object ID3D12Object;

#endif 	/* __ID3D12Object_FWD_DEFINED__ */


#ifndef __ID3D12DeviceChild_FWD_DEFINED__
#define __ID3D12DeviceChild_FWD_DEFINED__
typedef interface ID3D12DeviceChild ID3D12DeviceChild;

#endif 	/* __ID3D12DeviceChild_FWD_DEFINED__ */


#ifndef __ID3D12RootSignature_FWD_DEFINED__
#define __ID3D12RootSignature_FWD_DEFINED__
typedef interface ID3D12RootSignature ID3D12RootSignature;

#endif 	/* __ID3D12RootSignature_FWD_DEFINED__ */


#ifndef __ID3D12RootSignatureDeserializer_FWD_DEFINED__
#define __ID3D12RootSignatureDeserializer_FWD_DEFINED__
typedef interface ID3D12RootSignatureDeserializer ID3D12RootSignatureDeserializer;

#endif 	/* __ID3D12RootSignatureDeserializer_FWD_DEFINED__ */


#ifndef __ID3D12VersionedRootSignatureDeserializer_FWD_DEFINED__
#define __ID3D12VersionedRootSignatureDeserializer_FWD_DEFINED__
typedef interface ID3D12VersionedRootSignatureDeserializer ID3D12VersionedRootSignatureDeserializer;

#endif 	/* __ID3D12VersionedRootSignatureDeserializer_FWD_DEFINED__ */


#ifndef __ID3D12Pageable_FWD_DEFINED__
#define __ID3D12Pageable_FWD_DEFINED__
typedef interface ID3D12Pageable ID3D12Pageable;

#endif 	/* __ID3D12Pageable_FWD_DEFINED__ */


#ifndef __ID3D12Heap_FWD_DEFINED__
#define __ID3D12Heap_FWD_DEFINED__
typedef interface ID3D12Heap ID3D12Heap;

#endif 	/* __ID3D12Heap_FWD_DEFINED__ */


#ifndef __ID3D12Resource_FWD_DEFINED__
#define __ID3D12Resource_FWD_DEFINED__
typedef interface ID3D12Resource ID3D12Resource;

#endif 	/* __ID3D12Resource_FWD_DEFINED__ */


#ifndef __ID3D12CommandAllocator_FWD_DEFINED__
#define __ID3D12CommandAllocator_FWD_DEFINED__
typedef interface ID3D12CommandAllocator ID3D12CommandAllocator;

#endif 	/* __ID3D12CommandAllocator_FWD_DEFINED__ */


#ifndef __ID3D12Fence_FWD_DEFINED__
#define __ID3D12Fence_FWD_DEFINED__
typedef interface ID3D12Fence ID3D12Fence;

#endif 	/* __ID3D12Fence_FWD_DEFINED__ */


#ifndef __ID3D12Fence1_FWD_DEFINED__
#define __ID3D12Fence1_FWD_DEFINED__
typedef interface ID3D12Fence1 ID3D12Fence1;

#endif 	/* __ID3D12Fence1_FWD_DEFINED__ */


#ifndef __ID3D12PipelineState_FWD_DEFINED__
#define __ID3D12PipelineState_FWD_DEFINED__
typedef interface ID3D12PipelineState ID3D12PipelineState;

#endif 	/* __ID3D12PipelineState_FWD_DEFINED__ */


#ifndef __ID3D12DescriptorHeap_FWD_DEFINED__
#define __ID3D12DescriptorHeap_FWD_DEFINED__
typedef interface ID3D12DescriptorHeap ID3D12DescriptorHeap;

#endif 	/* __ID3D12DescriptorHeap_FWD_DEFINED__ */


#ifndef __ID3D12QueryHeap_FWD_DEFINED__
#define __ID3D12QueryHeap_FWD_DEFINED__
typedef interface ID3D12QueryHeap ID3D12QueryHeap;

#endif 	/* __ID3D12QueryHeap_FWD_DEFINED__ */


#ifndef __ID3D12CommandSignature_FWD_DEFINED__
#define __ID3D12CommandSignature_FWD_DEFINED__
typedef interface ID3D12CommandSignature ID3D12CommandSignature;

#endif 	/* __ID3D12CommandSignature_FWD_DEFINED__ */


#ifndef __ID3D12CommandList_FWD_DEFINED__
#define __ID3D12CommandList_FWD_DEFINED__
typedef interface ID3D12CommandList ID3D12CommandList;

#endif 	/* __ID3D12CommandList_FWD_DEFINED__ */


#ifndef __ID3D12GraphicsCommandList_FWD_DEFINED__
#define __ID3D12GraphicsCommandList_FWD_DEFINED__
typedef interface ID3D12GraphicsCommandList ID3D12GraphicsCommandList;

#endif 	/* __ID3D12GraphicsCommandList_FWD_DEFINED__ */


#ifndef __ID3D12GraphicsCommandList1_FWD_DEFINED__
#define __ID3D12GraphicsCommandList1_FWD_DEFINED__
typedef interface ID3D12GraphicsCommandList1 ID3D12GraphicsCommandList1;

#endif 	/* __ID3D12GraphicsCommandList1_FWD_DEFINED__ */


#ifndef __ID3D12GraphicsCommandList2_FWD_DEFINED__
#define __ID3D12GraphicsCommandList2_FWD_DEFINED__
typedef interface ID3D12GraphicsCommandList2 ID3D12GraphicsCommandList2;

#endif 	/* __ID3D12GraphicsCommandList2_FWD_DEFINED__ */


#ifndef __ID3D12CommandQueue_FWD_DEFINED__
#define __ID3D12CommandQueue_FWD_DEFINED__
typedef interface ID3D12CommandQueue ID3D12CommandQueue;

#endif 	/* __ID3D12CommandQueue_FWD_DEFINED__ */


#ifndef __ID3D12Device_FWD_DEFINED__
#define __ID3D12Device_FWD_DEFINED__
typedef interface ID3D12Device ID3D12Device;

#endif 	/* __ID3D12Device_FWD_DEFINED__ */


#ifndef __ID3D12PipelineLibrary_FWD_DEFINED__
#define __ID3D12PipelineLibrary_FWD_DEFINED__
typedef interface ID3D12PipelineLibrary ID3D12PipelineLibrary;

#endif 	/* __ID3D12PipelineLibrary_FWD_DEFINED__ */


#ifndef __ID3D12PipelineLibrary1_FWD_DEFINED__
#define __ID3D12PipelineLibrary1_FWD_DEFINED__
typedef interface ID3D12PipelineLibrary1 ID3D12PipelineLibrary1;

#endif 	/* __ID3D12PipelineLibrary1_FWD_DEFINED__ */


#ifndef __ID3D12Device1_FWD_DEFINED__
#define __ID3D12Device1_FWD_DEFINED__
typedef interface ID3D12Device1 ID3D12Device1;

#endif 	/* __ID3D12Device1_FWD_DEFINED__ */


#ifndef __ID3D12Device2_FWD_DEFINED__
#define __ID3D12Device2_FWD_DEFINED__
typedef interface ID3D12Device2 ID3D12Device2;

#endif 	/* __ID3D12Device2_FWD_DEFINED__ */


#ifndef __ID3D12Device3_FWD_DEFINED__
#define __ID3D12Device3_FWD_DEFINED__
typedef interface ID3D12Device3 ID3D12Device3;

#endif 	/* __ID3D12Device3_FWD_DEFINED__ */


#ifndef __ID3D12ProtectedSession_FWD_DEFINED__
#define __ID3D12ProtectedSession_FWD_DEFINED__
typedef interface ID3D12ProtectedSession ID3D12ProtectedSession;

#endif 	/* __ID3D12ProtectedSession_FWD_DEFINED__ */


#ifndef __ID3D12ProtectedResourceSession_FWD_DEFINED__
#define __ID3D12ProtectedResourceSession_FWD_DEFINED__
typedef interface ID3D12ProtectedResourceSession ID3D12ProtectedResourceSession;

#endif 	/* __ID3D12ProtectedResourceSession_FWD_DEFINED__ */


#ifndef __ID3D12Device4_FWD_DEFINED__
#define __ID3D12Device4_FWD_DEFINED__
typedef interface ID3D12Device4 ID3D12Device4;

#endif 	/* __ID3D12Device4_FWD_DEFINED__ */


#ifndef __ID3D12Resource1_FWD_DEFINED__
#define __ID3D12Resource1_FWD_DEFINED__
typedef interface ID3D12Resource1 ID3D12Resource1;

#endif 	/* __ID3D12Resource1_FWD_DEFINED__ */


#ifndef __ID3D12Heap1_FWD_DEFINED__
#define __ID3D12Heap1_FWD_DEFINED__
typedef interface ID3D12Heap1 ID3D12Heap1;

#endif 	/* __ID3D12Heap1_FWD_DEFINED__ */


#ifndef __ID3D12GraphicsCommandList3_FWD_DEFINED__
#define __ID3D12GraphicsCommandList3_FWD_DEFINED__
typedef interface ID3D12GraphicsCommandList3 ID3D12GraphicsCommandList3;

#endif 	/* __ID3D12GraphicsCommandList3_FWD_DEFINED__ */


#ifndef __ID3D12Tools_FWD_DEFINED__
#define __ID3D12Tools_FWD_DEFINED__
typedef interface ID3D12Tools ID3D12Tools;

#endif 	/* __ID3D12Tools_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "dxgiformat.h"
#include "d3dcommon.h"

#ifdef __cplusplus
extern "C"{
#endif 


  /* interface __MIDL_itf_d3d12_0000_0000 */
  /* [local] */ 

#include <winapifamily.h>
#pragma region App Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
#ifndef _D3D12_CONSTANTS
#define _D3D12_CONSTANTS
#define	D3D12_16BIT_INDEX_STRIP_CUT_VALUE	( 0xffff )

#define	D3D12_32BIT_INDEX_STRIP_CUT_VALUE	( 0xffffffff )

#define	D3D12_8BIT_INDEX_STRIP_CUT_VALUE	( 0xff )

#define	D3D12_APPEND_ALIGNED_ELEMENT	( 0xffffffff )

#define	D3D12_ARRAY_AXIS_ADDRESS_RANGE_BIT_COUNT	( 9 )

#define	D3D12_CLIP_OR_CULL_DISTANCE_COUNT	( 8 )

#define	D3D12_CLIP_OR_CULL_DISTANCE_ELEMENT_COUNT	( 2 )

#define	D3D12_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT	( 14 )

#define	D3D12_COMMONSHADER_CONSTANT_BUFFER_COMPONENTS	( 4 )

#define	D3D12_COMMONSHADER_CONSTANT_BUFFER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT	( 15 )

#define	D3D12_COMMONSHADER_CONSTANT_BUFFER_PARTIAL_UPDATE_EXTENTS_BYTE_ALIGNMENT	( 16 )

#define	D3D12_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COMPONENTS	( 4 )

#define	D3D12_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT	( 15 )

#define	D3D12_COMMONSHADER_CONSTANT_BUFFER_REGISTER_READS_PER_INST	( 1 )

#define	D3D12_COMMONSHADER_CONSTANT_BUFFER_REGISTER_READ_PORTS	( 1 )

#define	D3D12_COMMONSHADER_FLOWCONTROL_NESTING_LIMIT	( 64 )

#define	D3D12_COMMONSHADER_IMMEDIATE_CONSTANT_BUFFER_REGISTER_COMPONENTS	( 4 )

#define	D3D12_COMMONSHADER_IMMEDIATE_CONSTANT_BUFFER_REGISTER_COUNT	( 1 )

#define	D3D12_COMMONSHADER_IMMEDIATE_CONSTANT_BUFFER_REGISTER_READS_PER_INST	( 1 )

#define	D3D12_COMMONSHADER_IMMEDIATE_CONSTANT_BUFFER_REGISTER_READ_PORTS	( 1 )

#define	D3D12_COMMONSHADER_IMMEDIATE_VALUE_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_COMMONSHADER_INPUT_RESOURCE_REGISTER_COMPONENTS	( 1 )

#define	D3D12_COMMONSHADER_INPUT_RESOURCE_REGISTER_COUNT	( 128 )

#define	D3D12_COMMONSHADER_INPUT_RESOURCE_REGISTER_READS_PER_INST	( 1 )

#define	D3D12_COMMONSHADER_INPUT_RESOURCE_REGISTER_READ_PORTS	( 1 )

#define	D3D12_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT	( 128 )

#define	D3D12_COMMONSHADER_SAMPLER_REGISTER_COMPONENTS	( 1 )

#define	D3D12_COMMONSHADER_SAMPLER_REGISTER_COUNT	( 16 )

#define	D3D12_COMMONSHADER_SAMPLER_REGISTER_READS_PER_INST	( 1 )

#define	D3D12_COMMONSHADER_SAMPLER_REGISTER_READ_PORTS	( 1 )

#define	D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT	( 16 )

#define	D3D12_COMMONSHADER_SUBROUTINE_NESTING_LIMIT	( 32 )

#define	D3D12_COMMONSHADER_TEMP_REGISTER_COMPONENTS	( 4 )

#define	D3D12_COMMONSHADER_TEMP_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_COMMONSHADER_TEMP_REGISTER_COUNT	( 4096 )

#define	D3D12_COMMONSHADER_TEMP_REGISTER_READS_PER_INST	( 3 )

#define	D3D12_COMMONSHADER_TEMP_REGISTER_READ_PORTS	( 3 )

#define	D3D12_COMMONSHADER_TEXCOORD_RANGE_REDUCTION_MAX	( 10 )

#define	D3D12_COMMONSHADER_TEXCOORD_RANGE_REDUCTION_MIN	( -10 )

#define	D3D12_COMMONSHADER_TEXEL_OFFSET_MAX_NEGATIVE	( -8 )

#define	D3D12_COMMONSHADER_TEXEL_OFFSET_MAX_POSITIVE	( 7 )

#define	D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT	( 256 )

#define	D3D12_CS_4_X_BUCKET00_MAX_BYTES_TGSM_WRITABLE_PER_THREAD	( 256 )

#define	D3D12_CS_4_X_BUCKET00_MAX_NUM_THREADS_PER_GROUP	( 64 )

#define	D3D12_CS_4_X_BUCKET01_MAX_BYTES_TGSM_WRITABLE_PER_THREAD	( 240 )

#define	D3D12_CS_4_X_BUCKET01_MAX_NUM_THREADS_PER_GROUP	( 68 )

#define	D3D12_CS_4_X_BUCKET02_MAX_BYTES_TGSM_WRITABLE_PER_THREAD	( 224 )

#define	D3D12_CS_4_X_BUCKET02_MAX_NUM_THREADS_PER_GROUP	( 72 )

#define	D3D12_CS_4_X_BUCKET03_MAX_BYTES_TGSM_WRITABLE_PER_THREAD	( 208 )

#define	D3D12_CS_4_X_BUCKET03_MAX_NUM_THREADS_PER_GROUP	( 76 )

#define	D3D12_CS_4_X_BUCKET04_MAX_BYTES_TGSM_WRITABLE_PER_THREAD	( 192 )

#define	D3D12_CS_4_X_BUCKET04_MAX_NUM_THREADS_PER_GROUP	( 84 )

#define	D3D12_CS_4_X_BUCKET05_MAX_BYTES_TGSM_WRITABLE_PER_THREAD	( 176 )

#define	D3D12_CS_4_X_BUCKET05_MAX_NUM_THREADS_PER_GROUP	( 92 )

#define	D3D12_CS_4_X_BUCKET06_MAX_BYTES_TGSM_WRITABLE_PER_THREAD	( 160 )

#define	D3D12_CS_4_X_BUCKET06_MAX_NUM_THREADS_PER_GROUP	( 100 )

#define	D3D12_CS_4_X_BUCKET07_MAX_BYTES_TGSM_WRITABLE_PER_THREAD	( 144 )

#define	D3D12_CS_4_X_BUCKET07_MAX_NUM_THREADS_PER_GROUP	( 112 )

#define	D3D12_CS_4_X_BUCKET08_MAX_BYTES_TGSM_WRITABLE_PER_THREAD	( 128 )

#define	D3D12_CS_4_X_BUCKET08_MAX_NUM_THREADS_PER_GROUP	( 128 )

#define	D3D12_CS_4_X_BUCKET09_MAX_BYTES_TGSM_WRITABLE_PER_THREAD	( 112 )

#define	D3D12_CS_4_X_BUCKET09_MAX_NUM_THREADS_PER_GROUP	( 144 )

#define	D3D12_CS_4_X_BUCKET10_MAX_BYTES_TGSM_WRITABLE_PER_THREAD	( 96 )

#define	D3D12_CS_4_X_BUCKET10_MAX_NUM_THREADS_PER_GROUP	( 168 )

#define	D3D12_CS_4_X_BUCKET11_MAX_BYTES_TGSM_WRITABLE_PER_THREAD	( 80 )

#define	D3D12_CS_4_X_BUCKET11_MAX_NUM_THREADS_PER_GROUP	( 204 )

#define	D3D12_CS_4_X_BUCKET12_MAX_BYTES_TGSM_WRITABLE_PER_THREAD	( 64 )

#define	D3D12_CS_4_X_BUCKET12_MAX_NUM_THREADS_PER_GROUP	( 256 )

#define	D3D12_CS_4_X_BUCKET13_MAX_BYTES_TGSM_WRITABLE_PER_THREAD	( 48 )

#define	D3D12_CS_4_X_BUCKET13_MAX_NUM_THREADS_PER_GROUP	( 340 )

#define	D3D12_CS_4_X_BUCKET14_MAX_BYTES_TGSM_WRITABLE_PER_THREAD	( 32 )

#define	D3D12_CS_4_X_BUCKET14_MAX_NUM_THREADS_PER_GROUP	( 512 )

#define	D3D12_CS_4_X_BUCKET15_MAX_BYTES_TGSM_WRITABLE_PER_THREAD	( 16 )

#define	D3D12_CS_4_X_BUCKET15_MAX_NUM_THREADS_PER_GROUP	( 768 )

#define	D3D12_CS_4_X_DISPATCH_MAX_THREAD_GROUPS_IN_Z_DIMENSION	( 1 )

#define	D3D12_CS_4_X_RAW_UAV_BYTE_ALIGNMENT	( 256 )

#define	D3D12_CS_4_X_THREAD_GROUP_MAX_THREADS_PER_GROUP	( 768 )

#define	D3D12_CS_4_X_THREAD_GROUP_MAX_X	( 768 )

#define	D3D12_CS_4_X_THREAD_GROUP_MAX_Y	( 768 )

#define	D3D12_CS_4_X_UAV_REGISTER_COUNT	( 1 )

#define	D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION	( 65535 )

#define	D3D12_CS_TGSM_REGISTER_COUNT	( 8192 )

#define	D3D12_CS_TGSM_REGISTER_READS_PER_INST	( 1 )

#define	D3D12_CS_TGSM_RESOURCE_REGISTER_COMPONENTS	( 1 )

#define	D3D12_CS_TGSM_RESOURCE_REGISTER_READ_PORTS	( 1 )

#define	D3D12_CS_THREADGROUPID_REGISTER_COMPONENTS	( 3 )

#define	D3D12_CS_THREADGROUPID_REGISTER_COUNT	( 1 )

#define	D3D12_CS_THREADIDINGROUPFLATTENED_REGISTER_COMPONENTS	( 1 )

#define	D3D12_CS_THREADIDINGROUPFLATTENED_REGISTER_COUNT	( 1 )

#define	D3D12_CS_THREADIDINGROUP_REGISTER_COMPONENTS	( 3 )

#define	D3D12_CS_THREADIDINGROUP_REGISTER_COUNT	( 1 )

#define	D3D12_CS_THREADID_REGISTER_COMPONENTS	( 3 )

#define	D3D12_CS_THREADID_REGISTER_COUNT	( 1 )

#define	D3D12_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP	( 1024 )

#define	D3D12_CS_THREAD_GROUP_MAX_X	( 1024 )

#define	D3D12_CS_THREAD_GROUP_MAX_Y	( 1024 )

#define	D3D12_CS_THREAD_GROUP_MAX_Z	( 64 )

#define	D3D12_CS_THREAD_GROUP_MIN_X	( 1 )

#define	D3D12_CS_THREAD_GROUP_MIN_Y	( 1 )

#define	D3D12_CS_THREAD_GROUP_MIN_Z	( 1 )

#define	D3D12_CS_THREAD_LOCAL_TEMP_REGISTER_POOL	( 16384 )

#define D3D12_DEFAULT_BLEND_FACTOR_ALPHA	( 1.0f )
#define D3D12_DEFAULT_BLEND_FACTOR_BLUE	( 1.0f )
#define D3D12_DEFAULT_BLEND_FACTOR_GREEN	( 1.0f )
#define D3D12_DEFAULT_BLEND_FACTOR_RED	( 1.0f )
#define D3D12_DEFAULT_BORDER_COLOR_COMPONENT	( 0.0f )
#define	D3D12_DEFAULT_DEPTH_BIAS	( 0 )

#define D3D12_DEFAULT_DEPTH_BIAS_CLAMP	( 0.0f )
#define	D3D12_DEFAULT_MAX_ANISOTROPY	( 16 )

#define D3D12_DEFAULT_MIP_LOD_BIAS	( 0.0f )
#define	D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT	( 4194304 )

#define	D3D12_DEFAULT_RENDER_TARGET_ARRAY_INDEX	( 0 )

#define	D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT	( 65536 )

#define	D3D12_DEFAULT_SAMPLE_MASK	( 0xffffffff )

#define	D3D12_DEFAULT_SCISSOR_ENDX	( 0 )

#define	D3D12_DEFAULT_SCISSOR_ENDY	( 0 )

#define	D3D12_DEFAULT_SCISSOR_STARTX	( 0 )

#define	D3D12_DEFAULT_SCISSOR_STARTY	( 0 )

#define D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS	( 0.0f )
#define	D3D12_DEFAULT_STENCIL_READ_MASK	( 0xff )

#define	D3D12_DEFAULT_STENCIL_REFERENCE	( 0 )

#define	D3D12_DEFAULT_STENCIL_WRITE_MASK	( 0xff )

#define	D3D12_DEFAULT_VIEWPORT_AND_SCISSORRECT_INDEX	( 0 )

#define	D3D12_DEFAULT_VIEWPORT_HEIGHT	( 0 )

#define D3D12_DEFAULT_VIEWPORT_MAX_DEPTH	( 0.0f )
#define D3D12_DEFAULT_VIEWPORT_MIN_DEPTH	( 0.0f )
#define	D3D12_DEFAULT_VIEWPORT_TOPLEFTX	( 0 )

#define	D3D12_DEFAULT_VIEWPORT_TOPLEFTY	( 0 )

#define	D3D12_DEFAULT_VIEWPORT_WIDTH	( 0 )

#define	D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND	( 0xffffffff )

#define	D3D12_DRIVER_RESERVED_REGISTER_SPACE_VALUES_END	( 0xfffffff7 )

#define	D3D12_DRIVER_RESERVED_REGISTER_SPACE_VALUES_START	( 0xfffffff0 )

#define	D3D12_DS_INPUT_CONTROL_POINTS_MAX_TOTAL_SCALARS	( 3968 )

#define	D3D12_DS_INPUT_CONTROL_POINT_REGISTER_COMPONENTS	( 4 )

#define	D3D12_DS_INPUT_CONTROL_POINT_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_DS_INPUT_CONTROL_POINT_REGISTER_COUNT	( 32 )

#define	D3D12_DS_INPUT_CONTROL_POINT_REGISTER_READS_PER_INST	( 2 )

#define	D3D12_DS_INPUT_CONTROL_POINT_REGISTER_READ_PORTS	( 1 )

#define	D3D12_DS_INPUT_DOMAIN_POINT_REGISTER_COMPONENTS	( 3 )

#define	D3D12_DS_INPUT_DOMAIN_POINT_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_DS_INPUT_DOMAIN_POINT_REGISTER_COUNT	( 1 )

#define	D3D12_DS_INPUT_DOMAIN_POINT_REGISTER_READS_PER_INST	( 2 )

#define	D3D12_DS_INPUT_DOMAIN_POINT_REGISTER_READ_PORTS	( 1 )

#define	D3D12_DS_INPUT_PATCH_CONSTANT_REGISTER_COMPONENTS	( 4 )

#define	D3D12_DS_INPUT_PATCH_CONSTANT_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_DS_INPUT_PATCH_CONSTANT_REGISTER_COUNT	( 32 )

#define	D3D12_DS_INPUT_PATCH_CONSTANT_REGISTER_READS_PER_INST	( 2 )

#define	D3D12_DS_INPUT_PATCH_CONSTANT_REGISTER_READ_PORTS	( 1 )

#define	D3D12_DS_INPUT_PRIMITIVE_ID_REGISTER_COMPONENTS	( 1 )

#define	D3D12_DS_INPUT_PRIMITIVE_ID_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_DS_INPUT_PRIMITIVE_ID_REGISTER_COUNT	( 1 )

#define	D3D12_DS_INPUT_PRIMITIVE_ID_REGISTER_READS_PER_INST	( 2 )

#define	D3D12_DS_INPUT_PRIMITIVE_ID_REGISTER_READ_PORTS	( 1 )

#define	D3D12_DS_OUTPUT_REGISTER_COMPONENTS	( 4 )

#define	D3D12_DS_OUTPUT_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_DS_OUTPUT_REGISTER_COUNT	( 32 )

#define D3D12_FLOAT16_FUSED_TOLERANCE_IN_ULP	( 0.6 )
#define D3D12_FLOAT32_MAX	( 3.402823466e+38f )
#define D3D12_FLOAT32_TO_INTEGER_TOLERANCE_IN_ULP	( 0.6f )
#define D3D12_FLOAT_TO_SRGB_EXPONENT_DENOMINATOR	( 2.4f )
#define D3D12_FLOAT_TO_SRGB_EXPONENT_NUMERATOR	( 1.0f )
#define D3D12_FLOAT_TO_SRGB_OFFSET	( 0.055f )
#define D3D12_FLOAT_TO_SRGB_SCALE_1	( 12.92f )
#define D3D12_FLOAT_TO_SRGB_SCALE_2	( 1.055f )
#define D3D12_FLOAT_TO_SRGB_THRESHOLD	( 0.0031308f )
#define D3D12_FTOI_INSTRUCTION_MAX_INPUT	( 2147483647.999f )
#define D3D12_FTOI_INSTRUCTION_MIN_INPUT	( -2147483648.999f )
#define D3D12_FTOU_INSTRUCTION_MAX_INPUT	( 4294967295.999f )
#define D3D12_FTOU_INSTRUCTION_MIN_INPUT	( 0.0f )
#define	D3D12_GS_INPUT_INSTANCE_ID_READS_PER_INST	( 2 )

#define	D3D12_GS_INPUT_INSTANCE_ID_READ_PORTS	( 1 )

#define	D3D12_GS_INPUT_INSTANCE_ID_REGISTER_COMPONENTS	( 1 )

#define	D3D12_GS_INPUT_INSTANCE_ID_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_GS_INPUT_INSTANCE_ID_REGISTER_COUNT	( 1 )

#define	D3D12_GS_INPUT_PRIM_CONST_REGISTER_COMPONENTS	( 1 )

#define	D3D12_GS_INPUT_PRIM_CONST_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_GS_INPUT_PRIM_CONST_REGISTER_COUNT	( 1 )

#define	D3D12_GS_INPUT_PRIM_CONST_REGISTER_READS_PER_INST	( 2 )

#define	D3D12_GS_INPUT_PRIM_CONST_REGISTER_READ_PORTS	( 1 )

#define	D3D12_GS_INPUT_REGISTER_COMPONENTS	( 4 )

#define	D3D12_GS_INPUT_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_GS_INPUT_REGISTER_COUNT	( 32 )

#define	D3D12_GS_INPUT_REGISTER_READS_PER_INST	( 2 )

#define	D3D12_GS_INPUT_REGISTER_READ_PORTS	( 1 )

#define	D3D12_GS_INPUT_REGISTER_VERTICES	( 32 )

#define	D3D12_GS_MAX_INSTANCE_COUNT	( 32 )

#define	D3D12_GS_MAX_OUTPUT_VERTEX_COUNT_ACROSS_INSTANCES	( 1024 )

#define	D3D12_GS_OUTPUT_ELEMENTS	( 32 )

#define	D3D12_GS_OUTPUT_REGISTER_COMPONENTS	( 4 )

#define	D3D12_GS_OUTPUT_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_GS_OUTPUT_REGISTER_COUNT	( 32 )

#define	D3D12_HS_CONTROL_POINT_PHASE_INPUT_REGISTER_COUNT	( 32 )

#define	D3D12_HS_CONTROL_POINT_PHASE_OUTPUT_REGISTER_COUNT	( 32 )

#define	D3D12_HS_CONTROL_POINT_REGISTER_COMPONENTS	( 4 )

#define	D3D12_HS_CONTROL_POINT_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_HS_CONTROL_POINT_REGISTER_READS_PER_INST	( 2 )

#define	D3D12_HS_CONTROL_POINT_REGISTER_READ_PORTS	( 1 )

#define	D3D12_HS_FORK_PHASE_INSTANCE_COUNT_UPPER_BOUND	( 0xffffffff )

#define	D3D12_HS_INPUT_FORK_INSTANCE_ID_REGISTER_COMPONENTS	( 1 )

#define	D3D12_HS_INPUT_FORK_INSTANCE_ID_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_HS_INPUT_FORK_INSTANCE_ID_REGISTER_COUNT	( 1 )

#define	D3D12_HS_INPUT_FORK_INSTANCE_ID_REGISTER_READS_PER_INST	( 2 )

#define	D3D12_HS_INPUT_FORK_INSTANCE_ID_REGISTER_READ_PORTS	( 1 )

#define	D3D12_HS_INPUT_JOIN_INSTANCE_ID_REGISTER_COMPONENTS	( 1 )

#define	D3D12_HS_INPUT_JOIN_INSTANCE_ID_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_HS_INPUT_JOIN_INSTANCE_ID_REGISTER_COUNT	( 1 )

#define	D3D12_HS_INPUT_JOIN_INSTANCE_ID_REGISTER_READS_PER_INST	( 2 )

#define	D3D12_HS_INPUT_JOIN_INSTANCE_ID_REGISTER_READ_PORTS	( 1 )

#define	D3D12_HS_INPUT_PRIMITIVE_ID_REGISTER_COMPONENTS	( 1 )

#define	D3D12_HS_INPUT_PRIMITIVE_ID_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_HS_INPUT_PRIMITIVE_ID_REGISTER_COUNT	( 1 )

#define	D3D12_HS_INPUT_PRIMITIVE_ID_REGISTER_READS_PER_INST	( 2 )

#define	D3D12_HS_INPUT_PRIMITIVE_ID_REGISTER_READ_PORTS	( 1 )

#define	D3D12_HS_JOIN_PHASE_INSTANCE_COUNT_UPPER_BOUND	( 0xffffffff )

#define D3D12_HS_MAXTESSFACTOR_LOWER_BOUND	( 1.0f )
#define D3D12_HS_MAXTESSFACTOR_UPPER_BOUND	( 64.0f )
#define	D3D12_HS_OUTPUT_CONTROL_POINTS_MAX_TOTAL_SCALARS	( 3968 )

#define	D3D12_HS_OUTPUT_CONTROL_POINT_ID_REGISTER_COMPONENTS	( 1 )

#define	D3D12_HS_OUTPUT_CONTROL_POINT_ID_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_HS_OUTPUT_CONTROL_POINT_ID_REGISTER_COUNT	( 1 )

#define	D3D12_HS_OUTPUT_CONTROL_POINT_ID_REGISTER_READS_PER_INST	( 2 )

#define	D3D12_HS_OUTPUT_CONTROL_POINT_ID_REGISTER_READ_PORTS	( 1 )

#define	D3D12_HS_OUTPUT_PATCH_CONSTANT_REGISTER_COMPONENTS	( 4 )

#define	D3D12_HS_OUTPUT_PATCH_CONSTANT_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_HS_OUTPUT_PATCH_CONSTANT_REGISTER_COUNT	( 32 )

#define	D3D12_HS_OUTPUT_PATCH_CONSTANT_REGISTER_READS_PER_INST	( 2 )

#define	D3D12_HS_OUTPUT_PATCH_CONSTANT_REGISTER_READ_PORTS	( 1 )

#define	D3D12_HS_OUTPUT_PATCH_CONSTANT_REGISTER_SCALAR_COMPONENTS	( 128 )

#define	D3D12_IA_DEFAULT_INDEX_BUFFER_OFFSET_IN_BYTES	( 0 )

#define	D3D12_IA_DEFAULT_PRIMITIVE_TOPOLOGY	( 0 )

#define	D3D12_IA_DEFAULT_VERTEX_BUFFER_OFFSET_IN_BYTES	( 0 )

#define	D3D12_IA_INDEX_INPUT_RESOURCE_SLOT_COUNT	( 1 )

#define	D3D12_IA_INSTANCE_ID_BIT_COUNT	( 32 )

#define	D3D12_IA_INTEGER_ARITHMETIC_BIT_COUNT	( 32 )

#define	D3D12_IA_PATCH_MAX_CONTROL_POINT_COUNT	( 32 )

#define	D3D12_IA_PRIMITIVE_ID_BIT_COUNT	( 32 )

#define	D3D12_IA_VERTEX_ID_BIT_COUNT	( 32 )

#define	D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT	( 32 )

#define	D3D12_IA_VERTEX_INPUT_STRUCTURE_ELEMENTS_COMPONENTS	( 128 )

#define	D3D12_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT	( 32 )

#define	D3D12_INTEGER_DIVIDE_BY_ZERO_QUOTIENT	( 0xffffffff )

#define	D3D12_INTEGER_DIVIDE_BY_ZERO_REMAINDER	( 0xffffffff )

#define	D3D12_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL	( 0xffffffff )

#define	D3D12_KEEP_UNORDERED_ACCESS_VIEWS	( 0xffffffff )

#define D3D12_LINEAR_GAMMA	( 1.0f )
#define	D3D12_MAJOR_VERSION	( 12 )

#define D3D12_MAX_BORDER_COLOR_COMPONENT	( 1.0f )
#define D3D12_MAX_DEPTH	( 1.0f )
#define	D3D12_MAX_LIVE_STATIC_SAMPLERS	( 2032 )

#define	D3D12_MAX_MAXANISOTROPY	( 16 )

#define	D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT	( 32 )

#define D3D12_MAX_POSITION_VALUE	( 3.402823466e+34f )
#define	D3D12_MAX_ROOT_COST	( 64 )

#define	D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_1	( 1000000 )

#define	D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2	( 1000000 )

#define	D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE	( 2048 )

#define	D3D12_MAX_TEXTURE_DIMENSION_2_TO_EXP	( 17 )

#define	D3D12_MAX_VIEW_INSTANCE_COUNT	( 4 )

#define	D3D12_MINOR_VERSION	( 0 )

#define D3D12_MIN_BORDER_COLOR_COMPONENT	( 0.0f )
#define D3D12_MIN_DEPTH	( 0.0f )
#define	D3D12_MIN_MAXANISOTROPY	( 0 )

#define D3D12_MIP_LOD_BIAS_MAX	( 15.99f )
#define D3D12_MIP_LOD_BIAS_MIN	( -16.0f )
#define	D3D12_MIP_LOD_FRACTIONAL_BIT_COUNT	( 8 )

#define	D3D12_MIP_LOD_RANGE_BIT_COUNT	( 8 )

#define D3D12_MULTISAMPLE_ANTIALIAS_LINE_WIDTH	( 1.4f )
#define	D3D12_NONSAMPLE_FETCH_OUT_OF_RANGE_ACCESS_RESULT	( 0 )

#define	D3D12_OS_RESERVED_REGISTER_SPACE_VALUES_END	( 0xffffffff )

#define	D3D12_OS_RESERVED_REGISTER_SPACE_VALUES_START	( 0xfffffff8 )

#define	D3D12_PACKED_TILE	( 0xffffffff )

#define	D3D12_PIXEL_ADDRESS_RANGE_BIT_COUNT	( 15 )

#define	D3D12_PRE_SCISSOR_PIXEL_ADDRESS_RANGE_BIT_COUNT	( 16 )

#define	D3D12_PS_CS_UAV_REGISTER_COMPONENTS	( 1 )

#define	D3D12_PS_CS_UAV_REGISTER_COUNT	( 8 )

#define	D3D12_PS_CS_UAV_REGISTER_READS_PER_INST	( 1 )

#define	D3D12_PS_CS_UAV_REGISTER_READ_PORTS	( 1 )

#define	D3D12_PS_FRONTFACING_DEFAULT_VALUE	( 0xffffffff )

#define	D3D12_PS_FRONTFACING_FALSE_VALUE	( 0 )

#define	D3D12_PS_FRONTFACING_TRUE_VALUE	( 0xffffffff )

#define	D3D12_PS_INPUT_REGISTER_COMPONENTS	( 4 )

#define	D3D12_PS_INPUT_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_PS_INPUT_REGISTER_COUNT	( 32 )

#define	D3D12_PS_INPUT_REGISTER_READS_PER_INST	( 2 )

#define	D3D12_PS_INPUT_REGISTER_READ_PORTS	( 1 )

#define D3D12_PS_LEGACY_PIXEL_CENTER_FRACTIONAL_COMPONENT	( 0.0f )
#define	D3D12_PS_OUTPUT_DEPTH_REGISTER_COMPONENTS	( 1 )

#define	D3D12_PS_OUTPUT_DEPTH_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_PS_OUTPUT_DEPTH_REGISTER_COUNT	( 1 )

#define	D3D12_PS_OUTPUT_MASK_REGISTER_COMPONENTS	( 1 )

#define	D3D12_PS_OUTPUT_MASK_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_PS_OUTPUT_MASK_REGISTER_COUNT	( 1 )

#define	D3D12_PS_OUTPUT_REGISTER_COMPONENTS	( 4 )

#define	D3D12_PS_OUTPUT_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_PS_OUTPUT_REGISTER_COUNT	( 8 )

#define D3D12_PS_PIXEL_CENTER_FRACTIONAL_COMPONENT	( 0.5f )
#define	D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT	( 16 )

#define	D3D12_REQ_BLEND_OBJECT_COUNT_PER_DEVICE	( 4096 )

#define	D3D12_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP	( 27 )

#define	D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT	( 4096 )

#define	D3D12_REQ_DEPTH_STENCIL_OBJECT_COUNT_PER_DEVICE	( 4096 )

#define	D3D12_REQ_DRAWINDEXED_INDEX_COUNT_2_TO_EXP	( 32 )

#define	D3D12_REQ_DRAW_VERTEX_COUNT_2_TO_EXP	( 32 )

#define	D3D12_REQ_FILTERING_HW_ADDRESSABLE_RESOURCE_DIMENSION	( 16384 )

#define	D3D12_REQ_GS_INVOCATION_32BIT_OUTPUT_COMPONENT_LIMIT	( 1024 )

#define	D3D12_REQ_IMMEDIATE_CONSTANT_BUFFER_ELEMENT_COUNT	( 4096 )

#define	D3D12_REQ_MAXANISOTROPY	( 16 )

#define	D3D12_REQ_MIP_LEVELS	( 15 )

#define	D3D12_REQ_MULTI_ELEMENT_STRUCTURE_SIZE_IN_BYTES	( 2048 )

#define	D3D12_REQ_RASTERIZER_OBJECT_COUNT_PER_DEVICE	( 4096 )

#define	D3D12_REQ_RENDER_TO_BUFFER_WINDOW_WIDTH	( 16384 )

#define	D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM	( 128 )

#define D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_B_TERM	( 0.25f )
#define	D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_C_TERM	( 2048 )

#define	D3D12_REQ_RESOURCE_VIEW_COUNT_PER_DEVICE_2_TO_EXP	( 20 )

#define	D3D12_REQ_SAMPLER_OBJECT_COUNT_PER_DEVICE	( 4096 )

#define	D3D12_REQ_SUBRESOURCES	( 30720 )

#define	D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION	( 2048 )

#define	D3D12_REQ_TEXTURE1D_U_DIMENSION	( 16384 )

#define	D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION	( 2048 )

#define	D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION	( 16384 )

#define	D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION	( 2048 )

#define	D3D12_REQ_TEXTURECUBE_DIMENSION	( 16384 )

#define	D3D12_RESINFO_INSTRUCTION_MISSING_COMPONENT_RETVAL	( 0 )

#define	D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES	( 0xffffffff )

#define	D3D12_SHADER_MAJOR_VERSION	( 5 )

#define	D3D12_SHADER_MAX_INSTANCES	( 65535 )

#define	D3D12_SHADER_MAX_INTERFACES	( 253 )

#define	D3D12_SHADER_MAX_INTERFACE_CALL_SITES	( 4096 )

#define	D3D12_SHADER_MAX_TYPES	( 65535 )

#define	D3D12_SHADER_MINOR_VERSION	( 1 )

#define	D3D12_SHIFT_INSTRUCTION_PAD_VALUE	( 0 )

#define	D3D12_SHIFT_INSTRUCTION_SHIFT_VALUE_BIT_COUNT	( 5 )

#define	D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT	( 8 )

#define	D3D12_SMALL_MSAA_RESOURCE_PLACEMENT_ALIGNMENT	( 65536 )

#define	D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT	( 4096 )

#define	D3D12_SO_BUFFER_MAX_STRIDE_IN_BYTES	( 2048 )

#define	D3D12_SO_BUFFER_MAX_WRITE_WINDOW_IN_BYTES	( 512 )

#define	D3D12_SO_BUFFER_SLOT_COUNT	( 4 )

#define	D3D12_SO_DDI_REGISTER_INDEX_DENOTING_GAP	( 0xffffffff )

#define	D3D12_SO_NO_RASTERIZED_STREAM	( 0xffffffff )

#define	D3D12_SO_OUTPUT_COMPONENT_COUNT	( 128 )

#define	D3D12_SO_STREAM_COUNT	( 4 )

#define	D3D12_SPEC_DATE_DAY	( 14 )

#define	D3D12_SPEC_DATE_MONTH	( 11 )

#define	D3D12_SPEC_DATE_YEAR	( 2014 )

#define D3D12_SPEC_VERSION	( 1.16 )
#define D3D12_SRGB_GAMMA	( 2.2f )
#define D3D12_SRGB_TO_FLOAT_DENOMINATOR_1	( 12.92f )
#define D3D12_SRGB_TO_FLOAT_DENOMINATOR_2	( 1.055f )
#define D3D12_SRGB_TO_FLOAT_EXPONENT	( 2.4f )
#define D3D12_SRGB_TO_FLOAT_OFFSET	( 0.055f )
#define D3D12_SRGB_TO_FLOAT_THRESHOLD	( 0.04045f )
#define D3D12_SRGB_TO_FLOAT_TOLERANCE_IN_ULP	( 0.5f )
#define	D3D12_STANDARD_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_STANDARD_COMPONENT_BIT_COUNT_DOUBLED	( 64 )

#define	D3D12_STANDARD_MAXIMUM_ELEMENT_ALIGNMENT_BYTE_MULTIPLE	( 4 )

#define	D3D12_STANDARD_PIXEL_COMPONENT_COUNT	( 128 )

#define	D3D12_STANDARD_PIXEL_ELEMENT_COUNT	( 32 )

#define	D3D12_STANDARD_VECTOR_SIZE	( 4 )

#define	D3D12_STANDARD_VERTEX_ELEMENT_COUNT	( 32 )

#define	D3D12_STANDARD_VERTEX_TOTAL_COMPONENT_COUNT	( 64 )

#define	D3D12_SUBPIXEL_FRACTIONAL_BIT_COUNT	( 8 )

#define	D3D12_SUBTEXEL_FRACTIONAL_BIT_COUNT	( 8 )

#define	D3D12_SYSTEM_RESERVED_REGISTER_SPACE_VALUES_END	( 0xffffffff )

#define	D3D12_SYSTEM_RESERVED_REGISTER_SPACE_VALUES_START	( 0xfffffff0 )

#define	D3D12_TESSELLATOR_MAX_EVEN_TESSELLATION_FACTOR	( 64 )

#define	D3D12_TESSELLATOR_MAX_ISOLINE_DENSITY_TESSELLATION_FACTOR	( 64 )

#define	D3D12_TESSELLATOR_MAX_ODD_TESSELLATION_FACTOR	( 63 )

#define	D3D12_TESSELLATOR_MAX_TESSELLATION_FACTOR	( 64 )

#define	D3D12_TESSELLATOR_MIN_EVEN_TESSELLATION_FACTOR	( 2 )

#define	D3D12_TESSELLATOR_MIN_ISOLINE_DENSITY_TESSELLATION_FACTOR	( 1 )

#define	D3D12_TESSELLATOR_MIN_ODD_TESSELLATION_FACTOR	( 1 )

#define	D3D12_TEXEL_ADDRESS_RANGE_BIT_COUNT	( 16 )

#define	D3D12_TEXTURE_DATA_PITCH_ALIGNMENT	( 256 )

#define	D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT	( 512 )

#define	D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES	( 65536 )

#define	D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT	( 4096 )

#define	D3D12_UAV_SLOT_COUNT	( 64 )

#define	D3D12_UNBOUND_MEMORY_ACCESS_RESULT	( 0 )

#define	D3D12_VIDEO_DECODE_MAX_ARGUMENTS	( 10 )

#define	D3D12_VIDEO_DECODE_MAX_HISTOGRAM_COMPONENTS	( 4 )

#define	D3D12_VIDEO_DECODE_MIN_BITSTREAM_OFFSET_ALIGNMENT	( 256 )

#define	D3D12_VIDEO_DECODE_MIN_HISTOGRAM_OFFSET_ALIGNMENT	( 256 )

#define	D3D12_VIDEO_DECODE_STATUS_MACROBLOCKS_AFFECTED_UNKNOWN	( 0xffffffff )

#define	D3D12_VIDEO_PROCESS_MAX_FILTERS	( 32 )

#define	D3D12_VIDEO_PROCESS_STEREO_VIEWS	( 2 )

#define	D3D12_VIEWPORT_AND_SCISSORRECT_MAX_INDEX	( 15 )

#define	D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE	( 16 )

#define	D3D12_VIEWPORT_BOUNDS_MAX	( 32767 )

#define	D3D12_VIEWPORT_BOUNDS_MIN	( -32768 )

#define	D3D12_VS_INPUT_REGISTER_COMPONENTS	( 4 )

#define	D3D12_VS_INPUT_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_VS_INPUT_REGISTER_COUNT	( 32 )

#define	D3D12_VS_INPUT_REGISTER_READS_PER_INST	( 2 )

#define	D3D12_VS_INPUT_REGISTER_READ_PORTS	( 1 )

#define	D3D12_VS_OUTPUT_REGISTER_COMPONENTS	( 4 )

#define	D3D12_VS_OUTPUT_REGISTER_COMPONENT_BIT_COUNT	( 32 )

#define	D3D12_VS_OUTPUT_REGISTER_COUNT	( 32 )

#define	D3D12_WHQL_CONTEXT_COUNT_FOR_RESOURCE_LIMIT	( 10 )

#define	D3D12_WHQL_DRAWINDEXED_INDEX_COUNT_2_TO_EXP	( 25 )

#define	D3D12_WHQL_DRAW_VERTEX_COUNT_2_TO_EXP	( 25 )

#endif

  typedef UINT64 D3D12_GPU_VIRTUAL_ADDRESS;

  typedef 
    enum D3D12_COMMAND_LIST_TYPE
  {
    D3D12_COMMAND_LIST_TYPE_DIRECT	= 0,
    D3D12_COMMAND_LIST_TYPE_BUNDLE	= 1,
    D3D12_COMMAND_LIST_TYPE_COMPUTE	= 2,
    D3D12_COMMAND_LIST_TYPE_COPY	= 3,
    D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE	= 4,
    D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS	= 5
  } 	D3D12_COMMAND_LIST_TYPE;

  typedef 
    enum D3D12_COMMAND_QUEUE_FLAGS
  {
    D3D12_COMMAND_QUEUE_FLAG_NONE	= 0,
    D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT	= 0x1
  } 	D3D12_COMMAND_QUEUE_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_COMMAND_QUEUE_FLAGS );
  typedef 
    enum D3D12_COMMAND_QUEUE_PRIORITY
  {
    D3D12_COMMAND_QUEUE_PRIORITY_NORMAL	= 0,
    D3D12_COMMAND_QUEUE_PRIORITY_HIGH	= 100,
    D3D12_COMMAND_QUEUE_PRIORITY_GLOBAL_REALTIME	= 10000
  } 	D3D12_COMMAND_QUEUE_PRIORITY;

  typedef struct D3D12_COMMAND_QUEUE_DESC
  {
    D3D12_COMMAND_LIST_TYPE Type;
    INT Priority;
    D3D12_COMMAND_QUEUE_FLAGS Flags;
    UINT NodeMask;
  } 	D3D12_COMMAND_QUEUE_DESC;

  typedef 
    enum D3D12_PRIMITIVE_TOPOLOGY_TYPE
  {
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED	= 0,
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT	= 1,
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE	= 2,
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE	= 3,
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH	= 4
  } 	D3D12_PRIMITIVE_TOPOLOGY_TYPE;

  typedef 
    enum D3D12_INPUT_CLASSIFICATION
  {
    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA	= 0,
    D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA	= 1
  } 	D3D12_INPUT_CLASSIFICATION;

  typedef struct D3D12_INPUT_ELEMENT_DESC
  {
    LPCSTR SemanticName;
    UINT SemanticIndex;
    DXGI_FORMAT Format;
    UINT InputSlot;
    UINT AlignedByteOffset;
    D3D12_INPUT_CLASSIFICATION InputSlotClass;
    UINT InstanceDataStepRate;
  } 	D3D12_INPUT_ELEMENT_DESC;

  typedef 
    enum D3D12_FILL_MODE
  {
    D3D12_FILL_MODE_WIREFRAME	= 2,
    D3D12_FILL_MODE_SOLID	= 3
  } 	D3D12_FILL_MODE;

  typedef D3D_PRIMITIVE_TOPOLOGY D3D12_PRIMITIVE_TOPOLOGY;

  typedef D3D_PRIMITIVE D3D12_PRIMITIVE;

  typedef 
    enum D3D12_CULL_MODE
  {
    D3D12_CULL_MODE_NONE	= 1,
    D3D12_CULL_MODE_FRONT	= 2,
    D3D12_CULL_MODE_BACK	= 3
  } 	D3D12_CULL_MODE;

  typedef struct D3D12_SO_DECLARATION_ENTRY
  {
    UINT Stream;
    LPCSTR SemanticName;
    UINT SemanticIndex;
    BYTE StartComponent;
    BYTE ComponentCount;
    BYTE OutputSlot;
  } 	D3D12_SO_DECLARATION_ENTRY;

  typedef struct D3D12_VIEWPORT
  {
    FLOAT TopLeftX;
    FLOAT TopLeftY;
    FLOAT Width;
    FLOAT Height;
    FLOAT MinDepth;
    FLOAT MaxDepth;
  } 	D3D12_VIEWPORT;

  typedef RECT D3D12_RECT;

  typedef struct D3D12_BOX
  {
    UINT left;
    UINT top;
    UINT front;
    UINT right;
    UINT bottom;
    UINT back;
  } 	D3D12_BOX;

  typedef 
    enum D3D12_COMPARISON_FUNC
  {
    D3D12_COMPARISON_FUNC_NEVER	= 1,
    D3D12_COMPARISON_FUNC_LESS	= 2,
    D3D12_COMPARISON_FUNC_EQUAL	= 3,
    D3D12_COMPARISON_FUNC_LESS_EQUAL	= 4,
    D3D12_COMPARISON_FUNC_GREATER	= 5,
    D3D12_COMPARISON_FUNC_NOT_EQUAL	= 6,
    D3D12_COMPARISON_FUNC_GREATER_EQUAL	= 7,
    D3D12_COMPARISON_FUNC_ALWAYS	= 8
  } 	D3D12_COMPARISON_FUNC;

  typedef 
    enum D3D12_DEPTH_WRITE_MASK
  {
    D3D12_DEPTH_WRITE_MASK_ZERO	= 0,
    D3D12_DEPTH_WRITE_MASK_ALL	= 1
  } 	D3D12_DEPTH_WRITE_MASK;

  typedef 
    enum D3D12_STENCIL_OP
  {
    D3D12_STENCIL_OP_KEEP	= 1,
    D3D12_STENCIL_OP_ZERO	= 2,
    D3D12_STENCIL_OP_REPLACE	= 3,
    D3D12_STENCIL_OP_INCR_SAT	= 4,
    D3D12_STENCIL_OP_DECR_SAT	= 5,
    D3D12_STENCIL_OP_INVERT	= 6,
    D3D12_STENCIL_OP_INCR	= 7,
    D3D12_STENCIL_OP_DECR	= 8
  } 	D3D12_STENCIL_OP;

  typedef struct D3D12_DEPTH_STENCILOP_DESC
  {
    D3D12_STENCIL_OP StencilFailOp;
    D3D12_STENCIL_OP StencilDepthFailOp;
    D3D12_STENCIL_OP StencilPassOp;
    D3D12_COMPARISON_FUNC StencilFunc;
  } 	D3D12_DEPTH_STENCILOP_DESC;

  typedef struct D3D12_DEPTH_STENCIL_DESC
  {
    BOOL DepthEnable;
    D3D12_DEPTH_WRITE_MASK DepthWriteMask;
    D3D12_COMPARISON_FUNC DepthFunc;
    BOOL StencilEnable;
    UINT8 StencilReadMask;
    UINT8 StencilWriteMask;
    D3D12_DEPTH_STENCILOP_DESC FrontFace;
    D3D12_DEPTH_STENCILOP_DESC BackFace;
  } 	D3D12_DEPTH_STENCIL_DESC;

  typedef struct D3D12_DEPTH_STENCIL_DESC1
  {
    BOOL DepthEnable;
    D3D12_DEPTH_WRITE_MASK DepthWriteMask;
    D3D12_COMPARISON_FUNC DepthFunc;
    BOOL StencilEnable;
    UINT8 StencilReadMask;
    UINT8 StencilWriteMask;
    D3D12_DEPTH_STENCILOP_DESC FrontFace;
    D3D12_DEPTH_STENCILOP_DESC BackFace;
    BOOL DepthBoundsTestEnable;
  } 	D3D12_DEPTH_STENCIL_DESC1;

  typedef 
    enum D3D12_BLEND
  {
    D3D12_BLEND_ZERO	= 1,
    D3D12_BLEND_ONE	= 2,
    D3D12_BLEND_SRC_COLOR	= 3,
    D3D12_BLEND_INV_SRC_COLOR	= 4,
    D3D12_BLEND_SRC_ALPHA	= 5,
    D3D12_BLEND_INV_SRC_ALPHA	= 6,
    D3D12_BLEND_DEST_ALPHA	= 7,
    D3D12_BLEND_INV_DEST_ALPHA	= 8,
    D3D12_BLEND_DEST_COLOR	= 9,
    D3D12_BLEND_INV_DEST_COLOR	= 10,
    D3D12_BLEND_SRC_ALPHA_SAT	= 11,
    D3D12_BLEND_BLEND_FACTOR	= 14,
    D3D12_BLEND_INV_BLEND_FACTOR	= 15,
    D3D12_BLEND_SRC1_COLOR	= 16,
    D3D12_BLEND_INV_SRC1_COLOR	= 17,
    D3D12_BLEND_SRC1_ALPHA	= 18,
    D3D12_BLEND_INV_SRC1_ALPHA	= 19
  } 	D3D12_BLEND;

  typedef 
    enum D3D12_BLEND_OP
  {
    D3D12_BLEND_OP_ADD	= 1,
    D3D12_BLEND_OP_SUBTRACT	= 2,
    D3D12_BLEND_OP_REV_SUBTRACT	= 3,
    D3D12_BLEND_OP_MIN	= 4,
    D3D12_BLEND_OP_MAX	= 5
  } 	D3D12_BLEND_OP;

  typedef 
    enum D3D12_COLOR_WRITE_ENABLE
  {
    D3D12_COLOR_WRITE_ENABLE_RED	= 1,
    D3D12_COLOR_WRITE_ENABLE_GREEN	= 2,
    D3D12_COLOR_WRITE_ENABLE_BLUE	= 4,
    D3D12_COLOR_WRITE_ENABLE_ALPHA	= 8,
    D3D12_COLOR_WRITE_ENABLE_ALL	= ( ( ( D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN )  | D3D12_COLOR_WRITE_ENABLE_BLUE )  | D3D12_COLOR_WRITE_ENABLE_ALPHA ) 
  } 	D3D12_COLOR_WRITE_ENABLE;

  typedef 
    enum D3D12_LOGIC_OP
  {
    D3D12_LOGIC_OP_CLEAR	= 0,
    D3D12_LOGIC_OP_SET	= ( D3D12_LOGIC_OP_CLEAR + 1 ) ,
    D3D12_LOGIC_OP_COPY	= ( D3D12_LOGIC_OP_SET + 1 ) ,
    D3D12_LOGIC_OP_COPY_INVERTED	= ( D3D12_LOGIC_OP_COPY + 1 ) ,
    D3D12_LOGIC_OP_NOOP	= ( D3D12_LOGIC_OP_COPY_INVERTED + 1 ) ,
    D3D12_LOGIC_OP_INVERT	= ( D3D12_LOGIC_OP_NOOP + 1 ) ,
    D3D12_LOGIC_OP_AND	= ( D3D12_LOGIC_OP_INVERT + 1 ) ,
    D3D12_LOGIC_OP_NAND	= ( D3D12_LOGIC_OP_AND + 1 ) ,
    D3D12_LOGIC_OP_OR	= ( D3D12_LOGIC_OP_NAND + 1 ) ,
    D3D12_LOGIC_OP_NOR	= ( D3D12_LOGIC_OP_OR + 1 ) ,
    D3D12_LOGIC_OP_XOR	= ( D3D12_LOGIC_OP_NOR + 1 ) ,
    D3D12_LOGIC_OP_EQUIV	= ( D3D12_LOGIC_OP_XOR + 1 ) ,
    D3D12_LOGIC_OP_AND_REVERSE	= ( D3D12_LOGIC_OP_EQUIV + 1 ) ,
    D3D12_LOGIC_OP_AND_INVERTED	= ( D3D12_LOGIC_OP_AND_REVERSE + 1 ) ,
    D3D12_LOGIC_OP_OR_REVERSE	= ( D3D12_LOGIC_OP_AND_INVERTED + 1 ) ,
    D3D12_LOGIC_OP_OR_INVERTED	= ( D3D12_LOGIC_OP_OR_REVERSE + 1 ) 
  } 	D3D12_LOGIC_OP;

  typedef struct D3D12_RENDER_TARGET_BLEND_DESC
  {
    BOOL BlendEnable;
    BOOL LogicOpEnable;
    D3D12_BLEND SrcBlend;
    D3D12_BLEND DestBlend;
    D3D12_BLEND_OP BlendOp;
    D3D12_BLEND SrcBlendAlpha;
    D3D12_BLEND DestBlendAlpha;
    D3D12_BLEND_OP BlendOpAlpha;
    D3D12_LOGIC_OP LogicOp;
    UINT8 RenderTargetWriteMask;
  } 	D3D12_RENDER_TARGET_BLEND_DESC;

  typedef struct D3D12_BLEND_DESC
  {
    BOOL AlphaToCoverageEnable;
    BOOL IndependentBlendEnable;
    D3D12_RENDER_TARGET_BLEND_DESC RenderTarget[ 8 ];
  } 	D3D12_BLEND_DESC;

  /* Note, the array size for RenderTarget[] above is D3D12_SIMULTANEOUS_RENDERTARGET_COUNT. 
  IDL processing/generation of this header replaces the define; this comment is merely explaining what happened. */
  typedef 
    enum D3D12_CONSERVATIVE_RASTERIZATION_MODE
  {
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF	= 0,
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON	= 1
  } 	D3D12_CONSERVATIVE_RASTERIZATION_MODE;

  typedef struct D3D12_RASTERIZER_DESC
  {
    D3D12_FILL_MODE FillMode;
    D3D12_CULL_MODE CullMode;
    BOOL FrontCounterClockwise;
    INT DepthBias;
    FLOAT DepthBiasClamp;
    FLOAT SlopeScaledDepthBias;
    BOOL DepthClipEnable;
    BOOL MultisampleEnable;
    BOOL AntialiasedLineEnable;
    UINT ForcedSampleCount;
    D3D12_CONSERVATIVE_RASTERIZATION_MODE ConservativeRaster;
  } 	D3D12_RASTERIZER_DESC;



  extern RPC_IF_HANDLE __MIDL_itf_d3d12_0000_0000_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_d3d12_0000_0000_v0_0_s_ifspec;

#ifndef __ID3D12Object_INTERFACE_DEFINED__
#define __ID3D12Object_INTERFACE_DEFINED__

  /* interface ID3D12Object */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12Object;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("c4fec28f-7966-4e95-9f94-f431cb56c3b8")
    ID3D12Object : public IUnknown
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetPrivateData( 
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetPrivateData( 
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface( 
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetName( 
      _In_z_  LPCWSTR Name) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12ObjectVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12Object * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12Object * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12Object * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12Object * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12Object * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12Object * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12Object * This,
      _In_z_  LPCWSTR Name);

    END_INTERFACE
  } ID3D12ObjectVtbl;

  interface ID3D12Object
  {
    CONST_VTBL struct ID3D12ObjectVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12Object_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12Object_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12Object_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12Object_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12Object_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12Object_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12Object_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12Object_INTERFACE_DEFINED__ */


#ifndef __ID3D12DeviceChild_INTERFACE_DEFINED__
#define __ID3D12DeviceChild_INTERFACE_DEFINED__

  /* interface ID3D12DeviceChild */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12DeviceChild;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("905db94b-a00c-4140-9df5-2b64ca9ea357")
    ID3D12DeviceChild : public ID3D12Object
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetDevice( 
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12DeviceChildVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12DeviceChild * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12DeviceChild * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12DeviceChild * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12DeviceChild * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12DeviceChild * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12DeviceChild * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12DeviceChild * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12DeviceChild * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    END_INTERFACE
  } ID3D12DeviceChildVtbl;

  interface ID3D12DeviceChild
  {
    CONST_VTBL struct ID3D12DeviceChildVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12DeviceChild_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12DeviceChild_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12DeviceChild_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12DeviceChild_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12DeviceChild_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12DeviceChild_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12DeviceChild_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12DeviceChild_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12DeviceChild_INTERFACE_DEFINED__ */


#ifndef __ID3D12RootSignature_INTERFACE_DEFINED__
#define __ID3D12RootSignature_INTERFACE_DEFINED__

  /* interface ID3D12RootSignature */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12RootSignature;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("c54a6b66-72df-4ee8-8be5-a946a1429214")
    ID3D12RootSignature : public ID3D12DeviceChild
  {
  public:
  };


#else 	/* C style interface */

  typedef struct ID3D12RootSignatureVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12RootSignature * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12RootSignature * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12RootSignature * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12RootSignature * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12RootSignature * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12RootSignature * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12RootSignature * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12RootSignature * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    END_INTERFACE
  } ID3D12RootSignatureVtbl;

  interface ID3D12RootSignature
  {
    CONST_VTBL struct ID3D12RootSignatureVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12RootSignature_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12RootSignature_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12RootSignature_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12RootSignature_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12RootSignature_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12RootSignature_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12RootSignature_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12RootSignature_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12RootSignature_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_d3d12_0000_0001 */
  /* [local] */ 

  typedef struct D3D12_SHADER_BYTECODE
  {
    _Field_size_bytes_full_(BytecodeLength)  const void *pShaderBytecode;
    SIZE_T BytecodeLength;
  } 	D3D12_SHADER_BYTECODE;

  typedef struct D3D12_STREAM_OUTPUT_DESC
  {
    _Field_size_full_(NumEntries)  const D3D12_SO_DECLARATION_ENTRY *pSODeclaration;
    UINT NumEntries;
    _Field_size_full_(NumStrides)  const UINT *pBufferStrides;
    UINT NumStrides;
    UINT RasterizedStream;
  } 	D3D12_STREAM_OUTPUT_DESC;

  typedef struct D3D12_INPUT_LAYOUT_DESC
  {
    _Field_size_full_(NumElements)  const D3D12_INPUT_ELEMENT_DESC *pInputElementDescs;
    UINT NumElements;
  } 	D3D12_INPUT_LAYOUT_DESC;

  typedef 
    enum D3D12_INDEX_BUFFER_STRIP_CUT_VALUE
  {
    D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED	= 0,
    D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF	= 1,
    D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF	= 2
  } 	D3D12_INDEX_BUFFER_STRIP_CUT_VALUE;

  typedef struct D3D12_CACHED_PIPELINE_STATE
  {
    _Field_size_bytes_full_(CachedBlobSizeInBytes)  const void *pCachedBlob;
    SIZE_T CachedBlobSizeInBytes;
  } 	D3D12_CACHED_PIPELINE_STATE;

  typedef 
    enum D3D12_PIPELINE_STATE_FLAGS
  {
    D3D12_PIPELINE_STATE_FLAG_NONE	= 0,
    D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG	= 0x1
  } 	D3D12_PIPELINE_STATE_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_PIPELINE_STATE_FLAGS );
  typedef struct D3D12_GRAPHICS_PIPELINE_STATE_DESC
  {
    ID3D12RootSignature *pRootSignature;
    D3D12_SHADER_BYTECODE VS;
    D3D12_SHADER_BYTECODE PS;
    D3D12_SHADER_BYTECODE DS;
    D3D12_SHADER_BYTECODE HS;
    D3D12_SHADER_BYTECODE GS;
    D3D12_STREAM_OUTPUT_DESC StreamOutput;
    D3D12_BLEND_DESC BlendState;
    UINT SampleMask;
    D3D12_RASTERIZER_DESC RasterizerState;
    D3D12_DEPTH_STENCIL_DESC DepthStencilState;
    D3D12_INPUT_LAYOUT_DESC InputLayout;
    D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
    D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
    UINT NumRenderTargets;
    DXGI_FORMAT RTVFormats[ 8 ];
    DXGI_FORMAT DSVFormat;
    DXGI_SAMPLE_DESC SampleDesc;
    UINT NodeMask;
    D3D12_CACHED_PIPELINE_STATE CachedPSO;
    D3D12_PIPELINE_STATE_FLAGS Flags;
  } 	D3D12_GRAPHICS_PIPELINE_STATE_DESC;

  typedef struct D3D12_COMPUTE_PIPELINE_STATE_DESC
  {
    ID3D12RootSignature *pRootSignature;
    D3D12_SHADER_BYTECODE CS;
    UINT NodeMask;
    D3D12_CACHED_PIPELINE_STATE CachedPSO;
    D3D12_PIPELINE_STATE_FLAGS Flags;
  } 	D3D12_COMPUTE_PIPELINE_STATE_DESC;

  struct D3D12_RT_FORMAT_ARRAY
  {
    DXGI_FORMAT RTFormats[ 8 ];
    UINT NumRenderTargets;
  } ;
  typedef struct D3D12_PIPELINE_STATE_STREAM_DESC
  {
    _In_  SIZE_T SizeInBytes;
    _In_reads_(_Inexpressible_("Dependent on size of subobjects"))  void *pPipelineStateSubobjectStream;
  } 	D3D12_PIPELINE_STATE_STREAM_DESC;

  typedef 
    enum D3D12_PIPELINE_STATE_SUBOBJECT_TYPE
  {
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE	= 0,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1 + 1 ) ,
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MAX_VALID	= ( D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING + 1 ) 
  } 	D3D12_PIPELINE_STATE_SUBOBJECT_TYPE;

  typedef 
    enum D3D12_FEATURE
  {
    D3D12_FEATURE_D3D12_OPTIONS	= 0,
    D3D12_FEATURE_ARCHITECTURE	= 1,
    D3D12_FEATURE_FEATURE_LEVELS	= 2,
    D3D12_FEATURE_FORMAT_SUPPORT	= 3,
    D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS	= 4,
    D3D12_FEATURE_FORMAT_INFO	= 5,
    D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT	= 6,
    D3D12_FEATURE_SHADER_MODEL	= 7,
    D3D12_FEATURE_D3D12_OPTIONS1	= 8,
    D3D12_FEATURE_PROTECTED_RESOURCE_SESSION_SUPPORT	= 10,
    D3D12_FEATURE_ROOT_SIGNATURE	= 12,
    D3D12_FEATURE_ARCHITECTURE1	= 16,
    D3D12_FEATURE_D3D12_OPTIONS2	= 18,
    D3D12_FEATURE_SHADER_CACHE	= 19,
    D3D12_FEATURE_COMMAND_QUEUE_PRIORITY	= 20,
    D3D12_FEATURE_D3D12_OPTIONS3	= 21,
    D3D12_FEATURE_EXISTING_HEAPS	= 22,
    D3D12_FEATURE_D3D12_OPTIONS4	= 23,
    D3D12_FEATURE_SERIALIZATION	= 24,
    D3D12_FEATURE_CROSS_NODE	= 25
  } 	D3D12_FEATURE;

  typedef 
    enum D3D12_SHADER_MIN_PRECISION_SUPPORT
  {
    D3D12_SHADER_MIN_PRECISION_SUPPORT_NONE	= 0,
    D3D12_SHADER_MIN_PRECISION_SUPPORT_10_BIT	= 0x1,
    D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT	= 0x2
  } 	D3D12_SHADER_MIN_PRECISION_SUPPORT;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_SHADER_MIN_PRECISION_SUPPORT );
  typedef 
    enum D3D12_TILED_RESOURCES_TIER
  {
    D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED	= 0,
    D3D12_TILED_RESOURCES_TIER_1	= 1,
    D3D12_TILED_RESOURCES_TIER_2	= 2,
    D3D12_TILED_RESOURCES_TIER_3	= 3,
    D3D12_TILED_RESOURCES_TIER_4	= 4
  } 	D3D12_TILED_RESOURCES_TIER;

  typedef 
    enum D3D12_RESOURCE_BINDING_TIER
  {
    D3D12_RESOURCE_BINDING_TIER_1	= 1,
    D3D12_RESOURCE_BINDING_TIER_2	= 2,
    D3D12_RESOURCE_BINDING_TIER_3	= 3
  } 	D3D12_RESOURCE_BINDING_TIER;

  typedef 
    enum D3D12_CONSERVATIVE_RASTERIZATION_TIER
  {
    D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED	= 0,
    D3D12_CONSERVATIVE_RASTERIZATION_TIER_1	= 1,
    D3D12_CONSERVATIVE_RASTERIZATION_TIER_2	= 2,
    D3D12_CONSERVATIVE_RASTERIZATION_TIER_3	= 3
  } 	D3D12_CONSERVATIVE_RASTERIZATION_TIER;

  typedef 
    enum D3D12_FORMAT_SUPPORT1
  {
    D3D12_FORMAT_SUPPORT1_NONE	= 0,
    D3D12_FORMAT_SUPPORT1_BUFFER	= 0x1,
    D3D12_FORMAT_SUPPORT1_IA_VERTEX_BUFFER	= 0x2,
    D3D12_FORMAT_SUPPORT1_IA_INDEX_BUFFER	= 0x4,
    D3D12_FORMAT_SUPPORT1_SO_BUFFER	= 0x8,
    D3D12_FORMAT_SUPPORT1_TEXTURE1D	= 0x10,
    D3D12_FORMAT_SUPPORT1_TEXTURE2D	= 0x20,
    D3D12_FORMAT_SUPPORT1_TEXTURE3D	= 0x40,
    D3D12_FORMAT_SUPPORT1_TEXTURECUBE	= 0x80,
    D3D12_FORMAT_SUPPORT1_SHADER_LOAD	= 0x100,
    D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE	= 0x200,
    D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE_COMPARISON	= 0x400,
    D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE_MONO_TEXT	= 0x800,
    D3D12_FORMAT_SUPPORT1_MIP	= 0x1000,
    D3D12_FORMAT_SUPPORT1_RENDER_TARGET	= 0x4000,
    D3D12_FORMAT_SUPPORT1_BLENDABLE	= 0x8000,
    D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL	= 0x10000,
    D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RESOLVE	= 0x40000,
    D3D12_FORMAT_SUPPORT1_DISPLAY	= 0x80000,
    D3D12_FORMAT_SUPPORT1_CAST_WITHIN_BIT_LAYOUT	= 0x100000,
    D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RENDERTARGET	= 0x200000,
    D3D12_FORMAT_SUPPORT1_MULTISAMPLE_LOAD	= 0x400000,
    D3D12_FORMAT_SUPPORT1_SHADER_GATHER	= 0x800000,
    D3D12_FORMAT_SUPPORT1_BACK_BUFFER_CAST	= 0x1000000,
    D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW	= 0x2000000,
    D3D12_FORMAT_SUPPORT1_SHADER_GATHER_COMPARISON	= 0x4000000,
    D3D12_FORMAT_SUPPORT1_DECODER_OUTPUT	= 0x8000000,
    D3D12_FORMAT_SUPPORT1_VIDEO_PROCESSOR_OUTPUT	= 0x10000000,
    D3D12_FORMAT_SUPPORT1_VIDEO_PROCESSOR_INPUT	= 0x20000000,
    D3D12_FORMAT_SUPPORT1_VIDEO_ENCODER	= 0x40000000
  } 	D3D12_FORMAT_SUPPORT1;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_FORMAT_SUPPORT1 );
  typedef 
    enum D3D12_FORMAT_SUPPORT2
  {
    D3D12_FORMAT_SUPPORT2_NONE	= 0,
    D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_ADD	= 0x1,
    D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_BITWISE_OPS	= 0x2,
    D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_COMPARE_STORE_OR_COMPARE_EXCHANGE	= 0x4,
    D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_EXCHANGE	= 0x8,
    D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_SIGNED_MIN_OR_MAX	= 0x10,
    D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_UNSIGNED_MIN_OR_MAX	= 0x20,
    D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD	= 0x40,
    D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE	= 0x80,
    D3D12_FORMAT_SUPPORT2_OUTPUT_MERGER_LOGIC_OP	= 0x100,
    D3D12_FORMAT_SUPPORT2_TILED	= 0x200,
    D3D12_FORMAT_SUPPORT2_MULTIPLANE_OVERLAY	= 0x4000
  } 	D3D12_FORMAT_SUPPORT2;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_FORMAT_SUPPORT2 );
  typedef 
    enum D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS
  {
    D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE	= 0,
    D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_TILED_RESOURCE	= 0x1
  } 	D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS );
  typedef 
    enum D3D12_CROSS_NODE_SHARING_TIER
  {
    D3D12_CROSS_NODE_SHARING_TIER_NOT_SUPPORTED	= 0,
    D3D12_CROSS_NODE_SHARING_TIER_1_EMULATED	= 1,
    D3D12_CROSS_NODE_SHARING_TIER_1	= 2,
    D3D12_CROSS_NODE_SHARING_TIER_2	= 3,
    D3D12_CROSS_NODE_SHARING_TIER_3	= 4
  } 	D3D12_CROSS_NODE_SHARING_TIER;

  typedef 
    enum D3D12_RESOURCE_HEAP_TIER
  {
    D3D12_RESOURCE_HEAP_TIER_1	= 1,
    D3D12_RESOURCE_HEAP_TIER_2	= 2
  } 	D3D12_RESOURCE_HEAP_TIER;

  typedef 
    enum D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER
  {
    D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED	= 0,
    D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_1	= 1,
    D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_2	= 2
  } 	D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER;

  typedef 
    enum D3D12_VIEW_INSTANCING_TIER
  {
    D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED	= 0,
    D3D12_VIEW_INSTANCING_TIER_1	= 1,
    D3D12_VIEW_INSTANCING_TIER_2	= 2,
    D3D12_VIEW_INSTANCING_TIER_3	= 3
  } 	D3D12_VIEW_INSTANCING_TIER;

  typedef struct D3D12_FEATURE_DATA_D3D12_OPTIONS
  {
    _Out_  BOOL DoublePrecisionFloatShaderOps;
    _Out_  BOOL OutputMergerLogicOp;
    _Out_  D3D12_SHADER_MIN_PRECISION_SUPPORT MinPrecisionSupport;
    _Out_  D3D12_TILED_RESOURCES_TIER TiledResourcesTier;
    _Out_  D3D12_RESOURCE_BINDING_TIER ResourceBindingTier;
    _Out_  BOOL PSSpecifiedStencilRefSupported;
    _Out_  BOOL TypedUAVLoadAdditionalFormats;
    _Out_  BOOL ROVsSupported;
    _Out_  D3D12_CONSERVATIVE_RASTERIZATION_TIER ConservativeRasterizationTier;
    _Out_  UINT MaxGPUVirtualAddressBitsPerResource;
    _Out_  BOOL StandardSwizzle64KBSupported;
    _Out_  D3D12_CROSS_NODE_SHARING_TIER CrossNodeSharingTier;
    _Out_  BOOL CrossAdapterRowMajorTextureSupported;
    _Out_  BOOL VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation;
    _Out_  D3D12_RESOURCE_HEAP_TIER ResourceHeapTier;
  } 	D3D12_FEATURE_DATA_D3D12_OPTIONS;

  typedef struct D3D12_FEATURE_DATA_D3D12_OPTIONS1
  {
    _Out_  BOOL WaveOps;
    _Out_  UINT WaveLaneCountMin;
    _Out_  UINT WaveLaneCountMax;
    _Out_  UINT TotalLaneCount;
    _Out_  BOOL ExpandedComputeResourceStates;
    _Out_  BOOL Int64ShaderOps;
  } 	D3D12_FEATURE_DATA_D3D12_OPTIONS1;

  typedef struct D3D12_FEATURE_DATA_D3D12_OPTIONS2
  {
    _Out_  BOOL DepthBoundsTestSupported;
    _Out_  D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER ProgrammableSamplePositionsTier;
  } 	D3D12_FEATURE_DATA_D3D12_OPTIONS2;

  typedef 
    enum D3D_ROOT_SIGNATURE_VERSION
  {
    D3D_ROOT_SIGNATURE_VERSION_1	= 0x1,
    D3D_ROOT_SIGNATURE_VERSION_1_0	= 0x1,
    D3D_ROOT_SIGNATURE_VERSION_1_1	= 0x2
  } 	D3D_ROOT_SIGNATURE_VERSION;

  typedef struct D3D12_FEATURE_DATA_ROOT_SIGNATURE
  {
    _Inout_  D3D_ROOT_SIGNATURE_VERSION HighestVersion;
  } 	D3D12_FEATURE_DATA_ROOT_SIGNATURE;

  typedef struct D3D12_FEATURE_DATA_ARCHITECTURE
  {
    _In_  UINT NodeIndex;
    _Out_  BOOL TileBasedRenderer;
    _Out_  BOOL UMA;
    _Out_  BOOL CacheCoherentUMA;
  } 	D3D12_FEATURE_DATA_ARCHITECTURE;

  typedef struct D3D12_FEATURE_DATA_ARCHITECTURE1
  {
    _In_  UINT NodeIndex;
    _Out_  BOOL TileBasedRenderer;
    _Out_  BOOL UMA;
    _Out_  BOOL CacheCoherentUMA;
    _Out_  BOOL IsolatedMMU;
  } 	D3D12_FEATURE_DATA_ARCHITECTURE1;

  typedef struct D3D12_FEATURE_DATA_FEATURE_LEVELS
  {
    _In_  UINT NumFeatureLevels;
    _In_reads_(NumFeatureLevels)  const D3D_FEATURE_LEVEL *pFeatureLevelsRequested;
    _Out_  D3D_FEATURE_LEVEL MaxSupportedFeatureLevel;
  } 	D3D12_FEATURE_DATA_FEATURE_LEVELS;

  typedef 
    enum D3D_SHADER_MODEL
  {
    D3D_SHADER_MODEL_5_1	= 0x51,
    D3D_SHADER_MODEL_6_0	= 0x60,
    D3D_SHADER_MODEL_6_1	= 0x61,
    D3D_SHADER_MODEL_6_2	= 0x62
  } 	D3D_SHADER_MODEL;

  typedef struct D3D12_FEATURE_DATA_SHADER_MODEL
  {
    _Inout_  D3D_SHADER_MODEL HighestShaderModel;
  } 	D3D12_FEATURE_DATA_SHADER_MODEL;

  typedef struct D3D12_FEATURE_DATA_FORMAT_SUPPORT
  {
    _In_  DXGI_FORMAT Format;
    _Out_  D3D12_FORMAT_SUPPORT1 Support1;
    _Out_  D3D12_FORMAT_SUPPORT2 Support2;
  } 	D3D12_FEATURE_DATA_FORMAT_SUPPORT;

  typedef struct D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS
  {
    _In_  DXGI_FORMAT Format;
    _In_  UINT SampleCount;
    _In_  D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS Flags;
    _Out_  UINT NumQualityLevels;
  } 	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS;

  typedef struct D3D12_FEATURE_DATA_FORMAT_INFO
  {
    DXGI_FORMAT Format;
    UINT8 PlaneCount;
  } 	D3D12_FEATURE_DATA_FORMAT_INFO;

  typedef struct D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT
  {
    UINT MaxGPUVirtualAddressBitsPerResource;
    UINT MaxGPUVirtualAddressBitsPerProcess;
  } 	D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT;

  typedef 
    enum D3D12_SHADER_CACHE_SUPPORT_FLAGS
  {
    D3D12_SHADER_CACHE_SUPPORT_NONE	= 0,
    D3D12_SHADER_CACHE_SUPPORT_SINGLE_PSO	= 0x1,
    D3D12_SHADER_CACHE_SUPPORT_LIBRARY	= 0x2,
    D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_INPROC_CACHE	= 0x4,
    D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_DISK_CACHE	= 0x8
  } 	D3D12_SHADER_CACHE_SUPPORT_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_SHADER_CACHE_SUPPORT_FLAGS );
  typedef struct D3D12_FEATURE_DATA_SHADER_CACHE
  {
    _Out_  D3D12_SHADER_CACHE_SUPPORT_FLAGS SupportFlags;
  } 	D3D12_FEATURE_DATA_SHADER_CACHE;

  typedef struct D3D12_FEATURE_DATA_COMMAND_QUEUE_PRIORITY
  {
    _In_  D3D12_COMMAND_LIST_TYPE CommandListType;
    _In_  UINT Priority;
    _Out_  BOOL PriorityForTypeIsSupported;
  } 	D3D12_FEATURE_DATA_COMMAND_QUEUE_PRIORITY;

  typedef 
    enum D3D12_COMMAND_LIST_SUPPORT_FLAGS
  {
    D3D12_COMMAND_LIST_SUPPORT_FLAG_NONE	= 0,
    D3D12_COMMAND_LIST_SUPPORT_FLAG_DIRECT	= ( 1 << D3D12_COMMAND_LIST_TYPE_DIRECT ) ,
    D3D12_COMMAND_LIST_SUPPORT_FLAG_BUNDLE	= ( 1 << D3D12_COMMAND_LIST_TYPE_BUNDLE ) ,
    D3D12_COMMAND_LIST_SUPPORT_FLAG_COMPUTE	= ( 1 << D3D12_COMMAND_LIST_TYPE_COMPUTE ) ,
    D3D12_COMMAND_LIST_SUPPORT_FLAG_COPY	= ( 1 << D3D12_COMMAND_LIST_TYPE_COPY ) ,
    D3D12_COMMAND_LIST_SUPPORT_FLAG_VIDEO_DECODE	= ( 1 << D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE ) ,
    D3D12_COMMAND_LIST_SUPPORT_FLAG_VIDEO_PROCESS	= ( 1 << D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS ) 
  } 	D3D12_COMMAND_LIST_SUPPORT_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_COMMAND_LIST_SUPPORT_FLAGS );
  typedef struct D3D12_FEATURE_DATA_D3D12_OPTIONS3
  {
    _Out_  BOOL CopyQueueTimestampQueriesSupported;
    _Out_  BOOL CastingFullyTypedFormatSupported;
    _Out_  D3D12_COMMAND_LIST_SUPPORT_FLAGS WriteBufferImmediateSupportFlags;
    _Out_  D3D12_VIEW_INSTANCING_TIER ViewInstancingTier;
    _Out_  BOOL BarycentricsSupported;
  } 	D3D12_FEATURE_DATA_D3D12_OPTIONS3;

  typedef struct D3D12_FEATURE_DATA_EXISTING_HEAPS
  {
    _Out_  BOOL Supported;
  } 	D3D12_FEATURE_DATA_EXISTING_HEAPS;

  typedef 
    enum D3D12_SHARED_RESOURCE_COMPATIBILITY_TIER
  {
    D3D12_SHARED_RESOURCE_COMPATIBILITY_TIER_0	= 0,
    D3D12_SHARED_RESOURCE_COMPATIBILITY_TIER_1	= ( D3D12_SHARED_RESOURCE_COMPATIBILITY_TIER_0 + 1 ) 
  } 	D3D12_SHARED_RESOURCE_COMPATIBILITY_TIER;

  typedef struct D3D12_FEATURE_DATA_D3D12_OPTIONS4
  {
    _Out_  BOOL MSAA64KBAlignedTextureSupported;
    _Out_  D3D12_SHARED_RESOURCE_COMPATIBILITY_TIER SharedResourceCompatibilityTier;
    _Out_  BOOL Native16BitShaderOpsSupported;
  } 	D3D12_FEATURE_DATA_D3D12_OPTIONS4;

  typedef 
    enum D3D12_HEAP_SERIALIZATION_TIER
  {
    D3D12_HEAP_SERIALIZATION_TIER_0	= 0,
    D3D12_HEAP_SERIALIZATION_TIER_10	= 10
  } 	D3D12_HEAP_SERIALIZATION_TIER;

  typedef struct D3D12_FEATURE_DATA_SERIALIZATION
  {
    _In_  UINT NodeIndex;
    _Out_  D3D12_HEAP_SERIALIZATION_TIER HeapSerializationTier;
  } 	D3D12_FEATURE_DATA_SERIALIZATION;

  typedef struct D3D12_FEATURE_DATA_CROSS_NODE
  {
    D3D12_CROSS_NODE_SHARING_TIER SharingTier;
    BOOL AtomicShaderInstructions;
  } 	D3D12_FEATURE_DATA_CROSS_NODE;

  typedef struct D3D12_RESOURCE_ALLOCATION_INFO
  {
    UINT64 SizeInBytes;
    UINT64 Alignment;
  } 	D3D12_RESOURCE_ALLOCATION_INFO;

  typedef struct D3D12_RESOURCE_ALLOCATION_INFO1
  {
    UINT64 Offset;
    UINT64 Alignment;
    UINT64 SizeInBytes;
  } 	D3D12_RESOURCE_ALLOCATION_INFO1;

  typedef 
    enum D3D12_HEAP_TYPE
  {
    D3D12_HEAP_TYPE_DEFAULT	= 1,
    D3D12_HEAP_TYPE_UPLOAD	= 2,
    D3D12_HEAP_TYPE_READBACK	= 3,
    D3D12_HEAP_TYPE_CUSTOM	= 4
  } 	D3D12_HEAP_TYPE;

  typedef 
    enum D3D12_CPU_PAGE_PROPERTY
  {
    D3D12_CPU_PAGE_PROPERTY_UNKNOWN	= 0,
    D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE	= 1,
    D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE	= 2,
    D3D12_CPU_PAGE_PROPERTY_WRITE_BACK	= 3
  } 	D3D12_CPU_PAGE_PROPERTY;

  typedef 
    enum D3D12_MEMORY_POOL
  {
    D3D12_MEMORY_POOL_UNKNOWN	= 0,
    D3D12_MEMORY_POOL_L0	= 1,
    D3D12_MEMORY_POOL_L1	= 2
  } 	D3D12_MEMORY_POOL;

  typedef struct D3D12_HEAP_PROPERTIES
  {
    D3D12_HEAP_TYPE Type;
    D3D12_CPU_PAGE_PROPERTY CPUPageProperty;
    D3D12_MEMORY_POOL MemoryPoolPreference;
    UINT CreationNodeMask;
    UINT VisibleNodeMask;
  } 	D3D12_HEAP_PROPERTIES;

  typedef 
    enum D3D12_HEAP_FLAGS
  {
    D3D12_HEAP_FLAG_NONE	= 0,
    D3D12_HEAP_FLAG_SHARED	= 0x1,
    D3D12_HEAP_FLAG_DENY_BUFFERS	= 0x4,
    D3D12_HEAP_FLAG_ALLOW_DISPLAY	= 0x8,
    D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER	= 0x20,
    D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES	= 0x40,
    D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES	= 0x80,
    D3D12_HEAP_FLAG_HARDWARE_PROTECTED	= 0x100,
    D3D12_HEAP_FLAG_ALLOW_WRITE_WATCH	= 0x200,
    D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS	= 0x400,
    D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES	= 0,
    D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS	= 0xc0,
    D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES	= 0x44,
    D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES	= 0x84
  } 	D3D12_HEAP_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_HEAP_FLAGS );
  typedef struct D3D12_HEAP_DESC
  {
    UINT64 SizeInBytes;
    D3D12_HEAP_PROPERTIES Properties;
    UINT64 Alignment;
    D3D12_HEAP_FLAGS Flags;
  } 	D3D12_HEAP_DESC;

  typedef 
    enum D3D12_RESOURCE_DIMENSION
  {
    D3D12_RESOURCE_DIMENSION_UNKNOWN	= 0,
    D3D12_RESOURCE_DIMENSION_BUFFER	= 1,
    D3D12_RESOURCE_DIMENSION_TEXTURE1D	= 2,
    D3D12_RESOURCE_DIMENSION_TEXTURE2D	= 3,
    D3D12_RESOURCE_DIMENSION_TEXTURE3D	= 4
  } 	D3D12_RESOURCE_DIMENSION;

  typedef 
    enum D3D12_TEXTURE_LAYOUT
  {
    D3D12_TEXTURE_LAYOUT_UNKNOWN	= 0,
    D3D12_TEXTURE_LAYOUT_ROW_MAJOR	= 1,
    D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE	= 2,
    D3D12_TEXTURE_LAYOUT_64KB_STANDARD_SWIZZLE	= 3
  } 	D3D12_TEXTURE_LAYOUT;

  typedef 
    enum D3D12_RESOURCE_FLAGS
  {
    D3D12_RESOURCE_FLAG_NONE	= 0,
    D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET	= 0x1,
    D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL	= 0x2,
    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS	= 0x4,
    D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE	= 0x8,
    D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER	= 0x10,
    D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS	= 0x20,
    D3D12_RESOURCE_FLAG_VIDEO_DECODE_REFERENCE_ONLY	= 0x40
  } 	D3D12_RESOURCE_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_RESOURCE_FLAGS );
  typedef struct D3D12_RESOURCE_DESC
  {
    D3D12_RESOURCE_DIMENSION Dimension;
    UINT64 Alignment;
    UINT64 Width;
    UINT Height;
    UINT16 DepthOrArraySize;
    UINT16 MipLevels;
    DXGI_FORMAT Format;
    DXGI_SAMPLE_DESC SampleDesc;
    D3D12_TEXTURE_LAYOUT Layout;
    D3D12_RESOURCE_FLAGS Flags;
  } 	D3D12_RESOURCE_DESC;

  typedef struct D3D12_DEPTH_STENCIL_VALUE
  {
    FLOAT Depth;
    UINT8 Stencil;
  } 	D3D12_DEPTH_STENCIL_VALUE;

  typedef struct D3D12_CLEAR_VALUE
  {
    DXGI_FORMAT Format;
    union 
    {
      FLOAT Color[ 4 ];
      D3D12_DEPTH_STENCIL_VALUE DepthStencil;
    } 	;
  } 	D3D12_CLEAR_VALUE;

  typedef struct D3D12_RANGE
  {
    SIZE_T Begin;
    SIZE_T End;
  } 	D3D12_RANGE;

  typedef struct D3D12_RANGE_UINT64
  {
    UINT64 Begin;
    UINT64 End;
  } 	D3D12_RANGE_UINT64;

  typedef struct D3D12_SUBRESOURCE_RANGE_UINT64
  {
    UINT Subresource;
    D3D12_RANGE_UINT64 Range;
  } 	D3D12_SUBRESOURCE_RANGE_UINT64;

  typedef struct D3D12_SUBRESOURCE_INFO
  {
    UINT64 Offset;
    UINT RowPitch;
    UINT DepthPitch;
  } 	D3D12_SUBRESOURCE_INFO;

  typedef struct D3D12_TILED_RESOURCE_COORDINATE
  {
    UINT X;
    UINT Y;
    UINT Z;
    UINT Subresource;
  } 	D3D12_TILED_RESOURCE_COORDINATE;

  typedef struct D3D12_TILE_REGION_SIZE
  {
    UINT NumTiles;
    BOOL UseBox;
    UINT Width;
    UINT16 Height;
    UINT16 Depth;
  } 	D3D12_TILE_REGION_SIZE;

  typedef 
    enum D3D12_TILE_RANGE_FLAGS
  {
    D3D12_TILE_RANGE_FLAG_NONE	= 0,
    D3D12_TILE_RANGE_FLAG_NULL	= 1,
    D3D12_TILE_RANGE_FLAG_SKIP	= 2,
    D3D12_TILE_RANGE_FLAG_REUSE_SINGLE_TILE	= 4
  } 	D3D12_TILE_RANGE_FLAGS;

  typedef struct D3D12_SUBRESOURCE_TILING
  {
    UINT WidthInTiles;
    UINT16 HeightInTiles;
    UINT16 DepthInTiles;
    UINT StartTileIndexInOverallResource;
  } 	D3D12_SUBRESOURCE_TILING;

  typedef struct D3D12_TILE_SHAPE
  {
    UINT WidthInTexels;
    UINT HeightInTexels;
    UINT DepthInTexels;
  } 	D3D12_TILE_SHAPE;

  typedef struct D3D12_PACKED_MIP_INFO
  {
    UINT8 NumStandardMips;
    UINT8 NumPackedMips;
    UINT NumTilesForPackedMips;
    UINT StartTileIndexInOverallResource;
  } 	D3D12_PACKED_MIP_INFO;

  typedef 
    enum D3D12_TILE_MAPPING_FLAGS
  {
    D3D12_TILE_MAPPING_FLAG_NONE	= 0,
    D3D12_TILE_MAPPING_FLAG_NO_HAZARD	= 0x1
  } 	D3D12_TILE_MAPPING_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_TILE_MAPPING_FLAGS );
  typedef 
    enum D3D12_TILE_COPY_FLAGS
  {
    D3D12_TILE_COPY_FLAG_NONE	= 0,
    D3D12_TILE_COPY_FLAG_NO_HAZARD	= 0x1,
    D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE	= 0x2,
    D3D12_TILE_COPY_FLAG_SWIZZLED_TILED_RESOURCE_TO_LINEAR_BUFFER	= 0x4
  } 	D3D12_TILE_COPY_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_TILE_COPY_FLAGS );
  typedef 
    enum D3D12_RESOURCE_STATES
  {
    D3D12_RESOURCE_STATE_COMMON	= 0,
    D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER	= 0x1,
    D3D12_RESOURCE_STATE_INDEX_BUFFER	= 0x2,
    D3D12_RESOURCE_STATE_RENDER_TARGET	= 0x4,
    D3D12_RESOURCE_STATE_UNORDERED_ACCESS	= 0x8,
    D3D12_RESOURCE_STATE_DEPTH_WRITE	= 0x10,
    D3D12_RESOURCE_STATE_DEPTH_READ	= 0x20,
    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE	= 0x40,
    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE	= 0x80,
    D3D12_RESOURCE_STATE_STREAM_OUT	= 0x100,
    D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT	= 0x200,
    D3D12_RESOURCE_STATE_COPY_DEST	= 0x400,
    D3D12_RESOURCE_STATE_COPY_SOURCE	= 0x800,
    D3D12_RESOURCE_STATE_RESOLVE_DEST	= 0x1000,
    D3D12_RESOURCE_STATE_RESOLVE_SOURCE	= 0x2000,
    D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE	= 0x400000,
    D3D12_RESOURCE_STATE_GENERIC_READ	= ( ( ( ( ( 0x1 | 0x2 )  | 0x40 )  | 0x80 )  | 0x200 )  | 0x800 ) ,
    D3D12_RESOURCE_STATE_PRESENT	= 0,
    D3D12_RESOURCE_STATE_PREDICATION	= 0x200,
    D3D12_RESOURCE_STATE_VIDEO_DECODE_READ	= 0x10000,
    D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE	= 0x20000,
    D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ	= 0x40000,
    D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE	= 0x80000
  } 	D3D12_RESOURCE_STATES;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_RESOURCE_STATES );
  typedef 
    enum D3D12_RESOURCE_BARRIER_TYPE
  {
    D3D12_RESOURCE_BARRIER_TYPE_TRANSITION	= 0,
    D3D12_RESOURCE_BARRIER_TYPE_ALIASING	= ( D3D12_RESOURCE_BARRIER_TYPE_TRANSITION + 1 ) ,
    D3D12_RESOURCE_BARRIER_TYPE_UAV	= ( D3D12_RESOURCE_BARRIER_TYPE_ALIASING + 1 ) 
  } 	D3D12_RESOURCE_BARRIER_TYPE;


  typedef struct D3D12_RESOURCE_TRANSITION_BARRIER
  {
    ID3D12Resource *pResource;
    UINT Subresource;
    D3D12_RESOURCE_STATES StateBefore;
    D3D12_RESOURCE_STATES StateAfter;
  } 	D3D12_RESOURCE_TRANSITION_BARRIER;

  typedef struct D3D12_RESOURCE_ALIASING_BARRIER
  {
    ID3D12Resource *pResourceBefore;
    ID3D12Resource *pResourceAfter;
  } 	D3D12_RESOURCE_ALIASING_BARRIER;

  typedef struct D3D12_RESOURCE_UAV_BARRIER
  {
    ID3D12Resource *pResource;
  } 	D3D12_RESOURCE_UAV_BARRIER;

  typedef 
    enum D3D12_RESOURCE_BARRIER_FLAGS
  {
    D3D12_RESOURCE_BARRIER_FLAG_NONE	= 0,
    D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY	= 0x1,
    D3D12_RESOURCE_BARRIER_FLAG_END_ONLY	= 0x2
  } 	D3D12_RESOURCE_BARRIER_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_RESOURCE_BARRIER_FLAGS );
  typedef struct D3D12_RESOURCE_BARRIER
  {
    D3D12_RESOURCE_BARRIER_TYPE Type;
    D3D12_RESOURCE_BARRIER_FLAGS Flags;
    union 
    {
      D3D12_RESOURCE_TRANSITION_BARRIER Transition;
      D3D12_RESOURCE_ALIASING_BARRIER Aliasing;
      D3D12_RESOURCE_UAV_BARRIER UAV;
    } 	;
  } 	D3D12_RESOURCE_BARRIER;

  typedef struct D3D12_SUBRESOURCE_FOOTPRINT
  {
    DXGI_FORMAT Format;
    UINT Width;
    UINT Height;
    UINT Depth;
    UINT RowPitch;
  } 	D3D12_SUBRESOURCE_FOOTPRINT;

  typedef struct D3D12_PLACED_SUBRESOURCE_FOOTPRINT
  {
    UINT64 Offset;
    D3D12_SUBRESOURCE_FOOTPRINT Footprint;
  } 	D3D12_PLACED_SUBRESOURCE_FOOTPRINT;

  typedef 
    enum D3D12_TEXTURE_COPY_TYPE
  {
    D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX	= 0,
    D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT	= 1
  } 	D3D12_TEXTURE_COPY_TYPE;

  typedef struct D3D12_TEXTURE_COPY_LOCATION
  {
    ID3D12Resource *pResource;
    D3D12_TEXTURE_COPY_TYPE Type;
    union 
    {
      D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedFootprint;
      UINT SubresourceIndex;
    } 	;
  } 	D3D12_TEXTURE_COPY_LOCATION;

  typedef 
    enum D3D12_RESOLVE_MODE
  {
    D3D12_RESOLVE_MODE_DECOMPRESS	= 0,
    D3D12_RESOLVE_MODE_MIN	= 1,
    D3D12_RESOLVE_MODE_MAX	= 2,
    D3D12_RESOLVE_MODE_AVERAGE	= 3
  } 	D3D12_RESOLVE_MODE;

  typedef struct D3D12_SAMPLE_POSITION
  {
    INT8 X;
    INT8 Y;
  } 	D3D12_SAMPLE_POSITION;

  typedef struct D3D12_VIEW_INSTANCE_LOCATION
  {
    UINT ViewportArrayIndex;
    UINT RenderTargetArrayIndex;
  } 	D3D12_VIEW_INSTANCE_LOCATION;

  typedef 
    enum D3D12_VIEW_INSTANCING_FLAGS
  {
    D3D12_VIEW_INSTANCING_FLAG_NONE	= 0,
    D3D12_VIEW_INSTANCING_FLAG_ENABLE_VIEW_INSTANCE_MASKING	= 0x1
  } 	D3D12_VIEW_INSTANCING_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_VIEW_INSTANCING_FLAGS );
  typedef struct D3D12_VIEW_INSTANCING_DESC
  {
    UINT ViewInstanceCount;
    _Field_size_full_(ViewInstanceCount)  const D3D12_VIEW_INSTANCE_LOCATION *pViewInstanceLocations;
    D3D12_VIEW_INSTANCING_FLAGS Flags;
  } 	D3D12_VIEW_INSTANCING_DESC;

  typedef 
    enum D3D12_SHADER_COMPONENT_MAPPING
  {
    D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0	= 0,
    D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1	= 1,
    D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2	= 2,
    D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3	= 3,
    D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0	= 4,
    D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1	= 5
  } 	D3D12_SHADER_COMPONENT_MAPPING;

#define D3D12_SHADER_COMPONENT_MAPPING_MASK 0x7 
#define D3D12_SHADER_COMPONENT_MAPPING_SHIFT 3 
#define D3D12_SHADER_COMPONENT_MAPPING_ALWAYS_SET_BIT_AVOIDING_ZEROMEM_MISTAKES (1<<(D3D12_SHADER_COMPONENT_MAPPING_SHIFT*4)) 
#define D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(Src0,Src1,Src2,Src3) ((((Src0)&D3D12_SHADER_COMPONENT_MAPPING_MASK)| \
                                                                (((Src1)&D3D12_SHADER_COMPONENT_MAPPING_MASK)<<D3D12_SHADER_COMPONENT_MAPPING_SHIFT)| \
                                                                (((Src2)&D3D12_SHADER_COMPONENT_MAPPING_MASK)<<(D3D12_SHADER_COMPONENT_MAPPING_SHIFT*2))| \
                                                                (((Src3)&D3D12_SHADER_COMPONENT_MAPPING_MASK)<<(D3D12_SHADER_COMPONENT_MAPPING_SHIFT*3))| \
                                                                D3D12_SHADER_COMPONENT_MAPPING_ALWAYS_SET_BIT_AVOIDING_ZEROMEM_MISTAKES))
#define D3D12_DECODE_SHADER_4_COMPONENT_MAPPING(ComponentToExtract,Mapping) ((D3D12_SHADER_COMPONENT_MAPPING)(Mapping >> (D3D12_SHADER_COMPONENT_MAPPING_SHIFT*ComponentToExtract) & D3D12_SHADER_COMPONENT_MAPPING_MASK))
#define D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0,1,2,3) 
  typedef 
    enum D3D12_BUFFER_SRV_FLAGS
  {
    D3D12_BUFFER_SRV_FLAG_NONE	= 0,
    D3D12_BUFFER_SRV_FLAG_RAW	= 0x1
  } 	D3D12_BUFFER_SRV_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_BUFFER_SRV_FLAGS );
  typedef struct D3D12_BUFFER_SRV
  {
    UINT64 FirstElement;
    UINT NumElements;
    UINT StructureByteStride;
    D3D12_BUFFER_SRV_FLAGS Flags;
  } 	D3D12_BUFFER_SRV;

  typedef struct D3D12_TEX1D_SRV
  {
    UINT MostDetailedMip;
    UINT MipLevels;
    FLOAT ResourceMinLODClamp;
  } 	D3D12_TEX1D_SRV;

  typedef struct D3D12_TEX1D_ARRAY_SRV
  {
    UINT MostDetailedMip;
    UINT MipLevels;
    UINT FirstArraySlice;
    UINT ArraySize;
    FLOAT ResourceMinLODClamp;
  } 	D3D12_TEX1D_ARRAY_SRV;

  typedef struct D3D12_TEX2D_SRV
  {
    UINT MostDetailedMip;
    UINT MipLevels;
    UINT PlaneSlice;
    FLOAT ResourceMinLODClamp;
  } 	D3D12_TEX2D_SRV;

  typedef struct D3D12_TEX2D_ARRAY_SRV
  {
    UINT MostDetailedMip;
    UINT MipLevels;
    UINT FirstArraySlice;
    UINT ArraySize;
    UINT PlaneSlice;
    FLOAT ResourceMinLODClamp;
  } 	D3D12_TEX2D_ARRAY_SRV;

  typedef struct D3D12_TEX3D_SRV
  {
    UINT MostDetailedMip;
    UINT MipLevels;
    FLOAT ResourceMinLODClamp;
  } 	D3D12_TEX3D_SRV;

  typedef struct D3D12_TEXCUBE_SRV
  {
    UINT MostDetailedMip;
    UINT MipLevels;
    FLOAT ResourceMinLODClamp;
  } 	D3D12_TEXCUBE_SRV;

  typedef struct D3D12_TEXCUBE_ARRAY_SRV
  {
    UINT MostDetailedMip;
    UINT MipLevels;
    UINT First2DArrayFace;
    UINT NumCubes;
    FLOAT ResourceMinLODClamp;
  } 	D3D12_TEXCUBE_ARRAY_SRV;

  typedef struct D3D12_TEX2DMS_SRV
  {
    UINT UnusedField_NothingToDefine;
  } 	D3D12_TEX2DMS_SRV;

  typedef struct D3D12_TEX2DMS_ARRAY_SRV
  {
    UINT FirstArraySlice;
    UINT ArraySize;
  } 	D3D12_TEX2DMS_ARRAY_SRV;

  typedef struct D3D12_RAYTRACING_ACCELERATION_STRUCTURE_SRV
  {
    D3D12_GPU_VIRTUAL_ADDRESS Location;
  } 	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_SRV;

  typedef 
    enum D3D12_SRV_DIMENSION
  {
    D3D12_SRV_DIMENSION_UNKNOWN	= 0,
    D3D12_SRV_DIMENSION_BUFFER	= 1,
    D3D12_SRV_DIMENSION_TEXTURE1D	= 2,
    D3D12_SRV_DIMENSION_TEXTURE1DARRAY	= 3,
    D3D12_SRV_DIMENSION_TEXTURE2D	= 4,
    D3D12_SRV_DIMENSION_TEXTURE2DARRAY	= 5,
    D3D12_SRV_DIMENSION_TEXTURE2DMS	= 6,
    D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY	= 7,
    D3D12_SRV_DIMENSION_TEXTURE3D	= 8,
    D3D12_SRV_DIMENSION_TEXTURECUBE	= 9,
    D3D12_SRV_DIMENSION_TEXTURECUBEARRAY	= 10,
    D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE	= 11
  } 	D3D12_SRV_DIMENSION;

  typedef struct D3D12_SHADER_RESOURCE_VIEW_DESC
  {
    DXGI_FORMAT Format;
    D3D12_SRV_DIMENSION ViewDimension;
    UINT Shader4ComponentMapping;
    union 
    {
      D3D12_BUFFER_SRV Buffer;
      D3D12_TEX1D_SRV Texture1D;
      D3D12_TEX1D_ARRAY_SRV Texture1DArray;
      D3D12_TEX2D_SRV Texture2D;
      D3D12_TEX2D_ARRAY_SRV Texture2DArray;
      D3D12_TEX2DMS_SRV Texture2DMS;
      D3D12_TEX2DMS_ARRAY_SRV Texture2DMSArray;
      D3D12_TEX3D_SRV Texture3D;
      D3D12_TEXCUBE_SRV TextureCube;
      D3D12_TEXCUBE_ARRAY_SRV TextureCubeArray;
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_SRV RaytracingAccelerationStructure;
    } 	;
  } 	D3D12_SHADER_RESOURCE_VIEW_DESC;

  typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC
  {
    D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
    UINT SizeInBytes;
  } 	D3D12_CONSTANT_BUFFER_VIEW_DESC;

  typedef 
    enum D3D12_FILTER
  {
    D3D12_FILTER_MIN_MAG_MIP_POINT	= 0,
    D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR	= 0x1,
    D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT	= 0x4,
    D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR	= 0x5,
    D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT	= 0x10,
    D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR	= 0x11,
    D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT	= 0x14,
    D3D12_FILTER_MIN_MAG_MIP_LINEAR	= 0x15,
    D3D12_FILTER_ANISOTROPIC	= 0x55,
    D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT	= 0x80,
    D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR	= 0x81,
    D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT	= 0x84,
    D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR	= 0x85,
    D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT	= 0x90,
    D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR	= 0x91,
    D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT	= 0x94,
    D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR	= 0x95,
    D3D12_FILTER_COMPARISON_ANISOTROPIC	= 0xd5,
    D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT	= 0x100,
    D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR	= 0x101,
    D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT	= 0x104,
    D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR	= 0x105,
    D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT	= 0x110,
    D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR	= 0x111,
    D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT	= 0x114,
    D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR	= 0x115,
    D3D12_FILTER_MINIMUM_ANISOTROPIC	= 0x155,
    D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT	= 0x180,
    D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR	= 0x181,
    D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT	= 0x184,
    D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR	= 0x185,
    D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT	= 0x190,
    D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR	= 0x191,
    D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT	= 0x194,
    D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR	= 0x195,
    D3D12_FILTER_MAXIMUM_ANISOTROPIC	= 0x1d5
  } 	D3D12_FILTER;

  typedef 
    enum D3D12_FILTER_TYPE
  {
    D3D12_FILTER_TYPE_POINT	= 0,
    D3D12_FILTER_TYPE_LINEAR	= 1
  } 	D3D12_FILTER_TYPE;

  typedef 
    enum D3D12_FILTER_REDUCTION_TYPE
  {
    D3D12_FILTER_REDUCTION_TYPE_STANDARD	= 0,
    D3D12_FILTER_REDUCTION_TYPE_COMPARISON	= 1,
    D3D12_FILTER_REDUCTION_TYPE_MINIMUM	= 2,
    D3D12_FILTER_REDUCTION_TYPE_MAXIMUM	= 3
  } 	D3D12_FILTER_REDUCTION_TYPE;

#define	D3D12_FILTER_REDUCTION_TYPE_MASK	( 0x3 )

#define	D3D12_FILTER_REDUCTION_TYPE_SHIFT	( 7 )

#define	D3D12_FILTER_TYPE_MASK	( 0x3 )

#define	D3D12_MIN_FILTER_SHIFT	( 4 )

#define	D3D12_MAG_FILTER_SHIFT	( 2 )

#define	D3D12_MIP_FILTER_SHIFT	( 0 )

#define	D3D12_ANISOTROPIC_FILTERING_BIT	( 0x40 )

#define D3D12_ENCODE_BASIC_FILTER( min, mag, mip, reduction )                                                     \
                                   ( ( D3D12_FILTER ) (                                                           \
                                   ( ( ( min ) & D3D12_FILTER_TYPE_MASK ) << D3D12_MIN_FILTER_SHIFT ) |           \
                                   ( ( ( mag ) & D3D12_FILTER_TYPE_MASK ) << D3D12_MAG_FILTER_SHIFT ) |           \
                                   ( ( ( mip ) & D3D12_FILTER_TYPE_MASK ) << D3D12_MIP_FILTER_SHIFT ) |           \
                                   ( ( ( reduction ) & D3D12_FILTER_REDUCTION_TYPE_MASK ) << D3D12_FILTER_REDUCTION_TYPE_SHIFT ) ) ) 
#define D3D12_ENCODE_ANISOTROPIC_FILTER( reduction )                                                  \
                                         ( ( D3D12_FILTER ) (                                         \
                                         D3D12_ANISOTROPIC_FILTERING_BIT |                            \
                                         D3D12_ENCODE_BASIC_FILTER( D3D12_FILTER_TYPE_LINEAR,         \
                                                                    D3D12_FILTER_TYPE_LINEAR,         \
                                                                    D3D12_FILTER_TYPE_LINEAR,         \
                                                                    reduction ) ) )                     
#define D3D12_DECODE_MIN_FILTER( D3D12Filter )                                                              \
                                 ( ( D3D12_FILTER_TYPE )                                                    \
                                 ( ( ( D3D12Filter ) >> D3D12_MIN_FILTER_SHIFT ) & D3D12_FILTER_TYPE_MASK ) ) 
#define D3D12_DECODE_MAG_FILTER( D3D12Filter )                                                              \
                                 ( ( D3D12_FILTER_TYPE )                                                    \
                                 ( ( ( D3D12Filter ) >> D3D12_MAG_FILTER_SHIFT ) & D3D12_FILTER_TYPE_MASK ) ) 
#define D3D12_DECODE_MIP_FILTER( D3D12Filter )                                                              \
                                 ( ( D3D12_FILTER_TYPE )                                                    \
                                 ( ( ( D3D12Filter ) >> D3D12_MIP_FILTER_SHIFT ) & D3D12_FILTER_TYPE_MASK ) ) 
#define D3D12_DECODE_FILTER_REDUCTION( D3D12Filter )                                                        \
                                 ( ( D3D12_FILTER_REDUCTION_TYPE )                                                      \
                                 ( ( ( D3D12Filter ) >> D3D12_FILTER_REDUCTION_TYPE_SHIFT ) & D3D12_FILTER_REDUCTION_TYPE_MASK ) ) 
#define D3D12_DECODE_IS_COMPARISON_FILTER( D3D12Filter )                                                    \
                                 ( D3D12_DECODE_FILTER_REDUCTION( D3D12Filter ) == D3D12_FILTER_REDUCTION_TYPE_COMPARISON ) 
#define D3D12_DECODE_IS_ANISOTROPIC_FILTER( D3D12Filter )                                               \
                            ( ( ( D3D12Filter ) & D3D12_ANISOTROPIC_FILTERING_BIT ) &&                  \
                            ( D3D12_FILTER_TYPE_LINEAR == D3D12_DECODE_MIN_FILTER( D3D12Filter ) ) &&   \
                            ( D3D12_FILTER_TYPE_LINEAR == D3D12_DECODE_MAG_FILTER( D3D12Filter ) ) &&   \
                            ( D3D12_FILTER_TYPE_LINEAR == D3D12_DECODE_MIP_FILTER( D3D12Filter ) ) )      
  typedef 
    enum D3D12_TEXTURE_ADDRESS_MODE
  {
    D3D12_TEXTURE_ADDRESS_MODE_WRAP	= 1,
    D3D12_TEXTURE_ADDRESS_MODE_MIRROR	= 2,
    D3D12_TEXTURE_ADDRESS_MODE_CLAMP	= 3,
    D3D12_TEXTURE_ADDRESS_MODE_BORDER	= 4,
    D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE	= 5
  } 	D3D12_TEXTURE_ADDRESS_MODE;

  typedef struct D3D12_SAMPLER_DESC
  {
    D3D12_FILTER Filter;
    D3D12_TEXTURE_ADDRESS_MODE AddressU;
    D3D12_TEXTURE_ADDRESS_MODE AddressV;
    D3D12_TEXTURE_ADDRESS_MODE AddressW;
    FLOAT MipLODBias;
    UINT MaxAnisotropy;
    D3D12_COMPARISON_FUNC ComparisonFunc;
    FLOAT BorderColor[ 4 ];
    FLOAT MinLOD;
    FLOAT MaxLOD;
  } 	D3D12_SAMPLER_DESC;

  typedef 
    enum D3D12_BUFFER_UAV_FLAGS
  {
    D3D12_BUFFER_UAV_FLAG_NONE	= 0,
    D3D12_BUFFER_UAV_FLAG_RAW	= 0x1
  } 	D3D12_BUFFER_UAV_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_BUFFER_UAV_FLAGS );
  typedef struct D3D12_BUFFER_UAV
  {
    UINT64 FirstElement;
    UINT NumElements;
    UINT StructureByteStride;
    UINT64 CounterOffsetInBytes;
    D3D12_BUFFER_UAV_FLAGS Flags;
  } 	D3D12_BUFFER_UAV;

  typedef struct D3D12_TEX1D_UAV
  {
    UINT MipSlice;
  } 	D3D12_TEX1D_UAV;

  typedef struct D3D12_TEX1D_ARRAY_UAV
  {
    UINT MipSlice;
    UINT FirstArraySlice;
    UINT ArraySize;
  } 	D3D12_TEX1D_ARRAY_UAV;

  typedef struct D3D12_TEX2D_UAV
  {
    UINT MipSlice;
    UINT PlaneSlice;
  } 	D3D12_TEX2D_UAV;

  typedef struct D3D12_TEX2D_ARRAY_UAV
  {
    UINT MipSlice;
    UINT FirstArraySlice;
    UINT ArraySize;
    UINT PlaneSlice;
  } 	D3D12_TEX2D_ARRAY_UAV;

  typedef struct D3D12_TEX3D_UAV
  {
    UINT MipSlice;
    UINT FirstWSlice;
    UINT WSize;
  } 	D3D12_TEX3D_UAV;

  typedef 
    enum D3D12_UAV_DIMENSION
  {
    D3D12_UAV_DIMENSION_UNKNOWN	= 0,
    D3D12_UAV_DIMENSION_BUFFER	= 1,
    D3D12_UAV_DIMENSION_TEXTURE1D	= 2,
    D3D12_UAV_DIMENSION_TEXTURE1DARRAY	= 3,
    D3D12_UAV_DIMENSION_TEXTURE2D	= 4,
    D3D12_UAV_DIMENSION_TEXTURE2DARRAY	= 5,
    D3D12_UAV_DIMENSION_TEXTURE3D	= 8
  } 	D3D12_UAV_DIMENSION;

  typedef struct D3D12_UNORDERED_ACCESS_VIEW_DESC
  {
    DXGI_FORMAT Format;
    D3D12_UAV_DIMENSION ViewDimension;
    union 
    {
      D3D12_BUFFER_UAV Buffer;
      D3D12_TEX1D_UAV Texture1D;
      D3D12_TEX1D_ARRAY_UAV Texture1DArray;
      D3D12_TEX2D_UAV Texture2D;
      D3D12_TEX2D_ARRAY_UAV Texture2DArray;
      D3D12_TEX3D_UAV Texture3D;
    } 	;
  } 	D3D12_UNORDERED_ACCESS_VIEW_DESC;

  typedef struct D3D12_BUFFER_RTV
  {
    UINT64 FirstElement;
    UINT NumElements;
  } 	D3D12_BUFFER_RTV;

  typedef struct D3D12_TEX1D_RTV
  {
    UINT MipSlice;
  } 	D3D12_TEX1D_RTV;

  typedef struct D3D12_TEX1D_ARRAY_RTV
  {
    UINT MipSlice;
    UINT FirstArraySlice;
    UINT ArraySize;
  } 	D3D12_TEX1D_ARRAY_RTV;

  typedef struct D3D12_TEX2D_RTV
  {
    UINT MipSlice;
    UINT PlaneSlice;
  } 	D3D12_TEX2D_RTV;

  typedef struct D3D12_TEX2DMS_RTV
  {
    UINT UnusedField_NothingToDefine;
  } 	D3D12_TEX2DMS_RTV;

  typedef struct D3D12_TEX2D_ARRAY_RTV
  {
    UINT MipSlice;
    UINT FirstArraySlice;
    UINT ArraySize;
    UINT PlaneSlice;
  } 	D3D12_TEX2D_ARRAY_RTV;

  typedef struct D3D12_TEX2DMS_ARRAY_RTV
  {
    UINT FirstArraySlice;
    UINT ArraySize;
  } 	D3D12_TEX2DMS_ARRAY_RTV;

  typedef struct D3D12_TEX3D_RTV
  {
    UINT MipSlice;
    UINT FirstWSlice;
    UINT WSize;
  } 	D3D12_TEX3D_RTV;

  typedef 
    enum D3D12_RTV_DIMENSION
  {
    D3D12_RTV_DIMENSION_UNKNOWN	= 0,
    D3D12_RTV_DIMENSION_BUFFER	= 1,
    D3D12_RTV_DIMENSION_TEXTURE1D	= 2,
    D3D12_RTV_DIMENSION_TEXTURE1DARRAY	= 3,
    D3D12_RTV_DIMENSION_TEXTURE2D	= 4,
    D3D12_RTV_DIMENSION_TEXTURE2DARRAY	= 5,
    D3D12_RTV_DIMENSION_TEXTURE2DMS	= 6,
    D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY	= 7,
    D3D12_RTV_DIMENSION_TEXTURE3D	= 8
  } 	D3D12_RTV_DIMENSION;

  typedef struct D3D12_RENDER_TARGET_VIEW_DESC
  {
    DXGI_FORMAT Format;
    D3D12_RTV_DIMENSION ViewDimension;
    union 
    {
      D3D12_BUFFER_RTV Buffer;
      D3D12_TEX1D_RTV Texture1D;
      D3D12_TEX1D_ARRAY_RTV Texture1DArray;
      D3D12_TEX2D_RTV Texture2D;
      D3D12_TEX2D_ARRAY_RTV Texture2DArray;
      D3D12_TEX2DMS_RTV Texture2DMS;
      D3D12_TEX2DMS_ARRAY_RTV Texture2DMSArray;
      D3D12_TEX3D_RTV Texture3D;
    } 	;
  } 	D3D12_RENDER_TARGET_VIEW_DESC;

  typedef struct D3D12_TEX1D_DSV
  {
    UINT MipSlice;
  } 	D3D12_TEX1D_DSV;

  typedef struct D3D12_TEX1D_ARRAY_DSV
  {
    UINT MipSlice;
    UINT FirstArraySlice;
    UINT ArraySize;
  } 	D3D12_TEX1D_ARRAY_DSV;

  typedef struct D3D12_TEX2D_DSV
  {
    UINT MipSlice;
  } 	D3D12_TEX2D_DSV;

  typedef struct D3D12_TEX2D_ARRAY_DSV
  {
    UINT MipSlice;
    UINT FirstArraySlice;
    UINT ArraySize;
  } 	D3D12_TEX2D_ARRAY_DSV;

  typedef struct D3D12_TEX2DMS_DSV
  {
    UINT UnusedField_NothingToDefine;
  } 	D3D12_TEX2DMS_DSV;

  typedef struct D3D12_TEX2DMS_ARRAY_DSV
  {
    UINT FirstArraySlice;
    UINT ArraySize;
  } 	D3D12_TEX2DMS_ARRAY_DSV;

  typedef 
    enum D3D12_DSV_FLAGS
  {
    D3D12_DSV_FLAG_NONE	= 0,
    D3D12_DSV_FLAG_READ_ONLY_DEPTH	= 0x1,
    D3D12_DSV_FLAG_READ_ONLY_STENCIL	= 0x2
  } 	D3D12_DSV_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_DSV_FLAGS );
  typedef 
    enum D3D12_DSV_DIMENSION
  {
    D3D12_DSV_DIMENSION_UNKNOWN	= 0,
    D3D12_DSV_DIMENSION_TEXTURE1D	= 1,
    D3D12_DSV_DIMENSION_TEXTURE1DARRAY	= 2,
    D3D12_DSV_DIMENSION_TEXTURE2D	= 3,
    D3D12_DSV_DIMENSION_TEXTURE2DARRAY	= 4,
    D3D12_DSV_DIMENSION_TEXTURE2DMS	= 5,
    D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY	= 6
  } 	D3D12_DSV_DIMENSION;

  typedef struct D3D12_DEPTH_STENCIL_VIEW_DESC
  {
    DXGI_FORMAT Format;
    D3D12_DSV_DIMENSION ViewDimension;
    D3D12_DSV_FLAGS Flags;
    union 
    {
      D3D12_TEX1D_DSV Texture1D;
      D3D12_TEX1D_ARRAY_DSV Texture1DArray;
      D3D12_TEX2D_DSV Texture2D;
      D3D12_TEX2D_ARRAY_DSV Texture2DArray;
      D3D12_TEX2DMS_DSV Texture2DMS;
      D3D12_TEX2DMS_ARRAY_DSV Texture2DMSArray;
    } 	;
  } 	D3D12_DEPTH_STENCIL_VIEW_DESC;

  typedef 
    enum D3D12_CLEAR_FLAGS
  {
    D3D12_CLEAR_FLAG_DEPTH	= 0x1,
    D3D12_CLEAR_FLAG_STENCIL	= 0x2
  } 	D3D12_CLEAR_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_CLEAR_FLAGS );
  typedef 
    enum D3D12_FENCE_FLAGS
  {
    D3D12_FENCE_FLAG_NONE	= 0,
    D3D12_FENCE_FLAG_SHARED	= 0x1,
    D3D12_FENCE_FLAG_SHARED_CROSS_ADAPTER	= 0x2,
    D3D12_FENCE_FLAG_NON_MONITORED	= 0x4
  } 	D3D12_FENCE_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_FENCE_FLAGS );
  typedef 
    enum D3D12_DESCRIPTOR_HEAP_TYPE
  {
    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV	= 0,
    D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER	= ( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV + 1 ) ,
    D3D12_DESCRIPTOR_HEAP_TYPE_RTV	= ( D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER + 1 ) ,
    D3D12_DESCRIPTOR_HEAP_TYPE_DSV	= ( D3D12_DESCRIPTOR_HEAP_TYPE_RTV + 1 ) ,
    D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES	= ( D3D12_DESCRIPTOR_HEAP_TYPE_DSV + 1 ) 
  } 	D3D12_DESCRIPTOR_HEAP_TYPE;

  typedef 
    enum D3D12_DESCRIPTOR_HEAP_FLAGS
  {
    D3D12_DESCRIPTOR_HEAP_FLAG_NONE	= 0,
    D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE	= 0x1
  } 	D3D12_DESCRIPTOR_HEAP_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_DESCRIPTOR_HEAP_FLAGS );
  typedef struct D3D12_DESCRIPTOR_HEAP_DESC
  {
    D3D12_DESCRIPTOR_HEAP_TYPE Type;
    UINT NumDescriptors;
    D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
    UINT NodeMask;
  } 	D3D12_DESCRIPTOR_HEAP_DESC;

  typedef 
    enum D3D12_DESCRIPTOR_RANGE_TYPE
  {
    D3D12_DESCRIPTOR_RANGE_TYPE_SRV	= 0,
    D3D12_DESCRIPTOR_RANGE_TYPE_UAV	= ( D3D12_DESCRIPTOR_RANGE_TYPE_SRV + 1 ) ,
    D3D12_DESCRIPTOR_RANGE_TYPE_CBV	= ( D3D12_DESCRIPTOR_RANGE_TYPE_UAV + 1 ) ,
    D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER	= ( D3D12_DESCRIPTOR_RANGE_TYPE_CBV + 1 ) 
  } 	D3D12_DESCRIPTOR_RANGE_TYPE;

  typedef struct D3D12_DESCRIPTOR_RANGE
  {
    D3D12_DESCRIPTOR_RANGE_TYPE RangeType;
    UINT NumDescriptors;
    UINT BaseShaderRegister;
    UINT RegisterSpace;
    UINT OffsetInDescriptorsFromTableStart;
  } 	D3D12_DESCRIPTOR_RANGE;

  typedef struct D3D12_ROOT_DESCRIPTOR_TABLE
  {
    UINT NumDescriptorRanges;
    _Field_size_full_(NumDescriptorRanges)  const D3D12_DESCRIPTOR_RANGE *pDescriptorRanges;
  } 	D3D12_ROOT_DESCRIPTOR_TABLE;

  typedef struct D3D12_ROOT_CONSTANTS
  {
    UINT ShaderRegister;
    UINT RegisterSpace;
    UINT Num32BitValues;
  } 	D3D12_ROOT_CONSTANTS;

  typedef struct D3D12_ROOT_DESCRIPTOR
  {
    UINT ShaderRegister;
    UINT RegisterSpace;
  } 	D3D12_ROOT_DESCRIPTOR;

  typedef 
    enum D3D12_SHADER_VISIBILITY
  {
    D3D12_SHADER_VISIBILITY_ALL	= 0,
    D3D12_SHADER_VISIBILITY_VERTEX	= 1,
    D3D12_SHADER_VISIBILITY_HULL	= 2,
    D3D12_SHADER_VISIBILITY_DOMAIN	= 3,
    D3D12_SHADER_VISIBILITY_GEOMETRY	= 4,
    D3D12_SHADER_VISIBILITY_PIXEL	= 5
  } 	D3D12_SHADER_VISIBILITY;

  typedef 
    enum D3D12_ROOT_PARAMETER_TYPE
  {
    D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE	= 0,
    D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS	= ( D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE + 1 ) ,
    D3D12_ROOT_PARAMETER_TYPE_CBV	= ( D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS + 1 ) ,
    D3D12_ROOT_PARAMETER_TYPE_SRV	= ( D3D12_ROOT_PARAMETER_TYPE_CBV + 1 ) ,
    D3D12_ROOT_PARAMETER_TYPE_UAV	= ( D3D12_ROOT_PARAMETER_TYPE_SRV + 1 ) 
  } 	D3D12_ROOT_PARAMETER_TYPE;

  typedef struct D3D12_ROOT_PARAMETER
  {
    D3D12_ROOT_PARAMETER_TYPE ParameterType;
    union 
    {
      D3D12_ROOT_DESCRIPTOR_TABLE DescriptorTable;
      D3D12_ROOT_CONSTANTS Constants;
      D3D12_ROOT_DESCRIPTOR Descriptor;
    } 	;
    D3D12_SHADER_VISIBILITY ShaderVisibility;
  } 	D3D12_ROOT_PARAMETER;

  typedef 
    enum D3D12_ROOT_SIGNATURE_FLAGS
  {
    D3D12_ROOT_SIGNATURE_FLAG_NONE	= 0,
    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT	= 0x1,
    D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS	= 0x2,
    D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS	= 0x4,
    D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS	= 0x8,
    D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS	= 0x10,
    D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS	= 0x20,
    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT	= 0x40,
    D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE	= 0x80
  } 	D3D12_ROOT_SIGNATURE_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_ROOT_SIGNATURE_FLAGS );
  typedef 
    enum D3D12_STATIC_BORDER_COLOR
  {
    D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK	= 0,
    D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK	= ( D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK + 1 ) ,
    D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE	= ( D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK + 1 ) 
  } 	D3D12_STATIC_BORDER_COLOR;

  typedef struct D3D12_STATIC_SAMPLER_DESC
  {
    D3D12_FILTER Filter;
    D3D12_TEXTURE_ADDRESS_MODE AddressU;
    D3D12_TEXTURE_ADDRESS_MODE AddressV;
    D3D12_TEXTURE_ADDRESS_MODE AddressW;
    FLOAT MipLODBias;
    UINT MaxAnisotropy;
    D3D12_COMPARISON_FUNC ComparisonFunc;
    D3D12_STATIC_BORDER_COLOR BorderColor;
    FLOAT MinLOD;
    FLOAT MaxLOD;
    UINT ShaderRegister;
    UINT RegisterSpace;
    D3D12_SHADER_VISIBILITY ShaderVisibility;
  } 	D3D12_STATIC_SAMPLER_DESC;

  typedef struct D3D12_ROOT_SIGNATURE_DESC
  {
    UINT NumParameters;
    _Field_size_full_(NumParameters)  const D3D12_ROOT_PARAMETER *pParameters;
    UINT NumStaticSamplers;
    _Field_size_full_(NumStaticSamplers)  const D3D12_STATIC_SAMPLER_DESC *pStaticSamplers;
    D3D12_ROOT_SIGNATURE_FLAGS Flags;
  } 	D3D12_ROOT_SIGNATURE_DESC;

  typedef 
    enum D3D12_DESCRIPTOR_RANGE_FLAGS
  {
    D3D12_DESCRIPTOR_RANGE_FLAG_NONE	= 0,
    D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE	= 0x1,
    D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE	= 0x2,
    D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE	= 0x4,
    D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC	= 0x8
  } 	D3D12_DESCRIPTOR_RANGE_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_DESCRIPTOR_RANGE_FLAGS );
  typedef struct D3D12_DESCRIPTOR_RANGE1
  {
    D3D12_DESCRIPTOR_RANGE_TYPE RangeType;
    UINT NumDescriptors;
    UINT BaseShaderRegister;
    UINT RegisterSpace;
    D3D12_DESCRIPTOR_RANGE_FLAGS Flags;
    UINT OffsetInDescriptorsFromTableStart;
  } 	D3D12_DESCRIPTOR_RANGE1;

  typedef struct D3D12_ROOT_DESCRIPTOR_TABLE1
  {
    UINT NumDescriptorRanges;
    _Field_size_full_(NumDescriptorRanges)  const D3D12_DESCRIPTOR_RANGE1 *pDescriptorRanges;
  } 	D3D12_ROOT_DESCRIPTOR_TABLE1;

  typedef 
    enum D3D12_ROOT_DESCRIPTOR_FLAGS
  {
    D3D12_ROOT_DESCRIPTOR_FLAG_NONE	= 0,
    D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE	= 0x2,
    D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE	= 0x4,
    D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC	= 0x8
  } 	D3D12_ROOT_DESCRIPTOR_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_ROOT_DESCRIPTOR_FLAGS );
  typedef struct D3D12_ROOT_DESCRIPTOR1
  {
    UINT ShaderRegister;
    UINT RegisterSpace;
    D3D12_ROOT_DESCRIPTOR_FLAGS Flags;
  } 	D3D12_ROOT_DESCRIPTOR1;

  typedef struct D3D12_ROOT_PARAMETER1
  {
    D3D12_ROOT_PARAMETER_TYPE ParameterType;
    union 
    {
      D3D12_ROOT_DESCRIPTOR_TABLE1 DescriptorTable;
      D3D12_ROOT_CONSTANTS Constants;
      D3D12_ROOT_DESCRIPTOR1 Descriptor;
    } 	;
    D3D12_SHADER_VISIBILITY ShaderVisibility;
  } 	D3D12_ROOT_PARAMETER1;

  typedef struct D3D12_ROOT_SIGNATURE_DESC1
  {
    UINT NumParameters;
    _Field_size_full_(NumParameters)  const D3D12_ROOT_PARAMETER1 *pParameters;
    UINT NumStaticSamplers;
    _Field_size_full_(NumStaticSamplers)  const D3D12_STATIC_SAMPLER_DESC *pStaticSamplers;
    D3D12_ROOT_SIGNATURE_FLAGS Flags;
  } 	D3D12_ROOT_SIGNATURE_DESC1;

  typedef struct D3D12_VERSIONED_ROOT_SIGNATURE_DESC
  {
    D3D_ROOT_SIGNATURE_VERSION Version;
    union 
    {
      D3D12_ROOT_SIGNATURE_DESC Desc_1_0;
      D3D12_ROOT_SIGNATURE_DESC1 Desc_1_1;
    } 	;
  } 	D3D12_VERSIONED_ROOT_SIGNATURE_DESC;



  extern RPC_IF_HANDLE __MIDL_itf_d3d12_0000_0001_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_d3d12_0000_0001_v0_0_s_ifspec;

#ifndef __ID3D12RootSignatureDeserializer_INTERFACE_DEFINED__
#define __ID3D12RootSignatureDeserializer_INTERFACE_DEFINED__

  /* interface ID3D12RootSignatureDeserializer */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12RootSignatureDeserializer;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("34AB647B-3CC8-46AC-841B-C0965645C046")
    ID3D12RootSignatureDeserializer : public IUnknown
  {
  public:
    virtual const D3D12_ROOT_SIGNATURE_DESC *STDMETHODCALLTYPE GetRootSignatureDesc( void) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12RootSignatureDeserializerVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12RootSignatureDeserializer * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12RootSignatureDeserializer * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12RootSignatureDeserializer * This);

    const D3D12_ROOT_SIGNATURE_DESC *( STDMETHODCALLTYPE *GetRootSignatureDesc )( 
      ID3D12RootSignatureDeserializer * This);

    END_INTERFACE
  } ID3D12RootSignatureDeserializerVtbl;

  interface ID3D12RootSignatureDeserializer
  {
    CONST_VTBL struct ID3D12RootSignatureDeserializerVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12RootSignatureDeserializer_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12RootSignatureDeserializer_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12RootSignatureDeserializer_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12RootSignatureDeserializer_GetRootSignatureDesc(This)	\
    ( (This)->lpVtbl -> GetRootSignatureDesc(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12RootSignatureDeserializer_INTERFACE_DEFINED__ */


#ifndef __ID3D12VersionedRootSignatureDeserializer_INTERFACE_DEFINED__
#define __ID3D12VersionedRootSignatureDeserializer_INTERFACE_DEFINED__

  /* interface ID3D12VersionedRootSignatureDeserializer */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12VersionedRootSignatureDeserializer;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("7F91CE67-090C-4BB7-B78E-ED8FF2E31DA0")
    ID3D12VersionedRootSignatureDeserializer : public IUnknown
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetRootSignatureDescAtVersion( 
      D3D_ROOT_SIGNATURE_VERSION convertToVersion,
      _Out_  const D3D12_VERSIONED_ROOT_SIGNATURE_DESC **ppDesc) = 0;

    virtual const D3D12_VERSIONED_ROOT_SIGNATURE_DESC *STDMETHODCALLTYPE GetUnconvertedRootSignatureDesc( void) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12VersionedRootSignatureDeserializerVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12VersionedRootSignatureDeserializer * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12VersionedRootSignatureDeserializer * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12VersionedRootSignatureDeserializer * This);

    HRESULT ( STDMETHODCALLTYPE *GetRootSignatureDescAtVersion )( 
      ID3D12VersionedRootSignatureDeserializer * This,
      D3D_ROOT_SIGNATURE_VERSION convertToVersion,
      _Out_  const D3D12_VERSIONED_ROOT_SIGNATURE_DESC **ppDesc);

    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC *( STDMETHODCALLTYPE *GetUnconvertedRootSignatureDesc )( 
      ID3D12VersionedRootSignatureDeserializer * This);

    END_INTERFACE
  } ID3D12VersionedRootSignatureDeserializerVtbl;

  interface ID3D12VersionedRootSignatureDeserializer
  {
    CONST_VTBL struct ID3D12VersionedRootSignatureDeserializerVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12VersionedRootSignatureDeserializer_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12VersionedRootSignatureDeserializer_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12VersionedRootSignatureDeserializer_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12VersionedRootSignatureDeserializer_GetRootSignatureDescAtVersion(This,convertToVersion,ppDesc)	\
    ( (This)->lpVtbl -> GetRootSignatureDescAtVersion(This,convertToVersion,ppDesc) ) 

#define ID3D12VersionedRootSignatureDeserializer_GetUnconvertedRootSignatureDesc(This)	\
    ( (This)->lpVtbl -> GetUnconvertedRootSignatureDesc(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12VersionedRootSignatureDeserializer_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_d3d12_0000_0003 */
  /* [local] */ 

  typedef HRESULT (WINAPI* PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)(
    _In_ const D3D12_ROOT_SIGNATURE_DESC* pRootSignature,
    _In_ D3D_ROOT_SIGNATURE_VERSION Version,
    _Out_ ID3DBlob** ppBlob,
    _Always_(_Outptr_opt_result_maybenull_) ID3DBlob** ppErrorBlob);

  HRESULT WINAPI D3D12SerializeRootSignature(
    _In_ const D3D12_ROOT_SIGNATURE_DESC* pRootSignature,
    _In_ D3D_ROOT_SIGNATURE_VERSION Version,
    _Out_ ID3DBlob** ppBlob,
    _Always_(_Outptr_opt_result_maybenull_) ID3DBlob** ppErrorBlob);

  typedef HRESULT (WINAPI* PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER)(
    _In_reads_bytes_(SrcDataSizeInBytes) LPCVOID pSrcData,
    _In_ SIZE_T SrcDataSizeInBytes,
    _In_ REFIID pRootSignatureDeserializerInterface,
    _Out_ void** ppRootSignatureDeserializer);

  HRESULT WINAPI D3D12CreateRootSignatureDeserializer(
    _In_reads_bytes_(SrcDataSizeInBytes) LPCVOID pSrcData,
    _In_ SIZE_T SrcDataSizeInBytes,
    _In_ REFIID pRootSignatureDeserializerInterface,
    _Out_ void** ppRootSignatureDeserializer);

  typedef HRESULT (WINAPI* PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE)(
    _In_ const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pRootSignature,
    _Out_ ID3DBlob** ppBlob,
    _Always_(_Outptr_opt_result_maybenull_) ID3DBlob** ppErrorBlob);

  HRESULT WINAPI D3D12SerializeVersionedRootSignature(
    _In_ const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pRootSignature,
    _Out_ ID3DBlob** ppBlob,
    _Always_(_Outptr_opt_result_maybenull_) ID3DBlob** ppErrorBlob);

  typedef HRESULT (WINAPI* PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER)(
    _In_reads_bytes_(SrcDataSizeInBytes) LPCVOID pSrcData,
    _In_ SIZE_T SrcDataSizeInBytes,
    _In_ REFIID pRootSignatureDeserializerInterface,
    _Out_ void** ppRootSignatureDeserializer);

  HRESULT WINAPI D3D12CreateVersionedRootSignatureDeserializer(
    _In_reads_bytes_(SrcDataSizeInBytes) LPCVOID pSrcData,
    _In_ SIZE_T SrcDataSizeInBytes,
    _In_ REFIID pRootSignatureDeserializerInterface,
    _Out_ void** ppRootSignatureDeserializer);

  typedef struct D3D12_CPU_DESCRIPTOR_HANDLE
  {
    SIZE_T ptr;
  } 	D3D12_CPU_DESCRIPTOR_HANDLE;

  typedef struct D3D12_GPU_DESCRIPTOR_HANDLE
  {
    UINT64 ptr;
  } 	D3D12_GPU_DESCRIPTOR_HANDLE;

  // If rects are supplied in D3D12_DISCARD_REGION, below, the resource 
  // must have 2D subresources with all specified subresources the same dimension.
  typedef struct D3D12_DISCARD_REGION
  {
    UINT NumRects;
    _In_reads_(NumRects)  const D3D12_RECT *pRects;
    UINT FirstSubresource;
    UINT NumSubresources;
  } 	D3D12_DISCARD_REGION;

  typedef 
    enum D3D12_QUERY_HEAP_TYPE
  {
    D3D12_QUERY_HEAP_TYPE_OCCLUSION	= 0,
    D3D12_QUERY_HEAP_TYPE_TIMESTAMP	= 1,
    D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS	= 2,
    D3D12_QUERY_HEAP_TYPE_SO_STATISTICS	= 3,
    D3D12_QUERY_HEAP_TYPE_VIDEO_DECODE_STATISTICS	= 4,
    D3D12_QUERY_HEAP_TYPE_COPY_QUEUE_TIMESTAMP	= 5
  } 	D3D12_QUERY_HEAP_TYPE;

  typedef struct D3D12_QUERY_HEAP_DESC
  {
    D3D12_QUERY_HEAP_TYPE Type;
    UINT Count;
    UINT NodeMask;
  } 	D3D12_QUERY_HEAP_DESC;

  typedef 
    enum D3D12_QUERY_TYPE
  {
    D3D12_QUERY_TYPE_OCCLUSION	= 0,
    D3D12_QUERY_TYPE_BINARY_OCCLUSION	= 1,
    D3D12_QUERY_TYPE_TIMESTAMP	= 2,
    D3D12_QUERY_TYPE_PIPELINE_STATISTICS	= 3,
    D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0	= 4,
    D3D12_QUERY_TYPE_SO_STATISTICS_STREAM1	= 5,
    D3D12_QUERY_TYPE_SO_STATISTICS_STREAM2	= 6,
    D3D12_QUERY_TYPE_SO_STATISTICS_STREAM3	= 7,
    D3D12_QUERY_TYPE_VIDEO_DECODE_STATISTICS	= 8
  } 	D3D12_QUERY_TYPE;

  typedef 
    enum D3D12_PREDICATION_OP
  {
    D3D12_PREDICATION_OP_EQUAL_ZERO	= 0,
    D3D12_PREDICATION_OP_NOT_EQUAL_ZERO	= 1
  } 	D3D12_PREDICATION_OP;

  typedef struct D3D12_QUERY_DATA_PIPELINE_STATISTICS
  {
    UINT64 IAVertices;
    UINT64 IAPrimitives;
    UINT64 VSInvocations;
    UINT64 GSInvocations;
    UINT64 GSPrimitives;
    UINT64 CInvocations;
    UINT64 CPrimitives;
    UINT64 PSInvocations;
    UINT64 HSInvocations;
    UINT64 DSInvocations;
    UINT64 CSInvocations;
  } 	D3D12_QUERY_DATA_PIPELINE_STATISTICS;

  typedef struct D3D12_QUERY_DATA_SO_STATISTICS
  {
    UINT64 NumPrimitivesWritten;
    UINT64 PrimitivesStorageNeeded;
  } 	D3D12_QUERY_DATA_SO_STATISTICS;

  typedef struct D3D12_STREAM_OUTPUT_BUFFER_VIEW
  {
    D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
    UINT64 SizeInBytes;
    D3D12_GPU_VIRTUAL_ADDRESS BufferFilledSizeLocation;
  } 	D3D12_STREAM_OUTPUT_BUFFER_VIEW;

  typedef struct D3D12_DRAW_ARGUMENTS
  {
    UINT VertexCountPerInstance;
    UINT InstanceCount;
    UINT StartVertexLocation;
    UINT StartInstanceLocation;
  } 	D3D12_DRAW_ARGUMENTS;

  typedef struct D3D12_DRAW_INDEXED_ARGUMENTS
  {
    UINT IndexCountPerInstance;
    UINT InstanceCount;
    UINT StartIndexLocation;
    INT BaseVertexLocation;
    UINT StartInstanceLocation;
  } 	D3D12_DRAW_INDEXED_ARGUMENTS;

  typedef struct D3D12_DISPATCH_ARGUMENTS
  {
    UINT ThreadGroupCountX;
    UINT ThreadGroupCountY;
    UINT ThreadGroupCountZ;
  } 	D3D12_DISPATCH_ARGUMENTS;

  typedef struct D3D12_VERTEX_BUFFER_VIEW
  {
    D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
    UINT SizeInBytes;
    UINT StrideInBytes;
  } 	D3D12_VERTEX_BUFFER_VIEW;

  typedef struct D3D12_INDEX_BUFFER_VIEW
  {
    D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
    UINT SizeInBytes;
    DXGI_FORMAT Format;
  } 	D3D12_INDEX_BUFFER_VIEW;

  typedef 
    enum D3D12_INDIRECT_ARGUMENT_TYPE
  {
    D3D12_INDIRECT_ARGUMENT_TYPE_DRAW	= 0,
    D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED	= ( D3D12_INDIRECT_ARGUMENT_TYPE_DRAW + 1 ) ,
    D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH	= ( D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED + 1 ) ,
    D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW	= ( D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH + 1 ) ,
    D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW	= ( D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW + 1 ) ,
    D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT	= ( D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW + 1 ) ,
    D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW	= ( D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT + 1 ) ,
    D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW	= ( D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW + 1 ) ,
    D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW	= ( D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW + 1 ) 
  } 	D3D12_INDIRECT_ARGUMENT_TYPE;

  typedef struct D3D12_INDIRECT_ARGUMENT_DESC
  {
    D3D12_INDIRECT_ARGUMENT_TYPE Type;
    union 
    {
      struct 
      {
        UINT Slot;
      } 	VertexBuffer;
      struct 
      {
        UINT RootParameterIndex;
        UINT DestOffsetIn32BitValues;
        UINT Num32BitValuesToSet;
      } 	Constant;
      struct 
      {
        UINT RootParameterIndex;
      } 	ConstantBufferView;
      struct 
      {
        UINT RootParameterIndex;
      } 	ShaderResourceView;
      struct 
      {
        UINT RootParameterIndex;
      } 	UnorderedAccessView;
    } 	;
  } 	D3D12_INDIRECT_ARGUMENT_DESC;

  typedef struct D3D12_COMMAND_SIGNATURE_DESC
  {
    UINT ByteStride;
    UINT NumArgumentDescs;
    _Field_size_full_(NumArgumentDescs)  const D3D12_INDIRECT_ARGUMENT_DESC *pArgumentDescs;
    UINT NodeMask;
  } 	D3D12_COMMAND_SIGNATURE_DESC;




  extern RPC_IF_HANDLE __MIDL_itf_d3d12_0000_0003_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_d3d12_0000_0003_v0_0_s_ifspec;

#ifndef __ID3D12Pageable_INTERFACE_DEFINED__
#define __ID3D12Pageable_INTERFACE_DEFINED__

  /* interface ID3D12Pageable */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12Pageable;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("63ee58fb-1268-4835-86da-f008ce62f0d6")
    ID3D12Pageable : public ID3D12DeviceChild
  {
  public:
  };


#else 	/* C style interface */

  typedef struct ID3D12PageableVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12Pageable * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12Pageable * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12Pageable * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12Pageable * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12Pageable * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12Pageable * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12Pageable * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12Pageable * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    END_INTERFACE
  } ID3D12PageableVtbl;

  interface ID3D12Pageable
  {
    CONST_VTBL struct ID3D12PageableVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12Pageable_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12Pageable_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12Pageable_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12Pageable_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12Pageable_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12Pageable_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12Pageable_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12Pageable_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12Pageable_INTERFACE_DEFINED__ */


#ifndef __ID3D12Heap_INTERFACE_DEFINED__
#define __ID3D12Heap_INTERFACE_DEFINED__

  /* interface ID3D12Heap */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12Heap;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("6b3b2502-6e51-45b3-90ee-9884265e8df3")
    ID3D12Heap : public ID3D12Pageable
  {
  public:
    virtual D3D12_HEAP_DESC STDMETHODCALLTYPE GetDesc( void) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12HeapVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12Heap * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12Heap * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12Heap * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12Heap * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12Heap * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12Heap * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12Heap * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12Heap * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    D3D12_HEAP_DESC ( STDMETHODCALLTYPE *GetDesc )( 
      ID3D12Heap * This);

    END_INTERFACE
  } ID3D12HeapVtbl;

  interface ID3D12Heap
  {
    CONST_VTBL struct ID3D12HeapVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12Heap_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12Heap_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12Heap_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12Heap_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12Heap_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12Heap_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12Heap_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12Heap_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 



#define ID3D12Heap_GetDesc(This)	\
    ( (This)->lpVtbl -> GetDesc(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12Heap_INTERFACE_DEFINED__ */


#ifndef __ID3D12Resource_INTERFACE_DEFINED__
#define __ID3D12Resource_INTERFACE_DEFINED__

  /* interface ID3D12Resource */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12Resource;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("696442be-a72e-4059-bc79-5b5c98040fad")
    ID3D12Resource : public ID3D12Pageable
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE Map( 
      UINT Subresource,
      _In_opt_  const D3D12_RANGE *pReadRange,
      _Outptr_opt_result_bytebuffer_(_Inexpressible_("Dependent on resource"))  void **ppData) = 0;

    virtual void STDMETHODCALLTYPE Unmap( 
      UINT Subresource,
      _In_opt_  const D3D12_RANGE *pWrittenRange) = 0;

    virtual D3D12_RESOURCE_DESC STDMETHODCALLTYPE GetDesc( void) = 0;

    virtual D3D12_GPU_VIRTUAL_ADDRESS STDMETHODCALLTYPE GetGPUVirtualAddress( void) = 0;

    virtual HRESULT STDMETHODCALLTYPE WriteToSubresource( 
      UINT DstSubresource,
      _In_opt_  const D3D12_BOX *pDstBox,
      _In_  const void *pSrcData,
      UINT SrcRowPitch,
      UINT SrcDepthPitch) = 0;

    virtual HRESULT STDMETHODCALLTYPE ReadFromSubresource( 
      _Out_  void *pDstData,
      UINT DstRowPitch,
      UINT DstDepthPitch,
      UINT SrcSubresource,
      _In_opt_  const D3D12_BOX *pSrcBox) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetHeapProperties( 
      _Out_opt_  D3D12_HEAP_PROPERTIES *pHeapProperties,
      _Out_opt_  D3D12_HEAP_FLAGS *pHeapFlags) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12ResourceVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12Resource * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12Resource * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12Resource * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12Resource * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12Resource * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12Resource * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12Resource * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12Resource * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    HRESULT ( STDMETHODCALLTYPE *Map )( 
      ID3D12Resource * This,
      UINT Subresource,
      _In_opt_  const D3D12_RANGE *pReadRange,
      _Outptr_opt_result_bytebuffer_(_Inexpressible_("Dependent on resource"))  void **ppData);

    void ( STDMETHODCALLTYPE *Unmap )( 
      ID3D12Resource * This,
      UINT Subresource,
      _In_opt_  const D3D12_RANGE *pWrittenRange);

    D3D12_RESOURCE_DESC ( STDMETHODCALLTYPE *GetDesc )( 
      ID3D12Resource * This);

    D3D12_GPU_VIRTUAL_ADDRESS ( STDMETHODCALLTYPE *GetGPUVirtualAddress )( 
      ID3D12Resource * This);

    HRESULT ( STDMETHODCALLTYPE *WriteToSubresource )( 
      ID3D12Resource * This,
      UINT DstSubresource,
      _In_opt_  const D3D12_BOX *pDstBox,
      _In_  const void *pSrcData,
      UINT SrcRowPitch,
      UINT SrcDepthPitch);

    HRESULT ( STDMETHODCALLTYPE *ReadFromSubresource )( 
      ID3D12Resource * This,
      _Out_  void *pDstData,
      UINT DstRowPitch,
      UINT DstDepthPitch,
      UINT SrcSubresource,
      _In_opt_  const D3D12_BOX *pSrcBox);

    HRESULT ( STDMETHODCALLTYPE *GetHeapProperties )( 
      ID3D12Resource * This,
      _Out_opt_  D3D12_HEAP_PROPERTIES *pHeapProperties,
      _Out_opt_  D3D12_HEAP_FLAGS *pHeapFlags);

    END_INTERFACE
  } ID3D12ResourceVtbl;

  interface ID3D12Resource
  {
    CONST_VTBL struct ID3D12ResourceVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12Resource_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12Resource_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12Resource_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12Resource_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12Resource_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12Resource_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12Resource_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12Resource_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 



#define ID3D12Resource_Map(This,Subresource,pReadRange,ppData)	\
    ( (This)->lpVtbl -> Map(This,Subresource,pReadRange,ppData) ) 

#define ID3D12Resource_Unmap(This,Subresource,pWrittenRange)	\
    ( (This)->lpVtbl -> Unmap(This,Subresource,pWrittenRange) ) 

#define ID3D12Resource_GetDesc(This)	\
    ( (This)->lpVtbl -> GetDesc(This) ) 

#define ID3D12Resource_GetGPUVirtualAddress(This)	\
    ( (This)->lpVtbl -> GetGPUVirtualAddress(This) ) 

#define ID3D12Resource_WriteToSubresource(This,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch)	\
    ( (This)->lpVtbl -> WriteToSubresource(This,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch) ) 

#define ID3D12Resource_ReadFromSubresource(This,pDstData,DstRowPitch,DstDepthPitch,SrcSubresource,pSrcBox)	\
    ( (This)->lpVtbl -> ReadFromSubresource(This,pDstData,DstRowPitch,DstDepthPitch,SrcSubresource,pSrcBox) ) 

#define ID3D12Resource_GetHeapProperties(This,pHeapProperties,pHeapFlags)	\
    ( (This)->lpVtbl -> GetHeapProperties(This,pHeapProperties,pHeapFlags) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12Resource_INTERFACE_DEFINED__ */


#ifndef __ID3D12CommandAllocator_INTERFACE_DEFINED__
#define __ID3D12CommandAllocator_INTERFACE_DEFINED__

  /* interface ID3D12CommandAllocator */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12CommandAllocator;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("6102dee4-af59-4b09-b999-b44d73f09b24")
    ID3D12CommandAllocator : public ID3D12Pageable
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12CommandAllocatorVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12CommandAllocator * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12CommandAllocator * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12CommandAllocator * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12CommandAllocator * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12CommandAllocator * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12CommandAllocator * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12CommandAllocator * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12CommandAllocator * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    HRESULT ( STDMETHODCALLTYPE *Reset )( 
      ID3D12CommandAllocator * This);

    END_INTERFACE
  } ID3D12CommandAllocatorVtbl;

  interface ID3D12CommandAllocator
  {
    CONST_VTBL struct ID3D12CommandAllocatorVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12CommandAllocator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12CommandAllocator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12CommandAllocator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12CommandAllocator_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12CommandAllocator_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12CommandAllocator_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12CommandAllocator_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12CommandAllocator_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 



#define ID3D12CommandAllocator_Reset(This)	\
    ( (This)->lpVtbl -> Reset(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12CommandAllocator_INTERFACE_DEFINED__ */


#ifndef __ID3D12Fence_INTERFACE_DEFINED__
#define __ID3D12Fence_INTERFACE_DEFINED__

  /* interface ID3D12Fence */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12Fence;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("0a753dcf-c4d8-4b91-adf6-be5a60d95a76")
    ID3D12Fence : public ID3D12Pageable
  {
  public:
    virtual UINT64 STDMETHODCALLTYPE GetCompletedValue( void) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetEventOnCompletion( 
      UINT64 Value,
      HANDLE hEvent) = 0;

    virtual HRESULT STDMETHODCALLTYPE Signal( 
      UINT64 Value) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12FenceVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12Fence * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12Fence * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12Fence * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12Fence * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12Fence * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12Fence * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12Fence * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12Fence * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    UINT64 ( STDMETHODCALLTYPE *GetCompletedValue )( 
      ID3D12Fence * This);

    HRESULT ( STDMETHODCALLTYPE *SetEventOnCompletion )( 
      ID3D12Fence * This,
      UINT64 Value,
      HANDLE hEvent);

    HRESULT ( STDMETHODCALLTYPE *Signal )( 
      ID3D12Fence * This,
      UINT64 Value);

    END_INTERFACE
  } ID3D12FenceVtbl;

  interface ID3D12Fence
  {
    CONST_VTBL struct ID3D12FenceVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12Fence_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12Fence_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12Fence_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12Fence_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12Fence_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12Fence_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12Fence_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12Fence_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 



#define ID3D12Fence_GetCompletedValue(This)	\
    ( (This)->lpVtbl -> GetCompletedValue(This) ) 

#define ID3D12Fence_SetEventOnCompletion(This,Value,hEvent)	\
    ( (This)->lpVtbl -> SetEventOnCompletion(This,Value,hEvent) ) 

#define ID3D12Fence_Signal(This,Value)	\
    ( (This)->lpVtbl -> Signal(This,Value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12Fence_INTERFACE_DEFINED__ */


#ifndef __ID3D12Fence1_INTERFACE_DEFINED__
#define __ID3D12Fence1_INTERFACE_DEFINED__

  /* interface ID3D12Fence1 */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12Fence1;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("433685fe-e22b-4ca0-a8db-b5b4f4dd0e4a")
    ID3D12Fence1 : public ID3D12Fence
  {
  public:
    virtual D3D12_FENCE_FLAGS STDMETHODCALLTYPE GetCreationFlags( void) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12Fence1Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12Fence1 * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12Fence1 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12Fence1 * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12Fence1 * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12Fence1 * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12Fence1 * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12Fence1 * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12Fence1 * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    UINT64 ( STDMETHODCALLTYPE *GetCompletedValue )( 
      ID3D12Fence1 * This);

    HRESULT ( STDMETHODCALLTYPE *SetEventOnCompletion )( 
      ID3D12Fence1 * This,
      UINT64 Value,
      HANDLE hEvent);

    HRESULT ( STDMETHODCALLTYPE *Signal )( 
      ID3D12Fence1 * This,
      UINT64 Value);

    D3D12_FENCE_FLAGS ( STDMETHODCALLTYPE *GetCreationFlags )( 
      ID3D12Fence1 * This);

    END_INTERFACE
  } ID3D12Fence1Vtbl;

  interface ID3D12Fence1
  {
    CONST_VTBL struct ID3D12Fence1Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12Fence1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12Fence1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12Fence1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12Fence1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12Fence1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12Fence1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12Fence1_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12Fence1_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 



#define ID3D12Fence1_GetCompletedValue(This)	\
    ( (This)->lpVtbl -> GetCompletedValue(This) ) 

#define ID3D12Fence1_SetEventOnCompletion(This,Value,hEvent)	\
    ( (This)->lpVtbl -> SetEventOnCompletion(This,Value,hEvent) ) 

#define ID3D12Fence1_Signal(This,Value)	\
    ( (This)->lpVtbl -> Signal(This,Value) ) 


#define ID3D12Fence1_GetCreationFlags(This)	\
    ( (This)->lpVtbl -> GetCreationFlags(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12Fence1_INTERFACE_DEFINED__ */


#ifndef __ID3D12PipelineState_INTERFACE_DEFINED__
#define __ID3D12PipelineState_INTERFACE_DEFINED__

  /* interface ID3D12PipelineState */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12PipelineState;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("765a30f3-f624-4c6f-a828-ace948622445")
    ID3D12PipelineState : public ID3D12Pageable
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetCachedBlob( 
      _COM_Outptr_  ID3DBlob **ppBlob) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12PipelineStateVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12PipelineState * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12PipelineState * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12PipelineState * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12PipelineState * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12PipelineState * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12PipelineState * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12PipelineState * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12PipelineState * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    HRESULT ( STDMETHODCALLTYPE *GetCachedBlob )( 
      ID3D12PipelineState * This,
      _COM_Outptr_  ID3DBlob **ppBlob);

    END_INTERFACE
  } ID3D12PipelineStateVtbl;

  interface ID3D12PipelineState
  {
    CONST_VTBL struct ID3D12PipelineStateVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12PipelineState_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12PipelineState_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12PipelineState_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12PipelineState_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12PipelineState_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12PipelineState_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12PipelineState_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12PipelineState_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 



#define ID3D12PipelineState_GetCachedBlob(This,ppBlob)	\
    ( (This)->lpVtbl -> GetCachedBlob(This,ppBlob) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12PipelineState_INTERFACE_DEFINED__ */


#ifndef __ID3D12DescriptorHeap_INTERFACE_DEFINED__
#define __ID3D12DescriptorHeap_INTERFACE_DEFINED__

  /* interface ID3D12DescriptorHeap */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12DescriptorHeap;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("8efb471d-616c-4f49-90f7-127bb763fa51")
    ID3D12DescriptorHeap : public ID3D12Pageable
  {
  public:
    virtual D3D12_DESCRIPTOR_HEAP_DESC STDMETHODCALLTYPE GetDesc( void) = 0;

    virtual D3D12_CPU_DESCRIPTOR_HANDLE STDMETHODCALLTYPE GetCPUDescriptorHandleForHeapStart( void) = 0;

    virtual D3D12_GPU_DESCRIPTOR_HANDLE STDMETHODCALLTYPE GetGPUDescriptorHandleForHeapStart( void) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12DescriptorHeapVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12DescriptorHeap * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12DescriptorHeap * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12DescriptorHeap * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12DescriptorHeap * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12DescriptorHeap * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12DescriptorHeap * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12DescriptorHeap * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12DescriptorHeap * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    D3D12_DESCRIPTOR_HEAP_DESC ( STDMETHODCALLTYPE *GetDesc )( 
      ID3D12DescriptorHeap * This);

    D3D12_CPU_DESCRIPTOR_HANDLE ( STDMETHODCALLTYPE *GetCPUDescriptorHandleForHeapStart )( 
      ID3D12DescriptorHeap * This);

    D3D12_GPU_DESCRIPTOR_HANDLE ( STDMETHODCALLTYPE *GetGPUDescriptorHandleForHeapStart )( 
      ID3D12DescriptorHeap * This);

    END_INTERFACE
  } ID3D12DescriptorHeapVtbl;

  interface ID3D12DescriptorHeap
  {
    CONST_VTBL struct ID3D12DescriptorHeapVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12DescriptorHeap_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12DescriptorHeap_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12DescriptorHeap_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12DescriptorHeap_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12DescriptorHeap_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12DescriptorHeap_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12DescriptorHeap_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12DescriptorHeap_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 



#define ID3D12DescriptorHeap_GetDesc(This)	\
    ( (This)->lpVtbl -> GetDesc(This) ) 

#define ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(This)	\
    ( (This)->lpVtbl -> GetCPUDescriptorHandleForHeapStart(This) ) 

#define ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(This)	\
    ( (This)->lpVtbl -> GetGPUDescriptorHandleForHeapStart(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12DescriptorHeap_INTERFACE_DEFINED__ */


#ifndef __ID3D12QueryHeap_INTERFACE_DEFINED__
#define __ID3D12QueryHeap_INTERFACE_DEFINED__

  /* interface ID3D12QueryHeap */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12QueryHeap;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("0d9658ae-ed45-469e-a61d-970ec583cab4")
    ID3D12QueryHeap : public ID3D12Pageable
  {
  public:
  };


#else 	/* C style interface */

  typedef struct ID3D12QueryHeapVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12QueryHeap * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12QueryHeap * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12QueryHeap * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12QueryHeap * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12QueryHeap * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12QueryHeap * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12QueryHeap * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12QueryHeap * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    END_INTERFACE
  } ID3D12QueryHeapVtbl;

  interface ID3D12QueryHeap
  {
    CONST_VTBL struct ID3D12QueryHeapVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12QueryHeap_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12QueryHeap_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12QueryHeap_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12QueryHeap_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12QueryHeap_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12QueryHeap_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12QueryHeap_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12QueryHeap_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 



#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12QueryHeap_INTERFACE_DEFINED__ */


#ifndef __ID3D12CommandSignature_INTERFACE_DEFINED__
#define __ID3D12CommandSignature_INTERFACE_DEFINED__

  /* interface ID3D12CommandSignature */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12CommandSignature;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("c36a797c-ec80-4f0a-8985-a7b2475082d1")
    ID3D12CommandSignature : public ID3D12Pageable
  {
  public:
  };


#else 	/* C style interface */

  typedef struct ID3D12CommandSignatureVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12CommandSignature * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12CommandSignature * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12CommandSignature * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12CommandSignature * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12CommandSignature * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12CommandSignature * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12CommandSignature * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12CommandSignature * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    END_INTERFACE
  } ID3D12CommandSignatureVtbl;

  interface ID3D12CommandSignature
  {
    CONST_VTBL struct ID3D12CommandSignatureVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12CommandSignature_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12CommandSignature_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12CommandSignature_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12CommandSignature_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12CommandSignature_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12CommandSignature_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12CommandSignature_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12CommandSignature_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 



#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12CommandSignature_INTERFACE_DEFINED__ */


#ifndef __ID3D12CommandList_INTERFACE_DEFINED__
#define __ID3D12CommandList_INTERFACE_DEFINED__

  /* interface ID3D12CommandList */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12CommandList;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("7116d91c-e7e4-47ce-b8c6-ec8168f437e5")
    ID3D12CommandList : public ID3D12DeviceChild
  {
  public:
    virtual D3D12_COMMAND_LIST_TYPE STDMETHODCALLTYPE GetType( void) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12CommandListVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12CommandList * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12CommandList * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12CommandList * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12CommandList * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12CommandList * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12CommandList * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12CommandList * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12CommandList * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    D3D12_COMMAND_LIST_TYPE ( STDMETHODCALLTYPE *GetType )( 
      ID3D12CommandList * This);

    END_INTERFACE
  } ID3D12CommandListVtbl;

  interface ID3D12CommandList
  {
    CONST_VTBL struct ID3D12CommandListVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12CommandList_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12CommandList_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12CommandList_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12CommandList_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12CommandList_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12CommandList_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12CommandList_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12CommandList_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 


#define ID3D12CommandList_GetType(This)	\
    ( (This)->lpVtbl -> GetType(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12CommandList_INTERFACE_DEFINED__ */


#ifndef __ID3D12GraphicsCommandList_INTERFACE_DEFINED__
#define __ID3D12GraphicsCommandList_INTERFACE_DEFINED__

  /* interface ID3D12GraphicsCommandList */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12GraphicsCommandList;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("5b160d0f-ac1b-4185-8ba8-b3ae42a5a455")
    ID3D12GraphicsCommandList : public ID3D12CommandList
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE Close( void) = 0;

    virtual HRESULT STDMETHODCALLTYPE Reset( 
      _In_  ID3D12CommandAllocator *pAllocator,
      _In_opt_  ID3D12PipelineState *pInitialState) = 0;

    virtual void STDMETHODCALLTYPE ClearState( 
      _In_opt_  ID3D12PipelineState *pPipelineState) = 0;

    virtual void STDMETHODCALLTYPE DrawInstanced( 
      _In_  UINT VertexCountPerInstance,
      _In_  UINT InstanceCount,
      _In_  UINT StartVertexLocation,
      _In_  UINT StartInstanceLocation) = 0;

    virtual void STDMETHODCALLTYPE DrawIndexedInstanced( 
      _In_  UINT IndexCountPerInstance,
      _In_  UINT InstanceCount,
      _In_  UINT StartIndexLocation,
      _In_  INT BaseVertexLocation,
      _In_  UINT StartInstanceLocation) = 0;

    virtual void STDMETHODCALLTYPE Dispatch( 
      _In_  UINT ThreadGroupCountX,
      _In_  UINT ThreadGroupCountY,
      _In_  UINT ThreadGroupCountZ) = 0;

    virtual void STDMETHODCALLTYPE CopyBufferRegion( 
      _In_  ID3D12Resource *pDstBuffer,
      UINT64 DstOffset,
      _In_  ID3D12Resource *pSrcBuffer,
      UINT64 SrcOffset,
      UINT64 NumBytes) = 0;

    virtual void STDMETHODCALLTYPE CopyTextureRegion( 
      _In_  const D3D12_TEXTURE_COPY_LOCATION *pDst,
      UINT DstX,
      UINT DstY,
      UINT DstZ,
      _In_  const D3D12_TEXTURE_COPY_LOCATION *pSrc,
      _In_opt_  const D3D12_BOX *pSrcBox) = 0;

    virtual void STDMETHODCALLTYPE CopyResource( 
      _In_  ID3D12Resource *pDstResource,
      _In_  ID3D12Resource *pSrcResource) = 0;

    virtual void STDMETHODCALLTYPE CopyTiles( 
      _In_  ID3D12Resource *pTiledResource,
      _In_  const D3D12_TILED_RESOURCE_COORDINATE *pTileRegionStartCoordinate,
      _In_  const D3D12_TILE_REGION_SIZE *pTileRegionSize,
      _In_  ID3D12Resource *pBuffer,
      UINT64 BufferStartOffsetInBytes,
      D3D12_TILE_COPY_FLAGS Flags) = 0;

    virtual void STDMETHODCALLTYPE ResolveSubresource( 
      _In_  ID3D12Resource *pDstResource,
      _In_  UINT DstSubresource,
      _In_  ID3D12Resource *pSrcResource,
      _In_  UINT SrcSubresource,
      _In_  DXGI_FORMAT Format) = 0;

    virtual void STDMETHODCALLTYPE IASetPrimitiveTopology( 
      _In_  D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology) = 0;

    virtual void STDMETHODCALLTYPE RSSetViewports( 
      _In_range_(0, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumViewports,
      _In_reads_( NumViewports)  const D3D12_VIEWPORT *pViewports) = 0;

    virtual void STDMETHODCALLTYPE RSSetScissorRects( 
      _In_range_(0, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumRects,
      _In_reads_( NumRects)  const D3D12_RECT *pRects) = 0;

    virtual void STDMETHODCALLTYPE OMSetBlendFactor( 
      _In_reads_opt_(4)  const FLOAT BlendFactor[ 4 ]) = 0;

    virtual void STDMETHODCALLTYPE OMSetStencilRef( 
      _In_  UINT StencilRef) = 0;

    virtual void STDMETHODCALLTYPE SetPipelineState( 
      _In_  ID3D12PipelineState *pPipelineState) = 0;

    virtual void STDMETHODCALLTYPE ResourceBarrier( 
      _In_  UINT NumBarriers,
      _In_reads_(NumBarriers)  const D3D12_RESOURCE_BARRIER *pBarriers) = 0;

    virtual void STDMETHODCALLTYPE ExecuteBundle( 
      _In_  ID3D12GraphicsCommandList *pCommandList) = 0;

    virtual void STDMETHODCALLTYPE SetDescriptorHeaps( 
      _In_  UINT NumDescriptorHeaps,
      _In_reads_(NumDescriptorHeaps)  ID3D12DescriptorHeap *const *ppDescriptorHeaps) = 0;

    virtual void STDMETHODCALLTYPE SetComputeRootSignature( 
      _In_opt_  ID3D12RootSignature *pRootSignature) = 0;

    virtual void STDMETHODCALLTYPE SetGraphicsRootSignature( 
      _In_opt_  ID3D12RootSignature *pRootSignature) = 0;

    virtual void STDMETHODCALLTYPE SetComputeRootDescriptorTable( 
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor) = 0;

    virtual void STDMETHODCALLTYPE SetGraphicsRootDescriptorTable( 
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor) = 0;

    virtual void STDMETHODCALLTYPE SetComputeRoot32BitConstant( 
      _In_  UINT RootParameterIndex,
      _In_  UINT SrcData,
      _In_  UINT DestOffsetIn32BitValues) = 0;

    virtual void STDMETHODCALLTYPE SetGraphicsRoot32BitConstant( 
      _In_  UINT RootParameterIndex,
      _In_  UINT SrcData,
      _In_  UINT DestOffsetIn32BitValues) = 0;

    virtual void STDMETHODCALLTYPE SetComputeRoot32BitConstants( 
      _In_  UINT RootParameterIndex,
      _In_  UINT Num32BitValuesToSet,
      _In_reads_(Num32BitValuesToSet*sizeof(UINT))  const void *pSrcData,
      _In_  UINT DestOffsetIn32BitValues) = 0;

    virtual void STDMETHODCALLTYPE SetGraphicsRoot32BitConstants( 
      _In_  UINT RootParameterIndex,
      _In_  UINT Num32BitValuesToSet,
      _In_reads_(Num32BitValuesToSet*sizeof(UINT))  const void *pSrcData,
      _In_  UINT DestOffsetIn32BitValues) = 0;

    virtual void STDMETHODCALLTYPE SetComputeRootConstantBufferView( 
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) = 0;

    virtual void STDMETHODCALLTYPE SetGraphicsRootConstantBufferView( 
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) = 0;

    virtual void STDMETHODCALLTYPE SetComputeRootShaderResourceView( 
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) = 0;

    virtual void STDMETHODCALLTYPE SetGraphicsRootShaderResourceView( 
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) = 0;

    virtual void STDMETHODCALLTYPE SetComputeRootUnorderedAccessView( 
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) = 0;

    virtual void STDMETHODCALLTYPE SetGraphicsRootUnorderedAccessView( 
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) = 0;

    virtual void STDMETHODCALLTYPE IASetIndexBuffer( 
      _In_opt_  const D3D12_INDEX_BUFFER_VIEW *pView) = 0;

    virtual void STDMETHODCALLTYPE IASetVertexBuffers( 
      _In_  UINT StartSlot,
      _In_  UINT NumViews,
      _In_reads_opt_(NumViews)  const D3D12_VERTEX_BUFFER_VIEW *pViews) = 0;

    virtual void STDMETHODCALLTYPE SOSetTargets( 
      _In_  UINT StartSlot,
      _In_  UINT NumViews,
      _In_reads_opt_(NumViews)  const D3D12_STREAM_OUTPUT_BUFFER_VIEW *pViews) = 0;

    virtual void STDMETHODCALLTYPE OMSetRenderTargets( 
      _In_  UINT NumRenderTargetDescriptors,
      _In_opt_  const D3D12_CPU_DESCRIPTOR_HANDLE *pRenderTargetDescriptors,
      _In_  BOOL RTsSingleHandleToDescriptorRange,
      _In_opt_  const D3D12_CPU_DESCRIPTOR_HANDLE *pDepthStencilDescriptor) = 0;

    virtual void STDMETHODCALLTYPE ClearDepthStencilView( 
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView,
      _In_  D3D12_CLEAR_FLAGS ClearFlags,
      _In_  FLOAT Depth,
      _In_  UINT8 Stencil,
      _In_  UINT NumRects,
      _In_reads_(NumRects)  const D3D12_RECT *pRects) = 0;

    virtual void STDMETHODCALLTYPE ClearRenderTargetView( 
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView,
      _In_  const FLOAT ColorRGBA[ 4 ],
      _In_  UINT NumRects,
      _In_reads_(NumRects)  const D3D12_RECT *pRects) = 0;

    virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewUint( 
      _In_  D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle,
      _In_  ID3D12Resource *pResource,
      _In_  const UINT Values[ 4 ],
      _In_  UINT NumRects,
      _In_reads_(NumRects)  const D3D12_RECT *pRects) = 0;

    virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewFloat( 
      _In_  D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle,
      _In_  ID3D12Resource *pResource,
      _In_  const FLOAT Values[ 4 ],
      _In_  UINT NumRects,
      _In_reads_(NumRects)  const D3D12_RECT *pRects) = 0;

    virtual void STDMETHODCALLTYPE DiscardResource( 
      _In_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_DISCARD_REGION *pRegion) = 0;

    virtual void STDMETHODCALLTYPE BeginQuery( 
      _In_  ID3D12QueryHeap *pQueryHeap,
      _In_  D3D12_QUERY_TYPE Type,
      _In_  UINT Index) = 0;

    virtual void STDMETHODCALLTYPE EndQuery( 
      _In_  ID3D12QueryHeap *pQueryHeap,
      _In_  D3D12_QUERY_TYPE Type,
      _In_  UINT Index) = 0;

    virtual void STDMETHODCALLTYPE ResolveQueryData( 
      _In_  ID3D12QueryHeap *pQueryHeap,
      _In_  D3D12_QUERY_TYPE Type,
      _In_  UINT StartIndex,
      _In_  UINT NumQueries,
      _In_  ID3D12Resource *pDestinationBuffer,
      _In_  UINT64 AlignedDestinationBufferOffset) = 0;

    virtual void STDMETHODCALLTYPE SetPredication( 
      _In_opt_  ID3D12Resource *pBuffer,
      _In_  UINT64 AlignedBufferOffset,
      _In_  D3D12_PREDICATION_OP Operation) = 0;

    virtual void STDMETHODCALLTYPE SetMarker( 
      UINT Metadata,
      _In_reads_bytes_opt_(Size)  const void *pData,
      UINT Size) = 0;

    virtual void STDMETHODCALLTYPE BeginEvent( 
      UINT Metadata,
      _In_reads_bytes_opt_(Size)  const void *pData,
      UINT Size) = 0;

    virtual void STDMETHODCALLTYPE EndEvent( void) = 0;

    virtual void STDMETHODCALLTYPE ExecuteIndirect( 
      _In_  ID3D12CommandSignature *pCommandSignature,
      _In_  UINT MaxCommandCount,
      _In_  ID3D12Resource *pArgumentBuffer,
      _In_  UINT64 ArgumentBufferOffset,
      _In_opt_  ID3D12Resource *pCountBuffer,
      _In_  UINT64 CountBufferOffset) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12GraphicsCommandListVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12GraphicsCommandList * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12GraphicsCommandList * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12GraphicsCommandList * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12GraphicsCommandList * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12GraphicsCommandList * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12GraphicsCommandList * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12GraphicsCommandList * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12GraphicsCommandList * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    D3D12_COMMAND_LIST_TYPE ( STDMETHODCALLTYPE *GetType )( 
      ID3D12GraphicsCommandList * This);

    HRESULT ( STDMETHODCALLTYPE *Close )( 
      ID3D12GraphicsCommandList * This);

    HRESULT ( STDMETHODCALLTYPE *Reset )( 
      ID3D12GraphicsCommandList * This,
      _In_  ID3D12CommandAllocator *pAllocator,
      _In_opt_  ID3D12PipelineState *pInitialState);

    void ( STDMETHODCALLTYPE *ClearState )( 
      ID3D12GraphicsCommandList * This,
      _In_opt_  ID3D12PipelineState *pPipelineState);

    void ( STDMETHODCALLTYPE *DrawInstanced )( 
      ID3D12GraphicsCommandList * This,
      _In_  UINT VertexCountPerInstance,
      _In_  UINT InstanceCount,
      _In_  UINT StartVertexLocation,
      _In_  UINT StartInstanceLocation);

    void ( STDMETHODCALLTYPE *DrawIndexedInstanced )( 
      ID3D12GraphicsCommandList * This,
      _In_  UINT IndexCountPerInstance,
      _In_  UINT InstanceCount,
      _In_  UINT StartIndexLocation,
      _In_  INT BaseVertexLocation,
      _In_  UINT StartInstanceLocation);

    void ( STDMETHODCALLTYPE *Dispatch )( 
      ID3D12GraphicsCommandList * This,
      _In_  UINT ThreadGroupCountX,
      _In_  UINT ThreadGroupCountY,
      _In_  UINT ThreadGroupCountZ);

    void ( STDMETHODCALLTYPE *CopyBufferRegion )( 
      ID3D12GraphicsCommandList * This,
      _In_  ID3D12Resource *pDstBuffer,
      UINT64 DstOffset,
      _In_  ID3D12Resource *pSrcBuffer,
      UINT64 SrcOffset,
      UINT64 NumBytes);

    void ( STDMETHODCALLTYPE *CopyTextureRegion )( 
      ID3D12GraphicsCommandList * This,
      _In_  const D3D12_TEXTURE_COPY_LOCATION *pDst,
      UINT DstX,
      UINT DstY,
      UINT DstZ,
      _In_  const D3D12_TEXTURE_COPY_LOCATION *pSrc,
      _In_opt_  const D3D12_BOX *pSrcBox);

    void ( STDMETHODCALLTYPE *CopyResource )( 
      ID3D12GraphicsCommandList * This,
      _In_  ID3D12Resource *pDstResource,
      _In_  ID3D12Resource *pSrcResource);

    void ( STDMETHODCALLTYPE *CopyTiles )( 
      ID3D12GraphicsCommandList * This,
      _In_  ID3D12Resource *pTiledResource,
      _In_  const D3D12_TILED_RESOURCE_COORDINATE *pTileRegionStartCoordinate,
      _In_  const D3D12_TILE_REGION_SIZE *pTileRegionSize,
      _In_  ID3D12Resource *pBuffer,
      UINT64 BufferStartOffsetInBytes,
      D3D12_TILE_COPY_FLAGS Flags);

    void ( STDMETHODCALLTYPE *ResolveSubresource )( 
      ID3D12GraphicsCommandList * This,
      _In_  ID3D12Resource *pDstResource,
      _In_  UINT DstSubresource,
      _In_  ID3D12Resource *pSrcResource,
      _In_  UINT SrcSubresource,
      _In_  DXGI_FORMAT Format);

    void ( STDMETHODCALLTYPE *IASetPrimitiveTopology )( 
      ID3D12GraphicsCommandList * This,
      _In_  D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology);

    void ( STDMETHODCALLTYPE *RSSetViewports )( 
      ID3D12GraphicsCommandList * This,
      _In_range_(0, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumViewports,
      _In_reads_( NumViewports)  const D3D12_VIEWPORT *pViewports);

    void ( STDMETHODCALLTYPE *RSSetScissorRects )( 
      ID3D12GraphicsCommandList * This,
      _In_range_(0, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumRects,
      _In_reads_( NumRects)  const D3D12_RECT *pRects);

    void ( STDMETHODCALLTYPE *OMSetBlendFactor )( 
      ID3D12GraphicsCommandList * This,
      _In_reads_opt_(4)  const FLOAT BlendFactor[ 4 ]);

    void ( STDMETHODCALLTYPE *OMSetStencilRef )( 
      ID3D12GraphicsCommandList * This,
      _In_  UINT StencilRef);

    void ( STDMETHODCALLTYPE *SetPipelineState )( 
      ID3D12GraphicsCommandList * This,
      _In_  ID3D12PipelineState *pPipelineState);

    void ( STDMETHODCALLTYPE *ResourceBarrier )( 
      ID3D12GraphicsCommandList * This,
      _In_  UINT NumBarriers,
      _In_reads_(NumBarriers)  const D3D12_RESOURCE_BARRIER *pBarriers);

    void ( STDMETHODCALLTYPE *ExecuteBundle )( 
      ID3D12GraphicsCommandList * This,
      _In_  ID3D12GraphicsCommandList *pCommandList);

    void ( STDMETHODCALLTYPE *SetDescriptorHeaps )( 
      ID3D12GraphicsCommandList * This,
      _In_  UINT NumDescriptorHeaps,
      _In_reads_(NumDescriptorHeaps)  ID3D12DescriptorHeap *const *ppDescriptorHeaps);

    void ( STDMETHODCALLTYPE *SetComputeRootSignature )( 
      ID3D12GraphicsCommandList * This,
      _In_opt_  ID3D12RootSignature *pRootSignature);

    void ( STDMETHODCALLTYPE *SetGraphicsRootSignature )( 
      ID3D12GraphicsCommandList * This,
      _In_opt_  ID3D12RootSignature *pRootSignature);

    void ( STDMETHODCALLTYPE *SetComputeRootDescriptorTable )( 
      ID3D12GraphicsCommandList * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);

    void ( STDMETHODCALLTYPE *SetGraphicsRootDescriptorTable )( 
      ID3D12GraphicsCommandList * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);

    void ( STDMETHODCALLTYPE *SetComputeRoot32BitConstant )( 
      ID3D12GraphicsCommandList * This,
      _In_  UINT RootParameterIndex,
      _In_  UINT SrcData,
      _In_  UINT DestOffsetIn32BitValues);

    void ( STDMETHODCALLTYPE *SetGraphicsRoot32BitConstant )( 
      ID3D12GraphicsCommandList * This,
      _In_  UINT RootParameterIndex,
      _In_  UINT SrcData,
      _In_  UINT DestOffsetIn32BitValues);

    void ( STDMETHODCALLTYPE *SetComputeRoot32BitConstants )( 
      ID3D12GraphicsCommandList * This,
      _In_  UINT RootParameterIndex,
      _In_  UINT Num32BitValuesToSet,
      _In_reads_(Num32BitValuesToSet*sizeof(UINT))  const void *pSrcData,
      _In_  UINT DestOffsetIn32BitValues);

    void ( STDMETHODCALLTYPE *SetGraphicsRoot32BitConstants )( 
      ID3D12GraphicsCommandList * This,
      _In_  UINT RootParameterIndex,
      _In_  UINT Num32BitValuesToSet,
      _In_reads_(Num32BitValuesToSet*sizeof(UINT))  const void *pSrcData,
      _In_  UINT DestOffsetIn32BitValues);

    void ( STDMETHODCALLTYPE *SetComputeRootConstantBufferView )( 
      ID3D12GraphicsCommandList * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *SetGraphicsRootConstantBufferView )( 
      ID3D12GraphicsCommandList * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *SetComputeRootShaderResourceView )( 
      ID3D12GraphicsCommandList * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *SetGraphicsRootShaderResourceView )( 
      ID3D12GraphicsCommandList * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *SetComputeRootUnorderedAccessView )( 
      ID3D12GraphicsCommandList * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *SetGraphicsRootUnorderedAccessView )( 
      ID3D12GraphicsCommandList * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *IASetIndexBuffer )( 
      ID3D12GraphicsCommandList * This,
      _In_opt_  const D3D12_INDEX_BUFFER_VIEW *pView);

    void ( STDMETHODCALLTYPE *IASetVertexBuffers )( 
      ID3D12GraphicsCommandList * This,
      _In_  UINT StartSlot,
      _In_  UINT NumViews,
      _In_reads_opt_(NumViews)  const D3D12_VERTEX_BUFFER_VIEW *pViews);

    void ( STDMETHODCALLTYPE *SOSetTargets )( 
      ID3D12GraphicsCommandList * This,
      _In_  UINT StartSlot,
      _In_  UINT NumViews,
      _In_reads_opt_(NumViews)  const D3D12_STREAM_OUTPUT_BUFFER_VIEW *pViews);

    void ( STDMETHODCALLTYPE *OMSetRenderTargets )( 
      ID3D12GraphicsCommandList * This,
      _In_  UINT NumRenderTargetDescriptors,
      _In_opt_  const D3D12_CPU_DESCRIPTOR_HANDLE *pRenderTargetDescriptors,
      _In_  BOOL RTsSingleHandleToDescriptorRange,
      _In_opt_  const D3D12_CPU_DESCRIPTOR_HANDLE *pDepthStencilDescriptor);

    void ( STDMETHODCALLTYPE *ClearDepthStencilView )( 
      ID3D12GraphicsCommandList * This,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView,
      _In_  D3D12_CLEAR_FLAGS ClearFlags,
      _In_  FLOAT Depth,
      _In_  UINT8 Stencil,
      _In_  UINT NumRects,
      _In_reads_(NumRects)  const D3D12_RECT *pRects);

    void ( STDMETHODCALLTYPE *ClearRenderTargetView )( 
      ID3D12GraphicsCommandList * This,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView,
      _In_  const FLOAT ColorRGBA[ 4 ],
      _In_  UINT NumRects,
      _In_reads_(NumRects)  const D3D12_RECT *pRects);

    void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewUint )( 
      ID3D12GraphicsCommandList * This,
      _In_  D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle,
      _In_  ID3D12Resource *pResource,
      _In_  const UINT Values[ 4 ],
      _In_  UINT NumRects,
      _In_reads_(NumRects)  const D3D12_RECT *pRects);

    void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewFloat )( 
      ID3D12GraphicsCommandList * This,
      _In_  D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle,
      _In_  ID3D12Resource *pResource,
      _In_  const FLOAT Values[ 4 ],
      _In_  UINT NumRects,
      _In_reads_(NumRects)  const D3D12_RECT *pRects);

    void ( STDMETHODCALLTYPE *DiscardResource )( 
      ID3D12GraphicsCommandList * This,
      _In_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_DISCARD_REGION *pRegion);

    void ( STDMETHODCALLTYPE *BeginQuery )( 
      ID3D12GraphicsCommandList * This,
      _In_  ID3D12QueryHeap *pQueryHeap,
      _In_  D3D12_QUERY_TYPE Type,
      _In_  UINT Index);

    void ( STDMETHODCALLTYPE *EndQuery )( 
      ID3D12GraphicsCommandList * This,
      _In_  ID3D12QueryHeap *pQueryHeap,
      _In_  D3D12_QUERY_TYPE Type,
      _In_  UINT Index);

    void ( STDMETHODCALLTYPE *ResolveQueryData )( 
      ID3D12GraphicsCommandList * This,
      _In_  ID3D12QueryHeap *pQueryHeap,
      _In_  D3D12_QUERY_TYPE Type,
      _In_  UINT StartIndex,
      _In_  UINT NumQueries,
      _In_  ID3D12Resource *pDestinationBuffer,
      _In_  UINT64 AlignedDestinationBufferOffset);

    void ( STDMETHODCALLTYPE *SetPredication )( 
      ID3D12GraphicsCommandList * This,
      _In_opt_  ID3D12Resource *pBuffer,
      _In_  UINT64 AlignedBufferOffset,
      _In_  D3D12_PREDICATION_OP Operation);

    void ( STDMETHODCALLTYPE *SetMarker )( 
      ID3D12GraphicsCommandList * This,
      UINT Metadata,
      _In_reads_bytes_opt_(Size)  const void *pData,
      UINT Size);

    void ( STDMETHODCALLTYPE *BeginEvent )( 
      ID3D12GraphicsCommandList * This,
      UINT Metadata,
      _In_reads_bytes_opt_(Size)  const void *pData,
      UINT Size);

    void ( STDMETHODCALLTYPE *EndEvent )( 
      ID3D12GraphicsCommandList * This);

    void ( STDMETHODCALLTYPE *ExecuteIndirect )( 
      ID3D12GraphicsCommandList * This,
      _In_  ID3D12CommandSignature *pCommandSignature,
      _In_  UINT MaxCommandCount,
      _In_  ID3D12Resource *pArgumentBuffer,
      _In_  UINT64 ArgumentBufferOffset,
      _In_opt_  ID3D12Resource *pCountBuffer,
      _In_  UINT64 CountBufferOffset);

    END_INTERFACE
  } ID3D12GraphicsCommandListVtbl;

  interface ID3D12GraphicsCommandList
  {
    CONST_VTBL struct ID3D12GraphicsCommandListVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12GraphicsCommandList_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12GraphicsCommandList_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12GraphicsCommandList_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12GraphicsCommandList_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12GraphicsCommandList_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12GraphicsCommandList_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12GraphicsCommandList_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12GraphicsCommandList_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 


#define ID3D12GraphicsCommandList_GetType(This)	\
    ( (This)->lpVtbl -> GetType(This) ) 


#define ID3D12GraphicsCommandList_Close(This)	\
    ( (This)->lpVtbl -> Close(This) ) 

#define ID3D12GraphicsCommandList_Reset(This,pAllocator,pInitialState)	\
    ( (This)->lpVtbl -> Reset(This,pAllocator,pInitialState) ) 

#define ID3D12GraphicsCommandList_ClearState(This,pPipelineState)	\
    ( (This)->lpVtbl -> ClearState(This,pPipelineState) ) 

#define ID3D12GraphicsCommandList_DrawInstanced(This,VertexCountPerInstance,InstanceCount,StartVertexLocation,StartInstanceLocation)	\
    ( (This)->lpVtbl -> DrawInstanced(This,VertexCountPerInstance,InstanceCount,StartVertexLocation,StartInstanceLocation) ) 

#define ID3D12GraphicsCommandList_DrawIndexedInstanced(This,IndexCountPerInstance,InstanceCount,StartIndexLocation,BaseVertexLocation,StartInstanceLocation)	\
    ( (This)->lpVtbl -> DrawIndexedInstanced(This,IndexCountPerInstance,InstanceCount,StartIndexLocation,BaseVertexLocation,StartInstanceLocation) ) 

#define ID3D12GraphicsCommandList_Dispatch(This,ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ)	\
    ( (This)->lpVtbl -> Dispatch(This,ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ) ) 

#define ID3D12GraphicsCommandList_CopyBufferRegion(This,pDstBuffer,DstOffset,pSrcBuffer,SrcOffset,NumBytes)	\
    ( (This)->lpVtbl -> CopyBufferRegion(This,pDstBuffer,DstOffset,pSrcBuffer,SrcOffset,NumBytes) ) 

#define ID3D12GraphicsCommandList_CopyTextureRegion(This,pDst,DstX,DstY,DstZ,pSrc,pSrcBox)	\
    ( (This)->lpVtbl -> CopyTextureRegion(This,pDst,DstX,DstY,DstZ,pSrc,pSrcBox) ) 

#define ID3D12GraphicsCommandList_CopyResource(This,pDstResource,pSrcResource)	\
    ( (This)->lpVtbl -> CopyResource(This,pDstResource,pSrcResource) ) 

#define ID3D12GraphicsCommandList_CopyTiles(This,pTiledResource,pTileRegionStartCoordinate,pTileRegionSize,pBuffer,BufferStartOffsetInBytes,Flags)	\
    ( (This)->lpVtbl -> CopyTiles(This,pTiledResource,pTileRegionStartCoordinate,pTileRegionSize,pBuffer,BufferStartOffsetInBytes,Flags) ) 

#define ID3D12GraphicsCommandList_ResolveSubresource(This,pDstResource,DstSubresource,pSrcResource,SrcSubresource,Format)	\
    ( (This)->lpVtbl -> ResolveSubresource(This,pDstResource,DstSubresource,pSrcResource,SrcSubresource,Format) ) 

#define ID3D12GraphicsCommandList_IASetPrimitiveTopology(This,PrimitiveTopology)	\
    ( (This)->lpVtbl -> IASetPrimitiveTopology(This,PrimitiveTopology) ) 

#define ID3D12GraphicsCommandList_RSSetViewports(This,NumViewports,pViewports)	\
    ( (This)->lpVtbl -> RSSetViewports(This,NumViewports,pViewports) ) 

#define ID3D12GraphicsCommandList_RSSetScissorRects(This,NumRects,pRects)	\
    ( (This)->lpVtbl -> RSSetScissorRects(This,NumRects,pRects) ) 

#define ID3D12GraphicsCommandList_OMSetBlendFactor(This,BlendFactor)	\
    ( (This)->lpVtbl -> OMSetBlendFactor(This,BlendFactor) ) 

#define ID3D12GraphicsCommandList_OMSetStencilRef(This,StencilRef)	\
    ( (This)->lpVtbl -> OMSetStencilRef(This,StencilRef) ) 

#define ID3D12GraphicsCommandList_SetPipelineState(This,pPipelineState)	\
    ( (This)->lpVtbl -> SetPipelineState(This,pPipelineState) ) 

#define ID3D12GraphicsCommandList_ResourceBarrier(This,NumBarriers,pBarriers)	\
    ( (This)->lpVtbl -> ResourceBarrier(This,NumBarriers,pBarriers) ) 

#define ID3D12GraphicsCommandList_ExecuteBundle(This,pCommandList)	\
    ( (This)->lpVtbl -> ExecuteBundle(This,pCommandList) ) 

#define ID3D12GraphicsCommandList_SetDescriptorHeaps(This,NumDescriptorHeaps,ppDescriptorHeaps)	\
    ( (This)->lpVtbl -> SetDescriptorHeaps(This,NumDescriptorHeaps,ppDescriptorHeaps) ) 

#define ID3D12GraphicsCommandList_SetComputeRootSignature(This,pRootSignature)	\
    ( (This)->lpVtbl -> SetComputeRootSignature(This,pRootSignature) ) 

#define ID3D12GraphicsCommandList_SetGraphicsRootSignature(This,pRootSignature)	\
    ( (This)->lpVtbl -> SetGraphicsRootSignature(This,pRootSignature) ) 

#define ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(This,RootParameterIndex,BaseDescriptor)	\
    ( (This)->lpVtbl -> SetComputeRootDescriptorTable(This,RootParameterIndex,BaseDescriptor) ) 

#define ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(This,RootParameterIndex,BaseDescriptor)	\
    ( (This)->lpVtbl -> SetGraphicsRootDescriptorTable(This,RootParameterIndex,BaseDescriptor) ) 

#define ID3D12GraphicsCommandList_SetComputeRoot32BitConstant(This,RootParameterIndex,SrcData,DestOffsetIn32BitValues)	\
    ( (This)->lpVtbl -> SetComputeRoot32BitConstant(This,RootParameterIndex,SrcData,DestOffsetIn32BitValues) ) 

#define ID3D12GraphicsCommandList_SetGraphicsRoot32BitConstant(This,RootParameterIndex,SrcData,DestOffsetIn32BitValues)	\
    ( (This)->lpVtbl -> SetGraphicsRoot32BitConstant(This,RootParameterIndex,SrcData,DestOffsetIn32BitValues) ) 

#define ID3D12GraphicsCommandList_SetComputeRoot32BitConstants(This,RootParameterIndex,Num32BitValuesToSet,pSrcData,DestOffsetIn32BitValues)	\
    ( (This)->lpVtbl -> SetComputeRoot32BitConstants(This,RootParameterIndex,Num32BitValuesToSet,pSrcData,DestOffsetIn32BitValues) ) 

#define ID3D12GraphicsCommandList_SetGraphicsRoot32BitConstants(This,RootParameterIndex,Num32BitValuesToSet,pSrcData,DestOffsetIn32BitValues)	\
    ( (This)->lpVtbl -> SetGraphicsRoot32BitConstants(This,RootParameterIndex,Num32BitValuesToSet,pSrcData,DestOffsetIn32BitValues) ) 

#define ID3D12GraphicsCommandList_SetComputeRootConstantBufferView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetComputeRootConstantBufferView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList_SetGraphicsRootConstantBufferView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetGraphicsRootConstantBufferView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList_SetComputeRootShaderResourceView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetComputeRootShaderResourceView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList_SetGraphicsRootShaderResourceView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetGraphicsRootShaderResourceView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList_SetComputeRootUnorderedAccessView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetComputeRootUnorderedAccessView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList_SetGraphicsRootUnorderedAccessView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetGraphicsRootUnorderedAccessView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList_IASetIndexBuffer(This,pView)	\
    ( (This)->lpVtbl -> IASetIndexBuffer(This,pView) ) 

#define ID3D12GraphicsCommandList_IASetVertexBuffers(This,StartSlot,NumViews,pViews)	\
    ( (This)->lpVtbl -> IASetVertexBuffers(This,StartSlot,NumViews,pViews) ) 

#define ID3D12GraphicsCommandList_SOSetTargets(This,StartSlot,NumViews,pViews)	\
    ( (This)->lpVtbl -> SOSetTargets(This,StartSlot,NumViews,pViews) ) 

#define ID3D12GraphicsCommandList_OMSetRenderTargets(This,NumRenderTargetDescriptors,pRenderTargetDescriptors,RTsSingleHandleToDescriptorRange,pDepthStencilDescriptor)	\
    ( (This)->lpVtbl -> OMSetRenderTargets(This,NumRenderTargetDescriptors,pRenderTargetDescriptors,RTsSingleHandleToDescriptorRange,pDepthStencilDescriptor) ) 

#define ID3D12GraphicsCommandList_ClearDepthStencilView(This,DepthStencilView,ClearFlags,Depth,Stencil,NumRects,pRects)	\
    ( (This)->lpVtbl -> ClearDepthStencilView(This,DepthStencilView,ClearFlags,Depth,Stencil,NumRects,pRects) ) 

#define ID3D12GraphicsCommandList_ClearRenderTargetView(This,RenderTargetView,ColorRGBA,NumRects,pRects)	\
    ( (This)->lpVtbl -> ClearRenderTargetView(This,RenderTargetView,ColorRGBA,NumRects,pRects) ) 

#define ID3D12GraphicsCommandList_ClearUnorderedAccessViewUint(This,ViewGPUHandleInCurrentHeap,ViewCPUHandle,pResource,Values,NumRects,pRects)	\
    ( (This)->lpVtbl -> ClearUnorderedAccessViewUint(This,ViewGPUHandleInCurrentHeap,ViewCPUHandle,pResource,Values,NumRects,pRects) ) 

#define ID3D12GraphicsCommandList_ClearUnorderedAccessViewFloat(This,ViewGPUHandleInCurrentHeap,ViewCPUHandle,pResource,Values,NumRects,pRects)	\
    ( (This)->lpVtbl -> ClearUnorderedAccessViewFloat(This,ViewGPUHandleInCurrentHeap,ViewCPUHandle,pResource,Values,NumRects,pRects) ) 

#define ID3D12GraphicsCommandList_DiscardResource(This,pResource,pRegion)	\
    ( (This)->lpVtbl -> DiscardResource(This,pResource,pRegion) ) 

#define ID3D12GraphicsCommandList_BeginQuery(This,pQueryHeap,Type,Index)	\
    ( (This)->lpVtbl -> BeginQuery(This,pQueryHeap,Type,Index) ) 

#define ID3D12GraphicsCommandList_EndQuery(This,pQueryHeap,Type,Index)	\
    ( (This)->lpVtbl -> EndQuery(This,pQueryHeap,Type,Index) ) 

#define ID3D12GraphicsCommandList_ResolveQueryData(This,pQueryHeap,Type,StartIndex,NumQueries,pDestinationBuffer,AlignedDestinationBufferOffset)	\
    ( (This)->lpVtbl -> ResolveQueryData(This,pQueryHeap,Type,StartIndex,NumQueries,pDestinationBuffer,AlignedDestinationBufferOffset) ) 

#define ID3D12GraphicsCommandList_SetPredication(This,pBuffer,AlignedBufferOffset,Operation)	\
    ( (This)->lpVtbl -> SetPredication(This,pBuffer,AlignedBufferOffset,Operation) ) 

#define ID3D12GraphicsCommandList_SetMarker(This,Metadata,pData,Size)	\
    ( (This)->lpVtbl -> SetMarker(This,Metadata,pData,Size) ) 

#define ID3D12GraphicsCommandList_BeginEvent(This,Metadata,pData,Size)	\
    ( (This)->lpVtbl -> BeginEvent(This,Metadata,pData,Size) ) 

#define ID3D12GraphicsCommandList_EndEvent(This)	\
    ( (This)->lpVtbl -> EndEvent(This) ) 

#define ID3D12GraphicsCommandList_ExecuteIndirect(This,pCommandSignature,MaxCommandCount,pArgumentBuffer,ArgumentBufferOffset,pCountBuffer,CountBufferOffset)	\
    ( (This)->lpVtbl -> ExecuteIndirect(This,pCommandSignature,MaxCommandCount,pArgumentBuffer,ArgumentBufferOffset,pCountBuffer,CountBufferOffset) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12GraphicsCommandList_INTERFACE_DEFINED__ */


#ifndef __ID3D12GraphicsCommandList1_INTERFACE_DEFINED__
#define __ID3D12GraphicsCommandList1_INTERFACE_DEFINED__

  /* interface ID3D12GraphicsCommandList1 */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12GraphicsCommandList1;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("553103fb-1fe7-4557-bb38-946d7d0e7ca7")
    ID3D12GraphicsCommandList1 : public ID3D12GraphicsCommandList
  {
  public:
    virtual void STDMETHODCALLTYPE AtomicCopyBufferUINT( 
      _In_  ID3D12Resource *pDstBuffer,
      UINT64 DstOffset,
      _In_  ID3D12Resource *pSrcBuffer,
      UINT64 SrcOffset,
      UINT Dependencies,
      _In_reads_(Dependencies)  ID3D12Resource *const *ppDependentResources,
      _In_reads_(Dependencies)  const D3D12_SUBRESOURCE_RANGE_UINT64 *pDependentSubresourceRanges) = 0;

    virtual void STDMETHODCALLTYPE AtomicCopyBufferUINT64( 
      _In_  ID3D12Resource *pDstBuffer,
      UINT64 DstOffset,
      _In_  ID3D12Resource *pSrcBuffer,
      UINT64 SrcOffset,
      UINT Dependencies,
      _In_reads_(Dependencies)  ID3D12Resource *const *ppDependentResources,
      _In_reads_(Dependencies)  const D3D12_SUBRESOURCE_RANGE_UINT64 *pDependentSubresourceRanges) = 0;

    virtual void STDMETHODCALLTYPE OMSetDepthBounds( 
      _In_  FLOAT Min,
      _In_  FLOAT Max) = 0;

    virtual void STDMETHODCALLTYPE SetSamplePositions( 
      _In_  UINT NumSamplesPerPixel,
      _In_  UINT NumPixels,
      _In_reads_(NumSamplesPerPixel*NumPixels)  D3D12_SAMPLE_POSITION *pSamplePositions) = 0;

    virtual void STDMETHODCALLTYPE ResolveSubresourceRegion( 
      _In_  ID3D12Resource *pDstResource,
      _In_  UINT DstSubresource,
      _In_  UINT DstX,
      _In_  UINT DstY,
      _In_  ID3D12Resource *pSrcResource,
      _In_  UINT SrcSubresource,
      _In_opt_  D3D12_RECT *pSrcRect,
      _In_  DXGI_FORMAT Format,
      _In_  D3D12_RESOLVE_MODE ResolveMode) = 0;

    virtual void STDMETHODCALLTYPE SetViewInstanceMask( 
      _In_  UINT Mask) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12GraphicsCommandList1Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12GraphicsCommandList1 * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12GraphicsCommandList1 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12GraphicsCommandList1 * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12GraphicsCommandList1 * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12GraphicsCommandList1 * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    D3D12_COMMAND_LIST_TYPE ( STDMETHODCALLTYPE *GetType )( 
      ID3D12GraphicsCommandList1 * This);

    HRESULT ( STDMETHODCALLTYPE *Close )( 
      ID3D12GraphicsCommandList1 * This);

    HRESULT ( STDMETHODCALLTYPE *Reset )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  ID3D12CommandAllocator *pAllocator,
      _In_opt_  ID3D12PipelineState *pInitialState);

    void ( STDMETHODCALLTYPE *ClearState )( 
      ID3D12GraphicsCommandList1 * This,
      _In_opt_  ID3D12PipelineState *pPipelineState);

    void ( STDMETHODCALLTYPE *DrawInstanced )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT VertexCountPerInstance,
      _In_  UINT InstanceCount,
      _In_  UINT StartVertexLocation,
      _In_  UINT StartInstanceLocation);

    void ( STDMETHODCALLTYPE *DrawIndexedInstanced )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT IndexCountPerInstance,
      _In_  UINT InstanceCount,
      _In_  UINT StartIndexLocation,
      _In_  INT BaseVertexLocation,
      _In_  UINT StartInstanceLocation);

    void ( STDMETHODCALLTYPE *Dispatch )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT ThreadGroupCountX,
      _In_  UINT ThreadGroupCountY,
      _In_  UINT ThreadGroupCountZ);

    void ( STDMETHODCALLTYPE *CopyBufferRegion )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  ID3D12Resource *pDstBuffer,
      UINT64 DstOffset,
      _In_  ID3D12Resource *pSrcBuffer,
      UINT64 SrcOffset,
      UINT64 NumBytes);

    void ( STDMETHODCALLTYPE *CopyTextureRegion )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  const D3D12_TEXTURE_COPY_LOCATION *pDst,
      UINT DstX,
      UINT DstY,
      UINT DstZ,
      _In_  const D3D12_TEXTURE_COPY_LOCATION *pSrc,
      _In_opt_  const D3D12_BOX *pSrcBox);

    void ( STDMETHODCALLTYPE *CopyResource )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  ID3D12Resource *pDstResource,
      _In_  ID3D12Resource *pSrcResource);

    void ( STDMETHODCALLTYPE *CopyTiles )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  ID3D12Resource *pTiledResource,
      _In_  const D3D12_TILED_RESOURCE_COORDINATE *pTileRegionStartCoordinate,
      _In_  const D3D12_TILE_REGION_SIZE *pTileRegionSize,
      _In_  ID3D12Resource *pBuffer,
      UINT64 BufferStartOffsetInBytes,
      D3D12_TILE_COPY_FLAGS Flags);

    void ( STDMETHODCALLTYPE *ResolveSubresource )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  ID3D12Resource *pDstResource,
      _In_  UINT DstSubresource,
      _In_  ID3D12Resource *pSrcResource,
      _In_  UINT SrcSubresource,
      _In_  DXGI_FORMAT Format);

    void ( STDMETHODCALLTYPE *IASetPrimitiveTopology )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology);

    void ( STDMETHODCALLTYPE *RSSetViewports )( 
      ID3D12GraphicsCommandList1 * This,
      _In_range_(0, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumViewports,
      _In_reads_( NumViewports)  const D3D12_VIEWPORT *pViewports);

    void ( STDMETHODCALLTYPE *RSSetScissorRects )( 
      ID3D12GraphicsCommandList1 * This,
      _In_range_(0, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumRects,
      _In_reads_( NumRects)  const D3D12_RECT *pRects);

    void ( STDMETHODCALLTYPE *OMSetBlendFactor )( 
      ID3D12GraphicsCommandList1 * This,
      _In_reads_opt_(4)  const FLOAT BlendFactor[ 4 ]);

    void ( STDMETHODCALLTYPE *OMSetStencilRef )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT StencilRef);

    void ( STDMETHODCALLTYPE *SetPipelineState )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  ID3D12PipelineState *pPipelineState);

    void ( STDMETHODCALLTYPE *ResourceBarrier )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT NumBarriers,
      _In_reads_(NumBarriers)  const D3D12_RESOURCE_BARRIER *pBarriers);

    void ( STDMETHODCALLTYPE *ExecuteBundle )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  ID3D12GraphicsCommandList *pCommandList);

    void ( STDMETHODCALLTYPE *SetDescriptorHeaps )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT NumDescriptorHeaps,
      _In_reads_(NumDescriptorHeaps)  ID3D12DescriptorHeap *const *ppDescriptorHeaps);

    void ( STDMETHODCALLTYPE *SetComputeRootSignature )( 
      ID3D12GraphicsCommandList1 * This,
      _In_opt_  ID3D12RootSignature *pRootSignature);

    void ( STDMETHODCALLTYPE *SetGraphicsRootSignature )( 
      ID3D12GraphicsCommandList1 * This,
      _In_opt_  ID3D12RootSignature *pRootSignature);

    void ( STDMETHODCALLTYPE *SetComputeRootDescriptorTable )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);

    void ( STDMETHODCALLTYPE *SetGraphicsRootDescriptorTable )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);

    void ( STDMETHODCALLTYPE *SetComputeRoot32BitConstant )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT RootParameterIndex,
      _In_  UINT SrcData,
      _In_  UINT DestOffsetIn32BitValues);

    void ( STDMETHODCALLTYPE *SetGraphicsRoot32BitConstant )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT RootParameterIndex,
      _In_  UINT SrcData,
      _In_  UINT DestOffsetIn32BitValues);

    void ( STDMETHODCALLTYPE *SetComputeRoot32BitConstants )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT RootParameterIndex,
      _In_  UINT Num32BitValuesToSet,
      _In_reads_(Num32BitValuesToSet*sizeof(UINT))  const void *pSrcData,
      _In_  UINT DestOffsetIn32BitValues);

    void ( STDMETHODCALLTYPE *SetGraphicsRoot32BitConstants )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT RootParameterIndex,
      _In_  UINT Num32BitValuesToSet,
      _In_reads_(Num32BitValuesToSet*sizeof(UINT))  const void *pSrcData,
      _In_  UINT DestOffsetIn32BitValues);

    void ( STDMETHODCALLTYPE *SetComputeRootConstantBufferView )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *SetGraphicsRootConstantBufferView )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *SetComputeRootShaderResourceView )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *SetGraphicsRootShaderResourceView )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *SetComputeRootUnorderedAccessView )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *SetGraphicsRootUnorderedAccessView )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *IASetIndexBuffer )( 
      ID3D12GraphicsCommandList1 * This,
      _In_opt_  const D3D12_INDEX_BUFFER_VIEW *pView);

    void ( STDMETHODCALLTYPE *IASetVertexBuffers )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT StartSlot,
      _In_  UINT NumViews,
      _In_reads_opt_(NumViews)  const D3D12_VERTEX_BUFFER_VIEW *pViews);

    void ( STDMETHODCALLTYPE *SOSetTargets )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT StartSlot,
      _In_  UINT NumViews,
      _In_reads_opt_(NumViews)  const D3D12_STREAM_OUTPUT_BUFFER_VIEW *pViews);

    void ( STDMETHODCALLTYPE *OMSetRenderTargets )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT NumRenderTargetDescriptors,
      _In_opt_  const D3D12_CPU_DESCRIPTOR_HANDLE *pRenderTargetDescriptors,
      _In_  BOOL RTsSingleHandleToDescriptorRange,
      _In_opt_  const D3D12_CPU_DESCRIPTOR_HANDLE *pDepthStencilDescriptor);

    void ( STDMETHODCALLTYPE *ClearDepthStencilView )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView,
      _In_  D3D12_CLEAR_FLAGS ClearFlags,
      _In_  FLOAT Depth,
      _In_  UINT8 Stencil,
      _In_  UINT NumRects,
      _In_reads_(NumRects)  const D3D12_RECT *pRects);

    void ( STDMETHODCALLTYPE *ClearRenderTargetView )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView,
      _In_  const FLOAT ColorRGBA[ 4 ],
      _In_  UINT NumRects,
      _In_reads_(NumRects)  const D3D12_RECT *pRects);

    void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewUint )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle,
      _In_  ID3D12Resource *pResource,
      _In_  const UINT Values[ 4 ],
      _In_  UINT NumRects,
      _In_reads_(NumRects)  const D3D12_RECT *pRects);

    void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewFloat )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle,
      _In_  ID3D12Resource *pResource,
      _In_  const FLOAT Values[ 4 ],
      _In_  UINT NumRects,
      _In_reads_(NumRects)  const D3D12_RECT *pRects);

    void ( STDMETHODCALLTYPE *DiscardResource )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_DISCARD_REGION *pRegion);

    void ( STDMETHODCALLTYPE *BeginQuery )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  ID3D12QueryHeap *pQueryHeap,
      _In_  D3D12_QUERY_TYPE Type,
      _In_  UINT Index);

    void ( STDMETHODCALLTYPE *EndQuery )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  ID3D12QueryHeap *pQueryHeap,
      _In_  D3D12_QUERY_TYPE Type,
      _In_  UINT Index);

    void ( STDMETHODCALLTYPE *ResolveQueryData )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  ID3D12QueryHeap *pQueryHeap,
      _In_  D3D12_QUERY_TYPE Type,
      _In_  UINT StartIndex,
      _In_  UINT NumQueries,
      _In_  ID3D12Resource *pDestinationBuffer,
      _In_  UINT64 AlignedDestinationBufferOffset);

    void ( STDMETHODCALLTYPE *SetPredication )( 
      ID3D12GraphicsCommandList1 * This,
      _In_opt_  ID3D12Resource *pBuffer,
      _In_  UINT64 AlignedBufferOffset,
      _In_  D3D12_PREDICATION_OP Operation);

    void ( STDMETHODCALLTYPE *SetMarker )( 
      ID3D12GraphicsCommandList1 * This,
      UINT Metadata,
      _In_reads_bytes_opt_(Size)  const void *pData,
      UINT Size);

    void ( STDMETHODCALLTYPE *BeginEvent )( 
      ID3D12GraphicsCommandList1 * This,
      UINT Metadata,
      _In_reads_bytes_opt_(Size)  const void *pData,
      UINT Size);

    void ( STDMETHODCALLTYPE *EndEvent )( 
      ID3D12GraphicsCommandList1 * This);

    void ( STDMETHODCALLTYPE *ExecuteIndirect )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  ID3D12CommandSignature *pCommandSignature,
      _In_  UINT MaxCommandCount,
      _In_  ID3D12Resource *pArgumentBuffer,
      _In_  UINT64 ArgumentBufferOffset,
      _In_opt_  ID3D12Resource *pCountBuffer,
      _In_  UINT64 CountBufferOffset);

    void ( STDMETHODCALLTYPE *AtomicCopyBufferUINT )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  ID3D12Resource *pDstBuffer,
      UINT64 DstOffset,
      _In_  ID3D12Resource *pSrcBuffer,
      UINT64 SrcOffset,
      UINT Dependencies,
      _In_reads_(Dependencies)  ID3D12Resource *const *ppDependentResources,
      _In_reads_(Dependencies)  const D3D12_SUBRESOURCE_RANGE_UINT64 *pDependentSubresourceRanges);

    void ( STDMETHODCALLTYPE *AtomicCopyBufferUINT64 )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  ID3D12Resource *pDstBuffer,
      UINT64 DstOffset,
      _In_  ID3D12Resource *pSrcBuffer,
      UINT64 SrcOffset,
      UINT Dependencies,
      _In_reads_(Dependencies)  ID3D12Resource *const *ppDependentResources,
      _In_reads_(Dependencies)  const D3D12_SUBRESOURCE_RANGE_UINT64 *pDependentSubresourceRanges);

    void ( STDMETHODCALLTYPE *OMSetDepthBounds )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  FLOAT Min,
      _In_  FLOAT Max);

    void ( STDMETHODCALLTYPE *SetSamplePositions )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT NumSamplesPerPixel,
      _In_  UINT NumPixels,
      _In_reads_(NumSamplesPerPixel*NumPixels)  D3D12_SAMPLE_POSITION *pSamplePositions);

    void ( STDMETHODCALLTYPE *ResolveSubresourceRegion )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  ID3D12Resource *pDstResource,
      _In_  UINT DstSubresource,
      _In_  UINT DstX,
      _In_  UINT DstY,
      _In_  ID3D12Resource *pSrcResource,
      _In_  UINT SrcSubresource,
      _In_opt_  D3D12_RECT *pSrcRect,
      _In_  DXGI_FORMAT Format,
      _In_  D3D12_RESOLVE_MODE ResolveMode);

    void ( STDMETHODCALLTYPE *SetViewInstanceMask )( 
      ID3D12GraphicsCommandList1 * This,
      _In_  UINT Mask);

    END_INTERFACE
  } ID3D12GraphicsCommandList1Vtbl;

  interface ID3D12GraphicsCommandList1
  {
    CONST_VTBL struct ID3D12GraphicsCommandList1Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12GraphicsCommandList1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12GraphicsCommandList1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12GraphicsCommandList1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12GraphicsCommandList1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12GraphicsCommandList1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12GraphicsCommandList1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12GraphicsCommandList1_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12GraphicsCommandList1_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 


#define ID3D12GraphicsCommandList1_GetType(This)	\
    ( (This)->lpVtbl -> GetType(This) ) 


#define ID3D12GraphicsCommandList1_Close(This)	\
    ( (This)->lpVtbl -> Close(This) ) 

#define ID3D12GraphicsCommandList1_Reset(This,pAllocator,pInitialState)	\
    ( (This)->lpVtbl -> Reset(This,pAllocator,pInitialState) ) 

#define ID3D12GraphicsCommandList1_ClearState(This,pPipelineState)	\
    ( (This)->lpVtbl -> ClearState(This,pPipelineState) ) 

#define ID3D12GraphicsCommandList1_DrawInstanced(This,VertexCountPerInstance,InstanceCount,StartVertexLocation,StartInstanceLocation)	\
    ( (This)->lpVtbl -> DrawInstanced(This,VertexCountPerInstance,InstanceCount,StartVertexLocation,StartInstanceLocation) ) 

#define ID3D12GraphicsCommandList1_DrawIndexedInstanced(This,IndexCountPerInstance,InstanceCount,StartIndexLocation,BaseVertexLocation,StartInstanceLocation)	\
    ( (This)->lpVtbl -> DrawIndexedInstanced(This,IndexCountPerInstance,InstanceCount,StartIndexLocation,BaseVertexLocation,StartInstanceLocation) ) 

#define ID3D12GraphicsCommandList1_Dispatch(This,ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ)	\
    ( (This)->lpVtbl -> Dispatch(This,ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ) ) 

#define ID3D12GraphicsCommandList1_CopyBufferRegion(This,pDstBuffer,DstOffset,pSrcBuffer,SrcOffset,NumBytes)	\
    ( (This)->lpVtbl -> CopyBufferRegion(This,pDstBuffer,DstOffset,pSrcBuffer,SrcOffset,NumBytes) ) 

#define ID3D12GraphicsCommandList1_CopyTextureRegion(This,pDst,DstX,DstY,DstZ,pSrc,pSrcBox)	\
    ( (This)->lpVtbl -> CopyTextureRegion(This,pDst,DstX,DstY,DstZ,pSrc,pSrcBox) ) 

#define ID3D12GraphicsCommandList1_CopyResource(This,pDstResource,pSrcResource)	\
    ( (This)->lpVtbl -> CopyResource(This,pDstResource,pSrcResource) ) 

#define ID3D12GraphicsCommandList1_CopyTiles(This,pTiledResource,pTileRegionStartCoordinate,pTileRegionSize,pBuffer,BufferStartOffsetInBytes,Flags)	\
    ( (This)->lpVtbl -> CopyTiles(This,pTiledResource,pTileRegionStartCoordinate,pTileRegionSize,pBuffer,BufferStartOffsetInBytes,Flags) ) 

#define ID3D12GraphicsCommandList1_ResolveSubresource(This,pDstResource,DstSubresource,pSrcResource,SrcSubresource,Format)	\
    ( (This)->lpVtbl -> ResolveSubresource(This,pDstResource,DstSubresource,pSrcResource,SrcSubresource,Format) ) 

#define ID3D12GraphicsCommandList1_IASetPrimitiveTopology(This,PrimitiveTopology)	\
    ( (This)->lpVtbl -> IASetPrimitiveTopology(This,PrimitiveTopology) ) 

#define ID3D12GraphicsCommandList1_RSSetViewports(This,NumViewports,pViewports)	\
    ( (This)->lpVtbl -> RSSetViewports(This,NumViewports,pViewports) ) 

#define ID3D12GraphicsCommandList1_RSSetScissorRects(This,NumRects,pRects)	\
    ( (This)->lpVtbl -> RSSetScissorRects(This,NumRects,pRects) ) 

#define ID3D12GraphicsCommandList1_OMSetBlendFactor(This,BlendFactor)	\
    ( (This)->lpVtbl -> OMSetBlendFactor(This,BlendFactor) ) 

#define ID3D12GraphicsCommandList1_OMSetStencilRef(This,StencilRef)	\
    ( (This)->lpVtbl -> OMSetStencilRef(This,StencilRef) ) 

#define ID3D12GraphicsCommandList1_SetPipelineState(This,pPipelineState)	\
    ( (This)->lpVtbl -> SetPipelineState(This,pPipelineState) ) 

#define ID3D12GraphicsCommandList1_ResourceBarrier(This,NumBarriers,pBarriers)	\
    ( (This)->lpVtbl -> ResourceBarrier(This,NumBarriers,pBarriers) ) 

#define ID3D12GraphicsCommandList1_ExecuteBundle(This,pCommandList)	\
    ( (This)->lpVtbl -> ExecuteBundle(This,pCommandList) ) 

#define ID3D12GraphicsCommandList1_SetDescriptorHeaps(This,NumDescriptorHeaps,ppDescriptorHeaps)	\
    ( (This)->lpVtbl -> SetDescriptorHeaps(This,NumDescriptorHeaps,ppDescriptorHeaps) ) 

#define ID3D12GraphicsCommandList1_SetComputeRootSignature(This,pRootSignature)	\
    ( (This)->lpVtbl -> SetComputeRootSignature(This,pRootSignature) ) 

#define ID3D12GraphicsCommandList1_SetGraphicsRootSignature(This,pRootSignature)	\
    ( (This)->lpVtbl -> SetGraphicsRootSignature(This,pRootSignature) ) 

#define ID3D12GraphicsCommandList1_SetComputeRootDescriptorTable(This,RootParameterIndex,BaseDescriptor)	\
    ( (This)->lpVtbl -> SetComputeRootDescriptorTable(This,RootParameterIndex,BaseDescriptor) ) 

#define ID3D12GraphicsCommandList1_SetGraphicsRootDescriptorTable(This,RootParameterIndex,BaseDescriptor)	\
    ( (This)->lpVtbl -> SetGraphicsRootDescriptorTable(This,RootParameterIndex,BaseDescriptor) ) 

#define ID3D12GraphicsCommandList1_SetComputeRoot32BitConstant(This,RootParameterIndex,SrcData,DestOffsetIn32BitValues)	\
    ( (This)->lpVtbl -> SetComputeRoot32BitConstant(This,RootParameterIndex,SrcData,DestOffsetIn32BitValues) ) 

#define ID3D12GraphicsCommandList1_SetGraphicsRoot32BitConstant(This,RootParameterIndex,SrcData,DestOffsetIn32BitValues)	\
    ( (This)->lpVtbl -> SetGraphicsRoot32BitConstant(This,RootParameterIndex,SrcData,DestOffsetIn32BitValues) ) 

#define ID3D12GraphicsCommandList1_SetComputeRoot32BitConstants(This,RootParameterIndex,Num32BitValuesToSet,pSrcData,DestOffsetIn32BitValues)	\
    ( (This)->lpVtbl -> SetComputeRoot32BitConstants(This,RootParameterIndex,Num32BitValuesToSet,pSrcData,DestOffsetIn32BitValues) ) 

#define ID3D12GraphicsCommandList1_SetGraphicsRoot32BitConstants(This,RootParameterIndex,Num32BitValuesToSet,pSrcData,DestOffsetIn32BitValues)	\
    ( (This)->lpVtbl -> SetGraphicsRoot32BitConstants(This,RootParameterIndex,Num32BitValuesToSet,pSrcData,DestOffsetIn32BitValues) ) 

#define ID3D12GraphicsCommandList1_SetComputeRootConstantBufferView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetComputeRootConstantBufferView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList1_SetGraphicsRootConstantBufferView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetGraphicsRootConstantBufferView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList1_SetComputeRootShaderResourceView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetComputeRootShaderResourceView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList1_SetGraphicsRootShaderResourceView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetGraphicsRootShaderResourceView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList1_SetComputeRootUnorderedAccessView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetComputeRootUnorderedAccessView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList1_SetGraphicsRootUnorderedAccessView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetGraphicsRootUnorderedAccessView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList1_IASetIndexBuffer(This,pView)	\
    ( (This)->lpVtbl -> IASetIndexBuffer(This,pView) ) 

#define ID3D12GraphicsCommandList1_IASetVertexBuffers(This,StartSlot,NumViews,pViews)	\
    ( (This)->lpVtbl -> IASetVertexBuffers(This,StartSlot,NumViews,pViews) ) 

#define ID3D12GraphicsCommandList1_SOSetTargets(This,StartSlot,NumViews,pViews)	\
    ( (This)->lpVtbl -> SOSetTargets(This,StartSlot,NumViews,pViews) ) 

#define ID3D12GraphicsCommandList1_OMSetRenderTargets(This,NumRenderTargetDescriptors,pRenderTargetDescriptors,RTsSingleHandleToDescriptorRange,pDepthStencilDescriptor)	\
    ( (This)->lpVtbl -> OMSetRenderTargets(This,NumRenderTargetDescriptors,pRenderTargetDescriptors,RTsSingleHandleToDescriptorRange,pDepthStencilDescriptor) ) 

#define ID3D12GraphicsCommandList1_ClearDepthStencilView(This,DepthStencilView,ClearFlags,Depth,Stencil,NumRects,pRects)	\
    ( (This)->lpVtbl -> ClearDepthStencilView(This,DepthStencilView,ClearFlags,Depth,Stencil,NumRects,pRects) ) 

#define ID3D12GraphicsCommandList1_ClearRenderTargetView(This,RenderTargetView,ColorRGBA,NumRects,pRects)	\
    ( (This)->lpVtbl -> ClearRenderTargetView(This,RenderTargetView,ColorRGBA,NumRects,pRects) ) 

#define ID3D12GraphicsCommandList1_ClearUnorderedAccessViewUint(This,ViewGPUHandleInCurrentHeap,ViewCPUHandle,pResource,Values,NumRects,pRects)	\
    ( (This)->lpVtbl -> ClearUnorderedAccessViewUint(This,ViewGPUHandleInCurrentHeap,ViewCPUHandle,pResource,Values,NumRects,pRects) ) 

#define ID3D12GraphicsCommandList1_ClearUnorderedAccessViewFloat(This,ViewGPUHandleInCurrentHeap,ViewCPUHandle,pResource,Values,NumRects,pRects)	\
    ( (This)->lpVtbl -> ClearUnorderedAccessViewFloat(This,ViewGPUHandleInCurrentHeap,ViewCPUHandle,pResource,Values,NumRects,pRects) ) 

#define ID3D12GraphicsCommandList1_DiscardResource(This,pResource,pRegion)	\
    ( (This)->lpVtbl -> DiscardResource(This,pResource,pRegion) ) 

#define ID3D12GraphicsCommandList1_BeginQuery(This,pQueryHeap,Type,Index)	\
    ( (This)->lpVtbl -> BeginQuery(This,pQueryHeap,Type,Index) ) 

#define ID3D12GraphicsCommandList1_EndQuery(This,pQueryHeap,Type,Index)	\
    ( (This)->lpVtbl -> EndQuery(This,pQueryHeap,Type,Index) ) 

#define ID3D12GraphicsCommandList1_ResolveQueryData(This,pQueryHeap,Type,StartIndex,NumQueries,pDestinationBuffer,AlignedDestinationBufferOffset)	\
    ( (This)->lpVtbl -> ResolveQueryData(This,pQueryHeap,Type,StartIndex,NumQueries,pDestinationBuffer,AlignedDestinationBufferOffset) ) 

#define ID3D12GraphicsCommandList1_SetPredication(This,pBuffer,AlignedBufferOffset,Operation)	\
    ( (This)->lpVtbl -> SetPredication(This,pBuffer,AlignedBufferOffset,Operation) ) 

#define ID3D12GraphicsCommandList1_SetMarker(This,Metadata,pData,Size)	\
    ( (This)->lpVtbl -> SetMarker(This,Metadata,pData,Size) ) 

#define ID3D12GraphicsCommandList1_BeginEvent(This,Metadata,pData,Size)	\
    ( (This)->lpVtbl -> BeginEvent(This,Metadata,pData,Size) ) 

#define ID3D12GraphicsCommandList1_EndEvent(This)	\
    ( (This)->lpVtbl -> EndEvent(This) ) 

#define ID3D12GraphicsCommandList1_ExecuteIndirect(This,pCommandSignature,MaxCommandCount,pArgumentBuffer,ArgumentBufferOffset,pCountBuffer,CountBufferOffset)	\
    ( (This)->lpVtbl -> ExecuteIndirect(This,pCommandSignature,MaxCommandCount,pArgumentBuffer,ArgumentBufferOffset,pCountBuffer,CountBufferOffset) ) 


#define ID3D12GraphicsCommandList1_AtomicCopyBufferUINT(This,pDstBuffer,DstOffset,pSrcBuffer,SrcOffset,Dependencies,ppDependentResources,pDependentSubresourceRanges)	\
    ( (This)->lpVtbl -> AtomicCopyBufferUINT(This,pDstBuffer,DstOffset,pSrcBuffer,SrcOffset,Dependencies,ppDependentResources,pDependentSubresourceRanges) ) 

#define ID3D12GraphicsCommandList1_AtomicCopyBufferUINT64(This,pDstBuffer,DstOffset,pSrcBuffer,SrcOffset,Dependencies,ppDependentResources,pDependentSubresourceRanges)	\
    ( (This)->lpVtbl -> AtomicCopyBufferUINT64(This,pDstBuffer,DstOffset,pSrcBuffer,SrcOffset,Dependencies,ppDependentResources,pDependentSubresourceRanges) ) 

#define ID3D12GraphicsCommandList1_OMSetDepthBounds(This,Min,Max)	\
    ( (This)->lpVtbl -> OMSetDepthBounds(This,Min,Max) ) 

#define ID3D12GraphicsCommandList1_SetSamplePositions(This,NumSamplesPerPixel,NumPixels,pSamplePositions)	\
    ( (This)->lpVtbl -> SetSamplePositions(This,NumSamplesPerPixel,NumPixels,pSamplePositions) ) 

#define ID3D12GraphicsCommandList1_ResolveSubresourceRegion(This,pDstResource,DstSubresource,DstX,DstY,pSrcResource,SrcSubresource,pSrcRect,Format,ResolveMode)	\
    ( (This)->lpVtbl -> ResolveSubresourceRegion(This,pDstResource,DstSubresource,DstX,DstY,pSrcResource,SrcSubresource,pSrcRect,Format,ResolveMode) ) 

#define ID3D12GraphicsCommandList1_SetViewInstanceMask(This,Mask)	\
    ( (This)->lpVtbl -> SetViewInstanceMask(This,Mask) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12GraphicsCommandList1_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_d3d12_0000_0018 */
  /* [local] */ 

  typedef struct D3D12_WRITEBUFFERIMMEDIATE_PARAMETER
  {
    D3D12_GPU_VIRTUAL_ADDRESS Dest;
    UINT32 Value;
  } 	D3D12_WRITEBUFFERIMMEDIATE_PARAMETER;

  typedef 
    enum D3D12_WRITEBUFFERIMMEDIATE_MODE
  {
    D3D12_WRITEBUFFERIMMEDIATE_MODE_DEFAULT	= 0,
    D3D12_WRITEBUFFERIMMEDIATE_MODE_MARKER_IN	= 0x1,
    D3D12_WRITEBUFFERIMMEDIATE_MODE_MARKER_OUT	= 0x2
  } 	D3D12_WRITEBUFFERIMMEDIATE_MODE;



  extern RPC_IF_HANDLE __MIDL_itf_d3d12_0000_0018_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_d3d12_0000_0018_v0_0_s_ifspec;

#ifndef __ID3D12GraphicsCommandList2_INTERFACE_DEFINED__
#define __ID3D12GraphicsCommandList2_INTERFACE_DEFINED__

  /* interface ID3D12GraphicsCommandList2 */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12GraphicsCommandList2;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("38C3E585-FF17-412C-9150-4FC6F9D72A28")
    ID3D12GraphicsCommandList2 : public ID3D12GraphicsCommandList1
  {
  public:
    virtual void STDMETHODCALLTYPE WriteBufferImmediate( 
      UINT Count,
      _In_reads_(Count)  const D3D12_WRITEBUFFERIMMEDIATE_PARAMETER *pParams,
      _In_reads_opt_(Count)  const D3D12_WRITEBUFFERIMMEDIATE_MODE *pModes) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12GraphicsCommandList2Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12GraphicsCommandList2 * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12GraphicsCommandList2 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12GraphicsCommandList2 * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12GraphicsCommandList2 * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12GraphicsCommandList2 * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    D3D12_COMMAND_LIST_TYPE ( STDMETHODCALLTYPE *GetType )( 
      ID3D12GraphicsCommandList2 * This);

    HRESULT ( STDMETHODCALLTYPE *Close )( 
      ID3D12GraphicsCommandList2 * This);

    HRESULT ( STDMETHODCALLTYPE *Reset )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  ID3D12CommandAllocator *pAllocator,
      _In_opt_  ID3D12PipelineState *pInitialState);

    void ( STDMETHODCALLTYPE *ClearState )( 
      ID3D12GraphicsCommandList2 * This,
      _In_opt_  ID3D12PipelineState *pPipelineState);

    void ( STDMETHODCALLTYPE *DrawInstanced )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT VertexCountPerInstance,
      _In_  UINT InstanceCount,
      _In_  UINT StartVertexLocation,
      _In_  UINT StartInstanceLocation);

    void ( STDMETHODCALLTYPE *DrawIndexedInstanced )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT IndexCountPerInstance,
      _In_  UINT InstanceCount,
      _In_  UINT StartIndexLocation,
      _In_  INT BaseVertexLocation,
      _In_  UINT StartInstanceLocation);

    void ( STDMETHODCALLTYPE *Dispatch )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT ThreadGroupCountX,
      _In_  UINT ThreadGroupCountY,
      _In_  UINT ThreadGroupCountZ);

    void ( STDMETHODCALLTYPE *CopyBufferRegion )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  ID3D12Resource *pDstBuffer,
      UINT64 DstOffset,
      _In_  ID3D12Resource *pSrcBuffer,
      UINT64 SrcOffset,
      UINT64 NumBytes);

    void ( STDMETHODCALLTYPE *CopyTextureRegion )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  const D3D12_TEXTURE_COPY_LOCATION *pDst,
      UINT DstX,
      UINT DstY,
      UINT DstZ,
      _In_  const D3D12_TEXTURE_COPY_LOCATION *pSrc,
      _In_opt_  const D3D12_BOX *pSrcBox);

    void ( STDMETHODCALLTYPE *CopyResource )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  ID3D12Resource *pDstResource,
      _In_  ID3D12Resource *pSrcResource);

    void ( STDMETHODCALLTYPE *CopyTiles )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  ID3D12Resource *pTiledResource,
      _In_  const D3D12_TILED_RESOURCE_COORDINATE *pTileRegionStartCoordinate,
      _In_  const D3D12_TILE_REGION_SIZE *pTileRegionSize,
      _In_  ID3D12Resource *pBuffer,
      UINT64 BufferStartOffsetInBytes,
      D3D12_TILE_COPY_FLAGS Flags);

    void ( STDMETHODCALLTYPE *ResolveSubresource )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  ID3D12Resource *pDstResource,
      _In_  UINT DstSubresource,
      _In_  ID3D12Resource *pSrcResource,
      _In_  UINT SrcSubresource,
      _In_  DXGI_FORMAT Format);

    void ( STDMETHODCALLTYPE *IASetPrimitiveTopology )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology);

    void ( STDMETHODCALLTYPE *RSSetViewports )( 
      ID3D12GraphicsCommandList2 * This,
      _In_range_(0, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumViewports,
      _In_reads_( NumViewports)  const D3D12_VIEWPORT *pViewports);

    void ( STDMETHODCALLTYPE *RSSetScissorRects )( 
      ID3D12GraphicsCommandList2 * This,
      _In_range_(0, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumRects,
      _In_reads_( NumRects)  const D3D12_RECT *pRects);

    void ( STDMETHODCALLTYPE *OMSetBlendFactor )( 
      ID3D12GraphicsCommandList2 * This,
      _In_reads_opt_(4)  const FLOAT BlendFactor[ 4 ]);

    void ( STDMETHODCALLTYPE *OMSetStencilRef )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT StencilRef);

    void ( STDMETHODCALLTYPE *SetPipelineState )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  ID3D12PipelineState *pPipelineState);

    void ( STDMETHODCALLTYPE *ResourceBarrier )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT NumBarriers,
      _In_reads_(NumBarriers)  const D3D12_RESOURCE_BARRIER *pBarriers);

    void ( STDMETHODCALLTYPE *ExecuteBundle )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  ID3D12GraphicsCommandList *pCommandList);

    void ( STDMETHODCALLTYPE *SetDescriptorHeaps )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT NumDescriptorHeaps,
      _In_reads_(NumDescriptorHeaps)  ID3D12DescriptorHeap *const *ppDescriptorHeaps);

    void ( STDMETHODCALLTYPE *SetComputeRootSignature )( 
      ID3D12GraphicsCommandList2 * This,
      _In_opt_  ID3D12RootSignature *pRootSignature);

    void ( STDMETHODCALLTYPE *SetGraphicsRootSignature )( 
      ID3D12GraphicsCommandList2 * This,
      _In_opt_  ID3D12RootSignature *pRootSignature);

    void ( STDMETHODCALLTYPE *SetComputeRootDescriptorTable )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);

    void ( STDMETHODCALLTYPE *SetGraphicsRootDescriptorTable )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);

    void ( STDMETHODCALLTYPE *SetComputeRoot32BitConstant )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT RootParameterIndex,
      _In_  UINT SrcData,
      _In_  UINT DestOffsetIn32BitValues);

    void ( STDMETHODCALLTYPE *SetGraphicsRoot32BitConstant )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT RootParameterIndex,
      _In_  UINT SrcData,
      _In_  UINT DestOffsetIn32BitValues);

    void ( STDMETHODCALLTYPE *SetComputeRoot32BitConstants )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT RootParameterIndex,
      _In_  UINT Num32BitValuesToSet,
      _In_reads_(Num32BitValuesToSet*sizeof(UINT))  const void *pSrcData,
      _In_  UINT DestOffsetIn32BitValues);

    void ( STDMETHODCALLTYPE *SetGraphicsRoot32BitConstants )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT RootParameterIndex,
      _In_  UINT Num32BitValuesToSet,
      _In_reads_(Num32BitValuesToSet*sizeof(UINT))  const void *pSrcData,
      _In_  UINT DestOffsetIn32BitValues);

    void ( STDMETHODCALLTYPE *SetComputeRootConstantBufferView )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *SetGraphicsRootConstantBufferView )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *SetComputeRootShaderResourceView )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *SetGraphicsRootShaderResourceView )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *SetComputeRootUnorderedAccessView )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *SetGraphicsRootUnorderedAccessView )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *IASetIndexBuffer )( 
      ID3D12GraphicsCommandList2 * This,
      _In_opt_  const D3D12_INDEX_BUFFER_VIEW *pView);

    void ( STDMETHODCALLTYPE *IASetVertexBuffers )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT StartSlot,
      _In_  UINT NumViews,
      _In_reads_opt_(NumViews)  const D3D12_VERTEX_BUFFER_VIEW *pViews);

    void ( STDMETHODCALLTYPE *SOSetTargets )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT StartSlot,
      _In_  UINT NumViews,
      _In_reads_opt_(NumViews)  const D3D12_STREAM_OUTPUT_BUFFER_VIEW *pViews);

    void ( STDMETHODCALLTYPE *OMSetRenderTargets )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT NumRenderTargetDescriptors,
      _In_opt_  const D3D12_CPU_DESCRIPTOR_HANDLE *pRenderTargetDescriptors,
      _In_  BOOL RTsSingleHandleToDescriptorRange,
      _In_opt_  const D3D12_CPU_DESCRIPTOR_HANDLE *pDepthStencilDescriptor);

    void ( STDMETHODCALLTYPE *ClearDepthStencilView )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView,
      _In_  D3D12_CLEAR_FLAGS ClearFlags,
      _In_  FLOAT Depth,
      _In_  UINT8 Stencil,
      _In_  UINT NumRects,
      _In_reads_(NumRects)  const D3D12_RECT *pRects);

    void ( STDMETHODCALLTYPE *ClearRenderTargetView )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView,
      _In_  const FLOAT ColorRGBA[ 4 ],
      _In_  UINT NumRects,
      _In_reads_(NumRects)  const D3D12_RECT *pRects);

    void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewUint )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle,
      _In_  ID3D12Resource *pResource,
      _In_  const UINT Values[ 4 ],
      _In_  UINT NumRects,
      _In_reads_(NumRects)  const D3D12_RECT *pRects);

    void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewFloat )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle,
      _In_  ID3D12Resource *pResource,
      _In_  const FLOAT Values[ 4 ],
      _In_  UINT NumRects,
      _In_reads_(NumRects)  const D3D12_RECT *pRects);

    void ( STDMETHODCALLTYPE *DiscardResource )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_DISCARD_REGION *pRegion);

    void ( STDMETHODCALLTYPE *BeginQuery )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  ID3D12QueryHeap *pQueryHeap,
      _In_  D3D12_QUERY_TYPE Type,
      _In_  UINT Index);

    void ( STDMETHODCALLTYPE *EndQuery )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  ID3D12QueryHeap *pQueryHeap,
      _In_  D3D12_QUERY_TYPE Type,
      _In_  UINT Index);

    void ( STDMETHODCALLTYPE *ResolveQueryData )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  ID3D12QueryHeap *pQueryHeap,
      _In_  D3D12_QUERY_TYPE Type,
      _In_  UINT StartIndex,
      _In_  UINT NumQueries,
      _In_  ID3D12Resource *pDestinationBuffer,
      _In_  UINT64 AlignedDestinationBufferOffset);

    void ( STDMETHODCALLTYPE *SetPredication )( 
      ID3D12GraphicsCommandList2 * This,
      _In_opt_  ID3D12Resource *pBuffer,
      _In_  UINT64 AlignedBufferOffset,
      _In_  D3D12_PREDICATION_OP Operation);

    void ( STDMETHODCALLTYPE *SetMarker )( 
      ID3D12GraphicsCommandList2 * This,
      UINT Metadata,
      _In_reads_bytes_opt_(Size)  const void *pData,
      UINT Size);

    void ( STDMETHODCALLTYPE *BeginEvent )( 
      ID3D12GraphicsCommandList2 * This,
      UINT Metadata,
      _In_reads_bytes_opt_(Size)  const void *pData,
      UINT Size);

    void ( STDMETHODCALLTYPE *EndEvent )( 
      ID3D12GraphicsCommandList2 * This);

    void ( STDMETHODCALLTYPE *ExecuteIndirect )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  ID3D12CommandSignature *pCommandSignature,
      _In_  UINT MaxCommandCount,
      _In_  ID3D12Resource *pArgumentBuffer,
      _In_  UINT64 ArgumentBufferOffset,
      _In_opt_  ID3D12Resource *pCountBuffer,
      _In_  UINT64 CountBufferOffset);

    void ( STDMETHODCALLTYPE *AtomicCopyBufferUINT )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  ID3D12Resource *pDstBuffer,
      UINT64 DstOffset,
      _In_  ID3D12Resource *pSrcBuffer,
      UINT64 SrcOffset,
      UINT Dependencies,
      _In_reads_(Dependencies)  ID3D12Resource *const *ppDependentResources,
      _In_reads_(Dependencies)  const D3D12_SUBRESOURCE_RANGE_UINT64 *pDependentSubresourceRanges);

    void ( STDMETHODCALLTYPE *AtomicCopyBufferUINT64 )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  ID3D12Resource *pDstBuffer,
      UINT64 DstOffset,
      _In_  ID3D12Resource *pSrcBuffer,
      UINT64 SrcOffset,
      UINT Dependencies,
      _In_reads_(Dependencies)  ID3D12Resource *const *ppDependentResources,
      _In_reads_(Dependencies)  const D3D12_SUBRESOURCE_RANGE_UINT64 *pDependentSubresourceRanges);

    void ( STDMETHODCALLTYPE *OMSetDepthBounds )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  FLOAT Min,
      _In_  FLOAT Max);

    void ( STDMETHODCALLTYPE *SetSamplePositions )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT NumSamplesPerPixel,
      _In_  UINT NumPixels,
      _In_reads_(NumSamplesPerPixel*NumPixels)  D3D12_SAMPLE_POSITION *pSamplePositions);

    void ( STDMETHODCALLTYPE *ResolveSubresourceRegion )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  ID3D12Resource *pDstResource,
      _In_  UINT DstSubresource,
      _In_  UINT DstX,
      _In_  UINT DstY,
      _In_  ID3D12Resource *pSrcResource,
      _In_  UINT SrcSubresource,
      _In_opt_  D3D12_RECT *pSrcRect,
      _In_  DXGI_FORMAT Format,
      _In_  D3D12_RESOLVE_MODE ResolveMode);

    void ( STDMETHODCALLTYPE *SetViewInstanceMask )( 
      ID3D12GraphicsCommandList2 * This,
      _In_  UINT Mask);

    void ( STDMETHODCALLTYPE *WriteBufferImmediate )( 
      ID3D12GraphicsCommandList2 * This,
      UINT Count,
      _In_reads_(Count)  const D3D12_WRITEBUFFERIMMEDIATE_PARAMETER *pParams,
      _In_reads_opt_(Count)  const D3D12_WRITEBUFFERIMMEDIATE_MODE *pModes);

    END_INTERFACE
  } ID3D12GraphicsCommandList2Vtbl;

  interface ID3D12GraphicsCommandList2
  {
    CONST_VTBL struct ID3D12GraphicsCommandList2Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12GraphicsCommandList2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12GraphicsCommandList2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12GraphicsCommandList2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12GraphicsCommandList2_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12GraphicsCommandList2_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12GraphicsCommandList2_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12GraphicsCommandList2_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12GraphicsCommandList2_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 


#define ID3D12GraphicsCommandList2_GetType(This)	\
    ( (This)->lpVtbl -> GetType(This) ) 


#define ID3D12GraphicsCommandList2_Close(This)	\
    ( (This)->lpVtbl -> Close(This) ) 

#define ID3D12GraphicsCommandList2_Reset(This,pAllocator,pInitialState)	\
    ( (This)->lpVtbl -> Reset(This,pAllocator,pInitialState) ) 

#define ID3D12GraphicsCommandList2_ClearState(This,pPipelineState)	\
    ( (This)->lpVtbl -> ClearState(This,pPipelineState) ) 

#define ID3D12GraphicsCommandList2_DrawInstanced(This,VertexCountPerInstance,InstanceCount,StartVertexLocation,StartInstanceLocation)	\
    ( (This)->lpVtbl -> DrawInstanced(This,VertexCountPerInstance,InstanceCount,StartVertexLocation,StartInstanceLocation) ) 

#define ID3D12GraphicsCommandList2_DrawIndexedInstanced(This,IndexCountPerInstance,InstanceCount,StartIndexLocation,BaseVertexLocation,StartInstanceLocation)	\
    ( (This)->lpVtbl -> DrawIndexedInstanced(This,IndexCountPerInstance,InstanceCount,StartIndexLocation,BaseVertexLocation,StartInstanceLocation) ) 

#define ID3D12GraphicsCommandList2_Dispatch(This,ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ)	\
    ( (This)->lpVtbl -> Dispatch(This,ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ) ) 

#define ID3D12GraphicsCommandList2_CopyBufferRegion(This,pDstBuffer,DstOffset,pSrcBuffer,SrcOffset,NumBytes)	\
    ( (This)->lpVtbl -> CopyBufferRegion(This,pDstBuffer,DstOffset,pSrcBuffer,SrcOffset,NumBytes) ) 

#define ID3D12GraphicsCommandList2_CopyTextureRegion(This,pDst,DstX,DstY,DstZ,pSrc,pSrcBox)	\
    ( (This)->lpVtbl -> CopyTextureRegion(This,pDst,DstX,DstY,DstZ,pSrc,pSrcBox) ) 

#define ID3D12GraphicsCommandList2_CopyResource(This,pDstResource,pSrcResource)	\
    ( (This)->lpVtbl -> CopyResource(This,pDstResource,pSrcResource) ) 

#define ID3D12GraphicsCommandList2_CopyTiles(This,pTiledResource,pTileRegionStartCoordinate,pTileRegionSize,pBuffer,BufferStartOffsetInBytes,Flags)	\
    ( (This)->lpVtbl -> CopyTiles(This,pTiledResource,pTileRegionStartCoordinate,pTileRegionSize,pBuffer,BufferStartOffsetInBytes,Flags) ) 

#define ID3D12GraphicsCommandList2_ResolveSubresource(This,pDstResource,DstSubresource,pSrcResource,SrcSubresource,Format)	\
    ( (This)->lpVtbl -> ResolveSubresource(This,pDstResource,DstSubresource,pSrcResource,SrcSubresource,Format) ) 

#define ID3D12GraphicsCommandList2_IASetPrimitiveTopology(This,PrimitiveTopology)	\
    ( (This)->lpVtbl -> IASetPrimitiveTopology(This,PrimitiveTopology) ) 

#define ID3D12GraphicsCommandList2_RSSetViewports(This,NumViewports,pViewports)	\
    ( (This)->lpVtbl -> RSSetViewports(This,NumViewports,pViewports) ) 

#define ID3D12GraphicsCommandList2_RSSetScissorRects(This,NumRects,pRects)	\
    ( (This)->lpVtbl -> RSSetScissorRects(This,NumRects,pRects) ) 

#define ID3D12GraphicsCommandList2_OMSetBlendFactor(This,BlendFactor)	\
    ( (This)->lpVtbl -> OMSetBlendFactor(This,BlendFactor) ) 

#define ID3D12GraphicsCommandList2_OMSetStencilRef(This,StencilRef)	\
    ( (This)->lpVtbl -> OMSetStencilRef(This,StencilRef) ) 

#define ID3D12GraphicsCommandList2_SetPipelineState(This,pPipelineState)	\
    ( (This)->lpVtbl -> SetPipelineState(This,pPipelineState) ) 

#define ID3D12GraphicsCommandList2_ResourceBarrier(This,NumBarriers,pBarriers)	\
    ( (This)->lpVtbl -> ResourceBarrier(This,NumBarriers,pBarriers) ) 

#define ID3D12GraphicsCommandList2_ExecuteBundle(This,pCommandList)	\
    ( (This)->lpVtbl -> ExecuteBundle(This,pCommandList) ) 

#define ID3D12GraphicsCommandList2_SetDescriptorHeaps(This,NumDescriptorHeaps,ppDescriptorHeaps)	\
    ( (This)->lpVtbl -> SetDescriptorHeaps(This,NumDescriptorHeaps,ppDescriptorHeaps) ) 

#define ID3D12GraphicsCommandList2_SetComputeRootSignature(This,pRootSignature)	\
    ( (This)->lpVtbl -> SetComputeRootSignature(This,pRootSignature) ) 

#define ID3D12GraphicsCommandList2_SetGraphicsRootSignature(This,pRootSignature)	\
    ( (This)->lpVtbl -> SetGraphicsRootSignature(This,pRootSignature) ) 

#define ID3D12GraphicsCommandList2_SetComputeRootDescriptorTable(This,RootParameterIndex,BaseDescriptor)	\
    ( (This)->lpVtbl -> SetComputeRootDescriptorTable(This,RootParameterIndex,BaseDescriptor) ) 

#define ID3D12GraphicsCommandList2_SetGraphicsRootDescriptorTable(This,RootParameterIndex,BaseDescriptor)	\
    ( (This)->lpVtbl -> SetGraphicsRootDescriptorTable(This,RootParameterIndex,BaseDescriptor) ) 

#define ID3D12GraphicsCommandList2_SetComputeRoot32BitConstant(This,RootParameterIndex,SrcData,DestOffsetIn32BitValues)	\
    ( (This)->lpVtbl -> SetComputeRoot32BitConstant(This,RootParameterIndex,SrcData,DestOffsetIn32BitValues) ) 

#define ID3D12GraphicsCommandList2_SetGraphicsRoot32BitConstant(This,RootParameterIndex,SrcData,DestOffsetIn32BitValues)	\
    ( (This)->lpVtbl -> SetGraphicsRoot32BitConstant(This,RootParameterIndex,SrcData,DestOffsetIn32BitValues) ) 

#define ID3D12GraphicsCommandList2_SetComputeRoot32BitConstants(This,RootParameterIndex,Num32BitValuesToSet,pSrcData,DestOffsetIn32BitValues)	\
    ( (This)->lpVtbl -> SetComputeRoot32BitConstants(This,RootParameterIndex,Num32BitValuesToSet,pSrcData,DestOffsetIn32BitValues) ) 

#define ID3D12GraphicsCommandList2_SetGraphicsRoot32BitConstants(This,RootParameterIndex,Num32BitValuesToSet,pSrcData,DestOffsetIn32BitValues)	\
    ( (This)->lpVtbl -> SetGraphicsRoot32BitConstants(This,RootParameterIndex,Num32BitValuesToSet,pSrcData,DestOffsetIn32BitValues) ) 

#define ID3D12GraphicsCommandList2_SetComputeRootConstantBufferView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetComputeRootConstantBufferView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList2_SetGraphicsRootConstantBufferView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetGraphicsRootConstantBufferView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList2_SetComputeRootShaderResourceView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetComputeRootShaderResourceView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList2_SetGraphicsRootShaderResourceView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetGraphicsRootShaderResourceView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList2_SetComputeRootUnorderedAccessView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetComputeRootUnorderedAccessView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList2_SetGraphicsRootUnorderedAccessView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetGraphicsRootUnorderedAccessView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList2_IASetIndexBuffer(This,pView)	\
    ( (This)->lpVtbl -> IASetIndexBuffer(This,pView) ) 

#define ID3D12GraphicsCommandList2_IASetVertexBuffers(This,StartSlot,NumViews,pViews)	\
    ( (This)->lpVtbl -> IASetVertexBuffers(This,StartSlot,NumViews,pViews) ) 

#define ID3D12GraphicsCommandList2_SOSetTargets(This,StartSlot,NumViews,pViews)	\
    ( (This)->lpVtbl -> SOSetTargets(This,StartSlot,NumViews,pViews) ) 

#define ID3D12GraphicsCommandList2_OMSetRenderTargets(This,NumRenderTargetDescriptors,pRenderTargetDescriptors,RTsSingleHandleToDescriptorRange,pDepthStencilDescriptor)	\
    ( (This)->lpVtbl -> OMSetRenderTargets(This,NumRenderTargetDescriptors,pRenderTargetDescriptors,RTsSingleHandleToDescriptorRange,pDepthStencilDescriptor) ) 

#define ID3D12GraphicsCommandList2_ClearDepthStencilView(This,DepthStencilView,ClearFlags,Depth,Stencil,NumRects,pRects)	\
    ( (This)->lpVtbl -> ClearDepthStencilView(This,DepthStencilView,ClearFlags,Depth,Stencil,NumRects,pRects) ) 

#define ID3D12GraphicsCommandList2_ClearRenderTargetView(This,RenderTargetView,ColorRGBA,NumRects,pRects)	\
    ( (This)->lpVtbl -> ClearRenderTargetView(This,RenderTargetView,ColorRGBA,NumRects,pRects) ) 

#define ID3D12GraphicsCommandList2_ClearUnorderedAccessViewUint(This,ViewGPUHandleInCurrentHeap,ViewCPUHandle,pResource,Values,NumRects,pRects)	\
    ( (This)->lpVtbl -> ClearUnorderedAccessViewUint(This,ViewGPUHandleInCurrentHeap,ViewCPUHandle,pResource,Values,NumRects,pRects) ) 

#define ID3D12GraphicsCommandList2_ClearUnorderedAccessViewFloat(This,ViewGPUHandleInCurrentHeap,ViewCPUHandle,pResource,Values,NumRects,pRects)	\
    ( (This)->lpVtbl -> ClearUnorderedAccessViewFloat(This,ViewGPUHandleInCurrentHeap,ViewCPUHandle,pResource,Values,NumRects,pRects) ) 

#define ID3D12GraphicsCommandList2_DiscardResource(This,pResource,pRegion)	\
    ( (This)->lpVtbl -> DiscardResource(This,pResource,pRegion) ) 

#define ID3D12GraphicsCommandList2_BeginQuery(This,pQueryHeap,Type,Index)	\
    ( (This)->lpVtbl -> BeginQuery(This,pQueryHeap,Type,Index) ) 

#define ID3D12GraphicsCommandList2_EndQuery(This,pQueryHeap,Type,Index)	\
    ( (This)->lpVtbl -> EndQuery(This,pQueryHeap,Type,Index) ) 

#define ID3D12GraphicsCommandList2_ResolveQueryData(This,pQueryHeap,Type,StartIndex,NumQueries,pDestinationBuffer,AlignedDestinationBufferOffset)	\
    ( (This)->lpVtbl -> ResolveQueryData(This,pQueryHeap,Type,StartIndex,NumQueries,pDestinationBuffer,AlignedDestinationBufferOffset) ) 

#define ID3D12GraphicsCommandList2_SetPredication(This,pBuffer,AlignedBufferOffset,Operation)	\
    ( (This)->lpVtbl -> SetPredication(This,pBuffer,AlignedBufferOffset,Operation) ) 

#define ID3D12GraphicsCommandList2_SetMarker(This,Metadata,pData,Size)	\
    ( (This)->lpVtbl -> SetMarker(This,Metadata,pData,Size) ) 

#define ID3D12GraphicsCommandList2_BeginEvent(This,Metadata,pData,Size)	\
    ( (This)->lpVtbl -> BeginEvent(This,Metadata,pData,Size) ) 

#define ID3D12GraphicsCommandList2_EndEvent(This)	\
    ( (This)->lpVtbl -> EndEvent(This) ) 

#define ID3D12GraphicsCommandList2_ExecuteIndirect(This,pCommandSignature,MaxCommandCount,pArgumentBuffer,ArgumentBufferOffset,pCountBuffer,CountBufferOffset)	\
    ( (This)->lpVtbl -> ExecuteIndirect(This,pCommandSignature,MaxCommandCount,pArgumentBuffer,ArgumentBufferOffset,pCountBuffer,CountBufferOffset) ) 


#define ID3D12GraphicsCommandList2_AtomicCopyBufferUINT(This,pDstBuffer,DstOffset,pSrcBuffer,SrcOffset,Dependencies,ppDependentResources,pDependentSubresourceRanges)	\
    ( (This)->lpVtbl -> AtomicCopyBufferUINT(This,pDstBuffer,DstOffset,pSrcBuffer,SrcOffset,Dependencies,ppDependentResources,pDependentSubresourceRanges) ) 

#define ID3D12GraphicsCommandList2_AtomicCopyBufferUINT64(This,pDstBuffer,DstOffset,pSrcBuffer,SrcOffset,Dependencies,ppDependentResources,pDependentSubresourceRanges)	\
    ( (This)->lpVtbl -> AtomicCopyBufferUINT64(This,pDstBuffer,DstOffset,pSrcBuffer,SrcOffset,Dependencies,ppDependentResources,pDependentSubresourceRanges) ) 

#define ID3D12GraphicsCommandList2_OMSetDepthBounds(This,Min,Max)	\
    ( (This)->lpVtbl -> OMSetDepthBounds(This,Min,Max) ) 

#define ID3D12GraphicsCommandList2_SetSamplePositions(This,NumSamplesPerPixel,NumPixels,pSamplePositions)	\
    ( (This)->lpVtbl -> SetSamplePositions(This,NumSamplesPerPixel,NumPixels,pSamplePositions) ) 

#define ID3D12GraphicsCommandList2_ResolveSubresourceRegion(This,pDstResource,DstSubresource,DstX,DstY,pSrcResource,SrcSubresource,pSrcRect,Format,ResolveMode)	\
    ( (This)->lpVtbl -> ResolveSubresourceRegion(This,pDstResource,DstSubresource,DstX,DstY,pSrcResource,SrcSubresource,pSrcRect,Format,ResolveMode) ) 

#define ID3D12GraphicsCommandList2_SetViewInstanceMask(This,Mask)	\
    ( (This)->lpVtbl -> SetViewInstanceMask(This,Mask) ) 


#define ID3D12GraphicsCommandList2_WriteBufferImmediate(This,Count,pParams,pModes)	\
    ( (This)->lpVtbl -> WriteBufferImmediate(This,Count,pParams,pModes) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12GraphicsCommandList2_INTERFACE_DEFINED__ */


#ifndef __ID3D12CommandQueue_INTERFACE_DEFINED__
#define __ID3D12CommandQueue_INTERFACE_DEFINED__

  /* interface ID3D12CommandQueue */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12CommandQueue;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("0ec870a6-5d7e-4c22-8cfc-5baae07616ed")
    ID3D12CommandQueue : public ID3D12Pageable
  {
  public:
    virtual void STDMETHODCALLTYPE UpdateTileMappings( 
      _In_  ID3D12Resource *pResource,
      UINT NumResourceRegions,
      _In_reads_opt_(NumResourceRegions)  const D3D12_TILED_RESOURCE_COORDINATE *pResourceRegionStartCoordinates,
      _In_reads_opt_(NumResourceRegions)  const D3D12_TILE_REGION_SIZE *pResourceRegionSizes,
      _In_opt_  ID3D12Heap *pHeap,
      UINT NumRanges,
      _In_reads_opt_(NumRanges)  const D3D12_TILE_RANGE_FLAGS *pRangeFlags,
      _In_reads_opt_(NumRanges)  const UINT *pHeapRangeStartOffsets,
      _In_reads_opt_(NumRanges)  const UINT *pRangeTileCounts,
      D3D12_TILE_MAPPING_FLAGS Flags) = 0;

    virtual void STDMETHODCALLTYPE CopyTileMappings( 
      _In_  ID3D12Resource *pDstResource,
      _In_  const D3D12_TILED_RESOURCE_COORDINATE *pDstRegionStartCoordinate,
      _In_  ID3D12Resource *pSrcResource,
      _In_  const D3D12_TILED_RESOURCE_COORDINATE *pSrcRegionStartCoordinate,
      _In_  const D3D12_TILE_REGION_SIZE *pRegionSize,
      D3D12_TILE_MAPPING_FLAGS Flags) = 0;

    virtual void STDMETHODCALLTYPE ExecuteCommandLists( 
      _In_  UINT NumCommandLists,
      _In_reads_(NumCommandLists)  ID3D12CommandList *const *ppCommandLists) = 0;

    virtual void STDMETHODCALLTYPE SetMarker( 
      UINT Metadata,
      _In_reads_bytes_opt_(Size)  const void *pData,
      UINT Size) = 0;

    virtual void STDMETHODCALLTYPE BeginEvent( 
      UINT Metadata,
      _In_reads_bytes_opt_(Size)  const void *pData,
      UINT Size) = 0;

    virtual void STDMETHODCALLTYPE EndEvent( void) = 0;

    virtual HRESULT STDMETHODCALLTYPE Signal( 
      ID3D12Fence *pFence,
      UINT64 Value) = 0;

    virtual HRESULT STDMETHODCALLTYPE Wait( 
      ID3D12Fence *pFence,
      UINT64 Value) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetTimestampFrequency( 
      _Out_  UINT64 *pFrequency) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetClockCalibration( 
      _Out_  UINT64 *pGpuTimestamp,
      _Out_  UINT64 *pCpuTimestamp) = 0;

    virtual D3D12_COMMAND_QUEUE_DESC STDMETHODCALLTYPE GetDesc( void) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12CommandQueueVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12CommandQueue * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12CommandQueue * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12CommandQueue * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12CommandQueue * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12CommandQueue * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12CommandQueue * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12CommandQueue * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12CommandQueue * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    void ( STDMETHODCALLTYPE *UpdateTileMappings )( 
      ID3D12CommandQueue * This,
      _In_  ID3D12Resource *pResource,
      UINT NumResourceRegions,
      _In_reads_opt_(NumResourceRegions)  const D3D12_TILED_RESOURCE_COORDINATE *pResourceRegionStartCoordinates,
      _In_reads_opt_(NumResourceRegions)  const D3D12_TILE_REGION_SIZE *pResourceRegionSizes,
      _In_opt_  ID3D12Heap *pHeap,
      UINT NumRanges,
      _In_reads_opt_(NumRanges)  const D3D12_TILE_RANGE_FLAGS *pRangeFlags,
      _In_reads_opt_(NumRanges)  const UINT *pHeapRangeStartOffsets,
      _In_reads_opt_(NumRanges)  const UINT *pRangeTileCounts,
      D3D12_TILE_MAPPING_FLAGS Flags);

    void ( STDMETHODCALLTYPE *CopyTileMappings )( 
      ID3D12CommandQueue * This,
      _In_  ID3D12Resource *pDstResource,
      _In_  const D3D12_TILED_RESOURCE_COORDINATE *pDstRegionStartCoordinate,
      _In_  ID3D12Resource *pSrcResource,
      _In_  const D3D12_TILED_RESOURCE_COORDINATE *pSrcRegionStartCoordinate,
      _In_  const D3D12_TILE_REGION_SIZE *pRegionSize,
      D3D12_TILE_MAPPING_FLAGS Flags);

    void ( STDMETHODCALLTYPE *ExecuteCommandLists )( 
      ID3D12CommandQueue * This,
      _In_  UINT NumCommandLists,
      _In_reads_(NumCommandLists)  ID3D12CommandList *const *ppCommandLists);

    void ( STDMETHODCALLTYPE *SetMarker )( 
      ID3D12CommandQueue * This,
      UINT Metadata,
      _In_reads_bytes_opt_(Size)  const void *pData,
      UINT Size);

    void ( STDMETHODCALLTYPE *BeginEvent )( 
      ID3D12CommandQueue * This,
      UINT Metadata,
      _In_reads_bytes_opt_(Size)  const void *pData,
      UINT Size);

    void ( STDMETHODCALLTYPE *EndEvent )( 
      ID3D12CommandQueue * This);

    HRESULT ( STDMETHODCALLTYPE *Signal )( 
      ID3D12CommandQueue * This,
      ID3D12Fence *pFence,
      UINT64 Value);

    HRESULT ( STDMETHODCALLTYPE *Wait )( 
      ID3D12CommandQueue * This,
      ID3D12Fence *pFence,
      UINT64 Value);

    HRESULT ( STDMETHODCALLTYPE *GetTimestampFrequency )( 
      ID3D12CommandQueue * This,
      _Out_  UINT64 *pFrequency);

    HRESULT ( STDMETHODCALLTYPE *GetClockCalibration )( 
      ID3D12CommandQueue * This,
      _Out_  UINT64 *pGpuTimestamp,
      _Out_  UINT64 *pCpuTimestamp);

    D3D12_COMMAND_QUEUE_DESC ( STDMETHODCALLTYPE *GetDesc )( 
      ID3D12CommandQueue * This);

    END_INTERFACE
  } ID3D12CommandQueueVtbl;

  interface ID3D12CommandQueue
  {
    CONST_VTBL struct ID3D12CommandQueueVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12CommandQueue_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12CommandQueue_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12CommandQueue_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12CommandQueue_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12CommandQueue_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12CommandQueue_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12CommandQueue_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12CommandQueue_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 



#define ID3D12CommandQueue_UpdateTileMappings(This,pResource,NumResourceRegions,pResourceRegionStartCoordinates,pResourceRegionSizes,pHeap,NumRanges,pRangeFlags,pHeapRangeStartOffsets,pRangeTileCounts,Flags)	\
    ( (This)->lpVtbl -> UpdateTileMappings(This,pResource,NumResourceRegions,pResourceRegionStartCoordinates,pResourceRegionSizes,pHeap,NumRanges,pRangeFlags,pHeapRangeStartOffsets,pRangeTileCounts,Flags) ) 

#define ID3D12CommandQueue_CopyTileMappings(This,pDstResource,pDstRegionStartCoordinate,pSrcResource,pSrcRegionStartCoordinate,pRegionSize,Flags)	\
    ( (This)->lpVtbl -> CopyTileMappings(This,pDstResource,pDstRegionStartCoordinate,pSrcResource,pSrcRegionStartCoordinate,pRegionSize,Flags) ) 

#define ID3D12CommandQueue_ExecuteCommandLists(This,NumCommandLists,ppCommandLists)	\
    ( (This)->lpVtbl -> ExecuteCommandLists(This,NumCommandLists,ppCommandLists) ) 

#define ID3D12CommandQueue_SetMarker(This,Metadata,pData,Size)	\
    ( (This)->lpVtbl -> SetMarker(This,Metadata,pData,Size) ) 

#define ID3D12CommandQueue_BeginEvent(This,Metadata,pData,Size)	\
    ( (This)->lpVtbl -> BeginEvent(This,Metadata,pData,Size) ) 

#define ID3D12CommandQueue_EndEvent(This)	\
    ( (This)->lpVtbl -> EndEvent(This) ) 

#define ID3D12CommandQueue_Signal(This,pFence,Value)	\
    ( (This)->lpVtbl -> Signal(This,pFence,Value) ) 

#define ID3D12CommandQueue_Wait(This,pFence,Value)	\
    ( (This)->lpVtbl -> Wait(This,pFence,Value) ) 

#define ID3D12CommandQueue_GetTimestampFrequency(This,pFrequency)	\
    ( (This)->lpVtbl -> GetTimestampFrequency(This,pFrequency) ) 

#define ID3D12CommandQueue_GetClockCalibration(This,pGpuTimestamp,pCpuTimestamp)	\
    ( (This)->lpVtbl -> GetClockCalibration(This,pGpuTimestamp,pCpuTimestamp) ) 

#define ID3D12CommandQueue_GetDesc(This)	\
    ( (This)->lpVtbl -> GetDesc(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12CommandQueue_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_d3d12_0000_0020 */
  /* [local] */ 

#ifdef __midl
#ifndef LUID_DEFINED
#define LUID_DEFINED 1
  typedef struct __LUID
  {
    DWORD LowPart;
    LONG HighPart;
  } 	LUID;

  typedef struct __LUID *PLUID;

#endif
#endif


  extern RPC_IF_HANDLE __MIDL_itf_d3d12_0000_0020_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_d3d12_0000_0020_v0_0_s_ifspec;

#ifndef __ID3D12Device_INTERFACE_DEFINED__
#define __ID3D12Device_INTERFACE_DEFINED__

  /* interface ID3D12Device */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12Device;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("189819f1-1db6-4b57-be54-1821339b85f7")
    ID3D12Device : public ID3D12Object
  {
  public:
    virtual UINT STDMETHODCALLTYPE GetNodeCount( void) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateCommandQueue( 
      _In_  const D3D12_COMMAND_QUEUE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppCommandQueue) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateCommandAllocator( 
      _In_  D3D12_COMMAND_LIST_TYPE type,
      REFIID riid,
      _COM_Outptr_  void **ppCommandAllocator) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateGraphicsPipelineState( 
      _In_  const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateComputePipelineState( 
      _In_  const D3D12_COMPUTE_PIPELINE_STATE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateCommandList( 
      _In_  UINT nodeMask,
      _In_  D3D12_COMMAND_LIST_TYPE type,
      _In_  ID3D12CommandAllocator *pCommandAllocator,
      _In_opt_  ID3D12PipelineState *pInitialState,
      REFIID riid,
      _COM_Outptr_  void **ppCommandList) = 0;

    virtual HRESULT STDMETHODCALLTYPE CheckFeatureSupport( 
      D3D12_FEATURE Feature,
      _Inout_updates_bytes_(FeatureSupportDataSize)  void *pFeatureSupportData,
      UINT FeatureSupportDataSize) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateDescriptorHeap( 
      _In_  const D3D12_DESCRIPTOR_HEAP_DESC *pDescriptorHeapDesc,
      REFIID riid,
      _COM_Outptr_  void **ppvHeap) = 0;

    virtual UINT STDMETHODCALLTYPE GetDescriptorHandleIncrementSize( 
      _In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateRootSignature( 
      _In_  UINT nodeMask,
      _In_reads_(blobLengthInBytes)  const void *pBlobWithRootSignature,
      _In_  SIZE_T blobLengthInBytes,
      REFIID riid,
      _COM_Outptr_  void **ppvRootSignature) = 0;

    virtual void STDMETHODCALLTYPE CreateConstantBufferView( 
      _In_opt_  const D3D12_CONSTANT_BUFFER_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) = 0;

    virtual void STDMETHODCALLTYPE CreateShaderResourceView( 
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_SHADER_RESOURCE_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) = 0;

    virtual void STDMETHODCALLTYPE CreateUnorderedAccessView( 
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  ID3D12Resource *pCounterResource,
      _In_opt_  const D3D12_UNORDERED_ACCESS_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) = 0;

    virtual void STDMETHODCALLTYPE CreateRenderTargetView( 
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_RENDER_TARGET_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) = 0;

    virtual void STDMETHODCALLTYPE CreateDepthStencilView( 
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_DEPTH_STENCIL_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) = 0;

    virtual void STDMETHODCALLTYPE CreateSampler( 
      _In_  const D3D12_SAMPLER_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) = 0;

    virtual void STDMETHODCALLTYPE CopyDescriptors( 
      _In_  UINT NumDestDescriptorRanges,
      _In_reads_(NumDestDescriptorRanges)  const D3D12_CPU_DESCRIPTOR_HANDLE *pDestDescriptorRangeStarts,
      _In_reads_opt_(NumDestDescriptorRanges)  const UINT *pDestDescriptorRangeSizes,
      _In_  UINT NumSrcDescriptorRanges,
      _In_reads_(NumSrcDescriptorRanges)  const D3D12_CPU_DESCRIPTOR_HANDLE *pSrcDescriptorRangeStarts,
      _In_reads_opt_(NumSrcDescriptorRanges)  const UINT *pSrcDescriptorRangeSizes,
      _In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType) = 0;

    virtual void STDMETHODCALLTYPE CopyDescriptorsSimple( 
      _In_  UINT NumDescriptors,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptorRangeStart,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRangeStart,
      _In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType) = 0;

    virtual D3D12_RESOURCE_ALLOCATION_INFO STDMETHODCALLTYPE GetResourceAllocationInfo( 
      _In_  UINT visibleMask,
      _In_  UINT numResourceDescs,
      _In_reads_(numResourceDescs)  const D3D12_RESOURCE_DESC *pResourceDescs) = 0;

    virtual D3D12_HEAP_PROPERTIES STDMETHODCALLTYPE GetCustomHeapProperties( 
      _In_  UINT nodeMask,
      D3D12_HEAP_TYPE heapType) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateCommittedResource( 
      _In_  const D3D12_HEAP_PROPERTIES *pHeapProperties,
      D3D12_HEAP_FLAGS HeapFlags,
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialResourceState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      REFIID riidResource,
      _COM_Outptr_opt_  void **ppvResource) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateHeap( 
      _In_  const D3D12_HEAP_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvHeap) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreatePlacedResource( 
      _In_  ID3D12Heap *pHeap,
      UINT64 HeapOffset,
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvResource) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateReservedResource( 
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvResource) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateSharedHandle( 
      _In_  ID3D12DeviceChild *pObject,
      _In_opt_  const SECURITY_ATTRIBUTES *pAttributes,
      DWORD Access,
      _In_opt_  LPCWSTR Name,
      _Out_  HANDLE *pHandle) = 0;

    virtual HRESULT STDMETHODCALLTYPE OpenSharedHandle( 
      _In_  HANDLE NTHandle,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvObj) = 0;

    virtual HRESULT STDMETHODCALLTYPE OpenSharedHandleByName( 
      _In_  LPCWSTR Name,
      DWORD Access,
      /* [annotation][out] */ 
      _Out_  HANDLE *pNTHandle) = 0;

    virtual HRESULT STDMETHODCALLTYPE MakeResident( 
      UINT NumObjects,
      _In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects) = 0;

    virtual HRESULT STDMETHODCALLTYPE Evict( 
      UINT NumObjects,
      _In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateFence( 
      UINT64 InitialValue,
      D3D12_FENCE_FLAGS Flags,
      REFIID riid,
      _COM_Outptr_  void **ppFence) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetDeviceRemovedReason( void) = 0;

    virtual void STDMETHODCALLTYPE GetCopyableFootprints( 
      _In_  const D3D12_RESOURCE_DESC *pResourceDesc,
      _In_range_(0,D3D12_REQ_SUBRESOURCES)  UINT FirstSubresource,
      _In_range_(0,D3D12_REQ_SUBRESOURCES-FirstSubresource)  UINT NumSubresources,
      UINT64 BaseOffset,
      _Out_writes_opt_(NumSubresources)  D3D12_PLACED_SUBRESOURCE_FOOTPRINT *pLayouts,
      _Out_writes_opt_(NumSubresources)  UINT *pNumRows,
      _Out_writes_opt_(NumSubresources)  UINT64 *pRowSizeInBytes,
      _Out_opt_  UINT64 *pTotalBytes) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateQueryHeap( 
      _In_  const D3D12_QUERY_HEAP_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvHeap) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetStablePowerState( 
      BOOL Enable) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateCommandSignature( 
      _In_  const D3D12_COMMAND_SIGNATURE_DESC *pDesc,
      _In_opt_  ID3D12RootSignature *pRootSignature,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvCommandSignature) = 0;

    virtual void STDMETHODCALLTYPE GetResourceTiling( 
      _In_  ID3D12Resource *pTiledResource,
      _Out_opt_  UINT *pNumTilesForEntireResource,
      _Out_opt_  D3D12_PACKED_MIP_INFO *pPackedMipDesc,
      _Out_opt_  D3D12_TILE_SHAPE *pStandardTileShapeForNonPackedMips,
      _Inout_opt_  UINT *pNumSubresourceTilings,
      _In_  UINT FirstSubresourceTilingToGet,
      _Out_writes_(*pNumSubresourceTilings)  D3D12_SUBRESOURCE_TILING *pSubresourceTilingsForNonPackedMips) = 0;

    virtual LUID STDMETHODCALLTYPE GetAdapterLuid( void) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12DeviceVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12Device * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12Device * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12Device * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12Device * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12Device * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12Device * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12Device * This,
      _In_z_  LPCWSTR Name);

    UINT ( STDMETHODCALLTYPE *GetNodeCount )( 
      ID3D12Device * This);

    HRESULT ( STDMETHODCALLTYPE *CreateCommandQueue )( 
      ID3D12Device * This,
      _In_  const D3D12_COMMAND_QUEUE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppCommandQueue);

    HRESULT ( STDMETHODCALLTYPE *CreateCommandAllocator )( 
      ID3D12Device * This,
      _In_  D3D12_COMMAND_LIST_TYPE type,
      REFIID riid,
      _COM_Outptr_  void **ppCommandAllocator);

    HRESULT ( STDMETHODCALLTYPE *CreateGraphicsPipelineState )( 
      ID3D12Device * This,
      _In_  const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState);

    HRESULT ( STDMETHODCALLTYPE *CreateComputePipelineState )( 
      ID3D12Device * This,
      _In_  const D3D12_COMPUTE_PIPELINE_STATE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState);

    HRESULT ( STDMETHODCALLTYPE *CreateCommandList )( 
      ID3D12Device * This,
      _In_  UINT nodeMask,
      _In_  D3D12_COMMAND_LIST_TYPE type,
      _In_  ID3D12CommandAllocator *pCommandAllocator,
      _In_opt_  ID3D12PipelineState *pInitialState,
      REFIID riid,
      _COM_Outptr_  void **ppCommandList);

    HRESULT ( STDMETHODCALLTYPE *CheckFeatureSupport )( 
      ID3D12Device * This,
      D3D12_FEATURE Feature,
      _Inout_updates_bytes_(FeatureSupportDataSize)  void *pFeatureSupportData,
      UINT FeatureSupportDataSize);

    HRESULT ( STDMETHODCALLTYPE *CreateDescriptorHeap )( 
      ID3D12Device * This,
      _In_  const D3D12_DESCRIPTOR_HEAP_DESC *pDescriptorHeapDesc,
      REFIID riid,
      _COM_Outptr_  void **ppvHeap);

    UINT ( STDMETHODCALLTYPE *GetDescriptorHandleIncrementSize )( 
      ID3D12Device * This,
      _In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType);

    HRESULT ( STDMETHODCALLTYPE *CreateRootSignature )( 
      ID3D12Device * This,
      _In_  UINT nodeMask,
      _In_reads_(blobLengthInBytes)  const void *pBlobWithRootSignature,
      _In_  SIZE_T blobLengthInBytes,
      REFIID riid,
      _COM_Outptr_  void **ppvRootSignature);

    void ( STDMETHODCALLTYPE *CreateConstantBufferView )( 
      ID3D12Device * This,
      _In_opt_  const D3D12_CONSTANT_BUFFER_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateShaderResourceView )( 
      ID3D12Device * This,
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_SHADER_RESOURCE_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateUnorderedAccessView )( 
      ID3D12Device * This,
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  ID3D12Resource *pCounterResource,
      _In_opt_  const D3D12_UNORDERED_ACCESS_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateRenderTargetView )( 
      ID3D12Device * This,
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_RENDER_TARGET_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateDepthStencilView )( 
      ID3D12Device * This,
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_DEPTH_STENCIL_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateSampler )( 
      ID3D12Device * This,
      _In_  const D3D12_SAMPLER_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CopyDescriptors )( 
      ID3D12Device * This,
      _In_  UINT NumDestDescriptorRanges,
      _In_reads_(NumDestDescriptorRanges)  const D3D12_CPU_DESCRIPTOR_HANDLE *pDestDescriptorRangeStarts,
      _In_reads_opt_(NumDestDescriptorRanges)  const UINT *pDestDescriptorRangeSizes,
      _In_  UINT NumSrcDescriptorRanges,
      _In_reads_(NumSrcDescriptorRanges)  const D3D12_CPU_DESCRIPTOR_HANDLE *pSrcDescriptorRangeStarts,
      _In_reads_opt_(NumSrcDescriptorRanges)  const UINT *pSrcDescriptorRangeSizes,
      _In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType);

    void ( STDMETHODCALLTYPE *CopyDescriptorsSimple )( 
      ID3D12Device * This,
      _In_  UINT NumDescriptors,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptorRangeStart,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRangeStart,
      _In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType);

    D3D12_RESOURCE_ALLOCATION_INFO ( STDMETHODCALLTYPE *GetResourceAllocationInfo )( 
      ID3D12Device * This,
      _In_  UINT visibleMask,
      _In_  UINT numResourceDescs,
      _In_reads_(numResourceDescs)  const D3D12_RESOURCE_DESC *pResourceDescs);

    D3D12_HEAP_PROPERTIES ( STDMETHODCALLTYPE *GetCustomHeapProperties )( 
      ID3D12Device * This,
      _In_  UINT nodeMask,
      D3D12_HEAP_TYPE heapType);

    HRESULT ( STDMETHODCALLTYPE *CreateCommittedResource )( 
      ID3D12Device * This,
      _In_  const D3D12_HEAP_PROPERTIES *pHeapProperties,
      D3D12_HEAP_FLAGS HeapFlags,
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialResourceState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      REFIID riidResource,
      _COM_Outptr_opt_  void **ppvResource);

    HRESULT ( STDMETHODCALLTYPE *CreateHeap )( 
      ID3D12Device * This,
      _In_  const D3D12_HEAP_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvHeap);

    HRESULT ( STDMETHODCALLTYPE *CreatePlacedResource )( 
      ID3D12Device * This,
      _In_  ID3D12Heap *pHeap,
      UINT64 HeapOffset,
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvResource);

    HRESULT ( STDMETHODCALLTYPE *CreateReservedResource )( 
      ID3D12Device * This,
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvResource);

    HRESULT ( STDMETHODCALLTYPE *CreateSharedHandle )( 
      ID3D12Device * This,
      _In_  ID3D12DeviceChild *pObject,
      _In_opt_  const SECURITY_ATTRIBUTES *pAttributes,
      DWORD Access,
      _In_opt_  LPCWSTR Name,
      _Out_  HANDLE *pHandle);

    HRESULT ( STDMETHODCALLTYPE *OpenSharedHandle )( 
      ID3D12Device * This,
      _In_  HANDLE NTHandle,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvObj);

    HRESULT ( STDMETHODCALLTYPE *OpenSharedHandleByName )( 
      ID3D12Device * This,
      _In_  LPCWSTR Name,
      DWORD Access,
      /* [annotation][out] */ 
      _Out_  HANDLE *pNTHandle);

    HRESULT ( STDMETHODCALLTYPE *MakeResident )( 
      ID3D12Device * This,
      UINT NumObjects,
      _In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects);

    HRESULT ( STDMETHODCALLTYPE *Evict )( 
      ID3D12Device * This,
      UINT NumObjects,
      _In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects);

    HRESULT ( STDMETHODCALLTYPE *CreateFence )( 
      ID3D12Device * This,
      UINT64 InitialValue,
      D3D12_FENCE_FLAGS Flags,
      REFIID riid,
      _COM_Outptr_  void **ppFence);

    HRESULT ( STDMETHODCALLTYPE *GetDeviceRemovedReason )( 
      ID3D12Device * This);

    void ( STDMETHODCALLTYPE *GetCopyableFootprints )( 
      ID3D12Device * This,
      _In_  const D3D12_RESOURCE_DESC *pResourceDesc,
      _In_range_(0,D3D12_REQ_SUBRESOURCES)  UINT FirstSubresource,
      _In_range_(0,D3D12_REQ_SUBRESOURCES-FirstSubresource)  UINT NumSubresources,
      UINT64 BaseOffset,
      _Out_writes_opt_(NumSubresources)  D3D12_PLACED_SUBRESOURCE_FOOTPRINT *pLayouts,
      _Out_writes_opt_(NumSubresources)  UINT *pNumRows,
      _Out_writes_opt_(NumSubresources)  UINT64 *pRowSizeInBytes,
      _Out_opt_  UINT64 *pTotalBytes);

    HRESULT ( STDMETHODCALLTYPE *CreateQueryHeap )( 
      ID3D12Device * This,
      _In_  const D3D12_QUERY_HEAP_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvHeap);

    HRESULT ( STDMETHODCALLTYPE *SetStablePowerState )( 
      ID3D12Device * This,
      BOOL Enable);

    HRESULT ( STDMETHODCALLTYPE *CreateCommandSignature )( 
      ID3D12Device * This,
      _In_  const D3D12_COMMAND_SIGNATURE_DESC *pDesc,
      _In_opt_  ID3D12RootSignature *pRootSignature,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvCommandSignature);

    void ( STDMETHODCALLTYPE *GetResourceTiling )( 
      ID3D12Device * This,
      _In_  ID3D12Resource *pTiledResource,
      _Out_opt_  UINT *pNumTilesForEntireResource,
      _Out_opt_  D3D12_PACKED_MIP_INFO *pPackedMipDesc,
      _Out_opt_  D3D12_TILE_SHAPE *pStandardTileShapeForNonPackedMips,
      _Inout_opt_  UINT *pNumSubresourceTilings,
      _In_  UINT FirstSubresourceTilingToGet,
      _Out_writes_(*pNumSubresourceTilings)  D3D12_SUBRESOURCE_TILING *pSubresourceTilingsForNonPackedMips);

    LUID ( STDMETHODCALLTYPE *GetAdapterLuid )( 
      ID3D12Device * This);

    END_INTERFACE
  } ID3D12DeviceVtbl;

  interface ID3D12Device
  {
    CONST_VTBL struct ID3D12DeviceVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12Device_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12Device_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12Device_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12Device_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12Device_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12Device_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12Device_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12Device_GetNodeCount(This)	\
    ( (This)->lpVtbl -> GetNodeCount(This) ) 

#define ID3D12Device_CreateCommandQueue(This,pDesc,riid,ppCommandQueue)	\
    ( (This)->lpVtbl -> CreateCommandQueue(This,pDesc,riid,ppCommandQueue) ) 

#define ID3D12Device_CreateCommandAllocator(This,type,riid,ppCommandAllocator)	\
    ( (This)->lpVtbl -> CreateCommandAllocator(This,type,riid,ppCommandAllocator) ) 

#define ID3D12Device_CreateGraphicsPipelineState(This,pDesc,riid,ppPipelineState)	\
    ( (This)->lpVtbl -> CreateGraphicsPipelineState(This,pDesc,riid,ppPipelineState) ) 

#define ID3D12Device_CreateComputePipelineState(This,pDesc,riid,ppPipelineState)	\
    ( (This)->lpVtbl -> CreateComputePipelineState(This,pDesc,riid,ppPipelineState) ) 

#define ID3D12Device_CreateCommandList(This,nodeMask,type,pCommandAllocator,pInitialState,riid,ppCommandList)	\
    ( (This)->lpVtbl -> CreateCommandList(This,nodeMask,type,pCommandAllocator,pInitialState,riid,ppCommandList) ) 

#define ID3D12Device_CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize)	\
    ( (This)->lpVtbl -> CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize) ) 

#define ID3D12Device_CreateDescriptorHeap(This,pDescriptorHeapDesc,riid,ppvHeap)	\
    ( (This)->lpVtbl -> CreateDescriptorHeap(This,pDescriptorHeapDesc,riid,ppvHeap) ) 

#define ID3D12Device_GetDescriptorHandleIncrementSize(This,DescriptorHeapType)	\
    ( (This)->lpVtbl -> GetDescriptorHandleIncrementSize(This,DescriptorHeapType) ) 

#define ID3D12Device_CreateRootSignature(This,nodeMask,pBlobWithRootSignature,blobLengthInBytes,riid,ppvRootSignature)	\
    ( (This)->lpVtbl -> CreateRootSignature(This,nodeMask,pBlobWithRootSignature,blobLengthInBytes,riid,ppvRootSignature) ) 

#define ID3D12Device_CreateConstantBufferView(This,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateConstantBufferView(This,pDesc,DestDescriptor) ) 

#define ID3D12Device_CreateShaderResourceView(This,pResource,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateShaderResourceView(This,pResource,pDesc,DestDescriptor) ) 

#define ID3D12Device_CreateUnorderedAccessView(This,pResource,pCounterResource,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateUnorderedAccessView(This,pResource,pCounterResource,pDesc,DestDescriptor) ) 

#define ID3D12Device_CreateRenderTargetView(This,pResource,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateRenderTargetView(This,pResource,pDesc,DestDescriptor) ) 

#define ID3D12Device_CreateDepthStencilView(This,pResource,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateDepthStencilView(This,pResource,pDesc,DestDescriptor) ) 

#define ID3D12Device_CreateSampler(This,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateSampler(This,pDesc,DestDescriptor) ) 

#define ID3D12Device_CopyDescriptors(This,NumDestDescriptorRanges,pDestDescriptorRangeStarts,pDestDescriptorRangeSizes,NumSrcDescriptorRanges,pSrcDescriptorRangeStarts,pSrcDescriptorRangeSizes,DescriptorHeapsType)	\
    ( (This)->lpVtbl -> CopyDescriptors(This,NumDestDescriptorRanges,pDestDescriptorRangeStarts,pDestDescriptorRangeSizes,NumSrcDescriptorRanges,pSrcDescriptorRangeStarts,pSrcDescriptorRangeSizes,DescriptorHeapsType) ) 

#define ID3D12Device_CopyDescriptorsSimple(This,NumDescriptors,DestDescriptorRangeStart,SrcDescriptorRangeStart,DescriptorHeapsType)	\
    ( (This)->lpVtbl -> CopyDescriptorsSimple(This,NumDescriptors,DestDescriptorRangeStart,SrcDescriptorRangeStart,DescriptorHeapsType) ) 

#define ID3D12Device_GetResourceAllocationInfo(This,visibleMask,numResourceDescs,pResourceDescs)	\
    ( (This)->lpVtbl -> GetResourceAllocationInfo(This,visibleMask,numResourceDescs,pResourceDescs) ) 

#define ID3D12Device_GetCustomHeapProperties(This,nodeMask,heapType)	\
    ( (This)->lpVtbl -> GetCustomHeapProperties(This,nodeMask,heapType) ) 

#define ID3D12Device_CreateCommittedResource(This,pHeapProperties,HeapFlags,pDesc,InitialResourceState,pOptimizedClearValue,riidResource,ppvResource)	\
    ( (This)->lpVtbl -> CreateCommittedResource(This,pHeapProperties,HeapFlags,pDesc,InitialResourceState,pOptimizedClearValue,riidResource,ppvResource) ) 

#define ID3D12Device_CreateHeap(This,pDesc,riid,ppvHeap)	\
    ( (This)->lpVtbl -> CreateHeap(This,pDesc,riid,ppvHeap) ) 

#define ID3D12Device_CreatePlacedResource(This,pHeap,HeapOffset,pDesc,InitialState,pOptimizedClearValue,riid,ppvResource)	\
    ( (This)->lpVtbl -> CreatePlacedResource(This,pHeap,HeapOffset,pDesc,InitialState,pOptimizedClearValue,riid,ppvResource) ) 

#define ID3D12Device_CreateReservedResource(This,pDesc,InitialState,pOptimizedClearValue,riid,ppvResource)	\
    ( (This)->lpVtbl -> CreateReservedResource(This,pDesc,InitialState,pOptimizedClearValue,riid,ppvResource) ) 

#define ID3D12Device_CreateSharedHandle(This,pObject,pAttributes,Access,Name,pHandle)	\
    ( (This)->lpVtbl -> CreateSharedHandle(This,pObject,pAttributes,Access,Name,pHandle) ) 

#define ID3D12Device_OpenSharedHandle(This,NTHandle,riid,ppvObj)	\
    ( (This)->lpVtbl -> OpenSharedHandle(This,NTHandle,riid,ppvObj) ) 

#define ID3D12Device_OpenSharedHandleByName(This,Name,Access,pNTHandle)	\
    ( (This)->lpVtbl -> OpenSharedHandleByName(This,Name,Access,pNTHandle) ) 

#define ID3D12Device_MakeResident(This,NumObjects,ppObjects)	\
    ( (This)->lpVtbl -> MakeResident(This,NumObjects,ppObjects) ) 

#define ID3D12Device_Evict(This,NumObjects,ppObjects)	\
    ( (This)->lpVtbl -> Evict(This,NumObjects,ppObjects) ) 

#define ID3D12Device_CreateFence(This,InitialValue,Flags,riid,ppFence)	\
    ( (This)->lpVtbl -> CreateFence(This,InitialValue,Flags,riid,ppFence) ) 

#define ID3D12Device_GetDeviceRemovedReason(This)	\
    ( (This)->lpVtbl -> GetDeviceRemovedReason(This) ) 

#define ID3D12Device_GetCopyableFootprints(This,pResourceDesc,FirstSubresource,NumSubresources,BaseOffset,pLayouts,pNumRows,pRowSizeInBytes,pTotalBytes)	\
    ( (This)->lpVtbl -> GetCopyableFootprints(This,pResourceDesc,FirstSubresource,NumSubresources,BaseOffset,pLayouts,pNumRows,pRowSizeInBytes,pTotalBytes) ) 

#define ID3D12Device_CreateQueryHeap(This,pDesc,riid,ppvHeap)	\
    ( (This)->lpVtbl -> CreateQueryHeap(This,pDesc,riid,ppvHeap) ) 

#define ID3D12Device_SetStablePowerState(This,Enable)	\
    ( (This)->lpVtbl -> SetStablePowerState(This,Enable) ) 

#define ID3D12Device_CreateCommandSignature(This,pDesc,pRootSignature,riid,ppvCommandSignature)	\
    ( (This)->lpVtbl -> CreateCommandSignature(This,pDesc,pRootSignature,riid,ppvCommandSignature) ) 

#define ID3D12Device_GetResourceTiling(This,pTiledResource,pNumTilesForEntireResource,pPackedMipDesc,pStandardTileShapeForNonPackedMips,pNumSubresourceTilings,FirstSubresourceTilingToGet,pSubresourceTilingsForNonPackedMips)	\
    ( (This)->lpVtbl -> GetResourceTiling(This,pTiledResource,pNumTilesForEntireResource,pPackedMipDesc,pStandardTileShapeForNonPackedMips,pNumSubresourceTilings,FirstSubresourceTilingToGet,pSubresourceTilingsForNonPackedMips) ) 

#define ID3D12Device_GetAdapterLuid(This)	\
    ( (This)->lpVtbl -> GetAdapterLuid(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12Device_INTERFACE_DEFINED__ */


#ifndef __ID3D12PipelineLibrary_INTERFACE_DEFINED__
#define __ID3D12PipelineLibrary_INTERFACE_DEFINED__

  /* interface ID3D12PipelineLibrary */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12PipelineLibrary;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("c64226a8-9201-46af-b4cc-53fb9ff7414f")
    ID3D12PipelineLibrary : public ID3D12DeviceChild
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE StorePipeline( 
      _In_opt_  LPCWSTR pName,
      _In_  ID3D12PipelineState *pPipeline) = 0;

    virtual HRESULT STDMETHODCALLTYPE LoadGraphicsPipeline( 
      _In_  LPCWSTR pName,
      _In_  const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState) = 0;

    virtual HRESULT STDMETHODCALLTYPE LoadComputePipeline( 
      _In_  LPCWSTR pName,
      _In_  const D3D12_COMPUTE_PIPELINE_STATE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState) = 0;

    virtual SIZE_T STDMETHODCALLTYPE GetSerializedSize( void) = 0;

    virtual HRESULT STDMETHODCALLTYPE Serialize( 
      _Out_writes_(DataSizeInBytes)  void *pData,
      SIZE_T DataSizeInBytes) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12PipelineLibraryVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12PipelineLibrary * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12PipelineLibrary * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12PipelineLibrary * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12PipelineLibrary * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12PipelineLibrary * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12PipelineLibrary * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12PipelineLibrary * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12PipelineLibrary * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    HRESULT ( STDMETHODCALLTYPE *StorePipeline )( 
      ID3D12PipelineLibrary * This,
      _In_opt_  LPCWSTR pName,
      _In_  ID3D12PipelineState *pPipeline);

    HRESULT ( STDMETHODCALLTYPE *LoadGraphicsPipeline )( 
      ID3D12PipelineLibrary * This,
      _In_  LPCWSTR pName,
      _In_  const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState);

    HRESULT ( STDMETHODCALLTYPE *LoadComputePipeline )( 
      ID3D12PipelineLibrary * This,
      _In_  LPCWSTR pName,
      _In_  const D3D12_COMPUTE_PIPELINE_STATE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState);

    SIZE_T ( STDMETHODCALLTYPE *GetSerializedSize )( 
      ID3D12PipelineLibrary * This);

    HRESULT ( STDMETHODCALLTYPE *Serialize )( 
      ID3D12PipelineLibrary * This,
      _Out_writes_(DataSizeInBytes)  void *pData,
      SIZE_T DataSizeInBytes);

    END_INTERFACE
  } ID3D12PipelineLibraryVtbl;

  interface ID3D12PipelineLibrary
  {
    CONST_VTBL struct ID3D12PipelineLibraryVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12PipelineLibrary_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12PipelineLibrary_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12PipelineLibrary_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12PipelineLibrary_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12PipelineLibrary_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12PipelineLibrary_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12PipelineLibrary_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12PipelineLibrary_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 


#define ID3D12PipelineLibrary_StorePipeline(This,pName,pPipeline)	\
    ( (This)->lpVtbl -> StorePipeline(This,pName,pPipeline) ) 

#define ID3D12PipelineLibrary_LoadGraphicsPipeline(This,pName,pDesc,riid,ppPipelineState)	\
    ( (This)->lpVtbl -> LoadGraphicsPipeline(This,pName,pDesc,riid,ppPipelineState) ) 

#define ID3D12PipelineLibrary_LoadComputePipeline(This,pName,pDesc,riid,ppPipelineState)	\
    ( (This)->lpVtbl -> LoadComputePipeline(This,pName,pDesc,riid,ppPipelineState) ) 

#define ID3D12PipelineLibrary_GetSerializedSize(This)	\
    ( (This)->lpVtbl -> GetSerializedSize(This) ) 

#define ID3D12PipelineLibrary_Serialize(This,pData,DataSizeInBytes)	\
    ( (This)->lpVtbl -> Serialize(This,pData,DataSizeInBytes) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12PipelineLibrary_INTERFACE_DEFINED__ */


#ifndef __ID3D12PipelineLibrary1_INTERFACE_DEFINED__
#define __ID3D12PipelineLibrary1_INTERFACE_DEFINED__

  /* interface ID3D12PipelineLibrary1 */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12PipelineLibrary1;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("80eabf42-2568-4e5e-bd82-c37f86961dc3")
    ID3D12PipelineLibrary1 : public ID3D12PipelineLibrary
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE LoadPipeline( 
      _In_  LPCWSTR pName,
      _In_  const D3D12_PIPELINE_STATE_STREAM_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12PipelineLibrary1Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12PipelineLibrary1 * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12PipelineLibrary1 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12PipelineLibrary1 * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12PipelineLibrary1 * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12PipelineLibrary1 * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12PipelineLibrary1 * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12PipelineLibrary1 * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12PipelineLibrary1 * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    HRESULT ( STDMETHODCALLTYPE *StorePipeline )( 
      ID3D12PipelineLibrary1 * This,
      _In_opt_  LPCWSTR pName,
      _In_  ID3D12PipelineState *pPipeline);

    HRESULT ( STDMETHODCALLTYPE *LoadGraphicsPipeline )( 
      ID3D12PipelineLibrary1 * This,
      _In_  LPCWSTR pName,
      _In_  const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState);

    HRESULT ( STDMETHODCALLTYPE *LoadComputePipeline )( 
      ID3D12PipelineLibrary1 * This,
      _In_  LPCWSTR pName,
      _In_  const D3D12_COMPUTE_PIPELINE_STATE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState);

    SIZE_T ( STDMETHODCALLTYPE *GetSerializedSize )( 
      ID3D12PipelineLibrary1 * This);

    HRESULT ( STDMETHODCALLTYPE *Serialize )( 
      ID3D12PipelineLibrary1 * This,
      _Out_writes_(DataSizeInBytes)  void *pData,
      SIZE_T DataSizeInBytes);

    HRESULT ( STDMETHODCALLTYPE *LoadPipeline )( 
      ID3D12PipelineLibrary1 * This,
      _In_  LPCWSTR pName,
      _In_  const D3D12_PIPELINE_STATE_STREAM_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState);

    END_INTERFACE
  } ID3D12PipelineLibrary1Vtbl;

  interface ID3D12PipelineLibrary1
  {
    CONST_VTBL struct ID3D12PipelineLibrary1Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12PipelineLibrary1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12PipelineLibrary1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12PipelineLibrary1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12PipelineLibrary1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12PipelineLibrary1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12PipelineLibrary1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12PipelineLibrary1_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12PipelineLibrary1_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 


#define ID3D12PipelineLibrary1_StorePipeline(This,pName,pPipeline)	\
    ( (This)->lpVtbl -> StorePipeline(This,pName,pPipeline) ) 

#define ID3D12PipelineLibrary1_LoadGraphicsPipeline(This,pName,pDesc,riid,ppPipelineState)	\
    ( (This)->lpVtbl -> LoadGraphicsPipeline(This,pName,pDesc,riid,ppPipelineState) ) 

#define ID3D12PipelineLibrary1_LoadComputePipeline(This,pName,pDesc,riid,ppPipelineState)	\
    ( (This)->lpVtbl -> LoadComputePipeline(This,pName,pDesc,riid,ppPipelineState) ) 

#define ID3D12PipelineLibrary1_GetSerializedSize(This)	\
    ( (This)->lpVtbl -> GetSerializedSize(This) ) 

#define ID3D12PipelineLibrary1_Serialize(This,pData,DataSizeInBytes)	\
    ( (This)->lpVtbl -> Serialize(This,pData,DataSizeInBytes) ) 


#define ID3D12PipelineLibrary1_LoadPipeline(This,pName,pDesc,riid,ppPipelineState)	\
    ( (This)->lpVtbl -> LoadPipeline(This,pName,pDesc,riid,ppPipelineState) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12PipelineLibrary1_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_d3d12_0000_0023 */
  /* [local] */ 

  typedef 
    enum D3D12_MULTIPLE_FENCE_WAIT_FLAGS
  {
    D3D12_MULTIPLE_FENCE_WAIT_FLAG_NONE	= 0,
    D3D12_MULTIPLE_FENCE_WAIT_FLAG_ANY	= 0x1,
    D3D12_MULTIPLE_FENCE_WAIT_FLAG_ALL	= 0
  } 	D3D12_MULTIPLE_FENCE_WAIT_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_MULTIPLE_FENCE_WAIT_FLAGS );
  typedef 
    enum D3D12_RESIDENCY_PRIORITY
  {
    D3D12_RESIDENCY_PRIORITY_MINIMUM	= 0x28000000,
    D3D12_RESIDENCY_PRIORITY_LOW	= 0x50000000,
    D3D12_RESIDENCY_PRIORITY_NORMAL	= 0x78000000,
    D3D12_RESIDENCY_PRIORITY_HIGH	= 0xa0010000,
    D3D12_RESIDENCY_PRIORITY_MAXIMUM	= 0xc8000000
  } 	D3D12_RESIDENCY_PRIORITY;



  extern RPC_IF_HANDLE __MIDL_itf_d3d12_0000_0023_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_d3d12_0000_0023_v0_0_s_ifspec;

#ifndef __ID3D12Device1_INTERFACE_DEFINED__
#define __ID3D12Device1_INTERFACE_DEFINED__

  /* interface ID3D12Device1 */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12Device1;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("77acce80-638e-4e65-8895-c1f23386863e")
    ID3D12Device1 : public ID3D12Device
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE CreatePipelineLibrary( 
      _In_reads_(BlobLength)  const void *pLibraryBlob,
      SIZE_T BlobLength,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineLibrary) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetEventOnMultipleFenceCompletion( 
      _In_reads_(NumFences)  ID3D12Fence *const *ppFences,
      _In_reads_(NumFences)  const UINT64 *pFenceValues,
      UINT NumFences,
      D3D12_MULTIPLE_FENCE_WAIT_FLAGS Flags,
      HANDLE hEvent) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetResidencyPriority( 
      UINT NumObjects,
      _In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects,
      _In_reads_(NumObjects)  const D3D12_RESIDENCY_PRIORITY *pPriorities) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12Device1Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12Device1 * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12Device1 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12Device1 * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12Device1 * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12Device1 * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12Device1 * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12Device1 * This,
      _In_z_  LPCWSTR Name);

    UINT ( STDMETHODCALLTYPE *GetNodeCount )( 
      ID3D12Device1 * This);

    HRESULT ( STDMETHODCALLTYPE *CreateCommandQueue )( 
      ID3D12Device1 * This,
      _In_  const D3D12_COMMAND_QUEUE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppCommandQueue);

    HRESULT ( STDMETHODCALLTYPE *CreateCommandAllocator )( 
      ID3D12Device1 * This,
      _In_  D3D12_COMMAND_LIST_TYPE type,
      REFIID riid,
      _COM_Outptr_  void **ppCommandAllocator);

    HRESULT ( STDMETHODCALLTYPE *CreateGraphicsPipelineState )( 
      ID3D12Device1 * This,
      _In_  const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState);

    HRESULT ( STDMETHODCALLTYPE *CreateComputePipelineState )( 
      ID3D12Device1 * This,
      _In_  const D3D12_COMPUTE_PIPELINE_STATE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState);

    HRESULT ( STDMETHODCALLTYPE *CreateCommandList )( 
      ID3D12Device1 * This,
      _In_  UINT nodeMask,
      _In_  D3D12_COMMAND_LIST_TYPE type,
      _In_  ID3D12CommandAllocator *pCommandAllocator,
      _In_opt_  ID3D12PipelineState *pInitialState,
      REFIID riid,
      _COM_Outptr_  void **ppCommandList);

    HRESULT ( STDMETHODCALLTYPE *CheckFeatureSupport )( 
      ID3D12Device1 * This,
      D3D12_FEATURE Feature,
      _Inout_updates_bytes_(FeatureSupportDataSize)  void *pFeatureSupportData,
      UINT FeatureSupportDataSize);

    HRESULT ( STDMETHODCALLTYPE *CreateDescriptorHeap )( 
      ID3D12Device1 * This,
      _In_  const D3D12_DESCRIPTOR_HEAP_DESC *pDescriptorHeapDesc,
      REFIID riid,
      _COM_Outptr_  void **ppvHeap);

    UINT ( STDMETHODCALLTYPE *GetDescriptorHandleIncrementSize )( 
      ID3D12Device1 * This,
      _In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType);

    HRESULT ( STDMETHODCALLTYPE *CreateRootSignature )( 
      ID3D12Device1 * This,
      _In_  UINT nodeMask,
      _In_reads_(blobLengthInBytes)  const void *pBlobWithRootSignature,
      _In_  SIZE_T blobLengthInBytes,
      REFIID riid,
      _COM_Outptr_  void **ppvRootSignature);

    void ( STDMETHODCALLTYPE *CreateConstantBufferView )( 
      ID3D12Device1 * This,
      _In_opt_  const D3D12_CONSTANT_BUFFER_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateShaderResourceView )( 
      ID3D12Device1 * This,
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_SHADER_RESOURCE_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateUnorderedAccessView )( 
      ID3D12Device1 * This,
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  ID3D12Resource *pCounterResource,
      _In_opt_  const D3D12_UNORDERED_ACCESS_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateRenderTargetView )( 
      ID3D12Device1 * This,
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_RENDER_TARGET_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateDepthStencilView )( 
      ID3D12Device1 * This,
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_DEPTH_STENCIL_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateSampler )( 
      ID3D12Device1 * This,
      _In_  const D3D12_SAMPLER_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CopyDescriptors )( 
      ID3D12Device1 * This,
      _In_  UINT NumDestDescriptorRanges,
      _In_reads_(NumDestDescriptorRanges)  const D3D12_CPU_DESCRIPTOR_HANDLE *pDestDescriptorRangeStarts,
      _In_reads_opt_(NumDestDescriptorRanges)  const UINT *pDestDescriptorRangeSizes,
      _In_  UINT NumSrcDescriptorRanges,
      _In_reads_(NumSrcDescriptorRanges)  const D3D12_CPU_DESCRIPTOR_HANDLE *pSrcDescriptorRangeStarts,
      _In_reads_opt_(NumSrcDescriptorRanges)  const UINT *pSrcDescriptorRangeSizes,
      _In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType);

    void ( STDMETHODCALLTYPE *CopyDescriptorsSimple )( 
      ID3D12Device1 * This,
      _In_  UINT NumDescriptors,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptorRangeStart,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRangeStart,
      _In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType);

    D3D12_RESOURCE_ALLOCATION_INFO ( STDMETHODCALLTYPE *GetResourceAllocationInfo )( 
      ID3D12Device1 * This,
      _In_  UINT visibleMask,
      _In_  UINT numResourceDescs,
      _In_reads_(numResourceDescs)  const D3D12_RESOURCE_DESC *pResourceDescs);

    D3D12_HEAP_PROPERTIES ( STDMETHODCALLTYPE *GetCustomHeapProperties )( 
      ID3D12Device1 * This,
      _In_  UINT nodeMask,
      D3D12_HEAP_TYPE heapType);

    HRESULT ( STDMETHODCALLTYPE *CreateCommittedResource )( 
      ID3D12Device1 * This,
      _In_  const D3D12_HEAP_PROPERTIES *pHeapProperties,
      D3D12_HEAP_FLAGS HeapFlags,
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialResourceState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      REFIID riidResource,
      _COM_Outptr_opt_  void **ppvResource);

    HRESULT ( STDMETHODCALLTYPE *CreateHeap )( 
      ID3D12Device1 * This,
      _In_  const D3D12_HEAP_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvHeap);

    HRESULT ( STDMETHODCALLTYPE *CreatePlacedResource )( 
      ID3D12Device1 * This,
      _In_  ID3D12Heap *pHeap,
      UINT64 HeapOffset,
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvResource);

    HRESULT ( STDMETHODCALLTYPE *CreateReservedResource )( 
      ID3D12Device1 * This,
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvResource);

    HRESULT ( STDMETHODCALLTYPE *CreateSharedHandle )( 
      ID3D12Device1 * This,
      _In_  ID3D12DeviceChild *pObject,
      _In_opt_  const SECURITY_ATTRIBUTES *pAttributes,
      DWORD Access,
      _In_opt_  LPCWSTR Name,
      _Out_  HANDLE *pHandle);

    HRESULT ( STDMETHODCALLTYPE *OpenSharedHandle )( 
      ID3D12Device1 * This,
      _In_  HANDLE NTHandle,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvObj);

    HRESULT ( STDMETHODCALLTYPE *OpenSharedHandleByName )( 
      ID3D12Device1 * This,
      _In_  LPCWSTR Name,
      DWORD Access,
      /* [annotation][out] */ 
      _Out_  HANDLE *pNTHandle);

    HRESULT ( STDMETHODCALLTYPE *MakeResident )( 
      ID3D12Device1 * This,
      UINT NumObjects,
      _In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects);

    HRESULT ( STDMETHODCALLTYPE *Evict )( 
      ID3D12Device1 * This,
      UINT NumObjects,
      _In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects);

    HRESULT ( STDMETHODCALLTYPE *CreateFence )( 
      ID3D12Device1 * This,
      UINT64 InitialValue,
      D3D12_FENCE_FLAGS Flags,
      REFIID riid,
      _COM_Outptr_  void **ppFence);

    HRESULT ( STDMETHODCALLTYPE *GetDeviceRemovedReason )( 
      ID3D12Device1 * This);

    void ( STDMETHODCALLTYPE *GetCopyableFootprints )( 
      ID3D12Device1 * This,
      _In_  const D3D12_RESOURCE_DESC *pResourceDesc,
      _In_range_(0,D3D12_REQ_SUBRESOURCES)  UINT FirstSubresource,
      _In_range_(0,D3D12_REQ_SUBRESOURCES-FirstSubresource)  UINT NumSubresources,
      UINT64 BaseOffset,
      _Out_writes_opt_(NumSubresources)  D3D12_PLACED_SUBRESOURCE_FOOTPRINT *pLayouts,
      _Out_writes_opt_(NumSubresources)  UINT *pNumRows,
      _Out_writes_opt_(NumSubresources)  UINT64 *pRowSizeInBytes,
      _Out_opt_  UINT64 *pTotalBytes);

    HRESULT ( STDMETHODCALLTYPE *CreateQueryHeap )( 
      ID3D12Device1 * This,
      _In_  const D3D12_QUERY_HEAP_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvHeap);

    HRESULT ( STDMETHODCALLTYPE *SetStablePowerState )( 
      ID3D12Device1 * This,
      BOOL Enable);

    HRESULT ( STDMETHODCALLTYPE *CreateCommandSignature )( 
      ID3D12Device1 * This,
      _In_  const D3D12_COMMAND_SIGNATURE_DESC *pDesc,
      _In_opt_  ID3D12RootSignature *pRootSignature,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvCommandSignature);

    void ( STDMETHODCALLTYPE *GetResourceTiling )( 
      ID3D12Device1 * This,
      _In_  ID3D12Resource *pTiledResource,
      _Out_opt_  UINT *pNumTilesForEntireResource,
      _Out_opt_  D3D12_PACKED_MIP_INFO *pPackedMipDesc,
      _Out_opt_  D3D12_TILE_SHAPE *pStandardTileShapeForNonPackedMips,
      _Inout_opt_  UINT *pNumSubresourceTilings,
      _In_  UINT FirstSubresourceTilingToGet,
      _Out_writes_(*pNumSubresourceTilings)  D3D12_SUBRESOURCE_TILING *pSubresourceTilingsForNonPackedMips);

    LUID ( STDMETHODCALLTYPE *GetAdapterLuid )( 
      ID3D12Device1 * This);

    HRESULT ( STDMETHODCALLTYPE *CreatePipelineLibrary )( 
      ID3D12Device1 * This,
      _In_reads_(BlobLength)  const void *pLibraryBlob,
      SIZE_T BlobLength,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineLibrary);

    HRESULT ( STDMETHODCALLTYPE *SetEventOnMultipleFenceCompletion )( 
      ID3D12Device1 * This,
      _In_reads_(NumFences)  ID3D12Fence *const *ppFences,
      _In_reads_(NumFences)  const UINT64 *pFenceValues,
      UINT NumFences,
      D3D12_MULTIPLE_FENCE_WAIT_FLAGS Flags,
      HANDLE hEvent);

    HRESULT ( STDMETHODCALLTYPE *SetResidencyPriority )( 
      ID3D12Device1 * This,
      UINT NumObjects,
      _In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects,
      _In_reads_(NumObjects)  const D3D12_RESIDENCY_PRIORITY *pPriorities);

    END_INTERFACE
  } ID3D12Device1Vtbl;

  interface ID3D12Device1
  {
    CONST_VTBL struct ID3D12Device1Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12Device1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12Device1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12Device1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12Device1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12Device1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12Device1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12Device1_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12Device1_GetNodeCount(This)	\
    ( (This)->lpVtbl -> GetNodeCount(This) ) 

#define ID3D12Device1_CreateCommandQueue(This,pDesc,riid,ppCommandQueue)	\
    ( (This)->lpVtbl -> CreateCommandQueue(This,pDesc,riid,ppCommandQueue) ) 

#define ID3D12Device1_CreateCommandAllocator(This,type,riid,ppCommandAllocator)	\
    ( (This)->lpVtbl -> CreateCommandAllocator(This,type,riid,ppCommandAllocator) ) 

#define ID3D12Device1_CreateGraphicsPipelineState(This,pDesc,riid,ppPipelineState)	\
    ( (This)->lpVtbl -> CreateGraphicsPipelineState(This,pDesc,riid,ppPipelineState) ) 

#define ID3D12Device1_CreateComputePipelineState(This,pDesc,riid,ppPipelineState)	\
    ( (This)->lpVtbl -> CreateComputePipelineState(This,pDesc,riid,ppPipelineState) ) 

#define ID3D12Device1_CreateCommandList(This,nodeMask,type,pCommandAllocator,pInitialState,riid,ppCommandList)	\
    ( (This)->lpVtbl -> CreateCommandList(This,nodeMask,type,pCommandAllocator,pInitialState,riid,ppCommandList) ) 

#define ID3D12Device1_CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize)	\
    ( (This)->lpVtbl -> CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize) ) 

#define ID3D12Device1_CreateDescriptorHeap(This,pDescriptorHeapDesc,riid,ppvHeap)	\
    ( (This)->lpVtbl -> CreateDescriptorHeap(This,pDescriptorHeapDesc,riid,ppvHeap) ) 

#define ID3D12Device1_GetDescriptorHandleIncrementSize(This,DescriptorHeapType)	\
    ( (This)->lpVtbl -> GetDescriptorHandleIncrementSize(This,DescriptorHeapType) ) 

#define ID3D12Device1_CreateRootSignature(This,nodeMask,pBlobWithRootSignature,blobLengthInBytes,riid,ppvRootSignature)	\
    ( (This)->lpVtbl -> CreateRootSignature(This,nodeMask,pBlobWithRootSignature,blobLengthInBytes,riid,ppvRootSignature) ) 

#define ID3D12Device1_CreateConstantBufferView(This,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateConstantBufferView(This,pDesc,DestDescriptor) ) 

#define ID3D12Device1_CreateShaderResourceView(This,pResource,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateShaderResourceView(This,pResource,pDesc,DestDescriptor) ) 

#define ID3D12Device1_CreateUnorderedAccessView(This,pResource,pCounterResource,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateUnorderedAccessView(This,pResource,pCounterResource,pDesc,DestDescriptor) ) 

#define ID3D12Device1_CreateRenderTargetView(This,pResource,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateRenderTargetView(This,pResource,pDesc,DestDescriptor) ) 

#define ID3D12Device1_CreateDepthStencilView(This,pResource,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateDepthStencilView(This,pResource,pDesc,DestDescriptor) ) 

#define ID3D12Device1_CreateSampler(This,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateSampler(This,pDesc,DestDescriptor) ) 

#define ID3D12Device1_CopyDescriptors(This,NumDestDescriptorRanges,pDestDescriptorRangeStarts,pDestDescriptorRangeSizes,NumSrcDescriptorRanges,pSrcDescriptorRangeStarts,pSrcDescriptorRangeSizes,DescriptorHeapsType)	\
    ( (This)->lpVtbl -> CopyDescriptors(This,NumDestDescriptorRanges,pDestDescriptorRangeStarts,pDestDescriptorRangeSizes,NumSrcDescriptorRanges,pSrcDescriptorRangeStarts,pSrcDescriptorRangeSizes,DescriptorHeapsType) ) 

#define ID3D12Device1_CopyDescriptorsSimple(This,NumDescriptors,DestDescriptorRangeStart,SrcDescriptorRangeStart,DescriptorHeapsType)	\
    ( (This)->lpVtbl -> CopyDescriptorsSimple(This,NumDescriptors,DestDescriptorRangeStart,SrcDescriptorRangeStart,DescriptorHeapsType) ) 

#define ID3D12Device1_GetResourceAllocationInfo(This,visibleMask,numResourceDescs,pResourceDescs)	\
    ( (This)->lpVtbl -> GetResourceAllocationInfo(This,visibleMask,numResourceDescs,pResourceDescs) ) 

#define ID3D12Device1_GetCustomHeapProperties(This,nodeMask,heapType)	\
    ( (This)->lpVtbl -> GetCustomHeapProperties(This,nodeMask,heapType) ) 

#define ID3D12Device1_CreateCommittedResource(This,pHeapProperties,HeapFlags,pDesc,InitialResourceState,pOptimizedClearValue,riidResource,ppvResource)	\
    ( (This)->lpVtbl -> CreateCommittedResource(This,pHeapProperties,HeapFlags,pDesc,InitialResourceState,pOptimizedClearValue,riidResource,ppvResource) ) 

#define ID3D12Device1_CreateHeap(This,pDesc,riid,ppvHeap)	\
    ( (This)->lpVtbl -> CreateHeap(This,pDesc,riid,ppvHeap) ) 

#define ID3D12Device1_CreatePlacedResource(This,pHeap,HeapOffset,pDesc,InitialState,pOptimizedClearValue,riid,ppvResource)	\
    ( (This)->lpVtbl -> CreatePlacedResource(This,pHeap,HeapOffset,pDesc,InitialState,pOptimizedClearValue,riid,ppvResource) ) 

#define ID3D12Device1_CreateReservedResource(This,pDesc,InitialState,pOptimizedClearValue,riid,ppvResource)	\
    ( (This)->lpVtbl -> CreateReservedResource(This,pDesc,InitialState,pOptimizedClearValue,riid,ppvResource) ) 

#define ID3D12Device1_CreateSharedHandle(This,pObject,pAttributes,Access,Name,pHandle)	\
    ( (This)->lpVtbl -> CreateSharedHandle(This,pObject,pAttributes,Access,Name,pHandle) ) 

#define ID3D12Device1_OpenSharedHandle(This,NTHandle,riid,ppvObj)	\
    ( (This)->lpVtbl -> OpenSharedHandle(This,NTHandle,riid,ppvObj) ) 

#define ID3D12Device1_OpenSharedHandleByName(This,Name,Access,pNTHandle)	\
    ( (This)->lpVtbl -> OpenSharedHandleByName(This,Name,Access,pNTHandle) ) 

#define ID3D12Device1_MakeResident(This,NumObjects,ppObjects)	\
    ( (This)->lpVtbl -> MakeResident(This,NumObjects,ppObjects) ) 

#define ID3D12Device1_Evict(This,NumObjects,ppObjects)	\
    ( (This)->lpVtbl -> Evict(This,NumObjects,ppObjects) ) 

#define ID3D12Device1_CreateFence(This,InitialValue,Flags,riid,ppFence)	\
    ( (This)->lpVtbl -> CreateFence(This,InitialValue,Flags,riid,ppFence) ) 

#define ID3D12Device1_GetDeviceRemovedReason(This)	\
    ( (This)->lpVtbl -> GetDeviceRemovedReason(This) ) 

#define ID3D12Device1_GetCopyableFootprints(This,pResourceDesc,FirstSubresource,NumSubresources,BaseOffset,pLayouts,pNumRows,pRowSizeInBytes,pTotalBytes)	\
    ( (This)->lpVtbl -> GetCopyableFootprints(This,pResourceDesc,FirstSubresource,NumSubresources,BaseOffset,pLayouts,pNumRows,pRowSizeInBytes,pTotalBytes) ) 

#define ID3D12Device1_CreateQueryHeap(This,pDesc,riid,ppvHeap)	\
    ( (This)->lpVtbl -> CreateQueryHeap(This,pDesc,riid,ppvHeap) ) 

#define ID3D12Device1_SetStablePowerState(This,Enable)	\
    ( (This)->lpVtbl -> SetStablePowerState(This,Enable) ) 

#define ID3D12Device1_CreateCommandSignature(This,pDesc,pRootSignature,riid,ppvCommandSignature)	\
    ( (This)->lpVtbl -> CreateCommandSignature(This,pDesc,pRootSignature,riid,ppvCommandSignature) ) 

#define ID3D12Device1_GetResourceTiling(This,pTiledResource,pNumTilesForEntireResource,pPackedMipDesc,pStandardTileShapeForNonPackedMips,pNumSubresourceTilings,FirstSubresourceTilingToGet,pSubresourceTilingsForNonPackedMips)	\
    ( (This)->lpVtbl -> GetResourceTiling(This,pTiledResource,pNumTilesForEntireResource,pPackedMipDesc,pStandardTileShapeForNonPackedMips,pNumSubresourceTilings,FirstSubresourceTilingToGet,pSubresourceTilingsForNonPackedMips) ) 

#define ID3D12Device1_GetAdapterLuid(This)	\
    ( (This)->lpVtbl -> GetAdapterLuid(This) ) 


#define ID3D12Device1_CreatePipelineLibrary(This,pLibraryBlob,BlobLength,riid,ppPipelineLibrary)	\
    ( (This)->lpVtbl -> CreatePipelineLibrary(This,pLibraryBlob,BlobLength,riid,ppPipelineLibrary) ) 

#define ID3D12Device1_SetEventOnMultipleFenceCompletion(This,ppFences,pFenceValues,NumFences,Flags,hEvent)	\
    ( (This)->lpVtbl -> SetEventOnMultipleFenceCompletion(This,ppFences,pFenceValues,NumFences,Flags,hEvent) ) 

#define ID3D12Device1_SetResidencyPriority(This,NumObjects,ppObjects,pPriorities)	\
    ( (This)->lpVtbl -> SetResidencyPriority(This,NumObjects,ppObjects,pPriorities) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12Device1_INTERFACE_DEFINED__ */


#ifndef __ID3D12Device2_INTERFACE_DEFINED__
#define __ID3D12Device2_INTERFACE_DEFINED__

  /* interface ID3D12Device2 */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12Device2;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("30baa41e-b15b-475c-a0bb-1af5c5b64328")
    ID3D12Device2 : public ID3D12Device1
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE CreatePipelineState( 
      const D3D12_PIPELINE_STATE_STREAM_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12Device2Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12Device2 * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12Device2 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12Device2 * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12Device2 * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12Device2 * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12Device2 * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12Device2 * This,
      _In_z_  LPCWSTR Name);

    UINT ( STDMETHODCALLTYPE *GetNodeCount )( 
      ID3D12Device2 * This);

    HRESULT ( STDMETHODCALLTYPE *CreateCommandQueue )( 
      ID3D12Device2 * This,
      _In_  const D3D12_COMMAND_QUEUE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppCommandQueue);

    HRESULT ( STDMETHODCALLTYPE *CreateCommandAllocator )( 
      ID3D12Device2 * This,
      _In_  D3D12_COMMAND_LIST_TYPE type,
      REFIID riid,
      _COM_Outptr_  void **ppCommandAllocator);

    HRESULT ( STDMETHODCALLTYPE *CreateGraphicsPipelineState )( 
      ID3D12Device2 * This,
      _In_  const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState);

    HRESULT ( STDMETHODCALLTYPE *CreateComputePipelineState )( 
      ID3D12Device2 * This,
      _In_  const D3D12_COMPUTE_PIPELINE_STATE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState);

    HRESULT ( STDMETHODCALLTYPE *CreateCommandList )( 
      ID3D12Device2 * This,
      _In_  UINT nodeMask,
      _In_  D3D12_COMMAND_LIST_TYPE type,
      _In_  ID3D12CommandAllocator *pCommandAllocator,
      _In_opt_  ID3D12PipelineState *pInitialState,
      REFIID riid,
      _COM_Outptr_  void **ppCommandList);

    HRESULT ( STDMETHODCALLTYPE *CheckFeatureSupport )( 
      ID3D12Device2 * This,
      D3D12_FEATURE Feature,
      _Inout_updates_bytes_(FeatureSupportDataSize)  void *pFeatureSupportData,
      UINT FeatureSupportDataSize);

    HRESULT ( STDMETHODCALLTYPE *CreateDescriptorHeap )( 
      ID3D12Device2 * This,
      _In_  const D3D12_DESCRIPTOR_HEAP_DESC *pDescriptorHeapDesc,
      REFIID riid,
      _COM_Outptr_  void **ppvHeap);

    UINT ( STDMETHODCALLTYPE *GetDescriptorHandleIncrementSize )( 
      ID3D12Device2 * This,
      _In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType);

    HRESULT ( STDMETHODCALLTYPE *CreateRootSignature )( 
      ID3D12Device2 * This,
      _In_  UINT nodeMask,
      _In_reads_(blobLengthInBytes)  const void *pBlobWithRootSignature,
      _In_  SIZE_T blobLengthInBytes,
      REFIID riid,
      _COM_Outptr_  void **ppvRootSignature);

    void ( STDMETHODCALLTYPE *CreateConstantBufferView )( 
      ID3D12Device2 * This,
      _In_opt_  const D3D12_CONSTANT_BUFFER_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateShaderResourceView )( 
      ID3D12Device2 * This,
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_SHADER_RESOURCE_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateUnorderedAccessView )( 
      ID3D12Device2 * This,
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  ID3D12Resource *pCounterResource,
      _In_opt_  const D3D12_UNORDERED_ACCESS_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateRenderTargetView )( 
      ID3D12Device2 * This,
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_RENDER_TARGET_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateDepthStencilView )( 
      ID3D12Device2 * This,
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_DEPTH_STENCIL_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateSampler )( 
      ID3D12Device2 * This,
      _In_  const D3D12_SAMPLER_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CopyDescriptors )( 
      ID3D12Device2 * This,
      _In_  UINT NumDestDescriptorRanges,
      _In_reads_(NumDestDescriptorRanges)  const D3D12_CPU_DESCRIPTOR_HANDLE *pDestDescriptorRangeStarts,
      _In_reads_opt_(NumDestDescriptorRanges)  const UINT *pDestDescriptorRangeSizes,
      _In_  UINT NumSrcDescriptorRanges,
      _In_reads_(NumSrcDescriptorRanges)  const D3D12_CPU_DESCRIPTOR_HANDLE *pSrcDescriptorRangeStarts,
      _In_reads_opt_(NumSrcDescriptorRanges)  const UINT *pSrcDescriptorRangeSizes,
      _In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType);

    void ( STDMETHODCALLTYPE *CopyDescriptorsSimple )( 
      ID3D12Device2 * This,
      _In_  UINT NumDescriptors,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptorRangeStart,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRangeStart,
      _In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType);

    D3D12_RESOURCE_ALLOCATION_INFO ( STDMETHODCALLTYPE *GetResourceAllocationInfo )( 
      ID3D12Device2 * This,
      _In_  UINT visibleMask,
      _In_  UINT numResourceDescs,
      _In_reads_(numResourceDescs)  const D3D12_RESOURCE_DESC *pResourceDescs);

    D3D12_HEAP_PROPERTIES ( STDMETHODCALLTYPE *GetCustomHeapProperties )( 
      ID3D12Device2 * This,
      _In_  UINT nodeMask,
      D3D12_HEAP_TYPE heapType);

    HRESULT ( STDMETHODCALLTYPE *CreateCommittedResource )( 
      ID3D12Device2 * This,
      _In_  const D3D12_HEAP_PROPERTIES *pHeapProperties,
      D3D12_HEAP_FLAGS HeapFlags,
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialResourceState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      REFIID riidResource,
      _COM_Outptr_opt_  void **ppvResource);

    HRESULT ( STDMETHODCALLTYPE *CreateHeap )( 
      ID3D12Device2 * This,
      _In_  const D3D12_HEAP_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvHeap);

    HRESULT ( STDMETHODCALLTYPE *CreatePlacedResource )( 
      ID3D12Device2 * This,
      _In_  ID3D12Heap *pHeap,
      UINT64 HeapOffset,
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvResource);

    HRESULT ( STDMETHODCALLTYPE *CreateReservedResource )( 
      ID3D12Device2 * This,
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvResource);

    HRESULT ( STDMETHODCALLTYPE *CreateSharedHandle )( 
      ID3D12Device2 * This,
      _In_  ID3D12DeviceChild *pObject,
      _In_opt_  const SECURITY_ATTRIBUTES *pAttributes,
      DWORD Access,
      _In_opt_  LPCWSTR Name,
      _Out_  HANDLE *pHandle);

    HRESULT ( STDMETHODCALLTYPE *OpenSharedHandle )( 
      ID3D12Device2 * This,
      _In_  HANDLE NTHandle,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvObj);

    HRESULT ( STDMETHODCALLTYPE *OpenSharedHandleByName )( 
      ID3D12Device2 * This,
      _In_  LPCWSTR Name,
      DWORD Access,
      /* [annotation][out] */ 
      _Out_  HANDLE *pNTHandle);

    HRESULT ( STDMETHODCALLTYPE *MakeResident )( 
      ID3D12Device2 * This,
      UINT NumObjects,
      _In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects);

    HRESULT ( STDMETHODCALLTYPE *Evict )( 
      ID3D12Device2 * This,
      UINT NumObjects,
      _In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects);

    HRESULT ( STDMETHODCALLTYPE *CreateFence )( 
      ID3D12Device2 * This,
      UINT64 InitialValue,
      D3D12_FENCE_FLAGS Flags,
      REFIID riid,
      _COM_Outptr_  void **ppFence);

    HRESULT ( STDMETHODCALLTYPE *GetDeviceRemovedReason )( 
      ID3D12Device2 * This);

    void ( STDMETHODCALLTYPE *GetCopyableFootprints )( 
      ID3D12Device2 * This,
      _In_  const D3D12_RESOURCE_DESC *pResourceDesc,
      _In_range_(0,D3D12_REQ_SUBRESOURCES)  UINT FirstSubresource,
      _In_range_(0,D3D12_REQ_SUBRESOURCES-FirstSubresource)  UINT NumSubresources,
      UINT64 BaseOffset,
      _Out_writes_opt_(NumSubresources)  D3D12_PLACED_SUBRESOURCE_FOOTPRINT *pLayouts,
      _Out_writes_opt_(NumSubresources)  UINT *pNumRows,
      _Out_writes_opt_(NumSubresources)  UINT64 *pRowSizeInBytes,
      _Out_opt_  UINT64 *pTotalBytes);

    HRESULT ( STDMETHODCALLTYPE *CreateQueryHeap )( 
      ID3D12Device2 * This,
      _In_  const D3D12_QUERY_HEAP_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvHeap);

    HRESULT ( STDMETHODCALLTYPE *SetStablePowerState )( 
      ID3D12Device2 * This,
      BOOL Enable);

    HRESULT ( STDMETHODCALLTYPE *CreateCommandSignature )( 
      ID3D12Device2 * This,
      _In_  const D3D12_COMMAND_SIGNATURE_DESC *pDesc,
      _In_opt_  ID3D12RootSignature *pRootSignature,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvCommandSignature);

    void ( STDMETHODCALLTYPE *GetResourceTiling )( 
      ID3D12Device2 * This,
      _In_  ID3D12Resource *pTiledResource,
      _Out_opt_  UINT *pNumTilesForEntireResource,
      _Out_opt_  D3D12_PACKED_MIP_INFO *pPackedMipDesc,
      _Out_opt_  D3D12_TILE_SHAPE *pStandardTileShapeForNonPackedMips,
      _Inout_opt_  UINT *pNumSubresourceTilings,
      _In_  UINT FirstSubresourceTilingToGet,
      _Out_writes_(*pNumSubresourceTilings)  D3D12_SUBRESOURCE_TILING *pSubresourceTilingsForNonPackedMips);

    LUID ( STDMETHODCALLTYPE *GetAdapterLuid )( 
      ID3D12Device2 * This);

    HRESULT ( STDMETHODCALLTYPE *CreatePipelineLibrary )( 
      ID3D12Device2 * This,
      _In_reads_(BlobLength)  const void *pLibraryBlob,
      SIZE_T BlobLength,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineLibrary);

    HRESULT ( STDMETHODCALLTYPE *SetEventOnMultipleFenceCompletion )( 
      ID3D12Device2 * This,
      _In_reads_(NumFences)  ID3D12Fence *const *ppFences,
      _In_reads_(NumFences)  const UINT64 *pFenceValues,
      UINT NumFences,
      D3D12_MULTIPLE_FENCE_WAIT_FLAGS Flags,
      HANDLE hEvent);

    HRESULT ( STDMETHODCALLTYPE *SetResidencyPriority )( 
      ID3D12Device2 * This,
      UINT NumObjects,
      _In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects,
      _In_reads_(NumObjects)  const D3D12_RESIDENCY_PRIORITY *pPriorities);

    HRESULT ( STDMETHODCALLTYPE *CreatePipelineState )( 
      ID3D12Device2 * This,
      const D3D12_PIPELINE_STATE_STREAM_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState);

    END_INTERFACE
  } ID3D12Device2Vtbl;

  interface ID3D12Device2
  {
    CONST_VTBL struct ID3D12Device2Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12Device2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12Device2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12Device2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12Device2_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12Device2_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12Device2_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12Device2_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12Device2_GetNodeCount(This)	\
    ( (This)->lpVtbl -> GetNodeCount(This) ) 

#define ID3D12Device2_CreateCommandQueue(This,pDesc,riid,ppCommandQueue)	\
    ( (This)->lpVtbl -> CreateCommandQueue(This,pDesc,riid,ppCommandQueue) ) 

#define ID3D12Device2_CreateCommandAllocator(This,type,riid,ppCommandAllocator)	\
    ( (This)->lpVtbl -> CreateCommandAllocator(This,type,riid,ppCommandAllocator) ) 

#define ID3D12Device2_CreateGraphicsPipelineState(This,pDesc,riid,ppPipelineState)	\
    ( (This)->lpVtbl -> CreateGraphicsPipelineState(This,pDesc,riid,ppPipelineState) ) 

#define ID3D12Device2_CreateComputePipelineState(This,pDesc,riid,ppPipelineState)	\
    ( (This)->lpVtbl -> CreateComputePipelineState(This,pDesc,riid,ppPipelineState) ) 

#define ID3D12Device2_CreateCommandList(This,nodeMask,type,pCommandAllocator,pInitialState,riid,ppCommandList)	\
    ( (This)->lpVtbl -> CreateCommandList(This,nodeMask,type,pCommandAllocator,pInitialState,riid,ppCommandList) ) 

#define ID3D12Device2_CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize)	\
    ( (This)->lpVtbl -> CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize) ) 

#define ID3D12Device2_CreateDescriptorHeap(This,pDescriptorHeapDesc,riid,ppvHeap)	\
    ( (This)->lpVtbl -> CreateDescriptorHeap(This,pDescriptorHeapDesc,riid,ppvHeap) ) 

#define ID3D12Device2_GetDescriptorHandleIncrementSize(This,DescriptorHeapType)	\
    ( (This)->lpVtbl -> GetDescriptorHandleIncrementSize(This,DescriptorHeapType) ) 

#define ID3D12Device2_CreateRootSignature(This,nodeMask,pBlobWithRootSignature,blobLengthInBytes,riid,ppvRootSignature)	\
    ( (This)->lpVtbl -> CreateRootSignature(This,nodeMask,pBlobWithRootSignature,blobLengthInBytes,riid,ppvRootSignature) ) 

#define ID3D12Device2_CreateConstantBufferView(This,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateConstantBufferView(This,pDesc,DestDescriptor) ) 

#define ID3D12Device2_CreateShaderResourceView(This,pResource,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateShaderResourceView(This,pResource,pDesc,DestDescriptor) ) 

#define ID3D12Device2_CreateUnorderedAccessView(This,pResource,pCounterResource,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateUnorderedAccessView(This,pResource,pCounterResource,pDesc,DestDescriptor) ) 

#define ID3D12Device2_CreateRenderTargetView(This,pResource,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateRenderTargetView(This,pResource,pDesc,DestDescriptor) ) 

#define ID3D12Device2_CreateDepthStencilView(This,pResource,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateDepthStencilView(This,pResource,pDesc,DestDescriptor) ) 

#define ID3D12Device2_CreateSampler(This,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateSampler(This,pDesc,DestDescriptor) ) 

#define ID3D12Device2_CopyDescriptors(This,NumDestDescriptorRanges,pDestDescriptorRangeStarts,pDestDescriptorRangeSizes,NumSrcDescriptorRanges,pSrcDescriptorRangeStarts,pSrcDescriptorRangeSizes,DescriptorHeapsType)	\
    ( (This)->lpVtbl -> CopyDescriptors(This,NumDestDescriptorRanges,pDestDescriptorRangeStarts,pDestDescriptorRangeSizes,NumSrcDescriptorRanges,pSrcDescriptorRangeStarts,pSrcDescriptorRangeSizes,DescriptorHeapsType) ) 

#define ID3D12Device2_CopyDescriptorsSimple(This,NumDescriptors,DestDescriptorRangeStart,SrcDescriptorRangeStart,DescriptorHeapsType)	\
    ( (This)->lpVtbl -> CopyDescriptorsSimple(This,NumDescriptors,DestDescriptorRangeStart,SrcDescriptorRangeStart,DescriptorHeapsType) ) 

#define ID3D12Device2_GetResourceAllocationInfo(This,visibleMask,numResourceDescs,pResourceDescs)	\
    ( (This)->lpVtbl -> GetResourceAllocationInfo(This,visibleMask,numResourceDescs,pResourceDescs) ) 

#define ID3D12Device2_GetCustomHeapProperties(This,nodeMask,heapType)	\
    ( (This)->lpVtbl -> GetCustomHeapProperties(This,nodeMask,heapType) ) 

#define ID3D12Device2_CreateCommittedResource(This,pHeapProperties,HeapFlags,pDesc,InitialResourceState,pOptimizedClearValue,riidResource,ppvResource)	\
    ( (This)->lpVtbl -> CreateCommittedResource(This,pHeapProperties,HeapFlags,pDesc,InitialResourceState,pOptimizedClearValue,riidResource,ppvResource) ) 

#define ID3D12Device2_CreateHeap(This,pDesc,riid,ppvHeap)	\
    ( (This)->lpVtbl -> CreateHeap(This,pDesc,riid,ppvHeap) ) 

#define ID3D12Device2_CreatePlacedResource(This,pHeap,HeapOffset,pDesc,InitialState,pOptimizedClearValue,riid,ppvResource)	\
    ( (This)->lpVtbl -> CreatePlacedResource(This,pHeap,HeapOffset,pDesc,InitialState,pOptimizedClearValue,riid,ppvResource) ) 

#define ID3D12Device2_CreateReservedResource(This,pDesc,InitialState,pOptimizedClearValue,riid,ppvResource)	\
    ( (This)->lpVtbl -> CreateReservedResource(This,pDesc,InitialState,pOptimizedClearValue,riid,ppvResource) ) 

#define ID3D12Device2_CreateSharedHandle(This,pObject,pAttributes,Access,Name,pHandle)	\
    ( (This)->lpVtbl -> CreateSharedHandle(This,pObject,pAttributes,Access,Name,pHandle) ) 

#define ID3D12Device2_OpenSharedHandle(This,NTHandle,riid,ppvObj)	\
    ( (This)->lpVtbl -> OpenSharedHandle(This,NTHandle,riid,ppvObj) ) 

#define ID3D12Device2_OpenSharedHandleByName(This,Name,Access,pNTHandle)	\
    ( (This)->lpVtbl -> OpenSharedHandleByName(This,Name,Access,pNTHandle) ) 

#define ID3D12Device2_MakeResident(This,NumObjects,ppObjects)	\
    ( (This)->lpVtbl -> MakeResident(This,NumObjects,ppObjects) ) 

#define ID3D12Device2_Evict(This,NumObjects,ppObjects)	\
    ( (This)->lpVtbl -> Evict(This,NumObjects,ppObjects) ) 

#define ID3D12Device2_CreateFence(This,InitialValue,Flags,riid,ppFence)	\
    ( (This)->lpVtbl -> CreateFence(This,InitialValue,Flags,riid,ppFence) ) 

#define ID3D12Device2_GetDeviceRemovedReason(This)	\
    ( (This)->lpVtbl -> GetDeviceRemovedReason(This) ) 

#define ID3D12Device2_GetCopyableFootprints(This,pResourceDesc,FirstSubresource,NumSubresources,BaseOffset,pLayouts,pNumRows,pRowSizeInBytes,pTotalBytes)	\
    ( (This)->lpVtbl -> GetCopyableFootprints(This,pResourceDesc,FirstSubresource,NumSubresources,BaseOffset,pLayouts,pNumRows,pRowSizeInBytes,pTotalBytes) ) 

#define ID3D12Device2_CreateQueryHeap(This,pDesc,riid,ppvHeap)	\
    ( (This)->lpVtbl -> CreateQueryHeap(This,pDesc,riid,ppvHeap) ) 

#define ID3D12Device2_SetStablePowerState(This,Enable)	\
    ( (This)->lpVtbl -> SetStablePowerState(This,Enable) ) 

#define ID3D12Device2_CreateCommandSignature(This,pDesc,pRootSignature,riid,ppvCommandSignature)	\
    ( (This)->lpVtbl -> CreateCommandSignature(This,pDesc,pRootSignature,riid,ppvCommandSignature) ) 

#define ID3D12Device2_GetResourceTiling(This,pTiledResource,pNumTilesForEntireResource,pPackedMipDesc,pStandardTileShapeForNonPackedMips,pNumSubresourceTilings,FirstSubresourceTilingToGet,pSubresourceTilingsForNonPackedMips)	\
    ( (This)->lpVtbl -> GetResourceTiling(This,pTiledResource,pNumTilesForEntireResource,pPackedMipDesc,pStandardTileShapeForNonPackedMips,pNumSubresourceTilings,FirstSubresourceTilingToGet,pSubresourceTilingsForNonPackedMips) ) 

#define ID3D12Device2_GetAdapterLuid(This)	\
    ( (This)->lpVtbl -> GetAdapterLuid(This) ) 


#define ID3D12Device2_CreatePipelineLibrary(This,pLibraryBlob,BlobLength,riid,ppPipelineLibrary)	\
    ( (This)->lpVtbl -> CreatePipelineLibrary(This,pLibraryBlob,BlobLength,riid,ppPipelineLibrary) ) 

#define ID3D12Device2_SetEventOnMultipleFenceCompletion(This,ppFences,pFenceValues,NumFences,Flags,hEvent)	\
    ( (This)->lpVtbl -> SetEventOnMultipleFenceCompletion(This,ppFences,pFenceValues,NumFences,Flags,hEvent) ) 

#define ID3D12Device2_SetResidencyPriority(This,NumObjects,ppObjects,pPriorities)	\
    ( (This)->lpVtbl -> SetResidencyPriority(This,NumObjects,ppObjects,pPriorities) ) 


#define ID3D12Device2_CreatePipelineState(This,pDesc,riid,ppPipelineState)	\
    ( (This)->lpVtbl -> CreatePipelineState(This,pDesc,riid,ppPipelineState) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12Device2_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_d3d12_0000_0025 */
  /* [local] */ 

  typedef 
    enum D3D12_RESIDENCY_FLAGS
  {
    D3D12_RESIDENCY_FLAG_NONE	= 0,
    D3D12_RESIDENCY_FLAG_DENY_OVERBUDGET	= 0x1
  } 	D3D12_RESIDENCY_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_RESIDENCY_FLAGS );


  extern RPC_IF_HANDLE __MIDL_itf_d3d12_0000_0025_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_d3d12_0000_0025_v0_0_s_ifspec;

#ifndef __ID3D12Device3_INTERFACE_DEFINED__
#define __ID3D12Device3_INTERFACE_DEFINED__

  /* interface ID3D12Device3 */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12Device3;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("81dadc15-2bad-4392-93c5-101345c4aa98")
    ID3D12Device3 : public ID3D12Device2
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE OpenExistingHeapFromAddress( 
      _In_  const void *pAddress,
      REFIID riid,
      _COM_Outptr_  void **ppvHeap) = 0;

    virtual HRESULT STDMETHODCALLTYPE OpenExistingHeapFromFileMapping( 
      _In_  HANDLE hFileMapping,
      REFIID riid,
      _COM_Outptr_  void **ppvHeap) = 0;

    virtual HRESULT STDMETHODCALLTYPE EnqueueMakeResident( 
      D3D12_RESIDENCY_FLAGS Flags,
      UINT NumObjects,
      _In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects,
      _In_  ID3D12Fence *pFenceToSignal,
      UINT64 FenceValueToSignal) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12Device3Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12Device3 * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12Device3 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12Device3 * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12Device3 * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12Device3 * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12Device3 * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12Device3 * This,
      _In_z_  LPCWSTR Name);

    UINT ( STDMETHODCALLTYPE *GetNodeCount )( 
      ID3D12Device3 * This);

    HRESULT ( STDMETHODCALLTYPE *CreateCommandQueue )( 
      ID3D12Device3 * This,
      _In_  const D3D12_COMMAND_QUEUE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppCommandQueue);

    HRESULT ( STDMETHODCALLTYPE *CreateCommandAllocator )( 
      ID3D12Device3 * This,
      _In_  D3D12_COMMAND_LIST_TYPE type,
      REFIID riid,
      _COM_Outptr_  void **ppCommandAllocator);

    HRESULT ( STDMETHODCALLTYPE *CreateGraphicsPipelineState )( 
      ID3D12Device3 * This,
      _In_  const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState);

    HRESULT ( STDMETHODCALLTYPE *CreateComputePipelineState )( 
      ID3D12Device3 * This,
      _In_  const D3D12_COMPUTE_PIPELINE_STATE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState);

    HRESULT ( STDMETHODCALLTYPE *CreateCommandList )( 
      ID3D12Device3 * This,
      _In_  UINT nodeMask,
      _In_  D3D12_COMMAND_LIST_TYPE type,
      _In_  ID3D12CommandAllocator *pCommandAllocator,
      _In_opt_  ID3D12PipelineState *pInitialState,
      REFIID riid,
      _COM_Outptr_  void **ppCommandList);

    HRESULT ( STDMETHODCALLTYPE *CheckFeatureSupport )( 
      ID3D12Device3 * This,
      D3D12_FEATURE Feature,
      _Inout_updates_bytes_(FeatureSupportDataSize)  void *pFeatureSupportData,
      UINT FeatureSupportDataSize);

    HRESULT ( STDMETHODCALLTYPE *CreateDescriptorHeap )( 
      ID3D12Device3 * This,
      _In_  const D3D12_DESCRIPTOR_HEAP_DESC *pDescriptorHeapDesc,
      REFIID riid,
      _COM_Outptr_  void **ppvHeap);

    UINT ( STDMETHODCALLTYPE *GetDescriptorHandleIncrementSize )( 
      ID3D12Device3 * This,
      _In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType);

    HRESULT ( STDMETHODCALLTYPE *CreateRootSignature )( 
      ID3D12Device3 * This,
      _In_  UINT nodeMask,
      _In_reads_(blobLengthInBytes)  const void *pBlobWithRootSignature,
      _In_  SIZE_T blobLengthInBytes,
      REFIID riid,
      _COM_Outptr_  void **ppvRootSignature);

    void ( STDMETHODCALLTYPE *CreateConstantBufferView )( 
      ID3D12Device3 * This,
      _In_opt_  const D3D12_CONSTANT_BUFFER_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateShaderResourceView )( 
      ID3D12Device3 * This,
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_SHADER_RESOURCE_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateUnorderedAccessView )( 
      ID3D12Device3 * This,
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  ID3D12Resource *pCounterResource,
      _In_opt_  const D3D12_UNORDERED_ACCESS_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateRenderTargetView )( 
      ID3D12Device3 * This,
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_RENDER_TARGET_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateDepthStencilView )( 
      ID3D12Device3 * This,
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_DEPTH_STENCIL_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateSampler )( 
      ID3D12Device3 * This,
      _In_  const D3D12_SAMPLER_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CopyDescriptors )( 
      ID3D12Device3 * This,
      _In_  UINT NumDestDescriptorRanges,
      _In_reads_(NumDestDescriptorRanges)  const D3D12_CPU_DESCRIPTOR_HANDLE *pDestDescriptorRangeStarts,
      _In_reads_opt_(NumDestDescriptorRanges)  const UINT *pDestDescriptorRangeSizes,
      _In_  UINT NumSrcDescriptorRanges,
      _In_reads_(NumSrcDescriptorRanges)  const D3D12_CPU_DESCRIPTOR_HANDLE *pSrcDescriptorRangeStarts,
      _In_reads_opt_(NumSrcDescriptorRanges)  const UINT *pSrcDescriptorRangeSizes,
      _In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType);

    void ( STDMETHODCALLTYPE *CopyDescriptorsSimple )( 
      ID3D12Device3 * This,
      _In_  UINT NumDescriptors,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptorRangeStart,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRangeStart,
      _In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType);

    D3D12_RESOURCE_ALLOCATION_INFO ( STDMETHODCALLTYPE *GetResourceAllocationInfo )( 
      ID3D12Device3 * This,
      _In_  UINT visibleMask,
      _In_  UINT numResourceDescs,
      _In_reads_(numResourceDescs)  const D3D12_RESOURCE_DESC *pResourceDescs);

    D3D12_HEAP_PROPERTIES ( STDMETHODCALLTYPE *GetCustomHeapProperties )( 
      ID3D12Device3 * This,
      _In_  UINT nodeMask,
      D3D12_HEAP_TYPE heapType);

    HRESULT ( STDMETHODCALLTYPE *CreateCommittedResource )( 
      ID3D12Device3 * This,
      _In_  const D3D12_HEAP_PROPERTIES *pHeapProperties,
      D3D12_HEAP_FLAGS HeapFlags,
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialResourceState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      REFIID riidResource,
      _COM_Outptr_opt_  void **ppvResource);

    HRESULT ( STDMETHODCALLTYPE *CreateHeap )( 
      ID3D12Device3 * This,
      _In_  const D3D12_HEAP_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvHeap);

    HRESULT ( STDMETHODCALLTYPE *CreatePlacedResource )( 
      ID3D12Device3 * This,
      _In_  ID3D12Heap *pHeap,
      UINT64 HeapOffset,
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvResource);

    HRESULT ( STDMETHODCALLTYPE *CreateReservedResource )( 
      ID3D12Device3 * This,
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvResource);

    HRESULT ( STDMETHODCALLTYPE *CreateSharedHandle )( 
      ID3D12Device3 * This,
      _In_  ID3D12DeviceChild *pObject,
      _In_opt_  const SECURITY_ATTRIBUTES *pAttributes,
      DWORD Access,
      _In_opt_  LPCWSTR Name,
      _Out_  HANDLE *pHandle);

    HRESULT ( STDMETHODCALLTYPE *OpenSharedHandle )( 
      ID3D12Device3 * This,
      _In_  HANDLE NTHandle,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvObj);

    HRESULT ( STDMETHODCALLTYPE *OpenSharedHandleByName )( 
      ID3D12Device3 * This,
      _In_  LPCWSTR Name,
      DWORD Access,
      /* [annotation][out] */ 
      _Out_  HANDLE *pNTHandle);

    HRESULT ( STDMETHODCALLTYPE *MakeResident )( 
      ID3D12Device3 * This,
      UINT NumObjects,
      _In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects);

    HRESULT ( STDMETHODCALLTYPE *Evict )( 
      ID3D12Device3 * This,
      UINT NumObjects,
      _In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects);

    HRESULT ( STDMETHODCALLTYPE *CreateFence )( 
      ID3D12Device3 * This,
      UINT64 InitialValue,
      D3D12_FENCE_FLAGS Flags,
      REFIID riid,
      _COM_Outptr_  void **ppFence);

    HRESULT ( STDMETHODCALLTYPE *GetDeviceRemovedReason )( 
      ID3D12Device3 * This);

    void ( STDMETHODCALLTYPE *GetCopyableFootprints )( 
      ID3D12Device3 * This,
      _In_  const D3D12_RESOURCE_DESC *pResourceDesc,
      _In_range_(0,D3D12_REQ_SUBRESOURCES)  UINT FirstSubresource,
      _In_range_(0,D3D12_REQ_SUBRESOURCES-FirstSubresource)  UINT NumSubresources,
      UINT64 BaseOffset,
      _Out_writes_opt_(NumSubresources)  D3D12_PLACED_SUBRESOURCE_FOOTPRINT *pLayouts,
      _Out_writes_opt_(NumSubresources)  UINT *pNumRows,
      _Out_writes_opt_(NumSubresources)  UINT64 *pRowSizeInBytes,
      _Out_opt_  UINT64 *pTotalBytes);

    HRESULT ( STDMETHODCALLTYPE *CreateQueryHeap )( 
      ID3D12Device3 * This,
      _In_  const D3D12_QUERY_HEAP_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvHeap);

    HRESULT ( STDMETHODCALLTYPE *SetStablePowerState )( 
      ID3D12Device3 * This,
      BOOL Enable);

    HRESULT ( STDMETHODCALLTYPE *CreateCommandSignature )( 
      ID3D12Device3 * This,
      _In_  const D3D12_COMMAND_SIGNATURE_DESC *pDesc,
      _In_opt_  ID3D12RootSignature *pRootSignature,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvCommandSignature);

    void ( STDMETHODCALLTYPE *GetResourceTiling )( 
      ID3D12Device3 * This,
      _In_  ID3D12Resource *pTiledResource,
      _Out_opt_  UINT *pNumTilesForEntireResource,
      _Out_opt_  D3D12_PACKED_MIP_INFO *pPackedMipDesc,
      _Out_opt_  D3D12_TILE_SHAPE *pStandardTileShapeForNonPackedMips,
      _Inout_opt_  UINT *pNumSubresourceTilings,
      _In_  UINT FirstSubresourceTilingToGet,
      _Out_writes_(*pNumSubresourceTilings)  D3D12_SUBRESOURCE_TILING *pSubresourceTilingsForNonPackedMips);

    LUID ( STDMETHODCALLTYPE *GetAdapterLuid )( 
      ID3D12Device3 * This);

    HRESULT ( STDMETHODCALLTYPE *CreatePipelineLibrary )( 
      ID3D12Device3 * This,
      _In_reads_(BlobLength)  const void *pLibraryBlob,
      SIZE_T BlobLength,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineLibrary);

    HRESULT ( STDMETHODCALLTYPE *SetEventOnMultipleFenceCompletion )( 
      ID3D12Device3 * This,
      _In_reads_(NumFences)  ID3D12Fence *const *ppFences,
      _In_reads_(NumFences)  const UINT64 *pFenceValues,
      UINT NumFences,
      D3D12_MULTIPLE_FENCE_WAIT_FLAGS Flags,
      HANDLE hEvent);

    HRESULT ( STDMETHODCALLTYPE *SetResidencyPriority )( 
      ID3D12Device3 * This,
      UINT NumObjects,
      _In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects,
      _In_reads_(NumObjects)  const D3D12_RESIDENCY_PRIORITY *pPriorities);

    HRESULT ( STDMETHODCALLTYPE *CreatePipelineState )( 
      ID3D12Device3 * This,
      const D3D12_PIPELINE_STATE_STREAM_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState);

    HRESULT ( STDMETHODCALLTYPE *OpenExistingHeapFromAddress )( 
      ID3D12Device3 * This,
      _In_  const void *pAddress,
      REFIID riid,
      _COM_Outptr_  void **ppvHeap);

    HRESULT ( STDMETHODCALLTYPE *OpenExistingHeapFromFileMapping )( 
      ID3D12Device3 * This,
      _In_  HANDLE hFileMapping,
      REFIID riid,
      _COM_Outptr_  void **ppvHeap);

    HRESULT ( STDMETHODCALLTYPE *EnqueueMakeResident )( 
      ID3D12Device3 * This,
      D3D12_RESIDENCY_FLAGS Flags,
      UINT NumObjects,
      _In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects,
      _In_  ID3D12Fence *pFenceToSignal,
      UINT64 FenceValueToSignal);

    END_INTERFACE
  } ID3D12Device3Vtbl;

  interface ID3D12Device3
  {
    CONST_VTBL struct ID3D12Device3Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12Device3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12Device3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12Device3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12Device3_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12Device3_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12Device3_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12Device3_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12Device3_GetNodeCount(This)	\
    ( (This)->lpVtbl -> GetNodeCount(This) ) 

#define ID3D12Device3_CreateCommandQueue(This,pDesc,riid,ppCommandQueue)	\
    ( (This)->lpVtbl -> CreateCommandQueue(This,pDesc,riid,ppCommandQueue) ) 

#define ID3D12Device3_CreateCommandAllocator(This,type,riid,ppCommandAllocator)	\
    ( (This)->lpVtbl -> CreateCommandAllocator(This,type,riid,ppCommandAllocator) ) 

#define ID3D12Device3_CreateGraphicsPipelineState(This,pDesc,riid,ppPipelineState)	\
    ( (This)->lpVtbl -> CreateGraphicsPipelineState(This,pDesc,riid,ppPipelineState) ) 

#define ID3D12Device3_CreateComputePipelineState(This,pDesc,riid,ppPipelineState)	\
    ( (This)->lpVtbl -> CreateComputePipelineState(This,pDesc,riid,ppPipelineState) ) 

#define ID3D12Device3_CreateCommandList(This,nodeMask,type,pCommandAllocator,pInitialState,riid,ppCommandList)	\
    ( (This)->lpVtbl -> CreateCommandList(This,nodeMask,type,pCommandAllocator,pInitialState,riid,ppCommandList) ) 

#define ID3D12Device3_CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize)	\
    ( (This)->lpVtbl -> CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize) ) 

#define ID3D12Device3_CreateDescriptorHeap(This,pDescriptorHeapDesc,riid,ppvHeap)	\
    ( (This)->lpVtbl -> CreateDescriptorHeap(This,pDescriptorHeapDesc,riid,ppvHeap) ) 

#define ID3D12Device3_GetDescriptorHandleIncrementSize(This,DescriptorHeapType)	\
    ( (This)->lpVtbl -> GetDescriptorHandleIncrementSize(This,DescriptorHeapType) ) 

#define ID3D12Device3_CreateRootSignature(This,nodeMask,pBlobWithRootSignature,blobLengthInBytes,riid,ppvRootSignature)	\
    ( (This)->lpVtbl -> CreateRootSignature(This,nodeMask,pBlobWithRootSignature,blobLengthInBytes,riid,ppvRootSignature) ) 

#define ID3D12Device3_CreateConstantBufferView(This,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateConstantBufferView(This,pDesc,DestDescriptor) ) 

#define ID3D12Device3_CreateShaderResourceView(This,pResource,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateShaderResourceView(This,pResource,pDesc,DestDescriptor) ) 

#define ID3D12Device3_CreateUnorderedAccessView(This,pResource,pCounterResource,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateUnorderedAccessView(This,pResource,pCounterResource,pDesc,DestDescriptor) ) 

#define ID3D12Device3_CreateRenderTargetView(This,pResource,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateRenderTargetView(This,pResource,pDesc,DestDescriptor) ) 

#define ID3D12Device3_CreateDepthStencilView(This,pResource,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateDepthStencilView(This,pResource,pDesc,DestDescriptor) ) 

#define ID3D12Device3_CreateSampler(This,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateSampler(This,pDesc,DestDescriptor) ) 

#define ID3D12Device3_CopyDescriptors(This,NumDestDescriptorRanges,pDestDescriptorRangeStarts,pDestDescriptorRangeSizes,NumSrcDescriptorRanges,pSrcDescriptorRangeStarts,pSrcDescriptorRangeSizes,DescriptorHeapsType)	\
    ( (This)->lpVtbl -> CopyDescriptors(This,NumDestDescriptorRanges,pDestDescriptorRangeStarts,pDestDescriptorRangeSizes,NumSrcDescriptorRanges,pSrcDescriptorRangeStarts,pSrcDescriptorRangeSizes,DescriptorHeapsType) ) 

#define ID3D12Device3_CopyDescriptorsSimple(This,NumDescriptors,DestDescriptorRangeStart,SrcDescriptorRangeStart,DescriptorHeapsType)	\
    ( (This)->lpVtbl -> CopyDescriptorsSimple(This,NumDescriptors,DestDescriptorRangeStart,SrcDescriptorRangeStart,DescriptorHeapsType) ) 

#define ID3D12Device3_GetResourceAllocationInfo(This,visibleMask,numResourceDescs,pResourceDescs)	\
    ( (This)->lpVtbl -> GetResourceAllocationInfo(This,visibleMask,numResourceDescs,pResourceDescs) ) 

#define ID3D12Device3_GetCustomHeapProperties(This,nodeMask,heapType)	\
    ( (This)->lpVtbl -> GetCustomHeapProperties(This,nodeMask,heapType) ) 

#define ID3D12Device3_CreateCommittedResource(This,pHeapProperties,HeapFlags,pDesc,InitialResourceState,pOptimizedClearValue,riidResource,ppvResource)	\
    ( (This)->lpVtbl -> CreateCommittedResource(This,pHeapProperties,HeapFlags,pDesc,InitialResourceState,pOptimizedClearValue,riidResource,ppvResource) ) 

#define ID3D12Device3_CreateHeap(This,pDesc,riid,ppvHeap)	\
    ( (This)->lpVtbl -> CreateHeap(This,pDesc,riid,ppvHeap) ) 

#define ID3D12Device3_CreatePlacedResource(This,pHeap,HeapOffset,pDesc,InitialState,pOptimizedClearValue,riid,ppvResource)	\
    ( (This)->lpVtbl -> CreatePlacedResource(This,pHeap,HeapOffset,pDesc,InitialState,pOptimizedClearValue,riid,ppvResource) ) 

#define ID3D12Device3_CreateReservedResource(This,pDesc,InitialState,pOptimizedClearValue,riid,ppvResource)	\
    ( (This)->lpVtbl -> CreateReservedResource(This,pDesc,InitialState,pOptimizedClearValue,riid,ppvResource) ) 

#define ID3D12Device3_CreateSharedHandle(This,pObject,pAttributes,Access,Name,pHandle)	\
    ( (This)->lpVtbl -> CreateSharedHandle(This,pObject,pAttributes,Access,Name,pHandle) ) 

#define ID3D12Device3_OpenSharedHandle(This,NTHandle,riid,ppvObj)	\
    ( (This)->lpVtbl -> OpenSharedHandle(This,NTHandle,riid,ppvObj) ) 

#define ID3D12Device3_OpenSharedHandleByName(This,Name,Access,pNTHandle)	\
    ( (This)->lpVtbl -> OpenSharedHandleByName(This,Name,Access,pNTHandle) ) 

#define ID3D12Device3_MakeResident(This,NumObjects,ppObjects)	\
    ( (This)->lpVtbl -> MakeResident(This,NumObjects,ppObjects) ) 

#define ID3D12Device3_Evict(This,NumObjects,ppObjects)	\
    ( (This)->lpVtbl -> Evict(This,NumObjects,ppObjects) ) 

#define ID3D12Device3_CreateFence(This,InitialValue,Flags,riid,ppFence)	\
    ( (This)->lpVtbl -> CreateFence(This,InitialValue,Flags,riid,ppFence) ) 

#define ID3D12Device3_GetDeviceRemovedReason(This)	\
    ( (This)->lpVtbl -> GetDeviceRemovedReason(This) ) 

#define ID3D12Device3_GetCopyableFootprints(This,pResourceDesc,FirstSubresource,NumSubresources,BaseOffset,pLayouts,pNumRows,pRowSizeInBytes,pTotalBytes)	\
    ( (This)->lpVtbl -> GetCopyableFootprints(This,pResourceDesc,FirstSubresource,NumSubresources,BaseOffset,pLayouts,pNumRows,pRowSizeInBytes,pTotalBytes) ) 

#define ID3D12Device3_CreateQueryHeap(This,pDesc,riid,ppvHeap)	\
    ( (This)->lpVtbl -> CreateQueryHeap(This,pDesc,riid,ppvHeap) ) 

#define ID3D12Device3_SetStablePowerState(This,Enable)	\
    ( (This)->lpVtbl -> SetStablePowerState(This,Enable) ) 

#define ID3D12Device3_CreateCommandSignature(This,pDesc,pRootSignature,riid,ppvCommandSignature)	\
    ( (This)->lpVtbl -> CreateCommandSignature(This,pDesc,pRootSignature,riid,ppvCommandSignature) ) 

#define ID3D12Device3_GetResourceTiling(This,pTiledResource,pNumTilesForEntireResource,pPackedMipDesc,pStandardTileShapeForNonPackedMips,pNumSubresourceTilings,FirstSubresourceTilingToGet,pSubresourceTilingsForNonPackedMips)	\
    ( (This)->lpVtbl -> GetResourceTiling(This,pTiledResource,pNumTilesForEntireResource,pPackedMipDesc,pStandardTileShapeForNonPackedMips,pNumSubresourceTilings,FirstSubresourceTilingToGet,pSubresourceTilingsForNonPackedMips) ) 

#define ID3D12Device3_GetAdapterLuid(This)	\
    ( (This)->lpVtbl -> GetAdapterLuid(This) ) 


#define ID3D12Device3_CreatePipelineLibrary(This,pLibraryBlob,BlobLength,riid,ppPipelineLibrary)	\
    ( (This)->lpVtbl -> CreatePipelineLibrary(This,pLibraryBlob,BlobLength,riid,ppPipelineLibrary) ) 

#define ID3D12Device3_SetEventOnMultipleFenceCompletion(This,ppFences,pFenceValues,NumFences,Flags,hEvent)	\
    ( (This)->lpVtbl -> SetEventOnMultipleFenceCompletion(This,ppFences,pFenceValues,NumFences,Flags,hEvent) ) 

#define ID3D12Device3_SetResidencyPriority(This,NumObjects,ppObjects,pPriorities)	\
    ( (This)->lpVtbl -> SetResidencyPriority(This,NumObjects,ppObjects,pPriorities) ) 


#define ID3D12Device3_CreatePipelineState(This,pDesc,riid,ppPipelineState)	\
    ( (This)->lpVtbl -> CreatePipelineState(This,pDesc,riid,ppPipelineState) ) 


#define ID3D12Device3_OpenExistingHeapFromAddress(This,pAddress,riid,ppvHeap)	\
    ( (This)->lpVtbl -> OpenExistingHeapFromAddress(This,pAddress,riid,ppvHeap) ) 

#define ID3D12Device3_OpenExistingHeapFromFileMapping(This,hFileMapping,riid,ppvHeap)	\
    ( (This)->lpVtbl -> OpenExistingHeapFromFileMapping(This,hFileMapping,riid,ppvHeap) ) 

#define ID3D12Device3_EnqueueMakeResident(This,Flags,NumObjects,ppObjects,pFenceToSignal,FenceValueToSignal)	\
    ( (This)->lpVtbl -> EnqueueMakeResident(This,Flags,NumObjects,ppObjects,pFenceToSignal,FenceValueToSignal) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12Device3_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_d3d12_0000_0026 */
  /* [local] */ 

  typedef 
    enum D3D12_COMMAND_LIST_FLAGS
  {
    D3D12_COMMAND_LIST_FLAG_NONE	= 0
  } 	D3D12_COMMAND_LIST_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_COMMAND_LIST_FLAGS );
  typedef 
    enum D3D12_COMMAND_POOL_FLAGS
  {
    D3D12_COMMAND_POOL_FLAG_NONE	= 0
  } 	D3D12_COMMAND_POOL_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_COMMAND_POOL_FLAGS );
  typedef 
    enum D3D12_COMMAND_RECORDER_FLAGS
  {
    D3D12_COMMAND_RECORDER_FLAG_NONE	= 0
  } 	D3D12_COMMAND_RECORDER_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_COMMAND_RECORDER_FLAGS );
  typedef 
    enum D3D12_PROTECTED_SESSION_STATUS
  {
    D3D12_PROTECTED_SESSION_STATUS_OK	= 0,
    D3D12_PROTECTED_SESSION_STATUS_INVALID	= 1
  } 	D3D12_PROTECTED_SESSION_STATUS;



  extern RPC_IF_HANDLE __MIDL_itf_d3d12_0000_0026_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_d3d12_0000_0026_v0_0_s_ifspec;

#ifndef __ID3D12ProtectedSession_INTERFACE_DEFINED__
#define __ID3D12ProtectedSession_INTERFACE_DEFINED__

  /* interface ID3D12ProtectedSession */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12ProtectedSession;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("A1533D18-0AC1-4084-85B9-89A96116806B")
    ID3D12ProtectedSession : public ID3D12DeviceChild
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetStatusFence( 
      REFIID riid,
      _COM_Outptr_opt_  void **ppFence) = 0;

    virtual D3D12_PROTECTED_SESSION_STATUS STDMETHODCALLTYPE GetSessionStatus( void) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12ProtectedSessionVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12ProtectedSession * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12ProtectedSession * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12ProtectedSession * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12ProtectedSession * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12ProtectedSession * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12ProtectedSession * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12ProtectedSession * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12ProtectedSession * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    HRESULT ( STDMETHODCALLTYPE *GetStatusFence )( 
      ID3D12ProtectedSession * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppFence);

    D3D12_PROTECTED_SESSION_STATUS ( STDMETHODCALLTYPE *GetSessionStatus )( 
      ID3D12ProtectedSession * This);

    END_INTERFACE
  } ID3D12ProtectedSessionVtbl;

  interface ID3D12ProtectedSession
  {
    CONST_VTBL struct ID3D12ProtectedSessionVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12ProtectedSession_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12ProtectedSession_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12ProtectedSession_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12ProtectedSession_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12ProtectedSession_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12ProtectedSession_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12ProtectedSession_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12ProtectedSession_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 


#define ID3D12ProtectedSession_GetStatusFence(This,riid,ppFence)	\
    ( (This)->lpVtbl -> GetStatusFence(This,riid,ppFence) ) 

#define ID3D12ProtectedSession_GetSessionStatus(This)	\
    ( (This)->lpVtbl -> GetSessionStatus(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12ProtectedSession_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_d3d12_0000_0027 */
  /* [local] */ 

  typedef 
    enum D3D12_PROTECTED_RESOURCE_SESSION_SUPPORT_FLAGS
  {
    D3D12_PROTECTED_RESOURCE_SESSION_SUPPORT_FLAG_NONE	= 0,
    D3D12_PROTECTED_RESOURCE_SESSION_SUPPORT_FLAG_SUPPORTED	= 0x1
  } 	D3D12_PROTECTED_RESOURCE_SESSION_SUPPORT_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_PROTECTED_RESOURCE_SESSION_SUPPORT_FLAGS );
  typedef struct D3D12_FEATURE_DATA_PROTECTED_RESOURCE_SESSION_SUPPORT
  {
    UINT NodeIndex;
    D3D12_PROTECTED_RESOURCE_SESSION_SUPPORT_FLAGS Support;
  } 	D3D12_FEATURE_DATA_PROTECTED_RESOURCE_SESSION_SUPPORT;

  typedef 
    enum D3D12_PROTECTED_RESOURCE_SESSION_FLAGS
  {
    D3D12_PROTECTED_RESOURCE_SESSION_FLAG_NONE	= 0
  } 	D3D12_PROTECTED_RESOURCE_SESSION_FLAGS;

  DEFINE_ENUM_FLAG_OPERATORS( D3D12_PROTECTED_RESOURCE_SESSION_FLAGS );
  typedef struct D3D12_PROTECTED_RESOURCE_SESSION_DESC
  {
    UINT NodeMask;
    D3D12_PROTECTED_RESOURCE_SESSION_FLAGS Flags;
  } 	D3D12_PROTECTED_RESOURCE_SESSION_DESC;



  extern RPC_IF_HANDLE __MIDL_itf_d3d12_0000_0027_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_d3d12_0000_0027_v0_0_s_ifspec;

#ifndef __ID3D12ProtectedResourceSession_INTERFACE_DEFINED__
#define __ID3D12ProtectedResourceSession_INTERFACE_DEFINED__

  /* interface ID3D12ProtectedResourceSession */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12ProtectedResourceSession;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("6CD696F4-F289-40CC-8091-5A6C0A099C3D")
    ID3D12ProtectedResourceSession : public ID3D12ProtectedSession
  {
  public:
    virtual D3D12_PROTECTED_RESOURCE_SESSION_DESC STDMETHODCALLTYPE GetDesc( void) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12ProtectedResourceSessionVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12ProtectedResourceSession * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12ProtectedResourceSession * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12ProtectedResourceSession * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12ProtectedResourceSession * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12ProtectedResourceSession * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12ProtectedResourceSession * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12ProtectedResourceSession * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12ProtectedResourceSession * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    HRESULT ( STDMETHODCALLTYPE *GetStatusFence )( 
      ID3D12ProtectedResourceSession * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppFence);

    D3D12_PROTECTED_SESSION_STATUS ( STDMETHODCALLTYPE *GetSessionStatus )( 
      ID3D12ProtectedResourceSession * This);

    D3D12_PROTECTED_RESOURCE_SESSION_DESC ( STDMETHODCALLTYPE *GetDesc )( 
      ID3D12ProtectedResourceSession * This);

    END_INTERFACE
  } ID3D12ProtectedResourceSessionVtbl;

  interface ID3D12ProtectedResourceSession
  {
    CONST_VTBL struct ID3D12ProtectedResourceSessionVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12ProtectedResourceSession_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12ProtectedResourceSession_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12ProtectedResourceSession_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12ProtectedResourceSession_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12ProtectedResourceSession_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12ProtectedResourceSession_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12ProtectedResourceSession_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12ProtectedResourceSession_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 


#define ID3D12ProtectedResourceSession_GetStatusFence(This,riid,ppFence)	\
    ( (This)->lpVtbl -> GetStatusFence(This,riid,ppFence) ) 

#define ID3D12ProtectedResourceSession_GetSessionStatus(This)	\
    ( (This)->lpVtbl -> GetSessionStatus(This) ) 


#define ID3D12ProtectedResourceSession_GetDesc(This)	\
    ( (This)->lpVtbl -> GetDesc(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12ProtectedResourceSession_INTERFACE_DEFINED__ */


#ifndef __ID3D12Device4_INTERFACE_DEFINED__
#define __ID3D12Device4_INTERFACE_DEFINED__

  /* interface ID3D12Device4 */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12Device4;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("e865df17-a9ee-46f9-a463-3098315aa2e5")
    ID3D12Device4 : public ID3D12Device3
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE CreateCommandList1( 
      _In_  UINT nodeMask,
      _In_  D3D12_COMMAND_LIST_TYPE type,
      _In_  D3D12_COMMAND_LIST_FLAGS flags,
      REFIID riid,
      _COM_Outptr_  void **ppCommandList) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateProtectedResourceSession( 
      _In_  const D3D12_PROTECTED_RESOURCE_SESSION_DESC *pDesc,
      _In_  REFIID riid,
      _COM_Outptr_  void **ppSession) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateCommittedResource1( 
      _In_  const D3D12_HEAP_PROPERTIES *pHeapProperties,
      D3D12_HEAP_FLAGS HeapFlags,
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialResourceState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      _In_opt_  ID3D12ProtectedResourceSession *pProtectedSession,
      REFIID riidResource,
      _COM_Outptr_opt_  void **ppvResource) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateHeap1( 
      _In_  const D3D12_HEAP_DESC *pDesc,
      _In_opt_  ID3D12ProtectedResourceSession *pProtectedSession,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvHeap) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateReservedResource1( 
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      _In_opt_  ID3D12ProtectedResourceSession *pProtectedSession,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvResource) = 0;

    virtual D3D12_RESOURCE_ALLOCATION_INFO STDMETHODCALLTYPE GetResourceAllocationInfo1( 
      UINT visibleMask,
      UINT numResourceDescs,
      _In_reads_(numResourceDescs)  const D3D12_RESOURCE_DESC *pResourceDescs,
      _Out_writes_opt_(numResourceDescs)  D3D12_RESOURCE_ALLOCATION_INFO1 *pResourceAllocationInfo1) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12Device4Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12Device4 * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12Device4 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12Device4 * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12Device4 * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12Device4 * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12Device4 * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12Device4 * This,
      _In_z_  LPCWSTR Name);

    UINT ( STDMETHODCALLTYPE *GetNodeCount )( 
      ID3D12Device4 * This);

    HRESULT ( STDMETHODCALLTYPE *CreateCommandQueue )( 
      ID3D12Device4 * This,
      _In_  const D3D12_COMMAND_QUEUE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppCommandQueue);

    HRESULT ( STDMETHODCALLTYPE *CreateCommandAllocator )( 
      ID3D12Device4 * This,
      _In_  D3D12_COMMAND_LIST_TYPE type,
      REFIID riid,
      _COM_Outptr_  void **ppCommandAllocator);

    HRESULT ( STDMETHODCALLTYPE *CreateGraphicsPipelineState )( 
      ID3D12Device4 * This,
      _In_  const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState);

    HRESULT ( STDMETHODCALLTYPE *CreateComputePipelineState )( 
      ID3D12Device4 * This,
      _In_  const D3D12_COMPUTE_PIPELINE_STATE_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState);

    HRESULT ( STDMETHODCALLTYPE *CreateCommandList )( 
      ID3D12Device4 * This,
      _In_  UINT nodeMask,
      _In_  D3D12_COMMAND_LIST_TYPE type,
      _In_  ID3D12CommandAllocator *pCommandAllocator,
      _In_opt_  ID3D12PipelineState *pInitialState,
      REFIID riid,
      _COM_Outptr_  void **ppCommandList);

    HRESULT ( STDMETHODCALLTYPE *CheckFeatureSupport )( 
      ID3D12Device4 * This,
      D3D12_FEATURE Feature,
      _Inout_updates_bytes_(FeatureSupportDataSize)  void *pFeatureSupportData,
      UINT FeatureSupportDataSize);

    HRESULT ( STDMETHODCALLTYPE *CreateDescriptorHeap )( 
      ID3D12Device4 * This,
      _In_  const D3D12_DESCRIPTOR_HEAP_DESC *pDescriptorHeapDesc,
      REFIID riid,
      _COM_Outptr_  void **ppvHeap);

    UINT ( STDMETHODCALLTYPE *GetDescriptorHandleIncrementSize )( 
      ID3D12Device4 * This,
      _In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType);

    HRESULT ( STDMETHODCALLTYPE *CreateRootSignature )( 
      ID3D12Device4 * This,
      _In_  UINT nodeMask,
      _In_reads_(blobLengthInBytes)  const void *pBlobWithRootSignature,
      _In_  SIZE_T blobLengthInBytes,
      REFIID riid,
      _COM_Outptr_  void **ppvRootSignature);

    void ( STDMETHODCALLTYPE *CreateConstantBufferView )( 
      ID3D12Device4 * This,
      _In_opt_  const D3D12_CONSTANT_BUFFER_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateShaderResourceView )( 
      ID3D12Device4 * This,
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_SHADER_RESOURCE_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateUnorderedAccessView )( 
      ID3D12Device4 * This,
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  ID3D12Resource *pCounterResource,
      _In_opt_  const D3D12_UNORDERED_ACCESS_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateRenderTargetView )( 
      ID3D12Device4 * This,
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_RENDER_TARGET_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateDepthStencilView )( 
      ID3D12Device4 * This,
      _In_opt_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_DEPTH_STENCIL_VIEW_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CreateSampler )( 
      ID3D12Device4 * This,
      _In_  const D3D12_SAMPLER_DESC *pDesc,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

    void ( STDMETHODCALLTYPE *CopyDescriptors )( 
      ID3D12Device4 * This,
      _In_  UINT NumDestDescriptorRanges,
      _In_reads_(NumDestDescriptorRanges)  const D3D12_CPU_DESCRIPTOR_HANDLE *pDestDescriptorRangeStarts,
      _In_reads_opt_(NumDestDescriptorRanges)  const UINT *pDestDescriptorRangeSizes,
      _In_  UINT NumSrcDescriptorRanges,
      _In_reads_(NumSrcDescriptorRanges)  const D3D12_CPU_DESCRIPTOR_HANDLE *pSrcDescriptorRangeStarts,
      _In_reads_opt_(NumSrcDescriptorRanges)  const UINT *pSrcDescriptorRangeSizes,
      _In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType);

    void ( STDMETHODCALLTYPE *CopyDescriptorsSimple )( 
      ID3D12Device4 * This,
      _In_  UINT NumDescriptors,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptorRangeStart,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRangeStart,
      _In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType);

    D3D12_RESOURCE_ALLOCATION_INFO ( STDMETHODCALLTYPE *GetResourceAllocationInfo )( 
      ID3D12Device4 * This,
      _In_  UINT visibleMask,
      _In_  UINT numResourceDescs,
      _In_reads_(numResourceDescs)  const D3D12_RESOURCE_DESC *pResourceDescs);

    D3D12_HEAP_PROPERTIES ( STDMETHODCALLTYPE *GetCustomHeapProperties )( 
      ID3D12Device4 * This,
      _In_  UINT nodeMask,
      D3D12_HEAP_TYPE heapType);

    HRESULT ( STDMETHODCALLTYPE *CreateCommittedResource )( 
      ID3D12Device4 * This,
      _In_  const D3D12_HEAP_PROPERTIES *pHeapProperties,
      D3D12_HEAP_FLAGS HeapFlags,
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialResourceState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      REFIID riidResource,
      _COM_Outptr_opt_  void **ppvResource);

    HRESULT ( STDMETHODCALLTYPE *CreateHeap )( 
      ID3D12Device4 * This,
      _In_  const D3D12_HEAP_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvHeap);

    HRESULT ( STDMETHODCALLTYPE *CreatePlacedResource )( 
      ID3D12Device4 * This,
      _In_  ID3D12Heap *pHeap,
      UINT64 HeapOffset,
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvResource);

    HRESULT ( STDMETHODCALLTYPE *CreateReservedResource )( 
      ID3D12Device4 * This,
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvResource);

    HRESULT ( STDMETHODCALLTYPE *CreateSharedHandle )( 
      ID3D12Device4 * This,
      _In_  ID3D12DeviceChild *pObject,
      _In_opt_  const SECURITY_ATTRIBUTES *pAttributes,
      DWORD Access,
      _In_opt_  LPCWSTR Name,
      _Out_  HANDLE *pHandle);

    HRESULT ( STDMETHODCALLTYPE *OpenSharedHandle )( 
      ID3D12Device4 * This,
      _In_  HANDLE NTHandle,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvObj);

    HRESULT ( STDMETHODCALLTYPE *OpenSharedHandleByName )( 
      ID3D12Device4 * This,
      _In_  LPCWSTR Name,
      DWORD Access,
      /* [annotation][out] */ 
      _Out_  HANDLE *pNTHandle);

    HRESULT ( STDMETHODCALLTYPE *MakeResident )( 
      ID3D12Device4 * This,
      UINT NumObjects,
      _In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects);

    HRESULT ( STDMETHODCALLTYPE *Evict )( 
      ID3D12Device4 * This,
      UINT NumObjects,
      _In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects);

    HRESULT ( STDMETHODCALLTYPE *CreateFence )( 
      ID3D12Device4 * This,
      UINT64 InitialValue,
      D3D12_FENCE_FLAGS Flags,
      REFIID riid,
      _COM_Outptr_  void **ppFence);

    HRESULT ( STDMETHODCALLTYPE *GetDeviceRemovedReason )( 
      ID3D12Device4 * This);

    void ( STDMETHODCALLTYPE *GetCopyableFootprints )( 
      ID3D12Device4 * This,
      _In_  const D3D12_RESOURCE_DESC *pResourceDesc,
      _In_range_(0,D3D12_REQ_SUBRESOURCES)  UINT FirstSubresource,
      _In_range_(0,D3D12_REQ_SUBRESOURCES-FirstSubresource)  UINT NumSubresources,
      UINT64 BaseOffset,
      _Out_writes_opt_(NumSubresources)  D3D12_PLACED_SUBRESOURCE_FOOTPRINT *pLayouts,
      _Out_writes_opt_(NumSubresources)  UINT *pNumRows,
      _Out_writes_opt_(NumSubresources)  UINT64 *pRowSizeInBytes,
      _Out_opt_  UINT64 *pTotalBytes);

    HRESULT ( STDMETHODCALLTYPE *CreateQueryHeap )( 
      ID3D12Device4 * This,
      _In_  const D3D12_QUERY_HEAP_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvHeap);

    HRESULT ( STDMETHODCALLTYPE *SetStablePowerState )( 
      ID3D12Device4 * This,
      BOOL Enable);

    HRESULT ( STDMETHODCALLTYPE *CreateCommandSignature )( 
      ID3D12Device4 * This,
      _In_  const D3D12_COMMAND_SIGNATURE_DESC *pDesc,
      _In_opt_  ID3D12RootSignature *pRootSignature,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvCommandSignature);

    void ( STDMETHODCALLTYPE *GetResourceTiling )( 
      ID3D12Device4 * This,
      _In_  ID3D12Resource *pTiledResource,
      _Out_opt_  UINT *pNumTilesForEntireResource,
      _Out_opt_  D3D12_PACKED_MIP_INFO *pPackedMipDesc,
      _Out_opt_  D3D12_TILE_SHAPE *pStandardTileShapeForNonPackedMips,
      _Inout_opt_  UINT *pNumSubresourceTilings,
      _In_  UINT FirstSubresourceTilingToGet,
      _Out_writes_(*pNumSubresourceTilings)  D3D12_SUBRESOURCE_TILING *pSubresourceTilingsForNonPackedMips);

    LUID ( STDMETHODCALLTYPE *GetAdapterLuid )( 
      ID3D12Device4 * This);

    HRESULT ( STDMETHODCALLTYPE *CreatePipelineLibrary )( 
      ID3D12Device4 * This,
      _In_reads_(BlobLength)  const void *pLibraryBlob,
      SIZE_T BlobLength,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineLibrary);

    HRESULT ( STDMETHODCALLTYPE *SetEventOnMultipleFenceCompletion )( 
      ID3D12Device4 * This,
      _In_reads_(NumFences)  ID3D12Fence *const *ppFences,
      _In_reads_(NumFences)  const UINT64 *pFenceValues,
      UINT NumFences,
      D3D12_MULTIPLE_FENCE_WAIT_FLAGS Flags,
      HANDLE hEvent);

    HRESULT ( STDMETHODCALLTYPE *SetResidencyPriority )( 
      ID3D12Device4 * This,
      UINT NumObjects,
      _In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects,
      _In_reads_(NumObjects)  const D3D12_RESIDENCY_PRIORITY *pPriorities);

    HRESULT ( STDMETHODCALLTYPE *CreatePipelineState )( 
      ID3D12Device4 * This,
      const D3D12_PIPELINE_STATE_STREAM_DESC *pDesc,
      REFIID riid,
      _COM_Outptr_  void **ppPipelineState);

    HRESULT ( STDMETHODCALLTYPE *OpenExistingHeapFromAddress )( 
      ID3D12Device4 * This,
      _In_  const void *pAddress,
      REFIID riid,
      _COM_Outptr_  void **ppvHeap);

    HRESULT ( STDMETHODCALLTYPE *OpenExistingHeapFromFileMapping )( 
      ID3D12Device4 * This,
      _In_  HANDLE hFileMapping,
      REFIID riid,
      _COM_Outptr_  void **ppvHeap);

    HRESULT ( STDMETHODCALLTYPE *EnqueueMakeResident )( 
      ID3D12Device4 * This,
      D3D12_RESIDENCY_FLAGS Flags,
      UINT NumObjects,
      _In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects,
      _In_  ID3D12Fence *pFenceToSignal,
      UINT64 FenceValueToSignal);

    HRESULT ( STDMETHODCALLTYPE *CreateCommandList1 )( 
      ID3D12Device4 * This,
      _In_  UINT nodeMask,
      _In_  D3D12_COMMAND_LIST_TYPE type,
      _In_  D3D12_COMMAND_LIST_FLAGS flags,
      REFIID riid,
      _COM_Outptr_  void **ppCommandList);

    HRESULT ( STDMETHODCALLTYPE *CreateProtectedResourceSession )( 
      ID3D12Device4 * This,
      _In_  const D3D12_PROTECTED_RESOURCE_SESSION_DESC *pDesc,
      _In_  REFIID riid,
      _COM_Outptr_  void **ppSession);

    HRESULT ( STDMETHODCALLTYPE *CreateCommittedResource1 )( 
      ID3D12Device4 * This,
      _In_  const D3D12_HEAP_PROPERTIES *pHeapProperties,
      D3D12_HEAP_FLAGS HeapFlags,
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialResourceState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      _In_opt_  ID3D12ProtectedResourceSession *pProtectedSession,
      REFIID riidResource,
      _COM_Outptr_opt_  void **ppvResource);

    HRESULT ( STDMETHODCALLTYPE *CreateHeap1 )( 
      ID3D12Device4 * This,
      _In_  const D3D12_HEAP_DESC *pDesc,
      _In_opt_  ID3D12ProtectedResourceSession *pProtectedSession,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvHeap);

    HRESULT ( STDMETHODCALLTYPE *CreateReservedResource1 )( 
      ID3D12Device4 * This,
      _In_  const D3D12_RESOURCE_DESC *pDesc,
      D3D12_RESOURCE_STATES InitialState,
      _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
      _In_opt_  ID3D12ProtectedResourceSession *pProtectedSession,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvResource);

    D3D12_RESOURCE_ALLOCATION_INFO ( STDMETHODCALLTYPE *GetResourceAllocationInfo1 )( 
      ID3D12Device4 * This,
      UINT visibleMask,
      UINT numResourceDescs,
      _In_reads_(numResourceDescs)  const D3D12_RESOURCE_DESC *pResourceDescs,
      _Out_writes_opt_(numResourceDescs)  D3D12_RESOURCE_ALLOCATION_INFO1 *pResourceAllocationInfo1);

    END_INTERFACE
  } ID3D12Device4Vtbl;

  interface ID3D12Device4
  {
    CONST_VTBL struct ID3D12Device4Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12Device4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12Device4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12Device4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12Device4_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12Device4_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12Device4_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12Device4_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12Device4_GetNodeCount(This)	\
    ( (This)->lpVtbl -> GetNodeCount(This) ) 

#define ID3D12Device4_CreateCommandQueue(This,pDesc,riid,ppCommandQueue)	\
    ( (This)->lpVtbl -> CreateCommandQueue(This,pDesc,riid,ppCommandQueue) ) 

#define ID3D12Device4_CreateCommandAllocator(This,type,riid,ppCommandAllocator)	\
    ( (This)->lpVtbl -> CreateCommandAllocator(This,type,riid,ppCommandAllocator) ) 

#define ID3D12Device4_CreateGraphicsPipelineState(This,pDesc,riid,ppPipelineState)	\
    ( (This)->lpVtbl -> CreateGraphicsPipelineState(This,pDesc,riid,ppPipelineState) ) 

#define ID3D12Device4_CreateComputePipelineState(This,pDesc,riid,ppPipelineState)	\
    ( (This)->lpVtbl -> CreateComputePipelineState(This,pDesc,riid,ppPipelineState) ) 

#define ID3D12Device4_CreateCommandList(This,nodeMask,type,pCommandAllocator,pInitialState,riid,ppCommandList)	\
    ( (This)->lpVtbl -> CreateCommandList(This,nodeMask,type,pCommandAllocator,pInitialState,riid,ppCommandList) ) 

#define ID3D12Device4_CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize)	\
    ( (This)->lpVtbl -> CheckFeatureSupport(This,Feature,pFeatureSupportData,FeatureSupportDataSize) ) 

#define ID3D12Device4_CreateDescriptorHeap(This,pDescriptorHeapDesc,riid,ppvHeap)	\
    ( (This)->lpVtbl -> CreateDescriptorHeap(This,pDescriptorHeapDesc,riid,ppvHeap) ) 

#define ID3D12Device4_GetDescriptorHandleIncrementSize(This,DescriptorHeapType)	\
    ( (This)->lpVtbl -> GetDescriptorHandleIncrementSize(This,DescriptorHeapType) ) 

#define ID3D12Device4_CreateRootSignature(This,nodeMask,pBlobWithRootSignature,blobLengthInBytes,riid,ppvRootSignature)	\
    ( (This)->lpVtbl -> CreateRootSignature(This,nodeMask,pBlobWithRootSignature,blobLengthInBytes,riid,ppvRootSignature) ) 

#define ID3D12Device4_CreateConstantBufferView(This,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateConstantBufferView(This,pDesc,DestDescriptor) ) 

#define ID3D12Device4_CreateShaderResourceView(This,pResource,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateShaderResourceView(This,pResource,pDesc,DestDescriptor) ) 

#define ID3D12Device4_CreateUnorderedAccessView(This,pResource,pCounterResource,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateUnorderedAccessView(This,pResource,pCounterResource,pDesc,DestDescriptor) ) 

#define ID3D12Device4_CreateRenderTargetView(This,pResource,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateRenderTargetView(This,pResource,pDesc,DestDescriptor) ) 

#define ID3D12Device4_CreateDepthStencilView(This,pResource,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateDepthStencilView(This,pResource,pDesc,DestDescriptor) ) 

#define ID3D12Device4_CreateSampler(This,pDesc,DestDescriptor)	\
    ( (This)->lpVtbl -> CreateSampler(This,pDesc,DestDescriptor) ) 

#define ID3D12Device4_CopyDescriptors(This,NumDestDescriptorRanges,pDestDescriptorRangeStarts,pDestDescriptorRangeSizes,NumSrcDescriptorRanges,pSrcDescriptorRangeStarts,pSrcDescriptorRangeSizes,DescriptorHeapsType)	\
    ( (This)->lpVtbl -> CopyDescriptors(This,NumDestDescriptorRanges,pDestDescriptorRangeStarts,pDestDescriptorRangeSizes,NumSrcDescriptorRanges,pSrcDescriptorRangeStarts,pSrcDescriptorRangeSizes,DescriptorHeapsType) ) 

#define ID3D12Device4_CopyDescriptorsSimple(This,NumDescriptors,DestDescriptorRangeStart,SrcDescriptorRangeStart,DescriptorHeapsType)	\
    ( (This)->lpVtbl -> CopyDescriptorsSimple(This,NumDescriptors,DestDescriptorRangeStart,SrcDescriptorRangeStart,DescriptorHeapsType) ) 

#define ID3D12Device4_GetResourceAllocationInfo(This,visibleMask,numResourceDescs,pResourceDescs)	\
    ( (This)->lpVtbl -> GetResourceAllocationInfo(This,visibleMask,numResourceDescs,pResourceDescs) ) 

#define ID3D12Device4_GetCustomHeapProperties(This,nodeMask,heapType)	\
    ( (This)->lpVtbl -> GetCustomHeapProperties(This,nodeMask,heapType) ) 

#define ID3D12Device4_CreateCommittedResource(This,pHeapProperties,HeapFlags,pDesc,InitialResourceState,pOptimizedClearValue,riidResource,ppvResource)	\
    ( (This)->lpVtbl -> CreateCommittedResource(This,pHeapProperties,HeapFlags,pDesc,InitialResourceState,pOptimizedClearValue,riidResource,ppvResource) ) 

#define ID3D12Device4_CreateHeap(This,pDesc,riid,ppvHeap)	\
    ( (This)->lpVtbl -> CreateHeap(This,pDesc,riid,ppvHeap) ) 

#define ID3D12Device4_CreatePlacedResource(This,pHeap,HeapOffset,pDesc,InitialState,pOptimizedClearValue,riid,ppvResource)	\
    ( (This)->lpVtbl -> CreatePlacedResource(This,pHeap,HeapOffset,pDesc,InitialState,pOptimizedClearValue,riid,ppvResource) ) 

#define ID3D12Device4_CreateReservedResource(This,pDesc,InitialState,pOptimizedClearValue,riid,ppvResource)	\
    ( (This)->lpVtbl -> CreateReservedResource(This,pDesc,InitialState,pOptimizedClearValue,riid,ppvResource) ) 

#define ID3D12Device4_CreateSharedHandle(This,pObject,pAttributes,Access,Name,pHandle)	\
    ( (This)->lpVtbl -> CreateSharedHandle(This,pObject,pAttributes,Access,Name,pHandle) ) 

#define ID3D12Device4_OpenSharedHandle(This,NTHandle,riid,ppvObj)	\
    ( (This)->lpVtbl -> OpenSharedHandle(This,NTHandle,riid,ppvObj) ) 

#define ID3D12Device4_OpenSharedHandleByName(This,Name,Access,pNTHandle)	\
    ( (This)->lpVtbl -> OpenSharedHandleByName(This,Name,Access,pNTHandle) ) 

#define ID3D12Device4_MakeResident(This,NumObjects,ppObjects)	\
    ( (This)->lpVtbl -> MakeResident(This,NumObjects,ppObjects) ) 

#define ID3D12Device4_Evict(This,NumObjects,ppObjects)	\
    ( (This)->lpVtbl -> Evict(This,NumObjects,ppObjects) ) 

#define ID3D12Device4_CreateFence(This,InitialValue,Flags,riid,ppFence)	\
    ( (This)->lpVtbl -> CreateFence(This,InitialValue,Flags,riid,ppFence) ) 

#define ID3D12Device4_GetDeviceRemovedReason(This)	\
    ( (This)->lpVtbl -> GetDeviceRemovedReason(This) ) 

#define ID3D12Device4_GetCopyableFootprints(This,pResourceDesc,FirstSubresource,NumSubresources,BaseOffset,pLayouts,pNumRows,pRowSizeInBytes,pTotalBytes)	\
    ( (This)->lpVtbl -> GetCopyableFootprints(This,pResourceDesc,FirstSubresource,NumSubresources,BaseOffset,pLayouts,pNumRows,pRowSizeInBytes,pTotalBytes) ) 

#define ID3D12Device4_CreateQueryHeap(This,pDesc,riid,ppvHeap)	\
    ( (This)->lpVtbl -> CreateQueryHeap(This,pDesc,riid,ppvHeap) ) 

#define ID3D12Device4_SetStablePowerState(This,Enable)	\
    ( (This)->lpVtbl -> SetStablePowerState(This,Enable) ) 

#define ID3D12Device4_CreateCommandSignature(This,pDesc,pRootSignature,riid,ppvCommandSignature)	\
    ( (This)->lpVtbl -> CreateCommandSignature(This,pDesc,pRootSignature,riid,ppvCommandSignature) ) 

#define ID3D12Device4_GetResourceTiling(This,pTiledResource,pNumTilesForEntireResource,pPackedMipDesc,pStandardTileShapeForNonPackedMips,pNumSubresourceTilings,FirstSubresourceTilingToGet,pSubresourceTilingsForNonPackedMips)	\
    ( (This)->lpVtbl -> GetResourceTiling(This,pTiledResource,pNumTilesForEntireResource,pPackedMipDesc,pStandardTileShapeForNonPackedMips,pNumSubresourceTilings,FirstSubresourceTilingToGet,pSubresourceTilingsForNonPackedMips) ) 

#define ID3D12Device4_GetAdapterLuid(This)	\
    ( (This)->lpVtbl -> GetAdapterLuid(This) ) 


#define ID3D12Device4_CreatePipelineLibrary(This,pLibraryBlob,BlobLength,riid,ppPipelineLibrary)	\
    ( (This)->lpVtbl -> CreatePipelineLibrary(This,pLibraryBlob,BlobLength,riid,ppPipelineLibrary) ) 

#define ID3D12Device4_SetEventOnMultipleFenceCompletion(This,ppFences,pFenceValues,NumFences,Flags,hEvent)	\
    ( (This)->lpVtbl -> SetEventOnMultipleFenceCompletion(This,ppFences,pFenceValues,NumFences,Flags,hEvent) ) 

#define ID3D12Device4_SetResidencyPriority(This,NumObjects,ppObjects,pPriorities)	\
    ( (This)->lpVtbl -> SetResidencyPriority(This,NumObjects,ppObjects,pPriorities) ) 


#define ID3D12Device4_CreatePipelineState(This,pDesc,riid,ppPipelineState)	\
    ( (This)->lpVtbl -> CreatePipelineState(This,pDesc,riid,ppPipelineState) ) 


#define ID3D12Device4_OpenExistingHeapFromAddress(This,pAddress,riid,ppvHeap)	\
    ( (This)->lpVtbl -> OpenExistingHeapFromAddress(This,pAddress,riid,ppvHeap) ) 

#define ID3D12Device4_OpenExistingHeapFromFileMapping(This,hFileMapping,riid,ppvHeap)	\
    ( (This)->lpVtbl -> OpenExistingHeapFromFileMapping(This,hFileMapping,riid,ppvHeap) ) 

#define ID3D12Device4_EnqueueMakeResident(This,Flags,NumObjects,ppObjects,pFenceToSignal,FenceValueToSignal)	\
    ( (This)->lpVtbl -> EnqueueMakeResident(This,Flags,NumObjects,ppObjects,pFenceToSignal,FenceValueToSignal) ) 


#define ID3D12Device4_CreateCommandList1(This,nodeMask,type,flags,riid,ppCommandList)	\
    ( (This)->lpVtbl -> CreateCommandList1(This,nodeMask,type,flags,riid,ppCommandList) ) 

#define ID3D12Device4_CreateProtectedResourceSession(This,pDesc,riid,ppSession)	\
    ( (This)->lpVtbl -> CreateProtectedResourceSession(This,pDesc,riid,ppSession) ) 

#define ID3D12Device4_CreateCommittedResource1(This,pHeapProperties,HeapFlags,pDesc,InitialResourceState,pOptimizedClearValue,pProtectedSession,riidResource,ppvResource)	\
    ( (This)->lpVtbl -> CreateCommittedResource1(This,pHeapProperties,HeapFlags,pDesc,InitialResourceState,pOptimizedClearValue,pProtectedSession,riidResource,ppvResource) ) 

#define ID3D12Device4_CreateHeap1(This,pDesc,pProtectedSession,riid,ppvHeap)	\
    ( (This)->lpVtbl -> CreateHeap1(This,pDesc,pProtectedSession,riid,ppvHeap) ) 

#define ID3D12Device4_CreateReservedResource1(This,pDesc,InitialState,pOptimizedClearValue,pProtectedSession,riid,ppvResource)	\
    ( (This)->lpVtbl -> CreateReservedResource1(This,pDesc,InitialState,pOptimizedClearValue,pProtectedSession,riid,ppvResource) ) 

#define ID3D12Device4_GetResourceAllocationInfo1(This,visibleMask,numResourceDescs,pResourceDescs,pResourceAllocationInfo1)	\
    ( (This)->lpVtbl -> GetResourceAllocationInfo1(This,visibleMask,numResourceDescs,pResourceDescs,pResourceAllocationInfo1) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12Device4_INTERFACE_DEFINED__ */


#ifndef __ID3D12Resource1_INTERFACE_DEFINED__
#define __ID3D12Resource1_INTERFACE_DEFINED__

  /* interface ID3D12Resource1 */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12Resource1;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("9D5E227A-4430-4161-88B3-3ECA6BB16E19")
    ID3D12Resource1 : public ID3D12Resource
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetProtectedResourceSession( 
      REFIID riid,
      _COM_Outptr_opt_  void **ppProtectedSession) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12Resource1Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12Resource1 * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12Resource1 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12Resource1 * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12Resource1 * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12Resource1 * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12Resource1 * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12Resource1 * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12Resource1 * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    HRESULT ( STDMETHODCALLTYPE *Map )( 
      ID3D12Resource1 * This,
      UINT Subresource,
      _In_opt_  const D3D12_RANGE *pReadRange,
      _Outptr_opt_result_bytebuffer_(_Inexpressible_("Dependent on resource"))  void **ppData);

    void ( STDMETHODCALLTYPE *Unmap )( 
      ID3D12Resource1 * This,
      UINT Subresource,
      _In_opt_  const D3D12_RANGE *pWrittenRange);

    D3D12_RESOURCE_DESC ( STDMETHODCALLTYPE *GetDesc )( 
      ID3D12Resource1 * This);

    D3D12_GPU_VIRTUAL_ADDRESS ( STDMETHODCALLTYPE *GetGPUVirtualAddress )( 
      ID3D12Resource1 * This);

    HRESULT ( STDMETHODCALLTYPE *WriteToSubresource )( 
      ID3D12Resource1 * This,
      UINT DstSubresource,
      _In_opt_  const D3D12_BOX *pDstBox,
      _In_  const void *pSrcData,
      UINT SrcRowPitch,
      UINT SrcDepthPitch);

    HRESULT ( STDMETHODCALLTYPE *ReadFromSubresource )( 
      ID3D12Resource1 * This,
      _Out_  void *pDstData,
      UINT DstRowPitch,
      UINT DstDepthPitch,
      UINT SrcSubresource,
      _In_opt_  const D3D12_BOX *pSrcBox);

    HRESULT ( STDMETHODCALLTYPE *GetHeapProperties )( 
      ID3D12Resource1 * This,
      _Out_opt_  D3D12_HEAP_PROPERTIES *pHeapProperties,
      _Out_opt_  D3D12_HEAP_FLAGS *pHeapFlags);

    HRESULT ( STDMETHODCALLTYPE *GetProtectedResourceSession )( 
      ID3D12Resource1 * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppProtectedSession);

    END_INTERFACE
  } ID3D12Resource1Vtbl;

  interface ID3D12Resource1
  {
    CONST_VTBL struct ID3D12Resource1Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12Resource1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12Resource1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12Resource1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12Resource1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12Resource1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12Resource1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12Resource1_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12Resource1_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 



#define ID3D12Resource1_Map(This,Subresource,pReadRange,ppData)	\
    ( (This)->lpVtbl -> Map(This,Subresource,pReadRange,ppData) ) 

#define ID3D12Resource1_Unmap(This,Subresource,pWrittenRange)	\
    ( (This)->lpVtbl -> Unmap(This,Subresource,pWrittenRange) ) 

#define ID3D12Resource1_GetDesc(This)	\
    ( (This)->lpVtbl -> GetDesc(This) ) 

#define ID3D12Resource1_GetGPUVirtualAddress(This)	\
    ( (This)->lpVtbl -> GetGPUVirtualAddress(This) ) 

#define ID3D12Resource1_WriteToSubresource(This,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch)	\
    ( (This)->lpVtbl -> WriteToSubresource(This,DstSubresource,pDstBox,pSrcData,SrcRowPitch,SrcDepthPitch) ) 

#define ID3D12Resource1_ReadFromSubresource(This,pDstData,DstRowPitch,DstDepthPitch,SrcSubresource,pSrcBox)	\
    ( (This)->lpVtbl -> ReadFromSubresource(This,pDstData,DstRowPitch,DstDepthPitch,SrcSubresource,pSrcBox) ) 

#define ID3D12Resource1_GetHeapProperties(This,pHeapProperties,pHeapFlags)	\
    ( (This)->lpVtbl -> GetHeapProperties(This,pHeapProperties,pHeapFlags) ) 


#define ID3D12Resource1_GetProtectedResourceSession(This,riid,ppProtectedSession)	\
    ( (This)->lpVtbl -> GetProtectedResourceSession(This,riid,ppProtectedSession) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12Resource1_INTERFACE_DEFINED__ */


#ifndef __ID3D12Heap1_INTERFACE_DEFINED__
#define __ID3D12Heap1_INTERFACE_DEFINED__

  /* interface ID3D12Heap1 */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12Heap1;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("572F7389-2168-49E3-9693-D6DF5871BF6D")
    ID3D12Heap1 : public ID3D12Heap
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetProtectedResourceSession( 
      REFIID riid,
      _COM_Outptr_opt_  void **ppProtectedSession) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12Heap1Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12Heap1 * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12Heap1 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12Heap1 * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12Heap1 * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12Heap1 * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12Heap1 * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12Heap1 * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12Heap1 * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    D3D12_HEAP_DESC ( STDMETHODCALLTYPE *GetDesc )( 
      ID3D12Heap1 * This);

    HRESULT ( STDMETHODCALLTYPE *GetProtectedResourceSession )( 
      ID3D12Heap1 * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppProtectedSession);

    END_INTERFACE
  } ID3D12Heap1Vtbl;

  interface ID3D12Heap1
  {
    CONST_VTBL struct ID3D12Heap1Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12Heap1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12Heap1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12Heap1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12Heap1_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12Heap1_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12Heap1_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12Heap1_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12Heap1_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 



#define ID3D12Heap1_GetDesc(This)	\
    ( (This)->lpVtbl -> GetDesc(This) ) 


#define ID3D12Heap1_GetProtectedResourceSession(This,riid,ppProtectedSession)	\
    ( (This)->lpVtbl -> GetProtectedResourceSession(This,riid,ppProtectedSession) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12Heap1_INTERFACE_DEFINED__ */


#ifndef __ID3D12GraphicsCommandList3_INTERFACE_DEFINED__
#define __ID3D12GraphicsCommandList3_INTERFACE_DEFINED__

  /* interface ID3D12GraphicsCommandList3 */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12GraphicsCommandList3;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("6FDA83A7-B84C-4E38-9AC8-C7BD22016B3D")
    ID3D12GraphicsCommandList3 : public ID3D12GraphicsCommandList2
  {
  public:
    virtual void STDMETHODCALLTYPE SetProtectedResourceSession( 
      _In_opt_  ID3D12ProtectedResourceSession *pProtectedResourceSession) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12GraphicsCommandList3Vtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12GraphicsCommandList3 * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12GraphicsCommandList3 * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12GraphicsCommandList3 * This);

    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  REFGUID guid,
      _Inout_  UINT *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  REFGUID guid,
      _In_  UINT DataSize,
      _In_reads_bytes_opt_( DataSize )  const void *pData);

    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  REFGUID guid,
      _In_opt_  const IUnknown *pData);

    HRESULT ( STDMETHODCALLTYPE *SetName )( 
      ID3D12GraphicsCommandList3 * This,
      _In_z_  LPCWSTR Name);

    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
      ID3D12GraphicsCommandList3 * This,
      REFIID riid,
      _COM_Outptr_opt_  void **ppvDevice);

    D3D12_COMMAND_LIST_TYPE ( STDMETHODCALLTYPE *GetType )( 
      ID3D12GraphicsCommandList3 * This);

    HRESULT ( STDMETHODCALLTYPE *Close )( 
      ID3D12GraphicsCommandList3 * This);

    HRESULT ( STDMETHODCALLTYPE *Reset )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  ID3D12CommandAllocator *pAllocator,
      _In_opt_  ID3D12PipelineState *pInitialState);

    void ( STDMETHODCALLTYPE *ClearState )( 
      ID3D12GraphicsCommandList3 * This,
      _In_opt_  ID3D12PipelineState *pPipelineState);

    void ( STDMETHODCALLTYPE *DrawInstanced )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT VertexCountPerInstance,
      _In_  UINT InstanceCount,
      _In_  UINT StartVertexLocation,
      _In_  UINT StartInstanceLocation);

    void ( STDMETHODCALLTYPE *DrawIndexedInstanced )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT IndexCountPerInstance,
      _In_  UINT InstanceCount,
      _In_  UINT StartIndexLocation,
      _In_  INT BaseVertexLocation,
      _In_  UINT StartInstanceLocation);

    void ( STDMETHODCALLTYPE *Dispatch )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT ThreadGroupCountX,
      _In_  UINT ThreadGroupCountY,
      _In_  UINT ThreadGroupCountZ);

    void ( STDMETHODCALLTYPE *CopyBufferRegion )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  ID3D12Resource *pDstBuffer,
      UINT64 DstOffset,
      _In_  ID3D12Resource *pSrcBuffer,
      UINT64 SrcOffset,
      UINT64 NumBytes);

    void ( STDMETHODCALLTYPE *CopyTextureRegion )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  const D3D12_TEXTURE_COPY_LOCATION *pDst,
      UINT DstX,
      UINT DstY,
      UINT DstZ,
      _In_  const D3D12_TEXTURE_COPY_LOCATION *pSrc,
      _In_opt_  const D3D12_BOX *pSrcBox);

    void ( STDMETHODCALLTYPE *CopyResource )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  ID3D12Resource *pDstResource,
      _In_  ID3D12Resource *pSrcResource);

    void ( STDMETHODCALLTYPE *CopyTiles )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  ID3D12Resource *pTiledResource,
      _In_  const D3D12_TILED_RESOURCE_COORDINATE *pTileRegionStartCoordinate,
      _In_  const D3D12_TILE_REGION_SIZE *pTileRegionSize,
      _In_  ID3D12Resource *pBuffer,
      UINT64 BufferStartOffsetInBytes,
      D3D12_TILE_COPY_FLAGS Flags);

    void ( STDMETHODCALLTYPE *ResolveSubresource )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  ID3D12Resource *pDstResource,
      _In_  UINT DstSubresource,
      _In_  ID3D12Resource *pSrcResource,
      _In_  UINT SrcSubresource,
      _In_  DXGI_FORMAT Format);

    void ( STDMETHODCALLTYPE *IASetPrimitiveTopology )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology);

    void ( STDMETHODCALLTYPE *RSSetViewports )( 
      ID3D12GraphicsCommandList3 * This,
      _In_range_(0, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumViewports,
      _In_reads_( NumViewports)  const D3D12_VIEWPORT *pViewports);

    void ( STDMETHODCALLTYPE *RSSetScissorRects )( 
      ID3D12GraphicsCommandList3 * This,
      _In_range_(0, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumRects,
      _In_reads_( NumRects)  const D3D12_RECT *pRects);

    void ( STDMETHODCALLTYPE *OMSetBlendFactor )( 
      ID3D12GraphicsCommandList3 * This,
      _In_reads_opt_(4)  const FLOAT BlendFactor[ 4 ]);

    void ( STDMETHODCALLTYPE *OMSetStencilRef )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT StencilRef);

    void ( STDMETHODCALLTYPE *SetPipelineState )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  ID3D12PipelineState *pPipelineState);

    void ( STDMETHODCALLTYPE *ResourceBarrier )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT NumBarriers,
      _In_reads_(NumBarriers)  const D3D12_RESOURCE_BARRIER *pBarriers);

    void ( STDMETHODCALLTYPE *ExecuteBundle )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  ID3D12GraphicsCommandList *pCommandList);

    void ( STDMETHODCALLTYPE *SetDescriptorHeaps )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT NumDescriptorHeaps,
      _In_reads_(NumDescriptorHeaps)  ID3D12DescriptorHeap *const *ppDescriptorHeaps);

    void ( STDMETHODCALLTYPE *SetComputeRootSignature )( 
      ID3D12GraphicsCommandList3 * This,
      _In_opt_  ID3D12RootSignature *pRootSignature);

    void ( STDMETHODCALLTYPE *SetGraphicsRootSignature )( 
      ID3D12GraphicsCommandList3 * This,
      _In_opt_  ID3D12RootSignature *pRootSignature);

    void ( STDMETHODCALLTYPE *SetComputeRootDescriptorTable )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);

    void ( STDMETHODCALLTYPE *SetGraphicsRootDescriptorTable )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);

    void ( STDMETHODCALLTYPE *SetComputeRoot32BitConstant )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT RootParameterIndex,
      _In_  UINT SrcData,
      _In_  UINT DestOffsetIn32BitValues);

    void ( STDMETHODCALLTYPE *SetGraphicsRoot32BitConstant )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT RootParameterIndex,
      _In_  UINT SrcData,
      _In_  UINT DestOffsetIn32BitValues);

    void ( STDMETHODCALLTYPE *SetComputeRoot32BitConstants )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT RootParameterIndex,
      _In_  UINT Num32BitValuesToSet,
      _In_reads_(Num32BitValuesToSet*sizeof(UINT))  const void *pSrcData,
      _In_  UINT DestOffsetIn32BitValues);

    void ( STDMETHODCALLTYPE *SetGraphicsRoot32BitConstants )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT RootParameterIndex,
      _In_  UINT Num32BitValuesToSet,
      _In_reads_(Num32BitValuesToSet*sizeof(UINT))  const void *pSrcData,
      _In_  UINT DestOffsetIn32BitValues);

    void ( STDMETHODCALLTYPE *SetComputeRootConstantBufferView )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *SetGraphicsRootConstantBufferView )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *SetComputeRootShaderResourceView )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *SetGraphicsRootShaderResourceView )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *SetComputeRootUnorderedAccessView )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *SetGraphicsRootUnorderedAccessView )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT RootParameterIndex,
      _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

    void ( STDMETHODCALLTYPE *IASetIndexBuffer )( 
      ID3D12GraphicsCommandList3 * This,
      _In_opt_  const D3D12_INDEX_BUFFER_VIEW *pView);

    void ( STDMETHODCALLTYPE *IASetVertexBuffers )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT StartSlot,
      _In_  UINT NumViews,
      _In_reads_opt_(NumViews)  const D3D12_VERTEX_BUFFER_VIEW *pViews);

    void ( STDMETHODCALLTYPE *SOSetTargets )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT StartSlot,
      _In_  UINT NumViews,
      _In_reads_opt_(NumViews)  const D3D12_STREAM_OUTPUT_BUFFER_VIEW *pViews);

    void ( STDMETHODCALLTYPE *OMSetRenderTargets )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT NumRenderTargetDescriptors,
      _In_opt_  const D3D12_CPU_DESCRIPTOR_HANDLE *pRenderTargetDescriptors,
      _In_  BOOL RTsSingleHandleToDescriptorRange,
      _In_opt_  const D3D12_CPU_DESCRIPTOR_HANDLE *pDepthStencilDescriptor);

    void ( STDMETHODCALLTYPE *ClearDepthStencilView )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView,
      _In_  D3D12_CLEAR_FLAGS ClearFlags,
      _In_  FLOAT Depth,
      _In_  UINT8 Stencil,
      _In_  UINT NumRects,
      _In_reads_(NumRects)  const D3D12_RECT *pRects);

    void ( STDMETHODCALLTYPE *ClearRenderTargetView )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView,
      _In_  const FLOAT ColorRGBA[ 4 ],
      _In_  UINT NumRects,
      _In_reads_(NumRects)  const D3D12_RECT *pRects);

    void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewUint )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle,
      _In_  ID3D12Resource *pResource,
      _In_  const UINT Values[ 4 ],
      _In_  UINT NumRects,
      _In_reads_(NumRects)  const D3D12_RECT *pRects);

    void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewFloat )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap,
      _In_  D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle,
      _In_  ID3D12Resource *pResource,
      _In_  const FLOAT Values[ 4 ],
      _In_  UINT NumRects,
      _In_reads_(NumRects)  const D3D12_RECT *pRects);

    void ( STDMETHODCALLTYPE *DiscardResource )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  ID3D12Resource *pResource,
      _In_opt_  const D3D12_DISCARD_REGION *pRegion);

    void ( STDMETHODCALLTYPE *BeginQuery )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  ID3D12QueryHeap *pQueryHeap,
      _In_  D3D12_QUERY_TYPE Type,
      _In_  UINT Index);

    void ( STDMETHODCALLTYPE *EndQuery )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  ID3D12QueryHeap *pQueryHeap,
      _In_  D3D12_QUERY_TYPE Type,
      _In_  UINT Index);

    void ( STDMETHODCALLTYPE *ResolveQueryData )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  ID3D12QueryHeap *pQueryHeap,
      _In_  D3D12_QUERY_TYPE Type,
      _In_  UINT StartIndex,
      _In_  UINT NumQueries,
      _In_  ID3D12Resource *pDestinationBuffer,
      _In_  UINT64 AlignedDestinationBufferOffset);

    void ( STDMETHODCALLTYPE *SetPredication )( 
      ID3D12GraphicsCommandList3 * This,
      _In_opt_  ID3D12Resource *pBuffer,
      _In_  UINT64 AlignedBufferOffset,
      _In_  D3D12_PREDICATION_OP Operation);

    void ( STDMETHODCALLTYPE *SetMarker )( 
      ID3D12GraphicsCommandList3 * This,
      UINT Metadata,
      _In_reads_bytes_opt_(Size)  const void *pData,
      UINT Size);

    void ( STDMETHODCALLTYPE *BeginEvent )( 
      ID3D12GraphicsCommandList3 * This,
      UINT Metadata,
      _In_reads_bytes_opt_(Size)  const void *pData,
      UINT Size);

    void ( STDMETHODCALLTYPE *EndEvent )( 
      ID3D12GraphicsCommandList3 * This);

    void ( STDMETHODCALLTYPE *ExecuteIndirect )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  ID3D12CommandSignature *pCommandSignature,
      _In_  UINT MaxCommandCount,
      _In_  ID3D12Resource *pArgumentBuffer,
      _In_  UINT64 ArgumentBufferOffset,
      _In_opt_  ID3D12Resource *pCountBuffer,
      _In_  UINT64 CountBufferOffset);

    void ( STDMETHODCALLTYPE *AtomicCopyBufferUINT )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  ID3D12Resource *pDstBuffer,
      UINT64 DstOffset,
      _In_  ID3D12Resource *pSrcBuffer,
      UINT64 SrcOffset,
      UINT Dependencies,
      _In_reads_(Dependencies)  ID3D12Resource *const *ppDependentResources,
      _In_reads_(Dependencies)  const D3D12_SUBRESOURCE_RANGE_UINT64 *pDependentSubresourceRanges);

    void ( STDMETHODCALLTYPE *AtomicCopyBufferUINT64 )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  ID3D12Resource *pDstBuffer,
      UINT64 DstOffset,
      _In_  ID3D12Resource *pSrcBuffer,
      UINT64 SrcOffset,
      UINT Dependencies,
      _In_reads_(Dependencies)  ID3D12Resource *const *ppDependentResources,
      _In_reads_(Dependencies)  const D3D12_SUBRESOURCE_RANGE_UINT64 *pDependentSubresourceRanges);

    void ( STDMETHODCALLTYPE *OMSetDepthBounds )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  FLOAT Min,
      _In_  FLOAT Max);

    void ( STDMETHODCALLTYPE *SetSamplePositions )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT NumSamplesPerPixel,
      _In_  UINT NumPixels,
      _In_reads_(NumSamplesPerPixel*NumPixels)  D3D12_SAMPLE_POSITION *pSamplePositions);

    void ( STDMETHODCALLTYPE *ResolveSubresourceRegion )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  ID3D12Resource *pDstResource,
      _In_  UINT DstSubresource,
      _In_  UINT DstX,
      _In_  UINT DstY,
      _In_  ID3D12Resource *pSrcResource,
      _In_  UINT SrcSubresource,
      _In_opt_  D3D12_RECT *pSrcRect,
      _In_  DXGI_FORMAT Format,
      _In_  D3D12_RESOLVE_MODE ResolveMode);

    void ( STDMETHODCALLTYPE *SetViewInstanceMask )( 
      ID3D12GraphicsCommandList3 * This,
      _In_  UINT Mask);

    void ( STDMETHODCALLTYPE *WriteBufferImmediate )( 
      ID3D12GraphicsCommandList3 * This,
      UINT Count,
      _In_reads_(Count)  const D3D12_WRITEBUFFERIMMEDIATE_PARAMETER *pParams,
      _In_reads_opt_(Count)  const D3D12_WRITEBUFFERIMMEDIATE_MODE *pModes);

    void ( STDMETHODCALLTYPE *SetProtectedResourceSession )( 
      ID3D12GraphicsCommandList3 * This,
      _In_opt_  ID3D12ProtectedResourceSession *pProtectedResourceSession);

    END_INTERFACE
  } ID3D12GraphicsCommandList3Vtbl;

  interface ID3D12GraphicsCommandList3
  {
    CONST_VTBL struct ID3D12GraphicsCommandList3Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12GraphicsCommandList3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12GraphicsCommandList3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12GraphicsCommandList3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12GraphicsCommandList3_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12GraphicsCommandList3_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12GraphicsCommandList3_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12GraphicsCommandList3_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12GraphicsCommandList3_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 


#define ID3D12GraphicsCommandList3_GetType(This)	\
    ( (This)->lpVtbl -> GetType(This) ) 


#define ID3D12GraphicsCommandList3_Close(This)	\
    ( (This)->lpVtbl -> Close(This) ) 

#define ID3D12GraphicsCommandList3_Reset(This,pAllocator,pInitialState)	\
    ( (This)->lpVtbl -> Reset(This,pAllocator,pInitialState) ) 

#define ID3D12GraphicsCommandList3_ClearState(This,pPipelineState)	\
    ( (This)->lpVtbl -> ClearState(This,pPipelineState) ) 

#define ID3D12GraphicsCommandList3_DrawInstanced(This,VertexCountPerInstance,InstanceCount,StartVertexLocation,StartInstanceLocation)	\
    ( (This)->lpVtbl -> DrawInstanced(This,VertexCountPerInstance,InstanceCount,StartVertexLocation,StartInstanceLocation) ) 

#define ID3D12GraphicsCommandList3_DrawIndexedInstanced(This,IndexCountPerInstance,InstanceCount,StartIndexLocation,BaseVertexLocation,StartInstanceLocation)	\
    ( (This)->lpVtbl -> DrawIndexedInstanced(This,IndexCountPerInstance,InstanceCount,StartIndexLocation,BaseVertexLocation,StartInstanceLocation) ) 

#define ID3D12GraphicsCommandList3_Dispatch(This,ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ)	\
    ( (This)->lpVtbl -> Dispatch(This,ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ) ) 

#define ID3D12GraphicsCommandList3_CopyBufferRegion(This,pDstBuffer,DstOffset,pSrcBuffer,SrcOffset,NumBytes)	\
    ( (This)->lpVtbl -> CopyBufferRegion(This,pDstBuffer,DstOffset,pSrcBuffer,SrcOffset,NumBytes) ) 

#define ID3D12GraphicsCommandList3_CopyTextureRegion(This,pDst,DstX,DstY,DstZ,pSrc,pSrcBox)	\
    ( (This)->lpVtbl -> CopyTextureRegion(This,pDst,DstX,DstY,DstZ,pSrc,pSrcBox) ) 

#define ID3D12GraphicsCommandList3_CopyResource(This,pDstResource,pSrcResource)	\
    ( (This)->lpVtbl -> CopyResource(This,pDstResource,pSrcResource) ) 

#define ID3D12GraphicsCommandList3_CopyTiles(This,pTiledResource,pTileRegionStartCoordinate,pTileRegionSize,pBuffer,BufferStartOffsetInBytes,Flags)	\
    ( (This)->lpVtbl -> CopyTiles(This,pTiledResource,pTileRegionStartCoordinate,pTileRegionSize,pBuffer,BufferStartOffsetInBytes,Flags) ) 

#define ID3D12GraphicsCommandList3_ResolveSubresource(This,pDstResource,DstSubresource,pSrcResource,SrcSubresource,Format)	\
    ( (This)->lpVtbl -> ResolveSubresource(This,pDstResource,DstSubresource,pSrcResource,SrcSubresource,Format) ) 

#define ID3D12GraphicsCommandList3_IASetPrimitiveTopology(This,PrimitiveTopology)	\
    ( (This)->lpVtbl -> IASetPrimitiveTopology(This,PrimitiveTopology) ) 

#define ID3D12GraphicsCommandList3_RSSetViewports(This,NumViewports,pViewports)	\
    ( (This)->lpVtbl -> RSSetViewports(This,NumViewports,pViewports) ) 

#define ID3D12GraphicsCommandList3_RSSetScissorRects(This,NumRects,pRects)	\
    ( (This)->lpVtbl -> RSSetScissorRects(This,NumRects,pRects) ) 

#define ID3D12GraphicsCommandList3_OMSetBlendFactor(This,BlendFactor)	\
    ( (This)->lpVtbl -> OMSetBlendFactor(This,BlendFactor) ) 

#define ID3D12GraphicsCommandList3_OMSetStencilRef(This,StencilRef)	\
    ( (This)->lpVtbl -> OMSetStencilRef(This,StencilRef) ) 

#define ID3D12GraphicsCommandList3_SetPipelineState(This,pPipelineState)	\
    ( (This)->lpVtbl -> SetPipelineState(This,pPipelineState) ) 

#define ID3D12GraphicsCommandList3_ResourceBarrier(This,NumBarriers,pBarriers)	\
    ( (This)->lpVtbl -> ResourceBarrier(This,NumBarriers,pBarriers) ) 

#define ID3D12GraphicsCommandList3_ExecuteBundle(This,pCommandList)	\
    ( (This)->lpVtbl -> ExecuteBundle(This,pCommandList) ) 

#define ID3D12GraphicsCommandList3_SetDescriptorHeaps(This,NumDescriptorHeaps,ppDescriptorHeaps)	\
    ( (This)->lpVtbl -> SetDescriptorHeaps(This,NumDescriptorHeaps,ppDescriptorHeaps) ) 

#define ID3D12GraphicsCommandList3_SetComputeRootSignature(This,pRootSignature)	\
    ( (This)->lpVtbl -> SetComputeRootSignature(This,pRootSignature) ) 

#define ID3D12GraphicsCommandList3_SetGraphicsRootSignature(This,pRootSignature)	\
    ( (This)->lpVtbl -> SetGraphicsRootSignature(This,pRootSignature) ) 

#define ID3D12GraphicsCommandList3_SetComputeRootDescriptorTable(This,RootParameterIndex,BaseDescriptor)	\
    ( (This)->lpVtbl -> SetComputeRootDescriptorTable(This,RootParameterIndex,BaseDescriptor) ) 

#define ID3D12GraphicsCommandList3_SetGraphicsRootDescriptorTable(This,RootParameterIndex,BaseDescriptor)	\
    ( (This)->lpVtbl -> SetGraphicsRootDescriptorTable(This,RootParameterIndex,BaseDescriptor) ) 

#define ID3D12GraphicsCommandList3_SetComputeRoot32BitConstant(This,RootParameterIndex,SrcData,DestOffsetIn32BitValues)	\
    ( (This)->lpVtbl -> SetComputeRoot32BitConstant(This,RootParameterIndex,SrcData,DestOffsetIn32BitValues) ) 

#define ID3D12GraphicsCommandList3_SetGraphicsRoot32BitConstant(This,RootParameterIndex,SrcData,DestOffsetIn32BitValues)	\
    ( (This)->lpVtbl -> SetGraphicsRoot32BitConstant(This,RootParameterIndex,SrcData,DestOffsetIn32BitValues) ) 

#define ID3D12GraphicsCommandList3_SetComputeRoot32BitConstants(This,RootParameterIndex,Num32BitValuesToSet,pSrcData,DestOffsetIn32BitValues)	\
    ( (This)->lpVtbl -> SetComputeRoot32BitConstants(This,RootParameterIndex,Num32BitValuesToSet,pSrcData,DestOffsetIn32BitValues) ) 

#define ID3D12GraphicsCommandList3_SetGraphicsRoot32BitConstants(This,RootParameterIndex,Num32BitValuesToSet,pSrcData,DestOffsetIn32BitValues)	\
    ( (This)->lpVtbl -> SetGraphicsRoot32BitConstants(This,RootParameterIndex,Num32BitValuesToSet,pSrcData,DestOffsetIn32BitValues) ) 

#define ID3D12GraphicsCommandList3_SetComputeRootConstantBufferView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetComputeRootConstantBufferView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList3_SetGraphicsRootConstantBufferView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetGraphicsRootConstantBufferView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList3_SetComputeRootShaderResourceView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetComputeRootShaderResourceView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList3_SetGraphicsRootShaderResourceView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetGraphicsRootShaderResourceView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList3_SetComputeRootUnorderedAccessView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetComputeRootUnorderedAccessView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList3_SetGraphicsRootUnorderedAccessView(This,RootParameterIndex,BufferLocation)	\
    ( (This)->lpVtbl -> SetGraphicsRootUnorderedAccessView(This,RootParameterIndex,BufferLocation) ) 

#define ID3D12GraphicsCommandList3_IASetIndexBuffer(This,pView)	\
    ( (This)->lpVtbl -> IASetIndexBuffer(This,pView) ) 

#define ID3D12GraphicsCommandList3_IASetVertexBuffers(This,StartSlot,NumViews,pViews)	\
    ( (This)->lpVtbl -> IASetVertexBuffers(This,StartSlot,NumViews,pViews) ) 

#define ID3D12GraphicsCommandList3_SOSetTargets(This,StartSlot,NumViews,pViews)	\
    ( (This)->lpVtbl -> SOSetTargets(This,StartSlot,NumViews,pViews) ) 

#define ID3D12GraphicsCommandList3_OMSetRenderTargets(This,NumRenderTargetDescriptors,pRenderTargetDescriptors,RTsSingleHandleToDescriptorRange,pDepthStencilDescriptor)	\
    ( (This)->lpVtbl -> OMSetRenderTargets(This,NumRenderTargetDescriptors,pRenderTargetDescriptors,RTsSingleHandleToDescriptorRange,pDepthStencilDescriptor) ) 

#define ID3D12GraphicsCommandList3_ClearDepthStencilView(This,DepthStencilView,ClearFlags,Depth,Stencil,NumRects,pRects)	\
    ( (This)->lpVtbl -> ClearDepthStencilView(This,DepthStencilView,ClearFlags,Depth,Stencil,NumRects,pRects) ) 

#define ID3D12GraphicsCommandList3_ClearRenderTargetView(This,RenderTargetView,ColorRGBA,NumRects,pRects)	\
    ( (This)->lpVtbl -> ClearRenderTargetView(This,RenderTargetView,ColorRGBA,NumRects,pRects) ) 

#define ID3D12GraphicsCommandList3_ClearUnorderedAccessViewUint(This,ViewGPUHandleInCurrentHeap,ViewCPUHandle,pResource,Values,NumRects,pRects)	\
    ( (This)->lpVtbl -> ClearUnorderedAccessViewUint(This,ViewGPUHandleInCurrentHeap,ViewCPUHandle,pResource,Values,NumRects,pRects) ) 

#define ID3D12GraphicsCommandList3_ClearUnorderedAccessViewFloat(This,ViewGPUHandleInCurrentHeap,ViewCPUHandle,pResource,Values,NumRects,pRects)	\
    ( (This)->lpVtbl -> ClearUnorderedAccessViewFloat(This,ViewGPUHandleInCurrentHeap,ViewCPUHandle,pResource,Values,NumRects,pRects) ) 

#define ID3D12GraphicsCommandList3_DiscardResource(This,pResource,pRegion)	\
    ( (This)->lpVtbl -> DiscardResource(This,pResource,pRegion) ) 

#define ID3D12GraphicsCommandList3_BeginQuery(This,pQueryHeap,Type,Index)	\
    ( (This)->lpVtbl -> BeginQuery(This,pQueryHeap,Type,Index) ) 

#define ID3D12GraphicsCommandList3_EndQuery(This,pQueryHeap,Type,Index)	\
    ( (This)->lpVtbl -> EndQuery(This,pQueryHeap,Type,Index) ) 

#define ID3D12GraphicsCommandList3_ResolveQueryData(This,pQueryHeap,Type,StartIndex,NumQueries,pDestinationBuffer,AlignedDestinationBufferOffset)	\
    ( (This)->lpVtbl -> ResolveQueryData(This,pQueryHeap,Type,StartIndex,NumQueries,pDestinationBuffer,AlignedDestinationBufferOffset) ) 

#define ID3D12GraphicsCommandList3_SetPredication(This,pBuffer,AlignedBufferOffset,Operation)	\
    ( (This)->lpVtbl -> SetPredication(This,pBuffer,AlignedBufferOffset,Operation) ) 

#define ID3D12GraphicsCommandList3_SetMarker(This,Metadata,pData,Size)	\
    ( (This)->lpVtbl -> SetMarker(This,Metadata,pData,Size) ) 

#define ID3D12GraphicsCommandList3_BeginEvent(This,Metadata,pData,Size)	\
    ( (This)->lpVtbl -> BeginEvent(This,Metadata,pData,Size) ) 

#define ID3D12GraphicsCommandList3_EndEvent(This)	\
    ( (This)->lpVtbl -> EndEvent(This) ) 

#define ID3D12GraphicsCommandList3_ExecuteIndirect(This,pCommandSignature,MaxCommandCount,pArgumentBuffer,ArgumentBufferOffset,pCountBuffer,CountBufferOffset)	\
    ( (This)->lpVtbl -> ExecuteIndirect(This,pCommandSignature,MaxCommandCount,pArgumentBuffer,ArgumentBufferOffset,pCountBuffer,CountBufferOffset) ) 


#define ID3D12GraphicsCommandList3_AtomicCopyBufferUINT(This,pDstBuffer,DstOffset,pSrcBuffer,SrcOffset,Dependencies,ppDependentResources,pDependentSubresourceRanges)	\
    ( (This)->lpVtbl -> AtomicCopyBufferUINT(This,pDstBuffer,DstOffset,pSrcBuffer,SrcOffset,Dependencies,ppDependentResources,pDependentSubresourceRanges) ) 

#define ID3D12GraphicsCommandList3_AtomicCopyBufferUINT64(This,pDstBuffer,DstOffset,pSrcBuffer,SrcOffset,Dependencies,ppDependentResources,pDependentSubresourceRanges)	\
    ( (This)->lpVtbl -> AtomicCopyBufferUINT64(This,pDstBuffer,DstOffset,pSrcBuffer,SrcOffset,Dependencies,ppDependentResources,pDependentSubresourceRanges) ) 

#define ID3D12GraphicsCommandList3_OMSetDepthBounds(This,Min,Max)	\
    ( (This)->lpVtbl -> OMSetDepthBounds(This,Min,Max) ) 

#define ID3D12GraphicsCommandList3_SetSamplePositions(This,NumSamplesPerPixel,NumPixels,pSamplePositions)	\
    ( (This)->lpVtbl -> SetSamplePositions(This,NumSamplesPerPixel,NumPixels,pSamplePositions) ) 

#define ID3D12GraphicsCommandList3_ResolveSubresourceRegion(This,pDstResource,DstSubresource,DstX,DstY,pSrcResource,SrcSubresource,pSrcRect,Format,ResolveMode)	\
    ( (This)->lpVtbl -> ResolveSubresourceRegion(This,pDstResource,DstSubresource,DstX,DstY,pSrcResource,SrcSubresource,pSrcRect,Format,ResolveMode) ) 

#define ID3D12GraphicsCommandList3_SetViewInstanceMask(This,Mask)	\
    ( (This)->lpVtbl -> SetViewInstanceMask(This,Mask) ) 


#define ID3D12GraphicsCommandList3_WriteBufferImmediate(This,Count,pParams,pModes)	\
    ( (This)->lpVtbl -> WriteBufferImmediate(This,Count,pParams,pModes) ) 


#define ID3D12GraphicsCommandList3_SetProtectedResourceSession(This,pProtectedResourceSession)	\
    ( (This)->lpVtbl -> SetProtectedResourceSession(This,pProtectedResourceSession) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12GraphicsCommandList3_INTERFACE_DEFINED__ */


#ifndef __ID3D12Tools_INTERFACE_DEFINED__
#define __ID3D12Tools_INTERFACE_DEFINED__

  /* interface ID3D12Tools */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D12Tools;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("7071e1f0-e84b-4b33-974f-12fa49de65c5")
    ID3D12Tools : public IUnknown
  {
  public:
    virtual void STDMETHODCALLTYPE EnableShaderInstrumentation( 
      BOOL bEnable) = 0;

    virtual BOOL STDMETHODCALLTYPE ShaderInstrumentationEnabled( void) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D12ToolsVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D12Tools * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D12Tools * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D12Tools * This);

    void ( STDMETHODCALLTYPE *EnableShaderInstrumentation )( 
      ID3D12Tools * This,
      BOOL bEnable);

    BOOL ( STDMETHODCALLTYPE *ShaderInstrumentationEnabled )( 
      ID3D12Tools * This);

    END_INTERFACE
  } ID3D12ToolsVtbl;

  interface ID3D12Tools
  {
    CONST_VTBL struct ID3D12ToolsVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D12Tools_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12Tools_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12Tools_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12Tools_EnableShaderInstrumentation(This,bEnable)	\
    ( (This)->lpVtbl -> EnableShaderInstrumentation(This,bEnable) ) 

#define ID3D12Tools_ShaderInstrumentationEnabled(This)	\
    ( (This)->lpVtbl -> ShaderInstrumentationEnabled(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12Tools_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_d3d12_0000_0033 */
  /* [local] */ 

  typedef struct D3D12_SUBRESOURCE_DATA
  {
    const void *pData;
    LONG_PTR RowPitch;
    LONG_PTR SlicePitch;
  } 	D3D12_SUBRESOURCE_DATA;

  typedef struct D3D12_MEMCPY_DEST
  {
    void *pData;
    SIZE_T RowPitch;
    SIZE_T SlicePitch;
  } 	D3D12_MEMCPY_DEST;

//#if !defined( D3D12_IGNORE_SDK_LAYERS ) 
//#include "d3d12sdklayers.h" 
//#endif 

  ///////////////////////////////////////////////////////////////////////////
  // D3D12CreateDevice
  // ------------------
  //
  // pAdapter
  //      If NULL, D3D12CreateDevice will choose the primary adapter.
  //      If non-NULL, D3D12CreateDevice will use the provided adapter.
  // MinimumFeatureLevel
  //      The minimum feature level required for successful device creation.
  // riid
  //      The interface IID of the device to be returned. Expected: ID3D12Device.
  // ppDevice
  //      Pointer to returned interface. May be NULL.
  //
  // Return Values
  //  Any of those documented for 
  //          CreateDXGIFactory1
  //          IDXGIFactory::EnumAdapters
  //          D3D12CreateDevice
  //
  ///////////////////////////////////////////////////////////////////////////
  typedef HRESULT (WINAPI* PFN_D3D12_CREATE_DEVICE)( _In_opt_ IUnknown*, 
                                                    D3D_FEATURE_LEVEL, 
                                                    _In_ REFIID, _COM_Outptr_opt_ void** );

  HRESULT WINAPI D3D12CreateDevice(
    _In_opt_ IUnknown* pAdapter,
    D3D_FEATURE_LEVEL MinimumFeatureLevel,
    _In_ REFIID riid, // Expected: ID3D12Device
    _COM_Outptr_opt_ void** ppDevice );


  typedef HRESULT (WINAPI* PFN_D3D12_GET_DEBUG_INTERFACE)( _In_ REFIID, _COM_Outptr_opt_ void** );

  HRESULT WINAPI D3D12GetDebugInterface( _In_ REFIID riid, _COM_Outptr_opt_ void** ppvDebug );

  // --------------------------------------------------------------------------------------------------------------------------------
  // D3D12EnableExperimentalFeatures
  //
  // Pass in a list of feature GUIDs to be enabled together.
  // 
  // If a particular feature requires some configuration information on enablement, it will have
  // a configuration struct that can be passed alongside the GUID.
  // 
  // Some features might use an interface IID as the GUID.  For these, once the feature is enabled via
  // D3D12EnableExperimentalFeatures, D3D12GetDebugInterface can then be called with the IID to retrieve the interface
  // for manipulating the feature.  This allows for control that might not cleanly be expressed by just 
  // the configuration struct that D3D12EnableExperimentalFeatures provides.
  //
  // If this method is called and a change to existing feature enablement is made, 
  // all current D3D12 devices are set to DEVICE_REMOVED state, since under the covers there is really only one
  // singleton device for a process.  Removing the devices when configuration changes prevents
  // mismatched expectations of how a device is supposed to work after it has been created from the app's point of view.
  //
  // The call returns E_NOINTERFACE if an unrecognized feature is passed in or Windows Developer mode is not on.
  // The call returns E_INVALIDARG if the configuration of a feature is incorrect, the set of features passed
  // in are known to be incompatible with each other, or other errors.
  // Returns S_OK otherwise.
  //
  // --------------------------------------------------------------------------------------------------------------------------------
  HRESULT WINAPI D3D12EnableExperimentalFeatures(
    UINT                                    NumFeatures,
    __in_ecount(NumFeatures) const IID*     pIIDs,
    __in_ecount_opt(NumFeatures) void*      pConfigurationStructs,
    __in_ecount_opt(NumFeatures) UINT*      pConfigurationStructSizes);

  // --------------------------------------------------------------------------------------------------------------------------------
  // Experimental Feature: D3D12ExperimentalShaderModels
  //
  // Use with D3D12EnableExperimentalFeatures to enable experimental shader model support,
  // meaning shader models that haven't been finalized for use in retail.
  //
  // Enabling D3D12ExperimentalShaderModels needs no configuration struct, pass NULL in the pConfigurationStructs array.
  //
  // --------------------------------------------------------------------------------------------------------------------------------
  static const UUID D3D12ExperimentalShaderModels = { /* 76f5573e-f13a-40f5-b297-81ce9e18933f */
    0x76f5573e,
    0xf13a,
    0x40f5,
  { 0xb2, 0x97, 0x81, 0xce, 0x9e, 0x18, 0x93, 0x3f }
  };
  // --------------------------------------------------------------------------------------------------------------------------------
  // Experimental Feature: D3D12TiledResourceTier4
  //
  // Use with D3D12EnableExperimentalFeatures to enable tiled resource tier 4 support,
  // meaning texture tile data-inheritance is allowed.
  //
  // Enabling D3D12TiledResourceTier4 needs no configuration struct, pass NULL in the pConfigurationStructs array.
  //
  // --------------------------------------------------------------------------------------------------------------------------------
  static const UUID D3D12TiledResourceTier4 = { /* c9c4725f-a81a-4f56-8c5b-c51039d694fb */
    0xc9c4725f,
    0xa81a,
    0x4f56,
  { 0x8c, 0x5b, 0xc5, 0x10, 0x39, 0xd6, 0x94, 0xfb }
  };
  // --------------------------------------------------------------------------------------------------------------------------------
  // Experimental Feature: D3D12RaytracingPrototype
  //
  // Use with D3D12EnableExperimentalFeatures to enable the D3D12 Raytracing Prototype.
  //
  // Enabling D3D12RaytracingPrototype needs no configuration struct, pass NULL in the pConfigurationStructs array.
  //
  // --------------------------------------------------------------------------------------------------------------------------------
  static const UUID D3D12RaytracingPrototype = { /* 5d15d3b2-015a-4f39-8d47-299ac37190d3 */
    0x5d15d3b2,
    0x015a,
    0x4f39,
  {0x8d, 0x47, 0x29, 0x9a, 0xc3, 0x71, 0x90, 0xd3}
  };
  // --------------------------------------------------------------------------------------------------------------------------------
  // Experimental Feature: D3D12MetaCommand
  //
  // Use with D3D12EnableExperimentalFeatures to enable the D3D12 Meta Command.
  //
  // Enabling D3D12MetaCommand needs no configuration struct, pass NULL in the pConfigurationStructs array.
  //
  // --------------------------------------------------------------------------------------------------------------------------------
  static const UUID D3D12MetaCommand = { /* C734C97E-8077-48C8-9FDC-D9D1DD31DD77 */
    0xc734c97e,
    0x8077,
    0x48c8,
  { 0x9f, 0xdc, 0xd9, 0xd1, 0xdd, 0x31, 0xdd, 0x77 }
  };
#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion
  DEFINE_GUID(IID_ID3D12Object,0xc4fec28f,0x7966,0x4e95,0x9f,0x94,0xf4,0x31,0xcb,0x56,0xc3,0xb8);
  DEFINE_GUID(IID_ID3D12DeviceChild,0x905db94b,0xa00c,0x4140,0x9d,0xf5,0x2b,0x64,0xca,0x9e,0xa3,0x57);
  DEFINE_GUID(IID_ID3D12RootSignature,0xc54a6b66,0x72df,0x4ee8,0x8b,0xe5,0xa9,0x46,0xa1,0x42,0x92,0x14);
  DEFINE_GUID(IID_ID3D12RootSignatureDeserializer,0x34AB647B,0x3CC8,0x46AC,0x84,0x1B,0xC0,0x96,0x56,0x45,0xC0,0x46);
  DEFINE_GUID(IID_ID3D12VersionedRootSignatureDeserializer,0x7F91CE67,0x090C,0x4BB7,0xB7,0x8E,0xED,0x8F,0xF2,0xE3,0x1D,0xA0);
  DEFINE_GUID(IID_ID3D12Pageable,0x63ee58fb,0x1268,0x4835,0x86,0xda,0xf0,0x08,0xce,0x62,0xf0,0xd6);
  DEFINE_GUID(IID_ID3D12Heap,0x6b3b2502,0x6e51,0x45b3,0x90,0xee,0x98,0x84,0x26,0x5e,0x8d,0xf3);
  DEFINE_GUID(IID_ID3D12Resource,0x696442be,0xa72e,0x4059,0xbc,0x79,0x5b,0x5c,0x98,0x04,0x0f,0xad);
  DEFINE_GUID(IID_ID3D12CommandAllocator,0x6102dee4,0xaf59,0x4b09,0xb9,0x99,0xb4,0x4d,0x73,0xf0,0x9b,0x24);
  DEFINE_GUID(IID_ID3D12Fence,0x0a753dcf,0xc4d8,0x4b91,0xad,0xf6,0xbe,0x5a,0x60,0xd9,0x5a,0x76);
  DEFINE_GUID(IID_ID3D12Fence1,0x433685fe,0xe22b,0x4ca0,0xa8,0xdb,0xb5,0xb4,0xf4,0xdd,0x0e,0x4a);
  DEFINE_GUID(IID_ID3D12PipelineState,0x765a30f3,0xf624,0x4c6f,0xa8,0x28,0xac,0xe9,0x48,0x62,0x24,0x45);
  DEFINE_GUID(IID_ID3D12DescriptorHeap,0x8efb471d,0x616c,0x4f49,0x90,0xf7,0x12,0x7b,0xb7,0x63,0xfa,0x51);
  DEFINE_GUID(IID_ID3D12QueryHeap,0x0d9658ae,0xed45,0x469e,0xa6,0x1d,0x97,0x0e,0xc5,0x83,0xca,0xb4);
  DEFINE_GUID(IID_ID3D12CommandSignature,0xc36a797c,0xec80,0x4f0a,0x89,0x85,0xa7,0xb2,0x47,0x50,0x82,0xd1);
  DEFINE_GUID(IID_ID3D12CommandList,0x7116d91c,0xe7e4,0x47ce,0xb8,0xc6,0xec,0x81,0x68,0xf4,0x37,0xe5);
  DEFINE_GUID(IID_ID3D12GraphicsCommandList,0x5b160d0f,0xac1b,0x4185,0x8b,0xa8,0xb3,0xae,0x42,0xa5,0xa4,0x55);
  DEFINE_GUID(IID_ID3D12GraphicsCommandList1,0x553103fb,0x1fe7,0x4557,0xbb,0x38,0x94,0x6d,0x7d,0x0e,0x7c,0xa7);
  DEFINE_GUID(IID_ID3D12GraphicsCommandList2,0x38C3E585,0xFF17,0x412C,0x91,0x50,0x4F,0xC6,0xF9,0xD7,0x2A,0x28);
  DEFINE_GUID(IID_ID3D12CommandQueue,0x0ec870a6,0x5d7e,0x4c22,0x8c,0xfc,0x5b,0xaa,0xe0,0x76,0x16,0xed);
  DEFINE_GUID(IID_ID3D12Device,0x189819f1,0x1db6,0x4b57,0xbe,0x54,0x18,0x21,0x33,0x9b,0x85,0xf7);
  DEFINE_GUID(IID_ID3D12PipelineLibrary,0xc64226a8,0x9201,0x46af,0xb4,0xcc,0x53,0xfb,0x9f,0xf7,0x41,0x4f);
  DEFINE_GUID(IID_ID3D12PipelineLibrary1,0x80eabf42,0x2568,0x4e5e,0xbd,0x82,0xc3,0x7f,0x86,0x96,0x1d,0xc3);
  DEFINE_GUID(IID_ID3D12Device1,0x77acce80,0x638e,0x4e65,0x88,0x95,0xc1,0xf2,0x33,0x86,0x86,0x3e);
  DEFINE_GUID(IID_ID3D12Device2,0x30baa41e,0xb15b,0x475c,0xa0,0xbb,0x1a,0xf5,0xc5,0xb6,0x43,0x28);
  DEFINE_GUID(IID_ID3D12Device3,0x81dadc15,0x2bad,0x4392,0x93,0xc5,0x10,0x13,0x45,0xc4,0xaa,0x98);
  DEFINE_GUID(IID_ID3D12ProtectedSession,0xA1533D18,0x0AC1,0x4084,0x85,0xB9,0x89,0xA9,0x61,0x16,0x80,0x6B);
  DEFINE_GUID(IID_ID3D12ProtectedResourceSession,0x6CD696F4,0xF289,0x40CC,0x80,0x91,0x5A,0x6C,0x0A,0x09,0x9C,0x3D);
  DEFINE_GUID(IID_ID3D12Device4,0xe865df17,0xa9ee,0x46f9,0xa4,0x63,0x30,0x98,0x31,0x5a,0xa2,0xe5);
  DEFINE_GUID(IID_ID3D12Resource1,0x9D5E227A,0x4430,0x4161,0x88,0xB3,0x3E,0xCA,0x6B,0xB1,0x6E,0x19);
  DEFINE_GUID(IID_ID3D12Heap1,0x572F7389,0x2168,0x49E3,0x96,0x93,0xD6,0xDF,0x58,0x71,0xBF,0x6D);
  DEFINE_GUID(IID_ID3D12GraphicsCommandList3,0x6FDA83A7,0xB84C,0x4E38,0x9A,0xC8,0xC7,0xBD,0x22,0x01,0x6B,0x3D);
  DEFINE_GUID(IID_ID3D12Tools,0x7071e1f0,0xe84b,0x4b33,0x97,0x4f,0x12,0xfa,0x49,0xde,0x65,0xc5);


  extern RPC_IF_HANDLE __MIDL_itf_d3d12_0000_0033_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_d3d12_0000_0033_v0_0_s_ifspec;

  /* Additional Prototypes for ALL interfaces */

  /* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif





































interface ID3D11Device;
interface ID3D12Device;

#include <atlbase.h>
struct IWrapDXGISwapChain : IDXGISwapChain4
{
  IWrapDXGISwapChain ( ID3D11Device   *pDevice,
                       IDXGISwapChain *pSwapChain ) :
    pReal (pSwapChain),
    pDev  (pDevice),
    ver_  (0)
  {
                                  pSwapChain->AddRef  (),
    InterlockedExchange  (&refs_, pSwapChain->Release ());
    InterlockedIncrement (&SK_DXGI_LiveWrappedSwapChains);

    //// Immediately try to upgrade
    CComQIPtr <IDXGISwapChain4> pSwap4 (this);
  }

  IWrapDXGISwapChain ( ID3D11Device    *pDevice,
                       IDXGISwapChain1 *pSwapChain ) :
    pReal (pSwapChain),
    pDev  (pDevice),
    ver_  (1)
  {
                                  pSwapChain->AddRef  (),
    InterlockedExchange  (&refs_, pSwapChain->Release ());
    InterlockedIncrement (&SK_DXGI_LiveWrappedSwapChain1s);

    //// Immediately try to upgrade
    CComQIPtr <IDXGISwapChain4> pSwap4 (this);
  }

  //IWrapDXGISwapChain ( ID3D12Device   *pDevice12,
  //                     IDXGISwapChain *pSwapChain ) :
  //  pReal  (pSwapChain),
  //  pDev12 ( pDevice12),
  //  ver_   (    0     )
  //{
  //                                pSwapChain->AddRef  (),
  //  InterlockedExchange  (&refs_, pSwapChain->Release ());
  //  InterlockedIncrement (&SK_DXGI_LiveWrappedSwapChains);
  //
  //  //// Immediately try to upgrade
  //  CComQIPtr <IDXGISwapChain4> pSwap4 (this);
  //}



  virtual ~IWrapDXGISwapChain (void)
  {
  }


  IWrapDXGISwapChain            (const IWrapDXGISwapChain &) = delete;
  IWrapDXGISwapChain &operator= (const IWrapDXGISwapChain &) = delete;

  #pragma region IUnknown
  virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, void **ppvObj) override; // 0
  virtual ULONG   STDMETHODCALLTYPE AddRef         (void)                       override; // 1
  virtual ULONG   STDMETHODCALLTYPE Release        (void)                       override; // 2
  #pragma endregion

  #pragma region IDXGIObject
  virtual HRESULT STDMETHODCALLTYPE SetPrivateData          (REFGUID Name,        UINT       DataSize, const void *pData) override; // 3
  virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface (REFGUID Name, const  IUnknown  *pUnknown)                    override; // 4
  virtual HRESULT STDMETHODCALLTYPE GetPrivateData          (REFGUID Name,        UINT      *pDataSize,      void *pData) override; // 5
  virtual HRESULT STDMETHODCALLTYPE GetParent               (REFIID  riid,        void     **ppParent)                    override; // 6
  #pragma endregion

  #pragma region IDXGIDeviceSubObject
  virtual HRESULT STDMETHODCALLTYPE GetDevice (REFIID riid, void **ppDevice) override; // 7
  #pragma endregion

  #pragma region IDXGISwapChain
  virtual HRESULT STDMETHODCALLTYPE Present             (      UINT                   SyncInterval, UINT          Flags)                   override; // 8
  virtual HRESULT STDMETHODCALLTYPE GetBuffer           (      UINT                   Buffer,       REFIID        riid,  void **ppSurface) override; // 9
  virtual HRESULT STDMETHODCALLTYPE SetFullscreenState  (      BOOL                   Fullscreen,   IDXGIOutput  *pTarget)                 override; // 10
  virtual HRESULT STDMETHODCALLTYPE GetFullscreenState  (      BOOL                   *pFullscreen, IDXGIOutput **ppTarget)                override; // 11
  virtual HRESULT STDMETHODCALLTYPE GetDesc             (      DXGI_SWAP_CHAIN_DESC   *pDesc)                                              override; // 12
  virtual HRESULT STDMETHODCALLTYPE ResizeBuffers       (      UINT                    BufferCount, UINT          Width, UINT   Height,
                                                               DXGI_FORMAT             NewFormat,   UINT          SwapChainFlags )         override; // 13
  virtual HRESULT STDMETHODCALLTYPE ResizeTarget        (const DXGI_MODE_DESC         *pNewTargetParameters)                               override; // 14
  virtual HRESULT STDMETHODCALLTYPE GetContainingOutput (      IDXGIOutput           **ppOutput)                                           override; // 15
  virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics  (      DXGI_FRAME_STATISTICS  *pStats)                                             override; // 16
  virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount (      UINT                   *pLastPresentCount)                                  override; // 17
  #pragma endregion

  #pragma region IDXGISwapChain1
  virtual HRESULT STDMETHODCALLTYPE GetDesc1                 (      DXGI_SWAP_CHAIN_DESC1            *pDesc)                override; // 18
  virtual HRESULT STDMETHODCALLTYPE GetFullscreenDesc        (      DXGI_SWAP_CHAIN_FULLSCREEN_DESC  *pDesc)                override; // 19
  virtual HRESULT STDMETHODCALLTYPE GetHwnd                  (      HWND                             *pHwnd)                override; // 20
  virtual HRESULT STDMETHODCALLTYPE GetCoreWindow            (      REFIID                            refiid, void **ppUnk) override; // 21
  virtual HRESULT STDMETHODCALLTYPE Present1                 (      UINT                              SyncInterval,
                                                                    UINT                              PresentFlags,
                                                              const DXGI_PRESENT_PARAMETERS          *pPresentParameters)   override; // 22
  virtual BOOL    STDMETHODCALLTYPE IsTemporaryMonoSupported (      void)                                                   override; // 23
  virtual HRESULT STDMETHODCALLTYPE GetRestrictToOutput      (      IDXGIOutput                     **ppRestrictToOutput)   override; // 24
  virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor       (const DXGI_RGBA                        *pColor)               override; // 25
  virtual HRESULT STDMETHODCALLTYPE GetBackgroundColor       (      DXGI_RGBA                        *pColor)               override; // 26
  virtual HRESULT STDMETHODCALLTYPE SetRotation              (      DXGI_MODE_ROTATION                Rotation)             override; // 27
  virtual HRESULT STDMETHODCALLTYPE GetRotation              (      DXGI_MODE_ROTATION               *pRotation)            override; // 28
  #pragma endregion

  #pragma region IDXGISwapChain2
  virtual HRESULT STDMETHODCALLTYPE SetSourceSize                 (      UINT               Width,  UINT  Height)  override; // 29
  virtual HRESULT STDMETHODCALLTYPE GetSourceSize                 (      UINT              *pWidth, UINT *pHeight) override; // 30
  virtual HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency        (      UINT               MaxLatency)            override; // 31
  virtual HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency        (      UINT              *pMaxLatency)           override; // 32
  virtual HANDLE  STDMETHODCALLTYPE GetFrameLatencyWaitableObject (      void)                                     override; // 33
  virtual HRESULT STDMETHODCALLTYPE SetMatrixTransform            (const DXGI_MATRIX_3X2_F *pMatrix)               override; // 34
  virtual HRESULT STDMETHODCALLTYPE GetMatrixTransform            (      DXGI_MATRIX_3X2_F *pMatrix)               override; // 35
  #pragma endregion
  #pragma region IDXGISwapChain3
  virtual UINT    STDMETHODCALLTYPE GetCurrentBackBufferIndex (void)                                                                         override; // 36
  virtual HRESULT STDMETHODCALLTYPE CheckColorSpaceSupport    (DXGI_COLOR_SPACE_TYPE  ColorSpace,           UINT        *pColorSpaceSupport) override; // 37
  virtual HRESULT STDMETHODCALLTYPE SetColorSpace1            (DXGI_COLOR_SPACE_TYPE  ColorSpace)                                            override; // 38
  virtual HRESULT STDMETHODCALLTYPE ResizeBuffers1            (UINT                   BufferCount,          UINT         Width,
                                                               UINT                   Height,               DXGI_FORMAT  Format,
                                                               UINT                   SwapChainFlags, const UINT        *pCreationNodeMask,
                                                               IUnknown *const       *ppPresentQueue)                                        override; // 39
  #pragma endregion
  #pragma region IDXGISwapChain4
  virtual HRESULT STDMETHODCALLTYPE SetHDRMetaData (DXGI_HDR_METADATA_TYPE Type, UINT Size, void *pMetaData) override; // 40
  #pragma endregion

  volatile LONG   refs_ = 1;
  IDXGISwapChain *pReal;
  ID3D11Device   *pDev;
//ID3D12Device   *pDev12;
  unsigned int    ver_;
};













/*-------------------------------------------------------------------------------------
*
* Copyright (c) Microsoft Corporation
*
*-------------------------------------------------------------------------------------*/


/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 8.01.0622 */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
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

#ifndef __d3d11on12_h__
#define __d3d11on12_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ID3D11On12Device_FWD_DEFINED__
#define __ID3D11On12Device_FWD_DEFINED__
typedef interface ID3D11On12Device ID3D11On12Device;

#endif 	/* __ID3D11On12Device_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "d3d11.h"
//#include "d3d12.h"

#ifdef __cplusplus
extern "C"{
#endif 


  /* interface __MIDL_itf_d3d11on12_0000_0000 */
  /* [local] */ 

  /*#include <winapifamily.h>*/
  /*#pragma region App Family*/
  /*#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)*/

  ///////////////////////////////////////////////////////////////////////////
  // D3D11On12CreateDevice
  // ------------------
  //
  // pDevice
  //      Specifies a pre-existing D3D12 device to use for D3D11 interop.
  //      May not be NULL.
  // Flags
  //      Any of those documented for D3D11CreateDeviceAndSwapChain.
  // pFeatureLevels
  //      Array of any of the following:
  //          D3D_FEATURE_LEVEL_12_1
  //          D3D_FEATURE_LEVEL_12_0
  //          D3D_FEATURE_LEVEL_11_1
  //          D3D_FEATURE_LEVEL_11_0
  //          D3D_FEATURE_LEVEL_10_1
  //          D3D_FEATURE_LEVEL_10_0
  //          D3D_FEATURE_LEVEL_9_3
  //          D3D_FEATURE_LEVEL_9_2
  //          D3D_FEATURE_LEVEL_9_1
  //       The first feature level which is less than or equal to the
  //       D3D12 device's feature level will be used to perform D3D11 validation.
  //       Creation will fail if no acceptable feature levels are provided.
  //       Providing NULL will default to the D3D12 device's feature level.
  // FeatureLevels
  //      Size of feature levels array.
  // ppCommandQueues
  //      Array of unique queues for D3D11On12 to use. Valid queue types:
  //          3D command queue.
  //      Flags must be compatible with device flags, and its NodeMask must
  //      be a subset of the NodeMask provided to this API.
  // NumQueues
  //      Size of command queue array.
  // NodeMask
  //      Which node of the D3D12 device to use.  Only 1 bit may be set.
  // ppDevice
  //      Pointer to returned interface. May be NULL.
  // ppImmediateContext
  //      Pointer to returned interface. May be NULL.
  // pChosenFeatureLevel
  //      Pointer to returned feature level. May be NULL.
  //
  // Return Values
  //  Any of those documented for 
  //          D3D11CreateDevice
  //
  ///////////////////////////////////////////////////////////////////////////

  typedef struct D3D11_RESOURCE_FLAGS
  {
    UINT BindFlags;
    UINT MiscFlags;
    UINT CPUAccessFlags;
    UINT StructureByteStride;
  } 	D3D11_RESOURCE_FLAGS;



  extern RPC_IF_HANDLE __MIDL_itf_d3d11on12_0000_0000_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_d3d11on12_0000_0000_v0_0_s_ifspec;

#ifndef __ID3D11On12Device_INTERFACE_DEFINED__
#define __ID3D11On12Device_INTERFACE_DEFINED__

  /* interface ID3D11On12Device */
  /* [unique][local][object][uuid] */ 


  EXTERN_C const IID IID_ID3D11On12Device;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("85611e73-70a9-490e-9614-a9e302777904")
    ID3D11On12Device : public IUnknown
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE CreateWrappedResource( 
      _In_  IUnknown *pResource12,
      _In_  const D3D11_RESOURCE_FLAGS *pFlags11,
      D3D12_RESOURCE_STATES InState,
      D3D12_RESOURCE_STATES OutState,
      REFIID riid,
      _COM_Outptr_opt_  void **ppResource11) = 0;

    virtual void STDMETHODCALLTYPE ReleaseWrappedResources( 
      _In_reads_( NumResources )  ID3D11Resource *const *ppResources,
      UINT NumResources) = 0;

    virtual void STDMETHODCALLTYPE AcquireWrappedResources( 
      _In_reads_( NumResources )  ID3D11Resource *const *ppResources,
      UINT NumResources) = 0;

  };


#else 	/* C style interface */

  typedef struct ID3D11On12DeviceVtbl
  {
    BEGIN_INTERFACE

      HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ID3D11On12Device * This,
        REFIID riid,
        _COM_Outptr_  void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )( 
      ID3D11On12Device * This);

    ULONG ( STDMETHODCALLTYPE *Release )( 
      ID3D11On12Device * This);

    HRESULT ( STDMETHODCALLTYPE *CreateWrappedResource )( 
      ID3D11On12Device * This,
      _In_  IUnknown *pResource12,
      _In_  const D3D11_RESOURCE_FLAGS *pFlags11,
      D3D12_RESOURCE_STATES InState,
      D3D12_RESOURCE_STATES OutState,
      REFIID riid,
      _COM_Outptr_opt_  void **ppResource11);

    void ( STDMETHODCALLTYPE *ReleaseWrappedResources )( 
      ID3D11On12Device * This,
      _In_reads_( NumResources )  ID3D11Resource *const *ppResources,
      UINT NumResources);

    void ( STDMETHODCALLTYPE *AcquireWrappedResources )( 
      ID3D11On12Device * This,
      _In_reads_( NumResources )  ID3D11Resource *const *ppResources,
      UINT NumResources);

    END_INTERFACE
  } ID3D11On12DeviceVtbl;

  interface ID3D11On12Device
  {
    CONST_VTBL struct ID3D11On12DeviceVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define ID3D11On12Device_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11On12Device_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11On12Device_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11On12Device_CreateWrappedResource(This,pResource12,pFlags11,InState,OutState,riid,ppResource11)	\
    ( (This)->lpVtbl -> CreateWrappedResource(This,pResource12,pFlags11,InState,OutState,riid,ppResource11) ) 

#define ID3D11On12Device_ReleaseWrappedResources(This,ppResources,NumResources)	\
    ( (This)->lpVtbl -> ReleaseWrappedResources(This,ppResources,NumResources) ) 

#define ID3D11On12Device_AcquireWrappedResources(This,ppResources,NumResources)	\
    ( (This)->lpVtbl -> AcquireWrappedResources(This,ppResources,NumResources) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11On12Device_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_d3d11on12_0000_0001 */
  /* [local] */ 

  /*#endif*/ /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
             /*#pragma endregion*/
  DEFINE_GUID(IID_ID3D11On12Device,0x85611e73,0x70a9,0x490e,0x96,0x14,0xa9,0xe3,0x02,0x77,0x79,0x04);


  extern RPC_IF_HANDLE __MIDL_itf_d3d11on12_0000_0001_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_d3d11on12_0000_0001_v0_0_s_ifspec;

  /* Additional Prototypes for ALL interfaces */

  /* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif







#endif /* __SK__DXGI_SWAPCHAIN_H__ */