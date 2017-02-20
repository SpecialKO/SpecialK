// ImGui - standalone example application for DirectX 9
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include <imgui/imgui.h>
#include <imgui/backends/imgui_d3d9.h>
#include <imgui/backends/imgui_d3d11.h>
#include <d3d9.h>

#include <SpecialK/config.h>
#include <SpecialK/DLL_VERSION.H>
#include <SpecialK/command.h>
#include <SpecialK/console.h>

#include <SpecialK/render_backend.h>


#include <windows.h>
#include <cstdio>
#include "resource.h"

void
LoadFileInResource ( int          name,
                     int          type,
                     DWORD&       size,
                     const char*& data )
{
  HMODULE handle =
    GetModuleHandle (nullptr);

  HRSRC rc =
   FindResource ( handle,
                    MAKEINTRESOURCE (name),
                    MAKEINTRESOURCE (type) );

  HGLOBAL rcData = 
    LoadResource (handle, rc);

  size =
    SizeofResource (handle, rc);

  data =
    static_cast <const char *>(
      LockResource (rcData)
    );
}

extern void SK_Steam_SetNotifyCorner (void);


extern bool SK_ImGui_Visible;

extern void
__stdcall
SK_SteamAPI_SetOverlayState (bool active);

extern bool
__stdcall
SK_SteamAPI_GetOverlayState (bool real);

#include <unordered_set>
#include <d3d11.h>

// Actually more of a cache manager at the moment...
class SK_D3D11_TexMgr {
public:
  SK_D3D11_TexMgr (void) {
    QueryPerformanceFrequency (&PerfFreq);
  }

  bool             isTexture2D  (uint32_t crc32);

  ID3D11Texture2D* getTexture2D ( uint32_t              crc32,
                            const D3D11_TEXTURE2D_DESC *pDesc,
                                  size_t               *pMemSize   = nullptr,
                                  float                *pTimeSaved = nullptr );

  void             refTexture2D ( ID3D11Texture2D      *pTex,
                            const D3D11_TEXTURE2D_DESC *pDesc,
                                  uint32_t              crc32,
                                  size_t                mem_size,
                                  uint64_t              load_time );

  void             reset         (void);
  bool             purgeTextures (size_t size_to_free, int* pCount, size_t* pFreed);

  struct tex2D_descriptor_s {
    ID3D11Texture2D      *texture    = nullptr;
    D3D11_TEXTURE2D_DESC  desc       = { 0 };
    size_t                mem_size   = 0L;
    uint64_t              load_time  = 0ULL;
    uint32_t              crc32      = 0x00;
    uint32_t              hits       = 0;
    uint64_t              last_used  = 0ULL;
  };

  std::unordered_set <ID3D11Texture2D *>      TexRefs_2D;

  std::unordered_map < uint32_t,
                       ID3D11Texture2D *  >   HashMap_2D;
  std::unordered_map < ID3D11Texture2D *,
                       tex2D_descriptor_s  >  Textures_2D;

  size_t                                      AggregateSize_2D  = 0ULL;
  size_t                                      RedundantData_2D  = 0ULL;
  uint32_t                                    RedundantLoads_2D = 0UL;
  uint32_t                                    Evicted_2D        = 0UL;
  float                                       RedundantTime_2D  = 0.0f;
  LARGE_INTEGER                               PerfFreq;
} extern SK_D3D11_Textures;


#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

__declspec (dllexport)
bool
SK_ImGui_ControlPanel (void)
{
  ImGuiIO& io =
    ImGui::GetIO ();

  //
  // Framerate history
  //
  static float values [120]  = { 0 };
  static int   values_offset =   0;

  values [values_offset] = 1000.0f * ImGui::GetIO ().DeltaTime;
  values_offset = (values_offset + 1) % IM_ARRAYSIZE (values);


  static float last_width  = -1;
  static float last_height = -1;

  if (last_width != io.DisplaySize.x || last_height != io.DisplaySize.y) {
    ImGui::SetNextWindowPosCenter       (ImGuiSetCond_Always);
    last_width = io.DisplaySize.x; last_height = io.DisplaySize.y;
  }


  ImGui::SetNextWindowSizeConstraints (ImVec2 (250, 50), ImVec2 ( 0.9 * io.DisplaySize.x,
                                                                  0.9 * io.DisplaySize.y ) );

  const char* szTitle = "Special K (v " SK_VERSION_STR_A ") Control Panel";
  bool        open    = true;


  ImGui::Begin (szTitle, &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders);
               char szName [32] = { '\0' };
    snprintf ( szName, 32, "%ws    "
#ifndef _WIN64
                                   "[ 32-bit ]",
#else
                                   "[ 64-bit ]",
#endif
                             SK_GetCurrentRenderBackend ().name );

    ImGui::MenuItem ("Active Render API        ", szName);

    char szResolution [64] = { '\0' };
    snprintf ( szResolution, 63, "   %lux%lu", 
                                   (UINT)io.DisplayFramebufferScale.x, (UINT)io.DisplayFramebufferScale.y );

    ImGui::MenuItem (" Framebuffer Resolution", szResolution);
    
    RECT client;
    GetClientRect ((HWND)io.ImeWindowHandle, &client);
    snprintf ( szResolution, 63, "   %lux%lu", 
                                   client.right - client.left,
                                     client.bottom - client.top );

    ImGui::MenuItem (" Window Resolution     ", szResolution);

    ImGui::Columns   ( 1 );
    ImGui::Separator (   );

    static HMODULE hModTZFix = GetModuleHandle (L"tzfix.dll");
    static HMODULE hModTBFix = GetModuleHandle (L"tbfix.dll");
    static bool has_own_scale = (hModTZFix || hModTBFix);

    if ((! has_own_scale) && ImGui::CollapsingHeader ("UI Scale"))
    {
      ImGui::TreePush    ("");

      if (ImGui::SliderFloat ("Scale (only 1.0 is officially supported)", &config.imgui.scale, 1.0f, 3.0f))
        ImGui::GetIO ().FontGlobalScale = config.imgui.scale;

      ImGui::TreePop     ();
    }

    const  float font_size           =              ImGui::GetFont ()->FontSize                        * io.FontGlobalScale;
    const  float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

    static bool has_own_limiter = (hModTZFix || hModTBFix);


    if (! has_own_limiter)
    {
      ImGui::PushItemWidth (ImGui::GetWindowWidth () * 0.666f);

      if ( ImGui::CollapsingHeader ("Framerate Limiter", ImGuiTreeNodeFlags_CollapsingHeader | 
                                                         ImGuiTreeNodeFlags_DefaultOpen ) )
      {
#if 0
        if (reset_frame_history) {
          const float fZero = 0.0f;
          memset (values, *(reinterpret_cast <const DWORD *> (&fZero)), sizeof (float) * 120);
    
          values_offset       = 0;
          reset_frame_history = false;
          was_reset           = true;
        }
#endif
  
        float sum = 0.0f;
      
        float min = FLT_MAX;
        float max = 0.0f;
      
        for (float val : values) {
          sum += val;
      
          if (val > max)
            max = val;
      
          if (val < min)
            min = val;
        }
      
        static char szAvg [512];
      
        float target_frametime = ( config.render.framerate.target_fps == 0.0f ) ?
                                    ( 1000.0f / 60.0f ) :
                                      ( 1000.0f / config.render.framerate.target_fps );
      
        sprintf_s
              ( szAvg,
                  512,
                    "Avg milliseconds per-frame: %6.3f  (Target: %6.3f)\n"
                    "    Extreme frametimes:      %6.3f min, %6.3f max\n\n\n\n"
                    "Variation:  %8.5f ms  ==>  %.1f FPS  +/-  %3.1f frames",
                      sum / 120.0f, target_frametime,
                        min, max, max - min,
                          1000.0f / (sum / 120.0f), (max - min) / (1000.0f / (sum / 120.0f)) );
      
        ImGui::PlotLines ( "",
                             values,
                               IM_ARRAYSIZE (values),
                                 values_offset,
                                   szAvg,
                                     0.0f,
                                       2.0f * target_frametime,
                                         ImVec2 (
                                           std::max (500.0f, ImGui::GetContentRegionAvailWidth ()), font_size * 7) );
      
#if 0
        bool changed = ImGui::SliderFloat ( "Special K Framerate Tolerance", &config.framerate.tolerance, 0.005f, 0.5);
        
        if (ImGui::IsItemHovered ()) {
          ImGui::BeginTooltip ();
          ImGui::PushStyleColor (ImGuiCol_Text, ImColor (0.95f, 0.75f, 0.25f, 1.0f));
          ImGui::Text           ("Controls Framerate Smoothness\n\n");
          ImGui::PushStyleColor (ImGuiCol_Text, ImColor (0.75f, 0.75f, 0.75f, 1.0f));
          ImGui::Text           ("  Lower = Smoother, but setting ");
          ImGui::SameLine       ();
          ImGui::PushStyleColor (ImGuiCol_Text, ImColor (0.95f, 1.0f, 0.65f, 1.0f));
          ImGui::Text           ("too low");
          ImGui::SameLine       ();
          ImGui::PushStyleColor (ImGuiCol_Text, ImColor(0.75f, 0.75f, 0.75f, 1.0f));
          ImGui::Text           (" will cause framerate instability...");
          ImGui::PopStyleColor  (4);
          ImGui::EndTooltip     ();
        }
        
        if (changed) {
          //tbf::FrameRateFix::DisengageLimiter ();
          SK_GetCommandProcessor ()->ProcessCommandFormatted ( "LimiterTolerance %f", config.framerate.tolerance );
          //tbf::FrameRateFix::BlipFramerate    ();
        }
#endif
      }

      ImGui::PopItemWidth ();
    }

    SK_RenderAPI api = SK_GetCurrentRenderBackend ().api;

    if ( api == SK_RenderAPI::D3D11 &&
         ImGui::CollapsingHeader ("Direct3D 11 Settings" ) )
    {
      if (ImGui::TreeNode("Texture Management"))
      {
        ImGui::Checkbox ("Enable Texture Caching", &config.textures.d3d11.cache); 

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Reduce driver memory management overhead in games that stream textures.");

        ImGui::SameLine (); ImGui::BulletText ("Requires restart");


        if (config.textures.d3d11.cache) {
          ImGui::Checkbox ("Ignore textures with no mipmaps", &config.textures.cache.ignore_nonmipped);

          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ("Important compatibility setting for some games (e.g. The Witcher 3) that improperly modify immutable textures.");
        }

        ImGui::TreePop  ();

        if (config.textures.d3d11.cache) {
          ImGui::Separator (   );
          ImGui::Columns   ( 3 );
            ImGui::Text    ( "Size" );                                                                     ImGui::NextColumn ();
            ImGui::Text    ( "Activity" );                                                                 ImGui::NextColumn ();
            ImGui::Text    ( "Savings" );
          ImGui::Columns   ( 1 );

          ImGui::Separator (   );

          ImGui::Columns   ( 3 );
            ImGui::Text    ( "%#7zu      MiB",
                                                           SK_D3D11_Textures.AggregateSize_2D >> 20ULL );  ImGui::NextColumn (); 
             ImGui::Text   ("%#5lu      Hits",             SK_D3D11_Textures.RedundantLoads_2D         );  ImGui::NextColumn ();
             ImGui::Text   ( "Time:        %#7.01lf ms  ", SK_D3D11_Textures.RedundantTime_2D          );
          ImGui::Columns   ( 1 );

          ImGui::Separator (   );

          ImGui::Columns   ( 3 );
            ImGui::Text    ( "%#7zu Textures",             SK_D3D11_Textures.TexRefs_2D.size () );        ImGui::NextColumn ();
            ImGui::Text    ( "%#5lu Evictions",            SK_D3D11_Textures.Evicted_2D         );        ImGui::NextColumn ();
            ImGui::Text    ( "Driver I/O: %#7zu MiB  ",    SK_D3D11_Textures.RedundantData_2D >> 20ULL );
          ImGui::Columns   ( 1 );

          ImGui::Separator (   );
        }
      }

      static bool enable_resolution_limits = ! ( config.render.dxgi.res.min.isZero () && 
                                                 config.render.dxgi.res.max.isZero () );

      bool res_limits =
        ImGui::TreeNodeEx ("Resolution Limiting", enable_resolution_limits ? ImGuiTreeNodeFlags_DefaultOpen : 0x00);

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Restrict the lowest/highest available resolution a game knows about; this may be useful for games that compute aspect ratio based on the highest reported resolution.");

      if (res_limits) {
        ImGui::InputInt2 ("Minimum Resolution", (int *)&config.render.dxgi.res.min.x);
        ImGui::InputInt2 ("Maximum Resolution", (int *)&config.render.dxgi.res.max.x);
        ImGui::TreePop   ();
       }
    }

    //ImGui::CollapsingHeader ("On-Screen Display");
    //ImGui::CollapsingHeader ("Render Features");

    if ( ImGui::CollapsingHeader ("Compatibility Settings") )
    {
      if (ImGui::TreeNodeEx ("Render API Hooks", ImGuiTreeNodeFlags_DefaultOpen))
      {
        auto EnableActiveAPI = [](SK_RenderAPI api) ->
        void
        {
          switch (api)
          {
            case SK_RenderAPI::D3D9Ex:
              config.apis.d3d9ex.hook     = true;
            case SK_RenderAPI::D3D9:
              config.apis.d3d9.hook       = true;
              break;

            case SK_RenderAPI::D3D12:
              config.apis.dxgi.d3d12.hook = true;
            case SK_RenderAPI::D3D11:
              config.apis.dxgi.d3d11.hook = true;
              break;

            case SK_RenderAPI::OpenGL:
              config.apis.OpenGL.hook     = true;
              break;

            case SK_RenderAPI::Vulkan:
              config.apis.Vulkan.hook     = true;
              break;
          }
        };

        ImGui::TreePop    (  );
        ImGui::PushStyleVar                                                           (ImGuiStyleVar_ChildWindowRounding, 10.0f);
        ImGui::BeginChild ("", ImVec2 (font_size * 39, font_size_multiline * 4), true, ImGuiWindowFlags_ChildWindowAutoFitY);

        ImGui::Columns    ( 2 );

        ImGui::Checkbox ("Direct3D 9", &config.apis.d3d9.hook);

        if (config.apis.d3d9.hook)
          ImGui::Checkbox ("Direct3D 9Ex", &config.apis.d3d9ex.hook);
        else {
          ImGui::TextDisabled ("   Direct3D 9Ex");
          config.apis.d3d9ex.hook = false;
        }

        ImGui::NextColumn (  );

        ImGui::Checkbox ("Direct3D 11", &config.apis.dxgi.d3d11.hook);

        if (config.apis.dxgi.d3d11.hook)
          ImGui::Checkbox ("Direct3D 12", &config.apis.dxgi.d3d12.hook);
        else {
          ImGui::TextDisabled ("   Direct3D 12");
          config.apis.dxgi.d3d12.hook = false;
        }

        ImGui::Columns    ( 1 );
        ImGui::Separator  (   );
        ImGui::Columns    ( 2 );
                          
        ImGui::Checkbox   ("OpenGL ", &config.apis.OpenGL.hook); ImGui::SameLine ();
        ImGui::Checkbox   ("Vulkan ", &config.apis.Vulkan.hook);

        ImGui::NextColumn (  );

        if (ImGui::Button (" Disable All But the Active API "))
        {
          config.apis.d3d9ex.hook     = false; config.apis.d3d9.hook       = false;
          config.apis.dxgi.d3d11.hook = false; config.apis.dxgi.d3d12.hook = false;
          config.apis.OpenGL.hook     = false; config.apis.Vulkan.hook     = false;
        }

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Application start time and negative interactions with third-party software can be reduced by turning off APIs that are not needed...");

        ImGui::Columns    ( 1 );

        ImGui::EndChild   (  );
        ImGui::PopStyleVar ( );

        EnableActiveAPI   (api);
      }

      if (ImGui::TreeNode("Hardware Monitoring APIs"))
      {
        ImGui::Checkbox ("NvAPI  ", &config.apis.NvAPI.enable);
        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("NVIDIA's hardware monitoring API, needed for the GPU stats on the OSD. Turn off only if your driver is buggy.");

        ImGui::SameLine ();
        ImGui::Checkbox ("ADL   ",   &config.apis.ADL.enable);
        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("AMD's hardware monitoring API, needed for the GPU stats on the OSD. Turn off only if your driver is buggy.");
        ImGui::TreePop();
      }
    }

    if ( ImGui::CollapsingHeader ("Input Management") )
    {
      if (ImGui::TreeNode ("Mouse Cursor"))
      {
        ImGui::Checkbox ( "Auto-Hide When Not Moved", &config.input.cursor.manage        );

        ImGui::Checkbox ( "Keyboard Input Activates Cursor",
                                       &config.input.cursor.keys_activate );

        float seconds = 
          (float)config.input.cursor.timeout  / 1000.0f;

        if ( ImGui::SliderFloat ( "Seconds Before Hiding",
                                    &seconds, 0.0f, 30.0f ) )
        {
          config.input.cursor.timeout = static_cast <LONG> (( seconds * 1000.0f ));
        }

        ImGui::TreePop  ();
      }

      ImGui::Checkbox ("Block Input to Game While UI is Open", &config.input.ui.capture);

      if (ImGui::IsItemHovered ()) {
        ImGui::BeginTooltip ();
          ImGui::Text       ("Special K tries to be cooperative with games and only captures input if the UI has focus...");
          ImGui::BulletText ("If mouselook is causing problems, use this setting to force input capture unconditionally.");
        ImGui::EndTooltip   ();
      }
    }

    if ( ImGui::CollapsingHeader ("Window Management") )
    {
      if (ImGui::TreeNode ("Overrides"))
      {
        bool borderless        = config.window.borderless;
        bool center            = config.window.center;
        bool fullscreen        = config.window.fullscreen;
        bool confine           = config.window.confine_cursor;
        bool background_render = config.window.background_render;
        bool background_mute   = config.window.background_mute;

        //
        // If we did this from the render thread, we would deadlock most games
        //
        auto DeferCommand = [=] (const char* szCommand) ->
          void
            {
              CreateThread ( nullptr,
                               0x00,
                            [ ](LPVOID user) ->
                DWORD
                  {
                    SK_GetCommandProcessor ()->ProcessCommandLine (
                       (const char*)user
                    );

                    CloseHandle (GetCurrentThread ());

                    return 0;
                  },

                (LPVOID)szCommand,
              0x00,
            nullptr
          );
        };

        if ( ImGui::Checkbox ( "Borderless", &borderless ) )
          DeferCommand ("Window.Borderless toggle");

        if (borderless) {
          if ( ImGui::Checkbox ( "Fullscreen (Borderless)", &fullscreen ) )
            DeferCommand ("Window.Fullscreen toggle");
        }

        if ( ImGui::Checkbox ( "Center", &center ) )
          DeferCommand ("Window.Center toggle");

        if ( ImGui::Checkbox ( "Mute Game in Background", &background_mute ) )
          DeferCommand ("Window.BackgroundMute toggle");

        if ( ImGui::Checkbox ( "Continue Rendering in Background", &background_render ) )
          DeferCommand ("Window.BackgroundRender toggle");

        if ( ImGui::Checkbox ( "Restrict Cursor to Window", &confine ) )
          DeferCommand ("Window.ConfineCursor toggle");

        ImGui::TreePop  ();
      }
    }

    if ( ImGui::CollapsingHeader ("Steam Enhancements", ImGuiTreeNodeFlags_CollapsingHeader | 
                                                        ImGuiTreeNodeFlags_DefaultOpen ) )
    {
      if (ImGui::TreeNode ("Achievements"))
      {
        ImGui::Checkbox ("Play Sound on Unlock ", &config.steam.achievements.play_sound);

        if (config.steam.achievements.play_sound) {
          ImGui::SameLine ();
          
          int i = 0;
          
          if (! _wcsicmp (config.steam.achievements.sound_file.c_str (), L"xbox"))
            i = 1;
          else
            i = 0;
          
          if (ImGui::Combo ("", &i, "PlayStation Network\0Xbox Live\0\0", 2))
          {
            if (i == 0) config.steam.achievements.sound_file = L"psn";
                   else config.steam.achievements.sound_file = L"xbox";
          
            extern void
            SK_SteamAPI_LoadUnlockSound (const wchar_t* wszUnlockSound);
          
            SK_SteamAPI_LoadUnlockSound (config.steam.achievements.sound_file.c_str ());
          }
        }

        ImGui::Checkbox ("Take Screenshot on Unlock", &config.steam.achievements.take_screenshot);

        if (ImGui::TreeNode  ("Enhanced Popup"))
        {
          int mode = (config.steam.achievements.popup.show + config.steam.achievements.popup.animate);

          if ( ImGui::Combo     ( "Draw Mode",
                                        &mode,
                                          "Disabled\0"
                                          "Stationary\0"
                                          "Animated\0\0" ) ) {
            config.steam.achievements.popup.show    = (mode > 0);
            config.steam.achievements.popup.animate = (mode > 1);

            // Make sure the duration gets set non-zero when this changes
            if (config.steam.achievements.popup.show)
            {
              if ( config.steam.achievements.popup.duration == 0 )
                config.steam.achievements.popup.duration = 6666UL;
            }
          }

          float duration = std::max ( 1.0f, ( (float)config.steam.achievements.popup.duration / 1000.0f ) );

          if ( ImGui::SliderFloat ( "Duration (seconds)",            &duration, 1.0f, 30.0f ) )
          {
            config.steam.achievements.popup.duration = static_cast <LONG> ( duration * 1000.0f );
          }

          ImGui::Combo     ( "Location",                      &config.steam.achievements.popup.origin,
                                      "Top-Left\0"
                                      "Top-Right\0"
                                      "Bottom-Left\0"
                                      "Bottom-Right\0\0" );
          
          //ImGui::SliderFloat ("Inset Percentage",    &config.steam.achievements.popup.inset, 0.0f, 1.0f, "%.3f%%", 0.01f);
          ImGui::TreePop   ();
        }

        extern void
        SK_UnlockSteamAchievement (uint32_t idx);

        if (ImGui::Button ("Test Unlock"))
          SK_UnlockSteamAchievement (0);

        ImGui::TreePop     ();
      }

      if (ImGui::TreeNode ("Steam Compatibility Modes"))
      {
        ImGui::Checkbox ("Disable User Stats Receipt Callback", &config.steam.block_stat_callback);

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("For games that freak out if they are flooded with achievement information, turn this on\n\n"
                             "You can tell if a game has this problem by a stuck SteamAPI Frame counter.");
        ImGui::TreePop     ();
      }

      if (ImGui::TreeNode  ("Steam Overlay Notifications"))
      {
        if (ImGui::Combo ( " ", &config.steam.notify_corner,
                                  "Top-Left\0"
                                  "Top-Right\0"
                                  "Bottom-Left\0"
                                  "Bottom-Right\0"
                                  "(Let Game Decide)\0\0" ))
        {
          SK_Steam_SetNotifyCorner ();
        }

        ImGui::TreePop ();
      }

      bool valid = (! config.steam.silent);
      if (valid)
        ImGui::MenuItem ("I am not a pirate", "", &valid, false);
      else
        ImGui::MenuItem ("I am a filthy pirate!");
    }

    //ImGui::CollapsingHeader ("Window Management");
    //if (ImGui::CollapsingHeader ("Software Updates")) {
      //config.
    //}

    ImGui::Columns   ( 1 );
    ImGui::Separator (   );

    extern volatile LONGLONG SK_SteamAPI_CallbackRunCount;

    ImGui::Bullet     ();   ImGui::SameLine ();

    bool pause =
      SK_SteamAPI_GetOverlayState (false);

    if (ImGui::Selectable ( "Current SteamAPI Frame", &pause ))
      SK_SteamAPI_SetOverlayState (pause);

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip   (       );
      ImGui::Text           ( "In " );                   ImGui::SameLine ();
      ImGui::PushStyleColor ( ImGuiCol_Text, ImColor (0.95f, 0.75f, 0.25f, 1.0f) ); 
      ImGui::Text           ( " Steam Overlay Aware ");  ImGui::SameLine ();
      ImGui::PopStyleColor  (       );
      ImGui::Text           ( "software, this will trigger the game's overlay pause mode." );
      ImGui::EndTooltip     (       );
    }

    ImGui::SameLine ();
    ImGui::Text     ( ": %10llu", InterlockedAdd64 (&SK_SteamAPI_CallbackRunCount, 0) );
  ImGui::End   ();

  return open;
}

#include <string>
#include <vector>

#include <SpecialK/render_backend.h>

__declspec (dllexport)
void
SK_ImGui_Toggle (void);

//
// Hook this to override Special K's GUI
//
__declspec (dllexport)
DWORD
SK_ImGui_DrawFrame ( _Unreferenced_parameter_ DWORD  dwFlags, 
                                              LPVOID lpUser )
{
  UNREFERENCED_PARAMETER (dwFlags);
  UNREFERENCED_PARAMETER (lpUser);

  bool d3d9  = false;
  bool d3d11 = false;

  if ((int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::D3D9) {
    d3d9 = true;

    ImGui_ImplDX9_NewFrame ();
  }

  else if (SK_GetCurrentRenderBackend ().api == SK_RenderAPI::D3D11) {
    d3d11 = true;

    ImGui_ImplDX11_NewFrame ();
  }

  else
    return 0x00;

  // Default action is to draw the Special K Control panel,
  //   but if you hook this function you can do anything you
  //     want...
  //
  //    To use the Special K Control Panel from a plug-in,
  //      import SK_ImGui_ControlPanel and call that somewhere
  //        from your hook.
  bool keep_open = SK_ImGui_ControlPanel ();

  if (d3d9) {
    extern LPDIRECT3DDEVICE9 g_pd3dDevice;

    if ( SUCCEEDED (
           g_pd3dDevice->BeginScene ()
         )
       )
    {
      ImGui::Render          ();
      g_pd3dDevice->EndScene ();
    }
  }

  else if (d3d11) {
    ImGui::Render ();
  }

  if (! keep_open)
    SK_ImGui_Toggle ();

  return 0;
}

__declspec (dllexport)
void
SK_ImGui_Toggle (void)
{
  bool d3d11 = false;
  bool d3d9  = false;

  if ((int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::D3D9)  d3d9  = true;
  if (     SK_GetCurrentRenderBackend ().api ==     SK_RenderAPI::D3D11) d3d11 = true;

  if (d3d9 || d3d11)
  {    
    static HMODULE hModTBFix = GetModuleHandle (L"tbfix.dll");

    if (hModTBFix == nullptr) 
    {
      static int cursor_refs = 0;

      if (SK_ImGui_Visible)
      {
        while (ShowCursor (FALSE) > cursor_refs)
          ;
        while (ShowCursor (TRUE)  < cursor_refs)
          ;
      }

      else
      {
        cursor_refs = (ShowCursor (FALSE) + 1);
    
        if (cursor_refs > 0) {
          while (ShowCursor (FALSE) > 0)
            ;
        }
      }
    }
    
    SK_ImGui_Visible = (! SK_ImGui_Visible);
    
    if (SK_ImGui_Visible)
      SK_Console::getInstance ()->visible = false;
    
    
    extern const wchar_t*
    __stdcall
    SK_GetBackend (void);
    
    const wchar_t* config_name = SK_GetBackend ();
    
    extern bool
    __stdcall
    SK_IsInjected (bool set = false);
    
    if (SK_IsInjected ())
      config_name = L"SpecialK";
    
    SK_SaveConfig (config_name);

    // Immediately stop capturing keyboard/mouse events,
    //   this is the only way to preserve cursor visibility
    //     in some games (i.e. Tales of Berseria)
    if (SK_ImGui_Visible)
    {
      ImGui::GetIO ().WantCaptureKeyboard = false;
      ImGui::GetIO ().WantCaptureMouse    = false;
    }
  }
}