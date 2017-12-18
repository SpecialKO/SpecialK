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

sk::ParameterFactory okami_factory;
sk::ParameterBool*   sk_okami_grain;

bool SK_Okami_use_grain = true;

extern iSK_INI* dll_ini;

void
WINAPI
SK_D3D11_AddTexHash ( const wchar_t* name, uint32_t top_crc32, uint32_t hash );

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
}

void
SK_Okami_SaveConfig (void)
{
  sk_okami_grain->store (SK_Okami_use_grain);
  dll_ini->write (dll_ini->get_filename ());
}