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

#ifndef __SK__DXGI_HDR_H__
#define __SK__DXGI_HDR_H__

#include <SpecialK/render/dxgi/dxgi_interfaces.h>

struct SK_DXGI_HDRControl
{
  struct
  {
    UINT                  BitsPerColor     = 0;
    DXGI_COLOR_SPACE_TYPE ColorSpace       = DXGI_COLOR_SPACE_RESERVED;

    FLOAT                 RedPrimary   [2] = { 0.0f, 0.0f };
    FLOAT                 GreenPrimary [2] = { 0.0f, 0.0f };
    FLOAT                 BluePrimary  [2] = { 0.0f, 0.0f };
    FLOAT                 WhitePoint   [2] = { 0.0f, 0.0f };

    FLOAT                 MinLuminance          = 0.0f,
                          MaxLuminance          = 0.0f,
                          MaxFullFrameLuminance = 0.0f;
  } devcaps;

  struct
  {
    UINT   MaxMasteringLuminance     = 0;
    UINT   MinMasteringLuminance     = 0;

    UINT16 MaxContentLightLevel      = 0;
    UINT16 MaxFrameAverageLightLevel = 0;

    LONG   _AdjustmentCount          = 0;
  } meta;

  struct
  {
    bool MaxMaster                 = false;
    bool MinMaster                 = false;
    bool MaxContentLightLevel      = false;
    bool MaxFrameAverageLightLevel = false;
  } overrides;
};

SK_DXGI_HDRControl*
SK_HDR_GetControl (void);

#endif __SK__DXGI_HDR_H__