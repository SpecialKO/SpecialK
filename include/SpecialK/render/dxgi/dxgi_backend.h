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

#include <Unknwnbase.h>

#include <SpecialK/tls.h>
#include <SpecialK/thread.h>
#include <SpecialK/utility.h>
#include <SpecialK/render/dxgi/dxgi_interfaces.h>
#include <SpecialK/render/d3d11/d3d11_interfaces.h>
#include <SpecialK/render/d3d12/d3d12_interfaces.h>
#include <dxgidebug.h>

#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>
#include <concurrent_vector.h>

#include <string>
#include <typeindex>

#include <unordered_set>
#include <unordered_map>
#include <array>
#include <set>
#include <map>

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
  dll_log->Log ( L"[   DXGI   ] [@]  Return: %s  -  < " __FUNCTIONW__ L" >",\
                SK_DescribeHRESULT (_Ret) );                                \
}

#define DXGI_SKIP_CALL(_Ret, _Call) {                                        \
  (_Ret) = (_Call);                                                          \
  dll_log->Log ( L"[   DXGI   ] [@]  Skipped: %s  -  < " __FUNCTIONW__ L" >",\
                SK_DescribeHRESULT (_Ret) );                                 \
}

  // Interface-based DXGI call
#define DXGI_LOG_CALL_I(_Interface,_Name,_Format)                            \
  {                                                                          \
    wchar_t* pwszBuffer        = SK_TLS_Bottom ()->scratch_memory->log.      \
                                  formatted_output.alloc (4096, true);       \
    if (pwszBuffer != nullptr)                                               \
    {                                                                        \
      wchar_t*  wszPreFormatted  =  pwszBuffer;                              \
      wchar_t*  wszPostFormatted = &pwszBuffer [1024];                       \
      swprintf ( wszPreFormatted,  L"%s::%s (", _Interface, _Name );         \
      swprintf (wszPostFormatted, _Format

  // Global DXGI call
#define DXGI_LOG_CALL(_Name,_Format)                                         \
  {                                                                          \
    wchar_t* pwszBuffer        = SK_TLS_Bottom ()->scratch_memory->log.      \
                                  formatted_output.alloc (4096, true);       \
    if (pwszBuffer != nullptr)                                               \
    {                                                                        \
      wchar_t*  wszPreFormatted  =  pwszBuffer;                              \
      wchar_t*  wszPostFormatted = &pwszBuffer [1024];                       \
      swprintf (wszPreFormatted,  L"%s (", _Name);                           \
      swprintf (wszPostFormatted, _Format

#define DXGI_LOG_CALL_END                                                    \
      wchar_t* wszFullyFormatted = wszPostFormatted + 1024;                  \
      swprintf   ( wszFullyFormatted, L"%s%s)",                              \
                     wszPreFormatted, wszPostFormatted );                    \
      dll_log->Log ( L"[   DXGI   ] [!] %-102s -- %s", wszFullyFormatted,    \
                      SK_SummarizeCaller ().c_str () );                      \
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
WINAPI
CreateDXGIFactory  (              REFIID   riid,
                     _COM_Outptr_ void   **ppFactory );
HRESULT
WINAPI
CreateDXGIFactory1 (              REFIID   riid,
                     _COM_Outptr_ void   **ppFactory );
HRESULT
WINAPI
CreateDXGIFactory2 (              UINT     Flags,
                                  REFIID   riid,
                     _COM_Outptr_ void   **ppFactory );



HRESULT
STDMETHODCALLTYPE
SK_DXGI_FindClosestMode ( IDXGISwapChain *pSwapChain,
                    _In_  DXGI_MODE_DESC *pModeToMatch,
                   _Out_  DXGI_MODE_DESC *pClosestMatch,
                _In_opt_  IUnknown       *pConcernedDevice,
                          BOOL            bApplyOverrides = FALSE );

HRESULT
STDMETHODCALLTYPE
SK_DXGI_ResizeTarget ( IDXGISwapChain *This,
                  _In_ DXGI_MODE_DESC *pNewTargetParameters,
                       BOOL            bApplyOverrides = FALSE );

HRESULT
STDMETHODCALLTYPE
SK_DXGI_SwapChain_ResizeBuffers_Impl (
  _In_ IDXGISwapChain *pSwapChain,
  _In_ UINT            BufferCount,
  _In_ UINT            Width,
  _In_ UINT            Height,
  _In_ DXGI_FORMAT     NewFormat,
  _In_ UINT            SwapChainFlags,
       BOOL            bWrapped );

HRESULT
STDMETHODCALLTYPE
SK_DXGI_SwapChain_ResizeTarget_Impl (
  _In_       IDXGISwapChain *pSwapChain,
  _In_ const DXGI_MODE_DESC *pNewTargetParameters,
             BOOL            bWrapped );

HRESULT
STDMETHODCALLTYPE
SK_DXGI_SwapChain_SetFullscreenState_Impl (
  _In_       IDXGISwapChain *pSwapChain,
  _In_       BOOL            Fullscreen,
  _In_opt_   IDXGIOutput    *pTarget,
             BOOL            bWrapped );


__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
DXGIFactory_CreateSwapChain_Override (
              IDXGIFactory          *This,
  _In_        IUnknown              *pDevice,
  _In_  const DXGI_SWAP_CHAIN_DESC  *pDesc,
  _Out_       IDXGISwapChain       **ppSwapChain
);

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
STDMETHODCALLTYPE
DXGISwap3_ResizeBuffers1_Override (        IDXGISwapChain3 *This,
                                _In_       UINT             BufferCount,
                                _In_       UINT             Width,
                                _In_       UINT             Height,
                                _In_       DXGI_FORMAT      NewFormat,
                                _In_       UINT             SwapChainFlags,
                                _In_ const UINT            *pCreationNodeMask,
                                _In_       IUnknown* const *ppPresentQueue);

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
   constexpr int        MAX_GPU_NODES = 4;

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

  int64_t  min_availability  = INT64_MAX;
  double   max_load_percent  = 0.0;

  uint64_t budget_changes    = 0;
} extern dxgi_mem_stats [MAX_GPU_NODES];

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
} extern dxgi_mem_info [NumBuffers];

// Increased to 192 for Disgaea 5 (WTF?!)
static constexpr int SK_D3D11_MAX_DEV_CONTEXTS = 256;
extern           int SK_D3D11_AllocatedDevContexts;

LONG
SK_D3D11_GetDeviceContextHandle (ID3D11DeviceContext *pDevCtx);

// Use this when wrapping a device context, we want the wrapper and the
//   internal pointer to agree on context handle
void
SK_D3D11_CopyContextHandle ( ID3D11DeviceContext *pSrcCtx,
                             ID3D11DeviceContext *pDstCtx );

extern std::string
SK_WideCharToUTF8 (const std::wstring& in);

bool SK_DXGI_HasDebugName (       IDXGIObject* pDXGIObject );
void SK_DXGI_SetDebugName (       IDXGIObject* pDXGIObject,
                           const std::wstring& kName );

bool SK_D3D11_HasDebugName (       ID3D11DeviceChild* pDevChild );
void SK_D3D11_SetDebugName (       ID3D11DeviceChild* pDevChild,
                             const std::wstring&      kName );

bool SK_D3D12_HasDebugName (       ID3D12Object* pD3D12Obj );
void SK_D3D12_SetDebugName (       ID3D12Object* pD3D12Obj,
                             const std::wstring& kName );

std::wstring SK_D3D12_GetDebugNameW    (ID3D12Object* pD3D12Obj);
std::string  SK_D3D12_GetDebugNameA    (ID3D12Object* pD3D12Obj);
std::string  SK_D3D12_GetDebugNameUTF8 (ID3D12Object* pD3D12Obj);

std::wstring SK_D3D11_GetDebugNameW    (ID3D11DeviceChild* pD3D11Obj);
std::string  SK_D3D11_GetDebugNameA    (ID3D11DeviceChild* pD3D11Obj);
std::string  SK_D3D11_GetDebugNameUTF8 (ID3D11DeviceChild* pD3D11Obj);

namespace SK
{
  namespace DXGI
  {
    bool Startup  (void);
    bool Shutdown (void);

    std::string getPipelineStatsDesc (void);

    //extern HMODULE hModD3D10;
    static HMODULE hModD3D12 = nullptr;
    extern HMODULE hModD3D11;

    HRESULT StartBudgetThread           (IDXGIAdapter** ppAdapter);
    HRESULT StartBudgetThread_NoAdapter (void);

    void    ShutdownBudgetThread        (void);

    DWORD
    WINAPI
    BudgetThread (LPVOID user_data);

    template <typename _ShaderType>
    struct ShaderBase
    {
      _ShaderType *shader           = nullptr;

      bool loadPreBuiltShader ( ID3D11Device* pDev, const BYTE pByteCode [] )
      {
        HRESULT hrCompile =
          E_NOTIMPL;

        if ( std::type_index ( typeid (    _ShaderType   ) ) ==
             std::type_index ( typeid (ID3D11VertexShader) ) )
        {
          hrCompile =
            pDev->CreateVertexShader (
                       pByteCode,
              sizeof (*pByteCode), nullptr,
              reinterpret_cast <ID3D11VertexShader **>(&shader)
            );
        }

        else if ( std::type_index ( typeid (   _ShaderType   ) ) ==
                  std::type_index ( typeid (ID3D11PixelShader) ) )
        {
          hrCompile =
            pDev->CreatePixelShader (
                       pByteCode,
              sizeof (*pByteCode), nullptr,
              reinterpret_cast <ID3D11PixelShader **>(&shader)
            );
        }

        return
          SUCCEEDED (hrCompile);
      }

      void releaseResources (void)
      {
        if (shader           != nullptr) {           shader->Release (); shader           = nullptr; }
      }
    };
  }
}

bool SK_DXGI_LinearizeSRGB (IDXGISwapChain* pChainThatUsedToBeSRGB);


typedef HRESULT (STDMETHODCALLTYPE *CreateDXGIFactory2_pfn) \
  (UINT Flags, REFIID riid,  void** ppFactory);
typedef HRESULT (STDMETHODCALLTYPE *CreateDXGIFactory1_pfn) \
  (REFIID riid,  void** ppFactory);
typedef HRESULT (STDMETHODCALLTYPE *CreateDXGIFactory_pfn)  \
  (REFIID riid,  void** ppFactory);

extern CreateDXGIFactory_pfn  CreateDXGIFactory_Import;
extern CreateDXGIFactory1_pfn CreateDXGIFactory1_Import;
extern CreateDXGIFactory2_pfn CreateDXGIFactory2_Import;

std::string_view
__stdcall
SK_DXGI_FormatToStr (DXGI_FORMAT fmt) noexcept;

const char*
SK_DXGI_DescribeScalingMode (DXGI_MODE_SCALING mode) noexcept;

const char*
SK_DXGI_DescribeScanlineOrder (DXGI_MODE_SCANLINE_ORDER order) noexcept;

const char*
SK_DXGI_DescribeSwapEffect (DXGI_SWAP_EFFECT swap_effect) noexcept;

std::string
SK_DXGI_DescribeSwapChainFlags (DXGI_SWAP_CHAIN_FLAG swap_flags, INT* pLines = nullptr);

std::string
SK_DXGI_FeatureLevelsToStr (       int    FeatureLevels,
                             const DWORD* pFeatureLevels );

void
WINAPI
SK_DXGI_AdapterOverride ( IDXGIAdapter**   ppAdapter,
                          D3D_DRIVER_TYPE* DriverType );

int
SK_GetDXGIFactoryInterfaceVer (const IID& riid);

std::string
SK_GetDXGIFactoryInterfaceEx (const IID& riid);

int
SK_GetDXGIFactoryInterfaceVer (gsl::not_null <IUnknown *> pFactory);

std::string
SK_GetDXGIFactoryInterface    (gsl::not_null <IUnknown *> pFactory);

int
SK_GetDXGIAdapterInterfaceVer (const IID& riid);

std::string
SK_GetDXGIAdapterInterfaceEx  (const IID& riid);

int
SK_GetDXGIAdapterInterfaceVer (gsl::not_null <IUnknown *> pAdapter);

std::string
SK_GetDXGIAdapterInterface    (gsl::not_null <IUnknown *> pAdapter);

void        SK_DXGI_QuickHook (void);
void WINAPI SK_HookDXGI       (void);

void WINAPI SK_DXGI_SetPreferredAdapter (int override_id) noexcept;

void SK_DXGI_UpdateSwapChain (IDXGISwapChain* This);


HRESULT SK_DXGI_GetDebugInterface (REFIID riid, void** ppDebug);
HRESULT SK_DXGI_OutputDebugString (const std::string& str, DXGI_INFO_QUEUE_MESSAGE_SEVERITY severity);
HRESULT SK_DXGI_ReportLiveObjects (IUnknown* pDev = nullptr);


static constexpr
BOOL
SK_DXGI_IsFlipModelSwapEffect (DXGI_SWAP_EFFECT swapEffect) noexcept
{
  return
    ( swapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD ||
      swapEffect == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL );
}

static
BOOL
SK_DXGI_IsFlipModelSwapChain (const DXGI_SWAP_CHAIN_DESC& desc) noexcept
{
  return
    SK_DXGI_IsFlipModelSwapEffect (desc.SwapEffect);
};

HRESULT WINAPI SK_DXGI_DisableVBlankVirtualization (void);

// Returns true if the selected mode scaling would cause an exact match to be ignored
bool
SK_DXGI_IsScalingPreventingRequestedResolution ( DXGI_MODE_DESC *pDesc,
                                                 IDXGIOutput    *pOutput,
                                                 IUnknown       *pDevice );

void
SK_DXGI_UpdateColorSpace ( IDXGISwapChain3   *This,
                           DXGI_OUTPUT_DESC1 *outDesc = nullptr );


struct dxgi_caps_t {
  struct {
    bool latency_control = false;
    bool enqueue_event   = false;
  } device;

  struct {
    bool flip_sequential = false;
    bool flip_discard    = false;
    bool waitable        = false;
  } present;

  struct {
    BOOL allow_tearing   = FALSE;
  } swapchain;

  std::atomic_bool init  = false;
};

extern dxgi_caps_t dxgi_caps;


#endif /* __SK__DXGI_BACKEND_H__ */