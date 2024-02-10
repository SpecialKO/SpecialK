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


#ifdef _HAS_CEGUI_REPLACEMENT
SK_PopupWindow::SK_PopupWindow (const char* szLayout)
{
  if (! (config.cegui.enable && config.cegui.frames_drawn > 0)) return;


  try
  {
    CEGUI::WindowManager& window_mgr =
      CEGUI::WindowManager::getDllSingleton ();

    window_ =
      window_mgr.loadLayoutFromFile (szLayout);
  }

  catch (const CEGUI::GenericException& e)
  {
    dll_log->Log (L"CEGUI Exception: %hs", e.getMessage ().c_str ());
  }
}

SK_PopupWindow::~SK_PopupWindow (void)
{
  if (! (config.cegui.enable && config.cegui.frames_drawn > 0)) return;


  if (window_ != nullptr)
  {
    try
    {
      CEGUI::WindowManager& window_mgr =
        CEGUI::WindowManager::getDllSingleton ();

      window_mgr.destroyWindow (window_);

      window_ = nullptr;
    }

    catch (const CEGUI::GenericException&)
    {
    }
  }
}

CEGUI::Window*
SK_PopupWindow::getChild (const char* szName)
{
  if (window_ == nullptr)
    return nullptr;

  try
  {
    return window_->getChild (szName);
  }

  catch (const CEGUI::GenericException&)
  {
  }

  return nullptr;
}

SK_PopupWindow::operator CEGUI::Window* (void)
{
  return window_;
}




CRITICAL_SECTION SK_PopupManager::cs;
SK_PopupManager* SK_PopupManager::__manager__ = nullptr;

SK_PopupManager*
SK_PopupManager::getInstance (void)
{
  if (__manager__ == nullptr)
  {
    __manager__ =
      new SK_PopupManager ();
  }

  return __manager__;
}

SK_PopupManager::SK_PopupManager (void)
{
  gui_ctx_ = nullptr;
}

bool
SK_PopupManager::isPopup (SK_PopupWindow* popup)
{
  if (popups_.count (popup))
    return true;

  return false;
}


SK_PopupWindow*
SK_PopupManager::createPopup (const char* szLayout)
{
  if (! (config.cegui.enable && config.cegui.frames_drawn > 0)) return nullptr;


  if (gui_ctx_ == nullptr)
  {
    gui_ctx_ =
      &CEGUI::System::getDllSingleton ().getDefaultGUIContext ();
  }

  try {
    auto* popup =
      new SK_PopupWindow (szLayout);

    if (popup->window_ != nullptr)
    {
      popups_.addKeyValue (popup, popup->window_);

      popup->window_->subscribeEvent (
        "DestructStart",//CEGUI::Window::EventDestructionStarted,
          CEGUI::Event::Subscriber ( &SK_PopupManager::OnDestroyPopup,
                                       this
                                   )
      );

      //gui_ctx_->setRootWindow (popup->window_);

      return popup;
    }

    delete popup;
  }

  catch (...)
  {
  }

  return nullptr;
}

void
SK_PopupManager::destroyPopup (SK_PopupWindow* popup)
{
  if (! (config.cegui.enable && config.cegui.frames_drawn > 0)) return;


  if (isPopup (popup))
  {
    CEGUI::WindowManager& window_mgr =
      CEGUI::WindowManager::getDllSingleton ();

    popups_.erase (popup->window_);

    //popup->window_->hide ();
    window_mgr.destroyWindow (popup->window_);

    popup->window_ = nullptr;

    delete popup;
  }

  //SK_TextOverlayFactory::getInstance ()->drawAllOverlays (0.0f, 0.0f, true);
}

void
SK_PopupManager::destroyAllPopups (void)
{
  if (gui_ctx_ != nullptr)
    CEGUI::System::getDllSingleton ().destroyGUIContext (*gui_ctx_);

  gui_ctx_ = nullptr;

  auto it = popups_.cbegin ();

  CEGUI::WindowManager& window_mgr =
    CEGUI::WindowManager::getDllSingleton ();

  while (it != popups_.cend ())
  {
    SK_PopupWindow* popup =
      (it)->first;

    window_mgr.destroyWindow (popup->window_);

    popup->window_ = nullptr;

    delete (it++)->first;
  }

  popups_.clear     ();
}


bool
SK_PopupManager::OnDestroyPopup (const CEGUI::EventArgs& e)
{
  auto& win_event =
    (CEGUI::WindowEventArgs &)e;

  popups_.erase (win_event.window);

  return true;
}
#endif

#include <control_panel.h>
#include <concurrent_queue.h>
#include <imgui/font_awesome.h>

extern iSK_INI* notify_ini;

concurrency::concurrent_queue <SK_ImGui_Toast> SK_ImGui_Notifications;

#define NOTIFY_PADDING_X           20.f // Bottom-left X padding
#define NOTIFY_PADDING_Y           20.f // Bottom-left Y padding
#define NOTIFY_PADDING_MESSAGE_Y   10.f // Padding Y between each message
#define NOTIFY_FADE_IN_OUT_TIME    420  // Fade in and out duration
#define NOTIFY_OPACITY             0.8f // 0-1 Toast opacity
#define NOTIFY_DEFAULT_IMGUI_FLAGS ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing

ImVec4 SK_ImGui_GetToastColor (SK_ImGui_Toast::Type type)
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

const char* SK_ImGui_GetToastIcon (SK_ImGui_Toast::Type type)
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

float SK_ImGui_GetToastFadePercent (SK_ImGui_Toast& toast)
{
  const DWORD dwElapsed =
    SK::ControlPanel::current_time - toast.displayed;

  if (toast.stage == SK_ImGui_Toast::FadeIn)
  {
  	return ( (float)dwElapsed /
             (float)NOTIFY_FADE_IN_OUT_TIME ) * NOTIFY_OPACITY;
  }

  else if (toast.stage == SK_ImGui_Toast::FadeOut)
  {
  	return (1.f - (((float)dwElapsed       - (float)NOTIFY_FADE_IN_OUT_TIME -
                    (float)toast.duration) / (float)NOTIFY_FADE_IN_OUT_TIME)) * NOTIFY_OPACITY;
  }

  return 1.f * NOTIFY_OPACITY;
}

bool
SK_ImGui_CreateNotification ( const char* szID,
                     SK_ImGui_Toast::Type type,
                              const char* szCaption,
                              const char* szTitle,
                                    DWORD dwMilliseconds,
                                    DWORD flags )
  
{
  if ( ( szID != nullptr &&
        *szID != '\0'  ) && ( flags & SK_ImGui_Toast::ShowOnce ) )
  {
    static concurrency::concurrent_unordered_set <std::string> shown_ids;

    if (shown_ids.find (szID) != shown_ids.end ())
    {
      return false;
    }

    shown_ids.insert (szID);
  }

  SK_ImGui_Toast toast;

  if (szID      != nullptr) toast.id      = szID;
  if (szCaption != nullptr) toast.caption = szCaption;
  if (szTitle   != nullptr) toast.title   = szTitle;

  if (! szCaption)
    flags &= ~SK_ImGui_Toast::ShowCaption;

  if (! szTitle)
    flags &= ~SK_ImGui_Toast::ShowTitle;

  if (! szID)
    flags &= ~( SK_ImGui_Toast::ShowOnce |
                SK_ImGui_Toast::ShowNewest );

  if (notify_ini != nullptr)
  {
    std::wstring wstr_id (
      SK_UTF8ToWideChar (toast.id.c_str ())
    );

    bool has_section =
      notify_ini->contains_section (wstr_id.c_str ());

    auto& toast_cfg =
      notify_ini->get_section (wstr_id.c_str ());

    toast.duration = toast_cfg.contains_key (L"DurationInMs") ?
              _wtoi (toast_cfg.get_value    (L"DurationInMs").c_str ()):
                                                               0;

    if (toast.duration == 0)
        toast_cfg.add_key_value (L"DurationInMs", std::to_wstring (dwMilliseconds)),
                                                  toast.duration = dwMilliseconds;

    toast.stage = toast_cfg.contains_key (L"DoNotShow") ?
       SK_IsTrue (toast_cfg.get_value    (L"DoNotShow").c_str ()) ?
                                         SK_ImGui_Toast::Finished : SK_ImGui_Toast::FadeIn
                                                                  : SK_ImGui_Toast::FadeIn;
    if (toast.stage == SK_ImGui_Toast::FadeIn)
        toast_cfg.add_key_value (L"DoNotShow", L"false");

    const bool bShowOnce =
      toast_cfg.contains_key           (L"ShowOnce") ?
        SK_IsTrue (toast_cfg.get_value (L"ShowOnce").c_str ()) ?
                                                          true : false
                                                               : false;

    if (bShowOnce) flags |=  SK_ImGui_Toast::ShowOnce;
    else           flags &= ~SK_ImGui_Toast::ShowOnce;

    if (! toast_cfg.contains_key  (L"ShowOnce"))
    {     toast_cfg.add_key_value (L"ShowOnce",
             (flags & SK_ImGui_Toast::ShowOnce) ? L"true"
                                                : L"false");
    }

#if 0
    toast.anchor = toast_cfg.contains_key (L"AnchorPoint") ?
        SK_IsTrue (toast_cfg.get_value    (L"AnchorPoint").c_str ()) ?
                                            SK_ImGui_Toast::Finished : SK_ImGui_Toast::FadeIn
                                                                     : SK_ImGui_Toast::FadeIn;
#endif

    const bool bDoNotShow =
      ( toast.stage == SK_ImGui_Toast::Finished );

    toast_cfg.get_value (L"DoNotShow") =
                          bDoNotShow ? L"true"
                                     : L"false";

    if (! has_section)
      SK_SaveConfig ();
  }

  toast.inserted = SK::ControlPanel::current_time;
  toast.type     = type;
  toast.flags    = (SK_ImGui_Toast::Flags)flags;

  SK_ImGui_Notifications.push (toast);

  return true;
}

void
SK_ImGui_DrawNotifications (void)
{
  auto pFont =
    ImGui::GetFont ();

  std::vector <SK_ImGui_Toast> notifications;

  while (! SK_ImGui_Notifications.empty ())
  {
    SK_ImGui_Toast                           toast;
    while (! SK_ImGui_Notifications.try_pop (toast))
      ;

    notifications.emplace_back (toast);
  }

  float fHeight = NOTIFY_PADDING_Y;

  const ImVec2 viewport_size =
    ImGui::GetMainViewport()->Size;

  int i = 0;


  std::unordered_map <std::string, DWORD> shown;

  for ( auto it  = notifications.rbegin () ;
             it != notifications.rend   () ;
             it++ )
  {
    if (it->flags & SK_ImGui_Toast::ShowNewest)
    {
      auto& time_shown =
        shown [it->id];

      if (time_shown >= it->inserted)
      {
        it->stage = SK_ImGui_Toast::Finished;
      }

      else
      {
        shown [it->id] = it->inserted;
      }
    }
  }


  auto orig_scale =
    pFont->Scale;

  pFont->Scale *= 3.0f;

  for ( auto& toast : notifications )
  {
    if (toast.stage == SK_ImGui_Toast::Finished)
      continue;

    bool remove = false;

    if (toast.flags & SK_ImGui_Toast::UseDuration)
    {
      if (toast.displayed == 0)
          toast.displayed = SK::ControlPanel::current_time;

      if (toast.displayed < SK::ControlPanel::current_time - (toast.duration + 2 * NOTIFY_FADE_IN_OUT_TIME))
      {
        if (toast.duration != INFINITE)
        {
          if (toast.stage != SK_ImGui_Toast::Config)
            remove = true;
        }
      }
    }

    if (toast.stage != SK_ImGui_Toast::Config)
    {
      DWORD dwElapsed = SK::ControlPanel::current_time - toast.displayed;

      if (dwElapsed > NOTIFY_FADE_IN_OUT_TIME + toast.duration)
        toast.stage = SK_ImGui_Toast::FadeOut;

      else
      {
        if (dwElapsed > NOTIFY_FADE_IN_OUT_TIME)
        {
          toast.stage = SK_ImGui_Toast::Drawing;
        }

        else
          toast.stage = SK_ImGui_Toast::FadeIn;
      }
    }

    ImGui::SetNextWindowBgAlpha (
      SK_ImGui_GetToastFadePercent (toast)
    );

    const ImVec2 viewport_pos =
      ImGui::GetMainViewport ()->Pos;

#if 1
    ImGui::SetNextWindowPos (
      ImVec2 (                                   NOTIFY_PADDING_X,
              viewport_pos.y + viewport_size.y - NOTIFY_PADDING_Y - fHeight),
        ImGuiCond_Always,
          ImVec2 (0.0f, 1.0f)
    );
#else
    ImGui::SetNextWindowPos (
      ImVec2 (viewport_pos.x + viewport_size.x - NOTIFY_PADDING_X,
              viewport_pos.y + viewport_size.y - NOTIFY_PADDING_Y - fHeight),
        ImGuiCond_Always,
          ImVec2 (1.0f, 1.0f)
    );
#endif

    char         window_id [64] = { };
    _snprintf_s (window_id, 63, "##TOAST%d", (int)i++);

    DWORD        window_flags = NOTIFY_DEFAULT_IMGUI_FLAGS;

    ImGui::Begin (window_id, nullptr, window_flags);
    {
      ImGui::BeginGroup ();
      ImGui::BringWindowToDisplayFront (ImGui::GetCurrentWindow ());
      ImGui::PushTextWrapPos           (viewport_size.x / 3.333f);

      if (toast.stage != SK_ImGui_Toast::Config)
      {
        ImGui::TextColored (
          SK_ImGui_GetToastColor (toast.type),
          SK_ImGui_GetToastIcon  (toast.type)
        );

        bool has_title = false;

        if ((! toast.title.empty ()) && (toast.flags & SK_ImGui_Toast::ShowTitle))
        {
          ImGui::SameLine ();

          ImGui::TextUnformatted (toast.title.c_str ());

          has_title = true;
        }

        if ((! toast.caption.empty ()) && (toast.flags & SK_ImGui_Toast::ShowCaption))
        {
          if (! has_title)
          {
            ImGui::SameLine ();
          }

          else
            ImGui::Separator ();

          ImGui::TextColored (
            ImVec4 (.85f, .85f, .85f, 0.925f), "%hs",
            toast.caption.c_str ()
          );
        }

        ImGui::PopTextWrapPos  ();
      }

      else
      {
        if (notify_ini != nullptr)
        {
          auto& toast_cfg =
            notify_ini->get_section (
              SK_UTF8ToWideChar (toast.id.c_str ()).c_str ()
            );

          bool                                                  bDoNotShow =
            SK_IsTrue (toast_cfg.get_value (L"DoNotShow").c_str ()) ? true
                                                                    : false;
          if (ImGui::Checkbox ("Never Show This Notification", &bDoNotShow))
          {
            toast_cfg.add_key_value (L"DoNotShow", bDoNotShow ? L"true"
                                                              : L"false");

            // Immediately save and remove the notification
            if (bDoNotShow)
            {
              SK_SaveConfig ();
              toast.stage = SK_ImGui_Toast::Finished;
            }
          }

          if (! bDoNotShow)
          {
            bool                                                          bShowOnce =
            SK_IsTrue (toast_cfg.get_value (L"ShowOnce").c_str ()) ? true
                                                                   : false;
            if (ImGui::Checkbox ("Show This Notification Once Per-Game", &bShowOnce))
            {
              toast_cfg.add_key_value (L"ShowOnce", bShowOnce ? L"true"
                                                              : L"false");
            }

            DWORD dwMilliseconds =
              _wtoi (toast_cfg.get_value (L"DurationInMs").c_str ());

            float fSeconds = static_cast <float> (dwMilliseconds) / 1000.0f;

            if (ImGui::SliderFloat ("Duration (seconds)", &fSeconds, 1.f, 60.f))
            {
              dwMilliseconds =
                static_cast <DWORD> (fSeconds * 1000.0f);

              toast_cfg.add_key_value (L"DurationInMs", std::to_wstring (dwMilliseconds));
            }
          }

          ImGui::Separator ();
        }

        if (ImGui::Button ("Save Settings"))
        {
          SK_SaveConfig ();

          toast.stage = SK_ImGui_Toast::Drawing;
        }
      }

      fHeight +=
        ( ImGui::GetWindowHeight () + NOTIFY_PADDING_MESSAGE_Y );

      ImGui::EndGroup ();

      if (ImGui::IsItemClicked (ImGuiMouseButton_Right))
      {
        toast.stage = SK_ImGui_Toast::Config;
      }

      if (toast.stage != SK_ImGui_Toast::Config && ImGui::IsItemHovered ())
      {
        ImGui::SetTooltip ("Right-click to configure this notification");
      }
    }
    ImGui::End   ();

    if (! (remove || toast.stage == SK_ImGui_Toast::Finished))
      SK_ImGui_Notifications.push (toast);
  }

  pFont->Scale = orig_scale;
}