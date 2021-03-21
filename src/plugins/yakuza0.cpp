z//
// Copyright 2018 - 2019 Andon "Kaldaien" Coleman
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

#include <SpecialK/stdafx.h>
#include <SpecialK/render/d3d11/d3d11_core.h>

extern volatile
  LONG SK_D3D11_DrawTrackingReqs;
extern volatile
  LONG SK_D3D11_CBufferTrackingReqs;

extern bool SK_D3D11_EnableTracking;

extern iSK_INI* dll_ini;


volatile LONG __SK_Y0_InitiateHudFreeShot = 0;
volatile LONG __SK_Y0_QueuedShots         = 0;

bool
SK_Yakuza0_PlugInCfg (void);

typedef void (__stdcall *SK_ReShade_SetResolutionScale_pfn)(float fScale);
                  static SK_ReShade_SetResolutionScale_pfn
                         SK_ReShade_SetResolutionScale = nullptr;


typedef HRESULT (STDMETHODCALLTYPE *DXGISwap_ResizeBuffers_pfn)(
       IDXGISwapChain* This,
  _In_ UINT            BufferCount,
  _In_ UINT            Width,
  _In_ UINT            Height,
  _In_ DXGI_FORMAT     NewFormat,
  _In_ UINT            SwapChainFlags
);

static DXGISwap_ResizeBuffers_pfn
       DXGISwap_ResizeBuffers_NeverCallThis = nullptr;

static HRESULT
STDMETHODCALLTYPE
SK_Yakuza_ResizeBuffers_NOP (IDXGISwapChain* This,
                            _In_ UINT            BufferCount,
                            _In_ UINT            Width,
                            _In_ UINT            Height,
                            _In_ DXGI_FORMAT     NewFormat,
                            _In_ UINT            SwapChainFlags)
{
  UNREFERENCED_PARAMETER (This);
  UNREFERENCED_PARAMETER (BufferCount);
  UNREFERENCED_PARAMETER (Width);
  UNREFERENCED_PARAMETER (Height);
  UNREFERENCED_PARAMETER (NewFormat);
  UNREFERENCED_PARAMETER (SwapChainFlags);

  // Game will panic and try to release references to resources that it never
  //   acquired, so tell it to STFU and revoke its ability to resize buffers.
  return S_OK;

  //return
  //  DXGISwap_ResizeBuffers_NeverCallThis (
  //    This, BufferCount, Width, Height, NewFormat, SwapChainFlags
  //  );
}


void
SK_YS0_TriggerHudFreeScreenshot (void) noexcept
{
  InterlockedIncrement (&__SK_Y0_QueuedShots);
}


bool __SK_Y0_1024_512 = true;
bool __SK_Y0_1024_768 = true;
bool __SK_Y0_960_540  = true;

bool __SK_Yakuza_TrackRTVs = true;

sk::ParameterBool* _SK_Y0_NoFPBlur;
sk::ParameterBool* _SK_Y0_NoSSAO;
sk::ParameterBool* _SK_Y0_NoDOF;

sk::ParameterBool* _SK_Y_NoDOF0;
sk::ParameterBool* _SK_Y_NoDOF1;
sk::ParameterBool* _SK_Y_NoDOF2;
sk::ParameterBool* _SK_Y_NoDOF3;
sk::ParameterBool* _SK_Y_NoDOF4;
sk::ParameterBool* _SK_Y_NoBlur0;

sk::ParameterBool* _SK_Y_SaveFace;
sk::ParameterBool* _SK_Y_SafetyLeak;

sk::ParameterBool*  _SK_Y0_LockVolume;
sk::ParameterFloat* _SK_Y0_LockLevel;
sk::ParameterBool*  _SK_Y0_QuietStart;
sk::ParameterFloat* _SK_Y0_QuietLevel;

sk::ParameterBool* _SK_Y0_FixAniso;
sk::ParameterBool* _SK_Y0_ClampLODBias;
sk::ParameterInt*  _SK_Y0_ForceAniso;

sk::ParameterInt*   _SK_Y0_SaveAnywhere;
iSK_INI*            _SK_Y0_Settings;

extern SK_LazyGlobal <
  concurrency::concurrent_vector <d3d11_shader_tracking_s::cbuffer_override_s>
> __SK_D3D11_PixelShader_CBuffer_Overrides;

struct {
  int   save_anywhere =     0;
  float lock_level    =  1.0f;
  float quiet_level   = 0.10f;
  bool  quiet_start   =  true;
  bool  __quiet_mode  = false;
  bool  lock_volume   =  true;

  bool  no_fp_blur    = false;
  bool  no_ssao       = false;
  bool  no_dof        = false;

  bool  no_dof0       = false;
  bool  no_dof1       = false;
  bool  no_dof2       = false;
  bool  no_dof3       = false;
  bool  no_dof4       = false;
  bool  no_blur0      = false;
  bool  save_face     = false;
  bool  safety_leak   = true;
} _SK_Y0_Cfg;

bool __SK_Y0_FixShadowAniso  = false;
bool __SK_Y0_FixAniso        =  true;
bool __SK_Y0_ClampLODBias    =  true;
int  __SK_Y0_ForceAnisoLevel =     0;
bool __SK_Y0_FilterUpdate    = false;
bool __SK_Y0_SafetyLeak      =  true;

// The two pixel shaders are for the foreground DepthOfField effect
#define SK_Y0_DOF_PS0_CRC32C 0x10d88ce3
#define SK_Y0_DOF_PS1_CRC32C 0x419dcbfc
#define SK_Y0_DOF_VS_CRC32C  0x0f5fefc2


void
SK_Yakuza0_BeginFrame (void)
{
  static bool yakuza0 =
    SK_GetCurrentGameID () == SK_GAME_ID::Yakuza0;

  if (! yakuza0)
  {
    static bool yakuza_kiwami2 =
      SK_GetCurrentGameID () == SK_GAME_ID::YakuzaKiwami2;

    if (yakuza_kiwami2)
    {
      extern volatile PVOID __SK_GameBaseAddr;
      static         LPVOID         pBaseAddr =
        ReadPointerAcquire (&__SK_GameBaseAddr);

      // Will be set to false if an access violation occurs
      static bool         has_scale = true;
      static float* fp_render_scale =
        (float *)((uintptr_t)pBaseAddr + 0x3F13AB0);//0x3F10BC0);


      if (has_scale)
      {
        auto orig_se =
        SK_SEH_ApplyTranslator (
          SK_FilteringStructuredExceptionTranslator (
            EXCEPTION_ACCESS_VIOLATION
          )
        );
        try
        {
          float fScale =
            (*fp_render_scale);

          if (fScale >= 0.33f && fScale <= 3.0f)
          {
            if (SK_ReShade_SetResolutionScale != nullptr)
                SK_ReShade_SetResolutionScale (fScale);
          }
        }

        catch (const SK_SEH_IgnoredException&)
        {
          has_scale = false;
        }
        SK_SEH_RemoveTranslator (orig_se);
      }
    }
    return;
  }

  if ( ReadAcquire (&__SK_Y0_QueuedShots)          > 0 ||
       ReadAcquire (&__SK_Y0_InitiateHudFreeShot) != 0    )
  {
#define SK_Y0_HUD_PS_CRC32C 0x2e24510d

    if (InterlockedCompareExchange (&__SK_Y0_InitiateHudFreeShot, -1, 1) == 1)
    {
      SK_D3D11_Shaders->pixel.addTrackingRef (
        SK_D3D11_Shaders->pixel.blacklist, SK_Y0_HUD_PS_CRC32C
      );

      SK::SteamAPI::TakeScreenshot (SK_ScreenshotStage::BeforeOSD);
    }

    else if (InterlockedCompareExchange (&__SK_Y0_InitiateHudFreeShot, 0, -1) == -1)
    {
      SK_D3D11_Shaders->pixel.releaseTrackingRef (
        SK_D3D11_Shaders->pixel.blacklist, SK_Y0_HUD_PS_CRC32C
      );
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

    static SK_ComPtr <ISimpleAudioVolume> pVolume =
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
    static SK_ComPtr <ISimpleAudioVolume> pVolume =
      SK_WASAPI_GetVolumeControl (GetCurrentProcessId ());

    if (pVolume != nullptr)
       pVolume->SetMasterVolume (_SK_Y0_Cfg.lock_level, nullptr);
  }
}

d3d11_shader_tracking_s::cbuffer_override_s* blur_shader0 = nullptr;
d3d11_shader_tracking_s::cbuffer_override_s* dof_shader0  = nullptr;
d3d11_shader_tracking_s::cbuffer_override_s* dof_shader1  = nullptr;
d3d11_shader_tracking_s::cbuffer_override_s* dof_shader2  = nullptr;
d3d11_shader_tracking_s::cbuffer_override_s* dof_shader3  = nullptr;
d3d11_shader_tracking_s::cbuffer_override_s* dof_shader4  = nullptr;

struct SK_Yakuza_SaveFace {
  const uint32_t disable_face = 0x43fc79da;
  const uint32_t wire_face0   = 0x9d91a465;
  const uint32_t wire_face1   = 0xdb4b4f21;

  bool face_saved             = false;

  bool set (bool state)
  {
    bool orig_state = face_saved;

    face_saved = state;

    if (face_saved != orig_state)
    {
      if (face_saved)
      {
        SK_D3D11_Shaders->pixel.
          addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, disable_face);
        SK_D3D11_Shaders->pixel.
          addTrackingRef (SK_D3D11_Shaders->pixel.wireframe, wire_face0  );
        SK_D3D11_Shaders->pixel.
          addTrackingRef (SK_D3D11_Shaders->pixel.wireframe, wire_face1  );

        InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
      }

      else
      {
        SK_D3D11_Shaders->pixel.
          releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, disable_face);
        SK_D3D11_Shaders->pixel.
          releaseTrackingRef (SK_D3D11_Shaders->pixel.wireframe, wire_face0  );
        SK_D3D11_Shaders->pixel.
          releaseTrackingRef (SK_D3D11_Shaders->pixel.wireframe, wire_face1  );

        InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
      }
    }

    return orig_state;
  }

  void toggle (void)
  {
    set (! face_saved);
  }
};

SK_LazyGlobal <SK_Yakuza_SaveFace> kiwami2_face;

void
SK_Yakuza0_PlugInInit (void)
{
  plugin_mgr->config_fns.push_back      (SK_Yakuza0_PlugInCfg);
  plugin_mgr->begin_frame_fns.push_back (SK_Yakuza0_BeginFrame);
  static bool yakuza0 =
    SK_GetCurrentGameID () == SK_GAME_ID::Yakuza0;
  static bool yakuza_dragon =
    SK_GetCurrentGameID () == SK_GAME_ID::YakuzaUnderflow;

  extern std::wstring&
    SK_GetRoamingDir (void);

  if (yakuza0)
  {
    std::wstring game_settings =
      SK_GetRoamingDir ();

    game_settings += LR"(\Sega\Yakuza0\settings.ini)";

    _SK_Y0_Settings =
      SK_CreateINI (game_settings.c_str ());

    _SK_Y0_SaveAnywhere =
      dynamic_cast <sk::ParameterInt *> (
        g_ParameterFactory->create_parameter <int> (L"Save Anywhere")
      );

    _SK_Y0_SaveAnywhere->register_to_ini (_SK_Y0_Settings, L"General", L"SaveAnywhere");
    _SK_Y0_SaveAnywhere->load            (_SK_Y0_Cfg.save_anywhere);

    _SK_Y0_NoFPBlur =
      dynamic_cast <sk::ParameterBool *> (
        g_ParameterFactory->create_parameter <bool> (L"No First-Person Blur")
        );

    _SK_Y0_NoSSAO =
      dynamic_cast <sk::ParameterBool *> (
        g_ParameterFactory->create_parameter <bool> (L"No SSAO")
        );

    _SK_Y0_NoFPBlur->register_to_ini (
      SK_GetDLLConfig (), L"Yakuza0.Shaders", L"DisableFirstPersonBlur"
    );

    _SK_Y0_NoSSAO->register_to_ini (
      SK_GetDLLConfig (), L"Yakuza0.Shaders", L"DisableSSAO"
    );
  }


  _SK_Y0_FixAniso =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory->create_parameter <bool> (L"Fix Anisotropy")
      );
  _SK_Y0_ClampLODBias =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory->create_parameter <bool> (L"Clamp Negative LOD Bias")
      );
  _SK_Y0_ForceAniso =
    dynamic_cast <sk::ParameterInt *> (
      g_ParameterFactory->create_parameter <int> (L"Force Anisotropic Filtering")
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

  _SK_Y_SafetyLeak =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory->create_parameter <bool> (L"Leak Memory For Stability")
    );

  _SK_Y_SafetyLeak->register_to_ini (
    SK_GetDLLConfig (), L"YakuzaKiwami.Memory", L"LeakInsteadOfCrash"
  );

  _SK_Y_SafetyLeak->load (_SK_Y0_Cfg.safety_leak);
  __SK_Y0_SafetyLeak    = _SK_Y0_Cfg.safety_leak;

  config.textures.d3d11.cache = __SK_Y0_SafetyLeak;
  __SK_Yakuza_TrackRTVs       = __SK_Y0_SafetyLeak;

  if (__SK_Y0_SafetyLeak)
  {
    SK_CreateFuncHook ( L"DXGISwap_ResizeBuffers_Override",
                          DXGISwap_ResizeBuffers_Override,
                         SK_Yakuza_ResizeBuffers_NOP,
 static_cast_p2p <void> (&DXGISwap_ResizeBuffers_NeverCallThis) );
    SK_EnableHook     (   DXGISwap_ResizeBuffers_Override );
  }

  if (! (yakuza0 || yakuza_dragon))
  {
    _SK_Y_NoBlur0 =
      dynamic_cast <sk::ParameterBool *> (
        g_ParameterFactory->create_parameter <bool> (L"No Blur")
      );

    _SK_Y_NoBlur0->register_to_ini (
      SK_GetDLLConfig (), L"YakuzaKiwami.Shaders", L"DisableBlur0"
    );

    // Default to turning game's FXAA/SMAA off
    if (! _SK_Y_NoBlur0->load (_SK_Y0_Cfg.no_blur0))
                               _SK_Y0_Cfg.no_blur0 = true;

    _SK_Y_NoDOF0 =
      dynamic_cast <sk::ParameterBool *> (
        g_ParameterFactory->create_parameter <bool> (L"No Depth of Field")
      );

    _SK_Y_NoDOF0->register_to_ini (
      SK_GetDLLConfig (), L"YakuzaKiwami.Shaders", L"DisableDOF0"
    );

    _SK_Y_NoDOF0->load    (_SK_Y0_Cfg.no_dof0);

    _SK_Y_NoDOF1 =
      dynamic_cast <sk::ParameterBool *> (
        g_ParameterFactory->create_parameter <bool> (L"No Depth of Field")
      );

    _SK_Y_NoDOF1->register_to_ini (
      SK_GetDLLConfig (), L"YakuzaKiwami.Shaders", L"DisableDOF1"
    );

    _SK_Y_NoDOF1->load    (_SK_Y0_Cfg.no_dof1);

    _SK_Y_NoDOF2 =
      dynamic_cast <sk::ParameterBool *> (
        g_ParameterFactory->create_parameter <bool> (L"No Depth of Field")
      );

    _SK_Y_NoDOF2->register_to_ini (
      SK_GetDLLConfig (), L"YakuzaKiwami.Shaders", L"DisableDOF2"
    );

    _SK_Y_NoDOF2->load    (_SK_Y0_Cfg.no_dof2);

    _SK_Y_NoDOF3 =
      dynamic_cast <sk::ParameterBool *> (
        g_ParameterFactory->create_parameter <bool> (L"No Depth of Field")
      );

    _SK_Y_NoDOF3->register_to_ini (
      SK_GetDLLConfig (), L"YakuzaKiwami.Shaders", L"DisableDOF3"
    );

    _SK_Y_NoDOF3->load    (_SK_Y0_Cfg.no_dof3);

    _SK_Y_NoDOF4 =
      dynamic_cast <sk::ParameterBool *> (
        g_ParameterFactory->create_parameter <bool> (L"No Depth of Field")
      );

    _SK_Y_NoDOF4->register_to_ini (
      SK_GetDLLConfig (), L"YakuzaKiwami.Shaders", L"DisableDOF4"
    );

    _SK_Y_NoDOF4->load    (_SK_Y0_Cfg.no_dof4);
  }


  if (yakuza0)
  {
    __SK_Y0_SafetyLeak    = false;

    config.textures.d3d11.cache = __SK_Y0_SafetyLeak;
    __SK_Yakuza_TrackRTVs       = true;//__SK_Y0_SafetyLeak;

    _SK_Y0_NoDOF =
      dynamic_cast <sk::ParameterBool *> (
        g_ParameterFactory->create_parameter <bool> (L"No Depth of Field")
      );

    _SK_Y0_NoDOF->register_to_ini (
      SK_GetDLLConfig (), L"Yakuza0.Shaders", L"DisableDOF"
    );

    _SK_Y0_NoDOF->load    (_SK_Y0_Cfg.no_dof);

    _SK_Y0_QuietStart =
      dynamic_cast <sk::ParameterBool *> (
        g_ParameterFactory->create_parameter <bool> (L"Quieter Start")
        );
    _SK_Y0_LockVolume =
      dynamic_cast <sk::ParameterBool *> (
        g_ParameterFactory->create_parameter <bool> (L"Prevent Volume Changes")
        );

    _SK_Y0_LockLevel =
      dynamic_cast <sk::ParameterFloat *> (
        g_ParameterFactory->create_parameter <float> (L"Volume Lock Level")
        );
    _SK_Y0_QuietLevel =
      dynamic_cast <sk::ParameterFloat *> (
        g_ParameterFactory->create_parameter <float> (L"Volume Start Level")
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

    _SK_Y0_NoSSAO->load   (_SK_Y0_Cfg.no_ssao);
    _SK_Y0_NoFPBlur->load (_SK_Y0_Cfg.no_fp_blur);

    _SK_Y0_QuietStart->load (_SK_Y0_Cfg.quiet_start);
    _SK_Y0_QuietLevel->load (_SK_Y0_Cfg.quiet_level);
    _SK_Y0_LockVolume->load (_SK_Y0_Cfg.lock_volume);
    _SK_Y0_LockLevel->load  (_SK_Y0_Cfg.lock_level);
  }

  _SK_Y0_ForceAniso->load   (__SK_Y0_ForceAnisoLevel);
  _SK_Y0_FixAniso->load     (__SK_Y0_FixAniso);
  _SK_Y0_ClampLODBias->load (__SK_Y0_ClampLODBias);

  if (yakuza0)
  {
    if (_SK_Y0_Cfg.no_ssao)
    { SK_D3D11_Shaders->vertex.addTrackingRef (SK_D3D11_Shaders->vertex.blacklist, 0x97837269);
      SK_D3D11_Shaders->vertex.addTrackingRef (SK_D3D11_Shaders->vertex.blacklist, 0x7cc07f78);
      SK_D3D11_Shaders->vertex.addTrackingRef (SK_D3D11_Shaders->vertex.blacklist, 0xe5d4a297);
      SK_D3D11_Shaders->pixel.addTrackingRef  (SK_D3D11_Shaders->pixel.blacklist,  0x4d2973a3);
      SK_D3D11_Shaders->pixel.addTrackingRef  (SK_D3D11_Shaders->pixel.blacklist,  0x0ed648e1);
      SK_D3D11_Shaders->pixel.addTrackingRef  (SK_D3D11_Shaders->pixel.blacklist,  0x170885b9);
      SK_D3D11_Shaders->pixel.addTrackingRef  (SK_D3D11_Shaders->pixel.blacklist,  0x4d2973a3);
      SK_D3D11_Shaders->pixel.addTrackingRef  (SK_D3D11_Shaders->pixel.blacklist,  0x5256777a);
      SK_D3D11_Shaders->pixel.addTrackingRef  (SK_D3D11_Shaders->pixel.blacklist,  0x69b8ef91); }

    if (_SK_Y0_Cfg.no_dof)
    { SK_D3D11_Shaders->vertex.addTrackingRef (SK_D3D11_Shaders->vertex.blacklist, SK_Y0_DOF_VS_CRC32C);
      SK_D3D11_Shaders->pixel.addTrackingRef  (SK_D3D11_Shaders->pixel.blacklist,  SK_Y0_DOF_PS0_CRC32C);
      SK_D3D11_Shaders->pixel.addTrackingRef  (SK_D3D11_Shaders->pixel.blacklist,  SK_Y0_DOF_PS1_CRC32C); }

    if (_SK_Y0_Cfg.no_fp_blur)
    { SK_D3D11_Shaders->vertex.addTrackingRef (SK_D3D11_Shaders->vertex.blacklist, 0xb008686a);
      SK_D3D11_Shaders->pixel.addTrackingRef  (SK_D3D11_Shaders->pixel.blacklist,  0x1c599fa7); }
  }

  else if (! yakuza_dragon)
  {
   /*
    * 0: Hash,    1: CBuffer Size
    * 2: Enable?, 3: Binding Slot,
    * 4: Offset,  5: Value List Size (in bytes),
    * 6: Value List
    */
    //{ 0x08cc13a6, 52,
    //  false,      3,
    //  0,          4,
    //  { 0.0f }
    //}
    //);

    __SK_D3D11_PixelShader_CBuffer_Overrides->push_back (
    { 0x8e2250f4, 2560, _SK_Y0_Cfg.no_blur0, 0, 224, 512, { 0.f } } );

    blur_shader0 = &__SK_D3D11_PixelShader_CBuffer_Overrides->back ();

    __SK_D3D11_PixelShader_CBuffer_Overrides->push_back (
    { 0xe20da726, 2560, _SK_Y0_Cfg.no_dof0, 0, 1584, 248, { 0.f } } );

    dof_shader0 = &__SK_D3D11_PixelShader_CBuffer_Overrides->back ();

    __SK_D3D11_PixelShader_CBuffer_Overrides->push_back (
    { 0xd9fb983b, 2560, _SK_Y0_Cfg.no_dof1, 0, 1584, 248, { 0.f } } );

    dof_shader1 = &__SK_D3D11_PixelShader_CBuffer_Overrides->back ();

    __SK_D3D11_PixelShader_CBuffer_Overrides->push_back (
    { 0xc0e7b478, 2560, _SK_Y0_Cfg.no_dof2, 0, 1584, 248, { 0.f } } );

    dof_shader2 = &__SK_D3D11_PixelShader_CBuffer_Overrides->back ();

    __SK_D3D11_PixelShader_CBuffer_Overrides->push_back (
    { 0xa6685e36, 2560, _SK_Y0_Cfg.no_dof3, 0, 1584, 248, { 0.f } } );

    dof_shader3 = &__SK_D3D11_PixelShader_CBuffer_Overrides->back ();

    __SK_D3D11_PixelShader_CBuffer_Overrides->push_back (
    { 0x442b7887, 2560, _SK_Y0_Cfg.no_dof4, 0, 1584, 248, { 0.f } } );

    dof_shader4 = &__SK_D3D11_PixelShader_CBuffer_Overrides->back ();

    for (int i = 0; i < (248 / 4); ++i)
    {
      dof_shader0->Values [i] = 0.0f;
      dof_shader1->Values [i] = 0.0f;
      dof_shader2->Values [i] = 0.0f;
      dof_shader3->Values [i] = 0.0f;
      dof_shader4->Values [i] = 0.0f;
    }

    for (int i = 0; i < (512 / 4); ++i)
    {
      blur_shader0->Values[i] = 0.0f;
    }

    if (  dof_shader0->Enable |
          dof_shader1->Enable |
          dof_shader2->Enable |
          dof_shader3->Enable |
          dof_shader4->Enable |
         blur_shader0->Enable )
    {
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
      InterlockedIncrement (&SK_D3D11_CBufferTrackingReqs);
    }
  }

  if (SK_ReShade_SetResolutionScale == nullptr)
  {
    SK_ReShade_SetResolutionScale =
      (SK_ReShade_SetResolutionScale_pfn)SK_GetProcAddress (
        SK_GetModuleHandle (L"ReShade64.dll"),
          "SK_ReShade_SetResolutionScale"
      );
  }
}

bool
SK_Yakuza0_PlugInCfg (void)
{
  static bool yakuza0 =
    SK_GetCurrentGameID () == SK_GAME_ID::Yakuza0;

  static bool yakuza_cant_count =
    SK_GetCurrentGameID () == SK_GAME_ID::YakuzaUnderflow;

  if ( (yakuza0           && ImGui::CollapsingHeader ("Yakuza 0",                  ImGuiTreeNodeFlags_DefaultOpen)) ||
       (yakuza_cant_count && ImGui::CollapsingHeader ("Yakuza: Trouble Counting?", ImGuiTreeNodeFlags_DefaultOpen)) ||
                             ImGui::CollapsingHeader ("Yakuza Kiwami 2",           ImGuiTreeNodeFlags_DefaultOpen |
                                                                                   ImGuiTreeNodeFlags_AllowItemOverlap) )
  {
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.68f, 0.90f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.72f, 0.90f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.78f, 0.87f, 0.80f));

    if (! (yakuza0 || yakuza_cant_count))
    {
      ImGui::SameLine ();

      bool save_face =
        kiwami2_face->face_saved;

      if (ImGui::Checkbox ((const char *)u8"Yakuza Face Saver™", &save_face))
      {
        kiwami2_face->set (save_face);
      }
    }

    ImGui::TreePush ();

    bool leak =
      ImGui::Checkbox ( "Enable (small) Memory Leaks to Prevent Crashing",
                      &__SK_Y0_SafetyLeak );

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip ();
      ImGui::Text         ("Prevent game from deleting memory that is still in use.");
      ImGui::Separator    ();
      ImGui::BulletText   ("Has the potential to cause a slow memory leak over time; still preferable to crashing!");
      ImGui::BulletText   ("This setting is dangerous to change while in-game, do this from the main menu.");
      ImGui::EndTooltip   ();
    }

    if (leak)
    {
      _SK_Y0_Cfg.safety_leak = __SK_Y0_SafetyLeak;
      _SK_Y_SafetyLeak->store (_SK_Y0_Cfg.safety_leak);

      __SK_Yakuza_TrackRTVs =
        __SK_Y0_SafetyLeak;

      config.textures.d3d11.cache = __SK_Y0_SafetyLeak;
    }

    if (! (yakuza0 || yakuza_cant_count))
    {
      extern volatile PVOID __SK_GameBaseAddr;
      static         LPVOID         pBaseAddr =
        ReadPointerAcquire (&__SK_GameBaseAddr);

      bool restart_required = false;
      bool disabled         =
        (  dof_shader0->Enable |
           dof_shader1->Enable |
           dof_shader2->Enable |
           dof_shader3->Enable |
           dof_shader4->Enable |
          blur_shader0->Enable );

      if (ImGui::CollapsingHeader ("Post-Processing", ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::TreePush ();

        //// Debug setting, hide it from normal users.
        //if (config.system.log_level > 0)
        //{
        //  ImGui::Checkbox ("Enable RenderTargetView Tracking", &__SK_Yakuza_TrackRTVs);
        //
        //  if (ImGui::IsItemHovered ())
        //      ImGui::SetTooltip ("High probability of crashing the game during alt-tab, only needed for advanced render mods.");
        //}

            ImGui::BeginGroup ();
        if (ImGui::TreeNodeEx ("Normal Depth of Field", ImGuiTreeNodeFlags_DefaultOpen))
        {   ImGui::Checkbox   ("Disable Pass #0", &dof_shader0->Enable);
            ImGui::Checkbox   ("Disable Pass #2", &dof_shader2->Enable);
            ImGui::Checkbox   ("Disable Pass #3", &dof_shader3->Enable);
            ImGui::TreePop    ();
        }   ImGui::EndGroup   ();
            ImGui::SameLine   ();

        ImGui::BeginGroup (); ImGui::Spacing    ();
        ImGui::SameLine   (); ImGui::Spacing    ();
        ImGui::SameLine   (); ImGui::Spacing    ();
        ImGui::SameLine   (); ImGui::Spacing    ();
        ImGui::SameLine   (); ImGui::Spacing    ();
        ImGui::EndGroup   ();

        ImGui::SameLine   ();
        ImGui::BeginGroup ();
        if (ImGui::TreeNode ("Experimental Depth of Field"))
        {
          auto _Disclaimer =
          [&](void) -> void
           {
             ImGui::SetTooltip ( "NOTE: Only DoF Pass #0, #2 and #3 have "
                                 "been extensively tested, if you disable "
                                 "this pass, weird stuff may happen." );
           };

          ImGui::Checkbox ("Disable Pass #1", &dof_shader1->Enable);

          if (
            ImGui::IsItemHovered ())
                     _Disclaimer ();

            ImGui::Checkbox ("Disable Pass #4", &dof_shader4->Enable);

          if (
            ImGui::IsItemHovered ())
                     _Disclaimer ();

          ImGui::TreePop  ();
        }
        ImGui::EndGroup   ();
        ImGui::TreePop    ();
      }

      ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
      ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
      ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));

      ImGui::BeginGroup   ();

      if (ImGui::CollapsingHeader ("Anti-Aliasing", ImGuiTreeNodeFlags_DefaultOpen |
                                                    ImGuiTreeNodeFlags_AllowItemOverlap))
      {
        ImGui::TreePush   ();
        ImGui::BeginGroup ();

        ImGui::Checkbox   ("Disable Built-In FXAA/SMAA", &blur_shader0->Enable);

        if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ("The provided SMAA preset in ReShade is higher quality.");

        static
        bool non_std = false;
        bool checked =
          ImGui::Checkbox ("Enable Custom SSAA Resolution", &non_std);

        if (ImGui::IsItemHovered ())
            ImGui::SetTooltip    ("Must be applied manually each time game starts.");
        if (checked)
        {
          static bool run_once   = false;
          static bool impossible = false;

          if ((! impossible) && non_std && (! run_once))
          {
            auto orig_se =
            SK_SEH_ApplyTranslator (
              SK_FilteringStructuredExceptionTranslator (
                EXCEPTION_ACCESS_VIOLATION
              )
            );
            try {
                    auto        fp_nop0 = "\x90\x90\x90\x90\x90\x90\x90\x90";
              const uintptr_t addr_nop0 = (uintptr_t)pBaseAddr + 0x6A7E6F;

                    auto        fp_nop1 = "\x90\x90\x90\x90\x90\x90\x90\x90";
              const uintptr_t addr_nop1 = (uintptr_t)pBaseAddr + 0x6A76F6;

                    auto        fp_nop2 = "\x90\x90\x90\x90\x90\x90\x90\x90";
              const uintptr_t addr_nop2 = (uintptr_t)pBaseAddr + 0x6A76DF;

              DWORD dwOrig;

              VirtualProtect ((void *)addr_nop0, 8, PAGE_EXECUTE_READWRITE, &dwOrig);
              memcpy         ((void *)addr_nop0, fp_nop0, 8);
              VirtualProtect ((void *)addr_nop0, 8, dwOrig,                 &dwOrig);

              VirtualProtect ((void *)addr_nop1, 8, PAGE_EXECUTE_READWRITE, &dwOrig);
              memcpy         ((void *)addr_nop1, fp_nop1, 8);
              VirtualProtect ((void *)addr_nop1, 8, dwOrig,                 &dwOrig);

              VirtualProtect ((void *)addr_nop2, 8, PAGE_EXECUTE_READWRITE, &dwOrig);
              memcpy         ((void *)addr_nop2, fp_nop2, 8);
              VirtualProtect ((void *)addr_nop2, 8, dwOrig,                 &dwOrig);
            }
            catch (const SK_SEH_IgnoredException&)
            {
              non_std    = false;
              impossible = true;
            }
            SK_SEH_RemoveTranslator (orig_se);

            run_once = true;
          }

          if (impossible)
            non_std = false;
        }

        if (non_std)
        {
          static float* fp_render_scale =
            (float *)((uintptr_t)pBaseAddr + 0x3F13AB0);
                                          // 0x3F10BC0 Beta 1.1
                                          // 0x3F142C4 Launch

          float fScale =
            ( 100.0f * (*fp_render_scale) );

          ImGui::SameLine ();

          if ( ImGui::SliderFloat (
                 "###YKW2_Custom_Scale",
                   &fScale, 100.f,
                            200.f,
                              "%5.2f%%" )
             )
          {
            *fp_render_scale =
              ( std::max (    25.0f,
                std::min (   300.0f, fScale )
                         ) / 100.0f );
          }

          if (ImGui::IsItemHovered ())
          {   ImGui::SetTooltip    (
                "To make this stick, go into the graphics settings menu and "
                "select one of the Off/FXAA/SMAA settings and press apply."
              );
          }
        }
        ImGui::EndGroup      ( );
        ImGui::TreePop       ( );
      } ImGui::PopStyleColor (3);

      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();

      #define SCEPAD_ENABLEMENT_FILE L"SpecialK.libScePad"

      static bool disable_scepad =
          ( INVALID_FILE_ATTRIBUTES ==
              GetFileAttributesW (SCEPAD_ENABLEMENT_FILE) );

      static bool original_scepad_state =
                   disable_scepad;

      if (ImGui::CollapsingHeader ("Input Management", ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::TreePush ();

        bool toggle_scepad =
          ImGui::Checkbox ("Disable libScePad", &disable_scepad);

        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip ( "If not using a DualShock 4 controller, "
                              "disable this to reduce input processing "
                              "overhead." );
        }

        if (toggle_scepad)
        {
          if (disable_scepad)
          {
            if (! DeleteFileW (SCEPAD_ENABLEMENT_FILE))
            {
              disable_scepad = false;
            }
          }

          else
          {
            FILE* fVirtualFile =
              _wfopen (SCEPAD_ENABLEMENT_FILE, L"w");

            if (fVirtualFile != nullptr)
            {
              fputws (L"My Contents Do Not Matter", fVirtualFile);
              fclose (fVirtualFile);
            }

            else
            {
              disable_scepad = true;
            }
          }
        }

        ImGui::TreePop       ( );
      } ImGui::EndGroup      ( );

      bool disabled_now =
        (  dof_shader0->Enable |
           dof_shader1->Enable |
           dof_shader2->Enable |
           dof_shader3->Enable |
           dof_shader4->Enable |
          blur_shader0->Enable );

      if (disabled_now != disabled)
      {
        if (! disabled_now)
        {
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedDecrement (&SK_D3D11_CBufferTrackingReqs);
        }

        else
        {
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedIncrement (&SK_D3D11_CBufferTrackingReqs);
        }
      }

      // Misnomer -- dof_shader0->Enable means OVERRIDE ENABLE
      _SK_Y0_Cfg.no_blur0 =
        blur_shader0->Enable;
      _SK_Y0_Cfg.no_dof0 =
        dof_shader0->Enable;
      _SK_Y0_Cfg.no_dof1 =
        dof_shader1->Enable;
      _SK_Y0_Cfg.no_dof2 =
        dof_shader2->Enable;
      _SK_Y0_Cfg.no_dof3 =
        dof_shader3->Enable;
      _SK_Y0_Cfg.no_dof4 =
        dof_shader4->Enable;

      _SK_Y_NoBlur0->store (_SK_Y0_Cfg.no_blur0);
      _SK_Y_NoDOF0->store  (_SK_Y0_Cfg.no_dof0);
      _SK_Y_NoDOF1->store  (_SK_Y0_Cfg.no_dof1);
      _SK_Y_NoDOF2->store  (_SK_Y0_Cfg.no_dof2);
      _SK_Y_NoDOF3->store  (_SK_Y0_Cfg.no_dof3);
      _SK_Y_NoDOF4->store  (_SK_Y0_Cfg.no_dof4);

      if (original_scepad_state != disable_scepad)
          restart_required = true;

      if (restart_required)
        ImGui::BulletText ("Game Restart Required");
    }

    bool changed = false;

    if (yakuza0)
    {
      static bool ssao_changed = false;

      ImGui::BeginGroup ();
      changed |= ImGui::Checkbox ("Disable First-Person Blur", &_SK_Y0_Cfg.no_fp_blur);
      changed |= ImGui::Checkbox ("Disable Depth of Field",    &_SK_Y0_Cfg.no_dof);
      changed |= ImGui::Checkbox ("Disable Ambient Occlusion", &_SK_Y0_Cfg.no_ssao);
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();

      static SK_ComPtr <ISimpleAudioVolume> pVolume =
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
    }

    if ((! yakuza_cant_count) &&
        ImGui::CollapsingHeader ("Texture Management"))
    {
      static bool tex_changed = false;

      ImGui::TreePush ();

      bool new_change = false;

      new_change |= ImGui::Checkbox  ("Fix Anisotropic Filtering", &__SK_Y0_FixAniso);
      new_change |= ImGui::Checkbox  ("Clamp LOD Bias",            &__SK_Y0_ClampLODBias);
      new_change |= ImGui::SliderInt ("Force Anisotropic Level",   &__SK_Y0_ForceAnisoLevel, 0, 16);

      if (new_change)
      {
        tex_changed = true;

        _SK_Y0_FixAniso->store     (__SK_Y0_FixAniso);
        _SK_Y0_ClampLODBias->store (__SK_Y0_ClampLODBias);
        _SK_Y0_ForceAniso->store   (__SK_Y0_ForceAnisoLevel);
        SK_GetDLLConfig ()->write  (SK_GetDLLConfig ()->get_filename ());
      }

      if (tex_changed)
      {
        ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (.3f, .8f, .9f));
        ImGui::BulletText     ("Restart Game");
        ImGui::PopStyleColor  ();
      }
      ImGui::TreePop ();
    }

    if (config.steam.screenshots.enable_hook)
    {
      if (ImGui::CollapsingHeader ("Screenshots###YAKUZA_SCREENSHOTS"))
      {
        ImGui::TreePush  ();
        ImGui::PushID    ("Y0_Screenshots");

        auto Keybinding = [] (SK_Keybind* binding, sk::ParameterStringW* param) ->
          auto
        {
          std::string label  = SK_WideCharToUTF8 (binding->human_readable) + "##";
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
          &config.screenshots.game_hud_free_keybind
        };

        ImGui::BeginGroup ();
        for ( auto keybind : keybinds )
        {
          ImGui::Text          ( "%s:  ",
                                keybind->bind_name );
        }
        ImGui::EndGroup   ();
        ImGui::SameLine   ();
        ImGui::BeginGroup ();
        for ( auto keybind : keybinds )
        {
          Keybinding ( keybind, keybind->param );
        }
        ImGui::EndGroup   ();

        bool png_changed = false;

        if (config.steam.screenshots.enable_hook)
        {
          png_changed =
            ImGui::Checkbox ( "Keep Lossless .PNG Screenshots",
                              &config.screenshots.png_compress      );
        }

        auto& rb =
          SK_GetCurrentRenderBackend ();

        if ( rb.screenshot_mgr.getRepoStats ().files > 0 )
        {
          ImGui::SameLine ();

          const SK_ScreenshotManager::screenshot_repository_s& repo =
            rb.screenshot_mgr.getRepoStats (png_changed);

          ImGui::BeginGroup ();
          ImGui::TreePush   ();
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
            SK_ShellExecuteW (
              nullptr,
                L"explore",
                  rb.screenshot_mgr.getBasePath (),
                  nullptr, nullptr,
                    SW_NORMAL
            );
          }

          ImGui::TreePop  ();
          ImGui::EndGroup ();
        }
        ImGui::PopID      ();
        ImGui::TreePop    ();
      }
    }

    if (changed)
    {
      if (yakuza0)
      {
        // SSAO
        if (_SK_Y0_Cfg.no_ssao)
        { SK_D3D11_Shaders->vertex.addTrackingRef (SK_D3D11_Shaders->vertex.blacklist, 0x97837269);
          SK_D3D11_Shaders->vertex.addTrackingRef (SK_D3D11_Shaders->vertex.blacklist, 0x7cc07f78);
          SK_D3D11_Shaders->vertex.addTrackingRef (SK_D3D11_Shaders->vertex.blacklist, 0xe5d4a297);
          SK_D3D11_Shaders->pixel.addTrackingRef  (SK_D3D11_Shaders->vertex.blacklist, 0x4d2973a3);
          SK_D3D11_Shaders->pixel.addTrackingRef  (SK_D3D11_Shaders->vertex.blacklist, 0x0ed648e1);
          SK_D3D11_Shaders->pixel.addTrackingRef  (SK_D3D11_Shaders->vertex.blacklist, 0x170885b9);
          SK_D3D11_Shaders->pixel.addTrackingRef  (SK_D3D11_Shaders->vertex.blacklist, 0x4d2973a3);
          SK_D3D11_Shaders->pixel.addTrackingRef  (SK_D3D11_Shaders->vertex.blacklist, 0x5256777a);
          SK_D3D11_Shaders->pixel.addTrackingRef  (SK_D3D11_Shaders->vertex.blacklist, 0x69b8ef91); }
        else
        { SK_D3D11_Shaders->vertex.releaseTrackingRef (SK_D3D11_Shaders->vertex.blacklist, 0x97837269);
          SK_D3D11_Shaders->vertex.releaseTrackingRef (SK_D3D11_Shaders->vertex.blacklist, 0x7cc07f78);
          SK_D3D11_Shaders->vertex.releaseTrackingRef (SK_D3D11_Shaders->vertex.blacklist, 0xe5d4a297);
          SK_D3D11_Shaders->pixel.releaseTrackingRef  (SK_D3D11_Shaders->pixel.blacklist,  0x4d2973a3);
          SK_D3D11_Shaders->pixel.releaseTrackingRef  (SK_D3D11_Shaders->pixel.blacklist,  0x0ed648e1);
          SK_D3D11_Shaders->pixel.releaseTrackingRef  (SK_D3D11_Shaders->pixel.blacklist,  0x170885b9);
          SK_D3D11_Shaders->pixel.releaseTrackingRef  (SK_D3D11_Shaders->pixel.blacklist,  0x4d2973a3);
          SK_D3D11_Shaders->pixel.releaseTrackingRef  (SK_D3D11_Shaders->pixel.blacklist,  0x5256777a);
          SK_D3D11_Shaders->pixel.releaseTrackingRef  (SK_D3D11_Shaders->pixel.blacklist,  0x69b8ef91); }

        // DOF
        if (_SK_Y0_Cfg.no_dof)
        { SK_D3D11_Shaders->vertex.addTrackingRef (SK_D3D11_Shaders->vertex.blacklist, SK_Y0_DOF_VS_CRC32C);
          SK_D3D11_Shaders->pixel.addTrackingRef  (SK_D3D11_Shaders->pixel.blacklist,  SK_Y0_DOF_PS0_CRC32C);
          SK_D3D11_Shaders->pixel.addTrackingRef  (SK_D3D11_Shaders->pixel.blacklist,  SK_Y0_DOF_PS1_CRC32C);
        }
        else
        { SK_D3D11_Shaders->vertex.releaseTrackingRef (SK_D3D11_Shaders->vertex.blacklist, SK_Y0_DOF_VS_CRC32C);
          SK_D3D11_Shaders->pixel.releaseTrackingRef  (SK_D3D11_Shaders->pixel.blacklist,  SK_Y0_DOF_PS0_CRC32C);
          SK_D3D11_Shaders->pixel.releaseTrackingRef  (SK_D3D11_Shaders->pixel.blacklist,  SK_Y0_DOF_PS1_CRC32C);
        }

        // First Person Blur
        if (_SK_Y0_Cfg.no_fp_blur)
        { SK_D3D11_Shaders->vertex.addTrackingRef (SK_D3D11_Shaders->vertex.blacklist, 0xb008686a);
          SK_D3D11_Shaders->pixel.addTrackingRef  (SK_D3D11_Shaders->pixel.blacklist,  0x1c599fa7); }
        else
        { SK_D3D11_Shaders->vertex.releaseTrackingRef (SK_D3D11_Shaders->vertex.blacklist, 0xb008686a);
          SK_D3D11_Shaders->pixel.releaseTrackingRef  (SK_D3D11_Shaders->pixel.blacklist,  0x1c599fa7); }

        _SK_Y0_NoDOF->store    (_SK_Y0_Cfg.no_dof);
        _SK_Y0_NoSSAO->store   (_SK_Y0_Cfg.no_ssao);
        _SK_Y0_NoFPBlur->store (_SK_Y0_Cfg.no_fp_blur);
      }

      SK_GetDLLConfig   ( )->write (
        SK_GetDLLConfig ( )->get_filename ( )
                                   );
    }

    ImGui::TreePop       ( );
    ImGui::PopStyleColor (3);

    return true;
  }

  return false;
}