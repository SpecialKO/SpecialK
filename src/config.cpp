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
#include <SpecialK/render/d3d11/d3d11_core.h>

#include <filesystem>

#define D3D11_RAISE_FLAG_DRIVER_INTERNAL_ERROR 1

iSK_INI*       dll_ini    = nullptr;
iSK_INI*       osd_ini    = nullptr;
iSK_INI*       steam_ini  = nullptr;
iSK_INI*       macro_ini  = nullptr;

SK_LazyGlobal <SK_AppCache_Manager> app_cache_mgr;

SK_LazyGlobal <std::unordered_map <wstring_hash, BYTE>> humanKeyNameToVirtKeyCode;
SK_LazyGlobal <std::unordered_map <BYTE, wchar_t [64]>> virtKeyCodeToHumanKeyName;

extern SK_LazyGlobal <Concurrency::concurrent_unordered_map <DepotId_t, SK_DepotList> >           SK_Steam_DepotManifestRegistry;
extern SK_LazyGlobal <Concurrency::concurrent_unordered_map <DepotId_t, SK_Steam_DepotManifest> > SK_Steam_InstalledManifest;

extern float __target_fps;
extern float __target_fps_bg;


SK_GAME_ID
__stdcall
SK_GetCurrentGameID (void)
{
  static SK_GAME_ID current_game =
         SK_GAME_ID::UNKNOWN_GAME;

  static bool first_check = true;
  if         (first_check)
  {
    auto constexpr hash_lower = [&](const wchar_t* const wstr) -> size_t
          { return hash_string                          (wstr, true); };

    static std::unordered_map <size_t, SK_GAME_ID> _games = {
      { hash_lower (L"Tyranny.exe"),                            SK_GAME_ID::Tyranny                      },
      { hash_lower (L"TidesOfNumenera.exe"),                    SK_GAME_ID::TidesOfNumenera              },
      { hash_lower (L"MassEffectAndromeda.exe"),                SK_GAME_ID::MassEffect_Andromeda         },
      { hash_lower (L"MadMax.exe"),                             SK_GAME_ID::MadMax                       },
      { hash_lower (L"Dreamfall Chapters.exe"),                 SK_GAME_ID::Dreamfall_Chapters           },
      { hash_lower (L"TheWitness.exe"),                         SK_GAME_ID::TheWitness                   },
      { hash_lower (L"Obduction-Win64-Shipping.exe"),           SK_GAME_ID::Obduction                    },
      { hash_lower (L"witcher3.exe"),                           SK_GAME_ID::TheWitcher3                  },
      { hash_lower (L"re7.exe"),                                SK_GAME_ID::ResidentEvil7                },
      { hash_lower (L"DDDA.exe"),                               SK_GAME_ID::DragonsDogma                 },
      { hash_lower (L"eqgame.exe"),                             SK_GAME_ID::EverQuest                    },
      { hash_lower (L"GE2RB.exe"),                              SK_GAME_ID::GodEater2RageBurst           },
      { hash_lower (L"ge3.exe"),                                SK_GAME_ID::GodEater3                    },
      { hash_lower (L"WatchDogs2.exe"),                         SK_GAME_ID::WatchDogs2                   },
      { hash_lower (L"NieRAutomata.exe"),                       SK_GAME_ID::NieRAutomata                 },
      { hash_lower (L"Warframe.x64.exe"),                       SK_GAME_ID::Warframe_x64                 },
      { hash_lower (L"LEGOLCUR_DX11.exe"),                      SK_GAME_ID::LEGOCityUndercover           },
      { hash_lower (L"Sacred.exe"),                             SK_GAME_ID::Sacred                       },
      { hash_lower (L"sacred2.exe"),                            SK_GAME_ID::Sacred2                      },
      { hash_lower (L"FF9.exe"),                                SK_GAME_ID::FinalFantasy9                },
      { hash_lower (L"FinchGame.exe"),                          SK_GAME_ID::EdithFinch                   },
      { hash_lower (L"FFX.exe"),                                SK_GAME_ID::FinalFantasyX_X2             },
      { hash_lower (L"FFX-2.exe"),                              SK_GAME_ID::FinalFantasyX_X2             },
      { hash_lower (L"FFX&X-2_Will.exe"),                       SK_GAME_ID::FinalFantasyX_X2             },
      { hash_lower (L"DP.exe"),                                 SK_GAME_ID::DeadlyPremonition            },
      { hash_lower (L"GG2Game.exe"),                            SK_GAME_ID::GalGun_Double_Peace          },
      { hash_lower (L"Ys7.exe"),                                SK_GAME_ID::YS_Seven                     },
      { hash_lower (L"TOS.exe"),                                SK_GAME_ID::Tales_of_Symphonia           },
      { hash_lower (L"Tales of Zestiria.exe"),                  SK_GAME_ID::Tales_of_Zestiria            },
      { hash_lower (L"TOV_DE.exe"),                             SK_GAME_ID::Tales_of_Vesperia            },
      { hash_lower (L"Life is Strange - Before the Storm.exe"), SK_GAME_ID::LifeIsStrange_BeforeTheStorm },
      { hash_lower (L"EoCApp.exe"),                             SK_GAME_ID::DivinityOriginalSin          },
      { hash_lower (L"Hob.exe"),                                SK_GAME_ID::Hob                          },
      { hash_lower (L"DukeForever.exe"),                        SK_GAME_ID::DukeNukemForever             },
      { hash_lower (L"BLUE_REFLECTION.exe"),                    SK_GAME_ID::BlueReflection               },
      { hash_lower (L"Zero Escape.exe"),                        SK_GAME_ID::ZeroEscape                   },
      { hash_lower (L"hackGU.exe"),                             SK_GAME_ID::DotHackGU                    },
      { hash_lower (L"WOFF.exe"),                               SK_GAME_ID::WorldOfFinalFantasy          },
      { hash_lower (L"StarOceanTheLastHope.exe"),               SK_GAME_ID::StarOcean4                   },
      { hash_lower (L"LEGOMARVEL2_DX11.exe"),                   SK_GAME_ID::LEGOMarvelSuperheroes2       },
      { hash_lower (L"okami.exe"),                              SK_GAME_ID::Okami                        },
      { hash_lower (L"DuckTales.exe"),                          SK_GAME_ID::DuckTalesRemastered          },
      { hash_lower (L"mafia3.exe"),                             SK_GAME_ID::Mafia3                       },
      { hash_lower (L"Owlboy.exe"),                             SK_GAME_ID::Owlboy                       },
      { hash_lower (L"DarkSoulsIII.exe"),                       SK_GAME_ID::DarkSouls3                   },
      { hash_lower (L"Fallout4.exe"),                           SK_GAME_ID::Fallout4                     },
      { hash_lower (L"dis1_st.exe"),                            SK_GAME_ID::DisgaeaPC                    },
      { hash_lower (L"Secret_of_Mana.exe"),                     SK_GAME_ID::SecretOfMana                 },
      { hash_lower (L"DBFighterZ.exe"),                         SK_GAME_ID::DragonBallFighterZ           },
      { hash_lower (L"Nino2.exe"),                              SK_GAME_ID::NiNoKuni2                    },
      { hash_lower (L"FarCry5.exe"),                            SK_GAME_ID::FarCry5                      },
      { hash_lower (L"Chrono Trigger.exe"),                     SK_GAME_ID::ChronoTrigger                },
      { hash_lower (L"ys8.exe"),                                SK_GAME_ID::Ys_Eight                     },
      { hash_lower (L"PillarsOfEternityII.exe"),                SK_GAME_ID::PillarsOfEternity2           },
      { hash_lower (L"Yakuza0.exe"),                            SK_GAME_ID::Yakuza0                      },
      { hash_lower (L"YakuzaKiwami.exe"),                       SK_GAME_ID::YakuzaKiwami                 },
      { hash_lower (L"YakuzaKiwami2.exe"),                      SK_GAME_ID::YakuzaKiwami2                },
      { hash_lower (L"MonsterHunterWorld.exe"),                 SK_GAME_ID::MonsterHunterWorld           },
      { hash_lower (L"Shenmue.exe"),                            SK_GAME_ID::Shenmue                      },
      { hash_lower (L"Shenmue2.exe"),                           SK_GAME_ID::Shenmue                      },
      { hash_lower (L"SteamLauncher.exe"),                      SK_GAME_ID::Shenmue                      }, // Bad idea
      { hash_lower (L"DRAGON QUEST XI.exe"),                    SK_GAME_ID::DragonQuestXI                },
      { hash_lower (L"ACOdyssey.exe"),                          SK_GAME_ID::AssassinsCreed_Odyssey       },
      { hash_lower (L"ACOrigins.exe"),                          SK_GAME_ID::AssassinsCreed_Odyssey       },
      { hash_lower (L"JustCause3.exe"),                         SK_GAME_ID::JustCause3                   },
      { hash_lower (L"ed8.exe"),                                SK_GAME_ID::TrailsOfColdSteel            },
      { hash_lower (L"sekiro.exe"),                             SK_GAME_ID::Sekiro                       },
      { hash_lower (L"Octopath_Traveler-Win64-Shipping.exe"),   SK_GAME_ID::OctopathTraveler             },
      { hash_lower (L"SonicMania.exe"),                         SK_GAME_ID::SonicMania                   },
      { hash_lower (L"P4G.exe"),                                SK_GAME_ID::Persona4                     },
      { hash_lower (L"HorizonZeroDawn.exe"),                    SK_GAME_ID::HorizonZeroDawn              },
      { hash_lower (L"bg3.exe"),                                SK_GAME_ID::BaldursGate3                 },
      { hash_lower (L"Cyberpunk2077.exe"),                      SK_GAME_ID::Cyberpunk2077                },
      { hash_lower (L"Atelier_Ryza_2.exe"),                     SK_GAME_ID::AtelierRyza2                 },
      { hash_lower (L"nioh2.exe"),                              SK_GAME_ID::Nioh2                        },
      { hash_lower (L"HuniePop 2 - Double Date.exe"),           SK_GAME_ID::HuniePop2                    },
      { hash_lower (L"NieR Replicant ver.1.22474487139.exe"),   SK_GAME_ID::NieR_Sqrt_1_5                }
    };

    first_check = false;

#ifdef _M_AMD64
    // For games that can't be matched using a single executable filename
    if (! _games.count (hash_lower (SK_GetHostApp ())))
    {
      auto app_id =
        SK_Steam_GetAppID_NoAPI ();

      if (app_id == 1382330)
        current_game = SK_GAME_ID::Persona5Strikers;

      if ( StrStrIW ( SK_GetHostApp (), L"ffxv" ) )
      {
        current_game = SK_GAME_ID::FinalFantasyXV;

        extern void SK_FFXV_InitPlugin (void);
                    SK_FFXV_InitPlugin ();
      }

      else if ( StrStrIW ( SK_GetHostApp (), L"ACValhalla" ) )
      {
        current_game = SK_GAME_ID::AssassinsCreed_Valhalla;

        extern void SK_ACV_InitPlugin (void);
                    SK_ACV_InitPlugin ();
      }

      // Basically, _every single Yakuza game ever_ releases more references than it acquires...
      //   so just assume this is never going to get better and anything with Yakuza in the name
      //     needs help counting.
      else if ( StrStrIW ( SK_GetHostApp (), L"Yakuza" ) )
      {
        current_game = SK_GAME_ID::YakuzaUnderflow;
      }

      else if ( StrStrIW ( SK_GetHostApp  (), L"game"           ) )
      {    if ( StrStrIW ( SK_GetHostPath (), L"GalGun Returns" ) )
        {
          current_game = SK_GAME_ID::GalGunReturns;
        }

        else if ( StrStrIW ( SK_GetFullyQualifiedApp (),
                             L"P5S" ) )
        {
          current_game = SK_GAME_ID::Persona5Strikers;
          config.apis.d3d9.hook                    = false;
          config.apis.d3d9ex.hook                  = false;
          config.input.keyboard.override_alt_f4    = true;
          config.input.keyboard.catch_alt_f4       = true;
          config.window.background_render          = true;
        }
      }
    }

    else
#endif
    {
      current_game =
        _games [hash_lower (SK_GetHostApp ())];
    }
  }

  return
    current_game;
}


auto LoadKeybind =
  [](SK_ConfigSerializedKeybind* binding) ->
    auto
    {
      if (binding != nullptr)
      {
        auto ret =
          binding->param;

        if (ret != nullptr)
        {
          if (! static_cast <sk::iParameter *> (ret)->load  ())
          {
            // If the key exists without a value, skip assigning the default
            if (! static_cast <sk::iParameter *> (ret)->empty ())
            {
              binding->parse ();
              ret->store     (binding->human_readable);
            }
          }

          binding->human_readable = ret->get_value ();
          binding->parse ();
        }

        return ret;
      }

      return
        (sk::ParameterStringW *)nullptr;
    };


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
    sk::ParameterBool*    frametime;
    sk::ParameterBool*    advanced;
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
  struct {
    sk::ParameterBool*    trace_reads;
    sk::ParameterStringW  ignore_reads;
    sk::ParameterBool*    trace_writes;
    sk::ParameterStringW  ignore_writes;
  } file;
} reverse_engineering;

struct {
  struct {
    sk::ParameterFloat*   duration;
  } version_banner;


  sk::ParameterBool*      show;

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
  sk::ParameterBool*      disable_alpha;
  sk::ParameterBool*      antialias_lines;
  sk::ParameterBool*      antialias_contours;
} imgui;

struct {
  struct {
    sk::ParameterStringW*   sound_file;
    sk::ParameterBool*      play_sound;
    sk::ParameterBool*      take_screenshot;
    sk::ParameterBool*      fetch_friend_stats;

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
    sk::ParameterBool*    auto_inject;
    sk::ParameterBool*    reuse_overlay_pause;
    sk::ParameterStringW* dll_path;
  } system;

  struct {
    sk::ParameterInt*     online_status;
  } social;

  struct {
    sk::ParameterBool*    silent;
  } log;

  struct {
    sk::ParameterBool*    spoof_BLoggedOn;
  } drm;

  struct {
    sk::ParameterStringW* blacklist;
  } cloud;

  struct
  {
    sk::ParameterInt*     throttle;
  } callbacks;

  struct
  {
    // Will evolve over time, only supports D3D11/12 right now.
    sk::ParameterBool*    smart_capture;
  } screenshots;

  struct
  {
    sk::ParameterFloat*   hdr_luminance;
  } overlay;
} steam;

struct {
  sk::ParameterBool*      include_osd_default;
  sk::ParameterBool*      keep_png_copy;
  sk::ParameterBool*      play_sound;
  sk::ParameterBool*      copy_to_clipboard;
} screenshots;

struct {
  struct
  {
    sk::ParameterFloat*   hdr_luminance;
  } overlay;
} uplay;

struct {
  struct
  {
    sk::ParameterFloat*   hdr_luminance;
  } overlay;
} discord;

struct {
  struct
  {
    sk::ParameterFloat*   hdr_luminance;
  } overlay;
} rtss;

struct {
  sk::ParameterBool*      per_monitor_aware;
  sk::ParameterBool*      per_monitor_all_threads;
  sk::ParameterBool*      disable;
} dpi;

struct {
  struct {
    sk::ParameterBool*    override;
    sk::ParameterStringW* compatibility;
    sk::ParameterStringW* num_gpus;
    sk::ParameterStringW* mode;
  } sli;

  struct {
    sk::ParameterBool*    disable;
    sk::ParameterBool*    disable_hdr;
  } api;

  struct
  {
  //sk::ParameterBool*    fix_10bit_gsync;
    sk::ParameterBool*    snuffed_ansel;
    sk::ParameterBool*    bypass_ansel;
  } bugs;

  struct
  {
    sk::ParameterBool* enable;
    sk::ParameterBool* low_latency;
    sk::ParameterBool* low_latency_boost;
    sk::ParameterBool* marker_optimization;
    sk::ParameterInt*  engagement_policy;
  } reflex;
} nvidia;

struct {
  struct {
    sk::ParameterBool*    disable;
  } adl;
} amd;

sk::ParameterBool*        enable_cegui;
sk::ParameterBool*        safe_cegui;
sk::ParameterFloat*       mem_reserve;
sk::ParameterBool*        debug_output;
sk::ParameterBool*        debug_wait;
sk::ParameterBool*        game_output;
sk::ParameterBool*        handle_crashes;
sk::ParameterBool*        prefer_fahrenheit;
sk::ParameterBool*        ignore_rtss_delay;
sk::ParameterInt*         log_level;
sk::ParameterBool*        trace_libraries;
sk::ParameterBool*        strict_compliance;
sk::ParameterBool*        silent;
sk::ParameterFloat*       init_delay;
sk::ParameterStringW*     version; // Version at last boot

struct {
  struct {
    sk::ParameterFloat*   target_fps;
    sk::ParameterFloat*   target_fps_bg;
    sk::ParameterFloat*   limiter_tolerance;
    sk::ParameterInt*     prerender_limit;
    sk::ParameterInt*     present_interval;
    sk::ParameterInt*     buffer_count;
    sk::ParameterInt*     max_delta_time;
    sk::ParameterBool*    flip_discard;
    sk::ParameterBool*    disable_flip_model;
    sk::ParameterFloat*   refresh_rate;
    sk::ParameterStringW* rescan_ratio;
    sk::ParameterBool*    wait_for_vblank;
    sk::ParameterBool*    allow_dwm_tearing;
    sk::ParameterBool*    sleepless_window;
    sk::ParameterBool*    sleepless_render;
    sk::ParameterBool*    enable_mmcss;
    sk::ParameterInt*     override_cpu_count;
    sk::ParameterInt*     enforcement_policy;
    sk::ParameterBool*    drop_late_frames;
    sk::ParameterBool*    auto_low_latency;

    struct
    {
      sk::ParameterInt*   render_ahead;
    } control;
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
    sk::ParameterBool*    deferred_isolation;
    sk::ParameterInt*     msaa_samples;
    sk::ParameterBool*    skip_present_test;
    sk::ParameterInt*     srgb_behavior;
    sk::ParameterBool*    low_spec_mode;
    sk::ParameterBool*    hide_hdr_support;
  } dxgi;
  struct {
    sk::ParameterBool*    force_d3d9ex;
    sk::ParameterBool*    impure;
    sk::ParameterBool*    enable_texture_mods;
  } d3d9;
  struct {
    sk::ParameterBool*    draw_in_vidcap;
    sk::ParameterFloat*   hdr_luminance;
  } osd;
} render;

struct {
  sk::ParameterBool*      force_fullscreen;
  sk::ParameterBool*      force_windowed;
} display;

struct {
  struct {
    sk::ParameterBool*    clamp_lod_bias;
  } d3d9;
  struct {
    sk::ParameterBool*    precise_hash;
    sk::ParameterBool*    inject;
    sk::ParameterBool*    injection_keeps_format;
    sk::ParameterBool*    gen_mips;
    sk::ParameterBool*    cache;
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
    sk::ParameterBool*    allow_unsafe_refs;
    sk::ParameterBool*    manage_residency;
  } cache;
    sk::ParameterStringW* res_root;
    sk::ParameterBool*    dump_on_load;
} texture;

struct {
  struct
  {
    sk::ParameterBool*    catch_alt_f4;
    sk::ParameterBool*    bypass_alt_f4;
    sk::ParameterBool*    disabled_to_game;
  } keyboard;

  struct
  {
    sk::ParameterBool*    disabled_to_game;
  } mouse;

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
    sk::ParameterBool*    disable_rumble;
    sk::ParameterBool*    hook_dinput8;
    sk::ParameterBool*    hook_dinput7;
    sk::ParameterBool*    hook_hid;
    sk::ParameterBool*    hook_xinput;

    struct {
      sk::ParameterInt*     ui_slot;
      sk::ParameterInt*     placeholders;
      sk::ParameterStringW* assignment;
    } xinput;

    struct {
      sk::ParameterInt*     ui_slot;
    } steam;

    sk::ParameterBool*   native_ps4;
    sk::ParameterBool*   disabled_to_game;
  } gamepad;
} input;

struct {
  sk::ParameterBool*      enable_mem_alloc_trace;
  sk::ParameterBool*      enable_file_io_trace;
} threads;

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
  sk::ParameterInt*       always_on_top;
  sk::ParameterBool*      disable_screensaver;
  sk::ParameterBool*      dont_hook_wndproc;
} window;

struct {
  sk::ParameterStringW*   power_scheme_guid;
} cpu;

struct {
  sk::ParameterBool*      rehook_loadlibrary;
  sk::ParameterBool*      disable_nv_bloat;
} compatibility;

struct {
  struct {
    sk::ParameterBool*    hook;
  }
#ifdef _M_IX86
      ddraw, d3d8,
#endif
      d3d9,  d3d9ex,
      d3d11,
#ifdef _M_AMD64
      d3d12,
      Vulkan,
#endif
      OpenGL;

  sk::ParameterInt*       last_known;
} apis;

bool
SK_LoadConfig (const std::wstring& name) {
  return
    SK_LoadConfigEx (name);
}


__declspec (noinline)
const wchar_t*
__stdcall
SK_GetConfigPathEx (bool reset)
{
  if ((! SK_IsInjected ()) && (! config.system.central_repository))
    return SK_GetNaiveConfigPath ();

  static volatile LONG init = FALSE;

  if (! InterlockedCompareExchange (&init, 1, 0))
  {
    InterlockedIncrement (&init);
    app_cache_mgr->loadAppCacheForExe ( SK_GetFullyQualifiedApp () );
  }

  else
    SK_Thread_SpinUntilAtomicMin (&init, 2);

  static wchar_t
    cached_path [MAX_PATH + 2] = { };

  if (reset || InterlockedCompareExchange (&init, 3, 2) == 2)
  {
    wcscpy_s ( cached_path, MAX_PATH,
               app_cache_mgr->getConfigPathFromAppPath (
                 SK_GetFullyQualifiedApp ()
               ).c_str ()
           );

    InterlockedIncrement (&init);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&init, 3);

  return
    cached_path;
}

__declspec (noinline)
const wchar_t*
__stdcall
SK_GetConfigPath (void)
{
  return
    SK_GetConfigPathEx ();
}



template <typename _Tp>
_Tp*
SK_CreateINIParameter ( const wchar_t *wszDescription,
                        iSK_INI       *pINIFile,
                        const wchar_t *wszSection,
                        const wchar_t *wszKey )
{
  static_assert (std::is_polymorphic <_Tp> ());

  auto ret =
    dynamic_cast <_Tp *> (
      g_ParameterFactory->create_parameter <_Tp::value_type> (
        wszDescription )
    );

  if (ret != nullptr)
      ret->register_to_ini (pINIFile, wszSection, wszKey);

  return ret;
};


bool
SK_LoadConfigEx (std::wstring name, bool create)
{
  if (name.empty ())
  {
    if (SK_IsInjected ())
      name = L"SpecialK";
    else
      name = SK_GetBackend ();
  }

  static std::wstring orig_name = name;

  if (name.empty ()) name = orig_name;

  // Load INI File
  std::wstring full_name;
  std::wstring custom_name; // User may have custom prefs
  std::wstring master_name;

  std::wstring osd_config, steam_config, macro_config;

  full_name = // For paths with :, do not prepend the config root
    name.find (L':') != std::wstring::npos ?
      SK_FormatStringW ( L"%s.ini",             name.c_str () ) :
      SK_FormatStringW ( L"%s%s.ini",
                           SK_GetConfigPath (), name.c_str () );


  std::wstring undecorated_name (name);

  if ( undecorated_name.find (L"default_") != std::wstring::npos &&
       undecorated_name.find (L':')        == std::wstring::npos )
  {
      undecorated_name.erase ( undecorated_name.find (L"default_"),
                                        std::wstring (L"default_").length () );
  }

  custom_name =
    SK_FormatStringW ( L"%scustom_%s.ini",
                       SK_GetConfigPath (), undecorated_name.c_str () );

  if (create)
    SK_CreateDirectories ( full_name.c_str () );

  auto& rb =
    SK_GetCurrentRenderBackend ();


  static LONG         init     = FALSE;
  static bool         empty    = true;
  static std::wstring last_try = name;

  // Allow a second load attempt using a different name
  if (last_try != name)
  {
    init     = FALSE;
    last_try = std::move (name);
  }


  master_name        =
    SK_GetDocumentsDir () + LR"(\My Mods\SpecialK\Global\master_)" + undecorated_name + L".ini";

  osd_config         =
    SK_GetDocumentsDir () + LR"(\My Mods\SpecialK\Global\osd.ini)";

  steam_config =
    SK_GetDocumentsDir () + LR"(\My Mods\SpecialK\Global\steam.ini)";

  std::wstring migrate_steam_config;

  /////////if (! PathFileExistsW (steam_config.c_str ()))
  /////////{
  /////////  std::wstring achievement_config =
  /////////    SK_GetDocumentsDir () + LR"(\My Mods\SpecialK\Global\achievements.ini)";
  /////////
  /////////  if (PathFileExistsW (achievement_config.c_str ()))
  /////////  {
  /////////    migrate_steam_config =
  /////////      steam_config;
  /////////    steam_config =
  /////////      achievement_config;
  /////////  }
  /////////}

  macro_config       =
    SK_GetDocumentsDir () + LR"(\My Mods\SpecialK\Global\macros.ini)";

  if (init == FALSE || dll_ini == nullptr)
  {
    init = -1;


    dll_ini =
      SK_CreateINI (full_name.c_str ());

    if (! dll_ini)
    {
      assert (false && L"Out Of Memory");
      init = FALSE; return false;
    }


    empty   =
      dll_ini->get_sections ().empty ();


    SK_CreateDirectories (osd_config.c_str ());


    osd_ini         =
      SK_CreateINI (osd_config.c_str ());

    steam_ini =
      SK_CreateINI (steam_config.c_str ());

    macro_ini       =
      SK_CreateINI (macro_config.c_str ());

auto DeclKeybind =
  [](SK_ConfigSerializedKeybind* binding, iSK_INI* ini, const wchar_t* sec) ->
    auto
    {
      auto* ret =
       dynamic_cast <sk::ParameterStringW *>
        (g_ParameterFactory->create_parameter <std::wstring> (L"DESCRIPTION HERE"));

      if (ret != nullptr && binding != nullptr)
      {
        ret->register_to_ini ( ini, sec, binding->short_name );
      }

      return ret;
    };

// TODO:  Covert this to a template
#define ConfigEntry(param,descrip,ini,sec,key) {   \
  reinterpret_cast <sk::iParameter **> (&(param)), \
  std::type_index (              typeid ((param))),\
                    (descrip),  (ini),             \
                    (sec),      (key)              \
}

#define Keybind(bind,descrip,ini,sec) {             \
  reinterpret_cast <sk::iParameter **>  ((bind)),   \
  std::type_index (              typeid ((bind))),  \
                    (descrip),  (ini),              \
                    (sec),      ((bind)->short_name)\
}

  //
  // Create Parameters
  //
  struct param_decl_s {
           sk::iParameter **parameter_;
          std::type_index   type_;
    const wchar_t          *description_;
          iSK_INI          *ini_;
    const wchar_t          *section_;
    const wchar_t          *key_;
  } params_to_build [] =

  //// nb: If you want any hope of reading this table, turn line wrapping off.
    //
  {
    ConfigEntry (osd.version_banner.duration,            L"How long to display version info at startup, 0=disable)",   osd_ini,         L"SpecialK.VersionBanner",L"Duration"),
    ConfigEntry (osd.show,                               L"OSD Visibility",                                            osd_ini,         L"SpecialK.OSD",          L"Show"),

    ConfigEntry (osd.text.red,                           L"OSD Color (Red)",                                           osd_ini,         L"SpecialK.OSD",          L"TextColorRed"),
    ConfigEntry (osd.text.green,                         L"OSD Color (Green)",                                         osd_ini,         L"SpecialK.OSD",          L"TextColorGreen"),
    ConfigEntry (osd.text.blue,                          L"OSD Color (Blue)",                                          osd_ini,         L"SpecialK.OSD",          L"TextColorBlue"),

    ConfigEntry (osd.viewport.pos_x,                     L"OSD Position (X)",                                          osd_ini,         L"SpecialK.OSD",          L"PositionX"),
    ConfigEntry (osd.viewport.pos_y,                     L"OSD Position (Y)",                                          osd_ini,         L"SpecialK.OSD",          L"PositionY"),

    ConfigEntry (osd.viewport.scale,                     L"OSD Scale",                                                 osd_ini,         L"SpecialK.OSD",          L"Scale"),

    ConfigEntry (osd.state.remember,                     L"Remember status monitoring state",                          osd_ini,         L"SpecialK.OSD",          L"RememberMonitoringState"),

    ConfigEntry (monitoring.SLI.show,                    L"Show SLI Monitoring",                                       osd_ini,         L"Monitor.SLI",           L"Show"),

    ConfigEntry (uplay.overlay.hdr_luminance,            L"Make the uPlay Overlay visible in HDR mode!",               osd_ini,         L"uPlay.Overlay",         L"Luminance_scRGB"),
    ConfigEntry (rtss.overlay.hdr_luminance,             L"Make the RTSS Overlay visible in HDR mode!",                osd_ini,         L"RTSS.Overlay",          L"Luminance_scRGB"),
    ConfigEntry (discord.overlay.hdr_luminance,          L"Make the Discord Overlay visible in HDR mode!",             osd_ini,         L"Discord.Overlay",       L"Luminance_scRGB"),

    // Performance Monitoring  (Global Settings)
    //////////////////////////////////////////////////////////////////////////

    ConfigEntry (monitoring.io.show,                     L"Show IO Monitoring",                                        osd_ini,         L"Monitor.IO",            L"Show"),
    ConfigEntry (monitoring.io.interval,                 L"IO Monitoring Interval",                                    osd_ini,         L"Monitor.IO",            L"Interval"),

    ConfigEntry (monitoring.disk.show,                   L"Show Disk Monitoring",                                      osd_ini,         L"Monitor.Disk",          L"Show"),
    ConfigEntry (monitoring.disk.interval,               L"Disk Monitoring Interval",                                  osd_ini,         L"Monitor.Disk",          L"Interval"),
    ConfigEntry (monitoring.disk.type,                   L"Disk Monitoring Type (0 = Physical, 1 = Logical)",          osd_ini,         L"Monitor.Disk",          L"Type"),

    ConfigEntry (monitoring.cpu.show,                    L"Show CPU Monitoring",                                       osd_ini,         L"Monitor.CPU",           L"Show"),
    ConfigEntry (monitoring.cpu.interval,                L"CPU Monitoring Interval (seconds)",                         osd_ini,         L"Monitor.CPU",           L"Interval"),
    ConfigEntry (monitoring.cpu.simple,                  L"Minimal CPU Info",                                          osd_ini,         L"Monitor.CPU",           L"Simple"),

    ConfigEntry (monitoring.gpu.show,                    L"Show GPU Monitoring",                                       osd_ini,         L"Monitor.GPU",           L"Show"),
    ConfigEntry (monitoring.gpu.interval,                L"GPU Monitoring Interval (msecs)",                           osd_ini,         L"Monitor.GPU",           L"Interval"),
    ConfigEntry (monitoring.gpu.print_slowdown,          L"Print GPU Slowdown Reason (NVIDA GPUs)",                    osd_ini,         L"Monitor.GPU",           L"PrintSlowdown"),

    ConfigEntry (monitoring.pagefile.show,               L"Show Pagefile Monitoring",                                  osd_ini,         L"Monitor.Pagefile",      L"Show"),
    ConfigEntry (monitoring.pagefile.interval,           L"Pagefile Monitoring INterval (seconds)",                    osd_ini,         L"Monitor.Pagefile",      L"Interval"),

    ConfigEntry (monitoring.memory.show,                 L"Show Memory Monitoring",                                    osd_ini,         L"Monitor.Memory",        L"Show"),
    ConfigEntry (monitoring.fps.show,                    L"Show Framerate Monitoring",                                 osd_ini,         L"Monitor.FPS",           L"Show"),
    ConfigEntry (monitoring.fps.frametime,               L"Show Frametime in Framerate Counter",                       osd_ini,         L"Monitor.FPS",           L"DisplayFrametime"),
    ConfigEntry (monitoring.fps.advanced,                L"Show Advanced Statistics in Framerate Counter",             osd_ini,         L"Monitor.FPS",           L"AdvancedStatistics"),
    ConfigEntry (monitoring.time.show,                   L"Show System Clock",                                         osd_ini,         L"Monitor.Time",          L"Show"),

    ConfigEntry (prefer_fahrenheit,                      L"Prefer Fahrenheit Units",                                   osd_ini,         L"SpecialK.OSD",          L"PreferFahrenheit"),

    ConfigEntry (imgui.scale,                            L"ImGui Scale",                                               osd_ini,         L"ImGui.Global",          L"FontScale"),
    ConfigEntry (imgui.show_playtime,                    L"Display Playing Time in Config UI",                         osd_ini,         L"ImGui.Global",          L"ShowPlaytime"),
    ConfigEntry (imgui.show_gsync_status,                L"Show G-Sync Status on Control Panel",                       osd_ini,         L"ImGui.Global",          L"ShowGSyncStatus"),
    ConfigEntry (imgui.mac_style_menu,                   L"Use Mac-style Menu Bar",                                    osd_ini,         L"ImGui.Global",          L"UseMacStyleMenu"),
    ConfigEntry (imgui.show_input_apis,                  L"Show Input APIs currently in-use",                          osd_ini,         L"ImGui.Global",          L"ShowActiveInputAPIs"),

    ConfigEntry (screenshots.keep_png_copy,              L"Keep a .PNG compressed copy of each screenshot?",           osd_ini,         L"Screenshot.System",     L"KeepLosslessPNG"),
    ConfigEntry (screenshots.play_sound,                 L"Play a Sound when triggeirng Screenshot Capture",           osd_ini,         L"Screenshot.System",     L"PlaySoundOnCapture"),
    ConfigEntry (screenshots.copy_to_clipboard,          L"Copy an LDR copy to the Windows Clipboard",                 osd_ini,         L"Screenshot.System",     L"CopyToClipboard"),
    Keybind ( &config.render.keys.hud_toggle,            L"Toggle Game's HUD",                                         osd_ini,         L"Game.HUD"),
    Keybind ( &config.screenshots.game_hud_free_keybind, L"Take a screenshot without the HUD",                         osd_ini,         L"Screenshot.System"),
    Keybind ( &config.screenshots.sk_osd_free_keybind,   L"Take a screenshot without SK's OSD",                        osd_ini,         L"Screenshot.System"),
    Keybind ( &config.screenshots.
                               sk_osd_insertion_keybind, L"Take a screenshot and insert SK's OSD",                     osd_ini,         L"Screenshot.System"),


    // Input
    //////////////////////////////////////////////////////////////////////////

    ConfigEntry (input.keyboard.catch_alt_f4,            L"If the game does not handle Alt+F4, offer a replacement",   dll_ini,         L"Input.Keyboard",        L"CatchAltF4"),
    ConfigEntry (input.keyboard.bypass_alt_f4,           L"Forcefully disable a game's Alt+F4 handler",                dll_ini,         L"Input.Keyboard",        L"BypassAltF4Handler"),
    ConfigEntry (input.keyboard.disabled_to_game,        L"Completely stop all keyboard input from reaching the Game", dll_ini,         L"Input.Keyboard",        L"DisabledToGame"),

    ConfigEntry (input.mouse.disabled_to_game,           L"Completely stop all mouse input from reaching the Game",    dll_ini,         L"Input.Mouse",           L"DisabledToGame"),

    ConfigEntry (input.cursor.manage,                    L"Manage Cursor Visibility (due to inactivity)",              dll_ini,         L"Input.Cursor",          L"Manage"),
    ConfigEntry (input.cursor.keys_activate,             L"Keyboard Input Activates Cursor",                           dll_ini,         L"Input.Cursor",          L"KeyboardActivates"),
    ConfigEntry (input.cursor.timeout,                   L"Inactivity Timeout (in milliseconds)",                      dll_ini,         L"Input.Cursor",          L"Timeout"),
    ConfigEntry (input.cursor.ui_capture,                L"Forcefully Capture Mouse Cursor in UI Mode",                dll_ini,         L"Input.Cursor",          L"ForceCaptureInUI"),
    ConfigEntry (input.cursor.hw_cursor,                 L"Use a Hardware Cursor for Special K's UI Features",         dll_ini,         L"Input.Cursor",          L"UseHardwareCursor"),
    ConfigEntry (input.cursor.block_invisible,           L"Block Mouse Input if Hardware Cursor is Invisible",         dll_ini,         L"Input.Cursor",          L"BlockInvisibleCursorInput"),
    ConfigEntry (input.cursor.fix_synaptics,             L"Fix Synaptic Touchpad Scroll",                              dll_ini,         L"Input.Cursor",          L"FixSynapticsTouchpadScroll"),
    ConfigEntry (input.cursor.use_relative_input,        L"Use Raw Input Relative Motion if Needed",                   dll_ini,         L"Input.Cursor",          L"UseRelativeInput"),
    ConfigEntry (input.cursor.antiwarp_deadzone,         L"Percentage of Screen that the game may try to move the "
                                                         L"cursor to for mouselook.",                                  dll_ini,         L"Input.Cursor",          L"AntiwarpDeadzonePercent"),
    ConfigEntry (input.cursor.no_warp_ui,                L"Prevent Games from Warping Cursor while Config UI is Open", dll_ini,         L"Input.Cursor",          L"NoWarpUI"),
    ConfigEntry (input.cursor.no_warp_visible,           L"Prevent Games from Warping Cursor while Cursor is Visible", dll_ini,         L"Input.Cursor",          L"NoWarpVisibleGameCursor"),

    ConfigEntry (input.gamepad.disabled_to_game,         L"Disable ALL Gamepad Input (across all APIs)",               dll_ini,         L"Input.Gamepad",         L"DisabledToGame"),
    ConfigEntry (input.gamepad.disable_ps4_hid,          L"Disable PS4 HID Interface (prevent double-input)",          dll_ini,         L"Input.Gamepad",         L"DisablePS4HID"),
    ConfigEntry (input.gamepad.haptic_ui,                L"Give tactile feedback on gamepads when navigating the UI",  dll_ini,         L"Input.Gamepad",         L"AllowHapticUI"),
    ConfigEntry (input.gamepad.hook_dinput8,             L"Install hooks for DirectInput 8",                           dll_ini,         L"Input.Gamepad",         L"EnableDirectInput8"),
    ConfigEntry (input.gamepad.hook_dinput7,             L"Install hooks for DirectInput 7",                           dll_ini,         L"Input.Gamepad",         L"EnableDirectInput7"),
    ConfigEntry (input.gamepad.hook_hid,                 L"Install hooks for HID",                                     dll_ini,         L"Input.Gamepad",         L"EnableHID"),
    ConfigEntry (input.gamepad.native_ps4,               L"Native PS4 Mode (temporary)",                               dll_ini,         L"Input.Gamepad",         L"EnableNativePS4"),
    ConfigEntry (input.gamepad.disable_rumble,           L"Disable Rumble from ALL SOURCES (across all APIs)",         dll_ini,         L"Input.Gamepad",         L"DisableRumble"),

    ConfigEntry (input.gamepad.hook_xinput,              L"Install hooks for XInput",                                  dll_ini,         L"Input.XInput",          L"Enable"),
    ConfigEntry (input.gamepad.rehook_xinput,            L"Re-install XInput hooks if hookchain is modified",          dll_ini,         L"Input.XInput",          L"Rehook"),
    ConfigEntry (input.gamepad.xinput.ui_slot,           L"XInput Controller that owns the config UI",                 dll_ini,         L"Input.XInput",          L"UISlot"),
    ConfigEntry (input.gamepad.xinput.placeholders,      L"XInput Controller Slots to Fake Connectivity On",           dll_ini,         L"Input.XInput",          L"PlaceholderMask"),
    ConfigEntry (input.gamepad.xinput.assignment,        L"Re-Assign XInput Slots",                                    dll_ini,         L"Input.XInput",          L"SlotReassignment"),
  //DEPRECATED  (                                                                                                                       L"Input.XInput",          L"DisableRumble"),

    ConfigEntry (input.gamepad.steam.ui_slot,            L"Steam Controller that owns the config UI",                  dll_ini,         L"Input.Steam",           L"UISlot"),

    // Thread Monitoring
    //////////////////////////////////////////////////////////////////////////

    ConfigEntry (threads.enable_mem_alloc_trace,         L"Trace per-Thread Memory Allocation in Threads Widget",      dll_ini,         L"Threads.Analyze",       L"MemoryAllocation"),
    ConfigEntry (threads.enable_file_io_trace,           L"Trace per-Thread File I/O Activity in Threads Widget",      dll_ini,         L"Threads.Analyze",       L"FileActivity"),

    // Window Management
    //////////////////////////////////////////////////////////////////////////

    ConfigEntry (window.borderless,                      L"Borderless Window Mode",                                    dll_ini,         L"Window.System",         L"Borderless"),
    ConfigEntry (window.center,                          L"Center the Window",                                         dll_ini,         L"Window.System",         L"Center"),
    ConfigEntry (window.background_render,               L"Render While Window is in Background",                      dll_ini,         L"Window.System",         L"RenderInBackground"),
    ConfigEntry (window.background_mute,                 L"Mute While Window is in Background",                        dll_ini,         L"Window.System",         L"MuteInBackground"),
    ConfigEntry (window.offset.x,                        L"X Offset (Percent or Absolute)",                            dll_ini,         L"Window.System",         L"XOffset"),
    ConfigEntry (window.offset.y,                        L"Y Offset (Percent or Absolute)",                            dll_ini,         L"Window.System",         L"YOffset"),
    ConfigEntry (window.confine_cursor,                  L"Confine the Mouse Cursor to the Game Window",               dll_ini,         L"Window.System",         L"ConfineCursor"),
    ConfigEntry (window.unconfine_cursor,                L"Unconfine the Mouse Cursor from the Game Window",           dll_ini,         L"Window.System",         L"UnconfineCursor"),
    ConfigEntry (window.persistent_drag,                 L"Remember where the window is dragged to",                   dll_ini,         L"Window.System",         L"PersistentDragPos"),
    ConfigEntry (window.fullscreen,                      L"Make the Game Window Fill the Screen (scale to fit)",       dll_ini,         L"Window.System",         L"Fullscreen"),
    ConfigEntry (window.override,                        L"Force the Client Region to this Size in Windowed Mode",     dll_ini,         L"Window.System",         L"OverrideRes"),
    ConfigEntry (window.fix_mouse_coords,                L"Re-Compute Mouse Coordinates for Resized Windows",          dll_ini,         L"Window.System",         L"FixMouseCoords"),
    ConfigEntry (window.always_on_top,                   L"Prevent (0) or Force (1) a game's window Always-On-Top",    dll_ini,         L"Window.System",         L"AlwaysOnTop"),
    ConfigEntry (window.disable_screensaver,             L"Prevent the Windows Screensaver from activating",           dll_ini,         L"Window.System",         L"DisableScreensaver"),
    ConfigEntry (window.dont_hook_wndproc,               L"Disable WndProc / ClassProc hooks (wrap instead of hook)",  dll_ini,         L"Window.System",         L"DontHookWndProc"),

    // Compatibility
    //////////////////////////////////////////////////////////////////////////

    ConfigEntry (compatibility.disable_nv_bloat,         L"Disable All NVIDIA BloatWare (GeForce Experience)",         dll_ini,         L"Compatibility.General", L"DisableBloatWare_NVIDIA"),
    ConfigEntry (compatibility.rehook_loadlibrary,       L"Rehook LoadLibrary When RTSS/Steam/ReShade hook it",        dll_ini,         L"Compatibility.General", L"RehookLoadLibrary"),

    ConfigEntry (apis.last_known,                        L"Last Known Render API",                                     dll_ini,         L"API.Hook",              L"LastKnown"),

#ifdef _M_IX86
    ConfigEntry (apis.ddraw.hook,                        L"Enable DirectDraw Hooking",                                 dll_ini,         L"API.Hook",              L"ddraw"),
    ConfigEntry (apis.d3d8.hook,                         L"Enable Direct3D 8 Hooking",                                 dll_ini,         L"API.Hook",              L"d3d8"),
#endif

    ConfigEntry (apis.d3d9.hook,                         L"Enable Direct3D 9 Hooking",                                 dll_ini,         L"API.Hook",              L"d3d9"),
    ConfigEntry (apis.d3d9ex.hook,                       L"Enable Direct3D 9Ex Hooking",                               dll_ini,         L"API.Hook",              L"d3d9ex"),
    ConfigEntry (apis.d3d11.hook,                        L"Enable Direct3D 11 Hooking",                                dll_ini,         L"API.Hook",              L"d3d11"),

#ifdef _M_AMD64
    ConfigEntry (apis.d3d12.hook,                        L"Enable Direct3D 12 Hooking",                                dll_ini,         L"API.Hook",              L"d3d12"),
    ConfigEntry (apis.Vulkan.hook,                       L"Enable Vulkan Hooking",                                     dll_ini,         L"API.Hook",              L"Vulkan"),
#endif

    ConfigEntry (apis.OpenGL.hook,                       L"Enable OpenGL Hooking",                                     dll_ini,         L"API.Hook",              L"OpenGL"),


    // Misc.
    //////////////////////////////////////////////////////////////////////////

      // Hidden setting (it will be read, but not written -- setting this is discouraged and I intend to phase it out)
    ConfigEntry (mem_reserve,                            L"Memory Reserve Percentage",                                 dll_ini,         L"Manage.Memory",         L"ReservePercent"),


    // General Mod System Settings
    //////////////////////////////////////////////////////////////////////////

    ConfigEntry (silent,                                 L"Log Silence",                                               dll_ini,         L"SpecialK.System",       L"Silent"),
    ConfigEntry (strict_compliance,                      L"Strict DLL Loader Compliance",                              dll_ini,         L"SpecialK.System",       L"StrictCompliant"),
    ConfigEntry (trace_libraries,                        L"Trace DLL Loading (needed for dynamic API detection)",      dll_ini,         L"SpecialK.System",       L"TraceLoadLibrary"),
    ConfigEntry (log_level,                              L"Log Verbosity (0=General, 5=Insane Debug)",                 dll_ini,         L"SpecialK.System",       L"LogLevel"),
    ConfigEntry (handle_crashes,                         L"Use Custom Crash Handler",                                  dll_ini,         L"SpecialK.System",       L"UseCrashHandler"),
    ConfigEntry (debug_wait,                             L"Halt Special K Initialization Until Debugger is Attached",  dll_ini,         L"SpecialK.System",       L"WaitForDebugger"),
    ConfigEntry (debug_output,                           L"Print Application's Debug Output in real-time",             dll_ini,         L"SpecialK.System",       L"DebugOutput"),
    ConfigEntry (game_output,                            L"Log Application's Debug Output",                            dll_ini,         L"SpecialK.System",       L"GameOutput"),
    ConfigEntry (ignore_rtss_delay,                      L"Ignore RTSS Delay Incompatibilities",                       dll_ini,         L"SpecialK.System",       L"IgnoreRTSSHookDelay"),
    ConfigEntry (enable_cegui,                           L"Enable CEGUI (lazy loading)",                               dll_ini,         L"SpecialK.System",       L"EnableCEGUI"),
    ConfigEntry (safe_cegui,                             L"Safely Initialize CEGUI",                                   dll_ini,         L"SpecialK.System",       L"SafeInitCEGUI"),
    ConfigEntry (init_delay,                             L"Delay Global Injection Initialization for x-many Seconds",  dll_ini,         L"SpecialK.System",       L"GlobalInjectDelay"),
    ConfigEntry (version,                                L"The last version that wrote the config file",               dll_ini,         L"SpecialK.System",       L"Version"),



    ConfigEntry (display.force_fullscreen,               L"Force Fullscreen Mode",                                     dll_ini,         L"Display.Output",        L"ForceFullscreen"),
    ConfigEntry (display.force_windowed,                 L"Force Windowed Mode",                                       dll_ini,         L"Display.Output",        L"ForceWindowed"),
    ConfigEntry (render.osd.hdr_luminance,               L"OSD's Luminance (cd.m^-2) in HDR games",                    dll_ini,         L"Render.OSD",            L"HDRLuminance"),


    // Framerate Limiter
    //////////////////////////////////////////////////////////////////////////

    ConfigEntry (render.framerate.target_fps,            L"Framerate Target (negative signed values are non-limiting)",dll_ini,         L"Render.FrameRate",      L"TargetFPS"),
    ConfigEntry (render.framerate.target_fps_bg,         L"Framerate Target (window in background;  0.0 = same as fg)",dll_ini,         L"Render.FrameRate",      L"BackgroundFPS"),
    ConfigEntry (render.framerate.limiter_tolerance,     L"Limiter Tolerance",                                         dll_ini,         L"Render.FrameRate",      L"LimiterTolerance"),
    ConfigEntry (render.framerate.wait_for_vblank,       L"Limiter Will Wait for VBLANK",                              dll_ini,         L"Render.FrameRate",      L"WaitForVBLANK"),
    ConfigEntry (render.framerate.buffer_count,          L"Number of Backbuffers in the Swapchain",                    dll_ini,         L"Render.FrameRate",      L"BackBufferCount"),
    ConfigEntry (render.framerate.present_interval,      L"Presentation Interval (VSYNC)",                             dll_ini,         L"Render.FrameRate",      L"PresentationInterval"),
    ConfigEntry (render.framerate.prerender_limit,       L"Maximum Frames to Render-Ahead",                            dll_ini,         L"Render.FrameRate",      L"PreRenderLimit"),
    ConfigEntry (render.framerate.sleepless_render,      L"Sleep Free Render Thread",                                  dll_ini,         L"Render.FrameRate",      L"SleeplessRenderThread"),
    ConfigEntry (render.framerate.sleepless_window,      L"Sleep Free Window Thread",                                  dll_ini,         L"Render.FrameRate",      L"SleeplessWindowThread"),
    ConfigEntry (render.framerate.enable_mmcss,          L"Enable Multimedia Class Scheduling for FPS Limiter Sleep",  dll_ini,         L"Render.FrameRate",      L"EnableMMCSS"),
    ConfigEntry (render.framerate.refresh_rate,          L"Fullscreen Refresh Rate",                                   dll_ini,         L"Render.FrameRate",      L"RefreshRate"),
    ConfigEntry (render.framerate.rescan_ratio,          L"Fullscreen Rational Scan Rate (precise refresh rate)",      dll_ini,         L"Render.FrameRate",      L"RescanRatio"),
    ConfigEntry (render.framerate.enforcement_policy,    L"Place Framerate Limiter Wait Before/After Present, etc.",   dll_ini,         L"Render.FrameRate",      L"LimitEnforcementPolicy"),

    ConfigEntry (render.framerate.control.render_ahead,  L"Maximum number of CPU-side frames to work ahead of GPU.",   dll_ini,         L"FrameRate.Control",     L"MaxRenderAheadFrames"),
    ConfigEntry (render.framerate.override_cpu_count,    L"Number of CPU cores to tell the game about",                dll_ini,         L"FrameRate.Control",     L"OverrideCPUCoreCount"),

    ConfigEntry (render.framerate.allow_dwm_tearing,     L"Enable DWM Tearing (Windows 10+)",                          dll_ini,         L"Render.DXGI",           L"AllowTearingInDWM"),
    ConfigEntry (render.framerate.drop_late_frames,      L"Enable Flip Model to Render (and drop) frames at rates >"
                                                         L"refresh rate with VSYNC enabled (similar to NV Fast Sync).",dll_ini,         L"Render.DXGI",           L"DropLateFrames"),
    ConfigEntry (render.framerate.auto_low_latency,      L"If G-Sync is seen supported, automatically change limiter"
                                                         L" mode to low-latency.",                                     dll_ini,         L"Render.DXGI",           L"AutoLowLatency"),

    ConfigEntry (nvidia.reflex.enable,                   L"Enable NVIDIA Reflex Integration w/ SK's limiter",          dll_ini,         L"NVIDIA.Reflex",         L"Enable"),
    ConfigEntry (nvidia.reflex.low_latency,              L"Low Latency Mode",                                          dll_ini,         L"NVIDIA.Reflex",         L"LowLatency"),
    ConfigEntry (nvidia.reflex.low_latency_boost,        L"Reflex Boost (lower-latency power scaling)",                dll_ini,         L"NVIDIA.Reflex",         L"LowLatencyBoost"),
    ConfigEntry (nvidia.reflex.marker_optimization,      L"Train Reflex using Latency Markers for Optimization",       dll_ini,         L"NVIDIA.Reflex",         L"OptimizeByMarkers"),
    ConfigEntry (nvidia.reflex.engagement_policy,        L"When to apply Reflex's magic",                              dll_ini,         L"NVIDIA.Reflex",         L"EngagementPolicy"),

    // OpenGL
    //////////////////////////////////////////////////////////////////////////

    ConfigEntry (render.osd.draw_in_vidcap,              L"Changes hook order in order to allow recording the OSD.",   dll_ini,         L"Render.OSD",            L"ShowInVideoCapture"),

    // D3D9
    //////////////////////////////////////////////////////////////////////////

    ConfigEntry (render.d3d9.force_d3d9ex,               L"Force D3D9Ex Context",                                      dll_ini,         L"Render.D3D9",           L"ForceD3D9Ex"),
    ConfigEntry (render.d3d9.impure,                     L"Force PURE device off",                                     dll_ini,         L"Render.D3D9",           L"ForceImpure"),
    ConfigEntry (render.d3d9.enable_texture_mods,        L"Enable Texture Modding Support",                            dll_ini,         L"Render.D3D9",           L"EnableTextureMods"),


    // D3D10/11/12
    //////////////////////////////////////////////////////////////////////////

    ConfigEntry (render.framerate.max_delta_time,        L"Maximum Frame Delta Time",                                  dll_ini,         L"Render.DXGI",           L"MaxDeltaTime"),
    ConfigEntry (render.framerate.flip_discard,          L"Use Flip Discard - Windows 10+",                            dll_ini,         L"Render.DXGI",           L"UseFlipDiscard"),
    ConfigEntry (render.framerate.disable_flip_model,    L"Disable Flip Model - Fix AMD Drivers in Yakuza0",           dll_ini,         L"Render.DXGI",           L"DisableFlipModel"),

    ConfigEntry (render.dxgi.adapter_override,           L"Override DXGI Adapter",                                     dll_ini,         L"Render.DXGI",           L"AdapterOverride"),
    ConfigEntry (render.dxgi.max_res,                    L"Maximum Resolution To Report",                              dll_ini,         L"Render.DXGI",           L"MaxRes"),
    ConfigEntry (render.dxgi.min_res,                    L"Minimum Resolution To Report",                              dll_ini,         L"Render.DXGI",           L"MinRes"),

    ConfigEntry (render.dxgi.swapchain_wait,             L"Time to wait in msec. for SwapChain",                       dll_ini,         L"Render.DXGI",           L"SwapChainWait"),
    ConfigEntry (render.dxgi.scaling_mode,               L"Scaling Preference (DontCare | Centered | Stretched"
                                                         L" | Unspecified)",                                           dll_ini,         L"Render.DXGI",           L"Scaling"),
    ConfigEntry (render.dxgi.exception_mode,             L"D3D11 Exception Handling (DontCare | Raise | Ignore)",      dll_ini,         L"Render.DXGI",           L"ExceptionMode"),

    ConfigEntry (render.dxgi.debug_layer,                L"DXGI Debug Layer Support",                                  dll_ini,         L"Render.DXGI",           L"EnableDebugLayer"),
    ConfigEntry (render.dxgi.scanline_order,             L"Scanline Order (DontCare | Progressive | LowerFieldFirst |"
                                                         L" UpperFieldFirst )",                                        dll_ini,         L"Render.DXGI",           L"ScanlineOrder"),
    ConfigEntry (render.dxgi.rotation,                   L"Screen Rotation (DontCare | Identity | 90 | 180 | 270 )",   dll_ini,         L"Render.DXGI",           L"Rotation"),
    ConfigEntry (render.dxgi.test_present,               L"Test SwapChain Presentation Before Actually Presenting",    dll_ini,         L"Render.DXGI",           L"TestSwapChainPresent"),
    ConfigEntry (render.dxgi.safe_fullscreen,            L"Prevent DXGI Deadlocks in Improperly Written Games",        dll_ini,         L"Render.DXGI",           L"SafeFullscreenMode"),
    ConfigEntry (render.dxgi.enhanced_depth,             L"Use 32-bit Depth + 8-bit Stencil + 24-bit Padding",         dll_ini,         L"Render.DXGI",           L"Use64BitDepthStencil"),
    ConfigEntry (render.dxgi.deferred_isolation,         L"Isolate D3D11 Deferred Context Queues instead of Tracking"
                                                         L" in Immediate Mode.",                                       dll_ini,         L"Render.DXGI",           L"IsolateD3D11DeferredContexts"),
    ConfigEntry (render.dxgi.msaa_samples,               L"Override ON-SCREEN Multisample Antialiasing Level;-1=None", dll_ini,         L"Render.DXGI",           L"OverrideMSAA"),
    ConfigEntry (render.dxgi.skip_present_test,          L"Nix the swapchain present flag: DXGI_PRESENT_TEST to "
                                                         L"workaround bad third-party software that doesn't handle it"
                                                         L" correctly.",                                               dll_ini,         L"Render.DXGI",           L"SkipSwapChainPresentTest"),
    ConfigEntry (render.dxgi.srgb_behavior,              L"How to handle sRGB SwapChains when we have to kill them for"
                                                         L" Flip Model support (-1=Passthrough, 0=Strip, 1=Apply)",    dll_ini,         L"Render.DXGI",           L"sRGBBypassBehavior"),
    ConfigEntry (render.dxgi.low_spec_mode,              L"Disable D3D11 Render Mods (for slight perf. increase)",     dll_ini,         L"Render.DXGI",           L"LowSpecMode"),
    ConfigEntry (render.dxgi.hide_hdr_support,           L"Prevent games from detecting monitor HDR support",          dll_ini,         L"Render.DXGI",           L"HideHDRSupport"),

    ConfigEntry (texture.d3d9.clamp_lod_bias,            L"Clamp Negative LOD Bias",                                   dll_ini,         L"Textures.D3D9",         L"ClampNegativeLODBias"),
    ConfigEntry (texture.d3d11.cache,                    L"Cache Textures",                                            dll_ini,         L"Textures.D3D11",        L"Cache"),
    ConfigEntry (texture.d3d11.precise_hash,             L"Precise Hash Generation",                                   dll_ini,         L"Textures.D3D11",        L"PreciseHash"),

    ConfigEntry (texture.d3d11.inject,                   L"Inject Textures",                                           dll_ini,         L"Textures.D3D11",        L"Inject"),
    ConfigEntry (texture.d3d11.injection_keeps_format,   L"Allow image format to change during texture injection",     dll_ini,         L"Textures.D3D11",        L"InjectionKeepsFormat"),
    ConfigEntry (texture.d3d11.gen_mips,                 L"Create complete mipmap chain for textures without them",    dll_ini,         L"Textures.D3D11",        L"GenerateMipmaps"),
    ConfigEntry (texture.res_root,                       L"Resource Root",                                             dll_ini,         L"Textures.General",      L"ResourceRoot"),
    ConfigEntry (texture.dump_on_load,                   L"Dump Textures while Loading",                               dll_ini,         L"Textures.General",      L"DumpOnFirstLoad"),
    ConfigEntry (texture.cache.min_entries,              L"Minimum Cached Textures",                                   dll_ini,         L"Textures.Cache",        L"MinEntries"),
    ConfigEntry (texture.cache.max_entries,              L"Maximum Cached Textures",                                   dll_ini,         L"Textures.Cache",        L"MaxEntries"),
    ConfigEntry (texture.cache.min_evict,                L"Minimum Textures to Evict",                                 dll_ini,         L"Textures.Cache",        L"MinEvict"),
    ConfigEntry (texture.cache.max_evict,                L"Maximum Textures to Evict",                                 dll_ini,         L"Textures.Cache",        L"MaxEvict"),
    ConfigEntry (texture.cache.min_size,                 L"Minimum Data Size to Evict",                                dll_ini,         L"Textures.Cache",        L"MinSizeInMiB"),
    ConfigEntry (texture.cache.max_size,                 L"Maximum Data Size to Evict",                                dll_ini,         L"Textures.Cache",        L"MaxSizeInMiB"),

    ConfigEntry (texture.cache.ignore_non_mipped,        L"Ignore textures without mipmaps?",                          dll_ini,         L"Textures.Cache",        L"IgnoreNonMipmapped"),
    ConfigEntry (texture.cache.allow_staging,            L"Enable texture caching/dumping/injecting staged textures",  dll_ini,         L"Textures.Cache",        L"AllowStaging"),
    ConfigEntry (texture.cache.allow_unsafe_refs,        L"For games with broken resource reference counting, allow"
                                                         L" textures to be cached anyway (needed for injection).",     dll_ini,         L"Textures.Cache",        L"AllowUnsafeRefCounting"),
    ConfigEntry (texture.cache.manage_residency,         L"Actively manage D3D11 teture residency",                    dll_ini,         L"Textures.Cache",        L"ManageResidency"),


    ConfigEntry (nvidia.api.disable,                     L"Disable NvAPI",                                             dll_ini,         L"NVIDIA.API",            L"Disable"),
    ConfigEntry (nvidia.api.disable_hdr,                 L"Prevent Game from Using NvAPI HDR Features",                dll_ini,         L"NVIDIA.API",            L"DisableHDR"),
    ConfigEntry (nvidia.bugs.snuffed_ansel,              L"By default, Special K disables Ansel at first launch, but"
                                                         L" users have an option under 'Help|..' to turn it back on.", dll_ini,         L"NVIDIA.Bugs",           L"AnselSleepsWithFishes"),
    ConfigEntry (nvidia.bugs.bypass_ansel,               L"Forcefully block nvcamera{64}.dll",                         dll_ini,         L"NVIDIA.Bugs",           L"DisableAnselShimLoader"),
    ConfigEntry (nvidia.sli.compatibility,               L"SLI Compatibility Bits",                                    dll_ini,         L"NVIDIA.SLI",            L"CompatibilityBits"),
    ConfigEntry (nvidia.sli.num_gpus,                    L"SLI GPU Count",                                             dll_ini,         L"NVIDIA.SLI",            L"NumberOfGPUs"),
    ConfigEntry (nvidia.sli.mode,                        L"SLI Mode",                                                  dll_ini,         L"NVIDIA.SLI",            L"Mode"),
    ConfigEntry (nvidia.sli.override,                    L"Override Driver Defaults",                                  dll_ini,         L"NVIDIA.SLI",            L"Override"),

    ConfigEntry (amd.adl.disable,                        L"Disable AMD's ADL library",                                 dll_ini,         L"AMD.ADL",               L"Disable"),

    ConfigEntry (imgui.show_eula,                        L"Show Software EULA",                                        dll_ini,         L"SpecialK.System",       L"ShowEULA"),
    ConfigEntry (imgui.disable_alpha,                    L"Disable Alpha Transparency (reduce flicker)",               dll_ini,         L"ImGui.Render",          L"DisableAlpha"),
    ConfigEntry (imgui.antialias_lines,                  L"Reduce Aliasing on (but dim) Line Edges",                   dll_ini,         L"ImGui.Render",          L"AntialiasLines"),
    ConfigEntry (imgui.antialias_contours,               L"Reduce Aliasing on (but widen) Window Borders",             dll_ini,         L"ImGui.Render",          L"AntialiasContours"),

    ConfigEntry (dpi.disable,                            L"Disable DPI Scaling",                                       dll_ini,         L"DPI.Scaling",           L"Disable"),
    ConfigEntry (dpi.per_monitor_aware,                  L"Windows 8.1+ Monitor DPI Awareness (needed for UE4)",       dll_ini,         L"DPI.Scaling",           L"PerMonitorAware"),
    ConfigEntry (dpi.per_monitor_all_threads,            L"Further fixes required for UE4",                            dll_ini,         L"DPI.Scaling",           L"MonitorAwareOnAllThreads"),

    ConfigEntry (reverse_engineering.file.trace_reads,   L"Log file read activity to logs/file_read.log",              dll_ini,         L"FileIO.Trace",          L"LogReads"),
    ConfigEntry (reverse_engineering.file.ignore_reads,  L"Don't log activity for files in this list",                 dll_ini,         L"FileIO.Trace",          L"IgnoreReads"),
    ConfigEntry (reverse_engineering.file.trace_writes,  L"Log file write activity to logs/file_write.log",            dll_ini,         L"FileIO.Trace",          L"LogWrites"),
    ConfigEntry (reverse_engineering.file.ignore_writes, L"Don't log activity for files in this list",                 dll_ini,         L"FileIO.Trace",          L"IgnoreWrites"),

    ConfigEntry (cpu.power_scheme_guid,                  L"Power Policy (GUID) to Apply At Application Start",         dll_ini,         L"CPU.Power",             L"PowerSchemeGUID"),


    // The one odd-ball Steam achievement setting that can be specified per-game
    ConfigEntry (steam.achievements.sound_file,          L"Achievement Sound File",                                    dll_ini,         L"Steam.Achievements",    L"SoundFile"),

    // Steam Achievement Enhancements  (Global Settings)
    //////////////////////////////////////////////////////////////////////////

    ConfigEntry (steam.achievements.play_sound,          L"Silence is Bliss?",                                         steam_ini,       L"Steam.Achievements",    L"PlaySound"),
    ConfigEntry (steam.achievements.take_screenshot,     L"Precious Memories",                                         steam_ini,       L"Steam.Achievements",    L"TakeScreenshot"),
    ConfigEntry (steam.achievements.fetch_friend_stats,  L"Friendly Competition",                                      steam_ini,       L"Steam.Achievements",    L"FetchFriendStats"),
    ConfigEntry (steam.achievements.popup.origin,        L"Achievement Popup Position",                                steam_ini,       L"Steam.Achievements",    L"PopupOrigin"),
    ConfigEntry (steam.achievements.popup.animate,       L"Achievement Notification Animation",                        steam_ini,       L"Steam.Achievements",    L"AnimatePopup"),
    ConfigEntry (steam.achievements.popup.show_title,    L"Achievement Popup Includes Game Title?",                    steam_ini,       L"Steam.Achievements",    L"ShowPopupTitle"),
    ConfigEntry (steam.achievements.popup.inset,         L"Achievement Notification Inset X",                          steam_ini,       L"Steam.Achievements",    L"PopupInset"),
    ConfigEntry (steam.achievements.popup.duration,      L"Achievement Popup Duration (in ms)",                        steam_ini,       L"Steam.Achievements",    L"PopupDuration"),

    ConfigEntry (steam.system.notify_corner,             L"Overlay Notification Position  (non-Big Picture Mode)",     dll_ini,         L"Steam.System",          L"NotifyCorner"),
    ConfigEntry (steam.system.appid,                     L"Steam AppID",                                               dll_ini,         L"Steam.System",          L"AppID"),
    ConfigEntry (steam.system.init_delay,                L"Delay SteamAPI initialization if the game doesn't do it",   dll_ini,         L"Steam.System",          L"AutoInitDelay"),
    ConfigEntry (steam.system.auto_pump,                 L"Should we force the game to run Steam callbacks?",          dll_ini,         L"Steam.System",          L"AutoPumpCallbacks"),
    ConfigEntry (steam.system.block_stat_callback,       L"Block the User Stats Receipt Callback?",                    dll_ini,         L"Steam.System",          L"BlockUserStatsCallback"),
    ConfigEntry (steam.system.filter_stat_callbacks,     L"Filter Unrelated Data from the User Stats Receipt Callback",dll_ini,         L"Steam.System",          L"FilterExternalDataFromCallbacks"),
    ConfigEntry (steam.system.load_early,                L"Load the Steam Client DLL Early?",                          dll_ini,         L"Steam.System",          L"PreLoadSteamClient"),
    ConfigEntry (steam.system.early_overlay,             L"Load the Steam Overlay Early",                              dll_ini,         L"Steam.System",          L"PreLoadSteamOverlay"),
    ConfigEntry (steam.system.force_load,                L"Forcefully load steam_api{64}.dll",                         dll_ini,         L"Steam.System",          L"ForceLoadSteamAPI"),
    ConfigEntry (steam.system.auto_inject,               L"Automatically load steam_api{64}.dll into any game whose "
                                                         L"path includes SteamApps\\common, but doesn't use steam_api",dll_ini,         L"Steam.System",          L"AutoInjectSteamAPI"),
    ConfigEntry (steam.system.reuse_overlay_pause,       L"Pause Overlay Aware games when control panel is visible",   dll_ini,         L"Steam.System",          L"ReuseOverlayPause"),
    ConfigEntry (steam.social.online_status,             L"Always apply a social state (defined by EPersonaState) at"
                                                         L" application start",                                        dll_ini,         L"Steam.Social",          L"OnlineStatus"),
    ConfigEntry (steam.system.dll_path,                  L"Path to a known-working SteamAPI dll for this game.",       dll_ini,         L"Steam.System",          L"SteamPipeDLL"),
    ConfigEntry (steam.callbacks.throttle,               L"-1=Unlimited, 0-oo=Upper bound limit to SteamAPI rate",     dll_ini,         L"Steam.System",          L"CallbackThrottle"),

    // This option is per-game, since it has potential compatibility issues...
    ConfigEntry (steam.screenshots.smart_capture,        L"Enhanced screenshot speed and HUD options; D3D11-only.",    dll_ini,         L"Steam.Screenshots",     L"EnableSmartCapture"),

    // These are all system-wide for all Steam games
    ConfigEntry (steam.overlay.hdr_luminance,            L"Make the Steam Overlay visible in HDR mode!",               steam_ini,       L"Steam.Overlay",         L"Luminance_scRGB"),
    ConfigEntry (screenshots.include_osd_default,        L"Should a screenshot triggered BY Steam include SK's OSD?",  steam_ini,       L"Steam.Screenshots",     L"DefaultKeybindCapturesOSD"),

    // Swashbucklers pay attention
    //////////////////////////////////////////////////////////////////////////

    ConfigEntry (steam.log.silent,                       L"Makes steam_api.log go away [DISABLES STEAMAPI FEATURES]",  dll_ini,         L"Steam.Log",             L"Silent"),
    ConfigEntry (steam.cloud.blacklist,                  L"CSV list of files to block from cloud sync.",               dll_ini,         L"Steam.Cloud",           L"FilesNotToSync"),
    ConfigEntry (steam.drm.spoof_BLoggedOn,              L"Fix For Stupid Games That Don't Know How DRM Works",        dll_ini,         L"Steam.DRMWorks",        L"SpoofBLoggedOn"),
  };


  for ( auto& decl : params_to_build )
  {
    if ( decl.type_ == std::type_index ( typeid ( sk::ParameterBool* ) ) )
    {
      *decl.parameter_ =
        SK_CreateINIParameter <sk::ParameterBool> (decl.description_, decl.ini_, decl.section_, decl.key_);

      continue;
    }

    if ( decl.type_ == std::type_index ( typeid ( sk::ParameterInt* ) ) )
    {
      *decl.parameter_ =
        SK_CreateINIParameter <sk::ParameterInt> (decl.description_, decl.ini_, decl.section_, decl.key_);

      continue;
    }

    if ( decl.type_ == std::type_index ( typeid ( sk::ParameterInt64* ) ) )
    {
      *decl.parameter_ =
        SK_CreateINIParameter <sk::ParameterInt64> (decl.description_, decl.ini_, decl.section_, decl.key_);

      continue;
    }

    if ( decl.type_ == std::type_index ( typeid ( sk::ParameterFloat* ) ) )
    {
      *decl.parameter_ =
        SK_CreateINIParameter <sk::ParameterFloat> (decl.description_, decl.ini_, decl.section_, decl.key_);

      continue;
    }

    if ( decl.type_ == std::type_index ( typeid ( sk::ParameterStringW* ) ) )
    {
      *decl.parameter_ =
        SK_CreateINIParameter <sk::ParameterStringW> (decl.description_, decl.ini_, decl.section_, decl.key_);

      continue;
    }

    if ( decl.type_ == std::type_index ( typeid ( sk::ParameterVec2f* ) ) )
    {
      *decl.parameter_ =
        SK_CreateINIParameter <sk::ParameterVec2f> (decl.description_, decl.ini_, decl.section_, decl.key_);

      continue;
    }

    if ( decl.type_ == std::type_index ( typeid ( SK_ConfigSerializedKeybind* ) ) )
    {
      ((SK_ConfigSerializedKeybind *)decl.parameter_)->param =
        DeclKeybind ( (SK_ConfigSerializedKeybind *)decl.parameter_, decl.ini_, decl.section_ );

      continue;
    }
  }

  iSK_INI::_TSectionMap& sections =
    dll_ini->get_sections ();

  auto sec =
    sections.cbegin ();

  int import = 0;

  auto& host_executable =
    imports->host_executable;

  host_executable.hLibrary     = GetModuleHandle     (nullptr);
  host_executable.product_desc = SK_GetDLLVersionStr (SK_GetModuleFullName (host_executable.hLibrary).c_str ());


  SK_KeyboardMacros->clear   ();


  if (GetFileAttributesW (master_name.c_str ()) != INVALID_FILE_ATTRIBUTES)
  {
    dll_ini->import_file (master_name.c_str ());
  }

  if (GetFileAttributesW (custom_name.c_str ()) != INVALID_FILE_ATTRIBUTES)
  {
    dll_ini->import_file (custom_name.c_str ());
  }


  while (sec != sections.cend ())
  {
    auto& import_ =
      imports->imports
         [import];

    if ((*sec).first.find (L"Import.") != std::wstring::npos)
    {
      const wchar_t* wszNext =
        wcschr ((*sec).first.c_str (), L'.');

      import_.name =
        wszNext != nullptr    ?
          CharNextW (wszNext) : L"";

      import_.filename =
         dynamic_cast <sk::ParameterStringW *>
             (g_ParameterFactory->create_parameter <std::wstring> (
                L"Import Filename")
             );
      import_.filename->register_to_ini (
        dll_ini,
          (*sec).first,
            L"Filename" );

      import_.when =
         dynamic_cast <sk::ParameterStringW *>
             (g_ParameterFactory->create_parameter <std::wstring> (
                L"Import Timeframe")
             );
      import_.when->register_to_ini (
        dll_ini,
          (*sec).first,
            L"When" );

      import_.role =
         dynamic_cast <sk::ParameterStringW *>
             (g_ParameterFactory->create_parameter <std::wstring> (
                L"Import Role")
             );
      import_.role->register_to_ini (
        dll_ini,
          (*sec).first,
            L"Role" );

      import_.architecture =
         dynamic_cast <sk::ParameterStringW *>
             (g_ParameterFactory->create_parameter <std::wstring> (
                L"Import Architecture")
             );
      import_.architecture->register_to_ini (
        dll_ini,
          (*sec).first,
            L"Architecture" );

      import_.blacklist =
         dynamic_cast <sk::ParameterStringW *>
             (g_ParameterFactory->create_parameter <std::wstring> (
                L"Blacklisted Executables")
             );
      import_.blacklist->register_to_ini (
        dll_ini,
          (*sec).first,
            L"Blacklist" );

      ((sk::iParameter *)import_.filename)->load     ();
      ((sk::iParameter *)import_.when)->load         ();
      ((sk::iParameter *)import_.role)->load         ();
      ((sk::iParameter *)import_.architecture)->load ();
      ((sk::iParameter *)import_.blacklist)->load    ();

      import_.hLibrary = nullptr;

      ++import;

      if (import > SK_MAX_IMPORTS)
        break;
    }



    if ((*sec).first.find (L"Macro.") != std::wstring::npos)
    {
      for ( auto& it : (*sec).second.keys )
      {
        SK_KeyCommand cmd;

        cmd.binding.human_readable = it.first;
        cmd.binding.parse ();

        cmd.command = it.second;

        SK_KeyboardMacros->emplace (cmd.binding.masked_code, cmd);
      }
    }



    ++sec;
  }



  //
  // Load Global Macro Table
  //
  if (macro_ini->get_sections ().empty ())
  {
    macro_ini->import ( L"[Macro.SpecialK_OSD_Toggles]\n"
                        L"Ctrl+Shift+O=OSD.Show toggle\n"
                        L"Ctrl+Shift+M=OSD.Memory.Show toggle\n"
                        L"Ctrl+Shift+T=OSD.Clock.Show toggle\n"
                        L"Ctrl+Shift+I=OSD.IOPS.Show toggle\n"
                        L"Ctrl+Shift+F=OSD.FPS.Show toggle\n"
                        L"Ctrl+Shift+G=OSD.GPU.Show toggle\n"
                        L"Ctrl+Shift+R=OSD.Shaders.Show toggle\n"
                        L"Ctrl+Alt+Shift+D=OSD.Disk.Show toggle\n"
                        L"Ctrl+Alt+Shift+P=OSD.Pagefile.Show toggle\n"
                        L"Ctrl+Alt+Shift+S=OSD.SLI.Show toggle\n"
                        L"Ctrl+Shift+C=OSD.CPU.Show toggle\n\n"
                      );
                        //L"[Macro.SpecialK_CommandConsole]\n"
                        //L"Ctrl+Shift+Tab=Console.Show toggle\n\n" );
    macro_ini->write (macro_ini->get_filename ());
  }

  auto& macro_sections =
    macro_ini->get_sections ();

  sec =
    macro_sections.begin ();

  while (sec != macro_sections.cend ())
  {
    if ((*sec).first.find (L"Macro.") != std::wstring::npos)
    {
      for ( auto& it : (*sec).second.keys )
      {
        SK_KeyCommand cmd;

        cmd.binding.human_readable = it.first;
        cmd.binding.parse ();

        cmd.command = it.second;

        SK_KeyboardMacros->emplace (cmd.binding.masked_code, cmd);
      }
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

  // Default = Don't Care
  config.render.dxgi.exception_mode = -1;
  config.render.dxgi.scaling_mode   = -1;


  //
  // Application Compatibility Overrides
  // ===================================
  //
  if (SK_GetCurrentGameID () != SK_GAME_ID::UNKNOWN_GAME)
  {
    switch (SK_GetCurrentGameID ())
    {
      case SK_GAME_ID::Tyranny:
        // Cannot auto-detect API?!
        config.apis.dxgi.d3d11.hook       = false;
        config.apis.OpenGL.hook           = false;
        config.steam.filter_stat_callback = true; // Will stop running SteamAPI when it receives
                                                  //   data it didn't ask for
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


      case SK_GAME_ID::ResidentEvil7:
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


      case SK_GAME_ID::GodEater3:
        //Does not support XInput hot-plugging, needs Special K loving :)
        config.input.gamepad.xinput.placehold [0] = true;
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
        break;

#ifdef _M_IX86
      case SK_GAME_ID::DeadlyPremonition:
        config.steam.force_load_steamapi       = true;
        config.apis.d3d9.hook                  = true;
        config.apis.d3d9ex.hook                = false;
        config.apis.d3d8.hook                  = false;
        config.input.mouse.add_relative_motion = false;
        break;
#endif

#ifdef _M_AMD64
      case SK_GAME_ID::LifeIsStrange_BeforeTheStorm:
        config.apis.d3d9.hook       = false;
        config.apis.d3d9ex.hook     = false;
        config.apis.OpenGL.hook     = false;
        config.apis.Vulkan.hook     = false;
        config.apis.dxgi.d3d11.hook = true;
        config.apis.dxgi.d3d12.hook = false;
        break;

      case SK_GAME_ID::DivinityOriginalSin:
        config.textures.cache.ignore_nonmipped = true;
        break;
#endif

      case SK_GAME_ID::Hob:
        // Fails to start Steamworks correctly, we have to kickstart it
        config.steam.force_load_steamapi = true;
        break;

#ifdef _M_IX86
      case SK_GAME_ID::DukeNukemForever:
        // The mouse cursor's coordinate space is limited to 1920x1080 even at 4K, which
        //   has the unfortunate side effect of reducing aiming precision when the game
        //     isn't using HID.
        config.window.unconfine_cursor = true; // Remap the coordinates and increase precision

        // The graphics engine doesn't import any render APIs directly aside from ddraw.dll,
        //   which is obviously not the correct API. D3D9 and only D3D9 is the correct API.
        config.apis.d3d9.hook          = true;
        config.apis.ddraw.hook         = false;
        config.apis.d3d8.hook          = false;
        config.apis.dxgi.d3d11.hook    = false;
        config.apis.OpenGL.hook        = false;
        break;
#endif

      case SK_GAME_ID::DotHackGU:
        config.cegui.safe_init         = false; // If not turned off, the game will have problems
                                                // loading its constituent DLLs
      //config.textures.d3d11.generate_mips = true;
        break;

      case SK_GAME_ID::WorldOfFinalFantasy:
      {
        config.window.borderless                 = true;
        config.window.fullscreen                 = true;
        config.window.offset.x.absolute          = -1;
        config.window.offset.y.absolute          = -1;
        config.window.center                     = false;
        config.render.framerate.buffer_count     =  3;
        config.render.framerate.target_fps       = -30.0f;
        config.render.framerate.flip_discard     = true;
        config.render.framerate.present_interval = 2;
        config.render.framerate.pre_render_limit = 2;
        config.render.framerate.sleepless_window = false;
        config.input.cursor.manage               = true;
        config.input.cursor.timeout              = 0;

        HMONITOR hMonitor =
          MonitorFromWindow ( HWND_DESKTOP,
                                MONITOR_DEFAULTTOPRIMARY );

        MONITORINFO mi   = {         };
        mi.cbSize        = sizeof (mi);
        GetMonitorInfo (hMonitor, &mi);

        config.window.res.override.x = mi.rcMonitor.right  - mi.rcMonitor.left;
        config.window.res.override.y = mi.rcMonitor.bottom - mi.rcMonitor.top;
      } break;

      case SK_GAME_ID::StarOcean4:
        // Prevent the game from layering windows always on top.
        config.window.always_on_top             = 0;
        config.window.disable_screensaver       = true;
        config.textures.d3d11.uncompressed_mips = true;
        config.textures.d3d11.cache_gen_mips    = false;
        break;

      case SK_GAME_ID::Okami:
        config.render.dxgi.deferred_isolation   = true;
        break;

      case SK_GAME_ID::Mafia3:
        config.steam.force_load_steamapi        = true;
        break;

      // (0.8.58 - 1/1/18) -> API autodetect doesn't like XNA games
      case SK_GAME_ID::Owlboy:
        config.apis.d3d9.hook       = true;
        config.apis.d3d9ex.hook     = true;
        config.apis.OpenGL.hook     = false;
        config.apis.dxgi.d3d11.hook = false;
        break;

      // (0.9.1 - 2/15/18) -> SteamAPI init happens before CBT hook injects SpecialK
      case SK_GAME_ID::SecretOfMana:
        config.steam.init_delay = 1; // Non-zero value causes forced injection
        break;

      case SK_GAME_ID::FinalFantasyXV:
        // On many systems, people have third-party software that is behaving
        //   incorrectly when the game issues DXGI_PRESENT_TEST; so disable
        //     this feature to improve performance and compat.
        config.render.dxgi.present_test_skip  = false;
        config.render.dxgi.deferred_isolation = false;

        // Replace sleep calls that would normally block the message queue with
        //   calls that wakeup and dispatch these events.
        config.render.framerate.sleepless_window = false;

        config.textures.d3d11.cache           = true;
        config.textures.cache.max_entries     = 16384; // Uses a ton of small textures

        // Don't show the cursor, ever, because the game doesn't use it.
        config.input.cursor.manage            = true;
        config.input.cursor.timeout           = 0;

        config.threads.enable_file_io_trace   = true;

        config.steam.preload_overlay          = true;

        SK_D3D11_DeclHUDShader (0x3be1c239, ID3D11VertexShader);
        SK_D3D11_DeclHUDShader (0x466e477c, ID3D11VertexShader);
        SK_D3D11_DeclHUDShader (0x588dea7a, ID3D11VertexShader);
        SK_D3D11_DeclHUDShader (0xf15a90ab, ID3D11VertexShader);

        InterlockedExchange (&SK_SteamAPI_CallbackRateLimit, 10);
        break;

      case SK_GAME_ID::NiNoKuni2:
        config.window.treat_fg_as_active      = true;
        config.window.background_render       = true;

        // Evaluate deferred command lists for state tracking
        config.render.dxgi.deferred_isolation = true;
        break;

      case SK_GAME_ID::PillarsOfEternity2:
        config.textures.cache.ignore_nonmipped    = true;
        config.textures.cache.residency_managemnt = false;
        config.apis.OpenGL.hook                   = false;
        config.steam.appid                        = 560130;
        config.steam.auto_pump_callbacks          = true;
        config.steam.auto_inject                  = true;
        config.steam.force_load_steamapi          = true;
        config.steam.dll_path                     = LR"(PillarsOfEternityII_Data\Plugins\steam_api64.dll)";
        break;

      case SK_GAME_ID::DragonBallFighterZ:
        WinExec (R"(RED\Binaries\Win64\RED-Win64-Shipping.exe)", SW_SHOWNORMAL);
        exit (0);
        break;

#ifdef _M_AMD64
      case SK_GAME_ID::FarCry5:
      {
        // Game shares buggy XInput code with Watch_Dogs2
        config.input.gamepad.xinput.placehold [0] = true;
      } break;
#endif

      case SK_GAME_ID::ChronoTrigger:
        // Don't accidentally hook the D3D9 device used for video playback
        config.apis.d3d9.hook   = false;
        config.apis.d3d9ex.hook = false;
        break;

      case SK_GAME_ID::Ys_Eight:
        config.textures.d3d11.uncompressed_mips = true;
        config.textures.d3d11.cache_gen_mips    = false;
        break;

      case SK_GAME_ID::TrailsOfColdSteel:
        SK_D3D11_DeclHUDShader (0x497dc49d, ID3D11VertexShader);
        SK_D3D11_DeclHUDShader (0x671ca0fa, ID3D11VertexShader);
        SK_D3D11_DeclHUDShader (0x75cf58de, ID3D11VertexShader);
        break;

      case SK_GAME_ID::Sekiro:
        config.input.gamepad.xinput.placehold [0] = true;
        config.input.gamepad.xinput.placehold [1] = true;
        config.input.gamepad.xinput.placehold [2] = true;
        config.input.gamepad.xinput.placehold [3] = true;
        config.render.framerate.buffer_count      = 3;
        config.render.framerate.pre_render_limit  = 4;
      //config.render.framerate.limiter_tolerance = 4.0f;

        SK_D3D11_DeclHUDShader (0x15888ef2, ID3D11VertexShader);
        SK_D3D11_DeclHUDShader (0x1893edbd, ID3D11VertexShader);
        SK_D3D11_DeclHUDShader (0x26dc61b1, ID3D11VertexShader);
        SK_D3D11_DeclHUDShader (0x3d205776, ID3D11VertexShader);
        SK_D3D11_DeclHUDShader (0x4c8dd6ec, ID3D11VertexShader);
        SK_D3D11_DeclHUDShader (0x5e74f0b4, ID3D11VertexShader);
        SK_D3D11_DeclHUDShader (0xc4ee4c93, ID3D11VertexShader);
        SK_D3D11_DeclHUDShader (0xd931a68c, ID3D11VertexShader);
        SK_D3D11_DeclHUDShader (0xdb921b64, ID3D11VertexShader);
        SK_D3D11_DeclHUDShader (0xe15a43f4, ID3D11VertexShader);
        SK_D3D11_DeclHUDShader (0xf497bad8, ID3D11VertexShader);
        SK_D3D11_DeclHUDShader (0x6fd3fed7, ID3D11VertexShader);
        config.render.dxgi.deferred_isolation = true;

        // ReShade will totally crash if it is permitted to hook D3D9
        config.apis.d3d9.hook                 = false;
        config.apis.d3d9ex.hook               = false;
        break;


      case SK_GAME_ID::SonicMania:
      {
        config.apis.d3d9ex.hook = false;
      } break;

      case SK_GAME_ID::BaldursGate3:
        config.compatibility.impersonate_debugger = true;
        break;

      case SK_GAME_ID::Persona4:
      {
        config.window.treat_fg_as_active          = true;
        config.dpi.per_monitor.aware              = true;
        config.textures.cache.ignore_nonmipped    = true;
        config.compatibility.impersonate_debugger = true;
        config.apis.d3d9.hook                     = false;
        config.apis.d3d9ex.hook                   = false;
        config.apis.OpenGL.hook                   = false;
      } break;


#ifdef _M_AMD64
      case SK_GAME_ID::GalGunReturns:
        config.input.keyboard.override_alt_f4    = true;
        break;

      case SK_GAME_ID::Persona5Strikers:
        config.apis.d3d9.hook                    = false;
        config.apis.d3d9ex.hook                  = false;
        config.input.keyboard.override_alt_f4    = true;
        config.input.keyboard.catch_alt_f4       = true;
        config.window.background_render          = true;
        break;

      case SK_GAME_ID::HorizonZeroDawn:
        // Game uses UI mouse cursor position for mouselook,
        //   it needs to be clipped because developers don't
        //     know about relative input (e.g. RawInput).
        config.input.mouse.ignore_small_clips     = false;
        config.compatibility.impersonate_debugger = false;
        break;

      case SK_GAME_ID::Yakuza0:
      {
        if (! IsProcessDPIAware ())
        {
          void SK_Display_ForceDPIAwarenessUsingAppCompat (void);
               SK_Display_ForceDPIAwarenessUsingAppCompat ();

          void SK_Display_SetMonitorDPIAwareness (bool bOnlyIfWin10);
               SK_Display_SetMonitorDPIAwareness (false);

          // Oly do this for Steam games, the Microsoft Store Yakuza games
          //   are chronically DPI unaware and broken
          if (StrStrIW (SK_GetHostPath (), L"SteamApps"))
            SK_RestartGame ();
        }

        ///// Engine has a problem with its texture management that
        /////   makes texture caching / modding impossible.
        config.textures.d3d11.cache               = false;
        config.textures.cache.allow_unsafe_refs   =  true;
        config.render.dxgi.deferred_isolation     = false;
        config.textures.cache.residency_managemnt = false;
        config.cegui.enable                       = false; // Off by default
        config.render.framerate.disable_flip      = false;
        config.render.framerate.swapchain_wait    =     1;
        config.window.borderless                  =  true;
        config.window.fullscreen                  =  true;
        config.input.keyboard.override_alt_f4     =  true;

        HMONITOR hMonitor =
          MonitorFromWindow ( HWND_DESKTOP,
                                MONITOR_DEFAULTTOPRIMARY );

        MONITORINFO mi   = {         };
        mi.cbSize        = sizeof (mi);
        GetMonitorInfo (hMonitor, &mi);

        config.window.res.override.x = mi.rcMonitor.right  - mi.rcMonitor.left;
        config.window.res.override.y = mi.rcMonitor.bottom - mi.rcMonitor.top;
      }
      break;

      case SK_GAME_ID::YakuzaKiwami2:
        config.apis.d3d9.hook                     =  false;
        config.apis.d3d9ex.hook                   =  false;

        dll_ini->import (L"[Import.ReShade64_Custom]\n"
                         L"Architecture=x64\n"
                         L"Role=Unofficial\n"
                         L"When=PlugIn\n"
                         L"Filename=ReShade64.dll\n");

        SK_D3D11_DeclHUDShader (0x062173ec, ID3D11VertexShader);
        SK_D3D11_DeclHUDShader (0x48dd4bc3, ID3D11VertexShader);
        SK_D3D11_DeclHUDShader (0x54c0d366, ID3D11VertexShader);
        SK_D3D11_DeclHUDShader (0xb943178b, ID3D11VertexShader);

      case SK_GAME_ID::YakuzaKiwami:
      case SK_GAME_ID::YakuzaUnderflow:
      {
        if (! IsProcessDPIAware ())
        {
          void SK_Display_ForceDPIAwarenessUsingAppCompat (void);
               SK_Display_ForceDPIAwarenessUsingAppCompat ();

          void SK_Display_SetMonitorDPIAwareness (bool bOnlyIfWin10);
               SK_Display_SetMonitorDPIAwareness (false);

          // Oly do this for Steam games, the Microsoft Store Yakuza games
          //   are chronically DPI unaware and broken
          if (StrStrIW (SK_GetHostPath (), L"SteamApps"))
            SK_RestartGame ();
        }

        config.textures.d3d11.cache               =  false;
        config.window.background_render           =   true;
        config.window.always_on_top               =      0;
        config.render.dxgi.deferred_isolation     =   true;
        config.render.framerate.flip_discard      =   true;
        config.steam.reuse_overlay_pause          =  false;
        config.render.framerate.swapchain_wait    =      1;
        config.window.borderless                  =   true;
        config.window.fullscreen                  =   true;
        config.input.keyboard.override_alt_f4     =   true;

        HMONITOR hMonitor =
          MonitorFromWindow ( HWND_DESKTOP,
                                MONITOR_DEFAULTTOPRIMARY );

        MONITORINFO mi   = {         };
        mi.cbSize        = sizeof (mi);
        GetMonitorInfo (hMonitor, &mi);

        config.window.res.override.x = mi.rcMonitor.right  - mi.rcMonitor.left;
        config.window.res.override.y = mi.rcMonitor.bottom - mi.rcMonitor.top;
      }
      break;

      case SK_GAME_ID::DragonQuestXI:
        config.apis.d3d9.hook                     = false;
        config.apis.d3d9ex.hook                   = false;
        config.apis.OpenGL.hook                   = false;
        config.apis.Vulkan.hook                   = false;
        config.apis.dxgi.d3d12.hook               = false;
        config.render.framerate.enable_mmcss      = true;
        break;

      case SK_GAME_ID::AssassinsCreed_Odyssey:
        config.apis.d3d9.hook                     = false;
        config.apis.d3d9ex.hook                   = false;
        config.apis.OpenGL.hook                   = false;
        config.apis.Vulkan.hook                   = false;
        config.render.framerate.enable_mmcss      = true;
        config.textures.cache.residency_managemnt = false;
        config.render.framerate.buffer_count      = 3;
        config.render.framerate.pre_render_limit  = 4;
        config.textures.cache.max_size            = 5120;
        break;

      case SK_GAME_ID::AssassinsCreed_Valhalla:
        config.apis.d3d9.hook                    = false;
        config.apis.d3d9ex.hook                  = false;
        config.apis.OpenGL.hook                  = false;
        config.apis.Vulkan.hook                  = false;
        config.apis.dxgi.d3d11.hook              = false;
        config.apis.dxgi.d3d12.hook              =  true;
        config.render.framerate.flip_discard     =  true;
        config.render.framerate.swapchain_wait   =     1;
        config.render.framerate.buffer_count     =     6;
        config.render.framerate.pre_render_limit =     6;
        config.render.framerate.present_interval =     1;
        config.render.framerate.sleepless_render =  true;
        config.render.framerate.sleepless_window =  true;
        break;

      case SK_GAME_ID::Shenmue:
        config.textures.d3d11.generate_mips       = true;
        config.textures.d3d11.uncompressed_mips   = true;
        config.textures.d3d11.cache_gen_mips      = true;
        config.render.framerate.target_fps        = 30.0f;
        break;

      case SK_GAME_ID::CallOfCthulhu:
      {
        SK_D3D11_DeclHUDShader (0x28c1a0d6, ID3D11PixelShader);
      } break;

      case SK_GAME_ID::JustCause3:
      {
        // This game has multiple windows, we can't hook the wndproc
        config.window.dont_hook_wndproc   = true;
        //config.system.display_debug_out = true;
        config.steam.force_load_steamapi  = true;
        config.steam.auto_inject          = true;
        config.steam.auto_pump_callbacks  = true;
        config.steam.silent               = true;
      } break;

      case SK_GAME_ID::Tales_of_Vesperia:
      {
        //config.render.framerate.limiter_tolerance
        //                                        = 4.25f;
        config.window.treat_fg_as_active        = true;
        config.input.ui.use_hw_cursor           = false;
        SK_ImGui_Cursor.prefs.no_warp.ui_open   = false;
        SK_ImGui_Cursor.prefs.no_warp.visible   = false;
        config.textures.d3d11.uncompressed_mips = true;
        config.textures.d3d11.cache_gen_mips    = true;
      } break;

      case SK_GAME_ID::MonsterHunterWorld:
      {
        config.window.dont_hook_wndproc = true;
        config.steam.silent             = true;
      } break;

      case SK_GAME_ID::Cyberpunk2077:
      {
        extern void SK_CP2077_InitPlugin (void);
                    SK_CP2077_InitPlugin (    );
      } break;

      case SK_GAME_ID::AtelierRyza2:
      {
        config.render.framerate.flip_discard  = true;
        config.render.dxgi.deferred_isolation = true; // For texture mods / HUD tracking

        // Don't accidentally hook the D3D9 device used for video playback
        config.apis.d3d9.hook   = false;
        config.apis.d3d9ex.hook = false;

        SK_D3D11_DeclHUDShader_Vtx (0x1a7704f4);
      } break;

      case SK_GAME_ID::Nioh2:
      {
        // Kill the win32 message box, SK's in-game confirmation is gamepad friendly
        config.input.keyboard.override_alt_f4    = true;
        config.render.framerate.flip_discard     = true;
        config.render.framerate.swapchain_wait   = 1;
        config.render.framerate.sleepless_window = true;
      } break;

      case SK_GAME_ID::OctopathTraveler:
      {
        // It's a Denuvo game, so it may take a while to start...
        //   but the game DOES run its own callbacks so don't worry.
        config.steam.auto_pump_callbacks         = false;
        config.window.borderless                 =  true;
        config.window.fullscreen                 =  true;
        config.window.offset.x.absolute          =    -1;
        config.window.offset.y.absolute          =    -1;
        config.window.center                     = false;

        HMONITOR hMonitor =
          MonitorFromWindow ( HWND_DESKTOP,
                                MONITOR_DEFAULTTOPRIMARY );

        MONITORINFO mi   = {         };
        mi.cbSize        = sizeof (mi);
        GetMonitorInfo (hMonitor, &mi);

        config.window.res.override.x = mi.rcMonitor.right  - mi.rcMonitor.left;
        config.window.res.override.y = mi.rcMonitor.bottom - mi.rcMonitor.top;

        config.render.dxgi.res.min.x = config.window.res.override.x;
        config.render.dxgi.res.min.y = config.window.res.override.y;

        config.render.dxgi.res.max.x = config.window.res.override.x;
        config.render.dxgi.res.max.y = config.window.res.override.y;

      //config.render.framerate.limiter_tolerance =  5.0f;
        config.render.framerate.buffer_count      =     3;
        config.render.framerate.target_fps        =    60;
        config.render.framerate.pre_render_limit  =     4;
        config.render.framerate.sleepless_render  =  true;
        config.render.framerate.sleepless_window  =  true;

        config.window.treat_fg_as_active          =  true;
        config.window.background_render           =  true;
        config.input.ui.use_hw_cursor             = false;
        config.input.cursor.manage                =  true;
        config.input.cursor.timeout               =     0;
        config.input.cursor.keys_activate         = false;

        config.apis.d3d9.hook                     = false;
        config.apis.d3d9ex.hook                   = false;
        config.apis.OpenGL.hook                   = false;
        config.apis.Vulkan.hook                   = false;

        config.threads.enable_file_io_trace       =  true;

        extern void SK_OPT_InitPlugin (void);
                    SK_OPT_InitPlugin (    );
      } break;

      case SK_GAME_ID::HuniePop2:
      {
        // Unity engine game is detected as GL due to no imports
        config.apis.OpenGL.hook                = false;
        config.apis.d3d9.hook                  = false;
        config.textures.cache.ignore_nonmipped = true;
      } break;

      case SK_GAME_ID::NieR_Sqrt_1_5:
      {
        config.textures.d3d11.cache               = false;
        config.apis.OpenGL.hook                   = false;
        config.apis.d3d9.hook                     = false;
        config.apis.d3d9ex.hook                   = false;
        config.window.treat_fg_as_active           = true;
        config.input.cursor.keys_activate         = false;
        config.input.cursor.manage                =  true;
        config.input.cursor.timeout               =  1500;
        config.input.ui.use_hw_cursor             = false;
        config.input.ui.capture_hidden            = true;
        SK_ImGui_Cursor.prefs.no_warp.ui_open     = true;
        config.render.framerate.present_interval  =     1;
        config.render.framerate.sleepless_window  =  true;
        config.render.framerate.buffer_count      =     4;
        config.render.framerate.swapchain_wait    =     1;
        config.render.framerate.pre_render_limit  =     4;
        config.render.framerate.target_fps        =    60;
        config.render.framerate.flip_discard      =  true;
        config.input.gamepad.disable_ps4_hid      =  true;
        config.threads.enable_file_io_trace       =  true;
        config.input.keyboard.catch_alt_f4        =  true;
        config.input.keyboard.override_alt_f4     =  true;
      } break;
#endif
    }
  }

  init = TRUE; }


  //
  // Load Parameters
  //
  compatibility.disable_nv_bloat->load   (config.compatibility.disable_nv_bloat);
  compatibility.rehook_loadlibrary->load (config.compatibility.rehook_loadlibrary);


  if (! apis.last_known->load ((int &)config.apis.last_known))
    config.apis.last_known = SK_RenderAPI::Reserved;


#ifdef _M_IX86
  apis.ddraw.hook->load  (config.apis.ddraw.hook);
  apis.d3d8.hook->load   (config.apis.d3d8.hook);
#endif

  if (! apis.d3d9.hook->load (config.apis.d3d9.hook))
    config.apis.d3d9.hook = true;

  if (! apis.d3d9ex.hook->load (config.apis.d3d9ex.hook))
    config.apis.d3d9ex.hook = true;

  // D3D9Ex cannot exist without D3D9...
  if (config.apis.d3d9ex.hook)
      config.apis.d3d9.hook = true;

  if (! apis.d3d11.hook->load  (config.apis.dxgi.d3d11.hook))
    config.apis.dxgi.d3d11.hook = true;

#ifdef _M_AMD64
  apis.d3d12.hook->load  (config.apis.dxgi.d3d12.hook);

  // We need to enable D3D11 hooking for D3D12 to work reliably
  if (config.apis.dxgi.d3d12.hook)
      config.apis.dxgi.d3d11.hook = true;

#endif

  if (! apis.OpenGL.hook->load (config.apis.OpenGL.hook))
    config.apis.OpenGL.hook = true;

#ifdef _M_AMD64
  apis.Vulkan.hook->load (config.apis.Vulkan.hook);
#endif


  osd.version_banner.duration->load      (config.version_banner.duration);
  osd.state.remember->load               (config.osd.remember_state);

  imgui.scale->load                      (config.imgui.scale);
  imgui.show_eula->load                  (config.imgui.show_eula);
  imgui.show_playtime->load              (config.steam.show_playtime);
  imgui.show_gsync_status->load          (config.apis.NvAPI.gsync_status);
  imgui.mac_style_menu->load             (config.imgui.use_mac_style_menu);
  imgui.show_input_apis->load            (config.imgui.show_input_apis);

  imgui.disable_alpha->load              (config.imgui.render.disable_alpha);
  imgui.antialias_lines->load            (config.imgui.render.antialias_lines);
  imgui.antialias_contours->load         (config.imgui.render.antialias_contours);


  config.imgui.show_eula =
    app_cache_mgr->getLicenseRevision () !=
         SK_LICENSE_REVISION;


  if (((sk::iParameter *)monitoring.io.show)->load     () && config.osd.remember_state)
    config.io.show =     monitoring.io.show->get_value ();
                         monitoring.io.interval->load  (config.io.interval);

  monitoring.fps.show->load      (config.fps.show);
  monitoring.fps.frametime->load (config.fps.frametime);
  monitoring.fps.advanced->load  (config.fps.advanced);

  if (((sk::iParameter *)monitoring.memory.show)->load     () && config.osd.remember_state)
       config.mem.show = monitoring.memory.show->get_value ();
  mem_reserve->load (config.mem.reserve);

  if (((sk::iParameter *)monitoring.cpu.show)->load     () && config.osd.remember_state)
    config.cpu.show =    monitoring.cpu.show->get_value ();
                         monitoring.cpu.interval->load  (config.cpu.interval);
                         monitoring.cpu.simple->load    (config.cpu.simple);

  monitoring.gpu.show->load           (config.gpu.show);
  monitoring.gpu.print_slowdown->load (config.gpu.print_slowdown);
  monitoring.gpu.interval->load       (config.gpu.interval);

  if (((sk::iParameter *)monitoring.disk.show)->load     () && config.osd.remember_state)
    config.disk.show =   monitoring.disk.show->get_value ();
                         monitoring.disk.interval->load  (config.disk.interval);
                         monitoring.disk.type->load      (config.disk.type);

  monitoring.pagefile.interval->load (config.pagefile.interval);

  monitoring.time.show->load (config.time.show);
  monitoring.SLI.show->load  (config.sli.show);

  if (nvidia.api.disable->load (config.apis.NvAPI.enable))
     config.apis.NvAPI.enable = (! nvidia.api.disable->get_value ());

  nvidia.api.disable_hdr->load    (config.apis.NvAPI.disable_hdr);
  nvidia.bugs.snuffed_ansel->load (config.nvidia.bugs.snuffed_ansel);
  nvidia.bugs.bypass_ansel->load  (config.nvidia.bugs.bypass_ansel);

  if (amd.adl.disable->load (config.apis.ADL.enable))
     config.apis.ADL.enable = (! amd.adl.disable->get_value ());


  display.force_fullscreen->load            (config.display.force_fullscreen);
  display.force_windowed->load              (config.display.force_windowed);

  render.framerate.target_fps->load         (config.render.framerate.target_fps);
  render.framerate.target_fps_bg->load      (config.render.framerate.target_fps_bg);
//render.framerate.limiter_tolerance->load  (config.render.framerate.limiter_tolerance);
  render.framerate.sleepless_render->load   (config.render.framerate.sleepless_render);
  render.framerate.sleepless_window->load   (config.render.framerate.sleepless_window);
  render.framerate.enable_mmcss->load       (config.render.framerate.enable_mmcss);

//  if (config.render.framerate.limiter_tolerance < 1.0f)
//      config.render.framerate.limiter_tolerance = 2.75f;

  __target_fps    = config.render.framerate.target_fps;
  __target_fps_bg = config.render.framerate.target_fps_bg;

//  render.framerate.control.
//                  render_ahead->load        (config.render.framerate.max_render_ahead);
  render.framerate.override_cpu_count->load (config.render.framerate.override_num_cpus);


/////// Range-restrict this to prevent the user from destroying performance
///if (config.render.framerate.limiter_tolerance < 1.25f)
///    config.render.framerate.limiter_tolerance = 1.25f;


  render.osd.draw_in_vidcap->load           (config.render.osd. draw_in_vidcap);

  if (render.osd.hdr_luminance->load        (config.render.osd.hdr_luminance))
    rb.ui_luminance =                        config.render.osd.hdr_luminance;

  // D3D9/11
  //

  nvidia.sli.compatibility->load            (config.nvidia.sli.compatibility);
  nvidia.sli.mode->load                     (config.nvidia.sli.mode);
  nvidia.sli.num_gpus->load                 (config.nvidia.sli.num_gpus);
  nvidia.sli.override->load                 (config.nvidia.sli.override);

  nvidia.reflex.enable->load                (config.nvidia.sleep.enable);
  nvidia.reflex.low_latency->load           (config.nvidia.sleep.low_latency);
  nvidia.reflex.low_latency_boost->load     (config.nvidia.sleep.low_latency_boost);
  nvidia.reflex.marker_optimization->load   (config.nvidia.sleep.marker_optimization);
  nvidia.reflex.engagement_policy->load     (config.nvidia.sleep.enforcement_site);

  render.framerate.wait_for_vblank->load    (config.render.framerate.wait_for_vblank);
  render.framerate.buffer_count->load       (config.render.framerate.buffer_count);
  render.framerate.prerender_limit->load    (config.render.framerate.pre_render_limit);
  render.framerate.present_interval->load   (config.render.framerate.present_interval);

  if (render.framerate.refresh_rate)
  {
    render.framerate.refresh_rate->load     (config.render.framerate.refresh_rate);
  }

  if (render.framerate.rescan_ratio)
  {
    render.framerate.rescan_ratio->load     (config.render.framerate.rescan_ratio);

    if (! config.render.framerate.rescan_ratio.empty ())
    {
      swscanf (config.render.framerate.rescan_ratio.c_str (), L"%i/%i", (INT*)&config.render.framerate.rescan_.Numerator,
                                                                        (INT*)&config.render.framerate.rescan_.Denom);
    }

    if ( config.render.framerate.rescan_.Numerator != static_cast <UINT> (-1) &&
         config.render.framerate.rescan_.Denom     !=                      0 )
    {
      config.render.framerate.refresh_rate =
        gsl::narrow_cast <float> (config.render.framerate.rescan_.Numerator) /
        gsl::narrow_cast <float> (config.render.framerate.rescan_.Denom);
    }
  }

  render.framerate.enforcement_policy->load (config.render.framerate.enforcement_policy);

  render.d3d9.force_d3d9ex->load        (config.render.d3d9.force_d3d9ex);
  render.d3d9.impure->load              (config.render.d3d9.force_impure);
  render.d3d9.enable_texture_mods->load (config.textures.d3d9_mod);
  texture.d3d9.clamp_lod_bias->load     (config.textures.clamp_lod_bias);


  // DXGI
  //
  render.framerate.max_delta_time->load   (config.render.framerate.max_delta_time);


  if (render.framerate.flip_discard->load (config.render.framerate.flip_discard) && config.render.framerate.flip_discard)
  {
    config.render.framerate.disable_flip = false;

    if (render.framerate.allow_dwm_tearing->load (config.render.dxgi.allow_tearing))
    {
      //if (config.render.dxgi.allow_tearing) config.render.framerate.flip_discard = true;
    }
  }

  render.framerate.drop_late_frames->load (config.render.framerate.drop_late_flips);
  render.framerate.auto_low_latency->load (config.render.framerate.auto_low_latency);

  if (render.framerate.disable_flip_model->load (config.render.framerate.disable_flip))
  {
    if (config.render.framerate.disable_flip)
    {
      if (config.render.framerate.flip_discard)
        SK_ImGui_Warning (L"Conflicting Flip Model Overrides Detected (Disable and Force)");

      config.render.framerate.flip_discard = false;
      config.render.dxgi.allow_tearing     = false;
    }
  }

  render.dxgi.adapter_override->load (config.render.dxgi.adapter_override);

  if (((sk::iParameter *)render.dxgi.max_res)->load ())
  {
    swscanf ( render.dxgi.max_res->get_value_str ().c_str (),
                L"%ux%u",
                &config.render.dxgi.res.max.x,
                  &config.render.dxgi.res.max.y );
  }
  if (((sk::iParameter *)render.dxgi.min_res)->load ())
  {
    swscanf ( render.dxgi.min_res->get_value_str ().c_str (),
                L"%ux%u",
                &config.render.dxgi.res.min.x,
                  &config.render.dxgi.res.min.y );
  }

  //
  // MUST establish sanity
  //
  //  * If both limits exist, then ( max.x >= min.x  &&  max.y >= min.y )
  //
  bool sane = true;

  if ( config.render.dxgi.res.max.x != 0 &&
       config.render.dxgi.res.min.x >
       config.render.dxgi.res.max.x ) sane = false;
  if ( config.render.dxgi.res.min.x != 0 &&
       config.render.dxgi.res.max.x <
       config.render.dxgi.res.min.x ) sane = false;

  if ( config.render.dxgi.res.max.y != 0 &&
       config.render.dxgi.res.min.y >
       config.render.dxgi.res.max.y ) sane = false;
  if ( config.render.dxgi.res.min.y != 0 &&
       config.render.dxgi.res.max.y <
       config.render.dxgi.res.min.y ) sane = false;

  // Discard the lower-bound and hope for the best
  if (! sane) { config.render.dxgi.res.min.x = 0;
                config.render.dxgi.res.min.y = 0;

    // Correct handling involves actually querying supported resolutions
    //   from the display device, but that's not practical at the time
    //     the config file is parsed.
  };



  if (((sk::iParameter *)render.dxgi.scaling_mode)->load ())
  {
    std::wstring&& mode =
      render.dxgi.scaling_mode->get_value_str ();

    if (! _wcsicmp (mode.c_str (), L"Unspecified"))
    {
      config.render.dxgi.scaling_mode =
        DXGI_MODE_SCALING_UNSPECIFIED;
    }

    else if (! _wcsicmp (mode.c_str (), L"Centered"))
    {
      config.render.dxgi.scaling_mode =
        DXGI_MODE_SCALING_CENTERED;
    }

    else if (! _wcsicmp (mode.c_str (), L"Stretched"))
    {
      config.render.dxgi.scaling_mode =
        DXGI_MODE_SCALING_STRETCHED;
    }
  }

  if (((sk::iParameter *)render.dxgi.scanline_order)->load ())
  {
    std::wstring&& order =
      render.dxgi.scanline_order->get_value_str ();

    if (! _wcsicmp (order.c_str (), L"Unspecified"))
    {
      config.render.dxgi.scanline_order =
        DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    }

    else if (! _wcsicmp (order.c_str (), L"Progressive"))
    {
      config.render.dxgi.scanline_order =
        DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
    }

    else if (! _wcsicmp (order.c_str (), L"LowerFieldFirst"))
    {
      config.render.dxgi.scanline_order =
        DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST;
    }

    else if (! _wcsicmp (order.c_str (), L"UpperFieldFirst"))
    {
      config.render.dxgi.scanline_order =
        DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST;
    }

    // If a user specifies Interlaced, default to Lower Field First
    else if (! _wcsicmp (order.c_str (), L"Interlaced"))
    {
      config.render.dxgi.scanline_order =
        DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST;
    }
  }

  render.dxgi.debug_layer->load (config.render.dxgi.debug_layer);

  if (((sk::iParameter *)render.dxgi.exception_mode)->load ())
  {
    std::wstring&& except_mode =
      render.dxgi.exception_mode->get_value_str ();

    if (! _wcsicmp (except_mode.c_str (), L"Raise"))
    {
      #define D3D11_RAISE_FLAG_DRIVER_INTERNAL_ERROR 1
      config.render.dxgi.exception_mode = D3D11_RAISE_FLAG_DRIVER_INTERNAL_ERROR;
    }

    else if (! _wcsicmp (except_mode.c_str (), L"Ignore"))
    {
      config.render.dxgi.exception_mode = 0;
    }
    else
      config.render.dxgi.exception_mode = -1;
  }

  render.dxgi.test_present->load        (config.render.dxgi.test_present);
  render.dxgi.swapchain_wait->load      (config.render.framerate.swapchain_wait);

  render.dxgi.safe_fullscreen->load     (config.render.dxgi.safe_fullscreen);

  render.dxgi.enhanced_depth->load      (config.render.dxgi.enhanced_depth);
  render.dxgi.deferred_isolation->load  (config.render.dxgi.deferred_isolation);
  render.dxgi.skip_present_test->load   (config.render.dxgi.present_test_skip);
  render.dxgi.msaa_samples->load        (config.render.dxgi.msaa_samples);
  render.dxgi.srgb_behavior->load       (config.render.dxgi.srgb_behavior);
  render.dxgi.low_spec_mode->load       (config.render.dxgi.low_spec_mode);
  render.dxgi.hide_hdr_support->load    (config.render.dxgi.hide_hdr_support);

  texture.d3d11.cache->load             (config.textures.d3d11.cache);
  texture.d3d11.precise_hash->load      (config.textures.d3d11.precise_hash);
  texture.d3d11.inject->load            (config.textures.d3d11.inject);
        texture.res_root->load          (config.textures.d3d11.res_root);

  texture.d3d11.injection_keeps_format->
                                   load (config.textures.d3d11.injection_keeps_fmt);
             texture.dump_on_load->load (config.textures.d3d11.dump);
             texture.dump_on_load->load (config.textures.dump_on_load);

  texture.d3d11.gen_mips->load          (config.textures.d3d11.generate_mips);

  texture.cache.max_entries->load       (config.textures.cache.max_entries);
  texture.cache.min_entries->load       (config.textures.cache.min_entries);
  texture.cache.max_evict->load         (config.textures.cache.max_evict);
  texture.cache.min_evict->load         (config.textures.cache.min_evict);
  texture.cache.max_size->load          (config.textures.cache.max_size);
  texture.cache.min_size->load          (config.textures.cache.min_size);
  texture.cache.ignore_non_mipped->load (config.textures.cache.ignore_nonmipped);
  texture.cache.allow_staging->load     (config.textures.cache.allow_staging);
  texture.cache.allow_unsafe_refs->load (config.textures.cache.allow_unsafe_refs);
  texture.cache.manage_residency->load  (config.textures.cache.residency_managemnt);

  if (config.render.dxgi.adapter_override != -1)
    SK_DXGI_SetPreferredAdapter (config.render.dxgi.adapter_override);

  input.keyboard.catch_alt_f4->load     (config.input.keyboard.catch_alt_f4);
  input.keyboard.bypass_alt_f4->load    (config.input.keyboard.override_alt_f4);
  input.keyboard.disabled_to_game->load (config.input.keyboard.disabled_to_game);

  input.mouse.disabled_to_game->load    (config.input.mouse.disabled_to_game);

  input.cursor.manage->load             (config.input.cursor.manage);
  input.cursor.keys_activate->load      (config.input.cursor.keys_activate);

                            float fTimeout;
  if (input.cursor.timeout->load (fTimeout))
    config.input.cursor.timeout = (int)(1000.0 * fTimeout);

  input.cursor.ui_capture->load         (config.input.ui.capture);
  input.cursor.hw_cursor->load          (config.input.ui.use_hw_cursor);
  input.cursor.no_warp_ui->load         (SK_ImGui_Cursor.prefs.no_warp.ui_open);
  input.cursor.no_warp_visible->load    (SK_ImGui_Cursor.prefs.no_warp.visible);
  input.cursor.block_invisible->load    (config.input.ui.capture_hidden);
  input.cursor.fix_synaptics->load      (config.input.mouse.fix_synaptics);
  input.cursor.antiwarp_deadzone->load  (config.input.mouse.antiwarp_deadzone);
  input.cursor.use_relative_input->load (config.input.mouse.add_relative_motion);

  input.gamepad.disabled_to_game->load  (config.input.gamepad.disabled_to_game);
  input.gamepad.disable_ps4_hid->load   (config.input.gamepad.disable_ps4_hid);
  input.gamepad.rehook_xinput->load     (config.input.gamepad.rehook_xinput);
  input.gamepad.hook_xinput->load       (config.input.gamepad.hook_xinput);

  // Hidden INI values; they're loaded, but never written
  input.gamepad.hook_dinput8->load      (config.input.gamepad.hook_dinput8);
  input.gamepad.hook_dinput7->load      (config.input.gamepad.hook_dinput7);
  input.gamepad.hook_hid->load          (config.input.gamepad.hook_hid);
  input.gamepad.native_ps4->load        (config.input.gamepad.native_ps4);

  input.gamepad.haptic_ui->load         (config.input.gamepad.haptic_ui);

  int placeholder_mask;

  if (input.gamepad.xinput.placeholders->load (placeholder_mask))
  {
    config.input.gamepad.xinput.placehold [0] = ( placeholder_mask & 0x1 );
    config.input.gamepad.xinput.placehold [1] = ( placeholder_mask & 0x2 );
    config.input.gamepad.xinput.placehold [2] = ( placeholder_mask & 0x4 );
    config.input.gamepad.xinput.placehold [3] = ( placeholder_mask & 0x8 );
  }

  input.gamepad.disable_rumble->load (config.input.gamepad.disable_rumble);


  if (((sk::iParameter *)input.gamepad.xinput.assignment)->load ())
  {
    wchar_t* wszAssign =
      _wcsdup (input.gamepad.xinput.assignment->get_value ().c_str ());

    wchar_t* wszBuf = nullptr;
    wchar_t* wszTok =
      std::wcstok (wszAssign, L",", &wszBuf);

    if (wszTok == nullptr)
    {
      config.input.gamepad.xinput.assignment [0] = 0;
      config.input.gamepad.xinput.assignment [1] = 1;
      config.input.gamepad.xinput.assignment [2] = 2;
      config.input.gamepad.xinput.assignment [3] = 3;
    }

    int idx = 0;

    while (wszTok != nullptr && idx < 4)
    {
      config.input.gamepad.xinput.assignment [idx++] =
        _wtoi (wszTok);

      wszTok =
        std::wcstok (nullptr, L",", &wszBuf);
    }

    free (wszAssign);
  }

  input.gamepad.xinput.ui_slot->load   ((int &)config.input.gamepad.xinput.ui_slot);
  input.gamepad.steam.ui_slot->load    ((int &)config.input.gamepad.steam.ui_slot);


  threads.enable_mem_alloc_trace->load (config.threads.enable_mem_alloc_trace);
  threads.enable_file_io_trace->load   (config.threads.enable_file_io_trace);

  window.borderless->load              (config.window.borderless);

  window.center->load                  (config.window.center);
  window.background_render->load       (config.window.background_render);
  window.background_mute->load         (config.window.background_mute);

  std::wstring offset;

  if (window.offset.x->load (offset))
  {
    if (offset.find (L'%') != std::wstring::npos)
    {
      config.window.offset.x.absolute = 0;
      swscanf (offset.c_str (), L"%f%%", &config.window.offset.x.percent);
      config.window.offset.x.percent /= 100.0f;
    }

    else
    {
      config.window.offset.x.percent = 0.0f;
      swscanf (offset.c_str (), L"%i", &config.window.offset.x.absolute);
    }
  }

  if (window.offset.y->load (offset))
  {
    if (offset.find (L'%') != std::wstring::npos)
    {
      config.window.offset.y.absolute = 0;
      swscanf (offset.c_str (), L"%f%%", &config.window.offset.y.percent);
      config.window.offset.y.percent /= 100.0f;
    }

    else
    {
      config.window.offset.y.percent = 0.0f;
      swscanf (offset.c_str (), L"%i", &config.window.offset.y.absolute);
    }
  }

  window.confine_cursor->load      (config.window.confine_cursor);
  window.unconfine_cursor->load    (config.window.unconfine_cursor);
  window.persistent_drag->load     (config.window.persistent_drag);
  window.fullscreen->load          (config.window.fullscreen);
  window.fix_mouse_coords->load    (config.window.res.override.fix_mouse);
  window.always_on_top->load       (config.window.always_on_top);
  window.disable_screensaver->load (config.window.disable_screensaver);
  window.dont_hook_wndproc->load   (config.window.dont_hook_wndproc);

  if (((sk::iParameter *)window.override)->load ())
  {
    swscanf ( window.override->get_value_str ().c_str (),
                L"%ux%u",
                &config.window.res.override.x,
                  &config.window.res.override.y );
  }


  reverse_engineering.file.trace_reads->load  (config.file_io.trace_reads);
  reverse_engineering.file.trace_writes->load (config.file_io.trace_writes);

  if (config.file_io.trace_reads)
  {
    extern iSK_Logger *read_log;

    if (read_log == nullptr)
        read_log = SK_CreateLog (LR"(logs\file_reads.log)");
  }

  if (config.file_io.trace_writes)
  {
    extern iSK_Logger *write_log;

    if (write_log == nullptr)
        write_log = SK_CreateLog (LR"(logs\file_writes.log)");
  }

  if (((sk::iParameter *)&reverse_engineering.file.ignore_reads)->load ())
  {
    std::unique_ptr <wchar_t> wszCSV (
      _wcsdup (reverse_engineering.file.ignore_reads.get_value ().c_str ())
    );

    wchar_t* wszBuf = nullptr;
    wchar_t* wszTok =
      std::wcstok (wszCSV.get (), L",", &wszBuf);

    while (wszTok != nullptr)
    {
      if (*wszTok != L'>')
        config.file_io.ignore_reads.single_file.insert   (           wszTok);
      else
        config.file_io.ignore_reads.entire_thread.insert (CharNextW (wszTok));

      wszTok =
        std::wcstok (nullptr, L",", &wszBuf);
    }
  }

  if (((sk::iParameter *)&reverse_engineering.file.ignore_writes)->load ())
  {
    std::unique_ptr <wchar_t> wszCSV (
      _wcsdup (reverse_engineering.file.ignore_writes.get_value ().c_str ())
    );

    wchar_t* wszBuf = nullptr;
    wchar_t* wszTok =
      std::wcstok (wszCSV.get (), L",", &wszBuf);

    while (wszTok != nullptr)
    {
      if (*wszTok != L'>')
        config.file_io.ignore_writes.single_file.insert   (           wszTok);
      else
        config.file_io.ignore_writes.entire_thread.insert (CharNextW (wszTok));

      wszTok =
        std::wcstok (nullptr, L",", &wszBuf);
    }
  }


  std::wstring tmp;
  cpu.power_scheme_guid->load (tmp);

  if (! tmp.empty ())
  {
    CLSIDFromString  (tmp.c_str (), &config.cpu.power_scheme_guid);
  }

  dpi.disable->load                 (config.dpi.disable_scaling);
  dpi.per_monitor_aware->load       (config.dpi.per_monitor.aware);
  dpi.per_monitor_all_threads->load (config.dpi.per_monitor.aware_on_all_threads);

  steam.achievements.play_sound->load         (config.steam.achievements.play_sound);
  steam.achievements.sound_file->load         (config.steam.achievements.sound_file);
  steam.achievements.take_screenshot->load    (config.steam.achievements.take_screenshot);
  steam.achievements.fetch_friend_stats->load (config.steam.achievements.pull_friend_stats);
  steam.achievements.popup.animate->load      (config.steam.achievements.popup.animate);
  steam.achievements.popup.show_title->load   (config.steam.achievements.popup.show_title);

  if (((sk::iParameter *)steam.achievements.popup.origin)->load ())
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

  steam.achievements.popup.inset->load    (config.steam.achievements.popup.inset);
  steam.achievements.popup.duration->load (config.steam.achievements.popup.duration);

  if (config.steam.achievements.popup.duration == 0)
  {
    config.steam.achievements.popup.show        = false;
    config.steam.achievements.pull_friend_stats = false;
    config.steam.achievements.pull_global_stats = false;
  }


  if (((sk::iParameter *)steam.cloud.blacklist)->load ())
  {
    std::unique_ptr <wchar_t> wszCSV (
        _wcsdup (steam.cloud.blacklist->get_value ().c_str ())
    );

    wchar_t* wszBuf = nullptr;
    wchar_t* wszTok =
      std::wcstok (wszCSV.get (), L",", &wszBuf);

    while (wszTok != nullptr)
    {
      config.steam.cloud.blacklist.emplace (SK_WideCharToUTF8 (wszTok));

      wszTok =
        std::wcstok (nullptr, L",", &wszBuf);
    }
  }


  steam.log.silent->load                      (config.steam.silent);
  steam.drm.spoof_BLoggedOn->load             (config.steam.spoof_BLoggedOn);

  // We may already know the AppID before loading the game's config.
  if (config.steam.appid == 0)
    steam.system.appid->load                  (config.steam.appid);

  if (config.steam.appid != 0)
  {
    SetEnvironmentVariableA ( "SteamGameId",
             SK_FormatString ("%lu", config.steam.appid).c_str ()
                            );
  }

  steam.system.init_delay->load               (config.steam.init_delay);
  steam.system.auto_pump->load                (config.steam.auto_pump_callbacks);
  steam.system.block_stat_callback->load      (config.steam.block_stat_callback);
  steam.system.filter_stat_callbacks->load    (config.steam.filter_stat_callback);
  steam.system.load_early->load               (config.steam.preload_client);
  steam.system.early_overlay->load            (config.steam.preload_overlay);
  steam.system.force_load->load               (config.steam.force_load_steamapi);
  steam.system.auto_inject->load              (config.steam.auto_inject);
  steam.system.reuse_overlay_pause->load      (config.steam.reuse_overlay_pause);

  int                                 throttle = -1;
  if (steam.callbacks.throttle->load (throttle))
  {
    InterlockedExchange ( &SK_SteamAPI_CallbackRateLimit,
                                      throttle );
  }

  steam.overlay.hdr_luminance->load           (config.steam.overlay_hdr_luminance);
  steam.screenshots.smart_capture->load       (config.steam.screenshots.enable_hook);
  uplay.overlay.hdr_luminance->load           (config.uplay.overlay_luminance);
  rtss.overlay.hdr_luminance->load            (config.rtss.overlay_luminance);
  discord.overlay.hdr_luminance->load         (config.discord.overlay_luminance);

  screenshots.include_osd_default->load       (config.screenshots.show_osd_by_default);
  screenshots.keep_png_copy->load             (config.screenshots.png_compress);
  screenshots.play_sound->load                (config.screenshots.play_sound);
  screenshots.copy_to_clipboard->load         (config.screenshots.copy_to_clipboard);

  LoadKeybind (&config.render.keys.hud_toggle);
  LoadKeybind (&config.screenshots.game_hud_free_keybind);
  LoadKeybind (&config.screenshots.sk_osd_free_keybind);
  LoadKeybind (&config.screenshots.sk_osd_insertion_keybind);


  if (SK_GetCurrentGameID () != SK_GAME_ID::MonsterHunterWorld)
  {
    // Setup sane initial values
    config.steam.dll_path = SK_RunLHIfBitness ( 64, L"steam_api64.dll",
                                                    L"steam_api.dll" );
  }

  steam.system.dll_path->load                 (config.steam.dll_path);


  bool global_override = false;

  if (steam_ini->contains_section (L"Steam.Social"))
  {
    if (steam_ini->get_section (L"Steam.Social").contains_key (L"OnlineStatus"))
    {
      swscanf ( steam_ini->get_section (L"Steam.Social").
                           get_value   (L"OnlineStatus").c_str (),
                  L"%d", &config.steam.online_status );
      global_override = true;
    }
  }

  // If no global value is set, this can be established per-game.
  if (! global_override)
    steam.social.online_status->load (config.steam.online_status);


  if (((sk::iParameter *)steam.system.notify_corner)->load ())
  {
    config.steam.notify_corner =
      SK_Steam_PopupOriginWStrToEnum (
        steam.system.notify_corner->get_value ().c_str ()
    );
  }

  osd.show->load           (config.osd.show);

  osd.text.red->load       (config.osd.red);
  osd.text.green->load     (config.osd.green);
  osd.text.blue->load      (config.osd.blue);

  osd.viewport.pos_x->load (config.osd.pos_x);
  osd.viewport.pos_y->load (config.osd.pos_y);
  osd.viewport.scale->load (config.osd.scale);


  silent->load            (config.system.silent);
  trace_libraries->load   (config.system.trace_load_library);
  strict_compliance->load (config.system.strict_compliance);
  log_level->load         (config.system.log_level);
  prefer_fahrenheit->load (config.system.prefer_fahrenheit);
  ignore_rtss_delay->load (config.system.ignore_rtss_delay);
  handle_crashes->load    (config.system.handle_crashes);
  debug_wait->load        (config.system.wait_for_debugger);
  debug_output->load      (config.system.display_debug_out);
  game_output->load       (config.system.game_output);
  enable_cegui->load      (config.cegui.enable);
  safe_cegui->load        (config.cegui.safe_init);
  init_delay->load        (config.system.global_inject_delay);
  version->load           (config.system.version);

  SK_RunOnce (config.cegui.orig_enable = config.cegui.enable);




  ///// Disable Smart Screenshot capture if address space is constrained
  ///if (! SK_PE32_IsLargeAddressAware ())
  ///{
  ///  if (config.steam.screenshots.enable_hook)
  ///  {   config.steam.screenshots.enable_hook = false;
  ///    SK_ImGui_Warning ( L"Executable is not Large Address Aware, disabling Smart Screenshots." );
  ///  }
  ///}


  void
  WINAPI
  SK_D3D11_SetResourceRoot (const wchar_t* root);
  SK_D3D11_SetResourceRoot (config.textures.d3d11.res_root.c_str ());


  //
  // EMERGENCY OVERRIDES
  //
  config.input.ui.use_raw_input = false;



  config.imgui.font.default_font.file = "arial.ttf";
  config.imgui.font.default_font.size = 18.0f;

  config.imgui.font.japanese.file     = "msgothic.ttc";
  config.imgui.font.japanese.size     = 18.0f;

  config.imgui.font.cyrillic.file     = "arial.ttf";
  config.imgui.font.cyrillic.size     = 18.0f;

  config.imgui.font.korean.file       = "malgun.ttf";
  config.imgui.font.korean.size       = 18.0f;

  config.imgui.font.chinese.file      = "msyh.ttc";
  config.imgui.font.chinese.size      = 18.0f;



  static bool scanned = false;

  if ((! scanned) && (! config.window.res.override.isZero ()))
  {
    scanned = true;

    if (SK_GetCurrentGameID () != SK_GAME_ID::UNKNOWN_GAME)
    {
      switch (SK_GetCurrentGameID ())
      {
      //case SK_GAME_ID::GalGunReturns:
        case SK_GAME_ID::GalGun_Double_Peace:
        case SK_GAME_ID::DuckTalesRemastered:
        {
          SK_Thread_Create ([](LPVOID) ->
          DWORD
          {
            void
            SK_ResHack_PatchGame (uint32_t w, uint32_t h);
            SK_ResHack_PatchGame (1920, 1080);
            SK_Thread_CloseSelf  ();

            return 0;
          });
        } break;


        case SK_GAME_ID::YS_Seven:
        {
          SK_Thread_Create ([] (LPVOID) ->
          DWORD
          {
            void
            SK_ResHack_PatchGame2 (uint32_t w, uint32_t h);
            SK_ResHack_PatchGame2 (1920, 1080);
            SK_Thread_CloseSelf   ();

            return 0;
          });
        } break;


        case SK_GAME_ID::Ys_Eight:
        {
          //void
          //SK_ResHack_PatchGame3 (uint32_t w, uint32_t h);
          //
          //SK_ResHack_PatchGame3 (3200, 1800);
          //SK_ResHack_PatchGame3 (1920, 1080);
          //SK_ResHack_PatchGame3 (3840, 2160);
        } break;
      }
    }
  }

  // The name of this config file is different in 0.10.x, but want to load existing values
  //   if the user has any; that involves renaming the file after loading it.
  if (! migrate_steam_config.empty ())
    steam_ini->rename (migrate_steam_config.c_str ());


  // Config opted-in to debugger wait
  if (config.system.wait_for_debugger)
  {
    if (     ! SK_IsDebuggerPresent ())
    { while (! SK_IsDebuggerPresent ())
               SK_SleepEx (100, FALSE);

      __debugbreak ();
    }
  }

  return (! empty);
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

        uint32_t* pOut     = nullptr;
  const void*     pPattern = &res_mod.pattern;

  for (int i = 0 ; i < 3; i++)
  {
    pOut =
    pOut != nullptr ?
      static_cast <uint32_t *> (
        SK_ScanAlignedEx ( pPattern, 8, nullptr, pOut, 8 )
      )             : pOut;


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
      dll_log->Log ( L"[GalGunHACK] ** %lu Resolution Replacements Made  ==>  "
                                          L"( %lux%lu --> %lux%lu )",
                       replacements,
                         width, height,
                           res_mod.replacement.w, res_mod.replacement.h );
      break;
    }
  }
}


void
SK_ResHack_PatchGame3 ( uint32_t width,
                        uint32_t height )
{
  static unsigned int replacements = 0;

        char orig [] = "\xC7\x05\x00\x00\x00\x00"
                       "\x00\x00\x00\x00"
                       "\xC7\x05\x00\x00\x00\x00"
                       "\x00\x00\x00\x00";

  const char* mask = "\xFF\xFF\x00\x00\x00\x00"
                     "\xFF\xFF\xFF\xFF"
                     "\xFF\xFF\x00\x00\x00\x00"
                     "\xFF\xFF\xFF\xFF";

  *((uint32_t *)(uint8_t*)(orig +  6)) = width;
  *((uint32_t *)(uint8_t*)(orig + 16)) = height;

  uint8_t* pOut = nullptr;

  for (int i = 0 ; i < 5; i++)
  {
    pOut =
    pOut != nullptr           ?
      static_cast <uint8_t *> (
        SK_ScanAlignedEx (orig, 20, mask, pOut, 1)
      )                       : pOut;

    if (pOut != nullptr)
    {
      uint8_t data [20] = { };

      memcpy (data, pOut, 20);

      *((uint32_t *)(uint8_t *)(data +  6)) = (uint32_t)(config.window.res.override.x);
      *((uint32_t *)(uint8_t *)(data + 16)) = (uint32_t)(config.window.res.override.y);

      if ( SK_InjectMemory ( pOut,
                               data,
                                 20,
                                   PAGE_EXECUTE_READWRITE )
         )
      {
        ++replacements;
      }

      pOut += 20;
    }

    if (pOut == nullptr)
    {
      dll_log->Log ( L"[Resolution] ** %lu Resolution Replacements Made  ==>  "
                                       L"( %lux%lu --> %lux%lu )",
                       replacements,
                         width, height,
                           config.window.res.override.x, config.window.res.override.y );
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

  uint32_t *pOut = nullptr;

  for (int i = 0 ; i < 5; i++)
  {
    pOut =
    pOut != nullptr            ?
      static_cast <uint32_t *> (
        SK_ScanAlignedEx (orig, 8, nullptr, pOut, 4)
      )                        : pOut;

    if (pOut != nullptr)
    {
      struct {
        uint32_t w = gsl::narrow_cast <uint32_t> (config.window.res.override.x),
                 h = gsl::narrow_cast <uint32_t> (config.window.res.override.y);
      } out_data = { };


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
      dll_log->Log ( L"[Resolution] ** %lu Resolution Replacements Made  ==>  "
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
  if (name.empty ())
  {
    if (SK_IsInjected ())
      name = L"SpecialK";
    else
      name = SK_GetBackend ();
  }


  wchar_t wszFullName [ MAX_PATH + 2 ] = { };

  if ( name.find (L'/' ) == std::wstring::npos &&
       name.find (L'\\') == std::wstring::npos )
    lstrcatW (wszFullName, SK_GetConfigPath ());

  lstrcatW (wszFullName,   name.c_str ());

  if (name.find (L".ini") == std::wstring::npos)
    lstrcatW (wszFullName, L".ini");


  return (DeleteFileW (wszFullName) != FALSE);
}

void
SK_SaveConfig ( std::wstring name,
                bool         close_config )
{
  if (name.empty ())
  {
    if (SK_IsInjected ())
      name = L"SpecialK";
    else
      name = SK_GetBackend ();
  }

  //
  // Shutting down before initialization would be damn near fatal if we didn't catch this! :)
  //
  if ( dll_ini         == nullptr ||
       osd_ini         == nullptr    )
    return;

  auto& rb =
    SK_GetCurrentRenderBackend ();

  compatibility.disable_nv_bloat->store       (config.compatibility.disable_nv_bloat);
  compatibility.rehook_loadlibrary->store     (config.compatibility.rehook_loadlibrary);

  monitoring.memory.show->set_value           (config.mem.show);
//mem_reserve->store                          (config.mem.reserve);

  monitoring.fps.show->store                  (config.fps.show);
  monitoring.fps.advanced->store              (config.fps.advanced);
  monitoring.fps.frametime->store             (config.fps.frametime);

  monitoring.io.show->set_value               (config.io.show);
  monitoring.io.interval->store               (config.io.interval);

  monitoring.cpu.show->set_value              (config.cpu.show);
  monitoring.cpu.interval->store              (config.cpu.interval);
  monitoring.cpu.simple->store                (config.cpu.simple);

  monitoring.gpu.show->store                  (config.gpu.show);
  monitoring.gpu.print_slowdown->store        (config.gpu.print_slowdown);
  monitoring.gpu.interval->store              (config.gpu.interval);

  monitoring.disk.show->set_value             (config.disk.show);
  monitoring.disk.interval->store             (config.disk.interval);
  monitoring.disk.type->store                 (config.disk.type);

  monitoring.pagefile.show->set_value         (config.pagefile.show);
  monitoring.pagefile.interval->store         (config.pagefile.interval);

  if (! (nvapi_init && sk::NVAPI::nv_hardware && sk::NVAPI::CountSLIGPUs () > 1))
    config.sli.show = false;

  monitoring.SLI.show->store                  (config.sli.show);
  monitoring.time.show->store                 (config.time.show);

  osd.version_banner.duration->store          (config.version_banner.duration);
  osd.show->store                             (config.osd.show);
  osd.text.red->store                         (config.osd.red);
  osd.text.green->store                       (config.osd.green);
  osd.text.blue->store                        (config.osd.blue);
  osd.viewport.pos_x->store                   (config.osd.pos_x);
  osd.viewport.pos_y->store                   (config.osd.pos_y);
  osd.viewport.scale->store                   (config.osd.scale);
  osd.state.remember->store                   (config.osd.remember_state);

  imgui.scale->store                          (config.imgui.scale);
  imgui.show_eula->store                      (config.imgui.show_eula);
  imgui.show_playtime->store                  (config.steam.show_playtime);
  imgui.show_gsync_status->store              (config.apis.NvAPI.gsync_status);
  imgui.mac_style_menu->store                 (config.imgui.use_mac_style_menu);
  imgui.show_input_apis->store                (config.imgui.show_input_apis);
  imgui.disable_alpha->store                  (config.imgui.render.disable_alpha);
  imgui.antialias_lines->store                (config.imgui.render.antialias_lines);
  imgui.antialias_contours->store             (config.imgui.render.antialias_contours);

  apis.last_known->store                      (static_cast <int> (config.apis.last_known));

#ifdef _M_IX86
  apis.ddraw.hook->store                      (config.apis.ddraw.hook);
  apis.d3d8.hook->store                       (config.apis.d3d8.hook);
#endif
  apis.d3d9.hook->store                       (config.apis.d3d9.hook);
  apis.d3d9ex.hook->store                     (config.apis.d3d9ex.hook);
  apis.d3d11.hook->store                      (config.apis.dxgi.d3d11.hook);
  apis.OpenGL.hook->store                     (config.apis.OpenGL.hook);
#ifdef _M_AMD64
  apis.d3d12.hook->store                      (config.apis.dxgi.d3d12.hook);
  apis.Vulkan.hook->store                     (config.apis.Vulkan.hook);
#endif

  nvidia.api.disable_hdr->store               (config.apis.NvAPI.disable_hdr);
  nvidia.bugs.snuffed_ansel->store            (config.nvidia.bugs.snuffed_ansel);
  nvidia.bugs.bypass_ansel->store             (config.nvidia.bugs.bypass_ansel);

  input.keyboard.catch_alt_f4->store          (config.input.keyboard.catch_alt_f4);
  input.keyboard.bypass_alt_f4->store         (config.input.keyboard.override_alt_f4);
  input.keyboard.disabled_to_game->store      (config.input.keyboard.disabled_to_game);

  input.mouse.disabled_to_game->store         (config.input.mouse.disabled_to_game);

  input.cursor.manage->store                  (config.input.cursor.manage);
  input.cursor.keys_activate->store           (config.input.cursor.keys_activate);
  input.cursor.timeout->store                 (static_cast <float> (config.input.cursor.timeout) / 1000.0f);
  input.cursor.ui_capture->store              (config.input.ui.capture);
  input.cursor.hw_cursor->store               (config.input.ui.use_hw_cursor);
  input.cursor.block_invisible->store         (config.input.ui.capture_hidden);
  input.cursor.no_warp_ui->store              (SK_ImGui_Cursor.prefs.no_warp.ui_open);
  input.cursor.no_warp_visible->store         (SK_ImGui_Cursor.prefs.no_warp.visible);
  input.cursor.fix_synaptics->store           (config.input.mouse.fix_synaptics);
  input.cursor.antiwarp_deadzone->store       (config.input.mouse.antiwarp_deadzone);
  input.cursor.use_relative_input->store      (config.input.mouse.add_relative_motion);

  input.gamepad.disabled_to_game->store       (config.input.gamepad.disabled_to_game);
  input.gamepad.disable_ps4_hid->store        (config.input.gamepad.disable_ps4_hid);
  input.gamepad.rehook_xinput->store          (config.input.gamepad.rehook_xinput);
  input.gamepad.haptic_ui->store              (config.input.gamepad.haptic_ui);

  int placeholder_mask = 0x0;

  placeholder_mask |= (config.input.gamepad.xinput.placehold [0] ? 0x1 : 0x0);
  placeholder_mask |= (config.input.gamepad.xinput.placehold [1] ? 0x2 : 0x0);
  placeholder_mask |= (config.input.gamepad.xinput.placehold [2] ? 0x4 : 0x0);
  placeholder_mask |= (config.input.gamepad.xinput.placehold [3] ? 0x8 : 0x0);

  input.gamepad.xinput.placeholders->store    (placeholder_mask);
  input.gamepad.xinput.ui_slot->store         (config.input.gamepad.xinput.ui_slot);
  input.gamepad.steam.ui_slot->store          (config.input.gamepad.steam.ui_slot);

  std::wstring xinput_assign;

  for (int i = 0; i < 4; i++)
  {
    xinput_assign += std::to_wstring (
      config.input.gamepad.xinput.assignment [i]
    );

    if (i != 3)
      xinput_assign += L",";
  }

  input.gamepad.xinput.assignment->store     (xinput_assign);
  input.gamepad.disable_rumble->store        (config.input.gamepad.disable_rumble);

  threads.enable_mem_alloc_trace->store      (config.threads.enable_mem_alloc_trace);
  threads.enable_file_io_trace->store        (config.threads.enable_file_io_trace);

  window.borderless->store                   (config.window.borderless);
  window.center->store                       (config.window.center);
  window.background_render->store            (config.window.background_render);
  window.background_mute->store              (config.window.background_mute);
  if (config.window.offset.x.absolute != 0)
  {
    wchar_t   wszAbsolute [16] = { };
    swprintf (wszAbsolute, L"%li", config.window.offset.x.absolute);

    window.offset.x->store (wszAbsolute);
  }

  else
  {
      wchar_t wszPercent [16] = { };
    swprintf (wszPercent, L"%08.6f", 100.0 * config.window.offset.x.percent);

    SK_RemoveTrailingDecimalZeros (wszPercent);
    lstrcatW                      (wszPercent, L"%");
    window.offset.x->store        (wszPercent);
  }

  if (config.window.offset.y.absolute != 0)
  {
    wchar_t   wszAbsolute [16] = { };
    swprintf (wszAbsolute, L"%li", config.window.offset.y.absolute);

    window.offset.y->store (wszAbsolute);
  }

  else
  {
      wchar_t wszPercent [16] = { };
    swprintf (wszPercent, L"%08.6f", 100.0 * config.window.offset.y.percent);

    SK_RemoveTrailingDecimalZeros (wszPercent);
    lstrcatW                      (wszPercent, L"%");
    window.offset.y->store        (wszPercent);
  }

  window.confine_cursor->store                (config.window.confine_cursor);
  window.unconfine_cursor->store              (config.window.unconfine_cursor);
  window.persistent_drag->store               (config.window.persistent_drag);
  window.fullscreen->store                    (config.window.fullscreen);
  window.fix_mouse_coords->store              (config.window.res.override.fix_mouse);
  window.always_on_top->store                 (config.window.always_on_top);
  window.disable_screensaver->store           (config.window.disable_screensaver);
  window.dont_hook_wndproc->store             (config.window.dont_hook_wndproc);

  wchar_t wszFormattedRes [64] = { };

  wsprintf ( wszFormattedRes, L"%lux%lu",
               config.window.res.override.x,
                 config.window.res.override.y );

  window.override->store (wszFormattedRes);

  display.force_fullscreen->store             (config.display.force_fullscreen);
  display.force_windowed->store               (config.display.force_windowed);

//if (close_config)
  render.framerate.target_fps->store          (__target_fps);
  render.framerate.target_fps_bg->store       (__target_fps_bg);
  render.framerate.sleepless_render->store    (config.render.framerate.sleepless_render);
  render.framerate.sleepless_window->store    (config.render.framerate.sleepless_window);
  render.framerate.enable_mmcss->store        (config.render.framerate.enable_mmcss);

  render.framerate.override_cpu_count->store  (config.render.framerate.override_num_cpus);
  
  if ( SK_IsInjected () || (SK_GetDLLRole () & DLL_ROLE::DInput8) ||
      (SK_GetDLLRole () & DLL_ROLE::D3D9 || SK_GetDLLRole () & DLL_ROLE::DXGI) )
  {
    render.framerate.wait_for_vblank->store   (config.render.framerate.wait_for_vblank);
    render.framerate.prerender_limit->store   (config.render.framerate.pre_render_limit);
    render.framerate.buffer_count->store      (config.render.framerate.buffer_count);
    render.framerate.present_interval->store  (config.render.framerate.present_interval);

    if (render.framerate.rescan_ratio != nullptr)
    {
      render.framerate.rescan_ratio->store    (config.render.framerate.rescan_ratio);

      if (! config.render.framerate.rescan_ratio.empty ())
      {
        swscanf ( config.render.framerate.rescan_ratio.c_str (), L"%i/%i",
          (INT *)&config.render.framerate.rescan_.Numerator,
          (INT *)&config.render.framerate.rescan_.Denom);
      }

      if ( config.render.framerate.rescan_.Numerator != static_cast <UINT> (-1) &&
           config.render.framerate.rescan_.Denom     !=                      0 )
      {
        config.render.framerate.refresh_rate =
            gsl::narrow_cast <float> (config.render.framerate.rescan_.Numerator) /
            gsl::narrow_cast <float> (config.render.framerate.rescan_.Denom);
      }
    }

    if (render.framerate.refresh_rate != nullptr)
      render.framerate.refresh_rate->store        (config.render.framerate.refresh_rate);

    render.framerate.enforcement_policy->store    (config.render.framerate.enforcement_policy);
    texture.d3d9.clamp_lod_bias->store            (config.textures.clamp_lod_bias);

    // SLI only works in Direct3D
    nvidia.sli.compatibility->store               (config.nvidia.sli.compatibility);
    nvidia.sli.mode->store                        (config.nvidia.sli.mode);
    nvidia.sli.num_gpus->store                    (config.nvidia.sli.num_gpus);
    nvidia.sli.override->store                    (config.nvidia.sli.override);

    render.framerate.auto_low_latency->store      (config.render.framerate.auto_low_latency);

    if (  SK_IsInjected ()                       ||
        ( SK_GetDLLRole () & DLL_ROLE::DInput8 ) ||
        ( SK_GetDLLRole () & DLL_ROLE::DXGI    ) )
    {
      nvidia.reflex.enable->store                 (config.nvidia.sleep.enable);
      nvidia.reflex.low_latency->store            (config.nvidia.sleep.low_latency);
      nvidia.reflex.low_latency_boost->store      (config.nvidia.sleep.low_latency_boost);
      nvidia.reflex.engagement_policy->store      (config.nvidia.sleep.enforcement_site);
      nvidia.reflex.marker_optimization->store    (config.nvidia.sleep.marker_optimization);
      render.framerate.max_delta_time->store      (config.render.framerate.max_delta_time);
      render.framerate.flip_discard->store        (config.render.framerate.flip_discard);
      render.framerate.disable_flip_model->store  (config.render.framerate.disable_flip);
      render.framerate.allow_dwm_tearing->store   (config.render.dxgi.allow_tearing);
      render.framerate.drop_late_frames->store    (config.render.framerate.drop_late_flips);

      texture.d3d11.cache->store                  (config.textures.d3d11.cache);
      texture.d3d11.precise_hash->store           (config.textures.d3d11.precise_hash);
      texture.d3d11.inject->store                 (config.textures.d3d11.inject);
      texture.d3d11.injection_keeps_format->store (config.textures.d3d11.injection_keeps_fmt);
      texture.d3d11.gen_mips->store               (config.textures.d3d11.generate_mips);

      texture.cache.max_entries->store            (config.textures.cache.max_entries);
      texture.cache.min_entries->store            (config.textures.cache.min_entries);
      texture.cache.max_evict->store              (config.textures.cache.max_evict);
      texture.cache.min_evict->store              (config.textures.cache.min_evict);
      texture.cache.max_size->store               (config.textures.cache.max_size);
      texture.cache.min_size->store               (config.textures.cache.min_size);

      texture.cache.ignore_non_mipped->store      (config.textures.cache.ignore_nonmipped);
      texture.cache.allow_staging->store          (config.textures.cache.allow_staging);
      texture.cache.allow_unsafe_refs->store      (config.textures.cache.allow_unsafe_refs);
      texture.cache.manage_residency->store       (config.textures.cache.residency_managemnt);

      wsprintf ( wszFormattedRes, L"%lux%lu",
                   config.render.dxgi.res.max.x,
                     config.render.dxgi.res.max.y );

      render.dxgi.max_res->store (wszFormattedRes);

      wsprintf ( wszFormattedRes, L"%lux%lu",
                   config.render.dxgi.res.min.x,
                     config.render.dxgi.res.min.y );

      render.dxgi.min_res->store (wszFormattedRes);

      render.dxgi.swapchain_wait->store (config.render.framerate.swapchain_wait);

      switch (config.render.dxgi.scaling_mode)
      {
        case DXGI_MODE_SCALING_UNSPECIFIED:
          render.dxgi.scaling_mode->store (L"Unspecified");
          break;
        case DXGI_MODE_SCALING_CENTERED:
          render.dxgi.scaling_mode->store (L"Centered");
          break;
        case DXGI_MODE_SCALING_STRETCHED:
          render.dxgi.scaling_mode->store (L"Stretched");
          break;
        default:
          render.dxgi.scaling_mode->store (L"DontCare");
          break;
      }

      switch (config.render.dxgi.scanline_order)
      {
        case DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED:
          render.dxgi.scanline_order->store (L"Unspecified");
          break;
        case DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE:
          render.dxgi.scanline_order->store (L"Progressive");
          break;
        case DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST:
          render.dxgi.scanline_order->store (L"LowerFieldFirst");
          break;
        case DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST:
          render.dxgi.scanline_order->store (L"UpperFieldFirst");
          break;
        default:
          render.dxgi.scanline_order->store (L"DontCare");
          break;
      }

      switch (config.render.dxgi.exception_mode)
      {
        case D3D11_RAISE_FLAG_DRIVER_INTERNAL_ERROR:
          render.dxgi.exception_mode->store (L"Raise");
          break;
        case 0:
          render.dxgi.exception_mode->store (L"Ignore");
          break;
        default:
          render.dxgi.exception_mode->store (L"DontCare");
          break;
      }

      render.dxgi.debug_layer->store          (config.render.dxgi.debug_layer);
      render.dxgi.safe_fullscreen->store      (config.render.dxgi.safe_fullscreen);
      render.dxgi.enhanced_depth->store       (config.render.dxgi.enhanced_depth);
      render.dxgi.deferred_isolation->store   (config.render.dxgi.deferred_isolation);
      render.dxgi.skip_present_test->store    (config.render.dxgi.present_test_skip);
      render.dxgi.msaa_samples->store         (config.render.dxgi.msaa_samples);
      render.dxgi.srgb_behavior->store        (config.render.dxgi.srgb_behavior);
      render.dxgi.low_spec_mode->store        (config.render.dxgi.low_spec_mode);
      render.dxgi.hide_hdr_support->store     (config.render.dxgi.hide_hdr_support);
    }

    if ( SK_IsInjected () || ( SK_GetDLLRole () & DLL_ROLE::D3D9    ) ||
                             ( SK_GetDLLRole () & DLL_ROLE::DInput8 ) )
    {
      render.d3d9.force_d3d9ex->store         (config.render.d3d9.force_d3d9ex);
      render.d3d9.enable_texture_mods->store  (config.textures.d3d9_mod);
    }
  }

  if (SK_GetFramesDrawn ())
  {
    render.osd.draw_in_vidcap->store            (config.render.osd.draw_in_vidcap);

    config.render.osd.hdr_luminance = rb.ui_luminance;
    render.osd.hdr_luminance->store  (rb.ui_luminance);
  }

  texture.res_root->store                     (config.textures.d3d11.res_root);
  texture.dump_on_load->store                 (config.textures.dump_on_load);


  // Keep this setting hidden (not to be sneaky; but to prevent overwhelming users with
  //   extremely esoteric options -- this is needed for a lot of settings =P)
  if (! ( config.file_io.trace_reads  || config.file_io.trace_writes ||
          ( config.file_io.ignore_reads.entire_thread.size  () +
            config.file_io.ignore_reads.single_file.size    () ) > 0 ||
          ( config.file_io.ignore_writes.entire_thread.size () +
            config.file_io.ignore_writes.single_file.size   () ) > 0
          )
      )
  {
    dll_ini->remove_section (L"FileIO.Trace");
  }

  else
  {
    if (config.file_io.trace_reads)
      reverse_engineering.file.trace_reads->store  (config.file_io.trace_reads);
    if (config.file_io.trace_writes)
      reverse_engineering.file.trace_writes->store (config.file_io.trace_writes);
  }


  static const GUID empty_guid = { };

  if (! IsEqualGUID (config.cpu.power_scheme_guid, empty_guid))
  {
                                                   wchar_t wszGuid [40] = { L'\0' };
    StringFromGUID2 ( config.cpu.power_scheme_guid,        wszGuid, 40);
                             cpu.power_scheme_guid->store (wszGuid);
  }
  else
    dll_ini->get_section (L"CPU.Power").remove_key (L"PowerSchemeGUID");


  dpi.disable->store                           (config.dpi.disable_scaling);
  dpi.per_monitor_aware->store                 (config.dpi.per_monitor.aware);
  dpi.per_monitor_all_threads->store           (config.dpi.per_monitor.aware_on_all_threads);


  steam.achievements.sound_file->store         (config.steam.achievements.sound_file);
  steam.achievements.play_sound->store         (config.steam.achievements.play_sound);
  steam.achievements.take_screenshot->store    (config.steam.achievements.take_screenshot);
  steam.achievements.fetch_friend_stats->store (config.steam.achievements.pull_friend_stats);
  steam.achievements.popup.origin->store       (
    SK_Steam_PopupOriginToWStr (config.steam.achievements.popup.origin)
  );
  steam.achievements.popup.inset->store        (config.steam.achievements.popup.inset);

  if (! config.steam.achievements.popup.show)
  {
    config.steam.achievements.popup.duration = 0;
  }

  steam.achievements.popup.duration->store     (config.steam.achievements.popup.duration);
  steam.achievements.popup.animate->store      (config.steam.achievements.popup.animate);
  steam.achievements.popup.show_title->store   (config.steam.achievements.popup.show_title);

  if (config.steam.appid == 0)
  {
    if (SK::SteamAPI::AppID () != 0 &&
        SK::SteamAPI::AppID () != 1)
    {
      config.steam.appid = SK::SteamAPI::AppID ();
    }
  }

  steam.system.appid->store                    (config.steam.appid);
  steam.system.init_delay->store               (config.steam.init_delay);
  steam.system.auto_pump->store                (config.steam.auto_pump_callbacks);
  steam.system.block_stat_callback->store      (config.steam.block_stat_callback);
  steam.system.filter_stat_callbacks->store    (config.steam.filter_stat_callback);
  steam.system.load_early->store               (config.steam.preload_client);
  steam.system.early_overlay->store            (config.steam.preload_overlay);
  steam.system.force_load->store               (config.steam.force_load_steamapi);
  steam.system.auto_inject->store              (config.steam.auto_inject);
  steam.system.notify_corner->store            (
                    SK_Steam_PopupOriginToWStr (config.steam.notify_corner)
  );
  steam.system.reuse_overlay_pause->store      (config.steam.reuse_overlay_pause);
  steam.system.dll_path->store                 (config.steam.dll_path);

  steam.callbacks.throttle->store              (ReadAcquire (&SK_SteamAPI_CallbackRateLimit));

  steam.social.online_status->store            (config.steam.online_status);

  steam.log.silent->store                      (config.steam.silent);
  steam.drm.spoof_BLoggedOn->store             (config.steam.spoof_BLoggedOn);

  steam.overlay.hdr_luminance->store           (config.steam.overlay_hdr_luminance);

  steam.screenshots.smart_capture->store       (config.steam.screenshots.enable_hook);

  screenshots.include_osd_default->store       (config.screenshots.show_osd_by_default);
  screenshots.keep_png_copy->store             (config.screenshots.png_compress);
  screenshots.play_sound->store                (config.screenshots.play_sound);
  screenshots.copy_to_clipboard->store         (config.screenshots.copy_to_clipboard);

  uplay.overlay.hdr_luminance->store           (config.uplay.overlay_luminance);
  discord.overlay.hdr_luminance->store         (config.discord.overlay_luminance);
  rtss.overlay.hdr_luminance->store            (config.rtss.overlay_luminance);

  silent->store                                (config.system.silent);
  log_level->store                             (config.system.log_level);
  prefer_fahrenheit->store                     (config.system.prefer_fahrenheit);


  nvidia.api.disable->store                    (! config.apis.NvAPI.enable);
  amd.adl.disable->store                       (! config.apis.ADL.enable);

  ignore_rtss_delay->store                     (config.system.ignore_rtss_delay);


  // Don't store this setting at shutdown  (it may have been turned off automatically)
  if (__SK_DLL_Ending == false)
  {
    handle_crashes->store                      (config.system.handle_crashes);
  }

  game_output->store                           (config.system.game_output);

  // Only add this to the INI file if it differs from default
  if (config.system.display_debug_out != debug_output->get_value ())
  {
    debug_output->store                        (config.system.display_debug_out);
  }

  debug_wait->store                            (config.system.wait_for_debugger);
  enable_cegui->store                          (config.cegui.enable);
  safe_cegui->store                            (config.cegui.safe_init);
  trace_libraries->store                       (config.system.trace_load_library);
  strict_compliance->store                     (config.system.strict_compliance);
  init_delay->store                            (config.system.global_inject_delay);
  version->store                               (SK_GetVersionStrW ());

  if (dll_ini != nullptr && (! (nvapi_init && sk::NVAPI::nv_hardware) || (! sk::NVAPI::CountSLIGPUs ())))
    dll_ini->remove_section (L"NVIDIA.SLI");



  wchar_t wszFullName [ MAX_PATH + 2 ] = { };

  lstrcatW (wszFullName, SK_GetConfigPath ());
  lstrcatW (wszFullName,       name.c_str ());
  lstrcatW (wszFullName,             L".ini");


  if (SK_GetFramesDrawn ())
  {
    if (dll_ini != nullptr)
        dll_ini->write ( wszFullName );
  }


  SK_ImGui_Widgets->SaveConfig ();

  if (osd_ini)   osd_ini->write   ( osd_ini->get_filename   () );
  if (steam_ini) steam_ini->write ( steam_ini->get_filename () );
  if (macro_ini) macro_ini->write ( macro_ini->get_filename () );


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

    if (steam_ini != nullptr)
    {
      delete steam_ini;
             steam_ini = nullptr;
    }

    if (macro_ini != nullptr)
    {
      delete macro_ini;
             macro_ini = nullptr;
    }
  }
}

constexpr UINT
SK_MakeKeyMask ( const SHORT vKey,
                 const bool  ctrl,
                 const bool  shift,
                 const bool  alt )
{
  return
    static_cast <UINT> (
      ( vKey | ( (ctrl != 0) <<  9 ) |
               ( (shift!= 0) << 10 ) |
               ( (alt  != 0) << 11 ) )
    );
}

void
SK_KeyMap_StandardizeNames (wchar_t* wszNameToFormalize)
{
  wchar_t*                pwszName = wszNameToFormalize;
              CharUpperW (pwszName);
   pwszName = CharNextW  (pwszName);

  bool lower = true;

  while (*pwszName != L'\0')
  {
    if (lower) CharLowerW (pwszName);
    else       CharUpperW (pwszName);

    lower =
      (! iswspace (*pwszName));

    pwszName =
      CharNextW (pwszName);
  }
}
void
SK_Keybind::update (void)
{
  human_readable.clear ();

  wchar_t* key_name =
    (*virtKeyCodeToHumanKeyName)[(BYTE)(vKey & 0xFF)];

  if (*key_name == L'\0')
  {
    ctrl           = false;
    alt            = false;
    shift          = false;
    masked_code    = 0x0;
    human_readable = L"<Not Bound>";

    return;
  }

  std::queue <std::wstring> words;

  if (ctrl)
    words.emplace (L"Ctrl");

  if (alt)
    words.emplace (L"Alt");

  if (shift)
    words.emplace (L"Shift");

  words.emplace (key_name);

  while (! words.empty ())
  {
    human_readable += words.front ();
    words.pop ();

    if (! words.empty ())
      human_readable += L"+";
  }

  masked_code =
    SK_MakeKeyMask (vKey & 0xFF, ctrl, shift, alt);
}

void
SK_Keybind::parse (void)
{
  vKey = 0x00;

  static bool init = false;

  static auto& humanToVirtual = humanKeyNameToVirtKeyCode.get ();
  static auto& virtualToHuman = virtKeyCodeToHumanKeyName.get ();

  auto _PushVirtualToHuman =
  [&] (BYTE vKey, const wchar_t* wszHumanName) ->
  void
  {
    auto& pair_builder =
      virtualToHuman [vKey];

    wcsncpy_s ( pair_builder, 64,
                wszHumanName, _TRUNCATE );
  };

  auto _PushHumanToVirtual =
  [&] (const wchar_t* wszHumanName, BYTE vKey) ->
  void
  {
    humanToVirtual.emplace (
      hash_string (wszHumanName),
        vKey
    );
  };

  if (! init)
  {
    for (int i = 0; i < 0xFF; i++)
    {
      wchar_t name [64] = { };

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
            ( MapVirtualKey (i, 0) & 0xFFU );
          unsigned short int temp      =  0;

          bool asc = (i <= 32);

          if (! asc && i != VK_DIVIDE)
          {
                                     BYTE buf [256] = { };
             asc = ToAscii ( i, scanCode, buf, &temp, 1 );
          }

          scanCode             <<= 16U;
          scanCode   |= ( 0x1U <<  25U  );

          if (! asc)
            scanCode |= ( 0x1U << 24U   );

          GetKeyNameText ( scanCode,
                             name,
                               32 );

          SK_KeyMap_StandardizeNames (name);
        } break;
      }


      if ( i != VK_CONTROL  && i != VK_MENU     &&
           i != VK_SHIFT    && i != VK_OEM_PLUS && i != VK_OEM_MINUS &&
           i != VK_LSHIFT   && i != VK_RSHIFT   &&
           i != VK_LCONTROL && i != VK_RCONTROL &&
           i != VK_LMENU    && i != VK_RMENU )
      {
        _PushHumanToVirtual (name, gsl::narrow_cast <BYTE> (i));
        _PushVirtualToHuman (      gsl::narrow_cast <BYTE> (i), name);
      }
    }

    _PushHumanToVirtual (L"Plus",        gsl::narrow_cast <BYTE> (VK_OEM_PLUS));
    _PushHumanToVirtual (L"Minus",       gsl::narrow_cast <BYTE> (VK_OEM_MINUS));
    _PushHumanToVirtual (L"Ctrl",        gsl::narrow_cast <BYTE> (VK_CONTROL));
    _PushHumanToVirtual (L"Alt",         gsl::narrow_cast <BYTE> (VK_MENU));
    _PushHumanToVirtual (L"Shift",       gsl::narrow_cast <BYTE> (VK_SHIFT));
    _PushHumanToVirtual (L"Left Shift",  gsl::narrow_cast <BYTE> (VK_LSHIFT));
    _PushHumanToVirtual (L"Right Shift", gsl::narrow_cast <BYTE> (VK_RSHIFT));
    _PushHumanToVirtual (L"Left Alt",    gsl::narrow_cast <BYTE> (VK_LMENU));
    _PushHumanToVirtual (L"Right Alt",   gsl::narrow_cast <BYTE> (VK_RMENU));
    _PushHumanToVirtual (L"Left Ctrl",   gsl::narrow_cast <BYTE> (VK_LCONTROL));
    _PushHumanToVirtual (L"Right Ctrl",  gsl::narrow_cast <BYTE> (VK_RCONTROL));


    _PushVirtualToHuman (gsl::narrow_cast <BYTE> (VK_CONTROL),   L"Ctrl");
    _PushVirtualToHuman (gsl::narrow_cast <BYTE> (VK_MENU),      L"Alt");
    _PushVirtualToHuman (gsl::narrow_cast <BYTE> (VK_SHIFT),     L"Shift");
    _PushVirtualToHuman (gsl::narrow_cast <BYTE> (VK_OEM_PLUS),  L"Plus");
    _PushVirtualToHuman (gsl::narrow_cast <BYTE> (VK_OEM_MINUS), L"Minus");
    _PushVirtualToHuman (gsl::narrow_cast <BYTE> (VK_LSHIFT),    L"Left Shift");
    _PushVirtualToHuman (gsl::narrow_cast <BYTE> (VK_RSHIFT),    L"Right Shift");
    _PushVirtualToHuman (gsl::narrow_cast <BYTE> (VK_LMENU),     L"Left Alt");
    _PushVirtualToHuman (gsl::narrow_cast <BYTE> (VK_RMENU),     L"Right Alt");
    _PushVirtualToHuman (gsl::narrow_cast <BYTE> (VK_LCONTROL),  L"Left Ctrl");
    _PushVirtualToHuman (gsl::narrow_cast <BYTE> (VK_RCONTROL),  L"Right Ctrl");

    init = true;
  }

  wchar_t   wszKeyBind [128] = { };
  lstrcatW (wszKeyBind, human_readable.c_str ());

  wchar_t* wszBuf = nullptr;
  wchar_t* wszTok = std::wcstok (wszKeyBind, L"+", &wszBuf);

  vKey  = 0x0;
  ctrl  = false;
  alt   = false;
  shift = false;

  if (wszTok == nullptr)
  {
    if (*wszKeyBind != L'\0')
    {
      SK_KeyMap_StandardizeNames (wszKeyBind);

      if (*wszKeyBind != L'\0')
      {
        vKey =
          humanToVirtual [hash_string (wszKeyBind)];
      }
    }
  }

  while (wszTok != nullptr)
  {
    SK_KeyMap_StandardizeNames (wszTok);

    if (*wszTok != L'\0')
    {
      BYTE vKey_ =
        humanToVirtual [hash_string (wszTok)];

      if (vKey_ == VK_CONTROL)
        ctrl  = true;
      else if (vKey_ == VK_SHIFT)
        shift = true;
      else if (vKey_ == VK_MENU)
        alt   = true;
      else
        vKey = vKey_;
    }

    wszTok =
      std::wcstok (nullptr, L"+", &wszBuf);
  }

  masked_code =
    SK_MakeKeyMask (vKey & 0xFFU, ctrl, shift, alt);
}





bool
SK_AppCache_Manager::loadAppCacheForExe (const wchar_t* wszExe)
{
  ///SK_LOG0 ( ( L" [!] SK_AppCache_Manager::loadAppCacheForExe (%ws)", wszExe ),
  ///            L" AppCache " );

  //
  // "AppCaches" are basically fast-path lookups for a game's user-friendly
  //   name that avoid using SteamAPI, the Steam Client or exhaustive app-
  //     manifest file searches for games we have seen before.
  //
  const wchar_t* wszPath =
    StrStrIW ( wszExe, LR"(SteamApps\common\)" );

  if (wszPath != nullptr)
  {
    std::wstring wszRelPath
    (
      CharNextW (
       StrStrIW (
         CharNextW (
          StrStrIW ( wszPath, LR"(\)" )
                   ),         LR"(\)"
       )
      )
    ); wszRelPath += L'\0';

    PathRemoveFileSpecW (
      wszRelPath.data ()
    );

    //SK_LOG0 ( ( L" Relative Path: %ws ", wszRelPath.data () ),
    //            L" AppCache " );

    wchar_t    wszAppCache [MAX_PATH + 2] = { };
    wcsncpy_s (wszAppCache, MAX_PATH,
      SK_FormatStringW ( LR"(%s\My Mods\SpecialK\Profiles\AppCache\%s\SpecialK.AppCache)",
                           SK_GetDocumentsDir ().c_str (),
                             wszRelPath.data ()
                       ).c_str (), MAX_PATH);

    // It may be necessary to write the INI immediately after creating it,
    //   but usually not.
    bool need_to_create =
      ( GetFileAttributesW (wszAppCache) == INVALID_FILE_ATTRIBUTES );

    if ( need_to_create )
      SK_CreateDirectories (wszAppCache);

    app_cache_db =
      SK_CreateINI         (wszAppCache);

    loadDepotCache ();

    if ( need_to_create )
      app_cache_db->write (app_cache_db->get_filename ());
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
    StrStrIW ( wszPath, LR"(SteamApps\common\)" );

  if (wszSteamApps != nullptr)
  {
    wchar_t* wszRelPath =
    {
      CharNextW (
        StrStrIW (
          CharNextW (
            StrStrIW ( wszSteamApps, LR"(\)" )
                    ),               LR"(\)"
        )
      )
    };

    if (            fwd_map.contains_key (wszRelPath)           )
      return _wtoi (fwd_map.get_value    (wszRelPath).c_str ());
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

  std::wstring app_id_as_wstr =
    std::to_wstring (uiAppID);

  if (     name_map.contains_key (app_id_as_wstr.c_str ()) )
    return name_map.get_value    (app_id_as_wstr.c_str ());

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
  if (app_cache_db == nullptr)
    return false;

  if (StrStrIW (wszFullPath, LR"(SteamApps\common\)") == nullptr)
      return false;

  iSK_INISection& rev_map =
    app_cache_db->get_section (L"AppID_Cache.RevMap");
  iSK_INISection& fwd_map =
    app_cache_db->get_section (L"AppID_Cache.FwdMap");
  iSK_INISection& name_map =
    app_cache_db->get_section (L"AppID_Cache.Names");


  std::wstring wszRelativeCopy (
    std::wstring (wszFullPath) + L'\0'
                               );

  wchar_t* wszRelPath =
  {
    CharNextW (
      StrStrIW (
        CharNextW (
          StrStrIW (
            StrStrIW ( wszRelativeCopy.data (),
                                         LR"(SteamApps\common\)" ),
                                         LR"(\)"  )
                  ),                     LR"(\)"
               )
              )
  };


  wchar_t   wszAppID [32] = { };
  swprintf (wszAppID, L"%0u", uiAppID);

  if (fwd_map.contains_key (wszRelPath))
    fwd_map.get_value      (wszRelPath) = wszAppID;
  else
    fwd_map.add_key_value  (wszRelPath,   wszAppID);


  if (rev_map.contains_key (wszAppID))
    rev_map.get_value      (wszAppID) = wszRelPath;
  else
    rev_map.add_key_value  (wszAppID,   wszRelPath);


  if (name_map.contains_key (wszAppID))
    name_map.get_value      (wszAppID) = wszAppName;
  else
    name_map.add_key_value  (wszAppID,   wszAppName);


  app_cache_db->write ( app_cache_db->get_filename () );


  return true;
}

std::wstring
SK_AppCache_Manager::getConfigPathFromAppPath (const wchar_t* wszPath) const
{
  return
    getConfigPathForAppID ( getAppIDFromPath (wszPath) );
}


std::wstring
SK_AppCache_Manager::getConfigPathForAppID (uint32_t uiAppID) const
{
  static bool recursing = false;

  // If no AppCache (probably not a Steam game), or opting-out of central repo,
  //   then don't parse crap and just use the traditional path.
  if ( app_cache_db == nullptr || (! config.system.central_repository) )
    return SK_GetNaiveConfigPath ();

  std::wstring path = SK_GetNaiveConfigPath (       );
  std::wstring name ( getAppNameFromID      (uiAppID) );

  // Non-trivial name = custom path, remove the old-style <program.exe>
  if (! name.empty ())
  {
    std::wstring original_dir (path);

    size_t pos = 0;
    if (  (pos = path.find (SK_GetHostApp (), pos)) != std::wstring::npos)
    {
      std::wstring       host_app (SK_GetHostApp ());
      path.replace (pos, host_app.length (), L"\0");
    }

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
                                          L'<',  L'>', L'|',
                                        //L'&',

                                          //
                                          // Obviously a period is not an invalid character,
                                          //   but three of them in a row messes with
                                          //     Windows Explorer and some Steam games use
                                          //       ellipsis in their titles.
                                          //
                                          L'.'
                                        };

                                      return
                                        ( invalid_file_char.find (tval) !=
                                          invalid_file_char.end  (    ) );
                                    }
                                ),

                     name.end ()
               );

    // Strip trailing spaces from name, these are usually the result of
    //   deleting one of the non-useable characters above.
    for (auto it = name.rbegin (); it != name.rend (); ++it)
    {
      if (*it == L' ') *it = L'\0';
      else                   break;
    }

    // Truncating a std::wstring by inserting L'\0' actually does nothing,
    //   so construct path by treating name as a C-String.
    path =
      SK_FormatStringW ( LR"(%s\%s\)",
                         path.c_str (),
                         name.c_str () );

    SK_StripTrailingSlashesW (path.data ());

    if (recursing)
      return path;

    DWORD dwAttribs =
      GetFileAttributesW (original_dir.c_str ());

    //if ( GetFileAttributesW (path.c_str ()) == INVALID_FILE_ATTRIBUTES &&
    if (   dwAttribs != INVALID_FILE_ATTRIBUTES &&
         ( dwAttribs & FILE_ATTRIBUTE_DIRECTORY )  )
    {
      UINT
      SK_RecursiveMove ( const wchar_t* wszOrigDir,
                         const wchar_t* wszDestDir,
                               bool     replace );

      if (dll_ini != nullptr)
      {
        DeleteFileW (dll_ini->get_filename ());
              delete dll_ini;
                     dll_ini = nullptr;
      }

      SK_RecursiveMove (original_dir.c_str (), path.c_str (), false);

      recursing         = true;
      SK_GetConfigPathEx (true);
      SK_LoadConfigEx    (L"" );
      recursing         = false;
    }
  }

  return
    path;
}

bool
SK_AppCache_Manager::saveAppCache (bool close)
{
  if (app_cache_db != nullptr)
  {
    storeDepotCache ();

    app_cache_db->write ( app_cache_db->get_filename () );

    if (close)
    {
      delete app_cache_db,
             app_cache_db = nullptr;
    }

    return true;
  }

  return false;
}


time_t
SK_AppCache_Manager::setFriendOwnership ( uint64_t                       friend_,
                                          SK_AppCache_Manager::Ownership owns   )
{
  if (app_cache_db == nullptr)
    return 0;

  auto& sec =
    app_cache_db->get_section (L"Friend.Ownership");

  static_assert ( sizeof (time_t) ==
                  sizeof (unsigned long long) );

  SK_AppCache_Manager::Ownership ownership    = SK_AppCache_Manager::Unknown;
  time_t                         last_updated = 0ULL;

  time_t update_time;
  time (&update_time);

  if (sec.contains_key (std::to_wstring (friend_).c_str ()))
  {
    std::wstring friend_status =
      sec.get_value (std::to_wstring (friend_).c_str ());

    if ( 2 ==
           std::swscanf ( friend_status.c_str (),
                            L"%li {%lli}",
                              &ownership, (int64_t *)&last_updated
                        )
       )
    {
      if (ownership != owns)
      {
        last_updated = update_time;

        sec.add_key_value ( std::to_wstring (friend_).c_str (),
                              SK_FormatStringW (
                                L"%li {%llu}",
                                  owns,
                                    last_updated
                              ).c_str ()
                          );
      }

      return last_updated;
    }
  }

  last_updated = update_time;

  sec.add_key_value ( std::to_wstring (friend_).c_str (),
                        SK_FormatStringW (
                          L"%li {%llu}",
                            owns,
                              last_updated
                        ).c_str ()
                    );

  return last_updated;
}

SK_AppCache_Manager::Ownership
SK_AppCache_Manager::getFriendOwnership ( uint64_t friend_,
                                          time_t*  updated )
{
  if (app_cache_db != nullptr)
  {
    auto& sec =
      app_cache_db->get_section (L"Friend.Ownership");

    SK_AppCache_Manager::Ownership ownership    = SK_AppCache_Manager::Unknown;
    time_t                         last_updated = 0ULL;

    static_assert ( sizeof (time_t) ==
                    sizeof (unsigned long long) );

    if (sec.contains_key (std::to_wstring (friend_).c_str ()))
    {
      std::wstring friend_status =
        sec.get_value    (std::to_wstring (friend_).c_str ());

      if ( 2 ==
             std::swscanf ( friend_status.c_str (),
                              L"%li {%lli}",
                                &ownership,
                                  &last_updated
                          )
         )
      {
        if (updated != nullptr)
           *updated  = last_updated;

        return
          SK_AppCache_Manager::Ownership (ownership);
      }
    }
  }

  if (updated != nullptr)
    *updated = 0;

  return
    SK_AppCache_Manager::Unknown;
}

time_t
SK_AppCache_Manager::setFriendAchievPct (uint64_t friend_, float new_percent)
{
  if (app_cache_db == nullptr)
    return 0;

  auto& sec =
    app_cache_db->get_section (L"Friend.Achievements");

  static_assert ( sizeof (time_t) ==
                  sizeof (unsigned long long) );

   float percent      = 0.0f;
  time_t last_updated = 0ULL;

  time_t update_time;
  time (&update_time);

  if (sec.contains_key (std::to_wstring (friend_).c_str ()))
  {
    std::wstring friend_status =
      sec.get_value    (std::to_wstring (friend_).c_str ());

    if ( 2 ==
           std::swscanf ( friend_status.c_str (),
                            L"%f {%lli}",
                              &percent, (int64_t *)&last_updated
                        )
       )
    {
      if (new_percent != percent)
      {
        last_updated = update_time;

        sec.add_key_value ( std::to_wstring (friend_).c_str (),
                              SK_FormatStringW (
                                L"%f {%lli}",
                                  new_percent,
                                    (int64_t)last_updated
                              ).c_str ()
                          );
      }

      return last_updated;
    }
  }

  last_updated = update_time;

  sec.add_key_value ( std::to_wstring (friend_).c_str (),
                        SK_FormatStringW (
                          L"%f {%lli}",
                            new_percent,
                              (int64_t)last_updated
                        ).c_str ()
                    );

  return last_updated;
}

float
SK_AppCache_Manager::getFriendAchievPct (uint64_t friend_, time_t* updated)
{
  if (app_cache_db != nullptr)
  {
    auto& sec =
      app_cache_db->get_section (L"Friend.Achievements");

    float  percent      = 0.0f;
    time_t last_updated = 0ULL;

    static_assert ( sizeof (time_t) ==
                    sizeof (unsigned long long) );

    time_t update_time;
    time (&update_time);

    if (sec.contains_key (std::to_wstring (friend_).c_str ()))
    {
      std::wstring friend_status =
        sec.get_value    (std::to_wstring (friend_).c_str ());

      if ( 2 ==
             std::swscanf ( friend_status.c_str (),
                              L"%f {%lli}",
                                &percent,
                                  &last_updated
                          )
         )
      {
        if (updated != nullptr)
           *updated  = last_updated;

        return
          percent;
      }
    }
  }

  if (updated != nullptr)
    time (updated);

  return
    0.0f;
}

bool
SK_AppCache_Manager::wantFriendStats (void)
{
  bool global_pref =
    config.steam.achievements.pull_friend_stats;

  if (app_cache_db == nullptr)
    return global_pref;

  auto& sec =
    app_cache_db->get_section (L"Friend.Achievements");

  if (sec.contains_key (L"FetchFromServer"))
  {
    std::wstring& val =
      sec.get_value    (L"FetchFromServer");

    if (val._Equal (L"UseGlobalPreference"))
      return global_pref;

    return
      SK_IsTrue (val.c_str ());
  }

  sec.add_key_value ( L"FetchFromServer",
                        L"UseGlobalPreference" );

  return
    global_pref;
}

int
SK_AppCache_Manager::getLicenseRevision (void)
{
  bool must_show =
    config.imgui.show_eula;

  if (app_cache_db == nullptr)
    return must_show ? 0 : SK_LICENSE_REVISION;

  auto& sec =
    app_cache_db->get_section (L"License.Info");

  if (sec.contains_key (L"LastRevision"))
  {
    std::wstring& val =
      sec.get_value    (L"LastRevision");

    return must_show ? 0 :
      _wtoi (val.c_str ());
  }

  return 0;
}

void
SK_AppCache_Manager::setLicenseRevision (int rev)
{
  if (app_cache_db == nullptr)
    return;

  auto& sec =
    app_cache_db->get_section (L"License.Info");

  sec.add_key_value ( L"LastRevision",
                       std::to_wstring (rev).c_str () );
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
  if (config.apis.last_known != SK_RenderAPI::Reserved)
  {
    return static_cast <SK_RenderAPI> (config.apis.last_known);
  }

  int mask = 0;

#ifdef _M_IX86
  if (config.apis.d3d8.hook)       mask |= static_cast <int> (SK_RenderAPI::D3D8);
  if (config.apis.ddraw.hook)      mask |= static_cast <int> (SK_RenderAPI::DDraw);
#endif
  if (config.apis.d3d9.hook)       mask |= static_cast <int> (SK_RenderAPI::D3D9);
  if (config.apis.d3d9ex.hook)     mask |= static_cast <int> (SK_RenderAPI::D3D9Ex);
  if (config.apis.dxgi.d3d11.hook) mask |= static_cast <int> (SK_RenderAPI::D3D11);
  if (config.apis.OpenGL.hook)     mask |= static_cast <int> (SK_RenderAPI::OpenGL);
#ifdef _M_AMD64
  if (config.apis.Vulkan.hook)     mask |= static_cast <int> (SK_RenderAPI::Vulkan);
  if (config.apis.dxgi.d3d12.hook) mask |= static_cast <int> (SK_RenderAPI::D3D12);
#endif

  return
    static_cast <SK_RenderAPI> (mask);
}

sk_config_t config;