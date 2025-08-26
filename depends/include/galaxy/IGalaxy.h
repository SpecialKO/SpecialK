#ifndef GALAXY_I_GALAXY_H
#define GALAXY_I_GALAXY_H

/**
 * @file
 * Contains IGalaxy, which is the main interface for controlling the Galaxy
 * Peer.
 */

#include "GalaxyExport.h"

namespace galaxy
{
	namespace api
	{
		/**
		 * @addtogroup api
		 * @{
		 */

		class IUser;
		class IFriends;
		class IMatchmaking;
		class INetworking;
		class IStats;
		class IUtils;
		class IApps;
		class IStorage;
		class ICustomNetworking;
		class IListenerRegistrar;
		class ILogger;
		class IError;

		/**
		 * The group of options used for Init configuration.
		 */
		struct InitOptions
		{
			/**
			 * InitOptions constructor.
			 *
			 * @param _clientID The ID of the client.
			 * @param _clientSecret The secret of the client.
			 * @param _galaxyPeerPath Path to the galaxyPeer library location.
			 * @param _throwExceptions Indicates if Galaxy should throw exceptions.
			 * @param _configFilePath The path to folder which contains configuration files.
			 */
			InitOptions(const char* _clientID, const char* _clientSecret, const char* _galaxyPeerPath = "", bool _throwExceptions = true, const char* _configFilePath = ".")
				: throwExceptions(_throwExceptions)
				, clientID(_clientID)
				, clientSecret(_clientSecret)
				, galaxyPeerPath(_galaxyPeerPath)
				, configFilePath(_configFilePath)
			{
			}

			const bool throwExceptions; ///< Indicates if Galaxy should throw exceptions.
			const char* clientID; ///< The ID of the client.
			const char* clientSecret; ///< The secret of the client.
			const char* galaxyPeerPath; ///< Path to the galaxyPeer library location.
			const char* configFilePath; ///< The path to folder which contains configuration files.
		};

		/**
		 * The main interface for controlling the Galaxy Peer.
		 */
		class GALAXY_DLL_EXPORT IGalaxy
		{
		public:

			virtual ~IGalaxy()
			{
			}

			/**
			 * Initializes the Galaxy Peer with specified credentials.
			 *
			 * @remark When you do not need the Galaxy Peer anymore, you should call
			 * Shutdown() in order to deactivate it and free its resources.
			 *
			 * @remark This method can succeed partially, in which case it leaves
			 * Galaxy Peer partially functional, hence even in case of an error, be
			 * sure to call Shutdown() when you do not need the Galaxy Peer anymore.
			 * See the documentation of specific interfaces on how they behave.
			 *
			 * @param clientID The ID of the client.
			 * @param clientSecret The secret of the client.
			 * @param throwExceptions Indicates if Galaxy should throw exceptions.
			 */
			virtual void Init(const char* clientID, const char* clientSecret, bool throwExceptions = true) = 0;

			/**
			 * Initializes the Galaxy Peer with specified credentials.
			 *
			 * @remark When you do not need the Galaxy Peer anymore, you should call
			 * Shutdown() in order to deactivate it and free its resources.
			 *
			 * @remark This method can succeed partially, in which case it leaves
			 * Galaxy Peer partially functional, hence even in case of an error, be
			 * sure to call Shutdown() when you do not need the Galaxy Peer anymore.
			 * See the documentation of specific interfaces on how they behave.
			 *
			 * @param initOptions The group of the init options.
			 */
			virtual void Init(const InitOptions& initOptions) = 0;

			/**
			 * Initializes the Galaxy Peer with specified credentials.
			 *
			 * @remark When you do not need the Galaxy Peer anymore, you should call
			 * Shutdown() in order to deactivate it and free its resources.
			 *
			 * @remark This method can succeed partially, in which case it leaves
			 * Galaxy Peer partially functional, hence even in case of an error, be
			 * sure to call Shutdown() when you do not need the Galaxy Peer anymore.
			 * See the documentation of specific interfaces on how they behave.
			 *
			 * @warning This method allows for using local Galaxy Peer library
			 * instead of the one provided by the desktop service Galaxy Client.
			 * In the future the method will be removed and loading of the library
			 * will rely solely on the desktop service.
			 *
			 * @param clientID The ID of the client.
			 * @param clientSecret The secret of the client.
			 * @param galaxyPeerPath Path to the galaxyPeer library location.
			 * @param throwExceptions indicates if Galaxy should throw exceptions.
			 */
			virtual void InitLocal(const char* clientID, const char* clientSecret, const char* galaxyPeerPath = ".", bool throwExceptions = true) = 0;

			/**
			 * Shuts down the Galaxy Peer.
			 *
			 * The Galaxy Peer is deactivated and brought to the state it had when it
			 * was created and before it was initialized.
			 *
			 * @remark Delete all self-registering listeners before calling
			 * Shutdown().
			 */
			virtual void Shutdown() = 0;

			/**
			 * Returns an instance of IUser.
			 *
			 * @return An instance of IUser.
			 */
			virtual IUser* GetUser() const = 0;

			/**
			 * Returns an instance of IFriends.
			 *
			 * @return An instance of IFriends.
			 */
			virtual IFriends* GetFriends() const = 0;

			/**
			 * Returns an instance of IMatchmaking.
			 *
			 * @return An instance of IMatchmaking.
			 */
			virtual IMatchmaking* GetMatchmaking() const = 0;

			/**
			 * Returns an instance of INetworking that allows to communicate
			 * as a regular lobby member.
			 *
			 * @return An instance of INetworking.
			 */
			virtual INetworking* GetNetworking() const = 0;

			/**
			 * Returns an instance of INetworking that allows to communicate
			 * as the lobby host.
			 *
			 * @return An instance of INetworking.
			 */
			virtual INetworking* GetServerNetworking() const = 0;

			/**
			 * Returns an instance of IStats.
			 *
			 * @return An instance of IStats.
			 */
			virtual IStats* GetStats() const = 0;

			/**
			 * Returns an instance of IUtils.
			 *
			 * @return An instance of IUtils.
			 */
			virtual IUtils* GetUtils() const = 0;

			/**
			 * Returns an instance of IApps.
			 *
			 * @return An instance of IApps.
			 */
			virtual IApps* GetApps() const = 0;

			/**
			 * Returns an instance of IStorage.
			 *
			 * @return An instance of IStorage.
			 */
			virtual IStorage* GetStorage() const = 0;

			/**
			* Returns an instance of ICustomNetworking.
			*
			* @return An instance of ICustomNetworking.
			*/
			virtual ICustomNetworking* GetCustomNetworking() const = 0;

			/**
			 * Returns an instance of IListenerRegistrar.
			 *
			 * @return An instance of IListenerRegistrar.
			 */
			virtual IListenerRegistrar* GetListenerRegistrar() const = 0;

			/**
			 * Returns an instance of ILogger.
			 *
			 * @return An instance of ILogger.
			 */
			virtual ILogger* GetLogger() const = 0;

			/**
			 * Makes the Galaxy Peer process its input and output streams.
			 *
			 * During the phase of processing data, Galaxy Peer recognizes specific
			 * events and casts notifications for callback listeners immediately.
			 */
			virtual void ProcessData() = 0;

			/**
			 * Retrieves error connected with the last API call.
			 *
			 * @return Either the last API call error or NULL if there was no error.
			 */
			virtual const IError* GetError() const = 0;
		};

		/** @} */
	}
}

#endif