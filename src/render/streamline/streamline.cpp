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
#define __SK_SUBSYSTEM__ L"Streamline"

using  sl_DXGIGetDebugInterface1_pfn = HRESULT (WINAPI *)(UINT   Flags, REFIID riid, _Out_ void **pDebug);
static sl_DXGIGetDebugInterface1_pfn
       sl_DXGIGetDebugInterface1_Original = nullptr;

HRESULT
WINAPI
sl_DXGIGetDebugInterface1_Detour (
        UINT   Flags,
        REFIID riid,
  _Out_ void   **pDebug )
{
  wchar_t                wszGUID [41] = { };
  StringFromGUID2 (riid, wszGUID, 40);

  SK_LOGi0 ( L"sl_DXGIGetDebugInterface1 (%x, %ws, %p)", Flags, riid, pDebug );

  return
    sl_DXGIGetDebugInterface1_Original (Flags, riid, pDebug);
}

void
SK_Streamline_InitBypass (void)
{
#if 0
  SK_CreateDLLHook2 ( L"sl.interposer.dll",
                       "DXGIGetDebugInterface1",
                     sl_DXGIGetDebugInterface1_Detour,
           (void **)&sl_DXGIGetDebugInterface1_Original );

  SK_ApplyQueuedHooks ();
#endif
}

using  slIsFeatureSupported_pfn = sl::Result (*)(sl::Feature feature, const sl::AdapterInfo& adapterInfo);
static slIsFeatureSupported_pfn
       slIsFeatureSupported_Original = nullptr;

using  slInit_pfn = sl::Result (*)(sl::Preferences *pref, uint64_t sdkVersion);
static slInit_pfn
       slInit_Original = nullptr;

sl::Result
slIsFeatureSupported_Detour (sl::Feature feature, const sl::AdapterInfo& adapterInfo)
{
  std::ignore = feature;
  std::ignore = adapterInfo;

  return sl::Result::eOk;
}

sl::Result
slInit_Detour (sl::Preferences *pref, uint64_t sdkVersion = sl::kSDKVersion)
{
  SK_LOG_FIRST_CALL

  SK_LOGi0 (L"[!] slInit (pref->structVersion=%d, sdkVersion=%d)",
                          pref->structVersion,    sdkVersion);

  // For versions of Streamline using a compatible preferences struct,
  //   start overriding stuff for compatibility.
  if (pref->structVersion <= sl::kStructVersion1)
  {
    //
    // Always print Streamline debug output to Special K's game_output.log,
    //   disable any log redirection the game would ordinarily do on its own.
    //
    if (config.nvidia.dlss.streamline_dbg_out)
    {
      pref->logMessageCallback = nullptr;
#ifdef _DEBUG
      pref->logLevel           = sl::LogLevel::eVerbose;
#else
      pref->logLevel           = sl::LogLevel::eDefault;
#endif
    }

    // Make forcing proxies into an option
    //pref->flags |=
    //  sl::PreferenceFlags::eUseDXGIFactoryProxy;

    if (SK_IsCurrentGame (SK_GAME_ID::AssassinsCreed_Shadows))
    {
      // Fix Assassin's Creed Shadows not passing sl::PreferenceFlags::eLoadDownloadedPlugins
      pref->flags |= (sl::PreferenceFlags::eAllowOTA |
                      sl::PreferenceFlags::eLoadDownloadedPlugins);
      pref->flags |=  sl::PreferenceFlags::eBypassOSVersionCheck;
      pref->flags |=  sl::PreferenceFlags::eUseDXGIFactoryProxy;
    }

    return
      slInit_Original (pref, sdkVersion);
  }

  SK_LOGi0 (
    L"WARNING: Game is using a version of Streamline too new for Special K!"
  );

  return
    slInit_Original (pref, sdkVersion);
}

void
SK_COMPAT_InstallStreamlineHooks (const wchar_t* sl_interposer_dll)
{
  SK_PROFILE_FIRST_CALL

  SK_CreateDLLHook (         sl_interposer_dll,
                            "slInit",
                             slInit_Detour,
    static_cast_p2p <void> (&slInit_Original));

  // Feature Spoofing
/*
    SK_CreateDLLHook (        sl_interposer_dll,
                             "slIsFeatureSupported",
                              slIsFeatureSupported_Detour,
     static_cast_p2p <void> (&slIsFeatureSupported_Original));
*/
}

bool
SK_COMPAT_CheckStreamlineSupport (HMODULE hModInterposer)
{
  if (hModInterposer != nullptr)
  {
    SK_RunOnce (
      SK_COMPAT_InstallStreamlineHooks (
        SK_GetModuleFullName (hModInterposer).c_str ()
      )
    );
  }

  //
  // As of 23.9.13, basic compatibility in all known games is perfect!
  //

  return true;
}

// It is never necessary to call this, it can be implemented using QueryInterface
using slGetNativeInterface_pfn = sl::Result (*)(void* proxyInterface, void** baseInterface);
// This, on the other hand, requires an import from sl.interposer.dll
using slUpgradeInterface_pfn   = sl::Result (*)(                      void** baseInterface);

struct DECLSPEC_UUID ("ADEC44E2-61F0-45C3-AD9F-1B37379284FF")
  IStreamlineBaseInterface : IUnknown { };

sl::Result
SK_slGetNativeInterface (void *proxyInterface, void **baseInterface)
{
#if 0
  if (proxyInterface == nullptr || baseInterface == nullptr)
    return sl::Result::eErrorMissingInputParameter;

  IUnknown* pUnk =
    static_cast <IUnknown *> (proxyInterface);

  if (FAILED (pUnk->QueryInterface (__uuidof (IStreamlineBaseInterface), baseInterface)))
    return sl::Result::eErrorUnsupportedInterface;
#else
  sl::Result result;

  static
      slGetNativeInterface_pfn
      slGetNativeInterface  = nullptr;
  if (slGetNativeInterface == nullptr && SK_GetFramesDrawn () < 240)
      slGetNativeInterface =
     (slGetNativeInterface_pfn)SK_GetProcAddress (L"sl.interposer.dll",
     "slGetNativeInterface"); result =
      slGetNativeInterface != nullptr                      ?
      slGetNativeInterface (proxyInterface, baseInterface) :
                          sl::Result::eErrorNotInitialized ;

  return result;
#endif
}

sl::Result
SK_slUpgradeInterface (void **baseInterface)
{
#if 0
  if (baseInterface == nullptr)
    return sl::Result::eErrorMissingInputParameter;

  IUnknown* pUnkInterface = *(IUnknown **)baseInterface;
  void*     pProxy        = nullptr;

  if (SUCCEEDED (pUnkInterface->QueryInterface (__uuidof (IStreamlineBaseInterface), &pProxy)))
  {
    // The passed interface already has a proxy, do nothing...
    static_cast <IUnknown *> (pProxy)->Release ();

    return sl::Result::eOk;
  }

  // If slInit (...) has not been called yet, sl.common.dll will not be present 
  if (! SK_IsModuleLoaded (L"sl.common.dll"))
    return sl::Result::eErrorInitNotCalled;

  auto slUpgradeInterface =
      (slUpgradeInterface_pfn)SK_GetProcAddress (L"sl.interposer.dll",
      "slUpgradeInterface");

  if (slUpgradeInterface != nullptr)
  {
    auto result =
      slUpgradeInterface (baseInterface);

    if (result == sl::Result::eOk)
    {
      static HMODULE
          hModPinnedInterposer  = nullptr,   hModPinnedCommon  = nullptr;
      if (hModPinnedInterposer == nullptr || hModPinnedCommon == nullptr)
      {
        // Once we have done this, there's no going back...
        //   we MUST pin the interposer DLL permanently.
        const BOOL bPinnedSL =
           (nullptr != hModPinnedInterposer || GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_PIN, L"sl.interposer.dll", &hModPinnedInterposer))
        && (nullptr != hModPinnedCommon     || GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_PIN,     L"sl.common.dll", &hModPinnedCommon    ));

        if (! bPinnedSL)
          SK_LOGi0 (L"Streamline Integration Has Invalid State!");
      }
    }

    return result;
  }
#else
  sl::Result result;

  static
      slUpgradeInterface_pfn
      slUpgradeInterface  = nullptr;
  if (slUpgradeInterface == nullptr/* && SK_GetFramesDrawn() < 240*/)
      slUpgradeInterface =
     (slUpgradeInterface_pfn)SK_GetProcAddress (L"sl.interposer.dll",
     "slUpgradeInterface"); result =
      slUpgradeInterface != nullptr      ?
      slUpgradeInterface (baseInterface) :
        sl::Result::eErrorNotInitialized ;

  return result;
#endif
}