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
    UINT                  BitsPerColor;
    DXGI_COLOR_SPACE_TYPE ColorSpace;

    FLOAT                 RedPrimary   [2];
    FLOAT                 GreenPrimary [2];
    FLOAT                 BluePrimary  [2];
    FLOAT                 WhitePoint   [2];

    FLOAT                 MinLuminance,
                          MaxLuminance,
                          MaxFullFrameLuminance;
  } devcaps;

  struct
  {
    UINT   MaxMasteringLuminance;
    UINT   MinMasteringLuminance;

    UINT16 MaxContentLightLevel;
    UINT16 MaxFrameAverageLightLevel;

    LONG   _AdjustmentCount = 0;
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