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

#ifndef __SK__NETWORK_H__
#define __SK__NETWORK_H__

#ifndef _WINSOCKAPI_
#include <ws2def.h>
#endif

void
SK_Network_InitHooks (void);

typedef int (WINAPI *WSARecv_pfn)(
  _In_    SOCKET  s,
  _Inout_ LPVOID  lpBuffers,
  _In_    DWORD   dwBufferCount,
  _Out_   LPDWORD lpNumberOfBytesRecvd,
  _Inout_ LPDWORD lpFlags,
  _In_    LPVOID  lpOverlapped,
  _In_    LPVOID  lpCompletionRoutine
);

typedef int (WINAPI *WSASend_pfn)(
  _In_  SOCKET  s,
  _In_  LPVOID  lpBuffers,
  _In_  DWORD   dwBufferCount,
  _Out_ LPDWORD lpNumberOfBytesSent,
  _In_  DWORD   dwFlags,
  _In_  LPVOID  lpOverlapped,
  _In_  LPVOID  lpCompletionRoutine
);

extern WSARecv_pfn WSARecv_Original;
extern WSASend_pfn WSASend_Original;

#endif /* __SK__NETWORK_H__ */