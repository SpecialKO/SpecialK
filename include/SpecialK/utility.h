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

#pragma once

#ifndef __SK__UTILITY_H__
#define __SK__UTILITY_H__

#include <SpecialK/log.h>

BOOL SK_IsHandleValid   (HANDLE hHandle);
BOOL SK_SafeCloseHandle (HANDLE hHandle) noexcept;

#define SK_CloseHandle SK_SafeCloseHandle

class SK_AutoHandle
{
  // Signed handles are invalid, since handles are pointers and
  //   the signed half of the address space is only for kernel

public:
   SK_AutoHandle (_Inout_ SK_AutoHandle& h) noexcept : m_h (nullptr)
   {
     Attach (
       h.Detach ()
     );
   }

   explicit
   SK_AutoHandle (HANDLE hHandle) noexcept : m_h (hHandle)              { };
   SK_AutoHandle (void)           noexcept : m_h (INVALID_HANDLE_VALUE) { };
  ~SK_AutoHandle (void)           noexcept
  {
    // We cannot close these handles because technically they
    //   were never opened (by usermode code).
    if ((intptr_t)m_h < (intptr_t)nullptr)
                  m_h =           nullptr;

    // Signed handles are often special cases
    //   such as -2 = Current Thread, -1 = Current Process
    if (m_h != nullptr)
    {
      Close ();
    }
  }

  SK_AutoHandle& operator= (_Inout_ SK_AutoHandle& h) noexcept
  {
    if (this != &h)
    {
      if (m_h != nullptr)
      {
        __try {
          Close ();
        }

        __except (EXCEPTION_EXECUTE_HANDLER)
        {
          std::ignore = m_h;
        }
      }

      Attach (
        h.Detach ()
      );
    }

    return (*this);
  }

  operator HANDLE (void) const noexcept
  {
    return m_h;
  };

  // Attach to an existing handle (takes ownership).
  void Attach (_In_ HANDLE h) noexcept
  {
    //SK_ReleaseAssert (m_h == nullptr);
    m_h = h; // Take ownership
  }

  // Detach the handle from the object (releases ownership).
  HANDLE Detach (void) noexcept
  {
    return (  // Release ownership
      std::exchange (m_h, nullptr)
    );
  }

  // Close the handle.
  void Close (void) noexcept
  {
    __try
    {
      __try
      {
        CloseHandle (m_h);
      }

      __finally
      {
        m_h = nullptr;
      }
    }

    // Anti-debug does stuff here...
    __except (GetExceptionCode () == EXCEPTION_INVALID_HANDLE  ?
                                     EXCEPTION_EXECUTE_HANDLER :
                                     EXCEPTION_CONTINUE_SEARCH )
    {
      (void)m_h;
    }
  }

  bool isValid (void) const noexcept
  {
    return
      (reinterpret_cast <intptr_t> (m_h) > 0);
  }

public:
  HANDLE m_h;
};

#include <Unknwnbase.h>

#include <intrin.h>
#include <Windows.h>

#include <cstdint>
#include <mutex>
#include <queue>
#include <vector>
#include <bitset>
#include <array>
#include <string>
#include <typeindex>

#include <SpecialK/SpecialK.h>
#include <SpecialK/ini.h>
#include <SpecialK/core.h>
#include <SpecialK/sha1.h>
#include <SpecialK/framerate.h>
#include <SpecialK/diagnostics/debug_utils.h>

#include <atlbase.h>

using HANDLE = void *;

enum SK_UNITS {
  Celsius    = 0,
  Fahrenheit = 1,
  B          = 2,
  KiB        = 3,
  MiB        = 4,
  GiB        = 5,
  Auto       = MAXDWORD
};


//
// NOTE: Most of these functions are not intended to be DLL exported, so returning and
//         passing std::wstring is permissible for convenience.
//

enum DLL_ROLE : unsigned;


const wchar_t* __stdcall
               SK_GetRootPath               (void);
const wchar_t* SK_GetHostApp                (void);
const wchar_t* SK_GetHostPath               (void);
const wchar_t* SK_GetBlacklistFilename      (void);

bool           SK_GetDocumentsDir           (_Out_opt_ wchar_t* buf, _Inout_ uint32_t* pdwLen);
std::wstring&  SK_GetDocumentsDir           (void);
std::wstring&  SK_GetLocalAppDataDir        (void);
std::wstring   SK_GetFontsDir               (void);
std::wstring   SK_GetRTSSInstallDir         (void);
bool
__stdcall      SK_CreateDirectories         (const wchar_t* wszPath);
bool
__stdcall      SK_CreateDirectoriesEx       (const wchar_t* wszPath, bool strip_filespec);
uint64_t       SK_DeleteTemporaryFiles      (const wchar_t* wszPath    = SK_GetHostPath (),
                                             const wchar_t* wszPattern = L"SKI*.tmp");
std::wstring   SK_EvalEnvironmentVars       (const wchar_t* wszEvaluateMe);
bool           SK_GetUserProfileDir         (wchar_t*       buf, uint32_t* pdwLen);
bool           SK_IsTrue                    (const wchar_t* string);
bool           SK_IsAdmin                   (void);
void           SK_ElevateToAdmin            (const wchar_t* wszCmd     = nullptr,
                                             bool           bRestart   = true);
void           SK_RestartGame               (const wchar_t* wszDLL     = nullptr,
                                             const wchar_t* wszFailMsg = nullptr);
int            SK_MessageBox                (std::wstring caption,
                                             std::wstring title,
                                             uint32_t     flags);

LPVOID         SK_Win32_GetTokenInfo        (_TOKEN_INFORMATION_CLASS tic);
LPVOID         SK_Win32_ReleaseTokenInfo    (LPVOID                   lpTokenBuf);

time_t         SK_Win32_FILETIME_to_time_t  (FILETIME const& ft);
FILETIME       SK_Win32_time_t_to_FILETIME  (time_t          epoch);

std::string    SK_WideCharToUTF8            (const std::wstring& in);
std::wstring   SK_UTF8ToWideChar            (const std::string&  in);

std::string
__cdecl        SK_FormatString              (char    const* const _Format, ...);
std::wstring
__cdecl        SK_FormatStringW             (wchar_t const* const _Format, ...);
size_t
__cdecl        SK_FormatStringView          (std::string_view&  out,    char const* const _Format, ...);
size_t
__cdecl        SK_FormatStringViewW         (std::wstring_view& out, wchar_t const* const _Format, ...);

void           SK_StripTrailingSlashesW     (wchar_t *wszInOut);
void           SK_StripTrailingSlashesA     (char    *szInOut);
void           SK_FixSlashesW               (wchar_t *wszInOut);
void           SK_FixSlashesA               (char    *szInOut);
void           SK_StripLeadingSlashesW      (wchar_t *wszInOut);
void           SK_StripLeadingSlashesA      (char    *szInOut);

void           SK_File_SetNormalAttribs     (const wchar_t* file);
void           SK_File_MoveNoFail           (const wchar_t* wszOld,    const wchar_t* wszNew);
void           SK_File_FullCopy             (const wchar_t* from,      const wchar_t* to);
BOOL           SK_File_SetAttribs           (const wchar_t* file,      DWORD          dwAttribs);
BOOL           SK_File_ApplyAttribMask      (const wchar_t* file,      DWORD          dwAttribMask,
                                             bool           clear = false);
BOOL           SK_File_SetHidden            (const wchar_t* file,      bool           hidden);
BOOL           SK_File_SetTemporary         (const wchar_t* file,      bool           temp);
uint64_t       SK_File_GetSize              (const wchar_t* wszFile);
BOOL           SK_File_GetAttribs           (const wchar_t* wszFile, const wchar_t* wszPerms = nullptr, int buf_len = 0);


//
// These all return a view of Thread Local Memory; the returned view is not valid across multiple calls.
//
//   -:- Copy the returned data ASAP
//
std::wstring_view
               SK_File_SizeToString         (uint64_t       size,      SK_UNITS       unit = Auto, SK_TLS *pTLS = nullptr);
std::string_view
               SK_File_SizeToStringA        (uint64_t       size,      SK_UNITS       unit = Auto, SK_TLS *pTLS = nullptr);
std::wstring_view
               SK_File_SizeToStringF        (uint64_t       size,      int            width,
                                             int            precision, SK_UNITS       unit = Auto, SK_TLS *pTLS = nullptr);
std::string_view
               SK_File_SizeToStringAF       (uint64_t       size,      int            width,
                                             int            precision, SK_UNITS       unit = Auto, SK_TLS *pTLS = nullptr);

bool           SK_File_IsDirectory          (const wchar_t* wszPath);
bool           SK_File_CanUserWriteToPath   (const wchar_t* wszPath);

std::wstring   SK_SYS_GetInstallPath        (void);

const wchar_t* SK_GetHostApp                (void);
const wchar_t* SK_GetSystemDirectory        (void);
iSK_INI*       SK_GetDLLConfig              (void);

#pragma intrinsic (_ReturnAddress)

HMODULE        SK_GetCallingDLL             (LPCVOID pReturn = _ReturnAddress ());
std::wstring   SK_GetCallerName             (LPCVOID pReturn = _ReturnAddress ());
HMODULE        SK_GetModuleFromAddr         (LPCVOID addr);
std::wstring   SK_GetModuleName             (HMODULE hDll);
std::wstring   SK_GetModuleFullName         (HMODULE hDll);
std::wstring   SK_GetModuleNameFromAddr     (LPCVOID addr);
std::wstring   SK_GetModuleFullNameFromAddr (LPCVOID addr);
std::wstring   SK_MakePrettyAddress         (LPCVOID addr, DWORD dwFlags = 0x0);
bool           SK_IsModuleLoaded            (const wchar_t* wszModule); // Avoids GetModuleHandle and the loader-lock
bool           SK_ValidatePointer           (LPCVOID addr, bool silent = false, MEMORY_BASIC_INFORMATION *pmi = nullptr);
bool           SK_SAFE_ValidatePointer      (LPCVOID addr, bool silent = false, MEMORY_BASIC_INFORMATION *pmi = nullptr) noexcept;
bool           SK_IsAddressExecutable       (LPCVOID addr, bool silent = false);
void           SK_LogSymbolName             (LPCVOID addr);

char*          SK_StripUserNameFromPathA    (   char*  szInOut);
wchar_t*       SK_StripUserNameFromPathW    (wchar_t* wszInOut);

#define SK_ConcealUserDir  SK_StripUserNameFromPathW
#define SK_ConcealUserDirA SK_StripUserNameFromPathA

FARPROC WINAPI SK_GetProcAddress            (const HMODULE  hMod,      const char* szFunc) noexcept;
FARPROC WINAPI SK_GetProcAddress            (const wchar_t* wszModule, const char* szFunc);


std::wstring
     __stdcall SK_GetDLLVersionStr          (const wchar_t* wszDLL);
std::wstring
     __stdcall SK_GetDLLVersionShort        (const wchar_t* wszDLL);
std::wstring
     __stdcall SK_GetDLLProductName         (const wchar_t* wszDLL);
bool __stdcall SK_Assert_SameDLLVersion     (const wchar_t* wszTestFile0,
                                             const wchar_t* wszTestFile1);

const wchar_t*
        __stdcall
               SK_GetCanonicalDLLForRole    (enum DLL_ROLE role);

const wchar_t* SK_DescribeHRESULT           (HRESULT hr);

void           SK_DeferCommand              (const char* szCommand);

void           SK_GetSystemInfo             (LPSYSTEM_INFO lpSystemInfo);

PSID SK_Win32_GetTokenSid     (_TOKEN_INFORMATION_CLASS tic );
PSID SK_Win32_ReleaseTokenSid (PSID                     pSid);

extern void WINAPI  SK_ExitProcess      (      UINT      uExitCode  ) noexcept;
extern void WINAPI  SK_ExitThread       (      DWORD     dwExitCode ) noexcept;
extern BOOL WINAPI  SK_TerminateThread  (      HANDLE    hThread,
                                               DWORD     dwExitCode ) noexcept;
extern BOOL WINAPI  SK_TerminateProcess (      HANDLE    hProcess,
                                               UINT      uExitCode  ) noexcept;
extern void __cdecl SK__endthreadex     ( _In_ unsigned _ReturnCode ) noexcept;
 
char* SK_CharNextA (const char *szInput, int n = 1);

static inline wchar_t*
SK_CharNextW (const wchar_t *wszInput, size_t n = 1)
{
  if (n <= 0 || wszInput == nullptr) [[unlikely]]
    return nullptr;

  return
    const_cast <wchar_t *> (wszInput + n);
};

static inline wchar_t*
SK_CharPrevW (const wchar_t *start, const wchar_t *x)
{
  if (x > start) return const_cast <wchar_t *> (x - 1);
  else           return const_cast <wchar_t *> (x);
}

enum SK_Bitness {
  ThirtyTwoBit = 32,
  SixtyFourBit = 64
};

constexpr SK_Bitness
__stdcall
SK_GetBitness (void)
{
#ifdef _WIN64
  return SixtyFourBit;
#endif
  return ThirtyTwoBit;
}

// Avoid the C++ stdlib and use CPU interlocked instructions instead, so this
//   is safe to use even by parts of the DLL that run before the CRT initializes
#define SK_RunOnce(x) do { static volatile LONG __once = TRUE; \
               if (InterlockedCompareExchange (&__once, FALSE, TRUE)) { x; } } while (0)
static inline auto
        SK_RunOnceEx =
              [](auto x){ static std::once_flag the_wuncler;
                            std::call_once (    the_wuncler, [&]{ (x)(); }); };

#define SK_RunIf32Bit(x)         { SK_GetBitness () == ThirtyTwoBit  ? (x) :  0; }
#define SK_RunIf64Bit(x)         { SK_GetBitness () == SixtyFourBit  ? (x) :  0; }
#define SK_RunLHIfBitness(b,l,r)  (SK_GetBitness () == (b)           ? (l) : (r))


#define SK_LOG_FIRST_CALL SK_RunOnce ({                                               \
        SK_LOG0 ( (L"[!] > First Call: %34s", __FUNCTIONW__),      __SK_SUBSYSTEM__); \
        SK_LOG1 ( (L"    <*> %s", SK_SummarizeCaller ().c_str ()), __SK_SUBSYSTEM__); });


void SK_ImGui_Warning          (const wchar_t* wszMessage);
void SK_ImGui_WarningWithTitle (const wchar_t* wszMessage,
                                const wchar_t* wszTitle);

using SK_ImGui_ToastOwnerDrawn_pfn = bool (__stdcall *)(void *pUserData);

struct SK_ImGui_Toast {
  enum Type {
    Success,
    Warning,
    Error,
    Info,
    Other
  } type = Other;

  enum Flags {
    UseDuration  = 0x001,
    ShowCaption  = 0x002,
    ShowTitle    = 0x004,
    ShowOnce     = 0x008,
    ShowNewest   = 0x010,
    ShowDismiss  = 0x020,
    Silenced     = 0x040,
    Unsilencable = 0x080,
    DoNotSaveINI = 0x100,
    ShowCfgPanel = 0x200
  } flags = (Flags)(UseDuration | ShowCaption | ShowTitle);

  enum Anchor {
    TopLeft      = 0x00,
    TopCenter    = 0x01,
    TopRight     = 0x02,
    MiddleRight  = 0x04,
    BottomRight  = 0x08,
    BottomCenter = 0x10,
    BottomLeft   = 0x20,
    MiddleLeft   = 0x40,
    MiddleCenter = 0x80
  } anchor;

  std::string   id        = "";
  std::string   caption   = "";
  std::string   title     = "";

  DWORD         duration  = 0;
  DWORD         displayed = 0;
  DWORD         inserted  = 0;

  struct {
    std::string glyph     = "";
    uint32_t    color     = 0xFFFFFFFFUL;
  } icon;

  enum Stage {
    FadeIn   = 0x0,
    Drawing  = 0x1,
    FadeOut  = 0x2,
    Finished = 0x4,
    Config   = 0x8
  } stage = FadeIn;

  SK_ImGui_ToastOwnerDrawn_pfn owner_draw = nullptr;
  void*                        owner_data = nullptr;
};

bool
SK_ImGui_CreateNotification ( const char* szID,
                     SK_ImGui_Toast::Type type,
                              const char* szCaption,
                              const char* szTitle,
                                    DWORD dwMilliseconds,
                                    DWORD flags = SK_ImGui_Toast::UseDuration |
                                                  SK_ImGui_Toast::ShowCaption |
                                                  SK_ImGui_Toast::ShowTitle );

bool
SK_ImGui_CreateNotificationEx ( const char* szID,
                       SK_ImGui_Toast::Type type,
                                const char* szCaption,
                                const char* szTitle,
                                      DWORD dwMilliseconds,
                                      DWORD flags = SK_ImGui_Toast::UseDuration |
                                                    SK_ImGui_Toast::ShowCaption |
                                                    SK_ImGui_Toast::ShowTitle,
                                      SK_ImGui_ToastOwnerDrawn_pfn draw_fn   = nullptr,
                                      void*                        draw_data = nullptr );

int SK_ImGui_DismissNotification (const char* szID);

void
SK_ImGui_DrawNotifications (void);

#define SK_ReleaseAssertEx(_expr,_msg,_file,_line,_func) {                \
  if (! (_expr)) [[unlikely]]                                             \
  {                                                                       \
    SK_LOG0 ( (  L"Critical Assertion Failure: '%ws' (%ws:%u) -- %hs",    \
                   (_msg), (_file), (_line), (_func)                      \
              ), L" SpecialK ");                                          \
                                                                          \
    if (sk::logs::base_log_lvl > 1 && SK_IsDebuggerPresent ())            \
    {                                                                     \
      __debugbreak ();                                                    \
    }                                                                     \
                                                                          \
    else                                                                  \
    { SK_RunOnce (                                                        \
          SK_ImGui_CreateNotification (                                   \
            "Debug.Assertion_Failure", SK_ImGui_Toast::Error,             \
            SK_FormatString ( "Critical Assertion Failure: "              \
                              "'%ws'\r\n\r\n\tFunction:\t%hs"             \
                                       "\r\n\tSource:\t(%ws:%u)",         \
                                 (_msg),  (_func),                        \
                                 (_file), (_line)                         \
                             ).c_str (), "First-Chance Assertion Failure" \
                                       , 20000,                           \
                                         SK_ImGui_Toast::UseDuration |    \
                                         SK_ImGui_Toast::ShowTitle   |    \
                                         SK_ImGui_Toast::ShowCaption |    \
                                         SK_ImGui_Toast::ShowNewest )     \
                 );          }                                      }     }

#define SK_ReleaseAssert(expr) do { SK_ReleaseAssertEx ( (expr),L#expr, \
                                                    __FILEW__,__LINE__, \
                                                    __FUNCSIG__ ) } while (0)


struct SK_ThreadSuspension_Ctx {
  std::queue <DWORD> tids;

  ULONG              ldrState  = LDR_LOCK_LOADER_LOCK_DISPOSITION_INVALID;
  ULONG_PTR          ldrCookie = 0x0;
};

SK_ThreadSuspension_Ctx
               SK_SuspendAllOtherThreads (void);
void
               SK_ResumeThreads          (SK_ThreadSuspension_Ctx threads);


bool __cdecl   SK_IsServiceHost          (void);
bool __cdecl   SK_IsRunDLLInvocation     (void);
bool __cdecl   SK_IsSuperSpecialK        (void);


// TODO: Push the SK_GetHostApp (...) stuff into this class
class SK_HostAppUtil
{
public:
  void         init                      (void);

  bool         isInjectionTool           (void)
  {
    init ();

    return SKIF || SKIM || (RunDll32 && SK_IsRunDLLInvocation ());
  }
  bool         isBlacklisted             (void)     const;
  void         setBlacklisted            (bool set);


protected:
  bool        SKIM        = false;
  bool        SKIF        = false;
  bool        RunDll32    = false;
  bool        Blacklisted = false;
};

SK_HostAppUtil*
SK_GetHostAppUtil (void);

bool __stdcall SK_IsDLLSpecialK          (const wchar_t* wszName);
void __stdcall SK_SelfDestruct           (void) noexcept;



struct sk_import_test_s {
  const char* szModuleName;
  bool        used;
};

void __stdcall SK_TestImports          (HMODULE hMod, sk_import_test_s* pTests, int nCount);

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
                       bool*   d3d12,
                       bool*   d3d8,
                       bool*   ddraw,
                       bool*   glide );

void
__stdcall
SK_wcsrep ( const wchar_t*   wszIn,
                  wchar_t** pwszOut,
            const wchar_t*   wszOld,
            const wchar_t*   wszNew );


const wchar_t*
SK_Path_wcsrchr (const wchar_t* wszStr, wchar_t wchr);

const wchar_t*
SK_Path_wcsstr (const wchar_t* wszStr, const wchar_t* wszSubStr);

int
SK_Path_wcsicmp (const wchar_t* wszStr1, const wchar_t* wszStr2);

size_t
SK_RemoveTrailingDecimalZeros (wchar_t* wszNum, size_t bufLen = 0);

size_t
SK_RemoveTrailingDecimalZeros (char* szNum, size_t bufLen = 0);


void*
__stdcall
SK_Scan         (const void* pattern, size_t len, const void* mask);

void*
__stdcall
SK_ScanAligned (const void* pattern, size_t len, const void* mask, int align = 1);

void*
__stdcall
SK_ScanAlignedEx (const void* pattern, size_t len, const void* mask, void* after = nullptr, int align = 1);

void*
__stdcall
SK_ScanAlignedExec (const void* pattern, size_t len, const void* mask, void* after = nullptr, int align = 1);

BOOL
__stdcall
SK_InjectMemory ( LPVOID  base_addr,
            const void   *new_data,
                  size_t  data_size,
                  DWORD   permissions,
                  void   *old_data     = nullptr );

bool
SK_IsProcessRunning (const wchar_t* wszProcName);

BOOL
WINAPI
SK_Module_IsProcAddrLocal ( HMODULE  hModExpected,
                             LPCSTR  lpProcName,
                            FARPROC  lpProcAddr,
          PLDR_DATA_TABLE_ENTRY__SK *ppldrEntry = nullptr );


// Call this instead of ShellExecuteW in order to handle COM init. so that all
// verbs (lpOperation) work.
HINSTANCE
WINAPI
SK_ShellExecuteW ( _In_opt_ HWND    hwnd,
                   _In_opt_ LPCWSTR lpOperation,
                   _In_     LPCWSTR lpFile,
                   _In_opt_ LPCWSTR lpParameters,
                   _In_opt_ LPCWSTR lpDirectory,
                   _In_     INT     nShowCmd );

HINSTANCE
WINAPI
SK_ShellExecuteA ( _In_opt_ HWND   hwnd,
                   _In_opt_ LPCSTR lpOperation,
                   _In_     LPCSTR lpFile,
                   _In_opt_ LPCSTR lpParameters,
                   _In_opt_ LPCSTR lpDirectory,
                   _In_     INT    nShowCmd );


constexpr size_t
hash_string (const wchar_t* const wstr, bool lowercase = false)
{
  if (wstr == nullptr)
    return 0;

  // Obviously this is completely blind to locale, but it does what
  //   is required while keeping the entire lambda constexpr-valid.
  constexpr auto constexpr_towlower = [&](const wchar_t val) ->
  wchar_t
  {
    return
      ( ( val >= L'A'  &&
          val <= L'Z' ) ? val + 32 :
                          val );

  };

  auto __h =
    size_t { 0 };

  for ( auto ptr = wstr ; *ptr != L'\0' ; ++ptr )
  {
    __h =
      ( lowercase ? constexpr_towlower (*ptr) :
                                        *ptr  )
               +
   (__h << 06) + (__h << 16) -
    __h;
  }

  return
    __h;
}

constexpr size_t
hash_string_utf8 (const char* const ustr, bool lowercase = false)
{
  if (ustr == nullptr)
    return 0;

  // Obviously this is completely blind to locale, but it does what
  //   is required while keeping the entire lambda constexpr-valid.
  constexpr auto constexpr_tolower = [&](const char val) ->
  char
  {
    return
      ( ( val >= 'A'  &&
          val <= 'Z' ) ? val + 32 :
                         val );
  };

  auto __h =
    size_t { 0 };

  for ( auto ptr = ustr ; *ptr != '\0' ; ++ptr )
  {
    __h =
      ( lowercase ? constexpr_tolower (*ptr) :
                                       *ptr  )
               +
   (__h << 06) + (__h << 16) -
    __h;
  }

  return
    __h;
}

constexpr size_t
hash_lower (const wchar_t* const wstr)
{
  return
    hash_string (wstr, true);
}

constexpr size_t
hash_lower_utf8 (const char* const ustr)
{
  return
    hash_string_utf8 (ustr, true);
}



#ifdef SK_BUILT_BY_CLANG
#if __i386__
#define __llvm_cpuid(__leaf, __eax, __ebx, __ecx, __edx) \
    __asm("cpuid" : "=a"(__eax), "=b" (__ebx), "=c"(__ecx), "=d"(__edx) \
                  : "0"(__leaf))

#define __llvm_cpuid_count(__leaf, __count, __eax, __ebx, __ecx, __edx) \
    __asm("cpuid" : "=a"(__eax), "=b" (__ebx), "=c"(__ecx), "=d"(__edx) \
                  : "0"(__leaf), "2"(__count))
#else
/* x86-64 uses %rbx as the base register, so preserve it. */
#define __llvm_cpuid(__leaf, __eax, __ebx, __ecx, __edx) \
    __asm("  xchgq  %%rbx,%q1\n" \
          "  cpuid\n" \
          "  xchgq  %%rbx,%q1" \
        : "=a"(__eax), "=r" (__ebx), "=c"(__ecx), "=d"(__edx) \
        : "0"(__leaf))

#define __llvm_cpuid_count(__leaf, __count, __eax, __ebx, __ecx, __edx) \
    __asm("  xchgq  %%rbx,%q1\n" \
          "  cpuid\n" \
          "  xchgq  %%rbx,%q1" \
        : "=a"(__eax), "=r" (__ebx), "=c"(__ecx), "=d"(__edx) \
        : "0"(__leaf), "2"(__count))
#endif
#endif



class InstructionSet
{
  // Fwd decl
  class InstructionSet_Internal;

public:
  // Accessors
  //
  static std::string Vendor (void)          { return CPU_Rep->vendor_;          }
  static std::string Brand  (void)          { return CPU_Rep->brand_;           }

  static int  Family        (void) noexcept { return CPU_Rep->family_;          }
  static int  ExtFamily     (void) noexcept { return CPU_Rep->extended_family_; }
  static int  Model         (void) noexcept { return CPU_Rep->model_;           }
  static int  ExtModel      (void) noexcept { return CPU_Rep->extended_model_;  }
  static int  Stepping      (void) noexcept { return CPU_Rep->stepping_;        }

  static bool SSE3          (void)          { return CPU_Rep->f_1_ECX_  [ 0]; }
  static bool PCLMULQDQ     (void)          { return CPU_Rep->f_1_ECX_  [ 1]; }
  static bool MONITOR       (void)          { return CPU_Rep->f_1_ECX_  [ 3]; }
  static bool SSSE3         (void)          { return CPU_Rep->f_1_ECX_  [ 9]; }
  static bool FMA           (void)          { return CPU_Rep->f_1_ECX_  [12]; }
  static bool CMPXCHG16B    (void)          { return CPU_Rep->f_1_ECX_  [13]; }
  static bool SSE41         (void)          { return CPU_Rep->f_1_ECX_  [19]; }
  static bool SSE42         (void)          { return CPU_Rep->f_1_ECX_  [20]; }
  static bool MOVBE         (void)          { return CPU_Rep->f_1_ECX_  [22]; }
  static bool POPCNT        (void)          { return CPU_Rep->f_1_ECX_  [23]; }
  static bool AES           (void)          { return CPU_Rep->f_1_ECX_  [25]; }
  static bool XSAVE         (void)          { return CPU_Rep->f_1_ECX_  [26]; }
  static bool OSXSAVE       (void)          { return CPU_Rep->f_1_ECX_  [27]; }
  static bool AVX           (void)          { return CPU_Rep->f_1_ECX_  [28]; }
  static bool F16C          (void)          { return CPU_Rep->f_1_ECX_  [29]; }
  static bool RDRAND        (void)          { return CPU_Rep->f_1_ECX_  [30]; }

  static bool MSR           (void)          { return CPU_Rep->f_1_EDX_  [ 5]; }
  static bool CX8           (void)          { return CPU_Rep->f_1_EDX_  [ 8]; }
  static bool SEP           (void)          { return CPU_Rep->f_1_EDX_  [11]; }
  static bool CMOV          (void)          { return CPU_Rep->f_1_EDX_  [15]; }
  static bool CLFSH         (void)          { return CPU_Rep->f_1_EDX_  [19]; }
  static bool MMX           (void)          { return CPU_Rep->f_1_EDX_  [23]; }
  static bool FXSR          (void)          { return CPU_Rep->f_1_EDX_  [24]; }
  static bool SSE           (void)          { return CPU_Rep->f_1_EDX_  [25]; }
  static bool SSE2          (void)          { return CPU_Rep->f_1_EDX_  [26]; }

  static bool FSGSBASE      (void)          { return CPU_Rep->f_7_EBX_  [ 0]; }
  static bool BMI1          (void)          { return CPU_Rep->f_7_EBX_  [ 3]; }
  static bool HLE           (void)          { return CPU_Rep->isIntel_  &&
                                                     CPU_Rep->f_7_EBX_  [ 4]; }
  static bool AVX2          (void)          { return CPU_Rep->f_7_EBX_  [ 5]; }
  static bool BMI2          (void)          { return CPU_Rep->f_7_EBX_  [ 8]; }
  static bool ERMS          (void)          { return CPU_Rep->f_7_EBX_  [ 9]; }
  static bool INVPCID       (void)          { return CPU_Rep->f_7_EBX_  [10]; }
  static bool RTM           (void)          { return CPU_Rep->isIntel_  &&
                                                     CPU_Rep->f_7_EBX_  [11]; }
  static bool AVX512F       (void)          { return CPU_Rep->f_7_EBX_  [16]; }
  static bool RDSEED        (void)          { return CPU_Rep->f_7_EBX_  [18]; }
  static bool ADX           (void)          { return CPU_Rep->f_7_EBX_  [19]; }
  static bool AVX512PF      (void)          { return CPU_Rep->f_7_EBX_  [26]; }
  static bool AVX512ER      (void)          { return CPU_Rep->f_7_EBX_  [27]; }
  static bool AVX512CD      (void)          { return CPU_Rep->f_7_EBX_  [28]; }
  static bool SHA           (void)          { return CPU_Rep->f_7_EBX_  [29]; }

  static bool PREFETCHWT1   (void)          { return CPU_Rep->f_7_ECX_  [ 0]; }

  static bool LAHF          (void)          { return CPU_Rep->f_81_ECX_ [ 0]; }
  static bool LZCNT         (void)          { return CPU_Rep->isIntel_ &&
                                                     CPU_Rep->f_81_ECX_ [ 5]; }
  static bool ABM           (void)          { return CPU_Rep->isAMD_   &&
                                                     CPU_Rep->f_81_ECX_ [ 5]; }
  static bool SSE4a         (void)          { return CPU_Rep->isAMD_   &&
                                                     CPU_Rep->f_81_ECX_ [ 6]; }
  static bool XOP           (void)          { return CPU_Rep->isAMD_   &&
                                                     CPU_Rep->f_81_ECX_ [11]; }
  static bool TBM           (void)          { return CPU_Rep->isAMD_   &&
                                                     CPU_Rep->f_81_ECX_ [21]; }

  static bool SYSCALL       (void)          { return CPU_Rep->isIntel_ &&
                                                     CPU_Rep->f_81_EDX_ [11]; }
  static bool MMXEXT        (void)          { return CPU_Rep->isAMD_   &&
                                                     CPU_Rep->f_81_EDX_ [22]; }
  static bool RDTSCP        (void)          { return CPU_Rep->isIntel_ &&
                                                     CPU_Rep->f_81_EDX_ [27]; }
  static bool _3DNOWEXT     (void)          { return CPU_Rep->isAMD_   &&
                                                     CPU_Rep->f_81_EDX_ [30]; }
  static bool _3DNOW        (void)          { return CPU_Rep->isAMD_   &&
                                                     CPU_Rep->f_81_EDX_ [31]; }

  static void deferredInit  (void)          { SK_RunOnce (CPU_Rep = std::make_unique <InstructionSet_Internal> ()); }

private:
  static std::unique_ptr <InstructionSet_Internal> CPU_Rep;

  class InstructionSet_Internal
  {
  public:
    InstructionSet_Internal (void) : nIds_     { 0     }, nExIds_   { 0     },
                                     vendor_   (       ), brand_    (       ),
                                     family_   { 0     }, isIntel_  { false },
                            extended_family_   { 0     }, isAMD_    { false },
                                     model_    { 0     }, stepping_ { 0     },
                            extended_model_    { 0     },
                                     f_1_ECX_  { 0     }, f_1_EDX_  { 0     },
                                     f_7_EBX_  { 0     }, f_7_ECX_  { 0     },
                                     f_81_ECX_ { 0     }, f_81_EDX_ { 0     }
    {
      //int cpuInfo[4] = {-1};
      std::array <int, 4> cpui = { -1, -1, -1, -1 };

      // Calling __cpuid with 0x0 as the function_id argument
      // gets the number of the highest valid function ID.

#ifndef SK_BUILT_BY_CLANG
      __cpuid (cpui.data (), 0);
#else
      __llvm_cpuid (0, cpui [0], cpui [1],
                       cpui [2], cpui [3]);
#endif

      nIds_ = cpui [0];

      for (int i = 0; i <= nIds_; ++i)
      {
        __cpuidex (cpui.data (), i, 0);
        data_.emplace_back (cpui);
      }

      // Capture vendor string
      //
      int vendor  [8] = { };

      if (nIds_ >= 0)
      {
        * vendor      = data_ [0][1];
        *(vendor + 1) = data_ [0][3];
        *(vendor + 2) = data_ [0][2];
      }

      vendor_ =
        (char *)vendor;

           if  (vendor_ == "GenuineIntel")  isIntel_ = true;
      else if  (vendor_ == "AuthenticAMD")  isAMD_   = true;

      if (nIds_ >= 1)
      {
        stepping_ =  data_ [1][0]       & 0xF;
        model_    = (data_ [1][0] >> 4) & 0xF;
        family_   = (data_ [1][0] >> 8) & 0xF;

        extended_model_  = (data_ [1][0] >> 16) & 0x0F;
        extended_family_ = (data_ [1][0] >> 20) & 0xFF;

        // Load Bitset with Flags for Function 0x00000001
        //
        f_1_ECX_ = data_ [1][2];
        f_1_EDX_ = data_ [1][3];
      }

      // Load Bitset with Flags for Function 0x00000007
      //
      if (nIds_ >= 7)
      {
        f_7_EBX_ = data_ [7][1];
        f_7_ECX_ = data_ [7][2];
      }

      // Calling __cpuid with 0x80000000 as the function_id argument
      // gets the number of the highest valid extended ID.
      //
#ifndef SK_BUILT_BY_CLANG
       __cpuid (cpui.data ( ), 0x80000000U);
      nExIds_ = cpui      [0];
#else
      __llvm_cpuid (0x80000000U, cpui [0], cpui [1],
                                 cpui [2], cpui [3]);
      nExIds_                  = cpui [0];
#endif

      for (int i = 0x80000000U; i <= nExIds_; ++i)
      {
        __cpuidex          (cpui.data (), i, 0);
        extdata_.push_back (cpui);
      }

      // Load Bitset with Flags for Function 0x80000001
      //
      if (nExIds_ >= 0x80000001U)
      {
        f_81_ECX_ = extdata_ [1][2];
        f_81_EDX_ = extdata_ [1][3];
      }

      // Interpret CPU Brand String if Reported
      if (nExIds_ >= 0x80000004U)
      {
        char    brand [0x40] =
        {                    };
        memcpy (brand,      extdata_ [2].data (), sizeof (cpui));
        memcpy (brand + 16, extdata_ [3].data (), sizeof (cpui));
        memcpy (brand + 32, extdata_ [4].data (), sizeof (cpui));

        brand_ = brand;
      }
    };

                             int       nIds_;
                             int       nExIds_;
                      std::string      vendor_;
                      std::string      brand_;
                     unsigned int      family_;
                     unsigned int      model_;
                     unsigned int      stepping_;
                     unsigned int      extended_model_;
                     unsigned int      extended_family_;
                             bool      isIntel_;
                             bool      isAMD_;
                      std::bitset <32> f_1_ECX_;
                      std::bitset <32> f_1_EDX_;
                      std::bitset <32> f_7_EBX_;
                      std::bitset <32> f_7_ECX_;
                      std::bitset <32> f_81_ECX_;
                      std::bitset <32> f_81_EDX_;
    std::vector < std::array <
                  int,     4 >
                >                      data_;
    std::vector < std::array <
                  int,     4 >
                >                      extdata_;
  };
};

//void  SK_AVX2_ZeroMemory (void *pvDst,                    size_t nBytes);
//void* SK_AVX2_memcpy     (void *pvDst, const void *pvSrc, size_t nBytes);

//#define SK_ZeroMemory SK_AVX2_ZeroMemory
//#define SK_memcpy     SK_AVX2_memcpy

auto constexpr CountSetBits = [](ULONG_PTR bitMask) noexcept ->
DWORD
{
  constexpr
        DWORD     LSHIFT      = sizeof (ULONG_PTR) * 8 - 1;
        DWORD     bitSetCount = 0;
        ULONG_PTR bitTest     =         ULONG_PTR (1) << LSHIFT;
        DWORD     i;

  for (i = 0; i <= LSHIFT; ++i)
  {
    bitSetCount += ((bitMask & bitTest) ? 1 : 0);
    bitTest     /= 2;
  }

  return
    bitSetCount;
};

// The underlying function (PathCombineW) has unexpected behavior if pszFile
//   begins with a slash, so it is best to call this wrapper and we will take
//     care of the leading slash.
LPWSTR SK_PathCombineW ( _Out_writes_ (MAX_PATH) LPWSTR pszDest,
                                       _In_opt_ LPCWSTR pszDir,
                                       _In_opt_ LPCWSTR pszFile );


extern
void WINAPI
SK_Sleep (DWORD dwMilliseconds) noexcept;

template < class T,
           class ...    Args > std::unique_ptr <T>
SK_make_unique_nothrow (Args && ... args) noexcept
(                                         noexcept
(  T ( std::forward   < Args >     (args)   ... ))
);

DWORD WINAPI SK_timeGetTime (void) noexcept;
BOOL  WINAPI SK_PlaySound   (_In_opt_ LPCWSTR pszSound,
                             _In_opt_ HMODULE hmod,
                             _In_     DWORD   fdwSound);

HINSTANCE
SK_Util_OpenURI (
  const std::wstring_view& path,
               DWORD       dwAction );

HINSTANCE
SK_Util_ExplorePath (
  const std::wstring_view& path);


void
SK_ImportRegistryValue ( const wchar_t *wszPath,
                         const wchar_t *wszKey,
                         const wchar_t *wszType,
                         const wchar_t *wszValue,
                         bool           admin = true );



namespace SK
{
  namespace WindowsRegistry
  {
    template <class _Tp>
    class KeyValue
    {
      struct KeyDesc {
        HKEY         hKey                 = HKEY_CURRENT_USER;
        wchar_t    wszSubKey   [MAX_PATH] =               L"";
        wchar_t    wszKeyValue [MAX_PATH] =               L"";
        DWORD        dwType               =          REG_NONE;
        DWORD        dwFlags              =        RRF_RT_ANY;
      };

    public:
      bool hasData (void)
      {
        auto _GetValue =
        [&]( _Tp*     pVal,
               DWORD* pLen = nullptr ) ->
        LSTATUS
        {
          LSTATUS lStat =
            RegGetValueW ( _desc.hKey,
                             _desc.wszSubKey,
                               _desc.wszKeyValue,
                               _desc.dwFlags,
                                 &_desc.dwType,
                                   pVal, pLen );

          return lStat;
        };

        auto _SizeofData =
        [&](void) ->
        DWORD
        {
          DWORD len = 0;

          if ( ERROR_SUCCESS ==
                 _GetValue ( nullptr, &len )
             ) return len;

          return 0;
        };

        _Tp   out      = { };
        DWORD dwOutLen =  0;

        auto type_idx =
          std::type_index (typeid (_Tp));

        if ( type_idx == std::type_index (typeid (std::wstring)) )
        {
          // Two null terminators are stored at the end of REG_SZ, so account for those
          return (_SizeofData() > 4);
        }

        if ( type_idx == std::type_index (typeid (bool)) )
        {
          _desc.dwType = REG_BINARY;
              dwOutLen = sizeof (bool);
        }

        if ( type_idx == std::type_index (typeid (int)) )
        {
          _desc.dwType = REG_DWORD;
              dwOutLen = sizeof (int);
        }

        if ( type_idx == std::type_index (typeid (float)) )
        {
          _desc.dwFlags = RRF_RT_REG_BINARY;
          _desc.dwType  = REG_BINARY;
               dwOutLen = sizeof (float);
        }

        if ( ERROR_SUCCESS !=
               _GetValue (&out, &dwOutLen) ) return false;

        return true;
      };

      std::wstring getWideString (void)
      {
        auto _GetValue =
        [&]( _Tp*     pVal,
               DWORD* pLen = nullptr ) ->
        LSTATUS
        {
          LSTATUS lStat =
            RegGetValueW ( _desc.hKey,
                             _desc.wszSubKey,
                               _desc.wszKeyValue,
                               _desc.dwFlags,
                                 &_desc.dwType,
                                   pVal, pLen );

          return lStat;
        };

        auto _SizeofData =
        [&](void) ->
        DWORD
        {
          DWORD len = 0;

          if ( ERROR_SUCCESS ==
                 _GetValue ( nullptr, &len )
             ) return len;

          return 0;
        };

        _Tp   out      = { };
        DWORD dwOutLen = _SizeofData ();
        std::wstring _out (dwOutLen, '\0');

        auto type_idx =
          std::type_index (typeid (_Tp));

        if ( type_idx == std::type_index (typeid (std::wstring)) )
        {
          _desc.dwFlags = RRF_RT_REG_SZ;
          _desc.dwType  = REG_SZ;

          if ( ERROR_SUCCESS != 
            RegGetValueW ( _desc.hKey,
                             _desc.wszSubKey,
                               _desc.wszKeyValue,
                               _desc.dwFlags,
                                 &_desc.dwType,
                                   _out.data (), &dwOutLen )
             ) return std::wstring ();

          // Strip null terminators
          _out.erase  (
            std::find ( _out.begin(),
                        _out.end  (), '\0'
                      ),_out.end  () );
        }

        return _out;
      };

      _Tp getData (void)
      {
        auto _GetValue =
        [&]( _Tp*     pVal,
               DWORD* pLen = nullptr ) ->
        LSTATUS
        {
          LSTATUS lStat =
            RegGetValueW ( _desc.hKey,
                             _desc.wszSubKey,
                               _desc.wszKeyValue,
                               _desc.dwFlags,
                                 &_desc.dwType,
                                   pVal, pLen );

          return lStat;
        };

        auto _SizeofData =
        [&](void) ->
        DWORD
        {
          DWORD len = 0;

          if ( ERROR_SUCCESS ==
                 _GetValue ( nullptr, &len )
             ) return len;

          return 0;
        };

        _Tp   out;
        DWORD dwOutLen;

        auto type_idx =
          std::type_index (typeid (_Tp));

        if ( type_idx == std::type_index (typeid (bool)) )
        {
          _desc.dwType = REG_BINARY;
              dwOutLen = sizeof (bool);
        }

        if ( type_idx == std::type_index (typeid (int)) )
        {
          _desc.dwType = REG_DWORD;
              dwOutLen = sizeof (int);
        }

        if ( type_idx == std::type_index (typeid (float)) )
        {
          _desc.dwFlags = RRF_RT_REG_BINARY;
          _desc.dwType  = REG_BINARY;
               dwOutLen = sizeof (float);
        }

        if ( ERROR_SUCCESS !=
               _GetValue (&out, &dwOutLen) ) out = _Tp ();

        return out;
      };

      _Tp putData (_Tp in)
      {
        auto _SetValue =
        [&]( _Tp*    pVal,
             LPDWORD pLen = nullptr ) ->
        LSTATUS
        {
          LSTATUS lStat         = STATUS_INVALID_DISPOSITION;
          HKEY    hKeyToSet     = 0;
          DWORD   dwDisposition = 0;
          DWORD   dwDataSize    = 0;

          lStat =
            RegCreateKeyExW (
              _desc.hKey,
                _desc.wszSubKey,
                  0x00, nullptr,
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS, nullptr,
                      &hKeyToSet, &dwDisposition );

          auto type_idx =
            std::type_index (typeid (_Tp));

          if ( type_idx == std::type_index (typeid (std::wstring)) )
          {
            std::wstring _in = std::wstringstream (in).str();

            _desc.dwType     = REG_SZ;
                  dwDataSize = (DWORD) _in.size ( ) * sizeof (wchar_t);

            lStat =
              RegSetKeyValueW ( hKeyToSet,
                                  nullptr,
                                  _desc.wszKeyValue,
                                  _desc.dwType,
                           (LPBYTE) _in.data(), dwDataSize );
            
            RegCloseKey (hKeyToSet);

            return lStat;
          }

          if ( type_idx == std::type_index (typeid (bool)) )
          {
            _desc.dwType     = REG_BINARY;
                  dwDataSize = sizeof (bool);
          }

          if ( type_idx == std::type_index (typeid (int)) )
          {
            _desc.dwType     = REG_DWORD;
                  dwDataSize = sizeof (int);
          }

          if ( type_idx == std::type_index (typeid (float)) )
          {
            _desc.dwFlags    = RRF_RT_DWORD;
            _desc.dwType     = REG_BINARY;
                  dwDataSize = sizeof (float);
          }

          lStat =
            RegSetKeyValueW ( hKeyToSet,
                                nullptr,
                                _desc.wszKeyValue,
                                _desc.dwType,
                                  pVal, dwDataSize );

          RegCloseKey (hKeyToSet);

          UNREFERENCED_PARAMETER (pLen);

          return lStat;
        };

        if ( ERROR_SUCCESS == _SetValue (&in) )
          return in;

        return _Tp ();
      };

      static KeyValue <typename _Tp>
         MakeKeyValue ( const wchar_t *wszSubKey,
                        const wchar_t *wszKeyValue,
                        HKEY           hKey    = HKEY_CURRENT_USER,
                        LPDWORD        pdwType = nullptr,
                        DWORD          dwFlags = RRF_RT_ANY )
      {
        KeyValue <_Tp> kv;

        wcsncpy_s ( kv._desc.wszSubKey, MAX_PATH,
                             wszSubKey, _TRUNCATE );

        wcsncpy_s ( kv._desc.wszKeyValue, MAX_PATH,
                             wszKeyValue, _TRUNCATE );

        kv._desc.hKey    = hKey;
        kv._desc.dwType  = ( pdwType != nullptr ) ?
                                         *pdwType : REG_NONE;
        kv._desc.dwFlags = dwFlags;

        return kv;
      }

    protected:
    private:
      KeyDesc _desc;
    };
  };

#define SK_MakeRegKeyF  SK::WindowsRegistry::KeyValue <float       >::MakeKeyValue
#define SK_MakeRegKeyB  SK::WindowsRegistry::KeyValue <bool        >::MakeKeyValue
#define SK_MakeRegKeyI  SK::WindowsRegistry::KeyValue <int         >::MakeKeyValue
#define SK_MakeRegKeyWS SK::WindowsRegistry::KeyValue <std::wstring>::MakeKeyValue
};

extern BOOL SK_IsWindows8Point1OrGreater (void);
extern BOOL SK_IsWindows10OrGreater      (void);

HRESULT
ModifyPrivilege (IN LPCTSTR szPrivilege,
                 IN BOOL     fEnable);

struct
SK_VerifyTrust_SignatureInfo
{
  std::wstring subject;
  FILETIME     valid_beginning;
  FILETIME     valid_ending;
};

SK_VerifyTrust_SignatureInfo
SK_VerifyTrust_GetCodeSignature (const wchar_t *wszExecutableFileName);

void SK_Win32_NotifyHWND   (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Avoids a call to IsWindowUnicode (...) if the Unicode status is known
void SK_Win32_NotifyHWND_W (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void SK_Win32_NotifyHWND_A (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void SK_WinSock_GoOffline (void);

std::string SK_CountToString (uint64_t count);

std::string_view
SK_FormatTemperature (double in_temp, SK_UNITS in_unit, SK_UNITS out_unit, SK_TLS* pTLS);

size_t SK_Memory_EmptyWorkingSet (void);

#endif /* __SK__UTILITY_H__ */

