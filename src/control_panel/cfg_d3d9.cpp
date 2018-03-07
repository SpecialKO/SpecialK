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

#include <imgui/imgui.h>

#include <SpecialK/config.h>

#include <SpecialK/control_panel.h>
#include <SpecialK/control_panel/d3d9.h>
#include <SpecialK/control_panel/osd.h>

#include <SpecialK/render/d3d9/d3d9_backend.h>
#include <SpecialK/render/d3d9/d3d9_swapchain.h>
#include <SpecialK/render/d3d9/d3d9_texmgr.h>

extern bool __remap_textures; // RENAME

using namespace SK::ControlPanel;

bool SK::ControlPanel::D3D9::show_shader_mod_dlg = false;
extern bool              SK_D3D9_TextureModDlg (void);

bool
SK::ControlPanel::D3D9::Draw (void)
{
  if (show_shader_mod_dlg)
    show_shader_mod_dlg = SK_D3D9_TextureModDlg ();


  if ( static_cast <int> (SK::ControlPanel::render_api) & static_cast <int> (SK_RenderAPI::D3D9) &&
       ImGui::CollapsingHeader ("Direct3D 9 Settings", ImGuiTreeNodeFlags_DefaultOpen) )
  {
    ImGui::TreePush ("");

    if (ImGui::Button ("  D3D9 Render Mod Tools  "))
      show_shader_mod_dlg ^= 1;

    ImGui::SameLine ();

    ImGui::Checkbox ("Enable Texture Modding (Experimental)", &config.textures.d3d9_mod);

    if (ImGui::IsItemHovered ())
    {
      ImGui::SetTooltip ("Requires a game restart.");
    }

    ImGui::SameLine ();

    if (ImGui::Checkbox ("Enable CEGUI", &config.cegui.enable))
    {
      SK_CEGUI_QueueResetD3D9 ();
    }

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip    ();
      ImGui::TextUnformatted ("Disabling may resolve graphics issues, but will disable achievement pop-ups and OSD text.");
      ImGui::EndTooltip      ();
    }

    // This only works when we have wrapped SwapChains
    if ( ReadAcquire (&SK_D3D9_LiveWrappedSwapChains) ||
         ReadAcquire (&SK_D3D9_LiveWrappedSwapChainsEx) )
    {
      ImGui::SameLine              ();
      OSD::DrawVideoCaptureOptions ();
    }

    if (config.textures.d3d9_mod)
    {
    ImGui::TreePush ("");
    if (ImGui::CollapsingHeader ("Texture Memory Stats", ImGuiTreeNodeFlags_DefaultOpen))
    {
      SK::D3D9::TextureManager& tex_mgr =
        SK_D3D9_GetTextureManager ();

      ImGui::PushStyleVar (ImGuiStyleVar_ChildWindowRounding, 15.0f);
      ImGui::TreePush     ("");

      ImGui::BeginChild  ("Texture Details", ImVec2 ( font.size           * 30,
                                                      font.size_multiline * 6.0f ),
                                               true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

      ImGui::Columns   ( 3 );
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::Text    ( "          Size" );                                                                 ImGui::NextColumn ();
        ImGui::Text    ( "      Activity" );                                                                 ImGui::NextColumn ();
        ImGui::Text    ( "       Savings" );
        ImGui::PopStyleColor  ();
      ImGui::Columns   ( 1 );

      ImGui::PushStyleColor
                       ( ImGuiCol_Text, ImVec4 (0.75f, 0.75f, 0.75f, 1.0f) );

      ImGui::Separator (   );

    //ImGui::PushFont  (ImGui::GetIO ().Fonts->Fonts [1]);
      ImGui::Columns   ( 3 );
        ImGui::Text    ( "%#6zu MiB Total",
                                                       tex_mgr.cacheSizeTotal () >> 20ULL ); ImGui::NextColumn ();

        static DWORD  dwLastVRAMUpdate   = 0UL;
        static size_t d3d9_tex_mem_avail = 0UL;

        if (dwLastVRAMUpdate < current_time - 1500)
        {
          CComQIPtr <IDirect3DDevice9> pDev (SK_GetCurrentRenderBackend ().device);

          if (pDev != nullptr)
          {
            d3d9_tex_mem_avail =
              pDev->GetAvailableTextureMem () / 1048576UL;
            dwLastVRAMUpdate = current_time;
          }
        }

        ImGui::TextColored
                       (ImVec4 (0.3f, 1.0f, 0.3f, 1.0f),
                         "%#5lu     Hits",             tex_mgr.getHitCount    ()          ); ImGui::NextColumn ();
        ImGui::Text       ( "Budget: %#7zu MiB  ",     d3d9_tex_mem_avail );
      ImGui::Columns   ( 1 );

      ImGui::Separator (   );

      ImColor  active   ( 1.0f,  1.0f,  1.0f, 1.0f);
      ImColor  inactive (0.75f, 0.75f, 0.75f, 1.0f);
      ImColor& color   = __remap_textures ? inactive : active;

      ImGui::PushStyleColor (ImGuiCol_Text, color);

      bool selected = (! __remap_textures);

      ImGui::Columns   ( 3 );
        ImGui::Selectable  ( SK_FormatString ( "%#6zu MiB Base###D3D9_BaseTextures",
                                                 tex_mgr.cacheSizeBasic () >> 20ULL ).c_str (),
                               &selected ); ImGui::NextColumn ();

        ImGui::PopStyleColor ();

        if (SK_ImGui_IsItemClicked ())
          __remap_textures = false;
        if ((! selected) && (ImGui::IsItemHovered () || ImGui::IsItemFocused ()))
          ImGui::SetTooltip ("Click here to use the game's original textures.");

        ImGui::TextColored
                       (ImVec4 (1.0f, 0.3f, 0.3f, 1.0f),
                         "%#5lu   Misses",             tex_mgr.getMissCount   ()          );  ImGui::NextColumn ();
        ImGui::Text    ( "Time:    %#7.03lf  s  ",     tex_mgr.getTimeSaved   () / 1000.0f);
      ImGui::Columns   ( 1 );

      ImGui::Separator (   );

      color    = __remap_textures ? active : inactive;
      selected = __remap_textures;

      ImGui::PushStyleColor (ImGuiCol_Text, color);

      ImGui::Columns   ( 3 );
        ImGui::Selectable  ( SK_FormatString ( "%#6zu MiB Injected###D3D9_InjectedTextures",
                                                           tex_mgr.cacheSizeInjected () >> 20ULL ).c_str (),
                                         &selected ); ImGui::NextColumn ();

        ImGui::PopStyleColor ();

        if (SK_ImGui_IsItemClicked ())
          __remap_textures = true;
        if ((! selected) && (ImGui::IsItemHovered () || ImGui::IsItemFocused ()))
          ImGui::SetTooltip ("Click here to use custom textures.");

        ImGui::TextColored (ImColor::HSV (std::min ( 0.4f * (float)tex_mgr.getHitCount  ()   / 
                                                            (float)tex_mgr.getMissCount (), 0.4f ), 0.98f, 1.0f),
                         "%.2f  Hit/Miss",                 (double)tex_mgr.getHitCount  () / 
                                                           (double)tex_mgr.getMissCount ()          ); ImGui::NextColumn ();
        ImGui::Text    ( "Driver: %#7zu MiB  ",                    tex_mgr.getByteSaved () >> 20ULL );

      ImGui::PopStyleColor
                       (   );
      ImGui::Columns   ( 1 );
      ImGui::SliderInt ("Cache Size (in MiB)###D3D9_TexCache_Size_MiB", &config.textures.cache.max_size, 256, 4096);
    //ImGui::PopFont   (   );
      ImGui::EndChild  (   );

#if 0
      if (ImGui::CollapsingHeader ("Thread Stats"))
      {
        std::vector <SK::D3D9::TexThreadStats> stats =
          SK::D3D9::tex_mgr.getThreadStats ();

        int thread_id = 0;

        for (auto&& it : stats)
        {
          ImGui::Text ("Thread #%lu  -  %6lu jobs retired, %5lu MiB loaded  -  %.6f User / %.6f Kernel / %3.1f Idle",
                       thread_id++,
                       it.jobs_retired, it.bytes_loaded >> 20UL,
                       (double)ULARGE_INTEGER
          {
            it.runtime.user.dwLowDateTime, it.runtime.user.dwHighDateTime
          }.QuadPart / 10000000.0,
                       (double)ULARGE_INTEGER
          {
            it.runtime.kernel.dwLowDateTime, it.runtime.kernel.dwHighDateTime
          }.QuadPart / 10000000.0,
              (double)ULARGE_INTEGER
            {
              it.runtime.idle.dwLowDateTime, it.runtime.idle.dwHighDateTime
            }.QuadPart / 10000000.0);
        }
      }
#endif

      ImGui::TreePop     ();
      ImGui::PopStyleVar ();
    }
    ImGui::TreePop ();
    }

    ImGui::TreePop ();

    return true;
  }

  return false;
}


void
SK_ImGui_SummarizeD3D9Swapchain (IDirect3DSwapChain9 *pSwap9)
{
  if (pSwap9 != nullptr)
  {
    D3DPRESENT_PARAMETERS pparams = { };

    if (SUCCEEDED (pSwap9->GetPresentParameters (&pparams)))
    {
      ImGui::BeginTooltip    ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImColor (0.95f, 0.95f, 0.45f));
      ImGui::TextUnformatted ("Framebuffer and Presentation Setup");
      ImGui::PopStyleColor   ();
      ImGui::Separator       ();

      ImGui::BeginGroup      ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImColor (0.785f, 0.785f, 0.785f));
      ImGui::TextUnformatted ("Color:");
      ImGui::TextUnformatted ("Depth/Stencil:");
      ImGui::TextUnformatted ("Resolution:");
      ImGui::TextUnformatted ("Back Buffers:");
      if (! pparams.Windowed)
      ImGui::TextUnformatted ("Refresh Rate:");
      ImGui::TextUnformatted ("Swap Interval:");
      ImGui::TextUnformatted ("Swap Effect:");
      ImGui::TextUnformatted ("MSAA Samples:");
      if (pparams.Flags != 0)
      ImGui::TextUnformatted ("Flags:");
      ImGui::PopStyleColor   ();
      ImGui::EndGroup        ();

      ImGui::SameLine        ();

      ImGui::BeginGroup      ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImColor (1.0f, 1.0f, 1.0f));
      ImGui::Text            ("%ws",                SK_D3D9_FormatToStr (pparams.BackBufferFormat).c_str       ());
      ImGui::Text            ("%ws",                SK_D3D9_FormatToStr (pparams.AutoDepthStencilFormat).c_str ());
      ImGui::Text            ("%ux%u",                                   pparams.BackBufferWidth, pparams.BackBufferHeight);
      ImGui::Text            ("%u",                                      pparams.BackBufferCount);
      if (! pparams.Windowed)
      ImGui::Text            ("%u Hz",                                   pparams.FullScreen_RefreshRateInHz);
      if (pparams.PresentationInterval == 0)
        ImGui::Text          ("%u: VSYNC OFF",                           pparams.PresentationInterval);
      else if (pparams.PresentationInterval == 1)
        ImGui::Text          ("%u: Normal V-SYNC",                       pparams.PresentationInterval);
      else if (pparams.PresentationInterval == 2)
        ImGui::Text          ("%u: 1/2 Refresh V-SYNC",                  pparams.PresentationInterval);
      else if (pparams.PresentationInterval == 3)
        ImGui::Text          ("%u: 1/3 Refresh V-SYNC",                  pparams.PresentationInterval);
      else if (pparams.PresentationInterval == 4)
        ImGui::Text          ("%u: 1/4 Refresh V-SYNC",                  pparams.PresentationInterval);
      else
        ImGui::Text          ("%u: UNKNOWN or Invalid",                  pparams.PresentationInterval);
      ImGui::Text            ("%ws",            SK_D3D9_SwapEffectToStr (pparams.SwapEffect).c_str ());
      ImGui::Text            ("%u",                                      pparams.MultiSampleType);
      if (pparams.Flags != 0)
      ImGui::Text            ("%ws", SK_D3D9_PresentParameterFlagsToStr (pparams.Flags).c_str ()) ;
      ImGui::PopStyleColor   ();
      ImGui::EndGroup        ();
      ImGui::EndTooltip      ();
    }
  }
}