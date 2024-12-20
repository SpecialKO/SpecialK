// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
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
#include <SpecialK/render/d3dkmt/d3dkmt.h>

extern "C" FARPROC D3DKMTCloseAdapter                     = nullptr;
extern "C" FARPROC D3DKMTDestroyAllocation                = nullptr;
extern "C" FARPROC D3DKMTDestroyContext                   = nullptr;
extern "C" FARPROC D3DKMTDestroyDevice                    = nullptr;
extern "C" FARPROC D3DKMTDestroySynchronizationObject     = nullptr;
extern "C" FARPROC D3DKMTQueryAdapterInfo                 = nullptr;
extern "C" FARPROC D3DKMTSetDisplayPrivateDriverFormat    = nullptr;
extern "C" FARPROC D3DKMTSignalSynchronizationObject      = nullptr;
extern "C" FARPROC D3DKMTUnlock                           = nullptr;
extern "C" FARPROC D3DKMTWaitForSynchronizationObject     = nullptr;
extern "C" FARPROC D3DKMTCreateAllocation                 = nullptr;
extern "C" FARPROC D3DKMTCreateContext                    = nullptr;
extern "C" FARPROC D3DKMTCreateDevice                     = nullptr;
extern "C" FARPROC D3DKMTCreateSynchronizationObject      = nullptr;
extern "C" FARPROC D3DKMTEscape                           = nullptr;
extern "C" FARPROC D3DKMTGetContextSchedulingPriority     = nullptr;
extern "C" FARPROC D3DKMTGetDeviceState                   = nullptr;
extern "C" FARPROC D3DKMTGetDisplayModeList               = nullptr;
extern "C" FARPROC D3DKMTGetMultisampleMethodList         = nullptr;
extern "C" FARPROC D3DKMTGetRuntimeData                   = nullptr;
extern "C" FARPROC D3DKMTGetSharedPrimaryHandle           = nullptr;
extern "C" FARPROC D3DKMTLock                             = nullptr;
extern "C" FARPROC D3DKMTOpenAdapterFromHdc               = nullptr;
extern "C" FARPROC D3DKMTOpenAdapterFromLuid              = nullptr;
extern "C" FARPROC D3DKMTOpenAdapterFromGdiDisplayName    = nullptr;
extern "C" FARPROC D3DKMTOpenResource                     = nullptr;
extern "C" FARPROC D3DKMTPresent                          = nullptr;
extern "C" FARPROC D3DKMTQueryAllocationResidency         = nullptr;
extern "C" FARPROC D3DKMTQueryResourceInfo                = nullptr;
extern "C" FARPROC D3DKMTRender                           = nullptr;
extern "C" FARPROC D3DKMTSetAllocationPriority            = nullptr;
extern "C" FARPROC D3DKMTSetContextSchedulingPriority     = nullptr;
extern "C" FARPROC D3DKMTSetDisplayMode                   = nullptr;
extern "C" FARPROC D3DKMTSetGammaRamp                     = nullptr;
extern "C" FARPROC D3DKMTSetVidPnSourceOwner              = nullptr;
extern "C" FARPROC D3DKMTWaitForVerticalBlankEvent        = nullptr;
extern "C" FARPROC D3DKMTGetMultiPlaneOverlayCaps         = nullptr;
extern "C" FARPROC D3DKMTGetScanLine                      = nullptr;

HRESULT
SK_D3DKMT_QueryAdapterInfo (D3DKMT_QUERYADAPTERINFO *pQueryAdapterInfo)
{
  if (! D3DKMTQueryAdapterInfo)
        D3DKMTQueryAdapterInfo =
         SK_GetProcAddress (
           SK_LoadLibraryW ( L"gdi32.dll" ),
             "D3DKMTQueryAdapterInfo"
                           );

  if (D3DKMTQueryAdapterInfo != nullptr)
  {
    return
       reinterpret_cast <
         PFND3DKMT_QUERYADAPTERINFO                    > (
            D3DKMTQueryAdapterInfo) (pQueryAdapterInfo);
  }

  return E_FAIL;
}

HRESULT
SK_D3DKMT_CloseAdapter (D3DKMT_CLOSEADAPTER *pCloseAdapter)
{
  if (! D3DKMTCloseAdapter)
        D3DKMTCloseAdapter =
         SK_GetProcAddress (
           SK_LoadLibraryW ( L"gdi32.dll" ),
             "D3DKMTCloseAdapter"
                           );

  if (D3DKMTCloseAdapter != nullptr)
  {
    return
       reinterpret_cast <
         PFND3DKMT_CLOSEADAPTER> (
            D3DKMTCloseAdapter) (pCloseAdapter);
  }

  return E_FAIL;
}

HRESULT
SK_D3DKMT_OpenAdapterFromDXGI (IUnknown *pDevice, D3DKMT_HANDLE *pHandle)
{
  if (! (pDevice && pHandle))
    return E_POINTER;

  SK_ComQIPtr <IDXGIDevice>
      pDXGIDevice (pDevice);
  if (pDXGIDevice.p != nullptr)
  {
    SK_ComPtr <IDXGIAdapter>  pAdapter;
    pDXGIDevice->GetAdapter (&pAdapter.p);

    if (pAdapter.p != nullptr)
    {
      DXGI_ADAPTER_DESC                  adapter_desc = { };
      if (SUCCEEDED (pAdapter->GetDesc (&adapter_desc)))
      {
        if (D3DKMTOpenAdapterFromLuid == nullptr)
            D3DKMTOpenAdapterFromLuid =
          SK_GetProcAddress ( L"gdi32.dll",
           "D3DKMTOpenAdapterFromLuid" );

        if (D3DKMTOpenAdapterFromLuid)
        {
          D3DKMT_OPENADAPTERFROMLUID oaLuid =
                 { adapter_desc.AdapterLuid };

          if (NT_SUCCESS (((D3DKMTOpenAdapterFromLuid_pfn)D3DKMTOpenAdapterFromLuid) (&oaLuid)))
          {
            *pHandle = oaLuid.hAdapter;

            return S_OK;
          }

          return
            E_NOTFOUND;
        }

        return
          E_NOTIMPL;
      }

      return
        E_UNEXPECTED;
    }

    return
      E_ACCESSDENIED;
  }

  return
    E_NOINTERFACE;
}