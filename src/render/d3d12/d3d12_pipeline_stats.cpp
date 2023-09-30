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
#include <../depends/include/DirectXTex/d3dx12.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"  D3D 12  "

////class SK_D3D12_PipelineQuery
////{
////public:
////  void reset (void)
////  {
////    //_heap.Release ();
////    results.clear ();
////
////    if (           fence.event != 0)
////    { CloseHandle (fence.event);
////                   fence.post.Release   ();
////                   fence.sequence.clear ();
////    }
////  }
////
////  void init  ( ID3D12Device *pDevice,
////               UINT          uiFrames )
////  {
////    D3D12_QUERY_HEAP_DESC
////    QueryHeapDesc          = {      };
////    QueryHeapDesc.Count    = uiFrames;
////    QueryHeapDesc.Type     = D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;
////
////    if (SUCCEEDED (
////          pDevice->CreateQueryHeap (
////                                  &QueryHeapDesc,
////            IID_PPV_ARGS (&PipelineQuery::_heap)
////          )
////        )                          )
////    {
////        _frameCount = uiFrames;
////      results.resize (uiFrames);
////
////      pDevice->CreateFence ( 0,
////               D3D12_FENCE_FLAG_NONE,
////      IID_PPV_ARGS (&fence.post.p) );
////
////      fence.event =
////        SK_CreateEvent ( nullptr, FALSE, FALSE, nullptr );
////    }
////  }
////
////  void issue ( ID3D12GraphicsCommandList* pList )
////  {
////    pList->BeginQuery (
////      SK_D3D12_PipelineQuery::_heap,
////          D3D12_QUERY_TYPE_PIPELINE_STATISTICS,
////            _currentFrame.fetch_add (1)
////    );
////  }
////
////
////
////private:
////  std::atomic_uint _currentFrame = 0;
////  std::atomic_uint _frameCount   = 0;
////
////  std::vector <
////    SK_ComPtr <ID3D12PipelineState>
////  > results;
////
////  struct {
////    SK_ComPtr   <ID3D12Fence> post;
////    std::vector <UINT64>      sequence;
////    HANDLE                    event;
////  } fence;
////
////  static SK_ComPtr <ID3D12QueryHeap> _heap;
////};

namespace SK
{
  namespace DXGI
  {
    struct PipelineStatsD3D12
    {
      SK_ComPtr <ID3D12QueryHeap> _heap;

      struct StatQueryD3D12
      {
        struct {
          SK_ComPtr <ID3D12Fence>        post;
          HANDLE                        event;
          std::atomic_uint64_t _issuedOnFrame;
          std::atomic_uint64_t _sampleOnFrame;
          std::atomic_uint64_t _resolvOnFrame;
        } fence;

        enum QueryPhase { None, Initiate, Sample };

        std::atomic_uint _currentBackBuffer =   0U;
        std::atomic_uint _currentDataPhase  = None;

        SK_ComPtr <ID3D12Resource> readbackData;
      } query;

      std::array < D3D12_QUERY_DATA_PIPELINE_STATISTICS,
                   DXGI_MAX_SWAP_CHAIN_BUFFERS
                 > _previous_results;

      void issue ( ID3D12GraphicsCommandList *pList,
                   IDXGISwapChain3           *pSwapChain )
      {
        query.fence._issuedOnFrame = SK_GetFramesDrawn ();
        query._currentBackBuffer   =
                pSwapChain->GetCurrentBackBufferIndex ();

        pList->BeginQuery ( _heap,
              D3D12_QUERY_TYPE_PIPELINE_STATISTICS,
                query._currentBackBuffer.load () );
      }

      void sample ( ID3D12GraphicsCommandList *pList/*,
                    IDXGISwapChain3           *pSwapChain*/ )
      {
        query.fence._sampleOnFrame =
                       SK_GetFramesDrawn ();

        pList->EndQuery ( _heap,
              D3D12_QUERY_TYPE_PIPELINE_STATISTICS,
                        query._currentBackBuffer.load () );

        pList->ResolveQueryData ( _heap,
              D3D12_QUERY_TYPE_PIPELINE_STATISTICS,
                    query._currentBackBuffer.load (), 1,
                    query.readbackData,               0 );

        query.fence.post->Signal ( query.fence._sampleOnFrame );
        query.fence.post->SetEventOnCompletion (
                                   query.fence._sampleOnFrame,
                                   query.fence.event
                                               );
      }

      D3D12_QUERY_DATA_PIPELINE_STATISTICS&
      readback ( void )
      {
        query.fence._resolvOnFrame =
               SK_GetFramesDrawn ();

        D3D12_RANGE r = {
          0, sizeof (D3D12_QUERY_DATA_PIPELINE_STATISTICS)
        };

        void *pData = nullptr;

        if (SUCCEEDED (query.readbackData->Map (0, &r, &pData)))
        {
          memcpy (&last_results, pData, sizeof (D3D12_QUERY_DATA_PIPELINE_STATISTICS));

          query.readbackData->Unmap (0, nullptr);
        }

        return last_results;
      }

      HRESULT init (ID3D12Device *pDevice)
      {
        D3D12_QUERY_HEAP_DESC
              query_heap_desc       = {                                       };
              query_heap_desc.Type  = D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;
              query_heap_desc.Count =               DXGI_MAX_SWAP_CHAIN_BUFFERS;


        HRESULT hr =
          pDevice->CreateQueryHeap ( &query_heap_desc,
                            IID_PPV_ARGS (&_heap)
          );

        if (FAILED (hr))
             return hr;

        hr = pDevice->CreateFence ( 0,
                      D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(&query.fence.post.p) );

        if (FAILED (hr))
        {
          _heap.Release ();
          return    hr;
        }

        CD3DX12_HEAP_PROPERTIES readback     (D3D12_HEAP_TYPE_READBACK);
        CD3DX12_RESOURCE_DESC   readbackDesc (CD3DX12_RESOURCE_DESC::Buffer (sizeof (D3D12_QUERY_DATA_PIPELINE_STATISTICS)));

        hr =
          pDevice->CreateCommittedResource ( &readback,
                       D3D12_HEAP_FLAG_NONE, &readbackDesc, D3D12_RESOURCE_STATE_COPY_DEST,
                                              nullptr, IID_PPV_ARGS (&query.readbackData.p)
                                           );

        if (FAILED (hr))
          return    hr;

        query.fence.event =
          SK_CreateEvent ( nullptr, FALSE, FALSE, nullptr );

        return
          query.fence.event != 0 ? S_OK
                                 : E_HANDLE;
      }

      D3D12_QUERY_DATA_PIPELINE_STATISTICS last_results;
    } pipeline_stats_d3d12;
  };
};

void
__stdcall
SK_D3D12_UpdateRenderStatsEx (ID3D12GraphicsCommandList *pList, IDXGISwapChain3 *pSwapChain)
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  SK_ReleaseAssert (rb.api == SK_RenderAPI::D3D12);

  auto pDev =
    rb.getDevice <ID3D12Device>                 (                      );
  SK_ComQIPtr    <ID3D12CommandQueue> pCmdQueue (rb.d3d12.command_queue);

  if ( pDev       == nullptr ||
       pCmdQueue  == nullptr ||
       pSwapChain == nullptr ) return;

  SK_ReleaseAssert (rb.swapchain == pSwapChain);

  SK::DXGI::PipelineStatsD3D12& pipeline_stats =
                      SK::DXGI::pipeline_stats_d3d12;

  SK_ComPtr <ID3D12Device>         pDevice;
  pList->GetDevice (IID_PPV_ARGS (&pDevice.p));

  SK_RunOnce (pipeline_stats.init (pDevice));

  switch ( pipeline_stats.query._currentDataPhase.load () )
  {
    case SK::DXGI::PipelineStatsD3D12::StatQueryD3D12::Initiate:
      pipeline_stats.sample (pList);
      pipeline_stats.query._currentDataPhase.store (SK::DXGI::PipelineStatsD3D12::StatQueryD3D12::Sample);
      break;

    case SK::DXGI::PipelineStatsD3D12::StatQueryD3D12::Sample:
      if (pipeline_stats.query.fence._resolvOnFrame.load () != SK_GetFramesDrawn ())
      {
        if (WAIT_OBJECT_0 == SK_WaitForSingleObject (pipeline_stats.query.fence.event, 0))
        {
          pipeline_stats.readback ();
          pipeline_stats.query._currentDataPhase.store (SK::DXGI::PipelineStatsD3D12::StatQueryD3D12::None);
        }
      }
      break;

    default:
    case SK::DXGI::PipelineStatsD3D12::StatQueryD3D12::None:
      if (pipeline_stats.query.fence._resolvOnFrame != SK_GetFramesDrawn ())
      {
        pipeline_stats.issue (pList, pSwapChain);
        pipeline_stats.query._currentDataPhase.store (SK::DXGI::PipelineStatsD3D12::StatQueryD3D12::Initiate);
      }
      break;
  }
}

void
__stdcall
SK_D3D12_UpdateRenderStats (ID3D12GraphicsCommandList *pList, IDXGISwapChain3* pSwapChain)
{
  return;

  //if (! (pSwapChain && ( config.render.show ||
  //                        ( SK_ImGui_Widgets->d3d12_pipeline != nullptr &&
  //                        ( SK_ImGui_Widgets->d3d12_pipeline->isActive () ) ) ) ) )
  //  return;

  SK_D3D12_UpdateRenderStatsEx (pList, pSwapChain);
}










extern std::string
SK_CountToString (uint64_t count);


class SKWG_D3D12_Pipeline : public SK_Widget
{
public:
  SKWG_D3D12_Pipeline (void) : SK_Widget ("D3D1x_Pipeline")
  {
    //SK_ImGui_Widgets->d3d12_pipeline = this;

    setAutoFit (true).setDockingPoint (DockAnchor::West).setClickThrough (true);
  };

  void run (void) noexcept override
  {
    if ( ( static_cast <int> (SK_GetCurrentRenderBackend ().api) &
           static_cast <int> (SK_RenderAPI::D3D12) ) !=
           static_cast <int> (SK_RenderAPI::D3D12) )
    {
      setActive (false);
      return;
    }

    if (last_update < SK::ControlPanel::current_time - update_freq)
    {
      D3D12_QUERY_DATA_PIPELINE_STATISTICS& stats =
        SK::DXGI::pipeline_stats_d3d12.last_results;

      pipeline.raster.fill_ratio.addValue          ( 100.0f * static_cast <float> (stats.CPrimitives) /
                                                              static_cast <float> (stats.CInvocations),  false );
      pipeline.raster.triangles_submitted.addValue (          static_cast <float> (stats.CInvocations),  false );
      pipeline.raster.pixels_filled.addValue       (          static_cast <float> (stats.PSInvocations), false );
      pipeline.raster.triangles_filled.addValue    (          static_cast <float> (stats.CPrimitives),   false );


      pipeline.tessellation.hull.addValue          (static_cast <float> (stats.HSInvocations), false);
      pipeline.tessellation.domain.addValue        (static_cast <float> (stats.DSInvocations), false);

      pipeline.vertex.verts_input.addValue         (static_cast <float> (stats.IAVertices),    false);
      pipeline.vertex.prims_input.addValue         (static_cast <float> (stats.IAPrimitives),  false);
      pipeline.vertex.verts_invoked.addValue       (static_cast <float> (stats.VSInvocations), false);

      pipeline.vertex.gs_invokeed.addValue         (static_cast <float> (stats.GSInvocations), false);
      pipeline.vertex.gs_output.addValue           (static_cast <float> (stats.GSPrimitives),  false);

      pipeline.compute.dispatches.addValue         (static_cast <float> (stats.CSInvocations), false);

      last_update = SK::ControlPanel::current_time;
    }
  }

  void draw (void) noexcept override
  {
    if ( ( static_cast <int> (SK_GetCurrentRenderBackend ().api) &
           static_cast <int> (SK_RenderAPI::D3D12) ) !=
           static_cast <int> (SK_RenderAPI::D3D12) )
    {
      setVisible (false);
      return;
    }

    if (! ImGui::GetFont ()) return;

    const  float font_size           =             ImGui::GetFont  ()->FontSize;//                        * scale;
#ifdef _ProperSpacing
    const  float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;
#endif

    char szAvg  [512] = { };

    if (pipeline.vertex.verts_invoked.getAvg () > 0)
    {
      static uint64_t max_invoke = (                      static_cast <uint64_t>    (pipeline.vertex.verts_invoked.getMax ()));
                      max_invoke = static_cast<uint64_t> (static_cast <long double> (max_invoke) * 0.8888f);
                      max_invoke = std::max (max_invoke,  static_cast <uint64_t>    (pipeline.vertex.verts_invoked.getMax ()));

      snprintf
        ( szAvg,
            511,
              "Vertex Invocations:\n\n\n"
              "          min: %s Invocations,   max: %s Invocations,   avg: %s Invocations\n",
                  SK_CountToString   (static_cast <uint64_t> (pipeline.vertex.verts_invoked.getMin ())).c_str   (),
                    SK_CountToString (max_invoke).c_str                                                         (),
                      SK_CountToString (static_cast <uint64_t> (pipeline.vertex.verts_invoked.getAvg ())).c_str () );

      int samples =
        std::min ( pipeline.vertex.verts_invoked.getUpdates  (),
                   pipeline.vertex.verts_invoked.getCapacity () );

      ImGui::PlotLinesC ( "###Vtx_Assembly",
                         pipeline.vertex.verts_invoked.getValues ().data (),
                           samples,
                             pipeline.vertex.verts_invoked.getOffset     (),
                               szAvg,
                                 pipeline.vertex.verts_invoked.getMin    () / 2.0f,
               static_cast <float> (max_invoke)                             * 1.05f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvail ().x, font_size * 4.5f),
                                         sizeof (float), 0.0f, static_cast <float> (max_invoke) );

      static uint64_t max_verts = (                     static_cast <uint64_t>    (pipeline.vertex.verts_input.getMax ()));
                      max_verts = static_cast<uint64_t>(static_cast <long double> (max_verts) * 0.8888f);
                      max_verts = std::max (max_verts,  static_cast <uint64_t>    (pipeline.vertex.verts_input.getMax ()));

      snprintf
        ( szAvg,
            511,
              "Vertices Input:\n\n\n"
              "          min: %s Vertices,   max: %s Vertices,   avg: %s Vertices\n",
                  SK_CountToString     (static_cast <uint64_t> (pipeline.vertex.verts_input.getMin ())).c_str (),
                    SK_CountToString   (max_verts).c_str                                                      (),
                      SK_CountToString (static_cast <uint64_t> (pipeline.vertex.verts_input.getAvg ())).c_str () );

      samples =
        std::min ( pipeline.vertex.verts_input.getUpdates  (),
                   pipeline.vertex.verts_input.getCapacity () );

      ImGui::PlotLinesC ( "###Vtx_Assembly",
                         pipeline.vertex.verts_input.getValues ().data (),
                           samples,
                             pipeline.vertex.verts_input.getOffset     (),
                               szAvg,
                                 pipeline.vertex.verts_input.getMin    () / 2.0f,
               static_cast <float> (max_verts)                            * 1.05f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvail ().x, font_size * 4.5f),
                                         sizeof (float), 0.0f, static_cast <float> (max_verts) );

      static uint64_t max_prims = (                     static_cast <uint64_t>    (pipeline.vertex.prims_input.getMax ()));
                      max_prims = static_cast<uint64_t>(static_cast <long double> (max_prims) * 0.8888f);
                      max_prims = std::max (max_prims,  static_cast <uint64_t>    (pipeline.vertex.prims_input.getMax ()));

      snprintf
        ( szAvg,
            511,
              "Primitives Assembled:\n\n\n"
              "          min: %s Triangles,   max: %s Triangles,   avg: %s Triangles\n",
                  SK_CountToString     (static_cast <uint64_t> (pipeline.vertex.prims_input.getMin ())).c_str (),
                    SK_CountToString   (max_prims).c_str                                                      (),
                      SK_CountToString (static_cast <uint64_t> (pipeline.vertex.prims_input.getAvg ())).c_str () );

      samples =
        std::min ( pipeline.vertex.prims_input.getUpdates  (),
                   pipeline.vertex.prims_input.getCapacity () );

      ImGui::PlotLinesC ( "###Prim_Assembly",
                         pipeline.vertex.prims_input.getValues ().data (),
                           samples,
                             pipeline.vertex.prims_input.getOffset     (),
                               szAvg,
                                 pipeline.vertex.prims_input.getMin    () / 2.0f,
               static_cast <float> (max_prims)                            * 1.05f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvail ().x, font_size * 4.5f),
                                         sizeof (float), 0.0f, static_cast <float> (max_prims) );
    }

    if (pipeline.raster.triangles_submitted.getAvg ( ) > 0)
    {
      //_swprintf ( wszDesc,
      //             L"%s  RASTER : %5.1f%% Filled     (%s Triangles IN )",
      //               wszDesc, 100.0f *
      //                   ( (float)stats.CPrimitives /
      //                     (float)stats.CInvocations ),
      //                 SK_CountToString (stats.CInvocations).c_str () );

      snprintf
        ( szAvg,
            511,
              "Raster Fill Rate:\n\n\n"
              "          min: %5.1f%%,   max: %5.1f%%,   avg: %5.1f%%\n",
                pipeline.raster.fill_ratio.getMin   (), pipeline.raster.fill_ratio.getMax (),
                  pipeline.raster.fill_ratio.getAvg () );

      int samples =
        std::min ( pipeline.raster.fill_ratio.getUpdates  (),
                   pipeline.raster.fill_ratio.getCapacity () );

      static float max_ratio = (pipeline.raster.fill_ratio.getMax ());
                   max_ratio = std::fmax (max_ratio, pipeline.raster.fill_ratio.getMax ());

      ImGui::PlotLinesC ( "###Raster_Rate",
                         pipeline.raster.fill_ratio.getValues ().data (),
                           samples,
                             pipeline.raster.fill_ratio.getOffset     (),
                               szAvg,
                                 0.0f,//pipeline.raster.fill_ratio.getMin    () / 2.0f,
                                   100.0f,//max_ratio                             * 1.05f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvail ().x, font_size * 4.5f), sizeof (float),
                                         0.0, 100.0, 0.0, true );

      static uint64_t max_fill = (   static_cast <uint64_t> (pipeline.raster.pixels_filled.getMax ()) );
                      max_fill =      static_cast<uint64_t> (static_cast <long double> (max_fill) * 0.8888f);
                      max_fill = std::max (
                                   max_fill,
                                     static_cast <uint64_t> (pipeline.raster.pixels_filled.getMax ())
                                 );


      snprintf
        ( szAvg,
            511,
              "Pixels Shaded:\n\n\n"
              "          min: %s Pixels,   max: %s Pixels,   avg: %s Pixels\n",
                SK_CountToString     (static_cast <uint64_t> (pipeline.raster.pixels_filled.getMin ())).c_str (),
                  SK_CountToString   (max_fill).c_str                                                         (),
                    SK_CountToString (static_cast <uint64_t> (pipeline.raster.pixels_filled.getAvg ())).c_str () );

      samples =
        std::min ( pipeline.raster.pixels_filled.getUpdates  (),
                   pipeline.raster.pixels_filled.getCapacity () );

      ImGui::PlotLinesC ( "###Pixels_Filled",
                         pipeline.raster.pixels_filled.getValues ().data (),
                           samples,
                             pipeline.raster.pixels_filled.getOffset     (),
                               szAvg,
                                 pipeline.raster.pixels_filled.getMin    () / 2.0f,
               static_cast <float> (max_fill)                               * 1.05f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvail ().x, font_size * 4.5f),
                                         sizeof (float), 0.0f, static_cast <float> (max_fill) );
    }

    if (pipeline.compute.dispatches.getAvg () > 0)
    {
      static uint64_t max_dispatch = (   static_cast <uint64_t> (pipeline.compute.dispatches.getMax ()) );
                      max_dispatch =     static_cast <uint64_t> (static_cast <double> (max_dispatch) * 0.8888);
                      max_dispatch = std::max (
                                       max_dispatch,
                                         static_cast <uint64_t> (pipeline.compute.dispatches.getMax ())
                                     );

      snprintf
        ( szAvg,
            511,
              "Compute Shader Invocations:\n\n\n"
              "          min: %s Compels,   max: %s Compels,   avg: %s Compels\n",
                SK_CountToString     (static_cast <uint64_t> (pipeline.compute.dispatches.getMin ())).c_str (),
                  SK_CountToString   (max_dispatch).c_str                                                   (),
                    SK_CountToString (static_cast <uint64_t> (pipeline.compute.dispatches.getAvg ())).c_str () );

      int samples =
        std::min ( pipeline.compute.dispatches.getUpdates  (),
                   pipeline.compute.dispatches.getCapacity () );

      ImGui::PlotLinesC ( "###Compels_Dispatched",
                         pipeline.compute.dispatches.getValues ().data (),
                           samples,
                             pipeline.compute.dispatches.getOffset     (),
                               szAvg,
                                 pipeline.compute.dispatches.getMin    () / 2.0f,
               static_cast <float> (max_dispatch)                         * 1.05f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvail ().x, font_size * 4.5f),
                                         sizeof (float), 0.0f, static_cast <float> (max_dispatch) );
    }
  }

  void OnConfig (ConfigEvent event) noexcept override
  {
    switch (event)
    {
      case SK_Widget::ConfigEvent::LoadComplete:
        break;

      case SK_Widget::ConfigEvent::SaveStart:
        break;
    }
  }

protected:
  const DWORD update_freq = 4UL;

private:
  DWORD last_update = 0UL;

  struct {
    struct {
      SK_Stat_DataHistory <float, 600> verts_invoked;
      SK_Stat_DataHistory <float, 600> verts_input;
      SK_Stat_DataHistory <float, 600> prims_input;
      SK_Stat_DataHistory <float, 600> gs_invokeed;
      SK_Stat_DataHistory <float, 600> gs_output;
    } vertex;

    struct {
      SK_Stat_DataHistory <float, 600> hull;
      SK_Stat_DataHistory <float, 600> domain;
    } tessellation;

    struct {
      SK_Stat_DataHistory <float, 600> fill_ratio;
      SK_Stat_DataHistory <float, 600> triangles_submitted;
      SK_Stat_DataHistory <float, 600> triangles_filled;
      SK_Stat_DataHistory <float, 600> pixels_filled;
    } raster;

    struct
    {
      SK_Stat_DataHistory <float, 600> dispatches;
    } compute;
  } pipeline;
};

SKWG_D3D12_Pipeline* SK_Widget_GetD3D12Pipeline (void)
{
  static SKWG_D3D12_Pipeline  __d3d12_pipeline__;
  return                     &__d3d12_pipeline__;
}