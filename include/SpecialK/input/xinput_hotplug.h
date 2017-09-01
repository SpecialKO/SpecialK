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

#ifndef __SK__XINPUT_HOTPLUG_H__
#define __SK__XINPUT_HOTPLUG_H__

#include <SpecialK/input/xinput.h>

typedef struct _SK_XInput_PacketJournal
{
  struct
  {
    DWORD virt    = 0;
    DWORD real    = 0;
  } packet_count;

  struct
  {
    DWORD last    = 0;
    DWORD current = 0;
  } sequence;
} SK_XInput_PacketJournal;


SK_XInput_PacketJournal
SK_XInput_GetPacketJournal (DWORD dwUserIndex);

bool
SK_XInput_Holding ( DWORD dwUserIndex );

DWORD
SK_XInput_PlaceHold ( DWORD         dwRet,
                      DWORD         dwUserIndex,
                      XINPUT_STATE *pState );

DWORD
SK_XInput_PlaceHoldEx ( DWORD            dwRet,
                        DWORD            dwUserIndex,
                        XINPUT_STATE_EX *pState );

DWORD
SK_XInput_PlaceHoldCaps ( DWORD                dwRet,
                          DWORD                dwUserIndex,
                          DWORD                dwFlags,
                          XINPUT_CAPABILITIES *pCapabilities );

DWORD
SK_XInput_PlaceHoldBattery ( DWORD                       dwRet,
                             DWORD                       dwUserIndex,
                             BYTE                        devType,
                             XINPUT_BATTERY_INFORMATION *pBatteryInformation );

DWORD
SK_XInput_PlaceHoldSet ( DWORD             dwRet,
                         DWORD             dwUserIndex,
                         XINPUT_VIBRATION *pVibration );

void
SK_XInput_PacketJournalize (DWORD dwRet, DWORD dwUserIndex, XINPUT_STATE *pState);


void
SK_XInput_InitHotPlugHooks (void);

#endif /* __SK__XINPUT_HOTPLUG_H__ */