#ifndef GALAXY_I_LISTENER_REGISTRAR_H
#define GALAXY_I_LISTENER_REGISTRAR_H

#include "stdint.h"
#include <stdlib.h>
#include "GalaxyExport.h"

namespace galaxy
{
    namespace api
    {
        enum ListenerType
        {
            LISTENER_TYPE_BEGIN,
            LOBBY_LIST = LISTENER_TYPE_BEGIN,
            LOBBY_CREATED,
            LOBBY_ENTERED,
            LOBBY_LEFT,
            LOBBY_DATA,
            LOBBY_MEMBER_STATE,
            LOBBY_OWNER_CHANGE,
            AUTH,
            LOBBY_MESSAGE,
            NETWORKING,
            USER_DATA,
            USER_STATS_AND_ACHIEVEMENTS_RETRIEVE,
            STATS_AND_ACHIEVEMENTS_STORE,
            ACHIEVEMENT_CHANGE,
            LEADERBOARDS_RETRIEVE,
            LEADERBOARD_ENTRIES_RETRIEVE,
            LEADERBOARD_SCORE_UPDATE_LISTENER,
            PERSONA_DATA_CHANGED,
            RICH_PRESENCE_CHANGE_LISTENER,
            GAME_JOIN_REQUESTED_LISTENER,
            OPERATIONAL_STATE_CHANGE,
            FRIEND_LIST_RETRIEVE,
            ENCRYPTED_APP_TICKET_RETRIEVE,
            ACCESS_TOKEN_CHANGE,
            LEADERBOARD_RETRIEVE,
            SPECIFIC_USER_DATA,
            INVITATION_SEND,
            RICH_PRESENCE_LISTENER,
            GAME_INVITATION_RECEIVED_LISTENER,
            NOTIFICATION_LISTENER,
            LOBBY_DATA_RETRIEVE,
            USER_TIME_PLAYED_RETRIEVE,
            OTHER_SESSION_START,
            FILE_SHARE,
            SHARED_FILE_DOWNLOAD,
            CUSTOM_NETWORKING_CONNECTION_OPEN,
            CUSTOM_NETWORKING_CONNECTION_CLOSE,
            CUSTOM_NETWORKING_CONNECTION_DATA,
            OVERLAY_INITIALIZATION_STATE_CHANGE,
            OVERLAY_VISIBILITY_CHANGE,
            CHAT_ROOM_WITH_USER_RETRIEVE_LISTENER,
            CHAT_ROOM_MESSAGE_SEND_LISTENER,
            CHAT_ROOM_MESSAGES_LISTENER,
            FRIEND_INVITATION_SEND_LISTENER,
            FRIEND_INVITATION_LIST_RETRIEVE_LISTENER,
            FRIEND_INVITATION_LISTENER,
            FRIEND_INVITATION_RESPOND_TO_LISTENER,
            FRIEND_ADD_LISTENER,
            FRIEND_DELETE_LISTENER,
            CHAT_ROOM_MESSAGES_RETRIEVE_LISTENER,
            USER_FIND_LISTENER,
            NAT_TYPE_DETECTION,
            SENT_FRIEND_INVITATION_LIST_RETRIEVE_LISTENER,
            LOBBY_DATA_UPDATE_LISTENER,
            LOBBY_MEMBER_DATA_UPDATE_LISTENER,
            USER_INFORMATION_RETRIEVE_LISTENER,
            RICH_PRESENCE_RETRIEVE_LISTENER,
            GOG_SERVICES_CONNECTION_STATE_LISTENER,
            TELEMETRY_EVENT_SEND_LISTENER,
            CLOUD_STORAGE_GET_FILE_LIST,
            CLOUD_STORAGE_GET_FILE,
            CLOUD_STORAGE_PUT_FILE,
            CLOUD_STORAGE_DELETE_FILE,
            IS_DLC_OWNED,
            PLAYFAB_CREATE_OPENID_CONNECTION,
            PLAYFAB_LOGIN_WITH_OPENID_CONNECT,
            LISTENER_TYPE_END
        };

        class IGalaxyListener
        {
        public:

            virtual ~IGalaxyListener()
            {
            }
        };

        template<ListenerType type> class GalaxyTypeAwareListener : public IGalaxyListener
        {
        public:

            static ListenerType GetListenerType()
            {
                return type;
            }
        };

        class IListenerRegistrar
        {
        public:

            virtual ~IListenerRegistrar()
            {
            }

            virtual void Register(ListenerType listenerType, IGalaxyListener* listener) = 0;

            virtual void Unregister(ListenerType listenerType, IGalaxyListener* listener) = 0;
        };

        GALAXY_DLL_EXPORT IListenerRegistrar* GALAXY_CALLTYPE ListenerRegistrar();

        GALAXY_DLL_EXPORT IListenerRegistrar* GALAXY_CALLTYPE GameServerListenerRegistrar();

        template<typename _TypeAwareListener, IListenerRegistrar*(*_Registrar)() = ListenerRegistrar>
        class SelfRegisteringListener : public _TypeAwareListener
        {
        public:

            SelfRegisteringListener()
            {
                if (_Registrar())
                    _Registrar()->Register(_TypeAwareListener::GetListenerType(), this);
            }

            ~SelfRegisteringListener()
            {
                if (_Registrar())
                    _Registrar()->Unregister(_TypeAwareListener::GetListenerType(), this);
            }
        };

    }
}

#endif