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
#define NOMINMAX

#include <SpecialK/widgets/widget.h>
#include <SpecialK/gpu_monitor.h>
#include <SpecialK/dxgi_backend.h>
#include <SpecialK/render_backend.h>

#include <algorithm>

extern iSK_INI* osd_ini;

extern std::wstring
SK_CountToString (uint64_t count);

class SKWG_D3D11_Pipeline : public SK_Widget
{
public:
  SKWG_D3D11_Pipeline (void) : SK_Widget ("D3D11_Pipeline")
  {
    SK_ImGui_Widgets.d3d11_pipeline = this;

    setAutoFit (true).setDockingPoint (DockAnchor::West).setClickThrough (true);
  };

  void run (void)
  {
    if (! ( static_cast <int> (SK_GetCurrentRenderBackend ().api) &
            static_cast <int> (SK_RenderAPI::D3D11              ) ) )
    {
      setActive (false);
      return;
    }

    DWORD dwNow = timeGetTime ();

    if (last_update < dwNow - update_freq)
    {
      D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
        SK::DXGI::pipeline_stats_d3d11.last_results;

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

      last_update = dwNow;
    }
  }

  void draw (void)
  {
    if (! ( static_cast <int> (SK_GetCurrentRenderBackend ().api) &
            static_cast <int> (SK_RenderAPI::D3D11              ) ) )
    {
      setVisible (false);
      return;
    }

    ImGuiIO& io (ImGui::GetIO ( ));

    const  float font_size           =             ImGui::GetFont  ()->FontSize                        * io.FontGlobalScale;
    const  float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

    char szAvg  [512] = { };

    if (pipeline.vertex.verts_invoked.getAvg () > 0)
    {
      static uint64_t max_invoke = (                     static_cast <uint64_t> (pipeline.vertex.verts_invoked.getMax ()));
                      max_invoke = std::max (max_invoke, static_cast <uint64_t> (pipeline.vertex.verts_invoked.getMax ()));

      sprintf_s
        ( szAvg,
            512,
              u8"Vertex Invocations:\n\n\n"
              u8"          min: %ws Invocations,   max: %ws Invocations,   avg: %ws Invocations\n",
                  SK_CountToString   (static_cast <uint64_t> (pipeline.vertex.verts_invoked.getMin ())).c_str   (),
                    SK_CountToString (max_invoke).c_str                                                         (),
                      SK_CountToString (static_cast <uint64_t> (pipeline.vertex.verts_invoked.getAvg ())).c_str () );

      int samples = 
        std::min ( pipeline.vertex.verts_invoked.getUpdates  (),
                   pipeline.vertex.verts_invoked.getCapacity () );

      ImGui::PushStyleColor ( ImGuiCol_PlotLines, 
                                ImColor::HSV ( 0.31f - 0.31f *
                         std::min ( 1.0f, pipeline.vertex.verts_invoked.getMax () / static_cast <float> (max_invoke) ),
                                                 0.86f,
                                                   0.95 ) );

      ImGui::PlotLinesC ( "###Vtx_Assembly",
                         pipeline.vertex.verts_invoked.getValues ().data (),
                           samples,
                             pipeline.vertex.verts_invoked.getOffset     (),
                               szAvg,
                                 pipeline.vertex.verts_invoked.getMin    () / 2.0f,
               static_cast <float> (max_invoke)                             * 1.05f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvailWidth (), font_size * 4.5f) );

      static uint64_t max_verts = (                    static_cast <uint64_t> (pipeline.vertex.verts_input.getMax ()));
                      max_verts = std::max (max_verts, static_cast <uint64_t> (pipeline.vertex.verts_input.getMax ()));

      sprintf_s
        ( szAvg,
            512,
              u8"Vertices Input:\n\n\n"
              u8"          min: %ws Vertices,   max: %ws Vertices,   avg: %ws Vertices\n",
                  SK_CountToString     (static_cast <uint64_t> (pipeline.vertex.verts_input.getMin ())).c_str (),
                    SK_CountToString   (max_verts).c_str                                                      (),
                      SK_CountToString (static_cast <uint64_t> (pipeline.vertex.verts_input.getAvg ())).c_str () );

      samples = 
        std::min ( pipeline.vertex.verts_input.getUpdates  (),
                   pipeline.vertex.verts_input.getCapacity () );

      ImGui::PushStyleColor ( ImGuiCol_PlotLines, 
                                ImColor::HSV ( 0.31f - 0.31f *
                         std::min ( 1.0f, pipeline.vertex.verts_input.getMax () / static_cast <float> (max_verts) ),
                                                 0.86f,
                                                   0.95 ) );

      ImGui::PlotLinesC ( "###Vtx_Assembly",
                         pipeline.vertex.verts_input.getValues ().data (),
                           samples,
                             pipeline.vertex.verts_input.getOffset     (),
                               szAvg,
                                 pipeline.vertex.verts_input.getMin    () / 2.0f,
               static_cast <float> (max_verts)                            * 1.05f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvailWidth (), font_size * 4.5f) );

      static uint64_t max_prims = (                    static_cast <uint64_t> (pipeline.vertex.prims_input.getMax ()));
                      max_prims = std::max (max_prims, static_cast <uint64_t> (pipeline.vertex.prims_input.getMax ()));

      sprintf_s
        ( szAvg,
            512,
              u8"Primitives Assembled:\n\n\n"
              u8"          min: %ws Triangles,   max: %ws Triangles,   avg: %ws Triangles\n",
                  SK_CountToString     (static_cast <uint64_t> (pipeline.vertex.prims_input.getMin ())).c_str (),
                    SK_CountToString   (max_prims).c_str                                                      (),
                      SK_CountToString (static_cast <uint64_t> (pipeline.vertex.prims_input.getAvg ())).c_str () );

      samples = 
        std::min ( pipeline.vertex.prims_input.getUpdates  (),
                   pipeline.vertex.prims_input.getCapacity () );

      ImGui::PushStyleColor ( ImGuiCol_PlotLines, 
                                ImColor::HSV ( 0.31f - 0.31f *
                         std::min ( 1.0f, pipeline.vertex.prims_input.getMax () / static_cast <float> (max_prims) ),
                                                 0.86f,
                                                   0.95 ) );

      ImGui::PlotLinesC ( "###Prim_Assembly",
                         pipeline.vertex.prims_input.getValues ().data (),
                           samples,
                             pipeline.vertex.prims_input.getOffset     (),
                               szAvg,
                                 pipeline.vertex.prims_input.getMin    () / 2.0f,
               static_cast <float> (max_prims)                            * 1.05f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvailWidth (), font_size * 4.5f) );

      ImGui::PopStyleColor (3);
    }

    if (pipeline.raster.triangles_submitted.getAvg ( ) > 0)
    {
      //_swprintf ( wszDesc,
      //             L"%s  RASTER : %5.1f%% Filled     (%s Triangles IN )",
      //               wszDesc, 100.0f *
      //                   ( (float)stats.CPrimitives /
      //                     (float)stats.CInvocations ),
      //                 SK_CountToString (stats.CInvocations).c_str () );

      sprintf_s
        ( szAvg,
            512,
              u8"Raster Fill Rate:\n\n\n"
              u8"          min: %5.1f%%,   max: %5.1f%%,   avg: %5.1f%%\n",
                pipeline.raster.fill_ratio.getMin   (), pipeline.raster.fill_ratio.getMax (),
                  pipeline.raster.fill_ratio.getAvg () );

      int samples = 
        std::min ( pipeline.raster.fill_ratio.getUpdates  (),
                   pipeline.raster.fill_ratio.getCapacity () );

      static float max_ratio = (pipeline.raster.fill_ratio.getMax ());
                   max_ratio = std::max (max_ratio, pipeline.raster.fill_ratio.getMax ());

      ImGui::PushStyleColor ( ImGuiCol_PlotLines, 
                                ImColor::HSV ( 0.31f - 0.31f *
                         std::min ( 1.0f, ((100.0f - pipeline.raster.fill_ratio.getAvg ()) / 100.0f) ),
                                                 0.86f,
                                                   0.95 ) );

      ImGui::PlotLinesC ( "###Raster_Rate",
                         pipeline.raster.fill_ratio.getValues ().data (),
                           samples,
                             pipeline.raster.fill_ratio.getOffset     (),
                               szAvg,
                                 0.0f,//pipeline.raster.fill_ratio.getMin    () / 2.0f,
                                   100.0f,//max_ratio                             * 1.05f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvailWidth (), font_size * 4.5f), sizeof (float), 0.0, 0.0, 0.0, true );

      static uint64_t max_fill = (   static_cast <uint64_t> (pipeline.raster.pixels_filled.getMax ()) );
                      max_fill = std::max (
                                   max_fill,
                                     static_cast <uint64_t> (pipeline.raster.pixels_filled.getMax ())
                                 );


                      sprintf_s
        ( szAvg,
            512,
              u8"Pixels Shaded:\n\n\n"
              u8"          min: %ws Pixels,   max: %ws Pixels,   avg: %ws Pixels\n",
                SK_CountToString     (static_cast <uint64_t> (pipeline.raster.pixels_filled.getMin ())).c_str (),
                  SK_CountToString   (max_fill).c_str                                                         (),
                    SK_CountToString (static_cast <uint64_t> (pipeline.raster.pixels_filled.getAvg ())).c_str () );

      samples = 
        std::min ( pipeline.raster.pixels_filled.getUpdates  (),
                   pipeline.raster.pixels_filled.getCapacity () );

      ImGui::PushStyleColor ( ImGuiCol_PlotLines, 
                                ImColor::HSV ( 0.31f - 0.31f *
                         std::min ( 1.0f, pipeline.raster.pixels_filled.getMax () / max_fill ),
                                                 0.86f,
                                                   0.95 ) );

      ImGui::PlotLinesC ( "###Pixels_Filled",
                         pipeline.raster.pixels_filled.getValues ().data (),
                           samples,
                             pipeline.raster.pixels_filled.getOffset     (),
                               szAvg,
                                 pipeline.raster.pixels_filled.getMin    () / 2.0f,
               static_cast <float> (max_fill)                               * 1.05f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvailWidth (), font_size * 4.5f) );

      ImGui::PopStyleColor (2);
    }

    if (pipeline.compute.dispatches.getAvg () > 0)
    {
      static uint64_t max_dispatch = (   static_cast <uint64_t> (pipeline.compute.dispatches.getMax ()) );
                      max_dispatch = std::max (
                                       max_dispatch,
                                         static_cast <uint64_t> (pipeline.compute.dispatches.getMax ())
                                     );

      sprintf_s
        ( szAvg,
            512,
              u8"Compute Shader Invocations:\n\n\n"
              u8"          min: %ws Compels,   max: %ws Compels,   avg: %ws Compels\n",
                SK_CountToString     (static_cast <uint64_t> (pipeline.compute.dispatches.getMin ())).c_str (),
                  SK_CountToString   (max_dispatch).c_str                                                   (),
                    SK_CountToString (static_cast <uint64_t> (pipeline.compute.dispatches.getAvg ())).c_str () );

      int samples = 
        std::min ( pipeline.compute.dispatches.getUpdates  (),
                   pipeline.compute.dispatches.getCapacity () );

      ImGui::PushStyleColor ( ImGuiCol_PlotLines, 
                                ImColor::HSV ( 0.31f - 0.31f *
                         std::min ( 1.0f, pipeline.raster.pixels_filled.getMax () / max_dispatch ),
                                                 0.86f,
                                                   0.95 ) );

      ImGui::PlotLinesC ( "###Compels_Dispatched",
                         pipeline.compute.dispatches.getValues ().data (),
                           samples,
                             pipeline.compute.dispatches.getOffset     (),
                               szAvg,
                                 pipeline.compute.dispatches.getMin    () / 2.0f,
               static_cast <float> (max_dispatch)                         * 1.05f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvailWidth (), font_size * 4.5f) );

      ImGui::PopStyleColor (1);
    }
  }

  void OnConfig (ConfigEvent event)
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
      SK_ImGui_DataHistory <float, 600> verts_invoked;
      SK_ImGui_DataHistory <float, 600> verts_input;
      SK_ImGui_DataHistory <float, 600> prims_input;
      SK_ImGui_DataHistory <float, 600> gs_invokeed;
      SK_ImGui_DataHistory <float, 600> gs_output;
    } vertex;

    struct {
      SK_ImGui_DataHistory <float, 600> hull;
      SK_ImGui_DataHistory <float, 600> domain;
    } tessellation;

    struct {
      SK_ImGui_DataHistory <float, 600> fill_ratio;
      SK_ImGui_DataHistory <float, 600> triangles_submitted;
      SK_ImGui_DataHistory <float, 600> triangles_filled;
      SK_ImGui_DataHistory <float, 600> pixels_filled;
    } raster;

    struct
    {
      SK_ImGui_DataHistory <float, 600> dispatches;
    } compute;
  } pipeline;
} __d3d11_pipeline__;

