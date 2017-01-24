// ImGui - standalone example application for DirectX 9
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include <imgui/imgui.h>
#include <imgui/backends/imgui_d3d9.h>
#include <d3d9.h>

#include <SpecialK/config.h>
#include <SpecialK/DLL_VERSION.H>
#include <SpecialK/command.h>
#include <SpecialK/console.h>


extern bool SK_ImGui_Visible;


__declspec (dllexport)
bool
SK_ImGui_ControlPanel (void)
{
  ImGuiIO& io =
    ImGui::GetIO ();

  ImGui::SetNextWindowPosCenter       (ImGuiSetCond_Always);
  ImGui::SetNextWindowSizeConstraints (ImVec2 (250, 50), ImGui::GetIO ().DisplaySize);

  const char* szTitle = "Special K (v " SK_VERSION_STR_A ") Control Panel";
  bool        open    = true;

  ImGui::Begin (szTitle, &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders);
    //ImGui::CollapsingHeader ("On-Screen Display");
    //ImGui::CollapsingHeader ("Render Features");

    if ( ImGui::CollapsingHeader ("Steam Enhancements", ImGuiTreeNodeFlags_CollapsingHeader | 
                                                        ImGuiTreeNodeFlags_DefaultOpen ) )
    {
      ImGui::Checkbox ("Play Achievement Unlock Sound",      &config.steam.achievements.play_sound);

      if (config.steam.achievements.play_sound) {
        ImGui::SameLine ();

        int i = 0;

        if (! wcsicmp (config.steam.achievements.sound_file.c_str (), L"xbox"))
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

      ImGui::Checkbox ("Take Screenshot on Achievement Unlock", &config.steam.achievements.take_screenshot);

      if (ImGui::TreeNode  ("Achievement Popup"))
      {
        ImGui::Checkbox  ("Draw Popup on Achievement Unlock", &config.steam.achievements.popup.show);
        ImGui::SliderInt ("Popup Duration (msecs)", &config.steam.achievements.popup.duration, 1000UL, 30000UL);
        ImGui::Combo     ( "Popup Corner",          &config.steam.achievements.popup.origin,
                                      "Top-Left\0"
                                      "Top-Right\0"
                                      "Bottom-Left\0"
                                      "Bottom-Right\0\0" );
        ImGui::Checkbox  ("Animate Popup",          &config.steam.achievements.popup.animate);
        //ImGui::SliderFloat ("Inset Percentage",    &config.steam.achievements.popup.inset, 0.0f, 1.0f, "%.3f%%", 0.01f);
        ImGui::TreePop   ();
      }

      extern void
      SK_UnlockSteamAchievement (uint32_t idx);

      if (ImGui::Button ("Test Achievement Unlock"))
        SK_UnlockSteamAchievement (0);

      ImGui::Combo ( "Steam Notification Corner", &config.steam.notify_corner,
                                     "Top-Left\0"
                                     "Top-Right\0"
                                     "Bottom-Left\0"
                                     "Bottom-Right\0"
                                     "Don't Care (use game's default)\0\0" );

      extern volatile LONGLONG SK_SteamAPI_CallbackRunCount;

      ImGui::Text ( "  * SteamAPI Frame: %llu", InterlockedAdd64 (&SK_SteamAPI_CallbackRunCount, 0) );
    }

    //ImGui::CollapsingHeader ("Window Management");
    //ImGui::CollapsingHeader ("Version Control");
  ImGui::End   ();

  return open;
}

#if 0
#include "config.h"

// Data
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp;

extern LRESULT ImGui_ImplDX9_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT
WINAPI
WndProc ( HWND   hWnd,
          UINT   msg,
          WPARAM wParam,
          LPARAM lParam )
{
  if (ImGui_ImplDX9_WndProcHandler (hWnd, msg, wParam, lParam))
    return TRUE;

  switch (msg)
  {
  case WM_SIZE:
    if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
    {
      ImGui_ImplDX9_InvalidateDeviceObjects  ();
      g_d3dpp.BackBufferWidth       = LOWORD (lParam);
      g_d3dpp.BackBufferHeight      = HIWORD (lParam);

      HRESULT hr =
        g_pd3dDevice->Reset (&g_d3dpp);

      if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT (0);

      ImGui_ImplDX9_CreateDeviceObjects ();
    }
    return 0;

  case WM_SYSCOMMAND:
    if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
      return 0;
     break;

  case WM_DESTROY:
    PostQuitMessage (0);
    return           0;
  }

  return DefWindowProc (hWnd, msg, wParam, lParam);
}

#include <string>
#include <vector>


bool show_config      = true;
bool show_test_window = false;

ImVec4 clear_col = ImColor(114, 144, 154);

struct {
  std::vector <const char*> array;
  int                       sel   = 0;
} gamepads;

void
TBFix_PauseGame (bool pause)
{
  extern HMODULE hInjectorDLL;

  typedef void (__stdcall *SK_SteamAPI_SetOverlayState_pfn)(bool active);

  static SK_SteamAPI_SetOverlayState_pfn SK_SteamAPI_SetOverlayState =
    (SK_SteamAPI_SetOverlayState_pfn)
      GetProcAddress ( hInjectorDLL,
                         "SK_SteamAPI_SetOverlayState" );

  SK_SteamAPI_SetOverlayState (pause);
}

int cursor_refs = 0;

void
TBFix_ToggleConfigUI (void)
{
  static int cursor_refs = 0;

  if (config.input.ui.visible) {
    while (ShowCursor (FALSE) > cursor_refs)
      ;
    while (ShowCursor (TRUE) < cursor_refs)
      ;
  }
  else {
    cursor_refs = (ShowCursor (FALSE) + 1);

    if (cursor_refs > 0) {
      while (ShowCursor (FALSE) > 0)
        ;
    }
  }

  config.input.ui.visible = (! config.input.ui.visible);

  if (config.input.ui.pause)
    TBFix_PauseGame (config.input.ui.visible);
}


void
TBFix_GamepadConfigDlg (void)
{
  if (gamepads.array.size () == 0)
  {
    if (GetFileAttributesA ("TBFix_Res\\Gamepads\\") != INVALID_FILE_ATTRIBUTES)
    {
      std::vector <std::string> gamepads_;

      WIN32_FIND_DATAA fd;
      HANDLE           hFind  = INVALID_HANDLE_VALUE;
      int              files  = 0;
      LARGE_INTEGER    liSize = { 0 };

      hFind = FindFirstFileA ("TBFix_Res\\Gamepads\\*", &fd);

      if (hFind != INVALID_HANDLE_VALUE)
      {
        do {
          if ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
               fd.cFileName[0]    != '.' )
          {
            gamepads_.push_back (fd.cFileName);
          }
        } while (FindNextFileA (hFind, &fd) != 0);

        FindClose (hFind);
      }

            char current_gamepad [128] = { '\0' };
      snprintf ( current_gamepad, 127,
                   "%ws",
                     config.input.gamepad.texture_set.c_str () );

      for (int i = 0; i < gamepads_.size (); i++)
      {
        gamepads.array.push_back (
          _strdup ( gamepads_ [i].c_str () )
        );

        if (! stricmp (gamepads.array [i], current_gamepad))
          gamepads.sel = i;
      }
    }
  }

  if (ImGui::BeginPopupModal ("Gamepad Config", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders))
  {
    int orig_sel = gamepads.sel;

    if (ImGui::ListBox ("Gamepad\nIcons", &gamepads.sel, gamepads.array.data (), gamepads.array.size (), 3))
    {
      if (orig_sel != gamepads.sel)
      {
        wchar_t pad[128] = { L'\0' };

        swprintf(pad, L"%hs", gamepads.array[gamepads.sel]);

        config.input.gamepad.texture_set = pad;

        tbf::RenderFix::need_reset.textures = true;
      }

      ImGui::CloseCurrentPopup ();
    }

    ImGui::EndPopup();
  }
}

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

void
TBFix_DrawConfigUI (LPDIRECT3DDEVICE9 pDev = nullptr)
{
  if (pDev != nullptr)
    g_pd3dDevice = pDev;

  ImGui_ImplDX9_NewFrame ();

  //ImGui::SetNextWindowPos             (ImVec2 ( 640, 360), ImGuiSetCond_FirstUseEver);
  ImGui::SetNextWindowPosCenter       (ImGuiSetCond_Always);
  ImGui::SetNextWindowSizeConstraints (ImVec2 (50, 50), ImGui::GetIO ().DisplaySize);

  // 1. Show a simple window
  // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
    ImGui::Begin ("Tales of Berseria \"Fix\" Control Panel", &show_config, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders);

    ImGui::PushItemWidth (ImGui::GetWindowWidth () * 0.65f);

    if (tbf::RenderFix::need_reset.graphics || tbf::RenderFix::need_reset.textures) {
      ImGui::TextColored ( ImVec4 (0.8f, 0.8f, 0.1f, 1.0f),
                             "    You have made changes that will not apply until you change Screen Modes in Graphics Settings,\n"
                             "      or by performing Alt + Tab with the game set to Fullscreen mode." );
    }

    if (ImGui::CollapsingHeader ("Framerate Control", ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_DefaultOpen))
    {
      int limiter =
        config.framerate.replace_limiter ? 1 : 0;

      const char* szLabel = (limiter == 0 ? "Framerate Limiter  (choose something else!)" : "Framerate Limiter");

      ImGui::Combo (szLabel, &limiter, "Namco          (A.K.A. Stutterfest 2017)\0"
                                       "Special K      (Precision Timing For The Win!)\0\0" );

      static float values [120]  = { 0 };
      static int   values_offset =   0;

      values [values_offset] = 1000.0f * ImGui::GetIO ().DeltaTime;
      values_offset = (values_offset + 1) % IM_ARRAYSIZE (values);

      if (limiter != config.framerate.replace_limiter)
      {
        config.framerate.replace_limiter = limiter;

        if (config.framerate.replace_limiter)
          tbf::FrameRateFix::BlipFramerate    ();
        else
          tbf::FrameRateFix::DisengageLimiter ();

        float fZero = 0.0f;
        memset (values, *(reinterpret_cast <DWORD *> (&fZero)), sizeof (float) * 120);
        values_offset = 0;
      }

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

      sprintf ( szAvg,
                  "Avg milliseconds per-frame: %6.3f  (Target: %6.3f)\n"
                  "    Extreme frametimes:      %6.3f min, %6.3f max\n\n\n\n"
                  "Variation:  %8.5f ms  ==>  %.1f FPS  +/-  %3.1f frames",
                    sum / 120.0f, tbf::FrameRateFix::GetTargetFrametime (),
                      min, max, max - min,
                        1000.0f / (sum / 120.0f), (max - min) / (1000.0f / (sum / 120.0f)) );

      ImGui::PlotLines ( "",
                           values,
                             IM_ARRAYSIZE (values),
                               values_offset,
                                 szAvg,
                                   0.0f,
                                     2.0f * tbf::FrameRateFix::GetTargetFrametime (),
                                       ImVec2 (0, 80) );

      ImGui::SameLine ();

      if (! config.framerate.replace_limiter) {
        ImGui::TextColored ( ImVec4 (1.0f, 1.0f, 0.0f, 1.0f),
                               "\n"
                               "\n"
                               " ... working limiters do not resemble EKGs!" );
      } else {
        ImGui::TextColored ( ImVec4 ( 0.2f, 1.0f, 0.2f, 1.0f),
                               "\n"
                               "\n"
                               "This is how a framerate limiter should work." );
      }

      //ImGui::Text ( "Application average %.3f ms/frame (%.1f FPS)",
                      //1000.0f / ImGui::GetIO ().Framerate,
                                //ImGui::GetIO ().Framerate );
    }

    if (ImGui::CollapsingHeader ("Texture Options"))
    {
      if (ImGui::Checkbox ("Dump Textures",    &config.textures.dump))     tbf::RenderFix::need_reset.graphics = true;
      if (ImGui::Checkbox ("Generate Mipmaps", &config.textures.remaster)) tbf::RenderFix::need_reset.graphics = true;

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Eliminates distant texture aliasing");

      if (config.textures.remaster) {
        ImGui::SameLine (150); 

        if (ImGui::Checkbox ("(Uncompressed)", &config.textures.uncompressed)) tbf::RenderFix::need_reset.graphics = true;

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Uses more VRAM, but avoids texture compression artifacts on generated mipmaps.");
      }

      ImGui::SliderFloat ("LOD Bias", &config.textures.lod_bias, -3.0f, config.textures.uncompressed ? 16.0f : 3.0f);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("Controls texture sharpness;  -3 = Sharpest (WILL shimmer),  0 = Neutral,  16 = Laughably blurry");
        ImGui::EndTooltip   ();
      }
    }

#if 0
    if (ImGui::CollapsingHeader ("Shader Options"))
    {
      ImGui::Checkbox ("Dump Shaders", &config.render.dump_shaders);
    }
#endif

    if (ImGui::CollapsingHeader ("Shadow Quality"))
    {
      struct shadow_imp_s
      {
        shadow_imp_s (int scale)
        {
          scale = std::abs (scale);

          if (scale > 3)
            scale = 3;

          radio    = scale;
          last_sel = radio;
        }

        int radio    = 0;
        int last_sel = 0;
      };

      static shadow_imp_s shadows     (config.render.shadow_rescale);
      static shadow_imp_s env_shadows (config.render.env_shadow_rescale);

      ImGui::Combo ("Character Shadow Resolution",     &shadows.radio,     "Normal\0Enhanced\0High\0Ultra\0\0");
      ImGui::Combo ("Environmental Shadow Resolution", &env_shadows.radio, "Normal\0High\0Ultra\0\0");

      ImGui::TextColored ( ImVec4 ( 0.999f, 0.01f, 0.999f, 1.0f),
                             " * Changes to these settings will produce weird results until you change Screen Mode in-game...\n" );

      if (env_shadows.radio != env_shadows.last_sel) {
        config.render.env_shadow_rescale    = env_shadows.radio;
        env_shadows.last_sel                = env_shadows.radio;
        tbf::RenderFix::need_reset.graphics = true;
      }

      if (shadows.radio != shadows.last_sel) {
        config.render.shadow_rescale        = -shadows.radio;
        shadows.last_sel                    =  shadows.radio;
        tbf::RenderFix::need_reset.graphics = true;
      }
    }

    if (ImGui::CollapsingHeader ("Audio Configuration"))
    {
      ImGui::Checkbox ("Enable 7.1 Channel Audio Fix", &config.audio.enable_fix);

      if (config.audio.enable_fix) {
        ImGui::RadioButton ("Stereo",       (int *)&config.audio.channels, 2);
        ImGui::RadioButton ("Quadraphonic", (int *)&config.audio.channels, 4);
        ImGui::RadioButton ("5.1 Surround", (int *)&config.audio.channels, 6);
      }
    }

    if (ImGui::Button ("Gamepad Config"))
      ImGui::OpenPopup ("Gamepad Config");

    TBFix_GamepadConfigDlg ();

    ImGui::SameLine ();

    //if (ImGui::Button ("Special K Config"))
      //show_gamepad_config ^= 1;

    if ( ImGui::Checkbox ("Pause Game While This Menu Is Open", &config.input.ui.pause) )
      TBFix_PauseGame (config.input.ui.pause);

    ImGui::SameLine (500);

    if (ImGui::Selectable ("...", show_test_window))
      show_test_window ^= show_test_window;

    ImGui::End ();

  if (show_test_window)
  {
    //ImGui::SetNextWindowPos (ImVec2 (650, 20), ImGuiSetCond_FirstUseEver);
    //ImGui::ShowTestWindow   (&show_test_window);
  }

  // Rendering
  g_pd3dDevice->SetRenderState (D3DRS_ZENABLE,           false);
  g_pd3dDevice->SetRenderState (D3DRS_ALPHABLENDENABLE,  false);
  g_pd3dDevice->SetRenderState (D3DRS_SCISSORTESTENABLE, false);

  if (pDev == nullptr) {
    D3DCOLOR clear_col_dx =
      D3DCOLOR_RGBA ( (int)(clear_col.x * 255.0f),
                      (int)(clear_col.y * 255.0f),
                      (int)(clear_col.z * 255.0f),
                      (int)(clear_col.w * 255.0f) );

    g_pd3dDevice->Clear (
      0,
        NULL,
          D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
            clear_col_dx,
              1.0f,
                0 );

    
  }

  if (SUCCEEDED (g_pd3dDevice->BeginScene ())) {
    ImGui::Render          ();
    g_pd3dDevice->EndScene ();
  }
}

__declspec (dllexport)
void
CALLBACK
tbt_main (HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
  // Create application window
  WNDCLASSEX wc =
  { 
    sizeof (WNDCLASSEX),
    CS_CLASSDC,
    WndProc,
    0L, 0L,
    GetModuleHandle (nullptr),
    nullptr,
    LoadCursor      (nullptr, IDC_ARROW),
    nullptr, nullptr,
    L"Tales of Berseria Tweak",
    nullptr
  };

  RegisterClassEx (&wc);
  hwnd =
    CreateWindow ( L"Tales of Berseria Tweak", L"Tales of Berseria Tweak",
                   WS_OVERLAPPEDWINDOW,
                   100,  100,
                   1280, 800,
                   NULL, NULL,
                   wc.hInstance,
                   NULL );

  typedef IDirect3D9*
    (STDMETHODCALLTYPE *Direct3DCreate9_pfn)(UINT SDKVersion);

  extern HMODULE hInjectorDLL; // Handle to Special K

  hInjectorDLL = LoadLibrary (L"d3d9.dll");

  Direct3DCreate9_pfn SK_Direct3DCreate9 =
    (Direct3DCreate9_pfn)
      GetProcAddress (hInjectorDLL, "Direct3DCreate9");

  // Initialize Direct3D
  LPDIRECT3D9 pD3D;
  if ((pD3D = SK_Direct3DCreate9 (D3D_SDK_VERSION)) == NULL)
    UnregisterClass (L"Tales of Berseria Tweak", wc.hInstance);

  ZeroMemory (&g_d3dpp, sizeof (g_d3dpp));
  g_d3dpp.Windowed               = TRUE;
  g_d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
  g_d3dpp.BackBufferFormat       = D3DFMT_UNKNOWN;
  g_d3dpp.EnableAutoDepthStencil = TRUE;
  g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
  g_d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_IMMEDIATE;

  // Create the D3DDevice
  if ( pD3D->CreateDevice ( D3DADAPTER_DEFAULT,
                              D3DDEVTYPE_HAL,
                                hwnd,
                                  D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                    &g_d3dpp,
                                      &g_pd3dDevice ) < 0 )
  {
    pD3D->Release   ();
    UnregisterClass (L"Tales of Berseria Tweak", wc.hInstance);
  }

  // Setup ImGui binding
  ImGui_ImplDX9_Init (hwnd, g_pd3dDevice);

  // Load Fonts
  // (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
  //ImGuiIO& io = ImGui::GetIO();
  //io.Fonts->AddFontDefault();
  //io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
  //io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
  //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
  //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
  //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

  // Main loop
  MSG         msg;
  ZeroMemory (&msg, sizeof (msg));

  ShowWindow   (hwnd, SW_SHOWDEFAULT);
  UpdateWindow (hwnd);

  //TBF_LoadConfig();

  while (msg.message != WM_QUIT)
  {
    if (PeekMessage (&msg, NULL, 0U, 0U, PM_REMOVE))
    {
      TranslateMessage (&msg);
      DispatchMessage  (&msg);
      continue;
    }

    TBFix_DrawConfigUI ();

    g_pd3dDevice->Present (NULL, NULL, NULL, NULL);
  }

  ImGui_ImplDX9_Shutdown ();

  if (g_pd3dDevice)
    g_pd3dDevice->Release ();

  if (pD3D)
    pD3D->Release ();

  UnregisterClass (L"Tales of Berseria Tweak", wc.hInstance);
}
#endif

#include <SpecialK/render_backend.h>

__declspec (dllexport)
void
SK_ImGui_Toggle (void);

//
// Hook this to override Special K's GUI
//
__declspec (dllexport)
DWORD
SK_ImGui_DrawFrame (DWORD dwFlags, void* user)
{
  bool d3d9 = false;

  if ((int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::D3D9) {
    d3d9 = true;

    ImGui_ImplDX9_NewFrame ();
  }

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

  if (! keep_open)
    SK_ImGui_Toggle ();

  return 0;
}

__declspec (dllexport)
void
SK_ImGui_Toggle (void)
{
  bool d3d9 = false;

  if ((int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::D3D9) {
    d3d9 = true;

  static int cursor_refs = 0;

  if (SK_ImGui_Visible) {
    while (ShowCursor (FALSE) > cursor_refs)
      ;
    while (ShowCursor (TRUE)  < cursor_refs)
      ;
  }
  else {
    cursor_refs = (ShowCursor (FALSE) + 1);

    if (cursor_refs > 0) {
      while (ShowCursor (FALSE) > 0)
        ;
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
  }
}