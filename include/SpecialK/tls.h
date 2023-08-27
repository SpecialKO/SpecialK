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

class SK_TLS;

//SK_TLS* __cdecl SK_TLS_Get      (void); // Alias: SK_TLS_Top
extern SK_TLS* SK_TLS_Bottom   (void);
extern SK_TLS* SK_TLS_BottomEx (DWORD dwTid);

#include <Windows.h>

#undef _WINGDI_
#define NOGDI
#include <comdef.h>
#include <Unknwnbase.h>
#include <atlcomcli.h>

#include <stack>
#include <unordered_map>

#include <SpecialK/thread.h>
#include <SpecialK/com_util.h>
#include <SpecialK/input/input.h>

#include <assert.h>

// Not so global in this case, but the concept remains the same;
//   deferred initialization until first use of complicated objects.
#include <SpecialK/utility/lazy_global.h>

#include <vcruntime_exception.h>

struct SK_MMCS_TaskEntry;

#include <SpecialK/render/d3d11/d3d11_interfaces.h>

#ifndef _D3D11_CONSTANTS
#define	D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT	( 15 )
#endif


#define MAX_THREAD_NAME_LEN MAX_PATH

class SK_ModuleAddrMap
{
public:
  SK_ModuleAddrMap (void);

  bool contains (LPCVOID pAddr, HMODULE* phMod);
  void insert   (LPCVOID pAddr, HMODULE   hMod);

  void* pResolved = nullptr;
};

void SK_TLS_LogLeak ( const wchar_t* wszFunc,
                      const wchar_t* wszFile,
                            int      line,
                            size_t   size ) noexcept;

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


class SK_TLS;

// Low-level construct, encapsulates a TLS slot's kernel index
//   and a pointer to any allocated storage.
struct SK_TlsRecord {
  DWORD   dwTlsIdx = std::numeric_limits <DWORD>::max ();
  SK_TLS *pTLS     = nullptr;
};

SK_TlsRecord* SK_GetTLS     (SK_TLS** ppTLS);
SK_TLS*       SK_CleanupTLS (void);

template <typename _T>
class SK_TLS_HeapDataStore
{
public:
  _T*    alloc   (size_t needed, bool zero_fill = false)
  {
    if (data == nullptr || len < needed)
    {
      if (data != nullptr && len > 0)
        _aligned_free (data);

      len  = std::max (len, needed);
      data = static_cast <_T *> (
        _aligned_malloc (len * sizeof (_T), 16)
      );

      if (data == nullptr)
        len = 0;
    }

    if (zero_fill && data != nullptr)
      memset (data, 0, needed * sizeof (_T));

    return data;
  }

  size_t reclaim (void)
  {
    if (len > 0)
    {
      if (data != nullptr)
      {
        size_t freed = len;
                 len = 0;

        _aligned_free (data);
                       data = nullptr;

        return freed * sizeof (_T);
      }

      else
      {
        SK_TLS_LogLeak ( __FUNCTIONW__, __FILEW__, __LINE__, len );

               len = 0;
        return len;
      }
    }

    return 0;
  }

  bool   empty   (void) const { return data == nullptr || len == 0; }

//protected:
  _T*    data = nullptr;
  size_t len  = 0;
};


HLOCAL
WINAPI
SK_LocalFree       (
  _In_ HLOCAL hMem ) noexcept;

HLOCAL
WINAPI
SK_LocalAlloc (
  _In_ UINT   uFlags,
  _In_ SIZE_T uBytes ) noexcept;

template <typename _T>
class SK_TLS_LocalDataStore
{
public:
  _T*    alloc   (size_t needed, bool zero_fill = false)
  {
    if (data == nullptr || len < needed)
    {
      if (data != nullptr)
        SK_LocalFree (static_cast <HLOCAL> (data));

      const UINT
        uFlags =
          ( zero_fill ?
                 LPTR : LMEM_FIXED );

      len  = std::max (len, needed);
      data = static_cast <_T *>(
        SK_LocalAlloc (uFlags, len * sizeof (_T))
      );

      if (data == nullptr)
        len = 0;
    }

    // This allocation was a NOP, but it is still
    //   expected that we return a zero-filled buffer
    else if (zero_fill)
      RtlSecureZeroMemory (data, needed * sizeof (_T));

    return data;
  }

  size_t reclaim (void)
  {
    if (len > 0)
    {
      if (data != nullptr)
      {
        SK_LocalFree (data);
                      data = nullptr;

        size_t freed = len;
                       len = 0;

        return freed * sizeof (_T);
      }

      else
      {
        SK_TLS_LogLeak ( __FUNCTIONW__, __FILEW__, __LINE__, len );

               len = 0;
        return len;
      }
    }

    else assert (data == nullptr);

    return 0;
  }

  bool   empty   (void) const { return data == nullptr || len == 0; }

//protected:
  _T*    data = nullptr;
  size_t len  = 0;
};


// CCD Forward Declarations, sort of (lol), to avoid GDI header pollution
using  SK_MINGDI_DISPLAYCONFIG_PATH_INFO = uint8_t [72]; // sizeof (DISPLAYCONFIG_PATH_INFO);
using  SK_MINGDI_DISPLAYCONFIG_MODE_INFO = uint8_t [64]; // sizeof (DISPLAYCONFIG_MODE_INFO);


class SK_TLS_DynamicContext
{
public:
  size_t virtual Cleanup                (SK_TLS_CleanupReason_e reason = Unload);

                  SK_TLS_DynamicContext (void) { };
         virtual ~SK_TLS_DynamicContext (void) { };
};

class SK_TLS_ScratchMemory : public SK_TLS_DynamicContext
{
public:
  SK_TLS_ScratchMemory (void) = default;

  SK_TLS_HeapDataStore <char> cmd;
  SK_TLS_HeapDataStore <char> sym_resolve;
  SK_TLS_HeapDataStore <char> eula;
  SK_TLS_HeapDataStore <char> cpu_info;

  struct
  {
    SK_TLS_HeapDataStore <wchar_t> val;
    SK_TLS_HeapDataStore <wchar_t> key;
    SK_TLS_HeapDataStore <wchar_t> sec;
  } ini;

  struct
  {
    SK_TLS_HeapDataStore <wchar_t> formatted_output;
  } log;

  struct {
    SK_TLS_HeapDataStore <DXGI_MODE_DESC> mode_list;
  } dxgi;

  struct {
    std::stack <DPI_AWARENESS_CONTEXT> dpi_ctx_stack;
  } dpi;

  struct {
    SK_TLS_HeapDataStore <wchar_t> wszTemperature;
    SK_TLS_HeapDataStore <char>     szTemperature;
    SK_TLS_HeapDataStore <wchar_t>    wszFileSize;
    SK_TLS_HeapDataStore <char>        szFileSize;
  } osd;

  struct {
    SK_TLS_HeapDataStore <SK_MINGDI_DISPLAYCONFIG_PATH_INFO> display_paths;
    SK_TLS_HeapDataStore <SK_MINGDI_DISPLAYCONFIG_MODE_INFO> display_modes;
  } ccd;

  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload) override;
};

struct SK_NtQuerySystemInformation {
  SK_TLS_LocalDataStore <BYTE> NtInfo   = { };
  LONG                         NtStatus =  0 ;
};

class SK_TLS_ScratchMemoryLocal : public SK_TLS_DynamicContext
{
public:
  SK_TLS_ScratchMemoryLocal (void) = default;

  SK_NtQuerySystemInformation query [3] = { };

  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload) override;
};


class SK_TLS_RenderContext
{
public:
  BOOL ctx_init_thread    = FALSE;
  BOOL ctx_init_thread_ex = FALSE;
};

class SK_D3D9_ThreadContext : public SK_TLS_DynamicContext,
                              public SK_TLS_RenderContext
{
public:
  LPVOID temp_fullscreen = nullptr;

  struct scratch_mem_s
  {
    LPVOID storage = nullptr;
    size_t size    = 0;

    size_t reclaim (void)
    {
      if (size > 0)
      {
        if (storage != nullptr)
        {
          _aligned_free (storage);

          const size_t orig_size = size;
                            size = 0;

          return orig_size;
        }

        else
        {
          SK_TLS_LogLeak ( __FUNCTIONW__, __FILEW__, __LINE__, size );

                 size = 0;
          return size;
        }
      }

      else assert (storage == nullptr);

      return 0;
    }
  } stack_scratch;

  void* allocStackScratchStorage   (size_t size);
  void* allocTempFullscreenStorage (size_t D3DDISPLAYMODEEX_is_always_24_bytes = 24);

  // Needed to safely override D3D9Ex fullscreen mode during device
  //   creation

  size_t virtual Cleanup (SK_TLS_CleanupReason_e reason = Unload);
};

class SK_D3D8_ThreadContext : public SK_TLS_RenderContext
{
};

class SK_DDraw_ThreadContext : public SK_TLS_RenderContext
{
};

class SK_D3D11_ThreadContext : public SK_TLS_DynamicContext,
                               public SK_TLS_RenderContext
{
public:
  SK_ComPtr <ID3D11DeviceContext>    pDevCtx;

  SK_ComPtr <ID3D11RasterizerState>  pRasterStateOrig;
  SK_ComPtr <ID3D11RasterizerState>  pRasterStateNew;

  SK_ComPtr <ID3D11DepthStencilState>pDepthStencilStateOrig;
  SK_ComPtr <ID3D11DepthStencilState>pDepthStencilStateNew;
  SK_ComPtr <ID3D11DepthStencilView> pDSVOrig;

  SK_ComPtr <ID3D11BlendState>       pOrigBlendState;

  UINT                               uiOrigBlendMask          = 0x0;
  FLOAT                              fOrigBlendFactors [4]    =
                                       { 0.0f, 0.0f, 0.0f, 0.0f };

  UINT                               StencilRefOrig           = 0;
  UINT                               StencilRefNew            = 0;

  SK_ComPtr <ID3D11Buffer>           pOriginalCBuffers [6]
        [D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT]
                                                              = { };
  bool                               empty_cbuffers    [6]    = { false };

  // Prevent recursion during hook installation
  BOOL                               skip_d3d11_create_device = FALSE;

  // For ImGui, mostly
  SK_LazyGlobal <std::vector <BYTE>> state_block;

  struct {
    uint8_t* buffer  = nullptr;
    size_t   reserve = 0UL;
  } screenshot;

  uint8_t* allocScreenshotMemory (size_t bytesNeeded);

  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload) override;
};

class SK_D3D12_ThreadContext : public SK_TLS_RenderContext
{
};

class SK_GL_ThreadContext : public SK_TLS_RenderContext
{
public:
  HGLRC current_hglrc = nullptr;
  HDC   current_hdc   = nullptr;
  HWND  current_hwnd  = nullptr;
};

class SK_Render_ThreadContext : SK_TLS_DynamicContext
{
public:
  volatile ULONG64                       frames_presented = 0;

  SK_LazyGlobal <SK_DDraw_ThreadContext> ddraw;
  SK_LazyGlobal <SK_D3D8_ThreadContext>  d3d8;
  SK_LazyGlobal <SK_D3D9_ThreadContext>  d3d9;
  SK_LazyGlobal <SK_D3D11_ThreadContext> d3d11;
  SK_LazyGlobal <SK_D3D12_ThreadContext> d3d12;
  SK_LazyGlobal <SK_GL_ThreadContext>    gl;

  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload) override;
};


class SK_DXTex_ThreadContext : public SK_TLS_DynamicContext
{
public:
  // Generally SK already 16-byte aligns most things for SIMD -- DXTex
  //   coincidentally NEEDS said alignment, so let's make this explicit.
  uint8_t* alignedAlloc (size_t alignment, size_t elems);
  bool     tryTrim      (void);
  void     moveAlloc    (void);

  size_t Cleanup (SK_TLS_CleanupReason_e) override;

private:
  uint8_t* buffer  = nullptr;
  size_t   reserve = 0UL;
//size_t   slack   = 0UL;

  // We'll compact the address space occasionally otherwise texture
  //   processing can destroy a 32-bit game's ability to function.
  DWORD last_realloc = 0,
        last_trim    = 0;

#ifdef _WIN64
  // Once every (idle) thirty-seconds, compact DXTex's scratch space
  static const DWORD  _TimeBetweenTrims =           30000UL;
  static const SIZE_T _SlackSpace       = ( 16384UL << 10UL ); // 16 MiB per-thread
#else
  static const DWORD  _TimeBetweenTrims =           10000UL;
  static const SIZE_T _SlackSpace       = ( 4096UL << 10UL ); //  4 MiB per-thread

#endif
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

  HRAWINPUT       last_input  = nullptr;

  struct {
    HRAWINPUT hRawInput  =   0;
    UINT      uiCommand  =   0;
    RAWINPUT  Data [32]  = { };
    UINT      Size       =   0;
    UINT      SizeHeader =   0;
  }               cached_input;

  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload) override;
};

class SK_Input_ThreadContext
{
public:
  BOOL                      hid             = FALSE;
  BOOL                      ctx_init_thread = FALSE;
  SK_ImGui_InputLanguage_s  input_language  = {   };
};


class SK_Win32_ThreadContext
{
public:
  int  getThreadPriority (bool nocache = false);

  struct
  {
    FILETIME last_time     = { };
    LPVOID   call_site     = nullptr;
    DWORD    code          = NO_ERROR;
  } error_state;

  HWND last_active         = reinterpret_cast <HWND> (-1);
  HWND active              = reinterpret_cast <HWND> (-1);
  LONG GUI                 =    -1;

  // Cached view
  std::pair <HWND, BOOL>
       unicode;

  bool mmcs_task           = false;

  int  thread_prio         =     0;

  struct
  {
    DWORD   time           = 0;
    ULONG64 frame          = 0;
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

  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload) override;
};

class SK_Steam_ThreadContext : public SK_TLS_DynamicContext
{
public:
  // Allocates and grows this buffer until a cleanup operation demands
  //   we shrink it.
  wchar_t* allocScratchText (size_t needed);

  wchar_t*  text          = nullptr;
  size_t    text_capacity = 0;

  int32_t client_pipe = 0;
  int32_t client_user = 0;

  volatile LONG64 callback_count = 0;

  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload) override;
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


class SK_Memory_ThreadContext
{
public:
  volatile LONG64 virtual_bytes = 0ULL;
  volatile LONG64 heap_bytes    = 0ULL;
  volatile LONG64 global_bytes  = 0ULL;
  volatile LONG64 local_bytes   = 0ULL;

  BOOL   allocating_virtual = FALSE;
  BOOL   allocating_heap    = FALSE;
  BOOL   allocating_local   = FALSE;
  BOOL   allocating_global  = FALSE;
};

class SK_Disk_ThreadContext
{
public:
  volatile LONG64 bytes_read        = 0ULL;
  volatile LONG64 bytes_written     = 0ULL;

           BOOL   ignore_reads      = FALSE;
           BOOL   ignore_writes     = FALSE;

           HANDLE last_file_read    = INVALID_HANDLE_VALUE;
           HANDLE last_file_written = INVALID_HANDLE_VALUE;
};

class SK_Net_ThreadContext
{
public:
  volatile LONG64 bytes_sent     = 0LL;
  volatile LONG64 bytes_received = 0LL;
};


class SK_Sched_ThreadContext : public SK_TLS_DynamicContext
{
public:
  DWORD         priority      = THREAD_PRIORITY_NORMAL;
  DWORD_PTR     affinity_mask = static_cast <DWORD_PTR> (-1);
  bool          lock_affinity = false;
  bool          background_io = false;
  SK_MMCS_TaskEntry*
                mmcs_task     = nullptr;

  ULONG         sleep0_count  = 0UL;
  ULONG64       last_frame    = 0ULL;
  ULONG         switch_count  = 0UL;

  volatile
    LONG          alert_waits = 0L;

  struct wait_record_s {
    LONG          calls        = 0L;
    LONG          time         = 0L;
    volatile
      LONG64      time_blocked = 0LL;
  };

  std::unordered_map <HANDLE, wait_record_s>*
    objects_waited = { };

  struct most_recent_wait_s
  {
    HANDLE        handle      = INVALID_HANDLE_VALUE;
    LARGE_INTEGER start       = LARGE_INTEGER { 0LL, 0LL };
    LARGE_INTEGER last_wait   = LARGE_INTEGER { 0LL, 0LL };
    LONG          sequence    =                      0UL;
    BOOL          preemptive  =                    FALSE;

    float getRate (void);
  } mru_wait;

public:
  size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload) override;
};


class SK_TLS
{
public:
  SK_TLS (DWORD idx) {
    Init (idx);
  }

  virtual ~SK_TLS (void) noexcept
  {
    if (debug.handle.isValid ())
        debug.handle.Close ();

    // Cleanup ();
  }

  void Init (DWORD idx)
  {
    context_record.dwTlsIdx = idx;
    context_record.pTLS     = this;

    // Try to grab a reference to the existing thread handle (w/ ALL_ACCESS) first
    HANDLE hThread =
      OpenThread (THREAD_ALL_ACCESS, FALSE, SK_Thread_GetCurrentId ());

    if (hThread != 0)
      debug.handle.Attach (hThread);

    // If that fails, duplicate the thread handle with greater access
    if (! debug.handle.isValid ())
    {
      if (! DuplicateHandle ( SK_GetCurrentProcess (), SK_GetCurrentThread  (),
                              SK_GetCurrentProcess (), &debug.handle.m_h,
                              THREAD_ALL_ACCESS,       FALSE,
                              0x0
                            )
         )
      {
        debug.handle.m_h =
          INVALID_HANDLE_VALUE;
      }
    }

    debug.tid     = SK_Thread_GetCurrentId ();
    debug.tls_idx = idx;
  }

  SK_TlsRecord                              context_record = { };

  SK_LazyGlobal <SK_ModuleAddrMap>          known_modules;
  SK_LazyGlobal <SK_TLS_ScratchMemory>      scratch_memory;
  SK_LazyGlobal <SK_TLS_ScratchMemoryLocal> local_scratch; // Takes memory from LocalAlloc

  SK_LazyGlobal <SK_Render_ThreadContext>   render;

  // Scratch memory pool for DXTex to reduce its tendency to fragment the
  //   the address space up while batching multiple format conversion jobs.
  SK_DXTex_ThreadContext                    dxtex   = { };

  SK_LazyGlobal <SK_DInput7_ThreadContext>  dinput7;
  SK_LazyGlobal <SK_DInput8_ThreadContext>  dinput8;

  SK_LazyGlobal <SK_ImGui_ThreadContext>    imgui;
  SK_LazyGlobal <SK_Input_ThreadContext>    input_core;
  SK_LazyGlobal <SK_RawInput_ThreadContext> raw_input;
  SK_LazyGlobal <SK_Win32_ThreadContext>    win32;

  SK_LazyGlobal <SK_Steam_ThreadContext>    steam;

  SK_LazyGlobal <SK_Sched_ThreadContext>    scheduler;

                SK_Memory_ThreadContext    _memory = { };
                SK_Memory_ThreadContext*    memory =
                                          &_memory;

  SK_LazyGlobal <SK_Disk_ThreadContext>     disk;
  SK_LazyGlobal <SK_Net_ThreadContext>      net;

  // All stack frames except for bottom
  //   have meaningless values for these,
  //
  //  >> Always access through SK_TLS_Bottom <<
  //
  struct
  {
    using callsite_list_t =
      std::unordered_set <void *>;

    CONTEXT          last_ctx          = {   };
    EXCEPTION_RECORD last_exc          = {   };
    LONG             exception_repeats =     0;
    callsite_list_t  suppressed_addrs  = {   };
    wchar_t          name
         [MAX_THREAD_NAME_LEN]         = {   };
    SK_AutoHandle    handle            =
      SK_AutoHandle (INVALID_HANDLE_VALUE);
    DWORD            tls_idx           =     0;
    DWORD            tid               =     0;
    ULONG            last_frame        = sk::narrow_cast <ULONG>(-1);
    volatile LONG    exceptions        =     0;
    bool             hidden            = false;
    bool             silent_exceptions = false;
    bool             mapped            = false;
    bool             last_chance       = false;
    bool             in_DllMain        = false;
    bool             naming            = false; // Don't recursively set thread names
  } debug;

  struct tex_mgmt_s
  {
    struct stream_pool_s
    {
      uint8_t* data          = nullptr;
      size_t   data_len      = 0;
      uint32_t data_age      = 0;

      struct heap_s {
        HANDLE primary       = INVALID_HANDLE_VALUE;
        HANDLE temporary     = INVALID_HANDLE_VALUE;

        ~heap_s (void)
        {
          if (primary   != INVALID_HANDLE_VALUE) HeapDestroy (std::exchange (primary,   INVALID_HANDLE_VALUE));
          if (temporary != INVALID_HANDLE_VALUE) HeapDestroy (std::exchange (temporary, INVALID_HANDLE_VALUE));
        }
      } heap;
    } streaming_memory;

    IUnknown* refcount_obj     = nullptr; // Object to expect a reference count change on
    LONG      refcount_test    = 0;       // Used to validate 3rd party D3D texture wrappers
    BOOL      injection_thread = false;
  } texture_management;


  // Pending removal
  //struct stack
  //{
  //               int current = 0;
  //  static const int max     = 1;
  //} stack;

  virtual size_t Cleanup (SK_TLS_CleanupReason_e reason = Unload);
};

class SK_ScopedBool
{
public:
  SK_ScopedBool (BOOL *pBool) noexcept : pBool_ ( pBool),
                                         bOrig_ ( pBool != nullptr ?
                                                            *pBool : false ) { };

  SK_ScopedBool (std::pair <BOOL *, BOOL> pair) noexcept : pBool_ (pair.first),
                                                           bOrig_ (pair.second) { };

  SK_ScopedBool            (const SK_ScopedBool& ) = delete;
  SK_ScopedBool            (      SK_ScopedBool&&) = delete;
  SK_ScopedBool& operator= (const SK_ScopedBool& ) = delete;
  SK_ScopedBool& operator= (      SK_ScopedBool&&) = delete;

  ~SK_ScopedBool (void) noexcept
  {
    if (pBool_ != nullptr)
       *pBool_ = bOrig_;
  }

  BOOL* getDestPtr (void) noexcept { return pBool_; }

private:
  BOOL* pBool_ = nullptr;
  BOOL  bOrig_ = false;
};



class SK_ScopedBoolFwd
{
public:
  SK_ScopedBoolFwd (std::pair <BOOL*, BOOL> args) noexcept : args_ (args) { };


  operator std::pair <BOOL*, BOOL> (void) noexcept { return args_; };

  std::pair <BOOL *, BOOL> args_;
};

extern volatile LONG _SK_IgnoreTLSAlloc;
extern volatile LONG _SK_IgnoreTLSMap;
