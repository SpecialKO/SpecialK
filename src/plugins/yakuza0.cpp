//
// Copyright 2018 Andon "Kaldaien" Coleman
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

#include <imgui/imgui.h>

#include <SpecialK/config.h>
#include <SpecialK/parameter.h>
#include <SpecialK/control_panel.h>
#include <SpecialK/render/dxgi/dxgi_backend.h>

extern bool SK_D3D11_EnableTracking;

extern iSK_INI*             dll_ini;
extern sk::ParameterFactory g_ParameterFactory;


volatile LONG __SK_Y0_InitiateHudFreeShot = 0;
volatile LONG __SK_Y0_QueuedShots         = 0;

void
SK_YS0_TriggerHudFreeScreenshot (void) noexcept
{
  InterlockedIncrement (&__SK_Y0_QueuedShots);
}

sk::ParameterBool* _SK_Y0_NoFPBlur;
sk::ParameterBool* _SK_Y0_NoSSAO;
sk::ParameterBool* _SK_Y0_NoDOF;

sk::ParameterBool*  _SK_Y0_LockVolume;
sk::ParameterFloat* _SK_Y0_LockLevel;
sk::ParameterBool*  _SK_Y0_QuietStart;
sk::ParameterFloat* _SK_Y0_QuietLevel;

sk::ParameterBool* _SK_Y0_FixAniso;
sk::ParameterBool* _SK_Y0_ClampLODBias;
sk::ParameterInt*  _SK_Y0_ForceAniso;

sk::ParameterInt*   _SK_Y0_SaveAnywhere;
iSK_INI*            _SK_Y0_Settings;

struct {
  int   save_anywhere =     0;
  bool  no_fp_blur    = false;
  bool  no_ssao       = false;
  bool  no_dof        = false;

  bool  lock_volume   =  true;
  float lock_level    =  1.0f;
  bool  quiet_start   =  true;
  float quiet_level   = 0.10f;
  int   __quiet_mode  = false;
} _SK_Y0_Cfg;

bool __SK_Y0_FixShadowAniso  = false;
bool __SK_Y0_FixAniso        =  true;
bool __SK_Y0_ClampLODBias    =  true;
int  __SK_Y0_ForceAnisoLevel =     0;
bool __SK_Y0_FilterUpdate    = false; 

// The two pixel shaders are for the foreground DepthOfField effect
#define SK_Y0_DOF_PS0_CRC32C 0x10d88ce3
#define SK_Y0_DOF_PS1_CRC32C 0x419dcbfc
#define SK_Y0_DOF_VS_CRC32C  0x0f5fefc2

#include <SpecialK/sound.h>

void
SK_Yakuza0_BeginFrame (void)
{
  if ( ReadAcquire (&__SK_Y0_QueuedShots)          > 0 ||
       ReadAcquire (&__SK_Y0_InitiateHudFreeShot) != 0    )
  {
#define SK_Y0_HUD_PS_CRC32C 0x2e24510d

    if (InterlockedCompareExchange (&__SK_Y0_InitiateHudFreeShot, -1, 1) == 1)
    {
      SK_D3D11_Shaders.pixel.addTrackingRef (SK_D3D11_Shaders.pixel.blacklist, SK_Y0_HUD_PS_CRC32C);

      SK::SteamAPI::TakeScreenshot (SK::ScreenshotStage::BeforeOSD);
    }

    else if (InterlockedCompareExchange (&__SK_Y0_InitiateHudFreeShot, 0, -1) == -1)
    {
      SK_D3D11_Shaders.pixel.releaseTrackingRef (SK_D3D11_Shaders.pixel.blacklist, SK_Y0_HUD_PS_CRC32C);
    }

    else
    {
      InterlockedDecrement (&__SK_Y0_QueuedShots);
      InterlockedExchange  (&__SK_Y0_InitiateHudFreeShot, 1);

      return
        SK_Yakuza0_BeginFrame ();
    }
  }


  static bool done = false;

  if (_SK_Y0_Cfg.quiet_start && (! done))
  {
    _SK_Y0_Cfg.__quiet_mode = true;

    static CComPtr <ISimpleAudioVolume> pVolume =
      SK_WASAPI_GetVolumeControl (GetCurrentProcessId ());

    if (pVolume == nullptr)
        pVolume  = SK_WASAPI_GetVolumeControl (GetCurrentProcessId ());

    static float fOrigVol    = 0.0;

    if (pVolume != nullptr    &&   fOrigVol == 0.0f)
        pVolume->GetMasterVolume (&fOrigVol);

    static DWORD dwStartTime =
        timeGetTime ();
    if (timeGetTime () < (dwStartTime + 20000UL))
    {
      if (pVolume != nullptr)
      {
        pVolume->SetMasterVolume ( _SK_Y0_Cfg.quiet_level,
                                     nullptr );
      }
    }

    else
    {
      if (pVolume != nullptr)
          pVolume->SetMasterVolume (fOrigVol, nullptr);

      _SK_Y0_Cfg.__quiet_mode = false;
      done = true;
    }
  }

  else if (_SK_Y0_Cfg.lock_volume)
  {
    static CComPtr <ISimpleAudioVolume> pVolume =
      SK_WASAPI_GetVolumeControl (GetCurrentProcessId ());

    if (pVolume != nullptr)
       pVolume->SetMasterVolume (_SK_Y0_Cfg.lock_level, nullptr);
  }
}

void
SK_Yakuza0_PlugInInit (void)
{
  extern std::wstring&
    SK_GetRoamingDir (void);

  std::wstring game_settings =
    SK_GetRoamingDir ();

  game_settings += LR"(\Sega\Yakuza0\settings.ini)";

  _SK_Y0_Settings =
    SK_CreateINI (game_settings.c_str ());

  _SK_Y0_SaveAnywhere =
    dynamic_cast <sk::ParameterInt *> (
      g_ParameterFactory.create_parameter <int> (L"Save Anywhere")
    );

  _SK_Y0_SaveAnywhere->register_to_ini (_SK_Y0_Settings, L"General", L"SaveAnywhere");
  _SK_Y0_SaveAnywhere->load            (_SK_Y0_Cfg.save_anywhere);

  _SK_Y0_NoFPBlur =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory.create_parameter <bool> (L"No First-Person Blur")
      );

  _SK_Y0_NoSSAO =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory.create_parameter <bool> (L"No SSAO")
      );

  _SK_Y0_NoDOF =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory.create_parameter <bool> (L"No Depth of Field")
      );

  _SK_Y0_NoFPBlur->register_to_ini (
    SK_GetDLLConfig (), L"Yakuza0.Shaders", L"DisableFirstPersonBlur"
  );

  _SK_Y0_NoSSAO->register_to_ini (
    SK_GetDLLConfig (), L"Yakuza0.Shaders", L"DisableSSAO"
  );

  _SK_Y0_NoDOF->register_to_ini (
    SK_GetDLLConfig (), L"Yakuza0.Shaders", L"DisableDOF"
  );


  _SK_Y0_FixAniso = 
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory.create_parameter <bool> (L"Fix Anisotropy")
      );
  _SK_Y0_ClampLODBias = 
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory.create_parameter <bool> (L"Clamp Negative LOD Bias")
      );
  _SK_Y0_ForceAniso = 
    dynamic_cast <sk::ParameterInt *> (
      g_ParameterFactory.create_parameter <int> (L"Force Anisotropic Filtering")
      );

  _SK_Y0_FixAniso->register_to_ini (
    SK_GetDLLConfig (), L"Yakuza0.Textures", L"TrilinearToAniso"
  );
  _SK_Y0_ClampLODBias->register_to_ini (
    SK_GetDLLConfig (), L"Yakuza0.Textures", L"ClampLODBias"
  );
  _SK_Y0_ForceAniso->register_to_ini (
    SK_GetDLLConfig (), L"Yakuza0.Textures", L"ForceAnisoLevel"
  );


  _SK_Y0_QuietStart =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory.create_parameter <bool> (L"Quieter Start")
      );
  _SK_Y0_LockVolume =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory.create_parameter <bool> (L"Prevent Volume Changes")
      );

  _SK_Y0_LockLevel =
    dynamic_cast <sk::ParameterFloat *> (
      g_ParameterFactory.create_parameter <float> (L"Volume Lock Level")
      );
  _SK_Y0_QuietLevel =
    dynamic_cast <sk::ParameterFloat *> (
      g_ParameterFactory.create_parameter <float> (L"Volume Start Level")
      );

  _SK_Y0_QuietStart->register_to_ini (
    SK_GetDLLConfig (), L"Yakuza0.Sound", L"QuietStart"
  );
  _SK_Y0_QuietLevel->register_to_ini (
    SK_GetDLLConfig (), L"Yakuza0.Sound", L"QuietLevel"
  );
  _SK_Y0_LockVolume->register_to_ini (
    SK_GetDLLConfig (), L"Yakuza0.Sound", L"LockVolume"
  );
  _SK_Y0_LockLevel->register_to_ini (
    SK_GetDLLConfig (), L"Yakuza0.Sound", L"LockLevel"
  );

  _SK_Y0_NoDOF->load    (_SK_Y0_Cfg.no_dof);
  _SK_Y0_NoSSAO->load   (_SK_Y0_Cfg.no_ssao);
  _SK_Y0_NoFPBlur->load (_SK_Y0_Cfg.no_fp_blur);

  _SK_Y0_QuietStart->load (_SK_Y0_Cfg.quiet_start);
  _SK_Y0_QuietLevel->load (_SK_Y0_Cfg.quiet_level);
  _SK_Y0_LockVolume->load (_SK_Y0_Cfg.lock_volume);
  _SK_Y0_LockLevel->load  (_SK_Y0_Cfg.lock_level);

  _SK_Y0_ForceAniso->load   (__SK_Y0_ForceAnisoLevel);
  _SK_Y0_FixAniso->load     (__SK_Y0_FixAniso);
  _SK_Y0_ClampLODBias->load (__SK_Y0_ClampLODBias);

  extern bool SK_D3D11_EnableTracking;

  if ( _SK_Y0_Cfg.no_fp_blur ||
       _SK_Y0_Cfg.no_dof     ||
       _SK_Y0_Cfg.no_ssao       ) SK_D3D11_EnableTracking = true;

  if (_SK_Y0_Cfg.no_ssao)
  { SK_D3D11_Shaders.vertex.addTrackingRef (SK_D3D11_Shaders.vertex.blacklist, 0x97837269);
    SK_D3D11_Shaders.vertex.addTrackingRef (SK_D3D11_Shaders.vertex.blacklist, 0x7cc07f78);
    SK_D3D11_Shaders.vertex.addTrackingRef (SK_D3D11_Shaders.vertex.blacklist, 0xe5d4a297);
    SK_D3D11_Shaders.pixel.addTrackingRef  (SK_D3D11_Shaders.pixel.blacklist,  0x4d2973a3); 
    SK_D3D11_Shaders.pixel.addTrackingRef  (SK_D3D11_Shaders.pixel.blacklist,  0x0ed648e1);
    SK_D3D11_Shaders.pixel.addTrackingRef  (SK_D3D11_Shaders.pixel.blacklist,  0x170885b9);
    SK_D3D11_Shaders.pixel.addTrackingRef  (SK_D3D11_Shaders.pixel.blacklist,  0x4d2973a3);
    SK_D3D11_Shaders.pixel.addTrackingRef  (SK_D3D11_Shaders.pixel.blacklist,  0x5256777a);
    SK_D3D11_Shaders.pixel.addTrackingRef  (SK_D3D11_Shaders.pixel.blacklist,  0x69b8ef91); }

  if (_SK_Y0_Cfg.no_dof)
  { SK_D3D11_Shaders.vertex.addTrackingRef (SK_D3D11_Shaders.vertex.blacklist, SK_Y0_DOF_VS_CRC32C);
    SK_D3D11_Shaders.pixel.addTrackingRef  (SK_D3D11_Shaders.pixel.blacklist,  SK_Y0_DOF_PS0_CRC32C);
    SK_D3D11_Shaders.pixel.addTrackingRef  (SK_D3D11_Shaders.pixel.blacklist,  SK_Y0_DOF_PS1_CRC32C); }

  if (_SK_Y0_Cfg.no_fp_blur)
  { SK_D3D11_Shaders.vertex.addTrackingRef (SK_D3D11_Shaders.vertex.blacklist, 0xb008686a); 
    SK_D3D11_Shaders.pixel.addTrackingRef  (SK_D3D11_Shaders.pixel.blacklist,  0x1c599fa7); }
}

bool __SK_Y0_1024_512 = true;
bool __SK_Y0_1024_768 = true;
bool __SK_Y0_960_540  = true;

bool
SK_Yakuza0_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("Yakuza 0", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    bool changed = false;

    static bool ssao_changed = false;

    ImGui::BeginGroup ();
    changed |= ImGui::Checkbox ("Disable First-Person Blur", &_SK_Y0_Cfg.no_fp_blur);
    changed |= ImGui::Checkbox ("Disable Depth of Field",    &_SK_Y0_Cfg.no_dof);
    changed |= ImGui::Checkbox ("Disable Ambient Occlusion", &_SK_Y0_Cfg.no_ssao);
    ImGui::EndGroup ();

    ImGui::SameLine ();

    ImGui::BeginGroup ();

    static CComPtr <ISimpleAudioVolume> pVolume =
      SK_WASAPI_GetVolumeControl (GetCurrentProcessId ());

    bool sound_changed = false;

    if (! _SK_Y0_Cfg.__quiet_mode)
    {
      sound_changed |=
        ImGui::Checkbox ("Lock Volume", &_SK_Y0_Cfg.lock_volume);

      if (_SK_Y0_Cfg.lock_volume)
      {
        ImGui::SameLine ();

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("The game occasionally fudges with volume, but you lock it down.");

        if (ImGui::SliderFloat ("Master Volume Control", &_SK_Y0_Cfg.lock_level, 0.0, 1.0, ""))
        {
          if (_SK_Y0_Cfg.lock_volume)
          {
            sound_changed = true;
          }

          pVolume->SetMasterVolume (_SK_Y0_Cfg.lock_level, nullptr);
        }
        ImGui::SameLine ();
        ImGui::TextColored ( ImColor::HSV ( 0.15f, 0.9f,
                            0.5f + _SK_Y0_Cfg.lock_level * 0.5f),
                            "(%03.1f%%)  ",
                            _SK_Y0_Cfg.lock_level * 100.0f );
      }
    }

    sound_changed |= ImGui::Checkbox ("Quiet Start Mode", &_SK_Y0_Cfg.quiet_start);

    if (_SK_Y0_Cfg.quiet_start)
    {
      ImGui::SameLine ();
      sound_changed |=
        ImGui::SliderFloat ("Intro Volume Level", &_SK_Y0_Cfg.quiet_level, 0.0, 1.0, "");
      ImGui::SameLine ();
      ImGui::TextColored ( ImColor::HSV ( 0.3f, 0.9f,
                          1.0f - _SK_Y0_Cfg.quiet_level * 0.5f),
                          "(%03.1f%%)  ",
                          _SK_Y0_Cfg.quiet_level * 100.0f );
    }

    if (sound_changed)
    {
      _SK_Y0_QuietStart->store (_SK_Y0_Cfg.quiet_start);
      _SK_Y0_QuietLevel->store (_SK_Y0_Cfg.quiet_level);
      _SK_Y0_LockVolume->store (_SK_Y0_Cfg.lock_volume);
      _SK_Y0_LockLevel->store  (_SK_Y0_Cfg.lock_level);
    }

    ImGui::EndGroup   ();

    if (config.steam.screenshots.enable_hook)
    {
      ImGui::PushID    ("Y0_Screenshots");
      ImGui::Separator ();

      auto Keybinding = [] (SK_Keybind* binding, sk::ParameterStringW* param) ->
        auto
      {
        std::string label  = SK_WideCharToUTF8 (binding->human_readable) + "###";
        label += binding->bind_name;

        if (ImGui::Selectable (label.c_str (), false))
        {
          ImGui::OpenPopup (binding->bind_name);
        }

        std::wstring original_binding = binding->human_readable;

        SK_ImGui_KeybindDialog (binding);

        if (original_binding != binding->human_readable)
        {
          param->store (binding->human_readable);

          SK_SaveConfig ();

          return true;
        }

        return false;
      };

      static std::set <SK_ConfigSerializedKeybind *>
        keybinds = {
        &config.steam.screenshots.game_hud_free_keybind
      };

      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      for ( auto& keybind : keybinds )
      {
        ImGui::Text          ( "%s:  ",
                              keybind->bind_name );
      }
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      for ( auto& keybind : keybinds )
      {
        Keybinding ( keybind, keybind->param );
      }
      ImGui::EndGroup   ();

      bool png_changed = false;

      if (config.steam.screenshots.enable_hook)
      {
        png_changed =
          ImGui::Checkbox ( "Keep Lossless .PNG Screenshots",
                           &config.steam.screenshots.png_compress      );
      }

      if ( ( screenshot_manager != nullptr &&
          screenshot_manager->getExternalScreenshotRepository ().files > 0 ) )
      {
        ImGui::SameLine ();

        const SK_Steam_ScreenshotManager::screenshot_repository_s& repo =
          screenshot_manager->getExternalScreenshotRepository (png_changed);

        ImGui::BeginGroup (  );
        ImGui::TreePush   ("");
        ImGui::Text ( "%lu files using %ws",
                     repo.files,
                     SK_File_SizeToString (repo.liSize.QuadPart).c_str  ()
        );

        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip ( "Steam does not support .png screenshots, so "
                             "SK maintains its own storage for lossless screenshots." );
        }

        ImGui::SameLine ();

        if (ImGui::Button ("Browse"))
        {
          ShellExecuteW ( GetActiveWindow (),
                         L"explore",
                         screenshot_manager->getExternalScreenshotPath (),
                         nullptr, nullptr,
                         SW_NORMAL
          );
        }

        ImGui::TreePop  ();
        ImGui::EndGroup ();
      }
      ImGui::PopID      ();
    }

    ImGui::Separator ();

    if (ImGui::CollapsingHeader ("Texture Settings"))
    {
      static bool tex_changed = false;

      ImGui::TreePush ("");

      bool new_change = false;

      new_change |= ImGui::Checkbox  ("Fix Anisotropic Filtering", &__SK_Y0_FixAniso);
      new_change |= ImGui::Checkbox  ("Clamp LOD Bias",            &__SK_Y0_ClampLODBias);
      new_change |= ImGui::SliderInt ("Force Anisotropic Level",   &__SK_Y0_ForceAnisoLevel, 0, 16);

      if (new_change)
      {
        tex_changed |= new_change;

        _SK_Y0_FixAniso->store     (__SK_Y0_FixAniso);
        _SK_Y0_ClampLODBias->store (__SK_Y0_ClampLODBias);
        _SK_Y0_ForceAniso->store   (__SK_Y0_ForceAnisoLevel);
        SK_GetDLLConfig ()->write  (SK_GetDLLConfig ()->get_filename ());
      }

      if (tex_changed)
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f));
        ImGui::BulletText     ("Restart Game");
        ImGui::PopStyleColor  ();
      }
      ImGui::TreePop ();
    }

    if (changed)
    {
      // SSAO
      if (_SK_Y0_Cfg.no_ssao)
      { SK_D3D11_Shaders.vertex.addTrackingRef (SK_D3D11_Shaders.vertex.blacklist, 0x97837269);
        SK_D3D11_Shaders.vertex.addTrackingRef (SK_D3D11_Shaders.vertex.blacklist, 0x7cc07f78);
        SK_D3D11_Shaders.vertex.addTrackingRef (SK_D3D11_Shaders.vertex.blacklist, 0xe5d4a297);
        SK_D3D11_Shaders.pixel.addTrackingRef  (SK_D3D11_Shaders.vertex.blacklist, 0x4d2973a3); 
        SK_D3D11_Shaders.pixel.addTrackingRef  (SK_D3D11_Shaders.vertex.blacklist, 0x0ed648e1);
        SK_D3D11_Shaders.pixel.addTrackingRef  (SK_D3D11_Shaders.vertex.blacklist, 0x170885b9);
        SK_D3D11_Shaders.pixel.addTrackingRef  (SK_D3D11_Shaders.vertex.blacklist, 0x4d2973a3);
        SK_D3D11_Shaders.pixel.addTrackingRef  (SK_D3D11_Shaders.vertex.blacklist, 0x5256777a);
        SK_D3D11_Shaders.pixel.addTrackingRef  (SK_D3D11_Shaders.vertex.blacklist, 0x69b8ef91); }
      else
      { SK_D3D11_Shaders.vertex.releaseTrackingRef (SK_D3D11_Shaders.vertex.blacklist, 0x97837269);
        SK_D3D11_Shaders.vertex.releaseTrackingRef (SK_D3D11_Shaders.vertex.blacklist, 0x7cc07f78);
        SK_D3D11_Shaders.vertex.releaseTrackingRef (SK_D3D11_Shaders.vertex.blacklist, 0xe5d4a297);
        SK_D3D11_Shaders.pixel.releaseTrackingRef  (SK_D3D11_Shaders.pixel.blacklist,  0x4d2973a3); 
        SK_D3D11_Shaders.pixel.releaseTrackingRef  (SK_D3D11_Shaders.pixel.blacklist,  0x0ed648e1);
        SK_D3D11_Shaders.pixel.releaseTrackingRef  (SK_D3D11_Shaders.pixel.blacklist,  0x170885b9);
        SK_D3D11_Shaders.pixel.releaseTrackingRef  (SK_D3D11_Shaders.pixel.blacklist,  0x4d2973a3);
        SK_D3D11_Shaders.pixel.releaseTrackingRef  (SK_D3D11_Shaders.pixel.blacklist,  0x5256777a);
        SK_D3D11_Shaders.pixel.releaseTrackingRef  (SK_D3D11_Shaders.pixel.blacklist,  0x69b8ef91); }

      // DOF
      if (_SK_Y0_Cfg.no_dof)
      { SK_D3D11_Shaders.vertex.addTrackingRef (SK_D3D11_Shaders.vertex.blacklist, SK_Y0_DOF_VS_CRC32C);
        SK_D3D11_Shaders.pixel.addTrackingRef  (SK_D3D11_Shaders.pixel.blacklist,  SK_Y0_DOF_PS0_CRC32C);
        SK_D3D11_Shaders.pixel.addTrackingRef  (SK_D3D11_Shaders.pixel.blacklist,  SK_Y0_DOF_PS1_CRC32C);
      }
      else
      { SK_D3D11_Shaders.vertex.releaseTrackingRef (SK_D3D11_Shaders.vertex.blacklist, SK_Y0_DOF_VS_CRC32C);
        SK_D3D11_Shaders.pixel.releaseTrackingRef  (SK_D3D11_Shaders.pixel.blacklist,  SK_Y0_DOF_PS0_CRC32C);
        SK_D3D11_Shaders.pixel.releaseTrackingRef  (SK_D3D11_Shaders.pixel.blacklist,  SK_Y0_DOF_PS1_CRC32C);
      }

      // First Person Blur
      if (_SK_Y0_Cfg.no_fp_blur)
      { SK_D3D11_Shaders.vertex.addTrackingRef (SK_D3D11_Shaders.vertex.blacklist, 0xb008686a); 
        SK_D3D11_Shaders.pixel.addTrackingRef  (SK_D3D11_Shaders.pixel.blacklist,  0x1c599fa7); }
      else
      { SK_D3D11_Shaders.vertex.releaseTrackingRef (SK_D3D11_Shaders.vertex.blacklist, 0xb008686a); 
        SK_D3D11_Shaders.pixel.releaseTrackingRef  (SK_D3D11_Shaders.pixel.blacklist,  0x1c599fa7); }

      _SK_Y0_NoDOF->store    (_SK_Y0_Cfg.no_dof);
      _SK_Y0_NoSSAO->store   (_SK_Y0_Cfg.no_ssao);
      _SK_Y0_NoFPBlur->store (_SK_Y0_Cfg.no_fp_blur);

      SK_GetDLLConfig   ( )->write (
        SK_GetDLLConfig ( )->get_filename ( )
                                   );

      extern bool SK_D3D11_EnableTracking;

      if ( _SK_Y0_Cfg.no_fp_blur ||
           _SK_Y0_Cfg.no_dof     ||
           _SK_Y0_Cfg.no_ssao       ) SK_D3D11_EnableTracking = true;

      return true;
    }

    ImGui::TreePop ();
  }

  return false;
}