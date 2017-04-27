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


bool
SK_ImGui_HandlesMessage (LPMSG lpMsg, bool remove);


typedef void (WINAPI *keybd_event_pfn)(
    _In_ BYTE bVk,
    _In_ BYTE bScan,
    _In_ DWORD dwFlags,
    _In_ ULONG_PTR dwExtraInfo );

extern keybd_event_pfn keybd_event_Original;

#endif /* __SK__INPUT_H__ */
