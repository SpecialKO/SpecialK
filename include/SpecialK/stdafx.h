/**
* This file is part of Batman "Fix".
*
* Batman "Fix" is free software : you can redistribute it and / or modify
* it under the terms of the GNU General Public License as published by
* The Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Batman "Fix" is distributed in the hope that it will be useful,
* But WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Batman "Fix". If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#undef COM_NO_WINDOWS_H
#include <Windows.h>

#include "core.h"

#if 0

// TODO: reference additional headers your program requires here

/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

/* verify that the <rpcsal.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCSAL_H_VERSION__
#define __REQUIRED_RPCSAL_H_VERSION__ 100
#endif

#define DXGI_USAGE_SHADER_INPUT             ( 1L << (0 + 4) )
#define DXGI_USAGE_RENDER_TARGET_OUTPUT     ( 1L << (1 + 4) )
#define DXGI_USAGE_BACK_BUFFER              ( 1L << (2 + 4) )
#define DXGI_USAGE_SHARED                   ( 1L << (3 + 4) )
#define DXGI_USAGE_READ_ONLY                ( 1L << (4 + 4) )
#define DXGI_USAGE_DISCARD_ON_PRESENT       ( 1L << (5 + 4) )
#define DXGI_USAGE_UNORDERED_ACCESS         ( 1L << (6 + 4) )
typedef UINT DXGI_USAGE;

#include "rpc.h"
#include "rpcndr.h"

#include <Unknwnbase.h>

EXTERN_C const IID IID_IDXGIObject;

MIDL_INTERFACE ("aec22fb8-76f3-4639-9be0-28eb43a67a2e")
IDXGIObject : public IUnknown
{
public:
  virtual HRESULT STDMETHODCALLTYPE SetPrivateData (
    /* [annotation][in] */
    _In_  REFGUID Name,
    /* [in] */ UINT DataSize,
    /* [annotation][in] */
    _In_reads_bytes_ (DataSize)  const void *pData) = 0;

  virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface (
    /* [annotation][in] */
    _In_  REFGUID Name,
    /* [annotation][in] */
    _In_  const IUnknown *pUnknown) = 0;

  virtual HRESULT STDMETHODCALLTYPE GetPrivateData (
    /* [annotation][in] */
    _In_  REFGUID Name,
    /* [annotation][out][in] */
    _Inout_  UINT *pDataSize,
    /* [annotation][out] */
    _Out_writes_bytes_ (*pDataSize)  void *pData) = 0;

  virtual HRESULT STDMETHODCALLTYPE GetParent (
    /* [annotation][in] */
    _In_  REFIID riid,
    /* [annotation][retval][out] */
    _Out_  void **ppParent) = 0;

};

#include <dxgitype.h>

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


EXTERN_C const IID IID_IDXGIDeviceSubObject;


MIDL_INTERFACE ("3d3e0379-f9de-4d58-bb6c-18d62992f1a6")
IDXGIDeviceSubObject : public IDXGIObject
{
public:
  virtual HRESULT STDMETHODCALLTYPE GetDevice (
    /* [annotation][in] */
    _In_  REFIID riid,
    /* [annotation][retval][out] */
    _Out_  void **ppDevice) = 0;

};

typedef struct DXGI_SURFACE_DESC
{
  UINT Width;
  UINT Height;
  DXGI_FORMAT Format;
  DXGI_SAMPLE_DESC SampleDesc;
} 	DXGI_SURFACE_DESC;

EXTERN_C const IID IID_IDXGISurface;

MIDL_INTERFACE ("cafcb56c-6ac3-4889-bf47-9e23bbd260ec")
IDXGISurface : public IDXGIDeviceSubObject
{
public:
  virtual HRESULT STDMETHODCALLTYPE GetDesc (
    /* [annotation][out] */
    _Out_  DXGI_SURFACE_DESC *pDesc) = 0;

  virtual HRESULT STDMETHODCALLTYPE Map (
    /* [annotation][out] */
    _Out_  DXGI_MAPPED_RECT *pLockedRect,
    /* [in] */ UINT MapFlags) = 0;

  virtual HRESULT STDMETHODCALLTYPE Unmap (void) = 0;

};


typedef struct DXGI_OUTPUT_DESC
{
  WCHAR DeviceName [32];
  RECT DesktopCoordinates;
  BOOL AttachedToDesktop;
  DXGI_MODE_ROTATION Rotation;
  HMONITOR Monitor;
} 	DXGI_OUTPUT_DESC;

EXTERN_C const IID IID_IDXGIOutput;

MIDL_INTERFACE ("ae02eedb-c735-4690-8d52-5a8dc20213aa")
IDXGIOutput : public IDXGIObject
{
public:
  virtual HRESULT STDMETHODCALLTYPE GetDesc (
    /* [annotation][out] */
    _Out_  DXGI_OUTPUT_DESC *pDesc) = 0;

  virtual HRESULT STDMETHODCALLTYPE GetDisplayModeList (
    /* [in] */ DXGI_FORMAT EnumFormat,
    /* [in] */ UINT Flags,
    /* [annotation][out][in] */
    _Inout_  UINT *pNumModes,
    /* [annotation][out] */
    _Out_writes_to_opt_ (*pNumModes,*pNumModes)  DXGI_MODE_DESC *pDesc) = 0;

  virtual HRESULT STDMETHODCALLTYPE FindClosestMatchingMode (
    /* [annotation][in] */
    _In_  const DXGI_MODE_DESC *pModeToMatch,
    /* [annotation][out] */
    _Out_  DXGI_MODE_DESC *pClosestMatch,
    /* [annotation][in] */
    _In_opt_  IUnknown *pConcernedDevice) = 0;

  virtual HRESULT STDMETHODCALLTYPE WaitForVBlank (void) = 0;

  virtual HRESULT STDMETHODCALLTYPE TakeOwnership (
    /* [annotation][in] */
    _In_  IUnknown *pDevice,
    BOOL Exclusive) = 0;

  virtual void STDMETHODCALLTYPE ReleaseOwnership (void) = 0;

  virtual HRESULT STDMETHODCALLTYPE GetGammaControlCapabilities (
    /* [annotation][out] */
    _Out_  DXGI_GAMMA_CONTROL_CAPABILITIES *pGammaCaps) = 0;

  virtual HRESULT STDMETHODCALLTYPE SetGammaControl (
    /* [annotation][in] */
    _In_  const DXGI_GAMMA_CONTROL *pArray) = 0;

  virtual HRESULT STDMETHODCALLTYPE GetGammaControl (
    /* [annotation][out] */
    _Out_  DXGI_GAMMA_CONTROL *pArray) = 0;

  virtual HRESULT STDMETHODCALLTYPE SetDisplaySurface (
    /* [annotation][in] */
    _In_  IDXGISurface *pScanoutSurface) = 0;

  virtual HRESULT STDMETHODCALLTYPE GetDisplaySurfaceData (
    /* [annotation][in] */
    _In_  IDXGISurface *pDestination) = 0;

  virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics (
    /* [annotation][out] */
    _Out_  DXGI_FRAME_STATISTICS *pStats) = 0;

};



typedef struct DXGI_ADAPTER_DESC
{
  WCHAR Description [128];
  UINT VendorId;
  UINT DeviceId;
  UINT SubSysId;
  UINT Revision;
  SIZE_T DedicatedVideoMemory;
  SIZE_T DedicatedSystemMemory;
  SIZE_T SharedSystemMemory;
  LUID AdapterLuid;
} 	DXGI_ADAPTER_DESC;

EXTERN_C const IID IID_IDXGIAdapter;

MIDL_INTERFACE ("2411e7e1-12ac-4ccf-bd14-9798e8534dc0")
IDXGIAdapter : public IDXGIObject
{
public:
  virtual HRESULT STDMETHODCALLTYPE EnumOutputs (
    /* [in] */ UINT Output,
    /* [annotation][out][in] */
    _Out_  IDXGIOutput **ppOutput) = 0;

  virtual HRESULT STDMETHODCALLTYPE GetDesc (
    /* [annotation][out] */
    _Out_  DXGI_ADAPTER_DESC *pDesc) = 0;

  virtual HRESULT STDMETHODCALLTYPE CheckInterfaceSupport (
    /* [annotation][in] */
    _In_  REFGUID InterfaceName,
    /* [annotation][out] */
    _Out_  LARGE_INTEGER *pUMDVersion) = 0;

};

typedef
enum DXGI_SWAP_EFFECT
{
  DXGI_SWAP_EFFECT_DISCARD         = 0,
  DXGI_SWAP_EFFECT_SEQUENTIAL      = 1,
  DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL = 3,
  DXGI_SWAP_EFFECT_FLIP_DISCARD    = 4
} DXGI_SWAP_EFFECT;

typedef
enum DXGI_SWAP_CHAIN_FLAG
{
  DXGI_SWAP_CHAIN_FLAG_NONPREROTATED = 1,
  DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH = 2,
  DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE = 4,
  DXGI_SWAP_CHAIN_FLAG_RESTRICTED_CONTENT = 8,
  DXGI_SWAP_CHAIN_FLAG_RESTRICT_SHARED_RESOURCE_DRIVER = 16,
  DXGI_SWAP_CHAIN_FLAG_DISPLAY_ONLY = 32,
  DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT = 64,
  DXGI_SWAP_CHAIN_FLAG_FOREGROUND_LAYER = 128,
  DXGI_SWAP_CHAIN_FLAG_FULLSCREEN_VIDEO = 256,
  DXGI_SWAP_CHAIN_FLAG_YUV_VIDEO = 512
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
} DXGI_SWAP_CHAIN_DESC;

EXTERN_C const IID IID_IDXGISwapChain;


MIDL_INTERFACE ("310d36a0-d2e7-4c0a-aa04-6a9d23b8886a")
IDXGISwapChain : public IDXGIDeviceSubObject
{
public:
  virtual HRESULT STDMETHODCALLTYPE Present (
    /* [in] */ UINT SyncInterval,
    /* [in] */ UINT Flags) = 0;

  virtual HRESULT STDMETHODCALLTYPE GetBuffer (
    /* [in] */ UINT Buffer,
    /* [annotation][in] */
    _In_  REFIID riid,
    /* [annotation][out][in] */
    _Out_  void **ppSurface) = 0;

  virtual HRESULT STDMETHODCALLTYPE SetFullscreenState (
    /* [in] */ BOOL Fullscreen,
    /* [annotation][in] */
    _In_opt_  IDXGIOutput *pTarget) = 0;

  virtual HRESULT STDMETHODCALLTYPE GetFullscreenState (
    /* [annotation][out] */
    _Out_opt_  BOOL *pFullscreen,
    /* [annotation][out] */
    _Out_opt_  IDXGIOutput **ppTarget) = 0;

  virtual HRESULT STDMETHODCALLTYPE GetDesc (
    /* [annotation][out] */
    _Out_  DXGI_SWAP_CHAIN_DESC *pDesc) = 0;

  virtual HRESULT STDMETHODCALLTYPE ResizeBuffers (
    /* [in] */ UINT BufferCount,
    /* [in] */ UINT Width,
    /* [in] */ UINT Height,
    /* [in] */ DXGI_FORMAT NewFormat,
    /* [in] */ UINT SwapChainFlags) = 0;

  virtual HRESULT STDMETHODCALLTYPE ResizeTarget (
    /* [annotation][in] */
    _In_  const DXGI_MODE_DESC *pNewTargetParameters) = 0;

  virtual HRESULT STDMETHODCALLTYPE GetContainingOutput (
    /* [annotation][out] */
    _Out_  IDXGIOutput **ppOutput) = 0;

  virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics (
    /* [annotation][out] */
    _Out_  DXGI_FRAME_STATISTICS *pStats) = 0;

  virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount (
    /* [annotation][out] */
    _Out_  UINT *pLastPresentCount) = 0;

};

EXTERN_C const IID IID_IDXGIFactory;

MIDL_INTERFACE ("7b7166ec-21c7-44ae-b21a-c9ae321ae369")
IDXGIFactory : public IDXGIObject
{
public:
  virtual HRESULT STDMETHODCALLTYPE EnumAdapters (
    /* [in] */ UINT Adapter,
    /* [annotation][out] */
    _Out_  IDXGIAdapter **ppAdapter) = 0;

  virtual HRESULT STDMETHODCALLTYPE MakeWindowAssociation (
    HWND WindowHandle,
    UINT Flags) = 0;

  virtual HRESULT STDMETHODCALLTYPE GetWindowAssociation (
    /* [annotation][out] */
    _Out_  HWND *pWindowHandle) = 0;

  virtual HRESULT STDMETHODCALLTYPE CreateSwapChain (
    /* [annotation][in] */
    _In_  IUnknown *pDevice,
    /* [annotation][in] */
    _In_  DXGI_SWAP_CHAIN_DESC *pDesc,
    /* [annotation][out] */
    _Out_  IDXGISwapChain **ppSwapChain) = 0;

  virtual HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter (
    /* [in] */ HMODULE Module,
    /* [annotation][out] */
    _Out_  IDXGIAdapter **ppAdapter) = 0;

};

  typedef struct DXGI_ADAPTER_DESC1
  {
    WCHAR Description [128];
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

  EXTERN_C const IID IID_IDXGIAdapter1;

  MIDL_INTERFACE ("29038f61-3839-4626-91fd-086879011a05")
    IDXGIAdapter1 : public IDXGIAdapter
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetDesc1 (
      /* [annotation][out] */
      _Out_  DXGI_ADAPTER_DESC1 *pDesc) = 0;

  };

  EXTERN_C const IID IID_IDXGIFactory1;

  MIDL_INTERFACE ("770aae78-f26f-4dba-a829-253c83d1b387")
    IDXGIFactory1 : public IDXGIFactory
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE EnumAdapters1 (
      /* [in] */ UINT Adapter,
      /* [annotation][out] */
      _Out_  IDXGIAdapter1 **ppAdapter) = 0;

    virtual BOOL STDMETHODCALLTYPE IsCurrent (void) = 0;

  };

    typedef
  enum DXGI_ALPHA_MODE
  {
    DXGI_ALPHA_MODE_UNSPECIFIED = 0,
    DXGI_ALPHA_MODE_PREMULTIPLIED = 1,
    DXGI_ALPHA_MODE_STRAIGHT = 2,
    DXGI_ALPHA_MODE_IGNORE = 3,
    DXGI_ALPHA_MODE_FORCE_DWORD = 0xffffffff
  } 	DXGI_ALPHA_MODE;

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
    DXGI_SCALING_STRETCH = 0,
    DXGI_SCALING_NONE = 1,
    DXGI_SCALING_ASPECT_RATIO_STRETCH = 2
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

  EXTERN_C const IID IID_IDXGISwapChain1;

  MIDL_INTERFACE ("790a45f7-0d42-4876-983a-0a55cfe6f4aa")
    IDXGISwapChain1 : public IDXGISwapChain
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetDesc1 (
      /* [annotation][out] */
      _Out_  DXGI_SWAP_CHAIN_DESC1 *pDesc) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetFullscreenDesc (
      /* [annotation][out] */
      _Out_  DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetHwnd (
      /* [annotation][out] */
      _Out_  HWND *pHwnd) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetCoreWindow (
      /* [annotation][in] */
      _In_  REFIID refiid,
      /* [annotation][out] */
      _Out_  void **ppUnk) = 0;

    virtual HRESULT STDMETHODCALLTYPE Present1 (
      /* [in] */ UINT SyncInterval,
      /* [in] */ UINT PresentFlags,
      /* [annotation][in] */
      _In_  const DXGI_PRESENT_PARAMETERS *pPresentParameters) = 0;

    virtual BOOL STDMETHODCALLTYPE IsTemporaryMonoSupported (void) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetRestrictToOutput (
      /* [annotation][out] */
      _Out_  IDXGIOutput **ppRestrictToOutput) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor (
      /* [annotation][in] */
      _In_  const DXGI_RGBA *pColor) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetBackgroundColor (
      /* [annotation][out] */
      _Out_  DXGI_RGBA *pColor) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetRotation (
      /* [annotation][in] */
      _In_  DXGI_MODE_ROTATION Rotation) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetRotation (
      /* [annotation][out] */
      _Out_  DXGI_MODE_ROTATION *pRotation) = 0;

  };

  EXTERN_C const IID IID_IDXGIFactory2;

  MIDL_INTERFACE ("50c83a1c-e072-4c48-87b0-3630fa36a6d0")
    IDXGIFactory2 : public IDXGIFactory1
  {
  public:
    virtual BOOL STDMETHODCALLTYPE IsWindowedStereoEnabled (void) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForHwnd (
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

    virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForCoreWindow (
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

    virtual HRESULT STDMETHODCALLTYPE GetSharedResourceAdapterLuid (
      /* [annotation] */
      _In_  HANDLE hResource,
      /* [annotation] */
      _Out_  LUID *pLuid) = 0;

    virtual HRESULT STDMETHODCALLTYPE RegisterStereoStatusWindow (
      /* [annotation][in] */
      _In_  HWND WindowHandle,
      /* [annotation][in] */
      _In_  UINT wMsg,
      /* [annotation][out] */
      _Out_  DWORD *pdwCookie) = 0;

    virtual HRESULT STDMETHODCALLTYPE RegisterStereoStatusEvent (
      /* [annotation][in] */
      _In_  HANDLE hEvent,
      /* [annotation][out] */
      _Out_  DWORD *pdwCookie) = 0;

    virtual void STDMETHODCALLTYPE UnregisterStereoStatus (
      /* [annotation][in] */
      _In_  DWORD dwCookie) = 0;

    virtual HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusWindow (
      /* [annotation][in] */
      _In_  HWND WindowHandle,
      /* [annotation][in] */
      _In_  UINT wMsg,
      /* [annotation][out] */
      _Out_  DWORD *pdwCookie) = 0;

    virtual HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusEvent (
      /* [annotation][in] */
      _In_  HANDLE hEvent,
      /* [annotation][out] */
      _Out_  DWORD *pdwCookie) = 0;

    virtual void STDMETHODCALLTYPE UnregisterOcclusionStatus (
      /* [annotation][in] */
      _In_  DWORD dwCookie) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForComposition (
      /* [annotation][in] */
      _In_  IUnknown *pDevice,
      /* [annotation][in] */
      _In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
      /* [annotation][in] */
      _In_opt_  IDXGIOutput *pRestrictToOutput,
      /* [annotation][out] */
      _Outptr_  IDXGISwapChain1 **ppSwapChain) = 0;

  };

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

  EXTERN_C const IID IID_IDXGIAdapter2;

    MIDL_INTERFACE("0AA1AE0A-FA0E-4B84-8644-E05FF8E5ACB5")
    IDXGIAdapter2 : public IDXGIAdapter1
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDesc2( 
            /* [annotation][out] */ 
            _Out_  DXGI_ADAPTER_DESC2 *pDesc) = 0;

    };

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

    EXTERN_C const IID IID_IDXGIFactory3;

MIDL_INTERFACE("25483823-cd46-4c7d-86ca-47aa95b837bd")
IDXGIFactory3 : public IDXGIFactory2
{
public:
    virtual UINT STDMETHODCALLTYPE GetCreationFlags(void) = 0;
};

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
enum D3D_DRIVER_TYPE
    {
        D3D_DRIVER_TYPE_UNKNOWN	= 0,
        D3D_DRIVER_TYPE_HARDWARE	= ( D3D_DRIVER_TYPE_UNKNOWN + 1 ) ,
        D3D_DRIVER_TYPE_REFERENCE	= ( D3D_DRIVER_TYPE_HARDWARE + 1 ) ,
        D3D_DRIVER_TYPE_NULL	= ( D3D_DRIVER_TYPE_REFERENCE + 1 ) ,
        D3D_DRIVER_TYPE_SOFTWARE	= ( D3D_DRIVER_TYPE_NULL + 1 ) ,
        D3D_DRIVER_TYPE_WARP	= ( D3D_DRIVER_TYPE_SOFTWARE + 1 ) 
    } 	D3D_DRIVER_TYPE;

typedef 
enum D3D_FEATURE_LEVEL
    {
        D3D_FEATURE_LEVEL_9_1	= 0x9100,
        D3D_FEATURE_LEVEL_9_2	= 0x9200,
        D3D_FEATURE_LEVEL_9_3	= 0x9300,
        D3D_FEATURE_LEVEL_10_0	= 0xa000,
        D3D_FEATURE_LEVEL_10_1	= 0xa100,
        D3D_FEATURE_LEVEL_11_0	= 0xb000,
        D3D_FEATURE_LEVEL_11_1	= 0xb100,
        D3D_FEATURE_LEVEL_12_0  = 0xc000,
        D3D_FEATURE_LEVEL_12_1  = 0xc100
    } 	D3D_FEATURE_LEVEL;

EXTERN_C const IID IID_ID3D11Device;
    
    MIDL_INTERFACE("db6f6ddb-ac77-4e88-8253-819df9bbf140")
    ID3D11Device : public IUnknown
    {
    public:
#if 0
        virtual HRESULT STDMETHODCALLTYPE CreateBuffer( 
            /* [annotation] */ 
            _In_  const D3D11_BUFFER_DESC *pDesc,
            /* [annotation] */ 
            _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _Out_opt_  ID3D11Buffer **ppBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateTexture1D( 
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE1D_DESC *pDesc,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _Out_opt_  ID3D11Texture1D **ppTexture1D) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateTexture2D( 
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE2D_DESC *pDesc,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _Out_opt_  ID3D11Texture2D **ppTexture2D) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateTexture3D( 
            /* [annotation] */ 
            _In_  const D3D11_TEXTURE3D_DESC *pDesc,
            /* [annotation] */ 
            _In_reads_opt_(_Inexpressible_(pDesc->MipLevels))  const D3D11_SUBRESOURCE_DATA *pInitialData,
            /* [annotation] */ 
            _Out_opt_  ID3D11Texture3D **ppTexture3D) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateShaderResourceView( 
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _Out_opt_  ID3D11ShaderResourceView **ppSRView) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateUnorderedAccessView( 
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _Out_opt_  ID3D11UnorderedAccessView **ppUAView) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateRenderTargetView( 
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _Out_opt_  ID3D11RenderTargetView **ppRTView) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilView( 
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_opt_  const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,
            /* [annotation] */ 
            _Out_opt_  ID3D11DepthStencilView **ppDepthStencilView) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateInputLayout( 
            /* [annotation] */ 
            _In_reads_(NumElements)  const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs,
            /* [annotation] */ 
            _In_range_( 0, D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT )  UINT NumElements,
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecodeWithInputSignature,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _Out_opt_  ID3D11InputLayout **ppInputLayout) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateVertexShader( 
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _Out_opt_  ID3D11VertexShader **ppVertexShader) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateGeometryShader( 
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _Out_opt_  ID3D11GeometryShader **ppGeometryShader) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateGeometryShaderWithStreamOutput( 
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
            _Out_opt_  ID3D11GeometryShader **ppGeometryShader) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreatePixelShader( 
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _Out_opt_  ID3D11PixelShader **ppPixelShader) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateHullShader( 
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _Out_opt_  ID3D11HullShader **ppHullShader) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDomainShader( 
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _Out_opt_  ID3D11DomainShader **ppDomainShader) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateComputeShader( 
            /* [annotation] */ 
            _In_reads_(BytecodeLength)  const void *pShaderBytecode,
            /* [annotation] */ 
            _In_  SIZE_T BytecodeLength,
            /* [annotation] */ 
            _In_opt_  ID3D11ClassLinkage *pClassLinkage,
            /* [annotation] */ 
            _Out_opt_  ID3D11ComputeShader **ppComputeShader) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateClassLinkage( 
            /* [annotation] */ 
            _Out_  ID3D11ClassLinkage **ppLinkage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateBlendState( 
            /* [annotation] */ 
            _In_  const D3D11_BLEND_DESC *pBlendStateDesc,
            /* [annotation] */ 
            _Out_opt_  ID3D11BlendState **ppBlendState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilState( 
            /* [annotation] */ 
            _In_  const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc,
            /* [annotation] */ 
            _Out_opt_  ID3D11DepthStencilState **ppDepthStencilState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateRasterizerState( 
            /* [annotation] */ 
            _In_  const D3D11_RASTERIZER_DESC *pRasterizerDesc,
            /* [annotation] */ 
            _Out_opt_  ID3D11RasterizerState **ppRasterizerState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateSamplerState( 
            /* [annotation] */ 
            _In_  const D3D11_SAMPLER_DESC *pSamplerDesc,
            /* [annotation] */ 
            _Out_opt_  ID3D11SamplerState **ppSamplerState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateQuery( 
            /* [annotation] */ 
            _In_  const D3D11_QUERY_DESC *pQueryDesc,
            /* [annotation] */ 
            _Out_opt_  ID3D11Query **ppQuery) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreatePredicate( 
            /* [annotation] */ 
            _In_  const D3D11_QUERY_DESC *pPredicateDesc,
            /* [annotation] */ 
            _Out_opt_  ID3D11Predicate **ppPredicate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateCounter( 
            /* [annotation] */ 
            _In_  const D3D11_COUNTER_DESC *pCounterDesc,
            /* [annotation] */ 
            _Out_opt_  ID3D11Counter **ppCounter) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDeferredContext( 
            UINT ContextFlags,
            /* [annotation] */ 
            _Out_opt_  ID3D11DeviceContext **ppDeferredContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenSharedResource( 
            /* [annotation] */ 
            _In_  HANDLE hResource,
            /* [annotation] */ 
            _In_  REFIID ReturnedInterface,
            /* [annotation] */ 
            _Out_opt_  void **ppResource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CheckFormatSupport( 
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _Out_  UINT *pFormatSupport) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CheckMultisampleQualityLevels( 
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _In_  UINT SampleCount,
            /* [annotation] */ 
            _Out_  UINT *pNumQualityLevels) = 0;
        
        virtual void STDMETHODCALLTYPE CheckCounterInfo( 
            /* [annotation] */ 
            _Out_  D3D11_COUNTER_INFO *pCounterInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CheckCounter( 
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
            _Inout_opt_  UINT *pDescriptionLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CheckFeatureSupport( 
            D3D11_FEATURE Feature,
            /* [annotation] */ 
            _Out_writes_bytes_(FeatureSupportDataSize)  void *pFeatureSupportData,
            UINT FeatureSupportDataSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPrivateData( 
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_(*pDataSize)  void *pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPrivateData( 
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_(DataSize)  const void *pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface( 
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData) = 0;
        
        virtual D3D_FEATURE_LEVEL STDMETHODCALLTYPE GetFeatureLevel( void) = 0;
        
        virtual UINT STDMETHODCALLTYPE GetCreationFlags( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDeviceRemovedReason( void) = 0;
        
        virtual void STDMETHODCALLTYPE GetImmediateContext( 
            /* [annotation] */ 
            _Out_  ID3D11DeviceContext **ppImmediateContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetExceptionMode( 
            UINT RaiseFlags) = 0;
        
        virtual UINT STDMETHODCALLTYPE GetExceptionMode( void) = 0;
#endif

    };

    EXTERN_C const IID IID_ID3D11Device1;
    
    MIDL_INTERFACE("a04bfb29-08ef-43d6-a49c-a9bdbdcbe686")
    ID3D11Device1 : public ID3D11Device
    {
    public:
#if 0
        virtual void STDMETHODCALLTYPE GetImmediateContext1( 
            /* [annotation] */ 
            _Out_  ID3D11DeviceContext1 **ppImmediateContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDeferredContext1( 
            UINT ContextFlags,
            /* [annotation] */ 
            _Out_opt_  ID3D11DeviceContext1 **ppDeferredContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateBlendState1( 
            /* [annotation] */ 
            _In_  const D3D11_BLEND_DESC1 *pBlendStateDesc,
            /* [annotation] */ 
            _Out_opt_  ID3D11BlendState1 **ppBlendState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateRasterizerState1( 
            /* [annotation] */ 
            _In_  const D3D11_RASTERIZER_DESC1 *pRasterizerDesc,
            /* [annotation] */ 
            _Out_opt_  ID3D11RasterizerState1 **ppRasterizerState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDeviceContextState( 
            UINT Flags,
            /* [annotation] */ 
            _In_reads_( FeatureLevels )  const D3D_FEATURE_LEVEL *pFeatureLevels,
            UINT FeatureLevels,
            UINT SDKVersion,
            REFIID EmulatedInterface,
            /* [annotation] */ 
            _Out_opt_  D3D_FEATURE_LEVEL *pChosenFeatureLevel,
            /* [annotation] */ 
            _Out_opt_  ID3DDeviceContextState **ppContextState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenSharedResource1( 
            /* [annotation] */ 
            _In_  HANDLE hResource,
            /* [annotation] */ 
            _In_  REFIID returnedInterface,
            /* [annotation] */ 
            _Out_  void **ppResource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenSharedResourceByName( 
            /* [annotation] */ 
            _In_  LPCWSTR lpName,
            /* [annotation] */ 
            _In_  DWORD dwDesiredAccess,
            /* [annotation] */ 
            _In_  REFIID returnedInterface,
            /* [annotation] */ 
            _Out_  void **ppResource) = 0;
#endif  
    };

    EXTERN_C const IID IID_ID3D11Device2;
    
    MIDL_INTERFACE("9d06dffa-d1e5-4d07-83a8-1bb123f2f841")
    ID3D11Device2 : public ID3D11Device1
    {
    public:
#if 0
        virtual void STDMETHODCALLTYPE GetImmediateContext2( 
            /* [annotation] */ 
            _Out_  ID3D11DeviceContext2 **ppImmediateContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDeferredContext2( 
            UINT ContextFlags,
            /* [annotation] */ 
            _Out_opt_  ID3D11DeviceContext2 **ppDeferredContext) = 0;
        
        virtual void STDMETHODCALLTYPE GetResourceTiling( 
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
            _Out_writes_(*pNumSubresourceTilings)  D3D11_SUBRESOURCE_TILING *pSubresourceTilingsForNonPackedMips) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CheckMultisampleQualityLevels1( 
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _In_  UINT SampleCount,
            /* [annotation] */ 
            _In_  UINT Flags,
            /* [annotation] */ 
            _Out_  UINT *pNumQualityLevels) = 0;
#endif  
    };


    EXTERN_C const IID IID_ID3D11DeviceChild;

    MIDL_INTERFACE("1841e5c8-16b0-489b-bcc8-44cfb0d5deae")
    ID3D11DeviceChild : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE GetDevice( 
            /* [annotation] */ 
            _Out_  ID3D11Device **ppDevice) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPrivateData( 
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPrivateData( 
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface( 
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData) = 0;
        
    };

    EXTERN_C const IID IID_ID3D11DeviceContext;

    MIDL_INTERFACE("c0bfa96c-e089-44fb-8eaf-26f8796190da")
    ID3D11DeviceContext : public ID3D11DeviceChild
    {
    public:
#if 0
        virtual void STDMETHODCALLTYPE VSSetConstantBuffers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers) = 0;
        
        virtual void STDMETHODCALLTYPE PSSetShaderResources( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews) = 0;
        
        virtual void STDMETHODCALLTYPE PSSetShader( 
            /* [annotation] */ 
            _In_opt_  ID3D11PixelShader *pPixelShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances) = 0;
        
        virtual void STDMETHODCALLTYPE PSSetSamplers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers) = 0;
        
        virtual void STDMETHODCALLTYPE VSSetShader( 
            /* [annotation] */ 
            _In_opt_  ID3D11VertexShader *pVertexShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances) = 0;
        
        virtual void STDMETHODCALLTYPE DrawIndexed( 
            /* [annotation] */ 
            _In_  UINT IndexCount,
            /* [annotation] */ 
            _In_  UINT StartIndexLocation,
            /* [annotation] */ 
            _In_  INT BaseVertexLocation) = 0;
        
        virtual void STDMETHODCALLTYPE Draw( 
            /* [annotation] */ 
            _In_  UINT VertexCount,
            /* [annotation] */ 
            _In_  UINT StartVertexLocation) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Map( 
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_  UINT Subresource,
            /* [annotation] */ 
            _In_  D3D11_MAP MapType,
            /* [annotation] */ 
            _In_  UINT MapFlags,
            /* [annotation] */ 
            _Out_  D3D11_MAPPED_SUBRESOURCE *pMappedResource) = 0;
        
        virtual void STDMETHODCALLTYPE Unmap( 
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            /* [annotation] */ 
            _In_  UINT Subresource) = 0;
        
        virtual void STDMETHODCALLTYPE PSSetConstantBuffers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers) = 0;
        
        virtual void STDMETHODCALLTYPE IASetInputLayout( 
            /* [annotation] */ 
            _In_opt_  ID3D11InputLayout *pInputLayout) = 0;
        
        virtual void STDMETHODCALLTYPE IASetVertexBuffers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppVertexBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pStrides,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pOffsets) = 0;
        
        virtual void STDMETHODCALLTYPE IASetIndexBuffer( 
            /* [annotation] */ 
            _In_opt_  ID3D11Buffer *pIndexBuffer,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _In_  UINT Offset) = 0;
        
        virtual void STDMETHODCALLTYPE DrawIndexedInstanced( 
            /* [annotation] */ 
            _In_  UINT IndexCountPerInstance,
            /* [annotation] */ 
            _In_  UINT InstanceCount,
            /* [annotation] */ 
            _In_  UINT StartIndexLocation,
            /* [annotation] */ 
            _In_  INT BaseVertexLocation,
            /* [annotation] */ 
            _In_  UINT StartInstanceLocation) = 0;
        
        virtual void STDMETHODCALLTYPE DrawInstanced( 
            /* [annotation] */ 
            _In_  UINT VertexCountPerInstance,
            /* [annotation] */ 
            _In_  UINT InstanceCount,
            /* [annotation] */ 
            _In_  UINT StartVertexLocation,
            /* [annotation] */ 
            _In_  UINT StartInstanceLocation) = 0;
        
        virtual void STDMETHODCALLTYPE GSSetConstantBuffers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers) = 0;
        
        virtual void STDMETHODCALLTYPE GSSetShader( 
            /* [annotation] */ 
            _In_opt_  ID3D11GeometryShader *pShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances) = 0;
        
        virtual void STDMETHODCALLTYPE IASetPrimitiveTopology( 
            /* [annotation] */ 
            _In_  D3D11_PRIMITIVE_TOPOLOGY Topology) = 0;
        
        virtual void STDMETHODCALLTYPE VSSetShaderResources( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews) = 0;
        
        virtual void STDMETHODCALLTYPE VSSetSamplers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers) = 0;
        
        virtual void STDMETHODCALLTYPE Begin( 
            /* [annotation] */ 
            _In_  ID3D11Asynchronous *pAsync) = 0;
        
        virtual void STDMETHODCALLTYPE End( 
            /* [annotation] */ 
            _In_  ID3D11Asynchronous *pAsync) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetData( 
            /* [annotation] */ 
            _In_  ID3D11Asynchronous *pAsync,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( DataSize )  void *pData,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_  UINT GetDataFlags) = 0;
        
        virtual void STDMETHODCALLTYPE SetPredication( 
            /* [annotation] */ 
            _In_opt_  ID3D11Predicate *pPredicate,
            /* [annotation] */ 
            _In_  BOOL PredicateValue) = 0;
        
        virtual void STDMETHODCALLTYPE GSSetShaderResources( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews) = 0;
        
        virtual void STDMETHODCALLTYPE GSSetSamplers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers) = 0;
        
        virtual void STDMETHODCALLTYPE OMSetRenderTargets( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11RenderTargetView *const *ppRenderTargetViews,
            /* [annotation] */ 
            _In_opt_  ID3D11DepthStencilView *pDepthStencilView) = 0;
        
        virtual void STDMETHODCALLTYPE OMSetRenderTargetsAndUnorderedAccessViews( 
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
            _In_reads_opt_(NumUAVs)  const UINT *pUAVInitialCounts) = 0;
        
        virtual void STDMETHODCALLTYPE OMSetBlendState( 
            /* [annotation] */ 
            _In_opt_  ID3D11BlendState *pBlendState,
            /* [annotation] */ 
            _In_opt_  const FLOAT BlendFactor[ 4 ],
            /* [annotation] */ 
            _In_  UINT SampleMask) = 0;
        
        virtual void STDMETHODCALLTYPE OMSetDepthStencilState( 
            /* [annotation] */ 
            _In_opt_  ID3D11DepthStencilState *pDepthStencilState,
            /* [annotation] */ 
            _In_  UINT StencilRef) = 0;
        
        virtual void STDMETHODCALLTYPE SOSetTargets( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_SO_BUFFER_SLOT_COUNT)  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppSOTargets,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pOffsets) = 0;
        
        virtual void STDMETHODCALLTYPE DrawAuto( void) = 0;
        
        virtual void STDMETHODCALLTYPE DrawIndexedInstancedIndirect( 
            /* [annotation] */ 
            _In_  ID3D11Buffer *pBufferForArgs,
            /* [annotation] */ 
            _In_  UINT AlignedByteOffsetForArgs) = 0;
        
        virtual void STDMETHODCALLTYPE DrawInstancedIndirect( 
            /* [annotation] */ 
            _In_  ID3D11Buffer *pBufferForArgs,
            /* [annotation] */ 
            _In_  UINT AlignedByteOffsetForArgs) = 0;
        
        virtual void STDMETHODCALLTYPE Dispatch( 
            /* [annotation] */ 
            _In_  UINT ThreadGroupCountX,
            /* [annotation] */ 
            _In_  UINT ThreadGroupCountY,
            /* [annotation] */ 
            _In_  UINT ThreadGroupCountZ) = 0;
        
        virtual void STDMETHODCALLTYPE DispatchIndirect( 
            /* [annotation] */ 
            _In_  ID3D11Buffer *pBufferForArgs,
            /* [annotation] */ 
            _In_  UINT AlignedByteOffsetForArgs) = 0;
        
        virtual void STDMETHODCALLTYPE RSSetState( 
            /* [annotation] */ 
            _In_opt_  ID3D11RasterizerState *pRasterizerState) = 0;
        
        virtual void STDMETHODCALLTYPE RSSetViewports( 
            /* [annotation] */ 
            _In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumViewports,
            /* [annotation] */ 
            _In_reads_opt_(NumViewports)  const D3D11_VIEWPORT *pViewports) = 0;
        
        virtual void STDMETHODCALLTYPE RSSetScissorRects( 
            /* [annotation] */ 
            _In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumRects,
            /* [annotation] */ 
            _In_reads_opt_(NumRects)  const D3D11_RECT *pRects) = 0;
        
        virtual void STDMETHODCALLTYPE CopySubresourceRegion( 
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
            _In_opt_  const D3D11_BOX *pSrcBox) = 0;
        
        virtual void STDMETHODCALLTYPE CopyResource( 
            /* [annotation] */ 
            _In_  ID3D11Resource *pDstResource,
            /* [annotation] */ 
            _In_  ID3D11Resource *pSrcResource) = 0;
        
        virtual void STDMETHODCALLTYPE UpdateSubresource( 
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
        
        virtual void STDMETHODCALLTYPE CopyStructureCount( 
            /* [annotation] */ 
            _In_  ID3D11Buffer *pDstBuffer,
            /* [annotation] */ 
            _In_  UINT DstAlignedByteOffset,
            /* [annotation] */ 
            _In_  ID3D11UnorderedAccessView *pSrcView) = 0;
        
        virtual void STDMETHODCALLTYPE ClearRenderTargetView( 
            /* [annotation] */ 
            _In_  ID3D11RenderTargetView *pRenderTargetView,
            /* [annotation] */ 
            _In_  const FLOAT ColorRGBA[ 4 ]) = 0;
        
        virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewUint( 
            /* [annotation] */ 
            _In_  ID3D11UnorderedAccessView *pUnorderedAccessView,
            /* [annotation] */ 
            _In_  const UINT Values[ 4 ]) = 0;
        
        virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewFloat( 
            /* [annotation] */ 
            _In_  ID3D11UnorderedAccessView *pUnorderedAccessView,
            /* [annotation] */ 
            _In_  const FLOAT Values[ 4 ]) = 0;
        
        virtual void STDMETHODCALLTYPE ClearDepthStencilView( 
            /* [annotation] */ 
            _In_  ID3D11DepthStencilView *pDepthStencilView,
            /* [annotation] */ 
            _In_  UINT ClearFlags,
            /* [annotation] */ 
            _In_  FLOAT Depth,
            /* [annotation] */ 
            _In_  UINT8 Stencil) = 0;
        
        virtual void STDMETHODCALLTYPE GenerateMips( 
            /* [annotation] */ 
            _In_  ID3D11ShaderResourceView *pShaderResourceView) = 0;
        
        virtual void STDMETHODCALLTYPE SetResourceMinLOD( 
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource,
            FLOAT MinLOD) = 0;
        
        virtual FLOAT STDMETHODCALLTYPE GetResourceMinLOD( 
            /* [annotation] */ 
            _In_  ID3D11Resource *pResource) = 0;
        
        virtual void STDMETHODCALLTYPE ResolveSubresource( 
            /* [annotation] */ 
            _In_  ID3D11Resource *pDstResource,
            /* [annotation] */ 
            _In_  UINT DstSubresource,
            /* [annotation] */ 
            _In_  ID3D11Resource *pSrcResource,
            /* [annotation] */ 
            _In_  UINT SrcSubresource,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format) = 0;
        
        virtual void STDMETHODCALLTYPE ExecuteCommandList( 
            /* [annotation] */ 
            _In_  ID3D11CommandList *pCommandList,
            BOOL RestoreContextState) = 0;
        
        virtual void STDMETHODCALLTYPE HSSetShaderResources( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews) = 0;
        
        virtual void STDMETHODCALLTYPE HSSetShader( 
            /* [annotation] */ 
            _In_opt_  ID3D11HullShader *pHullShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances) = 0;
        
        virtual void STDMETHODCALLTYPE HSSetSamplers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers) = 0;
        
        virtual void STDMETHODCALLTYPE HSSetConstantBuffers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers) = 0;
        
        virtual void STDMETHODCALLTYPE DSSetShaderResources( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews) = 0;
        
        virtual void STDMETHODCALLTYPE DSSetShader( 
            /* [annotation] */ 
            _In_opt_  ID3D11DomainShader *pDomainShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances) = 0;
        
        virtual void STDMETHODCALLTYPE DSSetSamplers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers) = 0;
        
        virtual void STDMETHODCALLTYPE DSSetConstantBuffers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers) = 0;
        
        virtual void STDMETHODCALLTYPE CSSetShaderResources( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews) = 0;
        
        virtual void STDMETHODCALLTYPE CSSetUnorderedAccessViews( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - StartSlot )  UINT NumUAVs,
            /* [annotation] */ 
            _In_reads_opt_(NumUAVs)  ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
            /* [annotation] */ 
            _In_reads_opt_(NumUAVs)  const UINT *pUAVInitialCounts) = 0;
        
        virtual void STDMETHODCALLTYPE CSSetShader( 
            /* [annotation] */ 
            _In_opt_  ID3D11ComputeShader *pComputeShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances) = 0;
        
        virtual void STDMETHODCALLTYPE CSSetSamplers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers) = 0;
        
        virtual void STDMETHODCALLTYPE CSSetConstantBuffers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers) = 0;
        
        virtual void STDMETHODCALLTYPE VSGetConstantBuffers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers) = 0;
        
        virtual void STDMETHODCALLTYPE PSGetShaderResources( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews) = 0;
        
        virtual void STDMETHODCALLTYPE PSGetShader( 
            /* [annotation] */ 
            _Out_  ID3D11PixelShader **ppPixelShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances) = 0;
        
        virtual void STDMETHODCALLTYPE PSGetSamplers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers) = 0;
        
        virtual void STDMETHODCALLTYPE VSGetShader( 
            /* [annotation] */ 
            _Out_  ID3D11VertexShader **ppVertexShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances) = 0;
        
        virtual void STDMETHODCALLTYPE PSGetConstantBuffers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers) = 0;
        
        virtual void STDMETHODCALLTYPE IAGetInputLayout( 
            /* [annotation] */ 
            _Out_  ID3D11InputLayout **ppInputLayout) = 0;
        
        virtual void STDMETHODCALLTYPE IAGetVertexBuffers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppVertexBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pStrides,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  UINT *pOffsets) = 0;
        
        virtual void STDMETHODCALLTYPE IAGetIndexBuffer( 
            /* [annotation] */ 
            _Out_opt_  ID3D11Buffer **pIndexBuffer,
            /* [annotation] */ 
            _Out_opt_  DXGI_FORMAT *Format,
            /* [annotation] */ 
            _Out_opt_  UINT *Offset) = 0;
        
        virtual void STDMETHODCALLTYPE GSGetConstantBuffers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers) = 0;
        
        virtual void STDMETHODCALLTYPE GSGetShader( 
            /* [annotation] */ 
            _Out_  ID3D11GeometryShader **ppGeometryShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances) = 0;
        
        virtual void STDMETHODCALLTYPE IAGetPrimitiveTopology( 
            /* [annotation] */ 
            _Out_  D3D11_PRIMITIVE_TOPOLOGY *pTopology) = 0;
        
        virtual void STDMETHODCALLTYPE VSGetShaderResources( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews) = 0;
        
        virtual void STDMETHODCALLTYPE VSGetSamplers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers) = 0;
        
        virtual void STDMETHODCALLTYPE GetPredication( 
            /* [annotation] */ 
            _Out_opt_  ID3D11Predicate **ppPredicate,
            /* [annotation] */ 
            _Out_opt_  BOOL *pPredicateValue) = 0;
        
        virtual void STDMETHODCALLTYPE GSGetShaderResources( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews) = 0;
        
        virtual void STDMETHODCALLTYPE GSGetSamplers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers) = 0;
        
        virtual void STDMETHODCALLTYPE OMGetRenderTargets( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11RenderTargetView **ppRenderTargetViews,
            /* [annotation] */ 
            _Out_opt_  ID3D11DepthStencilView **ppDepthStencilView) = 0;
        
        virtual void STDMETHODCALLTYPE OMGetRenderTargetsAndUnorderedAccessViews( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT )  UINT NumRTVs,
            /* [annotation] */ 
            _Out_writes_opt_(NumRTVs)  ID3D11RenderTargetView **ppRenderTargetViews,
            /* [annotation] */ 
            _Out_opt_  ID3D11DepthStencilView **ppDepthStencilView,
            /* [annotation] */ 
            _In_range_( 0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1 )  UINT UAVStartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_PS_CS_UAV_REGISTER_COUNT - UAVStartSlot )  UINT NumUAVs,
            /* [annotation] */ 
            _Out_writes_opt_(NumUAVs)  ID3D11UnorderedAccessView **ppUnorderedAccessViews) = 0;
        
        virtual void STDMETHODCALLTYPE OMGetBlendState( 
            /* [annotation] */ 
            _Out_opt_  ID3D11BlendState **ppBlendState,
            /* [annotation] */ 
            _Out_opt_  FLOAT BlendFactor[ 4 ],
            /* [annotation] */ 
            _Out_opt_  UINT *pSampleMask) = 0;
        
        virtual void STDMETHODCALLTYPE OMGetDepthStencilState( 
            /* [annotation] */ 
            _Out_opt_  ID3D11DepthStencilState **ppDepthStencilState,
            /* [annotation] */ 
            _Out_opt_  UINT *pStencilRef) = 0;
        
        virtual void STDMETHODCALLTYPE SOGetTargets( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_SO_BUFFER_SLOT_COUNT )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppSOTargets) = 0;
        
        virtual void STDMETHODCALLTYPE RSGetState( 
            /* [annotation] */ 
            _Out_  ID3D11RasterizerState **ppRasterizerState) = 0;
        
        virtual void STDMETHODCALLTYPE RSGetViewports( 
            /* [annotation] */ 
            _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumViewports,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumViewports)  D3D11_VIEWPORT *pViewports) = 0;
        
        virtual void STDMETHODCALLTYPE RSGetScissorRects( 
            /* [annotation] */ 
            _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumRects,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumRects)  D3D11_RECT *pRects) = 0;
        
        virtual void STDMETHODCALLTYPE HSGetShaderResources( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews) = 0;
        
        virtual void STDMETHODCALLTYPE HSGetShader( 
            /* [annotation] */ 
            _Out_  ID3D11HullShader **ppHullShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances) = 0;
        
        virtual void STDMETHODCALLTYPE HSGetSamplers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers) = 0;
        
        virtual void STDMETHODCALLTYPE HSGetConstantBuffers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers) = 0;
        
        virtual void STDMETHODCALLTYPE DSGetShaderResources( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews) = 0;
        
        virtual void STDMETHODCALLTYPE DSGetShader( 
            /* [annotation] */ 
            _Out_  ID3D11DomainShader **ppDomainShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances) = 0;
        
        virtual void STDMETHODCALLTYPE DSGetSamplers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers) = 0;
        
        virtual void STDMETHODCALLTYPE DSGetConstantBuffers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers) = 0;
        
        virtual void STDMETHODCALLTYPE CSGetShaderResources( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews) = 0;
        
        virtual void STDMETHODCALLTYPE CSGetUnorderedAccessViews( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_PS_CS_UAV_REGISTER_COUNT - StartSlot )  UINT NumUAVs,
            /* [annotation] */ 
            _Out_writes_opt_(NumUAVs)  ID3D11UnorderedAccessView **ppUnorderedAccessViews) = 0;
        
        virtual void STDMETHODCALLTYPE CSGetShader( 
            /* [annotation] */ 
            _Out_  ID3D11ComputeShader **ppComputeShader,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
            /* [annotation] */ 
            _Inout_opt_  UINT *pNumClassInstances) = 0;
        
        virtual void STDMETHODCALLTYPE CSGetSamplers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers) = 0;
        
        virtual void STDMETHODCALLTYPE CSGetConstantBuffers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers) = 0;
        
        virtual void STDMETHODCALLTYPE ClearState( void) = 0;
        
        virtual void STDMETHODCALLTYPE Flush( void) = 0;
        
        virtual D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE GetType( void) = 0;
        
        virtual UINT STDMETHODCALLTYPE GetContextFlags( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FinishCommandList( 
            BOOL RestoreDeferredContextState,
            /* [annotation] */ 
            _Out_opt_  ID3D11CommandList **ppCommandList) = 0;
#endif
    };

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

    EXTERN_C const IID IID_IDXGIDevice;
    
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

#if 0
    EXTERN_C const IID IID_IDXGISwapChain2;

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
#endif
#else
# include "dxgi_interfaces.h"
#endif