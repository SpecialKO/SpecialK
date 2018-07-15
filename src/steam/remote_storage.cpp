#include <SpecialK/steam_api.h>
#include <SpecialK/config.h>
#include <SpecialK/utility.h>

#include <Windows.h>
#include <cstdint>

#include <unordered_map>

class ISteamRemoteStorage;
class IWrapSteamRemoteStorage;

#include <concurrent_unordered_map.h>
concurrency::concurrent_unordered_map <ISteamRemoteStorage*, IWrapSteamRemoteStorage*>   SK_SteamWrapper_remap_remotestorage;

class IWrapSteamRemoteStorage012 : public ISteamRemoteStorage
{
public:
  IWrapSteamRemoteStorage012 (ISteamRemoteStorage* pRemoteStorage) :
                                     pRealStorage (pRemoteStorage) {
  };

  // NOTE
  //
  // Filenames are case-insensitive, and will be converted to lowercase automatically.
  // So "foo.bar" and "Foo.bar" are the same file, and if you write "Foo.bar" then
  // iterate the files, the filename returned will be "foo.bar".
  //

  // file operations
  bool                        FileWrite                 ( const char                   *pchFile,
                                                          const void                   *pvData,
                                                                int32                   cubData                ) override
   { return pRealStorage->FileWrite (pchFile, pvData, cubData);                                                                  }
  int32                       FileRead                  ( const char                   *pchFile,
                                                                void                   *pvData,
                                                                int32                   cubDataToRead          ) override
   { return pRealStorage->FileRead (pchFile, pvData, cubDataToRead);                                                             }
  bool                        FileForget                ( const char                   *pchFile                ) override
   { return pRealStorage->FileForget (pchFile);                                                                                  }
  bool                        FileDelete                ( const char                   *pchFile                ) override
   { return pRealStorage->FileDelete (pchFile);                                                                                  }
  SteamAPICall_t              FileShare                 ( const char                   *pchFile                ) override
   { return pRealStorage->FileShare  (pchFile);                                                                                  }
  bool                        SetSyncPlatforms          ( const char                   *pchFile,
                                                                ERemoteStoragePlatform  eRemoteStoragePlatform ) override
   { return pRealStorage->SetSyncPlatforms (pchFile, eRemoteStoragePlatform);                                                    }

  // file operations that cause network IO
  UGCFileWriteStreamHandle_t  FileWriteStreamOpen       ( const char                       *pchFile      ) override
   { return pRealStorage->FileWriteStreamOpen (pchFile);                                                                   }
  bool                        FileWriteStreamWriteChunk (       UGCFileWriteStreamHandle_t  writeHandle,
                                                          const void                       *pvData,
                                                                int32                       cubData      ) override
   { return pRealStorage->FileWriteStreamWriteChunk (writeHandle, pvData, cubData);                                        }
  bool                        FileWriteStreamClose      (       UGCFileWriteStreamHandle_t  writeHandle  ) override
   { return pRealStorage->FileWriteStreamClose (writeHandle);                                                              }
  bool                        FileWriteStreamCancel     (       UGCFileWriteStreamHandle_t  writeHandle  ) override
   { return pRealStorage->FileWriteStreamCancel (writeHandle);                                                             }

  // file information
  bool                        FileExists                ( const char *pchFile ) override
   { return pRealStorage->FileExists (pchFile);                                                 }
  bool                        FilePersisted             ( const char *pchFile ) override
   { return pRealStorage->FilePersisted (pchFile);                                              }
  int32                       GetFileSize               ( const char *pchFile ) override
   { return pRealStorage->GetFileSize (pchFile);                                                }
  int64                       GetFileTimestamp          ( const char *pchFile ) override
   { return pRealStorage->GetFileTimestamp (pchFile);                                           }
  ERemoteStoragePlatform      GetSyncPlatforms          ( const char *pchFile ) override
   { return pRealStorage->GetSyncPlatforms (pchFile);                                           }

  // iteration
  int32                       GetFileCount              ( void                     ) override
   { return pRealStorage->GetFileCount ();                                                           }
  const char*                 GetFileNameAndSize        ( int    iFile,
                                                                  int32 *pnFileSizeInBytes ) override
   {
     const char* pszRet =
       pRealStorage->GetFileNameAndSize (iFile, pnFileSizeInBytes);

     steam_log.Log ( L"ISteamRemoteStorage ** File: (%hs:%lu) - %lu Bytes",
                       pszRet, iFile, pnFileSizeInBytes != nullptr ? *pnFileSizeInBytes : 0 );

     return pszRet;
   }

  // configuration management
  bool                        GetQuota                  ( int32 *pnTotalBytes,
                                                          int32 *puAvailableBytes  ) override
   { return pRealStorage->GetQuota (pnTotalBytes, puAvailableBytes);                                 }
  bool                        IsCloudEnabledForAccount  ( void                     ) override
   { return pRealStorage->IsCloudEnabledForAccount ();                                               }
  bool                        IsCloudEnabledForApp      ( void                     ) override
   { return pRealStorage->IsCloudEnabledForApp ();                                                   }
  void                        SetCloudEnabledForApp     ( bool   bEnabled          ) override
   { return pRealStorage->SetCloudEnabledForApp (bEnabled);                                          }

  // user generated content

  // Downloads a UGC file.  A priority value of 0 will download the file immediately,
  // otherwise it will wait to download the file until all downloads with a lower priority
  // value are completed.  Downloads with equal priority will occur simultaneously.
  SteamAPICall_t              UGCDownload               ( UGCHandle_t hContent,
                                                          uint32      unPriority       ) override
   {
     steam_log.Log (L"ISteamRemoteStorage::UGCDownload (...)");

     return pRealStorage->UGCDownload (hContent, unPriority);                                            }

  // Gets the amount of data downloaded so far for a piece of content. pnBytesExpected can be 0 if function returns false
  // or if the transfer hasn't started yet, so be careful to check for that before dividing to get a percentage
  bool                        GetUGCDownloadProgress    ( UGCHandle_t  hContent,
                                                          int32       *pnBytesDownloaded,
                                                          int32       *pnBytesExpected ) override
   { return pRealStorage->GetUGCDownloadProgress (hContent, pnBytesDownloaded, pnBytesExpected);         }

  // Gets metadata for a file after it has been downloaded. This is the same metadata given in the RemoteStorageDownloadUGCResult_t call result
  bool                        GetUGCDetails             ( UGCHandle_t   hContent,
                                                          AppId_t      *pnAppID,
                                                          char        **ppchName,
                                                          int32        *pnFileSizeInBytes,
                                             OUT_STRUCT() CSteamID     *pSteamIDOwner  ) override
   { return pRealStorage->GetUGCDetails (hContent, pnAppID, ppchName, pnFileSizeInBytes, pSteamIDOwner); }

  // After download, gets the content of the file.  
  // Small files can be read all at once by calling this function with an offset of 0 and cubDataToRead equal to the size of the file.
  // Larger files can be read in chunks to reduce memory usage (since both sides of the IPC client and the game itself must allocate
  // enough memory for each chunk).  Once the last byte is read, the file is implicitly closed and further calls to UGCRead will fail
  // unless UGCDownload is called again.
  // For especially large files (anything over 100MB) it is a requirement that the file is read in chunks.
  int32	                      UGCRead                   ( UGCHandle_t     hContent,
                                                          void           *pvData,
                                                          int32           cubDataToRead,
                                                          uint32          cOffset,
                                                          EUGCReadAction  eAction ) override
   { return pRealStorage->UGCRead (hContent, pvData, cubDataToRead, cOffset, eAction);              }

  // Functions to iterate through UGC that has finished downloading but has not yet been read via UGCRead()
  int32	                      GetCachedUGCCount         ( void                    ) override
   {
     steam_log.Log (L"ISteamRemoteStorage::GetCachedUGCCount (...)");

     return pRealStorage->GetCachedUGCCount ();                                                     }
  UGCHandle_t                 GetCachedUGCHandle        ( int32 iCachedContent    ) override
   { return pRealStorage->GetCachedUGCHandle (iCachedContent);                                      }

  // publishing UGC
  SteamAPICall_t              PublishWorkshopFile              ( const char                                  *pchFile,
                                                                 const char                                  *pchPreviewFile,
                                                                       AppId_t                                nConsumerAppId,
                                                                 const char                                  *pchTitle,
                                                                 const char                                  *pchDescription,
                                                                       ERemoteStoragePublishedFileVisibility  eVisibility,
                                                                       SteamParamStringArray_t               *pTags,
                                                                       EWorkshopFileType                      eWorkshopFileType ) override
   { return pRealStorage->PublishWorkshopFile ( pchFile, pchPreviewFile, nConsumerAppId,
                                                  pchTitle, pchDescription, eVisibility, pTags, eWorkshopFileType );                              }

  PublishedFileUpdateHandle_t CreatePublishedFileUpdateRequest (       PublishedFileId_t                      unPublishedFileId ) override
   { return pRealStorage->CreatePublishedFileUpdateRequest (unPublishedFileId);                                                                   }
  bool                        UpdatePublishedFileFile          (       PublishedFileUpdateHandle_t            updateHandle,
                                                                 const char                                  *pchFile           ) override
   { return pRealStorage->UpdatePublishedFileFile        (updateHandle, pchFile);                                                                 }
   bool                        UpdatePublishedFilePreviewFile   (       PublishedFileUpdateHandle_t            updateHandle,
                                                                  const char                                  *pchPreviewFile    ) override
   { return pRealStorage->UpdatePublishedFilePreviewFile (updateHandle, pchPreviewFile);                                                          }
  bool                        UpdatePublishedFileTitle         (       PublishedFileUpdateHandle_t            updateHandle,
                                                                 const char                                  *pchTitle          ) override
   { return pRealStorage->UpdatePublishedFileTitle       (updateHandle, pchTitle);                                                                }
  bool                        UpdatePublishedFileDescription   (       PublishedFileUpdateHandle_t            updateHandle,
                                                                 const char                                  *pchDescription    ) override
   { return pRealStorage->UpdatePublishedFileDescription (updateHandle, pchDescription);                                                          }
  bool                        UpdatePublishedFileVisibility    (       PublishedFileUpdateHandle_t            updateHandle,
                                                                       ERemoteStoragePublishedFileVisibility  eVisibility       ) override
   { return pRealStorage->UpdatePublishedFileVisibility  (updateHandle, eVisibility);                                                             }
  bool                        UpdatePublishedFileTags          (       PublishedFileUpdateHandle_t            updateHandle,
                                                                       SteamParamStringArray_t               *pTags             ) override
   { return pRealStorage->UpdatePublishedFileTags        (updateHandle, pTags);                                                                   }
  SteamAPICall_t              CommitPublishedFileUpdate        (       PublishedFileUpdateHandle_t            updateHandle      ) override
   { return pRealStorage->CommitPublishedFileUpdate      (updateHandle);                                                                          }

  // Gets published file details for the given publishedfileid.  If unMaxSecondsOld is greater than 0,
  // cached data may be returned, depending on how long ago it was cached.  A value of 0 will force a refresh.
  // A value of k_WorkshopForceLoadPublishedFileDetailsFromCache will use cached data if it exists, no matter how old it is.
  SteamAPICall_t              GetPublishedFileDetails                 (       PublishedFileId_t                      unPublishedFileId,
                                                                              uint32                                 unMaxSecondsOld      ) override
   { return pRealStorage->GetPublishedFileDetails      (unPublishedFileId, unMaxSecondsOld);                                                                }
  SteamAPICall_t              DeletePublishedFile                     (       PublishedFileId_t                      unPublishedFileId    ) override
   { return pRealStorage->DeletePublishedFile          (unPublishedFileId);                                                                                 }
  // enumerate the files that the current user published with this app
  SteamAPICall_t              EnumerateUserPublishedFiles             (       uint32                                 unStartIndex         ) override
   {
    steam_log.Log (L"ISteamRemoteStorage::EnumeratePublishedFiles (...)");

     return pRealStorage->EnumerateUserPublishedFiles  (unStartIndex);                                                                                      }
  SteamAPICall_t              SubscribePublishedFile                  (       PublishedFileId_t                      unPublishedFileId    ) override
   { return pRealStorage->SubscribePublishedFile       (unPublishedFileId);                                                                                 }
  SteamAPICall_t              EnumerateUserSubscribedFiles            (       uint32                                 unStartIndex         ) override
   {
     steam_log.Log (L"ISteamRemoteStorage::EnumerateSubscribedFiles (...)");

     return pRealStorage->EnumerateUserSubscribedFiles (unStartIndex);                                                                                      }
  SteamAPICall_t              UnsubscribePublishedFile                (       PublishedFileId_t                      unPublishedFileId    ) override
   { return pRealStorage->UnsubscribePublishedFile     (unPublishedFileId);                                                                                 }
  bool                        UpdatePublishedFileSetChangeDescription (       PublishedFileUpdateHandle_t            updateHandle,
                                                                        const char                                  *pchChangeDescription ) override
   { return pRealStorage->UpdatePublishedFileSetChangeDescription (updateHandle, pchChangeDescription);                                                     }
  SteamAPICall_t              GetPublishedItemVoteDetails             (       PublishedFileId_t                      unPublishedFileId    ) override
   { return pRealStorage->GetPublishedItemVoteDetails (unPublishedFileId);                                                                                  }
  SteamAPICall_t              UpdateUserPublishedItemVote             (       PublishedFileId_t                      unPublishedFileId,
                                                                              bool                                   bVoteUp              ) override
   { return pRealStorage->UpdateUserPublishedItemVote (unPublishedFileId, bVoteUp);                                                                         }
  SteamAPICall_t              GetUserPublishedItemVoteDetails         (       PublishedFileId_t                      unPublishedFileId    ) override
   { return pRealStorage->GetUserPublishedItemVoteDetails (unPublishedFileId);                                                                              }
  SteamAPICall_t              EnumerateUserSharedWorkshopFiles        (       CSteamID                               steamId,
                                                                              uint32                                 unStartIndex,
                                                                              SteamParamStringArray_t               *pRequiredTags,
                                                                              SteamParamStringArray_t               *pExcludedTags        ) override
   { return pRealStorage->EnumerateUserSharedWorkshopFiles (steamId, unStartIndex, pRequiredTags, pExcludedTags);                                           }
  SteamAPICall_t              PublishVideo                            (       EWorkshopVideoProvider                 eVideoProvider,
                                                                        const char                                  *pchVideoAccount,
                                                                        const char                                  *pchVideoIdentifier,
                                                                        const char                                  *pchPreviewFile,
                                                                              AppId_t                                nConsumerAppId,
                                                                        const char                                  *pchTitle,
                                                                        const char                                  *pchDescription,
                                                                              ERemoteStoragePublishedFileVisibility  eVisibility,
                                                                              SteamParamStringArray_t               *pTags                ) override
   { return pRealStorage->PublishVideo ( eVideoProvider, pchVideoAccount, pchVideoIdentifier,
                                           pchPreviewFile, nConsumerAppId, pchTitle, pchDescription,
                                             eVisibility, pTags );                                                                                          }
  SteamAPICall_t              SetUserPublishedFileAction              (       PublishedFileId_t                      unPublishedFileId,
                                                                              EWorkshopFileAction                    eAction              ) override
   { return pRealStorage->SetUserPublishedFileAction (unPublishedFileId, eAction);                                                                          }
  SteamAPICall_t              EnumeratePublishedFilesByUserAction     (       EWorkshopFileAction                    eAction,
                                                                              uint32                                 unStartIndex         ) override
   {
     steam_log.Log (L"ISteamRemoteStorage::EnumeratePublishedFilesByUserAction (...)");

     return pRealStorage->EnumeratePublishedFilesByUserAction (eAction, unStartIndex);                                                                      }

  // this method enumerates the public view of workshop files
  SteamAPICall_t              EnumeratePublishedWorkshopFiles         (       EWorkshopEnumerationType               eEnumerationType,
                                                                              uint32                                 unStartIndex,
                                                                              uint32                                 unCount,
                                                                              uint32                                 unDays,
                                                                              SteamParamStringArray_t               *pTags,
                                                                              SteamParamStringArray_t               *pUserTags            ) override
   { 
     steam_log.Log (L"ISteamRemoteStorage::EnumeratePublishedWorkshopFiles (...)");

     return pRealStorage->EnumeratePublishedWorkshopFiles (eEnumerationType, unStartIndex, unCount, unDays, pTags, pUserTags);                               }

  SteamAPICall_t              UGCDownloadToLocation                   (       UGCHandle_t                            hContent,
                                                                        const char                                  *pchLocation,
                                                                              uint32                                 unPriority           ) override
   { return pRealStorage->UGCDownloadToLocation (hContent, pchLocation, unPriority);                                                                        }

private:
  ISteamRemoteStorage* pRealStorage;
};

class ISteamRemoteStorage014
{
	public:
		// NOTE
		//
		// Filenames are case-insensitive, and will be converted to lowercase automatically.
		// So "foo.bar" and "Foo.bar" are the same file, and if you write "Foo.bar" then
		// iterate the files, the filename returned will be "foo.bar".
		//

		// file operations
		virtual bool	FileWrite( const char *pchFile, const void *pvData, int32 cubData ) = 0;
		virtual int32	FileRead( const char *pchFile, void *pvData, int32 cubDataToRead ) = 0;
		
		virtual SteamAPICall_t FileWriteAsync( const char *pchFile, const void *pvData, uint32 cubData ) = 0;
		
		virtual SteamAPICall_t FileReadAsync( const char *pchFile, uint32 nOffset, uint32 cubToRead ) = 0;
		virtual bool	FileReadAsyncComplete( SteamAPICall_t hReadCall, void *pvBuffer, uint32 cubToRead ) = 0;
		
		virtual bool	FileForget( const char *pchFile ) = 0;
		virtual bool	FileDelete( const char *pchFile ) = 0;

		virtual SteamAPICall_t FileShare( const char *pchFile ) = 0;
		virtual bool	SetSyncPlatforms( const char *pchFile, ERemoteStoragePlatform eRemoteStoragePlatform ) = 0;

		// file operations that cause network IO
		virtual UGCFileWriteStreamHandle_t FileWriteStreamOpen( const char *pchFile ) = 0;
		virtual bool FileWriteStreamWriteChunk( UGCFileWriteStreamHandle_t writeHandle, const void *pvData, int32 cubData ) = 0;
		virtual bool FileWriteStreamClose( UGCFileWriteStreamHandle_t writeHandle ) = 0;
		virtual bool FileWriteStreamCancel( UGCFileWriteStreamHandle_t writeHandle ) = 0;

		// file information
		virtual bool	FileExists( const char *pchFile ) = 0;
		virtual bool	FilePersisted( const char *pchFile ) = 0;
		virtual int32	GetFileSize( const char *pchFile ) = 0;
		virtual int64	GetFileTimestamp( const char *pchFile ) = 0;
		virtual ERemoteStoragePlatform GetSyncPlatforms( const char *pchFile ) = 0;

		// iteration
		virtual int32 GetFileCount() = 0;
		virtual const char *GetFileNameAndSize( int iFile, int32 *pnFileSizeInBytes ) = 0;

		// configuration management
		virtual bool GetQuota( uint64 *pnTotalBytes, uint64 *puAvailableBytes ) = 0;
		virtual bool IsCloudEnabledForAccount() = 0;
		virtual bool IsCloudEnabledForApp() = 0;
		virtual void SetCloudEnabledForApp( bool bEnabled ) = 0;

		// user generated content

		// Downloads a UGC file.  A priority value of 0 will download the file immediately,
		// otherwise it will wait to download the file until all downloads with a lower priority
		// value are completed.  Downloads with equal priority will occur simultaneously.

		virtual SteamAPICall_t UGCDownload( UGCHandle_t hContent, uint32 unPriority ) = 0;
		
		// Gets the amount of data downloaded so far for a piece of content. pnBytesExpected can be 0 if function returns false
		// or if the transfer hasn't started yet, so be careful to check for that before dividing to get a percentage
		virtual bool	GetUGCDownloadProgress( UGCHandle_t hContent, int32 *pnBytesDownloaded, int32 *pnBytesExpected ) = 0;

		// Gets metadata for a file after it has been downloaded. This is the same metadata given in the RemoteStorageDownloadUGCResult_t call result
		virtual bool	GetUGCDetails( UGCHandle_t hContent, AppId_t *pnAppID, char **ppchName, int32 *pnFileSizeInBytes, OUT_STRUCT() CSteamID *pSteamIDOwner ) = 0;

		// After download, gets the content of the file.  
		// Small files can be read all at once by calling this function with an offset of 0 and cubDataToRead equal to the size of the file.
		// Larger files can be read in chunks to reduce memory usage (since both sides of the IPC client and the game itself must allocate
		// enough memory for each chunk).  Once the last byte is read, the file is implicitly closed and further calls to UGCRead will fail
		// unless UGCDownload is called again.
		// For especially large files (anything over 100MB) it is a requirement that the file is read in chunks.
		virtual int32	UGCRead( UGCHandle_t hContent, void *pvData, int32 cubDataToRead, uint32 cOffset, EUGCReadAction eAction ) = 0;

		// Functions to iterate through UGC that has finished downloading but has not yet been read via UGCRead()
		virtual int32	GetCachedUGCCount() = 0;
		virtual	UGCHandle_t GetCachedUGCHandle( int32 iCachedContent ) = 0;

		// The following functions are only necessary on the Playstation 3. On PC & Mac, the Steam client will handle these operations for you
		// On Playstation 3, the game controls which files are stored in the cloud, via FilePersist, FileFetch, and FileForget.
			
#if defined(_PS3) || defined(_SERVER)
		// Connect to Steam and get a list of files in the Cloud - results in a RemoteStorageAppSyncStatusCheck_t callback
		virtual void GetFileListFromServer() = 0;
		// Indicate this file should be downloaded in the next sync
		virtual bool FileFetch( const char *pchFile ) = 0;
		// Indicate this file should be persisted in the next sync
		virtual bool FilePersist( const char *pchFile ) = 0;
		// Pull any requested files down from the Cloud - results in a RemoteStorageAppSyncedClient_t callback
		virtual bool SynchronizeToClient() = 0;
		// Upload any requested files to the Cloud - results in a RemoteStorageAppSyncedServer_t callback
		virtual bool SynchronizeToServer() = 0;
		// Reset any fetch/persist/etc requests
		virtual bool ResetFileRequestState() = 0;
#endif

		// publishing UGC
		virtual SteamAPICall_t	PublishWorkshopFile( const char *pchFile, const char *pchPreviewFile, AppId_t nConsumerAppId, const char *pchTitle, const char *pchDescription, ERemoteStoragePublishedFileVisibility eVisibility, SteamParamStringArray_t *pTags, EWorkshopFileType eWorkshopFileType ) = 0;
		virtual PublishedFileUpdateHandle_t CreatePublishedFileUpdateRequest( PublishedFileId_t unPublishedFileId ) = 0;
		virtual bool UpdatePublishedFileFile( PublishedFileUpdateHandle_t updateHandle, const char *pchFile ) = 0;
		virtual bool UpdatePublishedFilePreviewFile( PublishedFileUpdateHandle_t updateHandle, const char *pchPreviewFile ) = 0;
		virtual bool UpdatePublishedFileTitle( PublishedFileUpdateHandle_t updateHandle, const char *pchTitle ) = 0;
		virtual bool UpdatePublishedFileDescription( PublishedFileUpdateHandle_t updateHandle, const char *pchDescription ) = 0;
		virtual bool UpdatePublishedFileVisibility( PublishedFileUpdateHandle_t updateHandle, ERemoteStoragePublishedFileVisibility eVisibility ) = 0;
		virtual bool UpdatePublishedFileTags( PublishedFileUpdateHandle_t updateHandle, SteamParamStringArray_t *pTags ) = 0;

		virtual SteamAPICall_t	CommitPublishedFileUpdate( PublishedFileUpdateHandle_t updateHandle ) = 0;
		// Gets published file details for the given publishedfileid.  If unMaxSecondsOld is greater than 0,
		// cached data may be returned, depending on how long ago it was cached.  A value of 0 will force a refresh.
		// A value of k_WorkshopForceLoadPublishedFileDetailsFromCache will use cached data if it exists, no matter how old it is.

		virtual SteamAPICall_t	GetPublishedFileDetails( PublishedFileId_t unPublishedFileId, uint32 unMaxSecondsOld ) = 0;

		virtual SteamAPICall_t	DeletePublishedFile( PublishedFileId_t unPublishedFileId ) = 0;
		// enumerate the files that the current user published with this app

		virtual SteamAPICall_t	EnumerateUserPublishedFiles( uint32 unStartIndex ) = 0;

		virtual SteamAPICall_t	SubscribePublishedFile( PublishedFileId_t unPublishedFileId ) = 0;

		virtual SteamAPICall_t	EnumerateUserSubscribedFiles( uint32 unStartIndex ) = 0;

		virtual SteamAPICall_t	UnsubscribePublishedFile( PublishedFileId_t unPublishedFileId ) = 0;
		virtual bool UpdatePublishedFileSetChangeDescription( PublishedFileUpdateHandle_t updateHandle, const char *pchChangeDescription ) = 0;

		virtual SteamAPICall_t	GetPublishedItemVoteDetails( PublishedFileId_t unPublishedFileId ) = 0;

		virtual SteamAPICall_t	UpdateUserPublishedItemVote( PublishedFileId_t unPublishedFileId, bool bVoteUp ) = 0;

		virtual SteamAPICall_t	GetUserPublishedItemVoteDetails( PublishedFileId_t unPublishedFileId ) = 0;

		virtual SteamAPICall_t	EnumerateUserSharedWorkshopFiles( CSteamID steamId, uint32 unStartIndex, SteamParamStringArray_t *pRequiredTags, SteamParamStringArray_t *pExcludedTags ) = 0;

		virtual SteamAPICall_t	PublishVideo( EWorkshopVideoProvider eVideoProvider, const char *pchVideoAccount, const char *pchVideoIdentifier, const char *pchPreviewFile, AppId_t nConsumerAppId, const char *pchTitle, const char *pchDescription, ERemoteStoragePublishedFileVisibility eVisibility, SteamParamStringArray_t *pTags ) = 0;

		virtual SteamAPICall_t	SetUserPublishedFileAction( PublishedFileId_t unPublishedFileId, EWorkshopFileAction eAction ) = 0;

		virtual SteamAPICall_t	EnumeratePublishedFilesByUserAction( EWorkshopFileAction eAction, uint32 unStartIndex ) = 0;
		// this method enumerates the public view of workshop files

		virtual SteamAPICall_t	EnumeratePublishedWorkshopFiles( EWorkshopEnumerationType eEnumerationType, uint32 unStartIndex, uint32 unCount, uint32 unDays, SteamParamStringArray_t *pTags, SteamParamStringArray_t *pUserTags ) = 0;


		virtual SteamAPICall_t UGCDownloadToLocation( UGCHandle_t hContent, const char *pchLocation, uint32 unPriority ) = 0;
};

#define SK_STEAM_INVALID_UGC_FILE MAXUINT64-1

bool
SK_Steam_IsCloudFileBlacklisted (const char *pchFile)
{
  return config.steam.cloud.blacklist.count (pchFile) != 0;
}

class IWrapSteamRemoteStorage014 : public ISteamRemoteStorage014
{
public:
  IWrapSteamRemoteStorage014 (ISteamRemoteStorage014* pRemoteStorage) :
                                        pRealStorage (pRemoteStorage) {
  };

  // NOTE
  //
  // Filenames are case-insensitive, and will be converted to lowercase automatically.
  // So "foo.bar" and "Foo.bar" are the same file, and if you write "Foo.bar" then
  // iterate the files, the filename returned will be "foo.bar".
  //

  // file operations
  bool                        FileWrite                 ( const char                   *pchFile,
                                                          const void                   *pvData,
                                                                int32                   cubData                ) override
   {
     //if (SK_SteamUser_BLoggedOn () == SK_SteamUser_LoggedOn_e::Online)
     //{
      if (SK_Steam_IsCloudFileBlacklisted (pchFile))
        return true;

      return pRealStorage->FileWrite (pchFile, pvData, cubData);
     //}

     //return false;
   }
  int32                       FileRead                  ( const char                   *pchFile,
                                                                void                   *pvData,
                                                                int32                   cubDataToRead          ) override
   { return pRealStorage->FileRead (pchFile, pvData, cubDataToRead);                                                             }

  SteamAPICall_t              FileWriteAsync            ( const char                   *pchFile,
                                                          const void                   *pvData,
                                                                uint32                  cubData                ) override
   { return pRealStorage->FileWriteAsync (pchFile, pvData, cubData);                                                             }
  SteamAPICall_t              FileReadAsync             ( const char                   *pchFile,
                                                                uint32                  nOffset,
                                                                uint32                  cubToRead              ) override
   { return pRealStorage->FileReadAsync (pchFile, nOffset, cubToRead);                                                           }
  bool                        FileReadAsyncComplete     (       SteamAPICall_t          hReadCall,
                                                                void                   *pvBuffer,
                                                                uint32                  cubToRead              ) override
   { return pRealStorage->FileReadAsyncComplete (hReadCall, pvBuffer, cubToRead);                                                }

  bool                        FileForget                ( const char                   *pchFile                ) override
   {
     if (SK_Steam_IsCloudFileBlacklisted (pchFile))
       return true;

     //if (SK_SteamUser_BLoggedOn () == SK_SteamUser_LoggedOn_e::Online)
       return pRealStorage->FileForget (pchFile);

     //return true;
   }
  bool                        FileDelete                ( const char                   *pchFile                ) override
   {
     if (SK_Steam_IsCloudFileBlacklisted (pchFile))
        return true;

     //if (SK_SteamUser_BLoggedOn () == SK_SteamUser_LoggedOn_e::Online)
       return pRealStorage->FileDelete (pchFile);

     //return true;
   }
  SteamAPICall_t              FileShare                 ( const char                   *pchFile                ) override
   {
     if (SK_Steam_IsCloudFileBlacklisted (pchFile))
        return 0;
   
     return pRealStorage->FileShare (pchFile);
   }
  bool                        SetSyncPlatforms          ( const char                   *pchFile,
                                                                ERemoteStoragePlatform  eRemoteStoragePlatform ) override
   {
     if (SK_Steam_IsCloudFileBlacklisted (pchFile))
       return true;

     return pRealStorage->SetSyncPlatforms (pchFile, eRemoteStoragePlatform);
   }

  // file operations that cause network IO
  UGCFileWriteStreamHandle_t  FileWriteStreamOpen       ( const char                       *pchFile      ) override
   {
      if (SK_Steam_IsCloudFileBlacklisted (pchFile))
        return SK_STEAM_INVALID_UGC_FILE;
   
     return pRealStorage->FileWriteStreamOpen (pchFile);
   }
  bool                        FileWriteStreamWriteChunk (       UGCFileWriteStreamHandle_t  writeHandle,
                                                          const void                       *pvData,
                                                                int32                       cubData      ) override
   {
     if (writeHandle == SK_STEAM_INVALID_UGC_FILE)
       return true;
   
     return pRealStorage->FileWriteStreamWriteChunk (writeHandle, pvData, cubData);
   }
  bool                        FileWriteStreamClose      (       UGCFileWriteStreamHandle_t  writeHandle  ) override
   {
     if (writeHandle == SK_STEAM_INVALID_UGC_FILE)
       return true;

     return pRealStorage->FileWriteStreamClose (writeHandle);
   }
  bool                        FileWriteStreamCancel     (       UGCFileWriteStreamHandle_t  writeHandle  ) override
   {
     if (writeHandle == SK_STEAM_INVALID_UGC_FILE)
       return true;

     return pRealStorage->FileWriteStreamCancel (writeHandle);
   }

  // file information
  bool                        FileExists                ( const char *pchFile ) override
   {
   //steam_log.Log (L"ISteamRemoteStorage::FileExists (%hs)", pchFile);

     //if (SK_SteamUser_BLoggedOn () == SK_SteamUser_LoggedOn_e::Online)
     //{
      // if (SK_Steam_IsCloudFileBlacklisted (pchFile))
      //   return false;

       return pRealStorage->FileExists (pchFile);
     //}
     //
     //return false;
   }
  bool                        FilePersisted             ( const char *pchFile ) override
   {
     //if (SK_Steam_IsCloudFileBlacklisted (pchFile))
     //  return false;

     return pRealStorage->FilePersisted (pchFile);
   }
  int32                       GetFileSize               ( const char *pchFile ) override
   { return pRealStorage->GetFileSize (pchFile);                                                }
  int64                       GetFileTimestamp          ( const char *pchFile ) override
   { return pRealStorage->GetFileTimestamp (pchFile);                                           }
  ERemoteStoragePlatform      GetSyncPlatforms          ( const char *pchFile ) override
   { return pRealStorage->GetSyncPlatforms (pchFile);                                           }

  // iteration
  int32                       GetFileCount              ( void                     ) override
   {
   //steam_log.Log (L"ISteamRemoteStorage::GetFileCount ()");

     //if (SK_SteamUser_BLoggedOn () == SK_SteamUser_LoggedOn_e::Online)
      return pRealStorage->GetFileCount ();

     //return 0;
  }
   const char*                 GetFileNameAndSize        ( int    iFile,
                                                           int32 *pnFileSizeInBytes ) override
   {
     //if (SK_SteamUser_BLoggedOn () != SK_SteamUser_LoggedOn_e::Online)
       //return "";

     const char* pszRet =
       pRealStorage->GetFileNameAndSize (iFile, pnFileSizeInBytes);

     steam_log.Log ( L"ISteamRemoteStorage014 ** File: (%hs:%lu) - %lu Bytes",
                       pszRet, iFile, pnFileSizeInBytes != nullptr ? *pnFileSizeInBytes : 0 );

     if (SK_Steam_IsCloudFileBlacklisted (pszRet))
       return "BlackListed.IAm"; // Sam

     return pszRet;
   }

  // configuration management
  bool                        GetQuota                  ( uint64 *pnTotalBytes,
                                                          uint64 *puAvailableBytes ) override
   {
     return pRealStorage->GetQuota (pnTotalBytes, puAvailableBytes);                                 }
  bool                        IsCloudEnabledForAccount  ( void                     ) override
   { return pRealStorage->IsCloudEnabledForAccount ();                                               }
  bool                        IsCloudEnabledForApp      ( void                     ) override
   { return pRealStorage->IsCloudEnabledForApp ();                                                   }
  void                        SetCloudEnabledForApp     ( bool   bEnabled          ) override
   { return pRealStorage->SetCloudEnabledForApp (bEnabled);                                          }

  // user generated content

  // Downloads a UGC file.  A priority value of 0 will download the file immediately,
  // otherwise it will wait to download the file until all downloads with a lower priority
  // value are completed.  Downloads with equal priority will occur simultaneously.
  SteamAPICall_t              UGCDownload               ( UGCHandle_t hContent,
                                                          uint32      unPriority       ) override
   { return pRealStorage->UGCDownload (hContent, unPriority);                                            }

  // Gets the amount of data downloaded so far for a piece of content. pnBytesExpected can be 0 if function returns false
  // or if the transfer hasn't started yet, so be careful to check for that before dividing to get a percentage
  bool                        GetUGCDownloadProgress    ( UGCHandle_t  hContent,
                                                          int32       *pnBytesDownloaded,
                                                          int32       *pnBytesExpected ) override
   { return pRealStorage->GetUGCDownloadProgress (hContent, pnBytesDownloaded, pnBytesExpected);         }

  // Gets metadata for a file after it has been downloaded. This is the same metadata given in the RemoteStorageDownloadUGCResult_t call result
  bool                        GetUGCDetails             ( UGCHandle_t   hContent,
                                                          AppId_t      *pnAppID,
                                                          char        **ppchName,
                                                          int32        *pnFileSizeInBytes,
                                             OUT_STRUCT() CSteamID     *pSteamIDOwner  ) override
   { return pRealStorage->GetUGCDetails (hContent, pnAppID, ppchName, pnFileSizeInBytes, pSteamIDOwner); }

  // After download, gets the content of the file.  
  // Small files can be read all at once by calling this function with an offset of 0 and cubDataToRead equal to the size of the file.
  // Larger files can be read in chunks to reduce memory usage (since both sides of the IPC client and the game itself must allocate
  // enough memory for each chunk).  Once the last byte is read, the file is implicitly closed and further calls to UGCRead will fail
  // unless UGCDownload is called again.
  // For especially large files (anything over 100MB) it is a requirement that the file is read in chunks.
  int32	                      UGCRead                   ( UGCHandle_t     hContent,
                                                          void           *pvData,
                                                          int32           cubDataToRead,
                                                          uint32          cOffset,
                                                          EUGCReadAction  eAction ) override
   { return pRealStorage->UGCRead (hContent, pvData, cubDataToRead, cOffset, eAction);              }

  // Functions to iterate through UGC that has finished downloading but has not yet been read via UGCRead()
  int32	                      GetCachedUGCCount         ( void                    ) override
   { 
     steam_log.Log (L"ISteamRemoteStorage::GetCachedUGCCount ()");

     return pRealStorage->GetCachedUGCCount  ();                                                    }
  UGCHandle_t                 GetCachedUGCHandle        ( int32 iCachedContent    ) override
   { return pRealStorage->GetCachedUGCHandle (iCachedContent);                                      }

  // publishing UGC
  SteamAPICall_t              PublishWorkshopFile              ( const char                                  *pchFile,
                                                                 const char                                  *pchPreviewFile,
                                                                       AppId_t                                nConsumerAppId,
                                                                 const char                                  *pchTitle,
                                                                 const char                                  *pchDescription,
                                                                       ERemoteStoragePublishedFileVisibility  eVisibility,
                                                                       SteamParamStringArray_t               *pTags,
                                                                       EWorkshopFileType                      eWorkshopFileType ) override
   { return pRealStorage->PublishWorkshopFile ( pchFile, pchPreviewFile, nConsumerAppId,
                                                  pchTitle, pchDescription, eVisibility, pTags, eWorkshopFileType );                              }

  PublishedFileUpdateHandle_t CreatePublishedFileUpdateRequest (       PublishedFileId_t                      unPublishedFileId ) override
   { return pRealStorage->CreatePublishedFileUpdateRequest (unPublishedFileId);                                                                   }
  bool                        UpdatePublishedFileFile          (       PublishedFileUpdateHandle_t            updateHandle,
                                                                 const char                                  *pchFile           ) override
   { return pRealStorage->UpdatePublishedFileFile        (updateHandle, pchFile);                                                                 }
  bool                        UpdatePublishedFilePreviewFile   (       PublishedFileUpdateHandle_t            updateHandle,
                                                                 const char                                  *pchPreviewFile    ) override
   { return pRealStorage->UpdatePublishedFilePreviewFile (updateHandle, pchPreviewFile);                                                          }
  bool                        UpdatePublishedFileTitle         (       PublishedFileUpdateHandle_t            updateHandle,
                                                                 const char                                  *pchTitle          ) override
   { return pRealStorage->UpdatePublishedFileTitle       (updateHandle, pchTitle);                                                                }
  bool                        UpdatePublishedFileDescription   (       PublishedFileUpdateHandle_t            updateHandle,
                                                                 const char                                  *pchDescription    ) override
   { return pRealStorage->UpdatePublishedFileDescription (updateHandle, pchDescription);                                                          }
  bool                        UpdatePublishedFileVisibility    (       PublishedFileUpdateHandle_t            updateHandle,
                                                                       ERemoteStoragePublishedFileVisibility  eVisibility       ) override
   { return pRealStorage->UpdatePublishedFileVisibility  (updateHandle, eVisibility);                                                             }
  bool                        UpdatePublishedFileTags          (       PublishedFileUpdateHandle_t            updateHandle,
                                                                       SteamParamStringArray_t               *pTags             ) override
   { return pRealStorage->UpdatePublishedFileTags        (updateHandle, pTags);                                                                   }
  SteamAPICall_t              CommitPublishedFileUpdate        (       PublishedFileUpdateHandle_t            updateHandle      ) override
   { return pRealStorage->CommitPublishedFileUpdate      (updateHandle);                                                                          }

  // Gets published file details for the given publishedfileid.  If unMaxSecondsOld is greater than 0,
  // cached data may be returned, depending on how long ago it was cached.  A value of 0 will force a refresh.
  // A value of k_WorkshopForceLoadPublishedFileDetailsFromCache will use cached data if it exists, no matter how old it is.
  SteamAPICall_t              GetPublishedFileDetails                 (       PublishedFileId_t                      unPublishedFileId,
                                                                              uint32                                 unMaxSecondsOld      ) override
   { return pRealStorage->GetPublishedFileDetails      (unPublishedFileId, unMaxSecondsOld);                                                                }
  SteamAPICall_t              DeletePublishedFile                     (       PublishedFileId_t                      unPublishedFileId    ) override
   { return pRealStorage->DeletePublishedFile          (unPublishedFileId);                                                                                 }

  // enumerate the files that the current user published with this app
  SteamAPICall_t              EnumerateUserPublishedFiles             (       uint32                                 unStartIndex         ) override
   { steam_log.Log (L"ISteamRemoteStorage::EnumerateUserPublishedFiles ()");

     return pRealStorage->EnumerateUserPublishedFiles  (unStartIndex);                                                                                      }
  SteamAPICall_t              SubscribePublishedFile                  (       PublishedFileId_t                      unPublishedFileId    ) override
   { return pRealStorage->SubscribePublishedFile       (unPublishedFileId);                                                                                 }
  SteamAPICall_t              EnumerateUserSubscribedFiles            (       uint32                                 unStartIndex         ) override
   { steam_log.Log (L"ISteamRemoteStorage::EnumerateUserSubscribedFiles ()");

     return pRealStorage->EnumerateUserSubscribedFiles (unStartIndex);                                                                                      }
  SteamAPICall_t              UnsubscribePublishedFile                (       PublishedFileId_t                      unPublishedFileId    ) override
   { return pRealStorage->UnsubscribePublishedFile     (unPublishedFileId);                                                                                 }
  bool                        UpdatePublishedFileSetChangeDescription (       PublishedFileUpdateHandle_t            updateHandle,
                                                                        const char                                  *pchChangeDescription ) override
   { return pRealStorage->UpdatePublishedFileSetChangeDescription (updateHandle, pchChangeDescription);                                                     }
  SteamAPICall_t              GetPublishedItemVoteDetails             (       PublishedFileId_t                      unPublishedFileId    ) override
   { return pRealStorage->GetPublishedItemVoteDetails (unPublishedFileId);                                                                                  }
  SteamAPICall_t              UpdateUserPublishedItemVote             (       PublishedFileId_t                      unPublishedFileId,
                                                                              bool                                   bVoteUp              ) override
   { return pRealStorage->UpdateUserPublishedItemVote (unPublishedFileId, bVoteUp);                                                                         }
  SteamAPICall_t              GetUserPublishedItemVoteDetails         (       PublishedFileId_t                      unPublishedFileId    ) override
   { return pRealStorage->GetUserPublishedItemVoteDetails (unPublishedFileId);                                                                              }
  SteamAPICall_t              EnumerateUserSharedWorkshopFiles        (       CSteamID                               steamId,
                                                                              uint32                                 unStartIndex,
                                                                              SteamParamStringArray_t               *pRequiredTags,
                                                                              SteamParamStringArray_t               *pExcludedTags        ) override
   {
     steam_log.Log (L"ISteamRemoteStorage::EnumerateUserSharedWorkshopFiles ()");

     return pRealStorage->EnumerateUserSharedWorkshopFiles (steamId, unStartIndex, pRequiredTags, pExcludedTags);                                           }
  SteamAPICall_t              PublishVideo                            (       EWorkshopVideoProvider                 eVideoProvider,
                                                                        const char                                  *pchVideoAccount,
                                                                        const char                                  *pchVideoIdentifier,
                                                                        const char                                  *pchPreviewFile,
                                                                              AppId_t                                nConsumerAppId,
                                                                        const char                                  *pchTitle,
                                                                        const char                                  *pchDescription,
                                                                              ERemoteStoragePublishedFileVisibility  eVisibility,
                                                                              SteamParamStringArray_t               *pTags                ) override
   { return pRealStorage->PublishVideo ( eVideoProvider, pchVideoAccount, pchVideoIdentifier,
                                           pchPreviewFile, nConsumerAppId, pchTitle, pchDescription,
                                             eVisibility, pTags );                                                                                          }
  SteamAPICall_t              SetUserPublishedFileAction              (       PublishedFileId_t                      unPublishedFileId,
                                                                              EWorkshopFileAction                    eAction              ) override
   { return pRealStorage->SetUserPublishedFileAction (unPublishedFileId, eAction);                                                                          }
  SteamAPICall_t              EnumeratePublishedFilesByUserAction     (       EWorkshopFileAction                    eAction,
                                                                              uint32                                 unStartIndex         ) override
   { steam_log.Log (L"ISteamRemoteStorage::EnumeratePublishedFilesByUserAction ()");

    return pRealStorage->EnumeratePublishedFilesByUserAction (eAction, unStartIndex);                                                                      }

  // this method enumerates the public view of workshop files
  SteamAPICall_t              EnumeratePublishedWorkshopFiles         (       EWorkshopEnumerationType               eEnumerationType,
                                                                              uint32                                 unStartIndex,
                                                                              uint32                                 unCount,
                                                                              uint32                                 unDays,
                                                                              SteamParamStringArray_t               *pTags,
                                                                              SteamParamStringArray_t               *pUserTags            ) override
   {
     steam_log.Log (L"ISteamRemoteStorage::EnumeratePublishedWorkshopFiles ()");

     return pRealStorage->EnumeratePublishedWorkshopFiles ( eEnumerationType, unStartIndex, unCount, unDays, pTags, pUserTags );                            }

  SteamAPICall_t              UGCDownloadToLocation                   (       UGCHandle_t                            hContent,
                                                                        const char                                  *pchLocation,
                                                                              uint32                                 unPriority           ) override
   { return pRealStorage->UGCDownloadToLocation (hContent, pchLocation, unPriority);                                                                        }

private:
  ISteamRemoteStorage014* pRealStorage;
};


using SteamAPI_ISteamClient_GetISteamRemoteStorage_pfn = ISteamRemoteStorage* (S_CALLTYPE *)(
  ISteamClient *This,
  HSteamUser    hSteamuser,
  HSteamPipe    hSteamPipe,
  const char   *pchVersion
  );
SteamAPI_ISteamClient_GetISteamRemoteStorage_pfn   SteamAPI_ISteamClient_GetISteamRemoteStorage_Original = nullptr;

#define STEAMREMOTESTORAGE_INTERFACE_VERSION_012 "STEAMREMOTESTORAGE_INTERFACE_VERSION012"
#define STEAMREMOTESTORAGE_INTERFACE_VERSION_014 "STEAMREMOTESTORAGE_INTERFACE_VERSION014"

ISteamRemoteStorage*
S_CALLTYPE
SteamAPI_ISteamClient_GetISteamRemoteStorage_Detour ( ISteamClient *This,
                                                      HSteamUser    hSteamuser,
                                                      HSteamPipe    hSteamPipe,
                                                      const char   *pchVersion )
{
  SK_RunOnce (
    steam_log.Log ( L"[!] %hs (..., %hs)",
                      __FUNCTION__, pchVersion )
  );

  ISteamRemoteStorage* pRemoteStorage =
    SteamAPI_ISteamClient_GetISteamRemoteStorage_Original ( This,
                                                              hSteamuser,
                                                                hSteamPipe,
                                                                  pchVersion );

  if (pRemoteStorage != nullptr)
  {
    if ((! lstrcmpA (pchVersion, STEAMREMOTESTORAGE_INTERFACE_VERSION_012)))
    {
      if (SK_SteamWrapper_remap_remotestorage.count (pRemoteStorage))
         return reinterpret_cast <ISteamRemoteStorage *> (SK_SteamWrapper_remap_remotestorage [pRemoteStorage]);

      else
      {
        SK_SteamWrapper_remap_remotestorage [pRemoteStorage] =
          reinterpret_cast <IWrapSteamRemoteStorage *> (
                new IWrapSteamRemoteStorage012 (pRemoteStorage)
          );

        return reinterpret_cast <ISteamRemoteStorage *> (SK_SteamWrapper_remap_remotestorage [pRemoteStorage]);
      }
    }

    else if ((! lstrcmpA (pchVersion, STEAMREMOTESTORAGE_INTERFACE_VERSION_014)))
    {
      if (SK_SteamWrapper_remap_remotestorage.count (pRemoteStorage))
         return reinterpret_cast <ISteamRemoteStorage *> (SK_SteamWrapper_remap_remotestorage [pRemoteStorage]);

      else
      {
        SK_SteamWrapper_remap_remotestorage [pRemoteStorage] =
          reinterpret_cast <IWrapSteamRemoteStorage *> (
                new IWrapSteamRemoteStorage014 (
                  reinterpret_cast <ISteamRemoteStorage014 *> (pRemoteStorage)
                )
          );

        return reinterpret_cast <ISteamRemoteStorage *> (SK_SteamWrapper_remap_remotestorage [pRemoteStorage]);
      }
    }

    else
    {
      SK_RunOnce (
        steam_log.Log ( L"Game requested unexpected interface version (%hs)!",
                          pchVersion )
      );

      return pRemoteStorage;
    }
  }

  return nullptr;
}

ISteamRemoteStorage*
SK_SteamWrapper_WrappedClient_GetISteamRemoteStorage ( ISteamClient *This,
                                                       HSteamUser    hSteamuser,
                                                       HSteamPipe    hSteamPipe,
                                                       const char   *pchVersion )
{
  SK_RunOnce (
    steam_log.Log ( L"[!] %hs (..., %hs)",
                      __FUNCTION__, pchVersion )
  );

  ISteamRemoteStorage* pRemoteStorage =
    This->GetISteamRemoteStorage ( hSteamuser,
                                     hSteamPipe,
                                       pchVersion );

  if (pRemoteStorage != nullptr)
  {
    if ((! lstrcmpA (pchVersion, STEAMREMOTESTORAGE_INTERFACE_VERSION_012)))
    {
      if (SK_SteamWrapper_remap_remotestorage.count (pRemoteStorage))
         return reinterpret_cast <ISteamRemoteStorage *> (SK_SteamWrapper_remap_remotestorage [pRemoteStorage]);

      else
      {
        SK_SteamWrapper_remap_remotestorage [pRemoteStorage] =
          reinterpret_cast <IWrapSteamRemoteStorage *> (
                new IWrapSteamRemoteStorage012 (pRemoteStorage)
          );

        return reinterpret_cast <ISteamRemoteStorage *> (SK_SteamWrapper_remap_remotestorage [pRemoteStorage]);
      }
    }

    else if ((! lstrcmpA (pchVersion, STEAMREMOTESTORAGE_INTERFACE_VERSION_014)))
    {
      if (SK_SteamWrapper_remap_remotestorage.count (pRemoteStorage))
         return reinterpret_cast <ISteamRemoteStorage *> (SK_SteamWrapper_remap_remotestorage [pRemoteStorage]);

      else
      {
        SK_SteamWrapper_remap_remotestorage [pRemoteStorage] =
          reinterpret_cast <IWrapSteamRemoteStorage *> (
                new IWrapSteamRemoteStorage014 (
                  reinterpret_cast <ISteamRemoteStorage014 *> (pRemoteStorage)
                )
          );

        return reinterpret_cast <ISteamRemoteStorage *> (SK_SteamWrapper_remap_remotestorage [pRemoteStorage]);
      }
    }

    else
    {
      SK_RunOnce (
        steam_log.Log ( L"Game requested unexpected interface version (%hs)!",
                          pchVersion )
      );

      return pRemoteStorage;
    }
  }

  return nullptr;
}

using SteamRemoteStorage_pfn = ISteamRemoteStorage* (S_CALLTYPE *)(
        void
      );
SteamRemoteStorage_pfn SteamRemoteStorage_Original = nullptr;

ISteamRemoteStorage*
S_CALLTYPE
SteamRemoteStorage_Detour (void)
{
  SK_RunOnce (
    steam_log.Log ( L"[!] %hs ()",
                      __FUNCTION__ )
  );

#ifndef DANGEROUS_INTERFACE_ALIASING
  return SteamRemoteStorage_Original ();
#else
  if (steam_ctx.RemoteStorageVersion () == 12)
  {
    ISteamRemoteStorage* pRemoteStorage =
      static_cast <ISteamRemoteStorage *> ( SteamRemoteStorage_Original () );
    
    if (SK_SteamWrapper_remap_remotestorage.count (pRemoteStorage))
       return reinterpret_cast <ISteamRemoteStorage *> (SK_SteamWrapper_remap_remotestorage [pRemoteStorage]);

    else
    {
      SK_SteamWrapper_remap_remotestorage [pRemoteStorage] =
        reinterpret_cast <IWrapSteamRemoteStorage *> (
              new IWrapSteamRemoteStorage012 (pRemoteStorage)
        );

      return reinterpret_cast <ISteamRemoteStorage *> (SK_SteamWrapper_remap_remotestorage [pRemoteStorage]);
    }
  }

  else if (steam_ctx.RemoteStorageVersion () == 14)
  {
    ISteamRemoteStorage* pRemoteStorage =
      static_cast <ISteamRemoteStorage *> ( SteamRemoteStorage_Original () );

    if (SK_SteamWrapper_remap_remotestorage.count (pRemoteStorage))
       return reinterpret_cast <ISteamRemoteStorage *> (SK_SteamWrapper_remap_remotestorage [pRemoteStorage]);

    else
    {
      SK_SteamWrapper_remap_remotestorage [pRemoteStorage] =
        reinterpret_cast <IWrapSteamRemoteStorage *> (
              new IWrapSteamRemoteStorage014 (
                reinterpret_cast <ISteamRemoteStorage014 *> (pRemoteStorage)
              )
        );

      return reinterpret_cast <ISteamRemoteStorage *> (SK_SteamWrapper_remap_remotestorage [pRemoteStorage]);
    }
  }

  return SteamRemoteStorage_Original ();
#endif
}