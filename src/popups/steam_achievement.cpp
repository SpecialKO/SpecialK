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
#include <SpecialK/popups/popup.h>

class SK_ImGui_AchievementPopup : SK_ImGui_PopupWindow {
public:
  void draw              (void);
  void setPos            (void);
  void triggerScreenshot (void);

protected:
  struct config_s {
    struct {
      bool    show_global;
      bool    show_friend;
    } rarity;

    struct {
      bool    show_title;
      bool    show_details;
    } description;

    struct {
      bool    show_icon;
    } images;

    struct {
      bool    show_progress_bar;
      bool    use_gradient;
    } progress;

    struct {
      ImFont* heading;
      ImFont* description;
      ImFont* progress;
      ImFont* statistics;
    } fonts;

    struct {
      struct { float width, height; } ideal;
      float                             dpi;
    } scale;
  } static config;

private:
  ImTextureID   lock_img;
  ImTextureID unlock_img;

  struct {
    bool       unlocked;
    __time64_t unlock_time;

    struct {
      float currentProgress;
      float maxProgress;

      float getPercent (void)
      {
        return
          ( maxProgress / currentProgress ) * 100.0f;
      }
    } progress;
  } state;

  struct {
    std::string desc;
    std::string title;
  } text;

  struct {
    int       ach_id;
    struct {
      AppId_t app_id;
    } steam;
  } platform;
};

SK_ImGui_AchievementPopup::config_s SK_ImGui_AchievementPopup::config;