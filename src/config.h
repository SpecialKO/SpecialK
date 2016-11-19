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

extern const wchar_t* SK_VER_STR;

const wchar_t*
__stdcall
SK_GetVersionStr (void);

struct sk_config_t
{
  struct {
    bool   show           = true;
    LONG   format         = LOCALE_CUSTOM_UI_DEFAULT;

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
    bool   use            = false;

    struct {
      BYTE toggle [4]     = { VK_CONTROL, VK_SHIFT, 'B', 0 };
    } keys;
  } load_balance;


  struct {
    bool   show           = true;

    bool   pump           = false;
    float  pump_interval  = 0.0166666666f;

    DWORD  red            = -1;
    DWORD  green          = -1;
    DWORD  blue           = -1;
    DWORD  scale          =  1;
    DWORD  pos_x          =  0;
    DWORD  pos_y          =  0;

    struct {
      BYTE toggle [4]     = { VK_CONTROL, VK_SHIFT, 'O',          0 };
      BYTE shrink [4]     = { VK_CONTROL, VK_SHIFT, VK_OEM_MINUS, 0 };
      BYTE expand [4]     = { VK_CONTROL, VK_SHIFT, VK_OEM_PLUS,  0 };
    } keys;
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
      BYTE toggle [4]      = { VK_CONTROL, VK_SHIFT, 'D', 0 };
    } keys;
  } disk;


  struct {
    bool   show            = false;
    float  interval        = 2.5f;

    struct {
      BYTE toggle [4]      = { VK_CONTROL, VK_SHIFT, 'P', 0 };
    } keys;
  } pagefile;


  struct {
    bool    enable              = true;
  } cegui;


  struct {
    struct {
      bool  pull_friend_stats   = true;
      bool  pull_global_stats   = true;
      bool  show_popup          = true;
      int   popup_duration      = 5000UL;
      int   popup_corner        = 1; // XXX: Use actual coordinates
    } achievements;

    std::wstring
            achievement_sound   = L"";
    bool    achievement_sshot   = false;
    int     notify_corner       = 4; // 0=Top-Left,
                                     // 1=Top-Right,
                                     // 2=Bottom-Left,
                                     // 3=Bottom-Right,
                                     // 4=Don't Care
    int     inset_x             = 0;
    int     inset_y             = 0;

    bool    nosound             = false;
    bool    silent              = false;

    int     init_delay          = 0UL; // Disable to prevent crashing in many games
    bool    auto_pump_callbacks = true;

    int     appid               = 0;
  } steam;


  struct {
    struct {
      float   target_fps        =  0.0f;
      int     pre_render_limit  = -1;
      int     present_interval  = -1;
      int     buffer_count      = -1;
      int     max_delta_time    =  0; // Bad old setting; needs to be phased out
      bool    flip_discard      = false;
      int     swapchain_wait    =  0;
      int     pin_render_thread = -1;
    } framerate;
    struct {
      bool    force_d3d9ex      = false;
      bool    force_fullscreen  = false;
      int     hook_type         = 0;
      int     refresh_rate      = -1;
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
      int     scaling_mode      =  -1; // -1 = Don't Care
    } dxgi;

    // OSD Render Stats (D3D11 Only Right Now)
    bool      show              = false;
    struct {
      BYTE toggle [4]      = { VK_CONTROL, VK_SHIFT, 'R', 0 };
    } keys;
  } render;

  struct {
    struct {
      bool    precise_hash      = false;
      bool    dump              = false;
      bool    inject            = true;
      bool    cache             = false;
      std::wstring
              res_root          = L"SK_Res";
    } d3d11;
    struct {
      int     min_evict         = 32;
      int     max_evict         = 1024;
      int     min_entries       = 256;
      int     max_entries       = 8192;
      int     min_size          = 384L;
      int     max_size          = 2048L;
      bool    ignore_nonmipped  = false;
    } cache;
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

    struct {
      bool    disable           = false;
    } api;
  } nvidia;

  struct {
    struct {
      int     timeout           = 1500UL;
      bool    manage            = false;
      bool    keys_activate     = true;
    } cursor;
  } input;

  struct {
    bool    borderless          = false;
    bool    center              = false;
    int     x_offset            = 0;
    int     y_offset            = 0;
    bool    background_render   = false;
    bool    confine_cursor      = false;
    bool    fullscreen          = false;
  } window;

  struct {
    bool    ignore_raptr        = false;
    bool    disable_raptr       = false;
    bool    rehook_loadlibrary  = false;

    struct {
      bool  rehook_reset        = false;
      bool  rehook_present      = false;
      bool  hook_reset_vftbl    = false;
      bool  hook_present_vftbl  = false;
    } d3d9;
  } compatibility;

  struct {
    int     init_delay          = 250;
    bool    silent              = false;
    bool    handle_crashes      = true;
    bool    prefer_fahrenheit   = true;
    bool    display_debug_out   = false;
    bool    game_output         = true;
    bool    central_repository  = false;
    bool    ignore_rtss_delay   = false;
    std::wstring
            version             = SK_VER_STR;
  } system;
};

extern sk_config_t config;

bool SK_LoadConfig (std::wstring name         = L"dxgi");
void SK_SaveConfig (std::wstring name         = L"dxgi",
                    bool         close_config = false);

//
// Sort of a nasty hack until I come up with a way for plug-ins to
//   formally read the parent DLL's configuration.
//
uint32_t
__cdecl
SK_Config_GetTargetFPS (void);

#endif __SK__CONFIG_H__