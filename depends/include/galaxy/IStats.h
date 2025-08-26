#ifndef GALAXY_I_STATS_H
#define GALAXY_I_STATS_H

#include "IListenerRegistrar.h"
#include "GalaxyID.h"

namespace galaxy
{
    namespace api
    {
        enum LeaderboardSortMethod
        {
            LEADERBOARD_SORT_METHOD_NONE,
            LEADERBOARD_SORT_METHOD_ASCENDING,
            LEADERBOARD_SORT_METHOD_DESCENDING
        };

        enum LeaderboardDisplayType
        {
            LEADERBOARD_DISPLAY_TYPE_NONE,
            LEADERBOARD_DISPLAY_TYPE_NUMBER,
            LEADERBOARD_DISPLAY_TYPE_TIME_SECONDS,
            LEADERBOARD_DISPLAY_TYPE_TIME_MILLISECONDS
        };

        class IUserStatsAndAchievementsRetrieveListener : public GalaxyTypeAwareListener<USER_STATS_AND_ACHIEVEMENTS_RETRIEVE>
        {
        public:

            virtual void OnUserStatsAndAchievementsRetrieveSuccess(GalaxyID userID) = 0;

            enum FailureReason
            {
                FAILURE_REASON_UNDEFINED,
                FAILURE_REASON_CONNECTION_FAILURE
            };

            virtual void OnUserStatsAndAchievementsRetrieveFailure(GalaxyID userID, FailureReason failureReason) = 0;
        };

        typedef SelfRegisteringListener<IUserStatsAndAchievementsRetrieveListener> GlobalUserStatsAndAchievementsRetrieveListener;

        class IStatsAndAchievementsStoreListener : public GalaxyTypeAwareListener<STATS_AND_ACHIEVEMENTS_STORE>
        {
        public:

            virtual void OnUserStatsAndAchievementsStoreSuccess() = 0;

            enum FailureReason
            {
                FAILURE_REASON_UNDEFINED,
                FAILURE_REASON_CONNECTION_FAILURE
            };

            virtual void OnUserStatsAndAchievementsStoreFailure(FailureReason failureReason) = 0;
        };

        typedef SelfRegisteringListener<IStatsAndAchievementsStoreListener> GlobalStatsAndAchievementsStoreListener;

        class IAchievementChangeListener : public GalaxyTypeAwareListener<ACHIEVEMENT_CHANGE>
        {
        public:

            // // /**
            // //  * Notification for the event of changing progress in unlocking
            // //  * a particular achievement.
            // //  *
            // //  * @param [in] name The code name of the achievement.
            // //  * @param [in] currentProgress Current value of progress for the achievement.
            // //  * @param [in] maxProgress The maximum value of progress for the achievement.
            // //  */
            // // void OnAchievementProgressChanged(const char* name, uint32_t currentProgress, uint32_t maxProgress) = 0;

            virtual void OnAchievementUnlocked(const char* name) = 0;
        };

        typedef SelfRegisteringListener<IAchievementChangeListener> GlobalAchievementChangeListener;

        class ILeaderboardsRetrieveListener : public GalaxyTypeAwareListener<LEADERBOARDS_RETRIEVE>
        {
        public:

            virtual void OnLeaderboardsRetrieveSuccess() = 0;

            enum FailureReason
            {
                FAILURE_REASON_UNDEFINED,
                FAILURE_REASON_CONNECTION_FAILURE
            };

            virtual void OnLeaderboardsRetrieveFailure(FailureReason failureReason) = 0;
        };

        typedef SelfRegisteringListener<ILeaderboardsRetrieveListener> GlobalLeaderboardsRetrieveListener;

        class ILeaderboardEntriesRetrieveListener : public GalaxyTypeAwareListener<LEADERBOARD_ENTRIES_RETRIEVE>
        {
        public:

            virtual void OnLeaderboardEntriesRetrieveSuccess(const char* name, uint32_t entryCount) = 0;

            enum FailureReason
            {
                FAILURE_REASON_UNDEFINED,
                FAILURE_REASON_NOT_FOUND,
                FAILURE_REASON_CONNECTION_FAILURE
            };

            virtual void OnLeaderboardEntriesRetrieveFailure(const char* name, FailureReason failureReason) = 0;
        };

        typedef SelfRegisteringListener<ILeaderboardEntriesRetrieveListener> GlobalLeaderboardEntriesRetrieveListener;

        class ILeaderboardScoreUpdateListener : public GalaxyTypeAwareListener<LEADERBOARD_SCORE_UPDATE_LISTENER>
        {
        public:

            virtual void OnLeaderboardScoreUpdateSuccess(const char* name, int32_t score, uint32_t oldRank, uint32_t newRank) = 0;

            enum FailureReason
            {
                FAILURE_REASON_UNDEFINED,
                FAILURE_REASON_NO_IMPROVEMENT,
                FAILURE_REASON_CONNECTION_FAILURE
            };

            virtual void OnLeaderboardScoreUpdateFailure(const char* name, int32_t score, FailureReason failureReason) = 0;
        };

        typedef SelfRegisteringListener<ILeaderboardScoreUpdateListener> GlobalLeaderboardScoreUpdateListener;

        class ILeaderboardRetrieveListener : public GalaxyTypeAwareListener<LEADERBOARD_RETRIEVE>
        {
        public:

            virtual void OnLeaderboardRetrieveSuccess(const char* name) = 0;

            enum FailureReason
            {
                FAILURE_REASON_UNDEFINED,
                FAILURE_REASON_CONNECTION_FAILURE
            };

            virtual void OnLeaderboardRetrieveFailure(const char* name, FailureReason failureReason) = 0;
        };

        typedef SelfRegisteringListener<ILeaderboardRetrieveListener> GlobalLeaderboardRetrieveListener;

        class IUserTimePlayedRetrieveListener : public GalaxyTypeAwareListener<USER_TIME_PLAYED_RETRIEVE>
        {
        public:

            virtual void OnUserTimePlayedRetrieveSuccess(GalaxyID userID) = 0;

            enum FailureReason
            {
                FAILURE_REASON_UNDEFINED,
                FAILURE_REASON_CONNECTION_FAILURE
            };

            virtual void OnUserTimePlayedRetrieveFailure(GalaxyID userID, FailureReason failureReason) = 0;
        };

        typedef SelfRegisteringListener<IUserTimePlayedRetrieveListener> GlobalUserTimePlayedRetrieveListener;

        class IStats
        {
        public:

            virtual ~IStats()
            {
            }

            virtual void RequestUserStatsAndAchievements(GalaxyID userID = GalaxyID(), IUserStatsAndAchievementsRetrieveListener* const listener = NULL) = 0;

            virtual int32_t GetStatInt(const char* name, GalaxyID userID = GalaxyID()) = 0;

            virtual float GetStatFloat(const char* name, GalaxyID userID = GalaxyID()) = 0;

            virtual void SetStatInt(const char* name, int32_t value) = 0;

            virtual void SetStatFloat(const char* name, float value) = 0;

            virtual void UpdateAvgRateStat(const char* name, float countThisSession, double sessionLength) = 0;

            virtual void GetAchievement(const char* name, bool& unlocked, uint32_t& unlockTime, GalaxyID userID = GalaxyID()) = 0;

            virtual void SetAchievement(const char* name) = 0;

            virtual void ClearAchievement(const char* name) = 0;

            virtual void StoreStatsAndAchievements(IStatsAndAchievementsStoreListener* const listener = NULL) = 0;

            virtual void ResetStatsAndAchievements(IStatsAndAchievementsStoreListener* const listener = NULL) = 0;

            virtual const char* GetAchievementDisplayName(const char* name) = 0;

            virtual void GetAchievementDisplayNameCopy(const char* name, char* buffer, uint32_t bufferLength) = 0;

            virtual const char* GetAchievementDescription(const char* name) = 0;

            virtual void GetAchievementDescriptionCopy(const char* name, char* buffer, uint32_t bufferLength) = 0;

            virtual bool IsAchievementVisible(const char* name) = 0;

            virtual bool IsAchievementVisibleWhileLocked(const char* name) = 0;

            virtual void RequestLeaderboards(ILeaderboardsRetrieveListener* const listener = NULL) = 0;

            virtual const char* GetLeaderboardDisplayName(const char* name) = 0;

            virtual void GetLeaderboardDisplayNameCopy(const char* name, char* buffer, uint32_t bufferLength) = 0;

            virtual LeaderboardSortMethod GetLeaderboardSortMethod(const char* name) = 0;

            virtual LeaderboardDisplayType GetLeaderboardDisplayType(const char* name) = 0;

            virtual void RequestLeaderboardEntriesGlobal(
                const char* name,
                uint32_t rangeStart,
                uint32_t rangeEnd,
                ILeaderboardEntriesRetrieveListener* const listener = NULL) = 0;

            virtual void RequestLeaderboardEntriesAroundUser(
                const char* name,
                uint32_t countBefore,
                uint32_t countAfter,
                GalaxyID userID = GalaxyID(),
                ILeaderboardEntriesRetrieveListener* const listener = NULL) = 0;

            virtual void RequestLeaderboardEntriesForUsers(
                const char* name,
                GalaxyID* userArray,
                uint32_t userArraySize,
                ILeaderboardEntriesRetrieveListener* const listener = NULL) = 0;

            virtual void GetRequestedLeaderboardEntry(uint32_t index, uint32_t& rank, int32_t& score, GalaxyID& userID) = 0;

            virtual void GetRequestedLeaderboardEntryWithDetails(
                uint32_t index,
                uint32_t& rank,
                int32_t& score,
                void* details,
                uint32_t detailsSize,
                uint32_t& outDetailsSize,
                GalaxyID& userID) = 0;

            virtual void SetLeaderboardScore(
                const char* name,
                int32_t score,
                bool forceUpdate = false,
                ILeaderboardScoreUpdateListener* const listener = NULL) = 0;

            virtual void SetLeaderboardScoreWithDetails(
                const char* name,
                int32_t score,
                const void* details,
                uint32_t detailsSize,
                bool forceUpdate = false,
                ILeaderboardScoreUpdateListener* const listener = NULL) = 0;

            virtual uint32_t GetLeaderboardEntryCount(const char* name) = 0;

            virtual void FindLeaderboard(const char* name, ILeaderboardRetrieveListener* const listener = NULL) = 0;

            virtual void FindOrCreateLeaderboard(
                const char* name,
                const char* displayName,
                const LeaderboardSortMethod& sortMethod,
                const LeaderboardDisplayType& displayType,
                ILeaderboardRetrieveListener* const listener = NULL) = 0;

            virtual void RequestUserTimePlayed(GalaxyID userID = GalaxyID(), IUserTimePlayedRetrieveListener* const listener = NULL) = 0;

            virtual uint32_t GetUserTimePlayed(GalaxyID userID = GalaxyID()) = 0;
        };

    }
}

#endif