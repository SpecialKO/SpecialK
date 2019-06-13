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

struct IUnknown;
#include <Unknwnbase.h>

#include <Windows.h>
#include <string>
#include <set>
#include <unordered_set>
#include <concurrent_unordered_map.h>
#include <intsafe.h>

#include <SpecialK/render/backend.h>

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
  SK_ConfigSerializedKeybind (SK_Keybind&& bind, const wchar_t* cfg_name) :
    SK_Keybind (bind)
  {
    wcscpy (short_name, cfg_name);
  }

  wchar_t               short_name [32] = L"Uninitialized";
  sk::ParameterStringW* param           = nullptr;
};


struct sk_config_t
{
  struct {
    float  duration       = 20.0f;
  } version_banner;

  struct {
    LONG   format         = LOCALE_USER_DEFAULT;

    struct {
      BYTE toggle [4]     = { VK_CONTROL, VK_SHIFT, 'T', 0 };
    } keys;

    bool   show           = true;
  } time;

  struct {
    float  reserve        = 0.0f;// Unused / Unlimited
    float  interval       = 0.25f;

    struct {
      BYTE toggle [4]     = { VK_CONTROL, VK_SHIFT, 'M', 0 };
    } keys;

    bool   show           = false;
  } mem;


  struct {
    float  interval       = 0.25f; // 250 msecs (4 Hz)

    struct {
      BYTE toggle [4]     = { VK_CONTROL, VK_SHIFT, 'I', 0 };
    } keys;

    bool   show           = false;
  } io;


  struct {
    bool   show           = false;

    struct {
      BYTE toggle [4]     = { VK_CONTROL, VK_SHIFT, 'S', 0 };
    } keys;
  } sli;


  struct {
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
    bool   show           = false;
  } osd;


  struct {
    GUID      power_scheme_guid      = { };
    GUID      power_scheme_guid_orig = { };

    float  interval       = 0.33333333f;

    bool   simple         = true;
    bool   show           = false;

    struct {
      BYTE toggle  [4]    = { VK_CONTROL, VK_SHIFT, 'C', 0 };
    } keys;
  } cpu;


  struct {
    bool   show           = true;
    bool   advanced       = false;
    bool   frametime      = true;

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
    ULONG   frames_drawn       = 0;     //   Count the number of frames drawn using it
    bool    enable             = true;
    bool    orig_enable        = false; // Since CEGUI is a frequent source of crashes.
    bool    safe_init          = true;
  } cegui;

  struct {
    float   scale              = 1.0f;
    bool    show_eula          = false; // Will be flipped on if no AppCache is present
    bool    show_input_apis    = true;
    bool    use_mac_style_menu = false;

    struct {
      struct {
        std::string file   = "";
        float       size   = 7.0f;
      } chinese,  cyrillic, default_font,
        japanese, korean;
    } font;

    // Per-game (mostly compatibility) settings
    struct
    {
      bool  disable_alpha      = false;
      bool  antialias_lines    = true;
      bool  antialias_contours = true;
    } render;
  } imgui;


  struct {
    struct callback_cache_s {
      HMODULE
        module               = nullptr;
      uintptr_t
        offset               = 0;
      void* resolved         = nullptr;
    } cached_overlay_callback;

    struct {
      std::wstring
        sound_file          = L"";

      struct {
        float inset               = 0.005f;
        int   origin              = 0;
        int   duration            = 5000UL;
        bool  show                = true;
        bool  show_title          = true;
        bool  animate             = true;
      } popup;

      bool    take_screenshot     = false;
      bool    play_sound          = true;
      bool    pull_friend_stats   = true;
      bool    pull_global_stats   = true;
    } achievements;

    struct {
      std::set <std::string> blacklist;
    } cloud;

    std::wstring
            dll_path              = L"";

    int     appid                 = 0;
    int     notify_corner         = 4;   // 0=Top-Left,
                                         // 1=Top-Right,
                                         // 2=Bottom-Left,
                                         // 3=Bottom-Right,
                                         // 4=Don't Care
    int     online_status         = -1;  // Force a certain online status at all times
    int     init_delay            = 0UL; // Disable to prevent crashing in many games
    int     callback_throttle     = -1;

    float   overlay_hdr_luminance = 4.375f; // 350 nits
                                            //   that do not use it

    bool      silent               = false;
    bool      preload_client       = false;
    bool      preload_overlay      = false; // For input processing, this is important
    bool      show_playtime        = true;  // In the control panel title
    bool      force_load_steamapi  = false; // Load steam_api{64}.dll even in games
    bool      auto_pump_callbacks  = true;
    bool      block_stat_callback  = false;
    bool      filter_stat_callback = false;
    bool      spoof_BLoggedOn      = false;
    bool      overlay_hides_sk_osd = true;
    bool      reuse_overlay_pause  = true;  // Use Steam's overlay pause mode for our own
                                          //   control panel
    bool      auto_inject          = true;  // Control implicit steam_api.dll bootstrapping

    struct screenshot_handler_s {
      bool    enable_hook          = true;
      bool    png_compress         = true;
      bool    show_osd_by_default  = true;

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
          "Take Screenshot without Special K's OSD", L"F8",
           true, true, false, VK_F8
        }, L"WithoutOSD"
      };

      SK_ConfigSerializedKeybind
              sk_osd_insertion_keybind = {
        SK_Keybind {
          "Take Screenshot and insert Special K's OSD", L"Ctrl+Shift+F8",
           false, false, true, VK_F8
        }, L"InsertOSD"
      };
    } screenshots;
  } steam;

  struct {
    float overlay_luminance     = 4.375f; // 350 nits
  } uplay;


  struct {
    struct {
      float   target_fps        =  0.0f;
      float   target_fps_bg     =  0.0f;
      float   limiter_tolerance =  2.0f;
      int     max_render_ahead  =  0;
      int     override_num_cpus = -1;
      int     pre_render_limit  = -1;
      int     present_interval  = -1;
      int     buffer_count      = -1;
      int     max_delta_time    =  0; // Bad old setting; needs to be phased
      int     swapchain_wait    =  0;
      float   refresh_rate      = -1.0f;
 std::wstring rescan_ratio     =L"-1/1";
      struct {
        UINT Denom              =  1;
        UINT Numerator          =
                            UINT (-1);
      } rescan_;
      int     refresh_denom     =  1;
      int     pin_render_thread = -1;
      bool    flip_discard      = false;
      bool    disable_flip      = false;
      bool    wait_for_vblank   = false;
      bool    sleepless_render  = false;
      bool    sleepless_window  = false;
      bool    enable_mmcss      = true;
    } framerate;
    struct {
      bool    force_d3d9ex      = false;
      bool    force_impure      = false;
    } d3d9;
    struct {
      int     adapter_override  = -1;
      struct {
        struct {
          unsigned int x        =  0;
          unsigned int y        =  0;
          bool isZero (void) noexcept { return x == 0 && y == 0; };
        } min;
        struct {
          unsigned int x        =  0;
          unsigned int y        =  0;
          bool isZero (void) noexcept { return x == 0 && y == 0; };
        } max;
      } res;
      int     exception_mode     =    -1; // -1 = Don't Care
      int     scaling_mode       =    -1; // -1 = Don't Care
      int     scanline_order     =    -1; // -1 = Don't Care
      int     msaa_samples       =    -1;
      // DXGI 1.1 (Ignored for now)
      int     rotation           =    -1; // -1 = Don't Care
      bool    test_present       = false;
      bool    full_state_cache   = false;
      bool    debug_layer        = false;
      bool    allow_tearing      = false;
      bool    safe_fullscreen    = false;
      bool    enhanced_depth     = false;
      bool    deferred_isolation = false;
      bool    present_test_skip  = false;
    } dxgi;

    struct {
      ULONG _last_vidcap_frame   = 0;
      ULONG _last_normal_frame   = 0;
      float   hdr_luminance      = 4.375f; // 350 nits
      // Required by default for compatibility with Mirillis Action!
      bool    draw_in_vidcap     = true;
    } osd;

    // OSD Render Stats (D3D11 Only Right Now)
    bool      show               = false;
    struct {
      BYTE    toggle [4]         = { VK_CONTROL, VK_SHIFT, 'R', 0 };
    } keys;
  } render;

  struct {
    int       monitor_idx         =    -1; // TODO
    float     refresh_rate        =  0.0f; // TODO
    bool      force_fullscreen    = false;
    bool      force_windowed      = false;
  } display;

  struct {
    struct {
      std::wstring
        res_root                  = L"SK_Res";
      bool    precise_hash        = false;
      bool    dump                = false;
      bool    inject              = true;
      bool    cache               = true;
      bool    highlight_debug     = true;
      bool    injection_keeps_fmt = false;
      bool    generate_mips       = false;
      bool    cache_gen_mips      =  true;
      bool    uncompressed_mips   = false;
    } d3d11;
    struct {
      int     min_evict           = 64;
      int     max_evict           = 1024;
      int     min_entries         = 512;
      int     max_entries         = 65536; // Bump max up from 8192 since the hashmap
                                           //   is now segmented by number of mipmap LODs;
                                           //
                                           //  Overhead of managing a large hashmap is
                                           //    much lower.
      int     min_size            = 384L;
      int     max_size            = 2048L;
      bool    ignore_nonmipped    = false;
      bool    allow_staging       = false;
      bool    allow_unsafe_refs   = false; // Allow texture caching even in engines that
                                           //   are not correctly keeping track of resources
      bool    residency_managemnt = false;// true;
      bool    vibrate_on_miss     = false;
    } cache;

    bool highlight_debug_tex      = false;
    bool on_demand_dump           = false;
    bool d3d9_mod                 = false; // Causing compat issues in some D3D9Ex games ATM
    bool dump_on_load             = false;
  } textures;

  struct {
    bool trace_reads              = false;
    bool trace_writes             = false;

    struct ignore_files_s {
      Concurrency::concurrent_unordered_set <std::wstring> single_file;
      Concurrency::concurrent_unordered_set <std::wstring> entire_thread;
    } ignore_reads,
      ignore_writes;

  } file_io;

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
    struct {
    //bool    fix_10bit_gsync   = false;
      bool    kill_hdr          = false;
    } bugs;
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
      int     predefined_layout = 1;    //0 = PS4, 1 = Steam, 2 = Xbox
      bool    disabled_to_game  = false;
      bool    disable_ps4_hid   = false;
      bool    rehook_xinput     = false;
      bool    haptic_ui         = true;
      bool    disable_rumble    = false;
      bool    hook_dinput8      = true;
      bool    hook_dinput7      = true;
      bool    hook_hid          = true;
      bool    hook_xinput       = true; // Kind of important ;)
      bool    native_ps4        = false;

      struct {
        unsigned
        int   ui_slot           =    0;
        bool  placehold  [4]    = { false };
        int   assignment [4]    = { 0, 1, 2, 3 };
      } xinput;

      struct
      {
        unsigned
        int   ui_slot           =    0;
      } steam;
    } gamepad;

    struct {
      bool    block_windows_key = false;
      bool    catch_alt_f4      = true;
      bool    disabled_to_game  = false;
    } keyboard;

    struct {
      //
      // Uses APIs such as DirectInput or RawInput that only send relative motion events
      //   to derive the virtual position of the cursor, since the game hijacks the
      //     physical position.
      //
      //   >> Ideally we want absolute cursor position every frame for the UI, but
      //        that's not always possible. <<
      //
      float   antiwarp_deadzone   = 2.5f;

      // Translate WM_MOUSEWHEEL messages into actual events that will trigger
      //   other mouse APIs such as DirectInput and RawInput.
      //
      //  * Without this, mousewheel scrolling doesn't work in MANY games on a
      //      Synaptics touchpad.
      //
      bool    fix_synaptics       = false;
      // If absolute cursor position is stuck (i.e. Dreamfall Chapters) use this
      bool    add_relative_motion = true;
      bool    disabled_to_game    = false;
    } mouse;
  } input;

  struct {
    bool    enable_mem_alloc_trace = false;
    bool    enable_file_io_trace   = false;
  } threads;

  struct {
    struct {
      bool  use_static_addresses = false;
    } global;
  } injection;

  struct {
    bool    borderless          = false;
    bool    border_override     = false;
    bool    center              = false;
    struct {
      struct {
      int   absolute            = 0;
      float percent             = 0.0f;
      } x, y;
      bool isZero (void) noexcept
            { return x.absolute == 0        && y.absolute == 0        &&
                     x.percent  > -0.00001f && x.percent   < 0.00001f &&
                     y.percent  > -0.00001f && y.percent   < 0.00001f; }
    } offset;
    int     always_on_top       = 0;
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
    struct {
      struct {
        unsigned int x          = 0;
        unsigned int y          = 0;
        bool         fix_mouse  = false;
        bool isZero (void) noexcept { return x == 0 && y == 0; };
      } override;
    } res;
  } window;

  struct {
    bool    rehook_loadlibrary   = false;
    bool    disable_nv_bloat     = false;
    bool   init_while_suspended  = true;
  } compatibility;

  struct {
    struct {
      bool   hook              = true;
    } glide;

#ifdef _M_IX86
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
      } d3d12, d3d11;
    } dxgi;

    struct {
      bool   hook              = true;
    } Vulkan, OpenGL;

    struct {
      bool         enable       = true;
      bool         gsync_status = true;
      bool         disable_hdr  = false;
      std::wstring bpc_enum     = L"NV_BPC_DEFAULT";
      std::wstring col_fmt_enum = L"NV_COLOR_FORMAT_AUTO";
    } NvAPI;

    struct {
      bool   enable            = true;
    } ADL;

    SK_RenderAPI last_known    = SK_RenderAPI::Reserved;
  } apis;

  struct {
    std::wstring
            version             = SK_GetVersionStrW ();
    int     log_level           = 0;
    float   global_inject_delay = 0.0f;
#ifdef _DEBUG
    bool    trace_create_thread = true;
#else
    bool    trace_create_thread = false;
#endif
    bool    trace_load_library  = true;
    bool    strict_compliance   = false;
    bool    silent              = false;
    bool    handle_crashes      = true;
    bool    prefer_fahrenheit   = false;
    bool    display_debug_out   = false;
    bool    game_output         = true;
    bool    central_repository  = false;
    bool    ignore_rtss_delay   = false;
  } system;
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

extern sk_config_t _config;
#define config _config

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
    std::string  name;
    DepotId_t    id;
  } depot;

  struct
  {
    std::string  date;
    ManifestId_t id;
  } manifest;
};

typedef std::vector <SK_Steam_DepotManifest> SK_DepotList;

struct SK_AppCache_Manager
{ enum Ownership
  { Unknown    = -1,
    DoesNotOwn =  0,
    OwnsGame   =  1,
    _Alignment_=  DWORD_MAX };

  bool          saveAppCache       (bool           close = false);
  bool          loadAppCacheForExe (const wchar_t* wszExe);

  uint32_t      getAppIDFromPath   (const wchar_t* wszPath) const;
  std::wstring  getAppNameFromID   (uint32_t       uiAppID) const;
  std::wstring  getAppNameFromPath (const wchar_t* wszPath) const;

  bool          addAppToCache      ( const wchar_t* wszRelativePath,
                                     const wchar_t* wszExecutable,
                                     const wchar_t* wszAppName,
                                           uint32_t uiAppID );

  std::wstring  getConfigPathFromAppPath (const wchar_t* wszPath) const;
  std::wstring  getConfigPathForAppID    (uint32_t       uiAppID) const;

  int           migrateProfileData       (LPVOID reserved = nullptr);

  SK_DepotList& getAvailableManifests    (DepotId_t steam_depot);
  ManifestId_t  getInstalledManifest     (DepotId_t steam_depot);
  int           loadDepotCache           (DepotId_t steam_depot = 0);
  int           storeDepotCache          (DepotId_t steam_depot = 0);


  time_t        setFriendOwnership (uint64_t friend_, Ownership owns             );
  Ownership     getFriendOwnership (uint64_t friend_, time_t*   updated = nullptr);

  time_t        setFriendAchievPct (uint64_t friend_, float   percent          );
  float         getFriendAchievPct (uint64_t friend_, time_t* updated = nullptr);

  bool          wantFriendStats    (void); // Per-application override

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
  DeadlyPremonition,            // DP.exe DPLauncher.exe
  GalGun_Double_Peace,          // GG2Game.exe
  AKIBAs_Trip,                  // AkibaUU.exe
  YS_Seven,                     // Ys7.exe
  LifeIsStrange_BeforeTheStorm, // Life is Strange - Before the Storm.exe
  Tales_of_Symphonia,           // TOS.exe
  Tales_of_Zestiria,            // Tales of Zestiria.exe
  Tales_of_Vesperia,            // TOV_DE.exe
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
  DragonBallFighterZ,           // DBFighterZ.exe
  NiNoKuni2,                    // Nino2.exe
  FarCry5,                      // FarCry5.exe
  ChronoTrigger,                // Chrono Trigger.exe
  Ys_Eight,                     // ys8.exe
  PillarsOfEternity2,           // PillarsOfEternityII.exe
  Yakuza0,                      // Yakuza0.exe
  YakuzaKiwami2,                // YakuzaKiwami2.exe
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
SK_GetNaiveConfigPath (void);

extern const wchar_t*
SK_GetFullyQualifiedApp (void);

const wchar_t*
__stdcall
SK_GetVersionStr (void);


extern __declspec (dllexport) void
__stdcall
SK_ImGui_KeybindDialog (SK_Keybind* keybind);

extern SK_LazyGlobal <std::unordered_map <std::wstring, BYTE>> humanKeyNameToVirtKeyCode;
extern SK_LazyGlobal <std::unordered_map <BYTE, std::wstring>> virtKeyCodeToHumanKeyName;

#endif /* __SK__CONFIG_H__ */