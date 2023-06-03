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

#ifndef tobii_h_included
#define tobii_h_included

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
    #define TOBII_CALL __cdecl
    #ifdef TOBII_STATIC_LIB
        #define TOBII_API
    #else
        #ifdef TOBII_EXPORTING
            #define TOBII_API __declspec( dllexport )
        #else
            #define TOBII_API __declspec( dllimport )
        #endif /* TOBII_EXPORTING */
    #endif /* TOBII_STATIC_LIB */
#elif __GNUC__ >= 4
    #define TOBII_API __attribute__((visibility("default")))
    #define TOBII_CALL
#else
    #define TOBII_API
    #define TOBII_CALL
#endif /* _WIN32 */


typedef enum tobii_error_t
{
    TOBII_ERROR_NO_ERROR,
    TOBII_ERROR_INTERNAL,
    TOBII_ERROR_INSUFFICIENT_LICENSE,
    TOBII_ERROR_NOT_SUPPORTED,
    TOBII_ERROR_NOT_AVAILABLE,
    TOBII_ERROR_CONNECTION_FAILED,
    TOBII_ERROR_TIMED_OUT,
    TOBII_ERROR_ALLOCATION_FAILED,
    TOBII_ERROR_INVALID_PARAMETER,
    TOBII_ERROR_CALIBRATION_ALREADY_STARTED,
    TOBII_ERROR_CALIBRATION_NOT_STARTED,
    TOBII_ERROR_ALREADY_SUBSCRIBED,
    TOBII_ERROR_NOT_SUBSCRIBED,
    TOBII_ERROR_OPERATION_FAILED,
    TOBII_ERROR_CONFLICTING_API_INSTANCES,
    TOBII_ERROR_CALIBRATION_BUSY,
    TOBII_ERROR_CALLBACK_IN_PROGRESS,
    TOBII_ERROR_TOO_MANY_SUBSCRIBERS
} tobii_error_t;

TOBII_API char const* TOBII_CALL tobii_error_message( tobii_error_t error );


typedef struct tobii_version_t
{
    int major;
    int minor;
    int revision;
    int build;
} tobii_version_t;

TOBII_API tobii_error_t TOBII_CALL tobii_get_api_version( tobii_version_t* version );


typedef enum tobii_log_level_t
{
    TOBII_LOG_LEVEL_ERROR,
    TOBII_LOG_LEVEL_WARN,
    TOBII_LOG_LEVEL_INFO,
    TOBII_LOG_LEVEL_DEBUG,
    TOBII_LOG_LEVEL_TRACE,
} tobii_log_level_t;

typedef void (*tobii_log_func_t)( void* log_context, tobii_log_level_t level, char const* text );

typedef struct tobii_custom_log_t
{
    void* log_context;
    tobii_log_func_t log_func;
} tobii_custom_log_t;


typedef void* (*tobii_malloc_func_t)( void* mem_context, size_t size );
typedef void (*tobii_free_func_t)( void* mem_context, void* ptr );

typedef struct tobii_custom_alloc_t
{
    void* mem_context;
    tobii_malloc_func_t malloc_func;
    tobii_free_func_t free_func;
} tobii_custom_alloc_t;

typedef struct tobii_api_t tobii_api_t;

TOBII_API tobii_error_t TOBII_CALL tobii_api_create(tobii_api_t** api,
    tobii_custom_alloc_t const* custom_alloc, tobii_custom_log_t const* custom_log );

TOBII_API tobii_error_t TOBII_CALL tobii_api_destroy( tobii_api_t* api );

TOBII_API tobii_error_t TOBII_CALL tobii_system_clock( tobii_api_t* api, int64_t* timestamp_us );

typedef void( *tobii_device_url_receiver_t )( char const* url, void* user_data );
TOBII_API tobii_error_t TOBII_CALL tobii_enumerate_local_device_urls( tobii_api_t* api,
    tobii_device_url_receiver_t receiver, void* user_data );

#define TOBII_DEVICE_GENERATION_G5 0x00000002
#define TOBII_DEVICE_GENERATION_IS3 0x00000004
#define TOBII_DEVICE_GENERATION_IS4 0x00000008

TOBII_API tobii_error_t TOBII_CALL tobii_enumerate_local_device_urls_ex( tobii_api_t* api,
    tobii_device_url_receiver_t receiver, void* user_data, uint32_t device_generations );


typedef struct tobii_engine_t tobii_engine_t;
typedef struct tobii_device_t tobii_device_t;

TOBII_API tobii_error_t TOBII_CALL tobii_wait_for_callbacks( tobii_engine_t* engine, int device_count, 
    tobii_device_t* const* devices );


TOBII_API tobii_error_t TOBII_CALL tobii_device_create( tobii_api_t* api, char const* url, tobii_device_t** device );

TOBII_API tobii_error_t TOBII_CALL tobii_device_destroy( tobii_device_t* device );

TOBII_API tobii_error_t TOBII_CALL tobii_device_reconnect( tobii_device_t* device );

TOBII_API tobii_error_t TOBII_CALL tobii_device_process_callbacks( tobii_device_t* device );

TOBII_API tobii_error_t TOBII_CALL tobii_device_clear_callback_buffers( tobii_device_t* device );


TOBII_API tobii_error_t TOBII_CALL tobii_update_timesync( tobii_device_t* device );


typedef struct tobii_device_info_t
{
    char serial_number[ 128 ];
    char model[ 64];
    char generation[ 64 ];
    char firmware_version[ 128];
} tobii_device_info_t;

TOBII_API tobii_error_t TOBII_CALL tobii_get_device_info( tobii_device_t* device,
    tobii_device_info_t* device_info );


typedef struct tobii_track_box_t
{
    float front_upper_right_xyz[ 3 ];
    float front_upper_left_xyz[ 3 ];
    float front_lower_left_xyz[ 3 ];
    float front_lower_right_xyz[ 3 ];
    float back_upper_right_xyz[ 3 ];
    float back_upper_left_xyz[ 3 ];
    float back_lower_left_xyz[ 3 ];
    float back_lower_right_xyz[ 3 ];
} tobii_track_box_t;

TOBII_API tobii_error_t TOBII_CALL tobii_get_track_box( tobii_device_t* device,
    tobii_track_box_t* track_box );


typedef enum tobii_state_t
{
    TOBII_STATE_POWER_SAVE_ACTIVE,
    TOBII_STATE_REMOTE_WAKE_ACTIVE,
    TOBII_STATE_DEVICE_PAUSED,
    TOBII_STATE_EXCLUSIVE_MODE,
    TOBII_STATE_FAULT,
    TOBII_STATE_WARNING,
    TOBII_STATE_CALIBRATION_ID,
    TOBII_STATE_CALIBRATION_ACTIVE,
} tobii_state_t;

typedef enum tobii_state_bool_t
{
    TOBII_STATE_BOOL_FALSE,
    TOBII_STATE_BOOL_TRUE,
} tobii_state_bool_t;

TOBII_API tobii_error_t TOBII_CALL tobii_get_state_bool( tobii_device_t* device, tobii_state_t state,
    tobii_state_bool_t* value );

TOBII_API tobii_error_t TOBII_CALL tobii_get_state_uint32( tobii_device_t* device, tobii_state_t state,
    uint32_t* value );

typedef char tobii_state_string_t[512];
TOBII_API tobii_error_t TOBII_CALL tobii_get_state_string( tobii_device_t* device, tobii_state_t state,
    tobii_state_string_t value );

typedef enum tobii_supported_t
{
    TOBII_NOT_SUPPORTED,
    TOBII_SUPPORTED,
} tobii_supported_t;

typedef enum tobii_capability_t
{
    TOBII_CAPABILITY_DISPLAY_AREA_WRITABLE,
    TOBII_CAPABILITY_CALIBRATION_2D,
    TOBII_CAPABILITY_CALIBRATION_3D,
    TOBII_CAPABILITY_PERSISTENT_STORAGE,
    TOBII_CAPABILITY_CALIBRATION_PER_EYE,
    TOBII_CAPABILITY_COMBINED_GAZE_VR,
    TOBII_CAPABILITY_FACE_TYPE,
} tobii_capability_t;

TOBII_API tobii_error_t TOBII_CALL tobii_capability_supported( tobii_device_t* device,
    tobii_capability_t capability, tobii_supported_t* supported );


typedef enum tobii_stream_t
{
    TOBII_STREAM_GAZE_POINT,
    TOBII_STREAM_GAZE_ORIGIN,
    TOBII_STREAM_EYE_POSITION_NORMALIZED,
    TOBII_STREAM_USER_PRESENCE,
    TOBII_STREAM_HEAD_POSE,
    TOBII_STREAM_WEARABLE,
    TOBII_STREAM_GAZE_DATA,
    TOBII_STREAM_DIGITAL_SYNCPORT,
    TOBII_STREAM_DIAGNOSTICS_IMAGE,
} tobii_stream_t;

TOBII_API tobii_error_t TOBII_CALL tobii_stream_supported( tobii_device_t* device, tobii_stream_t stream, 
    tobii_supported_t* supported );


typedef void (*tobii_data_receiver_t)( void const* data, size_t size, void* user_data );


typedef enum tobii_validity_t
{
    TOBII_VALIDITY_INVALID,
    TOBII_VALIDITY_VALID
} tobii_validity_t;


typedef struct tobii_display_area_t
{
    float top_left_mm_xyz[ 3 ];
    float top_right_mm_xyz[ 3 ];
    float bottom_left_mm_xyz[ 3 ];
} tobii_display_area_t;


typedef enum tobii_enabled_eye_t
{
    TOBII_ENABLED_EYE_LEFT,
    TOBII_ENABLED_EYE_RIGHT,
    TOBII_ENABLED_EYE_BOTH,
} tobii_enabled_eye_t;


#ifdef __cplusplus
}
#endif

#endif /* tobii_h_included */


/**
@defgroup tobii tobii.h

tobii.h
=======

The tobii.h header file collects the core API functions of stream engine. It contains functions to initialize the API
and establish a connection to a tracker, as well as enumerating connected devices and requesting callbacks for 
subscriptions. There are also functions for querying the current state of a tracker, and to query its capabilities.

The API documentation includes example code snippets that shows the use of each function, they don't necessarily
describe the best practice in which to use the api. For a more in-depth example of the best practices, see the samples
that are supplied together with the stream engine library.


### Thread safety

The Tobii Stream Engine API implements full thread safety across all API functions. However, it is up to the user to guarantee
thread safety in code injected into Stream Engine, for example inside callbacks or if a custom memory allocator is supplied.
It is not allowed to call Stream Engine API functions from within a callback invoked by stream engine. Attempting to do so 
will result in TOBII_ERROR_CALLBACK_IN_PROGRESS. A specific exception to this is tobii_system_clock() which specifically *is*
allowed to be called even from within a callback function.

In the *samples* folder, you can find complete examples on how to use Stream Engine with multiple threads, such as
*background_thread_sample* and *game_loop_sample*.

*/


/**
@fn TOBII_API char const* TOBII_CALL tobii_error_message( tobii_error_t error );
@ingroup tobii

tobii_error_message
-------------------

### Function

Returns a printable error message.


### Syntax

    #include <tobii/tobii.h>
    char const* tobii_error_message( tobii_error_t error );


### Remarks

All other functions in the API returns an error code from the tobii_error_t enumeration. tobii_error_message translates
from these error codes to a human readable message. If the value passed in the *error* parameter is not within the
range of the tobii_error_t enum, a generic message is returned.


### Return value

tobii_error_message returns a zero-terminated C string describing the specified error code. The string returned is
statically allocated, so it should not be freed.


### Example

@code{.c}

    #include <tobii/tobii.h>
    #include <stdio.h>

    int main()
    {
        tobii_api_t* api;

        tobii_error_t error = tobii_api_create( &api, NULL, NULL );
        if( error != TOBII_ERROR_NO_ERROR ) printf( "%s\n", tobii_error_message( error ) );

        error = tobii_api_destroy( api );
        if( error != TOBII_ERROR_NO_ERROR ) printf( "%s\n", tobii_error_message( error ) );

        return 0;
    }

@endcode

*/


/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_get_api_version( tobii_version_t* version );
@ingroup tobii

tobii_get_api_version
---------------------

### Function

Query the current version of the API.


### Syntax

    #include <tobii/tobii.h>
    tobii_error_t tobii_get_api_version( tobii_version_t* version );


### Remarks

tobii_get_api_version can be used to query the version of the stream engine dll currently used.

*version* is a pointer to an tobii_version_t variable to receive the current version numbers. It contains
the following members:

-   *major* incremented for API changes which are not backward-compatible.
-   *minor* incremented for releases which add new, but backward-compatible, API features.
-   *revision* incremented for minor changes and bug fixes which do not change the API.
-   *build* incremented every time a new build is done, even when there are no changes.


### Return value

If the call is successful, tobii_get_api_version returns **TOBII_ERROR_NO_ERROR**. If the call fails,
tobii_get_api_version returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *version* parameter was passed in as NULL. *version* is not optional.


### Example

@code{.c}

    #include <tobii/tobii.h>
    #include <stdio.h>

    int main()
    {
        tobii_version_t version;
        tobii_error_t error = tobii_get_api_version( &version );
        if( error == TOBII_ERROR_NO_ERROR )
            printf( "Current API version: %d.%d.%d\n", version.major, version.minor, 
                version.revision );

        return 0;
    }

@endcode

*/


/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_api_create( tobii_api_t** api, tobii_custom_alloc_t const* custom_alloc, tobii_custom_log_t const* custom_log );
@ingroup tobii

tobii_api_create
----------------

### Function

Initializes the stream engine API, with optionally provided custom memory allocation and logging functions.


### Syntax

    #include <tobii/tobii.h>
    tobii_error_t tobii_api_create( tobii_api_t** api, 
        tobii_custom_alloc_t const* custom_alloc, tobii_custom_log_t const* custom_log );


### Remarks

Before any other API function can be invoked (with the exception of tobii_error_message and tobii_get_api_version), the
API needs to be set up for use, by calling tobii_api_create. The resulting tobii_api_t instance is  passed explicitly to 
some functions, or implicitly to some by passing a device instance. When creating an API instance, it is possible, but 
not necessary, to customize the behavior by passing one or more of the optional parameters *custom_alloc* and 
*custom_log*.

*api* must be a pointer to a variable of the type `tobii_api_t*` that is, a pointer to a tobii_api_t-pointer. This 
variable will be filled in with a pointer to the created instance. tobii_api_t is an opaque type, and only its 
declaration is available in the API.


*custom_alloc* is used to specify a custom allocator for dynamic memory. A custom allocator is specified as a pointer to
a tobii_custom_alloc_t instance, which has the following fields:

-   *mem_context* a custom user data pointer which will be passed through unmodified to the allocator functions when 
    they are called.

-   *malloc_func* a pointer to a function implementing allocation of memory. It must have the following signature:

        void* custom_malloc( void* mem_context, size_t size )

    where *mem_context* will be the same value as the *mem_context* field of tobii_custom_alloc_t, and *size* is the
    number of bytes to allocate. The function must return a pointer to a memory area of, at least, *size* bytes, but
    may return NULL if memory could not be allocated, in which case the API function invoking the allocation will fail
    and return the error **TOBII_ERROR_ALLOCATION_FAILED**.

-   *free_func* a pointer to a function implementing deallocation of memory. It must have the following signature:

        void custom_free( void* mem_context, void* ptr )

    where *mem_context* will be the same value as the *mem_context* field of tobii_custom_alloc_t, and *ptr* is a
    pointer to the memory block (as returned by a call to the custom malloc_func) to be released. The value of *ptr*
    will never be NULL, and only a single call to free_func will be made for each call made to malloc_func.

*custom_alloc* is an optional parameter, and may be NULL, in which case a default allocator is used.

**NOTE:** Stream engine does not guarantee thread safety on *custom_alloc*. If thread safety is a requirement, it should
be satisfied in the implementation of *custom_alloc*. Default allocator runs thread safe.

*custom_log* is used to specify a custom function to handle log printouts. A custom logger is specified as a pointer to
a tobii_custom_log_t instance, which has the following fields:

-   *log_context* a custom user data pointer which will be passed through unmodified to the custom log function when it 
    is called.

-   *log_func* a pointer to a function implementing allocation of memory. It must have the following signature:

        void custom_log( void* log_context, tobii_log_level_t level, char const* text )

    where *log_context* will be the same value as the *log_context* field of tobii_custom_log_t, *level* is one of the
    log levels defined in the tobii_log_level_t enum:
    - **TOBII_LOG_LEVEL_ERROR**
    - **TOBII_LOG_LEVEL_WARN**
    - **TOBII_LOG_LEVEL_INFO**
    - **TOBII_LOG_LEVEL_DEBUG**
    - **TOBII_LOG_LEVEL_TRACE**

    and *text* is the message to be logged. The *level* parameter can be used for filtering log messages by severity,
    but it is up to the custom log function how to make use of it.

*custom_log* is an optional parameter, and may be NULL. In this case, no logging will be done.

**NOTE:** Stream engine does not guarantee thread safety on *custom_log*. If thread safety is a requirement, it should
be satisfied in the implementation of *custom_log*.


### Return value

If API instance creation was successful, tobii_api_create returns **TOBII_ERROR_NO_ERROR**. If creation failed, 
tobii_api_create returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *api* parameter was passed in as NULL, or the *custom_alloc* parameter was provided (it was not NULL), but 
    one or more of its function pointers was NULL. If a custom allocator is provided, both functions (malloc_func and 
    free_func) must be specified. Or the *custom_log* parameter was provided (it was not NULL), but the function pointer 
    log_func was NULL. If a custom log i provided, log_func must be specified.

-   **TOBII_ERROR_ALLOCATION_FAILED**

    The internal call to malloc or to the custom memory allocator (if used) returned NULL, so api creation failed.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.


### See also

tobii_api_destroy(), tobii_device_create()


### Example

@code{.c}

    #include <tobii/tobii.h>
    #include <stdlib.h>
    #include <stdio.h>
    #include <assert.h>

    // we will use custom_alloc to track allocations
    typedef struct allocation_tracking
    {
        int total_allocations;
        int current_allocations;
    } allocation_tracking;

    void* custom_malloc( void* mem_context, size_t size )
    {
        allocation_tracking* tracking = (allocation_tracking*)mem_context;
        // both total allocations, and current allocations increase
        tracking->total_allocations++;
        tracking->current_allocations++;
        return malloc( size ); // pass through to C runtime
    }

    void custom_free( void* mem_context, void* ptr )
    {
        allocation_tracking* tracking = (allocation_tracking*)mem_context;
        // only current allocations decrease, as free doesn't affect our total count
        tracking->current_allocations--;
        free( ptr ); // pass through to C runtime
    }

    void custom_logging( void* log_context, tobii_log_level_t level, char const* text )
    {
        // log messages can be filtered by log level if desired
        if( level == TOBII_LOG_LEVEL_ERROR )
            printf( "[%d] %s\n", (int) level, text ); 
    }

    int main()
    {
        allocation_tracking tracking;
        tracking.total_allocations = 0;
        tracking.current_allocations = 0;

        tobii_custom_alloc_t custom_alloc;
        custom_alloc.mem_context = &tracking;
        custom_alloc.malloc_func = &custom_malloc;
        custom_alloc.free_func = &custom_free;

        tobii_custom_log_t custom_log;
        custom_log.log_context = NULL; // we don't use the log_context in this example
        custom_log.log_func = &custom_logging;

        tobii_api_t* api;
        tobii_error_t error = tobii_api_create( &api, &custom_alloc, &custom_log );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_api_destroy( api );
        assert( error == TOBII_ERROR_NO_ERROR );

        printf( "Total allocations: %d\n", tracking.total_allocations );
        printf( "Current allocations: %d\n", tracking.current_allocations );

        return 0;
    }

@endcode

*/


/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_api_destroy( tobii_api_t* api );
@ingroup tobii

tobii_api_destroy
-----------------

### Function

Destroys an API instance.


### Syntax

    #include <tobii/tobii.h>
    tobii_error_t tobii_api_destroy( tobii_api_t* api );


### Remarks

When creating an instance with tobii_api_create, some system resources are acquired. When finished using the API 
(typically  during the shutdown process), tobii_api_destroy should be called to destroy the instance and ensure that 
those resources are released.
 
tobii_api_destroy should only be called if tobii_api_create completed successfully.

*api* must be a pointer to a valid tobii_api_t instance as created by calling tobii_api_create.


### Return value

If the call was successful, tobii_api_destroy returns **TOBII_ERROR_NO_ERROR** otherwise it can return one of 
the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *api* parameter was passed in as NULL. 

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(),
    or tobii_license_key_retrieve().
    Calling tobii_api_destroy from within a callback function is not supported.


### See also

tobii_api_create(), tobii_device_destroy()


### Example

See tobii_api_create()
*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_enumerate_local_device_urls( tobii_api_t* api, tobii_device_url_receiver_t receiver, void* user_data );
@ingroup tobii

tobii_enumerate_local_device_urls
---------------------------------

### Function

Retrieves the URLs for stream engine compatible devices currently connected to the system.


### Syntax

    #include <tobii/tobii.h>
    tobii_error_t tobii_enumerate_local_device_urls( tobii_api_t* api,
        tobii_device_url_receiver_t receiver, void* user_data );


### Remarks

A system might have multiple devices connected, which the stream engine is able to communicate with.
tobii_enumerate_local_device_urls iterates over all such (excluding IS1 and IS2) devices found.
It will only enumerate devices connected directly to the system, not devices connected on the network.

*api* must be a pointer to a valid tobii_api_t instance as created by calling tobii_api_create.

*receiver* is a function pointer to a function with the prototype:

    void url_receiver( char const* url, void* user_data )

This function will be called for each device found during enumeration. It is called with the following parameters:

-   *url*
    The URL string for the device, zero terminated. This pointer will be invalid after returning from the function,
    so ensure you make a copy of the string rather than storing the pointer directly.

-   *user_data*
    This is the custom pointer sent in to tobii_enumerate_local_device_urls.

*user_data* custom pointer which will be passed unmodified to the receiver function.


### Return value

If the enumeration is successful, tobii_enumerate_local_device_urls returns **TOBII_ERROR_NO_ERROR**. If the call fails,
tobii_enumerate_local_device_urls returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *api* or *receiver* parameters has been passed in as NULL. 

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.


### See also

tobii_device_create(), tobii_enumerate_local_device_urls_ex()


### Example

@code{.c}

    #include <tobii/tobii.h>
    #include <stdio.h>
    #include <assert.h>

    void url_receiver( char const* url, void* user_data )
    {
        int* count = (int*) user_data;
        ++(*count);
        printf( "%d. %s\n", *count, url );
    }

    int main()
    {
        tobii_api_t* api;
        tobii_error_t error = tobii_api_create( &api, NULL, NULL );
        assert( error == TOBII_ERROR_NO_ERROR );

        int count = 0;
        error = tobii_enumerate_local_device_urls( api, url_receiver, &count );        
        if( error == TOBII_ERROR_NO_ERROR )
            printf( "Found %d devices.\n", count );
        else
            printf( "Enumeration failed.\n" );

        error = tobii_api_destroy( api );
        assert( error == TOBII_ERROR_NO_ERROR );
        return 0;
    }

@endcode
*/


/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_enumerate_local_device_urls_ex( tobii_api_t* api, tobii_device_url_receiver_t receiver, void* user_data, uint32_t device_generations );
@ingroup tobii

tobii_enumerate_local_device_urls_ex
------------------------------------

### Function

Retrieves the URLs for the stream engine compatible devices, of the specified generation, currently connected to the
system.


### Syntax

    #include <tobii/tobii.h>
    tobii_error_t tobii_enumerate_local_device_urls_ex( tobii_api_t* api, 
        tobii_device_url_receiver_t receiver, void* user_data, 
        uint32_t device_generations );


### Remarks

A system might have multiple devices connected, which the stream engine is able to communicate with.
tobii_enumerate_local_device_urls_ex works similar to tobii_enumerate_local_device_urls(), but allows for more control.
It only iterates over devices of the specified hardware generations, allowing for limiting the results and the 
processing required to enumerate devices which are not of interest for the application. It will only enumerate devices 
connected directly to the system, not devices connected on the network.

*api* must be a pointer to a valid tobii_api_t instance as created by calling tobii_api_create.

*receiver* is a function pointer to a function with the prototype:

    void url_receiver( char const* url, void* user_data )

This function will be called for each device found during enumeration. It is called with the following parameters:

-   *url*
    The URL string for the device, zero terminated. This pointer will be invalid after returning from the function,
    so ensure you make a copy of the string rather than storing the pointer directly.

-   *user_data*
    This is the custom pointer sent in to tobii_enumerate_local_device_urls_ex.

*user_data* custom pointer which will be passed unmodified to the receiver function.

*device_generations* is a bit-field specifying which hardware generations are to be included in the enumeration. It
is created by bitwise OR-ing of the following constants:

-   TOBII_DEVICE_GENERATION_G5
-   TOBII_DEVICE_GENERATION_IS3
-   TOBII_DEVICE_GENERATION_IS4


### Return value

If the enumeration is successful, tobii_enumerate_local_device_urls_ex returns **TOBII_ERROR_NO_ERROR**. If the call 
fails, tobii_enumerate_local_device_urls_ex returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *api* or *receiver* parameters was passed in as NULL, or the *device_generations* parameter was passed in
    as 0. At least one generation must be selected for enumeration.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.


### See also

tobii_device_create(), tobii_enumerate_local_device_urls()


### Example

@code{.c}

    #include <tobii/tobii.h>
    #include <stdio.h>
    #include <assert.h>

    void url_receiver( char const* url, void* user_data )
    {
        int* count = (int*) user_data;
        ++(*count);
        printf( "%d. %s\n", *count, url );
    }

    int main()
    {
        tobii_api_t* api;
        tobii_error_t error = tobii_api_create( &api, NULL, NULL );
        assert( error == TOBII_ERROR_NO_ERROR );

        int count = 0;
        error = tobii_enumerate_local_device_urls_ex( api, url_receiver, &count,
            TOBII_DEVICE_GENERATION_G5 | TOBII_DEVICE_GENERATION_IS4 );
        if( error == TOBII_ERROR_NO_ERROR )
            printf( "Found %d devices.\n", count );
        else
            printf( "Enumeration failed.\n" );

        error = tobii_api_destroy( api );
        assert( error == TOBII_ERROR_NO_ERROR );
        return 0;
    }

@endcode

*/


/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_device_create( tobii_api_t* api, char const* url, tobii_device_t** device );
@ingroup tobii

tobii_device_create
-------------------

### Function

Creates a device instance to be used for communicating with a specific device.


### Syntax

    #include <tobii/tobii.h>
    tobii_error_t tobii_device_create( tobii_api_t* api,
        char const* url, tobii_device_t** device );


### Remarks

In order to communicate with a specific device, stream engine needs to keep track of internal states.
tobii_device_create allocates and initializes this state, and is needed for all functions which communicates 
with a device. Creating a device will establish a connection to the tracker, and can be used to query the device for 
more information.

*api* must be a pointer to a valid tobii_api_t as created by calling tobii_api_create.

*url* must be a valid device url as returned by tobii_enumerate_local_device_urls.

*device* must be a pointer to a variable of the type `tobii_device_t*` that is, a pointer to a 
tobii_device_t-pointer. This variable will be filled in with a pointer to the created device instance. 
tobii_device_t is an opaque type.


### Return value

If the device is successfully created, tobii_device_create returns **TOBII_ERROR_NO_ERROR**. If the call fails,
tobii_device_create returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *api* or *device* parameters were passed in as NULL, or the url string is not a valid device url (or NULL).

-   **TOBII_ERROR_ALLOCATION_FAILED**

    The internal call to malloc or to the custom memory allocator (if used) returned NULL, so device creation failed.

-   **TOBII_ERROR_CONNECTION_FAILED**

    The connection to the device was lost. Call tobii_device_reconnect() to re-establish connection.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_device_create from within a callback function is not supported.


### See also

tobii_device_destroy(), tobii_enumerate_local_device_urls(), tobii_api_create(), tobii_get_device_info(),
tobii_get_feature_group()


### Example

@code{.c}

    #include <tobii/tobii.h>
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

        // --> code to use the device would go here <--

        error = tobii_device_destroy( device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_api_destroy( api );
        assert( error == TOBII_ERROR_NO_ERROR );
        return 0;
    }

@endcode

*/


/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_device_destroy( tobii_device_t* device );
@ingroup tobii

tobii_device_destroy
----------------------------

### Function

Destroy a device previously created through a call to tobii_device_create.


### Syntax

    #include <tobii/tobii.h>
    tobii_error_t tobii_device_destroy( tobii_device_t* device );


### Remarks

tobii_device_destroy will disconnect from the device, perform cleanup and free the memory allocated by calling
tobii_device_create.

**NOTE:** Make sure that no background thread is using the device, for example in the thread calling 
tobii_device_process_callbacks, before calling tobii_device_destroy in order to avoid the risk of encountering undefined 
behavior.

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create or 
tobii_device_create_ex.


### Return value

If the device is successfully destroyed, tobii_device_create returns **TOBII_ERROR_NO_ERROR**. If the call fails,
tobii_device_create returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* parameter was passed in as NULL. 

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_device_destroy from within a callback function is not supported.


### See also

tobii_device_create(), tobii_device_create_ex()


### Example

See tobii_device_create()

*/


/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_wait_for_callbacks( tobii_engine_t* engine, int device_count, tobii_device_t* const* devices );
@ingroup tobii

tobii_wait_for_callbacks
------------------------

### Function

Puts the calling thread to sleep until there are new callbacks available to process.


### Syntax

    #include <tobii/tobii.h>
    tobii_error_t tobii_wait_for_callbacks( tobii_engine_t* engine, 
        int device_count, tobii_device_t* const* devices )


### Remarks

Stream engine does not use any threads to do processing or receive data. Instead, the functions
tobii_device_process_callbacks() and tobii_device_process_callbacks() have to be called regularly, to receive data 
from the device and from Tobii Engine, and process it. 

The typical use case is to implement your own thread to call tobii_device_process_callbacks and 
tobii_engine_process_callbacks from, and to avoid busy-waiting for data to become available, tobii_wait_for_callbacks 
can be called before each call to tobii_device_process_callbacks and tobii_engine_process_callbacks. It will sleep the 
calling thread until new data is available to process, after which tobii_device_process_callbacks and 
tobii_engine_process_callbacks should be called to process it.

In addition to waiting for data, tobii_wait_for_callbacks will also periodically call tobii_update_timesync() to ensure 
synchronization of system and device timestamps. This means you will not have to call tobii_update_timesync() if you 
regularly call tobii_wait_for_callbacks.

tobii_wait_for_callbacks will not wait indefinitely. There is a timeout of some hundred milliseconds, after which 
tobii_wait_for_callbacks will return **TOBII_ERROR_TIMED_OUT**. This does not indicate a failure - it is given as an 
opportunity for the calling thread to perform its own internal housekeeping (like checking for exit conditions and the 
like). It is valid to immediately call tobii_wait_for_callbacks again to resume waiting.

*engine* is a pointer to a valid tobii_engine_t instance as created by calling tobii_engine_create. It can be passed in
as NULL if there is no tobii_engine_t instance.

*device_count* must be the number of devices in the array passed in the *devices* parameter.

*devices* should be an array of pointers to valid tobii_device_t instances as created by calling tobii_device_create or 
tobii_device_create_ex. It can be NULL if there are no tobii_device_t instances to process. In this case, *device_count*
must be 0.


### Return value

If the operation is successful, tobii_wait_for_callbacks returns **TOBII_ERROR_NO_ERROR**. If the call fails,
or if the wait times out, tobii_wait_for_callbacks returns one of the following:

-   **TOBII_ERROR_TIMED_OUT**

    This does not indicate a failure. A timeout happened before any data was received. Call tobii_wait_for_callbacks() 
    again (it is not necessary to call tobii_device_process_callbacks() or tobii_engine_process_callbacks(), as it 
    doesn't have any new data to process).

-   **TOBII_ERROR_INVALID_PARAMETER**

    No valid device or engine instance was provided. At least one valid pointer to a device or engine instance must be
    provided. 

-   **TOBII_ERROR_CONFLICTING_API_INSTANCES** 

    Every instance of device or engine passed in must be created with the same instance of tobii_api_t. If different
    api instances where used, this error will be returned.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.


### See also

tobii_device_process_callbacks(), tobii_engine_process_callbacks(),


### Example

@code{.c}

    #include <tobii/tobii.h>
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

        int is_running = 1000; // in this sample, exit after some iterations
        while( --is_running > 0 )
        {
            error = tobii_wait_for_callbacks( NULL, 1, &device );
            assert( error == TOBII_ERROR_NO_ERROR || error == TOBII_ERROR_TIMED_OUT );

            error = tobii_device_process_callbacks( device );
            assert( error == TOBII_ERROR_NO_ERROR );
        }

        error = tobii_device_destroy( device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_api_destroy( api );
        assert( error == TOBII_ERROR_NO_ERROR );

        return 0;
    }

@endcode
*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_device_process_callbacks( tobii_device_t* device );
@ingroup tobii

tobii_device_process_callbacks
------------------------------

### Function

Receives data packages from the device, and sends the data through any registered callbacks.


### Syntax

    #include <tobii/tobii.h>
    tobii_error_t tobii_device_process_callbacks( tobii_device_t* device );


### Remarks

Stream engine does not do any kind of background processing, it doesn't start any threads. It doesn't use any
asynchronous callbacks. This means that in order to receive data from the device, the application needs to manually
request the callbacks to happen synchronously, and this is done by calling tobii_device_process_callbacks.

tobii_device_process_callbacks will receive any data packages that are incoming from the device, process them and call any
subscribed callbacks with the data. No callbacks will be called outside of tobii_device_process_callbacks, so the application
have full control over when to receive callbacks.

tobii_device_process_callbacks will not wait for data, and will early-out if there's nothing to process. In order to maintain
the connection to the device, tobii_device_process_callbacks should be called at least 10 times per second.

The recommended way to use tobii_device_process_callbacks, is to start a dedicated thread, and alternately call
tobii_wait_for_callbacks and tobii_device_process_callbacks. See tobii_wait_for_callbacks() for more details.

If there is already a suitable thread to regularly run tobii_device_process_callbacks from (possibly interleaved with
application specific operations), it is possible to do this without calling tobii_wait_for_callbacks(). In this
scenario, time synchronization needs to be handled manually or the timestamps will start drifting. See 
tobii_update_timesync() for more details.

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create or 
tobii_device_create_ex.


### Return value

If the operation is successful, tobii_device_process_callbacks returns **TOBII_ERROR_NO_ERROR**. If the call fails,
tobii_device_process_callbacks returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* parameter was passed in as NULL. 

-   **TOBII_ERROR_ALLOCATION_FAILED**

    The internal call to malloc or to the custom memory allocator (if used) returned NULL, so device creation failed.

-   **TOBII_ERROR_CONNECTION_FAILED**

    The connection to the device was lost. Call tobii_device_reconnect() to re-establish connection.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_device_process_callbacks from within a callback function is not supported.

### See also

tobii_wait_for_callbacks(), tobii_device_clear_callback_buffers(), tobii_device_reconnect(), tobii_update_timesync()


### Example

@code{.c}

    #include <tobii/tobii.h>
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

        int is_running = 1000; // in this sample, exit after some iterations
        while( --is_running > 0 )
        {
            // other parts of main loop would be executed here

            error = tobii_device_process_callbacks( device );
            assert( error == TOBII_ERROR_NO_ERROR );
        }

        error = tobii_device_destroy( device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_api_destroy( api );
        assert( error == TOBII_ERROR_NO_ERROR );

        return 0;
    }

@endcode

*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_device_clear_callback_buffers( tobii_device_t* device );
@ingroup tobii

tobii_device_clear_callback_buffers
----------------------------

### Function

Removes all unprocessed entries from the callback queues.


### Syntax

    #include <tobii/tobii.h>
    tobii_error_t tobii_device_clear_callback_buffers( tobii_device_t* device );


### Remarks

All the data that is received and processed are written into internal buffers used for the callbacks. In some
circumstances, for example during initialization, you might want to discard any data that has been buffered but not
processed, without having to destroy/recreate the device, and without having to implement the filtering out of unwanted
data. tobii_device_clear_callback_buffers will clear all buffered data, and only data arriving *after* the call to
tobii_device_clear_callback_buffers will be forwarded to callbacks.

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create or 
tobii_device_create_ex.


### Return value

If the operation is successful, tobii_device_clear_callback_buffers returns **TOBII_ERROR_NO_ERROR**. If the call fails,
tobii_device_clear_callback_buffers returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* parameter was passed in as NULL.

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_device_clear_callback_buffers from within a callback function is not supported.

### See also

tobii_wait_for_callbacks(), tobii_device_process_callbacks()

*/


/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_device_reconnect( tobii_device_t* device );
@ingroup tobii

tobii_device_reconnect
---------------

### Function

Establish a new connection after a disconnect.


### Syntax

    #include <tobii/tobii.h>
    tobii_error_t tobii_device_reconnect( tobii_device_t* device );


### Remarks

When receiving the error code TOBII_ERROR_CONNECTION_FAILED, it is necessary to explicitly request reconnection, by
calling tobii_device_reconnect.

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create or 
tobii_device_create_ex.


### Return value

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* parameter was passed in as NULL. 

-   **TOBII_ERROR_CONNECTION_FAILED**

    When attempting to reconnect, a connection could not be established. You might want to wait for a bit and try again, 
    for a few times, and if the problem persists, display a message for the user.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_device_reconnect from within a callback function is not supported.

### See also

tobii_device_process_callbacks()


### Example

See tobii_device_process_callbacks()

*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_update_timesync( tobii_device_t* device );
@ingroup tobii

tobii_update_timesync
---------------------

### Function

Makes a manual re-synchronization of system timestamps and device timestamps.


### Syntax

    #include <tobii/tobii.h>
    tobii_error_t tobii_update_timesync( tobii_device_t* device );


### Remarks

The clock on the device and the clock on the system it is connected to may drift over time, and therefore
they need to be periodically re-synchronized. In the default usage scenario, when regularly calling
tobii_wait_for_callbacks(), this re-sychronization is handled automatically at a pre-determined interval.
When not using tobii_wait_for_callbacks, and instead relying on only tobii_device_process_callbacks, it is necessary
to re-synchronize manually, which is done by calling tobii_update_timesync every ~30 seconds.

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create or 
tobii_device_create_ex.


### Return value

If the call to tobii_update_timesync is successful, tobii_update_timesync returns **TOBII_ERROR_NO_ERROR**. If
the call fails, tobii_update_timesync returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* parameter was passed in as NULL. 

-   **TOBII_ERROR_OPERATION_FAILED**

    Timesync operation could not be performed at this time. Please wait a while and try again.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_update_timesync from within a callback function is not supported.

### See also

tobii_wait_for_callbacks(), tobii_device_reconnect(),  tobii_device_process_callbacks(), tobii_system_clock()


### Example

@code{.c}

    #include <tobii/tobii.h>
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

        int is_running = 1000; // in this sample, exit after some iterations
        while( --is_running > 0 )
        {
            error = tobii_device_process_callbacks( device );
            assert( error == TOBII_ERROR_NO_ERROR );

            error = tobii_update_timesync( device );
            assert( error == TOBII_ERROR_NO_ERROR );
        }

        error = tobii_device_destroy( device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_api_destroy( api );
        assert( error == TOBII_ERROR_NO_ERROR );
        return 0;
    }

@endcode

*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_system_clock( tobii_api_t* api, int64_t* timestamp_us );
@ingroup tobii

tobii_system_clock
------------------

### Function

Returns the current system time, from the same clock used to time-stamp callback data.


### Syntax

    #include <tobii/tobii.h>
    tobii_error_t tobii_system_clock( tobii_api_t* api, int64_t* timestamp_us );


### Remarks

Many of the data streams provided by the stream engine API, contains a timestamp value, measured in microseconds (us).
The epoch is undefined, so these timestamps are only useful for calculating the time elapsed between a pair of values.
To facilitate making comparisons between stream engine provided timestamps and application specific events,
tobii_system_clock can be used to retrieve a timestamp using the same clock and same relative values as the timestamps
used in stream engine callbacks.

*api* must be a pointer to a valid tobii_api_t instance as created by calling tobii_api_create.

*timestamp_us* must be a pointer to a int64_t variable to receive the timestamp value.


### Return value

If the operation is successful, tobii_system_clock returns **TOBII_ERROR_NO_ERROR**. If the call fails, 
tobii_system_clock returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *api* or *timestamp_us* parameters were passed in as NULL.

### See also

tobii_api_create()


### Example

@code{.c}

    #include <tobii/tobii.h>
    #include <stdio.h>
    #include <inttypes.h>
    #include <assert.h>

    int main()
    {
        tobii_api_t* api;
        tobii_error_t error = tobii_api_create( &api, NULL, NULL );
        assert( error == TOBII_ERROR_NO_ERROR );

        int64_t time;
        error = tobii_system_clock( api, &time );
        if( error == TOBII_ERROR_NO_ERROR )
            printf( "timestamp: %" PRId64 "\n", time );

        error = tobii_api_destroy( api );
        assert( error == TOBII_ERROR_NO_ERROR );

        return 0;
    }

@endcode

*/


/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_get_device_info( tobii_device_t* device, tobii_device_info_t* device_info );
@ingroup tobii

tobii_get_device_info
---------------------

### Function

Retrieves detailed information about the device, such as name and serial number.


### Syntax

    #include <tobii/tobii.h>
    tobii_error_t tobii_get_device_info( tobii_device_t* device, 
        tobii_device_info_t* device_info );


### Remarks

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create or 
tobii_device_create_ex.

*device_info* is a pointer to a tobii_device_info_t variable to receive the information. It contains the following
fields, all containing zero-terminated ASCII strings:

-   *serial_number* the unique serial number of the device.
-   *model* the model identifier for the device.
-   *generation* the hardware generation, such as G5, IS3 or IS4, of the device.
-   *firmware_version* the version number of the software currently installed on the device.


### Return value

If device info was successfully retrieved, tobii_get_device_info returns **TOBII_ERROR_NO_ERROR**. If the call
fails, tobii_get_device_info returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    One or more of the *device* and *device_info* parameters were passed in as NULL.

-   **TOBII_ERROR_CONNECTION_FAILED**

    The connection to the device was lost. Call tobii_device_reconnect() to re-establish connection.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_get_device_info from within a callback function is not supported.


### See also

tobii_device_create(), tobii_enumerate_local_device_urls()


### Example

@code{.c}

    #include <tobii/tobii.h>
    #include <assert.h>
    #include <stdio.h>

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

        tobii_device_info_t info;
        error = tobii_get_device_info( device, &info );
        assert( error == TOBII_ERROR_NO_ERROR );

        printf( "Serial number: %s\n", info.serial_number );

        error = tobii_device_destroy( device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_api_destroy( api );
        assert( error == TOBII_ERROR_NO_ERROR );
        return 0;
    }

@endcode

*/


/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_get_track_box( tobii_device_t* device, tobii_track_box_t* track_box );
@ingroup tobii

tobii_get_track_box
-------------------

### Function

Retrieves 3d coordinates of the track box frustum, given in millimeters from the device center.


### Syntax

    #include <tobii/tobii.h>
    tobii_error_t tobii_get_track_box( tobii_device_t* device, tobii_track_box_t* track_box );


### Remarks

The track box is a volume in front of the tracker within which the user can be tracked.

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create or 
tobii_device_create_ex.

*track_box* is a pointer to a tobii_track_box_t variable to receive the result. It contains the following
fields, all being arrays of three floating point values, describing the track box frustum:

-   *front_upper_right_xyz*, *front_upper_left_xyz*, *front_lower_left_xyz*, *front_lower_right_xyz*

    The four points on the frustum plane closest to the device.

-   *back_upper_right_xyz*, *back_upper_left_xyz*, *back_lower_left_xyz*, *back_lower_right_xyz*

    The four points on the frustum plane furthest from the device.


### Return value

If track box coordinates were successfully retrieved, tobii_get_track_box returns **TOBII_ERROR_NO_ERROR**. If the call
fails, tobii_get_track_box returns one of the following:

-   **TOBII_ERROR_INVALID_PARAMETER**

    One or more of the *device* and *track_box* parameters were passed in as NULL. 

-   **TOBII_ERROR_CONNECTION_FAILED**

    The connection to the device was lost. Call tobii_device_reconnect() to re-establish connection.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_get_track_box from within a callback function is not supported.


### Example

@code{.c}

    #include <tobii/tobii.h>
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

        tobii_track_box_t track_box;
        error = tobii_get_track_box( device, &track_box );
        assert( error == TOBII_ERROR_NO_ERROR );

        // print just a couple of values of the track box data
        printf( "Front upper left is (%f, %f, %f)\n",
            track_box.front_upper_left_xyz[ 0 ],
            track_box.front_upper_left_xyz[ 1 ],
            track_box.front_upper_left_xyz[ 2 ] );
        printf( "Back lower right is (%f, %f, %f)\n",
            track_box.back_lower_right_xyz[ 0 ],
            track_box.back_lower_right_xyz[ 1 ],
            track_box.back_lower_right_xyz[ 2 ] );

        error = tobii_device_destroy( device );
        assert( error == TOBII_ERROR_NO_ERROR );

        error = tobii_api_destroy( api );
        assert( error == TOBII_ERROR_NO_ERROR );
        return 0;
    }

@endcode

*/


/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_get_state_bool( tobii_device_t* device, tobii_state_t state, tobii_state_bool_t* value );
@ingroup tobii

tobii_get_state_bool
--------------------

### Function

Gets the current value of a state in the tracker.


### Syntax

    #include <tobii/tobii.h>
    tobii_error_t tobii_get_state_bool( tobii_device_t* device, tobii_state_t state,
        tobii_state_bool_t* value );


### Remarks

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create or 
tobii_device_create_ex.

*state* is one of the enum values in tobii_state_t:

-   **TOBII_STATE_POWER_SAVE_ACTIVE**

    Is the power save feature active on the device. This does not necessarily mean power saving measures have been
    engaged.

-   **TOBII_STATE_REMOTE_WAKE_ACTIVE**

    Is the remote wake feature active on the device.

-   **TOBII_STATE_DEVICE_PAUSED**

    Is the device paused. A paused device will keep the connection open but will not send any data while paused. This
    can indicate that the user temporarily wants to disable the device.

-   **TOBII_STATE_EXCLUSIVE_MODE**

    Is the device in an exclusive mode. Similar to TOBII_STATE_DEVICE_PAUSED but the device is sending data to a client
    with exclusive access. This state is only true for short durations and does not normally need to be handled in a
    normal application.

*value* must be a pointer to a valid tobii_state_bool_t instance. On success, *value* will be set to 
**TOBII_STATE_BOOL_TRUE** if the state is true, otherwise **TOBII_STATE_BOOL_FALSE**. *value* will remain
unmodified if the call failed.

**NOTE:** This method relies on cached values which is updated when tobii_device_process_callbacks() is called, so it might not
represent the true state of the device if some time have passed since the last call to tobii_device_process_callbacks().


### Return value

If the call was successful **TOBII_ERROR_NO_ERROR** will be returned. If the call has failed one of the following error
will be returned:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* or *value* parameter has been passed in as NULL or you passed in a *state* that is not a boolean state.

-   **TOBII_ERROR_NOT_SUPPORTED**

    The device firmware has no support for retrieving the value of this state.

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_get_state_bool from within a callback function is not supported.

### Example

@code{.c}

    #include <tobii/tobii.h>
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

        tobii_state_bool_t value;
        error = tobii_get_state_bool( device, TOBII_STATE_DEVICE_PAUSED, &value );
        assert( error == TOBII_ERROR_NO_ERROR );

        if( value == TOBII_STATE_BOOL_TRUE )
            printf( "Device is paused!" );
        else
            printf( "Device is running!" );

        tobii_device_destroy( device );
        tobii_api_destroy( api );

        return 0;
    }

@endcode
*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_get_state_uint32( tobii_device_t* device, tobii_state_t state, uint32_t* value );
@ingroup tobii

tobii_get_state_uint32
--------------------

### Function

Gets the current value of a state in the tracker.


### Syntax

    #include <tobii/tobii.h>
    tobii_error_t tobii_get_state_uint32( tobii_device_t* device, tobii_state_t state,
        uint32_t* value );


### Remarks

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create or
tobii_device_create_ex.

*state* is one of the enum values in tobii_state_t listed below:

-   **TOBII_STATE_CALIBRATION_ID**

    Is the unique value identifying the calibration blob. 0 value indicates default calibration/no calibration done.

*value* must be a pointer to a valid uint32 instance. On success, *value* will be set to id of the calibration blob.

**NOTE:** This method relies on cached values which is updated when tobii_process_callbacks() is called, so it might not
represent the true state of the device if some time have passed since the last call to tobii_process_callbacks().


### Return value

If the call was successful **TOBII_ERROR_NO_ERROR** will be returned. If the call has failed one of the following error
will be returned:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* or *value* parameter has been passed in as NULL or you passed in a *state* that is not a uint32 state i.e
    TOBII_STATE_FAULT.

-   **TOBII_ERROR_NOT_SUPPORTED**

    The device firmware has no support for retrieving the value of this state.

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_get_state_uint32 from within a callback function is not supported.


### Example

@code{.c}

    #include <tobii/tobii.h>
    #include <stdio.h>
    #include <inttypes.h>
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

        uint32_t value;
        error = tobii_get_state_uint32( device, TOBII_STATE_DEVICE_PAUSED, &value );
        assert( error == TOBII_ERROR_NO_ERROR );

        printf( "%" PRIu32 "\n", value );

        tobii_device_destroy( device );
        tobii_api_destroy( api );

        return 0;
    }

@endcode
*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_get_state_string( tobii_device_t* device, tobii_state_t state, tobii_state_string_t value );
@ingroup tobii

tobii_get_state_string
--------------------

### Function

Gets the current string value of a state in the tracker.


### Syntax

    #include <tobii/tobii.h>
    tobii_error_t tobii_get_state_string( tobii_device_t* device, tobii_state_t state,
        tobii_state_string_t value );


### Remarks

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create or
tobii_device_create_ex.

*state* is one of the enum values in tobii_state_t listed below:

-   **TOBII_STATE_FAULT**

    Retrieves a comma separated list of critical errors, if no errors exists the string "ok" is returned. If a critical
    error has occured the device will be unable to track or accept subscriptions.

-   **TOBII_STATE_WARNING**

    Retrieves a comma separated list of warnings, if no warnings exists the string "ok" is returned. If a warning has
    occured the device should still be able to track and accept subscriptions.

*value* must be a pointer to a valid tobii_state_string_t instance. On success, *value* will be set to a null
terminated string containing a maximum of 512 characters including the null termination. On failure, *value* parameter
remains untouched.

**NOTE:** This method relies on cached values which is updated when tobii_process_callbacks() is called, so it might not
represent the true state of the device if some time have passed since the last call to tobii_process_callbacks().


### Return value

If the call was successful **TOBII_ERROR_NO_ERROR** will be returned. If the call has failed one of the following error
will be returned:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* or *value* parameter has been passed in as NULL or you passed in a *state* that is not a string state i.e
    TOBII_STATE_CALIBRATION_ID.

-   **TOBII_ERROR_NOT_SUPPORTED**

    The device firmware has no support for retrieving the value of this state.

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_get_state_string from within a callback function is not supported.


### Example

@code{.c}

    #include <tobii/tobii.h>
    #include <stdio.h>
    #include <inttypes.h>
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

        tobii_state_string_t value;
        error = tobii_get_state_string( device, TOBII_STATE_FAULT, value );
        assert( error == TOBII_ERROR_NO_ERROR );

        printf( "Device fault status: %s\n", value );

        tobii_device_destroy( device );
        tobii_api_destroy( api );

        return 0;
    }

@endcode
*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_capability_supported( tobii_device_t* device, tobii_capability_t capability, tobii_supported_t* supported );
@ingroup tobii

tobii_capability_supported
--------------------------

### Function

Ask if a specific feature is supported or not.


### Syntax

    #include <tobii/tobii.h>
    tobii_error_t tobii_capability_supported( tobii_device_t* device, 
        tobii_capability_t capability, tobii_supported_t* supported );


### Remarks

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create or 
tobii_device_create_ex.

*capability* is one of the enum values in tobii_capability_t:

-   **TOBII_CAPABILITY_DISPLAY_AREA_WRITABLE**

    Query if the display area of the display can be changed by calling tobii_set_display_area().    

-   **TOBII_CAPABILITY_CALIBRATION_2D**

    Query if the device supports performing 2D calibration by calling tobii_calibration_collect_data_2d().

-   **TOBII_CAPABILITY_CALIBRATION_3D**

    Query if the device supports performing 3D calibration by calling tobii_calibration_collect_data_3d().

-   **TOBII_CAPABILITY_PERSISTENT_STORAGE**

    Query if the device supports persistent storage, needed to use tobii_license_key_store and tobii_license_key_retrieve.

-   **TOBII_CAPABILITY_CALIBRATION_PER_EYE**

    Query if the device supports per-eye calibration, needed to use the per-eye calibration api.

-   **TOBII_CAPABILITY_COMBINED_GAZE_VR**

    Query if the device supports combined gaze point in the wearable data stream.

-   **TOBII_CAPABILITY_FACE_TYPE**

    Query if the device supports face type setting, needed to use tobii_get_face_type(), tobii_set_face_type() and
    tobii_enumerate_face_types().

*supported* must be a pointer to a valid tobii_supported_t instance. If tobii_capability_supported is successful, 
*supported* will be set to **TOBII_SUPPORTED** if the feature is supported, and **TOBII_NOT_SUPPORTED** if it is not.


### Return value

If the call was successful **TOBII_ERROR_NO_ERROR** will be returned. If the call has failed one of the following error
will be returned:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* or *supported* parameter has been passed in as NULL or you passed in an invalid enum value
    for *capability*.

-   **TOBII_ERROR_CONNECTION_FAILED**

    The connection to the device was lost. Call tobii_device_reconnect() to re-establish connection.

-   **TOBII_ERROR_INTERNAL**

    Some unexpected internal error occurred. This error should normally not be returned, so if it is, please contact
    the support.

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_capability_supported from within a callback function is not supported.

### See also

tobii_stream_supported()


### Example

@code{.c}

    #include <tobii/tobii.h>
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

        tobii_supported_t supported;
        error = tobii_capability_supported( device, TOBII_CAPABILITY_CALIBRATION_3D, &supported );
        assert( error == TOBII_ERROR_NO_ERROR );

        if( supported == TOBII_SUPPORTED )
            printf( "Device supports 3D calibration." );
        else
            printf( "Device does not support 3D calibration." );

        tobii_device_destroy( device );
        tobii_api_destroy( api );

        return 0;
    }

@endcode
*/

/**
@fn TOBII_API tobii_error_t TOBII_CALL tobii_stream_supported( tobii_device_t* device, tobii_stream_t stream, tobii_supported_t* supported );
@ingroup tobii

tobii_stream_supported
----------------------

### Function

Ask if a specific stream is supported or not.


### Syntax

    #include <tobii/tobii.h>
    tobii_error_t tobii_stream_supported( tobii_device_t* device, 
        tobii_stream_t stream, tobii_supported_t* supported );


### Remarks

*device* must be a pointer to a valid tobii_device_t instance as created by calling tobii_device_create or 
tobii_device_create_ex.

*stream* is one of the enum values in tobii_stream_t, each corresponding to one of the streams from tobii_streams.h,
tobii_wearable.h and tobii_advanced.h

-   **TOBII_STREAM_GAZE_POINT**
-   **TOBII_STREAM_GAZE_ORIGIN**
-   **TOBII_STREAM_EYE_POSITION_NORMALIZED**
-   **TOBII_STREAM_USER_PRESENCE**
-   **TOBII_STREAM_HEAD_POSE**
-   **TOBII_STREAM_WEARABLE**
-   **TOBII_STREAM_GAZE_DATA**
-   **TOBII_STREAM_DIGITAL_SYNCPORT**
-   **TOBII_STREAM_DIAGNOSTICS_IMAGE**

*supported* must be a pointer to a valid tobii_supported_t instance. If tobii_stream_supported is successful, 
*supported* will be set to  **TOBII_SUPPORTED** if the feature is supported, and **TOBII_NOT_SUPPORTED** if it is not.


### Return value

If the call was successful **TOBII_ERROR_NO_ERROR** will be returned. If the call has failed one of the following error
will be returned:

-   **TOBII_ERROR_INVALID_PARAMETER**

    The *device* or *supported* parameter has been passed in as NULL or you passed in an invalid enum value
    for *stream*.

-   **TOBII_ERROR_CALLBACK_IN_PROGRESS**

    The function failed because it was called from within a callback triggered from an API call such as 
    tobii_device_process_callbacks(), tobii_calibration_retrieve(), tobii_enumerate_illumination_modes(), 
    or tobii_license_key_retrieve().
    Calling tobii_stream_supported from within a callback function is not supported.

### See also

tobii_capability_supported()


### Example

@code{.c}

    #include <tobii/tobii.h>
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

        tobii_supported_t supported;
        error = tobii_stream_supported( device, TOBII_STREAM_GAZE_POINT, &supported );
        assert( error == TOBII_ERROR_NO_ERROR );

        if( supported == TOBII_SUPPORTED )
            printf( "Device supports gaze point stream." );
        else
            printf( "Device does not support gaze point stream." );

        tobii_device_destroy( device );
        tobii_api_destroy( api );

        return 0;
    }

@endcode
*/

