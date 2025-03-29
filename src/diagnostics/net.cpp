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

WSARecv_pfn WSARecv_Original = nullptr;
WSASend_pfn WSASend_Original = nullptr;

int
WINAPI
WSARecv_Detour (
  _In_    SOCKET  s,
  _Inout_ LPVOID  lpBuffers,
  _In_    DWORD   dwBufferCount,
  _Out_   LPDWORD lpNumberOfBytesRecvd,
  _Inout_ LPDWORD lpFlags,
  _In_    LPVOID  lpOverlapped,
  _In_    LPVOID  lpCompletionRoutine )
{
  const int ret =
    WSARecv_Original (s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine);

  if (lpNumberOfBytesRecvd != nullptr)
  {
    SK_TLS* pTLS =
      SK_TLS_Bottom ();

    InterlockedAdd64 (&pTLS->net->bytes_received, *lpNumberOfBytesRecvd);
  }

  return ret;
}

int
WINAPI
WSASend_Detour (
  _In_  SOCKET  s,
  _In_  LPVOID  lpBuffers,
  _In_  DWORD   dwBufferCount,
  _Out_ LPDWORD lpNumberOfBytesSent,
  _In_  DWORD   dwFlags,
  _In_  LPVOID  lpOverlapped,
  _In_  LPVOID  lpCompletionRoutine )
{
  const int ret =
    WSASend_Original (s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);

  if (lpNumberOfBytesSent != nullptr)
  {
    SK_TLS* pTLS =
      SK_TLS_Bottom ();

    InterlockedAdd64 (&pTLS->net->bytes_sent, *lpNumberOfBytesSent);
  }

  return ret;
}

void
SK_Network_InitHooks (void)
{
  //SK_PROFILE_FIRST_CALL
  //
  //SK_CreateDLLHook2 (      L"ws2_32.dll",
  //                          "WSASend",
  //                           WSASend_Detour,
  //  static_cast_p2p <void> (&WSASend_Original) );
  //
  //SK_CreateDLLHook2 (      L"ws2_32.dll",
  //                          "WSARecv",
  //                           WSARecv_Detour,
  //  static_cast_p2p <void> (&WSARecv_Original) );
}