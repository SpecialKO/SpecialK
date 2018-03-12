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

// Useless warning:  'typedef ': ignored on left of '' when no variable is declared
#pragma warning (disable: 4091)

#include <Windows.h>
#include <algorithm>

struct ID3D11RasterizerState;
struct ID3D11DepthStencilState;
struct ID3D11DepthStencilView;
struct ID3D11ShaderResourceView;

#include <unordered_map>

extern volatile LONG __SK_TLS_INDEX;

class SK_ModuleAddrMap
{
public:
  SK_ModuleAddrMap (void);

  bool contains (LPCVOID pAddr, HMODULE* phMod);
  void insert   (LPCVOID pAddr, HMODULE   hMod);

  void* pResolved = nullptr;
};


enum SK_TLS_STACK_MASK
{
  SK_TLS_RESERVED_BIT   = 0x01,

  SK_TLS_INPUT_BIT      = 0x02,
  SK_TLS_RENDER_BIT     = 0x04,
  SK_TLS_IMGUI_BIT      = 0x08,
  SK_TLS_TEX_INJECT_BIT = 0x10,

  SK_TLS_DEBUG_BIT      = 0x80,

  SK_TLS_DWORD_ALIGNED  = 0xFFFFFFFF
};

enum SK_TLS_CleanupReason_e
{
  Periodic = 1, // Periodic temporary buffer cleanup
  Unload   = 2  // TLS is being completely unloaded for this thread
};


// Low-level construct, encapsulates a TLS slot's kernel index
//   and a pointer to any allocated storage.
struct SK_TlsRecord {
  DWORD  dwTlsIdx;
  LPVOID lpvData;
};

SK_TlsRecord SK_GetTLS     (bool initialize = false);
void         SK_CleanupTLS (void);


template <typename _T>
class SK_TLS_LocalDataStore
{
public:
  _T*    alloc   (size_t needed, bool zero_fill = false)
  {
    if (data == nullptr || len < needed)
    {
      if (data != nullptr)
        _aligned_free (data);

      len  = std::max (len, needed);
      data = (_T *)_aligned_malloc (len * sizeof (_T), 16);

      if (data == nullptr)
        len = 0;
    }

    if (zero_fill)
      RtlZeroMemory (data, needed * sizeof (_T));

    return data;
  }

  size_t reclaim (void)
  {
    if (data != nullptr)
    {
      _aligned_free (data);
                     data = nullptr;

      size_t freed = len;
                     len = 0;

      return freed * sizeof (_T);
    }

    return 0;
  }

  bool   empty   (void) const { return data == nullptr || len == 0; }

//protected:
  _T*    data = nullptr;
  size_t len  = 0;
};



class SK_TLS_DynamicContext
{
public:
  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload);
};

class SK_TLS_ScratchMemory : public SK_TLS_DynamicContext
{
public:
  SK_TLS_ScratchMemory (void) = default;

  SK_TLS_LocalDataStore <char> cmd;
  SK_TLS_LocalDataStore <char> eula;

  struct
  {
    SK_TLS_LocalDataStore <wchar_t> val;
    SK_TLS_LocalDataStore <wchar_t> key;
    SK_TLS_LocalDataStore <wchar_t> sec;
  } ini;

  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload);
};


class SK_TLS_RenderContext
{
public:
  BOOL ctx_init_thread = FALSE;
};

class SK_D3D9_ThreadContext : public SK_TLS_DynamicContext,
                              public SK_TLS_RenderContext
{
public:
  LPVOID temp_fullscreen = nullptr;

  struct scratch_mem_s
  {
    LPVOID   storage = nullptr;
    uint32_t size    = 0;
  } stack_scratch;

  void* allocStackScratchStorage   (size_t size);
  void* allocTempFullscreenStorage (size_t D3DDISPLAYMODEEX_is_always_24_bytes = 24);

  // Needed to safely override D3D9Ex fullscreen mode during device
  //   creation

  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload);
};

class SK_D3D8_ThreadContext : public SK_TLS_RenderContext
{
};

class SK_DDraw_ThreadContext : public SK_TLS_RenderContext
{
};

struct SK_D3D11_Stateblock_Lite;

class SK_D3D11_ThreadContext : public SK_TLS_DynamicContext,
                               public SK_TLS_RenderContext
{
public:
  ID3D11RasterizerState*   pRasterStateOrig       = nullptr;
  ID3D11RasterizerState*   pRasterStateNew        = nullptr;

  ID3D11DepthStencilState* pDepthStencilStateOrig = nullptr;
  ID3D11DepthStencilState* pDepthStencilStateNew  = nullptr;
  ID3D11DepthStencilView*  pDSVOrig               = nullptr;

  UINT                     StencilRefOrig         = 0;
  UINT                     StencilRefNew          = 0;

  ID3D11ShaderResourceView* newResourceViews [128];

  SK_D3D11_Stateblock_Lite* stateBlock            = nullptr;
  size_t                    stateBlockSize        = 0;

  SK_D3D11_Stateblock_Lite* getStateBlock (void);

  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload);
};

class SK_GL_ThreadContext : public SK_TLS_RenderContext
{
public:
  HGLRC current_hglrc = 0;
  HDC   current_hdc   = 0;
  HWND  current_hwnd  = 0;
};


class SK_RawInput_ThreadContext : public SK_TLS_DynamicContext
{
public:
  uint8_t* allocData (size_t needed);

  void*  data     = nullptr;
  size_t capacity = 0UL;

  RAWINPUTDEVICE* allocateDevices (size_t needed);

  RAWINPUTDEVICE* devices     = nullptr;
  size_t          num_devices = 0UL;

  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload);
};

class SK_Input_ThreadContext
{
public:
  BOOL hid                 = FALSE;
  BOOL ctx_init_thread     = FALSE;
};


class SK_Win32_ThreadContext
{
public:
  int  getThreadPriority (bool nocache = false);

  LONG GUI                 = -1;

  int  thread_prio         =  0;

  struct
  {
    DWORD time             = 0;
    ULONG frame            = 0;
  } last_tested_prio;
};

class SK_ImGui_ThreadContext : public SK_TLS_DynamicContext
{
public:
  BOOL drawing             = FALSE;

  // Allocates and grows this buffer until a cleanup operation demands
  //   we shrink it.
  void* allocPolylineStorage (size_t needed);

  void*  polyline_storage  = nullptr;
  size_t polyline_capacity = 0UL;

  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload);
};

class SK_OSD_ThreadContext : public SK_TLS_DynamicContext
{
public:
  // Allocates and grows this buffer until a cleanup operation demands
  //   we shrink it.
  char* allocText (size_t needed);

  char*  text          = nullptr;
  size_t text_capacity = 0;

  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload);
};

class SK_Steam_ThreadContext : public SK_TLS_DynamicContext
{
public:
  int32_t client_pipe = 0;
  int32_t client_user = 0;

  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload);
};


class SK_DInput7_ThreadContext
{
public:
  HRESULT hr_GetDevicestate;
};

class SK_DInput8_ThreadContext
{
public:
  HRESULT hr_GetDevicestate;
};


class SK_TLS
{
public:
  SK_ModuleAddrMap          known_modules;
  SK_TLS_ScratchMemory      scratch_memory;

  SK_DDraw_ThreadContext    ddraw;
  SK_D3D8_ThreadContext     d3d8;
  SK_D3D9_ThreadContext     d3d9;
  SK_D3D11_ThreadContext    d3d11;
  SK_GL_ThreadContext       gl;

  SK_DInput7_ThreadContext  dinput7;
  SK_DInput8_ThreadContext  dinput8;

  SK_ImGui_ThreadContext    imgui;
  SK_Input_ThreadContext    input_core;
  SK_RawInput_ThreadContext raw_input;
  SK_Win32_ThreadContext    win32;

  SK_OSD_ThreadContext      osd;
  SK_Steam_ThreadContext    steam;

  // All stack frames except for bottom
  //   have meaningless values for these,
  //
  //  >> Always access through SK_TLS_Bottom <<
  //
  struct
  {
    CONTEXT          last_ctx    = {   };
    EXCEPTION_RECORD last_exc    = {   };
    bool             last_chance = false;
    bool             in_DllMain  = false;
    wchar_t          name [256]  = {   };
  } debug;

  struct tex_mgmt_s
  {
    struct stream_pool_s
    {
      void*    data          = nullptr;
      size_t   data_len      = 0;
      uint32_t data_age      = 0;
    } streaming_memory;

    BOOL injection_thread    = FALSE;

    IUnknown* refcount_obj   = nullptr; // Object to expect a reference count change on
    LONG      refcount_test  = 0;       // Used to validate 3rd party D3D texture wrappers
  } texture_management;


  struct stack
  {
                 int current = 0;
    static const int max     = 2;
  } stack;

  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload);
};

extern volatile LONG __SK_TLS_INDEX;

SK_TLS* __stdcall SK_TLS_Get    (void); // Alias: SK_TLS_Top
SK_TLS* __stdcall SK_TLS_Bottom (void);

class SK_ScopedBool
{
public:
  SK_ScopedBool (BOOL* pBool)
  {
    pBool_ =  pBool;
    bOrig_ = *pBool;
  }

  ~SK_ScopedBool (void)
  {
    *pBool_ = bOrig_;
  }

private:
  BOOL* pBool_;
  BOOL  bOrig_;
};