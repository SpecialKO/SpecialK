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

#ifndef __SK__INPUT_H__
#define __SK__INPUT_H__

#include <SpecialK/utility/lazy_global.h>

#include <Unknwnbase.h>

#include <Windows.h>
#include <joystickapi.h>

#include <cstdint>
#include <assert.h>

extern LARGE_INTEGER SK_QueryPerf (void) noexcept;
extern int64_t       SK_PerfFreq;

#define SK_LOG_INPUT_CALL { static int  calls  = 0; { SK_LOG0 ( (L"[!] > Call #%lu: %hs", calls++, __FUNCTION__), L"Input Mgr." ); } }

bool SK_ImGui_WantGamepadCapture  (void);
bool SK_ImGui_WantMouseCapture    (void);
bool SK_ImGui_WantMouseCaptureEx  (DWORD dwReasonMask = 0xFFFF);
bool SK_ImGui_WantKeyboardCapture (void);
bool SK_ImGui_WantTextCapture     (void);
void SK_ImGui_UpdateMouseTracker  (void);

bool SK_WantBackgroundRender (void);

void SK_Input_HookDI8         (void);
void SK_Input_HookHID         (void);

void SK_Input_HookXInput1_3   (void);
void SK_Input_HookXInput1_4   (void);
void SK_Input_HookXInput9_1_0 (void);
void SK_Input_HookScePad      (void);
void SK_Input_HookWGI         (void);

void SK_Input_PreHookXInput   (void);
void SK_Input_PreHookScePad   (void);

void SK_Input_PreInit (void);
void SK_Input_Init    (void);

void SK_Input_SetLatencyMarker (void) noexcept;

// SK doesn't use SDL, but many games crash on exit due to polling
//   joystick after XInput is unloaded... so we'll just terminate
//     the thread manually so it doesn't crash.
void SK_SDL_ShutdownInput (void);


SHORT WINAPI SK_GetAsyncKeyState (int vKey);


enum class sk_cursor_state {
  None    = 0,
  Visible = 1,
  Hidden  = 2
};

struct sk_imgui_cursor_s
{
  HWND    child_input   =   HWND_DESKTOP;
  RECT    child_client  = { 0, 0, 0, 0 };
  RECT    child_rect    = { 0, 0, 0, 0 };

  HCURSOR real_img      =        nullptr;
  POINT   orig_pos      =       { 0, 0 };
  bool    orig_vis      =          false;

  HCURSOR img           =        nullptr;
  POINT   pos           =       { 0, 0 };

  bool    visible       =          false;
  bool    idle          =          false; // Hasn't moved
  DWORD   last_move     =       MAXDWORD;
  DWORD   refs_added    =              0;

  sk_cursor_state force = sk_cursor_state::None;

  void    showSystemCursor (bool system = true);
  void    showImGuiCursor  (void);

  void    update           (void);

  void    LocalToScreen    (LPPOINT lpPoint);
  void    LocalToClient    (LPPOINT lpPoint);
  void    ClientToLocal    (LPPOINT lpPoint);
  void    ScreenToLocal    (LPPOINT lpPoint);

  void    activateWindow   (bool active);

  struct {
    struct {
      bool visible = false;
      bool gamepad = false; // TODO - Disable warping if gamepad is plugged in
      bool ui_open = true;
    } no_warp;
  } prefs;
};

extern sk_imgui_cursor_s SK_ImGui_Cursor;


enum class sk_input_dev_type {
  Mouse    = 1,
  Keyboard = 2,
  Gamepad  = 4,
  Other    = 8,

  // Steam Input sub-types
  Gamepad_Xbox        = 16,
  Gamepad_PlayStation = 17,
  Gamepad_Generic     = 18,
  Gamepad_Nintendo    = 19

};


enum class sk_win32_func {
  GetCursorPos     = 1,
  GetKeyState      = 2,
  GetKeyboardState = 4,
  GetAsyncKeystate = 8
};


struct sk_input_api_context_s
{
  constexpr sk_input_api_context_s (void) noexcept { };

  volatile LONG reads  [4] = { },
                writes [4] = { };

  struct {
    volatile LONG reads  [4] = { },
                  writes [4] = { };
  } last_frame;

  struct {
    union {
      bool keyboard;
      bool gamepad_xbox;
    };
    union {
      bool mouse;
      bool gamepad_playstation;
    };
    union {
      bool gamepad;
      bool gamepad_generic;
    };
    union {
      bool other;
      bool gamepad_nintendo;
    };
  } active { false, false, false, false };

  struct {
    union {
      volatile uint64_t keyboard;
      volatile uint64_t gamepad_xbox;
    };
    union {
      volatile uint64_t mouse;
      volatile uint64_t gamepad_playstation;
    };
    union {
      volatile uint64_t gamepad;
      volatile uint64_t gamepad_generic;
    };
    union {
      volatile uint64_t other;
      volatile uint64_t gamepad_nintendo;
    };
  } viewed { 0ULL, 0ULL, 0ULL, 0ULL }; // Data was processed by the game at this time (QPC)

  void markRead  (sk_input_dev_type type) noexcept
  { SK_Input_SetLatencyMarker ();
    InterlockedIncrement (&last_frame.reads    [ type == sk_input_dev_type::Mouse    ? 0 :
                                                 type == sk_input_dev_type::Keyboard ? 1 :
                                                 type == sk_input_dev_type::Gamepad  ? 2 : 3 ] ); }
  void markRead  (sk_win32_func type) noexcept
  { InterlockedIncrement (&last_frame.reads    [ type == sk_win32_func::GetCursorPos     ? 0 :
                                                 type == sk_win32_func::GetKeyState      ? 1 :
                                                 type == sk_win32_func::GetKeyboardState ? 2 : 3 ] ); }
  void markRead  (DWORD slot) noexcept
  { SK_Input_SetLatencyMarker ();
    InterlockedIncrement (&last_frame.reads    [ slot ]); }
  void markWrite  (sk_input_dev_type type) noexcept
  { InterlockedIncrement (&last_frame.writes  [ type == sk_input_dev_type::Mouse    ? 0 :
                                                type == sk_input_dev_type::Keyboard ? 1 :
                                                type == sk_input_dev_type::Gamepad  ? 2 : 3 ] ); }
  void markViewed (sk_input_dev_type type) noexcept
  {
    const auto perfNow =
      sk::narrow_cast <uint64_t> (SK_QueryPerf ().QuadPart);

    switch (type)
    {
      case sk_input_dev_type::Other:               WriteULong64Release (&viewed.other,               perfNow); break;
      case sk_input_dev_type::Mouse:               WriteULong64Release (&viewed.mouse,               perfNow); break;
      case sk_input_dev_type::Keyboard:            WriteULong64Release (&viewed.keyboard,            perfNow); break;
      case sk_input_dev_type::Gamepad:             WriteULong64Release (&viewed.gamepad,             perfNow); break;
      case sk_input_dev_type::Gamepad_Xbox:        WriteULong64Release (&viewed.gamepad_xbox,        perfNow);
                                                  InterlockedIncrement (&last_frame.reads [0]);      SK_Input_SetLatencyMarker (); break;
      case sk_input_dev_type::Gamepad_PlayStation: WriteULong64Release (&viewed.gamepad_playstation, perfNow);
                                                  InterlockedIncrement (&last_frame.reads [1]);      SK_Input_SetLatencyMarker (); break;
      case sk_input_dev_type::Gamepad_Generic:     WriteULong64Release (&viewed.gamepad_generic,     perfNow);
                                                  InterlockedIncrement (&last_frame.reads [2]);      SK_Input_SetLatencyMarker (); break;
      case sk_input_dev_type::Gamepad_Nintendo:    WriteULong64Release (&viewed.gamepad_nintendo,    perfNow);
                                                  InterlockedIncrement (&last_frame.reads [3]);      SK_Input_SetLatencyMarker (); break;
    }
  }

  bool nextFrameWin32 (void)
  {
    bool active_data = false;

    active.keyboard = false; active.gamepad = false;
    active.mouse    = false; active.other   = false;


    InterlockedAdd  (&reads   [0], last_frame.reads  [0]);
    InterlockedAdd  (&writes  [0], last_frame.writes [0]);

      if (ReadAcquire (&last_frame.reads  [0]))  { active.mouse = true; active_data = true; }
      if (ReadAcquire (&last_frame.writes [0]))  { active.mouse = true; active_data = true; }

    InterlockedAdd  (&reads   [1], last_frame.reads  [1]);
    InterlockedAdd  (&writes  [1], last_frame.writes [1]);

      if (ReadAcquire (&last_frame.reads  [1]))  { active.keyboard = true; active_data = true; }
      if (ReadAcquire (&last_frame.writes [1]))  { active.keyboard = true; active_data = true; }

    InterlockedAdd  (&reads   [2], last_frame.reads  [2]);
    InterlockedAdd  (&writes  [2], last_frame.writes [2]);

      if (ReadAcquire (&last_frame.reads  [2]))  { active.keyboard = true; active_data = true; }
      if (ReadAcquire (&last_frame.writes [2]))  { active.keyboard = true; active_data = true; }

    InterlockedAdd  (&reads   [3], last_frame.reads  [3]);
    InterlockedAdd  (&writes  [3], last_frame.writes [3]);

      if (ReadAcquire (&last_frame.reads  [3]))  { active.keyboard = true; active_data = true; }
      if (ReadAcquire (&last_frame.writes [3]))  { active.keyboard = true; active_data = true; }


    InterlockedExchange (&last_frame.reads  [0], 0);   InterlockedExchange (&last_frame.reads  [1], 0);
    InterlockedExchange (&last_frame.writes [0], 0);   InterlockedExchange (&last_frame.writes [1], 0);
    InterlockedExchange (&last_frame.reads  [2], 0);   InterlockedExchange (&last_frame.reads  [3], 0);
    InterlockedExchange (&last_frame.writes [2], 0);   InterlockedExchange (&last_frame.writes [3], 0);


    return active_data;
  }

  bool nextFrame (void)
  {
    bool active_data = false;


    active.keyboard = false; active.gamepad = false;
    active.mouse    = false; active.other   = false;


    InterlockedAdd  (&reads   [0], last_frame.reads  [0]);
    InterlockedAdd  (&writes  [0], last_frame.writes [0]);

      if (ReadAcquire (&last_frame.reads  [0]))  { active.keyboard = true; active_data = true; }
      if (ReadAcquire (&last_frame.writes [0]))  { active.keyboard = true; active_data = true; }

    InterlockedAdd  (&reads   [1], last_frame.reads  [1]);
    InterlockedAdd  (&writes  [1], last_frame.writes [1]);

      if (ReadAcquire (&last_frame.reads  [1]))  { active.mouse    = true; active_data = true; }
      if (ReadAcquire (&last_frame.writes [1]))  { active.mouse    = true; active_data = true; }

    InterlockedAdd  (&reads   [2], last_frame.reads  [2]);
    InterlockedAdd  (&writes  [2], last_frame.writes [2]);

      if (ReadAcquire (&last_frame.reads  [2]))  { active.gamepad  = true; active_data = true; }
      if (ReadAcquire (&last_frame.writes [2]))  { active.gamepad  = true; active_data = true; }

    InterlockedAdd  (&reads   [3], last_frame.reads  [3]);
    InterlockedAdd  (&writes  [3], last_frame.writes [3]);

      if (ReadAcquire (&last_frame.reads  [3]))  { active.other    = true; active_data = true; }
      if (ReadAcquire (&last_frame.writes [3]))  { active.other    = true; active_data = true; }


    InterlockedExchange (&last_frame.reads  [0], 0);   InterlockedExchange (&last_frame.reads  [1], 0);
    InterlockedExchange (&last_frame.writes [0], 0);   InterlockedExchange (&last_frame.writes [1], 0);
    InterlockedExchange (&last_frame.reads  [2], 0);   InterlockedExchange (&last_frame.reads  [3], 0);
    InterlockedExchange (&last_frame.writes [2], 0);   InterlockedExchange (&last_frame.writes [3], 0);


    return active_data;
  }

  // Age in seconds since the game last saw input for this class of device
  float getInputAge (sk_input_dev_type devType)
  {
    uint64_t perfSample = 0ULL;

    switch (devType)
    {
      case sk_input_dev_type::Mouse:               perfSample = ReadULong64Acquire (&viewed.mouse);               break;
      case sk_input_dev_type::Keyboard:            perfSample = ReadULong64Acquire (&viewed.keyboard);            break;
      case sk_input_dev_type::Gamepad:             perfSample = ReadULong64Acquire (&viewed.gamepad);             break;
      case sk_input_dev_type::Gamepad_Xbox:        perfSample = ReadULong64Acquire (&viewed.gamepad_xbox);        break;
      case sk_input_dev_type::Gamepad_PlayStation: perfSample = ReadULong64Acquire (&viewed.gamepad_playstation); break;
      case sk_input_dev_type::Gamepad_Generic:     perfSample = ReadULong64Acquire (&viewed.gamepad_generic);     break;
      case sk_input_dev_type::Gamepad_Nintendo:    perfSample = ReadULong64Acquire (&viewed.gamepad_nintendo);    break;
      case sk_input_dev_type::Other:               perfSample = ReadULong64Acquire (&viewed.other);               break;
    }

    return static_cast <float> (
      static_cast <double> (SK_QueryPerf ().QuadPart - perfSample) /
      static_cast <double> (SK_PerfFreq)
                               );
  }

  // XInput only
  float getInputAge (DWORD devSlot)
  {
    assert (devSlot <= 4UL);

    uint64_t perfSample = 0ULL;

    switch (devSlot)
    {
      case 0: perfSample = ReadULong64Acquire (&viewed.mouse);    break;
      case 1: perfSample = ReadULong64Acquire (&viewed.keyboard); break;
      case 2: perfSample = ReadULong64Acquire (&viewed.gamepad);  break;
      case 3: perfSample = ReadULong64Acquire (&viewed.other);    break;

      // Returns the newest input on any slot
      case 4:
      {
        return
          std::min (     getInputAge (0),
            std::min (   getInputAge (1),
              std::min ( getInputAge (2), getInputAge (3) ) ) );
      } break;

      default: return -INFINITY;
    }

    return static_cast <float> (
      static_cast <double> (SK_QueryPerf ().QuadPart - perfSample) /
      static_cast <double> (SK_PerfFreq)
                               );
  }
};

extern SK_LazyGlobal <sk_input_api_context_s> SK_XInput_Backend;
extern SK_LazyGlobal <sk_input_api_context_s> SK_ScePad_Backend;
extern SK_LazyGlobal <sk_input_api_context_s> SK_Steam_Backend;
extern SK_LazyGlobal <sk_input_api_context_s> SK_WGI_Backend;
extern SK_LazyGlobal <sk_input_api_context_s> SK_DI8_Backend;
extern SK_LazyGlobal <sk_input_api_context_s> SK_DI7_Backend;
extern SK_LazyGlobal <sk_input_api_context_s> SK_HID_Backend;
extern SK_LazyGlobal <sk_input_api_context_s> SK_Win32_Backend;
extern SK_LazyGlobal <sk_input_api_context_s> SK_WinMM_Backend;
extern SK_LazyGlobal <sk_input_api_context_s> SK_WinHook_Backend;    // (Low-Level) KB/M Hook
extern SK_LazyGlobal <sk_input_api_context_s> SK_RawInput_Backend;
extern SK_LazyGlobal <sk_input_api_context_s> SK_MessageBus_Backend; // NVIDIA stuff



enum class SK_Input_BindFlags
{
  Invalid           =    0,

  NonExclusive      = ( 1UL << 0 ),
     Exclusive      = ( 1UL << 1 ),
  ModeAgnostic      = ( NonExclusive  |
                        Exclusive     ),

  ActivateOnPress   = ( 1UL << 2 ),
  ActivateOnRelease = ( 1UL << 3 ),

  Repeatable        = ( 1UL << 4 ),

  ReleaseOnActivate = ( 1UL << 5 ), // Immediately behave as though the buttons were released
  HoldOnActivate    = ( 1UL << 6 ), // Immediately behave as though the buttons are held

  Default           = ( ModeAgnostic    |
                        ActivateOnPress )
};

enum class SK_Input_ModifierKeys
{
  None        =    0,


  Left_Ctrl   = ( 1UL << 0 ),
  Right_Ctrl  = ( 1UL << 1 ),

  Left_Alt    = ( 1UL << 2 ),
  Right_Alt   = ( 1UL << 3 ),

  Left_Shift  = ( 1UL << 4 ),
  Right_Shift = ( 1UL << 5 ),


  Ctrl        = ( Left_Ctrl |
                 Right_Ctrl ),

  Alt         = ( Left_Alt |
                 Right_Alt ),

  Shift       = ( Left_Shift |
                 Right_Shift )
};


struct SK_Input_Duration
{
  uint16_t min = 0,
           max = 0;
};

struct SK_Input_KeyBinding
{
  SK_Input_ModifierKeys modifiers  = SK_Input_ModifierKeys::None;
  SK_Input_BindFlags    flags      = SK_Input_BindFlags::Default;

  struct key_s
  {
    SK_Input_Duration   duration   = {    };
    uint16_t            scancode   = 0x0000;
  } keys [1];

  uint8_t               combo_size = 1;
};

struct SK_Input_PadBinding
{
  SK_Input_BindFlags flags       = SK_Input_BindFlags::Default;

  struct button_s
  {
    SK_Input_Duration duration   = {};
    uint8_t           button     = 00;
  } buttons [1];

  uint8_t             combo_size = 1;
};


class SK_Input_KeyBindFactory
{

};

class SK_Input_PadBindFactory
{

};


struct SK_ImGui_InputLanguage_s
{
  // Cause the keybd_layout to be populated the
  //   first time update (...) is called
  static bool changed;
  static HKL  keybd_layout;

  void update (void);
};

__forceinline
HKL
SK_Input_GetKeyboardLayout (void) noexcept
{
  return
    SK_ImGui_InputLanguage_s::keybd_layout;
}

bool
SK_ImGui_HandlesMessage (LPMSG lpMsg, bool remove, bool peek);


using keybd_event_pfn = void (WINAPI *)(
  _In_ BYTE      bVk,
  _In_ BYTE      bScan,
  _In_ DWORD     dwFlags,
  _In_ ULONG_PTR dwExtraInfo
);

using SetCursor_pfn = HCURSOR (WINAPI *)(HCURSOR hCursor);
using GetCursor_pfn = HCURSOR (WINAPI *)(VOID);

using ShowCursor_pfn = int (WINAPI *)(BOOL);

extern keybd_event_pfn keybd_event_Original;
extern SetCursor_pfn   SetCursor_Original;
extern GetCursor_pfn   GetCursor_Original;

#include <cstdint>

extern
void
SK_ImGui_CenterCursorOnWindow (void);

//////////////////////////////////////////////////////////////
//
// HIDClass (Usermode)
//
//////////////////////////////////////////////////////////////

#include <Hidsdi.h>
#include <Windows.h>
#include <winnt.h>
#include <hidusage.h>
#include <Hidpi.h>

using HidP_GetCaps_pfn = NTSTATUS (__stdcall *)(
  _In_  PHIDP_PREPARSED_DATA PreparsedData,
  _Out_ PHIDP_CAPS           Capabilities
);

using HidD_GetPreparsedData_pfn = BOOLEAN (__stdcall *)(
  _In_  HANDLE                HidDeviceObject,
  _Out_ PHIDP_PREPARSED_DATA *PreparsedData
);

using HidD_FreePreparsedData_pfn = BOOLEAN (__stdcall *)(
  _In_ PHIDP_PREPARSED_DATA PreparsedData
);

using HidP_GetData_pfn = NTSTATUS (__stdcall *)(
  _In_    HIDP_REPORT_TYPE     ReportType,
  _Out_   PHIDP_DATA           DataList,
  _Inout_ PULONG               DataLength,
  _In_    PHIDP_PREPARSED_DATA PreparsedData,
  _In_    PCHAR                Report,
  _In_    ULONG                ReportLength
);

using HidD_GetFeature_pfn = BOOLEAN (__stdcall *)(
  _In_  HANDLE HidDeviceObject,
  _Out_ PVOID  ReportBuffer,
  _In_  ULONG  ReportBufferLength
);

extern HidP_GetCaps_pfn           HidP_GetCaps_Original          ;
extern HidD_GetPreparsedData_pfn  HidD_GetPreparsedData_Original ;
extern HidD_FreePreparsedData_pfn HidD_FreePreparsedData_Original;
extern HidD_GetFeature_pfn        HidD_GetFeature_Original       ;
extern HidP_GetData_pfn           HidP_GetData_Original          ;
extern SetCursor_pfn              SetCursor_Original             ;


BOOLEAN
WINAPI
SK_HidD_GetPreparsedData (_In_  HANDLE                HidDeviceObject,
                          _Out_ PHIDP_PREPARSED_DATA *PreparsedData);
BOOLEAN
WINAPI
SK_HidD_FreePreparsedData (_In_ PHIDP_PREPARSED_DATA PreparsedData);

NTSTATUS
WINAPI
SK_HidP_GetCaps (_In_  PHIDP_PREPARSED_DATA PreparsedData,
                 _Out_ PHIDP_CAPS           Capabilities);

BOOL
WINAPI
SK_DeviceIoControl (HANDLE       hDevice,
                    DWORD        dwIoControlCode,
                    LPVOID       lpInBuffer,
                    DWORD        nInBufferSize,
                    LPVOID       lpOutBuffer,
                    DWORD        nOutBufferSize,
                    LPDWORD      lpBytesReturned,
                    LPOVERLAPPED lpOverlapped);



// Temporarily override game's preferences for input device window message generation
bool
SK_RawInput_EnableLegacyMouse (bool enable);

// Restore the game's original setting
void
SK_RawInput_RestoreLegacyMouse (void);

// Temporarily override game's preferences for input device window message generation
bool
SK_RawInput_EnableLegacyKeyboard (bool enable);

// Restore the game's original setting
void
SK_RawInput_RestoreLegacyKeyboard (void);


using GetRegisteredRawInputDevices_pfn = UINT (WINAPI *)(
  _Out_opt_ PRAWINPUTDEVICE pRawInputDevices,
  _Inout_   PUINT           puiNumDevices,
  _In_      UINT            cbSize
);extern
      GetRegisteredRawInputDevices_pfn
      GetRegisteredRawInputDevices_Original;

extern LONG  SK_RawInput_MouseX;
extern LONG  SK_RawInput_MouseY;
extern POINT SK_RawInput_Mouse;


UINT
WINAPI
SK_GetRawInputData ( HRAWINPUT hRawInput,
                     UINT      uiCommand,
                     LPVOID    pData,
                     PUINT     pcbSize,
                     UINT      cbSizeHeader );

UINT
WINAPI
SK_GetRegisteredRawInputDevices ( PRAWINPUTDEVICE pRawInputDevices,
                                  PUINT           puiNumDevices,
                                  UINT            cbSize );

BOOL
WINAPI
SK_RegisterRawInputDevices ( PCRAWINPUTDEVICE pRawInputDevices,
                             UINT             uiNumDevices,
                             UINT             cbSize );



HCURSOR
WINAPI
SK_GetCursor (VOID);

HCURSOR
WINAPI
SK_SetCursor (_In_opt_ HCURSOR hCursor);

SHORT WINAPI SK_GetKeyState      (int   nVirtKey);
BOOL  WINAPI SK_GetKeyboardState (PBYTE lpKeyState);

extern char SK_KeyMap_LeftHand_Arrow (char key);

extern DWORD SK_GetCurrentMS (void) noexcept;

UINT WINAPI SK_joyGetNumDevs  (void);
UINT WINAPI SK_joyGetPosEx    (UINT,LPJOYINFOEX);
UINT WINAPI SK_joyGetDevCapsW (UINT_PTR,LPJOYCAPSW,UINT);

void
WINAPI
SK_keybd_event (
  _In_ BYTE       bVk,
  _In_ BYTE       bScan,
  _In_ DWORD     dwFlags,
  _In_ ULONG_PTR dwExtraInfo
);

int WINAPI SK_ShowCursor (BOOL bShow);

bool SK_InputUtil_IsHWCursorVisible (void);
bool SK_Window_IsCursorActive       (void);

enum SK_InputEnablement {
  Enabled              = 0,
  Disabled             = 1,
  DisabledInBackground = 2
};

using CreateFile2_pfn =
  HANDLE (WINAPI *)(LPCWSTR,DWORD,DWORD,DWORD,
                      LPCREATEFILE2_EXTENDED_PARAMETERS);

using CreateFileW_pfn =
  HANDLE (WINAPI *)(LPCWSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,
                      DWORD,DWORD,HANDLE);

using CreateFileA_pfn =
  HANDLE (WINAPI *)(LPCSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,
                      DWORD,DWORD,HANDLE);

using ReadFile_pfn =
  BOOL (WINAPI *)(HANDLE,LPVOID,DWORD,LPDWORD,LPOVERLAPPED);

using ReadFileEx_pfn =
  BOOL (WINAPI *)(HANDLE,LPVOID,DWORD,LPOVERLAPPED,
                    LPOVERLAPPED_COMPLETION_ROUTINE);

using OpenFileMappingW_pfn =
  HANDLE (WINAPI *)(DWORD,BOOL,LPCWSTR);

using CreateFileMappingW_pfn =
  HANDLE (WINAPI *)(HANDLE,LPSECURITY_ATTRIBUTES,DWORD,
                     DWORD,DWORD,LPCWSTR);

using DeviceIoControl_pfn =
BOOL (WINAPI *)(HANDLE       hDevice,
                DWORD        dwIoControlCode,
                LPVOID       lpInBuffer,
                DWORD        nInBufferSize,
                LPVOID       lpOutBuffer,
                DWORD        nOutBufferSize,
                LPDWORD      lpBytesReturned,
                LPOVERLAPPED lpOverlapped);

using GetOverlappedResult_pfn =
BOOL (WINAPI *)(HANDLE       hFile,
                LPOVERLAPPED lpOverlapped,
                LPDWORD      lpNumberOfBytesTransferred,
                BOOL         bWait);

using GetOverlappedResultEx_pfn =
BOOL (WINAPI *)(HANDLE       hFile,
                LPOVERLAPPED lpOverlapped,
                LPDWORD      lpNumberOfBytesTransferred,
                DWORD        dwMilliseconds,
                BOOL         bWait);

using joyGetNumDevs_pfn  = MMRESULT (WINAPI *)(void);
using joyGetPos_pfn      = MMRESULT (WINAPI *)(UINT,LPJOYINFO);
using joyGetPosEx_pfn    = MMRESULT (WINAPI *)(UINT,LPJOYINFOEX);
using joyGetDevCapsW_pfn = MMRESULT (WINAPI *)(UINT_PTR,LPJOYCAPSW,UINT);

extern joyGetPos_pfn   joyGetPos_Original;
extern joyGetPosEx_pfn joyGetPosEx_Original;

#endif /* __SK__INPUT_H__ */
