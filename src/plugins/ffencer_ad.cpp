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

#define FFAD_VERSION_NUM L"0.0.1"
#define FFAD_VERSION_STR L"Fairy Fencer License Fix v " FFAD_VERSION_NUM

#include <Windows.h>
#include <SpecialK/hooks.h>

extern void
__stdcall
SKX_SetPluginName (const wchar_t* wszName);

typedef __time64_t (*_time64_pfn)(__time64_t *timer);
                     _time64_pfn _time64_Original = nullptr;

__time64_t
_time64_Detour (__time64_t *timer)
{
  static volatile ULONG call_count = 0;

  // Only need the initial license check calls to spoof the time; everything else gets
  //   valid time.
  if (InterlockedIncrement (&call_count) < 10)
  {
    *timer = 1498807199; // ~Jun 30 2017, 23:59.59 -- Within middleware license period
    return *timer;
  }

  return _time64_Original (timer);
}

void
SK_FFAD_InitPlugin (void)
{
#if 0
  SKX_SetPluginName (FFAD_VERSION_STR);

  SK_CreateDLLHook2 (L"msvcr100.dll", "_time64", _time64_Detour, (LPVOID *)&_time64_Original);
  MH_ApplyQueued    ();
#endif
}