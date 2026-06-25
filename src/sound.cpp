// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
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
#include <deque>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"  WASAPI  "

const PROPERTYKEY PKEY_AudioEngine_DeviceFormat = { { 0xf19f064d, 0x082c, 0x4e27,{ 0xbc, 0x73, 0x68, 0x82, 0xa1, 0xbb, 0x8e, 0x4c, } }, 0 };
const PROPERTYKEY PKEY_AudioEndpoint_GUID       = { { 0x1da5d803, 0xd492, 0x4edd,{ 0x8c, 0x23, 0xe0, 0xc0, 0xff, 0xee, 0x7f, 0x0e, } }, 4 };

const IID IID_IAudioClient3 = __uuidof (IAudioClient3);

SK_IAudioClient3
__stdcall
SK_WASAPI_GetAudioClient (SK_IMMDevice pDevice, bool uncached)
{
  if (config.compatibility.using_wine)
    return nullptr;

  static bool             unsupported   = false;
  static SK_IAudioClient3 pCachedClient = nullptr;
  static DWORD            dwLastUpdate  = 0;

  if (unsupported)
    return nullptr;

  // TODO: Stash this in the session manager SK already has, and keep it
  //         around persistently
  if (SK::ControlPanel::current_time > dwLastUpdate + 2500UL || uncached)
  {
    dwLastUpdate = SK::ControlPanel::current_time;

    HRESULT hr = S_OK; 

    try
    {
      if (pDevice == nullptr)
      {
        SK_IMMDeviceEnumerator pDevEnum = nullptr;

        ThrowIfFailed (
          SK_CoCreateInstance (__uuidof (MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS (&pDevEnum.p)));

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

      ThrowIfFailed (hr = pDevice->Activate (IID_IAudioClient3, CLSCTX_ALL, nullptr, IID_PPV_ARGS_Helper (&pAudioClient.p)));

      pCachedClient =
        pAudioClient;
    }

    catch (const std::exception& e)
    {
      SK_LOG0 ( ( L"%ws (...) Failed: %hs", __FUNCTIONW__, e.what ()
                ),L"  WASAPI  " );

      if (hr == E_NOINTERFACE)
        unsupported = true;

      // Delay future attempts on failure
      dwLastUpdate += 5000UL;
      pCachedClient = nullptr;

      return nullptr;
    }
  }

  return
    pCachedClient;
}

using CPROPVARIANT                                        = const PROPVARIANT;
using ISpatialAudioClient__ActivateSpatialAudioStream_pfn = HRESULT (STDMETHODCALLTYPE *)
    ( ISpatialAudioClient *This,
        _In_ CPROPVARIANT *activationParams,
        _In_       REFIID  riid,
_COM_Outptr_       void  **stream );

ISpatialAudioClient__\
ActivateSpatialAudioStream_pfn
ActivateSpatialAudioStream_Original = nullptr;

bool             ProcessUsingSpatialAudio=false;
bool SK_WASAPI_IsProcessUsingSpatialAudio (void)
{ return         ProcessUsingSpatialAudio;     }

HRESULT
STDMETHODCALLTYPE
ISpatialAudioClient__ActivateSpatialAudioStream_Detour (
ISpatialAudioClient *This,
CPROPVARIANT        *activationParams,
REFIID               riid,
void               **stream )
{
  SK_LOG_FIRST_CALL

  HRESULT hr =
    ActivateSpatialAudioStream_Original (
      This, activationParams,
        riid, stream
  );

  if (SUCCEEDED (hr))
  {
    ProcessUsingSpatialAudio = true;
  }

  return hr;
}

SK_ISpatialAudioClient
__stdcall
SK_WASAPI_GetSpatialAudioClient (SK_IMMDevice pDevice, bool uncached)
{
  if (config.compatibility.using_wine)
    return nullptr;

  static bool                   unsupported   = false;
  static SK_ISpatialAudioClient pCachedClient = nullptr;
  static DWORD                  dwLastUpdate  = 0;

  if (unsupported)
    return nullptr;

  // TODO: Stash this in the session manager SK already has, and keep it
  //         around persistently
  if (SK::ControlPanel::current_time > dwLastUpdate + 2500UL || uncached)
  {
    dwLastUpdate = SK::ControlPanel::current_time;

    HRESULT hr = S_OK; 

    try
    {
      if (pDevice == nullptr)
      {
        SK_IMMDeviceEnumerator pDevEnum = nullptr;

        ThrowIfFailed (
          SK_CoCreateInstance (__uuidof (MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS (&pDevEnum.p)));

        if (pDevEnum == nullptr)
          return nullptr;

        ThrowIfFailed (
          pDevEnum->GetDefaultAudioEndpoint (eRender,
                                               eConsole,
                                                 &pDevice));
      }

      if (pDevice == nullptr)
        return nullptr;

      SK_ComPtr                     <ISpatialAudioClient>                        pSpatialAudioClient; ThrowIfFailed ( hr =
        pDevice->Activate (__uuidof (ISpatialAudioClient), CLSCTX_INPROC_SERVER,
                                                  nullptr, IID_PPV_ARGS_Helper (&pSpatialAudioClient.p)) );

      pCachedClient =
        pSpatialAudioClient;
    }

    catch (const std::exception& e)
    {
      SK_LOG0 ( ( L"%ws (...) Failed: %hs", __FUNCTIONW__, e.what ()
                ),L"  WASAPI  " );

      if (hr == E_NOINTERFACE)
        unsupported = true;

      // Delay future attempts on failure
      dwLastUpdate += 5000UL;
      pCachedClient = nullptr;

      return nullptr;
    }
  }

  if (pCachedClient == nullptr)
    return nullptr;

  AudioObjectType                                obj_type_mask = AudioObjectType_None;
  pCachedClient->GetNativeStaticObjectTypeMask (&obj_type_mask);

  if (obj_type_mask != AudioObjectType_None)
  {
    if ((obj_type_mask & AudioObjectType_Dynamic)          != 0) SK_LOGi1 (L"Spatial Channel: Dynamic");
    if ((obj_type_mask & AudioObjectType_FrontLeft)        != 0) SK_LOGi1 (L"Spatial Channel: Front Left");
    if ((obj_type_mask & AudioObjectType_FrontRight)       != 0) SK_LOGi1 (L"Spatial Channel: Front Right");
    if ((obj_type_mask & AudioObjectType_FrontCenter)      != 0) SK_LOGi1 (L"Spatial Channel: Front Center");
    if ((obj_type_mask & AudioObjectType_LowFrequency)     != 0) SK_LOGi1 (L"Spatial Channel: Low Frequency");
    if ((obj_type_mask & AudioObjectType_SideLeft)         != 0) SK_LOGi1 (L"Spatial Channel: Side Left");
    if ((obj_type_mask & AudioObjectType_SideRight)        != 0) SK_LOGi1 (L"Spatial Channel: Side Right");
    if ((obj_type_mask & AudioObjectType_BackLeft)         != 0) SK_LOGi1 (L"Spatial Channel: Back Left");
    if ((obj_type_mask & AudioObjectType_BackRight)        != 0) SK_LOGi1 (L"Spatial Channel: Back Right");
    if ((obj_type_mask & AudioObjectType_TopFrontLeft)     != 0) SK_LOGi1 (L"Spatial Channel: Top Front Left");
    if ((obj_type_mask & AudioObjectType_TopFrontRight)    != 0) SK_LOGi1 (L"Spatial Channel: Top Front Right");
    if ((obj_type_mask & AudioObjectType_TopBackLeft)      != 0) SK_LOGi1 (L"Spatial Channel: Top Back Left");
    if ((obj_type_mask & AudioObjectType_TopBackRight)     != 0) SK_LOGi1 (L"Spatial Channel: Top Back Right");
    if ((obj_type_mask & AudioObjectType_BottomFrontLeft)  != 0) SK_LOGi1 (L"Spatial Channel: Bottom Front Left");
    if ((obj_type_mask & AudioObjectType_BottomFrontRight) != 0) SK_LOGi1 (L"Spatial Channel: Bottom Front Right");
    if ((obj_type_mask & AudioObjectType_BottomBackLeft)   != 0) SK_LOGi1 (L"Spatial Channel: Bottom Back Left");
    if ((obj_type_mask & AudioObjectType_BottomBackRight)  != 0) SK_LOGi1 (L"Spatial Channel: Bottom Back Right");
    if ((obj_type_mask & AudioObjectType_BackCenter)       != 0) SK_LOGi1 (L"Spatial Channel: Back Center");
    if ((obj_type_mask & AudioObjectType_StereoLeft)       != 0) SK_LOGi1 (L"Spatial Channel: Stereo Left");
    if ((obj_type_mask & AudioObjectType_StereoRight)      != 0) SK_LOGi1 (L"Spatial Channel: Stereo Right");

    UINT32                                                   maxDynamicObjCount = 0;
    if (SUCCEEDED (pCachedClient->GetMaxDynamicObjectCount (&maxDynamicObjCount)))
    {
      SK_LOGi1 (L"Spatial Audio Client Supports up to %lu dynamic objects.");

      SK_RunOnce (
        // ISpatialAudioClient
        // -------------------
        //  0  QueryInterface
        //  1  AddRef
        //  2  Release
        //  3  GetStaticObjectPosition
        //  4  GetNativeStaticObjectTypeMask
        //  5  GetMaxDynamicObjectCount
        //  6  GetSupportedAudioObjectFormatEnumerator
        //  7  GetMaxFrameCount
        //  8  IsAudioObjectFormatSupported
        //  9  IsSpatialAudioStreamAvailable
        // 10  ActivateSpatialAudioStream

        void** vftable = *(void***)pCachedClient.p;

        SK_CreateFuncHook ( L"ISpatialAudioClient::ActivateSpatialAudioStream",
                              vftable [10],
                              ISpatialAudioClient__ActivateSpatialAudioStream_Detour,
          static_cast_p2p <void> (                &ActivateSpatialAudioStream_Original) );
        SK_EnableHook (       vftable [10]                                              );
      );
    }
  }

  return
    pCachedClient;
}

SK_IAudioMeterInformation
__stdcall
SK_WASAPI_GetAudioMeterInfo (SK_IMMDevice pDevice)
{
  if (config.compatibility.using_wine)
    return nullptr;

  SK_IAudioMeterInformation pMeterInfo = nullptr;

  try
  {
    if (pDevice == nullptr)
    {
      SK_IMMDeviceEnumerator pDevEnum = nullptr;

      ThrowIfFailed (
        SK_CoCreateInstance (__uuidof (MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS (&pDevEnum.p)));

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

SK_IMMDevice          SK_MMDevAPI_DefaultDevice;
SK_IMMDevice          SK_MMDevAPI_VolumeDevice;
SK_ISimpleAudioVolume SK_VolumeControl;

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
  if (config.compatibility.using_wine)
    return;

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
      SK_CoCreateInstance (__uuidof (MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS (&pDevEnum.p)));

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
SK_WASAPI_GetAudioSessionControl ( EDataFlow     data_flow     = eRender,
                                   ERole         endpoint_role = eConsole,
                                   SK_IMMDevice  pDevice       = nullptr,
                                   DWORD         proc_id       = GetCurrentProcessId (),
                                   IMMDevice**   ppDevice      = nullptr )
{
  if (config.compatibility.using_wine)
    return nullptr;

  static BOOL bCanWineCountCorrectly = SK_NoPreference;

  // Prevent incorrectly implemented Windows Audio APIs from leaking memory.
  if (! bCanWineCountCorrectly)
    return nullptr;

  SK_IMMDeviceEnumerator     pDevEnum;
  SK_IAudioSessionEnumerator pSessionEnum;
  SK_IAudioSessionManager2   pSessionMgr2;
  SK_IAudioSessionControl2   pSessionCtl2;

  int num_sessions = 0;

  try {
    if (pDevice == nullptr)
    {
      ThrowIfFailed (
        SK_CoCreateInstance (__uuidof (MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS (&pDevEnum.p)));

      ThrowIfFailed (
        pDevEnum->GetDefaultAudioEndpoint ( data_flow,
                                              endpoint_role,
                                                &pDevice.p ));

      if (ppDevice != nullptr) {
         *ppDevice  = pDevice;
                      pDevice.p->AddRef ();
      }
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

  //
  // Sanity check for Wine -- if they ever correctly implement
  //   the Windows Core Audio APIs, audio functions in Special K
  //     will automatically become available.
  //
  if (config.compatibility.using_wine)
  {
    // It's highly unlikely that any Linux system will have
    //   128 audio sessions... if they do, that's too many to
    //     sensibly manage with SK's mixing UI anyway.
    if (num_sessions > 128)
    {
      // Wine indicates success for pSessionEnum->GetCount (...),
      //   but the returned count is uninitialized garbage.
      bCanWineCountCorrectly = FALSE;
      return nullptr;
    }
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

      pSessionCtl2 = nullptr;

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
  if (config.compatibility.using_wine)
    return nullptr;

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
  if (config.compatibility.using_wine)
    return nullptr;

  SK_IMMDevice pVolumeDevice;

  SK_IAudioSessionControl pSessionCtl =
    SK_WASAPI_GetAudioSessionControl (eRender, eConsole, pDevice, proc_id, &pVolumeDevice.p);

  if (pSessionCtl != nullptr)
  {
    //SK_VolumeDevice = pDevice;
    SK_ISimpleAudioVolume                                      pSimpleAudioVolume;
    if (SUCCEEDED (pSessionCtl->QueryInterface (IID_PPV_ARGS (&pSimpleAudioVolume.p))))
    {
      SK_MMDevAPI_DefaultDevice = pVolumeDevice;
      SK_MMDevAPI_VolumeDevice  = pVolumeDevice;
      return                                                   pSimpleAudioVolume;
    }
  }

  return nullptr;
}

SK_IAudioEndpointVolume
__stdcall
SK_MMDev_GetEndpointVolumeControl (SK_IMMDevice pDevice)
{
  if (config.compatibility.using_wine)
    return nullptr;

  SK_IAudioEndpointVolume pEndVol = nullptr;

  try {
    if (pDevice == nullptr)
    {
      SK_IMMDeviceEnumerator pDevEnum = nullptr;

      ThrowIfFailed (
        SK_CoCreateInstance (__uuidof (MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS (&pDevEnum.p)));

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
  if (config.compatibility.using_wine)
    return nullptr;

  SK_IAudioLoudness pLoudness = nullptr;

  try
  {
    if (pDevice == nullptr)
    {
      SK_IMMDeviceEnumerator pDevEnum = nullptr;

      ThrowIfFailed (
        SK_CoCreateInstance (__uuidof (MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS (&pDevEnum.p)));

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
  if (config.compatibility.using_wine)
    return nullptr;

  SK_IAudioAutoGainControl pAutoGain = nullptr;

  try
  {
    if (pDevice == nullptr)
    {
      SK_IMMDeviceEnumerator pDevEnum = nullptr;

      ThrowIfFailed (
        SK_CoCreateInstance (__uuidof (MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS (&pDevEnum.p)));

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

    pAutoGain = nullptr;
  }

  return pAutoGain;
}

#include <dsound.h>

// Misnomer, since this uses DirectSound instead (much simpler =P)
const char*
__stdcall
SK_WASAPI_GetChannelName (int channel_idx)
{
  if (config.compatibility.using_wine)
    return nullptr;

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
        channel_names.try_emplace (0, "Headphone Left");
        channel_names.try_emplace (1, "Headphone Right");
        break;

      case DSSPEAKER_MONO:
      //case KSAUDIO_SPEAKER_MONO:
        channel_names.try_emplace (0, "Center");
        break;

      case DSSPEAKER_STEREO:
      //case KSAUDIO_SPEAKER_STEREO:
        channel_names.try_emplace (0, "Front Left");
        channel_names.try_emplace (1, "Front Right");
        break;

       case DSSPEAKER_QUAD:
      //case KSAUDIO_SPEAKER_QUAD:
        channel_names.try_emplace (0, "Front Left");
        channel_names.try_emplace (1, "Front Right");
        channel_names.try_emplace (2, "Back Left");
        channel_names.try_emplace (3, "Back Right");
        break;

      case DSSPEAKER_SURROUND:
      //case KSAUDIO_SPEAKER_SURROUND:
        channel_names.try_emplace (0, "Front Left");
        channel_names.try_emplace (1, "Front Right");
        channel_names.try_emplace (2, "Front Center");
        channel_names.try_emplace (3, "Back Center");
        break;

      case DSSPEAKER_5POINT1:
      //case KSAUDIO_SPEAKER_5POINT1:
        channel_names.try_emplace (0, "Front Left");
        channel_names.try_emplace (1, "Front Right");
        channel_names.try_emplace (2, "Center");
        channel_names.try_emplace (3, "Low Frequency Emitter");
        channel_names.try_emplace (4, "Back Left");
        channel_names.try_emplace (5, "Back Right");
        break;

      case DSSPEAKER_5POINT1_SURROUND:
      //case KSAUDIO_SPEAKER_5POINT1_SURROUND:
        channel_names.try_emplace (0, "Front Left");
        channel_names.try_emplace (1, "Front Right");
        channel_names.try_emplace (2, "Center");
        channel_names.try_emplace (3, "Low Frequency Emitter");
        channel_names.try_emplace (4, "Side Left");
        channel_names.try_emplace (5, "Side Right");
        break;

      case DSSPEAKER_7POINT1:
      //case KSAUDIO_SPEAKER_7POINT1:
        channel_names.try_emplace (0, "Front Left");
        channel_names.try_emplace (1, "Front Right");
        channel_names.try_emplace (2, "Center");
        channel_names.try_emplace (3, "Low Frequency Emitter");
        channel_names.try_emplace (4, "Back Left");
        channel_names.try_emplace (5, "Back Right");
        channel_names.try_emplace (6, "Front Left of Center");
        channel_names.try_emplace (7, "Front Right of Center");
        break;

      case DSSPEAKER_7POINT1_SURROUND:
      //case KSAUDIO_SPEAKER_7POINT1_SURROUND:
        channel_names.try_emplace (0, "Front Left");
        channel_names.try_emplace (1, "Front Right");
        channel_names.try_emplace (2, "Center");
        channel_names.try_emplace (3, "Low Frequency Emitter");
        channel_names.try_emplace (4, "Back Left");
        channel_names.try_emplace (5, "Back Right");
        channel_names.try_emplace (6, "Side Left");
        channel_names.try_emplace (7, "Side Right");
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

           channel_names.try_emplace (channel_idx, szChannelOrdinal);
    return channel_names             [channel_idx].c_str ();
  }
}

// Rate-limit failures
static DWORD dwLastMuteCheck = 0;

void
__stdcall
SK_SetGameMute (bool bMute)
{
  if (config.compatibility.using_wine)
    return;

  if (dwLastMuteCheck < SK::ControlPanel::current_time - 750UL && (SK_VolumeControl == nullptr || SK_MMDevAPI_VolumeDevice != SK_MMDevAPI_DefaultDevice))
  {   dwLastMuteCheck = SK::ControlPanel::current_time;
    SK_VolumeControl = SK_WASAPI_GetVolumeControl (GetCurrentProcessId ());
  }

  SK_ComPtr<ISimpleAudioVolume> pVolume = SK_VolumeControl;

  if (             pVolume != nullptr)
  { if (SUCCEEDED (pVolume->SetMute (bMute, nullptr)))
    {
      SK_ImGui_CreateNotification (
        "Sound.Mute", SK_ImGui_Toast::Info,
          bMute ? "Game has been muted"
                : "Game has been unmuted",
          nullptr, 3333UL, SK_ImGui_Toast::UseDuration |
                           SK_ImGui_Toast::ShowCaption |
                           SK_ImGui_Toast::ShowNewest
      );
    }
  }
}

BOOL
__stdcall
SK_IsGameMuted (void)
{
  if (config.compatibility.using_wine)
    return FALSE;

  if (dwLastMuteCheck < SK::ControlPanel::current_time - 750UL && (SK_VolumeControl == nullptr || SK_MMDevAPI_VolumeDevice != SK_MMDevAPI_DefaultDevice))
  {   dwLastMuteCheck = SK::ControlPanel::current_time;
      SK_VolumeControl = SK_WASAPI_GetVolumeControl (GetCurrentProcessId ());
  }

  BOOL bMuted = FALSE;

  SK_ComPtr<ISimpleAudioVolume> pVolume = SK_VolumeControl;

  if ( pVolume != nullptr && SUCCEEDED (
       pVolume->GetMute ( &bMuted )    )
     ) return              bMuted;

  // Might not be initialized yet, only process this assertion
  //   if pVolume is an actual COM interface pointer.
  if ( pVolume != nullptr )
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

  SK_VolumeControl = pVolumeCtl;

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
        if (SK_VolumeControl != nullptr && var->getValuePointer () == &volume)
        {
          volume = *(float *)val;

          volume =
            std::clamp (volume, 0.0f, 100.0f);

          SK_ImGui_CreateNotification (
            "Sound.Volume", SK_ImGui_Toast::Info,
              SK_FormatString ("Game Volume: %1.0f%%", volume).c_str (),
              nullptr, 3333UL, SK_ImGui_Toast::UseDuration |
                               SK_ImGui_Toast::ShowCaption |
                               SK_ImGui_Toast::ShowNewest
          );

          SK_VolumeControl->SetMasterVolume (volume / 100.0f, nullptr);

          if (volume != 0.0f)
          {
            if (SK_IsGameMuted ())
                SK_SetGameMute (false);
          }
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

  if (cmd != nullptr)
  {
    cmd->AddVariable ("Sound.Volume", SK_CreateVar (SK_IVariable::Float,   &volume, &sound_control));
    cmd->AddVariable ("Sound.Mute",   SK_CreateVar (SK_IVariable::Boolean, &mute,   &sound_control));
  }

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

  if (! custom_name_)
  {
    app_name_ =
      SK_WideCharToUTF8 (NewDisplayName);
  }

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

  const SK_RenderBackend_V2 &rb =
    SK_GetCurrentRenderBackend ();

  if (StrStrW (pwstrDeviceId, rb.displays [rb.active_display].audio.paired_device) && dwNewState == DEVICE_STATE_ACTIVE)
    rb.routeAudioForDisplay (&rb.displays [rb.active_display], true);

  if (dwNewState != DEVICE_STATE_ACTIVE)
  {
    SK_MMDevAPI_DefaultDevice = nullptr;
  }

  return S_OK;
}

HRESULT
STDMETHODCALLTYPE
SK_WASAPI_EndPointManager::OnDeviceAdded (_In_ LPCWSTR pwstrDeviceId)
{
  std::ignore = pwstrDeviceId;

  resetSessionManager ();

  const SK_RenderBackend_V2 &rb =
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

    SK_IMMDevice pVolumeDevice;

    SK_IAudioSessionControl pSessionCtl =
      SK_WASAPI_GetAudioSessionControl (eRender, eConsole, nullptr, GetCurrentProcessId (), &pVolumeDevice.p);

    if (pSessionCtl != nullptr)
    {
      SK_MMDevAPI_DefaultDevice = pVolumeDevice;
    }
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

SK_IAudioMeterInformation
SK_WASAPI_SessionManager::getMeterInfo (void)
{
  static SK_ComPtr <IMMDeviceEnumerator>
      pDevEnum;
  if (pDevEnum == nullptr)
  {

    if (FAILED (SK_CoCreateInstance (__uuidof (MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS (&pDevEnum.p))))
      return nullptr;
  }

  // Most game audio a user will not want to hear while a game is in the
  //   background will pass through eConsole.
  //
  //   eCommunication will be headset stuff and that's something a user is not
  //     going to appreciate having muted :) Consider overloading this function
  //       to allow independent control.
  //
  SK_ComPtr <IMMDevice> pDefaultDevice;
  if ( FAILED (
         pDevEnum->GetDefaultAudioEndpoint ( eRender,
                                               eMultimedia,
                                                 &pDefaultDevice )
              )
     ) return nullptr;

  return
    SK_WASAPI_GetAudioMeterInfo (pDefaultDevice);
}

void
SK_WASAPI_EndPointManager::Activate (void)
{
  std::scoped_lock <SK_Thread_HybridSpinlock> lock0 (activation_lock_);

  try
  {
    SK_ComPtr <IMMDeviceEnumerator> pDevEnum;

    ThrowIfFailed (
      SK_CoCreateInstance (__uuidof (MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS (&pDevEnum.p)));

    SK_RunOnce (
      pDevEnum->RegisterEndpointNotificationCallback (this)
    );

    SK_ComPtr <IMMDeviceCollection>                                dev_collection;
    ThrowIfFailed (
      pDevEnum->EnumAudioEndpoints (eRender, DEVICE_STATE_ACTIVE, &dev_collection.p));

    if (dev_collection.p != nullptr)
    {
      UINT                         uiDevices = 0;
      ThrowIfFailed (
        dev_collection->GetCount (&uiDevices));

      for ( UINT i = 0 ; i < uiDevices ; ++i )
      {
        SK_MMDev_Endpoint endpoint;

        SK_ComPtr <IMMDevice>       pDevice;
        ThrowIfFailed (
          dev_collection->Item (i, &pDevice.p));

        if (pDevice.p != nullptr)
        {
          wchar_t*           wszId = nullptr;
          ThrowIfFailed (
            pDevice->GetId (&wszId));

          auto _FindRenderDevice = [&](const wchar_t *device_id) -> SK_MMDev_Endpoint *
          {
            for ( auto& device : render_devices_ )
            {
              if (device.id_._Equal (device_id))
                return &device;
            }

            return nullptr;
          };

          if (wszId != nullptr && !_FindRenderDevice (wszId))
          {
            SK_ComPtr                 <IPropertyStore>             props;
            if (SUCCEEDED (pDevice->OpenPropertyStore (STGM_READ, &props.p)))
            {
              if (props.p != nullptr)
              {
                PROPVARIANT       propvarName;
                PropVariantInit (&propvarName);

                if (SUCCEEDED (props->GetValue (PKEY_Device_FriendlyName, &propvarName)))
                {
                  endpoint.flow_   = eRender;
                  endpoint.id_     = wszId;
                  endpoint.name_   = SK_WideCharToUTF8 (propvarName.pwszVal);
                  endpoint.device_ = pDevice;
                }

                PropVariantClear (&propvarName);

                if (SUCCEEDED (pDevice->GetState (&endpoint.state_)))
                {
                  if (FAILED (pDevice->Activate (
                      __uuidof (IAudioSessionManager2),
                        CLSCTX_ALL,
                          nullptr,
                            reinterpret_cast <void **>(&endpoint.control_.sessions)
                     )       )                  )
                  {
                    CoTaskMemFree (wszId);
                    return;
                  }

                  endpoint.control_.meter.Attach (
                    SK_WASAPI_GetAudioMeterInfo (pDevice).Detach ()
                  );

                  endpoint.control_.volume.Attach         (SK_MMDev_GetEndpointVolumeControl (pDevice).Detach ());
                  endpoint.control_.auto_gain.Attach      (SK_MMDev_GetAutoGainControl       (pDevice).Detach ());
                  endpoint.control_.loudness.    Attach   (SK_MMDev_GetLoudness              (pDevice).Detach ());
                  endpoint.control_.audio_client.Attach   (SK_WASAPI_GetAudioClient          (pDevice).Detach ());
                  endpoint.control_.spatial_client.Attach (SK_WASAPI_GetSpatialAudioClient   (pDevice).Detach ());

                  endpoint.control_.sessions->RegisterSessionNotification (&endpoint);

                  render_devices_.emplace_back (endpoint);
                }
              }
            }
          }

          if (wszId != nullptr)
            CoTaskMemFree (wszId);
        }
      }
    }
    //pDevEnum->EnumAudioEndpoints (eCapture, DEVICE_STATEMASK_ALL, &dev_collection.p);
  }

  catch (const std::exception& e)
  {
    SK_LOG0 ( ( L"%ws (...) Failed "
                L"During Endpoint Enumeration : %hs", __FUNCTIONW__, e.what ()
              ),L"  WASAPI  " );

    return;
  }
}

void
SK_WASAPI_SessionManager::AddSession (SK_WASAPI_AudioSession *pSession, AudioSessionState state)
{
  const bool new_session =
    sessions_.emplace (pSession).second;

  SK_ReleaseAssert (new_session);

  SetSessionState (pSession, state);
}

void
SK_WASAPI_SessionManager::RemoveSession (SK_WASAPI_AudioSession *pSession)
{
  if (! pSession)
    return;

  SetSessionState (pSession, AudioSessionStateExpired);

  if (sessions_.count (pSession))
  {
    sessions_.erase   (pSession);
    pSession->Release ();
  }
}







#include <concurrent_unordered_map.h>

class SK_IVirtualMMDevice;
class SK_IVirtualMMDeviceCollection;
class SK_IVirtualMMDeviceEnumerator;

class SK_VirtualAudio_Registrar
{
friend class SK_IVirtualMMDeviceCollection;
friend class SK_IVirtualMMDeviceEnumerator;
public:
  void addNotificationClient (IMMNotificationClient* pClient)
  {
    if (pClient != nullptr)
    {
      if ( clients_.count (pClient) == 0 ||
           clients_       [pClient] == false )
           clients_       [pClient]  = true;
    }
  }

  void removeNotificationClient (IMMNotificationClient* pClient)
  {
    if (pClient != nullptr)
    {
      if ( clients_.count (pClient) != 0 &&
           clients_       [pClient] == true )
           clients_       [pClient]  = false;
    }
  }

  int broadcastDeviceAdded (LPCWSTR pwstrDeviceId)
  {
    int notified = 0;

    for (auto& [client, active] : clients_)
    {
      if (active)
      {
        SK_LOGi0 (L"Broadcasting Device Added: %ws", pwstrDeviceId);

        client->OnDeviceAdded (pwstrDeviceId);
        ++notified;
      }
    }

    return notified;
  }

  int broadcastDeviceRemoved (LPCWSTR pwstrDeviceId)
  {
    int notified = 0;

    for (auto& [client, active] : clients_)
    {
      if (active)
      {
        client->OnDeviceRemoved (pwstrDeviceId);
        ++notified;
      }
    }

    return notified;
  }

  void addDevice (SK_IVirtualMMDevice* pDevice)
  {
    ((IMMDevice *)pDevice)->AddRef ();
    virtual_devices_.push_back (pDevice);

    wchar_t*                        wszDevicePath = nullptr;
    ((IMMDevice *)pDevice)->GetId (&wszDevicePath);

    if (wszDevicePath != nullptr)
    {
      broadcastDeviceAdded (wszDevicePath);
      CoTaskMemFree        (wszDevicePath);
    }
  }

protected:
  Concurrency::concurrent_vector        <SK_ComPtr <SK_IVirtualMMDevice>>         virtual_devices_;
  Concurrency::concurrent_unordered_map <SK_ComPtr <IMMNotificationClient>, bool> clients_;
};

SK_LazyGlobal <SK_VirtualAudio_Registrar> virtual_audio_registrar;

#include <devpkey.h>

class SK_IVirtualPropertyStore : public IPropertyStore
{
public:
  SK_IVirtualPropertyStore (SK_IVirtualMMDevice* pDevice);
  SK_IVirtualPropertyStore (IPropertyStore* real_property_store) : pReal (real_property_store), refs_ (1) {}

  virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
  {
    HRESULT hr = E_NOTIMPL;

    if (pReal == nullptr)
    {
      if (ppvObject != nullptr)
      {
        if ( riid == __uuidof (IUnknown) ||
             riid == __uuidof (IPropertyStore) )
        {
          hr = S_OK;
          *ppvObject = this;
        }

        else
        {
          static concurrency::concurrent_unordered_set <std::wstring> reported_guids;

          wchar_t                wszGUID [41] = { };
          StringFromGUID2 (riid, wszGUID, 40);

          const bool once =
            reported_guids.count (wszGUID) > 0;

          if (! once)
          {
            reported_guids.insert (wszGUID);

            SK_LOG0 ( ( L"QueryInterface on virtual IPropertyStore for Mystery UUID: %s",
                            wszGUID ), L"VirtualSnd" );
          }
        }
      }
    }

    else
    {
      hr =
        pReal->QueryInterface (riid, ppvObject);
    }

    if (SUCCEEDED (hr))
      AddRef ();

    return hr;
  }

  virtual ULONG STDMETHODCALLTYPE AddRef (void)
  {
    InterlockedIncrement (&refs_);

    return refs_;
  }

  virtual ULONG STDMETHODCALLTYPE Release (void)
  {
    ULONG ref =
      InterlockedDecrement (&refs_);

    if (ref == 0)
      delete this;

    return ref;
  }

  virtual HRESULT STDMETHODCALLTYPE GetCount (__RPC__out DWORD *cProps)
  {
    if (pReal == nullptr)
    {
      if (cProps != nullptr)
      {
        *cProps =
          static_cast <DWORD> (virtual_properties_.size ());
      }

      return E_POINTER;
    }

    return
      pReal->GetCount (cProps);
  }

  virtual HRESULT STDMETHODCALLTYPE GetAt (DWORD iProp, __RPC__out PROPERTYKEY* pkey)
  {
    if (pReal == nullptr)
    {
      if (iProp < virtual_properties_.size ())
      {
        if (pkey != nullptr)
        {
          *pkey = virtual_properties_ [iProp].first;
          return S_OK;
        }

        return E_POINTER;
      }

      return E_INVALIDARG;
    }

    return
      pReal->GetAt (iProp, pkey);
  }

  virtual HRESULT STDMETHODCALLTYPE GetValue (__RPC__in REFPROPERTYKEY key, __RPC__out PROPVARIANT* pv)
  {
    SK_LOG_FIRST_CALL

    OLECHAR                     wszPropertyKeyName [128] = { };
    StringFromGUID2 (key.fmtid, wszPropertyKeyName, 127);

    HRESULT hr = E_INVALIDARG;

    struct haptics_format_s {
      union {
        WAVEFORMATEXTENSIBLE format;
        unsigned char        rawData [40] = {
          0xFE, 0xFF, 0x04, 0x00, 0x80, 0xBB, 0x00, 0x00, 0x00, 0xDC, 0x05, 0x00,
          0x08, 0x00, 0x10, 0x00, 0x16, 0x00, 0x10, 0x00, 0x33, 0x00, 0x00, 0x00,
          0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA,
          0x00, 0x38, 0x9B, 0x71
        };
      };
    };

    static constexpr haptics_format_s format;

#if 0
    const WAVEFORMATEXTENSIBLE* pWaveFormat =
      reinterpret_cast <const WAVEFORMATEXTENSIBLE *> (&format.format);

    SK_LOGi0 (
      L"Format: %u Hz, %u Channels, %d bits per-sample, %d valid bits per-sample, %u Samples per-block",
        pWaveFormat->Format.nSamplesPerSec, pWaveFormat->Format.nChannels,
        pWaveFormat->Format.wBitsPerSample, pWaveFormat->Samples.wValidBitsPerSample,
        pWaveFormat->Samples.wSamplesPerBlock );

    SK_LOGi0 (
      L"nAvgBytesPerSec: %u, nBlockAlign: %u, cbSize: %u",
        pWaveFormat->Format.nAvgBytesPerSec,
        pWaveFormat->Format.nBlockAlign,
        pWaveFormat->Format.cbSize
    );

    wchar_t                              wszChannels [33] = {};
    _itow_s (pWaveFormat->dwChannelMask, wszChannels, sizeof (wszChannels) / sizeof (wchar_t), 2);


    SK_LOGi0 (
      L"Channels: [%ws], Format Tag: %u, SubFormat: {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        wszChannels, pWaveFormat->Format.wFormatTag, // KSDATAFORMAT_SUBTYPE_PCM
                     pWaveFormat->SubFormat.Data1, pWaveFormat->SubFormat.Data2,
                     pWaveFormat->SubFormat.Data3, pWaveFormat->SubFormat.Data4 [0],
                     pWaveFormat->SubFormat.Data4 [1], pWaveFormat->SubFormat.Data4 [2],
                     pWaveFormat->SubFormat.Data4 [3], pWaveFormat->SubFormat.Data4 [4],
                     pWaveFormat->SubFormat.Data4 [5], pWaveFormat->SubFormat.Data4 [6],
                     pWaveFormat->SubFormat.Data4 [7] );
#endif

    if (pReal == nullptr)
    {
      for (auto& [virtual_key, virtual_value] : virtual_properties_)
      {
        if (IsEqualPropertyKey (virtual_key, key))
        {
          if (pv != nullptr)
          {
            SK_LOGi1 (
              L"SK_IVirtualPropertyStore::GetValue (...) - Returning Virtual Property for Key: %ws.%u",
              wszPropertyKeyName, key.pid
            );

            size_t size = 0;

            pv->vt = virtual_value.vt;
            hr     = S_OK;

            switch (pv->vt)
            {
              case VT_LPWSTR:
                size        = (wcslen (virtual_value.pwszVal) + 1) * sizeof (wchar_t);
                pv->pwszVal = (wchar_t *)CoTaskMemAlloc (size);
                memcpy (pv->pwszVal, virtual_value.pwszVal, size);

                SK_LOGi0 (L"SK_IVirtualPropertyStore::GetValue (...) - Returning Virtual String: %ws", pv->pwszVal);
                break;
              case VT_BLOB:
                size               = virtual_value.blob.cbSize;
                pv->blob.pBlobData = (BYTE *)CoTaskMemAlloc (size);
                memcpy (pv->blob.pBlobData, virtual_value.blob.pBlobData, size);
                break;
              case VT_CLSID:
                size      = sizeof (GUID);
                pv->puuid = (GUID *)CoTaskMemAlloc (size);
                memcpy (pv->puuid, virtual_value.puuid, size);
                break;
              default:
                hr = E_NOTIMPL;
                SK_ReleaseAssert (! L"Unsupported Variant Type in Virtual Property Store");
                break;
            }
          }

          else
            hr = E_POINTER;

          break;
        }
      }

      if (hr == E_INVALIDARG)
      {
        SK_LOGi0 (
          L"SK_IVirtualPropertyStore::GetValue (...) - Unrecognized Property Key: %ws.%u",
          wszPropertyKeyName, key.pid
        );
      }

      return hr;
    }

    SK_LOGi0 (
      L"SK_IVirtualPropertyStore::GetValue (...) called for key %ws.%u",
        wszPropertyKeyName, key.pid
    );

    hr =
      pReal->GetValue (key, pv);

    if (SUCCEEDED (hr))
    {
      if (key.fmtid == PKEY_AudioEngine_DeviceFormat.fmtid &&
          key.pid   == PKEY_AudioEngine_DeviceFormat.pid)
      {
        if (pv->blob.cbSize == 40)
        {
          //SK_ReleaseAssert (! memcmp (pWaveFormat, pv->blob.pBlobData, 40));
        }
      }
    }

    return hr;
  }

  virtual HRESULT STDMETHODCALLTYPE SetValue (__RPC__in REFPROPERTYKEY key, __RPC__in REFPROPVARIANT propvar)
  {
    if (pReal == nullptr)
    {
      SK_LOGi0 (L"SK_IVirtualPropertyStore::SetValue (...) - Unimplemented Stub Called!");

      return S_OK;
    }

    return
      pReal->SetValue (key, propvar);
  }

  virtual HRESULT STDMETHODCALLTYPE Commit (void)
  {
    if (pReal == nullptr)
    {
      SK_LOG_FIRST_CALL
      return S_OK;
    }

    return
      pReal->Commit ();
  }

protected:
           IPropertyStore* pReal;
  // Ordering is important, so avoid std::map
  std::vector <std::pair <PROPERTYKEY, PROPVARIANT>>
                           virtual_properties_;
  volatile ULONG           refs_;
};

#pragma warning (push)
#pragma warning (disable : 4100) // unreferenced formal parameter)
class SK_IVirtualAudioClient2;
class SK_IVirtualAudioRenderClient : public IAudioRenderClient
{
public:
  SK_IVirtualAudioRenderClient (SK_IVirtualAudioClient2* parent) : refs_ (1), parent_ (parent) {};

  virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
  {
    HRESULT hr = E_NOTIMPL;

    if (ppvObject != nullptr)
    {
      if ( riid == __uuidof (IUnknown) ||
           riid == __uuidof (IAudioRenderClient) )
      {
        hr = S_OK;
        *ppvObject = this;
      }

      else
      {
        static concurrency::concurrent_unordered_set <std::wstring> reported_guids;

        wchar_t                wszGUID [41] = { };
        StringFromGUID2 (riid, wszGUID, 40);

        const bool once =
          reported_guids.count (wszGUID) > 0;

        if (! once)
        {
          reported_guids.insert (wszGUID);

          SK_LOG0 ( ( L"QueryInterface on virtual IAudioRenderClient for Mystery UUID: %s",
                          wszGUID ), L"VirtualSnd" );
        }
      }
    }

    if (SUCCEEDED (hr))
      AddRef ();

    return hr;
  }

  virtual ULONG STDMETHODCALLTYPE AddRef (void)
  {
    InterlockedIncrement (&refs_);

    return refs_;
  }

  virtual ULONG STDMETHODCALLTYPE Release (void)
  {
    ULONG ref =
      InterlockedDecrement (&refs_);

    if (ref == 0)
      delete this;

    return ref;
  }

  virtual HRESULT STDMETHODCALLTYPE GetBuffer     (UINT32 NumFramesRequested, BYTE **ppData);
  virtual HRESULT STDMETHODCALLTYPE ReleaseBuffer (UINT32 NumFramesWritten, DWORD dwFlags);

protected:
  volatile ULONG                    refs_;
           SK_IVirtualAudioClient2* parent_;
           UINT32                   acquired_frames_ = 0;
};

class SK_IVirtualAudioSessionControl2 : public IAudioSessionControl2
{
public:
  SK_IVirtualAudioSessionControl2 (void) : refs_ (1) {};

  virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
  {
    HRESULT hr = E_NOTIMPL;

    if (ppvObject != nullptr)
    {
      if ( riid == __uuidof (IUnknown)             ||
           riid == __uuidof (IAudioSessionControl) ||
           riid == __uuidof (IAudioSessionControl2) )
      {
        hr = S_OK;
        *ppvObject = this;
      }

      else
      {
        static concurrency::concurrent_unordered_set <std::wstring> reported_guids;

        wchar_t                wszGUID [41] = { };
        StringFromGUID2 (riid, wszGUID, 40);

        const bool once =
          reported_guids.count (wszGUID) > 0;

        if (! once)
        {
          reported_guids.insert (wszGUID);

          SK_LOG0 ( ( L"QueryInterface on virtual IAudioSessionControl2 for Mystery UUID: %s",
                          wszGUID ), L"VirtualSnd" );
        }
      }
    }

    if (SUCCEEDED (hr))
      AddRef ();

    return hr;
  }

  virtual ULONG STDMETHODCALLTYPE AddRef (void)
  {
    InterlockedIncrement (&refs_);

    return refs_;
  }

  virtual ULONG STDMETHODCALLTYPE Release (void)
  {
    ULONG ref =
      InterlockedDecrement (&refs_);

    if (ref == 0)
      delete this;

    return ref;
  }

  virtual HRESULT STDMETHODCALLTYPE GetState (AudioSessionState *pRetVal)
  {
    SK_LOG_FIRST_CALL

    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE GetDisplayName (LPWSTR *pRetVal)
  {
    SK_LOG_FIRST_CALL

    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE SetDisplayName (LPCWSTR Value, LPCGUID EventContext)
  {
    SK_LOG_FIRST_CALL

    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE GetIconPath (LPWSTR* pRetVal)
  {
    SK_LOG_FIRST_CALL

    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE SetIconPath (LPCWSTR Value, LPCGUID EventContext)
  {
    SK_LOG_FIRST_CALL

    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE GetGroupingParam (GUID *pRetVal)
  {
    SK_LOG_FIRST_CALL

    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE SetGroupingParam (LPCGUID Override, LPCGUID EventContext)
  {
    SK_LOG_FIRST_CALL

    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE RegisterAudioSessionNotification (IAudioSessionEvents* NewNotifications)
  {
    SK_LOG_FIRST_CALL

    return S_OK;//E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE UnregisterAudioSessionNotification (IAudioSessionEvents *NewNotifications)
  {
    SK_LOG_FIRST_CALL

    return S_OK;//E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE GetSessionIdentifier (LPWSTR *pRetVal)
  {
    SK_LOG_FIRST_CALL

    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE GetSessionInstanceIdentifier (LPWSTR* pRetVal)
  {
    SK_LOG_FIRST_CALL

    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE GetProcessId (DWORD* pRetVal)
  {
    SK_LOG_FIRST_CALL

    if (pRetVal != nullptr)
    {
      *pRetVal = GetCurrentProcessId ();

      return S_OK;
    }

    return E_INVALIDARG;
  }

  virtual HRESULT STDMETHODCALLTYPE IsSystemSoundsSession (void)
  {
    SK_LOG_FIRST_CALL

    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE SetDuckingPreference (BOOL optOut)
  {
    SK_LOG_FIRST_CALL

    return S_OK;//E_NOTIMPL;
  }

protected:
  volatile ULONG refs_;
};

class SK_IVirtualAudioSessionManager2 : public IAudioSessionManager2
{
public:
  SK_IVirtualAudioSessionManager2 (void) : refs_ (1) {};

  virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
  {
    HRESULT hr = E_NOTIMPL;

    if (ppvObject != nullptr)
    {
      if ( riid == __uuidof (IUnknown)             ||
           riid == __uuidof (IAudioSessionManager) ||
           riid == __uuidof (IAudioSessionManager2) )
      {
        hr = S_OK;
        *ppvObject = this;
      }

      else
      {
        static concurrency::concurrent_unordered_set <std::wstring> reported_guids;

        wchar_t                wszGUID [41] = { };
        StringFromGUID2 (riid, wszGUID, 40);

        const bool once =
          reported_guids.count (wszGUID) > 0;

        if (! once)
        {
          reported_guids.insert (wszGUID);

          SK_LOG0 ( ( L"QueryInterface on virtual IAudioSessionManager2 for Mystery UUID: %s",
                          wszGUID ), L"VirtualSnd" );
        }
      }
    }

    if (SUCCEEDED (hr))
      AddRef ();

    return hr;
  }

  virtual ULONG STDMETHODCALLTYPE AddRef (void)
  {
    InterlockedIncrement (&refs_);

    return refs_;
  }

  virtual ULONG STDMETHODCALLTYPE Release (void)
  {
    ULONG ref =
      InterlockedDecrement (&refs_);

    if (ref == 0)
      delete this;

    return ref;
  }

  virtual HRESULT STDMETHODCALLTYPE GetAudioSessionControl (LPCGUID AudioSessionGuid, DWORD StreamFlags, IAudioSessionControl **SessionControl)
  {
    SK_LOG_FIRST_CALL

    if (SessionControl != nullptr)
    {
      *SessionControl =
        new SK_IVirtualAudioSessionControl2 ();

      return S_OK;
    }

    return E_POINTER;
  }

  virtual HRESULT STDMETHODCALLTYPE GetSimpleAudioVolume (LPCGUID AudioSessionGuid, DWORD StreamFlags, ISimpleAudioVolume **AudioVolume)
  {
    SK_LOG_FIRST_CALL

    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE GetSessionEnumerator (IAudioSessionEnumerator **SessionEnum)
  {
    SK_LOG_FIRST_CALL

    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE RegisterSessionNotification (IAudioSessionNotification *SessionNotification)
  {
    SK_LOG_FIRST_CALL

    return S_OK;//E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE UnregisterSessionNotification (IAudioSessionNotification *SessionNotification)
  {
    SK_LOG_FIRST_CALL

    return S_OK;//E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE RegisterDuckNotification (LPCWSTR sessionID, IAudioVolumeDuckNotification *duckNotification)
  {
    SK_LOG_FIRST_CALL

    return S_OK;//E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE UnregisterDuckNotification (IAudioVolumeDuckNotification *duckNotification)
  {
    SK_LOG_FIRST_CALL

    return S_OK;//E_NOTIMPL;
  }

protected:
  volatile ULONG refs_;
};

#pragma pack (push, 1)
#if 1
#define REPORT_SIZE 141
#define REPORT_ID   0x32
#define SAMPLE_SIZE 64
#define SAMPLE_RATE 3000
#else
#define REPORT_SIZE 333
#define REPORT_ID   0x35
#define SAMPLE_SIZE 200
#define SAMPLE_RATE 48000
#endif

typedef struct SK_HID_DualSense_HapticsPacket {
  uint8_t pid   : 6;
  bool    unk   : 1,
          sized : 1;
  uint8_t length;
  uint8_t data [SAMPLE_SIZE];
} SK_HID_DualSense_HapticsPacket_t;

struct SK_HID_DualSense_HapticsReport {
  uint8_t report_id;
  union {
    struct {
      uint8_t tag : 4,
              seq : 4;
      uint8_t data [1];
    };
    struct {
      uint8_t  payload [REPORT_SIZE - sizeof (uint32_t)];
      uint32_t crc;
    };
  };
};
#pragma pack (pop)

uint32_t playstation_crc32 (uint8_t* data, uint32_t size)
{
  uint32_t crc = ~0xEADA2D49; // 0xA2 CRC seed

  for (uint32_t i = 0; i < size; i++)
  {
    crc ^= data [i];
    for (int j = 0; j < 8; j++)
    {
      crc = ((crc >> 1) ^ (0xEDB88320 & (uint32_t)-((int32_t)crc & 1)));
    }
  }

  return ~crc;
}

// Structure representing a single 4-channel input frame
struct InputFrame {
  int16_t channels [4];
};

// Structure representing a single 2-channel output frame
struct OutputFrame {
  int16_t channels [2];
};

/**
 * Resamples 4-channel 48kHz audio to 2-channel 3kHz audio.
 * Removes the first two channels and keeps the last two.
 * 
 * @param input Pointer to the array of 1024 input frames.
 * @param output Vector to store the resulting 64 output frames.
 */
void processAudio (const InputFrame* input, std::vector <OutputFrame>& output)
{
  const int INPUT_FRAMES      = 1024;
  const int DOWNSAMPLE_FACTOR = 16; // 48000 Hz / 3000 Hz

  output.clear ();

  // 1024 / 16 = 64 output frames
  output.reserve (INPUT_FRAMES / DOWNSAMPLE_FACTOR); 

  for (int i = 0; i < INPUT_FRAMES; i += DOWNSAMPLE_FACTOR)
  {
    OutputFrame outFrame;

    // Keep only channel 3 (index 2) and channel 4 (index 3)
    outFrame.channels [0] = input [i].channels [2];
    outFrame.channels [1] = input [i].channels [3];

    output.push_back (outFrame);
  }
}

struct OutputHapticsFrame {
  uint16_t channels [2];
};

struct OutputHapticsPeriod {
  OutputHapticsFrame frames [480]; // 480 frames for 1/10th of a second at 4800 Hz
};

class SK_IVirtualAudioClient2 : public IAudioClient2
{
friend class SK_IVirtualAudioRenderClient;
public:
  SK_IVirtualAudioClient2 (void) : refs_ (1) {
    for (auto& ps_controller : SK_HID_PlayStationControllers)
    {
      if (ps_controller.bConnected && ps_controller.bDualSense && ps_controller.audio_endpoint.p)
      {
        // This will leak, we're only querying it to get the appropriate buffer size, etc. for a real DualSense controller.
        ps_controller.audio_endpoint.p->QueryInterface (__uuidof (IAudioClient2), reinterpret_cast<void**> (&pReal.p));
        break;
      }
    }
  };

  virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
  {
    HRESULT hr = E_NOTIMPL;

    if (ppvObject != nullptr)
    {
      if ( riid == __uuidof (IUnknown)     ||
           riid == __uuidof (IAudioClient) ||
           riid == __uuidof (IAudioClient2) )
      {
        hr = S_OK;
        *ppvObject = this;
      }

      else
      {
        static concurrency::concurrent_unordered_set <std::wstring> reported_guids;

        wchar_t                wszGUID [41] = { };
        StringFromGUID2 (riid, wszGUID, 40);

        const bool once =
          reported_guids.count (wszGUID) > 0;

        if (! once)
        {
          reported_guids.insert (wszGUID);

          SK_LOG0 ( ( L"QueryInterface on virtual IAudioClient for Mystery UUID: %s",
                          wszGUID ), L"VirtualSnd" );
        }
      }
    }

    if (SUCCEEDED (hr))
      AddRef ();

    return hr;
  }

  virtual ULONG STDMETHODCALLTYPE AddRef (void)
  {
    InterlockedIncrement (&refs_);

    return refs_;
  }

  virtual ULONG STDMETHODCALLTYPE Release (void)
  {
    ULONG ref =
      InterlockedDecrement (&refs_);

    if (ref == 0)
      delete this;

    return ref;
  }

  virtual HRESULT STDMETHODCALLTYPE Initialize (AUDCLNT_SHAREMODE ShareMode, DWORD StreamFlags, REFERENCE_TIME hnsBufferDuration, REFERENCE_TIME hnsPeriodicity, const WAVEFORMATEX* pFormat, LPCGUID AudioSessionGuid)
  {
    SK_LOG_FIRST_CALL

    if (StreamFlags != 0 && StreamFlags != AUDCLNT_STREAMFLAGS_EVENTCALLBACK)
    {
      SK_LOGi0 (L"SK_IVirtualAudioClient2::Initialize (...) - Unimplemented Stream Flags: 0x%08X", StreamFlags);
    }

    SK_LOGi0 (L"Buffer Duration: %lld hns, Periodicity: %lld hns", hnsBufferDuration, hnsPeriodicity);

    if (StreamFlags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK)
    {
      event_driven = true;

      const WAVEFORMATEXTENSIBLE* pWaveFormat =
        (WAVEFORMATEXTENSIBLE *)pFormat;

      SK_LOGi0 (
        L"Format: %u Hz, %u Channels, %d bits per-sample, %d valid bits per-sample, %u Samples per-block",
          pWaveFormat->Format.nSamplesPerSec, pWaveFormat->Format.nChannels,
          pWaveFormat->Format.wBitsPerSample, pWaveFormat->Samples.wValidBitsPerSample,
          pWaveFormat->Samples.wSamplesPerBlock );

      SK_LOGi0 (
        L"nAvgBytesPerSec: %u, nBlockAlign: %u, cbSize: %u",
          pWaveFormat->Format.nAvgBytesPerSec,
          pWaveFormat->Format.nBlockAlign,
          pWaveFormat->Format.cbSize
      );

      wchar_t                              wszChannels [33] = {};
      _itow_s (pWaveFormat->dwChannelMask, wszChannels, sizeof (wszChannels) / sizeof (wchar_t), 2);


      SK_LOGi0 (
        L"Channels: [%ws], Format Tag: %u, SubFormat: {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
          wszChannels, pWaveFormat->Format.wFormatTag, // KSDATAFORMAT_SUBTYPE_PCM
                       pWaveFormat->SubFormat.Data1, pWaveFormat->SubFormat.Data2,
                       pWaveFormat->SubFormat.Data3, pWaveFormat->SubFormat.Data4 [0],
                       pWaveFormat->SubFormat.Data4 [1], pWaveFormat->SubFormat.Data4 [2],
                       pWaveFormat->SubFormat.Data4 [3], pWaveFormat->SubFormat.Data4 [4],
                       pWaveFormat->SubFormat.Data4 [5], pWaveFormat->SubFormat.Data4 [6],
                       pWaveFormat->SubFormat.Data4 [7] );
    }

    SK_ReleaseAssert (pFormat != nullptr && pFormat->nChannels == 4);

    SK_ReleaseAssert (ShareMode == AUDCLNT_SHAREMODE_SHARED && hnsPeriodicity == 0);

    if (AudioSessionGuid != nullptr)
    {
      wchar_t guid_str [41] = {};

      StringFromGUID2 (*AudioSessionGuid, guid_str, 40);

      SK_LOGi0 (L"Initializing Audio Client with Audio Session GUID: %ws", guid_str);
    }

    buffer_duration_ = hnsBufferDuration;

    // TODO: Calculate the proper size of this.
                    buffer_frame_count_ = (int)ceil (((double)hnsBufferDuration / 10000000.0) * (double)pFormat->nSamplesPerSec);
    buffer_.resize (buffer_frame_count_ * pFormat->nChannels * (pFormat->wBitsPerSample / 8));

    SK_LOGi0 (L"Calculated Buffer Size: %u frames (%d bytes)", buffer_frame_count_, buffer_.size ());

    initialized = true;

    return S_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE GetBufferSize (UINT32* pNumBufferFrames)
  {
    SK_LOG_FIRST_CALL

    if (pReal != nullptr) {
        pReal->GetBufferSize (pNumBufferFrames);

    //SK_LOGi0 (L"SK_IVirtualAudioClient2::GetBufferSize (...) - Returning Buffer Size: %u frames", *pNumBufferFrames);
    }

    *pNumBufferFrames = buffer_frame_count_;

    return S_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE GetStreamLatency (REFERENCE_TIME *phnsLatency)
  {
    SK_LOG_FIRST_CALL

    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE GetCurrentPadding (UINT32 *pNumPaddingFrames)
  {
    SK_LOG_FIRST_CALL

    if (pNumPaddingFrames == nullptr)
      return E_POINTER;

    // ?
    *pNumPaddingFrames = buffer_padding_;// += NumFramesWritten;

    return S_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE IsFormatSupported (AUDCLNT_SHAREMODE ShareMode, const WAVEFORMATEX *pFormat, WAVEFORMATEX **ppClosestMatch)
  {
    SK_LOG_FIRST_CALL

    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE GetMixFormat (WAVEFORMATEX** ppDeviceFormat)
  {
    SK_LOG_FIRST_CALL

    static unsigned char virtual_format_blob [40] = {
      0xFE, 0xFF, 0x04, 0x00, 0x80, 0xBB, 0x00, 0x00, 0x00, 0xDC, 0x05, 0x00,
      0x08, 0x00, 0x10, 0x00, 0x16, 0x00, 0x10, 0x00, 0x33, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA,
      0x00, 0x38, 0x9B, 0x71
    };

    if (ppDeviceFormat != nullptr)
    {
      WAVEFORMATEX* pFormat =
        (WAVEFORMATEX*)CoTaskMemAlloc (sizeof (WAVEFORMATEX) + 22);

      memcpy (pFormat, virtual_format_blob, sizeof (virtual_format_blob));

      *ppDeviceFormat = pFormat;

      return S_OK;
    }

    return E_POINTER;
  }

  virtual HRESULT STDMETHODCALLTYPE GetDevicePeriod (REFERENCE_TIME* phnsDefaultDevicePeriod, REFERENCE_TIME* phnsMinimumDevicePeriod)
  {
    SK_LOG_FIRST_CALL

    if (phnsDefaultDevicePeriod == nullptr ||
        phnsMinimumDevicePeriod == nullptr)
    {
      return E_INVALIDARG;
    }

    *phnsDefaultDevicePeriod = 100000;
    *phnsMinimumDevicePeriod = 30000;

    return S_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE Start (void)
  {
    SK_LOG_FIRST_CALL

    if (! initialized)
      return AUDCLNT_E_NOT_INITIALIZED;

    if (started)
      return AUDCLNT_E_NOT_STOPPED;

    if (event_driven && event_handle == nullptr)
      return AUDCLNT_E_EVENTHANDLE_NOT_SET;

    started = true;

    return S_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE Stop (void)
  {
    SK_LOG_FIRST_CALL

    if (started)
    {
      started = false;

      return S_OK;
    }

    return S_FALSE;
  }

  virtual HRESULT STDMETHODCALLTYPE Reset (void)
  {
    SK_LOG_FIRST_CALL

    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE SetEventHandle (HANDLE eventHandle)
  {
    SK_LOG_FIRST_CALL

    if (pReal != nullptr)
        pReal->SetEventHandle (eventHandle);

    SK_ReleaseAssert (event_driven || eventHandle == 0);

    const bool first_set =
      (event_handle == 0);

    event_handle = eventHandle;

    if (first_set)
    SK_Thread_CreateEx ([](LPVOID pUser)->DWORD
    {
      auto pThis = (SK_IVirtualAudioClient2 *)pUser;

      SK_LOGi0 (L"Audio Client Event Thread Started");

      while (WaitForSingleObject (pThis->event_handle, INFINITE) == WAIT_OBJECT_0)
      {
        if (! pThis->haptics_buffer_.empty ())
        {
          auto haptics_frame = pThis->haptics_buffer_.front     ();
                               pThis->haptics_buffer_.pop_front ();

          //if (! pThis->silent_)
          {
            SK_LOGi0 (L"Audio Client Event Signaled (%d frames still buffered)", pThis->buffer_padding_);
          }

          pThis->last_frame_count_ = pThis->frame_count_;

          //if (! pThis->silent_)
          {
            static const SK_HID_DualSense_HapticsPacket_t packet_0x11 = {
              .pid    = 0x11,
              .sized  = true,
              .length = 7,
              .data   = {0b11111110, 0, 0, 0, 0, 0xFF, 0},
            }, packet_0x12 = {
              .pid    = 0x12,
              .sized  = true,
              .length = SAMPLE_SIZE,
              .data   = {},
            };

            static SK_HID_DualSense_HapticsReport report { .report_id = REPORT_ID };

            report.tag = 0;

            ////SK_HID_DualSense_HapticsPacket_t* packets [] = {
            ////  (SK_HID_DualSense_HapticsPacket_t *)(report.data + 0),
            ////  (SK_HID_DualSense_HapticsPacket_t *)(report.data + sizeof (packet_0x11) + packet_0x11.length),
            ////};
            ////
            ////memcpy (packets [0], &packet_0x11, sizeof (packet_0x11) + packet_0x11.length);
            ////memcpy (packets [1], &packet_0x12, sizeof (packet_0x12));

            static uint8_t reportSeqCounter = 0;
            static uint8_t packetCounter    = 0;

            uint8_t* data      = (uint8_t *)&report;
                     data [ 0] = (uint8_t)(REPORT_ID);
                     data [ 1] = (uint8_t)( reportSeqCounter << 4);
          reportSeqCounter     = (uint8_t)((reportSeqCounter + 1) & 0x0F);

                   // Packet 0x11
                     data [ 2] = 0x11 | 0 << 6 | 1 << 7; // pid(0x11) unk(false) sized(true)
                     data [ 3] = 7;
                     data [ 4] = 0b11111110;
                     data [ 5] = 0;
                     data [ 6] = 0;
                     data [ 7] = 0;
                     data [ 8] = 0;
                     data [ 9] = 0xFF;
                     data [10] = packetCounter++;

#if 1
                     // Packet 0x12
                     data [11] = 0x12 | 0 << 6 | 1 << 7;
                     data [12] = (byte)SAMPLE_SIZE;
#else
                     // Packet 0x16
                     data [11] = 0x13 | 0 << 6 | 1 << 7;
                     data [12] = (byte)200;
#endif
            uint16_t* outputBuffer = &haptics_frame.frames->channels [0];

            #define OPUS_APPLICATION_AUDIO                 2049
            #define OPUS_APPLICATION_RESTRICTED_LOWDELAY   2051
            #define OPUS_SET_BITRATE_REQUEST               4002
            #define OPUS_SET_VBR_REQUEST                   4006
            #define OPUS_SET_COMPLEXITY_REQUEST            4010
            #define OPUS_SET_EXPERT_FRAME_DURATION_REQUEST 4040

            #define OPUS_FRAMESIZE_ARG                   5000 /**< Select frame size from the argument (default) */
            #define OPUS_FRAMESIZE_10_MS                 5003 /**< Use 10 ms frames */
            
            #define opus_check_int(x) (((void)((x) == (opus_int32)0)), (opus_int32)(x))

            #define OPUS_SET_BITRATE(x)               OPUS_SET_BITRATE_REQUEST,               opus_check_int(x)
            #define OPUS_SET_EXPERT_FRAME_DURATION(x) OPUS_SET_EXPERT_FRAME_DURATION_REQUEST, opus_check_int(x)
            #define OPUS_SET_VBR(x)                   OPUS_SET_VBR_REQUEST,                   opus_check_int(x)
            #define OPUS_SET_COMPLEXITY(x)            OPUS_SET_COMPLEXITY_REQUEST,            opus_check_int(x)

            typedef struct OpusEncoder OpusEncoder;

            typedef          __int32 opus_int32;
            typedef unsigned __int32 opus_uint32;
            typedef          __int16 opus_int16;
            typedef unsigned __int16 opus_uint16;

            using opus_encoder_create_pfn = OpusEncoder* (*)(opus_int32 Fs, int channels, int application, int *error);
            using opus_encoder_ctl_pfn    = int          (*)(OpusEncoder *st, int request, ...);
            using opus_encode_pfn         = opus_int32   (*)(OpusEncoder *st, const opus_int16 *pcm, int analysis_frame_size, unsigned char *data, opus_int32 max_data_bytes);

            static opus_encoder_create_pfn opus_encoder_create = (opus_encoder_create_pfn)SK_GetProcAddress (SK_LoadLibraryW ((std::filesystem::path (SK_GetPlugInDirectory (SK_PlugIn_Type::ThirdParty)) / L"Audio Codecs/opus/opus_x64.dll").c_str ()), "opus_encoder_create");
            static opus_encoder_ctl_pfn    opus_encoder_ctl    = (opus_encoder_ctl_pfn)   SK_GetProcAddress (SK_LoadLibraryW ((std::filesystem::path (SK_GetPlugInDirectory (SK_PlugIn_Type::ThirdParty)) / L"Audio Codecs/opus/opus_x64.dll").c_str ()), "opus_encoder_ctl");
            static opus_encode_pfn         opus_encode         = (opus_encode_pfn)        SK_GetProcAddress (SK_LoadLibraryW ((std::filesystem::path (SK_GetPlugInDirectory (SK_PlugIn_Type::ThirdParty)) / L"Audio Codecs/opus/opus_x64.dll").c_str ()), "opus_encode");

            if ( opus_encoder_create &&
                 opus_encoder_ctl    &&
                 opus_encode )
            {
              auto opus_strerror = [](int error) -> const char*
              {
                static const char * const error_strings[8] = {
                   "success",
                   "invalid argument",
                   "buffer too small",
                   "internal error",
                   "corrupted stream",
                   "request not implemented",
                   "invalid state",
                   "memory allocation failed"
                };
                if (error > 0 || error < -7)
                   return "unknown error";
                else
                   return error_strings [-error];
              };

              static int error = 0;

              static OpusEncoder* encoder =
                opus_encoder_create (48000, 2, OPUS_APPLICATION_RESTRICTED_LOWDELAY, &error);

              if (error >= 0)
              {
                SK_RunOnce (
                  auto x = opus_encoder_ctl (encoder, OPUS_SET_BITRATE               (SAMPLE_SIZE * 8 * 100));
                  auto y = opus_encoder_ctl (encoder, OPUS_SET_EXPERT_FRAME_DURATION (OPUS_FRAMESIZE_10_MS));
                  auto z = opus_encoder_ctl (encoder, OPUS_SET_VBR                   (0));
                  auto w = opus_encoder_ctl (encoder, OPUS_SET_COMPLEXITY            (0));

                  SK_LOGi0 (L"Opus Encoder Config: Bitrate: %d, Frame Duration: %d, VBR: %d, Complexity: %d", x, y, z, w);
                );

                auto bytes_encoded =
                  opus_encode (encoder, (opus_int16 *)outputBuffer, 480, &data [13], 64);

                if (bytes_encoded > 0) {
                  SK_LOGi0 (L"Encoded %d bytes of Opus", bytes_encoded);
                }
              }

              else
              {
                SK_LOGi0 (L"Opus Encoder Creation Failed: %d", error);
              }
            }

            for (auto& ps_controller : SK_HID_PlayStationControllers)
            {
              if (ps_controller.bBluetooth && ps_controller.bConnected && ps_controller.bDualSense)
              {
                SK_AutoHandle hSyncFile (
                  CreateFile (ps_controller.wszDevicePath, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0x0, nullptr)
                );

                *((uint32_t *)&data [REPORT_SIZE-3]) =
                  playstation_crc32 (data, REPORT_SIZE-3);

                uint8_t report32 [142] = {};
                        report32 [0] = 0x32;
                        report32 [1] = 0x10;

                uint8_t packet_0x10 [] =
                {
                  0x90, // Packet: 0x10
                  0x3f, // 63
                  // Length: 47 ⬇️
                  // SetStateData 
                  0xfd, 0xf7, 0x00, 0x00, 0x7f, 0x7f,
                  0xff, 0x09, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0,
                  0x07, 0x00, 0x00, 0x02, 0x01,
                  0x00,
                  0xff,0xd7,0x00 // RGB LED: R, G, B
                };

                memcpy       (&report32 [  2], packet_0x10, sizeof (packet_0x10));
                *((uint32_t *)&report32 [138]) = playstation_crc32 (report32, 138);

                SK_AutoHandle hOutputEvent (
                  CreateEvent (nullptr, FALSE, FALSE, nullptr)
                );

                SK_RunOnce (
                  SK_AutoHandle hOutputEvent2 (
                    CreateEvent (nullptr, FALSE, FALSE, nullptr)
                  );

                  OVERLAPPED async_output_request2        = { };
                             async_output_request2.hEvent = hOutputEvent2;

                  SK_WriteFile (ps_controller.hDeviceFile, report32, 142, nullptr, &async_output_request2);

                  //GetOverlappedResult (ps_controller.hDeviceFile, &async_output_request2, nullptr, TRUE);
                );

                OVERLAPPED async_output_request        = { };
                           async_output_request.hEvent = hOutputEvent;

                async_output_request.hEvent = hOutputEvent;

                const BOOL bWriteAsync =
                  SK_WriteFile ( ps_controller.hDeviceFile, &report, sizeof (report), nullptr, &async_output_request);

                if (bWriteAsync)
                {
                  //WaitForSingleObject (async_output_request.hEvent, 20);
                  GetOverlappedResult (ps_controller.hDeviceFile, &async_output_request, nullptr, FALSE);
                }
                
                else
                {
                  DWORD dwLastErr = GetLastError ();
                
                  _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));
                
                  if (dwLastErr != ERROR_IO_PENDING && dwLastErr != ERROR_INVALID_HANDLE)
                  {
                    SK_CancelIoEx (ps_controller.hDeviceFile, &async_output_request);
                
                    SK_LOGi0 (L"Haptics Write Failed: 0x%04X (%ws",
                      err.WCode (), err.ErrorMessage ()
                    );
                  }
                }
                break;
              }
            }
          }
        }
      }

      return 0;
    }, L"[SK] Haptics Audio Thread", (LPVOID)this);

    return S_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE GetService (REFIID riid, void** ppv)
  {
    SK_LOG_FIRST_CALL

    if (IsEqualGUID (riid, __uuidof (IAudioRenderClient)))
    {
      if (ppv != nullptr)
      {
        *ppv =
          new SK_IVirtualAudioRenderClient (this);

        return S_OK;
      }

      return E_POINTER;
    }

    wchar_t                wszGUID [41] = { };
    StringFromGUID2 (riid, wszGUID, 40);

    SK_LOGi0 (L"SK_IVirtualAudioClient2::GetService (%ws, %p)", wszGUID, ppv);

    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE IsOffloadCapable (AUDIO_STREAM_CATEGORY Category, BOOL *pbOffloadCapable)
  {
    SK_LOG_FIRST_CALL

    if (pbOffloadCapable != nullptr)
    {
      // Don't know what this is...?
      *pbOffloadCapable = TRUE;

      return S_OK;
    }

    return E_INVALIDARG;
  }

  virtual HRESULT STDMETHODCALLTYPE SetClientProperties (const AudioClientProperties* pProperties)
  {
    SK_LOG_FIRST_CALL

    // Ignore client properties for now

    return S_OK;//E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE GetBufferSizeLimits (const WAVEFORMATEX *pFormat, BOOL bEventDriven, REFERENCE_TIME *phnsMinBufferDuration, REFERENCE_TIME *phnsMaxBufferDuration)
  {
    SK_LOG_FIRST_CALL

    return E_NOTIMPL;
  }

protected:
  volatile ULONG refs_;
  REFERENCE_TIME buffer_duration_ = 0;
            bool event_driven     = false;
          HANDLE event_handle     = 0;
            bool started          = false;
            bool initialized      = false;

            bool silent_          = false;
          UINT32 buffer_head_        = 0;
          UINT32 buffer_tail_        = 0;
          UINT32 buffer_padding_     = 0;
          UINT32 buffer_frame_count_ = 3840;
     std::vector <BYTE>
                 buffer_;
     std::deque <OutputHapticsPeriod>
                 haptics_buffer_;
          UINT32 haptics_frame_count_ = 0;

          UINT64 last_frame_count_ = 0;
          UINT64      frame_count_ = 0;

  SK_ComPtr <IAudioClient2> pReal;
};

HRESULT STDMETHODCALLTYPE SK_IVirtualAudioRenderClient::GetBuffer (UINT32 NumFramesRequested, BYTE **ppData)
{
  SK_LOG_FIRST_CALL

  if (ppData == nullptr)
    return E_POINTER;

  if (NumFramesRequested > parent_->buffer_frame_count_)
    return AUDCLNT_E_BUFFER_TOO_LARGE;

  if (acquired_frames_ != 0)
    return AUDCLNT_E_OUT_OF_ORDER;

  acquired_frames_ = NumFramesRequested;
  *ppData          = &parent_->buffer_.data ()[parent_->buffer_padding_ * 8];

  return S_OK;
}

HRESULT STDMETHODCALLTYPE SK_IVirtualAudioRenderClient::ReleaseBuffer (UINT32 NumFramesWritten, DWORD dwFlags)
{
  SK_LOG_FIRST_CALL

  if (NumFramesWritten != acquired_frames_)
  {
    acquired_frames_ = 0;
    return AUDCLNT_E_INVALID_SIZE;
  }

  if (dwFlags & AUDCLNT_BUFFERFLAGS_SILENT)
  {
    parent_->silent_ = true;
    memset (parent_->buffer_.data () + parent_->buffer_padding_ * 8, 0, NumFramesWritten * 8); // 4 channels * 16 bits per sample = 8 bytes per frame
  }

  else
  {
    parent_->silent_ = false;
  }

  parent_->buffer_padding_ += NumFramesWritten;

  static size_t frames_written_total = 0;
                frames_written_total += NumFramesWritten;

  parent_->frame_count_ += NumFramesWritten;

  //if (! parent_->silent_)
  {
    int complete_10ms_periods =
      parent_->buffer_padding_ / 480;

    for (int i = 0; i < complete_10ms_periods; ++i)
    {
      parent_->buffer_padding_ -= 480;

      OutputHapticsPeriod buffered_10ms = {};

      for (int j = 0; j < 480; j++)
      {
        OutputHapticsFrame& outFrame = buffered_10ms.frames [j];
        InputFrame*         inFrame  = (InputFrame *)&parent_->buffer_ [(i * 480 * 8) + j * 8];

        // Keep only channel 3 (index 2) and channel 4 (index 3)
        outFrame.channels [0] = inFrame->channels [2];
        outFrame.channels [1] = inFrame->channels [3];
      }

      this->parent_->haptics_buffer_.push_back (buffered_10ms);
    }

    SK_LOGi0 (L"Audio Render Client: Frames Written = %zu", frames_written_total);
  }

  acquired_frames_ = 0;

  // For now, just ignore the data that's been written to the buffer, but in the future we may want to actually do something with it.
  return S_OK;
}

#pragma warning (pop)

class SK_IVirtualMMDevice : public IMMDevice
{
friend class SK_IVirtualPropertyStore;
public:
  static volatile ULONG NumberOfVirtualDevices;

  SK_IVirtualMMDevice (GUID container_id, const wchar_t* wszDeviceName) : pReal (nullptr), refs_ (1)
  {
    virtual_container_id_ = container_id;
    virtual_device_name_  = wszDeviceName;

    GUID    guid;
    wchar_t guid_str [41] = {};

    if (SUCCEEDED (CoCreateGuid (&guid)))
    {
      StringFromGUID2 (guid, guid_str, 40);
    }

    wnsprintfW (wszVirtualPath, 63, L"{0.0.0.00000000}.{%ws}", guid_str);

    InterlockedIncrement (&NumberOfVirtualDevices);

    virtual_property_store_.p =
      new SK_IVirtualPropertyStore (this);
  }

  SK_IVirtualMMDevice (IMMDevice* real_device) : pReal (real_device), refs_ (1) {}

  virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
  {
    if (ppvObject == nullptr)
      return E_POINTER;

    HRESULT hr = E_NOTIMPL;

    if (pReal == nullptr)
    {
      if (ppvObject != nullptr)
      {
        if ( riid == __uuidof (IUnknown) ||
             riid == __uuidof (IMMDevice) )
        {
          hr = S_OK;
          *ppvObject = this;
        }

        else if ( ( riid == __uuidof (IAudioClient) ||
                    riid == __uuidof (IAudioClient2) ) && client_ != nullptr )
        {
          hr = S_OK;

          client_.p->AddRef ();
          *ppvObject = client_;

          return hr;
        }

        else
        {
          static concurrency::concurrent_unordered_set <std::wstring> reported_guids;

          wchar_t                wszGUID [41] = { };
          StringFromGUID2 (riid, wszGUID, 40);

          const bool once =
            reported_guids.count (wszGUID) > 0;

          if (! once)
          {
            reported_guids.insert (wszGUID);

            SK_LOG0 ( ( L"QueryInterface on virtual IMMDevice for Mystery UUID: %s",
                            wszGUID ), L"VirtualSnd" );
          }
        }
      }
    }

    else
    {
      hr =
        pReal->QueryInterface (riid, ppvObject);
    }

    if (SUCCEEDED (hr))
      AddRef ();

    return hr;
  }

  virtual ULONG STDMETHODCALLTYPE AddRef (void)
  {
    InterlockedIncrement (&refs_);

    return refs_;
  }

  virtual ULONG STDMETHODCALLTYPE Release (void)
  {
    ULONG ref =
      InterlockedDecrement (&refs_);

    if (ref == 0)
      delete this;

    return ref;
  }

  virtual HRESULT STDMETHODCALLTYPE Activate (REFIID iid, DWORD dwClsCtx, PROPVARIANT *pActivationParams, void **ppInterface)
  {
    SK_LOG_FIRST_CALL

    if (pReal == nullptr)
    {
      if (IsEqualGUID (iid, __uuidof (IAudioClient)) ||
          IsEqualGUID (iid, __uuidof (IAudioClient2)))
      {
        if (ppInterface != nullptr)
        {
          if (client_ == nullptr)
          {
            client_ =
              new SK_IVirtualAudioClient2 ();
          }

                         client_.p->AddRef ();
          *ppInterface = client_;

          return S_OK;
        }

        return E_POINTER;
      }

      if (IsEqualGUID (iid, __uuidof (IAudioSessionManager)) ||
          IsEqualGUID (iid, __uuidof (IAudioSessionManager2)))
      {
        if (ppInterface != nullptr)
        {
          *ppInterface =
            new SK_IVirtualAudioSessionManager2 ();

          return S_OK;
        }

        return E_POINTER;
      }

      wchar_t               wszGUID [41] = { };
      StringFromGUID2 (iid, wszGUID, 40);

      SK_LOGi0 (L"SK_IVirtualMMDevice::Activate (%ws,...) - Unimplemented Stub Called!", wszGUID);

      return E_NOTIMPL;
    }

    return
      pReal->Activate (iid, dwClsCtx, pActivationParams, ppInterface);
  }

  virtual HRESULT STDMETHODCALLTYPE OpenPropertyStore (DWORD stgmAccess, IPropertyStore** ppProperties)
  {
    IPropertyStore* pPropertyStore = nullptr;

    HRESULT hr = E_POINTER;

    if (pReal == nullptr)
    {
      if (ppProperties != nullptr)
      {
                        virtual_property_store_.p->AddRef ();
        *ppProperties = virtual_property_store_;

        return S_OK;
      }

      return hr;
    }

    pReal->OpenPropertyStore (stgmAccess, &pPropertyStore); 

    if (SUCCEEDED (hr))
    {
      *ppProperties =
        new SK_IVirtualPropertyStore (pPropertyStore);
    }

    return hr;
  }

  virtual HRESULT STDMETHODCALLTYPE GetId (LPWSTR* ppstrId)
  {
    if (pReal == nullptr)
    {
      if (ppstrId != nullptr)
      {
        SK_LOG_FIRST_CALL

                *ppstrId = (LPWSTR)CoTaskMemAlloc (sizeof (wchar_t) * (wcslen (wszVirtualPath) + 1));
        memcpy (*ppstrId,          wszVirtualPath, sizeof (wchar_t) * (wcslen (wszVirtualPath) + 1));

        return S_OK;
      }

      return E_POINTER;
    }

    return
      pReal->GetId (ppstrId);
  }

  virtual HRESULT STDMETHODCALLTYPE GetState (DWORD* pdwState)
  {
    if (pReal == nullptr)
    {
      if (pdwState != nullptr)
         *pdwState = DEVICE_STATE_ACTIVE;

      return S_OK;
    }

    return
      pReal->GetState (pdwState);
  }

protected:
           IMMDevice*    pReal;
           wchar_t       wszVirtualPath [64]     = {};
           SK_ComPtr <SK_IVirtualPropertyStore>
                         virtual_property_store_ = nullptr;
           GUID          virtual_container_id_;
           std::wstring  virtual_device_name_;
           unsigned char virtual_format_blob_ [40] = {
              0xFE, 0xFF, 0x04, 0x00, 0x80, 0xBB, 0x00, 0x00, 0x00, 0xDC, 0x05, 0x00,
              0x08, 0x00, 0x10, 0x00, 0x16, 0x00, 0x10, 0x00, 0x33, 0x00, 0x00, 0x00,
              0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA,
              0x00, 0x38, 0x9B, 0x71
           };
           SK_ComPtr <SK_IVirtualAudioClient2>
                         client_;
  volatile ULONG         refs_;
};

volatile ULONG SK_IVirtualMMDevice::NumberOfVirtualDevices = 0;

SK_IVirtualPropertyStore::SK_IVirtualPropertyStore (SK_IVirtualMMDevice* pDevice)
{
  pReal = nullptr;
  refs_ = 1;
  PROPVARIANT
    propvarName;
    propvarName.vt      = VT_LPWSTR;
    propvarName.pwszVal = (wchar_t *)pDevice->virtual_device_name_.c_str ();

  PROPVARIANT
    propvarFormat;
    propvarFormat.vt             = VT_BLOB;
    propvarFormat.blob.cbSize    = 40;
  //propvarFormat.blob.pBlobData = (uint8_t *)&pDevice->virtual_format_;
    propvarFormat.blob.pBlobData = pDevice->virtual_format_blob_;

  PROPVARIANT
    propvarContainer;
    propvarContainer.vt    = VT_CLSID;
    propvarContainer.puuid = &pDevice->virtual_container_id_;

  virtual_properties_.push_back ( { PKEY_Device_FriendlyName,      propvarName      } );
  virtual_properties_.push_back ( { PKEY_AudioEngine_DeviceFormat, propvarFormat    } );
  virtual_properties_.push_back ( { PKEY_Device_ContainerId,       propvarContainer } );
}

IMMDevice*
SK_VirtualAudio_CreateVirtualDevice (GUID container_id, const wchar_t* wszDeviceName)
{
  wchar_t                        wszGUID [41] = { };
  StringFromGUID2 (container_id, wszGUID, 40);

  SK_LOGi0 (L"Created Virtual Audio Device: %ws [%ws]", wszDeviceName, wszGUID);

  SK_IVirtualMMDevice* pDevice =
    new SK_IVirtualMMDevice (container_id, wszDeviceName);

  virtual_audio_registrar->addDevice (pDevice);

  return pDevice;
}

class SK_IVirtualMMDeviceCollection : public IMMDeviceCollection
{
public:
  SK_IVirtualMMDeviceCollection (IMMDeviceCollection* real_collection) : pReal (real_collection), refs_ (1) {}

  virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
  {
    HRESULT hr =
      pReal->QueryInterface (riid, ppvObject);

    if (SUCCEEDED (hr))
      AddRef ();

    return hr;
  }

  virtual ULONG STDMETHODCALLTYPE AddRef (void)
  {
    InterlockedIncrement (&refs_);

    return refs_;
  }

  virtual ULONG STDMETHODCALLTYPE Release (void)
  {
    ULONG ref =
      InterlockedDecrement (&refs_);

    if (ref == 0)
      delete this;

    return ref;
  }

  virtual HRESULT STDMETHODCALLTYPE GetCount (UINT *pcDevices)
  {
    UINT count = 0;

    HRESULT hr =
      pReal->GetCount (&count);

    if (SUCCEEDED (hr))
    {
      count +=
        (UINT)virtual_audio_registrar->virtual_devices_.size ();
    }

    if (pcDevices != nullptr)
       *pcDevices = count;

    return hr;
  }

  virtual HRESULT STDMETHODCALLTYPE Item (UINT nDevice, IMMDevice** ppDevice)
  {
    UINT              real_count = 0;
    pReal->GetCount (&real_count);

    // Logic to wrap the real device is kept here in case it is necessary to examine
    //   the API behavior of a real DualSense controller.
    //
    // This is our virtual device
    if ((nDevice < real_count + virtual_audio_registrar->virtual_devices_.size () && (real_count == 0 || nDevice > real_count - 1))/* || nDevice == real_count - 1)*/)
    {
      IMMDevice* pDevice = nullptr;

      // Not implemented yet, just return a wrapper around the last device.
      HRESULT hr = E_INVALIDARG;

      static int reported_count = 0;

      if (nDevice < real_count)
      {
        if (++reported_count < 10)
          SK_LOGi0 (L"SK_IVirtualMMDeviceCollection::Item (...) returning wrapped device for index %u", nDevice);

        hr = pReal->Item (nDevice, &pDevice);
      }
      else if (nDevice < real_count + virtual_audio_registrar->virtual_devices_.size ())
      {
        if (++reported_count < 10)
          SK_LOGi0 (L"SK_IVirtualMMDeviceCollection::Item (...) returning virtual device for index %u", nDevice);

        pDevice =
          virtual_audio_registrar->virtual_devices_ [nDevice - real_count];
        pDevice->AddRef ();

        *ppDevice = pDevice;

        return S_OK;
      }

      if (SUCCEEDED (hr))
      {
        SK_ComPtr <IPropertyStore>              pPropertyStore = nullptr;
        pDevice->OpenPropertyStore (STGM_READ, &pPropertyStore.p);

        if (pPropertyStore.p != nullptr)
        {
          PROPVARIANT       friendly_name = {};
          PropVariantInit (&friendly_name);

          pPropertyStore->GetValue (PKEY_Device_FriendlyName, &friendly_name);

          PROPVARIANT       container_id = {};
          PropVariantInit (&container_id);

          if (SUCCEEDED (pPropertyStore->GetValue (PKEY_Device_ContainerId, &container_id)))
          {
            if (container_id.vt == VT_CLSID)
            {
              for (auto& controller : SK_HID_PlayStationControllers)
              {
                if (IsEqualGUID (*container_id.puuid, controller.container_id))
                {
                  SK_ImGui_Warning (friendly_name.pwszVal);

                  SK_LOGi0 (L"Controller Name: %ws", friendly_name.pwszVal);
                  break;
                }
              }
            }
          }

          PropVariantClear (&container_id);
          PropVariantClear (&friendly_name);
        }

        *ppDevice =
          new SK_IVirtualMMDevice (pDevice);
      }

      return
        hr;
    }

    return
      pReal->Item (nDevice, ppDevice);
  }

protected:
           IMMDeviceCollection* pReal;
  volatile ULONG                refs_;
};

class SK_IVirtualMMDeviceEnumerator : public IMMDeviceEnumerator
{
public:
  SK_IVirtualMMDeviceEnumerator (IMMDeviceEnumerator* real_enumerator) : pReal (real_enumerator), refs_ (1) {}

  virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
  {
    HRESULT hr =
      pReal->QueryInterface (riid, ppvObject);

    if (SUCCEEDED (hr))
      AddRef ();

    return hr;
  }

  virtual ULONG STDMETHODCALLTYPE AddRef (void)
  {
    InterlockedIncrement (&refs_);

    return refs_;
  }

  virtual ULONG STDMETHODCALLTYPE Release (void)
  {
    ULONG ref =
      InterlockedDecrement (&refs_);

    if (ref == 0)
      delete this;

    return ref;
  }

  virtual HRESULT STDMETHODCALLTYPE EnumAudioEndpoints (EDataFlow dataFlow, DWORD dwStateMask, IMMDeviceCollection **ppDevices)
  {
    // SK has no interest in adding a virtual microphone or disconnected devices...
    if ((dataFlow != eRender && dataFlow != eAll) || (dwStateMask & DEVICE_STATE_ACTIVE) == 0)
    {
      return
        pReal->EnumAudioEndpoints (dataFlow, dwStateMask, ppDevices);
    }

    IMMDeviceCollection* pRealCollection = nullptr;

    HRESULT hr =
      pReal->EnumAudioEndpoints (dataFlow, dwStateMask, &pRealCollection);

    if (SUCCEEDED (hr))
    {
      if (ppDevices != nullptr)
      {
        *ppDevices =
          new SK_IVirtualMMDeviceCollection (pRealCollection);

        return hr;
      }
    }

    if (ppDevices != nullptr)
       *ppDevices = pRealCollection;

    return hr;
  }

  virtual HRESULT STDMETHODCALLTYPE GetDefaultAudioEndpoint (EDataFlow dataFlow, ERole role, IMMDevice **ppEndpoint)
  {
    return
      pReal->GetDefaultAudioEndpoint (dataFlow, role, ppEndpoint);
  }

  virtual HRESULT STDMETHODCALLTYPE GetDevice (LPCWSTR pwstrId, IMMDevice **ppDevice)
  {
    SK_LOGi0 (L"SK_IVirtualMMDeviceEnumerator::GetDevice (%ws, %p)", pwstrId, ppDevice);

    HRESULT hr =
      pReal->GetDevice (pwstrId, ppDevice);

    if (FAILED (hr))
    {
      UINT device_count =
        static_cast <UINT> (virtual_audio_registrar->virtual_devices_.size ());

      for ( UINT dev_idx = 0 ; dev_idx < device_count ; dev_idx++ )
      {
        auto device =
          virtual_audio_registrar->virtual_devices_ [dev_idx];

        wchar_t*        wszId = nullptr;
        device->GetId (&wszId);

        if (wszId != nullptr && 0 == wcscmp (pwstrId, wszId))
        {
          CoTaskMemFree (wszId);

          SK_LOGi0 (L"SK_IVirtualMMDeviceEnumerator::GetDevice (...) - Returning Virtual Device for ID %ws", pwstrId);

           *ppDevice = device;
          (*ppDevice)->AddRef ();

          return S_OK;
        }
      }
    }

    return hr;
  }

  virtual HRESULT STDMETHODCALLTYPE RegisterEndpointNotificationCallback (IMMNotificationClient* pClient)
  {
    SK_LOG_FIRST_CALL

    SK_LOGi0 (L"RegisterEndpointNotificationCallback (%p) [%ws]", pClient, SK_GetCallerName ().c_str ());

    if (pClient != nullptr)
    {
      virtual_audio_registrar->addNotificationClient (pClient);

      UINT device_count =
        static_cast <UINT> (virtual_audio_registrar->virtual_devices_.size ());

      for ( UINT dev_idx = 0 ; dev_idx < device_count ; ++dev_idx )
      {
        auto& device =
          virtual_audio_registrar->virtual_devices_ [dev_idx];

        wchar_t*        wszId = nullptr;
        device->GetId (&wszId);

        if (wszId != nullptr)
        {
          virtual_audio_registrar->broadcastDeviceAdded (wszId);
          CoTaskMemFree                                 (wszId);
        }
      }
    }

    return
      pReal->RegisterEndpointNotificationCallback (pClient);
  }

  virtual HRESULT STDMETHODCALLTYPE UnregisterEndpointNotificationCallback (IMMNotificationClient *pClient)
  {
    SK_LOG_FIRST_CALL

    SK_LOGi0 (L"UnregisterEndpointNotificationCallback (%p) [%ws]", pClient, SK_GetCallerName ().c_str ());

    if (pClient != nullptr)
      virtual_audio_registrar->removeNotificationClient (pClient);

    return
      pReal->UnregisterEndpointNotificationCallback (pClient);
  }

protected:
           IMMDeviceEnumerator* pReal;
  volatile ULONG                refs_;
};

HRESULT
SK_MMDevAPI_CreateVirtualEnumerator (IMMDeviceEnumerator** ppEnum)
{
  IMMDeviceEnumerator* pRealEnum = nullptr;

  HRESULT hr =
    SK_CoCreateInstance (__uuidof (MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS (&pRealEnum));

  // Not ready for primetime
#ifdef SK_ENABLE_DUALSENSE_VIRTUAL_HAPTICS
  if (SUCCEEDED (hr))
  {
    *ppEnum =
      new SK_IVirtualMMDeviceEnumerator (pRealEnum);

    return hr;
  }
#endif

  *ppEnum = pRealEnum;

  return hr;
}