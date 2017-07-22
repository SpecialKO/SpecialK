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

#define _CRT_SECURE_NO_WARNINGS

#include <Mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <atlbase.h>

#include <SpecialK/config.h>
#include <SpecialK/log.h>
#include <SpecialK/window.h>

#include <unordered_map>
#include <set>

IAudioMeterInformation*
__stdcall
SK_WASAPI_GetAudioMeterInfo (void)
{
  CComPtr <IMMDeviceEnumerator> pDevEnum = nullptr;

  if ( FAILED (
         pDevEnum.CoCreateInstance (__uuidof (MMDeviceEnumerator))
              ) || pDevEnum == nullptr
     ) return nullptr;

  CComPtr <IMMDevice> pDevice = nullptr;
  if ( FAILED (
         pDevEnum->GetDefaultAudioEndpoint ( eRender,
                                               eConsole,
                                                 &pDevice )
              ) || pDevice == nullptr
     ) return nullptr;

  IAudioMeterInformation* pMeterInfo = nullptr;

  HRESULT hr = pDevice->Activate ( __uuidof (IAudioMeterInformation),
                                     CLSCTX_ALL,
                                       nullptr,
                                         IID_PPV_ARGS_Helper (&pMeterInfo) );

  if (SUCCEEDED (hr))
    return pMeterInfo;

  return nullptr;
}

// OLD NAME for DLL Export
IAudioMeterInformation*
__stdcall
SK_GetAudioMeterInfo (void)
{
  return SK_WASAPI_GetAudioMeterInfo ();
}

void
__stdcall
SK_WASAPI_GetAudioSessionProcs (size_t* count, DWORD* procs)
{
  std::set <DWORD> unique_procs;

  size_t max_count = 0;

  if (count != nullptr)
  {
    max_count = *count;
    *count    = 0;
  }

  CComPtr <IMMDeviceEnumerator> pDevEnum;
  if (FAILED ((pDevEnum.CoCreateInstance (__uuidof (MMDeviceEnumerator)))))
    return;

  // Most game audio a user will not want to hear while a game is in the
  //   background will pass through eConsole.
  //
  //   eCommunication will be headset stuff and that's something a user is not
  //     going to appreciate having muted :) Consider overloading this function
  //       to allow independent control.
  //
  CComPtr <IMMDevice> pDevice;
  if ( FAILED (
         pDevEnum->GetDefaultAudioEndpoint ( eRender,
                                               eConsole,
                                                 &pDevice )
              )
     ) return;

  CComPtr <IAudioSessionManager2> pSessionMgr2;
  if (FAILED (pDevice->Activate (
                __uuidof (IAudioSessionManager2),
                  CLSCTX_ALL,
                    nullptr,
                      reinterpret_cast <void **>(&pSessionMgr2)
             )
         )
     ) return;

  CComPtr <IAudioSessionEnumerator> pSessionEnum;
  if (FAILED (pSessionMgr2->GetSessionEnumerator (&pSessionEnum)))
    return;

  int num_sessions;

  if (FAILED (pSessionEnum->GetCount (&num_sessions)))
    return;

  for (int pass = 0; pass < 2; pass++) // First Pass:  Top-level windows
                                       // Second Pass:  Everything else
  for (int i = 0; i < num_sessions; i++)
  {
    CComPtr <IAudioSessionControl> pSessionCtl;
    if (FAILED (pSessionEnum->GetSession (i, &pSessionCtl)))
      continue;

    CComPtr <IAudioSessionControl2> pSessionCtl2;
    if (FAILED (pSessionCtl->QueryInterface (IID_PPV_ARGS (&pSessionCtl2))))
      continue;

    DWORD dwProcess = 0;
    if (FAILED (pSessionCtl2->GetProcessId (&dwProcess)))
      continue;

    AudioSessionState state;

if (SUCCEEDED (pSessionCtl2->GetState (&state)) && state == AudioSessionStateActive)
    {
      if ( unique_procs.count (dwProcess) == 0 && ( max_count == 0 || *count < max_count ) )
      {
        if ((pass == 1 || SK_FindRootWindow (dwProcess).root != 0) || dwProcess == 0)
        {
          if (procs != nullptr)
            procs [unique_procs.size ()] = dwProcess;

          unique_procs.insert (dwProcess);

          if (count != nullptr)
            (*count)++;

          wchar_t* wszDisplayName = nullptr;
          if (SUCCEEDED (pSessionCtl2->GetSessionIdentifier (&wszDisplayName)))
          {
            dll_log.Log (L"Name: %ws", wszDisplayName);
            CoTaskMemFree (wszDisplayName);
          }

          SK_LOG4 ( ( L" Audio Session (pid=%lu)", dwProcess ),
                      L"  WASAPI  " );
        }
      }
    }
  }
}

IAudioSessionControl*
__stdcall
SK_WASAPI_GetAudioSessionControl (DWORD proc_id = GetCurrentProcessId ())
{
  CComPtr <IMMDeviceEnumerator> pDevEnum;
  if (FAILED ((pDevEnum.CoCreateInstance (__uuidof (MMDeviceEnumerator)))))
    return nullptr;

  // Most game audio a user will not want to hear while a game is in the
  //   background will pass through eConsole.
  //
  //   eCommunication will be headset stuff and that's something a user is not
  //     going to appreciate having muted :) Consider overloading this function
  //       to allow independent control.
  //
  CComPtr <IMMDevice> pDevice;
  if ( FAILED (
         pDevEnum->GetDefaultAudioEndpoint ( eRender,
                                               eConsole,
                                                 &pDevice )
              )
     ) return nullptr;

  CComPtr <IAudioSessionManager2> pSessionMgr2;
  if (FAILED (pDevice->Activate (
                __uuidof (IAudioSessionManager2),
                  CLSCTX_ALL,
                    nullptr,
                      reinterpret_cast <void **>(&pSessionMgr2)
             )
         )
     ) return nullptr;

  CComPtr <IAudioSessionEnumerator> pSessionEnum;
  if (FAILED (pSessionMgr2->GetSessionEnumerator (&pSessionEnum)))
    return nullptr;

  int num_sessions;

  if (FAILED (pSessionEnum->GetCount (&num_sessions)))
    return nullptr;

  for (int i = 0; i < num_sessions; i++)
  {
    IAudioSessionControl *pSessionCtl;
    if (FAILED (pSessionEnum->GetSession (i, &pSessionCtl)))
      continue;

    AudioSessionState state;
    if (FAILED (pSessionCtl->GetState (&state)) || state != AudioSessionStateActive)
    {
      pSessionCtl->Release ();
      continue;
    }

    CComPtr <IAudioSessionControl2> pSessionCtl2;
    if (FAILED (pSessionCtl->QueryInterface (IID_PPV_ARGS (&pSessionCtl2))))
    {
      pSessionCtl->Release ();
      continue;
    }

    DWORD dwProcess = 0;
    if (FAILED (pSessionCtl2->GetProcessId (&dwProcess)))
    {
      pSessionCtl->Release ();
      continue;
    }

    if (dwProcess == proc_id)
      return pSessionCtl;

    else
      pSessionCtl->Release ();
  }

  return nullptr;
}

IChannelAudioVolume*
__stdcall
SK_WASAPI_GetChannelVolumeControl (DWORD proc_id)
{
  CComPtr <IAudioSessionControl> pSessionCtl =
    SK_WASAPI_GetAudioSessionControl (proc_id);

  if (pSessionCtl != nullptr)
  {
    IChannelAudioVolume *pChannelAudioVolume = nullptr;

    if (SUCCEEDED (pSessionCtl->QueryInterface (IID_PPV_ARGS (&pChannelAudioVolume))))
      return pChannelAudioVolume;
  }

  return nullptr;
}

ISimpleAudioVolume*
__stdcall
SK_WASAPI_GetVolumeControl (DWORD proc_id)
{
  CComPtr <IAudioSessionControl> pSessionCtl =
    SK_WASAPI_GetAudioSessionControl (proc_id);

  if (pSessionCtl != nullptr)
  {
    ISimpleAudioVolume *pSimpleAudioVolume = nullptr;

    if (SUCCEEDED (pSessionCtl->QueryInterface (IID_PPV_ARGS (&pSimpleAudioVolume))))
      return pSimpleAudioVolume;
  }

  return nullptr;
}

#include <dsound.h>

// Misnomer, since this uses DirectSound instead (much simpler =P)
const char*
__stdcall
SK_WASAPI_GetChannelName (int channel_idx)
{
  static bool init = false;

  static std::unordered_map <int, std::string> channel_names;

  if (! init)
  {
    DWORD dwConfig = 0x00;

    typedef HRESULT (WINAPI *DirectSoundCreate_pfn)(
      LPGUID         lpGuid, 
      LPDIRECTSOUND* ppDS, 
      LPUNKNOWN      pUnkOuter 
    );

    HMODULE hModDSound =
      LoadLibraryW (L"dsound.dll");

    if (hModDSound != nullptr)
    {
      DirectSoundCreate_pfn DirectSoundCreate_Import =
        (DirectSoundCreate_pfn)
          GetProcAddress ( hModDSound,
                             "DirectSoundCreate" );

      if (DirectSoundCreate_Import != nullptr)
      {
        CComPtr <IDirectSound> pDSound = nullptr;

        HRESULT hr;

        if (SUCCEEDED ((hr = DirectSoundCreate_Import (nullptr, &pDSound, nullptr))))
        {
          if (SUCCEEDED ((hr = pDSound->Initialize (nullptr))) || hr == DSERR_ALREADYINITIALIZED)
          {
            if (FAILED ((hr = pDSound->GetSpeakerConfig (&dwConfig))))
            {
              dwConfig = 0x00;
              SK_LOG0 ( ( L" >> IDirectSound::GetSpeakerConfig (...) Failed: hr=%x <<", hr ),
                          L"  DSound  " );
            }
          }

          else
            SK_LOG0 ( ( L" >> IDirectSound::Initialize (...) Failed: hr=%x <<", hr ),
                        L"  DSound  " );
        }

        else
          SK_LOG0 ( ( L" >> DirectSoundCreate (...) Failed: hr=%x <<", hr ),
                      L"  DSound  " );
      }

      FreeLibrary (hModDSound);
    }

    switch (DSSPEAKER_CONFIG (dwConfig))
    {
      case DSSPEAKER_HEADPHONE:
        channel_names.emplace (0, "Headphone Left");
        channel_names.emplace (1, "Headphone Right");
        break;

      case DSSPEAKER_MONO:
      //case KSAUDIO_SPEAKER_MONO:
        channel_names.emplace (0, "Center");
        break;

      case DSSPEAKER_STEREO:
      //case KSAUDIO_SPEAKER_STEREO:
        channel_names.emplace (0, "Front Left");
        channel_names.emplace (1, "Front Right");
        break;

       case DSSPEAKER_QUAD:
      //case KSAUDIO_SPEAKER_QUAD:
        channel_names.emplace (0, "Front Left");
        channel_names.emplace (1, "Front Right");
        channel_names.emplace (2, "Back Left");
        channel_names.emplace (3, "Back Right");
        break;

      case DSSPEAKER_SURROUND:
      //case KSAUDIO_SPEAKER_SURROUND:
        channel_names.emplace (0, "Front Left");
        channel_names.emplace (1, "Front Right");
        channel_names.emplace (2, "Front Center");
        channel_names.emplace (3, "Back Center");
        break;

      case DSSPEAKER_5POINT1:
      //case KSAUDIO_SPEAKER_5POINT1:
        channel_names.emplace (0, "Front Left");
        channel_names.emplace (1, "Front Right");
        channel_names.emplace (2, "Center");
        channel_names.emplace (3, "Low Frequency Emitter");
        channel_names.emplace (4, "Back Left");
        channel_names.emplace (5, "Back Right");
        break;

      case DSSPEAKER_5POINT1_SURROUND:
      //case KSAUDIO_SPEAKER_5POINT1_SURROUND:
        channel_names.emplace (0, "Front Left");
        channel_names.emplace (1, "Front Right");
        channel_names.emplace (2, "Center");
        channel_names.emplace (3, "Low Frequency Emitter");
        channel_names.emplace (4, "Side Left");
        channel_names.emplace (5, "Side Right");
        break;

      case DSSPEAKER_7POINT1:
      //case KSAUDIO_SPEAKER_7POINT1:
        channel_names.emplace (0, "Front Left");
        channel_names.emplace (1, "Front Right");
        channel_names.emplace (2, "Center");
        channel_names.emplace (3, "Low Frequency Emitter");
        channel_names.emplace (4, "Back Left");
        channel_names.emplace (5, "Back Right");
        channel_names.emplace (6, "Front Left of Center");
        channel_names.emplace (7, "Front Right of Center");
        break;

      case DSSPEAKER_7POINT1_SURROUND:
      //case KSAUDIO_SPEAKER_7POINT1_SURROUND:
        channel_names.emplace (0, "Front Left");
        channel_names.emplace (1, "Front Right");
        channel_names.emplace (2, "Center");
        channel_names.emplace (3, "Low Frequency Emitter");
        channel_names.emplace (4, "Back Left");
        channel_names.emplace (5, "Back Right");
        channel_names.emplace (6, "Side Left");
        channel_names.emplace (7, "Side Right");
        break;

      default:
        SK_LOG0 ( ( L" >> UNKNOWN Speaker Config: %x <<", dwConfig ),
                    L"  WASAPI  " );
        break;
    }

    init = true;
  }

  if (channel_names.count (channel_idx))
    return channel_names [channel_idx].c_str ();

  // We couldn't use the Speaker Config to get a name, so just return
  //   the ordinal instead and add that fallback name to the hashmap
  else
  {
    char szChannelOrdinal [32] = { };
    snprintf (szChannelOrdinal, 31, "Unknown Channel (%02i)", channel_idx);

    channel_names.emplace (channel_idx, szChannelOrdinal);

    return channel_names [channel_idx].c_str ();
  }
}

void
__stdcall
SK_SetGameMute (bool bMute)
{
  ISimpleAudioVolume* pVolume =
    SK_WASAPI_GetVolumeControl (GetCurrentProcessId ());

  if (pVolume != nullptr)
  {
      pVolume->SetMute (bMute, nullptr);
      pVolume->Release ();
  }
}



#include <SpecialK/sound.h>

HRESULT
SK_WASAPI_AudioSession::OnStateChanged (AudioSessionState NewState)
{
  parent_->SetSessionState (this, NewState);

  return S_OK;
}
        
HRESULT
SK_WASAPI_AudioSession::OnSessionDisconnected (AudioSessionDisconnectReason DisconnectReason)
{
  UNREFERENCED_PARAMETER (DisconnectReason);

  parent_->RemoveSession (this);

  return S_OK;
}