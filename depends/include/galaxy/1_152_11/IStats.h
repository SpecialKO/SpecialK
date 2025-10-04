#ifndef GALAXY_I_STATS_152_10_H
#define GALAXY_I_STATS_152_10_H

#include "galaxy/IListenerRegistrar.h"
#include "galaxy/GalaxyID.h"

namespace galaxy
{
    namespace api
    {
        class IStats_1_152_10
    		{
    		public:
    
    			virtual ~IStats_1_152_10()
    			{
    			}
    
    			/**
    			 * Performs a request for statistics and achievements of a specified user.
    			 *
    			 * This call is asynchronous. Responses come to the IUserStatsAndAchievementsRetrieveListener
    			 * (for all GlobalUserStatsAndAchievementsRetrieveListener-derived and optional listener passed as argument).
    			 *
    			 * @param [in] userID The ID of the user. It can be omitted when requesting for own data.
    			 * @param [in] listener The listener for specific operation.
    			 */
    			virtual void RequestUserStatsAndAchievements(GalaxyID userID = GalaxyID(), IUserStatsAndAchievementsRetrieveListener* const listener = NULL) = 0;
    
    			/**
    			 * Reads integer value of a statistic of a specified user.
    			 *
    			 * @pre Retrieve the statistics first by calling RequestUserStatsAndAchievements().
    			 *
    			 * @param [in] name The code name of the statistic.
    			 * @param [in] userID The ID of the user. It can be omitted when requesting for own data.
    			 * @return The value of the statistic.
    			 */
    			virtual int32_t GetStatInt(const char* name, GalaxyID userID = GalaxyID()) = 0;
    
    			/**
    			 * Reads floating point value of a statistic of a specified user.
    			 *
    			 * @pre Retrieve the statistics first by calling RequestUserStatsAndAchievements().
    			 *
    			 * @param [in] name The code name of the statistic.
    			 * @param [in] userID The ID of the user. It can be omitted when requesting for own data.
    			 * @return The value of the statistic.
    			 */
    			virtual float GetStatFloat(const char* name, GalaxyID userID = GalaxyID()) = 0;
    
    			/**
    			 * Updates a statistic with an integer value.
    			 *
    			 * @remark In order to make this and other changes persistent, call StoreStatsAndAchievements().
    			 *
    			 * @pre Retrieve the statistics first by calling RequestUserStatsAndAchievements().
    			 *
    			 * @param [in] name The code name of the statistic.
    			 * @param [in] value The value of the statistic to set.
    			 */
    			virtual void SetStatInt(const char* name, int32_t value) = 0;
    
    			/**
    			 * Updates a statistic with a floating point value.
    			 *
    			 * @remark In order to make this and other changes persistent, call StoreStatsAndAchievements().
    			 *
    			 * @pre Retrieve the statistics first by calling RequestUserStatsAndAchievements().
    			 *
    			 * @param [in] name The code name of the statistic.
    			 * @param [in] value The value of the statistic to set.
    			 */
    			virtual void SetStatFloat(const char* name, float value) = 0;
    
    			/**
    			 * Updates an average-rate statistic with a delta.
    			 *
    			 * @remark In order to make this and other changes persistent, call StoreStatsAndAchievements().
    			 *
    			 * @pre Retrieve the statistics first by calling RequestUserStatsAndAchievements().
    			 *
    			 * @param [in] name The code name of the statistic.
    			 * @param [in] countThisSession The delta of the count.
    			 * @param [in] sessionLength The delta of the session.
    			 */
    			virtual void UpdateAvgRateStat(const char* name, float countThisSession, double sessionLength) = 0;
    
    			/**
    			 * Returns a number of achievements.
    			 *
    			 * @pre Retrieve the achievements first by calling RequestUserStatsAndAchievements().
    			 */
    			virtual uint32_t GetAchievementsNumber() = 0;
    
    			/**
    			 * Returns the 'API KEY' for an achievement index between 0 and GetAchievementsNumber.
    			 *
    			 * @pre Retrieve the achievements first by calling RequestUserStatsAndAchievements().
    			 *
    			 * @param [in] index The index of the achievement.
    			 */
    			virtual const char* GetAchievementName(uint32_t index) = 0;
    
    			/**
    			 * Copies the 'API KEY' for an achievement index between 0 and GetAchievementsNumber.
    			 *
    			 * @pre Retrieve the achievements first by calling RequestUserStatsAndAchievements().
    			 *
    			 * @param [in] index The index of the achievement.
    			 * @param [in, out] buffer The output buffer.
    			 * @param [in] bufferLength The size of the output buffer.
    			 */
    			virtual void GetAchievementNameCopy(uint32_t index, char* buffer, uint32_t bufferLength) = 0;
    
    			/**
    			 * Reads the state of an achievement of a specified user.
    			 *
    			 * @pre Retrieve the achievements first by calling RequestUserStatsAndAchievements().
    			 *
    			 * @param [in] name The code name of the achievement.
    			 * @param [in, out] unlocked Indicates if the achievement has been unlocked.
    			 * @param [out] unlockTime The time at which the achievement was unlocked.
    			 * @param [in] userID The ID of the user. It can be omitted when requesting for own data.
    			 */
    			virtual void GetAchievement(const char* name, bool& unlocked, uint32_t& unlockTime, GalaxyID userID = GalaxyID()) = 0;
    
    			/**
    			 * Unlocks an achievement. The achievement is marked as unlocked at the time
    			 * at which this message was called.
    			 *
    			 * @remark In order to make this and other changes persistent, call StoreStatsAndAchievements().
    			 *
    			 * @pre Retrieve the achievements first by calling RequestUserStatsAndAchievements().
    			 *
    			 * @param [in] name The code name of the achievement.
    			 */
    			virtual void SetAchievement(const char* name) = 0;
    
    			/**
    			 * Clears an achievement.
    			 *
    			 * @remark In order to make this and other changes persistent, call StoreStatsAndAchievements().
    			 *
    			 * @pre Retrieve the achievements first by calling RequestUserStatsAndAchievements().
    			 *
    			 * @param [in] name The code name of the achievement.
    			 */
    			virtual void ClearAchievement(const char* name) = 0;
    
    			/**
    			 * Persists all changes in statistics and achievements.
    			 *
    			 * This call is asynchronous. Responses come to the IStatsAndAchievementsStoreListener
    			 * (for all GlobalStatsAndAchievementsStoreListener-derived and optional listener passed as argument).
    			 *
    			 * @remark Notifications about storing changes that result in unlocking
    			 * achievements come to the IAchievementChangeListener.
    			 *
    			 * @param [in] listener The listener for specific operation.
    			 */
    			virtual void StoreStatsAndAchievements(IStatsAndAchievementsStoreListener* const listener = NULL) = 0;
    
    			/**
    			 * Resets all statistics and achievements.
    			 *
    			 * This is the same as setting statistics and achievements to their
    			 * initial values and calling StoreStatsAndAchievements().
    			 *
    			 * This call is asynchronous. Responses come to the IStatsAndAchievementsStoreListener
    			 * (for all GlobalStatsAndAchievementsStoreListener-derived and optional listener passed as argument).
    			 *
    			 * @param [in] listener The listener for specific operation.
    			 */
    			virtual void ResetStatsAndAchievements(IStatsAndAchievementsStoreListener* const listener = NULL) = 0;
    
    			/**
    			 * Returns display name of a specified achievement.
    			 *
    			 * @remark This call is not thread-safe as opposed to GetAchievementDisplayNameCopy().
    			 *
    			 * @pre Retrieve the achievements first by calling RequestUserStatsAndAchievements().
    			 *
    			 * @param [in] name The name of the achievement.
    			 * @return Display name of the specified achievement.
    			 */
    			virtual const char* GetAchievementDisplayName(const char* name) = 0;
    
    			/**
    			 * Copies display name of a specified achievement.
    			 *
    			 * @pre Retrieve the achievements first by calling RequestUserStatsAndAchievements().
    			 *
    			 * @param [in] name The name of the achievement.
    			 * @param [in, out] buffer The output buffer.
    			 * @param [in] bufferLength The size of the output buffer.
    			 */
    			virtual void GetAchievementDisplayNameCopy(const char* name, char* buffer, uint32_t bufferLength) = 0;
    
    			/**
    			 * Returns description of a specified achievement.
    			 *
    			 * @remark This call is not thread-safe as opposed to GetAchievementDescriptionCopy().
    			 *
    			 * @pre Retrieve the achievements first by calling RequestUserStatsAndAchievements().
    			 *
    			 * @param [in] name The name of the achievement.
    			 * @return Description of the specified achievement.
    			 */
    			virtual const char* GetAchievementDescription(const char* name) = 0;
    
    			/**
    			 * Copies description of a specified achievement.
    			 *
    			 * @pre Retrieve the achievements first by calling RequestUserStatsAndAchievements().
    			 *
    			 * @param [in] name The name of the achievement.
    			 * @param [in, out] buffer The output buffer.
    			 * @param [in] bufferLength The size of the output buffer.
    			 */
    			virtual void GetAchievementDescriptionCopy(const char* name, char* buffer, uint32_t bufferLength) = 0;
    
    			/**
    			 * Returns visibility status of a specified achievement.
    			 *
    			 * @pre Retrieve the achievements first by calling RequestUserStatsAndAchievements().
    			 *
    			 * @param [in] name The name of the achievement.
    			 * @return If the achievement is visible.
    			 */
    			virtual bool IsAchievementVisible(const char* name) = 0;
    
    			/**
    			 * Returns visibility status of a specified achievement before unlocking.
    			 *
    			 * @pre Retrieve the achievements first by calling RequestUserStatsAndAchievements().
    			 *
    			 * @param [in] name The name of the achievement.
    			 * @return If the achievement is visible before unlocking.
    			 */
    			virtual bool IsAchievementVisibleWhileLocked(const char* name) = 0;
    
    			/**
    			 * Performs a request for definitions of leaderboards.
    			 *
    			 * This call is asynchronous. Responses come to the ILeaderboardsRetrieveListener.
    			 *
    			 * @param [in] listener The listener for specific operation.
    			 */
    			virtual void RequestLeaderboards(ILeaderboardsRetrieveListener* const listener = NULL) = 0;
    
    			/**
    			 * Returns display name of a specified leaderboard.
    			 *
    			 * @remark This call is not thread-safe as opposed to GetLeaderboardDisplayNameCopy().
    			 *
    			 * @pre Retrieve definition of this particular leaderboard first by calling
    			 * either FindLeaderboard() or FindOrCreateLeaderboard(), or definitions of all existing leaderboards
    			 * by calling RequestLeaderboards().
    			 *
    			 * @param [in] name The name of the leaderboard.
    			 * @return Display name of the leaderboard.
    			 */
    			virtual const char* GetLeaderboardDisplayName(const char* name) = 0;
    
    			/**
    			 * Copies display name of a specified leaderboard.
    			 *
    			 * @pre Retrieve definition of this particular leaderboard first by calling
    			 * either FindLeaderboard() or FindOrCreateLeaderboard(), or definitions of all existing leaderboards
    			 * by calling RequestLeaderboards().
    			 *
    			 * @param [in] name The name of the leaderboard.
    			 * @param [in, out] buffer The output buffer.
    			 * @param [in] bufferLength The size of the output buffer.
    			 */
    			virtual void GetLeaderboardDisplayNameCopy(const char* name, char* buffer, uint32_t bufferLength) = 0;
    
    			/**
    			 * Returns sort method of a specified leaderboard.
    			 *
    			 * @pre Retrieve definition of this particular leaderboard first by calling
    			 * either FindLeaderboard() or FindOrCreateLeaderboard(), or definitions of all existing leaderboards
    			 * by calling RequestLeaderboards().
    			 *
    			 * @param [in] name The name of the leaderboard.
    			 * @return Sort method of the leaderboard.
    			 */
    			virtual LeaderboardSortMethod GetLeaderboardSortMethod(const char* name) = 0;
    
    			/**
    			 * Returns display type of a specified leaderboard.
    			 *
    			 * @pre Retrieve definition of this particular leaderboard first by calling
    			 * either FindLeaderboard() or FindOrCreateLeaderboard(), or definitions of all existing leaderboards
    			 * by calling RequestLeaderboards().
    			 *
    			 * @param [in] name The name of the leaderboard.
    			 * @return Display type of the leaderboard.
    			 */
    			virtual LeaderboardDisplayType GetLeaderboardDisplayType(const char* name) = 0;
    
    			/**
    			 * Performs a request for entries of a specified leaderboard in a global scope,
    			 * i.e. without any specific users in the scope of interest.
    			 *
    			 * The entries are indexed by integers in the range of [0, number of entries).
    			 *
    			 * This call is asynchronous. Responses come to the ILeaderboardEntriesRetrieveListener.
    			 *
    			 * @pre Retrieve definition of this particular leaderboard first by calling
    			 * either FindLeaderboard() or FindOrCreateLeaderboard(), or definitions of all existing leaderboards
    			 * by calling RequestLeaderboards().
    			 *
    			 * @param [in] name The name of the leaderboard.
    			 * @param [in] rangeStart The index position of the entry to start with.
    			 * @param [in] rangeEnd The index position of the entry to finish with.
    			 * @param [in] listener The listener for specific operation.
    			 */
    			virtual void RequestLeaderboardEntriesGlobal(
    				const char* name,
    				uint32_t rangeStart,
    				uint32_t rangeEnd,
    				ILeaderboardEntriesRetrieveListener* const listener = NULL) = 0;
    
    			/**
    			 * Performs a request for entries of a specified leaderboard for and near the specified user.
    			 *
    			 * The specified numbers of entries before and after the specified user are treated as hints.
    			 * If the requested range would go beyond the set of all leaderboard entries, it is shifted
    			 * so that it fits in the set of all leaderboard entries and preserves its size if possible.
    			 *
    			 * This call is asynchronous. Responses come to the ILeaderboardEntriesRetrieveListener.
    			 *
    			 * @remark This call will end with failure in case there is no entry for the specified user
    			 * in the specified leaderboard.
    			 *
    			 * @pre Retrieve definition of this particular leaderboard first by calling
    			 * either FindLeaderboard() or FindOrCreateLeaderboard(), or definitions of all existing leaderboards
    			 * by calling RequestLeaderboards().
    			 *
    			 * @param [in] name The name of the leaderboard.
    			 * @param [in] countBefore The number of entries placed before the user's entry to retrieve (hint).
    			 * @param [in] countAfter The number of entries placed after the user's entry to retrieve (hint).
    			 * @param [in] userID The ID of the user. It can be omitted when requesting for own data.
    			 * @param [in] listener The listener for specific operation.
    			 */
    			virtual void RequestLeaderboardEntriesAroundUser(
    				const char* name,
    				uint32_t countBefore,
    				uint32_t countAfter,
    				GalaxyID userID = GalaxyID(),
    				ILeaderboardEntriesRetrieveListener* const listener = NULL) = 0;
    
    			/**
    			 * Performs a request for entries of a specified leaderboard for specified users.
    			 *
    			 * This call is asynchronous. Responses come to the ILeaderboardEntriesRetrieveListener.
    			 *
    			 * @pre Retrieve definition of this particular leaderboard first by calling
    			 * either FindLeaderboard() or FindOrCreateLeaderboard(), or definitions of all existing leaderboards
    			 * by calling RequestLeaderboards().
    			 *
    			 * @param [in] name The name of the leaderboard.
    			 * @param [in] userArray An array with the list of IDs of the users in scope.
    			 * @param [in] userArraySize The size of the array, i.e. the number of users in the specified list.
    			 * @param [in] listener The listener for specific operation.
    			 */
    			virtual void RequestLeaderboardEntriesForUsers(
    				const char* name,
    				GalaxyID* userArray,
    				uint32_t userArraySize,
    				ILeaderboardEntriesRetrieveListener* const listener = NULL) = 0;
    
    			/**
    			 * Returns data from the currently processed request for leaderboard entries.
    			 *
    			 * Use this call to iterate over last retrieved leaderboard entries, indexed from 0.
    			 *
    			 * @remark This method can be used only inside of
    			 * ILeaderboardEntriesRetrieveListener::OnLeaderboardEntriesRetrieveSuccess().
    			 *
    			 * @pre In order to retrieve lobbies and get their count, first you need to call
    			 * RequestLeaderboardEntriesGlobal(), RequestLeaderboardEntriesAroundUser(),
    			 * or RequestLeaderboardEntriesForUsers().
    			 *
    			 * @param [in] index Index as an integer in the range of [0, number of entries fetched).
    			 * @param [out] rank User's rank in the leaderboard.
    			 * @param [out] score User's score in the leaderboard.
    			 * @param [out] userID The ID of the user.
    			 */
    			virtual void GetRequestedLeaderboardEntry(uint32_t index, uint32_t& rank, int32_t& score, GalaxyID& userID) = 0;
    
    			/**
    			 * Returns data with details from the currently processed request for leaderboard entries.
    			 *
    			 * Use this call to iterate over last retrieved leaderboard entries, indexed from 0.
    			 *
    			 * If the buffer that is supposed to take the details data is too small,
    			 * the details will be truncated to its size.
    			 *
    			 * @pre In order to retrieve lobbies and get their count, first you need to call
    			 * RequestLeaderboardEntriesGlobal(), RequestLeaderboardEntriesAroundUser(),
    			 * or RequestLeaderboardEntriesForUsers().
    			 *
    			 * @remark This method can be used only inside of
    			 * ILeaderboardEntriesRetrieveListener::OnLeaderboardEntriesRetrieveSuccess().
    			 *
    			 * @param [in] index Index as an integer in the range of [0, number of entries fetched).
    			 * @param [out] rank User's rank in the leaderboard.
    			 * @param [out] score User's score in the leaderboard.
    			 * @param [in, out] details An extra, outgoing game-defined information regarding how the user got that score.
    			 * @param [in] detailsSize The size of passed buffer of the extra game-defined information.
    			 * @param [out] outDetailsSize The size of the extra game-defined information.
    			 * @param [out] userID The ID of the user.
    			 */
    			virtual void GetRequestedLeaderboardEntryWithDetails(
    				uint32_t index,
    				uint32_t& rank,
    				int32_t& score,
    				void* details,
    				uint32_t detailsSize,
    				uint32_t& outDetailsSize,
    				GalaxyID& userID) = 0;
    
    			/**
    			 * Updates entry for own user in a specified leaderboard.
    			 *
    			 * This call is asynchronous. Responses come to the ILeaderboardScoreUpdateListener.
    			 *
    			 * @pre Retrieve definition of this particular leaderboard first by calling
    			 * either FindLeaderboard() or FindOrCreateLeaderboard(), or definitions of all existing leaderboards
    			 * by calling RequestLeaderboards().
    			 *
    			 * @pre For this call to work while the user is logged off, the definition of the leaderboard
    			 * must have been retrieved at least once while the user was logged on.
    			 *
    			 * @param [in] name The name of the leaderboard.
    			 * @param [in] score The score to set.
    			 * @param [in] forceUpdate If the update should be performed in case the score is worse than the previous score.
    			 * @param [in] listener The listener for specific operation.
    			 */
    			virtual void SetLeaderboardScore(
    				const char* name,
    				int32_t score,
    				bool forceUpdate = false,
    				ILeaderboardScoreUpdateListener* const listener = NULL) = 0;
    
    			/**
    			 * Updates entry with details for own user in a specified leaderboard.
    			 *
    			 * This call is asynchronous. Responses come to the ILeaderboardScoreUpdateListener.
    			 *
    			 * @pre Retrieve definition of this particular leaderboard first by calling
    			 * either FindLeaderboard() or FindOrCreateLeaderboard(), or definitions of all existing leaderboards
    			 * by calling RequestLeaderboards().
    			 *
    			 * @pre For this call to work while the user is logged off, the definition of the leaderboard
    			 * must have been retrieved at least once while the user was logged on.
    			 *
    			 * @param [in] name The name of the leaderboard.
    			 * @param [in] score The score to set.
    			 * @param [in] details An extra game-defined information regarding how the user got that score with the limit of 3071 bytes.
    			 * @param [in] detailsSize The size of buffer of the extra game-defined information.
    			 * @param [in] forceUpdate If the update should be performed in case the score is worse than the previous score.
    			 * @param [in] listener The listener for specific operation.
    			 */
    			virtual void SetLeaderboardScoreWithDetails(
    				const char* name,
    				int32_t score,
    				const void* details,
    				uint32_t detailsSize,
    				bool forceUpdate = false,
    				ILeaderboardScoreUpdateListener* const listener = NULL) = 0;
    
    			/**
    			 * Returns the leaderboard entry count for requested leaderboard.
    			 *
    			 * @pre In order to retrieve leaderboard entry count, first you need to call
    			 * RequestLeaderboardEntriesGlobal(), RequestLeaderboardEntriesAroundUser(),
    			 * or RequestLeaderboardEntriesForUsers().
    			 *
    			 * @param [in] name The name of the leaderboard.
    			 * @return The leaderboard entry count.
    			 */
    			virtual uint32_t GetLeaderboardEntryCount(const char* name) = 0;
    
    			/**
    			 * Performs a request for definition of a specified leaderboard.
    			 *
    			 * This call is asynchronous. Responses come to the ILeaderboardRetrieveListener.
    			 *
    			 * @param [in] name The name of the leaderboard.
    			 * @param [in] listener The listener for specific operation.
    			 */
    			virtual void FindLeaderboard(const char* name, ILeaderboardRetrieveListener* const listener = NULL) = 0;
    
    			/**
    			 * Performs a request for definition of a specified leaderboard, creating it
    			 * if there is no such leaderboard yet.
    			 *
    			 * This call is asynchronous. Responses come to the ILeaderboardRetrieveListener.
    			 *
    			 * @remark If the leaderboard of the specified name is found, this call ends
    			 * with success no matter if the definition of the leaderboard matches
    			 * the parameters specified in the call to this method.
    			 *
    			 * @param [in] name The name of the leaderboard.
    			 * @param [in] displayName The display name of the leaderboard.
    			 * @param [in] sortMethod The sort method of the leaderboard.
    			 * @param [in] displayType The display method of the leaderboard.
    			 * @param [in] listener The listener for specific operation.
    			 */
    			virtual void FindOrCreateLeaderboard(
    				const char* name,
    				const char* displayName,
    				const LeaderboardSortMethod& sortMethod,
    				const LeaderboardDisplayType& displayType,
    				ILeaderboardRetrieveListener* const listener = NULL) = 0;
    
    			/**
    			 * Performs a request for user time played.
    			 *
    			 * This call is asynchronous. Responses come to the IUserTimePlayedRetrieveListener.
    			 *
    			 * @param [in] userID The ID of the user. It can be omitted when requesting for own data.
    			 * @param [in] listener The listener for specific operation.
    			 */
    			virtual void RequestUserTimePlayed(GalaxyID userID = GalaxyID(), IUserTimePlayedRetrieveListener* const listener = NULL) = 0;
    
    			/**
    			 * Reads the number of seconds played by a specified user.
    			 *
    			 * @pre Retrieve the statistics first by calling RequestUserTimePlayed().
    			 *
    			 * @param [in] userID The ID of the user. It can be omitted when requesting for own data.
    			 * @return The number of seconds played by the specified user.
    			 */
    			virtual uint32_t GetUserTimePlayed(GalaxyID userID = GalaxyID()) = 0;
    		};
    }
}

#endif