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

#include <imgui/imgui.h>

#include <SpecialK/control_panel.h>
#include <SpecialK/control_panel/sound.h>

#include <SpecialK/sound.h>

#include <map>
#include <string>

#include <TlHelp32.h>

static SK_WASAPI_SessionManager sessions;
static SK_WASAPI_AudioSession*  audio_session;

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

using namespace SK::ControlPanel;

bool
SK_ImGui_SelectAudioSessionDlg (void)
{
  ImGuiIO& io (ImGui::GetIO ());

         bool  changed   = false;
  const  float font_size = ImGui::GetFont ()->FontSize * io.FontGlobalScale;

  int            sel_idx = -1;


  ImGui::SetNextWindowSizeConstraints ( ImVec2 (io.DisplaySize.x * 0.25f,
                                                io.DisplaySize.y * 0.15f),
                                        ImVec2 (io.DisplaySize.x * 0.75f,
                                                io.DisplaySize.y * 0.666f) );

  if (ImGui::BeginPopupModal ("Audio Session Selector", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders |
                                                                 ImGuiWindowFlags_NoScrollbar      | ImGuiWindowFlags_NoScrollWithMouse))
  {
    int count = 0;

    SK_WASAPI_AudioSession** pSessions =
      sessions.getActive (&count);

    float max_width =
      ImGui::CalcTextSize ("Audio Session Selector").x;

    for (int i = 0; i < count; i++)
    {
      ImVec2 size = ImGui::CalcTextSize (pSessions [i]->getName ());

      if (size.x > max_width) max_width = size.x;
    }
    ImGui::PushItemWidth (max_width * 5.0f * io.FontGlobalScale);

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
    ImGui::BeginGroup ( );
    {
      ImGui::PushStyleColor (ImGuiCol_ChildWindowBg, ImColor (0, 0, 0, 0));
      ImGui::BeginChild     ("SessionSelectData",    ImVec2  (0, 0), true,  ImGuiWindowFlags_NavFlattened);

      ImGui::BeginGroup ();//"SessionSelectData");
      ImGui::Columns    (2);

      for (int i = 0; i < count; i++)
      {
        SK_WASAPI_AudioSession* pSession =
          pSessions [i];

        //bool selected     = false;
        bool drawing_self = (pSession->getProcessId () == GetCurrentProcessId ());

        CComPtr <ISimpleAudioVolume> volume_ctl =
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

          ImGui::PushStyleColor (ImGuiCol_Text,           mute ? ImColor (0.5f, 0.5f, 0.5f) : ImColor (1.0f, 1.0f, 1.0f));
          ImGui::PushStyleColor (ImGuiCol_FrameBg,        ImColor::HSV ( 0.4f * volume, 0.6f, mute ? 0.2f : 0.4f));
          ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, ImColor::HSV ( 0.4f * volume, 0.7f, mute ? 0.2f : 0.4f));
          ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  ImColor::HSV ( 0.4f * volume, 0.8f, mute ? 0.2f : 0.4f));
          ImGui::PushStyleColor (ImGuiCol_SliderGrab,     ImColor::HSV ( 0.4f * volume, 0.9f,        0.6f * (mute ? 0.5f : 1.0f)));

          volume *= 100.0f;

          ImGui::PushItemWidth (ImGui::GetContentRegionAvailWidth () - 37.0f);
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

    ImGui::PopItemWidth ();
    ImGui::EndPopup     ();
  }

  return changed;
}

void
SK_ImGui_VolumeManager (void)
{
  ImGuiIO& io (ImGui::GetIO ());

  const  float font_size           =             ImGui::GetFont  ()->FontSize                        * io.FontGlobalScale;
  const  float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

  std::string app_name;

  sessions.Activate ();

  CComPtr <IAudioMeterInformation> pMeterInfo =
    sessions.getMeterInfo ();

  if (pMeterInfo == nullptr)
    audio_session = nullptr;

  if (audio_session != nullptr)
    app_name = audio_session->getName ();

  app_name += "###AudioSessionAppName";

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
    if (ImGui::Button (u8"  <<  "))
    {
      ISteamMusic* pMusic =
        SK_SteamAPI_Music ();

      if (pMusic && pMusic->BIsEnabled ())
      {
        if (pMusic->BIsPlaying ()) pMusic->PlayPrevious ();
      }

      keybd_event_Original (VK_MEDIA_PREV_TRACK, 0, KEYEVENTF_EXTENDEDKEY,                   0);
      keybd_event_Original (VK_MEDIA_PREV_TRACK, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
    }

    ImGui::SameLine ();

    if (ImGui::Button (u8"  Play / Pause  "))
    {
      ISteamMusic* pMusic =
        SK_SteamAPI_Music ();

      if (pMusic && pMusic->BIsEnabled ())
      {
        if (pMusic->BIsPlaying ()) pMusic->Pause ();
        else                       pMusic->Play  ();
      }


      keybd_event_Original (VK_MEDIA_PLAY_PAUSE, 0, KEYEVENTF_EXTENDEDKEY,                   0);
      keybd_event_Original (VK_MEDIA_PLAY_PAUSE, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0); 
    }

    ImGui::SameLine ();

    if (ImGui::Button (u8"  >>  "))
    {
      ISteamMusic* pMusic =
        SK_SteamAPI_Music ();

      if (pMusic && pMusic->BIsEnabled ())
      {
        if (pMusic->BIsPlaying ()) pMusic->PlayNext ();
      }


      keybd_event_Original (VK_MEDIA_NEXT_TRACK, 0, KEYEVENTF_EXTENDEDKEY,                   0);
      keybd_event_Original (VK_MEDIA_NEXT_TRACK, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
    }
    ImGui::PopItemWidth ();
  }
  ImGui::EndGroup   ();
  ImGui::Columns    (1);

  ImGui::PopStyleColor (3);


  bool session_changed = SK_ImGui_SelectAudioSessionDlg ();

  ImGui::TreePush ("");

  if (audio_session == nullptr)
  {
    int count;

    SK_WASAPI_AudioSession** ppSessions =
      sessions.getActive (&count);

    // Find the session for the current game and select that first...
    for (int i = 0; i < count; i++)
    {
      if (ppSessions [i]->getProcessId () == GetCurrentProcessId ())
      {
        audio_session = ppSessions [i];
        break;
      }
    }
  }

  else
  {
    CComPtr <IChannelAudioVolume> pChannelVolume =
      audio_session->getChannelAudioVolume ();
    CComPtr <ISimpleAudioVolume>  pVolume        =
      audio_session->getSimpleAudioVolume ();

    UINT channels = 0;

    if (SUCCEEDED (pMeterInfo->GetMeteringChannelCount (&channels)))
    {
      static float channel_peaks_ [32] { };

      struct
      {
        struct {
          float inst_min = FLT_MAX;  DWORD dwMinSample = 0;  float disp_min = FLT_MAX;
          float inst_max = FLT_MIN;  DWORD dwMaxSample = 0;  float disp_max = FLT_MIN;
        } vu_peaks;

        float peaks    [120] { };
        int   current_idx =  0L;
        DWORD update_time =  0UL;
      } static history [ 32] { };

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

      static std::map <int, volume_s> channel_volumes;

      pVolume->GetMasterVolume (&master_vol);
      pVolume->GetMute         (&master_mute);

      const char* szMuteButtonTitle = ( master_mute ? "  Unmute  ###MasterMute" :
                                                      "   Mute   ###MasterMute" );

      if (ImGui::Button (szMuteButtonTitle))
      {
        master_mute ^= TRUE;

        pVolume->SetMute (master_mute, nullptr);
      }

      ImGui::SameLine ();

      float val = master_mute ? 0.0f : 1.0f;

      ImGui::PushStyleColor (ImGuiCol_FrameBg,        ImColor ( 0.3f,  0.3f,  0.3f,  val));
      ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, ImColor ( 0.6f,  0.6f,  0.6f,  val));
      ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  ImColor ( 0.9f,  0.9f,  0.9f,  val));
      ImGui::PushStyleColor (ImGuiCol_SliderGrab,     ImColor ( 1.0f,  1.0f,  1.0f, 1.0f));
      ImGui::PushStyleColor (ImGuiCol_Text,           ImColor::HSV ( 0.15f, 0.0f,
                                                                       0.5f + master_vol * 0.5f) );

      if (ImGui::SliderFloat ("      Master Volume Control  ", &master_vol, 0.0, 1.0, ""))
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
                               master_vol * 100.0f );

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

          if (channel_volumes.count (i) == 0)
          {
            session_changed = true;

            snprintf (channel_volumes [i].mute_button, 13, "  Mute  ##%lu", i);
            snprintf (channel_volumes [i].slider_label, 7, "##vol%lu",      i);
          }

          if (pChannelVolume && SUCCEEDED (pChannelVolume->GetChannelVolume (i, &channel_volumes [i].volume)))
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
            float volume_orig = ch_vol.normalized;

            ImGui::BeginGroup  ();
            ImGui::TextColored (ImVec4 (0.80f, 0.90f, 1.0f,  1.0f), "%-22s", SK_WASAPI_GetChannelName (i));
                                                                                      ImGui::SameLine ( );
            ImGui::TextColored (ImVec4 (0.7f,  0.7f,  0.7f,  1.0f), "      Volume:"); ImGui::SameLine ( );
            ImGui::TextColored (ImColor::HSV (
                                        0.25f, 0.9f,
                                          0.5f + ch_vol.volume * 0.5f),
                                                                    "%03.1f%%   ", 100.0f * ch_vol.volume);
            ImGui::EndGroup    ();

            ImGui::BeginGroup  ();

                   val        = ch_vol.muted ? 0.1f : 0.5f;
            float *pSliderVal = ch_vol.muted ? &ch_vol.normalized : &ch_vol.volume;

            ImGui::PushStyleColor (ImGuiCol_FrameBg,        ImColor::HSV ( ( i + 1 ) / (float)channels, 0.5f, val));
            ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, ImColor::HSV ( ( i + 1 ) / (float)channels, 0.6f, val));
            ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  ImColor::HSV ( ( i + 1 ) / (float)channels, 0.7f, val));
            ImGui::PushStyleColor (ImGuiCol_SliderGrab,     ImColor::HSV ( ( i + 1 ) / (float)channels, 0.9f, 0.9f));

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

          ImGui::BeginGroup ();
          ImGui::PushStyleColor (ImGuiCol_PlotHistogram, ImColor::HSV ( ( i + 1 ) / (float)channels, 1.0f, val));
          ImGui::PlotHistogram ( "",
                                   history [i].peaks,
                                     IM_ARRAYSIZE (history [i].peaks),
                                       history [i].current_idx,
                                         "",
                                              history [i].vu_peaks.disp_min,
                                              1.0f,
                                               ImVec2 (ImGui::GetContentRegionAvailWidth (), ht) );
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
  if (ImGui::CollapsingHeader ("Volume Management"))
  {
    SK_ImGui_VolumeManager ();

    return true;
  }

  return false;
}