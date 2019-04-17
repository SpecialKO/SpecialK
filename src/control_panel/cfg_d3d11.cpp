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

#include <SpecialK/config.h>
#include <imgui/imgui.h>

#include <SpecialK/control_panel.h>
#include <SpecialK/control_panel/d3d11.h>
#include <SpecialK/control_panel/osd.h>

#include <SpecialK/nvapi.h>

const wchar_t*
DXGIColorSpaceToStr (DXGI_COLOR_SPACE_TYPE space);


extern bool SK_D3D11_EnableTracking;
extern bool SK_D3D11_EnableMMIOTracking;
extern volatile
       LONG SK_D3D11_DrawTrackingReqs;
extern volatile
       LONG SK_D3D11_CBufferTrackingReqs;


using namespace SK::ControlPanel;

bool SK::ControlPanel::D3D11::show_shader_mod_dlg = false;

void
SK_ImGui_NV_DepthBoundsD3D11 (void)
{
  static bool  enable = false;
  static float fMin   = 0.0f;
  static float fMax   = 1.0f;

  bool changed = false;

  changed |= ImGui::Checkbox ("Enable Depth Bounds Test", &enable);

  if (enable)
  {
    changed |= ImGui::SliderFloat ("fMinDepth", &fMin, 0.0f, fMax);
    changed |= ImGui::SliderFloat ("fMaxDepth", &fMax, fMin, 1.0f);
  }

  if (changed)
  {
    NvAPI_D3D11_SetDepthBoundsTest ( SK_GetCurrentRenderBackend ().device,
                                       enable ? 0x1 : 0x0,
                                         fMin, fMax );
  }
}

extern bool
WINAPI
SK_DXGI_IsTrackingBudget (void);

SK_LazyGlobal <SK_D3D11_TexCacheResidency_s> SK_D3D11_TexCacheResidency;

void
SK_ImGui_DrawTexCache_Chart (void)
{
  if (config.textures.d3d11.cache)
  {
    ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.961f,0.961f,0.961f,1.f));
    ImGui::Separator (   );
    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::Columns   ( 3 );
      ImGui::Text    ( "          Size" );                                                                 ImGui::NextColumn ();
      ImGui::Text    ( "      Activity" );                                                                 ImGui::NextColumn ();
      ImGui::Text    ( "       Savings" );
    ImGui::Columns   ( 1 );
    ImGui::PopStyleColor
                     (   );

    ImGui::PushStyleColor
                     ( ImGuiCol_Text, ImVec4 (0.75f, 0.75f, 0.75f, 1.0f) );

    ImGui::Separator      ( );
    ImGui::PopStyleColor  (1);

    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 0.666f));
    ImGui::Columns   ( 3 );
      ImGui::Text    ( "%#7zu      MiB",
                                                     SK_D3D11_Textures->AggregateSize_2D >> 20ui64 );    ImGui::NextColumn ();
       ImGui::TextColored
                     (ImVec4 (0.3f, 1.0f, 0.3f, 1.0f),
                       "%#5lu      Hits",            SK_D3D11_Textures->RedundantLoads_2D.load () );     ImGui::NextColumn ();
       if (SK_D3D11_Textures->Budget != 0)
         ImGui::Text ( "Budget:  %#7zu MiB  ",       SK_D3D11_Textures->Budget / 1048576ui64 );
    ImGui::Columns   ( 1 );

    ImGui::Separator (   );

    ImGui::Columns   ( 3 );
      ImGui::Text    ( "%#7zu Textures",             SK_D3D11_Textures->Entries_2D.load () );            ImGui::NextColumn ();
      ImGui::TextColored
                     ( ImVec4 (1.0f, 0.3f, 0.3f, 1.60f),
                       "%#5lu   Misses",           SK_D3D11_Textures->CacheMisses_2D.load () );          ImGui::NextColumn ();
     ImGui::Text   ( "Time:        %#7.01lf ms  ", SK_D3D11_Textures->RedundantTime_2D       );
    ImGui::Columns   ( 1 );

    ImGui::Separator (   );

    ImGui::Columns   ( 3 );
      ImGui::Text    ( "%#6lu   Evictions",            SK_D3D11_Textures->Evicted_2D.load ()   );        ImGui::NextColumn ();
      ImGui::TextColored (ImColor::HSV (std::min ( 0.4f * (float)SK_D3D11_Textures->RedundantLoads_2D /
                                                          (float)SK_D3D11_Textures->CacheMisses_2D, 0.4f ), 0.95f, 0.8f),
                       " %.2f  Hit/Miss",                (double)SK_D3D11_Textures->RedundantLoads_2D /
                                                         (double)SK_D3D11_Textures->CacheMisses_2D  );   ImGui::NextColumn ();
      ImGui::Text    ( "Driver I/O: %#7llu MiB  ",     SK_D3D11_Textures->RedundantData_2D >> 20ui64 );

    ImGui::Columns   ( 1 );

    ImGui::Separator (   );

    ImGui::PopStyleColor ( );

    float size =
      gsl::narrow_cast <float> (config.textures.cache.max_size);

    ImGui::TreePush  ( "" );

    if (ImGui::SliderFloat ( "Maximum Cache Size", &size, 256.f, 8192.f, "%.0f MiB"))
    {
      config.textures.cache.max_size =
        gsl::narrow_cast <int> (size);

      SK_GetCommandProcessor ()->ProcessCommandFormatted ("TexCache.MaxSize %d ", config.textures.cache.max_size);
    }

    ImGui::TreePop       ();
    ImGui::PopStyleColor ();

  //if (SK_DXGI_IsTrackingBudget ())
    {
      ImGui::Separator ();

      ImGui::Checkbox ("Measure residency", &config.textures.cache.residency_managemnt);

      ImGui::SameLine ();
      ImGui::Checkbox ("Vibrate on cache miss", &config.textures.cache.vibrate_on_miss);

      if (config.textures.cache.residency_managemnt)
      {
        const int fully_resident = ReadAcquire (&SK_D3D11_TexCacheResidency->count.InVRAM);
        const int shared_memory  = ReadAcquire (&SK_D3D11_TexCacheResidency->count.Shared);
        const int on_disk        = ReadAcquire (&SK_D3D11_TexCacheResidency->count.PagedOut);

        const LONG64 size_vram   = ReadAcquire64 (&SK_D3D11_TexCacheResidency->size.InVRAM);
        const LONG64 size_shared = ReadAcquire64 (&SK_D3D11_TexCacheResidency->size.Shared);
        const LONG64 size_disk   = ReadAcquire64 (&SK_D3D11_TexCacheResidency->size.PagedOut);

        ImGui::BeginGroup ();
        if (fully_resident != 0)
          ImGui::TextColored (ImColor (0.3f, 0.78f, 0.3f),   "%lu Textures in VRAM\t",          fully_resident);

        if (shared_memory != 0)
          ImGui::TextColored (ImColor (0.78f, 0.78f, 0.55f), "%lu Textures in Shared Memory\t", shared_memory);

        if (on_disk != 0)
          ImGui::TextColored (ImColor (0.78f, 0.3f, 0.3f),   "%lu Textures Paged to Disk\t",    on_disk);
        ImGui::EndGroup ();

        ImGui::SameLine ();

        ImGui::BeginGroup ();
        if (fully_resident != 0)
          ImGui::TextColored (ImColor (0.1f, 0.98f, 0.1f),   "\t\t%ws", SK_File_SizeToStringF (size_vram,   2, 3).c_str ());

        if (shared_memory != 0)
          ImGui::TextColored (ImColor (0.98f, 0.98f, 0.25f), "\t\t%ws", SK_File_SizeToStringF (size_shared, 2, 3).c_str ());

        if (on_disk != 0)
          ImGui::TextColored (ImColor (0.98f, 0.1f, 0.1f),   "\t\t%ws", SK_File_SizeToStringF (size_disk,   2, 3).c_str ());
        ImGui::EndGroup ();
      }
    }
  }
}

#include <SpecialK/tls.h>
#include <SpecialK/render/d3d11/d3d11_core.h>

extern bool SK_D3D11_ShaderModDlg (SK_TLS *pTLS = SK_TLS_Bottom ());

using SK_ReShade_OnDrawD3D11_pfn =
void (__stdcall *)(void*, ID3D11DeviceContext*, unsigned int);

extern SK_RESHADE_CALLBACK_DRAW SK_ReShade_DrawCallback;

bool
SK::ControlPanel::D3D11::Draw (void)
{
  if (show_shader_mod_dlg)
    show_shader_mod_dlg = SK_D3D11_ShaderModDlg ();


  if ( static_cast <int> (render_api) & static_cast <int> (SK_RenderAPI::D3D11) &&
       ImGui::CollapsingHeader ("Direct3D 11 Settings", ImGuiTreeNodeFlags_DefaultOpen) )
  {
    ImGui::SameLine ();
    ImGui::TextUnformatted ("     Tracking:  ");

    ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.173f, 0.428f, 0.96f));
    ImGui::SameLine ();

    if (SK_D3D11_EnableTracking)
      ImGui::TextUnformatted ("( ALL State/Ops --> [Mod Tools Window Active] )");

    else
    {
      char* szThreadLocalStr =
          SK_TLS_Bottom ()->scratch_memory.cmd.alloc (
                        256,   true                  );

      bool tracking = false;

      if ( ( ReadAcquire (&SK_D3D11_DrawTrackingReqs) > 0 ||
            SK_ReShade_DrawCallback.fn != nullptr  )        )
      {
        tracking = true;

        if (ReadAcquire (&SK_D3D11_DrawTrackingReqs) == 0)
          lstrcatA (szThreadLocalStr,   "  Draw Calls  ( ReShade Trigger ) ");

        else
        {
          if (SK_ReShade_DrawCallback.fn != nullptr)
            lstrcatA (szThreadLocalStr, "  Draw Calls  ( Generic & ReShade Trigger ) ");
          else
            lstrcatA (szThreadLocalStr, "  Draw Calls  ( Generic ) ");
        }
      }

      if (ReadAcquire (&SK_D3D11_CBufferTrackingReqs) > 0)
      {
        tracking = true;
        lstrcatA (szThreadLocalStr, "  Constant Buffers ");
      }

      if (SK_D3D11_EnableMMIOTracking)
      {
        tracking = true;
        lstrcatA (szThreadLocalStr, "  Memory-Mapped I/O ");
      }

      if (SK_ReShade_DrawCallback.fn != nullptr)
      {
      }

      ImGui::TextUnformatted (tracking ? szThreadLocalStr : " ");
    }
    ImGui::PopStyleColor ();

    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));
    ImGui::TreePush       ("");

    ////ImGui::Checkbox ("Overlay Compatibility Mode", &SK_DXGI_SlowStateCache);

    ////if (ImGui::IsItemHovered ())
      ////ImGui::SetTooltip ("Increased compatibility with video capture software")

    const bool swapchain =
      ImGui::CollapsingHeader ("SwapChain Management");

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip   ();
      ImGui::TextColored    ( ImColor (235, 235, 235),
                              "Highly Advanced Render Tweaks" );
      ImGui::Separator      ();
      ImGui::BulletText     ("Altering these settings may require manual INI edits to recover from");
      ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (1.0f, 0.85f, 0.1f, 0.9f));
      ImGui::BulletText     ("For best results, consult your nearest Kaldaien ;)");
      ImGui::PopStyleColor  ();
      ImGui::EndTooltip     ();
    }

    if (swapchain)
    {
      static bool flip         = config.render.framerate.flip_discard;
      static bool waitable     = config.render.framerate.swapchain_wait > 0;
      static int  buffer_count = config.render.framerate.buffer_count;

      ImGui::TreePush ("");

      ImGui::Checkbox ("Use Flip Model Presentation", &config.render.framerate.flip_discard);
      ImGui::InputInt ("Presentation Interval",       &config.render.framerate.present_interval);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();

        if (! config.render.framerate.flip_discard)
        {
          ImGui::Text       ("In Regular Presentation, this Controls V-Sync");
          ImGui::Separator  (                                               );
          ImGui::BulletText ("-1=Game Controlled,  0=Force Off,  1=Force On");
        }

        else
        {
          ImGui::Text       ("In Flip Model, this Controls Frame Queuing Rather than V-Sync)");
          ImGui::Separator  (                                                                );
          ImGui::BulletText ("Values > 1 will disable G-Sync but will produce the most "
                             "consistent frame rates possible."                              );
        }

        ImGui::EndTooltip ();
      }

      config.render.framerate.present_interval =
        std::max (-1, std::min (4, config.render.framerate.present_interval));

      if (ImGui::InputInt ("BackBuffer Count",       &config.render.framerate.buffer_count))
      {
        // Trigger a compliant game to invoke IDXGISwapChain::ResizeBuffers (...)
        PostMessage (SK_GetGameWindow (), WM_SIZE, SIZE_MAXIMIZED, MAKELPARAM ( (LONG)ImGui::GetIO ().DisplaySize.x,
                                                                                (LONG)ImGui::GetIO ().DisplaySize.y ) );
      }

      // Clamp to [-1,oo)
      if (config.render.framerate.buffer_count < -1)
        config.render.framerate.buffer_count = -1;

      if (ImGui::InputInt ("Maximum Device Latency", &config.render.framerate.pre_render_limit))
      {
        if (config.render.framerate.pre_render_limit < -1)
          config.render.framerate.pre_render_limit = -1;

        else if (config.render.framerate.pre_render_limit > config.render.framerate.buffer_count + 1)
          config.render.framerate.pre_render_limit = config.render.framerate.buffer_count + 1;

        CComQIPtr <IDXGISwapChain> pSwapChain (SK_GetCurrentRenderBackend ().swapchain);

        if (pSwapChain != nullptr)
        {
          void
          SK_DXGI_UpdateLatencies ( IDXGISwapChain *pSwapChain );
          SK_DXGI_UpdateLatencies (pSwapChain);
        }
      }

      if ((! config.render.framerate.flip_discard) || config.render.framerate.swapchain_wait == 0)
      {
        ImGui::Checkbox ("Wait for VBLANK", &config.render.framerate.wait_for_vblank);
      }

      if (config.render.framerate.flip_discard)
      {
        bool waitable_ = config.render.framerate.swapchain_wait > 0;

        if (ImGui::Checkbox ("Waitable SwapChain", &waitable_))
        {
          if (! waitable_) config.render.framerate.swapchain_wait = 0;
          else             config.render.framerate.swapchain_wait = 15;
        }

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Reduces input latency, BUT makes it impossible to change resolution.");

        if (waitable) {
          ImGui::SliderInt ("Maximum Wait Period", &config.render.framerate.swapchain_wait, 1, 500);
        }

        if (SK_DXGI_SupportsTearing ())
        {
          bool tearing_pref = config.render.dxgi.allow_tearing;
          if (ImGui::Checkbox ("Enable DWM Tearing", &tearing_pref))
          {
            config.render.dxgi.allow_tearing = tearing_pref;
          }

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
            ImGui::Text         ("Enables tearing in windowed mode (Windows 10+); not particularly useful.");
            ImGui::EndTooltip   ();
          }
        }
      }

      const bool changed =
        (flip         != config.render.framerate.flip_discard      ) ||
        (waitable     != config.render.framerate.swapchain_wait > 0) ||
        (buffer_count != config.render.framerate.buffer_count      );

      if (changed)
      {
        ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (.3f, .8f, .9f));
        ImGui::BulletText     ("Game Restart Required");
        ImGui::PopStyleColor  ();
      }

      ImGui::TreePop  (  );
    }

    if (ImGui::CollapsingHeader ("Texture Management"))
    {
      ImGui::TreePush ("");
      ImGui::Checkbox ("Enable Texture Caching", &config.textures.d3d11.cache);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip    ();
        ImGui::TextUnformatted ("Reduces Driver Memory Management Overhead in Games that Stream Textures");

        static bool orig_cache = config.textures.d3d11.cache;

        if (orig_cache)
        {
#if 0
          LONG contains_ = 0L;
          LONG erase_    = 0L;
          LONG index_    = 0L;

          std::pair <int, std::pair <LONG, std::pair <LONG, char*>>> busiest = { 0, { 0, { 0, "Invalid" } } };

          int idx = 0;

          for (auto it : SK_D3D11_Textures.HashMap_2D)
          {
            LONG i = ReadAcquire (&it.contention_score.index);
            LONG c = ReadAcquire (&it.contention_score.contains);
            LONG a = 0L;
            LONG e = ReadAcquire (&it.contention_score.erase);

            a = ( i + c + a + e );

            if ( idx > 0 && busiest.second.first < a )
            {
              busiest.first        = idx;
              busiest.second.first = a;

              LONG max_val =
                std::max (i, std::max (c, e));

              if (max_val == i)
                busiest.second.second.second = "operator []()";
              else if (max_val == c)
                busiest.second.second.second = "contains ()";
              else if (max_val == e)
                busiest.second.second.second = "erase ()";
              else
                busiest.second.second.second = "unknown";

              busiest.second.second.first    = max_val;
            }

            ++idx;

            contains_ += c; erase_ += e; index_ += i;
          }

          if (busiest.first != 0)
          {
            ImGui::Separator  (                                 );
            ImGui::BeginGroup (                                 );
            ImGui::BeginGroup (                                 );
            ImGui::BulletText ( "HashMap Contains: "            );
            ImGui::BulletText ( "HashMap Erase:    "            );
            ImGui::BulletText ( "HashMap Index:    "            );
            ImGui::Text       ( ""                              );
            ImGui::BulletText ( "Most Contended:   "            );
            ImGui::EndGroup   (                                 );
            ImGui::SameLine   (                                 );
            ImGui::BeginGroup (                                 );
            ImGui::Text       ( "%li Ops", contains_            );
            ImGui::Text       ( "%li Ops", erase_               );
            ImGui::Text       ( "%li Ops", index_               );
            ImGui::Text       ( ""                              );
            ImGui::Text       ( R"(Mipmap LOD%02li (%li :: <"%s">))",
                                 busiest.first - 1,
                                   busiest.second.second.first,
                                   busiest.second.second.second );
            ImGui::EndGroup   (                                 );
            ImGui::EndGroup   (                                 );
            ImGui::SameLine   (                                 );
            ImGui::BeginGroup (                                 );
            int lod = 0;
            for ( auto& it : SK_D3D11_Textures.HashMap_2D )
            {
              ImGui::BulletText ("LOD %02lu Load Factor: ", lod++);
            }
            ImGui::EndGroup   (                                 );
            ImGui::SameLine   (                                 );
            ImGui::BeginGroup (                                 );
            for ( auto& it : SK_D3D11_Textures.HashMap_2D )
            {
              ImGui::Text     (" %.2f", it.entries.load_factor());
            }
            ImGui::EndGroup   (                                 );
          }
#endif
        }
        else
        {
          ImGui::Separator  (                                   );
          ImGui::BulletText ( "Requires Application Restart"    );
        }
        ImGui::EndTooltip   (                                   );
      }

      //ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (1.0f, 0.85f, 0.1f, 0.9f));
      //ImGui::SameLine (); ImGui::BulletText ("Requires restart");
      //ImGui::PopStyleColor  ();

      if (config.textures.d3d11.cache)
      {
        ImGui::SameLine ();
        ImGui::Spacing  ();
        ImGui::SameLine ();

        ImGui::Checkbox ("Ignore Textures Without Mipmaps", &config.textures.cache.ignore_nonmipped);

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Important Compatibility Setting for Some Games (e.g. The Witcher 3)");

        ImGui::SameLine ();

        ImGui::Checkbox ("Cache Staged Texture Uploads", &config.textures.cache.allow_staging);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         ("Enable Texture Dumping and Injection in Unity-based Games");
          ImGui::Separator    ();
          ImGui::BulletText   ("May cause degraded performance.");
          ImGui::BulletText   ("Try to leave this off unless textures are missing from the mod tools.");
          ImGui::EndTooltip   ();
        }
      }

      ImGui::TreePop  ();

      SK_ImGui_DrawTexCache_Chart ();
    }

    static bool enable_resolution_limits = ! ( config.render.dxgi.res.min.isZero () &&
                                               config.render.dxgi.res.max.isZero () );

    const bool res_limits =
      ImGui::CollapsingHeader ("Resolution Limiting", enable_resolution_limits ? ImGuiTreeNodeFlags_DefaultOpen : 0x00);

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip ();
      ImGui::Text         ("Restrict the lowest/highest resolutions reported to a game");
      ImGui::Separator    ();
      ImGui::BulletText   ("Useful for games that compute aspect ratio based on the highest reported resolution.");
      ImGui::EndTooltip   ();
    }

    if (res_limits)
    {
      ImGui::TreePush  ("");
      ImGui::InputInt2 ("Minimum Resolution", reinterpret_cast <int *> (&config.render.dxgi.res.min.x));
      ImGui::InputInt2 ("Maximum Resolution", reinterpret_cast <int *> (&config.render.dxgi.res.max.x));

      // Fix for stupid users ... and stupid programmers who don't range validate
      //
      if (config.render.dxgi.res.max.x < config.render.dxgi.res.min.x && config.render.dxgi.res.min.x < 8192) config.render.dxgi.res.max.x = config.render.dxgi.res.min.x;
      if (config.render.dxgi.res.max.y < config.render.dxgi.res.min.y && config.render.dxgi.res.min.y < 8192) config.render.dxgi.res.max.y = config.render.dxgi.res.min.y;

      if (config.render.dxgi.res.min.x > config.render.dxgi.res.max.x && config.render.dxgi.res.max.x > 0)    config.render.dxgi.res.min.x = config.render.dxgi.res.max.x;
      if (config.render.dxgi.res.min.y > config.render.dxgi.res.max.y && config.render.dxgi.res.max.y > 0)    config.render.dxgi.res.min.y = config.render.dxgi.res.max.y;

      ImGui::TreePop   ();
     }

    if (ImGui::Button (" Render Mod Tools "))
    {
      show_shader_mod_dlg = ( ! show_shader_mod_dlg );
    }

    ImGui::SameLine ();

    ImGui::Checkbox ("D3D11 Deferred Mode", &config.render.dxgi.deferred_isolation);

    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("Try changing this option if textures / shaders are missing from the mod tools.");

    ImGui::SameLine ();

    if (ImGui::Checkbox ("Enable CEGUI", &config.cegui.enable))
    {
      SK_CEGUI_QueueResetD3D11 ();
    }

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip    ();
      ImGui::TextUnformatted ("Disabling may resolve graphics issues, but will disable achievement pop-ups and OSD text.");
      ImGui::EndTooltip      ();
    }

    // This only works when we have wrapped SwapChains
    if ( ReadAcquire (&SK_DXGI_LiveWrappedSwapChains) ||
         ReadAcquire (&SK_DXGI_LiveWrappedSwapChain1s) )
    {
      ImGui::SameLine              ();
      OSD::DrawVideoCaptureOptions ();
    }

    ImGui::SameLine ();

    const bool advanced =
      ImGui::TreeNode ("Advanced (Debug)###Advanced_NVD3D11");

    if (advanced)
    {
      ImGui::TreePop               ();
      ImGui::Separator             ();

      ImGui::Checkbox ("Enhanced (64-bit) Depth+Stencil Buffer", &config.render.dxgi.enhanced_depth);

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Requires application restart");

      if (sk::NVAPI::nv_hardware)
      {
        ImGui::SameLine              ();
        SK_ImGui_NV_DepthBoundsD3D11 ();
      }
    }

    ImGui::TreePop       ( );
    ImGui::PopStyleColor (3);

    return true;
  }

  return false;
}


bool
SK_D3D11_ShowShaderModDlg (void)
{
  return
    SK::ControlPanel::D3D11::show_shader_mod_dlg;
}


extern std::wstring
SK_DXGI_DescribeSwapChainFlags (DXGI_SWAP_CHAIN_FLAG swap_flags, INT* pLines = nullptr);


void
SK_ImGui_SummarizeDXGISwapchain (IDXGISwapChain* pSwapDXGI)
{
  if (pSwapDXGI != nullptr)
  {
    DXGI_SWAP_CHAIN_DESC swap_desc = { };

    if (SUCCEEDED (pSwapDXGI->GetDesc (&swap_desc)))
    {
      static SK_RenderBackend& rb =
        SK_GetCurrentRenderBackend ();

      ID3D11DeviceContext *pDevCtx = rb.d3d11.immediate_ctx;
      //CComPtr <ID3D11DeviceContext>   pDevCtx;
      //rb.d3d11.immediate_ctx.CopyTo (&pDevCtx.p);

      // This limits us to D3D11 for now, but who cares -- D3D10 sucks and D3D12 can't be drawn to yet :)
      if (! pDevCtx)
        return;

      INT          swap_flag_count = 0;
      std::wstring swap_flags      =
        SK_DXGI_DescribeSwapChainFlags (
          static_cast <DXGI_SWAP_CHAIN_FLAG> (swap_desc.Flags),
                                             &swap_flag_count);

      ImGui::BeginTooltip    ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (0.95f, 0.95f, 0.45f, 1.0f));
      ImGui::TextUnformatted ("Framebuffer and Presentation Setup");
      ImGui::PopStyleColor   ();
      ImGui::Separator       ();

      ImGui::BeginGroup      ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (0.685f, 0.685f, 0.685f, 1.0f));
      ImGui::TextUnformatted ("Color:");
    //ImGui::TextUnformatted ("Depth/Stencil:");
      ImGui::TextUnformatted ("Resolution:");
      ImGui::TextUnformatted ("Back Buffers:");
      if ((! swap_desc.Windowed) && swap_desc.BufferDesc.Scaling          != DXGI_MODE_SCALING_UNSPECIFIED)
      ImGui::TextUnformatted ("Scaling Mode:");
      if ((! swap_desc.Windowed) && swap_desc.BufferDesc.ScanlineOrdering != DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED)
      ImGui::TextUnformatted ("Scanlines:");
      if (! swap_desc.Windowed && swap_desc.BufferDesc.RefreshRate.Denominator != 0)
      ImGui::TextUnformatted ("Refresh Rate:");
      ImGui::TextUnformatted ("Swap Interval:");
      ImGui::TextUnformatted ("Swap Effect:");
      if  (swap_desc.SampleDesc.Count > 1)
      ImGui::TextUnformatted ("MSAA Samples:");
      if (swap_desc.Flags != 0)
      ImGui::TextUnformatted ("Flags:");
      if (swap_flag_count > 1) { for ( int i = 1; i < swap_flag_count; i++ ) ImGui::TextUnformatted ("\n"); }
      if (rb.isHDRCapable ())
      {
        ImGui::Separator  ();
        ImGui::Text ("Display Device:   ");
        ImGui::Text ("HDR Color Space:  ");
        ImGui::Text ("Output Bit Depth: ");
      }
      ImGui::PopStyleColor   ();
      ImGui::EndGroup        ();

      ImGui::SameLine        ();

      ImGui::BeginGroup      ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.0f, 1.0f, 1.0f, 1.0f));
      ImGui::Text            ("%ws",                SK_DXGI_FormatToStr (swap_desc.BufferDesc.Format).c_str ());
    //ImGui::Text            ("%ws",                SK_DXGI_FormatToStr (dsv_desc.Format).c_str             ());
      ImGui::Text            ("%ux%u",                                   swap_desc.BufferDesc.Width, swap_desc.BufferDesc.Height);
      ImGui::Text            ("%u",                                      std::max (1UL, swap_desc.Windowed ? swap_desc.BufferCount : swap_desc.BufferCount - 1UL));
      if ((! swap_desc.Windowed) && swap_desc.BufferDesc.Scaling          != DXGI_MODE_SCALING_UNSPECIFIED)
      ImGui::Text            ("%ws",        SK_DXGI_DescribeScalingMode (swap_desc.BufferDesc.Scaling));
      if ((! swap_desc.Windowed) && swap_desc.BufferDesc.ScanlineOrdering != DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED)
      ImGui::Text            ("%ws",      SK_DXGI_DescribeScanlineOrder (swap_desc.BufferDesc.ScanlineOrdering));
      if (! swap_desc.Windowed && swap_desc.BufferDesc.RefreshRate.Denominator != 0)
      ImGui::Text            ("%.2f Hz",                                 static_cast <float> (swap_desc.BufferDesc.RefreshRate.Numerator) /
                                                                         static_cast <float> (swap_desc.BufferDesc.RefreshRate.Denominator));
      if (rb.present_interval == 0)
        ImGui::Text          ("%u: VSYNC OFF",                           rb.present_interval);
      else if (rb.present_interval == 1)
        ImGui::Text ("%u: Normal V-SYNC", rb.present_interval);
      else if (rb.present_interval == 2)
        ImGui::Text          ("%u: 1/2 Refresh V-SYNC",                  rb.present_interval);
      else if (rb.present_interval == 3)
        ImGui::Text          ("%u: 1/3 Refresh V-SYNC",                  rb.present_interval);
      else if (rb.present_interval == 4)
        ImGui::Text          ("%u: 1/4 Refresh V-SYNC",                  rb.present_interval);
      else
        ImGui::Text          ("%u: UNKNOWN or Invalid",                  0);//pparams.PresentationInterval);
      ImGui::Text            ("%ws",            SK_DXGI_DescribeSwapEffect (swap_desc.SwapEffect));
      if  (swap_desc.SampleDesc.Count > 1)
      ImGui::Text            ("%u",                                         swap_desc.SampleDesc.Count);
      if (swap_desc.Flags != 0)
        ImGui::Text          ("%ws",                                        swap_flags.c_str ());
      if (rb.isHDRCapable ())
      {
        bool _fullscreen = true;

        CComQIPtr <IDXGISwapChain4> pSwap3 (pSwapDXGI);
        if (pSwap3 != nullptr)
        {
          DXGI_SWAP_CHAIN_FULLSCREEN_DESC full_desc = { };
              pSwap3->GetFullscreenDesc (&full_desc);

          _fullscreen =
            (! full_desc.Windowed);
        }

        ImGui::Separator  ();
        ImGui::Text ("%ws", rb.display_name);

        if (! rb.scanout.nvapi_hdr.active)
        {
          if (_fullscreen)
            ImGui::Text ("%ws",                DXGIColorSpaceToStr ((DXGI_COLOR_SPACE_TYPE)rb.scanout.dxgi_colorspace));
          else
            ImGui::Text ("%ws (DWM Assigned)", DXGIColorSpaceToStr ((DXGI_COLOR_SPACE_TYPE)rb.scanout.dwm_colorspace));
          ImGui::Text   ("%lu", rb.scanout.bpc);
        }

        else
        {
          ImGui::Text ("%ws (%s)", HDRModeToStr (rb.scanout.nvapi_hdr.mode),
                                                 rb.scanout.nvapi_hdr.getFormatStr ());
          ImGui::Text ("%s",                     rb.scanout.nvapi_hdr.getBpcStr    ());
        }
      }
      ImGui::PopStyleColor   ();
      ImGui::EndGroup        ();
      ImGui::EndTooltip      ();
    }
  }
}

extern std::wstring SK_D3D11_res_root;

void
SK::ControlPanel::D3D11::TextureMenu (void)
{
  if (ImGui::BeginMenu ("Browse Texture Assets"))
  {
    wchar_t wszPath [MAX_PATH * 2] = { };

    if (ImGui::MenuItem ("Injectable Textures", SK_FormatString ("%ws", SK_File_SizeToString (SK_D3D11_Textures->injectable_texture_bytes).c_str ()).c_str (), nullptr))
    {
      wcscpy      (wszPath, SK_D3D11_res_root.c_str ());
      PathAppendW (wszPath, LR"(inject\textures)");

      ShellExecuteW (GetActiveWindow (), L"explore", wszPath, nullptr, nullptr, SW_NORMAL);
    }

    if ((! SK_D3D11_Textures->dumped_textures.empty ()) &&
          ImGui::MenuItem ("Dumped Textures", SK_FormatString ("%ws", SK_File_SizeToString (SK_D3D11_Textures->dumped_texture_bytes).c_str ()).c_str (), nullptr))
    {
      wcscpy      (wszPath, SK_D3D11_res_root.c_str ());
      PathAppendW (wszPath, LR"(dump\textures)");
      PathAppendW (wszPath, SK_GetHostApp ());

      ShellExecuteW (GetActiveWindow (), L"explore", wszPath, nullptr, nullptr, SW_NORMAL);
    }

    ImGui::EndMenu ();
  }
}