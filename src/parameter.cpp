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

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#include <Windows.h>
#include <SpecialK/parameter.h>
#include <SpecialK/utility.h>

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


std::wstring
sk::ParameterBool::get_value_str (void)
{
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
  value = (float)wcstod (str.c_str (), NULL);
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


template <>
sk::iParameter*
sk::ParameterFactory::create_parameter <int> (const wchar_t* name)
{
  iParameter* param = new ParameterInt ();
  params.push_back (param);

  return param;
}

template <>
sk::iParameter*
sk::ParameterFactory::create_parameter <int64_t> (const wchar_t* name)
{
  iParameter* param = new ParameterInt64 ();
  params.push_back (param);

  return param;
}

template <>
sk::iParameter*
sk::ParameterFactory::create_parameter <bool> (const wchar_t* name)
{
  iParameter* param = new ParameterBool ();
  params.push_back (param);

  return param;
}

template <>
sk::iParameter*
sk::ParameterFactory::create_parameter <float> (const wchar_t* name)
{
  iParameter* param = new ParameterFloat ();
  params.push_back (param);

  return param;
}

template <>
sk::iParameter*
sk::ParameterFactory::create_parameter <std::wstring> (const wchar_t* name)
{
  iParameter* param = new ParameterStringW ();
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
      section.get_value (ini_key) = get_value_str ().c_str ();
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