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

static boolean                visible              = false;
static EventRegistrationToken visibility_event;
static boolean                input_redirected     = false;
static EventRegistrationToken input_event;
static boolean                callbacks_registered = false;

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

  static bool   last_state = false;
  static UINT64 last_frame =     0;

  // Slow your horses! You can have a cached value instead.
  if (last_frame + 10 > SK_GetFramesDrawn ())
    return last_state;

  SK_RunOnce (
    SK_LOGi0 (L"SK_Xbox_GetOverlayState: Falling back to slow codepath!")
  );

  boolean redirected = false;

  SK_GameBar_Statics->get_IsInputRedirected (&redirected);

  // If redirected, but not visible, assume something is wrong and ignore.
  if (redirected != false)
    SK_GameBar_Statics->get_Visible         (&redirected);

  last_state = redirected != false;
  last_frame = SK_GetFramesDrawn ();

  return redirected != false;
}

void
SK_Xbox_RegisterCallbacks (void)
{
  if (! SK_GameBar_Statics || callbacks_registered)
    return;

  if (callbacks_registered)
    return;

  static int
      failure_count =  0;
  if (failure_count > 10)
  {
    SK_RunOnce (
      SK_LOGi0 (L"SK_Xbox_RegisterCallbacks: too many failures, aborting!")
    );
    return;
  }

  static      std::mutex        registrar_mutex;
  auto lock = std::scoped_lock (registrar_mutex);

  if (callbacks_registered)
    return;

  SK_LOGi0 (
    L"SK_Xbox_RegisterCallbacks: attempting registration (frame=%llu)",
      SK_GetFramesDrawn ()
  );

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

  SK_GameBar_Statics->get_Visible           (         &visible);
  SK_GameBar_Statics->get_IsInputRedirected (&input_redirected);

  if (input_event.value      == 0)
    SK_GameBar_Statics->add_IsInputRedirectedChanged (input_callback.Get (),           &input_event);
  if (visibility_event.value == 0)
    SK_GameBar_Statics->add_VisibilityChanged        (visibility_callback.Get (), &visibility_event);

  if (input_event     .value != 0 &&
      visibility_event.value != 0)
  {
    callbacks_registered = true;

    SK_LOGi0 (L"SK_Xbox_RegisterCallbacks: GameBar callbacks registered");
  }

  else
    ++failure_count;
}

boolean
SK_Xbox_GetOverlayState_UsingCallbacks (void)
{
  if (! SK_GameBar_Statics)
    return false;

  return
    (visible && input_redirected);
}

bool
SK_Xbox_IsGameBarInstalled (void)
{
  return
    SK_IsProcessRunning (L"GameBar.exe");
}

bool
__stdcall
SK_Xbox_GetOverlayState (bool real)
{
  SK_PROFILE_SCOPED_TASK (SK_Xbox_GetOverlayState)

  std::ignore = real;

  static bool bHasGameBar =
    SK_Xbox_IsGameBarInstalled ();

  if (! bHasGameBar)
    return false;

  //
  // Wait until the first frame has been rendered before registering
  // Xbox/Game Bar callbacks. Some games create temporary startup or
  // splash windows before the real render loop begins, and this
  // function may be called during frame 0 before rendering is active.
  // Registering callbacks that early has been observed to cause
  // initialization ordering issues or freezes on some systems.
  //
  if (! callbacks_registered &&
      SK_GetFramesDrawn () >= 120)
  {
    SK_Xbox_RegisterCallbacks ();
  }

  // nb: The above description is likely inaccurate, the actual source
  //       of the problem appears to be failure to register callbacks on
  //         systems with gamebar not installed.
  //
  //   However, there is no known harm in a 120 frame delay at the moment,
  //     so might as well keep it as is.

  boolean has_callbacks =
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