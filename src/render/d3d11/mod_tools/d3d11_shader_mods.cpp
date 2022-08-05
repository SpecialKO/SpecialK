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
#include <cstdio>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"D3D11Shade"

#include <SpecialK/render/d3d11/d3d11_core.h>
#include <SpecialK/render/d3d11/d3d11_tex_mgr.h>
#include <SpecialK/render/d3d11/d3d11_state_tracker.h>
#include <SpecialK/render/d3d11/utility/d3d11_texture.h>

#include <algorithm>
#include <execution>


// Indicates whether the shader mod window is tracking render target refs
bool live_rt_view = true;

class SK_D3D11_AbstractBlacklist
{
public:
  explicit operator uint32_t (void) const {
    return for_shader.crc32c;
  };

  struct {
    uint32_t crc32c;
  } for_shader;// [6];

  struct     {
    struct   {
      struct vtx_count_cond_s {
        std::pair <bool,int> vertices;
                              }
        more_than,
        less_than;
             } have;
             } if_meshes;
};

SK_LazyGlobal <Concurrency::concurrent_unordered_multimap <uint32_t, SK_D3D11_AbstractBlacklist>>
SK_D3D11_BlacklistDrawcalls;

void
_make_blacklist_draw_min_verts ( uint32_t crc32c,
                                int      value  )
{
  SK_D3D11_AbstractBlacklist min_verts = { };

  min_verts.for_shader.crc32c = crc32c;
  min_verts.if_meshes.have.less_than.vertices =
    std::make_pair ( true, value );
  min_verts.if_meshes.have.more_than.vertices =
    std::make_pair ( false, 0    );

  SK_D3D11_BlacklistDrawcalls->insert (std::make_pair (crc32c, min_verts));
}

void
_make_blacklist_draw_max_verts ( uint32_t crc32c,
                                 int      value  )
{
  SK_D3D11_AbstractBlacklist max_verts = { };

  max_verts.for_shader.crc32c = crc32c;
  max_verts.if_meshes.have.more_than.vertices =
    std::make_pair ( true, value );
  max_verts.if_meshes.have.less_than.vertices =
    std::make_pair ( false, 0    );

  SK_D3D11_BlacklistDrawcalls->insert (std::make_pair (crc32c, max_verts));
}

bool SK_D3D11_KnownTargets::_mod_tool_wants = false;

bool
SK_D3D11_IsValidRTV (ID3D11RenderTargetView* pRTV)
{
  if (pRTV == nullptr)
    return false;

  ID3D11Resource* pRes = nullptr;

  __try {
    ID3D11RenderTargetView*                                     pUnkle = nullptr;
    if (FAILED (pRTV->QueryInterface <ID3D11RenderTargetView> (&pUnkle)) ||
                                                     nullptr == pUnkle)
      return false;

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = { };
    pUnkle->GetDesc             (&rtvDesc);

    if (rtvDesc.ViewDimension == D3D11_RTV_DIMENSION_UNKNOWN)
    {
      pUnkle->Release ();
      return false;
    }

    pUnkle->GetResource (&pRes);
    pUnkle->Release ();

    if (pRes != nullptr)
        pRes->Release ();
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    return false;
  }

  return true;
}

bool
SK_D3D11_ShaderModDlg (SK_TLS* pTLS = SK_TLS_Bottom ())
{
  if (pTLS == nullptr)
      pTLS  = SK_TLS_Bottom ();

  auto& io =
    ImGui::GetIO ();

  // Flag this thread so the IUnknown::AddRef (...) that comes as a result
  //   of GetResource (...) does not count as texture cache hits.
  SK_ScopedBool auto_draw (&pTLS->imgui->drawing);
                            pTLS->imgui->drawing = TRUE;

  SK_ScopedBool decl_tex_scope (
    SK_D3D11_DeclareTexInjectScope (pTLS)
  );

  static SK_RenderBackend_V2& rb =
    SK_GetCurrentRenderBackend ();

  std::scoped_lock < SK_Thread_HybridSpinlock, SK_Thread_HybridSpinlock,
                     SK_Thread_HybridSpinlock, SK_Thread_HybridSpinlock,
                     SK_Thread_HybridSpinlock, SK_Thread_HybridSpinlock,
                     SK_Thread_HybridSpinlock >
    fort_knox ( *cs_shader,    *cs_shader_vs, *cs_shader_ps,
                *cs_shader_gs, *cs_shader_hs, *cs_shader_ds,
                *cs_shader_cs );


  const float font_size           = (ImGui::GetFont ()->FontSize * io.FontGlobalScale);
  const float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y +
                                                ImGui::GetStyle ().ItemInnerSpacing.y;

  bool show_dlg = true;

  ImGui::SetNextWindowSize ( ImVec2 ( io.DisplaySize.x * 0.66f,
                                      io.DisplaySize.y * 0.42f ), ImGuiCond_Appearing);
  ImGui::SetNextWindowSizeConstraints ( /*ImVec2 (768.0f, 384.0f),*/
                                        ImVec2 ( io.DisplaySize.x * 0.16f, io.DisplaySize.y * 0.16f ),
                                        ImVec2 ( io.DisplaySize.x * 0.96f, io.DisplaySize.y * 0.96f ) );

  if ( ImGui::Begin ( "D3D11 Render Mod Toolkit###D3D11_RenderDebug",
  //SK_D3D11_MemoryThreads.count_active         (), SK_D3D11_MemoryThreads.count_all   (),
  //  SK_D3D11_ShaderThreads.count_active       (), SK_D3D11_ShaderThreads.count_all   (),
  //    SK_D3D11_DrawThreads.count_active       (), SK_D3D11_DrawThreads.count_all     (),
  //      SK_D3D11_DispatchThreads.count_active (), SK_D3D11_DispatchThreads.count_all () ).c_str (),
                        &show_dlg ) )
  {
    SK_D3D11_EnableTracking = true;

    bool can_scroll = (
      ImGui::IsWindowFocused     () &&
      ImGui::IsMouseHoveringRect ( ImVec2 (ImGui::GetWindowPos ().x,
                                           ImGui::GetWindowPos ().y),
                                   ImVec2 (ImGui::GetWindowPos ().x + ImGui::GetWindowSize ().x,
                                           ImGui::GetWindowPos ().y + ImGui::GetWindowSize ().y) )
    );

    ImGui::PushItemWidth (ImGui::GetWindowWidth () * 0.666f);

    ImGui::Columns (2);

    ImGui::BeginChild ( ImGui::GetID ("Render_Left_Side"), ImVec2 (0,0), false,
                          ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

    if (ImGui::CollapsingHeader ("Live Shader View", ImGuiTreeNodeFlags_DefaultOpen))
    {
      SK_D3D11_LiveShaderView (can_scroll);
    }

    auto FormatNumber = [](int num) ->
    const char*
    {
      static char szNumber       [16] = { };
      static char szPrettyNumber [32] = { };

      char dot   [2] = ".";
      char comma [2] = ",";

      const NUMBERFMTA fmt = { 0, 0, 3, dot, comma, 0 };

      snprintf (szNumber, 15, "%i", num), szNumber [15] = '\0';

      GetNumberFormatA ( MAKELCID (LOCALE_USER_DEFAULT, SORT_DEFAULT),
                           0x00,
                             szNumber, &fmt,
                               szPrettyNumber, 32 );

      return szPrettyNumber;
    };

    if (ImGui::CollapsingHeader ("Draw Call Filters", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush ("");

      static auto& shaders = SK_D3D11_Shaders;
      static auto& vertex  = shaders->vertex;

      auto const tracker =
        &vertex.tracked;

      static int min_verts_input,
                 max_verts_input;

      ImGui::Text   ("Vertex Shader: %x", tracker->crc32c.load ());

      bool add_min = ImGui::Button   ("Add Min Filter"); ImGui::SameLine ();
                     ImGui::InputInt ("Min Verts", &min_verts_input);
      bool add_max = ImGui::Button   ("Add Max Filter"); ImGui::SameLine ();
                     ImGui::InputInt ("Max Verts", &max_verts_input);

      ImGui::Separator ();

      if (add_min) _make_blacklist_draw_min_verts (tracker->crc32c, min_verts_input);
      if (add_max) _make_blacklist_draw_max_verts (tracker->crc32c, max_verts_input);

      int idx = 0;
      ImGui::BeginGroup ();
      for (auto& blacklist : *SK_D3D11_BlacklistDrawcalls)
      {
        if ( blacklist.second.if_meshes.have.less_than.vertices.first ||
             blacklist.second.if_meshes.have.more_than.vertices.first    )
        {
          ImGui::PushID (idx++);
          if (ImGui::Button ("Remove Filter"))
          {
            blacklist.second.if_meshes.have.less_than.vertices =
              std::make_pair ( false, 0 );
            blacklist.second.if_meshes.have.more_than.vertices =
              std::make_pair ( false, 0 );
          }
          ImGui::PopID  ();
        }
      }
      ImGui::EndGroup   ();
      ImGui::SameLine   ();

      int rule_idx = 0;

      ImGui::BeginGroup ();
      for (auto& blacklist : *SK_D3D11_BlacklistDrawcalls)
      {
        if ( blacklist.second.if_meshes.have.less_than.vertices.first ||
             blacklist.second.if_meshes.have.more_than.vertices.first    )
        {
          ImGui::Text ("Rule%lu  ", rule_idx++);
        }
      }
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      for (auto& blacklist : *SK_D3D11_BlacklistDrawcalls)
      {
        if ( blacklist.second.if_meshes.have.less_than.vertices.first ||
             blacklist.second.if_meshes.have.more_than.vertices.first    )
        {
          ImGui::Text ("Vtx Shader: %x  ", blacklist.first);
        }
      }
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      for (auto& blacklist : *SK_D3D11_BlacklistDrawcalls)
      {
        if ( blacklist.second.if_meshes.have.less_than.vertices.first ||
             blacklist.second.if_meshes.have.more_than.vertices.first    )
        {
          if (blacklist.second.if_meshes.have.less_than.vertices.first)
          {
            ImGui::Text ("Min. Verts = %lu", blacklist.second.if_meshes.have.less_than.vertices.second);
          }
        }
      }
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      for (auto& blacklist : *SK_D3D11_BlacklistDrawcalls)
      {
        if ( blacklist.second.if_meshes.have.less_than.vertices.first ||
             blacklist.second.if_meshes.have.more_than.vertices.first    )
        {
          if (blacklist.second.if_meshes.have.more_than.vertices.first)
          {
            ImGui::Text ("Max. Verts = %lu", blacklist.second.if_meshes.have.more_than.vertices.second);
          }
        }
      }
      ImGui::EndGroup  ();
      ImGui::TreePop   ();
    }

    if (ImGui::CollapsingHeader ("Live Memory View", ImGuiTreeNodeFlags_DefaultOpen))
    {
      SK_D3D11_EnableMMIOTracking = true;
      ////std::scoped_lock <SK_Thread_CriticalSection> auto_lock (cs_mmio);

      ImGui::BeginChild ( ImGui::GetID ("Render_MemStats_D3D11"), ImVec2 (0, 0), false,
                          ImGuiWindowFlags_NoNavInputs |
                          ImGuiWindowFlags_NoNavFocus  |
                          ImGuiWindowFlags_AlwaysAutoResize );

      auto& last_frame =
        mem_map_stats->last_frame;

      ImGui::TreePush   (""                      );
      ImGui::BeginGroup (                        );
      ImGui::BeginGroup (                        );
      ImGui::TextColored(ImColor (0.9f, 1.0f, 0.15f, 1.0f), "Mapped Memory"  );
      ImGui::TreePush   (""                      );
 ImGui::TextUnformatted ("Read-Only:            ");
 ImGui::TextUnformatted ("Write-Only:           ");
 ImGui::TextUnformatted ("Read-Write:           ");
 ImGui::TextUnformatted ("Write (Discard):      ");
 ImGui::TextUnformatted ("Write (No Overwrite): ");
 ImGui::TextUnformatted (""               );
      ImGui::TreePop    (                        );
      ImGui::TextColored(ImColor (0.9f, 1.0f, 0.15f, 1.0f), "Resource Types"  );
      ImGui::TreePush   (""               );
 ImGui::TextUnformatted ("Unknown:       ");
 ImGui::TextUnformatted ("Buffers:       ");
      ImGui::TreePush   (""               );
 ImGui::TextUnformatted ("Index:         ");
 ImGui::TextUnformatted ("Vertex:        ");
 ImGui::TextUnformatted ("Constant:      ");
      ImGui::TreePop    (                 );
 ImGui::TextUnformatted ("Textures:      ");
      ImGui::TreePush   (""               );
 ImGui::TextUnformatted ("Textures (1D): ");
 ImGui::TextUnformatted ("Textures (2D): ");
 ImGui::TextUnformatted ("Textures (3D): ");
      ImGui::TreePop    (                 );
 ImGui::TextUnformatted (""               );
      ImGui::TreePop    (                 );
      ImGui::TextColored(ImColor (0.9f, 1.0f, 0.15f, 1.0f), "Memory Totals"  );
      ImGui::TreePush   (""               );
 ImGui::TextUnformatted ("Bytes Read:    ");
 ImGui::TextUnformatted ("Bytes Written: ");
 ImGui::TextUnformatted ("Bytes Copied:  ");
      ImGui::TreePop    (                 );
      ImGui::EndGroup   (                 );

      ImGui::SameLine   (                        );

      ImGui::BeginGroup (                        );
 ImGui::TextUnformatted (""                      );
      ImGui::Text       ("( %s )", FormatNumber (last_frame.map_types [0]));
      ImGui::Text       ("( %s )", FormatNumber (last_frame.map_types [1]));
      ImGui::Text       ("( %s )", FormatNumber (last_frame.map_types [2]));
      ImGui::Text       ("( %s )", FormatNumber (last_frame.map_types [3]));
      ImGui::Text       ("( %s )", FormatNumber (last_frame.map_types [4]));
 ImGui::TextUnformatted (""                      );
 ImGui::TextUnformatted (""                      );
      ImGui::Text       ("( %s )", FormatNumber (last_frame.resource_types [0]));
      ImGui::Text       ("( %s )", FormatNumber (last_frame.resource_types [1]));
      ImGui::TreePush   (""                      );
      ImGui::Text       ("%s",     FormatNumber ((int)last_frame.buffer_types [0]));
      ImGui::Text       ("%s",     FormatNumber ((int)last_frame.buffer_types [1]));
      ImGui::Text       ("%s",     FormatNumber ((int)last_frame.buffer_types [2]));
      ImGui::TreePop    (                        );
      ImGui::Text       ("( %s )", FormatNumber (last_frame.resource_types [2] +
                                                 last_frame.resource_types [3] +
                                                 last_frame.resource_types [4]));
      ImGui::Text       ("( %s )", FormatNumber (last_frame.resource_types [2]));
      ImGui::Text       ("( %s )", FormatNumber (last_frame.resource_types [3]));
      ImGui::Text       ("( %s )", FormatNumber (last_frame.resource_types [4]));
 ImGui::TextUnformatted (""                      );
 ImGui::TextUnformatted (""                      );

      if ((double)last_frame.bytes_read < (0.75f * 1024.0 * 1024.0))
        ImGui::Text     ("( %06.2f KiB )", (double)last_frame.bytes_read    / (1024.0));
      else
        ImGui::Text     ("( %06.2f MiB )", (double)last_frame.bytes_read    / (1024.0 * 1024.0));

      if ((double)last_frame.bytes_written < (0.75f * 1024.0 * 1024.0))
        ImGui::Text     ("( %06.2f KiB )", (double)last_frame.bytes_written / (1024.0));
      else
        ImGui::Text     ("( %06.2f MiB )", (double)last_frame.bytes_written / (1024.0 * 1024.0));

      if ((double)last_frame.bytes_copied < (0.75f * 1024.0 * 1024.0))
        ImGui::Text     ("( %06.2f KiB )", (double)last_frame.bytes_copied / (1024.0));
      else
        ImGui::Text     ("( %06.2f MiB )", (double)last_frame.bytes_copied / (1024.0 * 1024.0));

      ImGui::EndGroup   (                        );

      ImGui::SameLine   (                        );

      auto& lifetime =
        mem_map_stats->lifetime;

      ImGui::BeginGroup (                        );
      ImGui::Text       (""                      );
      ImGui::Text       (" / %s", FormatNumber (lifetime.map_types [0]));
      ImGui::Text       (" / %s", FormatNumber (lifetime.map_types [1]));
      ImGui::Text       (" / %s", FormatNumber (lifetime.map_types [2]));
      ImGui::Text       (" / %s", FormatNumber (lifetime.map_types [3]));
      ImGui::Text       (" / %s", FormatNumber (lifetime.map_types [4]));
      ImGui::Text       (""                      );
      ImGui::Text       (""                      );
      ImGui::Text       (" / %s", FormatNumber (lifetime.resource_types [0]));
      ImGui::Text       (" / %s", FormatNumber (lifetime.resource_types [1]));
      ImGui::Text       ("");
      ImGui::Text       ("");
      ImGui::Text       ("");
      ImGui::Text       (" / %s", FormatNumber (lifetime.resource_types [2] +
                                                lifetime.resource_types [3] +
                                                lifetime.resource_types [4]));
      ImGui::Text       (" / %s", FormatNumber (lifetime.resource_types [2]));
      ImGui::Text       (" / %s", FormatNumber (lifetime.resource_types [3]));
      ImGui::Text       (" / %s", FormatNumber (lifetime.resource_types [4]));
      ImGui::Text       (""                      );
      ImGui::Text       (""                      );

      if ((double)lifetime.bytes_read < (0.75f * 1024.0 * 1024.0 * 1024.0))
        ImGui::Text     (" / %06.2f MiB", (double)lifetime.bytes_read    / (1024.0 * 1024.0));
      else
        ImGui::Text     (" / %06.2f GiB", (double)lifetime.bytes_read    / (1024.0 * 1024.0 * 1024.0));

      if ((double)lifetime.bytes_written < (0.75f * 1024.0 * 1024.0 * 1024.0))
        ImGui::Text     (" / %06.2f MiB", (double)lifetime.bytes_written / (1024.0 * 1024.0));
      else
        ImGui::Text     (" / %06.2f GiB", (double)lifetime.bytes_written / (1024.0 * 1024.0 * 1024.0));

      if ((double)lifetime.bytes_copied < (0.75f * 1024.0 * 1024.0 * 1024.0))
        ImGui::Text     (" / %06.2f MiB", (double)lifetime.bytes_copied / (1024.0 * 1024.0));
      else
        ImGui::Text     (" / %06.2f GiB", (double)lifetime.bytes_copied / (1024.0 * 1024.0 * 1024.0));

      ImGui::EndGroup   (                        );
      ImGui::EndGroup   (                        );
      ImGui::TreePop    (                        );
      ImGui::EndChild   ();
    }

    else
      SK_D3D11_EnableMMIOTracking = false;

    ImGui::EndChild   ();

    ImGui::NextColumn ();

    ImGui::BeginChild ( ImGui::GetID ("Render_Right_Side"), ImVec2 (0, 0), false,
                          ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened | ImGuiWindowFlags_NoScrollbar );

    static bool uncollapsed_tex = true;
    static bool uncollapsed_rtv = true;

    float scale = (uncollapsed_tex ? 1.0f * (uncollapsed_rtv ? 0.75f : 1.0f) : -1.0f);

    ImGui::BeginChild     ( ImGui::GetID ("Live_Texture_View_Panel"),
                            ImVec2 ( -1.0f, scale == -1.0f ? font_size_multiline * 1.666f :
                   ( ImGui::GetWindowContentRegionMax ().y/*- ImGui::GetWindowContentRegionMin ().y*/) *
                                   scale - (scale == 1.0f ? font_size_multiline * 1.666f : 0.0f) ),
                              true,
                                ImGuiWindowFlags_AlwaysAutoResize |
                                ImGuiWindowFlags_NavFlattened );

    uncollapsed_tex =
      ImGui::CollapsingHeader ( "Live Texture View",
                                /*ImGuiTreeNodeFlags_AllowItemOverlap |*/
                                        ( config.textures.d3d11.cache ?
                                       ImGuiTreeNodeFlags_DefaultOpen : 0x0 ) );

    if (! config.textures.d3d11.cache)
    {
      ImGui::SameLine    ( 2.5f);
      ImGui::TextColored ( ImColor::HSV (0.15f, 1.0f, 1.0f),
                            "\t(Unavailable because Texture Caching is not enabled!)" );
    }

    uncollapsed_tex =
      uncollapsed_tex && config.textures.d3d11.cache;

    if (uncollapsed_tex)
    {
      static bool warned_invalid_ref_count = false;

      if ((! warned_invalid_ref_count) && ReadAcquire (&SK_D3D11_TexRefCount_Failures) > 0)
      {
        SK_ImGui_Warning ( L"The game's graphics engine is not correctly tracking texture memory.\n\n"
                           L"\t\t\t\t>> Texture mod support has been partially disabled to prevent memory leaks.\n\n"
                           L"\t\tYou may force support for texture mods by setting AllowUnsafeRefCounting=true" );

        warned_invalid_ref_count = true;
      }

      SK_D3D11_LiveTextureView (can_scroll, pTLS);
    }

    ImGui::EndChild ();

    scale = (live_rt_view ? (1.0f * (uncollapsed_tex ? 0.25f : 1.0f)) : -1.0f);

    ImGui::BeginChild     ( ImGui::GetID ("Live_RenderTarget_View_Panel"),
                            ImVec2 ( -1.0f, scale == -1.0f ? font_size_multiline * 1.666f :
                   ( ImGui::GetWindowContentRegionMax ().y - ImGui::GetWindowContentRegionMin ().y ) *
                                    scale - (scale == 1.0f ? font_size_multiline * 1.666f : 0.0f) ),
                              true,
                                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

    live_rt_view =
        ImGui::CollapsingHeader ("Live RenderTarget View", ImGuiTreeNodeFlags_DefaultOpen);

    SK_D3D11_KnownTargets::_mod_tool_wants =
        live_rt_view;
    if (live_rt_view)
    {
      std::unordered_map < ID3D11RenderTargetView*, UINT > rt_indexes;

      std::scoped_lock <SK_Thread_CriticalSection> auto_lock (*cs_render_view);

      //SK_AutoCriticalSection auto_cs_rv (&cs_render_view, true);

      //if (auto_cs2.try_result ())
      {
      static float last_ht    = 256.0f;
      static float last_width = 256.0f;

      static std::vector <std::string> list_contents;
      static int                       list_filled    =    0;
      static bool                      list_dirty     = true;
      static UINT                      last_sel_idx   =    0;
      static size_t                    sel            = std::numeric_limits <size_t>::max ();
      static bool                      first_frame    = true;

      std::set < SK_ComPtr <ID3D11RenderTargetView> > live_textures;

      struct lifetime
      {
        ULONG64 last_frame;
        ULONG64 frames_active;
      };

      ULONG64 frames_drawn =
        SK_GetFramesDrawn ();

      std::unordered_map < ID3D11RenderTargetView* , lifetime> render_lifetime;
      std::vector        < ID3D11RenderTargetView* >           render_textures;

      //render_textures.reserve (128);
      //render_textures.clear   ();

      const UINT dev_idx =
        SK_D3D11_GetDeviceContextHandle (rb.d3d11.immediate_ctx);

      std::set <ID3D11RenderTargetView*> invalid_views;

      //for (auto& rtl : *SK_D3D11_RenderTargets )
      auto& rtl =
        SK_D3D11_RenderTargets [dev_idx];

      if (! ( rtl.rt_views.empty () ||
              rtl.max_rt_views   == 0 ) )
      {
        for ( auto it : rtl.rt_views )
        {
          D3D11_RENDER_TARGET_VIEW_DESC desc = { };

          if (it == nullptr || invalid_views.contains (it))
            continue;

          if (! SK_D3D11_IsValidRTV (it))
          {
            desc.Format = DXGI_FORMAT_UNKNOWN;
          }

          else
          {
            it->GetDesc (&desc);
          }

          if (desc.Format == DXGI_FORMAT_UNKNOWN)
          {
            invalid_views.emplace (it);
            continue;
          }

          if ( desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D ||
               desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMS )
          {
            SK_ComPtr <ID3D11Resource>  pRes = nullptr;
            SK_ComPtr <ID3D11Texture2D> pTex = nullptr;

            it->GetResource (&pRes.p);

            if (pRes.p != nullptr)
                pRes->QueryInterface <ID3D11Texture2D> (&pTex.p);

            if (pTex.p == nullptr)
            {
              invalid_views.emplace (it);
              continue;
            }

            if ( pRes != nullptr &&
                 pTex != nullptr )
            {
              D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = { };

              srv_desc.Format                    = desc.Format;
              srv_desc.Texture2D.MipLevels       = desc.Texture2D.MipSlice + 1;
              srv_desc.Texture2D.MostDetailedMip = desc.Texture2D.MipSlice;
              srv_desc.ViewDimension             = desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMS ?
                                                                         D3D11_SRV_DIMENSION_TEXTURE2DMS :
                                                                         D3D11_SRV_DIMENSION_TEXTURE2D;

              auto pDev =
                rb.getDevice <ID3D11Device> ();

              if (pDev != nullptr)
              {
                if (render_lifetime.count     (it) == 0)
                {   render_textures.push_back (it);
                    render_lifetime.insert    (
                               std::make_pair (it,
                                    lifetime { frames_drawn, 1 })
                    );
                }

                else
                {
                  auto&    lifetime =
                    render_lifetime [it];

                  lifetime.frames_active++;
                  lifetime.last_frame = frames_drawn;
                }

                live_textures.insert (it);
              }
            }
          }
        }
      }

      for ( auto it : invalid_views )
      {
        if (rtl.rt_views.count (it))
            rtl.rt_views.erase (it);
      }

   ///////constexpr ULONG64      zombie_threshold = 4;//120;
   ///////   static ULONG64 last_zombie_pass      = frames_drawn;
   ///////
   ///////   if (last_zombie_pass <= frames_drawn - zombie_threshold / 2)
   ///////   {
   ///////     bool newly_dead = false;
   ///////
   ///////     const auto time_to_live =
   ///////       frames_drawn - zombie_threshold;
   ///////
   ///////     for ( auto it : render_textures )
   ///////     {
   ///////       if ( render_lifetime.count (it) != 0 &&
   ///////                  render_lifetime [it].last_frame < time_to_live )
   ///////       {
   ///////         render_lifetime.erase (it);
   ///////         newly_dead = true;
   ///////       }
   ///////     }
   ///////
   ///////     if (newly_dead)
   ///////     {
   ///////       render_textures.clear ();
   ///////
   ///////       for ( auto& it : render_lifetime )
   ///////         render_textures.push_back (it.first);
   ///////     }
   ///////
   ///////     last_zombie_pass = frames_drawn;
   ///////   }


      std::unordered_set <ID3D11RenderTargetView *> discard_views;

      static volatile
        LONG idx_counter = 0;

      static size_t
        list_max_text_len = 7;

      if (list_dirty)
      {
            sel = std::numeric_limits <size_t>::max ();
        int idx = 0;

        std::vector <std::pair <ID3D11RenderTargetView*, UINT>>
          rt2;
          rt2.reserve (render_textures.size ());

        for ( auto it : render_textures )
        {
          UINT size = sizeof (UINT);
          UINT data = 0;

          if (live_textures.contains (it))
          {
            if (! SK_D3D11_IsValidRTV (it))
              continue;

            if ( FAILED ( it->GetPrivateData (
                            SKID_D3D11DeviceContextHandle, &size, &data ) ) )
            {
              size = sizeof (UINT);
              data =
                ( InterlockedIncrement (&idx_counter) + 1 );

              it->SetPrivateData (
                SKID_D3D11DeviceContextHandle, size, &data
              );
            }

            rt2.emplace_back (
              std::make_pair (it, data)
            );
          }
        }

        // The underlying list is unsorted for speed, but that's not at all
        //   intuitive to humans, so sort the thing when we have the RT view open.
        std::sort ( std::execution::par,
                        rt2.begin (),
                        rt2.end   (),
          []( std::pair <ID3D11RenderTargetView*, UINT> a,
              std::pair <ID3D11RenderTargetView*, UINT> b )
          {
            UINT ax = a.second;
            UINT bx = b.second;

            if (ax == std::numeric_limits <size_t>::max () ||
                bx == std::numeric_limits <size_t>::max ())
            {
              return false;
            }

            return ( ax < bx );
          }
        );

        std::vector        < ID3D11RenderTargetView*       > rt1;

        for ( auto& it : rt2 )
        {
          rt1.emplace_back       (it.first);
          rt_indexes [it.first] = it.second;
        }

        std::swap (rt1, render_textures);

        static char
          szDesc [128] = { };

        static std::vector <std::string> temp_list;
                                         temp_list.reserve (render_textures.size ());
                                         temp_list.clear   ();

        list_max_text_len = 7;

        for ( auto it : render_textures )
        {
          if (it == nullptr)
            continue;

          char     szDebugDesc [128] = { };
          wchar_t wszDebugDesc [128] = { };
          UINT     uiDebugLen        = 127;

          bool named = false;

          UINT rtv_idx = 0;

          if (live_textures.count (it) != 0)
          {
            if (! SK_D3D11_IsValidRTV (it))
            {
              SK_LOG1 ( (L" >> RTV name lifetime shorter than object."),
                         L"  D3D 11  " );
              continue;
            }

            rtv_idx =
              rt_indexes [it];

            if (rtv_idx != std::numeric_limits <UINT>::max ())
            {
              uiDebugLen = sizeof (wszDebugDesc) - sizeof (wchar_t);

              if ( SUCCEEDED (
                     it->GetPrivateData (
                       WKPDID_D3DDebugObjectNameW, &uiDebugLen, wszDebugDesc )
                              )                  && uiDebugLen > sizeof (wchar_t)
                 )
              {
                snprintf (szDesc, 127, "%ws###rtv_%u", wszDebugDesc, rtv_idx);
                named = true;
              }

              else
              {
                uiDebugLen = sizeof (szDebugDesc) - sizeof (char);

                if ( SUCCEEDED (
                     it->GetPrivateData (
                       WKPDID_D3DDebugObjectName, &uiDebugLen, szDebugDesc )
                               )                && uiDebugLen > sizeof (char)
                   )
                {
                  snprintf (szDesc, 127, "%s###rtv_%u", szDebugDesc, rtv_idx);
                  named = true;
                }

                else
                {
                  uiDebugLen = sizeof (wszDebugDesc) - sizeof (wchar_t);

                  SK_ComPtr <ID3D11Resource> pResource;
                  it->GetResource (         &pResource);

                  if ( SUCCEEDED (
                     pResource->GetPrivateData (
                       WKPDID_D3DDebugObjectNameW, &uiDebugLen, wszDebugDesc )
                                 )               && uiDebugLen > sizeof (wchar_t)
                     )
                  {
                    snprintf (szDesc, 127, "%ws###rtv_%u", wszDebugDesc, rtv_idx);
                    named = true;
                  }

                  else
                  {
                    uiDebugLen = sizeof (szDebugDesc) - sizeof (char);

                    if ( SUCCEEDED (
                         pResource->GetPrivateData (
                           WKPDID_D3DDebugObjectName, &uiDebugLen, szDebugDesc )
                                   )                && uiDebugLen > sizeof (char)
                       )
                    {
                      snprintf (szDesc, 127, "%s###rtv_%u", szDebugDesc, rtv_idx);
                      named = true;
                    }
                  }
                }
              }
            }

            else { discard_views.emplace (it); }
          }
          else   { discard_views.emplace (it); }

          if (! named)
          {
            sprintf ( szDesc, "%07u###rtv_%u",
                       (discard_views.count (it) == 0) ? rtv_idx :
                             ReadAcquire (&idx_counter), rtv_idx );
          }

          list_max_text_len =
            std::max ( list_max_text_len,
                         (size_t)ImGui::CalcTextSize (szDesc, nullptr, true).x );

          temp_list.emplace_back (szDesc);

          if (rtv_idx == last_sel_idx)
          {
            sel = idx;
          }

          ++idx;
        }

        std::swap (list_contents, temp_list);
      }

      static bool hovered = false;
      static bool focused = false;

      if (hovered || focused)
      {
        can_scroll = false;

        if (! render_textures.empty ())
        {
          if (! focused)//hovered)
          {
            ImGui::BeginTooltip ();
            ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "You can view the output of individual render passes");
            ImGui::Separator    ();
            ImGui::BulletText   ("Press %ws while the mouse is hovering this list to select the previous output", virtKeyCodeToHumanKeyName [VK_OEM_4]);
            ImGui::BulletText   ("Press %ws while the mouse is hovering this list to select the next output",     virtKeyCodeToHumanKeyName [VK_OEM_6]);
            ImGui::EndTooltip   ();
          }

          else
          {
            ImGui::BeginTooltip ();
            ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "You can view the output of individual render passes");
            ImGui::Separator    ();
            ImGui::BulletText   ("Press LB to select the previous output");
            ImGui::BulletText   ("Press RB to select the next output");
            ImGui::EndTooltip   ();
          }

          int direction = 0;

               if (io.KeysDown [VK_OEM_4] && io.KeysDownDuration [VK_OEM_4] == 0.0f) { direction--;  io.WantCaptureKeyboard = true; }
          else if (io.KeysDown [VK_OEM_6] && io.KeysDownDuration [VK_OEM_6] == 0.0f) { direction++;  io.WantCaptureKeyboard = true; }

          else {
                 if (io.NavInputs [ImGuiNavInput_FocusPrev] && io.NavInputsDownDuration [ImGuiNavInput_FocusPrev] == 0.0f) { direction--; }
            else if (io.NavInputs [ImGuiNavInput_FocusNext] && io.NavInputsDownDuration [ImGuiNavInput_FocusNext] == 0.0f) { direction++; }
          }

          int neutral_idx = 0;

          for (UINT i = 0; i < render_textures.size (); i++)
          {
            if (rt_indexes [render_textures [i]] >= last_sel_idx)
            {
              neutral_idx = i;
              break;
            }
          }

          size_t last_sel = sel;
                      sel =
            sk::narrow_cast <size_t> (neutral_idx) + direction;

          if ((SSIZE_T)sel <  0) sel = 0;

          if ((ULONG)sel >= (ULONG)render_textures.size ())
          {
            sel = render_textures.size () - 1;
          }

          if ((SSIZE_T)sel <  0) sel = 0;

          if (direction != 0 && last_sel != sel)
          {
            last_sel_idx  =
              rt_indexes [render_textures [sel]];
          }
        }
      }

      ImGui::BeginGroup     ();
      ImGui::PushStyleVar   (ImGuiStyleVar_ChildRounding, 0.0f);
      ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.9f, 0.7f, 0.5f, 1.0f));

      const float text_spacing = 2.5f * ImGui::GetStyle ().ItemInnerSpacing.x +
                                        ImGui::GetStyle ().ScrollbarSize;

      ImGui::BeginChild ( ImGui::GetID ("RenderTargetViewList"),
                          ImVec2 ( io.FontGlobalScale * list_max_text_len +
                                   io.FontGlobalScale * text_spacing, -1.0f),
                            true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

      if (! render_textures.empty ())
      {
        ImGui::BeginGroup ();

        if (first_frame)
        {
          sel         = 0;
          first_frame = false;
        }

        static bool sel_changed  = false;

        ///if ((SSIZE_T)sel >= 0 && sel < (int)render_textures.size ())
        ///{
        ///  if (last_sel_idx != _GetStashedRTVIndex (render_textures [sel]))
        ///  {
        ///    int i = 0;
        ///
        ///    for ( auto& entry : render_textures )
        ///    {
        ///      if (_GetStashedRTVIndex (entry) == last_sel_idx)
        ///      {
        ///        sel         = i;
        ///        sel_changed = true;
        ///        break;
        ///      }
        ///
        ///      ++i;
        ///    }
        ///  }
        ///}

        for ( UINT line = 0; line < list_contents.size (); line++ )
        {
          ImGuiSelectableFlags flags =
            discard_views.count (render_textures [line]) ?
                           ImGuiSelectableFlags_Disabled : 0;

          bool selected = (! sel_changed) &&
            ( rt_indexes [render_textures [line]] == last_sel_idx );

          if (selected) { sel = line; }

          if (line == sel)
          {
            ImGui::Selectable (list_contents [line].c_str (), &selected, flags);

            if (sel_changed)
            {
              if (! ImGui::IsItemVisible  ())
                ImGui::SetScrollHereY     (0.5f);
              ImGui::SetKeyboardFocusHere (    );

              sel_changed  = false;
              last_sel_idx =
                rt_indexes [render_textures [sel]];

              InterlockedExchangePointer ( (PVOID *)&tracked_rtv->resource,
                                              render_textures [sel] );
            }
          }

          else
          {
            if (ImGui::Selectable (list_contents [line].c_str (), &selected, flags))
            {
              if (selected)
              {
                sel_changed          = true;
                sel                  =  line;
                last_sel_idx         =
                  rt_indexes [render_textures [sel]];

                InterlockedExchangePointer ( (PVOID *)&tracked_rtv->resource,
                                                render_textures [sel] );
              }
            }
          }
        }

        ImGui::EndGroup ();
      }

      ImGui::EndChild      ();
      ImGui::PopStyleColor ();
      ImGui::PopStyleVar   ();
      ImGui::EndGroup      ();


      if (ImGui::IsItemHovered (ImGuiHoveredFlags_RectOnly))
      {
        hovered = ImGui::IsItemHovered ();
        focused = ImGui::IsItemFocused ();
      }

      else
      {
        hovered = false; focused = false;
      }


      if ( render_textures.size  () >      (size_t)sel   &&
             live_textures.count (render_textures [sel]) &&
             discard_views.count (render_textures [sel]) == 0 )
      {
        SK_ComPtr <ID3D11RenderTargetView>
          rt_view (render_textures [sel]);

        D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = { };
        rt_view->GetDesc            (&rtv_desc);

        D3D11_TEXTURE2D_DESC desc = { };

        ULONG refs =
          rt_view.p->AddRef  () - 1;
          rt_view.p->Release ();

        if ( rtv_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D ||
             rtv_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMS )
        {
          SK_ComPtr   <ID3D11Resource>        pRes = nullptr;
                       rt_view->GetResource (&pRes.p);
          SK_ComQIPtr <ID3D11Texture2D> pTex (pRes.p);

          if ( pRes != nullptr &&
               pTex != nullptr )
          {
            pTex->GetDesc (&desc);

            auto pDev =
              rb.getDevice <ID3D11Device> ();

            SK_ComPtr   <ID3D11ShaderResourceView> pSRV;

            SK_D3D11_MakeDrawableCopy (pDev, pTex, rt_view, &pSRV.p);

            //D3D11_SHADER_RESOURCE_VIEW_DESC  srv_desc = { };
            //
            //srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
            //srv_desc.Format                    = SK_D3D11_MakeTypedFormat (rtv_desc.Format);
            //srv_desc.Texture2D.MipLevels       = (UINT)-1;
            //srv_desc.Texture2D.MostDetailedMip =  0;

            if (pDev != nullptr)
            {
              const size_t row0  = std::max (tracked_rtv->ref_vs.size (), tracked_rtv->ref_ps.size ());
              const size_t row1  =           tracked_rtv->ref_gs.size ();
              const size_t row2  = std::max (tracked_rtv->ref_hs.size (), tracked_rtv->ref_ds.size ());
              const size_t row3  =           tracked_rtv->ref_cs.size ();

              const size_t bottom_list = row0 + row1 + row2 + row3;

              const bool success = ( pSRV.p != nullptr );

              const float content_avail_y = ImGui::GetWindowContentRegionMax ().y - ImGui::GetWindowContentRegionMin ().y;
              const float content_avail_x = ImGui::GetWindowContentRegionMax ().x - ImGui::GetWindowContentRegionMin ().x;
                    float effective_width = 0.0f, effective_height = 0.0f;

              if (success)
              {
                // Some Render Targets are MASSIVE, let's try to keep the damn things on the screen ;)
                if (bottom_list > 0)
                  effective_height = std::max (256.0f, content_avail_y - ((float)(bottom_list + 4) * font_size_multiline * 1.125f));
                else
                  effective_height = std::max (256.0f, std::max (content_avail_y, (float)desc.Height));

                effective_width    = effective_height  * ((float)desc.Width / (float)desc.Height );

                if (effective_width > content_avail_x)
                {
                  effective_width  = std::max (content_avail_x, 256.0f);
                  effective_height = effective_width * ((float)desc.Height / (float)desc.Width);
                }
              }

              ImGui::SameLine ();

              ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.5f, 0.5f, 0.5f, 1.0f));
              ImGui::BeginChild      ( ImGui::GetID ("RenderTargetPreview"),
                                       ImVec2 ( -1.0f, -1.0f ),
                                         true,
                                           ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

              last_width  = content_avail_x;//effective_width;
              last_ht     = content_avail_y;//effective_height + ( font_size_multiline * (bottom_list + 4) * 1.125f );

              ImGui::BeginGroup (                  );
              ImGui::Text       ( "Dimensions:   " );
              ImGui::Text       ( "Format:       " );
              ImGui::Text       ( "Usage:        " );
              ImGui::EndGroup   (                  );

              ImGui::SameLine   ( );

              ImGui::BeginGroup (                                             );
              ImGui::Text       ( "%lux%lu",
                                    desc.Width, desc.Height/*, effective_width, effective_height, 0.9875f * content_avail_y - ((float)(bottom_list + 3) * font_size * 1.125f), content_avail_y*//*,
                                      pTex->d3d9_tex->GetLevelCount ()*/      );
              ImGui::Text       ( "%hs",
                                    SK_DXGI_FormatToStr (desc.Format).data () );
              ImGui::Text       ( "%hs",
                                    SK_D3D11_DescribeUsage (desc.Usage) );
              ImGui::EndGroup   ();

              ImGui::SameLine   ();

              ImGui::BeginGroup ();
              ImGui::Text       ( "References:   " );
              ImGui::Text       ( "Bind Flags:   " );
              ImGui::Text       ( "Misc. Flags:  " );
              ImGui::EndGroup   (                  );

              ImGui::SameLine   ();

              bool multisampled =
                rtv_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMS;

              ImGui::BeginGroup ();
              ImGui::Text       ( "%lu",                   refs );
              ImGui::Text       ( "%hs", SK_D3D11_DescribeBindFlags ((D3D11_BIND_FLAG)         desc.BindFlags).c_str ());
              ImGui::Text       ( multisampled ? "Multi-Sampled (%lux)" : "", desc.SampleDesc.Count); ImGui::SameLine ();
              ImGui::Text       ( "%hs", SK_D3D11_DescribeMiscFlags ((D3D11_RESOURCE_MISC_FLAG)desc.MiscFlags).c_str ());
              ImGui::EndGroup   (                  );

              if (success && pSRV != nullptr)
              {
                ImGui::Separator  ( );

                ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));
                ImGui::BeginChildFrame   (ImGui::GetID ("ShaderResourceView_Frame"),
                                            ImVec2 (effective_width + 8.0f, effective_height + 8.0f),
                                            ImGuiWindowFlags_AlwaysAutoResize );

                SK_D3D11_TempResources->push_back (pSRV.p);

                ImGui::Image             ( pSRV.p,
                                             ImVec2 (effective_width, effective_height),
                                               ImVec2  (0,0),             ImVec2  (1,1),
                                               ImColor (255,255,255,255), ImColor (255,255,255,128)
                                         );

#if 0
                if (ImGui::IsItemHovered ())
                {
                  ImGui::BeginTooltip ();
                  ImGui::BeginGroup   ();
                  ImGui::TextUnformatted ("Mip Levels:   ");
                  if (desc.SampleDesc.Count > 1)
                  {
                    ImGui::TextUnformatted ("Sample Count: ");
                    ImGui::TextUnformatted ("MSAA Quality: ");
                  }
                  ImGui::TextUnformatted ("Usage:        ");
                  ImGui::TextUnformatted ("Bind Flags:   ");
                  ImGui::TextUnformatted ("CPU Access:   ");
                  ImGui::TextUnformatted ("Misc Flags:   ");
                  ImGui::EndGroup     ();

                  ImGui::SameLine     ();

                  ImGui::BeginGroup   ();
                  ImGui::Text ("%u", desc.MipLevels);
                  if (desc.SampleDesc.Count > 1)
                  {
                    ImGui::Text ("%u", desc.SampleDesc.Count);
                    ImGui::Text ("%u", desc.SampleDesc.Quality);
                  }
                  ImGui::Text (      "%hs", SK_D3D11_DescribeUsage (desc.Usage));
                  ImGui::Text ("%u (  %hs)", desc.BindFlags,
                                          SK_D3D11_DescribeBindFlags (
                    (D3D11_BIND_FLAG)desc.BindFlags).c_str ());
                  ImGui::Text ("%x", desc.CPUAccessFlags);
                  ImGui::Text ("%x", desc.MiscFlags);
                  ImGui::EndGroup   ();
                  ImGui::EndTooltip ();
                }
#endif

                ImGui::EndChildFrame     (    );
                ImGui::PopStyleColor     (    );
              }

              if (bottom_list > 0)
              {
                ImGui::Separator  ( );

                SK_D3D11_ShaderModDlg_RTVContributors ();
              }

              ImGui::EndChild      ( );
              ImGui::PopStyleColor (1);
            }
          }
        }
      }
      }
    }

    ImGui::EndChild     ( );
    ImGui::EndChild     ( );
    ImGui::Columns      (1);

    ImGui::PopItemWidth ( );
  }

  ImGui::End            ( );

  SK_D3D11_EnableTracking =
         show_dlg;
  return show_dlg;
}