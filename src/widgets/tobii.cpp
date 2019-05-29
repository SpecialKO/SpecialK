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
#include <SpecialK/widgets/widget.h>

#include <SpecialK/plugin/plugin_mgr.h>

#include <tobii/tobii.h>
#include <tobii/tobii_streams.h>

#ifdef _WIN64
#pragma comment (lib, "tobii_stream_engine_sk64.lib")
#else
#pragma comment (lib, "tobii_stream_engine_sk32.lib")
#endif

#define SK_TOBII_SECTION   L"Widget.Tobii"

extern iSK_INI*             osd_ini;

sk::ParameterStringW*
DeclKeybind (       SK_ConfigSerializedKeybind* binding,
                    iSK_INI*                    ini,
              const wchar_t*                    sec )
{
  auto* ret =
    dynamic_cast <sk::ParameterStringW *>
    (g_ParameterFactory->create_parameter <std::wstring> (L"DESCRIPTION HERE"));

  ret->register_to_ini ( ini, sec, binding->short_name );

  return ret;
};

bool
Keybinding (SK_Keybind* binding, sk::ParameterStringW* param)
{
  bool        ret   = false;
  std::string label =
    SK_WideCharToUTF8 (binding->human_readable) + "###";

  label += binding->bind_name;

  if (ImGui::Selectable (label.c_str (), false))
  {
    ImGui::OpenPopup (binding->bind_name);
  }

  std::wstring original_binding =
    binding->human_readable;

  SK_ImGui_KeybindDialog (binding);

  if (original_binding != binding->human_readable)
  {
    param->store (binding->human_readable);

    SK_SaveConfig ();

    ret = true;
  }

  return ret;
};


sk::ParameterBool*  _SK_Tobii_ShowGazeCursor;
sk::ParameterBool*  _SK_Tobii_WidgetGazing;
sk::ParameterInt*   _SK_Tobii_CursorType;
sk::ParameterInt*   _SK_Tobii_CursorSize;
sk::ParameterFloat* _SK_Tobii_CursorColor_R;
sk::ParameterFloat* _SK_Tobii_CursorColor_G;
sk::ParameterFloat* _SK_Tobii_CursorColor_B;
sk::ParameterFloat* _SK_Tobii_CursorColor_A;


#define GAZE_CURSOR_OPEN_CIRCLE   0
#define GAZE_CURSOR_FILLED_CIRCLE 1
#define GAZE_CURSOR_CROSSHAIR     2

#define MAX_CURSOR_SIZE          48.0f
#define CURSOR_BORDER             8.0f

struct gaze_cursor_s {
  int   type = GAZE_CURSOR_FILLED_CIRCLE;
  bool  draw = false;
  int   size =    10;
  float color
         [4] = { 1.f,  0.85f, 0.15f, 1.f };
  ImVec2 pos = { 0.0f, 0.0f };
};

static SK_LazyGlobal <gaze_cursor_s> tobii_cursor;

ImVec2&
SK_Tobii_GetGazePosition (void)
{
  static auto gaze_cursor =
    tobii_cursor.get ();

  return
    gaze_cursor.pos;
}

void
SK_Tobii_Callback_GazePoint ( tobii_gaze_point_t const* gaze_point,
                                                  void* /*user_data*/ )
{
  static auto gaze_cursor =
    tobii_cursor.get ();

  if (gaze_point->validity == TOBII_VALIDITY_VALID)
  {
    auto& io =
      ImGui::GetIO ();

    gaze_cursor.pos.x = io.DisplaySize.x * gaze_point->position_xy [0];
    gaze_cursor.pos.y = io.DisplaySize.y * gaze_point->position_xy [1];
  }

  else
  {
    gaze_cursor.pos.x = 0.0f;
    gaze_cursor.pos.y = 0.0f;
  }
}

static
void
SK_Tobii_URL_Receiver ( char const* url,
                              void* user_data )
{
  char* buffer =
    (char *)user_data;

  if (buffer != nullptr)
  {
    if (*buffer != '\0')
      return; // only keep first value

    if (strlen (url) < 256)
      strcpy (buffer, url);
  }
}



#include <SpecialK/resource.h>

void
SK_UnpackTobiiStreamEngine (void)
{
  HMODULE hModSelf =
    SK_GetDLL ();

  HRSRC res =
    FindResource ( hModSelf, MAKEINTRESOURCE (IDR_TOBII_PACKAGE), L"7ZIP" );

  if (res)
  {
    SK_LOG0 ( ( L"Unpacking tobii_stream_engine_sk.dll." ),
                L"Tobii Eyes" );

    DWORD   res_size     =
      SizeofResource ( hModSelf, res );

    HGLOBAL packed_tobii =
      LoadResource   ( hModSelf, res );

    if (! packed_tobii) return;


    const void* const locked =
      (void *)LockResource (packed_tobii);


    if (locked != nullptr)
    {
      wchar_t      wszArchive     [MAX_PATH * 2 + 1] = { };
      wchar_t      wszDestination [MAX_PATH * 2 + 1] = { };

      wcscpy      (wszDestination, SK_GetDocumentsDir ().c_str ());
      PathAppendW (wszDestination, LR"(My Mods\SpecialK\PlugIns\ThirdParty\StreamEngine\)");

      if (GetFileAttributesW (wszDestination) == INVALID_FILE_ATTRIBUTES)
        SK_CreateDirectories (wszDestination);

      wcscpy      (wszArchive, wszDestination);
      PathAppendW (wszArchive, L"tobii_stream_engine_sk.7z");

      ///SK_LOG0 ( ( L" >> Archive: %s [Destination: %s]", wszArchive,wszDestination ),
      ///            L"D3DCompile" );

      FILE* fPackedTobii =
        _wfopen   (wszArchive, L"wb");

      if (fPackedTobii != nullptr)
      {
        fwrite (locked, 1, res_size, fPackedTobii);
        fclose (fPackedTobii);
      }

      if (GetFileAttributes (wszArchive) != INVALID_FILE_ATTRIBUTES)
      {
        using SK_7Z_DECOMP_PROGRESS_PFN = int (__stdcall *)(int current, int total);

        //extern
        //HRESULT
        //SK_Decompress7zEx ( const wchar_t*            wszArchive,
        //                    const wchar_t*            wszDestination,
        //                    SK_7Z_DECOMP_PROGRESS_PFN callback );

        SK_Decompress7zEx (wszArchive, wszDestination, nullptr);
        DeleteFileW       (wszArchive);
      }
    }

    UnlockResource (packed_tobii);
  }
};


bool has_tobii       = false;
bool tobii_lost      = false;
int  tobii_reconnect =     0;
bool widget_gazing   = false;

bool SK_Tobii_IsCursorVisible (void)
{
  static auto gaze_cursor =
    tobii_cursor.get ();

  return gaze_cursor.draw;
}

bool SK_Tobii_WantWidgetGazing (void)
{
  return widget_gazing;
}

void
SK_Tobii_Startup ( tobii_api_t*&    api,
                   tobii_device_t*& device );

class SKWG_Tobii : public SK_Widget
{
public:
  tobii_api_t*    api    = nullptr;
  tobii_device_t* device = nullptr;

protected:
  SK_ConfigSerializedKeybind
    toggle_cursor = {
      SK_Keybind {
        "Toggle Gaze Cursor", L"",
          false, false, false, 255
      }, L"ToggleCursor"
    };

  SK_ConfigSerializedKeybind
    toggle_widget_gazing = {
      SK_Keybind {
        "Toggle Widget Gazing", L"",
          false, false, false, 255
      }, L"ToggleWidgetGazing"
    };

public:
  SKWG_Tobii (void) noexcept : SK_Widget ("Tobii")
  {
    SK_ImGui_Widgets->tobii = this;

    setAutoFit (true).setDockingPoint (DockAnchor::NorthEast).setClickThrough (false);
  };

  void save (iSK_INI* config_file) noexcept override
  {
    if (config_file == nullptr)
      return;

    if (_SK_Tobii_ShowGazeCursor != nullptr)
    {
      static auto gaze_cursor =
        tobii_cursor.get ();

      _SK_Tobii_ShowGazeCursor->store (gaze_cursor.draw);
      _SK_Tobii_WidgetGazing->store   (widget_gazing);
      _SK_Tobii_CursorType->store     (gaze_cursor.type);
      _SK_Tobii_CursorSize->store     (gaze_cursor.size);
      _SK_Tobii_CursorColor_R->store  (gaze_cursor.color [0]);
      _SK_Tobii_CursorColor_G->store  (gaze_cursor.color [1]);
      _SK_Tobii_CursorColor_B->store  (gaze_cursor.color [2]);
      _SK_Tobii_CursorColor_A->store  (gaze_cursor.color [3]);

      config_file->write (
        config_file->get_filename ()
      );
    }
  }

  bool keyboard (BOOL Control, BOOL Shift, BOOL Alt, BYTE vkCode) noexcept override
  {
    static auto gaze_cursor =
      tobii_cursor.get ();

#define SK_MakeKeyMask(vKey,ctrl,shift,alt)             \
    static_cast <UINT>((vKey) | (((ctrl) != 0) <<  9) | \
                                (((shift)!= 0) << 10) | \
                                (((alt)  != 0) << 11))

    if ( toggle_cursor.masked_code ==
           SK_MakeKeyMask ( vkCode,
                            Control != 0 ? 1 : 0,
                            Shift   != 0 ? 1 : 0,
                            Alt     != 0 ? 1 : 0 )
       )
    {
      gaze_cursor.draw = (! gaze_cursor.draw);
      return true;
    }

    if ( toggle_widget_gazing.masked_code ==
           SK_MakeKeyMask ( vkCode,
                            Control != 0 ? 1 : 0,
                            Shift   != 0 ? 1 : 0,
                            Alt     != 0 ? 1 : 0 )
       )
    {
      widget_gazing = (! widget_gazing);
      return true;
    }

    return false;
  }

  void run (void) override
  {
           bool first = false;
    SK_RunOnce (first = true);

    static auto gaze_cursor =
      tobii_cursor.get ();

    if (first)
    {
      _SK_Tobii_ShowGazeCursor =
        _CreateConfigParameterBool ( SK_TOBII_SECTION,
                                    L"ShowGazeCursor", gaze_cursor.draw,
                                    L"Display Gaze Cursor",
                                    L"Global/osd.ini" );

      _SK_Tobii_WidgetGazing =
        _CreateConfigParameterBool ( SK_TOBII_SECTION,
                                    L"EnableWidgetGazing", widget_gazing,
                                    L"Dynamic Widget Visibility",
                                    L"Global/osd.ini" );

      _SK_Tobii_CursorSize =
        _CreateConfigParameterInt ( SK_TOBII_SECTION,
                                   L"CursorSize", gaze_cursor.size,
                                   L"Size of Cursor",
                                   L"Global/osd.ini" );

      _SK_Tobii_CursorType =
        _CreateConfigParameterInt ( SK_TOBII_SECTION,
                                   L"CursorType", gaze_cursor.type,
                                   L"Type of Cursor",
                                   L"Global/osd.ini" );

      _SK_Tobii_CursorColor_R =
        _CreateConfigParameterFloat ( SK_TOBII_SECTION,
                                     L"CursorColor_R", gaze_cursor.color [0],
                                     L"Cursor Red Intensity",
                                     L"Global/osd.ini" );

      _SK_Tobii_CursorColor_G =
        _CreateConfigParameterFloat ( SK_TOBII_SECTION,
                                     L"CursorColor_G", gaze_cursor.color [1],
                                     L"Cursor Green Intensity",
                                     L"Global/osd.ini" );

      _SK_Tobii_CursorColor_B =
        _CreateConfigParameterFloat ( SK_TOBII_SECTION,
                                     L"CursorColor_B", gaze_cursor.color [2],
                                     L"Cursor Blue Intensity",
                                     L"Global/osd.ini" );

      _SK_Tobii_CursorColor_A =
        _CreateConfigParameterFloat ( SK_TOBII_SECTION,
                                     L"CursorColor_A", gaze_cursor.color [3],
                                     L"Cursor Alpha Intensity",
                                     L"Global/osd.ini" );


      auto keybinds = {
        &toggle_cursor,
        &toggle_widget_gazing
      };

      for ( auto& keybind : keybinds )
      {
        if (keybind == nullptr)
          continue;

        keybind->param =
          DeclKeybind (keybind, osd_ini, SK_TOBII_SECTION);

        if (! keybind->param->load (keybind->human_readable))
        {
          keybind->human_readable =
            L"Not Bound";
        }

        keybind->parse ();
        keybind->param->store (keybind->human_readable);
      }

      // Push the settings to INI file in case they don't exist
      SK_SaveConfig ();

      SK_Tobii_Startup (api, device);
    }
  }

  void draw (void) override
  {
    if (! ImGui::GetFont ())
      return;

    if (tobii_lost || tobii_reconnect > 0)
    {
      if (ImGui::Button ("Reconnect to Tobii Device"))
      {
        ++tobii_reconnect;
        SK_Tobii_Startup (api, device);
      }

      if (tobii_reconnect)
      {
        ImGui::SameLine ();
        ImGui::Text     ( "Device Reconnection Attempt: %li... ",
                            tobii_reconnect );
      }
    }

    else if (has_tobii)
    {
      static auto gaze_cursor =
        tobii_cursor.get ();

      bool draw_gaze_cursor =
        ( ( gaze_cursor.pos.x + 1.f ) *
          ( gaze_cursor.pos.y + 1.f ) ) > 1.0f;

      if ( gaze_cursor.draw     &&
             ( draw_gaze_cursor || isVisible () )
         )
      {
        ImVec2 vScreenCursorPos =
          ImGui::GetCursorScreenPos ();

        if (isVisible ())
        {
          ImGui::BeginGroup      ( );
          ImGui::PushStyleVar    (ImGuiStyleVar_ItemSpacing, ImVec2 (0, 8));
          ImGui::PushStyleColor  ( ImGuiCol_Border, ImVec4 (0.369f, 0.369f, 0.369f, 1.0f));
          ImGui::BeginChildFrame ( ImGui::GetID ("Tobii_Eye_Cursor"),
                                     ImVec2 ( MAX_CURSOR_SIZE + CURSOR_BORDER,
                                              MAX_CURSOR_SIZE + CURSOR_BORDER ) );
        }

        float fRadius =
                 0.5f * static_cast <float> (gaze_cursor.size);
        float fRd     =
          ( MAX_CURSOR_SIZE + CURSOR_BORDER )
                            - gaze_cursor.size;
        float fInset  = fRd / 2.0f;

        ImDrawList* draw_list =
          ImGui::GetWindowDrawList ();

        draw_list->PushClipRectFullScreen ();

        ImVec2
          window_coords [] = {
                        gaze_cursor.pos,

          ImVec2 ( vScreenCursorPos.x + fInset + fRadius,
                   vScreenCursorPos.y + fInset + fRadius )
        };

        std::pair <const ImVec2&, const bool>
          cursors [] = {
            { window_coords [0], draw_gaze_cursor },
            { window_coords [1],     isVisible () }
          };

        ImColor
          gaze_cursor_color ( gaze_cursor.color [0], gaze_cursor.color [1],
                              gaze_cursor.color [2], gaze_cursor.color [3] );

        for ( auto& cursor : cursors )
        {
          if (cursor.second)
          {
            switch (gaze_cursor.type)
            {
              default:
              case GAZE_CURSOR_OPEN_CIRCLE:
                draw_list->AddCircle        (cursor.first, fRadius, gaze_cursor_color);
                break;

              case GAZE_CURSOR_FILLED_CIRCLE:
                draw_list->AddCircleFilled  (cursor.first, fRadius, gaze_cursor_color);
                break;

              case GAZE_CURSOR_CROSSHAIR:
              {
                draw_list->AddLine ( ImVec2 ( cursor.first.x - fRadius,
                                              cursor.first.y ),
                                     ImVec2 ( cursor.first.x + fRadius,
                                              cursor.first.y ), gaze_cursor_color );
                draw_list->AddLine ( ImVec2 ( cursor.first.x,
                                              cursor.first.y - fRadius ),
                                     ImVec2 ( cursor.first.x,
                                              cursor.first.y + fRadius ),
                                                                gaze_cursor_color );
              } break;
            }
          }
        }

        if (isVisible ())
        {
          ImGui::PopStyleVar   ();
          ImGui::EndChildFrame ();
          ImGui::PopStyleColor ();
          ImGui::EndGroup      ();
        }

        draw_list->PopClipRect ();
      }

      if (! isVisible ()) return;
      bool changed = false;

      if (gaze_cursor.draw)
      {
        ImGui::SameLine ();

        ImGui::BeginGroup   ();
        ImGui::PushStyleVar (ImGuiStyleVar_ItemSpacing, ImVec2 (0, 8));

        changed |= ImGui::VSliderInt ( "##TobiiSize", ImVec2 (22, MAX_CURSOR_SIZE +
                                                                  CURSOR_BORDER ),
                                         &gaze_cursor.size,
                                                         16, int (MAX_CURSOR_SIZE) );
        ImGui::SameLine    ();
        ImGui::BeginGroup  ();

        changed |= ImGui::Combo      ( "Cursor Type",  &gaze_cursor.type,
                                          "Open Circle\0"
                                        "Filled Circle\0"
                                         "Cross Hair\0\0"                 );
        changed |= ImGui::ColorEdit3 ( "Cursor Color",  gaze_cursor.color );

        ImGui::EndGroup    ();
        ImGui::PopStyleVar ();
        ImGui::EndGroup    ();
        ImGui::Separator   ();
      }

      ImGui::Columns    (2, nullptr, false);
      ImGui::BeginGroup ( );
      changed |= ImGui::MenuItem ("Draw Gaze Cursor", nullptr, &gaze_cursor.draw);
      changed |= ImGui::MenuItem ("Widget Gazing",    nullptr, &widget_gazing);
      ImGui::EndGroup   ();
      ImGui::SameLine   (); ImGui::Spacing  ();
      ImGui::NextColumn ();
      ImGui::Spacing    (); ImGui::SameLine ();
      ImGui::BeginGroup ();
      ImGui::TextColored (
        ImVec4 (0.4f, 0.4f, 0.4f, 1.f), "%s",
        "Toggle Key: ");                   ImGui::SameLine   ();

      Keybinding ( &toggle_cursor,
                    toggle_cursor.param );
      ImGui::TextColored (
        ImVec4 (0.4f, 0.4f, 0.4f, 1.f), "%s",
        "Toggle Key: ");                   ImGui::SameLine   ();

      Keybinding ( &toggle_widget_gazing,
                    toggle_widget_gazing.param );
      ImGui::EndGroup ( );
      ImGui::Columns  (1);



      static tobii_device_info_t tdi = { };

      if (*tdi.firmware_version == '\0' && device != nullptr)
        tobii_get_device_info (device, &tdi);

      ImGui::Separator  ();
      ImGui::BeginGroup ();
      ImGui::BeginGroup ();
      ImGui::TextUnformatted ("Device: ");
      ImGui::TextUnformatted ("Firmware Version: ");
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      ImGui::TextColored (ImVec4 (0.5f, 0.5f, 0.5f, 1.f), "%s (%s)", tdi.model, tdi.generation);
      ImGui::TextColored (ImVec4 (0.5f, 0.5f, 0.5f, 1.f), "%s",      tdi.firmware_version);
      ImGui::EndGroup   ();
      ImGui::EndGroup   ();

      ImGui::SameLine   ();

      ImGui::BeginGroup ();
      tobii_supported_t supported =
        TOBII_NOT_SUPPORTED;
      tobii_capability_supported (
        device,
        TOBII_CAPABILITY_FACE_TYPE,
        &supported           );
      if (supported == TOBII_SUPPORTED)
      {
        ImGui::BulletText ("Supports Facial Recognition");
      }
      ImGui::EndGroup   ();

      if (changed)
      {
        SK_SaveConfig ();
      }
    }
  }

  void OnConfig (ConfigEvent event) noexcept override
  {
    switch (event)
    {
      case SK_Widget::ConfigEvent::LoadComplete:
        break;

      case SK_Widget::ConfigEvent::SaveStart:
        break;
    }
  }

protected:

private:
};

SK_LazyGlobal <SKWG_Tobii> __tobii_widget__;

void SK_Widget_InitTobii (void)
{
  SK_RunOnce (__tobii_widget__.getPtr ());
}

void
SK_Tobii_Startup ( tobii_api_t*&    api,
                   tobii_device_t*& device )
{
  if (has_tobii)
    return;

#ifdef _WIN64
#define LIBRARY_NAME L"tobii_stream_engine_sk64.dll"
#else
#define LIBRARY_NAME L"tobii_stream_engine_sk32.dll"
#endif

  wchar_t      wszDestination [MAX_PATH * 2 + 1] = { };

  wcscpy      (wszDestination, SK_GetDocumentsDir ().c_str ());
  PathAppendW (wszDestination, LR"(My Mods\SpecialK\PlugIns\ThirdParty\StreamEngine)");
  PathAppendW (wszDestination, LIBRARY_NAME);

  static HMODULE hModTobii;

  hModTobii =
    SK_Modules->LoadLibraryLL (wszDestination);

  if (! hModTobii)
  {
    // Unpack on first failure
    SK_UnpackTobiiStreamEngine ();

    // Give up on second
    hModTobii =
      SK_Modules->LoadLibraryLL (wszDestination);

    if (! hModTobii)
      return;
  }

  tobii_error_t error =
    tobii_api_create (&api, nullptr, nullptr);

  SK_ReleaseAssert (error == TOBII_ERROR_NO_ERROR)

    if (error != TOBII_ERROR_NO_ERROR)
    {
      FreeLibrary (hModTobii);
      return;
    }

  char url [256] = { 0 };

  error =
    tobii_enumerate_local_device_urls ( api,
                                       SK_Tobii_URL_Receiver,
                                       url );

  if (error != TOBII_ERROR_NO_ERROR || *url == '\0')
  {
    tobii_api_destroy (api);
    FreeLibrary       (hModTobii);

    api       = nullptr;
    hModTobii = nullptr;

    return;
  }

  error =
    tobii_device_create (api, url, &device);

  SK_ReleaseAssert (error == TOBII_ERROR_NO_ERROR)

  if (error != TOBII_ERROR_NO_ERROR)
  {
    tobii_api_destroy (api);
    FreeLibrary       (hModTobii);

    api       = nullptr;
    hModTobii = nullptr;

    return;
  }

  error =
    tobii_gaze_point_subscribe ( device,
                                   SK_Tobii_Callback_GazePoint,
                                     nullptr );

  SK_ReleaseAssert (error == TOBII_ERROR_NO_ERROR)

  if (error != TOBII_ERROR_NO_ERROR)
  {
    tobii_device_destroy (device);
    tobii_api_destroy    (api);
    FreeLibrary          (hModTobii);

    device    = nullptr;
    api       = nullptr;
    hModTobii = nullptr;

    return;
  }

  has_tobii  = true;
  tobii_lost = false;

  SK_Thread_Create ([](LPVOID module) -> DWORD
  {
    SK_Thread_SetCurrentPriority (THREAD_PRIORITY_BELOW_NORMAL);
    SetCurrentThreadDescription  (L"[SK] Tobii Raw Datastream");

    tobii_error_t error =
      TOBII_ERROR_NO_ERROR;

    int connection_tries = 0;

    while ( error == TOBII_ERROR_NO_ERROR          ||
            error == TOBII_ERROR_TIMED_OUT         ||
            error == TOBII_ERROR_CONNECTION_FAILED   )
    {
      if (error == TOBII_ERROR_CONNECTION_FAILED)
      {
        do
        {
          tobii_reconnect++;

          switch (connection_tries++)
          {
            case 0:
              SK_LOG0 ( ( L"Connection to Tobii Eyetracker was Lost, re-trying..." ),
                          L"Tobii Eyes");
            default:
            {
              static int  retry_wait_period = 150;
              SK_SleepEx (retry_wait_period, TRUE);

              error =
                tobii_device_reconnect (__tobii_widget__->device);

              retry_wait_period =
                ( error != TOBII_ERROR_NO_ERROR ) ?
                retry_wait_period * 2 : 150;
            } break;

            case 101:
            {
              SK_LOG0 ( ( L"Shutting Down Tobii Integration Because Device Cannot be Reconnected." ),
                          L"Tobii Eyes" );
              tobii_reconnect = 0;
            } break;
          }
        } while (error == TOBII_ERROR_CONNECTION_FAILED);
      }

      if (error == TOBII_ERROR_NO_ERROR)
        tobii_reconnect = 0;

      error =
        tobii_wait_for_callbacks (
          nullptr,   1,   &__tobii_widget__->device
        );

      assert ( error == TOBII_ERROR_NO_ERROR ||
               error == TOBII_ERROR_TIMED_OUT );

      if ( error != TOBII_ERROR_NO_ERROR &&
           error != TOBII_ERROR_TIMED_OUT )
      {
        continue;
      }

      if (error != TOBII_ERROR_TIMED_OUT)
      {
        error =
          tobii_device_process_callbacks ( __tobii_widget__->device );
      }
    }

    error =
      tobii_gaze_point_unsubscribe (__tobii_widget__->device);

    SK_ReleaseAssert (error == TOBII_ERROR_NO_ERROR);

    error =
      tobii_device_destroy (__tobii_widget__->device);

    SK_ReleaseAssert (error == TOBII_ERROR_NO_ERROR);

    error =
      tobii_api_destroy (__tobii_widget__->api);

    SK_ReleaseAssert (error == TOBII_ERROR_NO_ERROR);

    FreeLibrary ((HMODULE)module);

    __tobii_widget__->api    = nullptr;
    __tobii_widget__->device = nullptr;

    tobii_lost = true;

    SK_Thread_CloseSelf ();

    return 0;
  }, (LPVOID)hModTobii);
}