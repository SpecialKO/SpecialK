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

#ifndef __SK__RESHADE_H__
#define __SK__RESHADE_H__

#include <Unknwnbase.h>

#include <Windows.h>

HMODULE
__stdcall
SK_ReShade_GetDLL (void);

void
SK_ReShade_LoadIfPresent (void);

bool SK_ReShadeAddOn_RenderEffectsDXGI (IDXGISwapChain1 *pSwapChain);
bool SK_ReShadeAddOn_Init              (HMODULE          reshade_module = nullptr);

#endif /* __SK__RESHADE_H__ */