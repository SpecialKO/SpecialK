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

#include <SpecialK/control_panel/compatibility.h>

using namespace SK::ControlPanel;

bool
SK::ControlPanel::Compatibility::Draw (void)
{
  if ( ImGui::CollapsingHeader ("Compatibility Settings###SK_CPL") )
  {
    static SK_RenderBackend& rb =
      SK_GetCurrentRenderBackend ();

    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.68f, 0.90f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.72f, 0.90f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.78f, 0.87f, 0.80f));
    ImGui::TreePush       ("");

    if (ImGui::CollapsingHeader ("Third-Party Software"))
    {
      ImGui::TreePush ("");
      ImGui::Checkbox     ("Disable GeForce Experience and NVIDIA Shield", &config.compatibility.disable_nv_bloat);
      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("May improve software compatibility, but disables ShadowPlay, couch co-op and various Shield-related functionality.");
      ImGui::TreePop  ();
    }

    if ( ImGui::CollapsingHeader ("Render Backends",
           SK_IsInjected () ? ImGuiTreeNodeFlags_DefaultOpen :
                              0 ) )
    {
      ImGui::TreePush ("");

      auto EnableActiveAPI =
      [ ](SK_RenderAPI api)
      {
        switch (api)
        {
          case SK_RenderAPI::D3D9Ex:
            config.apis.d3d9ex.hook     = true; // Fallthrough, D3D9Ex
          case SK_RenderAPI::D3D9:              //      implies D3D9!
            config.apis.d3d9.hook       = true;
            break;

#ifdef _M_AMD64
          case SK_RenderAPI::D3D12:
            config.apis.dxgi.d3d12.hook = true;
            break;
#endif
          case SK_RenderAPI::D3D11:
            config.apis.dxgi.d3d11.hook = true;
            break;

#ifdef _M_IX86
          case SK_RenderAPI::DDrawOn11:
            config.apis.ddraw.hook       = true;
            config.apis.dxgi.d3d11.hook  = true;
            break;

          case SK_RenderAPI::D3D8On11:
            config.apis.d3d8.hook        = true;
            config.apis.dxgi.d3d11.hook  = true;
            break;
#endif

          case SK_RenderAPI::OpenGL:
            config.apis.OpenGL.hook     = true;
            break;

#ifdef _M_AMD64
          case SK_RenderAPI::Vulkan:
            config.apis.Vulkan.hook     = true;
            break;
#endif
          default:
            break;
        }
      };

      using Tooltip_pfn = void (*)(void);

      auto ImGui_CheckboxEx =
      [ ]( const char* szName, bool* pVar,
                               bool  enabled = true,
           Tooltip_pfn tooltip_disabled      = nullptr )
      {
        if (! pVar)
          return;

        if (enabled)
        {
          ImGui::Checkbox (szName, pVar);
        }

        else
        {
          ImGui::TreePush     ("");
          ImGui::TextDisabled ("%s", szName);

          if (tooltip_disabled != nullptr)
              tooltip_disabled ();

          ImGui::TreePop      (  );

          *pVar = false;
        }
      };

#ifdef _M_AMD64
      constexpr float num_lines = 4.0f; // Basic set of APIs
#else
      constexpr float num_lines = 5.0f; // + DirectDraw / Direct3D 8
#endif

      ImGui::PushStyleVar                                                                             (ImGuiStyleVar_ChildRounding, 10.0f);
      ImGui::BeginChild ("", ImVec2 (font.size * 39.0f, font.size_multiline * num_lines * 1.1f), true, ImGuiWindowFlags_NavFlattened);

      ImGui::Columns    ( 2 );

      ImGui_CheckboxEx ("Direct3D 9",   &config.apis.d3d9.hook);
      ImGui_CheckboxEx ("Direct3D 9Ex", &config.apis.d3d9ex.hook, config.apis.d3d9.hook);

      ImGui::NextColumn (   );

      ImGui_CheckboxEx ("Direct3D 11",  &config.apis.dxgi.d3d11.hook);

#ifdef _M_AMD64
      ImGui_CheckboxEx ("Direct3D 12",  &config.apis.dxgi.d3d12.hook, config.apis.dxgi.d3d11.hook);
#endif

      ImGui::Columns    ( 1 );
      ImGui::Separator  (   );

#ifdef _M_IX86
      ImGui::Columns    ( 2 );

      static bool has_dgvoodoo2 =
        GetFileAttributesA (
          SK_FormatString ( R"(%ws\PlugIns\ThirdParty\dgVoodoo\d3dimm.dll)",
                              std::wstring ( SK_GetDocumentsDir () + L"\\My Mods\\SpecialK" ).c_str ()
                          ).c_str ()
        ) != INVALID_FILE_ATTRIBUTES;

      // Leaks memory, but who cares? :P
      static const char* dgvoodoo2_install_path =
        _strdup (
          SK_FormatString ( R"(%ws\PlugIns\ThirdParty\dgVoodoo)",
                  std::wstring ( SK_GetDocumentsDir () + L"\\My Mods\\SpecialK" ).c_str ()
              ).c_str ()
        );

      auto Tooltip_dgVoodoo2 = []
      {
        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
            ImGui::TextColored (ImColor (235, 235, 235), "Requires Third-Party Plug-In:");
            ImGui::SameLine    ();
            ImGui::TextColored (ImColor (255, 255, 0),   "dgVoodoo2");
            ImGui::Separator   ();
            ImGui::BulletText  ("Please install this to: '%s'", dgvoodoo2_install_path);
          ImGui::EndTooltip   ();
        }
      };

      ImGui_CheckboxEx ("Direct3D 8", &config.apis.d3d8.hook,   has_dgvoodoo2, Tooltip_dgVoodoo2);

      ImGui::NextColumn (  );

      ImGui_CheckboxEx ("Direct Draw", &config.apis.ddraw.hook, has_dgvoodoo2, Tooltip_dgVoodoo2);

      ImGui::Columns    ( 1 );
      ImGui::Separator  (   );
#endif

      ImGui::Columns    ( 2 );

      ImGui::Checkbox   ("OpenGL ", &config.apis.OpenGL.hook); ImGui::SameLine ();
#ifdef _M_AMD64
      ImGui::Checkbox   ("Vulkan ", &config.apis.Vulkan.hook);
#endif

      ImGui::NextColumn (  );

      if (ImGui::Button (" Disable All But the Active API "))
      {
        config.apis.d3d9ex.hook     = false; config.apis.d3d9.hook       = false;
        config.apis.dxgi.d3d11.hook = false;
        config.apis.OpenGL.hook     = false;
#ifdef _M_AMD64
        config.apis.dxgi.d3d12.hook = false; config.apis.Vulkan.hook     = false;
#else
        config.apis.d3d8.hook       = false; config.apis.ddraw.hook      = false;
#endif
      }

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Application start time and negative interactions with third-party software can be reduced by turning off APIs that are not needed...");

      ImGui::Columns    ( 1 );

      ImGui::EndChild   (  );
      ImGui::PopStyleVar ( );

      EnableActiveAPI   (render_api);
      ImGui::TreePop    ();
    }

    if (ImGui::CollapsingHeader ("Hardware Monitoring"))
    {
      ImGui::TreePush ("");
      ImGui::Checkbox ("NvAPI  ", &config.apis.NvAPI.enable);
      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("NVIDIA's hardware monitoring API, needed for the GPU stats on the OSD. Turn off only if your driver is buggy.");

      ImGui::SameLine ();
      ImGui::Checkbox ("ADL   ",   &config.apis.ADL.enable);
      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("AMD's hardware monitoring API, needed for the GPU stats on the OSD. Turn off only if your driver is buggy.");
      ImGui::TreePop  ();
    }

    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));

    if (ImGui::CollapsingHeader ("Debugging"))
    {
      ImGui::TreePush   ("");
      ImGui::BeginGroup (  );
      ImGui::Checkbox   ("Enable Crash Handler",          &config.system.handle_crashes);

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Play Metal Gear Solid Alert Sound and Log Crashes in logs/crash.log");

      ImGui::Checkbox  ("ReHook LoadLibrary",             &config.compatibility.rehook_loadlibrary);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("Keep LoadLibrary Hook at Front of Hook Chain");
        ImGui::Separator    ();
        ImGui::BulletText   ("Improves Debug Log Accuracy");
        ImGui::BulletText   ("Third-Party Software May Deadlock Game at Startup if Enabled");
        ImGui::EndTooltip   ();
      }

      ImGui::Checkbox ("Log File Reads", &config.file_io.trace_reads);

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Traces file read activity and reports it in logs/file_read.log");

      ImGui::SliderInt ("Log Level",                      &config.system.log_level, 0, 5);

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Controls Debug Log Verbosity; Higher = Bigger/Slower Logs");

      ImGui::Checkbox  ("Log Game Output",                &config.system.game_output);

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Log any Game Text Output to logs/game_output.log");

      if (ImGui::Checkbox  ("Print Debug Output to Console",  &config.system.display_debug_out))
      {
        if (config.system.display_debug_out)
        {
          if (! SK::Diagnostics::Debugger::CloseConsole ()) config.system.display_debug_out = true;
        }

        else
        {
          SK::Diagnostics::Debugger::SpawnConsole ();
        }
      }

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Spawns Debug Console at Startup for Debug Text from Third-Party Software");

      ImGui::Checkbox  ("Trace LoadLibrary",              &config.system.trace_load_library);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("Monitor DLL Load Activity");
        ImGui::Separator    ();
        ImGui::BulletText   ("Required for Render API Auto-Detection in Global Injector");
        ImGui::EndTooltip   ();
      }

      ImGui::Checkbox  ("Strict DLL Loader Compliance",   &config.system.strict_compliance);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip    (  );
        ImGui::TextUnformatted ("Prevent Loading DLLs Simultaneously Across Multiple Threads");
        ImGui::Separator       (  );
        ImGui::BulletText      ("Eliminates Race Conditions During DLL Startup");
        ImGui::BulletText      ("Unsafe for a LOT of Improperly Designed Third-Party Software\n");
        ImGui::TreePush        ("");
        ImGui::TextUnformatted ("");
        ImGui::BeginGroup      (  );
        ImGui::TextUnformatted ("PROPER DLL DESIGN:  ");
        ImGui::EndGroup        (  );
        ImGui::SameLine        (  );
        ImGui::BeginGroup      (  );
        ImGui::TextUnformatted ("Never Call LoadLibrary (...) from DllMain (...)'s Thread !!!");
        ImGui::TextUnformatted ("Never Wait on a Synchronization Object from DllMain (...) !!");
        ImGui::EndGroup        (  );
        ImGui::TreePop         (  );
        ImGui::EndTooltip      (  );
      }

      ImGui::EndGroup    ( );

      ImGui::SameLine    ( );
      ImGui::BeginGroup  ( );

      static int window_pane = 1;

      RECT window = { };
      RECT client = { };

      ImGui::TextColored (ImColor (1.f,1.f,1.f), "Window Management:  "); ImGui::SameLine ();
      ImGui::RadioButton ("Dimensions", &window_pane, 0);                 ImGui::SameLine ();
      ImGui::RadioButton ("Details",    &window_pane, 1);
      ImGui::Separator   (                             );

      auto DescribeRect = [](LPRECT rect, const char* szType, const char* szName)
      {
        ImGui::TextUnformatted (szType);
        ImGui::NextColumn ();
        ImGui::TextUnformatted (szName);
        ImGui::NextColumn ();
        ImGui::Text ( "| (%4li,%4li) / %4lix%li |  ",
                          rect->left, rect->top,
                            rect->right-rect->left, rect->bottom - rect->top );
        ImGui::NextColumn ();
      };

      SK_ImGui_AutoFont fixed_font (
        ImGui::GetIO ().Fonts->Fonts [1]
      );

      switch (window_pane)
      {
      case 0:
        ImGui::Columns   (3);

        DescribeRect (&game_window.actual.window, "Window", "Actual" );
        DescribeRect (&game_window.actual.client, "Client", "Actual" );

        ImGui::Columns   (1);
        ImGui::Separator ( );
        ImGui::Columns   (3);

        DescribeRect (&game_window.game.window,   "Window", "Game"   );
        DescribeRect (&game_window.game.client,   "Client", "Game"   );

        ImGui::Columns   (1);
        ImGui::Separator ( );
        ImGui::Columns   (3);

        GetClientRect (game_window.hWnd, &client);
        GetWindowRect (game_window.hWnd, &window);

        DescribeRect  (&window,   "Window", "GetWindowRect"   );
        DescribeRect  (&client,   "Client", "GetClientRect"   );

        ImGui::Columns   (1);
        break;

      case 1:
      {
        auto _SummarizeWindowStyle = [&](DWORD dwStyle) -> std::string
        {
          std::string summary;

          if ((dwStyle & WS_OVERLAPPED)   == WS_OVERLAPPED)   summary += "Overlapped, ";
          if ((dwStyle & WS_POPUP)        == WS_POPUP)        summary += "Popup, ";
          if ((dwStyle & WS_CHILD)        == WS_CHILD)        summary += "Child, ";
          if ((dwStyle & WS_MINIMIZE)     == WS_MINIMIZE)     summary += "Minimize, ";
          if ((dwStyle & WS_VISIBLE)      == WS_VISIBLE)      summary += "Visible, ";
          if ((dwStyle & WS_DISABLED)     == WS_DISABLED)     summary += "Disabled, ";
          if ((dwStyle & WS_CLIPSIBLINGS) == WS_CLIPSIBLINGS) summary += "Clip Siblings, ";
          if ((dwStyle & WS_CLIPCHILDREN) == WS_CLIPCHILDREN) summary += "Clip Children, ";
          if ((dwStyle & WS_MAXIMIZE)     == WS_MAXIMIZE)     summary += "Maximize, ";
          if ((dwStyle & WS_CAPTION)      == WS_CAPTION)      summary += "Caption, ";
          if ((dwStyle & WS_BORDER)       == WS_BORDER)       summary += "Border, ";
          if ((dwStyle & WS_DLGFRAME)     == WS_DLGFRAME)     summary += "Dialog Frame, ";
          if ((dwStyle & WS_VSCROLL)      == WS_VSCROLL)      summary += "Vertical Scrollbar, ";
          if ((dwStyle & WS_HSCROLL)      == WS_HSCROLL)      summary += "Horizontal Scrollbar, ";
          if ((dwStyle & WS_SYSMENU)      == WS_SYSMENU)      summary += "System Menu, ";
          if ((dwStyle & WS_THICKFRAME)   == WS_THICKFRAME)   summary += "Thick Frame, ";
          if ((dwStyle & WS_GROUP)        == WS_GROUP)        summary += "Group, ";
          if ((dwStyle & WS_TABSTOP)      == WS_TABSTOP)      summary += "Tabstop, ";
          
          if ((dwStyle & WS_OVERLAPPEDWINDOW) == WS_OVERLAPPEDWINDOW)
                                                              summary += "Overlapped Window, ";
          if ((dwStyle & WS_POPUPWINDOW)      == WS_POPUPWINDOW)
                                                              summary += "Popup Window, ";

          return summary;
        };

        ImGui::BeginGroup ();
        ImGui::Text      ( "App_Active   :" );
        ImGui::Text      ( "Active HWND  :" );
        ImGui::Text      ( "Foreground   :" );
        ImGui::Text      ( "Input Focus  :" );
        ImGui::Separator (                  );
        ImGui::Text      ( ""               );
        ImGui::Text      ( "HWND         :" );

        ImGui::Text      ( "Window Class :" );

        ImGui::Text      ( "Window Title :" );

        ImGui::Text      ( "Owner PID    :" );

        ImGui::Text      ( "Owner TID    :" );

        ImGui::Text      ( "Init. Frame  :" );

        ImGui::Text      ( "Unicode      :" );

        ImGui::Text      ( "Top          :" );

        ImGui::Text      ( "Parent       :" );
        ImGui::Text      ( "Style        :" );
        ImGui::Text      ( "ExStyle      :" );

        ImGui::EndGroup  ();
        ImGui::SameLine  ();
        ImGui::BeginGroup();
        ImGui::Text      ( "%s", SK_IsGameWindowActive  () ? "Yes" : "No" );
        ImGui::Text      ( "%p", GetActiveWindow        () );
        ImGui::Text      ( "%p", SK_GetForegroundWindow () );
        ImGui::Text      ( "%p", SK_GetFocus            () );
        ImGui::Separator (                                 );                                  
        ImGui::BeginGroup();
        ImGui::Text      ( "Focus"                                         );
        ImGui::Text      ( "%6p",          rb.windows.focus.hwnd           );
        ImGui::Text      ( "%ws",          rb.windows.focus.class_name     );
        ImGui::Text      ( "%ws",          rb.windows.focus.title          );
        ImGui::Text      ( "%8lu",         rb.windows.focus.owner.pid      );
        ImGui::Text      ( "%8lu",         rb.windows.focus.owner.tid      );
        ImGui::Text      ( "%8lu",         rb.windows.focus.last_changed   );
        ImGui::Text      ( "%8s",          rb.windows.focus.unicode ?
                                                              "Yes" : "No" );
        ImGui::Text      ( "%8p", 
                             GetTopWindow (rb.windows.focus.hwnd)          );
        ImGui::Text      ( "%8p",                                          
                             GetTopWindow (rb.windows.focus.parent)        );
        ImGui::Text      ( "%8x", SK_GetWindowLongPtrW (rb.windows.focus.hwnd, GWL_STYLE)   );

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ( _SummarizeWindowStyle (
                                  static_cast <DWORD> (
                                    SK_GetWindowLongPtrW (rb.windows.focus.hwnd, GWL_STYLE)
                                  )
                              ).c_str ());

        ImGui::Text      ( "%8x", static_cast <DWORD> (
                                    SK_GetWindowLongPtrW (rb.windows.focus.hwnd, GWL_EXSTYLE)
                         )                            );
        ImGui::EndGroup ();
        if (rb.windows.focus.hwnd != rb.windows.device.hwnd)
        {
          ImGui::SameLine  ();
          ImGui::BeginGroup();
          ImGui::Text      ( "Device"                                        );
          ImGui::Text      ( "%6p",          rb.windows.device.hwnd          );
          ImGui::Text      ( "%ws",          rb.windows.device.class_name    );
          ImGui::Text      ( "%ws",          rb.windows.device.title         );
          ImGui::Text      ( "%8lu",         rb.windows.device.owner.pid     );
          ImGui::Text      ( "%8lu",         rb.windows.device.owner.tid     );
          ImGui::Text      ( "%8lu",         rb.windows.device.last_changed  );
          ImGui::Text      ( "%8s",          rb.windows.device.unicode ?
                                                                 "Yes" : "No" );
          ImGui::Text      ( "%8p", 
                               GetTopWindow (rb.windows.device.hwnd)         );
          ImGui::Text      ( "%8p",                                          
                               GetTopWindow (rb.windows.device.parent)       );
          ImGui::Text      ( "%8x", SK_GetWindowLongPtrW (rb.windows.device.hwnd, GWL_STYLE)   );
          
          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ( _SummarizeWindowStyle (
                                    static_cast <DWORD> (
                                      SK_GetWindowLongPtrW (rb.windows.device.hwnd, GWL_STYLE)
                                    )
                                ).c_str ());

          ImGui::Text      ( "%8x", SK_GetWindowLongPtrW (rb.windows.device.hwnd, GWL_EXSTYLE) );
          ImGui::EndGroup  ();
        }
        ImGui::EndGroup ();
        }
        break;
      }

      fixed_font.Detach  ( );
      ImGui::Separator   ( );

      ImGui::Text        ( "ImGui Cursor State: %lu (%lu,%lu) { %lu, %lu }",
              (unsigned long)SK_ImGui_Cursor.visible, SK_ImGui_Cursor.pos.x,
                                                      SK_ImGui_Cursor.pos.y,
                               SK_ImGui_Cursor.orig_pos.x, SK_ImGui_Cursor.orig_pos.y );
      ImGui::SameLine    ( );
      ImGui::Text        (" {%s :: Last Update: %lu}",
                            SK_ImGui_Cursor.idle ? "Idle" :
                                                   "Not Idle",
                              SK_ImGui_Cursor.last_move);
      ImGui::EndGroup    ( );
      ImGui::TreePop     ( );
    }

    ImGui::PopStyleColor  (3);

    ImGui::TreePop        ( );
    ImGui::PopStyleColor  (3);

    return true;
  }

  return false;
}