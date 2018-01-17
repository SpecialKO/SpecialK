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

#include <intrin.h>
#include <Windows.h>

#include <cstdint>
#include <queue>
#include <string>
#include <mutex>

#include <SpecialK/SpecialK.h>
#include <SpecialK/sha1.h>

interface iSK_INI;

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


const wchar_t* __stdcall
               SK_GetRootPath               (void);
const wchar_t* SK_GetHostApp                (void);
const wchar_t* SK_GetHostPath               (void);
const wchar_t* SK_GetBlacklistFilename      (void);

bool           SK_GetDocumentsDir           (_Out_opt_ wchar_t* buf, _Inout_ uint32_t* pdwLen);
std::wstring   SK_GetDocumentsDir           (void);
std::wstring   SK_GetFontsDir               (void);
std::wstring   SK_GetRTSSInstallDir         (void);
bool
__stdcall      SK_CreateDirectories         (const wchar_t* wszPath);
size_t         SK_DeleteTemporaryFiles      (const wchar_t* wszPath    = SK_GetHostPath (),
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

std::string    SK_WideCharToUTF8            (std::wstring in);
std::wstring   SK_UTF8ToWideChar            (std::string  in);

std::string
__cdecl        SK_FormatString              (char    const* const _Format, ...);
std::wstring
__cdecl        SK_FormatStringW             (wchar_t const* const _Format, ...);

void           SK_StripTrailingSlashesW     (wchar_t* wszInOut);
void           SK_FixSlashesW               (wchar_t* wszInOut);
void           SK_StripTrailingSlashesA     (char*     szInOut);
void           SK_FixSlashesA               (char*     szInOut);

void           SK_File_SetNormalAttribs     (std::wstring   file);
void           SK_File_MoveNoFail           (const wchar_t* wszOld,    const wchar_t* wszNew);
void           SK_File_FullCopy                 (std::wstring   from,      std::wstring   to);
BOOL           SK_File_SetAttribs           (std::wstring   file,      DWORD          dwAttribs);
BOOL           SK_File_ApplyAttribMask      (std::wstring   file,      DWORD          dwAttribMask,
                                             bool           clear = false);
BOOL           SK_File_SetHidden            (std::wstring   file,      bool           hidden);
BOOL           SK_File_SetTemporary         (std::wstring   file,      bool           temp);
uint64_t       SK_File_GetSize              (const wchar_t* wszFile);
std::wstring   SK_File_SizeToString         (uint64_t       size,      SK_UNITS       unit = Auto);
std::wstring   SK_File_SizeToStringF        (uint64_t       size,      int            width,
                                             int            precision, SK_UNITS       unit = Auto);
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
bool           SK_ValidatePointer           (LPCVOID addr);

bool           SK_StripUserNameFromPathA    (   char*  szInOut);
bool           SK_StripUserNameFromPathW    (wchar_t* wszInOut);

LPVOID         SK_GetProcAddress            (const wchar_t* wszModule, const char* szFunc);


std::wstring
        __stdcall
               SK_GetDLLVersionStr          (const wchar_t* wszName);

const wchar_t*
        __stdcall
               SK_GetCanonicalDLLForRole    (enum DLL_ROLE role);

const wchar_t*
SK_DescribeHRESULT (HRESULT hr);



constexpr uint8_t
__stdcall
SK_GetBitness (void)
{
#ifdef _WIN64
  return 64;
#endif
  return 32;
}

#define SK_RunOnce(x)    { static volatile LONG first = TRUE; if (InterlockedCompareExchange (&first, FALSE, TRUE)) { (x); first = false; } }

#define SK_RunIf32Bit(x)         { SK_GetBitness () == 32  ? (x) :  0; }
#define SK_RunIf64Bit(x)         { SK_GetBitness () == 64  ? (x) :  0; }
#define SK_RunLHIfBitness(b,l,r)   SK_GetBitness () == (b) ? (l) : (r)




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
               SK_HostAppUtil            (void);

  bool         isInjectionTool           (void)
  {
    return SKIM || (RunDll32 && SK_IsRunDLLInvocation ());
  }


protected:
  bool        SKIM     = false;
  bool        RunDll32 = false;
} extern SK_HostApp;

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
                  void   *new_data,
                  size_t  data_size,
                  DWORD   permissions,
                  void   *old_data     = nullptr );

bool
SK_IsProcessRunning (const wchar_t* wszProcName);

#endif /* __SK__UTILITY_H__ */