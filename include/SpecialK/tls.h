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

// Useless warninn:  'typedef ': ignored on left of '' when no variable is declared
#pragma warning (disable: 4091)

#include <Windows.h>

#define _NO_CVCONST_H
#define _IMAGEHLP_SOURCE_
#include <dbghelp.h>

struct ID3D11RasterizerState;
struct ID3D11DepthStencilState;
struct ID3D11DepthStencilView;

#include <unordered_map>

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

struct SK_TLS
{
  SK_ModuleAddrMap known_modules;

  struct tex_mgmt_s
  {
    struct stream_pool_s
    {
      void*    data     = nullptr;
      size_t   data_len = 0;
      uint32_t data_age = 0;
    } streaming_memory;

    BOOL injection_thread = FALSE;

    IUnknown* refcount_obj  = nullptr; // Object to expect a reference count change on
    LONG      refcount_test = 0;       // Used to validate 3rd party D3D texture wrappers
  } texture_management;

  struct {
    ID3D11RasterizerState*   pRasterStateOrig       = nullptr;
    ID3D11RasterizerState*   pRasterStateNew        = nullptr;

    ID3D11DepthStencilState* pDepthStencilStateOrig = nullptr;
    ID3D11DepthStencilState* pDepthStencilStateNew  = nullptr;
    ID3D11DepthStencilView*  pDSVOrig               = nullptr;

    UINT                     StencilRefOrig         = 0;
    UINT                     StencilRefNew          = 0;
  } d3d11;

  struct
  {
    HGLRC current_hglrc = 0;
    HDC   current_hdc   = 0;
    HWND  current_hwnd  = 0;
  } gl;

  struct {
    BOOL drawing             = FALSE;
  } imgui;

  struct  {
    BOOL hid                 = FALSE;
  } input;

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
  } debug;

  struct stack
  {
                 int current = 0;
    static const int max     = 2;


  } stack;
};

extern volatile LONG __SK_TLS_INDEX;

SK_TLS* __stdcall SK_TLS_Get    (void); // Alias: SK_TLS_Top
SK_TLS* __stdcall SK_TLS_Top    (void);
SK_TLS* __stdcall SK_TLS_Bottom (void);

//bool    __stdcall SK_TLS_Push   (void);
//bool    __stdcall SK_TLS_Pop    (void);

void    __stdcall SK_TLS_Push   (SK_TLS_STACK_MASK mask);
void    __stdcall SK_TLS_Pop    (SK_TLS_STACK_MASK mask);

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