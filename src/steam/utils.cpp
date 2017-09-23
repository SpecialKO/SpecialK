#include <SpecialK/steam_api.h>
#include <SpecialK/config.h>
#include <SpecialK/utility.h>

#include <Windows.h>
#include <cstdint>

#include <unordered_map>

class ISteamUtils;
class IWrapSteamUtils;

std::unordered_map <ISteamUtils*, IWrapSteamUtils*>   SK_SteamWrapper_remap_utils;

class IWrapSteamUtils : public ISteamUtils
{
public:
  IWrapSteamUtils (ISteamUtils* pUtils) :
                    pRealUtils (pUtils) {
  };

  virtual uint32               GetSecondsSinceAppActive       (void)                            override
   { return pRealUtils->GetSecondsSinceAppActive       ();                                              }
  virtual uint32               GetSecondsSinceComputerActive  (void)                            override
   { return pRealUtils->GetSecondsSinceComputerActive  ();                                              }
  virtual EUniverse            GetConnectedUniverse           (void)                            override
   { return pRealUtils->GetConnectedUniverse           ();                                              }
  virtual uint32               GetServerRealTime              (void)                            override
   { steam_log.Log (L"[!] ISteamUtils::GetServerRealTime (...)");
     return pRealUtils->GetServerRealTime              ();                                              }

  virtual const char*          GetIPCountry                   (void)                            override
   { return pRealUtils->GetIPCountry                   ();                                              }

  virtual bool                 GetImageSize                   ( int     iImage,
                                                                uint32 *pnWidth,
                                                                uint32 *pnHeight )              override
   { return pRealUtils->GetImageSize            (iImage, pnWidth, pnHeight);                            }
  virtual bool                 GetImageRGBA                   ( int    iImage,
                                                                uint8 *pubDest,
                                                                int    nDestBufferSize )        override
   { return pRealUtils->GetImageRGBA            (iImage, pubDest, nDestBufferSize);                     }

  virtual bool                 GetCSERIPPort                  ( uint32 *unIP,
                                                                uint16 *usPort )                override
   { return pRealUtils->GetCSERIPPort           (unIP, usPort);                                         }

  virtual uint8                GetCurrentBatteryPower         (void)                            override
   { return pRealUtils->GetCurrentBatteryPower  ();                                                     }

  virtual uint32               GetAppID                       (void)                            override
   { return pRealUtils->GetAppID                ();                                                     }

  virtual void                 SetOverlayNotificationPosition ( ENotificationPosition
                                                                eNotificationPosition )         override
   { return pRealUtils->SetOverlayNotificationPosition (eNotificationPosition);                         }


  virtual bool                 IsAPICallCompleted             ( SteamAPICall_t  hSteamAPICall,
                                                                bool           *pbFailed )      override
   { return pRealUtils->IsAPICallCompleted      (hSteamAPICall, pbFailed);                              }

  virtual ESteamAPICallFailure GetAPICallFailureReason        ( SteamAPICall_t  hSteamAPICall ) override
   { return pRealUtils->GetAPICallFailureReason (hSteamAPICall);                                        }

  virtual bool                 GetAPICallResult               ( SteamAPICall_t  hSteamAPICall,
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


  virtual void                 RunFrame                       (void)                            override
   { return pRealUtils->RunFrame              ();                                                       }


  virtual uint32               GetIPCCallCount                (void)                            override
   { return pRealUtils->GetIPCCallCount       ();                                                       }


  virtual void                 SetWarningMessageHook          ( SteamAPIWarningMessageHook_t
                                                                 pFunction )                    override
   { return pRealUtils->SetWarningMessageHook (pFunction);                                              }


  virtual bool                 IsOverlayEnabled               (void)                            override
   { return pRealUtils->IsOverlayEnabled      ();                                                       }


  virtual bool                 BOverlayNeedsPresent           (void)                            override
   { return pRealUtils->BOverlayNeedsPresent  ();                                                       }


  virtual SteamAPICall_t       CheckFileSignature             ( const char *szFileName )        override
   { return pRealUtils->CheckFileSignature    (szFileName);                                             }


  virtual bool                 ShowGamepadTextInput           ( EGamepadTextInputMode      eInputMode,
                                                                EGamepadTextInputLineMode  eLineInputMode,
                                                                const char                *pchDescription,
                                                                      uint32               unCharMax,
                                                                const char                *pchExistingText ) override
   { return pRealUtils->ShowGamepadTextInput ( eInputMode,
                                                 eLineInputMode,
                                                   pchDescription,
                                                     unCharMax,
                                                       pchExistingText );                                             }

  virtual uint32               GetEnteredGamepadTextLength    (void)                            override
   { return pRealUtils->GetEnteredGamepadTextLength  ();                                                }
  virtual bool                 GetEnteredGamepadTextInput     ( char *pchText, uint32 cchText ) override
   { return pRealUtils->GetEnteredGamepadTextInput   (pchText, cchText);                                }


  virtual const char*          GetSteamUILanguage             (void)                            override
   { return pRealUtils->GetSteamUILanguage           ();                                                }


  virtual bool                 IsSteamRunningInVR             (void)                            override
   { return pRealUtils->IsSteamRunningInVR           ();                                                }


  virtual void                 SetOverlayNotificationInset    ( int nHorizontalInset,
                                                                int nVerticalInset )            override
   { return pRealUtils->SetOverlayNotificationInset  (nHorizontalInset, nVerticalInset);                }


  // 008
  virtual bool                 IsSteamInBigPictureMode        (void)                            override
   { return pRealUtils->IsSteamInBigPictureMode      ();                                                }


  // 009
  virtual void                 StartVRDashboard               (void)                            override
   { return pRealUtils->StartVRDashboard             ();                                                }
  virtual bool                 IsVRHeadsetStreamingEnabled    (void)                            override
   { return pRealUtils->IsVRHeadsetStreamingEnabled  ();                                                }
  virtual void                 SetVRHeadsetStreamingEnabled   (bool bEnabled)                   override
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
      if (SK_SteamWrapper_remap_utils.count (pUtils))
         return SK_SteamWrapper_remap_utils [pUtils];

      else
      {
        SK_SteamWrapper_remap_utils [pUtils] =
                new IWrapSteamUtils (pUtils);

        return SK_SteamWrapper_remap_utils [pUtils];
      }
    }

    else
    {
      SK_RunOnce (
        steam_log.Log ( L"Game requested unexpected interface version (%hs)!",
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
  ISteamUtils* pUtils =
    This->GetISteamUtils ( hSteamPipe,
                             pchVersion );

  if (pUtils != nullptr)
  {
    if ((! lstrcmpA (pchVersion, STEAMUTILS_INTERFACE_VERSION_007)) ||
        (! lstrcmpA (pchVersion, STEAMUTILS_INTERFACE_VERSION_009)) ||
        (! lstrcmpA (pchVersion, STEAMUTILS_INTERFACE_VERSION_008)) ||
        (! lstrcmpA (pchVersion, STEAMUTILS_INTERFACE_VERSION_006)) ||
        (! lstrcmpA (pchVersion, STEAMUTILS_INTERFACE_VERSION_005)))
    {
      if (SK_SteamWrapper_remap_utils.count (pUtils))
         return SK_SteamWrapper_remap_utils [pUtils];

      else
      {
        SK_SteamWrapper_remap_utils [pUtils] =
                new IWrapSteamUtils (pUtils);

        return SK_SteamWrapper_remap_utils [pUtils];
      }
    }

    else
    {
      SK_RunOnce (
        steam_log.Log ( L"Game requested unexpected interface version (%hs)!",
                          pchVersion )
      );

      return pUtils;
    }
  }

  return nullptr;
}