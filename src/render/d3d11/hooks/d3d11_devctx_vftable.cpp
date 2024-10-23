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

#define __SK_SUBSYSTEM__ L"  D3D 11  "

#include <SpecialK/render/d3d11/d3d11_tex_mgr.h>
#include <SpecialK/render/d3d11/d3d11_state_tracker.h>
#include <SpecialK/render/d3d11/utility/d3d11_texture.h>
#include <SpecialK/render/dxgi/dxgi_util.h>

D3D11Dev_CreateRasterizerState_pfn                  D3D11Dev_CreateRasterizerState_Original                  = nullptr;
D3D11Dev_CreateSamplerState_pfn                     D3D11Dev_CreateSamplerState_Original                     = nullptr;
D3D11Dev_CreateBuffer_pfn                           D3D11Dev_CreateBuffer_Original                           = nullptr;
D3D11Dev_CreateTexture2D_pfn                        D3D11Dev_CreateTexture2D_Original                        = nullptr;
D3D11Dev_CreateTexture2D1_pfn                       D3D11Dev_CreateTexture2D1_Original                       = nullptr;
D3D11Dev_CreateRenderTargetView_pfn                 D3D11Dev_CreateRenderTargetView_Original                 = nullptr;
D3D11Dev_CreateRenderTargetView1_pfn                D3D11Dev_CreateRenderTargetView1_Original                = nullptr;
D3D11Dev_CreateShaderResourceView_pfn               D3D11Dev_CreateShaderResourceView_Original               = nullptr;
D3D11Dev_CreateShaderResourceView1_pfn              D3D11Dev_CreateShaderResourceView1_Original              = nullptr;
D3D11Dev_CreateDepthStencilView_pfn                 D3D11Dev_CreateDepthStencilView_Original                 = nullptr;
D3D11Dev_CreateUnorderedAccessView_pfn              D3D11Dev_CreateUnorderedAccessView_Original              = nullptr;
D3D11Dev_CreateUnorderedAccessView1_pfn             D3D11Dev_CreateUnorderedAccessView1_Original             = nullptr;

D3D11Dev_CreateVertexShader_pfn                     D3D11Dev_CreateVertexShader_Original                     = nullptr;
D3D11Dev_CreatePixelShader_pfn                      D3D11Dev_CreatePixelShader_Original                      = nullptr;
D3D11Dev_CreateGeometryShader_pfn                   D3D11Dev_CreateGeometryShader_Original                   = nullptr;
D3D11Dev_CreateGeometryShaderWithStreamOutput_pfn   D3D11Dev_CreateGeometryShaderWithStreamOutput_Original   = nullptr;
D3D11Dev_CreateHullShader_pfn                       D3D11Dev_CreateHullShader_Original                       = nullptr;
D3D11Dev_CreateDomainShader_pfn                     D3D11Dev_CreateDomainShader_Original                     = nullptr;
D3D11Dev_CreateComputeShader_pfn                    D3D11Dev_CreateComputeShader_Original                    = nullptr;

D3D11Dev_CreateDeferredContext_pfn                  D3D11Dev_CreateDeferredContext_Original                  = nullptr;
D3D11Dev_CreateDeferredContext1_pfn                 D3D11Dev_CreateDeferredContext1_Original                 = nullptr;
D3D11Dev_CreateDeferredContext2_pfn                 D3D11Dev_CreateDeferredContext2_Original                 = nullptr;
D3D11Dev_CreateDeferredContext3_pfn                 D3D11Dev_CreateDeferredContext3_Original                 = nullptr;
D3D11Dev_GetImmediateContext_pfn                    D3D11Dev_GetImmediateContext_Original                    = nullptr;
D3D11Dev_GetImmediateContext1_pfn                   D3D11Dev_GetImmediateContext1_Original                   = nullptr;
D3D11Dev_GetImmediateContext2_pfn                   D3D11Dev_GetImmediateContext2_Original                   = nullptr;
D3D11Dev_GetImmediateContext3_pfn                   D3D11Dev_GetImmediateContext3_Original                   = nullptr;

D3D11_ClearState_pfn                                D3D11_ClearState_Original                                = nullptr;
D3D11_ExecuteCommandList_pfn                        D3D11_ExecuteCommandList_Original                        = nullptr;
D3D11_FinishCommandList_pfn                         D3D11_FinishCommandList_Original                         = nullptr;

D3D11_RSSetScissorRects_pfn                         D3D11_RSSetScissorRects_Original                         = nullptr;
D3D11_RSSetViewports_pfn                            D3D11_RSSetViewports_Original                            = nullptr;
D3D11_VSSetConstantBuffers_pfn                      D3D11_VSSetConstantBuffers_Original                      = nullptr;
D3D11_VSSetShaderResources_pfn                      D3D11_VSSetShaderResources_Original                      = nullptr;
D3D11_PSSetShaderResources_pfn                      D3D11_PSSetShaderResources_Original                      = nullptr;
D3D11_PSSetConstantBuffers_pfn                      D3D11_PSSetConstantBuffers_Original                      = nullptr;
D3D11_GSSetShaderResources_pfn                      D3D11_GSSetShaderResources_Original                      = nullptr;
D3D11_HSSetShaderResources_pfn                      D3D11_HSSetShaderResources_Original                      = nullptr;
D3D11_DSSetShaderResources_pfn                      D3D11_DSSetShaderResources_Original                      = nullptr;
D3D11_CSSetShaderResources_pfn                      D3D11_CSSetShaderResources_Original                      = nullptr;
D3D11_CSSetUnorderedAccessViews_pfn                 D3D11_CSSetUnorderedAccessViews_Original                 = nullptr;
D3D11_UpdateSubresource_pfn                         D3D11_UpdateSubresource_Original                         = nullptr;
D3D11_DrawIndexed_pfn                               D3D11_DrawIndexed_Original                               = nullptr;
D3D11_Draw_pfn                                      D3D11_Draw_Original                                      = nullptr;
D3D11_DrawAuto_pfn                                  D3D11_DrawAuto_Original                                  = nullptr;
D3D11_DrawIndexedInstanced_pfn                      D3D11_DrawIndexedInstanced_Original                      = nullptr;
D3D11_DrawIndexedInstancedIndirect_pfn              D3D11_DrawIndexedInstancedIndirect_Original              = nullptr;
D3D11_DrawInstanced_pfn                             D3D11_DrawInstanced_Original                             = nullptr;
D3D11_DrawInstancedIndirect_pfn                     D3D11_DrawInstancedIndirect_Original                     = nullptr;
D3D11_Dispatch_pfn                                  D3D11_Dispatch_Original                                  = nullptr;
D3D11_DispatchIndirect_pfn                          D3D11_DispatchIndirect_Original                          = nullptr;
D3D11_Map_pfn                                       D3D11_Map_Original                                       = nullptr;
D3D11_Unmap_pfn                                     D3D11_Unmap_Original                                     = nullptr;

D3D11_OMSetRenderTargets_pfn                        D3D11_OMSetRenderTargets_Original                        = nullptr;
D3D11_OMSetRenderTargetsAndUnorderedAccessViews_pfn D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Original = nullptr;
D3D11_OMGetRenderTargets_pfn                        D3D11_OMGetRenderTargets_Original                        = nullptr;
D3D11_OMGetRenderTargetsAndUnorderedAccessViews_pfn D3D11_OMGetRenderTargetsAndUnorderedAccessViews_Original = nullptr;
D3D11_ClearDepthStencilView_pfn                     D3D11_ClearDepthStencilView_Original                     = nullptr;

D3D11_PSSetSamplers_pfn                             D3D11_PSSetSamplers_Original                             = nullptr;

D3D11_VSSetShader_pfn                               D3D11_VSSetShader_Original                               = nullptr;
D3D11_PSSetShader_pfn                               D3D11_PSSetShader_Original                               = nullptr;
D3D11_GSSetShader_pfn                               D3D11_GSSetShader_Original                               = nullptr;
D3D11_HSSetShader_pfn                               D3D11_HSSetShader_Original                               = nullptr;
D3D11_DSSetShader_pfn                               D3D11_DSSetShader_Original                               = nullptr;
D3D11_CSSetShader_pfn                               D3D11_CSSetShader_Original                               = nullptr;

D3D11_VSGetShader_pfn                               D3D11_VSGetShader_Original                               = nullptr;
D3D11_PSGetShader_pfn                               D3D11_PSGetShader_Original                               = nullptr;
D3D11_GSGetShader_pfn                               D3D11_GSGetShader_Original                               = nullptr;
D3D11_HSGetShader_pfn                               D3D11_HSGetShader_Original                               = nullptr;
D3D11_DSGetShader_pfn                               D3D11_DSGetShader_Original                               = nullptr;
D3D11_CSGetShader_pfn                               D3D11_CSGetShader_Original                               = nullptr;

D3D11_GetData_pfn                                   D3D11_GetData_Original                                   = nullptr;

D3D11_CopyResource_pfn                              D3D11_CopyResource_Original                              = nullptr;
D3D11_CopySubresourceRegion_pfn                     D3D11_CopySubresourceRegion_Original                     = nullptr;
D3D11_UpdateSubresource1_pfn                        D3D11_UpdateSubresource1_Original                        = nullptr;
D3D11_ResolveSubresource_pfn                        D3D11_ResolveSubresource_Original                        = nullptr;

//   0 QueryInterface
//   1 AddRef
//   2 Release

//   3 GetDevice
//   4 GetPrivateData
//   5 SetPrivateData
//   6 SetPrivateDataInterface

//   7 VSSetConstantBuffers
//   8 PSSetShaderResources
//   9 PSSetShader
//  10 PSSetSamplers
//  11 VSSetShader
//  12 DrawIndexed
//  13 Draw
//  14 Map
//  15 Unmap
//  16 PSSetConstantBuffers
//  17 IASetInputLayout
//  18 IASetVertexBuffers
//  19 IASetIndexBuffer
//  20 DrawIndexedInstanced
//  21 DrawInstanced
//  22 GSSetConstantBuffers
//  23 GSSetShader
//  24 IASetPrimitiveTopology
//  25 VSSetShaderResources
//  26 VSSetSamplers
//  27 Begin
//  28 End
//  29 GetData
//  30 SetPredication
//  31 GSSetShaderResources
//  32 GSSetSamplers
//  33 OMSetRenderTargets
//  34 OMSetRenderTargetsAndUnorderedAccessViews
//  35 OMSetBlendState
//  36 OMSetDepthStencilState
//  37 SOSetTargets
//  38 DrawAuto
//  39 DrawIndexedInstancedIndirect
//  40 DrawInstancedIndirect
//  41 Dispatch
//  42 DispatchIndirect
//  43 RSSetState
//  44 RSSetViewports
//  45 RSSetScissorRects
//  46 CopySubresourceRegion
//  47 CopyResource
//  48 UpdateSubresource
//  49 CopyStructureCount
//  50 ClearRenderTargetView
//  51 ClearUnorderedAccessViewUint
//  52 ClearUnorderedAccessViewFloat
//  53 ClearDepthStencilView
//  54 GenerateMips
//  55 SetResourceMinLOD
//  56 GetResourceMinLOD
//  57 ResolveSubresource
//  58 ExecuteCommandList
//  59 HSSetShaderResources
//  60 HSSetShader
//  61 HSSetSamplers
//  62 HSSetConstantBuffers
//  63 DSSetShaderResources
//  64 DSSetShader
//  65 DSSetSamplers
//  66 DSSetConstantBuffers
//  67 CSSetShaderResources
//  68 CSSetUnorderedAccessViews
//  69 CSSetShader
//  70 CSSetSamplers
//  71 CSSetConstantBuffers
//  72 VSGetConstantBuffers
//  73 PSGetShaderResources
//  74 PSGetShader
//  75 PSGetSamplers
//  76 VSGetShader
//  77 PSGetConstantBuffers
//  78 IAGetInputLayout
//  79 IAGetVertexBuffers
//  80 IAGetIndexBuffer
//  81 GSGetConstantBuffers
//  82 GSGetShader
//  83 IAGetPrimitiveTopology
//  84 VSGetShaderResources
//  85 VSGetSamplers
//  86 GetPredication
//  87 GSGetShaderResources
//  88 GSGetSamplers
//  89 OMGetRenderTargets
//  90 OMGetRenderTargetsAndUnorderedAccessViews
//  91 OMGetBlendState
//  92 OMGetDepthStencilState
//  93 SOGetTargets
//  94 RSGetState
//  95 RSGetViewports
//  96 RSGetScissorRects
//  97 HSGetShaderResources
//  98 HSGetShader
//  99 HSGetSamplers
// 100 HSGetConstantBuffers
// 101 DSGetShaderResources
// 102 DSGetShader
// 103 DSGetSamplers
// 104 DSGetConstantBuffers
// 105 CSGetShaderResources
// 106 CSGetUnorderedAccessViews
// 107 CSGetShader
// 108 CSGetSamplers
// 109 CSGetConstantBuffers
// 110 ClearState
// 111 Flush
// 112 GetType
// 113 GetContextFlags
// 114 FinishCommandList


// TODO
//////#include <../src/render/d3d11/d3d11_dev_ctx.cpp>

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11_GetData_Override (
 _In_  ID3D11DeviceContext *This,
 _In_  ID3D11Asynchronous  *pAsync,
 _Out_writes_bytes_opt_   ( DataSize )
       void                *pData,
 _In_  UINT                 DataSize,
 _In_  UINT                 GetDataFlags )
{
  if (pAsync == nullptr)
    return E_INVALIDARG;

  HRESULT hr =
    D3D11_GetData_Original (
    This,    pAsync,
      pData, DataSize,
          GetDataFlags     );

#if 1
  return hr;
#else
  dll_log->Log (L"Query Type: %lu, Misc Flags: %x - Result: %x, Value: %llu", qDesc.Query,
                qDesc.MiscFlags, hr, *(uint64_t *)pData);

  dll_log->Log (L"Query - Size: %lu", DataSize);

  if (pCounter)
  {
    dll_log->Log (L"Counter - Size: %lu", DataSize);
  }
  auto isDataReady =
   [&](void)
     -> bool
        {
          HRESULT hr =
            D3D11_GetData_Original (
              This, pAsync,
                    nullptr, 0,
                             GetDataFlags |
                             D3D11_ASYNC_GETDATA_DONOTFLUSH
                                   );
          return
            ( SUCCEEDED (hr) &&
                         hr  != S_FALSE );
        };


  auto finishGetData =
   [&](void)
     -> HRESULT
        {
          return
            D3D11_GetData_Original (
              This,    pAsync,
                pData, DataSize,
                    GetDataFlags
                                   );
        };

  extern bool
         __SK_DQXI_MakeAsyncObjectsGreatAgain;
  if ((! __SK_DQXI_MakeAsyncObjectsGreatAgain) ||
       (pData == nullptr && DataSize == 0)     ||
                           isDataReady ())
  {
    return
      finishGetData ();
  }

  int spins = 0;

  do
  {
    YieldProcessor ();

    if (++spins > 3)
    {
      break;
    }
  } while (! isDataReady ());

  return
    finishGetData ();
#endif
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_VSSetShader_Override (
 _In_     ID3D11DeviceContext        *This,
 _In_opt_ ID3D11VertexShader         *pVertexShader,
 _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
          UINT                        NumClassInstances )
{
  if (ppClassInstances != nullptr && NumClassInstances > 256)
  {
    SK_ReleaseAssert (!"Too many class instances, is hook corrupted?");
  }

  else if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    if (ppClassInstances == nullptr)
       NumClassInstances = 0;

    return
      SK_D3D11_SetShader_Impl (
        This,           pVertexShader,
        sk_shader_class::Vertex,
               ppClassInstances,
              NumClassInstances );
  }

  D3D11_VSSetShader_Original
  (   This,
     pVertexShader,
    ppClassInstances,
   NumClassInstances
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_VSGetShader_Override (
 _In_        ID3D11DeviceContext  *This,
 _Out_       ID3D11VertexShader  **ppVertexShader,
 _Out_opt_   ID3D11ClassInstance **ppClassInstances,
 _Inout_opt_ UINT                 *pNumClassInstances )
{
  if (ppVertexShader == nullptr)
    return;

  return
    D3D11_VSGetShader_Original ( This,
      ppVertexShader, ppClassInstances,
                    pNumClassInstances
    );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_PSSetShader_Override (
 _In_     ID3D11DeviceContext        *This,
 _In_opt_ ID3D11PixelShader          *pPixelShader,
 _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
          UINT                        NumClassInstances )
{
  if (ppClassInstances != nullptr && NumClassInstances > 256)
  {
    SK_ReleaseAssert (!"Too many class instances, is hook corrupted?");
  }

  else if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    if (ppClassInstances == nullptr)
       NumClassInstances = 0;

    return
      SK_D3D11_SetShader_Impl (
        This,           pPixelShader,
        sk_shader_class::Pixel,
                ppClassInstances,
               NumClassInstances );
  }

  D3D11_PSSetShader_Original
  (   This,
     pPixelShader,
    ppClassInstances,
   NumClassInstances
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_PSGetShader_Override (
 _In_        ID3D11DeviceContext  *This,
 _Out_       ID3D11PixelShader   **ppPixelShader,
 _Out_opt_   ID3D11ClassInstance **ppClassInstances,
 _Inout_opt_ UINT                 *pNumClassInstances )
{
  if (ppPixelShader == nullptr)
    return;

  return
    D3D11_PSGetShader_Original ( This,
      ppPixelShader, ppClassInstances,
                   pNumClassInstances
    );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_GSSetShader_Override (
 _In_     ID3D11DeviceContext        *This,
 _In_opt_ ID3D11GeometryShader       *pGeometryShader,
 _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
          UINT                        NumClassInstances )
{
  if (ppClassInstances != nullptr && NumClassInstances > 256)
  {
    SK_ReleaseAssert (!"Too many class instances, is hook corrupted?");
  }

  else if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    if (ppClassInstances == nullptr)
       NumClassInstances = 0;

    return
      SK_D3D11_SetShader_Impl (
        This,           pGeometryShader,
        sk_shader_class::Geometry,
                ppClassInstances,
               NumClassInstances );
  }

  D3D11_GSSetShader_Original
  (   This,
     pGeometryShader,
    ppClassInstances,
   NumClassInstances
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_GSGetShader_Override (
 _In_        ID3D11DeviceContext   *This,
 _Out_       ID3D11GeometryShader **ppGeometryShader,
 _Out_opt_   ID3D11ClassInstance  **ppClassInstances,
 _Inout_opt_ UINT                  *pNumClassInstances )
{
  if (ppGeometryShader == nullptr)
    return;

  return
    D3D11_GSGetShader_Original ( This,
      ppGeometryShader, ppClassInstances,
                      pNumClassInstances
    );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_HSSetShader_Override (
 _In_     ID3D11DeviceContext        *This,
 _In_opt_ ID3D11HullShader           *pHullShader,
 _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
          UINT                        NumClassInstances )
{
  if (ppClassInstances != nullptr && NumClassInstances > 256)
  {
    SK_ReleaseAssert (!"Too many class instances, is hook corrupted?");
  }

  else if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    if (ppClassInstances == nullptr)
       NumClassInstances = 0;

    return
      SK_D3D11_SetShader_Impl (
        This,           pHullShader,
        sk_shader_class::Hull,
                ppClassInstances,
               NumClassInstances );

  }

  D3D11_HSSetShader_Original
  (   This,
     pHullShader,
    ppClassInstances,
   NumClassInstances
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_HSGetShader_Override (
 _In_        ID3D11DeviceContext  *This,
 _Out_       ID3D11HullShader    **ppHullShader,
 _Out_opt_   ID3D11ClassInstance **ppClassInstances,
 _Inout_opt_ UINT                 *pNumClassInstances )
{
  if (ppHullShader == nullptr)
    return;

  return
    D3D11_HSGetShader_Original ( This,
         ppHullShader,
         ppClassInstances,
       pNumClassInstances
    );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DSSetShader_Override (
 _In_     ID3D11DeviceContext        *This,
 _In_opt_ ID3D11DomainShader         *pDomainShader,
 _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
          UINT                        NumClassInstances )
{
  if (ppClassInstances != nullptr && NumClassInstances > 256)
  {
    SK_ReleaseAssert (!"Too many class instances, is hook corrupted?");
  }

  else if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    if (ppClassInstances == nullptr)
       NumClassInstances = 0;

    return
      SK_D3D11_SetShader_Impl (
        This,           pDomainShader,
        sk_shader_class::Domain,
                ppClassInstances,
               NumClassInstances );
  }

  D3D11_DSSetShader_Original
  (   This,
     pDomainShader,
    ppClassInstances,
   NumClassInstances
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DSGetShader_Override (
 _In_        ID3D11DeviceContext  *This,
 _Out_       ID3D11DomainShader  **ppDomainShader,
 _Out_opt_   ID3D11ClassInstance **ppClassInstances,
 _Inout_opt_ UINT                 *pNumClassInstances )
{
  if (ppDomainShader == nullptr)
    return;

  return D3D11_DSGetShader_Original ( This,
    ppDomainShader, ppClassInstances,
                  pNumClassInstances
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CSSetShader_Override (
 _In_     ID3D11DeviceContext        *This,
 _In_opt_ ID3D11ComputeShader        *pComputeShader,
 _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
          UINT                        NumClassInstances )
{
  if (ppClassInstances != nullptr && NumClassInstances > 256)
  {
    SK_ReleaseAssert (!"Too many class instances, is hook corrupted?");
  }

  else if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    if (ppClassInstances == nullptr)
       NumClassInstances = 0;

    return
      SK_D3D11_SetShader_Impl (
        This,           pComputeShader,
        sk_shader_class::Compute,
                ppClassInstances,
               NumClassInstances );
  }

  D3D11_CSSetShader_Original
  (   This,
     pComputeShader,
    ppClassInstances,
   NumClassInstances
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CSGetShader_Override (
 _In_        ID3D11DeviceContext  *This,
 _Out_       ID3D11ComputeShader **ppComputeShader,
 _Out_opt_   ID3D11ClassInstance **ppClassInstances,
 _Inout_opt_ UINT                 *pNumClassInstances )
{
  if (ppComputeShader == nullptr)
    return;

  return
    D3D11_CSGetShader_Original ( This, ppComputeShader,
                                   ppClassInstances, pNumClassInstances );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_ClearState_Override (ID3D11DeviceContext* This)
{
  SK_LOG_FIRST_CALL

  D3D11_ClearState_Original (This);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_ExecuteCommandList_Override (
    _In_  ID3D11DeviceContext *This,
    _In_  ID3D11CommandList   *pCommandList,
          BOOL                 RestoreContextState )
{
  if (pCommandList == nullptr)
    return;

  SK_ComPtr <ID3D11Device> pDevice;
  This->GetDevice (       &pDevice.p);

  if (pDevice.IsEqualObject (SK_GetCurrentRenderBackend ().device))
  {

    SK_LOG_FIRST_CALL

    SK_ComPtr <ID3D11DeviceContext>
         pBuildContext (nullptr);
    UINT  size        =        sizeof (LPVOID);

    // Fix for Yakuza0, why the hell is it passing nullptr?!
    if (pCommandList == nullptr)
    {
      D3D11_ExecuteCommandList_Original (
        This,
          nullptr,
            RestoreContextState
      );

      if (RestoreContextState == FALSE)
      {
        SK_D3D11_ResetContextState
        (
          This, SK_D3D11_GetDeviceContextHandle (This)
        );
      }

      return;
    }


    // Broken
#if 0
    if ( SUCCEEDED (
      pCommandList->GetPrivateData (
      SKID_D3D11DeviceContextOrigin,
         &size,   &pBuildContext.p )
       )           )
    {
      if (! pBuildContext.IsEqualObject (This))
      {
        SK_D3D11_MergeCommandLists (
          pBuildContext,
            This
        );
      }

      pCommandList->SetPrivateDataInterface (SKID_D3D11DeviceContextOrigin, nullptr);
    }
#else
    UNREFERENCED_PARAMETER (size);
#endif


    D3D11_ExecuteCommandList_Original ( This,
      pCommandList,
          RestoreContextState
    );

    if (RestoreContextState == FALSE)
    {
      SK_D3D11_ResetContextState (
        This, SK_D3D11_GetDeviceContextHandle (This)
      );
    }
  }

  else
  {
    D3D11_ExecuteCommandList_Original ( This,
      pCommandList,
          RestoreContextState
    );
  }
}

HRESULT
STDMETHODCALLTYPE
D3D11_FinishCommandList_Override (
            ID3D11DeviceContext  *This,
            BOOL                  RestoreDeferredContextState,
  _Out_opt_ ID3D11CommandList   **ppCommandList )
{
  SK_ComPtr <ID3D11Device> pDevice;
  This->GetDevice (       &pDevice.p);

  if (pDevice.IsEqualObject (SK_GetCurrentRenderBackend ().device))
  {
    SK_LOG_FIRST_CALL

    HRESULT hr =
      D3D11_FinishCommandList_Original
          (This, RestoreDeferredContextState, ppCommandList);

    if (SUCCEEDED (hr) && (ppCommandList               != nullptr &&
                          (RestoreDeferredContextState == FALSE)))
    {
      (*ppCommandList)->SetPrivateDataInterface ( SKID_D3D11DeviceContextOrigin, This );
    }

    return hr;
  }

  return
    D3D11_FinishCommandList_Original
        (This, RestoreDeferredContextState, ppCommandList);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_RSSetScissorRects_Override (
                 ID3D11DeviceContext *This,
  _In_           UINT                 NumRects,
  _In_opt_ const D3D11_RECT          *pRects )
{
  if (pRects == nullptr)
    return D3D11_RSSetScissorRects_Original (This, NumRects, pRects);

  static const auto game_id =
        SK_GetCurrentGameID ();

#ifdef _M_AMD64
  switch (game_id)
  {
    case SK_GAME_ID::GalGunReturns:
    {
      if (! config.window.res.override.isZero ())
      {
        if (NumRects == 0)
            NumRects  = 1;

        if (NumRects        == 1    &&
            ((pRects->right == 1920 ||
              pRects->right == 480) ||
             (pRects->right < static_cast <LONG> (config.window.res.override.x) &&
              pRects->right > static_cast <LONG> (config.window.res.override.x) * 0.97f) ) )
        {
          D3D11_RECT rectNew = *pRects;

          if (rectNew.right == 480)
          {
            rectNew.right  = static_cast <LONG> (config.window.res.override.x / 4);
            rectNew.bottom = static_cast <LONG> (config.window.res.override.y / 4);
          }

          else
          {
            rectNew.right  = static_cast <LONG> (config.window.res.override.x);
            rectNew.bottom = static_cast <LONG> (config.window.res.override.y);
          }

          return
            D3D11_RSSetScissorRects_Original (
              This, NumRects, &rectNew
            );
        }
      }
    } break;

    case SK_GAME_ID::ShinMegamiTensei3:
    {
      if (NumRects == 1)
      {
      //SK_LOGi0 ( L"Scissor Left: %d, Right: %d - Top: %d, Bottom: %d",
      //               pRects->left, pRects->right, pRects->top, pRects->bottom );

        if (pRects->right == 1280)
        {
          const SK_RenderBackend& rb =
            SK_GetCurrentRenderBackend ();
        
          SK_ComQIPtr <IDXGISwapChain>
                   pSwapChain (
                 rb.swapchain );
          DXGI_SWAP_CHAIN_DESC  swapDesc = { };
        
          if (pSwapChain != nullptr)
          {   pSwapChain->GetDesc (&swapDesc);
        
            D3D11_RECT rectNew = *pRects;
        
            rectNew.right  = swapDesc.BufferDesc.Width;
            rectNew.bottom = swapDesc.BufferDesc.Height;
        
            return
              D3D11_RSSetScissorRects_Original (
                This, NumRects, &rectNew
              );
          }
        }

        else
        if (pRects->right == 1080 && pRects->bottom == 720)
        {
          D3D11_RECT rectNew = *pRects;

            rectNew.right  *= 2;
            rectNew.bottom *= 2;

            return
              D3D11_RSSetScissorRects_Original (
                This, NumRects, &rectNew
              );
        }

      //if (pRects->right == 512 && pRects->bottom == 512)
      //{
      //  D3D11_RECT rectNew = *pRects;
      //
      //    rectNew.right  = 8192;
      //    rectNew.bottom = 8192;
      //
      //    return
      //      D3D11_RSSetScissorRects_Original (
      //        This, NumRects, &rectNew
      //      );
      //}
      }
    } break;
  }
#endif

  return
    D3D11_RSSetScissorRects_Original (This, NumRects, pRects);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_VSSetConstantBuffers_Override (
           ID3D11DeviceContext*  This,
  _In_     UINT                  StartSlot,
  _In_     UINT                  NumBuffers,
  _In_opt_ ID3D11Buffer *const  *ppConstantBuffers )
{
  //dll_log->Log (L"[   DXGI   ] [!]D3D11_VSSetConstantBuffers (%lu, %lu, ...)", StartSlot, NumBuffers);
  return
    D3D11_VSSetConstantBuffers_Original (
      This, StartSlot,
                    NumBuffers,
             ppConstantBuffers
    );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_PSSetConstantBuffers_Override (
           ID3D11DeviceContext*  This,
  _In_     UINT                  StartSlot,
  _In_     UINT                  NumBuffers,
  _In_opt_ ID3D11Buffer *const  *ppConstantBuffers )
{
  //dll_log->Log (L"[   DXGI   ] [!]D3D11_VSSetConstantBuffers (%lu, %lu, ...)", StartSlot, NumBuffers);
  return
    D3D11_PSSetConstantBuffers_Original (This, StartSlot, NumBuffers, ppConstantBuffers );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_VSSetShaderResources_Override (
  _In_     ID3D11DeviceContext             *This,
  _In_     UINT                             StartSlot,
  _In_     UINT                             NumViews,
  _In_opt_ ID3D11ShaderResourceView* const *ppShaderResourceViews )
{
  if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    return
      SK_D3D11_SetShaderResources_Impl(
           SK_D3D11_ShaderType::Vertex,
                                 FALSE,
                                  This,
                               nullptr,
                             StartSlot,
                              NumViews,
                 ppShaderResourceViews);
  }

  D3D11_VSSetShaderResources_Original ( This,
             StartSlot, NumViews,
           ppShaderResourceViews
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_PSSetShaderResources_Override (
  _In_     ID3D11DeviceContext             *This,
  _In_     UINT                             StartSlot,
  _In_     UINT                             NumViews,
  _In_opt_ ID3D11ShaderResourceView* const *ppShaderResourceViews )
{
  if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    return
      SK_D3D11_SetShaderResources_Impl(
            SK_D3D11_ShaderType::Pixel,
                                 FALSE,
                                  This,
                               nullptr,
                             StartSlot,
                              NumViews,
                 ppShaderResourceViews);
  }

  D3D11_PSSetShaderResources_Original ( This,
      StartSlot, NumViews,
    ppShaderResourceViews
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_GSSetShaderResources_Override (
  _In_     ID3D11DeviceContext             *This,
  _In_     UINT                             StartSlot,
  _In_     UINT                             NumViews,
  _In_opt_ ID3D11ShaderResourceView* const *ppShaderResourceViews )
{
  if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    return
      SK_D3D11_SetShaderResources_Impl(
         SK_D3D11_ShaderType::Geometry,
                                 FALSE,
                                  This,
                               nullptr,
                             StartSlot,
                              NumViews,
                 ppShaderResourceViews);
  }

  D3D11_GSSetShaderResources_Original ( This,
             StartSlot, NumViews,
           ppShaderResourceViews
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_HSSetShaderResources_Override (
  _In_     ID3D11DeviceContext             *This,
  _In_     UINT                             StartSlot,
  _In_     UINT                             NumViews,
  _In_opt_ ID3D11ShaderResourceView* const *ppShaderResourceViews )
{
  if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    return
      SK_D3D11_SetShaderResources_Impl(
             SK_D3D11_ShaderType::Hull,
                                 FALSE,
                                  This,
                               nullptr,
                             StartSlot,
                              NumViews,
                 ppShaderResourceViews);
  }

  D3D11_HSSetShaderResources_Original ( This,
             StartSlot, NumViews,
           ppShaderResourceViews
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DSSetShaderResources_Override (
  _In_     ID3D11DeviceContext             *This,
  _In_     UINT                             StartSlot,
  _In_     UINT                             NumViews,
  _In_opt_ ID3D11ShaderResourceView* const *ppShaderResourceViews )
{
  if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    return
      SK_D3D11_SetShaderResources_Impl(
           SK_D3D11_ShaderType::Domain,
                                 FALSE,
                                  This,
                               nullptr,
                             StartSlot,
                              NumViews,
                 ppShaderResourceViews);
  }

  D3D11_DSSetShaderResources_Original ( This,
             StartSlot, NumViews,
           ppShaderResourceViews
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CSSetShaderResources_Override (
  _In_     ID3D11DeviceContext             *This,
  _In_     UINT                             StartSlot,
  _In_     UINT                             NumViews,
  _In_opt_ ID3D11ShaderResourceView* const *ppShaderResourceViews )
{
  if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    return
      SK_D3D11_SetShaderResources_Impl (
              SK_D3D11_ShaderType::Compute,
        SK_D3D11_IsDevCtxDeferred (This),
                                   This , nullptr,
          StartSlot,
            NumViews,
              ppShaderResourceViews    );
  }

  D3D11_CSSetShaderResources_Original (
    This, StartSlot, NumViews, ppShaderResourceViews
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CSSetUnorderedAccessViews_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumUAVs,
  _In_opt_       ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
  _In_opt_ const UINT                             *pUAVInitialCounts )
{
  return
    D3D11_CSSetUnorderedAccessViews_Original ( This,
      StartSlot,              NumUAVs,
        ppUnorderedAccessViews, pUAVInitialCounts
    );
}


__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_UpdateSubresource1_Override (
  _In_           ID3D11DeviceContext1 *This,
  _In_           ID3D11Resource       *pDstResource,
  _In_           UINT                  DstSubresource,
  _In_opt_ const D3D11_BOX            *pDstBox,
  _In_     const void                 *pSrcData,
  _In_           UINT                  SrcRowPitch,
  _In_           UINT                  SrcDepthPitch,
  _In_           UINT                  CopyFlags)
{
#if 0
  if (pDstResource != nullptr)
  {
    SK_ComPtr <ID3D11Device>  pParentDevice;
    SK_ComPtr <ID3D11Device>  pActualDevice;
    pDstResource->GetDevice (&pActualDevice.p);
    This->GetDevice         (&pParentDevice.p);

    if (! (pActualDevice.IsEqualObject (pParentDevice)))
    {
      SK_LOGi0 (
        L"Device Context Hook Trying to Update Subresource (Version=1) Belonging to a Different Device"
      );
      return;
    }
  }
#endif

  SK_LOG_FIRST_CALL

  // Validate non-optional pointers, WatchDogs 2 passes nullptr for pDstResource
  if (pDstResource == nullptr || pSrcData == nullptr)
    return;

  bool early_out = false;

  SK_TLS *pTLS = nullptr;

  // UB: If it's happening, pretend we never saw this...
  if ( pDstResource == nullptr ||
       pSrcData     == nullptr    )
  {
    early_out = true;
  }

  else
  {     early_out = (! SK_D3D11_ShouldTrackRenderOp (This));
    if (early_out)
    {   early_out = (! SK_D3D11_ShouldTrackMMIO     (This, &pTLS)); }
  }


  if (early_out)
  {
    return
      D3D11_UpdateSubresource1_Original (
        This, pDstResource, DstSubresource,
              pDstBox,
              pSrcData,
               SrcRowPitch,
               SrcDepthPitch, CopyFlags
      );
  }


  if (SK_D3D11_IsDevCtxDeferred (This))
  {
    return
      D3D11_UpdateSubresource1_Original (
        This, pDstResource, DstSubresource,
              pDstBox,
              pSrcData,
               SrcRowPitch,
               SrcDepthPitch, CopyFlags
      );
  }

  D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
  pDstResource->GetType  (&rdim);

  if (SK_D3D11_IsStagingCacheable (rdim, pDstResource) && DstSubresource == 0)
  {
    SK_ComQIPtr <ID3D11Texture2D>
        pTex (pDstResource);

    if (pTex != nullptr)
    {
      auto& textures =
        SK_D3D11_Textures;

      D3D11_TEXTURE2D_DESC desc = { };
           pTex->GetDesc (&desc);

      D3D11_SUBRESOURCE_DATA srd = { };

      srd.pSysMem          = pSrcData;
      srd.SysMemPitch      = SrcRowPitch;
      srd.SysMemSlicePitch = 0;

      size_t   size        = 0;
      uint32_t top_crc32c  = 0x0;

      uint32_t checksum    =
        crc32_tex ( &desc, &srd,
                           &size, &top_crc32c,
                           false );

      const uint32_t cache_tag =
        safe_crc32c ( top_crc32c, (uint8_t *)(&desc),
                        sizeof (D3D11_TEXTURE2D_DESC) );

      const auto start =
        SK_QueryPerf ().QuadPart;

      SK_ComPtr <ID3D11Texture2D> pCachedTex =
        textures->getTexture2D (   cache_tag, &desc,
                                     nullptr, nullptr,
                                                 pTLS );

      if (pCachedTex != nullptr)
      {
        SK_ComQIPtr <ID3D11Resource> pCachedResource (pCachedTex.p);

        D3D11_CopyResource_Original (This, pDstResource, pCachedResource.p);

        SK_LOG1 ( ( L"Texture Cache Hit (Slow Path): (%lux%lu) -- %x",
                      desc.Width, desc.Height, top_crc32c ),
                    L"DX11TexMgr" );

        return;
      }

      if (SK_D3D11_TextureIsCached (pTex))
      {
        SK_LOG0 ( (L"Cached texture was updated (UpdateSubresource)... removing from cache! - <%s>",
                       SK_GetCallerName ().c_str ()), L"DX11TexMgr" );
        SK_D3D11_RemoveTexFromCache     (pTex, true);
        SK_D3D11_MarkTextureUncacheable (pTex);
      }

      D3D11_UpdateSubresource_Original ( This, pDstResource, DstSubresource,
                                           pDstBox, pSrcData, SrcRowPitch,
                                             SrcDepthPitch );
      const auto end     = SK_QueryPerf ().QuadPart;
            auto elapsed = end - start;

      if (desc.Usage == D3D11_USAGE_STAGING)
      {
        auto& map_ctx = (*mapped_resources)[This];

        map_ctx.dynamic_textures  [pDstResource] = checksum;
        map_ctx.dynamic_texturesx [pDstResource] = top_crc32c;

        SK_LOG1 ( ( L"New Staged Texture: (%lux%lu) -- %x",
                      desc.Width, desc.Height, top_crc32c ),
                    L"DX11TexMgr" );

        map_ctx.dynamic_times2    [checksum]  = elapsed;
        map_ctx.dynamic_sizes2    [checksum]  = size;

        return;
      }

      // Various engines use tiny 4x4, 2x2 and even 1x1 textures that we stand to gain no
      //   benefits from caching or supporting texture injection on, so skip them.
      bool cacheable = ( desc.MiscFlags <= 4 &&
                         desc.Width      > 4 &&
                         desc.Height     > 4 &&
                         desc.ArraySize == 1 //||
                       //((desc.ArraySize  % 6 == 0) && (desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE))
                       );

      bool compressed = false;

      if ( (desc.Format >= DXGI_FORMAT_BC1_TYPELESS  &&
            desc.Format <= DXGI_FORMAT_BC5_SNORM)    ||
           (desc.Format >= DXGI_FORMAT_BC6H_TYPELESS &&
            desc.Format <= DXGI_FORMAT_BC7_UNORM_SRGB) )
      {
        compressed = true;
      }

      // If this isn't an injectable texture, then filter out non-mipmapped
      //   textures.
      if (/*(! injectable) && */
          cache_opts.ignore_non_mipped)
        cacheable &= (desc.MipLevels > 1 || compressed);

      if (cacheable)
      {
        SK_LOG1 ( ( L"New Cacheable Texture: (%lux%lu) -- %x",
                      desc.Width, desc.Height, top_crc32c ),
                    L"DX11TexMgr" );

        textures->CacheMisses_2D++;
        textures->refTexture2D ( pTex, &desc, cache_tag, size, elapsed, top_crc32c,
                                   L"", nullptr, (HMODULE)(intptr_t)-1/*SK_GetCallingDLL ()*/, pTLS );

        return;
      }
    }
  }

  return D3D11_UpdateSubresource1_Original ( This, pDstResource, DstSubresource,
                                               pDstBox, pSrcData, SrcRowPitch,
                                                 SrcDepthPitch, CopyFlags );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_UpdateSubresource_Override (
  _In_           ID3D11DeviceContext* This,
  _In_           ID3D11Resource      *pDstResource,
  _In_           UINT                 DstSubresource,
  _In_opt_ const D3D11_BOX           *pDstBox,
  _In_     const void                *pSrcData,
  _In_           UINT                 SrcRowPitch,
  _In_           UINT                 SrcDepthPitch)
{
#if 0
  if (pDstResource != nullptr)
  {
    SK_ComPtr <ID3D11Device>  pParentDevice;
    SK_ComPtr <ID3D11Device>  pActualDevice;
    pDstResource->GetDevice (&pActualDevice.p);
    This->GetDevice         (&pParentDevice.p);

    if (! (pActualDevice.IsEqualObject (pParentDevice)))
    {
      SK_LOGi0 (
        L"Device Context Hook Trying to Update Subresource Belonging to a Different Device"
      );
      return;
    }
  }
#endif

  // Validate non-optional pointers, WatchDogs 2 passes nullptr for pDstResource
  if (pDstResource == nullptr || pSrcData == nullptr)
    return;

  // Hack for Martha is Dead
  __try
  {
    if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
    {
      return
        SK_D3D11_UpdateSubresource_Impl ( This,
                                            pDstResource,
                                             DstSubresource,
                                            pDstBox,
                                            pSrcData, SrcRowPitch,
                                                      SrcDepthPitch,
                                            FALSE, _ReturnAddress () );
    }

    D3D11_UpdateSubresource_Original (
      This,
        pDstResource,
         DstSubresource, pDstBox,
                         pSrcData, SrcRowPitch,
                                   SrcDepthPitch );
  }

  __except ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                    EXCEPTION_EXECUTE_HANDLER  :
                                    EXCEPTION_CONTINUE_SEARCH )
  {
    SK_LOGi0 (L"Access Violation during ID3D11DeviceContext::UpdateSubresource (...)");
  }
}


__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11_Map_Override (
     _In_ ID3D11DeviceContext      *This,
     _In_ ID3D11Resource           *pResource,
     _In_ UINT                      Subresource,
     _In_ D3D11_MAP                 MapType,
     _In_ UINT                      MapFlags,
_Out_opt_ D3D11_MAPPED_SUBRESOURCE *pMappedResource )
{
#ifdef _SK_D3D11_VALIDATE_DEVICE_RESOURCES
  if (pResource != nullptr)
  {
    SK_ComPtr <ID3D11Device>  pParentDevice;
    SK_ComPtr <ID3D11Device>  pActualDevice;
    pResource->GetDevice    (&pActualDevice.p);
    This->GetDevice         (&pParentDevice.p);

    if (! (pActualDevice.IsEqualObject (pParentDevice)))
    {
      SK_LOGi0 (
        L"Device Context Hook Trying to Map Resource Belonging to a Different Device"
      );
      return DXGI_ERROR_DEVICE_RESET;
    }
  }
#endif

  if (pResource == nullptr)
    return E_INVALIDARG;

  if (! SK_D3D11_IgnoreWrappedOrDeferred (false, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    return
      SK_D3D11_Map_Impl ( This,
                            pResource, Subresource,
                              MapType, MapFlags,
                                pMappedResource, FALSE );
  }

  return
    D3D11_Map_Original (
      This, pResource, Subresource,
        MapType, MapFlags,
          pMappedResource
    );
}


__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_Unmap_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Resource      *pResource,
  _In_ UINT                 Subresource )
{
  if (pResource == nullptr)
    return;

#ifdef _SK_D3D11_VALIDATE_DEVICE_RESOURCES
  if (pResource != nullptr)
  {
    SK_ComPtr <ID3D11Device>  pParentDevice;
    SK_ComPtr <ID3D11Device>  pActualDevice;
    pResource->GetDevice    (&pActualDevice.p);
    This->GetDevice         (&pParentDevice.p);

    if (! (pActualDevice.IsEqualObject (pParentDevice)))
    {
      SK_LOGi0 (
        L"Device Context Hook Trying to Unmap Resource Belonging to a Different Device"
      );
      return;
    }
  }
#endif

  if (! (SK_D3D11_IgnoreWrappedOrDeferred (false, SK_D3D11_IsDevCtxDeferred (This), This)))
  {
    return
      SK_D3D11_Unmap_Impl (This, pResource, Subresource, FALSE);
  }

  D3D11_Unmap_Original (
    This, pResource, Subresource
  );
}



__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CopyResource_Override (
       ID3D11DeviceContext *This,
  _In_ ID3D11Resource      *pDstResource,
  _In_ ID3D11Resource      *pSrcResource )
{
#ifdef _SK_D3D11_VALIDATE_DEVICE_RESOURCES
  if (pSrcResource != nullptr && pDstResource != nullptr)
  {
    SK_ComPtr <ID3D11Device>  pParentDevice;
    SK_ComPtr <ID3D11Device>  pActualSrcDevice;
    SK_ComPtr <ID3D11Device>  pActualDstDevice;
    pSrcResource->GetDevice (&pActualSrcDevice.p);
    pDstResource->GetDevice (&pActualDstDevice.p);
    This->GetDevice         (&pParentDevice.p);

    if (! (pActualSrcDevice.IsEqualObject (pParentDevice) &&
           pActualDstDevice.IsEqualObject (pParentDevice))
        )
    {
      SK_LOGi0 (
        L"Device Context Hook Trying to Copy Src/Dst Resource Belonging to a Different Device"
      );
      return;
    }
  }
#endif

  if (pDstResource == nullptr || pSrcResource == nullptr)
    return;

  ////
  // This API must be processed, even on deferred contexts, or HDR remastering
  //   may cause the call to fail.
  //
  ////if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    return
      SK_D3D11_CopyResource_Impl ( This,
        pDstResource,
        pSrcResource,
          FALSE
      );
  }

  D3D11_CopyResource_Original (
    This, pDstResource, pSrcResource
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CopySubresourceRegion_Override (
  _In_           ID3D11DeviceContext *This,
  _In_           ID3D11Resource      *pDstResource,
  _In_           UINT                 DstSubresource,
  _In_           UINT                 DstX,
  _In_           UINT                 DstY,
  _In_           UINT                 DstZ,
  _In_           ID3D11Resource      *pSrcResource,
  _In_           UINT                 SrcSubresource,
  _In_opt_ const D3D11_BOX           *pSrcBox )
{
#ifdef _SK_D3D11_VALIDATE_DEVICE_RESOURCES
  if (pSrcResource != nullptr && pDstResource != nullptr)
  {
    SK_ComPtr <ID3D11Device>  pParentDevice;
    SK_ComPtr <ID3D11Device>  pActualSrcDevice;
    SK_ComPtr <ID3D11Device>  pActualDstDevice;
    pSrcResource->GetDevice (&pActualSrcDevice.p);
    pDstResource->GetDevice (&pActualDstDevice.p);
    This->GetDevice         (&pParentDevice.p);

    if (! (pActualSrcDevice.IsEqualObject (pParentDevice) &&
           pActualDstDevice.IsEqualObject (pParentDevice))
        )
    {
      SK_LOGi0 (
        L"Device Context Hook Trying to Copy Src/Dst Resource Belonging to a Different Device"
      );
      return;
    }
  }
#endif

  // UB: If it's happening, pretend we never saw this...
  if (pDstResource == nullptr || pSrcResource == nullptr)
  {
    return;
  }

  ////
  // This API must be processed, even on deferred contexts, or HDR remastering
  //   may cause the call to fail.
  //
  ////if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    return
      SK_D3D11_CopySubresourceRegion_Impl (
        This, pDstResource, DstSubresource,
          DstX, DstY, DstZ, pSrcResource,
            SrcSubresource, pSrcBox,
              FALSE
      );
  }

  D3D11_CopySubresourceRegion_Original (
    This, pDstResource, DstSubresource,
      DstX, DstY, DstZ, pSrcResource,
        SrcSubresource, pSrcBox
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_ResolveSubresource_Override (
       ID3D11DeviceContext *This,
  _In_ ID3D11Resource      *pDstResource,
  _In_ UINT                 DstSubresource,
  _In_ ID3D11Resource      *pSrcResource,
  _In_ UINT                 SrcSubresource,
  _In_ DXGI_FORMAT          Format )
{
#ifdef _SK_D3D11_VALIDATE_DEVICE_RESOURCES
  if (pSrcResource != nullptr && pDstResource != nullptr)
  {
    SK_ComPtr <ID3D11Device>  pParentDevice;
    SK_ComPtr <ID3D11Device>  pActualSrcDevice;
    SK_ComPtr <ID3D11Device>  pActualDstDevice;
    pSrcResource->GetDevice (&pActualSrcDevice.p);
    pDstResource->GetDevice (&pActualDstDevice.p);
    This->GetDevice         (&pParentDevice.p);

    if (! (pActualSrcDevice.IsEqualObject (pParentDevice) &&
           pActualDstDevice.IsEqualObject (pParentDevice))
        )
    {
      SK_LOGi0 (
        L"Device Context Hook Trying to Copy Src/Dst Resource Belonging to a Different Device"
      );
      //return;
    }
  }
#endif

  if (pDstResource == nullptr || pSrcResource == nullptr)
    return;

  ////
  // This API must be processed, even on deferred contexts, or HDR remastering
  //   may cause the call to fail.
  //
  ////if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    return
      SK_D3D11_ResolveSubresource_Impl ( This,
        pDstResource, DstSubresource,
        pSrcResource, SrcSubresource,
        Format,
          FALSE
      );
  }
  
  D3D11_ResolveSubresource_Original (
    This, pDstResource, DstSubresource,
          pSrcResource, SrcSubresource,
          Format
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawAuto_Override (_In_ ID3D11DeviceContext *This)
{
  SK_LOG_FIRST_CALL

  if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    return
      SK_D3D11_DrawAuto_Impl ( This, FALSE );
  }

  D3D11_DrawAuto_Original (
    This
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawIndexed_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation )
{
  SK_LOG_FIRST_CALL

  if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    return
      SK_D3D11_DrawIndexed_Impl ( This,
                   IndexCount,
              StartIndexLocation,
              BaseVertexLocation, FALSE
      );
  }

  D3D11_DrawIndexed_Original (
    This,   IndexCount,
       StartIndexLocation,
       BaseVertexLocation
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_Draw_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCount,
  _In_ UINT                 StartVertexLocation )
{
  SK_LOG_FIRST_CALL

  if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    return
      SK_D3D11_Draw_Impl ( This,
             VertexCount,
        StartVertexLocation,
          false
      );
  }

  D3D11_Draw_Original (
    This,
         VertexCount,
    StartVertexLocation
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawIndexedInstanced_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation,
  _In_ UINT                 StartInstanceLocation )
{
  SK_LOG_FIRST_CALL

  if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    return
      SK_D3D11_DrawIndexedInstanced_Impl ( This,
                   IndexCountPerInstance,
                                InstanceCount,
              StartIndexLocation,
              BaseVertexLocation,
           StartInstanceLocation, FALSE
      );
  }

  D3D11_DrawIndexedInstanced_Original ( This,
            IndexCountPerInstance,
                         InstanceCount,
       StartIndexLocation,
       BaseVertexLocation,
    StartInstanceLocation
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawIndexedInstancedIndirect_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs )
{
  SK_LOG_FIRST_CALL

  if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    return
      SK_D3D11_DrawIndexedInstancedIndirect_Impl (
            This, pBufferForArgs,
        AlignedByteOffsetForArgs, FALSE
      );
  }

  D3D11_DrawIndexedInstancedIndirect_Original (
    This,       pBufferForArgs,
      AlignedByteOffsetForArgs
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawInstanced_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartVertexLocation,
  _In_ UINT                 StartInstanceLocation )
{
  SK_LOG_FIRST_CALL

  if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    return
      SK_D3D11_DrawInstanced_Impl (
           This,
                  VertexCountPerInstance,
                InstanceCount,
             StartVertexLocation,
           StartInstanceLocation, FALSE
      );
  }

  D3D11_DrawInstanced_Original (
    This,
           VertexCountPerInstance,
         InstanceCount,
      StartVertexLocation,
    StartInstanceLocation
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawInstancedIndirect_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs )
{
  SK_LOG_FIRST_CALL

  if (pBufferForArgs == nullptr)
    return;

  if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    return
      SK_D3D11_DrawInstancedIndirect_Impl ( This,
                         pBufferForArgs,
               AlignedByteOffsetForArgs, FALSE
      );
  }

  D3D11_DrawInstancedIndirect_Original ( This,
                  pBufferForArgs,
        AlignedByteOffsetForArgs
  );
}


__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_Dispatch_Override ( _In_ ID3D11DeviceContext *This,
                          _In_ UINT                 ThreadGroupCountX,
                          _In_ UINT                 ThreadGroupCountY,
                          _In_ UINT                 ThreadGroupCountZ )
{
  SK_LOG_FIRST_CALL

  if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    const UINT dev_idx =
      SK_D3D11_GetDeviceContextHandle (This);

    return
      SK_D3D11_Dispatch_Impl           ( This,
        ThreadGroupCountX,
          ThreadGroupCountY,
            ThreadGroupCountZ, FALSE,
                                 dev_idx
      );
  }

  D3D11_Dispatch_Original ( This,
    ThreadGroupCountX,
      ThreadGroupCountY,
        ThreadGroupCountZ
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DispatchIndirect_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs )
{
  SK_LOG_FIRST_CALL

  if (pBufferForArgs == nullptr)
    return;

  if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    const UINT dev_idx =
      SK_D3D11_GetDeviceContextHandle (This);

    return
      SK_D3D11_DispatchIndirect_Impl   ( This,
                  pBufferForArgs,
        AlignedByteOffsetForArgs, FALSE,
                                    dev_idx
      );
  }

  D3D11_DispatchIndirect_Original ( This,
              pBufferForArgs,
    AlignedByteOffsetForArgs
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_OMSetRenderTargets_Override (
         ID3D11DeviceContext           *This,
_In_     UINT                           NumViews,
_In_opt_ ID3D11RenderTargetView *const *ppRenderTargetViews,
_In_opt_ ID3D11DepthStencilView        *pDepthStencilView )
{
  if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    return
      SK_D3D11_OMSetRenderTargets_Impl ( This,
                             NumViews,
                  ppRenderTargetViews,
                   pDepthStencilView,
        FALSE
      );
  }

  D3D11_OMSetRenderTargets_Original ( This,
                      NumViews,
           ppRenderTargetViews,
            pDepthStencilView
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Override (
                 ID3D11DeviceContext              *This,
  _In_           UINT                              NumRTVs,
  _In_opt_       ID3D11RenderTargetView    *const *ppRenderTargetViews,
  _In_opt_       ID3D11DepthStencilView           *pDepthStencilView,
  _In_           UINT                              UAVStartSlot,
  _In_           UINT                              NumUAVs,
  _In_opt_       ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
  _In_opt_ const UINT                             *pUAVInitialCounts )
{
  if (! SK_D3D11_IgnoreWrappedOrDeferred (FALSE, SK_D3D11_IsDevCtxDeferred (This), This))
  {
    return
      SK_D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Impl ( This,
                                                     NumRTVs,
                                     ppRenderTargetViews,
                                      pDepthStencilView,
                                                UAVStartSlot,
                                             NumUAVs,
                                     ppUnorderedAccessViews,
                                               pUAVInitialCounts,
            FALSE
          );
  }

  D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Original ( This,
    NumRTVs, ppRenderTargetViews, pDepthStencilView,
      UAVStartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_OMGetRenderTargets_Override (ID3D11DeviceContext     *This,
                              _In_ UINT                     NumViews,
                         _Out_opt_ ID3D11RenderTargetView **ppRenderTargetViews,
                         _Out_opt_ ID3D11DepthStencilView **ppDepthStencilView)
{
  D3D11_OMGetRenderTargets_Original (
    This,        NumViews,
      ppRenderTargetViews,
      ppDepthStencilView );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_OMGetRenderTargetsAndUnorderedAccessViews_Override (
            ID3D11DeviceContext        *This,
  _In_      UINT                        NumRTVs,
  _Out_opt_ ID3D11RenderTargetView    **ppRenderTargetViews,
  _Out_opt_ ID3D11DepthStencilView    **ppDepthStencilView,
  _In_      UINT                        UAVStartSlot,
  _In_      UINT                        NumUAVs,
  _Out_opt_ ID3D11UnorderedAccessView **ppUnorderedAccessViews)
{
  D3D11_OMGetRenderTargetsAndUnorderedAccessViews_Original (
    This, NumRTVs, ppRenderTargetViews, ppDepthStencilView,
          UAVStartSlot, NumUAVs, ppUnorderedAccessViews    );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_ClearDepthStencilView_Override (ID3D11DeviceContext    *This,
                                 _In_ ID3D11DepthStencilView *pDepthStencilView,
                                 _In_ UINT                    ClearFlags,
                                 _In_ FLOAT                   Depth,
                                 _In_ UINT8                   Stencil)
{
  D3D11_ClearDepthStencilView_Original (
    This,      pDepthStencilView,
    ClearFlags, Depth,
                     Stencil
  );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_RSSetViewports_Override (
                 ID3D11DeviceContext* This,
  _In_           UINT                 NumViewports,
  _In_opt_ const D3D11_VIEWPORT*      pViewports )
{
  if (pViewports == nullptr)
    return D3D11_RSSetViewports_Original (
             This, NumViewports, pViewports );

#ifdef _M_AMD64
  static const auto game_id =
        SK_GetCurrentGameID ();

  switch (game_id)
  {
    case SK_GAME_ID::GalGunReturns:
    {
      if (! config.window.res.override.isZero ())
      {
        if (NumViewports == 0)
            NumViewports  = 1;

        if (NumViewports        == 1    &&
            ((pViewports->Width == 1920 ||
              pViewports->Width == 480) ||
             (pViewports->Width < config.window.res.override.x &&
              pViewports->Width > config.window.res.override.x * 0.97f) ) )
        {
          D3D11_VIEWPORT vpNew = *pViewports;

          if (pViewports->Width == 480)
          {
            vpNew.Width  = static_cast <float> (config.window.res.override.x / 4);
            vpNew.Height = static_cast <float> (config.window.res.override.y / 4);
          }

          else
          {
            vpNew.Width  = static_cast <float> (config.window.res.override.x);
            vpNew.Height = static_cast <float> (config.window.res.override.y);
          }

          return
            D3D11_RSSetViewports_Original (
              This, NumViewports, &vpNew
            );
        }
      }
    } break;

    case SK_GAME_ID::ShinMegamiTensei3:
    {
      if (NumViewports > 1)
      {
        SK_LOGi0 ( L"VP Width: %4.1f, Height: %4.1f - [%4.1f, %4.1f]",
                       pViewports->Width,    pViewports->Height,
                       pViewports->TopLeftX, pViewports->TopLeftY );
      }

    if (NumViewports        == 1 &&
          pViewports->Width == 1280)
    {
      const SK_RenderBackend& rb =
        SK_GetCurrentRenderBackend ();
    
      SK_ComQIPtr <IDXGISwapChain>
               pSwapChain (
             rb.swapchain );
      DXGI_SWAP_CHAIN_DESC  swapDesc = { };
    
      if (pSwapChain != nullptr)
      {   pSwapChain->GetDesc (&swapDesc);
    
        D3D11_VIEWPORT vpNew = *pViewports;
    
        vpNew.Width  = static_cast <FLOAT> (swapDesc.BufferDesc.Width);
        vpNew.Height = static_cast <FLOAT> (swapDesc.BufferDesc.Height);
    
        return
          D3D11_RSSetViewports_Original (
            This, NumViewports, &vpNew
          );
      }
    }
    //
    //else
    //if (NumViewports        == 1 &&
    //      pViewports->Width == 1920)
    //{
    //  const SK_RenderBackend& rb =
    //    SK_GetCurrentRenderBackend ();
    //
    //  SK_ComQIPtr <IDXGISwapChain>
    //           pSwapChain (
    //         rb.swapchain );
    //  DXGI_SWAP_CHAIN_DESC  swapDesc = { };
    //
    //  if (pSwapChain != nullptr)
    //  {   pSwapChain->GetDesc (&swapDesc);
    //
    //    D3D11_VIEWPORT vpNew = *pViewports;
    //
    //    vpNew.Width  = static_cast <FLOAT> (swapDesc.BufferDesc.Width);
    //    vpNew.Height = static_cast <FLOAT> (swapDesc.BufferDesc.Height);
    //
    //    return
    //      D3D11_RSSetViewports_Original (
    //        This, NumViewports, &vpNew
    //      );
    //  }
    //}

      else
      if (NumViewports         ==   1  &&
            pViewports->Width  == 1080 &&
            pViewports->Height ==  720)
      {
        D3D11_VIEWPORT vpNew = *pViewports;
      
        vpNew.Width  *= 2;
        vpNew.Height *= 2;
      
        return
          D3D11_RSSetViewports_Original (
            This, NumViewports, &vpNew
          );
      }

      else
      if (NumViewports == 1 && pViewports->Width == 512.0f)
      {
      //  D3D11_VIEWPORT vpNew = *pViewports;
      //
      //  vpNew.Width  = 8192.0f;
      //  vpNew.Height = 8192.0f;
      //
      //  return
      //    D3D11_RSSetViewports_Original (
      //      This, NumViewports, &vpNew
      //    );
      }

//      else
//      if (NumViewports == 1 && pViewports->Width <= 640.0f)
//      {
//        D3D11_VIEWPORT vpNew = *pViewports;
//
//        vpNew.Width  *= 3.0f;
//        vpNew.Height *= 3.0f;
//
//        return
//          D3D11_RSSetViewports_Original (
//            This, NumViewports, &vpNew
//          );
//      }
    } break;
  }
#endif

  ///if (NumViewports == 0 && pViewports != nullptr)
  ///    NumViewports  = 1;
  ///
  ///if (NumViewports == 1)
  ///{
  ///  D3D11_VIEWPORT* pViewportsNew =
  ///    (D3D11_VIEWPORT *)(
  ///      SK_TLS_Bottom ()->d3d9->allocStackScratchStorage (
  ///        sizeof (D3D11_VIEWPORT) * NumViewports         )
  ///    );
  ///
  ///  SK_ComPtr <ID3D11RenderTargetView> pRTV;
  ///  SK_ComPtr <ID3D11DepthStencilView> pDSV;
  ///
  ///  This->OMGetRenderTargets (
  ///    1, &pRTV.p, &pDSV.p
  ///  );
  ///
  ///  if (pRTV.p != nullptr)
  ///  {
  ///    SK_ComPtr <ID3D11Resource> pRTV_Resource;
  ///           pRTV->GetResource (&pRTV_Resource);
  ///
  ///    D3D11_RESOURCE_DIMENSION res_dim;
  ///    pRTV_Resource->GetType (&res_dim);
  ///
  ///    if (res_dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
  ///    {
  ///      SK_ComQIPtr <ID3D11Texture2D> pTex (pRTV_Resource);
  ///
  ///      D3D11_TEXTURE2D_DESC tex_desc = { };
  ///      pTex->GetDesc      (&tex_desc);
  ///
  ///      if (tex_desc.Width  == pViewports [0].Width  * 2 &&
  ///          tex_desc.Height == pViewports [0].Height * 2)
  ///      {
  ///        pViewportsNew [0] =
  ///        pViewports    [0];
  ///        pViewportsNew [0].Width  *= 2;
  ///        pViewportsNew [0].Height *= 2;
  ///
  ///        return
  ///          D3D11_RSSetViewports_Original ( This, NumViewports,
  ///                                                  pViewportsNew );
  ///      }
  ///    }
  ///  }
  ///}
  ///
  ///else dll_log->Log (L"%lu viewports", NumViewports);

  D3D11_RSSetViewports_Original (
    This, NumViewports, pViewports
  );
}

extern concurrency::concurrent_unordered_set <ID3D11SamplerState *>
    _SK_D3D11_OverrideSamplers__UserDefinedLODBias,
    _SK_D3D11_OverrideSamplers__UserDefinedAnisotropy,
    _SK_D3D11_OverrideSamplers__UserForcedAnisotropic;

void
WINAPI
D3D11_PSSetSamplers_Override
(
  _In_     ID3D11DeviceContext          *This,
  _In_     UINT                          StartSlot,
  _In_     UINT                          NumSamplers,
  _In_opt_ ID3D11SamplerState */*const*/* ppSamplers)
{
  if (! (SK_D3D11_IsDevCtxDeferred (This)))
  {
#if 0
    if ( ppSamplers != nullptr )
    {
      SK_TLS *pTLS;
      //if (SK_GetCurrentRenderBackend ().d3d11.immediate_ctx.IsEqualObject (This))
      if (SK_GetCurrentRenderBackend ().device.p != nullptr && (pTLS = SK_TLS_Bottom ()))
      {
        //ID3D11SamplerState** pSamplerCopy =
        //  (ID3D11SamplerState **)pTLS->scratch_memory.cmd.alloc (
        //     sizeof (ID3D11SamplerState  *) * 4096
        //  );
        //
        //bool ys8_wrap_ui  = false,
        //     ys8_clamp_ui = false;

        //if (SK_GetCurrentGameID () == SK_GAME_ID::Ys_Eight)
        //{
        //  SK_D3D11_EnableTracking = true;
        //
        //  auto HashFromCtx =
        //    [] ( std::array <uint32_t, SK_D3D11_MAX_DEV_CONTEXTS+1>& registry,
        //         UINT                                                dev_idx ) ->
        //  uint32_t
        //  {
        //    return
        //      registry [dev_idx];
        //  };
        //
        //  UINT dev_idx = SK_D3D11_GetDeviceContextHandle (This);
        //
        //  uint32_t current_ps = HashFromCtx (SK_D3D11_Shaders.pixel.current.shader,  dev_idx);
        //  uint32_t current_vs = HashFromCtx (SK_D3D11_Shaders.vertex.current.shader, dev_idx);
        //
        //  switch (current_ps)
        //  {
        //    case 0x66b35959:
        //    case 0x9d665ae2:
        //    case 0xb21c8ab9:
        //    case 0x05da09bd:
        //    {
        //      if (current_ps == 0x66b35959 && b_66b35959)                             ys8_clamp_ui = true;
        //      if (current_ps == 0x9d665ae2 && b_9d665ae2)                             ys8_clamp_ui = true;
        //      if (current_ps == 0xb21c8ab9 && b_b21c8ab9)                             ys8_clamp_ui = true;
        //      if (current_ps == 0x05da09bd && b_05da09bd && current_vs == 0x7759c300) ys8_clamp_ui = true;
        //    }break;
        //    case 0x6bb0972d:
        //    {
        //      if (current_ps == 0x6bb0972d && b_6bb0972d) ys8_wrap_ui = true;
        //    } break;
        //  }
        //}

        if (true)////! (pTLS->imgui.drawing || ys8_clamp_ui || ys8_wrap_ui))
        {
          for ( UINT i = 0 ; i < NumSamplers ; i++ )
          {
            pSamplerCopy [i] = ppSamplers [i];

            if (ppSamplers [i] != nullptr)
            {
              D3D11_SAMPLER_DESC        new_desc = { };
              ppSamplers [i]->GetDesc (&new_desc);

              ((ID3D11Device *)SK_GetCurrentRenderBackend ().device.p)->CreateSamplerState (
                &new_desc,
                  &pSamplerCopy [i]
              );
            }
          }
        }

        else
        {
          for ( UINT i = 0 ; i < NumSamplers ; i++ )
          {
            if (! ys8_wrap_ui)
              pSamplerCopy [i] = pTLS->render->d3d11->uiSampler_clamp;
            else
              pSamplerCopy [i] = pTLS->render->d3d11->uiSampler_wrap;
          }
        }

        return
          D3D11_PSSetSamplers_Original (
            This, StartSlot,
              NumSamplers,
                pSamplerCopy
          );
      }
    }
#endif
  }

  //
  // TODO: Make this adjustable in real-time; requires recycling a constant set of override
  //         samplers...
  //
#if 0
  SK_ComPtr <ID3D11SamplerState> samplers [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];

  if (StartSlot + NumSamplers <= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT)
  {
    for (UINT i = 0 ; i < NumSamplers ; ++i)
    {
      if ( ppSamplers     != nullptr &&
           ppSamplers [i] != nullptr )
      {
        const bool bUserDefinedLOD =
          _SK_D3D11_OverrideSamplers__UserDefinedLODBias.count    (ppSamplers [i]),
                   bUserDefinedAnisotropy =
          _SK_D3D11_OverrideSamplers__UserDefinedAnisotropy.count (ppSamplers [i]),
                   bUserForcedAnisotropic =
          _SK_D3D11_OverrideSamplers__UserForcedAnisotropic.count (ppSamplers [i]);
  
        if ( bUserDefinedLOD        ||
             bUserDefinedAnisotropy ||
             bUserForcedAnisotropic )
        {
          D3D11_SAMPLER_DESC        samplerDesc = { };
          ppSamplers [i]->GetDesc (&samplerDesc);
  
          bool createNew = false;
  
          // Check if the sampler desc matches the user's current prefs.
          if (bUserDefinedAnisotropy && samplerDesc.MaxAnisotropy != (UINT)config.render.d3d12.max_anisotropy)
          {
            createNew                 = true;
            samplerDesc.MaxAnisotropy = config.render.d3d12.max_anisotropy;
          }
  
          if (bUserDefinedLOD && samplerDesc.MipLODBias != config.render.d3d12.force_lod_bias)
          {
            createNew              = true;
            samplerDesc.MipLODBias = config.render.d3d12.force_lod_bias;
          }
  
          ///if (bUserForcedAnisotropic && samplerDesc.Filter != config.render.d3d12.force_lod_bias)
          ///{
          ///  // Need MipLOD Bias Adjust...
          ///}
  
          if (createNew)
          {
            SK_ComPtr <ID3D11Device>
                              pDev11;
            This->GetDevice (&pDev11.p);
  
            if (SUCCEEDED (D3D11Dev_CreateSamplerState_Original (pDev11.p, &samplerDesc, &samplers [i].p)))
            {
              ppSamplers [i]->AddRef (); // Ensure that the game doesn't release the final reference
                                         //   to the object we are not going to bind to the dev ctx.
              ppSamplers [i] = samplers [i];
            }
          }
        }
      }
    }
  }
#endif

  D3D11_PSSetSamplers_Original (
    This, StartSlot,
      NumSamplers,
       ppSamplers
  );
}



#if 0
SetCurrentThreadDescription (L"[SK] DXGI Hook Crawler");

  // "Normal" games don't change render APIs mid-game; Talos does, but it's
  //   not normal :)
  if (SK_GetFramesDrawn ())
  {
    SK_Thread_CloseSelf ();
    return 0;
  }


  UNREFERENCED_PARAMETER (user);

  if (! (config.apis.dxgi.d3d11.hook ||
         config.apis.dxgi.d3d12.hook) )
  {
    SK_Thread_CloseSelf ();
    return 0;
  }

  // Wait for DXGI to boot
  if (CreateDXGIFactory_Import == nullptr)
  {
    static volatile ULONG implicit_init = FALSE;

    // If something called a D3D11 function before DXGI was initialized,
    //   begin the process, but ... only do this once.
    if (! InterlockedCompareExchange (&implicit_init, TRUE, FALSE))
    {
      dll_log->Log (L"[  D3D 11  ]  >> Implicit Initialization Triggered <<");
      SK_BootDXGI ();
    }

    while (CreateDXGIFactory_Import == nullptr)
      MsgWaitForMultipleObjectsEx (0, nullptr, 33, QS_ALLEVENTS, MWMO_INPUTAVAILABLE);

    // TODO: Handle situation where CreateDXGIFactory is unloadable
  }

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  if ( __SK_bypass     || ReadAcquire (&__dxgi_ready) ||
       pTLS == nullptr || pTLS->render->d3d11->ctx_init_thread )
  {
    SK_Thread_CloseSelf ();
    return 0;
  }


  static volatile LONG __hooked = FALSE;

  if (! InterlockedCompareExchangeAcquire (&__hooked, TRUE, FALSE))
  {
    pTLS->render->d3d11->ctx_init_thread = true;

    SK_AutoCOMInit auto_com;

    SK_D3D11_Init ();

    if (D3D11CreateDeviceAndSwapChain_Import == nullptr)
    {
      pTLS->render->d3d11->ctx_init_thread = false;

      SK_ApplyQueuedHooks ();
      return 0;
    }

    dll_log->Log (L"[   DXGI   ]   Installing DXGI Hooks");

    D3D_FEATURE_LEVEL            levels [] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_11_1,
                                               D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_10_1 };

    D3D_FEATURE_LEVEL            featureLevel;
    SK_ComPtr <ID3D11Device>        pDevice           = nullptr;
    SK_ComPtr <ID3D11DeviceContext> pImmediateContext = nullptr;
//    ID3D11DeviceContext           *pDeferredContext  = nullptr;

    // DXGI stuff is ready at this point, we'll hook the swapchain stuff
    //   after this call.

    HRESULT hr = E_NOTIMPL;

    SK_ComPtr <IDXGISwapChain> pSwapChain = nullptr;
    DXGI_SWAP_CHAIN_DESC       desc       = { };

    desc.BufferDesc.Format           = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc.BufferDesc.Scaling          = DXGI_MODE_SCALING_UNSPECIFIED;
    desc.SampleDesc.Count            = 1;
    desc.SampleDesc.Quality          = 0;
    desc.BufferDesc.Width            = 2;
    desc.BufferDesc.Height           = 2;
    desc.BufferUsage                 = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount                 = 1;
    desc.OutputWindow                = SK_Win32_CreateDummyWindow ();
    desc.Windowed                    = TRUE;
    desc.SwapEffect                  = DXGI_SWAP_EFFECT_DISCARD;

    extern LPVOID pfnD3D11CreateDeviceAndSwapChain;

    SK_COMPAT_UnloadFraps ();

    if ((SK_GetDLLRole () & DLL_ROLE::DXGI) || (SK_GetDLLRole () & DLL_ROLE::DInput8))
    {
      // PlugIns need to be loaded AFTER we've hooked the device creation functions
      SK_DXGI_InitHooksBeforePlugIn ();

      // Load user-defined DLLs (Plug-In)
      SK_RunLHIfBitness ( 64, SK_LoadPlugIns64 (),
                              SK_LoadPlugIns32 () );
    }

    hr =
      D3D11CreateDeviceAndSwapChain_Import (
        nullptr,
          D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
              0x0,
                levels,
                  _ARRAYSIZE(levels),
                    D3D11_SDK_VERSION, &desc,
                      &pSwapChain.p,
                        &pDevice.p,
                          &featureLevel,
                            &pImmediateContext.p );

    sk_hook_d3d11_t d3d11_hook_ctx = { };

    d3d11_hook_ctx.ppDevice           = &pDevice.p;
    d3d11_hook_ctx.ppImmediateContext = &pImmediateContext.p;

    SK_ComPtr <IDXGIDevice>  pDevDXGI = nullptr;
    SK_ComPtr <IDXGIAdapter> pAdapter = nullptr;
    SK_ComPtr <IDXGIFactory> pFactory = nullptr;

    if ( pDevice != nullptr &&
         SUCCEEDED (pDevice->QueryInterface <IDXGIDevice> (&pDevDXGI)) &&
         SUCCEEDED (pDevDXGI->GetAdapter                  (&pAdapter)) &&
         SUCCEEDED (pAdapter->GetParent     (IID_PPV_ARGS (&pFactory))) )
    {
      //if (config.render.dxgi.deferred_isolation)
      //{
        //    pDevice->CreateDeferredContext (0x0, &pDeferredContext);
        //d3d11_hook_ctx.ppImmediateContext = &pDeferredContext;
      //}

      HookD3D11             (&d3d11_hook_ctx);
      SK_DXGI_HookFactory   (pFactory);
      //if (SUCCEEDED (pFactory->CreateSwapChain (pDevice, &desc, &pSwapChain)))
      SK_DXGI_HookSwapChain (pSwapChain);

      // This won't catch Present1 (...), but no games use that
      //   and we can deal with it later if it happens.
      SK_DXGI_HookPresentBase ((IDXGISwapChain *)pSwapChain);

      SK_ComQIPtr <IDXGISwapChain1> pSwapChain1 (pSwapChain);

      if (pSwapChain1 != nullptr)
        SK_DXGI_HookPresent1 (pSwapChain1);

      SK_ApplyQueuedHooks ();


      InterlockedIncrementRelease (&SK_D3D11_initialized);

      if (config.apis.dxgi.d3d11.hook) SK_D3D11_EnableHooks ();

/////      if (config.apis.dxgi.d3d12.hook) SK_D3D12_EnableHooks ();

      WriteRelease (&__dxgi_ready, TRUE);
    }

    else
    {
      _com_error err (hr);

      dll_log->Log (L"[   DXGI   ] Unable to hook D3D11?! HRESULT=%x ('%s')",
                                err.Error (), err.ErrorMessage () != nullptr ?
                                              err.ErrorMessage ()            : L"Unknown" );

      // NOTE: Calling err.ErrorMessage () above generates the storage for these string functions
      //         --> They may be NULL if allocation failed.
      std::wstring err_desc (err.ErrorInfo () != nullptr ? err.Description () : L"Unknown");
      std::wstring err_src  (err.ErrorInfo () != nullptr ? err.Source      () : L"Unknown");

      dll_log->Log (L"[   DXGI   ]  >> %s, in %s",
                               err_desc.c_str (), err_src.c_str () );
    }

    SK_Win32_CleanupDummyWindow (desc.OutputWindow);

    InterlockedIncrementRelease (&__hooked);
  }

  SK_Thread_SpinUntilAtomicMin (&__hooked, 2);

  SK_Thread_CloseSelf ();

  return 0;
#endif