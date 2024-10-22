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

#ifndef __SK__CPU_H__
#define __SK__CPU_H__

#include <vector>
#include <cstdint>

const std::vector <uintptr_t>&
SK_CPU_GetLogicalCorePairs (void);

void
SK_FPU_LogPrecision (void);

struct SK_FPU_ControlWord {
  UINT x87, sse2;
};

SK_FPU_ControlWord SK_FPU_SetPrecision   (UINT precision);
SK_FPU_ControlWord SK_FPU_SetControlWord (UINT mask, SK_FPU_ControlWord *pNewControl);

#if NTDDI_VERSION < NTDDI_WIN10_RS5
typedef enum EFFECTIVE_POWER_MODE {
  EffectivePowerModeNone = -1,

  EffectivePowerModeBatterySaver,
  EffectivePowerModeBetterBattery,
  EffectivePowerModeBalanced,
  EffectivePowerModeHighPerformance,
  EffectivePowerModeMaxPerformance, // EFFECTIVE_POWER_MODE_V1
  EffectivePowerModeGameMode,
  EffectivePowerModeMixedReality,   // EFFECTIVE_POWER_MODE_V2
} EFFECTIVE_POWER_MODE;
#endif

#if NTDDI_VERSION >= NTDDI_WIN10_RS5
#include <powersetting.h>
#define EffectivePowerModeNone -1
#endif

extern std::atomic <EFFECTIVE_POWER_MODE> SK_Power_EffectiveMode;

#if NTDDI_VERSION < NTDDI_WIN10_RS5
#define EFFECTIVE_POWER_MODE_V1 (0x00000001)
#define EFFECTIVE_POWER_MODE_V2 (0x00000002)
#endif

typedef VOID WINAPI EFFECTIVE_POWER_MODE_CALLBACK (
    _In_     EFFECTIVE_POWER_MODE  Mode,
    _In_opt_ VOID                 *Context
);

EFFECTIVE_POWER_MODE SK_Power_GetCurrentEffectiveMode    (void);
const char*          SK_Power_GetEffectiveModeStr        (EFFECTIVE_POWER_MODE mode);
bool                 SK_Power_InitEffectiveModeCallbacks (void);
bool                 SK_Power_StopEffectiveModeCallbacks (void);

void                 SK_CPU_InstallHooks                 (void);
bool                 SK_CPU_IsZen                        (void);

#endif /* __SK__CPU_H__ */