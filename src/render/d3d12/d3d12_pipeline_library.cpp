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
#define __SK_SUBSYSTEM__ L"D12PipeLib"

#include <SpecialK/render/d3d12/d3d12_pipeline_library.h>
HRESULT
STDMETHODCALLTYPE
D3D12Device1_CreatePipelineLibrary_Detour (
        ID3D12Device1 *This,
  const void   *pLibraryBlob,
        SIZE_T          BlobLength,
        REFIID          riid,
        void       **ppPipelineLibrary )
{
  SK_LOG_FIRST_CALL

  HRESULT hr =
    D3D12Device1_CreatePipelineLibrary_Original (
                        This, pLibraryBlob,
                                      BlobLength, riid,
                     ppPipelineLibrary );

  if (FAILED (hr))
  {
    BlobLength = 0;

    SK_LOG0 ( ( L"CreatePipelineLibrary failed with HRESULT=%x", hr ),
                __SK_SUBSYSTEM__ );

    if ( hr == D3D12_ERROR_ADAPTER_NOT_FOUND ||
         hr == D3D12_ERROR_DRIVER_VERSION_MISMATCH )
    {
      SK_LOG0 ( ( L" * Recovering from Invalid Cache => initializing empty..." ),
                  __SK_SUBSYSTEM__ );

      return
        D3D12Device1_CreatePipelineLibrary_Original (
                            This, pLibraryBlob,
                                             0, riid,
                         ppPipelineLibrary );
    }
  }

  return hr;
}

void
SK_D3D12_HookPipelineLibrary (ID3D12Device1* pDevice1)
{
  // ID3D12Device1
  //---------------
  // 44 CreatePipelineLibrary
  // 45 SetEventOnMultipleFenceCompletion
  // 46 SetResidencyPriority

  SK_CreateVFTableHook2 ( L"ID3D12Device1::CreatePipelineLibrary",
                         *(void ***)*(&pDevice1), 44,
                          D3D12Device1_CreatePipelineLibrary_Detour,
                (void **)&D3D12Device1_CreatePipelineLibrary_Original );
}