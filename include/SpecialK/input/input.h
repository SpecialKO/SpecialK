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

#include <Windows.h>

bool SK_ImGui_WantGamepadCapture  (void);
bool SK_ImGui_WantMouseCapture    (void);
bool SK_ImGui_WantKeyboardCapture (void);
bool SK_ImGui_WantTextCapture     (void);

void SK_Input_HookDI8         (void);
void SK_Input_HookHID         (void);

void SK_Input_HookXInput1_3   (void);
void SK_Input_HookXInput1_4   (void);
void SK_Input_HookXInput9_1_0 (void);

void SK_Input_PreHookXInput   (void);

void SK_Input_Init (void);


struct sk_imgui_cursor_s
{
  HCURSOR orig_img   =      NULL;
  POINT   orig_pos   =  { 0, 0 };
  bool    orig_vis   =     false;
                       
  HCURSOR img        =      NULL;
  POINT   pos        =  { 0, 0 };
                       
  bool    visible    =     false;
  bool    idle       =     false; // Hasn't moved
  DWORD   last_move  =  MAXDWORD;

  void    showSystemCursor (bool system = true);
  void    showImGuiCursor  (void);

  void    update           (void);

  void   LocalToScreen    (LPPOINT lpPoint);
  void   LocalToClient    (LPPOINT lpPoint);
  void   ClientToLocal    (LPPOINT lpPoint);
  void   ScreenToLocal    (LPPOINT lpPoint);

  void   activateWindow   (bool active);

  struct {
    struct {
      bool visible = true;
      bool gamepad = false; // TODO - Disable warping ifgamepad is plugged in
      bool ui_open = true;
    } no_warp;
  } prefs;
} extern SK_ImGui_Cursor;


struct sk_input_api_context_s
{
  volatile LONG reads, writes;

  struct {
    volatile LONG reads, writes;
  } last_frame;

  void markRead  (void) { InterlockedIncrement (&last_frame.reads);  }
  void markWrite (void) { InterlockedIncrement (&last_frame.writes); }

  bool nextFrame (void) {
    InterlockedAdd (&reads, last_frame.reads);
    InterlockedAdd (&writes, last_frame.writes);

    bool active = InterlockedAdd (&last_frame.reads, 0) || InterlockedAdd (&last_frame.writes, 0);

    InterlockedExchange (&last_frame.reads,  0);
    InterlockedExchange (&last_frame.writes, 0);

    return active;
  }
};

extern sk_input_api_context_s SK_XInput_Backend;
extern sk_input_api_context_s SK_DI8_Backend;
extern sk_input_api_context_s SK_HID_Backend;
extern sk_input_api_context_s SK_Win32_Backend;
extern sk_input_api_context_s SK_RawInput_Backend;

extern sk_input_api_context_s SK_WinMM_Backend;
extern sk_input_api_context_s SK_Steam_Backend;


bool
SK_ImGui_HandlesMessage (LPMSG lpMsg, bool remove);


typedef void (WINAPI *keybd_event_pfn)(
    _In_ BYTE bVk,
    _In_ BYTE bScan,
    _In_ DWORD dwFlags,
    _In_ ULONG_PTR dwExtraInfo );

extern keybd_event_pfn keybd_event_Original;

#endif /* __SK__INPUT_H__ */
