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


//#define SK_USE_D3D11_DEVICE_CTX_STATE

static const UINT
  minus_one [D3D11_PS_CS_UAV_REGISTER_COUNT] =
  { std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max (),
    std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max (),
    std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max (),
    std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max () };

void CreateStateblock (ID3D11DeviceContext* dc, D3DX11_STATE_BLOCK* sb)
{
  if (dc == nullptr)
    return;

  SK_ComPtr <ID3D11Device>
                  pDev;
  dc->GetDevice (&pDev.p);

  if ( pDev == nullptr )
    return;


  SK_ComPtr <ID3D11DeviceContext>        pUnwrapped;
  if (SUCCEEDED (dc->QueryInterface (IID_IUnwrappedD3D11DeviceContext,
                               (void **)&pUnwrapped.p)))
  {
    dc = pUnwrapped.p;
  }


  const D3D_FEATURE_LEVEL         ft_lvl   = pDev->GetFeatureLevel ();
#ifdef SK_USE_D3D11_DEVICE_CTX_STATE
  const D3D11_DEVICE_CONTEXT_TYPE ctx_type =   dc->GetType         ();
#endif

  *sb = { };


#ifdef SK_USE_D3D11_DEVICE_CTX_STATE
  // Use Device Context State if available
  if (ctx_type != D3D11_DEVICE_CONTEXT_DEFERRED)
  {
    SK_ComQIPtr <ID3D11Device1> 
        pDevice1 (pDev);
    if (pDevice1)
    {
      if ( SUCCEEDED (
             pDevice1->CreateDeviceContextState (
               ( pDev->GetCreationFlags () & D3D11_CREATE_DEVICE_SINGLETHREADED )
                                           ? D3D11_1_CREATE_DEVICE_CONTEXT_STATE_SINGLETHREADED
                                           : 0x0,
               &ft_lvl, 1, D3D11_SDK_VERSION, IID_ID3D11Device1, nullptr, &sb->DeviceContext )
                     )
         ) return;
    }
  }
#endif


  sb->OMBlendFactor [0] = 0.0f;
  sb->OMBlendFactor [1] = 0.0f;
  sb->OMBlendFactor [2] = 0.0f;
  sb->OMBlendFactor [3] = 0.0f;


  dc->VSGetShader (&sb->VS, sb->VSInterfaces, &sb->VSInterfaceCount);

  dc->VSGetSamplers        (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->VSSamplers);
  dc->VSGetShaderResources (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->VSShaderResources);
  dc->VSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->VSConstantBuffers);

  if (ft_lvl >= D3D_FEATURE_LEVEL_10_0)
  {
    dc->GSGetShader (&sb->GS, sb->GSInterfaces, &sb->GSInterfaceCount);

    dc->GSGetSamplers        (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->GSSamplers);
    dc->GSGetShaderResources (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->GSShaderResources);
    dc->GSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->GSConstantBuffers);
  }

  if (ft_lvl >= D3D_FEATURE_LEVEL_11_0)
  {
    dc->HSGetShader (&sb->HS, sb->HSInterfaces, &sb->HSInterfaceCount);

    dc->HSGetSamplers        (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->HSSamplers);
    dc->HSGetShaderResources (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->HSShaderResources);
    dc->HSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->HSConstantBuffers);

    dc->DSGetShader          (&sb->DS, sb->DSInterfaces, &sb->DSInterfaceCount);
    dc->DSGetSamplers        (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->DSSamplers);
    dc->DSGetShaderResources (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->DSShaderResources);
    dc->DSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->DSConstantBuffers);
  }

  dc->PSGetShader (&sb->PS, sb->PSInterfaces, &sb->PSInterfaceCount);

  dc->PSGetSamplers        (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->PSSamplers);
  dc->PSGetShaderResources (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->PSShaderResources);
  dc->PSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->PSConstantBuffers);

  if (ft_lvl >= D3D_FEATURE_LEVEL_11_0)
  {
    dc->CSGetShader (&sb->CS, sb->CSInterfaces, &sb->CSInterfaceCount);

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

  dc->OMGetRenderTargets     (D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, sb->OMRenderTargets, &sb->OMRenderTargetStencilView );
  sb->OMRenderTargetCount =                               calc_count (sb->OMRenderTargets,
                              D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);

  sb->OMUnorderedAccessViewCount =
    ft_lvl < D3D_FEATURE_LEVEL_11_0 ?
                                  0 :
    ft_lvl > D3D_FEATURE_LEVEL_11_0 ? D3D11_1_UAV_SLOT_COUNT
                                    : D3D11_PS_CS_UAV_REGISTER_COUNT;

  if (ft_lvl >= D3D_FEATURE_LEVEL_11_0)
  {
    dc->OMGetRenderTargetsAndUnorderedAccessViews
                             (0, nullptr, nullptr, sb->OMRenderTargetCount,
                                                   sb->OMUnorderedAccessViewCount - sb->OMRenderTargetCount,
                                                  &sb->OMUnorderedAccessViews [0]);
  }
  sb->OMUnorderedAccessViewCount
                            =                             calc_count (sb->OMUnorderedAccessViews,
                                                                      sb->OMUnorderedAccessViewCount) - sb->OMRenderTargetCount;

  sb->RSViewportCount     = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;

  dc->RSGetViewports         (&sb->RSViewportCount, sb->RSViewports);

  sb->RSScissorRectCount  = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;

  dc->RSGetScissorRects      (&sb->RSScissorRectCount, sb->RSScissorRects);
  dc->RSGetState             (&sb->RSRasterizerState);

  if (ft_lvl >= D3D_FEATURE_LEVEL_10_0)
  {
    dc->SOGetTargets         (D3D11_SO_BUFFER_SLOT_COUNT, sb->SOBuffers);
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

  SK_ComPtr <ID3D11DeviceContext>        pUnwrapped;
  if (SUCCEEDED (dc->QueryInterface (IID_IUnwrappedD3D11DeviceContext,
                               (void **)&pUnwrapped.p)))
  {
    dc = pUnwrapped.p;
  }

  const D3D_FEATURE_LEVEL         ft_lvl   = pDev->GetFeatureLevel ();
#ifdef SK_USE_D3D11_DEVICE_CTX_STATE
  const D3D11_DEVICE_CONTEXT_TYPE ctx_type =   dc->GetType         ();
#endif


#ifdef SK_USE_D3D11_DEVICE_CTX_STATE
  // Use Device Context State if available
  if (ctx_type != D3D11_DEVICE_CONTEXT_DEFERRED)
  {
    SK_ComQIPtr <ID3D11Device1>        pDev1  (pDev);
    SK_ComQIPtr <ID3D11DeviceContext1> pDevCtx1 (dc);

    if ( pDev1.p           != nullptr &&
         pDevCtx1.p        != nullptr &&
         sb->DeviceContext != nullptr )
    {
      pDevCtx1->SwapDeviceContextState (
         sb->DeviceContext, nullptr
      ); sb->DeviceContext->Release ();

      return;
    } 
  }
#endif

  if (ft_lvl >= D3D_FEATURE_LEVEL_11_0)
  {
    UINT UAVStartSlot = sb->OMUnorderedAccessViewCount > 0
                      ? sb->OMRenderTargetCount        : 0;

    dc->OMSetRenderTargetsAndUnorderedAccessViews (
      sb->OMRenderTargetCount, sb->OMRenderTargets,
                               sb->OMRenderTargetStencilView,
                                                           UAVStartSlot,
                               sb->OMUnorderedAccessViewCount,
                               sb->OMUnorderedAccessViews, minus_one );

    for (UINT i = 0; i <    sb->OMRenderTargetCount; i++)
    {
      if (sb->OMRenderTargets [i] != nullptr)
          sb->OMRenderTargets [i]->Release ();
    }

    for (UINT i = 0; i <    sb->OMUnorderedAccessViewCount; i++)
    {
      if (sb->OMUnorderedAccessViews [i] != nullptr)
          sb->OMUnorderedAccessViews [i]->Release ();
    }
  }

  else
  {
    dc->OMSetRenderTargets (sb->OMRenderTargetCount, sb->OMRenderTargets,
                                                     sb->OMRenderTargetStencilView);

    for (UINT i = 0; i <    sb->OMRenderTargetCount; i++)
    {
      if (sb->OMRenderTargets [i] != nullptr)
          sb->OMRenderTargets [i]->Release ();
    }
  }


  dc->VSSetShader (sb->VS, sb->VSInterfaces, sb->VSInterfaceCount);

  if (sb->VS != nullptr) sb->VS->Release ();

  for (UINT i = 0; i < sb->VSInterfaceCount; i++)
  {
    if (sb->VSInterfaces [i] != nullptr)
        sb->VSInterfaces [i]->Release ();
  }

  const UINT VSSamplerCount =
    D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;

  dc->VSSetSamplers (0, VSSamplerCount, sb->VSSamplers);
  for (UINT i = 0; i <  VSSamplerCount; i++)
  {
    if (sb->VSSamplers [i] != nullptr)
        sb->VSSamplers [i]->Release ();
  }

  const UINT VSShaderResourceCount =
    D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;

  dc->VSSetShaderResources (0, VSShaderResourceCount, sb->VSShaderResources);
  for (UINT i = 0; i <         VSShaderResourceCount; i++)
  {
    if (sb->VSShaderResources [i] != nullptr)
        sb->VSShaderResources [i]->Release ();
  }

  const UINT VSConstantBufferCount =
    D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;

  dc->VSSetConstantBuffers (0, VSConstantBufferCount, sb->VSConstantBuffers);
  for (UINT i = 0; i <         VSConstantBufferCount; i++)
  {
    if (sb->VSConstantBuffers [i] != nullptr)
        sb->VSConstantBuffers [i]->Release ();
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
      D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;

    dc->GSSetSamplers (0, GSSamplerCount, sb->GSSamplers);
    for (UINT i = 0; i <  GSSamplerCount; i++)
    {
      if (sb->GSSamplers [i] != nullptr)
          sb->GSSamplers [i]->Release ();
    }

    const UINT GSShaderResourceCount =
      D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;

    dc->GSSetShaderResources (0, GSShaderResourceCount, sb->GSShaderResources);
    for (UINT i = 0; i <         GSShaderResourceCount; i++)
    {
      if (sb->GSShaderResources [i] != nullptr)
          sb->GSShaderResources [i]->Release ();
    }

    const UINT GSConstantBufferCount =
      D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;

    dc->GSSetConstantBuffers (0, GSConstantBufferCount, sb->GSConstantBuffers);
    for (UINT i = 0; i <         GSConstantBufferCount; i++)
    {
      if (sb->GSConstantBuffers [i] != nullptr)
          sb->GSConstantBuffers [i]->Release ();
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
      D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;

    dc->HSSetSamplers (0, HSSamplerCount, sb->HSSamplers);
    for (UINT i = 0; i <  HSSamplerCount; i++)
    {
      if (sb->HSSamplers [i] != nullptr)
          sb->HSSamplers [i]->Release ();
    }
    

    const UINT HSShaderResourceCount =
      D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;

    dc->HSSetShaderResources (0, HSShaderResourceCount, sb->HSShaderResources);
    for (UINT i = 0; i <         HSShaderResourceCount; i++)
    {
      if (sb->HSShaderResources [i] != nullptr)
          sb->HSShaderResources [i]->Release ();
    }

    const UINT HSConstantBufferCount =
      D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;

    dc->HSSetConstantBuffers (0, HSConstantBufferCount, sb->HSConstantBuffers);
    for (UINT i = 0; i <         HSConstantBufferCount; i++)
    {
      if (sb->HSConstantBuffers [i] != nullptr)
          sb->HSConstantBuffers [i]->Release ();
    }
    


    dc->DSSetShader       (sb->DS, sb->DSInterfaces, sb->DSInterfaceCount);

    if (sb->DS != nullptr) sb->DS->Release ();

    for (UINT i = 0; i < sb->DSInterfaceCount; i++)
    {
      if (sb->DSInterfaces [i] != nullptr)
          sb->DSInterfaces [i]->Release ();
    }

    const UINT DSSamplerCount =
      D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;

    dc->DSSetSamplers (0, DSSamplerCount, sb->DSSamplers);
    for (UINT i = 0; i <  DSSamplerCount; i++)
    {
      if (sb->DSSamplers [i] != nullptr)
          sb->DSSamplers [i]->Release ();
    }
    

    const UINT DSShaderResourceCount =
      D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;

    dc->DSSetShaderResources (0, DSShaderResourceCount, sb->DSShaderResources);
    for (UINT i = 0; i <         DSShaderResourceCount; i++)
    {
      if (sb->DSShaderResources [i] != nullptr)
          sb->DSShaderResources [i]->Release ();
    }
    

    const UINT DSConstantBufferCount =
      D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;

    dc->DSSetConstantBuffers (0, DSConstantBufferCount, sb->DSConstantBuffers);
    for (UINT i = 0; i <         DSConstantBufferCount; i++)
    {
      if (sb->DSConstantBuffers [i] != nullptr)
          sb->DSConstantBuffers [i]->Release ();
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
    D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;

  dc->PSSetSamplers (0, PSSamplerCount, sb->PSSamplers);
  for (UINT i = 0; i <  PSSamplerCount; i++)
  {
    if (sb->PSSamplers [i] != nullptr)
        sb->PSSamplers [i]->Release ();
  }

  const UINT PSShaderResourceCount =
    D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;

  dc->PSSetShaderResources (0, PSShaderResourceCount, sb->PSShaderResources);
  for (UINT i = 0; i <         PSShaderResourceCount; i++)
  {
    if (sb->PSShaderResources [i] != nullptr)
        sb->PSShaderResources [i]->Release ();
  }

  const UINT PSConstantBufferCount =
    D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;

  dc->PSSetConstantBuffers (0, PSConstantBufferCount, sb->PSConstantBuffers);
  for (UINT i = 0; i <         PSConstantBufferCount; i++)
  {
    if (sb->PSConstantBuffers [i] != nullptr)
        sb->PSConstantBuffers [i]->Release ();
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
      D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;

    dc->CSSetSamplers (0, CSSamplerCount, sb->CSSamplers);
    for (UINT i = 0; i <  CSSamplerCount; i++)
    {
      if (sb->CSSamplers [i] != nullptr)
          sb->CSSamplers [i]->Release ();
    }
    

    const UINT CSShaderResourceCount =
      D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;

    dc->CSSetShaderResources (0, CSShaderResourceCount, sb->CSShaderResources);
    for (UINT i = 0; i <         CSShaderResourceCount; i++)
    {
      if (sb->CSShaderResources [i] != nullptr)
          sb->CSShaderResources [i]->Release ();
    }
    

    const UINT CSConstantBufferCount =
      D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;

    dc->CSSetConstantBuffers (0, CSConstantBufferCount, sb->CSConstantBuffers);
    for (UINT i = 0; i <         CSConstantBufferCount; i++)
    {
      if (sb->CSConstantBuffers [i] != nullptr)
          sb->CSConstantBuffers [i]->Release ();
    }
    

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
      max_vtx_input_slots;

  dc->IASetVertexBuffers (0, IAVertexBufferCount, sb->IAVertexBuffers,
                                                  sb->IAVertexBuffersStrides,
                                                  sb->IAVertexBuffersOffsets);
  for (UINT i = 0; i <       IAVertexBufferCount; i++)
  {
    if (sb->IAVertexBuffers [i] != nullptr)
        sb->IAVertexBuffers [i]->Release ();
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

  if (sb->OMRenderTargetStencilView != nullptr)
      sb->OMRenderTargetStencilView->Release ();

  sb->RSViewportCount    = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
  sb->RSScissorRectCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;

  dc->RSSetViewports         (sb->RSViewportCount,     sb->RSViewports);
  dc->RSSetScissorRects      (sb->RSScissorRectCount,  sb->RSScissorRects);

  dc->RSSetState             (sb->RSRasterizerState);

  if (sb->RSRasterizerState != nullptr)
      sb->RSRasterizerState->Release ();

  if (ft_lvl >= D3D_FEATURE_LEVEL_10_0)
  {
    const UINT SOBufferCount =
      D3D11_SO_BUFFER_SLOT_COUNT;

    constexpr UINT                                     SOBuffersOffsets [D3D11_SO_BUFFER_SLOT_COUNT] = { };
    dc->SOSetTargets (   SOBufferCount, sb->SOBuffers, SOBuffersOffsets);
    for (UINT i = 0; i < SOBufferCount; i++)
    {
      if (sb->SOBuffers [i] != nullptr)
          sb->SOBuffers [i]->Release ();
    }
  }

  dc->SetPredication (sb->Predication, sb->PredicationValue);

  if (sb->Predication != nullptr)
      sb->Predication->Release ();
}