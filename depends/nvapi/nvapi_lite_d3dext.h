#pragma once
#include"nvapi_lite_salstart.h"
#include"nvapi_lite_common.h"
#pragma pack(push,8)
#ifdef __cplusplus
extern "C" {
#endif
#if defined(__cplusplus) && (defined(__d3d10_h__) || defined(__d3d10_1_h__) || defined(__d3d11_h__))
//! \ingroup dx
//! D3D_FEATURE_LEVEL supported - used in NvAPI_D3D11_CreateDevice() and NvAPI_D3D11_CreateDeviceAndSwapChain()
typedef enum
{
    NVAPI_DEVICE_FEATURE_LEVEL_NULL       = -1,
    NVAPI_DEVICE_FEATURE_LEVEL_10_0       = 0,
    NVAPI_DEVICE_FEATURE_LEVEL_10_0_PLUS  = 1,
    NVAPI_DEVICE_FEATURE_LEVEL_10_1       = 2,
    NVAPI_DEVICE_FEATURE_LEVEL_11_0       = 3,
} NVAPI_DEVICE_FEATURE_LEVEL;

#endif  //defined(__cplusplus) && (defined(__d3d10_h__) || defined(__d3d10_1_h__) || defined(__d3d11_h__))
#if defined(__cplusplus) && defined(__d3d11_h__)
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D11_CreateDevice
//
//!   DESCRIPTION: This function tries to create a DirectX 11 device. If the call fails (if we are running
//!                on pre-DirectX 11 hardware), depending on the type of hardware it will try to create a DirectX 10.1 OR DirectX 10.0+
//!                OR DirectX 10.0 device. The function call is the same as D3D11CreateDevice(), but with an extra 
//!                argument (D3D_FEATURE_LEVEL supported by the device) that the function fills in. This argument
//!                can contain -1 (NVAPI_DEVICE_FEATURE_LEVEL_NULL), if the requested featureLevel is less than DirecX 10.0.
//!
//! SUPPORTED OS:  Windows Vista and higher
//!
//!
//! \since Release: 185
//!
//! \param [in]   pAdapter
//! \param [in]   DriverType
//! \param [in]   Software
//! \param [in]   Flags
//! \param [in]   *pFeatureLevels
//! \param [in]   FeatureLevels
//! \param [in]   SDKVersion
//! \param [in]   **ppDevice
//! \param [in]   *pFeatureLevel
//! \param [in]   **ppImmediateContext
//! \param [in]   *pSupportedLevel  D3D_FEATURE_LEVEL supported
//!
//! \return NVAPI_OK if the createDevice call succeeded.
//!
//! \ingroup dx
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_D3D11_CreateDevice(IDXGIAdapter* pAdapter,
                                         D3D_DRIVER_TYPE DriverType,
                                         HMODULE Software,
                                         UINT Flags,
                                         CONST D3D_FEATURE_LEVEL *pFeatureLevels,
                                         UINT FeatureLevels,
                                         UINT SDKVersion,
                                         ID3D11Device **ppDevice,
                                         D3D_FEATURE_LEVEL *pFeatureLevel,
                                         ID3D11DeviceContext **ppImmediateContext,
                                         NVAPI_DEVICE_FEATURE_LEVEL *pSupportedLevel);


#endif //defined(__cplusplus) && defined(__d3d11_h__)
#if defined(__cplusplus) && defined(__d3d11_h__)
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D11_CreateDeviceAndSwapChain
//
//!   DESCRIPTION: This function tries to create a DirectX 11 device and swap chain. If the call fails (if we are 
//!                running on pre=DirectX 11 hardware), depending on the type of hardware it will try to create a DirectX 10.1 OR 
//!                DirectX 10.0+ OR DirectX 10.0 device. The function call is the same as D3D11CreateDeviceAndSwapChain,  
//!                but with an extra argument (D3D_FEATURE_LEVEL supported by the device) that the function fills
//!                in. This argument can contain -1 (NVAPI_DEVICE_FEATURE_LEVEL_NULL), if the requested featureLevel
//!                is less than DirectX 10.0.
//!
//! SUPPORTED OS:  Windows Vista and higher
//!
//!
//! \since Release: 185
//!
//! \param [in]     pAdapter
//! \param [in]     DriverType
//! \param [in]     Software
//! \param [in]     Flags
//! \param [in]     *pFeatureLevels
//! \param [in]     FeatureLevels
//! \param [in]     SDKVersion
//! \param [in]     *pSwapChainDesc
//! \param [in]     **ppSwapChain
//! \param [in]     **ppDevice
//! \param [in]     *pFeatureLevel
//! \param [in]     **ppImmediateContext
//! \param [in]     *pSupportedLevel  D3D_FEATURE_LEVEL supported
//!
//!return  NVAPI_OK if the createDevice with swap chain call succeeded.
//!
//! \ingroup dx
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_D3D11_CreateDeviceAndSwapChain(IDXGIAdapter* pAdapter,
                                         D3D_DRIVER_TYPE DriverType,
                                         HMODULE Software,
                                         UINT Flags,
                                         CONST D3D_FEATURE_LEVEL *pFeatureLevels,
                                         UINT FeatureLevels,
                                         UINT SDKVersion,
                                         CONST DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
                                         IDXGISwapChain **ppSwapChain,
                                         ID3D11Device **ppDevice,
                                         D3D_FEATURE_LEVEL *pFeatureLevel,
                                         ID3D11DeviceContext **ppImmediateContext,
                                         NVAPI_DEVICE_FEATURE_LEVEL *pSupportedLevel);



#endif //defined(__cplusplus) && defined(__d3d11_h__)
#if defined(__cplusplus) && defined(__d3d11_h__)
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D11_SetDepthBoundsTest
//
//!   DESCRIPTION: This function enables/disables the depth bounds test
//!
//! SUPPORTED OS:  Windows Vista and higher
//!
//!
//! \param [in]        pDeviceOrContext   The device or device context to set depth bounds test
//! \param [in]        bEnable            Enable(non-zero)/disable(zero) the depth bounds test
//! \param [in]        fMinDepth          The minimum depth for depth bounds test
//! \param [in]        fMaxDepth          The maximum depth for depth bounds test
//!                                       The valid values for fMinDepth and fMaxDepth
//!                                       are such that 0 <= fMinDepth <= fMaxDepth <= 1
//!
//! \return  ::NVAPI_OK if the depth bounds test was correcly enabled or disabled
//!
//! \ingroup dx
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_D3D11_SetDepthBoundsTest(IUnknown* pDeviceOrContext,
                                               NvU32 bEnable,
                                               float fMinDepth,
                                               float fMaxDepth);

#endif //defined(__cplusplus) && defined(__d3d11_h__)

#include"nvapi_lite_salend.h"
#ifdef __cplusplus
}
#endif
#pragma pack(pop)
