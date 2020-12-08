//
// Copyright 2020 Andon "Kaldaien" Coleman
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

#include <SpecialK/stdafx.h>
#include <SpecialK/render/d3d11/d3d11_core.h>
#include <SpecialK/utility.h>

#define ACVD_VERSION_NUM L"0.0.1"
#define ACVD_VERSION_STR L"Assassin's Creed: Valhalla v " ACVD_VERSION_NUM

//static NtReadFile_pfn NtReadFile_Original = nullptr;

#define NT_SUCCESS(Status)                      ((NTSTATUS)(Status) >= 0)
#define STATUS_SUCCESS                          0
#define STATUS_NO_SUCH_FILE                     0xC000000F

#include <SpecialK/diagnostics/file.h>

void
SK_ACV_InitPlugin (void)
{
//SK_SetPluginName (ACVD_VERSION_STR);

  auto intro_vids =
    SK_GetDLLConfig ()->get_section (L"ACValhalla.PlugIn").
                        get_value   (L"DisableIntroVideos");

  if (SK_IsTrue (intro_vids.c_str ()))
  {
  }
}
