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

#include <SpecialK/stdafx.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"SteamTier0"

using Plat_IsSteamDeck_pfn = bool (*)(void);
      Plat_IsSteamDeck_pfn
      Plat_IsSteamDeck_Original = nullptr;

bool
SK_Plat_IsSteamDeck (void)
{
  SK_LOG_FIRST_CALL

  return true;
}

void SK_InitSteamTier0 (void)
{
  SK_RunOnce (
    SK_CreateDLLHook2 (      L"tier0_s64.dll",
                              "Plat_IsSteamDeck",
                            SK_Plat_IsSteamDeck,
      static_cast_p2p <void> (&Plat_IsSteamDeck_Original) )
  );
}