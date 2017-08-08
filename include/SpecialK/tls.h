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

// Useless warninn:  'typedef ': ignored on left of '' when no variable is declared
#pragma warning (disable: 4091)

#include <Windows.h>

#define _NO_CVCONST_H
#include <dbghelp.h>

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

  // All stack frames except for bottom
  //   have meaningless values for these,
  //
  //  >> Always access through SK_TLS_Bottom <<
  //
  struct
  {
    CONTEXT          last_ctx    = {   };
    EXCEPTION_RECORD last_exc    = {   };
    bool             last_chance = false;
  } debug;

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

bool    __stdcall SK_TLS_Push   (void);
bool    __stdcall SK_TLS_Pop    (void);


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