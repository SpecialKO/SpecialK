#ifndef GALAXY_I_UTILS_H
#define GALAXY_I_UTILS_H

/**
 * @file
 * Contains data structures and interfaces related to common activities.
 */

#include "IListenerRegistrar.h"

namespace galaxy
{
	namespace api
	{
		/**
		 * @addtogroup api
		 * @{
		 */

		/**
		 * The ID of the notification.
		 */
		typedef uint64_t NotificationID;

		/**
		 * Listener for the event of changing overlay state.
		 */
		class IOverlayStateChangeListener : public GalaxyTypeAwareListener<OVERLAY_VISIBILITY_CHANGE>
		{
		public:

			/**
			 * Notification for the event of changing overlay state.
			 *
			 * @param overlayIsActive Indicates if overlay is in front of the game.
			 */
			virtual void OnOverlayStateChanged(bool overlayIsActive) = 0;
		};

		/**
		 * Globally self-registering version of IOverlayStateChangeListener.
		 */
		typedef SelfRegisteringListener<IOverlayStateChangeListener> GlobalOverlayStateChangeListener;

		/**
		 * Listener for the event of receiving a notification.
		 */
		class INotificationListener : public GalaxyTypeAwareListener<NOTIFICATION_LISTENER>
		{
		public:

			/**
			 * Notification for the event of receiving a notification.
			 *
			 * To read the message you need to call GetNotification().
			 *
			 * @param notificationID The ID of the notification.
			 * @param typeLength The size of the type of the notification.
			 * @param contentSize The size of the content of the notification.
			 */
			virtual void OnNotificationReceived(NotificationID notificationID, uint32_t typeLength, uint32_t contentSize) = 0;
		};

		/**
		 * Globally self-registering version of INotificationListener.
		 */
		typedef SelfRegisteringListener<INotificationListener> GlobalNotificationListener;

		/**
		 * The interface for managing images.
		 */
		class IUtils
		{
		public:

			virtual ~IUtils()
			{
			}

			/**
			 * Reads width and height of the image of a specified ID.
			 *
			 * @param imageID The ID of the image.
			 * @param width The width of the image.
			 * @param height The height of the image.
			 */
			virtual void GetImageSize(uint32_t imageID, int32_t& width, int32_t& height) = 0;

			/**
			 * Reads the image of a specified ID.
			 *
			 * @remark The size of the output buffer should be 4 * height * width * sizeof(char).
			 *
			 * @param imageID The ID of the image.
			 * @param buffer The output buffer.
			 * @param bufferLength The size of the output buffer.
			 */
			virtual void GetImageRGBA(uint32_t imageID, unsigned char* buffer, uint32_t bufferLength) = 0;

			/**
			 * Register for notifications of a specified type.
			 *
			 * @remark Notification types starting "__" are reserved and cannot be used.
			 *
			 * @param type The name of the type.
			 */
			virtual void RegisterForNotification(const char* type) = 0;

			/**
			 * Reads a specified notification.
			 *
			 * @param notificationID The ID of the notification.
			 * @param type The output buffer for the type of the notification.
			 * @param typeLength The size of the output buffer for the type of the notification.
			 * @param content The output buffer for the content of the notification.
			 * @param contentSize The size of the output buffer for the content of the notification.
			 * @return The number of bytes written to the content buffer.
			 */
			virtual uint32_t GetNotification(NotificationID notificationID, char* type, uint32_t typeLength, void* content, uint32_t contentSize) = 0;

			/**
			 * Shows web page in overlay.
			 *
			 * @param url The URL address of a specified web page with the limit of 2047 bytes.
			 **/
			virtual void ShowOverlayWithWebPage(const char* url) = 0;
		};

		/** @} */
	}
}

#endif