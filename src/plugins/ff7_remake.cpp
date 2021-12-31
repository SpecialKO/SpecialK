//
// Copyright 2021 Andon "Kaldaien" Coleman
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
#include <SpecialK/utility.h>
#include <SpecialK/DLL_VERSION.H>
#include <imgui/font_awesome.h>

#include <SpecialK/control_panel/plugins.h>

#define RRM_VERSION_NUM L"0.0.1"
#define RRM_VERSION_STR L"Final Fantasy Re-Re-Make v " RRM_VERSION_NUM

static iSK_INI* ff7_engine_ini = nullptr;

bool
SK_FF7R_PlugInCfg (void)
{
  if ( ImGui::CollapsingHeader ( "FINAL FANTASY VII REMAKE INTERGRADE",
                                   ImGuiTreeNodeFlags_DefaultOpen )
     )
  {
    static uint64_t XInput1_3_size =
      SK_File_GetSize (L"XInput1_3.dll");

    if (XInput1_3_size > 98304 || XInput1_3_size == 0) // 0.1 - 96 KiB
    {
      ImGui::BulletText (
        "FFVIIHook by Havoc is required; install it as XInput1_3.dll"
      );
      ImGui::Separator (  );
      ImGui::TreePush  ("");
      ImGui::Text      ("Click [here] for more details...");

      if (ImGui::IsItemClicked ())
      {
        SK_Util_OpenURI (
          LR"(https://www.nexusmods.com/finalfantasy7remake/mods/74)",
          SW_SHOWNORMAL );

        SK_Util_ExplorePath (
          SK_GetHostPath () );
      }

      ImGui::TreePop ();

      return false;
    }

    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));
    ImGui::TreePush       ("");

    static auto& sec =
      ff7_engine_ini->get_section (L"SystemSettings");

    bool shader_caching =
      sec.contains_key (L"r.UseShaderCaching") &&
      sec.get_value    (L"r.UseShaderCaching")._Equal (L"1");

    static bool bChanged = false;

    if ( ImGui::CollapsingHeader (
           ICON_FA_PALETTE "\tShader Cache",
             ImGuiTreeNodeFlags_DefaultOpen )
       )
    {
      ImGui::TreePush ("");

      if (ImGui::Checkbox ("Enable Caching", &shader_caching))
      {
        bChanged = true;

        if (shader_caching)
        {
          ff7_engine_ini->import (
            L"[SystemSettings]\r\n"
            L"r.CreateShadersOnLoad=1\r\n"
            L"r.Shaders.Optimize=1\r\n"
            L"r.Shaders.FastMath=1\r\n"
            L"r.UseShaderCaching=1\r\n"
            L"r.UseUserShaderCache=1\r\n"
            L"r.UseShaderPredraw=1\r\n"
            L"r.UseAsyncShaderPrecompilation=1\r\n"
            L"r.TargetPrecompileFrameTime=5\r\n"
            L"r.PredrawBatchTime=5\r\n"
            L"r.AccelPredrawBatchTime=5\r\n"
            L"r.AccelTargetPrecompileFrameTime=5\r\n"
            L"r.OneFrameThreadLag=1\r\n"
            L"r.FinishCurrentFrame=0\r\n"
            L"r.ShaderPipelineCache.Enabled=1\r\n"
            L"r.ShaderPipelineCache.BatchSize=8\r\n"
            L"r.ShaderPipelineCache.BackgroundBatchSize=32\r\n"
            L"D3D12.PSO.DiskCache=1\r\n"
            L"D3D12.PSO.DriverOptimizedDiskCache=1\r\n"
          );
        }

        else
        {
          sec.remove_key (L"r.CreateShadersOnLoad");
          sec.remove_key (L"r.Shaders.Optimize");
          sec.remove_key (L"r.Shaders.FastMath");
          sec.remove_key (L"r.UseShaderCaching");
          sec.remove_key (L"r.UseUserShaderCache");
          sec.remove_key (L"r.UseShaderPredraw");
          sec.remove_key (L"r.UseAsyncShaderPrecompilation");
          sec.remove_key (L"r.TargetPrecompileFrameTime");
          sec.remove_key (L"r.PredrawBatchTime");
          sec.remove_key (L"r.AccelPredrawBatchTime");
          sec.remove_key (L"r.AccelTargetPrecompileFrameTime");
          sec.remove_key (L"r.OneFrameThreadLag");
          sec.remove_key (L"r.FinishCurrentFrame");
          sec.remove_key (L"r.ShaderPipelineCache.Enabled");
          sec.remove_key (L"r.ShaderPipelineCache.BatchSize");
          sec.remove_key (L"r.ShaderPipelineCache.BackgroundBatchSize");
          sec.remove_key (L"D3D12.PSO.DiskCache");
          sec.remove_key (L"D3D12.PSO.DriverOptimizedDiskCache");
        }

        ff7_engine_ini->write ();
      }

      if (shader_caching)
      {
        DWORD dwNow =
          SK_timeGetTime ();

        static DWORD dwLastChecked = dwNow;

        static uint64_t sizeCompute  = 0ULL;
        static uint64_t sizeGraphics = 0ULL;

        if (dwLastChecked < dwNow - 750)
        {   dwLastChecked = dwNow;

          static const std::wstring
          cache_files [] =
          {
            ( SK_GetDocumentsDir () +
                LR"(\My Games\FINAL FANTASY VII REMAKE\Saved\)"
                LR"(D3DCompute.ushaderprecache)"  ),
            ( SK_GetDocumentsDir () +
                LR"(\My Games\FINAL FANTASY VII REMAKE\Saved\)"
                LR"(D3DGraphics.ushaderprecache)" ),
          };

          sizeCompute =
            SK_File_GetSize (cache_files [0].c_str ());
          sizeGraphics =
            SK_File_GetSize (cache_files [1].c_str ());
        }

        ImGui::SameLine   (  );
        ImGui::BeginGroup (  );
        ImGui::TreePush   ("");
        ImGui::Text       ("Compute Cache:\t%s",  SK_File_SizeToStringA (
                        sizeCompute).data  ( )                          );
        ImGui::Text       ("Graphics Cache:\t%s", SK_File_SizeToStringA (
                        sizeGraphics).data ( )                          );
        ImGui::TreePop    (  );
        ImGui::EndGroup   (  );
      }

      ImGui::TreePop ();
    }

    if ( ImGui::CollapsingHeader (
           ICON_FA_IMAGE "\tTexture Streaming",
             ImGuiTreeNodeFlags_DefaultOpen
       )                         )
    {
      ImGui::TreePush ("");

      if (ImGui::Button ("Load Suggested Streaming Settings"))
      {
        bChanged = true;

        ff7_engine_ini->import (
          L"[SystemSettings]\r\n"
          L"r.Streaming.HLODStrategy=0\r\n"
          L"r.Streaming.PoolSize=0\r\n"
          L"r.Streaming.LimitPoolSizeToVRAM=1\r\n"
          L"r.Streaming.MaxTempMemoryAllowed=256\r\n"
          L"r.Streaming.AmortizeCPUToGPUCopy=1\r\n"
          L"r.Streaming.MaxNumTexturesToStreamPerFrame=20\r\n"
          L"r.Streaming.NumStaticComponentsProcessedPerFrame=20\r\n"
          L"r.Streaming.FramesForFullUpdate=1\r\n"
          L"s.AsyncLoadingThreadEnabled=1\r\n"
          L"s.AsyncLoadingTimeLimit=0.5\r\n"
          L"s.LevelStreamingActorsUpdateTimeLimit=0.5\r\n"
          L"s.UnregisterComponentsTimeLimit=0.5\r\n"
          L"s.AsyncLoadingUseFullTimeLimit=1\r\n"
          L"s.IoDispatcherCacheSizeMB=512\r\n"
          L"s.LevelStreamingComponentsRegistrationGranularity=1\r\n"
          L"s.LevelStreamingComponentsUnregistrationGranularity=1\r\n"
          L"s.MaxIncomingRequestsToStall=2\r\n"
          L"s.MaxReadyRequestsToStallMB=8\r\n"
          L"s.MinBulkDataSizeForAsyncLoading=16\r\n"
          L"s.PriorityAsyncLoadingExtraTime=2\r\n"
          L"r.MipMapLODBias=0\r\n"
          L"r.SkeletalMeshLODBias=0\r\n"
          L"r.LandscapeLODBias=0\r\n"
          L"r.ParticleLODBias=0\r\n"
          L"r.Streaming.MinMipForSplitRequest=1\r\n"
          L"r.HZBOcclusion=0\r\n"
          L"r.AllowOcclusionQueries=1\r\n" );

        ff7_engine_ini->write ();
      }

      ImGui::TreePop ();
    }

    bool bResolutionScaling =
      (! sec.get_value (L"r.DynamicRes.OperationMode")._Equal (L"0"));

    extern float __target_fps;

    if (ImGui::CollapsingHeader ( ICON_FA_BALANCE_SCALE "\tResolution Scaling",
                                    ImGuiTreeNodeFlags_DefaultOpen ))
    {
      ImGui::TreePush ("");

      if ( ImGui::Checkbox ( "Enable Functional Dynamic Resolution",
                                                      &bResolutionScaling ) )
      {
        bChanged = true;

        if (__target_fps <= 0.0f)
          bResolutionScaling = false;

        if (! sec.contains_key  (L"r.DynamicRes.OperationMode"))
              sec.add_key_value (L"r.DynamicRes.OperationMode",
                                 bResolutionScaling ? L"2"
                                                    : L"0");

        if (bResolutionScaling)
        {
          if (! sec.contains_key  (L"r.DynamicRes.FrameTimeBudget"))
                sec.add_key_value (L"r.DynamicRes.FrameTimeBudget",
                                   L"16.666667");

          if (! sec.contains_key  (L"r.DynamicRes.MinScreenPercentage"))
                sec.add_key_value (L"r.DynamicRes.MinScreenPercentage",
                                   L"50");

          if (! sec.contains_key  (L"r.DynamicRes.MaxScreenPercentage"))
                sec.add_key_value (L"r.DynamicRes.MaxScreenPercentage",
                                   L"100");
        }

        sec.get_value (L"r.DynamicRes.OperationMode").
               assign (bResolutionScaling ? L"2"
                                          : L"0");
        sec.get_value (L"r.DynamicRes.FrameTimeBudget").
               assign (std::to_wstring (1000.0 / __target_fps));

        ff7_engine_ini->write ();
      }

      if (ImGui::IsItemHovered ())
      {
        if (__target_fps <= 0.0f)
        {
          ImGui::SetTooltip (
            "Scaling is controlled by Special K's Framerate Limiter; "
            "it must be turned on."
          );
        }
      }

      if (bResolutionScaling)
      {
        static float
          fMinMax [2] = {
            50.0f, 100.0f
          };

        if (! sec.contains_key (L"r.DynamicRes.MinScreenPercentage"))
             fMinMax [0] =  50.0f;
        else fMinMax [0] =
          std::wcstof (
            sec.get_value (L"r.DynamicRes.MinScreenPercentage").c_str (),
              nullptr );

        if (! sec.contains_key (L"r.DynamicRes.MaxScreenPercentage"))
             fMinMax [1] = 100.0f;
        else fMinMax [1] =
          std::wcstof (
            sec.get_value (L"r.DynamicRes.MaxScreenPercentage").c_str (),
              nullptr );

        ImGui::SameLine    ();
        ImGui::Text        ("\t( %4.1f fps )\t", 1000.0f / std::wcstof (
                sec.get_value (L"r.DynamicRes.FrameTimeBudget").c_str (),
              nullptr      )                                     );
        ImGui::SameLine    ();

        if (ImGui::InputFloat2 ("Min/Max", fMinMax, "%3.1f%%"))
        {
          fMinMax [1] =
            std::clamp (fMinMax [1],
              std::max (fMinMax [0], 25.0f), 100.0f);
          fMinMax [0] =
            std::clamp (fMinMax [0], 25.0f,
              std::max (fMinMax [1], 25.0f));

          sec.get_value (L"r.DynamicRes.MinScreenPercentage") =
                                std::to_wstring (fMinMax [0]);
          sec.get_value (L"r.DynamicRes.MaxScreenPercentage") =
                                std::to_wstring (fMinMax [1]);
          sec.get_value (L"r.DynamicRes.FrameTimeBudget")     =
                      std::to_wstring (1000.0 / __target_fps);

          ff7_engine_ini->write ();

          bChanged = true;
        }
      }

      ImGui::TreePop ();
    }

    if (bChanged)
    {
      ImGui::BulletText ("Game Restart Required");
      ImGui::SameLine   ();
      ImGui::Text       ("\tOriginal Settings:  [ BackupEngine.ini ]");
    }

    ImGui::TreePop       ( );
    ImGui::PopStyleColor (3);
  }

  return true;
}

void
SK_FF7R_BeginFrame (void)
{
  static uintptr_t pBase =
    (uintptr_t)SK_Debug_GetImageBaseAddr ();

  static uint8_t orig_bytes_3304DC8 [6] = { 0x0 };
  static uint8_t orig_bytes_3304DD0 [6] = { 0x0 };
  static uint8_t orig_bytes_3304E03 [2] = { 0x0 };
  static uint8_t orig_bytes_1C0C42A [5] = { 0x0 };
  static uint8_t orig_bytes_1C0C425 [5] = { 0x0 };

  std::queue <DWORD> suspended;

  auto orig_se =
    SK_SEH_ApplyTranslator (SK_FilteringStructuredExceptionTranslator (EXCEPTION_ACCESS_VIOLATION));
  try
  {
    static bool        bPatchable    = false;
    static bool        bInit         = false;
    if (std::exchange (bInit, true) == false)
    {
      DWORD dwOldProtect = 0x0;

      //suspended =
      //  SK_SuspendAllOtherThreads ();

      VirtualProtect ((LPVOID)(pBase + 0x3304DC6), 2, PAGE_EXECUTE_READWRITE, &dwOldProtect);
      bPatchable = ( 0 == memcmp (
                      (LPVOID)(pBase + 0x3304DC6), "\x73\x3d", 2) );
      VirtualProtect ((LPVOID)(pBase + 0x3304DC6), 2, dwOldProtect,           &dwOldProtect);

      if (bPatchable)
      {
        VirtualProtect ((LPVOID)(pBase + 0x3304DC8), 6, PAGE_EXECUTE_READWRITE, &dwOldProtect);
        memcpy (                orig_bytes_3304DC8,
                        (LPVOID)(pBase + 0x3304DC8),                                        6);
        memcpy (        (LPVOID)(pBase + 0x3304DC8), "\x90\x90\x90\x90\x90\x90",            6);
        VirtualProtect ((LPVOID)(pBase + 0x3304DC8), 6, dwOldProtect,           &dwOldProtect);
        VirtualProtect ((LPVOID)(pBase + 0x3304DD0), 6, PAGE_EXECUTE_READWRITE, &dwOldProtect);
        memcpy (                orig_bytes_3304DD0,
                        (LPVOID)(pBase + 0x3304DD0),                                        6);
        memcpy (        (LPVOID)(pBase + 0x3304DD0), "\x90\x90\x90\x90\x90\x90",            6);
        VirtualProtect ((LPVOID)(pBase + 0x3304DD0), 6, dwOldProtect,           &dwOldProtect);

        VirtualProtect ((LPVOID)(pBase + 0x3304E03), 2, PAGE_EXECUTE_READWRITE, &dwOldProtect);
        memcpy (                orig_bytes_3304E03,
                        (LPVOID)(pBase + 0x3304E03),                                        2);
        memcpy (        (LPVOID)(pBase + 0x3304E03), "\x90\x90",                            2);
        VirtualProtect ((LPVOID)(pBase + 0x3304E03), 2, dwOldProtect,           &dwOldProtect);

#if 0
#if 1
        VirtualProtect ((LPVOID)(pBase + 0x1C0C42A), 5, PAGE_EXECUTE_READWRITE, &dwOldProtect);
        memcpy (                orig_bytes_1C0C42A,
                        (LPVOID)(pBase + 0x1C0C42A),                                        5);
        memcpy (        (LPVOID)(pBase + 0x1C0C42A), "\x90\x90\x90\x90\x90",                5);
        VirtualProtect ((LPVOID)(pBase + 0x1C0C42A), 5, dwOldProtect,           &dwOldProtect);
#endif
#if 1
        VirtualProtect ((LPVOID)(pBase + 0x1C0C425), 5, PAGE_EXECUTE_READWRITE, &dwOldProtect);
        memcpy (                orig_bytes_1C0C425,
                        (LPVOID)(pBase + 0x1C0C425),                                        5);
        memcpy (        (LPVOID)(pBase + 0x1C0C425), "\x90\x90\x90\x90\x90",                5);
        VirtualProtect ((LPVOID)(pBase + 0x1C0C425), 5, dwOldProtect,           &dwOldProtect);
#endif
#endif
      }
    }

    extern float      __target_fps;
    if (bPatchable && __target_fps > 0.0f)
    {
      **(float **)(pBase + 0x0590C750) =
                      __target_fps;
    }
  }

  catch (...)
  {
  }
  SK_SEH_RemoveTranslator (orig_se);

  //if (! suspended.empty ())
  //  SK_ResumeThreads (suspended);
}

void
SK_FF7R_InitPlugin (void)
{
  plugin_mgr->begin_frame_fns.emplace (SK_FF7R_BeginFrame);
  plugin_mgr->config_fns.emplace      (SK_FF7R_PlugInCfg);

  ff7_engine_ini =
    SK_CreateINI (
      ( SK_GetDocumentsDir () +
          LR"(\My Games\FINAL FANTASY VII REMAKE\Saved\Config\WindowsNoEditor\)"
          LR"(Engine.ini)"
      ).c_str () );

  if ( ff7_engine_ini->contains_section (L"SystemSettings") &&
         0 == SK_File_GetSize (
       ( SK_GetDocumentsDir () +
           LR"(\My Games\FINAL FANTASY VII REMAKE\Saved\Config\WindowsNoEditor\)"
           LR"(BackupEngine.ini)"
       ).c_str ()             )
     ) SK_File_FullCopy (
       ( SK_GetDocumentsDir () +
           LR"(\My Games\FINAL FANTASY VII REMAKE\Saved\Config\WindowsNoEditor\)"
           LR"(Engine.ini)"
       ).c_str (),
       ( SK_GetDocumentsDir () +
           LR"(\My Games\FINAL FANTASY VII REMAKE\Saved\Config\WindowsNoEditor\)"
           LR"(BackupEngine.ini)"
       ).c_str ()
     );
}