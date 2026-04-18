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

#include <wrl/client.h>
#include <wrl/event.h>
#include <wrl/wrappers/corewrappers.h>
#include <windows.gaming.ui.h>
#include <EventToken.h>

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

static boolean                visible          = false;
static EventRegistrationToken visibility_event;
static boolean                input_redirected = false;
static EventRegistrationToken input_event;

void
SK::Xbox::Shutdown (void)
{
  if (SK_GameBar_Statics != nullptr)
  {
    SK_GameBar_Statics->remove_IsInputRedirectedChanged (input_event);
    SK_GameBar_Statics->remove_VisibilityChanged        (visibility_event);

    std::exchange (SK_GameBar_Statics, nullptr)->Release ();
  }
}

boolean
SK_Xbox_GetOverlayState_WithCaching (void)
{
  if (! SK_GameBar_Statics)
    return false;

  SK_RunOnce (
    SK_LOGi0 (L"SK_Xbox_GetOverlayState: Falling back to slow codepath!")
  );

  static bool   last_state = false;
  static UINT64 last_frame =     0;

  // Slow your horses! You can have a cached value instead.
  if (last_frame > SK_GetFramesDrawn () - 10)
    return last_state;

  boolean redirected = false;

  SK_GameBar_Statics->get_IsInputRedirected (&redirected);

  // If redirected, but not visible, assume something is wrong and ignore.
  if (redirected != false)
    SK_GameBar_Statics->get_Visible         (&redirected);

  last_state = redirected != false;
  last_frame = SK_GetFramesDrawn ();

  return redirected != false;
}

boolean
SK_Xbox_GetOverlayState_UsingCallbacks (void)
{
  if (! SK_GameBar_Statics)
    return false;

  static auto visibility_callback =
    Microsoft::WRL::Callback <ABI::Windows::Foundation::IEventHandler <IInspectable *>> (
      [](IInspectable*, IInspectable*)
      {
        SK_GameBar_Statics->get_Visible (&visible);
        return S_OK;
      }
    );

  static auto input_callback =
    Microsoft::WRL::Callback <ABI::Windows::Foundation::IEventHandler <IInspectable *>> (
      [](IInspectable*, IInspectable*) {
        SK_GameBar_Statics->get_IsInputRedirected (&input_redirected);
        return S_OK;
      }
    );

  SK_RunOnce (
    SK_GameBar_Statics->get_Visible                  (                                     &visible);
    SK_GameBar_Statics->add_VisibilityChanged        (visibility_callback.Get (), &visibility_event);
    SK_GameBar_Statics->get_IsInputRedirected        (                            &input_redirected);
    SK_GameBar_Statics->add_IsInputRedirectedChanged (     input_callback.Get (),      &input_event);
  );

  return
    (visible && input_redirected);
}

bool
__stdcall
SK_Xbox_GetOverlayState (bool real)
{
  SK_PROFILE_SCOPED_TASK (SK_Xbox_GetOverlayState)

  std::ignore = real;

  SK_RunOnce (SK_Xbox_GetOverlayState_UsingCallbacks ());

  static boolean has_callbacks =
    (visibility_event.value != 0);

  //
  // Fast Codepath: Does not query state unnecessarily.
  //
  if (has_callbacks)
    return SK_Xbox_GetOverlayState_UsingCallbacks ();

  //
  // Slow Codepath: preserved in case the callback system breaks again.
  //    
  return SK_Xbox_GetOverlayState_WithCaching ();
}