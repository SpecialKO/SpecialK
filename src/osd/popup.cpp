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

concurrency::concurrent_queue <SK_ImGui_Toast> SK_ImGui_Notifications;

#define NOTIFY_MAX_MSG_LENGTH      4096
#define NOTIFY_PADDING_X           20.f  // Bottom-left X padding
#define NOTIFY_PADDING_Y           20.f  // Bottom-left Y padding
#define NOTIFY_PADDING_MESSAGE_Y   10.f  // Padding Y between each message
#define NOTIFY_FADE_IN_OUT_TIME    150   // Fade in and out duration
#define NOTIFY_OPACITY             0.8f  // 0-1 Toast opacity
#define NOTIFY_USE_SEPARATOR       false // If true, a separator will be rendered between the title and the content
#define NOTIFY_USE_DISMISS_BUTTON  true  // If true, a dismiss button will be rendered in the top right corner of the toast
#define NOTIFY_DEFAULT_IMGUI_FLAGS ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing

bool
SK_ImGui_CreateNotification ( const char* szID,
                     SK_ImGui_Toast::Type type,
                              const char* szCaption,
                              const char* szTitle,
                                    DWORD dwMilliseconds,
                                    DWORD flags )
  
{
  if (flags & SK_ImGui_Toast::ShowOnce)
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

  toast.duration = dwMilliseconds;
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
        it->finished = true;
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
    if (toast.finished)
      continue;

    bool remove = false;

    if (toast.flags & SK_ImGui_Toast::UseDuration)
    {
      if (toast.displayed == 0)
          toast.displayed = SK::ControlPanel::current_time;

      if (toast.displayed < SK::ControlPanel::current_time - toast.duration)
        remove = true;
    }

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
      ImGui::BringWindowToDisplayFront (ImGui::GetCurrentWindow ());
      ImGui::PushTextWrapPos           (viewport_size.x / 4.0f);

      ImGui::TextColored (
        SK_ImGui_GetToastColor (toast.type),
        SK_ImGui_GetToastIcon  (toast.type)
      );

      bool has_title = false;

      if ((! toast.title.empty ()) && toast.flags & SK_ImGui_Toast::ShowTitle)
      {
        ImGui::SameLine ();

        ImGui::TextUnformatted (toast.title.c_str ());

        has_title = true;
      }

      if ((! toast.caption.empty ()) && toast.flags & SK_ImGui_Toast::ShowCaption)
      {
        if (! has_title)
        {
          ImGui::SameLine ();
        }

        ImGui::TextUnformatted (toast.caption.c_str ());
      }

      ImGui::PopTextWrapPos  ();

      fHeight +=
        ( ImGui::GetWindowHeight () + NOTIFY_PADDING_MESSAGE_Y );
    }
    ImGui::End   ();

    if (! (remove || toast.finished))
      SK_ImGui_Notifications.push (toast);
  }

  pFont->Scale = orig_scale;
}