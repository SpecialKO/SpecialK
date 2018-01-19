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

#include <SpecialK/com_util.h>
#include <SpecialK/utility.h>
#include <SpecialK/thread.h>
#include <SpecialK/hooks.h>
#include <SpecialK/log.h>

#pragma comment (lib, "wbemuuid.lib")

COM::Base COM::base = { };

void
COM::Base::WMI::Lock (void)
{
  //EnterCriticalSection (&wmi_cs);
  wmi_cs->lock ();
}

void
COM::Base::WMI::Unlock (void)
{
  //LeaveCriticalSection (&wmi_cs);
  wmi_cs->unlock ();
}


#include <dinput.h>
DEFINE_GUID(CLSID_DirectInput,        0x25E609E0,0xB259,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(CLSID_DirectInputDevice,  0x25E609E1,0xB259,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);

DEFINE_GUID(CLSID_DirectInput8,       0x25E609E4,0xB259,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(CLSID_DirectInputDevice8,	0x25E609E5,0xB259,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);

DEFINE_GUID(CLSID_DirectDraw,         0xD7B70EE0,0x4340,0x11CF,0xB0,0x63,0x00,0x20,0xAF,0xC2,0xCD,0x35);


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

  return CoCreateInstance_Original (rclsid, pUnkOuter, dwClsContext, riid, ppv);
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
  bool device =   (rclsid == CLSID_DirectInputDevice8 || rclsid == CLSID_DirectInputDevice);
  bool base   = device    ?                     false :
                  (rclsid == CLSID_DirectInput8       || rclsid == CLSID_DirectInput);

  int version = device ? (rclsid == CLSID_DirectInputDevice8 ? 8 : 7 )
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
SK_InitCOM (void)
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
      dll_log.Log (L"[COM Secure] Failure to initialize COM Security (%s:%d) -- 0x%X",
                   __FILEW__, __LINE__, hr);
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
  SK_AutoCOMInit auto_com;

  UNREFERENCED_PARAMETER (lpUser);

  HRESULT hr;

  if (FAILED (hr = CoCreateInstance_Original (
                     CLSID_WbemLocator, 
                     nullptr,
                     CLSCTX_INPROC_SERVER,
                     IID_IWbemLocator,
                     (void**) &COM::base.wmi.pWbemLocator )
             )
     )
  {
    dll_log.Log (L"[ WMI Wbem ] Failed to create Wbem Locator (%s:%d) -- 0x%X",
      __FILEW__, __LINE__, hr);
    goto WMI_CLEANUP;
  }

  // Connect to the desired namespace.
  COM::base.wmi.bstrNameSpace = SysAllocString (L"\\\\.\\Root\\CIMv2");
  if (COM::base.wmi.bstrNameSpace == nullptr)
  {
    dll_log.Log (L"[ WMI Wbem ] Out of Memory (%s:%d)",
      __FILEW__, __LINE__);
    hr = E_OUTOFMEMORY;
    goto WMI_CLEANUP;
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
    dll_log.Log (L"[ WMI Wbem ] Failure to Connect to Wbem Server (%s:%d) -- 0x%X",
      __FILEW__, __LINE__, hr);
    goto WMI_CLEANUP;
  }

  IUnknown* pUnk = nullptr;

  if (FAILED (COM::base.wmi.pNameSpace->QueryInterface (IID_IUnknown, (void **)&pUnk)))
    goto WMI_CLEANUP;

  // Set the proxy so that impersonation of the client occurs.
  if (FAILED (hr = CoSetProxyBlanket (
                     pUnk,
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
    dll_log.Log (L"[ WMI Wbem ] Failure to set proxy impersonation (%s:%d) -- 0x%X",
      __FILEW__, __LINE__, hr);
    pUnk->Release ();
    goto WMI_CLEANUP;
  }

  pUnk->Release ();

  InterlockedExchange (&COM::base.wmi.init, 1);

  // Keep the thread alive indefinitely so that the WMI stuff continues running
  while (MsgWaitForMultipleObjects (1, &COM::base.wmi.hShutdownServer, FALSE, INFINITE, QS_ALLEVENTS) != WAIT_OBJECT_0)
    ;

WMI_CLEANUP:
  if (COM::base.wmi.bstrNameSpace != nullptr)
  {
    SysFreeString (COM::base.wmi.bstrNameSpace);
    COM::base.wmi.bstrNameSpace       = nullptr;
  }

  if (COM::base.wmi.pWbemLocator != nullptr)
  {
    COM::base.wmi.pWbemLocator->Release ();
    COM::base.wmi.pWbemLocator   = nullptr;
  }

  if (COM::base.wmi.pNameSpace != nullptr)
  {
    COM::base.wmi.pNameSpace->Release ();
    COM::base.wmi.pNameSpace   = nullptr;
  }

  InterlockedExchange (&COM::base.wmi.init, 0);

  return 0;
}
bool
SK_InitWMI (void)
{
#if 0
  CoCreateInstance_Original =
    (CoCreateInstance_pfn)GetProcAddress   (GetModuleHandleW (L"ole32.dll"), "CoCreateInstance");
  CoCreateInstanceEx_Original =
    (CoCreateInstanceEx_pfn)GetProcAddress (GetModuleHandleW (L"ole32.dll"), "CoCreateInstanceEx");
#else
  SK_CreateDLLHook2 (      L"ole32.dll",
                            "CoCreateInstance",
                             CoCreateInstance_Detour,
    static_cast_p2p <void> (&CoCreateInstance_Original) );

  SK_CreateDLLHook2 (      L"ole32.dll",
                            "CoCreateInstanceEx",
                             CoCreateInstanceEx_Detour,
    static_cast_p2p <void> (&CoCreateInstanceEx_Original) );

  SK_ApplyQueuedHooks ();
#endif
  
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
      CreateEvent (nullptr, TRUE, FALSE, L"WMI Shutdown");

    InterlockedExchangePointer (&COM::base.wmi.hServerThread,
      CreateThread (nullptr, 0,
                    [](LPVOID) ->
                    DWORD
                    {
                      SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_LOWEST);

                      SK_AutoCOMInit auto_com;

                      SK_InitCOM ();

                      return SK_WMI_ServerThread (nullptr);
                    },
                    nullptr, 0x00, nullptr
                    )
    );
  }

  else
  {
    SK_WaitForInit_WMI ();
  }

  COM::base.wmi.Unlock ();

  return true;
}

void
SK_ShutdownWMI (void)
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

    const auto _SpinMax = 250;

    while (ReadAcquire (&COM::base.wmi.init))
    {
      for (int i = 0; i < _SpinMax && (ReadAcquire (&COM::base.wmi.init)); i++)
        ;

      if (! ReadAcquire (&COM::base.wmi.init))
        break;

      MsgWaitForMultipleObjectsEx (0, nullptr, 16, QS_ALLEVENTS, MWMO_ALERTABLE);
    }
  }
}