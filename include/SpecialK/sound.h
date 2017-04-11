/**
 * This file is part of Special K.
 *
 * Special K is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Special K is distributed in the hope that it will be useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Special K.
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/

#ifndef __SK__SOUND_H__
#define __SK__SOUND_H__

#include <Mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>

void                    __stdcall SK_SetGameMute                    (bool bMute);

IAudioMeterInformation* __stdcall SK_WASAPI_GetAudioMeterInfo       (void);
ISimpleAudioVolume*     __stdcall SK_WASAPI_GetVolumeControl        (DWORD   proc_id = GetCurrentProcessId ());
IChannelAudioVolume*    __stdcall SK_WASAPI_GetChannelVolumeControl (DWORD   proc_id = GetCurrentProcessId ());
void                    __stdcall SK_WASAPI_GetAudioSessionProcs    (size_t* count, DWORD* procs = nullptr);

const char*             __stdcall SK_WASAPI_GetChannelName          (int channel_idx);

#endif /* __SK__SOUND_H__ */