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

SK_IAudioMeterInformation
__stdcall
SK_WASAPI_GetAudioMeterInfo (void)
{
  SK_IMMDeviceEnumerator    pDevEnum   = nullptr;
  SK_IMMDevice              pDevice    = nullptr;

  SK_IAudioMeterInformation pMeterInfo = nullptr;


  try
  {
    ThrowIfFailed (
      pDevEnum.CoCreateInstance (__uuidof (MMDeviceEnumerator)));

    if (pDevEnum == nullptr)
      return nullptr;

    ThrowIfFailed (
      pDevEnum->GetDefaultAudioEndpoint (eRender,
                                           eConsole,
                                             &pDevice));

    if (pDevice == nullptr)
      return nullptr;

    ThrowIfFailed (
      pDevice->Activate ( __uuidof (IAudioMeterInformation),
                                      CLSCTX_ALL,
                                        nullptr,
                                          IID_PPV_ARGS_Helper (&pMeterInfo.p) ));
  }

  catch (const std::exception& e)
  {
    SK_LOG0 ( ( L"%ws (...) Failed: %hs", __FUNCTIONW__, e.what ()
              ),L"  WASAPI  " );

    pMeterInfo = nullptr;
  }

  return pMeterInfo;
}

// OLD NAME for DLL Export
//   [ Smart Ptr. Not Allowed;  Caller -MUST- Release ]
IAudioMeterInformation*
__stdcall
SK_GetAudioMeterInfo (void)
{
  return
    SK_WASAPI_GetAudioMeterInfo ();
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

  int                        num_sessions = 0;

  SK_IMMDevice               pDevice;
  SK_IMMDeviceEnumerator     pDevEnum;
  SK_IAudioSessionEnumerator pSessionEnum;
  SK_IAudioSessionManager2   pSessionMgr2;
  SK_IAudioSessionControl    pSessionCtl;
  SK_IAudioSessionControl2   pSessionCtl2;

  try
  {
    ThrowIfFailed (
      pDevEnum.CoCreateInstance (__uuidof (MMDeviceEnumerator)));

    ThrowIfFailed (
      pDevEnum->GetDefaultAudioEndpoint (eRender,
                                           eConsole,
                                             &pDevice));

    ThrowIfFailed (
      pDevice->Activate (
                __uuidof (IAudioSessionManager2),
                  CLSCTX_ALL,
                    nullptr,
                      reinterpret_cast <void **>(&pSessionMgr2.p)));

    ThrowIfFailed (
      pSessionMgr2->GetSessionEnumerator (&pSessionEnum.p));

    ThrowIfFailed (
      pSessionEnum->GetCount (&num_sessions));
  }

  catch (const std::exception& e)
  {
    SK_LOG0 ( ( L"%ws (...) Failed "
                L"During Enumerator Setup : %hs", __FUNCTIONW__, e.what ()
              ),L"  WASAPI  " );

    return;
  }


  for (int pass = 0; pass < 2;            pass++) // First Pass:   Top-level windows
  for (int i    = 0;    i < num_sessions; i++   ) // Second Pass:  Everything else
  {
    try
    {
      ThrowIfFailed (
        pSessionEnum->GetSession (i, &pSessionCtl.p));

      ThrowIfFailed (
        pSessionCtl->QueryInterface (IID_PPV_ARGS (&pSessionCtl2.p)));

      DWORD dwProcess = 0;

      ThrowIfFailed (
        pSessionCtl2->GetProcessId (&dwProcess));

      AudioSessionState state;

if (SUCCEEDED (pSessionCtl2->GetState (&state)) && state == AudioSessionStateActive)
      {
        if ( unique_procs.count (dwProcess) == 0  && ( max_count == 0 || (count != nullptr && *count < max_count ) ) )
        {
          if ((pass == 1 || SK_FindRootWindow (dwProcess).root != nullptr) || dwProcess == 0)
          {
            if (procs != nullptr)
              procs [unique_procs.size ()] = dwProcess;

            unique_procs.insert (dwProcess);

            if (count != nullptr)
              (*count)++;

            wchar_t* wszDisplayName = nullptr;
            if (SUCCEEDED (pSessionCtl2->GetSessionIdentifier (&wszDisplayName)))
            {
              dll_log->Log  (L"Name: %ws", wszDisplayName);
              CoTaskMemFree (wszDisplayName);
            }

            SK_LOG4 ( ( L" Audio Session (pid=%lu)", dwProcess ),
                        L"  WASAPI  " );
          }
        }
      }
    }

    catch (const std::exception& e)
    {
      SK_LOG0 ( ( L"%ws (...) Failed "
                  L"During Process Enumeration : %hs", __FUNCTIONW__, e.what ()
                ),L"  WASAPI  " );

      continue;
    }
  }
}

SK_IAudioSessionControl
__stdcall
SK_WASAPI_GetAudioSessionControl ( EDataFlow data_flow     = eRender,
                                   ERole     endpoint_role = eConsole,
                                   DWORD     proc_id       = GetCurrentProcessId () )
{
  SK_IMMDevice               pDevice;
  SK_IMMDeviceEnumerator     pDevEnum;
  SK_IAudioSessionEnumerator pSessionEnum;
  SK_IAudioSessionManager2   pSessionMgr2;
  SK_IAudioSessionControl2   pSessionCtl2;

  int num_sessions = 0;

  try {
    ThrowIfFailed (
      pDevEnum.CoCreateInstance (__uuidof (MMDeviceEnumerator)));

    ThrowIfFailed (
      pDevEnum->GetDefaultAudioEndpoint (data_flow,
                                           endpoint_role,
                                             &pDevice.p));

    ThrowIfFailed (
      pDevice->Activate (
        __uuidof (IAudioSessionManager2),
          CLSCTX_ALL,
            nullptr,
              reinterpret_cast <void **>(&pSessionMgr2.p)));

    ThrowIfFailed (
      pSessionMgr2->GetSessionEnumerator (&pSessionEnum.p));

    ThrowIfFailed (
      pSessionEnum->GetCount (&num_sessions));
  }

  catch (const std::exception& e)
  {
    SK_LOG0 ( ( L"%ws (...) Failed "
                L"During Enumerator Setup : %hs", __FUNCTIONW__, e.what ()
              ),L"  WASAPI  " );

    return nullptr;
  }

  for (int i = 0; i < num_sessions; i++)
  {
    try
    {
      SK_IAudioSessionControl pSessionCtl (nullptr);
          AudioSessionState   state (
            AudioSessionStateInactive
          );          DWORD   dwProcess = 0;

      ThrowIfFailed (
        pSessionEnum->GetSession (i, &pSessionCtl.p));

      ThrowIfFailed (
        pSessionCtl->GetState (&state));

      if (AudioSessionStateActive != state)
        continue;

      ThrowIfFailed (
          pSessionCtl->QueryInterface (
            IID_PPV_ARGS (&pSessionCtl2.p)));

      ThrowIfFailed (
        pSessionCtl2->GetProcessId (&dwProcess));

      if (dwProcess == proc_id)
        return pSessionCtl;
    }

    catch (const std::exception& e)
    {
      SK_LOG0 ( ( L"%ws (...) Failed "
                  L"During Session Enumeration : %hs", __FUNCTIONW__, e.what ()
                ),L"  WASAPI  " );
    }
  }

  return nullptr;
}

SK_IChannelAudioVolume
__stdcall
SK_WASAPI_GetChannelVolumeControl (DWORD proc_id)
{
  SK_IAudioSessionControl pSessionCtl =
    SK_WASAPI_GetAudioSessionControl (eRender, eConsole, proc_id);

  if (pSessionCtl != nullptr)
  {
    SK_IChannelAudioVolume pChannelAudioVolume;

    if (SUCCEEDED (pSessionCtl->QueryInterface (IID_PPV_ARGS (&pChannelAudioVolume.p))))
      return pChannelAudioVolume.p;
  }

  return nullptr;
}

SK_ISimpleAudioVolume
__stdcall
SK_WASAPI_GetVolumeControl (DWORD proc_id)
{
  SK_IAudioSessionControl pSessionCtl =
    SK_WASAPI_GetAudioSessionControl (eRender, eConsole, proc_id);

  if (pSessionCtl != nullptr)
  {
    SK_ISimpleAudioVolume pSimpleAudioVolume;

    if (SUCCEEDED (pSessionCtl->QueryInterface (IID_PPV_ARGS (&pSimpleAudioVolume.p))))
      return pSimpleAudioVolume;
  }

  return nullptr;
}

SK_IAudioEndpointVolume
__stdcall
SK_MMDev_GetEndpointVolumeControl (void)
{
  SK_IAudioEndpointVolume pEndVol  = nullptr;
  SK_IMMDeviceEnumerator  pDevEnum = nullptr;
  SK_IMMDevice            pDevice  = nullptr;

  try {
    ThrowIfFailed (
      pDevEnum.CoCreateInstance (__uuidof (MMDeviceEnumerator)));

    ThrowIfFailed (
      pDevEnum->GetDefaultAudioEndpoint (eRender,
                                           eConsole,
                                             &pDevice.p));

    ThrowIfFailed (
      pDevice->Activate (__uuidof (IAudioEndpointVolume),
                           CLSCTX_ALL,
                             nullptr,
                               IID_PPV_ARGS_Helper (&pEndVol.p)));
  }

  catch (const std::exception& e)
  {
    SK_LOG0 ( ( L"%ws (...) Failed: %hs", __FUNCTIONW__, e.what ()
              ),L"  WASAPI  " );

    pEndVol = nullptr;
  }

  return pEndVol;
}

SK_IAudioLoudness
__stdcall
SK_MMDev_GetLoudness (void)
{
  SK_IAudioLoudness      pLoudness = nullptr;
  SK_IMMDeviceEnumerator pDevEnum  = nullptr;
  SK_IMMDevice           pDevice   = nullptr;

  try
  {
    ThrowIfFailed (
      pDevEnum.CoCreateInstance (__uuidof (MMDeviceEnumerator)));

    ThrowIfFailed (
      pDevEnum->GetDefaultAudioEndpoint (eRender,
                                           eConsole,
                                             &pDevice));

    pDevice->Activate (__uuidof (IAudioLoudness),
                         CLSCTX_ALL,
                           nullptr,
                             IID_PPV_ARGS_Helper (&pLoudness.p));
  }

  catch (const std::exception& e)
  {
    // This is optional and almost nothing implements it,
    //   silence this at log level 0!
    SK_LOG1 ( ( L"%ws (...) Failed: %hs", __FUNCTIONW__, e.what ()
              ),L"  WASAPI  " );

    pLoudness = nullptr;
  }

  return pLoudness;
}

SK_IAudioAutoGainControl
__stdcall
SK_MMDev_GetAutoGainControl (void)
{
  SK_IAudioAutoGainControl pAutoGain = nullptr;
  SK_IMMDeviceEnumerator   pDevEnum  = nullptr;
  SK_IMMDevice             pDevice   = nullptr;

  try
  {
    ThrowIfFailed (
      pDevEnum.CoCreateInstance (__uuidof (MMDeviceEnumerator)));

    ThrowIfFailed (
      pDevEnum->GetDefaultAudioEndpoint (eRender,
                                           eConsole,
                                             &pDevice.p));

    pDevice->Activate (__uuidof (IAudioAutoGainControl),
                         CLSCTX_ALL,
                           nullptr,
                             IID_PPV_ARGS_Helper (&pAutoGain.p));
  }

  catch (const std::exception& e)
  {
    // This is optional and almost nothing implements it,
    //   silence this at log level 0!
    SK_LOG1 ( ( L"%ws (...) Failed: %hs", __FUNCTIONW__, e.what ()
              ),L"  WASAPI  " );
  }

  return pAutoGain;
}

#include <dsound.h>

// Misnomer, since this uses DirectSound instead (much simpler =P)
const char*
__stdcall
SK_WASAPI_GetChannelName (int channel_idx)
{
  static bool init = false;

  static std::unordered_map <int, std::string> channel_names;

  // Delay this because of the Steam Overlay
  if ((! init) && SK_GetFramesDrawn () > 1)
  {
    DWORD dwConfig = 0x00;

    using DirectSoundCreate_pfn = HRESULT (WINAPI *)(
      LPGUID         lpGuid,
      LPDIRECTSOUND* ppDS,
      LPUNKNOWN      pUnkOuter
    );

    HMODULE hModDSound =
      SK_LoadLibraryW (L"dsound.dll");

    if (hModDSound != nullptr)
    {
      auto DirectSoundCreate_Import =
        reinterpret_cast <DirectSoundCreate_pfn> (
       SK_GetProcAddress ( hModDSound,
                             "DirectSoundCreate" )
        );

      if (DirectSoundCreate_Import != nullptr)
      {
        SK_ComPtr <IDirectSound> pDSound = nullptr;

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

      SK_FreeLibrary (hModDSound);
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
    static char szChannelOrdinal [32] = { };
    snprintf   (szChannelOrdinal, 31, "Unknown Channel (%02i)", channel_idx);

    if (! init)
      return szChannelOrdinal;

    channel_names.emplace (channel_idx, szChannelOrdinal);

    return channel_names [channel_idx].c_str ();
  }
}

void
__stdcall
SK_SetGameMute (bool bMute)
{
  SK_ISimpleAudioVolume pVolume =
    SK_WASAPI_GetVolumeControl (GetCurrentProcessId ());

  if (pVolume != nullptr)
      pVolume->SetMute (bMute, nullptr);
}

BOOL
__stdcall
SK_IsGameMuted (void)
{
  SK_ISimpleAudioVolume pVolume =
    SK_WASAPI_GetVolumeControl (GetCurrentProcessId ());

  BOOL bMuted = FALSE;

  if ( pVolume != nullptr && SUCCEEDED (
       pVolume->GetMute ( &bMuted )    )
     ) return              bMuted;

  SK_ReleaseAssert (! L"pVolume->GetMute (...) Failed");

  return
    bMuted;
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

SK_IAudioEndpointVolume
SK_WASAPI_AudioSession::getEndpointVolume (void)
{
  return
    parent_->endpoint_vol_;
}

SK_IAudioLoudness
SK_WASAPI_AudioSession::getLoudness (void)
{
  return
    parent_->loudness_;
}

SK_IAudioAutoGainControl
SK_WASAPI_AudioSession::getAutoGainControl (void)
{
  return
    parent_->auto_gain_;
}