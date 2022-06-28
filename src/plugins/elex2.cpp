//
// Copyright 2022 Andon "Kaldaien" Coleman
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

#define __SK_SUBSYSTEM__ L"Elex 2"

#include <SpecialK/log.h>
#include <imgui/font_awesome.h>

using CreateFileW_pfn =
  HANDLE (WINAPI *)(LPCWSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,
                      DWORD,DWORD,HANDLE);

static
CreateFileW_pfn
CreateFileW_Original = nullptr;

static
HANDLE
WINAPI
CreateFileW_Detour ( LPCWSTR               lpFileName,
                     DWORD                 dwDesiredAccess,
                     DWORD                 dwShareMode,
                     LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                     DWORD                 dwCreationDisposition,
                     DWORD                 dwFlagsAndAttributes,
                     HANDLE                hTemplateFile )
{
  if (StrStrIW (lpFileName, LR"(data\packed\)") || StrStrIW (lpFileName, L".pak"))
  {
    SK_LOG_FIRST_CALL

    static
      concurrency::concurrent_unordered_map <std::wstring, HANDLE>
        pinned_files;

    if (StrStrIW (lpFileName, LR"(.pak)"))
    {
      if (pinned_files.count (lpFileName) != 0)
      {
        SK_LOGi1 (L"!! Using already opened copy of %ws", lpFileName);

        return
          pinned_files [lpFileName];
      }

      HANDLE hRet =
        CreateFileW_Original (
          lpFileName, dwDesiredAccess, dwShareMode,
            lpSecurityAttributes, dwCreationDisposition,
              dwFlagsAndAttributes, hTemplateFile );


      if (hRet != INVALID_HANDLE_VALUE)
      {
        // Try as you might, you're not closing this file, you're going to get
        //   this handle back the next time you try to open and close a file for
        //     tiny 50 byte reads.
        //
        //  ** For Future Reference, OPENING files on Windows takes longer than
        //       reading them does when data is this small.
        // 
        //                  ( STOP IT!!!~ )
        //
        if (SetHandleInformation (hRet, HANDLE_FLAG_PROTECT_FROM_CLOSE,
                                        HANDLE_FLAG_PROTECT_FROM_CLOSE))
        {
          pinned_files [lpFileName] = hRet;
        }
      }

      return hRet;
    }
  }

  return
    CreateFileW_Original (
      lpFileName, dwDesiredAccess, dwShareMode,
        lpSecurityAttributes, dwCreationDisposition,
          dwFlagsAndAttributes, hTemplateFile );
}

struct {
  bool bUnfuckFileAccess = true;

  struct {
    sk::ParameterBool* unfuck_file_io = nullptr;
  } ini;
} SK_ELEX2_PlugIn;

bool
SK_ELEX2_PlugInCfg (void)
{
  if ( ImGui::CollapsingHeader ( "ELEX II",
                                   ImGuiTreeNodeFlags_DefaultOpen )
     )
  {
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));
    ImGui::TreePush       ("");

    if ( ImGui::CollapsingHeader (
           ICON_FA_TACHOMETER_ALT "\tPerformance Settings",
                          ImGuiTreeNodeFlags_DefaultOpen )
       )
    {
      static bool orig_state =
        SK_ELEX2_PlugIn.bUnfuckFileAccess;

      ImGui::TreePush    ("");
      ImGui::BeginGroup  (  );

      if ( ImGui::Checkbox ("Fix World's Worst File Code",
                            &SK_ELEX2_PlugIn.bUnfuckFileAccess ) )
      {
        SK_ELEX2_PlugIn.ini.unfuck_file_io->store (
          SK_ELEX2_PlugIn.bUnfuckFileAccess
        );
      }

      if (orig_state != SK_ELEX2_PlugIn.bUnfuckFileAccess)
      {
        ImGui::Bullet      ();
        ImGui::SameLine    ();
        ImGui::TextColored (
          ImVec4 (1.f,1.f,0.f,1.f),
            "Game Restart Required"
                            );
      }

      ImGui::EndGroup    (  );
      ImGui::TreePop     (  );
    }

    ImGui::PopStyleColor (3);
    ImGui::TreePop       ( );
  }

  return true;
}

void
SK_ELEX2_InitConfig (void)
{
  SK_ELEX2_PlugIn.ini.unfuck_file_io =
    _CreateConfigParameterBool ( L"ELEX2.PlugIn",
                                  L"UnfuckFileIO", SK_ELEX2_PlugIn.bUnfuckFileAccess,
            L"Files Do Not Like Being Opened And Closed Reepeatedly (!!)" );

  if (! SK_ELEX2_PlugIn.ini.unfuck_file_io->load  (SK_ELEX2_PlugIn.bUnfuckFileAccess))
        SK_ELEX2_PlugIn.ini.unfuck_file_io->store (true);
}

void
SK_UE_KeepFilesOpen (void)
{
  SK_CreateDLLHook2 (      L"kernel32",
                            "CreateFileW",
                             CreateFileW_Detour,
    static_cast_p2p <void> (&CreateFileW_Original) );
  
  SK_ApplyQueuedHooks ();
}

void
SK_ELEX2_InitPlugin (void)
{
  SK_ELEX2_InitConfig ();

  plugin_mgr->config_fns.emplace (SK_ELEX2_PlugInCfg);

  if (SK_ELEX2_PlugIn.bUnfuckFileAccess)
  {
    SK_CreateDLLHook2 (      L"kernel32",
                              "CreateFileW",
                               CreateFileW_Detour,
      static_cast_p2p <void> (&CreateFileW_Original) );

    SK_ApplyQueuedHooks ();
  }

  return;
}