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

#ifndef __SK__D3D9_BACKEND_H__
#define __SK__D3D9_BACKEND_H__

#undef D3D_DISABLE_9EX

struct ID3DXConstantTable;

typedef enum _D3DXREGISTER_SET    D3DXREGISTER_SET,    *LPD3DXREGISTER_SET;
typedef enum _D3DXPARAMETER_CLASS D3DXPARAMETER_CLASS, *LPD3DXPARAMETER_CLASS;
typedef enum _D3DXPARAMETER_TYPE  D3DXPARAMETER_TYPE,  *LPD3DXPARAMETER_TYPE;

struct IUnknown;
#include <Unknwnbase.h>

#include <Windows.h>

#if (defined (NOGDI) || (! defined (_WINGDI_)))
#undef        NOGDI
#undef      _WINGDI_
#include    <wingdi.h>
#endif

#if (! (defined (_d3d9TYPES_H_) && defined (_D3D9_H_))) || ((! defined (DIRECT3D_VERSION)) || DIRECT3D_VERSION < 0x0900) || (! defined (D3DAPI))
#undef  _D3D9_H_
#include <d3d9.h>
#endif

#include <unordered_map>
#include <unordered_set>
#include <cstdint>

#include <dxgiformat.h>

void SK_D3D9_QuickHook (void);

std::string   SK_D3D9_FormatToStr                (D3DFORMAT Format, bool include_ordinal);
std::string   SK_D3D9_PresentParameterFlagsToStr (DWORD dwFlags);
std::string   SK_D3D9_SwapEffectToStr            (D3DSWAPEFFECT Effect);
INT __stdcall SK_D3D9_BytesPerPixel              (D3DFORMAT Format);
std::string   SK_D3D9_UsageToStr                 (DWORD dwUsage);
DXGI_FORMAT   SK_D3D9_FormatToDXGI               (D3DFORMAT format);

namespace SK   {
namespace D3D9 {
  struct KnownShaders
  {
    //typedef std::unordered_map <uint32_t, std::unordered_set <ID3D11ShaderResourceView *>> conditional_blacklist_t;

    template <typename _T>
    struct ShaderRegistry
    {
      void clear (void)
      {
        rev.clear ();
        clear_state ();

        changes_last_frame = 0;
      }

      void clear_state (void)
      {
        current.crc32c = 0x0;
        current.ptr = nullptr;
      }

      std::unordered_map <_T*, uint32_t>      rev;
      //std::unordered_map <uint32_t, SK_D3D11_ShaderDesc> descs;

      std::unordered_set <uint32_t>           wireframe;
      std::unordered_set <uint32_t>           blacklist;

      //conditional_blacklist_t                 blacklist_if_texture;

      struct
      {
        uint32_t                              crc32c = 0x00;
        _T* ptr = nullptr;
      } current;

      //d3d11_shader_tracking_s                 tracked;

      ULONG                                   changes_last_frame = 0;
    };

    ShaderRegistry <IDirect3DPixelShader9>    pixel;
    ShaderRegistry <IDirect3DVertexShader9>   vertex;
  };

// Dumb macro in WinUser.h
#undef DrawState

  struct DrawState
  {
    // Most of these states are not tracked
    DWORD           srcblend        = 0;
    DWORD           dstblend        = 0;
    DWORD           srcalpha        = 0;     // Separate Alpha Blend Eq: Src
    DWORD           dstalpha        = 0;     // Separate Alpha Blend Eq: Dst
    bool            alpha_test      = false; // Test Alpha?
    DWORD           alpha_ref       = 0;     // Value to test.
    bool            zwrite          = false; // Depth Mask

    volatile ULONG  draws           = 0; // Number of draw calls
    volatile ULONG  frames          = 0;

    bool            cegui_active    = false;

    int             instances       = 0;

    uint32_t        current_tex  [256] = {      };
    float           viewport_off [4]   = { 0.0f }; // Most vertex shaders use this and we can
                                                   //   test the set values to determine if a draw
                                                   //     is HUD or world-space.
                                                   //
    volatile ULONG  draw_count  = 0;
    volatile ULONG  next_draw   = 0;
    volatile ULONG  scene_count = 0;
  };

  struct FrameState
  {
    void clear (void) noexcept
    {
      pixel_shaders.clear          (); vertex_shaders.clear           ();
      vertex_buffers.dynamic.clear (); vertex_buffers.immutable.clear ();
    }

    std::unordered_set <uint32_t>                 pixel_shaders;
    std::unordered_set <uint32_t>                 vertex_shaders;

    struct {
      std::unordered_set <IDirect3DVertexBuffer9 *> dynamic;
      std::unordered_set <IDirect3DVertexBuffer9 *> immutable;
    } vertex_buffers;
  };

  struct KnownObjects
  {
    void clear (void) noexcept {
      static_vbs.clear  ();
      dynamic_vbs.clear ();
    };

    std::unordered_set <IDirect3DVertexBuffer9 *> static_vbs;
    std::unordered_set <IDirect3DVertexBuffer9 *> dynamic_vbs;
  };

  struct RenderTargetTracker
  {
    void clear (void) noexcept
    {
      pixel_shaders.clear  ();
      vertex_shaders.clear ();

      active = false;
    }

    IDirect3DBaseTexture9*        tracking_tex  = nullptr;

    std::unordered_set <uint32_t> pixel_shaders;
    std::unordered_set <uint32_t> vertex_shaders;

    bool                          active        = false;
  };

  struct ShaderTracker
  {
    void clear (void) noexcept
    {
      active    = false;
      InterlockedExchange (&num_draws, 0);
      used_textures.clear ();

      for (auto&& current_texture : current_textures)
        current_texture = nullptr;
    }

    void use (IUnknown* pShader);

    uint32_t                      crc32c       =  0x00;
    bool                          cancel_draws = false;
    bool                          clamp_coords = false;
    bool                          active       = false;
    volatile LONG                 num_draws    =     0;
    std::unordered_set <IDirect3DBaseTexture9*> used_textures;
                        IDirect3DBaseTexture9*  current_textures [16] = { };

    //std::vector <IDirect3DBaseTexture9 *> samplers;

    IUnknown*                     shader_obj  = nullptr;
    ID3DXConstantTable*           ctable      = nullptr;

    struct shader_constant_s
    {
      char                Name [128];
      D3DXREGISTER_SET    RegisterSet;
      UINT                RegisterIndex;
      UINT                RegisterCount;
      D3DXPARAMETER_CLASS Class;
      D3DXPARAMETER_TYPE  Type;
      UINT                Rows;
      UINT                Columns;
      UINT                Elements;
      std::vector <shader_constant_s>
                          struct_members;
      bool                Override;
      float               Data [4]; // TEMP HACK
    };

    std::vector <shader_constant_s> constants;
  };

  struct VertexBufferTracker
  {
    void clear (void);
    void use   (void);

    IDirect3DVertexBuffer9*       vertex_buffer = nullptr;

    std::unordered_set <
      IDirect3DVertexDeclaration9*
    >                             vertex_decls;

    //uint32_t                      crc32c       =  0x00;
    bool                          cancel_draws = false;
    bool                          wireframe    = false;
    bool                          active       = false;
    volatile LONG                 num_draws    = 0L;
    volatile LONG                 instanced    = 0L;
    int                           instances    = 1;

    std::unordered_set <uint32_t> vertex_shaders;
    std::unordered_set <uint32_t> pixel_shaders;
    std::unordered_set <uint32_t> textures;

    std::unordered_set <IDirect3DVertexBuffer9*>
      wireframes;
  };

  extern SK_LazyGlobal <KnownShaders>        Shaders;
  extern SK_LazyGlobal <DrawState>           draw_state;
  extern SK_LazyGlobal <FrameState>          last_frame;
  extern SK_LazyGlobal <KnownObjects>        known_objs;
  extern SK_LazyGlobal <RenderTargetTracker> tracked_rt;
  extern SK_LazyGlobal <ShaderTracker>       tracked_vs;
  extern SK_LazyGlobal <ShaderTracker>       tracked_ps;
  extern SK_LazyGlobal <VertexBufferTracker> tracked_vb;

  struct ShaderDisassembly {
    std::string header;
    std::string code;
    std::string footer;
  };

  enum class ShaderClass {
    Unknown = 0x00,
    Pixel   = 0x01,
    Vertex  = 0x02
  };
}
}

#include <string>

namespace SK
{
  namespace D3D9
  {
    bool Startup  (void);
    bool Shutdown (void);

    std::string WINAPI getPipelineStatsDesc (void);

    struct PipelineStatsD3D9 {
      struct StatQueryD3D9 {
        IDirect3DQuery9* object = nullptr;
        bool             active = false;
      } query;

      D3DDEVINFO_D3D9PIPELINETIMINGS
                 last_results;
    };

    extern SK_LazyGlobal <PipelineStatsD3D9> pipeline_stats_d3d9;
  }
}


using D3D9Device_Present_pfn = HRESULT (STDMETHODCALLTYPE *)(
           IDirect3DDevice9    *This,
_In_ const RECT                *pSourceRect,
_In_ const RECT                *pDestRect,
_In_       HWND                 hDestWindowOverride,
_In_ const RGNDATA             *pDirtyRegion);

using D3D9ExDevice_PresentEx_pfn = HRESULT (STDMETHODCALLTYPE *)(
           IDirect3DDevice9Ex  *This,
_In_ const RECT                *pSourceRect,
_In_ const RECT                *pDestRect,
_In_       HWND                 hDestWindowOverride,
_In_ const RGNDATA             *pDirtyRegion,
_In_       DWORD                dwFlags);

using D3D9Swap_Present_pfn = HRESULT (STDMETHODCALLTYPE *)(
             IDirect3DSwapChain9 *This,
  _In_ const RECT                *pSourceRect,
  _In_ const RECT                *pDestRect,
  _In_       HWND                 hDestWindowOverride,
  _In_ const RGNDATA             *pDirtyRegion,
  _In_       DWORD                dwFlags);

using D3D9_CreateDevice_pfn = HRESULT (STDMETHODCALLTYPE *)(
           IDirect3D9             *This,
           UINT                    Adapter,
           D3DDEVTYPE              DeviceType,
           HWND                    hFocusWindow,
           DWORD                   BehaviorFlags,
           D3DPRESENT_PARAMETERS  *pPresentationParameters,
           IDirect3DDevice9      **ppReturnedDeviceInterface);

using D3D9Ex_CreateDeviceEx_pfn = HRESULT (STDMETHODCALLTYPE *)(
           IDirect3D9Ex           *This,
           UINT                    Adapter,
           D3DDEVTYPE              DeviceType,
           HWND                    hFocusWindow,
           DWORD                   BehaviorFlags,
           D3DPRESENT_PARAMETERS  *pPresentationParameters,
           D3DDISPLAYMODEEX       *pFullscreenDisplayMode,
           IDirect3DDevice9Ex    **ppReturnedDeviceInterface);

using D3D9Device_Reset_pfn = HRESULT (STDMETHODCALLTYPE *)(
           IDirect3DDevice9      *This,
           D3DPRESENT_PARAMETERS *pPresentationParameters);

using D3D9ExDevice_ResetEx_pfn = HRESULT (STDMETHODCALLTYPE *)(
           IDirect3DDevice9Ex    *This,
           D3DPRESENT_PARAMETERS *pPresentationParameters,
           D3DDISPLAYMODEEX      *pFullscreenDisplayMode );

using D3D9Device_SetGammaRamp_pfn = void (STDMETHODCALLTYPE *)(
           IDirect3DDevice9      *This,
_In_       UINT                   iSwapChain,
_In_       DWORD                  Flags,
_In_ const D3DGAMMARAMP          *pRamp );

using D3D9Device_DrawPrimitive_pfn = HRESULT (STDMETHODCALLTYPE *)
  ( IDirect3DDevice9* This,
    D3DPRIMITIVETYPE  PrimitiveType,
    UINT              StartVertex,
    UINT              PrimitiveCount );

using D3D9Device_DrawIndexedPrimitive_pfn = HRESULT (STDMETHODCALLTYPE *)
  ( IDirect3DDevice9* This,
    D3DPRIMITIVETYPE  Type,
    INT               BaseVertexIndex,
    UINT              MinVertexIndex,
    UINT              NumVertices,
    UINT              startIndex,
    UINT              primCount );

using D3D9Device_DrawPrimitiveUP_pfn = HRESULT (STDMETHODCALLTYPE *)
  ( IDirect3DDevice9* This,
    D3DPRIMITIVETYPE  PrimitiveType,
    UINT              PrimitiveCount,
    const void       *pVertexStreamZeroData,
    UINT              VertexStreamZeroStride );

using D3D9Device_DrawIndexedPrimitiveUP_pfn = HRESULT (STDMETHODCALLTYPE *)
  ( IDirect3DDevice9* This,
    D3DPRIMITIVETYPE  PrimitiveType,
    UINT              MinVertexIndex,
    UINT              NumVertices,
    UINT              PrimitiveCount,
    const void       *pIndexData,
    D3DFORMAT         IndexDataFormat,
    const void       *pVertexStreamZeroData,
    UINT              VertexStreamZeroStride );

using D3D9Device_GetTexture_pfn = HRESULT (STDMETHODCALLTYPE *)
  (     IDirect3DDevice9      *This,
   _In_ DWORD                  Sampler,
  _Out_ IDirect3DBaseTexture9 *pTexture);

using D3D9Device_SetTexture_pfn = HRESULT (STDMETHODCALLTYPE *)
  (     IDirect3DDevice9      *This,
   _In_ DWORD                  Sampler,
   _In_ IDirect3DBaseTexture9 *pTexture);

using D3D9Device_SetSamplerState_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9*   This,
   DWORD               Sampler,
   D3DSAMPLERSTATETYPE Type,
   DWORD               Value);

using D3D9Device_SetViewport_pfn = HRESULT (STDMETHODCALLTYPE *)
  (      IDirect3DDevice9* This,
   CONST D3DVIEWPORT9*     pViewport);

using D3D9Device_SetRenderState_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9*  This,
   D3DRENDERSTATETYPE State,
   DWORD              Value);

using D3D9Device_SetVertexShaderConstantF_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9* This,
    UINT             StartRegister,
    CONST float*     pConstantData,
    UINT             Vector4fCount);

using D3D9Device_SetPixelShaderConstantF_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9* This,
    UINT             StartRegister,
    CONST float*     pConstantData,
    UINT             Vector4fCount);

struct IDirect3DPixelShader9;

using D3D9Device_SetPixelShader_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9*      This,
   IDirect3DPixelShader9* pShader);

struct IDirect3DVertexShader9;

using D3D9Device_SetVertexShader_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9*       This,
   IDirect3DVertexShader9* pShader);

using D3D9Device_SetScissorRect_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9* This,
   CONST RECT*       pRect);

using D3D9Device_CreateTexture_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9   *This,
   UINT                Width,
   UINT                Height,
   UINT                Levels,
   DWORD               Usage,
   D3DFORMAT           Format,
   D3DPOOL             Pool,
   IDirect3DTexture9 **ppTexture,
   HANDLE             *pSharedHandle);

struct IDirect3DSurface9;

using D3D9Device_CreateVertexBuffer_pfn = HRESULT (STDMETHODCALLTYPE *)
(
  _In_  IDirect3DDevice9        *This,
  _In_  UINT                     Length,
  _In_  DWORD                    Usage,
  _In_  DWORD                    FVF,
  _In_  D3DPOOL                  Pool,
  _Out_ IDirect3DVertexBuffer9 **ppVertexBuffer,
  _In_  HANDLE                  *pSharedHandle
);

using D3D9Device_SetStreamSource_pfn = HRESULT (STDMETHODCALLTYPE *)
(
  IDirect3DDevice9       *This,
  UINT                    StreamNumber,
  IDirect3DVertexBuffer9 *pStreamData,
  UINT                    OffsetInBytes,
  UINT                    Stride
);

using D3D9Device_SetStreamSourceFreq_pfn = HRESULT (STDMETHODCALLTYPE *)
( _In_ IDirect3DDevice9 *This,
  _In_ UINT              StreamNumber,
  _In_ UINT              FrequencyParameter
);

using D3D9Device_SetFVF_pfn = HRESULT (STDMETHODCALLTYPE *)
(
  IDirect3DDevice9 *This,
  DWORD             FVF
);

using D3D9Device_SetVertexDeclaration_pfn = HRESULT (STDMETHODCALLTYPE *)
(
  IDirect3DDevice9            *This,
  IDirect3DVertexDeclaration9 *pDecl
);

using D3D9Device_CreateRenderTarget_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9     *This,
   UINT                  Width,
   UINT                  Height,
   D3DFORMAT             Format,
   D3DMULTISAMPLE_TYPE   MultiSample,
   DWORD                 MultisampleQuality,
   BOOL                  Lockable,
   IDirect3DSurface9   **ppSurface,
   HANDLE               *pSharedHandle);

using D3D9Device_CreateDepthStencilSurface_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9     *This,
   UINT                  Width,
   UINT                  Height,
   D3DFORMAT             Format,
   D3DMULTISAMPLE_TYPE   MultiSample,
   DWORD                 MultisampleQuality,
   BOOL                  Discard,
   IDirect3DSurface9   **ppSurface,
   HANDLE               *pSharedHandle);

using D3D9Device_SetRenderTarget_pfn           = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9  *This,
   DWORD              RenderTargetIndex,
   IDirect3DSurface9 *pRenderTarget);

using D3D9Device_SetDepthStencilSurface_pfn    = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9  *This,
   IDirect3DSurface9 *pNewZStencil);

struct IDirect3DBaseTexture9;

using D3D9Device_UpdateTexture_pfn             = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9      *This,
   IDirect3DBaseTexture9 *pSourceTexture,
   IDirect3DBaseTexture9 *pDestinationTexture);

using D3D9Device_StretchRect_pfn               = HRESULT (STDMETHODCALLTYPE *)
  (      IDirect3DDevice9    *This,
         IDirect3DSurface9   *pSourceSurface,
   const RECT                *pSourceRect,
         IDirect3DSurface9   *pDestSurface,
   const RECT                *pDestRect,
         D3DTEXTUREFILTERTYPE Filter);

using D3D9Device_SetCursorPosition_pfn         = HRESULT (STDMETHODCALLTYPE *)
(
       IDirect3DDevice9 *This,
  _In_ INT               X,
  _In_ INT               Y,
  _In_ DWORD             Flags
);

using D3D9Device_TestCooperativeLevel_pfn      = HRESULT (STDMETHODCALLTYPE *)(IDirect3DDevice9* This);

using D3D9Device_BeginScene_pfn                = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9* This);

using D3D9Device_EndScene_pfn                  = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9* This);

using D3D9Device_CreateVertexDeclaration_pfn   = HRESULT (STDMETHODCALLTYPE *)
(
        IDirect3DDevice9             *This,
  CONST D3DVERTEXELEMENT9            *pVertexElements,
        IDirect3DVertexDeclaration9 **ppDecl
);

using D3D9Device_CreateOffscreenPlainSurface_pfn = HRESULT (STDMETHODCALLTYPE *)(
  _In_  IDirect3DDevice9   *This,
  _In_  UINT                Width,
  _In_  UINT                Height,
  _In_  D3DFORMAT           Format,
  _In_  D3DPOOL             Pool,
  _Out_ IDirect3DSurface9 **ppSurface,
  _In_  HANDLE             *pSharedHandle );



__declspec (noinline)
HRESULT
WINAPI
D3D9ExDevice_PresentEx ( IDirect3DDevice9Ex *This,
              _In_ const RECT               *pSourceRect,
              _In_ const RECT               *pDestRect,
              _In_       HWND                hDestWindowOverride,
              _In_ const RGNDATA            *pDirtyRegion,
              _In_       DWORD               dwFlags );

__declspec (noinline)
HRESULT
WINAPI
D3D9Device_Present ( IDirect3DDevice9 *This,
           _In_ const RECT             *pSourceRect,
           _In_ const RECT             *pDestRect,
           _In_       HWND              hDestWindowOverride,
           _In_ const RGNDATA          *pDirtyRegion );

__declspec (noinline)
HRESULT
WINAPI
D3D9Swap_Present ( IDirect3DSwapChain9 *This,
        _In_ const RECT                *pSourceRect,
        _In_ const RECT                *pDestRect,
        _In_       HWND                 hDestWindowOverride,
        _In_ const RGNDATA             *pDirtyRegion,
        _In_       DWORD                dwFlags );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9Reset_Override ( IDirect3DDevice9      *This,
                     D3DPRESENT_PARAMETERS *pPresentationParameters );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9ResetEx ( IDirect3DDevice9Ex    *This,
              D3DPRESENT_PARAMETERS *pPresentationParameters,
              D3DDISPLAYMODEEX      *pFullscreenDisplayMode );

__declspec (noinline)
HRESULT
WINAPI
D3D9CreateDevice_Override ( IDirect3D9*            This,
                            UINT                   Adapter,
                            D3DDEVTYPE             DeviceType,
                            HWND                   hFocusWindow,
                            DWORD                  BehaviorFlags,
                            D3DPRESENT_PARAMETERS* pPresentationParameters,
                            IDirect3DDevice9**     ppReturnedDeviceInterface );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9CreateAdditionalSwapChain_Override ( IDirect3DDevice9       *This,
                                         D3DPRESENT_PARAMETERS  *pPresentationParameters,
                                         IDirect3DSwapChain9   **pSwapChain );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9BeginScene_Override (IDirect3DDevice9 *This);

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9EndScene_Override (IDirect3DDevice9 *This);

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9TestCooperativeLevel_Override (IDirect3DDevice9 *This);

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9CreateDeviceEx_Override ( IDirect3D9Ex           *This,
                              UINT                    Adapter,
                              D3DDEVTYPE              DeviceType,
                              HWND                    hFocusWindow,
                              DWORD                   BehaviorFlags,
                              D3DPRESENT_PARAMETERS  *pPresentationParameters,
                              D3DDISPLAYMODEEX       *pFullscreenDisplayMode,
                              IDirect3DDevice9Ex    **ppReturnedDeviceInterface );


void
__stdcall
SK_D3D9_UpdateRenderStats (IDirect3DSwapChain9* pSwapChain, IDirect3DDevice9* pDevice = nullptr);

__declspec (noinline)
D3DPRESENT_PARAMETERS*
WINAPI
SK_SetPresentParamsD3D9 (IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pparams);

__declspec (noinline)
D3DPRESENT_PARAMETERS*
WINAPI
SK_SetPresentParamsD3D9Ex ( IDirect3DDevice9       *pDevice,
                            D3DPRESENT_PARAMETERS  *pparams,
                            D3DDISPLAYMODEEX      **ppFullscreenDisplayMode = nullptr );




using GetSwapChain_pfn              = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9* This, UINT iSwapChain, IDirect3DSwapChain9** pSwapChain);

using D3D9Device_CreateAdditionalSwapChain_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9* This, D3DPRESENT_PARAMETERS* pPresentationParameters,
   IDirect3DSwapChain9** pSwapChain);




std::string
SK_D3D9_UsageToStr (DWORD dwUsage);

std::string
SK_D3D9_FormatToStr (D3DFORMAT Format, bool include_ordinal = true);

const char*
SK_D3D9_PoolToStr (D3DPOOL pool);

std::string
SK_D3D9_SwapEffectToStr (D3DSWAPEFFECT Effect);

std::string
SK_D3D9_PresentParameterFlagsToStr (DWORD dwFlags);

void
SK_D3D9_TriggerReset (bool);

void
SK_ImGui_QueueResetD3D9 (void);

void
WINAPI
SK_HookD3D9 (void);

void
SK_D3D9_InitShaderModTools (void);


// Private API
//
unsigned int
__stdcall
HookD3D9 (LPVOID user);

unsigned int
__stdcall
HookD3D9Ex (LPVOID user);


HRESULT
WINAPI
SK_D3DXDisassembleShader (
const DWORD         *pShader, 
      BOOL           EnableColorCode, 
      LPCSTR         pComments, 
      LPD3DXBUFFER *ppDisassembly );

HRESULT
WINAPI
SK_D3DXGetShaderConstantTable (
const DWORD               *pFunction,
      LPD3DXCONSTANTTABLE *ppConstantTable );

HRESULT
WINAPI
SK_D3DXGetImageInfoFromFileInMemory (
      LPCVOID         pSrcData,
      UINT            SrcDataSize,
      D3DXIMAGE_INFO* pSrcInfo );

HRESULT
WINAPI
SK_D3DXSaveTextureToFile (
      LPCTSTR                pDestFile,
      D3DXIMAGE_FILEFORMAT   DestFormat,
      LPDIRECT3DBASETEXTURE9 pSrcTexture,
const PALETTEENTRY           *pSrcPalette );

HRESULT
WINAPI
SK_D3DXCreateTextureFromFileInMemoryEx (
  _In_        LPDIRECT3DDEVICE9  pDevice,
  _In_        LPCVOID            pSrcData,
  _In_        UINT               SrcDataSize,
  _In_        UINT               Width,
  _In_        UINT               Height,
  _In_        UINT               MipLevels,
  _In_        DWORD              Usage,
  _In_        D3DFORMAT          Format,
  _In_        D3DPOOL            Pool,
  _In_        DWORD              Filter,
  _In_        DWORD              MipFilter,
  _In_        D3DCOLOR           ColorKey,
  _Inout_opt_ D3DXIMAGE_INFO     *pSrcInfo,
  _Out_opt_   PALETTEENTRY       *pPalette,
  _Out_       LPDIRECT3DTEXTURE9 *ppTexture );


#endif /* __SK__D3D9_BACKEND_H__ */