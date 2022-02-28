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

void STDMETHODCALLTYPE
D3D12CommandQueue_ExecuteCommandLists_Detour (
  ID3D12CommandQueue        *This,
  UINT                      NumCommandLists,
  ID3D12CommandList* const  *ppCommandLists )
{
  static bool          once = false;
  if (! std::exchange (once,  true))
  {
    static auto& rb =
      SK_GetCurrentRenderBackend ();

    SK_ComPtr <ID3D12Device> pDevice12;

    D3D12_COMMAND_QUEUE_DESC
         queueDesc  =  This->GetDesc ();
    if ( queueDesc.Type == D3D12_COMMAND_LIST_TYPE_DIRECT &&
         SUCCEEDED (   This->GetDevice (
                         IID_ID3D12Device,
                        (void **)&pDevice12.p
                                       ) // We are not holding a ref, so test pointers before using
                   ) && SK_ValidatePointer                         (pLazyD3D12Device,      true) &&
                        SK_IsAddressExecutable ((*(const void***)*(&pLazyD3D12Device))[0], true) &&
                        pDevice12.IsEqualObject (
               pLazyD3D12Device                 )
       )
    {
      if (rb.d3d12.command_queue == nullptr)
      {
        // Now we are holding a ref...
        rb.setDevice            (pLazyD3D12Device);
        rb.swapchain           = pLazyD3D12Chain;
        rb.d3d12.command_queue = This;
        rb.api                 = SK_RenderAPI::D3D12;

        _d3d12_rbk->init (
          (IDXGISwapChain3 *)pLazyD3D12Chain,
            This
        );
      }
    }

    else once = false;
  }


  if (once)
  {
    if ( ReadULong64Acquire (&SK_Reflex_LastFrameMarked) <
         ReadULong64Acquire (&SK_RenderBackend::frames_drawn) )
    {
      static auto& rb =
       SK_GetCurrentRenderBackend ();

      WriteULong64Release (
        &SK_Reflex_LastFrameMarked,
         ReadULong64Acquire (&SK_RenderBackend::frames_drawn)
      );

      rb.setLatencyMarkerNV (RENDERSUBMIT_START);
    }
  }


  return
    D3D12CommandQueue_ExecuteCommandLists_Original (
      This,
        NumCommandLists,
         ppCommandLists
    );
}

void
SK_D3D12_InstallCommandQueueHooks (ID3D12Device *pDev12)
{
  assert (pDev12 != nullptr);

  if (pDev12 == nullptr)
    return;

  SK_ComPtr < ID3D12CommandQueue > p12Queue;
  D3D12_COMMAND_QUEUE_DESC queue_desc = { };

  static bool once = false;
  if         (once) return;

  if (SUCCEEDED (pDev12->CreateCommandQueue (
                          &queue_desc,
                    IID_PPV_ARGS (&p12Queue.p))))
  {
    SK_CreateVFTableHook2 ( L"ID3D12CommandQueue::ExecuteCommandLists",
                             *(void ***)*(&p12Queue.p), 10,
                              D3D12CommandQueue_ExecuteCommandLists_Detour,
                    (void **)&D3D12CommandQueue_ExecuteCommandLists_Original );

    once = true;
  }
}