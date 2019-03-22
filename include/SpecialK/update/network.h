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
#ifndef __SK__Update__Network_H__
#define __SK__Update__Network_H__

#include <winnt.h>

extern HRESULT
  __stdcall
    SK_UpdateSoftware1 (const wchar_t* wszProduct, bool force = false);

extern HRESULT
  __stdcall
    SK_UpdateSoftware (const wchar_t* wszProduct);

#include <WinInet.h>


struct sk_internet_get_t {
  enum {
    STATUS_INVALID   = 0,
    STATUS_UPDATED   = 1,
    STATUS_REMINDER  = 2,
    STATUS_CANCELLED = 4,
    STATUS_FAILED    = 8
  };

  wchar_t wszHostName  [INTERNET_MAX_HOST_NAME_LENGTH] = { };
  wchar_t wszHostPath  [INTERNET_MAX_PATH_LENGTH]      = { };
  wchar_t wszLocalPath [MAX_PATH]                      = { };
  HWND    hTaskDlg                                     = HWND_DESKTOP;
  int     status                                       = STATUS_INVALID;
};

#endif /* __SK_Update__Network_H__ */