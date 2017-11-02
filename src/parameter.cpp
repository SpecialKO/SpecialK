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

#include <Windows.h>

#include <SpecialK/parameter.h>
#include <SpecialK/utility.h>
#include <SpecialK/ini.h>

// Read value from INI
bool
sk::iParameter::load (void)
{
  if (ini != nullptr)
  {
    iSK_INISection& section =
      ini->get_section (ini_section.c_str ());

    if (section.contains_key (ini_key.c_str ()))
    {
      set_value_str (section.get_value (ini_key.c_str ()));

      return true;
    }
  }

  return false;
}

bool
sk::iParameter::store (void)
{
  bool ret = false;

  if (ini != nullptr)
  {
    iSK_INISection& section =
      ini->get_section (ini_section.c_str ());

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


std::wstring
sk::ParameterInt::get_value_str (void)
{
  wchar_t str [32];
  _itow (value, str, 10);

  return std::wstring (str);
}

int
sk::ParameterInt::get_value (void)
{
  return value;
}

void
sk::ParameterInt::set_value (int val)
{
  value = val;
}


void
sk::ParameterInt::set_value_str (std::wstring str)
{
  value = _wtoi (str.c_str ());
}


void
sk::ParameterInt::store (int val)
{
  set_value      (val);
  iParameter::store ();
}

void
sk::ParameterInt::store_str (std::wstring str)
{
  set_value_str  (str);
  iParameter::store ();
}

bool
sk::ParameterInt::load (int& ref)
{
  bool bRet = 
    iParameter::load ();

  if (bRet)
    ref = get_value ();

  return bRet;
}


std::wstring
sk::ParameterInt64::get_value_str (void)
{
  wchar_t str [32];
  _i64tow (value, str, 10);

  return std::wstring (str);
}

int64_t
sk::ParameterInt64::get_value (void)
{
  return value;
}

void
sk::ParameterInt64::set_value (int64_t val)
{
  value = val;
}


void
sk::ParameterInt64::set_value_str (std::wstring str)
{
  value = _wtoll (str.c_str ());
}


void
sk::ParameterInt64::store (int64_t val)
{
  set_value      (val);
  iParameter::store ();
}

void
sk::ParameterInt64::store_str (std::wstring str)
{
  set_value_str  (str);
  iParameter::store ();
}

bool
sk::ParameterInt64::load (int64_t& ref)
{
  bool bRet = 
    iParameter::load ();

  if (bRet)
    ref = get_value ();

  return bRet;
}


std::wstring
sk::ParameterBool::get_value_str (void)
{
#ifdef CAP_FIRST_LETTER
  switch (type) {
    case ZeroNonZero:
      return value  ?  L"1"    : L"0";
    case YesNo:
      return value  ?  L"Yes"  : L"No";
    case OnOff:
      return value  ?  L"On"   : L"Off";
    case TrueFalse:
    default:
      return value  ?  L"True" : L"False";
  }
#else
  // Traditional behavior, more soothing ont he eyes ;)
  switch (type) {
    case ZeroNonZero:
      return value  ?  L"1"    : L"0";
    case YesNo:
      return value  ?  L"yes"  : L"no";
    case OnOff:
      return value  ?  L"on"   : L"off";
    case TrueFalse:
    default:
      return value  ?  L"true" : L"false";
  }
#endif
}

bool
sk::ParameterBool::get_value (void)
{
  return value;
}

void
sk::ParameterBool::set_value (bool val)
{
  value = val;
}


void
sk::ParameterBool::set_value_str (std::wstring str)
{
  size_t len = str.length ();

  type = TrueFalse;

  switch (len)
  {
    case 1:
      type = ZeroNonZero;

      if (str [0] == L'1')
        value = true;
      break;

    case 2:
      if ( towlower (str [0]) == L'o' &&
           towlower (str [1]) == L'n' ) {
        type  = OnOff;
        value = true;
      } else if ( towlower (str [0]) == L'n' &&
                  towlower (str [1]) == L'o' ) {
        type  = YesNo;
        value = false;
      }
      break;

    case 3:
      if ( towlower (str [0]) == L'y' &&
           towlower (str [1]) == L'e' &&
           towlower (str [2]) == L's' ) {
        type  = YesNo;
        value = true;
      } else if ( towlower (str [0]) == L'o' &&
                  towlower (str [1]) == L'f' &&
                  towlower (str [2]) == L'f' ) {
        type  = OnOff;
        value = false;
      }
      break;

    case 4:
      if ( towlower (str [0]) == L't' &&
           towlower (str [1]) == L'r' &&
           towlower (str [2]) == L'u' &&
           towlower (str [3]) == L'e' )
        value = true;
      break;

    default:
      value = false;
      break;
  }
}

void
sk::ParameterBool::store (bool val)
{
  set_value      (val);
  iParameter::store ();
}

void
sk::ParameterBool::store_str (std::wstring str)
{
  set_value_str  (str);
  iParameter::store ();
}

bool
sk::ParameterBool::load (bool& ref)
{
  bool bRet = 
    iParameter::load ();

  if (bRet)
    ref = get_value ();

  return bRet;
}



std::wstring
sk::ParameterFloat::get_value_str (void)
{
  wchar_t val_str [16];
  swprintf (val_str, L"%f", value);

  SK_RemoveTrailingDecimalZeros (val_str);

  return std::wstring (val_str);
}

float
sk::ParameterFloat::get_value (void)
{
  return value;
}

void
sk::ParameterFloat::set_value (float val)
{
  value = val;
}


void
sk::ParameterFloat::set_value_str (std::wstring str)
{
  value = (float)wcstod (str.c_str (), nullptr);
}


void
sk::ParameterFloat::store (float val)
{
  set_value      (val);
  iParameter::store ();
}

void
sk::ParameterFloat::store_str (std::wstring str)
{
  set_value_str  (str);
  iParameter::store ();
}

bool
sk::ParameterFloat::load (float& ref)
{
  bool bRet = 
    iParameter::load ();

  if (bRet)
    ref = get_value ();

  return bRet;
}


std::wstring
sk::ParameterStringW::get_value_str (void)
{
  return value;
}

std::wstring
sk::ParameterStringW::get_value (void)
{
  return value;
}

void
sk::ParameterStringW::set_value (std::wstring val)
{
  value = val;
}


void
sk::ParameterStringW::set_value_str (std::wstring str)
{
  value = str;
}

void
sk::ParameterStringW::store (std::wstring val)
{
  set_value      (val);
  iParameter::store ();
}

void
sk::ParameterStringW::store_str (std::wstring str)
{
  set_value_str  (str);
  iParameter::store ();
}

bool
sk::ParameterStringW::load (std::wstring& ref)
{
  bool bRet = 
    iParameter::load ();

  if (bRet)
    ref = get_value ();

  return bRet;
}



std::wstring
sk::ParameterVec2f::get_value_str (void)
{
  wchar_t x_str [16] = { };
  wchar_t y_str [16] = { };

  swprintf (x_str, L"%f", value.x);
  swprintf (y_str, L"%f", value.y);

  SK_RemoveTrailingDecimalZeros (x_str);
  SK_RemoveTrailingDecimalZeros (y_str);

  return SK_FormatStringW (L"(%s,%s)", x_str, y_str);
}

ImVec2
sk::ParameterVec2f::get_value (void)
{
  return value;
}

void
sk::ParameterVec2f::set_value (ImVec2 val)
{
  value = val;
}

void
sk::ParameterVec2f::set_value_str (std::wstring str)
{
  swscanf (str.c_str (), L"(%f,%f)", &value.x, &value.y);
}

void
sk::ParameterVec2f::store (ImVec2 val)
{
  set_value      (val);
  iParameter::store ();
}

void
sk::ParameterVec2f::store_str (std::wstring str)
{
  set_value_str  (str);
  iParameter::store ();
}

bool
sk::ParameterVec2f::load (ImVec2& ref)
{
  bool bRet = 
    iParameter::load ();

  if (bRet)
    ref = get_value ();

  return bRet;
}


template <>
sk::iParameter*
sk::ParameterFactory::create_parameter <int> (const wchar_t* name)
{
  UNREFERENCED_PARAMETER (name);

  iParameter* param = new ParameterInt ();
  params.push_back (param);

  return param;
}

#if 0
template<>
sk::iParameter*
sk::ParameterFactory::create_parameter (const wchar_t* name)
{
  UNREFERENCED_PARAMETER (name);

  return nullptr;
}
#endif

template <>
sk::iParameter*
sk::ParameterFactory::create_parameter <int64_t> (const wchar_t* name)
{
  UNREFERENCED_PARAMETER (name);

  iParameter* param = new ParameterInt64 ();
  params.push_back (param);

  return param;
}

template <>
sk::iParameter*
sk::ParameterFactory::create_parameter <bool> (const wchar_t* name)
{
  UNREFERENCED_PARAMETER (name);

  iParameter* param = new ParameterBool ();
  params.push_back (param);

  return param;
}

template <>
sk::iParameter*
sk::ParameterFactory::create_parameter <float> (const wchar_t* name)
{
  UNREFERENCED_PARAMETER (name);

  iParameter* param = new ParameterFloat ();
  params.push_back (param);

  return param;
}

template <>
sk::iParameter*
sk::ParameterFactory::create_parameter <std::wstring> (const wchar_t* name)
{
  UNREFERENCED_PARAMETER (name);

  iParameter* param = new ParameterStringW ();
  params.push_back (param);

  return param;
}

template <>
sk::iParameter*
sk::ParameterFactory::create_parameter <ImVec2> (const wchar_t* name)
{
  UNREFERENCED_PARAMETER (name);

  iParameter* param = new ParameterVec2f ();
  params.push_back (param);

  return param;
}


#if 0
bool
STDMETHODCALLTYPE
iSK_Parameter::load (void)
{
  if (ini != nullptr) {
    iSK_INISection& section = ini->get_section (ini_section);

    if (section.contains_key (ini_key)) {
      set_value_str (section.get_value (ini_key));
      return true;
    }
  }

  return false;
}

bool
STDMETHODCALLTYPE
iSK_Parameter::store (void)
{
  bool ret = false;

  if (ini != nullptr) {
    iSK_INISection& section = ini->get_section (ini_section);

    // If this operation actually creates a section, we need to make sure
    //   that section has a name!
    section.set_name (ini_section);

    if (section.contains_key (ini_key)) {
      section.get_value (ini_key) = get_value_str ();
      ret = true;
    }

    // Add this key/value if it doesn't already exist.
    else {
      section.add_key_value (ini_key, get_value_str ().c_str ());
      ret = true;// +1;
    }
  }

  return ret;
}
#endif

#if 0
void
STDMETHODCALLTYPE
iSK_ParameterBase::register_to_ini ( iSK_INI      *file,
                                     std::wstring  section,
                                     std::wstring  key )
{
  ini         = file;
  ini_section = section;
  ini_key     = key;
}
#endif