#if 0
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

#include <SpecialK/ansel.h>
#include <SpecialK/hooks.h>
#include <SpecialK/utility.h>

#include <windef.h>
#include <concurrent_unordered_map.h>

// This is actually a thread entry-point, we'll hook this and that will
// give us the necessary execution context to "hijack" Ansel :)
typedef DWORD (WINAPI *AnselEnableCheck_pfn)(LPVOID lpParams);
typedef BOOL  (WINAPI *isAnselAvailable_pfn)(void);

static AnselEnableCheck_pfn AnselEnableCheck_Original = nullptr;
static isAnselAvailable_pfn isAnselAvailable_Original = nullptr;

Concurrency::concurrent_unordered_map <DWORD, BOOL> __SK_AnselThreads (4);


#include <SpecialK/config.h>

DWORD
WINAPI
SK_NvCamera_AnselEnableCheck_Detour (LPVOID lpParams)
{
  SetCurrentThreadDescription (L"[NvCamera] AnselEnableCheck Thread");

  DWORD dwTid = GetCurrentThreadId ();

  // Ansel thread is running, is that good or bad?
  __SK_AnselThreads [dwTid] = TRUE;

  bool orig_disable_mouse =
    config.input.mouse.disabled_to_game;
    config.input.mouse.disabled_to_game = (! SK_NvCamera_IsActive ());

  DWORD dwRet =
    AnselEnableCheck_Original (lpParams);

  config.input.mouse.disabled_to_game   = orig_disable_mouse;

  // Thread is done, so mark it as such.
  __SK_AnselThreads [dwTid] = FALSE;

  return dwRet;
}


BOOL
WINAPI
SK_NvCamera_IsActive (void)
{
  DWORD dwCount = 0;

  for ( auto it : __SK_AnselThreads )
  {
    if (it.second)
      ++dwCount;

    if (dwCount > 1)
      return TRUE;
  }

  return FALSE;
}

SK_AnselEnablement __SK_EnableAnsel =
  SK_ANSEL_FORCEUNAVAILABLE;

BOOL
WINAPI
SK_AnselSDK_isAnselAvailable_Detour (void)
{
  if (__SK_EnableAnsel == SK_ANSEL_DONTCARE)
    return isAnselAvailable_Original ();

  return
    ( __SK_EnableAnsel == SK_ANSEL_FORCEAVAILABLE ?
                                             TRUE : FALSE );
}


extern "C"
MH_STATUS
__stdcall
SK_NvCamera_ApplyHook__AnselEnableCheck (HMODULE hModule, const char* lpProcName)
{
  static bool hooked    = false;
    MH_STATUS hook_stat = MH_ERROR_ALREADY_CREATED;

  if (! hooked)
  {
    if ( MH_OK == SK_CreateDLLHook ( SK_GetModuleFullName (hModule).c_str (), lpProcName,
                                     SK_NvCamera_AnselEnableCheck_Detour,
                    static_cast_p2p <void> (    &AnselEnableCheck_Original) ) )
    {
      hooked = true;
      return MH_OK;
    }
  }

  return hook_stat;
}

extern "C"
MH_STATUS
__stdcall
SK_AnselSDK_ApplyHook__isAnselAvailable (HMODULE hModule, const char* lpProcName)
{
  static bool hooked    = false;
    MH_STATUS hook_stat = MH_ERROR_ALREADY_CREATED;

  if (! hooked)
  {
    if ( MH_OK == SK_CreateDLLHook ( SK_GetModuleFullName (hModule).c_str (), lpProcName,
                                     SK_AnselSDK_isAnselAvailable_Detour,
                     static_cast_p2p <void> (   &isAnselAvailable_Original) ) )
    {
      hooked = true;
      return MH_OK;
    }
  }

  return hook_stat;
}
#endif