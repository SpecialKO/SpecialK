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
#ifndef __SK__CONFIG_H__
#define __SK__CONFIG_H__

#include <Windows.h>
#include <string>

#include <SpecialK/render_backend.h>

extern const wchar_t* SK_VER_STR;

const wchar_t*
__stdcall
SK_GetVersionStr (void);

struct sk_config_t
{
  struct {
    bool   show           = true;
    LONG   format         = LOCALE_USER_DEFAULT;

    struct {
      BYTE toggle [4]     = { VK_CONTROL, VK_SHIFT, 'T', 0 };
    } keys;
  } time;

  struct {
    bool   show           = false;
    float  reserve        = 75.0f; // 75%
    float  interval       = 0.25f;

    struct {
      BYTE toggle [4]     = { VK_CONTROL, VK_SHIFT, 'M', 0 };
    } keys;
  } mem;


  struct {
    bool   show           = false;
    float  interval       = 0.25f; // 250 msecs (4 Hz)

    struct {
      BYTE toggle [4]     = { VK_CONTROL, VK_SHIFT, 'I', 0 };
    } keys;
  } io;


  struct {
    bool   show           = false;

    struct {
      BYTE toggle [4]     = { VK_CONTROL, VK_SHIFT, 'S', 0 };
    } keys;
  } sli;


  struct {
    bool   show           = false;

    bool   pump           = false;
    float  pump_interval  = 0.0166666666f;

    int    red            = MAXDWORD32;
    int    green          = MAXDWORD32;
    int    blue           = MAXDWORD32;
    float  scale          =  1.0f;
    int    pos_x          =  0;
    int    pos_y          =  0;

    struct {
      BYTE toggle [4]     = { VK_CONTROL, VK_SHIFT, 'O',          0 };
      BYTE shrink [4]     = { VK_CONTROL, VK_SHIFT, VK_OEM_MINUS, 0 };
      BYTE expand [4]     = { VK_CONTROL, VK_SHIFT, VK_OEM_PLUS,  0 };
    } keys;

    bool   remember_state = false;
  } osd;


  struct {
    bool   show           = false;
    float  interval       = 0.33333333f;
    bool   simple         = true;

    struct {
      BYTE toggle  [4]    = { VK_CONTROL, VK_SHIFT, 'C', 0 };
    } keys;
  } cpu;


  struct {
    bool   show           = true;

    struct {
      BYTE toggle [4]     = { VK_CONTROL, VK_SHIFT, 'F', 0 };
    } keys;
    //float fps_interval  = 1.0f;
  } fps;


  struct {
    bool   show           = true;
    bool   print_slowdown = false;
    float  interval       = 0.333333f;

    struct {
      BYTE toggle [4]     = { VK_CONTROL, VK_SHIFT, 'G', 0 };
    } keys;
  } gpu;


  struct {
    bool   show            = false;

    float  interval        = 0.333333f;
    int    type            = 0; // Physical = 0,
                                // Logical  = 1

    struct {
      BYTE toggle [4]      = { VK_CONTROL, VK_MENU, VK_SHIFT, 'D' };
    } keys;
  } disk;


  struct {
    bool   show            = false;
    float  interval        = 2.5f;

    struct {
      BYTE toggle [4]      = { VK_CONTROL, VK_MENU, VK_SHIFT, 'P' };
    } keys;
  } pagefile;


  struct {
    bool    enable             = true;
    bool    safe_init          = true;
  } cegui;

  struct {
    float   scale              = 1.0f;
    bool    show_eula          = true;
    bool    show_input_apis    = true;
    bool    use_mac_style_menu = false;

    struct {
      struct {
        std::string file   = "";
        float       size   = 7.0f;
      } chinese,  cyrillic, default,
        japanese, korean;
    } font;
  } imgui;


  struct {
    struct {
      bool    pull_friend_stats   = true;
      bool    pull_global_stats   = true;

      struct {
        bool  show                = true;
        bool  show_title          = true;
        int   origin              = 0;
        bool  animate             = true;
        int   duration            = 5000UL;
        float inset               = 0.005f;
      } popup;

      std::wstring
              sound_file          = L"";
      bool    take_screenshot     = false;
      bool    play_sound          = true;
    } achievements;

    int     notify_corner        = 0; // 0=Top-Left,
                                      // 1=Top-Right,
                                      // 2=Bottom-Left,
                                      // 3=Bottom-Right,
                                      // 4=Don't Care

    bool    silent               = false;

    int     init_delay           = 0UL; // Disable to prevent crashing in many games
    bool    auto_pump_callbacks  = true;
    bool    block_stat_callback  = false;
    bool    filter_stat_callback = true;

    int     appid                = 0;
    bool    preload_client       = false;
    bool    preload_overlay      = true;  // For input processing, this is important
    bool    show_playtime        = true;  // In the control panel title
    bool    force_load_steamapi  = false; // Load steam_api{64}.dll even in games
                                          //   that do not use it
    bool    spoof_BLoggedOn      = false;
  } steam;


  struct {
    struct {
      float   target_fps        =  0.0f;
      float   limiter_tolerance = 0.25f;
      int     pre_render_limit  = -1;
      int     present_interval  = -1;
      int     buffer_count      = -1;
      int     max_delta_time    =  0; // Bad old setting; needs to be phased out
      bool    flip_discard      = false;
      int     swapchain_wait    =  0;
      int     refresh_rate      = -1;
      int     pin_render_thread = -1;
      bool    wait_for_vblank   = false;
      bool    sleepless_render  = true;
      bool    sleepless_window  = false;
    } framerate;
    struct {
      bool    force_d3d9ex      = false;
      bool    force_impure      = false;
      int     hook_type         = 0;
      bool    osd_in_vidcap     = false;
    } d3d9;
    struct {
      int     adapter_override  = -1;
      struct {
        struct {
          unsigned int x        =  0;
          unsigned int y        =  0;
          bool isZero (void) { return x == 0 && y == 0; };
        } min;
        struct {
          unsigned int x        =  0;
          unsigned int y        =  0;
          bool isZero (void) { return x == 0 && y == 0; };
        } max;
      } res;
      int     exception_mode    =    -1; // -1 = Don't Care
      int     scaling_mode      =    -1; // -1 = Don't Care
      int     scanline_order    =    -1; // -1 = Don't Care
      // DXGI 1.1 (Ignored for now)
      int     rotation          =    -1; // -1 = Don't Care
      bool    test_present      = false;
      bool    full_state_cache  = false;
      bool    debug_layer       = false;
      bool    allow_tearing     = false;
      bool    safe_fullscreen   = false;
      bool    enhanced_depth    = false;
    } dxgi;

    // OSD Render Stats (D3D11 Only Right Now)
    bool      show              = false;
    struct {
      BYTE toggle [4]      = { VK_CONTROL, VK_SHIFT, 'R', 0 };
    } keys;
  } render;

  struct {
    bool      force_fullscreen  = false;
    bool      force_windowed    = false;
    int       monitor_idx       =    -1; // TODO
    float     refresh_rate      =  0.0f; // TODO
  } display;

  struct {
    struct {
      bool    precise_hash      = false;
      bool    dump              = false;
      bool    inject            = true;
      bool    cache             = true;
      std::wstring
              res_root          = L"SK_Res";
      bool    highlight_debug   = true;
    } d3d11;
    struct {
      int     min_evict         = 64;
      int     max_evict         = 1024;
      int     min_entries       = 512;
      int     max_entries       = 16384; // Bump max up from 8192 since the hashmap
                                         //   is now segmented by number of mipmap LODs;
                                         //
                                         //  Overhead of managing a large hashmap is
                                         //    much lower.
      int     min_size          = 384L;
      int     max_size          = 2048L;
      bool    ignore_nonmipped  = false;
      bool    allow_staging     = false;
    } cache;

    bool highlight_debug_tex    = false;
    bool on_demand_dump         = false;
    bool d3d9_mod               = true;
    bool dump_on_load           = false;
  } textures;

  struct {
    struct {
      std::wstring
              compatibility     = L"0x00000000";
      std::wstring
              mode              = L"Auto";
      std::wstring
              num_gpus          = L"Auto";
      bool    override          = false;
    } sli;
  } nvidia;

  struct {
    struct {
      int     timeout           = 1500UL;
      bool    manage            = false;
      bool    keys_activate     = true;
    } cursor;

    struct {
      union {
        bool  capture           = false;
        bool  capture_mouse;
      };
      bool    capture_hidden    = false; // Capture mouse if HW cursor is not visible
      bool    capture_keyboard  = false;
      bool    capture_gamepad   = false;
      bool    use_hw_cursor     =  true;
      bool    use_raw_input     =  true;
    } ui;

    struct {
      bool    disable_ps4_hid   = false;
      bool    rehook_xinput     = false;
      bool    haptic_ui         = true;
      bool    hook_dinput8      = true;
      bool    hook_hid          = true;
      bool    hook_xinput       = true; // Kind of important ;)
      bool    native_ps4        = false;
      int     predefined_layout = 1;    //0 = PS4, 1 = Steam, 2 = Xbox

      struct {
        unsigned
        int   ui_slot           =    0;
        bool  placehold  [4]    = { false };
        int   assignment [4]    = { 0, 1, 2, 3 };
        bool  disable_rumble    = false;
      } xinput;
    } gamepad;

    struct {
      bool    block_windows_key = false;
    } keyboard;

    struct {
      // Translate WM_MOUSEWHEEL messages into actual events that will trigger
      //   other mouse APIs such as DirectInput and RawInput.
      //
      //  * Without this, mousewheel scrolling doesn't work in MANY games on a
      //      Synaptics touchpad.
      //
      bool    fix_synaptics       = false;
      // If absolute cursor position is stuck (i.e. Dreamfall Chapters) use this
      bool    add_relative_motion = true;
//
// Uses APIs such as DirectInput or RawInput that only send relative motion events
//   to derive the virtual position of the cursor, since the game hijacks the
//     physical position.
//
//   >> Ideally we want absolute cursor position every frame for the UI, but
//        that's not always possible. <<
//
      float   antiwarp_deadzone = 2.5f;
    } mouse;
  } input;

  struct {
    bool    borderless          = false;
    bool    border_override     = false;
    bool    center              = false;
    struct {
      struct {
      int   absolute            = 0;
      float percent             = 0.0f;
      } x, y;
      bool isZero (void) { return x.absolute == 0    && y.absolute == 0 &&
                                  x.percent > -0.00001f && x.percent < 0.00001f &&
                                  y.percent > -0.00001f && y.percent < 0.00001f; }
    } offset;
    bool    background_render   = false;
    bool    background_mute     = false;
    bool    confine_cursor      = false;
    bool    unconfine_cursor    = false;
    bool    persistent_drag     = false;
    bool    drag_lock           = false; // NOT SAVED IN INI
    bool    fullscreen          = false;
    struct {
      struct {
        unsigned int x          = 0;
        unsigned int y          = 0;
        bool         fix_mouse  = false;
        bool isZero (void) { return x == 0 && y == 0; };
      } override;
    } res;
  } window;

  struct {
    bool    ignore_raptr         = false;
    bool    disable_raptr        = false;
    bool    rehook_loadlibrary   = false;
    bool    disable_nv_bloat     = false;
    bool    disable_msi_deadlock = true;
    bool    disable_FRAPS_evil   = true;
    bool    disable_razer_crash  = true;
    bool    disable_discord      = true;

    struct {
      bool  rehook_reset         = false;
      bool  rehook_present       = false;
      bool  hook_reset_vftbl     = false;
      bool  hook_present_vftbl   = false;
    } d3d9;

    bool   init_while_suspended  = true;
  } compatibility;

  struct {
    struct {
      bool   hook              = true;
    } glide;

#ifndef _WIN64
    struct {
      bool   hook              = true;
    } d3d8, ddraw;
#endif

    struct {
      bool   hook              = true;
    } d3d9, d3d9ex;

    struct {
      struct {
        bool hook              = true;
      }
#ifdef _WIN64
        d3d12,
#endif
        d3d11;
    } dxgi;

    struct {
      bool   hook              = true;
    }
#ifdef _WIN64
      Vulkan,
#endif
      OpenGL;

    struct {
      bool   enable            = true;
      bool   gsync_status      = true;
    } NvAPI;

    struct {
      bool   enable            = true;
    } ADL;

    SK_RenderAPI last_known    = SK_RenderAPI::Reserved;
  } apis;

  struct {
    int     init_delay          = 250;
    bool    silent              = false;
    int     log_level           = 0;
    bool    handle_crashes      = true;
    bool    prefer_fahrenheit   = false;
    bool    display_debug_out   = false;
    bool    game_output         = true;
    bool    central_repository  = false;
    bool    ignore_rtss_delay   = false;
    std::wstring
            version             = SK_VER_STR;
    bool    trace_load_library  = true;
    bool    strict_compliance   = false;
    float   global_inject_delay = 0.0f;
  } system;
} extern config;


struct SK_Keybind
{
  const char*  bind_name;
  std::wstring human_readable;

  struct {
    BOOL ctrl,
         shift,
         alt;
  };

  SHORT vKey;

  UINT  masked_code; // For fast comparison

  void parse  (void);
  void update (void);
};

bool SK_LoadConfigEx (std::wstring name, bool create = true);
bool SK_LoadConfig   (std::wstring name         = L"dxgi");
bool SK_DeleteConfig (std::wstring name         = L"dxgi");
void SK_SaveConfig   (std::wstring name         = L"dxgi",
                      bool         close_config = false);

//
// Sort of a nasty hack until I come up with a way for plug-ins to
//   formally read the parent DLL's configuration.
//
uint32_t
__cdecl
SK_Config_GetTargetFPS (void);


struct iSK_INI;

struct SK_AppCache_Manager
{
  bool         saveAppCache       (bool           close = false);
  bool         loadAppCacheForExe (const wchar_t* wszExe);

  uint32_t     getAppIDFromPath   (const wchar_t* wszPath) const;
  std::wstring getAppNameFromID   (uint32_t       uiAppID) const;
  std::wstring getAppNameFromPath (const wchar_t* wszPath) const;

  bool         addAppToCache      ( const wchar_t* wszRelativePath,
                                    const wchar_t* wszExecutable,
                                    const wchar_t* wszAppName,
                                          uint32_t uiAppID );

  std::wstring getConfigPathFromAppPath (const wchar_t* wszPath) const;
  std::wstring getConfigPathForAppID    (uint32_t       uiAppID) const;

  int          migrateProfileData       (LPVOID reserved = nullptr);

protected:
  iSK_INI* app_cache_db = nullptr;
} extern app_cache_mgr;





enum class SK_GAME_ID
{
  Tyranny,                      // Tyranny.exe
  Shadowrun_HongKong,           // SRHK.exe
  TidesOfNumenera,              // TidesOfNumenera.exe
  MassEffect_Andromeda,         // MassEffectAndromeda.exe
  MadMax,                       // MadMax.exe
  Dreamfall_Chapters,           // Dreamfall Chapters.exe
  TheWitness,                   // witness_d3d11.exe, witness64_d3d11.exe
  Obduction,                    // Obduction-Win64-Shipping.exe
  TheWitcher3,                  // witcher3.exe
  ResidentEvil7,                // re7.exe
  DragonsDogma,                 // DDDA.exe
  EverQuest,                    // eqgame.exe
  GodEater2RageBurst,           // GE2RB.exe
  WatchDogs2,                   // WatchDogs2.exe
  NieRAutomata,                 // NieRAutomata.exe
  Warframe_x64,                 // Warframe.x64.exe
  LEGOCityUndercover,           // LEGOLCUR_DX11.exe
  Sacred,                       // sacred.exe
  Sacred2,                      // sacred2.exe
  FinalFantasy9,                // FF9.exe   
  EdithFinch,                   // FinchGame.exe
  FinalFantasyX_X2,             // FFX.exe / FFX-2.exe
  DeadlyPremonition,            // DP.exe DPLauncher.exe
  GalGun_Double_Peace,          // GG2Game.exe
  AKIBAs_Trip,                  // AkibaUU.exe
  YS_Seven,                     // Ys7.exe
  LifeIsStrange_BeforeTheStorm, // Life is Strange - Before the Storm.exe
  Tales_of_Symphonia,           // TOS.exe
  UNKNOWN_GAME               = 0xffff
};

SK_GAME_ID
__stdcall
SK_GetCurrentGameID (void);

const wchar_t*
__stdcall
SK_GetNaiveConfigPath (void);

extern const wchar_t*
SK_GetFullyQualifiedApp (void);

#endif __SK__CONFIG_H__