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

#include <Unknwnbase.h>

#include <Windows.h>
#include <string>
#include <set>
#include <unordered_set>
#include <concurrent_unordered_map.h>
#include <filesystem>
#include <intsafe.h>

#include <SpecialK/render/backend.h>
#include <SpecialK/window.h>

struct SK_Keybind
{
  const char*  bind_name      = nullptr;
  std::wstring human_readable =     L"";

  struct {
    BOOL ctrl  = FALSE,
         shift = FALSE,
         alt   = FALSE;
  };

  SHORT vKey        =   0;
  UINT  masked_code = 0x0; // For fast comparison

  void parse  (void);
  void update (void);
};

namespace sk
{
  class ParameterStringW;
};

// Adds a parameter to store and retrieve the keybind in an INI / XML file
struct SK_ConfigSerializedKeybind : public SK_Keybind
{
  SK_ConfigSerializedKeybind ( SK_Keybind&& bind,
                             const wchar_t* cfg_name) :
                               SK_Keybind  (bind)
  {
    wcsncpy_s ( short_name, 32,
                  cfg_name, _TRUNCATE );
  }

  bool                  assigning       = false;
  wchar_t               short_name [32] = L"Uninitialized";
  sk::ParameterStringW* param           = nullptr;
};


namespace skif
{
  enum class AutoStopBehavior {
    Never   = 0,
    AtStart = 1,
    AtExit  = 2
  };
}


struct sk_config_t
{
  sk_config_t (void)
  {
    // This struct is one of the earliest non-static initalized
    //   parts of the DLL, so important early init is best performed
    //     in the constructor of sk_config_t
    //
    LARGE_INTEGER               liQpcFreq = { };
    QueryPerformanceFrequency (&liQpcFreq);

    SK_QpcFreq       = liQpcFreq.QuadPart;
    SK_QpcTicksPerMs = SK_QpcFreq / 1000LL;
  }
  struct whats_new_s {
    float  duration       = 20.0F;
  } version_banner;

  struct time_osd_s {
    LONG   format         = LOCALE_USER_DEFAULT;

    struct keybinds_s {
      BYTE toggle [4]     = { VK_CONTROL, VK_SHIFT, 'T', 0 };
    } keys;

    bool   show           = true;
  } time;

  struct mem_osd_s {
    float  reserve        = 0.0F;// Unused / Unlimited
    float  interval       = 0.25F;

    struct keybinds_s {
      BYTE toggle [4]     = { VK_CONTROL, VK_SHIFT, 'M', 0 };
    } keys;

    bool   show           = false;
  } mem;


  struct io_osd_s {
    float  interval       = 0.25F; // 250 msecs (4 Hz)

    struct keybinds_s {
      BYTE toggle [4]     = { VK_CONTROL, VK_SHIFT, 'I', 0 };
    } keys;

    bool   show           = false;
  } io;


  struct sli_s {
    bool   show           = false;

    struct keybinds_s {
      BYTE toggle [4]     = { VK_CONTROL, VK_SHIFT, 'S', 0 };
    } keys;
  } sli;


  struct basic_osd_s {
    int    red            = MAXDWORD32;
    int    green          = MAXDWORD32;
    int    blue           = MAXDWORD32;
    float  scale          =  1.0F;
    int    pos_x          =  0;
    int    pos_y          =  0;

    struct keybinds_s {
      BYTE toggle [4]     = { VK_CONTROL, VK_SHIFT, 'O',          0 };
      BYTE shrink [4]     = { VK_CONTROL, VK_SHIFT, VK_OEM_MINUS, 0 };
      BYTE expand [4]     = { VK_CONTROL, VK_SHIFT, VK_OEM_PLUS,  0 };
    } keys;

    bool   remember_state = false;
    bool   show           = false;
  } osd;


  struct cpu_osd_s {
    GUID      power_scheme_guid      = { };
    GUID      power_scheme_guid_orig = { };

    float  interval       = 0.33333333f;

    bool   simple         = true;
    bool   show           = false;

    struct keybinds_s {
      BYTE toggle  [4]    = { VK_CONTROL, VK_SHIFT, 'C', 0 };
    } keys;
  } cpu;


  struct fps_osd_s {
    bool   show           = true;
    bool   compact        = false;
    bool   advanced       = false;
    bool   frametime      = true;

    struct keybinds_s {
      BYTE toggle [4]     = { VK_CONTROL, VK_SHIFT, 'F', 0 };
    } keys;
    //float fps_interval  = 1.0F;
  } fps;


  struct gpu_osd_s {
    bool   show           = true;
    bool   print_slowdown = false;
    float  interval       = 0.333333F;

    struct keybinds_s {
      BYTE toggle [4]     = { VK_CONTROL, VK_SHIFT, 'G', 0 };
    } keys;
  } gpu;


  struct disk_osd_s {
    bool   show            = false;

    float  interval        = 0.333333F;
    int    type            = 0; // Physical = 0,
                                // Logical  = 1

    struct keybinds_s {
      BYTE toggle [4]      = { VK_CONTROL, VK_MENU, VK_SHIFT, 'D' };
    } keys;
  } disk;


  struct pagefile_osd_s {
    bool   show            = false;
    float  interval        = 2.5F;

    struct keybinds_s {
      BYTE toggle [4]      = { VK_CONTROL, VK_MENU, VK_SHIFT, 'P' };
    } keys;
  } pagefile;


  struct cegui_s {
    ULONG   frames_drawn       = 0;     //   Count the number of frames drawn using it
    bool    enable             = true;
    bool    orig_enable        = false; // Since CEGUI is a frequent source of crashes.
    bool    safe_init          = true;
  } cegui;

  struct imgui_s {
    float   scale              = 1.0F;
    bool    show_eula          = false; // Will be flipped on if no AppCache is present
    bool    show_input_apis    = true;
    bool    use_mac_style_menu = false;

    struct font_s {
      struct font_params_s {
        std::string file   = "";
        float       size   = 7.0F;
      } chinese      { "msyh.ttc",     18.0f },
        cyrillic     { "arial.ttf",    18.0f },
        default_font { "arial.ttf",    18.0f },
        japanese     { "msgothic.ttc", 18.0f },
        korean       { "malgun.ttf",   18.0f };
    } font;

    // Per-game (mostly compatibility) settings
    struct render_s
    {
      bool  strip_alpha        = false; // Alpha is forcefully stripped
      bool  disable_alpha      = false; // Window backgrounds are opaque
      bool  antialias_lines    = true;
      bool  antialias_contours = true;
    } render;
  } imgui;


  struct steam_s {
    struct callback_cache_s {
      HMODULE
        module               = nullptr;
      uintptr_t
        offset               = 0;
      void* resolved         = nullptr;
    } cached_overlay_callback;

    struct cloud_s {
      std::set <std::string> blacklist;
    } cloud;

    std::wstring
                dll_path              = L"";

    int64_t     appid                 = 0LL;
    int         online_status         = -1;  // Force a certain online status at all times
    int         init_delay            = 0UL; // Disable to prevent crashing in many games
    int         callback_throttle     = -1;
    bool        preload_client        = false;
    bool        preload_overlay       = false;  // For input processing, this is important
    bool        force_load_steamapi   = false;  // Load steam_api{64}.dll even in games
    bool        auto_pump_callbacks   =  true;
    bool        block_stat_callback   = false;
    bool        filter_stat_callback  = false;
    bool        spoof_BLoggedOn       = false;
    bool        auto_inject           =  true;  // Control implicit steam_api.dll bootstrapping

    struct screenshot_handler_s {
      bool      enable_hook           =  true;
    } screenshots;
  } steam;

  struct platform_s {
    struct achievements_s {
      std::wstring
                sound_file            = L"";

      struct popup_s {
        float   inset                 = 0.005F;
        int     origin                = 0;
        int     duration              = 5000UL;
        bool    show                  =  true;
        bool    show_title            =  true;
        bool    animate               =  true;
      } popup;

      bool      take_screenshot       = false;
      bool      play_sound            =  true;
      bool      pull_friend_stats     =  true;
      bool      pull_global_stats     =  true; // N/A for EOS
    } achievements;

    float       overlay_hdr_luminance = 4.37F; // 350 nits
                                               //   that do not use it

    int         notify_corner         = 4;     // 0=Top-Left,
                                               // 1=Top-Right,
                                               // 2=Bottom-Left,
                                               // 3=Bottom-Right,
                                               // 4=Don't Care
    bool        show_playtime         =  true; // In the control panel title
    bool        overlay_hides_sk_osd  =  true;
    bool        reuse_overlay_pause   =  false;// Use Steam's overlay pause mode for our own
                                               //   control panel
    bool        silent                = false;
  } platform;

  struct epic_s {
    std::string app_name              = "";
    float       overlay_luminance     = 4.375F; // 350 nits
    bool        present               = false;  // Is the overlay detected?
    bool        warned_online         = false;
  } epic;

  struct uplay_s {
    float       overlay_luminance     = 4.375F; // 350 nits
    bool        present               = false;  // Is the overlay detected?
  } uplay;

  struct discord_s {
    float       overlay_luminance     = 4.375F; // 350 nits
    bool        present               = false;  // Is the overlay detected?
  } discord;

  struct rtss_s {
    float       overlay_luminance     = 4.375F; // 350 nits
    bool        present               = false;  // Is the overlay detected?
  } rtss;

  struct sound_s {
    SK_ConfigSerializedKeybind
         game_mute_keybind = {
      SK_Keybind {
        "Mute the Game", L"Ctrl+Shift+Home",
         true, true, false, VK_HOME
      }, L"MuteGame"
    };

    SK_ConfigSerializedKeybind
         game_volume_up_keybind = {
      SK_Keybind {
        "Increase Volume 10%", L"Ctrl+Shift+Insert",
         true, true, false, VK_INSERT
      }, L"VolumePlus5%"
    };

    SK_ConfigSerializedKeybind
         game_volume_down_keybind = {
      SK_Keybind {
        "Decrease Volume 10%", L"Ctrl+Shift+Delete",
         true, true, false, VK_DELETE
      }, L"VolumeMinus5%"
    };
  } sound;

  struct screenshots_s {
    bool        png_compress          =  true;
    bool        show_osd_by_default   =  true;
    bool        play_sound            =  true;
    bool        copy_to_clipboard     =  true;

    SK_ConfigSerializedKeybind
         game_hud_free_keybind = {
      SK_Keybind {
        "Take Screenshot without Game's HUD", L"Num -",
         false, false, false, VK_OEM_MINUS
      }, L"HUDFree"
    };

    SK_ConfigSerializedKeybind
           sk_osd_free_keybind = {
      SK_Keybind {
        "Take Screenshot without Special K's OSD", L"F9",
         false, false, false, VK_F9
      }, L"WithoutOSD"
    };

    SK_ConfigSerializedKeybind
      sk_osd_insertion_keybind = {
      SK_Keybind {
        "Take Screenshot and insert Special K's OSD", L"F8",
         false, false, false, VK_F8
      }, L"InsertOSD"
    };
  } screenshots;

  struct monitor_s {
    SK_ConfigSerializedKeybind
         monitor_primary_keybind = {
      SK_Keybind {
        "Move Game to Primary Monitor", L"<Not Bound>",
         false, false, false, 0,
      }, L"MoveToPrimaryMonitor"
    };

    SK_ConfigSerializedKeybind
         monitor_next_keybind = {
      SK_Keybind {
        "Move Game to Next Monitor", L"<Not Bound>",
         false, false, false, 0,
      }, L"MoveToNextMonitor"
    };

    SK_ConfigSerializedKeybind
         monitor_prev_keybind = {
      SK_Keybind {
        "Move Game to Previous Monitor", L"<Not Bound>",
         false, false, false, 0,
      }, L"MoveToPrevMonitor"
    };

    SK_ConfigSerializedKeybind
         monitor_toggle_hdr = {
      SK_Keybind {
        "Toggle HDR on Active Monitor", L"<Not Bound>",
         false, false, false, 0
      }, L"ToggleHDR"
    };
  } monitors;

  struct render_s {
    struct framerate_s {
      float   target_fps          =  0.0F;
      float   target_fps_bg       =  0.0F;
      int     override_num_cpus   = -1;
      int     pre_render_limit    = -1;
      int     present_interval    = -1;
      int     sync_interval_clamp = -1;
      int     buffer_count        = -1;
      int     max_delta_time      =  0; // Bad old setting; needs to be phased
      int     swapchain_wait      =  0;
      float   refresh_rate        = -1.0F;
 std::wstring rescan_ratio        =L"-1/1";
      struct rescan_s {
        UINT Denom                =  1;
        UINT Numerator            =
               static_cast <UINT> (-1);
      } rescan_;
      int     refresh_denom       =     1;
      int     pin_render_thread   =    -1;
      bool    flip_discard        =  true; // Enabled by default (7/6/21)
      bool    flip_sequential     = false;
      bool    disable_flip        = false;
      bool    drop_late_flips     =  true;
      bool    wait_for_vblank     = false;
      bool    sleepless_render    = false;
      bool    sleepless_window    = false;
      bool    enable_mmcss        =  true;
      int     enforcement_policy  =     4; // Refer to framerate.cpp
      bool    auto_low_latency    =  true; // VRR users have the limiter default to low-latency
      bool    enable_etw_tracing  =  true;
      bool    supports_etw_trace  =  false;// Not stored in config file
      struct latent_sync_s {
        SK_ConfigSerializedKeybind
          tearline_move_up_keybind = {
            SK_Keybind {
              "Move Tear Location Up 5 Scanlines", L"<Not Bound>",
               false, false, false, 0,
            }, L"MoveTearlineUp"
          };
        SK_ConfigSerializedKeybind
          tearline_move_down_keybind = {
            SK_Keybind {
              "Move Tear Location Down 5 Scanlines", L"<Not Bound>",
               false, false, false, 0,
            }, L"MoveTearlineDown"
          };
        SK_ConfigSerializedKeybind
          timing_resync_keybind = {
            SK_Keybind {
              "Force a Timing Resync", L"<Not Bound>",
               false, false, false, 0,
            }, L"ManualResync"
          };
        SK_ConfigSerializedKeybind
          toggle_fcat_bars_keybind = {
            SK_Keybind {
              "Toggle Tearline Visualizer", L"<Not Bound>",
               false, false, false, 0,
            }, L"ToggleFCATBars"
          };
        int   scanline_offset      =    -1;
        int   scanline_resync      =   750;
        int   scanline_error       =     1;
        bool  adaptive_sync        =  true;
        float delay_bias           =  0.0f;
        bool  show_fcat_bars       = false; // Not INI-persistent

        bool flush_before_present  = true;
        bool finish_before_present = false;

        bool flush_after_present   = false;
        bool finish_after_present  = true;
      } latent_sync;
    } framerate;
    struct d3d9_s {
      bool    force_d3d9ex         = false;
      bool    force_impure         = false;
      bool    enable_flipex        = false;
      bool    use_d3d9on12         = false;
    } d3d9;
    struct dxgi_s {
      int     adapter_override     = -1;
      struct resolution_s {
        struct minimum_s {
          unsigned int x           =  0;
          unsigned int y           =  0;
          bool isZero (void) noexcept { return x == 0 && y == 0; };
        } min;
        struct maximum_s {
          unsigned int x           =  0;
          unsigned int y           =  0;
          bool isZero (void) noexcept { return x == 0 && y == 0; };
        } max;
      } res;
      struct refresh_s {
        float min = 0.0f;
        float max = 0.0f;
      } refresh;
      int     exception_mode       =    -1; // -1 = Don't Care
      int     scaling_mode         =    -1; // -1 = Don't Care
      int     scanline_order       =    -1; // -1 = Don't Care
      int     msaa_samples         =    -1;
      // DXGI 1.1 (Ignored for now)
      int     rotation             =    -1; // -1 = Don't Care
      int     srgb_behavior        =    -2; // -2 = sRGB Not Encountered Yet
                                            // -1 = Passthrough,
                                            //  0 = Strip,
                                            //  1 = Apply
      bool    test_present         = false;
      bool    full_state_cache     = false;
      bool    debug_layer          = false;
      bool    low_spec_mode        =  true; // Disable D3D11 Render Mods
      bool    allow_tearing        =  true;
      bool    safe_fullscreen      = false;
      bool    enhanced_depth       = false;
      bool    deferred_isolation   = false;
      bool    present_test_skip    = false;
      bool    hide_hdr_support     = false; // Games won't know HDR is supported
      bool    use_factory_cache    =  true; // Fix performance issues in Resident Evil 8
      bool    skip_mode_changes    = false; // Try to skip rendundant resolution changes
      bool    temporary_dwm_hdr    = false; // Always turns HDR on and off for this game
      bool    disable_virtual_vbi  =  true; // Disable Windows 11 Dynamic Refresh Rate
      bool    ignore_thread_flags  = false; // Remove threading flags from D3D11 devices
      bool    clear_flipped_chain  =  true; // Clear buffers on present? (non-compliant)
      float   chain_clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
      bool    suppress_resize_fail =  true; // Workaround EOS Overlay bug in D3D12
    } dxgi;

    struct {
      bool    disable_fullscreen   = true;
      bool    enable_16bit_hdr     = false;
      bool    enable_10bit_hdr     = false;
    } gl;

    struct osd_s {
      ULONG64 _last_vidcap_frame   = 0;
      ULONG64 _last_normal_frame   = 0;
      float   hdr_luminance        = 4.375F; // 350 nits
      // Required by default for compatibility with Mirillis Action!
      bool    draw_in_vidcap       = true;
    } osd;

    // OSD Render Stats
    bool      show                 = false;
    struct keybinds_s {
      //BYTE    toggle [4]         = { VK_CONTROL, VK_SHIFT, 'R', 0 };
      SK_ConfigSerializedKeybind
        hud_toggle = {
          SK_Keybind {
            "Toggle Game's HUD", L"Alt+Shift+H",
             false, true, true, 'H'
          }, L"HUDToggle"
        };
    } keys;

    struct output_format_s {
      bool    force_8bpc           = false; ///< UNUSED
      bool    force_10bpc          = false; ///< D3D9/11/12
      bool    force_16bpc          = false; ///< UNUSED
    } output;

    struct {
      bool enable_32bpc            = false;
    } hdr;
  } render;

  struct display_s {
    std::wstring
              monitor_path_ccd     =   L"";
    int       monitor_idx          =     0;
    HMONITOR  monitor_handle       =     0;
    int       monitor_default      = MONITOR_DEFAULTTONEAREST;
    float     refresh_rate         =  0.0F; // TODO
    bool      force_fullscreen     = false;
    bool      force_windowed       = false;
    bool      aspect_ratio_stretch = false;
    bool      confirm_mode_changes = true;
    bool      save_monitor_prefs   = true;
    bool      warn_no_mpo_planes   = false;
    struct resolution_s {
      bool           save          = true;
      bool           applied       = false;
      struct desktop_override_s {
        unsigned int x             = 0;
        unsigned int y             = 0;
        bool isZero (void) noexcept { return x == 0 && y == 0; };
      } override, original;
      // TODO
      std::wstring  monitor_path_ccd = L"";
    } resolution;
  } display;

  struct textures_s {
    struct d3d11_s {
      std::wstring
        res_root                   = L"SK_Res";
      bool    precise_hash         = false;
      bool    dump                 = false;
      bool    inject               = true;
      bool    cache                = true;
      bool    highlight_debug      = true;
      bool    injection_keeps_fmt  = false;
      bool    generate_mips        = false;
      bool    cache_gen_mips       =  true;
      bool    uncompressed_mips    = false;
    } d3d11;
    struct cache_s {
      int     min_evict            = 64;
      int     max_evict            = 1024;
      int     min_entries          = 512;
      int     max_entries          = 65536; // Bump max up from 8192 since the hashmap
                                            //   is now segmented by number of mipmap LODs;
                                            //
                                            //  Overhead of managing a large hashmap is
                                            //    much lower.
      int     min_size             = 384L;
      int     max_size             = 2048L;
      bool    ignore_nonmipped     = false;
      bool    allow_staging        = false;
      bool    allow_unsafe_refs    = false; // Allow texture caching even in engines that
                                            //   are not correctly keeping track of resources
      bool    residency_managemnt  = false;// true;
      bool    vibrate_on_miss      = false;
    } cache;

    bool highlight_debug_tex       =  true;
    bool on_demand_dump            = false;
    bool d3d9_mod                  = false; // Causing compat issues in some D3D9Ex games ATM
    bool dump_on_load              = false;
    bool clamp_lod_bias            = false;
  } textures;

  struct file_trace_s {
    bool trace_reads               = false;
    bool trace_writes              = false;

    struct ignore_files_s {
      Concurrency::concurrent_unordered_set <std::wstring> single_file;
      Concurrency::concurrent_unordered_set <std::wstring> entire_thread;
    } ignore_reads,
      ignore_writes;

  } file_io;

  struct nvidia_s {
    struct sli_s {
      std::wstring
              compatibility       = L"0x00000000";
      std::wstring
              mode                = L"Auto";
      std::wstring
              num_gpus            = L"Auto";
      bool    override            = false;
    } sli;
    struct bugs_s {
    //bool    fix_10bit_gsync     = false;
      bool    kill_hdr            = false;
      bool    snuffed_ansel       = false;
      bool    bypass_ansel        =  true;
    } bugs;
    struct sleep_s {
      UINT    frame_interval_us   =      0;
      int     enforcement_site    =      1;
      bool    low_latency         =  false;
      bool    low_latency_boost   =  false;
      bool    marker_optimization =  false;
      bool    enable              =  false;
    } sleep;
  } nvidia;

  struct input_s {
    struct cursor_s {
      bool    keys_activate       =  false;
      bool    manage              =  false;
      int     timeout             = 1500UL;
    } cursor;

    struct ui_s {
      union {
        bool  capture             = false;
        bool  capture_mouse;
      };
      bool    capture_hidden      = false; // Capture mouse if HW cursor is not visible
      bool    capture_keyboard    = false;
      bool    capture_gamepad     = false;
      bool    use_hw_cursor       =  true;
      bool    use_raw_input       =  true;
    } ui;

    struct gamepad_s {
      int     predefined_layout   = 1;    //0 = PS4, 1 = Steam, 2 = Xbox
      int     disabled_to_game    = 0;    //0 = Never, 1 = Always, 2 = In Background
      bool    disable_ps4_hid     = false;
      bool    rehook_xinput       = false;
      bool    haptic_ui           = true;
      bool    disable_rumble      = false;
      bool    hook_dinput8        = true;
      bool    hook_dinput7        = true;
      bool    hook_hid            = true;
      bool    hook_xinput         = true; // Kind of important ;)
      bool    hook_scepad         = true;
      bool    native_ps4          = false;

      struct xinput_s {
        unsigned
        int   ui_slot             =    0;
        bool  placehold  [4]      = { false };
        unsigned
        int   assignment [4]      = { 0, 1, 2, 3 };
        bool  hook_setstate       =  true; // Some software causes feedback loops
        bool  auto_slot_assign    = false;
      } xinput;

      struct scepad_s {
        bool disable_touch        = false;
        bool share_clicks_touch   =  true;
        bool mute_applies_to_game =  true;
        bool enhanced_ps_button   =  true;
      } scepad;

      struct steam_s
      {
        unsigned
        int   ui_slot             =     0;
        bool  disabled_to_game    = false;
      } steam;
    } gamepad;

    struct keyboard_s {
      bool    block_windows_key   = false;
      bool    catch_alt_f4        =  true;
      bool    override_alt_f4     = false; // For games that have prompts (i.e. DQ XI / Yakuza)
      int     disabled_to_game    =     2; //0 = Never, 1 = Always, 2 = In Background
    } keyboard;

    struct mouse_s {
      //
      // Uses APIs such as DirectInput or RawInput that only send relative motion events
      //   to derive the virtual position of the cursor, since the game hijacks the
      //     physical position.
      //
      //   >> Ideally we want absolute cursor position every frame for the UI, but
      //        that's not always possible. <<
      //
      float   antiwarp_deadzone   = 2.5F;

      // Translate WM_MOUSEWHEEL messages into actual events that will trigger
      //   other mouse APIs such as DirectInput and RawInput.
      //
      //  * Without this, mousewheel scrolling doesn't work in MANY games on a
      //      Synaptics touchpad.
      //
      bool    fix_synaptics       = false;
      // If absolute cursor position is stuck (i.e. Dreamfall Chapters) use this
      bool    add_relative_motion = true;
      int     disabled_to_game    =    0; //0 = Never, 1 = Always, 2 = In Background
      bool    ignore_small_clips  = false; // Ignore mouse clipping rects < 75% the
                                          //   dimensions of the client window, so
                                          //     that UI input works.
    } mouse;

    // Avoids calling SK_Input_PreInit (...)
    bool dont_hook_core = false;
  } input;

  struct threads_s {
    bool    enable_mem_alloc_trace   = false;
    bool    enable_file_io_trace     = false;
    bool    enable_dynamic_spinlocks = false;
  } threads;

  struct injection_s {
    struct global_cache_s {
      bool  use_static_addresses = false;
    } global;
  } injection;

  struct window_s {
    bool    borderless          = false;
    bool    border_override     = false;
    bool    center              = false;
    int     zband               = ZBID_DESKTOP;
    struct offset_s {
      struct coordinate_s {
      int   absolute            = 0;
      float percent             = 0.0F;
      } x, y;
      bool isZero (void) noexcept
            { return x.absolute == 0        && y.absolute == 0        &&
                     x.percent  > -0.00001F && x.percent   < 0.00001F &&
                     y.percent  > -0.00001F && y.percent   < 0.00001F; }
    } offset;
    int     always_on_top       =    -1;
    bool    background_render   = false;
    bool    background_mute     = false;
    bool    confine_cursor      = false;
    bool    unconfine_cursor    = false;
    bool    persistent_drag     = false;
    bool    drag_lock           = false; // NOT SAVED IN INI
    bool    fullscreen          = false;
    bool    disable_screensaver = false;
    bool    treat_fg_as_active  = false; // Compat. hack for NiNoKuni 2
    bool    dont_hook_wndproc   = false;
    struct resolution_s {
      struct dim_override_s {
        unsigned int x          = 0;
        unsigned int y          = 0;
        bool         fix_mouse  = false;
        bool isZero (void) noexcept { return x == 0 && y == 0; };
      } override;
    } res;
  } window;

  struct dpi_s {
    struct awareness_s{
      bool   aware                = true;
      bool   aware_on_all_threads = true;
    } per_monitor;
    bool     disable_scaling      = true;
  } dpi;

  struct compatibility_s {
    bool     rehook_loadlibrary       = false;
    bool     disable_nv_bloat         = false;
    bool     init_while_suspended     =  true;
    bool     impersonate_debugger     = false; // Can disable games' crash handlers
    bool     disable_debug_features   = false;
    bool     using_wine               = false;
    bool     allow_dxdiagn            = false;
    bool     auto_large_address_patch =  true;
    bool     init_on_separate_thread  =  true;
  } compatibility;

  struct apis_s {
    struct glide_s {
      bool   hook = true;
    } glide;

#ifdef _M_IX86
    struct legacy_dx_s {
      bool   hook = true;
    } d3d8,
      ddraw;
#endif

    struct d3d9_s {
      bool   hook        =  true;
      bool   translated  = false;
      int    native_dxvk =    -1;
    } d3d9,
      d3d9ex;

    struct dxgi_s {
      struct d3d11or12_s{
        bool hook = true;
      } d3d12,
        d3d11;
    } dxgi;

    struct khronos_s {
      bool   hook      = true;
      bool   translate = true;
    } Vulkan,
      OpenGL;

    struct NvAPI_s {
      bool         enable        = true;
      bool         gsync_status  = true;
      bool         disable_hdr   = false;
      int          vulkan_bridge = -1;
      std::wstring bpc_enum      = L"NV_BPC_DEFAULT";
      std::wstring col_fmt_enum  = L"NV_COLOR_FORMAT_AUTO";
    } NvAPI;

    struct ADL_s {
      bool   enable            = true;
    } ADL;

    SK_RenderAPI last_known    = SK_RenderAPI::Reserved;
  } apis;

  struct system_s {
    std::wstring
            version             = SK_GetVersionStrW ();
    int     log_level           =    0;
    float   global_inject_delay = 0.0F;
#ifdef _DEBUG
    bool    trace_create_thread =  true;
#else
    bool    trace_create_thread = false;
#endif
    bool    trace_load_library  =  true;
    bool    strict_compliance   = false;
    bool    silent              = false;
    bool    handle_crashes      =  true;
    bool    silent_crash        = false;
    bool    suppress_crashes    = false;
    bool    prefer_fahrenheit   = false;
    bool    display_debug_out   = false;
    bool    game_output         =  true;
    bool    central_repository  = false;
    bool    ignore_rtss_delay   = false;
    bool    wait_for_debugger   = false;
    bool    return_to_skif      = false;
  } system;

  struct priority_scheduling_s {
    bool    raise_always        = false;
    bool    raise_bg            = false;
    bool    raise_fg            =  true;
    bool    deny_foreign_change =  true;
  } priority;

  struct skif_s {
    int     auto_stop_behavior  = 1; // 0=Never, 1=AtStart, 2=AtExit
  } skif;

  struct network_s {
    bool    disable_winsock     =  false;
    bool    strict_blocking     =  false; // Block third-party modules from using Winsock?
  } network;
};

template <class T>
class SK_DanglingRef
{
protected:
  typedef T* (*init_fn)(void);
               init_fn init = nullptr;

  T* pPtr = nullptr;

public:
  SK_DanglingRef (void) noexcept
  {
    pPtr = nullptr;
    init = nullptr;
  }

  SK_DanglingRef (T* pRef, init_fn _init) noexcept
  {
    pPtr =  pRef;
    init = _init;
  }

  SK_DanglingRef (init_fn _init) noexcept
  {
    pPtr = nullptr;
    init = _init;
  }

  operator SK_DanglingRef& (void) = delete;
  operator SK_DanglingRef  (void) = delete;


  operator T& ()
  {
    if (pPtr != nullptr)
      return *pPtr;

    else if (init != nullptr)
    {
      pPtr = init ();
    }

    static T _dummy;

    if (pPtr == nullptr)
      return _dummy;

    return *pPtr;
  }

  T* operator -> ()
  {
    if (pPtr != nullptr)
      return pPtr;

    else if (init != nullptr)
    {
      pPtr = init ();
    }

    return pPtr;
  }
};

extern sk_config_t config;

struct SK_KeyCommand
{
  SK_Keybind   binding;
  std::wstring command;
};


bool SK_LoadConfigEx (      std::wstring  name, bool create = true);
bool SK_LoadConfig   (const std::wstring& name              = L"");
bool SK_DeleteConfig (      std::wstring  name              = L"");
void SK_SaveConfig   (      std::wstring  name              = L"",
                      bool                close_config      = false);

//
// Sort of a nasty hack until I come up with a way for plug-ins to
//   formally read the parent DLL's configuration.
//
uint32_t
__cdecl
SK_Config_GetTargetFPS (void);


struct iSK_INI;

typedef uint32_t DepotId_t;
typedef uint64_t ManifestId_t;

struct SK_Steam_DepotManifest {
  struct {
    std::string  name = "";
    DepotId_t    id   =  0;
  } depot;

  struct
  {
    std::string  date = "";
    ManifestId_t id   =  0;
  } manifest;
};

typedef std::vector <SK_Steam_DepotManifest> SK_DepotList;

struct SK_AppCache_Manager
{ enum Ownership
  { Unknown    = -1,
    DoesNotOwn =  0,
    OwnsGame   =  1,
    _Alignment_=  LONG_MAX };

  enum FriendPreference
  { Default    = -1, // Use Global Setting
    Disable    =  0,
    Enable     =  1 };

  bool          saveAppCache           (bool           close = false);
  bool          loadAppCacheForExe     (const wchar_t* wszExe);
  bool          loadAppCacheForEpicApp (const char*    szEpicApp);

  uint64_t      getAppIDFromPath       (const wchar_t* wszPath)   const;
  std::wstring  getAppNameFromID       (uint64_t       uiAppID)   const;
  std::wstring  getAppNameFromPath     (const wchar_t* wszPath)   const;
  std::wstring  getAppNameFromEpicApp  (const char*    szEpicApp) const;

  bool          addAppToCache       ( const wchar_t* wszRelativePath,
                                      const wchar_t* wszExecutable,
                                      const wchar_t* wszAppName,
                                            uint64_t uiAppID );
  bool          addAppToCache       ( const wchar_t* wszRelativePath,
                                      const wchar_t* wszExecutable,
                                      const wchar_t* wszAppName,
                                         const char* szEpicApp );

  std::wstring  getConfigPathFromAppPath (const wchar_t* wszPath)    const; // Steam
  std::wstring  getConfigPathFromCmdLine (const wchar_t* wszCmdLine) const; // Epic
  std::wstring  getConfigPathForAppID    (uint64_t       uiAppID)    const;
  std::wstring  getConfigPathForEpicApp  (const char*    szEpicApp)  const;

  int           migrateProfileData       (LPVOID reserved = nullptr);

  SK_DepotList& getAvailableManifests    (DepotId_t steam_depot);
  ManifestId_t  getInstalledManifest     (DepotId_t steam_depot);
  int           loadDepotCache           (DepotId_t steam_depot = 0);
  int           storeDepotCache          (DepotId_t steam_depot = 0);


  time_t        setFriendOwnership  (uint64_t friend_, Ownership owns             );
  Ownership     getFriendOwnership  (uint64_t friend_, time_t*   updated = nullptr);

  time_t        setFriendAchievPct  (uint64_t friend_, float   percent          );
  float         getFriendAchievPct  (uint64_t friend_, time_t* updated = nullptr);

  void          setFriendPreference (FriendPreference);
  bool          wantFriendStats     (void); // Per-application override

#define SK_LICENSE_REVISION 20190408

  // For open source software license screen, only display when
  //   licensed software is updated.
  int           getLicenseRevision (void);
  void          setLicenseRevision (int);


protected:
  iSK_INI* app_cache_db = nullptr;
};

extern SK_LazyGlobal <SK_AppCache_Manager> app_cache_mgr;

enum class SK_GAME_ID
{
  Launcher,                     // A generic launcher...

  Tyranny,                      // Tyranny.exe
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
  GodEater3,                    // ge3.exe
  WatchDogs2,                   // WatchDogs2.exe
  NieRAutomata,                 // NieRAutomata.exe
  Warframe_x64,                 // Warframe.x64.exe
  LEGOCityUndercover,           // LEGOLCUR_DX11.exe
  Sacred,                       // sacred.exe
  Sacred2,                      // sacred2.exe
  FinalFantasy9,                // FF9.exe
  EdithFinch,                   // FinchGame.exe
  FinalFantasyX_X2,             // FFX.exe / FFX-2.exe / FFX&X-2_Will.exe
  FinalFantasy13,               // ffxiiiimg.exe / FFXiiiLauncher.exe
  DeadlyPremonition,            // DP.exe DPLauncher.exe
  GalGun_Double_Peace,          // GG2Game.exe
  YS_Seven,                     // Ys7.exe
  LifeIsStrange_BeforeTheStorm, // Life is Strange - Before the Storm.exe
  Tales_of_Symphonia,           // TOS.exe
  Tales_of_Zestiria,            // Tales of Zestiria.exe
  Tales_of_Vesperia,            // TOV_DE.exe
  Tales_of_Arise,               // Tales of Arise.exe
  DivinityOriginalSin,          // EoCApp.exe
  Hob,                          // Hob.exe and HobLauncher.exe
  DukeNukemForever,             // DukeForever.exe
  BlueReflection,               // BLUE_REFLECTION.exe
  ZeroEscape,                   // Zero Escape.exe
  DotHackGU,                    // hackGU.exe
  WorldOfFinalFantasy,          // WOFF.exe
  StarOcean4,                   // StarOceanTheLastHope.exe
  LEGOMarvelSuperheroes2,       // LEGOMARVEL2_DX11.exe
  Okami,                        // okami.exe
  DuckTalesRemastered,          // DuckTales.exe
  Mafia3,                       // mafia3.exe
  Owlboy,                       // Owlboy.exe
  DarkSouls3,                   // DarkSoulsIII.exe
  Fallout4,                     // Fallout4.exe
  DisgaeaPC,                    // dis1_st.exe
  SecretOfMana,                 // Secret_of_Mana.exe
  FinalFantasyXV,               // ffxv*.exe
  FinalFantasyXIV,              // ffxiv_dx11.exe
  DragonBallFighterZ,           // DBFighterZ.exe
  NiNoKuni2,                    // Nino2.exe
  FarCry5,                      // FarCry5.exe
  ChronoTrigger,                // Chrono Trigger.exe
  Ys_Eight,                     // ys8.exe
  PillarsOfEternity2,           // PillarsOfEternityII.exe
  Yakuza0,                      // Yakuza0.exe
  YakuzaKiwami,                 // YakuzaKiwami.exe
  YakuzaKiwami2,                // YakuzaKiwami2.exe
  YakuzaUnderflow,              // Yakuza*.exe
  MonsterHunterWorld,           // MonsterHunterWorld.exe
  Shenmue,                      // Shenmue.exe
  DragonQuestXI,                // DRAGON QUEST XI.exe
  AssassinsCreed_Odyssey,       // ACOdyssey.exe
  JustCause3,                   // JustCause3.exe
  CallOfCthulhu,                // CallOfCthulhu.exe
  TrailsOfColdSteel,            // ed8.exe
  Sekiro,                       // sekiro.exe
  OctopathTraveler,             // Octopath_Traveler-Win64-Shipping.exe
  SonicMania,                   // SonicMania.exe
  Persona4,                     // P4G.exe
  Persona5,                     // P5R.exe
  HorizonZeroDawn,              // HorizonZeroDawn.exe
  BaldursGate3,                 // bg3.exe
  AssassinsCreed_Valhalla,      // ACValhalla.exe / ACValhalla_Plus.exe
  Cyberpunk2077,                // Cyberpunk2077.exe
  AtelierRyza2,                 // Atelier_Ryza_2.exe
  Nioh2,                        // nioh2.exe
  HuniePop2,                    // HuniePop 2 - Double Date.exe
  GalGunReturns,                // GalGun Returns/game.exe
  Persona5Strikers,             // P5S/game.exe
  NieR_Sqrt_1_5,                // NieR Replicant ver.1.22474487139.exe
  ResidentEvil8,                // re8.exe
  LegendOfMana,                 // Legend Of Mana.exe
  MonsterHunterStories2,        // game.exe (fantastic),
  FarCry6,                      // FarCry6.exe
  Ryujinx,                      // Ryujinx.exe
  yuzu,                         // yuzu.exe
  cemu,                         // cemu.exe
  RPCS3,                        // rpcs3.exe
  ForzaHorizon5,                // ForzaHorizon5.exe
  HaloInfinite,                 // HaloInfinite.exe
  FinalFantasy7Remake,          // ff7remake*.exe
  DyingLight2,                  // DyingLightGame_x64_rwdi.exe
  EasyAntiCheat,                // start_protected_game.exe
  EldenRing,                    // eldenring.exe
  TinyTinasWonderlands,         // Wonderlands.exe
  Elex2,                        // ELEX2.exe
  ChronoCross,                  // CHRONOCROSS.exe
  HatsuneMikuDIVAMegaMix,       // DivaMegaMix.exe
  ShinMegamiTensei3,            // smt3hd.exe
  TheQuarry,                    // TheQuarry-Win64-Shipping.exe
  GenshinImpact,                // GenshinImpact.exe
  PathOfExile,                  // PathOfExileSteam.exe
  Disgaea5,                     // Disgaea5.exe
  SoulHackers2,                 // SOUL HACKERS2.exe
  MegaManBattleNetwork,         // MMBN_LC2.exe, MMBN_LC1.exe
  HonkaiStarRail,               // StarRail.exe
  NoMansSky,                    // NMS.exe
  DiabloIV,                     // Diablo IV.exe
  CallOfDuty,                   // CoDSP.exe, CoDMP.exe (???)

  UNKNOWN_GAME               = 0xffff
};

SK_GAME_ID
__stdcall
SK_GetCurrentGameID (void);

const wchar_t*
__stdcall
SK_GetConfigPath (void);

const wchar_t*
__stdcall
SK_GetConfigPathEx (bool reset = false);

const wchar_t*
__stdcall
SK_GetNaiveConfigPath (void);

extern const wchar_t*
SK_GetFullyQualifiedApp (void);

const wchar_t*
__stdcall
SK_GetVersionStr (void) noexcept;

void
WINAPI
SK_Resource_SetRoot (const wchar_t* root);

std::filesystem::path
SK_Resource_GetRoot (void);

extern bool
SK_ImGui_KeybindSelect (SK_Keybind* keybind, const char* szLabel);

extern SK_API void
__stdcall
SK_ImGui_KeybindDialog (SK_Keybind* keybind);

using wstring_hash = size_t;

extern SK_LazyGlobal <std::unordered_map <wstring_hash, BYTE>>           humanKeyNameToVirtKeyCode;
extern SK_LazyGlobal <std::unordered_map <BYTE, wchar_t [32]>>           virtKeyCodeToHumanKeyName;
extern SK_LazyGlobal <std::unordered_map <BYTE, wchar_t [32]>>  virtKeyCodeToFullyLocalizedKeyName;
extern SK_LazyGlobal <std::unordered_multimap <uint32_t, SK_KeyCommand>> SK_KeyboardMacros;

#endif /* __SK__CONFIG_H__ */