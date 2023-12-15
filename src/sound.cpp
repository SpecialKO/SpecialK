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

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"  WASAPI  "

const IID IID_IAudioClient3 = __uuidof(IAudioClient3);

SK_IAudioClient3
__stdcall
SK_WASAPI_GetAudioClient (SK_IMMDevice pDevice, bool uncached)
{
  static SK_IAudioClient3 pCachedClient = nullptr;
  static DWORD            dwLastUpdate  = 0;

  // TODO: Stash this in the session manager SK already has, and keep it
  //         around persistently
  if (SK::ControlPanel::current_time > dwLastUpdate + 2500UL || uncached)
  {
    dwLastUpdate = SK::ControlPanel::current_time;

    SK_IMMDeviceEnumerator pDevEnum = nullptr;

    try
    {
      if (pDevice == nullptr)
      {
        ThrowIfFailed (
          pDevEnum.CoCreateInstance (__uuidof (MMDeviceEnumerator)));

        if (pDevEnum == nullptr)
          return nullptr;

        ThrowIfFailed (
          pDevEnum->GetDefaultAudioEndpoint (eRender,
                                               eConsole,
                                                 &pDevice));
      }

      if (pDevice == nullptr)
        return nullptr;

      SK_ComPtr <IAudioClient3> pAudioClient;

      ThrowIfFailed (
        pDevice->Activate (IID_IAudioClient3, CLSCTX_ALL, nullptr, IID_PPV_ARGS_Helper (&pAudioClient.p)));

      pCachedClient =
        pAudioClient;
    }

    catch (const std::exception& e)
    {
      SK_LOG0 ( ( L"%ws (...) Failed: %hs", __FUNCTIONW__, e.what ()
                ),L"  WASAPI  " );

      return nullptr;
    }
  }

  return
    pCachedClient;
}

SK_IAudioMeterInformation
__stdcall
SK_WASAPI_GetAudioMeterInfo (SK_IMMDevice pDevice)
{
  SK_IMMDeviceEnumerator    pDevEnum   = nullptr;
  SK_IAudioMeterInformation pMeterInfo = nullptr;

  try
  {
    if (pDevice == nullptr)
    {
      ThrowIfFailed (
        pDevEnum.CoCreateInstance (__uuidof (MMDeviceEnumerator)));

      if (pDevEnum == nullptr)
        return nullptr;

      ThrowIfFailed (
        pDevEnum->GetDefaultAudioEndpoint (eRender,
                                             eConsole,
                                               &pDevice));
    }

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

SK_WASAPI_AudioLatency
__stdcall
SK_WASAPI_GetCurrentLatency (SK_IMMDevice pDevice)
{
  auto pAudioClient =
    SK_WASAPI_GetAudioClient (pDevice);

  if (! pAudioClient.p)
    return { 0.0f, 0 };

  try
  {
    WAVEFORMATEX                                      *pFormat;
    UINT32                                                       currentPeriodInFrames;
    ThrowIfFailed (
      pAudioClient->GetCurrentSharedModeEnginePeriod (&pFormat, &currentPeriodInFrames));

    return
    {
      static_cast <float> ((1.0 / static_cast <double> (pFormat->nSamplesPerSec) * currentPeriodInFrames) * 1000.0),
                                                                                   currentPeriodInFrames,
                                                        pFormat->nSamplesPerSec
    };
  }

  catch (const std::exception& e)
  {
    SK_LOG0 ( ( L"%ws (...) Failed: %hs", __FUNCTIONW__, e.what ()
              ),L"  WASAPI  " );
  }

  return
    { 0.0f, 0 };
}

SK_WASAPI_AudioLatency
__stdcall
SK_WASAPI_GetDefaultLatency (SK_IMMDevice pDevice)
{
  auto pAudioClient =
    SK_WASAPI_GetAudioClient (pDevice);

  if (! pAudioClient.p)
    return { 0.0f, 0 };

  try
  {
    WAVEFORMATEX *pFormat;

    ThrowIfFailed (
      pAudioClient->GetMixFormat (&pFormat));

    UINT32 defaultPeriodInFrames;
    UINT32 fundamentalPeriodInFrames;
    UINT32 minPeriodInFrames;
    UINT32 maxPeriodInFrames;

    ThrowIfFailed (
      pAudioClient->GetSharedModeEnginePeriod ( pFormat,
                                                  &defaultPeriodInFrames,
                                              &fundamentalPeriodInFrames,
                                                      &minPeriodInFrames,
                                                      &maxPeriodInFrames ));

    return
    {
      static_cast <float> ((1.0 / static_cast <double> (pFormat->nSamplesPerSec) * defaultPeriodInFrames) * 1000.0),
                                                                                   defaultPeriodInFrames,
                                                        pFormat->nSamplesPerSec
    };
  }

  catch (const std::exception& e)
  {
    SK_LOG0 ( ( L"%ws (...) Failed: %hs", __FUNCTIONW__, e.what ()
              ),L"  WASAPI  " );
  }

  return
    { 0.0f, 0 };
}

SK_WASAPI_AudioLatency
__stdcall
SK_WASAPI_GetMinimumLatency (SK_IMMDevice pDevice)
{
  auto pAudioClient =
    SK_WASAPI_GetAudioClient (pDevice);

  if (! pAudioClient.p)
    return { 0.0f, 0 };

  try
  {
    WAVEFORMATEX *pFormat;

    ThrowIfFailed (
      pAudioClient->GetMixFormat (&pFormat));

    UINT32 defaultPeriodInFrames;
    UINT32 fundamentalPeriodInFrames;
    UINT32 minPeriodInFrames;
    UINT32 maxPeriodInFrames;

    ThrowIfFailed (
      pAudioClient->GetSharedModeEnginePeriod ( pFormat,
                                                  &defaultPeriodInFrames,
                                              &fundamentalPeriodInFrames,
                                                      &minPeriodInFrames,
                                                      &maxPeriodInFrames ));

    return
    {
      static_cast <float> ((1.0 / static_cast <double> (pFormat->nSamplesPerSec) * minPeriodInFrames) * 1000.0),
                                                                                   minPeriodInFrames,
                                                        pFormat->nSamplesPerSec
    };
  }

  catch (const std::exception& e)
  {
    SK_LOG0 ( ( L"%ws (...) Failed: %hs", __FUNCTIONW__, e.what ()
              ),L"  WASAPI  " );
  }

  return
    { 0.0f, 0 };
}

SK_WASAPI_AudioLatency
__stdcall
SK_WASAPI_GetMaximumLatency (void)
{
  auto pAudioClient =
    SK_WASAPI_GetAudioClient ();

  if (! pAudioClient.p)
    return { 0.0f, 0 };

  try
  {
    WAVEFORMATEX *pFormat;

    ThrowIfFailed (
      pAudioClient->GetMixFormat (&pFormat));

    UINT32 defaultPeriodInFrames;
    UINT32 fundamentalPeriodInFrames;
    UINT32 minPeriodInFrames;
    UINT32 maxPeriodInFrames;

    ThrowIfFailed (
      pAudioClient->GetSharedModeEnginePeriod ( pFormat,
                                                  &defaultPeriodInFrames,
                                              &fundamentalPeriodInFrames,
                                                      &minPeriodInFrames,
                                                      &maxPeriodInFrames ));

    return
    {
      static_cast <float> ((1.0 / static_cast <double> (pFormat->nSamplesPerSec) * maxPeriodInFrames) * 1000.0),
                                                                                   maxPeriodInFrames,
                                                        pFormat->nSamplesPerSec
    };
  }

  catch (const std::exception& e)
  {
    SK_LOG0 ( ( L"%ws (...) Failed: %hs", __FUNCTIONW__, e.what ()
              ),L"  WASAPI  " );
  }

  return
    { 0.0f, 0 };
}

SK_WASAPI_AudioLatency
__stdcall
SK_WASAPI_SetLatency (SK_WASAPI_AudioLatency latency, SK_IMMDevice pDevice)
{
  auto pAudioClient =
    SK_WASAPI_GetAudioClient (pDevice);

  if (! pAudioClient.p)
    return { 0.0f, 0 };

  try
  {
    WAVEFORMATEX *pFormat;

    ThrowIfFailed (
      pAudioClient->GetMixFormat (&pFormat));

    ThrowIfFailed (
      pAudioClient->InitializeSharedAudioStream (0, latency.frames, pFormat, nullptr));

    ThrowIfFailed (
      pAudioClient->Start ());

    // We need to keep this alive after setting it, we can destroy it if changing it.
    static SK_IAudioClient3
        pPersistentClient  = nullptr;
    if (pPersistentClient != nullptr)
        pPersistentClient.Release ();

    pPersistentClient =
      pAudioClient.Detach ();

    return
    {
      static_cast <float> ((1.0 / static_cast <double> (pFormat->nSamplesPerSec) * latency.frames) * 1000.0),
                                                                                   latency.frames,
                                                        pFormat->nSamplesPerSec
    };
  }

  catch (const std::exception& e)
  {
    SK_LOG0 ( ( L"%ws (...) Failed: %hs", __FUNCTIONW__, e.what ()
              ),L"  WASAPI  " );
  }

  return
    { 0.0f, 0 };
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
SK_WASAPI_GetAudioSessionControl ( EDataFlow    data_flow     = eRender,
                                   ERole        endpoint_role = eConsole,
                                   SK_IMMDevice pDevice       = nullptr,
                                   DWORD        proc_id       = GetCurrentProcessId () )
{
  SK_IMMDeviceEnumerator     pDevEnum;
  SK_IAudioSessionEnumerator pSessionEnum;
  SK_IAudioSessionManager2   pSessionMgr2;
  SK_IAudioSessionControl2   pSessionCtl2;

  int num_sessions = 0;

  try {
    if (pDevice == nullptr)
    {
      ThrowIfFailed (
        pDevEnum.CoCreateInstance (__uuidof (MMDeviceEnumerator)));

      ThrowIfFailed (
        pDevEnum->GetDefaultAudioEndpoint ( data_flow,
                                              endpoint_role,
                                                &pDevice.p ));
    }

    if (! pDevice)
      return nullptr;

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
    auto _LogException = [&](void)
    {
      SK_LOG0 ( ( L"%ws (...) Failed "
                  L"During Enumerator Setup : %hs", __FUNCTIONW__, e.what ()
                ),L"  WASAPI  " );
    };

    if (config.system.log_level > 0)
    {
      _LogException ();
    }

    else
    {
      // This will happen a lot, just log it once...
      SK_RunOnce (_LogException ());
    }

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
SK_WASAPI_GetChannelVolumeControl (DWORD proc_id, SK_IMMDevice pDevice)
{
  SK_IAudioSessionControl pSessionCtl =
    SK_WASAPI_GetAudioSessionControl (eRender, eConsole, pDevice, proc_id);

  if (pSessionCtl != nullptr)
  {
    SK_IChannelAudioVolume                                     pChannelAudioVolume;
    if (SUCCEEDED (pSessionCtl->QueryInterface (IID_PPV_ARGS (&pChannelAudioVolume.p))))
      return                                                   pChannelAudioVolume.p;
  }

  return nullptr;
}

SK_ISimpleAudioVolume
__stdcall
SK_WASAPI_GetVolumeControl (DWORD proc_id, SK_IMMDevice pDevice)
{
  SK_IAudioSessionControl pSessionCtl =
    SK_WASAPI_GetAudioSessionControl (eRender, eConsole, pDevice, proc_id);

  if (pSessionCtl != nullptr)
  {
    SK_ISimpleAudioVolume                                      pSimpleAudioVolume;
    if (SUCCEEDED (pSessionCtl->QueryInterface (IID_PPV_ARGS (&pSimpleAudioVolume.p))))
      return                                                   pSimpleAudioVolume;
  }

  return nullptr;
}

SK_IAudioEndpointVolume
__stdcall
SK_MMDev_GetEndpointVolumeControl (SK_IMMDevice pDevice)
{
  SK_IAudioEndpointVolume pEndVol  = nullptr;
  SK_IMMDeviceEnumerator  pDevEnum = nullptr;

  try {
    if (pDevice == nullptr)
    {
      ThrowIfFailed (
        pDevEnum.CoCreateInstance (__uuidof (MMDeviceEnumerator)));

      ThrowIfFailed (
        pDevEnum->GetDefaultAudioEndpoint (eRender,
                                             eConsole,
                                               &pDevice.p));
    }

    if (pDevice == nullptr)
      return nullptr;

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
SK_MMDev_GetLoudness (SK_IMMDevice pDevice)
{
  SK_IAudioLoudness      pLoudness = nullptr;
  SK_IMMDeviceEnumerator pDevEnum  = nullptr;

  try
  {
    if (pDevice == nullptr)
    {
      ThrowIfFailed (
        pDevEnum.CoCreateInstance (__uuidof (MMDeviceEnumerator)));

      ThrowIfFailed (
        pDevEnum->GetDefaultAudioEndpoint (eRender,
                                             eConsole,
                                               &pDevice));
    }

    if (pDevice == nullptr)
      return nullptr;

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
SK_MMDev_GetAutoGainControl (SK_IMMDevice pDevice)
{
  SK_IAudioAutoGainControl pAutoGain = nullptr;
  SK_IMMDeviceEnumerator   pDevEnum  = nullptr;

  try
  {
    if (pDevice == nullptr)
    {
      ThrowIfFailed (
        pDevEnum.CoCreateInstance (__uuidof (MMDeviceEnumerator)));

      ThrowIfFailed (
        pDevEnum->GetDefaultAudioEndpoint (eRender,
                                             eConsole,
                                               &pDevice.p));
    }

    if (pDevice == nullptr)
      return nullptr;

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

bool
__stdcall
SK_WASAPI_Init (void)
{
  auto pVolumeCtl =
    SK_WASAPI_GetVolumeControl ();

  if (pVolumeCtl == nullptr)
    return false;

  static float volume = 100.0f;
  static bool  mute   = SK_IsGameMuted ();

  static bool        once = false;
  if (std::exchange (once, true))
    return true;

  pVolumeCtl->GetMasterVolume (&volume);
                                volume *= 100.0f;

  auto cmd =
    SK_GetCommandProcessor ();

  class SoundListener : public SK_IVariableListener
  {
  public:
    virtual bool OnVarChange (SK_IVariable* var, void* val = nullptr)
    {
      if (val != nullptr && var != nullptr )
      {
        auto pVolumeCtl =
          SK_WASAPI_GetVolumeControl ();

        if (pVolumeCtl != nullptr && var->getValuePointer () == &volume)
        {
          volume = *(float *)val;

          volume =
            std::clamp (volume, 0.0f, 100.0f);

          pVolumeCtl->SetMasterVolume (volume / 100.0f, nullptr);
        }

        else if (var->getValuePointer () == &mute)
        {
          mute = *(bool *)val;

          SK_SetGameMute (mute);
        }
      }

      return true;
    }
  } static sound_control;

  cmd->AddVariable ("Sound.Volume", SK_CreateVar (SK_IVariable::Float,   &volume, &sound_control));
  cmd->AddVariable ("Sound.Mute",   SK_CreateVar (SK_IVariable::Boolean, &mute,   &sound_control));

  auto cur_lat = SK_WASAPI_GetCurrentLatency ();
  auto min_lat = SK_WASAPI_GetMinimumLatency ();

  SK_LOGi0 (
    L"Current Audio Mixing Latency: %.1f ms @ %d kHz", cur_lat.milliseconds,
                                                       cur_lat.samples_per_sec / 1000UL
  );
  SK_LOGi0 (
    L"Minimum Audio Mixing Latency: %.1f ms @ %d kHz", min_lat.milliseconds,
                                                       min_lat.samples_per_sec / 1000UL
  );

  if (config.sound.minimize_latency)
  {
    if (cur_lat.frames       != 0 && min_lat.frames != 0 &&
        cur_lat.milliseconds !=      min_lat.milliseconds)
    {
      auto latency =
        SK_WASAPI_SetLatency (min_lat);

      SK_LOG0 ( ( L"Shared Mixing Latency Changed from %.1f ms to %.1f ms",
                    cur_lat.milliseconds, latency.milliseconds
              ),L"  WASAPI  " );
    }
  }

  return true;
}

#include <SpecialK/sound.h>

HRESULT
STDMETHODCALLTYPE
SK_WASAPI_AudioSession::OnDisplayNameChanged (PCWSTR NewDisplayName, LPCGUID EventContext)
{
  UNREFERENCED_PARAMETER (NewDisplayName);
  UNREFERENCED_PARAMETER (EventContext);

  this->app_name_ =
    SK_WideCharToUTF8 (NewDisplayName);

  parent_->signalReset ();

  return S_OK;
};

HRESULT
SK_WASAPI_AudioSession::OnStateChanged (AudioSessionState NewState)
{
  parent_->SetSessionState (this, NewState);
  parent_->signalReset     ();

  return S_OK;
}

HRESULT
SK_WASAPI_AudioSession::OnSessionDisconnected (AudioSessionDisconnectReason DisconnectReason)
{
  UNREFERENCED_PARAMETER (DisconnectReason);

  parent_->RemoveSession (this);
  parent_->signalReset   ();

  return S_OK;
}


SK_IAudioMeterInformation
SK_WASAPI_AudioSession::getMeterInfo (void)
{
  return
    device_->control_.meter;
}

SK_IAudioEndpointVolume
SK_WASAPI_AudioSession::getEndpointVolume (void)
{
  return
    device_->control_.volume;
}

SK_IAudioLoudness
SK_WASAPI_AudioSession::getLoudness (void)
{
  return
    device_->control_.loudness;
}

SK_IAudioAutoGainControl
SK_WASAPI_AudioSession::getAutoGainControl (void)
{
  return
    device_->control_.auto_gain;
}

SK_WASAPI_SessionManager &
SK_WASAPI_GetSessionManager (void);

HRESULT
STDMETHODCALLTYPE
SK_MMDev_Endpoint::OnSessionCreated (IAudioSessionControl *pNewSession)
{
  if (pNewSession)
  {
    pNewSession->AddRef ();

    SK_IAudioSessionControl2                                             pSessionCtl2;
    if (SUCCEEDED (pNewSession->QueryInterface <IAudioSessionControl2> (&pSessionCtl2.p)))
    {
      AudioSessionState        state = AudioSessionStateExpired;
      pSessionCtl2->GetState (&state);

      DWORD dwProcess = 0;
      if (SUCCEEDED (pSessionCtl2->GetProcessId (&dwProcess)) && state != AudioSessionStateExpired)
      { 
        auto* pSession =
          new SK_WASAPI_AudioSession (pSessionCtl2, this, session_manager_);

        session_manager_->AddSession (pSession, state);
      }
    }
  }

  return S_OK;
}

size_t
SK_WASAPI_EndPointManager::getNumRenderEndpoints (DWORD dwState)
{
  if (dwState == DEVICE_STATEMASK_ALL)
    return render_devices_.size ();

  size_t count = 0;

  for ( UINT i = 0 ; i < render_devices_.size () ; ++i )
  {
    render_devices_ [i].device_->GetState (&render_devices_ [i].state_);

    if ((render_devices_ [i].state_ & dwState) != 0x0)
      ++count;
  }

  return count;
}

SK_MMDev_Endpoint&
SK_WASAPI_EndPointManager::getRenderEndpoint (UINT idx)
{
  static SK_MMDev_Endpoint invalid = {}; return idx < render_devices_.size  () ? render_devices_  [idx] : invalid;
}

size_t
SK_WASAPI_EndPointManager::getNumCaptureEndpoints (DWORD dwState)
{
  if (dwState == DEVICE_STATEMASK_ALL)
    return capture_devices_.size ();

  size_t count = 0;

  for ( UINT i = 0 ; i < capture_devices_.size () ; ++i )
  {
    capture_devices_ [i].device_->GetState (&render_devices_ [i].state_);

    if ((capture_devices_ [i].state_ & dwState) != 0x0)
      ++count;
  }

  return count;
}

SK_MMDev_Endpoint&
SK_WASAPI_EndPointManager::getCaptureEndpoint     (UINT idx)
{
  static SK_MMDev_Endpoint invalid = {}; return idx < capture_devices_.size () ? capture_devices_ [idx] : invalid;
}

HRESULT
STDMETHODCALLTYPE
SK_WASAPI_EndPointManager::OnDeviceStateChanged (_In_ LPCWSTR pwstrDeviceId, _In_ DWORD dwNewState)
{
  std::ignore = pwstrDeviceId;
  std::ignore = dwNewState;

  resetSessionManager ();

  static auto &rb =
    SK_GetCurrentRenderBackend ();

  if (StrStrW (pwstrDeviceId, rb.displays [rb.active_display].audio.paired_device) && dwNewState == DEVICE_STATE_ACTIVE)
    rb.routeAudioForDisplay (&rb.displays [rb.active_display], true);

  return S_OK;
}

HRESULT
STDMETHODCALLTYPE
SK_WASAPI_EndPointManager::OnDeviceAdded (_In_ LPCWSTR pwstrDeviceId)
{
  std::ignore = pwstrDeviceId;

  resetSessionManager ();

  static auto &rb =
    SK_GetCurrentRenderBackend ();

  if (StrStrW (pwstrDeviceId, rb.displays [rb.active_display].audio.paired_device))
    rb.routeAudioForDisplay (&rb.displays [rb.active_display], true);

  return S_OK;
}

HRESULT
STDMETHODCALLTYPE
SK_WASAPI_EndPointManager::OnDeviceRemoved (_In_ LPCWSTR pwstrDeviceId)
{
  std::ignore = pwstrDeviceId;

  resetSessionManager ();

  //SK_ImGui_Warning (pwstrDeviceId);

  return S_OK;
}

HRESULT
STDMETHODCALLTYPE
SK_WASAPI_EndPointManager::OnDefaultDeviceChanged (_In_ EDataFlow flow, _In_ ERole role, _In_ LPCWSTR pwstrDefaultDeviceId)
 {
   std::ignore = role;
   std::ignore = pwstrDefaultDeviceId;

   if (flow == eAll || flow == eRender)
   {
     //SK_ImGui_Warning (pwstrDefaultDeviceId);
     //resetSessionManager ();
   }

   return S_OK;
 }

HRESULT
STDMETHODCALLTYPE
SK_WASAPI_EndPointManager::OnPropertyValueChanged (_In_ LPCWSTR pwstrDeviceId, _In_ const PROPERTYKEY key)
{
  std::ignore = pwstrDeviceId;
  std::ignore = key;

  //SK_ImGui_Warning (pwstrDeviceId);
  //resetSessionManager ();

  return S_OK;
}