/*
COPYRIGHT 2015 - PROPERTY OF TOBII AB
-------------------------------------
2015 TOBII AB - KARLSROVAGEN 2D, DANDERYD 182 53, SWEDEN - All Rights Reserved.

NOTICE:  All information contained herein is, and remains, the property of Tobii AB and its suppliers, if any.  
The intellectual and technical concepts contained herein are proprietary to Tobii AB and its suppliers and may be 
covered by U.S.and Foreign Patents, patent applications, and are protected by trade secret or copyright law. 
Dissemination of this information or reproduction of this material is strictly forbidden unless prior written 
permission is obtained from Tobii AB.
*/

#ifndef tobii_streams_h_included
#define tobii_streams_h_included

#include "tobii.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tobii_gaze_point_t
{
    int64_t timestamp_us;
    tobii_validity_t validity;
    float position_xy[ 2 ];
} tobii_gaze_point_t;

typedef void ( *tobii_gaze_point_callback_t )( tobii_gaze_point_t const* gaze_point, void* user_data );

TOBII_API tobii_error_t TOBII_CALL tobii_gaze_point_subscribe( tobii_device_t* device,
    tobii_gaze_point_callback_t callback, void* user_data );
TOBII_API tobii_error_t TOBII_CALL tobii_gaze_point_unsubscribe( tobii_device_t* device );


typedef struct tobii_gaze_origin_t
{
    int64_t timestamp_us;
    tobii_validity_t left_validity;
    float left_xyz[ 3 ];
    tobii_validity_t right_validity;
    float right_xyz[ 3 ];
} tobii_gaze_origin_t;

typedef void ( *tobii_gaze_origin_callback_t )( tobii_gaze_origin_t const* gaze_origin, void* user_data );
TOBII_API tobii_error_t TOBII_CALL tobii_gaze_origin_subscribe( tobii_device_t* device,
    tobii_gaze_origin_callback_t callback, void* user_data );
TOBII_API tobii_error_t TOBII_CALL tobii_gaze_origin_unsubscribe( tobii_device_t* device );


typedef struct tobii_eye_position_normalized_t
{
    int64_t timestamp_us;
    tobii_validity_t left_validity;
    float left_xyz[ 3 ];
    tobii_validity_t right_validity;
    float right_xyz[ 3 ];
} tobii_eye_position_normalized_t;

typedef void ( *tobii_eye_position_normalized_callback_t )( tobii_eye_position_normalized_t const* eye_position, void* user_data );
TOBII_API tobii_error_t TOBII_CALL tobii_eye_position_normalized_subscribe( tobii_device_t* device,
    tobii_eye_position_normalized_callback_t callback, void* user_data );
TOBII_API tobii_error_t TOBII_CALL tobii_eye_position_normalized_unsubscribe( tobii_device_t* device );


typedef enum tobii_user_presence_status_t
{
    TOBII_USER_PRESENCE_STATUS_UNKNOWN,
    TOBII_USER_PRESENCE_STATUS_AWAY,
    TOBII_USER_PRESENCE_STATUS_PRESENT,
} tobii_user_presence_status_t;

typedef void ( *tobii_user_presence_callback_t )( tobii_user_presence_status_t status, int64_t timestamp_us,
    void* user_data );
TOBII_API tobii_error_t TOBII_CALL tobii_user_presence_subscribe( tobii_device_t* device,
    tobii_user_presence_callback_t callback, void* user_data );
TOBII_API tobii_error_t TOBII_CALL tobii_user_presence_unsubscribe( tobii_device_t* device );


typedef struct tobii_head_pose_t
{
    int64_t timestamp_us;
    tobii_validity_t position_validity;
    float position_xyz[ 3 ];
    tobii_validity_t rotation_validity_xyz[ 3 ];
    float rotation_xyz[ 3 ];
} tobii_head_pose_t;

typedef void( *tobii_head_pose_callback_t )( tobii_head_pose_t const* head_pose, void* user_data );
TOBII_API tobii_error_t TOBII_CALL tobii_head_pose_subscribe( tobii_device_t* device,
    tobii_head_pose_callback_t callback, void* user_data );
TOBII_API tobii_error_t TOBII_CALL tobii_head_pose_unsubscribe( tobii_device_t* device );


typedef enum tobii_notification_type_t
{
    TOBII_NOTIFICATION_TYPE_CALIBRATION_STATE_CHANGED,
    TOBII_NOTIFICATION_TYPE_EXCLUSIVE_MODE_STATE_CHANGED,
    TOBII_NOTIFICATION_TYPE_TRACK_BOX_CHANGED,
    TOBII_NOTIFICATION_TYPE_DISPLAY_AREA_CHANGED,
    TOBII_NOTIFICATION_TYPE_FRAMERATE_CHANGED,
    TOBII_NOTIFICATION_TYPE_POWER_SAVE_STATE_CHANGED,
    TOBII_NOTIFICATION_TYPE_DEVICE_PAUSED_STATE_CHANGED,
    TOBII_NOTIFICATION_TYPE_CALIBRATION_ENABLED_EYE_CHANGED,
    TOBII_NOTIFICATION_TYPE_CALIBRATION_ID_CHANGED,
    TOBII_NOTIFICATION_TYPE_COMBINED_GAZE_FACTOR_CHANGED,
    TOBII_NOTIFICATION_TYPE_FAULTS_CHANGED,
    TOBII_NOTIFICATION_TYPE_WARNINGS_CHANGED,
    TOBII_NOTIFICATION_TYPE_FACE_TYPE_CHANGED,
} tobii_notification_type_t;

typedef enum tobii_notification_value_type_t
{
    TOBII_NOTIFICATION_VALUE_TYPE_NONE,
    TOBII_NOTIFICATION_VALUE_TYPE_FLOAT,
    TOBII_NOTIFICATION_VALUE_TYPE_STATE,
    TOBII_NOTIFICATION_VALUE_TYPE_DISPLAY_AREA,
    TOBII_NOTIFICATION_VALUE_TYPE_UINT,
    TOBII_NOTIFICATION_VALUE_TYPE_ENABLED_EYE,
    TOBII_NOTIFICATION_VALUE_TYPE_STRING,
} tobii_notification_value_type_t;

typedef struct tobii_notification_t
{
    tobii_notification_type_t type;
    tobii_notification_value_type_t value_type;
    union
    {
        float float_;
        tobii_state_bool_t state;
        tobii_display_area_t display_area;
        uint32_t uint_;
        tobii_enabled_eye_t enabled_eye;
        tobii_state_string_t string_;
    } value;

} tobii_notification_t;

typedef void( *tobii_notifications_callback_t )( tobii_notification_t const* notification,
    void* user_data );
TOBII_API tobii_error_t TOBII_CALL tobii_notifications_subscribe( tobii_device_t* device,
    tobii_notifications_callback_t callback, void* user_data );
TOBII_API tobii_error_t TOBII_CALL tobii_notifications_unsubscribe( tobii_device_t* device );



#ifdef __cplusplus
}
#endif

#endif /* tobii_streams_h_included */


/**
@defgroup tobii_streams tobii_streams.h

tobii_streams.h
===============

The tobii_streams.h header file is used for managing data stream subscriptions. There are several types of data streams
in the API, and tobii_streams.h contains functions to subscribe to and unsubscribe from these streams, as well as data
structures describing the data packages.

Please note that there can only be one callback registered to a stream at a time. To register a new callback, first
unsubscribe from the stream, then resubscribe with the new callback function.

Do NOT call StreamEngine API functions from within the callback functions, due to risk of internal deadlocks. Generally
one should finish the callback functions as quickly as possible and not make any blocking calls.

*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_gaze_point_subscribe( tobii_device_t* device, tobii_gaze_point_callback_t callback, void* user_data );
@ingroup tobii_streams

tobii_gaze_point_subscribe
--------------------------

### Function

Start listening for gaze point data; the position on the screen that the user is currently looking at.


### Syntax

    #include <tobii/tobii_streams.h>
    tobii_error_t tobii_gaze_point_subscribe( tobii_device_t* device,
        tobii_gaze_point_callback_t callback, void* user_data );


### Remarks

This subscription is for receiving the point on the screen, in normalized (0 to 1) coordinates, that the user is
currently looking at. The data is lightly filtered for stability.

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create.

*callback* is a function pointer to a function with the prototype:

    void gaze_point_callback( tobii_gaze_point_t const* gaze_point, void* user_data )

This function will be called when there is new gaze data available. It is called with the following parameters:

-   *gaze_point*

    This is a pointer to a struct containing the following data:

    -   *timestamp_us*
        Timestamp value for when the gaze point was captured, measured in microseconds (us). The epoch is undefined,
        so these timestamps are only useful for calculating the time elapsed between a pair of values. The function
        tobii_system_clock() can be used to retrieve a timestamp using the same clock and same relative values as this
        timestamp.

    -   *validity*
        **TOBII_VALIDITY_VALID** if the gaze point is valid, **TOBII_VALIDITY_INVALID** if it is not. The value of
        the *position_xy* field is unspecified unless *validity* is **TOBII_VALIDITY_VALID**.

    -   *position_xy*
        An array of two floats, for the horizontal (x) and vertical (y) screen coordinate of the gaze point. The left 
        edge of the screen is 0.0, and the right edge is 1.0. The top edge of the screen is 0.0, and the bottom edge is 
        1.0. Note that the value might be outside the 0.0 to 1.0 range, if the user looks outside the screen.


-   *user_data*
    This is the custom pointer sent in when registering the callback.


*user_data* custom pointer which will be passed unmodified to the callback.


### Return value

If the operation is successful, tobii_gaze_point_subscribe returns **TOBII_ERROR_NO_ERROR**. If the call
fails, tobii_gaze_point_subscribe returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* or *callback* parameters were passed in as NULL. 

-   **TOBII_ERROR_ALREADY_SUBSCRIBED**

    A subscription for gaze points were already made. There can only be one callback registered at a time.
    To change to another callback, first call tobii_gaze_point_unsubscribe().

-   **TOBII_ERROR_NOT_SUPPORTED**

    The device doesn't support the stream. This error is returned if the API is called with an old device and/or that is
    running outdated firmware.

-   **TOBII_ERROR_TOO_MANY_SUBSCRIBERS**

    Too many subscribers for the requested stream. Tobii eye trackers can have a limitation on the number of concurrent
    subscribers to specific streams due to high bandwidth and/or high frequency of the data stream.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_gaze_point_subscribe from within a callback function is not supported.

### See also

tobii_gaze_point_unsubscribe(), tobii_device_process_callbacks(), tobii_system_clock()


### Example

@code{.c}

    #include <tobii/tobii_streams.h>
    #include <stdio.h>
    #include <assert.h>

    void gaze_point_callback( tobii_gaze_point_t const* gaze_point, void* user_data )
    {
        if( gaze_point->validity == TOBII_VALIDITY_VALID )
            printf( "Gaze point: %f, %f\n",
                gaze_point->position_xy[ 0 ],
                gaze_point->position_xy[ 1 ] );
    }

    static void url_receiver( char const* url, void* user_data )
    {
        char* buffer = (char*)user_data;
        if( *buffer != '\0' ) return; // only keep first value

        if( strlen( url ) < 256 )
            strcpy( buffer, url );
    }

    int main()
    {
        tobii_api_t* api;
        tobii_error_t error = tobii_api_create( &api, NULL, NULL );
        assert( error == TOBII_ERROR_NO_ERROR );

        char url[ 256 ] = { 0 };
        error = tobii_enumerate_local_device_urls( api, url_receiver, url );
        assert( error == TOBII_ERROR_NO_ERROR && *url != '\0' );

        tobii_device_t* device;
        error = tobii_device_create( api, url, &device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_gaze_point_subscribe( device, gaze_point_callback, 0 );
        assert( error == TOBII_ERROR_NO_ERROR );

        int is_running = 1000; // in this sample, exit after some iterations
        while( --is_running > 0 )
        {
            error = tobii_wait_for_callbacks( NULL, 1, &device );
            assert( error == TOBII_ERROR_NO_ERROR || error == TOBII_ERROR_TIMED_OUT );

            error = tobii_device_process_callbacks( device );
            assert( error == TOBII_ERROR_NO_ERROR );
        }

        error = tobii_gaze_point_unsubscribe( device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_device_destroy( device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_api_destroy( api );
        assert( error == TOBII_ERROR_NO_ERROR );
        return 0;
    }

@endcode

*/


/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_gaze_point_unsubscribe( tobii_device_t* device );
@ingroup tobii_streams

tobii_gaze_point_unsubscribe
----------------------------

### Function

Stops listening to gaze point stream that was subscribed to by a call to tobii_gaze_point_subscribe()


### Syntax

    #include <tobii/tobii_streams.h>
    tobii_error_t tobii_gaze_point_unsubscribe( tobii_device_t* device );


### Remarks

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create.


### Return value

If the operation is successful, tobii_gaze_point_unsubscribe returns **TOBII_ERROR_NO_ERROR**. If the call
fails, tobii_gaze_point_unsubscribe returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* parameter was passed in as NULL. 

-   **TOBII_ERROR_NOT_SUBSCRIBED**

    There was no subscription for gaze points. It is only valid to call tobii_gaze_point_unsubscribe()
    after first successfully calling tobii_gaze_point_subscribe().

-   **TOBII_ERROR_NOT_SUPPORTED**

    The device doesn't support the stream. This error is returned if the API is called with an old device and/or that is
    running outdated firmware.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_gaze_point_unsubscribe from within a callback function is not supported.

### See also

tobii_gaze_point_subscribe()


### Example

See tobii_gaze_point_subscribe()

*/


/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_gaze_origin_subscribe( tobii_device_t* device, tobii_gaze_origin_callback_t callback, void* user_data );
@ingroup tobii_streams

tobii_gaze_origin_subscribe
---------------------------

### Function

Start listening for gaze origin data. Gaze origin is a point on the users eye, reported in millimeters from the center
of the display.


### Syntax

    #include <tobii/tobii_streams.h>
    tobii_error_t tobii_gaze_origin_subscribe( tobii_device_t* device,
        tobii_gaze_origin_callback_t callback, void* user_data );


### Remarks

This subscription is for receiving the origin of the gaze vector, measured in millimeters from the center of the 
display. Gaze origin is a point on the users eye, but the exact point of the origin varies by device. For example, it 
might be defined as the center of the pupil or the center of the cornea. The data is lightly filtered for stability.

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create.

*callback* is a function pointer to a function with the prototype:

    void gaze_origin_callback( tobii_gaze_origin_t const* gaze_origin, void* user_data )

This function will be called when there is new gaze origin data available. It is called with the following parameters:

-   *gaze_origin*

    This is a pointer to a struct containing the following data:

    -   *timestamp_us*
        Timestamp value for when the gaze origin was calculated, measured in microseconds (us). The epoch is undefined,
        so these timestamps are only useful for calculating the time elapsed between a pair of values. The function
        tobii_system_clock() can be used to retrieve a timestamp using the same clock and same relative values as this
        timestamp.

    -   *left_validity*
        **TOBII_VALIDITY_INVALID** if the values for the left eye are not valid, **TOBII_VALIDITY_VALID** if they are.

    -   *left_xyz*
        An array of three floats, for the x, y and z coordinate of the gaze origin point on the left eye of the user, as
        measured in millimeters from the center of the display.

    -   *right_validity*
        **TOBII_VALIDITY_INVALID** if the values for the right eye are not valid, **TOBII_VALIDITY_VALID** if they are.

    -   *right_xyz*
        An array of three floats, for the x, y and z coordinate of the gaze origin point on the right eye of the user,
        as measured in millimeters from the center of the display.


-   *user_data*
    This is the custom pointer sent in when registering the callback.

*user_data* custom pointer which will be passed unmodified to the callback.


### Return value

If the operation is successful, tobii_gaze_origin_subscribe returns **TOBII_ERROR_NO_ERROR**. If the call
fails, tobii_gaze_origin_subscribe returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* parameter was passed in as NULL. 

-   **TOBII_ERROR_ALREADY_SUBSCRIBED**

    A subscription for gaze origins were already made. There can only be one callback registered at a time.
    To change to another callback, first call tobii_gaze_origin_unsubscribe().

-   **TOBII_ERROR_NOT_SUPPORTED**

    The device doesn't support the stream. This error is returned if the API is called with an old device and/or that is
    running outdated firmware.

-   **TOBII_ERROR_TOO_MANY_SUBSCRIBERS**

    Too many subscribers for the requested stream. Tobii eye trackers can have a limitation on the number of concurrent
    subscribers to specific streams due to high bandwidth and/or high frequency of the data stream.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_gaze_origin_subscribe from within a callback function is not supported.

### See also

tobii_eye_position_normalized_subscribe(), tobii_gaze_origin_unsubscribe(), tobii_device_process_callbacks(),
tobii_system_clock()


### Example

@code{.c}

    #include <tobii/tobii_streams.h>
    #include <stdio.h>
    #include <assert.h>

    void gaze_origin_callback( tobii_gaze_origin_t const* gaze_origin, void* user_data )
    {
        if( gaze_origin->left_validity == TOBII_VALIDITY_VALID )
            printf( "Left: %f, %f, %f ",
                gaze_origin->left_xyz[ 0 ],
                gaze_origin->left_xyz[ 1 ],
                gaze_origin->left_xyz[ 2 ] );

        if( gaze_origin->right_validity == TOBII_VALIDITY_VALID )
            printf( "Right: %f, %f, %f ",
                gaze_origin->right_xyz[ 0 ],
                gaze_origin->right_xyz[ 1 ],
                gaze_origin->right_xyz[ 2 ] );

        printf( "\n" );
    }

    static void url_receiver( char const* url, void* user_data )
    {
        char* buffer = (char*)user_data;
        if( *buffer != '\0' ) return; // only keep first value

        if( strlen( url ) < 256 )
            strcpy( buffer, url );
    }

    int main()
    {
        tobii_api_t* api;
        tobii_error_t error = tobii_api_create( &api, NULL, NULL );
        assert( error == TOBII_ERROR_NO_ERROR );

        char url[ 256 ] = { 0 };
        error = tobii_enumerate_local_device_urls( api, url_receiver, url );
        assert( error == TOBII_ERROR_NO_ERROR && *url != '\0' );

        tobii_device_t* device;
        error = tobii_device_create( api, url, &device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_gaze_origin_subscribe( device, gaze_origin_callback, 0 );
        assert( error == TOBII_ERROR_NO_ERROR );

        int is_running = 1000; // in this sample, exit after some iterations
        while( --is_running > 0 )
        {
            error = tobii_wait_for_callbacks( NULL, 1, &device );
            assert( error == TOBII_ERROR_NO_ERROR || error == TOBII_ERROR_TIMED_OUT );

            error = tobii_device_process_callbacks( device );
            assert( error == TOBII_ERROR_NO_ERROR );
        }

        error = tobii_gaze_origin_unsubscribe( device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_device_destroy( device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_api_destroy( api );
        assert( error == TOBII_ERROR_NO_ERROR );
        return 0;
    }

@endcode

*/


/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_gaze_origin_unsubscribe( tobii_device_t* device );
@ingroup tobii_streams

tobii_gaze_origin_unsubscribe
-----------------------------

### Function

Stops listening to gaze origin stream that was subscribed to by a call to tobii_gaze_origin_subscribe()


### Syntax

    #include <tobii/tobii_streams.h>
    tobii_error_t tobii_gaze_origin_unsubscribe( tobii_device_t* device );


### Remarks

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create.


### Return value

If the operation is successful, tobii_gaze_origin_unsubscribe returns **TOBII_ERROR_NO_ERROR**. If the call
fails, tobii_gaze_origin_unsubscribe returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* parameter was passed in as NULL. 

-   **TOBII_ERROR_NOT_SUBSCRIBED**

    There was no subscription for gaze origins. It is only valid to call tobii_gaze_origin_unsubscribe()
    after first successfully calling tobii_gaze_origin_subscribe().

-   **TOBII_ERROR_NOT_SUPPORTED**

    The device doesn't support the stream. This error is returned if the API is called with an old device and/or that is
    running outdated firmware.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_gaze_origin_unsubscribe from within a callback function is not supported.

### See also

tobii_gaze_origin_subscribe()


### Example

See tobii_gaze_origin_subscribe()

*/


/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_eye_position_normalized_subscribe( tobii_device_t* device, tobii_eye_position_normalized_callback_t callback, void* user_data );
@ingroup tobii_streams

tobii_eye_position_normalized_subscribe
---------------------------------------

### Function

Start listening for normalized eye position data. Eye position is a point on the users eye, reported in normalized track box
coordinates.


### Syntax

    #include <tobii/tobii_streams.h>
    tobii_error_t tobii_eye_position_normalized_subscribe( tobii_device_t* device,
        tobii_eye_position_normalized_callback_t callback, void* user_data );


### Remarks

This subscription is for receiving the position of the eyes, given in normalized (0 to 1) track box coordinates.
The exact point on the eye varies by device. For example, the center of the pupil or the center of the cornea.
The data is lightly filtered for stability. The track box is a the volume around the user that the device can track within.

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create.

*callback* is a function pointer to a function with the prototype:

    void eye_position_normalized_callback( tobii_eye_position_normalized_t const* eye_position, void* user_data )

This function will be called when there is new normalized eye position data available. It is called with the following parameters:

-   *eye_position*

    This is a pointer to a struct containing the following data:

    -   *timestamp_us*

        Timestamp value for when the gaze origin was calculated, measured in microseconds (us). The epoch is undefined,
        so these timestamps are only useful for calculating the time elapsed between a pair of values. The function
        tobii_system_clock() can be used to retrieve a timestamp using the same clock and same relative values as this
        timestamp.

    -   *left_validity*

        **TOBII_VALIDITY_INVALID** if the values for the left eye are not valid, **TOBII_VALIDITY_VALID** if they are.

    -   *left_xyz*

        An array of three floats, for the x, y and z coordinate of the eye position on the left eye of the user, as
        a normalized value within the track box.

    -   *right_validity*

        **TOBII_VALIDITY_INVALID** if the values for the right eye are not valid, **TOBII_VALIDITY_VALID** if they are.

    -   *right_xyz*

        An array of three floats, for the x, y and z coordinate of the eye position on the right eye of the user,
        as a normalized value within the track box.


-   *user_data* This is the custom pointer sent in when registering the callback.

*user_data* custom pointer which will be passed unmodified to the callback.


### Return value

If the operation is successful, tobii_eye_position_normalized_subscribe returns **TOBII_ERROR_NO_ERROR**. If the call
fails, tobii_eye_position_normalized_subscribe returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* or *callback* parameter were passed in as NULL. 

-   **TOBII_ERROR_ALREADY_SUBSCRIBED**

    A subscription for normalized eye positions were already made. There can only be one callback registered at a time.
    To change to another callback, first call tobii_eye_position_normalized_unsubscribe().

-   **TOBII_ERROR_NOT_SUPPORTED**

    The device doesn't support the stream. This error is returned if the API is called with an old device and/or that is
    running outdated firmware.

-   **TOBII_ERROR_TOO_MANY_SUBSCRIBERS**

    Too many subscribers for the requested stream. Tobii eye trackers can have a limitation on the number of concurrent
    subscribers to specific streams due to high bandwidth and/or high frequency of the data stream.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_eye_position_normalized_subscribe from within a callback function is not supported.

### See also

tobii_gaze_origin_subscribe(), tobii_eye_position_normalized_unsubscribe(), tobii_device_process_callbacks(),
tobii_system_clock()


### Example

@code{.c}

    #include <tobii/tobii_streams.h>
    #include <stdio.h>
    #include <assert.h>

    void eye_position_callback( tobii_eye_position_normalized_t const* eye_pos, void* user_data )
    {
        if( eye_pos->left_validity == TOBII_VALIDITY_VALID )
            printf( "Left: %f, %f, %f ",
                eye_pos->left_xyz[ 0 ],
                eye_pos->left_xyz[ 1 ],
                eye_pos->left_xyz[ 2 ] );

        if( eye_pos->right_validity == TOBII_VALIDITY_VALID )
            printf( "Right: %f, %f, %f ",
                eye_pos->right_xyz[ 0 ],
                eye_pos->right_xyz[ 1 ],
                eye_pos->right_xyz[ 2 ] );

        printf( "\n" );
    }

    static void url_receiver( char const* url, void* user_data )
    {
        char* buffer = (char*)user_data;
        if( *buffer != '\0' ) return; // only keep first value

        if( strlen( url ) < 256 )
            strcpy( buffer, url );
    }

    int main()
    {
        tobii_api_t* api;
        tobii_error_t error = tobii_api_create( &api, NULL, NULL );
        assert( error == TOBII_ERROR_NO_ERROR );

        char url[ 256 ] = { 0 };
        error = tobii_enumerate_local_device_urls( api, url_receiver, url );
        assert( error == TOBII_ERROR_NO_ERROR && *url != '\0' );

        tobii_device_t* device;
        error = tobii_device_create( api, url, &device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_eye_position_normalized_subscribe( device, eye_position_callback, 0 );
        assert( error == TOBII_ERROR_NO_ERROR );

        int is_running = 1000; // in this sample, exit after some iterations
        while( --is_running > 0 )
        {
            error = tobii_wait_for_callbacks( NULL, 1, &device );
            assert( error == TOBII_ERROR_NO_ERROR || error == TOBII_ERROR_TIMED_OUT );

            error = tobii_device_process_callbacks( device );
            assert( error == TOBII_ERROR_NO_ERROR );
        }

        error = tobii_eye_position_normalized_unsubscribe( device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_device_destroy( device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_api_destroy( api );
        assert( error == TOBII_ERROR_NO_ERROR );
        return 0;
    }

@endcode

*/


/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_eye_position_normalized_unsubscribe( tobii_device_t* device );
@ingroup tobii_streams

tobii_eye_position_normalized_unsubscribe
----------------------------------------

### Function

Stops listening to normalized eye position stream that was subscribed to by a call to tobii_eye_position_normalized_subscribe()


### Syntax

    #include <tobii/tobii_streams.h>
    tobii_error_t tobii_eye_position_normalized_unsubscribe( 
        tobii_device_t* device );


### Remarks

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create.


### Return value

If the operation is successful, tobii_eye_position_normalized_unsubscribe returns **TOBII_ERROR_NO_ERROR**. If the call
fails, tobii_eye_position_normalized_unsubscribe returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* parameter was passed in as NULL. 

-   **TOBII_ERROR_NOT_SUBSCRIBED**

    There was no subscription for normalized eye positions. It is only valid to call tobii_eye_position_normalized_unsubscribe()
    after first successfully calling tobii_eye_position_normalized_subscribe().

-   **TOBII_ERROR_NOT_SUPPORTED**

    The device doesn't support the stream. This error is returned if the API is called with an old device and/or that is
    running outdated firmware.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_eye_position_normalized_unsubscribe from within a callback function is not supported.

### See also

tobii_eye_position_normalized_subscribe()


### Example

See tobii_eye_position_normalized_subscribe()

*/


/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_user_presence_subscribe( tobii_device_t* device, tobii_user_presence_callback_t callback, void* user_data );
@ingroup tobii_streams

tobii_user_presence_subscribe
-----------------------------

### Function

Start listening for user presence notifications, reporting whether there is a person in front of the device.


### Syntax

    #include <tobii/tobii_streams.h>
    tobii_error_t tobii_user_presence_subscribe( tobii_device_t* device,
        tobii_user_presence_callback_t callback, void* user_data );


### Remarks

This subscription is for being notified when a user is detected by the device, and when a user is no longer detected.

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create.

*callback* is a function pointer to a function with the prototype:

    void presence_callback( tobii_user_presence_status_t status, int64_t timestamp_us, 
        void* user_data )


This function will be called when there is a change in presence state. It is called with the following parameters:

-   *status*
    One of the following values:

    -   **TOBII_USER_PRESENCE_STATUS_UNKNOWN** if user presence could not be determined.
    -   **TOBII_USER_PRESENCE_STATUS_AWAY** if there is a user in front of the device.
    -   **TOBII_USER_PRESENCE_STATUS_PRESENT** if there is no user in front of the device.


-   *timestamp_us*
    Timestamp value for when the user presence was calculated, measured in microseconds (us). The epoch is undefined,
    so these timestamps are only useful for calculating the time elapsed between a pair of values. The function
    tobii_system_clock() can be used to retrieve a timestamp using the same clock and same relative values as this
    timestamp.

-   *user_data*
    This is the custom pointer sent in when registering the callback.


*user_data* custom pointer which will be passed unmodified to the callback.


### Return value

If the operation is successful, tobii_user_presence_subscribe returns **TOBII_ERROR_NO_ERROR**. If the call
fails, tobii_user_presence_subscribe returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* or *callback* parameter were passed in as NULL. 

-   **TOBII_ERROR_ALREADY_SUBSCRIBED**

    A subscription for presence data was already made. There can only be one callback registered at a time.
    To change to another callback, first call tobii_user_presence_unsubscribe().

-   **TOBII_ERROR_NOT_SUPPORTED**

    The device doesn't support the stream. This error is returned if the API is called with an old device and/or that is
    running outdated firmware.

-   **TOBII_ERROR_TOO_MANY_SUBSCRIBERS**

    Too many subscribers for the requested stream. Tobii eye trackers can have a limitation on the number of concurrent
    subscribers to specific streams due to high bandwidth and/or high frequency of the data stream.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_user_presence_subscribe from within a callback function is not supported.

### See also

tobii_user_presence_unsubscribe(), tobii_device_process_callbacks(), tobii_system_clock()


### Example

@code{.c}

    #include <tobii/tobii_streams.h>
    #include <stdio.h>
    #include <assert.h>

    void presence_callback( tobii_user_presence_status_t status, int64_t timestamp_us, void* user_data )
    {
        switch( status )
        {
            case TOBII_USER_PRESENCE_STATUS_UNKNOWN:
                printf( "User presence status is unknown.\n" );
                break;
            case TOBII_USER_PRESENCE_STATUS_AWAY:
                printf( "User is away.\n" );
                break;
            case TOBII_USER_PRESENCE_STATUS_PRESENT:
                printf( "User is present.\n" );
                break;
        }
    }

    static void url_receiver( char const* url, void* user_data )
    {
        char* buffer = (char*)user_data;
        if( *buffer != '\0' ) return; // only keep first value

        if( strlen( url ) < 256 )
            strcpy( buffer, url );
    }

    int main()
    {
        tobii_api_t* api;
        tobii_error_t error = tobii_api_create( &api, NULL, NULL );
        assert( error == TOBII_ERROR_NO_ERROR );

        char url[ 256 ] = { 0 };
        error = tobii_enumerate_local_device_urls( api, url_receiver, url );
        assert( error == TOBII_ERROR_NO_ERROR && *url != '\0' );

        tobii_device_t* device;
        error = tobii_device_create( api, url, &device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_user_presence_subscribe( device, presence_callback, 0 );
        assert( error == TOBII_ERROR_NO_ERROR );

        int is_running = 1000; // in this sample, exit after some iterations
        while( --is_running > 0 )
        {
            error = tobii_wait_for_callbacks( NULL, 1, &device );
            assert( error == TOBII_ERROR_NO_ERROR || error == TOBII_ERROR_TIMED_OUT );

            error = tobii_device_process_callbacks( device );
            assert( error == TOBII_ERROR_NO_ERROR );
        }

        error = tobii_user_presence_unsubscribe( device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_device_destroy( device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_api_destroy( api );
        assert( error == TOBII_ERROR_NO_ERROR );
        return 0;
    }

@endcode

*/


/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_user_presence_unsubscribe( tobii_device_t* device );
@ingroup tobii_streams

tobii_user_presence_unsubscribe
-------------------------------

### Function

Stops listening to presence stream that was subscribed to by a call to tobii_user_presence_subscribe().


### Syntax

    #include <tobii/tobii_streams.h>
    tobii_error_t tobii_user_presence_unsubscribe( tobii_device_t* device );


### Remarks

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create.


### Return value

If the operation is successful, tobii_user_presence_unsubscribe returns **TOBII_ERROR_NO_ERROR**. If the call
fails, tobii_user_presence_unsubscribe returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* parameter was passed in as NULL.

-   **TOBII_ERROR_NOT_SUBSCRIBED**

    There was no subscription for presence. It is only valid to call tobii_user_presence_unsubscribe()
    after first successfully calling tobii_user_presence_subscribe().

-   **TOBII_ERROR_NOT_SUPPORTED**

    The device doesn't support the stream. This error is returned if the API is called with an old device and/or that is
    running outdated firmware.

-   **TOBII_ERROR_INTERNAL**	

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_user_presence_unsubscribe from within a callback function is not supported.

### See also

tobii_user_presence_subscribe()


### Example

See tobii_user_presence_subscribe()

*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_head_pose_subscribe( tobii_device_t* device, tobii_head_pose_callback_t callback, void* user_data );
@ingroup tobii_streams

tobii_head_pose_subscribe
-------------------------

### Function

Start listening to the head pose stream, which reports the position and rotation of the user's head.


### Syntax

    #include <tobii/tobii_streams.h>
    tobii_error_t TOBII_CALL tobii_head_pose_subscribe( tobii_device_t* device,
        tobii_head_pose_callback_t callback, void* user_data );


### Remarks

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create.

*callback* is a function pointer to a function with the prototype:

    void head_pose_callback( tobii_head_pose_t const* head_pose, void* user_data )

This function will be called when there is new head pose data to be sent to the subscriber. It is called with the
following parameters:

-   *head_pose*

    This is a pointer to a struct containing the following data:

    -   *timestamp_us*

        Timestamp value for when the head pose was calculated, measured in microseconds (us). The epoch is 
        undefined, so these timestamps are only useful for calculating the time elapsed between a pair of values. 
        The function tobii_system_clock() can be used to retrieve a timestamp using the same clock and same relative 
        values as this timestamp.

    -   *position_validity*

        Indicates the validity of the position_xyz field. **TOBII_VALIDITY_INVALID** if the field is not valid, 
        **TOBII_VALIDITY_VALID** if it is.

    -   *position_xyz*

        An array of three floats, for the x, y and z coordinate of the head of the user, as measured in millimeters 
        from the center of the display.

    -   *rotation_validity_xyz*

        An array indicating the validity of each elemnt of the rotation_xyz field. **TOBII_VALIDITY_INVALID** if the 
        element is not valid, **TOBII_VALIDITY_VALID** if it is.

    -   *rotation_xyz*

        An array of three floats, for the x, y and z rotation of the head of the user. The rotation is expressed in 
        Euler angles using right-handed rotations around each axis. The z rotation describes the rotation around the 
        vector pointing towards the user.


-   *user_data*

    This is the custom pointer sent in when registering the callback.

*user_data* custom pointer which will be passed unmodified to the notification callback.


### Return value

If the operation is successful, tobii_head_pose_subscribe returns **TOBII_ERROR_NO_ERROR**. If the call
fails, tobii_head_pose_subscribe returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* or *callback* parameter were passed in as NULL. 

-   **TOBII_ERROR_ALREADY_SUBSCRIBED**

    A subscription for head pose were already made. There can only be one callback registered at a time.
    To change to another callback, first call tobii_head_pose_unsubscribe().

-   **TOBII_ERROR_NOT_SUPPORTED**

    The device doesn't support head pose. This error is returned if the API is called with an old device which
    doesn't support head pose.

-   **TOBII_ERROR_TOO_MANY_SUBSCRIBERS**

    Too many subscribers for the requested stream. Tobii eye trackers can have a limitation on the number of concurrent
    subscribers to specific streams due to high bandwidth and/or high frequency of the data stream.

-   **TOBII_ERROR_NOT_AVAILABLE**

    Head pose is not available as the software component responsible for providing it is not running. Head pose requires
    the Tobii Eye Tracking Core Software to be installed and running.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_head_pose_subscribe from within a callback function is not supported.

### See also

tobii_head_pose_unsubscribe()


### Example

@code{.c}

    #include <tobii/tobii_streams.h>
    #include <stdio.h>
    #include <assert.h>

    void head_pose_callback( tobii_head_pose_t const* head_pose, void* user_data )
    {
        if( head_pose->position_validity == TOBII_VALIDITY_VALID )
            printf( "Position: (%f, %f, %f)\n",
                head_pose->position_xyz[ 0 ],
                head_pose->position_xyz[ 1 ],
                head_pose->position_xyz[ 2 ] );

        printf( "Rotation:\n" );
        for( int i = 0; i < 3; ++i )
            if( head_pose->rotation_validity_xyz[ i ] == TOBII_VALIDITY_VALID )
                printf( "%f\n", head_pose->rotation_xyz[ i ] );
    }

    static void url_receiver( char const* url, void* user_data )
    {
        char* buffer = (char*)user_data;
        if( *buffer != '\0' ) return; // only keep first value

        if( strlen( url ) < 256 )
            strcpy( buffer, url );
    }

    int main()
    {
        tobii_api_t* api;
        tobii_error_t error = tobii_api_create( &api, NULL, NULL );
        assert( error == TOBII_ERROR_NO_ERROR );

        char url[ 256 ] = { 0 };
        error = tobii_enumerate_local_device_urls( api, url_receiver, url );
        assert( error == TOBII_ERROR_NO_ERROR && *url != '\0' );

        tobii_device_t* device;
        error = tobii_device_create( api, url, &device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_head_pose_subscribe( device, head_pose_callback, 0 );
        assert( error == TOBII_ERROR_NO_ERROR );

        int is_running = 1000; // in this sample, exit after some iterations
        while( --is_running > 0 )
        {
            error = tobii_wait_for_callbacks( NULL, 1, &device );
            assert( error == TOBII_ERROR_NO_ERROR || error == TOBII_ERROR_TIMED_OUT );

            error = tobii_device_process_callbacks( device );
            assert( error == TOBII_ERROR_NO_ERROR );
        }

        error = tobii_head_pose_unsubscribe( device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_device_destroy( device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_api_destroy( api );
        assert( error == TOBII_ERROR_NO_ERROR );
        return 0;
    }

@endcode

*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_head_pose_unsubscribe( tobii_device_t* device );
@ingroup tobii_streams

tobii_head_pose_unsubscribe
---------------------------

### Function

Stops listening to the head pose stream that was subscribed to by a call to tobii_head_pose_subscribe().


### Syntax

    #include <tobii/tobii_streams.h>
    tobii_error_t TOBII_CALL tobii_head_pose_unsubscribe( tobii_device_t* device );


### Remarks

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create.


### Return value

If the operation is successful, tobii_head_pose_unsubscribe returns **TOBII_ERROR_NO_ERROR**. If the call
fails, tobii_head_pose_unsubscribe returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* parameter was passed in as NULL. 

-   **TOBII_ERROR_NOT_SUBSCRIBED**

    There was no subscription for head pose. It is only valid to call tobii_head_pose_unsubscribe()
    after first successfully calling tobii_head_pose_subscribe().

-   **TOBII_ERROR_NOT_SUPPORTED**

    The device doesn't support head pose. This error is returned if the API is called with an old device which
    doesn't support head pose.

-   **TOBII_ERROR_NOT_AVAILABLE**

    Head pose is not available as the software component responsible for providing it is not running.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_head_pose_unsubscribe from within a callback function is not supported.

### See also

tobii_head_pose_subscribe()


### Example

See tobii_head_pose_subscribe()

*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_notifications_subscribe( tobii_device_t* device, tobii_notifications_callback_t callback, void* user_data );
@ingroup tobii_streams

tobii_notifications_subscribe
-----------------------------

### Function

Start listening to the notifications stream, which reports state changes for a device.


### Syntax

    #include <tobii/tobii_streams.h>
    tobii_error_t tobii_notifications_subscribe( tobii_device_t* device,
        tobii_notifications_callback_t callback, void* user_data );


### Remarks

As the device is a shared resource, which may be in use by multiple client applications, notifications are used to 
inform when a state change have occured on the device, as an effect of another client performing some operation (such 
as starting a calibration, or changing the display area). 

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create.

*callback* is a function pointer to a function with the prototype:

    void notification_callback( tobii_notification_t const* notification, void* user_data )

This function will be called when there is a new notification to be sent to the subscriber. It is called with the
following parameters:

-   *notification*

    This is a pointer to a struct containing the following data:

    -   *type*
        
        Denotes the type of notification that was received. Can be one of the following values:

        **TOBII_NOTIFICATION_TYPE_CALIBRATION_STATE_CHANGED**
        **TOBII_NOTIFICATION_TYPE_EXCLUSIVE_MODE_STATE_CHANGED**
        **TOBII_NOTIFICATION_TYPE_TRACK_BOX_CHANGED**
        **TOBII_NOTIFICATION_TYPE_DISPLAY_AREA_CHANGED**
        **TOBII_NOTIFICATION_TYPE_FRAMERATE_CHANGED**
        **TOBII_NOTIFICATION_TYPE_POWER_SAVE_STATE_CHANGED**
        **TOBII_NOTIFICATION_TYPE_DEVICE_PAUSED_STATE_CHANGED**
        **TOBII_NOTIFICATION_TYPE_CALIBRATION_ENABLED_EYE_CHANGED**
        **TOBII_NOTIFICATION_TYPE_CALIBRATION_ID_CHANGED**
        **TOBII_NOTIFICATION_TYPE_COMBINED_GAZE_FACTOR_CHANGED**
        **TOBII_NOTIFICATION_TYPE_FAULTS_CHANGED**
        **TOBII_NOTIFICATION_TYPE_WARNINGS_CHANGED**
        **TOBII_NOTIFICATION_TYPE_FACE_TYPE_CHANGED**

    -   *value_type*

        Indicates which of the fields of the *value* union contains the data. Can be one of the following:

        **TOBII_NOTIFICATION_VALUE_TYPE_NONE**
        **TOBII_NOTIFICATION_VALUE_TYPE_FLOAT**
        **TOBII_NOTIFICATION_VALUE_TYPE_STATE**
        **TOBII_NOTIFICATION_VALUE_TYPE_DISPLAY_AREA**
        **TOBII_NOTIFICATION_VALUE_TYPE_UINT**
        **TOBII_NOTIFICATION_VALUE_TYPE_ENABLED_EYE**
        **TOBII_NOTIFICATION_VALUE_TYPE_STRING**

    -   *value*

        The attached data described in *value_type*, which is used to access the corresponding data field. This value 
        is guaranteed to be related to the notification its attached to.


-   *user_data*

    This is the custom pointer sent in when registering the callback.

*user_data* custom pointer which will be passed unmodified to the notification callback.


### Return value

If the operation is successful, tobii_notifications_subscribe returns **TOBII_ERROR_NO_ERROR**. If the call
fails, tobii_notifications_subscribe returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* or *callback* parameters were passed in as NULL. 

-   **TOBII_ERROR_ALREADY_SUBSCRIBED**

    A subscription for notifications were already made. There can only be one callback registered at a time.
    To change to another callback, first call tobii_notifications_unsubscribe().

-   **TOBII_ERROR_TOO_MANY_SUBSCRIBERS**

    Too many subscribers for the requested stream. Tobii eye trackers can have a limitation on the number of concurrent
    subscribers to specific streams due to high bandwidth and/or high frequency of the data stream.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_notifications_subscribe from within a callback function is not supported.

### See also

tobii_notifications_unsubscribe(), tobii_device_process_callbacks()


### Example

@code{.c}

    #include <tobii/tobii_streams.h>
    #include <stdio.h>
    #include <assert.h>

    void notifications_callback( tobii_notification_t const* notification, void* user_data )
    {
        if( notification->type == TOBII_NOTIFICATION_TYPE_CALIBRATION_STATE_CHANGED )
        {
            if( notification->value.state == TOBII_STATE_BOOL_TRUE )
                printf( "Calibration started\n" );
            else
                printf( "Calibration stopped\n" );
        }

        if( notification->type == TOBII_NOTIFICATION_TYPE_FRAMERATE_CHANGED )
            printf( "Framerate changed\nNew framerate: %f\n", notification->value.float_ );
    }

    static void url_receiver( char const* url, void* user_data )
    {
        char* buffer = (char*)user_data;
        if( *buffer != '\0' ) return; // only keep first value

        if( strlen( url ) < 256 )
            strcpy( buffer, url );
    }

    int main()
    {
        tobii_api_t* api;
        tobii_error_t error = tobii_api_create( &api, NULL, NULL );
        assert( error == TOBII_ERROR_NO_ERROR );

        char url[ 256 ] = { 0 };
        error = tobii_enumerate_local_device_urls( api, url_receiver, url );
        assert( error == TOBII_ERROR_NO_ERROR && *url != '\0' );

        tobii_device_t* device;
        error = tobii_device_create( api, url, &device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_notifications_subscribe( device, notifications_callback, 0 );
        assert( error == TOBII_ERROR_NO_ERROR );

        int is_running = 1000; // in this sample, exit after some iterations
        while( --is_running > 0 )
        {
            error = tobii_wait_for_callbacks( NULL, 1, &device );
            assert( error == TOBII_ERROR_NO_ERROR || error == TOBII_ERROR_TIMED_OUT );

            error = tobii_device_process_callbacks( device );
            assert( error == TOBII_ERROR_NO_ERROR );
        }

        error = tobii_notifications_unsubscribe( device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_device_destroy( device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_api_destroy( api );
        assert( error == TOBII_ERROR_NO_ERROR );
        return 0;
    }

@endcode

*/


/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_notifications_unsubscribe( tobii_device_t* device );
@ingroup tobii_streams

tobii_notifications_unsubscribe
-------------------------------

### Function

Stops listening to notifications stream that was subscribed to by a call to tobii_notifications_subscribe()


### Syntax

    #include <tobii/tobii_streams.h>
    tobii_error_t tobii_notifications_unsubscribe( tobii_device_t* device );


### Remarks

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create.


### Return value

If the operation is successful, tobii_notifications_unsubscribe returns **TOBII_ERROR_NO_ERROR**. If the call
fails, tobii_notifications_unsubscribe returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* parameter was passed in as NULL.

-   **TOBII_ERROR_NOT_SUBSCRIBED**

    There was no subscription for notifications. It is only valid to call tobii_notifications_unsubscribe()
    after first successfully calling tobii_notifications_subscribe().

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_notifications_unsubscribe from within a callback function is not supported.

### See also

tobii_notifications_subscribe()


### Example

See tobii_notifications_subscribe()

*/


