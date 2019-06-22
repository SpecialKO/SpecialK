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

#ifndef __SK__GAME_TRAITS_H__
#define __SK__GAME_TRAITS_H__

enum SK_GameQuirk
{ // Unity is better described as 0xffffffff; it's all of these things!
  Unity                       =       0x1,
  UnsafeFullscreen            =       0x2,
  MemoryLeak                  =       0x4,
  VRAMLeak                    =       0x8,
  SteamAPI_TooFast            =      0x10,
  SteamAPI_LoadManually       =      0x20,
  MouseCursorVisible          =      0x40,
  MouseStuckInGameWindow      =      0x80,
  PollsDisconnectedXInput     =     0x100,
  LacksGamepadHotPlug         =     0x200,
  NeedsDPIScalingDisabled     =     0x400,
  SteamAPIConnectivityReq     =     0x800,
  ShutsdownSteamAPIOnErr      =    0x1000,
  GameWindowAlwaysOnTop       =    0x2000,
  IncludesAnticheat           =    0x4000,
  BrokenMultichannelAudio     =    0x8000,
  MissingMipmaps              =   0x10000,
  UsesWrongDocumentsPath      =   0x20000,
  SpawnsTooManyWorkerThreads  =   0x40000,
  SeparateRenderAndMsgThreads =   0x80000,
  HasSleepyMessageThread      =  0x100000,
  HasSleepyRenderThread       =  0x200000,
  BadFramerateLimiter         =  0x400000,
  SixteenByNineOnly           =  0x800000,
  DoesNotBlockAltSpace        = 0x1000000,
  HasSeparateLauncher         = 0x2000000,
  ScreenSaverNotDeactivated   = 0x4000000,
  DriverHDRInsteadOfDXGI      = 0x8000000,

};


#endif /* __SK__GAME_TRAITS_H__ */