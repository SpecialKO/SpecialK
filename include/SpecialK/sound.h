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

#include <span>

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
    EDataFlow    endpoint_flow_  = eRender;
    DWORD        endpoint_state_ = DEVICE_STATE_NOTPRESENT;
    std::wstring endpoint_id_;
    std::string  friendly_name_;

    std::wstring getDeviceId (void)
    {
      return
        SK_FormatStringW ( L"%ws%ws%ws", MMDEVAPI_DEVICE_PREFIX, endpoint_id_.c_str (),
             endpoint_flow_ == eRender ? MMDEVAPI_RENDER_POSTFIX
                                       : MMDEVAPI_CAPTURE_POSTFIX );
    }

    bool isSameDevice (const wchar_t* wszShortDeviceId)
    {
      if (! wszShortDeviceId)
        return false;

      if (*wszShortDeviceId == L'\0')
      {
        return endpoint_id_.empty ();
      }

      return
        StrStrIW (getDeviceId ().c_str (), wszShortDeviceId) != nullptr;
    }

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
  SK_Thread_HybridSpinlock  activation_lock_;

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
    std::scoped_lock <SK_Thread_HybridSpinlock> lock0 (activation_lock_);

    try
    {
      SK_ComPtr <IMMDeviceEnumerator> pDevEnum;

      ThrowIfFailed (
        pDevEnum.CoCreateInstance (__uuidof (MMDeviceEnumerator)));

      SK_RunOnce (
        pDevEnum->RegisterEndpointNotificationCallback (this)
      );

      SK_ComPtr <IMMDeviceCollection>                                dev_collection;
      ThrowIfFailed (
        pDevEnum->EnumAudioEndpoints (eRender, DEVICE_STATE_ACTIVE, &dev_collection.p));

      if (dev_collection.p != nullptr)
      {
        render_devices_.clear ();

        UINT                         uiDevices = 0;
        ThrowIfFailed (
          dev_collection->GetCount (&uiDevices));

        for ( UINT i = 0 ; i < uiDevices ; ++i )
        {
          endpoint_s endpoint;

          SK_ComPtr <IMMDevice>       pDevice;
          ThrowIfFailed (
            dev_collection->Item (i, &pDevice.p));

          if (pDevice.p != nullptr)
          {
            wchar_t*           wszId = nullptr;
            ThrowIfFailed (
              pDevice->GetId (&wszId));

            if (wszId != nullptr)
            {
              SK_ComPtr <IPropertyStore>                props;
              ThrowIfFailed (
                pDevice->OpenPropertyStore (STGM_READ, &props.p));

              if (props.p != nullptr)
              {
                PROPVARIANT       propvarName;
                PropVariantInit (&propvarName);

                ThrowIfFailed (
                  props->GetValue (PKEY_Device_FriendlyName, &propvarName));

                endpoint.endpoint_flow_ = eRender;
                endpoint.endpoint_id_   = wszId;
                endpoint.friendly_name_ = SK_WideCharToUTF8 (propvarName.pwszVal);

                ThrowIfFailed (
                  pDevice->GetState (&endpoint.endpoint_state_));

                PropVariantClear (&propvarName);

                render_devices_.push_back (endpoint);
              }

              CoTaskMemFree (wszId);
            }
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

  static const wchar_t* MMDEVAPI_DEVICE_PREFIX; 
  static const wchar_t* MMDEVAPI_RENDER_POSTFIX;
  static const wchar_t* MMDEVAPI_CAPTURE_POSTFIX;

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
    
    // Fallback does not work.
#if 0
    if (hr == E_NOINTERFACE)
    {
      hr =
        RoGetActivationFactory (hClassName, __uuidof (IAudioPolicyConfigFactoryLegacy), (void **)&policy_cfg_factory);
    }
#endif

    WindowsDeleteString        (hClassName);

    return
      SUCCEEDED (hr);
  }

  size_t      getNumRenderEndpoints  (void) const { return render_devices_.size (); }
  endpoint_s& getRenderEndpoint      (UINT idx)   { static endpoint_s invalid = {}; return idx < render_devices_.size  () ? render_devices_  [idx] : invalid; }

  size_t      getNumCaptureEndpoints (void) const { return capture_devices_.size  (); }
  endpoint_s& getCaptureEndpoint     (UINT idx)   { static endpoint_s invalid = {}; return idx < capture_devices_.size () ? capture_devices_ [idx] : invalid; }

  bool setPersistedDefaultAudioEndpoint (int pid, EDataFlow flow, std::wstring_view deviceId)
  {
    if (policy_cfg_factory == nullptr)
      return false;

    HSTRING         hDeviceId = nullptr;
    std::wstring fullDeviceId;

    if (! deviceId.empty ())
    {
      fullDeviceId =
        endpoint_s { .endpoint_flow_ = flow,
                     .endpoint_id_   = deviceId.data ()
        }.getDeviceId ();

      auto hr =
        WindowsCreateString ( fullDeviceId.c_str  (),
                      (UINT32)fullDeviceId.length (), &hDeviceId );

      if (FAILED (hr))
        return false;
    }

    HSTRING hExistingDeviceId = nullptr;

    policy_cfg_factory->GetPersistedDefaultAudioEndpoint (
      pid, flow, eMultimedia | eConsole, &hExistingDeviceId
    );

    UINT32                                           len;
    std::wstring existing_device =
      WindowsGetStringRawBuffer (hExistingDeviceId, &len);

    bool needs_change =
      !(         existing_device.empty () && fullDeviceId.empty ()) &&
      !StrStrIW (existing_device.c_str (),   fullDeviceId.c_str ());

    auto hrConsole    =
      needs_change ? policy_cfg_factory->SetPersistedDefaultAudioEndpoint (pid, flow, eConsole,    hDeviceId)
                   : S_OK;
    auto hrMultimedia =
      needs_change ? policy_cfg_factory->SetPersistedDefaultAudioEndpoint (pid, flow, eMultimedia, hDeviceId)
                   : S_OK;

    WindowsDeleteString (        hDeviceId);
    WindowsDeleteString (hExistingDeviceId);

    return
      SUCCEEDED (hrConsole) &&
      SUCCEEDED (hrMultimedia);
  }

  std::wstring getPersistedDefaultAudioEndpoint (int pid, EDataFlow flow)
  {
    if (policy_cfg_factory == nullptr)
      return std::wstring ();

    HSTRING hDeviceId = nullptr;

    policy_cfg_factory->GetPersistedDefaultAudioEndpoint (
      pid, flow, eMultimedia | eConsole, &hDeviceId
    );

    UINT32                                   len;
    std::wstring ret =
      WindowsGetStringRawBuffer (hDeviceId, &len);
      WindowsDeleteString       (hDeviceId);

    return
      ret;
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

      wchar_t*                   wszDisplayName = nullptr;
      pSession->GetDisplayName (&wszDisplayName);

      char szTitle [512]  =
      {                   };

      std::string_view     title_view (szTitle);
      SK_FormatStringView (title_view, "%ws", wszDisplayName);

      const DWORD proc_id =
             getProcessId ();

      // Use the window name if there is no session name
      if ('\0' == *szTitle)
      {
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
        if ('\0' == *szTitle)
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
      }

      if (proc_id == GetCurrentProcessId ())
      {
        if (SK::SteamAPI::AppName ().length ())
          app_name_ = SK::SteamAPI::AppName ();
      }

      if (app_name_.empty ())
          app_name_ = szTitle;

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
  OnDisplayNameChanged (PCWSTR NewDisplayName, LPCGUID EventContext)  override;

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
    if (             control_!=nullptr)
      std::exchange (control_, nullptr)->UnregisterAudioSessionNotification (this);
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
    reset_event_.m_h =
      SK_CreateEvent (nullptr, FALSE, FALSE, nullptr);
  };

  virtual
  ~SK_WASAPI_SessionManager (void) noexcept (false)
  {
    Deactivate ();
  }

  void signalReset (void)
  {
    SetEvent (reset_event_);
  }

  void Deactivate (void)
  {
    std::scoped_lock <SK_Thread_HybridSpinlock> lock (activation_lock_);

    for (auto& session_mgr : session_mgrs_)
    {
      if (session_mgr != nullptr)
      {
      //session_mgr->UnregisterSessionNotification (this);
        session_mgr = nullptr;
      }
    }

    meter_info_   = nullptr;
    endpoint_vol_ = nullptr;
    auto_gain_    = nullptr;
    loudness_     = nullptr;
    audio_client_ = nullptr;
  }

  void Activate (void)
  {
    if (reset_event_.isValid () && WaitForSingleObject (reset_event_, 0) != WAIT_TIMEOUT)
    {
      Deactivate ();
    }

    std::scoped_lock <SK_Thread_HybridSpinlock> lock (activation_lock_);

    if (meter_info_ == nullptr)
    {
      sessions_.clear ();

      active_sessions_.data.clear ();
      active_sessions_.view.clear ();

      inactive_sessions_.data.clear ();
      inactive_sessions_.view.clear ();
    }

    else
      return;

    meter_info_.Attach (
      SK_WASAPI_GetAudioMeterInfo ().Detach ()
    );

    if (meter_info_ != nullptr && (! sessions_.empty ()))
      return;

    session_mgr_ = nullptr;

    for ( auto& session_mgr : session_mgrs_ )
    {
      session_mgr = nullptr;
    }

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
    SK_ComPtr <IMMDevice> pDefaultDevice;
    if ( FAILED (
           pDevEnum->GetDefaultAudioEndpoint ( eRender,
                                                 eMultimedia,
                                                   &pDefaultDevice )
                )
       ) return;

    size_t dev_idx   = 0;
    size_t dev_count =
      SK_WASAPI_EndPointMgr->getNumRenderEndpoints ();

    wchar_t*                default_dev_id = nullptr;
    pDefaultDevice->GetId (&default_dev_id);

    for (auto &session_mgr : session_mgrs_)
    {
      if (dev_idx >= dev_count)
      {
        // Destroy any existing session managers for any
        //   inactive devices
        session_mgr = nullptr;
        break;
      }

      auto& endpoint =
        SK_WASAPI_EndPointMgr->getRenderEndpoint (dev_idx++);

      if (endpoint.isSameDevice (default_dev_id))
      {
        if (FAILED (pDefaultDevice->Activate (
                    __uuidof (IAudioSessionManager2),
                      CLSCTX_ALL,
                        nullptr,
                          reinterpret_cast <void **>(&session_mgr)
                 )
             )
         ) return;

        session_mgr_ = session_mgr;
      }

      else
      {
#if 1
        continue;
#else
        std::wstring device_id_str;

        if (! endpoint.endpoint_id_.empty ())
        {
          device_id_str =
            SK_FormatStringW ( L"%ws%ws%ws",
              SK_WASAPI_EndPointManager::MMDEVAPI_DEVICE_PREFIX,
               endpoint.endpoint_id_.c_str (),
              SK_WASAPI_EndPointManager::MMDEVAPI_RENDER_POSTFIX
            );
        }

        SK_ComPtr<IMMDevice>  pSecondaryDevice;
        pDevEnum->GetDevice (           device_id_str.c_str (),
                             &pSecondaryDevice.p );

        if (! pSecondaryDevice)
          continue;

        if (FAILED (pSecondaryDevice->Activate (
                    __uuidof (IAudioSessionManager2),
                      CLSCTX_ALL,
                        nullptr,
                          reinterpret_cast <void **>(&session_mgr)
                 )
             )
         ) continue;
#endif
      }

      SK_ComPtr <IAudioSessionEnumerator>             pSessionEnum;
      if (FAILED (session_mgr->GetSessionEnumerator (&pSessionEnum)))
        continue;

      int                                  num_sessions = 0;
      if (FAILED (pSessionEnum->GetCount (&num_sessions)))
        continue;

      for (int i = 0; i < num_sessions; i++)
      {
        SK_ComPtr <IAudioSessionControl>          pSessionCtl;
        if (FAILED (pSessionEnum->GetSession (i, &pSessionCtl)))
          continue;

        SK_IAudioSessionControl2                                          pSessionCtl2;
        if (FAILED (pSessionCtl->QueryInterface <IAudioSessionControl2> (&pSessionCtl2.p)))
          continue;

        DWORD                                    dwProcess = 0;
        if (FAILED (pSessionCtl2->GetProcessId (&dwProcess))) {
          continue;
        }

        AudioSessionState                       state = AudioSessionStateInactive;
        if (SUCCEEDED (pSessionCtl2->GetState (&state)))
        {
          if (state != AudioSessionStateExpired)
          {
            auto* pSession =
              new SK_WASAPI_AudioSession (pSessionCtl2, this);

            sessions_.emplace (pSession);

            switch (state)
            {
              case AudioSessionStateActive:
                active_sessions_.data.emplace (pSession);
                break;
              case AudioSessionStateInactive:
                inactive_sessions_.data.emplace (pSession);
                break;
              //case AudioSessionStateExpired:
              //  if (  active_sessions_.data.contains (pSession))   active_sessions_.data.erase (pSession);
              //  if (inactive_sessions_.data.contains (pSession)) inactive_sessions_.data.erase (pSession);
              //  break;
              default:
                break;
            }

            if (active_sessions_.data.empty ())
                active_sessions_.view.clear ();

            else active_sessions_.view =
              std::vector <SK_WASAPI_AudioSession *> ( active_sessions_.data.cbegin (),
                                                       active_sessions_.data.cend   () );

            if (inactive_sessions_.data.empty ())
                inactive_sessions_.view.clear ();

            else inactive_sessions_.view =
              std::vector <SK_WASAPI_AudioSession *> ( inactive_sessions_.data.cbegin (),
                                                       inactive_sessions_.data.cend   () );
          }
        }
      }

      session_mgr->RegisterSessionNotification (this);
    }

    CoTaskMemFree (default_dev_id);

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
        AudioSessionState        state = AudioSessionStateExpired;
        pSessionCtl2->GetState (&state);

        DWORD dwProcess = 0;
        if (SUCCEEDED (pSessionCtl2->GetProcessId (&dwProcess)) && state != AudioSessionStateExpired)
        {
          auto* pSession =
            new SK_WASAPI_AudioSession (pSessionCtl2, this);

          sessions_.emplace (pSession);

          switch (state)
          {
            case AudioSessionStateActive:
              active_sessions_.data.emplace (pSession);
              break;
            case AudioSessionStateInactive:
              inactive_sessions_.data.emplace (pSession);
              break;
            default:
              break;
          }

          if (active_sessions_.data.empty ())
              active_sessions_.view.clear ();

          else active_sessions_.view =
            std::vector <SK_WASAPI_AudioSession *> ( active_sessions_.data.cbegin (),
                                                     active_sessions_.data.cend   () );

          if (inactive_sessions_.data.empty ())
              inactive_sessions_.view.clear ();

          else inactive_sessions_.view =
            std::vector <SK_WASAPI_AudioSession *> ( inactive_sessions_.data.cbegin (),
                                                     inactive_sessions_.data.cend   () );
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

  SK_AutoHandle                      reset_event_;

  struct {
    using session_set_t         =
             std::set <   SK_WASAPI_AudioSession *>;
          session_set_t    data = {  };

    using session_vec_t         =
             std::vector <SK_WASAPI_AudioSession *>;
          session_vec_t    view = {  };
  }                                  active_sessions_,
                                     inactive_sessions_;

  SK_IAudioSessionManager2           session_mgrs_ [16];
  SK_IAudioSessionManager2&          session_mgr_ = session_mgrs_ [0];
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