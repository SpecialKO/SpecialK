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
#include <Functiondiscoverykeys_devpkey.h>
#include <roapi.h>
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


#include <winstring.h>

interface DECLSPEC_UUID ("ab3d4648-e242-459f-b02f-541c70306324") IAudioPolicyConfigFactory;
interface DECLSPEC_UUID ("2a59116d-6c4f-45e0-a74f-707e3fef9258") IAudioPolicyConfigFactoryLegacy;
interface IAudioPolicyConfigFactory
{
public:
  virtual HRESULT STDMETHODCALLTYPE __incomplete__QueryInterface                    (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__AddRef                            (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__Release                           (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__GetIids                           (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__GetRuntimeClassName               (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__GetTrustLevel                     (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__add_CtxVolumeChange               (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__remove_CtxVolumeChanged           (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__add_RingerVibrateStateChanged     (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__remove_RingerVibrateStateChange   (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__SetVolumeGroupGainForId           (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__GetVolumeGroupGainForId           (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__GetActiveVolumeGroupForEndpointId (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__GetVolumeGroupsForEndpoint        (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__GetCurrentVolumeContext           (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__SetVolumeGroupMuteForId           (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__GetVolumeGroupMuteForId           (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__SetRingerVibrateState             (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__GetRingerVibrateState             (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__SetPreferredChatApplication       (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__ResetPreferredChatApplication     (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__GetPreferredChatApplication       (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__GetCurrentChatApplications        (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__add_ChatContextChanged            (void);
  virtual HRESULT STDMETHODCALLTYPE __incomplete__remove_ChatContextChanged         (void);
  
  virtual HRESULT STDMETHODCALLTYPE SetPersistedDefaultAudioEndpoint                ( UINT      processId,
                                                                                      EDataFlow flow,
                                                                                      ERole     role,
                                                                                      HSTRING   deviceId);

  virtual HRESULT STDMETHODCALLTYPE GetPersistedDefaultAudioEndpoint                ( UINT      processId,
                                                                                      EDataFlow flow,
                                                                                      int       role,
                                     _Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING* string );

  virtual HRESULT STDMETHODCALLTYPE ClearAllPersistedApplicationDefaultEndpoints    (void);
};

class SK_WASAPI_EndPointManager : public IMMNotificationClient
{
private:
  void resetSessionManager (void)
  {
    extern void SK_WASAPI_ResetSessionManager (void);
                SK_WASAPI_ResetSessionManager ();
  }

public:
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

    else if (__uuidof (IMMNotificationClient) == riid)
    {
      AddRef ();
      *ppv = (IMMNotificationClient *)this;
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

  HRESULT STDMETHODCALLTYPE OnDeviceStateChanged (_In_ LPCWSTR pwstrDeviceId, _In_ DWORD dwNewState) override
  {
    std::ignore = pwstrDeviceId;
    std::ignore = dwNewState;

    resetSessionManager ();

    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE OnDeviceAdded (_In_ LPCWSTR pwstrDeviceId) override
  {
    std::ignore = pwstrDeviceId;

    resetSessionManager ();

    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE OnDeviceRemoved (_In_ LPCWSTR pwstrDeviceId) override
  {
    std::ignore = pwstrDeviceId;

    resetSessionManager ();

    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged (_In_ EDataFlow flow, _In_ ERole role, _In_ LPCWSTR pwstrDefaultDeviceId) override
  {
    std::ignore = role;
    std::ignore = pwstrDefaultDeviceId;

    if (flow == eAll || flow == eRender)
    {
      resetSessionManager ();
    }

    return S_OK;
  }
  
  HRESULT STDMETHODCALLTYPE OnPropertyValueChanged (_In_ LPCWSTR pwstrDeviceId, _In_ const PROPERTYKEY key) override
  {
    std::ignore = pwstrDeviceId;
    std::ignore = key;

    resetSessionManager ();

    return S_OK;
  }

  struct endpoint_s
  {
    DWORD        endpoint_state_ = DEVICE_STATE_NOTPRESENT;
    std::wstring endpoint_id_;
    std::string  friendly_name_;

    const wchar_t* describeState (void)
    {
      switch (endpoint_state_)
      {
        default:                      return L"Unknown";
        case DEVICE_STATE_ACTIVE:     return L"Active";
        case DEVICE_STATE_DISABLED:   return L"Disabled";
        case DEVICE_STATE_NOTPRESENT: return L"Not Present";
        case DEVICE_STATE_UNPLUGGED:  return L"Unplugged";
      }
    }
  };

protected:
  IAudioPolicyConfigFactory *policy_cfg_factory = nullptr;

  std::vector <endpoint_s> render_devices_;
  std::vector <endpoint_s> capture_devices_;

  volatile LONG            refs_;

public:
  SK_WASAPI_EndPointManager (void) : refs_ (1)
  {
    initAudioPolicyConfigFactory ();

    Activate ();
  }

  virtual ~SK_WASAPI_EndPointManager (void)
  {
  }

  void Activate (void)
  {
    SK_ComPtr <IMMDeviceEnumerator> pDevEnum;
    if (FAILED ((pDevEnum.CoCreateInstance (__uuidof (MMDeviceEnumerator)))))
      return;

    SK_ComPtr <IMMDeviceCollection>                                dev_collection;
    pDevEnum->EnumAudioEndpoints (eRender,  DEVICE_STATEMASK_ALL, &dev_collection.p);

    SK_RunOnce (pDevEnum->RegisterEndpointNotificationCallback (this));

    if (dev_collection.p != nullptr)
    {
      render_devices_.clear ();

      UINT                       uiDevices = 0;
      dev_collection->GetCount (&uiDevices);

      for ( UINT i = 0 ; i < uiDevices ; ++i )
      {
        endpoint_s endpoint;

        SK_ComPtr <IMMDevice>     pDevice;
        dev_collection->Item (i, &pDevice.p);

        if (pDevice.p != nullptr)
        {
          wchar_t*         wszId = nullptr;
          pDevice->GetId (&wszId);

          if (wszId != nullptr)
          {
            SK_ComPtr <IPropertyStore>              props;
            pDevice->OpenPropertyStore (STGM_READ, &props.p);

            if (props.p != nullptr)
            {
              PROPVARIANT       propvarName;
              PropVariantInit (&propvarName);

              props->GetValue (PKEY_Device_FriendlyName, &propvarName);

              endpoint.endpoint_id_   = wszId;
              endpoint.friendly_name_ = SK_WideCharToUTF8 (propvarName.pwszVal);

              pDevice->GetState (&endpoint.endpoint_state_);

              PropVariantClear (&propvarName);

              render_devices_.emplace_back (endpoint);
            }

            CoTaskMemFree (wszId);
          }
        }
      }
    }
    //pDevEnum->EnumAudioEndpoints (eCapture, DEVICE_STATEMASK_ALL, &dev_collection.p);
  }

  static std::wstring MMDEVAPI_DEVICE_PREFIX; 
  static std::wstring MMDEVAPI_RENDER_POSTFIX;
  static std::wstring MMDEVAPI_CAPTURE_POSTFIX;

  bool initAudioPolicyConfigFactory (void)
  {
    static const wchar_t* name = L"Windows.Media.Internal.AudioPolicyConfig";
           const UINT32   len  = (UINT32)wcslen (name);

    HSTRING        hClassName = nullptr;
    HSTRING_HEADER header;
    HRESULT        hr =
      WindowsCreateStringReference (name, len, &header, &hClassName);

    if (FAILED (hr))
    {
      WindowsDeleteString (hClassName);

      return false;
    }

    hr =
      RoGetActivationFactory   (hClassName, __uuidof (IAudioPolicyConfigFactory),       (void **)&policy_cfg_factory);

    if (hr == E_NOINTERFACE)
    {
      hr =
        RoGetActivationFactory (hClassName, __uuidof (IAudioPolicyConfigFactoryLegacy), (void **)&policy_cfg_factory);
    }

    WindowsDeleteString        (hClassName);

    return
      SUCCEEDED (hr);
  }

  size_t      getNumRenderEndpoints  (void) const { return render_devices_.size (); }
  endpoint_s& getRenderEndpoint      (UINT idx)   { static endpoint_s invalid = {}; return idx < render_devices_.size  () ? render_devices_  [idx] : invalid; }

  size_t      getNumCaptureEndpoints (void) const { return capture_devices_.size  (); }
  endpoint_s& getCaptureEndpoint     (UINT idx)   { static endpoint_s invalid = {}; return idx < capture_devices_.size () ? capture_devices_ [idx] : invalid; }

  bool setPersistedDefaultAudioEndpoint (int pid, EDataFlow flow, std::wstring deviceId)
  {
    if (policy_cfg_factory == nullptr)
      return false;

    HSTRING hDeviceId = nullptr;

    if (! deviceId.empty ())
    {
      std::wstring fullDeviceId (
        MMDEVAPI_DEVICE_PREFIX + deviceId + (flow == eRender ?
                                     MMDEVAPI_RENDER_POSTFIX : MMDEVAPI_CAPTURE_POSTFIX)
                                );

      auto hr =
        WindowsCreateString ( fullDeviceId.c_str  (),
                      (UINT32)fullDeviceId.length (), &hDeviceId );

      if (FAILED (hr))
        return false;
    }

    auto hrConsole    = policy_cfg_factory->SetPersistedDefaultAudioEndpoint (pid, flow, eConsole,    hDeviceId);
    auto hrMultimedia = policy_cfg_factory->SetPersistedDefaultAudioEndpoint (pid, flow, eMultimedia, hDeviceId);

    return
      SUCCEEDED (hrConsole) &&
      SUCCEEDED (hrMultimedia);
  }

  std::wstring getPersistedDefaultAudioEndpoint (int pid, EDataFlow flow)
  {
    if (policy_cfg_factory == nullptr)
      return std::wstring ();

    HSTRING hDeviceId = nullptr;

    policy_cfg_factory->GetPersistedDefaultAudioEndpoint (pid, flow, eMultimedia | eConsole, &hDeviceId);

    UINT32                                   len;
    return
      WindowsGetStringRawBuffer (hDeviceId, &len);
  }
};

SK_LazyGlobal <SK_WASAPI_EndPointManager> SK_WASAPI_EndPointMgr;


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
  SK_WASAPI_SessionManager (void) noexcept : refs_ (1)
  {
  };

  virtual
  ~SK_WASAPI_SessionManager (void) noexcept (false)
  {
    Deactivate ();
  }

  void Deactivate (void)
  {
    std::scoped_lock <SK_Thread_HybridSpinlock> lock (deactivation_lock_);

    if (session_mgr_ != nullptr)
        session_mgr_->UnregisterSessionNotification (this);

    for ( auto& session : sessions_ )
    {
      session->Release ();
    }

    sessions_.clear ();

    active_sessions_.data.clear ();
    active_sessions_.view.clear ();

    inactive_sessions_.data.clear ();
    inactive_sessions_.view.clear ();

    meter_info_   = nullptr;
    endpoint_vol_ = nullptr;
    auto_gain_    = nullptr;
    loudness_     = nullptr;
    audio_client_ = nullptr;
  }

  void Activate (void)
  {
    std::scoped_lock <SK_Thread_HybridSpinlock> lock (activation_lock_);

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
    audio_client_.Attach (SK_WASAPI_GetAudioClient          ().Detach ());

    SK_WASAPI_EndPointMgr->Activate ();
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

  SK_Thread_HybridSpinlock           activation_lock_;
  SK_Thread_HybridSpinlock           deactivation_lock_;

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
  SK_IAudioClient3                   audio_client_;
};

struct SK_WASAPI_AudioLatency
{
  float    milliseconds;
  uint32_t frames;
  uint32_t samples_per_sec;
};

SK_WASAPI_AudioLatency __stdcall SK_WASAPI_GetCurrentLatency (void);
SK_WASAPI_AudioLatency __stdcall SK_WASAPI_GetDefaultLatency (void);
SK_WASAPI_AudioLatency __stdcall SK_WASAPI_GetMinimumLatency (void);
SK_WASAPI_AudioLatency __stdcall SK_WASAPI_GetMaximumLatency (void);
SK_WASAPI_AudioLatency __stdcall SK_WASAPI_SetLatency        (SK_WASAPI_AudioLatency latency);

#endif /* __SK__SOUND_H__ */