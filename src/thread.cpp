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

#include <SpecialK/tls.h>
#include <SpecialK/thread.h>
#include <SpecialK/utility.h>

#include <string>


///////////////////////////////////////////////////////////////////////////
//
// Thread Name Assignment for Meaningful Debug Identification
//
//  ** Necessary given the number of lambdas serving as thread functions
//      in this codebase and the truly useless name mangling that causes.
//
///////////////////////////////////////////////////////////////////////////
HRESULT WINAPI SetThreadDescription_NOP (HANDLE, PCWSTR) { return E_NOTIMPL; }
HRESULT WINAPI GetThreadDescription_NOP (HANDLE, PWSTR*) { return E_NOTIMPL; }

const DWORD MAGIC_THREAD_EXCEPTION = 0x406D1388;

extern "C" {

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
  DWORD  dwType;     // Always 4096
  LPCSTR szName;     // Pointer to name (in user addr space).
  DWORD  dwThreadID; // Thread ID (-1=caller thread).
  DWORD  dwFlags;    // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

SetThreadDescription_pfn SetThreadDescription;
GetThreadDescription_pfn GetThreadDescription;

HRESULT
WINAPI
SetCurrentThreadDescription (_In_ PCWSTR lpThreadDescription)
{
  // Push this to the TLS datastore so we can get thread names even
  //   when no debugger is attached.
  wcsncpy (SK_TLS_Bottom ()->debug.name, lpThreadDescription, 255);


  char      szDesc [256] = { };
  wcstombs (szDesc, lpThreadDescription, 255);

  const DWORD tid =
    GetCurrentThreadId ();

  const THREADNAME_INFO info =
   { 4096, szDesc, tid, 0x0 };


  // Next: The old way (requires a debugger attached at the time we raise
  //                      this stupid exception)
  if (IsDebuggerPresent ())
  {
    __try
    {
      const DWORD argc = sizeof (info) /
                         sizeof (ULONG_PTR);
  
      RaiseException ( MAGIC_THREAD_EXCEPTION,
                         0,
                           argc,
                             reinterpret_cast <const ULONG_PTR *>(&info) );
    }
  
    __except (EXCEPTION_EXECUTE_HANDLER) { }
  }


  // Windows 7 / 8 can go no further, they will have to be happy with the
  //   TLS-backed name or a debugger must catch the exception above.
  //
  if (SetThreadDescription == &SetThreadDescription_NOP)
    return S_OK;


  // Finally, use the new API added in Windows 10...
  HRESULT hr = E_UNEXPECTED;
  HANDLE  hRealHandle;

  if ( DuplicateHandle ( GetCurrentProcess (),
                         GetCurrentThread  (),
                         GetCurrentProcess (),
                           &hRealHandle,
                             0,
                               FALSE,
                                 DUPLICATE_SAME_ACCESS ) )
  {
    hr =
      SetThreadDescription (hRealHandle, lpThreadDescription);

    CloseHandle (hRealHandle);
  }

  return hr;
}

HRESULT
WINAPI
GetCurrentThreadDescription (_Out_  PWSTR  *threadDescription)
{
  // Always use the TLS value if there is one
  if (wcslen (SK_TLS_Bottom ()->debug.name))
  {
    // This is not freed here; the caller is expected to free it!
    *threadDescription =
      (wchar_t *)LocalAlloc (LPTR, 1024);

    wcsncpy (*threadDescription, SK_TLS_Bottom ()->debug.name, 255);

    return S_OK;
  }


  // No TLS, no GetThreadDescription (...) -- we are boned :-\
  //
  if (GetThreadDescription == &GetThreadDescription_NOP)
  {
    return E_NOTIMPL;
  }


  HRESULT hr          = E_UNEXPECTED;
  HANDLE  hRealHandle = nullptr;

  if ( DuplicateHandle ( GetCurrentProcess (),
                         GetCurrentThread  (),
                         GetCurrentProcess (),
                           &hRealHandle,
                             0,
                               FALSE,
                                 DUPLICATE_SAME_ACCESS ) )
  {
    hr =
      GetThreadDescription (hRealHandle, threadDescription);

    CloseHandle (hRealHandle);
  }

  return hr;
}


bool
SK_Thread_InitDebugExtras (void)
{
  static volatile LONG run_once = FALSE;

  if (! InterlockedCompareExchange (&run_once, 1, 0))
  {
    // Only available in Windows 10
    //
    SetThreadDescription =
      (SetThreadDescription_pfn)
        GetProcAddress ( GetModuleHandle (L"Kernel32.dll"),
                                           "SetThreadDescription" );
    GetThreadDescription =
      (GetThreadDescription_pfn)
        GetProcAddress ( GetModuleHandle (L"Kernel32.dll"),
                                           "GetThreadDescription" );

    if (SetThreadDescription == nullptr)
      SetThreadDescription = &SetThreadDescription_NOP;

    if (GetThreadDescription == nullptr)
      GetThreadDescription = &GetThreadDescription_NOP;

    InterlockedIncrement (&run_once);
  }

  SK_Thread_SpinUntilAtomicMin (&run_once, 2);

  if (GetThreadDescription != &GetThreadDescription_NOP)
    return true;

  return false;
}

} /* extern "C" */