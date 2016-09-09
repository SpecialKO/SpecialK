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

#ifndef __SK__CORE_H__
#define __SK__CORE_H__

#undef COM_NO_WINDOWS_H
#include <Windows.h>

#include "../depends/MinHook/include/MinHook.h"

#ifdef _WIN64
#pragma comment (lib, "../depends/MinHook/lib/libMinHook.x64.lib")
#else
#pragma comment (lib, "../depends/MinHook/lib/libMinHook.x86.lib")
#endif

#include "memory_monitor.h"
#include "nvapi.h"

const int MAX_GPU_NODES = 4;

struct memory_stats_t {
  uint64_t min_reserve       = UINT64_MAX;
  uint64_t max_reserve       = 0;

  uint64_t min_avail_reserve = UINT64_MAX;
  uint64_t max_avail_reserve = 0;

  uint64_t min_budget        = UINT64_MAX;
  uint64_t max_budget        = 0;

  uint64_t min_usage         = UINT64_MAX;
  uint64_t max_usage         = 0;

  uint64_t min_over_budget   = UINT64_MAX;
  uint64_t max_over_budget   = 0;

  uint64_t budget_changes    = 0;
};

enum buffer_t {
  Front = 0,
  Back  = 1,
  NumBuffers
};

#include "dxgi_interfaces.h"

#if 0
typedef  enum DXGI_MEMORY_SEGMENT_GROUP
{
  DXGI_MEMORY_SEGMENT_GROUP_LOCAL     = 0,
  DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL = 1
} DXGI_MEMORY_SEGMENT_GROUP;

typedef struct DXGI_QUERY_VIDEO_MEMORY_INFO
{
  UINT64 Budget;
  UINT64 CurrentUsage;
  UINT64 AvailableForReservation;
  UINT64 CurrentReservation;
} DXGI_QUERY_VIDEO_MEMORY_INFO;
#endif

struct mem_info_t {
  DXGI_QUERY_VIDEO_MEMORY_INFO local    [MAX_GPU_NODES];
  DXGI_QUERY_VIDEO_MEMORY_INFO nonlocal [MAX_GPU_NODES];
  SYSTEMTIME                   time;
  buffer_t                     buffer = Front;
  int                          nodes  = 0;//MAX_GPU_NODES;
};

extern memory_stats_t mem_stats [MAX_GPU_NODES];
extern mem_info_t     mem_info  [NumBuffers];

extern HANDLE           dll_heap;
extern HMODULE          backend_dll;
extern CRITICAL_SECTION budget_mutex;
extern CRITICAL_SECTION init_mutex;

// Disable SLI memory in Batman Arkham Knight
extern bool USE_SLI;

extern NV_GET_CURRENT_SLI_STATE sli_state;
extern BOOL                     nvapi_init;
extern uint32_t                 frames_drawn;

extern "C" {
  // We have some really sneaky overlays that manage to call some of our
  //   exported functions before the DLL's even attached -- make them wait,
  //     so we don't crash and burn!
  void WaitForInit (void);

  void __stdcall SK_InitCore     (const wchar_t* backend, void* callback);
  bool __stdcall SK_StartupCore  (const wchar_t* backend, void* callback);
  bool WINAPI    SK_ShutdownCore (const wchar_t* backend);

  MH_STATUS WINAPI
      SK_CreateFuncHook ( LPCWSTR pwszFuncName,
                          LPVOID  pTarget,
                          LPVOID  pDetour,
                          LPVOID *ppOriginal );

  MH_STATUS WINAPI
      SK_CreateDLLHook ( LPCWSTR pwszModule, LPCSTR  pszProcName,
                         LPVOID  pDetour,    LPVOID *ppOriginal,
                         LPVOID *ppFuncAddr = nullptr );

  MH_STATUS WINAPI
    SK_CreateVFTableHook ( LPCWSTR pwszFuncName,
                           LPVOID *ppVFTable,
                           DWORD   dwOffset,
                           LPVOID  pDetour,
                           LPVOID *ppOriginal );


  MH_STATUS WINAPI
       SK_EnableHook   (LPVOID pTarget);

  MH_STATUS WINAPI
       SK_DisableHook  (LPVOID pTarget);

  MH_STATUS WINAPI
       SK_RemoveHook   (LPVOID pTarget);

  struct IDXGISwapChain;

  void    STDMETHODCALLTYPE SK_BeginBufferSwap (void);
  HRESULT STDMETHODCALLTYPE SK_EndBufferSwap   (HRESULT hr, IUnknown* device = nullptr);

  struct IDXGIAdapter;
  void           __stdcall SK_StartDXGI_1_4_BudgetThread (IDXGIAdapter** ppAdapter);

  const wchar_t* __stdcall SK_DescribeHRESULT (HRESULT result);
}

void
__stdcall
SK_SetConfigPath (std::wstring path);

std::wstring
__stdcall
SK_GetConfigPath (void);

enum DLL_ROLE {
  // Graphics APIs
  DXGI       = 0x01, // D3D 10-12
  D3D9       = 0x02,
  OpenGL     = 0x04,
  Vulkan     = 0x08,

  // Other DLLs
  PlugIn     = 0x00010000, // Stuff like Tales of Zestiria "Fix"
  ThirdParty = 0x80000000
};

extern DLL_ROLE dll_role;

#endif /* __SK__CORE_H__ */