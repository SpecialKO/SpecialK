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

  // Interface-based DXGI call
#define DXGI_LOG_CALL_I(_Interface,_Name,_Format)                            \
  {                                                                          \
    std::unique_ptr    <wchar_t[]> pwszBuffer =                              \
      std::make_unique <wchar_t[]> (4096);                                   \
    wchar_t*  wszPreFormatted  = pwszBuffer.get ();                          \
    wchar_t*  wszPostFormatted = pwszBuffer.get () + 1024;                   \
    if (pwszBuffer != nullptr)                                               \
    {                                                                        \
      swprintf ( wszPreFormatted,  L"%s::%s (", _Interface, _Name );         \
      swprintf (wszPostFormatted, _Format

  // Global DXGI call
#define DXGI_LOG_CALL(_Name,_Format)                                         \
  {                                                                          \
    std::unique_ptr    <wchar_t[]>  pwszBuffer =                             \
      std::make_unique <wchar_t[]> (4096);                                   \
    wchar_t*  wszPreFormatted  = pwszBuffer.get ();                          \
    wchar_t*  wszPostFormatted = pwszBuffer.get () + 1024;                   \
    if (pwszBuffer != nullptr)                                               \
    {                                                                        \
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
CreateDXGIFactory  (       REFIID   riid,
                     _Out_ void   **ppFactory );
HRESULT
WINAPI
CreateDXGIFactory1 (       REFIID   riid,
                     _Out_ void   **ppFactory );
HRESULT
WINAPI
CreateDXGIFactory2 (       UINT     Flags,
                           REFIID   riid,
                     _Out_ void   **ppFactory );



HRESULT
STDMETHODCALLTYPE
SK_DXGI_FindClosestMode ( IDXGISwapChain *pSwapChain,
              _In_  const DXGI_MODE_DESC *pModeToMatch,
                   _Out_  DXGI_MODE_DESC *pClosestMatch,
                _In_opt_  IUnknown       *pConcernedDevice,
                          BOOL            bApplyOverrides = FALSE );

HRESULT
STDMETHODCALLTYPE
SK_DXGI_ResizeTarget ( IDXGISwapChain *This,
                  _In_ DXGI_MODE_DESC *pNewTargetParameters,
                       BOOL            bApplyOverrides = FALSE );

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

// Increased to 64 for Crysis... they're really going out of their way to make
// sure stuff _can't_ run Crysis.
static const int SK_D3D11_MAX_DEV_CONTEXTS = 128;

LONG
SK_D3D11_GetDeviceContextHandle (ID3D11DeviceContext *pCtx);

// Use this when wrapping a device context, we want the wrapper and the
//   internal pointer to agree on context handle
void
SK_D3D11_CopyContextHandle ( ID3D11DeviceContext *pSrcCtx,
                             ID3D11DeviceContext *pDstCtx );

static int __SK_D3D11_DebugLayerActive = -1;

__forceinline
void
SK_D3D11_SetDebugName (       ID3D11DeviceChild* pDevChild,
                        const std::wstring&      kName )
{
  if (__SK_D3D11_DebugLayerActive == -1)
  {
    SK_ComPtr <ID3D11Device> pDev;
    pDevChild->GetDevice (  &pDev.p );

    SK_ComQIPtr <ID3D11Debug> pDebug (pDev);
    __SK_D3D11_DebugLayerActive =
                              pDebug.p != nullptr ?
                                                1 : 0;
  }

  if (! __SK_D3D11_DebugLayerActive)
    return;

  if (pDevChild != nullptr && kName.size () > 0)
  {
    D3D_SET_OBJECT_NAME_N_W ( pDevChild,
                   static_cast <UINT> ( kName.size () ),
                                        kName.data ()
                            );
  }
}

__forceinline
void
SK_D3D12_SetDebugName (       ID3D12Object* pD3D12Obj,
                        const std::wstring&     kName )
{
  if (pD3D12Obj != nullptr && kName.size () > 0)
  {
#if 1
    D3D_SET_OBJECT_NAME_N_W ( pD3D12Obj,
                   static_cast <UINT> ( kName.size () ),
                                        kName.data ()
                            );
#else
    pD3D12Obj->SetName ( kName.c_str () );
#endif
  }
}

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
              sizeof (pByteCode), nullptr,
              reinterpret_cast <ID3D11VertexShader **>(&shader)
            );
        }

        else if ( std::type_index ( typeid (   _ShaderType   ) ) ==
                  std::type_index ( typeid (ID3D11PixelShader) ) )
        {
          hrCompile =
            pDev->CreatePixelShader (
                      pByteCode,
              sizeof (pByteCode), nullptr,
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

std::wstring
__stdcall
SK_DXGI_FormatToStr (DXGI_FORMAT fmt) noexcept;

const wchar_t*
SK_DXGI_DescribeScalingMode (DXGI_MODE_SCALING mode) noexcept;

const wchar_t*
SK_DXGI_DescribeScanlineOrder (DXGI_MODE_SCANLINE_ORDER order) noexcept;

const wchar_t*
SK_DXGI_DescribeSwapEffect (DXGI_SWAP_EFFECT swap_effect) noexcept;

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

void        SK_DXGI_QuickHook (void);
void WINAPI SK_HookDXGI       (void);

void SK_DXGI_BorderCompensation (UINT& x, UINT& y);

void WINAPI SK_DXGI_SetPreferredAdapter (int override_id) noexcept;

#endif /* __SK__DXGI_BACKEND_H__ */