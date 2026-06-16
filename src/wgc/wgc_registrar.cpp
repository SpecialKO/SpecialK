//
// Copyright 2026 Andon "Kaldaien" Coleman
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

#include <windows.h>
#include <atlbase.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <wincrypt.h>
#include <combaseapi.h>
#include <shlwapi.h>

#ifdef SK_BUILTIN
#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"WinGameCfg"

#include <SpecialK/log.h>
#include <SpecialK/config.h>
#include <../imgui/imgui.h>
#endif

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
                            input.c_str (), -1, nullptr, 0, nullptr, nullptr, 0);

  std::wstring output (size, 0);

  // Map the string to lowercase
  LCMapStringEx (LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, 
                 input.c_str (), -1, &output [0], size, nullptr, nullptr, 0);

  // Remove the trailing null terminator that LCMapStringEx includes
  output.resize (size - 1);

  return output;
}

bool
SK_WGC_AddGameIfNeeded (const wchar_t* wszExePath)
{
  CRegKey              hkGameConfigParentsRoot;
  CRegKey              hkGameConfigChildRoot;
  if (ERROR_SUCCESS == hkGameConfigChildRoot.  Open (HKEY_CURRENT_USER, LR"(System\GameConfigStore\Children)") &&
      ERROR_SUCCESS == hkGameConfigParentsRoot.Open (HKEY_CURRENT_USER, LR"(System\GameConfigStore\Parents)"))
  {
    bool is_a_game = false;

    wchar_t         wszExeFile [MAX_PATH + 2] = {};
    wcsncpy_s      (wszExeFile, MAX_PATH, wszExePath, _TRUNCATE);
    PathStripPathW (wszExeFile);

    const auto wszExeFileLower =
      SK_Win32_ToLowerInvariant (wszExeFile);

    const std::wstring parentKeyName =
      GetSha1Utf16Le (wszExeFileLower.c_str ());

    CRegKey              hkParentKey;
    if (ERROR_SUCCESS == hkParentKey.Open (hkGameConfigParentsRoot, parentKeyName.c_str ()))
    {
      std::vector <std::wstring> child_key_names;
      ULONG                      children_key_size = 0;

      // MultiString values are double-null terminated, so a size of 2 means an empty list. Only proceed if there's something there.
      if (hkParentKey.QueryMultiStringValue (L"Children", NULL, &children_key_size) == ERROR_SUCCESS &&
                                                                 children_key_size > 2)
      {
        children_key_size += 2;

        std::vector <wchar_t> buffer (children_key_size);

        if (hkParentKey.QueryMultiStringValue (L"Children", buffer.data (), &children_key_size) == ERROR_SUCCESS)
        {
          wchar_t* p = buffer.data ();
          while (*p)
          {
            std::wstring child_str (p);

#ifdef SK_BUILTIN
            SK_LOGi0 (L"Child Key: %ws", child_str.c_str ());
#endif

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
          if (StrStrIW (wszMatchedExeFullPath, wszExeFile))
          {
#ifdef SK_BUILTIN
            if (config.system.log_level > 0)
              SK_ImGui_Warning (L"Game Detection: This is a game!");
#endif

            is_a_game = true;
            break;
          }
        }
      }
    }

    if (! is_a_game)
    {
      wchar_t             wszExeParentDir [MAX_PATH + 2] = { };
      wcsncpy_s          (wszExeParentDir, MAX_PATH, wszExePath, _TRUNCATE);
      PathRemoveFileSpec (wszExeParentDir);

      {
        std::vector <BYTE> parentBlob =
          ProtectLocalMachineUtf16Le (wszExeFileLower);

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

#ifdef SK_BUILTIN
        if (config.system.log_level > 0)
          SK_ImGui_Warning (SK_FormatStringW (L"New Game GUID: %ws", childGuid.c_str ()).c_str ());
#endif

        // Create subkeys inside Children and Parents path trees
        CRegKey parentSubKey, childSubKey;
        if (parentSubKey.Create (hkGameConfigParentsRoot, parentKeyName.c_str (), REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE) == ERROR_SUCCESS &&
            childSubKey.Create  (hkGameConfigChildRoot,   childGuid.c_str     (), REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE) == ERROR_SUCCESS)
        {
          // Write properties to the Child GUID target path
          childSubKey.SetStringValue (L"ExeParentDirectory",   wszExeParentDir);
          childSubKey.SetDWORDValue  (L"Type",                             0x1);
          childSubKey.SetDWORDValue  (L"Revision",                         0x1);
          childSubKey.SetDWORDValue  (L"Flags",                           0x11);
          childSubKey.SetBinaryValue (L"Parent",           parentBlob.data (),
                                      static_cast <ULONG> (parentBlob.size ()));
          childSubKey.SetStringValue (L"GameDVR_GameGUID",   gameGuid.c_str ());
          childSubKey.SetStringValue (L"MatchedExeFullPath",        wszExePath);
          childSubKey.SetQWORDValue  (L"LastAccessed",        lastAccessedTime);

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
            }
          }

          multiStringBuffer.push_back (L'\0');

          RegSetValueExW (parentSubKey.m_hKey, L"Children", 0, REG_MULTI_SZ,
              reinterpret_cast <const BYTE *> (multiStringBuffer.data ()), 
                   static_cast <DWORD>        (multiStringBuffer.size () * sizeof (wchar_t)));

          return true;
        }
      }
    }
  }

  return false;
}