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

  if (SK_GetFramesDrawn () < 10)
    return false;

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





#include <windows.h>
#include <atlbase.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <wincrypt.h>
#include <combaseapi.h>
#include <shlwapi.h>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")

// Helper to convert lowercase string to SHA1 Hex string
std::wstring
GetSha1Utf16Le (const std::wstring& text)
{
  HCRYPTPROV hProv = 0;
  HCRYPTHASH hHash = 0;
  
  std::wstring hexResult = L"";
  
  if (CryptAcquireContext (&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
  {
    if (CryptCreateHash (hProv, CALG_SHA1, 0, 0, &hHash))
    {
      // Hash the UTF-16LE bytes directly
      DWORD cbData =
        static_cast <DWORD> (text.length () * sizeof (wchar_t));

      if (CryptHashData (hHash, reinterpret_cast <const BYTE *>(text.c_str ()), cbData, 0))
      {
        BYTE rgbHash [20] = {};
        DWORD cbHash      = sizeof (rgbHash);

        if (CryptGetHashParam (hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
        {
          wchar_t hex [3];

          for (DWORD i = 0; i < cbHash; i++)
          {
            swprintf_s (hex, 3, L"%02x", rgbHash [i]);

            hexResult += hex;
          }
        }
      }

      CryptDestroyHash (hHash);
    }

    CryptReleaseContext (hProv, 0);
  }

  return hexResult;
}

// Helper for DPAPI CryptProtectData using LocalMachine scope
std::vector<BYTE>
ProtectLocalMachineUtf16Le (const std::wstring& text)
{
  DATA_BLOB dataIn;
  DATA_BLOB dataOut;
  
  std::vector <BYTE> encryptedBlob;
  
  dataIn.pbData = reinterpret_cast <BYTE *>(const_cast <wchar_t *> (text.c_str ()));
  dataIn.cbData = static_cast      <DWORD> (text.length () * sizeof (wchar_t));
  
  // CRYPTPROTECT_LOCAL_MACHINE matches the PowerShell DataProtectionScope::LocalMachine
  if (CryptProtectData (&dataIn, NULL, NULL, NULL, NULL, CRYPTPROTECT_LOCAL_MACHINE, &dataOut))
  {
    encryptedBlob.assign (dataOut.pbData, dataOut.pbData + dataOut.cbData);
    LocalFree            (dataOut.pbData);
  }

  return encryptedBlob;
}

// Helper to generate an upper/lowercase GUID string via COM API
std::wstring
CreateGuidString (void)
{
  GUID         guid;
  std::wstring guidStr = L"";

  if (SUCCEEDED (CoCreateGuid (&guid)))
  {
    wchar_t wszGuid [40];

    // StringFromGUID2 includes braces; we can manually format to match PowerShell's GUID string
    swprintf_s (wszGuid, 40, L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

    guidStr = wszGuid;
  }

  return guidStr;
}

// Scans target key children for an existing absolute path match
bool
CheckDuplicate (CRegKey& parentRootKey, const std::wstring& exePath)
{
  wchar_t                      child_name [256];
  DWORD   capacity = _countof (child_name);
  DWORD   idx      = 0;

  while (parentRootKey.EnumKey (idx, child_name, &capacity) == ERROR_SUCCESS)
  {
    CRegKey childKey;

    if (childKey.Open (parentRootKey, child_name, KEY_READ) == ERROR_SUCCESS)
    {
      wchar_t                 matched_path [MAX_PATH];
      ULONG chars = _countof (matched_path);

      if (childKey.QueryStringValue (L"MatchedExeFullPath", matched_path, &chars) == ERROR_SUCCESS)
      {
        if (! _wcsicmp (matched_path, exePath.c_str ()))
        {
          return true;
        }
      }
    }

    idx++;

    capacity = _countof (child_name);
  }
  return false;
}

std::wstring
SK_Win32_ToLowerInvariant (const std::wstring& input)
{
  if (input.empty ())
    return L"";

  // Determine the required buffer size
  int size = LCMapStringEx (LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, 
                            input.c_str(), -1, nullptr, 0, nullptr, nullptr, 0);

  std::wstring output (size, 0);

  // Map the string to lowercase
  LCMapStringEx (LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, 
                 input.c_str(), -1, &output[0], size, nullptr, nullptr, 0);

  // Remove the trailing null terminator that LCMapStringEx includes
  output.resize (size - 1);

  return output;
}

void
SK_WindowsGameConfig_AddGameIfNeeded (void)
{
  CRegKey              hkGameConfigParentsRoot;
  CRegKey              hkGameConfigChildRoot;
  if (ERROR_SUCCESS == hkGameConfigChildRoot.  Open (HKEY_CURRENT_USER, LR"(System\GameConfigStore\Children)") &&
      ERROR_SUCCESS == hkGameConfigParentsRoot.Open (HKEY_CURRENT_USER, LR"(System\GameConfigStore\Parents)"))
  {
    bool is_a_game = false;

    const auto wszHostAppLower =
      SK_Win32_ToLowerInvariant (SK_GetHostApp ());

    const std::wstring parentKeyName =
      GetSha1Utf16Le (wszHostAppLower.c_str ());

    CRegKey              hkParentKey;
    if (ERROR_SUCCESS == hkParentKey.Open (hkGameConfigParentsRoot, parentKeyName.c_str ()))
    {
      SK_LOGi0 (L"Parent Key: %ws opened", parentKeyName.c_str ());

      std::vector <std::wstring> child_key_names;
      ULONG                      children_key_size = 0;

      // MultiString values are double-null terminated, so a size of 2 means an empty list. Only proceed if there's something there.
      if (hkParentKey.QueryMultiStringValue (L"Children", NULL, &children_key_size) == ERROR_SUCCESS &&
                                                                 children_key_size > 2)
      {
        SK_LOGi0 (L"Children Key Size: %d characters", children_key_size);

        children_key_size += 2;

        std::vector <wchar_t> buffer (children_key_size);

        if (hkParentKey.QueryMultiStringValue (L"Children", buffer.data (), &children_key_size) == ERROR_SUCCESS)
        {
          wchar_t* p = buffer.data ();
          while (*p)
          {
            std::wstring child_str (p);

            SK_LOGi0 (L"Child Key: %ws", child_str.c_str ());

            child_key_names.push_back (child_str);

            p += child_str.length () + 1;
          }
        }
      }

      for ( auto& child_key_name : child_key_names )
      {
        ULONG    ulMatchedExeFullPathLen = MAX_PATH;
        wchar_t wszMatchedExeFullPath     [MAX_PATH] = {};

        CRegKey
            hkChildKey;
        if (hkChildKey.Open (hkGameConfigChildRoot, child_key_name.c_str ()) == ERROR_SUCCESS &&
            hkChildKey.QueryStringValue (L"MatchedExeFullPath",
                                        wszMatchedExeFullPath,
                                        &ulMatchedExeFullPathLen)            == ERROR_SUCCESS)
        {
          if (StrStrIW (wszMatchedExeFullPath, SK_GetHostApp ()))
          {
            if (config.system.log_level > 0)
              SK_ImGui_Warning (L"Game Detection: This is a game!");

            is_a_game = true;
            break;
          }
        }
      }
    }

    if (! is_a_game)
    {
      wchar_t wszTargetExePath [MAX_PATH * 2] = { };
      wchar_t wszExeParentDir  [MAX_PATH * 2] = { };

      HANDLE hFile =
        CreateFileW (SK_GetFullyQualifiedApp (), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

      if (hFile != INVALID_HANDLE_VALUE)
      {
        SK_File_GetNameFromHandle (hFile, wszTargetExePath, MAX_PATH);
        CloseHandle               (hFile);
      }

      PathResolve        (wszTargetExePath, nullptr, PRF_VERIFYEXISTS | PRF_REQUIREABSOLUTE);
                          wszTargetExePath [0] = SK_GetHostPath ()[0]; // Ensure drive letter case matches host path

      wcsncpy_s          (wszExeParentDir, MAX_PATH, wszTargetExePath, _TRUNCATE);
      PathRemoveFileSpec (wszExeParentDir);

      {
        std::vector <BYTE> parentBlob =
          ProtectLocalMachineUtf16Le (wszHostAppLower.c_str ());

        // Calculate current 64-bit UTC file time
        FILETIME                  ft = {};
        GetSystemTimeAsFileTime (&ft);

        ULARGE_INTEGER
          uli          = {};
          uli.LowPart  = ft.dwLowDateTime;
          uli.HighPart = ft.dwHighDateTime;

        ULONGLONG lastAccessedTime = uli.QuadPart;

        auto childGuid = CreateGuidString ();
        auto gameGuid  = CreateGuidString ();

        if (config.system.log_level > 0)
          SK_ImGui_Warning (SK_FormatStringW (L"New Game GUID: %ws", childGuid.c_str ()).c_str ());

        // Create subkeys inside Children and Parents path trees
        CRegKey parentSubKey, childSubKey;
        if (parentSubKey.Create (hkGameConfigParentsRoot, parentKeyName.c_str (), REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE) == ERROR_SUCCESS &&
            childSubKey.Create  (hkGameConfigChildRoot,   childGuid.c_str     (), REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE) == ERROR_SUCCESS)
        {
          // Write properties to the Child GUID target path
          childSubKey.SetStringValue (L"ExeParentDirectory", wszExeParentDir);
          childSubKey.SetDWORDValue  (L"Type",     0x1);
          childSubKey.SetDWORDValue  (L"Revision", 0x1);
          childSubKey.SetDWORDValue  (L"Flags",   0x11);
          childSubKey.SetBinaryValue (L"Parent", parentBlob.data (),
                            static_cast <ULONG> (parentBlob.size ()));
          childSubKey.SetStringValue (L"GameDVR_GameGUID",   gameGuid.c_str ());
          childSubKey.SetStringValue (L"MatchedExeFullPath", wszTargetExePath);
          childSubKey.SetQWORDValue  (L"LastAccessed",       lastAccessedTime);

          // Update parent's 'Children' MultiString properties
          std::vector <std::wstring> children_array;
          ULONG                      multi_str_size = 0;

          // Query existing MultiString buffer sizing requirement
          if(parentSubKey.QueryMultiStringValue (L"Children", NULL, &multi_str_size) == ERROR_SUCCESS &&
                                                                     multi_str_size > 2)
          {
            std::vector <wchar_t>                                buffer          (multi_str_size);
            if (parentSubKey.QueryMultiStringValue (L"Children", buffer.data (), &multi_str_size) == ERROR_SUCCESS)
            {
              wchar_t* p = buffer.data ();
              while (*p)
              {
                std::wstring child_str (p);

                children_array.push_back (child_str);

                p += child_str.length () + 1;
              }
            }
          }

          // Ensure uniqueness and avoid array duplicates before adding
          if (std::find (children_array.begin (), children_array.end (), childGuid) == children_array.end ())
          {
            children_array.push_back (childGuid);
          }

          // Reconstruct MultiString structure bytes block (sequences ending with double null bytes)
          std::vector <wchar_t> multiStringBuffer;

          for (const auto& str : children_array)
          {
            // If there is no matching Child record, then remove this from Parents in order
            //   to prevent Windows from ignoring ALL of the children.
            CRegKey
                hkChildKey;
            if (hkChildKey.Open (hkGameConfigChildRoot, str.c_str ()) == ERROR_SUCCESS)
            {
              multiStringBuffer.insert    (multiStringBuffer.end (),
                                           str.begin (), str.end ());
              multiStringBuffer.push_back (L'\0');

              // XXX: It may be a good idea to verify the EXE file in this child actually exists.
            }
          }

          multiStringBuffer.push_back (L'\0'); // Double null terminator

          // Finalize update inside parent multi-string table
          RegSetValueExW (parentSubKey.m_hKey, L"Children", 0, REG_MULTI_SZ,
              reinterpret_cast <const BYTE *> (multiStringBuffer.data ()), 
                   static_cast <DWORD>        (multiStringBuffer.size () * sizeof (wchar_t)));
        }
      }
    }
  }
}