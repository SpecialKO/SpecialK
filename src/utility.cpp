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

#include <UserEnv.h>
#pragma comment (lib, "userenv.lib")

#include <Shlobj.h>
#pragma comment (lib, "shell32.lib")

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
SK_WideCharToUTF8 (std::wstring in)
{
  int len = WideCharToMultiByte ( CP_UTF8, 0x00, in.c_str (), -1, nullptr, 0, nullptr, FALSE );

  std::string out;
              out.resize (len);

  WideCharToMultiByte           ( CP_UTF8, 0x00, in.c_str (), static_cast <int> (in.length ()), const_cast <char *> (out.data ()), len, nullptr, FALSE );

  return out;
}

std::wstring
SK_UTF8ToWideChar (std::string in)
{
  int len = MultiByteToWideChar ( CP_UTF8, 0x00, in.c_str (), -1, nullptr, 0 );

  std::wstring out;
               out.resize (len);

  MultiByteToWideChar           ( CP_UTF8, 0x00, in.c_str (), static_cast <int> (in.length ()), const_cast <wchar_t *> (out.data ()), len );

  return out;
}

std::wstring
SK_GetDocumentsDir (void)
{
  CHandle               hToken;
  CComHeapPtr <wchar_t> str;

  if (! OpenProcessToken (GetCurrentProcess (), TOKEN_READ, &hToken.m_h))
    return L"(null)";

  SHGetKnownFolderPath (FOLDERID_Documents, 0, hToken, &str);

  return std::wstring (str);
}

std::wstring
SK_GetFontsDir (void)
{
  CHandle               hToken;
  CComHeapPtr <wchar_t> str;

  if (! OpenProcessToken (GetCurrentProcess (), TOKEN_READ, &hToken.m_h))
    return L"(null)";

  SHGetKnownFolderPath (FOLDERID_Fonts, 0, hToken, &str);

  return std::wstring (str);
}

bool
SK_GetDocumentsDir (wchar_t* buf, uint32_t* pdwLen)
{
  CHandle               hToken;
  CComHeapPtr <wchar_t> str;

  if (! OpenProcessToken (GetCurrentProcess (), TOKEN_READ, &hToken.m_h))
  {
    if (buf != nullptr && pdwLen != nullptr && *pdwLen > 0)
    {
      *pdwLen = 0;
      *buf    = L'\0';
    }

    return false;
  }

  if ( SUCCEEDED (
         SHGetKnownFolderPath (
           FOLDERID_Documents, 0, hToken, &str
         )
       )
     )
  {
    if (buf != nullptr && pdwLen != nullptr && *pdwLen > 0)
    {
      *buf = '\0';
      wcsncat (buf, str, *pdwLen);

      return true;
    }
  }

  return false;
}

bool
SK_GetUserProfileDir (wchar_t* buf, uint32_t* pdwLen)
{
  CHandle hToken;

  if (! OpenProcessToken (GetCurrentProcess (), TOKEN_READ, &hToken.m_h))
    return false;

  if (! GetUserProfileDirectory ( hToken, buf,
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
SK_CreateDirectories ( const wchar_t* wszPath )
{
  CHeapPtr <wchar_t>
           wszSubDir         (_wcsdup (wszPath));

  wchar_t* iter;
  wchar_t* wszLastSlash     = wcsrchr (wszSubDir, L'/');
  wchar_t* wszLastBackslash = wcsrchr (wszSubDir, L'\\');

  if (wszLastSlash > wszLastBackslash)
    *wszLastSlash = L'\0';
  else if (wszLastBackslash != nullptr)
    *wszLastBackslash = L'\0';
  else
    return false;

  for (iter = wszSubDir; *iter != L'\0'; iter = CharNextW (iter))
  {
    if (*iter == L'\\' || *iter == L'/')
    {
      *iter = L'\0';

      if (GetFileAttributes (wszPath) == INVALID_FILE_ATTRIBUTES)
        CreateDirectoryW (wszSubDir, nullptr);

      *iter = L'\\';
    }
  }

  // The final subdirectory (FULL PATH)
  if (GetFileAttributes (wszPath) == INVALID_FILE_ATTRIBUTES)
    CreateDirectoryW (wszSubDir, nullptr);

  return true;
}

std::wstring
SK_EvalEnvironmentVars (const wchar_t* wszEvaluateMe)
{
  CHeapPtr <wchar_t> wszEvaluated;
  wszEvaluated.Allocate (MAX_PATH + 2);

  if (wszEvaluated == nullptr)
    return L"OUT_OF_MEMORY";

  ExpandEnvironmentStringsW ( wszEvaluateMe,
                                wszEvaluated,
                                  MAX_PATH + 2 );

  std::wstring ret_str (wszEvaluated);

  return ret_str;
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
SK_File_FullCopy (std::wstring from, std::wstring to)
{
  // Strip Read-Only
  SK_File_SetNormalAttribs (to);

  DeleteFile (to.c_str   ());
  CopyFile   (from.c_str (), to.c_str (), FALSE);

  WIN32_FIND_DATA FromFileData;
  HANDLE          hFrom =
    FindFirstFile (from.c_str (), &FromFileData);

  CHandle hTo (
    CreateFile ( to.c_str (),
                   GENERIC_READ      | GENERIC_WRITE,
                     FILE_SHARE_READ | FILE_SHARE_WRITE |
                                       FILE_SHARE_DELETE,
                       nullptr,
                         OPEN_EXISTING,
                           GetFileAttributes (to.c_str ()),
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
SK_File_SetAttribs (std::wstring file, DWORD dwAttribs)
{
  return
    SetFileAttributesW (
      file.c_str (),
         dwAttribs );
}

BOOL
SK_File_ApplyAttribMask (std::wstring file, DWORD dwAttribMask, bool clear)
{
  DWORD dwFileMask =
    GetFileAttributesW (file.c_str ());

  if (clear)
    dwAttribMask = ( dwFileMask & ~dwAttribMask );

  else
    dwAttribMask = ( dwFileMask |  dwAttribMask );

  return SK_File_SetAttribs (file, dwAttribMask);
}

BOOL
SK_File_SetHidden (std::wstring file, bool hide)
{
  return SK_File_ApplyAttribMask (file, FILE_ATTRIBUTE_HIDDEN, (! hide));
}

BOOL
SK_File_SetTemporary (std::wstring file, bool temp)
{
  return SK_File_ApplyAttribMask (file, FILE_ATTRIBUTE_TEMPORARY, (! temp));
}


void
SK_File_SetNormalAttribs (std::wstring file)
{
  SK_File_SetAttribs (file, FILE_ATTRIBUTE_NORMAL);
}


bool
SK_IsAdmin (void)
{
  bool    bRet = false;
  CHandle hToken;

  if ( OpenProcessToken ( GetCurrentProcess (),
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



LPVOID
SK_GetProcAddress (const wchar_t* wszModule, const char* szFunc)
{
  HMODULE hMod = 
    GetModuleHandle (wszModule);

  if (hMod != nullptr)
   return GetProcAddress (hMod, szFunc);

  return nullptr;
}

std::wstring
SK_GetModuleFullName (HMODULE hDll)
{
  wchar_t wszDllFullName [MAX_PATH + 2] = { };

  GetModuleFileName ( hDll,
                        wszDllFullName,
                          MAX_PATH );

  return wszDllFullName;
}

std::wstring
SK_GetModuleName (HMODULE hDll)
{
  wchar_t wszDllFullName [MAX_PATH + 2] = { };

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

  if (hModOut != INVALID_HANDLE_VALUE)
    return SK_GetModuleName (hModOut);

  return L"#Invalid.dll#";
}

std::wstring
SK_GetModuleFullNameFromAddr (LPCVOID addr)
{
  HMODULE hModOut =
    SK_GetModuleFromAddr (addr);

  if (hModOut != INVALID_HANDLE_VALUE)
    return SK_GetModuleFullName (hModOut);

  return L"#Extremely#Invalid.dll#";
}

std::wstring
SK_MakePrettyAddress (LPCVOID addr, DWORD /*dwFlags*/)
{
  return
    SK_FormatStringW ( L"( %s ) + %6xh",
      SK_StripUserNameFromPathW (
                         SK_GetModuleFullNameFromAddr (addr).data () ),
                                            (uintptr_t)addr -
                      (uintptr_t)SK_GetModuleFromAddr (addr) );
}


bool
SK_ValidatePointer (LPCVOID addr)
{
  MEMORY_BASIC_INFORMATION minfo = { };

  if (VirtualQuery (addr, &minfo, sizeof minfo))
  {
    if (! (minfo.Protect & PAGE_NOACCESS))
      return true;
  }

  SK_LOG0 ( ( L"Address Validation for addr. %s FAILED!",
                SK_MakePrettyAddress (addr).c_str () ),
              L" SK Debug " );

  return false;
}

bool
SK_IsAddressExecutable (LPCVOID addr)
{
  MEMORY_BASIC_INFORMATION minfo = { };

  if (VirtualQuery (addr, &minfo, sizeof minfo))
  {
    if (! (minfo.Protect & PAGE_NOACCESS))
    {
      if ( (minfo.Protect & ( PAGE_EXECUTE           |
                              PAGE_EXECUTE_READ      |
                              PAGE_EXECUTE_READWRITE |
                              PAGE_EXECUTE_WRITECOPY   )
           )
         )
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
  PROCESSENTRY32 pe32 =
    FindProcessByName (L"RTSS.exe");

  wchar_t wszPath [MAX_PATH] = { };

  if (wcsstr (pe32.szExeFile, L"RTSS.exe"))
  {
    CHandle hProcess (
      OpenProcess (PROCESS_QUERY_LIMITED_INFORMATION , FALSE, pe32.th32ProcessID)
    );

    DWORD len = MAX_PATH;
    QueryFullProcessImageName (hProcess, 0, wszPath, &len);

    wchar_t* wszRTSS =
      wcsstr (wszPath, L"RTSS.exe");

    if (wszRTSS != nullptr)
      *wszRTSS = L'\0';
  }

  return wszPath;
}

#include <SpecialK/ini.h>

iSK_INI*
SK_GetDLLConfig (void)
{
  extern iSK_INI* dll_ini;
  return dll_ini;
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
    //FreeLibrary (SK_GetDLL ());
    const wchar_t* wszBackend = SK_GetBackend ();

    if (     ! _wcsicmp (wszBackend, L"d3d9"))
      SK::D3D9::Shutdown ();
    else if (! _wcsicmp (wszBackend, L"dxgi"))
      SK::DXGI::Shutdown ();
    else if (! _wcsicmp (wszBackend, L"dinput8"))
      SK::DI8::Shutdown ();
#ifndef SK_BUILD__INSTALLER
    else if (! _wcsicmp (wszBackend, L"OpenGL32"))
      SK::OpenGL::Shutdown ();
#endif

    using  TerminateProcess_pfn = BOOL (WINAPI *)(HANDLE hProcess, UINT uExitCode);

    ( (TerminateProcess_pfn)GetProcAddress ( GetModuleHandle ( L"kernel32.dll" ),
                                               "TerminateProcess" )
    )(GetCurrentProcess (), 0x00);
  }
}




#include <SpecialK/tls.h>

HMODULE
SK_GetCallingDLL (LPCVOID pReturn)
{
  HMODULE hCallingMod = nullptr;

  if (SK_TLS_Bottom ()->known_modules.contains (pReturn, &hCallingMod))
  {
    return hCallingMod;
  }

  GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT |
                      GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                        static_cast <const wchar_t *> (pReturn),
                          &hCallingMod );

  SK_TLS_Bottom ()->known_modules.insert (pReturn, hCallingMod);

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
               EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH ) {
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
//#pragma comment (lib, "Mincore_Downlevel.lib") // Windows 8     (Delay-Load)
#pragma comment (lib, "version.lib")             // Windows 2000+ (Normal Import)

bool
__stdcall
SK_IsDLLSpecialK (const wchar_t* wszName)
{
  UINT     cbTranslatedBytes = 0,
           cbProductBytes    = 0;

  uint8_t  cbData     [4096] = { };

  wchar_t* wszProduct        = nullptr; // Will point somewhere in cbData

  struct LANGANDCODEPAGE {
    WORD wLanguage;
    WORD wCodePage;
  } *lpTranslate = nullptr;

  wchar_t wszFullyQualifiedName [MAX_PATH * 2] = { };

  lstrcatW (wszFullyQualifiedName, SK_GetHostPath ());
  lstrcatW (wszFullyQualifiedName, L"\\");
  lstrcatW (wszFullyQualifiedName, wszName);

  if (GetFileAttributes (wszFullyQualifiedName) == INVALID_FILE_ATTRIBUTES)
    return false;

  GetFileVersionInfoEx ( FILE_VER_GET_NEUTRAL |
                         FILE_VER_GET_PREFETCHED,
                           wszFullyQualifiedName,
                             0x00,
                               4096,
                                 cbData );

  if ( VerQueryValue ( cbData,
                       TEXT ("\\VarFileInfo\\Translation"),
           static_cast_p2p <void> (&lpTranslate),
                                   &cbTranslatedBytes ) && cbTranslatedBytes && lpTranslate )
  {
    wchar_t wszPropName [64] = { };

    wsprintfW ( wszPropName,
                  LR"(\StringFileInfo\%04x%04x\ProductName)",
                    lpTranslate   [0].wLanguage,
                      lpTranslate [0].wCodePage );

    VerQueryValue ( cbData,
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

  uint8_t  cbData     [4096] = { };

  wchar_t* wszFileDescrip = nullptr; // Will point somewhere in cbData
  wchar_t* wszFileVersion = nullptr; // "

  struct LANGANDCODEPAGE {
    WORD wLanguage;
    WORD wCodePage;
  } *lpTranslate = nullptr;

  if (GetFileAttributes (wszName) == INVALID_FILE_ATTRIBUTES)
    return L"N/A";

  GetFileVersionInfoEx ( FILE_VER_GET_NEUTRAL |
                         FILE_VER_GET_PREFETCHED,
                           wszName,
                             0x00,
                               4096,
                                 cbData );

  if ( VerQueryValue ( cbData,
                         TEXT ("\\VarFileInfo\\Translation"),
             static_cast_p2p <void> (&lpTranslate),
                                     &cbTranslatedBytes ) && cbTranslatedBytes && lpTranslate )
  {
    wchar_t wszPropName [64] = { };

    wsprintfW ( wszPropName,
                  LR"(\StringFileInfo\%04x%04x\FileDescription)",
                    lpTranslate   [0].wLanguage,
                      lpTranslate [0].wCodePage );

    VerQueryValue ( cbData,
                      wszPropName,
          static_cast_p2p <void> (&wszFileDescrip),
                                  &cbProductBytes );

    wsprintfW ( wszPropName,
                  LR"(\StringFileInfo\%04x%04x\FileVersion)",
                    lpTranslate   [0].wLanguage,
                      lpTranslate [0].wCodePage );

    VerQueryValue ( cbData,
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
    ret += wszFileDescrip;
    ret += L"  ";
  }

  if (cbVersionBytes)
    ret += wszFileVersion;

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
SK_ScanAlignedEx2 (const void* pattern, size_t len, const void* mask, void* after, int align, uint8_t* base_addr)
{
  MEMORY_BASIC_INFORMATION minfo;
  VirtualQuery (base_addr, &minfo, sizeof minfo);

  //
  // VMProtect kills this, so let's do something else...
  //
#ifdef VMPROTECT_IS_NOT_A_FACTOR
  IMAGE_DOS_HEADER* pDOS =
    (IMAGE_DOS_HEADER *)minfo.AllocationBase;
  IMAGE_NT_HEADERS* pNT  =
    (IMAGE_NT_HEADERS *)((uintptr_t)(pDOS + pDOS->e_lfanew));

  uint8_t* end_addr = base_addr + pNT->OptionalHeader.SizeOfImage;
#else
           base_addr = static_cast <uint8_t *> (minfo.BaseAddress);//AllocationBase;
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
      static_cast <uint8_t *> (PAGE_WALK_LIMIT);
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

  while (it < end_addr)
  {
    VirtualQuery (it, &minfo, sizeof minfo);

    // Bail-out once we walk into an address range that is not resident, because
    //   it does not belong to the original executable.
    if (minfo.RegionSize == 0)
      break;

    uint8_t* next_rgn =
     (uint8_t *)minfo.BaseAddress + minfo.RegionSize;

    if ( (! (minfo.Type    & MEM_IMAGE))  ||
         (! (minfo.State   & MEM_COMMIT)) ||
             minfo.Protect & PAGE_NOACCESS )
    {
      it    = next_rgn;
      idx   = 0;
      begin = it;

      continue;
    }

    // Do not search past the end of the module image!
    if (next_rgn >= end_addr)
      break;

    while (it < next_rgn)
    {
      uint8_t* scan_addr = it;

      bool match = (*scan_addr == static_cast <const uint8_t *> (pattern) [idx]);

      // For portions we do not care about... treat them
      //   as matching.
      if (mask != nullptr && (! static_cast <const uint8_t *> (mask) [idx]))
        match = true;

      if (match)
      {
        if (++idx == len)
        {
          if ((reinterpret_cast <uintptr_t> (begin) % align) == 0)
          {
            return
              static_cast <void *> (begin);
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
SK_ScanAlignedEx (const void* pattern, size_t len, const void* mask, void* after, int align)
{
  auto* base_addr =
    reinterpret_cast <uint8_t *> (GetModuleHandle (nullptr));

  return SK_ScanAlignedEx2 (pattern, len, mask, after, align, base_addr);
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
                  void   *new_data,
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
      if (old_data != nullptr) memcpy (old_data, base_addr, data_size);
                               memcpy (base_addr, new_data, data_size);

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
  wchar_t wszApp       [MAX_PATH * 2] = { };
  wchar_t wszPath      [MAX_PATH * 2] = { };
  wchar_t wszFullName  [MAX_PATH * 2] = { };
  wchar_t wszBlacklist [MAX_PATH * 2] = { };
  wchar_t wszSystemDir [MAX_PATH * 2] = { };
} host_proc;

bool __SK_RunDLL_Bypass = false;

bool
__cdecl
SK_IsHostAppSKIM (void)
{
  return ((! __SK_RunDLL_Bypass) && StrStrIW (SK_GetHostApp (), L"SKIM") != nullptr);
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
    wchar_t* wszArgs =
      _wcsdup (PathGetArgsW (GetCommandLineW ()));

    // If the command line does not reference our DLL
    if (! StrStrW (wszArgs, L"RunDLL_"))
      rundll_invoked = false;

    free (wszArgs);
  }

  return rundll_invoked;
}

bool
__cdecl
SK_IsSuperSpecialK (void)
{
  return (SK_IsRunDLLInvocation () || SK_IsHostAppSKIM ());
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
  static volatile
    ULONG init = FALSE;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  {
    lstrcatW (host_proc.wszBlacklist, SK_GetHostPath ());
    lstrcatW (host_proc.wszBlacklist, L"\\SpecialK.deny.");
    lstrcatW (host_proc.wszBlacklist, SK_GetHostApp  ());

    SK_PathRemoveExtension (host_proc.wszBlacklist);
  }

  return host_proc.wszBlacklist;
}

const wchar_t*
SK_GetHostApp (void)
{
  static volatile
    ULONG init = FALSE;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  {
    DWORD   dwProcessSize =  MAX_PATH * 2;
    wchar_t wszProcessName [ MAX_PATH * 2 ] = { };

    HANDLE hProc =
      GetCurrentProcess ();

    QueryFullProcessImageName (
      hProc,
        0,
          wszProcessName,
            &dwProcessSize );

    int      len           = lstrlenW (wszProcessName);
    wchar_t* pwszShortName =           wszProcessName;

    for (int i = 0; i < len; i++)
      pwszShortName = CharNextW (pwszShortName);

    while (  pwszShortName > wszProcessName )
    {
      wchar_t* wszPrev =
        CharPrevW (wszProcessName, pwszShortName);

      if (wszPrev < wszProcessName)
        break;

      if (*wszPrev != L'\\' && *wszPrev != L'/')
      {
        pwszShortName = wszPrev;
        continue;
      }

      break;
    }

    lstrcpynW (
      host_proc.wszApp,
        pwszShortName,
          MAX_PATH * 2 - 1
    );
  }

  return host_proc.wszApp;
}

const wchar_t*
SK_GetFullyQualifiedApp (void)
{
  static volatile
    ULONG init = FALSE;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  {
    DWORD   dwProcessSize = MAX_PATH * 2;
    wchar_t wszProcessName [MAX_PATH * 2] = { };

    HANDLE hProc =
      GetCurrentProcess ();

    QueryFullProcessImageName (
      hProc,
        0,
          wszProcessName,
            &dwProcessSize );

    lstrcpynW (
      host_proc.wszFullName,
        wszProcessName,
          MAX_PATH * 2 - 1
    );
  }

  return host_proc.wszFullName;
}

// NOT the working directory, this is the directory that
//   the executable is located in.

const wchar_t*
SK_GetHostPath (void)
{
  static volatile
    ULONG init = FALSE;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  {
    DWORD   dwProcessSize = MAX_PATH * 2;
    wchar_t wszProcessName [MAX_PATH * 2] = { };

    HANDLE hProc =
      GetCurrentProcess ();

    QueryFullProcessImageName (
      hProc,
        0,
          wszProcessName,
            &dwProcessSize );

    int      len           = lstrlenW (wszProcessName);
    wchar_t* pwszShortName =           wszProcessName;

    for (int i = 0; i < len; i++)
      pwszShortName = CharNextW (pwszShortName);

    wchar_t* wszFirstSep = nullptr;

    while (  pwszShortName > wszProcessName )
    {
      wchar_t* wszPrev =
        CharPrevW (wszProcessName, pwszShortName);

      if (wszPrev < wszProcessName)
        break;

      if (*wszPrev == L'\\' || *wszPrev == L'/')
      {                              // Leave the trailing separator
        wszFirstSep = wszPrev; 
        break;
      }

      pwszShortName = wszPrev;
    }

    if (wszFirstSep != nullptr)
       *wszFirstSep  = L'\0';

    lstrcpynW (
      host_proc.wszPath,
        wszProcessName,
          MAX_PATH * 2 - 1
    );
  }

  return host_proc.wszPath;
}


const wchar_t*
SK_GetSystemDirectory (void)
{
  static volatile
    ULONG init = FALSE;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  {
#ifdef _WIN64
    GetSystemDirectory (host_proc.wszSystemDir, MAX_PATH);
#else
    HANDLE hProc = GetCurrentProcess ();

    BOOL   bWOW64;
    ::IsWow64Process (hProc, &bWOW64);

    if (bWOW64)
      GetSystemWow64Directory (host_proc.wszSystemDir, MAX_PATH);
    else
      GetSystemDirectory      (host_proc.wszSystemDir, MAX_PATH);
#endif
  }

  return host_proc.wszSystemDir;
}




size_t
SK_DeleteTemporaryFiles (const wchar_t* wszPath, const wchar_t* wszPattern)
{
  WIN32_FIND_DATA fd     = {      };
  HANDLE          hFind  = INVALID_HANDLE_VALUE;
  size_t          files  =   0UL;
  LARGE_INTEGER   liSize = { 0ULL };

  wchar_t wszFindPattern [MAX_PATH * 2] = { };

  lstrcatW (wszFindPattern, wszPath);
  lstrcatW (wszFindPattern, L"\\");
  lstrcatW (wszFindPattern, wszPattern);

  hFind = FindFirstFileW (wszFindPattern, &fd);

  if (hFind != INVALID_HANDLE_VALUE)
  {
    dll_log.LogEx ( true, L"[Clean Mgr.] Cleaning temporary files in '%s'...    ", wszPath );

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
          ++files;
      }
    } while (FindNextFileW (hFind, &fd) != 0);

    dll_log.LogEx ( false, L"%zu files deleted\n", files);

    FindClose (hFind);
  }

  return files;
}


bool
SK_FileHasSpaces (const wchar_t* wszLongFileName)
{
  return StrStrIW (wszLongFileName, L" ") != nullptr;
}

BOOL
SK_FileHas8Dot3Name (const wchar_t* wszLongFileName)
{
  wchar_t wszShortPath [MAX_PATH + 2] = { };
 
  if ((! GetShortPathName   (wszLongFileName, wszShortPath, MAX_PATH )) ||
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
    if (!OpenProcessToken(GetCurrentProcess(),
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
  wchar_t* wszFileName  = _wcsdup (wszLongFileName);
  wchar_t* wszFileName1 = _wcsdup (wszLongFileName);

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
      free (wszFileName);
      free (wszFileName1);

      return false;
    }

    DWORD dwAttrib =
      GetFileAttributes (wszFileName);

    if (dwAttrib == INVALID_FILE_ATTRIBUTES)
    {
      free (wszFileName);
      free (wszFileName1);

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

      if (*pwszExt == L'.')
      {
        if (*CharNextW (pwszExt) != L'\0')
        {
          if (*CharNextW (CharNextW (pwszExt)) != L'\0')
          {
            if (*CharNextW (CharNextW (CharNextW (pwszExt))) != L'\0')
            {
              // DOT3 Satisfied
              *CharNextW (CharNextW (CharNextW (CharNextW (pwszExt)))) = L'\0';

              wcsncpy (wszDot3, CharNextW (pwszExt), 3);
            }
          }
        }
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
      free (wszFileName);
      free (wszFileName1);

      return false;
    }

    PathRemoveFileSpec  (wszFileName1);
    wcscpy (wszFileName, wszFileName1);
  }

  free (wszFileName);
  free (wszFileName1);

  return true;
}



void
SK_RestartGame (const wchar_t* wszDLL)
{
  wchar_t wszShortPath [MAX_PATH + 2] = { };
  wchar_t wszFullname  [MAX_PATH + 2] = { };

  wcsncpy ( wszFullname, wszDLL != nullptr ?
                         wszDLL :
                           SK_GetModuleFullName ( SK_GetDLL ()).c_str (),
                                                    MAX_PATH );

  SK_Generate8Dot3 (wszFullname);
  wcscpy           (wszShortPath, wszFullname);
 
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
      global_dll += L"64.dll";
#else
      global_dll += L"32.dll";
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

  TerminateProcess (GetCurrentProcess (), 0x00);
}

void
SK_ElevateToAdmin (void)
{
  wchar_t wszRunDLLCmd [MAX_PATH * 4] = { };
  wchar_t wszShortPath [MAX_PATH + 2] = { };
  wchar_t wszFullname  [MAX_PATH + 2] = { };

  wcsncpy (wszFullname, SK_GetModuleFullName (SK_GetDLL ()).c_str (), MAX_PATH );
 
  SK_Generate8Dot3 (wszFullname);
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

  TerminateProcess    (GetCurrentProcess (), 0x00);
}

#include <memory>

std::string
__cdecl
SK_FormatString (char const* const _Format, ...)
{
  int len = 0;

  va_list   _ArgList;
  va_start (_ArgList, _Format);
  {
    len = vsnprintf (nullptr, 0, _Format, _ArgList);
  }
  va_end (_ArgList);

  auto* out = new char [len + 1] { };

  va_start (_ArgList, _Format);
  {
    len = vsprintf (out, _Format, _ArgList);
  }
  va_end (_ArgList);

  std::string str_out (out);

  delete [] out;

  return str_out;
}

std::wstring
__cdecl
SK_FormatStringW (wchar_t const* const _Format, ...)
{
  int len = 0;

  va_list   _ArgList;
  va_start (_ArgList, _Format);
  {
    len = _vsnwprintf (nullptr, 0, _Format, _ArgList);
  }
  va_end (_ArgList);

  auto* out = new wchar_t [len + 1] { };

  va_start (_ArgList, _Format);
  {
    len = _vswprintf (out, _Format, _ArgList);
  }
  va_end (_ArgList);

  std::wstring wstr_out (out);

  delete [] out;

  return wstr_out;
}


void
SK_StripTrailingSlashesW (wchar_t* wszInOut)
{
  struct test_slashes
  {
    bool operator () (wchar_t a, wchar_t b) const
    {
      auto IsSlash = [](wchar_t a) -> bool {
        return (a == L'\\' || a == L'/');
      };

      return IsSlash (a) && IsSlash (b);
    }
  };
  
  std::wstring wstr (wszInOut);
  
  wstr.erase ( std::unique ( wstr.begin (),
                             wstr.end   (), test_slashes () ),
                 wstr.end () );

  wcscpy (wszInOut, wstr.c_str ());
}

void
SK_FixSlashesW (wchar_t* wszInOut)
{ 
  std::wstring wstr (wszInOut);

  for ( auto&& it : wstr )
  {
    if (it == L'/')
      it = L'\\';
  }

  wcscpy (wszInOut, wstr.c_str ());
}

void
SK_StripTrailingSlashesA (char* szInOut)
{
  struct test_slashes
  {
    bool operator () (char a, char b) const
    {
      auto IsSlash = [](char a) -> bool {
        return (a == '\\' || a == '/');
      };

      return IsSlash (a) && IsSlash (b);
    }
  };
  
  std::string str (szInOut);
  
  str.erase ( std::unique ( str.begin (),
                            str.end   (), test_slashes () ),
                str.end () );

  strcpy (szInOut, str.c_str ());
}

void
SK_FixSlashesA (char* szInOut)
{ 
  std::string str (szInOut);

  for ( auto&& it : str )
  {
    if (it == '/')
      it = '\\';
  }

  strcpy (szInOut, str.c_str ());
}


#define SECURITY_WIN32
#include <Security.h>
#pragma comment (lib, "secur32.lib")

// Doesn't need to be this complicated; it's a string function, might as well optimize it.

char*
SK_StripUserNameFromPathA (char* szInOut)
{
  static char szUserName        [MAX_PATH + 2] = { };
  static char szUserNameDisplay [MAX_PATH + 2] = { };
  static char szUserProfile     [MAX_PATH + 2] = { }; // Most likely to match

  if (*szUserProfile == '\0')
  {
    wchar_t wszUserProfile [MAX_PATH + 2] = { };

                                        uint32_t len = MAX_PATH;
    if (! SK_GetUserProfileDir (wszUserProfile, &len))
      *wszUserProfile = L'?'; // Invalid filesystem char
    else
      PathStripPathW (wszUserProfile);

    strcpy (szUserProfile, SK_WideCharToUTF8 (wszUserProfile).c_str ());
  }

  if (*szUserName == '\0')
  {
                                        DWORD dwLen = MAX_PATH;
    GetUserNameExA (NameUnknown, szUserName, &dwLen);

    if (dwLen == 0)
      *szUserName = '?'; // Invalid filesystem char
    else
      PathStripPathA (szUserName);
  }

  if (*szUserNameDisplay == '\0')
  {
                                               DWORD dwLen = MAX_PATH;
    GetUserNameExA (NameDisplay, szUserNameDisplay, &dwLen);

    if (dwLen == 0)
      *szUserNameDisplay = '?'; // Invalid filesystem char
  }


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
  static wchar_t wszUserName        [MAX_PATH + 2] = { };
  static wchar_t wszUserNameDisplay [MAX_PATH + 2] = { };
  static wchar_t wszUserProfile     [MAX_PATH + 2] = { }; // Most likely to match

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
    GetUserNameExW (NameSamCompatible, wszUserName, &dwLen);

    if (dwLen == 0)
      *wszUserName = L'?'; // Invalid filesystem char
    else
      PathStripPathW (wszUserName);
  }

  if (*wszUserNameDisplay == L'\0')
  {
                                                DWORD dwLen = MAX_PATH;
    GetUserNameExW (NameDisplay, wszUserNameDisplay, &dwLen);

    if (dwLen == 0)
      *wszUserNameDisplay = L'?'; // Invalid filesystem char
  }


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
    CreateEvent (nullptr, FALSE, FALSE, L"DeferredCmdEvent");
  static volatile HANDLE                                      hCommandThread = nullptr;

  for (int i = 0; i < count; i++)
  {
    cmds.push (szCommands [i]);
  }

  SetEvent (hNewCmds);

  // ============================================== //

  if (! InterlockedCompareExchangePointer (&hCommandThread, (LPVOID)1, 0))
  {     InterlockedExchangePointer        ((void **)&hCommandThread,

    CreateThread   ( nullptr, 0x00,
    [ ](LPVOID) ->
    DWORD
    {
      SetCurrentThreadDescription (L"[SK] Async Command Processor");

      SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_BELOW_NORMAL);
      CHandle  hThread  (GetCurrentThread ());

      while (! ReadAcquire (&__SK_DLL_Ending))
      {
        if (WaitForSingleObjectEx (hNewCmds, INFINITE, TRUE) == WAIT_OBJECT_0)
        {
          std::string cmd = "";

          while (cmds.try_pop (cmd))
          {
            SK_GetCommandProcessor ()->ProcessCommandLine (cmd.c_str ());
          }
        }
      }

      CloseHandle (hNewCmds);

      return 0;
    }, nullptr, 0x00, nullptr ) );
  }
};

//
// Issue a command that would deadlock the game if executed synchronously
//   from the render thread.
//
void
SK_DeferCommand (const char* szCommand)
{
  return SK_DeferCommands (&szCommand, 1);
};



SK_HostAppUtil::SK_HostAppUtil (void)
{
  SKIM     = StrStrIW (SK_GetHostApp (), L"SKIM")     != nullptr;
  RunDll32 = StrStrIW (SK_GetHostApp (), L"RunDLL32") != nullptr;
}

SK_HostAppUtil SK_HostApp;

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