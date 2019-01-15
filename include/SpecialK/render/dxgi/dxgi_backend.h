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

#ifndef __SK__DXGI_BACKEND_H__
#define __SK__DXGI_BACKEND_H__

#define __D3DX11TEX_H__

struct IUnknown;
#include <Unknwnbase.h>

#include <SpecialK/render/dxgi/dxgi_interfaces.h>
#include <SpecialK/utility.h>
#include <SpecialK/thread.h>
#include <SpecialK/tls.h>
#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>

#include <string>
#include <typeindex>
#include <d3d11.h>
#include <d3d11_1.h>

static const GUID IID_ID3D11Device2 = { 0x9d06dffa, 0xd1e5, 0x4d07, { 0x83, 0xa8, 0x1b, 0xb1, 0x23, 0xf2, 0xf8, 0x41 } };
static const GUID IID_ID3D11Device3 = { 0xa05c8c37, 0xd2c6, 0x4732, { 0xb3, 0xa0, 0x9c, 0xe0, 0xb0, 0xdc, 0x9a, 0xe6 } };
static const GUID IID_ID3D11Device4 = { 0x8992ab71, 0x02e6, 0x4b8d, { 0xba, 0x48, 0xb0, 0x56, 0xdc, 0xda, 0x42, 0xc4 } };
static const GUID IID_ID3D11Device5 = { 0x8ffde202, 0xa0e7, 0x45df, { 0x9e, 0x01, 0xe8, 0x37, 0x80, 0x1b, 0x5e, 0xa0 } };

#define DXGI_VIRTUAL_HOOK_IMM(_Base,_Index,_Name,_Override,_Original,_Type) { \
  void** _vftable = *(void***)*(_Base);                                       \
                                                                              \
  if ((_Original) == nullptr) {                                               \
    SK_CreateVFTableHook ( L##_Name,                                          \
                             _vftable,                                        \
                               (_Index),                                      \
                                 (_Override),                                 \
                                   (LPVOID *)&(_Original));                   \
  }                                                                           \
}

#define DXGI_VIRTUAL_HOOK(_Base,_Index,_Name,_Override,_Original,_Type) {     \
  void** _vftable = *(void***)*(_Base);                                       \
                                                                              \
  if ((_Original) == nullptr) {                                               \
    SK_CreateVFTableHook2 ( L##_Name,                                         \
                              _vftable,                                       \
                                (_Index),                                     \
                                  (_Override),                                \
                                    (LPVOID *)&(_Original));                  \
  }                                                                           \
}


#define DXGI_CALL(_Ret, _Call) {                                            \
  (_Ret) = (_Call);                                                         \
  dll_log.Log ( L"[   DXGI   ] [@]  Return: %s  -  < " __FUNCTIONW__ L" >", \
                SK_DescribeHRESULT (_Ret) );                                \
}

  // Interface-based DXGI call
#define DXGI_LOG_CALL_I(_Interface,_Name,_Format)                            \
  {                                                                          \
    wchar_t* pwszBuffer        = new wchar_t [4096] { };                     \
    wchar_t*  wszPreFormatted  = pwszBuffer;                                 \
    wchar_t*  wszPostFormatted = pwszBuffer + 1024;                          \
    if (pwszBuffer != nullptr)                                               \
    {                                                                        \
      _swprintf ( wszPreFormatted,  L"%s::%s (", _Interface, _Name );        \
      _swprintf (wszPostFormatted, _Format

  // Global DXGI call
#define DXGI_LOG_CALL(_Name,_Format)                                         \
  {                                                                          \
    wchar_t* pwszBuffer        = new wchar_t [4096] { };                     \
    wchar_t*  wszPreFormatted  = pwszBuffer;                                 \
    wchar_t*  wszPostFormatted = pwszBuffer + 1024;                          \
    if (pwszBuffer != nullptr)                                               \
    {                                                                        \
      _swprintf (wszPreFormatted,  L"%s (", _Name);                          \
      _swprintf (wszPostFormatted, _Format

#define DXGI_LOG_CALL_END                                                    \
      wchar_t* wszFullyFormatted = wszPostFormatted + 1024;                  \
      _swprintf   ( wszFullyFormatted, L"%s%s)",                             \
                      wszPreFormatted, wszPostFormatted );                   \
      dll_log.Log ( L"[   DXGI   ] [!] %-102s -- %s", wszFullyFormatted,     \
                      SK_SummarizeCaller ().c_str () );                      \
      delete [] pwszBuffer;                                                  \
    }                                                                        \
  }

#define DXGI_LOG_CALL_I0(_Interface,_Name) {                                 \
  DXGI_LOG_CALL_I   (_Interface,_Name, L"void"));                            \
  DXGI_LOG_CALL_END                                                          \
}

#define DXGI_LOG_CALL_I1(_Interface,_Name,_Format,_Args) {                   \
  DXGI_LOG_CALL_I   (_Interface,_Name, _Format), _Args);                     \
  DXGI_LOG_CALL_END                                                          \
}

#define DXGI_LOG_CALL_I2(_Interface,_Name,_Format,_Args0,_Args1) {           \
  DXGI_LOG_CALL_I   (_Interface,_Name, _Format), _Args0, _Args1);            \
  DXGI_LOG_CALL_END                                                          \
}

#define DXGI_LOG_CALL_I3(_Interface,_Name,_Format,_Args0,_Args1,_Args2) {    \
  DXGI_LOG_CALL_I   (_Interface,_Name, _Format), _Args0, _Args1, _Args2);    \
  DXGI_LOG_CALL_END                                                          \
}

#define DXGI_LOG_CALL_I4(_Interface,_Name,_Format,_Args0,_Args1,_Args2,      \
                         _Args3) {                                           \
  DXGI_LOG_CALL_I   (_Interface,_Name, _Format), _Args0, _Args1, _Args2,     \
                                                 _Args3);                    \
  DXGI_LOG_CALL_END                                                          \
}

#define DXGI_LOG_CALL_I5(_Interface,_Name,_Format,_Args0,_Args1,_Args2,      \
                         _Args3,_Args4) {                                    \
  DXGI_LOG_CALL_I   (_Interface,_Name, _Format), _Args0, _Args1, _Args2,     \
                                                 _Args3, _Args4);            \
  DXGI_LOG_CALL_END                                                          \
}
#define DXGI_LOG_CALL_I6(_Interface,_Name,_Format,_Args0,_Args1,_Args2,      \
                         _Args3,_Args4,_Args5) {                             \
  DXGI_LOG_CALL_I   (_Interface,_Name, _Format), _Args0, _Args1, _Args2,     \
                                                 _Args3, _Args4, _Args5);    \
  DXGI_LOG_CALL_END                                                          \
}


#define DXGI_LOG_CALL_0(_Name) {                               \
  DXGI_LOG_CALL   (_Name, L"void"));                           \
  DXGI_LOG_CALL_END                                            \
}

#define DXGI_LOG_CALL_1(_Name,_Format,_Args0) {                \
  DXGI_LOG_CALL   (_Name, _Format), _Args0);                   \
  DXGI_LOG_CALL_END                                            \
}

#define DXGI_LOG_CALL_2(_Name,_Format,_Args0,_Args1) {         \
  DXGI_LOG_CALL     (_Name, _Format), _Args0, _Args1);         \
  DXGI_LOG_CALL_END                                            \
}

#define DXGI_LOG_CALL_3(_Name,_Format,_Args0,_Args1,_Args2) {  \
  DXGI_LOG_CALL     (_Name, _Format), _Args0, _Args1, _Args2); \
  DXGI_LOG_CALL_END                                            \
}


HRESULT
STDMETHODCALLTYPE
CreateDXGIFactory  (       REFIID   riid,
                     _Out_ void   **ppFactory );
HRESULT
STDMETHODCALLTYPE
CreateDXGIFactory1 (       REFIID   riid,
                     _Out_ void   **ppFactory );
HRESULT
STDMETHODCALLTYPE
CreateDXGIFactory2 (       UINT     Flags,
                           REFIID   riid,
                     _Out_ void   **ppFactory );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
DXGIFactory_CreateSwapChain_Override (             IDXGIFactory          *This,
                                       _In_        IUnknown              *pDevice,
                                       _In_  const DXGI_SWAP_CHAIN_DESC  *pDesc,
                                       _Out_       IDXGISwapChain       **ppSwapChain );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
DXGISwap_GetFullscreenState_Override ( IDXGISwapChain  *This,
                            _Out_opt_  BOOL            *pFullscreen,
                            _Out_opt_  IDXGIOutput    **ppTarget );
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
DXGISwap_SetFullscreenState_Override ( IDXGISwapChain *This,
                                       BOOL            Fullscreen,
                                       IDXGIOutput    *pTarget );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
DXGISwap_ResizeTarget_Override ( IDXGISwapChain *This,
                      _In_ const DXGI_MODE_DESC *pNewTargetParameters );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
DXGISwap_ResizeBuffers_Override ( IDXGISwapChain *This,
                             _In_ UINT            BufferCount,
                             _In_ UINT            Width,
                             _In_ UINT            Height,
                             _In_ DXGI_FORMAT     NewFormat,
                             _In_ UINT            SwapChainFlags );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE PresentCallback ( IDXGISwapChain *This,
                                    UINT            SyncInterval,
                                    UINT            Flags );


#if 1
extern SK_Thread_HybridSpinlock* budget_mutex;
#else
extern CRITICAL_SECTION budget_mutex;
#endif
       const int        MAX_GPU_NODES = 4;

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
} extern mem_stats [MAX_GPU_NODES];

enum buffer_t {
  Front = 0,
  Back  = 1,
  NumBuffers
};

struct mem_info_t {
  DXGI_QUERY_VIDEO_MEMORY_INFO local    [MAX_GPU_NODES] = { };
  DXGI_QUERY_VIDEO_MEMORY_INFO nonlocal [MAX_GPU_NODES] = { };
  SYSTEMTIME                   time                     = { };
  buffer_t                     buffer                   = Front;
  int                          nodes                    = 0;//MAX_GPU_NODES;
} extern mem_info [NumBuffers];

static const int SK_D3D11_MAX_DEV_CONTEXTS = 64;

LONG
SK_D3D11_GetDeviceContextHandle (ID3D11DeviceContext *pCtx);

namespace SK
{
  namespace DXGI
  {
    bool Startup  (void);
    bool Shutdown (void);

    std::wstring getPipelineStatsDesc (void);

    //extern HMODULE hModD3D10;
    extern HMODULE hModD3D11;
    extern HMODULE hModD3D12;

    HRESULT StartBudgetThread           (IDXGIAdapter** ppAdapter);
    HRESULT StartBudgetThread_NoAdapter (void);

    void    ShutdownBudgetThread        (void);

    DWORD
    WINAPI
    BudgetThread (LPVOID user_data);
  }
}

typedef HRESULT (STDMETHODCALLTYPE *CreateDXGIFactory2_pfn) \
  (UINT Flags, REFIID riid,  void** ppFactory);
typedef HRESULT (STDMETHODCALLTYPE *CreateDXGIFactory1_pfn) \
  (REFIID riid,  void** ppFactory);
typedef HRESULT (STDMETHODCALLTYPE *CreateDXGIFactory_pfn)  \
  (REFIID riid,  void** ppFactory);

extern CreateDXGIFactory_pfn  CreateDXGIFactory_Import;
extern CreateDXGIFactory1_pfn CreateDXGIFactory1_Import;
extern CreateDXGIFactory2_pfn CreateDXGIFactory2_Import;

extern DWORD dwRenderThread;

std::wstring
__stdcall
SK_DXGI_FormatToStr (DXGI_FORMAT fmt);

const wchar_t*
SK_DXGI_DescribeScalingMode (DXGI_MODE_SCALING mode);

const wchar_t*
SK_DXGI_DescribeScanlineOrder (DXGI_MODE_SCANLINE_ORDER order);

const wchar_t*
SK_DXGI_DescribeSwapEffect (DXGI_SWAP_EFFECT swap_effect);

std::wstring
SK_DXGI_DescribeSwapChainFlags (DXGI_SWAP_CHAIN_FLAG swap_flags);

std::wstring
SK_DXGI_FeatureLevelsToStr (       int    FeatureLevels,
                             const DWORD* pFeatureLevels );

void
WINAPI
SK_DXGI_AdapterOverride ( IDXGIAdapter**   ppAdapter,
                          D3D_DRIVER_TYPE* DriverType );

int
SK_GetDXGIFactoryInterfaceVer (const IID& riid);

std::wstring
SK_GetDXGIFactoryInterfaceEx (const IID& riid);

int
SK_GetDXGIFactoryInterfaceVer (IUnknown *pFactory);

std::wstring
SK_GetDXGIFactoryInterface (IUnknown *pFactory);

int
SK_GetDXGIAdapterInterfaceVer (const IID& riid);

std::wstring
SK_GetDXGIAdapterInterfaceEx (const IID& riid);

int
SK_GetDXGIAdapterInterfaceVer (IUnknown *pAdapter);

std::wstring
SK_GetDXGIAdapterInterface (IUnknown *pAdapter);


typedef HRESULT (WINAPI *D3D11CreateDevice_pfn)(
  _In_opt_                            IDXGIAdapter         *pAdapter,
                                      D3D_DRIVER_TYPE       DriverType,
                                      HMODULE               Software,
                                      UINT                  Flags,
  _In_opt_                      const D3D_FEATURE_LEVEL    *pFeatureLevels,
                                      UINT                  FeatureLevels,
                                      UINT                  SDKVersion,
  _Out_opt_                           ID3D11Device        **ppDevice,
  _Out_opt_                           D3D_FEATURE_LEVEL    *pFeatureLevel,
  _Out_opt_                           ID3D11DeviceContext **ppImmediateContext);

typedef HRESULT (WINAPI *D3D11CreateDeviceAndSwapChain_pfn)(
  _In_opt_                             IDXGIAdapter*,
                                       D3D_DRIVER_TYPE,
                                       HMODULE,
                                       UINT,
  _In_reads_opt_ (FeatureLevels) CONST D3D_FEATURE_LEVEL*,
                                       UINT FeatureLevels,
                                       UINT,
  _In_opt_                       CONST DXGI_SWAP_CHAIN_DESC*,
  _Out_opt_                            IDXGISwapChain**,
  _Out_opt_                            ID3D11Device**,
  _Out_opt_                            D3D_FEATURE_LEVEL*,
  _Out_opt_                            ID3D11DeviceContext**);

typedef HRESULT (WINAPI *D3D12CreateDevice_pfn)(
  _In_opt_  IUnknown          *pAdapter,
            D3D_FEATURE_LEVEL  MinimumFeatureLevel,
  _In_      REFIID             riid,
  _Out_opt_ void             **ppDevice);

extern D3D11CreateDevice_pfn             D3D11CreateDevice_Import;
extern D3D11CreateDeviceAndSwapChain_pfn D3D11CreateDeviceAndSwapChain_Import;
extern D3D12CreateDevice_pfn             D3D12CreateDevice_Import;

  extern          ID3D11Device*         g_pD3D11Dev;
  extern          IUnknown*             g_pD3D12Dev;

extern void SK_DXGI_QuickHook     (void);
extern bool SK_D3D11_Init         (void);
extern void SK_D3D11_InitTextures (void);
extern void SK_D3D11_Shutdown     (void);
extern void SK_D3D11_EnableHooks  (void);
extern void SK_D3D11_QuickHook    (void);

extern bool SK_D3D12_Init         (void);
extern void SK_D3D12_Shutdown     (void);
extern void SK_D3D12_EnableHooks  (void);

void SK_DXGI_BorderCompensation (UINT& x, UINT& y);



#include <unordered_set>
#include <unordered_map>
#include <array>
#include <set>
#include <map>

// Actually more of a cache manager at the moment...
class SK_D3D11_TexMgr {
public:
  SK_D3D11_TexMgr (void) {
    QueryPerformanceFrequency (&PerfFreq);
    HashMap_2D.resize   (20);
    Blacklist_2D.resize (20);

    //TexRefs_2D.reserve       (8192);
    //Textures_2D.reserve      (8192);
    HashMap_2D [ 1].reserve  ( 256); // Only      1x1
    HashMap_2D [ 2].reserve  ( 512); // Up to     2x2
    HashMap_2D [ 3].reserve  ( 512); // Up to     4x4
    HashMap_2D [ 4].reserve  ( 512); // Up to     8x8
    HashMap_2D [ 5].reserve  (1024); // Up to    16x16
    HashMap_2D [ 6].reserve  ( 512); // Up to    32x32
    HashMap_2D [ 7].reserve  ( 256); // Up to    64x64
    HashMap_2D [ 8].reserve  ( 256); // Up to   128x128
    HashMap_2D [ 9].reserve  (1280); // Up to   256x256
    HashMap_2D [10].reserve  (2048); // Up to   512x512
    HashMap_2D [11].reserve  (4096); // Up to  1024x1024
    HashMap_2D [12].reserve  (4096); // Up to  2048x2048
    HashMap_2D [13].reserve  (1024); // Up to  4096x4096
    HashMap_2D [14].reserve  (  16); // Up to  8192x8192
    HashMap_2D [15].reserve  (   8); // Up to 16384x16384
    HashMap_2D [16].reserve  (   4); // Up to 32768x32768
    HashMap_2D [17].reserve  (   2); // Up to 65536x65536

    AggregateSize_2D  = 0ULL;
    RedundantData_2D  = 0ULL;
    RedundantLoads_2D = 0UL;
    Entries_2D        = 0UL;
    CacheMisses_2D    = 0UL;
    Evicted_2D        = 0UL;
    Budget            = 0ULL;
  }

  bool             isTexture2D  (uint32_t crc32, const D3D11_TEXTURE2D_DESC *pDesc);

  ID3D11Texture2D* getTexture2D ( uint32_t              crc32,
                            const D3D11_TEXTURE2D_DESC *pDesc,
                                  size_t               *pMemSize   = nullptr,
                                  float                *pTimeSaved = nullptr,
                                  SK_TLS               *pTLS       = SK_TLS_Bottom () );

  void             refTexture2D ( ID3D11Texture2D      *pTex,
                            const D3D11_TEXTURE2D_DESC *pDesc,
                                  uint32_t              tag,
                                  size_t                mem_size,
                                  uint64_t              load_time,
                                  uint32_t              crc32c,
                            const wchar_t              *fileName   = L"",
                            const D3D11_TEXTURE2D_DESC *pOrigDesc  = nullptr,
                         _In_opt_ HMODULE               hModCaller = (HMODULE)(intptr_t)-1,
                         _In_opt_ SK_TLS               *pTLS       = SK_TLS_Bottom () );

  // Some texture upload paths (i.e. CopyResource or UpdateSubresoure)
  //   result in cache hits where no new object is created; call this to
  //     indicate a cache hit, but leave the reference count alone.
  LONG            recordCacheHit ( ID3D11Texture2D      *pTex );

  void           reset           (void);
  bool           purgeTextures   (size_t size_to_free, int* pCount, size_t* pFreed);

  struct tex2D_descriptor_s {
    volatile LONG         hits       = 0L;
    ID3D11Texture2D      *texture    = nullptr;
    D3D11_TEXTURE2D_DESC  desc       = { };
    D3D11_TEXTURE2D_DESC  orig_desc  = { };
    size_t                mem_size   = 0L;
    uint64_t              load_time  = 0ULL;
    uint32_t              tag        = 0x00; // Combined data and descriptor hash for collision mitigation
    uint32_t              crc32c     = 0x00;
        bool              injected   = false;
        bool              discard    = false;
    uint32_t              last_frame = 0UL;
    uint64_t              last_used  = 0ULL;
    std::wstring          file_name  = L"";  // If injected, this is the source file
  };

  concurrency::concurrent_unordered_set <ID3D11Texture2D *> TexRefs_2D;

  struct lod_hash_table_s
  {
    lod_hash_table_s (void) {
      InitializeCriticalSectionAndSpinCount (&mutex, 120);
    }

    ~lod_hash_table_s (void)
    {
      DeleteCriticalSection (&mutex);
    }

    void              reserve     (size_t   resrv ) { SK_AutoCriticalSection cs (&mutex); InterlockedIncrement (&contention_score.reserve);         entries.reserve (resrv ); };
    bool              contains    (uint32_t crc32c) { SK_AutoCriticalSection cs (&mutex); InterlockedIncrement (&contention_score.contains); return entries.count   (crc32c); };
    void              erase       (uint32_t crc32c) { SK_AutoCriticalSection cs (&mutex); InterlockedIncrement (&contention_score.erase);           entries.erase   (crc32c); };
    ID3D11Texture2D*& operator [] (uint32_t crc32c) { SK_AutoCriticalSection cs (&mutex); InterlockedIncrement (&contention_score.index);    return entries         [crc32c]; };

    std::unordered_map < uint32_t,
                         ID3D11Texture2D * > entries;
    CRITICAL_SECTION                         mutex;

    struct {
      volatile LONG reserve  = 0L;
      volatile LONG contains = 0L;
      volatile LONG erase    = 0L;
      volatile LONG index    = 0L;
    } contention_score;
  };

  std::vector        < lod_hash_table_s   >   HashMap_2D;
  std::vector        < std::unordered_set <
                        uint32_t          >
                     >                        Blacklist_2D;

#if 1
  concurrency::concurrent_unordered_map < ID3D11Texture2D *,
                                          tex2D_descriptor_s  >  Textures_2D;
#else
  std::unordered_map <ID3D11Texture2D *, tex2D_descriptor_s> Textures_2D;
#endif

  std::atomic_uint64_t                        AggregateSize_2D  = 0ULL;
  std::atomic_uint64_t                        RedundantData_2D  = 0ULL;
  std::atomic_uint32_t                        RedundantLoads_2D = 0UL;
  std::atomic_uint32_t                        Entries_2D        = 0UL;
  std::atomic_uint32_t                        CacheMisses_2D    = 0UL;
  std::atomic_uint32_t                        Evicted_2D        = 0UL;
  std::atomic_uint64_t                        Budget            = 0ULL;
  float                                       RedundantTime_2D  = 0.0f;

  std::atomic_int64_t                         LastModified_2D   = 0ULL;
  std::atomic_int64_t                         LastPurge_2D      = 0ULL;

  LARGE_INTEGER                               PerfFreq;

  std::unordered_map <uint32_t, std::wstring> tex_hashes;
  std::unordered_map <uint32_t, std::wstring> tex_hashes_ex;
  
  std::unordered_set <uint32_t>               dumped_textures;
  uint64_t                                    dumped_texture_bytes;
  std::unordered_set <uint32_t>               dumped_collisions;
  std::unordered_set <uint32_t>               injectable_textures;
  uint64_t                                    injectable_texture_bytes;
  std::unordered_set <uint32_t>               injected_collisions;
  
  std::unordered_set <uint32_t>               injectable_ffx; // HACK FOR FFX
};

using  SK_D3D11_TexMgr_Singleton = SK_D3D11_TexMgr&;
extern SK_D3D11_TexMgr_Singleton __SK_Singleton_D3D11_Textures (void);

#define SK_D3D11_Textures __SK_Singleton_D3D11_Textures()


interface ID3D11DeviceContext2;
interface ID3D11DeviceContext3;

typedef HRESULT (STDMETHODCALLTYPE *D3D11Dev_CreateDeferredContext_pfn)( 
  _In_            ID3D11Device         *This,
  _In_            UINT                  ContextFlags,
  _Out_opt_       ID3D11DeviceContext **ppDeferredContext
);
typedef HRESULT (STDMETHODCALLTYPE *D3D11Dev_CreateDeferredContext1_pfn)( 
  _In_            ID3D11Device          *This,
  _In_            UINT                   ContextFlags,
  _Out_opt_       ID3D11DeviceContext1 **ppDeferredContext
);
typedef HRESULT (STDMETHODCALLTYPE *D3D11Dev_CreateDeferredContext2_pfn)( 
  _In_            ID3D11Device          *This,
  _In_            UINT                   ContextFlags,
  _Out_opt_       ID3D11DeviceContext2 **ppDeferredContext
);
typedef HRESULT (STDMETHODCALLTYPE *D3D11Dev_CreateDeferredContext3_pfn)( 
  _In_            ID3D11Device          *This,
  _In_            UINT                   ContextFlags,
  _Out_opt_       ID3D11DeviceContext3 **ppDeferredContext
);
typedef void (STDMETHODCALLTYPE *D3D11Dev_GetImmediateContext_pfn)( 
  _In_            ID3D11Device         *This,
  _Out_           ID3D11DeviceContext **ppImmediateContext
);
typedef void (STDMETHODCALLTYPE *D3D11Dev_GetImmediateContext1_pfn)( 
  _In_            ID3D11Device          *This,
  _Out_           ID3D11DeviceContext1 **ppImmediateContext
);
typedef void (STDMETHODCALLTYPE *D3D11Dev_GetImmediateContext2_pfn)( 
  _In_            ID3D11Device          *This,
  _Out_           ID3D11DeviceContext2 **ppImmediateContext
);
typedef void (STDMETHODCALLTYPE *D3D11Dev_GetImmediateContext3_pfn)( 
  _In_            ID3D11Device          *This,
  _Out_           ID3D11DeviceContext3 **ppImmediateContext
);
typedef HRESULT (WINAPI *D3D11Dev_CreateSamplerState_pfn)(
  _In_            ID3D11Device        *This,
  _In_      const D3D11_SAMPLER_DESC  *pSamplerDesc,
  _Out_opt_       ID3D11SamplerState **ppSamplerState
);
typedef HRESULT (WINAPI *D3D11Dev_CreateTexture2D_pfn)(
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D
);
typedef HRESULT (WINAPI *D3D11Dev_CreateRenderTargetView_pfn)(
  _In_            ID3D11Device                   *This,
  _In_            ID3D11Resource                 *pResource,
  _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC  *pDesc,
  _Out_opt_       ID3D11RenderTargetView        **ppRTView
);
typedef void (WINAPI *D3D11_PSSetSamplers_pfn)(
  _In_     ID3D11DeviceContext       *This,
  _In_     UINT                       StartSlot,
  _In_     UINT                       NumSamplers,
  _In_opt_ ID3D11SamplerState *const *ppSamplers
);
typedef void (WINAPI *D3D11_RSSetScissorRects_pfn)(
  _In_           ID3D11DeviceContext *This,
  _In_           UINT                 NumRects,
  _In_opt_ const D3D11_RECT          *pRects
);
typedef void (WINAPI *D3D11_RSSetViewports_pfn)(
  _In_           ID3D11DeviceContext* This,
  _In_           UINT                 NumViewports,
  _In_opt_ const D3D11_VIEWPORT     * pViewports
);
typedef void (WINAPI *D3D11_UpdateSubresource_pfn)(
  _In_           ID3D11DeviceContext *This,
  _In_           ID3D11Resource      *pDstResource,
  _In_           UINT                 DstSubresource,
  _In_opt_ const D3D11_BOX           *pDstBox,
  _In_     const void                *pSrcData,
  _In_           UINT                 SrcRowPitch,
  _In_           UINT                 SrcDepthPitch
);
typedef HRESULT (WINAPI *D3D11_Map_pfn)(
  _In_      ID3D11DeviceContext      *This,
  _In_      ID3D11Resource           *pResource,
  _In_      UINT                      Subresource,
  _In_      D3D11_MAP                 MapType,
  _In_      UINT                      MapFlags,
  _Out_opt_ D3D11_MAPPED_SUBRESOURCE *pMappedResource
);
typedef void (WINAPI *D3D11_Unmap_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Resource      *pResource,
  _In_ UINT                 Subresource
);

typedef void (WINAPI *D3D11_CopyResource_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Resource      *pDstResource,
  _In_ ID3D11Resource      *pSrcResource
);
typedef void (WINAPI *D3D11_CopySubresourceRegion_pfn)(
  _In_           ID3D11DeviceContext *This,
  _In_           ID3D11Resource      *pDstResource,
  _In_           UINT                 DstSubresource,
  _In_           UINT                 DstX,
  _In_           UINT                 DstY,
  _In_           UINT                 DstZ,
  _In_           ID3D11Resource      *pSrcResource,
  _In_           UINT                 SrcSubresource,
  _In_opt_ const D3D11_BOX           *pSrcBox
);

typedef void (WINAPI *D3D11_VSSetShaderResources_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews
);
typedef void (WINAPI *D3D11_VSSetConstantBuffers_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumBuffers,
  _In_opt_       ID3D11Buffer *const             *ppConstantBuffers
);
typedef void (WINAPI *D3D11_PSSetShaderResources_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews
);
typedef void (WINAPI *D3D11_PSSetConstantBuffers_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumBuffers,
  _In_opt_       ID3D11Buffer *const             *ppConstantBuffers
);
typedef void (WINAPI *D3D11_GSSetShaderResources_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews
);
typedef void (WINAPI *D3D11_GSSetConstantBuffers_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumBuffers,
  _In_opt_       ID3D11Buffer *const             *ppConstantBuffers
);
typedef void (WINAPI *D3D11_HSSetShaderResources_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews
);
typedef void (WINAPI *D3D11_HSSetConstantBuffers_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumBuffers,
  _In_opt_       ID3D11Buffer *const             *ppConstantBuffers
);
typedef void (WINAPI *D3D11_DSSetShaderResources_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews
);
typedef void (WINAPI *D3D11_DSSetConstantBuffers_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumBuffers,
  _In_opt_       ID3D11Buffer *const             *ppConstantBuffers
);
typedef void (WINAPI *D3D11_CSSetShaderResources_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews
);
typedef void (WINAPI *D3D11_CSSetUnorderedAccessViews_pfn)(
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumUAVs,
  _In_opt_       ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
  _In_opt_ const UINT                             *pUAVInitialCounts
);


typedef HRESULT (WINAPI *D3D11Dev_CreateBuffer_pfn)(
  _In_           ID3D11Device            *This,
  _In_     const D3D11_BUFFER_DESC       *pDesc,
  _In_opt_ const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_      ID3D11Buffer           **ppBuffer
);
typedef HRESULT (WINAPI *D3D11Dev_CreateShaderResourceView_pfn)(
  _In_           ID3D11Device                     *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC  *pDesc,
  _Out_opt_      ID3D11ShaderResourceView        **ppSRView
);
typedef HRESULT (WINAPI *D3D11Dev_CreateDepthStencilView_pfn)(
  _In_            ID3D11Device                  *This,
  _In_            ID3D11Resource                *pResource,
  _In_opt_  const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,
  _Out_opt_       ID3D11DepthStencilView        **ppDepthStencilView
);
typedef HRESULT (WINAPI *D3D11Dev_CreateUnorderedAccessView_pfn)(
  _In_            ID3D11Device                     *This,
  _In_            ID3D11Resource                   *pResource,
  _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc,
  _Out_opt_       ID3D11UnorderedAccessView       **ppUAView
);

typedef HRESULT (WINAPI *D3D11Dev_CheckFeatureSupport_pfn)(
  _In_            ID3D11Device  *This, 
  _In_            D3D11_FEATURE  Feature,
  _Out_           void           *pFeatureSupportData,
  _In_            UINT            FeatureSupportDataSize );

typedef void (WINAPI *D3D11_DrawIndexed_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation
);
typedef void (WINAPI *D3D11_Draw_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCount,
  _In_ UINT                 StartVertexLocation
);
typedef void (WINAPI *D3D11_DrawIndexedInstanced_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation,
  _In_ UINT                 StartInstanceLocation
);
typedef void (WINAPI *D3D11_DrawIndexedInstancedIndirect_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs
);
typedef void (WINAPI *D3D11_DrawInstanced_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartVertexLocation,
  _In_ UINT                 StartInstanceLocation
);
typedef void (WINAPI *D3D11_DrawInstancedIndirect_pfn)(
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs
);


extern D3D11Dev_CreateBuffer_pfn                          D3D11Dev_CreateBuffer_Original;
extern D3D11Dev_CreateTexture2D_pfn                       D3D11Dev_CreateTexture2D_Original;
extern D3D11Dev_CreateRenderTargetView_pfn                D3D11Dev_CreateRenderTargetView_Original;
extern D3D11Dev_CreateShaderResourceView_pfn              D3D11Dev_CreateShaderResourceView_Original;

extern D3D11Dev_CreateVertexShader_pfn                    D3D11Dev_CreateVertexShader_Original;
extern D3D11Dev_CreatePixelShader_pfn                     D3D11Dev_CreatePixelShader_Original;
extern D3D11Dev_CreateGeometryShader_pfn                  D3D11Dev_CreateGeometryShader_Original;
extern D3D11Dev_CreateGeometryShaderWithStreamOutput_pfn  D3D11Dev_CreateGeometryShaderWithStreamOutput_Original;
extern D3D11Dev_CreateHullShader_pfn                      D3D11Dev_CreateHullShader_Original;
extern D3D11Dev_CreateDomainShader_pfn                    D3D11Dev_CreateDomainShader_Original;
extern D3D11Dev_CreateComputeShader_pfn                   D3D11Dev_CreateComputeShader_Original;

extern D3D11Dev_CreateDeferredContext_pfn                 D3D11Dev_CreateDeferredContext_Original;
extern D3D11Dev_CreateDeferredContext1_pfn                D3D11Dev_CreateDeferredContext1_Original;
extern D3D11Dev_CreateDeferredContext2_pfn                D3D11Dev_CreateDeferredContext2_Original;
extern D3D11Dev_CreateDeferredContext3_pfn                D3D11Dev_CreateDeferredContext3_Original;
extern D3D11Dev_GetImmediateContext_pfn                   D3D11Dev_GetImmediateContext_Original;
extern D3D11Dev_GetImmediateContext1_pfn                  D3D11Dev_GetImmediateContext1_Original;
extern D3D11Dev_GetImmediateContext2_pfn                  D3D11Dev_GetImmediateContext2_Original;
extern D3D11Dev_GetImmediateContext3_pfn                  D3D11Dev_GetImmediateContext3_Original;

extern D3D11_RSSetScissorRects_pfn                        D3D11_RSSetScissorRects_Original;
extern D3D11_RSSetViewports_pfn                           D3D11_RSSetViewports_Original;
extern D3D11_VSSetConstantBuffers_pfn                     D3D11_VSSetConstantBuffers_Original;
extern D3D11_PSSetShaderResources_pfn                     D3D11_PSSetShaderResources_Original;
extern D3D11_PSSetConstantBuffers_pfn                     D3D11_PSSetConstantBuffers_Original;
extern D3D11_UpdateSubresource_pfn                        D3D11_UpdateSubresource_Original;
extern D3D11_DrawIndexed_pfn                              D3D11_DrawIndexed_Original;
extern D3D11_Draw_pfn                                     D3D11_Draw_Original;
extern D3D11_DrawIndexedInstanced_pfn                     D3D11_DrawIndexedInstanced_Original;
extern D3D11_DrawIndexedInstancedIndirect_pfn             D3D11_DrawIndexedInstancedIndirect_Original;
extern D3D11_DrawInstanced_pfn                            D3D11_DrawInstanced_Original;
extern D3D11_DrawInstancedIndirect_pfn                    D3D11_DrawInstancedIndirect_Original;
extern D3D11_Map_pfn                                      D3D11_Map_Original;

extern D3D11_VSSetShader_pfn                              D3D11_VSSetShader_Original;
extern D3D11_PSSetShader_pfn                              D3D11_PSSetShader_Original;
extern D3D11_GSSetShader_pfn                              D3D11_GSSetShader_Original;
extern D3D11_HSSetShader_pfn                              D3D11_HSSetShader_Original;
extern D3D11_DSSetShader_pfn                              D3D11_DSSetShader_Original;
extern D3D11_CSSetShader_pfn                              D3D11_CSSetShader_Original;

extern D3D11_CopyResource_pfn                             D3D11_CopyResource_Original;
extern D3D11_CopySubresourceRegion_pfn                    D3D11_CopySubresourceRegion_Original;

enum class SK_D3D11_ShaderType {
  Vertex   =  1,
  Pixel    =  2,
  Geometry =  4,
  Domain   =  8,
  Hull     = 16,
  Compute  = 32,

  Invalid  = MAXINT
};


struct SK_D3D11_ShaderDesc
{
  SK_D3D11_ShaderType type    = SK_D3D11_ShaderType::Invalid;
  uint32_t            crc32c  = 0UL;
  IUnknown*           pShader = nullptr;

  std::vector <BYTE>  bytecode;

  struct
  {
    volatile ULONG last_frame = 0UL;
    __time64_t     last_time  = 0ULL;
    ULONG          refs       = 0;
  } usage;
};

#include <map>

struct SK_DisjointTimerQueryD3D11
{
  // Always issue this from the immediate context

  volatile ID3D11Query* async  = nullptr;
  volatile LONG         active = FALSE;

  D3D11_QUERY_DATA_TIMESTAMP_DISJOINT last_results = { };
};

struct SK_TimerQueryD3D11
{
  volatile ID3D11Query*         async   = nullptr;
  volatile LONG                 active  = FALSE;

  // Required per-query to support timing the execution of commands batched
  //   using deferred render contexts.
  volatile ID3D11DeviceContext* dev_ctx = nullptr;

  UINT64 last_results = { };
};

struct d3d11_shader_tracking_s
{
  void clear (void)
  {
    //active    = false;

    num_draws = 0;

    if ( set_of_views.empty () &&
         used_views.empty   () &&
         classes.empty      () )
    {
      return;
    }

    auto shader_class_crit_sec = [&](void)
    {
      extern SK_Thread_HybridSpinlock *cs_shader_vs, *cs_shader_ps,
                                      *cs_shader_gs,
                                      *cs_shader_hs, *cs_shader_ds,
                                      *cs_shader_cs;

      switch (type_)
      {
        default:
        //assert (false);
          return cs_shader_vs;

        case SK_D3D11_ShaderType::Vertex:   return cs_shader_vs;
        case SK_D3D11_ShaderType::Pixel:    return cs_shader_ps;
        case SK_D3D11_ShaderType::Geometry: return cs_shader_gs;
        case SK_D3D11_ShaderType::Hull:     return cs_shader_hs;
        case SK_D3D11_ShaderType::Domain:   return cs_shader_ds;
        case SK_D3D11_ShaderType::Compute:  return cs_shader_cs;
      }
    };

    std::lock_guard <SK_Thread_CriticalSection> auto_lock_gs (*shader_class_crit_sec ());

    for ( auto it : set_of_views )
      it->Release ();

    set_of_views.clear ();
    used_views.clear   ();


    for ( auto it : classes )
      it->Release ();

    classes.clear ();

    if ( pre_hud_rtv != nullptr)
    {
         pre_hud_rtv->Release ();
         pre_hud_rtv = nullptr;
    }

    //used_textures.clear ();

    //for (int i = 0; i < 16; i++)
      //current_textures [i] = 0x00;
  }

  void use (IUnknown* pShader) ;

  // Used for timing queries and interface tracking
  void activate   ( ID3D11DeviceContext        *pDevContext,
                    ID3D11ClassInstance *const *ppClassInstances,
                    UINT                        NumClassInstances );
  void deactivate ( ID3D11DeviceContext        *pDevContext );

  std::atomic_uint32_t    crc32c           =  0x00;
  std::atomic_bool        cancel_draws     = false;
  std::atomic_bool        highlight_draws  = false;
  std::atomic_bool        wireframe        = false;
  std::atomic_bool        on_top           =  true;

  struct
  {
    std::array < bool, SK_D3D11_MAX_DEV_CONTEXTS+1 > contexts;

    // Only examine the hash map when at least one context is active,
    //   or we will kill performance!
    volatile LONG
      active_count = 0L; 

    bool get (int dev_idx)
    {
      if (ReadAcquire (&active_count) > 0)
      {
        assert (dev_idx < SK_D3D11_MAX_DEV_CONTEXTS);

        if (dev_idx <= SK_D3D11_MAX_DEV_CONTEXTS)
          return contexts [dev_idx];
      }

      return false;
    }

    bool get (ID3D11DeviceContext* pDevCtx)
    {
      return get (SK_D3D11_GetDeviceContextHandle (pDevCtx));
    }

    void set (int dev_idx, bool active)
    {
      assert (dev_idx < SK_D3D11_MAX_DEV_CONTEXTS);

      if (ReadAcquire (&active_count) > 0 || active == true)
      {
        if (dev_idx <= SK_D3D11_MAX_DEV_CONTEXTS)
        {
          if (contexts [dev_idx] != active)
          {
                 if (active) InterlockedIncrement (&active_count);
            else if (ReadAcquire (&active_count) > 0)
                             InterlockedDecrement (&active_count);

            contexts [dev_idx] = active;
          }
        }
      }
    }

    void set (ID3D11DeviceContext* pDevCtx, bool active)
    {
      set (
        SK_D3D11_GetDeviceContextHandle (pDevCtx),
          active
      );
    }
  } active;

  std::atomic_ulong       num_draws        =     0;


  std::atomic_bool        pre_hud_source   =   false;
  std::atomic_long        pre_hud_rt_slot  =      -1;
  std::atomic_long        pre_hud_srv_slot =      -1;
  ID3D11RenderTargetView* pre_hud_rtv      = nullptr;

  // The slot used has meaning, but I think we can ignore it for now...
  //std::unordered_map <UINT, ID3D11ShaderResourceView *> used_views;

  std::set    <ID3D11ShaderResourceView *> set_of_views;
  std::vector <ID3D11ShaderResourceView *> used_views;


  struct cbuffer_override_s {
    uint32_t      parent;
    size_t        BufferSize; // Parent buffer's size
    bool          Enable;
    uint32_t      Slot;
    uint32_t      StartAddr;
    uint32_t      Size;
    float         Values [16];
  };

  std::vector <cbuffer_override_s> overrides;


  IUnknown*                         shader_obj     = nullptr;


  static SK_DisjointTimerQueryD3D11 disjoint_query;

  struct duration_s
  {
    // Timestamp at beginning
    SK_TimerQueryD3D11 start;

    // Timestamp at end
    SK_TimerQueryD3D11 end;
  };
  std::vector <duration_s> timers;

  // Cumulative runtime of all timers after the disjoint query
  //   is finished and reading these results would not stall
  //     the pipeline
  std::atomic_uint64_t              runtime_ticks   = 0ULL;
  double                            runtime_ms      = 0.0;
  double                            last_runtime_ms = 0.0;


  void addClassInstance (ID3D11ClassInstance* pInstance)
  {
    if (! classes.count (pInstance))
    {
      pInstance->AddRef ();
      classes.insert    (pInstance);
    }
  }

  std::set <ID3D11ClassInstance *> classes;

//  struct shader_constant_s
//  {
//    char                Name [128];
//    D3DXREGISTER_SET    RegisterSet;
//    UINT                RegisterIndex;
//    UINT                RegisterCount;
//    D3DXPARAMETER_CLASS Class;
//    D3DXPARAMETER_TYPE  Type;
//    UINT                Rows;
//    UINT                Columns;
//    UINT                Elements;
//    std::vector <shader_constant_s>
//                        struct_members;
//    bool                Override;
//    float               Data [4]; // TEMP HACK
//  };

//  std::vector <shader_constant_s> constants;

    SK_D3D11_ShaderType type_;
};

struct SK_D3D11_KnownShaders
{
  typedef std::unordered_map <uint32_t, std::unordered_set <uint32_t>> conditional_blacklist_t;

  template <typename _T>
  class ShaderRegistry
  {
  public:
    ShaderRegistry (void)
    {
           if (std::type_index (typeid (_T)) == std::type_index (typeid (ID3D11VertexShader)))
        type_ = SK_D3D11_ShaderType::Vertex;
      else if (std::type_index (typeid (_T)) == std::type_index (typeid (ID3D11PixelShader)))
        type_ = SK_D3D11_ShaderType::Pixel;
      else if (std::type_index (typeid (_T)) == std::type_index (typeid (ID3D11GeometryShader)))
        type_ = SK_D3D11_ShaderType::Geometry;
      else if (std::type_index (typeid (_T)) == std::type_index (typeid (ID3D11DomainShader)))
        type_ = SK_D3D11_ShaderType::Domain;
      else if (std::type_index (typeid (_T)) == std::type_index (typeid (ID3D11HullShader)))
        type_ = SK_D3D11_ShaderType::Hull;
      else if (std::type_index (typeid (_T)) == std::type_index (typeid (ID3D11ComputeShader)))
        type_ = SK_D3D11_ShaderType::Compute;

      tracked.type_ = type_;
    }

    std::unordered_map <_T*, uint32_t>                   rev;
    std::unordered_map <uint32_t, SK_D3D11_ShaderDesc>   descs;

    std::unordered_map <uint32_t, LONG>                  wireframe;
    std::unordered_map <uint32_t, LONG>                  blacklist;

    std::unordered_map <uint32_t, LONG>                  on_top;
    std::unordered_map <uint32_t, LONG>                  rewind;
    std::unordered_map <uint32_t, LONG>                  hud;

    std::unordered_map <uint32_t, std::wstring>          names;

    struct {
      std::unordered_map <uint32_t, LONG> before;
      std::unordered_map <uint32_t, LONG> after;
    } trigger_reshade;

    bool addTrackingRef ( std::unordered_map <uint32_t, LONG>& state,
                                              uint32_t         crc32c )
    {
      if (state.count (crc32c))
      {
        state [crc32c]++;

        return false;
      }

      state [crc32c]++;

      return true;
    }

    bool releaseTrackingRef (std::unordered_map <uint32_t, LONG>& state,
                                                 uint32_t         crc32c )
    {
      if (state.count (crc32c))
      {
        state [crc32c]--;

        if (state [crc32c] <= 0)
        {
          state.erase (crc32c);

          return true;
        }

        return false;
      }

      return true;
    }

    conditional_blacklist_t                              blacklist_if_texture;
    d3d11_shader_tracking_s                              tracked;

    struct {
      uint32_t                  shader [SK_D3D11_MAX_DEV_CONTEXTS+1];
      ID3D11ShaderResourceView* views  [SK_D3D11_MAX_DEV_CONTEXTS+1][128];
    } current;

    volatile LONG                                        changes_last_frame = 0;

    SK_D3D11_ShaderType type_;
  };

  
  //static std::array <bool, SK_D3D11_MAX_DEV_CONTEXTS+1> reshade_triggered;
  static bool reshade_triggered;

  ShaderRegistry <ID3D11PixelShader>    pixel; 
  ShaderRegistry <ID3D11VertexShader>   vertex;
  ShaderRegistry <ID3D11GeometryShader> geometry;
  ShaderRegistry <ID3D11HullShader>     hull; 
  ShaderRegistry <ID3D11DomainShader>   domain;
  ShaderRegistry <ID3D11ComputeShader>  compute;
};

using  SK_D3D11_KnownShaders_Singleton = SK_D3D11_KnownShaders&;
extern SK_D3D11_KnownShaders_Singleton __SK_Singleton_D3D11_Shaders (void);

extern SK_D3D11_KnownShaders_Singleton& SK_D3D11_Shader_Lambda (void);

#define SK_D3D11_Shaders SK_D3D11_Shader_Lambda()


typedef HRESULT (WINAPI *D3D11CreateDevice_pfn)(
  _In_opt_                            IDXGIAdapter         *pAdapter,
                                      D3D_DRIVER_TYPE       DriverType,
                                      HMODULE               Software,
                                      UINT                  Flags,
  _In_opt_                      const D3D_FEATURE_LEVEL    *pFeatureLevels,
                                      UINT                  FeatureLevels,
                                      UINT                  SDKVersion,
  _Out_opt_                           ID3D11Device        **ppDevice,
  _Out_opt_                           D3D_FEATURE_LEVEL    *pFeatureLevel,
  _Out_opt_                           ID3D11DeviceContext **ppImmediateContext);

typedef HRESULT (WINAPI *D3D11CreateDeviceAndSwapChain_pfn)(
  _In_opt_                             IDXGIAdapter*,
                                       D3D_DRIVER_TYPE,
                                       HMODULE,
                                       UINT,
  _In_reads_opt_ (FeatureLevels) CONST D3D_FEATURE_LEVEL*,
                                       UINT FeatureLevels,
                                       UINT,
  _In_opt_                       CONST DXGI_SWAP_CHAIN_DESC*,
  _Out_opt_                            IDXGISwapChain**,
  _Out_opt_                            ID3D11Device**,
  _Out_opt_                            D3D_FEATURE_LEVEL*,
  _Out_opt_                            ID3D11DeviceContext**);

typedef void (WINAPI *D3D11_UpdateSubresource1_pfn)(
  _In_           ID3D11DeviceContext1 *This,
  _In_           ID3D11Resource       *pDstResource,
  _In_           UINT                  DstSubresource,
  _In_opt_ const D3D11_BOX            *pDstBox,
  _In_     const void                 *pSrcData,
  _In_           UINT                  SrcRowPitch,
  _In_           UINT                  SrcDepthPitch,
  _In_           UINT                  CopyFlags
);

#include <../depends/include/nvapi/nvapi_lite_common.h>
typedef NvAPI_Status (__cdecl *NvAPI_D3D11_CreateVertexShaderEx_pfn)( __in        ID3D11Device *pDevice,        __in     const void                *pShaderBytecode, 
                                                                      __in        SIZE_T        BytecodeLength, __in_opt       ID3D11ClassLinkage  *pClassLinkage, 
                                                                      __in  const LPVOID                                                            pCreateVertexShaderExArgs,
                                                                      __out       ID3D11VertexShader                                              **ppVertexShader );

typedef NvAPI_Status (__cdecl *NvAPI_D3D11_CreateHullShaderEx_pfn)( __in        ID3D11Device *pDevice,        __in const void               *pShaderBytecode, 
                                                                    __in        SIZE_T        BytecodeLength, __in_opt   ID3D11ClassLinkage *pClassLinkage, 
                                                                    __in  const LPVOID                                                       pCreateHullShaderExArgs,
                                                                    __out       ID3D11HullShader                                           **ppHullShader );

typedef NvAPI_Status (__cdecl *NvAPI_D3D11_CreateDomainShaderEx_pfn)( __in        ID3D11Device *pDevice,        __in     const void               *pShaderBytecode, 
                                                                      __in        SIZE_T        BytecodeLength, __in_opt       ID3D11ClassLinkage *pClassLinkage, 
                                                                      __in  const LPVOID                                                           pCreateDomainShaderExArgs,
                                                                      __out       ID3D11DomainShader                                             **ppDomainShader );

typedef NvAPI_Status (__cdecl *NvAPI_D3D11_CreateGeometryShaderEx_2_pfn)( __in        ID3D11Device *pDevice,        __in     const void               *pShaderBytecode, 
                                                                          __in        SIZE_T        BytecodeLength, __in_opt       ID3D11ClassLinkage *pClassLinkage, 
                                                                          __in  const LPVOID                                                           pCreateGeometryShaderExArgs,
                                                                          __out       ID3D11GeometryShader                                           **ppGeometryShader );

typedef NvAPI_Status (__cdecl *NvAPI_D3D11_CreateFastGeometryShaderExplicit_pfn)( __in        ID3D11Device *pDevice,        __in     const void               *pShaderBytecode,
                                                                                  __in        SIZE_T        BytecodeLength, __in_opt       ID3D11ClassLinkage *pClassLinkage,
                                                                                  __in  const LPVOID                                                           pCreateFastGSArgs,
                                                                                  __out       ID3D11GeometryShader                                           **ppGeometryShader );

typedef NvAPI_Status (__cdecl *NvAPI_D3D11_CreateFastGeometryShader_pfn)( __in  ID3D11Device *pDevice,        __in     const void                *pShaderBytecode, 
                                                                          __in  SIZE_T        BytecodeLength, __in_opt       ID3D11ClassLinkage  *pClassLinkage,
                                                                          __out ID3D11GeometryShader                                            **ppGeometryShader );


typedef HRESULT (WINAPI *D3D11CreateDevice_pfn)(
  _In_opt_                            IDXGIAdapter         *pAdapter,
                                      D3D_DRIVER_TYPE       DriverType,
                                      HMODULE               Software,
                                      UINT                  Flags,
  _In_opt_                      const D3D_FEATURE_LEVEL    *pFeatureLevels,
                                      UINT                  FeatureLevels,
                                      UINT                  SDKVersion,
  _Out_opt_                           ID3D11Device        **ppDevice,
  _Out_opt_                           D3D_FEATURE_LEVEL    *pFeatureLevel,
  _Out_opt_                           ID3D11DeviceContext **ppImmediateContext);

typedef enum D3DX11_IMAGE_FILE_FORMAT {
  D3DX11_IFF_BMP          = 0,
  D3DX11_IFF_JPG          = 1,
  D3DX11_IFF_PNG          = 3,
  D3DX11_IFF_DDS          = 4,
  D3DX11_IFF_TIFF         = 10,
  D3DX11_IFF_GIF          = 11,
  D3DX11_IFF_WMP          = 12,
  D3DX11_IFF_FORCE_DWORD  = 0x7fffffff
} D3DX11_IMAGE_FILE_FORMAT, *LPD3DX11_IMAGE_FILE_FORMAT;

typedef struct D3DX11_IMAGE_INFO {
  UINT                     Width;
  UINT                     Height;
  UINT                     Depth;
  UINT                     ArraySize;
  UINT                     MipLevels;
  UINT                     MiscFlags;
  DXGI_FORMAT              Format;
  D3D11_RESOURCE_DIMENSION ResourceDimension;
  D3DX11_IMAGE_FILE_FORMAT ImageFileFormat;
} D3DX11_IMAGE_INFO, *LPD3DX11_IMAGE_INFO;


typedef struct D3DX11_IMAGE_LOAD_INFO {
  UINT              Width;
  UINT              Height;
  UINT              Depth;
  UINT              FirstMipLevel;
  UINT              MipLevels;
  D3D11_USAGE       Usage;
  UINT              BindFlags;
  UINT              CpuAccessFlags;
  UINT              MiscFlags;
  DXGI_FORMAT       Format;
  UINT              Filter;
  UINT              MipFilter;
  D3DX11_IMAGE_INFO *pSrcInfo;
} D3DX11_IMAGE_LOAD_INFO, *LPD3DX11_IMAGE_LOAD_INFO;

interface ID3DX11ThreadPump;

typedef HRESULT (WINAPI *D3DX11FilterTexture_pfn)(
   ID3D11DeviceContext *pContext,
   ID3D11Resource      *pTexture,
   UINT                SrcLevel,
   UINT                MipFilter
);

typedef HRESULT (WINAPI *D3DX11CreateTextureFromMemory_pfn)(
  _In_  ID3D11Device           *pDevice,
  _In_  LPCVOID                pSrcData,
  _In_  SIZE_T                 SrcDataSize,
  _In_  D3DX11_IMAGE_LOAD_INFO *pLoadInfo,
  _In_  ID3DX11ThreadPump      *pPump,
  _Out_ ID3D11Resource         **ppTexture,
  _Out_ HRESULT                *pHResult
);

typedef HRESULT (WINAPI *D3DX11CreateTextureFromFileW_pfn)(
  _In_  ID3D11Device           *pDevice,
  _In_  LPCWSTR                pSrcFile,
  _In_  D3DX11_IMAGE_LOAD_INFO *pLoadInfo,
  _In_  IUnknown               *pPump,
  _Out_ ID3D11Resource         **ppTexture,
  _Out_ HRESULT                *pHResult
);

interface ID3DX11ThreadPump;

typedef HRESULT (WINAPI *D3DX11GetImageInfoFromFileW_pfn)(
  _In_  LPCWSTR           pSrcFile,
  _In_  ID3DX11ThreadPump *pPump,
  _In_  D3DX11_IMAGE_INFO *pSrcInfo,
  _Out_ HRESULT           *pHResult
);

typedef void (WINAPI *D3D11_UpdateSubresource1_pfn)(
  _In_           ID3D11DeviceContext1 *This,
  _In_           ID3D11Resource       *pDstResource,
  _In_           UINT                  DstSubresource,
  _In_opt_ const D3D11_BOX            *pDstBox,
  _In_     const void                 *pSrcData,
  _In_           UINT                  SrcRowPitch,
  _In_           UINT                  SrcDepthPitch,
  _In_           UINT                  CopyFlags
);

HRESULT
STDMETHODCALLTYPE
Present1Callback (IDXGISwapChain1         *This,
                  UINT                     SyncInterval,
                  UINT                     PresentFlags,
            const DXGI_PRESENT_PARAMETERS *pPresentParameters);

HRESULT
STDMETHODCALLTYPE
DXGIFactory2_CreateSwapChainForHwnd_Override ( IDXGIFactory2                   *This,
                                    _In_       IUnknown                        *pDevice,
                                    _In_       HWND                             hWnd,
                                    _In_ const DXGI_SWAP_CHAIN_DESC1           *pDesc,
                                _In_opt_       DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc,
                                _In_opt_       IDXGIOutput                     *pRestrictToOutput,
                                   _Out_       IDXGISwapChain1                 **ppSwapChain );
HRESULT
STDMETHODCALLTYPE
DXGIFactory2_CreateSwapChainForCoreWindow_Override ( IDXGIFactory2             *This,
                                          _In_       IUnknown                  *pDevice,
                                          _In_       IUnknown                  *pWindow,
                                          _In_ const DXGI_SWAP_CHAIN_DESC1     *pDesc,
                                      _In_opt_       IDXGIOutput               *pRestrictToOutput,
                                         _Out_       IDXGISwapChain1          **ppSwapChain );

HRESULT
STDMETHODCALLTYPE
DXGIFactory2_CreateSwapChainForComposition_Override ( IDXGIFactory2          *This,                           
                                       _In_           IUnknown               *pDevice,
                                       _In_     const DXGI_SWAP_CHAIN_DESC1  *pDesc,
                                       _In_opt_       IDXGIOutput            *pRestrictToOutput,
                                       _Outptr_       IDXGISwapChain1       **ppSwapChain );





namespace SK
{
  namespace DXGI
  {
    struct PipelineStatsD3D11
    {
      struct StatQueryD3D11  
      {
        volatile ID3D11Query* async  = nullptr;
        volatile LONG         active = FALSE;
      } query;

      D3D11_QUERY_DATA_PIPELINE_STATISTICS
                 last_results = { };
    } extern pipeline_stats_d3d11;
  };
};


void            WaitForInitDXGI             (void);

void  __stdcall SK_D3D11_PreLoadTextures    (void);

void  __stdcall SK_D3D11_TexCacheCheckpoint (void);
bool  __stdcall SK_D3D11_TextureIsCached    (ID3D11Texture2D*     pTex);
void  __stdcall SK_D3D11_UseTexture         (ID3D11Texture2D*     pTex);
bool  __stdcall SK_D3D11_RemoveTexFromCache (ID3D11Texture2D*     pTex, bool blacklist = false);
void  __stdcall SK_D3D11_PresentFirstFrame  (IDXGISwapChain*      pSwapChain);


void  __stdcall SK_D3D11_UpdateRenderStats  (IDXGISwapChain*      pSwapChain);


BOOL SK_DXGI_SupportsTearing  (void);
void SK_CEGUI_QueueResetD3D11 (void);



void SK_D3D11_AssociateVShaderWithHUD (uint32_t crc32, bool set = true);
void SK_D3D11_AssociatePShaderWithHUD (uint32_t crc32, bool set = true);


#define SK_D3D11_DeclHUDShader(crc32c,type) \
  SK_D3D11_RegisterHUDShader ((crc32c), std::type_index (typeid (type)));

bool
SK_D3D11_RegisterHUDShader (        uint32_t  bytecode_crc32c,
                             std::type_index _T =
                             std::type_index  (
                                       typeid ( ID3D11VertexShader )
                                              ),
                                        bool  remove = false       );
bool
SK_D3D11_UnRegisterHUDShader ( uint32_t         bytecode_crc32c,
                               std::type_index _T =
                               std::type_index  (
                                         typeid ( ID3D11VertexShader )
                                                )                    );

DWORD
__stdcall
HookD3D11 (LPVOID user);

void
WINAPI
SK_HookDXGI (void);

int  SK_D3D11_PurgeHookAddressCache  (void);
void SK_D3D11_UpdateHookAddressCache (void);

const wchar_t* SK_D3D11_DescribeUsage     (D3D11_USAGE              usage)  ;
const wchar_t* SK_D3D11_DescribeFilter    (D3D11_FILTER             filter) ;
std::wstring   SK_D3D11_DescribeMiscFlags (D3D11_RESOURCE_MISC_FLAG flags);
std::wstring   SK_D3D11_DescribeBindFlags (D3D11_BIND_FLAG          flags);

#endif /* __SK__DXGI_BACKEND_H__ */