
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
    ini        = nullptr;
    //type_      = std::type_index (typeid (iParameter));
  }

  virtual ~iParameter (void) noexcept { }

  virtual std::wstring get_value_str (void)                     = 0;
  virtual void         set_value_str (const wchar_t *str)       = 0;
//virtual void         set_value_str (      std::wstring&  str) = 0;
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

  enum boolean_term_t {
    TrueFalse   = 0,
    OnOff       = 1,
    YesNo       = 2,
    ZeroNonZero = 3
  };

protected:
  boolean_term_t type = TrueFalse;
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

    std::type_index typ_idx =
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

#if 0
interface iSK_ParameterBase
{
  iSK_ParameterBase (void) {
    ini = nullptr;
  }

  STDMETHOD_ (std::wstring, get_value_str)(THIS)                   = 0;
  STDMETHOD_ (void,         set_value_str)(THIS_ std::wstring str) = 0;

  // Store value in INI and/or XML
  STDMETHOD_ (void,         store_str)    (THIS_ std::wstring str) = 0;

  STDMETHOD_ (void, register_to_ini)(THIS_ iSK_INI      *file,
                                           std::wstring  section,
                                           std::wstring  key );

protected:
private:
  iSK_INI*             ini;
  std::wstring         ini_section;
  std::wstring         ini_key;
};

template <typename _T>
interface iSK_Parameter : public iSK_ParameterBase
{
  iSK_Parameter (void) {
    ini = nullptr;
  }

  STDMETHOD_ (void,         set_value_str)(THIS_ std::wstring str) = 0;
  STDMETHOD_ (std::wstring, get_value_str)(THIS)                   = 0;

  // Store value in INI and/or XML
  STDMETHOD_ (void, store_str)(THIS_ std::wstring str) = 0;
  STDMETHOD_ (void, store)    (THIS_ _T           val) = 0;

  STDMETHOD_ (_T,           get_value)    (THIS)                   = 0;
  STDMETHOD_ (void,         set_value)    (THIS_ _T           val) = 0;

  // Read value from INI
  STDMETHOD_ (bool, load)(THIS_ _T& ref) = 0;

protected:
private:
  _T                   value;
};

interface iSK_ParameterInt : public iSK_Parameter <int>
{
  std::wstring get_value_str (void);
  int          get_value     (void);

  void         set_value     (int          val);
  void         set_value_str (std::wstring str);

  void         store         (int          val);
  void         store_str     (std::wstring str);

  bool         load          (int& ref);

protected:
  int value;
};

interface iSK_ParameterInt64 : public iSK_Parameter <int64_t>
{
  std::wstring get_value_str (void);
  int64_t      get_value     (void);

  void         set_value     (int64_t      val);
  void         set_value_str (std::wstring str);

  void         store         (int64_t      val);
  void         store_str     (std::wstring str);

  bool         load          (int64_t&     ref);

protected:
  int64_t value;
};

interface iSK_ParameterBool : public iSK_Parameter <bool>
{
  std::wstring get_value_str (void);
  bool         get_value     (void);

  void         set_value     (bool         val);
  void         set_value_str (std::wstring str);

  void         store         (bool         val);
  void         store_str     (std::wstring str);

  bool         load          (bool&        ref);

protected:
  bool value;
};

interface iSK_ParameterFloat : public iSK_Parameter <float>
{
  std::wstring get_value_str (void);
  float        get_value     (void);

  void         set_value     (float        val);
  void         set_value_str (std::wstring str);

  void         store         (float        val);
  void         store_str     (std::wstring str);

  bool         load          (float&       ref);

protected:
  float value;
};

interface iSK_ParameterStringW : public iSK_Parameter <std::wstring>
{
public:
  std::wstring get_value_str (void);
  std::wstring get_value     (void);

  void         set_value     (std::wstring str);
  void         set_value_str (std::wstring str);

  void         store         (std::wstring val);
  void         store_str     (std::wstring str);

  bool         load          (std::wstring& ref);


protected:
  std::wstring value;
};

interface iSK_ParameterFactory {
  template <typename _T> iSK_ParameterBase* create_parameter  (const wchar_t* name);

private:
  std::vector <iSK_ParameterBase *> params;
};
#endif

extern SK_LazyGlobal <sk::ParameterFactory> g_ParameterFactory;

#endif /* __SK__PARAMETER_H__ */