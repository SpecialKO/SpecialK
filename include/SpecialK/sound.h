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

#ifndef __SK__SOUND_H__
#define __SK__SOUND_H__

#include <Mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <SpecialK/com_util.h>

using SK_ISimpleAudioVolume      = SK_ComPtr <ISimpleAudioVolume>;
using SK_IAudioMeterInformation  = SK_ComPtr <IAudioMeterInformation>;
using SK_IAudioAutoGainControl   = SK_ComPtr <IAudioAutoGainControl>;
using SK_IAudioEndpointVolume    = SK_ComPtr <IAudioEndpointVolume>;
using SK_IChannelAudioVolume     = SK_ComPtr <IChannelAudioVolume>;
using SK_IAudioLoudness          = SK_ComPtr <IAudioLoudness>;

using SK_IMMDevice               = SK_ComPtr <IMMDevice>;
using SK_IMMDeviceEnumerator     = SK_ComPtr <IMMDeviceEnumerator>;
using SK_IAudioSessionEnumerator = SK_ComPtr <IAudioSessionEnumerator>;
using SK_IAudioSessionControl    = SK_ComPtr <IAudioSessionControl>;
using SK_IAudioSessionControl2   = SK_ComPtr <IAudioSessionControl2>;
using SK_IAudioSessionManager2   = SK_ComPtr <IAudioSessionManager2>;
using SK_IAudioClient3           = SK_ComPtr <IAudioClient3>;

bool                      __stdcall SK_WASAPI_Init                    (void);
void                      __stdcall SK_SetGameMute                    (bool bMute);
BOOL                      __stdcall SK_IsGameMuted                    (void);
SK_IAudioMeterInformation __stdcall SK_WASAPI_GetAudioMeterInfo       (void);
SK_ISimpleAudioVolume     __stdcall SK_WASAPI_GetVolumeControl        (DWORD   proc_id = GetCurrentProcessId ());
SK_IChannelAudioVolume    __stdcall SK_WASAPI_GetChannelVolumeControl (DWORD   proc_id = GetCurrentProcessId ());
void                      __stdcall SK_WASAPI_GetAudioSessionProcs    (size_t* count, DWORD* procs = nullptr);
SK_IAudioClient3          __stdcall SK_WASAPI_GetAudioClient          (void);

const char*               __stdcall SK_WASAPI_GetChannelName          (int channel_idx);

SK_IAudioEndpointVolume   __stdcall SK_MMDev_GetEndpointVolumeControl (void);
SK_IAudioLoudness         __stdcall SK_MMDev_GetLoudness              (void);
SK_IAudioAutoGainControl  __stdcall SK_MMDev_GetAutoGainControl       (void);

#include <SpecialK/steam_api.h>
#include <SpecialK/window.h>

#include <atlbase.h>
#include <TlHelp32.h>

#include <unordered_map>
#include <set>

class SK_WASAPI_SessionManager;

class SK_WASAPI_AudioSession : public IAudioSessionEvents
{
public:
  SK_WASAPI_AudioSession ( SK_IAudioSessionControl2 pSession,
                           SK_WASAPI_SessionManager *pParent  ) :
    control_ (pSession),
     parent_ (pParent),
       refs_ (1)
  {
    if (pSession != nullptr)
    {
      pSession->RegisterAudioSessionNotification (this);

      char szTitle [512]  =
      {                   };

      const DWORD proc_id =
        getProcessId ();

      window_t win =
        SK_FindRootWindow (proc_id);

      if (win.root != nullptr)
      {
        wchar_t wszTitle [512] = { };

        BOOL bUsedDefaultChar = FALSE;

        // This is all happening from the application's message pump in most games,
        //   so this specialized function avoids deadlocking the pump.
        InternalGetWindowText (win.root, wszTitle, 511);
        WideCharToMultiByte   (CP_UTF8, 0x00, wszTitle, sk::narrow_cast <int> (wcslen (wszTitle)), szTitle, 511, nullptr, &bUsedDefaultChar);

        //SK_LOG4 ( ( L" Audio Session (pid=%lu)", proc_id ),
                    //L"  WASAPI  " );
      }

// Use the ANSI versions
#undef PROCESSENTRY32
#undef Process32First
#undef Process32Next

      // Use the exeuctable name if there is no window name
      if (0 == strnlen (szTitle, 512))
      {
        HANDLE hSnap =
          CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);

        if (hSnap)
        {
          PROCESSENTRY32
            pent        = {                     };
            pent.dwSize = sizeof (PROCESSENTRY32);

          if (Process32First (hSnap, &pent))
          {
            do
            {
              if (pent.th32ProcessID == proc_id)
              {
                *szTitle = '\0';
                strncat (szTitle, pent.szExeFile, 511);
                break;
              }

            } while (Process32Next (hSnap, &pent));
          }

          SK_CloseHandle (hSnap);
        }
      }

      app_name_ = szTitle;

      if (proc_id == GetCurrentProcessId ())
      {
        if (SK::SteamAPI::AppName ().length ())
          app_name_ = SK::SteamAPI::AppName ();
      }

      //SK_LOG4 ( ( L"   Name: %s", wszTitle ),
                  //L"  WASAPI  " );
    }
  }

  SK_ISimpleAudioVolume getSimpleAudioVolume (void)
  {
    SK_ISimpleAudioVolume pRet;

    if (SUCCEEDED (control_->QueryInterface <ISimpleAudioVolume> (&pRet.p)))
      return pRet;

    return nullptr;
  }

  SK_IChannelAudioVolume getChannelAudioVolume (void)
  {
    SK_IChannelAudioVolume pRet;

    if (SUCCEEDED (control_->QueryInterface <IChannelAudioVolume> (&pRet.p)))
      return pRet;

    return nullptr;
  }

  SK_IAudioEndpointVolume   getEndpointVolume  (void);
  SK_IAudioLoudness         getLoudness        (void);
  SK_IAudioAutoGainControl  getAutoGainControl (void);

  DWORD getProcessId (void)
  {
    DWORD dwProcId = 0;

    if (FAILED (control_->GetProcessId (&dwProcId)))
      return 0;

    return dwProcId;
  }

  const char* getName (void) noexcept { return app_name_.c_str (); };

  // IUnknown
  HRESULT
  STDMETHODCALLTYPE
  QueryInterface (REFIID riid, void **ppv) override
  {
    if (! ppv)
      return E_INVALIDARG;

    // UNSAFE, FIXEME
    if (IID_IUnknown == riid)
    {
      AddRef ();
      *ppv = (IUnknown *)this;
    }

    else if (__uuidof (IAudioSessionEvents) == riid)
    {
      AddRef ();
      *ppv = (IAudioSessionEvents *)this;
    }

    else
    {
      *ppv = nullptr;
      return E_NOINTERFACE;
    }

    return S_OK;
  }

  ULONG STDMETHODCALLTYPE AddRef (void) noexcept override
  {
    return
      InterlockedIncrement (&refs_);
  }

  ULONG STDMETHODCALLTYPE Release (void) noexcept override
  {
    const ULONG ulRef =
      InterlockedDecrement (&refs_);

    if (ulRef == 0)
    {
      delete this;
    }

    return ulRef;
  }

  HRESULT
  STDMETHODCALLTYPE
  OnDisplayNameChanged (PCWSTR NewDisplayName, LPCGUID EventContext)  override {
    UNREFERENCED_PARAMETER (NewDisplayName);
    UNREFERENCED_PARAMETER (EventContext);

    return S_OK;
  };

  HRESULT
  STDMETHODCALLTYPE
  OnIconPathChanged (LPCWSTR NewIconPath, LPCGUID EventContext)  override {
    UNREFERENCED_PARAMETER (NewIconPath);
    UNREFERENCED_PARAMETER (EventContext);

    return S_OK;
  };

  HRESULT
  STDMETHODCALLTYPE
  OnSimpleVolumeChanged (float NewVolume, BOOL NewMute, LPCGUID EventContext)  override {
    UNREFERENCED_PARAMETER (NewVolume);
    UNREFERENCED_PARAMETER (NewMute);
    UNREFERENCED_PARAMETER (EventContext);

    return S_OK;
  };

  HRESULT
  STDMETHODCALLTYPE
  OnChannelVolumeChanged (DWORD ChannelCount, float NewChannelVolumeArray[  ], DWORD ChangedChannel, LPCGUID EventContext)  override {
    // TODO
    UNREFERENCED_PARAMETER (ChannelCount);
    UNREFERENCED_PARAMETER (NewChannelVolumeArray);
    UNREFERENCED_PARAMETER (ChangedChannel);
    UNREFERENCED_PARAMETER (EventContext);

    return S_OK;
  };

  HRESULT
  STDMETHODCALLTYPE
  OnGroupingParamChanged (LPCGUID NewGroupingParam, LPCGUID EventContext)  override {
    UNREFERENCED_PARAMETER (NewGroupingParam);
    UNREFERENCED_PARAMETER (EventContext);

    return S_OK;
  }

  HRESULT
  STDMETHODCALLTYPE
  OnStateChanged (AudioSessionState NewState)  override;

  HRESULT
  STDMETHODCALLTYPE
  OnSessionDisconnected (AudioSessionDisconnectReason DisconnectReason)  override;

  virtual
 ~SK_WASAPI_AudioSession (void) noexcept (false)
  {
    if (control_ != nullptr)
      control_->UnregisterAudioSessionNotification (this);

    control_ = nullptr;
  };

protected:
private:
  volatile LONG                     refs_;
  SK_ComPtr <IAudioSessionControl2> control_;
  std::string                       app_name_;
  SK_WASAPI_SessionManager*         parent_;
};

class SK_WASAPI_SessionManager : public IAudioSessionNotification
{
public:
  SK_WASAPI_SessionManager (void) noexcept : refs_ (1) {

  };

  virtual
  ~SK_WASAPI_SessionManager (void) noexcept (false)
  {
    if (session_mgr_ != nullptr)
      session_mgr_->UnregisterSessionNotification (this);
  }

  void Deactivate (void)
  {
    meter_info_   = nullptr;
    endpoint_vol_ = nullptr;
    auto_gain_    = nullptr;
    loudness_     = nullptr;
  }

  void Activate (void)
  {
    if (meter_info_ == nullptr)
      sessions_.clear ();

    else
      return;

    meter_info_.Attach (
      SK_WASAPI_GetAudioMeterInfo ().Detach ()
    );

    if (meter_info_ != nullptr && (! sessions_.empty ()))
      return;

    SK_ComPtr <IMMDeviceEnumerator> pDevEnum;
    if (FAILED ((pDevEnum.CoCreateInstance (__uuidof (MMDeviceEnumerator)))))
      return;

    // Most game audio a user will not want to hear while a game is in the
    //   background will pass through eConsole.
    //
    //   eCommunication will be headset stuff and that's something a user is not
    //     going to appreciate having muted :) Consider overloading this function
    //       to allow independent control.
    //
    SK_ComPtr <IMMDevice> pDevice;
    if ( FAILED (
           pDevEnum->GetDefaultAudioEndpoint ( eRender,
                                                 eMultimedia,
                                                   &pDevice )
                )
       ) return;

    if (FAILED (pDevice->Activate (
                  __uuidof (IAudioSessionManager2),
                    CLSCTX_ALL,
                      nullptr,
                        reinterpret_cast <void **>(&session_mgr_)
               )
           )
       ) return;

    SK_ComPtr <IAudioSessionEnumerator> pSessionEnum;
    if (FAILED (session_mgr_->GetSessionEnumerator (&pSessionEnum)))
      return;

    int num_sessions;

    if (FAILED (pSessionEnum->GetCount (&num_sessions)))
      return;

    for (int i = 0; i < num_sessions; i++)
    {
      SK_ComPtr <IAudioSessionControl> pSessionCtl;
      if (FAILED (pSessionEnum->GetSession (i, &pSessionCtl)))
        continue;

      SK_IAudioSessionControl2 pSessionCtl2;
      if (FAILED (pSessionCtl->QueryInterface <IAudioSessionControl2> (&pSessionCtl2.p)))
        continue;

      DWORD dwProcess = 0;
      if (FAILED (pSessionCtl2->GetProcessId (&dwProcess))) {
        continue;
      }

      AudioSessionState state;

      if (SUCCEEDED (pSessionCtl2->GetState (&state)))
      {
        auto* pSession =
          new SK_WASAPI_AudioSession (pSessionCtl2, this);

        sessions_.emplace (pSession);

        if (state == AudioSessionStateActive)
          active_sessions_.data.emplace (pSession);
        else if (state == AudioSessionStateInactive)
          inactive_sessions_.data.emplace (pSession);

        if (! active_sessions_.data.empty ())
        {
          active_sessions_.view =
            std::vector <SK_WASAPI_AudioSession *> ( active_sessions_.data.cbegin (),
                                                     active_sessions_.data.cend   () );
        }
        else
          active_sessions_.view.clear ();

        if (! inactive_sessions_.data.empty ())
        {
          inactive_sessions_.view =
            std::vector <SK_WASAPI_AudioSession *> ( inactive_sessions_.data.cbegin (),
                                                     inactive_sessions_.data.cend   () );
        }
        else
          inactive_sessions_.view.clear ();
      }
    }

    session_mgr_->RegisterSessionNotification (this);

    endpoint_vol_.Attach (SK_MMDev_GetEndpointVolumeControl ().Detach ());
    auto_gain_.   Attach (SK_MMDev_GetAutoGainControl       ().Detach ());
    loudness_.    Attach (SK_MMDev_GetLoudness              ().Detach ());
  }

  // IUnknown
  HRESULT
  STDMETHODCALLTYPE
  QueryInterface (REFIID riid, void **ppv) override
  {
    if (! ppv)
      return E_INVALIDARG;

    if (IID_IUnknown == riid)
    {
      AddRef ();
      *ppv = (IUnknown *)this;
    }

    else if (__uuidof (IAudioSessionNotification) == riid)
    {
      AddRef ();
      *ppv = (IAudioSessionNotification *)this;
    }

    else
    {
      *ppv = nullptr;
      return E_NOINTERFACE;
    }

    return S_OK;
  }

  ULONG STDMETHODCALLTYPE AddRef (void) noexcept override
  {
    return
      InterlockedIncrement (&refs_);
  }

  ULONG STDMETHODCALLTYPE Release (void) noexcept override
  {
    const ULONG ulRef =
      InterlockedDecrement (&refs_);

    if (ulRef == 0)
      delete this;

    return ulRef;
  }

  SK_IAudioMeterInformation getMeterInfo (void)
  {
    return meter_info_.p;
  }

  SK_WASAPI_AudioSession** getActive   (int* pCount = nullptr) noexcept
  {
    if (pCount)
      *pCount = (int)active_sessions_.view.size ();

    return active_sessions_.view.data ();
  }

  SK_WASAPI_AudioSession** getInactive (int* pCount = nullptr) noexcept
  {
    if (pCount)
      *pCount = sk::narrow_cast <int> (inactive_sessions_.view.size ());

    return
      inactive_sessions_.view.data ();
  }

  HRESULT
  STDMETHODCALLTYPE
  OnSessionCreated (IAudioSessionControl *pNewSession) override
  {
    if (pNewSession)
    {
      pNewSession->AddRef ();

      SK_IAudioSessionControl2                                             pSessionCtl2;
      if (SUCCEEDED (pNewSession->QueryInterface <IAudioSessionControl2> (&pSessionCtl2.p)))
      {
        DWORD dwProcess = 0;
        if (SUCCEEDED (pSessionCtl2->GetProcessId (&dwProcess)))
        {
          auto* pSession =
            new SK_WASAPI_AudioSession (pSessionCtl2, this);

          sessions_.emplace (pSession);

          AudioSessionState state = AudioSessionStateExpired;
          pSessionCtl2->GetState (&state);

          if (state == AudioSessionStateActive)
            active_sessions_.data.emplace (pSession);
          else if (state == AudioSessionStateInactive)
            inactive_sessions_.data.emplace (pSession);


          if (! active_sessions_.data.empty ())
          {
            active_sessions_.view =
              std::vector <SK_WASAPI_AudioSession *> ( active_sessions_.data.cbegin (),
                                                       active_sessions_.data.cend   () );
          }
          else
            active_sessions_.view.clear ();

          if (! inactive_sessions_.data.empty ())
          {
            inactive_sessions_.view =
              std::vector <SK_WASAPI_AudioSession *> ( inactive_sessions_.data.cbegin (),
                                                       inactive_sessions_.data.cend   () );
          }
          else
            inactive_sessions_.view.clear ();
        }
      }
    }

    return S_OK;
  }

protected:
  friend class SK_WASAPI_AudioSession;

  void SetSessionState (SK_WASAPI_AudioSession* pSession, AudioSessionState state)
  {
    switch (state)
    {
      case AudioSessionStateExpired:
        if (inactive_sessions_.data.count (pSession))
            inactive_sessions_.data.erase (pSession);
        else if (active_sessions_.data.count (pSession))
                 active_sessions_.data.erase (pSession);
        break;

      case AudioSessionStateActive:
        if (inactive_sessions_.data.count   (pSession))
            inactive_sessions_.data.erase   (pSession);
              active_sessions_.data.emplace (pSession);
        break;

      case AudioSessionStateInactive:
        if (active_sessions_.data.count (pSession))
            active_sessions_.data.erase (pSession);
        inactive_sessions_.data.emplace (pSession);
        break;
    }

    if (! active_sessions_.data.empty ())
    {
      active_sessions_.view =
        std::vector <SK_WASAPI_AudioSession *> ( active_sessions_.data.cbegin (),
                                                 active_sessions_.data.cend   () );
    }
    else
      active_sessions_.view.clear ();

    if (! inactive_sessions_.data.empty ())
    {
      inactive_sessions_.view =
        std::vector <SK_WASAPI_AudioSession *> ( inactive_sessions_.data.cbegin (),
                                                 inactive_sessions_.data.cend   () );
    }
    else
      inactive_sessions_.view.clear ();
  }

  void RemoveSession (SK_WASAPI_AudioSession* pSession)
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



private:
    volatile LONG                      refs_;
    std::set <SK_WASAPI_AudioSession*> sessions_;

    struct {
      using session_set_t         =
               std::set <   SK_WASAPI_AudioSession *>;
            session_set_t    data = {  };

      using session_vec_t         =
               std::vector <SK_WASAPI_AudioSession *>;
            session_vec_t    view = {  };
    }                                  active_sessions_,
                                       inactive_sessions_;

    SK_IAudioSessionManager2           session_mgr_;
    SK_IAudioMeterInformation          meter_info_;
    SK_IAudioEndpointVolume            endpoint_vol_;
    SK_IAudioLoudness                  loudness_;
    SK_IAudioAutoGainControl           auto_gain_;
};

struct SK_WASAPI_LatencyPeriod
{
  float    milliseconds;
  uint32_t frames;
};

SK_WASAPI_LatencyPeriod __stdcall SK_WASAPI_GetCurrentLatency (void);
SK_WASAPI_LatencyPeriod __stdcall SK_WASAPI_GetDefaultLatency (void);
SK_WASAPI_LatencyPeriod __stdcall SK_WASAPI_GetMinimumLatency (void);
SK_WASAPI_LatencyPeriod __stdcall SK_WASAPI_GetMaximumLatency (void);
SK_WASAPI_LatencyPeriod __stdcall SK_WASAPI_SetLatency        (SK_WASAPI_LatencyPeriod latency);

#endif /* __SK__SOUND_H__ */