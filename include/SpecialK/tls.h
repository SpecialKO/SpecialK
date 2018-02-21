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

struct ID3D11RasterizerState;
struct ID3D11DepthStencilState;
struct ID3D11DepthStencilView;

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




struct SK_TLS_DynamicContext
{
  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload);
};

struct SK_TLS_ScratchMemory : SK_TLS_DynamicContext
{
  struct cmd_s
  {
    char*  allocFormattedStorage (size_t needed_len);
  
    char*  line = nullptr;
    size_t len  = 0;
  } cmd;
  
  struct eula_s
  {
    char*  allocTextStorage (size_t needed_len);
  
    char*  text = nullptr;
    size_t len  = 0;
  } eula;

  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload);
};


struct SK_TLS_RenderContext
{
  BOOL ctx_init_thread = FALSE;
};

struct SK_D3D9_ThreadContext : SK_TLS_DynamicContext,
                               SK_TLS_RenderContext
{
  LPVOID temp_fullscreen = nullptr;

  void* allocTempFullscreenStorage (size_t D3DDISPLAYMODEEX_is_always_24_bytes = 24);

  // Needed to safely override D3D9Ex fullscreen mode during device
  //   creation

  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload);
};

struct SK_D3D8_ThreadContext : SK_TLS_RenderContext
{
};

struct SK_DDraw_ThreadContext : SK_TLS_RenderContext
{
};

struct SK_D3D11_ThreadContext : SK_TLS_RenderContext
{
  ID3D11RasterizerState*   pRasterStateOrig       = nullptr;
  ID3D11RasterizerState*   pRasterStateNew        = nullptr;

  ID3D11DepthStencilState* pDepthStencilStateOrig = nullptr;
  ID3D11DepthStencilState* pDepthStencilStateNew  = nullptr;
  ID3D11DepthStencilView*  pDSVOrig               = nullptr;

  UINT                     StencilRefOrig         = 0;
  UINT                     StencilRefNew          = 0;
};

struct SK_GL_ThreadContext : SK_TLS_RenderContext
{
  HGLRC current_hglrc = 0;
  HDC   current_hdc   = 0;
  HWND  current_hwnd  = 0;
};


struct SK_RawInput_ThreadContext : SK_TLS_DynamicContext
{
  uint8_t* allocData (size_t needed);

  void*  data     = nullptr;
  size_t capacity = 0UL;

  RAWINPUTDEVICE* allocateDevices (size_t needed);

  RAWINPUTDEVICE* devices     = nullptr;
  size_t          num_devices = 0UL;

  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload);
};

struct SK_Input_ThreadContext
{
  BOOL hid                 = FALSE;
  BOOL ctx_init_thread     = FALSE;
};


struct SK_Win32_ThreadContext
{
  int  getThreadPriority (bool nocache = false);

  LONG GUI                 = -1;

  int  thread_prio         =  0;

  struct
  {
    DWORD time             = 0;
    ULONG frame            = 0;
  } last_tested_prio;
};

struct SK_ImGui_ThreadContext : SK_TLS_DynamicContext
{
  BOOL drawing             = FALSE;

  // Allocates and grows this buffer until a cleanup operation demands
  //   we shrink it.
  void* allocPolylineStorage (size_t needed);

  void*  polyline_storage  = nullptr;
  size_t polyline_capacity = 0UL;

  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload);
};

struct SK_OSD_ThreadContext : SK_TLS_DynamicContext
{
  // Allocates and grows this buffer until a cleanup operation demands
  //   we shrink it.
  char* allocText (size_t needed);

  char*  text          = nullptr;
  size_t text_capacity = 0;

  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload);
};

struct SK_Steam_ThreadContext : SK_TLS_DynamicContext
{
  int32_t client_pipe = 0;
  int32_t client_user = 0;

  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload);
};


struct SK_TLS
{
  SK_ModuleAddrMap          known_modules;
  SK_TLS_ScratchMemory      scratch_memory;

  SK_DDraw_ThreadContext    ddraw;
  SK_D3D8_ThreadContext     d3d8;
  SK_D3D9_ThreadContext     d3d9;
  SK_D3D11_ThreadContext    d3d11;
  SK_GL_ThreadContext       gl;

//SK_DInput7_ThreadContext   dinput7;
//SK_DInput8_ThreadContext   dinput8;

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