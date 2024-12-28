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

struct IUnknown;
#include <Unknwnbase.h>

#include <imgui/imgui.h>
#include <input/input.h>
#include <SpecialK/config.h>
#include <SpecialK/parameter.h>
#include <SpecialK/ini.h>

#include <typeindex>
#include <string>
#include <map>

struct ImGuiWindow;
class  SK_Widget;

struct SK_ImGui_WidgetRegistry
{
  bool texcache = false;

  SK_Widget* frame_pacing    = nullptr;
  SK_Widget* latency         = nullptr;
  SK_Widget* volume_control  = nullptr;
  SK_Widget* gpu_monitor     = nullptr;
  SK_Widget* cpu_monitor     = nullptr;
  SK_Widget* d3d11_pipeline  = nullptr;
  SK_Widget* thread_profiler = nullptr;

  SK_Widget* hdr_control     = nullptr;
  SK_Widget* tobii           = nullptr;

  SK_Widget* cmd_console     = nullptr;
  SK_Widget* txt_editor      = nullptr;
  SK_Widget* file_browser    = nullptr;

  //SK_Widget* texcache;

  SK_Widget* memory_monitor = nullptr;
  SK_Widget* disk_monitor   = nullptr;

  BOOL DispatchKeybinds ( BOOL Control, BOOL Shift,
                          BOOL Alt,     BYTE vkCode );
  BOOL SaveConfig (void);

  bool   hide_all = false;
  float  scale    = 1.0f;
};

extern SK_LazyGlobal <sk::ParameterFactory>    SK_Widget_ParameterFactory;
extern SK_LazyGlobal <SK_ImGui_WidgetRegistry> SK_ImGui_Widgets;

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

  virtual void run  (void) { };
  virtual void draw (void) { };

  virtual void run_base    (void);
  virtual void draw_base   (void);
  virtual void config_base (void);

  virtual bool keyboard ( BOOL Control, BOOL Shift,
                          BOOL Alt,     BYTE vkCode )
  {
    UNREFERENCED_PARAMETER (Control); UNREFERENCED_PARAMETER (Shift);
    UNREFERENCED_PARAMETER (Alt);     UNREFERENCED_PARAMETER (vkCode);

    return false;
  };


  SK_Widget& setName          (const char*   szName)                 { name           = szName;         return *this; }
  SK_Widget& setScale         (float         fScale)        noexcept { scale          = fScale;         return *this; }
//---------------------
  SK_Widget& setVisible       (bool          bVisible)      noexcept { visible        = bVisible;
                                                                   if (visible)  {
                                                            setActive (visible); }                      return *this; }
  SK_Widget& flashVisible     (void)                        noexcept { last_flash     =
                                                 static_cast <float> ( SK_GetCurrentMS () ) / 1000.0f;  return *this; }
  SK_Widget& setActive        (bool          bActive)       noexcept { active         = bActive;        return *this; }
//--------------------
  SK_Widget& setMovable       (bool          bMovable)      noexcept { movable        = bMovable;       return *this; }
  SK_Widget& setResizable     (bool          bResizable)    noexcept { resizable      = bResizable;     return *this; }
  SK_Widget& setAutoFit       (bool          bAutofit)      noexcept { autofit        = bAutofit;       return *this; }
  SK_Widget& setBorder        (bool          bBorder)       noexcept { border         = bBorder;        return *this; }
  SK_Widget& setClickThrough  (bool          bClickthrough) noexcept { click_through  = bClickthrough;  return *this; }
  SK_Widget& setFlashDuration (float         fSeconds)      noexcept { flash_duration = fSeconds;       return *this; }
  SK_Widget& setMinSize       (const ImVec2& iv2MinSize)    noexcept { min_size       = iv2MinSize;     return *this; }
  SK_Widget& setMaxSize       (const ImVec2& iv2MaxSize)    noexcept { max_size       = iv2MaxSize;     return *this; }
  SK_Widget& setSize          (const ImVec2& iv2Size)       noexcept { size           = iv2Size;        return *this; }
  SK_Widget& setPos           (const ImVec2& iv2Pos)        noexcept { pos            = iv2Pos;         return *this; }
  SK_Widget& setDockingPoint  (DockAnchor    dock_anchor)   noexcept { docking        = dock_anchor;    return *this; }
  SK_Widget& setAlpha         (float         fAlpha)        noexcept { alpha          = fAlpha;         return *this; }


  const std::string& getName          (void) const noexcept { return    name;                   }
        float        getScale         (void) const noexcept { return    scale;                  }
        bool         isVisible        (void) const noexcept { return  ( visible || isFlashed () )
                                                         && (! SK_ImGui_Widgets->hide_all);     }
        bool         isFlashed        (void) const noexcept { return    last_flash >
                                            static_cast <float> (SK_GetCurrentMS ()) / 1000.0f -
                                                                                flash_duration; }
        bool         isActive         (void) const noexcept { return    active || isFlashed (); }
        bool         isMovable        (void) const noexcept { return    movable;                }
        bool         isResizable      (void) const noexcept { return    resizable;              }
        bool         isAutoFitted     (void) const noexcept { return    autofit;                }
        bool         isClickable      (void) const noexcept { return (! click_through);         }
        bool         hasBorder        (void) const noexcept { return    border;                 }

        float        getFlashDuration (void) const noexcept { return    flash_duration;         }
  const ImVec2&      getMinSize       (void) const noexcept { return    min_size;               }
  const ImVec2&      getMaxSize       (void) const noexcept { return    max_size;               }
  const ImVec2&      getSize          (void) const noexcept { return    size;                   }
  const ImVec2&      getPos           (void) const noexcept { return    pos;                    }
  const DockAnchor&  getDockingPoint  (void) const noexcept { return    docking;                }
        float        getAlpha         (void) const noexcept { return    alpha;                  }

  const SK_Keybind&  getToggleKey     (void) const noexcept { return    toggle_key;             }
  const SK_Keybind&  getFocusKey      (void) const noexcept { return    focus_key;              }
  const SK_Keybind&  getFlashKey      (void) const noexcept { return    flash_key;              }

  virtual ~SK_Widget (void) noexcept { };


protected:
  friend struct SK_ImGui_WidgetRegistry;

  virtual void OnConfig (ConfigEvent event) { UNREFERENCED_PARAMETER (event); };

  virtual void load (iSK_INI* config_file);
  virtual void save (iSK_INI* config_file);

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
  SK_Keybind flash_key = {
    "Widget Flash Keybind", L"",
     false, false, false, 0, 0
  };

  sk::ParameterStringW* toggle_key_val       = nullptr;
  sk::ParameterStringW* focus_key_val        = nullptr;
  sk::ParameterStringW* flash_key_val        = nullptr;

  sk::ParameterBool*    param_visible        = nullptr;
  sk::ParameterBool*    param_movable        = nullptr;
  sk::ParameterBool*    param_autofit        = nullptr;
  sk::ParameterBool*    param_resizable      = nullptr;
  sk::ParameterBool*    param_border         = nullptr;
  sk::ParameterBool*    param_clickthrough   = nullptr;
  sk::ParameterVec2f*   param_minsize        = nullptr;
  sk::ParameterVec2f*   param_maxsize        = nullptr;
  sk::ParameterVec2f*   param_size           = nullptr;
  sk::ParameterVec2f*   param_pos            = nullptr;
  sk::ParameterInt*     param_docking        = nullptr;
  sk::ParameterFloat*   param_scale          = nullptr;
  sk::ParameterFloat*   param_flash_duration = nullptr;
  sk::ParameterFloat*   param_alpha          = nullptr;
  sk::ParameterFloat*   param_nits           = nullptr;
  // TODO: Add memory allocator and timing so that performance and resource
  //         consumption for individual widgets can be tracked.


//private:
  std::string name           = "###UninitializedWidget";

  float       scale          = 1.0f;

  bool        visible        = false;
  bool        active         = false;
  bool        locked         = false;
  bool        autofit        = true;
  bool        movable        = true;
  bool        resizable      = true;
  bool        border         = true;
  bool        click_through  = true;
  float       flash_duration = 1.5f;
  float       alpha          = 1.0f;
  float       nits           = 80.0f;

  ImVec2      min_size       = ImVec2 ( 375.0,  240.0);
  ImVec2      max_size       = ImVec2 (2048.0, 2048.0);
  ImVec2      size           = ImVec2 ( 375.0,  240.0); // Values (-1,1) are scaled to resolution
  ImVec2      pos            = ImVec2 (   0.0,    0.0); // Values (-∞,1] and [1,∞) are absolute

  DockAnchor docking         = DockAnchor::None;

  // Custom params
  std::map <std::string, sk::iParameter *> parameters;


  int state__;   // 0 = Normal, 1 = Config, SK_WS_USER = First User-Defined State
  int version__;

private:
  float          last_flash  = 0.0f; // Temporarily show a widget if "flashed"
  bool           run_once__  = false;
  ImGuiWindow*   pWindow__   = nullptr;
  bool           moved       = false;
};



static
__inline
sk::ParameterStringW*
LoadWidgetKeybind ( SK_Keybind *binding,
                    iSK_INI    *ini_file,
                const wchar_t*  wszDesc,
            const std::wstring& sec_name,
            const std::wstring& key_name )
{
  if (! binding)
    return nullptr;

  sk::ParameterStringW* ret =
    dynamic_cast <sk::ParameterStringW *>
      (SK_Widget_ParameterFactory->create_parameter <std::wstring> (wszDesc));

  if (ret != nullptr)
  {
    ret->register_to_ini ( ini_file, sec_name, key_name );

    if (! ret->load (binding->human_readable))
    {
      binding->parse  ();
      ret->store      (binding->human_readable);
    }

    binding->human_readable = ret->get_value ();
    binding->parse  ();
  }

  return ret;
}

static
__inline
sk::ParameterBool*
LoadWidgetBool ( bool    *pbVal,
                 iSK_INI *ini_file,
          const wchar_t*  wszDesc,
      const std::wstring& sec_name,
      const std::wstring& key_name )
{
  sk::ParameterBool* ret =
    dynamic_cast <sk::ParameterBool *>
      (SK_Widget_ParameterFactory->create_parameter <bool> (wszDesc));

  if (ret != nullptr)
  {
    ret->register_to_ini ( ini_file,
                             sec_name,
                               key_name );

    if (pbVal != nullptr)
    {
      if (! ret->load (*pbVal))
      {
        ret->store    (*pbVal);
      }

      *pbVal = ret->get_value ();
    }
  }

  return ret;
}

static
__inline
sk::ParameterInt*
LoadWidgetDocking ( SK_Widget::DockAnchor *pdaVal,
                                  iSK_INI *ini_file,
                           const wchar_t*  wszDesc,
                       const std::wstring& sec_name,
                       const std::wstring& key_name )
{
  sk::ParameterInt* ret =
   dynamic_cast <sk::ParameterInt *>
    (SK_Widget_ParameterFactory->create_parameter <int> (wszDesc));

  if (ret != nullptr)
  {
    ret->register_to_ini ( ini_file, sec_name, key_name );

    if (pdaVal != nullptr)
    {
      if (! ret->load (*(int *)pdaVal))
      {
        ret->store    (*(int *)pdaVal);
      }

      *(int *)pdaVal = ret->get_value ();
    }
  }

  return ret;
}

static
__inline
sk::ParameterVec2f*
LoadWidgetVec2 ( ImVec2  *piv2Val,
                 iSK_INI *ini_file,
          const wchar_t*  wszDesc,
      const std::wstring& sec_name,
      const std::wstring& key_name )
{
  sk::ParameterVec2f* ret =
   dynamic_cast <sk::ParameterVec2f *>
    (SK_Widget_ParameterFactory->create_parameter <ImVec2> (wszDesc));

  if (ret != nullptr)
  {
    ret->register_to_ini ( ini_file, sec_name, key_name );

    if (piv2Val != nullptr)
    {
      if (! ret->load (*piv2Val))
      {
        ret->store    (*piv2Val);
      }

      *piv2Val = ret->get_value ();
    }
  }

  return ret;
}

static
__inline
sk::ParameterFloat*
LoadWidgetFloat ( float  *pfVal,
                 iSK_INI *ini_file,
          const wchar_t*  wszDesc,
      const std::wstring& sec_name,
      const std::wstring& key_name )
{
  sk::ParameterFloat* ret =
   dynamic_cast <sk::ParameterFloat *>
    (SK_Widget_ParameterFactory->create_parameter <float> (wszDesc));

  if (ret != nullptr)
  {
    ret->register_to_ini ( ini_file, sec_name, key_name );

    if (pfVal != nullptr)
    {
      if (! ret->load (*pfVal))
      {
        ret->store    (*pfVal);
      }

      *pfVal = ret->get_value ();
    }
  }

  return ret;
}




#include <array>

template <typename _T, int max_samples>
class SK_Stat_DataHistory
{
public:
  int   getCapacity   (void) noexcept
  {
    return max_samples;
  };

  int   getUpdates    (void) noexcept
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

  _T    getLastValue (void) noexcept
  {
    return last_val;
  }


  void addValue (_T val, bool only_if_different = false) noexcept
  {
    const bool insert =
      (! only_if_different) || fabs (last_val - val) > std::numeric_limits <_T>::epsilon ();

    if (insert)
    {
      values [values_offset] = val;
              values_offset  = (values_offset + 1) % getCapacity ();

      ++updates;

      last_val = val;
    }
  }


  std::type_info getType (void) noexcept
  {
    return std::type_index (typeid (_T));
  }


  _T getMin (void) noexcept
  {
    calcStats ();

    return cached_stats.min;
  }

  _T getMax (void) noexcept
  {
    calcStats ();

    return cached_stats.max;
  }

  _T getAvg (void) noexcept
  {
    calcStats ();

    return cached_stats.avg;
  }



  int                           getOffset (void) noexcept { return values_offset; };
  std::array <_T, max_samples>& getValues (void) noexcept { return values;        };

  void   reset     (void) noexcept
  {
    last_val      = 0;
    values_offset = 0;
    updates       = 0;

    std::fill_n (values.begin (), max_samples, (_T)0);

    cached_stats.reset ();
  }

protected:
  void calcStats (void) noexcept
  {
    if (cached_stats.last_calc != updates)
    {
      int sample =  0;
      _T  sum    =  0;

      _T  min    = std::numeric_limits <_T>::max ();
      _T  max    = std::numeric_limits <_T>::min ();

      for (auto val : values)
      {
        if ((sample + 1) > updates)
          break;

        sum += val;

        if (++sample == 1) { max = val; min = val; }

        else
        {
          max = val != max ? std::max (val, max) : max;
          min = val != min ? std::min (val, min) : min;
        }
      }

      if (sample > 0)
      {
        cached_stats.avg = sum / sample;
        cached_stats.min = min;
        cached_stats.max = max;
      }

      else
      {
        cached_stats.avg = 0;
        cached_stats.min = 0;
        cached_stats.max = 0;
      }

      cached_stats.last_calc = updates;
    }
  }

  int                          updates       = 0;
  std::array <_T, max_samples> values        = { };
  int                          values_offset = 0;
  _T                           last_val      = 0;

  struct stat_cache_s {
    _T  min       = 0,
        max       = 0,
        avg       = 0;
    int last_calc = 0;

    void reset (void)
    {
            min = 0;
            max = 0;
            avg = 0;
      last_calc = 0;
    }
  } cached_stats;
};

bool SK_Widget_InitEverything    (void);
     SK_Widget* SK_HDR_GetWidget (void);

#endif /* __SK__WIDGET_H__ */