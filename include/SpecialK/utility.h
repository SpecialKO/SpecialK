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

#ifndef __SK__UTILITY_H__
#define __SK__UTILITY_H__

struct IUnknown;
#include <Unknwnbase.h>

#include <intrin.h>
#include <Windows.h>
#include <ini.h>

#include <cstdint>
#include <queue>
#include <string>
#include <mutex>

#include <SpecialK/SpecialK.h>
#include <SpecialK/core.h>
#include <SpecialK/sha1.h>
#include <SpecialK/framerate.h>
#include <SpecialK/diagnostics/debug_utils.h>

using HANDLE = void *;

template <typename T, typename T2, typename Q>
  __inline
  T
    static_const_cast ( const typename Q q )
    {
      return static_cast <T>  (
               const_cast <T2>  ( q )
                              );
    };

template <typename T, typename Q>
  __inline
  T**
    static_cast_p2p ( typename Q** p2p ) 
    {
      return static_cast <T **> (
               static_cast <T*>   ( p2p )
                                );
    };


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

const wchar_t* __stdcall
               SK_GetRootPath               (void);
const wchar_t* SK_GetHostApp                (void);
const wchar_t* SK_GetHostPath               (void);
const wchar_t* SK_GetBlacklistFilename      (void);

bool           SK_GetDocumentsDir           (_Out_opt_ wchar_t* buf, _Inout_ uint32_t* pdwLen);
std::wstring&  SK_GetDocumentsDir           (void);
std::wstring   SK_GetFontsDir               (void);
std::wstring   SK_GetRTSSInstallDir         (void);
bool
__stdcall      SK_CreateDirectories         (const wchar_t* wszPath);
uint64_t       SK_DeleteTemporaryFiles      (const wchar_t* wszPath    = SK_GetHostPath (),
                                             const wchar_t* wszPattern = L"SKI*.tmp");
std::wstring   SK_EvalEnvironmentVars       (const wchar_t* wszEvaluateMe);
bool           SK_GetUserProfileDir         (wchar_t*       buf, uint32_t* pdwLen);
bool           SK_IsTrue                    (const wchar_t* string);
bool           SK_IsAdmin                   (void);
void           SK_ElevateToAdmin            (void); // Needs DOS 8.3 filename support
void           SK_RestartGame               (const wchar_t* wszDLL = nullptr);
int            SK_MessageBox                (std::wstring caption,
                                             std::wstring title,
                                             uint32_t     flags);

LPVOID         SK_Win32_GetTokenInfo        (_TOKEN_INFORMATION_CLASS tic);
LPVOID         SK_Win32_ReleaseTokenInfo    (LPVOID                   lpTokenBuf);

time_t         SK_Win32_FILETIME_to_time_t  (FILETIME const& ft);

std::string    SK_WideCharToUTF8            (const std::wstring& in);
std::wstring   SK_UTF8ToWideChar            (const std::string&  in);

std::string
__cdecl        SK_FormatString              (char    const* const _Format, ...);
std::wstring
__cdecl        SK_FormatStringW             (wchar_t const* const _Format, ...);

void           SK_StripTrailingSlashesW     (wchar_t *wszInOut);
void           SK_StripTrailingSlashesA     (char    *szInOut);
void           SK_FixSlashesW               (wchar_t *wszInOut);
void           SK_FixSlashesA               (char    *szInOut);

void           SK_File_SetNormalAttribs     (const wchar_t* file);
void           SK_File_MoveNoFail           (const wchar_t* wszOld,    const wchar_t* wszNew);
void           SK_File_FullCopy             (const wchar_t* from,      const wchar_t* to);
BOOL           SK_File_SetAttribs           (const wchar_t* file,      DWORD          dwAttribs);
BOOL           SK_File_ApplyAttribMask      (const wchar_t* file,      DWORD          dwAttribMask,
                                             bool           clear = false);
BOOL           SK_File_SetHidden            (const wchar_t* file,      bool           hidden);
BOOL           SK_File_SetTemporary         (const wchar_t* file,      bool           temp);
uint64_t       SK_File_GetSize              (const wchar_t* wszFile);
std::wstring   SK_File_SizeToString         (uint64_t       size,      SK_UNITS       unit = Auto);
std::wstring   SK_File_SizeToStringF        (uint64_t       size,      int            width,
                                             int            precision, SK_UNITS       unit = Auto);
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
bool           SK_ValidatePointer           (LPCVOID addr, bool silent = false);
bool           SK_IsAddressExecutable       (LPCVOID addr, bool silent = false);
void           SK_LogSymbolName             (LPCVOID addr);

char*          SK_StripUserNameFromPathA    (   char*  szInOut);
wchar_t*       SK_StripUserNameFromPathW    (wchar_t* wszInOut);

FARPROC WINAPI SK_GetProcAddress            (      HMODULE  hMod,      const char* szFunc);
FARPROC WINAPI SK_GetProcAddress            (const wchar_t* wszModule, const char* szFunc);


std::wstring
        __stdcall
               SK_GetDLLVersionStr          (const wchar_t* wszName);

const wchar_t*
        __stdcall
               SK_GetCanonicalDLLForRole    (enum DLL_ROLE role);

const wchar_t* SK_DescribeHRESULT           (HRESULT hr);

void           SK_DeferCommand              (const char* szCommand);

void           SK_GetSystemInfo             (LPSYSTEM_INFO lpSystemInfo);

PSID SK_Win32_GetTokenSid     (_TOKEN_INFORMATION_CLASS tic );
PSID SK_Win32_ReleaseTokenSid (PSID                     pSid);


#ifdef _WINGDI_
#undef _WINGDI_
#endif

#include <atlbase.h>

class SK_AutoHandle : public CHandle
{
  // Signed handles are invalid, since handles are pointers and
  //   the signed half of the address space is only for kernel

public:
   SK_AutoHandle (HANDLE hHandle) : CHandle (hHandle) { };
  ~SK_AutoHandle (void)
  {
    // We cannot close these handles because technically they
    //   were never opened (by usermode code).
    if ((intptr_t)m_h < 0)
                  m_h = 0;

    // Signed handles are often special cases
    //   such as -2 = Current Thread, -1 = Current Process
  }
};



extern void WINAPI  SK_ExitProcess      (      UINT      uExitCode  );
extern void WINAPI  SK_ExitThread       (      DWORD     dwExitCode );
extern BOOL WINAPI  SK_TerminateThread  (      HANDLE    hThread,
                                               DWORD     dwExitCode );
extern BOOL WINAPI  SK_TerminateProcess (      HANDLE    hProcess,
                                               UINT      uExitCode  );
extern void __cdecl SK__endthreadex     ( _In_ unsigned _ReturnCode );


constexpr uint8_t
__stdcall
SK_GetBitness (void)
{
#ifdef _WIN64
  return 64;
#endif
  return 32;
}

#define SK_RunOnce(x)    { static volatile LONG __skro_first = TRUE; \
               if (InterlockedCompareExchange (&__skro_first, FALSE, TRUE)) { (x); } }

#define SK_RunIf32Bit(x)         { SK_GetBitness () == 32  ? (x) :  0; }
#define SK_RunIf64Bit(x)         { SK_GetBitness () == 64  ? (x) :  0; }
#define SK_RunLHIfBitness(b,l,r)   SK_GetBitness () == (b) ? (l) : (r)


#define SK_LOG_FIRST_CALL { bool called = false;                                      \
                     SK_RunOnce (called = true);                                      \
                             if (called) {                                            \
        SK_LOG0 ( (L"[!] > First Call: %34hs", __FUNCTION__),      __SK_SUBSYSTEM__); \
        SK_LOG1 ( (L"    <*> %s", SK_SummarizeCaller ().c_str ()), __SK_SUBSYSTEM__); } }


#define SK_ReleaseAssertEx(_expr,_msg,_file,_line)            \
{                                                             \
  if (! (_expr))                                              \
  {                                                           \
    SK_LOG0 ( ( L"Critical Assertion Failure: '%ws' (%ws:%u)",\
                  (_msg), (_file), (_line)                    \
              ),L" SpecialK ");                               \
    if (SK_IsDebuggerPresent ()) __debugbreak ();             \
  }                                                           \
}
#define SK_ReleaseAssert(expr) SK_ReleaseAssertEx ( (expr),L#expr,     \
                                                    __FILEW__,__LINE__ )


std::queue <DWORD>
               SK_SuspendAllOtherThreads (void);
void
               SK_ResumeThreads          (std::queue <DWORD> threads);


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

    return SKIM || (RunDll32 && SK_IsRunDLLInvocation ());
  }


protected:
  bool        SKIM     = false;
  bool        RunDll32 = false;
};

SK_HostAppUtil&
SK_GetHostAppUtil (void);

bool __stdcall SK_IsDLLSpecialK          (const wchar_t* wszName);
void __stdcall SK_SelfDestruct           (void);



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
SK_RemoveTrailingDecimalZeros (wchar_t* wszNum, size_t bufSize = 0);

size_t
SK_RemoveTrailingDecimalZeros (char* szNum, size_t bufSize = 0);


void*
__stdcall
SK_Scan         (const void* pattern, size_t len, const void* mask);

void*
__stdcall
SK_ScanAligned (const void* pattern, size_t len, const void* mask, int align = 1);

void*
__stdcall
SK_ScanAlignedEx (const void* pattern, size_t len, const void* mask, void* after = nullptr, int align = 1);

BOOL
__stdcall
SK_InjectMemory ( LPVOID  base_addr,
            const void   *new_data,
                  size_t  data_size,
                  DWORD   permissions,
                  void   *old_data     = nullptr );

bool
SK_IsProcessRunning (const wchar_t* wszProcName);


#include <vector>
#include <bitset>
#include <array>
#include <string>
#include <intrin.h>

class InstructionSet
{
  // Fwd decl
  class InstructionSet_Internal;

public:
  // Accessors
  //
  static std::string Vendor (void) { return CPU_Rep.vendor_;        }
  static std::string Brand  (void) { return CPU_Rep.brand_;         }

  static int  Family        (void)  { return CPU_Rep.family_;        }
  static int  Model         (void)  { return CPU_Rep.model_;         }
  static int  Stepping      (void)  { return CPU_Rep.stepping_;      }

  static bool SSE3          (void)  { return CPU_Rep.f_1_ECX_  [ 0]; }
  static bool PCLMULQDQ     (void)  { return CPU_Rep.f_1_ECX_  [ 1]; }
  static bool MONITOR       (void)  { return CPU_Rep.f_1_ECX_  [ 3]; }
  static bool SSSE3         (void)  { return CPU_Rep.f_1_ECX_  [ 9]; }
  static bool FMA           (void)  { return CPU_Rep.f_1_ECX_  [12]; }
  static bool CMPXCHG16B    (void)  { return CPU_Rep.f_1_ECX_  [13]; }
  static bool SSE41         (void)  { return CPU_Rep.f_1_ECX_  [19]; }
  static bool SSE42         (void)  { return CPU_Rep.f_1_ECX_  [20]; }
  static bool MOVBE         (void)  { return CPU_Rep.f_1_ECX_  [22]; }
  static bool POPCNT        (void)  { return CPU_Rep.f_1_ECX_  [23]; }
  static bool AES           (void)  { return CPU_Rep.f_1_ECX_  [25]; }
  static bool XSAVE         (void)  { return CPU_Rep.f_1_ECX_  [26]; }
  static bool OSXSAVE       (void)  { return CPU_Rep.f_1_ECX_  [27]; }
  static bool AVX           (void)  { return CPU_Rep.f_1_ECX_  [28]; }
  static bool F16C          (void)  { return CPU_Rep.f_1_ECX_  [29]; }
  static bool RDRAND        (void)  { return CPU_Rep.f_1_ECX_  [30]; }

  static bool MSR           (void)  { return CPU_Rep.f_1_EDX_  [ 5]; }
  static bool CX8           (void)  { return CPU_Rep.f_1_EDX_  [ 8]; }
  static bool SEP           (void)  { return CPU_Rep.f_1_EDX_  [11]; }
  static bool CMOV          (void)  { return CPU_Rep.f_1_EDX_  [15]; }
  static bool CLFSH         (void)  { return CPU_Rep.f_1_EDX_  [19]; }
  static bool MMX           (void)  { return CPU_Rep.f_1_EDX_  [23]; }
  static bool FXSR          (void)  { return CPU_Rep.f_1_EDX_  [24]; }
  static bool SSE           (void)  { return CPU_Rep.f_1_EDX_  [25]; }
  static bool SSE2          (void)  { return CPU_Rep.f_1_EDX_  [26]; }

  static bool FSGSBASE      (void)  { return CPU_Rep.f_7_EBX_  [ 0]; }
  static bool BMI1          (void)  { return CPU_Rep.f_7_EBX_  [ 3]; }
  static bool HLE           (void)  { return CPU_Rep.isIntel_  && 
                                             CPU_Rep.f_7_EBX_  [ 4]; }
  static bool AVX2          (void)  { return CPU_Rep.f_7_EBX_  [ 5]; }
  static bool BMI2          (void)  { return CPU_Rep.f_7_EBX_  [ 8]; }
  static bool ERMS          (void)  { return CPU_Rep.f_7_EBX_  [ 9]; }
  static bool INVPCID       (void)  { return CPU_Rep.f_7_EBX_  [10]; }
  static bool RTM           (void)  { return CPU_Rep.isIntel_  &&
                                             CPU_Rep.f_7_EBX_  [11]; }
  static bool AVX512F       (void)  { return CPU_Rep.f_7_EBX_  [16]; }
  static bool RDSEED        (void)  { return CPU_Rep.f_7_EBX_  [18]; }
  static bool ADX           (void)  { return CPU_Rep.f_7_EBX_  [19]; }
  static bool AVX512PF      (void)  { return CPU_Rep.f_7_EBX_  [26]; }
  static bool AVX512ER      (void)  { return CPU_Rep.f_7_EBX_  [27]; }
  static bool AVX512CD      (void)  { return CPU_Rep.f_7_EBX_  [28]; }
  static bool SHA           (void)  { return CPU_Rep.f_7_EBX_  [29]; }

  static bool PREFETCHWT1   (void)  { return CPU_Rep.f_7_ECX_  [ 0]; }

  static bool LAHF          (void)  { return CPU_Rep.f_81_ECX_ [ 0]; }
  static bool LZCNT         (void)  { return CPU_Rep.isIntel_ && 
                                             CPU_Rep.f_81_ECX_ [ 5]; }
  static bool ABM           (void)  { return CPU_Rep.isAMD_   &&
                                             CPU_Rep.f_81_ECX_ [ 5]; }
  static bool SSE4a         (void)  { return CPU_Rep.isAMD_   &&
                                             CPU_Rep.f_81_ECX_ [ 6]; }
  static bool XOP           (void)  { return CPU_Rep.isAMD_   &&
                                             CPU_Rep.f_81_ECX_ [11]; }
  static bool TBM           (void)  { return CPU_Rep.isAMD_   &&
                                             CPU_Rep.f_81_ECX_ [21]; }

  static bool SYSCALL       (void)  { return CPU_Rep.isIntel_ &&
                                             CPU_Rep.f_81_EDX_ [11]; }
  static bool MMXEXT        (void)  { return CPU_Rep.isAMD_   &&
                                             CPU_Rep.f_81_EDX_ [22]; }
  static bool RDTSCP        (void)  { return CPU_Rep.isIntel_ &&
                                             CPU_Rep.f_81_EDX_ [27]; }
  static bool _3DNOWEXT     (void)  { return CPU_Rep.isAMD_   &&
                                             CPU_Rep.f_81_EDX_ [30]; }
  static bool _3DNOW        (void)  { return CPU_Rep.isAMD_   &&
                                             CPU_Rep.f_81_EDX_ [31]; }

private:
  static const InstructionSet_Internal CPU_Rep;

  class InstructionSet_Internal
  {
  public:
    InstructionSet_Internal (void)/**/: nIds_     { 0     }, nExIds_   { 0     },
                                                isIntel_  { false }, isAMD_    { false },
                                                f_1_ECX_  { 0     }, f_1_EDX_  { 0     },
                                                f_7_EBX_  { 0     }, f_7_ECX_  { 0     },
                                                f_81_ECX_ { 0     }, f_81_EDX_ { 0     },
                                                family_   { 0     }, model_    { 0     },
                                                stepping_ { 0     },
                                                data_     {       }, extdata_  {       } 
    {
      //int cpuInfo[4] = {-1};
      std::array <int, 4> cpui;

      // Calling __cpuid with 0x0 as the function_id argument
      // gets the number of the highest valid function ID.

      __cpuid (cpui.data (), 0);
       nIds_ = cpui [0];

      for (int i = 0; i <= nIds_; ++i)
      {
        __cpuidex          (cpui.data (), i, 0);
        data_.emplace_back (cpui);
      }

      // Capture vendor string
      //
      int vendor  [8] = { };
        * vendor      = data_ [0][1];
        *(vendor + 1) = data_ [0][3];
        *(vendor + 2) = data_ [0][2];

      vendor_ =
        reinterpret_cast <char *> (vendor);

           if  (vendor_ == "GenuineIntel")  isIntel_ = true;
      else if  (vendor_ == "AuthenticAMD")  isAMD_   = true;

      stepping_ =  data_ [1][0]       & 0xF;
      model_    = (data_ [1][0] >> 4) & 0xF;
      family_   = (data_ [1][0] >> 8) & 0xF;

      // Load Bitset with Flags for Function 0x00000001
      //
      if (nIds_ >= 1)
      {
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
       __cpuid (cpui.data ( ), 0x80000000);
      nExIds_ = cpui      [0];

      for (int i = 0x80000000; i <= nExIds_; ++i)
      {
        __cpuidex          (cpui.data (), i, 0);
        extdata_.push_back (cpui);
      }

      // Load Bitset with Flags for Function 0x80000001
      //
      if (nExIds_ >= 0x80000001)
      {
        f_81_ECX_ = extdata_ [1][2];
        f_81_EDX_ = extdata_ [1][3];
      }

      // Interpret CPU Brand String if Reported
      if (nExIds_ >= 0x80000004)
      {
        char    brand [0x40] =
        {                    };
        memcpy (brand,      extdata_ [2].data (), sizeof cpui);
        memcpy (brand + 16, extdata_ [3].data (), sizeof cpui);
        memcpy (brand + 32, extdata_ [4].data (), sizeof cpui);

        brand_ = brand;
      }
    };

                             int       nIds_;
                             int       nExIds_;
                      std::string      vendor_;
                      std::string      brand_;
                             int       family_;
                             int       model_;
                             int       stepping_;
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

extern void WINAPI    SK_Sleep       (DWORD dwMilliseconds);

#endif /* __SK__UTILITY_H__ */