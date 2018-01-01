//
// Copyright 2017 Andon "Kaldaien" Coleman
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

#include <SpecialK/ini.h>
#include <SpecialK/config.h>
#include <SpecialK/parameter.h>
#include <SpecialK/hooks.h>
#include <SpecialK/log.h>

sk::ParameterFactory okami_factory;
sk::ParameterBool*   sk_okami_grain;

bool SK_Okami_use_grain = true;

extern iSK_INI* dll_ini;

void
WINAPI
SK_D3D11_AddTexHash ( const wchar_t* name, uint32_t top_crc32, uint32_t hash );


typedef bool (__cdecl *m2_WindowControl_resizeBackBuffers_pfn)(LPVOID This, unsigned int, unsigned int, bool);
typedef bool (__cdecl *m2_WindowControl_resizeRenderBuffers_pfn)(LPVOID This, unsigned int, unsigned int, bool);

m2_WindowControl_resizeBackBuffers_pfn   m2_WindowControl_resizeBackBuffers_Original   = nullptr;
m2_WindowControl_resizeRenderBuffers_pfn m2_WindowControl_resizeRenderBuffers_Original = nullptr;

bool
SK_Okami_m2_WindowControl_resizeBackBuffers_Detour (LPVOID This, unsigned short width, unsigned short height, bool unknown)
{
  dll_log.Log (L"m2::WindowControl::resizeBackBuffers (%ph, %ux%u, %lu)", This, width, height, unknown);

  if ((! config.window.res.override.isZero ()) && width > 853)
  {
    width  = (unsigned short)config.window.res.override.x;
    height = (unsigned short)config.window.res.override.y;
    //unknown = false;
  }

  return m2_WindowControl_resizeBackBuffers_Original (This, width, height, unknown);
}

bool
SK_Okami_m2_WindowControl_resizeRenderBuffers_Detour (LPVOID This, unsigned int width, unsigned int height, bool unknown)
{
  dll_log.Log (L"m2::WindowControl::resizeRenderBuffers (%ph, %lux%lu, %lu)", This, width, height, unknown);

  if ((! config.window.res.override.isZero ()) && width > 853)
  {
    width  = config.window.res.override.x;
    height = config.window.res.override.y;
    //unknown = false;
  }

  return m2_WindowControl_resizeRenderBuffers_Original (This, width, height, unknown);
}

#include <SpecialK/utility.h>

void
SK_Okami_LoadConfig (void)
{
  sk_okami_grain =
    dynamic_cast <sk::ParameterBool *> (
      okami_factory.create_parameter <bool> (
        L"Use Grain?"
      )
    );

  sk_okami_grain->register_to_ini (
    dll_ini,
      L"OKAMI HD / 大神 絶景版",
        L"EnableGrain" );

  sk_okami_grain->load (SK_Okami_use_grain);

  if (SK_Okami_use_grain)
  {
    SK_D3D11_AddTexHash (L"grain.dds", 0xced133fb, 0x00);
  }

  else
  {
    SK_D3D11_AddTexHash (L"no_grain.dds", 0xced133fb, 0x00);
  }

  SK_CreateDLLHook2 (                      L"main.dll",
                                            "?resizeBackBuffers@WindowControl@m2@@QEAA_NII_N@Z",
                    SK_Okami_m2_WindowControl_resizeBackBuffers_Detour,
    static_cast_p2p <void> (&m2_WindowControl_resizeBackBuffers_Original) );
  SK_CreateDLLHook2 (                      L"main.dll",
                                            "?resizeRenderBuffers@WindowControl@m2@@QEAA_NGG_N@Z",
                    SK_Okami_m2_WindowControl_resizeRenderBuffers_Detour,
    static_cast_p2p <void> (&m2_WindowControl_resizeRenderBuffers_Original) );

  SK_ApplyQueuedHooks ();
}

void
SK_Okami_SaveConfig (void)
{
  sk_okami_grain->store (SK_Okami_use_grain);
  dll_ini->write (dll_ini->get_filename ());
}