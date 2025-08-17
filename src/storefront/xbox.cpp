// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
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
#include <storefront/xbox.h>
#include <storefront/achievements.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"   Xbox   "

#include <windows.gaming.ui.h>

ABI::Windows::Gaming::UI::IGameBarStatics* SK_GameBar_Statics = nullptr;

void
SK::Xbox::Init (void)
{
  // Init COM on this thread
  SK_AutoCOMInit _;

  using namespace ABI::Windows::Gaming::UI;

  PCWSTR         wszNamespace = L"Windows.Gaming.UI.GameBar";
  HSTRING_HEADER   hNamespaceStringHeader;
  HSTRING          hNamespaceString;

  if (SUCCEEDED (WindowsCreateStringReference (wszNamespace,
                 static_cast <UINT32> (wcslen (wszNamespace)),
                                                &hNamespaceStringHeader,
                                                &hNamespaceString)))
  {
    if (SUCCEEDED (
      RoGetActivationFactory (hNamespaceString, IID_IGameBarStatics,
                                          (void **)&SK_GameBar_Statics)))
    {
      // Keep COM loaded indefinitely, this object is persistent
      CoInitializeEx (nullptr, COINIT_MULTITHREADED);
    }
  }
}

void
SK::Xbox::Shutdown (void)
{
  if (SK_GameBar_Statics != nullptr)
  {
    std::exchange (SK_GameBar_Statics, nullptr)->Release ();
  }
}

bool
__stdcall
SK_Xbox_GetOverlayState (bool real)
{
  SK_PROFILE_SCOPED_TASK (SK_Xbox_GetOverlayState)

  std::ignore = real;

  // This is a heavy API that requires IPC, ensure this is done at most
  //   a single time per-frame...
  static bool   last_state =      false;
  static UINT64 last_frame = UINT64_MAX;

  if (last_frame == SK_GetFramesDrawn ())
    return last_state;

  if (SK_GameBar_Statics != nullptr)
  {
    boolean redirected = false;

    // This whole thing is flaky, we should be using the callback to listen for activation instead...
    SK_GameBar_Statics->get_IsInputRedirected (&redirected);

    // If redirected, but not visible, assume something is wrong and ignore the status.
    if (redirected != false)
      SK_GameBar_Statics->get_Visible         (&redirected);

    last_state = redirected != false;
    last_frame = SK_GetFramesDrawn ();

    return redirected != false;
  }

  return false;
}