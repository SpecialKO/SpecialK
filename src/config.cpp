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

#include <SpecialK/config.h>
#include <SpecialK/core.h>
#include <SpecialK/dxgi_interfaces.h>
#include <SpecialK/parameter.h>
#include <SpecialK/import.h>
#include <SpecialK/utility.h>
#include <SpecialK/ini.h>
#include <SpecialK/log.h>
#include <SpecialK/steam_api.h>
#include <SpecialK/nvapi.h>

#include <SpecialK/DLL_VERSION.H>
#include <SpecialK/input/input.h>
#include <SpecialK/widgets/widget.h>

#include <unordered_map>

#include <Shlwapi.h>

#define D3D11_RAISE_FLAG_DRIVER_INTERNAL_ERROR 1

const wchar_t*       SK_VER_STR = SK_VERSION_STR_W;

iSK_INI*             dll_ini         = nullptr;
iSK_INI*             osd_ini         = nullptr;
iSK_INI*             achievement_ini = nullptr;
sk_config_t          config;
sk::ParameterFactory g_ParameterFactory;


static std::unordered_map <std::wstring, SK_GAME_ID> games;

SK_GAME_ID
__stdcall
SK_GetCurrentGameID (void)
{
  static SK_GAME_ID current_game =
    games.count (SK_GetHostApp ()) ?
          games [SK_GetHostApp ()] :
          SK_GAME_ID::UNKNOWN_GAME;

  return current_game;
}


struct {
  struct {
    sk::ParameterBool*    show;
  } time;

  struct {
    sk::ParameterBool*    show;
    sk::ParameterFloat*   interval;
  } io;

  struct {
    sk::ParameterBool*    show;
  } fps;

  struct {
    sk::ParameterBool*    show;
  } memory;

  struct {
    sk::ParameterBool*    show;
  } SLI;

  struct {
    sk::ParameterBool*    show;
    sk::ParameterFloat*   interval;
    sk::ParameterBool*    simple;
  } cpu;

  struct {
    sk::ParameterBool*    show;
    sk::ParameterBool*    print_slowdown;
    sk::ParameterFloat*   interval;
  } gpu;

  struct {
    sk::ParameterBool*    show;
    sk::ParameterFloat*   interval;
    sk::ParameterInt*     type;
  } disk;

  struct {
    sk::ParameterBool*    show;
    sk::ParameterFloat*   interval;
  } pagefile;
} monitoring;

struct {
  sk::ParameterBool*      show;

  struct {
    sk::ParameterBool*    pump;
    sk::ParameterFloat*   pump_interval;
  } update_method;

  struct {
    sk::ParameterInt*     red;
    sk::ParameterInt*     green;
    sk::ParameterInt*     blue;
  } text;

  struct {
    sk::ParameterFloat*   scale;
    sk::ParameterInt*     pos_x;
    sk::ParameterInt*     pos_y;
  } viewport;

  struct {
    sk::ParameterBool*    remember;
  } state;
} osd;

struct {
  sk::ParameterFloat*     scale;
  sk::ParameterBool*      show_eula;
  sk::ParameterBool*      show_playtime;
  sk::ParameterBool*      mac_style_menu;
  sk::ParameterBool*      show_gsync_status;
  sk::ParameterBool*      show_input_apis;
} imgui;

struct {
  struct {
    sk::ParameterStringW*   sound_file;
    sk::ParameterBool*      play_sound;
    sk::ParameterBool*      take_screenshot;

    struct {
      sk::ParameterBool*    show;
      sk::ParameterBool*    show_title;
      sk::ParameterBool*    animate;
      sk::ParameterStringW* origin;
      sk::ParameterFloat*   inset;
      sk::ParameterInt*     duration;
    } popup;
  } achievements;

  struct {
    sk::ParameterInt*     appid;
    sk::ParameterInt*     init_delay;
    sk::ParameterBool*    auto_pump;
    sk::ParameterStringW* notify_corner;
    sk::ParameterBool*    block_stat_callback;
    sk::ParameterBool*    filter_stat_callbacks;
    sk::ParameterBool*    load_early;
    sk::ParameterBool*    early_overlay;
    sk::ParameterBool*    force_load;
  } system;

  struct {
    sk::ParameterBool*    silent;
  } log;

  struct
  {
    sk::ParameterBool*    spoof_BLoggedOn;
  } drm;
} steam;

struct {
  struct {
    sk::ParameterBool*    override;
    sk::ParameterStringW* compatibility;
    sk::ParameterStringW* num_gpus;
    sk::ParameterStringW* mode;
  } sli;

  struct {
    sk::ParameterBool*    disable;
  } api;
} nvidia;

sk::ParameterBool*        enable_cegui;
sk::ParameterBool*        safe_cegui;
sk::ParameterFloat*       mem_reserve;
sk::ParameterBool*        debug_output;
sk::ParameterBool*        game_output;
sk::ParameterBool*        handle_crashes;
sk::ParameterBool*        prefer_fahrenheit;
sk::ParameterBool*        ignore_rtss_delay;
sk::ParameterInt*         init_delay;
sk::ParameterInt*         log_level;
sk::ParameterBool*        trace_libraries;
sk::ParameterBool*        strict_compliance;
sk::ParameterBool*        silent;
sk::ParameterStringW*     version;

struct {
  struct {
    sk::ParameterFloat*   target_fps;
    sk::ParameterFloat*   limiter_tolerance;
    sk::ParameterInt*     prerender_limit;
    sk::ParameterInt*     present_interval;
    sk::ParameterInt*     buffer_count;
    sk::ParameterInt*     max_delta_time;
    sk::ParameterBool*    flip_discard;
    sk::ParameterInt*     refresh_rate;
    sk::ParameterBool*    wait_for_vblank;
    sk::ParameterBool*    allow_dwm_tearing;
    sk::ParameterBool*    sleepless_window;
    sk::ParameterBool*    sleepless_render;
  } framerate;
  struct {
    sk::ParameterInt*     adapter_override;
    sk::ParameterStringW* max_res;
    sk::ParameterStringW* min_res;
    sk::ParameterInt*     swapchain_wait;
    sk::ParameterStringW* scaling_mode;
    sk::ParameterStringW* exception_mode;
    sk::ParameterStringW* scanline_order;
    sk::ParameterStringW* rotation;
    sk::ParameterBool*    test_present;
    sk::ParameterBool*    debug_layer;
    sk::ParameterBool*    safe_fullscreen;
    sk::ParameterBool*    enhanced_depth;
  } dxgi;
  struct {
    sk::ParameterBool*    force_d3d9ex;
    sk::ParameterInt*     hook_type;
    sk::ParameterBool*    impure;
    sk::ParameterBool*    enable_texture_mods;
  } d3d9;
} render;

struct {
  sk::ParameterBool*      force_fullscreen;
  sk::ParameterBool*      force_windowed;
} display;

struct {
  struct {
    sk::ParameterBool*    precise_hash;
    sk::ParameterBool*    dump;
    sk::ParameterBool*    inject;
    sk::ParameterBool*    cache;
    sk::ParameterStringW* res_root;
  } d3d11;
  struct {
    sk::ParameterInt*     min_evict;
    sk::ParameterInt*     max_evict;
    sk::ParameterInt*     min_size;
    sk::ParameterInt*     max_size;
    sk::ParameterInt*     min_entries;
    sk::ParameterInt*     max_entries;
    sk::ParameterBool*    ignore_non_mipped;
    sk::ParameterBool*    allow_staging;
  } cache;
    sk::ParameterStringW* res_root;
    sk::ParameterBool*    dump_on_load;
} texture;

struct {
  struct {
    sk::ParameterBool*    manage;
    sk::ParameterBool*    keys_activate;
    sk::ParameterFloat*   timeout;
    sk::ParameterBool*    ui_capture;
    sk::ParameterBool*    hw_cursor;
    sk::ParameterBool*    no_warp_ui;
    sk::ParameterBool*    no_warp_visible;
    sk::ParameterBool*    block_invisible;
    sk::ParameterBool*    fix_synaptics;
    sk::ParameterBool*    use_relative_input;
    sk::ParameterFloat*   antiwarp_deadzone;
  } cursor;

  struct {
    sk::ParameterBool*    disable_ps4_hid;
    sk::ParameterBool*    rehook_xinput;
    sk::ParameterBool*    haptic_ui;
    sk::ParameterBool*    hook_dinput8;
    sk::ParameterBool*    hook_hid;
    sk::ParameterBool*    hook_xinput;

    struct {
      sk::ParameterInt*     ui_slot;
      sk::ParameterInt*     placeholders;
      sk::ParameterBool*    disable_rumble;
      sk::ParameterStringW* assignment;
    } xinput;

    sk::ParameterBool*   native_ps4;
  } gamepad;
} input;

struct {
  sk::ParameterBool*      borderless;
  sk::ParameterBool*      center;
  struct {
    sk::ParameterStringW* x;
    sk::ParameterStringW* y;
  } offset;
  sk::ParameterBool*      background_render;
  sk::ParameterBool*      background_mute;
  sk::ParameterBool*      confine_cursor;
  sk::ParameterBool*      unconfine_cursor;
  sk::ParameterBool*      persistent_drag;
  sk::ParameterBool*      fullscreen;
  sk::ParameterStringW*   override;
  sk::ParameterBool*      fix_mouse_coords;
} window;

struct {
  sk::ParameterBool*      ignore_raptr;
  sk::ParameterBool*      disable_raptr;
  sk::ParameterBool*      rehook_loadlibrary;
  sk::ParameterBool*      disable_nv_bloat;

  struct {
    sk::ParameterBool*    rehook_reset;
    sk::ParameterBool*    rehook_present;
    sk::ParameterBool*    hook_reset_vtable;
    sk::ParameterBool*    hook_present_vtable;
  } d3d9;
} compatibility;

struct {
  struct {
    sk::ParameterBool*    hook;
  }   
#ifndef _WIN64
      ddraw, d3d8,
#endif
      d3d9,  d3d9ex,
      d3d11, 
#ifdef _WIN64
      d3d12,
      Vulkan,
#endif
      OpenGL;

  sk::ParameterInt*       last_known;
} apis;

bool
SK_LoadConfig (std::wstring name) {
  return SK_LoadConfigEx (name);
}


SK_AppCache_Manager app_cache_mgr;

__declspec (noinline)
const wchar_t*
__stdcall
SK_GetConfigPath (void)
{
  static bool init = false;

  if (! init)
  {
    app_cache_mgr.loadAppCacheForExe (SK_GetFullyQualifiedApp ());
    init = true;
  }

  static std::wstring path =
    app_cache_mgr.getConfigPathFromAppPath (SK_GetFullyQualifiedApp ());

  return path.c_str ();
}


bool
SK_LoadConfigEx (std::wstring name, bool create)
{
  // Load INI File
  std::wstring full_name;
  std::wstring osd_config;
  std::wstring achievement_config;


  full_name = SK_GetConfigPath () +
                name              +
                  L".ini";

  if (create)
    SK_CreateDirectories (full_name.c_str ());

  static bool         init     = false;
  static bool         empty    = true;
  static std::wstring last_try = name;

  // Allow a second load attempt using a different name
  if (last_try != name)
  {
    init     = false;
    last_try = name;
  }

  osd_config =
    SK_GetDocumentsDir () + L"\\My Mods\\SpecialK\\Global\\osd.ini";

  achievement_config =
    SK_GetDocumentsDir () + L"\\My Mods\\SpecialK\\Global\\achievements.ini";

  
  if (! init)
  {
   dll_ini =
    SK_CreateINI (full_name.c_str ());

  empty    = dll_ini->get_sections ().empty ();

  SK_CreateDirectories (osd_config.c_str ());

  osd_ini =
    SK_CreateINI (osd_config.c_str ());

  achievement_ini =
    SK_CreateINI (achievement_config.c_str ());

  //
  // Create Parameters
  //
  monitoring.io.show =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (L"Show IO Monitoring"));
  monitoring.io.show->register_to_ini (osd_ini, L"Monitor.IO", L"Show");

  monitoring.io.interval =
    dynamic_cast <sk::ParameterFloat *>
     (g_ParameterFactory.create_parameter <float> (L"IO Monitoring Interval"));
  monitoring.io.interval->register_to_ini (osd_ini, L"Monitor.IO", L"Interval");

  monitoring.disk.show =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (L"Show Disk Monitoring"));
  monitoring.disk.show->register_to_ini (osd_ini, L"Monitor.Disk", L"Show");

  monitoring.disk.interval =
    dynamic_cast <sk::ParameterFloat *>
     (g_ParameterFactory.create_parameter <float> (
       L"Disk Monitoring Interval")
     );
  monitoring.disk.interval->register_to_ini (
    osd_ini,
      L"Monitor.Disk",
        L"Interval" );

  monitoring.disk.type =
    dynamic_cast <sk::ParameterInt *>
     (g_ParameterFactory.create_parameter <int> (
       L"Disk Monitoring Type (0 = Physical, 1 = Logical)")
     );
  monitoring.disk.type->register_to_ini (
    osd_ini,
      L"Monitor.Disk",
        L"Type" );


  monitoring.cpu.show =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (L"Show CPU Monitoring"));
  monitoring.cpu.show->register_to_ini (osd_ini, L"Monitor.CPU", L"Show");

  monitoring.cpu.interval =
    dynamic_cast <sk::ParameterFloat *>
     (g_ParameterFactory.create_parameter <float> (
       L"CPU Monitoring Interval (seconds)")
     );
  monitoring.cpu.interval->register_to_ini (
    osd_ini,
      L"Monitor.CPU",
        L"Interval" );

  monitoring.cpu.simple =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (L"Minimal CPU info"));
  monitoring.cpu.simple->register_to_ini (osd_ini, L"Monitor.CPU", L"Simple");

  monitoring.gpu.show =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (L"Show GPU Monitoring"));
  monitoring.gpu.show->register_to_ini (osd_ini, L"Monitor.GPU", L"Show");

  monitoring.gpu.print_slowdown =
    dynamic_cast <sk::ParameterBool *>
    (g_ParameterFactory.create_parameter <bool>(L"Print GPU Slowdown Reason"));
  monitoring.gpu.print_slowdown->register_to_ini (
    osd_ini,
      L"Monitor.GPU",
        L"PrintSlowdown" );

  monitoring.gpu.interval =
    dynamic_cast <sk::ParameterFloat *>
     (g_ParameterFactory.create_parameter <float> (
       L"GPU Monitoring Interval (seconds)")
     );
  monitoring.gpu.interval->register_to_ini (
    osd_ini,
      L"Monitor.GPU",
        L"Interval" );


  monitoring.pagefile.show =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Show Pagefile Monitoring")
      );
  monitoring.pagefile.show->register_to_ini (
    osd_ini,
      L"Monitor.Pagefile",
        L"Show" );

  monitoring.pagefile.interval =
    dynamic_cast <sk::ParameterFloat *>
     (g_ParameterFactory.create_parameter <float> (
       L"Pagefile Monitoring Interval (seconds)")
     );
  monitoring.pagefile.interval->register_to_ini (
    osd_ini,
      L"Monitor.Pagefile",
        L"Interval" );


  monitoring.memory.show =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Show Memory Monitoring")
      );
  monitoring.memory.show->register_to_ini (
    osd_ini,
      L"Monitor.Memory",
        L"Show" );


  monitoring.fps.show =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Show Framerate Monitoring")
      );
  monitoring.fps.show->register_to_ini (
    osd_ini,
      L"Monitor.FPS",
        L"Show" );


  monitoring.time.show =
    dynamic_cast <sk::ParameterBool *>
    (g_ParameterFactory.create_parameter <bool> (
      L"Show Time")
    );
  monitoring.time.show->register_to_ini (
    osd_ini,
      L"Monitor.Time",
        L"Show" );


  input.cursor.manage =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Manage Cursor Visibility")
      );
  input.cursor.manage->register_to_ini (
    dll_ini,
      L"Input.Cursor",
        L"Manage" );

  input.cursor.keys_activate =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Keyboard Input Activates Cursor")
      );
  input.cursor.keys_activate->register_to_ini (
    dll_ini,
      L"Input.Cursor",
        L"KeyboardActivates" );

  input.cursor.timeout =
    dynamic_cast <sk::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"Hide Delay")
      );
  input.cursor.timeout->register_to_ini (
    dll_ini,
      L"Input.Cursor",
        L"Timeout" );

  input.cursor.ui_capture =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Forcefully Capture Mouse Cursor in UI Mode")
      );
  input.cursor.ui_capture->register_to_ini (
    dll_ini,
      L"Input.Cursor",
        L"ForceCaptureInUI" );

  input.cursor.hw_cursor =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Use a Hardware Cursor for Special K's UI Features")
      );
  input.cursor.hw_cursor->register_to_ini (
    dll_ini,
      L"Input.Cursor",
        L"UseHardwareCursor" );

  input.cursor.block_invisible =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Block Mouse Input if Hardware Cursor is Invisible")
      );
  input.cursor.block_invisible->register_to_ini (
    dll_ini,
      L"Input.Cursor",
        L"BlockInvisibleCursorInput"
  );

  input.cursor.fix_synaptics =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Fix Synaptic Touchpad Scroll")
      );
  input.cursor.fix_synaptics->register_to_ini (
    dll_ini,
      L"Input.Cursor",
        L"FixSynapticTouchpadScroll"
  );

  input.cursor.use_relative_input =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Use Raw Input Relative Motion if Needed")
      );
  input.cursor.use_relative_input->register_to_ini (
    dll_ini,
      L"Input.Cursor",
        L"UseRelativeInput"
  );

  input.cursor.antiwarp_deadzone =
    dynamic_cast <sk::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"Percentage of Screen that the game may try to "
        L"move the cursor to for mouselook.")
      );
  input.cursor.antiwarp_deadzone->register_to_ini (
    dll_ini,
      L"Input.Cursor",
        L"AntiwarpDeadzonePercent"
  );

  input.cursor.no_warp_ui =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Prevent Games from Warping Cursor while Config UI is Open")
      );
  input.cursor.no_warp_ui->register_to_ini(
    dll_ini,
      L"Input.Cursor",
        L"NoWarpUI"
  );

  input.cursor.no_warp_visible =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Prevent Games from Warping Cursor while Mouse Cursor is Visible.")
      );
  input.cursor.no_warp_visible->register_to_ini(
    dll_ini,
      L"Input.Cursor",
        L"NoWarpVisibleGameCursor"
  );


  input.gamepad.disable_ps4_hid =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Disable PS4 HID Interface (prevent double-input processing in some games)")
      );
  input.gamepad.disable_ps4_hid->register_to_ini (
    dll_ini,
      L"Input.Gamepad",
        L"DisablePS4HID"
  );

  input.gamepad.haptic_ui =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Give tactile feedback on gamepads when navigating the UI")
      );
  input.gamepad.haptic_ui->register_to_ini (
    dll_ini,
      L"Input.Gamepad",
        L"AllowHapticUI" );

  input.gamepad.hook_dinput8 =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Install hooks for DirectInput 8")
      );
  input.gamepad.hook_dinput8->register_to_ini (
    dll_ini,
      L"Input.Gamepad",
        L"EnableDirectInput8" );

  input.gamepad.hook_hid =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Install hooks for HID")
      );
  input.gamepad.hook_hid->register_to_ini (
    dll_ini,
      L"Input.Gamepad",
        L"EnableHID" );

  input.gamepad.native_ps4 =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Native PS4 Mode (temporary)")
      );
  input.gamepad.native_ps4->register_to_ini (
    dll_ini,
      L"Input.Gamepad",
        L"EnableNativePS4" );


  input.gamepad.hook_xinput =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Install hooks for XInput")
      );
  input.gamepad.hook_xinput->register_to_ini (
    dll_ini,
      L"Input.XInput",
        L"Enable" );

  input.gamepad.rehook_xinput =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Re-install XInput hooks if the hook-chain is modified (wrapper compat).")
      );
  input.gamepad.rehook_xinput->register_to_ini (
    dll_ini,
      L"Input.XInput",
        L"Rehook" );

  input.gamepad.xinput.ui_slot =
    dynamic_cast <sk::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"XInput Controller that owns the config UI.")
      );
  input.gamepad.xinput.ui_slot->register_to_ini (
    dll_ini,
      L"Input.XInput",
        L"UISlot" );

  input.gamepad.xinput.placeholders =
    dynamic_cast <sk::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"XInput Controllers to Fake.")
      );
  input.gamepad.xinput.placeholders->register_to_ini (
    dll_ini,
      L"Input.XInput",
        L"PlaceholderMask" );

  input.gamepad.xinput.disable_rumble =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Disable Rumble")
      );
  input.gamepad.xinput.disable_rumble->register_to_ini (
    dll_ini,
      L"Input.XInput",
        L"DisableRumble" );

  input.gamepad.xinput.assignment =
    dynamic_cast <sk::ParameterStringW *>
    ( g_ParameterFactory.create_parameter <std::wstring> (
      L"Re-Assign XInput Slots")
      );
  input.gamepad.xinput.assignment->register_to_ini (
    dll_ini,
      L"Input.XInput",
        L"SlotReassignment");


  window.borderless =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Borderless Window Mode")
      );
  window.borderless->register_to_ini (
    dll_ini,
      L"Window.System",
        L"Borderless" );

  window.center =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Center the Window")
      );
  window.center->register_to_ini (
    dll_ini,
      L"Window.System",
        L"Center" );

  window.background_render =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Render While Window is in Background")
      );
  window.background_render->register_to_ini (
    dll_ini,
      L"Window.System",
        L"RenderInBackground" );

  window.background_mute =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Mute While Window is in Background")
      );
  window.background_mute->register_to_ini (
    dll_ini,
      L"Window.System",
        L"MuteInBackground" );

  window.offset.x =
    dynamic_cast <sk::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"X Offset (Percent or Absolute)")
      );
  window.offset.x->register_to_ini (
    dll_ini,
      L"Window.System",
        L"XOffset" );

  window.offset.y =
    dynamic_cast <sk::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"Y Offset")
      );
  window.offset.y->register_to_ini (
    dll_ini,
      L"Window.System",
        L"YOffset" );

  window.confine_cursor =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Confine the Mouse Cursor to the Game Window.")
      );
  window.confine_cursor->register_to_ini (
    dll_ini,
      L"Window.System",
        L"ConfineCursor" );

  window.unconfine_cursor =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Unconfine the Mouse Cursor from the Game Window.")
      );
  window.unconfine_cursor->register_to_ini (
    dll_ini,
      L"Window.System",
        L"UnconfineCursor" );

  window.persistent_drag =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Remember where the window is dragged to.")
      );
  window.persistent_drag->register_to_ini (
    dll_ini,
      L"Window.System",
        L"PersistentDragPos" );

  window.fullscreen =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Make the Game Window Fill the Screen (scale to fit)")
      );
  window.fullscreen->register_to_ini (
    dll_ini,
      L"Window.System",
        L"Fullscreen" );

  window.override =
    dynamic_cast <sk::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"Force the Client Region to this Size in Windowed Mode")
      );
  window.override->register_to_ini (
    dll_ini,
      L"Window.System",
        L"OverrideRes" );

  window.fix_mouse_coords =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Re-Compute Mouse Coordinates for Resized Windows")
      );
  window.fix_mouse_coords->register_to_ini (
    dll_ini,
      L"Window.System",
        L"FixMouseCoords" );



  compatibility.ignore_raptr =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Ignore Raptr Warning")
      );
  compatibility.ignore_raptr->register_to_ini (
    dll_ini,
      L"Compatibility.General",
        L"IgnoreRaptr" );

  compatibility.disable_raptr =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Forcefully Disable Raptr")
      );
  compatibility.disable_raptr->register_to_ini (
    dll_ini,
      L"Compatibility.General",
        L"DisableRaptr" );

  compatibility.disable_nv_bloat =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Disable All NVIDIA BloatWare (GeForce Experience)")
      );
  compatibility.disable_nv_bloat->register_to_ini (
    dll_ini,
      L"Compatibility.General",
        L"DisableBloatWare_NVIDIA" );

  compatibility.rehook_loadlibrary =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Rehook LoadLibrary When RTSS/Steam/GeDoSaTo hook it")
      );
  compatibility.rehook_loadlibrary->register_to_ini (
    dll_ini,
      L"Compatibility.General",
        L"RehookLoadLibrary" );


  apis.last_known =
    dynamic_cast <sk::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Last Known Render API")
      );
  apis.last_known->register_to_ini (
    dll_ini,
      L"API.Hook",
        L"LastKnown" );

#ifndef _WIN64
  apis.ddraw.hook =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Enable DirectDraw Hooking")
      );
  apis.ddraw.hook->register_to_ini (
    dll_ini,
      L"API.Hook",
        L"ddraw" );

  apis.d3d8.hook =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Enable D3D8 Hooking")
      );
  apis.d3d8.hook->register_to_ini (
    dll_ini,
      L"API.Hook",
        L"d3d8" );
#endif

  apis.d3d9.hook =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Enable D3D9 Hooking")
      );
  apis.d3d9.hook->register_to_ini (
    dll_ini,
      L"API.Hook",
        L"d3d9" );

  apis.d3d9ex.hook =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Enable D3D9Ex Hooking")
      );
  apis.d3d9ex.hook->register_to_ini (
    dll_ini,
      L"API.Hook",
        L"d3d9ex" );

  apis.d3d11.hook =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Enable D3D11 Hooking")
      );
  apis.d3d11.hook->register_to_ini (
    dll_ini,
      L"API.Hook",
        L"d3d11" );

#ifdef _WIN64
  apis.d3d12.hook =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Enable D3D11 Hooking")
      );
  apis.d3d12.hook->register_to_ini (
    dll_ini,
      L"API.Hook",
        L"d3d12" );

  apis.Vulkan.hook =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Enable Vulkan Hooking")
      );
  apis.Vulkan.hook->register_to_ini (
    dll_ini,
      L"API.Hook",
        L"Vulkan" );
#endif

  apis.OpenGL.hook =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Enable OpenGL Hooking")
      );
  apis.OpenGL.hook->register_to_ini (
    dll_ini,
      L"API.Hook",
        L"OpenGL" );


  mem_reserve =
    dynamic_cast <sk::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"Memory Reserve Percentage")
      );
  mem_reserve->register_to_ini (
    dll_ini,
      L"Manage.Memory",
        L"ReservePercent" );


  init_delay =
    dynamic_cast <sk::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Initialization Delay (msecs)")
      );
  init_delay->register_to_ini (
    dll_ini,
      L"SpecialK.System",
        L"InitDelay" );

  silent =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Log Silence")
      );
  silent->register_to_ini (
    dll_ini,
      L"SpecialK.System",
        L"Silent" );

  strict_compliance =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Strict DLL Loader Compliance")
      );
  strict_compliance->register_to_ini (
    dll_ini,
      L"SpecialK.System",
        L"StrictCompliant" );

  trace_libraries =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Trace DLL Loading")
      );
  trace_libraries->register_to_ini (
    dll_ini,
      L"SpecialK.System",
        L"TraceLoadLibrary" );

  log_level =
    dynamic_cast <sk::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Log Verbosity")
      );
  log_level->register_to_ini (
    dll_ini,
      L"SpecialK.System",
        L"LogLevel" );

  prefer_fahrenheit =
    dynamic_cast <sk::ParameterBool *>
    (g_ParameterFactory.create_parameter <bool> (
      L"Prefer Fahrenheit Units")
      );
  prefer_fahrenheit->register_to_ini (
    osd_ini,
      L"SpecialK.OSD",
        L"PreferFahrenheit" );

  handle_crashes =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Use Custom Crash Handler")
      );
  handle_crashes->register_to_ini (
    dll_ini,
      L"SpecialK.System",
        L"UseCrashHandler" );

  debug_output =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Print Application's Debug Output in real-time")
      );
  debug_output->register_to_ini (
    dll_ini,
      L"SpecialK.System",
        L"DebugOutput" );

  game_output =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Log Application's Debug Output")
      );
  game_output->register_to_ini (
    dll_ini,
      L"SpecialK.System",
        L"GameOutput" );


  ignore_rtss_delay =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Ignore RTSS Delay Incompatibilities")
      );
  ignore_rtss_delay->register_to_ini (
    dll_ini,
      L"SpecialK.System",
        L"IgnoreRTSSHookDelay" );

  enable_cegui =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Enable CEGUI")
      );
  enable_cegui->register_to_ini (
    dll_ini,
      L"SpecialK.System",
        L"EnableCEGUI" );

  safe_cegui =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Safely Initialize CEGUI")
      );
  safe_cegui->register_to_ini (
    dll_ini,
      L"SpecialK.System",
        L"SafeInitCEGUI" );

  version =
    dynamic_cast <sk::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"Software Version")
      );
  version->register_to_ini (
    dll_ini,
      L"SpecialK.System",
        L"Version" );


  display.force_fullscreen =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Force Fullscreen Mode")
      );
  display.force_fullscreen->register_to_ini (
    dll_ini,
      L"Display.Output",
        L"ForceFullscreen" );

  display.force_windowed =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Force Windowed Mode")
      );
  display.force_windowed->register_to_ini (
    dll_ini,
      L"Display.Output",
        L"ForceWindowed" );


  render.framerate.target_fps =
    dynamic_cast <sk::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"Framerate Target")
      );
  render.framerate.target_fps->register_to_ini (
    dll_ini,
      L"Render.FrameRate",
        L"TargetFPS" );

  render.framerate.limiter_tolerance =
    dynamic_cast <sk::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"Limiter Tolerance")
      );
  render.framerate.limiter_tolerance->register_to_ini (
    dll_ini,
      L"Render.FrameRate",
        L"LimiterTolerance" );

  render.framerate.wait_for_vblank =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Limiter Waits for VBLANK?")
      );
  render.framerate.wait_for_vblank->register_to_ini (
    dll_ini,
      L"Render.FrameRate",
        L"WaitForVBLANK" );

  render.framerate.buffer_count =
    dynamic_cast <sk::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Number of BackBuffers in the SwapChain")
      );
  render.framerate.buffer_count->register_to_ini (
    dll_ini,
      L"Render.FrameRate",
        L"BackBufferCount" );

  render.framerate.present_interval =
    dynamic_cast <sk::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Presentation Interval")
      );
  render.framerate.present_interval->register_to_ini (
    dll_ini,
      L"Render.FrameRate",
        L"PresentationInterval" );

  render.framerate.prerender_limit =
    dynamic_cast <sk::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Maximum Frames to Render-Ahead")
      );
  render.framerate.prerender_limit->register_to_ini (
    dll_ini,
      L"Render.FrameRate",
        L"PreRenderLimit" );


  // D3D9 / DXGI
    render.framerate.refresh_rate =
      dynamic_cast <sk::ParameterInt *>
        (g_ParameterFactory.create_parameter <int> (
          L"Fullscreen Refresh Rate")
        );
    render.framerate.refresh_rate->register_to_ini (
      dll_ini,
        L"Render.FrameRate",
          L"RefreshRate" );


  render.framerate.allow_dwm_tearing =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Enable DWM Tearing (Windows 10+)")
      );
  render.framerate.allow_dwm_tearing->register_to_ini (
    dll_ini,
      L"Render.DXGI",
        L"AllowTearingInDWM" );

  render.framerate.sleepless_render =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Sleep Free Render Thread")
      );
  render.framerate.sleepless_render->register_to_ini (
    dll_ini,
      L"Render.FrameRate",
        L"SleeplessRenderThread" );

  render.framerate.sleepless_window =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Sleep Free Window Thread")
      );
  render.framerate.sleepless_window->register_to_ini (
    dll_ini,
      L"Render.FrameRate",
        L"SleeplessWindowThread" );


  // D3D9
    compatibility.d3d9.rehook_present =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory.create_parameter <bool> (
          L"Rehook D3D9 Present On Device Reset")
        );
    compatibility.d3d9.rehook_present->register_to_ini (
      dll_ini,
        L"Compatibility.D3D9",
          L"RehookPresent" );
    compatibility.d3d9.rehook_reset =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory.create_parameter <bool> (
          L"Rehook D3D9 Reset On Device Reset")
        );
    compatibility.d3d9.rehook_reset->register_to_ini (
      dll_ini,
        L"Compatibility.D3D9",
          L"RehookReset" );

    compatibility.d3d9.hook_present_vtable =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory.create_parameter <bool> (
          L"Use VFtable Override for Present")
        );
    compatibility.d3d9.hook_present_vtable->register_to_ini (
      dll_ini,
        L"Compatibility.D3D9",
          L"UseVFTableForPresent" );
    compatibility.d3d9.hook_reset_vtable =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory.create_parameter <bool> (
          L"Use VFtable Override for Reset")
        );
    compatibility.d3d9.hook_reset_vtable->register_to_ini (
      dll_ini,
        L"Compatibility.D3D9",
          L"UseVFTableForReset" );

    render.d3d9.force_d3d9ex =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory.create_parameter <bool> (
          L"Force D3D9Ex Context")
        );
    render.d3d9.force_d3d9ex->register_to_ini (
      dll_ini,
        L"Render.D3D9",
          L"ForceD3D9Ex" );
    render.d3d9.impure =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory.create_parameter <bool> (
          L"Force PURE device off")
        );
    render.d3d9.impure->register_to_ini (
      dll_ini,
        L"Render.D3D9",
          L"ForceImpure" );
    render.d3d9.enable_texture_mods =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory.create_parameter <bool> (
          L"Enable Texture Modding Support")
        );
    render.d3d9.enable_texture_mods->register_to_ini (
      dll_ini,
        L"Render.D3D9",
          L"EnableTextureMods" );
    render.d3d9.hook_type =
      dynamic_cast <sk::ParameterInt *>
        (g_ParameterFactory.create_parameter <int> (
          L"Hook Technique")
        );
    render.d3d9.hook_type->register_to_ini (
      dll_ini,
        L"Render.D3D9",
          L"HookType" );


    render.framerate.max_delta_time =
      dynamic_cast <sk::ParameterInt *>
        (g_ParameterFactory.create_parameter <int> (
          L"Maximum Frame Delta Time")
        );
    render.framerate.max_delta_time->register_to_ini (
      dll_ini,
        L"Render.DXGI",
          L"MaxDeltaTime" );

    render.framerate.flip_discard =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory.create_parameter <bool> (
          L"Use Flip Discard - Windows 10+")
        );
    render.framerate.flip_discard->register_to_ini (
      dll_ini,
        L"Render.DXGI",
          L"UseFlipDiscard" );

    render.dxgi.adapter_override =
      dynamic_cast <sk::ParameterInt *>
        (g_ParameterFactory.create_parameter <int> (
          L"Override DXGI Adapter")
        );
    render.dxgi.adapter_override->register_to_ini (
      dll_ini,
        L"Render.DXGI",
          L"AdapterOverride" );

    render.dxgi.max_res =
      dynamic_cast <sk::ParameterStringW *>
        (g_ParameterFactory.create_parameter <std::wstring> (
          L"Maximum Resolution To Report")
        );
    render.dxgi.max_res->register_to_ini (
      dll_ini,
        L"Render.DXGI",
          L"MaxRes" );

    render.dxgi.min_res =
      dynamic_cast <sk::ParameterStringW *>
        (g_ParameterFactory.create_parameter <std::wstring> (
          L"Minimum Resolution To Report")
        );
    render.dxgi.min_res->register_to_ini (
      dll_ini,
        L"Render.DXGI",
          L"MinRes" );

    render.dxgi.swapchain_wait =
      dynamic_cast <sk::ParameterInt *>
        (g_ParameterFactory.create_parameter <int> (
          L"Time to wait in msec. for SwapChain")
        );
    render.dxgi.swapchain_wait->register_to_ini (
      dll_ini,
        L"Render.DXGI",
          L"SwapChainWait" );

    render.dxgi.scaling_mode =
      dynamic_cast <sk::ParameterStringW *>
        (g_ParameterFactory.create_parameter <std::wstring> (
          L"Scaling Preference (DontCare | Centered | Stretched | Unspecified)")
        );
    render.dxgi.scaling_mode->register_to_ini (
      dll_ini,
        L"Render.DXGI",
          L"Scaling" );

    render.dxgi.exception_mode =
      dynamic_cast <sk::ParameterStringW *>
        (g_ParameterFactory.create_parameter <std::wstring> (
          L"D3D11 Exception Handling (DontCare | Raise | Ignore)")
        );
    render.dxgi.exception_mode->register_to_ini (
      dll_ini,
        L"Render.DXGI",
          L"ExceptionMode" );

    render.dxgi.debug_layer =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory.create_parameter <bool> (
          L"DXGI Debug Layer Support")
        );
    render.dxgi.debug_layer->register_to_ini (
      dll_ini,
        L"Render.DXGI",
          L"EnableDebugLayer" );

    render.dxgi.scanline_order =
      dynamic_cast <sk::ParameterStringW *>
        (g_ParameterFactory.create_parameter <std::wstring> (
          L"Scanline Order (DontCare | Progressive | LowerFieldFirst | UpperFieldFirst )")
        );
    render.dxgi.scanline_order->register_to_ini (
      dll_ini,
        L"Render.DXGI",
          L"ScanlineOrder" );

    render.dxgi.rotation =
      dynamic_cast <sk::ParameterStringW *>
        (g_ParameterFactory.create_parameter <std::wstring> (
          L"Screen Rotation (DontCare | Identity | 90 | 180 | 270 )")
        );
    render.dxgi.rotation->register_to_ini (
      dll_ini,
        L"Render.DXGI",
          L"Rotation" );

    render.dxgi.test_present =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory.create_parameter <bool> (
          L"Test SwapChain Presentation Before Actually Presenting")
        );
    render.dxgi.test_present->register_to_ini (
      dll_ini,
        L"Render.DXGI",
          L"TestSwapChainPresent" );

    render.dxgi.safe_fullscreen =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory.create_parameter <bool> (
          L"Prevent DXGI Deadlocks in Improperly Written Games")
        );
    render.dxgi.safe_fullscreen->register_to_ini (
      dll_ini,
        L"Render.DXGI",
          L"SafeFullscreenMode" );

    render.dxgi.enhanced_depth =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory.create_parameter <bool> (
          L"Use 32-bit Depth + 8-bit Stencil + 24-bit Padding")
        );
    render.dxgi.enhanced_depth->register_to_ini (
      dll_ini,
        L"Render.DXGI",
          L"Use64BitDepthStencil" );


    texture.d3d11.cache =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory.create_parameter <bool> (
          L"Cache Textures")
        );
    texture.d3d11.cache->register_to_ini (
      dll_ini,
        L"Textures.D3D11",
          L"Cache" );

    texture.d3d11.precise_hash =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory.create_parameter <bool> (
          L"Precise Hash Generation")
        );
    texture.d3d11.precise_hash->register_to_ini (
      dll_ini,
        L"Textures.D3D11",
          L"PreciseHash" );

    texture.d3d11.dump =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory.create_parameter <bool> (
          L"Dump Textures")
        );
    texture.d3d11.dump->register_to_ini (
      dll_ini,
        L"Textures.D3D11",
          L"Dump" );

    texture.d3d11.inject =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory.create_parameter <bool> (
          L"Inject Textures")
        );
    texture.d3d11.inject->register_to_ini (
      dll_ini,
        L"Textures.D3D11",
          L"Inject" );

    texture.d3d11.res_root =
      dynamic_cast <sk::ParameterStringW *>
        (g_ParameterFactory.create_parameter <std::wstring> (
          L"Resource Root")
        );
    texture.d3d11.res_root->register_to_ini (
      dll_ini,
        L"Textures.D3D11",
          L"ResourceRoot" );

    texture.res_root =
      dynamic_cast <sk::ParameterStringW *>
        (g_ParameterFactory.create_parameter <std::wstring> (
          L"Resource Root")
        );
    texture.res_root->register_to_ini (
      dll_ini,
        L"Textures.General",
          L"ResourceRoot" );

    texture.dump_on_load =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory.create_parameter <bool> (
          L"Dump Textures while Loading")
        );
    texture.dump_on_load->register_to_ini (
      dll_ini,
        L"Textures.General",
          L"DumpOnFirstLoad" );

    texture.cache.min_entries =
      dynamic_cast <sk::ParameterInt *>
        (g_ParameterFactory.create_parameter <int> (
          L"Minimum Cached Textures")
        );
    texture.cache.min_entries->register_to_ini (
      dll_ini,
        L"Textures.Cache",
          L"MinEntries" );

    texture.cache.max_entries =
      dynamic_cast <sk::ParameterInt *>
        (g_ParameterFactory.create_parameter <int> (
          L"Maximum Cached Textures")
        );
    texture.cache.max_entries->register_to_ini (
      dll_ini,
        L"Textures.Cache",
          L"MaxEntries" );

    texture.cache.min_evict =
      dynamic_cast <sk::ParameterInt *>
        (g_ParameterFactory.create_parameter <int> (
          L"Minimum Textures to Evict")
        );
    texture.cache.min_evict->register_to_ini (
      dll_ini,
        L"Textures.Cache",
          L"MinEvict" );

    texture.cache.max_evict =
      dynamic_cast <sk::ParameterInt *>
        (g_ParameterFactory.create_parameter <int> (
          L"Maximum Textures to Evict")
        );
    texture.cache.max_evict->register_to_ini (
      dll_ini,
        L"Textures.Cache",
          L"MaxEvict" );

    texture.cache.min_size =
      dynamic_cast <sk::ParameterInt *>
        (g_ParameterFactory.create_parameter <int> (
          L"Minimum Textures to Evict")
        );
    texture.cache.min_size->register_to_ini (
      dll_ini,
        L"Textures.Cache",
          L"MinSizeInMiB" );

    texture.cache.max_size =
      dynamic_cast <sk::ParameterInt *>
        (g_ParameterFactory.create_parameter <int> (
          L"Maximum Textures to Evict")
        );
    texture.cache.max_size->register_to_ini (
      dll_ini,
        L"Textures.Cache",
          L"MaxSizeInMiB" );

    texture.cache.ignore_non_mipped =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory.create_parameter <bool> (
          L"Ignore textures without mipmaps?")
        );
    texture.cache.ignore_non_mipped->register_to_ini (
      dll_ini,
        L"Textures.Cache",
          L"IgnoreNonMipmapped" );

    texture.cache.allow_staging =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory.create_parameter <bool> (
          L"Enable texture caching/dumping/injection on staged textures")
        );
    texture.cache.allow_staging->register_to_ini (
      dll_ini,
        L"Textures.Cache",
          L"AllowStaging" );


  nvidia.api.disable =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Disable NvAPI")
      );
  nvidia.api.disable->register_to_ini (
    dll_ini,
      L"NVIDIA.API",
        L"Disable" );


  nvidia.sli.compatibility =
    dynamic_cast <sk::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"SLI Compatibility Bits")
      );
  nvidia.sli.compatibility->register_to_ini (
    dll_ini,
      L"NVIDIA.SLI",
        L"CompatibilityBits" );

  nvidia.sli.num_gpus =
    dynamic_cast <sk::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"SLI GPU Count")
      );
  nvidia.sli.num_gpus->register_to_ini (
    dll_ini,
      L"NVIDIA.SLI",
        L"NumberOfGPUs" );

  nvidia.sli.mode =
    dynamic_cast <sk::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"SLI Mode")
      );
  nvidia.sli.mode->register_to_ini (
    dll_ini,
      L"NVIDIA.SLI",
        L"Mode" );

  nvidia.sli.override =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Override Driver Defaults")
      );
  nvidia.sli.override->register_to_ini (
    dll_ini,
      L"NVIDIA.SLI",
        L"Override" );


  imgui.scale =
    dynamic_cast <sk::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"ImGui Scale")
      );
  imgui.scale->register_to_ini (
    osd_ini,
      L"ImGui.Global",
        L"FontScale" );

  imgui.show_playtime =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Display Playing Time in Config UI")
      );
  imgui.show_playtime->register_to_ini (
    osd_ini,
      L"ImGui.Global",
        L"ShowPlaytime" );

  imgui.show_eula =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Show Software EULA")
      );
  imgui.show_eula->register_to_ini (
    dll_ini,
      L"SpecialK.System",
        L"ShowEULA" );

  imgui.show_gsync_status =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Show G-Sync Status on Control Panel")
      );
  imgui.show_gsync_status->register_to_ini (
    osd_ini,
      L"ImGui.Global",
        L"ShowGSyncStatus" );

  imgui.mac_style_menu =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Use Mac-style Menu Bar")
      );
  imgui.mac_style_menu->register_to_ini (
    osd_ini,
      L"ImGui.Global",
        L"UseMacStyleMenu" );

  imgui.show_input_apis =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Show Input APIs currently in-use")
      );
  imgui.show_input_apis->register_to_ini (
    osd_ini,
      L"ImGui.Global",
        L"ShowActiveInputAPIs" );


  osd.show =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"OSD Visibility")
      );
  osd.show->register_to_ini (
    osd_ini,
      L"SpecialK.OSD",
        L"Show" );

  osd.update_method.pump =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Refresh the OSD irrespective of frame completion")
      );
  osd.update_method.pump->register_to_ini (
    osd_ini,
      L"SpecialK.OSD",
        L"AutoPump" );

  osd.update_method.pump_interval =
    dynamic_cast <sk::ParameterFloat *>
    (g_ParameterFactory.create_parameter <float> (
      L"Time in seconds between OSD updates")
    );
  osd.update_method.pump_interval->register_to_ini (
    osd_ini,
      L"SpecialK.OSD",
        L"PumpInterval" );

  osd.text.red =
    dynamic_cast <sk::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"OSD Color (Red)")
      );
  osd.text.red->register_to_ini (
    osd_ini,
      L"SpecialK.OSD",
        L"TextColorRed" );

  osd.text.green =
    dynamic_cast <sk::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"OSD Color (Green)")
      );
  osd.text.green->register_to_ini (
    osd_ini,
      L"SpecialK.OSD",
        L"TextColorGreen" );

  osd.text.blue =
    dynamic_cast <sk::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"OSD Color (Blue)")
      );
  osd.text.blue->register_to_ini (
    osd_ini,
      L"SpecialK.OSD",
        L"TextColorBlue" );

  osd.viewport.pos_x =
    dynamic_cast <sk::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"OSD Position (X)")
      );
  osd.viewport.pos_x->register_to_ini (
    osd_ini,
      L"SpecialK.OSD",
        L"PositionX" );

  osd.viewport.pos_y =
    dynamic_cast <sk::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"OSD Position (Y)")
      );
  osd.viewport.pos_y->register_to_ini (
    osd_ini,
      L"SpecialK.OSD",
        L"PositionY" );

  osd.viewport.scale =
    dynamic_cast <sk::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"OSD Scale")
      );
  osd.viewport.scale->register_to_ini (
    osd_ini,
      L"SpecialK.OSD",
        L"Scale" );

  osd.state.remember =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Remember status monitoring state")
      );
  osd.state.remember->register_to_ini (
    osd_ini,
      L"SpecialK.OSD",
        L"RememberMonitoringState" );


  monitoring.SLI.show =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Show SLI Monitoring")
      );
  monitoring.SLI.show->register_to_ini (
    osd_ini,
      L"Monitor.SLI",
        L"Show" );


  steam.achievements.sound_file =
    dynamic_cast <sk::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"Achievement Sound File")
      );
  steam.achievements.sound_file->register_to_ini (
    dll_ini,
      L"Steam.Achievements",
        L"SoundFile" );

  steam.achievements.play_sound =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Silence is Bliss?")
      );
  steam.achievements.play_sound->register_to_ini(
    achievement_ini,
      L"Steam.Achievements",
        L"PlaySound" );

  steam.achievements.take_screenshot =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Precious Memories")
      );
  steam.achievements.take_screenshot->register_to_ini(
    achievement_ini,
      L"Steam.Achievements",
        L"TakeScreenshot" );

  steam.system.notify_corner =
    dynamic_cast <sk::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"Overlay Notification Position")
      );
  steam.system.notify_corner->register_to_ini (
    achievement_ini,
      L"Steam.System",
        L"NotifyCorner" );

  steam.achievements.popup.origin =
    dynamic_cast <sk::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"Achievement Popup Position")
      );
  steam.achievements.popup.origin->register_to_ini (
    achievement_ini,
      L"Steam.Achievements",
        L"PopupOrigin" );

  steam.achievements.popup.animate =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Achievement Notification Animation")
      );
  steam.achievements.popup.animate->register_to_ini (
    achievement_ini,
      L"Steam.Achievements",
        L"AnimatePopup" );

  steam.achievements.popup.show_title =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Achievement Popup Includes Game Title?")
      );
  steam.achievements.popup.show_title->register_to_ini (
    achievement_ini,
      L"Steam.Achievements",
        L"ShowPopupTitle" );

  steam.achievements.popup.inset =
    dynamic_cast <sk::ParameterFloat *>
    (g_ParameterFactory.create_parameter <float> (
      L"Achievement Notification Inset X")
    );
  steam.achievements.popup.inset->register_to_ini (
    achievement_ini,
      L"Steam.Achievements",
        L"PopupInset" );

  steam.achievements.popup.duration =
    dynamic_cast <sk::ParameterInt *>
    (g_ParameterFactory.create_parameter <int> (
      L"Achievement Popup Duration (in ms)")
    );
  steam.achievements.popup.duration->register_to_ini (
    achievement_ini,
      L"Steam.Achievements",
        L"PopupDuration" );

  steam.system.appid = 
    dynamic_cast <sk::ParameterInt *>
    (g_ParameterFactory.create_parameter <int> (
      L"Steam AppID")
    );
  steam.system.appid->register_to_ini (
    dll_ini,
      L"Steam.System",
        L"AppID" );

  steam.system.init_delay =
    dynamic_cast <sk::ParameterInt *>
    (g_ParameterFactory.create_parameter <int> (
      L"How long to delay SteamAPI initialization if the game doesn't do it")
    );
  steam.system.init_delay->register_to_ini (
    dll_ini,
      L"Steam.System",
        L"AutoInitDelay" );

  steam.system.auto_pump =
    dynamic_cast <sk::ParameterBool *>
    (g_ParameterFactory.create_parameter <bool> (
      L"Should we force the game to run Steam callbacks?")
    );
  steam.system.auto_pump->register_to_ini (
    dll_ini,
      L"Steam.System",
        L"AutoPumpCallbacks" );

  steam.system.block_stat_callback =
    dynamic_cast <sk::ParameterBool *>
    (g_ParameterFactory.create_parameter <bool> (
      L"Block the User Stats Receipt Callback?")
    );
  steam.system.block_stat_callback->register_to_ini (
    dll_ini,
      L"Steam.System",
        L"BlockUserStatsCallback" );

  steam.system.filter_stat_callbacks =
    dynamic_cast <sk::ParameterBool *>
    (g_ParameterFactory.create_parameter <bool> (
      L"Filter Unrelated Data from the User Stats Receipt Callback?")
    );
  steam.system.filter_stat_callbacks->register_to_ini (
    dll_ini,
      L"Steam.System",
        L"FilterExternalDataFromCallbacks" );

  steam.system.load_early =
    dynamic_cast <sk::ParameterBool *>
    (g_ParameterFactory.create_parameter <bool> (
      L"Load the Steam Client DLL Early?")
    );
  steam.system.load_early->register_to_ini (
    dll_ini,
      L"Steam.System",
        L"PreLoadSteamClient" );

  steam.system.early_overlay =
    dynamic_cast <sk::ParameterBool *>
    (g_ParameterFactory.create_parameter <bool> (
      L"Load the Steam Overlay Early")
    );
  steam.system.early_overlay->register_to_ini (
    dll_ini,
      L"Steam.System",
        L"PreLoadSteamOverlay" );

  steam.system.force_load =
    dynamic_cast <sk::ParameterBool *>
    (g_ParameterFactory.create_parameter <bool> (
      L"Forcefully load steam_api{64}.dll")
    );
  steam.system.force_load->register_to_ini (
    dll_ini,
      L"Steam.System",
        L"ForceLoadSteamAPI" );

  steam.log.silent =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Makes steam_api.log go away")
      );
  steam.log.silent->register_to_ini(
    dll_ini,
      L"Steam.Log",
        L"Silent" );

  steam.drm.spoof_BLoggedOn =
    dynamic_cast <sk::ParameterBool *>
    ( g_ParameterFactory.create_parameter <bool> (
        L"Fix For Stupid Games That Don't Know How DRM Works.")
      );
  steam.drm.spoof_BLoggedOn->register_to_ini (
    dll_ini,
      L"Steam.DRMWorks",
        L"SpoofBLoggedOn" );

  iSK_INI::_TSectionMap& sections =
    dll_ini->get_sections ();

  auto sec =
    sections.begin ();

  int import = 0;

  host_executable.hLibrary     = GetModuleHandle     (nullptr);
  host_executable.product_desc = SK_GetDLLVersionStr (SK_GetModuleFullName (host_executable.hLibrary).c_str ());

  while (sec != sections.end ())
  {
    if (wcsstr ((*sec).first.c_str (), L"Import."))
    {
      imports [import].name =
        CharNextW (wcsstr ((*sec).first.c_str (), L"."));

      imports [import].filename = 
         dynamic_cast <sk::ParameterStringW *>
             (g_ParameterFactory.create_parameter <std::wstring> (
                L"Import Filename")
             );
      imports [import].filename->register_to_ini (
        dll_ini,
          (*sec).first.c_str (),
            L"Filename" );

      imports [import].when = 
         dynamic_cast <sk::ParameterStringW *>
             (g_ParameterFactory.create_parameter <std::wstring> (
                L"Import Timeframe")
             );
      imports [import].when->register_to_ini (
        dll_ini,
          (*sec).first.c_str (),
            L"When" );

      imports [import].role = 
         dynamic_cast <sk::ParameterStringW *>
             (g_ParameterFactory.create_parameter <std::wstring> (
                L"Import Role")
             );
      imports [import].role->register_to_ini (
        dll_ini,
          (*sec).first.c_str (),
            L"Role" );

      imports [import].architecture = 
         dynamic_cast <sk::ParameterStringW *>
             (g_ParameterFactory.create_parameter <std::wstring> (
                L"Import Architecture")
             );
      imports [import].architecture->register_to_ini (
        dll_ini,
          (*sec).first.c_str (),
            L"Architecture" );

      imports [import].blacklist = 
         dynamic_cast <sk::ParameterStringW *>
             (g_ParameterFactory.create_parameter <std::wstring> (
                L"Blakclisted Executables")
             );
      imports [import].blacklist->register_to_ini (
        dll_ini,
          (*sec).first.c_str (),
            L"Blacklist" );

      imports [import].filename->load     ();
      imports [import].when->load         ();
      imports [import].role->load         ();
      imports [import].architecture->load ();
      imports [import].blacklist->load    ();

      imports [import].hLibrary = nullptr;

      ++import;

      if (import > SK_MAX_IMPORTS)
        break;
    }

    ++sec;
  }

  config.window.border_override    = true;


                                            //
  config.system.trace_load_library = true;  // Generally safe even with the 
                                            //   worst third-party software;
                                            //
                                            //  NEEDED for injector API detect


  config.system.strict_compliance  = false; // Will deadlock in DLLs that call
                                            //   LoadLibrary from DllMain
                                            //
                                            //  * NVIDIA Ansel, MSI Nahimic,
                                            //      Razer *, RTSS (Sometimes)
                                            //

  extern bool SK_DXGI_FullStateCache;
              SK_DXGI_FullStateCache = config.render.dxgi.full_state_cache;


  // Default = Don't Care
  config.render.dxgi.exception_mode = -1;
  config.render.dxgi.scaling_mode   = -1;

  games.emplace ( L"Tyranny.exe",                            SK_GAME_ID::Tyranny                      );
  games.emplace ( L"SRHK.exe",                               SK_GAME_ID::Shadowrun_HongKong           );
  games.emplace ( L"TidesOfNumenera.exe",                    SK_GAME_ID::TidesOfNumenera              );
  games.emplace ( L"MassEffectAndromeda.exe",                SK_GAME_ID::MassEffect_Andromeda         );
  games.emplace ( L"MadMax.exe",                             SK_GAME_ID::MadMax                       );
  games.emplace ( L"Dreamfall Chapters.exe",                 SK_GAME_ID::Dreamfall_Chapters           );
  games.emplace ( L"TheWitness.exe",                         SK_GAME_ID::TheWitness                   );
  games.emplace ( L"Obduction-Win64-Shipping.exe",           SK_GAME_ID::Obduction                    );
  games.emplace ( L"witcher3.exe",                           SK_GAME_ID::TheWitcher3                  );
  games.emplace ( L"re7.exe",                                SK_GAME_ID::ResidentEvil7                );
  games.emplace ( L"DDDA.exe",                               SK_GAME_ID::DragonsDogma                 );
  games.emplace ( L"eqgame.exe",                             SK_GAME_ID::EverQuest                    );
  games.emplace ( L"GE2RB.exe",                              SK_GAME_ID::GodEater2RageBurst           );
  games.emplace ( L"WatchDogs2.exe",                         SK_GAME_ID::WatchDogs2                   );
  games.emplace ( L"NieRAutomata.exe",                       SK_GAME_ID::NieRAutomata                 );
  games.emplace ( L"Warframe.x64.exe",                       SK_GAME_ID::Warframe_x64                 );
  games.emplace ( L"LEGOLCUR_DX11.exe",                      SK_GAME_ID::LEGOCityUndercover           );
  games.emplace ( L"Sacred.exe",                             SK_GAME_ID::Sacred                       );
  games.emplace ( L"sacred2.exe",                            SK_GAME_ID::Sacred2                      );
  games.emplace ( L"FF9.exe",                                SK_GAME_ID::FinalFantasy9                );
  games.emplace ( L"FinchGame.exe",                          SK_GAME_ID::EdithFinch                   );
  games.emplace ( L"FFX.exe",                                SK_GAME_ID::FinalFantasyX_X2             );
  games.emplace ( L"FFX-2.exe",                              SK_GAME_ID::FinalFantasyX_X2             );
  games.emplace ( L"DP.exe",                                 SK_GAME_ID::DeadlyPremonition            );
  games.emplace ( L"GG2Game.exe",                            SK_GAME_ID::GalGun_Double_Peace          );
  games.emplace ( L"AkibaUU.exe",                            SK_GAME_ID::AKIBAs_Trip                  );
  games.emplace ( L"Ys7.exe",                                SK_GAME_ID::YS_Seven                     );
  games.emplace ( L"TOS.exe",                                SK_GAME_ID::Tales_of_Symphonia           );
  games.emplace ( L"Life is Strange - Before the Storm.exe", SK_GAME_ID::LifeIsStrange_BeforeTheStorm );

  //
  // Application Compatibility Overrides
  // ===================================
  //
  if (games.count (std::wstring (SK_GetHostApp ())))
  {
    switch (games [std::wstring (SK_GetHostApp ())])
    {
      case SK_GAME_ID::Tyranny:
        // Cannot auto-detect API?!
        config.apis.dxgi.d3d11.hook       = false;
        config.apis.OpenGL.hook           = false;
        config.steam.filter_stat_callback = true; // Will stop running SteamAPI when it receives
                                                  //   data it didn't ask for
        break;


      case SK_GAME_ID::Shadowrun_HongKong:
        config.compatibility.d3d9.rehook_reset = true;
        break;


      case SK_GAME_ID::TidesOfNumenera:
        // API Auto-Detect Broken (0.7.43)
        //
        //   => Auto-Detection Thinks Game is OpenGL
        //
        config.apis.d3d9.hook       = true;
        config.apis.d3d9ex.hook     = false;
        config.apis.dxgi.d3d11.hook = false;
        config.apis.OpenGL.hook     = false;
        break;


      case SK_GAME_ID::MassEffect_Andromeda:
        // Disable Exception Handling Instead of Crashing at Shutdown
        config.render.dxgi.exception_mode      = D3D11_RAISE_FLAG_DRIVER_INTERNAL_ERROR;

        // Not a Steam game :(
        config.steam.silent                    = true;

        config.system.strict_compliance        = false; // Uses NVIDIA Ansel, so this won't work!

        config.apis.d3d9.hook                  = false;
        config.apis.d3d9ex.hook                = false;
        config.apis.OpenGL.hook                = false;

        config.input.ui.capture_hidden         = false; // Mouselook is a bitch
        config.input.mouse.add_relative_motion = true;
        SK_ImGui_Cursor.prefs.no_warp.ui_open  = false;
        SK_ImGui_Cursor.prefs.no_warp.visible  = false;

        config.textures.d3d11.cache            = true;
        config.textures.cache.ignore_nonmipped = true;
        config.textures.cache.max_size         = 4096;
        break;


      case SK_GAME_ID::MadMax:
        break;


      case SK_GAME_ID::Dreamfall_Chapters:
        config.system.trace_load_library       = true;
        config.system.strict_compliance        = false;

        // Game has mouselook problems without this
        config.input.mouse.add_relative_motion = true;

        // Chances are good that we will not catch SteamAPI early enough to hook callbacks, so
        //   auto-pump.
        config.steam.auto_pump_callbacks       = true;
        config.steam.preload_client            = true;
        config.steam.filter_stat_callback      = true; // Will stop running SteamAPI when it receives
                                                    //   data it didn't ask for

        config.apis.dxgi.d3d11.hook            = true;
        config.apis.d3d9.hook                  = true;
        config.apis.d3d9ex.hook                = true;
        config.apis.OpenGL.hook                = false;
        break;


      case SK_GAME_ID::TheWitness:
        config.system.trace_load_library    = true;
        config.system.strict_compliance     = false; // Uses Ansel

        // Game has mouselook problems without this
        config.input.ui.capture_mouse       = true;
        break;


      case SK_GAME_ID::Obduction:
        config.system.trace_load_library = true;  // Need to catch SteamAPI DLL load
        config.system.strict_compliance  = false; // Cannot block threads while loading DLLs
                                                  //   (uses an incorrectly written DLL)
        break;


      case SK_GAME_ID::TheWitcher3:
        config.system.strict_compliance   = false; // Uses NVIDIA Ansel, so this won't work!
        config.steam.filter_stat_callback = true;  // Will stop running SteamAPI when it receives
                                                   //   data it didn't ask for

        config.apis.d3d9.hook             = false;
        config.apis.d3d9ex.hook           = false;
        config.apis.OpenGL.hook           = false;

        config.textures.cache.ignore_nonmipped = true; // Invalid use of immutable textures
        break;


      case SK_GAME_ID::ResidentEvil7:
        config.system.trace_load_library = true;  // Need to catch SteamAPI DLL load
        config.system.strict_compliance  = false; // Cannot block threads while loading DLLs
                                                  //   (uses an incorrectly written DLL)
        break;


      case SK_GAME_ID::DragonsDogma:
        // BROKEN (by GeForce Experience)
        //
        // TODO: Debug the EXACT cause of NVIDIA's Deadlock
        //
        config.compatibility.disable_nv_bloat = true;  // PREVENT DEADLOCK CAUSED BY NVIDIA!

        config.system.trace_load_library      = true;  // Need to catch NVIDIA Bloat DLLs
        config.system.strict_compliance       = false; // Cannot block threads while loading DLLs
                                                       //   (uses an incorrectly written DLL)

        config.steam.auto_pump_callbacks      = false;
        config.steam.preload_client           = true;

        config.apis.d3d9.hook                 = true;
        config.apis.dxgi.d3d11.hook           = false;
        config.apis.d3d9ex.hook               = false;
        config.apis.OpenGL.hook               = false;
        break;


      case SK_GAME_ID::EverQuest:
        // Fix-up rare issues during Server Select -> Game
        //config.compatibility.d3d9.rehook_reset = true;
        break;


      case SK_GAME_ID::GodEater2RageBurst:
        //Does not support XInput hot-plugging, needs Special K loving :)
        config.input.gamepad.xinput.placehold [0] = true;

        config.apis.d3d9.hook                     = true;
        config.apis.d3d9ex.hook                   = true;
        config.apis.dxgi.d3d11.hook               = false;
        config.apis.OpenGL.hook                   = false;
        break;


      case SK_GAME_ID::WatchDogs2:
        //Does not support XInput hot-plugging, needs Special K loving :)
        config.input.gamepad.xinput.placehold [0] = true;
        config.input.mouse.add_relative_motion    = true;
        break;


      case SK_GAME_ID::NieRAutomata:
        // Maximize compatibility with 3rd party injectors that corrupt hooks
        //config.render.dxgi.slow_state_cache    = false;
        //SK_DXGI_SlowStateCache                 = config.render.dxgi.slow_state_cache;
        config.render.dxgi.scaling_mode        = DXGI_MODE_SCALING_UNSPECIFIED;
        config.input.mouse.add_relative_motion = false;
        break;


      case SK_GAME_ID::Warframe_x64:
        config.apis.d3d9.hook       = false;
        config.apis.d3d9ex.hook     = false;
        config.apis.dxgi.d3d11.hook = true;
        break;


      case SK_GAME_ID::LEGOCityUndercover:
        // Prevent the game from deadlocking its message pump in fullscreen
        config.render.dxgi.safe_fullscreen       = true;
        config.render.framerate.sleepless_window = true; // Fix framerate limiter
        break;


      case SK_GAME_ID::Sacred2:
        config.display.force_windowed      = true; // Fullscreen is not particularly well
                                                   //   supported in this game
      case SK_GAME_ID::Sacred:
        config.render.dxgi.safe_fullscreen = true; // dgVoodoo compat
        // Contrary to its name, this game needs this turned off ;)
        config.cegui.safe_init             = false;
        config.steam.force_load_steamapi   = true; // Not safe in all games, but it is here.
        break;


      case SK_GAME_ID::FinalFantasy9:
        // Don't auto-pump callbacks
        config.steam.auto_pump_callbacks = false;
        config.apis.OpenGL.hook          = false; // Not an OpenGL game, API auto-detect is borked
        break;


      case SK_GAME_ID::EdithFinch:
        config.render.framerate.sleepless_window = true;
        break;


      case SK_GAME_ID::FinalFantasyX_X2:
        // Don't auto-pump callbacks 
        //  Excessively lenghty startup is followed by actual SteamAPI init eventually...
        config.steam.auto_pump_callbacks = false;

        //config.render.dxgi.full_state_cache    = true;
        //SK_DXGI_FullStateCache                 = config.render.dxgi.full_state_cache;
        break;

#ifndef _WIN64
      case SK_GAME_ID::DeadlyPremonition:
        config.steam.force_load_steamapi       = true;
        config.apis.d3d9.hook                  = true;
        config.apis.d3d9ex.hook                = false;
        config.apis.d3d8.hook                  = false;
        config.input.mouse.add_relative_motion = false;
        break;
#endif

#ifdef _WIN64
        case SK_GAME_ID::LifeIsStrange_BeforeTheStorm:
          config.apis.d3d9.hook       = false;
          config.apis.d3d9ex.hook     = false;
          config.apis.OpenGL.hook     = false;
          config.apis.Vulkan.hook     = false;
          config.apis.dxgi.d3d11.hook = true;
          config.apis.dxgi.d3d12.hook = false;
          break;
#endif
    }
  }

  init = true; }



  //
  // Load Parameters
  //
  if (compatibility.ignore_raptr->load ())
    config.compatibility.ignore_raptr = compatibility.ignore_raptr->get_value ();
  if (compatibility.disable_raptr->load ())
    config.compatibility.disable_raptr = compatibility.disable_raptr->get_value ();
  if (compatibility.disable_nv_bloat->load ())
    config.compatibility.disable_nv_bloat = compatibility.disable_nv_bloat->get_value ();
  if (compatibility.rehook_loadlibrary->load ())
    config.compatibility.rehook_loadlibrary = compatibility.rehook_loadlibrary->get_value ();


  if (osd.state.remember->load ())
    config.osd.remember_state = osd.state.remember->get_value ();

  if (imgui.scale->load ())
    config.imgui.scale = imgui.scale->get_value ();

  if (imgui.show_eula->load ())
    config.imgui.show_eula = imgui.show_eula->get_value ();

  if (imgui.show_playtime->load ())
    config.steam.show_playtime = imgui.show_playtime->get_value ();

  if (imgui.show_gsync_status->load ())
    config.apis.NvAPI.gsync_status = imgui.show_gsync_status->get_value ();

  if (imgui.mac_style_menu->load ())
    config.imgui.use_mac_style_menu = imgui.mac_style_menu->get_value ();

  if (imgui.show_input_apis->load ())
    config.imgui.show_input_apis = imgui.show_input_apis->get_value ();


  if (monitoring.io.show->load () && config.osd.remember_state)
    config.io.show = monitoring.io.show->get_value ();
  if (monitoring.io.interval->load ())
    config.io.interval = monitoring.io.interval->get_value ();

  if (monitoring.fps.show->load ())
    config.fps.show = monitoring.fps.show->get_value ();

  if (monitoring.memory.show->load () && config.osd.remember_state)
    config.mem.show = monitoring.memory.show->get_value ();
  if (mem_reserve->load ())
    config.mem.reserve = mem_reserve->get_value ();

  if (monitoring.cpu.show->load () && config.osd.remember_state)
    config.cpu.show = monitoring.cpu.show->get_value ();
  if (monitoring.cpu.interval->load ())
    config.cpu.interval = monitoring.cpu.interval->get_value ();
  if (monitoring.cpu.simple->load ())
    config.cpu.simple = monitoring.cpu.simple->get_value ();

  if (monitoring.gpu.show->load ())
    config.gpu.show = monitoring.gpu.show->get_value ();
  if (monitoring.gpu.print_slowdown->load ())
    config.gpu.print_slowdown = monitoring.gpu.print_slowdown->get_value ();
  if (monitoring.gpu.interval->load ())
    config.gpu.interval = monitoring.gpu.interval->get_value ();

  if (monitoring.disk.show->load () && config.osd.remember_state)
    config.disk.show = monitoring.disk.show->get_value ();
  if (monitoring.disk.interval->load ())
    config.disk.interval = monitoring.disk.interval->get_value ();
  if (monitoring.disk.type->load ())
    config.disk.type = monitoring.disk.type->get_value ();

  //if (monitoring.pagefile.show->load () && config.osd.remember_state)
    //config.pagefile.show = monitoring.pagefile.show->get_value ();
  if (monitoring.pagefile.interval->load ())
    config.pagefile.interval = monitoring.pagefile.interval->get_value ();

  if (monitoring.time.show->load ())
    config.time.show = monitoring.time.show->get_value ();

  if (monitoring.SLI.show->load ())
    config.sli.show = monitoring.SLI.show->get_value ();


  if (apis.last_known->load ())
    config.apis.last_known = (SK_RenderAPI)apis.last_known->get_value ();

#ifndef _WIN64
  if (apis.ddraw.hook->load ())
    config.apis.ddraw.hook = apis.ddraw.hook->get_value ();

  if (apis.d3d8.hook->load ())
    config.apis.d3d8.hook = apis.d3d8.hook->get_value ();
#endif


  if (apis.d3d9.hook->load ())
    config.apis.d3d9.hook = apis.d3d9.hook->get_value ();

  if (apis.d3d9ex.hook->load ())
    config.apis.d3d9ex.hook = apis.d3d9ex.hook->get_value ();

  if (apis.d3d11.hook->load ())
    config.apis.dxgi.d3d11.hook = apis.d3d11.hook->get_value ();

#ifdef _WIN64
  if (apis.d3d12.hook->load ())
    config.apis.dxgi.d3d12.hook = apis.d3d12.hook->get_value ();
#endif

  if (apis.OpenGL.hook->load ())
    config.apis.OpenGL.hook = apis.OpenGL.hook->get_value ();

#ifdef _WIN64
  if (apis.Vulkan.hook->load ())
    config.apis.Vulkan.hook = apis.Vulkan.hook->get_value ();
#endif

  if (nvidia.api.disable->load ())
    config.apis.NvAPI.enable = (! nvidia.api.disable->get_value ());


  if (display.force_fullscreen->load ())
    config.display.force_fullscreen =
      display.force_fullscreen->get_value ();
  if (display.force_windowed->load ())
    config.display.force_windowed =
      display.force_windowed->get_value ();


  if (render.framerate.target_fps->load ())
    config.render.framerate.target_fps =
      render.framerate.target_fps->get_value ();
  if (render.framerate.limiter_tolerance->load ())
    config.render.framerate.limiter_tolerance =
      render.framerate.limiter_tolerance->get_value ();
  if (render.framerate.sleepless_render->load ())
    config.render.framerate.sleepless_render =
      render.framerate.sleepless_render->get_value ();
  if (render.framerate.sleepless_window->load ())
    config.render.framerate.sleepless_window =
      render.framerate.sleepless_window->get_value ();

  // D3D9/11
  //

  // SLI only works in Direct3D
  if (nvidia.sli.compatibility->load ())
    config.nvidia.sli.compatibility =
      nvidia.sli.compatibility->get_value ();
  if (nvidia.sli.mode->load ())
    config.nvidia.sli.mode =
      nvidia.sli.mode->get_value ();
  if (nvidia.sli.num_gpus->load ())
    config.nvidia.sli.num_gpus =
      nvidia.sli.num_gpus->get_value ();
  if (nvidia.sli.override->load ())
    config.nvidia.sli.override =
      nvidia.sli.override->get_value ();

  if (render.framerate.wait_for_vblank->load ())
    config.render.framerate.wait_for_vblank =
      render.framerate.wait_for_vblank->get_value ();
  if (render.framerate.buffer_count->load ())
    config.render.framerate.buffer_count =
      render.framerate.buffer_count->get_value ();
  if (render.framerate.prerender_limit->load ())
    config.render.framerate.pre_render_limit =
      render.framerate.prerender_limit->get_value ();
  if (render.framerate.present_interval->load ())
    config.render.framerate.present_interval =
      render.framerate.present_interval->get_value ();

  if (render.framerate.refresh_rate)
  {
    if (render.framerate.refresh_rate->load ())
      config.render.framerate.refresh_rate =
        render.framerate.refresh_rate->get_value ();
  }

  // D3D9
  //
  if (compatibility.d3d9.rehook_present->load ())
    config.compatibility.d3d9.rehook_present =
      compatibility.d3d9.rehook_present->get_value ();
  if (compatibility.d3d9.rehook_reset->load ())
    config.compatibility.d3d9.rehook_reset =
      compatibility.d3d9.rehook_reset->get_value ();

  if (compatibility.d3d9.hook_present_vtable->load ())
    config.compatibility.d3d9.hook_present_vftbl =
      compatibility.d3d9.hook_present_vtable->get_value ();
  if (compatibility.d3d9.hook_reset_vtable->load ())
    config.compatibility.d3d9.hook_reset_vftbl =
      compatibility.d3d9.hook_reset_vtable->get_value ();

  if (render.d3d9.force_d3d9ex->load ())
    config.render.d3d9.force_d3d9ex =
      render.d3d9.force_d3d9ex->get_value ();
  if (render.d3d9.impure->load ())
    config.render.d3d9.force_impure =
      render.d3d9.impure->get_value ();
  if (render.d3d9.enable_texture_mods->load ())
    config.textures.d3d9_mod =
      render.d3d9.enable_texture_mods->get_value ();
  if (render.d3d9.hook_type->load ())
    config.render.d3d9.hook_type =
      render.d3d9.hook_type->get_value ();


  // DXGI
  //
  if (render.framerate.max_delta_time->load ())
    config.render.framerate.max_delta_time =
      render.framerate.max_delta_time->get_value ();
  if (render.framerate.flip_discard->load ())
  {
    config.render.framerate.flip_discard =
      render.framerate.flip_discard->get_value ();

    if (render.framerate.allow_dwm_tearing->load ())
    {
      config.render.dxgi.allow_tearing = render.framerate.allow_dwm_tearing->get_value ();
      //if (config.render.dxgi.allow_tearing) config.render.framerate.flip_discard = true;
    }

    extern bool SK_DXGI_use_factory1;
    if (config.render.framerate.flip_discard)
      SK_DXGI_use_factory1 = true;
  }

  if (render.dxgi.adapter_override->load ())
    config.render.dxgi.adapter_override =
      render.dxgi.adapter_override->get_value ();

  if (render.dxgi.max_res->load ())
  {
    swscanf ( render.dxgi.max_res->get_value_str ().c_str (),
                L"%lux%lu",
                &config.render.dxgi.res.max.x,
                  &config.render.dxgi.res.max.y );
  }
  if (render.dxgi.min_res->load ())
  {
    swscanf ( render.dxgi.min_res->get_value_str ().c_str (),
                L"%lux%lu",
                &config.render.dxgi.res.min.x,
                  &config.render.dxgi.res.min.y );
  }

  if (render.dxgi.scaling_mode->load ())
  {
    if (! _wcsicmp (
            render.dxgi.scaling_mode->get_value_str ().c_str (),
            L"Unspecified"
          )
       )
    {
      config.render.dxgi.scaling_mode = DXGI_MODE_SCALING_UNSPECIFIED;
    }

    else if (! _wcsicmp (
                 render.dxgi.scaling_mode->get_value_str ().c_str (),
                 L"Centered"
               )
            )
    {
      config.render.dxgi.scaling_mode = DXGI_MODE_SCALING_CENTERED;
    }

    else if (! _wcsicmp (
                 render.dxgi.scaling_mode->get_value_str ().c_str (),
                 L"Stretched"
               )
            )
    {
      config.render.dxgi.scaling_mode = DXGI_MODE_SCALING_STRETCHED;
    }
  }

  if (render.dxgi.scanline_order->load ())
  {
    if (! _wcsicmp (
            render.dxgi.scanline_order->get_value_str ().c_str (),
            L"Unspecified"
          )
       )
    {
      config.render.dxgi.scanline_order = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    }

    else if (! _wcsicmp (
                 render.dxgi.scanline_order->get_value_str ().c_str (),
                 L"Progressive"
               )
            )
    {
      config.render.dxgi.scanline_order = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
    }

    else if (! _wcsicmp (
                 render.dxgi.scanline_order->get_value_str ().c_str (),
                 L"LowerFieldFirst"
               )
            )
    {
      config.render.dxgi.scanline_order = DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST;
    }

    else if (! _wcsicmp (
                 render.dxgi.scanline_order->get_value_str ().c_str (),
                 L"UpperFieldFirst"
               )
            )
    {
      config.render.dxgi.scanline_order = DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST;
    }

    // If a user specifies Interlaced, default to Lower Field First
    else if (! _wcsicmp (
                 render.dxgi.scanline_order->get_value_str ().c_str (),
                 L"Interlaced"
               )
            )
    {
      config.render.dxgi.scanline_order = DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST;
    }
  }

  if (render.dxgi.debug_layer->load ())
    config.render.dxgi.debug_layer = render.dxgi.debug_layer->get_value ();

  if (render.dxgi.exception_mode->load ())
  {
    if (! _wcsicmp (
            render.dxgi.exception_mode->get_value_str ().c_str (),
            L"Raise"
          )
       )
    {
      #define D3D11_RAISE_FLAG_DRIVER_INTERNAL_ERROR 1
      config.render.dxgi.exception_mode = D3D11_RAISE_FLAG_DRIVER_INTERNAL_ERROR;
    }

    else if (! _wcsicmp (
                 render.dxgi.exception_mode->get_value_str ().c_str (),
                 L"Ignore"
               )
            )
    {
      config.render.dxgi.exception_mode = 0;
    }
    else
      config.render.dxgi.exception_mode = -1;
  }

  if (render.dxgi.test_present->load ())
    config.render.dxgi.test_present = render.dxgi.test_present->get_value ();

  if (render.dxgi.swapchain_wait->load ())
    config.render.framerate.swapchain_wait = render.dxgi.swapchain_wait->get_value ();

  if (render.dxgi.safe_fullscreen->load ())
    config.render.dxgi.safe_fullscreen = render.dxgi.safe_fullscreen->get_value ();

  if (render.dxgi.enhanced_depth->load ())
    config.render.dxgi.enhanced_depth = render.dxgi.enhanced_depth->get_value ();


  if (texture.d3d11.cache->load ())
    config.textures.d3d11.cache = texture.d3d11.cache->get_value ();
  if (texture.d3d11.precise_hash->load ())
    config.textures.d3d11.precise_hash = texture.d3d11.precise_hash->get_value ();
  if (texture.d3d11.dump->load ())
    config.textures.d3d11.dump = texture.d3d11.dump->get_value ();
  if (texture.d3d11.inject->load ())
    config.textures.d3d11.inject = texture.d3d11.inject->get_value ();
  if (texture.d3d11.res_root->load ())
    config.textures.d3d11.res_root = texture.d3d11.res_root->get_value ();
  if (texture.res_root->load ())
    config.textures.d3d11.res_root = texture.res_root->get_value ();
  if (texture.dump_on_load->load ())
    config.textures.dump_on_load = texture.dump_on_load->get_value ();

  if (texture.cache.max_entries->load ())
    config.textures.cache.max_entries = texture.cache.max_entries->get_value ();
  if (texture.cache.min_entries->load ())
    config.textures.cache.min_entries = texture.cache.min_entries->get_value ();
  if (texture.cache.max_evict->load ())
    config.textures.cache.max_evict = texture.cache.max_evict->get_value ();
  if (texture.cache.min_evict->load ())
    config.textures.cache.min_evict = texture.cache.min_evict->get_value ();
  if (texture.cache.max_size->load ())
    config.textures.cache.max_size = texture.cache.max_size->get_value ();
  if (texture.cache.min_size->load ())
    config.textures.cache.min_size = texture.cache.min_size->get_value ();
  if (texture.cache.ignore_non_mipped->load ())
    config.textures.cache.ignore_nonmipped = texture.cache.ignore_non_mipped->get_value ();
  if (texture.cache.allow_staging->load ())
    config.textures.cache.allow_staging = texture.cache.allow_staging->get_value ();

  extern void WINAPI SK_DXGI_SetPreferredAdapter (int override_id);

  if (config.render.dxgi.adapter_override != -1)
    SK_DXGI_SetPreferredAdapter (config.render.dxgi.adapter_override);

  if (input.cursor.manage->load ())
    config.input.cursor.manage = input.cursor.manage->get_value ();
  if (input.cursor.keys_activate->load ())
    config.input.cursor.keys_activate = input.cursor.keys_activate->get_value ();
  if (input.cursor.timeout->load ())
    config.input.cursor.timeout = (int)(1000.0 * input.cursor.timeout->get_value ());
  if (input.cursor.ui_capture->load ())
    config.input.ui.capture = input.cursor.ui_capture->get_value ();
  if (input.cursor.hw_cursor->load ())
    config.input.ui.use_hw_cursor = input.cursor.hw_cursor->get_value ();
  if (input.cursor.no_warp_ui->load ())
    SK_ImGui_Cursor.prefs.no_warp.ui_open = input.cursor.no_warp_ui->get_value ();
  if (input.cursor.no_warp_visible->load ())
    SK_ImGui_Cursor.prefs.no_warp.visible = input.cursor.no_warp_visible->get_value ();
  if (input.cursor.block_invisible->load ())
    config.input.ui.capture_hidden = input.cursor.block_invisible->get_value ();
  if (input.cursor.fix_synaptics->load ())
    config.input.mouse.fix_synaptics = input.cursor.fix_synaptics->get_value ();
  if (input.cursor.antiwarp_deadzone->load ())
    config.input.mouse.antiwarp_deadzone = input.cursor.antiwarp_deadzone->get_value ();
  if (input.cursor.use_relative_input->load ())
    config.input.mouse.add_relative_motion = input.cursor.use_relative_input->get_value ();

  if (input.gamepad.disable_ps4_hid->load ())
    config.input.gamepad.disable_ps4_hid = input.gamepad.disable_ps4_hid->get_value ();
  if (input.gamepad.rehook_xinput->load ())
    config.input.gamepad.rehook_xinput = input.gamepad.rehook_xinput->get_value ();
  if (input.gamepad.hook_xinput->load ())
    config.input.gamepad.hook_xinput = input.gamepad.hook_xinput->get_value ();

  // Hidden INI values; they're loaded, but never written
  if (input.gamepad.hook_dinput8->load ())
    config.input.gamepad.hook_dinput8 = input.gamepad.hook_dinput8->get_value ();
  if (input.gamepad.hook_hid->load ())
    config.input.gamepad.hook_hid = input.gamepad.hook_hid->get_value ();
  if (input.gamepad.native_ps4->load ())
    config.input.gamepad.native_ps4 = input.gamepad.native_ps4->get_value ();

  if (input.gamepad.haptic_ui->load ())
    config.input.gamepad.haptic_ui = input.gamepad.haptic_ui->get_value ();

  if (input.gamepad.xinput.placeholders->load ()) {
    int placeholder_mask = input.gamepad.xinput.placeholders->get_value ();

    config.input.gamepad.xinput.placehold [0] = ( placeholder_mask & 0x1 );
    config.input.gamepad.xinput.placehold [1] = ( placeholder_mask & 0x2 );
    config.input.gamepad.xinput.placehold [2] = ( placeholder_mask & 0x4 );
    config.input.gamepad.xinput.placehold [3] = ( placeholder_mask & 0x8 );
  }

  if (input.gamepad.xinput.disable_rumble->load ())
    config.input.gamepad.xinput.disable_rumble = input.gamepad.xinput.disable_rumble->get_value ();


  if (input.gamepad.xinput.assignment->load ())
  {
    wchar_t* wszAssign =
      _wcsdup (input.gamepad.xinput.assignment->get_value ().c_str ());

    wchar_t* wszBuf = nullptr;
    wchar_t* wszTok =
      std::wcstok (wszAssign, L",", &wszBuf);

    if (wszTok == nullptr)
    {
      config.input.gamepad.xinput.assignment [0] = 0; config.input.gamepad.xinput.assignment [1] = 1;
      config.input.gamepad.xinput.assignment [2] = 2; config.input.gamepad.xinput.assignment [3] = 3;
    }

    int idx = 0;

    while (wszTok && idx < 4)
    {
      config.input.gamepad.xinput.assignment [idx++] =
        _wtoi (wszTok);

      wszTok =
        std::wcstok (nullptr, L",", &wszBuf);
    }

    free (wszAssign);
  }

  if (input.gamepad.xinput.ui_slot->load ())
    config.input.gamepad.xinput.ui_slot = input.gamepad.xinput.ui_slot->get_value ();

  if (window.borderless->load ())
    config.window.borderless = window.borderless->get_value ();

  if (window.center->load ())
    config.window.center = window.center->get_value ();
  if (window.background_render->load ())
    config.window.background_render = window.background_render->get_value ();
  if (window.background_mute->load ())
    config.window.background_mute = window.background_mute->get_value ();
  if (window.offset.x->load ()) {
    std::wstring offset = window.offset.x->get_value ();

    if (wcsstr (offset.c_str (), L"%"))
    {
      config.window.offset.x.absolute = 0;
      swscanf (offset.c_str (), L"%f%%", &config.window.offset.x.percent);
      config.window.offset.x.percent /= 100.0f;
    }

    else
    {
      config.window.offset.x.percent = 0.0f;
      swscanf (offset.c_str (), L"%li", &config.window.offset.x.absolute);
    }
  }
  if (window.offset.y->load ())
  {
    std::wstring offset = window.offset.y->get_value ();

    if (wcsstr (offset.c_str (), L"%"))
    {
      config.window.offset.y.absolute = 0;
      swscanf (offset.c_str (), L"%f%%", &config.window.offset.y.percent);
      config.window.offset.y.percent /= 100.0f;
    }

    else
    {
      config.window.offset.y.percent = 0.0f;
      swscanf (offset.c_str (), L"%li", &config.window.offset.y.absolute);
    }
  }

  if (window.confine_cursor->load ())
    config.window.confine_cursor = window.confine_cursor->get_value ();
  if (window.unconfine_cursor->load ())
    config.window.unconfine_cursor = window.unconfine_cursor->get_value ();
  if (window.persistent_drag->load ())
    config.window.persistent_drag = window.persistent_drag->get_value ();
  if (window.fullscreen->load ())
    config.window.fullscreen = window.fullscreen->get_value ();
  if (window.fix_mouse_coords->load ())
    config.window.res.override.fix_mouse =
      window.fix_mouse_coords->get_value ();
  if (window.override->load ())
  {
    swscanf ( window.override->get_value_str ().c_str (),
                L"%lux%lu",
                &config.window.res.override.x,
                  &config.window.res.override.y );
  }

  if (steam.achievements.play_sound->load ())
    config.steam.achievements.play_sound =
    steam.achievements.play_sound->get_value ();
  if (steam.achievements.sound_file->load ())
    config.steam.achievements.sound_file =
      steam.achievements.sound_file->get_value ();
  if (steam.achievements.take_screenshot->load ())
    config.steam.achievements.take_screenshot =
      steam.achievements.take_screenshot->get_value ();
  if (steam.achievements.popup.animate->load ())
    config.steam.achievements.popup.animate =
      steam.achievements.popup.animate->get_value ();
  if (steam.achievements.popup.show_title->load ())
    config.steam.achievements.popup.show_title =
      steam.achievements.popup.show_title->get_value ();
  if (steam.achievements.popup.origin->load ())
  {
    config.steam.achievements.popup.origin =
      SK_Steam_PopupOriginWStrToEnum (
        steam.achievements.popup.origin->get_value ().c_str ()
      );
  }
  else
  {
    config.steam.achievements.popup.origin = 3;
  }
  if (steam.achievements.popup.inset->load ())
    config.steam.achievements.popup.inset =
      steam.achievements.popup.inset->get_value ();
  if (steam.achievements.popup.duration->load ())
    config.steam.achievements.popup.duration =
      steam.achievements.popup.duration->get_value ();

  if (config.steam.achievements.popup.duration == 0)
  {
    config.steam.achievements.popup.show        = false;
    config.steam.achievements.pull_friend_stats = false;
    config.steam.achievements.pull_global_stats = false;
  }

  if (steam.log.silent->load ())
    config.steam.silent = steam.log.silent->get_value ();
  if (steam.drm.spoof_BLoggedOn->load ())
    config.steam.spoof_BLoggedOn = steam.drm.spoof_BLoggedOn->get_value ();

  if (steam.system.appid->load ())
    config.steam.appid = steam.system.appid->get_value ();
  if (steam.system.init_delay->load ())
    config.steam.init_delay = steam.system.init_delay->get_value ();
  if (steam.system.auto_pump->load ())
    config.steam.auto_pump_callbacks = steam.system.auto_pump->get_value ();
  if (steam.system.block_stat_callback->load ())
    config.steam.block_stat_callback = steam.system.block_stat_callback->get_value ();
  if (steam.system.filter_stat_callbacks->load ())
    config.steam.filter_stat_callback = steam.system.filter_stat_callbacks->get_value ();
  if (steam.system.load_early->load ())
    config.steam.preload_client = steam.system.load_early->get_value ();
  if (steam.system.early_overlay->load ())
    config.steam.preload_overlay = steam.system.early_overlay->get_value ();
  if (steam.system.force_load->load ())
    config.steam.force_load_steamapi = steam.system.force_load->get_value ();
  if (steam.system.notify_corner->load ())
    config.steam.notify_corner =
      SK_Steam_PopupOriginWStrToEnum (
        steam.system.notify_corner->get_value ().c_str ()
    );


  if (osd.show->load ())
    config.osd.show = osd.show->get_value ();

  if (osd.update_method.pump->load ())
    config.osd.pump = osd.update_method.pump->get_value ();

  if (osd.update_method.pump_interval->load ())
    config.osd.pump_interval = osd.update_method.pump_interval->get_value ();

  if (osd.text.red->load ())
    config.osd.red = osd.text.red->get_value ();
  if (osd.text.green->load ())
    config.osd.green = osd.text.green->get_value ();
  if (osd.text.blue->load ())
    config.osd.blue = osd.text.blue->get_value ();

  if (osd.viewport.pos_x->load ())
    config.osd.pos_x = osd.viewport.pos_x->get_value ();
  if (osd.viewport.pos_y->load ())
    config.osd.pos_y = osd.viewport.pos_y->get_value ();
  if (osd.viewport.scale->load ())
    config.osd.scale = osd.viewport.scale->get_value ();


  if (init_delay->load ())
    config.system.init_delay = init_delay->get_value ();
  if (silent->load ())
    config.system.silent = silent->get_value ();
  if (trace_libraries->load ())
    config.system.trace_load_library = trace_libraries->get_value ();
  if (strict_compliance->load ())
    config.system.strict_compliance = strict_compliance->get_value ();
  if (log_level->load ())
    config.system.log_level = log_level->get_value ();
  if (prefer_fahrenheit->load ())
    config.system.prefer_fahrenheit = prefer_fahrenheit->get_value ();

  if (ignore_rtss_delay->load ())
    config.system.ignore_rtss_delay = ignore_rtss_delay->get_value ();

  if (handle_crashes->load ())
    config.system.handle_crashes = handle_crashes->get_value ();

  if (debug_output->load ())
    config.system.display_debug_out = debug_output->get_value ();

  if (game_output->load ())
    config.system.game_output = game_output->get_value ();

  if (enable_cegui->load ())
    config.cegui.enable = enable_cegui->get_value ();

  if (safe_cegui->load ())
    config.cegui.safe_init = safe_cegui->get_value ();

  if (version->load ())
    config.system.version = version->get_value ();




  void
  WINAPI
  SK_D3D11_SetResourceRoot (const wchar_t* root);
  SK_D3D11_SetResourceRoot (config.textures.d3d11.res_root.c_str ());


  //
  // EMERGENCY OVERRIDES
  //
  config.input.ui.use_raw_input = false;



  config.imgui.font.default.file  = "arial.ttf";
  config.imgui.font.default.size  = 18.0f;

  config.imgui.font.japanese.file = "msgothic.ttc";
  config.imgui.font.japanese.size = 18.0f;

  config.imgui.font.cyrillic.file = "arial.ttf";
  config.imgui.font.cyrillic.size = 18.0f;

  config.imgui.font.korean.file   = "malgun.ttf";
  config.imgui.font.korean.size   = 18.0f;
                                  
  config.imgui.font.chinese.file  = "msyh.ttc";
  config.imgui.font.chinese.size  = 18.0f;



  static bool scanned = false;

  if ((! scanned) && (! config.window.res.override.isZero ()))
  {
    scanned = true;

    if (games.count (std::wstring (SK_GetHostApp ())))
    {
      switch (games [std::wstring (SK_GetHostApp ())])
      {
        case SK_GAME_ID::GalGun_Double_Peace:
        {
          CreateThread (nullptr, 0, [](LPVOID) ->
          DWORD
          {
            // Wait for the image relocation to settle down, or we'll probably
            //   break the memory scanner.
            WaitForInputIdle (GetCurrentProcess (), 3333UL);

            void
            SK_ResHack_PatchGame (uint32_t w, uint32_t h);

            SK_ResHack_PatchGame (1920, 1080);

            CloseHandle (GetCurrentThread ());

            return 0;
          }, nullptr, 0x00, nullptr);
        } break;


        case SK_GAME_ID::YS_Seven:
        {
          CreateThread (nullptr, 0, [ ] (LPVOID) ->
                        DWORD
          {
            // Wait for the image relocation to settle down, or we'll probably
            //   break the memory scanner.
            WaitForInputIdle (GetCurrentProcess (), 3333UL);
        
            void
              SK_ResHack_PatchGame2 (uint32_t w, uint32_t h);
        
            SK_ResHack_PatchGame2 (1920, 1080);
        
            CloseHandle (GetCurrentThread ());
        
            return 0;
          }, nullptr, 0x00, nullptr);
        } break;


        case SK_GAME_ID::AKIBAs_Trip:
        {
          CreateThread (nullptr, 0, [](LPVOID) ->
          DWORD
          {
            // Wait for the image relocation to settle down, or we'll probably
            //   break the memory scanner.
            WaitForInputIdle (GetCurrentProcess (), 3333UL);

            void
            SK_ResHack_PatchGame (uint32_t w, uint32_t h);

            SK_ResHack_PatchGame (1920, 1080);

            CloseHandle (GetCurrentThread ());

            return 0;
          }, nullptr, 0x00, nullptr);
        } break;
      }
    }
  }



  //if ( SK_GetDLLRole () == DLL_ROLE::D3D8 ||
  //     SK_GetDLLRole () == DLL_ROLE::DDraw )
  //{
  //  config.render.dxgi.safe_fullscreen = true;
  //}



  if (empty)
    return false;

  return true;
}

void
SK_ResHack_PatchGame ( uint32_t width,
                       uint32_t height )
{
  static unsigned int replacements = 0;

  struct
  {
    struct
    {
      uint32_t w, h;
    } pattern;

    struct
    {
      uint32_t w = config.window.res.override.x,
               h = config.window.res.override.y;
    } replacement;
  } res_mod;

  res_mod.pattern.w = width;
  res_mod.pattern.h = height;

        uint32_t* pOut;
  const void*     pPattern = &res_mod.pattern;

  pOut =
    reinterpret_cast <uint32_t *> (
      nullptr
    );


  for (int i = 0 ; i < 3; i++)
  {
    pOut =
      static_cast <uint32_t *> (
        SK_ScanAlignedEx ( pPattern, 8, nullptr, pOut, 8 )
      );


    if (pOut != nullptr)
    {
      if ( SK_InjectMemory ( pOut,
                               &res_mod.replacement.w,
                                8,
                                  PAGE_READWRITE )
         )
      {
        ++replacements;
      }

      pOut += 8;
    }

    else
    {
      dll_log.Log ( L"[GalGunHACK] ** %lu Resolution Replacements Made  ==>  "
                                         L"( %lux%lu --> %lux%lu )",
                      replacements,
                        width, height,
                          res_mod.replacement.w, res_mod.replacement.h );
      break;
    }
  }
}

void
SK_ResHack_PatchGame2 ( uint32_t width,
                        uint32_t height )
{
  static unsigned int replacements = 0;

  uint32_t orig [2] = { 0x00000000,
                        0x00000000 };

  *(orig + 0) = width;
  *(orig + 1) = height;

  auto* pOut = reinterpret_cast <uint32_t *> (nullptr);
    //reinterpret_cast  <uint32_t *> (GetModuleHandle (nullptr));

  for (int i = 0 ; i < 5; i++)
  {
    pOut =
      static_cast <uint32_t *> (
        SK_ScanAlignedEx (orig, 8, nullptr, pOut, 4)
      );

    if (pOut != nullptr)
    {
      struct {
        uint32_t w = static_cast <uint32_t> (config.window.res.override.x),
                 h = static_cast <uint32_t> (config.window.res.override.y);
      } out_data;


      if ( SK_InjectMemory ( pOut,
                               &out_data.w,
                                 8,
                                   PAGE_READWRITE )
         )
      {
        ++replacements;
      }

      pOut += 8;
    }

    if (pOut == nullptr)
    {
      dll_log.Log ( L"[AkibasHACK] ** %lu Resolution Replacements Made  ==>  "
                                      L"( %lux%lu --> %lux%lu )",
                      replacements,
                        width, height,
                          config.window.res.override.x, config.window.res.override.y );
      break;
    }
  }
}

bool
SK_DeleteConfig (std::wstring name)
{
  wchar_t wszFullName [ MAX_PATH + 2 ] = { };

  lstrcatW (wszFullName, SK_GetConfigPath ());
  lstrcatW (wszFullName,       name.c_str ());
  lstrcatW (wszFullName,             L".ini");

  return (DeleteFileW (wszFullName) != FALSE);
}

void
SK_SaveConfig ( std::wstring name,
                bool         close_config )
{
  //
  // Shutting down before initializaiton would be damn near fatal if we didn't catch this! :)
  //
  if (dll_ini == nullptr)
    return;

  compatibility.ignore_raptr->set_value       (config.compatibility.ignore_raptr);
  compatibility.disable_raptr->set_value      (config.compatibility.disable_raptr);
  compatibility.disable_nv_bloat->set_value   (config.compatibility.disable_nv_bloat);
  compatibility.rehook_loadlibrary->set_value (config.compatibility.rehook_loadlibrary);

  monitoring.memory.show->set_value           (config.mem.show);
  mem_reserve->set_value                      (config.mem.reserve);

  monitoring.fps.show->set_value              (config.fps.show);

  monitoring.io.show->set_value               (config.io.show);
  monitoring.io.interval->set_value           (config.io.interval);

  monitoring.cpu.show->set_value              (config.cpu.show);
  monitoring.cpu.interval->set_value          (config.cpu.interval);
  monitoring.cpu.simple->set_value            (config.cpu.simple);

  monitoring.gpu.show->set_value              (config.gpu.show);
  monitoring.gpu.print_slowdown->set_value    (config.gpu.print_slowdown);
  monitoring.gpu.interval->set_value          (config.gpu.interval);

  monitoring.disk.show->set_value             (config.disk.show);
  monitoring.disk.interval->set_value         (config.disk.interval);
  monitoring.disk.type->set_value             (config.disk.type);

  monitoring.pagefile.show->set_value         (config.pagefile.show);
  monitoring.pagefile.interval->set_value     (config.pagefile.interval);

  if (! (nvapi_init && sk::NVAPI::nv_hardware && sk::NVAPI::CountSLIGPUs () > 1))
    config.sli.show = false;

  monitoring.SLI.show->set_value              (config.sli.show);
  monitoring.time.show->set_value             (config.time.show);

  osd.show->set_value                         (config.osd.show);
  osd.update_method.pump->set_value           (config.osd.pump);
  osd.update_method.pump_interval->set_value  (config.osd.pump_interval);
  osd.text.red->set_value                     (config.osd.red);
  osd.text.green->set_value                   (config.osd.green);
  osd.text.blue->set_value                    (config.osd.blue);
  osd.viewport.pos_x->set_value               (config.osd.pos_x);
  osd.viewport.pos_y->set_value               (config.osd.pos_y);
  osd.viewport.scale->set_value               (config.osd.scale);
  osd.state.remember->set_value               (config.osd.remember_state);

  imgui.scale->set_value                      (config.imgui.scale);
  imgui.show_eula->set_value                  (config.imgui.show_eula);
  imgui.show_playtime->set_value              (config.steam.show_playtime);
  imgui.show_gsync_status->set_value          (config.apis.NvAPI.gsync_status);
  imgui.mac_style_menu->set_value             (config.imgui.use_mac_style_menu);
  imgui.show_input_apis->set_value            (config.imgui.show_input_apis);


  apis.last_known->set_value                  (static_cast <int> (config.apis.last_known));

#ifndef _WIN64
  apis.ddraw.hook->set_value                  (config.apis.ddraw.hook);
  apis.d3d8.hook->set_value                   (config.apis.d3d8.hook);
#endif
  apis.d3d9.hook->set_value                   (config.apis.d3d9.hook);
  apis.d3d9ex.hook->set_value                 (config.apis.d3d9ex.hook);
  apis.d3d11.hook->set_value                  (config.apis.dxgi.d3d11.hook);
  apis.OpenGL.hook->set_value                 (config.apis.OpenGL.hook);
#ifdef _WIN64
  apis.d3d12.hook->set_value                  (config.apis.dxgi.d3d12.hook);
  apis.Vulkan.hook->set_value                 (config.apis.Vulkan.hook);
#endif

  input.cursor.manage->set_value              (config.input.cursor.manage);
  input.cursor.keys_activate->set_value       (config.input.cursor.keys_activate);
  input.cursor.timeout->set_value             (static_cast <float> (config.input.cursor.timeout) / 1000.0f);
  input.cursor.ui_capture->set_value          (config.input.ui.capture);
  input.cursor.hw_cursor->set_value           (config.input.ui.use_hw_cursor);
  input.cursor.block_invisible->set_value     (config.input.ui.capture_hidden);
  input.cursor.no_warp_ui->set_value          (SK_ImGui_Cursor.prefs.no_warp.ui_open);
  input.cursor.no_warp_visible->set_value     (SK_ImGui_Cursor.prefs.no_warp.visible);
  input.cursor.fix_synaptics->set_value       (config.input.mouse.fix_synaptics);
  input.cursor.antiwarp_deadzone->set_value   (config.input.mouse.antiwarp_deadzone);
  input.cursor.use_relative_input->set_value  (config.input.mouse.add_relative_motion);

  input.gamepad.disable_ps4_hid->set_value    (config.input.gamepad.disable_ps4_hid);
  input.gamepad.rehook_xinput->set_value      (config.input.gamepad.rehook_xinput);
  input.gamepad.haptic_ui->set_value          (config.input.gamepad.haptic_ui);

  int placeholder_mask = 0x0;

  placeholder_mask |= (config.input.gamepad.xinput.placehold [0] ? 0x1 : 0x0);
  placeholder_mask |= (config.input.gamepad.xinput.placehold [1] ? 0x2 : 0x0);
  placeholder_mask |= (config.input.gamepad.xinput.placehold [2] ? 0x4 : 0x0);
  placeholder_mask |= (config.input.gamepad.xinput.placehold [3] ? 0x8 : 0x0);

  input.gamepad.xinput.placeholders->set_value (placeholder_mask);
  input.gamepad.xinput.ui_slot->set_value      (config.input.gamepad.xinput.ui_slot);

  std::wstring xinput_assign = L"";

  for (int i = 0; i < 4; i++)
  {
    xinput_assign += std::to_wstring (
      config.input.gamepad.xinput.assignment [i]
    );

    if (i != 3)
      xinput_assign += L",";
  }

  input.gamepad.xinput.assignment->set_value (xinput_assign);

  input.gamepad.xinput.disable_rumble->set_value (config.input.gamepad.xinput.disable_rumble);

  window.borderless->set_value                (config.window.borderless);
  window.center->set_value                    (config.window.center);
  window.background_render->set_value         (config.window.background_render);
  window.background_mute->set_value           (config.window.background_mute);
  if (config.window.offset.x.absolute != 0)
  {
    wchar_t wszAbsolute [16];
    _swprintf (wszAbsolute, L"%li", config.window.offset.x.absolute);
    window.offset.x->set_value (wszAbsolute);
  }

  else
  {
    wchar_t wszPercent [16];
    _swprintf (wszPercent, L"%08.6f", 100.0f * config.window.offset.x.percent);
    SK_RemoveTrailingDecimalZeros (wszPercent);
    lstrcatW (wszPercent, L"%");
    window.offset.x->set_value (wszPercent);
  }

  if (config.window.offset.y.absolute != 0)
  {
    wchar_t wszAbsolute [16];
    _swprintf (wszAbsolute, L"%li", config.window.offset.y.absolute);
    window.offset.y->set_value (wszAbsolute);
  }

  else
  {
    wchar_t wszPercent [16];
    _swprintf (wszPercent, L"%08.6f", 100.0f * config.window.offset.y.percent);
    SK_RemoveTrailingDecimalZeros (wszPercent);
    lstrcatW (wszPercent, L"%");
    window.offset.y->set_value (wszPercent);
  }

  window.confine_cursor->set_value            (config.window.confine_cursor);
  window.unconfine_cursor->set_value          (config.window.unconfine_cursor);
  window.persistent_drag->set_value           (config.window.persistent_drag);
  window.fullscreen->set_value                (config.window.fullscreen);
  window.fix_mouse_coords->set_value          (config.window.res.override.fix_mouse);

  wchar_t wszFormattedRes [64] = { };

  wsprintf ( wszFormattedRes, L"%lux%lu",
               config.window.res.override.x,
                 config.window.res.override.y );

  window.override->set_value (wszFormattedRes);

  extern float target_fps;

  display.force_fullscreen->set_value (config.display.force_fullscreen);
  display.force_windowed->set_value   (config.display.force_windowed);

  render.framerate.target_fps->set_value        (target_fps);
  render.framerate.limiter_tolerance->set_value (config.render.framerate.limiter_tolerance);
  render.framerate.sleepless_render->set_value  (config.render.framerate.sleepless_render);
  render.framerate.sleepless_window->set_value  (config.render.framerate.sleepless_window);

  if ( SK_IsInjected () || (SK_GetDLLRole () & DLL_ROLE::DInput8) ||
      (SK_GetDLLRole () & DLL_ROLE::D3D9 || SK_GetDLLRole () & DLL_ROLE::DXGI) )
  {
    render.framerate.wait_for_vblank->set_value   (config.render.framerate.wait_for_vblank);
    render.framerate.prerender_limit->set_value   (config.render.framerate.pre_render_limit);
    render.framerate.buffer_count->set_value      (config.render.framerate.buffer_count);
    render.framerate.present_interval->set_value  (config.render.framerate.present_interval);

    if (render.framerate.refresh_rate != nullptr)
      render.framerate.refresh_rate->set_value (config.render.framerate.refresh_rate);

    // SLI only works in Direct3D
    nvidia.sli.compatibility->set_value          (config.nvidia.sli.compatibility);
    nvidia.sli.mode->set_value                   (config.nvidia.sli.mode);
    nvidia.sli.num_gpus->set_value               (config.nvidia.sli.num_gpus);
    nvidia.sli.override->set_value               (config.nvidia.sli.override);

    if (  SK_IsInjected ()                       ||
        ( SK_GetDLLRole () & DLL_ROLE::DInput8 ) ||
        ( SK_GetDLLRole () & DLL_ROLE::DXGI    ) )
    {
      render.framerate.max_delta_time->set_value    (config.render.framerate.max_delta_time);
      render.framerate.flip_discard->set_value      (config.render.framerate.flip_discard);
      render.framerate.allow_dwm_tearing->set_value (config.render.dxgi.allow_tearing);

      texture.d3d11.cache->set_value        (config.textures.d3d11.cache);
      texture.d3d11.precise_hash->set_value (config.textures.d3d11.precise_hash);
      texture.d3d11.dump->set_value         (config.textures.d3d11.dump);
      texture.d3d11.inject->set_value       (config.textures.d3d11.inject);
      texture.d3d11.res_root->set_value     (config.textures.d3d11.res_root);

      texture.cache.max_entries->set_value (config.textures.cache.max_entries);
      texture.cache.min_entries->set_value (config.textures.cache.min_entries);
      texture.cache.max_evict->set_value   (config.textures.cache.max_evict);
      texture.cache.min_evict->set_value   (config.textures.cache.min_evict);
      texture.cache.max_size->set_value    (config.textures.cache.max_size);
      texture.cache.min_size->set_value    (config.textures.cache.min_size);

      texture.cache.ignore_non_mipped->set_value (config.textures.cache.ignore_nonmipped);
      texture.cache.allow_staging->set_value     (config.textures.cache.allow_staging);

      wsprintf ( wszFormattedRes, L"%lux%lu",
                   config.render.dxgi.res.max.x,
                     config.render.dxgi.res.max.y );

      render.dxgi.max_res->set_value (wszFormattedRes);

      wsprintf ( wszFormattedRes, L"%lux%lu",
                   config.render.dxgi.res.min.x,
                     config.render.dxgi.res.min.y );

      render.dxgi.min_res->set_value (wszFormattedRes);

      render.dxgi.swapchain_wait->set_value (config.render.framerate.swapchain_wait);

      switch (config.render.dxgi.scaling_mode)
      {
        case DXGI_MODE_SCALING_UNSPECIFIED:
          render.dxgi.scaling_mode->set_value (L"Unspecified");
          break;
        case DXGI_MODE_SCALING_CENTERED:
          render.dxgi.scaling_mode->set_value (L"Centered");
          break;
        case DXGI_MODE_SCALING_STRETCHED:
          render.dxgi.scaling_mode->set_value (L"Stretched");
          break;
        default:
          render.dxgi.scaling_mode->set_value (L"DontCare");
          break;
      }

      switch (config.render.dxgi.scanline_order)
      {
        case DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED:
          render.dxgi.scanline_order->set_value (L"Unspecified");
          break;
        case DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE:
          render.dxgi.scanline_order->set_value (L"Progressive");
          break;
        case DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST:
          render.dxgi.scanline_order->set_value (L"LowerFieldFirst");
          break;
        case DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST:
          render.dxgi.scanline_order->set_value (L"UpperFieldFirst");
          break;
        default:
          render.dxgi.scanline_order->set_value (L"DontCare");
          break;
      }

      switch (config.render.dxgi.exception_mode)
      {
        case D3D11_RAISE_FLAG_DRIVER_INTERNAL_ERROR:
          render.dxgi.exception_mode->set_value (L"Raise");
          break;
        case 0:
          render.dxgi.exception_mode->set_value (L"Ignore");
          break;
        default:
          render.dxgi.exception_mode->set_value (L"DontCare");
          break;
      }

      render.dxgi.debug_layer->set_value     (config.render.dxgi.debug_layer);
      render.dxgi.safe_fullscreen->set_value (config.render.dxgi.safe_fullscreen);
      render.dxgi.enhanced_depth->set_value  (config.render.dxgi.enhanced_depth);
    }

    if ( SK_IsInjected () || ( SK_GetDLLRole () & DLL_ROLE::D3D9    ) ||
                             ( SK_GetDLLRole () & DLL_ROLE::DInput8 ) )
    {
      render.d3d9.force_d3d9ex->set_value        (config.render.d3d9.force_d3d9ex);
      render.d3d9.hook_type->set_value           (config.render.d3d9.hook_type);
      render.d3d9.enable_texture_mods->set_value (config.textures.d3d9_mod);
    }
  }

  texture.res_root->set_value                   (config.textures.d3d11.res_root);
  texture.dump_on_load->set_value               (config.textures.dump_on_load);

  steam.achievements.sound_file->set_value      (config.steam.achievements.sound_file);
  steam.achievements.play_sound->set_value      (config.steam.achievements.play_sound);
  steam.achievements.take_screenshot->set_value (config.steam.achievements.take_screenshot);
  steam.achievements.popup.origin->set_value    (
    SK_Steam_PopupOriginToWStr (config.steam.achievements.popup.origin)
  );
  steam.achievements.popup.inset->set_value      (config.steam.achievements.popup.inset);

  if (! config.steam.achievements.popup.show)
    config.steam.achievements.popup.duration = 0;

  steam.achievements.popup.duration->set_value   (config.steam.achievements.popup.duration);
  steam.achievements.popup.animate->set_value    (config.steam.achievements.popup.animate);
  steam.achievements.popup.show_title->set_value (config.steam.achievements.popup.show_title);

  if (config.steam.appid == 0) {
    if (SK::SteamAPI::AppID () != 0 &&
        SK::SteamAPI::AppID () != 1)
      config.steam.appid = SK::SteamAPI::AppID ();
  }

  steam.system.appid->set_value                 (config.steam.appid);
  steam.system.init_delay->set_value            (config.steam.init_delay);
  steam.system.auto_pump->set_value             (config.steam.auto_pump_callbacks);
  steam.system.block_stat_callback->set_value   (config.steam.block_stat_callback);
  steam.system.filter_stat_callbacks->set_value (config.steam.filter_stat_callback);
  steam.system.load_early->set_value            (config.steam.preload_client);
  steam.system.early_overlay->set_value         (config.steam.preload_overlay);
  steam.system.force_load->set_value            (config.steam.force_load_steamapi);
  steam.system.notify_corner->set_value         (
    SK_Steam_PopupOriginToWStr (config.steam.notify_corner)
  );

  steam.log.silent->set_value                (config.steam.silent);
  steam.drm.spoof_BLoggedOn->set_value       (config.steam.spoof_BLoggedOn);

  init_delay->set_value                      (config.system.init_delay);
  silent->set_value                          (config.system.silent);
  log_level->set_value                       (config.system.log_level);
  prefer_fahrenheit->set_value               (config.system.prefer_fahrenheit);

  apis.last_known->store                  ();
#ifndef _WIN64
  apis.ddraw.hook->store                  ();
  apis.d3d8.hook->store                   ();
#endif
  apis.d3d9.hook->store                   ();
  apis.d3d9ex.hook->store                 ();
  apis.d3d11.hook->store                  ();
#ifdef _WIN64
  apis.d3d12.hook->store                  ();
#endif
  apis.OpenGL.hook->store                 ();
#ifdef _WIN64
  apis.Vulkan.hook->store                 ();
#endif

  compatibility.ignore_raptr->store       ();
  compatibility.disable_raptr->store      ();
  compatibility.disable_nv_bloat->store   ();
  compatibility.rehook_loadlibrary->store ();

  osd.state.remember->store               ();

  monitoring.memory.show->store           ();
  mem_reserve->store                      ();

  monitoring.SLI.show->store              ();
  monitoring.time.show->store             ();

  monitoring.fps.show->store              ();

  monitoring.io.show->store               ();
  monitoring.io.interval->store           ();

  monitoring.cpu.show->store               ();
  monitoring.cpu.interval->store           ();
  monitoring.cpu.simple->store             ();

  monitoring.gpu.show->store               ();
  monitoring.gpu.print_slowdown->store     ();
  monitoring.gpu.interval->store           ();

  monitoring.disk.show->store              ();
  monitoring.disk.interval->store          ();
  monitoring.disk.type->store              ();

  monitoring.pagefile.show->store          ();
  monitoring.pagefile.interval->store      ();

  input.cursor.manage->store               ();
  input.cursor.keys_activate->store        ();
  input.cursor.timeout->store              ();
  input.cursor.ui_capture->store           ();
  input.cursor.hw_cursor->store            ();
  input.cursor.block_invisible->store      ();
  input.cursor.no_warp_ui->store           ();
  input.cursor.no_warp_visible->store      ();
  input.cursor.fix_synaptics->store        ();
  input.cursor.antiwarp_deadzone->store    ();
  input.cursor.use_relative_input->store   ();

  input.gamepad.disable_ps4_hid->store       ();
  input.gamepad.rehook_xinput->store         ();
  input.gamepad.xinput.ui_slot->store        ();
  input.gamepad.xinput.placeholders->store   ();
  input.gamepad.xinput.disable_rumble->store ();
  input.gamepad.haptic_ui->store             ();
  input.gamepad.xinput.assignment->store   ();

  window.borderless->store                 ();
  window.center->store                     ();
  window.background_render->store          ();
  window.background_mute->store            ();
  window.offset.x->store                   ();
  window.offset.y->store                   ();
  window.confine_cursor->store             ();
  window.unconfine_cursor->store           ();
  window.persistent_drag->store            ();
  window.fullscreen->store                 ();
  window.fix_mouse_coords->store           ();
  window.override->store                   ();

  nvidia.api.disable->store                ();

  display.force_fullscreen->store          ();
  display.force_windowed->store            ();

  render.framerate.target_fps->store        ();
  render.framerate.limiter_tolerance->store ();
  render.framerate.sleepless_render->store  ();
  render.framerate.sleepless_window->store  ();

  if (  SK_IsInjected ()                     || 
      ( SK_GetDLLRole () & DLL_ROLE::DInput8 ||
        SK_GetDLLRole () & DLL_ROLE::DXGI    ||
        SK_GetDLLRole () & DLL_ROLE::D3D9 ) )
  {
    render.framerate.wait_for_vblank->store   ();
    render.framerate.buffer_count->store      ();
    render.framerate.prerender_limit->store   ();
    render.framerate.present_interval->store  ();

    if (sk::NVAPI::nv_hardware)
    {
      nvidia.sli.compatibility->store        ();
      nvidia.sli.mode->store                 ();
      nvidia.sli.num_gpus->store             ();
      nvidia.sli.override->store             ();
    }

    if (  SK_IsInjected ()                     ||
        ( SK_GetDLLRole () & DLL_ROLE::DInput8 ||
          SK_GetDLLRole () & DLL_ROLE::DXGI ) )
    {
      render.framerate.max_delta_time->store    ();
      render.framerate.flip_discard->store      ();
      render.framerate.allow_dwm_tearing->store ();

      texture.d3d11.cache->store        ();
      texture.d3d11.precise_hash->store ();
      texture.d3d11.dump->store         ();
      texture.d3d11.inject->store       ();
      texture.d3d11.res_root->store     ();

      texture.cache.max_entries->store ();
      texture.cache.min_entries->store ();
      texture.cache.max_evict->store   ();
      texture.cache.min_evict->store   ();
      texture.cache.max_size->store    ();
      texture.cache.min_size->store    ();

      texture.cache.ignore_non_mipped->store ();
      texture.cache.allow_staging->store     ();

      render.dxgi.max_res->store         ();
      render.dxgi.min_res->store         ();
      render.dxgi.scaling_mode->store    ();
      render.dxgi.scanline_order->store  ();
      render.dxgi.exception_mode->store  ();
      render.dxgi.debug_layer->store     ();
      render.dxgi.safe_fullscreen->store ();
      render.dxgi.enhanced_depth->store  ();

      render.dxgi.swapchain_wait->store ();
    }

    if (  SK_IsInjected ()                       ||
        ( SK_GetDLLRole () & DLL_ROLE::DInput8 ) ||
        ( SK_GetDLLRole () & DLL_ROLE::D3D9    ) )
    {
      render.d3d9.force_d3d9ex->store        ();
      render.d3d9.hook_type->store           ();
      render.d3d9.enable_texture_mods->store ();
    }
  }

  if (render.framerate.refresh_rate != nullptr)
    render.framerate.refresh_rate->store ();

  texture.res_root->store                ();
  texture.dump_on_load->store            ();

  osd.show->store                        ();
  osd.update_method.pump->store          ();
  osd.update_method.pump_interval->store ();
  osd.text.red->store                    ();
  osd.text.green->store                  ();
  osd.text.blue->store                   ();
  osd.viewport.pos_x->store              ();
  osd.viewport.pos_y->store              ();
  osd.viewport.scale->store              ();

  imgui.scale->store                     ();
  imgui.show_playtime->store             ();
  imgui.show_eula->store                 ();
  imgui.show_gsync_status->store         ();
  imgui.mac_style_menu->store            ();
  imgui.show_input_apis->store           ();

  steam.achievements.sound_file->store       ();
  steam.achievements.play_sound->store       ();
  steam.achievements.take_screenshot->store  ();
  steam.achievements.popup.show_title->store ();
  steam.achievements.popup.animate->store    ();
  steam.achievements.popup.duration->store   ();
  steam.achievements.popup.inset->store      ();
  steam.achievements.popup.origin->store     ();
  steam.system.notify_corner->store          ();
  steam.system.appid->store                  ();
  steam.system.init_delay->store             ();
  steam.system.auto_pump->store              ();
  steam.system.block_stat_callback->store    ();
  steam.system.filter_stat_callbacks->store  ();
  steam.system.load_early->store             ();
  steam.system.early_overlay->store          ();
  steam.system.force_load->store             ();
  steam.log.silent->store                    ();
  steam.drm.spoof_BLoggedOn->store           ();

  init_delay->store                      ();
  silent->store                          ();
  log_level->store                       ();
  prefer_fahrenheit->store               ();

  ignore_rtss_delay->set_value           (config.system.ignore_rtss_delay);
  ignore_rtss_delay->store               ();


  // Don't store this setting at shutdown
  extern volatile ULONG __SK_DLL_Ending;

  if (! InterlockedExchangeAdd (&__SK_DLL_Ending, 0))
  {
    handle_crashes->set_value              (config.system.handle_crashes);
    handle_crashes->store                  ();
  }

  game_output->set_value                 (config.system.game_output);
  game_output->store                     ();

  // Only add this to the INI file if it differs from default
  if (config.system.display_debug_out != debug_output->get_value ())
  {
    debug_output->set_value              (config.system.display_debug_out);
    debug_output->store                  ();
  }

  enable_cegui->set_value                (config.cegui.enable);
  enable_cegui->store                    ();

  safe_cegui->set_value                  (config.cegui.safe_init);
  safe_cegui->store                      ();

  trace_libraries->set_value             (config.system.trace_load_library);
  trace_libraries->store                 ();

  strict_compliance->set_value           (config.system.strict_compliance);
  strict_compliance->store               ();

  version->set_value                     (SK_VER_STR);
  version->store                         ();

  if (! (nvapi_init && sk::NVAPI::nv_hardware))
    dll_ini->remove_section (L"NVIDIA.SLI");

  wchar_t wszFullName [ MAX_PATH + 2 ] = { };

  lstrcatW (wszFullName, SK_GetConfigPath ());
  lstrcatW (wszFullName,       name.c_str ());
  lstrcatW (wszFullName,             L".ini");

  SK_ImGui_Widgets.SaveConfig ();

  dll_ini->write ( wszFullName );
  osd_ini->write ( std::wstring ( SK_GetDocumentsDir () +
                     L"\\My Mods\\SpecialK\\Global\\osd.ini"
                   ).c_str () );
  achievement_ini->write ( std::wstring ( SK_GetDocumentsDir () +
                     L"\\My Mods\\SpecialK\\Global\\achievements.ini"
                   ).c_str () );

  if (close_config)
  {
    if (dll_ini != nullptr)
    {
      delete dll_ini;
      dll_ini = nullptr;
    }

    if (osd_ini != nullptr)
    {
      delete osd_ini;
      osd_ini = nullptr;
    }

    if (achievement_ini != nullptr)
    {
      delete achievement_ini;
      achievement_ini = nullptr;
    }
  }
}

const wchar_t*
__stdcall
SK_GetVersionStr (void)
{
  return SK_VER_STR;
}


#include <unordered_map>

std::unordered_map <std::wstring, BYTE> humanKeyNameToVirtKeyCode;
std::unordered_map <BYTE, std::wstring> virtKeyCodeToHumanKeyName;

#include <queue>

#define SK_MakeKeyMask(vKey,ctrl,shift,alt) \
  (UINT)((vKey) | (((ctrl) != 0) <<  9) |   \
                  (((shift)!= 0) << 10) |   \
                  (((alt)  != 0) << 11))

void
SK_Keybind::update (void)
{
  human_readable = L"";

  std::wstring key_name = virtKeyCodeToHumanKeyName [(BYTE)(vKey & 0xFF)];

  if (! key_name.length ())
    return;

  std::queue <std::wstring> words;

  if (ctrl)
    words.push (L"Ctrl");

  if (alt)
    words.push (L"Alt");

  if (shift)
    words.push (L"Shift");

  words.push (key_name);

  while (! words.empty ())
  {
    human_readable += words.front ();
    words.pop ();

    if (! words.empty ())
      human_readable += L"+";
  }

  masked_code = SK_MakeKeyMask (vKey & 0xFF, ctrl, shift, alt);
}

void
SK_Keybind::parse (void)
{
  vKey = 0x00;

  static bool init = false;

  if (! init)
  {
    init = true;

    for (int i = 0; i < 0xFF; i++)
    {
      wchar_t name [32] = { };

      switch (i)
      {
        case VK_F1:     wcscat (name, L"F1");           break;
        case VK_F2:     wcscat (name, L"F2");           break;
        case VK_F3:     wcscat (name, L"F3");           break;
        case VK_F4:     wcscat (name, L"F4");           break;
        case VK_F5:     wcscat (name, L"F5");           break;
        case VK_F6:     wcscat (name, L"F6");           break;
        case VK_F7:     wcscat (name, L"F7");           break;
        case VK_F8:     wcscat (name, L"F8");           break;
        case VK_F9:     wcscat (name, L"F9");           break;
        case VK_F10:    wcscat (name, L"F10");          break;
        case VK_F11:    wcscat (name, L"F11");          break;
        case VK_F12:    wcscat (name, L"F12");          break;
        case VK_F13:    wcscat (name, L"F13");          break;
        case VK_F14:    wcscat (name, L"F14");          break;
        case VK_F15:    wcscat (name, L"F15");          break;
        case VK_F16:    wcscat (name, L"F16");          break;
        case VK_F17:    wcscat (name, L"F17");          break;
        case VK_F18:    wcscat (name, L"F18");          break;
        case VK_F19:    wcscat (name, L"F19");          break;
        case VK_F20:    wcscat (name, L"F20");          break;
        case VK_F21:    wcscat (name, L"F21");          break;
        case VK_F22:    wcscat (name, L"F22");          break;
        case VK_F23:    wcscat (name, L"F23");          break;
        case VK_F24:    wcscat (name, L"F24");          break;
        case VK_PRINT:  wcscat (name, L"Print Screen"); break;
        case VK_SCROLL: wcscat (name, L"Scroll Lock");  break;
        case VK_PAUSE:  wcscat (name, L"Pause Break");  break;

        default:
        {
          unsigned int scanCode =
            ( MapVirtualKey (i, 0) & 0xFF );

                        BYTE buf [256] = { };
          unsigned short int temp      =  0;
          
          bool asc = (i <= 32);

          if (! asc && i != VK_DIVIDE)
             asc = ToAscii ( i, scanCode, buf, &temp, 1 );

          scanCode            <<= 16;
          scanCode   |= ( 0x1 <<  25  );

          if (! asc)
            scanCode |= ( 0x1 << 24   );
    
          GetKeyNameText ( scanCode,
                             name,
                               32 );
        } break;
      }

    
      if ( i != VK_CONTROL  && i != VK_MENU     &&
           i != VK_SHIFT    && i != VK_OEM_PLUS && i != VK_OEM_MINUS &&
           i != VK_LSHIFT   && i != VK_RSHIFT   &&
           i != VK_LCONTROL && i != VK_RCONTROL &&
           i != VK_LMENU    && i != VK_RMENU )
      {

        humanKeyNameToVirtKeyCode.emplace (name, (BYTE)i);
        virtKeyCodeToHumanKeyName.emplace ((BYTE)i, name);
      }
    }
    
    humanKeyNameToVirtKeyCode.emplace (L"Plus",        (BYTE)VK_OEM_PLUS);
    humanKeyNameToVirtKeyCode.emplace (L"Minus",       (BYTE)VK_OEM_MINUS);
    humanKeyNameToVirtKeyCode.emplace (L"Ctrl",        (BYTE)VK_CONTROL);
    humanKeyNameToVirtKeyCode.emplace (L"Alt",         (BYTE)VK_MENU);
    humanKeyNameToVirtKeyCode.emplace (L"Shift",       (BYTE)VK_SHIFT);
    humanKeyNameToVirtKeyCode.emplace (L"Left Shift",  (BYTE)VK_LSHIFT);
    humanKeyNameToVirtKeyCode.emplace (L"Right Shift", (BYTE)VK_RSHIFT);
    humanKeyNameToVirtKeyCode.emplace (L"Left Alt",    (BYTE)VK_LMENU);
    humanKeyNameToVirtKeyCode.emplace (L"Right Alt",   (BYTE)VK_RMENU);
    humanKeyNameToVirtKeyCode.emplace (L"Left Ctrl",   (BYTE)VK_LCONTROL);
    humanKeyNameToVirtKeyCode.emplace (L"Right Ctrl",  (BYTE)VK_RCONTROL);
    
    virtKeyCodeToHumanKeyName.emplace ((BYTE)VK_CONTROL,   L"Ctrl");
    virtKeyCodeToHumanKeyName.emplace ((BYTE)VK_MENU,      L"Alt");
    virtKeyCodeToHumanKeyName.emplace ((BYTE)VK_SHIFT,     L"Shift");
    virtKeyCodeToHumanKeyName.emplace ((BYTE)VK_OEM_PLUS,  L"Plus");
    virtKeyCodeToHumanKeyName.emplace ((BYTE)VK_OEM_MINUS, L"Minus");
    virtKeyCodeToHumanKeyName.emplace ((BYTE)VK_LSHIFT,    L"Left Shift");
    virtKeyCodeToHumanKeyName.emplace ((BYTE)VK_RSHIFT,    L"Right Shift");
    virtKeyCodeToHumanKeyName.emplace ((BYTE)VK_LMENU,     L"Left Alt");
    virtKeyCodeToHumanKeyName.emplace ((BYTE)VK_RMENU,     L"Right Alt");  
    virtKeyCodeToHumanKeyName.emplace ((BYTE)VK_LCONTROL,  L"Left Ctrl"); 
    virtKeyCodeToHumanKeyName.emplace ((BYTE)VK_RCONTROL,  L"Right Ctrl");

    init = true;
  }

  wchar_t wszKeyBind [128] = { };

  lstrcatW (wszKeyBind, human_readable.c_str ());

  wchar_t* wszBuf = nullptr;
  wchar_t* wszTok = std::wcstok (wszKeyBind, L"+", &wszBuf);

  ctrl  = false;
  alt   = false;
  shift = false;

  if (wszTok == nullptr)
  {
    vKey = humanKeyNameToVirtKeyCode [wszKeyBind];
  }

  while (wszTok)
  {
    BYTE vKey_ = humanKeyNameToVirtKeyCode [wszTok];

    if (vKey_ == VK_CONTROL)
      ctrl  = true;
    else if (vKey_ == VK_SHIFT)
      shift = true;
    else if (vKey_ == VK_MENU)
      alt   = true;
    else
      vKey = vKey_;

    wszTok = std::wcstok (nullptr, L"+", &wszBuf);
  }

  masked_code = SK_MakeKeyMask (vKey & 0xFF, ctrl, shift, alt);
}




#include <SpecialK/utility.h>


bool
SK_AppCache_Manager::loadAppCacheForExe (const wchar_t* wszExe)
{
  std::wstring naive_name =
    SK_GetNaiveConfigPath ();

  wchar_t* wszPath =
    StrStrIW (wszExe, L"SteamApps\\common\\");

  if (wszPath != nullptr)
  {
    wchar_t* wszRelPath =
      _wcsdup (CharNextW (StrStrIW (CharNextW (StrStrIW (wszPath, L"\\")), L"\\")));

    PathRemoveFileSpecW (wszRelPath);

    std::wstring wstr_appcache =
     SK_FormatStringW ( L"%s\\..\\AppCache\\%s\\SpecialK.AppCache",
                          naive_name.c_str (),
                            wszRelPath );

    SK_CreateDirectories (wstr_appcache.c_str ());

    app_cache_db =
      SK_CreateINI (wstr_appcache.c_str ());

    app_cache_db->write (app_cache_db->get_filename ());

    free (wszRelPath);
  }

  if (app_cache_db != nullptr)
    return true;

  return false;
}

uint32_t
SK_AppCache_Manager::getAppIDFromPath (const wchar_t* wszPath) const
{
  if (app_cache_db == nullptr)
    return 0;

  iSK_INISection&
    fwd_map =
      app_cache_db->get_section (L"AppID_Cache.FwdMap");

  wchar_t* wszSteamApps =
    StrStrIW (wszPath, L"SteamApps\\common\\");

  if (wszSteamApps != nullptr)
  {
    wchar_t* wszRelPath =
      CharNextW (StrStrIW (CharNextW (StrStrIW (wszSteamApps, L"\\")), L"\\"));

    if (fwd_map.contains_key (wszRelPath))
    {
      return _wtoi (fwd_map.get_value (wszRelPath).c_str ());
    }
  }

  return 0;
}

std::wstring
SK_AppCache_Manager::getAppNameFromID (uint32_t uiAppID) const
{
  if (app_cache_db == nullptr)
    return L"";

  iSK_INISection&
    name_map =
      app_cache_db->get_section (L"AppID_Cache.Names");

  if (name_map.contains_key   (SK_FormatStringW (L"%u", uiAppID).c_str ()))
  {
    return name_map.get_value (SK_FormatStringW (L"%u", uiAppID).c_str ());
  }

  return L"";
}

std::wstring
SK_AppCache_Manager::getAppNameFromPath (const wchar_t* wszPath) const
{
  uint32_t uiAppID = getAppIDFromPath (wszPath);

  if (uiAppID != 0)
  {
    return getAppNameFromID (uiAppID);
  }

  return L"";
}

bool
SK_AppCache_Manager::addAppToCache ( const wchar_t* wszFullPath,
                                     const wchar_t*,
                                     const wchar_t* wszAppName,
                                           uint32_t uiAppID )
{
  if (! app_cache_db)
    return false;

  if (! StrStrIW (wszFullPath, L"SteamApps\\common\\"))
      return false;

  iSK_INISection& rev_map =
    app_cache_db->get_section (L"AppID_Cache.RevMap");
  iSK_INISection& fwd_map =
    app_cache_db->get_section (L"AppID_Cache.FwdMap");
  iSK_INISection& name_map =
    app_cache_db->get_section (L"AppID_Cache.Names");


  wchar_t* wszRelativePath = _wcsdup (wszFullPath);

  wchar_t* wszRelPath =
    CharNextW (StrStrIW (CharNextW (StrStrIW (StrStrIW (wszRelativePath, L"SteamApps\\common\\"), L"\\")), L"\\"));

  if (fwd_map.contains_key (wszRelPath))
    fwd_map.get_value (wszRelPath) = SK_FormatStringW   (L"%u", uiAppID).c_str ();
  else
    fwd_map.add_key_value (wszRelPath, SK_FormatStringW (L"%u", uiAppID).c_str ());


  if (rev_map.contains_key (SK_FormatStringW  (L"%u", uiAppID).c_str ()))
    rev_map.get_value (SK_FormatStringW       (L"%u", uiAppID).c_str ()) = wszRelPath;
  else
    rev_map.add_key_value (SK_FormatStringW   (L"%u", uiAppID).c_str (), wszRelPath);


  if (name_map.contains_key (SK_FormatStringW (L"%u", uiAppID).c_str ()))
    name_map.get_value (SK_FormatStringW      (L"%u", uiAppID).c_str ()) = wszAppName;
  else
    name_map.add_key_value (SK_FormatStringW  (L"%u", uiAppID).c_str (), wszAppName);


  app_cache_db->write (app_cache_db->get_filename ());


  free (wszRelativePath);

  return true;
}

std::wstring
SK_AppCache_Manager::getConfigPathFromAppPath (const wchar_t* wszPath) const
{
  return getConfigPathForAppID (getAppIDFromPath (wszPath));
}

#include <unordered_set>

std::wstring
SK_AppCache_Manager::getConfigPathForAppID (uint32_t uiAppID) const
{
  // If no AppCache (probably not a Steam game), or opting-out of central repo,
  //   then don't parse crap and just use the traditional path.
  if ( app_cache_db == nullptr || (! config.system.central_repository) )
    return SK_GetNaiveConfigPath ();

  std::wstring path = SK_GetNaiveConfigPath (       );
  std::wstring name ( getAppNameFromID      (uiAppID) );

  // Non-trivial name = custom path, remove the old-style <program.exe>
  if (name != L"")
  {
    std::wstring         original_dir (path);

    size_t       pos                     = 0;
    std::wstring host_app (SK_GetHostApp ());

    if ((pos = path.find (SK_GetHostApp (), pos)) != std::wstring::npos)
      path.replace (pos, host_app.length (), L"");

    name.erase ( std::remove_if ( name.begin (),
                                  name.end   (),

                                    [](wchar_t tval)
                                    {
                                      static
                                      const std::unordered_set <wchar_t>
                                        invalid_file_char =
                                        {
                                          L'\\', L'/', L':',
                                          L'*',  L'?', L'\"',
                                          L'<',  L'>', L'|'
                                        };

                                      return invalid_file_char.count (tval) > 0;
                                    }
                                ),

                     name.end ()
               );

    path += name;
    path += L"\\";

    SK_StripTrailingSlashesW (path.data ());

    MoveFileExW ( original_dir.c_str (),
                    path.c_str       (),
                      MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED );
  }

  return path;
}

bool
SK_AppCache_Manager::saveAppCache (bool close)
{
  if (app_cache_db != nullptr)
  {
    app_cache_db->write (app_cache_db->get_filename ());

    if (close)
    {
      delete app_cache_db;
      app_cache_db = nullptr;
    }

    return true;
  }

  return false;
}

int
SK_AppCache_Manager::migrateProfileData (LPVOID)
{
  // TODO
  return 0;
}


__declspec (dllexport)
SK_RenderAPI
__stdcall
SK_Render_GetAPIHookMask (void)
{
  int mask = 0;

#ifndef _WIN64
  if (config.apis.d3d8.hook)       mask |= static_cast <int> (SK_RenderAPI::D3D8);
  if (config.apis.ddraw.hook)      mask |= static_cast <int> (SK_RenderAPI::DDraw);
#endif
  if (config.apis.d3d9.hook)       mask |= static_cast <int> (SK_RenderAPI::D3D9);
  if (config.apis.d3d9ex.hook)     mask |= static_cast <int> (SK_RenderAPI::D3D9Ex);
  if (config.apis.dxgi.d3d11.hook) mask |= static_cast <int> (SK_RenderAPI::D3D11);
  if (config.apis.OpenGL.hook)     mask |= static_cast <int> (SK_RenderAPI::OpenGL);
#ifdef _WIN64
  if (config.apis.Vulkan.hook)     mask |= static_cast <int> (SK_RenderAPI::Vulkan);
  if (config.apis.dxgi.d3d12.hook) mask |= static_cast <int> (SK_RenderAPI::D3D12);
#endif

  return (SK_RenderAPI)mask;
}