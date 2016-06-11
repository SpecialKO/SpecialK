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

#include "utility.h"

#include <UserEnv.h>
#pragma comment (lib, "userenv.lib")

#include <Shlobj.h>
#pragma comment (lib, "shell32.lib")

#include <process.h>
#include <tlhelp32.h>

int
SK_MessageBox (std::wstring caption, std::wstring title, uint32_t flags)
{
  return
    MessageBox (NULL, caption.c_str (), title.c_str (), 
                flags | MB_SYSTEMMODAL | MB_TOPMOST | MB_SETFOREGROUND);
}

std::wstring
SK_GetDocumentsDir (void)
{
  HANDLE hToken;

  if (! OpenProcessToken (GetCurrentProcess (), TOKEN_READ, &hToken))
    return NULL;

  wchar_t* str;
  SHGetKnownFolderPath (FOLDERID_Documents, 0, hToken, &str);
  std::wstring ret = str;
  CoTaskMemFree (str);
  return ret;
}

bool
SK_GetUserProfileDir (wchar_t* buf, uint32_t* pdwLen)
{
  HANDLE hToken;

  if (! OpenProcessToken (GetCurrentProcess (), TOKEN_READ, &hToken))
    return false;

  if (! GetUserProfileDirectory (hToken, buf, (DWORD *)pdwLen))
    return false;

  CloseHandle (hToken);
  return true;
}

#include <string>

bool
SK_IsTrue (const wchar_t* string)
{
  if (std::wstring (string).length () == 1 &&
    string [0] == L'1')
    return true;

  if (std::wstring (string).length () != 4)
    return false;

  if (towlower (string [0]) != L't')
    return false;
  if (towlower (string [1]) != L'r')
    return false;
  if (towlower (string [2]) != L'u')
    return false;
  if (towlower (string [3]) != L'e')
    return false;

  return true;
}

// Copies a file preserving file times
void
SK_FullCopy (std::wstring from, std::wstring to)
{
  // Strip Read-Only
  SK_SetNormalFileAttribs (to);
  DeleteFile (to.c_str ());
  CopyFile   (from.c_str (), to.c_str (), FALSE);

  WIN32_FIND_DATA FromFileData;
  HANDLE hFrom = FindFirstFile (from.c_str (), &FromFileData);

  OFSTRUCT ofTo;
  ofTo.cBytes = sizeof (OFSTRUCT);

  char     szFileTo [MAX_PATH];

  WideCharToMultiByte (CP_OEMCP, 0, to.c_str (), -1, szFileTo, MAX_PATH, NULL, NULL);
  HFILE hfTo = OpenFile (szFileTo, &ofTo, NULL);

  
  HANDLE hTo = HandleToHandle64 (&hfTo);
  CloseHandle (hTo);

  // Here's where the magic happens, apply the attributes from the original file to the new one!
  SetFileTime (hTo, &FromFileData.ftCreationTime, &FromFileData.ftLastAccessTime, &FromFileData.ftLastWriteTime);

  FindClose   (hFrom);
}

//BOOL TakeOwnership (LPTSTR lpszOwnFile);

void
SK_SetNormalFileAttribs (std::wstring file)
{
  SetFileAttributes (file.c_str (), FILE_ATTRIBUTE_NORMAL);
}


bool
SK_IsAdmin (void)
{
  bool   bRet   = false;
  HANDLE hToken = 0;

  if (OpenProcessToken (GetCurrentProcess (), TOKEN_QUERY, &hToken)) {
    TOKEN_ELEVATION Elevation;
    DWORD cbSize = sizeof (TOKEN_ELEVATION);

    if (GetTokenInformation (hToken, TokenElevation, &Elevation, sizeof (Elevation), &cbSize)) {
      bRet = Elevation.TokenIsElevated != 0;
    }
  }

  if (hToken)
    CloseHandle (hToken);

  return bRet;
}

bool
SK_IsProcessRunning (const wchar_t* wszProcName)
{
  HANDLE         hProcSnap;
  PROCESSENTRY32 pe32;

  hProcSnap =
    CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);

  if (hProcSnap == INVALID_HANDLE_VALUE)
    return false;

  pe32.dwSize = sizeof PROCESSENTRY32;

  if (! Process32First (hProcSnap, &pe32)) {
    CloseHandle (hProcSnap);
    return false;
  }

  do {
    if (! lstrcmpiW (wszProcName, pe32.szExeFile)) {
      CloseHandle (hProcSnap);
      return true;
    }
  } while (Process32Next (hProcSnap, &pe32));

  CloseHandle (hProcSnap);

  return false;
}