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

#ifndef __SK__IMGUI_POPUP_H__
#define __SK__IMGUI_POPUP_H__

struct IUnknown;
#include <Unknwnbase.h>

#include <imgui/imgui.h>
#include <input/input.h>
#include <SpecialK/config.h>
#include <SpecialK/parameter.h>
#include <SpecialK/ini.h>

#include <string>
#include <map>

#include <SpecialK/utility/bidirectional_map.h>

struct ImGuiWindow;
//class  SK_Widget;

class SK_ImGui_PopupWindow
{ friend class SK_ImGui_PopupManager;

public:
           ImGuiWindow* getChild (const char *szName);
  operator ImGuiWindow*          (       void       );

protected:
   SK_ImGui_PopupWindow (const char *szLayout);
  ~SK_ImGui_PopupWindow (        void        );

private:
  ImGuiWindow* window_;
};

class SK_ImGui_PopupManager {
public:
  SK_ImGui_PopupWindow*  createPopup      (const char           *szLayout);
  bool                   isPopup          (SK_ImGui_PopupWindow *popup);
  void                   destroyPopup     (SK_ImGui_PopupWindow *popup);
  void                   destroyAllPopups (void);

  bool                   tryLockPopups    (void);
  void                   lockPopups       (void);
  void                   unlockPopups     (void);

  void                   drawAllPopups    (void);

//////
//bool                   OnDestroyPopup   (const ImGui::EventArgs& e);
//////

  static
  SK_ImGui_PopupManager* getInstance      (void);

protected:
  SK_ImGui_PopupManager (void);

private:
  static SK_ImGui_PopupManager *__manager__;
  static CRITICAL_SECTION       __critsec__;

  SKTL_BidirectionalHashMap <
    SK_ImGui_PopupWindow*,
             ImGuiWindow*   >   popups_ { };

  ImGuiContext                 *imgui_ctx_;
};

#endif /* __SK__IMGUI_POPUP_H__ */