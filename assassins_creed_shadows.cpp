// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
//
// Copyright 2025 Andon "Kaldaien" Coleman
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

#include <safetyhook/safetyhook.hpp>

extern bool __SK_ACS_DynamicCloth;

SafetyHookMid* __SK_ACS_ClothPhysicsMidHook = nullptr;

void
SK_ACS_ApplyClothPhysicsFix (bool enable)
{
  extern uintptr_t __SK_ACS_ClothSimAddr;

  if (enable && __SK_ACS_ClothPhysicsMidHook == nullptr)
  {
    // From ACShadowsFix (https://github.com/Lyall/ACShadowsFix/blob/5b53a3c84fa8dee39a1ed49b60995b334a119d97/src/dllmain.cpp#L246C1-L256C20)
    __SK_ACS_ClothPhysicsMidHook = new SafetyHookMid (
      safetyhook::create_mid (__SK_ACS_ClothSimAddr,
        [](SafetyHookContext& ctx)
        {
          if (! ctx.rcx) return;

          // By default the game appears to use 60fps cloth physics during gameplay and 30fps cloth physics during cutscenes.

          if (__SK_ACS_DynamicCloth)
          {
            // Use current frametime for cloth physics instead of fixed 0.01666/0.03333 values.
            if (uintptr_t pFramerate = *reinterpret_cast <uintptr_t *>(ctx.rcx + 0x70))
              ctx.xmm0.f32 [0] = *reinterpret_cast<float *>(pFramerate + 0x78); // Current frametime
          }
        }
      )
    );
  }

  if (enable)
  {
    if (__SK_ACS_ClothPhysicsMidHook != nullptr)
        std::ignore = __SK_ACS_ClothPhysicsMidHook->enable ();
  }

  else
  {
    if (__SK_ACS_ClothPhysicsMidHook != nullptr)
        std::ignore = __SK_ACS_ClothPhysicsMidHook->disable ();
  }
}