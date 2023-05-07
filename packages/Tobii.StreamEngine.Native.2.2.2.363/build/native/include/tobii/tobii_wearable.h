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

#ifndef tobii_wearable_h_included
#define tobii_wearable_h_included

#include "tobii.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tobii_wearable_eye_t
{
    tobii_validity_t gaze_origin_validity;
    float gaze_origin_mm_xyz[ 3 ];

    tobii_validity_t gaze_direction_validity;
    float gaze_direction_normalized_xyz[ 3 ];

    tobii_validity_t pupil_diameter_validity;
    float pupil_diameter_mm;

    tobii_validity_t eye_openness_validity;
    float eye_openness;

    tobii_validity_t pupil_position_in_sensor_area_validity;
    float pupil_position_in_sensor_area_xy[ 2 ];
} tobii_wearable_eye_t;

typedef struct tobii_wearable_data_t
{
    int64_t timestamp_tracker_us;
    int64_t timestamp_system_us;
    uint32_t frame_counter;
    uint32_t led_mode;
    tobii_wearable_eye_t left;
    tobii_wearable_eye_t right;

    tobii_validity_t gaze_origin_combined_validity;
    float gaze_origin_combined_mm_xyz[ 3 ];
    tobii_validity_t gaze_direction_combined_validity;
    float gaze_direction_combined_normalized_xyz[ 3 ];
} tobii_wearable_data_t;

typedef void ( *tobii_wearable_data_callback_t )( tobii_wearable_data_t const* data, void* user_data );
TOBII_API tobii_error_t TOBII_CALL tobii_wearable_data_subscribe( tobii_device_t* device,
    tobii_wearable_data_callback_t callback, void* user_data );

TOBII_API tobii_error_t TOBII_CALL tobii_wearable_data_unsubscribe( tobii_device_t* device );

typedef struct tobii_lens_configuration_t
{
    float left_xyz[ 3 ];
    float right_xyz[ 3 ];
} tobii_lens_configuration_t;

TOBII_API tobii_error_t TOBII_CALL tobii_get_lens_configuration( tobii_device_t* device,
    tobii_lens_configuration_t* lens_config );
TOBII_API tobii_error_t TOBII_CALL tobii_set_lens_configuration( tobii_device_t* device,
    tobii_lens_configuration_t const* lens_config );

typedef enum tobii_lens_configuration_writable_t
{
    TOBII_LENS_CONFIGURATION_NOT_WRITABLE,
    TOBII_LENS_CONFIGURATION_WRITABLE,
} tobii_lens_configuration_writable_t;

TOBII_API tobii_error_t TOBII_CALL tobii_lens_configuration_writable( tobii_device_t* device,
    tobii_lens_configuration_writable_t* writable );

#ifdef __cplusplus
}
#endif

#endif /* tobii_wearable_h_included */

/**
@defgroup tobii_wearable tobii_wearable.h

tobii_wearable.h
===============

tobii_wearable.h contains functions relating to wearable devices, such as VR headsets. It contains a specialized data
stream with different data from the regular streams, as well as functions to retrieve and modify the lens configuration
of the device. 
*/


/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_wearable_data_subscribe( tobii_device_t* device, tobii_wearable_data_callback_t callback, void* user_data );
@ingroup tobii_wearable

tobii_wearable_data_subscribe
-----------------------------

### Function

Start listening for eye tracking data from wearable device, such as VR headsets.


### Syntax

    #include <tobii/tobii_wearable.h>
    tobii_error_t tobii_wearable_data_subscribe( tobii_device_t* device,
        tobii_wearable_data_callback_t callback, void* user_data );


### Remarks

All coordinates are expressed in a right-handed Cartesian system.

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create or 
tobii_device_create_ex.

*callback* is a function pointer to a function with the prototype:

    void wearable_callback( tobii_wearable_data_t const* data, void* user_data )

This function will be called when there is new data available. It is called with the following parameters:

-   *data*
    This is a pointer to a struct containing the data listed below. Note that it is only valid during the callback. Its data
    should be copied if access is necessary at a later stage, from outside the callback.

    -   *timestamp_tracker_us*
        Timestamp value for when the data was captured, measured in microseconds (us). It is generated on the
        device responsible for capturing the data. The epoch is undefined, so these timestamps are only useful for 
        calculating the time elapsed between a pair of values. The value returned in *timestamp_system_us* is 
        calculated from this value. 

    -   *timestamp_system_us*
        Timestamp value for when the data was captured, measured in microseconds (us), and synchronized with the clock of 
        the computer.  The function tobii_system_clock can be used to retrieve a timestamp (at the time of the call) using 
        the same clock and same relative values as this timestamp. The epoch is undefined, so these timestamps are only 
        useful for calculating the time elapsed between a pair of values.

    -   *frame_counter*
        A counter that increments by one each frame. There is no guarantee on its initial value. Will eventually wrap
        around and restart at 0, which may be necessary to detect and handle if comparing the values between frames.

    -   *led_mode*
        A bitmask where each bit (starting from the least significant bit) represents a LED group and whether it is active
        or not, with a value of 1 being active and 0 inactive.

    -   *left*
        This is a struct containing the following data, related to the left eye:

        -   *gaze_origin_validity*
            **TOBII_VALIDITY_INVALID** if *gaze_origin_mm_xyz* is not valid for this frame, **TOBII_VALIDITY_VALID** if it is.

        -   *gaze_origin_mm_xyz*
            An array of three floats, for the x, y and z coordinate of the point in the user's eye from which the calculated
            gaze ray originates, expressed in a right-handed Cartesian coordinate system. See the wearable hardware specification
            for its origin.

        -   *gaze_direction_validity*
            **TOBII_VALIDITY_INVALID** if *gaze_direction_normalized_xyz* for the eye is not valid for this frame,
            **TOBII_VALIDITY_VALID** if it is.

        -   *gaze_direction_normalized_xyz*
            An array of three floats, for the x, y and z coordinate of the gaze direction of the eye of the user, expressed
            as a unit vector in a right-handed Cartesian coordinate system.

        -   *pupil_diameter_validity*
            **TOBII_VALIDITY_INVALID** if *pupil_diameter_mm* is not valid for this frame, **TOBII_VALIDITY_VALID** if it is.

        -   *pupil_diameter_mm*
            A float that represents the approximate diameter of the pupil, expressed in millimeters. Only relative changes
            are guaranteed to be accurate.

        -   *eye_openness_validity*
            **TOBII_VALIDITY_INVALID** if *eye_openess* for the eye is not valid for this frame, **TOBII_VALIDITY_VALID**
            if it is.

        -   *eye_openness*
            A float that represents how open the user's eye is, defined as the ratio between the height of the eye
            divided by its width, making a fully open eye yield a value of approximately 0.5.

        -   *pupil_position_in_sensor_area_validity*
            **TOBII_VALIDITY_INVALID** if *pupil_position_in_sensor_area_xy* is not valid for this frame,
            **TOBII_VALIDITY_VALID** if it is.

        -   *pupil_position_in_sensor_area_xy*
            An array of two floats, for the x and y of the position of the pupil normalized to the sensor area where
            (0, 0): is the top left of sensor area, from the sensor's perspective
            (1, 1): is the bottom right of sensor area, from the sensor's perspective
            In systems where multiple cameras observe both eyes, this signal gives the pupil position in the primary sensor.
            Useful for detecting and visualizing how well the eyes are centered in the sensor images.

    -   *right*
        This is another instance of the same struct as in *left*, but which holds data related to the right eye of the user.

    -   *gaze_origin_combined_validity*
        **TOBII_VALIDITY_INVALID** if *gaze_origin_combined_mm_xyz* is not valid for this frame, **TOBII_VALIDITY_VALID** if it is.

        This field will only be set if you have the capability TOBII_CAPABILITY_COMBINED_GAZE_VR. See tobii_capability_supported().

    -   *gaze_origin_combined_mm_xyz*
        An array of three floats, for the x, y and z coordinate of the point in from which the combined gaze ray originates, expressed 
        in a right-handed Cartesian coordinate system.

        This field will only be set if you have the capability TOBII_CAPABILITY_COMBINED_GAZE_VR. See tobii_capability_supported().

    -   *gaze_direction_combined_validity*
        **TOBII_VALIDITY_INVALID** if *gaze_direction_combined_normalized_xyz* is not valid for this frame, **TOBII_VALIDITY_VALID** if it is.

        This field will only be set if you have the capability TOBII_CAPABILITY_COMBINED_GAZE_VR. See tobii_capability_supported().

    -   *gaze_direction_combined_normalized_xyz*
        An array of three floats, for the x, y and z coordinate of the combined gaze direction of the left and right eye of the user, expressed
        as a unit vector in a right-handed Cartesian coordinate system.

        This field will only be set if you have the capability TOBII_CAPABILITY_COMBINED_GAZE_VR. See tobii_capability_supported().

-   *user_data*
    This is the custom pointer sent in when registering the callback.

*user_data* custom pointer which will be passed unmodified to the callback function.


### Return value

If the operation is successful, tobii_wearable_data_subscribe() returns **TOBII_ERROR_NO_ERROR**. If the call
fails, tobii_wearable_data_subscribe returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    One or more of the *device* and *callback* parameters were passed in as NULL. 

-   **TOBII_ERROR_ALREADY_SUBSCRIBED**

    A subscription for wearable data were already made. There can only be one callback registered at a time.
    To change to another callback, first call tobii_wearable_data_unsubscribe().

-   **TOBII_ERROR_NOT_SUPPORTED**

    The device doesn't support the stream. This error is returned if the API is called with a non-VR device.

-   **TOBII_ERROR_TOO_MANY_SUBSCRIBERS**

    Too many subscribers for the requested stream. Tobii eye trackers can have a limitation on the number of concurrent
    subscribers to specific streams due to high bandwidth and/or high frequency of the data stream.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_wearable_data_subscribe from within a callback function is not supported.

### See also

tobii_wearable_data_unsubscribe(), tobii_device_process_callbacks(), tobii_capability_supported()


### Example

@code{.c}

    #include <tobii/tobii_wearable.h>
    #include <stdio.h>
    #include <assert.h>

    void wearable_callback( tobii_wearable_data_t const* wearable,
        void* user_data )
    {
        if( wearable->left.gaze_direction_validity )
        {
            printf( "Left gaze direction: (%f, %f, %f)\n",
                wearable->left.gaze_direction_normalized_xyz[ 0 ],
                wearable->left.gaze_direction_normalized_xyz[ 1 ],
                wearable->left.gaze_direction_normalized_xyz[ 2 ] );
        }
        else
            printf( "Left gaze direction: INVALID\n" );

        if( wearable->right.gaze_direction_validity )
        {
            printf( "Right gaze direction: (%f, %f, %f)\n",
                wearable->right.gaze_direction_normalized_xyz[ 0 ],
                wearable->right.gaze_direction_normalized_xyz[ 1 ],
                wearable->right.gaze_direction_normalized_xyz[ 2 ] );
        }
        else
            printf( "Right gaze direction: INVALID\n" );
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

        error = tobii_wearable_data_subscribe( device, wearable_callback, 0 );
        assert( error == TOBII_ERROR_NO_ERROR );

        int is_running = 1000; // in this sample, exit after some iterations
        while( --is_running > 0 )
        {
            error = tobii_wait_for_callbacks( NULL, 1, &device );
            assert( error == TOBII_ERROR_NO_ERROR || error == TOBII_ERROR_TIMED_OUT );

            error = tobii_device_process_callbacks( device );
            assert( error == TOBII_ERROR_NO_ERROR );
        }

        error = tobii_wearable_data_unsubscribe( device );
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
@fn TOBII_API tobii_error_t TOBII_CALL tobii_wearable_data_unsubscribe( tobii_device_t* device );
@ingroup tobii_wearable

tobii_wearable_data_unsubscribe
-------------------------------

### Function

Stops listening to the wearable data stream that was subscribed to by a call to tobii_wearable_data_subscribe().


### Syntax

    #include <tobii/tobii_wearable.h>
    tobii_error_t TOBII_CALL tobii_wearable_data_unsubscribe( tobii_device_t* device );


### Remarks

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create or 
tobii_device_create_ex.


### Return value

If the operation is successful, tobii_wearable_data_unsubscribe() returns **TOBII_ERROR_NO_ERROR**. If the call
fails, tobii_wearable_data_unsubscribe returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* parameter was passed in as NULL. 

-   **TOBII_ERROR_NOT_SUBSCRIBED**

    There was no subscription for wearable data. It is only valid to call tobii_wearable_data_unsubscribe()
    after first successfully calling tobii_wearable_data_subscribe().

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
    Calling tobii_wearable_data_unsubscribe from within a callback function is not supported.

### See also

tobii_wearable_data_subscribe()

*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_get_lens_configuration( tobii_device_t* device, tobii_lens_configuration_t* lens_config );
@ingroup tobii_wearable

tobii_get_lens_configuration
----------------------------

### Function

Retrieves the current lens configuration in the tracker.


### Syntax

    #include <tobii/tobii_wearable.h>
    tobii_error_t TOBII_CALL tobii_get_lens_configuration( tobii_device_t* device,
        tobii_lens_configuration_t* lens_config );


### Remarks

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create or 
tobii_device_create_ex.

*lens_config* must be a pointer to a valid tobii_lens_configuration_t. Upon success, it will be populated with the
relevant data. It will remain unmodified upon failure. It is a pointer to a struct containing the following data:

-   *left*
    An array of three floats, for the x, y and z offset of the left lens in the headset, given in millimeters.

-   *right*
    An array of three floats, for the x, y and z offset of the right lens in the headset, given in millimeters.


### Return value

If the operation is successful, tobii_get_lens_configuration() returns **TOBII_ERROR_NO_ERROR**. If the call
fails, tobii_get_lens_configuration returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* or *lens_config* parameter was passed in as NULL. 

-   **TOBII_ERROR_CONNECTION_FAILED**

    The connection to the device was lost. Call tobii_device_reconnect() to re-establish connection.

-   **TOBII_ERROR_NOT_SUPPORTED**

    The device doesn't support this functionality. This error is returned if the API is called with a non-VR device.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_get_lens_configuration from within a callback function is not supported.

### See also

tobii_set_lens_configuration()


### Example
@code{.c}

    #include <tobii/tobii_wearable.h>
    #include <stdio.h>
    #include <assert.h>

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

        tobii_lens_configuration_t lens_config;
        error = tobii_get_lens_configuration( device, &lens_config );
        assert( error == TOBII_ERROR_NO_ERROR );

        printf( "VR lens offset (left): (%f, %f, %f)\n",
            lens_config.left_xyz[ 0 ],
            lens_config.left_xyz[ 1 ],
            lens_config.left_xyz[ 2 ] );

        printf( "VR lens offset (right): (%f, %f, %f)\n",
            lens_config.right_xyz[ 0 ],
            lens_config.right_xyz[ 1 ],
            lens_config.right_xyz[ 2 ] );

        error = tobii_device_destroy( device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_api_destroy( api );
        assert( error == TOBII_ERROR_NO_ERROR );
        return 0;
    }

@endcode

*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_set_lens_configuration( tobii_device_t* device, tobii_lens_configuration_t const* lens_config );
@ingroup tobii_wearable

tobii_set_lens_configuration
----------------------------

### Function

Sets the current lens configuration in the tracker.


### Syntax

    #include <tobii/tobii_wearable.h>
    tobii_error_t TOBII_CALL tobii_set_lens_configuration( tobii_device_t* device,
      tobii_lens_configuration_t const* lens_config );


### Remarks

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create or 
tobii_device_create_ex.

*lens_config* must be a pointer to a valid tobii_lens_configuration_t. Upon success, the values have been written to the
tracker. They should correspond to the physical attributes of the headset that they represent.

-   *left*
    An array of three floats, for the x, y and z offset of the left lens in the headset, given in millimeters.

-   *right*
    An array of three floats, for the x, y and z offset of the right lens in the headset, given in millimeters.


### Return value

If the operation is successful, tobii_get_lens_configuration() returns **TOBII_ERROR_NO_ERROR**. If the call
fails, tobii_get_lens_configuration returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* or *lens_config* parameter was passed in as NULL. 

-   **TOBII_ERROR_INSUFFICIENT_LICENSE**

    The provided license does not permit this operation.

-   **TOBII_ERROR_NOT_SUPPORTED**

    The device doesn't support this functionality. This error is returned if the API is called with a non-VR device.

-   **TOBII_ERROR_CONNECTION_FAILED**

    The connection to the device was lost. Call tobii_device_reconnect() to re-establish connection.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_set_lens_configuration from within a callback function is not supported.


### See also

tobii_get_lens_configuration()


### Example
@code{.c}

    #include <tobii/tobii_wearable.h>
    #include <stdio.h>
    #include <assert.h>

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

        tobii_lens_configuration_writable_t writable;
        error = tobii_lens_configuration_writable( device, &writable );
        assert( error == TOBII_ERROR_NO_ERROR );

        if( writable == TOBII_LENS_CONFIGURATION_WRITABLE )
        {
            tobii_lens_configuration_t lens_config;
            //Add 32 mm offset for each lens on the X-axis
            lens_config.left_xyz[ 0 ] = 32.0;
            lens_config.right_xyz[ 0 ] = -32.0;

            lens_config.left_xyz[ 1 ] = 0.0;
            lens_config.right_xyz[ 1 ] = 0.0;

            lens_config.left_xyz[ 2 ] = 0.0;
            lens_config.right_xyz[ 2 ] = 0.0;

            error = tobii_set_lens_configuration( device, &lens_config );
            assert( error == TOBII_ERROR_NO_ERROR );
        }
        else
            printf( "Unable to write lens configuration to tracker\n" );


        error = tobii_device_destroy( device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_api_destroy( api );
        assert( error == TOBII_ERROR_NO_ERROR );
        return 0;
    }

@endcode

*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_lens_configuration_writable( tobii_device_t* device, tobii_lens_configuration_writable_t* writable );
@ingroup tobii_wearable

tobii_lens_configuration_writable
---------------------------------

### Function

Query the tracker whether it is possible to write a new lens configuration to it or not.


### Syntax

    #include <tobii/tobii_wearable.h>
    tobii_error_t TOBII_CALL tobii_lens_configuration_writable( tobii_device_t* device,
        tobii_lens_configuration_writable_t* writable );


### Remarks

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create or 
tobii_device_create_ex.

*writable* must be a pointer to a valid tobii_lens_configuration_writable_t.

On success, *writable* will be assigned a value that tells whether the tracker can write a new lens configuration.
**TOBII_LENS_CONFIGURATION_WRITABLE** if it is writable and **TOBII_LENS_CONFIGURATION_NOT_WRITABLE** if not.


### Return value

If the operation is successful, tobii_lens_configuration_writable() returns **TOBII_ERROR_NO_ERROR**. If the call
fails, tobii_lens_configuration_writable returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* or *writable* parameter was passed in as NULL. 

-   **TOBII_ERROR_CONNECTION_FAILED**

    The connection to the device was lost. Call tobii_device_reconnect() to re-establish connection.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_lens_configuration_writable from within a callback function is not supported.

### See also

tobii_get_lens_configuration(), tobii_set_lens_configuration()

*/
