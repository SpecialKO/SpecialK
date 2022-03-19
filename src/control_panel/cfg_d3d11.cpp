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

#include <SpecialK/nvapi.h>


#include <SpecialK/control_panel/d3d11.h>
#include <SpecialK/control_panel/osd.h>

#include <SpecialK/render/dxgi/dxgi_swapchain.h>
#include <SpecialK/render/d3d11/d3d11_core.h>

extern float __target_fps;

extern float fSwapWaitFract;
extern float fSwapWaitRatio;

extern bool SK_GL_OnD3D11;

const char*
DXGIColorSpaceToStr (DXGI_COLOR_SPACE_TYPE space) noexcept;


extern bool SK_D3D11_EnableTracking;
extern bool SK_D3D11_EnableMMIOTracking;
extern volatile
       LONG SK_D3D11_DrawTrackingReqs;
extern volatile
       LONG SK_D3D11_CBufferTrackingReqs;


using namespace SK::ControlPanel;

bool SK::ControlPanel::D3D11::show_shader_mod_dlg = false;

extern concurrency::concurrent_unordered_map <ID3D12PipelineState*, bool>
                                                   _pixelShaders;
extern concurrency::concurrent_unordered_map <ID3D12PipelineState*, std::string>
                                                   _latePSOBlobs;

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
      ImGui::Text    ( "%7zu      MiB",
                                                     SK_D3D11_Textures->AggregateSize_2D >> 20ui64 );    ImGui::NextColumn ();
       ImGui::TextColored
                     (ImVec4 (0.3f, 1.0f, 0.3f, 1.0f),
                       "%5u      Hits",              SK_D3D11_Textures->RedundantLoads_2D.load () );     ImGui::NextColumn ();
       if (SK_D3D11_Textures->Budget != 0)
         ImGui::Text ( "Budget:  %7zu MiB  ",        SK_D3D11_Textures->Budget / 1048576ui64 );
    ImGui::Columns   ( 1 );

    ImGui::Separator (   );

    ImGui::Columns   ( 3 );
      ImGui::Text    ( "%7u Textures",               SK_D3D11_Textures->Entries_2D.load () );            ImGui::NextColumn ();
      ImGui::TextColored
                     ( ImVec4 (1.0f, 0.3f, 0.3f, 1.60f),
                       "%5u   Misses",             SK_D3D11_Textures->CacheMisses_2D.load () );          ImGui::NextColumn ();
     ImGui::Text   ( "Time:        %#7.01lf ms  ", SK_D3D11_Textures->RedundantTime_2D       );
    ImGui::Columns   ( 1 );

    ImGui::Separator (   );

    ImGui::Columns   ( 3 );
      ImGui::Text    ( "%6u   Evictions",              SK_D3D11_Textures->Evicted_2D.load ()   );        ImGui::NextColumn ();
      ImGui::TextColored (ImColor::HSV (std::min ( 0.4f * (float)SK_D3D11_Textures->RedundantLoads_2D /
                                                          (float)SK_D3D11_Textures->CacheMisses_2D, 0.4f ), 0.95f, 0.8f),
                       " %.2f  Hit/Miss",                (double)SK_D3D11_Textures->RedundantLoads_2D /
                                                         (double)SK_D3D11_Textures->CacheMisses_2D  );   ImGui::NextColumn ();
      ImGui::Text    ( "Driver I/O: %7llu MiB  ",      SK_D3D11_Textures->RedundantData_2D >> 20ui64 );

    ImGui::Columns   ( 1 );

    ImGui::Separator (   );

    ImGui::PopStyleColor ( );

    float size =
      sk::narrow_cast <float> (config.textures.cache.max_size);

    ImGui::TreePush  ( "" );

    if (ImGui::SliderFloat ( "Maximum Cache Size", &size,
                               256.f, 8192.f, "%.0f MiB" ))
    {
      config.textures.cache.max_size =
        sk::narrow_cast <int> (size);

      auto cp =
        SK_GetCommandProcessor ();

      cp->ProcessCommandFormatted ( "TexCache.MaxSize %d ",
                        config.textures.cache.max_size );
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
        static SK_D3D11_TexCacheResidency_s* tex_res =
          SK_D3D11_TexCacheResidency.getPtr ();

        auto& residency_count =
          tex_res->count;
        auto& residency_size  =
          tex_res->size;

        const int fully_resident = ReadAcquire (&residency_count.InVRAM);
        const int shared_memory  = ReadAcquire (&residency_count.Shared);
        const int on_disk        = ReadAcquire (&residency_count.PagedOut);

        const LONG64 size_vram   = ReadAcquire64 (&residency_size.InVRAM);
        const LONG64 size_shared = ReadAcquire64 (&residency_size.Shared);
        const LONG64 size_disk   = ReadAcquire64 (&residency_size.PagedOut);

        ImGui::BeginGroup ();
        if (fully_resident != 0)
          ImGui::TextColored (ImColor (0.3f, 0.78f, 0.3f),   "%d Textures in VRAM\t",          fully_resident);

        if (shared_memory != 0)
          ImGui::TextColored (ImColor (0.78f, 0.78f, 0.55f), "%d Textures in Shared Memory\t", shared_memory);

        if (on_disk != 0)
          ImGui::TextColored (ImColor (0.78f, 0.3f, 0.3f),   "%d Textures Paged to Disk\t",    on_disk);
        ImGui::EndGroup ();

        ImGui::SameLine ();

        SK_TLS* pTLS =
              SK_TLS_Bottom ();

        ImGui::BeginGroup ();
        if (fully_resident != 0)
          ImGui::TextColored (ImColor (0.1f, 0.98f, 0.1f),   "\t\t%hs",
                   SK_WideCharToUTF8 (SK_File_SizeToStringF (size_vram,   2, 3, Auto, pTLS).data ()).c_str ());
        if (shared_memory != 0)
          ImGui::TextColored (ImColor (0.98f, 0.98f, 0.25f), "\t\t%hs",
                   SK_WideCharToUTF8 (SK_File_SizeToStringF (size_shared, 2, 3, Auto, pTLS).data ()).c_str ());
        if (on_disk != 0)
          ImGui::TextColored (ImColor (0.98f, 0.1f, 0.1f),   "\t\t%hs",
                   SK_WideCharToUTF8 (SK_File_SizeToStringF (size_disk,   2, 3, Auto, pTLS).data ()).c_str ());
        ImGui::EndGroup ();
      }
    }
  }
}


extern bool SK_D3D11_ShaderModDlg   (SK_TLS *pTLS = SK_TLS_Bottom ());
extern void SK_DXGI_UpdateLatencies (IDXGISwapChain *pSwapChain);


using SK_ReShade_OnDrawD3D11_pfn =
void (__stdcall *)(void*, ID3D11DeviceContext*, unsigned int);

//extern SK_RESHADE_CALLBACK_DRAW SK_ReShade_DrawCallback;

bool
SK::ControlPanel::D3D11::Draw (void)
{
  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (show_shader_mod_dlg)
      show_shader_mod_dlg = SK_D3D11_ShaderModDlg ();

  bool d3d11 =
    static_cast <int> (render_api) & static_cast <int> (SK_RenderAPI::D3D11);
  bool d3d12 =
    static_cast <int> (render_api) & static_cast <int> (SK_RenderAPI::D3D12);

  if (                                 (d3d11 &&
       ImGui::CollapsingHeader ("Direct3D 11 Settings", ImGuiTreeNodeFlags_DefaultOpen)) ||
                                       (d3d12 &&
       ImGui::CollapsingHeader ("Direct3D 12 Settings", ImGuiTreeNodeFlags_DefaultOpen)) )
  {
    if (d3d11)
    {
      ImGui::SameLine ();
      ImGui::TextUnformatted ("     State Tracking:  ");

      ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.173f, 0.428f, 0.96f));
      ImGui::SameLine ();

      if (SK_D3D11_EnableTracking)
        ImGui::TextUnformatted ("( ALL State/Ops --> [Mod Tools Window Active] )");

      else
      {
        char* szThreadLocalStr =
                        pTLS->scratch_memory->cmd.alloc (
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
    }

    else
    {
      auto currentFrame =
        SK_GetFramesDrawn ();

      #pragma region "Advanced"
      if ( //config.system.log_level > 0
           false//ImGui::TreeNode ("Recently Used Shaders")
         )
      {
        static auto constexpr _RECENT_USE_THRESHOLD = 30;
        static auto constexpr _ACTIVE_THRESHOLD     = 300;

        static constexpr GUID SKID_D3D12DisablePipelineState =
          { 0x3d5298cb, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x70 } };

        static constexpr GUID SKID_D3D12KnownVtxShaderDigest =
          { 0x4d5298ca, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x00 } };

        static constexpr GUID SKID_D3D12KnownPixShaderDigest =
          { 0x4d5298ca, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x01 } };

        static auto constexpr   DxilContainerHashSize  = 16;
        std::multimap <uint32_t, ID3D12PipelineState *> shaders;

        for ( auto &[ps, live] : _pixelShaders )
        {
          if (! live) continue;

          UINT   size      = 8UL;
          UINT64 lastFrame = 0ULL;

          ps->GetPrivateData (SKID_D3D12LastFrameUsed, &size, &lastFrame);

        //if (currentFrame > lastFrame + _RECENT_USE_THRESHOLD) continue;

                  size =  DxilContainerHashSize;
          uint8_t digest [DxilContainerHashSize];

          ps->GetPrivateData ( SKID_D3D12KnownPixShaderDigest, &size, digest );

          shaders.emplace (
            crc32c (0x0, digest, 16), ps
          );
        }

        std::string name (" ", MAX_PATH + 1);

        ImGui::BeginGroup ();
        for ( const auto &[bucket, dump] : shaders )
        {
          bool   disable = false;
          UINT   size    = sizeof (bool);

          if ( FAILED ( dump->GetPrivateData (
                          SKID_D3D12DisablePipelineState, &size, &disable
             )        )                    ) {             size=1;disable=false; }

          bool enable =
            (! disable);

          ImGui::PushID ((int)(intptr_t)dump);

          auto range =
            shaders.equal_range (bucket);

          if (ImGui::Checkbox (" Pixel::", &enable))
          {
            disable =
              (! enable);

            for ( auto it = range.first ; it != range.second ; ++it )
            {
              it->second->SetPrivateData (
                SKID_D3D12DisablePipelineState, size, &disable
              );
            }
          }
          
                  size =  DxilContainerHashSize;
          uint8_t digest [DxilContainerHashSize];

            dump->GetPrivateData ( SKID_D3D12KnownPixShaderDigest, &size, digest );

            std::string out =
              std::format ("{:x}{:x}{:x}{:x}{:x}{:x}{:x}{:x}"
                           "{:x}{:x}{:x}{:x}{:x}{:x}{:x}{:x}",
                             digest[ 0],digest[ 1],digest[ 2],digest[ 3],
                             digest[ 4],digest[ 5],digest[ 6],digest[ 7],
                             digest[ 8],digest[ 9],digest[10],digest[11],
                             digest[12],digest[13],digest[14],digest[15]);

          if (ImGui::IsItemClicked (1))
          {
            SK_LOG0 ( ( L"%hs", out.c_str () ), L"   DXGI   " );
          }

          ImGui::SameLine ();
          ImGui::Text     ("  %hs", out.c_str ());
          ImGui::PopID    ();
        }
        ImGui::EndGroup   ();
        //ImGui::SameLine   ();
        //ImGui::BeginGroup ();
        //
        //for ( auto &[ps, live] : _vertexShaders )
        //{
        //  if (! live) continue;
        //
        //  UINT   size      = 8UL;
        //  UINT64 lastFrame = 0ULL;
        //
        //  ps->GetPrivateData (SKID_D3D12LastFrameUsed, &size, &lastFrame);
        //
        //  if (currentFrame > lastFrame + _RECENT_USE_THRESHOLD) continue;
        //
        //  *name.data () = '\0';
        //
        //  if ( UINT                                               uiStrLen = MAX_PATH ;
        //FAILED ( ps->GetPrivateData ( WKPDID_D3DDebugObjectName, &uiStrLen, name.data () )
        //     ) ) { *name.data () = '\0'; }
        //  
        //  ImGui::PushID ((int)(intptr_t)ps);
        //
        //  if (ImGui::InputText (SK_FormatString ("  %08x", ps).c_str (),
        //                                                  name.data  (), MAX_PATH))
        //    SK_D3D12_SetDebugName (ps, SK_UTF8ToWideChar (name));
        //
        //  ImGui::PopID ();
        //}
        //ImGui::EndGroup ();

        ImGui::Separator ();

        ImGui::BeginGroup ();
        for ( auto &[ps, str] : _latePSOBlobs )
        {
          bool   disable   = false;
          UINT   size      = sizeof (UINT64);
          UINT64 lastFrame = 0;

          ps->GetPrivateData (SKID_D3D12LastFrameUsed, &size, &lastFrame);

          if (currentFrame > lastFrame + _ACTIVE_THRESHOLD) continue;

          if ( FAILED ( ps->GetPrivateData (
                          SKID_D3D12DisablePipelineState, &size, &disable
             )        )                    ) {             size=1;disable=false; }

          bool enable =
            (! disable);

          ImGui::PushID ((int)(intptr_t)ps);

          if (ImGui::Checkbox (" Other::", &enable))
          {
            disable =
              (! enable);

            ps->SetPrivateData (SKID_D3D12DisablePipelineState, size, &disable);
          }

          ImGui::PopID  ();
        }
        ImGui::EndGroup   ();
        ImGui::SameLine   ();
        ImGui::BeginGroup ();

        for ( auto &[ps, str] : _latePSOBlobs )
        {
          UINT   size      = sizeof (UINT64);
          UINT64 lastFrame = 0;

          ps->GetPrivateData (SKID_D3D12LastFrameUsed, &size, &lastFrame);

          if (currentFrame > lastFrame + _ACTIVE_THRESHOLD) continue;

          *name.data () = '\0';

          ImGui::PushStyleColor ( ImGuiCol_Text,
            currentFrame > lastFrame - _RECENT_USE_THRESHOLD   ?
                                 ImColor (0.5f,0.5f,0.5f,1.0f) :
                                 ImColor (1.0f,1.0f,1.0f,1.0f) );

          if ( UINT                                               uiStrLen = MAX_PATH ;
        FAILED ( ps->GetPrivateData ( WKPDID_D3DDebugObjectName, &uiStrLen, name.data () )
             ) ) { *name.data () = '\0'; }

          ImGui::PopStyleColor ();
          
          ImGui::PushID ((int)(intptr_t)ps);

          if (ImGui::InputText (str.c_str (),             name.data (), MAX_PATH))
            SK_D3D12_SetDebugName (ps, SK_UTF8ToWideChar (name));

          ImGui::PopID  ();
        }
        ImGui::EndGroup ();
        ImGui::TreePop  ();
      }
#pragma endregion
    }

    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));
    ImGui::TreePush       ("");

    const bool swapchain =
      ImGui::CollapsingHeader ("SwapChain Management");

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip   ();
      ImGui::TextColored    ( ImColor (235, 235, 235),
                              "Latency and Framepacing Tweaks" );
      ImGui::EndTooltip     ();
    }

    if (swapchain)
    {
      auto _ResetLimiter = [&](void) -> void
      {
        static auto& rb =
          SK_GetCurrentRenderBackend ();

        auto *pLimiter =
          SK::Framerate::GetLimiter (rb.swapchain.p, false);

        if (pLimiter != nullptr)
            pLimiter->reset (true);
      };

      static bool flip         = config.render.framerate.flip_discard;
      static bool waitable     = config.render.framerate.swapchain_wait > 0;
      static int  buffer_count = config.render.framerate.buffer_count;
      static int  prerender    = config.render.framerate.pre_render_limit;

      ImGui::TreePush ("");

      ImGui::BeginGroup ();

      if (! d3d12)
        ImGui::Checkbox   ("Use Flip Model Presentation", &config.render.framerate.flip_discard);
      else
        ImGui::Checkbox   ("Force Flip Discard in D3D12", &config.render.framerate.flip_discard);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("High-Performance Windowed Rendering");
        ImGui::Separator    ();
        ImGui::BulletText   ("Makes Windowed Mode Perform Same as Fullscreen Exclusive");
        ImGui::EndTooltip   ();
      }

      if (config.render.framerate.flip_discard)
      {
        bool waitable_ = config.render.framerate.swapchain_wait > 0;

        if (! d3d12)
        {
          if (ImGui::Checkbox ("Waitable SwapChain", &waitable_))
          {
            if (! waitable_) config.render.framerate.swapchain_wait = 0;
            else             config.render.framerate.swapchain_wait = 15;

            if (waitable_)
            {
              // Setup default values when first turned on
              if (config.render.framerate.pre_render_limit == -1)
              {
                config.render.framerate.pre_render_limit =
                  std::min (
                    std::max ( config.render.framerate.buffer_count + 1, 3 ),
                                                                         3 );
              }

              else
                config.render.framerate.pre_render_limit = -1;
            }

            _ResetLimiter ();
          }

          static bool magic_stuff  = false;
                      magic_stuff |= ImGui::IsItemClicked (1);

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
            ImGui::Text         ("Reduces Input Latency in SK's Framerate Limiter");
            if (SK_GetCurrentRenderBackend ().api != SK_RenderAPI::D3D12)
            {
              ImGui::Separator  ();
              ImGui::BulletText ("Fullscreen Exclusive will not work while enabled");
              ImGui::BulletText ("Fullscreen Exclusive is obsolete");
            }
            ImGui::EndTooltip   ();
          }

          if (waitable_ && magic_stuff)
          {
            ImGui::SameLine   ();
            ImGui::BeginGroup ();
            ImGui::InputFloat ("SwapWaitFract", &fSwapWaitFract);
            ImGui::InputFloat ("SwapWaitRatio", &fSwapWaitRatio);
            ImGui::EndGroup   ();
          }
        }

        if (SK_DXGI_SupportsTearing ())
        {
          bool tearing_pref = config.render.dxgi.allow_tearing;
          if (ImGui::Checkbox ("Enable DWM Tearing", &tearing_pref))
          {
            config.render.dxgi.allow_tearing = tearing_pref;

            _ResetLimiter ();
          }

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
            ImGui::Text         ("Enables True VSYNC -OFF- (PresentInterval = 0) in Windowed Mode");
            ImGui::EndTooltip   ();
          }
        }
      }

      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();

      ImGui::InputInt ("Presentation Interval",       &config.render.framerate.present_interval);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();

        ImGui::Text       ("This Controls V-Sync");
        ImGui::Separator  (                                               );
        ImGui::BulletText ("-1=Game Controlled,  0=Force Off,  1=Force On");

        if (config.render.framerate.drop_late_flips && config.render.framerate.present_interval != 0)
          ImGui::BulletText ("Values > 1 do not Apply unless \"Drop Late Frames\" or SK's Framerate Limiter are Disabled");
        else
          ImGui::BulletText (">1=Fractional Refresh Rates");

        ImGui::EndTooltip ();
      }

      config.render.framerate.present_interval =
        std::max (-1, std::min (4, config.render.framerate.present_interval));

      if (! d3d12)
      {
        if (ImGui::InputInt ("BackBuffer Count", &config.render.framerate.buffer_count))
        {
          auto& io =
            ImGui::GetIO ();

          if (config.render.framerate.buffer_count > 0)
          {
            if (config.render.framerate.flip_discard)
                config.render.framerate.buffer_count = std::min (15, std::max (2, config.render.framerate.buffer_count));
            else
                config.render.framerate.buffer_count = std::min (15, std::max (1, config.render.framerate.buffer_count));

            // Trigger a compliant game to invoke IDXGISwapChain::ResizeBuffers (...)
            PostMessage (SK_GetGameWindow (), WM_SIZE, SIZE_RESTORED, MAKELPARAM ( (LONG)io.DisplaySize.x,
                                                                                   (LONG)io.DisplaySize.y ) );

            _ResetLimiter ();
          }
        }
      }

      // Clamp to [-1,oo)
      if (config.render.framerate.buffer_count <  0)
          config.render.framerate.buffer_count = -1;

      if (! d3d12)
      {
        if (ImGui::InputInt ("Maximum Device Latency", &config.render.framerate.pre_render_limit))
        {
          if (config.render.framerate.pre_render_limit <  0)
              config.render.framerate.pre_render_limit = -1;

          else
          {
            if (config.render.framerate.buffer_count > 0)
                config.render.framerate.pre_render_limit = std::min ( config.render.framerate.pre_render_limit,
                                                                        config.render.framerate.buffer_count + 1 );

            SK_ComQIPtr <IDXGISwapChain>
                pSwapChain (SK_GetCurrentRenderBackend ().swapchain);
            if (pSwapChain != nullptr)
            {
              SK_DXGI_UpdateLatencies (pSwapChain);

              _ResetLimiter ();
            }
          }
        }
      }
      ImGui::EndGroup ();

      const bool changed =
        (flip         != config.render.framerate.flip_discard      ) ||
        (waitable     != config.render.framerate.swapchain_wait > 0) ||
        (buffer_count != config.render.framerate.buffer_count      ) ||
        (prerender    != config.render.framerate.pre_render_limit);

      if (changed)
      {
        ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (.3f, .8f, .9f));
        ImGui::BulletText     ("Game Restart Required");
        ImGui::PopStyleColor  ();
      }
      ImGui::TreePop  ();
    }

    if (d3d11 && ImGui::CollapsingHeader ("Texture Management"))
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
            for ( auto it : SK_D3D11_Textures.HashMap_2D )
            {
              ImGui::BulletText ("LOD %02lu Load Factor: ", lod++);
            }
            ImGui::EndGroup   (                                 );
            ImGui::SameLine   (                                 );
            ImGui::BeginGroup (                                 );
            for ( auto it : SK_D3D11_Textures.HashMap_2D )
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

    if (d3d11)
    {
      if ((! SK_GL_OnD3D11) && ImGui::Button (" Render Mod Tools "))
      {
        show_shader_mod_dlg = (!show_shader_mod_dlg);
      }

      if (! SK_GL_OnD3D11) ImGui::SameLine ();
      if (! SK_GL_OnD3D11) ImGui::Checkbox ("D3D11 Deferred Mode", &config.render.dxgi.deferred_isolation);

      if (! SK_GL_OnD3D11) if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Try changing this option if textures / shaders are missing from the mod tools.");
    }

    // This only works when we have wrapped SwapChains
    if ( ReadAcquire (&SK_DXGI_LiveWrappedSwapChains)  != 0 ||
         ReadAcquire (&SK_DXGI_LiveWrappedSwapChain1s) != 0 )
    {
      if (d3d11) ImGui::SameLine   ();
      OSD::DrawVideoCaptureOptions ();
    }

    if (d3d11) ImGui::SameLine ();

    const bool advanced =
      d3d11 && ImGui::TreeNode ("Advanced (Debug)###Advanced_NVD3D11");

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


extern std::string
SK_DXGI_DescribeSwapChainFlags (DXGI_SWAP_CHAIN_FLAG swap_flags, INT* pLines = nullptr);


void
SK_ImGui_SummarizeDXGISwapchain (IDXGISwapChain* pSwapDXGI)
{
  if (pSwapDXGI != nullptr)
  {
    SK_ComQIPtr <IDXGISwapChain1> pSwap1 (pSwapDXGI);

    DXGI_SWAP_CHAIN_DESC1                 swap_desc = { };
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc = { };

    if (           pSwap1.p != nullptr                     &&
        SUCCEEDED (pSwap1->GetDesc1          (&swap_desc)) &&
        SUCCEEDED (pSwap1->GetFullscreenDesc (&fullscreen_desc)))
    {
      static SK_RenderBackend& rb =
        SK_GetCurrentRenderBackend ();

      INT         swap_flag_count = 0;
      std::string swap_flags      =
        SK_DXGI_DescribeSwapChainFlags (
          static_cast <DXGI_SWAP_CHAIN_FLAG> (swap_desc.Flags),
                  &swap_flag_count     );

      ImGui::BeginTooltip      ();
      ImGui::PushStyleColor    (ImGuiCol_Text, ImVec4 (0.95f, 0.95f, 0.45f, 1.0f));
      ImGui::TextUnformatted   ("Framebuffer and Presentation Setup");
      ImGui::PopStyleColor     ();
      ImGui::Separator         ();

      ImGui::BeginGroup        ();
      ImGui::PushStyleColor    (ImGuiCol_Text, ImVec4 (0.685f, 0.685f, 0.685f, 1.0f));
      ImGui::TextUnformatted   ("Color:");
    //ImGui::TextUnformatted   ("Depth/Stencil:");
      ImGui::TextUnformatted   ("Resolution:");
      ImGui::TextUnformatted   ("Back Buffers:");
      if ((! fullscreen_desc.Windowed) && fullscreen_desc.Scaling          != DXGI_MODE_SCALING_UNSPECIFIED)
        ImGui::TextUnformatted ("Scaling Mode:");
      if ((! fullscreen_desc.Windowed) && fullscreen_desc.ScanlineOrdering != DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED)
        ImGui::TextUnformatted ("Scanlines:");
      if ((! fullscreen_desc.Windowed) && fullscreen_desc.RefreshRate.Denominator != 0)
        ImGui::TextUnformatted ("Refresh Rate:");
      ImGui::TextUnformatted   ("Swap Interval:");
      ImGui::TextUnformatted   ("Swap Effect:");
      if  (swap_desc.SampleDesc.Count > 1)
        ImGui::TextUnformatted ("MSAA Samples:");
      if (swap_desc.Flags != 0)
      {
        ImGui::TextUnformatted ("Flags:");
        if (swap_flag_count > 1) { for ( int i = 1; i < swap_flag_count; i++ ) ImGui::TextUnformatted ("\n"); }
      }
      ImGui::PopStyleColor   ();
      ImGui::EndGroup        ();

      ImGui::SameLine        ();

      ImGui::BeginGroup      ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.0f, 1.0f, 1.0f, 1.0f));
      ImGui::Text            ("%hs",                SK_DXGI_FormatToStr (swap_desc.Format).data ());
      ImGui::Text            ("%ux%u",                                   swap_desc.Width, swap_desc.Height);
      ImGui::Text            ("%lu",                                     std::max (1U, swap_desc.BufferCount));
      if ((! fullscreen_desc.Windowed) && fullscreen_desc.Scaling          != DXGI_MODE_SCALING_UNSPECIFIED)
        ImGui::Text          ("%hs",        SK_DXGI_DescribeScalingMode (fullscreen_desc.Scaling));
      if ((! fullscreen_desc.Windowed) && fullscreen_desc.ScanlineOrdering != DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED)
        ImGui::Text          ("%hs",      SK_DXGI_DescribeScanlineOrder (fullscreen_desc.ScanlineOrdering));
      if ((! fullscreen_desc.Windowed) && fullscreen_desc.RefreshRate.Denominator != 0)
        ImGui::Text          ("%.2f Hz",                                 static_cast <float> (fullscreen_desc.RefreshRate.Numerator) /
                                                                         static_cast <float> (fullscreen_desc.RefreshRate.Denominator));
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
        ImGui::Text          ("%u: UNKNOWN or Invalid",                  0);
      ImGui::Text            ("%hs",            SK_DXGI_DescribeSwapEffect (swap_desc.SwapEffect));
      if  (swap_desc.SampleDesc.Count > 1)
        ImGui::Text          ("%u",                                         swap_desc.SampleDesc.Count);
      if (swap_desc.Flags != 0)
      {
        ImGui::Text          ("%hs",                                        swap_flags.c_str ());
      }
      ImGui::PopStyleColor   ();
      ImGui::EndGroup        ();

      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (0.95f, 0.95f, 0.45f, 1.0f));
      ImGui::TextUnformatted ("Display Output Configuration");
      ImGui::PopStyleColor   ();
      ImGui::Separator       ();

      ImGui::BeginGroup      ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (0.685f, 0.685f, 0.685f, 1.0f));
      //if (rb.isHDRCapable ())
      {
        ImGui::Text ("Display Device: ");
        ImGui::Text ("HDR Color Space: ");
        ImGui::Text ("Output Bit Depth: ");
      }
      ImGui::PopStyleColor   ();
      ImGui::EndGroup        ();

      ImGui::SameLine        ();

      ImGui::BeginGroup      ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.0f, 1.0f, 1.0f, 1.0f));
      //if (rb.isHDRCapable ())
      {
        bool _fullscreen = true;

        SK_ComQIPtr <IDXGISwapChain4> pSwap3 (pSwapDXGI);

        if (pSwap3 != nullptr)
        {
          DXGI_SWAP_CHAIN_FULLSCREEN_DESC full_desc = { };
              pSwap3->GetFullscreenDesc (&full_desc);

          _fullscreen =
            (! full_desc.Windowed);
        }

        ImGui::Text ("%ws", rb.display_name);

        if (! rb.scanout.nvapi_hdr.active)
        {
          if (_fullscreen)
            ImGui::Text ("%hs",                DXGIColorSpaceToStr ((DXGI_COLOR_SPACE_TYPE)rb.scanout.dxgi_colorspace));
          else
            ImGui::Text ("%hs (DWM Assigned)", DXGIColorSpaceToStr ((DXGI_COLOR_SPACE_TYPE)rb.scanout.dwm_colorspace));
          ImGui::Text   ("%d", rb.scanout.bpc);
        }

        else
        {
          ImGui::Text ("%hs (%s)", HDRModeToStr (rb.scanout.nvapi_hdr.mode),
                                                 rb.scanout.nvapi_hdr.getFormatStr ());
          ImGui::Text ("%hs",                    rb.scanout.nvapi_hdr.getBpcStr    ());
        }
      }
      ImGui::PopStyleColor   ();
      ImGui::EndGroup        ();
      ImGui::EndTooltip      ();
    }
  }
}

void
SK::ControlPanel::D3D11::TextureMenu (SK_TLS *pTLS)
{
  if (ImGui::BeginMenu ("Browse Texture Assets"))
  {
    wchar_t wszPath [MAX_PATH + 2] = { };

    if (pTLS == nullptr)
        pTLS =
      SK_TLS_Bottom ();

    static char injectable [MAX_PATH] = {};
    static char dumped     [MAX_PATH] = {};

    std::string_view inj_view (injectable, MAX_PATH),
                    dump_view (    dumped, MAX_PATH);

    SK_FormatStringView ( inj_view, "%ws",
                            SK_File_SizeToString (
                              SK_D3D11_Textures->injectable_texture_bytes, Auto, pTLS
                                                 ).data () );

    if ( ImGui::MenuItem ( "Injectable Textures", inj_view.data (), nullptr ) )
    {
      wcscpy      (wszPath, SK_D3D11_res_root->c_str ());
      PathAppendW (wszPath, LR"(inject\textures)");

      SK_ShellExecuteW (nullptr, L"explore", wszPath, nullptr, nullptr, SW_NORMAL);
    }

    if (! SK_D3D11_Textures->dumped_textures.empty ())
    {
       SK_FormatStringView ( dump_view, "%ws",
                               SK_File_SizeToString (
                                 SK_D3D11_Textures->dumped_texture_bytes, Auto, pTLS
                                                    ).data () );
       if ( ImGui::MenuItem ( "Dumped Textures", dump_view.data (), nullptr ) )
       {
         wcscpy      (wszPath, SK_D3D11_res_root->c_str ());
         PathAppendW (wszPath, LR"(dump\textures)");
         PathAppendW (wszPath, SK_GetHostApp ());

         SK_ShellExecuteW (nullptr, L"explore", wszPath, nullptr, nullptr, SW_NORMAL);
       }
     }

    ImGui::EndMenu ();
  }
}