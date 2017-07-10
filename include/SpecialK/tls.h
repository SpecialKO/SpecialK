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
#pragma once

#include <Windows.h>

struct SK_TLS {
  struct {
    BOOL texinject_thread    = FALSE;
  } d3d11;

  struct {
    BOOL drawing             = FALSE;
  } imgui;

  struct  {
    BOOL hid                 = FALSE;
  } input;

  struct stack
  {
                 int current = 0;
    static const int max     = 8;
  } stack;
};

extern volatile DWORD __SK_TLS_INDEX;

SK_TLS* __stdcall SK_TLS_Get    (void); // Alias: SK_TLS_Top
SK_TLS* __stdcall SK_TLS_Top    (void);
SK_TLS* __stdcall SK_TLS_Bottom (void);

bool    __stdcall SK_TLS_Push       (void);
bool    __stdcall SK_TLS_Pop        (void);


class SK_ScopedTLS
{
public:
  SK_ScopedTLS (void)
  {
    SK_TLS_Push ();
  }

  ~SK_ScopedTLS (void)
  {
    SK_TLS_Pop ();
  }
};