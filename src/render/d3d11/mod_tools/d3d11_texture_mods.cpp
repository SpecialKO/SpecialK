// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
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
#define __SK_SUBSYSTEM__ L"D3D 11 Tex"

#include <SpecialK/render/d3d11/d3d11_core.h>
#include <SpecialK/render/d3d11/d3d11_tex_mgr.h>
#include <SpecialK/render/d3d11/d3d11_state_tracker.h>
#include <SpecialK/render/d3d11/utility/d3d11_texture.h>

#include <algorithm>
#include <execution>

bool convert_typeless = false;

static size_t debug_tex_id = 0x0;
static size_t tex_dbg_idx  = 0;

struct SK_D3D11_TextureListFilter {
  struct {
    UINT minWidth = 1,     minHeight = 1;
    UINT maxWidth = 16384, maxHeight = 16384;
  } resolution;

  struct {
    bool complete   = true;
    bool incomplete = true;
    bool single_lod = true;
  } mipmaps;

  struct {
    bool bc7           = true;
    bool bc6h          = true;
    bool bc5           = true;
    bool bc4           = true;
    bool bc3           = true;
    bool bc2           = true;
    bool bc1           = true;
    bool r8g8b8a8      = true;
    bool b8g8r8a8      = true;
    bool r10g10b10a2   = true;
    bool r11g11b10     = true;
    bool rgba16        = true;
    bool rgba32        = true;
    bool rg16          = true;
    bool rg32          = true;
    bool others        = true;
  } formats;

  struct {
    bool dynamic_usage   = true;
    bool immutable_usage = true;
    bool default_usage   = true;
    bool staging_usage   = true;
  } usages;

  struct {
    bool vertex_buffer   = true;
    bool index_buffer    = true;
    bool constant_buffer = true;
    bool shader_resource = true;
    bool stream_output   = true;
    bool render_target   = true;
    bool depth_stencil   = true;
    bool unordered_access= true;
    bool video_decoder   = true;
    bool video_encoder   = true;
  } bind_types;

  struct {
    bool read            =  true;
    bool write           =  true;
    bool only_read_write = false;
  } cpu_access;

  struct {
    bool generate_mips              =  true;
    bool shared_texture             =  true;
    bool cubemap                    =  true;
    bool drawindirect_args          = false;
    bool allow_raw_buffer_views     = false;
    bool structured_buffer          = false;
    bool resource_clamp             =  true;
    bool shared_keymutex            =  true;
    bool gdi_compatible             =  true;
    bool shared_nthandle            =  true;
    bool restricted_content         = false;
    bool drm_shared_resource        = false;
    bool drm_shared_resource_driver = false;
    bool guarded                    =  true;
    bool tile_pool                  =  true;
    bool tiled                      =  true;
    bool hw_drm_protected           =  true;
    bool shared_displayable         =  true;
    bool shared_exclusive_writer    =  true;
  } misc_flags;

  struct {
    bool show_modified   = true;
    bool show_unmodified = true;
  } tex_mods;
};

void
SK_D3D11_LiveTextureView (bool& can_scroll, SK_TLS* pTLS = SK_TLS_Bottom ())
{
  auto& io =
    ImGui::GetIO ();

  const SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  auto& textures =
    SK_D3D11_Textures;

  auto& TexRefs_2D =
     textures->TexRefs_2D;

  auto& Textures_2D =
    textures->Textures_2D;

    std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_render_view);

  const float font_size           = ImGui::GetFont ()->FontSize * io.FontGlobalScale;

  static float last_ht    = 256.0f;
  static float last_width = 256.0f;

  struct list_entry_s {
    std::string          name      = "I need an adult!";
    uint32_t             tag       = 0UL;
    uint32_t             crc32c    = 0UL;
    bool                 injected  = false;
    D3D11_TEXTURE2D_DESC desc      = { };
    D3D11_TEXTURE2D_DESC orig_desc = { };
    BOOL                 mipmapped =  -1; // We must calculate this
    ID3D11Texture2D*     pTex      = nullptr;
    //SK_ComPtr <ID3D11Texture2D>
    //                     pTex      = nullptr;
    size_t               size      = 0;
  };

  static std::vector <list_entry_s> list_contents;
  static std::unordered_map
           <uint32_t, list_entry_s> texture_map;
  static              bool          list_dirty      = true;
  static              bool          lod_list_dirty  = true;
  static              float         max_name_len    = 0.0f; // For dynamic texture name list sizing
  static              size_t        sel             =    0;
  static              int           tex_set         =    1;
  static              int           lod             =    0;
  static              char          lod_list [1024]   {  };

  static              int           refresh_interval     = 0UL; // > 0 = Periodic Refresh
  static              ULONG64       last_frame           = 0UL;
  static              size_t        total_texture_memory = 0ULL;
  static              size_t        non_mipped           = 0ULL; // Num Non-mimpapped textures loaded

  // Whether to include these incredibly compute-intensive to encode textures
  //   in operations like mipmap generation + re-encoding.
  const bool bIncludeBC7andBC6H = false;

  ImGui::SameLine ();
  ImGui::Text     ("\tCurrent list represents %5.2f MiB of texture memory",
                      (double)total_texture_memory / (1024.0 * 1024.0) );

  ImGui::PushID     ("Texture2D_D3D11");
  ImGui::BeginGroup ();

  if (InterlockedCompareExchange (&SK_D3D11_LiveTexturesDirty, FALSE, TRUE))
  {
    texture_map.clear   ();
    list_contents.clear ();

    last_ht    =  0.0f;
    last_width =  0.0f;
    lod        =  0;

    list_dirty = true;
  }

  if (ImGui::Button ("  Refresh Textures  "))
  {
    list_dirty = true;
  }

  if (ImGui::IsItemHovered ())
  {
    if (tex_set == 1) ImGui::SetTooltip ("Refresh the list using textures drawn during the last frame.");
    else              ImGui::SetTooltip ("Refresh the list using ALL cached textures.");
  }

  if (non_mipped > 0)
  {
    ImGui::SameLine ();

    if (ImGui::Button ("  Generate Mipmaps  ###GenerateMipmaps_ALL"))
    {
      for (auto& entry_it : texture_map)
      {
        auto& entry = entry_it.second;

        if (! entry.injected)
        {
          if (! SK_D3D11_IsDumped (entry.crc32c, 0x0))
          {
            bool skip = false;

            static bool bToV =
              (SK_GetCurrentGameID () == SK_GAME_ID::Tales_of_Vesperia);

            if (bToV)
            {
              if ( StrStrIA (entry.name.c_str (), "E_")    == entry.name.c_str () ||
                   StrStrIA (entry.name.c_str (), "U_")    == entry.name.c_str () ||
                   StrStrIA (entry.name.c_str (), "LOGO_") == entry.name.c_str () )
              {
                skip = true;
              }
            }

            if (! skip)
            {
              switch (entry.desc.Format)
              {
                // These formats take an eternity!
                case DXGI_FORMAT_BC6H_TYPELESS: 
                case DXGI_FORMAT_BC6H_UF16:
                case DXGI_FORMAT_BC6H_SF16:
                case DXGI_FORMAT_BC7_TYPELESS:
                case DXGI_FORMAT_BC7_UNORM:
                case DXGI_FORMAT_BC7_UNORM_SRGB:
                  skip = (! bIncludeBC7andBC6H);
                default:
                  break;
              }
            }

            if ((! skip) && SUCCEEDED (SK_D3D11_MipmapCacheTexture2D (entry.pTex, entry.crc32c, pTLS)))
            {
              entry.mipmapped = TRUE;
              non_mipped--;
            }

            else if (skip)
            {
              if (DirectX::MakeTypeless (entry.desc.Format) == DXGI_FORMAT_BC7_TYPELESS)
              {
                SK_ScopedBool decl_tex_scope (
                  SK_D3D11_DeclareTexInjectScope (pTLS)
                );

                SK_ComPtr <ID3D11Device>        pDev    (rb.d3d11.device);
                SK_ComPtr <ID3D11DeviceContext> pDevCtx (rb.d3d11.immediate_ctx);
                DirectX::ScratchImage                                              captured;
                if (SUCCEEDED (DirectX::CaptureTexture (pDev, pDevCtx, entry.pTex, captured)))
                {
                  DXGI_FORMAT uncompressed_fmt =
                    (entry.desc.Format == DXGI_FORMAT_BC7_TYPELESS)   ? DXGI_FORMAT_R8G8B8A8_UNORM :
                    (entry.desc.Format == DXGI_FORMAT_BC7_UNORM)      ? DXGI_FORMAT_R8G8B8A8_UNORM :
                    (entry.desc.Format == DXGI_FORMAT_BC7_UNORM_SRGB) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
                                                                      : DXGI_FORMAT_UNKNOWN;
                  DirectX::ScratchImage                                              converted;
                  DirectX::Decompress (*captured.GetImage (0,0,0), uncompressed_fmt, converted);

                  auto desc      = entry.desc;
                  desc.Format    = uncompressed_fmt;
                  desc.MipLevels = 1;

                  auto metadata =
                    converted.GetMetadata ();

                  metadata.mipLevels = 1;

                  SK_ComPtr <ID3D11Texture2D>                                                            pNewTex;
                  DirectX::CreateTexture (pDev, converted.GetImages (), 1, metadata, (ID3D11Resource **)&pNewTex.p);

                  if (SUCCEEDED (SK_D3D11_MipmapCacheTexture2D (pNewTex, entry.crc32c, pTLS)))
                  {
                    entry.mipmapped = TRUE;
                    non_mipped--;
                  }
                }
              }
            }
          }
        }
      }
    }

    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("There are currently %lu textures without mipmaps", non_mipped);
  }

  static float const slen =
        (float)strlen ("Used Textures   ") / 2.0f;

  ImGui::SameLine      ();
  ImGui::PushItemWidth (font_size * slen);

  ImGui::Combo ("###TexturesD3D11_TextureSet", &tex_set, "All Textures\0Used Textures\0\0", 2);

  ImGui::PopItemWidth ();
  ImGui::SameLine     ();

  if (ImGui::Button (" Clear Debug "))
  {
    sel                     = std::numeric_limits <size_t>::max ();
    debug_tex_id            =  0;
    list_contents.clear ();
    last_ht                 =  0.0f;
    last_width              =  0.0f;
    lod                     =  0;
    SK_D3D11_TrackedTexture =  nullptr;
  }

  if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("Exits texture debug mode.");

  ImGui::SameLine ();

  if (ImGui::Button ("  Reload All Injected Textures  "))
  {
    SK_D3D11_ReloadAllTextures ();
  }

//ImGui::SameLine ();
  ImGui::Checkbox ("Highlight Selected Texture in Game##D3D11_HighlightSelectedTexture",
                                       &config.textures.d3d11.highlight_debug);
  ImGui::SameLine ();

  static bool hide_inactive = false;

  ImGui::Checkbox  ("Hide Inactive Textures##D3D11_HideInactiveTextures",
                                                  &hide_inactive);

  ImGui::SameLine  ();

  ImGui::PushItemWidth (font_size * slen);
  ImGui::SliderInt     ("Frames Between List Refreshes", &refresh_interval, 0, 120);
  ImGui::PopItemWidth  ();

  ImGui::Separator ();
  ImGui::EndGroup  ();


  extern SK_ComPtr <ID3D11Texture2D> SK_HDR_GetGamutTex     (void);
  extern SK_ComPtr <ID3D11Texture2D> SK_HDR_GetLuminanceTex (void);

  auto pHDRGamut     = SK_HDR_GetGamutTex     ();
  auto pHDRLuminance = SK_HDR_GetLuminanceTex ();

  for (auto& it_ctx : *SK_D3D11_PerCtxResources )
  {
    int spins = 0;

    while (InterlockedCompareExchange (&it_ctx.writing_, 1, 0) != 0)
    {
      if ( ++spins > 0x1000 )
      {
        SK_Sleep (1);
        spins  =  0;
      }
    }

    for ( auto& it_res : it_ctx.used_textures )
    {
      if (it_res != nullptr)
      {
        used_textures->insert (it_res);
      }
    }

    it_ctx.used_textures.clear ();

    InterlockedExchange (&it_ctx.writing_, 0);
  }


  if (pHDRGamut.p     != nullptr) used_textures->insert (pHDRGamut);
  if (pHDRLuminance.p != nullptr) used_textures->insert (pHDRLuminance);


  if (   list_dirty ||    refresh_interval > 0)
  {
    if ( list_dirty || ( (refresh_interval + last_frame) <
                                      (LONG)SK_GetFramesDrawn () ) )
    {
      list_dirty           = true;
      last_frame           = SK_GetFramesDrawn ();
      total_texture_memory = 0ULL;
      non_mipped           = 0ULL;
    }
  }

  if (list_dirty)
  {
    std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock1 (*cs_render_view);

    if (debug_tex_id == 0)
      last_ht = 0.0f;

    max_name_len = 0.0f;

    {
      texture_map.reserve (textures->HashMap_2D.size ());
      
      std::vector <lod_hash_table_s*>
                       hash_tables;

      if (config.textures.d3d11.use_l3_hash)
      {
        if (! textures->HashMap_Fmt.empty ())
              textures->updateDebugNames  ();

        for (auto& it0  : textures->HashMap_Fmt)
        { // Relatively immutable textures
          for (auto& it : it0.map)
            hash_tables.emplace_back (&it);
        }
      }

      else
      {
        if (! textures->HashMap_2D.empty ())
              textures->updateDebugNames ();
      
        // Relatively immutable textures
        for (auto& it : textures->HashMap_2D)
          hash_tables.emplace_back (&it);
      }

      for (auto *it : hash_tables)
      {
        std::scoped_lock <SK_Thread_HybridSpinlock>
                   _lock (*(it->mutex));

        for (auto& it2 : it->entries)
        {
          if (it2.second == nullptr)
            continue;

          const auto& tex_ref =
            TexRefs_2D.find (it2.second);

          if ( tex_ref != TexRefs_2D.cend () )
          {
            list_entry_s
              entry        = {       };
              entry.tag    = it2.first;
              entry.pTex   = it2.second;
              entry.name.clear ();

            if (SK_D3D11_TextureIsCached (it2.second))
            {
              const SK_D3D11_TexMgr::tex2D_descriptor_s& desc =
                textures->Textures_2D [it2.second];

              entry.desc      = desc.desc;
              entry.orig_desc = desc.orig_desc;
              entry.size      = desc.mem_size;
              entry.crc32c    = desc.crc32c;
              entry.injected  = desc.injected;

              if (desc.debug_name.empty ())
              {
                                 entry.name.resize (9);
                std::string_view entry_view
                               ( entry.name.data (),
                                 entry.name.size () );

                SK_FormatStringView (entry_view, "%08x", entry.crc32c);
              }
              else
                entry.name = desc.debug_name;
            }

            else
              continue;

            if (entry.size > 0 && entry.crc32c != 0x00)
              texture_map [entry.crc32c] = entry;
          }
        }
      }

      if ( pHDRGamut.p != nullptr )
      {
        list_entry_s
             entry        = {    };
             entry.tag    = 0x6969;
             entry.crc32c = 0x6969;
             entry.pTex   = pHDRGamut.p;
             entry.name   = "SK HDR Gamut";
             pHDRGamut.p->GetDesc (&entry.desc);
             pHDRGamut.p->GetDesc (&entry.orig_desc);
             entry.size   = 8 * 1024 * 1024;

        texture_map [entry.crc32c] = entry;
      }
      
      if ( pHDRLuminance.p != nullptr )
      {
        list_entry_s
             entry        = {    };
             entry.tag    = 0x6868;
             entry.crc32c = 0x6868;
             entry.pTex   = pHDRLuminance.p;
             entry.name   = "SK HDR Luminance";
             pHDRLuminance.p->GetDesc (&entry.desc);
             pHDRLuminance.p->GetDesc (&entry.orig_desc);
             entry.size   = 8 * 1024 * 1024;

        texture_map [entry.crc32c] = entry;
      }

      std::vector <list_entry_s> temp_list;
                                 temp_list.reserve (texture_map.size ());

      // Self-sorted list, yay :)
      for (auto& it : texture_map)
      {
        if (it.second.pTex == nullptr)
          continue;

        const bool active =
          ( used_textures->find (it.second.pTex) !=
            used_textures->cend (              )  );

        if (active || tex_set == 0)
        {
          list_entry_s entry        = { };

          entry.crc32c    = it.second.crc32c;
          entry.tag       = it.second.tag;
          entry.desc      = it.second.desc;
          entry.orig_desc = it.second.orig_desc;
          entry.name      = it.second.name;
          max_name_len    = std::max (max_name_len, ImGui::CalcTextSize (entry.name.c_str (), nullptr, true).x);

          entry.pTex      = it.second.pTex;
          entry.size      = it.second.size;
          entry.injected  = it.second.injected;

          entry.mipmapped =
            ( CalcMipmapLODs ( entry.desc.Width,
                               entry.desc.Height ) == entry.desc.MipLevels );

          if (! entry.mipmapped)
            non_mipped++;

          temp_list.emplace_back  (entry);
          total_texture_memory  += entry.size;
        }
      }

      std::sort ( std::execution::par,
                     temp_list.begin (),
                     temp_list.end   (),
        []( list_entry_s& a,
            list_entry_s& b ) noexcept
        {
          return
            ( a.name < b.name );
        }
      );

      std::swap (list_contents, temp_list);
    }

    list_dirty = false;

    if ((! TexRefs_2D.count (SK_D3D11_TrackedTexture)) ||
           Textures_2D      [SK_D3D11_TrackedTexture].crc32c == 0x0 )
    {
      SK_D3D11_TrackedTexture = nullptr;
    }
  }

  ImGui::BeginGroup ();

  ImGui::PushStyleVar   (ImGuiStyleVar_ChildRounding, 0.0f);
  ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.9f, 0.7f, 0.5f, 1.0f));

  const float text_spacing = 2.5f * ImGui::GetStyle ().ItemInnerSpacing.x +
                                    ImGui::GetStyle ().ScrollbarSize;

  ImGui::BeginChild ( ImGui::GetID ("D3D11_TexHashes_CRC32C"),
                      ImVec2 ( io.FontGlobalScale * text_spacing +
                               io.FontGlobalScale * max_name_len, std::max (font_size * 15.0f, last_ht)),
                        true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

  if (ImGui::IsWindowHovered ())
    can_scroll = false;

  static int draws = 0;

  if (! list_contents.empty ())
  {
    static size_t last_sel     = std::numeric_limits <size_t>::max ();
    static bool   sel_changed  = false;

    // Don't select a texture immediately
    if (sel != last_sel && draws++ != 0)
      sel_changed = true;

    last_sel = sel;

    for ( size_t line = 0; line < list_contents.size (); line++)
    {
      const bool active =
        ( used_textures->find (list_contents [line].pTex) !=
          used_textures->cend (                         )  );

      if (active)
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.95f, 0.95f, 0.95f, 1.0f));
      else
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.425f, 0.425f, 0.425f, 0.9f));

      if ((! hide_inactive) || active)
      {
        ImGui::PushID (list_contents [line].crc32c);

        if (line == sel)
        {
          bool selected = true;
          ImGui::Selectable (list_contents [line].name.c_str (), &selected);

          if (sel_changed)
          {
            if (! ImGui::IsItemVisible  ())
              ImGui::SetScrollHereY     (0.5f);
            ImGui::SetKeyboardFocusHere (    );

            sel_changed     = false;
            tex_dbg_idx     = line;
            debug_tex_id    = list_contents [line].crc32c;
    SK_D3D11_TrackedTexture = list_contents [line].pTex;
            lod             = 0;
            lod_list_dirty  = true;
            *lod_list       = '\0';
          }
        }

        else
        {
          bool selected = false;

          if (ImGui::Selectable (list_contents [line].name.c_str (), &selected))
          {
            sel_changed     = true;
            tex_dbg_idx     = line;
            sel             = line;
            debug_tex_id    = list_contents [line].crc32c;
    SK_D3D11_TrackedTexture = list_contents [line].pTex;
            lod             = 0;
            lod_list_dirty  = true;
            *lod_list       = '\0';
          }
        }

        ImGui::PopID ();
      }

      ImGui::PopStyleColor ();
    }
  }

  ImGui::EndChild ();

  if (ImGui::IsItemHovered () || ImGui::IsItemFocused ())
  {
    int dir = 0;

    if (ImGui::IsItemFocused ())
    {
      ImGui::BeginTooltip ();
      ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "");
      ImGui::Separator    ();
      ImGui::BulletText   ("Press LB to select the previous texture from this list");
      ImGui::BulletText   ("Press RB to select the next texture from this list");
      ImGui::EndTooltip   ();
    }

    else
    {
      ImGui::BeginTooltip ();
      ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "");
      ImGui::Separator    ();
      ImGui::BulletText   ("Press %hs to select the previous texture from this list", SK_WideCharToUTF8 (virtKeyCodeToHumanKeyName [VK_OEM_4]).c_str ());
      ImGui::BulletText   ("Press %hs to select the next texture from this list",     SK_WideCharToUTF8 (virtKeyCodeToHumanKeyName [VK_OEM_6]).c_str ());
      ImGui::EndTooltip   ();
    }

         if (io.NavInputs [ImGuiNavInput_FocusPrev] != 0.0f)
         { dir = -1; }
    else if (io.NavInputs [ImGuiNavInput_FocusNext] != 0.0f)
         { dir =  1; }
    
    else
    {
           if (ImGui::IsKeyPressed (ImGuiKey_LeftBracket, false))
           { dir = -1;  io.WantCaptureKeyboard = true; }
      else if (ImGui::IsKeyPressed (ImGuiKey_RightBracket, false))
           { dir =  1;  io.WantCaptureKeyboard = true; }
    }

    if (dir != 0)
    {
      if ((SSIZE_T)sel <  0)                     sel = 0;
      if (         sel >= list_contents.size ()) sel = list_contents.size () - 1;
      if ((SSIZE_T)sel <  0)                     sel = 0;

      while ((SSIZE_T)sel >= 0 && sel < list_contents.size ())
      {
        if ( (dir < 0 && sel == 0                        ) ||
             (dir > 0 && sel == list_contents.size () - 1)    )
        {
          break;
        }

        sel += dir;

        if (hide_inactive)
        {
          const bool active =
            ( used_textures->find (list_contents [sel].pTex) !=
              used_textures->cend (                        ) );

          if (active)
            break;
        }

        else
          break;
      }

      if ((SSIZE_T)sel <  0)                     sel = 0;
      if (         sel >= list_contents.size ()) sel = list_contents.size () - 1;
      if ((SSIZE_T)sel <  0)                     sel = 0;
    }
  }

  ImGui::SameLine     ();
  ImGui::PushStyleVar (ImGuiStyleVar_ChildRounding, 20.0f);

  last_ht    = std::max (last_ht,    16.0f);
  last_width = std::max (last_width, 16.0f);

  {
    list_entry_s& entry =
      texture_map [(uint32_t)debug_tex_id];

    D3D11_TEXTURE2D_DESC tex_desc  = entry.orig_desc;
    size_t               tex_size  = 0UL;
    float                load_time = 0.0f;

    SK_ComPtr <ID3D11Texture2D> pTex;
    if (debug_tex_id != 0x0 && texture_map.count ((uint32_t)debug_tex_id) > 0)
    {
      pTex.Attach (
        textures->getTexture2D ( (uint32_t)entry.tag,
                                                &tex_desc,
                                                &tex_size,
                                                &load_time, pTLS )
      );
    }

    if (! pTex.p)
    {
      switch (entry.tag)
      {
        case 0x6969: pTex = pHDRGamut.p;     break;
        case 0x6868: pTex = pHDRLuminance.p; break;
        default:
          break;
      }
    }

    const bool staged = false;

    if (pTex != nullptr)
    {
      // Get the REAL format, not the one the engine knows about through texture cache
      pTex->GetDesc (&tex_desc);
    }

    if (lod_list_dirty)
    {
      const UINT w = tex_desc.Width;
      const UINT h = tex_desc.Height;

      char* pszLODList = lod_list;

      for ( UINT i = 0 ; i < tex_desc.MipLevels ; i++ )
      {
        size_t len =
          sprintf ( pszLODList, "LOD%u: (%ux%u)", i,
                      std::max (1U, w >> i),
                      std::max (1U, h >> i) );

        pszLODList += (len + 1);
      }

      *pszLODList = '\0';

      lod_list_dirty = false;
    }


    SK_ComPtr <ID3D11ShaderResourceView> pSRV  = nullptr;
    D3D11_SHADER_RESOURCE_VIEW_DESC   srv_desc = {     };

    srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Format                    = tex_desc.Format;

    // Typeless compressed types need to assume a type, or they won't render :P
    switch (srv_desc.Format)
    {
      case DXGI_FORMAT_BC1_TYPELESS:
        srv_desc.Format = DXGI_FORMAT_BC1_UNORM;
        break;
      case DXGI_FORMAT_BC2_TYPELESS:
        srv_desc.Format = DXGI_FORMAT_BC2_UNORM;
        break;
      case DXGI_FORMAT_BC3_TYPELESS:
        srv_desc.Format = DXGI_FORMAT_BC3_UNORM;
        break;
      case DXGI_FORMAT_BC4_TYPELESS:
        srv_desc.Format = DXGI_FORMAT_BC4_UNORM;
        break;
      case DXGI_FORMAT_BC5_TYPELESS:
        srv_desc.Format = DXGI_FORMAT_BC5_UNORM;
        break;
      case DXGI_FORMAT_BC6H_TYPELESS:
        srv_desc.Format = DXGI_FORMAT_BC6H_SF16;
        break;
      case DXGI_FORMAT_BC7_TYPELESS:
        srv_desc.Format = DXGI_FORMAT_BC7_UNORM;
        break;

      case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        srv_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        break;
      case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;

      case DXGI_FORMAT_R8_TYPELESS:
        srv_desc.Format = DXGI_FORMAT_R8_UNORM;
        break;
      case DXGI_FORMAT_R8G8_TYPELESS:
        srv_desc.Format = DXGI_FORMAT_R8G8_UNORM;
        break;

      case DXGI_FORMAT_R16_TYPELESS:
        srv_desc.Format = DXGI_FORMAT_R16_FLOAT;
        break;
      case DXGI_FORMAT_R16G16_TYPELESS:
        srv_desc.Format = DXGI_FORMAT_R16G16_FLOAT;
        break;
      case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        srv_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        break;

      case DXGI_FORMAT_R32_TYPELESS:
        srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
        break;
      case DXGI_FORMAT_R32G32_TYPELESS:
        srv_desc.Format = DXGI_FORMAT_R32G32_FLOAT;
        break;
      case DXGI_FORMAT_R32G32B32_TYPELESS:
        srv_desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        break;
      case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        srv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        break;
    };

    srv_desc.Texture2D.MipLevels       = (UINT)-1;
    srv_desc.Texture2D.MostDetailedMip =        tex_desc.MipLevels;

    auto pDev =
      rb.getDevice <ID3D11Device> ();

    if (pDev != nullptr)
    {
#if 0
      ImVec4 border_color = config.textures.highlight_debug_tex ?
                              ImVec4 (0.3f, 0.3f, 0.3f, 1.0f) :
                                (__remap_textures && has_alternate) ? ImVec4 (0.5f,  0.5f,  0.5f, 1.0f) :
                                                                      ImVec4 (0.3f,  1.0f,  0.3f, 1.0f);
#else
      const ImVec4 border_color = entry.injected ? ImVec4 (0.3f,  0.3f,  1.0f, 1.0f) :
                                                   ImVec4 (0.3f,  1.0f,  0.3f, 1.0f);
#endif

      ImGui::PushStyleColor (ImGuiCol_Border, border_color);

      const float scale_factor = 1.0f;

      const float content_avail_y =
        ( ImGui::GetWindowContentRegionMax ().y -
          ImGui::GetWindowContentRegionMin ().y ) / scale_factor;
      const float content_avail_x =
        ( ImGui::GetWindowContentRegionMax ().x -
          ImGui::GetWindowContentRegionMin ().x ) / scale_factor;

          float effective_width  = 0.0f;
          float effective_height = 0.0f;

      const float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y   +
                                                    ImGui::GetStyle ().ItemInnerSpacing.y;

      effective_height =
        std::max (std::min ((float)(tex_desc.Height >> lod), 256.0f),
                  std::min ((float)(tex_desc.Height >> lod),
           (content_avail_y - font_size_multiline * 11.0f - 24.0f)));
      effective_width  =
        std::min (      (float)(tex_desc.Width  >> lod), (effective_height*
       (std::max (1.0f, (float)(tex_desc.Width  >> lod)) /
        std::max (1.0f, (float)(tex_desc.Height >> lod)))));

      if (effective_width > (content_avail_x - font_size * 28.0f))
      {
        effective_width   = std::max (std::min ((float)(tex_desc.Width >> lod), 256.0f),
                                                  (content_avail_x - font_size * 28.0f));
        effective_height  =  effective_width * (std::max (1.0f, (float)(tex_desc.Height >> lod))
                                              / std::max (1.0f, (float)(tex_desc.Width  >> lod)) );
      }

      ImGui::BeginGroup     ();
      ImGui::BeginChild     ( ImGui::GetID ("Texture_Select_D3D11"),
                              ImVec2 ( -1.0f, -1.0f ),
                                true,
                                  ImGuiWindowFlags_AlwaysAutoResize |
                                  ImGuiWindowFlags_NoScrollbar      |
                                  ImGuiWindowFlags_NavFlattened );

      //if ((! config.textures.highlight_debug_tex) && has_alternate)
      //{
      //  if (ImGui::IsItemHovered ())
      //    ImGui::SetTooltip ("Click me to make this texture the visible version.");
      //
      //  // Allow the user to toggle texture override by clicking the frame
      //  if (ImGui::IsItemClicked ())
      //    __remap_textures = false;
      //}

      last_width  = effective_width  + font_size * 28.0f;

      // Unsigned -> Signed so we can easily spot underflows
      if (pTex.p != nullptr)
      {
        LONG refs =
          pTex.p->AddRef  () - 1;
          pTex.p->Release ();

        ImGui::BeginGroup      (                  );
        ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (0.685f, 0.685f, 0.685f, 1.f));
        ImGui::TextUnformatted ( "Dimensions:   " );
        ImGui::PopStyleColor   (                  );
        ImGui::EndGroup        (                  );
        ImGui::SameLine        (                  );
        ImGui::BeginGroup      (                  );
        ImGui::PushItemWidth   (             -1.0f);
        ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.f, 1.f, 1.f, 1.f));
        ImGui::Combo           ("###Texture_LOD_D3D11", &lod, lod_list, tex_desc.MipLevels);
        ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (0.685f, 0.685f, 0.685f, 1.f));
        ImGui::PopItemWidth    (                  );
        ImGui::PopStyleColor   (                 2);
        ImGui::EndGroup        (                  );

        ImGui::BeginGroup      (                  );
        ImGui::TextUnformatted ( "Format:       " );
        ImGui::TextUnformatted ( "Hash:         " );
        ImGui::TextUnformatted ( "Data Size:    " );
        ImGui::TextUnformatted ( "Load Time:    " );
        ImGui::TextUnformatted ( "Cache Hits:   " );
        ImGui::TextUnformatted ( "References:   " );
        ImGui::EndGroup        (                  );
        ImGui::SameLine        (                  );
        ImGui::BeginGroup      (                  );
        ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.f, 1.f, 1.f, 1.f));
        ImGui::Text            ( "%hs",
                                   SK_DXGI_FormatToStr (tex_desc.Format).data () );
        ImGui::Text            ( "%08x", entry.crc32c);
        ImGui::Text            ( "%.3f MiB", (float)
                                   tex_size / (1024.0f * 1024.0f) );
        ImGui::Text            ( "%.3f ms",
                                   load_time );
        ImGui::Text            ( "%li", pTex != nullptr ?
                                   ReadAcquire (&textures->Textures_2D [pTex].hits)
                                                        : 0 );
        ImGui::Text            ( "%li", refs      );
        ImGui::PopStyleColor   (                  );
        ImGui::EndGroup        (                  );
        ImGui::Separator       (                  );

        static bool flip_vertical   = false;
        static bool flip_horizontal = false;

        ImGui::Checkbox ("Flip Vertically##D3D11_FlipVertical",     &flip_vertical);   ImGui::SameLine ();
        ImGui::Checkbox ("Flip Horizontally##D3D11_FlipHorizontal", &flip_horizontal);

        if (! entry.injected)
        {
          if (! SK_D3D11_IsDumped (entry.crc32c, 0x0))
          {
            if ( ImGui::Button ("  Dump Texture to Disk  ###DumpTexture") )
            {
              SK_ScopedBool decl_tex_scope (
                SK_D3D11_DeclareTexInjectScope (pTLS)
              );

              SK_D3D11_DumpTexture2D (
                pTex, entry.crc32c
              );
            }
          }

          else
          {
            if ( ImGui::Button ("  Delete Dumped Texture from Disk  ###DumpTexture") )
            {
              SK_D3D11_DeleteDumpedTexture (entry.crc32c);
            }
          }

          if (entry.mipmapped == -1)
          {   entry.mipmapped  = ( CalcMipmapLODs ( entry.desc.Width,
                                                    entry.desc.Height ) == entry.desc.MipLevels )
                               ? TRUE : FALSE;
          }

          if (entry.mipmapped == FALSE)
          {
            ImGui::SameLine ();

            bool ignore = false;

#if 0
            if (ImGui::BeginPopup ("BC7_BC6H_MipmapGen_Confirmation"))
            {
              if (ImGui::Button ("Really Generate Mipmaps for this Texture?"))
              {
                SK_ScopedBool decl_tex_scope (
                  SK_D3D11_DeclareTexInjectScope (pTLS)
                );

                if (SUCCEEDED (SK_D3D11_MipmapCacheTexture2D (pTex, entry.crc32c, pTLS)))
                {
                  entry.mipmapped = TRUE;
                  non_mipped--;
                }

                ImGui::CloseCurrentPopup ();
              }

              else
              {
                    ImGui::SameLine          (         );
                if (ImGui::Button            ("Cancel"))
                    ImGui::CloseCurrentPopup (         );
              }
            }

            if (ImGui::IsItemHovered ())
               {ImGui::BeginTooltip  ();
                ImGui::TextUnformatted (
                "This operation may take an EXTREMELY long time to complete!!"
                                       );
                ImGui::Separator  ();
                ImGui::BulletText (
                  "Mipmap generation must first Decompress BC7/BC6H textures");
                ImGui::BulletText (
                  "Once decompressed, mipmap generation is simple, but...");
                ImGui::BulletText (
                  "The mipmapped copy must be recompressed with BC7/BC6H(!!)");
                ImGui::Separator  ();
                ImGui::TextUnformatted ("\tCompressing BC7/BC6H is SLOW!");
                ImGui::EndTooltip ();}
#endif

            switch (entry.desc.Format)
            {
              // These formats take an eternity!
              case DXGI_FORMAT_BC6H_TYPELESS:
              case DXGI_FORMAT_BC6H_UF16:
              case DXGI_FORMAT_BC6H_SF16:
              case DXGI_FORMAT_BC7_TYPELESS:
              case DXGI_FORMAT_BC7_UNORM:
              case DXGI_FORMAT_BC7_UNORM_SRGB:
                ignore = true;//(! bIncludeBC7andBC6H);
              default:
                break;
            }

            //if (ignore) ImGui::BeginDisabled ();
            if (ImGui::Button ("  Generate Mipmaps  ###GenerateMipmaps"))
            {
              SK_ScopedBool decl_tex_scope (
                SK_D3D11_DeclareTexInjectScope (pTLS)
              );

              if (ignore)
              {
                SK_ComPtr <ID3D11DeviceContext> pDevCtx (rb.d3d11.immediate_ctx);
                DirectX::ScratchImage                                        captured;
                if (SUCCEEDED (DirectX::CaptureTexture (pDev, pDevCtx, pTex, captured)))
                {
                  DXGI_FORMAT uncompressed_fmt =
                    (entry.desc.Format == DXGI_FORMAT_BC7_TYPELESS)   ? DXGI_FORMAT_R8G8B8A8_UNORM :
                    (entry.desc.Format == DXGI_FORMAT_BC7_UNORM)      ? DXGI_FORMAT_R8G8B8A8_UNORM :
                    (entry.desc.Format == DXGI_FORMAT_BC7_UNORM_SRGB) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
                                                                      : DXGI_FORMAT_UNKNOWN;
                  DirectX::ScratchImage                                              converted;
                  DirectX::Decompress (*captured.GetImage (0,0,0), uncompressed_fmt, converted);

                  auto desc      = entry.desc;
                  desc.Format    = uncompressed_fmt;
                  desc.MipLevels = 1;

                  auto metadata =
                    converted.GetMetadata ();

                  metadata.mipLevels = 1;

                  SK_ComPtr <ID3D11Texture2D> pNewTex;
                  DirectX::CreateTexture (pDev, converted.GetImages (), 1, metadata, (ID3D11Resource **)&pNewTex.p);

                  if (SUCCEEDED (SK_D3D11_MipmapCacheTexture2D (pNewTex, entry.crc32c, pTLS)))
                  {
                    entry.mipmapped = TRUE;
                    non_mipped--;
                  }
                }
              }

              else if (SUCCEEDED (SK_D3D11_MipmapCacheTexture2D (pTex, entry.crc32c, pTLS)))
              {
                entry.mipmapped = TRUE;
                non_mipped--;
              }
            }

            //if (ignore)
            //{     ImGui::EndDisabled   ();
            //  if (ImGui::IsItemHovered ())
            //  {   ImGui::BeginTooltip  ();
            //      ImGui::TextUnformatted (
            //      "This operation may take an EXTREMELY long time to complete!!"
            //                             );
            //    ImGui::Separator  ();
            //    ImGui::BulletText (
            //      "Mipmap generation must first Decompress BC7/BC6H textures");
            //    ImGui::BulletText (
            //      "Once decompressed, mipmap generation is simple, but...");
            //    ImGui::BulletText (
            //      "The mipmapped copy must be recompressed with BC7/BC6H(!!)");
            //    ImGui::Separator  ();
            //    ImGui::TextUnformatted ("\tCompressing BC7/BC6H is SLOW!");
            //    ImGui::EndTooltip ();
            //  }
            //}
          }
        }

        if (staged)
        {
          ImGui::SameLine ();
          ImGui::TextColored (ImColor::HSV (0.25f, 1.0f, 1.0f), "Staged textures cannot be re-injected yet.");
        }

        if (SK_D3D11_IsTextureUncacheable (pTex))
        {
          ImGui::SameLine ();
          ImGui::TextColored (ImColor::HSV (0.05f, 1.0f, 1.0f), "Texture may not be re-injectable.");
        }

        if (entry.injected)
        {
          if ( ImGui::Button ("  Reload Texture  ###ReloadTexture") )
          {
            SK_D3D11_ReloadTexture (pTex);
          }

          ImGui::SameLine    ();
          ImGui::TextColored (ImVec4 (0.05f, 0.95f, 0.95f, 1.0f), "This texture has been injected over the original.");
        }

        if ( effective_height != (float)(tex_desc.Height >> lod) ||
             effective_width  != (float)(tex_desc.Width  >> lod) )
        {
          if (! entry.injected)
            ImGui::SameLine ();

          ImGui::TextColored (ImColor::HSV (0.5f, 1.0f, 1.0f), "Texture was rescaled to fit.");
        }

        if (! entry.injected)
          ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));
        else
          ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.05f, 0.95f, 0.95f, 1.0f));

        srv_desc.Texture2D.MipLevels       = 1;
        srv_desc.Texture2D.MostDetailedMip = lod;

        if (SUCCEEDED (pDev->CreateShaderResourceView (pTex, &srv_desc, &pSRV.p)))
        {
          const ImVec2 uv0 (flip_horizontal ? 1.0f : 0.0f, flip_vertical ? 1.0f : 0.0f);
          const ImVec2 uv1 (flip_horizontal ? 0.0f : 1.0f, flip_vertical ? 0.0f : 1.0f);

          ImGui::BeginChildFrame (ImGui::GetID ("TextureView_Frame"), ImVec2 (effective_width + 8.0f, effective_height + 8.0f),
                                  ImGuiWindowFlags_AlwaysAutoResize |
                                  ImGuiWindowFlags_NoInputs         | ImGuiWindowFlags_NoScrollbar |
                                  ImGuiWindowFlags_NoNavInputs      | ImGuiWindowFlags_NoNavFocus );

          SK_D3D11_TempResources->push_back (pSRV.p);

          ImGui::Image            ( pSRV,
                                     ImVec2 (effective_width, effective_height),
                                       uv0,                       uv1,
                                       ImColor (255,255,255,255), ImColor (255,255,255,128)
                               );
          ImGui::EndChildFrame ();

          static DWORD dwLastUnhovered = 0;

          if (ImGui::IsItemHovered ())
          {
            if (SK::ControlPanel::current_time - dwLastUnhovered > 666UL)
            {
              ImGui::BeginTooltip    ();
              ImGui::BeginGroup      ();
              ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (0.785f, 0.785f, 0.785f, 1.f));
              ImGui::TextUnformatted ("Usage:");
              ImGui::TextUnformatted ("Bind Flags:");
              if (tex_desc.MiscFlags != 0)
                ImGui::TextUnformatted("Misc Flags:");
              ImGui::PopStyleColor   ();
              ImGui::EndGroup        ();
              ImGui::SameLine        ();
              ImGui::BeginGroup      ();
              ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.0f, 1.0f, 1.0f, 1.f));
              ImGui::Text            ("%hs", SK_D3D11_DescribeUsage     (
                                               tex_desc.Usage )             );
              ImGui::Text            ("%hs", SK_D3D11_DescribeBindFlags (
                              (D3D11_BIND_FLAG)tex_desc.BindFlags).c_str () );
              if (tex_desc.MiscFlags != 0)
              {
                ImGui::Text          ("%hs", SK_D3D11_DescribeMiscFlags (
                     (D3D11_RESOURCE_MISC_FLAG)tex_desc.MiscFlags).c_str () );
              }
              ImGui::PopStyleColor   ();
              ImGui::EndGroup        ();
              ImGui::EndTooltip      ();
            }
          }

          else
            dwLastUnhovered = SK::ControlPanel::current_time;
        }
        ImGui::PopStyleColor   ();
      }

      ImGui::EndChild          ();
      ImGui::EndGroup          ();

      last_ht =
        ImGui::GetItemRectSize ().y;

      ImGui::PopStyleColor     ();
    }
  }
  ImGui::EndGroup      ( );
  ImGui::PopStyleColor (1);
  ImGui::PopStyleVar   (2);
  ImGui::PopID         ( );
}