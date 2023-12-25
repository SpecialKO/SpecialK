//
// Copyright 2018 Andon "Kaldaien" Coleman
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

/*
Nino2.exe+11DA450 // Input  timestep (single-precision float, in seconds)
Nino2.exe+11DA454 // Render timestep (single-precision float, in seconds)


=====
// dT (input, render)
//
Nino2.agsInit+2B298D - F3 0F11 35 7FD98500   - movss [Nino2.exe+11DA454],xmm6 { [0.02] }
Nino2.agsInit+2B2995 - F3 0F11 3D 73D98500   - movss [Nino2.exe+11DA450],xmm7 { [0.02] }

F3 0F 11 35 7F D9 85 00 // Variable-timing (Render)
F3 0F 11 3D 73 D9 85 00 // Variable-timing (Input)
-----
90 90 90 90 90 90 90 90 // Fixed-timing (Render)
90 90 90 90 90 90 90 90 // Fixed-timing (Input)

~~~~~
// Game's limiter (Blocks Win32 Message Pump -- bad idea!)
//
Nino2.agsInit+2B21E1 - 77 BD                 - ja Nino2.agsInit+2B21A0

66 0F 2F F8  77 BD  48 2B CA // Limiter Enabled
-----
66 0F 2F F8  90 90  48 2B CA // Limiter Disabled
*/



struct mem_addr_s
{
  const char*    pattern;
  const char*    pattern_mask;
  size_t         pattern_len;

  size_t         rep_size;
  ptrdiff_t      rep_off;

  bool           enabled;
  void*          scanned_addr;

  std::vector<BYTE> orig_bytes;

  const wchar_t* desc;

  void scan (void)
  {
    if (! scanned_addr)
    {
      scanned_addr = SK_ScanAlignedEx (pattern, pattern_len, pattern_mask);

      dll_log->Log (L"Scanned address for: %s: %p (alignment=%lu)", desc, scanned_addr, (uintptr_t)scanned_addr % 16);
    }
  }

  void enable (void)
  {
    scan ();

    if (scanned_addr != nullptr && (! enabled))
    {
      DWORD dwProtect;

      VirtualProtect ((void*)((intptr_t)scanned_addr + rep_off), rep_size, PAGE_EXECUTE_READWRITE, &dwProtect);

      if (rep_size == 8)
      {
        InterlockedExchange64 ((volatile LONG64*)((intptr_t)scanned_addr + rep_off), *(__int64 *)orig_bytes.data ());
      }

      else if (rep_size == 2)
      {
        InterlockedExchange16 ((volatile SHORT*)((intptr_t)scanned_addr + rep_off), *(SHORT *)"\x77\xBD");
      }

      // Other sizes are not atomic, so... not threadsafe
      else
      {
        memcpy ((void *)((intptr_t)scanned_addr + rep_off), orig_bytes.data (), rep_size);
      }

      VirtualProtect ((void*)((intptr_t)scanned_addr + rep_off), rep_size, dwProtect, &dwProtect);

      enabled = true;
    }
  }

  void disable (void)
  {
    scan ();

    if (scanned_addr != nullptr && enabled)
    {
      DWORD dwProtect;

      VirtualProtect ((void*)((intptr_t)scanned_addr + rep_off), rep_size, PAGE_EXECUTE_READWRITE, &dwProtect);

      orig_bytes.clear ();

      for ( UINT i = 0; i < rep_size; i++ )
      {
        orig_bytes.push_back (*(uint8_t *)((uintptr_t)scanned_addr + rep_off + i));
      }

      if (rep_size == 8)
      {
        InterlockedExchange64 ((volatile LONG64*)((intptr_t)scanned_addr + rep_off), *(LONG64 *)"\x90\x90\x90\x90\x90\x90\x90\x90");
      }

      else if (rep_size == 2)
      {
        InterlockedExchange16 ((volatile SHORT*)((intptr_t)scanned_addr + rep_off), *(SHORT *)"\x90\x90");
      }

      else
      {
        memset ((void *)((intptr_t)scanned_addr + rep_off), 0x90, rep_size);
      }

      VirtualProtect ((void*)((intptr_t)scanned_addr + rep_off), rep_size, dwProtect, &dwProtect);

      enabled = false;
    }
  }
};

////E8  06  E0  16  00     F3  0F  11  35     F2  76  86  00     F3     0F  11  3D     E6  76  86  00     E8 91AF9FFF
//"\xE8\x00\x00\x00\x00" "\xF3\x0F\x11\x35" "\x00\x00\x00\x00" "\xF3" "\x0F\x11\x3D" "\x00\x00\x00\x00" "\xE8"
//"\xE8\x00\x00\x00\x00" "\xF3\x0F\x11\x35" "\x00\x00\x00\x00" "\xF3" "\x0F\x11\x3D" "\x00\x00\x00\x00" "\xE8"

sk::ParameterFloat* SK_NNK2_VirtualFPS    = nullptr;
sk::ParameterBool*  SK_NNK2_ReduceShimmer = nullptr;

static float SK_NNK2_target_fps = 0.0f;


#define NNS_VERSION_NUM L"0.2.0"
#define NNS_VERSION_STR L"Ni no Stutter v " NNS_VERSION_NUM


unsigned int
__stdcall
SK_NNK2_CheckVersion (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  // 12/28/20: Disabled version checks, since I don't intend to ever update this thing again.
  //
  ////if (SK_FetchVersionInfo (L"NiNoStutter"))
  ////    SK_UpdateSoftware   (L"NiNoStutter");

  return 0;
}


bool SK_NNK2_CacheModes        = true;
bool SK_NNK2_FixTextureShimmer = false;


extern HRESULT
WINAPI
D3D11Dev_CreateSamplerState_Override
(
  _In_            ID3D11Device        *This,
  _In_      const D3D11_SAMPLER_DESC  *pSamplerDesc,
  _Out_opt_       ID3D11SamplerState **ppSamplerState );

static D3D11Dev_CreateSamplerState_pfn _D3D11Dev_CreateSamplerState_Original = nullptr;

HRESULT
WINAPI
SK_NNK2_CreateSamplerState
(
  _In_            ID3D11Device        *This,
  _In_      const D3D11_SAMPLER_DESC  *pSamplerDesc,
  _Out_opt_       ID3D11SamplerState **ppSamplerState )
{
  D3D11_SAMPLER_DESC new_desc = *pSamplerDesc;

  // Fix sharpened textures
  if (SK_NNK2_FixTextureShimmer && new_desc.MaxAnisotropy != 1)
  {
    if (new_desc.Filter == D3D11_FILTER_ANISOTROPIC)
    {
      new_desc.MaxAnisotropy = 16;
      new_desc.MipLODBias    = std::fmax (0.0f, new_desc.MipLODBias);

      const HRESULT hr =
        _D3D11Dev_CreateSamplerState_Original (This, &new_desc, ppSamplerState);

      if (SUCCEEDED (hr))
        return hr;
    }

    else
    {
      new_desc.MaxAnisotropy = 1;

      const HRESULT hr =
        _D3D11Dev_CreateSamplerState_Original (This, &new_desc, ppSamplerState);

      if (SUCCEEDED (hr))
        return hr;
    }
  }

  return
    _D3D11Dev_CreateSamplerState_Original (This, pSamplerDesc, ppSamplerState);
}

mem_addr_s instn__write_dt_input  = { };
mem_addr_s instn__write_dt_render = { };
mem_addr_s instn__limit_branch    = { };


#define TIMESTEP_BASE_ADDR 0x11F1470

bool SK_NNK2_PlugInCfg (void);

void
SK_NNK2_InitPlugin (void)
{
  static auto dll_ini =
    SK_GetDLLConfig ();

  SK_SetPluginName (NNS_VERSION_STR);

  plugin_mgr->config_fns.emplace (SK_NNK2_PlugInCfg);

  SK_NNK2_CheckVersion (nullptr);

  SK_NNK2_VirtualFPS =
    dynamic_cast <sk::ParameterFloat *> (
      g_ParameterFactory->create_parameter <float> (L"Fixed-Tick Timestep")
    );

  SK_NNK2_VirtualFPS->register_to_ini (
    dll_ini, L"NiNoStutter.Framerate", L"VirtualFPS"
  );

  SK_NNK2_VirtualFPS->load (SK_NNK2_target_fps);


  SK_NNK2_ReduceShimmer =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory->create_parameter <bool> (L"Eliminate negative LOD biases")
    );

  SK_NNK2_ReduceShimmer->register_to_ini (
    dll_ini, L"NiNoStutter.Textures", L"ReduceShimmer"
  );

  SK_NNK2_ReduceShimmer->load (SK_NNK2_FixTextureShimmer);


  SK_Thread_Create ([](LPVOID) -> DWORD
  {
    instn__write_dt_input =
        { "\xF3\x0F\x11\x35"     "\x00\x00\x00\x00"
          "\xF3\x0F\x11\x3D",
      //----------------------------------------------//
          "\xF3\x0F\x11\x35"     "\x00\x00\x00\x00"
          "\xF3\x0F\x11\x3D",
                 12, 8, 0,    true, nullptr,

        { }, L"write_dt_input" };



    instn__write_dt_render =
        { "\xF3\x0F\x11\x3D"     "\x73\xD9\x85\x00"  "\xE8",
      //----------------------------------------------//
         "\xF3\x0F\x11\x3D"      "\x00\x00\x00\x00"  "\xE8",
                 9,  8, 0,    true, nullptr,

          { }, L"write_dt_render" };

    instn__limit_branch =
        { "\x66\x0F\x2F\xF8\x77\xBD\x48\x2B\xCA",    nullptr,
                 9,  2, 4,    true, nullptr,

          { }, L"limit_branch"    };


    // Fix for window-management issues in windowed mode
    while (SK_GetFramesDrawn () < 8)
    {
      SK_Sleep (16UL);
    }


    instn__write_dt_render.scan ();
    instn__write_dt_input.scan  ();


    if (SK_NNK2_target_fps != 0.0f)
    {
      config.window.background_render = true;

      instn__write_dt_render.disable ();
      instn__write_dt_input.disable  ();
      instn__limit_branch.disable    ();

      float* pfBase =
        reinterpret_cast <float *>(((uintptr_t)SK_GetModuleHandle (nullptr) + TIMESTEP_BASE_ADDR));

      static float* pfdTi = pfBase;
      static float* pfdTr = pfBase+1;

      *pfdTr = ( 1.0f / SK_NNK2_target_fps );
      *pfdTi = ( 1.0f / SK_NNK2_target_fps );
    }

    SK_Thread_CloseSelf ();

    return 0;
  });


    SK_CreateFuncHook (       L"ID3D11Device::CreateSamplerState",
                               D3D11Dev_CreateSamplerState_Override,
                                 SK_NNK2_CreateSamplerState,
     static_cast_p2p <void> (&_D3D11Dev_CreateSamplerState_Original) );
  MH_QueueEnableHook (         D3D11Dev_CreateSamplerState_Override  );
}



bool
SK_NNK2_PlugInCfg (void)
{
  static float* pfdTi = (float *)((uint8_t *)SK_GetModuleHandle (nullptr) +  TIMESTEP_BASE_ADDR);
  static float* pfdTr = pfdTi + 1;

  if (ImGui::CollapsingHeader ("Ni no Kuni™ II Revenant Kingdom", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

  //ImGui::Checkbox ("Cache IDXGIDisplay::GetDisplayModeList (...)", &SK_NNK2_CacheModes);

    static bool fixed_timestep = SK_NNK2_target_fps != 0.0f;

    if (ImGui::Checkbox ("Fixed-Timestep", &fixed_timestep))
    {
      if (fixed_timestep)
      {
        extern float __target_fps;

        if (SK_NNK2_target_fps == 0.0f)
            SK_NNK2_target_fps = ( __target_fps == 0.0f ? 60.0f : std::fabs (__target_fps) );

        config.window.background_render = true;

        instn__write_dt_input.disable  ();
        instn__write_dt_render.disable ();
        instn__limit_branch.disable    ();

        *pfdTr = ( 1.0f / SK_NNK2_target_fps );
        *pfdTi = ( 1.0f / SK_NNK2_target_fps );

        SK_NNK2_VirtualFPS->store (SK_NNK2_target_fps);
      }

      else
      {
        instn__write_dt_input.enable  ();
        instn__write_dt_render.enable ();
        instn__limit_branch.enable    ();

        SK_NNK2_target_fps = 0.0f;
        SK_NNK2_VirtualFPS->store (SK_NNK2_target_fps);
      }
    }

    if (fixed_timestep)
    {
      ImGui::SameLine ();

      if (ImGui::InputFloat ("Virtual FPS", &SK_NNK2_target_fps, 0.5f, 5.0f))
      {
        if (SK_NNK2_target_fps < 1.0f) SK_NNK2_target_fps = 1.0f;

        *pfdTr = ( 1.0f / SK_NNK2_target_fps );
        *pfdTi = ( 1.0f / SK_NNK2_target_fps );

        SK_NNK2_VirtualFPS->store (SK_NNK2_target_fps);
      }

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("The framerate to base timing off of -- rendering slower than this rate will cause slow-mo");
    }

    ImGui::Checkbox ("Prevent Controller Disconnect Messages",        &config.input.gamepad.xinput.placehold [0]);
    ImGui::SameLine ();
    ImGui::Checkbox ("Continue Rendering if Game Window is Inactive", &config.window.background_render);

    if (ImGui::Checkbox ("Reduce Texture Shimmering", &SK_NNK2_FixTextureShimmer))
    {
      SK_NNK2_ReduceShimmer->store (SK_NNK2_FixTextureShimmer);
    }

    ImGui::TreePop  ();

    return false;
  }

  return true;
}