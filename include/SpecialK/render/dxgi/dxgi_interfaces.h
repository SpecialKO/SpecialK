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

#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>
#include <dxgi1_5.h>
#if __has_include (<dxgi1_6.h>)
#include <dxgi1_6.h>
#endif
#include <SpecialK/com_util.h>

interface IDXGISwapChain;
interface IDXGISwapChain1;

static const GUID SKID_D3D11_ResourceFormatOverride =
{ 0x96f1e9dd, 0x6cba, 0x42b2, { 0x90, 0x3c, 0xf1, 0x18, 0x89, 0x6f, 0x0, 0x5f } };

static const GUID SKID_D3D11_CachedBltCopySrc = 
{ 0x5f251889, 0x827a, 0x4a5c, { 0xb6, 0x34, 0xdf, 0x8c, 0x8c, 0xfe, 0x3c, 0x9f } };

static const GUID SKID_D3D11_CachedBltCopyDst = 
{ 0x5ae5e6e2, 0xf4d9, 0x4797, { 0xaa, 0x72, 0x44, 0x70, 0x6a, 0x6a, 0xda, 0x9e } };

static const GUID SKID_D3D11_SurrogateMultisampleResolveBuffer = 
{ 0x637d58a6, 0x356b, 0x4e90, { 0x95, 0x3a, 0x23, 0xc6, 0xb0, 0xd1, 0xb6, 0xd3 } };

static const GUID SKID_DXGI_WrappedSwapChain =
{ 0x42453612, 0xf56, 0x46ce, { 0xb5, 0xfb, 0x4a, 0xee, 0xcc, 0xd8, 0x35, 0xcf } };

static const GUID SKID_DXGI_SwapChainBackbufferFormat =
{ 0x8ea4b32a, 0x4150, 0xc421, { 0xea, 0xd, 0x88, 0xd3, 0xe9, 0x89, 0xa2, 0xf2 } };

// For D3D11 Flip Model Upgrade Only
static const GUID SKID_DXGI_SwapChainProxyBackbuffer_D3D11 =
{ 0x1313ea1f, 0xc3fe, 0x4588, { 0xaf, 0x89, 0xe8, 0xe, 0x13, 0x40, 0x2d, 0x69 } };

// For D3D11 Flip Model Upgrade Only
static const GUID SKID_DXGI_SwapChainRealBackbuffer_D3D11 =
{ 0x7ae4681, 0x102f, 0x4015, { 0xa9, 0x18, 0x61, 0x62, 0xa2, 0x85, 0x3a, 0x26 } };

static const GUID SKID_DXGI_SwapChainSkipBackbufferCopy_D3D11 =
{ 0x712afc45, 0xa320, 0x4fcf, { 0x9b, 0xce, 0xe9, 0x7f, 0x9c, 0x55, 0x21, 0xa1 } };

static const GUID SKID_DXGI_SwapChain_StateCache =
{ 0x6c08d7fe, 0xbfb4, 0x40c3, { 0xbc, 0x57, 0x6f, 0x2b, 0x4b, 0x42, 0x1e, 0x7f } };

static const GUID SKID_DXGI_VK_InteropSwapChain =
{ 0x22c246b, 0x322a, 0x4694, { 0x87, 0xaa, 0x7b, 0xcf, 0xfb, 0xac, 0x5e, 0xce } };

static const GUID SKID_DXVK_InteropFactory =
{ 0x4c5e1b0d, 0xb0c8, 0x4131, { 0xbf, 0xd8, 0x9b, 0x24, 0x76, 0xf7, 0xf4, 0x08 } };

enum
{
  SK_DXGI_VK_INTEROP_TYPE_NONE,
  SK_DXGI_VK_INTEROP_TYPE_IK,  ///< Vulkan-IK
  SK_DXGI_VK_INTEROP_TYPE_NV,  ///< NVIDIA's DXGI Layer
  SK_DXGI_VK_INTEROP_TYPE_AMD, ///< AMD's DXGI Layer
  SK_DXGI_VK_INTEROP_TYPE_DXVK ///< DXVK; not a layer
};

extern volatile LONG SK_DXGI_LiveWrappedSwapChains;
extern volatile LONG SK_DXGI_LiveWrappedSwapChain1s;



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

typedef HRESULT (STDMETHODCALLTYPE *ResizeBuffers1_pfn)(
                                             IDXGISwapChain3 *This,
                            /* [in] */       UINT             BufferCount,
                            /* [in] */       UINT             Width,
                            /* [in] */       UINT             Height,
                            /* [in] */       DXGI_FORMAT      NewFormat,
                            /* [in] */       UINT             SwapChainFlags,
                            /* [in] */ const UINT            *pCreationNodeMask,
                            /* [in] */       IUnknown* const *ppPresentQueue);

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

#include <dxgi.h>
//#define DXGI_PRESENT_ALLOW_TEARING          0x00000200UL
//#define DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING  2048

#if !__has_include (<dxgi1_6.h>)
#ifdef __cplusplus
extern "C"{
#endif


/* interface __MIDL_itf_dxgi1_6_0000_0000 */
/* [local] */

// Copyright (c) Microsoft Corporation.  All Rights Reserved
/*#include <winapifamily.h>*/
/*#pragma region App Family*/
/*#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)*/
typedef
enum DXGI_ADAPTER_FLAG3
{
  DXGI_ADAPTER_FLAG3_NONE	                        = 0,
  DXGI_ADAPTER_FLAG3_REMOTE	                      = 1,
  DXGI_ADAPTER_FLAG3_SOFTWARE	                    = 2,
  DXGI_ADAPTER_FLAG3_ACG_COMPATIBLE	              = 4,
  DXGI_ADAPTER_FLAG3_SUPPORT_MONITORED_FENCES	    = 8,
  DXGI_ADAPTER_FLAG3_SUPPORT_NON_MONITORED_FENCES	= 0x10,
  DXGI_ADAPTER_FLAG3_KEYED_MUTEX_CONFORMANCE      = 0x20,
  DXGI_ADAPTER_FLAG3_FORCE_DWORD                  = 0xffffffff
}   DXGI_ADAPTER_FLAG3;

DEFINE_ENUM_FLAG_OPERATORS (DXGI_ADAPTER_FLAG3);

typedef struct DXGI_ADAPTER_DESC3
{
  WCHAR                                Description [128];
  UINT                                 VendorId;
  UINT                                 DeviceId;
  UINT                                 SubSysId;
  UINT                                 Revision;
  SIZE_T                               DedicatedVideoMemory;
  SIZE_T                               DedicatedSystemMemory;
  SIZE_T                               SharedSystemMemory;
  LUID                                 AdapterLuid;
  DXGI_ADAPTER_FLAG3                   Flags;
  DXGI_GRAPHICS_PREEMPTION_GRANULARITY GraphicsPreemptionGranularity;
  DXGI_COMPUTE_PREEMPTION_GRANULARITY  ComputePreemptionGranularity;
}   DXGI_ADAPTER_DESC3;



extern RPC_IF_HANDLE __MIDL_itf_dxgi1_6_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_6_0000_0000_v0_0_s_ifspec;

#ifndef __IDXGIAdapter4_INTERFACE_DEFINED__
#define __IDXGIAdapter4_INTERFACE_DEFINED__

/* interface IDXGIAdapter4 */
/* [unique][local][uuid][object] */


EXTERN_C const IID IID_IDXGIAdapter4;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("3c8d99d1-4fbf-4181-a82c-af66bf7bd24e")
    IDXGIAdapter4 : public IDXGIAdapter3
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDesc3(
            /* [annotation][out] */
            _Out_  DXGI_ADAPTER_DESC3 *pDesc) = 0;

    };


#else 	/* C style interface */

    typedef struct IDXGIAdapter4Vtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
            IDXGIAdapter4 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */
            _COM_Outptr_  void **ppvObject);

        ULONG ( STDMETHODCALLTYPE *AddRef )(
            IDXGIAdapter4 * This);

        ULONG ( STDMETHODCALLTYPE *Release )(
            IDXGIAdapter4 * This);

        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )(
            IDXGIAdapter4 * This,
            /* [annotation][in] */
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */
            _In_reads_bytes_(DataSize)  const void *pData);

        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )(
            IDXGIAdapter4 * This,
            /* [annotation][in] */
            _In_  REFGUID Name,
            /* [annotation][in] */
            _In_opt_  const IUnknown *pUnknown);

        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )(
            IDXGIAdapter4 * This,
            /* [annotation][in] */
            _In_  REFGUID Name,
            /* [annotation][out][in] */
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */
            _Out_writes_bytes_(*pDataSize)  void *pData);

        HRESULT ( STDMETHODCALLTYPE *GetParent )(
            IDXGIAdapter4 * This,
            /* [annotation][in] */
            _In_  REFIID riid,
            /* [annotation][retval][out] */
            _COM_Outptr_  void **ppParent);

        HRESULT ( STDMETHODCALLTYPE *EnumOutputs )(
            IDXGIAdapter4 * This,
            /* [in] */ UINT Output,
            /* [annotation][out][in] */
            _COM_Outptr_  IDXGIOutput **ppOutput);

        HRESULT ( STDMETHODCALLTYPE *GetDesc )(
            IDXGIAdapter4 * This,
            /* [annotation][out] */
            _Out_  DXGI_ADAPTER_DESC *pDesc);

        HRESULT ( STDMETHODCALLTYPE *CheckInterfaceSupport )(
            IDXGIAdapter4 * This,
            /* [annotation][in] */
            _In_  REFGUID InterfaceName,
            /* [annotation][out] */
            _Out_  LARGE_INTEGER *pUMDVersion);

        HRESULT ( STDMETHODCALLTYPE *GetDesc1 )(
            IDXGIAdapter4 * This,
            /* [annotation][out] */
            _Out_  DXGI_ADAPTER_DESC1 *pDesc);

        HRESULT ( STDMETHODCALLTYPE *GetDesc2 )(
            IDXGIAdapter4 * This,
            /* [annotation][out] */
            _Out_  DXGI_ADAPTER_DESC2 *pDesc);

        HRESULT ( STDMETHODCALLTYPE *RegisterHardwareContentProtectionTeardownStatusEvent )(
            IDXGIAdapter4 * This,
            /* [annotation][in] */
            _In_  HANDLE hEvent,
            /* [annotation][out] */
            _Out_  DWORD *pdwCookie);

        void ( STDMETHODCALLTYPE *UnregisterHardwareContentProtectionTeardownStatus )(
            IDXGIAdapter4 * This,
            /* [annotation][in] */
            _In_  DWORD dwCookie);

        HRESULT ( STDMETHODCALLTYPE *QueryVideoMemoryInfo )(
            IDXGIAdapter4 * This,
            /* [annotation][in] */
            _In_  UINT NodeIndex,
            /* [annotation][in] */
            _In_  DXGI_MEMORY_SEGMENT_GROUP MemorySegmentGroup,
            /* [annotation][out] */
            _Out_  DXGI_QUERY_VIDEO_MEMORY_INFO *pVideoMemoryInfo);

        HRESULT ( STDMETHODCALLTYPE *SetVideoMemoryReservation )(
            IDXGIAdapter4 * This,
            /* [annotation][in] */
            _In_  UINT NodeIndex,
            /* [annotation][in] */
            _In_  DXGI_MEMORY_SEGMENT_GROUP MemorySegmentGroup,
            /* [annotation][in] */
            _In_  UINT64 Reservation);

        HRESULT ( STDMETHODCALLTYPE *RegisterVideoMemoryBudgetChangeNotificationEvent )(
            IDXGIAdapter4 * This,
            /* [annotation][in] */
            _In_  HANDLE hEvent,
            /* [annotation][out] */
            _Out_  DWORD *pdwCookie);

        void ( STDMETHODCALLTYPE *UnregisterVideoMemoryBudgetChangeNotification )(
            IDXGIAdapter4 * This,
            /* [annotation][in] */
            _In_  DWORD dwCookie);

        HRESULT ( STDMETHODCALLTYPE *GetDesc3 )(
            IDXGIAdapter4 * This,
            /* [annotation][out] */
            _Out_  DXGI_ADAPTER_DESC3 *pDesc);

        END_INTERFACE
    } IDXGIAdapter4Vtbl;

    interface IDXGIAdapter4
    {
        CONST_VTBL struct IDXGIAdapter4Vtbl *lpVtbl;
    };



#ifdef COBJMACROS


#define IDXGIAdapter4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) )

#define IDXGIAdapter4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) )

#define IDXGIAdapter4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) )


#define IDXGIAdapter4_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) )

#define IDXGIAdapter4_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) )

#define IDXGIAdapter4_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) )

#define IDXGIAdapter4_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) )


#define IDXGIAdapter4_EnumOutputs(This,Output,ppOutput)	\
    ( (This)->lpVtbl -> EnumOutputs(This,Output,ppOutput) )

#define IDXGIAdapter4_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) )

#define IDXGIAdapter4_CheckInterfaceSupport(This,InterfaceName,pUMDVersion)	\
    ( (This)->lpVtbl -> CheckInterfaceSupport(This,InterfaceName,pUMDVersion) )


#define IDXGIAdapter4_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) )


#define IDXGIAdapter4_GetDesc2(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc2(This,pDesc) )


#define IDXGIAdapter4_RegisterHardwareContentProtectionTeardownStatusEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterHardwareContentProtectionTeardownStatusEvent(This,hEvent,pdwCookie) )

#define IDXGIAdapter4_UnregisterHardwareContentProtectionTeardownStatus(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterHardwareContentProtectionTeardownStatus(This,dwCookie) )

#define IDXGIAdapter4_QueryVideoMemoryInfo(This,NodeIndex,MemorySegmentGroup,pVideoMemoryInfo)	\
    ( (This)->lpVtbl -> QueryVideoMemoryInfo(This,NodeIndex,MemorySegmentGroup,pVideoMemoryInfo) )

#define IDXGIAdapter4_SetVideoMemoryReservation(This,NodeIndex,MemorySegmentGroup,Reservation)	\
    ( (This)->lpVtbl -> SetVideoMemoryReservation(This,NodeIndex,MemorySegmentGroup,Reservation) )

#define IDXGIAdapter4_RegisterVideoMemoryBudgetChangeNotificationEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterVideoMemoryBudgetChangeNotificationEvent(This,hEvent,pdwCookie) )

#define IDXGIAdapter4_UnregisterVideoMemoryBudgetChangeNotification(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterVideoMemoryBudgetChangeNotification(This,dwCookie) )


#define IDXGIAdapter4_GetDesc3(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc3(This,pDesc) )

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIAdapter4_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_6_0000_0001 */
/* [local] */

typedef struct DXGI_OUTPUT_DESC1
{
  WCHAR                 DeviceName [32];
  RECT                  DesktopCoordinates;
  BOOL                  AttachedToDesktop;
  DXGI_MODE_ROTATION    Rotation;
  HMONITOR              Monitor;
  UINT                  BitsPerColor;
  DXGI_COLOR_SPACE_TYPE ColorSpace;
  FLOAT                 RedPrimary  [2];
  FLOAT                 GreenPrimary[2];
  FLOAT                 BluePrimary [2];
  FLOAT                 WhitePoint  [2];
  FLOAT                 MinLuminance;
  FLOAT                 MaxLuminance;
  FLOAT                 MaxFullFrameLuminance;
}   DXGI_OUTPUT_DESC1;

typedef
enum DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAGS
{
  DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAG_FULLSCREEN	      = 1,
  DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAG_WINDOWED	        = 2,
  DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAG_CURSOR_STRETCHED	= 4
}   DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAGS;

DEFINE_ENUM_FLAG_OPERATORS( DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAGS );


extern RPC_IF_HANDLE __MIDL_itf_dxgi1_6_0000_0001_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_6_0000_0001_v0_0_s_ifspec;

#ifndef __IDXGIOutput6_INTERFACE_DEFINED__
#define __IDXGIOutput6_INTERFACE_DEFINED__

/* interface IDXGIOutput6 */
/* [unique][local][uuid][object] */


EXTERN_C const IID IID_IDXGIOutput6;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("068346e8-aaec-4b84-add7-137f513f77a1")
    IDXGIOutput6 : public IDXGIOutput5
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDesc1(
            /* [annotation][out] */
            _Out_  DXGI_OUTPUT_DESC1 *pDesc) = 0;

        virtual HRESULT STDMETHODCALLTYPE CheckHardwareCompositionSupport(
            /* [annotation][out] */
            _Out_  UINT *pFlags) = 0;

    };


#else 	/* C style interface */

    typedef struct IDXGIOutput6Vtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
            IDXGIOutput6 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */
            _COM_Outptr_  void **ppvObject);

        ULONG ( STDMETHODCALLTYPE *AddRef )(
            IDXGIOutput6 * This);

        ULONG ( STDMETHODCALLTYPE *Release )(
            IDXGIOutput6 * This);

        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )(
            IDXGIOutput6 * This,
            /* [annotation][in] */
            _In_  REFGUID Name,
            /* [in] */ UINT DataSize,
            /* [annotation][in] */
            _In_reads_bytes_(DataSize)  const void *pData);

        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )(
            IDXGIOutput6 * This,
            /* [annotation][in] */
            _In_  REFGUID Name,
            /* [annotation][in] */
            _In_opt_  const IUnknown *pUnknown);

        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )(
            IDXGIOutput6 * This,
            /* [annotation][in] */
            _In_  REFGUID Name,
            /* [annotation][out][in] */
            _Inout_  UINT *pDataSize,
            /* [annotation][out] */
            _Out_writes_bytes_(*pDataSize)  void *pData);

        HRESULT ( STDMETHODCALLTYPE *GetParent )(
            IDXGIOutput6 * This,
            /* [annotation][in] */
            _In_  REFIID riid,
            /* [annotation][retval][out] */
            _COM_Outptr_  void **ppParent);

        HRESULT ( STDMETHODCALLTYPE *GetDesc )(
            IDXGIOutput6 * This,
            /* [annotation][out] */
            _Out_  DXGI_OUTPUT_DESC *pDesc);

        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList )(
            IDXGIOutput6 * This,
            /* [in] */ DXGI_FORMAT EnumFormat,
            /* [in] */ UINT Flags,
            /* [annotation][out][in] */
            _Inout_  UINT *pNumModes,
            /* [annotation][out] */
            _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC *pDesc);

        HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode )(
            IDXGIOutput6 * This,
            /* [annotation][in] */
            _In_  const DXGI_MODE_DESC *pModeToMatch,
            /* [annotation][out] */
            _Out_  DXGI_MODE_DESC *pClosestMatch,
            /* [annotation][in] */
            _In_opt_  IUnknown *pConcernedDevice);

        HRESULT ( STDMETHODCALLTYPE *WaitForVBlank )(
            IDXGIOutput6 * This);

        HRESULT ( STDMETHODCALLTYPE *TakeOwnership )(
            IDXGIOutput6 * This,
            /* [annotation][in] */
            _In_  IUnknown *pDevice,
            BOOL Exclusive);

        void ( STDMETHODCALLTYPE *ReleaseOwnership )(
            IDXGIOutput6 * This);

        HRESULT ( STDMETHODCALLTYPE *GetGammaControlCapabilities )(
            IDXGIOutput6 * This,
            /* [annotation][out] */
            _Out_  DXGI_GAMMA_CONTROL_CAPABILITIES *pGammaCaps);

        HRESULT ( STDMETHODCALLTYPE *SetGammaControl )(
            IDXGIOutput6 * This,
            /* [annotation][in] */
            _In_  const DXGI_GAMMA_CONTROL *pArray);

        HRESULT ( STDMETHODCALLTYPE *GetGammaControl )(
            IDXGIOutput6 * This,
            /* [annotation][out] */
            _Out_  DXGI_GAMMA_CONTROL *pArray);

        HRESULT ( STDMETHODCALLTYPE *SetDisplaySurface )(
            IDXGIOutput6 * This,
            /* [annotation][in] */
            _In_  IDXGISurface *pScanoutSurface);

        HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData )(
            IDXGIOutput6 * This,
            /* [annotation][in] */
            _In_  IDXGISurface *pDestination);

        HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )(
            IDXGIOutput6 * This,
            /* [annotation][out] */
            _Out_  DXGI_FRAME_STATISTICS *pStats);

        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeList1 )(
            IDXGIOutput6 * This,
            /* [in] */ DXGI_FORMAT EnumFormat,
            /* [in] */ UINT Flags,
            /* [annotation][out][in] */
            _Inout_  UINT *pNumModes,
            /* [annotation][out] */
            _Out_writes_to_opt_(*pNumModes,*pNumModes)  DXGI_MODE_DESC1 *pDesc);

        HRESULT ( STDMETHODCALLTYPE *FindClosestMatchingMode1 )(
            IDXGIOutput6 * This,
            /* [annotation][in] */
            _In_  const DXGI_MODE_DESC1 *pModeToMatch,
            /* [annotation][out] */
            _Out_  DXGI_MODE_DESC1 *pClosestMatch,
            /* [annotation][in] */
            _In_opt_  IUnknown *pConcernedDevice);

        HRESULT ( STDMETHODCALLTYPE *GetDisplaySurfaceData1 )(
            IDXGIOutput6 * This,
            /* [annotation][in] */
            _In_  IDXGIResource *pDestination);

        HRESULT ( STDMETHODCALLTYPE *DuplicateOutput )(
            IDXGIOutput6 * This,
            /* [annotation][in] */
            _In_  IUnknown *pDevice,
            /* [annotation][out] */
            _COM_Outptr_  IDXGIOutputDuplication **ppOutputDuplication);

        BOOL ( STDMETHODCALLTYPE *SupportsOverlays )(
            IDXGIOutput6 * This);

        HRESULT ( STDMETHODCALLTYPE *CheckOverlaySupport )(
            IDXGIOutput6 * This,
            /* [annotation][in] */
            _In_  DXGI_FORMAT EnumFormat,
            /* [annotation][out] */
            _In_  IUnknown *pConcernedDevice,
            /* [annotation][out] */
            _Out_  UINT *pFlags);

        HRESULT ( STDMETHODCALLTYPE *CheckOverlayColorSpaceSupport )(
            IDXGIOutput6 * This,
            /* [annotation][in] */
            _In_  DXGI_FORMAT Format,
            /* [annotation][in] */
            _In_  DXGI_COLOR_SPACE_TYPE ColorSpace,
            /* [annotation][in] */
            _In_  IUnknown *pConcernedDevice,
            /* [annotation][out] */
            _Out_  UINT *pFlags);

        HRESULT ( STDMETHODCALLTYPE *DuplicateOutput1 )(
            IDXGIOutput6 * This,
            /* [annotation][in] */
            _In_  IUnknown *pDevice,
            /* [in] */ UINT Flags,
            /* [annotation][in] */
            _In_  UINT SupportedFormatsCount,
            /* [annotation][in] */
            _In_reads_(SupportedFormatsCount)  const DXGI_FORMAT *pSupportedFormats,
            /* [annotation][out] */
            _COM_Outptr_  IDXGIOutputDuplication **ppOutputDuplication);

        HRESULT ( STDMETHODCALLTYPE *GetDesc1 )(
            IDXGIOutput6 * This,
            /* [annotation][out] */
            _Out_  DXGI_OUTPUT_DESC1 *pDesc);

        HRESULT ( STDMETHODCALLTYPE *CheckHardwareCompositionSupport )(
            IDXGIOutput6 * This,
            /* [annotation][out] */
            _Out_  UINT *pFlags);

        END_INTERFACE
    } IDXGIOutput6Vtbl;

    interface IDXGIOutput6
    {
        CONST_VTBL struct IDXGIOutput6Vtbl *lpVtbl;
    };



#ifdef COBJMACROS


#define IDXGIOutput6_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) )

#define IDXGIOutput6_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) )

#define IDXGIOutput6_Release(This)	\
    ( (This)->lpVtbl -> Release(This) )


#define IDXGIOutput6_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) )

#define IDXGIOutput6_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) )

#define IDXGIOutput6_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) )

#define IDXGIOutput6_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) )


#define IDXGIOutput6_GetDesc(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc(This,pDesc) )

#define IDXGIOutput6_GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList(This,EnumFormat,Flags,pNumModes,pDesc) )

#define IDXGIOutput6_FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode(This,pModeToMatch,pClosestMatch,pConcernedDevice) )

#define IDXGIOutput6_WaitForVBlank(This)	\
    ( (This)->lpVtbl -> WaitForVBlank(This) )

#define IDXGIOutput6_TakeOwnership(This,pDevice,Exclusive)	\
    ( (This)->lpVtbl -> TakeOwnership(This,pDevice,Exclusive) )

#define IDXGIOutput6_ReleaseOwnership(This)	\
    ( (This)->lpVtbl -> ReleaseOwnership(This) )

#define IDXGIOutput6_GetGammaControlCapabilities(This,pGammaCaps)	\
    ( (This)->lpVtbl -> GetGammaControlCapabilities(This,pGammaCaps) )

#define IDXGIOutput6_SetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> SetGammaControl(This,pArray) )

#define IDXGIOutput6_GetGammaControl(This,pArray)	\
    ( (This)->lpVtbl -> GetGammaControl(This,pArray) )

#define IDXGIOutput6_SetDisplaySurface(This,pScanoutSurface)	\
    ( (This)->lpVtbl -> SetDisplaySurface(This,pScanoutSurface) )

#define IDXGIOutput6_GetDisplaySurfaceData(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData(This,pDestination) )

#define IDXGIOutput6_GetFrameStatistics(This,pStats)	\
    ( (This)->lpVtbl -> GetFrameStatistics(This,pStats) )


#define IDXGIOutput6_GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc)	\
    ( (This)->lpVtbl -> GetDisplayModeList1(This,EnumFormat,Flags,pNumModes,pDesc) )

#define IDXGIOutput6_FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice)	\
    ( (This)->lpVtbl -> FindClosestMatchingMode1(This,pModeToMatch,pClosestMatch,pConcernedDevice) )

#define IDXGIOutput6_GetDisplaySurfaceData1(This,pDestination)	\
    ( (This)->lpVtbl -> GetDisplaySurfaceData1(This,pDestination) )

#define IDXGIOutput6_DuplicateOutput(This,pDevice,ppOutputDuplication)	\
    ( (This)->lpVtbl -> DuplicateOutput(This,pDevice,ppOutputDuplication) )


#define IDXGIOutput6_SupportsOverlays(This)	\
    ( (This)->lpVtbl -> SupportsOverlays(This) )


#define IDXGIOutput6_CheckOverlaySupport(This,EnumFormat,pConcernedDevice,pFlags)	\
    ( (This)->lpVtbl -> CheckOverlaySupport(This,EnumFormat,pConcernedDevice,pFlags) )


#define IDXGIOutput6_CheckOverlayColorSpaceSupport(This,Format,ColorSpace,pConcernedDevice,pFlags)	\
    ( (This)->lpVtbl -> CheckOverlayColorSpaceSupport(This,Format,ColorSpace,pConcernedDevice,pFlags) )


#define IDXGIOutput6_DuplicateOutput1(This,pDevice,Flags,SupportedFormatsCount,pSupportedFormats,ppOutputDuplication)	\
    ( (This)->lpVtbl -> DuplicateOutput1(This,pDevice,Flags,SupportedFormatsCount,pSupportedFormats,ppOutputDuplication) )


#define IDXGIOutput6_GetDesc1(This,pDesc)	\
    ( (This)->lpVtbl -> GetDesc1(This,pDesc) )

#define IDXGIOutput6_CheckHardwareCompositionSupport(This,pFlags)	\
    ( (This)->lpVtbl -> CheckHardwareCompositionSupport(This,pFlags) )

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDXGIOutput6_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxgi1_6_0000_0002 */
/* [local] */

/*#endif*/ /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
/*#pragma endregion*/
DEFINE_GUID(IID_IDXGIAdapter4,0x3c8d99d1,0x4fbf,0x4181,0xa8,0x2c,0xaf,0x66,0xbf,0x7b,0xd2,0x4e);
DEFINE_GUID(IID_IDXGIOutput6,0x068346e8,0xaaec,0x4b84,0xad,0xd7,0x13,0x7f,0x51,0x3f,0x77,0xa1);


extern RPC_IF_HANDLE __MIDL_itf_dxgi1_6_0000_0002_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxgi1_6_0000_0002_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

DEFINE_GUID(IID_IDXGIFactory6,0xc1b6694f,0xff09,0x44a9,0xb0,0x3c,0x77,0x90,0x0a,0x0a,0x1d,0x17);
DEFINE_GUID(IID_IDXGIFactory7,0xa4966eed,0x76db,0x44da,0x84,0xc1,0xee,0x9a,0x7a,0xfb,0x20,0xa8);
#endif

HRESULT
STDMETHODCALLTYPE
Present1Callback (IDXGISwapChain1         *This,
                  UINT                     SyncInterval,
                  UINT                     PresentFlags,
            const DXGI_PRESENT_PARAMETERS *pPresentParameters);

HRESULT
STDMETHODCALLTYPE
DXGIFactory2_CreateSwapChainForHwnd_Override (
                 IDXGIFactory2                   *This,
      _In_       IUnknown                        *pDevice,
      _In_       HWND                             hWnd,
      _In_ const DXGI_SWAP_CHAIN_DESC1           *pDesc,
  _In_opt_       DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc,
  _In_opt_       IDXGIOutput                     *pRestrictToOutput,
     _Out_       IDXGISwapChain1                 **ppSwapChain
);

HRESULT
STDMETHODCALLTYPE
DXGIFactory2_CreateSwapChainForCoreWindow_Override (
                 IDXGIFactory2             *This,
      _In_       IUnknown                  *pDevice,
      _In_       IUnknown                  *pWindow,
      _In_ const DXGI_SWAP_CHAIN_DESC1     *pDesc,
  _In_opt_       IDXGIOutput               *pRestrictToOutput,
     _Out_       IDXGISwapChain1          **ppSwapChain
);

HRESULT
STDMETHODCALLTYPE
DXGIFactory2_CreateSwapChainForComposition_Override (
                 IDXGIFactory2          *This,
  _In_           IUnknown               *pDevice,
  _In_     const DXGI_SWAP_CHAIN_DESC1  *pDesc,
  _In_opt_       IDXGIOutput            *pRestrictToOutput,
  _Outptr_       IDXGISwapChain1       **ppSwapChain
);




















#include <d3d12.h>

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
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11On12Device * This,
            REFIID riid,
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11On12Device * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11On12Device * This);
        
        DECLSPEC_XFGVIRT(ID3D11On12Device, CreateWrappedResource)
        HRESULT ( STDMETHODCALLTYPE *CreateWrappedResource )( 
            ID3D11On12Device * This,
            _In_  IUnknown *pResource12,
            _In_  const D3D11_RESOURCE_FLAGS *pFlags11,
            D3D12_RESOURCE_STATES InState,
            D3D12_RESOURCE_STATES OutState,
            REFIID riid,
            _COM_Outptr_opt_  void **ppResource11);
        
        DECLSPEC_XFGVIRT(ID3D11On12Device, ReleaseWrappedResources)
        void ( STDMETHODCALLTYPE *ReleaseWrappedResources )( 
            ID3D11On12Device * This,
            _In_reads_( NumResources )  ID3D11Resource *const *ppResources,
            UINT NumResources);
        
        DECLSPEC_XFGVIRT(ID3D11On12Device, AcquireWrappedResources)
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


#ifndef __ID3D11On12Device1_INTERFACE_DEFINED__
#define __ID3D11On12Device1_INTERFACE_DEFINED__

/* interface ID3D11On12Device1 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11On12Device1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("bdb64df4-ea2f-4c70-b861-aaab1258bb5d")
    ID3D11On12Device1 : public ID3D11On12Device
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetD3D12Device( 
            REFIID riid,
            _COM_Outptr_  void **ppvDevice) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11On12Device1Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11On12Device1 * This,
            REFIID riid,
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11On12Device1 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11On12Device1 * This);
        
        DECLSPEC_XFGVIRT(ID3D11On12Device, CreateWrappedResource)
        HRESULT ( STDMETHODCALLTYPE *CreateWrappedResource )( 
            ID3D11On12Device1 * This,
            _In_  IUnknown *pResource12,
            _In_  const D3D11_RESOURCE_FLAGS *pFlags11,
            D3D12_RESOURCE_STATES InState,
            D3D12_RESOURCE_STATES OutState,
            REFIID riid,
            _COM_Outptr_opt_  void **ppResource11);
        
        DECLSPEC_XFGVIRT(ID3D11On12Device, ReleaseWrappedResources)
        void ( STDMETHODCALLTYPE *ReleaseWrappedResources )( 
            ID3D11On12Device1 * This,
            _In_reads_( NumResources )  ID3D11Resource *const *ppResources,
            UINT NumResources);
        
        DECLSPEC_XFGVIRT(ID3D11On12Device, AcquireWrappedResources)
        void ( STDMETHODCALLTYPE *AcquireWrappedResources )( 
            ID3D11On12Device1 * This,
            _In_reads_( NumResources )  ID3D11Resource *const *ppResources,
            UINT NumResources);
        
        DECLSPEC_XFGVIRT(ID3D11On12Device1, GetD3D12Device)
        HRESULT ( STDMETHODCALLTYPE *GetD3D12Device )( 
            ID3D11On12Device1 * This,
            REFIID riid,
            _COM_Outptr_  void **ppvDevice);
        
        END_INTERFACE
    } ID3D11On12Device1Vtbl;

    interface ID3D11On12Device1
    {
        CONST_VTBL struct ID3D11On12Device1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11On12Device1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11On12Device1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11On12Device1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11On12Device1_CreateWrappedResource(This,pResource12,pFlags11,InState,OutState,riid,ppResource11)	\
    ( (This)->lpVtbl -> CreateWrappedResource(This,pResource12,pFlags11,InState,OutState,riid,ppResource11) ) 

#define ID3D11On12Device1_ReleaseWrappedResources(This,ppResources,NumResources)	\
    ( (This)->lpVtbl -> ReleaseWrappedResources(This,ppResources,NumResources) ) 

#define ID3D11On12Device1_AcquireWrappedResources(This,ppResources,NumResources)	\
    ( (This)->lpVtbl -> AcquireWrappedResources(This,ppResources,NumResources) ) 


#define ID3D11On12Device1_GetD3D12Device(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetD3D12Device(This,riid,ppvDevice) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D11On12Device1_INTERFACE_DEFINED__ */


#ifndef __ID3D11On12Device2_INTERFACE_DEFINED__
#define __ID3D11On12Device2_INTERFACE_DEFINED__

/* interface ID3D11On12Device2 */
/* [unique][local][object][uuid] */ 


EXTERN_C const IID IID_ID3D11On12Device2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("dc90f331-4740-43fa-866e-67f12cb58223")
    ID3D11On12Device2 : public ID3D11On12Device1
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE UnwrapUnderlyingResource( 
            _In_  ID3D11Resource *pResource11,
            _In_  ID3D12CommandQueue *pCommandQueue,
            REFIID riid,
            _COM_Outptr_  void **ppvResource12) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReturnUnderlyingResource( 
            _In_  ID3D11Resource *pResource11,
            UINT NumSync,
            _In_reads_(NumSync)   UINT64 *pSignalValues,
            _In_reads_(NumSync)   ID3D12Fence **ppFences) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D11On12Device2Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D11On12Device2 * This,
            REFIID riid,
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D11On12Device2 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D11On12Device2 * This);
        
        DECLSPEC_XFGVIRT(ID3D11On12Device, CreateWrappedResource)
        HRESULT ( STDMETHODCALLTYPE *CreateWrappedResource )( 
            ID3D11On12Device2 * This,
            _In_  IUnknown *pResource12,
            _In_  const D3D11_RESOURCE_FLAGS *pFlags11,
            D3D12_RESOURCE_STATES InState,
            D3D12_RESOURCE_STATES OutState,
            REFIID riid,
            _COM_Outptr_opt_  void **ppResource11);
        
        DECLSPEC_XFGVIRT(ID3D11On12Device, ReleaseWrappedResources)
        void ( STDMETHODCALLTYPE *ReleaseWrappedResources )( 
            ID3D11On12Device2 * This,
            _In_reads_( NumResources )  ID3D11Resource *const *ppResources,
            UINT NumResources);
        
        DECLSPEC_XFGVIRT(ID3D11On12Device, AcquireWrappedResources)
        void ( STDMETHODCALLTYPE *AcquireWrappedResources )( 
            ID3D11On12Device2 * This,
            _In_reads_( NumResources )  ID3D11Resource *const *ppResources,
            UINT NumResources);
        
        DECLSPEC_XFGVIRT(ID3D11On12Device1, GetD3D12Device)
        HRESULT ( STDMETHODCALLTYPE *GetD3D12Device )( 
            ID3D11On12Device2 * This,
            REFIID riid,
            _COM_Outptr_  void **ppvDevice);
        
        DECLSPEC_XFGVIRT(ID3D11On12Device2, UnwrapUnderlyingResource)
        HRESULT ( STDMETHODCALLTYPE *UnwrapUnderlyingResource )( 
            ID3D11On12Device2 * This,
            _In_  ID3D11Resource *pResource11,
            _In_  ID3D12CommandQueue *pCommandQueue,
            REFIID riid,
            _COM_Outptr_  void **ppvResource12);
        
        DECLSPEC_XFGVIRT(ID3D11On12Device2, ReturnUnderlyingResource)
        HRESULT ( STDMETHODCALLTYPE *ReturnUnderlyingResource )( 
            ID3D11On12Device2 * This,
            _In_  ID3D11Resource *pResource11,
            UINT NumSync,
            _In_reads_(NumSync)   UINT64 *pSignalValues,
            _In_reads_(NumSync)   ID3D12Fence **ppFences);
        
        END_INTERFACE
    } ID3D11On12Device2Vtbl;

    interface ID3D11On12Device2
    {
        CONST_VTBL struct ID3D11On12Device2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D11On12Device2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D11On12Device2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D11On12Device2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D11On12Device2_CreateWrappedResource(This,pResource12,pFlags11,InState,OutState,riid,ppResource11)	\
    ( (This)->lpVtbl -> CreateWrappedResource(This,pResource12,pFlags11,InState,OutState,riid,ppResource11) ) 

#define ID3D11On12Device2_ReleaseWrappedResources(This,ppResources,NumResources)	\
    ( (This)->lpVtbl -> ReleaseWrappedResources(This,ppResources,NumResources) ) 

#define ID3D11On12Device2_AcquireWrappedResources(This,ppResources,NumResources)	\
    ( (This)->lpVtbl -> AcquireWrappedResources(This,ppResources,NumResources) ) 


#define ID3D11On12Device2_GetD3D12Device(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetD3D12Device(This,riid,ppvDevice) ) 


#define ID3D11On12Device2_UnwrapUnderlyingResource(This,pResource11,pCommandQueue,riid,ppvResource12)	\
    ( (This)->lpVtbl -> UnwrapUnderlyingResource(This,pResource11,pCommandQueue,riid,ppvResource12) ) 

#define ID3D11On12Device2_ReturnUnderlyingResource(This,pResource11,NumSync,pSignalValues,ppFences)	\
    ( (This)->lpVtbl -> ReturnUnderlyingResource(This,pResource11,NumSync,pSignalValues,ppFences) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */
#endif 	/* __ID3D11On12Device2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_d3d11on12_0000_0003 */
/* [local] */ 

DEFINE_GUID(IID_ID3D11On12Device, 0x85611e73,0x70a9,0x490e,0x96,0x14,0xa9,0xe3,0x02,0x77,0x79,0x04);
DEFINE_GUID(IID_ID3D11On12Device1,0xbdb64df4,0xea2f,0x4c70,0xb8,0x61,0xaa,0xab,0x12,0x58,0xbb,0x5d);
DEFINE_GUID(IID_ID3D11On12Device2,0xdc90f331,0x4740,0x43fa,0x86,0x6e,0x67,0xf1,0x2c,0xb5,0x82,0x23);

extern RPC_IF_HANDLE __MIDL_itf_d3d11on12_0000_0003_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d11on12_0000_0003_v0_0_s_ifspec;

#endif /* __SK__DXGI_INTERFACES_H__ */