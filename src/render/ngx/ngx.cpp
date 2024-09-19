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

#include <SpecialK/stdafx.h>

#include <SpecialK/render/ngx/ngx.h>
#include <imgui/font_awesome.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L" NGX Core "

bool __SK_HasDLSSGStatusSupport = false;
bool __SK_IsDLSSGActive         = false;
bool __SK_DoubleUpOnReflex      = false;
bool __SK_ForceDLSSGPacing      = false;

SK_LazyGlobal <NGX_ThreadSafety> SK_NGX_Threading;

bool
SK_NGX_IsUsingDLSS (void)
{
  const auto& dlss_ss =
    (SK_NGX_DLSS12.apis_called    ?
     SK_NGX_DLSS12.super_sampling : SK_NGX_DLSS11.super_sampling);

  return                 dlss_ss.Handle     != nullptr                &&
    ReadULong64Acquire (&dlss_ss.LastFrame) >= SK_GetFramesDrawn () - 8;
}

bool
SK_NGX_IsUsingDLSS_D (void)
{
  const auto& dlss_ss =
    SK_NGX_DLSS12.super_sampling;

  return                 dlss_ss.Handle     != nullptr                  &&
    ReadULong64Acquire (&dlss_ss.LastFrame) >= SK_GetFramesDrawn () - 8 &&
                         dlss_ss.DLSS_Type  == NVSDK_NGX_Feature_RayReconstruction;
}

bool
SK_NGX_IsUsingDLSS_G (void)
{
  return /// TODO Refactor
    __SK_IsDLSSGActive;
}

static  unsigned  int SK_NGX_GameSetPerfQuality = 0;
static constexpr bool SK_NGX_LogAllParams       = false;

// NGX return-code conversion-to-string utility only as a helper for debugging/logging - not for official use.
GetNGXResultAsString_pfn                  GetNGXResultAsString                     = nullptr;

NVSDK_NGX_Parameter_SetF_pfn           NVSDK_NGX_Parameter_SetF_Original           = nullptr;
NVSDK_NGX_Parameter_SetD_pfn           NVSDK_NGX_Parameter_SetD_Original           = nullptr;
NVSDK_NGX_Parameter_SetI_pfn           NVSDK_NGX_Parameter_SetI_Original           = nullptr;
NVSDK_NGX_Parameter_SetUI_pfn          NVSDK_NGX_Parameter_SetUI_Original          = nullptr;
NVSDK_NGX_Parameter_SetULL_pfn         NVSDK_NGX_Parameter_SetULL_Original         = nullptr;
NVSDK_NGX_Parameter_GetI_pfn           NVSDK_NGX_Parameter_GetI_Original           = nullptr;
NVSDK_NGX_Parameter_GetUI_pfn          NVSDK_NGX_Parameter_GetUI_Original          = nullptr;
NVSDK_NGX_Parameter_GetULL_pfn         NVSDK_NGX_Parameter_GetULL_Original         = nullptr;
NVSDK_NGX_Parameter_GetVoidPointer_pfn NVSDK_NGX_Parameter_GetVoidPointer_Original = nullptr;

void
NVSDK_CONV
NVSDK_NGX_Parameter_SetF_Detour (NVSDK_NGX_Parameter* InParameter, const char* InName, float InValue)
{
  SK_LOG_FIRST_CALL

  SK_LOGn (((SK_NGX_LogAllParams == true) ? 0 : 1),
              L"NGX_Parameter_SetF (%hs, %f) - %ws", InName, InValue, SK_GetCallerName ().c_str ());

  if (! strcmp (InName, NVSDK_NGX_Parameter_Sharpness))
  {
    if (config.nvidia.dlss.use_sharpening != -1)
    {
      float sharpness = config.nvidia.dlss.use_sharpening == 1 ?
                        config.nvidia.dlss.forced_sharpness  : 0;

      if (InValue != sharpness)
      {
        SK_RunOnce (
        {
          SK_LOGi0 (
            L"Overriding DLSS Sharpness (Requested: %4.2f, Forced: %4.2f)",
              InValue, sharpness
          );
        });

        InValue = sharpness;
      }
    }
  }

  if (! strcmp (InName, NVSDK_NGX_Parameter_FrameTimeDeltaInMsec))
  {
    if (config.nvidia.dlss.calculate_delta_ms)
    {
      const SK_RenderBackend_V2 &rb =
        SK_GetCurrentRenderBackend ();

      const double dFrameTimeDeltaInMsec =
        1000.0 * ( static_cast <double> (rb.frame_delta.getDeltaTime ()) /
                   static_cast <double> (SK_QpcFreq) ) * SK_NGX_IsUsingDLSS_G () ? 2.0
                                                                                 : 1.0;

      NVSDK_NGX_Parameter_SetD_Original (InParameter, InName, dFrameTimeDeltaInMsec);
    }

    else
    {
      NVSDK_NGX_Parameter_SetD_Original (InParameter, InName, InValue);
#if 0
    SK_LOGi0 (L"Frame Time Delta: Game=%f, SK=%f -- %f difference", InValue, dFrameTimeDeltaInMsec, fabs (InValue - dFrameTimeDeltaInMsec));
#endif
    }

    return;
  }

  NVSDK_NGX_Parameter_SetF_Original (InParameter, InName, InValue);
}

void
NVSDK_CONV
NVSDK_NGX_Parameter_SetD_Detour (NVSDK_NGX_Parameter* InParameter, const char* InName, double InValue)
{
  SK_LOG_FIRST_CALL

  SK_LOGn (((SK_NGX_LogAllParams == true) ? 0 : 1),
              L"NGX_Parameter_SetD (%hs, %f) - %ws", InName, InValue, SK_GetCallerName ().c_str ());

  if (! strcmp (InName, NVSDK_NGX_Parameter_Sharpness))
  {
    if (config.nvidia.dlss.use_sharpening != -1)
    {
      float sharpness = config.nvidia.dlss.use_sharpening == 1 ?
                        config.nvidia.dlss.forced_sharpness  : 0;

      if (InValue != sharpness)
      {
        SK_RunOnce (
        {
          SK_LOGi0 (
            L"Overriding DLSS Sharpness (Requested: %4.2f, Forced: %4.2f)",
              InValue, sharpness
          );
        });

        InValue = sharpness;
      }
    }
  }

  if (! strcmp (InName, NVSDK_NGX_Parameter_FrameTimeDeltaInMsec))
  {
    if (config.nvidia.dlss.calculate_delta_ms)
    {
      const SK_RenderBackend_V2 &rb =
        SK_GetCurrentRenderBackend ();

      const double dFrameTimeDeltaInMsec =
        1000.0 * ( static_cast <double> (rb.frame_delta.getDeltaTime ()) /
                   static_cast <double> (SK_QpcFreq) ) * SK_NGX_IsUsingDLSS_G () ? 2.0
                                                                                 : 1.0;

      NVSDK_NGX_Parameter_SetD_Original (InParameter, InName, dFrameTimeDeltaInMsec);

#if 0
      SK_LOGi0 (L"Frame Time Delta: Game=%f, SK=%f -- %f difference", InValue, dFrameTimeDeltaInMsec, fabs (InValue - dFrameTimeDeltaInMsec));
#endif
    }

    else
    {
      NVSDK_NGX_Parameter_SetD_Original (InParameter, InName, InValue);
    }

    return;
  }

  NVSDK_NGX_Parameter_SetD_Original (InParameter, InName, InValue);
}

void
NVSDK_CONV
NVSDK_NGX_Parameter_SetI_Detour (NVSDK_NGX_Parameter* InParameter, const char* InName, int InValue)
{
  SK_LOG_FIRST_CALL

  SK_LOGn (((SK_NGX_LogAllParams == true) ? 0 : 1),
              L"NGX_Parameter_SetI (%hs, %i) - %ws", InName, InValue, SK_GetCallerName ().c_str ());

  if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Feature_Create_Flags))
  {
    if (config.nvidia.dlss.use_sharpening != -1)
    {
      if ((InValue & NVSDK_NGX_DLSS_Feature_Flags_DoSharpening) ==
                     NVSDK_NGX_DLSS_Feature_Flags_DoSharpening)
      {
        if (config.nvidia.dlss.use_sharpening == 0)
        {
          SK_LOGi0 (L"Forcing DLSS Sharpening OFF");

          InValue &= ~NVSDK_NGX_DLSS_Feature_Flags_DoSharpening;
        }
      }

      else
      {
        if (config.nvidia.dlss.use_sharpening == 1)
        {
          SK_LOGi0 (L"Forcing DLSS Sharpening ON");

          InValue |= NVSDK_NGX_DLSS_Feature_Flags_DoSharpening;
        }
      }
    }

    if (config.nvidia.dlss.forced_alpha_upscale != -1)
    {
      if ((InValue & NVSDK_NGX_DLSS_Feature_Flags_AlphaUpscaling) ==
                     NVSDK_NGX_DLSS_Feature_Flags_AlphaUpscaling)
      {
        if (config.nvidia.dlss.forced_alpha_upscale == 0)
        {
          SK_LOGi0 (L"Forcing DLSS Alpha Upscaling OFF");

          InValue &= ~NVSDK_NGX_DLSS_Feature_Flags_AlphaUpscaling;
        }
      }

      else
      {
        if (config.nvidia.dlss.forced_alpha_upscale == 1)
        {
          SK_LOGi0 (L"Forcing DLSS Alpha Upscaling ON");

          InValue |= NVSDK_NGX_DLSS_Feature_Flags_AlphaUpscaling;
        }
      }
    }

    if (config.nvidia.dlss.forced_auto_exposure != -1)
    {
      if ((InValue & NVSDK_NGX_DLSS_Feature_Flags_AutoExposure) ==
                     NVSDK_NGX_DLSS_Feature_Flags_AutoExposure)
      {
        if (config.nvidia.dlss.forced_auto_exposure == 0)
        {
          SK_LOGi0 (L"Forcing DLSS Auto Exposure OFF");

          InValue &= ~NVSDK_NGX_DLSS_Feature_Flags_AutoExposure;
        }
      }

      else
      {
        if (config.nvidia.dlss.forced_auto_exposure == 1)
        {
          SK_LOGi0 (L"Forcing DLSS Auto Exposure ON");

          InValue |= NVSDK_NGX_DLSS_Feature_Flags_AutoExposure;
        }
      }
    }
  }

  if (! strcmp (InName, NVSDK_NGX_Parameter_PerfQualityValue))
  {
    SK_NGX_GameSetPerfQuality = InValue;

    if (config.nvidia.dlss.force_dlaa)
    {
      if (SK_DLSS_Context::dlss_s::hasDLAAQualityLevel ())
      {
        InValue = NVSDK_NGX_PerfQuality_Value_DLAA;
      }
    }
  }

  static std::unordered_set <std::string> presets = {
    NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA,
    NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality,
    NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced,
    NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance,
    NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance
  };

  if (presets.count (InName) != 0)
  {
    if (config.nvidia.dlss.force_dlaa && (config.nvidia.dlss.forced_preset == -1))
    {
      InValue = NVSDK_NGX_DLSS_Hint_Render_Preset_F;
    }

    else if (config.nvidia.dlss.forced_preset != -1)
    {
      InValue = config.nvidia.dlss.forced_preset;
    }
  }

  NVSDK_NGX_Parameter_SetI_Original (InParameter, InName, InValue);
}

void
NVSDK_CONV
NVSDK_NGX_Parameter_SetUI_Detour (NVSDK_NGX_Parameter* InParameter, const char* InName, unsigned int InValue)
{
  SK_LOG_FIRST_CALL

  SK_LOGn (((SK_NGX_LogAllParams == true) ? 0 : 1),
              L"NGX_Parameter_SetUI (%hs, %u) - %ws", InName, InValue, SK_GetCallerName ().c_str ());

  if (! strcmp (InName, NVSDK_NGX_Parameter_PerfQualityValue))
  {
    SK_NGX_GameSetPerfQuality = InValue;

    if (config.nvidia.dlss.force_dlaa)
    {
      if (SK_DLSS_Context::dlss_s::hasDLAAQualityLevel ())
      {
        InValue = NVSDK_NGX_PerfQuality_Value_DLAA;
      }
    }
  }

  static std::unordered_set <std::string> presets = {
    NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA,
    NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality,
    NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced,
    NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance,
    NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance
  };

  if (presets.count (InName) != 0)
  {
    if (config.nvidia.dlss.force_dlaa && (config.nvidia.dlss.forced_preset == -1))
    {
      InValue = NVSDK_NGX_DLSS_Hint_Render_Preset_F;
    }

    else if (config.nvidia.dlss.forced_preset != -1)
    {
      InValue = config.nvidia.dlss.forced_preset;
    }
  }

  NVSDK_NGX_Parameter_SetUI_Original (InParameter, InName, InValue);
}

void
NVSDK_CONV
NVSDK_NGX_Parameter_SetULL_Detour (NVSDK_NGX_Parameter* InParameter, const char* InName, unsigned long long InValue)
{
  SK_LOG_FIRST_CALL

  SK_LOGn (((SK_NGX_LogAllParams == true) ? 0 : 1),
              L"NGX_Parameter_SetULL (%hs, %u) - %ws", InName, InValue, SK_GetCallerName ().c_str ());

  if (! strcmp (InName, NVSDK_NGX_Parameter_PerfQualityValue))
  {
    SK_NGX_GameSetPerfQuality = sk::narrow_cast <unsigned int> (InValue);

    if (config.nvidia.dlss.force_dlaa)
    {
      if (SK_DLSS_Context::dlss_s::hasDLAAQualityLevel ())
      {
        InValue = NVSDK_NGX_PerfQuality_Value_DLAA;
      }
    }
  }

  static std::unordered_set <std::string> presets = {
    NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA,
    NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality,
    NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced,
    NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance,
    NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance
  };

  if (presets.count (InName) != 0)
  {
    if (config.nvidia.dlss.force_dlaa && (config.nvidia.dlss.forced_preset == -1))
    {
      InValue = NVSDK_NGX_DLSS_Hint_Render_Preset_F;
    }

    else if (config.nvidia.dlss.forced_preset != -1)
    {
      InValue = config.nvidia.dlss.forced_preset;
    }
  }

  NVSDK_NGX_Parameter_SetULL_Original (InParameter, InName, InValue);
}

void *SK_NGX_DLSSG_UI_Buffer      = nullptr;
void *SK_NGX_DLSSG_HUDLess_Buffer = nullptr;
void *SK_NGX_DLSSG_Back_Buffer    = nullptr;
void *SK_NGX_DLSSG_MVecs_Buffer   = nullptr;
void *SK_NGX_DLSSG_Depth_Buffer   = nullptr;
bool  SK_NGX_DLSSG_LateInject     = false;

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_Parameter_GetVoidPointer_Detour (const NVSDK_NGX_Parameter *InParameter, const char *InName, void **OutValue)
{
  SK_LOG_FIRST_CALL

  SK_LOGn (((SK_NGX_LogAllParams == true) ? 0 : 1),
              L"NGX_Parameter_GetVoidPointer (%hs) - %ws", InName, SK_GetCallerName ().c_str ());

  if (InName != nullptr)
  {
    if (! strcmp (InName, "DLSSG.UI"))         NVSDK_NGX_Parameter_GetVoidPointer_Original (InParameter, InName, &SK_NGX_DLSSG_UI_Buffer);
    if (! strcmp (InName, "DLSSG.HUDLess"))    NVSDK_NGX_Parameter_GetVoidPointer_Original (InParameter, InName, &SK_NGX_DLSSG_HUDLess_Buffer);
    if (! strcmp (InName, "DLSSG.Depth"))      NVSDK_NGX_Parameter_GetVoidPointer_Original (InParameter, InName, &SK_NGX_DLSSG_Depth_Buffer);
    if (! strcmp (InName, "DLSSG.MVecs"))      NVSDK_NGX_Parameter_GetVoidPointer_Original (InParameter, InName, &SK_NGX_DLSSG_MVecs_Buffer);
    if (! strcmp (InName, "DLSSG.Backbuffer")) NVSDK_NGX_Parameter_GetVoidPointer_Original (InParameter, InName, &SK_NGX_DLSSG_Back_Buffer);
  }

  return
    NVSDK_NGX_Parameter_GetVoidPointer_Original (InParameter, InName, OutValue);
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_Parameter_GetUI_Detour (const NVSDK_NGX_Parameter *InParameter, const char *InName, unsigned int *OutValue)
{
  SK_LOG_FIRST_CALL

  auto ret =
    NVSDK_NGX_Parameter_GetUI_Original (InParameter, InName, OutValue);

  if (ret == NVSDK_NGX_Result_Success)
  {
    SK_LOGn (((SK_NGX_LogAllParams == true) ? 0 : 1),
                L"NGX_Parameter_GetUI (%hs) - %ws", InName, SK_GetCallerName ().c_str ());

    if (config.nvidia.dlss.force_dlaa)
    {
      if (! strcmp (InName, NVSDK_NGX_Parameter_OutWidth))                           { NVSDK_NGX_Parameter_GetUI_Original (InParameter, NVSDK_NGX_Parameter_Width,     OutValue); *OutValue = sk::narrow_cast <UINT> (std::max (1, (int)*OutValue + config.nvidia.dlss.compat.extra_pixels)); }
      if (! strcmp (InName, NVSDK_NGX_Parameter_OutHeight))                          { NVSDK_NGX_Parameter_GetUI_Original (InParameter, NVSDK_NGX_Parameter_Height,    OutValue); *OutValue = sk::narrow_cast <UINT> (std::max (1, (int)*OutValue + config.nvidia.dlss.compat.extra_pixels)); }
      if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Width))  { NVSDK_NGX_Parameter_GetUI_Detour   (InParameter, NVSDK_NGX_Parameter_OutWidth,  OutValue); *OutValue += 2; }
      if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Height)) { NVSDK_NGX_Parameter_GetUI_Detour   (InParameter, NVSDK_NGX_Parameter_OutHeight, OutValue); *OutValue += 2; }
      if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Width))  { NVSDK_NGX_Parameter_GetUI_Detour   (InParameter, NVSDK_NGX_Parameter_OutWidth,  OutValue); *OutValue = static_cast <UINT> (std::max (1, (INT)*OutValue - 2)); }
      if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Height)) { NVSDK_NGX_Parameter_GetUI_Detour   (InParameter, NVSDK_NGX_Parameter_OutHeight, OutValue); *OutValue = static_cast <UINT> (std::max (1, (INT)*OutValue - 2)); }
    }

    else if ( StrStrA (InName, "Width") ||
              StrStrA (InName, "Height") )
    {
      unsigned int dlss_perf_qual;

      InParameter->Get (NVSDK_NGX_Parameter_PerfQualityValue, &dlss_perf_qual);

      float scale         = 0.0f;
      float default_scale = 1.0f;

      switch (dlss_perf_qual)
      {
        case NVSDK_NGX_PerfQuality_Value_MaxPerf:          scale = config.nvidia.dlss.scale.performance;       default_scale = 0.5f;      break;
        case NVSDK_NGX_PerfQuality_Value_Balanced:         scale = config.nvidia.dlss.scale.balanced;          default_scale = 0.58f;     break;
        case NVSDK_NGX_PerfQuality_Value_MaxQuality:       scale = config.nvidia.dlss.scale.quality;           default_scale = 0.666667f; break;
        case NVSDK_NGX_PerfQuality_Value_UltraPerformance: scale = config.nvidia.dlss.scale.ultra_performance; default_scale = 0.333333f; break;
        default:
          break;
      }

      if (scale != 0.0f)
      {
        if (! strcmp (InName, NVSDK_NGX_Parameter_OutWidth))  { NVSDK_NGX_Parameter_GetUI_Original (InParameter, NVSDK_NGX_Parameter_Width,  OutValue); *OutValue = sk::narrow_cast <UINT> (*OutValue * scale); NVSDK_NGX_Parameter_SetUI_Original ((NVSDK_NGX_Parameter *)InParameter, NVSDK_NGX_Parameter_OutWidth,  *OutValue); }
        if (! strcmp (InName, NVSDK_NGX_Parameter_OutHeight)) { NVSDK_NGX_Parameter_GetUI_Original (InParameter, NVSDK_NGX_Parameter_Height, OutValue); *OutValue = sk::narrow_cast <UINT> (*OutValue * scale); NVSDK_NGX_Parameter_SetUI_Original ((NVSDK_NGX_Parameter *)InParameter, NVSDK_NGX_Parameter_OutHeight, *OutValue); }

        if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Width))  { unsigned int ui_max_width = 0; NVSDK_NGX_Parameter_GetUI_Original (InParameter, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Width, &ui_max_width); 
         NVSDK_NGX_Parameter_GetUI_Detour (InParameter, NVSDK_NGX_Parameter_OutWidth, OutValue); *OutValue = std::max (1u, std::min (sk::narrow_cast <UINT> (*OutValue * config.nvidia.dlss.scale.dynamic_max)  + 2, ui_max_width));  }
        if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Height)) { unsigned int ui_max_height = 0; NVSDK_NGX_Parameter_GetUI_Original (InParameter, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Height, &ui_max_height); 
         NVSDK_NGX_Parameter_GetUI_Detour (InParameter, NVSDK_NGX_Parameter_OutHeight, OutValue); *OutValue = std::max (1u, std::min (sk::narrow_cast <UINT> (*OutValue * config.nvidia.dlss.scale.dynamic_max) + 2, ui_max_height)); }
        if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Width))  { unsigned int ui_min_width = 0; NVSDK_NGX_Parameter_GetUI_Original (InParameter, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Width, &ui_min_width); 
         NVSDK_NGX_Parameter_GetUI_Detour (InParameter, NVSDK_NGX_Parameter_OutWidth, OutValue); *OutValue = std::max (1u, std::max (sk::narrow_cast <UINT> (*OutValue * config.nvidia.dlss.scale.dynamic_min)  - 2, ui_min_width));  }
        if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Height)) { unsigned int ui_min_height = 0; NVSDK_NGX_Parameter_GetUI_Original (InParameter, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Height, &ui_min_height); 
         NVSDK_NGX_Parameter_GetUI_Detour (InParameter, NVSDK_NGX_Parameter_OutHeight, OutValue); *OutValue = std::max (1u, std::max (sk::narrow_cast <UINT> (*OutValue * config.nvidia.dlss.scale.dynamic_min) - 2, ui_min_height)); }
      }
    }

    if (! strcmp (InName, NVSDK_NGX_Parameter_PerfQualityValue))
    {
      *OutValue = SK_NGX_GameSetPerfQuality;
    }

    if (config.nvidia.dlss.forced_preset != -1 && StrStrA (InName, "DLSS.Hint.Render.Preset."))
    {
      *OutValue = config.nvidia.dlss.forced_preset;
    }
  }

  return ret;
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_Parameter_GetI_Detour (const NVSDK_NGX_Parameter *InParameter, const char *InName, int *OutValue)
{
  SK_LOG_FIRST_CALL

  auto ret =
    NVSDK_NGX_Parameter_GetI_Original (InParameter, InName, OutValue);

  if (ret == NVSDK_NGX_Result_Success)
  {
    SK_LOGn (((SK_NGX_LogAllParams == true) ? 0 : 1),
                L"NGX_Parameter_GetI (%hs) - %ws", InName, SK_GetCallerName ().c_str ());

    // This stuff doesn't work for signed integers
#if 0
    if (config.nvidia.dlss.force_dlaa)
    {
      if (! strcmp (InName, NVSDK_NGX_Parameter_OutWidth))                           { NVSDK_NGX_Parameter_GetI_Original (InParameter, NVSDK_NGX_Parameter_Width,     OutValue); *OutValue += config.nvidia.dlss.compat.extra_pixels; *OutValue = std::max (1, *OutValue); }
      if (! strcmp (InName, NVSDK_NGX_Parameter_OutHeight))                          { NVSDK_NGX_Parameter_GetI_Original (InParameter, NVSDK_NGX_Parameter_Height,    OutValue); *OutValue += config.nvidia.dlss.compat.extra_pixels; *OutValue = std::max (1, *OutValue); }
      if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Width))  { NVSDK_NGX_Parameter_GetI_Detour   (InParameter, NVSDK_NGX_Parameter_OutWidth,  OutValue); *OutValue += 2; *OutValue = std::max (1, *OutValue); }
      if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Height)) { NVSDK_NGX_Parameter_GetI_Detour   (InParameter, NVSDK_NGX_Parameter_OutHeight, OutValue); *OutValue += 2; *OutValue = std::max (1, *OutValue); }
      if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Width))  { NVSDK_NGX_Parameter_GetI_Detour   (InParameter, NVSDK_NGX_Parameter_OutWidth,  OutValue); *OutValue = std::max (1, (INT)*OutValue - 2); }
      if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Height)) { NVSDK_NGX_Parameter_GetI_Detour   (InParameter, NVSDK_NGX_Parameter_OutHeight, OutValue); *OutValue = std::max (1, (INT)*OutValue - 2); }
    }

    else if ( StrStrA (InName, "Width") ||
              StrStrA (InName, "Height") )
    {
      unsigned int dlss_perf_qual;

      InParameter->Get (NVSDK_NGX_Parameter_PerfQualityValue, &dlss_perf_qual);

      float scale         = 0.0f;
      float default_scale = 1.0f;

      switch (dlss_perf_qual)
      {
        case NVSDK_NGX_PerfQuality_Value_MaxPerf:          scale = config.nvidia.dlss.scale.performance;       default_scale = 0.5f;      break;
        case NVSDK_NGX_PerfQuality_Value_Balanced:         scale = config.nvidia.dlss.scale.balanced;          default_scale = 0.58f;     break;
        case NVSDK_NGX_PerfQuality_Value_MaxQuality:       scale = config.nvidia.dlss.scale.quality;           default_scale = 0.666667f; break;
        case NVSDK_NGX_PerfQuality_Value_UltraPerformance: scale = config.nvidia.dlss.scale.ultra_performance; default_scale = 0.333333f; break;
        default:
          break;
      }

      if (scale != 0.0f)
      {
        if (! strcmp (InName, NVSDK_NGX_Parameter_OutWidth))  { NVSDK_NGX_Parameter_GetI_Original (InParameter, NVSDK_NGX_Parameter_Width,  OutValue); *OutValue = std::max (1, sk::narrow_cast <INT> (*OutValue * scale)); NVSDK_NGX_Parameter_SetI_Original ((NVSDK_NGX_Parameter *)InParameter, NVSDK_NGX_Parameter_OutWidth,  *OutValue); }
        if (! strcmp (InName, NVSDK_NGX_Parameter_OutHeight)) { NVSDK_NGX_Parameter_GetI_Original (InParameter, NVSDK_NGX_Parameter_Height, OutValue); *OutValue = std::max (1, sk::narrow_cast <INT> (*OutValue * scale)); NVSDK_NGX_Parameter_SetI_Original ((NVSDK_NGX_Parameter *)InParameter, NVSDK_NGX_Parameter_OutHeight, *OutValue); }

        if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Width))  { int i_max_width = 0; NVSDK_NGX_Parameter_GetI_Original (InParameter, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Width, &i_max_width); 
         NVSDK_NGX_Parameter_GetI_Detour (InParameter, NVSDK_NGX_Parameter_OutWidth, OutValue); *OutValue = std::max (1, std::min (sk::narrow_cast <INT> (*OutValue * config.nvidia.dlss.scale.dynamic_max)  + 2, i_max_width));  }
        if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Height)) { int i_max_height = 0; NVSDK_NGX_Parameter_GetI_Original (InParameter, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Height, &i_max_height); 
         NVSDK_NGX_Parameter_GetI_Detour (InParameter, NVSDK_NGX_Parameter_OutHeight, OutValue); *OutValue = std::max (1, std::min (sk::narrow_cast <INT> (*OutValue * config.nvidia.dlss.scale.dynamic_max) + 2, i_max_height)); }
        if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Width))  { int i_min_width = 0; NVSDK_NGX_Parameter_GetI_Original (InParameter, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Width, &i_min_width); 
         NVSDK_NGX_Parameter_GetI_Detour (InParameter, NVSDK_NGX_Parameter_OutWidth, OutValue); *OutValue = std::max (1, std::max (sk::narrow_cast <INT> (*OutValue * config.nvidia.dlss.scale.dynamic_min)  - 2, i_min_width));  }
        if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Height)) { int i_min_height = 0; NVSDK_NGX_Parameter_GetI_Original (InParameter, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Height, &i_min_height); 
         NVSDK_NGX_Parameter_GetI_Detour (InParameter, NVSDK_NGX_Parameter_OutHeight, OutValue); *OutValue = std::max (1, std::max (sk::narrow_cast <INT> (*OutValue * config.nvidia.dlss.scale.dynamic_min) - 2, i_min_height)); }
      }
    }
#endif

    if (! strcmp (InName, NVSDK_NGX_Parameter_PerfQualityValue))
    {
      *OutValue = SK_NGX_GameSetPerfQuality;
    }

    if (config.nvidia.dlss.forced_preset != -1 && StrStrA (InName, "DLSS.Hint.Render.Preset."))
    {
      *OutValue = config.nvidia.dlss.forced_preset;
    }
  }

  return ret;
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_Parameter_GetULL_Detour (const NVSDK_NGX_Parameter *InParameter, const char *InName, unsigned long long *OutValue)
{
  SK_LOG_FIRST_CALL

  auto ret =
    NVSDK_NGX_Parameter_GetULL_Original (InParameter, InName, OutValue);

  return ret;

  if (ret == NVSDK_NGX_Result_Success)
  {
    SK_LOGn (((SK_NGX_LogAllParams == true) ? 0 : 1),
                L"NGX_Parameter_GetULL (%hs) - %ws", InName, SK_GetCallerName ().c_str ());

    if (config.nvidia.dlss.force_dlaa)
    {
      if (! strcmp (InName, NVSDK_NGX_Parameter_OutWidth))                           { NVSDK_NGX_Parameter_GetULL_Original (InParameter, NVSDK_NGX_Parameter_Width,     OutValue); *OutValue = static_cast <ULONGLONG> (std::max (1LL, (LONGLONG)*OutValue + config.nvidia.dlss.compat.extra_pixels)); }
      if (! strcmp (InName, NVSDK_NGX_Parameter_OutHeight))                          { NVSDK_NGX_Parameter_GetULL_Original (InParameter, NVSDK_NGX_Parameter_Height,    OutValue); *OutValue = static_cast <ULONGLONG> (std::max (1LL, (LONGLONG)*OutValue + config.nvidia.dlss.compat.extra_pixels)); }
      if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Width))  { NVSDK_NGX_Parameter_GetULL_Detour   (InParameter, NVSDK_NGX_Parameter_OutWidth,  OutValue); *OutValue += 2; }
      if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Height)) { NVSDK_NGX_Parameter_GetULL_Detour   (InParameter, NVSDK_NGX_Parameter_OutHeight, OutValue); *OutValue += 2; }
      if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Width))  { NVSDK_NGX_Parameter_GetULL_Detour   (InParameter, NVSDK_NGX_Parameter_OutWidth,  OutValue); *OutValue = static_cast <ULONGLONG> (std::max (1LL, (LONGLONG)*OutValue - 2LL)); }
      if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Height)) { NVSDK_NGX_Parameter_GetULL_Detour   (InParameter, NVSDK_NGX_Parameter_OutHeight, OutValue); *OutValue = static_cast <ULONGLONG> (std::max (1LL, (LONGLONG)*OutValue - 2LL)); }
    }

    else if ( StrStrA (InName, "Width") ||
              StrStrA (InName, "Height") )
    {
      unsigned int dlss_perf_qual;

      InParameter->Get (NVSDK_NGX_Parameter_PerfQualityValue, &dlss_perf_qual);

      float scale         = 0.0f;
      float default_scale = 1.0f;

      switch (dlss_perf_qual)
      {
        case NVSDK_NGX_PerfQuality_Value_MaxPerf:          scale = config.nvidia.dlss.scale.performance;       default_scale = 0.5f;      break;
        case NVSDK_NGX_PerfQuality_Value_Balanced:         scale = config.nvidia.dlss.scale.balanced;          default_scale = 0.58f;     break;
        case NVSDK_NGX_PerfQuality_Value_MaxQuality:       scale = config.nvidia.dlss.scale.quality;           default_scale = 0.666667f; break;
        case NVSDK_NGX_PerfQuality_Value_UltraPerformance: scale = config.nvidia.dlss.scale.ultra_performance; default_scale = 0.333333f; break;
        default:
          break;
      }

      if (scale != 0.0f)
      {
        if (! strcmp (InName, NVSDK_NGX_Parameter_OutWidth))  { NVSDK_NGX_Parameter_GetULL_Original (InParameter, NVSDK_NGX_Parameter_Width,  OutValue); *OutValue = sk::narrow_cast <unsigned long long> (*OutValue * scale); NVSDK_NGX_Parameter_SetULL_Original ((NVSDK_NGX_Parameter *)InParameter, NVSDK_NGX_Parameter_OutWidth,  *OutValue); }
        if (! strcmp (InName, NVSDK_NGX_Parameter_OutHeight)) { NVSDK_NGX_Parameter_GetULL_Original (InParameter, NVSDK_NGX_Parameter_Height, OutValue); *OutValue = sk::narrow_cast <unsigned long long> (*OutValue * scale); NVSDK_NGX_Parameter_SetULL_Original ((NVSDK_NGX_Parameter *)InParameter, NVSDK_NGX_Parameter_OutHeight, *OutValue); }

        if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Width))  { unsigned long long ui_max_width = 0; NVSDK_NGX_Parameter_GetULL_Original (InParameter, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Width, &ui_max_width); 
         NVSDK_NGX_Parameter_GetULL_Detour (InParameter, NVSDK_NGX_Parameter_OutWidth, OutValue); *OutValue = std::max (1ull, std::min (sk::narrow_cast <unsigned long long> (*OutValue * config.nvidia.dlss.scale.dynamic_max)  + 2, ui_max_width));  }
        if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Height)) { unsigned long long ui_max_height = 0; NVSDK_NGX_Parameter_GetULL_Original (InParameter, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Height, &ui_max_height); 
         NVSDK_NGX_Parameter_GetULL_Detour (InParameter, NVSDK_NGX_Parameter_OutHeight, OutValue); *OutValue = std::max (1ull, std::min (sk::narrow_cast <unsigned long long> (*OutValue * config.nvidia.dlss.scale.dynamic_max) + 2, ui_max_height)); }
        if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Width))  { unsigned long long ui_min_width = 0; NVSDK_NGX_Parameter_GetULL_Original (InParameter, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Width, &ui_min_width); 
         NVSDK_NGX_Parameter_GetULL_Detour (InParameter, NVSDK_NGX_Parameter_OutWidth, OutValue); *OutValue = std::max (1ull, std::max (sk::narrow_cast <unsigned long long> (*OutValue * config.nvidia.dlss.scale.dynamic_min)  - 2, ui_min_width));  }
        if (! strcmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Height)) { unsigned long long ui_min_height = 0; NVSDK_NGX_Parameter_GetULL_Original (InParameter, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Height, &ui_min_height); 
         NVSDK_NGX_Parameter_GetULL_Detour (InParameter, NVSDK_NGX_Parameter_OutHeight, OutValue); *OutValue = std::max (1ull, std::max (sk::narrow_cast <unsigned long long> (*OutValue * config.nvidia.dlss.scale.dynamic_min) - 2, ui_min_height)); }
      }
    }

    if (! strcmp (InName, NVSDK_NGX_Parameter_PerfQualityValue))
    {
      *OutValue = SK_NGX_GameSetPerfQuality;
    }

    if (config.nvidia.dlss.forced_preset != -1 && StrStrA (InName, "DLSS.Hint.Render.Preset."))
    {
      *OutValue = config.nvidia.dlss.forced_preset;
    }
  }

  return ret;
}

using NVSDK_NGX_UpdateFeature_pfn =
      NVSDK_NGX_Result (NVSDK_CONV *)( const NVSDK_NGX_Application_Identifier *ApplicationId,
                                       const NVSDK_NGX_Feature                 FeatureID );

static NVSDK_NGX_UpdateFeature_pfn
       NVSDK_NGX_UpdateFeature_Original = nullptr;

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_UpdateFeature_Detour ( const NVSDK_NGX_Application_Identifier *ApplicationId,
                                 const NVSDK_NGX_Feature                 FeatureID )
{
  SK_LOG_FIRST_CALL

  if (config.nvidia.dlss.disable_ota_updates)
  {
    SK_LOGi0 (L"DLSS OTA Updates Forced OFF");

    return NVSDK_NGX_Result_Success;
  }

  NVSDK_NGX_Result ret =
    NVSDK_NGX_UpdateFeature_Original (ApplicationId, FeatureID);

  if (ret != NVSDK_NGX_Result_Success)
  {
    SK_LOGi0 (
      L"NVSDK_NGX_UpdateFeature (..., %d, ...) Failed - %x (%ws)",
        FeatureID, ret, GetNGXResultAsString != nullptr ?
                        GetNGXResultAsString (ret)      :
                        L"Unknown Result"
    );
  }

  return ret;
}

using NVSDK_NGX_DLSS_GetStatsCallback_pfn           = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter *InParams);
using NVSDK_NGX_DLSS_GetOptimalSettingsCallback_pfn = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter *InParams);

NVSDK_NGX_Result NVSDK_CONV NVSDK_NGX_DLSS_GetOptimalSettingsCallback (NVSDK_NGX_Parameter* InParams);
NVSDK_NGX_Result NVSDK_CONV NVSDK_NGX_DLSS_GetStatsCallback           (NVSDK_NGX_Parameter* InParams);

NVSDK_NGX_Parameter*
SK_NGX_GetDLSSParameters (void)
{
  if (     SK_NGX_DLSS12.apis_called)
    return SK_NGX_DLSS12.super_sampling.Parameters;

  return
    SK_NGX_DLSS11.super_sampling.Parameters;
}

SK_DLSS_Context::version_s SK_DLSS_Context::dlss_s::Version;
SK_DLSS_Context::version_s SK_DLSS_Context::dlssg_s::Version;

void
SK_NGX_EstablishDLSSVersion (void) noexcept
{
  static bool bHasVersion = false;

  if (bHasVersion)
    return;

  if (! GetModuleHandleW (L"nvngx_dlss.dll"))
    return;

  std::swscanf (
    SK_GetDLLVersionShort (L"nvngx_dlss.dll").c_str (), L"%d,%d,%d,%d",
      &SK_DLSS_Context::dlss_s::Version.major, &SK_DLSS_Context::dlss_s::Version.minor,
      &SK_DLSS_Context::dlss_s::Version.build, &SK_DLSS_Context::dlss_s::Version.revision
  );

  bHasVersion = true;

  // Turn off overrides before we break stuff!
  if (SK_DLSS_Context::dlss_s::Version.major < 2)
  {
    config.nvidia.dlss.auto_redirect_dlss = false;
    config.nvidia.dlss.forced_preset      = -1;
    config.nvidia.dlss.use_sharpening     = -1;
    config.nvidia.dlss.force_dlaa         = false;
  }
}

void
SK_NGX_EstablishDLSSGVersion (void) noexcept
{
  static bool bHasVersion = false;

  if (bHasVersion)
    return;

  if (! GetModuleHandleW (L"nvngx_dlssg.dll"))
    return;

  std::swscanf (
    SK_GetDLLVersionShort (L"nvngx_dlssg.dll").c_str (), L"%d,%d,%d,%d",
      &SK_DLSS_Context::dlssg_s::Version.major, &SK_DLSS_Context::dlssg_s::Version.minor,
      &SK_DLSS_Context::dlssg_s::Version.build, &SK_DLSS_Context::dlssg_s::Version.revision
  );

  bHasVersion = true;
}

SK_DLSS_Context::version_s
SK_NGX_GetDLSSVersion (void) noexcept
{
  if (     SK_NGX_DLSS12.apis_called)
    return SK_NGX_DLSS12.super_sampling.Version;

  return SK_NGX_DLSS11.super_sampling.Version;
}

SK_DLSS_Context::version_s
SK_NGX_GetDLSSGVersion (void) noexcept
{
  if (     SK_NGX_DLSS12.apis_called)
    return SK_NGX_DLSS12.frame_gen.Version;

  return SK_NGX_DLSS11.frame_gen.Version;
}

bool
SK_NGX_HookParameters (NVSDK_NGX_Parameter* Params)
{
  if (Params == nullptr)
    return false;

  if (NVSDK_NGX_Parameter_GetUI_Original != nullptr)
    return true;

  void** vftable = *(void***)*&Params;

  // [ 0] void* SetVoidPointer;
  // [ 1] void* SetD3d12Resource;
  // [ 2] void *SetD3d11Resource;
	// [ 3] void* SetI;
	// [ 4] void* SetUI;
	// [ 5] void* SetD;
	// [ 6] void* SetF;
	// [ 7] void* SetULL;
	// [ 8] void* GetVoidPointer;
	// [ 9] void* GetD3d12Resource;
	// [10] void* GetD3d11Resource;
  // [11] void *GetI;
  // [12] void *GetUI;
	// [13] void* GetD;
	// [14] void* GetF;
	// [15] void* GetULL;
	// [16] void* Reset;

  SK_CreateFuncHook (      L"NVSDK_NGX_Parameter::SetI",
                             vftable [3],
                             NVSDK_NGX_Parameter_SetI_Detour,
    static_cast_p2p <void> (&NVSDK_NGX_Parameter_SetI_Original) );

  SK_CreateFuncHook (      L"NVSDK_NGX_Parameter::SetUI",
                             vftable [4],
                             NVSDK_NGX_Parameter_SetUI_Detour,
    static_cast_p2p <void> (&NVSDK_NGX_Parameter_SetUI_Original) );

  SK_CreateFuncHook (      L"NVSDK_NGX_Parameter::SetD",
                             vftable [5],
                             NVSDK_NGX_Parameter_SetD_Detour,
    static_cast_p2p <void> (&NVSDK_NGX_Parameter_SetD_Original) );

  SK_CreateFuncHook (      L"NVSDK_NGX_Parameter::SetF",
                             vftable [6],
                             NVSDK_NGX_Parameter_SetF_Detour,
    static_cast_p2p <void> (&NVSDK_NGX_Parameter_SetF_Original) );

  SK_CreateFuncHook (      L"NVSDK_NGX_Parameter::SetULL",
                             vftable [7],
                             NVSDK_NGX_Parameter_SetULL_Detour,
    static_cast_p2p <void> (&NVSDK_NGX_Parameter_SetULL_Original) );

  SK_CreateFuncHook (      L"NVSDK_NGX_Parameter::GetVoidPointer",
                             vftable [8],
                             NVSDK_NGX_Parameter_GetVoidPointer_Detour,
    static_cast_p2p <void> (&NVSDK_NGX_Parameter_GetVoidPointer_Original) );

  SK_CreateFuncHook (      L"NVSDK_NGX_Parameter::GetI",
                             vftable [11],
                             NVSDK_NGX_Parameter_GetI_Detour,
    static_cast_p2p <void> (&NVSDK_NGX_Parameter_GetI_Original) );

  SK_CreateFuncHook (      L"NVSDK_NGX_Parameter::GetUI",
                             vftable [12],
                             NVSDK_NGX_Parameter_GetUI_Detour,
    static_cast_p2p <void> (&NVSDK_NGX_Parameter_GetUI_Original) );

  SK_CreateFuncHook (      L"NVSDK_NGX_Parameter::GetULL",
                             vftable [15],
                             NVSDK_NGX_Parameter_GetULL_Detour,
    static_cast_p2p <void> (&NVSDK_NGX_Parameter_GetULL_Original) );

  MH_QueueEnableHook (vftable [3]);
  MH_QueueEnableHook (vftable [4]);
  MH_QueueEnableHook (vftable [5]);
  MH_QueueEnableHook (vftable [6]);
  MH_QueueEnableHook (vftable [7]);
  MH_QueueEnableHook (vftable [8]);
  MH_QueueEnableHook (vftable [11]);
  MH_QueueEnableHook (vftable [12]);
  MH_QueueEnableHook (vftable [15]);

  SK_ApplyQueuedHooks ();

  return true;
}

void
SK_NGX_DLSS_CreateFeatureOverrideParams (NVSDK_NGX_Parameter *InParameters)
{
  int                                                                create_flags = 0x0;
  InParameters->Get (NVSDK_NGX_Parameter_DLSS_Feature_Create_Flags, &create_flags);

  // Trigger our hook in case we missed the setup of creation flags earlier
  NVSDK_NGX_Parameter_SetI_Detour (InParameters, NVSDK_NGX_Parameter_DLSS_Feature_Create_Flags, create_flags);

  if (config.nvidia.dlss.use_sharpening == 1)
    InParameters->Set (NVSDK_NGX_Parameter_Sharpness, config.nvidia.dlss.forced_sharpness);

  if (config.nvidia.dlss.forced_alpha_upscale == 1)
  {
    // Check if user is trying to force Alpha Upscaling on, with an incompatible DLL version...
    if (! SK_DLSS_Context::dlss_s::hasAlphaUpscaling ())
    {
      SK_LOGi0 (L"Alpha Upscaling requested, but DLSS version does not support it.");
    }
  }

  if (config.nvidia.dlss.force_dlaa)
  {
    if (config.nvidia.dlss.forced_preset == -1)
    {
      unsigned int preset =
        NVSDK_NGX_DLSS_Hint_Render_Preset_F;

      NVSDK_NGX_Parameter_SetUI_Original (InParameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA,             preset);
      NVSDK_NGX_Parameter_SetUI_Original (InParameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality,          preset);
      NVSDK_NGX_Parameter_SetUI_Original (InParameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced,         preset);
      NVSDK_NGX_Parameter_SetUI_Original (InParameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance,      preset);
      NVSDK_NGX_Parameter_SetUI_Original (InParameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance, preset);
    }

    if (SK_DLSS_Context::dlss_s::hasDLAAQualityLevel ())
    {
      int perf_quality =
        NVSDK_NGX_PerfQuality_Value_DLAA;

      NVSDK_NGX_Parameter_SetI_Original (InParameters, NVSDK_NGX_Parameter_PerfQualityValue, perf_quality);
    }
  }
}

extern void SK_NGX_InitD3D11 (void);
extern void SK_NGX_InitD3D12 (void);

extern void SK_NGX12_UpdateDLSSGStatus (void);
extern void SK_NGX11_UpdateDLSSGStatus (void);

void
SK_NGX_UpdateDLSSGStatus (void)
{
  SK_NGX11_UpdateDLSSGStatus ();
  SK_NGX12_UpdateDLSSGStatus ();
}

void
SK_NGX_Init (void)
{
  SK_RunOnce (
  {
    GetNGXResultAsString =
   (GetNGXResultAsString_pfn)SK_GetProcAddress (L"_nvngx.dll",
   "GetNGXResultAsString");

    using NVSDK_NGX_D3D12_GetParameters_pfn = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter **OutParameters);

    SK_NGX_InitD3D11 ();
    SK_NGX_InitD3D12 ();

    SK_CreateDLLHook2 (L"_nvngx.dll",
                         "NVSDK_NGX_UpdateFeature",
                          NVSDK_NGX_UpdateFeature_Detour,
                (void **)&NVSDK_NGX_UpdateFeature_Original);

    auto _GetParameters =
      (NVSDK_NGX_D3D12_GetParameters_pfn)SK_GetProcAddress (L"_nvngx.dll",
                                                             "NVSDK_NGX_D3D12_GetParameters");

    if (NVSDK_NGX_Parameter *Params = nullptr;
                         _GetParameters != nullptr)
    {
      if (NVSDK_NGX_Result_Success == _GetParameters (&Params) && Params != nullptr)
      {
        // If successful, this applies the hooks queued in InitD3D11 and InitD3D12
        if (SK_NGX_HookParameters (Params))
          return;
      }
    }

    // HookParameters was not successful -- apply queued hooks before returning.
    SK_ApplyQueuedHooks ();
  });
}

DWORD SK_DLSS_Context::dlss_s::IndicatorFlags  = DWORD_MAX;
DWORD SK_DLSS_Context::dlssg_s::IndicatorFlags = DWORD_MAX;

void
SK_DLSS_Context::dlss_s::showIndicator (bool show)
{
  const wchar_t* wszPath =
    LR"(HKEY_LOCAL_MACHINE\SOFTWARE\NVIDIA Corporation\Global\NGXCore)";
  const wchar_t *wszKey  = L"ShowDlssIndicator";
  const wchar_t *wszType = L"dword";
  
  DWORD dwValue =
    ( show ? 0x400 : 0x0 );

  wchar_t   wszValue [64] = { };
  wsprintf (wszValue, L"%08x", dwValue);

  SK_ImportRegistryValue (wszPath, wszKey, wszType, wszValue, true);

  SK_DLSS_Context::dlss_s::IndicatorFlags = dwValue;
}

bool
SK_DLSS_Context::dlss_s::isIndicatorShown (void)
{
  if (SK_DLSS_Context::dlss_s::IndicatorFlags != DWORD_MAX)
  {
    return
      (SK_DLSS_Context::dlss_s::IndicatorFlags & 0x400) == 0x400;
  }

  DWORD     len    =      MAX_PATH;
  LSTATUS   status =
    RegGetValueW ( HKEY_LOCAL_MACHINE,
                     LR"(SOFTWARE\NVIDIA Corporation\Global\NGXCore\)",
                                      L"ShowDlssIndicator",
                       RRF_RT_REG_DWORD,
                         nullptr,
                           (PVOID)&SK_DLSS_Context::dlss_s::IndicatorFlags,
                             (LPDWORD)&len );

  if (status == ERROR_SUCCESS)
    return (SK_DLSS_Context::dlss_s::IndicatorFlags & 0x400) == 0x400;
  else
  {
    // Don't call this repeatedly on failure
    SK_DLSS_Context::dlss_s::IndicatorFlags = 0x0;

    return false;
  }
}

void
SK_DLSS_Context::dlssg_s::showIndicator (bool show)
{
  const wchar_t* wszPath =
    LR"(HKEY_LOCAL_MACHINE\SOFTWARE\NVIDIA Corporation\Global\NGXCore)";
  const wchar_t *wszKey  = L"DLSSG_IndicatorText";
  const wchar_t *wszType = L"dword";

  DWORD dwValue =
    ( show ? 0x2 : 0x0 );

  wchar_t   wszValue [64] = { };
  wsprintf (wszValue, L"%08x", dwValue);

  SK_ImportRegistryValue (wszPath, wszKey, wszType, wszValue, true);

  SK_DLSS_Context::dlssg_s::IndicatorFlags = dwValue;
}

bool
SK_DLSS_Context::dlssg_s::isIndicatorShown (void)
{
  if (SK_DLSS_Context::dlssg_s::IndicatorFlags != DWORD_MAX)
  {
    return
      (SK_DLSS_Context::dlssg_s::IndicatorFlags & 0x2) == 0x2;
  }

  DWORD     len    =      MAX_PATH;
  LSTATUS   status =
    RegGetValueW ( HKEY_LOCAL_MACHINE,
                     LR"(SOFTWARE\NVIDIA Corporation\Global\NGXCore\)",
                                      L"DLSSG_IndicatorText",
                       RRF_RT_REG_DWORD,
                         nullptr,
                           (PVOID)&SK_DLSS_Context::dlssg_s::IndicatorFlags,
                             (LPDWORD)&len );

  if (status == ERROR_SUCCESS)
    return (SK_DLSS_Context::dlssg_s::IndicatorFlags & 0x2) == 0x2;
  else
  {
    // Don't call this repeatedly on failure
    SK_DLSS_Context::dlssg_s::IndicatorFlags = 0x0;

    return false;
  }
}

// We do not know the lifetime of these resources, expect them to occasionally be invalid pointers.
void
SK_SEH_NGX_GetInternalResolutionFromDLSS ( ID3D12Resource* pD3D12, ID3D11Resource* pD3D11,
                                           unsigned int&   width,  unsigned int&   height )
{
  __try
  {
    if (pD3D12 != nullptr)
    {
      width  = sk::narrow_cast <unsigned int> (pD3D12->GetDesc ().Width);
      height = sk::narrow_cast <unsigned int> (pD3D12->GetDesc ().Height);
    }

    else if (pD3D11 != nullptr)
    {
      ID3D11Texture2D *pTex2D = nullptr;

      pD3D11->QueryInterface <ID3D11Texture2D> (&pTex2D);

      if (pTex2D != nullptr)
      {
        D3D11_TEXTURE2D_DESC texDesc = { };
        pTex2D->GetDesc (&texDesc);

        width  = texDesc.Width;
        height = texDesc.Height;

        pTex2D->Release ();
      }
    }
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    width  = 0;
    height = 0;
  }
}

const char*
SK_NGX_DLSS_GetCurrentPerfQualityStr (void)
{
  auto params =
    SK_NGX_GetDLSSParameters ();

  if (! params)
  {
    return "Unknown Performance/Quality Mode";
  }

  unsigned int perf_quality = NVSDK_NGX_PerfQuality_Value_MaxPerf;

  NVSDK_NGX_Parameter_GetUI_Original (params, NVSDK_NGX_Parameter_PerfQualityValue, &perf_quality);

  switch (perf_quality)
  {
    case NVSDK_NGX_PerfQuality_Value_MaxPerf:
      return "Performance";
      break;
    case NVSDK_NGX_PerfQuality_Value_Balanced:
      return "Balanced";
      break;
    case NVSDK_NGX_PerfQuality_Value_MaxQuality:
      return "Quality";
      break;
    case NVSDK_NGX_PerfQuality_Value_UltraPerformance:
      return "Ultra Performance";
      break;
    case NVSDK_NGX_PerfQuality_Value_UltraQuality:
      return "Ultra Quality";
      break;
    case NVSDK_NGX_PerfQuality_Value_DLAA:
      return "DLAA";
      break;
    default:
      return "Unknown Performance/Quality Mode";
      break;
  }
}

const char*
SK_NGX_DLSS_GetCurrentPresetStr (void)
{
  auto params =
    SK_NGX_GetDLSSParameters ();

  if (! params)
  {
    return "Unknown/Invalid";
  }

  unsigned int preset;
  unsigned int perf_quality = NVSDK_NGX_PerfQuality_Value_MaxPerf;

  NVSDK_NGX_Parameter_GetUI_Original (params, NVSDK_NGX_Parameter_PerfQualityValue, &perf_quality);

  const char *szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA;

  switch (perf_quality)
  {
    case NVSDK_NGX_PerfQuality_Value_MaxPerf:           szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance;      break;
    case NVSDK_NGX_PerfQuality_Value_Balanced:          szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced;         break;
    case NVSDK_NGX_PerfQuality_Value_MaxQuality:        szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality;          break;
    // Extended PerfQuality modes                                  
    case NVSDK_NGX_PerfQuality_Value_UltraPerformance:  szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance; break;
    case NVSDK_NGX_PerfQuality_Value_UltraQuality:      szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraQuality;     break;
    case NVSDK_NGX_PerfQuality_Value_DLAA:              szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA;             break;
    default:
      break;
  }
  
  NVSDK_NGX_Parameter_GetUI_Original (params, szPresetHint, &preset);

  switch (preset)
  {
    case NVSDK_NGX_DLSS_Hint_Render_Preset_Default: return "Default";      break;
    case NVSDK_NGX_DLSS_Hint_Render_Preset_A:       return "A";            break;
    case NVSDK_NGX_DLSS_Hint_Render_Preset_B:       return "B";            break;
    case NVSDK_NGX_DLSS_Hint_Render_Preset_C:       return "C";            break;
    case NVSDK_NGX_DLSS_Hint_Render_Preset_D:       return "D";            break;
    case NVSDK_NGX_DLSS_Hint_Render_Preset_E:       return "E";            break;
    case NVSDK_NGX_DLSS_Hint_Render_Preset_F:       return "F";            break;
    case NVSDK_NGX_DLSS_Hint_Render_Preset_G:       return "G";            break;
    default:                                        return "DLSS Default"; break;
      break;
  }
}

void
SK_NGX_DLSS_GetResolution (int& x, int& y, int& out_x, int& out_y)
{ 
  if (NVSDK_NGX_Parameter_GetUI_Original == nullptr)
  {
    x = 0; y = 0;
    return;
  }

  auto params =
    SK_NGX_GetDLSSParameters ();

  if (params == nullptr)
  {
    x = 0; y = 0;
    return;
  }

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  unsigned int     width = 0,     height = 0;
  unsigned int out_width = 0, out_height = 0;
  
  NVSDK_NGX_Parameter_GetUI_Original (params, NVSDK_NGX_Parameter_Width,     &width);
  NVSDK_NGX_Parameter_GetUI_Original (params, NVSDK_NGX_Parameter_Height,    &height);
  NVSDK_NGX_Parameter_GetUI_Original (params, NVSDK_NGX_Parameter_OutWidth,  &out_width);
  NVSDK_NGX_Parameter_GetUI_Original (params, NVSDK_NGX_Parameter_OutHeight, &out_height);
  
  out_width  = std::max ( width,  out_width);
  out_height = std::max (height, out_height);
  
  //
  // Get the ACTUAL internal resolution by querying the underlying Direct3D resources
  //   and using their desc. Many games are not setting the Height/Width parameters.
  //
  ID3D12Resource                           *pD3D12Resource = nullptr;
  params->Get (NVSDK_NGX_Parameter_Color, &pD3D12Resource);
  ID3D11Resource                           *pD3D11Resource = nullptr;
  params->Get (NVSDK_NGX_Parameter_Color, &pD3D11Resource);
  
  NVSDK_NGX_Parameter_GetUI_Original (params, NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Width,  &width);
  NVSDK_NGX_Parameter_GetUI_Original (params, NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Height, &height);
  
  if (width == 0 || height == 0)
  {
    unsigned int res_width  = 0;
    unsigned int res_height = 0;
  
    if (                             nullptr != pD3D12Resource||pD3D11Resource != nullptr)
      SK_SEH_NGX_GetInternalResolutionFromDLSS (pD3D12Resource, pD3D11Resource, res_width, res_height);
  
    if (width  == 0) width  = res_width;
    if (height == 0) height = res_height;
  }

  x     = width;
  y     = height;
  out_x = out_width;
  out_y = out_height;
}

void
SK_NGX_DLSS_ControlPanel (void)
{
  if (SK_NGX_IsUsingDLSS () || SK_NGX_IsUsingDLSS_G ())
  {
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));
    ImGui::TreePush       ("");

    if (ImGui::CollapsingHeader ("NVIDIA DLSS", ImGuiTreeNodeFlags_DefaultOpen))
    {
      if (config.nvidia.dlss.show_active_features)
      {
        ImGui::SameLine ();

        const bool bFrameGen = SK_NGX_IsUsingDLSS_G ();
        const bool bRayRecon = SK_NGX_IsUsingDLSS_D ();

        ImGui::TextColored     ( bFrameGen ? ImVec4 (0.0f, 0.8f, 0.0f, 1.0f) : ImVec4 (0.8f, 0.0f, 0.0f, 1.0f),
                                 bFrameGen ? "     " ICON_FA_CHECK   : "     " ICON_FA_XMARK );
        ImGui::SameLine        ();
        ImGui::TextUnformatted ("Frame Generation\t");
        ImGui::SameLine        ();
        ImGui::TextColored     ( bRayRecon ? ImVec4 (0.0f, 0.8f, 0.0f, 1.0f) : ImVec4 (0.8f, 0.0f, 0.0f, 1.0f),
                                 bRayRecon ? ICON_FA_CHECK                   : ICON_FA_XMARK );
        ImGui::SameLine        ();
        ImGui::TextUnformatted ("Ray Reconstruction");
      }

      ImGui::TreePush     ("");

      auto params =
        SK_NGX_GetDLSSParameters ();
  
      if (params != nullptr)
      {
        static auto path_to_plugin_dlss =
          std::filesystem::path (
            SK_GetPlugInDirectory (SK_PlugIn_Type::ThirdParty)
          ) / L"NVIDIA" / L"nvngx_dlss.dll";

        static std::error_code                          ec;
        static bool bHasPlugInDLSS =
          std::filesystem::exists (path_to_plugin_dlss, ec);

        static std::wstring path_to_dlss =
          SK_GetModuleFullName (SK_GetModuleHandleW (L"nvngx_dlss.dll"));

        static std::filesystem::path dlss_directory =
               std::filesystem::path (path_to_dlss).remove_filename ();

        static auto dlss_version =
          SK_NGX_GetDLSSVersion ();

        static auto dlssg_version =
          SK_NGX_GetDLSSGVersion ();

        int                                               dlss_creation_flags = 0x0;
        params->Get (
          NVSDK_NGX_Parameter_DLSS_Feature_Create_Flags, &dlss_creation_flags);

        // Removed in 2.5.1
        static const bool bHasSharpening =
          SK_DLSS_Context::dlss_s::hasSharpening ();

        static const bool bHasDLAAQualityLevel =
          SK_DLSS_Context::dlss_s::hasDLAAQualityLevel ();

        static const bool bHasPresetE =
          SK_DLSS_Context::dlss_s::hasPresetE ();

        static const bool bHasAlphaUpscaling =
          SK_DLSS_Context::dlss_s::hasAlphaUpscaling () &&
          ( config.nvidia.dlss.forced_alpha_upscale != -1 ||
              (dlss_creation_flags & NVSDK_NGX_DLSS_Feature_Flags_AlphaUpscaling) ==
                                     NVSDK_NGX_DLSS_Feature_Flags_AlphaUpscaling );

        static const bool bHasAutoExposure =
          config.nvidia.dlss.forced_auto_exposure != -1 ||
            (dlss_creation_flags & NVSDK_NGX_DLSS_Feature_Flags_AutoExposure) ==
                                   NVSDK_NGX_DLSS_Feature_Flags_AutoExposure;

        static bool restart_required = false;
  
        unsigned int     width,     height;
        unsigned int out_width, out_height;
  
        NVSDK_NGX_Parameter_GetUI_Original (params, NVSDK_NGX_Parameter_Width,     &width);
        NVSDK_NGX_Parameter_GetUI_Original (params, NVSDK_NGX_Parameter_Height,    &height);
        NVSDK_NGX_Parameter_GetUI_Original (params, NVSDK_NGX_Parameter_OutWidth,  &out_width);
        NVSDK_NGX_Parameter_GetUI_Original (params, NVSDK_NGX_Parameter_OutHeight, &out_height);

        out_width  = std::max ( width,  out_width);
        out_height = std::max (height, out_height);

        //
        // Get the ACTUAL internal resolution by querying the underlying Direct3D resources
        //   and using their desc. Many games are not setting the Height/Width parameters.
        //
        ID3D12Resource                           *pD3D12Resource = nullptr;
        params->Get (NVSDK_NGX_Parameter_Color, &pD3D12Resource);
        ID3D11Resource                           *pD3D11Resource = nullptr;
        params->Get (NVSDK_NGX_Parameter_Color, &pD3D11Resource);

        NVSDK_NGX_Parameter_GetUI_Original (params, NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Width,  &width);
        NVSDK_NGX_Parameter_GetUI_Original (params, NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Height, &height);

        if (width == 0 || height == 0)
        {
          unsigned int res_width;
          unsigned int res_height;

          SK_SEH_NGX_GetInternalResolutionFromDLSS (pD3D12Resource, pD3D11Resource, res_width, res_height);

          if (width  == 0) width  = res_width;
          if (height == 0) height = res_height;
        }

        unsigned int perf_quality = NVSDK_NGX_PerfQuality_Value_MaxPerf;

        NVSDK_NGX_Parameter_GetUI_Original (params, NVSDK_NGX_Parameter_PerfQualityValue, &perf_quality);

        ImGui::BeginGroup ();
        ImGui::BeginGroup ();
        ImGui::
          TextUnformatted ("Internal Resolution: ");
        ImGui::
          TextUnformatted ("Upscaled Resolution: ");
        if (bHasAutoExposure)
        {
          ImGui::Spacing  ();
          ImGui::
          TextUnformatted ("Auto Exposure:       ");
        }
        if (bHasAlphaUpscaling)
        {
          ImGui::Spacing  ();
          ImGui::
          TextUnformatted ("Alpha Upscaling:     ");
        }
        ImGui::Spacing ();
        ImGui::
          TextUnformatted ("DLSS Preset:         ");
        ImGui::Spacing ();
        ImGui::Spacing ();
        ImGui::
          TextUnformatted ("DLSS Perf/Quality:   ");
        if (! config.nvidia.dlss.force_dlaa)
        {
          if (ImGui::IsItemHovered ())
          {
            ImGui::SetTooltip ("Right-click to define custom resolution scales.");
          }

          if (ImGui::IsItemClicked (ImGuiPopupFlags_MouseButtonRight))
          {
            ImGui::OpenPopup ("DLSS_PerfQuality_Popup");
          }
        }
        ImGui::EndGroup   ();
        ImGui::SameLine   ();
        ImGui::BeginGroup ();
        ImGui::Text       ("%dx%d",     width,     height);
        ImGui::Text       ("%dx%d", out_width, out_height);

        static const char* szPerfQuality = "DLAA";
        static const char* szPreset      = "DLSS Default";

        const float fItemSpacingX =
          std::max ({
               ImGui::CalcTextSize (szPerfQuality).x,
               ImGui::CalcTextSize (szPreset).x,
               ImGui::CalcTextSize ("Off").x
          }) + ImGui::GetStyle ().ItemSpacing.x;

        const float fComboBoxWidth =
          ImGui::CalcTextSize ("Game Default\t  ").x + ImGui::GetStyle ().FramePadding.x * 2;

        if (bHasAutoExposure)
        {
          int force_auto_exposure =
            config.nvidia.dlss.forced_auto_exposure + 1;
  
          ImVec2 vCursor = ImGui::GetCursorPos ();

          ImGui::Spacing ();

          if (dlss_creation_flags & NVSDK_NGX_DLSS_Feature_Flags_AutoExposure)
            ImGui::TextUnformatted ("On");
          else
            ImGui::TextUnformatted ("Off");

          ImGui::SetCursorPos (
            ImVec2 (vCursor.x + fItemSpacingX, vCursor.y)
          );

          ImGui::SetNextItemWidth (
            fComboBoxWidth
          );
  
          if ( ImGui::Combo (
                 "###Auto_Exposure",
             &force_auto_exposure, "Game Default\0"
                                   "Force Off\0"
                                   "Force On\0\0") )
          {
            config.nvidia.dlss.forced_auto_exposure =
              force_auto_exposure - 1;

            restart_required = true;

            config.utility.save_async ();
          }
        }

        if (bHasAlphaUpscaling)
        {
          int force_alpha_upscaling =
            config.nvidia.dlss.forced_alpha_upscale + 1;

          ImVec2 vCursor = ImGui::GetCursorPos ();

          if (dlss_creation_flags & NVSDK_NGX_DLSS_Feature_Flags_AlphaUpscaling)
            ImGui::TextUnformatted ("On");
          else
            ImGui::TextUnformatted ("Off");

          ImGui::SetCursorPos (
            ImVec2 (vCursor.x + fItemSpacingX, vCursor.y)
          );

          ImGui::SetNextItemWidth (
            fComboBoxWidth
          );
  
          if ( ImGui::Combo (
                 "###Alpha_Upscaling",
             &force_alpha_upscaling, "Game Default\0"
                                     "Force Off\0\0"/*
                                     "Force On\0\0" */) )
          { // Forcing on is meaningless, game needs special support
            config.nvidia.dlss.forced_alpha_upscale =
              force_alpha_upscaling - 1;
            restart_required = true;

            config.utility.save_async ();
          }

          if (ImGui::IsItemHovered ())
          {
            ImGui::SetTooltip ("Requires DLSS 3.7.0+ and enabling may degrade application performance.");
          }
        }

        unsigned int preset;

        const char *szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA;

        switch (perf_quality)
        {
          case NVSDK_NGX_PerfQuality_Value_MaxPerf:           szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance;      break;
          case NVSDK_NGX_PerfQuality_Value_Balanced:          szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced;         break;
          case NVSDK_NGX_PerfQuality_Value_MaxQuality:        szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality;          break;
          // Extended PerfQuality modes                                  
          case NVSDK_NGX_PerfQuality_Value_UltraPerformance:  szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance; break;
          case NVSDK_NGX_PerfQuality_Value_UltraQuality:      szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraQuality;     break;
          case NVSDK_NGX_PerfQuality_Value_DLAA:              szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA;             break;
          default:
            break;
        }

        NVSDK_NGX_Parameter_GetUI_Original (params, szPresetHint, &preset);

        switch (preset)
        {
          case NVSDK_NGX_DLSS_Hint_Render_Preset_Default: szPreset = "Default"; break;
          case NVSDK_NGX_DLSS_Hint_Render_Preset_A:       szPreset = "A";       break;
          case NVSDK_NGX_DLSS_Hint_Render_Preset_B:       szPreset = "B";       break;
          case NVSDK_NGX_DLSS_Hint_Render_Preset_C:       szPreset = "C";       break;
          case NVSDK_NGX_DLSS_Hint_Render_Preset_D:       szPreset = "D";       break;
          case NVSDK_NGX_DLSS_Hint_Render_Preset_E:       szPreset = "E";       break;
          case NVSDK_NGX_DLSS_Hint_Render_Preset_F:       szPreset = "F";       break;
          case NVSDK_NGX_DLSS_Hint_Render_Preset_G:       szPreset = "G";       break;
          default:
            break;
        }

        ImGui::BeginGroup      ();
        ImVec2 vCursor = ImGui::GetCursorPos ();
        ImGui::Spacing         ();
        ImGui::TextUnformatted (szPreset);
        ImGui::EndGroup        ();

        ImGui::SetCursorPos (
          ImVec2 (vCursor.x + fItemSpacingX, vCursor.y)
        );

        int preset_override = config.nvidia.dlss.forced_preset + 1;

        ImGui::SetNextItemWidth (
          fComboBoxWidth
        );

        if ( ImGui::Combo ( "",
                            &preset_override, "Game Default\0"
                                              "DLSS Default\0"
                                              "Override: A\0"
                                              "Override: B\0"
                                              "Override: C\0"
                                              "Override: D\0"
                                              "Override: E (DLSS 3.7+)\0"
                                              "Override: F\0"
                                              "Override: G (Invalid)\0", 9 )
           )
        {
          config.nvidia.dlss.forced_preset = preset_override - 1;

          if (config.nvidia.dlss.forced_preset != -1)
          {
            NVSDK_NGX_Parameter_SetUI_Original (params, szPresetHint, config.nvidia.dlss.forced_preset);
          }

          SK_NGX_Reset ();

          restart_required = true;

          config.utility.save_async ();
        }

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip    ();
          ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.f, 1.f, 1.f, 1.f));
          ImGui::TextUnformatted ("Preset A");
          ImGui::PopStyleColor   ();
          ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (.7f, .7f, .7f, 1.f));
          ImGui::BulletText      ("Intended for Performance/Balanced/Quality modes.");
          ImGui::BulletText      ("An older variant best suited to combat ghosting for elements with missing inputs, such as motion vectors.");
          ImGui::PopStyleColor   ();
          ImGui::Spacing         ();
          ImGui::Spacing         ();
          ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.f, 1.f, 1.f, 1.f));
          ImGui::TextUnformatted ("Preset B");
          ImGui::PopStyleColor   ();
          ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (.7f, .7f, .7f, 1.f));
          ImGui::BulletText      ("Intended for Ultra Performance mode.");
          ImGui::BulletText      ("Similar to Preset A but for Ultra Performance mode.");
          ImGui::PopStyleColor   ();
          ImGui::Spacing         ();
          ImGui::Spacing         ();
          ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.f, 1.f, 1.f, 1.f));
          ImGui::TextUnformatted ("Preset C");
          ImGui::PopStyleColor   ();
          ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (.7f, .7f, .7f, 1.f));
          ImGui::BulletText      ("Intended for Performance/Balanced/Quality modes.");
          ImGui::BulletText      ("Generally favors current frame information; well suited for fast-paced game content.");
          ImGui::PopStyleColor   ();
          ImGui::Spacing         ();
          ImGui::Spacing         ();
          ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.f, 1.f, 1.f, 1.f));
          ImGui::TextUnformatted ("Preset D");
          ImGui::PopStyleColor   ();
          ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (.7f, .7f, .7f, 1.f));
          if (! bHasPresetE)
          ImGui::BulletText      ("Default preset for Performance/Balanced/Quality modes; generally favors image stability.");
          else
          if (bHasPresetE)
            ImGui::BulletText    ("Default preset for Perf/Balanced/Quality modes (pre-3.7.0); generally favors image stability.");
          ImGui::PopStyleColor   ();
          ImGui::Spacing         ();
          ImGui::Spacing         ();
          ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.f, 1.f, 1.f, 1.f));
          ImGui::TextUnformatted ("Preset E");
          ImGui::PopStyleColor   ();
          if (! bHasPresetE)
          {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4 (.5f, .5f, .5f, 1.f));
            ImGui::SameLine      ();
            ImGui::TextUnformatted("\tNot supported by the current version of DLSS");
            ImGui::PopStyleColor ();
          }
          else
          {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4 (.7f, .7f, .7f, 1.f));
            ImGui::BulletText    ("The default preset for Perf/Balanced/Quality mode (3.7.0+); generally favors image stability.");
            ImGui::PopStyleColor ();
          }
          ImGui::Spacing         ();
          ImGui::Spacing         ();
          ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.f, 1.f, 1.f, 1.f));
          ImGui::TextUnformatted ("Preset F");
          ImGui::PopStyleColor   ();
          ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (.7f, .7f, .7f, 1.f));
          ImGui::BulletText      ("Default preset for Ultra Performance and DLAA modes.");
          ImGui::PopStyleColor   ();
          ImGui::Spacing         ();
          ImGui::Spacing         ();
          ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.f, 1.f, 1.f, 1.f));
          ImGui::TextUnformatted ("Preset G");
          ImGui::PopStyleColor   ();
          ImGui::SameLine        ();
          ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (.5f, .5f, .5f, 1.f));
          ImGui::TextUnformatted ("\tNot currently used");
          ImGui::PopStyleColor   ();
          ImGui::Separator       ();
          ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (.62f, .62f, .62f, 1.f));
          ImGui::TextUnformatted ("Descriptions valid as of DLSS 3.7, defaults refer to 'DLSS Default' and are subject to change between DLL versions.");
          ImGui::PopStyleColor   ();
          ImGui::EndTooltip      ();
        }
  
        ImGui::BeginGroup ();
        vCursor = ImGui::GetCursorPos ();
        ImGui::Spacing    ();

        if (config.nvidia.dlss.force_dlaa && (! SK_DLSS_Context::dlss_s::hasDLAAQualityLevel ()))
        {
          ImGui::TextColored (ImVec4 (.55f, .55f, 1.f, 1.f), ICON_FA_INFO);
          ImGui::SameLine ();
        }

        switch (perf_quality)
        {
          case NVSDK_NGX_PerfQuality_Value_MaxPerf:
            szPerfQuality = "Performance";
            break;
          case NVSDK_NGX_PerfQuality_Value_Balanced:
            szPerfQuality = "Balanced";
            break;
          case NVSDK_NGX_PerfQuality_Value_MaxQuality:
            szPerfQuality = "Quality";
            break;
          case NVSDK_NGX_PerfQuality_Value_UltraPerformance:
            szPerfQuality = "Ultra Performance";
            break;
          case NVSDK_NGX_PerfQuality_Value_UltraQuality:
            szPerfQuality = "Ultra Quality";
            break;
          case NVSDK_NGX_PerfQuality_Value_DLAA:
            szPerfQuality = "DLAA";
            break;
          default:
            szPerfQuality = "Unknown Performance/Quality Mode";
            break;
        }
        ImGui::TextUnformatted (szPerfQuality);
        ImGui::EndGroup ();

        ImGui::SetCursorPos (
          ImVec2 (vCursor.x + fItemSpacingX, vCursor.y)
        );

        if (config.nvidia.dlss.force_dlaa && (! SK_DLSS_Context::dlss_s::hasDLAAQualityLevel ()))
        {
          if (ImGui::IsItemHovered ())
          {
            ImGui::SetTooltip (
              "The DLSS version in-use does not have a DLAA Perf/Quality level, "
              "but DLAA is active if Internal/Upscaled resolution are equal."
            );
          }
        }

        else if (! config.nvidia.dlss.force_dlaa)
        {
          if (ImGui::IsItemHovered ())
          {
            ImGui::SetTooltip ("Right-click to define custom resolution scales.");
          }

          if (ImGui::IsItemClicked (ImGuiPopupFlags_MouseButtonRight))
          {
            ImGui::OpenPopup ("DLSS_PerfQuality_Popup");
          }

          if (ImGui::BeginPopup ("DLSS_PerfQuality_Popup"))
          {
            const SK_RenderBackend& rb =
              SK_GetCurrentRenderBackend ();

            SK_ComQIPtr <IDXGISwapChain>
               pSwapChain (rb.swapchain);

            DXGI_SWAP_CHAIN_DESC swapDesc = { };

            if (pSwapChain.p != nullptr)
                pSwapChain->GetDesc (&swapDesc);

            auto _ScaleOverride = [&](float &cfg_var, float fDefaultScale, const char *szName)
            {
              bool override = cfg_var != 0.0f;

              ImGui::PushID (szName);
              if (ImGui::Checkbox ("", &override))
              {
                if (override)
                  cfg_var = fDefaultScale;
                else
                  cfg_var = 0.0f;

                restart_required = true;

                config.utility.save_async ();
              }

              ImGui::SameLine ();

              if (! override)
                ImGui::BeginDisabled ();

              float* scale = override ?
                             &cfg_var : &fDefaultScale;

              char      fmt [128] = { };
              sprintf ( fmt, "%%6.4f\t(%ix%i)", static_cast <INT> (swapDesc.BufferDesc.Width  * *scale),
                                                static_cast <INT> (swapDesc.BufferDesc.Height * *scale) );

              if (ImGui::SliderFloat (szName, scale, 0.01f, 0.999f, fmt))
              {
                restart_required = true;

                config.utility.save_async ();
              }

              if (! override)
                ImGui::EndDisabled ();
              ImGui::PopID ();
            };

            ImGui::BeginGroup ();
            _ScaleOverride (config.nvidia.dlss.scale.ultra_performance, 0.333333f, "Ultra Performance");
            _ScaleOverride (config.nvidia.dlss.scale.performance,       0.5f,      "Performance");
            _ScaleOverride (config.nvidia.dlss.scale.balanced,          0.58f,     "Balanced");
            _ScaleOverride (config.nvidia.dlss.scale.quality,           0.666667f, "Quality");
            ImGui::EndGroup  ();
            ImGui::Separator ();

            ImGui::BulletText ("Dynamic Resolution Min/Max Applies only to Overridden Resolution Scales");
            ImGui::BulletText ("Set both to 100%% to disable Dynamic Resolution Scaling");
            ImGui::BulletText ("These settings do nothing if the game does not use DRS");

            float percent =
              config.nvidia.dlss.scale.dynamic_min * 100.0f;

            if (ImGui::SliderFloat ("Dynamic Resolution Minimum", &percent, 1.0f, 100.0f, "%6.2f%%"))
            {
              config.nvidia.dlss.scale.dynamic_min = percent / 100.0f;

              restart_required = true;

              config.utility.save_async ();
            }

            percent =
              config.nvidia.dlss.scale.dynamic_max * 100.0f;

            if (ImGui::SliderFloat ("Dynamic Resolution Maximum", &percent, 100.0f, 200.0f, "%6.2f%%"))
            {
              config.nvidia.dlss.scale.dynamic_max = percent / 100.0f;

              restart_required = true;

              config.utility.save_async ();
            }

            ImGui::EndPopup ();
          }
        }

        if (dlss_version.major > 1)
        {
          if (ImGui::Checkbox ("Force DLAA", &config.nvidia.dlss.force_dlaa))
          {
            restart_required = true;

            SK_NGX_Reset ();

            config.utility.save_async ();
          }

          if (ImGui::IsItemHovered ())
          {
            if (bHasDLAAQualityLevel)
              ImGui::SetTooltip ("For best results, set game's DLSS mode = Auto/Ultra Performance if it has them.");
            else
              ImGui::SetTooltip ("For best results, upgrade DLSS DLL to 3.1.13 or newer.");
          }
        }

        ImGui::EndGroup ();
  
        if (bHasSharpening)
        {
          float                                        fSharpness;
          params->Get (NVSDK_NGX_Parameter_Sharpness, &fSharpness);
  
          int use_sharpening =
            config.nvidia.dlss.use_sharpening + 1;
  
          if ( ImGui::Combo (
                 "Sharpening",
             &use_sharpening, "Game Default\0"
                              "Force Off\0"
                              "Force On\0\0") )
          {
            config.nvidia.dlss.use_sharpening =
              use_sharpening - 1;
            restart_required = true;

            config.utility.save_async ();
          }
  
          if (use_sharpening == 0 && (dlss_creation_flags & NVSDK_NGX_DLSS_Feature_Flags_DoSharpening))
            ImGui::Text ("Sharpness: %4.2f", fSharpness);
          else if (use_sharpening == 2)
          {
            fSharpness = config.nvidia.dlss.forced_sharpness;

            if (ImGui::SliderFloat ("Sharpness", &fSharpness, -1.0f, 1.0f))
            {
                    config.nvidia.dlss.forced_sharpness = fSharpness;
              params->Set (NVSDK_NGX_Parameter_Sharpness, fSharpness);

              restart_required = true;

              SK_NGX_Reset ();

              config.utility.save_async ();
            }
          }
        }
  
        if (restart_required)
        {
          ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f).Value);
          ImGui::BulletText     ("Game Restart (or Alt+Enter) May Be Required");
          ImGui::PopStyleColor  ();
        }
        if (SK_NGX_DLSSG_LateInject && SK_NGX_IsUsingDLSS_G ())
        {
          float fAlpha = 
            0.5f + ((sin ((float)(SK::ControlPanel::current_time % 2500) / 2500.0f))) / 2.0f;

          ImGui::TextColored     (ImColor::HSV (.166667f,1.0f,fAlpha).Value, ICON_FA_EXCLAMATION_TRIANGLE);
          ImGui::SameLine        ();
          ImGui::TextUnformatted (" Possible Frame Generation Conflict Detected");
          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip    ();
            ImGui::TextUnformatted ("Streamline Interposer was loaded before Special K");
            ImGui::Separator       ();
            ImGui::BulletText      ("SK's UI may be drawn as part of DLSS Frame Generation and full of artifacts.");
            ImGui::BulletText      ("Framepacing and FPS counting may be inaccurate (raw FPS), but limiting will work.");
            ImGui::BulletText      ("Try setting GlobalInjectDelay=0.01 or higher, or using Local Injection to fix.");
            ImGui::EndTooltip      ();
          }
        }
        ImGui::EndGroup    ();
        ImGui::SameLine    ();
        ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine    ();
        ImGui::BeginGroup  ();

        ImGui::Text ( "DLSS Version:\t%d.%d.%d\t", dlss_version.major, dlss_version.minor,
                                                   dlss_version.build );

        if (dlssg_version.major > 0)
        {
          ImGui::SameLine ();

          ImGui::Text ( "DLSS-G Version:\t%d.%d.%d", dlssg_version.major, dlssg_version.minor,
                                                     dlssg_version.build );
        }

        static bool bRestartNeeded = false;

        // Only offer option to replace DLSS DLLs in DLSS 2.x games
        if (dlss_version.major > 1)
        {
          if (bHasPlugInDLSS)
          {
            if (ImGui::Checkbox ("Use Special K's DLSS instead of Game's DLL", &config.nvidia.dlss.auto_redirect_dlss))
            {
              bRestartNeeded = true;

              config.utility.save_async ();
            }
          }

          else
          {
            static bool clicked_once = false;

            bool bClicked =
              ImGui::Selectable (ICON_FA_INFO_CIRCLE " Auto-Load a Newer DLSS DLL...");

            clicked_once |= bClicked;

            if (ImGui::IsItemHovered ())
            {
              ImGui::BeginTooltip ();
              ImGui::BulletText   ("Click to Create the Plug-In Directory for Auto-Load.");
              ImGui::BulletText   ("Place nvngx_dlss.dll in the Directory.");
              ImGui::EndTooltip   ();
            }

            if (bClicked)
            {
              static std::filesystem::path dlss_plugin_directory =
                     std::filesystem::path (path_to_plugin_dlss).remove_filename ();

              SK_CreateDirectories (       dlss_plugin_directory.c_str ());
              if (std::filesystem::exists (dlss_plugin_directory, ec))
                SK_Util_ExplorePath (      dlss_plugin_directory.wstring ().c_str ());
              else
              {
                clicked_once = false;
              }
            }

            if (clicked_once)
            {
              bHasPlugInDLSS =
                std::filesystem::exists (path_to_plugin_dlss, ec);
            }
          }

          if ((! bHasPlugInDLSS) || (! config.nvidia.dlss.auto_redirect_dlss))
          {
            if (ImGui::Button ("Browse DLSS Directory"))
            {
              SK_Util_ExplorePath (dlss_directory.wstring ().c_str ());
            }

            ImGui::SameLine ();
          }
        }
        
        if (ImGui::TreeNode ("DLSS Indicators"))
        {
          ImGui::BeginGroup ();

          bool show_dlss =
            SK_DLSS_Context::dlss_s::isIndicatorShown ();
          bool show_dlssg =
            SK_DLSS_Context::dlssg_s::isIndicatorShown ();

          if (ImGui::Checkbox ("Show DLSS", &show_dlss))
          {
            SK_DLSS_Context::dlss_s::showIndicator (show_dlss);

            bRestartNeeded = true;
          }

          if (ImGui::IsItemHovered ())
          {
            ImGui::SetTooltip ("Shows the official NVIDIA text overlay (bottom-left) for DLSS / DLSS Ray Reconstruction.");
          }

          if (ImGui::Checkbox ("Show DLSS-G", &show_dlssg))
          {
            SK_DLSS_Context::dlssg_s::showIndicator (show_dlssg);

            bRestartNeeded = true;
          }

          if (ImGui::IsItemHovered ())
          {
            ImGui::SetTooltip ("Shows the official NVIDIA text overlay (top-left) for DLSS Frame Generation.");
          }

          ImGui::EndGroup    ();
          ImGui::SameLine    ();
          ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
          ImGui::SameLine    ();
          ImGui::BeginGroup  ();
          if (ImGui::Checkbox ("Show Active Features", &config.nvidia.dlss.show_active_features))
          {
            config.utility.save_async ();
          }

          if (ImGui::IsItemHovered ())
          {
            ImGui::SetTooltip ("Displays which DLSS extensions are in use in the DLSS header.");
          }
        }
        else
          ImGui::BeginGroup ();

        bool bShowCompatHacks =
          ImGui::TreeNode ("Compatibility Hacks");

        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip ("Compatibility hacks to fix DLSS black screens in some games");
        }

        if (bShowCompatHacks)
        {
          bool bDLAAMinus2 = config.nvidia.dlss.compat.extra_pixels != 0;

          if (ImGui::Checkbox ("DLAA -2 Pixels", &bDLAAMinus2))
          {
            if (! bDLAAMinus2)
              config.nvidia.dlss.compat.extra_pixels = 0;
            else
              config.nvidia.dlss.compat.extra_pixels = -2;

            restart_required = true;

            config.utility.save_async ();
          }

          bool bFakeGenericAppID =
            config.nvidia.dlss.compat.override_appid != -1;

          if (ImGui::Checkbox ("Fake Generic AppID", &bFakeGenericAppID))
          {
            if (bFakeGenericAppID)
              config.nvidia.dlss.compat.override_appid = 0x24480451;
            else
              config.nvidia.dlss.compat.override_appid = -1;

            restart_required = true;

            config.utility.save_async ();
          }

          if (ImGui::Checkbox ("Disable Over-The-Air Updates", &config.nvidia.dlss.disable_ota_updates))
          {
            config.utility.save_async ();
          }

          ImGui::TreePop ();
        }

        if (bRestartNeeded)
        {
          ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (.1f, .8f, .9f));
          ImGui::BulletText     ("Game Restart Required");
          ImGui::PopStyleColor  ();
        }
        ImGui::EndGroup   ();
        ImGui::EndGroup   ();
      }

      ImGui::TreePop     ( );
    }
  
    ImGui::TreePop       ( );
    ImGui::PopStyleColor (3);
  }
}

void
SK_NGX_Reset (void)
{
  WriteULong64Release (&SK_NGX_DLSS11.super_sampling.ResetFrame, SK_GetFramesDrawn () + 1);
  WriteULong64Release (&SK_NGX_DLSS12.super_sampling.ResetFrame, SK_GetFramesDrawn () + 1);
}

void
SK_NGX_DumpParameters (const NVSDK_NGX_Parameter *Params)
{
  auto param_list = std::vector <const char *> {
    NVSDK_NGX_Parameter_OptLevel                                   ,
    NVSDK_NGX_Parameter_IsDevSnippetBranch                         ,
    NVSDK_NGX_Parameter_SuperSampling_ScaleFactor                  ,
    NVSDK_NGX_Parameter_ImageSignalProcessing_ScaleFactor          ,
    NVSDK_NGX_Parameter_SuperSampling_Available                    ,
    NVSDK_NGX_Parameter_InPainting_Available                       ,
    NVSDK_NGX_Parameter_ImageSuperResolution_Available             ,
    NVSDK_NGX_Parameter_SlowMotion_Available                       ,
    NVSDK_NGX_Parameter_VideoSuperResolution_Available             ,
    NVSDK_NGX_Parameter_ImageSignalProcessing_Available            ,
    NVSDK_NGX_Parameter_DeepResolve_Available                      ,
    NVSDK_NGX_Parameter_DeepDVC_Available                          ,
    NVSDK_NGX_Parameter_SuperSampling_NeedsUpdatedDriver           ,
    NVSDK_NGX_Parameter_InPainting_NeedsUpdatedDriver              ,
    NVSDK_NGX_Parameter_ImageSuperResolution_NeedsUpdatedDriver    ,
    NVSDK_NGX_Parameter_SlowMotion_NeedsUpdatedDriver              ,
    NVSDK_NGX_Parameter_VideoSuperResolution_NeedsUpdatedDriver    ,
    NVSDK_NGX_Parameter_ImageSignalProcessing_NeedsUpdatedDriver   ,
    NVSDK_NGX_Parameter_DeepResolve_NeedsUpdatedDriver             ,
    NVSDK_NGX_Parameter_DeepDVC_NeedsUpdatedDriver                 ,
    NVSDK_NGX_Parameter_FrameInterpolation_NeedsUpdatedDriver      ,
    NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMajor        ,
    NVSDK_NGX_Parameter_InPainting_MinDriverVersionMajor           ,
    NVSDK_NGX_Parameter_ImageSuperResolution_MinDriverVersionMajor ,
    NVSDK_NGX_Parameter_SlowMotion_MinDriverVersionMajor           ,
    NVSDK_NGX_Parameter_VideoSuperResolution_MinDriverVersionMajor ,
    NVSDK_NGX_Parameter_ImageSignalProcessing_MinDriverVersionMajor,
    NVSDK_NGX_Parameter_DeepResolve_MinDriverVersionMajor          ,
    NVSDK_NGX_Parameter_DeepDVC_MinDriverVersionMajor              ,
    NVSDK_NGX_Parameter_FrameInterpolation_MinDriverVersionMajor   ,
    NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMinor        ,
    NVSDK_NGX_Parameter_InPainting_MinDriverVersionMinor           ,
    NVSDK_NGX_Parameter_ImageSuperResolution_MinDriverVersionMinor ,
    NVSDK_NGX_Parameter_SlowMotion_MinDriverVersionMinor           ,
    NVSDK_NGX_Parameter_VideoSuperResolution_MinDriverVersionMinor ,
    NVSDK_NGX_Parameter_ImageSignalProcessing_MinDriverVersionMinor,
    NVSDK_NGX_Parameter_DeepResolve_MinDriverVersionMinor          ,
    NVSDK_NGX_Parameter_DeepDVC_MinDriverVersionMinor              ,
    NVSDK_NGX_Parameter_SuperSampling_FeatureInitResult            ,
    NVSDK_NGX_Parameter_InPainting_FeatureInitResult               ,
    NVSDK_NGX_Parameter_ImageSuperResolution_FeatureInitResult     ,
    NVSDK_NGX_Parameter_SlowMotion_FeatureInitResult               ,
    NVSDK_NGX_Parameter_VideoSuperResolution_FeatureInitResult     ,
    NVSDK_NGX_Parameter_ImageSignalProcessing_FeatureInitResult    ,
    NVSDK_NGX_Parameter_DeepResolve_FeatureInitResult              ,
    NVSDK_NGX_Parameter_DeepDVC_FeatureInitResult                  ,
    NVSDK_NGX_Parameter_FrameInterpolation_FeatureInitResult       ,
    NVSDK_NGX_Parameter_ImageSuperResolution_ScaleFactor_2_1       ,
    NVSDK_NGX_Parameter_ImageSuperResolution_ScaleFactor_3_1       ,
    NVSDK_NGX_Parameter_ImageSuperResolution_ScaleFactor_3_2       ,
    NVSDK_NGX_Parameter_ImageSuperResolution_ScaleFactor_4_3       ,
    NVSDK_NGX_Parameter_DeepDVC_Strength                           ,
    NVSDK_NGX_Parameter_NumFrames                                  ,
    NVSDK_NGX_Parameter_Scale                                      ,
    NVSDK_NGX_Parameter_Width                                      ,
    NVSDK_NGX_Parameter_Height                                     ,
    NVSDK_NGX_Parameter_OutWidth                                   ,
    NVSDK_NGX_Parameter_OutHeight                                  ,
    NVSDK_NGX_Parameter_Sharpness                                  ,
    NVSDK_NGX_Parameter_Scratch                                    ,
    NVSDK_NGX_Parameter_Scratch_SizeInBytes                        ,
    NVSDK_NGX_Parameter_Input1                                     ,
    NVSDK_NGX_Parameter_Input1_Format                              ,
    NVSDK_NGX_Parameter_Input1_SizeInBytes                         ,
    NVSDK_NGX_Parameter_Input2                                     ,
    NVSDK_NGX_Parameter_Input2_Format                              ,
    NVSDK_NGX_Parameter_Input2_SizeInBytes                         ,
    NVSDK_NGX_Parameter_Color                                      ,
    NVSDK_NGX_Parameter_Color_Format                               ,
    NVSDK_NGX_Parameter_Color_SizeInBytes                          ,
    NVSDK_NGX_Parameter_FI_Color1                                  ,
    NVSDK_NGX_Parameter_FI_Color2                                  ,
    NVSDK_NGX_Parameter_Albedo                                     ,
    NVSDK_NGX_Parameter_Output                                     ,
    NVSDK_NGX_Parameter_Output_SizeInBytes                         ,
    NVSDK_NGX_Parameter_FI_Output1                                 ,
    NVSDK_NGX_Parameter_FI_Output2                                 ,
    NVSDK_NGX_Parameter_FI_Output3                                 ,
    NVSDK_NGX_Parameter_Reset                                      ,
    NVSDK_NGX_Parameter_BlendFactor                                ,
    NVSDK_NGX_Parameter_MotionVectors                              ,
    NVSDK_NGX_Parameter_FI_MotionVectors1                          ,
    NVSDK_NGX_Parameter_FI_MotionVectors2                          ,
    NVSDK_NGX_Parameter_Rect_X                                     ,
    NVSDK_NGX_Parameter_Rect_Y                                     ,
    NVSDK_NGX_Parameter_Rect_W                                     ,
    NVSDK_NGX_Parameter_Rect_H                                     ,
    NVSDK_NGX_Parameter_MV_Scale_X                                 ,
    NVSDK_NGX_Parameter_MV_Scale_Y                                 ,
    NVSDK_NGX_Parameter_Model                                      ,
    NVSDK_NGX_Parameter_Format                                     ,
    NVSDK_NGX_Parameter_SizeInBytes                                ,
    NVSDK_NGX_Parameter_ResourceAllocCallback                      ,
    NVSDK_NGX_Parameter_BufferAllocCallback                        ,
    NVSDK_NGX_Parameter_Tex2DAllocCallback                         ,
    NVSDK_NGX_Parameter_ResourceReleaseCallback                    ,
    NVSDK_NGX_Parameter_CreationNodeMask                           ,
    NVSDK_NGX_Parameter_VisibilityNodeMask                         ,
    NVSDK_NGX_Parameter_MV_Offset_X                                ,
    NVSDK_NGX_Parameter_MV_Offset_Y                                ,
    NVSDK_NGX_Parameter_Hint_UseFireflySwatter                     ,
    NVSDK_NGX_Parameter_Resource_Width                             ,
    NVSDK_NGX_Parameter_Resource_Height                            ,
    NVSDK_NGX_Parameter_Resource_OutWidth                          ,
    NVSDK_NGX_Parameter_Resource_OutHeight                         ,
    NVSDK_NGX_Parameter_Depth                                      ,
    NVSDK_NGX_Parameter_FI_Depth1                                  ,
    NVSDK_NGX_Parameter_FI_Depth2                                  ,
    NVSDK_NGX_Parameter_DLSSOptimalSettingsCallback                ,
    NVSDK_NGX_Parameter_DLSSGetStatsCallback                       ,
    NVSDK_NGX_Parameter_PerfQualityValue                           ,
    NVSDK_NGX_Parameter_RTXValue                                   ,
    NVSDK_NGX_Parameter_DLSSMode                                   ,
    NVSDK_NGX_Parameter_FI_Mode                                    ,
    NVSDK_NGX_Parameter_FI_OF_Preset                               ,
    NVSDK_NGX_Parameter_FI_OF_GridSize                             ,
    NVSDK_NGX_Parameter_Jitter_Offset_X                            ,
    NVSDK_NGX_Parameter_Jitter_Offset_Y                            ,
    NVSDK_NGX_Parameter_Denoise                                    ,
    NVSDK_NGX_Parameter_TransparencyMask                           ,
    NVSDK_NGX_Parameter_ExposureTexture                            ,
    NVSDK_NGX_Parameter_DLSS_Feature_Create_Flags                  ,
    NVSDK_NGX_Parameter_DLSS_Checkerboard_Jitter_Hack              ,
    NVSDK_NGX_Parameter_GBuffer_Normals                            ,
    NVSDK_NGX_Parameter_GBuffer_Albedo                             ,
    NVSDK_NGX_Parameter_GBuffer_Roughness                          ,
    NVSDK_NGX_Parameter_GBuffer_DiffuseAlbedo                      ,
    NVSDK_NGX_Parameter_GBuffer_SpecularAlbedo                     ,
    NVSDK_NGX_Parameter_GBuffer_IndirectAlbedo                     ,
    NVSDK_NGX_Parameter_GBuffer_SpecularMvec                       ,
    NVSDK_NGX_Parameter_GBuffer_DisocclusionMask                   ,
    NVSDK_NGX_Parameter_GBuffer_Metallic                           ,
    NVSDK_NGX_Parameter_GBuffer_Specular                           ,
    NVSDK_NGX_Parameter_GBuffer_Subsurface                         ,
    NVSDK_NGX_Parameter_GBuffer_ShadingModelId                     ,
    NVSDK_NGX_Parameter_GBuffer_MaterialId                         ,
    NVSDK_NGX_Parameter_GBuffer_Atrrib_8                           ,
    NVSDK_NGX_Parameter_GBuffer_Atrrib_9                           ,
    NVSDK_NGX_Parameter_GBuffer_Atrrib_10                          ,
    NVSDK_NGX_Parameter_GBuffer_Atrrib_11                          ,
    NVSDK_NGX_Parameter_GBuffer_Atrrib_12                          ,
    NVSDK_NGX_Parameter_GBuffer_Atrrib_13                          ,
    NVSDK_NGX_Parameter_GBuffer_Atrrib_14                          ,
    NVSDK_NGX_Parameter_GBuffer_Atrrib_15                          ,
    NVSDK_NGX_Parameter_TonemapperType                             ,
    NVSDK_NGX_Parameter_FreeMemOnReleaseFeature                    ,
    NVSDK_NGX_Parameter_MotionVectors3D                            ,
    NVSDK_NGX_Parameter_IsParticleMask                             ,
    NVSDK_NGX_Parameter_AnimatedTextureMask                        ,
    NVSDK_NGX_Parameter_DepthHighRes                               ,
    NVSDK_NGX_Parameter_Position_ViewSpace                         ,
    NVSDK_NGX_Parameter_FrameTimeDeltaInMsec                       ,
    NVSDK_NGX_Parameter_RayTracingHitDistance                      ,
    NVSDK_NGX_Parameter_MotionVectorsReflection                    ,
    NVSDK_NGX_Parameter_DLSS_Enable_Output_Subrects                ,
    NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_X            ,
    NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_Y            ,
    NVSDK_NGX_Parameter_DLSS_Input_Depth_Subrect_Base_X            ,
    NVSDK_NGX_Parameter_DLSS_Input_Depth_Subrect_Base_Y            ,
    NVSDK_NGX_Parameter_DLSS_Input_MV_SubrectBase_X                ,
    NVSDK_NGX_Parameter_DLSS_Input_MV_SubrectBase_Y                ,
    NVSDK_NGX_Parameter_DLSS_Input_Translucency_SubrectBase_X      ,
    NVSDK_NGX_Parameter_DLSS_Input_Translucency_SubrectBase_Y      ,
    NVSDK_NGX_Parameter_DLSS_Output_Subrect_Base_X                 ,
    NVSDK_NGX_Parameter_DLSS_Output_Subrect_Base_Y                 ,
    NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Width       ,
    NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Height      ,
    NVSDK_NGX_Parameter_DLSS_Pre_Exposure                          ,
    NVSDK_NGX_Parameter_DLSS_Exposure_Scale                        ,
    NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_Mask         ,
    NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_SubrectBase_X,
    NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_SubrectBase_Y,
    NVSDK_NGX_Parameter_DLSS_Indicator_Invert_Y_Axis               ,
    NVSDK_NGX_Parameter_DLSS_Indicator_Invert_X_Axis               ,
    NVSDK_NGX_Parameter_DLSS_INV_VIEW_PROJECTION_MATRIX            ,
    NVSDK_NGX_Parameter_DLSS_CLIP_TO_PREV_CLIP_MATRIX              ,

    NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Width          ,
    NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Height         ,
    NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Width          ,
    NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Height         ,

    NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA               ,
    NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality            ,
    NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced           ,
    NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance        ,
    NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance   ,
    NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraQuality
  };

  for ( auto& param : param_list )
  {
    unsigned int ui_value;

    if ( NVSDK_NGX_Result_Success ==
           NVSDK_NGX_Parameter_GetUI_Original (Params, param, &ui_value) )
    {
      SK_LOGi0 (L"%hs -> %u", param, ui_value);
    }
  }

  for ( auto& param : param_list )
  {
    int i_value;

    if ( NVSDK_NGX_Result_Success ==
           Params->Get (param, &i_value) )
           //NVSDK_NGX_Parameter_GetI_Original (Params, param, &i_value) )
    {
      SK_LOGi0 (L"%hs -> %i", param, i_value);
    }
  }

  for ( auto& param : param_list )
  {
    void* p_value;

    if ( NVSDK_NGX_Result_Success ==
           Params->Get (param, &p_value) )
    {
      SK_LOGi0 (L"%hs -> %p", param, p_value);
    }
  }

  for ( auto& param : param_list )
  {
    float f_value;

    if ( NVSDK_NGX_Result_Success ==
           Params->Get (param, &f_value) )
    {
      SK_LOGi0 (L"%hs -> %f", param, f_value);
    }
  }

  for ( auto& param : param_list )
  {
    float d_value;

    if ( NVSDK_NGX_Result_Success ==
           Params->Get (param, &d_value) )
    {
      SK_LOGi0 (L"%hs -> %f", param, d_value);
    }
  }

  for ( auto& param : param_list )
  {
    unsigned long long ull_value;

    if ( NVSDK_NGX_Result_Success ==
           Params->Get (param, &ull_value) )
    {
      SK_LOGi0 (L"%hs -> %llu", param, ull_value);
    }
  }
}


const char*
SK_NGX_FeatureToStr (NVSDK_NGX_Feature feature) noexcept
{
  switch (feature)
  {
    case NVSDK_NGX_Feature_Reserved0:             return "Reserved0";
    case NVSDK_NGX_Feature_SuperSampling:         return "DLSS";
    case NVSDK_NGX_Feature_InPainting:            return "InPainting";
    case NVSDK_NGX_Feature_ImageSuperResolution:  return "Image Super Resolution";
    case NVSDK_NGX_Feature_SlowMotion:            return "Slow Motion";
    case NVSDK_NGX_Feature_VideoSuperResolution:  return "Video Super Resolution";
    case NVSDK_NGX_Feature_Reserved1:             return "Reserved1";
    case NVSDK_NGX_Feature_Reserved2:             return "Reserved2";
    case NVSDK_NGX_Feature_Reserved3:             return "Reserved3";
    case NVSDK_NGX_Feature_ImageSignalProcessing: return "Image Signal Processing (DLISP)";
    case NVSDK_NGX_Feature_DeepResolve:           return "Deep Resolve";
    case NVSDK_NGX_Feature_FrameGeneration:       return "DLSS Frame Generation";
    case NVSDK_NGX_Feature_DeepDVC:               return "Deep VC";
    case NVSDK_NGX_Feature_RayReconstruction:     return "DLSS Ray Reconstruction";
    default:                                      return "Unknown Feature";
  }
}

void
NVSDK_CONV
SK_NGX_LogCallback ( const char*             message,
                     NVSDK_NGX_Logging_Level loggingLevel,
                     NVSDK_NGX_Feature       sourceComponent )
{
  std::ignore = loggingLevel;

  game_debug->Log (
    LR"([%ws] SK_NGX_LogCallback (%hs) => "%hs")", __SK_SUBSYSTEM__,
              SK_NGX_FeatureToStr (sourceComponent),
          message );
}

void
SK_NGX12_DumpBuffers_DLSSG (ID3D12GraphicsCommandList* pCommandList)
{
  std::ignore = pCommandList;
  //void SK_D3D12_CopyTexRegion_Dump (ID3D12GraphicsCommandList* This, ID3D12Resource* pResource, const wchar_t *wszName);

  ////if (SK_NGX_DLSSG_UI_Buffer      != nullptr) SK_D3D12_CopyTexRegion_Dump (pCommandList, (ID3D12Resource *)SK_NGX_DLSSG_UI_Buffer,      L"DLSSG.UI");
  ////if (SK_NGX_DLSSG_HUDLess_Buffer != nullptr) SK_D3D12_CopyTexRegion_Dump (pCommandList, (ID3D12Resource *)SK_NGX_DLSSG_HUDLess_Buffer, L"DLSSG.HUDLess");
  ////if (SK_NGX_DLSSG_Back_Buffer    != nullptr) SK_D3D12_CopyTexRegion_Dump (pCommandList, (ID3D12Resource *)SK_NGX_DLSSG_Back_Buffer,    L"DLSSG.Backbuffer");
  ////if (SK_NGX_DLSSG_MVecs_Buffer   != nullptr) SK_D3D12_CopyTexRegion_Dump (pCommandList, (ID3D12Resource *)SK_NGX_DLSSG_MVecs_Buffer,   L"DLSSG.MVecs");
  ////if (SK_NGX_DLSSG_Depth_Buffer   != nullptr) SK_D3D12_CopyTexRegion_Dump (pCommandList, (ID3D12Resource *)SK_NGX_DLSSG_Depth_Buffer,   L"DLSSG.Depth");

  //SK_NGX_DLSSG_UI_Buffer      = nullptr;
  //SK_NGX_DLSSG_HUDLess_Buffer = nullptr;
  //SK_NGX_DLSSG_Back_Buffer    = nullptr;
  //SK_NGX_DLSSG_MVecs_Buffer   = nullptr;
  //SK_NGX_DLSSG_Depth_Buffer   = nullptr;
}