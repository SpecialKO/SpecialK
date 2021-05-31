/*
Copyright 2017-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <string>

#include <SpecialK/utility.h>

bool InPerfLogUsersGroup (void)
{
  // PERFLOG_USERS = S-1-5-32-559
  SID_IDENTIFIER_AUTHORITY authority       = SECURITY_NT_AUTHORITY;
  PSID                     sidPerfLogUsers = {                   };

  if ( FALSE ==
         AllocateAndInitializeSid (
                          &authority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                      DOMAIN_ALIAS_RID_LOGGING_USERS,
                                                    0, 0, 0, 0, 0, 0,
                          &sidPerfLogUsers ) )
  {
    return false;
  }

  BOOL
    isMember = FALSE;

  if (! CheckTokenMembership (nullptr, sidPerfLogUsers, &isMember))
    isMember = FALSE;

  FreeSid (sidPerfLogUsers);

  return
    ( isMember != FALSE );
}

bool
EnableDebugPrivilege (void)
{
  auto hModule =
    LoadLibraryEx ( L"advapi32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32 );

  auto pOpenProcessToken      = (decltype(&OpenProcessToken))      SK_GetProcAddress (hModule, "OpenProcessToken"     );
  auto pGetTokenInformation   = (decltype(&GetTokenInformation))   SK_GetProcAddress (hModule, "GetTokenInformation"  );
  auto pLookupPrivilegeValue  = (decltype(&LookupPrivilegeValueW)) SK_GetProcAddress (hModule, "LookupPrivilegeValueW");
  auto pAdjustTokenPrivileges = (decltype(&AdjustTokenPrivileges)) SK_GetProcAddress (hModule, "AdjustTokenPrivileges");

  if ( pOpenProcessToken      == nullptr ||
       pGetTokenInformation   == nullptr ||
       pLookupPrivilegeValue  == nullptr ||
       pAdjustTokenPrivileges == nullptr )
  {
    FreeLibrary (hModule);
    return false;
  }

  HANDLE hToken = NULL;

  if ( FALSE ==
         pOpenProcessToken ( GetCurrentProcess (),
                     TOKEN_ADJUST_PRIVILEGES, &hToken )
     )
  {
    FreeLibrary (hModule);
    return false;
  }

  // Try to enable required privilege
  TOKEN_PRIVILEGES
    tp                           = {                  };
    tp.PrivilegeCount            =                    1;
    tp.Privileges [0].Attributes = SE_PRIVILEGE_ENABLED;

  if ( FALSE ==
         pLookupPrivilegeValue ( nullptr, L"SeDebugPrivilege",
            &tp.Privileges [0].Luid
                               )
     )
  {
    CloseHandle (hToken);
    FreeLibrary (hModule);

    return false;
  }

  auto adjustResult =
    pAdjustTokenPrivileges (
          hToken, FALSE, &tp, sizeof (tp),
          nullptr, nullptr );

  auto adjustError =
      GetLastError ();

  CloseHandle (hToken);
  FreeLibrary (hModule);

  return
   ( adjustResult != 0         &&
     adjustError  != ERROR_NOT_ALL_ASSIGNED );
}

int
RestartAsAdministrator ( int argc, char **argv )
{
  UNREFERENCED_PARAMETER (argc);
  UNREFERENCED_PARAMETER (argv);

  return 1;
}