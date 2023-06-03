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

#ifndef tobii_engine_h_included
#define tobii_engine_h_included

#include "tobii.h"

#ifdef __cplusplus
extern "C" {
#endif


TOBII_API tobii_error_t TOBII_CALL tobii_engine_create( tobii_api_t* api, tobii_engine_t** engine );

TOBII_API tobii_error_t TOBII_CALL tobii_engine_destroy( tobii_engine_t* engine );

TOBII_API tobii_error_t TOBII_CALL tobii_engine_reconnect( tobii_engine_t* engine );

TOBII_API tobii_error_t TOBII_CALL tobii_engine_process_callbacks( tobii_engine_t* engine );

TOBII_API tobii_error_t TOBII_CALL tobii_engine_clear_callback_buffers( tobii_engine_t* engine );


typedef enum tobii_device_readiness_t
{
    TOBII_DEVICE_READINESS_WAITING_FOR_FIRMWARE_UPGRADE,
    TOBII_DEVICE_READINESS_UPGRADING_FIRMWARE,
    TOBII_DEVICE_READINESS_WAITING_FOR_DISPLAY_AREA,
    TOBII_DEVICE_READINESS_WAITING_FOR_CALIBRATION,
    TOBII_DEVICE_READINESS_CALIBRATING,
    TOBII_DEVICE_READINESS_READY,
    TOBII_DEVICE_READINESS_PAUSED,
    TOBII_DEVICE_READINESS_MALFUNCTIONING,
} tobii_device_readiness_t;

typedef struct tobii_enumerated_device_t
{
    char url[ 256 ];
    char serial_number[ 128 ];
    char model[ 64 ];
    char generation[ 64 ];
    char firmware_version[ 128 ];
    char integration[ 120 ];
    tobii_device_readiness_t readiness;
} tobii_enumerated_device_t;

typedef void( *tobii_enumerated_device_receiver_t )( tobii_enumerated_device_t const* enumerated_device,
    void* user_data );

TOBII_API tobii_error_t TOBII_CALL tobii_enumerate_devices( tobii_engine_t* engine,
    tobii_enumerated_device_receiver_t receiver, void* user_data );

typedef enum tobii_device_list_change_type_t
{
    TOBII_DEVICE_LIST_CHANGE_TYPE_ADDED,
    TOBII_DEVICE_LIST_CHANGE_TYPE_REMOVED,
    TOBII_DEVICE_LIST_CHANGE_TYPE_CHANGED
} tobii_device_list_change_type_t;

typedef void( *tobii_device_list_change_callback_t )( char const* url, tobii_device_list_change_type_t type,
    tobii_device_readiness_t readiness, int64_t timestamp_us, void* user_data );

TOBII_API tobii_error_t TOBII_CALL tobii_device_list_change_subscribe( tobii_engine_t* engine,
    tobii_device_list_change_callback_t callback, void* user_data );
TOBII_API tobii_error_t TOBII_CALL tobii_device_list_change_unsubscribe( tobii_engine_t* engine );

#ifdef __cplusplus
}
#endif

#endif /* tobii_engine_h_included */

/**
@defgroup tobii_engine tobii_engine.h

tobii_engine.h
===============

The tobii_engine.h header file is used for acquiring information about connected devices. It creates a dedicated
connection to the tobii engine, which runs as a service/daemon depending on the operating system. Using this interface
the user can subscribe to events about added and removed tracker devices, as well as changes in individual tracker
readiness states. The readiness states gives the user information about which readiness state the tracker is currently
in, i.e. if a firmware upgrade is in progress, a calibration is needed or if the tracker is ready for use.

Please note that there can only be one subscription callback registered to a stream at a time,
i.e the tobii_device_list_change_subscribe. To register a new callback, first unsubscribe from the stream, then
resubscribe with the new callback function.

Do NOT call StreamEngine API functions from within the callback functions, due to risk of internal deadlocks. Generally
one should finish the callback functions as quickly as possible and not make any blocking calls.
*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_engine_create( tobii_api_t* api, tobii_engine_t** engine );
@ingroup tobii_engine

tobii_engine_create
--------------------------

### Function

Creates a device instance to be used for communicating with a specific device.


### Syntax

    #include <tobii/tobii_engine.h>
    tobii_error_t TOBII_CALL tobii_engine_create( tobii_api_t* api, tobii_engine_t** engine );


### Remarks

Stream engine establishes a communication channel to the engine and keeps track of internal states, tobii_engine_create
allocates and initializes this state and establishes the connection. This connection can then be used to query the
engine for more information about which trackers are connected to the system.

*api* must be a pointer to a valid tobii_api_t as created by calling tobii_api_create.

*engine* must be a pointer to a variable of the type `tobii_engine_t*` that is, a pointer to a
tobii_engine_t-pointer. This variable will be filled in with a pointer to the created engine instance.
tobii_engine_t is an opaque type.


### Return value

If the engine is successfully created, tobii_engine_create returns **TOBII_ERROR_NO_ERROR**. If the call fails,
tobii_engine_create returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *api* or *engine* parameters were passed in as NULL.

-   **TOBII_ERROR_ALLOCATION_FAILED**

    The internal call to malloc or to the custom memory allocator (if used) returned NULL, so engine creation failed.

-   **TOBII_ERROR_NOT_AVAILABLE**

    A connection to the tobii engine could not be established. This error is returned if the tobii engine is not
    running, not installed on the system or if it is an older version of tobii engine.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support


### See also

tobii_engine_destroy(), tobii_api_create()


### Example

@code{.c}

    #include <tobii/tobii.h>
    #include <tobii/tobii_engine.h>
    #include <stdio.h>
    #include <assert.h>

    void device_list_change_callback( char const* url, tobii_device_list_change_type_t type,
        tobii_device_readiness_t readiness, int64_t timestamp_us, void* user_data )
    {
        (void)readiness; (void)timestamp_us; (void) user_data;
        switch( type )
        {
            case TOBII_DEVICE_LIST_CHANGE_TYPE_ADDED:
                printf( "A device with url:%s, has been added\n", url );
                break;
            case TOBII_DEVICE_LIST_CHANGE_TYPE_REMOVED:
                printf( "The device with url:%s, has been removed\n", url );
                break;
            case TOBII_DEVICE_LIST_CHANGE_TYPE_CHANGED:
                printf( "Readiness state changed for device with url:%s\n", url );
                break;
            default:
                printf( "Unknow device list change type\n" );
                break;
        }
    }

    int main()
    {
        tobii_api_t* api;
        tobii_error_t error = tobii_api_create( &api, NULL, NULL );
        assert( error == TOBII_ERROR_NO_ERROR );

        tobii_engine_t* engine;
        error = tobii_engine_create( api, &engine );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_device_list_change_subscribe( engine, device_list_change_callback, 0 );
        assert( error == TOBII_ERROR_NO_ERROR );

        int is_running = 1000; // in this sample, exit after some iterations
        while( --is_running > 0 )
        {
            error = tobii_wait_for_callbacks( engine, 0, NULL );
            assert( error == TOBII_ERROR_NO_ERROR || error == TOBII_ERROR_TIMED_OUT );

            error = tobii_engine_process_callbacks( engine );
            assert( error == TOBII_ERROR_NO_ERROR );
        }

        error = tobii_device_list_change_unsubscribe( engine );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_engine_destroy( engine );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_api_destroy( api );
        assert( error == TOBII_ERROR_NO_ERROR );
        return 0;
    }

@endcode

*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_engine_destroy( tobii_engine_t* engine );
@ingroup tobii_engine

tobii_engine_destroy
----------------------------

### Function

Destroy an engine previously created through a call to tobii_engine_create.


### Syntax

    #include <tobii/tobii_engine.h>
    tobii_error_t tobii_engine_destroy( tobii_engine_t* engine );


### Remarks

tobii_engine_destroy disconnects from the engine, perform cleanup and free the memory allocated by calling
tobii_engine_create.

**NOTE:** Make sure that no background thread is using the engine, for example in the thread calling
tobii_engine_process_callbacks, before calling tobii_engine_destroy in order to avoid the risk of encountering undefined
behavior.

*engine* must be a pointer to a valid tobii_engine_t instance as created by calling tobii_engine_create or
tobii_engine_create_ex.


### Return value

If the engine was successfully destroyed, tobii_engine_destroy returns **TOBII_ERROR_NO_ERROR**. If the call fails,
tobii_engine_destroy returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *engine* parameter was passed in as NULL.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.


### See also

tobii_engine_create()


### Example

See tobii_engine_create()

*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_engine_reconnect( tobii_engine_t* engine );
@ingroup tobii_engine

tobii_engine_reconnect
---------------

### Function

Establish a new connection after a disconnect.


### Syntax

    #include <tobii/tobii_engine.h>
    tobii_error_t tobii_engine_reconnect( tobii_engine_t* engine );


### Remarks

When receiving the error code TOBII_ERROR_CONNECTION_FAILED, it is necessary to explicitly request reconnection, by
calling tobii_engine_reconnect.

*engine* must be a pointer to a valid tobii_engine_t instance as created by calling tobii_engine_create.


### Return value

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *engine* parameter was passed in as NULL.

-   **TOBII_ERROR_CONNECTION_FAILED**

    When attempting to reconnect, a connection could not be established. You might want to wait for a bit and try again,
    for a few times, and if the problem persists, display a message for the user.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.


### See also

tobii_engine_process_callbacks()


### Example

See tobii_engine_process_callbacks()

*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_engine_process_callbacks( tobii_engine_t* engine );
@ingroup tobii_engine

tobii_engine_process_callbacks
------------------------------

### Function

Receives data packages from the engine, and sends the data through any registered callbacks.


### Syntax

    #include <tobii/tobii_engine.h>
    tobii_error_t tobii_engine_process_callbacks( tobii_engine_t* engine );


### Remarks

Stream engine does not do any kind of background processing, it doesn't start any threads. It doesn't use any
asynchronous callbacks. This means that in order to receive data from the engine, the application needs to manually
request the callbacks to happen synchronously, and this is done by calling tobii_engine_process_callbacks.

tobii_engine_process_callbacks will receive any data packages that are incoming from the engine, process them and call any
subscribed callbacks with the data. No callbacks will be called outside of tobii_engine_process_callbacks, so the application
have full control over when to receive callbacks.

tobii_engine_process_callbacks will not wait for data, and will early-out if there's nothing to process. In order to maintain
the connection to the engine, tobii_engine_process_callbacks should be called at least 10 times per second.

The recommended way to use tobii_engine_process_callbacks, is to start a dedicated thread, and alternately call
tobii_wait_for_callbacks and tobii_engine_process_callbacks. See tobii_wait_for_callbacks() for more details.

If there is already a suitable thread to regularly run tobii_engine_process_callbacks from (possibly interleaved with
application specific operations), it is possible to do this without calling tobii_wait_for_callbacks(). In this
scenario, time synchronization needs to be handled manually or the timestamps will start drifting. See
tobii_update_timesync() for more details.

*engine* must be a pointer to a valid tobii_engine_t instance as created by calling tobii_engine_create.

### Return value

If the operation is successful, tobii_engine_process_callbacks returns **TOBII_ERROR_NO_ERROR**. If the call fails,
tobii_engine_process_callbacks returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *engine* parameter was passed in as NULL.

-   **TOBII_ERROR_ALLOCATION_FAILED**

    The internal call to malloc or to the custom memory allocator (if used) returned NULL, so engine creation failed.

-   **TOBII_ERROR_CONNECTION_FAILED**

    The connection to the tobii engine was lost. Call tobii_engine_reconnect() to re-establish connection.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.


### See also

tobii_wait_for_callbacks(), tobii_engine_clear_callback_buffers(), tobii_engine_reconnect(), tobii_update_timesync()


### Example

@code{.c}

    #include <tobii/tobii.h>
    #include <tobii/tobii_engine.h>
    #include <stdio.h>
    #include <assert.h>

    int main()
    {
        tobii_api_t* api;
        tobii_error_t error = tobii_api_create( &api, NULL, NULL );
        assert( error == TOBII_ERROR_NO_ERROR );

        tobii_engine_t* engine;
        error = tobii_engine_create( api, &engine );
        assert( error == TOBII_ERROR_NO_ERROR );

        int is_running = 1000; // in this sample, exit after some iterations
        while( --is_running > 0 )
        {
            // other parts of main loop would be executed here

            error = tobii_engine_process_callbacks( engine );
            assert( error == TOBII_ERROR_NO_ERROR );
        }

        error = tobii_engine_destroy( engine );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_api_destroy( api );
        assert( error == TOBII_ERROR_NO_ERROR );

        return 0;
    }

@endcode

*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_engine_clear_callback_buffers( tobii_engine_t* engine );
@ingroup tobii_engine

tobii_engine_clear_callback_buffers
----------------------------

### Function

Removes all unprocessed entries from the callback queues.


### Syntax

    #include <tobii/tobii_engine.h>
    tobii_error_t tobii_engine_clear_callback_buffers( tobii_engine_t* engine );


### Remarks

All the data that is received and processed are written into internal buffers used for the callbacks. In some
circumstances, for example during initialization, you might want to discard any data that has been buffered but not
processed, without having to destroy/recreate the engine, and without having to implement the filtering out of unwanted
data. tobii_engine_clear_callback_buffers will clear all buffered data, and only data arriving *after* the call to
tobii_engine_clear_callback_buffers will be forwarded to callbacks.

*engine* must be a pointer to a valid tobii_engine_t instance as created by calling tobii_engine_create.


### Return value

If the operation is successful, tobii_engine_clear_callback_buffers returns **TOBII_ERROR_NO_ERROR**. If the call fails,
tobii_engine_clear_callback_buffers returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *engine* parameter was passed in as NULL.


### See also

tobii_wait_for_callbacks(), tobii_engine_process_callbacks()

*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_enumerate_devices( tobii_engine_t* engine, tobii_enumerated_device_receiver_t receiver, void* user_data );
@ingroup tobii_engine

tobii_enumerate_devices
---------------------------------

### Function

Retrieves information about trackers that are currently connected to the system.


### Syntax

    #include <tobii/tobii_engine.h>
    tobii_error_t tobii_enumerate_devices( tobii_engine_t* engine,
        tobii_enumerated_device_receiver_t receiver, void* user_data );


### Remarks

A system might have multiple devices connected, which the stream engine is able to communicate with.
tobii_enumerate_devices retrives a list of all such devices found and their respective readiness state.
It will only list locally connected devices, not devices connected on the network.

*engine* must be a pointer to a valid tobii_engine_t instance as created by calling tobii_engine_create.

*receiver* is a function pointer to a function with the prototype:

    void enumerated_device_receiver( tobii_enumerated_device_t const* enumerated_device, void* user_data );

This function will be called for each device found during enumeration. It is called with the following parameters:

-   *enumerated_devices*
    This is a pointer to a struct containing the following data:

    -   *url*
        The URL for the device, zero terminated ASCII string.

    -   *serial_number*
        The serial number of the device, zero terminated ASCII string.

    -   *model*
        The model of the device, zero terminated ASCII string.

    -   *generation*
        The generation of the device, zero terminated ASCII string.

    -   *firmware_version*
        The firmware version of the device, zero terminated ASCII string.

    -   *integration*
        The integration type of the device, zero terminated ASCII string.

    -   *readiness* is one of the enum values in tobii_device_readiness_t:

        -   **TOBII_DEVICE_READINESS_WAITING_FOR_FIRMWARE_UPGRADE**

            Is the device waiting for a firmware upgrade.

        -   **TOBII_DEVICE_READINESS_UPGRADING_FIRMWARE**

            Is the device in the process of upgrading firmware.

        -   **TOBII_DEVICE_READINESS_WAITING_FOR_DISPLAY_AREA**

            Is the device waiting for display area information, this requires the client to set the display area.

        -   **TOBII_DEVICE_READINESS_WAITING_FOR_CALIBRATION**

            Is the device waiting for a valid calibration, this requires the client to perform or set a calibration.

        -   **TOBII_DEVICE_READINESS_CALIBRATING**

            Is the device in the process of calibrating.

        -   **TOBII_DEVICE_READINESS_READY**

            Is the device ready for use by the client.

        -   **TOBII_DEVICE_READINESS_PAUSED**

            Is the device in a paused state, the tracker is disabled while in the paused state.

        -   **TOBII_DEVICE_READINESS_MALFUNCTIONING**

            Is the device in a malfunctioning state, the tracker can not be used when in this state.

-   *user_data*
    This is the custom pointer sent in to tobii_enumerate_devices.

*user_data* custom pointer which will be passed unmodified to the receiver function.


### Return value

If the operation is successful, tobii_enumerate_devices returns **TOBII_ERROR_NO_ERROR**. If the call fails,
tobii_enumerate_devices returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *api* or *receiver* parameters has been passed in as NULL.

-   **TOBII_ERROR_NOT_SUPPORTED**

    The tobii engine doesn't support the ability to list enumerated devices. This error is returned if the API is
    called with an old tobii engine.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.


### See also

tobii_engine_create(), tobii_device_list_change_subscribe()


### Example

@code{.c}

    #include <tobii/tobii.h>
    #include <tobii/tobii_engine.h>
    #include <stdio.h>
    #include <assert.h>

    void enumerated_device_receiver( tobii_enumerated_device_t const* enumerated_device, void* user_data )
    {
        int* count = (int*) user_data;
        ++(*count);
        printf( "%d. %s (%s)\n", *count, enumerated_device->model, enumerated_device->url );
    }

    int main()
    {
        tobii_api_t* api;
        tobii_error_t error = tobii_api_create( &api, NULL, NULL );
        assert( error == TOBII_ERROR_NO_ERROR );

        tobii_engine_t* engine;
        error = tobii_engine_create( api, &engine );
        assert( error == TOBII_ERROR_NO_ERROR );

        int count = 0;
        error = tobii_enumerate_devices( engine, enumerated_device_receiver, &count );
        if( error == TOBII_ERROR_NO_ERROR )
            printf( "Found %d devices.\n", count );
        else
            printf( "Enumeration failed.\n" );

        error = tobii_engine_destroy( engine );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_api_destroy( api );
        assert( error == TOBII_ERROR_NO_ERROR );

        return 0;
    }

@endcode
*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_device_list_change_subscribe( tobii_engine_t* engine, tobii_device_list_change_callback_t callback, void* user_data );
@ingroup tobii_engine

tobii_device_list_change_subscribe
--------------------------

### Function

Start listening for connected device events.


### Syntax

    #include <tobii/tobii_engine.h>
    tobii_error_t tobii_device_list_change_subscribe( tobii_engine_t* engine,
        tobii_device_list_change_callback_t callback, void* user_data );


### Remarks

This subscription is for receiving information regarding state changes for connected devices. A device list change
event is triggered when a device has been added, removed or the readiness state of a device have changed.

*engine* must be a pointer to a valid tobii_engine_t instance as created by calling tobii_engine_create.

*callback* is a function pointer to a function with the prototype:

    void tobii_device_list_change_callback( char const* url, tobii_device_list_change_type_t type,
        tobii_device_readiness_t readiness, int64_t timestamp_us, void* user_data );

This function will be called when there is a change to the device list. It is called with the following parameters:

-   *url*
    The URL string for the device, zero terminated. This pointer will be invalid after returning from the function,
    so ensure you make a copy of the string rather than storing the pointer directly.

-   *type* is one of the enum values in tobii_device_list_change_type_t:

    -   **TOBII_DEVICE_LIST_CHANGE_TYPE_ADDED**

        The device has been added to the system.

    -   **TOBII_DEVICE_LIST_CHANGE_TYPE_REMOVED**

        The device has been removed from the system.

    -   **TOBII_DEVICE_LIST_CHANGE_TYPE_CHANGED**

        The device readiness state has changed.

-   *readiness* is one of the enum values in tobii_device_readiness_t:

    -   **TOBII_DEVICE_READINESS_WAITING_FOR_FIRMWARE_UPGRADE**

        Is the device waiting for a firmware upgrade.

    -   **TOBII_DEVICE_READINESS_UPGRADING_FIRMWARE**

        Is the device in the process of upgrading firmware.

    -   **TOBII_DEVICE_READINESS_WAITING_FOR_DISPLAY_AREA**

        Is the device waiting for display area information, this requires the client to set the display area.

    -   **TOBII_DEVICE_READINESS_WAITING_FOR_CALIBRATION**

        Is the device waiting for a valid calibration, this requires the client to perform or set a calibration.

    -   **TOBII_DEVICE_READINESS_CALIBRATING**

        Is the device in the process of calibrating.

    -   **TOBII_DEVICE_READINESS_READY**

        Is the device ready for use by the client.

    -   **TOBII_DEVICE_READINESS_PAUSED**

        Is the device in a paused state, the tracker is disabled while in the paused state.

    -   **TOBII_DEVICE_READINESS_MALFUNCTIONING**

        Is the device in a malfunctioning state, the tracker can not be used when in this state.

-   *timestamp_us*
    Timestamp value for when the device list change event occurred, measured in microseconds (us). The epoch is
    undefined, so these timestamps are only useful for calculating the time elapsed between a pair of values. The
    function tobii_system_clock() can be used to retrieve a timestamp using the same clock and same relative values
    as this timestamp.

-   *user_data*
    This is the custom pointer sent in when registering the callback.

*user_data* custom pointer which will be passed unmodified to the callback.


### Return value

If the operation is successful, tobii_device_list_change_subscribe returns **TOBII_ERROR_NO_ERROR**. If the call
fails, tobii_device_list_change_subscribe returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *engine* or *callback* parameters were passed in as NULL.

-   **TOBII_ERROR_ALREADY_SUBSCRIBED**

    A subscription for device list changes were already made. There can only be one callback registered at a time.
    To change to another callback, first call tobii_device_list_change_unsubscribe().

-   **TOBII_ERROR_NOT_SUPPORTED**

    The tobii engine doesn't support the stream. This error is returned if the API is called with an old tobii engine.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support


### See also

tobii_device_list_change_unsubscribe(), tobii_engine_process_callbacks(), tobii_system_clock()


### Example

See tobii_engine_create()

*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_device_list_change_unsubscribe( tobii_engine_t* engine );
@ingroup tobii_engine

tobii_device_list_change_unsubscribe
----------------------------

### Function

Stops listening to gaze point stream that was subscribed to by a call to tobii_gaze_point_subscribe()


### Syntax

    #include <tobii/tobii_engine.h>
    tobii_error_t tobii_device_list_change_unsubscribe( tobii_engine_t* engine );


### Remarks

*engine* must be a pointer to a valid tobii_engine_t instance as created by calling tobii_engine_create.


### Return value

If the operation is successful, tobii_device_list_change_unsubscribe returns **TOBII_ERROR_NO_ERROR**. If the call
fails, tobii_device_list_change_unsubscribe returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *engine* parameter was passed in as NULL.

-   **TOBII_ERROR_NOT_SUBSCRIBED**

    There was no subscription for device list changes. It is only valid to call tobii_device_list_change_unsubscribe()
    after first successfully calling tobii_device_list_change_subscribe().

-   **TOBII_ERROR_NOT_SUPPORTED**

    The tobii engine doesn't support the stream. This error is returned if the API is called with an old tobii engine.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support


### See also

tobii_gaze_point_subscribe()


### Example

See tobii_engine_create()

*/