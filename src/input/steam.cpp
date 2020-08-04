#include <SpecialK/stdafx.h>

#include <SpecialK/input/steam.h>

SK_LazyGlobal <sk_input_api_context_s> SK_Steam_Backend;

#ifdef SK_STEAM_CONTROLLER_SUPPORT
SK_LazyGlobal <
  concurrency::concurrent_unordered_map <ISteamController*, IWrapSteamController*>
>  SK_SteamWrapper_remap_controller;

using SteamAPI_ISteamClient_GetISteamController_pfn = ISteamController* (S_CALLTYPE *)(
  ISteamClient *This,
  HSteamUser    hSteamUser,
  HSteamPipe    hSteamPipe,
  const char   *pchVersion
  );
SteamAPI_ISteamClient_GetISteamController_pfn   SteamAPI_ISteamClient_GetISteamController_Original = nullptr;

ISteamController*
S_CALLTYPE
SteamAPI_ISteamClient_GetISteamController_Detour ( ISteamClient *This,
                                                   HSteamUser    hSteamUser,
                                                   HSteamPipe    hSteamPipe,
                                                   const char   *pchVersion )
{
  steam_log->Log ( L"[!] %ws (%ws)",
                     __FUNCTIONW__, SK_UTF8ToWideChar (pchVersion).c_str () );

  ISteamController* pController =
    SteamAPI_ISteamClient_GetISteamController_Original ( This,
                                                           hSteamUser,
                                                             hSteamPipe,
                                                               pchVersion );

  if (pController != nullptr)
  {
    auto& _SK_SteamWrapper_remap_controller =
           SK_SteamWrapper_remap_controller.get ();

    if (! lstrcmpA (pchVersion, STEAMCONTROLLER_INTERFACE_VERSION))
    {
      if (_SK_SteamWrapper_remap_controller.count (pController) && _SK_SteamWrapper_remap_controller [pController] != nullptr)
         return _SK_SteamWrapper_remap_controller [pController];

      else
      {
        _SK_SteamWrapper_remap_controller [pController] =
                 new IWrapSteamController (pController);

        return
          _SK_SteamWrapper_remap_controller [pController];
      }
    }

    else
    {
      SK_RunOnce (
        steam_log->Log ( L"Game requested unexpected interface version (%s)!",
                           SK_UTF8ToWideChar (pchVersion).c_str () )
      );

      return pController;
    }
  }

  return nullptr;
}

ISteamController*
SK_SteamWrapper_WrappedClient_GetISteamController ( ISteamClient *pRealClient,
                                                    HSteamUser    hSteamUser,
                                                    HSteamPipe    hSteamPipe,
                                                    const char   *pchVersion )
{
  steam_log->Log ( L"[Steam Wrap] [!] GetISteamController (%ws)", SK_UTF8ToWideChar (pchVersion).c_str () );

  if (! lstrcmpA (pchVersion, "SteamController"))
  {
    return pRealClient->GetISteamController (hSteamUser, hSteamPipe, pchVersion);
    //ISteamController0* pController =
    //  reinterpret_cast <ISteamController0 *> ( pRealClient->GetISteamController (hSteamUser, hSteamPipe, pchVersion) );
    //
    //if (pController != nullptr)
    //{
    //  if (SK_SteamWrapper_remap_controller0.count (pController))
    //     return static_cast <ISteamController *> (SK_SteamWrapper_remap_controller0 [pController]);
    //
    //  else
    //  {
    //    SK_SteamWrapper_remap_controller0 [pController] =
    //            new ISteamControllerFake0 (pController);
    //
    //    return static_cast <ISteamController *> (SK_SteamWrapper_remap_controller0 [pController]);
    //  }
    //}
  }

  if (! lstrcmpA (pchVersion, "SteamController005"))
  {
    //if (SK_IsInjected ())
    //{
    //  static volatile LONG __init = FALSE;
    //
    //  if (! InterlockedCompareExchange (&__init, 1, 0))
    //  {
    //    void** vftable = *(void***)*(&pRealClient);
    //
    //    SK_CreateVFTableHook (             L"ISteamClient017_GetISteamController",
    //                             vftable, 25,
    //                             SteamAPI_ISteamClient_GetISteamController_Detour,
    //    static_cast_p2p <void> (&SteamAPI_ISteamClient_GetISteamController_Original) );
    //
    //    SK_EnableHook (vftable [25]);
    //  }
    //}

    //else
    {
      auto& _SK_SteamWrapper_remap_controller =
             SK_SteamWrapper_remap_controller.get ();

      auto *pController =
         pRealClient->GetISteamController (hSteamUser, hSteamPipe, pchVersion);

      if (pController != nullptr)
      {
        if (_SK_SteamWrapper_remap_controller.count (pController) && _SK_SteamWrapper_remap_controller [pController] != nullptr)
          return static_cast <ISteamController *> ( _SK_SteamWrapper_remap_controller [pController] );

        else
        {
          _SK_SteamWrapper_remap_controller [pController] =
                   new IWrapSteamController (pController);

          return
            static_cast <ISteamController *> ( _SK_SteamWrapper_remap_controller [pController] );
        }
      }
    }
  }

  return pRealClient->GetISteamController (hSteamUser, hSteamPipe, pchVersion);
}
#endif