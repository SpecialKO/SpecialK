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
#ifndef __SK__INI_H__
#define __SK__INI_H__

#include <string>
#include <map>
#include <vector>

#include <Unknwnbase.h>

// {B526D074-2F4D-4BAE-B6EC-11CB3779B199}
static const GUID IID_SK_INISection = 
{ 0xb526d074, 0x2f4d, 0x4bae, { 0xb6, 0xec, 0x11, 0xcb, 0x37, 0x79, 0xb1, 0x99 } };

interface iSK_INISection : public IUnknown
{
public:
  iSK_INISection (void) {
  }

  iSK_INISection (const wchar_t* section_name) {
    name = section_name;
  }

  /*** IUnknown methods ***/
  STDMETHOD  (       QueryInterface)(THIS_ REFIID riid, void** ppvObj);
  STDMETHOD_ (ULONG, AddRef)        (THIS);
  STDMETHOD_ (ULONG, Release)       (THIS);

  STDMETHOD_ (std::wstring&, get_value)    (const wchar_t* key);
  STDMETHOD_ (void,          set_name)     (const wchar_t* name_);
  STDMETHOD_ (bool,          contains_key) (const wchar_t* key);
  STDMETHOD_ (void,          add_key_value)(const wchar_t* key, const wchar_t* value);
  STDMETHOD_ (bool,          remove_key)   (const wchar_t* key);

  //protected:
  //private:
  std::wstring                              name;
  std::map     <std::wstring, std::wstring> pairs;
  std::vector  <std::wstring>               ordered_keys;

  ULONG                                     refs;
};

// {DD2B1E00-6C14-4659-8B45-FCEF1BC2C724}
static const GUID IID_SK_INI = 
{ 0xdd2b1e00, 0x6c14, 0x4659, { 0x8b, 0x45, 0xfc, 0xef, 0x1b, 0xc2, 0xc7, 0x24 } };

interface iSK_INI : public IUnknown
{
  typedef const std::map <std::wstring, iSK_INISection> _TSectionMap;

   iSK_INI (const wchar_t* filename);
  ~iSK_INI (void);

  /*** IUnknown methods ***/
  STDMETHOD  (       QueryInterface)(THIS_ REFIID riid, void** ppvObj);
  STDMETHOD_ (ULONG, AddRef)        (THIS);
  STDMETHOD_ (ULONG, Release)       (THIS);

  STDMETHOD_ (void, parse)  (THIS);
  STDMETHOD_ (void, import) (THIS_ const wchar_t* import_data);
  STDMETHOD_ (void, write)  (THIS_ const wchar_t* fname);

  STDMETHOD_ (_TSectionMap&,   get_sections)    (THIS);
  STDMETHOD_ (iSK_INISection&, get_section)     (const wchar_t* section);
  STDMETHOD_ (bool,            contains_section)(const wchar_t* section);
  STDMETHOD_ (bool,            remove_section)  (const wchar_t* section);

  STDMETHOD_ (iSK_INISection&, get_section_f)   ( THIS_ _In_z_ _Printf_format_string_
                                                  wchar_t const* const _Format,
                                                                       ... );

protected:
private:
  FILE*     fINI;

  wchar_t*  wszName;
  wchar_t*  wszData;

  std::map <std::wstring, iSK_INISection>
            sections;

  // Preserve insertion order so that we write the INI file in the
  //   same order we read it. Otherwise things get jumbled around
  //     arbitrarily as the map is re-hashed.
  std::vector <std::wstring>
            ordered_sections;

  // Preserve File Encoding
  enum CharacterEncoding {
    INI_INVALID = 0x00,
    INI_UTF8    = 0x01,
    INI_UTF16LE = 0x02,
    INI_UTF16BE = 0x04 // Not natively supported, but can be converted
  } encoding_;

  ULONG     refs;
};

#endif /* __SK__INI_H__ */