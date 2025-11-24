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

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L" DInput 8 "

#define _L2(w)  L ## w
#define  _L(w) _L2(w)


using finish_pfn = void (WINAPI *)(void);


#define SK_DI8_READ(type)  SK_DI8_Backend->markRead   (type);
#define SK_DI8_WRITE(type) SK_DI8_Backend->markWrite  (type);
#define SK_DI8_VIEW(type)  SK_DI8_Backend->markViewed (type);
#define SK_DI8_HIDE(type)  SK_DI8_Backend->markHidden (type);


#define DINPUT8_CALL(_Ret, _Call) {                                      \
  dll_log->LogEx (true, L"[   Input  ]  Calling original function: ");   \
  (_Ret) = (_Call);                                                      \
  _com_error err ((_Ret));                                               \
  if ((_Ret) != S_OK)                                                    \
    dll_log->LogEx (false, L"(ret=0x%04x - %s)\n", err.WCode        (),  \
                                                   err.ErrorMessage ()); \
  else                                                                   \
    dll_log->LogEx (false, L"(ret=S_OK)\n");                             \
}

///////////////////////////////////////////////////////////////////////////////
//
// DirectInput 8
//
///////////////////////////////////////////////////////////////////////////////
DirectInput8Create_pfn
         DirectInput8Create_Import                         = nullptr;

IDirectInput8W_CreateDevice_pfn
        IDirectInput8W_CreateDevice_Original               = nullptr;

IDirectInput8W_EnumDevices_pfn
        IDirectInput8W_EnumDevices_Original                = nullptr;

IDirectInputDevice8W_GetDeviceState_pfn
        IDirectInputDevice8W_GetDeviceState_Original       = nullptr;

IDirectInputDevice8W_SetCooperativeLevel_pfn
        IDirectInputDevice8W_SetCooperativeLevel_Original  = nullptr;

IDirectInput8A_CreateDevice_pfn
        IDirectInput8A_CreateDevice_Original               = nullptr;

IDirectInput8A_EnumDevices_pfn
        IDirectInput8A_EnumDevices_Original                = nullptr;

IDirectInputDevice8A_GetDeviceState_pfn
        IDirectInputDevice8A_GetDeviceState_Original       = nullptr;

IDirectInputDevice8A_SetCooperativeLevel_pfn
        IDirectInputDevice8A_SetCooperativeLevel_Original  = nullptr;

IDirectInputDevice8A_Poll_pfn
IDirectInputDevice8A_Poll_Original = nullptr;

IDirectInputDevice8W_Poll_pfn
IDirectInputDevice8W_Poll_Original = nullptr;

HRESULT
WINAPI
IDirectInput8A_CreateDevice_Detour ( IDirectInput8A        *This,
                                     REFGUID                rguid,
                                     LPDIRECTINPUTDEVICE8A *lplpDirectInputDevice,
                                     LPUNKNOWN              pUnkOuter );

HRESULT
WINAPI
IDirectInput8W_CreateDevice_Detour ( IDirectInput8W        *This,
                                     REFGUID                rguid,
                                     LPDIRECTINPUTDEVICE8W *lplpDirectInputDevice,
                                     LPUNKNOWN              pUnkOuter );

HRESULT
WINAPI
IDirectInput8A_EnumDevices_Detour ( IDirectInput8A*          This,
                                    DWORD                    dwDevType,
                                    LPDIENUMDEVICESCALLBACKA lpCallback,
                                    LPVOID                   pvRef,
                                    DWORD                    dwFlags );

HRESULT
WINAPI
IDirectInput8W_EnumDevices_Detour ( IDirectInput8W*          This,
                                    DWORD                    dwDevType,
                                    LPDIENUMDEVICESCALLBACKW lpCallback,
                                    LPVOID                   pvRef,
                                    DWORD                    dwFlags );



volatile LONG __di8_ready = FALSE;

void
WINAPI
WaitForInit_DI8 (void)
{
  SK_Thread_SpinUntilFlagged (&__di8_ready);
}


void
WINAPI
SK_BootDI8 (void);

__declspec (noinline)
HRESULT
WINAPI
DirectInput8Create ( HINSTANCE hinst,
                     DWORD     dwVersion,
                     REFIID    riidltf,
                     LPVOID   *ppvOut,
                     LPUNKNOWN punkOuter )
{
  // Prevent REFramework from breaking keyboard input in the various games it's used in
  if (SK_GetCallingDLL () != nullptr && SK_GetCallingDLL () == GetModuleHandleW (L"REFramework.dll"))
    return E_NOTIMPL;

  if (SK_GetDLLRole () == DLL_ROLE::DInput8)
  {
    SK_BootDI8      ();
    WaitForInit_DI8 ();
  }

  dll_log->Log ( L"[ DInput 8 ] [!] %s (%08" _L(PRIxPTR) L"h, %lu, {...}, ppvOut="
                                      L"%08" _L(PRIxPTR) L"h, %08" _L(PRIxPTR) L"h)"
                                      L"                             -- %s",
                   L"DirectInput8Create",
                     (uintptr_t)hinst,             dwVersion, /*,*/
                     (uintptr_t)ppvOut, (uintptr_t)punkOuter,
                       SK_SummarizeCaller ().c_str () );

  HRESULT hr =
    E_NOINTERFACE;

  std::vector <void *> fns_to_hook;

  if (DirectInput8Create_Import != nullptr)
  {
    if (riidltf == IID_IDirectInput8W)
    {
      if ( SUCCEEDED (
             ( hr = DirectInput8Create_Import ( hinst, dwVersion,
                                                  riidltf, ppvOut,
                                                    punkOuter )
             )
           ) && ppvOut != nullptr
         )
      {
        if (! IDirectInput8W_CreateDevice_Original)
        {
          void** vftable = *(void***)*ppvOut;

          if ( MH_OK ==
            SK_CreateFuncHook (       L"IDirectInput8W::CreateDevice",
                                       vftable [3],
                                       IDirectInput8W_CreateDevice_Detour,
              static_cast_p2p <void> (&IDirectInput8W_CreateDevice_Original) ) )
                fns_to_hook.push_back (vftable [3]);
        }

        if (! IDirectInput8W_EnumDevices_Original)
        {
          void** vftable = *(void***)*ppvOut;

          if ( MH_OK ==
            SK_CreateFuncHook (       L"IDirectInput8W::EnumDevices",
                                       vftable [4],
                                       IDirectInput8W_EnumDevices_Detour,
              static_cast_p2p <void> (&IDirectInput8W_EnumDevices_Original) ) )
                fns_to_hook.push_back (vftable [4]);
        }
      }
    }

    else if (riidltf == IID_IDirectInput8A)
    {
      if ( SUCCEEDED (
             ( hr = DirectInput8Create_Import ( hinst, dwVersion,
                                                  riidltf, ppvOut,
                                                    punkOuter )
             )
           ) && ppvOut != nullptr
         )
      {
        if (! IDirectInput8A_CreateDevice_Original)
        {
          void** vftable = *(void***)*ppvOut;

          if ( MH_OK ==
            SK_CreateFuncHook (    L"IDirectInput8A::CreateDevice",
                                     vftable [3],
                                     IDirectInput8A_CreateDevice_Detour,
            static_cast_p2p <void> (&IDirectInput8A_CreateDevice_Original) ) )
              fns_to_hook.push_back (vftable [3]);
        }

        if (! IDirectInput8A_EnumDevices_Original)
        {
          void** vftable = *(void***)*ppvOut;

          if ( MH_OK ==
            SK_CreateFuncHook (    L"IDirectInput8A::EnumDevices",
                                     vftable [4],
                                     IDirectInput8A_EnumDevices_Detour,
            static_cast_p2p <void> (&IDirectInput8A_EnumDevices_Original) ) )
              fns_to_hook.push_back (vftable [4]);
        }
      }
    }
  }

  if (! fns_to_hook.empty ())
  {
    auto suspend_ctx = SK_SuspendAllOtherThreads ();

    for (auto fn : fns_to_hook)
    {
      SK_EnableHook (fn);
    }

    SK_ResumeThreads (suspend_ctx);
  }

  return hr;
}


void
WINAPI
SK_BootDI8 (void)
{
  static volatile LONG hooked = FALSE;

  if (! InterlockedCompareExchange (&hooked, TRUE, FALSE))
  {
    if (DirectInput8Create_Import == nullptr)
    {
      HMODULE hBackend =
        (SK_GetDLLRole () & DLL_ROLE::DInput8) ?
              backend_dll : SK_Modules->LoadLibraryLL (L"dinput8.dll");

      dll_log->Log (L"[ DInput 8 ] Importing DirectInput8Create....");
      dll_log->Log (L"[ DInput 8 ] ================================");

      if (! _wcsicmp (SK_GetModuleName (SK_GetDLL ()).c_str (), L"dinput8.dll"))
      {
        dll_log->Log (L"[ DInput 8 ]   DirectInput8Create:   %08" _L(PRIxPTR) L"h",
          (uintptr_t)(DirectInput8Create_Import =  \
            reinterpret_cast <DirectInput8Create_pfn> (
              SK_GetProcAddress (hBackend, "DirectInput8Create")
            )
          )
        );
      }

      else if (hBackend != nullptr)
      {
        const bool bProxy =
          ( SK_GetModuleHandle (L"dinput8.dll") != hBackend );


        if ( InterlockedCompareExchangePointer ((void **)&DirectInput8Create_Import, (void*)1, nullptr) == nullptr &&
               MH_OK ==
                SK_CreateDLLHook2 (      L"dinput8.dll",
                                          "DirectInput8Create",
                                           DirectInput8Create,
                  static_cast_p2p <void> (&DirectInput8Create_Import) )
            )
        {
          if (bProxy)
          {
            DirectInput8Create_Import =
              reinterpret_cast <DirectInput8Create_pfn> (
                SK_GetProcAddress (hBackend, "DirectInput8Create")
              );
          }

          dll_log->Log (L"[ DInput 8 ]   DirectInput8Create:   %08" _L(PRIxPTR)
                                                        L"h  { Hooked }",
            (uintptr_t)DirectInput8Create_Import );
        }
      }
    }


    //
    // This whole thing is as smart as a sack of wet mice in DirectInput mode...
    //   let's get to the real work and start booting graphics APIs!
    //
    static bool gl   = false, vulkan = false, d3d9  = false, d3d11 = false, d3d12 = false,
                dxgi = false, d3d8   = false, ddraw = false, glide = false;

    SK_TestRenderImports (
      SK_GetModuleHandle (nullptr),
        &gl, &vulkan,
          &d3d9, &dxgi, &d3d11, &d3d12,
            &d3d8, &ddraw, &glide
    );


    // Load user-defined DLLs (Plug-In)
    SK_RunLHIfBitness ( 64, SK_LoadEarlyImports64 (),
                            SK_LoadEarlyImports32 () );


//#define SPAWN_THREAD
#ifdef SPAWN_THREAD
CreateThread (nullptr, 0x00, [](LPVOID/*user*/) -> unsigned int
{
UNREFERENCED_PARAMETER (user);
#endif

    // OpenGL
    //
    if (gl || (SK_IsModuleLoaded (L"OpenGL32.dll") && !SK_IsModuleLoaded (L"EOSOVH-Win64-Shipping.dll")))
      SK_BootOpenGL ();


    // Vulkan
    //
    //if (vulkan && GetModuleHandle (L"Vulkan-1.dll"))
    //  SK_BootVulkan ();


    // D3D9
    //
    if (d3d9 || SK_GetModuleHandle (L"d3d9.dll"))
      SK_BootD3D9 ();


    // D3D11
    //
    if (d3d11 || SK_GetModuleHandle (L"d3d11.dll"))
      SK_BootDXGI ();

    // D3D12
    //
    if (d3d12 || SK_GetModuleHandle (L"d3d12.dll"))
      SK_BootDXGI ();

    // Alternate form (or D3D10, but we don't care about that right now)
    else if (dxgi || SK_GetModuleHandle (L"dxgi.dll"))
      SK_BootDXGI ();


    // Load user-defined DLLs (Plug-In)
    SK_RunLHIfBitness (64, SK_LoadPlugIns64 (), SK_LoadPlugIns32 ());

#ifdef SPAWN_THREAD
  SK_Thread_CloseSelf ();

  return 0;
}, nullptr, 0x00, nullptr);
#endif

    InterlockedIncrement (&hooked);
    InterlockedExchange  (&__di8_ready, TRUE);
  }

  while (ReadAcquire (&hooked) < 2)
    ;
}

DEFINE_GUID(CLSID_DirectInput,        0x25E609E0,0xB259,0x11CF,0xBF,0xC7,
                                      0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(CLSID_DirectInputDevice,  0x25E609E1,0xB259,0x11CF,0xBF,0xC7,
                                      0x44,0x45,0x53,0x54,0x00,0x00);

DEFINE_GUID(CLSID_DirectInput8,       0x25E609E4,0xB259,0x11CF,0xBF,0xC7,
                                      0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(CLSID_DirectInputDevice8, 0x25E609E5,0xB259,0x11CF,0xBF,0xC7,
                                      0x44,0x45,0x53,0x54,0x00,0x00);

HRESULT
WINAPI
CoCreateInstance_DI8 (
  _In_  LPUNKNOWN pUnkOuter,
  _In_  DWORD     dwClsContext,
  _In_  REFIID    riid,
  _Out_ LPVOID   *ppv,
  _In_  LPVOID    pCallerAddr )
{
  if (DirectInput8Create_Import == nullptr)
  {
    SK_BootDI8 ();
  }

  if (SK_GetDLLRole () == DLL_ROLE::DInput8)
  {
    WaitForInit_DI8 ();
  }

  dll_log->Log ( L"[ DInput 8 ] [!] %s (%08" _L(PRIxPTR) L"h, %lu, {...}, ppvOut="
                                      L"%08" _L(PRIxPTR) L"h, %08" _L(PRIxPTR) L"h)"
                                      L"             -- %s",
                   L"DirectInput8Create <CoCreateInstance>",
                     0, 0x800,
                     (uintptr_t)ppv, (uintptr_t)pUnkOuter,
                       SK_SummarizeCaller (pCallerAddr).c_str () );

  HRESULT hr =
    E_NOINTERFACE;

  std::vector <void *> fns_to_hook;

  if (riid == IID_IDirectInput8A)
  {
    if ( SUCCEEDED (
           ( hr = CoCreateInstance_Original (
                    CLSID_DirectInput8, pUnkOuter, dwClsContext,
                                        riid, ppv )
           )
         ) && config.input.gamepad.hook_dinput8
       )
    {
      if (! IDirectInput8A_CreateDevice_Original)
      {
        void** vftable = *(void***)*ppv;

        SK_CreateFuncHook (       L"IDirectInput8A::CreateDevice",
                                   vftable [3],
                                   IDirectInput8A_CreateDevice_Detour,
          static_cast_p2p <void> (&IDirectInput8A_CreateDevice_Original) );

        fns_to_hook.push_back (vftable [3]);
      }

      if (! IDirectInput8A_EnumDevices_Original)
      {
        void** vftable = *(void***)*ppv;

        SK_CreateFuncHook (       L"IDirectInput8A::EnumDevices",
                                   vftable [4],
                                   IDirectInput8A_EnumDevices_Detour,
          static_cast_p2p <void> (&IDirectInput8A_EnumDevices_Original) );

        fns_to_hook.push_back (vftable [4]);
      }
    }
  }

  else if (riid == IID_IDirectInput8W)
  {
    if ( SUCCEEDED (
           ( hr = CoCreateInstance_Original (
                    CLSID_DirectInput8, pUnkOuter, dwClsContext,
                                        riid, ppv )
           )
         )
       )
    {
      if (! IDirectInput8W_CreateDevice_Original)
      {
        void** vftable = *(void***)*ppv;

        if ( MH_OK ==
          SK_CreateFuncHook (       L"IDirectInput8W::CreateDevice",
                                     vftable [3],
                                     IDirectInput8W_CreateDevice_Detour,
            static_cast_p2p <void> (&IDirectInput8W_CreateDevice_Original) ) )
              fns_to_hook.push_back (vftable [3]);
      }

      if (! IDirectInput8W_EnumDevices_Original)
      {
        void** vftable = *(void***)*ppv;

        if ( MH_OK ==
          SK_CreateFuncHook (       L"IDirectInput8W::EnumDevices",
                                     vftable [4],
                                     IDirectInput8W_EnumDevices_Detour,
            static_cast_p2p <void> (&IDirectInput8W_EnumDevices_Original) ) )
              fns_to_hook.push_back (vftable [4]);
      }
    }
  }

  if (! fns_to_hook.empty ())
  {
    auto suspend_ctx = SK_SuspendAllOtherThreads ();

    for (auto fn : fns_to_hook)
    {
      SK_EnableHook (fn);
    }

    SK_ResumeThreads (suspend_ctx);
  }

  return hr;
}

HRESULT
STDAPICALLTYPE
CoCreateInstanceEx_DI8 (
  _In_    REFCLSID     rclsid,
  _In_    IUnknown     *pUnkOuter,
  _In_    DWORD        dwClsCtx,
  _In_    COSERVERINFO *pServerInfo,
  _In_    DWORD        dwCount,
  _Inout_ MULTI_QI     *pResults,
  _In_    LPVOID        pCallerAddr )
{
  if (pResults == nullptr)
    return E_POINTER;

  SK_BootDI8        ();
  if (SK_GetDLLRole () == DLL_ROLE::DInput8)
  { WaitForInit_DI8 ();                    }

  dll_log->Log ( L"[ DInput 8 ] [!] %s (%08" _L(PRIxPTR) L"h, %lu, {...}, ppvOut="
                                      L"%08" _L(PRIxPTR) L"h)"
                                      L"           -- %s",
                   L"DirectInputCreate <CoCreateInstanceEx>",
                     0, 0x800,
                     (uintptr_t)pResults->pItf,
                       SK_SummarizeCaller (pCallerAddr).c_str () );

  std::vector <void *> fns_to_hook;

  HRESULT hr =
    E_NOINTERFACE;

  if (rclsid == CLSID_DirectInput8)
  {
    if ( SUCCEEDED (
           ( hr = CoCreateInstanceEx_Original (
                    CLSID_DirectInput8, pUnkOuter, dwClsCtx,
                                        pServerInfo, dwCount, pResults )
           )
         ) && config.input.gamepad.hook_dinput8
       )
    {
      if (SUCCEEDED (pResults->hr) && pResults->pItf != nullptr && pResults->pIID != nullptr)
      {
        if (*pResults->pIID == IID_IDirectInput8A)
        {
          if (! IDirectInput8A_CreateDevice_Original)
          {
            void** vftable = *(void***)*&pResults->pItf;

            if ( MH_OK ==
              SK_CreateFuncHook (       L"IDirectInput8A::CreateDevice",
                                         vftable [3],
                                         IDirectInput8A_CreateDevice_Detour,
                static_cast_p2p <void> (&IDirectInput8A_CreateDevice_Original) ) )
                  fns_to_hook.push_back (vftable [3]);
          }

          if (! IDirectInput8A_EnumDevices_Original)
          {
            void** vftable = *(void***)*&pResults->pItf;

            if ( MH_OK ==
              SK_CreateFuncHook (       L"IDirectInput8A::EnumDevices",
                                         vftable [4],
                                         IDirectInput8A_EnumDevices_Detour,
                static_cast_p2p <void> (&IDirectInput8A_EnumDevices_Original) ) )
                  fns_to_hook.push_back (vftable[4]);
          }
        }

        else if (*pResults->pIID == IID_IDirectInput8W)
        {
          if (! IDirectInput8W_CreateDevice_Original)
          {
            void** vftable = *(void***)*&pResults->pItf;

            if ( MH_OK ==
              SK_CreateFuncHook (       L"IDirectInput8W::CreateDevice",
                                         vftable [3],
                                         IDirectInput8W_CreateDevice_Detour,
                static_cast_p2p <void> (&IDirectInput8W_CreateDevice_Original) ) )
                  fns_to_hook.push_back (vftable [3]);
          }

          if (! IDirectInput8W_EnumDevices_Original)
          {
            void** vftable = *(void***)*&pResults->pItf;

            if ( MH_OK ==
              SK_CreateFuncHook (       L"IDirectInput8W::EnumDevices",
                                         vftable [4],
                                         IDirectInput8W_EnumDevices_Detour,
                static_cast_p2p <void> (&IDirectInput8W_EnumDevices_Original) ) )
                  fns_to_hook.push_back (vftable [4]);
          }
        }
      }
    }
  }

  if (! fns_to_hook.empty ())
  {
    auto suspend_ctx = SK_SuspendAllOtherThreads ();

    for (auto fn : fns_to_hook)
    {
      SK_EnableHook (fn);
    }

    SK_ResumeThreads (suspend_ctx);
  }

  return hr;
}

unsigned int
__stdcall
SK_HookDI8 (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  SK_AutoCOMInit auto_com;
  {
    SK_BootDI8 ();

    if (! (SK_GetDLLRole () & DLL_ROLE::DXGI))
      SK::DXGI::StartBudgetThread_NoAdapter ();
  }

  return 0;
}

void
WINAPI
di8_init_callback (finish_pfn finish)
{
  if (! SK_IsHostAppSKIM ())
  {
    SK_HookDI8 (nullptr);
    WaitForInit_DI8 (  );
  }

  finish ();
}


bool
SK::DI8::Startup (void)
{
  return SK_StartupCore (L"dinput8", di8_init_callback);
}

bool
SK::DI8::Shutdown (void)
{
  return SK_ShutdownCore (L"dinput8");
}


SK_LazyGlobal <SK_DI8_Keyboard> _dik8;
SK_LazyGlobal <SK_DI8_Mouse>    _dim8;

__declspec (noinline)
SK_DI8_Keyboard*
WINAPI
SK_Input_GetDI8Keyboard (void)
{
  return _dik8.getPtr ();
}

__declspec (noinline)
SK_DI8_Mouse*
WINAPI
SK_Input_GetDI8Mouse (void)
{
  return _dim8.getPtr ();
}

__declspec (noinline)
bool
WINAPI
SK_Input_DI8Mouse_Acquire (SK_DI8_Mouse* pMouse)
{
  if (pMouse == nullptr && _dim8->pDev != nullptr)
    pMouse = _dim8.getPtr ();

  if (pMouse != nullptr)
  {
    if (IDirectInputDevice8W_SetCooperativeLevel_Original)
    {
      IDirectInputDevice8W_SetCooperativeLevel_Original (
        pMouse->pDev,
          game_window.hWnd,
            pMouse->coop_level
      );
    }

    else if (IDirectInputDevice8A_SetCooperativeLevel_Original)
    {
      IDirectInputDevice8A_SetCooperativeLevel_Original (
        (IDirectInputDevice8A *)pMouse->pDev,
          game_window.hWnd,
            pMouse->coop_level
      );
    }

    else return false;

    return true;
  }

  return false;
}

__declspec (noinline)
bool
WINAPI
SK_Input_DI8Mouse_Release (SK_DI8_Mouse* pMouse)
{
  if (pMouse == nullptr && _dim8->pDev != nullptr)
    pMouse = _dim8.getPtr ();

  if (pMouse != nullptr)
  {
    if (IDirectInputDevice8W_SetCooperativeLevel_Original)
    {
      IDirectInputDevice8W_SetCooperativeLevel_Original (
        pMouse->pDev,
          game_window.hWnd,
            (pMouse->coop_level & (~DISCL_EXCLUSIVE)) | DISCL_NONEXCLUSIVE
      );
    }

    else if (IDirectInputDevice8A_SetCooperativeLevel_Original)
    {
      IDirectInputDevice8A_SetCooperativeLevel_Original (
        (IDirectInputDevice8A *)pMouse->pDev,
          game_window.hWnd,
            (pMouse->coop_level & (~DISCL_EXCLUSIVE)) | DISCL_NONEXCLUSIVE
      );
    }

    else return false;

    return true;
  }

  return false;
}



XINPUT_STATE di8_to_xi { };
XINPUT_STATE joy_to_xi { };

XINPUT_STATE
WINAPI
SK_JOY_TranslateToXInput (JOYINFOEX* pJoy, const JOYCAPSW* pCaps)
{
  UNREFERENCED_PARAMETER (pCaps);

  static DWORD dwPacket = 0;

  RtlZeroMemory (&joy_to_xi.Gamepad, sizeof (XINPUT_STATE::Gamepad));


  auto ComputeAxialPos_XInput =
    [ ] (UINT min, UINT max, DWORD pos) ->
    SHORT
  {
    float range  = ( static_cast <float> ( max ) -
                     static_cast <float> ( min ) );
    float center = ( static_cast <float> ( max ) +
                     static_cast <float> ( min ) ) / 2.0f;
    float rpos = 0.5f;

    if (static_cast <float> ( pos ) < center)
      rpos = center - ( center - static_cast <float> ( pos ) );
    else
      rpos = static_cast <float> ( pos ) - static_cast <float> ( min );

    auto max_xi    =
      std::numeric_limits <unsigned short>::max ( );
    auto center_xi =
      std::numeric_limits <unsigned short>::max ( ) / 2;

    return
      static_cast <SHORT> ( max_xi * ( rpos / range ) - center_xi );
  };

  auto TestTriggerThreshold_XInput =
    [ ] (UINT min, UINT max, float threshold, DWORD pos, bool& lt, bool& rt) ->
    void
  {
    float range = ( static_cast <float> ( max ) - static_cast <float> ( min ) );

    if (pos <= min + static_cast <UINT> ( threshold * range ))
      rt = true;

    if (pos >= max - static_cast <UINT> ( threshold * range ))
      lt = true;
  };

  bool lt = false,
       rt = false;

  switch (config.input.gamepad.predefined_layout)
  {
    case 0:
      if (pJoy->dwButtons & (1 << 1))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_A;
      if (pJoy->dwButtons & (1 << 2))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_B;
      if (pJoy->dwButtons & 1)
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_X;
      if (pJoy->dwButtons & (1 << 3))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_Y;

      if (pJoy->dwButtons & (1 << 9))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_START;
      if (pJoy->dwButtons & (1 << 8))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_BACK;

      if (pJoy->dwButtons & (1 << 10))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;

      if (pJoy->dwButtons & (1 << 11))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;

      if (pJoy->dwButtons & (1 << 6))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_TRIGGER;

      if (pJoy->dwButtons & (1 << 7))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_TRIGGER;

      if (pJoy->dwButtons & (1 << 4))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;

      if (pJoy->dwButtons & (1 << 5))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;

      joy_to_xi.Gamepad.sThumbLX =
        ComputeAxialPos_XInput (pCaps->wXmin, pCaps->wXmax, pJoy->dwXpos);
      joy_to_xi.Gamepad.sThumbLY =
        ComputeAxialPos_XInput (pCaps->wYmin, pCaps->wYmax, pJoy->dwYpos);

      // Invert Y-Axis for Steam controller
      joy_to_xi.Gamepad.sThumbLY = -joy_to_xi.Gamepad.sThumbLY;

      joy_to_xi.Gamepad.sThumbRX =
        ComputeAxialPos_XInput (pCaps->wRmin, pCaps->wRmax, pJoy->dwRpos);
      joy_to_xi.Gamepad.sThumbRY =
        ComputeAxialPos_XInput (pCaps->wUmin, pCaps->wUmax, pJoy->dwUpos);

      TestTriggerThreshold_XInput (
        pCaps->wZmin, pCaps->wZmax,
            0.00400f, pJoy->dwZpos, lt, rt
      );

      if (lt)
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_TRIGGER;
      if (rt)
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_TRIGGER;
      break;

    case 1:
      if (pJoy->dwButtons & (1 << 0))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_A;
      if (pJoy->dwButtons & (1 << 1))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_B;
      if (pJoy->dwButtons & (1 << 2))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_X;
      if (pJoy->dwButtons & (1 << 3))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_Y;

      if (pJoy->dwButtons & (1 << 4))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;
      if (pJoy->dwButtons & (1 << 5))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;

      if (pJoy->dwButtons & (1 << 6))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_BACK;
      if (pJoy->dwButtons & (1 << 7))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_START;

      if (pJoy->dwButtons & (1 << 8))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;

      if (pJoy->dwButtons & (1 << 9))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;

      if (pJoy->dwButtons & (1 << 10))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_TRIGGER;

      if (pJoy->dwButtons & (1 << 11))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_TRIGGER;

      joy_to_xi.Gamepad.sThumbLX =
        ComputeAxialPos_XInput (pCaps->wXmin, pCaps->wXmax, pJoy->dwXpos);
      joy_to_xi.Gamepad.sThumbLY =
        ComputeAxialPos_XInput (pCaps->wYmin, pCaps->wYmax, pJoy->dwYpos);

      // Invert Y-Axis for Steam controller
      joy_to_xi.Gamepad.sThumbLY = -joy_to_xi.Gamepad.sThumbLY;

      joy_to_xi.Gamepad.sThumbRX =
        ComputeAxialPos_XInput (pCaps->wRmin, pCaps->wRmax, pJoy->dwRpos);
      joy_to_xi.Gamepad.sThumbRY =
        ComputeAxialPos_XInput (pCaps->wUmin, pCaps->wUmax, pJoy->dwUpos);

      TestTriggerThreshold_XInput (
        pCaps->wZmin, pCaps->wZmax,
            0.00400f, pJoy->dwZpos, lt, rt
      );

      if (lt)
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_TRIGGER;
      if (rt)
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_TRIGGER;
      break;
  };

  //joy_to_xi.Gamepad.bLeftTrigger  =   (BYTE)((float)MAXBYTE  * ((float)pJoy->dwZpos / 255.0f));
  //joy_to_xi.Gamepad.bRightTrigger =  -(BYTE)((float)MAXBYTE  * ((float)pJoy->dwZpos / 255.0f));
  //joy_to_xi.Gamepad.bLeftTrigger  =   (BYTE)((float)MAXBYTE  * ((float)pJoy->dwZpos / 255.0f));
  //joy_to_xi.Gamepad.bRightTrigger =   (BYTE)((float)MAXBYTE  * ((float)pJoy->dwZpos / 255.0f));

  // One-eighth of a full rotation
  //DWORD JOY_OCTSPACE = JOY_POVRIGHT / 2;

#if 0
  // 315 - 45
  if ( ( pJoy->dwPOV >= JOY_POVLEFT   + JOY_OCTSPACE &&
         pJoy->dwPOV != JOY_POVCENTERED)
      || pJoy->dwPOV <= JOY_POVRIGHT  - JOY_OCTSPACE )
  {
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_UP;
  }

  if ( pJoy->dwPOV >= JOY_POVBACKWARD - JOY_OCTSPACE &&
       pJoy->dwPOV <= JOY_POVBACKWARD + JOY_OCTSPACE )
  {
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;
  }

  if ( pJoy->dwPOV >= JOY_POVLEFT     - JOY_OCTSPACE &&
       pJoy->dwPOV <= JOY_POVLEFT     + JOY_OCTSPACE )
  {
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;
  }

  if ( pJoy->dwPOV >= JOY_POVRIGHT    - JOY_OCTSPACE &&
       pJoy->dwPOV <= JOY_POVRIGHT    + JOY_OCTSPACE )
  {
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;
  }
#else
  // 315 - 45
  if (pJoy->dwPOV == JOY_POVFORWARD)
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_UP;

  if (pJoy->dwPOV == JOY_POVBACKWARD)
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;

  if (pJoy->dwPOV == JOY_POVLEFT)
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;

  if (pJoy->dwPOV == JOY_POVRIGHT)
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;
#endif

  joy_to_xi.dwPacketNumber = dwPacket++;

  return joy_to_xi;
}

XINPUT_STATE
WINAPI
SK_DI8_TranslateToXInput (DIJOYSTATE* pJoy)
{
  static DWORD dwPacket = 0;

  RtlZeroMemory (&di8_to_xi.Gamepad, sizeof (XINPUT_STATE::Gamepad));

  //
  // Hard-coded mappings for DualShock 4 -> XInput
  //

  if (pJoy->rgbButtons [ 9])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_START;

  if (pJoy->rgbButtons [ 8])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_BACK;

  if (pJoy->rgbButtons [10])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;

  if (pJoy->rgbButtons [11])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;

  if (pJoy->rgbButtons [ 6])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_TRIGGER;

  if (pJoy->rgbButtons [ 7])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_TRIGGER;

  if (pJoy->rgbButtons [ 4])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;

  if (pJoy->rgbButtons [ 5])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;

  if (pJoy->rgbButtons [ 1])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_A;

  if (pJoy->rgbButtons [ 2])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_B;

  if (pJoy->rgbButtons [ 0])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_X;

  if (pJoy->rgbButtons [ 3])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_Y;

  if (pJoy->rgdwPOV [0] == 0)
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_UP;

  if (pJoy->rgdwPOV [0] == 9000)
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;

  if (pJoy->rgdwPOV [0] == 18000)
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;

  if (pJoy->rgdwPOV [0] == 27000)
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;

  di8_to_xi.Gamepad.sThumbLX      =  (SHORT)((float)MAXSHORT *
                                            ((float)pJoy->lX / 255.0f));
  di8_to_xi.Gamepad.sThumbLY      = -(SHORT)((float)MAXSHORT *
                                            ((float)pJoy->lY / 255.0f));

  di8_to_xi.Gamepad.sThumbRX      =  (SHORT)((float)MAXSHORT *
                                            ((float)pJoy->lZ  / 255.0f));
  di8_to_xi.Gamepad.sThumbRY      = -(SHORT)((float)MAXSHORT *
                                            ((float)pJoy->lRz / 255.0f));

  di8_to_xi.Gamepad.bLeftTrigger  =   (BYTE)((float)MAXBYTE  *
                                            ((float)pJoy->lRx / 255.0f));
  di8_to_xi.Gamepad.bRightTrigger =   (BYTE)((float)MAXBYTE  *
                                            ((float)pJoy->lRy / 255.0f));

  di8_to_xi.dwPacketNumber = dwPacket++;

  return di8_to_xi;
}

HRESULT
WINAPI
IDirectInputDevice8_GetDeviceState_Detour ( LPDIRECTINPUTDEVICE8 This,
                                            DWORD                cbData,
                                            LPVOID               lpvData )
{
  SK_LOG_FIRST_CALL

  SK_LOG4 ( ( L" DirectInput 8 - GetDeviceState: cbData = %lu",
                cbData ),
              __SK_SUBSYSTEM__ );

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (! pTLS)
    return E_UNEXPECTED;

  HRESULT hr =
    pTLS->dinput8->hr_GetDevicestate;

  if (lpvData != nullptr)
  {
    auto
    _NeutralizeJoystickAxes = [&](LONG* pAxisData)
    {
      struct {
        DIPROPRANGE range;
        DIPROPDWORD mode;
      } static axes [] = {
        { { sizeof (DIPROPRANGE),
            sizeof (DIPROPHEADER), DIJOFS_X,
                                   DIPH_BYOFFSET,
            0, MAXLONG                         },
          { sizeof (DIPROPDWORD),
            sizeof (DIPROPHEADER), DIJOFS_X,
                                   DIPH_BYOFFSET,
            DIPROPAXISMODE_REL               } },
        { { sizeof (DIPROPRANGE),
            sizeof (DIPROPHEADER), DIJOFS_Y,
                                   DIPH_BYOFFSET,
            0, MAXLONG                         },
          { sizeof (DIPROPDWORD),
            sizeof (DIPROPHEADER), DIJOFS_Y,
                                   DIPH_BYOFFSET,
            DIPROPAXISMODE_REL               } },
        { { sizeof (DIPROPRANGE),
            sizeof (DIPROPHEADER), DIJOFS_Z,
                                   DIPH_BYOFFSET,
            0, MAXLONG                         },
          { sizeof (DIPROPDWORD),
            sizeof (DIPROPHEADER), DIJOFS_Z,
                                   DIPH_BYOFFSET,
            DIPROPAXISMODE_REL               } },
        { { sizeof (DIPROPRANGE),
            sizeof (DIPROPHEADER), DIJOFS_RX,
                                   DIPH_BYOFFSET,
            0, MAXLONG                         },
          { sizeof (DIPROPDWORD),
            sizeof (DIPROPHEADER), DIJOFS_RX,
                                   DIPH_BYOFFSET,
            DIPROPAXISMODE_ABS               } },
        { { sizeof (DIPROPRANGE),
            sizeof (DIPROPHEADER), DIJOFS_RY,
                                   DIPH_BYOFFSET,
            0, MAXLONG                         },
          { sizeof (DIPROPDWORD),
            sizeof (DIPROPHEADER), DIJOFS_RY,
                                   DIPH_BYOFFSET,
            DIPROPAXISMODE_ABS               } },
        { { sizeof (DIPROPRANGE),
            sizeof (DIPROPHEADER), DIJOFS_RZ,
                                   DIPH_BYOFFSET,
            0, MAXLONG                         },
          { sizeof (DIPROPDWORD),
            sizeof (DIPROPHEADER), DIJOFS_RZ,
                                   DIPH_BYOFFSET,
            DIPROPAXISMODE_REL               } }
      };

      for ( auto& axis : axes )
      {
        std::ignore = axis;

        if (SUCCEEDED (This->GetProperty (DIPROP_RANGE, &axis.range.diph)))
        {
          if (axis.mode.dwData == DIPROPAXISMODE_REL)
          {
            const auto mid_pt = 
              ( axis.range.lMax + axis.range.lMin ) / 2;

            *pAxisData++ =
              abs (mid_pt) > 1 ?
                   mid_pt      : 0;
          }

          else
            *pAxisData++ =
              axis.range.lMin;
        }
        else
          pAxisData++;
      }
    };

    if (cbData == sizeof (DIJOYSTATE2))
    {
      SK_DI8_READ (sk_input_dev_type::Gamepad)

      // Not only is this 1. not thread-safe, it also doesn't 2. account
      //   for multiple gamepads...
      //
      //  *** This is beyond stupid and you need to remove it :)
      //
      static DIJOYSTATE2 last_state = { };

      auto* out =
        static_cast <DIJOYSTATE2 *> (lpvData);

      bool disabled_to_game =
        SK_ImGui_WantGamepadCapture ();

      if (SUCCEEDED (hr))
      {
        if (! disabled_to_game)
          memcpy (&last_state, out, cbData);

        SK_DI8_TranslateToXInput (reinterpret_cast <DIJOYSTATE *> (out));
      }

      if (disabled_to_game || FAILED (hr))
      {
        // Not even going to bother neutralizing sliders and
        //   that fun stuff... we're reusing the last state!
        memcpy (out, &last_state, cbData);

        std::fill ( std::begin (out->rgbButtons),
                    std::end   (out->rgbButtons), 0ui8 );

        _NeutralizeJoystickAxes (&out->lX);

        // -1 = D-Pad Neutral, technically just LOWORD (-1)...
        //                       but -1L works in compliant code
        std::fill ( std::begin (out->rgdwPOV),
                    std::end   (out->rgdwPOV),
                      std::numeric_limits <DWORD>::max () );

        if (disabled_to_game)
          SK_DI8_HIDE (sk_input_dev_type::Gamepad);
      }

      else
        SK_DI8_VIEW (sk_input_dev_type::Gamepad);
    }

    else if (cbData == sizeof (DIJOYSTATE))
    {
      SK_DI8_READ (sk_input_dev_type::Gamepad)

      //dll_log.Log (L"Joy");

      static DIJOYSTATE last_state = { };

      auto* out =
        static_cast <DIJOYSTATE *> (lpvData);

      bool disabled_to_game =
        SK_ImGui_WantGamepadCapture ();

      if (SUCCEEDED (hr))
      {
        SK_DI8_TranslateToXInput (out);

        if (! disabled_to_game)
          memcpy (&last_state, out, cbData);
      }

      if (disabled_to_game || FAILED (hr))
      {
        // Not even going to bother neutralizing sliders and
        //   that fun stuff... we're reusing the last state!
        memcpy (out, &last_state, cbData);

        std::fill ( std::begin (out->rgbButtons),
                    std::end   (out->rgbButtons), 0ui8 );

        _NeutralizeJoystickAxes (&out->lX);

        // -1 = D-Pad Neutral, technically just LOWORD (-1)...
        //                       but -1L works in compliant code
        std::fill ( std::begin (out->rgdwPOV),
                    std::end   (out->rgdwPOV),
                      std::numeric_limits <DWORD>::max () );

        if (disabled_to_game)
          SK_DI8_HIDE (sk_input_dev_type::Gamepad);
      }

      else
        SK_DI8_VIEW (sk_input_dev_type::Gamepad);
    }

    else if (This == _dik8->pDev || cbData == 256)
    {
      SK_DI8_READ (sk_input_dev_type::Keyboard)

      bool disabled_to_game =
        SK_ImGui_WantKeyboardCapture ();

      if (SUCCEEDED (hr))
        memcpy (SK_Input_GetDI8Keyboard ()->state, lpvData, cbData);

      if (disabled_to_game || FAILED (hr))
      {
        RtlZeroMemory (lpvData, cbData);

        if (disabled_to_game)
          SK_DI8_HIDE (sk_input_dev_type::Keyboard);
      }

      else
        SK_DI8_VIEW (sk_input_dev_type::Keyboard);
    }

    else if ( cbData == sizeof (DIMOUSESTATE2) ||
              cbData == sizeof (DIMOUSESTATE)  )
    {
      SK_DI8_READ (sk_input_dev_type::Mouse)

      bool disabled_to_game =
        SK_ImGui_WantMouseCapture ();

      if (SUCCEEDED (hr))
        memcpy (&SK_Input_GetDI8Mouse ()->state, lpvData, cbData);

      if (disabled_to_game || FAILED (hr))
      {
        RtlZeroMemory (lpvData, cbData);

        if (disabled_to_game)
          SK_DI8_HIDE (sk_input_dev_type::Mouse);
      }

      else
        SK_DI8_VIEW (sk_input_dev_type::Mouse);
    }
  }

  return hr;
}

bool
SK_DInput8_HasKeyboard (void)
{
  return (_dik8->pDev && ( IDirectInputDevice8A_SetCooperativeLevel_Original ||
                           IDirectInputDevice8W_SetCooperativeLevel_Original ) );
}

bool
SK_DInput8_BlockWindowsKey (bool block)
{
  DWORD dwFlags =
    block ? DISCL_NOWINKEY : 0x0;

  dwFlags &= ~DISCL_EXCLUSIVE;
  dwFlags &= ~DISCL_BACKGROUND;

  dwFlags |= DISCL_NONEXCLUSIVE;
  dwFlags |= DISCL_FOREGROUND;

  if (SK_DInput8_HasKeyboard ())
  {
    if (IDirectInputDevice8W_SetCooperativeLevel_Original)
      IDirectInputDevice8W_SetCooperativeLevel_Original (                        _dik8->pDev, game_window.hWnd, dwFlags);
    else if (IDirectInputDevice8A_SetCooperativeLevel_Original)
      IDirectInputDevice8A_SetCooperativeLevel_Original ((IDirectInputDevice8A *)_dik8->pDev, game_window.hWnd, dwFlags);
    else
      return false;
  }
  else
    return false;

  return block;
}

bool
SK_DInput8_HasMouse (void)
{
  return (_dim8->pDev && ( IDirectInputDevice8A_SetCooperativeLevel_Original ||
                           IDirectInputDevice8W_SetCooperativeLevel_Original ) );
}

HRESULT
WINAPI
IDirectInputDevice8A_Poll_Detour ( IDirectInputDevice8A* This)
{
  SK_LOG_FIRST_CALL

  DIDEVICEINSTANCEA     devInst = { .dwSize = sizeof (DIDEVICEINSTANCEA) };
  This->GetDeviceInfo (&devInst);

  switch (devInst.dwDevType)
  {
    default:
    case DI8DEVTYPE_GAMEPAD:
    case DI8DEVTYPE_JOYSTICK:
      if (SK_ImGui_WantGamepadCapture ())
      {
        // Block gamepads
        if (devInst.wUsage == 0x5 && devInst.wUsagePage == 0x1)
        {
          SK_RunOnce (
            SK_LOGi0 (
              L"Prevented DirectInput from polling a gamepad because "
              L"gamepad input is currently blocked by SK."
            )
          );

          return DI_NOEFFECT;
        }

        else
          SK_RunOnce (
            SK_LOGi0 (
              "Unexpected Usage (%x) and/or UsagePage (%x) for %hs",
                devInst.wUsage, devInst.wUsagePage, devInst.tszProductName)
          );
      }

    case DI8DEVTYPE_MOUSE:
    case DI8DEVTYPE_KEYBOARD:
      break;
  }

  return
    IDirectInputDevice8A_Poll_Original (This);
}

HRESULT
WINAPI
IDirectInputDevice8W_Poll_Detour ( IDirectInputDevice8W* This)
{
  SK_LOG_FIRST_CALL

  DIDEVICEINSTANCEW     devInst = { .dwSize = sizeof (DIDEVICEINSTANCEW) };
  This->GetDeviceInfo (&devInst);

  switch (devInst.dwDevType)
  {
    default:
    case DI8DEVTYPE_GAMEPAD:
    case DI8DEVTYPE_JOYSTICK:
      if (SK_ImGui_WantGamepadCapture ())
      {
        // Block gamepads
        if (devInst.wUsage == 0x5 && devInst.wUsagePage == 0x1)
        {
          SK_RunOnce (
            SK_LOGi0 (
              L"Prevented DirectInput from polling a gamepad because "
              L"gamepad input is currently blocked by SK."
            )
          );

          return DI_NOEFFECT;
        }

        else
          SK_RunOnce (
            SK_LOGi0 (
              "Unexpected Usage (%x) and/or UsagePage (%x) for %ws",
                devInst.wUsage, devInst.wUsagePage, devInst.tszProductName)
          );
      }

    case DI8DEVTYPE_MOUSE:
    case DI8DEVTYPE_KEYBOARD:
      break;
  }

  return
    IDirectInputDevice8W_Poll_Original (This);
}

HRESULT
WINAPI
IDirectInputDevice8A_GetDeviceState_Detour ( LPDIRECTINPUTDEVICE8A This,
                                             DWORD                 cbData,
                                             LPVOID                lpvData )
{
  SK_LOG_FIRST_CALL

  SK_TLS_Bottom ()->dinput8->hr_GetDevicestate =
    IDirectInputDevice8A_GetDeviceState_Original ( This, cbData, lpvData );

  return
    IDirectInputDevice8_GetDeviceState_Detour ( (LPDIRECTINPUTDEVICE8)This, cbData, lpvData );
}

HRESULT
WINAPI
IDirectInputDevice8W_GetDeviceState_Detour ( LPDIRECTINPUTDEVICE8W      This,
                                             DWORD                      cbData,
                                             LPVOID                     lpvData )
{
  SK_LOG_FIRST_CALL

  SK_TLS_Bottom ()->dinput8->hr_GetDevicestate =
    IDirectInputDevice8W_GetDeviceState_Original ( This, cbData, lpvData );

  return
    IDirectInputDevice8_GetDeviceState_Detour ( (LPDIRECTINPUTDEVICE8)This, cbData, lpvData );
}



HRESULT
WINAPI
IDirectInputDevice8W_SetCooperativeLevel_Detour ( LPDIRECTINPUTDEVICE8W This,
                                                  HWND                  hwnd,
                                                  DWORD                 dwFlags )
{
  SK_LOG_FIRST_CALL

  bool bMouse    = ( This == _dim8->pDev );
  bool bKeyboard = ( This == _dik8->pDev );

  if (! (bMouse || bKeyboard))
  {
    if (config.window.background_render)
    {
      dwFlags &= ~DISCL_FOREGROUND;
      dwFlags |=  DISCL_BACKGROUND;
    }
  }

  if (bKeyboard)
  {
    dwFlags &= ~DISCL_EXCLUSIVE;
    dwFlags &= ~DISCL_BACKGROUND;

    dwFlags |= DISCL_NONEXCLUSIVE;
    dwFlags |= DISCL_FOREGROUND;

    // We will implement this ourselves with a low-level keyboard hook
    config.input.keyboard.dinput_win_key =
      (dwFlags & DISCL_NOWINKEY) ? SK_Disabled
                                 : SK_Enabled;

    // Strip the actual flag
    if (     config.input.keyboard.enable_win_key == SK_Enabled)
      dwFlags &= ~DISCL_NOWINKEY;
    else if (config.input.keyboard.enable_win_key == SK_Disabled)
      dwFlags |=  DISCL_NOWINKEY;
  }

  HRESULT hr =
    IDirectInputDevice8W_SetCooperativeLevel_Original (This, hwnd, dwFlags);

  if (SUCCEEDED (hr))
  {
    // Mouse
    if (bMouse)
      _dim8->coop_level = dwFlags;

    // Keyboard   (why do people use DirectInput for keyboards? :-\)
    else if (bKeyboard)
      _dik8->coop_level = dwFlags;

    // Anything else is probably not important
  }

  // SK's UI needs to be able to read this
  if (SK_ImGui_WantMouseCapture ())
  {
    dwFlags &= ~DISCL_EXCLUSIVE;
    dwFlags |=  DISCL_NONEXCLUSIVE;

    IDirectInputDevice8W_SetCooperativeLevel_Original (This, hwnd, dwFlags);
  }

  return hr;
}

HRESULT
WINAPI
IDirectInputDevice8A_SetCooperativeLevel_Detour ( LPDIRECTINPUTDEVICE8A This,
                                                  HWND                  hwnd,
                                                  DWORD                 dwFlags )
{
  SK_LOG_FIRST_CALL

  bool bMouse    = ( This == (IDirectInputDevice8A *)_dim8->pDev );
  bool bKeyboard = ( This == (IDirectInputDevice8A *)_dik8->pDev );

  if (! (bMouse || bKeyboard))
  {
    if (config.window.background_render)
    {
      dwFlags &= ~DISCL_FOREGROUND;
      dwFlags |=  DISCL_BACKGROUND;
    }
  }

  if (bKeyboard)
  {
    dwFlags &= ~DISCL_EXCLUSIVE;
    dwFlags &= ~DISCL_BACKGROUND;

    dwFlags |= DISCL_NONEXCLUSIVE;
    dwFlags |= DISCL_FOREGROUND;

    // We will implement this ourselves with a low-level keyboard hook
    config.input.keyboard.dinput_win_key =
      (dwFlags & DISCL_NOWINKEY) ? SK_Disabled
                                 : SK_Enabled;

    // Strip the actual flag
    if (     config.input.keyboard.enable_win_key == SK_Enabled)
      dwFlags &= ~DISCL_NOWINKEY;
    else if (config.input.keyboard.enable_win_key == SK_Disabled)
      dwFlags |=  DISCL_NOWINKEY;
  }

  HRESULT hr =
    IDirectInputDevice8A_SetCooperativeLevel_Original (This, hwnd, dwFlags);

  if (SUCCEEDED (hr))
  {
    // Mouse
    if (bMouse)
      _dim8->coop_level = dwFlags;

    // Keyboard   (why do people use DirectInput for keyboards? :-\)
    else if (bKeyboard)
      _dik8->coop_level = dwFlags;

    // Anything else is probably not important
  }

  // SK's UI needs to be able to read this

  if (SK_ImGui_WantMouseCapture ())
  {
    dwFlags &= ~DISCL_EXCLUSIVE;
    dwFlags |=  DISCL_NONEXCLUSIVE;

    IDirectInputDevice8A_SetCooperativeLevel_Original (This, hwnd, dwFlags);
  }

  return hr;
}



static SK_LazyGlobal <
  concurrency::concurrent_unordered_map <uint32_t, LPDIRECTINPUTDEVICE8W>
> devices8_w;
static SK_LazyGlobal <
  concurrency::concurrent_unordered_map <uint32_t, LPDIRECTINPUTDEVICE8A>
> devices8_a;

HRESULT
WINAPI
IDirectInput8W_CreateDevice_Detour ( IDirectInput8W        *This,
                                     REFGUID                rguid,
                                     LPDIRECTINPUTDEVICE8W *lplpDirectInputDevice,
                                     LPUNKNOWN              pUnkOuter )
{
  // Any device not in this giant mess is one Special K doesn't understand and
  //   should not be hooking...
  bool hookable =
    ( rguid == GUID_SysMouse      || rguid == GUID_SysKeyboard    ||
      rguid == GUID_SysMouseEm    || rguid == GUID_SysMouseEm2    ||
      rguid == GUID_SysKeyboardEm || rguid == GUID_SysKeyboardEm2 ||
      rguid == GUID_Joystick );

#if 0
  if (! hookable)
    return DIERR_DEVICENOTREG;
#endif

  std::vector <void *> fns_to_hook;

  hookable = true;

  uint32_t guid_crc32c = crc32c (0, &rguid, sizeof (REFGUID));

  const wchar_t* wszDevice = (rguid == GUID_SysKeyboard)   ? L"Default System Keyboard" :
                                (rguid == GUID_SysMouse)   ? L"Default System Mouse"    :
                                  (rguid == GUID_Joystick) ? L"Gamepad / Joystick"      :
                                                             L"Other Device";

  static auto& _devices_w =
               devices8_w.get ();

  if (hookable && _devices_w.count (guid_crc32c) && lplpDirectInputDevice != nullptr)
  {
    *lplpDirectInputDevice = _devices_w [guid_crc32c];
                             _devices_w [guid_crc32c]->AddRef ();
    return S_OK;
  }


  if (config.system.log_level > 1)
  {
    dll_log->Log ( L"[   Input  ] [!] IDirectInput8W::CreateDevice ("
                                      L"%08" _L(PRIxPTR) L"h, %s,"
                                      L"%08" _L(PRIxPTR) L"h, "
                                      L"%08" _L(PRIxPTR) L"h)",
                      (uintptr_t)This,
                                   wszDevice,
                          (uintptr_t)lplpDirectInputDevice,
                            (uintptr_t)pUnkOuter );
  }
  else
  {
    dll_log->Log ( L"[   Input  ] [!] IDirectInput8W::CreateDevice         [ %24s ]",
                        wszDevice );
  }

  HRESULT hr;
  DINPUT8_CALL ( hr,
                  IDirectInput8W_CreateDevice_Original ( This,
                                                          rguid,
                                                           lplpDirectInputDevice,
                                                            pUnkOuter ) );

  if (SUCCEEDED (hr) &&  lplpDirectInputDevice != nullptr &&
                        *lplpDirectInputDevice != nullptr)
  {
    void** vftable =
      *reinterpret_cast <void ***> (*lplpDirectInputDevice);

    if (hookable)
    {
      if (IDirectInputDevice8W_GetDeviceState_Original == nullptr)
      {
        if ( MH_OK ==
          SK_CreateFuncHook (      L"IDirectInputDevice8W::GetDeviceState",
                                     vftable [9],
                                     IDirectInputDevice8W_GetDeviceState_Detour,
            static_cast_p2p <void> (&IDirectInputDevice8W_GetDeviceState_Original) ) )
              fns_to_hook.push_back (vftable [9]);
      }

      if (! IDirectInputDevice8W_SetCooperativeLevel_Original)
      {
        if ( MH_OK ==
          SK_CreateFuncHook (      L"IDirectInputDevice8W::SetCooperativeLevel",
                                     vftable [13],
                                     IDirectInputDevice8W_SetCooperativeLevel_Detour,
            static_cast_p2p <void> (&IDirectInputDevice8W_SetCooperativeLevel_Original) ) )
              fns_to_hook.push_back (vftable [13]);
      }

      if (! IDirectInputDevice8W_Poll_Original)
      {
        if ( MH_OK ==
          SK_CreateFuncHook (      L"IDirectInputDevice8W::Poll",
                                     vftable [25],
                                     IDirectInputDevice8W_Poll_Detour,
            static_cast_p2p <void> (&IDirectInputDevice8W_Poll_Original) ) )
              fns_to_hook.push_back (vftable [25]);
      }

      if (rguid == GUID_SysMouse)
      {
        _dim8->pDev = *lplpDirectInputDevice;
      }
      else if (rguid == GUID_SysKeyboard)
        _dik8->pDev = *lplpDirectInputDevice;

      devices8_w [guid_crc32c] = *lplpDirectInputDevice;
      devices8_w [guid_crc32c]->AddRef ();
    }
  }

#if 0
  if (SUCCEEDED (hr) && lplpDirectInputDevice != nullptr)
  {
    DWORD dwFlag = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE;

    if (config.input.block_windows)
      dwFlag |= DISCL_NOWINKEY;

    (*lplpDirectInputDevice)->SetCooperativeLevel (SK_GetGameWindow (), dwFlag);
  }
#endif

  if (! fns_to_hook.empty ())
  {
    auto suspend_ctx = SK_SuspendAllOtherThreads ();

    for (auto fn : fns_to_hook)
    {
      SK_EnableHook (fn);
    }

    SK_ResumeThreads (suspend_ctx);
  }

  return hr;
}

HRESULT
WINAPI
IDirectInput8A_CreateDevice_Detour ( IDirectInput8A        *This,
                                     REFGUID                rguid,
                                     LPDIRECTINPUTDEVICE8A *lplpDirectInputDevice,
                                     LPUNKNOWN              pUnkOuter )
{
  // Any device not in this giant mess is one Special K doesn't understand and
  //   should not be hooking...
  bool hookable =
    ( rguid == GUID_SysMouse      || rguid == GUID_SysKeyboard    ||
      rguid == GUID_SysMouseEm    || rguid == GUID_SysMouseEm2    ||
      rguid == GUID_SysKeyboardEm || rguid == GUID_SysKeyboardEm2 ||
      rguid == GUID_Joystick );

#if 0
  if (! hookable)
    return DIERR_DEVICENOTREG;
#endif

  hookable = true;

  std::vector <void *> fns_to_hook;

  uint32_t guid_crc32c = crc32c (0, &rguid, sizeof (REFGUID));

  const wchar_t* wszDevice = (rguid == GUID_SysKeyboard)   ? L"Default System Keyboard" :
                                (rguid == GUID_SysMouse)   ? L"Default System Mouse"    :
                                  (rguid == GUID_Joystick) ? L"Gamepad / Joystick"      :
                                                             L"Other Device";

  static auto& _devices_a =
               devices8_a.get ();

  if (hookable && _devices_a.count (guid_crc32c) && lplpDirectInputDevice != nullptr)
  {
    *lplpDirectInputDevice = _devices_a [guid_crc32c];
                             _devices_a [guid_crc32c]->AddRef ();
    return S_OK;
  }

  if (config.system.log_level > 1)
  {
    dll_log->Log ( L"[   Input  ] [!] IDirectInput8A::CreateDevice ("
                                      L"%08" _L(PRIxPTR) L"h, %s,"
                                      L"%08" _L(PRIxPTR) L"h, "
                                      L"%08" _L(PRIxPTR) L"h)",
                      (uintptr_t)This,
                                   wszDevice,
                          (uintptr_t)lplpDirectInputDevice,
                            (uintptr_t)pUnkOuter );
  }
  else
  {
    dll_log->Log ( L"[   Input  ] [!] IDirectInput8A::CreateDevice         [ %24s ]",
                        wszDevice );
  }

  HRESULT hr;
  DINPUT8_CALL ( hr,
                  IDirectInput8A_CreateDevice_Original ( This,
                                                          rguid,
                                                           lplpDirectInputDevice,
                                                            pUnkOuter ) );

  if (SUCCEEDED (hr) &&  lplpDirectInputDevice != nullptr &&
                        *lplpDirectInputDevice != nullptr)
  {
    void** vftable =
      *reinterpret_cast <void ***> (*lplpDirectInputDevice);

    if (hookable)
    {
      if (IDirectInputDevice8A_GetDeviceState_Original == nullptr)
      {
        if ( MH_OK ==
          SK_CreateFuncHook (      L"IDirectInputDevice8A::GetDeviceState",
                                     vftable [9],
                                     IDirectInputDevice8A_GetDeviceState_Detour,
            static_cast_p2p <void> (&IDirectInputDevice8A_GetDeviceState_Original) ) )
              fns_to_hook.push_back (vftable [9]);
      }

      if (! IDirectInputDevice8A_SetCooperativeLevel_Original)
      {
        if ( MH_OK ==
          SK_CreateFuncHook (      L"IDirectInputDevice8A::SetCooperativeLevel",
                                     vftable [13],
                                     IDirectInputDevice8A_SetCooperativeLevel_Detour,
            static_cast_p2p <void> (&IDirectInputDevice8A_SetCooperativeLevel_Original) ) )
              fns_to_hook.push_back (vftable [13]);
      }

      if (! IDirectInputDevice8A_Poll_Original)
      {
        if ( MH_OK ==
          SK_CreateFuncHook (      L"IDirectInputDevice8A::Poll",
                                     vftable [25],
                                     IDirectInputDevice8A_Poll_Detour,
            static_cast_p2p <void> (&IDirectInputDevice8A_Poll_Original) ) )
              fns_to_hook.push_back (vftable [25]);
      }

      if (rguid == GUID_SysMouse)
      {
        _dim8->pDev = *(LPDIRECTINPUTDEVICE8W *)lplpDirectInputDevice;
      }
      else if (rguid == GUID_SysKeyboard)
        _dik8->pDev = *(LPDIRECTINPUTDEVICE8W *)lplpDirectInputDevice;

      devices8_a [guid_crc32c] = *lplpDirectInputDevice;
      devices8_a [guid_crc32c]->AddRef ();
    }
  }

#if 0
  if (SUCCEEDED (hr) && lplpDirectInputDevice != nullptr)
  {
    DWORD dwFlag = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE;

    if (config.input.block_windows)
      dwFlag |= DISCL_NOWINKEY;

    (*lplpDirectInputDevice)->SetCooperativeLevel (
      SK_GetGameWindow (), dwFlag
    );
  }
#endif

  if (! fns_to_hook.empty ())
  {
    auto suspend_ctx = SK_SuspendAllOtherThreads ();

    for (auto fn : fns_to_hook)
    {
      SK_EnableHook (fn);
    }

    SK_ResumeThreads (suspend_ctx);
  }

  return hr;
}

using GUIDFromStringW_pfn = BOOL (WINAPI *)(_In_  LPCWSTR psz,
                                            _Out_ LPGUID  pguid);

static GUIDFromStringW_pfn
       GUIDFromStringW = nullptr;

BOOL
WINAPI
SK_GUIDFromStringW (LPCWSTR psz, LPGUID pguid)
{
  if (GUIDFromStringW == nullptr)
  {
    static HMODULE   hMod_shlwapi =
      GetModuleHandleW (L"shlwapi.dll");

    if ((intptr_t)hMod_shlwapi > 0)
    {
      GUIDFromStringW = (GUIDFromStringW_pfn)GetProcAddress (hMod_shlwapi, (LPCSTR)MAKEINTRESOURCE (270));

      if (GUIDFromStringW != nullptr)
        return GUIDFromStringW (psz, pguid);
    }
  }

  return FALSE;
}

struct enum_devices_callback_detour_a {
  LPDIENUMDEVICESCALLBACKA fn;
  LPVOID                   puser;
};

BOOL
WINAPI
DI8_CallbackEnumDevicesA (LPCDIDEVICEINSTANCEA dev_inst, LPVOID pUser)
{
  //SK_LOGs0 (L"DInput8", L"%hs", dev_inst->tszProductName);

  //if (dev_inst->wUsage == 0x5 && dev_inst->wUsagePage == 0x1)
  //{
  //  return TRUE;
  //}

  if (StrStrIA (dev_inst->tszProductName, "Controller (XBOX 360 For Windows)"))
  {
    OLECHAR wszInstance [128] = { };
    OLECHAR wszProduct  [128] = { };
    OLECHAR wszFFDriver [128] = { };

    HRESULT hr =
      StringFromGUID2 (dev_inst->guidInstance, wszInstance, 127);
            hr =
      StringFromGUID2 (dev_inst->guidProduct,  wszProduct,  127);
            hr =
      StringFromGUID2 (dev_inst->guidFFDriver, wszFFDriver, 127);

    SK_LOGs1 (
      L"DInput8",
      L"dwSize: %d, guidInstance=%ws, guidProduct=%ws, dwDevType=%d, InstanceName=%hs, ProductName=%hs, FFDriver=%hs, UsagePage=%d, Usage=%d",
      dev_inst->dwSize, wszInstance, wszProduct, dev_inst->dwDevType, dev_inst->tszInstanceName, dev_inst->tszProductName, wszFFDriver, dev_inst->wUsagePage, dev_inst->wUsage
    );
  }

  enum_devices_callback_detour_a* detour =
    (enum_devices_callback_detour_a *)pUser;

  return
    detour->fn (dev_inst, detour->puser);
}

struct enum_devices_callback_detour_w {
  LPDIENUMDEVICESCALLBACKW fn;
  LPVOID                   puser;
};

BOOL
WINAPI
DI8_CallbackEnumDevicesW (LPCDIDEVICEINSTANCEW dev_inst, LPVOID pUser)
{
  //SK_LOGs0 (L"DInput8", L"%hs", dev_inst->tszProductName);

  //if (dev_inst->wUsage == 0x5 && dev_inst->wUsagePage == 0x1)
  //{
  //  return TRUE;
  //}

  if (StrStrIW (dev_inst->tszProductName, L"Controller (XBOX 360 For Windows)"))
  {
    OLECHAR wszInstance [128] = { };
    OLECHAR wszProduct  [128] = { };
    OLECHAR wszFFDriver [128] = { };

    HRESULT hr =
      StringFromGUID2 (dev_inst->guidInstance, wszInstance, 127);
            hr =
      StringFromGUID2 (dev_inst->guidProduct,  wszProduct,  127);
            hr =
      StringFromGUID2 (dev_inst->guidFFDriver, wszFFDriver, 127);

    SK_LOGs1 (
      L"DInput8",
      L"dwSize: %d, guidInstance=%ws, guidProduct=%ws, dwDevType=%d, InstanceName=%ws, ProductName=%ws, FFDriver=%hs, UsagePage=%d, Usage=%d",
      dev_inst->dwSize, wszInstance, wszProduct, dev_inst->dwDevType, dev_inst->tszInstanceName, dev_inst->tszProductName, wszFFDriver, dev_inst->wUsagePage, dev_inst->wUsage
    );
  }

  enum_devices_callback_detour_w* detour =
    (enum_devices_callback_detour_w *)pUser;

  return
    detour->fn (dev_inst, detour->puser);
}

HRESULT
WINAPI
IDirectInput8A_EnumDevices_Detour ( IDirectInput8A*          This,
                                    DWORD                    dwDevType,
                                    LPDIENUMDEVICESCALLBACKA lpCallback,
                                    LPVOID                   pvRef,
                                    DWORD                    dwFlags )
{
  SK_LOG_FIRST_CALL

  if (config.input.gamepad.dinput.block_enum_devices)
    return E_NOTIMPL;


#if 0
  if (lpCallback != nullptr && config.input.gamepad.xinput.emulate)
  {
    DIDEVICEINSTANCEA dev_inst;
                      dev_inst.dwSize     = sizeof (dev_inst);

                      SK_GUIDFromStringW (L"{375A5B10-2B71-11EC-8001-444553540000}", &dev_inst.guidInstance);
                      SK_GUIDFromStringW (L"{028E045E-0000-0000-0000-504944564944}", &dev_inst.guidProduct);

                      dev_inst.dwDevType  = 66069;
              strcpy (dev_inst.tszInstanceName, "Controller (XBOX 360 For Windows)");
              strcpy (dev_inst.tszProductName,  "Controller (XBOX 360 For Windows)");
                      dev_inst.wUsage     = 5;
                      dev_inst.wUsagePage = 1;

    lpCallback (&dev_inst, pvRef);
  }
#endif

  enum_devices_callback_detour_a
    detour_callback { .fn    = lpCallback,
                      .puser = pvRef };

  return
    IDirectInput8A_EnumDevices_Original ( This, dwDevType,
                                            DI8_CallbackEnumDevicesA, &detour_callback,
                                              dwFlags );
}

HRESULT
WINAPI
IDirectInput8W_EnumDevices_Detour ( IDirectInput8W*          This,
                                    DWORD                    dwDevType,
                                    LPDIENUMDEVICESCALLBACKW lpCallback,
                                    LPVOID                   pvRef,
                                    DWORD                    dwFlags )
{
  SK_LOG_FIRST_CALL

  if (config.input.gamepad.dinput.block_enum_devices)
    return E_NOTIMPL;

#if 0
  if (lpCallback != nullptr && config.input.gamepad.xinput.emulate)
  {
    DIDEVICEINSTANCEW dev_inst;
                      dev_inst.dwSize     = sizeof (dev_inst);

                      SK_GUIDFromStringW (L"{375A5B10-2B71-11EC-8001-444553540000}", &dev_inst.guidInstance);
                      SK_GUIDFromStringW (L"{028E045E-0000-0000-0000-504944564944}", &dev_inst.guidProduct);

                      dev_inst.dwDevType  = 66069;
              wcscpy (dev_inst.tszInstanceName, L"Controller (XBOX 360 For Windows)");
              wcscpy (dev_inst.tszProductName,  L"Controller (XBOX 360 For Windows)");
                      dev_inst.wUsage     = 5;
                      dev_inst.wUsagePage = 1;

    lpCallback (&dev_inst, pvRef);
  }
#endif

  enum_devices_callback_detour_w
  detour_callback { .fn    = lpCallback,
                    .puser = pvRef };

  return
    IDirectInput8W_EnumDevices_Original ( This, dwDevType,
                                            DI8_CallbackEnumDevicesW, &detour_callback,
                                              dwFlags );
}

void
SK_Input_HookDI8 (void)
{
  if (! config.input.gamepad.hook_dinput8)
    return;

  if (! SK_GetModuleHandle (L"dinput8.dll"))
           SK_LoadLibraryW (L"dinput8.dll");

  if (SK_GetModuleHandle (L"dinput8.dll") && InterlockedCompareExchangePointer ((void **)&DirectInput8Create_Import, (void*)1, nullptr) == nullptr)
  {
    SK_PROFILE_FIRST_CALL

    static volatile LONG               hooked    =   FALSE;
    if (! InterlockedCompareExchange (&hooked, TRUE, FALSE))
    {
      // DirectInput is layered on top of HID, so we should start
      //   by hooking HID.
      SK_Input_HookHID ();

      if (SK_GetDLLRole () & DLL_ROLE::DInput8)
      {
        InterlockedIncrement (&hooked);
        return;
      }

      SK_LOG0 ( ( L"Game uses DirectInput 8, installing input hooks..." ),
                    L"  Input   " );

      //HMODULE hBackend =
      //  (SK_GetDLLRole () & DLL_ROLE::DInput8) ? backend_dll :
      //                                  SK_GetModuleHandle (L"dinput8.dll");

      if (SK_GetProcAddress (SK_GetModuleHandle (L"dinput8.dll"), "DirectInput8Create"))
      {
        SK_CreateDLLHook2 (      L"dinput8.dll",
                                  "DirectInput8Create",
                                   DirectInput8Create,
          static_cast_p2p <void> (&DirectInput8Create_Import) );
      }

      InterlockedIncrementRelease (&hooked);

      if (SK_GetModuleHandle (L"dinput.dll"))
      {
        SK_Input_HookDI7 ();
      }
    }

    else
      SK_Thread_SpinUntilAtomicMin (&hooked, 2);
  }
}


void
SK_Input_PreHookDI8 (void)
{
  SK_PROFILE_FIRST_CALL

  if (SK_GetDLLRole () & DLL_ROLE::DInput8)
  {
    SK_BootDI8 ();

    if (SK_GetModuleHandle (L"dinput.dll"))
      SK_Input_HookDI7 ();
  }

  else
  {
    if (! config.input.gamepad.hook_dinput8)
      return;

    static sk_import_test_s tests [] = { { "dinput.dll",  false },
                                         { "dinput8.dll", false } };

    SK_TestImports (SK_GetModuleHandle (nullptr), tests, 2);

    if (tests [1].used || SK_GetModuleHandle (L"dinput8.dll"))
    {
      if (SK_GetDLLRole () != DLL_ROLE::DInput8)
        SK_Input_HookDI8 ();
    }

    if (tests [0].used || SK_GetModuleHandle (L"dinput.dll"))
    {
      SK_Modules->LoadLibraryLL (L"dinput.dll");
                         SK_Input_PreHookDI7 ();
    }
  }
}

SK_LazyGlobal <sk_input_api_context_s> SK_DI8_Backend;