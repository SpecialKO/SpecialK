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

#ifndef __SK__WIDGET_H__
#define __SK__WIDGET_H__

#include <imgui/imgui.h>
#include <input/input.h>
#include <SpecialK/config.h>
#include <SpecialK/parameter.h>
#include <SpecialK/ini.h>

#include <string>
#include <map>

extern sk::ParameterFactory SK_Widget_ParameterFactory;

struct ImGuiWindow;
class  SK_Widget;

struct SK_ImGui_WidgetRegistry
{
  bool texcache = false;

  SK_Widget* frame_pacing;
  SK_Widget* volume_control;
  SK_Widget* gpu_monitor;
  SK_Widget* cpu_monitor;
  SK_Widget* d3d11_pipeline;

  //SK_Widget* texcache;

  SK_Widget* memory_monitor = nullptr;
  SK_Widget* disk_monitor   = nullptr;

  BOOL DispatchKeybinds (BOOL Control, BOOL Shift, BOOL Alt, BYTE vkCode);
  BOOL SaveConfig (void);

  bool  hide_all = false;
  float scale    = 1.0f;
} extern SK_ImGui_Widgets;



class SK_Widget
{
public:
  enum class DockAnchor
  {
    None  = 0x00,

    North = 0x01,
    South = 0x02,
    East  = 0x10,
    West  = 0x20,

    NorthEast = North | East,
    SouthEast = South | East,
    NorthWest = North | West,
    SouthWest = South | West,

  };

  enum class ConfigEvent
  {
    LoadStart,
    LoadComplete,
    SaveStart,
    SaveComplete,
  };

  virtual void run    (void) { };
  virtual void draw   (void) { };


  void run_base    (void);
  void draw_base   (void);
  void config_base (void);


  SK_Widget& setName         (const char* szName)        { name          = szName;        return *this; }
  SK_Widget& setScale        (float       fScale)        { scale         = fScale;        return *this; }
//---------------------
  SK_Widget& setVisible      (bool        bVisible)      { visible       = bVisible;
                                                           if (visible)
                                                             setActive (visible);

                                                           if (param_visible != nullptr)
                                                           {
                                                             param_visible->set_value (visible);
                                                             param_visible->store     ();
                                                           }
                                                                                          return *this; }
  SK_Widget& setActive       (bool        bActive)       { active        = bActive;       return *this; }
//--------------------
  SK_Widget& setMovable      (bool        bMovable)      { movable       = bMovable;      return *this; }
  SK_Widget& setResizable    (bool        bResizable)    { resizable     = bResizable;    return *this; }
  SK_Widget& setAutoFit      (bool        bAutofit)      { autofit       = bAutofit;      return *this; }
  SK_Widget& setBorder       (bool        bBorder)       { border        = bBorder;       return *this; }
  SK_Widget& setClickThrough (bool        bClickthrough) { click_through = bClickthrough; return *this; }
  SK_Widget& setMinSize      (ImVec2&     iv2MinSize)    { min_size      = iv2MinSize;    return *this; }
  SK_Widget& setMaxSize      (ImVec2&     iv2MaxSize)    { max_size      = iv2MaxSize;    return *this; }
  SK_Widget& setSize         (ImVec2&     iv2Size)       { size          = iv2Size;       return *this; }
  SK_Widget& setPos          (ImVec2&     iv2Pos)        { pos           = iv2Pos;        return *this; }
  SK_Widget& setDockingPoint (DockAnchor  dock_anchor)   { docking       = dock_anchor;   return *this; }


  const std::string& getName         (void) const { return    name;           }
        float        getScale        (void) const { return    scale;          }
        bool         isVisible       (void) const { return    visible &&
                                               (! SK_ImGui_Widgets.hide_all); }
        bool         isActive        (void) const { return    active;         }
        bool         isMovable       (void) const { return    movable;        }
        bool         isResizable     (void) const { return    resizable;      }
        bool         isAutoFitted    (void) const { return    autofit;        }
        bool         isClickable     (void) const { return (! click_through); }
        bool         hasBorder       (void) const { return    border;         }
  const ImVec2&      getMinSize      (void) const { return    min_size;       }
  const ImVec2&      getMaxSize      (void) const { return    max_size;       }
  const ImVec2&      getSize         (void) const { return    size;           }
  const ImVec2&      getPos          (void) const { return    pos;            }
  const DockAnchor&  getDockingPoint (void) const { return    docking;        }

  const SK_Keybind&  getToggleKey    (void) const { return    toggle_key;     }
  const SK_Keybind&  getFocusKey     (void) const { return    focus_key;      }


protected:
  friend struct SK_ImGui_WidgetRegistry;

  virtual void OnConfig (ConfigEvent event) { UNREFERENCED_PARAMETER (event); };

  void load (iSK_INI* config);
  void save (iSK_INI* config);

  // This will be private when I get the factory setup
  SK_Widget (const char* szName) : name (szName)
  {
    version__ = 1;
    state__   = 0;
  }

  //SK_Input_KeyBinding toggle_key;
  //SK_Input_KeyBinding focus_key;
  SK_Keybind toggle_key = {
    "Widget Toggle Keybind", L"",
     false, false, false, 0, 0
  };
  SK_Keybind focus_key = {
    "Widget Focus Keybind", L"",
     false, false, false, 0, 0
  };

  sk::ParameterStringW* toggle_key_val;
  sk::ParameterStringW* focus_key_val;

  sk::ParameterBool*    param_visible;
  sk::ParameterBool*    param_movable;
  sk::ParameterBool*    param_autofit;
  sk::ParameterBool*    param_resizable;
  sk::ParameterBool*    param_border;
  sk::ParameterBool*    param_clickthrough;
  sk::ParameterVec2f*   param_minsize;
  sk::ParameterVec2f*   param_maxsize;
  sk::ParameterVec2f*   param_size;
  sk::ParameterVec2f*   param_pos;
  sk::ParameterInt*     param_docking;
  sk::ParameterFloat*   param_scale;

  // TODO: Add memory allocator and timing so that performance and resource
  //         consumption for individual widgets can be tracked.


//private:
  std::string name          = "###UninitializedWidget";

  float       scale         = 1.0f;

  bool        visible       = false;
  bool        active        = false;
  bool        locked        = false;
  bool        autofit       = true;
  bool        movable       = true;
  bool        resizable     = true;
  bool        border        = false;
  bool        click_through = false;

  ImVec2      min_size      = ImVec2 ( 375.0,  240.0);
  ImVec2      max_size      = ImVec2 (1024.0, 1024.0);
  ImVec2      size          = ImVec2 ( 375.0,  240.0); // Values (-1,1) are scaled to resolution
  ImVec2      pos           = ImVec2 (   0.0,    0.0); // Values (-∞,1] and [1,∞) are absolute

  DockAnchor docking        = DockAnchor::None;

  // Custom params
  std::map <std::string, sk::iParameter *> parameters;


  int state__;   // 0 = Normal, 1 = Config, SK_WS_USER = First User-Defined State
  int version__;

private:
  bool           run_once__ = false;
  ImGuiWindow*   pWindow__  = nullptr;
  bool           moved      = false;
};



static
__inline
sk::ParameterStringW*
LoadWidgetKeybind (SK_Keybind* binding, iSK_INI* ini_file, const wchar_t* wszDesc, const wchar_t* sec_name, const wchar_t* key_name)
{
  sk::ParameterStringW* ret =
   dynamic_cast <sk::ParameterStringW *>
    (SK_Widget_ParameterFactory.create_parameter <std::wstring> (wszDesc));

  ret->register_to_ini ( ini_file, sec_name, key_name );

  if (! ret->load ())
  {
    binding->parse  ();
    ret->set_value  (binding->human_readable);
    ret->store      ();
  }

  binding->human_readable = ret->get_value ();
  binding->parse  ();

  return ret;
}

static
__inline
sk::ParameterBool*
LoadWidgetBool (bool* pbVal, iSK_INI* ini_file, const wchar_t* wszDesc, const wchar_t* sec_name, const wchar_t* key_name)
{
  sk::ParameterBool* ret =
   dynamic_cast <sk::ParameterBool *>
    (SK_Widget_ParameterFactory.create_parameter <bool> (wszDesc));

  ret->register_to_ini ( ini_file, sec_name, key_name );

  if (! ret->load ())
  {
    ret->set_value  (*pbVal);
    ret->store      ();
  }

  *pbVal = ret->get_value ();

  return ret;
}

static
__inline
sk::ParameterInt*
LoadWidgetDocking (SK_Widget::DockAnchor* pdaVal, iSK_INI* ini_file, const wchar_t* wszDesc, const wchar_t* sec_name, const wchar_t* key_name)
{
  sk::ParameterInt* ret =
   dynamic_cast <sk::ParameterInt *>
    (SK_Widget_ParameterFactory.create_parameter <int> (wszDesc));

  ret->register_to_ini ( ini_file, sec_name, key_name );

  if (! ret->load ())
  {
    ret->set_value  (*(int *)pdaVal);
    ret->store      ();
  }

  *(int *)pdaVal = ret->get_value ();

  return ret;
}

static
__inline
sk::ParameterVec2f*
LoadWidgetVec2 (ImVec2* piv2Val, iSK_INI* ini_file, const wchar_t* wszDesc, const wchar_t* sec_name, const wchar_t* key_name)
{
  sk::ParameterVec2f* ret =
   dynamic_cast <sk::ParameterVec2f *>
    (SK_Widget_ParameterFactory.create_parameter <ImVec2> (wszDesc));

  ret->register_to_ini ( ini_file, sec_name, key_name );

  if (! ret->load ())
  {
    ret->set_value  (*piv2Val);
    ret->store      ();
  }

  *piv2Val = ret->get_value ();

  return ret;
}




#include <array>

template <typename _T, int max_samples>
class SK_Stat_DataHistory
{
public:
  int   getCapacity   (void)
  {
    return max_samples;
  };

  int   getUpdates    (void)
  {
    return updates;
  }

  _T    getEntry      (int idx)
  {
    if (idx + values_offset < max_samples)
      return values [idx + values_offset];

    else
      return values [(idx + values_offset) % max_samples];
  }


  void addValue (_T val, bool only_if_different = false)
  {
    bool insert = (! only_if_different) || (last_val != val);

    if (insert)
    {
      values [values_offset] = val;
              values_offset  = (values_offset + 1) % getCapacity ();

      ++updates;

      last_val = val;
    }
  }


  std::type_info getType (void)
  {
    return std::typeindex (typeid (_T));
  }


  _T getMin (void)
  {
    calcStats ();

    return cached_stats.min;
  }

  _T getMax (void)
  {
    calcStats ();

    return cached_stats.max;
  }

  _T getAvg (void)
  {
    calcStats ();

    return cached_stats.avg;
  }



  int                           getOffset (void) { return values_offset; };
  std::array <_T, max_samples>& getValues (void) { return values;        };

  void   reset     (void)
  {
    values_offset = 0;
    updates       = 0;
  }

protected:
  void calcStats (void)
  {
    if (cached_stats.last_calc != updates)
    {
      int sample =  0;
      _T  sum    = { };

      _T  min    = std::numeric_limits <_T>::max ();
      _T  max    = std::numeric_limits <_T>::min ();

      for (auto& val : values)
      {
        if (++sample > updates)
          break;

        sum += val;

        max = std::max (val, max);
        min = std::min (val, min);
      }

      cached_stats.avg = sum / sample;
      cached_stats.min = min;
      cached_stats.max = max;

      cached_stats.last_calc = updates;
    }
  }

  int                          updates       = 0;
  std::array <_T, max_samples> values;
  int                          values_offset = 0;
  _T                           last_val      = { };

  struct stat_cache_s {
    _T  min, max, avg;
    int last_calc = 0;
  } cached_stats;
};



#endif /* __SK__WIDGET_H__ */