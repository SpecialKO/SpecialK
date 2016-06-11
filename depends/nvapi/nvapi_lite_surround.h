#pragma once
#include"nvapi_lite_salstart.h"
#include"nvapi_lite_common.h"
#pragma pack(push,8)
#ifdef __cplusplus
extern "C" {
#endif
//! SUPPORTED OS:  Windows XP and higher
//!
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_DISP_GetGDIPrimaryDisplayId
//
//! DESCRIPTION:     This API returns the Display ID of the GDI Primary.
//!
//! \param [out]     displayId   Display ID of the GDI Primary display.
//!
//! \retval ::NVAPI_OK:                          Capabilties have been returned.
//! \retval ::NVAPI_NVIDIA_DEVICE_NOT_FOUND:     GDI Primary not on an NVIDIA GPU.
//! \retval ::NVAPI_INVALID_ARGUMENT:            One or more args passed in are invalid.
//! \retval ::NVAPI_API_NOT_INTIALIZED:          The NvAPI API needs to be initialized first
//! \retval ::NVAPI_NO_IMPLEMENTATION:           This entrypoint not available
//! \retval ::NVAPI_ERROR:                       Miscellaneous error occurred
//!
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DISP_GetGDIPrimaryDisplayId(NvU32* displayId);
#define NV_MOSAIC_MAX_DISPLAYS      (64)
//! SUPPORTED OS:  Windows Vista and higher
//!
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_Mosaic_GetDisplayViewportsByResolution
//
//! DESCRIPTION:     This API returns the viewports that would be applied on
//!                  the requested display.
//!
//! \param [in]      displayId       Display ID of a single display in the active
//!                                  mosaic topology to query.
//! \param [in]      srcWidth        Width of full display topology. If both
//!                                  width and height are 0, the current
//!                                  resolution is used.
//! \param [in]      srcHeight       Height of full display topology. If both
//!                                  width and height are 0, the current
//!                                  resolution is used.
//! \param [out]     viewports       Array of NV_RECT viewports which represent
//!                                  the displays as identified in
//!                                  NvAPI_Mosaic_EnumGridTopologies. If the
//!                                  requested resolution is a single-wide
//!                                  resolution, only viewports[0] will
//!                                  contain the viewport details, regardless
//!                                  of which display is driving the display.
//! \param [out]     bezelCorrected  Returns 1 if the requested resolution is
//!                                  bezel corrected. May be NULL.
//!
//! \retval ::NVAPI_OK                          Capabilties have been returned.
//! \retval ::NVAPI_INVALID_ARGUMENT            One or more args passed in are invalid.
//! \retval ::NVAPI_API_NOT_INTIALIZED          The NvAPI API needs to be initialized first
//! \retval ::NVAPI_MOSAIC_NOT_ACTIVE           The display does not belong to an active Mosaic Topology
//! \retval ::NVAPI_NO_IMPLEMENTATION           This entrypoint not available
//! \retval ::NVAPI_ERROR                       Miscellaneous error occurred
//!
//! \ingroup mosaicapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_Mosaic_GetDisplayViewportsByResolution(NvU32 displayId, NvU32 srcWidth, NvU32 srcHeight, NV_RECT viewports[NV_MOSAIC_MAX_DISPLAYS], NvU8* bezelCorrected);

#include"nvapi_lite_salend.h"
#ifdef __cplusplus
}
#endif
#pragma pack(pop)
