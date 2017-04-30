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

#include <SpecialK/injection/injection.h>
#include <SpecialK/window.h>


HHOOK g_hHookCBT = nullptr;


LRESULT
CALLBACK
CBTProc (int nCode, WPARAM wParam, LPARAM lParam)
{
  if (nCode < 0) {
    CallNextHookEx (g_hHookCBT, nCode, wParam, lParam);
    return 0;
  }
  
  return CallNextHookEx (g_hHookCBT, nCode, wParam, lParam);
}



extern "C" __declspec (dllexport)
void
__stdcall
SKX_InstallCBTHook (void)
{
  // Nothing to do here, move along.
  if (g_hHookCBT != nullptr)
    return;

  HMODULE hMod;

  if ( GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                             (wchar_t *) &SKX_InstallCBTHook,
                               (HMODULE *) &hMod ) )
  {
    g_hHookCBT =
      SetWindowsHookEx (WH_CBT, CBTProc, hMod, 0);
  }
}


extern "C" __declspec (dllexport)
void
__stdcall
SKX_RemoveCBTHook (void)
{
  if (g_hHookCBT)
  {
    if (UnhookWindowsHookEx (g_hHookCBT))
      g_hHookCBT = nullptr;
  }
}


extern "C" __declspec (dllexport)
bool
__stdcall
SKX_IsHookingCBT (void)
{
  return (g_hHookCBT != nullptr);
}