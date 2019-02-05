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

#define __SK_SUBSYSTEM__ L"  D3D 11  "

#include <SpecialK/render/d3d11/d3d11_core.h>

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateTexture2D_Override (
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D );

D3D11Dev_CreateRasterizerState_pfn                  D3D11Dev_CreateRasterizerState_Original                  = nullptr;
D3D11Dev_CreateSamplerState_pfn                     D3D11Dev_CreateSamplerState_Original                     = nullptr;
D3D11Dev_CreateBuffer_pfn                           D3D11Dev_CreateBuffer_Original                           = nullptr;
D3D11Dev_CreateTexture2D_pfn                        D3D11Dev_CreateTexture2D_Original                        = nullptr;
D3D11Dev_CreateRenderTargetView_pfn                 D3D11Dev_CreateRenderTargetView_Original                 = nullptr;
D3D11Dev_CreateShaderResourceView_pfn               D3D11Dev_CreateShaderResourceView_Original               = nullptr;
D3D11Dev_CreateDepthStencilView_pfn                 D3D11Dev_CreateDepthStencilView_Original                 = nullptr;
D3D11Dev_CreateUnorderedAccessView_pfn              D3D11Dev_CreateUnorderedAccessView_Original              = nullptr;

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


#include <../src/render/d3d11/d3d11_dev_ctx.cpp>