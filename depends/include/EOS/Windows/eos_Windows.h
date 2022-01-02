// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "eos_types.h"

#pragma pack(push, 8)
/** The most recent version of the EOS_Windows_RTCOptions structure. */
#define EOS_WINDOWS_RTCOPTIONS_API_LATEST 1

/**
 * Options for initializing rtc functionality required for some platforms.
 */
EOS_STRUCT(EOS_Windows_RTCOptions, (
	/** API Version: Set this to EOS_WINDOWS_RTCOPTIONS_API_LATEST. */
	int32_t ApiVersion;

	/** The absolute path to a `xaudio2_9redist.dll` dependency, including the file name */
	const char* XAudio29DllPath;
));

#pragma pack(pop)
