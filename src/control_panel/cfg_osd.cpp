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

#include <SpecialK/control_panel/osd.h>

using namespace SK::ControlPanel;

bool
SK::ControlPanel::OSD::DrawVideoCaptureOptions (void)
{
  const bool bRet =
    ImGui::Checkbox ("Show OSD in Video Capture", &config.render.osd.draw_in_vidcap);

  if (bRet)
  {
    struct toggle_state_s {
               bool    *dest         = nullptr;
               ULONG64  frames_drawn = 0;
      volatile LONG     testing      = FALSE;
               DWORD    initial_time = 0;
               DWORD    time_to_wait = 0;
               bool     original     = false,
                        new_val      = false;
    } static osd_toggle              = {   };

    if (FALSE == InterlockedCompareExchange (&osd_toggle.testing, TRUE, FALSE))
    {
      osd_toggle.frames_drawn = SK_GetFramesDrawn ();
      osd_toggle.initial_time = SK::ControlPanel::current_time;
      osd_toggle.time_to_wait = 125UL;
      osd_toggle.original     = (! config.render.osd.draw_in_vidcap);
      osd_toggle.new_val      =    config.render.osd.draw_in_vidcap;
      osd_toggle.dest         =   &config.render.osd.draw_in_vidcap;

      // Automated recovery in case changing this setting blows stuff up.
      //
      SK_Thread_Create ( [](LPVOID toggle) -> DWORD
      {
        SetCurrentThreadDescription (                 L"[SK] Video Capture OSD Failsafe" );
        SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_TIME_CRITICAL );
        SetThreadPriorityBoost      ( SK_GetCurrentThread (), TRUE                          );

        toggle_state_s* pToggle = (toggle_state_s *)toggle;

        SK_Sleep (pToggle->time_to_wait);

        if (SK_GetFramesDrawn () <= pToggle->frames_drawn + 1)
        {
          *pToggle->dest = pToggle->original;

          SK_ImGui_Warning (L"Disabling OSD in video capture is not supported in this game.");
        }

        InterlockedExchange (&pToggle->testing, FALSE);

        SK_Thread_CloseSelf (   );
        _endthreadex        (0x0);

        return 0;
      }, (LPVOID)&osd_toggle );
    }
  }

  if (ImGui::IsItemHovered ())
  {
    ImGui::BeginTooltip    ();
    ImGui::TextUnformatted ("Alters visibility in most video capture software");
    ImGui::Separator       ();
    ImGui::BulletText      ("Enabled by default for maximum compatibility");
    ImGui::BulletText      ("Enabling this has a high likelihood of interfering with ReShade");
    ImGui::EndTooltip      ();
  }

  return bRet;
}

bool
SK::ControlPanel::OSD::Draw (void)
{
  static const ImGuiIO& io =
    ImGui::GetIO ();

  bool bOSDIsOpen =
    ImGui::CollapsingHeader ("On Screen Display (OSD)");

  if (bOSDIsOpen)
  {
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));
    ImGui::TreePush       ("");

    ImGui::Checkbox ("Display OSD", &config.osd.show);

    bool show_banner =
      config.version_banner.duration > 0.0f;

    ImGui::SameLine ();
    ImGui::Checkbox ("Show Startup Banner", &show_banner);

    if (show_banner && config.version_banner.duration <= 0.0f)
                       config.version_banner.duration = 15.0f;

    if (show_banner)
    {
      ImGui::SameLine ();
      ImGui::SliderFloat ( "##Show Startup Banner",
                         &config.version_banner.duration,
                                                    0.0f, 30.0f,
                           "%3.1f Seconds" );
    }
    else
      config.version_banner.duration = 0.0f;

    if (config.osd.show && ImGui::CollapsingHeader ("Basic Monitoring", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush ("");

      ImGui::BeginGroup ();
      ImGui::Checkbox   ("Title ",       &config.title.show);
      ImGui::SameLine   ();
      ImGui::Checkbox   ("Clock ",       &config.time.show);
      ImGui::SameLine   ();
      ImGui::Checkbox   ("Framerate",    &config.fps.show);
      ImGui::EndGroup   ();
      
      ImGui::BeginGroup ();
      if (config.fps.show)
      {
        int idx = config.fps.compact ? 0 :
                  config.fps.show + config.fps.frametime + config.fps.advanced;

        ImGui::SameLine ();
        ImGui::Combo    ("Details", &idx, "Compact\0Simple FPS\0FPS + Frame Time (ms)\0Advanced Frame Pacing Analysis\0\0", 4);

             if (idx == 3) { config.fps.show = config.fps.frametime = config.fps.advanced       = true;  config.fps.compact = false; }
        else if (idx == 2) { config.fps.show = config.fps.frametime = true; config.fps.advanced = false; config.fps.compact = false; }
        else if (idx == 1) { config.fps.show = true; config.fps.frametime = config.fps.advanced = false; config.fps.compact = false; }
        else if (idx == 0) { config.fps.show = true; config.fps.frametime = config.fps.advanced = false; config.fps.compact =  true; }
      }

      ImGui::SameLine   ();
      ImGui::Checkbox   ("Frame Number", &config.fps.framenumber);
      ImGui::EndGroup   ();

      // New line

      ImGui::Checkbox   ("GPU Stats",    &config.gpu.show);
      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      if (config.gpu.show)
      {
        ImGui::TreePush ("");

        int temp_unit = config.system.prefer_fahrenheit ? 1 : 0;

        ImGui::RadioButton (" Celsius ",    &temp_unit, 0); ImGui::SameLine ();
        ImGui::RadioButton (" Fahrenheit ", &temp_unit, 1); ImGui::SameLine ();
        ImGui::Checkbox    (" Print Slowdown", &config.gpu.print_slowdown);

        config.system.prefer_fahrenheit = temp_unit == 1 ? true : false;

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("On NVIDIA GPUs, this will print driver throttling details (e.g. power saving)");

        ImGui::TreePop  ();
      }
      ImGui::EndGroup   ();

      ImGui::TreePop    ();
    }

    if (config.osd.show && ImGui::CollapsingHeader ("Extended Monitoring"))
    {
      ImGui::TreePush     ("");

      ImGui::PushStyleVar (ImGuiStyleVar_WindowRounding, 16.0f);
      ImGui::BeginChild   ("WMI Monitors", ImVec2 (font.size * 50.0f,font.size_multiline * 6.05f), true, ImGuiWindowFlags_NavFlattened);

      ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (1.0f, 0.785f, 0.0784f, 1.0f));
      ImGui::TextWrapped    ("These functions spawn a WMI monitoring service and may take several seconds to start.");
      ImGui::PopStyleColor  ();

      bool spawn = false;

      ImGui::Separator ( );
      ImGui::BeginColumns ("ExtendedMonitoringColumns", 3, ImGuiOldColumnFlags_NoResize);

      spawn |= ImGui::Checkbox ("CPU Stats",         &config.cpu.show);

      if (! config.cpu.show)
        SKIF_ImGui_PushDisableState ( );

      ImGui::TreePush ("");
      ImGui::Checkbox (" Simplified View",           &config.cpu.simple);
      ImGui::TreePop  (  );

      if (! config.cpu.show)
        SKIF_ImGui_PopDisableState ( );
      
      spawn |= ImGui::Checkbox ("General I/O Stats", &config.io.show);

      ImGui::NextColumn (  );

      spawn |= ImGui::Checkbox ("Memory Stats",      &config.mem.show);
      spawn |= ImGui::Checkbox ("Disk Stats",        &config.disk.show);
      
      if (! config.disk.show)
        SKIF_ImGui_PushDisableState ( );

      ImGui::TreePush ("");
      bool hovered;

      ImGui::RadioButton (" Logical",                &config.disk.type, 1);
      hovered = ImGui::IsItemHovered ();
      ImGui::SameLine    ( );
      ImGui::RadioButton (" Physical",               &config.disk.type, 0);
      hovered |= ImGui::IsItemHovered ();
      ImGui::TreePop  ();

      if (config.disk.show && hovered)
        ImGui::SetTooltip ("Requires Application Restart");

      if (! config.disk.show)
        SKIF_ImGui_PopDisableState ( );

      ImGui::NextColumn (  );

      spawn |= ImGui::Checkbox ("Pagefile Stats",    &config.pagefile.show);

      ImGui::NewLine (); ImGui::NewLine ();

      if (spawn)
        SK_StartPerfMonThreads ();

      ImGui::EndColumns  ();
      ImGui::Separator   ();

      ImGui::Columns     (3, "", false);
      ImGui::NextColumn  ();
      ImGui::NextColumn  ();
      ImGui::Checkbox    ("Remember These Settings", &config.osd.remember_state);
      ImGui::EndColumns  ();

      ImGui::EndChild    ();
      ImGui::PopStyleVar ();

      ImGui::TreePop     ();
    }

    if (config.osd.show && ImGui::CollapsingHeader ("Appearance", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush ("");

      float r = static_cast <float> (config.osd.red)   / 255.0F,
            g = static_cast <float> (config.osd.green) / 255.0F,
            b = static_cast <float> (config.osd.blue)  / 255.0F;

      float color [3] = { r, g, b };
      float       default_r,
                     default_g,
                        default_b;

      SK_OSD_GetDefaultColor (default_r, default_g, default_b);

      if (config.osd.red   == (int)MAXDWORD) color [0] = default_r;
      if (config.osd.green == (int)MAXDWORD) color [1] = default_g;
      if (config.osd.blue  == (int)MAXDWORD) color [2] = default_b;

      if (ImGui::ColorEdit3 ("OSD Color", color))
      {
        color [0] = std::max (std::min (color [0], 1.0F), 0.0f);
        color [1] = std::max (std::min (color [1], 1.0F), 0.0f);
        color [2] = std::max (std::min (color [2], 1.0F), 0.0f);

        config.osd.red   = static_cast <int>(color [0] * 255.0F);
        config.osd.green = static_cast <int>(color [1] * 255.0F);
        config.osd.blue  = static_cast <int>(color [2] * 255.0F);

        if ( color [0] >= default_r - 0.001F &&
             color [0] <= default_r + 0.001F    ) config.osd.red   = (int)MAXDWORD;
        if ( color [1] >= default_g - 0.001F &&
             color [1] <= default_g + 0.001F    ) config.osd.green = (int)MAXDWORD;
        if ( color [2] >= default_b - 0.001F &&
             color [2] <= default_b + 0.001F    ) config.osd.blue  = (int)MAXDWORD;
      }

      if (ImGui::SliderFloat ("OSD Scale", &config.osd.scale, 0.5F, 10.0F))
      {
        SK_SetOSDScale (config.osd.scale, false, nullptr);
      }

      int x_pos = std::abs (config.osd.pos_x);
      int y_pos = std::abs (config.osd.pos_y);

      bool right_align  = config.osd.pos_x < 0;
      bool bottom_align = config.osd.pos_y < 0;

      bool
      moved  = ImGui::SliderInt ("X Origin##OSD",       &x_pos, 1, static_cast <int> (io.DisplaySize.x)); ImGui::SameLine ();
      moved |= ImGui::Checkbox  ("Right-aligned##OSD",  &right_align);
      moved |= ImGui::SliderInt ("Y Origin##OSD",       &y_pos, 1, static_cast <int> (io.DisplaySize.y)); ImGui::SameLine ();
      moved |= ImGui::Checkbox  ("Bottom-aligned##OSD", &bottom_align);

      if (moved)
      {
        config.osd.pos_x = x_pos * (right_align  ? -1 : 1);
        config.osd.pos_y = y_pos * (bottom_align ? -1 : 1);

        if (right_align  && config.osd.pos_x >= 0)
          config.osd.pos_x = -1;

        if (bottom_align && config.osd.pos_y >= 0)
          config.osd.pos_y = -1;

        SK_SetOSDPos (config.osd.pos_x, config.osd.pos_y, nullptr);
      }

      ImGui::TreePop ();
    }

    ImGui::TreePop       ( );
    ImGui::PopStyleColor (3);

    return true;
  }

  return false;
}













#if 0
#define __NvAPI_GetPhysicalGPUFromDisplay                 0x1890E8DA
#define __NvAPI_GetPhysicalGPUFromGPUID                   0x5380AD1A
#define __NvAPI_GetGPUIDfromPhysicalGPU                   0x6533EA3E

#define __NvAPI_GetInfoFrameStatePvt                      0x7FC17574
#define __NvAPI_GPU_GetMemoryInfo                         0x07F9B368

#define __NvAPI_LoadMicrocode                             0x3119F36E
#define __NvAPI_GetLoadedMicrocodePrograms                0x919B3136
#define __NvAPI_GetDisplayDriverBuildTitle                0x7562E947
#define __NvAPI_GetDisplayDriverCompileType               0x988AEA78
#define __NvAPI_GetDisplayDriverSecurityLevel             0x9D772BBA
#define __NvAPI_AccessDisplayDriverRegistry               0xF5579360
#define __NvAPI_GetDisplayDriverRegistryPath              0x0E24CEEE
#define __NvAPI_GetUnAttachedDisplayDriverRegistryPath    0x633252D8
#define __NvAPI_GPU_GetRawFuseData                        0xE0B1DCE9
#define __NvAPI_GPU_GetFoundry                            0x5D857A00
#define __NvAPI_GPU_GetVPECount                           0xD8CBF37B

#define __NvAPI_GPU_GetTargetID                           0x35B5FD2F

#define __NvAPI_GPU_GetShortName                          0xD988F0F3

#define __NvAPI_GPU_GetVbiosMxmVersion                    0xE1D5DABA
#define __NvAPI_GPU_GetVbiosImage                         0xFC13EE11
#define __NvAPI_GPU_GetMXMBlock                           0xB7AB19B9

#define __NvAPI_GPU_SetCurrentPCIEWidth                   0x3F28E1B9
#define __NvAPI_GPU_SetCurrentPCIESpeed                   0x3BD32008
#define __NvAPI_GPU_GetPCIEInfo                           0xE3795199
#define __NvAPI_GPU_ClearPCIELinkErrorInfo                0x8456FF3D
#define __NvAPI_GPU_ClearPCIELinkAERInfo                  0x521566BB
#define __NvAPI_GPU_GetFrameBufferCalibrationLockFailures 0x524B9773
#define __NvAPI_GPU_SetDisplayUnderflowMode               0x387B2E41
#define __NvAPI_GPU_GetDisplayUnderflowStatus             0xED9E8057

#define __NvAPI_GPU_GetBarInfo                            0xE4B701E3

#define __NvAPI_GPU_GetPSFloorSweepStatus                 0xDEE047AB
#define __NvAPI_GPU_GetVSFloorSweepStatus                 0xD4F3944C
#define __NvAPI_GPU_GetSerialNumber                       0x14B83A5F
#define __NvAPI_GPU_GetManufacturingInfo                  0xA4218928

#define __NvAPI_GPU_GetRamConfigStrap                     0x51CCDB2A
#define __NvAPI_GPU_GetRamBusWidth                        0x7975C581

#define __NvAPI_GPU_GetRamBankCount                       0x17073A3C
#define __NvAPI_GPU_GetArchInfo                           0xD8265D24
#define __NvAPI_GPU_GetExtendedMinorRevision              0x25F17421
#define __NvAPI_GPU_GetSampleType                         0x32E1D697
#define __NvAPI_GPU_GetHardwareQualType                   0xF91E777B
#define __NvAPI_GPU_GetAllClocks                          0x1BD69F49
#define __NvAPI_GPU_SetClocks                             0x6F151055
#define __NvAPI_GPU_SetPerfHybridMode                     0x7BC207F8
#define __NvAPI_GPU_GetPerfHybridMode                     0x5D7CCAEB
#define __NvAPI_GPU_GetHybridControllerInfo               0xD26B8A58
#define __NvAPI_GetHybridMode                             0x0E23B68C1

#define __NvAPI_RestartDisplayDriver                      0xB4B26B65
#define __NvAPI_GPU_GetAllGpusOnSameBoard                 0x4DB019E6

#define __NvAPI_SetTopologyDisplayGPU                     0xF409D5E5
#define __NvAPI_GetTopologyDisplayGPU                     0x813D89A8
#define __NvAPI_SYS_GetSliApprovalCookie                  0xB539A26E

#define __NvAPI_CreateUnAttachedDisplayFromDisplay        0xA0C72EE4
#define __NvAPI_GetDriverModel                            0x25EEB2C4
#define __NvAPI_GPU_CudaEnumComputeCapableGpus            0x5786CC6E
#define __NvAPI_GPU_PhysxSetState                         0x4071B85E
#define __NvAPI_GPU_PhysxQueryRecommendedState            0x7A4174F4
#define __NvAPI_GPU_GetDeepIdleState                      0x1AAD16B4
#define __NvAPI_GPU_SetDeepIdleState                      0x568A2292

#define __NvAPI_GetScalingCaps                            0x8E875CF9
#define __NvAPI_GPU_GetThermalTable                       0xC729203C
#define __NvAPI_GPU_GetHybridControllerInfo               0xD26B8A58
#define __NvAPI_SYS_SetPostOutput                         0xD3A092B1

std::wstring
ErrorMessage ( _NvAPI_Status err,
               const char*   args,
               UINT          line_no,
               const char*   function_name,
               const char*   file_name )
{
  char szError [256];

  NvAPI_GetErrorMessage (err, szError);

  wchar_t wszFormattedError [1024] = { };

  swprintf ( wszFormattedError, 1024,
              L"Line %u of %hs (in %hs (...)):\n"
              L"------------------------\n\n"
              L"NvAPI_%hs\n\n\t>> %hs <<",
               line_no,
                file_name,
                 function_name,
                  args,
                   szError );

  return wszFormattedError;
}


std::string
SK_NvAPI_GetGPUInfoStr (void)
{
  return "";

  static char adapters [4096] = { };

  if (*adapters != L'\0')
    return adapters;

  if (sk::NVAPI::CountPhysicalGPUs ())
  {
    typedef NvU32 NvGPUID;

    typedef void*        (__cdecl *NvAPI_QueryInterface_pfn)                                   (unsigned int offset);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetRamType_pfn)            (NvPhysicalGpuHandle handle, NvU32* memtype);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetShaderPipeCount_pfn)    (NvPhysicalGpuHandle handle, NvU32* count);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetShaderSubPipeCount_pfn) (NvPhysicalGpuHandle handle, NvU32* count);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetFBWidthAndLocation_pfn) (NvPhysicalGpuHandle handle, NvU32* width, NvU32* loc);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetPartitionCount_pfn)     (NvPhysicalGpuHandle handle, NvU32* count);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetTotalSMCount_pfn)       (NvPhysicalGpuHandle handle, NvU32* count);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetTotalSPCount_pfn)       (NvPhysicalGpuHandle handle, NvU32* count);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetTotalTPCCount_pfn)      (NvPhysicalGpuHandle handle, NvU32* count);

    typedef NvAPI_Status (__cdecl *NvAPI_RestartDisplayDriver_pfn)      (void);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetSerialNumber_pfn)       (NvPhysicalGpuHandle handle,   NvU32* num);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetManufacturingInfo_pfn)  (NvPhysicalGpuHandle handle,    void* data);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetFoundry_pfn)            (NvPhysicalGpuHandle handle,    void* data);
    typedef NvAPI_Status (__cdecl *NvAPI_GetDriverModel_pfn)            (NvPhysicalGpuHandle handle,   NvU32* data);
    typedef NvAPI_Status (__cdecl *NvAPI_GetGPUIDFromPhysicalGPU_pfn)   (NvPhysicalGpuHandle handle, NvGPUID* gpuid);

    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetShortName_pfn)          (NvPhysicalGpuHandle handle, NvAPI_ShortString str);
    typedef NvAPI_Status (__cdecl *NvAPI_GetHybridMode_pfn)             (NvPhysicalGpuHandle handle, NvU32*            mode);

#ifdef _WIN64
    HMODULE hLib = SK_LoadLibrary (L"nvapi64.dll");
#else
    HMODULE hLib = SK_LoadLibrary (L"nvapi.dll");
#endif
    static NvAPI_QueryInterface_pfn            NvAPI_QueryInterface            = (NvAPI_QueryInterface_pfn)GetProcAddress (hLib, "nvapi_QueryInterface");

    static NvAPI_GPU_GetRamType_pfn            NvAPI_GPU_GetRamType            = (NvAPI_GPU_GetRamType_pfn)NvAPI_QueryInterface            (0x57F7CAAC);
    static NvAPI_GPU_GetShaderPipeCount_pfn    NvAPI_GPU_GetShaderPipeCount    = (NvAPI_GPU_GetShaderPipeCount_pfn)NvAPI_QueryInterface    (0x63E2F56F);
    static NvAPI_GPU_GetShaderSubPipeCount_pfn NvAPI_GPU_GetShaderSubPipeCount = (NvAPI_GPU_GetShaderSubPipeCount_pfn)NvAPI_QueryInterface (0x0BE17923);
    static NvAPI_GPU_GetFBWidthAndLocation_pfn NvAPI_GPU_GetFBWidthAndLocation = (NvAPI_GPU_GetFBWidthAndLocation_pfn)NvAPI_QueryInterface (0x11104158);
    static NvAPI_GPU_GetPartitionCount_pfn     NvAPI_GPU_GetPartitionCount     = (NvAPI_GPU_GetPartitionCount_pfn)NvAPI_QueryInterface     (0x86F05D7A);
    static NvAPI_RestartDisplayDriver_pfn      NvAPI_RestartDisplayDriver      = (NvAPI_RestartDisplayDriver_pfn)NvAPI_QueryInterface      (__NvAPI_RestartDisplayDriver);
    static NvAPI_GPU_GetSerialNumber_pfn       NvAPI_GPU_GetSerialNumber       = (NvAPI_GPU_GetSerialNumber_pfn)NvAPI_QueryInterface       (__NvAPI_GPU_GetSerialNumber);
    static NvAPI_GPU_GetManufacturingInfo_pfn  NvAPI_GPU_GetManufacturingInfo  = (NvAPI_GPU_GetManufacturingInfo_pfn)NvAPI_QueryInterface  (__NvAPI_GPU_GetManufacturingInfo);
    static NvAPI_GPU_GetFoundry_pfn            NvAPI_GPU_GetFoundry            = (NvAPI_GPU_GetFoundry_pfn)NvAPI_QueryInterface            (__NvAPI_GPU_GetFoundry);
    static NvAPI_GetDriverModel_pfn            NvAPI_GetDriverModel            = (NvAPI_GetDriverModel_pfn)NvAPI_QueryInterface            (__NvAPI_GetDriverModel);
    static NvAPI_GPU_GetShortName_pfn          NvAPI_GPU_GetShortName          = (NvAPI_GPU_GetShortName_pfn)NvAPI_QueryInterface          (__NvAPI_GPU_GetShortName);

    static NvAPI_GPU_GetTotalSMCount_pfn       NvAPI_GPU_GetTotalSMCount       = (NvAPI_GPU_GetTotalSMCount_pfn)NvAPI_QueryInterface       (0x0AE5FBCFE);// 0x329D77CD);// 0x0AE5FBCFE);
    static NvAPI_GPU_GetTotalSPCount_pfn       NvAPI_GPU_GetTotalSPCount       = (NvAPI_GPU_GetTotalSPCount_pfn)NvAPI_QueryInterface       (0xE4B701E3);// 0xE0B1DCE9);// 0x0B6D62591);
    static NvAPI_GPU_GetTotalTPCCount_pfn      NvAPI_GPU_GetTotalTPCCount      = (NvAPI_GPU_GetTotalTPCCount_pfn)NvAPI_QueryInterface      (0x4E2F76A8);
    //static NvAPI_GPU_GetTotalTPCCount_pfn NvAPI_GPU_GetTotalTPCCount = (NvAPI_GPU_GetTotalTPCCount_pfn)NvAPI_QueryInterface (0xD8265D24);// 0x4E2F76A8);// __NvAPI_GPU_Get
    static NvAPI_GetGPUIDFromPhysicalGPU_pfn   NvAPI_GetGPUIDFromPhysicalGPU   = (NvAPI_GetGPUIDFromPhysicalGPU_pfn)NvAPI_QueryInterface   (__NvAPI_GetGPUIDfromPhysicalGPU);
    static NvAPI_GetHybridMode_pfn             NvAPI_GetHybridMode             = (NvAPI_GetHybridMode_pfn)NvAPI_QueryInterface             (__NvAPI_GetHybridMode);
  }

  return adapters;
}
#endif
