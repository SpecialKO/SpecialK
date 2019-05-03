#include <SpecialK/stdafx.h>
#include <SpecialK/steam_api.h>

class ISteamUtils;
class IWrapSteamUtils;

SK_LazyGlobal <
  concurrency::concurrent_unordered_map <ISteamUtils*, IWrapSteamUtils*>
> SK_SteamWrapper_remap_utils;

class IWrapSteamUtils : public ISteamUtils
{
public:
  explicit
  IWrapSteamUtils (ISteamUtils* pUtils) :
                    pRealUtils (pUtils) {
  };

  uint32               GetSecondsSinceAppActive       (void)                            override
   { return pRealUtils->GetSecondsSinceAppActive       ();                                              }
  uint32               GetSecondsSinceComputerActive  (void)                            override
   { return pRealUtils->GetSecondsSinceComputerActive  ();                                              }
  EUniverse            GetConnectedUniverse           (void)                            override
   { return pRealUtils->GetConnectedUniverse           ();                                              }
  uint32               GetServerRealTime              (void)                            override
   { //steam_log->Log (L"[!] ISteamUtils::GetServerRealTime (...)");
     return pRealUtils->GetServerRealTime              ();                                              }

  const char*          GetIPCountry                   (void)                            override
   { return pRealUtils->GetIPCountry                   ();                                              }

  bool                 GetImageSize                   ( int     iImage,
                                                        uint32 *pnWidth,
                                                        uint32 *pnHeight )              override
   { return pRealUtils->GetImageSize            (iImage, pnWidth, pnHeight);                            }
  bool                 GetImageRGBA                   ( int    iImage,
                                                                uint8 *pubDest,
                                                                int    nDestBufferSize )        override
   { return pRealUtils->GetImageRGBA            (iImage, pubDest, nDestBufferSize);                     }

  bool                 GetCSERIPPort                  ( uint32 *unIP,
                                                        uint16 *usPort )                override
   { return pRealUtils->GetCSERIPPort           (unIP, usPort);                                         }

  uint8                GetCurrentBatteryPower         (void)                            override
   { return pRealUtils->GetCurrentBatteryPower  ();                                                     }

  uint32               GetAppID                       (void)                            override
   { return pRealUtils->GetAppID                ();                                                     }

  void                 SetOverlayNotificationPosition ( ENotificationPosition
                                                        eNotificationPosition )         override
   { return pRealUtils->SetOverlayNotificationPosition (eNotificationPosition);                         }


  bool                 IsAPICallCompleted             ( SteamAPICall_t  hSteamAPICall,
                                                        bool           *pbFailed )      override
   { return pRealUtils->IsAPICallCompleted      (hSteamAPICall, pbFailed);                              }

  ESteamAPICallFailure GetAPICallFailureReason        ( SteamAPICall_t  hSteamAPICall ) override
   { return pRealUtils->GetAPICallFailureReason (hSteamAPICall);                                        }

  bool                 GetAPICallResult               ( SteamAPICall_t  hSteamAPICall,
                                                        void           *pCallback,
                                                        int             cubCallback,
                                                        int             iCallbackExpected,
                                                        bool           *pbFailed )      override
   {
#if 0
     if (iCallbackExpected == k_iSteamUserStatsCallbacks + 1)
     {
       bool bFailed = false;

       if (pRealUtils->GetAPICallFailureReason (hSteamAPICall) != k_ESteamAPICallFailureNone)
       {
         pRealUtils->GetAPICallResult ( hSteamAPICall,
                                          nullptr,
                                            0,
                                              iCallbackExpected,
                                                &bFailed );

         if (bFailed)
         {
           return true;
         }
       }
     }
#endif

     return pRealUtils->GetAPICallResult ( hSteamAPICall,
                                             pCallback,
                                               cubCallback,
                                                 iCallbackExpected,
                                                   pbFailed );                                          }


  void                 RunFrame                       (void)                            override
   { return pRealUtils->RunFrame              ();                                                       }


  uint32               GetIPCCallCount                (void)                            override
   { return pRealUtils->GetIPCCallCount       ();                                                       }


  void                 SetWarningMessageHook          ( SteamAPIWarningMessageHook_t
                                                                 pFunction )                    override
   { return pRealUtils->SetWarningMessageHook (pFunction);                                              }


  bool                 IsOverlayEnabled               (void)                            override
   { return pRealUtils->IsOverlayEnabled      ();                                                       }


  bool                 BOverlayNeedsPresent           (void)                            override
   { return pRealUtils->BOverlayNeedsPresent  ();                                                       }


  SteamAPICall_t       CheckFileSignature             ( const char *szFileName )        override
   { return pRealUtils->CheckFileSignature    (szFileName);                                             }


  bool                 ShowGamepadTextInput           ( EGamepadTextInputMode      eInputMode,
                                                        EGamepadTextInputLineMode  eLineInputMode,
                                                        const char                *pchDescription,
                                                              uint32               unCharMax,
                                                        const char                *pchExistingText ) override
   { return pRealUtils->ShowGamepadTextInput ( eInputMode,
                                                 eLineInputMode,
                                                   pchDescription,
                                                     unCharMax,
                                                       pchExistingText );                                             }

  uint32               GetEnteredGamepadTextLength    (void)                            override
   { return pRealUtils->GetEnteredGamepadTextLength  ();                                                }
  bool                 GetEnteredGamepadTextInput     ( char *pchText, uint32 cchText ) override
   { return pRealUtils->GetEnteredGamepadTextInput   (pchText, cchText);                                }


  const char*          GetSteamUILanguage             (void)                            override
   { return pRealUtils->GetSteamUILanguage           ();                                                }


  bool                 IsSteamRunningInVR             (void)                            override
   { return pRealUtils->IsSteamRunningInVR           ();                                                }


  void                 SetOverlayNotificationInset    ( int nHorizontalInset,
                                                        int nVerticalInset )            override
   { return pRealUtils->SetOverlayNotificationInset  (nHorizontalInset, nVerticalInset);                }


  // 008
  bool                 IsSteamInBigPictureMode        (void)                            override
   { return pRealUtils->IsSteamInBigPictureMode      ();                                                }


  // 009
  void                 StartVRDashboard               (void)                            override
   { return pRealUtils->StartVRDashboard             ();                                                }
  bool                 IsVRHeadsetStreamingEnabled    (void)                            override
   { return pRealUtils->IsVRHeadsetStreamingEnabled  ();                                                }
  void                 SetVRHeadsetStreamingEnabled   (bool bEnabled)                   override
   { return pRealUtils->SetVRHeadsetStreamingEnabled (bEnabled); }


private:
  ISteamUtils* pRealUtils;
};


using SteamAPI_ISteamClient_GetISteamUtils_pfn = ISteamUtils* (S_CALLTYPE *)(
  ISteamClient *This,
  HSteamPipe    hSteamPipe,
  const char   *pchVersion
  );
SteamAPI_ISteamClient_GetISteamUtils_pfn   SteamAPI_ISteamClient_GetISteamUtils_Original = nullptr;

#define STEAMUTILS_INTERFACE_VERSION_005 "SteamUtils005"
#define STEAMUTILS_INTERFACE_VERSION_006 "SteamUtils006"
#define STEAMUTILS_INTERFACE_VERSION_007 "SteamUtils007"
#define STEAMUTILS_INTERFACE_VERSION_008 "SteamUtils008"
#define STEAMUTILS_INTERFACE_VERSION_009 "SteamUtils009"

ISteamUtils*
S_CALLTYPE
SteamAPI_ISteamClient_GetISteamUtils_Detour ( ISteamClient *This,
                                              HSteamPipe    hSteamPipe,
                                              const char   *pchVersion )
{
  SK_RunOnce (
    steam_log->Log ( L"[!] %hs (..., %hs)",
                       __FUNCTION__, pchVersion )
  );

  ISteamUtils* pUtils =
    SteamAPI_ISteamClient_GetISteamUtils_Original ( This,
                                                      hSteamPipe,
                                                        pchVersion );

  if (pUtils != nullptr)
  {
    if ((! lstrcmpA (pchVersion, STEAMUTILS_INTERFACE_VERSION_007)) ||
        (! lstrcmpA (pchVersion, STEAMUTILS_INTERFACE_VERSION_009)) ||
        (! lstrcmpA (pchVersion, STEAMUTILS_INTERFACE_VERSION_008)) ||
        (! lstrcmpA (pchVersion, STEAMUTILS_INTERFACE_VERSION_006)) ||
        (! lstrcmpA (pchVersion, STEAMUTILS_INTERFACE_VERSION_005)))
    {
      auto& _SK_SteamWrapper_remap_utils =
             SK_SteamWrapper_remap_utils.get ();

      if (_SK_SteamWrapper_remap_utils.count (pUtils))
         return _SK_SteamWrapper_remap_utils [pUtils];

      else
      {
        _SK_SteamWrapper_remap_utils [pUtils] =
                 new IWrapSteamUtils (pUtils);

        return
          _SK_SteamWrapper_remap_utils [pUtils];
      }
    }

    else
    {
      SK_RunOnce (
        steam_log->Log ( L"Game requested unexpected interface version (%hs)!",
                           pchVersion )
      );

      return pUtils;
    }
  }

  return nullptr;
}

ISteamUtils*
SK_SteamWrapper_WrappedClient_GetISteamUtils ( ISteamClient *This,
                                               HSteamPipe    hSteamPipe,
                                               const char   *pchVersion )
{
  SK_RunOnce (
    steam_log->Log ( L"[!] %hs (..., %hs)",
                       __FUNCTION__, pchVersion )
  );

  ISteamUtils* pUtils =
    This->GetISteamUtils ( hSteamPipe,
                             pchVersion );

  auto& _SK_SteamWrapper_remap_utils = SK_SteamWrapper_remap_utils.get ();

  if (pUtils != nullptr)
  {
    if ((! lstrcmpA (pchVersion, STEAMUTILS_INTERFACE_VERSION_007)) ||
        (! lstrcmpA (pchVersion, STEAMUTILS_INTERFACE_VERSION_009)) ||
        (! lstrcmpA (pchVersion, STEAMUTILS_INTERFACE_VERSION_008)) ||
        (! lstrcmpA (pchVersion, STEAMUTILS_INTERFACE_VERSION_006)) ||
        (! lstrcmpA (pchVersion, STEAMUTILS_INTERFACE_VERSION_005)))
    {
      if (_SK_SteamWrapper_remap_utils.count (pUtils))
         return _SK_SteamWrapper_remap_utils [pUtils];

      else
      {
        _SK_SteamWrapper_remap_utils [pUtils] =
                 new IWrapSteamUtils (pUtils);

        return _SK_SteamWrapper_remap_utils [pUtils];
      }
    }

    else
    {
      SK_RunOnce (
        steam_log->Log ( L"Game requested unexpected interface version (%hs)!",
                           pchVersion )
      );

      return pUtils;
    }
  }

  return nullptr;
}

using SteamUtils_pfn = ISteamUtils* (S_CALLTYPE *)(
        void
      );
SteamUtils_pfn SteamUtils_Original = nullptr;

ISteamUtils*
S_CALLTYPE
SteamUtils_Detour (void)
{
  SK_RunOnce (
    steam_log->Log ( L"[!] %hs ()",
                       __FUNCTION__ )
  );

#ifndef DANGEROUS_INTERFACE_ALIASING
  return SteamUtils_Original ();
#else

  auto& _SK_SteamWrapper_remap_utils =
         SK_SteamWrapper_remap_utils.get ();

  if (steam_ctx.UtilsVersion () >= 5 && steam_ctx.UtilsVersion () <= 9)
  {
    ISteamUtils* pUtils =
      static_cast <ISteamUtils *> ( SteamUtils_Original () );

    if (pUtils != nullptr)
    {
      if (_SK_SteamWrapper_remap_utils.count (pUtils))
         return _SK_SteamWrapper_remap_utils [pUtils];

      else
      {
        _SK_SteamWrapper_remap_utils [pUtils] =
                 new IWrapSteamUtils (pUtils);

        return _SK_SteamWrapper_remap_utils [pUtils];
      }
    }

    return nullptr;
  }

  return SteamUtils_Original ();
#endif
}