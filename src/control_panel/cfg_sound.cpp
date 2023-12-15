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


#include <SpecialK/control_panel/sound.h>
#include <imgui/font_awesome.h>

class SK_MMDev_AudioEndpointVolumeCallback;
SK_LazyGlobal <SK_MMDev_AudioEndpointVolumeCallback> volume_mgr;

SK_WASAPI_SessionManager&
SK_WASAPI_GetSessionManager (void)
{
  (void)volume_mgr.getPtr ();

  static SK_WASAPI_SessionManager sessions;
  return                          sessions;
}

SK_WASAPI_AudioSession*&
SK_WASAPI_GetAudioSession (void)
{
  (void)volume_mgr.getPtr ();

  static SK_WASAPI_AudioSession* audio_session;
  return                         audio_session;
}

void
SK_WASAPI_ResetSessionManager (void)
{
  SK_WASAPI_GetSessionManager ().signalReset ();
}

#define sessions      SK_WASAPI_GetSessionManager ()
#define audio_session SK_WASAPI_GetAudioSession   ()

using namespace SK::ControlPanel;

bool
SK_ImGui_SelectAudioSessionDlg (void)
{
  const ImGuiIO&
    io (ImGui::GetIO ());

  bool changed = false;

  static const float fMinX = 400.0f;
  static const float fMinY = 150.0f;

  float max_width = 0.0f;

  int sel_idx = -1;
  int count   =  0;

  SK_WASAPI_AudioSession **pSessions = nullptr;

  if (ImGui::IsPopupOpen ("Audio Session Selector"))
  {
    max_width =
      ImGui::CalcTextSize ("Audio Session Selector").x;

    pSessions =
      sessions.getActive (&count);

    std::set <DWORD> unique_processes;

    for ( auto i = 0 ; i < count ; ++i )
    {
      if ( unique_processes.emplace (
             pSessions [i]->getProcessId ()
           ).second )
      {
        const ImVec2 size =
          ImGui::CalcTextSize (pSessions [i]->getName ());

        if (size.x > max_width) max_width = size.x;
      }
    }

    ImGui::SetNextWindowSizeConstraints ( ImVec2 (
                                        std::max (fMinX * io.FontGlobalScale, max_width * 2.1f * io.FontGlobalScale +
                                                       ImGui::GetStyle ().ItemSpacing.x * io.FontGlobalScale * 5.0f),
                                        std::max (fMinY * io.FontGlobalScale, (unique_processes.size () + 3.0f) * 
                 ImGui::GetTextLineHeightWithSpacing () * io.FontGlobalScale +
                                                       ImGui::GetStyle ().ItemSpacing.y * io.FontGlobalScale * 13.0f)),
                                          ImVec2 (std::max (io.DisplaySize.x * 0.75f, fMinX),
                                                  std::max (io.DisplaySize.y * 0.666f,fMinY)) );
  }

  if (ImGui::BeginPopupModal ("Audio Session Selector", nullptr, ImGuiWindowFlags_AlwaysAutoResize |
                                                                 ImGuiWindowFlags_NoScrollbar      | ImGuiWindowFlags_NoScrollWithMouse))
  {
    ImGui::PushItemWidth (max_width * io.FontGlobalScale);

    ImGui::BeginChild ("SessionSelectHeader",   ImVec2 (0, 0), true,  ImGuiWindowFlags_NavFlattened | ImGuiWindowFlags_NoInputs |
                                                                      ImGuiWindowFlags_NoNavInputs);
    ImGui::BeginGroup ( );
    ImGui::Columns    (2);
    ImGui::Text       ("Task");
    ImGui::NextColumn ();
    ImGui::Text       ("Volume / Mute");
    ImGui::NextColumn ();
    ImGui::Columns    (1);

    ImGui::Separator  ();

    //if (ImGui::ListBoxHeader ("##empty", count, std::min (count + 3, 10)))
    ImGui::BeginGroup ();
    {
      ImGui::PushStyleColor (ImGuiCol_ChildBg,   ImVec4 (0.f, 0.f, 0.f, 0.f));
      ImGui::BeginChild     ("SessionSelectData",ImVec2 (0, 0), true,  ImGuiWindowFlags_NavFlattened);

      ImGui::BeginGroup ();//"SessionSelectData");
      ImGui::Columns    (2);

      std::set <DWORD> sessions_listed;

      for (int i = 0; i < count; i++)
      {
        SK_WASAPI_AudioSession* pSession =
          pSessions [i];

        auto pChannelVolume =
          pSession->getChannelAudioVolume ();

        // No duplicates please
        if (pChannelVolume == nullptr || (pSession->isActive () && (! sessions_listed.emplace (pSession->getProcessId ()).second)))
          continue;

        //bool selected     = false;
        const bool drawing_self =
          (pSession->getProcessId () == GetCurrentProcessId ());

        SK_ComPtr <ISimpleAudioVolume> volume_ctl =
          pSession->getSimpleAudioVolume ();

        BOOL mute = FALSE;

        if (volume_ctl != nullptr && SUCCEEDED (volume_ctl->GetMute (&mute)))
        {
          if (drawing_self)
          {
            ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
            ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
            ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));
          }
          if (ImGui::Selectable (pSession->getName (), drawing_self || pSession == audio_session))
            sel_idx = i;

          if (drawing_self)
            ImGui::PopStyleColor (3);

          ImGui::NextColumn ();

          float                         volume = 0.0f;
          volume_ctl->GetMasterVolume (&volume);

          char      szLabel [32] = { };
          snprintf (szLabel, 31, "###VolumeSlider%i", i);

          ImGui::PushStyleColor (ImGuiCol_Text,           mute ? ImVec4 (0.5f, 0.5f, 0.5f, 1.f) : ImVec4 (1.0f, 1.0f, 1.0f, 1.f));
          ImGui::PushStyleColor (ImGuiCol_FrameBg,        (ImVec4&&)ImColor::HSV ( 0.4f * volume, 0.6f, mute ? 0.2f : 0.4f));
          ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, (ImVec4&&)ImColor::HSV ( 0.4f * volume, 0.7f, mute ? 0.2f : 0.4f));
          ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  (ImVec4&&)ImColor::HSV ( 0.4f * volume, 0.8f, mute ? 0.2f : 0.4f));
          ImGui::PushStyleColor (ImGuiCol_SliderGrab,     (ImVec4&&)ImColor::HSV ( 0.4f * volume, 0.9f,        0.6f * (mute ? 0.5f : 1.0f)));

          volume *= 100.0f;

          ImGui::PushItemWidth (ImGui::GetContentRegionAvail ().x - 37.0f);
          {
            if (ImGui::SliderFloat (szLabel, &volume, 0.0f, 100.0f, "Volume: %03.1f%%"))
              volume_ctl->SetMasterVolume (volume / 100.0f, nullptr);
          }
          ImGui::PopItemWidth  ( );
          ImGui::PopStyleColor (5);

          ImGui::SameLine      ( );

          snprintf (szLabel, 31, "###VoumeCheckbox%i", i);

          ImGui::PushItemWidth (35.0f);
          {
            if (ImGui::Checkbox (szLabel, (bool *)&mute))
              volume_ctl->SetMute (mute, nullptr);
          }
          ImGui::PopItemWidth  (     );
          ImGui::NextColumn    (     );
        }
      }

      ImGui::Columns  (1);
      ImGui::EndGroup ( );
      ImGui::EndChild ( );

      ImGui::PopStyleColor ();

      ImGui::EndGroup ( );
      ImGui::EndGroup ( );
      ImGui::EndChild ( );
      //ImGui::ListBoxFooter ();

      if (sel_idx != -1)
      {
        audio_session = pSessions [sel_idx];

        changed  = true;
      }

      if (changed)
      {
        ImGui::CloseCurrentPopup ();
      }
    }

    // If there's nothing to select, then bail-out
    if (count == 0)
      ImGui::CloseCurrentPopup ();

    const ImRect window_rect
      ( ImGui::GetWindowPos (),
        ImGui::GetWindowPos () + ImGui::GetWindowSize () );

    const bool bEscape   = ImGui::IsKeyDown           ( ImGuiKey_Escape );
    const bool bClicked  = ImGui::IsAnyMouseDown      (                 ) && !ImGui::IsMouseDragging (ImGuiMouseButton_Left);
    const bool bHovering = ImGui::IsMouseHoveringRect ( window_rect.Min,
                                                        window_rect.Max );

    // Close the popup if user clicks outside of it, this behavior is
    //   more intuitive than ImGui's normal modal popup behavior.
    if (bEscape || (bClicked && (! bHovering)))
      ImGui::CloseCurrentPopup ();

    ImGui::PopItemWidth ();
    ImGui::EndPopup     ();
  }

  return changed;
}

bool
SK_ImGui_SelectAudioDeviceDlg (void)
{
  const ImGuiIO&
    io (ImGui::GetIO ());

  bool changed = false;

  static const float fMinX = 400.0f;
  static const float fMinY = 100.0f;

  size_t count =
    SK_WASAPI_EndPointMgr->getNumRenderEndpoints ();

  if (ImGui::IsPopupOpen ("Audio Device Selector"))
  {
    ImGui::SetNextWindowSizeConstraints ( ImVec2 (fMinX * io.FontGlobalScale,
                                        std::max (fMinY * io.FontGlobalScale,
         count * ImGui::GetTextLineHeightWithSpacing () * io.FontGlobalScale)),
                                          ImVec2 (std::max (io.DisplaySize.x * 0.75f, fMinX),
                                                  std::max (io.DisplaySize.y * 0.666f,fMinY)) );
  }

  if (ImGui::BeginPopupModal ("Audio Device Selector", nullptr, ImGuiWindowFlags_AlwaysAutoResize |
                                                                ImGuiWindowFlags_NoScrollbar      | ImGuiWindowFlags_NoScrollWithMouse))
  {
    std::wstring endpoint_id =
      SK_WASAPI_EndPointMgr->getPersistedDefaultAudioEndpoint (
        audio_session->getProcessId (), eRender
      );

    for ( UINT i = 0 ; i < count ; ++i )
    {
      auto& device =
        SK_WASAPI_EndPointMgr->getRenderEndpoint (i);

      if (device.state_ == DEVICE_STATE_ACTIVE)
      {
        bool selected =
          device.isSameDevice (endpoint_id.c_str ());

        if (ImGui::Selectable (device.name_.c_str (), selected))
        {
          SK_WASAPI_EndPointMgr->setPersistedDefaultAudioEndpoint (
            audio_session->getProcessId (), eRender, device.id_
          );

          sessions.signalReset ();

          ImGui::CloseCurrentPopup ();

          changed = true;

          break;
        }
      }
    }

    // If there's nothing to select, then bail-out
    if (count == 0)
      ImGui::CloseCurrentPopup ();

    const ImRect window_rect
      ( ImGui::GetWindowPos (),
        ImGui::GetWindowPos () + ImGui::GetWindowSize () );

    const bool bEscape   = ImGui::IsKeyDown           ( ImGuiKey_Escape );
    const bool bClicked  = ImGui::IsAnyMouseDown      (                 ) && !ImGui::IsMouseDragging (ImGuiMouseButton_Left);
    const bool bHovering = ImGui::IsMouseHoveringRect ( window_rect.Min,
                                                        window_rect.Max );

    // Close the popup if user clicks outside of it, this behavior is
    //   more intuitive than ImGui's normal modal popup behavior.
    if (bEscape || (bClicked && (! bHovering)))
      ImGui::CloseCurrentPopup ();

    ImGui::EndPopup ();
  }

  return changed;
}

class SK_MMDev_AudioEndpointVolumeCallback :
  public IAudioEndpointVolumeCallback
{
  volatile ULONG _cRef = 0UL;

public:
  SK_MMDev_AudioEndpointVolumeCallback (void) noexcept
  {
    InterlockedExchange (&_cRef, 1);
  }

  virtual ~SK_MMDev_AudioEndpointVolumeCallback (void) = default;

  ULONG STDMETHODCALLTYPE AddRef (void) noexcept override
  {
    return
      InterlockedIncrement (&_cRef);
  }

  ULONG STDMETHODCALLTYPE Release (void) noexcept override
  {
    const ULONG ulRef =
      InterlockedDecrement (&_cRef);

    if (0 == ulRef)
    {
      delete this;
    }

    return ulRef;
  }

  HRESULT STDMETHODCALLTYPE QueryInterface (REFIID   riid,
                                              VOID** ppvInterface) override
  {
    if (ppvInterface == nullptr)
      return E_POINTER;

    if (IID_IUnknown == riid)
    {
      AddRef ();

      *ppvInterface = this;
    }

    else if (__uuidof (IAudioEndpointVolumeCallback) == riid)
    {
      AddRef ();

      *ppvInterface = this;
    }

    else
    {
      *ppvInterface = nullptr;

      return
        E_NOINTERFACE;
    }

    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE OnNotify (PAUDIO_VOLUME_NOTIFICATION_DATA pNotify) noexcept override
  {
    if (pNotify == nullptr)
      return E_INVALIDARG;

    return S_OK;
  }
};

void
SK_ImGui_VolumeManager (void)
{
  const ImGuiIO&
    io (ImGui::GetIO ());

  const  float font_size           =             ImGui::GetFont  ()->FontSize                        * io.FontGlobalScale;
#ifdef _ProperSpacing
  const  float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;
#endif

  std::string app_name;

  sessions.Activate ();

  SK_ComPtr <IAudioMeterInformation> pMeterInfo =
    audio_session != nullptr ? audio_session->getMeterInfo ()
                             :       sessions.getMeterInfo ();

  if (pMeterInfo == nullptr)
    audio_session = nullptr;

  if (audio_session != nullptr)
  {
    const DWORD dwSessionPid =
      audio_session->getProcessId ();

    // Select a different session from the same process if the selected session is
    //   not currently active...
    if (! audio_session->isActive ())
    {
      int session_count = 0;

      auto** ppSessions =
        sessions.getActive (&session_count);

      for (int i = 0; i < session_count ; ++i)
      {
        if (ppSessions [i] == nullptr)
          continue;

        if ( ppSessions [i]->getProcessId () != dwSessionPid ||
             ppSessions [i]->isActive     () == false )
          continue;

        audio_session = ppSessions [i];
        break;
      }
    }

    float fLevelDB = 0.0f;

    auto pEndpointVolume =
      audio_session->getEndpointVolume ();

    if ( pEndpointVolume == nullptr || FAILED (
           pEndpointVolume->GetMasterVolumeLevel (&fLevelDB)) )
    {
      sessions.signalReset ();
      return;
    }
    app_name = audio_session->getName ();
  }

  app_name += "##AudioSessionAppName";

  ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.90f, 0.68f, 0.45f));
  ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.90f, 0.72f, 0.80f));
  ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.87f, 0.78f, 0.80f));

  ImGui::Columns (2, "Amazon Sep", false);

  bool selected = true;
  if (ImGui::Selectable (app_name.c_str (), &selected))
    ImGui::OpenPopup ("Audio Session Selector");

  if (ImGui::IsItemHovered ())
    ImGui::SetTooltip ("Click Here to Manage Another Application");

  ImGui::SetColumnOffset (1, 530 * io.FontGlobalScale);
  ImGui::NextColumn      (                           );

  // Send both keydown and keyup events because software like Amazon Music only responds to keyup
  ImGui::BeginGroup ();
  {
    ImGui::PushItemWidth (-1);
    if (ImGui::Button ("  " ICON_FA_BACKWARD "  "))
    {
      ISteamMusic* pMusic =
        SK_SteamAPI_Music ();

      if (pMusic && pMusic->BIsEnabled ())
      {
        if (pMusic->BIsPlaying ()) pMusic->PlayPrevious ();
      }

      BYTE bScancode =
       (BYTE)MapVirtualKey (VK_MEDIA_PREV_TRACK, 0);

      DWORD dwFlags =
        ( bScancode & 0xE0 ) == 0   ?
          static_cast <DWORD> (0x0) :
          static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

      keybd_event_Original (VK_MEDIA_PREV_TRACK, bScancode, dwFlags,                   0);
      keybd_event_Original (VK_MEDIA_PREV_TRACK, bScancode, dwFlags | KEYEVENTF_KEYUP, 0);
    }

    ImGui::SameLine ();

    if (ImGui::Button ("  " ICON_FA_PLAY ICON_FA_PAUSE "  "))
    {
      ISteamMusic* pMusic =
        SK_SteamAPI_Music ();

      if (pMusic && pMusic->BIsEnabled ())
      {
        if (pMusic->BIsPlaying ()) pMusic->Pause ();
        else                       pMusic->Play  ();
      }

      BYTE bScancode =
       (BYTE)MapVirtualKey (VK_MEDIA_PLAY_PAUSE, 0);

      DWORD dwFlags =
        ( bScancode & 0xE0 ) == 0   ?
          static_cast <DWORD> (0x0) :
          static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

      keybd_event_Original (VK_MEDIA_PLAY_PAUSE, bScancode, dwFlags,                   0);
      keybd_event_Original (VK_MEDIA_PLAY_PAUSE, bScancode, dwFlags | KEYEVENTF_KEYUP, 0);
    }

    ImGui::SameLine ();

    if (ImGui::Button ("  " ICON_FA_FORWARD "  "))
    {
      ISteamMusic* pMusic =
        SK_SteamAPI_Music ();

      if (pMusic && pMusic->BIsEnabled ())
      {
        if (pMusic->BIsPlaying ()) pMusic->PlayNext ();
      }

      BYTE bScancode =
       (BYTE)MapVirtualKey (VK_MEDIA_NEXT_TRACK, 0);

      DWORD dwFlags =
        ( bScancode & 0xE0 ) == 0   ?
          static_cast <DWORD> (0x0) :
          static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

      keybd_event_Original (VK_MEDIA_NEXT_TRACK, bScancode, dwFlags,                   0);
      keybd_event_Original (VK_MEDIA_NEXT_TRACK, bScancode, dwFlags | KEYEVENTF_KEYUP, 0);
    }
    ImGui::PopItemWidth ();
  }
  ImGui::EndGroup   ();
  ImGui::SameLine   ();
  static std::set <SK_ConfigSerializedKeybind *>
    keybinds = {
      &config.sound.game_mute_keybind,
      &config.sound.game_volume_up_keybind,
      &config.sound.game_volume_down_keybind
    };

  for ( auto& binding : keybinds )
  {
    if (binding->assigning)
    {
      if (! ImGui::IsPopupOpen (binding->bind_name))
        ImGui::OpenPopup (      binding->bind_name);

      std::wstring     original_binding =
                                binding->human_readable;

      SK_ImGui_KeybindDialog (  binding           );

      if (             original_binding !=
                                binding->human_readable)
        binding->param->store ( binding->human_readable);

      if (! ImGui::IsPopupOpen (binding->bind_name))
                                binding->assigning = false;
    }
  }

  if (ImGui::BeginMenu ("Keybinds###VolumeKeyMenu"))
  {
    const auto Keybinding =
    [] (SK_ConfigSerializedKeybind *binding) ->
    auto
    {
      if (binding == nullptr)
        return false;

      std::string label =
        SK_WideCharToUTF8      (binding->human_readable);

      ImGui::PushID            (binding->bind_name);

      binding->assigning =
        SK_ImGui_KeybindSelect (binding, label.c_str ());

      ImGui::PopID             ();

      return true;
    };

    ImGui::BeginGroup ();
    for ( auto& keybind : keybinds )
    {
      ImGui::Text ( "%s:  ",
                      keybind->bind_name );
    }
    ImGui::EndGroup   ();
    ImGui::SameLine   ();
    ImGui::BeginGroup ();
    for ( auto& keybind : keybinds )
    {
      Keybinding  (   keybind );
    }
    ImGui::EndGroup   ();
    ImGui::EndMenu    ();
  }
  ImGui::Columns    (1);

  ImGui::PopStyleColor (3);


  bool session_changed =
    SK_ImGui_SelectAudioSessionDlg ();

  ImGui::TreePush ("");

  // TODO : Use notification
  static DWORD dwLastTest = 0;
  static UINT  channels   = 0;

  if (audio_session == nullptr)
  {
    int count;

    SK_WASAPI_AudioSession** ppSessions =
      sessions.getActive (&count);

    // Find the session for the current game and select that first...
    for (int i = 0; i < count; i++)
    {
      if (ppSessions [i]->getProcessId () == GetCurrentProcessId () &&
          ppSessions [i]->isActive ())
      {
        dwLastTest    = 0;
        channels      = 0;
        audio_session = ppSessions [i];
        break;
      }
    }
  }

  if (audio_session != nullptr)
  {
    if (pMeterInfo == nullptr)
        pMeterInfo = audio_session->getMeterInfo ();

    if (pMeterInfo == nullptr)
        pMeterInfo = sessions.getMeterInfo ();

    if ( ( (dwLastTest + 45000)  <
              SK::ControlPanel::current_time  ) )
    {
      if ( FAILED  (
             pMeterInfo->GetMeteringChannelCount (
                                   &channels     )
                   )
         ) channels = 0;

      if (channels != 0)
      {
        dwLastTest =
          SK::ControlPanel::current_time;
      }
    }

    if (channels != 0)
    {
      SK_ComPtr <IChannelAudioVolume> pChannelVolume =
        audio_session->getChannelAudioVolume      ( );

      static float
        channel_peaks_ [32] = { };

      struct
      {
        struct {
          float inst_min = FLT_MAX;  DWORD dwMinSample = 0;
          float disp_min = FLT_MAX;

          float inst_max = FLT_MIN;  DWORD dwMaxSample = 0;
          float disp_max = FLT_MIN;
        } vu_peaks;

        float peaks       [120] = {   };
        int   current_idx       =   0L ;
        DWORD update_time       =  0UL ;
        char  hashed_name [ 32] = {   };
      } static history    [ 32] = {   };

      #define VUMETER_TIME 333

      static float master_vol  = -1.0f; // Master Volume
      static BOOL  master_mute =  FALSE;

      struct volume_s
      {
        float volume           =  1.0f; // Current Volume (0.0 when muted)
        float normalized       =  1.0f; // Unmuted Volume (stores volume before muting)

        bool  muted            = false;

        // Will fill-in with unique names for ImGui
        //   (all buttons say the same thing =P)
        //
        char mute_button  [14] = { };
        char slider_label [8 ] = { };
      };

      static
        std::map <int, volume_s>
          channel_volumes;

      SK_ComPtr <ISimpleAudioVolume>  pVolume  =
        audio_session->getSimpleAudioVolume ( );

      pVolume->GetMasterVolume (&master_vol);
      pVolume->GetMute         (&master_mute);

      static std::string label = "Switch Audio Device";

      SK_ImGui_SelectAudioDeviceDlg ();

      if (SK_WASAPI_EndPointMgr->getNumRenderEndpoints (DEVICE_STATE_ACTIVE) > 1)
      {
        if (ImGui::Button (label.c_str ()))
        {
          ImGui::OpenPopup ("Audio Device Selector");
        }

        ImGui::SameLine ();
      }

      IAudioEndpointVolume* pEndVol =
        audio_session->getEndpointVolume ();

      if (pEndVol != nullptr)
      {
        float min_dB  = 0.0f,
              max_dB  = 0.0f,
              incr_dB = 0.0f;
        float cur_dB  = 0.0f;

        pEndVol->GetVolumeRange       (&min_dB, &max_dB, &incr_dB);
        pEndVol->GetMasterVolumeLevel (&cur_dB);

        if (min_dB != max_dB)
        {
          if ( ImGui::InputFloat ( "System-Wide Volume (dB)", &cur_dB, incr_dB, incr_dB * 5 ) )
          {
            pEndVol->SetMasterVolumeLevel (std::max (min_dB, std::min (max_dB, cur_dB)), nullptr);
          }

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip    ();
            ImGui::Text            ("Your device supports %06.03f dB - %06.03f dB "
                                    "in %06.03f dB steps", min_dB, max_dB, incr_dB);
            ImGui::Separator       ();
            ImGui::BulletText      ("-dB represents attenuation from reference volume");
            ImGui::BulletText      ("+dB represents gain (amplification)%s", max_dB > 0.0f ?
                                             " " : " - your hardware does not support gain." );
            ImGui::Separator       ();
            ImGui::TextUnformatted ("Hold Ctrl for faster +/- adjustment.");
            ImGui::EndTooltip      ();
          }

          ImGui::SameLine ();
        }
      }

      ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
      ImGui::SameLine    ();
      ImGui::BeginGroup  ();

      static auto cur_lat = SK_WASAPI_GetCurrentLatency ();
      static auto min_lat = SK_WASAPI_GetMinimumLatency ();

      static DWORD last_update0 = SK::ControlPanel::current_time;
      static DWORD last_update1 = SK::ControlPanel::current_time + 3333UL;

      if (last_update0 < SK::ControlPanel::current_time - 3333UL)
      {   last_update0 = SK::ControlPanel::current_time;
        cur_lat = SK_WASAPI_GetCurrentLatency ();
      }

      if (last_update1 < SK::ControlPanel::current_time - 6666UL)
      {   last_update1 = SK::ControlPanel::current_time;
        min_lat = SK_WASAPI_GetMinimumLatency ();
      }

      if (min_lat.frames > 0 && cur_lat.frames > 0)
      {
        if (min_lat.frames != cur_lat.frames)
        {
          ImGui::SameLine ();
          if (ImGui::Button ("Minimize Latency"))
          {
            auto latency =
              SK_WASAPI_SetLatency (min_lat);

            if (latency.frames != 0 && latency.milliseconds != 0.0f)
            {
              SK_ImGui_WarningWithTitle (
                SK_FormatStringW (
                  L"Latency changed from %.1f ms to %.1f ms",
                    cur_lat.milliseconds, latency.milliseconds).c_str (),
                  L"Audio Latency Changed"
              );

              config.sound.minimize_latency = true;
            }
          }

          if (ImGui::IsItemHovered ())
          {
            auto default_latency =
              SK_WASAPI_GetDefaultLatency ();

            ImGui::BeginTooltip ();
            ImGui::BulletText   ("Current Latency: %.1f ms", cur_lat        .milliseconds);
            ImGui::BulletText   ("Minimum Latency: %.1f ms", min_lat        .milliseconds);
            ImGui::BulletText   ("Default Latency: %.1f ms", default_latency.milliseconds);
            ImGui::Separator    ();
            ImGui::Text         ("SK will remember to always minimize latency in this game.");
            ImGui::EndTooltip   ();
          }
        }

        else
          ImGui::Text ( "Latency:\t%.1f ms @ %d kHz",
                          cur_lat.milliseconds,
                          cur_lat.samples_per_sec / 1000UL );
      }

      float fPosY =
        ImGui::GetCursorPosY ();

      ImGui::Separator ();
      ImGui::EndGroup  ();

      ImGui::SetCursorPosY (fPosY);
      ImGui::Spacing ();

      const char* szMuteButtonTitle =
        ( master_mute ? "  Unmute  ###MasterMute" :
                        "   Mute   ###MasterMute" );

      if (ImGui::Button (szMuteButtonTitle))
      {
        master_mute ^= TRUE;

        pVolume->SetMute (master_mute, nullptr);
      }

      ImGui::SameLine ();

      float val = master_mute ? 0.0f : 1.0f;

      ImGui::PushStyleColor (ImGuiCol_FrameBg,        ImVec4 ( 0.3f,  0.3f,  0.3f,  val));
      ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, ImVec4 ( 0.6f,  0.6f,  0.6f,  val));
      ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  ImVec4 ( 0.9f,  0.9f,  0.9f,  val));
      ImGui::PushStyleColor (ImGuiCol_SliderGrab,     ImVec4 ( 1.0f,  1.0f,  1.0f, 1.0f));
      ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV ( 0.15f, 0.0f,
                                                                       0.5f + master_vol * 0.5f) );

      if (ImGui::SliderFloat ("     Session Master Volume   ", &master_vol, 0.0, 1.0, ""))
      {
        if (master_mute)
        {
                            master_mute = FALSE;
          pVolume->SetMute (master_mute, nullptr);
        }

        pVolume->SetMasterVolume (master_vol, nullptr);
      }

      ImGui::SameLine ();

      ImGui::TextColored ( ImColor::HSV ( 0.15f, 0.9f,
                                            0.5f + master_vol * 0.5f),
                             "(%03.1f%%)  ",
                               master_vol * 100.0 );

      ImGui::PopStyleColor (5);
      ImGui::Separator     ( );
      ImGui::Columns       (2);

      for (UINT i = 0 ; i < channels; i++)
      {
        // Throttle meter updates at extremely high frame rates
        //
        //   XXX: TODO: Make this a constant rate instead of sampling once-per frame
        //
        if ( history [i].update_time >= SK::ControlPanel::current_time - 2 ||
             SUCCEEDED (pMeterInfo->GetChannelsPeakValues (channels, channel_peaks_)) )
        {
          if (history [i].update_time < SK::ControlPanel::current_time - 2)
          {
            history [i].vu_peaks.inst_min = std::min (history [i].vu_peaks.inst_min, channel_peaks_ [i]);
            history [i].vu_peaks.inst_max = std::max (history [i].vu_peaks.inst_max, channel_peaks_ [i]);

            history [i].vu_peaks.disp_min = history [i].vu_peaks.inst_min;

            if (history [i].vu_peaks.dwMinSample < current_time - VUMETER_TIME * 3)
            {
              history [i].vu_peaks.inst_min    = channel_peaks_ [i];
              history [i].vu_peaks.dwMinSample = current_time;
            }

            history [i].vu_peaks.disp_max      = history [i].vu_peaks.inst_max;

            if (history [i].vu_peaks.dwMaxSample < current_time - VUMETER_TIME * 3)
            {
              history [i].vu_peaks.inst_max    = channel_peaks_ [i];
              history [i].vu_peaks.dwMaxSample = current_time;
            }

            history [i].peaks [history [i].current_idx] =
                        channel_peaks_ [i];

            // Alternate the direction of the graph for channels depending on left/right
            //   location
            if (i & 0x1) // Odd-channels go right->left
            {
              history [i].current_idx = (history [i].current_idx - 1);

              if (history [i].current_idx < 0)
                  history [i].current_idx = IM_ARRAYSIZE (history [i].peaks) - 1;
            }

            else
            {
              history [i].current_idx = (history [i].current_idx + 1) % IM_ARRAYSIZE (history [i].peaks);
            }

            history [i].update_time = SK::ControlPanel::current_time;
          }

          ImGui::BeginGroup ();

          const float ht = font_size * 6;

          if (channel_volumes.count (static_cast <int> (i)) == 0)
          {
            session_changed = true;

            snprintf (channel_volumes [i].mute_button, 13, "  Mute  ##%u", i);
            snprintf (channel_volumes [i].slider_label, 7, "##vol%u",      i);
          }

          if (             pChannelVolume != nullptr   &&
               SUCCEEDED ( pChannelVolume->GetChannelVolume (
                  static_cast <UINT32> (i),  &channel_volumes [i].volume )
                         )
             )
          {
            volume_s& ch_vol =
              channel_volumes [i];

            if (session_changed)
            {
              channel_volumes [i].muted      = (channel_volumes [i].volume <= 0.001f);
              channel_volumes [i].normalized = (channel_volumes [i].volume  > 0.001f ?
                                                channel_volumes [i].volume : 1.0f);
            }

                  bool  changed     = false;
            const float volume_orig = ch_vol.normalized;

            ImGui::BeginGroup  ();
            ImGui::TextColored (ImVec4 (0.80f, 0.90f, 1.0f,  1.0f), "%-22s", SK_WASAPI_GetChannelName (i));
                                                                                      ImGui::SameLine ( );
            ImGui::TextColored (ImVec4 (0.7f,  0.7f,  0.7f,  1.0f), "      Volume:"); ImGui::SameLine ( );
            ImGui::TextColored (ImColor::HSV (
                                        0.25f, 0.9f,
                                          0.5f + ch_vol.volume * 0.5f),
                                                                    "%03.1f%%   ", 100.0 * ch_vol.volume);
            ImGui::EndGroup    ();

            ImGui::BeginGroup  ();

                   val        = ch_vol.muted ? 0.1f : 0.5f;
            float *pSliderVal = ch_vol.muted ? &ch_vol.normalized : &ch_vol.volume;

            ImGui::PushStyleColor (ImGuiCol_FrameBg,        ImColor::HSV ( ( i + 1 ) / static_cast <float> (channels), 0.5f,  val).Value);
            ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, ImColor::HSV ( ( i + 1 ) / static_cast <float> (channels), 0.6f,  val).Value);
            ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  ImColor::HSV ( ( i + 1 ) / static_cast <float> (channels), 0.7f,  val).Value);
            ImGui::PushStyleColor (ImGuiCol_SliderGrab,     ImColor::HSV ( ( i + 1 ) / static_cast <float> (channels), 0.9f, 0.9f).Value);

            changed =
              ImGui::VSliderFloat ( ch_vol.slider_label,
                                      ImVec2 (font_size * 1.5f, ht),
                                        pSliderVal,
                                          0.0f, 1.0f,
                                            "" );

            ImGui::PopStyleColor  (4);

            if (changed)
            {
               ch_vol.volume =
                 ch_vol.muted  ? ch_vol.normalized :
                                 ch_vol.volume;

              if ( SUCCEEDED ( pChannelVolume->SetChannelVolume ( i,
                                                                    ch_vol.volume,
                                                                      nullptr )
                             )
                 )
              {
                ch_vol.muted      = false;
                ch_vol.normalized = ch_vol.volume;
              }

              else
                ch_vol.normalized = volume_orig;
            }

            if ( SK_ImGui::VerticalToggleButton (  ch_vol.mute_button,
                                                  &ch_vol.muted )
               )
            {
              if (ch_vol.muted)
              {
                ch_vol.normalized = ch_vol.volume;
                pChannelVolume->SetChannelVolume (i, 0.0f,              nullptr);
              }
              else
                pChannelVolume->SetChannelVolume (i, ch_vol.normalized, nullptr);
            }

            ImGui::EndGroup ();
            ImGui::SameLine ();
          }

          if (*history [i].hashed_name == '\0')
          {
            lstrcatA (history [i].hashed_name, "##ChannelHistogram");
            lstrcatA (history [i].hashed_name, std::to_string (i).c_str ());
          }

          ImGui::BeginGroup ();
          ImGui::PushStyleColor (ImGuiCol_PlotHistogram, (ImVec4&&)ImColor::HSV ( ( i + 1 ) / (float)channels, 1.0f, val));
          ImGui::PlotHistogram ( history [i].hashed_name,
                                   history [i].peaks,
                                     IM_ARRAYSIZE (history [i].peaks),
                                       history [i].current_idx,
                                         "",
                                              history [i].vu_peaks.disp_min,
                                              1.0f,
                                               ImVec2 (ImGui::GetContentRegionAvail ().x, ht) );
          ImGui::PopStyleColor  ();

          ImGui::PushStyleColor (ImGuiCol_PlotHistogram,        ImVec4 (0.9f, 0.1f, 0.1f, 0.15f));
          ImGui::ProgressBar    (history [i].vu_peaks.disp_max, ImVec2 (-1.0f, 0.0f));
          ImGui::PopStyleColor  ();

          ImGui::ProgressBar    (channel_peaks_ [i],            ImVec2 (-1.0f, 0.0f));

          ImGui::PushStyleColor (ImGuiCol_PlotHistogram,        ImVec4 (0.1f, 0.1f, 0.9f, 0.15f));
          ImGui::ProgressBar    (history [i].vu_peaks.disp_min, ImVec2 (-1.0f, 0.0f));
          ImGui::PopStyleColor  ();
          ImGui::EndGroup       ();
          ImGui::EndGroup       ();

          if (! (i % 2))
          {
            ImGui::SameLine (); ImGui::NextColumn ();
          } else {
            ImGui::Columns   ( 1 );
            ImGui::Separator (   );
            ImGui::Columns   ( 2 );
          }
        }
      }

      ImGui::Columns (1);
    }

    // Upon failure, deactivate and try to get a new session manager on the next
    //   frame.
    else
    {
      audio_session = nullptr;
      sessions.Deactivate ();
    }
  }

  ImGui::TreePop ();
}

bool
SK::ControlPanel::Sound::Draw (void)
{
  if (ImGui::CollapsingHeader ("Audio Management"))
  {
    SK_ImGui_VolumeManager ();

    return true;
  }

  return false;
}