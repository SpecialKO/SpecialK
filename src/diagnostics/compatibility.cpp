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

struct IUnknown;
#include <Unknwnbase.h>

#include <Windows.h>

#include <Commctrl.h>
#include <Shlwapi.h>

#pragma comment (linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' "  \
                         "version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df'" \
                         " language='*'\"")


#include <SpecialK/core.h>
#include <SpecialK/config.h>
#include <SpecialK/window.h>
#include <SpecialK/thread.h>
#include <SpecialK/utility.h>
#include <SpecialK/DLL_VERSION.H>
#include <SpecialK/render/backend.h>
#include <SpecialK/injection/injection.h>

#include <SpecialK/diagnostics/modules.h>
#include <SpecialK/diagnostics/load_library.h>

#include <queue>

volatile LONG __SK_TaskDialogActive = FALSE;

HRESULT
CALLBACK
TaskDialogCallback (
  _In_ HWND     hWnd,
  _In_ UINT     uNotification,
  _In_ WPARAM   wParam,
  _In_ LPARAM   lParam,
  _In_ LONG_PTR dwRefData
)
{
  UNREFERENCED_PARAMETER (dwRefData);
  UNREFERENCED_PARAMETER (wParam);

  if (uNotification == TDN_TIMER)
  {
    if (GetForegroundWindow () != hWnd &&
        GetFocus            () != hWnd)
    {
      SK_RealizeForegroundWindow (hWnd);
    }

    SetForegroundWindow (hWnd);
    SetWindowPos        (hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_ASYNCWINDOWPOS | SWP_SHOWWINDOW   |
                         SWP_NOSIZE         | SWP_NOMOVE       |
                         SWP_NOSENDCHANGING   );
    SetActiveWindow     (hWnd);

    SetWindowLongW      (hWnd, GWL_EXSTYLE,
     ( (GetWindowLongW  (hWnd, GWL_EXSTYLE) | (WS_EX_TOPMOST))));
  }

  if (uNotification == TDN_HYPERLINK_CLICKED)
  {
    ShellExecuteW (nullptr, L"open", reinterpret_cast <wchar_t *>(lParam), nullptr, nullptr, SW_SHOW);
    return S_OK;
  }

  // It's important to keep the compatibility menu always on top, far more important than even
  //   the "Always On Top" window style can give us.
  if (uNotification == TDN_DIALOG_CONSTRUCTED)
  {
    InterlockedExchange (&__SK_TaskDialogActive, TRUE);

    SK_RealizeForegroundWindow (hWnd);

    SetForegroundWindow (hWnd);
    SetWindowPos        (hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_ASYNCWINDOWPOS | SWP_SHOWWINDOW   |
                         SWP_NOSIZE         | SWP_NOMOVE       |
                         SWP_NOSENDCHANGING   );
    SetActiveWindow     (hWnd);
    SetFocus            (hWnd);

    SetWindowLongW      (hWnd, GWL_EXSTYLE,
     ( (GetWindowLongW  (hWnd, GWL_EXSTYLE) | (WS_EX_TOPMOST))));
  }

  if (uNotification == TDN_CREATED)
  {
  }

  if (uNotification == TDN_DESTROYED)
  {
    InterlockedExchange   (&__SK_TaskDialogActive, FALSE);
  }

  return S_OK;
}

HRESULT
__stdcall
SK_TaskBoxWithConfirm ( wchar_t* wszMainInstruction,
                        PCWSTR   wszMainIcon,
                        wchar_t* wszContent,
                        wchar_t* wszConfirmation,
                        wchar_t* wszFooter,
                        PCWSTR   wszFooterIcon,
                        wchar_t* wszVerifyText,
                        BOOL*    verify )
{
  // TODO
  UNREFERENCED_PARAMETER (wszConfirmation);

  const bool timer = true;

  int              nButtonPressed = 0;
  TASKDIALOGCONFIG task_config    = {0};

  task_config.cbSize              = sizeof (task_config);
  task_config.hInstance           = __SK_hModHost;
  task_config.hwndParent          =                   nullptr;
  task_config.pszWindowTitle      = L"Special K Compatibility Layer (v " SK_VERSION_STR_W L")";
  task_config.dwCommonButtons     = TDCBF_OK_BUTTON;
  task_config.pButtons            = nullptr;
  task_config.cButtons            = 0;
  task_config.dwFlags             = 0x00;
  task_config.pfCallback          = TaskDialogCallback;
  task_config.lpCallbackData      = 0;

  task_config.pszMainInstruction  = wszMainInstruction;

  task_config.pszMainIcon         = wszMainIcon;
  task_config.pszContent          = wszContent;

  task_config.pszFooterIcon       = wszFooterIcon;
  task_config.pszFooter           = wszFooter;

  task_config.pszVerificationText = wszVerifyText;

  if (verify != nullptr && *verify)
    task_config.dwFlags |= TDF_VERIFICATION_FLAG_CHECKED;

  if (timer)
    task_config.dwFlags |= TDF_CALLBACK_TIMER;

  HRESULT hr =
    TaskDialogIndirect ( &task_config,
                          &nButtonPressed,
                            nullptr,
                              verify );

  return hr;
}

HRESULT
__stdcall
SK_TaskBoxWithConfirmEx ( wchar_t* wszMainInstruction,
                          PCWSTR   wszMainIcon,
                          wchar_t* wszContent,
                          wchar_t* wszConfirmation,
                          wchar_t* wszFooter,
                          PCWSTR   wszFooterIcon,
                          wchar_t* wszVerifyText,
                          BOOL*    verify,
                          wchar_t* wszCommand )
{
  UNREFERENCED_PARAMETER (wszConfirmation);

  const bool timer = true;

  int              nButtonPressed =   0;
  TASKDIALOGCONFIG task_config    = {   };

  task_config.cbSize              = sizeof    (task_config);
  task_config.hInstance           = SK_Modules.HostApp ();
  task_config.hwndParent          =        nullptr;
  task_config.pszWindowTitle      = L"Special K Compatibility Layer (v " SK_VERSION_STR_W L")";
  task_config.dwCommonButtons     = TDCBF_OK_BUTTON;

  TASKDIALOG_BUTTON button;
  button.nButtonID               = 0xdead01ae;
  button.pszButtonText           = wszCommand;

  task_config.pButtons           = &button;
  task_config.cButtons           = 1;

  task_config.dwFlags            = 0x00;
  task_config.dwFlags           |= TDF_USE_COMMAND_LINKS | TDF_SIZE_TO_CONTENT |
                                   TDF_POSITION_RELATIVE_TO_WINDOW;

  task_config.pfCallback         = TaskDialogCallback;
  task_config.lpCallbackData     = 0;

  task_config.pszMainInstruction = wszMainInstruction;

  task_config.pszMainIcon        = wszMainIcon;
  task_config.pszContent         = wszContent;

  task_config.pszFooterIcon      = wszFooterIcon;
  task_config.pszFooter          = wszFooter;

  task_config.pszVerificationText = wszVerifyText;

  if (verify != nullptr && *verify)
    task_config.dwFlags |= TDF_VERIFICATION_FLAG_CHECKED;

  if (timer)
    task_config.dwFlags |= TDF_CALLBACK_TIMER;

  HRESULT hr =
    TaskDialogIndirect ( &task_config,
                          &nButtonPressed,
                            nullptr,
                              verify );

  return hr;
}

enum {
  SK_BYPASS_UNKNOWN    = 0x0,
  SK_BYPASS_ACTIVATE   = 0x1,
  SK_BYPASS_DEACTIVATE = 0x2
};

volatile LONG SK_BypassResult = SK_BYPASS_UNKNOWN;

struct sk_bypass_s {
  BOOL    disable;
  wchar_t wszBlacklist [MAX_PATH];
} __bypass;



LPVOID pfnGetClientRect    = nullptr;
LPVOID pfnGetWindowRect    = nullptr;
LPVOID pfnGetSystemMetrics = nullptr;

std::queue <DWORD> suspended_tids;


DWORD
WINAPI
SK_Bypass_CRT (LPVOID)
{
  SetCurrentThreadDescription (L"[SK] Compatibility Layer Dialog");

  static BOOL     disable      = __bypass.disable;
         wchar_t* wszBlacklist = __bypass.wszBlacklist;

  bool timer = true;

  int              nButtonPressed = 0;
  TASKDIALOGCONFIG task_config    = {0};

  enum {
    BUTTON_INSTALL         = 0,
    BUTTON_UNINSTALL       = 0,
    BUTTON_OK              = 1,
    BUTTON_DISABLE_PLUGINS = 2,
    BUTTON_RESET_CONFIG    = 3,
  };

  // The global injector; local wrapper not installed for the current game
  const TASKDIALOG_BUTTON  buttons_global [] = {  { BUTTON_INSTALL,         L"Install Wrapper DLL" },
                                                  { BUTTON_DISABLE_PLUGINS, L"Disable Plug-Ins"    },
                                                  { BUTTON_RESET_CONFIG,    L"Reset Config"        },
                                               };

  // Wrapper installation in system containing global version of Special K
  const TASKDIALOG_BUTTON  buttons_local  [] = {  { BUTTON_UNINSTALL,       L"Uninstall Wrapper DLL" },
                                                  { BUTTON_DISABLE_PLUGINS, L"Disable Plug-Ins"      },
                                                  { BUTTON_RESET_CONFIG,    L"Reset Config"          },
                                               };

  // Standalone Special K mod; no global injector detected
  const TASKDIALOG_BUTTON  buttons_standalone  [] = {  { BUTTON_UNINSTALL,       L"Uninstall Mod"    },
                                                       { BUTTON_DISABLE_PLUGINS, L"Disable Plug-Ins" },
                                                       { BUTTON_RESET_CONFIG,    L"Reset Config"     },
                                                    };

  bool gl     = false,
       vulkan = false,
       d3d9   = false,
       d3d8   = false,
       dxgi   = false,
       ddraw  = false,
       d3d11  = false,
       glide  = false;

  SK_TestRenderImports (__SK_hModHost, &gl, &vulkan, &d3d9, &dxgi, &d3d11, &d3d8, &ddraw, &glide);

  const bool no_imports = !(gl || vulkan || d3d9 || d3d8 || ddraw || d3d11 || glide);

  auto dgVoodoo_Check = [](void) ->
    bool
    {
      return
        GetFileAttributesA (
          SK_FormatString ( R"(%ws\PlugIns\ThirdParty\dgVoodoo\d3dimm.dll)",
                              std::wstring ( SK_GetDocumentsDir () + LR"(\My Mods\SpecialK)" ).c_str ()
                          ).c_str ()
        ) != INVALID_FILE_ATTRIBUTES;
    };

  auto dgVooodoo_Nag = [&](void) ->
    bool
    {
      while (
        MessageBox (HWND_DESKTOP, L"dgVoodoo is required for Direct3D8 / DirecrtDraw support\r\n\t"
                                  L"Please install its DLLs to 'Documents\\My Mods\\SpecialK\\PlugIns\\ThidParty\\dgVoodoo'",
                                    L"Third-Party Plug-In Required",
                                      MB_ICONSTOP | MB_RETRYCANCEL) == IDRETRY && (! dgVoodoo_Check ())
            ) ;

      return dgVoodoo_Check ();
    };

  const bool has_dgvoodoo = dgVoodoo_Check ();

  const wchar_t* wszAPI = SK_GetBackend ();

  if (config.apis.last_known != SK_RenderAPI::Reserved)
  {
    switch (static_cast <SK_RenderAPI> (config.apis.last_known))
    {
      case SK_RenderAPI::D3D8:
      case SK_RenderAPI::D3D8On11:
        wszAPI = L"d3d8";
        SK_SetDLLRole (DLL_ROLE::D3D8);
        break;
      case SK_RenderAPI::DDraw:
      case SK_RenderAPI::DDrawOn11:
        wszAPI = L"ddraw";
        SK_SetDLLRole (DLL_ROLE::DDraw);
        break;
      case SK_RenderAPI::D3D10:
      case SK_RenderAPI::D3D11:
      case SK_RenderAPI::D3D12:
        wszAPI = L"dxgi";
        SK_SetDLLRole (DLL_ROLE::DXGI);
        break;
      case SK_RenderAPI::D3D9:
      case SK_RenderAPI::D3D9Ex:
        wszAPI = L"d3d9";
        SK_SetDLLRole (DLL_ROLE::D3D9);
        break;
      case SK_RenderAPI::OpenGL:
        wszAPI = L"OpenGL32";
        SK_SetDLLRole (DLL_ROLE::OpenGL);
        break;
      case SK_RenderAPI::Vulkan:
        wszAPI = L"vulkan-1";
        SK_SetDLLRole (DLL_ROLE::Vulkan);
        break;
    }
  }

  static wchar_t wszAPIName [128] = { L"Auto-Detect   " };
       lstrcatW (wszAPIName, L"(");
       lstrcatW (wszAPIName, wszAPI);
       lstrcatW (wszAPIName, L")");

  if (no_imports && config.apis.last_known == SK_RenderAPI::Reserved)
    lstrcatW (wszAPIName, L"  { Import Address Table FAIL -> Detected API May Be Wrong }");

  const TASKDIALOG_BUTTON rbuttons [] = {  { 0, wszAPIName          },
                                           { 1, L"Enable All APIs"  },
#ifndef _WIN64
                                           { 7, L"Direct3D8"        },
                                           { 8, L"DirectDraw"       },
#endif
                                           { 2, L"Direct3D9{Ex}"    },
                                           { 3, L"Direct3D11"       },
#ifdef _WIN64
                                           { 4, L"Direct3D12"       },
#endif
                                           { 5, L"OpenGL"           },
#ifdef _WIN64
                                           { 6, L"Vulkan"           }
#endif
                                       };

  task_config.cbSize              = sizeof (task_config);
  task_config.hInstance           = __SK_hModHost;
  task_config.hwndParent          =        nullptr;
  task_config.pszWindowTitle      = L"Special K Compatibility Layer (v " SK_VERSION_STR_W L")";
  task_config.dwCommonButtons     = TDCBF_OK_BUTTON;
  task_config.pRadioButtons       = rbuttons;
  task_config.cRadioButtons       = ARRAYSIZE (rbuttons);

  if (SK_IsInjected ())
  {
    task_config.pButtons          =            buttons_global;
    task_config.cButtons          = ARRAYSIZE (buttons_global);
  }

  else
  {
    if (SK_HasGlobalInjector ())
    {
      task_config.pButtons        =            buttons_local;
      task_config.cButtons        = ARRAYSIZE (buttons_local);
    }

    else
    {
      task_config.pButtons        =            buttons_standalone;
      task_config.cButtons        = ARRAYSIZE (buttons_standalone);
    }
  }

  task_config.dwFlags             = 0x00;
  task_config.pfCallback          = TaskDialogCallback;
  task_config.lpCallbackData      = 0;
  task_config.nDefaultButton      = TDCBF_OK_BUTTON;

  task_config.pszMainInstruction  = L"Special K Injection Compatibility Options";

  task_config.pszMainIcon         = TD_SHIELD_ICON;
  task_config.pszContent          = L"Compatibility Mode has been Activated by Pressing and "
                                    L"Holding Ctrl + Shift"
                                    L"\n\nUse this menu to troubleshoot problems caused by"
                                    L" the software";

  if (SK_IsInjected ())
  {
    task_config.pszFooterIcon       = TD_INFORMATION_ICON;
    task_config.pszFooter           = L"WARNING: Installing the wrong API wrapper may require "
                                      L"manual recovery; if in doubt, use AUTO.";
    task_config.pszVerificationText = L"Disable Global Injection for this Game";
  }

  else
  {
    task_config.pszFooter           = nullptr;
    task_config.pszVerificationText = nullptr;
  }

  if (__bypass.disable)
  task_config.dwFlags |= TDF_VERIFICATION_FLAG_CHECKED;

  if (timer)
    task_config.dwFlags |= TDF_CALLBACK_TIMER;

  int nRadioPressed = 0;


  HRESULT hr =
    TaskDialogIndirect ( &task_config,
                          &nButtonPressed,
                            &nRadioPressed,
                              SK_IsInjected () ? &disable : nullptr );


  extern iSK_INI* dll_ini;
  const  wchar_t* wszConfigName = L"";

  if (SK_IsInjected ())
    wszConfigName = L"SpecialK";

  else
  {
    wszConfigName = dll_ini->get_filename ();
    //wszConfigName = SK_GetBackend ();
  }

  std::wstring temp_dll = L"";


  SK_LoadConfig (wszConfigName);

  config.apis.d3d9.hook       = false;
  config.apis.d3d9ex.hook     = false;
  config.apis.dxgi.d3d11.hook = false;

  config.apis.OpenGL.hook     = false;

#ifdef _WIN64
  config.apis.dxgi.d3d12.hook = false;
  config.apis.Vulkan.hook     = false;
#else
  config.apis.d3d8.hook       = false;
  config.apis.ddraw.hook      = false;
#endif

  if (SUCCEEDED (hr))
  {
    if (nRadioPressed == 0)
    {
      if (SK_GetDLLRole () & DLL_ROLE::DXGI)
      {
        config.apis.dxgi.d3d11.hook = true;
#ifdef _WIN64
        config.apis.dxgi.d3d12.hook = true;
#endif
      }

      if (SK_GetDLLRole () & DLL_ROLE::D3D9)
      {
        config.apis.d3d9.hook   = true;
        config.apis.d3d9ex.hook = true;
      }

      if (SK_GetDLLRole () & DLL_ROLE::OpenGL)
      {
        config.apis.OpenGL.hook = true;
      }

#ifndef _WIN64
      if (SK_GetDLLRole () & DLL_ROLE::D3D8)
      {
        config.apis.d3d8.hook       = true;
        config.apis.dxgi.d3d11.hook = true;
      }

      if (SK_GetDLLRole () & DLL_ROLE::DDraw)
      {
        config.apis.ddraw.hook      = true;
        config.apis.dxgi.d3d11.hook = true;
      }
#else
#endif
    }

    switch (nRadioPressed)
    {
      case 0:
        if (nButtonPressed == BUTTON_INSTALL)
        {
          if (SK_IsInjected ()) SK_Inject_SwitchToRenderWrapperEx (SK_GetDLLRole ());
          else
          {
            SK_Inject_SwitchToGlobalInjector ();

            temp_dll = SK_UTF8ToWideChar (
                         SK_FormatString ( R"(%ws\My Mods\SpecialK\SpecialK%lu.dll)",
                           SK_GetDocumentsDir ().c_str (),
#ifndef _WIN64
                             32
#else
                             64
#endif
                         )
                      );
            }
        }
        break;

      case 1:
        config.apis.d3d9.hook       = true;
        config.apis.d3d9ex.hook     = true;

        config.apis.dxgi.d3d11.hook = true;

        config.apis.OpenGL.hook     = true;

#ifndef _WIN64
        config.apis.d3d8.hook       = true;
        config.apis.ddraw.hook      = true;
#else
        config.apis.Vulkan.hook     = true;
        config.apis.dxgi.d3d12.hook = true;
#endif

        if (nButtonPressed == BUTTON_INSTALL)
        {
          if (SK_IsInjected ()) SK_Inject_SwitchToRenderWrapperEx  (SK_GetDLLRole ());
          else
          {
            SK_Inject_SwitchToGlobalInjector ();

            temp_dll = SK_UTF8ToWideChar (
                         SK_FormatString ( R"(%ws\My Mods\SpecialK\SpecialK%lu.dll)",
                           SK_GetDocumentsDir ().c_str (),
#ifndef _WIN64
                             32
#else
                             64
#endif
                         )
                      );
            }
        }
        break;

      case 2:
        config.apis.d3d9.hook       = true;
        config.apis.d3d9ex.hook     = true;

        if (nButtonPressed == BUTTON_INSTALL)
        {
          if (SK_IsInjected ())
          {
            //temp_dll = L"d3d9.dll";
            SK_Inject_SwitchToRenderWrapperEx (DLL_ROLE::D3D9);
          }

          else
          {
            temp_dll = SK_UTF8ToWideChar (
                         SK_FormatString ( R"(%ws\My Mods\SpecialK\SpecialK%lu.dll)",
                           SK_GetDocumentsDir ().c_str (),
#ifndef _WIN64
                             32
#else
                             64
#endif
                         )
                      );
            SK_Inject_SwitchToGlobalInjector ();
          }
        }
        break;

      case 3:
        config.apis.dxgi.d3d11.hook = true;

        if (nButtonPressed == BUTTON_INSTALL)
        {
          if (SK_IsInjected ())
          {
            //temp_dll = L"dxgi.dll";
            SK_Inject_SwitchToRenderWrapperEx (DLL_ROLE::DXGI);
          }

          else
          {
            SK_Inject_SwitchToGlobalInjector ();
            temp_dll = SK_UTF8ToWideChar (
                         SK_FormatString ( R"(%ws\My Mods\SpecialK\SpecialK%lu.dll)",
                           SK_GetDocumentsDir ().c_str (),
#ifndef _WIN64
                             32
#else
                             64
#endif
                         )
                      );
          }
        }
        break;

#ifdef _WIN64
      case 4:
        config.apis.dxgi.d3d11.hook = true; // Required by D3D12 code :P
        config.apis.dxgi.d3d12.hook = true;

        if (nButtonPressed == BUTTON_INSTALL)
        {
          if (SK_IsInjected ())
          {
            //temp_dll = L"dxgi.dll";
            SK_Inject_SwitchToRenderWrapperEx (DLL_ROLE::DXGI);
          }
          else
          {
            SK_Inject_SwitchToGlobalInjector ();
            temp_dll = SK_UTF8ToWideChar (
                         SK_FormatString ( R"(%ws\My Mods\SpecialK\SpecialK%lu.dll)",
                           SK_GetDocumentsDir ().c_str (),
#ifndef _WIN64
                             32
#else
                             64
#endif
                         )
                      );
            }
        }
        break;
#endif

      case 5:
        config.apis.OpenGL.hook = true;

        if (nButtonPressed == BUTTON_INSTALL)
        {
          if (SK_IsInjected ())
          {
            //temp_dll = L"OpenGL32.dll";
            SK_Inject_SwitchToRenderWrapperEx (DLL_ROLE::OpenGL);
          }

          else
          {
            SK_Inject_SwitchToGlobalInjector ();
            temp_dll = SK_UTF8ToWideChar (
                         SK_FormatString ( R"(%ws\My Mods\SpecialK\SpecialK%lu.dll)",
                           SK_GetDocumentsDir ().c_str (),
#ifndef _WIN64
                             32
#else
                             64
#endif
                         )
                      );
          }
        }
        break;

#ifdef _WIN64
      case 6:
        config.apis.Vulkan.hook = true;
        break;
#endif

#ifndef _WIN64
      case 7:
        config.apis.dxgi.d3d11.hook = true;  // D3D8 on D3D11 (not native D3D8)
        config.apis.d3d8.hook       = true;

        if (has_dgvoodoo || dgVooodoo_Nag ())
        {
          if (nButtonPressed == BUTTON_INSTALL)
          {
            if (SK_IsInjected ())
            {
              SK_Inject_SwitchToRenderWrapperEx (DLL_ROLE::D3D8);
            }

            else
            {
              SK_Inject_SwitchToGlobalInjector ();
              temp_dll = SK_UTF8ToWideChar (
                           SK_FormatString ( R"(%ws\My Mods\SpecialK\SpecialK%lu.dll)",
                             SK_GetDocumentsDir ().c_str (),
#ifndef _WIN64
                               32
#else
                               64
#endif
                           )
                        );
            }
          }
        }
        break;

      case 8:
        config.apis.dxgi.d3d11.hook = true;  // DDraw on D3D11 (not native DDraw)
        config.apis.ddraw.hook      = true;

        if (nButtonPressed == BUTTON_INSTALL)
        {
          if (has_dgvoodoo || dgVooodoo_Nag ())
          {
            if (SK_IsInjected ())
              SK_Inject_SwitchToRenderWrapperEx (DLL_ROLE::DDraw);
            else
            {
              SK_Inject_SwitchToGlobalInjector ();

              temp_dll = SK_UTF8ToWideChar (
                           SK_FormatString ( R"(%ws\My Mods\SpecialK\SpecialK%lu.dll)",
                             SK_GetDocumentsDir ().c_str (),
#ifndef _WIN64
                               32
#else
                               64
#endif
                           )
                        );
            }
          }
        }
        break;
#endif

      default:
        break;
    }


    if (nButtonPressed == BUTTON_DISABLE_PLUGINS)
    {
      // TEMPORARY: There will be a function to disable plug-ins here, for now
      //              just disable ReShade.
#ifdef _WIN64
      dll_ini->remove_section (L"Import.ReShade64");
      dll_ini->remove_section (L"Import.ReShade64_Custom");
#else
      dll_ini->remove_section (L"Import.ReShade32");
      dll_ini->remove_section (L"Import.ReShade32_Custom");
#endif
      dll_ini->write (dll_ini->get_filename ());
    }

    if (nButtonPressed == BUTTON_RESET_CONFIG)
    {
      std::wstring fname =
        dll_ini->get_filename ();

      delete dll_ini;

      SK_DeleteConfig (fname);
      SK_DeleteConfig (wszConfigName);

      // Hard-code known plug-in config files -- bad (lack of) design.
      //
      SK_DeleteConfig (L"FAR.ini");   SK_DeleteConfig (L"UnX.ini");   SK_DeleteConfig (L"PPrinny.ini");
      SK_DeleteConfig (L"TBFix.ini"); SK_DeleteConfig (L"TSFix.ini"); SK_DeleteConfig (L"TZFix.ini");
    }

    else if (nButtonPressed != BUTTON_OK)
    {
      SK_SaveConfig (wszConfigName);
    }

    if ( nButtonPressed         == BUTTON_INSTALL &&
         no_imports                               &&
         nRadioPressed          == 0 /* Auto */   &&
         config.apis.last_known == SK_RenderAPI::Reserved )
    {
      MessageBoxA   ( HWND_DESKTOP,
                        SK_FormatString ( "API detection may be incorrect, delete '%ws.dll' "
                                          "manually if Special K does not inject "
                                          "itself.", SK_GetBackend () ).c_str (),
                          "Possible API Detection Problems",
                            MB_ICONINFORMATION | MB_OK
                    );
      ShellExecuteW ( HWND_DESKTOP, L"explore", SK_GetHostPath (), nullptr, nullptr, SW_SHOWNORMAL );
    }

    if (disable)
    {
      FILE* fDeny =
        _wfopen (wszBlacklist, L"w");

      if (fDeny != nullptr)
      {
        fputc  ('\0', fDeny);
        fflush (      fDeny);
        fclose (      fDeny);
      }

      InterlockedExchange (&SK_BypassResult, SK_BYPASS_ACTIVATE);
    }

    else
    {
      DeleteFileW         (wszBlacklist);
      InterlockedExchange (&SK_BypassResult, SK_BYPASS_DEACTIVATE);
    }
  }

  if (! temp_dll.length ())
    SK_RestartGame (nullptr);
  else
    SK_RestartGame (temp_dll.c_str ());

  SK_Thread_CloseSelf ();

  ExitProcess (0);
  
  // The old system to try and restart the bootstrapping process
  //   is impractical, restarting the game works better.
  //
  ///else
  ///{
  ///  dll_ini->write (dll_ini->get_filename ());
  ///  SK_SaveConfig  ();
  ///
  ///  SK_EnumLoadedModules (SK_ModuleEnum::PostLoad);
  ///  SK_ResumeThreads     (suspended_tids);
  ///}

  return 0;
}



std::pair <std::queue <DWORD>, BOOL>
__stdcall
SK_BypassInject (void)
{
  lstrcpyW (__bypass.wszBlacklist, SK_GetBlacklistFilename ());

  __bypass.disable =
    (GetFileAttributesW (__bypass.wszBlacklist) != INVALID_FILE_ATTRIBUTES);

  SK_Thread_Create ( SK_Bypass_CRT, nullptr );

  return
    std::make_pair (suspended_tids, __bypass.disable);
}




void
SK_COMPAT_FixUpFullscreen_DXGI (bool Fullscreen)
{
  if (Fullscreen)
  {
    if (SK_GetCurrentGameID () == SK_GAME_ID::WorldOfFinalFantasy)
    {
      ShowCursor  (TRUE);
      ShowWindow  ( GetForegroundWindow (), SW_HIDE );
      MessageBox  ( GetForegroundWindow (),
                      L"Please re-configure this game to run in windowed mode",
                        L"Special K Conflict",
                          MB_OK        | MB_SETFOREGROUND |
                          MB_APPLMODAL | MB_ICONASTERISK );

      ShellExecuteW (HWND_DESKTOP, L"open", L"WOFF_config.exe", nullptr, nullptr, SW_NORMAL);

      while (SK_IsProcessRunning (L"WOFF_config.exe"))
        SleepEx (250UL, FALSE);

      ShellExecuteW (HWND_DESKTOP, L"open", L"WOFF.exe",        nullptr, nullptr, SW_NORMAL);
      ExitProcess   (0x00);
    }
  }
}



#include <SpecialK/sound.h>

#pragma comment (lib,"mmdevapi.lib")

//
// MSI Nahimic sometimes @#$%s the bed if MMDevAPI.dll is not loaded
//   in the right order, so to make it happy, we need a link-time reference
//     to a function in that DLL that won't be optimized away.
//
//   This causes the kernel to load MMDevAPI before Special K and before
//     Nahimic, and now we don't race to be the first to blow up the program.
//
HRESULT
SK_COMPAT_FixNahimicDeadlock (void)
{
  CComPtr <IActivateAudioInterfaceAsyncOperation> aOp;
  
  ActivateAudioInterfaceAsync ( L"I don't exist",
                                  __uuidof (IAudioClient),
                                    nullptr, nullptr, &aOp );
  
  if (GetModuleHandle (L"Nahimic2DevProps.dll"))
    return S_OK;

  return S_FALSE;
}


bool
SK_COMPAT_IsSystemDllInstalled (const wchar_t* wszDll, bool* locally)
{
  if (GetFileAttributesW (wszDll) != INVALID_FILE_ATTRIBUTES)
  {
    if (locally != nullptr) *locally = true;
  }

  else if (locally != nullptr) *locally = false;

  wchar_t wszSystemDir [MAX_PATH * 2] = { };

#ifdef _WIN64
  GetSystemDirectoryW (wszSystemDir, MAX_PATH);
#else
  BOOL bWoW64 = FALSE;
  if (IsWow64Process (SK_GetCurrentProcess (), &bWoW64) && bWoW64)
    GetSystemWow64DirectoryW (wszSystemDir, MAX_PATH);
  else
    GetSystemDirectoryW      (wszSystemDir, MAX_PATH);
#endif

  PathAppendW (wszSystemDir, wszDll);

  return GetFileAttributesW (wszSystemDir) != INVALID_FILE_ATTRIBUTES;
};


void
SK_COMPAT_UnloadFraps (void)
{
#ifdef _WIN64
  if (GetModuleHandle (L"fraps64.dll"))
  {
    UnhookWindowsHookEx ((HHOOK) SK_GetProcAddress (L"fraps64.dll", "FrapsProcCBT"));
    FreeLibrary (GetModuleHandle (L"fraps64.dll"));
  }
#else
  if (GetModuleHandle (L"fraps.dll"))
  {
    UnhookWindowsHookEx ((HHOOK) SK_GetProcAddress (L"fraps.dll", "FrapsProcCBT"));
    FreeLibrary (GetModuleHandle (L"fraps.dll"));
  }
#endif
}

#include <SpecialK/control_panel.h>
bool
SK_COMPAT_IsFrapsPresent (void)
{
#ifdef _WIN64
  if (GetModuleHandle (L"fraps64.dll") != nullptr)
#else
  if (GetModuleHandle (L"fraps.dll") != nullptr)
#endif
  {
    SK_ImGui_Warning (L"FRAPS has been detected, expect weird things to happen until you uninstall it.");
    return true;
  }

  return false;
}