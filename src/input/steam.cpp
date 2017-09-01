#include <SpecialK/steam_api.h>
#include <SpecialK/input/steam.h>
#include <SpecialK/input/input.h>

sk_input_api_context_s SK_Steam_Backend;

//std::unordered_map <ISteamController0*, ISteamControllerFake0*> SK_SteamWrapper_remap_controller0;
std::unordered_map <ISteamController*, ISteamControllerFake*>  SK_SteamWrapper_remap_controller;

ISteamController*
SK_SteamWrapper_FakeClient_GetISteamController (ISteamClient *pRealClient,
                                                HSteamUser    hSteamUser,
                                                HSteamPipe    hSteamPipe,
                                                const char   *pchVersion)
{
  steam_log.Log (L"[SteamWrap] [!] GetISteamController (%hs)", pchVersion);

  //if (! lstrcmpA (pchVersion, "SteamController"))
  //{
  //  ISteamController0* pController =
  //    reinterpret_cast <ISteamController0 *> ( pRealClient->GetISteamController (hSteamUser, hSteamPipe, pchVersion) );
  //
  //  if (pController != nullptr)
  //  {
  //    if (SK_SteamWrapper_remap_controller0.count (pController))
  //       return dynamic_cast <ISteamController *> (SK_SteamWrapper_remap_controller0 [pController]);
  //
  //    else
  //    {
  //      SK_SteamWrapper_remap_controller0 [pController] =
  //              new ISteamControllerFake0 (pController);
  //
  //      return dynamic_cast <ISteamController *> (SK_SteamWrapper_remap_controller0 [pController]);
  //    }
  //  }
  //}

  if (! lstrcmpA (pchVersion, "SteamController005"))
  {
    ISteamController* pController =
      reinterpret_cast <ISteamController *> ( pRealClient->GetISteamController (hSteamUser, hSteamPipe, pchVersion) );

    if (pController != nullptr)
    {
      if (SK_SteamWrapper_remap_controller.count (pController))
        return dynamic_cast <ISteamController *> ( SK_SteamWrapper_remap_controller [pController] );

      else
      {
        SK_SteamWrapper_remap_controller [pController] =
          new ISteamControllerFake (pController);

        return dynamic_cast <ISteamController *> ( SK_SteamWrapper_remap_controller [pController] );
      }
    }
  }

  return pRealClient->GetISteamController (hSteamUser, hSteamPipe, pchVersion);
}