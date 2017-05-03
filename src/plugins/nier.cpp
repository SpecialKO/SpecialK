#define _CRT_SECURE_NO_WARNINGS

#include <SpecialK/dxgi_backend.h>
#include <SpecialK/config.h>
#include <SpecialK/command.h>
#include <SpecialK/framerate.h>
#include <SpecialK/ini.h>
#include <SpecialK/parameter.h>
#include <SpecialK/utility.h>
#include <SpecialK/log.h>
#include <SpecialK/steam_api.h>

#include <SpecialK/input/input.h>
#include <SpecialK/input/xinput.h>

#include <SpecialK/hooks.h>
#include <SpecialK/core.h>
#include <process.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_d3d11.h>

#include <atlbase.h>


#define FAR_VERSION_NUM L"0.5.6.2"
#define FAR_VERSION_STR L"FAR v " FAR_VERSION_NUM

// Block until update finishes, otherwise the update dialog
//   will be dismissed as the game crashes when it tries to
//     draw the first frame.
volatile LONG __FAR_init = FALSE;


//#define WORKING_FPS_UNCAP
//#define WORKING_CAMERA_CONTROLS
//#define WORKING_GAMESTATES
#define WORKING_FPS_SLEEP_FIX


struct far_game_state_s {
  // Game state addresses courtesy of Francesco149
  DWORD* pMenu       = (DWORD *)0x1418F39C4;
  DWORD* pLoading    = (DWORD *)0x141975520;
  DWORD* pHacking    = (DWORD *)0x1410E0AB4;
  DWORD* pShortcuts  = (DWORD *)0x1413FC35C;

  float* pHUDOpacity = (float *)0x14196C63C;

  bool   capped      = true;  // Actual state of limiter
  bool   enforce_cap = true;  // User's current preference
  bool   patchable   = false; // True only if the memory addresses can be validated

  bool needFPSCap (void) {
    return enforce_cap; //|| (   *pMenu != 0) || (*pLoading   != 0) ||
                          //(*pHacking != 0) || (*pShortcuts != 0);
  }

  void capFPS   (void);
  void uncapFPS (void);
} static game_state;


sk::ParameterFactory  far_factory;
iSK_INI*              far_prefs                 = nullptr;
wchar_t               far_prefs_file [MAX_PATH] = { L'\0' };
sk::ParameterInt*     far_gi_workgroups         = nullptr;
sk::ParameterInt*     far_bloom_width           = nullptr;
sk::ParameterBool*    far_bloom_disable         = nullptr;
sk::ParameterBool*    far_fix_motion_blur       = nullptr;
sk::ParameterInt*     far_bloom_skip            = nullptr;
sk::ParameterInt*     far_ao_width              = nullptr;
sk::ParameterInt*     far_ao_height             = nullptr;
sk::ParameterBool*    far_ao_disable            = nullptr;
sk::ParameterBool*    far_limiter_busy          = nullptr;
sk::ParameterBool*    far_uncap_fps             = nullptr;
sk::ParameterBool*    far_slow_state_cache      = nullptr;
sk::ParameterBool*    far_rtss_warned           = nullptr;
sk::ParameterBool*    far_osd_disclaimer        = nullptr;
sk::ParameterBool*    far_accepted_license      = nullptr;
sk::ParameterStringW* far_hudless_binding       = nullptr;
sk::ParameterStringW* far_center_lock           = nullptr;
sk::ParameterStringW* far_focus_lock            = nullptr;
sk::ParameterStringW* far_free_look             = nullptr;

struct {
  bool        enqueue = false; // Trigger a Steam screenshot
  int         clear   = 4;     // Reset enqueue after 3 frames
  float       opacity = 1.0f;  // Original HUD opacity

  SK_Keybind  keybind = {
    "HUD Free Screenshot", L"Num -",
     false, false, false, VK_OEM_MINUS
  };
} static __FAR_HUDLESS;


typedef float vec3_t [3]; // X,Z,Y

struct far_cam_state_s {
  SK_Keybind freelook_binding = {
    "Toggle Camera Freelook Mode", L"Num 5",
      false, false, false, VK_NUMPAD5
  };

  // Memory addresses courtesy of Idk31 and Smithfield
  vec3_t* pCamera    = (vec3_t *)0x1415EB950; 
  vec3_t* pLook      = (vec3_t *)0x1415EB960;
  float*  pRoll      = (float *) 0x1415EB990;

  vec3_t  fwd, right, up;

  bool center_lock = false,
       focus_lock  = false;

  SK_Keybind center_binding {
     "Camera Center Lock Toggle", L"Num /",
     true, true, false, VK_DIVIDE
  };

  SK_Keybind focus_binding {
     "Camera Focus Lock Toggle", L"Ctrl+Shift+F11",
     true, true, false, VK_F11
  };

  bool toggleCenterLock (void) {
    if (center_lock) SK_GetCommandProcessor ()->ProcessCommandLine ("mem l 4cdc89 F0111DCC00D290F");
    else             SK_GetCommandProcessor ()->ProcessCommandLine ("mem l 4cdc89 F90909090909090");

    return (center_lock = (! center_lock));
  }

  bool toggleFocusLock (void) {
    if (focus_lock) SK_GetCommandProcessor ()->ProcessCommandLine ("mem l 4cdbc8 850111DD910D290F");
    else            SK_GetCommandProcessor ()->ProcessCommandLine ("mem l 4cdbc8 8590909090909090");

    return (focus_lock = (! focus_lock));
  }
} static far_cam;

bool __FAR_Freelook = false;


#include <unordered_set>
static std::unordered_set <ID3D11Texture2D          *> far_title_textures;
static std::unordered_set <ID3D11ShaderResourceView *> far_title_views;


// (Presumable) Size of compute shader workgroup
int    __FAR_GlobalIllumWorkGroupSize =   128;
bool   __FAR_GlobalIllumCompatMode    =  true;

struct {
  int  width   =    -1; // Set at startup from user prefs, never changed
  bool disable = false;
  int  skip    =     0;

  bool active  = false;

  std::unordered_set <ID3D11Texture2D          *> textures;
  std::unordered_set <ID3D11ShaderResourceView *> views;
} far_bloom;

struct {
  int  width           =    -1; // Set at startup from user prefs, never changed
  int  height          =    -1; // Set at startup from user prefs, never changed

  bool active          = false;

  bool disable         = false;
  bool fix_motion_blur = true;

  std::unordered_set <ID3D11Texture2D          *> textures;
  std::unordered_set <ID3D11ShaderResourceView *> views;
} far_ao;
 
double __FAR_TargetFPS                = 59.94;


extern void
__stdcall
SK_SetPluginName (std::wstring name);


typedef HRESULT (WINAPI *D3D11Dev_CreateBuffer_pfn)(
  _In_           ID3D11Device            *This,
  _In_     const D3D11_BUFFER_DESC       *pDesc,
  _In_opt_ const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_      ID3D11Buffer           **ppBuffer
);
typedef HRESULT (WINAPI *D3D11Dev_CreateShaderResourceView_pfn)(
  _In_           ID3D11Device                     *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC  *pDesc,
  _Out_opt_      ID3D11ShaderResourceView        **ppSRView
);

static D3D11Dev_CreateBuffer_pfn             D3D11Dev_CreateBuffer_Original;
static D3D11Dev_CreateShaderResourceView_pfn D3D11Dev_CreateShaderResourceView_Original;

extern
HRESULT
WINAPI
D3D11Dev_CreateBuffer_Override (
  _In_           ID3D11Device            *This,
  _In_     const D3D11_BUFFER_DESC       *pDesc,
  _In_opt_ const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_      ID3D11Buffer           **ppBuffer );

extern
HRESULT
WINAPI
D3D11Dev_CreateShaderResourceView_Override (
  _In_           ID3D11Device                     *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC  *pDesc,
  _Out_opt_      ID3D11ShaderResourceView        **ppSRView );

extern
void
WINAPI
D3D11_DrawIndexedInstanced_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation,
  _In_ UINT                 StartInstanceLocation );

extern
void
WINAPI
D3D11_DrawIndexedInstancedIndirect_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs );

extern
void
WINAPI
D3D11_DrawInstanced_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartVertexLocation,
  _In_ UINT                 StartInstanceLocation );

extern
void
WINAPI
D3D11_DrawInstancedIndirect_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs );


// Was threaded originally, but it is important to block until
//   the update check completes.
unsigned int
__stdcall
SK_FAR_CheckVersion (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  extern bool
  __stdcall
  SK_FetchVersionInfo (const wchar_t* wszProduct);

  extern HRESULT
  __stdcall
  SK_UpdateSoftware   (const wchar_t* wszProduct);

  if (SK_FetchVersionInfo (L"FAR"))
    SK_UpdateSoftware (L"FAR");

  return 0;
}

HRESULT
WINAPI
SK_FAR_CreateBuffer (
  _In_           ID3D11Device            *This,
  _In_     const D3D11_BUFFER_DESC       *pDesc,
  _In_opt_ const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_      ID3D11Buffer           **ppBuffer )
{
  // Global Illumination (DrDaxxy)
  if ( pDesc != nullptr && pDesc->StructureByteStride == 96 &&
                           pDesc->ByteWidth           == 96 * 128 )
  {
    D3D11_BUFFER_DESC new_desc = *pDesc;

    new_desc.ByteWidth = 96 * __FAR_GlobalIllumWorkGroupSize;

    return D3D11Dev_CreateBuffer_Original (This, &new_desc, pInitialData, ppBuffer);
  }

  return D3D11Dev_CreateBuffer_Original (This, pDesc, pInitialData, ppBuffer);
}

HRESULT
WINAPI
SK_FAR_CreateShaderResourceView (
  _In_           ID3D11Device                     *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC  *pDesc,
  _Out_opt_      ID3D11ShaderResourceView        **ppSRView )
{
  // Global Illumination (DrDaxxy)
  if ( pDesc != nullptr && pDesc->ViewDimension        == D3D_SRV_DIMENSION_BUFFEREX &&
                           pDesc->BufferEx.NumElements == 128 )
  {
    if (! __FAR_GlobalIllumCompatMode)
      return D3D11Dev_CreateShaderResourceView_Original (This, pResource, pDesc, ppSRView);

    CComPtr <ID3D11Buffer> pBuf;

    if ( SUCCEEDED (
           pResource->QueryInterface (__uuidof (ID3D11Buffer), (void **)&pBuf)
         )
       )
    {
      D3D11_SHADER_RESOURCE_VIEW_DESC new_desc = *pDesc;
      D3D11_BUFFER_DESC               buf_desc;

      pBuf->GetDesc (&buf_desc);

      if (buf_desc.ByteWidth == 96 * __FAR_GlobalIllumWorkGroupSize)
        new_desc.BufferEx.NumElements = __FAR_GlobalIllumWorkGroupSize;

      return D3D11Dev_CreateShaderResourceView_Original (This, pResource, &new_desc, ppSRView);
    }
  }


  HRESULT hr =
    D3D11Dev_CreateShaderResourceView_Original (This, pResource, pDesc, ppSRView);


  
  if (SUCCEEDED (hr) && pDesc != nullptr && ppSRView != nullptr)
  {
    // Bloom
    if (far_bloom.textures.count ((ID3D11Texture2D *)pResource))
      far_bloom.views.emplace (*ppSRView);

    // AO
    else if (far_ao.textures.count ((ID3D11Texture2D *)pResource))
      far_ao.views.emplace (*ppSRView);

    // Title Screen
    else if (far_title_textures.count ((ID3D11Texture2D *)pResource))
      far_title_views.emplace (*ppSRView);
  }


  return hr;
}


enum class SK_FAR_WaitBehavior
{
  Sleep = 0x1,
  Busy  = 0x2
};

SK_FAR_WaitBehavior wait_behavior (SK_FAR_WaitBehavior::Sleep);

extern LPVOID __SK_base_img_addr;
extern LPVOID __SK_end_img_addr;

extern void* __stdcall SK_Scan (const uint8_t* pattern, size_t len, const uint8_t* mask);

void
SK_FAR_SetFramerateCap (bool enable)
{
  if (enable)
  {
    game_state.enforce_cap =  false;
    far_uncap_fps->set_value (true);
  } else {
    far_uncap_fps->set_value (false);
    game_state.enforce_cap =  true;
  }
}

// Altimor's FPS cap removal
//
uint8_t* psleep     = (uint8_t *)0x14092E887; // Original pre-patch
uint8_t* pspinlock  = (uint8_t *)0x14092E8CF; // +0x48
uint8_t* pmin_tstep = (uint8_t *)0x140805DEC; // Original pre-patch
uint8_t* pmax_tstep = (uint8_t *)0x140805E18; // +0x2c

bool
SK_FAR_SetLimiterWait (SK_FAR_WaitBehavior behavior)
{
  static uint8_t sleep_wait [] = { 0x7e, 0x08, 0x8b, 0xca, 0xff, 0x15 };
  static uint8_t busy_wait  [] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

  static bool init = false;

  // Square-Enix rarely patches the games they publish, so just search for this pattern and
  //   don't bother to adjust memory addresses... if it's not found using the hard-coded address,
  //     bail-out.
  if (! init)
  {
    init = true;

    if ( (psleep = (uint8_t *)SK_Scan ( sleep_wait, 6, nullptr )) == nullptr )
    {
      dll_log.Log (L"[ FARLimit ]  Could not locate Framerate Limiter Sleep Addr.");
    }
    else {
      psleep += 4;
      dll_log.Log (L"[ FARLimit ]  Scanned Framerate Limiter Sleep Addr.: 0x%p", psleep);
      memcpy      (sleep_wait, psleep, 6);

      pspinlock  = psleep + 0x48;


      uint8_t tstep0      [] = { 0x73, 0x1C, 0xC7, 0x05, 0x00, 0x00 };
      uint8_t tstep0_mask [] = { 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00 };

      pmin_tstep = (uint8_t *)SK_Scan ( tstep0, sizeof tstep0, tstep0_mask );
      pmax_tstep = pmin_tstep + 0x2c;

      dll_log.Log (L"[ FARLimit ]  Scanned Framerate Limiter TStepMin Addr.: 0x%p", pmin_tstep);
    }
  }

  if (psleep == nullptr)
    return false;

  wait_behavior = behavior;

  DWORD dwProtect;
  VirtualProtect (psleep, 6, PAGE_EXECUTE_READWRITE, &dwProtect);

  // Hard coded for now; safe to do this without pausing threads and flushing caches
  //   because the config UI runs from the only thread that the game framerate limits.
  switch (behavior)
  {
    case SK_FAR_WaitBehavior::Busy:
      memcpy (psleep, busy_wait, 6);
      break;

    case SK_FAR_WaitBehavior::Sleep:
      memcpy (psleep, sleep_wait, 6);
      break;
  }

  VirtualProtect (psleep, 6, dwProtect, &dwProtect);

  return true;
}


extern void
STDMETHODCALLTYPE
SK_BeginBufferSwap (void);

extern BOOL
__stdcall
SK_DrawExternalOSD (std::string app_name, std::string text);

typedef void (STDMETHODCALLTYPE *SK_EndFrame_pfn)(void);
static SK_EndFrame_pfn SK_EndFrame_Original = nullptr;

void
STDMETHODCALLTYPE
SK_FAR_EndFrame (void)
{
  static LONGLONG frames_drawn = 0;

  SK_EndFrame_Original ();

  if (far_osd_disclaimer->get_value ())
    SK_DrawExternalOSD ( "FAR", "  Press Ctrl + Shift + O         to toggle In-Game OSD\n"
                                "  Press Ctrl + Shift + Backspace to access In-Game Config Menu\n"
                                "    ( Select + Start on Gamepads )\n\n"
                                "   * This message will go away the first time you actually read it and successfully toggle the OSD.\n" );
  else if (config.system.log_level == 1)
  {
    std::string validation = "";

    if (game_state.needFPSCap () && frames_drawn >= 0)
    {
      validation += "FRAME: ";

      static char szFrameNum [32] = { '\0' };
      snprintf (szFrameNum, 31, "%lli (%c) ", frames_drawn, 'A' + (int)(frames_drawn++ % 26LL) );

      validation += szFrameNum;
    }

    else //if ((! game_state.needFPSCap ()) || frames_drawn < 0)
    {
      // First offense is last offense
      frames_drawn = -1;

      validation += "*** CHEATER ***";
    }

    SK_DrawExternalOSD ( "FAR", validation );
  }

  else if (config.system.log_level > 1)
  {
    std::string state = "";

    if (game_state.needFPSCap ()) {
      state += "< Needs Cap :";

      std::string reasons = "";

      if (*game_state.pLoading)   reasons += " loading ";
      if (*game_state.pMenu)      reasons += " menu ";
      if (*game_state.pHacking)   reasons += " hacking ";
      if (*game_state.pShortcuts) reasons += " shortcuts ";

      state += reasons;
      state += ">";
    }

    if (game_state.capped)
      state += " { Capped }";
    else
      state += " { Uncapped }";

    SK_DrawExternalOSD ( "FAR", state);

    if (frames_drawn > 0)
      frames_drawn = -1;
  }

  else {
    SK_DrawExternalOSD            ( "FAR", "" );

    if (frames_drawn > 0)
      frames_drawn = -1;
  }

  // Prevent patching an altered executable
  if (game_state.patchable)
  {
    if (game_state.needFPSCap () && (! game_state.capped))
    {
      game_state.capFPS ();
      game_state.capped = true;
    }
    
    if ((! game_state.needFPSCap ()) && game_state.capped)
    {
      game_state.uncapFPS ();
      game_state.capped = false;
    }


    if (__FAR_HUDLESS.enqueue)
    {
      if (__FAR_HUDLESS.clear == 4-1)
      {
        // In all truth, I should capture the screenshot myself, but I don't
        //   want to bother with that right now ;)
        SK_SteamAPI_TakeScreenshot ();
        __FAR_HUDLESS.clear--;
      }

      else if (__FAR_HUDLESS.clear <= 0)
      {
        (*game_state.pHUDOpacity) = 
          __FAR_HUDLESS.opacity;

        __FAR_HUDLESS.clear   = 4;
        __FAR_HUDLESS.enqueue = false;
      }

      else
        __FAR_HUDLESS.clear--;
    }
  }

#ifdef WORKING_CAMERA_CONTROLS
  XINPUT_STATE state;
  if (__FAR_Freelook && SK_XInput_PollController (0, &state))
  {
#if 0
#define VEC3_NORM(v) sqrt( (v)[0]*(v)[0] + (v)[1]*(v)[1] + (v)[2]*(v)[2] )
#endif

    float ddX = (state.Gamepad.sThumbLX + 1.0f) / 32768.0f;
    float ddY = (state.Gamepad.sThumbLY + 1.0f) / 32768.0f;

    float ddA = (state.Gamepad.sThumbRX + 1.0f) / 32768.0f;
    float ddB = (state.Gamepad.sThumbRY + 1.0f) / 32768.0f;

    vec3_t pos;     pos     [0] = (*far_cam.pCamera) [0]; pos    [1] = (*far_cam.pCamera) [1]; pos    [2] = (*far_cam.pCamera) [2];
    vec3_t target;  target  [0] = (*far_cam.pLook)   [0]; target [1] = (*far_cam.pLook)   [1]; target [2] = (*far_cam.pLook)   [2];

    vec3_t diff; diff [0] = target [0] - pos [0];
                 diff [1] = target [1] - pos [1];
                 diff [2] = target [2] - pos [2];

    float hypXY = sqrtf (diff [0] * diff [0] + diff [2] * diff [2]);

    float dX, dY, dZ;

    dX = ddX*diff [2]/hypXY;
    dY = ddX*diff [0]/hypXY;

#if 0
    if (ddZ != 0.0f)
    {
      float roll    = far_cam.roll;
      float cosRoll = cosf (roll);

      dX = dX*cosRoll
      dY = dY*cosRoll
      dZ = ddX*sinf(roll)

      (*far_cam.pLook)[1]   = target [1] - dZ;
      (*far_cam.pCamera)[1] = pos    [1] - dZ;
    }
#endif

    (*far_cam.pLook) [0]   = target [0] - dX;
    (*far_cam.pLook) [2]   = target [2] + dY;

    (*far_cam.pCamera) [0] = pos    [0] - dX;
    (*far_cam.pCamera) [2] = pos    [2] + dY;

    pos     [0] = (*far_cam.pCamera) [0]; pos    [1] = (*far_cam.pCamera) [1]; pos    [2] = (*far_cam.pCamera) [2];
    target  [0] = (*far_cam.pLook)   [0]; target [1] = (*far_cam.pLook)   [1]; target [2] = (*far_cam.pLook)   [2];

    diff    [0] = target [0] - pos [0];
    diff    [1] = target [1] - pos [1];
    diff    [2] = target [2] - pos [2];

          hypXY = sqrtf (diff [0] * diff [0] + diff [2] * diff [2]);
    float hypZ  = sqrtf (diff [1] * diff [1] + hypXY * hypXY);

#if 0
    if useZ then
      dX = flySpeed*Xdiff/hypZ
      dY = flySpeed*Ydiff/hypZ
      dZ = flySpeed*Zdiff/hypZ
      if moveDir == "backward" then dZ = -dZ end
      writeFloat(tarZAD, tarZ+dZ)
      writeFloat(camZAD, camZ+dZ)
    else
#endif
    dX = ddY * diff [0] / hypXY;
    dY = ddY * diff [2] / hypXY;
    dZ = ddY * diff [1] / hypXY;
    
    
    (*far_cam.pLook) [0]   = target [0] + dX;
    (*far_cam.pLook) [2]   = target [2] + dY;
    
    (*far_cam.pCamera) [0] = pos    [0] + dX;
    (*far_cam.pCamera) [2] = pos    [2] + dY;
  }
#endif
}


// Sit and spin until the user figures out what an OSD is
//
DWORD
WINAPI
SK_FAR_OSD_Disclaimer (LPVOID user)
{
  while ((volatile bool&)config.osd.show)
    Sleep (66);

  far_osd_disclaimer->set_value (false);
  far_osd_disclaimer->store     ();

  far_prefs->write              (far_prefs_file);

  CloseHandle (GetCurrentThread ());

  return 0;
}


typedef void (CALLBACK *SK_PluginKeyPress_pfn)(
  BOOL Control, BOOL Shift, BOOL Alt,
  BYTE vkCode
);
SK_PluginKeyPress_pfn SK_PluginKeyPress_Original;

#define SK_MakeKeyMask(vKey,ctrl,shift,alt) \
  (UINT)((vKey) | (((ctrl) != 0) <<  9) |   \
                  (((shift)!= 0) << 10) |   \
                  (((alt)  != 0) << 11))

#define SK_ControlShiftKey(vKey) SK_MakeKeyMask ((vKey), true, true, false)

void
CALLBACK
SK_FAR_PluginKeyPress (BOOL Control, BOOL Shift, BOOL Alt, BYTE vkCode)
{
  UINT uiMaskedKeyCode =
    SK_MakeKeyMask (vkCode, Control, Shift, Alt);

  const UINT uiHudlessMask =
    SK_MakeKeyMask ( __FAR_HUDLESS.keybind.vKey,  __FAR_HUDLESS.keybind.ctrl,
                     __FAR_HUDLESS.keybind.shift, __FAR_HUDLESS.keybind.alt );

  const UINT uiLockCenterMask =
    SK_MakeKeyMask ( far_cam.center_binding.vKey,  far_cam.center_binding.ctrl,
                     far_cam.center_binding.shift, far_cam.center_binding.alt );

  const UINT uiLockFocusMask =
    SK_MakeKeyMask ( far_cam.focus_binding.vKey,  far_cam.focus_binding.ctrl,
                     far_cam.focus_binding.shift, far_cam.focus_binding.alt );

  const UINT uiToggleFreelookMask =
    SK_MakeKeyMask ( far_cam.freelook_binding.vKey,  far_cam.freelook_binding.ctrl,
                     far_cam.freelook_binding.shift, far_cam.freelook_binding.alt );

  switch (uiMaskedKeyCode)
  {
#ifdef WORKING_FPS_UNCAP
    case SK_ControlShiftKey (VK_OEM_PERIOD):
      SK_FAR_SetFramerateCap (game_state.enforce_cap);
      break;
#endif

    case SK_ControlShiftKey (VK_OEM_6): // ']'
    {
      if (__FAR_GlobalIllumWorkGroupSize < 8)
        __FAR_GlobalIllumWorkGroupSize = 8;

      __FAR_GlobalIllumWorkGroupSize <<= 1ULL;

      if (__FAR_GlobalIllumWorkGroupSize > 128)
        __FAR_GlobalIllumWorkGroupSize = 128;
    } break;

    case SK_ControlShiftKey (VK_OEM_4): // '['
    {
      if (__FAR_GlobalIllumWorkGroupSize > 128)
        __FAR_GlobalIllumWorkGroupSize = 128;

      __FAR_GlobalIllumWorkGroupSize >>= 1UL;

      if (__FAR_GlobalIllumWorkGroupSize < 16)
        __FAR_GlobalIllumWorkGroupSize = 0;
    } break;

    default:
    {
      if (uiMaskedKeyCode == uiHudlessMask)
      {
        if (__FAR_HUDLESS.enqueue == false)
        {
          __FAR_HUDLESS.clear     = 4;
          __FAR_HUDLESS.enqueue   = true;
          __FAR_HUDLESS.opacity   = (*game_state.pHUDOpacity);
          *game_state.pHUDOpacity = 0.0f;
        }
      }

      else if (uiMaskedKeyCode == uiLockCenterMask)
      {
        far_cam.toggleCenterLock ();
      }

      else if (uiMaskedKeyCode == uiLockFocusMask)
      {
        far_cam.toggleFocusLock ();
      }

      else if (uiMaskedKeyCode == uiToggleFreelookMask)
      {
        __FAR_Freelook = (! __FAR_Freelook);
      }
    } break;
  }

  SK_PluginKeyPress_Original (Control, Shift, Alt, vkCode);
}


HRESULT
STDMETHODCALLTYPE
SK_FAR_PresentFirstFrame (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
  // Wait for the mod to init, it may be held up during version check
  while (! InterlockedAdd (&__FAR_init, 0)) Sleep (16);

  // This actually determines whether the DLL is dxgi.dll or SpecialK64.dll.
  //
  //   If it is the latter, disable this feature -- this prevents nasty
  //     surprises if the plug-in falls out of maintenance.
  //if (! SK_IsInjected ())
  {
    game_state.enforce_cap = (! far_uncap_fps->get_value ());

    bool busy_wait = far_limiter_busy->get_value ();

    game_state.patchable =
      SK_FAR_SetLimiterWait ( busy_wait ? SK_FAR_WaitBehavior::Busy :
                                          SK_FAR_WaitBehavior::Sleep );

    //
    // Hook keyboard input, only necessary for the FPS cap toggle right now
    //
    extern void WINAPI SK_PluginKeyPress (BOOL,BOOL,BOOL,BYTE);
    SK_CreateFuncHook ( L"SK_PluginKeyPress",
                          SK_PluginKeyPress,
                          SK_FAR_PluginKeyPress,
               (LPVOID *)&SK_PluginKeyPress_Original );
    SK_EnableHook        (SK_PluginKeyPress);
  }

  if (GetModuleHandle (L"RTSSHooks64.dll"))
  {
    bool warned = far_rtss_warned->get_value ();

    if (! warned)
    {
      warned = true;
      
      SK_MessageBox ( L"RivaTuner Statistics Server Detected\r\n\r\n\t"
                      L"If FAR does not work correctly, this is probably why.",
                        L"Incompatible Third-Party Software", MB_OK | MB_ICONWARNING );

      far_rtss_warned->set_value (true);
      far_rtss_warned->store     ();
      far_prefs->write           (far_prefs_file);
    }
  }

  // Since people don't read guides, nag them to death...
  if (far_osd_disclaimer->get_value ())
  {
    CreateThread ( nullptr,                 0,
                     SK_FAR_OSD_Disclaimer, nullptr,
                       0x00,                nullptr );
  }

  return S_OK;
}



typedef HRESULT (WINAPI *D3D11Dev_CreateTexture2D_pfn)(
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D
);
typedef void (WINAPI *D3D11_DrawIndexed_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation
);
typedef void (WINAPI *D3D11_Draw_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCount,
  _In_ UINT                 StartVertexLocation
);
typedef void (WINAPI *D3D11_DrawIndexedInstanced_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation,
  _In_ UINT                 StartInstanceLocation
);
typedef void (WINAPI *D3D11_DrawIndexedInstancedIndirect_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs
);
typedef void (WINAPI *D3D11_DrawInstanced_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartVertexLocation,
  _In_ UINT                 StartInstanceLocation
);
typedef void (WINAPI *D3D11_DrawInstancedIndirect_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs
);

typedef void (WINAPI *D3D11_PSSetShaderResources_pfn)(
  _In_     ID3D11DeviceContext             *This,
  _In_     UINT                             StartSlot,
  _In_     UINT                             NumViews,
  _In_opt_ ID3D11ShaderResourceView* const *ppShaderResourceViews
);


static D3D11Dev_CreateTexture2D_pfn           D3D11Dev_CreateTexture2D_Original           = nullptr;
static D3D11_DrawIndexed_pfn                  D3D11_DrawIndexed_Original                  = nullptr;
static D3D11_Draw_pfn                         D3D11_Draw_Original                         = nullptr;
static D3D11_DrawIndexedInstanced_pfn         D3D11_DrawIndexedInstanced_Original         = nullptr;
static D3D11_DrawIndexedInstancedIndirect_pfn D3D11_DrawIndexedInstancedIndirect_Original = nullptr;
static D3D11_DrawInstanced_pfn                D3D11_DrawInstanced_Original                = nullptr;
static D3D11_DrawInstancedIndirect_pfn        D3D11_DrawInstancedIndirect_Original        = nullptr;
static D3D11_PSSetShaderResources_pfn         D3D11_PSSetShaderResources_Original         = nullptr;


extern HRESULT
WINAPI
D3D11Dev_CreateTexture2D_Override (
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D );

extern void
WINAPI
D3D11_DrawIndexed_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation );

extern void
WINAPI
D3D11_Draw_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCount,
  _In_ UINT                 StartVertexLocation );

extern void
WINAPI
D3D11_PSSetShaderResources_Override (
  _In_     ID3D11DeviceContext             *This,
  _In_     UINT                             StartSlot,
  _In_     UINT                             NumViews,
  _In_opt_ ID3D11ShaderResourceView* const *ppShaderResourceViews );


// Overview (Durante):
//
//  The bloom pyramid in Nier:A is built up of 5 buffers, which are sized
//  800x450, 400x225, 200x112, 100x56 and 50x28, regardless of resolution
//  the mismatch between the largest buffer size and the screen resolution (in e.g. 2560x1440 or even 1920x1080)
//  leads to some really ugly artifacts.
//
//  To change this, we need to
//    1) Replace the rendertarget textures in question at their creation point
//    2) Adjust the viewport and some constant shader parameter each time they are rendered to
//
//  Examples here:
//    http://abload.de/img/bloom_defaultjhuq9.jpg 
//    http://abload.de/img/bloom_fixedp7uef.jpg
//
//  Note that there are more low-res 800x450 buffers not yet handled by this, 
//  but which could probably be handled similarly. Primarily, SSAO.

HRESULT
WINAPI
SK_FAR_CreateTexture2D (
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D )
{
  if (ppTexture2D == nullptr)
    return D3D11Dev_CreateTexture2D_Original ( This, pDesc, pInitialData, nullptr );

  static UINT  resW      = far_bloom.width; // horizontal resolution, must be set at application start
  static float resFactor = resW / 1600.0f;  // the factor required to scale to the largest part of the pyramid

  bool bloom = false;
  bool ao    = false;

  switch (pDesc->Format)
  {
    // R11G11B10 float textures of these sizes are part of the BLOOM PYRAMID
    // Note: we do not manipulate the 50x28 buffer
    //    -- it's read by a compute shader and the whole screen white level can be off if it is the wrong size
    case DXGI_FORMAT_R11G11B10_FLOAT:
    {
      if (
              (pDesc->Width == 800 && pDesc->Height == 450)
           || (pDesc->Width == 400 && pDesc->Height == 225)
           || (pDesc->Width == 200 && pDesc->Height == 112)
           || (pDesc->Width == 100 && pDesc->Height == 56) 
           /*|| (pDesc->Width == 50 && pDesc->Height == 28)*/
         )
      {
        static int num_r11g11b10_textures = 0;

        num_r11g11b10_textures++;

        if (num_r11g11b10_textures > far_bloom.skip)
        {
          bloom = true;

          SK_LOG2 ( ( L"Bloom Tex (%lux%lu : %lu)",
                        pDesc->Width, pDesc->Height, pDesc->MipLevels ),
                      L"FAR PlugIn" );

          if (far_bloom.width != -1 && (pDesc->Width != 50 && pDesc->Height != 28))
          {
            D3D11_TEXTURE2D_DESC copy = *pDesc;

            // Scale the upper parts of the pyramid fully
            // and lower levels progressively less
            float pyramidLevelFactor  = (pDesc->Width - 50) / 750.0f;
            float scalingFactor       = 1.0f + (resFactor - 1.0f) * pyramidLevelFactor;

            copy.Width  = (UINT)(copy.Width  * scalingFactor);
            copy.Height = (UINT)(copy.Height * scalingFactor);

            pDesc       = &copy;
          }
        }
      }
    } break;

    // 800x450 R8G8B8A8_UNORM is the buffer used to store the AO result and subsequently blur it
    // 800x450 R32_FLOAT is used to store hierarchical Z information (individual mipmap levels are rendered to)
    //                   and serves as input to the main AO pass
    // 800x450 D24_UNORM_S8_UINT depth/stencil used together with R8G8B8A8_UNORM buffer for something (unclear) later on
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    {
      if (pDesc->Width == 800 && pDesc->Height == 450)
      {
        // Skip the first two textures that match this pattern, they are
        //   not related to AO.
        static int num_r32_textures = 0;

        if (pDesc->Format == DXGI_FORMAT_R32_FLOAT)
          num_r32_textures++;

        if ((! far_ao.fix_motion_blur) || (num_r32_textures > 0))
        {
          ao = true;

          if (far_ao.width != -1)
          {
            SK_LOG1 ( ( L"Mip Levels: %lu, Format: %x, (%x:%x:%x)",
                          pDesc->MipLevels,      pDesc->Format,
                          pDesc->CPUAccessFlags, pDesc->Usage,
                          pDesc->MiscFlags ),
                        L"FAR PlugIn" );

            SK_LOG1 ( ( L"AO Buffer (%lux%lu - Fmt: %x",
                          pDesc->Width, pDesc->Height,
                          pDesc->Format ),
                        L"FAR PlugIn" );

            // set to our display resolution instead
            D3D11_TEXTURE2D_DESC copy = *pDesc;

            copy.Width  = far_ao.width;
            copy.Height = far_ao.height;

            pDesc = &copy;
          }
        }
      }
    } break;
  }


  HRESULT hr = D3D11Dev_CreateTexture2D_Original ( This,
                                                     pDesc, pInitialData,
                                                       ppTexture2D );

#if 0
  //
  // Hash textures so we can track the title texture
  //
  if (SUCCEEDED (hr) && pInitialData != nullptr && ppTexture2D != nullptr)
  {
    extern uint32_t
    __cdecl // Meaningless in x64
    crc32_tex (  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
                 _In_      const D3D11_SUBRESOURCE_DATA *pInitialData,
                 _Out_opt_       size_t                 *pSize,
                 _Out_opt_       uint32_t               *pLOD0_CRC32 );

    uint32_t checksum  = 0;
    uint32_t cache_tag = 0;
    size_t   size      = 0;
    uint32_t top_crc32 = 0x00;

    checksum = crc32_tex (pDesc, pInitialData, &size, &top_crc32);

    if ( checksum == 0x713B879E ||
         checksum == 0x013F2718 )
    {
      SK_LOG2 ( ( L"Title Texture (%x) : ID3D11Texture2D (%ph)",
                    checksum, *ppTexture2D ),
                  L"FAR PlugIn" );

      far_title_textures.emplace (*ppTexture2D);
    }
  }
#endif

  if (SUCCEEDED (hr))
  {
    if (bloom)
      far_bloom.textures.emplace (*ppTexture2D);
    if (ao)
      far_ao.textures.emplace (*ppTexture2D);
  }

  return hr;
}


// High level description:
//
//  IF we have 
//   - 1 viewport
//   - with the size of one of the 4 elements of the pyramid we changed
//   - and a primary rendertarget of type R11G11B10
//   - which is associated with a texture of a size different from the viewport
//  THEN
//   - set the viewport to the texture size
//   - adjust the pixel shader constant buffer in slot #12 to this format (4 floats):
//     [ 0.5f / W, 0.5f / H, W, H ] (half-pixel size and total dimensions)
bool
SK_FAR_PreDraw (ID3D11DeviceContext* pDevCtx)
{
  if (far_bloom.active)
  {
    far_bloom.active = false;

    //if (far_bloom.disable)
      //return true;
  }

  if (far_ao.active)
  {
    far_ao.active = false;

    //if (far_ao.disable)
      //return true;
  }

  UINT numViewports = 0;

  pDevCtx->RSGetViewports (&numViewports, nullptr);

  if (numViewports == 1 && (far_bloom.width != -1 || far_ao.width != -1))
  {
    D3D11_VIEWPORT vp;

    pDevCtx->RSGetViewports (&numViewports, &vp);

    if (  (vp.Width == 800 && vp.Height == 450)
       || (vp.Width == 400 && vp.Height == 225)
       || (vp.Width == 200 && vp.Height == 112)
       || (vp.Width == 100 && vp.Height == 56 )
       || (vp.Width == 50  && vp.Height == 28 )
       || (vp.Width == 25  && vp.Height == 14 )
       )
    {
      CComPtr <ID3D11RenderTargetView> rtView = nullptr;

      pDevCtx->OMGetRenderTargets (1, &rtView, nullptr);

      if (rtView)
      {
        D3D11_RENDER_TARGET_VIEW_DESC desc;

        rtView->GetDesc (&desc);

        if ( desc.Format == DXGI_FORMAT_R11G11B10_FLOAT || // Bloom
             desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM  || // AO
             desc.Format == DXGI_FORMAT_R32_FLOAT )        // AO
        {
          CComPtr <ID3D11Resource> rt = nullptr;

          rtView->GetResource (&rt);

          if (rt != nullptr)
          {
            CComPtr <ID3D11Texture2D> rttex = nullptr;

            rt->QueryInterface <ID3D11Texture2D> (&rttex);

            if (rttex != nullptr)
            {
              D3D11_TEXTURE2D_DESC texdesc;
              rttex->GetDesc (&texdesc);

              if (texdesc.Width != vp.Width)
              {
                // Here we go!
                // Viewport is the easy part

                vp.Width  = (float)texdesc.Width;
                vp.Height = (float)texdesc.Height;

                // AO
                //   If we are at mip slice N, divide by 2^N
                if (desc.Texture2D.MipSlice > 0)
                {
                  vp.Width  = (float)(texdesc.Width  >> desc.Texture2D.MipSlice);
                  vp.Height = (float)(texdesc.Height >> desc.Texture2D.MipSlice);
                }

                pDevCtx->RSSetViewports (1, &vp);

                // The constant buffer is a bit more difficult

                // We don't want to create a new buffer every frame,
                // but we also can't use the game's because they are read-only
                // this just-in-time initialized map is a rather ugly solution,
                // but it works as long as the game only renders from 1 thread (which it does)
                // NOTE: rather than storing them statically here (basically a global) the lifetime should probably be managed

                CComPtr <ID3D11Device> dev;
                pDevCtx->GetDevice (&dev);

                D3D11_BUFFER_DESC buffdesc;
                buffdesc.ByteWidth           = 16;
                buffdesc.Usage               = D3D11_USAGE_IMMUTABLE;
                buffdesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
                buffdesc.CPUAccessFlags      = 0;
                buffdesc.MiscFlags           = 0;
                buffdesc.StructureByteStride = 16;

                D3D11_SUBRESOURCE_DATA initialdata;

                // Bloom
                //   If we are not rendering to a mip map for hierarchical Z, the format is 
                //   [ 0.5f / W, 0.5f / H, W, H ] (half-pixel size and total dimensions)
                if (desc.Texture2D.MipSlice == 0 && far_bloom.width != -1)
                {
                  static std::map <UINT, ID3D11Buffer*> buffers;

                  auto iter = buffers.find (texdesc.Width);
                  if (iter == buffers.cend ())
                  {
                    SK_LOG3 ( ( L"Create Bloom Buffer (%lu)", texdesc.Width ),
                                L"FAR PlugIn" );

                    float constants [4] = {
                      0.5f / vp.Width, 0.5f / vp.Height,
                      (float)vp.Width, (float)vp.Height
                    };

                    initialdata.pSysMem = constants;

                    ID3D11Buffer                                *replacementbuffer = nullptr;
                    dev->CreateBuffer (&buffdesc, &initialdata, &replacementbuffer);

                    buffers [texdesc.Width] = replacementbuffer;
                  }

                  pDevCtx->PSSetConstantBuffers (12, 1, &buffers [texdesc.Width]);

                  if (far_bloom.disable)
                    return true;
                }

                // AO
                //
                //   For hierarchical Z mips, the format is
                //   [ W, H, LOD (Mip-1), 0.0f ]
                else if (far_ao.width != -1)
                {
                  static std::map <UINT, ID3D11Buffer*> mipBuffers;

                  auto iter = mipBuffers.find (desc.Texture2D.MipSlice);
                  if (iter == mipBuffers.cend ())
                  {
                    SK_LOG3 ( ( L"Create AO Buffer (%lu)", desc.Texture2D.MipSlice ),
                                L"FAR PlugIn" );

                    float constants [4] = {
                                         vp.Width,   vp.Height,
                      (float)desc.Texture2D.MipSlice - 1, 0.0f
                    };

                    initialdata.pSysMem = constants;

                    ID3D11Buffer                                *replacementbuffer = nullptr;
                    dev->CreateBuffer (&buffdesc, &initialdata, &replacementbuffer);

                    mipBuffers [desc.Texture2D.MipSlice] = replacementbuffer;
                  }

                  pDevCtx->PSSetConstantBuffers (8, 1, &mipBuffers [desc.Texture2D.MipSlice]);

                  if (far_ao.disable)
                    return true;
                }
              }
            }
          }
        }
      }
    }
  }

  return false;
}

D3D11_VIEWPORT backup_vp;

void
SK_FAR_RestoreAspectRatio (ID3D11DeviceContext *pDevCtx)
{
  pDevCtx->RSSetViewports (1, &backup_vp);
}

bool
SK_FAR_CorrectAspectRatio (ID3D11DeviceContext *pDevCtx)
{
  return false;

  UINT numViewports = 0;

  pDevCtx->RSGetViewports (&numViewports, nullptr);

  if (numViewports <= 2 && (*game_state.pMenu || *game_state.pLoading))
  {
    D3D11_VIEWPORT vp;

    pDevCtx->RSGetViewports (&numViewports, &vp);

    backup_vp = vp;

    extern HWND __stdcall SK_GetGameWindow (void);

    RECT rect;
    GetClientRect (SK_GetGameWindow (), &rect);

    if ( (INT)vp.Width  == (INT)(rect.right  - rect.left) &&
         (INT)vp.Height == (INT)(rect.bottom - rect.top)  &&
         vp.TopLeftX == 0 && vp.TopLeftY == 0 )
    {
      if (vp.Width / vp.Height < (16.0f / 9.0f - 0.01f))
      {
        float orig  = vp.Height;
        vp.Height   = (9.0f * vp.Width)  / 16.0f;
        vp.TopLeftY = (orig - vp.Height) / 2.0f;
      }

      else if (vp.Width / vp.Height > (16.0f / 9.0f + 0.01f))
      {
        float orig  = vp.Width;
        vp.Width    = (16.0f * vp.Height) / 9.0f;
        vp.TopLeftX = (orig  - vp.Width)  / 2.0f;
      }

      pDevCtx->RSSetViewports (1, &vp);

      return true;
    }
  }

  return false;
}


void
WINAPI
SK_FAR_DrawIndexed (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation )
{
  bool cull   = false;
  bool aspect = false;
 
  if (IndexCount == 4 && StartIndexLocation == 0 && BaseVertexLocation == 0)
    cull = SK_FAR_PreDraw (This);
  else
    aspect = SK_FAR_CorrectAspectRatio (This);

  if (! cull)
    D3D11_DrawIndexed_Original ( This, IndexCount,
                                   StartIndexLocation, BaseVertexLocation );

  if (aspect)
    SK_FAR_RestoreAspectRatio (This);
}

void
WINAPI
SK_FAR_Draw (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCount,
  _In_ UINT                 StartVertexLocation )
{
  bool cull   = false;
  bool aspect = false;

  if (VertexCount == 4 && StartVertexLocation == 0)
    cull = SK_FAR_PreDraw (This);
  else
    aspect = SK_FAR_CorrectAspectRatio (This);

  if (! cull)
    D3D11_Draw_Original ( This, VertexCount,
                            StartVertexLocation );

  if (aspect)
    SK_FAR_RestoreAspectRatio (This);
}


void
WINAPI
SK_FAR_DrawIndexedInstanced (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation,
  _In_ UINT                 StartInstanceLocation )
{
  bool aspect = SK_FAR_CorrectAspectRatio (This);

  D3D11_DrawIndexedInstanced_Original (This, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);

  if (aspect)
    SK_FAR_RestoreAspectRatio (This);
}

void
WINAPI
SK_FAR_DrawIndexedInstancedIndirect (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs )
{
  bool aspect = SK_FAR_CorrectAspectRatio (This);

  D3D11_DrawIndexedInstancedIndirect_Original (This, pBufferForArgs, AlignedByteOffsetForArgs);

  if (aspect)
    SK_FAR_RestoreAspectRatio (This);
}

void
WINAPI
SK_FAR_DrawInstanced (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartVertexLocation,
  _In_ UINT                 StartInstanceLocation )
{
  bool aspect = SK_FAR_CorrectAspectRatio (This);

  D3D11_DrawInstanced_Original (This, VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);

  if (aspect)
    SK_FAR_RestoreAspectRatio (This);
}

void
WINAPI
SK_FAR_DrawInstancedIndirect (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs )
{
  bool aspect = SK_FAR_CorrectAspectRatio (This);

  D3D11_DrawInstancedIndirect_Original (This, pBufferForArgs, AlignedByteOffsetForArgs);

  if (aspect)
    SK_FAR_RestoreAspectRatio (This);
}


void
WINAPI
SK_FAR_PSSetShaderResources (
  _In_     ID3D11DeviceContext             *This,
  _In_     UINT                             StartSlot,
  _In_     UINT                             NumViews,
  _In_opt_ ID3D11ShaderResourceView* const *ppShaderResourceViews )
{
  static ID3D11ShaderResourceView* views [256];

  if (ppShaderResourceViews != nullptr)
  {
    for (UINT i = 0; i < NumViews; i++)
    {
      views [i] = ppShaderResourceViews [i];

      if (far_ao.views.count (ppShaderResourceViews [i]) && far_ao.disable) {
        views [i] = nullptr;
        far_ao.active = true;
      }

      else if (far_bloom.views.count (ppShaderResourceViews [i]) && far_bloom.disable) {
        views [i] = nullptr;
        far_bloom.active = true;
      }
    }
  }

  ppShaderResourceViews = views;

#if 0
  for (int i = 0; i < NumViews; i++)
  {
    if (far_title_views.count (ppShaderResourceViews [i]))
    {
      CComPtr <ID3D11SamplerState> pSamplerState [8] = { nullptr };

      This->PSGetSamplers (0, 8, &pSamplerState [0]);


      static ID3D11SamplerState *nearest_sampler = nullptr;

      if (nearest_sampler == nullptr)
      {
        D3D11_SAMPLER_DESC       sample_desc;
        pSamplerState [0]->GetDesc (&sample_desc);

        sample_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sample_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sample_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sample_desc.Filter   = D3D11_FILTER_MIN_MAG_MIP_POINT;

        CComPtr <ID3D11Device> pDev = nullptr;

        This->GetDevice (&pDev);

        pDev->CreateSamplerState (&sample_desc, &nearest_sampler);
      }


      for (int i = 0; i < 8; i++)
      {
        if (pSamplerState [i] != nullptr)
          This->PSSetSamplers (i, 1, &nearest_sampler);
      }
    }
  }
#endif

  D3D11_PSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);
}




void
SK_FAR_InitPlugin (void)
{
  if (! SK_IsInjected ())
    SK_FAR_CheckVersion (nullptr);

  SK_SetPluginName (FAR_VERSION_STR);


  SK_CreateFuncHook ( L"ID3D11Device::CreateBuffer",
                        D3D11Dev_CreateBuffer_Override,
                          SK_FAR_CreateBuffer,
                            (LPVOID *)&D3D11Dev_CreateBuffer_Original );
  MH_QueueEnableHook (D3D11Dev_CreateBuffer_Override);

  SK_CreateFuncHook ( L"ID3D11Device::CreateShaderResourceView",
                        D3D11Dev_CreateShaderResourceView_Override,
                          SK_FAR_CreateShaderResourceView,
                            (LPVOID *)&D3D11Dev_CreateShaderResourceView_Original );
  MH_QueueEnableHook (D3D11Dev_CreateShaderResourceView_Override);

  SK_CreateFuncHook ( L"ID3D11DeviceContext::PSSetShaderResources",
                        D3D11_PSSetShaderResources_Override,
                          SK_FAR_PSSetShaderResources,
                            (LPVOID *)&D3D11_PSSetShaderResources_Original );
  MH_QueueEnableHook (D3D11_PSSetShaderResources_Override);

  SK_CreateFuncHook ( L"ID3D11Device::CreateTexture2D",
                        D3D11Dev_CreateTexture2D_Override,
                          SK_FAR_CreateTexture2D,
                            (LPVOID *)&D3D11Dev_CreateTexture2D_Original );
  MH_QueueEnableHook (D3D11Dev_CreateTexture2D_Override);

  SK_CreateFuncHook ( L"ID3D11DeviceContext::Draw",
                        D3D11_Draw_Override,
                          SK_FAR_Draw,
                            (LPVOID *)&D3D11_Draw_Original );
  MH_QueueEnableHook (D3D11_Draw_Override);

  SK_CreateFuncHook ( L"ID3D11DeviceContext::DrawIndexed",
                        D3D11_DrawIndexed_Override,
                          SK_FAR_DrawIndexed,
                            (LPVOID *)&D3D11_DrawIndexed_Original );
  MH_QueueEnableHook (D3D11_DrawIndexed_Override);

typedef void (WINAPI *D3D11_DrawInstanced_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartVertexLocation,
  _In_ UINT                 StartInstanceLocation
);
typedef void (WINAPI *D3D11_DrawInstancedIndirect_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs
);

  SK_CreateFuncHook ( L"ID3D11DeviceContext::DrawIndexedInstanced",
                        D3D11_DrawIndexedInstanced_Override,
                          SK_FAR_DrawIndexedInstanced,
                            (LPVOID *)&D3D11_DrawIndexedInstanced_Original );
  MH_QueueEnableHook (D3D11_DrawIndexedInstanced_Override);

  SK_CreateFuncHook ( L"ID3D11DeviceContext::DrawIndexedInstancedIndirect",
                        D3D11_DrawIndexedInstancedIndirect_Override,
                          SK_FAR_DrawIndexedInstancedIndirect,
                            (LPVOID *)&D3D11_DrawIndexedInstancedIndirect_Original );
  MH_QueueEnableHook (D3D11_DrawIndexedInstancedIndirect_Override);

  SK_CreateFuncHook ( L"ID3D11DeviceContext::DrawInstanced",
                        D3D11_DrawInstanced_Override,
                          SK_FAR_DrawInstanced,
                            (LPVOID *)&D3D11_DrawInstanced_Original );
  MH_QueueEnableHook (D3D11_DrawInstanced_Override);

  SK_CreateFuncHook ( L"ID3D11DeviceContext::DrawInstancedIndirect",
                        D3D11_DrawInstancedIndirect_Override,
                          SK_FAR_DrawInstancedIndirect,
                            (LPVOID *)&D3D11_DrawInstancedIndirect_Original );
  MH_QueueEnableHook (D3D11_DrawInstancedIndirect_Override);


  if (far_prefs == nullptr)
  {
    lstrcatW (far_prefs_file, SK_GetConfigPath ());
    lstrcatW (far_prefs_file, L"FAR.ini");

    far_prefs = new iSK_INI (far_prefs_file);
    far_prefs->parse ();

    far_gi_workgroups = 
        static_cast <sk::ParameterInt *>
          (far_factory.create_parameter <int> (L"Global Illumination Compute Shader Workgroups"));

    far_gi_workgroups->register_to_ini ( far_prefs,
                                      L"FAR.Lighting",
                                        L"GlobalIlluminationWorkgroups" );

    if (far_gi_workgroups->load ())
      __FAR_GlobalIllumWorkGroupSize = far_gi_workgroups->get_value ();

    far_gi_workgroups->set_value (__FAR_GlobalIllumWorkGroupSize);
    far_gi_workgroups->store     ();


    far_limiter_busy = 
        static_cast <sk::ParameterBool *>
          (far_factory.create_parameter <bool> (L"Favor Busy-Wait For Better Timing"));

    far_limiter_busy->register_to_ini ( far_prefs,
                                      L"FAR.FrameRate",
                                        L"UseBusyWait" );

    if (! far_limiter_busy->load ())
    {
      // Enable by default, most people should have enough CPU cores for this
      //   policy to be practical.
      far_limiter_busy->set_value (true);
      far_limiter_busy->store     ();
    }

    far_uncap_fps =
        static_cast <sk::ParameterBool *>
          (far_factory.create_parameter <bool> (L"Bypass game's framerate ceiling"));

    far_uncap_fps->register_to_ini ( far_prefs,
                                       L"FAR.FrameRate",
                                         L"UncapFPS" );

    // Disable by default, needs more testing :)
    if (! far_uncap_fps->load ())
    {
      far_uncap_fps->set_value (false);
      far_uncap_fps->store     ();
    }

#ifndef WORKING_FPS_UNCAP
    // FORCE OFF UNTIL I CAN FIX
    far_uncap_fps->set_value (false);
#endif


    far_rtss_warned = 
        static_cast <sk::ParameterBool *>
          (far_factory.create_parameter <bool> (L"RTSS Warning Issued"));

    far_rtss_warned->register_to_ini ( far_prefs,
                                         L"FAR.Compatibility",
                                           L"WarnedAboutRTSS" );

    if (! far_rtss_warned->load ())
    {
      far_rtss_warned->set_value (false);
      far_rtss_warned->store     ();
    }

    far_slow_state_cache =
      static_cast <sk::ParameterBool *>
        (far_factory.create_parameter <bool> (L"Disable D3D11.1 Interop Stateblocks"));

    far_slow_state_cache->register_to_ini ( far_prefs,
                                              L"FAR.Compatibility",
                                                L"NoD3D11Interop" );

    extern bool SK_DXGI_SlowStateCache;

    if (! far_slow_state_cache->load ())
      SK_DXGI_SlowStateCache = true;
    else
      SK_DXGI_SlowStateCache = far_slow_state_cache->get_value ();

    config.render.dxgi.slow_state_cache = SK_DXGI_SlowStateCache;

    far_slow_state_cache->set_value (SK_DXGI_SlowStateCache);
    far_slow_state_cache->store     ();


    far_osd_disclaimer = 
        static_cast <sk::ParameterBool *>
          (far_factory.create_parameter <bool> (L"OSD Disclaimer Dismissed"));

    far_osd_disclaimer->register_to_ini ( far_prefs,
                                            L"FAR.OSD",
                                              L"ShowDisclaimer" );

    if (! far_osd_disclaimer->load ())
    {
      far_osd_disclaimer->set_value (true);
      far_osd_disclaimer->store     ();
    }


    far_accepted_license = 
        static_cast <sk::ParameterBool *>
          (far_factory.create_parameter <bool> (L"Has accepted the license terms"));

    far_accepted_license->register_to_ini ( far_prefs,
                                              L"FAR.System",
                                                L"AcceptedLicense" );

    if (! far_accepted_license->load ())
    {
      far_accepted_license->set_value (false);
      far_accepted_license->store     ();
    }

    else
      config.imgui.show_eula = (! far_accepted_license->get_value ());


    far_bloom_width =
      static_cast <sk::ParameterInt *>
        (far_factory.create_parameter <int> (L"Width of Bloom Post-Process"));

    far_bloom_width->register_to_ini ( far_prefs,
                                         L"FAR.Lighting",
                                           L"BloomWidth" );

    if (! far_bloom_width->load ())
    {
      far_bloom_width->set_value (-1);
      far_bloom_width->store     (  );
    }

    far_bloom.width = far_bloom_width->get_value ();

    // Bloom Width must be > 0 or -1, never 0!
    if (far_bloom.width <= 0) {
      far_bloom.width =                -1;
      far_bloom_width->set_value (far_bloom.width);
      far_bloom_width->store     (               );
    }


    far_bloom_disable =
      static_cast <sk::ParameterBool *>
        (far_factory.create_parameter <bool> (L"Disable Bloom"));

    far_bloom_disable->register_to_ini ( far_prefs,
                                           L"FAR.Lighting",
                                             L"DisableBloom" );

    if (! far_bloom_disable->load ())
    {
      far_bloom_disable->set_value (false);
      far_bloom_disable->store     ();
    }

    far_bloom.disable = far_bloom_disable->get_value ();


    far_bloom_skip =
      static_cast <sk::ParameterInt *>
        (far_factory.create_parameter <int> (L"Test Texture Skip Factor"));

    far_bloom_skip->register_to_ini ( far_prefs,
                                        L"FAR.Temporary",
                                          L"BloomSkipLevels" );

    if (! far_bloom_skip->load ())
    {
      far_bloom_skip->set_value (0);
      far_bloom_skip->store     ();
    }

    far_bloom.skip = far_bloom_skip->get_value ();


    far_fix_motion_blur =
      static_cast <sk::ParameterBool *>
        (far_factory.create_parameter <bool> (L"Test Fix for Motion Blur"));

    far_fix_motion_blur->register_to_ini ( far_prefs,
                                             L"FAR.Temporary",
                                               L"FixMotionBlur" );

    if (! far_fix_motion_blur->load ())
    {
      far_fix_motion_blur->set_value (true);
      far_fix_motion_blur->store     ();
    }

    far_ao.fix_motion_blur = far_fix_motion_blur->get_value ();


    far_ao_disable =
      static_cast <sk::ParameterBool *>
        (far_factory.create_parameter <bool> (L"Disable AO"));

    far_ao_disable->register_to_ini ( far_prefs,
                                        L"FAR.Lighting",
                                          L"DisableAO" );

    if (! far_ao_disable->load ())
    {
      far_ao_disable->set_value (false);
      far_ao_disable->store     ();
    }

    far_ao.disable = far_ao_disable->get_value ();


    far_ao_width =
      static_cast <sk::ParameterInt *>
        (far_factory.create_parameter <int> (L"Width of AO Post-Process"));

    far_ao_width->register_to_ini ( far_prefs,
                                         L"FAR.Lighting",
                                           L"AOWidth" );

    if (! far_ao_width->load ())
    {
      far_ao_width->set_value (-1);
      far_ao_width->store     (  );
    }

    far_ao.width = far_ao_width->get_value ();

    // AO Width must be > 0 or -1, never 0!
    if (far_ao.width <= 0) {
      far_ao.width =               -1;
      far_ao_width->set_value (far_ao.width);
      far_ao_width->store     (            );
    }

    far_ao_height =
      static_cast <sk::ParameterInt *>
        (far_factory.create_parameter <int> (L"Height of AO Post-Process"));

    far_ao_height->register_to_ini ( far_prefs,
                                       L"FAR.Lighting",
                                         L"AOHeight" );

    if (! far_ao_height->load ())
    {
      far_ao_height->set_value (-1);
      far_ao_height->store     (  );
    }

    far_ao.height = far_ao_height->get_value ();

    // AO Height must be > 0 or -1, never 0!
    if (far_ao.height <= 0) {
      far_ao.height =               -1;
      far_ao_height->set_value (far_ao.height);
      far_ao_height->store     (             );
    }



    auto LoadKeybind =
      [](SK_Keybind* binding, wchar_t* ini_name) ->
        auto
        {
          sk::ParameterStringW* ret =
           static_cast <sk::ParameterStringW *>
            (far_factory.create_parameter <std::wstring> (L"DESCRIPTION HERE"));

          ret->register_to_ini ( far_prefs, L"FAR.Keybinds", ini_name );

          if (! ret->load ())
          {
            binding->parse ();
            ret->set_value (binding->human_readable);
            ret->store     ();
          }

          binding->human_readable = ret->get_value ();
          binding->parse ();

          return ret;
        };

    far_hudless_binding = LoadKeybind (&__FAR_HUDLESS.keybind,    L"HUDFreeScreenshot");
    far_center_lock     = LoadKeybind (&far_cam.center_binding,   L"ToggleCameraCenterLock");
    far_focus_lock      = LoadKeybind (&far_cam.focus_binding,    L"ToggleCameraFocusLock");
    far_free_look       = LoadKeybind (&far_cam.freelook_binding, L"ToggleCameraFreelook");



    SK_CreateFuncHook ( L"SK_BeginBufferSwap", SK_BeginBufferSwap,
                                               SK_FAR_EndFrame,
                                    (LPVOID *)&SK_EndFrame_Original );
    MH_QueueEnableHook (SK_BeginBufferSwap);


    far_prefs->write (far_prefs_file);


    MH_ApplyQueued ();

    SK_GetCommandProcessor ()->AddVariable ("FAR.GIWorkgroups", SK_CreateVar (SK_IVariable::Int,     &__FAR_GlobalIllumWorkGroupSize));
    //SK_GetCommandProcessor ()->AddVariable ("FAR.BusyWait",     SK_CreateVar (SK_IVariable::Boolean, &__FAR_BusyWait));
  }

  InterlockedExchange (&__FAR_init, 1);
}

// Not currently used
bool
WINAPI
SK_FAR_ShutdownPlugin (const wchar_t* backend)
{
  UNREFERENCED_PARAMETER (backend);

  return true;
}


void
__stdcall
SK_FAR_ControlPanel (void)
{
  // Push this to FAR.ini so that mod updates don't repeatedly present the user with a license agreement.
  if ((! config.imgui.show_eula) && (! far_accepted_license->get_value ()))
  {
    far_accepted_license->set_value (true);
    far_accepted_license->store     ();
    far_prefs->write                (far_prefs_file);
  }


  bool changed = false;

  if (ImGui::CollapsingHeader("NieR: Automata", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));
    ImGui::TreePush       ("");

    if (ImGui::CollapsingHeader ("Post-Processing", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush ("");

      bool bloom = (! far_bloom.disable);

      if (ImGui::Checkbox ("Bloom", &bloom))
      {
        far_bloom.disable = (! bloom);
        far_bloom_disable->set_value (far_bloom.disable);
        far_bloom_disable->store     ();

        changed = true;
      }

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("For Debug Purposes ONLY, please leave enabled ;)");


      if (! far_bloom.disable)
      {
        ImGui::TreePush ("");

        int bloom_behavior = (far_bloom_width->get_value () != -1) ? 1 : 0;

        ImGui::BeginGroup ();

        if (ImGui::RadioButton ("Default Bloom Res. (800x450)", &bloom_behavior, 0))
        {
          changed = true;

          far_bloom_width->set_value (-1);
          far_bloom_width->store     ();
        }

        ImGui::SameLine ();

        // 1/4 resolution actually, but this is easier to describe to the end-user
        if (ImGui::RadioButton ("Native Bloom Res.",            &bloom_behavior, 1))
        {
          far_bloom_width->set_value ((int)ImGui::GetIO ().DisplaySize.x);
          far_bloom_width->store     ();

          changed = true;
        }

        if (ImGui::IsItemHovered ()) {
          ImGui::BeginTooltip ();
          ImGui::Text        ("Improve Bloom Quality");
          ImGui::Separator   ();
          ImGui::BulletText  ("Performance Cost is Negligible");
          ImGui::BulletText  ("Changing this setting requires a full application restart");
          ImGui::EndTooltip  ();
        }

        ImGui::EndGroup ();
        ImGui::TreePop  ();
      }


      bool ao = (! far_ao.disable);

      if (ImGui::Checkbox ("Ambient Occlusion", &ao))
      {
        far_ao.disable = (! ao);

        far_ao_disable->set_value (far_ao.disable);
        far_ao_disable->store     ();

        changed = true;
      }

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("For Debug Purposes ONLY, please leave enabled ;)");


      if (! far_ao.disable)
      {
        ImGui::TreePush ("");

        int ao_behavior = (far_ao_width->get_value () != -1) ? 3 : 2;

        ImGui::BeginGroup      ();
        if (ImGui::RadioButton ("Default AO Res.    (800x450)", &ao_behavior, 2))
        {
          changed = true;

          far_ao_width->set_value (-1);
          far_ao_width->store     ();

          far_ao_height->set_value (-1);
          far_ao_height->store     ();
        }

        ImGui::SameLine ();

        // 1/4 resolution actually, but this is easier to describe to the end-user
        if (ImGui::RadioButton ("Native AO Res.   ",            &ao_behavior, 3))
        {
          far_ao_width->set_value  ((int)(ImGui::GetIO ().DisplaySize.x));
          far_ao_width->store      ();

          far_ao_height->set_value ((int)(ImGui::GetIO ().DisplaySize.y));
          far_ao_height->store     ();

          changed = true;
        }

        if (ImGui::IsItemHovered ()) {
          ImGui::BeginTooltip ();
          ImGui::Text        ("Improve AO Quality");
          ImGui::Separator   ();
          ImGui::BulletText  ("Performance Cost is Negligible");
          ImGui::BulletText  ("Changing this setting requires a full application restart");
          ImGui::EndTooltip  ();
        }

        ImGui::EndGroup ();
        ImGui::TreePop  ();
      }

      ImGui::TreePop  ();
    }

    if (ImGui::CollapsingHeader ("Lighting", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush ("");

      int quality = 0;

      if (__FAR_GlobalIllumWorkGroupSize < 16)
        quality = 0;
      else if (__FAR_GlobalIllumWorkGroupSize < 32)
        quality = 1;
      else if (__FAR_GlobalIllumWorkGroupSize < 64)
        quality = 2;
      else if (__FAR_GlobalIllumWorkGroupSize < 128)
        quality = 3;
      else
        quality = 4;

      if ( ImGui::Combo ( "Global Illumination Quality", &quality, "Off (High Performance)\0"
                                                                   "Low\0"
                                                                   "Medium\0"
                                                                   "High\0"
                                                                   "Ultra (Game Default)\0\0", 5 ) )
      {
        changed = true;

        switch (quality)
        {
          case 0:
            __FAR_GlobalIllumWorkGroupSize = 0;
            break;

          case 1:
            __FAR_GlobalIllumWorkGroupSize = 16;
            break;

          case 2:
            __FAR_GlobalIllumWorkGroupSize = 32;
            break;

          case 3:
            __FAR_GlobalIllumWorkGroupSize = 64;
            break;

          default:
          case 4:
            __FAR_GlobalIllumWorkGroupSize = 128;
            break;
        }
      }

      far_gi_workgroups->set_value (__FAR_GlobalIllumWorkGroupSize);
      far_gi_workgroups->store     ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("Global Illumination Simulates Indirect Light Bouncing");
        ImGui::Separator    ();
        ImGui::BulletText   ("Lower quality for better performance, but less realistic lighting in shadows.");
        ImGui::BulletText   ("Please direct thanks for this feature to DrDaxxy ;)");
        ImGui::EndTooltip   ();
      }

      if (__FAR_GlobalIllumWorkGroupSize > 64)
      {
        ImGui::SameLine ();
        ImGui::TextColored (ImVec4 (0.5f, 1.0f, 0.1f, 1.0f), " Adjust this for Performance Boost");
        //ImGui::Checkbox ("Compatibility Mode", &__FAR_GlobalIllumCompatMode);
      }

      ImGui::TreePop ();
    }

    if (ImGui::CollapsingHeader ("Framerate", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush ("");

      bool remove_cap = far_uncap_fps->get_value ();
      bool busy_wait  = (wait_behavior == SK_FAR_WaitBehavior::Busy);

#ifdef WORKING_FPS_UNCAP
      if (ImGui::Checkbox ("Remove 60 FPS Cap  ", &remove_cap))
      {
        changed = true;

        SK_FAR_SetFramerateCap (remove_cap);
        far_uncap_fps->store   ();
      }

      if (ImGui::IsItemHovered ()) {
        ImGui::BeginTooltip ();
        ImGui::Text        ("Can be toggled with "); ImGui::SameLine ();
        ImGui::TextColored (ImVec4 (1.0f, 0.8f, 0.1f, 1.0f), "Ctrl + Shift + .");
        ImGui::Separator   ();
        ImGui::TreePush    ("");
        ImGui::TextColored (ImVec4 (0.9f, 0.9f, 0.9f, 1.0f), "Two things to consider when enabling this");
        ImGui::TreePush    ("");
        ImGui::BulletText  ("The game has no refresh rate setting, edit dxgi.ini to establish fullscreen refresh rate.");
        ImGui::BulletText  ("The mod is pre-configured with a 59.94 FPS framerate limit, adjust accordingly.");
        ImGui::TreePop     ();
        ImGui::TreePop     ();
        ImGui::EndTooltip  ();
      }

      ImGui::SameLine ();
#endif

      if (ImGui::Checkbox ("Use Busy-Wait For Capped FPS", &busy_wait))
      {
        changed = true;

        if (busy_wait)
          SK_FAR_SetLimiterWait (SK_FAR_WaitBehavior::Busy);
        else
          SK_FAR_SetLimiterWait (SK_FAR_WaitBehavior::Sleep);

        far_limiter_busy->set_value (busy_wait);
        far_limiter_busy->store     ();
      }

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Fixes video stuttering, but may cause it during gameplay.");

      ImGui::TreePop ();
    }

#ifdef WORKING_CAMERA_CONTROLS
    if (ImGui::CollapsingHeader ("Camera and HUD"))
    {
      auto Keybinding = [](SK_Keybind* binding, sk::ParameterStringW* param) ->
        auto
        {
          std::string label  = SK_WideCharToUTF8 (binding->human_readable) + "###";
                      label += binding->bind_name;

          if (ImGui::Selectable (label.c_str (), false))
          {
            ImGui::OpenPopup (binding->bind_name);
          }

          std::wstring original_binding = binding->human_readable;

          extern void SK_ImGui_KeybindDialog (SK_Keybind* keybind);
          SK_ImGui_KeybindDialog (binding);

          if (original_binding != binding->human_readable)
          {
            param->set_value (binding->human_readable);
            param->store     ();

            return true;
          }

          return false;
        };

      ImGui::TreePush      ("");
      ImGui::SliderFloat   ("HUD Opacity", game_state.pHUDOpacity, 0.0f, 2.0f);

      ImGui::Text          ("HUD Free Screenshot Keybinding:  "); ImGui::SameLine ();

      changed |= Keybinding (&__FAR_HUDLESS.keybind, far_hudless_binding);

      ImGui::Separator ();

      if (ImGui::Checkbox   ("Lock Camera Origin", &far_cam.center_lock))
      {
        far_cam.center_lock = (! far_cam.center_lock);
        far_cam.toggleCenterLock ();
      }

      ImGui::SameLine       ();
      changed |= Keybinding (&far_cam.center_binding, far_center_lock);

      if (ImGui::Checkbox   ("Lock Camera Focus", &far_cam.focus_lock))
      {
        far_cam.focus_lock = (! far_cam.focus_lock);
        far_cam.toggleFocusLock ();
      }


            ImGui::SameLine ();
      changed |= Keybinding (&far_cam.focus_binding, far_focus_lock);

      ImGui::Checkbox ("Use Gamepad Freelook", &__FAR_Freelook);

      ImGui::SameLine       ();
      changed |= Keybinding (&far_cam.freelook_binding, far_free_look);

      ImGui::Separator ();

        ImGui::Text ( "Origin: (%.3f, %.3f, %.3f) - Look: (%.3f,%.3f,%.3f",
                        ((float *)far_cam.pCamera)[0], ((float *)far_cam.pCamera)[1], ((float *)far_cam.pCamera)[2],
                        ((float *)far_cam.pLook)[0], ((float *)far_cam.pLook)[1], ((float *)far_cam.pLook)[2] );

      ImGui::TreePop        ();
    }
#endif

    ImGui::TreePop       ( );
    ImGui::PopStyleColor (3);
  }

  if (changed)
    far_prefs->write (far_prefs_file);
}

bool
__stdcall
SK_FAR_IsPlugIn (void)
{
  return far_prefs != nullptr;
}


#define mbegin(addr, len)   \
  VirtualProtect (          \
    addr,                   \
    len,                    \
    PAGE_EXECUTE_READWRITE, \
    &old_protect_mask       \
);

#define mend(addr, len)  \
  VirtualProtect (       \
    addr,                \
    len,                 \
    old_protect_mask,    \
    &old_protect_mask    \
);



void
far_game_state_s::uncapFPS (void)
{
  DWORD old_protect_mask;

  SK_FAR_SetLimiterWait (SK_FAR_WaitBehavior::Busy);
  SK::Framerate::GetLimiter ()->set_limit (__FAR_TargetFPS);

  mbegin (pspinlock, 2)
  memset (pspinlock, 0x90, 2);
  mend   (pspinlock, 2)

  mbegin (pmin_tstep, 1)
  *pmin_tstep = 0xEB;
  mend   (pmin_tstep, 1)

  mbegin (pmax_tstep, 2)
  pmax_tstep [0] = 0x90;
  pmax_tstep [1] = 0xE9;
  mend   (pmax_tstep, 2)
}



void
far_game_state_s::capFPS (void)
{
  DWORD  old_protect_mask;

  if (! far_limiter_busy->get_value ())
    SK_FAR_SetLimiterWait (SK_FAR_WaitBehavior::Sleep);
  else {
    // Save and later restore FPS
    //
    //   Avoid using Speical K's command processor because that
    //     would store this value persistently.
    __FAR_TargetFPS = SK::Framerate::GetLimiter ()->get_limit ();
                      SK::Framerate::GetLimiter ()->set_limit (59.94);
  }

  mbegin (pspinlock, 2)
  pspinlock [0] = 0x77;
  pspinlock [1] = 0x9F;
  mend   (pspinlock, 2)

  mbegin (pmin_tstep, 1)
  *pmin_tstep = 0x73;
  mend   (pmin_tstep, 1)

  mbegin (pmax_tstep, 2)
  pmax_tstep [0] = 0x0F;
  pmax_tstep [1] = 0x86;
  mend   (pmax_tstep, 2)
}


#undef mbegin
#undef mend