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

namespace sk
{
class iParameter
{
public:
  iParameter (void) {
    ini        = nullptr;
  }

  virtual std::wstring get_value_str (void) = 0;
  virtual void         set_value_str (std::wstring str) = 0;

  // Read value from INI
  bool load (void)
  {
    if (ini != nullptr) {
      INI::File::Section& section = ini->get_section (ini_section);

      if (section.contains_key (ini_key)) {
        set_value_str (section.get_value (ini_key));
        return true;
      }
    }

    return false;
  }

  // Store value in INI
  bool store (void)
  {
    bool ret = false;

    if (ini != nullptr) {
      INI::File::Section& section = ini->get_section (ini_section);

      // If this operation actually creates a section, we need to make sure
      //   that section has a name!
      section.name = ini_section;

      if (section.contains_key (ini_key)) {
        section.get_value (ini_key) = get_value_str ();
        ret = true;
      }

      // Add this key/value if it doesn't already exist.
      else {
        section.add_key_value (ini_key, get_value_str ());
        ret = true;// +1;
      }
    }

    return ret;
  }

  void register_to_ini (INI::File* file, std::wstring section, std::wstring key)
  {
    ini         = file;
    ini_section = section;
    ini_key     = key;
  }

protected:
private:
  INI::File*               ini;
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

protected:
  bool value;
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

class ParameterFactory {
public:
  template <typename _T> iParameter* create_parameter  (const wchar_t* name);
protected:
private:
  std::vector <iParameter *> params;
};
}

#endif /* __SK__PARAMETER_H__ */