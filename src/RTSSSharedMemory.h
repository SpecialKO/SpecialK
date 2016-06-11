/////////////////////////////////////////////////////////////////////////////
//
// This header file defines statistics server's shared memory format
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _RTSS_SHARED_MEMORY_INCLUDED_
#define _RTSS_SHARED_MEMORY_INCLUDED_
/////////////////////////////////////////////////////////////////////////////
// v1.0 memory structure
typedef struct RTSS_SHARED_MEMORY_V_1_0
{
	DWORD	dwSignature;
		//signature allows applications to verify status of shared memory

		//The signature can be set to:
		//'RTSS'	- statistics server's memory is initialized and contains 
		//			valid data 
		//0xDEAD	- statistics server's memory is marked for deallocation and
		//			no longer contain valid data
		//otherwise	the memory is not initialized
	DWORD	dwVersion;
		//structure version ((major<<16) + minor)
		//must be set to 0x00010000 for v1.0 structure
	DWORD	dwTime0;
		//start time of framerate measurement period (in milliseconds)

		//Take a note that this field must contain non-zero value to calculate 
		//framerate properly!
	DWORD	dwTime1;
		//end time of framerate measurement period (in milliseconds)
	DWORD	dwFrames;
		//amount of frames rendered during (dwTime1 - dwTime0) period 

		//to calculate framerate use the following formula:
		//1000.0f * dwFrames / (dwTime1 - dwTime0)

} RTSS_SHARED_MEMORY_V_1_0, *LPRTSS_SHARED_MEMORY_V_1_0;
/////////////////////////////////////////////////////////////////////////////
#define OSDFLAG_UPDATED			0x00000001
	//use this flag to force the server to update OSD
/////////////////////////////////////////////////////////////////////////////
// v1.1 memory structure
typedef struct RTSS_SHARED_MEMORY_V_1_1
{
	DWORD	dwSignature;
		//signature allows applications to verify status of shared memory

		//The signature can be set to:
		//'RTSS'	- statistics server's memory is initialized and contains 
		//			valid data 
		//0xDEAD	- statistics server's memory is marked for deallocation and
		//			no longer contain valid data
		//otherwise	the memory is not initialized
	DWORD	dwVersion;
		//structure version ((major<<16) + minor)
		//must be set to 0x00010001 for v1.1 structure
	DWORD	dwTime0;
		//start time of framerate measurement period (in milliseconds)

		//Take a note that this field must contain non-zero value to calculate 
		//framerate properly!
	DWORD	dwTime1;
		//end time of framerate measurement period (in milliseconds)
	DWORD	dwFrames;
		//amount of frames rendered during (dwTime1 - dwTime0) period

		//to calculate framerate use the following formula:
		//1000.0f * dwFrames / (dwTime1 - dwTime0)

	DWORD	dwOSDFlags;
		//bitmask, containing combination of OSDFLAG_... flags

		//Note: set OSDFLAG_UPDATED flag as soon as you change any OSD related
		//field
	DWORD	dwOSDX;
		//OSD X-coordinate (coordinate wrapping is allowed, i.e. -5 defines 5
		//pixel offset from the right side of the screen)
	DWORD	dwOSDY;
		//OSD Y-coordinate (coordinate wrapping is allowed, i.e. -5 defines 5
		//pixel offset from the bottom side of the screen)
	DWORD	dwOSDPixel;
		//OSD pixel zooming ratio
	DWORD	dwOSDColor;
		//OSD color in RGB format
	char	szOSD[256];
		//OSD text
	char	szOSDOwner[32];
		//OSD owner ID

		//Use this field to capture OSD and prevent other applications from
		//using OSD when it is already in use by your application.
		//You should change this field only if it is empty (i.e. when OSD is
		//not owned by any application) or if it is set to your own application's
		//ID (i.e. when you own OSD)
		//You shouldn't change any OSD related feilds until you own OSD

} RTSS_SHARED_MEMORY_V_1_1, *LPRTSS_SHARED_MEMORY_V_1_1;
/////////////////////////////////////////////////////////////////////////////
// v1.2 memory structure
typedef struct RTSS_SHARED_MEMORY_V_1_2
{
	DWORD	dwSignature;
		//signature allows applications to verify status of shared memory

		//The signature can be set to:
		//'RTSS'	- statistics server's memory is initialized and contains 
		//			valid data 
		//0xDEAD	- statistics server's memory is marked for deallocation and
		//			no longer contain valid data
		//otherwise	the memory is not initialized
	DWORD	dwVersion;
		//structure version ((major<<16) + minor)
		//must be set to 0x00010002 for v1.2 structure
	DWORD	dwTime0;
		//start time of framerate measurement period (in milliseconds)

		//Take a note that this field must contain non-zero value to calculate 
		//framerate properly!
	DWORD	dwTime1;
		//end time of framerate measurement period (in milliseconds)
	DWORD	dwFrames;
		//amount of frames rendered during (dwTime1 - dwTime0) period

		//to calculate framerate use the following formula:
		//1000.0f * dwFrames / (dwTime1 - dwTime0)

	DWORD	dwOSDFlags;
		//bitmask, containing combination of OSDFLAG_... flags

		//Note: set OSDFLAG_UPDATED flag as soon as you change any OSD related
		//field
	DWORD	dwOSDX;
		//OSD X-coordinate (coordinate wrapping is allowed, i.e. -5 defines 5
		//pixel offset from the right side of the screen)
	DWORD	dwOSDY;
		//OSD Y-coordinate (coordinate wrapping is allowed, i.e. -5 defines 5
		//pixel offset from the bottom side of the screen)
	DWORD	dwOSDPixel;
		//OSD pixel zooming ratio
	DWORD	dwOSDColor;
		//OSD color in RGB format
	char	szOSD[256];
		//primary OSD slot text
	char	szOSDOwner[32];
		//primary OSD slot owner ID

		//Use this field to capture OSD slot and prevent other applications from
		//using OSD when it is already in use by your application.
		//You should change this field only if it is empty (i.e. when OSD slot is
		//not owned by any application) or if it is set to your own application's
		//ID (i.e. when you own OSD slot)
		//You shouldn't change any OSD related feilds until you own OSD slot

	char	szOSD1[256];
		//OSD slot 1 text
	char	szOSD1Owner[32];
		//OSD slot 1 owner ID
	char	szOSD2[256];
		//OSD slot 2 text
	char	szOSD2Owner[32];
		//OSD slot 2 owner ID
	char	szOSD3[256];
		//OSD slot 3 text
	char	szOSD3Owner[32];
		//OSD slot 3 owner ID
} RTSS_SHARED_MEMORY_V_1_2, *LPRTSS_SHARED_MEMORY_V_1_2;
/////////////////////////////////////////////////////////////////////////////
#define STATFLAG_RECORD			0x00000001
/////////////////////////////////////////////////////////////////////////////
// v1.3 memory structure
typedef struct RTSS_SHARED_MEMORY_V_1_3
{
	DWORD	dwSignature;
		//signature allows applications to verify status of shared memory

		//The signature can be set to:
		//'RTSS'	- statistics server's memory is initialized and contains 
		//			valid data 
		//0xDEAD	- statistics server's memory is marked for deallocation and
		//			no longer contain valid data
		//otherwise	the memory is not initialized
	DWORD	dwVersion;
		//structure version ((major<<16) + minor)
		//must be set to 0x00010003 for v1.3 structure
	DWORD	dwTime0;
		//start time of framerate measurement period (in milliseconds)

		//Take a note that this field must contain non-zero value to calculate 
		//framerate properly!
	DWORD	dwTime1;
		//end time of framerate measurement period (in milliseconds)
	DWORD	dwFrames;
		//amount of frames rendered during (dwTime1 - dwTime0) period

		//to calculate framerate use the following formula:
		//1000.0f * dwFrames / (dwTime1 - dwTime0)

	DWORD	dwOSDFlags;
		//bitmask, containing combination of OSDFLAG_... flags

		//Note: set OSDFLAG_UPDATED flag as soon as you change any OSD related
		//field
	DWORD	dwOSDX;
		//OSD X-coordinate (coordinate wrapping is allowed, i.e. -5 defines 5
		//pixel offset from the right side of the screen)
	DWORD	dwOSDY;
		//OSD Y-coordinate (coordinate wrapping is allowed, i.e. -5 defines 5
		//pixel offset from the bottom side of the screen)
	DWORD	dwOSDPixel;
		//OSD pixel zooming ratio
	DWORD	dwOSDColor;
		//OSD color in RGB format
	char	szOSD[256];
		//primary OSD slot text
	char	szOSDOwner[32];
		//primary OSD slot owner ID

		//Use this field to capture OSD slot and prevent other applications from
		//using OSD when it is already in use by your application.
		//You should change this field only if it is empty (i.e. when OSD slot is
		//not owned by any application) or if it is set to your own application's
		//ID (i.e. when you own OSD slot)
		//You shouldn't change any OSD related feilds until you own OSD slot

	char	szOSD1[256];
		//OSD slot 1 text
	char	szOSD1Owner[32];
		//OSD slot 1 owner ID
	char	szOSD2[256];
		//OSD slot 2 text
	char	szOSD2Owner[32];
		//OSD slot 2 owner ID
	char	szOSD3[256];
		//OSD slot 3 text
	char	szOSD3Owner[32];
		//OSD slot 3 owner ID

	DWORD	dwStatFlags;
		//bitmask containing combination of STATFLAG_... flags
	DWORD	dwStatTime0;
		//statistics record period start time
	DWORD	dwStatTime1;
		//statistics record period end time
	DWORD	dwStatFrames;
		//total amount of frames rendered during statistics record period
	DWORD	dwStatCount;
		//amount of min/avg/max measurements during statistics record period 
	DWORD	dwStatFramerateMin;
		//minimum instantaneous framerate measured during statistics record period 
	DWORD	dwStatFramerateAvg;
		//average instantaneous framerate measured during statistics record period 
	DWORD	dwStatFramerateMax;
		//maximum instantaneous framerate measured during statistics record period 
} RTSS_SHARED_MEMORY_V_1_3, *LPRTSS_SHARED_MEMORY_V_1_3;
/////////////////////////////////////////////////////////////////////////////
#define APPFLAG_DD												0x00000010
#define APPFLAG_D3D8											0x00000100
#define APPFLAG_D3D9											0x00001000
#define APPFLAG_D3D9EX											0x00002000
#define APPFLAG_OGL												0x00010000
#define APPFLAG_D3D10											0x00100000
#define APPFLAG_D3D11											0x01000000

#define APPFLAG_API_USAGE_MASK									(APPFLAG_DD | APPFLAG_D3D8 | APPFLAG_D3D9 | APPFLAG_D3D9EX | APPFLAG_OGL | APPFLAG_D3D10  | APPFLAG_D3D11)

#define APPFLAG_PROFILE_UPDATE_REQUESTED						0x10000000
/////////////////////////////////////////////////////////////////////////////
#define SCREENCAPTUREFLAG_REQUEST_CAPTURE						0x00000001
#define SCREENCAPTUREFLAG_REQUEST_CAPTURE_OSD					0x00000010
/////////////////////////////////////////////////////////////////////////////
#define VIDEOCAPTUREFLAG_REQUEST_CAPTURE_START					0x00000001
#define VIDEOCAPTUREFLAG_REQUEST_CAPTURE_PROGRESS				0x00000002
#define VIDEOCAPTUREFLAG_REQUEST_CAPTURE_STOP					0x00000004
#define VIDEOCAPTUREFLAG_REQUEST_CAPTURE_MASK					0x00000007
#define VIDEOCAPTUREFLAG_REQUEST_CAPTURE_OSD					0x00000010

#define VIDEOCAPTUREFLAG_INTERNAL_RESIZE						0x00010000
/////////////////////////////////////////////////////////////////////////////

// v2.0 memory structure
typedef struct RTSS_SHARED_MEMORY
{
	DWORD	dwSignature;
		//signature allows applications to verify status of shared memory

		//The signature can be set to:
		//'RTSS'	- statistics server's memory is initialized and contains 
		//			valid data 
		//0xDEAD	- statistics server's memory is marked for deallocation and
		//			no longer contain valid data
		//otherwise	the memory is not initialized
	DWORD	dwVersion;
		//structure version ((major<<16) + minor)
		//must be set to 0x0002xxxx for v2.x structure 

	DWORD	dwAppEntrySize;
		//size of RTSS_SHARED_MEMORY_OSD_ENTRY for compatibility with future versions
	DWORD	dwAppArrOffset;
		//offset of arrOSD array for compatibility with future versions
	DWORD	dwAppArrSize;
		//size of arrOSD array for compatibility with future versions

	DWORD	dwOSDEntrySize;
		//size of RTSS_SHARED_MEMORY_APP_ENTRY for compatibility with future versions
	DWORD	dwOSDArrOffset;
		//offset of arrApp array for compatibility with future versions
	DWORD	dwOSDArrSize;
		//size of arrOSD array for compatibility with future versions

	DWORD	dwOSDFrame;
		//Global OSD frame ID. Increment it to force the server to update OSD for all currently active 3D
		//applications.

	//OSD slot descriptor structure

	typedef struct RTSS_SHARED_MEMORY_OSD_ENTRY
	{
		char	szOSD[256];
			//OSD slot text
		char	szOSDOwner[256];
			//OSD slot owner ID

		//next fields are valid for v2.7 and newer shared memory format only

		char	szOSDEx[4096];
			//extended OSD slot text

	} RTSS_SHARED_MEMORY_OSD_ENTRY, *LPRTSS_SHARED_MEMORY_OSD_ENTRY;

	//application descriptor structure

	typedef struct RTSS_SHARED_MEMORY_APP_ENTRY
	{
		//application identification related fields

		DWORD	dwProcessID;
			//process ID
		char	szName[MAX_PATH];
			//process executable name
		DWORD	dwFlags;
			//application specific flags

		//instantaneous framerate related fields

		DWORD	dwTime0;
			//start time of framerate measurement period (in milliseconds)

			//Take a note that this field must contain non-zero value to calculate 
			//framerate properly!
		DWORD	dwTime1;
			//end time of framerate measurement period (in milliseconds)
		DWORD	dwFrames;
			//amount of frames rendered during (dwTime1 - dwTime0) period
		DWORD	dwFrameTime;
			//frame time (in microseconds)


			//to calculate framerate use the following formulas:

			//1000.0f * dwFrames / (dwTime1 - dwTime0) for framerate calculated once per second
			//or
			//1000000.0f / dwFrameTime for framerate calculated once per frame 

		//framerate statistics related fields

		DWORD	dwStatFlags;
			//bitmask containing combination of STATFLAG_... flags
		DWORD	dwStatTime0;
			//statistics record period start time
		DWORD	dwStatTime1;
			//statistics record period end time
		DWORD	dwStatFrames;
			//total amount of frames rendered during statistics record period
		DWORD	dwStatCount;
			//amount of min/avg/max measurements during statistics record period 
		DWORD	dwStatFramerateMin;
			//minimum instantaneous framerate measured during statistics record period 
		DWORD	dwStatFramerateAvg;
			//average instantaneous framerate measured during statistics record period 
		DWORD	dwStatFramerateMax;
			//maximum instantaneous framerate measured during statistics record period 

		//OSD related fields

		DWORD	dwOSDX;
			//OSD X-coordinate (coordinate wrapping is allowed, i.e. -5 defines 5
			//pixel offset from the right side of the screen)
		DWORD	dwOSDY;
			//OSD Y-coordinate (coordinate wrapping is allowed, i.e. -5 defines 5
			//pixel offset from the bottom side of the screen)
		DWORD	dwOSDPixel;
			//OSD pixel zooming ratio
		DWORD	dwOSDColor;
			//OSD color in RGB format
		DWORD	dwOSDFrame;
			//application specific OSD frame ID. Don't change it directly!

		DWORD	dwScreenCaptureFlags;
		char	szScreenCapturePath[MAX_PATH];

		//next fields are valid for v2.1 and newer shared memory format only

		DWORD	dwOSDBgndColor;
			//OSD background color in RGB format

		//next fields are valid for v2.2 and newer shared memory format only

		DWORD	dwVideoCaptureFlags;
		char	szVideoCapturePath[MAX_PATH];
		DWORD	dwVideoFramerate;
		DWORD	dwVideoFramesize;
		DWORD	dwVideoFormat;
		DWORD	dwVideoQuality;
		DWORD	dwVideoCaptureThreads;

		DWORD	dwScreenCaptureQuality;
		DWORD	dwScreenCaptureThreads;

		//next fields are valid for v2.3 and newer shared memory format only

		DWORD	dwAudioCaptureFlags;

		//next fields are valid for v2.4 and newer shared memory format only

		DWORD	dwVideoCaptureFlagsEx;

		//next fields are valid for v2.5 and newer shared memory format only

		DWORD	dwAudioCaptureFlags2;

		DWORD	dwStatFrameTimeMin;
		DWORD	dwStatFrameTimeAvg;
		DWORD	dwStatFrameTimeMax;
		DWORD	dwStatFrameTimeCount;

		DWORD	dwStatFrameTimeBuf[1024];
		DWORD	dwStatFrameTimeBufPos;
		DWORD	dwStatFrameTimeBufFramerate;

		//next fields are valid for v2.6 and newer shared memory format only

		LARGE_INTEGER qwAudioCapturePTTEventPush;
		LARGE_INTEGER qwAudioCapturePTTEventRelease;

		LARGE_INTEGER qwAudioCapturePTTEventPush2;
		LARGE_INTEGER qwAudioCapturePTTEventRelease2;

	} RTSS_SHARED_MEMORY_APP_ENTRY, *LPRTSS_SHARED_MEMORY_APP_ENTRY;

	RTSS_SHARED_MEMORY_OSD_ENTRY arrOSD[8];
		//array of OSD slots
	RTSS_SHARED_MEMORY_APP_ENTRY arrApp[256];
		//array of application descriptors

} RTSS_SHARED_MEMORY, *LPRTSS_SHARED_MEMORY;
/////////////////////////////////////////////////////////////////////////////
#endif //_RTSS_SHARED_MEMORY_INCLUDED_