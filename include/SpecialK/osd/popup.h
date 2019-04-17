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

#include <CEGUI/CEGUI.h>

class SK_PopupWindow {
friend class SK_PopupManager;

public:
  CEGUI::Window* getChild (const char* szName);
  operator CEGUI::Window* (void);

protected:
   SK_PopupWindow (const char* szLayout);
  ~SK_PopupWindow (void);

private:
  CEGUI::Window* window_;
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

  bool            OnDestroyPopup   (const CEGUI::EventArgs& e);

protected:
  SK_PopupManager (void);

private:
  static SK_PopupManager* __manager__;
  static CRITICAL_SECTION cs;

  SKTL_BidirectionalHashMap <
    SK_PopupWindow *, CEGUI::Window  *
  > popups_ { };

  CEGUI::GUIContext* gui_ctx_;
};

#endif /* __SK__OSD_POPUP_H__ */