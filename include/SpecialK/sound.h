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

#include <SpecialK/window.h>

#include <Mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>

void                    __stdcall SK_SetGameMute                    (bool bMute);

IAudioMeterInformation* __stdcall SK_WASAPI_GetAudioMeterInfo       (void);
ISimpleAudioVolume*     __stdcall SK_WASAPI_GetVolumeControl        (DWORD   proc_id = GetCurrentProcessId ());
IChannelAudioVolume*    __stdcall SK_WASAPI_GetChannelVolumeControl (DWORD   proc_id = GetCurrentProcessId ());
void                    __stdcall SK_WASAPI_GetAudioSessionProcs    (size_t* count, DWORD* procs = nullptr);

const char*             __stdcall SK_WASAPI_GetChannelName          (int channel_idx);


#include <SpecialK/log.h>
#include <SpecialK/steam_api.h>

#include <atlbase.h>
#include <TlHelp32.h>

#include <unordered_map>
#include <set>

class SK_WASAPI_SessionManager;

class SK_WASAPI_AudioSession : public IAudioSessionEvents
{
public:
  SK_WASAPI_AudioSession (IAudioSessionControl2* pSession, SK_WASAPI_SessionManager* pParent) :
                                       control_ (pSession),                parent_  (pParent), refs_ (1)
  {
    pSession->RegisterAudioSessionNotification (this);

    DWORD proc_id = getProcessId ();

    char     szTitle [512] = { };
    wchar_t wszTitle [512] = { };
    
    window_t win = SK_FindRootWindow (proc_id);
    if (win.root != 0)
    { 
      // This is all happening from the application's message pump in most games,
      //   so this specialized function avoids deadlocking the pump.
      InternalGetWindowText (win.root, wszTitle, 511);
      WideCharToMultiByte   (CP_UTF8, 0x00, wszTitle, (int)wcslen (wszTitle), szTitle, 511, nullptr, FALSE);

      //SK_LOG4 ( ( L" Audio Session (pid=%lu)", proc_id ),
                  //L"  WASAPI  " );
    }

// Use the ANSI versions
#undef PROCESSENTRY32
#undef Process32First
#undef Process32Next

    // Use the exeuctable name if there is no window name
    if (! strlen (szTitle))
    {
      HANDLE hSnap =
        CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);

      if (hSnap)
      {
        PROCESSENTRY32 pent;
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

        CloseHandle (hSnap);
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

  ISimpleAudioVolume* getSimpleAudioVolume (void)
  {
    ISimpleAudioVolume* pRet = nullptr;

    if (SUCCEEDED (control_->QueryInterface <ISimpleAudioVolume> (&pRet)))
      return pRet;

    return nullptr;
  }

  IChannelAudioVolume* getChannelAudioVolume (void)
  {
    IChannelAudioVolume* pRet = nullptr;

    if (SUCCEEDED (control_->QueryInterface <IChannelAudioVolume> (&pRet)))
      return pRet;

    return nullptr;
  }

  DWORD getProcessId (void)
  {
    DWORD dwProcId = 0;

    if (FAILED (control_->GetProcessId (&dwProcId)))
      return 0;

    return dwProcId;
  }

  const char* getName (void) { return app_name_.c_str (); };

  // IUnknown
  HRESULT
  STDMETHODCALLTYPE
  QueryInterface (REFIID riid, void **ppv)
  {    
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
  
  ULONG STDMETHODCALLTYPE AddRef (void)
  {
    return InterlockedIncrement (&refs_);
  }
   
  ULONG STDMETHODCALLTYPE Release (void)
  {
    ULONG ulRef = InterlockedDecrement (&refs_);

    if (ulRef == 0) 
    {
      delete this;
    }

    return ulRef;
  }

  HRESULT
  STDMETHODCALLTYPE
  OnDisplayNameChanged (PCWSTR NewDisplayName, LPCGUID EventContext) {
    UNREFERENCED_PARAMETER (NewDisplayName);
    UNREFERENCED_PARAMETER (EventContext);

    return S_OK;
  };
        
  HRESULT
  STDMETHODCALLTYPE
  OnIconPathChanged (LPCWSTR NewIconPath, LPCGUID EventContext) {
    UNREFERENCED_PARAMETER (NewIconPath);
    UNREFERENCED_PARAMETER (EventContext);

    return S_OK;
  };
        
  HRESULT
  STDMETHODCALLTYPE
  OnSimpleVolumeChanged (float NewVolume, BOOL NewMute, LPCGUID EventContext) {
    UNREFERENCED_PARAMETER (NewVolume);
    UNREFERENCED_PARAMETER (NewMute);
    UNREFERENCED_PARAMETER (EventContext);

    return S_OK;
  };
      
  HRESULT
  STDMETHODCALLTYPE
  OnChannelVolumeChanged (DWORD ChannelCount, float NewChannelVolumeArray[  ], DWORD ChangedChannel, LPCGUID EventContext) {
    // TODO
    UNREFERENCED_PARAMETER (ChannelCount);
    UNREFERENCED_PARAMETER (NewChannelVolumeArray);
    UNREFERENCED_PARAMETER (ChangedChannel);
    UNREFERENCED_PARAMETER (EventContext);

    return S_OK;
  };
      
  HRESULT
  STDMETHODCALLTYPE
  OnGroupingParamChanged (LPCGUID NewGroupingParam, LPCGUID EventContext) {
    UNREFERENCED_PARAMETER (NewGroupingParam);
    UNREFERENCED_PARAMETER (EventContext);

    return S_OK;
  }

  HRESULT
  STDMETHODCALLTYPE
  OnStateChanged (AudioSessionState NewState);
        
  HRESULT
  STDMETHODCALLTYPE
  OnSessionDisconnected (AudioSessionDisconnectReason DisconnectReason);

  ~SK_WASAPI_AudioSession (void)
  {
    if (control_ != nullptr)
      control_->UnregisterAudioSessionNotification (this);

    control_ = nullptr;
  };

protected:
private:
  volatile LONG                   refs_;
  CComPtr <IAudioSessionControl2> control_;
  std::string                     app_name_;
  SK_WASAPI_SessionManager*       parent_;
};

class SK_WASAPI_SessionManager : public IAudioSessionNotification
{
public:
  SK_WASAPI_SessionManager (void) : refs_ (1) {

  };

  ~SK_WASAPI_SessionManager (void)
  {
    if (session_mgr_ != nullptr)
      session_mgr_->UnregisterSessionNotification (this);
  }

  void Deactivate (void)
  {
    meter_info_ = nullptr;
  }

  void Activate (void)
  {
    if (meter_info_ == nullptr)
      sessions_.clear ();

    else
      return;

    CComPtr <IAudioMeterInformation> pMeterInfo =
      SK_WASAPI_GetAudioMeterInfo ();

    meter_info_ = pMeterInfo;

    if (meter_info_ != nullptr && sessions_.size ())
      return;

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

    CComPtr <IAudioSessionEnumerator> pSessionEnum;
    if (FAILED (session_mgr_->GetSessionEnumerator (&pSessionEnum)))
      return;

    int num_sessions;

    if (FAILED (pSessionEnum->GetCount (&num_sessions)))
      return;

    for (int i = 0; i < num_sessions; i++)
    {
      CComPtr <IAudioSessionControl> pSessionCtl;
      if (FAILED (pSessionEnum->GetSession (i, &pSessionCtl)))
        continue;

      IAudioSessionControl2* pSessionCtl2;
      if (FAILED (pSessionCtl->QueryInterface <IAudioSessionControl2> (&pSessionCtl2)))
        continue;

      DWORD dwProcess = 0;
      if (FAILED (pSessionCtl2->GetProcessId (&dwProcess))) {
        pSessionCtl2->Release ();
        continue;
      }

      AudioSessionState state;

      if (SUCCEEDED (pSessionCtl2->GetState (&state)))
      {
        SK_WASAPI_AudioSession* pSession = new SK_WASAPI_AudioSession (pSessionCtl2, this);

        sessions_.emplace (pSession);

        if (state == AudioSessionStateActive)
          active_sessions_.data.emplace (pSession);
        else if (state == AudioSessionStateInactive)
          inactive_sessions_.data.emplace (pSession);

        active_sessions_.view =
          std::vector <SK_WASAPI_AudioSession *> ( active_sessions_.data.begin (), active_sessions_.data.end () );
        inactive_sessions_.view =
          std::vector <SK_WASAPI_AudioSession *> ( inactive_sessions_.data.begin (), inactive_sessions_.data.end () );
      }
    }

    session_mgr_->RegisterSessionNotification (this);
  }

  // IUnknown
  HRESULT
  STDMETHODCALLTYPE
  QueryInterface (REFIID riid, void **ppv)
  {    
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
  
  ULONG STDMETHODCALLTYPE AddRef (void)
  {
    return InterlockedIncrement (&refs_);
  }
   
  ULONG STDMETHODCALLTYPE Release (void)
  {
    ULONG ulRef = InterlockedDecrement (&refs_);

    if (ulRef == 0)
      delete this;

    return ulRef;
  }

  IAudioMeterInformation* getMeterInfo (void)
  {
    return meter_info_;
  }

  SK_WASAPI_AudioSession** getActive   (int* pCount = nullptr)
  {
    if (pCount)
      *pCount = (int)active_sessions_.view.size ();

    return active_sessions_.view.data ();
  }

  SK_WASAPI_AudioSession** getInactive (int* pCount = nullptr)
  {
    if (pCount)
      *pCount = (int)inactive_sessions_.view.size ();

    return inactive_sessions_.view.data ();
  }
  
  HRESULT
  STDMETHODCALLTYPE
  OnSessionCreated (IAudioSessionControl *pNewSession)
  {
    if (pNewSession)
    {
      pNewSession->AddRef ();

      CComPtr <IAudioSessionControl2> pSessionCtl2;
      if (SUCCEEDED (pNewSession->QueryInterface <IAudioSessionControl2> (&pSessionCtl2)))
      {
        DWORD dwProcess = 0;
        if (SUCCEEDED (pSessionCtl2->GetProcessId (&dwProcess)))
        {
          SK_WASAPI_AudioSession* pSession = new SK_WASAPI_AudioSession (pSessionCtl2, this);

          sessions_.emplace (pSession);

          AudioSessionState state = AudioSessionStateExpired;
          pSessionCtl2->GetState (&state);

          if (state == AudioSessionStateActive)
            active_sessions_.data.emplace (pSession);
          else if (state == AudioSessionStateInactive)
            inactive_sessions_.data.emplace (pSession);

          active_sessions_.view =
            std::vector <SK_WASAPI_AudioSession *> ( active_sessions_.data.begin (), active_sessions_.data.end () );
          inactive_sessions_.view =
            std::vector <SK_WASAPI_AudioSession *> ( inactive_sessions_.data.begin (), inactive_sessions_.data.end () );
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
        if (inactive_sessions_.data.count (pSession))
          inactive_sessions_.data.erase (pSession);
        active_sessions_.data.emplace (pSession);
        break;

      case AudioSessionStateInactive:
        if (active_sessions_.data.count (pSession))
          active_sessions_.data.erase (pSession);
        inactive_sessions_.data.emplace (pSession);
        break;
    }

    active_sessions_.view =
      std::vector <SK_WASAPI_AudioSession *> ( active_sessions_.data.begin (), active_sessions_.data.end () );
    inactive_sessions_.view =
      std::vector <SK_WASAPI_AudioSession *> ( inactive_sessions_.data.begin (), inactive_sessions_.data.end () );
  }

  void RemoveSession (SK_WASAPI_AudioSession* pSession)
  {
    SetSessionState (pSession, AudioSessionStateExpired);

    if (sessions_.count (pSession))
    {
      sessions_.erase   (pSession);
      pSession->Release ();
    }
  }
  


private:
    volatile LONG                                  refs_;
    std::set <SK_WASAPI_AudioSession*>             sessions_;

    struct {
      std::set    <SK_WASAPI_AudioSession *> data;
      std::vector <SK_WASAPI_AudioSession *> view;
    } active_sessions_, inactive_sessions_;
    
    CComPtr <IAudioSessionManager2>                session_mgr_;
    CComPtr <IAudioMeterInformation>               meter_info_;
};



#endif /* __SK__SOUND_H__ */