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

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Compat Sys"

volatile LONG __SK_TaskDialogActive = FALSE;

HWND WINAPI SK_GetForegroundWindow (void);
HWND WINAPI SK_GetFocus            (void);

HRESULT WINAPI
SK_TaskDialogIndirect ( _In_      const TASKDIALOGCONFIG* pTaskConfig,
                        _Out_opt_       int*              pnButton,
                        _Out_opt_       int*              pnRadioButton,
                        _Out_opt_       BOOL*             pfVerificationFlagChecked );

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
    if (SK_GetForegroundWindow () != hWnd &&
        SK_GetFocus            () != hWnd)
    {
      SK_RealizeForegroundWindow (hWnd);
    }

    SetForegroundWindow      (hWnd);
    SetWindowPos             (hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                              SWP_ASYNCWINDOWPOS | SWP_SHOWWINDOW   |
                              SWP_NOSIZE         | SWP_NOMOVE       |
                              SWP_NOSENDCHANGING   );
    SetActiveWindow          (hWnd);

    SK_SetWindowLongPtrW     (hWnd, GWL_EXSTYLE,
     ( (SK_GetWindowLongPtrW (hWnd, GWL_EXSTYLE) | (WS_EX_TOPMOST))));
  }

  if (uNotification == TDN_HYPERLINK_CLICKED)
  {
    SK_ShellExecuteW (nullptr, L"open",
      reinterpret_cast <wchar_t *>(lParam), nullptr, nullptr, SW_SHOW);
    return S_OK;
  }

  // It's important to keep the compatibility menu always on top, far more important than even
  //   the "Always On Top" window style can give us.
  if (uNotification == TDN_DIALOG_CONSTRUCTED)
  {
    InterlockedExchange (&__SK_TaskDialogActive, TRUE);

    SK_RealizeForegroundWindow
                             (hWnd);
       SetForegroundWindow   (hWnd);
    SK_SetWindowPos          (hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                              SWP_ASYNCWINDOWPOS | SWP_SHOWWINDOW   |
                              SWP_NOSIZE         | SWP_NOMOVE       |
                              SWP_NOSENDCHANGING   );
    SK_SetActiveWindow       (hWnd);
       SetFocus              (hWnd);

    SK_SetWindowLongPtrW     (hWnd, GWL_EXSTYLE,
     ( (SK_GetWindowLongPtrW (hWnd, GWL_EXSTYLE) | (WS_EX_TOPMOST))));
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

#ifdef _HAS_TIMER
  const bool timer = true;
#endif

  static const std::wstring ver_str =
    SK_FormatStringW ( L"Special K Compatibility Layer (v %ws)",
                         SK_GetVersionStrW () );

  int              nButtonPressed = 0;
  TASKDIALOGCONFIG task_config    = { };

  task_config.cbSize              = sizeof (task_config);
  task_config.hInstance           = __SK_hModHost;
  task_config.hwndParent          = nullptr;
  task_config.pszWindowTitle      = ver_str.c_str ();
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

#ifdef _HAS_TIMER
  if (timer)
#endif
    task_config.dwFlags |= TDF_CALLBACK_TIMER;

  HRESULT hr =
    SK_TaskDialogIndirect ( &task_config,
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

#ifdef _HAS_TIMER
  const bool timer = true;
#endif

  static const std::wstring ver_str =
    SK_FormatStringW ( L"Special K Compatibility Layer (v %ws)",
                         SK_GetVersionStrW () );

  int              nButtonPressed =   0;
  TASKDIALOGCONFIG task_config    = {   };

  task_config.cbSize              = sizeof (task_config);
  task_config.hInstance           = SK_Modules->HostApp ();
  task_config.hwndParent          = nullptr;
  task_config.pszWindowTitle      = ver_str.c_str ();
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

#ifdef _HAS_TIMER
  if (timer)
#endif
    task_config.dwFlags |= TDF_CALLBACK_TIMER;

  HRESULT hr =
    SK_TaskDialogIndirect ( &task_config,
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
  constexpr sk_bypass_s (void) = default;

  BOOL    disable                 = FALSE;
  wchar_t wszBlacklist [MAX_PATH] = {   };
} __bypass;



LPVOID pfnGetClientRect    = nullptr;
LPVOID pfnGetWindowRect    = nullptr;
LPVOID pfnGetSystemMetrics = nullptr;

DWORD
WINAPI
SK_Bypass_CRT (LPVOID)
{
  SetCurrentThreadDescription (L"[SK] Compatibility Layer Dialog");

  static BOOL     disable      = __bypass.disable;
         wchar_t* wszBlacklist = __bypass.wszBlacklist;

  bool timer = true;

  int              nButtonPressed = 0;
  TASKDIALOGCONFIG task_config    = { };

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
       d3d12  = false,
       glide  = false;

  SK_TestRenderImports (__SK_hModHost, &gl, &vulkan, &d3d9, &dxgi, &d3d11, &d3d12, &d3d8, &ddraw, &glide);

  const bool no_imports = !(gl || vulkan || d3d9 || d3d8 || ddraw || d3d11 || d3d12 || glide);

  auto dgVoodoo_Check = [](void) ->
    bool
    {
      return
        GetFileAttributesA (
          SK_FormatString ( R"(%ws\PlugIns\ThirdParty\dgVoodoo\d3dimm.dll)",
                              SK_GetInstallPath ()
                          ).c_str ()
        ) != INVALID_FILE_ATTRIBUTES;
    };

  auto dgVoodoo_Nag = [&](void) ->
    bool
    {
      while (
        MessageBox (HWND_DESKTOP, L"dgVoodoo is required for Direct3D8 / DirectDraw support\r\n\t"
                                  L"Please install its DLLs to 'Documents\\My Mods\\Special K\\PlugIns\\ThirdParty\\dgVoodoo'",
                                    L"Third-Party Plug-In Required",
                                      MB_ICONSTOP | MB_RETRYCANCEL) == IDRETRY && (! dgVoodoo_Check ())
            ) ;

      return dgVoodoo_Check ();
    };

#ifdef _M_IX86
  const bool has_dgvoodoo = dgVoodoo_Check ();
#endif

  const wchar_t* wszAPI = SK_GetBackend ();

  if (config.apis.last_known != SK_RenderAPI::Reserved)
  {
    switch (static_cast <SK_RenderAPI> (config.apis.last_known))
    {
      case SK_RenderAPI::D3D8:
      case SK_RenderAPI::D3D8On11:
      case SK_RenderAPI::D3D8On12:
        wszAPI = L"d3d8";
        SK_SetDLLRole (DLL_ROLE::D3D8);
        break;
      case SK_RenderAPI::DDraw:
      case SK_RenderAPI::DDrawOn11:
      case SK_RenderAPI::DDrawOn12:
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
#ifdef _M_IX86
                                           { 7, L"Direct3D8"        },
                                           { 8, L"DirectDraw"       },
#endif
                                           { 2, L"Direct3D9{Ex}"    },
                                           { 3, L"Direct3D11"       },
                                           { 4, L"Direct3D12"       },
                                           { 5, L"OpenGL"           },
#ifdef _M_AMD64
                                           { 6, L"Vulkan"           }
#endif
                                       };
  static const std::wstring ver_str =
    SK_FormatStringW ( L"Special K Compatibility Layer (v %ws)",
                         SK_GetVersionStrW () );

  task_config.cbSize              = sizeof (task_config);
  task_config.hInstance           = __SK_hModHost;
  task_config.hwndParent          = nullptr;
  task_config.pszWindowTitle      = ver_str.c_str ();
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

  if (timer) //-V547
    task_config.dwFlags |= TDF_CALLBACK_TIMER;

  int nRadioPressed = 0;


  HRESULT hr =
    SK_TaskDialogIndirect ( &task_config,
                             &nButtonPressed,
                               &nRadioPressed,
                                 SK_IsInjected () ? &disable : nullptr );


  extern iSK_INI* dll_ini;
  const  wchar_t* wszConfigName = L"";

  if (SK_IsInjected ())
    wszConfigName = L"SpecialK";

  else
  {
    wszConfigName = dll_ini != nullptr       ?
                    dll_ini->get_filename () :
                            SK_GetBackend ();
  }

  std::wstring temp_dll;


  SK_LoadConfig (wszConfigName);

  SK_ReleaseAssert (dll_ini != nullptr);

  config.apis.d3d9.hook       = false;
  config.apis.d3d9ex.hook     = false;
  config.apis.dxgi.d3d11.hook = false;
  config.apis.dxgi.d3d12.hook = false;

  config.apis.OpenGL.hook     = false;

#ifdef _M_AMD64
  config.apis.Vulkan.hook     = false;
#else /* _M_IX86 */
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
        config.apis.dxgi.d3d12.hook = true;
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

#ifdef _M_IX86
      if (SK_GetDLLRole () & DLL_ROLE::D3D8)
      {
        config.apis.d3d8.hook       = true;
        config.apis.dxgi.d3d11.hook = true;
        config.apis.dxgi.d3d12.hook = true;
      }

      if (SK_GetDLLRole () & DLL_ROLE::DDraw)
      {
        config.apis.ddraw.hook      = true;
        config.apis.dxgi.d3d11.hook = true;
        config.apis.dxgi.d3d12.hook = true;
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
                         SK_FormatString ( R"(%ws\SpecialK%lu.dll)",
                           SK_GetInstallPath (),
#ifdef _M_IX86
                             32
#else /* _M_AMD64 */
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
        config.apis.dxgi.d3d12.hook = true;

        config.apis.OpenGL.hook     = true;

#ifdef _M_IX86
        config.apis.d3d8.hook       = true;
        config.apis.ddraw.hook      = true;
#else /* _M_AMD64 */
        config.apis.Vulkan.hook     = true;
#endif

        if (nButtonPressed == BUTTON_INSTALL)
        {
          if (SK_IsInjected ()) SK_Inject_SwitchToRenderWrapperEx  (SK_GetDLLRole ());
          else
          {
            SK_Inject_SwitchToGlobalInjector ();

            temp_dll = SK_UTF8ToWideChar (
                         SK_FormatString ( R"(%ws\SpecialK%lu.dll)",
                           SK_GetInstallPath (),
#ifdef _M_IX86
                             32
#else /* _M_AMD64 */
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
                         SK_FormatString ( R"(%ws\SpecialK%lu.dll)",
                           SK_GetInstallPath (),
#ifdef _M_IX86
                             32
#else /* _M_AMD64 */
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
                         SK_FormatString ( R"(%ws\SpecialK%lu.dll)",
                           SK_GetInstallPath (),
#ifdef _M_IX86
                             32
#else /* _M_AMD64 */
                             64
#endif
                         )
                      );
          }
        }
        break;

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
                         SK_FormatString ( R"(%ws\SpecialK%lu.dll)",
                           SK_GetInstallPath (),
#ifdef _M_IX86
                             32
#else /* _M_AMD64 */
                             64
#endif
                         )
                      );
            }
        }
        break;

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
                         SK_FormatString ( R"(%ws\SpecialK%lu.dll)",
                           SK_GetInstallPath (),
#ifdef _M_IX86
                             32
#else /* _M_AMD64 */
                             64
#endif
                         )
                      );
          }
        }
        break;

#ifdef _M_AMD64
      case 6:
        config.apis.Vulkan.hook = true;
        break;
#endif

#ifdef _M_IX86
      case 7:
        config.apis.dxgi.d3d11.hook = true;  // D3D8 on D3D11 (not native D3D8)
        config.apis.dxgi.d3d12.hook = true;  // D3D8 on D3D12 (not native D3D8)
        config.apis.d3d8.hook       = true;

        if (has_dgvoodoo || dgVoodoo_Nag ())
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
                           SK_FormatString ( R"(%ws\SpecialK%lu.dll)",
                             SK_GetInstallPath (),
#ifdef _M_IX86
                               32
#else /* _M_AMD64 */
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
        config.apis.dxgi.d3d12.hook = true;  // DDraw on D3D12 (not native DDraw)
        config.apis.ddraw.hook      = true;

        if (nButtonPressed == BUTTON_INSTALL)
        {
          if (has_dgvoodoo || dgVoodoo_Nag ())
          {
            if (SK_IsInjected ())
              SK_Inject_SwitchToRenderWrapperEx (DLL_ROLE::DDraw);
            else
            {
              SK_Inject_SwitchToGlobalInjector ();

              temp_dll = SK_UTF8ToWideChar (
                           SK_FormatString ( R"(%ws\SpecialK%lu.dll)",
                             SK_GetInstallPath (),
#ifdef _M_IX86
                               32
#else /* _M_AMD64 */
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
#ifdef _M_AMD64
      dll_ini->remove_section (L"Import.ReShade64");
      dll_ini->remove_section (L"Import.ReShade64_Custom");
#else /* _M_IX86 */
      dll_ini->remove_section (L"Import.ReShade32");
      dll_ini->remove_section (L"Import.ReShade32_Custom");
#endif
      dll_ini->write ();
    }

    if (nButtonPressed == BUTTON_RESET_CONFIG)
    {
      std::wstring fname =
        dll_ini->get_filename ();

      // Invalidate, so we don't write the INI we just deleted :)
      dll_ini->rename (L"");

      DeleteFileW     (fname.c_str ());
      SK_DeleteConfig (wszConfigName);

      // Hard-code known plug-in config files -- bad (lack of) design.
      //
      SK_DeleteConfig (L"FAR.ini");   SK_DeleteConfig (L"UnX.ini");   SK_DeleteConfig (L"PPrinny.ini");
      SK_DeleteConfig (L"TBFix.ini"); SK_DeleteConfig (L"TSFix.ini"); SK_DeleteConfig (L"TZFix.ini");
    }

    else if (nButtonPressed != BUTTON_OK)
    {
      SK_SaveConfig   (wszConfigName);
      dll_ini->rename (L"");
    }

    if ( nButtonPressed         == BUTTON_INSTALL &&
         no_imports                               &&
         nRadioPressed          == 0 /* Auto */   &&
         config.apis.last_known == SK_RenderAPI::Reserved )
    {
      MessageBoxA   ( SK_HWND_DESKTOP,
                        SK_FormatString ( "API detection may be incorrect, delete '%ws.dll' "
                                          "manually if Special K does not inject "
                                          "itself.", SK_GetBackend () ).c_str (),
                          "Possible API Detection Problems",
                            MB_ICONINFORMATION | MB_OK
                    );
      SK_ShellExecuteW ( SK_HWND_DESKTOP, L"explore", SK_GetHostPath (), nullptr, nullptr, SW_SHOWNORMAL );
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

  if (nButtonPressed == BUTTON_INSTALL)
  {
    // Wake SKIF up and simulate a successful game launch (as global injection)
    SK_Inject_BroadcastInjectionNotify (true);
    SK_Inject_BroadcastExitNotify      (true);
  }

  if (temp_dll.empty ())
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
    std::make_pair (std::queue <DWORD> { }, __bypass.disable);
}



void
SK_COMPAT_FixUpFullscreen_DXGI (bool Fullscreen)
{
  if (Fullscreen)
  {
    if (SK_GetCurrentGameID () == SK_GAME_ID::WorldOfFinalFantasy)
    {
      ShowCursor  (TRUE);
      ShowWindow  ( SK_GetForegroundWindow (), SW_HIDE );
      MessageBox  ( SK_GetForegroundWindow (),
                      L"Please re-configure this game to run in windowed mode",
                        L"Special K Conflict",
                          MB_OK        | MB_SETFOREGROUND |
                          MB_APPLMODAL | MB_ICONASTERISK );

      SK_ShellExecuteW (HWND_DESKTOP, L"open", L"WOFF_config.exe", nullptr, nullptr, SW_NORMAL);

      while (SK_IsProcessRunning (L"WOFF_config.exe"))
        SK_Sleep (250UL);

      SK_ShellExecuteW (HWND_DESKTOP, L"open", L"WOFF.exe",        nullptr, nullptr, SW_NORMAL);

      ExitProcess   (0x00);
    }

#if 0
    if (config.window.background_render)
    {   config.window.background_render = false;

      SK_ImGui_Warning (L"\"Continue Rendering\" mode has been disabled because game is using Fullscreen Exclusive.");
    }

    // If this were left on, it would cause problems in some games like Dying Light
    if (config.window.fullscreen)
    {   config.window.fullscreen = false;
    }
#endif
  }
}




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
  SK_ComPtr <IActivateAudioInterfaceAsyncOperation> aOp;

  ActivateAudioInterfaceAsync ( L"I don't exist",
                                  __uuidof (IAudioClient),
                                    nullptr, nullptr, &aOp );

  if (SK_GetModuleHandle (L"Nahimic2DevProps.dll"))
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

  wchar_t wszSystemDir [MAX_PATH + 2] = { };

#ifdef _M_AMD64
  GetSystemDirectoryW (wszSystemDir, MAX_PATH);
#else /* _M_IX86 */
  BOOL bWoW64 = FALSE;
  if (IsWow64Process (SK_GetCurrentProcess (), &bWoW64) && bWoW64)
    GetSystemWow64DirectoryW (wszSystemDir, MAX_PATH);
  else
    GetSystemDirectoryW      (wszSystemDir, MAX_PATH);
#endif

  PathAppendW (wszSystemDir, wszDll);

  return
    GetFileAttributesW (wszSystemDir) != INVALID_FILE_ATTRIBUTES;
};


void
SK_COMPAT_UnloadFraps (void)
{
#ifdef _M_AMD64
  if (SK_GetModuleHandle (L"fraps64.dll"))
  {
       UnhookWindowsHookEx ((HHOOK) SK_GetProcAddress (L"fraps64.dll", "FrapsProcCBT"));
    SK_FreeLibrary (               SK_GetModuleHandle (L"fraps64.dll"));
  }
#else /* _M_IX86 */
  if (SK_GetModuleHandle (L"fraps.dll"))
  {
       UnhookWindowsHookEx ((HHOOK) SK_GetProcAddress (L"fraps.dll", "FrapsProcCBT"));
    SK_FreeLibrary (               SK_GetModuleHandle (L"fraps.dll"));
  }
#endif
}

bool
SK_COMPAT_IsFrapsPresent (void)
{
#ifdef _M_AMD64
  if (SK_GetModuleHandle (L"fraps64.dll") != nullptr)
#else /* _M_IX86 */
  if (SK_GetModuleHandle (L"fraps.dll") != nullptr)
#endif
  {
    SK_ImGui_Warning (L"FRAPS has been detected, expect weird things to happen until you uninstall it.");
    return true;
  }

  return false;
}

bool SK_COMPAT_IgnoreDxDiagnCall (LPCVOID pReturn)
{
  return
    (! config.compatibility.allow_dxdiagn) &&
    (! _wcsicmp (SK_GetCallerName (pReturn).c_str (),
                                L"dxdiagn.dll"));
}

bool SK_COMPAT_IgnoreNvCameraCall (LPCVOID pReturn)
{
  return
    ( config.nvidia.bugs.bypass_ansel) &&
    ( StrStrIW (SK_GetCallerName (pReturn).c_str (),
                                L"NvCamera"));
}

bool SK_COMPAT_IgnoreEOSOVHCall (LPCVOID pReturn)
{
  return
    (! _wcsicmp ( SK_GetCallerName (pReturn).c_str (),
       SK_RunLHIfBitness ( 32, L"EOSOVH-Win32-Shipping.dll",
                               L"EOSOVH-Win64-Shipping.dll" ) ) );
}

using  slIsFeatureSupported_pfn = sl::Result (*)(sl::Feature feature, const sl::AdapterInfo& adapterInfo);
static slIsFeatureSupported_pfn
       slIsFeatureSupported_Original = nullptr;

using  slInit_pfn = sl::Result (*)(const sl::Preferences &pref, uint64_t sdkVersion);
static slInit_pfn
       slInit_Original = nullptr;

sl::Result
slIsFeatureSupported_Detour (sl::Feature feature, const sl::AdapterInfo& adapterInfo)
{
  std::ignore = feature;
  std::ignore = adapterInfo;

  return sl::Result::eOk;
}

sl::Result
slInit_Detour (const sl::Preferences &pref, uint64_t sdkVersion = sl::kSDKVersion)
{
  SK_LOG_FIRST_CALL

  SK_LOGi0 (L"[!] slInit (pref.structVersion=%d, sdkVersion=%d)",
                          pref.structVersion,    sdkVersion);

  // For versions of Streamline using a compatible preferences struct,
  //   start overriding stuff for compatibility.
  if (pref.structVersion == sl::kStructVersion1)
  {
    sl::Preferences pref_copy = pref;

    //
    // Always print Streamline debug output to Special K's game_output.log,
    //   disable any log redirection the game would ordinarily do on its own.
    //
    pref_copy.logMessageCallback = nullptr;
#ifdef _DEBUG
    pref_copy.logLevel           = sl::LogLevel::eVerbose;
#else
    pref_copy.logLevel           = sl::LogLevel::eDefault;
#endif

    // Make forcing proxies into an option
    //pref_copy.flags |=
    //  sl::PreferenceFlags::eUseDXGIFactoryProxy;

    return
      slInit_Original (pref_copy, sdkVersion);
  }

  SK_LOGi0 (
    L"WARNING: Game is using a version of Streamline too new for Special K!"
  );

  return
    slInit_Original (pref, sdkVersion);
}

bool
SK_COMPAT_CheckStreamlineSupport (void)
{
  if (SK_IsModuleLoaded (L"sl.interposer.dll"))
  {
    if (! config.compatibility.allow_fake_streamline)
    {
      SK_RunOnce (
        SK_CreateDLLHook2 (      L"sl.interposer.dll",
                                  "slInit",
                                   slInit_Detour,
          static_cast_p2p <void> (&slInit_Original));

        SK_ApplyQueuedHooks ();
      );
    }

    // Feature Spoofing
/*
      SK_CreateDLLHook2 (      L"sl.interposer.dll",
                                "slIsFeatureSupported",
                                 slIsFeatureSupported_Detour,
        static_cast_p2p <void> (&slIsFeatureSupported_Original));
*/
  }

  return true;

  //
  // As of 23.9.13, compatibility in all known games is perfect!
  //
}

// It is never necessary to call this, it can be implemented using QueryInterface
using slGetNativeInterface_pfn = sl::Result (*)(void* proxyInterface, void** baseInterface);
// This, on the other hand, requires an import from sl.interposer.dll
using slUpgradeInterface_pfn   = sl::Result (*)(                      void** baseInterface);

struct DECLSPEC_UUID ("ADEC44E2-61F0-45C3-AD9F-1B37379284FF")
  IStreamlineBaseInterface : IUnknown { };

sl::Result
SK_slGetNativeInterface (void *proxyInterface, void **baseInterface)
{
#if 0
  if (proxyInterface == nullptr || baseInterface == nullptr)
    return sl::Result::eErrorMissingInputParameter;

  IUnknown* pUnk =
    static_cast <IUnknown *> (proxyInterface);

  if (FAILED (pUnk->QueryInterface (__uuidof (IStreamlineBaseInterface), baseInterface)))
    return sl::Result::eErrorUnsupportedInterface;
#else
  slGetNativeInterface_pfn
  slGetNativeInterface =
 (slGetNativeInterface_pfn)SK_GetProcAddress (L"sl.interposer.dll",
 "slGetNativeInterface");

  if (slGetNativeInterface != nullptr)
    return slGetNativeInterface (proxyInterface, baseInterface);
#endif

  return sl::Result::eErrorNotInitialized;
}

sl::Result
SK_slUpgradeInterface (void **baseInterface)
{
#if 0
  if (baseInterface == nullptr)
    return sl::Result::eErrorMissingInputParameter;

  IUnknown* pUnkInterface = *(IUnknown **)baseInterface;
  void*     pProxy        = nullptr;

  if (SUCCEEDED (pUnkInterface->QueryInterface (__uuidof (IStreamlineBaseInterface), &pProxy)))
  {
    // The passed interface already has a proxy, do nothing...
    static_cast <IUnknown *> (pProxy)->Release ();

    return sl::Result::eOk;
  }

  // If slInit (...) has not been called yet, sl.common.dll will not be present 
  if (! SK_IsModuleLoaded (L"sl.common.dll"))
    return sl::Result::eErrorInitNotCalled;

  auto slUpgradeInterface =
      (slUpgradeInterface_pfn)SK_GetProcAddress (L"sl.interposer.dll",
      "slUpgradeInterface");

  if (slUpgradeInterface != nullptr)
  {
    auto result =
      slUpgradeInterface (baseInterface);

    if (result == sl::Result::eOk)
    {
      static HMODULE
          hModPinnedInterposer  = nullptr,   hModPinnedCommon  = nullptr;
      if (hModPinnedInterposer == nullptr || hModPinnedCommon == nullptr)
      {
        // Once we have done this, there's no going back...
        //   we MUST pin the interposer DLL permanently.
        const BOOL bPinnedSL =
           (nullptr != hModPinnedInterposer || GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_PIN, L"sl.interposer.dll", &hModPinnedInterposer))
        && (nullptr != hModPinnedCommon     || GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_PIN,     L"sl.common.dll", &hModPinnedCommon    ));

        if (! bPinnedSL)
          SK_LOGi0 (L"Streamline Integration Has Invalid State!");
      }
    }

    return result;
  }
#else
  slUpgradeInterface_pfn
  slUpgradeInterface =
 (slUpgradeInterface_pfn)SK_GetProcAddress (L"sl.interposer.dll",
 "slUpgradeInterface");

  if (slUpgradeInterface != nullptr)
    return slUpgradeInterface (baseInterface);
#endif

  return sl::Result::eErrorNotInitialized;
}