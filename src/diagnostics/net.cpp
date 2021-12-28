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

#define __SK_SUBSYSTEM__ L" Net Core "

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


#pragma region DEATH_TO_WINSOCK
#undef AF_IPX
#undef AF_MAX
#undef SO_DONTLINGER
#undef IN_CLASSA

#define WSA_WAIT_IO_COMPLETION  (WAIT_IO_COMPLETION)
#define WSAPROTOCOL_LEN  255
#define FD_MAX_EVENTS    10

#define MAX_PROTOCOL_CHAIN 7

#define BASE_PROTOCOL      1
#define LAYERED_PROTOCOL   0

#pragma pack (push,8)
typedef struct _WSAPROTOCOLCHAIN {
    int ChainLen;                                 /* the length of the chain,     */
                                                  /* length = 0 means layered protocol, */
                                                  /* length = 1 means base protocol, */
                                                  /* length > 1 means protocol chain */
    DWORD ChainEntries[MAX_PROTOCOL_CHAIN];       /* a list of dwCatalogEntryIds */
} WSAPROTOCOLCHAIN, FAR * LPWSAPROTOCOLCHAIN;

typedef struct _WSAPROTOCOL_INFOW {
    DWORD dwServiceFlags1;
    DWORD dwServiceFlags2;
    DWORD dwServiceFlags3;
    DWORD dwServiceFlags4;
    DWORD dwProviderFlags;
    GUID ProviderId;
    DWORD dwCatalogEntryId;
    WSAPROTOCOLCHAIN ProtocolChain;
    int iVersion;
    int iAddressFamily;
    int iMaxSockAddr;
    int iMinSockAddr;
    int iSocketType;
    int iProtocol;
    int iProtocolMaxOffset;
    int iNetworkByteOrder;
    int iSecurityScheme;
    DWORD dwMessageSize;
    DWORD dwProviderReserved;
    WCHAR  szProtocol[WSAPROTOCOL_LEN+1];
} WSAPROTOCOL_INFOW, FAR * LPWSAPROTOCOL_INFOW;

typedef unsigned int             GROUP;

typedef WSAPROTOCOL_INFOW WSAPROTOCOL_INFO;
typedef LPWSAPROTOCOL_INFOW LPWSAPROTOCOL_INFO;

typedef int socklen_t;
#define WSAAPI                  FAR PASCAL
#define WSAEVENT                HANDLE
#define LPWSAEVENT              LPHANDLE
#define WSAOVERLAPPED           OVERLAPPED
typedef struct _OVERLAPPED *    LPWSAOVERLAPPED;

typedef struct _WSANETWORKEVENTS {
       long lNetworkEvents;
       int iErrorCode[FD_MAX_EVENTS];
} WSANETWORKEVENTS, FAR * LPWSANETWORKEVENTS;
#pragma pack (pop)

static bool disable_network_code   = true;

typedef int (WSAAPI *WSAStartup_pfn)
( WORD      wVersionRequired,
  LPWSADATA lpWSAData );

static
WSAStartup_pfn
WSAStartup_Original = nullptr;


typedef SOCKET (WSAAPI *WSASocketW_pfn)
( int                 af,
  int                 type,
  int                 protocol,
  LPWSAPROTOCOL_INFOW lpProtocolInfo,
  GROUP               g,
  DWORD               dwFlags );

static
WSASocketW_pfn
WSASocketW_Original = nullptr;

typedef DWORD (WSAAPI *WSAWaitForMultipleEvents_pfn)
(       DWORD     cEvents,
  const WSAEVENT* lphEvents,
        BOOL      fWaitAll,
        DWORD     dwTimeout,
        BOOL      fAlertable );

static
WSAWaitForMultipleEvents_pfn
WSAWaitForMultipleEvents_Original = nullptr;

typedef INT (WSAAPI *getnameinfo_pfn)
( const SOCKADDR* pSockaddr,
        socklen_t SockaddrLength,
        PCHAR     pNodeBuffer,
        DWORD     NodeBufferSize,
        PCHAR     pServiceBuffer,
        DWORD     ServiceBufferSize,
        INT       Flags );

static
getnameinfo_pfn
getnameinfo_Original = nullptr;

typedef int (WSAAPI *WSAEnumNetworkEvents_pfn)
( SOCKET             s,
  WSAEVENT           hEventObject,
  LPWSANETWORKEVENTS lpNetworkEvents );
#pragma endregion DEATH_TO_WINSOCK // This whole thing sucks, drown it in a region!

static INT
WSAAPI getnameinfo_Detour (
  const SOCKADDR* pSockaddr,
        socklen_t SockaddrLength,
        PCHAR     pNodeBuffer,
        DWORD     NodeBufferSize,
        PCHAR     pServiceBuffer,
        DWORD     ServiceBufferSize,
        INT       Flags )
{
  SK_LOG_FIRST_CALL

  const INT iRet =
    getnameinfo_Original ( pSockaddr, SockaddrLength, pNodeBuffer,
                           NodeBufferSize, pServiceBuffer, ServiceBufferSize,
                           Flags );

  return iRet;
}


static DWORD
WSAAPI WSAWaitForMultipleEvents_Detour (
        DWORD     cEvents,
  const WSAEVENT* lphEvents,
        BOOL      fWaitAll,
        DWORD     dwTimeout,
        BOOL      fAlertable )
{
  SK_LOG_FIRST_CALL

  //if (! config.network.disable_access)
  //{
  //  dll_log->Log ( L" >> Calling Thread for Network Activity: %s",
  //                   SK_Thread_GetName (SK_Thread_GetCurrentId ()).c_str () );
  //}

  UNREFERENCED_PARAMETER (cEvents);
  UNREFERENCED_PARAMETER (lphEvents);
  UNREFERENCED_PARAMETER (fWaitAll);
  UNREFERENCED_PARAMETER (dwTimeout);
  UNREFERENCED_PARAMETER (fAlertable);

  if (config.network.disable_access)
  {
    SK_WaitForSingleObject (__SK_DLL_TeardownEvent, INFINITE);

    return
      WSA_WAIT_IO_COMPLETION;
  }

  return
    WSAWaitForMultipleEvents_Original ( cEvents,  lphEvents,
                                        fWaitAll, dwTimeout,
                                        fAlertable );
}

static SOCKET
WSAAPI WSASocketW_Detour (
  int                 af,
  int                 type,
  int                 protocol,
  LPWSAPROTOCOL_INFOW lpProtocolInfo,
  GROUP               g,
  DWORD               dwFlags)
{
  SK_LOG_FIRST_CALL

  //if (! config.network.disable_access)
  //{
  //  dll_log->Log ( L" >> Calling Thread for Network Activity: %s",
  //                   SK_Thread_GetName (SK_Thread_GetCurrentId () ).c_str () );
  //}

  UNREFERENCED_PARAMETER (af);
  UNREFERENCED_PARAMETER (type);
  UNREFERENCED_PARAMETER (protocol);
  UNREFERENCED_PARAMETER (lpProtocolInfo);
  UNREFERENCED_PARAMETER (g);
  UNREFERENCED_PARAMETER (dwFlags);

  if (config.network.disable_access)
  {
    return
      WSAENETDOWN;
  }

  return
    WSASocketW_Original ( af, type, protocol, lpProtocolInfo,
                          g,  dwFlags );
}

static int
WSAAPI WSAStartup_Detour (
  WORD      wVersionRequired,
  LPWSADATA lpWSAData )
{
  SK_LOG_FIRST_CALL

  if (config.network.disable_access)
  {
    return WSASYSNOTREADY;
  }

  return
    WSAStartup_Original (wVersionRequired, lpWSAData);
}




void
SK_Network_InitHooks (void)
{
  //SK_CreateDLLHook2 (      L"ws2_32.dll",
  //                          "WSASend",
  //                           WSASend_Detour,
  //  static_cast_p2p <void> (&WSASend_Original) );
  //
  //SK_CreateDLLHook2 (      L"ws2_32.dll",
  //                          "WSARecv",
  //                           WSARecv_Detour,
  //  static_cast_p2p <void> (&WSARecv_Original) );

  SK_CreateDLLHook2 (L"Ws2_32.dll", "getnameinfo",
                                     getnameinfo_Detour,              (void **)&getnameinfo_Original);
  SK_CreateDLLHook2 (L"Ws2_32.dll", "WSAWaitForMultipleEvents",
                                     WSAWaitForMultipleEvents_Detour, (void **)&WSAWaitForMultipleEvents_Original);
//SK_CreateDLLHook2 (L"Ws2_32.dll", "WSASocketW",
//                                   WSASocketW_Detour,               (void **)&WSASocketW_Original);
//SK_CreateDLLHook2 (L"Ws2_32.dll", "WSAStartup",
//                                   WSAStartup_Detour,               (void **)&WSAStartup_Original);
}


