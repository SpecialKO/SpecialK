// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
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
#include <SpecialK/resource.h>

extern HRESULT WINAPI
SK_TaskDialogIndirect ( _In_      const TASKDIALOGCONFIG* pTaskConfig,
                        _Out_opt_       int*              pnButton,
                        _Out_opt_       int*              pnRadioButton,
                        _Out_opt_       BOOL*             pfVerificationFlagChecked );

extern void
SK_Inject_Stop (void);

extern void
SK_Inject_Start (void);

static const int SK_UPDATE_TIMER = 68991;

bool    update_dlg_backup = false;
bool    update_dlg_keep   = false;

wchar_t update_dlg_file     [MAX_PATH + 2]             = { };
wchar_t update_dlg_build    [MAX_PATH + 2]             = { };
wchar_t update_dlg_relnotes [INTERNET_MAX_PATH_LENGTH] = { };


inline
int
_ShowCursor (void)
{
  int refs = 0;

  CURSORINFO ci = { sizeof (CURSORINFO),
                      0x0, nullptr,
                        { 0, 0 } };

  SK_GetCursorInfo (&ci);

  if (! (ci.flags & CURSOR_SHOWING))
  {
    int nButtons =
      SK_GetSystemMetrics (SM_CMOUSEBUTTONS);

    if (nButtons > 0)
    {
      do
      {
        ++refs;
      } while (! ShowCursor (TRUE));
    }
  }

  return refs;
};


DWORD
WINAPI
DownloadThread (LPVOID user)
{
  SetCurrentThreadDescription (L"[SK] HTTP Worker Thread");

  ULONG ulTimeout = 2500UL;

  auto* get =
    static_cast <sk_internet_get_t *> (user);

  auto TaskMsg =
    [&get](UINT Msg, WPARAM wParam, LPARAM lParam) ->
      LRESULT
      {
        if (get->hTaskDlg != nullptr)
          return SendMessage ( get->hTaskDlg,
                                 Msg,
                                   wParam,
                                     lParam );
        return NULL;
      };

  auto SetProgress =
    [&](double cur, double max)
    {
      TaskMsg ( TDM_SET_PROGRESS_BAR_POS,
                  static_cast <INT16> (
                    static_cast <double> (MAXINT16) *
                             ( cur / std::max (1.0, max) )
                  ), 0L );
    };

  auto SetErrorState =
    [&](void)
    {
      TaskMsg (TDM_SET_PROGRESS_BAR_STATE, PBST_NORMAL, 0L);
      TaskMsg (TDM_SET_PROGRESS_BAR_RANGE, 0L,          MAKEWPARAM (0, 1));
      TaskMsg (TDM_SET_PROGRESS_BAR_POS,   1,           0L);
      TaskMsg (TDM_SET_PROGRESS_BAR_STATE, PBST_ERROR,  0L);

      TaskMsg (TDM_ENABLE_BUTTON, IDYES,    1); // Re-enable the update button
      TaskMsg (TDM_ENABLE_BUTTON, 0,        1); // Re-enable remind me later button
      TaskMsg (TDM_ENABLE_BUTTON, IDCANCEL, 1); // Re-enable cancel button

      // Re-name the update button from Yes to Retry
      SetDlgItemTextW (get->hTaskDlg, IDYES, L"Retry");

      get->status = sk_internet_get_t::STATUS_FAILED;
    };

  TaskMsg (TDM_SET_PROGRESS_BAR_RANGE, 0L,          MAKEWPARAM (0, MAXINT16));
  TaskMsg (TDM_SET_PROGRESS_BAR_STATE, PBST_NORMAL, 0L);

  SetProgress (0, 0);

  PCWSTR rgpszAcceptTypes [] = { L"*/*", nullptr };
  HINTERNET hInetHTTPGetReq  = 0,
            hInetHost        = 0,
  hInetRoot                  =
    InternetOpen (
      L"Special K Auto-Update",
        INTERNET_OPEN_TYPE_DIRECT,
          nullptr, nullptr,
            0x00
    );

  if (! hInetRoot)
    goto CLEANUP;

  DWORD dwInetCtx;

  hInetHost =
    InternetConnect ( hInetRoot,
                        get->wszHostName,
                          INTERNET_DEFAULT_HTTP_PORT,
                            nullptr, nullptr,
                              INTERNET_SERVICE_HTTP,
                                0x00,
                                  (DWORD_PTR)&dwInetCtx );

  if (! hInetHost)
  {
    InternetCloseHandle (hInetRoot);
    goto CLEANUP;
  }

  hInetHTTPGetReq =
    HttpOpenRequest ( hInetHost,
                        nullptr,
                          get->wszHostPath,
                            L"HTTP/1.1",
                              nullptr,
                                rgpszAcceptTypes,
                                                                    INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
                                  INTERNET_FLAG_CACHE_IF_NET_FAIL | INTERNET_FLAG_IGNORE_CERT_CN_INVALID   |
                                  INTERNET_FLAG_RESYNCHRONIZE     | INTERNET_FLAG_CACHE_ASYNC,
                                    (DWORD_PTR)&dwInetCtx );


  // Wait 2500 msecs for a dead connection, then give up
  //
  InternetSetOptionW ( hInetHTTPGetReq, INTERNET_OPTION_RECEIVE_TIMEOUT,
                         &ulTimeout,      sizeof ULONG );


  if (! hInetHTTPGetReq)
  {
    InternetCloseHandle (hInetHost);
    InternetCloseHandle (hInetRoot);
    goto CLEANUP;
  }

  if ( HttpSendRequestW ( hInetHTTPGetReq,
                            nullptr,
                              0,
                                nullptr,
                                  0 ) )
  {

    DWORD dwContentLength     = 0;
    DWORD dwContentLength_Len = sizeof DWORD;
    DWORD dwSize;

    HttpQueryInfo ( hInetHTTPGetReq,
                      HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
                        &dwContentLength,
                          &dwContentLength_Len,
                            nullptr );

    SetProgress (0, dwContentLength);

    DWORD dwTotalBytesDownloaded = 0UL;

    if ( InternetQueryDataAvailable ( hInetHTTPGetReq,
                                        &dwSize,
                                          0x00, NULL )
      )
    {
      HANDLE hGetFile =
        CreateFileW ( get->wszLocalPath,
                        GENERIC_WRITE,
                          FILE_SHARE_READ,
                            nullptr,
                              CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL |
                                FILE_FLAG_SEQUENTIAL_SCAN,
                                  nullptr );

      while (hGetFile != INVALID_HANDLE_VALUE && dwSize > 0)
      {
        DWORD  dwRead = 0;
        auto  *pData  =
          static_cast <uint8_t *> (malloc (dwSize));

        if (! pData)
          break;

        if ( InternetReadFile ( hInetHTTPGetReq,
                                  pData,
                                    dwSize,
                                      &dwRead )
          )
        {
          DWORD dwWritten;

          WriteFile ( hGetFile,
                        pData,
                          dwRead,
                            &dwWritten,
                              nullptr );

          dwTotalBytesDownloaded += dwRead;

          SetProgress (dwTotalBytesDownloaded, dwContentLength);
        }

        free (pData);
        pData = nullptr;

        if (! InternetQueryDataAvailable ( hInetHTTPGetReq,
                                             &dwSize,
                                               0x00, NULL
                                         )
           ) break;
      }

      if (hGetFile != INVALID_HANDLE_VALUE)
        SK_CloseHandle (hGetFile);

      else
        SetErrorState ();
    }

    //HttpEndRequest ( hInetHTTPGetReq, nullptr, 0x00, 0 );

    if (dwTotalBytesDownloaded >= dwContentLength)
      get->status = sk_internet_get_t::STATUS_UPDATED;

    else
      SetErrorState ();
  }

  else
    SetErrorState ();

  InternetCloseHandle (hInetHTTPGetReq);
  InternetCloseHandle (hInetHost);
  InternetCloseHandle (hInetRoot);

  goto END;


  // (Cleanup On Error)
CLEANUP:
  SetErrorState ();


END:
  //if (! get->hTaskDlg)
    //delete get;

  SK_Thread_CloseSelf ();

  return 0;
}

HWND hWndUpdateDlg =
  static_cast <HWND> (INVALID_HANDLE_VALUE);

HWND hWndRemind    = nullptr;

#define ID_REMIND 0

struct sk_pcm_sound_s {
  HGLOBAL  ref = nullptr;
  uint8_t* buf = nullptr;
};

//SK_LazyGlobal <sk_pcm_sound_s>       annoy_sound;
SK_LazyGlobal <sk::ParameterFactory> g_UpdateParameterFactory;

INT_PTR
CALLBACK
RemindMeLater_DlgProc (
  _In_ HWND   hWndDlg,
  _In_ UINT   uMsg,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam )
{
  UNREFERENCED_PARAMETER (lParam);

  HWND hWndNextCheck =
    GetDlgItem (hWndDlg, IDC_NEXT_VERSION_CHECK);

  static int cursor_refs = 0;

  switch (uMsg)
  {
    case WM_TIMER:
    {
      if (wParam == SK_UPDATE_TIMER)
      {
        CURSORINFO ci = { };
                   ci.cbSize = sizeof (CURSORINFO);

        SK_GetCursorInfo (&ci);

        if (! (ci.flags & CURSOR_SHOWING))
        {
          int nButtons =
            SK_GetSystemMetrics (SM_CMOUSEBUTTONS);

          while (nButtons > 0 && (! ShowCursor (TRUE)))
          {
            cursor_refs++;
          }
        }

        HWND hWndFocus =
          GetFocus ();

        if ( game_window.hWnd != SK_HWND_DESKTOP    &&
               ( hWndFocus == game_window.hWnd   ||
                            ( game_window.active &&
                              hWndDlg            != GetForegroundWindow () ) ) )
        {
          SetForegroundWindow ( hWndDlg );
          SK_SetWindowPos     ( hWndDlg, SK_HWND_TOP,
                                0, 0, 0, 0,
                                SWP_ASYNCWINDOWPOS | SWP_SHOWWINDOW   |
                                SWP_NOSIZE         | SWP_NOMOVE       |
                                SWP_NOSENDCHANGING   );
        }
      }
    } break;

    case WM_DESTROY:
    {
      KillTimer (hWndDlg, SK_UPDATE_TIMER);

      if (cursor_refs > 0)
      {
        for ( int i = 0 ; i < cursor_refs ; i++ )
        {
          ShowCursor (FALSE);
        }

        cursor_refs = 0;
      }
    } break;

    case WM_INITDIALOG:
    {
      SetTimer (hWndDlg, SK_UPDATE_TIMER, 150, nullptr);

#if 0
      HRSRC   default_sound =
        FindResource (__SK_hModSelf, MAKEINTRESOURCE (IDR_ANNOYING), L"WAVE");

      if (default_sound != nullptr)
      {
        annoy_sound->ref   =
          LoadResource (__SK_hModSelf, default_sound);

        if (annoy_sound->ref != nullptr)
            annoy_sound->buf = (uint8_t *)LockResource (annoy_sound->ref);
      }
#endif

      ComboBox_InsertString (hWndNextCheck, 0, L"Next launch");
      ComboBox_InsertString (hWndNextCheck, 1, L"After 15 Minutes");
      ComboBox_InsertString (hWndNextCheck, 2, L"After 1 Hour");
      ComboBox_InsertString (hWndNextCheck, 3, L"After 12 Hours");
      ComboBox_InsertString (hWndNextCheck, 4, L"Tomorrow");
      ComboBox_InsertString (hWndNextCheck, 5, L"Never");

      ComboBox_SetCurSel (hWndNextCheck, 0);

      return 0;
    } break;

    case WM_COMMAND:
    {
      if (LOWORD (wParam) == IDOK)
      {
#if 0
        srand (GetCurrentProcessId ());

        if (! (rand () % 2))
          SK_PlaySound ( (LPCWSTR)annoy_sound->buf,
                         nullptr,
                           SND_ASYNC |
                           SND_MEMORY );
#endif

        FILETIME                  ftNow;
        GetSystemTimeAsFileTime (&ftNow);

        ULARGE_INTEGER next_uli {
          ftNow.dwLowDateTime, ftNow.dwHighDateTime
        };

        ULONGLONG next =
          next_uli.QuadPart;

        bool never = false;

        const ULONGLONG _Minute =   600000000ULL;
        const ULONGLONG _Hour   = 36000000000ULL;

        switch (ComboBox_GetCurSel (hWndNextCheck))
        {
          // Immediate
          case 0:
          default:
            break;

          case 1:
            next += (15 * _Minute);
            break;

          case 2:
            next += (1 * _Hour);
            break;

          case 3:
            next += (12 * _Hour);
            break;

          case 4:
            next += (24 * _Hour);
            break;

          case 5:
            next  = MAXULONGLONG;
            never = true;
            break;
        }

        iSK_INI install_ini (SK_Version_GetInstallIniPath ().c_str ());

        bool empty = false;

        if (install_ini.get_sections ().empty ())
          empty = true;

        iSK_INISection& user_prefs =
          install_ini.get_section (L"Update.User");

        if (empty)
          user_prefs.set_name (L"Update.User");

        if (! never)
        {
          auto* remind_time =
            static_cast <sk::ParameterInt64 *> (
              g_UpdateParameterFactory->create_parameter <int64_t> (L"Reminder")
            );

          if (remind_time != nullptr)
          {
            remind_time->register_to_ini (
              &install_ini,
                L"Update.User",
                  L"Reminder"
            );

                   remind_time->store ((int64_t)next);
            delete remind_time;
          }
        }

        else
        {
          auto* frequency =
            static_cast <sk::ParameterStringW *> (
              g_UpdateParameterFactory->create_parameter <std::wstring> (
                L"Frequency"
              )
            );

          if (frequency != nullptr)
          {
            frequency->register_to_ini (
              &install_ini,
                L"Update.User",
                  L"Frequency"
            );

            user_prefs.remove_key (L"Reminder");

                   frequency->store (L"never");
            delete frequency;
          }
        }

        install_ini.write (SK_Version_GetInstallIniPath ().c_str ());

        EndDialog (hWndDlg, 0);
        hWndRemind = (HWND)INVALID_HANDLE_VALUE;

        return 0;
      }
    } break;
  }

  return 0;
}

HRESULT
CALLBACK
DownloadDialogCallback (
  _In_ HWND     hWnd,
  _In_ UINT     uNotification,
  _In_ WPARAM   wParam,
  _In_ LPARAM   lParam,
  _In_ LONG_PTR dwRefData )
{
  static int     cursor_refs = 0;
  static HWND    hWndOrigTop = nullptr;
  static HCURSOR hCursorOrig = nullptr;

  auto* get =
    reinterpret_cast <sk_internet_get_t *> (dwRefData);


  // Don't allow this window to be used while the
  //   reminder dialog is visible.
  if (IsWindow (hWndRemind))
    return S_FALSE;


  if (uNotification == TDN_TIMER)
  {
    cursor_refs += _ShowCursor ();

    HWND hWndFocus =
      GetFocus ();

    if ( game_window.hWnd != SK_HWND_DESKTOP &&
           ( hWndFocus == game_window.hWnd   ||
                        ( game_window.active &&
                          hWnd               != GetForegroundWindow () ) ) )
    {
      SetForegroundWindow ( hWnd );
      SK_SetWindowPos     ( hWnd, SK_HWND_TOP,
                            0, 0, 0, 0,
                            SWP_ASYNCWINDOWPOS | SWP_SHOWWINDOW   |
                            SWP_NOSIZE         | SWP_NOMOVE       |
                            SWP_NOSENDCHANGING   );
    }

    if ( get->status == sk_internet_get_t::STATUS_UPDATED   ||
         get->status == sk_internet_get_t::STATUS_CANCELLED ||
         get->status == sk_internet_get_t::STATUS_REMINDER )
    {
      EndDialog ( hWnd, 0 );

      return S_OK;
    }
  }

  else if (uNotification == TDN_HYPERLINK_CLICKED)
  {
    SK_ShellExecuteW (nullptr, L"open", (wchar_t *)lParam, nullptr, nullptr, SW_SHOWMAXIMIZED);

    return S_OK;
  }

  else if (uNotification == TDN_DIALOG_CONSTRUCTED)
  {
    SendMessage (hWnd, TDM_SET_PROGRESS_BAR_RANGE, 0L,          MAKEWPARAM (0, 1));
    SendMessage (hWnd, TDM_SET_PROGRESS_BAR_POS,   1,           0L);
    SendMessage (hWnd, TDM_SET_PROGRESS_BAR_STATE, PBST_PAUSED, 0L);

    SK_RealizeForegroundWindow (hWnd);
  //SK_SetWindowLongW          (hWnd, GWL_EXSTYLE,
  // ( (SK_GetWindowLongW      (hWnd, GWL_EXSTYLE) | (WS_EX_TOPMOST))));

    return S_OK;
  }

  else if (uNotification == TDN_CREATED)
  {
    return S_OK;
  }

  else if (uNotification == TDN_BUTTON_CLICKED)
  {
    if (wParam == ID_REMIND)
    {
      game_window.active = true;

      hWndRemind =
        CreateDialog ( __SK_hModSelf,
                         MAKEINTRESOURCE (IDD_REMIND_ME_LATER),
                           hWnd,
                             RemindMeLater_DlgProc );

      get->status = sk_internet_get_t::STATUS_REMINDER;
    }

    else if (wParam == IDYES)
    {
      // Disable the button
      SendMessage (hWnd, TDM_ENABLE_BUTTON, wParam, 0);

      // Also disable the remind me later button
      SendMessage (hWnd, TDM_ENABLE_BUTTON, 0,      0);

      // Also disable the cancel button
      SendMessage (hWnd, TDM_ENABLE_BUTTON, IDCANCEL, 0);

      get->hTaskDlg = hWnd;

      SK_Thread_Create ( DownloadThread, (LPVOID)get );
    }

    else if (wParam == IDCANCEL)
    {
      get->status = sk_internet_get_t::STATUS_CANCELLED;
    }

    else
    {
      //get->status = STATUS_CANCELLED;
      //return S_OK;
    }
  }

  else if (uNotification == TDN_DESTROYED)
  {
    if (cursor_refs > 0)
    {
      for (int i = 0; i < cursor_refs; i++)
      {
        ShowCursor (FALSE);
      }

      cursor_refs = 0;
    }

    if (game_window.hWnd != nullptr)
    {
      SetForegroundWindow ( game_window.hWnd);
      SK_SetWindowPos     ( game_window.hWnd, HWND_TOP, 0, 0, 0, 0,
                            SWP_ASYNCWINDOWPOS | SWP_SHOWWINDOW |
                            SWP_NOSIZE         | SWP_NOMOVE     |
                            SWP_NOREPOSITION   | SWP_NOACTIVATE |
                            SWP_NOSENDCHANGING );
      ShowWindowAsync     (game_window.hWnd,    SW_SHOWNORMAL);
    }
  }

  return S_FALSE;
}


int
__stdcall
DecompressionProgressCallback (int current, int total)
{
  static int last_total = 0;

  HWND hWndProgress =
    GetDlgItem (hWndUpdateDlg, IDC_UPDATE_PROGRESS);

  if (total != last_total)
  {
    SendMessage (hWndProgress, PBM_SETSTATE, PBST_NORMAL, 0UL);
    SendMessage (hWndProgress, PBM_SETRANGE, 0UL, MAKEWPARAM (0, total));
  }

  SendMessage (hWndProgress, PBM_SETPOS, current, 0UL);

  return 0;
}

static volatile LONG __SK_UpdateStatus = 0;

INT_PTR
CALLBACK
Update_DlgProc (
  _In_ HWND   hWndDlg,
  _In_ UINT   uMsg,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam )
{
  UNREFERENCED_PARAMETER (lParam);

  static int cursor_refs = 0;

  switch (uMsg)
  {
    default:
      return 0;

    case WM_INITDIALOG:
    {
      InterlockedExchange ( &__SK_UpdateStatus, 0 );

      hWndUpdateDlg = hWndDlg;

      HWND hWndBackup =
        GetDlgItem (hWndUpdateDlg, IDC_BACKUP_FILES);

      HWND hWndKeepDownloads =
        GetDlgItem (hWndUpdateDlg, IDC_KEEP_DOWNLOADS);

      Button_SetCheck (hWndBackup,        update_dlg_backup);
      Button_SetCheck (hWndKeepDownloads, update_dlg_keep);

      HWND hWndProgress =
        GetDlgItem (hWndUpdateDlg, IDC_UPDATE_PROGRESS);

      SendMessage (hWndProgress, PBM_SETRANGE, 0UL,         MAKEWPARAM (0, 1));
      SendMessage (hWndProgress, PBM_SETPOS,   1,           0UL);
      SendMessage (hWndProgress, PBM_SETSTATE, PBST_PAUSED, 0UL);

      uint64_t fsize = SK_File_GetSize (update_dlg_file);

      std::vector <sk_file_entry_s> files;
      SK_Get7ZFileContents (update_dlg_file, files);

      wchar_t wszDownloadSize [32] = { },
              wszBackupSize   [32] = { };

      swprintf ( wszDownloadSize, L"   1 File,  %5.2f MiB",
                   (double)fsize / (1024.0 * 1024.0) );

      unsigned int backup_count = 0;
      uint64_t     backup_size  = 0ULL;

      for ( auto& it : files )
      {
        wchar_t    wszFinalPath [MAX_PATH + 2] = { };
        wcsncpy_s (wszFinalPath, MAX_PATH, SK_SYS_GetInstallPath ().c_str (),
                               _TRUNCATE);

        lstrcatW (wszFinalPath, it.name.c_str ());

        // This function returns 0 if no file exists
        uint64_t bsize =
          SK_File_GetSize (wszFinalPath);

        if (bsize != 0)
        {
          backup_size += bsize;
          ++backup_count;
        }
      }

      swprintf ( wszBackupSize, L"%4u Files, %5.2f MiB",
                   backup_count,
                     (double)backup_size / (1024.0 * 1024.0) );

      HFONT header_font =
        CreateFont ( 14,
                       0, 0, 0,
                         FW_MEDIUM,
                           FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_NATURAL_QUALITY,
                                   MONO_FONT,
                                     L"Consolas" );

      SetWindowFont  (GetDlgItem (hWndDlg, IDC_DOWNLOAD_SIZE), header_font, true);
      SetWindowFont  (GetDlgItem (hWndDlg, IDC_BACKUP_SIZE),   header_font, true);

      // Initiate the update automatically.
      if (SK_IsHostAppSKIM ())
      {
        SK_ShowWindow (hWndBackup,                           SW_HIDE);
        SK_ShowWindow (hWndKeepDownloads,                    SW_HIDE);
        SK_ShowWindow (GetDlgItem (hWndDlg, IDC_AUTO_CMD),   SW_HIDE);
        SK_ShowWindow (GetDlgItem (hWndDlg, IDC_MANUAL_CMD), SW_HIDE);

        swprintf ( wszDownloadSize, L"Download:   %5.2f MiB",
                     (double)fsize / (1024.0 * 1024.0) );
      }

      Static_SetText (GetDlgItem (hWndDlg, IDC_DOWNLOAD_SIZE), wszDownloadSize);
      Static_SetText (GetDlgItem (hWndDlg, IDC_BACKUP_SIZE),   wszBackupSize);

      SK_RealizeForegroundWindow (hWndDlg);

      return TRUE;
    }

    case WM_COMMAND:
    {
      if (LOWORD (wParam) == IDC_MANUAL_CMD)
      {
        SK_ShellExecuteW ( nullptr,
                             L"OPEN",
                               (wchar_t *)update_dlg_file,
                                 nullptr, nullptr,
                                   SW_SHOWMAXIMIZED );

        InterlockedExchange ( &__SK_UpdateStatus, 1 );
        PostMessage         ( hWndUpdateDlg, WM_CLOSE, 0x0, 0x0 );
        hWndUpdateDlg = (HWND)INVALID_HANDLE_VALUE;
      }

      if (LOWORD (wParam) == IDC_AUTO_CMD)
      {
        InterlockedExchangeAcquire ( &__SK_UpdateStatus, 0 );

        // Stop injecting, we need to move a few files around...
        if (SK_IsInjected ())
        {
          SK_Inject_Stop ();
        }

        update_dlg_backup =
          Button_GetCheck (GetDlgItem (hWndUpdateDlg, IDC_BACKUP_FILES)) != 0;

        if ( SUCCEEDED ( SK_Decompress7z (
                           update_dlg_file,
                             update_dlg_build,
                               update_dlg_backup,
                                 DecompressionProgressCallback
                         )
             )
           )
        {
          TASKDIALOGCONFIG task_cfg = { };

          task_cfg.cbSize = sizeof TASKDIALOGCONFIG;

          task_cfg.hwndParent         = hWndUpdateDlg;
          task_cfg.hInstance          = SK_GetModuleHandleW (nullptr);

          task_cfg.pszMainIcon        = TD_INFORMATION_ICON;
          task_cfg.dwFlags            = TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS;

          TASKDIALOG_BUTTON buttons [2];
          buttons [0].nButtonID       = IDOK;

          if (! SK_IsHostAppSKIM ())
          {
            buttons [0].pszButtonText = L"Finish Update\nThe game will automatically exit.";
          }

          else
          {
            buttons [0].pszButtonText = L"Finish Install";
          }

          buttons [1].nButtonID       = 0;
          buttons [1].pszButtonText   = L"View Release Notes";

          task_cfg.pButtons           = buttons;
          task_cfg.cButtons           = 2;

          // Regular Software Update
          if (! SK_IsHostAppSKIM ())
          {
            task_cfg.pszWindowTitle     = L"Special K Software Update";
            task_cfg.pszMainInstruction = L"Software Update Successful";
          }

          // Software Installation
          else
          {
            task_cfg.pszWindowTitle     = L"Special K Software Install";
            task_cfg.pszMainInstruction = L"Software Installation Successful";
          }

          task_cfg.pfCallback =
            []( HWND     hWnd,
                UINT     uNotification,
                WPARAM   wParam,
                LPARAM   lParam,
                LONG_PTR dwRefData ) ->
            HRESULT
            {
              UNREFERENCED_PARAMETER (hWnd);
              UNREFERENCED_PARAMETER (wParam);
              UNREFERENCED_PARAMETER (lParam);
              UNREFERENCED_PARAMETER (dwRefData);

              switch (uNotification)
              {
                case TDN_CREATED:
                {
                  SK_RealizeForegroundWindow (hWnd);
                } break;

                case TDN_HYPERLINK_CLICKED:
                  SK_ShellExecuteW ( nullptr,
                                       L"OPEN",
                                         (wchar_t *)lParam,
                                           nullptr, nullptr,
                                             SW_SHOWMAXIMIZED );
                  break;

                case TDN_BUTTON_CLICKED:
                  if (wParam == 0)
                  {
                    SK_ShellExecuteW ( nullptr,
                                         L"OPEN",
                                           (wchar_t *)update_dlg_relnotes,
                                             nullptr, nullptr,
                                               SW_SHOWMAXIMIZED );
                  }
                  break;
              }

              return S_OK;
            };

          wchar_t wszBackupMessage [4096] = { };

          extern bool config_files_changed;


          if (! SK_IsHostAppSKIM ())
          {
            if (update_dlg_backup)
            {
              swprintf ( wszBackupMessage,
                           L"Your old files have been backed up "
                           L"<a href=\"%s\\Version\\%s\\\">here.</a>\n\n%s",
                             SK_GetConfigPath (),
                               update_dlg_build,
                                config_files_changed ?
                  L"Config files were altered, but your originals "
                  L"have been backed up." :
                    L"No config files were altered by this release."
              );
            }

            else
            {
              swprintf ( wszBackupMessage,
                           L"Existing mod files were overwritten%s",
                             config_files_changed ?
                  L"; config files were altered, but your originals "
                  L"have been backed up." :
                    L"; no config files were altered." );
            }
          }

          task_cfg.pszContent = wszBackupMessage;

          SK_TaskDialogIndirect ( &task_cfg, nullptr, nullptr, nullptr );

          update_dlg_keep =
            Button_GetCheck (GetDlgItem (hWndUpdateDlg, IDC_KEEP_DOWNLOADS)) != 0;

          if (! update_dlg_keep)
            DeleteFileW (update_dlg_file);

          // SUCCESS:
          InterlockedExchange ( &__SK_UpdateStatus, 1 );
          PostMessage         ( hWndUpdateDlg, WM_CLOSE, 0x0, 0x0 );

          hWndUpdateDlg =
            static_cast <HWND> (INVALID_HANDLE_VALUE);
        }

        else
        {
          // FAILURE:
          InterlockedExchange ( &__SK_UpdateStatus, -1            );
          PostMessage         ( hWndUpdateDlg, WM_CLOSE, 0x0, 0x0 );

          hWndUpdateDlg =
            static_cast <HWND> (INVALID_HANDLE_VALUE);
        }
      }

      return 1;
    }

    case WM_DESTROY:
    {
      KillTimer (hWndDlg, SK_UPDATE_TIMER+1);

      if (cursor_refs > 0)
      {
        for ( int i = 0 ; i < cursor_refs ; i++ )
        {
          ShowCursor (FALSE);
        }

        cursor_refs = 0;
      }

      hWndUpdateDlg =
        static_cast <HWND> (INVALID_HANDLE_VALUE);

      return 0;
    } break;

    case WM_TIMER:
    {
      if (wParam == SK_UPDATE_TIMER+1)
      {
        cursor_refs +=
          _ShowCursor ();

        HWND hWndFocus =
          GetFocus ();

        if ( game_window.hWnd != SK_HWND_DESKTOP &&
               ( hWndFocus == game_window.hWnd   ||
                            ( game_window.active &&
                              hWndDlg            != GetForegroundWindow () ) ) )
        {
          SetForegroundWindow ( hWndDlg );
          SK_SetWindowPos     ( hWndDlg, SK_HWND_TOP,
                                0, 0, 0, 0,
                                SWP_ASYNCWINDOWPOS | SWP_SHOWWINDOW   |
                                SWP_NOSIZE         | SWP_NOMOVE       |
                                SWP_NOSENDCHANGING   );
        }
      }
    } break;

    case WM_CREATE:
    {
      SetTimer (hWndDlg, SK_UPDATE_TIMER+1, 150, nullptr);

      SK_RealizeForegroundWindow (hWndDlg);
      InterlockedExchange        ( &__SK_UpdateStatus, 0 );
    } break;
  }

  return 0;
}

DWORD
WINAPI
UpdateDlg_Thread (LPVOID user)
{
  SetCurrentThreadDescription (L"[SK] Auto-Update Dialog Message Pump");

  bool started = false;

  game_window.active = true;

  HWND hWndDlg =
    CreateDialogW ( __SK_hModSelf,
                      MAKEINTRESOURCE (IDD_UPDATE),
                        (HWND)user,
                          Update_DlgProc );

  if (! hWndDlg)
  {
    SK_LOG0 ( ( L"Unable to Create Update Dialog :: GetLastError () = %lu", GetLastError () ),
                L"AutoUpdate" );

    InterlockedExchange ( &__SK_UpdateStatus, -1 );

    SK_Thread_CloseSelf ();

    return 0;
  }

  MSG  msg;
  BOOL bRet;

  while ((bRet = SK_GetMessageW (&msg, hWndDlg, 0, 0)) != 0)
  {
    if (bRet == -1)
    {
      SK_Thread_CloseSelf ();
      return 0;
    }

    SK_TranslateMessage (&msg);
    SK_DispatchMessageW (&msg);

    if ((! started) && msg.hwnd == hWndDlg)
    {
      if ( SK_IsHostAppSKIM () )
      {
        started = true;
        SendMessage (hWndDlg, WM_COMMAND, MAKEWPARAM (IDC_AUTO_CMD, 0), 0);
      }
    }
  }

  SK_Thread_CloseSelf ();

  return 0;
}

HRESULT
__stdcall
SK_UpdateSoftware1 (const wchar_t*, bool force)
{
#define INJECTOR
#ifndef INJECTOR
  if ((! SK_IsInjected ()))
  {
    if (! lstrcmpW (wszProduct, L"SpecialK"))
    {
      return E_FAIL;
    }
  }
#endif

  TASKDIALOGCONFIG  task_config  = { };

  task_config.cbSize             = sizeof ( task_config );
  task_config.hInstance          = SK_GetModuleHandleW (nullptr);
  task_config.hwndParent         =                      nullptr;


  if (! SK_IsHostAppSKIM ())
    task_config.pszWindowTitle   = L"Special K Auto-Update";
  else
    task_config.pszWindowTitle   = L"Special K Software Install";

  TASKDIALOG_BUTTON buttons [3];

  buttons [0].nButtonID     = IDYES;
  buttons [0].pszButtonText = L"Yes";

  if (! SK_IsHostAppSKIM ())
  {
    buttons [1].nButtonID     = 0;
    buttons [1].pszButtonText = L"Remind me later (or disable)";
  }

  else
  {
    buttons [1].nButtonID     = IDCANCEL;
    buttons [1].pszButtonText = L"No, I changed my mind...";
  }


  task_config.pButtons           = buttons;
  task_config.cButtons           = 2;
  task_config.dwCommonButtons    = 0x00;
  task_config.nDefaultButton     = IDYES;

  task_config.dwFlags            = TDF_ENABLE_HYPERLINKS | TDF_SHOW_PROGRESS_BAR   | TDF_SIZE_TO_CONTENT |
                                   TDF_CALLBACK_TIMER    | TDF_EXPANDED_BY_DEFAULT | TDF_EXPAND_FOOTER_AREA |
                                   TDF_POSITION_RELATIVE_TO_WINDOW;
  task_config.pfCallback         = DownloadDialogCallback;

  if (! SK_IsHostAppSKIM ())
  {
    task_config.pszMainInstruction = L"Software Update Available for Download";
    task_config.pszMainIcon        = TD_INFORMATION_ICON;

    task_config.pszContent         = L"Would you like to upgrade now?";
  }

  else
  {
    task_config.pszMainInstruction = L"Software Installation Ready to Download";
    task_config.pszMainIcon        = TD_INFORMATION_ICON;

    task_config.pszContent         = L"Would you like to begin installation now?";
  }

  iSK_INI install_ini (SK_Version_GetInstallIniPath ().c_str ());
  iSK_INI repo_ini    (SK_Version_GetRepoIniPath    ().c_str ());

  struct {
    signed int   installed     =  -1;
    wchar_t      branch  [128] = { };

    struct {
      signed int in_branch     =  -1; // TODO
      wchar_t    package [128] = { };
    } latest = { 0, { } };
  } build    = { 0, { } };

  bool empty = false;

  if (install_ini.get_sections ().empty ())
    empty = true;

  // Not exactly empty, but certainly not in a working state.
  if (! install_ini.contains_section (L"Version.Local"))
    empty = true;

  iSK_INISection& installed_ver =
    install_ini.get_section (L"Version.Local");

  if (empty)
  {
    installed_ver.set_name      (L"Version.Local");
    installed_ver.add_key_value (L"InstallPackage", L"Invalid,-1");
    installed_ver.add_key_value (L"Branch",         L"Latest");
    // ^^^^ Add a key/value pair so that the section isn't purged on write
  }

  wchar_t wszCurrentBuild [128] = { };

  if (empty)
  {
    *wszCurrentBuild = L'\0';
    build.installed  = -1;
    wcscpy (build.branch, L"Latest");
  }

  else
  {
    swscanf ( installed_ver.get_value (L"InstallPackage").c_str (),
                L"%128[^,],%i",
                  wszCurrentBuild,
                    &build.installed );
    wcscpy (build.branch, installed_ver.get_value (L"Branch").c_str ());
  }

  // Set the reminder in case the update fails... we do not want
  //   the repository.ini file's updated timestamp to delay a second
  //     attempt by the user to upgrade.
  install_ini.import (L"[Update.User]\nReminder=0\n\n");
  install_ini.write  (SK_Version_GetInstallIniPath ().c_str ());

  iSK_INISection& latest_ver =
    repo_ini.get_section_f (L"Version.%s", build.branch);

  wchar_t wszFullDetails [4096];
  swprintf ( wszFullDetails,
              L"<a href=\"%s\">%s</a>\n\n"
              L"%s",
                latest_ver.get_value (L"ReleaseNotes").c_str    (),
                  latest_ver.get_value (L"Title").c_str         (),
                    latest_ver.get_value (L"Description").c_str () );

  swscanf ( latest_ver.get_value (L"InstallPackage").c_str (),
              L"%128[^,],%i",
                build.latest.package,
                  &build.latest.in_branch );

  wcscpy_s ( update_dlg_relnotes, INTERNET_MAX_PATH_LENGTH,
             latest_ver.get_value (L"ReleaseNotes").c_str () );

  if (build.latest.in_branch > build.installed || force)
  {
    iSK_INISection& archive =
      repo_ini.get_section_f ( L"Archive.%s",
                                 build.latest.package );

    auto* get =
      new sk_internet_get_t { };

    URL_COMPONENTSW urlcomps;

    RtlZeroMemory (get,         sizeof *get);
    RtlZeroMemory (&urlcomps,   sizeof URL_COMPONENTSW);

    urlcomps.dwStructSize     = sizeof URL_COMPONENTSW;

    urlcomps.lpszHostName     = get->wszHostName;
    urlcomps.dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;

    urlcomps.lpszUrlPath      = get->wszHostPath;
    urlcomps.dwUrlPathLength  = INTERNET_MAX_PATH_LENGTH;

    if ( InternetCrackUrl (        archive.get_value (L"URL").c_str  (),
                            (DWORD)archive.get_value (L"URL").length (),
                                     0x00,
                                       &urlcomps
                          )
       )
    {
      task_config.lpCallbackData = (LONG_PTR)get;

      wchar_t    wszUpdateFile     [MAX_PATH + 2] = { };
      wchar_t    wszUpdateTempFile [MAX_PATH + 2] = { };

      wcsncpy_s ( wszUpdateFile, MAX_PATH, SK_SYS_GetVersionPath ().c_str (),
                                _TRUNCATE );
      swprintf  ( wszUpdateTempFile,
                    L"%s%s.7z",
                      SK_SYS_GetVersionPath ().c_str (),
                        build.latest.package );

      wcsncpy_s ( get->wszLocalPath,  MAX_PATH,
                  wszUpdateTempFile, _TRUNCATE );

      task_config.pszExpandedInformation  = wszFullDetails;

      task_config.pszExpandedControlText  = L"Hide Details";
      task_config.pszCollapsedControlText = L"Show More Details";

      int nButton = 0;

      if (SUCCEEDED (SK_TaskDialogIndirect (&task_config, &nButton, nullptr, nullptr)))
      {
        if (get->status == sk_internet_get_t::STATUS_UPDATED)
        {
          auto *backup_pref =
            static_cast <sk::ParameterBool *> (
              g_UpdateParameterFactory->create_parameter <bool> (L"BackupFiles")
            );

          if (backup_pref != nullptr)
          {
            backup_pref->register_to_ini (
              &install_ini,
                L"Update.User",
                  L"BackupFiles" );

            if (! backup_pref->load (update_dlg_backup))
              update_dlg_backup = false;
          }

          auto *keep_pref =
            static_cast <sk::ParameterBool *> (
              g_UpdateParameterFactory->create_parameter <bool> (L"KeepDownloads")
            );

          if (keep_pref != nullptr)
          {
            keep_pref->register_to_ini (
              &install_ini,
                L"Update.User",
                  L"KeepDownloads" );

            if (! keep_pref->load (update_dlg_keep))
              update_dlg_keep = true;
          }

          wcsncpy_s ( update_dlg_file,  MAX_PATH, wszUpdateTempFile, _TRUNCATE );
          wcsncpy_s ( update_dlg_build, 127,      wszCurrentBuild,   _TRUNCATE );

          InterlockedExchangeAcquire ( &__SK_UpdateStatus, 0 );

          SK_Thread_Create ( UpdateDlg_Thread, HWND_DESKTOP );

          LONG status = 0;

          while ( ( status =
                      InterlockedCompareExchange ( &__SK_UpdateStatus,
                                                     0,
                                                       0 )
                  ) == 0
                )
          {
            MsgWaitForMultipleObjectsEx ( 0, nullptr, 15UL,
                                            QS_ALLEVENTS,
                                              MWMO_INPUTAVAILABLE );
          }

          if ( InterlockedCompareExchange ( &__SK_UpdateStatus, 0, 0 ) == 1 )
          {
            if (empty)
            {
              install_ini.import ( L"[Version.Local]\n"
                                   L"Branch=Latest\n"
                                   L"InstallPackage=Invalid,-1\n\n"

                                   L"[Update.User]\n"
                                   L"Frequency=6h\n"
                                   L"BackupFiles=false\n"
                                   L"KeepDownloads=true\n\n" );
            }

            keep_pref->store      (update_dlg_keep);
            backup_pref->store    (update_dlg_backup);

            installed_ver.get_value (L"InstallPackage") =
              latest_ver.get_value  (L"InstallPackage");

            // Remove reminder if we successfully install...
            install_ini.get_section (L"Update.User").remove_key (L"Reminder");
            install_ini.write       (SK_Version_GetInstallIniPath ().c_str ());

            if (SK_IsInjected ())
            {
              SK_Inject_Start ();
            }

            if (! SK_IsSuperSpecialK ())
              SK_RestartGame ();

            return S_OK;
          }

          // Update Failed
          else {
            return E_FAIL;
          }
        }

        else if (get->status == sk_internet_get_t::STATUS_CANCELLED)
        {
          DeleteFileW (SK_Version_GetInstallIniPath ().c_str ());
          return E_FAIL;
        }
      }

      delete get;
    }

    else
    {
      delete get;
      return E_FAIL;
    }
  }

  else if (install_ini.contains_section (L"Update.User"))
  {
    install_ini.get_section (L"Update.User").remove_key (L"Reminder");
    install_ini.write       (SK_Version_GetInstallIniPath ().c_str ());
  }

  return S_FALSE;
}

HRESULT
__stdcall
SK_UpdateSoftware (const wchar_t* wszProduct)
{
  HRESULT hr = SK_UpdateSoftware1 (wszProduct);

  return hr;
}


std::wstring SK_Network_MakeEscapeSequencedURL (std::wstring url)
{
  DWORD                                            dwLen = 4095;
  wchar_t                                 wszURL [4096]  = {  };
  InternetCanonicalizeUrlW (url.c_str (), wszURL, &dwLen, ICU_ENCODE_PERCENT);

  return wszURL;
}