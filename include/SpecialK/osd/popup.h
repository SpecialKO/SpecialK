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

#ifndef __SK__OSD_POPUP_H__
#define __SK__OSD_POPUP_H__

class SK_PopupWindow {
friend class SK_PopupManager;

public:
  /*CEGUI::Window*/void* getChild (const char* szName);
  operator /*CEGUI::Window*/void* (void);

protected:
   SK_PopupWindow (const char* szLayout);
  ~SK_PopupWindow (void);

private:
  /*CEGUI::Window*/void* window_;
};

class SK_PopupManager {
public:
  static SK_PopupManager* getInstance (void);

  SK_PopupWindow* createPopup      (const char* szLayout);
  void            destroyPopup     (SK_PopupWindow* popup);
  void            destroyAllPopups (void);

  bool            tryLockPopups    (void);
  void            lockPopups       (void);
  void            unlockPopups     (void);

  void            drawAllPopups    (void);
  bool            isPopup          (SK_PopupWindow* popup);

  bool            OnDestroyPopup   (const void* e);

protected:
  SK_PopupManager (void);

private:
  static SK_PopupManager* __manager__;
  static CRITICAL_SECTION cs;

  SKTL_BidirectionalHashMap <
    SK_PopupWindow *, /*CEGUI::Window*/void*
  > popups_ { };

  /*CEGUI::GUIContext*/void* gui_ctx_;
};

#include <control_panel.h>
#include <concurrent_queue.h>
#include <imgui/font_awesome.h>

struct SK_ImGui_Toast {
  enum Type {
    Success,
    Warning,
    Error,
    Info,
    Other
  } type;

  enum Flags {
    UseDuration = 0x01,
    ShowCaption = 0x02,
    ShowTitle   = 0x04,
    ShowOnce    = 0x08,
    ShowNewest  = 0x10,
    ShowDismiss = 0x20
  } flags;

  std::string id        = "";
  std::string caption   = "";
  std::string title     = "";

  DWORD       duration  = 0;
  DWORD       displayed = 0;
  DWORD       inserted  = 0;

  BOOL        finished  = false;
};

static inline ImVec4 SK_ImGui_GetToastColor (SK_ImGui_Toast::Type type)
{
  switch (type)
  {
    case SK_ImGui_Toast::Success:
      return ImVec4 (0.f, 1.f, 0.f, 1.f);
    case SK_ImGui_Toast::Warning:
      return ImVec4 (1.f, 1.f, 0.f, 1.f);
    case SK_ImGui_Toast::Error:
      return ImVec4 (1.f, 0.f, 0.f, 1.f);
    case SK_ImGui_Toast::Info:
      return ImVec4 (0.f, 0.616f, 1.f, 1.f);
    case SK_ImGui_Toast::Other:
    default:
      return ImVec4 (1.f, 1.f, 1.f, 1.f);
  }
}

static inline const char* SK_ImGui_GetToastIcon (SK_ImGui_Toast::Type type)
{
  switch (type)
  {
    case SK_ImGui_Toast::Success:
      return ICON_FA_CHECK_CIRCLE;
    case SK_ImGui_Toast::Warning:
      return ICON_FA_EXCLAMATION_TRIANGLE;
    case SK_ImGui_Toast::Error:
      return ICON_FA_EXCLAMATION_CIRCLE;
    case SK_ImGui_Toast::Info:
      return ICON_FA_INFO_CIRCLE;
    case SK_ImGui_Toast::Other:
    default:
      return "";
  }
}

bool
SK_ImGui_CreateNotification ( const char* szID,
                     SK_ImGui_Toast::Type type,
                              const char* szCaption,
                              const char* szTitle,
                                    DWORD dwMilliseconds,
                                    DWORD flags = SK_ImGui_Toast::UseDuration |
                                                  SK_ImGui_Toast::ShowCaption |
                                                  SK_ImGui_Toast::ShowTitle );

void
SK_ImGui_DrawNotifications (void);

#endif /* __SK__OSD_POPUP_H__ */