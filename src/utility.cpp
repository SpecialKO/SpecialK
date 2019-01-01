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

#include <SpecialK/utility.h>
#include <SpecialK/command.h>
#include <SpecialK/config.h>
#include <SpecialK/core.h>
#include <SpecialK/log.h>

#include <SpecialK/thread.h>
#include <SpecialK/diagnostics/modules.h>

#include <userenv.h>
#include <Shlobj.h>

#include <process.h>
#include <tlhelp32.h>

#include <Shlwapi.h>
#include <atlbase.h>

int
SK_MessageBox (std::wstring caption, std::wstring title, uint32_t flags)
{
  return
    MessageBox (nullptr, caption.c_str (), title.c_str (),
                flags | MB_SYSTEMMODAL | MB_TOPMOST | MB_SETFOREGROUND);
}

std::string
SK_WideCharToUTF8 (const std::wstring& in)
{
  int len = WideCharToMultiByte ( CP_UTF8, 0x00, in.c_str (), -1, nullptr, 0, nullptr, FALSE );

  std::string out (len * 2 + 2, '\0');

  WideCharToMultiByte           ( CP_UTF8, 0x00, in.c_str (), static_cast <int> (in.length ()), const_cast <char *> (out.data ()), len, nullptr, FALSE );

  // Pretty sure this is pointless and already implied, but it makes me laugh
  return out;
}

std::wstring
SK_UTF8ToWideChar (const std::string& in)
{
  int len = MultiByteToWideChar ( CP_UTF8, 0x00, in.c_str (), -1, nullptr, 0 );

  std::wstring out
    (len * 2 + 2, L'\0');

  MultiByteToWideChar           ( CP_UTF8, 0x00, in.c_str (), static_cast <int> (in.length ()), const_cast <wchar_t *> (out.data ()), len );

  return out;
}

bool
SK_COM_TestInit (void)
{
  CHandle  hToken (INVALID_HANDLE_VALUE);
  wchar_t* str    = nullptr;

  if (! OpenProcessToken (
          SK_GetCurrentProcess (), TOKEN_QUERY | TOKEN_IMPERSONATE |
                                   TOKEN_READ, &hToken.m_h )
     )
  {
    return false;
  }

  HRESULT hr =
    SHGetKnownFolderPath (FOLDERID_Documents, 0, hToken, &str);

  if (SUCCEEDED (hr))
  {
    CoTaskMemFree (str);

    return true;
  }

  return false;
}
std::wstring&
SK_GetDocumentsDir (void)
{
  static volatile LONG __init = 0;

  // Fast Path  (cached)
  //
  static std::wstring dir (L"");

  if (ReadAcquire (&__init) == 2)
  {
    if (! dir.empty ())
      return dir;
  }

  if (! InterlockedCompareExchange (&__init, 1, 0))
  {
    CHandle  hToken (INVALID_HANDLE_VALUE);
    wchar_t* str    = nullptr;

    if (! OpenProcessToken (
            SK_GetCurrentProcess (), TOKEN_QUERY | TOKEN_IMPERSONATE |
                                     TOKEN_READ, &hToken.m_h )
       )
    {
      dir
        = std::move (L"(null)");

      InterlockedIncrement (&__init);
      return dir;
    }

    SK_AutoCOMInit need_com;

    HRESULT hr =
      SHGetKnownFolderPath (FOLDERID_Documents, 0, hToken, &str);

    dir =
      ( SUCCEEDED (hr) ? str : std::move (L"UNKNOWN") );
    if (SUCCEEDED (hr))
    {
      InterlockedIncrement (&__init);

      CoTaskMemFree (str);
      return dir;
    }

    SK_LOG0 ( ( L"ERROR: Could not get User's Documents Directory!  [HRESULT=%x]",
                  hr ),
                L" SpecialK " );

    InterlockedIncrement (&__init);
  }

  SK_Thread_SpinUntilAtomicMin (&__init, 2);

  return dir;
}

std::wstring&
SK_GetRoamingDir (void)
{
  // Fast Path  (cached)
  //
  static std::wstring dir (L"");

  if (! dir.empty ())
    return dir;

  CHandle  hToken (INVALID_HANDLE_VALUE);
  wchar_t* str    = nullptr;

  if (! OpenProcessToken (SK_GetCurrentProcess (), TOKEN_QUERY | TOKEN_IMPERSONATE |
                                                   TOKEN_READ, &hToken.m_h))
  {
    dir = std::move (L"(null)");
    return dir;
  }

  SK_AutoCOMInit need_com;

  HRESULT hr =
    SHGetKnownFolderPath (FOLDERID_RoamingAppData, 0, hToken, &str);

  dir = (SUCCEEDED (hr) ? str : std::move (L"UNKNOWN"));

  if (SUCCEEDED (hr))
  {
    CoTaskMemFree (str);
    return dir;
  }

  return dir;
}

std::wstring
SK_GetFontsDir (void)
{
  static volatile LONG __init = 0;

  // Fast Path  (cached)
  //
  static std::wstring dir (L"");

  if (ReadAcquire (&__init) == 2)
  {
    if (! dir.empty ())
      return dir;
  }

  if (! InterlockedCompareExchange (&__init, 1, 0))
  {
    CHandle  hToken (INVALID_HANDLE_VALUE);
    wchar_t* str    = nullptr;

    if (! OpenProcessToken (SK_GetCurrentProcess (), TOKEN_QUERY | TOKEN_IMPERSONATE |
                                                     TOKEN_READ, &hToken.m_h ) )
    {
      dir = std::move (L"(null)");

      InterlockedIncrement (&__init);

      return dir;
    }

    SK_AutoCOMInit need_com;

    HRESULT hr =
      SHGetKnownFolderPath (FOLDERID_Fonts, 0, hToken, &str);

    dir = (SUCCEEDED (hr) ? str : std::move (L"UNKNOWN"));

    if (SUCCEEDED (hr))
    {
      CoTaskMemFree (str);

      InterlockedIncrement (&__init);

      return dir;
    }
  }

  return dir;
}

bool
SK_GetDocumentsDir (wchar_t* buf, uint32_t* pdwLen)
{
  const std::wstring& docs =
    SK_GetDocumentsDir ();

  if (docs.empty ())
  {
    if (buf != nullptr && pdwLen != nullptr && *pdwLen > 0)
    {
      *pdwLen = 0;
      *buf    = L'\0';
    }

    return false;
  }

  else
  {
    if (buf != nullptr && pdwLen != nullptr && *pdwLen > 0)
    {
      *buf = '\0';
      wcsncat (buf, docs.c_str (), *pdwLen);

      return true;
    }
  }

  return false;
}

bool
SK_GetUserProfileDir (wchar_t* buf, uint32_t* pdwLen)
{
  CHandle hToken;

  if (! OpenProcessToken (SK_GetCurrentProcess (), TOKEN_READ, &hToken.m_h))
    return false;

  if (! GetUserProfileDirectoryW ( hToken, buf,
                                     reinterpret_cast <DWORD *> (pdwLen)
                                 )
     )
  {
    return false;
  }

  return true;
}

bool
__stdcall
SK_CreateDirectoriesEx ( const wchar_t* wszPath, bool strip_filespec )
{
    wchar_t   wszDirPath [MAX_PATH * 2 + 1] = { };
  wcsncpy_s ( wszDirPath, MAX_PATH * 2,
                wszPath,   _TRUNCATE );

  wchar_t* wszTest =         wszDirPath;
  size_t   len     = wcslen (wszDirPath);
  for (size_t
            i = 0; i <   len; i++)
  {
    wszTest =
      CharNextW (wszTest);
  }

  if ( *wszTest == L'\\' ||
       *wszTest == L'/'     )
  {
    strip_filespec = false;
  }

  if (strip_filespec)
  {
    PathRemoveFileSpecW (wszDirPath);
    lstrcatW            (wszDirPath, LR"(\)");
  }

  // If the final path already exists, well... there's no work to be done, so
  //   don't do that crazy loop of crap below and just abort early !
  if (GetFileAttributesW (wszDirPath) != INVALID_FILE_ATTRIBUTES)
    return true;


  wchar_t     wszSubDir [MAX_PATH * 2 + 1] = { };
  wcsncpy_s ( wszSubDir, MAX_PATH * 2,
                wszDirPath, _TRUNCATE );

  wchar_t* iter;
  wchar_t* wszLastSlash     = wcsrchr (wszSubDir, L'/');
  wchar_t* wszLastBackslash = wcsrchr (wszSubDir, L'\\');

  if (wszLastSlash > wszLastBackslash)
    *wszLastSlash     = L'\0';
  else if (wszLastBackslash != nullptr)
    *wszLastBackslash = L'\0';
  else
    return false;

  for ( iter  = wszSubDir;
       *iter != L'\0';
        iter  = CharNextW (iter) )
  {
    if (*iter == L'\\' || *iter == L'/')
    {
      *iter = L'\0';

      if (GetFileAttributes (wszDirPath) == INVALID_FILE_ATTRIBUTES)
        CreateDirectoryW (wszSubDir, nullptr);

      *iter = L'\\';
    }
  }

  // The final subdirectory (FULL PATH)
  if (GetFileAttributes (wszDirPath) == INVALID_FILE_ATTRIBUTES)
    CreateDirectoryW (wszSubDir, nullptr);

  return true;
}

bool
__stdcall
SK_CreateDirectories ( const wchar_t* wszPath )
{
  return
    SK_CreateDirectoriesEx (wszPath, false);// true);
}
std::wstring
SK_EvalEnvironmentVars (const wchar_t* wszEvaluateMe)
{
  if (! wszEvaluateMe)
    return std::wstring ();

  wchar_t wszEvaluated [MAX_PATH * 2 + 1] = { };

  ExpandEnvironmentStringsW ( wszEvaluateMe,
                                wszEvaluated,
                                  MAX_PATH * 2 );

  return
    wszEvaluated;
}

#include <string>

bool
SK_IsTrue (const wchar_t* string)
{
  const wchar_t* pstr = string;

  if ( std::wstring (string).length () == 1 &&
                                 *pstr == L'1' )
    return true;

  if (std::wstring (string).length () != 4)
    return false;

  if (towlower (*pstr) != L't')
    return false;

  pstr = CharNextW (pstr);

  if (towlower (*pstr) != L'r')
    return false;

  pstr = CharNextW (pstr);

  if (towlower (*pstr) != L'u')
    return false;

  pstr = CharNextW (pstr);

  if (towlower (*pstr) != L'e')
    return false;

  return true;
}

time_t
SK_Win32_FILETIME_to_time_t ( FILETIME const& ft)
{
  ULARGE_INTEGER ull;

  ull.LowPart  = ft.dwLowDateTime;
  ull.HighPart = ft.dwHighDateTime;

  return ull.QuadPart / 10000000ULL - 11644473600ULL;
}

bool
SK_File_IsDirectory (const wchar_t* wszPath)
{
  DWORD dwAttrib =
    GetFileAttributes (wszPath);

  if ( dwAttrib != INVALID_FILE_ATTRIBUTES &&
       dwAttrib  & FILE_ATTRIBUTE_DIRECTORY )
    return true;

  return false;
}


#include <Shlwapi.h>

void
SK_File_MoveNoFail ( const wchar_t* wszOld, const wchar_t* wszNew )
{
  WIN32_FIND_DATA OldFileData  = { };
  HANDLE          hOldFind     =
    FindFirstFile (wszOld, &OldFileData);

  // Strip read-only if need be
  SK_File_SetNormalAttribs (wszNew);

  if (! MoveFileExW ( wszOld,
                        wszNew,
                          MOVEFILE_REPLACE_EXISTING ) )
  {
    wchar_t wszTemp [MAX_PATH] = { };
    GetTempFileNameW (SK_SYS_GetInstallPath ().c_str (), L"SKI", timeGetTime (), wszTemp);

    MoveFileExW ( wszNew, wszTemp, MOVEFILE_REPLACE_EXISTING );
    MoveFileExW ( wszOld, wszNew,  MOVEFILE_REPLACE_EXISTING );
  }

  // Preserve file times
  if (hOldFind != INVALID_HANDLE_VALUE)
  {
    CHandle hNewFile ( CreateFile ( wszNew,
                                      GENERIC_READ      | GENERIC_WRITE,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE |
                                                          FILE_SHARE_DELETE,
                                          nullptr,
                                            OPEN_EXISTING,
                                              GetFileAttributes (wszNew),
                                                nullptr ) );

    FindClose         (hOldFind);
    SetFileTime       ( hNewFile,
                          &OldFileData.ftCreationTime,
                            &OldFileData.ftLastAccessTime,
                              &OldFileData.ftLastWriteTime );
    SetFileAttributes (wszNew, OldFileData.dwFileAttributes);
  }
}

// Copies a file preserving file times
void
SK_File_FullCopy ( const wchar_t* from,
                   const wchar_t* to )
{
  // Strip Read-Only
  SK_File_SetNormalAttribs (to);

  DeleteFile (      to);
  CopyFile   (from, to, FALSE);

  WIN32_FIND_DATA FromFileData;
  HANDLE          hFrom =
    FindFirstFile (from, &FromFileData);

  CHandle hTo (
    CreateFile ( to,
                   GENERIC_READ      | GENERIC_WRITE,
                     FILE_SHARE_READ | FILE_SHARE_WRITE |
                                       FILE_SHARE_DELETE,
                       nullptr,
                         OPEN_EXISTING,
                           GetFileAttributes (to),
                             nullptr )
  );

  // Here's where the magic happens, apply the attributes from the
  //   original file to the new one!
  SetFileTime ( hTo,
                  &FromFileData.ftCreationTime,
                    &FromFileData.ftLastAccessTime,
                      &FromFileData.ftLastWriteTime );

  FindClose   (hFrom);
}

//BOOL TakeOwnership (LPTSTR lpszOwnFile);

BOOL
SK_File_SetAttribs ( const wchar_t *file,
                             DWORD  dwAttribs )
{
  return
    SetFileAttributesW (
      file,
        dwAttribs );
}

BOOL
SK_File_ApplyAttribMask ( const wchar_t *file,
                                  DWORD  dwAttribMask,
                                   bool  clear )
{
  DWORD dwFileMask =
    GetFileAttributesW (file);

  if (clear)
    dwAttribMask = ( dwFileMask & ~dwAttribMask );

  else
    dwAttribMask = ( dwFileMask |  dwAttribMask );

  return
    SK_File_SetAttribs (file, dwAttribMask);
}

BOOL
SK_File_SetHidden (const wchar_t *file, bool hide)
{
  return
    SK_File_ApplyAttribMask (file, FILE_ATTRIBUTE_HIDDEN, (! hide));
}

BOOL
SK_File_SetTemporary (const wchar_t *file, bool temp)
{
  return
    SK_File_ApplyAttribMask (file, FILE_ATTRIBUTE_TEMPORARY, (! temp));
}


void
SK_File_SetNormalAttribs (const wchar_t *file)
{
  SK_File_SetAttribs (file, FILE_ATTRIBUTE_NORMAL);
}


bool
SK_IsAdmin (void)
{
  bool    bRet = false;
  CHandle hToken;

  if ( OpenProcessToken ( SK_GetCurrentProcess (),
                            TOKEN_QUERY,
                              &hToken.m_h )
     )
  {
    TOKEN_ELEVATION Elevation = { };

    DWORD cbSize =
      sizeof TOKEN_ELEVATION;

    if ( GetTokenInformation ( hToken,
                                 TokenElevation,
                                   &Elevation,
                                     sizeof (Elevation),
                                       &cbSize )
       )
    {
      bRet =
        ( Elevation.TokenIsElevated != 0 );
    }
  }

  return bRet;
}

bool
SK_IsProcessRunning (const wchar_t* wszProcName)
{
  PROCESSENTRY32 pe32 = { };

  CHandle hProcSnap (
    CreateToolhelp32Snapshot ( TH32CS_SNAPPROCESS,
                                 0 )
  );

  if (hProcSnap == INVALID_HANDLE_VALUE)
    return false;

  pe32.dwSize =
    sizeof PROCESSENTRY32;

  if (! Process32First ( hProcSnap,
                           &pe32    )
     )
  {
    return false;
  }

  do
  {
    if (! SK_Path_wcsicmp ( wszProcName,
                              pe32.szExeFile )
       )
    {
      return true;
    }
  } while ( Process32Next ( hProcSnap,
                              &pe32    )
          );

  return false;
}



typedef FARPROC (WINAPI *GetProcAddress_pfn)(HMODULE,LPCSTR);
                  extern GetProcAddress_pfn
                         GetProcAddress_Original;

FARPROC
WINAPI
SK_GetProcAddress (HMODULE hMod, const char* szFunc)
{
  if (hMod != nullptr)
  {
    if (GetProcAddress_Original != nullptr)
    {
      return
        GetProcAddress_Original (hMod, szFunc);
    }

    return
      GetProcAddress (hMod, szFunc);
  }

  return nullptr;
}

FARPROC
WINAPI
SK_GetProcAddress (const wchar_t* wszModule, const char* szFunc)
{
  HMODULE hMod =
    SK_GetModuleHandle (wszModule);

  if (hMod != nullptr)
  {
    return
      SK_GetProcAddress (hMod, szFunc);
  }

  return nullptr;
}

std::wstring
SK_GetModuleFullName (HMODULE hDll)
{
  wchar_t wszDllFullName [MAX_PATH * 2 + 1] = { };

  GetModuleFileName ( hDll,
                        wszDllFullName,
                          MAX_PATH );

  return wszDllFullName;
}

std::wstring
SK_GetModuleName (HMODULE hDll)
{
  wchar_t wszDllFullName [MAX_PATH * 2 + 1] = { };

  GetModuleFileName ( hDll,
                        wszDllFullName,
                          MAX_PATH );

  const wchar_t* wszShort =
    wcsrchr (wszDllFullName, L'\\');

  if (wszShort == nullptr)
    wszShort = wszDllFullName;
  else
    wszShort = CharNextW (wszShort);

  return wszShort;
}

HMODULE
SK_GetModuleFromAddr (LPCVOID addr)
{
  HMODULE hModOut = nullptr;

  if ( GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT |
                           GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                             (LPCWSTR)addr,
                               &hModOut
                         )
     )
  {
    return hModOut;
  }

  return static_cast <HMODULE> (INVALID_HANDLE_VALUE);
}

std::wstring
SK_GetModuleNameFromAddr (LPCVOID addr)
{
  HMODULE hModOut =
    SK_GetModuleFromAddr (addr);

  if (hModOut != INVALID_HANDLE_VALUE && hModOut > 0)
    return SK_GetModuleName (hModOut);

  return
    L"#Invalid.dll#";
}

std::wstring
SK_GetModuleFullNameFromAddr (LPCVOID addr)
{
  HMODULE hModOut =
    SK_GetModuleFromAddr (addr);

  if (hModOut != INVALID_HANDLE_VALUE && hModOut > 0)
    return SK_GetModuleFullName (hModOut);

  return
    L"#Extremely#Invalid.dll#";
}

std::wstring
SK_MakePrettyAddress (LPCVOID addr, DWORD /*dwFlags*/)
{
  return
    SK_FormatStringW ( L"( %s ) + %6xh",
      SK_StripUserNameFromPathW (
                         SK_GetModuleFullNameFromAddr (addr).data ()),
                                            (uintptr_t)addr -
                      (uintptr_t)SK_GetModuleFromAddr (addr));
}




struct SK_MemScan_Params__v0
{
  enum Privilege
  {
    Allowed    = true,
    Disallowed = false,
    DontCare   = (DWORD_PTR)-1
  };

  struct
  {
    Privilege execute = DontCare;
    Privilege read    = Allowed;
    Privilege write   = DontCare;
  } privileges;

  enum MemType
  {
    ImageCode  = SEC_IMAGE,
    FileData   = SEC_FILE,
    HeapMemory = SEC_COMMIT
  } mem_type;

  bool testPrivs (const MEMORY_BASIC_INFORMATION& mi)
  {
    if (mi.AllocationProtect == 0)
      return false;

    bool valid = true;


    if (privileges.execute != DontCare)
    {
      bool exec_matches = true;

      switch (mi.Protect)
      {
        case PAGE_EXECUTE:
        case PAGE_EXECUTE_READ:
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
          if (privileges.execute != Allowed)
            exec_matches = false;
          break;

        default:
          if (privileges.execute == Disallowed)
            exec_matches = false;
          break;
      }

      valid &= exec_matches;
    }


    if (privileges.read != DontCare)
    {
      bool read_matches = true;

      switch (mi.Protect)
      {
        case PAGE_READONLY:
        case PAGE_READWRITE:
        case PAGE_EXECUTE_READ:
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
          if (privileges.read != Allowed)
            read_matches = false;
          break;

        default:
          if (privileges.read == Disallowed)
            read_matches = false;
          break;
      }

      valid &= read_matches;
    }


    if (privileges.write != DontCare)
    {
      bool write_matches = true;

      switch (mi.Protect)
      {
        case PAGE_READWRITE:
        case PAGE_WRITECOPY:
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
          if (privileges.write != Allowed)
            write_matches = false;
          break;

        default:
          if (privileges.write == Disallowed)
            write_matches = false;
          break;
      }

      valid &= write_matches;
    }

    return valid;
  }
};


bool
SK_ValidatePointer (LPCVOID addr)
{
  MEMORY_BASIC_INFORMATION minfo = { };

  bool bFail = true;

  if (VirtualQuery (addr, &minfo, sizeof minfo))
  {
    bFail = false;

    if ((minfo.AllocationProtect == 0 ) ||
      (  minfo.Protect & PAGE_NOACCESS)   )
    {
      bFail = true;
    }
  }

  if (bFail)
  {
    SK_LOG0 ( ( L"Address Validation for addr. %s FAILED!",
                  SK_MakePrettyAddress (addr).c_str () ),
                L" SK Debug " );
  }

  return (! bFail);
}

bool
SK_IsAddressExecutable (LPCVOID addr)
{
  MEMORY_BASIC_INFORMATION minfo = { };

  if (VirtualQuery (addr, &minfo, sizeof minfo))
  {
    if (SK_ValidatePointer (addr))
    {
      static SK_MemScan_Params__v0 test_exec;

      SK_RunOnce(
        test_exec.privileges.execute = SK_MemScan_Params__v0::Allowed
      );

      if (test_exec.testPrivs (minfo))
      {
        return true;
      }
    }
  }

  SK_LOG0 ( ( L"Executable Address Validation for addr. %s FAILED!",
                SK_MakePrettyAddress (addr).c_str () ),
              L" SK Debug " );

  return false;
}

void
SK_LogSymbolName (LPCVOID addr)
{
  UNREFERENCED_PARAMETER (addr);

#ifdef _DEBUG
  char szSymbol [256] = { };

  SK_GetSymbolNameFromModuleAddr ( SK_GetModuleFromAddr (addr),
                                              (uintptr_t)addr,
                                                         szSymbol, 255 );

  SK_LOG0 ( ( L"=> %hs", szSymbol ), L"SymbolName" );
#endif
}


PROCESSENTRY32
FindProcessByName (const wchar_t* wszName)
{
  PROCESSENTRY32 pe32 = { };

  CHandle hProcessSnap (
    CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0)
  );

  if (hProcessSnap == INVALID_HANDLE_VALUE)
    return pe32;

  pe32.dwSize = sizeof (PROCESSENTRY32);

  if (! Process32First (hProcessSnap, &pe32))
  {
    return pe32;
  }

  do
  {
    if (wcsstr (pe32.szExeFile, wszName))
      return pe32;
  } while (Process32Next (hProcessSnap, &pe32));

  return pe32;
}

std::wstring
SK_GetRTSSInstallDir (void)
{
  //PROCESSENTRY32 pe32 =
  //  FindProcessByName (L"RTSS.exe");
  //
  wchar_t wszPath [MAX_PATH] = { };
  //
  //if (wcsstr (pe32.szExeFile, L"RTSS.exe"))
  //{
  //  CHandle hProcess (
  //    OpenProcess (PROCESS_QUERY_LIMITED_INFORMATION , FALSE, pe32.th32ProcessID)
  //  );
  //
  //  DWORD len = MAX_PATH;
  //  QueryFullProcessImageName (hProcess, 0, wszPath, &len);
  //
  //  wchar_t* wszRTSS =
  //    wcsstr (wszPath, L"RTSS.exe");
  //
  //  if (wszRTSS != nullptr)
  //    *wszRTSS = L'\0';
  //}

  return wszPath;
}

#include <SpecialK/ini.h>

iSK_INI*
SK_GetDLLConfig (void)
{
  extern iSK_INI* dll_ini;
  return dll_ini;
}

iSK_INI*
SK_GetOSDConfig (void)
{
  extern iSK_INI* osd_ini;
  return osd_ini;
}


extern BOOL APIENTRY DllMain (HMODULE hModule,
                              DWORD   ul_reason_for_call,
                              LPVOID  /* lpReserved */);

#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/render/d3d9/d3d9_backend.h>
#include <SpecialK/render/gl/opengl_backend.h>
#include <SpecialK/input/dinput8_backend.h>

void
__stdcall
SK_SelfDestruct (void)
{
  if (! InterlockedCompareExchange (&__SK_DLL_Ending, 1, 0))
  {
    BOOL
    __stdcall
    SK_Detach (DLL_ROLE role);

    SK_Detach (SK_GetDLLRole ());
  }
}




#include <SpecialK/tls.h>

HMODULE
SK_GetCallingDLL (LPCVOID pReturn)
{
  HMODULE hCallingMod = nullptr;

  GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT |
                      GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                        static_cast <const wchar_t *> (pReturn),
                          &hCallingMod );

  return hCallingMod;
}

std::wstring
SK_GetCallerName (LPCVOID pReturn)
{
  return SK_GetModuleName (SK_GetCallingDLL (pReturn));
}

#include <queue>

std::queue <DWORD>
SK_SuspendAllOtherThreads (void)
{
  std::queue <DWORD> threads;

  CHandle hSnap (
    CreateToolhelp32Snapshot (TH32CS_SNAPTHREAD, 0)
  );

  if (hSnap != INVALID_HANDLE_VALUE)
  {
    THREADENTRY32 tent        = {                  };
                  tent.dwSize = sizeof THREADENTRY32;

    if (Thread32First (hSnap, &tent))
    {
      //bool locked =
      //  dll_log.lock ();

      do
      {
        if ( tent.dwSize >= FIELD_OFFSET (THREADENTRY32, th32OwnerProcessID) +
                                  sizeof (tent.th32OwnerProcessID) )
        {
          if ( tent.th32ThreadID       != GetCurrentThreadId  () &&
               tent.th32OwnerProcessID == GetCurrentProcessId () )
          {
            CHandle hThread (
              OpenThread (THREAD_SUSPEND_RESUME, FALSE, tent.th32ThreadID)
            );

            if (hThread != nullptr)
            {
              threads.push  (tent.th32ThreadID);

              SuspendThread (hThread);
            }
          }
        }

        tent.dwSize = sizeof (tent);
      } while (Thread32Next (hSnap, &tent));

      //if (locked)
      //  dll_log.unlock ();
    }
  }

  return threads;
}

std::queue <DWORD>
SK_SuspendAllThreadsExcept (std::set <DWORD>& exempt_tids)
{
  std::queue <DWORD> threads;

  CHandle hSnap (
    CreateToolhelp32Snapshot (TH32CS_SNAPTHREAD, 0)
  );

  if (hSnap != INVALID_HANDLE_VALUE)
  {
    THREADENTRY32 tent        = {                  };
                  tent.dwSize = sizeof THREADENTRY32;

    if (Thread32First (hSnap, &tent))
    {
      //bool locked =
      //  dll_log.lock ();

      do
      {
        if ( tent.dwSize >= FIELD_OFFSET (THREADENTRY32, th32OwnerProcessID) +
                                  sizeof (tent.th32OwnerProcessID) )
        {
          if ( (! exempt_tids.count (tent.th32ThreadID)) &&
                                     tent.th32ThreadID       != GetCurrentThreadId  () &&
                                     tent.th32OwnerProcessID == GetCurrentProcessId () )
          {
            CHandle hThread (
              OpenThread (THREAD_SUSPEND_RESUME, FALSE, tent.th32ThreadID)
            );

            if (hThread != nullptr)
            {
              threads.push  (tent.th32ThreadID);

              SuspendThread (hThread);
            }
          }
        }

        tent.dwSize = sizeof (tent);
      } while (Thread32Next (hSnap, &tent));

      //if (locked)
      //  dll_log.unlock ();
    }
  }

  return threads;
}

void
SK_ResumeThreads (std::queue <DWORD> threads)
{
  while (! threads.empty ())
  {
    DWORD tid = threads.front ();

    CHandle hThread (
      OpenThread (THREAD_SUSPEND_RESUME, FALSE, tid)
    );

    if (hThread != nullptr)
    {
      ResumeThread (hThread);
    }

    threads.pop ();
  }
}

void
__stdcall
SK_TestImports (          HMODULE  hMod,
                 sk_import_test_s *pTests,
                              int  nCount )
{
  DBG_UNREFERENCED_PARAMETER (hMod);

  int hits = 0;

  __try
  {
    auto                pImgBase =
      (uintptr_t)GetModuleHandle (nullptr);

             MEMORY_BASIC_INFORMATION minfo;
    VirtualQuery ((LPCVOID)pImgBase, &minfo, sizeof minfo);

    pImgBase =
      (uintptr_t)minfo.BaseAddress;

    auto        pNtHdr   =
      PIMAGE_NT_HEADERS ( pImgBase + PIMAGE_DOS_HEADER (pImgBase)->e_lfanew );

    PIMAGE_DATA_DIRECTORY    pImgDir  =
        &pNtHdr->OptionalHeader.DataDirectory [IMAGE_DIRECTORY_ENTRY_IMPORT];

    auto pImpDesc =
      PIMAGE_IMPORT_DESCRIPTOR (
        pImgBase + pImgDir->VirtualAddress
      );

    //dll_log.Log (L"[Import Tbl] Size=%lu", pImgDir->Size);

    if (pImgDir->Size < (1024 * 8192))
    {
      auto end =
        reinterpret_cast <uintptr_t> (pImpDesc) + pImgDir->Size;

      while (reinterpret_cast <uintptr_t> (pImpDesc) < end)
      {
        __try
        {
          if (pImpDesc->Name == 0x00)
          {
            ++pImpDesc;
            continue;
          }

          const auto* szImport =
            reinterpret_cast <const char *> (
              pImgBase + (pImpDesc++)->Name
            );

          //dll_log.Log (L"%hs", szImport);

          for (int i = 0; i < nCount; i++)
          {
            if ((! pTests [i].used) && (! StrCmpIA (szImport, pTests [i].szModuleName)))
            {
              pTests [i].used = true;
                       ++hits;
            }
          }

          if (hits == nCount)
            break;
        }

        __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ) ?
                     EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
        {
        }
      }
    }
  }

  __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ) ?
               EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
  {
    dll_log.Log ( L"[Import Tbl] Access Violation Attempting to "
                  L"Walk Import Table." );
  }

  if (hits == 0)
  {
    // Supplement the import table test with a check for residency,
    //   this may catch games that load graphics APIs dynamically.
    for (int i = 0; i < nCount; i++)
    {
      if ( GetModuleHandleExA ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                  pTests [i].szModuleName,
                                    &hMod ) )
        pTests [i].used = true;
    }
  }
}

//
// This prototype is now completely ridiculous, this "design" sucks...
//   FIXME!!
//
void
SK_TestRenderImports ( HMODULE hMod,
                       bool*   gl,
                       bool*   vulkan,
                       bool*   d3d9,
                       bool*   dxgi,
                       bool*   d3d11,
                       bool*   d3d8,
                       bool*   ddraw,
                       bool*   glide )
{
  static sk_import_test_s tests [] = { { "OpenGL32.dll",  false },
                                       { "vulkan-1.dll",  false },
                                       { "d3d9.dll",      false },
                                       //{ "dxgi.dll",     false },
                                       { "d3d11.dll",     false },
                                       { "d3d8.dll",      false },
                                       { "ddraw.dll",     false },
                                       { "d3dx11_43.dll", false },

                                       // 32-bit only
                                       { "glide.dll",     false } };

  SK_TestImports (hMod, tests, sizeof (tests) / sizeof sk_import_test_s);

  *gl     = tests [0].used;
  *vulkan = tests [1].used;
  *d3d9   = tests [2].used;
  *dxgi   = false;
//*dxgi   = tests [3].used;
  *d3d11  = tests [3].used;
  *d3d8   = tests [4].used;
  *ddraw  = tests [5].used;
  *d3d11 |= tests [6].used;
  *dxgi  |= tests [6].used;
  *glide  = tests [7].used;
}

int
SK_Path_wcsicmp (const wchar_t* wszStr1, const wchar_t* wszStr2)
{
  int ret =
    CompareString ( LOCALE_INVARIANT,
                      NORM_IGNORECASE | NORM_IGNOREWIDTH |
                      NORM_IGNORENONSPACE,
                        wszStr1, lstrlenW (wszStr1),
                          wszStr2, lstrlenW (wszStr2) );

  // To make this a drop-in replacement for wcsicmp, subtract
  //   2 from non-zero return values
  return (ret != 0) ?
    (ret - 2) : 0;
}

const wchar_t*
SK_Path_wcsrchr (const wchar_t* wszStr, wchar_t wchr)
{
             int len     = 0;
  const wchar_t* pwszStr = wszStr;

  for (len = 0; len < MAX_PATH; ++len, pwszStr = CharNextW (pwszStr))
  {
    if (*pwszStr == L'\0')
      break;
  }

  const wchar_t* wszSearch = pwszStr;

  while (wszSearch >= wszStr)
  {
    if (*wszSearch == wchr)
      break;

    wszSearch = CharPrevW (wszStr, wszSearch);
  }

  return (wszSearch < wszStr) ?
           nullptr : wszSearch;
}

const wchar_t*
SK_Path_wcsstr (const wchar_t* wszStr, const wchar_t* wszSubStr)
{
#if 0
  int            len     =
    min (lstrlenW (wszSubStr), MAX_PATH);

  const wchar_t* it       = wszStr;
  const wchar_t* wszScan  = it;
  const wchar_t* wszBegin = it;

  int            idx     = 0;

  while (it < (wszStr + MAX_PATH)) {
    bool match = (*wszScan == wszSubStr [idx]);

    if (match) {
      if (++idx == len)
        return wszBegin;

      ++it;
    }

    else {
      if (it > (wszStr + MAX_PATH - len))
        break;

      it  = ++wszBegin;
      idx = 0;
    }
  }

  return (it <= (wszStr + MAX_PATH - len)) ?
           wszBegin : nullptr;
#else
  return StrStrIW (wszStr, wszSubStr);
#endif
}


#include <Winver.h>
static HMODULE hModVersion = nullptr;

__forceinline
bool
SK_Import_VersionDLL (void)
{
  if (hModVersion == nullptr)
  {
    //if(!(hModVersion = GetModuleHandle          (L"Version.dll")))
    {      hModVersion = SK_Modules.LoadLibraryLL (L"Version.dll"); }
  }
  //Api-ms-win-core-version-l1-1-0.dll");

  return
    ( hModVersion != nullptr );
}

BOOL
WINAPI
SK_VerQueryValueW (
  _In_                                                                       LPCVOID pBlock,
  _In_                                                                       LPCWSTR lpSubBlock,
  _Outptr_result_buffer_ (_Inexpressible_ ("buffer can be PWSTR or DWORD*")) LPVOID* lplpBuffer,
  _Out_                                                                       PUINT  puLen )
{
  SK_Import_VersionDLL ();

  using VerQueryValueW_pfn = BOOL (WINAPI *)(
    _In_                                                                      LPCVOID  pBlock,
    _In_                                                                      LPCWSTR  lpSubBlock,
    _Outptr_result_buffer_ (_Inexpressible_ ("buffer can be PWSTR or DWORD*")) LPVOID* lplpBuffer,
    _Out_                                                                       PUINT  puLen );

  static auto imp_VerQueryValueW =
    (VerQueryValueW_pfn)
       GetProcAddress (hModVersion, "VerQueryValueW");

  return
    imp_VerQueryValueW ( pBlock, lpSubBlock, lplpBuffer, puLen );
}

BOOL
WINAPI
SK_GetFileVersionInfoExW (_In_                      DWORD   dwFlags,
                          _In_                      LPCWSTR lpwstrFilename,
                          _Reserved_                DWORD   dwHandle,
                          _In_                      DWORD   dwLen,
                          _Out_writes_bytes_(dwLen) LPVOID  lpData)
{
  SK_Import_VersionDLL ();

  using GetFileVersionInfoExW_pfn = BOOL (WINAPI *)(
    _In_                      DWORD   dwFlags,
    _In_                      LPCWSTR lpwstrFilename,
    _Reserved_                DWORD   dwHandle,
    _In_                      DWORD   dwLen,
    _Out_writes_bytes_(dwLen) LPVOID  lpData
  );

  static auto imp_GetFileVersionInfoExW =
    (GetFileVersionInfoExW_pfn)
       GetProcAddress (hModVersion, "GetFileVersionInfoExW");

  return
    imp_GetFileVersionInfoExW ( dwFlags, lpwstrFilename,
                                dwHandle, dwLen, lpData );
}

DWORD
WINAPI
SK_GetFileVersionInfoSizeExW ( _In_  DWORD   dwFlags,
                               _In_  LPCWSTR lpwstrFilename,
                               _Out_ LPDWORD lpdwHandle )
{
  SK_Import_VersionDLL ();

  using GetFileVersionInfoSizeExW_pfn = DWORD (WINAPI *)(
    _In_  DWORD   dwFlags,
    _In_  LPCWSTR lpwstrFilename,
    _Out_ LPDWORD lpdwHandle
  );

  static auto imp_GetFileVersionInfoSizeExW =
    (GetFileVersionInfoSizeExW_pfn)
       GetProcAddress (hModVersion, "GetFileVersionInfoSizeExW");

  return
    imp_GetFileVersionInfoSizeExW ( dwFlags, lpwstrFilename,
                                    lpdwHandle );
}

bool
__stdcall
SK_IsDLLSpecialK (const wchar_t* wszName)
{
  UINT     cbTranslatedBytes = 0,
           cbProductBytes    = 0;

  DWORD    dwHandle          = 0;
  DWORD    dwSize            =
    SK_GetFileVersionInfoSizeExW ( FILE_VER_GET_NEUTRAL,
                                     wszName, &dwHandle );

  if (dwSize < 128)
    return false;

  dwSize =
 (dwSize + 1) * sizeof (wchar_t);

  SK_TLS *pTLS =
    ReadAcquire (&__SK_DLL_Attached) ?
      SK_TLS_Bottom () : nullptr;

  uint8_t *cbData =
    (pTLS != nullptr) ?
      (uint8_t *)pTLS->scratch_memory.cmd.alloc (dwSize + 1, true) :
      (uint8_t *)SK_LocalAlloc (LPTR,            dwSize + 1);

  wchar_t* wszProduct        = nullptr; // Will point somewhere in cbData

  struct LANGANDCODEPAGE {
    WORD wLanguage;
    WORD wCodePage;
  } *lpTranslate = nullptr;

  wchar_t wszFullyQualifiedName [MAX_PATH * 2 + 1] = { };

  lstrcatW (wszFullyQualifiedName, SK_GetHostPath ());
  lstrcatW (wszFullyQualifiedName, L"\\");
  lstrcatW (wszFullyQualifiedName, wszName);

  BOOL bRet =
    SK_GetFileVersionInfoExW ( FILE_VER_GET_NEUTRAL |
                               FILE_VER_GET_PREFETCHED,
                                 wszFullyQualifiedName,
                                   0x00,
                                     dwSize,
                                       cbData );

  if (! bRet) return false;

  if ( SK_VerQueryValueW ( cbData,
                           TEXT ("\\VarFileInfo\\Translation"),
               static_cast_p2p <void> (&lpTranslate),
                                       &cbTranslatedBytes ) && cbTranslatedBytes &&
                                                               lpTranslate )
  {
    wchar_t      wszPropName [64] = { };
    _snwprintf ( wszPropName, 64,
                   LR"(\StringFileInfo\%04x%04x\ProductName)",
                     lpTranslate   [0].wLanguage,
                       lpTranslate [0].wCodePage );

    SK_VerQueryValueW ( cbData,
                          wszPropName,
              static_cast_p2p <void> (&wszProduct),
                                      &cbProductBytes );

    return (cbProductBytes && (StrStrIW (wszProduct, L"Special K")));
  }

  return false;
}

std::wstring
__stdcall
SK_GetDLLVersionStr (const wchar_t* wszName)
{
  UINT     cbTranslatedBytes = 0,
           cbProductBytes    = 0,
           cbVersionBytes    = 0;

  DWORD    dwHandle          = 0;
  DWORD    dwSize            =
    SK_GetFileVersionInfoSizeExW ( FILE_VER_GET_NEUTRAL,
                                     wszName, &dwHandle );

  if (dwSize < 128)
    return L"N/A";

  (++dwSize) *=
    sizeof (wchar_t);

  SK_TLS *pTLS =
    ReadAcquire (&__SK_DLL_Attached) ?
      SK_TLS_Bottom () : nullptr;

  uint8_t *cbData =
    (pTLS != nullptr) ?
      (uint8_t *)pTLS->scratch_memory.cmd.alloc (dwSize + 1, true) :
      (uint8_t *)SK_LocalAlloc (LPTR,            dwSize + 1);

  wchar_t* wszFileDescrip = nullptr; // Will point somewhere in cbData
  wchar_t* wszFileVersion = nullptr; // "

  struct LANGANDCODEPAGE {
    WORD wLanguage;
    WORD wCodePage;
  } *lpTranslate = nullptr;

  BOOL bRet =
    SK_GetFileVersionInfoExW ( FILE_VER_GET_NEUTRAL |
                               FILE_VER_GET_PREFETCHED,
                                 wszName,
                                   0x00,
                                     dwSize,
                                       cbData );

  if (! bRet)
    return L"N/A";

  if ( SK_VerQueryValueW ( cbData,
                             TEXT ("\\VarFileInfo\\Translation"),
                 static_cast_p2p <void> (&lpTranslate),
                                         &cbTranslatedBytes ) && cbTranslatedBytes &&
                                                                 lpTranslate )
  {
    wchar_t wszPropName [64] = { };

    _snwprintf ( wszPropName, 64,
                  LR"(\StringFileInfo\%04x%04x\FileDescription)",
                    lpTranslate   [0].wLanguage,
                      lpTranslate [0].wCodePage );

    SK_VerQueryValueW ( cbData,
                          wszPropName,
              static_cast_p2p <void> (&wszFileDescrip),
                                      &cbProductBytes );

    _snwprintf ( wszPropName, 64,
                   LR"(\StringFileInfo\%04x%04x\FileVersion)",
                     lpTranslate   [0].wLanguage,
                       lpTranslate [0].wCodePage );

    SK_VerQueryValueW ( cbData,
                          wszPropName,
              static_cast_p2p <void> (&wszFileVersion),
                                      &cbVersionBytes );
  }

  if ( cbTranslatedBytes == 0 ||
         (cbProductBytes == 0 && cbVersionBytes == 0) )
  {
    return L"  ";
  }

  std::wstring ret = L"";

  if (cbProductBytes)
  {
    ret.append (wszFileDescrip);
    ret.append (L"  ");
  }

  if (cbVersionBytes)
    ret.append (wszFileVersion);

  return ret;
}



LPVOID __SK_base_img_addr = nullptr;
LPVOID __SK_end_img_addr  = nullptr;

void*
__stdcall
SK_Scan (const void* pattern, size_t len, const void* mask)
{
  return SK_ScanAligned (pattern, len, mask);
}


void*
__stdcall
SKX_ScanAlignedEx ( const void* pattern, size_t len,   const void* mask,
                          void* after,   int    align,    uint8_t* base_addr,

                          SK_MemScan_Params__v0 params =
                          SK_MemScan_Params__v0 ()       )
{
  MEMORY_BASIC_INFORMATION minfo = { };
  VirtualQuery (base_addr, &minfo, sizeof minfo);

  //
  // Valid Win32 software have valid NT headers even after relocation ...
  //
  //   VMProtect is not valid software and should think about what it has done!
  //
#ifdef VMPROTECT_ANTI_FUDGE
  IMAGE_DOS_HEADER* pDOS =
    (IMAGE_DOS_HEADER *)minfo.AllocationBase;
  IMAGE_NT_HEADERS* pNT  =
    (IMAGE_NT_HEADERS *)((uintptr_t)(pDOS + pDOS->e_lfanew));

  uint8_t* end_addr = base_addr + pNT->OptionalHeader.SizeOfImage;
#else
           base_addr = static_cast <uint8_t *> (minfo.BaseAddress);
  uint8_t* end_addr  = static_cast <uint8_t *> (minfo.BaseAddress) + minfo.RegionSize;

  ///if (base_addr != (uint8_t *)0x400000)
  ///{
  ///  static bool warned = false;
  ///  if (! warned)
  ///  {
  ///    dll_log.Log ( L"[ Sig Scan ] Expected module base addr. 40000h, but got: %ph",
  ///                    base_addr );
  ///    warned = true;
  ///  }
  ///}

  size_t pages = 0;

#ifndef _WIN64
  // Account for possible overflow in 32-bit address space in very rare (address randomization) cases
uint8_t* const PAGE_WALK_LIMIT =
  base_addr + static_cast <uintptr_t>(1UL << 27) > base_addr ?
                                                   base_addr + static_cast      <uintptr_t>( 1UL << 27) :
                                                               reinterpret_cast <uint8_t *>(~0UL      );
#else
  // Dark Souls 3 needs this, its address space is completely random to the point
  //   where it may be occupying a range well in excess of 36 bits. Probably a stupid
  //     anti-cheat attempt.
uint8_t* const PAGE_WALK_LIMIT = (base_addr + static_cast <uintptr_t>(1ULL << 36));
#endif

  //
  // For practical purposes, let's just assume that all valid games have less than 256 MiB of
  //   committed executable image data.
  //
  while (VirtualQuery (end_addr, &minfo, sizeof minfo) && end_addr < PAGE_WALK_LIMIT)
  {
    if (minfo.Protect & PAGE_NOACCESS || (! (minfo.Type & MEM_IMAGE)))
      break;

    pages += VirtualQuery (end_addr, &minfo, sizeof minfo);

    end_addr =
      static_cast <uint8_t *> (minfo.BaseAddress) + minfo.RegionSize;
  }

  if (end_addr > PAGE_WALK_LIMIT)
  {
    static bool warned = false;

    if (! warned)
    {
      dll_log.Log ( L"[ Sig Scan ] Module page walk resulted in end addr. out-of-range: %ph",
                      end_addr );
      dll_log.Log ( L"[ Sig Scan ]  >> Restricting to %ph",
                      PAGE_WALK_LIMIT );
      warned = true;
    }

    end_addr =
      PAGE_WALK_LIMIT;
  }

#if 0
  dll_log->Log ( L"[ Sig Scan ] Module image consists of %zu pages, from %ph to %ph",
                  pages,
                    base_addr,
                      end_addr );
#endif
#endif

  __SK_base_img_addr = base_addr;
  __SK_end_img_addr  = end_addr;

  uint8_t* begin = std::max (static_cast <uint8_t *> (after) + align, base_addr);
  uint8_t* it    = begin;
  size_t   idx   = 0;

  static MODULEINFO mi_sk = {};

  SK_RunOnce (
    GetModuleInformation ( SK_GetCurrentProcess (),
      SK_GetDLL (),
              &mi_sk,
       sizeof MODULEINFO )
  );

  while (it < end_addr)
  {
    VirtualQuery (it, &minfo, sizeof minfo);

    // Bail-out once we walk into an address range that is not resident, because
    //   it does not belong to the original executable.
    if (minfo.RegionSize == 0)
      break;

    uint8_t* next_rgn =
     (uint8_t *)minfo.BaseAddress + minfo.RegionSize;

    if ( (! (minfo.Type     & MEM_IMAGE))   ||
         (! (minfo.State    & MEM_COMMIT))  ||
             minfo.Protect  & PAGE_NOACCESS ||
             minfo.Protect == 0             ||
           ( ! params.testPrivs (minfo) )   ||

        // It is not a good idea to scan Special K's DLL, since in many cases the pattern
        //   we are scanning for becomes a part of the DLL and makes an exhaustive search
        //     even more exhausting.
        //
        //  * If scanning a Special K DLL is needed, see Kaldaien and we can probably
        //      come up with a more robust solution.
        //
             ( minfo.BaseAddress >=            mi_sk.lpBaseOfDll &&
               it                <= (uint8_t *)mi_sk.lpBaseOfDll + mi_sk.SizeOfImage ) )
    {
      it    = next_rgn;
      idx   = 0;
      begin = it;

      continue;
    }

    // Do not search past the end of the module image!
    if (next_rgn >= end_addr)
      break;

    while (it <= next_rgn)
    {
      uint8_t* scan_addr = it;

      uint8_t test_val = static_cast <const uint8_t *> (pattern)[idx];
      uint8_t mask_val = 0x0;

      bool match = (*scan_addr == test_val);

      if (mask != nullptr)
      {
        mask_val = static_cast <const uint8_t *> (mask)[idx];

        // Treat portions we do not care about (mask [idx] == 0) as wildcards.
        if (! mask_val)
          match = true;

        else if (match)
        {
          // The more complicated case, where the mask is an actual mask :)
          match = (test_val & mask_val);
        }
      }

      if (match)
      {
        if (++idx == len)
        {
          if ((reinterpret_cast <uintptr_t> (begin) % align) == 0)
          {
            return begin;
          }

          else
          {
            begin += idx;
            begin += align - (reinterpret_cast <uintptr_t> (begin) % align);

            it     = begin;
            idx    = 0;
          }
        }

        else
          ++it;
      }

      else
      {
        // No match?!
        if (it > end_addr - len)
          break;

        begin += idx;
        begin += align - (reinterpret_cast <uintptr_t> (begin) % align);

        it  = begin;
        idx = 0;
      }
    }
  }

  return nullptr;
}

void*
__stdcall
SK_ScanAlignedEx2 ( const void* pattern, size_t len,   const void* mask,
                          void* after,   int    align,    uint8_t* base_addr)
{
  return SKX_ScanAlignedEx ( pattern, len, mask, after, align, base_addr );
}

void*
__stdcall
SKX_ScanAlignedExec (const void* pattern, size_t len, const void* mask, void* after, int align)
{
  auto* base_addr =
    reinterpret_cast <uint8_t *> (GetModuleHandle (nullptr));

  SK_MemScan_Params__v0 params;
                        params.privileges.execute =
                    SK_MemScan_Params__v0::Allowed;

  return SKX_ScanAlignedEx (pattern, len, mask, after, align, base_addr, params);
}

void*
__stdcall
SK_ScanAlignedEx (const void* pattern, size_t len, const void* mask, void* after, int align)
{
  auto* base_addr =
    reinterpret_cast <uint8_t *> (GetModuleHandle (nullptr));

  return SKX_ScanAlignedEx (pattern, len, mask, after, align, base_addr);
}

void*
__stdcall
SK_ScanAligned (const void* pattern, size_t len, const void* mask, int align)
{
  return SK_ScanAlignedEx (pattern, len, mask, nullptr, align);
}

BOOL
__stdcall
SK_InjectMemory ( LPVOID  base_addr,
            const void   *new_data,
                  size_t  data_size,
                  DWORD   permissions,
                  void   *old_data )
{
  __try {
    DWORD dwOld =
      PAGE_NOACCESS;

    if ( VirtualProtect ( base_addr,   data_size,
                          permissions, &dwOld )   )
    {
      if (old_data != nullptr) RtlCopyMemory (old_data, base_addr, data_size);
                               RtlCopyMemory (base_addr, new_data, data_size);

      VirtualProtect ( base_addr, data_size,
                       dwOld,     &dwOld );

      return TRUE;
    }
  }

  __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ) ?
               EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
  {
    //assert (false);

    // Bad memory address, just discard the write attempt
    //
    //   This isn't atomic; if we fail, it's possible we wrote part
    //     of the data successfully - consider an undo mechanism.
    //
    return FALSE;
  }

  return FALSE;
}

uint64_t
SK_File_GetSize (const wchar_t* wszFile)
{
  WIN32_FILE_ATTRIBUTE_DATA
    file_attrib_data = { };

  if ( GetFileAttributesEx ( wszFile,
                               GetFileExInfoStandard,
                                 &file_attrib_data ) )
  {
    return ULARGE_INTEGER { file_attrib_data.nFileSizeLow,
                            file_attrib_data.nFileSizeHigh }.QuadPart;
  }

  return 0ULL;
}



#if 0
void
__stdcall
SK_wcsrep ( const wchar_t*   wszIn,
                  wchar_t** pwszOut,
            const wchar_t*   wszOld,
            const wchar_t*   wszNew )
{
        wchar_t* wszTemp;
  const wchar_t* wszFound = wcsstr (wszIn, wszOld);

  if (! wszFound) {
    *pwszOut =
      (wchar_t *)malloc (wcslen (wszIn) + 1);

    wcscpy (*pwszOut, wszIn);
    return;
  }

  int idx = wszFound - wszIn;

  *pwszOut =
    (wchar_t *)realloc (  *pwszOut,
                            wcslen (wszIn)  - wcslen (wszOld) +
                            wcslen (wszNew) + 1 );

  wcsncpy ( *pwszOut, wszIn, idx    );
  wcscpy  ( *pwszOut + idx,  wszNew );
  wcscpy  ( *pwszOut + idx + wcslen (wszNew),
               wszIn + idx + wcslen (wszOld) );

  wszTemp =
    (wchar_t *)malloc (idx + wcslen (wszNew) + 1);

  wcsncpy (wszTemp, *pwszOut, idx + wcslen (wszNew));
  wszTemp [idx + wcslen (wszNew)] = '\0';

  SK_wcsrep ( wszFound + wcslen (wszOld),
                pwszOut,
                  wszOld,
                    wszNew );

  wszTemp =
    (wchar_t *)realloc ( wszTemp,
                           wcslen ( wszTemp) +
                           wcslen (*pwszOut) + 1 );

  lstrcatW (wszTemp, *pwszOut);

  free (*pwszOut);

  *pwszOut = wszTemp;
}
#endif

size_t
SK_RemoveTrailingDecimalZeros (wchar_t* wszNum, size_t bufLen)
{
  // Remove trailing 0's after the .
  size_t len = bufLen == 0 ?
                  wcslen (wszNum) :
        std::min (wcslen (wszNum), bufLen);

  for (size_t i = (len - 1); i > 1; i--)
  {
    if (wszNum [i] == L'0' && wszNum [i - 1] != L'.')
      len--;

    if (wszNum [i] != L'0' && wszNum [i] != L'\0')
      break;
  }

  wszNum [len] = L'\0';

  return len;
}

size_t
SK_RemoveTrailingDecimalZeros (char* szNum, size_t bufLen)
{
  // Remove trailing 0's after the .
  size_t len = bufLen == 0 ?
                  strlen (szNum) :
        std::min (strlen (szNum), bufLen);

  for (size_t i = (len - 1); i > 1; i--)
  {
    if (szNum [i] == '0' && szNum [i - 1] != '.')
      len--;

    if (szNum [i] != '0' && szNum [i] != '\0')
      break;
  }

  szNum [len] = '\0';

  return len;
}



struct sk_host_process_s {
  wchar_t wszApp       [MAX_PATH * 2 + 1] = { };
  wchar_t wszPath      [MAX_PATH * 2 + 1] = { };
  wchar_t wszFullName  [MAX_PATH * 2 + 1] = { };
  wchar_t wszBlacklist [MAX_PATH * 2 + 1] = { };
  wchar_t wszSystemDir [MAX_PATH * 2 + 1] = { };

  std::atomic_bool app                = false;
  std::atomic_bool path               = false;
  std::atomic_bool full_name          = false;
  std::atomic_bool blacklist          = false;
  std::atomic_bool sys_dir            = false;
} host_proc;

bool __SK_RunDLL_Bypass = false;

bool
__cdecl
SK_IsHostAppSKIM (void)
{
  return ( (! __SK_RunDLL_Bypass) &&
      StrStrIW (SK_GetHostApp (), L"SKIM") != nullptr );
}

bool
__cdecl
SK_IsRunDLLInvocation (void)
{
  // Allow some commands invoked by RunDLL to function as a normal DLL
  if (__SK_RunDLL_Bypass)
    return false;

  bool rundll_invoked =
    (StrStrIW (SK_GetHostApp (), L"Rundll32") != nullptr);

  if (rundll_invoked)
  {
    // Not all instances of RunDLL32 that load this DLL are Special K ...
    //
    //  The CBT hook may have been triggered by some other software that used
    //    rundll32 and then launched a Win32 application with it.
    //
    // If the command line does not reference our DLL
    if (! StrStrW (PathGetArgsW (GetCommandLineW ()), L"RunDLL_"))
      rundll_invoked = false;
  }

  return rundll_invoked;
}

bool
__cdecl
SK_IsSuperSpecialK (void)
{
  return ( SK_IsRunDLLInvocation () ||
           SK_IsHostAppSKIM      () );
}


// Using this rather than the Path Shell API stuff due to
//   AppInit_DLL support requirements
void
SK_PathRemoveExtension (wchar_t* wszInOut)
{
  wchar_t *wszEnd  = wszInOut,
          *wszPrev = nullptr;

  while (*CharNextW (wszEnd) != L'\0')
    wszEnd = CharNextW (wszEnd);

  wszPrev = wszEnd;

  while (  CharPrevW (wszInOut, wszPrev) > wszInOut &&
          *CharPrevW (wszInOut, wszPrev) != L'.' )
    wszPrev = CharPrevW (wszInOut, wszPrev);

  if (CharPrevW (wszInOut, wszPrev) > wszInOut)
  {
    if (*CharPrevW (wszInOut, wszPrev) == L'.')
        *CharPrevW (wszInOut, wszPrev)  = L'\0';
  }
}


const wchar_t*
SK_GetBlacklistFilename (void)
{
  if ( host_proc.blacklist )
    return host_proc.wszBlacklist;

  static volatile
    LONG init = FALSE;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  {
    lstrcatW (host_proc.wszBlacklist, SK_GetHostPath ());
    lstrcatW (host_proc.wszBlacklist, L"\\SpecialK.deny.");
    lstrcatW (host_proc.wszBlacklist, SK_GetHostApp  ());

    SK_PathRemoveExtension (host_proc.wszBlacklist);

    host_proc.blacklist = true;

    InterlockedIncrement (&init);
  } SK_Thread_SpinUntilAtomicMin (&init, 2);

  return
    host_proc.wszBlacklist;
}

const wchar_t*
SK_GetHostApp (void)
{
  if ( host_proc.app )
    return host_proc.wszApp;

  static volatile
    LONG init = FALSE;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  {
    DWORD   dwProcessSize =  MAX_PATH * 2;
    wchar_t wszProcessName [ MAX_PATH * 2 + 1 ] = { };

    GetModuleFileNameW ( 0, wszProcessName, dwProcessSize );

    int      len    = lstrlenW (wszProcessName) - 1;
    wchar_t* wszEnd =           wszProcessName;

    for (int i = 0; i < len; i++)
      wszEnd = CharNextW (wszEnd);

    wchar_t* wszSepBack = wszEnd;
    wchar_t* wszSepFwd  = wszEnd;

    while (wszSepBack > wszProcessName)
    {
      wszSepBack = CharPrevW (wszProcessName, wszSepBack);

      if (*wszSepBack == L'\\')
        break;
    }

    while (wszSepFwd > wszProcessName)
    {
      wszSepFwd = CharPrevW (wszProcessName, wszSepFwd);

      if (*wszSepFwd == L'/')
        break;
    }

    wchar_t* wszLastSep =
      ( wszSepBack > wszSepFwd ? wszSepBack : wszSepFwd );

    if ( wszLastSep != nullptr )
    {
      wcsncpy_s ( host_proc.wszApp,         MAX_PATH * 2,
                    CharNextW (wszLastSep), _TRUNCATE  );
    }

    else
    {
      wcsncpy_s ( host_proc.wszApp, MAX_PATH * 2,
                    wszProcessName, _TRUNCATE );
    }

    host_proc.app = true;

    InterlockedIncrement (&init);
  }
 
  else
    SK_Thread_SpinUntilAtomicMin (&init, 2);

  return
    host_proc.wszApp;
}

const wchar_t*
SK_GetFullyQualifiedApp (void)
{
  if ( host_proc.full_name )
    return host_proc.wszFullName;

  static volatile
    LONG init = FALSE;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  {
    DWORD   dwProcessSize =  MAX_PATH * 2;
    wchar_t wszProcessName [ MAX_PATH * 2 + 1 ] = { };

    GetModuleFileNameW ( 0, wszProcessName, dwProcessSize );

    wcsncpy_s ( host_proc.wszFullName, MAX_PATH * 2,
                  wszProcessName,       _TRUNCATE );

    host_proc.full_name = true;

    InterlockedIncrement (&init);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&init, 2);

  return
    host_proc.wszFullName;
}

// NOT the working directory, this is the directory that
//   the executable is located in.

const wchar_t*
SK_GetHostPath (void)
{
  if ( host_proc.path )
    return host_proc.wszPath;

  static volatile
    LONG init = FALSE;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  {
    wchar_t     wszProcessName [ MAX_PATH * 2 + 1 ] = { };
    wcsncpy_s ( wszProcessName,  MAX_PATH * 2,
                  SK_GetFullyQualifiedApp (), _TRUNCATE );

    wchar_t* wszSepBack = wcsrchr (wszProcessName, L'\\');
    wchar_t* wszSepFwd  = wcsrchr (wszProcessName, L'/');

    wchar_t* wszLastSep =
      ( wszSepBack > wszSepFwd ? wszSepBack : wszSepFwd );

    if (wszLastSep != nullptr)
       *wszLastSep  = L'\0';

    wcsncpy_s (
      host_proc.wszPath, MAX_PATH * 2,
        wszProcessName,  _TRUNCATE  );

    host_proc.path = true;

    InterlockedIncrement (&init);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&init, 2);

  return
    host_proc.wszPath;
}


const wchar_t*
SK_GetSystemDirectory (void)
{
  if ( host_proc.sys_dir )
    return host_proc.wszSystemDir;

  static volatile
    LONG init = FALSE;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  {
#ifdef _WIN64
    GetSystemDirectory (host_proc.wszSystemDir, MAX_PATH);
#else
    HANDLE hProc = SK_GetCurrentProcess ();

    BOOL   bWOW64;
    ::IsWow64Process (hProc, &bWOW64);

    if (bWOW64)
      GetSystemWow64Directory (host_proc.wszSystemDir, MAX_PATH);
    else
      GetSystemDirectory      (host_proc.wszSystemDir, MAX_PATH);
#endif

    host_proc.sys_dir = true;

    InterlockedIncrement (&init);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&init, 2);

  return
    host_proc.wszSystemDir;
}




uint64_t
SK_DeleteTemporaryFiles (const wchar_t* wszPath, const wchar_t* wszPattern)
{
  WIN32_FIND_DATA fd     = {      };
  HANDLE          hFind  = INVALID_HANDLE_VALUE;
  size_t          files  =   0UL;
  LARGE_INTEGER   liSize = { 0ULL };

  wchar_t wszFindPattern [MAX_PATH * 2 + 1] = { };

  lstrcatW (wszFindPattern, wszPath);
  lstrcatW (wszFindPattern, L"\\");
  lstrcatW (wszFindPattern, wszPattern);

  hFind = FindFirstFileW (wszFindPattern, &fd);

  if (hFind != INVALID_HANDLE_VALUE)
  {
    dll_log.LogEx ( true, L"[Clean Mgr.] Cleaning temporary files in '%s'...    ",
                   SK_StripUserNameFromPathW (std::wstring (wszPath).data ()) );

    wchar_t wszFullPath [MAX_PATH * 2 + 1] = { };

    do
    {
      if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
      {
        *wszFullPath = L'\0';

        lstrcatW (wszFullPath, wszPath);
        lstrcatW (wszFullPath, L"\\");
        lstrcatW (wszFullPath, fd.cFileName);

        if (DeleteFileW (wszFullPath))
        {
          ++files;

          liSize.QuadPart += LARGE_INTEGER {       fd.nFileSizeLow,
                                             (LONG)fd.nFileSizeHigh }.QuadPart;
        }
      }
    } while (FindNextFileW (hFind, &fd) != 0);

    dll_log.LogEx ( false, L"%zu files deleted\n", files);

    FindClose (hFind);
  }

  return (uint64_t)liSize.QuadPart;
}


bool
SK_FileHasSpaces (const wchar_t* wszLongFileName)
{
  return StrStrIW (wszLongFileName, L" ") != nullptr;
}

BOOL
SK_FileHas8Dot3Name (const wchar_t* wszLongFileName)
{
  wchar_t wszShortPath [MAX_PATH * 2 + 1] = { };

  if ((! GetShortPathName   (wszLongFileName, wszShortPath, 1)) ||
         GetFileAttributesW (wszShortPath) == INVALID_FILE_ATTRIBUTES   ||
         StrStrIW           (wszLongFileName, L" "))
  {
    return FALSE;
  }

  return TRUE;
}

HRESULT ModifyPrivilege(
    IN LPCTSTR szPrivilege,
    IN BOOL fEnable)
{
    HRESULT hr = S_OK;
    TOKEN_PRIVILEGES NewState;
    LUID             luid;
    HANDLE hToken    = nullptr;

    // Open the process token for this process.
    if (! OpenProcessToken(SK_GetCurrentProcess (),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          &hToken ))
    {
        return ERROR_FUNCTION_FAILED;
    }

    // Get the local unique ID for the privilege.
    if ( !LookupPrivilegeValue( nullptr,
                                szPrivilege,
                                &luid ))
    {
        CloseHandle( hToken );
        return ERROR_FUNCTION_FAILED;
    }

    // Assign values to the TOKEN_PRIVILEGE structure.
    NewState.PrivilegeCount            = 1;
    NewState.Privileges [0].Luid       = luid;
    NewState.Privileges [0].Attributes =
              (fEnable ? SE_PRIVILEGE_ENABLED : 0);

    // Adjust the token privilege.
    if (!AdjustTokenPrivileges(hToken,
                               FALSE,
                               &NewState,
                               0,
                               nullptr,
                               nullptr))
    {
        hr = ERROR_FUNCTION_FAILED;
    }

    // Close the handle.
    CloseHandle(hToken);

    return hr;
}


bool
SK_Generate8Dot3 (const wchar_t* wszLongFileName)
{
  CHeapPtr <wchar_t> wszFileName  (_wcsdup (wszLongFileName));
  CHeapPtr <wchar_t> wszFileName1 (_wcsdup (wszLongFileName));

  wchar_t  wsz8     [11] = { }; // One non-nul for overflow
  wchar_t  wszDot3  [ 4] = { };
  wchar_t  wsz8Dot3 [14] = { };

  while (SK_FileHasSpaces (wszFileName))
  {
    ModifyPrivilege (SE_RESTORE_NAME, TRUE);
    ModifyPrivilege (SE_BACKUP_NAME,  TRUE);

    CHandle hFile (
      CreateFileW ( wszFileName,
                      GENERIC_WRITE      | DELETE,
                        FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          nullptr,
                            OPEN_EXISTING,
                              GetFileAttributes (wszFileName) |
                              FILE_FLAG_BACKUP_SEMANTICS,
                                nullptr ) );

    if (hFile == INVALID_HANDLE_VALUE)
    {
      return false;
    }

    DWORD dwAttrib =
      GetFileAttributes (wszFileName);

    if (dwAttrib == INVALID_FILE_ATTRIBUTES)
    {
      return false;
    }

    bool dir = false;

    if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)
    {
      dir = true;
    }

    else
    {
      dir = false;

      const wchar_t* pwszExt =
        PathFindExtension (wszFileName);

      if (pwszExt != nullptr && *pwszExt == L'.')
      {
        swprintf (wszDot3, L"%3s", CharNextW (pwszExt));
      }

      PathRemoveExtension (wszFileName);
    }

    PathStripPath       (wszFileName);
    PathRemoveBackslash (wszFileName);
    PathRemoveBlanks    (wszFileName);

    wcsncpy (wsz8, wszFileName, 10);

    wchar_t idx = 0;

    if (wcslen (wsz8) > 8)
    {
      wsz8 [6] = L'~';
      wsz8 [7] = L'0';
      wsz8 [8] = L'\0';

      _swprintf (wsz8Dot3, dir ? L"%s" : L"%s.%s", wsz8, wszDot3);

      while ((! SetFileShortNameW (hFile, wsz8Dot3)) && idx < 9)
      {
        wsz8 [6] = L'~';
        wsz8 [7] = L'0' + idx++;
        wsz8 [8] = L'\0';

        _swprintf (wsz8Dot3, dir ? L"%s" : L"%s.%s", wsz8, wszDot3);
      }
    }

    else
    {
      _swprintf (wsz8Dot3, dir ? L"%s" : L"%s.%s", wsz8, wszDot3);
    }

    if (idx == 9)
    {
      return false;
    }

    PathRemoveFileSpec  (wszFileName1);
    wcscpy (wszFileName, wszFileName1);
  }

  return true;
}



void
SK_RestartGame (const wchar_t* wszDLL)
{
  wchar_t wszShortPath [MAX_PATH * 2 + 1] = { };
  wchar_t wszFullname  [MAX_PATH * 2 + 1] = { };

  wcsncpy_s ( wszFullname, MAX_PATH,
              wszDLL != nullptr ?
              wszDLL :
              SK_GetModuleFullName ( SK_GetDLL ()).c_str (),
                          _TRUNCATE );

  SK_Generate8Dot3 (wszFullname);
  wcsncpy_s        (wszShortPath, MAX_PATH, wszFullname, _TRUNCATE);

  if (SK_FileHasSpaces (wszFullname))
    GetShortPathName   (wszFullname, wszShortPath, MAX_PATH  );
 
  if (SK_FileHasSpaces (wszShortPath))
  {
    if (wszDLL != nullptr)
    {
      SK_MessageBox ( L"Your computer is misconfigured; please enable DOS 8.3 filename generation."
                      L"\r\n\r\n\t"
                      L"This is a common problem for non-boot drives, please ensure that the drive your "
                      L"game is installed to has 8.3 filename generation enabled and then re-install "
                      L"the mod.",
                        L"Cannot Automatically Restart Game Because of Bad File system Policy.",
                          MB_OK | MB_SYSTEMMODAL | MB_SETFOREGROUND | MB_ICONASTERISK | MB_TOPMOST );
    }

    else if (SK_HasGlobalInjector ())
    {
      std::wstring global_dll =
        SK_GetDocumentsDir () + LR"(\My Mods\SpecialK\SpecialK)";

#ifdef _WIN64
      global_dll.append (L"64.dll");
#else
      global_dll.append (L"32.dll");
#endif

                wcsncpy ( wszFullname, global_dll.c_str (), MAX_PATH );
      GetShortPathName   (wszFullname, wszShortPath,        MAX_PATH );
    }

    if (SK_FileHasSpaces (wszShortPath))
      ExitProcess (0x00);
  }

  if (! SK_IsSuperSpecialK ())
  {
    wchar_t wszRunDLLCmd [MAX_PATH * 4] = { };

    _swprintf ( wszRunDLLCmd,
                 L"RunDll32.exe %s,RunDLL_RestartGame %s",
                   wszShortPath,
                     SK_GetFullyQualifiedApp () );

    STARTUPINFOW        sinfo = { };
    PROCESS_INFORMATION pinfo = { };

    sinfo.cb          = sizeof STARTUPINFOW;
    sinfo.wShowWindow = SW_SHOWNORMAL;
    sinfo.dwFlags     = STARTF_USESHOWWINDOW;

    CreateProcess ( nullptr, wszRunDLLCmd,             nullptr, nullptr,
                    FALSE,   CREATE_NEW_PROCESS_GROUP, nullptr, SK_GetHostPath (),
                    &sinfo,  &pinfo );

    CloseHandle (pinfo.hThread);
    CloseHandle (pinfo.hProcess);
  }

  SK_TerminateProcess (0x00);
}

void
SK_ElevateToAdmin (void)
{
  wchar_t wszRunDLLCmd [MAX_PATH * 4    ] = { };
  wchar_t wszShortPath [MAX_PATH * 2 + 1] = { };
  wchar_t wszFullname  [MAX_PATH * 2 + 1] = { };

  wcsncpy (wszFullname, SK_GetModuleFullName (SK_GetDLL ()).c_str (), MAX_PATH );

  SK_Generate8Dot3     (wszFullname);
  wcscpy (wszShortPath, wszFullname);

  if (SK_FileHasSpaces (wszFullname))
    GetShortPathName   (wszFullname, wszShortPath, MAX_PATH );
 
  if (SK_FileHasSpaces (wszShortPath))
  {
    SK_MessageBox ( L"Your computer is misconfigured; please enable DOS 8.3 filename generation."
                    L"\r\n\r\n\t"
                    L"This is a common problem for non-boot drives, please ensure that the drive your "
                    L"game is installed to has 8.3 filename generation enabled and then re-install "
                    L"the mod.",
                      L"Cannot Elevate To Admin Because of Bad File system Policy.",
                        MB_OK | MB_SYSTEMMODAL | MB_SETFOREGROUND | MB_ICONASTERISK | MB_TOPMOST );
    ExitProcess   (0x00);
  }

  _swprintf ( wszRunDLLCmd,
               L"RunDll32.exe %s,RunDLL_ElevateMe %s",
                 wszShortPath,
                   SK_GetFullyQualifiedApp () );

  STARTUPINFOW        sinfo = { };
  PROCESS_INFORMATION pinfo = { };

  sinfo.cb          = sizeof STARTUPINFOW;
  sinfo.wShowWindow = SW_SHOWNORMAL;
  sinfo.dwFlags     = STARTF_USESHOWWINDOW;

  CreateProcess ( nullptr, wszRunDLLCmd,             nullptr, nullptr,
                  FALSE,   CREATE_NEW_PROCESS_GROUP, nullptr, SK_GetHostPath (),
                  &sinfo,  &pinfo );

  CloseHandle (pinfo.hThread);
  CloseHandle (pinfo.hProcess);

  SK_TerminateProcess (0x00);
}

#include <memory>

std::string
__cdecl
SK_FormatString (char const* const _Format, ...)
{
  intptr_t len = 0;

  va_list   _ArgList;
  va_start (_ArgList, _Format);
  {
    len =
      vsnprintf ( nullptr, 0, _Format, _ArgList ) + 1;
  }
  va_end   (_ArgList);

  size_t alloc_size =
    sizeof (char) * (len + 2);

  char* pData =
    ( ReadAcquire (&__SK_DLL_Attached) ?
        (char *)SK_TLS_Bottom ()->scratch_memory.eula.alloc (alloc_size, true) :
        (char *)SK_LocalAlloc (                        LPTR, alloc_size      ) );

  if (! pData)
    return std::string ();

  va_start (_ArgList, _Format);
  {
    len =
      vsnprintf ( pData, len + 1, _Format, _ArgList );
  }
  va_end   (_ArgList);

  return
    pData;
}

std::wstring
__cdecl
SK_FormatStringW (wchar_t const* const _Format, ...)
{
  int len = 0;

  va_list   _ArgList;
  va_start (_ArgList, _Format);
  {
    len =
      _vsnwprintf ( nullptr, 0, _Format, _ArgList ) + 1;
  }
  va_end   (_ArgList);

  size_t alloc_size =
    sizeof (wchar_t) * (len + 2);

  wchar_t* pData = ReadAcquire (&__SK_DLL_Attached) ?
    (wchar_t *)SK_TLS_Bottom ()->scratch_memory.eula.alloc (alloc_size, true) :
    (wchar_t *)SK_LocalAlloc (                        LPTR, alloc_size      );

  if (! pData)
    return std::wstring ();

  va_start (_ArgList, _Format);
  {
    len =
      _vsnwprintf ( (wchar_t *)pData, len + 1, _Format, _ArgList );
  }
  va_end   (_ArgList);

  return
    pData;
}


//void
//SK_StripTrailingSlashesW_ (wchar_t* wszInOut)
//{
//  struct test_slashes
//  {
//    bool operator () (wchar_t a, wchar_t b) const
//    {
//      auto IsSlash = [](wchar_t a) -> bool {
//        return (a == L'\\' || a == L'/');
//      };
//
//      return IsSlash (a) && IsSlash (b);
//    }
//  };
//
//  std::wstring wstr (
//    wszInOut
//  );
//
//  if (wstr.empty ())
//    return;
//
//  wstr.erase ( std::unique ( wstr.begin (),
//              wstr.end   (), test_slashes () ),
//              wstr.end () );
//
//  if (wstr.empty ())
//    *wszInOut = L'\0';
//  else
//    wcsncpy_s ( wszInOut, wstr.length () + 1,
//                wstr.c_str  (), _TRUNCATE  );
//}

//
// In-place version of the old code that had to
//   make a copy of the string and then copy-back
//
void
SK_StripTrailingSlashesW (wchar_t* wszInOut)
{
  //wchar_t* wszValidate = wcsdup (wszInOut);

  auto IsSlash = [](wchar_t a) -> bool {
    return (a == L'\\' || a == L'/');
  };

  wchar_t* wszNextUnique = CharNextW (wszInOut);
  wchar_t* wszNext       = wszInOut;

  while (*wszNext != L'\0')
  {
    if (*wszNextUnique == L'\0')
    {
      *CharNextW (wszNext) = L'\0';
      break;
    }

    if (IsSlash (*wszNext))
    {
      if (IsSlash (*wszNextUnique))
      {
        wszNextUnique =
          CharNextW (wszNextUnique);

        continue;
      }
    }

    wszNext = CharNextW (wszNext);
   *wszNext = *wszNextUnique;
    wszNextUnique =
      CharNextW (wszNextUnique);
  }


  //extern void
  //  SK_StripTrailingSlashesW (wchar_t* wszInOut);
  //
  //wchar_t xxx [] = LR"(\\//\/\asda.\\/fasd/ads\d///\asdz/\\/)";
  //wchar_t yyy [] = LR"(a\\)";
  //wchar_t zzz [] = LR"(\\a)";
  //wchar_t yzy [] = LR"(\a/)";
  //wchar_t zzy [] = LR"(\/a)";
  //
  //SK_StripTrailingSlashesW (xxx);
  //SK_StripTrailingSlashesW (yyy);
  //SK_StripTrailingSlashesW (zzz);
  //SK_StripTrailingSlashesW (yzy);
  //SK_StripTrailingSlashesW (zzy);      //extern void
      //  SK_StripTrailingSlashesW (wchar_t* wszInOut);
      //
      //wchar_t xxx [] = LR"(\\//\/\asda.\\/fasd/ads\d///\asdz/\\/)";
      //wchar_t yyy [] = LR"(a\\)";
      //wchar_t zzz [] = LR"(\\a)";
      //wchar_t yzy [] = LR"(\a/)";
      //wchar_t zzy [] = LR"(\/a)";
      //
      //SK_StripTrailingSlashesW (xxx);
      //SK_StripTrailingSlashesW (yyy);
      //SK_StripTrailingSlashesW (zzz);
      //SK_StripTrailingSlashesW (yzy);
      //SK_StripTrailingSlashesW (zzy);

  //SK_StripTrailingSlashesW_ (wszValidate);
  //
  //SK_ReleaseAssert (! wcscmp (wszValidate, wszInOut))
  //
  //dll_log.Log (L"'%ws' and '%ws' are the same", wszValidate, wszInOut);
}

void
SK_FixSlashesW (wchar_t* wszInOut)
{
  if (wszInOut == nullptr)
    return;

  wchar_t* pwszInOut  = wszInOut;
  while ( *pwszInOut != L'\0' )
  {
    if (*pwszInOut == L'/')
        *pwszInOut = L'\\';

    pwszInOut =
      CharNextW (pwszInOut);
  }
}

void
SK_StripTrailingSlashesA (char* szInOut)
{
  auto IsSlash = [](char a) -> bool {
    return (a == '\\' || a == '/');
  };

  char* szNextUnique = szInOut + 1;
  char* szNext       = szInOut;

  while (*szNext != '\0')
  {
    if (*szNextUnique == '\0')
    {
      *CharNextA (szNext) = '\0';
      break;
    }

    if (IsSlash (*szNext))
    {
      if (IsSlash (*szNextUnique))
      {
        szNextUnique =
          CharNextA (szNextUnique);

        continue;
      }
    }

    ++szNext;
     *szNext = *szNextUnique;
                szNextUnique =
     CharNextA (szNextUnique);
  }
}

void
SK_FixSlashesA (char* szInOut)
{
  if (szInOut == nullptr)
    return;

  char*   pszInOut  = szInOut;
  while (*pszInOut != '\0')
  {
    if (*pszInOut == '/')
        *pszInOut = '\\';

    pszInOut =
      CharNextA (pszInOut);
  }
}


#define SECURITY_WIN32
#include <Security.h>

#pragma comment (lib, "secur32.lib")

_Success_(return != 0)
BOOLEAN
WINAPI
SK_GetUserNameExA (
  _In_                               EXTENDED_NAME_FORMAT  NameFormat,
  _Out_writes_to_opt_(*nSize,*nSize) LPSTR                 lpNameBuffer,
  _Inout_                            PULONG                nSize )
{
  return
    GetUserNameExA (NameFormat, lpNameBuffer, nSize);
}

_Success_(return != 0)
BOOLEAN
SEC_ENTRY
SK_GetUserNameExW (
    _In_                               EXTENDED_NAME_FORMAT NameFormat,
    _Out_writes_to_opt_(*nSize,*nSize) LPWSTR               lpNameBuffer,
    _Inout_                            PULONG               nSize )
{
  return
    GetUserNameExW (NameFormat, lpNameBuffer, nSize);
}

// Doesn't need to be this complicated; it's a string function, might as well optimize it.

static char     szUserName        [MAX_PATH * 2 + 1] = { };
static char     szUserNameDisplay [MAX_PATH * 2 + 1] = { };
static char     szUserProfile     [MAX_PATH * 2 + 1] = { }; // Most likely to match
static wchar_t wszUserName        [MAX_PATH * 2 + 1] = { };
static wchar_t wszUserNameDisplay [MAX_PATH * 2 + 1] = { };
static wchar_t wszUserProfile     [MAX_PATH * 2 + 1] = { }; // Most likely to match

char*
SK_StripUserNameFromPathA (char* szInOut)
{
  //static volatile LONG               calls  =  0;
  //if (InterlockedCompareExchange   (&calls, 1, 0))
  //    SK_Thread_SpinUntilAtomicMin (&calls, 2);

  if (*szUserProfile == '\0')
  {
                                        uint32_t len = MAX_PATH;
    if (! SK_GetUserProfileDir (wszUserProfile, &len))
      *wszUserProfile = L'?'; // Invalid filesystem char
    else
      PathStripPathW (wszUserProfile);

    strncpy_s ( szUserProfile, MAX_PATH + 2,
                  SK_WideCharToUTF8 (wszUserProfile).c_str (), _TRUNCATE );
  }

  if (*szUserName == '\0')
  {
                                           DWORD dwLen = MAX_PATH;
    SK_GetUserNameExA (NameUnknown, szUserName, &dwLen);

    if (dwLen == 0)
      *szUserName = '?'; // Invalid filesystem char
    else
      PathStripPathA (szUserName);
  }

  if (*szUserNameDisplay == '\0')
  {
                                                  DWORD dwLen = MAX_PATH;
    SK_GetUserNameExA (NameDisplay, szUserNameDisplay, &dwLen);

    if (dwLen == 0)
      *szUserNameDisplay = '?'; // Invalid filesystem char
  }

//InterlockedIncrement (&calls);

  char* pszUserNameSubstr =
    StrStrIA (szInOut, szUserProfile);

  if (pszUserNameSubstr != nullptr)
  {
    static const size_t len =
      strlen (szUserProfile);

    for (size_t i = 0; i < len; i++)
    {
      *pszUserNameSubstr = '*';
       pszUserNameSubstr = CharNextA (pszUserNameSubstr);
    }

    return szInOut;
  }

  pszUserNameSubstr =
    StrStrIA (szInOut, szUserNameDisplay);

  if (pszUserNameSubstr != nullptr)
  {
    static const size_t len =
      strlen (szUserNameDisplay);

    for (size_t i = 0; i < len; i++)
    {
      *pszUserNameSubstr = '*';
       pszUserNameSubstr = CharNextA (pszUserNameSubstr);
    }

    return szInOut;
  }

  pszUserNameSubstr =
    StrStrIA (szInOut, szUserName);

  if (pszUserNameSubstr != nullptr)
  {
    static const size_t len =
      strlen (szUserName);

    for (size_t i = 0; i < len; i++)
    {
      *pszUserNameSubstr = '*';
       pszUserNameSubstr = CharNextA (pszUserNameSubstr);
    }

    return szInOut;
  }

  return szInOut;
}

wchar_t*
SK_StripUserNameFromPathW (wchar_t* wszInOut)
{
//  static volatile LONG               calls  =  0;
//  if (InterlockedCompareExchange   (&calls, 1, 0))
//      SK_Thread_SpinUntilAtomicMin (&calls, 2);

  if (*wszUserProfile == L'\0')
  {
                                        uint32_t len = MAX_PATH;
    if (! SK_GetUserProfileDir (wszUserProfile, &len))
      *wszUserProfile = L'?'; // Invalid filesystem char
    else
      PathStripPathW (wszUserProfile);
  }

  if (*wszUserName == L'\0')
  {
                                                  DWORD dwLen = MAX_PATH;
    SK_GetUserNameExW (NameSamCompatible, wszUserName, &dwLen);

    if (dwLen == 0)
      *wszUserName = L'?'; // Invalid filesystem char
    else
      PathStripPathW (wszUserName);
  }

  if (*wszUserNameDisplay == L'\0')
  {
                                                   DWORD dwLen = MAX_PATH;
    SK_GetUserNameExW (NameDisplay, wszUserNameDisplay, &dwLen);

    if (dwLen == 0)
      *wszUserNameDisplay = L'?'; // Invalid filesystem char
  }


//InterlockedIncrement (&calls);
  //dll_log.Log (L"Profile: %ws, User: %ws, Display: %ws", wszUserProfile, wszUserName, wszUserNameDisplay);


  wchar_t* pwszUserNameSubstr =
    StrStrIW (wszInOut, wszUserProfile);

  if (pwszUserNameSubstr != nullptr)
  {
    static const size_t len =
      wcslen (wszUserProfile);

    for (size_t i = 0; i < len; i++)
    {
      *pwszUserNameSubstr = L'*';
       pwszUserNameSubstr = CharNextW (pwszUserNameSubstr);
    }

    return wszInOut;
  }

  pwszUserNameSubstr =
    StrStrIW (wszInOut, wszUserNameDisplay);

  if (pwszUserNameSubstr != nullptr)
  {
    static const size_t len =
      wcslen (wszUserNameDisplay);

    for (size_t i = 0; i < len; i++)
    {
      *pwszUserNameSubstr = L'*';
       pwszUserNameSubstr = CharNextW (pwszUserNameSubstr);
    }

    return wszInOut;
  }

  pwszUserNameSubstr =
    StrStrIW (wszInOut, wszUserName);

  if (pwszUserNameSubstr != nullptr)
  {
    static const size_t len =
      wcslen (wszUserName);

    for (size_t i = 0; i < len; i++)
    {
      *pwszUserNameSubstr = L'*';
       pwszUserNameSubstr = CharNextW (pwszUserNameSubstr);
    }

    return wszInOut;
  }

  return wszInOut;
}


#include <concurrent_queue.h>

void
SK_DeferCommands (const char** szCommands, int count)
{
  static          concurrency::concurrent_queue <std::string> cmds;
  static          HANDLE                                      hNewCmds =
    SK_CreateEvent (nullptr, FALSE, FALSE, L"DeferredCmdEvent");
  static volatile HANDLE                                      hCommandThread = nullptr;

  for (int i = 0; i < count; i++)
  {
    cmds.push (szCommands [i]);
  }

  SetEvent (hNewCmds);

  // ============================================== //

  if (! InterlockedCompareExchangePointer (&hCommandThread, (LPVOID)1, nullptr))
  {     InterlockedExchangePointer        ((void **)&hCommandThread,

    SK_Thread_CreateEx (
      [](LPVOID) ->
      DWORD
      {
        SetCurrentThreadDescription (                      L"[SK] Async Command Processor" );
        SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_IDLE         );

        while (! ReadAcquire (&__SK_DLL_Ending))
        {
          if (SK_WaitForSingleObject (hNewCmds, INFINITE) == WAIT_OBJECT_0)
          {
            std::string cmd = "";

            while (cmds.try_pop (cmd))
            {
              SK_GetCommandProcessor ()->ProcessCommandLine (cmd.c_str ());
            }
          }
        }

        CloseHandle (hNewCmds);
        SK_Thread_CloseSelf ();

        return 0;
      }
    ) );
  }
};

//
// Issue a command that would deadlock the game if executed synchronously
//   from the render thread.
//
void
SK_DeferCommand (const char* szCommand)
{
  return
    SK_DeferCommands (&szCommand, 1);
};


void
SK_HostAppUtil::init (void)
{
  SK_RunOnce (SKIM     = (StrStrIW ( SK_GetHostApp (), L"SKIM"     ) != nullptr));
  SK_RunOnce (RunDll32 = (StrStrIW ( SK_GetHostApp (), L"RunDLL32" ) != nullptr));
}

SK_HostAppUtil&
SK_GetHostAppUtil (void)
{
  static SK_HostAppUtil SK_HostApp;
  return                SK_HostApp;
}



const wchar_t*
__stdcall
SK_GetCanonicalDLLForRole (enum DLL_ROLE role)
{
  switch (role)
  {
    case DLL_ROLE::DXGI:
      return L"dxgi.dll";
    case DLL_ROLE::D3D9:
      return L"d3d9.dll";
    case DLL_ROLE::D3D8:
      return L"d3d8.dll";
    case DLL_ROLE::DDraw:
      return L"ddraw.dll";
    case DLL_ROLE::OpenGL:
      return L"OpenGL32.dll";
    case DLL_ROLE::Vulkan:
      return L"vk-1.dll";
    case DLL_ROLE::DInput8:
      return L"dinput8.dll";
    default:
      return SK_RunLHIfBitness ( 64, L"SpecialK64.dll",
                                     L"SpecialK32.dll" );
  }
}

const wchar_t*
SK_DescribeHRESULT (HRESULT hr)
{
  switch (hr)
  {
    /* Generic (SUCCEEDED) */

  case S_OK:
    return L"S_OK";

  case S_FALSE:
    return L"S_FALSE";


#ifndef SK_BUILD__INSTALLER
    /* DXGI */

  case DXGI_ERROR_DEVICE_HUNG:
    return L"DXGI_ERROR_DEVICE_HUNG";

  case DXGI_ERROR_DEVICE_REMOVED:
    return L"DXGI_ERROR_DEVICE_REMOVED";

  case DXGI_ERROR_DEVICE_RESET:
    return L"DXGI_ERROR_DEVICE_RESET";

  case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
    return L"DXGI_ERROR_DRIVER_INTERNAL_ERROR";

  case DXGI_ERROR_FRAME_STATISTICS_DISJOINT:
    return L"DXGI_ERROR_FRAME_STATISTICS_DISJOINT";

  case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE:
    return L"DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE";

  case DXGI_ERROR_INVALID_CALL:
    return L"DXGI_ERROR_INVALID_CALL";

  case DXGI_ERROR_MORE_DATA:
    return L"DXGI_ERROR_MORE_DATA";

  case DXGI_ERROR_NONEXCLUSIVE:
    return L"DXGI_ERROR_NONEXCLUSIVE";

  case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
    return L"DXGI_ERROR_NOT_CURRENTLY_AVAILABLE";

  case DXGI_ERROR_NOT_FOUND:
    return L"DXGI_ERROR_NOT_FOUND";

  case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED:
    return L"DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED";

  case DXGI_ERROR_REMOTE_OUTOFMEMORY:
    return L"DXGI_ERROR_REMOTE_OUTOFMEMORY";

  case DXGI_ERROR_WAS_STILL_DRAWING:
    return L"DXGI_ERROR_WAS_STILL_DRAWING";

  case DXGI_ERROR_UNSUPPORTED:
    return L"DXGI_ERROR_UNSUPPORTED";

  case DXGI_ERROR_ACCESS_LOST:
    return L"DXGI_ERROR_ACCESS_LOST";

  case DXGI_ERROR_WAIT_TIMEOUT:
    return L"DXGI_ERROR_WAIT_TIMEOUT";

  case DXGI_ERROR_SESSION_DISCONNECTED:
    return L"DXGI_ERROR_SESSION_DISCONNECTED";

  case DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE:
    return L"DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE";

  case DXGI_ERROR_CANNOT_PROTECT_CONTENT:
    return L"DXGI_ERROR_CANNOT_PROTECT_CONTENT";

  case DXGI_ERROR_ACCESS_DENIED:
    return L"DXGI_ERROR_ACCESS_DENIED";

  case DXGI_ERROR_NAME_ALREADY_EXISTS:
    return L"DXGI_ERROR_NAME_ALREADY_EXISTS";

  case DXGI_ERROR_SDK_COMPONENT_MISSING:
    return L"DXGI_ERROR_SDK_COMPONENT_MISSING";

  case DXGI_DDI_ERR_WASSTILLDRAWING:
    return L"DXGI_DDI_ERR_WASSTILLDRAWING";

  case DXGI_DDI_ERR_UNSUPPORTED:
    return L"DXGI_DDI_ERR_UNSUPPORTED";

  case DXGI_DDI_ERR_NONEXCLUSIVE:
    return L"DXGI_DDI_ERR_NONEXCLUSIVE";


    /* DXGI (Status) */
  case DXGI_STATUS_OCCLUDED:
    return L"DXGI_STATUS_OCCLUDED";

  case DXGI_STATUS_UNOCCLUDED:
    return L"DXGI_STATUS_UNOCCLUDED";

  case DXGI_STATUS_CLIPPED:
    return L"DXGI_STATUS_CLIPPED";

  case DXGI_STATUS_NO_REDIRECTION:
    return L"DXGI_STATUS_NO_REDIRECTION";

  case DXGI_STATUS_NO_DESKTOP_ACCESS:
    return L"DXGI_STATUS_NO_DESKTOP_ACCESS";

  case DXGI_STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE:
    return L"DXGI_STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE";

  case DXGI_STATUS_DDA_WAS_STILL_DRAWING:
    return L"DXGI_STATUS_DDA_WAS_STILL_DRAWING";

  case DXGI_STATUS_MODE_CHANGED:
    return L"DXGI_STATUS_MODE_CHANGED";

  case DXGI_STATUS_MODE_CHANGE_IN_PROGRESS:
    return L"DXGI_STATUS_MODE_CHANGE_IN_PROGRESS";


    /* D3D11 */

  case D3D11_ERROR_FILE_NOT_FOUND:
    return L"D3D11_ERROR_FILE_NOT_FOUND";

  case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:
    return L"D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";

  case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS:
    return L"D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS";

  case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD:
    return L"D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD";


    /* D3D9 */

  case D3DERR_WRONGTEXTUREFORMAT:
    return L"D3DERR_WRONGTEXTUREFORMAT";

  case D3DERR_UNSUPPORTEDCOLOROPERATION:
    return L"D3DERR_UNSUPPORTEDCOLOROPERATION";

  case D3DERR_UNSUPPORTEDCOLORARG:
    return L"D3DERR_UNSUPPORTEDCOLORARG";

  case D3DERR_UNSUPPORTEDALPHAOPERATION:
    return L"D3DERR_UNSUPPORTEDALPHAOPERATION";

  case D3DERR_UNSUPPORTEDALPHAARG:
    return L"D3DERR_UNSUPPORTEDALPHAARG";

  case D3DERR_TOOMANYOPERATIONS:
    return L"D3DERR_TOOMANYOPERATIONS";

  case D3DERR_CONFLICTINGTEXTUREFILTER:
    return L"D3DERR_CONFLICTINGTEXTUREFILTER";

  case D3DERR_UNSUPPORTEDFACTORVALUE:
    return L"D3DERR_UNSUPPORTEDFACTORVALUE";

  case D3DERR_CONFLICTINGRENDERSTATE:
    return L"D3DERR_CONFLICTINGRENDERSTATE";

  case D3DERR_UNSUPPORTEDTEXTUREFILTER:
    return L"D3DERR_UNSUPPORTEDTEXTUREFILTER";

  case D3DERR_CONFLICTINGTEXTUREPALETTE:
    return L"D3DERR_CONFLICTINGTEXTUREPALETTE";

  case D3DERR_DRIVERINTERNALERROR:
    return L"D3DERR_DRIVERINTERNALERROR";


  case D3DERR_NOTFOUND:
    return L"D3DERR_NOTFOUND";

  case D3DERR_MOREDATA:
    return L"D3DERR_MOREDATA";

  case D3DERR_DEVICELOST:
    return L"D3DERR_DEVICELOST";

  case D3DERR_DEVICENOTRESET:
    return L"D3DERR_DEVICENOTRESET";

  case D3DERR_NOTAVAILABLE:
    return L"D3DERR_NOTAVAILABLE";

  case D3DERR_OUTOFVIDEOMEMORY:
    return L"D3DERR_OUTOFVIDEOMEMORY";

  case D3DERR_INVALIDDEVICE:
    return L"D3DERR_INVALIDDEVICE";

  case D3DERR_INVALIDCALL:
    return L"D3DERR_INVALIDCALL";

  case D3DERR_DRIVERINVALIDCALL:
    return L"D3DERR_DRIVERINVALIDCALL";

  case D3DERR_WASSTILLDRAWING:
    return L"D3DERR_WASSTILLDRAWING";


  case D3DOK_NOAUTOGEN:
    return L"D3DOK_NOAUTOGEN";


    /* D3D12 */

    //case D3D12_ERROR_FILE_NOT_FOUND:
    //return L"D3D12_ERROR_FILE_NOT_FOUND";

    //case D3D12_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:
    //return L"D3D12_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";

    //case D3D12_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS:
    //return L"D3D12_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS";
#endif


    /* Generic (FAILED) */

  case E_FAIL:
    return L"E_FAIL";

  case E_INVALIDARG:
    return L"E_INVALIDARG";

  case E_OUTOFMEMORY:
    return L"E_OUTOFMEMORY";

  case E_POINTER:
    return L"E_POINTER";

  case E_ACCESSDENIED:
    return L"E_ACCESSDENIED";

  case E_HANDLE:
    return L"E_HANDLE";

  case E_NOTIMPL:
    return L"E_NOTIMPL";

  case E_NOINTERFACE:
    return L"E_NOINTERFACE";

  case E_ABORT:
    return L"E_ABORT";

  case E_UNEXPECTED:
    return L"E_UNEXPECTED";

  default:
    dll_log.Log (L" *** Encountered unknown HRESULT: (0x%08X)",
      (unsigned long)hr);
    return L"UNKNOWN";
  }
}



UINT
SK_RecursiveMove ( const wchar_t* wszOrigDir,
                   const wchar_t* wszDestDir,
                         bool     replace )
{
  UINT moved = 0;

  wchar_t       wszPath [MAX_PATH * 2 + 1] = { };
  PathCombineW (wszPath, wszOrigDir, L"*");

  WIN32_FIND_DATA fd          = {   };
  HANDLE          hFind       =
    FindFirstFileW ( wszPath, &fd);

  if (hFind == INVALID_HANDLE_VALUE) { return 0; }

  do
  {
    if ( wcscmp (fd.cFileName, L".")  == 0 ||
         wcscmp (fd.cFileName, L"..") == 0 )
    {
      continue;
    }

    if (! (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
      wchar_t       wszOld [MAX_PATH * 2 + 1];
                   *wszOld = L'\0';
      PathCombineW (wszOld, wszOrigDir, fd.cFileName);

      wchar_t       wszNew [MAX_PATH * 2 + 1];
                   *wszNew = L'\0';
      PathCombineW (wszNew, wszDestDir, fd.cFileName);


      bool move = true;

      if (GetFileAttributesW (wszNew) != INVALID_FILE_ATTRIBUTES)
        move = replace; // Only move the file if replacement is desired,
                        //   otherwise just delete the original.


      if (StrStrIW (fd.cFileName, L".log"))
      {
        iSK_Logger* log_file = nullptr;

        extern iSK_Logger steam_log,  tex_log,
                          game_debug, crash_log,
                          budget_log;

        if (dll_log.name.find (fd.cFileName) != std::wstring::npos)
        {
          log_file = &dll_log;
        }

        else if (steam_log.name.find (fd.cFileName) != std::wstring::npos)
        {
          log_file = &steam_log;
        }

        else if (crash_log.name.find (fd.cFileName) != std::wstring::npos)
        {
          log_file = &crash_log;
        }

        else if (game_debug.name.find (fd.cFileName) != std::wstring::npos)
        {
          log_file = &game_debug;
        }

        else if (tex_log.name.find (fd.cFileName) != std::wstring::npos)
        {
          log_file = &tex_log;
        }

        else if (budget_log.name.find (fd.cFileName) != std::wstring::npos)
        {
          log_file = &budget_log;
        }

        if (log_file != nullptr && log_file->fLog)
        {
          log_file->lockless = false;
          fflush (log_file->fLog);
          log_file->lock   ();
          fclose (log_file->fLog);
        }

        DeleteFileW                      (wszNew);
        SK_File_MoveNoFail       (wszOld, wszNew);
        SK_File_SetNormalAttribs (wszNew);

        ++moved;

        if (log_file != nullptr && log_file->fLog)// && log_file != &crash_log)
        {
          log_file->name = wszNew;
          log_file->fLog = _wfopen (log_file->name.c_str (), L"a");
          log_file->unlock ();
        }
      }

      else
      {
        if (move)
        {
          DeleteFileW                (wszNew);
          SK_File_MoveNoFail (wszOld, wszNew);
          ++moved;
        }
        else
        {
          std::wstring tmp_dest = SK_GetHostPath ();
                       tmp_dest.append (LR"(\SKI0.tmp)");

          SK_File_MoveNoFail (wszOld, tmp_dest.c_str ());
        }
      }
    }

    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      wchar_t            wszDescend0 [MAX_PATH * 2 + 1] = { };
      PathCombineW      (wszDescend0, wszOrigDir, fd.cFileName);
      PathAddBackslashW (wszDescend0);

      wchar_t            wszDescend1 [MAX_PATH * 2 + 1] = { };
      PathCombineW      (wszDescend1, wszDestDir, fd.cFileName);
      PathAddBackslashW (wszDescend1);

      SK_CreateDirectories (wszDescend1);

      moved +=
        SK_RecursiveMove (wszDescend0, wszDescend1, replace);
    }
  } while (FindNextFile (hFind, &fd));

  FindClose (hFind);

  RemoveDirectoryW (wszOrigDir);

  return moved;
}


#include <SpecialK/diagnostics/memory.h>

PSID
SK_Win32_ReleaseTokenSid (PSID pSid)
{
  if (pSid == nullptr) return pSid;

  if (SK_LocalFree ((HLOCAL)pSid))
    return nullptr;

  assert (pSid == nullptr);

  return pSid;
}

#include <aclapi.h>

PSID
SK_Win32_GetTokenSid (_TOKEN_INFORMATION_CLASS tic)
{
  PSID   pRet            = nullptr;
  BYTE   token_buf [256] = { };
  LPVOID pTokenBuf       = nullptr;
  DWORD  dwAllocSize     = 0;
  HANDLE hToken          = INVALID_HANDLE_VALUE;

  if (OpenProcessToken (GetCurrentProcess (), TOKEN_QUERY, &hToken))
  {
    if (! GetTokenInformation (hToken, tic, nullptr,
                               0, &dwAllocSize))
    {
      if (GetLastError () == ERROR_INSUFFICIENT_BUFFER)
      {
        if (     tic == TokenUser           && dwAllocSize <= 256)
          pTokenBuf = token_buf;
        else if (tic == TokenIntegrityLevel && dwAllocSize <= 256)
          pTokenBuf = token_buf;

        if (pTokenBuf != nullptr)
        {
          if (GetTokenInformation (hToken, tic, pTokenBuf,
                                   dwAllocSize, &dwAllocSize))
          {
            DWORD dwSidLen =
              GetLengthSid (((SID_AND_ATTRIBUTES *)pTokenBuf)->Sid);

            pRet =
              SK_LocalAlloc ( LPTR, dwSidLen );

            if (pRet != nullptr)
              CopySid (dwSidLen, pRet, ((SID_AND_ATTRIBUTES *)pTokenBuf)->Sid);
          }
        }
      }
    }

    CloseHandle (hToken);
  }

  return pRet;
}