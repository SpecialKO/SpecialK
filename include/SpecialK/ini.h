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

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <set>

#include <Unknwnbase.h>

// {B526D074-2F4D-4BAE-B6EC-11CB3779B199}
static const GUID IID_SK_INISection =
{ 0xb526d074, 0x2f4d, 0x4bae, { 0xb6, 0xec, 0x11, 0xcb, 0x37, 0x79, 0xb1, 0x99 } };

interface iSK_INI;
interface iSK_INISection : public IUnknown
{
public:
  iSK_INISection (void) = default;

  iSK_INISection (const wchar_t* section_name) : name   (section_name != nullptr ?
                                                         section_name            : L""),
                                                 parent (nullptr) {
  }

  iSK_INISection (const std::wstring& sec_name) : name   (sec_name),
                                                  parent (nullptr) {
  }

  iSK_INISection ( const wchar_t* section_name,
                         iSK_INI* _parent ) : name   (section_name != nullptr ?
                                                      section_name            : L""),
                                              parent (_parent) { };

  iSK_INISection ( const std::wstring& sec_name,
                              iSK_INI* _parent ) : name   (sec_name),
                                                   parent (_parent) { };

  iSK_INISection            (const iSK_INISection&)           = default;
  iSK_INISection& operator= (const iSK_INISection&)           = default;
  iSK_INISection            (      iSK_INISection&&) noexcept = default;
  iSK_INISection& operator= (      iSK_INISection&&) noexcept = default;

  virtual ~iSK_INISection (void) noexcept = default;

  /*** IUnknown methods ***/
  STDMETHOD  (       QueryInterface)(THIS_ REFIID riid, void** ppvObj)                                  noexcept final;
  STDMETHOD_ (ULONG, AddRef)        (THIS)                                                              noexcept final;
  STDMETHOD_ (ULONG, Release)       (THIS)                                                              noexcept final;

  STDMETHOD_ (std::wstring&, get_value)    (const wchar_t* key)                                                  final;
  STDMETHOD_ (void,          set_name)     (const wchar_t* name_)                                                final;
  STDMETHOD_ (bool,          contains_key) (const wchar_t* key) const                                            final;
  STDMETHOD_ (void,          add_key_value)(const wchar_t* key, const wchar_t* value)                            final;
  STDMETHOD_ (bool,          remove_key)   (const wchar_t* key)                                                  final;


  // Private to DLL
  STDMETHOD_ (std::wstring&, get_value)    (const std::wstring& key)                                    noexcept final;
  STDMETHOD_ (void,          set_name)     (const std::wstring& name_)                                  noexcept final;
  STDMETHOD_ (const
              std::wstring*, contains_key) (const std::wstring& key) const                              noexcept final;
  STDMETHOD_ (void,          add_key_value)(const std::wstring& key, const std::wstring& value)         noexcept final;
  STDMETHOD_ (bool,          remove_key)   (const std::wstring& key)                                    noexcept final;
  STDMETHOD_ (const
              std::wstring&, get_cvalue)   (const std::wstring& key) const                              noexcept final;


  //protected:
  //private:
  std::wstring                                    name;
  std::unordered_map <std::wstring, std::wstring> keys;
  std::vector        <std::wstring>               ordered_keys;
  iSK_INI*                                        parent;

  ULONG                                           refs = 0;
};

// {DD2B1E00-6C14-4659-8B45-FCEF1BC2C724}
static const GUID IID_SK_INI =
{ 0xdd2b1e00, 0x6c14, 0x4659, { 0x8b, 0x45, 0xfc, 0xef, 0x1b, 0xc2, 0xc7, 0x24 } };

interface iSK_INI : public IUnknown
{
  friend interface iSK_INISection;

  using _TSectionMap =
    std::unordered_map <std::wstring, iSK_INISection>;

  enum CharacterEncoding {
    INI_INVALID   = 0x00,
    INI_UTF8      = 0x01,
    INI_UTF16LE   = 0x02,
    INI_UTF16BE   = 0x04, // Not natively supported, but can be converted
    INI_UTF8NOBOM = 0x08
  };

           iSK_INI (const wchar_t* filename);
  virtual ~iSK_INI (void) noexcept;

  iSK_INI            (const iSK_INI&)           = default;
  iSK_INI& operator= (const iSK_INI&)           = default;
  iSK_INI            (      iSK_INI&&) noexcept = default;
  iSK_INI& operator= (      iSK_INI&&) noexcept = default;

  /*** IUnknown methods ***/
  STDMETHOD  (                   QueryInterface)  (THIS_ REFIID riid, void** ppvObj)     noexcept final;
  STDMETHOD_ (ULONG,             AddRef)          (THIS)                                 noexcept final;
  STDMETHOD_ (ULONG,             Release)         (THIS)                                 noexcept final;

  STDMETHOD_ (void,              parse)           (THIS)                                          final;
  STDMETHOD_ (void,              import)          (THIS_ const wchar_t* import_data)              final;
  STDMETHOD_ (void,              write)           (THIS_ const wchar_t* fname = nullptr)          final;

  STDMETHOD_ (_TSectionMap&,     get_sections)    (THIS)                                 noexcept final;
  STDMETHOD_ (iSK_INISection&,   get_section)     (const wchar_t* section)                        final;
  STDMETHOD_ (bool,              contains_section)(const wchar_t* section)                        final;
  STDMETHOD_ (bool,              remove_section)  (const wchar_t* section)                        final;

  STDMETHOD_ (iSK_INISection&,   get_section_f)   ( THIS_ _In_z_ _Printf_format_string_
                                                    wchar_t const* const _Format,
                                                                       ... )                      final;
  STDMETHOD_ (const wchar_t*,    get_filename)    (THIS) const                           noexcept final;
  STDMETHOD_ (bool,              import_file)     (const wchar_t* fname)                          final;
  STDMETHOD_ (bool,              rename)          (const wchar_t* fname)                 noexcept final;
  STDMETHOD_ (bool,              reload)          (const wchar_t* fname = nullptr)       noexcept final;

  STDMETHOD_ (CharacterEncoding, get_encoding)    (void) const                           noexcept final;
  STDMETHOD_ (bool,              set_encoding)    (CharacterEncoding encoding)           noexcept final;


  // Private to DLL
  STDMETHOD_ (iSK_INISection&,   get_section)     (const std::wstring& section)          noexcept final;
  STDMETHOD_ (iSK_INISection*,   contains_section)(const std::wstring& section)          noexcept final;
  STDMETHOD_ (bool,              remove_section)  (const std::wstring& section)          noexcept final;

  STDMETHOD_ (bool&,             get_allow_empty) (void)                                 noexcept final;

protected:
  std::recursive_mutex  lock;
  std::recursive_mutex  section_lock;

private:
  FILE*     fINI    = nullptr;

  std::wstring          name;
  std::vector <wchar_t> data;
  int                   bom_size = 0;

  std::unordered_map <
    std::wstring, iSK_INISection
  >         sections;

  // Preserve insertion order so that we write the INI file in the
  //   same order we read it. Otherwise things get jumbled around
  //     arbitrarily as the map is re-hashed.
  std::vector <iSK_INISection *>
              ordered_sections;

  // Preserve File Encoding
  CharacterEncoding encoding_ = INI_INVALID;

  ULONG    refs_       =    0;
  uint32_t crc32_      =    0; // Skip writing config files that haven't changed
  FILETIME flushed_    =  { 0 };
  FILETIME file_stamp  =  { 0 };
  HANDLE   file_watch  = nullptr;
  bool     allow_empty =  false;
};

iSK_INI*
__stdcall
SK_CreateINI (const wchar_t* const wszName) noexcept;

#endif /* __SK__INI_H__ */