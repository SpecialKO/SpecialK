
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

#include "ini.h"

#include <Windows.h>
#include <vector>
#include <imgui/imgui.h>

namespace sk
{
class iParameter
{
public:
  iParameter (void) {
    ini        = nullptr;
  }

  virtual std::wstring get_value_str (void)             = 0;
  virtual void         set_value_str (std::wstring str) = 0;

  // Read value from INI
  bool load (void)
  {
    if (ini != nullptr)
    {
      iSK_INISection& section = ini->get_section (ini_section.c_str ());

      if (section.contains_key (ini_key.c_str ()))
      {
        set_value_str (section.get_value (ini_key.c_str ()));
        return true;
      }
    }

    return false;
  }

  // Store value in INI
  bool store (void)
  {
    bool ret = false;

    if (ini != nullptr)
    {
      iSK_INISection& section = ini->get_section (ini_section.c_str ());

      // If this operation actually creates a section, we need to make sure
      //   that section has a name!
      section.name = ini_section;

      if (section.contains_key (ini_key.c_str ()))
      {
        section.get_value (ini_key.c_str ()) = get_value_str ();
        ret = true;
      }

      // Add this key/value if it doesn't already exist.
      else {
        section.add_key_value (ini_key.c_str (), get_value_str ().c_str ());
        ret = true;// +1;
      }
    }

    return ret;
  }

  void register_to_ini (iSK_INI* file, std::wstring section, std::wstring key)
  {
    ini         = file;
    ini_section = section;
    ini_key     = key;
  }

protected:
private:
  iSK_INI*                 ini;
  std::wstring             ini_section;
  std::wstring             ini_key;
};

template <typename _T>
class Parameter : public iParameter {
public:
  virtual std::wstring get_value_str (void) = 0;
  virtual _T           get_value     (void) = 0;

  virtual void         set_value     (_T val)           = 0;
  virtual void         set_value_str (std::wstring str) = 0;

protected:
  _T                       value;
};

class ParameterInt : public Parameter <int>
{
public:
  std::wstring get_value_str (void);
  int          get_value     (void);

  void         set_value     (int val);
  void         set_value_str (std::wstring str);

protected:
  int value;
};

class ParameterInt64 : public Parameter <int64_t>
{
public:
  std::wstring get_value_str (void);
  int64_t      get_value     (void);

  void         set_value     (int64_t val);
  void         set_value_str (std::wstring str);

protected:
  int64_t value;
};

class ParameterBool : public Parameter <bool>
{
public:
  std::wstring get_value_str (void);
  bool         get_value     (void);

  void         set_value     (bool val);
  void         set_value_str (std::wstring str);

  enum boolean_term_t {
    TrueFalse   = 0,
    OnOff       = 1,
    YesNo       = 2,
    ZeroNonZero = 3
  };

protected:
  bool           value;
  boolean_term_t type = TrueFalse;
};

class ParameterFloat : public Parameter <float>
{
public:
  std::wstring get_value_str (void);
  float        get_value (void);

  void         set_value (float val);
  void         set_value_str (std::wstring str);

protected:
  float value;
};

class ParameterStringW : public Parameter <std::wstring>
{
public:
  std::wstring get_value_str (void);
  std::wstring get_value     (void);

  void         set_value     (std::wstring str);
  void         set_value_str (std::wstring str);

protected:
  std::wstring value;
};

class ParameterVec2f : public Parameter <ImVec2>
{
public:
  std::wstring get_value_str (void);
  ImVec2       get_value     (void);

  void         set_value     (ImVec2       val);
  void         set_value_str (std::wstring str);

protected:
  ImVec2 value;
};

class ParameterFactory {
public:
  template <typename _T> iParameter* create_parameter  (const wchar_t* name);
protected:
private:
  std::vector <iParameter *> params;
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

#endif /* __SK__PARAMETER_H__ */