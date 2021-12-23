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
#include <SpecialK/render/d3d11/d3d11_core.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"D3D11State"

struct SK_D3D11_Stateblock_Lite : StateBlockDataStore
{
  void capture (ID3D11DeviceContext* pCtx)
  {
    if (pCtx == nullptr)
      return;

    SK_ComPtr <ID3D11Device>
                      pDev;
    pCtx->GetDevice (&pDev.p);

    if ( pDev == nullptr )
      return;

    const D3D_FEATURE_LEVEL ft_lvl =
      pDev->GetFeatureLevel ();

    ScissorRectsCount = ViewportsCount =
      D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;

    pCtx->RSGetScissorRects      (      &ScissorRectsCount, ScissorRects);
    pCtx->RSGetViewports         (      &ViewportsCount,    Viewports);
    pCtx->RSGetState             (      &RS);
    pCtx->OMGetBlendState        (      &BlendState,         BlendFactor,
                                                            &SampleMask);
    pCtx->OMGetDepthStencilState (      &DepthStencilState, &StencilRef);
  //pCtx->PSGetShaderResources   (0, 2, PSShaderResources);
    pCtx->PSGetConstantBuffers   (0, 1, &PSConstantBuffer);
    pCtx->PSGetSamplers          (0, 1, &PSSampler);

    PSInstancesCount = VSInstancesCount = GSInstancesCount =
    HSInstancesCount = DSInstancesCount  =
      D3D11_SHADER_MAX_INTERFACES;// D3D11_SHADER_MAX_INSTANCES_PER_CLASS;

    pCtx->PSGetShader            (&PS, PSInstances, &PSInstancesCount);
    pCtx->VSGetShader            (&VS, VSInstances, &VSInstancesCount);

    if (ft_lvl >= D3D_FEATURE_LEVEL_10_0)
    {
      pCtx->GSGetShader             (&GS, GSInstances, &GSInstancesCount);
      GSInstancesCount =     calc_count ( GSInstances,  GSInstancesCount);
    }

    if (ft_lvl >= D3D_FEATURE_LEVEL_11_0)
    {
      pCtx->HSGetShader            (&HS, HSInstances, &HSInstancesCount);
      HSInstancesCount =     calc_count (HSInstances,  HSInstancesCount);

      pCtx->DSGetShader            (&DS, DSInstances, &DSInstancesCount);
      DSInstancesCount =     calc_count (DSInstances,  DSInstancesCount);
    }

    pCtx->VSGetConstantBuffers   (0, 1, &VSConstantBuffer);
    pCtx->IAGetPrimitiveTopology (      &PrimitiveTopology);
    pCtx->IAGetIndexBuffer       (      &IndexBuffer,  &IndexBufferFormat,
                                                       &IndexBufferOffset);
    pCtx->IAGetVertexBuffers     (0, 1, &VertexBuffer, &VertexBufferStride,
                                                       &VertexBufferOffset);
    pCtx->IAGetInputLayout       (      &InputLayout );
    pCtx->OMGetRenderTargets     (1, &RenderTargetView, &DepthStencilView);

    PSInstancesCount = calc_count (PSInstances, PSInstancesCount);
    VSInstancesCount = calc_count (VSInstances, VSInstancesCount);
  }

  void apply (ID3D11DeviceContext* pCtx)
  {
    if (pCtx == nullptr)
      return;

    SK_ComPtr <ID3D11Device>
                      pDev;
    pCtx->GetDevice (&pDev.p);

    if (pDev.p == nullptr)
      return;

    const D3D_FEATURE_LEVEL ft_lvl =
      pDev->GetFeatureLevel ();

    pCtx->RSSetScissorRects      (ScissorRectsCount, ScissorRects);
    pCtx->RSSetViewports         (ViewportsCount,    Viewports);
    pCtx->OMSetDepthStencilState (DepthStencilState, StencilRef);
    pCtx->RSSetState             (RS);
    pCtx->PSSetShader            (PS, PSInstances,   PSInstancesCount);
    pCtx->VSSetShader            (VS, VSInstances,   VSInstancesCount);
    if (ft_lvl >= D3D_FEATURE_LEVEL_10_0)
      pCtx->GSSetShader          (GS, GSInstances,   GSInstancesCount);
    if (ft_lvl >= D3D_FEATURE_LEVEL_11_0)
    {
      pCtx->HSSetShader          (HS, HSInstances,   HSInstancesCount);
      pCtx->DSSetShader          (DS, DSInstances,   DSInstancesCount);
    }
    pCtx->OMSetBlendState        (BlendState,        BlendFactor,
                                                     SampleMask);
    pCtx->IASetIndexBuffer       (IndexBuffer,       IndexBufferFormat,
                                                     IndexBufferOffset);
    pCtx->IASetInputLayout       (InputLayout);
    pCtx->IASetPrimitiveTopology (PrimitiveTopology);
    pCtx->PSSetShaderResources   (0, 1, std::array <ID3D11ShaderResourceView *, 1> { nullptr }.data ()/*PSShaderResources*/);
    pCtx->PSSetConstantBuffers   (0, 1, &PSConstantBuffer);
    pCtx->VSSetConstantBuffers   (0, 1, &VSConstantBuffer);
    pCtx->PSSetSamplers          (0, 1, &PSSampler);
    pCtx->IASetVertexBuffers     (0, 1, &VertexBuffer,    &VertexBufferStride,
                                                          &VertexBufferOffset);
    pCtx->OMSetRenderTargets     (1,    &RenderTargetView, DepthStencilView);

    if (RS                != nullptr)                RS->Release ();
    if (PS                != nullptr)                PS->Release ();
    if (VS                != nullptr)                VS->Release ();
    if (ft_lvl >= D3D_FEATURE_LEVEL_10_0 &&
        GS                != nullptr)                GS->Release ();
    if (ft_lvl >= D3D_FEATURE_LEVEL_11_0 &&
        HS                != nullptr)                HS->Release ();
    if (ft_lvl >= D3D_FEATURE_LEVEL_11_0 &&
        DS                != nullptr)                DS->Release ();
    if (PSSampler         != nullptr)         PSSampler->Release ();
    if (BlendState        != nullptr)        BlendState->Release ();
    if (InputLayout       != nullptr)       InputLayout->Release ();
    if (IndexBuffer       != nullptr)       IndexBuffer->Release ();
    if (VertexBuffer      != nullptr)      VertexBuffer->Release ();
  //if (PSShaderResources [0]) PSShaderResources [0]->Release ();
  //if (PSShaderResources [1]) PSShaderResources [1]->Release ();
    if (VSConstantBuffer  != nullptr)  VSConstantBuffer->Release ();
    if (PSConstantBuffer  != nullptr)  PSConstantBuffer->Release ();
    if (RenderTargetView  != nullptr)  RenderTargetView->Release ();
    if (DepthStencilView  != nullptr)  DepthStencilView->Release ();
    if (DepthStencilState != nullptr) DepthStencilState->Release ();

    //
    // Now balance the reference counts that D3D added even though we did not want them :P
    //
    for (UINT i = 0; i < VSInstancesCount; i++)
    {
      if (VSInstances [i] != nullptr)
          VSInstances [i]->Release ();
    }

    for (UINT i = 0; i < PSInstancesCount; i++)
    {
      if (PSInstances [i] != nullptr)
          PSInstances [i]->Release ();
    }

    if (ft_lvl >= D3D_FEATURE_LEVEL_10_0)
    {
      for (UINT i = 0; i < GSInstancesCount; i++)
      {
        if (GSInstances [i] != nullptr)
            GSInstances [i]->Release ();
      }
    }

    if (ft_lvl >= D3D_FEATURE_LEVEL_11_0)
    {
      for (UINT i = 0; i < HSInstancesCount; i++)
      {
        if (HSInstances [i] != nullptr)
            HSInstances [i]->Release ();
      }

      for (UINT i = 0; i < DSInstancesCount; i++)
      {
        if (DSInstances [i] != nullptr)
            DSInstances [i]->Release ();
      }
    }
  }
};

void
SK_D3D11_CaptureStateBlock ( ID3D11DeviceContext*       pImmediateContext,
                             SK_D3D11_Stateblock_Lite** pSB )
{
  if (pSB != nullptr)
  {
    if (*pSB == nullptr)
    {
      *pSB = new SK_D3D11_Stateblock_Lite ();
    }

    RtlSecureZeroMemory ( *pSB,
                      sizeof (SK_D3D11_Stateblock_Lite) );

    (*pSB)->capture (pImmediateContext);
  }
}

void
SK_D3D11_ApplyStateBlock ( SK_D3D11_Stateblock_Lite* pBlock,
                           ID3D11DeviceContext*      pDevCtx )
{
  if (pBlock != nullptr && pDevCtx != nullptr)
      pBlock->apply (pDevCtx);
}

SK_D3D11_Stateblock_Lite*
SK_D3D11_CreateAndCaptureStateBlock (ID3D11DeviceContext* pImmediateContext)
{
  ///// Uses TLS to reduce dynamic memory pressure as much as possible
  ///auto* sb =
    //SK_TLS_Bottom ()->d3d11.getStateBlock ();
  SK_D3D11_Stateblock_Lite* sb = new SK_D3D11_Stateblock_Lite ();

  RtlSecureZeroMemory ( sb,
                  sizeof (SK_D3D11_Stateblock_Lite) );

  sb->capture (pImmediateContext);

  return sb;
}

void
SK_D3D11_ReleaseAndApplyStateBlock ( SK_D3D11_Stateblock_Lite* pBlock,
                                     ID3D11DeviceContext*      pDevCtx )
{
  if (pBlock == nullptr)
    return;

         pBlock->apply (pDevCtx);
  delete pBlock;
}

void CreateStateblock (ID3D11DeviceContext* dc, D3DX11_STATE_BLOCK* sb)
{
  if (dc == nullptr)
    return;

  SK_ComPtr <ID3D11Device>
                  pDev;
  dc->GetDevice (&pDev.p);

  if ( pDev == nullptr )
    return;

  const D3D_FEATURE_LEVEL ft_lvl = pDev->GetFeatureLevel ();

  RtlSecureZeroMemory (sb, sizeof D3DX11_STATE_BLOCK);

  sb->OMBlendFactor [0] = 0.0f;
  sb->OMBlendFactor [1] = 0.0f;
  sb->OMBlendFactor [2] = 0.0f;
  sb->OMBlendFactor [3] = 0.0f;


  dc->VSGetShader          (&sb->VS, sb->VSInterfaces, &sb->VSInterfaceCount);

  dc->VSGetSamplers        (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->VSSamplers);
  dc->VSGetShaderResources (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->VSShaderResources);
  dc->VSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->VSConstantBuffers);

  if (ft_lvl >= D3D_FEATURE_LEVEL_10_0)
  {
    dc->GSGetShader          (&sb->GS, sb->GSInterfaces, &sb->GSInterfaceCount);

    dc->GSGetSamplers        (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->GSSamplers);
    dc->GSGetShaderResources (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->GSShaderResources);
    dc->GSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->GSConstantBuffers);
  }

  if (ft_lvl >= D3D_FEATURE_LEVEL_11_0)
  {
    dc->HSGetShader          (&sb->HS, sb->HSInterfaces, &sb->HSInterfaceCount);

    dc->HSGetSamplers        (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->HSSamplers);
    dc->HSGetShaderResources (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->HSShaderResources);
    dc->HSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->HSConstantBuffers);

    dc->DSGetShader          (&sb->DS, sb->DSInterfaces, &sb->DSInterfaceCount);
    dc->DSGetSamplers        (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->DSSamplers);
    dc->DSGetShaderResources (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->DSShaderResources);
    dc->DSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->DSConstantBuffers);
  }

  dc->PSGetShader          (&sb->PS, sb->PSInterfaces, &sb->PSInterfaceCount);

  dc->PSGetSamplers        (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->PSSamplers);
  dc->PSGetShaderResources (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->PSShaderResources);
  dc->PSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->PSConstantBuffers);

  if (ft_lvl >= D3D_FEATURE_LEVEL_11_0)
  {
    dc->CSGetShader          (&sb->CS, sb->CSInterfaces, &sb->CSInterfaceCount);

    dc->CSGetSamplers             (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->CSSamplers);
    dc->CSGetShaderResources      (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->CSShaderResources);
    dc->CSGetConstantBuffers      (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->CSConstantBuffers);
    dc->CSGetUnorderedAccessViews (0, D3D11_PS_CS_UAV_REGISTER_COUNT,                    sb->CSUnorderedAccessViews);
  }

  const int max_vtx_input_slots =
    ft_lvl >= D3D_FEATURE_LEVEL_11_0 ? D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT
                                     : D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;

  dc->IAGetVertexBuffers     (0, max_vtx_input_slots,                        sb->IAVertexBuffers,
                                                                             sb->IAVertexBuffersStrides,
                                                                             sb->IAVertexBuffersOffsets);
  dc->IAGetIndexBuffer       (&sb->IAIndexBuffer, &sb->IAIndexBufferFormat, &sb->IAIndexBufferOffset);
  dc->IAGetInputLayout       (&sb->IAInputLayout);
  dc->IAGetPrimitiveTopology (&sb->IAPrimitiveTopology);


  dc->OMGetBlendState        (&sb->OMBlendState,         sb->OMBlendFactor, &sb->OMSampleMask);
  dc->OMGetDepthStencilState (&sb->OMDepthStencilState, &sb->OMDepthStencilRef);

  dc->OMGetRenderTargets ( D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, sb->OMRenderTargets, &sb->OMRenderTargetStencilView );

  sb->RSViewportCount    = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;

  dc->RSGetViewports         (&sb->RSViewportCount, sb->RSViewports);

  sb->RSScissorRectCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;

  dc->RSGetScissorRects      (&sb->RSScissorRectCount, sb->RSScissorRects);
  dc->RSGetState             (&sb->RSRasterizerState);

  if (ft_lvl >= D3D_FEATURE_LEVEL_10_0)
  {
    dc->SOGetTargets         (4, sb->SOBuffers);
  }

  dc->GetPredication         (&sb->Predication, &sb->PredicationValue);
}

void ApplyStateblock (ID3D11DeviceContext* dc, D3DX11_STATE_BLOCK* sb)
{
  if (dc == nullptr)
    return;

  SK_ComPtr <ID3D11Device>
                  pDev;
  dc->GetDevice (&pDev.p);

  if (pDev.p == nullptr)
    return;

  const D3D_FEATURE_LEVEL ft_lvl = pDev->GetFeatureLevel ();

  dc->VSSetShader            (sb->VS, sb->VSInterfaces, sb->VSInterfaceCount);

  if (sb->VS != nullptr) sb->VS->Release ();

  for (UINT i = 0; i < sb->VSInterfaceCount; i++)
  {
    if (sb->VSInterfaces [i] != nullptr)
        sb->VSInterfaces [i]->Release ();
  }

  const UINT VSSamplerCount =
    calc_count               (sb->VSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);

  if (VSSamplerCount > 0)
  {
    dc->VSSetSamplers (0, VSSamplerCount, sb->VSSamplers);

    for (UINT i = 0; i <  VSSamplerCount; i++)
    {
      if (sb->VSSamplers [i] != nullptr)
          sb->VSSamplers [i]->Release ();
    }
  }

  const UINT VSShaderResourceCount =
    calc_count               (sb->VSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

  if (VSShaderResourceCount > 0)
  {
    dc->VSSetShaderResources (0, VSShaderResourceCount, sb->VSShaderResources);

    for (UINT i = 0; i <         VSShaderResourceCount; i++)
    {
      if (sb->VSShaderResources [i] != nullptr)
          sb->VSShaderResources [i]->Release ();
    }
  }

  const UINT VSConstantBufferCount =
    calc_count               (sb->VSConstantBuffers, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);

  if (VSConstantBufferCount > 0)
  {
    dc->VSSetConstantBuffers (0, VSConstantBufferCount, sb->VSConstantBuffers);

    for (UINT i = 0; i < VSConstantBufferCount; i++)
    {
      if (sb->VSConstantBuffers [i] != nullptr)
          sb->VSConstantBuffers [i]->Release ();
    }
  }


  if (ft_lvl >= D3D_FEATURE_LEVEL_10_0)
  {
    dc->GSSetShader       (sb->GS, sb->GSInterfaces, sb->GSInterfaceCount);

    if (sb->GS != nullptr) sb->GS->Release ();

    for (UINT i = 0; i < sb->GSInterfaceCount; i++)
    {
      if (sb->GSInterfaces [i] != nullptr)
          sb->GSInterfaces [i]->Release ();
    }

    const UINT GSSamplerCount =
      calc_count               (sb->GSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);

    if (GSSamplerCount > 0)
    {
      dc->GSSetSamplers (0, GSSamplerCount, sb->GSSamplers);

      for (UINT i = 0; i <  GSSamplerCount; i++)
      {
        if (sb->GSSamplers [i] != nullptr)
            sb->GSSamplers [i]->Release ();
      }
    }

    const UINT GSShaderResourceCount =
      calc_count               (sb->GSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

    if (GSShaderResourceCount > 0)
    {
      dc->GSSetShaderResources (0, GSShaderResourceCount, sb->GSShaderResources);

      for (UINT i = 0; i <         GSShaderResourceCount; i++)
      {
        if (sb->GSShaderResources [i] != nullptr)
            sb->GSShaderResources [i]->Release ();
      }
    }

    const UINT GSConstantBufferCount =
      calc_count               (sb->GSConstantBuffers, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);

    if (GSConstantBufferCount > 0)
    {
      dc->GSSetConstantBuffers (0, GSConstantBufferCount, sb->GSConstantBuffers);

      for (UINT i = 0; i < GSConstantBufferCount; i++)
      {
        if (sb->GSConstantBuffers [i] != nullptr)
            sb->GSConstantBuffers [i]->Release ();
      }
    }
  }


  if (ft_lvl >= D3D_FEATURE_LEVEL_11_0)
  {
    dc->HSSetShader       (sb->HS, sb->HSInterfaces, sb->HSInterfaceCount);

    if (sb->HS != nullptr) sb->HS->Release ();

    for (UINT i = 0; i < sb->HSInterfaceCount; i++)
    {
      if (sb->HSInterfaces [i] != nullptr)
          sb->HSInterfaces [i]->Release ();
    }

    const UINT HSSamplerCount =
      calc_count               (sb->HSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);

    if (HSSamplerCount > 0)
    {
      dc->HSSetSamplers (0, HSSamplerCount, sb->HSSamplers);

      for (UINT i = 0; i <  HSSamplerCount; i++)
      {
        if (sb->HSSamplers [i] != nullptr)
            sb->HSSamplers [i]->Release ();
      }
    }

    const UINT HSShaderResourceCount =
      calc_count               (sb->HSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

    if (HSShaderResourceCount > 0)
    {
      dc->HSSetShaderResources (0, HSShaderResourceCount, sb->HSShaderResources);

      for (UINT i = 0; i <         HSShaderResourceCount; i++)
      {
        if (sb->HSShaderResources [i] != nullptr)
            sb->HSShaderResources [i]->Release ();
      }
    }

    const UINT HSConstantBufferCount =
      calc_count               (sb->HSConstantBuffers, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);

    if (HSConstantBufferCount > 0)
    {
      dc->HSSetConstantBuffers (0, HSConstantBufferCount, sb->HSConstantBuffers);

      for (UINT i = 0; i <         HSConstantBufferCount; i++)
      {
        if (sb->HSConstantBuffers [i] != nullptr)
            sb->HSConstantBuffers [i]->Release ();
      }
    }


    dc->DSSetShader       (sb->DS, sb->DSInterfaces, sb->DSInterfaceCount);

    if (sb->DS != nullptr) sb->DS->Release ();

    for (UINT i = 0; i < sb->DSInterfaceCount; i++)
    {
      if (sb->DSInterfaces [i] != nullptr)
          sb->DSInterfaces [i]->Release ();
    }

    const UINT DSSamplerCount =
      calc_count               (sb->DSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);

    if (DSSamplerCount > 0)
    {
      dc->DSSetSamplers (0, DSSamplerCount, sb->DSSamplers);

      for (UINT i = 0; i <  DSSamplerCount; i++)
      {
        if (sb->DSSamplers [i] != nullptr)
            sb->DSSamplers [i]->Release ();
      }
    }

    const UINT DSShaderResourceCount =
      calc_count               (sb->DSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

    if (DSShaderResourceCount > 0)
    {
      dc->DSSetShaderResources (0, DSShaderResourceCount, sb->DSShaderResources);

      for (UINT i = 0; i <         DSShaderResourceCount; i++)
      {
        if (sb->DSShaderResources [i] != nullptr)
            sb->DSShaderResources [i]->Release ();
      }
    }

    const UINT DSConstantBufferCount =
      calc_count               (sb->DSConstantBuffers, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);

    if (DSConstantBufferCount > 0)
    {
      dc->DSSetConstantBuffers (0, DSConstantBufferCount, sb->DSConstantBuffers);

      for (UINT i = 0; i <         DSConstantBufferCount; i++)
      {
        if (sb->DSConstantBuffers [i] != nullptr)
            sb->DSConstantBuffers [i]->Release ();
      }
    }
  }


  dc->PSSetShader       (sb->PS, sb->PSInterfaces, sb->PSInterfaceCount);

  if (sb->PS != nullptr) sb->PS->Release ();

  for (UINT i = 0; i < sb->PSInterfaceCount; i++)
  {
    if (sb->PSInterfaces [i] != nullptr)
        sb->PSInterfaces [i]->Release ();
  }

  const UINT PSSamplerCount =
    calc_count               (sb->PSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);

  if (PSSamplerCount > 0)
  {
    dc->PSSetSamplers (0, PSSamplerCount, sb->PSSamplers);

    for (UINT i = 0; i <  PSSamplerCount; i++)
    {
      if (sb->PSSamplers [i] != nullptr)
          sb->PSSamplers [i]->Release ();
    }
  }

  const UINT PSShaderResourceCount =
    calc_count               (sb->PSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

  if (PSShaderResourceCount > 0)
  {
    dc->PSSetShaderResources (0, PSShaderResourceCount, sb->PSShaderResources);

    for (UINT i = 0; i <         PSShaderResourceCount; i++)
    {
      if (sb->PSShaderResources [i] != nullptr)
          sb->PSShaderResources [i]->Release ();
    }
  }

  const UINT PSConstantBufferCount =
    calc_count               (sb->PSConstantBuffers, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);

  if (PSConstantBufferCount > 0)
  {
    dc->PSSetConstantBuffers (0, PSConstantBufferCount, sb->PSConstantBuffers);

    for (UINT i = 0; i <         PSConstantBufferCount; i++)
    {
      if (sb->PSConstantBuffers [i] != nullptr)
          sb->PSConstantBuffers [i]->Release ();
    }
  }


  if (ft_lvl >= D3D_FEATURE_LEVEL_11_0)
  {
    dc->CSSetShader            (sb->CS, sb->CSInterfaces, sb->CSInterfaceCount);

    if (sb->CS != nullptr)
      sb->CS->Release ();

    for (UINT i = 0; i < sb->CSInterfaceCount; i++)
    {
      if (sb->CSInterfaces [i] != nullptr)
          sb->CSInterfaces [i]->Release ();
    }

    const UINT CSSamplerCount =
      calc_count               (sb->CSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);

    if (CSSamplerCount > 0)
    {
      dc->CSSetSamplers (0, CSSamplerCount, sb->CSSamplers);

      for (UINT i = 0; i <  CSSamplerCount; i++)
      {
        if (sb->CSSamplers [i] != nullptr)
            sb->CSSamplers [i]->Release ();
      }
    }

    const UINT CSShaderResourceCount =
      calc_count               (sb->CSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

    if (CSShaderResourceCount > 0)
    {
      dc->CSSetShaderResources (0, CSShaderResourceCount, sb->CSShaderResources);

      for (UINT i = 0; i <         CSShaderResourceCount; i++)
      {
        if (sb->CSShaderResources [i] != nullptr)
            sb->CSShaderResources [i]->Release ();
      }
    }

    const UINT CSConstantBufferCount =
      calc_count               (sb->CSConstantBuffers, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);

    if (CSConstantBufferCount > 0)
    {
      dc->CSSetConstantBuffers (0, CSConstantBufferCount, sb->CSConstantBuffers);

      for (UINT i = 0; i <         CSConstantBufferCount; i++)
      {
        if (sb->CSConstantBuffers [i] != nullptr)
            sb->CSConstantBuffers [i]->Release ();
      }
    }

    const UINT
      minus_one [D3D11_PS_CS_UAV_REGISTER_COUNT] =
      { std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max (),
        std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max (),
        std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max (),
        std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max () };

    dc->CSSetUnorderedAccessViews (0, D3D11_PS_CS_UAV_REGISTER_COUNT, sb->CSUnorderedAccessViews, minus_one);

    for (auto& CSUnorderedAccessView : sb->CSUnorderedAccessViews)
    {
      if (CSUnorderedAccessView != nullptr)
          CSUnorderedAccessView->Release ();
    }
  }


  const
  int max_vtx_input_slots =
    ft_lvl >= D3D_FEATURE_LEVEL_11_0 ? D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT
                                     : D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
  const UINT IAVertexBufferCount =
    calc_count               (sb->IAVertexBuffers, max_vtx_input_slots);

  if (IAVertexBufferCount > 0)
  {
    dc->IASetVertexBuffers (0, IAVertexBufferCount, sb->IAVertexBuffers,
                                                    sb->IAVertexBuffersStrides,
                                                    sb->IAVertexBuffersOffsets);

    for (UINT i = 0; i <       IAVertexBufferCount; i++)
    {
      if (sb->IAVertexBuffers [i] != nullptr)
          sb->IAVertexBuffers [i]->Release ();
    }
  }

  dc->IASetIndexBuffer       (sb->IAIndexBuffer, sb->IAIndexBufferFormat, sb->IAIndexBufferOffset);
  dc->IASetInputLayout       (sb->IAInputLayout);
  dc->IASetPrimitiveTopology (sb->IAPrimitiveTopology);

  if (sb->IAIndexBuffer != nullptr) sb->IAIndexBuffer->Release ();
  if (sb->IAInputLayout != nullptr) sb->IAInputLayout->Release ();


  dc->OMSetBlendState        (sb->OMBlendState,        sb->OMBlendFactor,
                                                       sb->OMSampleMask);
  dc->OMSetDepthStencilState (sb->OMDepthStencilState, sb->OMDepthStencilRef);

  if (sb->OMBlendState        != nullptr) sb->OMBlendState->Release        ();
  if (sb->OMDepthStencilState != nullptr) sb->OMDepthStencilState->Release ();

  const UINT OMRenderTargetCount =
    calc_count (sb->OMRenderTargets, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);

  if (                      OMRenderTargetCount > 0)
  {
    dc->OMSetRenderTargets (OMRenderTargetCount, sb->OMRenderTargets,
                                                 sb->OMRenderTargetStencilView);

    for (UINT i = 0; i <    OMRenderTargetCount; i++)
    {
      if (sb->OMRenderTargets [i] != nullptr)
          sb->OMRenderTargets [i]->Release ();
    }
  }

  if (sb->OMRenderTargetStencilView != nullptr)
      sb->OMRenderTargetStencilView->Release ();

  dc->RSSetViewports         (sb->RSViewportCount,     sb->RSViewports);
  dc->RSSetScissorRects      (sb->RSScissorRectCount,  sb->RSScissorRects);

  dc->RSSetState             (sb->RSRasterizerState);

  if (sb->RSRasterizerState != nullptr)
      sb->RSRasterizerState->Release ();

  if (ft_lvl >= D3D_FEATURE_LEVEL_10_0)
  {
    const UINT SOBufferCount =
      calc_count (sb->SOBuffers, 4);

    if ( SOBufferCount > 0 )
    {
      constexpr UINT                                     SOBuffersOffsets [4] = {   };
      dc->SOSetTargets (   SOBufferCount, sb->SOBuffers, SOBuffersOffsets);

      for (UINT i = 0; i < SOBufferCount; i++)
      {
        if (sb->SOBuffers [i] != nullptr)
            sb->SOBuffers [i]->Release ();
      }
    }
  }

  dc->SetPredication (sb->Predication, sb->PredicationValue);

  if (sb->Predication != nullptr)
      sb->Predication->Release ();
}

// The struct is implemented here, so that's where we allocate it.
SK_D3D11_Stateblock_Lite*
SK_D3D11_AllocStateBlock (size_t& size) noexcept
{
  size =
    sizeof (                SK_D3D11_Stateblock_Lite );
  return new (std::nothrow) SK_D3D11_Stateblock_Lite { };
}

void
SK_D3D11_FreeStateBlock (SK_D3D11_Stateblock_Lite* sb) noexcept
{
  assert (sb != nullptr);

  delete sb;
}