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

#include <Windows.h>

#include <cstdint>
#include <string>

interface iSK_INI;

typedef void *HANDLE;

bool           SK_GetDocumentsDir      (_Out_opt_ wchar_t* buf, _Inout_ uint32_t* pdwLen);
std::wstring   SK_GetDocumentsDir      (void);
std::wstring   SK_GetRTSSInstallDir    (void);
bool           SK_CreateDirectories    (const wchar_t* wszPath);
std::wstring   SK_EvalEnvironmentVars  (const wchar_t* wszEvaluateMe);
bool           SK_GetUserProfileDir    (wchar_t* buf, uint32_t* pdwLen);
bool           SK_IsTrue               (const wchar_t* string);
bool           SK_IsAdmin              (void);
int            SK_MessageBox           (std::wstring caption,
                                        std::wstring title,
                                        uint32_t     flags);

void           SK_SetNormalFileAttribs (std::wstring file);

const wchar_t* SK_GetHostApp           (void);
iSK_INI*       SK_GetDLLConfig         (void);

#pragma intrinsic (_ReturnAddress)

HMODULE        SK_GetCallingDLL        (LPVOID pReturn = _ReturnAddress ());
std::wstring   SK_GetCallerName        (LPVOID pReturn = _ReturnAddress ());
std::wstring   SK_GetModuleName        (HMODULE hDll);

#include <queue>

std::queue <DWORD>
               SK_SuspendAllOtherThreads (void);
void
               SK_ResumeThreads          (std::queue <DWORD> threads);

bool __stdcall SK_IsDLLSpecialK        (const wchar_t* wszName);
void __stdcall SK_SelfDestruct         (void);


struct sk_import_test_s {
  const char* szModuleName;
  bool        used;
};

void __stdcall SK_TestImports          (HMODULE hMod, sk_import_test_s* pTests, int nCount);
void           SK_TestRenderImports    (HMODULE hMod, bool* gl, bool* vulkan, bool* d3d9, bool* dxgi);

void
__stdcall
SK_wcsrep ( const wchar_t*   wszIn,
                  wchar_t** pwszOut,
            const wchar_t*   wszOld,
            const wchar_t*   wszNew );

typedef void (__stdcall *SK_HashProgressCallback_pfn)(uint64_t current, uint64_t total);

uint64_t __stdcall SK_GetFileSize   ( const wchar_t* wszFile );
uint32_t __stdcall SK_GetFileCRC32  ( const wchar_t* wszFile,
                         SK_HashProgressCallback_pfn callback = nullptr );
uint32_t __stdcall SK_GetFileCRC32C ( const wchar_t* wszFile,
                         SK_HashProgressCallback_pfn callback = nullptr );


const wchar_t*
SK_Path_wcsrchr (const wchar_t* wszStr, wchar_t wchr);

const wchar_t*
SK_Path_wcsstr (const wchar_t* wszStr, const wchar_t* wszSubStr);

int
SK_Path_wcsicmp (const wchar_t* wszStr1, const wchar_t* wszStr2);


void*
__stdcall
SK_Scan (const uint8_t* pattern, size_t len, const uint8_t* mask);


class SK_AutoCriticalSection {
public:
  SK_AutoCriticalSection ( CRITICAL_SECTION* pCS,
                           bool              try_only = false )
  {
    cs_ = pCS;

    if (try_only)
      TryEnter ();
    else {
      Enter ();
    }
  }

  ~SK_AutoCriticalSection (void)
  {
    Leave ();
  }

  bool try_result (void)
  {
    return acquired_;
  }

protected:
  bool TryEnter (_Acquires_lock_(* this->cs_) void)
  {
    return (acquired_ = (TryEnterCriticalSection (cs_) != FALSE));
  }

  void Enter (_Acquires_lock_(* this->cs_) void)
  {
    EnterCriticalSection (cs_);

    acquired_ = true;
  }

  void Leave (_Releases_lock_(* this->cs_) void)
  {
    if (acquired_ != false)
      LeaveCriticalSection (cs_);

    acquired_ = false;
  }

private:
  bool              acquired_;
  CRITICAL_SECTION* cs_;
};



/*
    Computes CRC-32C (Castagnoli) checksum. Uses Intel's CRC32 instruction if it is available.
    Otherwise it uses a very fast software fallback.
*/
extern "C"
uint32_t
crc32c (
    uint32_t crc,               // Initial CRC value. Typically it's 0.
                                // You can supply non-trivial initial value here.
                                // Initial value can be used to chain CRC from multiple buffers.
    const uint8_t *input,       // Data to be put through the CRC algorithm.
    size_t length);             // Length of the data in the input buffer.


/*
	Software fallback version of CRC-32C (Castagnoli) checksum.
*/
extern "C"
uint32_t
crc32c_append_sw (uint32_t crc, const uint8_t *input, size_t length);

/*
	Hardware version of CRC-32C (Castagnoli) checksum. Will fail, if CPU does not support related instructions. Use a crc32c_append version instead of.
*/
extern "C"
uint32_t
crc32c_append_hw (uint32_t crc, const uint8_t *input, size_t length);

/*
	Checks is hardware version of CRC-32C is available.
*/
extern "C"
int
crc32c_hw_available (void);


#endif /* __SK__UTILITY_H__ */