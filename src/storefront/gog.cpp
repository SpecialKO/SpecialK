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
#include <storefront/gog.h>
#include <storefront/achievements.h>

#include <json/json.hpp>

#include <galaxy/IListenerRegistrar.h>
#include <galaxy/IUtils.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"GOG Galaxy"

#define GALAXY_VIRTUAL_HOOK(_Base,_Index,_Name,_Override,_Original,_Type) {   \
  void** _vftable = *(void***)*(_Base);                                       \
                                                                              \
  if ((_Original) == nullptr) {                                               \
    SK_CreateVFTableHook2 ( L##_Name,                                         \
                              _vftable,                                       \
                                (_Index),                                     \
                                  (_Override),                                \
                                    (LPVOID *)&(_Original));                  \
  }                                                                           \
}

class SK_Galaxy_OverlayManager : public galaxy::api::IOverlayStateChangeListener
{
public:
  bool isActive (void) const {
    return active_;
  }

  void OnOverlayStateChanged (bool overlayIsActive)
  {
    std::lock_guard <SK_Thread_HybridSpinlock>
         lock (callback_cs);

    // If the game has an activation callback installed, then
    //   it's also going to see this event... make a note of that when
    //     tracking its believed overlay state.
    if (SK::Galaxy::IsOverlayAware ())
        SK::Galaxy::overlay_state = overlayIsActive;


    // If we want to use this as our own, then don't let the Epic overlay
    //   unpause the game on deactivation unless the control panel is closed.
    if (config.platform.reuse_overlay_pause && SK_Platform_IsOverlayAware ())
    {
      // Deactivating, but we might want to hide this event from the game...
      if (overlayIsActive == false)
      {
        // Undo the event the game is about to receive.
        if (SK_ImGui_Visible) SK_Platform_SetOverlayState (true);
      }
    }

    const bool wasActive =
                  active_;

    active_ = overlayIsActive;

    if (wasActive != active_)
    {
      auto& io =
        ImGui::GetIO ();

      static bool capture_keys  = io.WantCaptureKeyboard;
      static bool capture_text  = io.WantTextInput;
      static bool capture_mouse = io.WantCaptureMouse;
      static bool nav_active    = io.NavActive;

      // When the overlay activates, stop blocking
      //   input !!
      if (! wasActive)
      {
        capture_keys  =
          io.WantCaptureKeyboard;
          io.WantCaptureKeyboard = false;

        capture_text  =
          io.WantTextInput;
          io.WantTextInput       = false;

        capture_mouse =
          io.WantCaptureMouse;
          io.WantCaptureMouse    = false;

        nav_active    =
          io.NavActive;
          io.NavActive           = false;

         ImGui::SetWindowFocus (nullptr);
      }

      else
      {
        io.WantCaptureKeyboard = SK_ImGui_Visible ? capture_keys  : false;
        io.WantCaptureMouse    = SK_ImGui_Visible ? capture_mouse : false;
        io.NavActive           = SK_ImGui_Visible ? nav_active    : false;

        ImGui::SetWindowFocus (nullptr);
        io.WantTextInput       = false;//capture_text;
      }
    }
  }

  void invokeCallbacks (bool active)
  {
    std::lock_guard <SK_Thread_HybridSpinlock>
         lock (callback_cs);

    for ( const auto &[listener,callback_data] : callbacks )
    {
      if (listener == nullptr)
        continue;

      auto callback =
        (galaxy::api::IOverlayStateChangeListener *)listener;

      callback->OnOverlayStateChanged (active);
    }
  }

  void
  Register (galaxy::api::IGalaxyListener* listener)
  {
    SK_LOG_FIRST_CALL

    std::lock_guard <SK_Thread_HybridSpinlock>
         lock (callback_cs);

    if (listener != nullptr)
    {
      callbacks [listener] = { };
    }
  }

  void
  Unregister (galaxy::api::IGalaxyListener* listener)
  {
    std::lock_guard <SK_Thread_HybridSpinlock>
         lock (callback_cs);

    if ( const auto cb  = callbacks.find (listener);
                    cb != callbacks.end  (  ) )
    {
      callbacks.erase (cb);
    }
  }

  bool
  isOverlayAware (void) const
  {
    return
      (! callbacks.empty ());
  }

private:
  bool cursor_visible_ = false; // Cursor visible prior to activation?
  bool active_         = false;

  struct callback_s
  {
    // TODO
  };

public:
  std::unordered_map <galaxy::api::IGalaxyListener*, callback_s> callbacks;
  SK_Thread_HybridSpinlock                                       callback_cs;
};

SK_LazyGlobal <SK_Galaxy_OverlayManager>     galaxy_overlay;
//SK_LazyGlobal <SK_Galaxy_AchievementManager> galaxy_achievements;

bool
WINAPI
SK_IsGalaxyOverlayActive (void)
{
  return galaxy_overlay->isActive ();
}

void
__stdcall
SK::Galaxy::SetOverlayState (bool active)
{
  if (config.platform.silent)
    return;

  galaxy_overlay->invokeCallbacks (active);

  overlay_state = active;
}

bool
__stdcall
SK::Galaxy::GetOverlayState (bool real)
{
  return real ? SK_IsGalaxyOverlayActive () :
    overlay_state;
}

bool
__stdcall
SK::Galaxy::IsOverlayAware (void)
{
  if (! galaxy_overlay.getPtr ())
    return false;

  return
    galaxy_overlay->isOverlayAware ();
}

class SK_Galaxy_AchievementManager : public SK_AchievementManager
{
public:
  void unlock (const char* szAchievement)
  {
    if (szAchievement == nullptr)
      return;

    size_t index   = (size_t)-1;
    bool   numeric = true;

    for ( const char* ch = szAchievement ;
                     *ch != '\0'         ;
                      ch = CharNextA (ch) )
    {
      if ((! isalnum (ch [0])) || isalpha (ch [0]))
      {
        numeric = false;
        break;
      }
    }

    if (numeric)
    {
      index = atoi (szAchievement);
    }

    for (uint32 i = 0; i < SK_Galaxy_GetNumPossibleAchievements (); i++)
    {
      Achievement* achievement =
                   achievements.list [i];

      if (achievement == nullptr || achievement->name_.empty ())
        continue;

      if (! _stricmp (achievement->name_.c_str (), szAchievement))
      {
        index = i;
        break;
      }
    }

    if (index >= SK_Galaxy_GetNumPossibleAchievements ())
      return;

    Achievement* achievement =
                 achievements.list [index];

    if ( config.platform.achievements.popup.show
              && achievement != nullptr )
    {
      if (platform_popup_cs != nullptr)
          platform_popup_cs->lock ();

      SK_AchievementPopup popup = { };

      // Little bit of sanity goes a long way
      if (achievement->progress_.current ==
          achievement->progress_.max)
      {
        // It's implicit
        achievement->unlocked_ = true;

        if (achievement->time_ == 0)
            achievement->time_ = time (nullptr);
      }

      popup.window      = nullptr;
      popup.final_pos   = false;
      popup.achievement = achievement;

      popups.push_back (popup);

      if (platform_popup_cs != nullptr)
          platform_popup_cs->unlock ();
    }

    if (config.platform.achievements.play_sound && (! unlock_sound.empty ()))
    {
      playSound ();
    }

    // If the user wants a screenshot, but no popups (why?!), this is when
    //   the screenshot needs to be taken.
    if (       config.platform.achievements.take_screenshot )
    {  if ( (! config.platform.achievements.popup.show) )
       {
         SK::SteamAPI::TakeScreenshot (
           SK_ScreenshotStage::EndOfFrame, false,
             SK_FormatString ("Achievements\\%hs", achievement->text_.unlocked.human_name.c_str ())
         );
       }
    }

    log_all_achievements ();
  }

  void log_all_achievements (void) const
  {
    for (uint32 i = 0; i < SK_Galaxy_GetNumPossibleAchievements (); i++)
    {
      const Achievement* achievement =
                         achievements.list [i];

      if (achievement == nullptr || achievement->name_.empty ())
        continue;

      gog_log->LogEx  (false, L"\n [%c] Achievement %03lu......: '%hs'\n",
                       achievement->unlocked_ ? L'X' : L' ',
                    i, achievement->name_.data ());
      gog_log->LogEx  (false,
                       L"  + Human Readable Name...: %hs\n",
                       achievement->      unlocked_                    ?
                       achievement->text_.unlocked.human_name.c_str () :
                       achievement->text_.  locked.human_name.c_str ());
      if (! (achievement->unlocked_ && achievement->text_.locked.desc.empty ()))
      {
        gog_log->LogEx (false,
                       L"  *- Detailed Description.: %hs\n",
                       achievement->text_.locked.desc.c_str ());
      }
      else if ((achievement->unlocked_ && !achievement->text_.unlocked.desc.empty ()))
      {
        gog_log->LogEx (false,
                        L"  *- Detailed Description.: %hs\n",
                        achievement->text_.unlocked.desc.c_str ());
      }

      if (achievement->global_percent_ > 0.0f)
      {
        gog_log->LogEx (false,
                        L"  #-- Rarity (Global).....: %6.2f%%\n",
                        achievement->global_percent_);
      }
      if (achievement->friends_.possible > 0)
      {
        gog_log->LogEx (false,
                        L"  #-- Rarity (Friend).....: %6.2f%%\n",
          100.0 * (static_cast <double> (achievement->friends_.unlocked) /
                   static_cast <double> (achievement->friends_.possible)) );
      }

      if (achievement->progress_.current != achievement->progress_.max &&
          achievement->progress_.max     != 1) // Don't log achievements that are simple yes/no
      {
        gog_log->LogEx (false,
                        L"  @--- Progress To Unlock.: % 6.2f%%　　[ %d / %d ]\n",
         100.0 * (static_cast <double> (achievement->progress_.current) /
                  static_cast <double> (achievement->progress_.max)),
                                        achievement->progress_.current,
                                        achievement->progress_.max);
      }

      if (achievement->unlocked_)
      {
        gog_log->LogEx (false,
                        L"  @--- Player Unlocked At.: %s",
                        _wctime64 (&achievement->time_));
      }
    }

    epic_log->LogEx (false, L"\n");
  }

  void clear_achievement (int idx)
  {
    std::ignore = idx;
    //const Achievement* achievement =
    //                   achievements.list [idx];

    // TODO
  }

  int possible = 0;
};

SK_LazyGlobal <SK_Galaxy_AchievementManager> galaxy_achievements;

SK_AchievementManager*
SK_Galaxy_GetAchievementManager (void)
{
  return
    galaxy_achievements.getPtr ();
}

// The Galaxy client is not capable of reporting this... wtf?!
size_t
SK_Galaxy_GetNumPossibleAchievements (void)
{
  return
    galaxy_achievements->possible;
}

float
__stdcall
SK_Galaxy_PercentOfAchievementsUnlocked (void)
{
  return galaxy_achievements->getPercentOfAchievementsUnlocked ();
}

float
__stdcall
SK::Galaxy::PercentOfAchievementsUnlocked (void)
{
  return SK_Galaxy_PercentOfAchievementsUnlocked ();
}

int
__stdcall
SK_Galaxy_NumberOfAchievementsUnlocked (void)
{
  return galaxy_achievements->getNumberOfAchievementsUnlocked ();
}

int
__stdcall
SK::Galaxy::NumberOfAchievementsUnlocked (void)
{
  return SK_Galaxy_NumberOfAchievementsUnlocked ();
}

void
SK_Galaxy_PlayUnlockSound (void)
{
  galaxy_achievements->playSound ();
}

void
SK_Galaxy_LoadUnlockSound (const wchar_t* wszUnlockSound)
{
  galaxy_achievements->loadSound (wszUnlockSound);
}

void
SK_Galaxy_LogAllAchievements (void)
{
  galaxy_achievements->log_all_achievements ();
}

void
SK_Galaxy_UnlockAchievement (uint32_t idx)
{
  galaxy_achievements->unlock (std::to_string (idx).c_str ());
}

int
SK_Galaxy_DrawOSD ()
{
  if (galaxy_achievements.getPtr () != nullptr)
  {
    return
      galaxy_achievements->drawPopups ();
  }

  return 0;
}

static bool has_unlock_callback = false;

class GalaxyAllocator;
class IGalaxyThreadFactory;

namespace galaxy
{
	namespace api
	{
		/**
		 * @addtogroup api
		 * @{
		 */

		 /**
		 * The group of options used for Init configuration.
		 */
		struct InitOptions_1_152_1
		{
			/**
			 * InitOptions constructor.
			 *
			 * @remark On Android device Galaxy SDK needs a directory for internal storage (to keep a copy of system CA certificates).
			 * It should be an existing directory within the application file storage.
			 *
			 * @param [in] _clientID The ID of the client.
			 * @param [in] _clientSecret The secret of the client.
			 * @param [in] _configFilePath The path to a folder which contains configuration files. Note: a relative path is relative to the current working directory, but not to an executable.
			 * @param [in] _galaxyAllocator The custom memory allocator. The same instance has to be passed to both Galaxy Peer and Game Server.
			 * @param [in] _storagePath Path to a directory that can be used for storing internal SDK data. Used only on Android devices. See remarks for more information.
			 * @param [in] _host The local IP address this peer would bind to.
			 * @param [in] _port The local port used to communicate with GOG Galaxy Multiplayer server and other players.
			 * @param [in] _galaxyThreadFactory The custom thread factory used by GOG Galaxy SDK to spawn internal threads.
			 */
			InitOptions_1_152_1(
				const char* _clientID,
				const char* _clientSecret,
				const char* _configFilePath = ".",
				GalaxyAllocator* _galaxyAllocator = NULL,
				const char* _storagePath = NULL,
				const char* _host = NULL,
				uint16_t _port = 0,
				IGalaxyThreadFactory* _galaxyThreadFactory = NULL)
				: clientID(_clientID)
				, clientSecret(_clientSecret)
				, configFilePath(_configFilePath)
				, storagePath(_storagePath)
				, galaxyAllocator(_galaxyAllocator)
				, galaxyThreadFactory(_galaxyThreadFactory)
				, host(_host)
				, port(_port)
			{
			}

			const char* clientID; ///< The ID of the client.
			const char* clientSecret; ///< The secret of the client.
			const char* configFilePath; ///< The path to folder which contains configuration files.
			const char* storagePath; ///< The path to folder for storing internal SDK data. Used only on Android devices.
			GalaxyAllocator* galaxyAllocator; ///< The custom memory allocator used by GOG Galaxy SDK.
			IGalaxyThreadFactory* galaxyThreadFactory; ///< The custom thread factory used by GOG Galaxy SDK to spawn internal threads.
			const char* host; ///< The local IP address this peer would bind to.
			uint16_t port; ///< The local port used to communicate with GOG Galaxy Multiplayer server and other players.
		};

		/** @} */
	}
}

/**
 * @file
 * Contains data structures and interfaces related to user account.
 */

#include "galaxy/GalaxyID.h"
#include "galaxy/IListenerRegistrar.h"

namespace galaxy
{
	namespace api
	{
		/**
		 * @addtogroup api
		 * @{
		 */

		/**
		 * The ID of the session.
		 */
		typedef uint64_t SessionID;

		/**
		 * Listener for the events related to user authentication.
		 */
		class IAuthListener : public GalaxyTypeAwareListener<AUTH>
		{
		public:

			/**
			 * Notification for the event of successful sign in.
			 */
			virtual void OnAuthSuccess() = 0;

			/**
			 * Reason of authentication failure.
			 */
			enum FailureReason
			{
				FAILURE_REASON_UNDEFINED, ///< Undefined error.
				FAILURE_REASON_GALAXY_SERVICE_NOT_AVAILABLE, ///< Galaxy Service is not installed properly or fails to start.
				FAILURE_REASON_GALAXY_SERVICE_NOT_SIGNED_IN, ///< Galaxy Service is not signed in properly.
				FAILURE_REASON_CONNECTION_FAILURE, ///< Unable to communicate with backend services.
				FAILURE_REASON_NO_LICENSE, ///< User that is being signed in has no license for this application.
				FAILURE_REASON_INVALID_CREDENTIALS, ///< Unable to match client credentials (ID, secret) or user credentials (username, password).
				FAILURE_REASON_GALAXY_NOT_INITIALIZED, ///< Galaxy has not been initialized.
				FAILURE_REASON_EXTERNAL_SERVICE_FAILURE ///< Unable to communicate with external service.
			};

			/**
			 * Notification for the event of unsuccessful sign in.
			 *
			 * @param [in] failureReason The cause of the failure.
			 */
			virtual void OnAuthFailure(FailureReason failureReason) = 0;

			/**
			 * Notification for the event of loosing authentication.
			 *
			 * @remark Might occur any time after successfully signing in.
			 */
			virtual void OnAuthLost() = 0;
		};

		/**
		 * Globally self-registering version of IAuthListener.
		 */
		typedef SelfRegisteringListener<IAuthListener> GlobalAuthListener;

		/**
		 * Globally self-registering version of IAuthListener for the Game Server.
		 */
		typedef SelfRegisteringListener<IAuthListener, GameServerListenerRegistrar> GameServerGlobalAuthListener;

		/**
		 * Listener for the events related to starting of other sessions.
		 */
		class IOtherSessionStartListener : public GalaxyTypeAwareListener<OTHER_SESSION_START>
		{
		public:

			/**
			 * Notification for the event of other session being started.
			 */
			virtual void OnOtherSessionStarted() = 0;
		};

		/**
		 * Globally self-registering version of IOtherSessionStartListener.
		 */
		typedef SelfRegisteringListener<IOtherSessionStartListener> GlobalOtherSessionStartListener;

		/**
		 * Globally self-registering version of IOtherSessionStartListener for the Game Server.
		 */
		typedef SelfRegisteringListener<IOtherSessionStartListener, GameServerListenerRegistrar> GameServerGlobalOtherSessionStartListener;

		/**
		 * Listener for the event of a change of the operational state.
		 */
		class IOperationalStateChangeListener : public GalaxyTypeAwareListener<OPERATIONAL_STATE_CHANGE>
		{
		public:

			/**
			 * Aspect of the operational state.
			 */
			enum OperationalState
			{
				OPERATIONAL_STATE_SIGNED_IN = 0x0001, ///< User signed in.
				OPERATIONAL_STATE_LOGGED_ON = 0x0002 ///< User logged on.
			};

			/**
			 * Notification for the event of a change of the operational state.
			 *
			 * @param [in] operationalState The sum of the bit flags representing the operational state, as defined in IOperationalStateChangeListener::OperationalState.
			 */
			virtual void OnOperationalStateChanged(uint32_t operationalState) = 0;
		};

		/**
		 * Globally self-registering version of IOperationalStateChangeListener.
		 */
		typedef SelfRegisteringListener<IOperationalStateChangeListener> GlobalOperationalStateChangeListener;

		/**
		 * Globally self-registering version of IOperationalStateChangeListener for the GameServer.
		 */
		typedef SelfRegisteringListener<IOperationalStateChangeListener, GameServerListenerRegistrar> GameServerGlobalOperationalStateChangeListener;

		/**
		 * Listener for the events related to user data changes of current user only.
		 *
		 * @remark In order to get notifications about changes to user data of all users,
		 * use ISpecificUserDataListener instead.
		 */
		class IUserDataListener : public GalaxyTypeAwareListener<USER_DATA>
		{
		public:

			/**
			 * Notification for the event of user data change.
			 */
			virtual void OnUserDataUpdated() = 0;
		};

		/**
		 * Globally self-registering version of IUserDataListener.
		 */
		typedef SelfRegisteringListener<IUserDataListener> GlobalUserDataListener;

		/**
		 * Globally self-registering version of IUserDataListener for the GameServer.
		 */
		typedef SelfRegisteringListener<IUserDataListener, GameServerListenerRegistrar> GameServerGlobalUserDataListener;

		/**
		 * Listener for the events related to user data changes.
		 */
		class ISpecificUserDataListener : public GalaxyTypeAwareListener<SPECIFIC_USER_DATA>
		{
		public:

			/**
			 * Notification for the event of user data change.
			 *
			 * @param [in] userID The ID of the user.
			 */
			virtual void OnSpecificUserDataUpdated(GalaxyID userID) = 0;
		};

		/**
		 * Globally self-registering version of ISpecificUserDataListener.
		 */
		typedef SelfRegisteringListener<ISpecificUserDataListener> GlobalSpecificUserDataListener;

		/**
		 * Globally self-registering version of ISpecificUserDataListener for the Game Server.
		 */
		typedef SelfRegisteringListener<ISpecificUserDataListener, GameServerListenerRegistrar> GameServerGlobalSpecificUserDataListener;

		/**
		 * Listener for the event of retrieving a requested Encrypted App Ticket.
		 */
		class IEncryptedAppTicketListener : public GalaxyTypeAwareListener<ENCRYPTED_APP_TICKET_RETRIEVE>
		{
		public:

			/**
			 * Notification for an event of a success in retrieving the Encrypted App Ticket.
			 *
			 * In order to read the Encrypted App Ticket, call IUser::GetEncryptedAppTicket().
			 */
			virtual void OnEncryptedAppTicketRetrieveSuccess() = 0;

			/**
			 * The reason of a failure in retrieving an Encrypted App Ticket.
			 */
			enum FailureReason
			{
				FAILURE_REASON_UNDEFINED, ///< Unspecified error.
				FAILURE_REASON_CONNECTION_FAILURE ///< Unable to communicate with backend services.
			};

			/**
			 * Notification for the event of a failure in retrieving an Encrypted App Ticket.
			 *
			 * @param [in] failureReason The cause of the failure.
			 */
			virtual void OnEncryptedAppTicketRetrieveFailure(FailureReason failureReason) = 0;
		};

		/**
		 * Globally self-registering version of IEncryptedAppTicketListener.
		 */
		typedef SelfRegisteringListener<IEncryptedAppTicketListener> GlobalEncryptedAppTicketListener;

		/**
		 * Globally self-registering version of IEncryptedAppTicketListener for the Game Server.
		 */
		typedef SelfRegisteringListener<IEncryptedAppTicketListener, GameServerListenerRegistrar> GameServerGlobalEncryptedAppTicketListener;


		/**
		 * Listener for the event of a change of current access token.
		 */
		class IAccessTokenListener : public GalaxyTypeAwareListener<ACCESS_TOKEN_CHANGE>
		{
		public:

			/**
			 * Notification for an event of retrieving an access token.
			 *
			 * In order to read the access token, call IUser::GetAccessToken().
			 */
			virtual void OnAccessTokenChanged() = 0;
		};

		/**
		 * Globally self-registering version of IAccessTokenListener.
		 */
		typedef SelfRegisteringListener<IAccessTokenListener> GlobalAccessTokenListener;

		/**
		 * Globally self-registering version of IAccessTokenListener for the GameServer.
		 */
		typedef SelfRegisteringListener<IAccessTokenListener, GameServerListenerRegistrar> GameServerGlobalAccessTokenListener;

		/**
		 * The interface for handling the user account.
		 */
		class IUser_1_152_1
		{
		public:

			virtual ~IUser_1_152_1()
			{
			}

			/**
			 * Checks if the user is signed in to Galaxy.
			 *
			 * The user should be reported as signed in as soon as the authentication
			 * process is finished.
			 *
			 * If the user is not able to sign in or gets signed out, there might be either
			 * a networking issue or a limited availability of Galaxy backend services.
			 *
			 * After loosing authentication the user needs to sign in again in order
			 * for the Galaxy Peer to operate.
			 *
			 * @remark Information about being signed in or signed out also comes to
			 * the IAuthListener and the IOperationalStateChangeListener.
			 *
			 * @return true if the user is signed in, false otherwise.
			 */
			virtual bool SignedIn() = 0;

			/**
			 * Returns the ID of the user, provided that the user is signed in.
			 *
			 * @return The ID of the user.
			 */
			virtual GalaxyID GetGalaxyID() = 0;

			/**
			 * Authenticates the Galaxy Peer with specified user credentials.
			 *
			 * This call is asynchronous. Responses come to the IAuthListener
			 * (for all GlobalAuthListener-derived and optional listener passed as argument).
			 *
			 * @warning This method is only for testing purposes and is not meant
			 * to be used in production environment in any way.
			 *
			 * @remark Information about being signed in or signed out also comes to
			 * the IOperationalStateChangeListener.
			 *
			 * @param [in] login The user's login.
			 * @param [in] password The user's password.
			 * @param [in] listener The listener for specific operation.
			 */
			virtual void SignInCredentials(const char* login, const char* password, IAuthListener* const listener = NULL) = 0;

			/**
			 * Authenticates the Galaxy Peer with refresh token.
			 *
			 * This call is asynchronous. Responses come to the IAuthListener
			 * (for all GlobalAuthListener-derived and optional listener passed as argument).
			 *
			 * This method is designed for application which renders Galaxy login page
			 * in its UI and obtains refresh token after user login.
			 *
			 * @remark Information about being signed in or signed out also comes to
			 * the IOperationalStateChangeListener.
			 *
			 * @param [in] refreshToken The refresh token obtained from Galaxy login page.
			 * @param [in] listener The listener for specific operation.
			 */
			virtual void SignInToken(const char* refreshToken, IAuthListener* const listener = NULL) = 0;

			/**
			 * Authenticates the Galaxy Peer based on launcher authentication.
			 *
			 * This call is asynchronous. Responses come to the IAuthListener
			 * (for all GlobalAuthListener-derived and optional listener passed as argument).
			 *
			 * @remark Information about being signed in or signed out also comes to
			 * the IOperationalStateChangeListener.
			 *
			 * @note This method is for internal use only.
			 *
			 * @param [in] listener The listener for specific operation.
			 */
			virtual void SignInLauncher(IAuthListener* const listener = NULL) = 0;

			/**
			 * Authenticates the Galaxy Peer based on Steam Encrypted App Ticket.
			 *
			 * This call is asynchronous. Responses come to the IAuthListener
			 * (for all GlobalAuthListener-derived and optional listener passed as argument).
			 *
			 * @remark Information about being signed in or signed out also comes to
			 * the IOperationalStateChangeListener.
			 *
			 * @param [in] steamAppTicket The Encrypted App Ticket from the Steam API.
			 * @param [in] steamAppTicketSize The size of the ticket.
			 * @param [in] personaName The user's persona name, i.e. the username from Steam.
			 * @param [in] listener The listener for specific operation.
			 */
			virtual void SignInSteam(const void* steamAppTicket, uint32_t steamAppTicketSize, const char* personaName, IAuthListener* const listener = NULL) = 0;

			/**
			 * Authenticates the Galaxy Peer based on Galaxy Client authentication.
			 *
			 * This call is asynchronous. Responses come to the IAuthListener
			 * (for all GlobalAuthListener-derived and optional listener passed as argument).
			 *
			 * @remark Information about being signed in or signed out also comes to
			 * the IOperationalStateChangeListener.
			 *
			 * @param [in] requireOnline Indicates if sign in with Galaxy backend is required.
			 * @param [in] listener The listener for specific operation.
			 */
			virtual void SignInGalaxy(bool requireOnline = false, IAuthListener* const listener = NULL) = 0;

			/**
			 * Authenticates the Galaxy Peer based on PS4 credentials.
			 *
			 * This call is asynchronous. Responses come to the IAuthListener
			 * (for all GlobalAuthListener-derived and optional listener passed as argument).
			 *
			 * @remark Information about being signed in or signed out also comes to
			 * the IOperationalStateChangeListener.
			 *
			 * @param [in] ps4ClientID The PlayStation 4 client ID.
			 * @param [in] listener The listener for specific operation.
			 */
			virtual void SignInPS4(const char* ps4ClientID, IAuthListener* const listener = NULL) = 0;

			/**
			 * Authenticates the Galaxy Peer based on XBOX ONE credentials.
			 *
			 * This call is asynchronous. Responses come to the IAuthListener
			 * (for all GlobalAuthListener-derived and optional listener passed as argument).
			 *
			 * @remark Information about being signed in or signed out also comes to
			 * the IOperationalStateChangeListener.
			 *
			 * @param [in] xboxOneUserID The XBOX ONE user ID.
			 * @param [in] listener The listener for specific operation.
			 */
			virtual void SignInXB1(const char* xboxOneUserID, IAuthListener* const listener = NULL) = 0;

			/**
			 * Authenticates the Galaxy Peer based on XBOX GDK credentials.
			 *
			 * This call is asynchronous. Responses come to the IAuthListener
			 * (for all GlobalAuthListener-derived and optional listener passed as argument).
			 *
			 * @remark Information about being signed in or signed out also comes to
			 * the IOperationalStateChangeListener.
			 *
			 * @param [in] xboxID The XBOX user ID.
			 * @param [in] listener The listener for specific operation.
			 */
			virtual void SignInXbox(uint64_t xboxID, IAuthListener* const listener = NULL) = 0;

			/**
			 * Authenticates the Galaxy Peer based on Xbox Live tokens.
			 *
			 * This call is asynchronous. Responses come to the IAuthListener
			 * (for all GlobalAuthListener-derived and optional listener passed as argument).
			 *
			 * @remark Information about being signed in or signed out also comes to
			 * the IOperationalStateChangeListener.
			 *
			 * @param [in] token The XSTS token.
			 * @param [in] signature The digital signature for the HTTP request.
			 * @param [in] marketplaceID The Marketplace ID
			 * @param [in] locale The user locale (example values: EN-US, FR-FR).
			 * @param [in] listener The listener for specific operation.
			 */
			virtual void SignInXBLive(const char* token, const char* signature, const char* marketplaceID, const char* locale, IAuthListener* const listener = NULL) = 0;

			/**
			 * Authenticates the Galaxy Game Server anonymously.
			 *
			 * This call is asynchronous. Responses come to the IAuthListener
			 * (for all GlobalAuthListener-derived and optional listener passed as argument).
			 *
			 * @remark Information about being signed in or signed out also comes to
			 * the IOperationalStateChangeListener.
			 *
			 * @param [in] listener The listener for specific operation.
			 */
			virtual void SignInAnonymous(IAuthListener* const listener = NULL) = 0;

			/**
			 * Authenticates the Galaxy Peer anonymously.
			 *
			 * This authentication method enables the peer to send anonymous telemetry events.
			 *
			 * This call is asynchronous. Responses come to the IAuthListener
			 * (for all GlobalAuthListener-derived and optional listener passed as argument).
			 *
			 * @param [in] listener The listener for specific operation.
			 */
			virtual void SignInAnonymousTelemetry(IAuthListener* const listener = NULL) = 0;

			/**
			 * Authenticates the Galaxy Peer with a specified server key.
			 *
			 * This call is asynchronous. Responses come to the IAuthListener.
			 *
			 * @warning Make sure you do not put your server key in public builds.
			 *
			 * @remark Information about being signed in or signed out also comes to
			 * the IOperationalStateChangeListener.
			 *
			 * @remark Signing in with a server key is meant for server-side usage,
			 * meaning that typically there will be no user associated to the session.
			 *
			 * @param [in] serverKey The server key.
			 * @param [in] listener The listener for specific operation.
			 */
			virtual void SignInServerKey(const char* serverKey, IAuthListener* const listener = NULL) = 0;

			/**
			 * Authenticates the Galaxy Peer based on OpenID Connect generated authorization code.
			 *
			 * This call is asynchronous. Responses come to the IAuthListener
			 * (for all GlobalAuthListener-derived and optional listener passed as argument).
			 *
			 * @remark Information about being signed in or signed out also comes to
			 * the IOperationalStateChangeListener.
			 *
			 * @param [in] requireOnline Indicates if sign in with Galaxy backend is required.
			 * @param [in] listener The listener for specific operation.
			 */
			virtual void SignInAuthorizationCode(const char* authorizationCode, const char* redirectURI, IAuthListener* const listener = NULL) = 0;

			/**
			 * Signs the Galaxy Peer out.
			 *
			 * This call is asynchronous. Responses come to the IAuthListener and IOperationalStateChangeListener.
			 *
			 * @remark All pending asynchronous operations will be finished immediately.
			 * Their listeners will be called with the next call to the ProcessData().
			 */
			virtual void SignOut() = 0;

			/**
			 * Retrieves/Refreshes user data storage.
			 *
			 * This call is asynchronous. Responses come to the IUserDataListener and ISpecificUserDataListener.
			 *
			 * @param [in] userID The ID of the user. It can be omitted when requesting for own data.
			 * @param [in] listener The listener for specific operation.
			 */
			virtual void RequestUserData(GalaxyID userID = GalaxyID(), ISpecificUserDataListener* const listener = NULL) = 0;

			/**
			 * Checks if user data exists.
			 *
			 * @pre Retrieve the user data first by calling RequestUserData().
			 *
			 * @param [in] userID The ID of the user. It can be omitted when checking for own data.
			 * @return true if user data exists, false otherwise.
			 */
			virtual bool IsUserDataAvailable(GalaxyID userID = GalaxyID()) = 0;

			/**
			 * Returns an entry from the data storage of current user.
			 *
			 * @remark This call is not thread-safe as opposed to GetUserDataCopy().
			 *
			 * @pre Retrieve the user data first by calling RequestUserData().
			 *
			 * @param [in] key The name of the property of the user data storage.
			 * @param [in] userID The ID of the user. It can be omitted when reading own data.
			 * @return The value of the property, or an empty string if failed.
			 */
			virtual const char* GetUserData(const char* key, GalaxyID userID = GalaxyID()) = 0;

			/**
			 * Copies an entry from the data storage of current user.
			 *
			 * @pre Retrieve the user data first by calling RequestUserData().
			 *
			 * @param [in] key The name of the property of the user data storage.
			 * @param [in, out] buffer The output buffer.
			 * @param [in] bufferLength The size of the output buffer.
			 * @param [in] userID The ID of the user. It can be omitted when reading own data.
			 */
			virtual void GetUserDataCopy(const char* key, char* buffer, uint32_t bufferLength, GalaxyID userID = GalaxyID()) = 0;

			/**
			 * Creates or updates an entry in the user data storage.
			 *
			 * This call in asynchronous. Responses come to the IUserDataListener and ISpecificUserDataListener.
			 *
			 * @remark To clear a property, set it to an empty string.
			 *
			 * @param [in] key The name of the property of the user data storage with the limit of 1023 bytes.
			 * @param [in] value The value of the property to set with the limit of 4095 bytes.
			 * @param [in] listener The listener for specific operation.
			 */
			virtual void SetUserData(const char* key, const char* value, ISpecificUserDataListener* const listener = NULL) = 0;

			/**
			 * Returns the number of entries in the user data storage
			 *
			 * @pre Retrieve the user data first by calling RequestUserData().
			 *
			 * @param [in] userID The ID of the user. It can be omitted when reading own data.
			 * @return The number of entries, or 0 if failed.
			 */
			virtual uint32_t GetUserDataCount(GalaxyID userID = GalaxyID()) = 0;

			/**
			 * Returns a property from the user data storage by index.
			 *
			 * @pre Retrieve the user data first by calling RequestUserData().
			 *
			 * @param [in] index Index as an integer in the range of [0, number of entries).
			 * @param [in, out] key The name of the property of the user data storage.
			 * @param [in] keyLength The length of the name of the property of the user data storage.
			 * @param [in, out] value The value of the property of the user data storage.
			 * @param [in] valueLength The length of the value of the property of the user data storage.
			 * @param [in] userID The ID of the user. It can be omitted when reading own data.
			 * @return true if succeeded, false when there is no such property.
			 */
			virtual bool GetUserDataByIndex(uint32_t index, char* key, uint32_t keyLength, char* value, uint32_t valueLength, GalaxyID userID = GalaxyID()) = 0;

			/**
			 * Clears a property of user data storage
			 *
			 * This is the same as calling SetUserData() and providing an empty string
			 * as the value of the property that is to be altered.
			 *
			 * This call in asynchronous. Responses come to the IUserDataListener and ISpecificUserDataListener.
			 *
			 * @param [in] key The name of the property of the user data storage.
			 * @param [in] listener The listener for specific operation.
			 */
			virtual void DeleteUserData(const char* key, ISpecificUserDataListener* const listener = NULL) = 0;

			/**
			 * Checks if the user is logged on to Galaxy backend services.
			 *
			 * @remark Only a user that has been successfully signed in within Galaxy Service
			 * can be logged on to Galaxy backend services, hence a user that is logged on
			 * is also signed in, and a user that is not signed in is also not logged on.
			 *
			 * @remark Information about being logged on or logged off also comes to
			 * the IOperationalStateChangeListener.
			 *
			 * @return true if the user is logged on to Galaxy backend services, false otherwise.
			 */
			virtual bool IsLoggedOn() = 0;

			/**
			 * Performs a request for an Encrypted App Ticket.
			 *
			 * This call is asynchronous. Responses come to the IEncryptedAppTicketListener.
			 *
			 * @param [in] data The additional data to be placed within the Encrypted App Ticket with the limit of 1023 bytes.
			 * @param [in] dataSize The size of the additional data.
			 * @param [in] listener The listener for specific operation.
			 */
			virtual void RequestEncryptedAppTicket(const void* data, uint32_t dataSize, IEncryptedAppTicketListener* const listener = NULL) = 0;

			/**
			 * Returns the Encrypted App Ticket.
			 *
			 * If the buffer that is supposed to take the data is too small,
			 * the Encrypted App Ticket will be truncated to its size.
			 *
			 * @pre Retrieve an Encrypted App Ticket first by calling RequestEncryptedAppTicket().
			 *
			 * @param [in, out] encryptedAppTicket The buffer for the Encrypted App Ticket.
			 * @param [in] maxEncryptedAppTicketSize The maximum size of the Encrypted App Ticket buffer.
			 * @param [out] currentEncryptedAppTicketSize The actual size of the Encrypted App Ticket.
			 */
			virtual void GetEncryptedAppTicket(void* encryptedAppTicket, uint32_t maxEncryptedAppTicketSize, uint32_t& currentEncryptedAppTicketSize) = 0;

			/**
			 * Returns the ID of current session.
			 *
			 * @return The session ID.
			 */
			virtual SessionID GetSessionID() = 0;

			/**
			 * Returns the access token for current session.
			 *
			 * @remark This call is not thread-safe as opposed to GetAccessTokenCopy().
			 *
			 * The access token that is used for the current session might be
			 * updated in the background automatically, without any request for that.
			 * Each time the access token is updated, a notification comes to the
			 * IAccessTokenListener.
			 *
			 * @return The access token.
			 */
			virtual const char* GetAccessToken() = 0;

			/**
			 * Copies the access token for current session.
			 *
			 * The access token that is used for the current session might be
			 * updated in the background automatically, without any request for that.
			 * Each time the access token is updated, a notification comes to the
			 * IAccessTokenListener.
			 *
			 * @param [in, out] buffer The output buffer.
			 * @param [in] bufferLength The size of the output buffer.
			 */
			virtual void GetAccessTokenCopy(char* buffer, uint32_t bufferLength) = 0;

			/**
			 * Returns the refresh token for the current session.
			 *
			 * @remark This call is not thread-safe as opposed to GetRefreshTokenCopy().
			 * 
			 * @remark Calling this function is allowed when the session has been signed in
			 * with IUser::SignInToken() only.
			 *
			 * The refresh token that is used for the current session might be
			 * updated in the background automatically, together with the access token, 
			 * without any request for that. Each time the access or refresh token 
			 * is updated, a notification comes to the IAccessTokenListener.
			 *
			 * @return The refresh token.
			 */
			virtual const char* GetRefreshToken() = 0;

			/**
			 * Copies the refresh token for the current session.
			 *
			 * @remark Calling this function is allowed when the session has been signed in
			 * with IUser::SignInToken() only.
			 *
			 * The refresh token that is used for the current session might be
			 * updated in the background automatically, together with access token,
			 * without any request for that. Each time the access or refresh token
			 * is updated, a notification comes to the IAccessTokenListener.
			 *
			 * @param [in, out] buffer The output buffer.
			 * @param [in] bufferLength The size of the output buffer.
			 */
			virtual void GetRefreshTokenCopy(char* buffer, uint32_t bufferLength) = 0;

			/**
			 * Returns the id token for the current session.
			 *
			 * @remark This call is not thread-safe as opposed to GetIDTokenCopy().
			 * 
			 * @remark Calling this function is allowed when the session has been signed in
			 * with IUser::SignInToken() only.
			 *
			 * The id token that is used for the current session might be
			 * updated in the background automatically, together with the access token, 
			 * without any request for that. Each time the access or id token 
			 * is updated, a notification comes to the IAccessTokenListener.
			 *
			 * @return The id token.
			 */
			virtual const char* GetIDToken() = 0;

			/**
			 * Copies the id token for the current session.
			 *
			 * @remark Calling this function is allowed when the session has been signed in
			 * with IUser::SignInToken() only.
			 *
			 * The id token that is used for the current session might be
			 * updated in the background automatically, together with access token,
			 * without any request for that. Each time the access or id token
			 * is updated, a notification comes to the IAccessTokenListener.
			 *
			 * @param [in, out] buffer The output buffer.
			 * @param [in] bufferLength The size of the output buffer.
			 */
			virtual void GetIDTokenCopy(char* buffer, uint32_t bufferLength) = 0;

			/**
			 * Reports current access token as no longer working.
			 *
			 * This starts the process of refreshing access token, unless the process
			 * is already in progress. The notifications come to IAccessTokenListener.
			 *
			 * @remark The access token that is used for current session might be
			 * updated in the background automatically, without any request for that.
			 * Each time the access token is updated, a notification comes to the
			 * IAccessTokenListener.
			 *
			 * @remark If the specified access token is not the same as the access token
			 * for current session that would have been returned by GetAccessToken(),
			 * the report will not be accepted and no further action will be performed.
			 * In such case do not expect a notification to IAccessTokenListener and
			 * simply get the new access token by calling GetAccessToken().
			 *
			 * @param [in] accessToken The invalid access token.
			 * @param [in] info Additional information, e.g. the URI of the resource it was used for.
			 * @return true if the report was accepted, false otherwise.
			 */
			virtual bool ReportInvalidAccessToken(const char* accessToken, const char* info = NULL) = 0;
		};

		/** @} */
	}
}

#include <galaxy/IStats.h>
#include <galaxy/1_152_11/IStats.h>
#include <galaxy/1_152_1/IStats.h>

namespace galaxy
{
  namespace api
  {
    using ProcessData_pfn         = void (__cdecl *)(void);
    using Init_pfn                = void (__cdecl *)(struct galaxy::api::InitOptions const&);
    using Shutdown_pfn            = void (__cdecl *)(void);
    using CreateInstance_pfn      = galaxy::api::IGalaxy* (__cdecl *)(void);
    using ProcessData_IGalaxy_pfn = void (__cdecl *)(IGalaxy* This);
    using Shutdown_IGalaxy_pfn    = void (__cdecl *)(IGalaxy* This);

    static Init_pfn                Init_Original                = nullptr;
    static Shutdown_pfn            Shutdown_Original            = nullptr;
    static ProcessData_pfn         ProcessData_Original         = nullptr;
    static CreateInstance_pfn      CreateInstance_Original      = nullptr;
    static ProcessData_IGalaxy_pfn ProcessData_IGalaxy_Original = nullptr;
    static Shutdown_IGalaxy_pfn    Shutdown_IGalaxy_Original    = nullptr;

    static volatile LONGLONG ticks = 0;

    void
    ProcessDataHook_Impl (IGalaxy* This = nullptr)
    {
      SK_LOG_FIRST_CALL

      if (This == nullptr && gog->Stats () == nullptr)
      {
        const auto Stats     = (IStats*             (__cdecl *)(void))
          SK_GetProcAddress (gog->GetGalaxyDLL (), "?Stats@api@galaxy@@YAPEAVIStats@12@XZ");
        const auto Utils     = (IUtils*             (__cdecl *)(void))
          SK_GetProcAddress (gog->GetGalaxyDLL (), "?Utils@api@galaxy@@YAPEAVIUtils@12@XZ");
        const auto User      = (IUser*              (__cdecl *)(void))
          SK_GetProcAddress (gog->GetGalaxyDLL (), "?User@api@galaxy@@YAPEAVIUser@12@XZ");
        const auto Registrar = (IListenerRegistrar* (__cdecl *)(void))
          SK_GetProcAddress (gog->GetGalaxyDLL (), "?ListenerRegistrar@api@galaxy@@YAPEAVIListenerRegistrar@12@XZ");

        IStats*             pStats     = Stats     != nullptr ? Stats     () : nullptr;
        IUtils*             pUtils     = Utils     != nullptr ? Utils     () : nullptr;
        IUser*              pUser      = User      != nullptr ? User      () : nullptr;
        IListenerRegistrar* pRegistrar = Registrar != nullptr ? Registrar () : nullptr;

        gog->Init (pStats, pUtils, pUser, pRegistrar);
      }

      else if (This != nullptr && gog->Stats () == nullptr)
      {
        IStats*             pStats     = This->GetStats             ();
        IUtils*             pUtils     = This->GetUtils             ();
        IUser*              pUser      = This->GetUser              ();
        IListenerRegistrar* pRegistrar = This->GetListenerRegistrar ();

        gog->Init (pStats, pUtils, pUser, pRegistrar);
      }

      auto stats = gog->Stats ();

      IListenerRegistrar* registrar =
        gog->Registrar ();

      class SK_IUserTimePlayedRetrieveListener : public IUserTimePlayedRetrieveListener
      {
      public:
        void OnUserTimePlayedRetrieveSuccess (GalaxyID userID) final
        {
          std::ignore = userID;

          auto stats = gog->Stats ();

          uint32_t playing_time = SK_Galaxy_Stats_GetUserTimePlayed (stats);
          if (playing_time != 0)
          {
            gog_log->Log (
              L"User has played this game for %5.2f Hours", (float)playing_time / 60.0f
            );
          }
        }

        void OnUserTimePlayedRetrieveFailure (GalaxyID userID, FailureReason failureReason) final
        {
          gog_log->Log (L"RequestUserTimePlayed Failed=%x", failureReason);

          std::ignore = userID;
          std::ignore = failureReason;
        }
      } static time_played;

      class SK_IAchievementChangeListener : public IAchievementChangeListener
      {
      public:
        virtual void OnAchievementUnlocked (const char* name) final
        {
          auto pAchievement =
            galaxy_achievements->getAchievement (name);

          if (pAchievement != nullptr)
          {
            // This callback gets sent for achievements that are already unlocked...
            if (! pAchievement->unlocked_)
            {
              gog_log->Log ( L" Achievement: '%hs' (%hs) - Unlocked!",
                               pAchievement->text_.unlocked.human_name.c_str (),
                               pAchievement->text_.unlocked.desc      .c_str () );

            //SK_Galaxy_Achievements_RefreshPlayerStats ();

              galaxy_achievements->unlock (name);
            }
          }
        }
      } static achievement_change;

      class SK_IUserStatsAndAchievementsRetrieveListener : public IUserStatsAndAchievementsRetrieveListener
      {
      public:
        virtual void OnUserStatsAndAchievementsRetrieveSuccess (GalaxyID userID) final
        {
          if (userID != ((IUser_1_152_1 *)gog->User ())->GetGalaxyID ())
            return;

          static int unlock_count = 0;

          SK_RunOnce (
          auto stats = gog->Stats ();

          static const auto
            achievements_path =
              std::filesystem::path (
                   SK_GetConfigPath () )
            / LR"(SK_Res/Achievements)";

          static auto const global_stats_filename =
            achievements_path / LR"(GlobalStatsForGame.json)";

          static FILE*   fGlobalStats  = nullptr;
          fGlobalStats = fGlobalStats != nullptr ? fGlobalStats :
                _wfopen (global_stats_filename.c_str (), L"rb+");

          if (fGlobalStats != nullptr)
          {
            static bool loaded = false;

            try
            {
              static std::vector <BYTE> data;
        
                           fseek (fGlobalStats, 0, SEEK_END);
              data.resize (ftell (fGlobalStats));
                          rewind (fGlobalStats);

              if (! data.empty ())
              {
                static nlohmann::json jsonStats;

                if (! loaded)
                {
                  if (0 != fread (data.data (), data.size (), 1, fGlobalStats))
                  {
                    jsonStats =
                      std::move (
                        nlohmann::json::parse ( data.cbegin (),
                                                data.cend   (), nullptr, true )
                      );

                    loaded = true;
                  }
                }

                if ( jsonStats.contains ("achievementpercentages") &&
                     jsonStats          ["achievementpercentages"].contains ("achievements") )
                {
                  const auto& achievements_ =
                    jsonStats ["achievementpercentages"]["achievements"];

                  int idx = 0;

                  for ( const auto& achievement : achievements_ )
                  {
                    auto galaxy_achievement =
                      new SK_AchievementManager::Achievement (
                        idx++, achievement ["name"].get <std::string_view> ().data (), (galaxy::api::IStats *)stats
                      );

                    galaxy_achievement->global_percent_ = static_cast <float> (
                      atof (achievement ["percent"].get <std::string_view> ().data ())
                    );

                    galaxy_achievements->possible++;
                    galaxy_achievements->addAchievement (galaxy_achievement);

                    if (galaxy_achievement->unlocked_)
                      unlock_count++;
                  }
                }
              }
            }

            catch (const std::exception& e)
            {
              std::ignore = e;

              loaded = false;

              fclose (fGlobalStats);
                      fGlobalStats = nullptr;
            }
          });

          galaxy_achievements->log_all_achievements ();

          if (! std::exchange (has_unlock_callback, true))
          {
            galaxy_achievements->loadSound (config.platform.achievements.sound_file.c_str ());

            gog->Registrar ()->Register (achievement_change.GetListenerType (), &achievement_change);
          }

          galaxy_achievements->total_unlocked   = unlock_count;
          galaxy_achievements->percent_unlocked =
            static_cast <float> (
              static_cast <double> (galaxy_achievements->total_unlocked) /
              static_cast <double> (SK_Galaxy_GetNumPossibleAchievements ())
            );
        }

        virtual void OnUserStatsAndAchievementsRetrieveFailure (GalaxyID userID, FailureReason failureReason) final
        {
          std::ignore = userID;
          std::ignore = failureReason;
        }
      } static stats_and_achievements;

      SK_RunOnce (
      //registrar->Register (           time_played.GetListenerType (), &time_played);
        registrar->Register (stats_and_achievements.GetListenerType (), &stats_and_achievements);

        std::ignore = stats;

         IUser_1_152_1* user =
        (IUser_1_152_1*)gog->User ();

       //stats->RequestUserTimePlayed           (user->GetGalaxyID ());
         SK_Galaxy_Stats_RequestUserStatsAndAchievements (stats, user->GetGalaxyID ());
      );
    }

    void
    __cdecl
    ProcessData_Detour (void)
    {
      InterlockedIncrement64 (&ticks);

      static bool
            crashed = false;
      if (! crashed)
      {
        __try {
          ProcessDataHook_Impl ();
        }

        __except (EXCEPTION_EXECUTE_HANDLER)
        {
          crashed = true;
        }
      }

      // The game's callback handlers might not be expecting to be invoked,
      //   and while that's a bug in the game itself, we can't let it crash.
      __try {
        ProcessData_Original ();
      }

      __except (EXCEPTION_EXECUTE_HANDLER)
      {
        gog_log->Log (
          L"Structured Exception encountered during ProcessData (...)"
        );
      }
    }

    void
    __cdecl
    ProcessData_IGalaxy_Detour (IGalaxy* This)
    {
      InterlockedIncrement64 (&ticks);

      static bool
            crashed = false;
      if (! crashed)
      {
        __try {
          ProcessDataHook_Impl ();
        }

        __except (EXCEPTION_EXECUTE_HANDLER)
        {
          crashed = true;
        }
      }

      // The game's callback handlers might not be expecting to be invoked,
      //   and while that's a bug in the game itself, we can't let it crash.
      __try {
        ProcessData_IGalaxy_Original (This);
      }

      __except (EXCEPTION_EXECUTE_HANDLER)
      {
        gog_log->Log (
          L"Structured Exception encountered during ProcessData (...)"
        );
      }
    }

    void
    __cdecl
    Shutdown_Detour (void)
    {
      SK_LOG_FIRST_CALL

      gog->Shutdown (true);

      Shutdown_Original ();
    }

    void
    __cdecl
    Shutdown_IGalaxy_Detour (IGalaxy* This)
    {
      SK_LOG_FIRST_CALL

      gog->Shutdown (true);

      Shutdown_IGalaxy_Original (This);
    }

    galaxy::api::IGalaxy*
    __cdecl
    CreateInstance_Detour (void)
    {
      SK_LOG_FIRST_CALL

      auto instance =
        CreateInstance_Original ();

      if (instance != nullptr)
      {
        //IStats*             pStats     = instance->GetStats             ();
        //IUtils*             pUtils     = instance->GetUtils             ();
        //IUser*              pUser      = instance->GetUser              ();
        //IListenerRegistrar* pRegistrar = instance->GetListenerRegistrar ();

        //gog->Init (pStats, pUtils, pUser, pRegistrar);

        GALAXY_VIRTUAL_HOOK ( &instance,      3,
                            "IGalaxy::Shutdown",
                             Shutdown_IGalaxy_Detour,
                             Shutdown_IGalaxy_Original,
                             Shutdown_IGalaxy_pfn );

        GALAXY_VIRTUAL_HOOK ( &instance,     16,
                            "IGalaxy::ProcessData",
                             ProcessData_IGalaxy_Detour,
                             ProcessData_IGalaxy_Original,
                             ProcessData_IGalaxy_pfn );

        bool bEnable = SK_EnableApplyQueuedHooks ();
                             SK_ApplyQueuedHooks ();
        if (!bEnable) SK_DisableApplyQueuedHooks ();
      }

      return
        instance;
    }

    void
    __cdecl
    Init_Detour (struct galaxy::api::InitOptions const& initOptions)
    {
      SK_LOG_FIRST_CALL

      const char* clientID     = initOptions.clientID;
      const char* clientSecret = initOptions.clientSecret;

      InitOptions_1_152_1& initOptions_Versioned =
        (InitOptions_1_152_1 &)initOptions;

      clientID     = initOptions_Versioned.clientID;
      clientSecret = initOptions_Versioned.clientSecret;

      gog_log->Log (L"clientID.....: %hs", clientID);
      gog_log->Log (L"clientSecret.: %hs", clientSecret);

      Init_Original (initOptions);

      const auto Stats     = (IStats*             (__cdecl *)(void))
        SK_GetProcAddress (gog->GetGalaxyDLL (), "?Stats@api@galaxy@@YAPEAVIStats@12@XZ"); // 64-bit
      const auto Utils     = (IUtils*             (__cdecl *)(void))
        SK_GetProcAddress (gog->GetGalaxyDLL (), "?Utils@api@galaxy@@YAPEAVIUtils@12@XZ"); // 64-bit
      const auto User      = (IUser*              (__cdecl *)(void))
        SK_GetProcAddress (gog->GetGalaxyDLL (), "?User@api@galaxy@@YAPEAVIUser@12@XZ"); // 64-bit
      const auto Registrar = (IListenerRegistrar* (__cdecl *)(void))
        SK_GetProcAddress (gog->GetGalaxyDLL (), "?ListenerRegistrar@api@galaxy@@YAPEAVIListenerRegistrar@12@XZ"); // 64-bit

      IStats*             pStats     = Stats     != nullptr ? Stats     () : nullptr;
      IUtils*             pUtils     = Utils     != nullptr ? Utils     () : nullptr;
      IUser*              pUser      = User      != nullptr ? User      () : nullptr;
      IListenerRegistrar* pRegistrar = Registrar != nullptr ? Registrar () : nullptr;

      gog->Init (pStats, pUtils, pUser, pRegistrar);
    }
  }
}

void
SK::Galaxy::Init (void)
{
  if (config.platform.silent)
    return;

  int             gog_gameId =  0;
  WIN32_FIND_DATA fd         = {};

  auto _FindGameInfo = [&](const wchar_t* wszPattern)
  {
    HANDLE hFind =
      FindFirstFileW (wszPattern, &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
      do
      {
        if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
        {
          if (1 == swscanf (fd.cFileName, L"goggame-%d.info", &gog_gameId))
          {
            gog_log->Log (
              L"GOG gameId: %d", gog_gameId
            );

            if (gog_gameId != 0)
            {
              config.platform.type = SK_Platform_GOG;
            }

            if (config.platform.equivalent_steam_app == -1)
            {
              std::wstring url =
                SK_FormatStringW (
                  LR"(https://www.pcgamingwiki.com/w/api.php?action=cargoquery&format=json&tables=Infobox_game&fields=Steam_AppID&where=GOGcom_ID+HOLDS+%%27%d%%27)",
                                                                                                                                       gog_gameId
                );

              SK_Network_EnqueueDownload (
                sk_download_request_s (L"pcgw_appid_map.json", url.data (),
                  []( const std::vector <uint8_t>&& data,
                      const std::wstring_view       file )
                  {
                    if (data.empty ())
                      return true;

                    std::ignore = file;

                    try {
                      gog_log->Log (L"PCGW Queried Data: %hs", data.data ());

                      nlohmann::json json =
                        std::move (
                          nlohmann::json::parse ( data.cbegin (),
                                                  data.cend   (), nullptr, true )
                        );

                      for ( auto& query : json ["cargoquery"] )
                      {
                        if ( query.contains ("title") &&
                             query.at       ("title").contains ("Steam AppID") )
                        {
                          config.platform.equivalent_steam_app =
                            atoi (
                              query.at ("title").
                                    at ("Steam AppID").
                                    get <std::string_view> ().
                                                      data () );
                          break;
                        }
                      }

                      gog_log->Log (L"Steam AppID: %d", config.platform.equivalent_steam_app);

                      if (config.platform.equivalent_steam_app != -1)
                      {   config.utility.save_async ();
                      }

                      else
                      {
                        config.platform.equivalent_steam_app = 0;
                      }
                    }

                    catch (const std::exception& e)
                    {
                      if (config.system.log_level > 0)
                      {
#ifdef __CPP20
                        const auto&& src_loc =
                          std::source_location::current ();

                        steam_log->Log ( L"%hs (%d;%d): json parse failure: %hs",
                                                     src_loc.file_name     (),
                                                     src_loc.line          (),
                                                     src_loc.column        (), e.what ());
                        steam_log->Log (L"%hs",      src_loc.function_name ());
                        steam_log->Log (L"%hs",
                          std::format ( std::string ("{:*>") +
                                     std::to_string (src_loc.column        ()), 'x').c_str ());
#else
                        std::ignore = e;
#endif
                      }
                    }

                    return true;
                  }
                )
              );
            }

            break;
          }
        }
      } while (FindNextFileW (hFind, &fd) != 0);

      FindClose (hFind);
    }
  };

  if (config.platform.equivalent_steam_app == -1)
  {
    // Stupid way of dealing with Unreal Engine games
                                   _FindGameInfo (               L"goggame-*.info");
    if         (gog_gameId == 0) { _FindGameInfo (            L"../goggame-*.info");
      if       (gog_gameId == 0) { _FindGameInfo (         L"../../goggame-*.info");
        if     (gog_gameId == 0) { _FindGameInfo (      L"../../../goggame-*.info");
          if   (gog_gameId == 0) { _FindGameInfo (   L"../../../../goggame-*.info");
            if (gog_gameId == 0) { _FindGameInfo (L"../../../../../goggame-*.info"); } } } } }

    if (gog_gameId != 0)
    {
      config.platform.type = SK_Platform_GOG;
    }
  }

  const wchar_t*
    wszGalaxyDLLName =
      SK_RunLHIfBitness ( 64, L"Galaxy64.dll",
                                L"Galaxy.dll" );

  static HMODULE     hModGalaxy = nullptr;
  if (std::exchange (hModGalaxy, SK_LoadLibraryW (wszGalaxyDLLName)) == nullptr && hModGalaxy != nullptr)
  {
    gog_log->init (L"logs/galaxy.log", L"wt+,ccs=UTF-8");
    gog_log->silent = config.platform.silent;

    gog_log->Log (L"Galaxy DLL: %p", SK_LoadLibraryW (wszGalaxyDLLName));
    gog->PreInit (hModGalaxy);

    std::wstring ver_str =
      SK_GetDLLVersionShort (wszGalaxyDLLName);

    gog_log->Log (L"Galaxy Ver: %ws", ver_str.c_str ());

    int major, minor, build, rev;
    swscanf (ver_str.c_str (), L"%d.%d.%d.%d", &major, &minor, &build, &rev);

    if ((major == 1 && (minor > 152 || (minor == 152 && build >= 10))) || major > 1)
      gog->version = SK_GalaxyContext::Version_1_152_10;

#ifdef _M_AMD64
    SK_CreateDLLHook2 (     wszGalaxyDLLName, "?Init@api@galaxy@@YAXAEBUInitOptions@12@@Z",
                               galaxy::api::Init_Detour,
      static_cast_p2p <void> (&galaxy::api::Init_Original) );
    SK_CreateDLLHook2 (     wszGalaxyDLLName, "?ProcessData@api@galaxy@@YAXXZ",
                               galaxy::api::ProcessData_Detour,
      static_cast_p2p <void> (&galaxy::api::ProcessData_Original) );
    SK_CreateDLLHook2 (     wszGalaxyDLLName, "?Shutdown@api@galaxy@@YAXXZ",
                               galaxy::api::Shutdown_Detour,
      static_cast_p2p <void> (&galaxy::api::Shutdown_Original) );

    bool bEnable = SK_EnableApplyQueuedHooks ();
                         SK_ApplyQueuedHooks ();
    if (!bEnable) SK_DisableApplyQueuedHooks ();
#else
    //SK_CreateDLLHook2 (     wszGalaxyDLLName, "?Init@api@galaxy@@YAXAEBUInitOptions@12@@Z", //64-bit name
    //                           galaxy::api::Init_Detour,
    //  static_cast_p2p <void> (&galaxy::api::Init_Original) );
    SK_CreateDLLHook2 (     wszGalaxyDLLName, "?CreateInstance@GalaxyFactory@api@galaxy@@SAPAVIGalaxy@23@XZ",
                               galaxy::api::CreateInstance_Detour,
      static_cast_p2p <void> (&galaxy::api::CreateInstance_Original) );
    //SK_CreateDLLHook2 (     wszGalaxyDLLName, "?ProcessData@api@galaxy@@YAXXZ", // 64-bit name
    //                           galaxy::api::ProcessData_Detour,
    //  static_cast_p2p <void> (&galaxy::api::ProcessData_Original) );
    //SK_CreateDLLHook2 (     wszGalaxyDLLName, "?Shutdown@api@galaxy@@YAXXZ", // 64-bit name
    //                           galaxy::api::Shutdown_Detour,
    //  static_cast_p2p <void> (&galaxy::api::Shutdown_Original) );

    galaxy::api::IGalaxy** ppInstance =
      (galaxy::api::IGalaxy **)SK_GetProcAddress (wszGalaxyDLLName, "?instance@GalaxyFactory@api@galaxy@@0PAVIGalaxy@23@A");

    if (ppInstance != nullptr && *ppInstance != nullptr)
    {
      galaxy::api::IStats*             pStats     = (*ppInstance)->GetStats             ();
      galaxy::api::IUtils*             pUtils     = (*ppInstance)->GetUtils             ();
      galaxy::api::IUser*              pUser      = (*ppInstance)->GetUser              ();
      galaxy::api::IListenerRegistrar* pRegistrar = (*ppInstance)->GetListenerRegistrar ();

      gog->Init (pStats, pUtils, pUser, pRegistrar);

      GALAXY_VIRTUAL_HOOK ( ppInstance,      3,
                          "IGalaxy::Shutdown",
                           galaxy::api::Shutdown_IGalaxy_Detour,
                           galaxy::api::Shutdown_IGalaxy_Original,
                                        Shutdown_IGalaxy_pfn );

      GALAXY_VIRTUAL_HOOK ( ppInstance,     16,
                          "IGalaxy::ProcessData",
                           galaxy::api::ProcessData_IGalaxy_Detour,
                           galaxy::api::ProcessData_IGalaxy_Original,
                                        ProcessData_IGalaxy_pfn );
    }

    bool bEnable = SK_EnableApplyQueuedHooks ();
                         SK_ApplyQueuedHooks ();
    if (!bEnable) SK_DisableApplyQueuedHooks ();
#endif

    SK_ICommandProcessor* cmd = nullptr;

    #define cmdAddAliasedVar(name,pVar)                 \
      for ( const char* alias : { "GOG."      #name,    \
                                  "Platform." #name } ) \
        cmd->AddVariable (alias, pVar);

    SK_RunOnce (
      cmd =
        SK_Render_InitializeSharedCVars ()
    );

    if (cmd != nullptr)
    {
#if 0
      cmdAddAliasedVar (TakeScreenshot,
          SK_CreateVar (SK_IVariable::Boolean,
                          (bool *)&config.platform.achievements.take_screenshot));
      cmdAddAliasedVar (ShowPopup,
          SK_CreateVar (SK_IVariable::Boolean,
                          (bool *)&config.platform.achievements.popup.show));
      cmdAddAliasedVar (PopupDuration,
          SK_CreateVar (SK_IVariable::Int,
                          (int  *)&config.platform.achievements.popup.duration));
      cmdAddAliasedVar (PopupInset,
          SK_CreateVar (SK_IVariable::Float,
                          (float*)&config.platform.achievements.popup.inset));
      cmdAddAliasedVar (ShowPopupTitle,
          SK_CreateVar (SK_IVariable::Boolean,
                          (bool *)&config.platform.achievements.popup.show_title));
      cmdAddAliasedVar (PopupAnimate,
          SK_CreateVar (SK_IVariable::Boolean,
                          (bool *)&config.platform.achievements.popup.animate));
      cmdAddAliasedVar (PlaySound,
          SK_CreateVar (SK_IVariable::Boolean,
                          (bool *)&config.platform.achievements.play_sound));
#endif
    }
  }

  auto _SetupGalaxy =
  [&](void)
  {
#if 0
    // Hook code here (i.e. Listener registrar Register/Unregister and IGalaxy::ProcessData (...))

    bool bEnable = SK_EnableApplyQueuedHooks ();
                         SK_ApplyQueuedHooks ();
    if (!bEnable) SK_DisableApplyQueuedHooks ();
#endif
  };

  if (hModGalaxy != nullptr)
  {
    SK_RunOnce (
      if (SK::Galaxy::GetTicksRetired () == 0)
        _SetupGalaxy ();
    );
  }
}

void
SK::Galaxy::Shutdown (void)
{
}

void
SK_GalaxyContext::PreInit (HMODULE hGalaxyDLL)
{
  sdk_dll_ = hGalaxyDLL;
}

void
SK_GalaxyContext::Init ( galaxy::api::IStats*             stats,
                         galaxy::api::IUtils*             utils,
                         galaxy::api::IUser*              user,
                         galaxy::api::IListenerRegistrar* registrar )
{
  stats_     = stats;
  utils_     = utils;
  user_      = user;
  registrar_ = registrar;
//friends_   = friends;
}

void
SK_GalaxyContext::Shutdown (bool bGameRequested)
{
  std::ignore = bGameRequested;

  stats_     = nullptr;
  utils_     = nullptr;
  user_      = nullptr;
  registrar_ = nullptr;
//friends_   = nullptr;
}

bool
__stdcall
SK_Galaxy_GetOverlayState (bool real)
{
  return
    SK::Galaxy::GetOverlayState (real);
}


SK_LazyGlobal <SK_GalaxyContext> gog;
bool SK::Galaxy::overlay_state = false;

LONGLONG
SK::Galaxy::GetTicksRetired (void)
{
  //if (ReadAcquire64 (&galaxy::api::ticks) > 0)
  //{ static auto
  //      lastFrame  = SK_GetFramesDrawn ();
  //  if (lastFrame != SK_GetFramesDrawn ())
  //  {  galaxy::api::ProcessData_Detour ();
  //      lastFrame  = SK_GetFramesDrawn ();
  //  }
  //}

  return
    ReadAcquire64 (&galaxy::api::ticks);
}

uint32_t
SK_Galaxy_Stats_GetUserTimePlayed (galaxy::api::IStats* This)
{
  switch (gog->version)
  {
    default:
    case SK_GalaxyContext::Version_1_152_1:
      return ((galaxy::api::IStats_1_152_1 *)This)->GetUserTimePlayed ();
      break;
    case SK_GalaxyContext::Version_1_152_10:
      return ((galaxy::api::IStats_1_152_10 *)This)->GetUserTimePlayed ();
      break;
  }
}

void
SK_Galaxy_Stats_GetAchievementNameCopy (galaxy::api::IStats* This, uint32_t index, char* buffer, uint32_t bufferLength)
{
  switch (gog->version)
  {
    default:
    case SK_GalaxyContext::Version_1_152_1:
      // Not Implemented
      //((galaxy::api::IStats_1_152_1 *)This)->GetAchievementNameCopy (index, buffer, bufferLength);
      break;
    case SK_GalaxyContext::Version_1_152_10:
      ((galaxy::api::IStats_1_152_10 *)This)->GetAchievementNameCopy (index, buffer, bufferLength);
      break;
  }
}
void
SK_Galaxy_Stats_GetAchievement (galaxy::api::IStats* This, const char* name, bool& unlocked, uint32_t& unlockTime, galaxy::api::GalaxyID userID)
{
  switch (gog->version)
  {
    default:
    case SK_GalaxyContext::Version_1_152_1:
      ((galaxy::api::IStats_1_152_1 *)This)->GetAchievement  (name, unlocked, unlockTime, userID);
      break;
    case SK_GalaxyContext::Version_1_152_10:
      ((galaxy::api::IStats_1_152_10 *)This)->GetAchievement (name, unlocked, unlockTime, userID);
      break;
  }
}

void
SK_Galaxy_Stats_GetAchievementDisplayNameCopy (galaxy::api::IStats* This, const char* name, char* buffer, uint32_t bufferLength)
{
  switch (gog->version)
  {
    default:
    case SK_GalaxyContext::Version_1_152_1:
      ((galaxy::api::IStats_1_152_1 *)This)->GetAchievementDisplayNameCopy  (name, buffer, bufferLength);
      break;
    case SK_GalaxyContext::Version_1_152_10:
      ((galaxy::api::IStats_1_152_10 *)This)->GetAchievementDisplayNameCopy (name, buffer, bufferLength);
      break;
  }
}
void
SK_Galaxy_Stats_GetAchievementDescriptionCopy (galaxy::api::IStats* This, const char* name, char* buffer, uint32_t bufferLength)
{
  switch (gog->version)
  {
    default:
    case SK_GalaxyContext::Version_1_152_1:
      ((galaxy::api::IStats_1_152_1 *)This)->GetAchievementDescriptionCopy  (name, buffer, bufferLength);
      break;
    case SK_GalaxyContext::Version_1_152_10:
      ((galaxy::api::IStats_1_152_10 *)This)->GetAchievementDescriptionCopy (name, buffer, bufferLength);
      break;
  }
}

bool
SK_Galaxy_Stats_IsAchievementVisibleWhileLocked (galaxy::api::IStats* This, const char* name)
{
  switch (gog->version)
  {
    default:
    case SK_GalaxyContext::Version_1_152_1:
      return ((galaxy::api::IStats_1_152_1 *)This)->IsAchievementVisibleWhileLocked (name);
      break;
    case SK_GalaxyContext::Version_1_152_10:
      return ((galaxy::api::IStats_1_152_10 *)This)->IsAchievementVisibleWhileLocked (name);
      break;
  }
}

void
SK_Galaxy_Stats_RequestUserTimePlayed (galaxy::api::IStats* This, galaxy::api::GalaxyID userID, galaxy::api::IUserTimePlayedRetrieveListener*           const listener)
{
  switch (gog->version)
  {
    default:
    case SK_GalaxyContext::Version_1_152_1:
      ((galaxy::api::IStats_1_152_1 *)This)->RequestUserTimePlayed (userID, listener);
      break;
    case SK_GalaxyContext::Version_1_152_10:
      ((galaxy::api::IStats_1_152_10 *)This)->RequestUserTimePlayed (userID, listener);
      break;
  }
}

void
SK_Galaxy_Stats_RequestUserStatsAndAchievements (galaxy::api::IStats* This, galaxy::api::GalaxyID userID, galaxy::api::IUserStatsAndAchievementsRetrieveListener* const listener)
{
  switch (gog->version)
  {
    default:
    case SK_GalaxyContext::Version_1_152_1:
      ((galaxy::api::IStats_1_152_1 *)This)->RequestUserStatsAndAchievements (userID, listener);
      break;
    case SK_GalaxyContext::Version_1_152_10:
      ((galaxy::api::IStats_1_152_10 *)This)->RequestUserStatsAndAchievements (userID, listener);
      break;
  }
}
void
SK_Galaxy_Stats_SetAchievement (galaxy::api::IStats* This, const char* name)
{
  switch (gog->version)
  {
    default:
    case SK_GalaxyContext::Version_1_152_1:
      ((galaxy::api::IStats_1_152_1 *)This)->SetAchievement (name);
      break;
    case SK_GalaxyContext::Version_1_152_10:
      ((galaxy::api::IStats_1_152_10 *)This)->SetAchievement (name);
      break;
  }
}

void
SK_Galaxy_Stats_ClearAchievement (galaxy::api::IStats* This, const char* name)
{
  switch (gog->version)
  {
    default:
    case SK_GalaxyContext::Version_1_152_1:
      ((galaxy::api::IStats_1_152_1 *)This)->ClearAchievement (name);
      break;
    case SK_GalaxyContext::Version_1_152_10:
      ((galaxy::api::IStats_1_152_10 *)This)->ClearAchievement (name);
      break;
  }
}

// TODO: Add hook on IGalaxy::ProcessData (...) here that increments __SK_Galaxy_Ticks