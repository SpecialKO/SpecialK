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

#ifndef __SK__PARAMETER_H__
#define __SK__PARAMETER_H__

struct IUnknown;
#include <Unknwnbase.h>

#include <Windows.h>
#include <vector>
#include <typeindex>
#include <memory>
#include <mutex>

#include <string>

#include <imgui/imgui.h>

#ifndef IM_VEC2_DEFINED
#define IM_VEC2_DEFINED
struct ImVec2
{
    float x, y;

    ImVec2 (void)               noexcept { x = y = 0.0f;   }
    ImVec2 (float _x, float _y) noexcept { x = _x; y = _y; }

#ifdef IM_VEC2_CLASS_EXTRA          // Define constructor and implicit cast operators in imconfig.h to convert back<>forth from your math types and ImVec2.
    IM_VEC2_CLASS_EXTRA
#endif
};
#endif

interface iSK_INI;

namespace sk
{
class iParameter
{
public:
  iParameter (void) noexcept : type_ (std::type_index (typeid (iParameter))) {
    ini = nullptr;
  }

  virtual ~iParameter (void) noexcept { }

  virtual std::wstring get_value_str (void)                     = 0;
  virtual void         set_value_str (const wchar_t *str)       = 0;
  virtual void         set_value_str (const std::wstring&  str) = 0;

  // Read value from INI
  virtual bool load (void);

  // Store value in INI
  virtual bool store (void);

  void register_to_ini (            iSK_INI* file,
                         const std::wstring& section,
                         const std::wstring& key      )
  {
    ini         = file;
    ini_section = section;
    ini_key     = key;
  }

  // Returns true if a key exists but has no value
  virtual bool empty (void);

  std::type_index          type_;

protected:
private:
  iSK_INI*                 ini;
  std::wstring             ini_section;
  std::wstring             ini_key;
};

template <typename _T>
class Parameter : public iParameter {
public:
  typedef _T           value_type;

  virtual std::wstring get_value_str (void)                    = 0;
  virtual value_type   get_value     (void)                    = 0;

  virtual void         set_value     (      value_type    val) = 0;
  virtual void         set_value_str (const wchar_t*      str) = 0;
  virtual void         set_value_str (const std::wstring& str) = 0;

  virtual void         store         (      value_type    val) = 0;
  virtual void         store_str     (const wchar_t*      str) = 0;
  virtual void         store_str     (const std::wstring& str) = 0;
  virtual bool         load          (      value_type&   ref) = 0;

protected:
          value_type   value = { };
};

class ParameterInt : public Parameter <int>
{
public:
  ParameterInt (void) noexcept {
    type_ = std::type_index (typeid (int));
  }

  std::wstring get_value_str (void)                     override;
  int          get_value     (void)                     override;

  void         set_value     (      int            val) override;
  void         set_value_str (const wchar_t       *str) override;
  void         set_value_str (const std::wstring&  str) override;

  void         store         (      int            val) override;
  void         store_str     (const wchar_t       *str) override;
  void         store_str     (const std::wstring&  str) override;

  bool         load          (      int&           ref) override;
};

class ParameterInt64 : public Parameter <int64_t>
{
public:
  ParameterInt64 (void) noexcept {
    type_ = std::type_index (typeid (int64_t));
  }

  std::wstring get_value_str (void)                     override;
  int64_t      get_value     (void)                     override;

  void         set_value     (      int64_t        val) override;
  void         set_value_str (const wchar_t       *str) override;
  void         set_value_str (const std::wstring&  str) override;

  void         store         (      int64_t        val) override;
  void         store_str     (const wchar_t       *str) override;
  void         store_str     (const std::wstring&  str) override;

  bool         load          (      int64_t&       ref) override;
};

class ParameterBool : public Parameter <bool>
{
public:
  ParameterBool (void) noexcept {
    type_ = std::type_index (typeid (bool));
  }

  std::wstring get_value_str (void)                    override;
  bool         get_value     (void)                    override;

  void         set_value     (      bool          val) override;
  void         set_value_str (const wchar_t*      str) override;
  void         set_value_str (const std::wstring& str) override;

  void         store         (      bool          val) override;
  void         store_str     (const wchar_t*      str) override;
  void         store_str     (const std::wstring& str) override;

  bool         load          (      bool&         ref) override;

  enum class boolean_term_e {
    TrueFalse   = 0,
    OnOff       = 1,
    YesNo       = 2,
    ZeroNonZero = 3
  };

protected:
  boolean_term_e type = boolean_term_e::TrueFalse;
};

class ParameterFloat : public Parameter <float>
{
public:
  ParameterFloat (void) noexcept {
    type_ = std::type_index (typeid (float));
  }

  std::wstring get_value_str (void)                     override;
  float        get_value     (void)                     override;

  void         set_value     (      float          val) override;
  void         set_value_str (const wchar_t       *str) override;
  void         set_value_str (const std::wstring&  str) override;

  void         store         (      float          val) override;
  void         store_str     (const wchar_t       *str) override;
  void         store_str     (const std::wstring&  str) override;

  bool         load          (      float&         ref) override;
};

class ParameterStringW : public Parameter <std::wstring>
{
public:
  ParameterStringW (void) noexcept {
    type_ = std::type_index (typeid (std::wstring));
  }

  std::wstring get_value_str (void)                     override;
  std::wstring get_value     (void)                     override;

  void         set_value     (       std::wstring  str) override;
  void         set_value     (const wchar_t       *val);
  void         set_value_str (const wchar_t       *str) override;
  void         set_value_str (const std::wstring&  str) override;

  void         store         (      std::wstring   val) override;
  void         store_str     (const wchar_t       *str) override;
  void         store_str     (const std::wstring&  str) override;

  bool         load          (      std::wstring&  ref) override;

         std::wstring& get_value_ref (void);
};

class ParameterVec2f : public Parameter <ImVec2>
{
public:
  ParameterVec2f (void) noexcept {
    type_ = std::type_index (typeid (ImVec2));
  }

  std::wstring get_value_str (void)                     override;
  ImVec2       get_value     (void)                     override;

  void         set_value     (      ImVec2         val) override;
  void         set_value_str (const wchar_t       *str) override;
  void         set_value_str (const std::wstring&  str) override;

  void         store         (      ImVec2         val) override;
  void         store_str     (const wchar_t       *str) override;
  void         store_str     (const std::wstring&  str) override;

  bool         load          (      ImVec2&        ref) override;
};

class ParameterFactory {
public:
  template <typename T>
              iParameter* create_parameter (const wchar_t* name)
  {
    UNREFERENCED_PARAMETER (name);

    std::scoped_lock <std::mutex> _scope_lock (lock);

    const std::type_index typ_idx =
          std::type_index (typeid (T));

    if ( typ_idx ==
           std::type_index (typeid (int)) )
    {
      params.emplace_back (
        std::make_unique <ParameterInt> ()
      );
    }

    else if ( typ_idx ==
                std::type_index (typeid (int64_t)) )
    {
      params.emplace_back (
        std::make_unique <ParameterInt64> ()
      );
    }

    else if ( typ_idx ==
                std::type_index (typeid (bool)) )
    {
      params.emplace_back (
        std::make_unique <ParameterBool> ()
      );
    }

    else if ( typ_idx ==
                std::type_index (typeid (float)) )
    {
      params.emplace_back (
        std::make_unique <ParameterFloat> ()
      );
    }

    else if ( typ_idx ==
                std::type_index (typeid (std::wstring)) )
    {
      params.emplace_back (
        std::make_unique <ParameterStringW> ()
      );
    }

    else if ( typ_idx ==
                std::type_index (typeid (ImVec2)) )
    {
      params.emplace_back (
        std::make_unique <ParameterVec2f> ()
      );
    }

    else
      SK_ReleaseAssert (! L"Unknown Parameter Type");

    return params.back ().get ();
  }
  //----------------------------------------------------------------------------
  //template <> iParameter* create_parameter <int>          (const wchar_t* name);
  //template <> iParameter* create_parameter <int64_t>      (const wchar_t* name);
  //template <> iParameter* create_parameter <bool>         (const wchar_t* name);
  //template <> iParameter* create_parameter <float>        (const wchar_t* name);
  //template <> iParameter* create_parameter <std::wstring> (const wchar_t* name);
  //template <> iParameter* create_parameter <ImVec2>       (const wchar_t* name);

protected:
private:
  std::mutex                                 lock;
  std::vector <std::unique_ptr <iParameter>> params;
};
}

extern SK_LazyGlobal <sk::ParameterFactory> g_ParameterFactory;

#endif /* __SK__PARAMETER_H__ */