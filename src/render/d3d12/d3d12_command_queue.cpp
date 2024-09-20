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
#define __SK_SUBSYSTEM__ L"Direct3D12"

#include <SpecialK/render/d3d12/d3d12_command_queue.h>
#include <SpecialK/render/ngx/ngx_dlss.h>

D3D12CommandQueue_ExecuteCommandLists_pfn
D3D12CommandQueue_ExecuteCommandLists_Original = nullptr;

IDXGISwapChain3* pLazyD3D12Chain  = nullptr;
ID3D12Device*    pLazyD3D12Device = nullptr;

void STDMETHODCALLTYPE
D3D12CommandQueue_ExecuteCommandLists_Detour (
  ID3D12CommandQueue        *This,
  UINT                      NumCommandLists,
  ID3D12CommandList* const  *ppCommandLists )
{
  SK_RenderBackend_V2 &rb =
    SK_GetCurrentRenderBackend ();

  static bool once = false;

  D3D12_COMMAND_QUEUE_DESC
    queueDesc = This->GetDesc ();

  if ( pLazyD3D12Chain  != nullptr &&
       pLazyD3D12Device != nullptr && queueDesc.Type == D3D12_COMMAND_LIST_TYPE_DIRECT )
  {
    if ((! std::exchange (once, true)) && rb.d3d12.command_queue.p == nullptr)
    {
      SK_ComPtr <ID3D12Device> pDevice12;

      if ( SUCCEEDED (   This->GetDevice (
                           IID_ID3D12Device,
                          (void **)&pDevice12.p
                                         ) // We are not holding a ref, so test pointers before using
                     ) && SK_ValidatePointer                         (pLazyD3D12Device,      true) &&
                          SK_IsAddressExecutable ((*(const void***)*(&pLazyD3D12Device))[0], true) &&
                          pDevice12.IsEqualObject (
                 pLazyD3D12Device                 )
         )
      {
        if (rb.d3d12.command_queue.p == nullptr)
        {
          SK_ComPtr <ID3D12Device>       pDevice;
          SK_ComPtr <IDXGISwapChain>     pSwapChain;
          SK_ComPtr <ID3D12CommandQueue> pCmdQueue;

          if (SK_slGetNativeInterface (pLazyD3D12Device, (void **)&pDevice.p   ) != sl::Result::eOk)
                             pDevice = pLazyD3D12Device;
          if (SK_slGetNativeInterface (pLazyD3D12Chain,  (void **)&pSwapChain.p) != sl::Result::eOk)
                          pSwapChain = pLazyD3D12Chain;
          if (SK_slGetNativeInterface (This,             (void **)&pCmdQueue.p ) != sl::Result::eOk)
                           pCmdQueue = This;

          // Now we are holding a ref...
          rb.setDevice            (pDevice.p);
          rb.swapchain           = pSwapChain.p;
          rb.d3d12.command_queue = pCmdQueue.p;
          rb.api                 = SK_RenderAPI::D3D12;

          _d3d12_rbk->init (
            (IDXGISwapChain3 *)pSwapChain.p,
              pCmdQueue.p
          );
        }
      }

      else once = false;
    }
  }

  bool bDLSSG = false;

  if (once && queueDesc.Type == D3D12_COMMAND_LIST_TYPE_DIRECT && (This == rb.d3d12.command_queue || This == _d3d12_rbk->_pCommandQueue))
  {
    const auto frame_id =
      SK_GetFramesDrawn ();

    if ( InterlockedExchange (&SK_Reflex_LastFrameMarked, frame_id) <
                                                          frame_id )
    {
      rb.setLatencyMarkerNV (SIMULATION_END);
      rb.setLatencyMarkerNV (RENDERSUBMIT_START);
    }
  }

  else if (queueDesc.Type == D3D12_COMMAND_LIST_TYPE_COMPUTE && SK_DLSSG_CopyCommandList != nullptr)
  {
    for (UINT i = 0; i < NumCommandLists ; ++i)
    {
      if (ppCommandLists [i] == SK_DLSSG_CopyCommandList)
      {
        SK_DLSSG_CopyCommandList = nullptr;
                          bDLSSG = true;
                                  break;
      }
    }
  }

  D3D12CommandQueue_ExecuteCommandLists_Original (
    This,
      NumCommandLists,
       ppCommandLists
  );

  if (bDLSSG)
  {
    auto &compute =
      _d3d12_rbk->computeCopy;

    if (compute.GPUTimestampFreq == 0)
      This->GetTimestampFrequency (&compute.GPUTimestampFreq);

    if ( compute.dlssg_fence.value == 0)
    {
      if ( UINT64      sync_value = compute.dlssg_fence.value + 1;
           SUCCEEDED (This->Signal (compute.dlssg_fence,
                       sync_value))
         )
      {
        compute.dlssg_fence.value =
                       sync_value;
      }
    }

    else
    if ( compute.dlssg_fence->GetCompletedValue () >=
         compute.dlssg_fence.value )
    {
      UINT64      *timestamps = nullptr;
      D3D12_RANGE       range = { 0,
                                  sizeof (UINT64) * 2 };

      if (SUCCEEDED (_d3d12_rbk->queries.dlssg.pReadBack.p->Map (0, &range, (void **)&timestamps)))
      {
        compute.timestamps [0].Start = timestamps [0];
        compute.timestamps [0].End   = timestamps [1];

                                                        range = { 0, 0 };
        _d3d12_rbk->queries.dlssg.pReadBack->Unmap (0, &range);

        if ( UINT64      sync_value = compute.dlssg_fence.value + 1;
             SUCCEEDED (This->Signal (compute.dlssg_fence,
                         sync_value))
           )
        {
          compute.dlssg_fence.value =
                         sync_value;
        }
      }
    }
  }
}

void
_InstallCommandQueueHooksImpl (ID3D12Device* pDevice12)
{
  assert (pDevice12 != nullptr);
  if (    pDevice12 == nullptr)
    return;

  const bool bHasStreamline =
    SK_IsModuleLoaded (L"sl.interposer.dll");

  SK_ComPtr <ID3D12Device> pDev12;

  if (bHasStreamline)
  {
    if (SK_slGetNativeInterface (pDevice12, (void **)&pDev12.p) == sl::Result::eOk)
      SK_LOGi0 (L"Hooking Streamline Native Interface for ID3D12CommandQueue...");
    else pDev12 = pDevice12;
  } else pDev12 = pDevice12;

  SK_ComPtr < ID3D12CommandQueue > p12Queue;
  D3D12_COMMAND_QUEUE_DESC queue_desc = { };

  if (SUCCEEDED (pDev12->CreateCommandQueue (
                          &queue_desc,
                    IID_PPV_ARGS (&p12Queue.p))))
  {
    SK_CreateVFTableHook2 ( L"ID3D12CommandQueue::ExecuteCommandLists",
                             *(void ***)*(&p12Queue.p), 10,
                              D3D12CommandQueue_ExecuteCommandLists_Detour,
                    (void **)&D3D12CommandQueue_ExecuteCommandLists_Original );
  }
}

bool
SK_D3D12_InstallCommandQueueHooks (ID3D12Device *pDev12)
{
  static bool s_Init = false;

  // Check the status of hooks
  if (pDev12 == nullptr)
    return s_Init;

  // Actually install hooks... once.
  if (! std::exchange (s_Init, true))
  {
    _InstallCommandQueueHooksImpl (pDev12);

    return true;
  }

  return false;
}