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
#include <SpecialK/com_util.h>

namespace COM
{
  Base base = { };
};

void
COM::Base::WMI::Lock (void) noexcept
{
  if (wmi_cs != nullptr)
      wmi_cs->lock ();
}

void
COM::Base::WMI::Unlock (void) noexcept
{
  if (wmi_cs != nullptr)
      wmi_cs->unlock ();
}

DEFINE_GUID(CLSID_DirectInput,        0x25E609E0,0xB259,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(CLSID_DirectInputDevice,  0x25E609E1,0xB259,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);

DEFINE_GUID(CLSID_DirectInput8,       0x25E609E4,0xB259,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(CLSID_DirectInputDevice8, 0x25E609E5,0xB259,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);

DEFINE_GUID(CLSID_DirectDraw,         0xD7B70EE0,0x4340,0x11CF,0xB0,0x63,0x00,0x20,0xAF,0xC2,0xCD,0x35);

DEFINE_GUID(CLSID_DxDiagProvider,     0xA65B8071,0x3BFE,0x4213,0x9A,0x5B,0x49,0x1D,0xA4,0x46,0x1C,0xA7);


CoCreateInstance_pfn   CoCreateInstance_Original   = nullptr;
CoCreateInstanceEx_pfn CoCreateInstanceEx_Original = nullptr;


extern
HRESULT
WINAPI
CoCreateInstance_DI8 (
  _In_  LPUNKNOWN pUnkOuter,
  _In_  DWORD     dwClsContext,
  _In_  REFIID    riid,
  _Out_ LPVOID   *ppv,
  _In_  LPVOID    pCallerAddr );

extern
HRESULT
WINAPI
CoCreateInstance_DI7 (
  _In_  LPUNKNOWN pUnkOuter,
  _In_  DWORD     dwClsContext,
  _In_  REFIID    riid,
  _Out_ LPVOID   *ppv,
  _In_  LPVOID    pCallerAddr );

HRESULT
WINAPI
CoCreateInstance_Detour (
  _In_  REFCLSID  rclsid,
  _In_  LPUNKNOWN pUnkOuter,
  _In_  DWORD     dwClsContext,
  _In_  REFIID    riid,
  _Out_ LPVOID   *ppv )
{
  if (ppv == nullptr)
    return E_POINTER;

  if (rclsid == CLSID_DirectInput8)
  {
    if (riid == IID_IDirectInput8A)
      return CoCreateInstance_DI8 (pUnkOuter, dwClsContext, riid, ppv, _ReturnAddress ());
    if (riid == IID_IDirectInput8W)
      return CoCreateInstance_DI8 (pUnkOuter, dwClsContext, riid, ppv, _ReturnAddress ());
    
  }

  if (rclsid == CLSID_DirectInput)
  {
    if (riid == IID_IDirectInputA)
     return CoCreateInstance_DI7 (pUnkOuter, dwClsContext, riid, ppv, _ReturnAddress ());
    if (riid == IID_IDirectInputW)
     return CoCreateInstance_DI7 (pUnkOuter, dwClsContext, riid, ppv, _ReturnAddress ());
  }

  if (rclsid == CLSID_DirectDraw)
  {
    //extern void SK_BootDDraw (void);
    //SK_BootDDraw ();
  }

  if (rclsid == CLSID_DxDiagProvider)
  {
    if (! config.compatibility.allow_dxdiagn)
    {
      SK_LOGs0 ( L"DXDiagKill",
        L" >> Game tried to use IDxDiagProvider, but it has been "
        L"blocked for maximum compatibility."
      );

      return E_NOINTERFACE;
    }
  }

  return CoCreateInstance_Original (rclsid, pUnkOuter, dwClsContext, riid, ppv);
}

using DxDiagn_DllGetClassObject_pfn = HRESULT (WINAPI *)(REFCLSID, REFIID, LPVOID);
      DxDiagn_DllGetClassObject_pfn
      DxDiagn_DllGetClassObject_Original = nullptr;

HRESULT
WINAPI
DxDiagn_DllGetClassObject_Detour (
  REFCLSID rclsid,
  REFIID   riid,
  LPVOID   *ppv )
{
  //if (rclsid == CLSID_DxDiagProvider)
  {
    if (! config.compatibility.allow_dxdiagn)
    {
      SK_LOGs0 ( L"DXDiagKill",
        L" >> Game tried to use IDxDiagProvider, but it has been "
        L"blocked for maximum compatibility."
      );

      return E_NOINTERFACE;
    }
  }

  return DxDiagn_DllGetClassObject_Original (rclsid, riid, ppv);
}


extern
HRESULT
STDAPICALLTYPE
CoCreateInstanceEx_DI8 (
  _In_    REFCLSID     rclsid,
  _In_    IUnknown     *punkOuter,
  _In_    DWORD        dwClsCtx,
  _In_    COSERVERINFO *pServerInfo,
  _In_    DWORD        dwCount,
  _Inout_ MULTI_QI     *pResults,
  _In_    LPVOID        pCallerAddr );

extern
HRESULT
STDAPICALLTYPE
CoCreateInstanceEx_DI7 (
  _In_    REFCLSID     rclsid,
  _In_    IUnknown     *punkOuter,
  _In_    DWORD        dwClsCtx,
  _In_    COSERVERINFO *pServerInfo,
  _In_    DWORD        dwCount,
  _Inout_ MULTI_QI     *pResults,
  _In_    LPVOID        pCallerAddr );

HRESULT
STDAPICALLTYPE
CoCreateInstanceEx_Detour (
  _In_    REFCLSID     rclsid,
  _In_    IUnknown     *pUnkOuter,
  _In_    DWORD        dwClsCtx,
  _In_    COSERVERINFO *pServerInfo,
  _In_    DWORD        dwCount,
  _Inout_ MULTI_QI     *pResults )
{
  const bool device =   (rclsid == CLSID_DirectInputDevice8 || rclsid == CLSID_DirectInputDevice);
  const bool base   = device    ?                     false :
                  (rclsid == CLSID_DirectInput8       || rclsid == CLSID_DirectInput);

  const int version = device ? (rclsid == CLSID_DirectInputDevice8 ? 8 : 7 )
                             : (rclsid == CLSID_DirectInput8       ? 8 : 7 );

  if (device || base)
  {
    switch (version)
    {
      case 7:
        return CoCreateInstanceEx_DI7 (rclsid, pUnkOuter, dwClsCtx, pServerInfo, dwCount, pResults, _ReturnAddress ());
      case 8:
        return CoCreateInstanceEx_DI8 (rclsid, pUnkOuter, dwClsCtx, pServerInfo, dwCount, pResults, _ReturnAddress ());
    }
  }

  return CoCreateInstanceEx_Original (rclsid, pUnkOuter, dwClsCtx, pServerInfo, dwCount, pResults);
}




bool
SK_COM_InitSecurity (void)
{
  HRESULT hr;

  if (FAILED (hr = CoInitializeSecurity (
                     nullptr,
                     -1,
                     nullptr,
                     nullptr,
                     RPC_C_AUTHN_LEVEL_NONE,
                     RPC_C_IMP_LEVEL_IMPERSONATE,
                     nullptr, EOAC_NONE, nullptr )
             )
     )
  {
    // It's possible that the application already did this, in which case
    //   it is immutable and we should try to deal with whatever the app
    //     initialized it to.
    if (hr != RPC_E_TOO_LATE)
    {
      SK_LOG0 ( ( L"Failure to initialize COM Security (%s:%d) -- %s",
                  __FILEW__, __LINE__, _com_error (hr).ErrorMessage () ),
                  L"COM Secure" );

      return false;
    }
  }

  return true;
}


HANDLE hShutdownWMI = nullptr;

DWORD
WINAPI
SK_WMI_ServerThread (LPVOID lpUser)
{
  std::ignore = lpUser;

  if (ReadAcquire (&COM::base.wmi.init))
  {
    SK_Thread_CloseSelf ();

    return 0;
  }

  SetCurrentThreadDescription (L"[SK] WMI Server");

  auto WMI_CLEANUP = [&](void) -> DWORD
  {
    if (COM::base.wmi.pNameSpace != nullptr)
    {
      COM::base.wmi.pNameSpace->Release ();
      COM::base.wmi.pNameSpace   = nullptr;
    }

    if (COM::base.wmi.pWbemLocator != nullptr)
    {
      COM::base.wmi.pWbemLocator->Release ();
      COM::base.wmi.pWbemLocator   = nullptr;
    }

    if (COM::base.wmi.bstrNameSpace != nullptr)
    {
      SysFreeString (COM::base.wmi.bstrNameSpace);
      COM::base.wmi.bstrNameSpace = nullptr;
    }

    InterlockedExchange (&COM::base.wmi.init, 0);

    SK_Thread_CloseSelf ();

    return 0;
  };

  SK_AutoCOMInit auto_com;
  HRESULT        hr;

  if (FAILED (hr = CoCreateInstance (
                     CLSID_WbemLocator,
                     nullptr,
                     CLSCTX_INPROC_SERVER,
                     IID_IWbemLocator,
                     (void**) &COM::base.wmi.pWbemLocator )
             )
     )
  {
    SK_LOG0 ( ( L"Failed to create Wbem Locator (%s:%d) -- %s",
                  __FILEW__, __LINE__, _com_error (hr).ErrorMessage () ),
                L" WMI Wbem " );

    return WMI_CLEANUP ();
  }

  // Connect to the desired namespace.
  COM::base.wmi.bstrNameSpace = SysAllocString (LR"(root\CIMV2)");

  if (COM::base.wmi.bstrNameSpace == nullptr)
  {
    SK_LOG0 ( ( L"Out of Memory (%s:%d)",
                  __FILEW__, __LINE__ ),
                L" WMI Wbem " );

    hr = E_OUTOFMEMORY;

    return WMI_CLEANUP ();
  }

  if (FAILED (hr = COM::base.wmi.pWbemLocator->ConnectServer (
                     COM::base.wmi.bstrNameSpace,
                     nullptr, // User name
                     nullptr, // Password
                     nullptr, // Locale
                     0L,      // Security flags
                     nullptr, // Authority
                     nullptr, // Wbem context
                     &COM::base.wmi.pNameSpace )
             )
     )
  {
    SK_LOG0 ( ( L"Failure to Connect to Wbem Server (%s:%d) -- %s",
                  __FILEW__, __LINE__, _com_error (hr).ErrorMessage () ),
                L" WMI Wbem " );

    return WMI_CLEANUP ();
  }

  // Set the proxy so that impersonation of the client occurs.
  if (FAILED (hr = CoSetProxyBlanket (
                     COM::base.wmi.pNameSpace,
                     RPC_C_AUTHN_WINNT,
                     RPC_C_AUTHZ_NONE,
                     nullptr,
                     RPC_C_AUTHN_LEVEL_CALL,
                     RPC_C_IMP_LEVEL_IMPERSONATE,
                     nullptr,
                     EOAC_NONE )
             )
     )
  {
    SK_LOG0 ( ( L"Failure to set proxy impersonation (%s:%d) -- %s",
                  __FILEW__, __LINE__, _com_error (hr).ErrorMessage () ),
                L" WMI Wbem " );

    return WMI_CLEANUP ();
  }

  SK_Thread_SetCurrentPriority (THREAD_PRIORITY_IDLE);

  InterlockedExchange (&COM::base.wmi.init, 1);

  // Keep the thread alive indefinitely so that the WMI stuff continues running
  while ( MsgWaitForMultipleObjects ( 1, &COM::base.wmi.hShutdownServer,
                                      FALSE, INFINITE, QS_ALLEVENTS ) != WAIT_OBJECT_0 )
    ;

  return WMI_CLEANUP ();
}

static HMODULE hModCOMBase = nullptr;

extern "C++"
bool
SK_WMI_Init (void)
{
  static bool          once = false;
  if (! std::exchange (once, true))
  {
    const wchar_t* wszCOMBase  = L"combase.dll";
                   hModCOMBase = SK_Modules->LoadLibrary (wszCOMBase);

    if (hModCOMBase == nullptr)
    {
      wszCOMBase  = L"ole32.dll";
      hModCOMBase = SK_Modules->LoadLibrary (wszCOMBase);
    }

    SK_CreateDLLHook2 (      wszCOMBase,
                              "CoCreateInstance",
                               CoCreateInstance_Detour,
      static_cast_p2p <void> (&CoCreateInstance_Original) );

    SK_CreateDLLHook2 (      wszCOMBase,
                              "CoCreateInstanceEx",
                               CoCreateInstanceEx_Detour,
      static_cast_p2p <void> (&CoCreateInstanceEx_Original) );

    if (SK_GetModuleHandle (L"dxdiagn.dll") != nullptr) {
      SK_CreateDLLHook2 (   L"dxdiagn.dll",
                                        "DllGetClassObject",
                                 DxDiagn_DllGetClassObject_Detour,
        static_cast_p2p <void> (&DxDiagn_DllGetClassObject_Original) );
    }
  }

  COM::base.wmi.Lock ();

  if (ReadAcquire (&COM::base.wmi.init) > 0)
  {
    COM::base.wmi.Unlock ();
    return true;
  }

  if ( InterlockedCompareExchangePointer (&COM::base.wmi.hServerThread, nullptr, INVALID_HANDLE_VALUE)
                                                                              == INVALID_HANDLE_VALUE )
  {
    COM::base.wmi.hShutdownServer =
      SK_CreateEvent (nullptr, TRUE, FALSE, nullptr);

    InterlockedExchangePointer (&COM::base.wmi.hServerThread,
      (HANDLE)
        SK_Thread_CreateEx (
            [](LPVOID) ->
            DWORD
            {
              SK_Thread_SetCurrentPriority (THREAD_PRIORITY_ABOVE_NORMAL);

              SK_AutoCOMInit auto_com;

              SK_COM_InitSecurity ();

              return
                SK_WMI_ServerThread (nullptr);
            },
          L"[SK] WMI Server Thread",
        nullptr
      )
    );
  }

  else
    SK_WMI_WaitForInit ();

  COM::base.wmi.Unlock ();

  return true;
}

extern "C++"
void
SK_WMI_Shutdown (void)
{
  if (InterlockedCompareExchange (&COM::base.wmi.init, 0, 1))
  {
    HANDLE hServerThread =
      ReadPointerAcquire (&COM::base.wmi.hServerThread);

    SetEvent (COM::base.wmi.hShutdownServer);

    if (hServerThread)
    {
      InterlockedExchangePointer (&COM::base.wmi.hServerThread, nullptr);
    }

    static constexpr auto _SpinMax = 250;

    while (ReadAcquire (&COM::base.wmi.init))
    {
      for (int i = 0; i < _SpinMax && (ReadAcquire (&COM::base.wmi.init) != 0L); i++) {
        ;
      }

      if (ReadAcquire (&COM::base.wmi.init) == 0L)
        break;

      MsgWaitForMultipleObjectsEx (0, nullptr, 16, QS_ALLEVENTS, MWMO_INPUTAVAILABLE);
    }

    if (hModCOMBase != nullptr)
    {
      SK_FreeLibrary (hModCOMBase);
                      hModCOMBase = nullptr;
    }
  }
}

bool
SK_AutoCOMInit::_assert_not_dllmain (void)
{
  SK_ASSERT_NOT_DLLMAIN_THREAD ();

  if (config.compatibility.init_on_separate_thread && (! config.compatibility.init_sync_for_streamline))
  {
    if (ReadAcquire (&__SK_DLL_Attached))
    {
      const SK_TLS* pTLS =
        SK_TLS_Bottom ();

      if (pTLS)
        return (!pTLS->debug.in_DllMain);
    }
  }
  else
    return true;

  // If we have no TLS, assume it's because we're in DLL main
  return false;//return true;
}


HRESULT
SK_SafeQueryInterface (IUnknown* pObj, REFIID riid, void** pUnk)
{
  if (pObj != nullptr)
  {
    __try
    {
      return
        pObj->QueryInterface (riid, pUnk);
    }

    __except ( (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION)
                                     ? EXCEPTION_EXECUTE_HANDLER :
                                       EXCEPTION_CONTINUE_SEARCH )
    {
      SK_LOG0 ( ( L"Access Violation Querying Interface For ..." ),
                  L"COM Helper" );
    }
  }

  return E_POINTER;
}