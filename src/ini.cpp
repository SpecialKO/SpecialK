#if 0
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

#include <windows.h>

#include "ini.h"
#include "log.h"
#include <string>

std::wstring
ErrorMessage (errno_t        err,
              const char*    args,
              const wchar_t* ini_name,
              UINT           line_no,
              const char*    function_name,
              const char*    file_name)
{
  wchar_t wszFormattedError [1024];

  *wszFormattedError = L'\0';

  swprintf ( wszFormattedError, 1024,
             L"\n"
             L"Line %u of %hs (in %hs (...)):\n"
             L"------------------------\n\n"
             L"%hs\n\n  File: %s\n\n"
             L"\t>> %s <<",
               line_no,
                 file_name,
                   function_name,
                     args,
                       ini_name,
                         _wcserror (err) );

  return wszFormattedError;
}

#define TRY_FILE_IO(x,y,z) { (z) = ##x; if ((z) != 0) \
dll_log.Log (L"[ SpecialK ] %ws", ErrorMessage ((z), #x, (y), __LINE__, __FUNCTION__, __FILE__).c_str ()); }

iSK_INI::iSK_INI (const wchar_t* filename)
{
  AddRef ();

  sections.clear ();

  // We skip a few bytes (Unicode BOM) in crertain cirumstances, so this is the
  //   actual pointer we need to free...
  wchar_t* alloc;

  wszName = _wcsdup (filename);

  errno_t ret;
  TRY_FILE_IO (_wfopen_s (&fINI, filename, L"rb"), filename, ret);

  if (ret == 0 && fINI != 0) {
                fseek  (fINI, 0, SEEK_END);
    long size = ftell  (fINI);
                rewind (fINI);

    wszData = new wchar_t [size];
    alloc   = wszData;

    fread (wszData, size, 1, fINI);

    // This is essentially our Unicode BOM, if the first character is '[', then it's ANSI.
    if (((char *)wszData) [0] == '[') {
      char* string = new char [size + 1];
      memcpy (string, wszData, size);
      string [size] = '\0';
      delete [] wszData;
      wszData = new wchar_t [size + 1];
      MultiByteToWideChar (CP_OEMCP, 0, string, -1, wszData, size + 1);
      alloc = wszData;
    }

    // Otherwise it's Unicode, let's fix up a couple of things
    else {
      // UTF16-LE  (All is well in the world)
      if (*wszData == 0xFEFF) {
        ++wszData;
      }

      // UTF16-BE  (Somehow we are swapped)
      else if (*wszData == 0xFFFE) {
        dll_log.Log ( L"[INI Parser] Encountered Byte-Swapped Unicode INI "
                      L"file ('%s'), attempting to recover...",
                        wszName );

        wchar_t* wszSwapMe = wszData;

        for (int i = 0; i < size; i += 2) {
          *wszSwapMe++ = _byteswap_ushort (*wszSwapMe);
        }

        ++wszData;
      }

      // Something else, time to freak out!
      else {
        delete [] alloc;
        wszData = nullptr;

        fclose (fINI);

        dll_log.Log ( L"[INI Parser] Encountered unknown Unicode Byte-Order "
                      L"Marker in INI file ('%s'): 0x%04X!\n",
                        wszName, *wszData );

        return;
      }

      wszData [size / 2 - 1] = '\0';
    }

    parse ();

    delete [] alloc;
    wszData = nullptr;

    fflush (fINI);
    fclose (fINI);
  }
  else {
    //AD_MessageBox (L"Unable to Locate INI File", filename, MB_OK);
    delete [] wszName;
    wszName = nullptr;
    wszData = nullptr;
  }
}

iSK_INI::~iSK_INI (void)
{
  if (wszName != nullptr) {
    delete [] wszName;
    wszName = nullptr;
  }

  if (wszData != nullptr) {
    delete[] wszData;
    wszData = nullptr;
  }

  Release ();
}

iSK_INISection
Process_Section (wchar_t* buffer, wchar_t* name, int start, int end)
{
  iSK_INISection section (name);

  int key = start;
  for (int k = key; k < end; k++) {
    if (k < end - 1 && buffer [k] == L'=') {
      wchar_t* key_str = new wchar_t [k - key + 1];
      wcsncpy (key_str, buffer + key, k - key);
      key_str [k - key] = L'\0';

      int value = k + 1;
      for (int l = value; l <= end; l++) {
        if (l > end - 1 || buffer [l] == L'\n') {
          key = l + 1;
          k = l + 1;
          wchar_t* val_str = new wchar_t [l - value + 1];
          wcsncpy (val_str, buffer + value, l - value);
          val_str [l - value] = L'\0';

          section.add_key_value (key_str, val_str);

          delete [] val_str;
          l = end;
        }
      }

      delete [] key_str;
    }
  }

  return section;
}

bool
Import_Section (iSK_INISection& section, wchar_t* buffer, int start, int end)
{
  int key = start;
  for (int k = key; k < end; k++) {
    if (k < end - 1 && buffer [k] == L'=') {
      wchar_t* key_str = new wchar_t [k - key + 1];
      wcsncpy (key_str, buffer + key, k - key);
      key_str [k - key] = L'\0';

      int value = k + 1;
      for (int l = value; l < end; l++) {
        if (l > end - 1 || buffer [l] == L'\n') {
          key = l + 1;
          k = l + 1;
          wchar_t* val_str = new wchar_t [l - value + 1];
          wcsncpy (val_str, buffer + value, l - value);
          val_str [l - value] = L'\0';

          // Prefer to change an existing value
          if (section.contains_key (key_str)) {
            std::wstring& val = section.get_value (key_str);
            val = val_str;
          }

          // But create a new one if it doesn't already exist
          else {
            section.add_key_value (key_str, val_str);
          }

          delete [] val_str;
          l = end;
        }
      }

      delete [] key_str;
    }
  }

  return true;
}

void
__stdcall
iSK_INI::parse (void)
{
  if (wszData != nullptr) {
    int len = lstrlenW (wszData);

    // We don't want CrLf, just Lf
    bool strip_cr = false;

    // Find if the file has any Cr's
    for (int i = 0; i < len; i++) {
      if (wszData [i] == L'\r')
        strip_cr = true;
    }

    if (strip_cr) {
      // Remove all Cr's and then re-NUL terminate the truncated file
      int out = 0;
      for (int i = 0; i < len; i++) {
        if (wszData [i] != L'\r')
          wszData [out++] = wszData [i];
      }
      wszData [out] = L'\0';

      len = out;
    }

    int begin = -1;
    int end   = -1;

    for (int i = 0; i < len; i++)
    {
      if (wszData [i] == L'[' && (i == 0 || wszData [i - 1] == L'\n')) {
        begin = i + 1;
      }

      if (wszData [i] == L']' && (i == len - 1 || wszData [i + 1] == L'\n'))
        end = i;

      if (end != -1) {
        wchar_t* sec_name = new wchar_t [end - begin + 1];
        wcsncpy (sec_name, wszData + begin, end - begin);
        sec_name [end - begin] = L'\0';
        //MessageBoxW (NULL, sec_name, L"Section", MB_OK);

        int start  = end + 2;
        int finish = start;

        bool eof = false;
        for (int j = start; j <= len; j++) {
          if (j == len) {
            finish = j;
            eof = true;
            break;
          }

          if (wszData [j - 1] == L'\n' && wszData [j] == L'[') {
            finish = j - 1;
            break;
          }
        }

        iSK_INISection section = Process_Section (wszData, sec_name, start, finish);

        sections.insert (std::pair <std::wstring, iSK_INISection> (sec_name, section));
        ordered_sections.push_back (sec_name);
        delete [] sec_name;

        if (eof)
          break;

        i = finish;

        end   = -1;
        begin = -1;
      }
    }
  }
}

void
__stdcall
iSK_INI::import (const wchar_t* import_data)
{
  wchar_t* wszImport = _wcsdup (import_data);

  if (wszImport != nullptr) {
    int len = lstrlenW (wszImport);

    // We don't want CrLf, just Lf
    bool strip_cr = false;

    // Find if the file has any Cr's
    for (int i = 0; i < len; i++) {
      if (wszImport [i] == L'\r')
        strip_cr = true;
    }

    if (strip_cr) {
      // Remove all Cr's and then re-NUL terminate the truncated file
      int out = 0;
      for (int i = 0; i < len; i++) {
        if (wszImport [i] != L'\r')
          wszImport [out++] = wszImport [i];
      }
      wszImport [out] = L'\0';

      len = out;
    }

    int begin = -1;
    int end   = -1;

    for (int i = 0; i < len; i++)
    {
      if (wszImport [i] == L'[' && (i == 0 || wszImport [i - 1] == L'\n')) {
        begin = i + 1;
      }

      if (wszImport [i] == L']' && (i == len - 1 || wszImport [i + 1] == L'\n'))
        end = i;

      if (end != -1) {
        wchar_t* sec_name = new wchar_t [end - begin + 1];
        wcsncpy (sec_name, wszImport + begin, end - begin);
        sec_name [end - begin] = L'\0';

        int start = end + 2;
        int finish = start;

        bool eof = false;
        for (int j = start; j <= len; j++) {
          if (j == len) {
            finish = j;
            eof    = true;
            break;
          }

          if (wszImport [j - 1] == L'\n' && wszImport [j] == L'[') {
            finish = j - 1;
            break;
          }
        }

        // Import if the section already exists
        if (contains_section (sec_name)) {
          iSK_INISection& section = get_section (sec_name);

          Import_Section (section, wszImport, start, finish);
        }

        // Insert otherwise
        else {
          iSK_INISection section = Process_Section (wszImport, sec_name, start, finish);

          sections.insert (std::pair <std::wstring, iSK_INISection> (sec_name, section));
          ordered_sections.push_back (sec_name);
        }
        delete [] sec_name;

        if (eof)
          break;

        i = finish;

        end   = -1;
        begin = -1;
      }
    }
  }

  delete [] wszImport;
}

std::wstring invalid = L"Invalid";

std::wstring&
__stdcall
iSK_INISection::get_value (const wchar_t* key)
{
  std::map <std::wstring, std::wstring>::iterator it_key = pairs.find (key);

  if (it_key != pairs.end ())
    return (*it_key).second;

  return invalid;
}

void
__stdcall
iSK_INISection::set_name (const wchar_t* name_)
{
  name = name_;
}

bool
__stdcall
iSK_INISection::contains_key (const wchar_t* key)
{
  for ( std::map <std::wstring, std::wstring>::iterator it = pairs.begin ();
          it != pairs.end ();
            it++ ) {
    if ((*it).first == std::wstring (key))
      return true;
  }

  return false;
}

void
__stdcall
iSK_INISection::add_key_value (const wchar_t* key, const wchar_t* value)
{
  pairs.insert (std::pair <std::wstring, std::wstring> (key, value));
  ordered_keys.push_back (key);
}

bool
__stdcall
iSK_INI::contains_section (const wchar_t* section)
{
  return sections.count (section) > 0;
}

iSK_INISection&
__stdcall
iSK_INI::get_section (const wchar_t* section)
{
  if (! sections.count (section))
    ordered_sections.push_back (section);

  iSK_INISection& ret = sections [section];

  ret.name = section;

  return ret;
}

#include "utility.h"

void
__stdcall
iSK_INI::write (const wchar_t* fname)
{
  SK_CreateDirectories (fname);

  FILE*   fOut = nullptr;
  errno_t ret  = 0;

  // Strip Read-Only
  /////////AD_SetNormalFileAttribs (fname);

  TRY_FILE_IO (_wfopen_s (&fOut, fname, L"wtc,ccs=UTF-16LE"), fname, ret);
  //fOut = _wfsopen (fname, L"wtc,ccs=UTF-16LE", _SH_DENYNO);

  if (ret != 0 || fOut == 0) {
    SK_MessageBox (L"ERROR: Cannot open INI file for writing. Is it read-only?", fname, MB_OK | MB_ICONSTOP);
    return;
  }

  std::vector <std::wstring>::iterator it  = ordered_sections.begin ();
  std::vector <std::wstring>::iterator end = ordered_sections.end   ();

  while (it != end) {
    iSK_INISection& section = get_section ((*it).c_str ());

    if (section.name.length () && section.ordered_keys.size ()) {
      fwprintf (fOut, L"[%s]\n", section.name.c_str ());

      std::vector <std::wstring>::iterator key_it  = section.ordered_keys.begin ();
      std::vector <std::wstring>::iterator key_end = section.ordered_keys.end   ();

      while (key_it != key_end) {
        std::wstring val = section.get_value ((*key_it).c_str ());
        fwprintf (fOut, L"%s=%s\n", key_it->c_str (), val.c_str ());
        ++key_it;
      }

      // Append a newline for everything except the last line...
      if ((it + 1) != end)
        fwprintf (fOut, L"\n");
    }

    ++it;
  }

  fflush (fOut);
  fclose (fOut);

  // Make Read-Only
  ////SetFileAttributes (fname.c_str (), FILE_ATTRIBUTE_READONLY);
}


iSK_INI::_TSectionMap&
__stdcall
iSK_INI::get_sections (void)
{
  return sections;
}


HRESULT
__stdcall
iSK_INI::QueryInterface (THIS_ REFIID riid, void** ppvObj)
{
  if (IsEqualGUID (riid, IID_SK_INI)) {
    AddRef ();
    *ppvObj = this;
    return S_OK;
  }

  return E_NOTIMPL;
}

ULONG
__stdcall
iSK_INI::AddRef (THIS)
{
  return InterlockedIncrement (&refs);
}

ULONG
__stdcall
iSK_INI::Release (THIS)
{
  return InterlockedDecrement (&refs);
}

bool
__stdcall
iSK_INI::remove_section (const wchar_t* wszSection)
{
  for ( auto it  = ordered_sections.begin ();
             it != ordered_sections.end   ();
           ++it )
  {
    if (*it == wszSection) {
      ordered_sections.erase (it);
      sections.erase         (wszSection);

      return true;
    }
  }

  return false;
}

bool
__stdcall
iSK_INISection::remove_key (const wchar_t* wszKey)
{
  for ( auto it  = ordered_keys.begin ();
             it != ordered_keys.end   ();
           ++it )
  {
    if (*it == wszKey) {
      ordered_keys.erase (it);
      pairs.erase        (wszKey);

      return true;
    }
  }

  return false;
}


HRESULT
__stdcall
iSK_INISection::QueryInterface (THIS_ REFIID riid, void** ppvObj)
{
  if (IsEqualGUID (riid, IID_SK_INISection)) {
    AddRef ();
    *ppvObj = this;
    return S_OK;
  }

  return E_NOTIMPL;
}

ULONG
__stdcall
iSK_INISection::AddRef (THIS)
{
  return InterlockedIncrement (&refs);
}

ULONG
__stdcall
iSK_INISection::Release (THIS)
{
  return InterlockedDecrement (&refs);
}

iSK_INI*
__stdcall
SK_CreateINI (const wchar_t* const wszName)
{
  iSK_INI* pINI = new iSK_INI (wszName);

  return pINI;
}

#else
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

#include <windows.h>

#include "ini.h"
#include "log.h"
#include <string>

std::wstring
ErrorMessage (errno_t        err,
              const char*    args,
              const wchar_t* ini_name,
              UINT           line_no,
              const char*    function_name,
              const char*    file_name)
{
  wchar_t wszFormattedError [1024];

  *wszFormattedError = L'\0';

  swprintf ( wszFormattedError, 1024,
             L"\n"
             L"Line %u of %hs (in %hs (...)):\n"
             L"------------------------\n\n"
             L"%hs\n\n  File: %s\n\n"
             L"\t>> %s <<",
               line_no,
                 file_name,
                   function_name,
                     args,
                       ini_name,
                         _wcserror (err) );

  return wszFormattedError;
}

#define TRY_FILE_IO(x,y,z) { (z) = ##x; if ((z) != 0) \
dll_log.Log (L"[ SpecialK ] %ws", ErrorMessage ((z), #x, (y), __LINE__, __FUNCTION__, __FILE__).c_str ()); }

iSK_INI::iSK_INI (const wchar_t* filename)
{
  AddRef ();

  sections.clear ();

  // We skip a few bytes (Unicode BOM) in crertain cirumstances, so this is the
  //   actual pointer we need to free...
  wchar_t* alloc;

  wszName = _wcsdup (filename);

  errno_t ret;
  TRY_FILE_IO (_wfopen_s (&fINI, filename, L"rb"), filename, ret);

  if (ret == 0 && fINI != 0) {
                fseek  (fINI, 0, SEEK_END);
    long size = ftell  (fINI);
                rewind (fINI);

    wszData = new wchar_t [size + 1];
    alloc   = wszData;

    ZeroMemory (wszData, sizeof (wchar_t) * size + 1);

    fread (wszData, size, 1, fINI);

    bool unicode = true;

    // First, consider Unicode
    // UTF16-LE  (All is well in the world)
    if (*wszData == 0xFEFF) {
      ++wszData;
    }

    // UTF16-BE  (Somehow we are swapped)
    else if (*wszData == 0xFFFE) {
      dll_log.Log ( L"[INI Parser] Encountered Byte-Swapped Unicode INI "
                    L"file ('%s'), attempting to recover...",
                      wszName );

      wchar_t* wszSwapMe = wszData;

      for (int i = 0; i < size; i += 2) {
        *wszSwapMe++ = _byteswap_ushort (*wszSwapMe);
      }

      ++wszData;
    }

    // Something else, if it's ANSI or UTF-8, let's hope Windows can figure
    //   out what to do...
    else {
      // Skip the silly UTF8 BOM if it is present
      bool utf8 = ((unsigned char *)wszData) [0] == 0xEF &&
                  ((unsigned char *)wszData) [1] == 0xBB &&
                  ((unsigned char *)wszData) [2] == 0xBF;

      const uintptr_t offset =
        utf8 ? 3 : 0;

      const int       real_size =
        size - offset;

      char* start_addr =
        ((char *)wszData) + offset;

      char* string =
        new char [real_size];

      memcpy (string, start_addr, real_size);

      delete [] wszData;

      int converted_size =
        MultiByteToWideChar ( CP_UTF8, 0, string, real_size, nullptr, 0 );

      if (! converted_size) {
        dll_log.Log ( L"[INI Parser] Could not convert UTF-8 / ANSI Encoded "
                      L".ini file ('%s') to UTF-16, aborting!",
                        wszName );
        wszData = nullptr;

        fclose (fINI);
        return;
      }

      wszData =
        new wchar_t [converted_size + 1];

      MultiByteToWideChar ( CP_UTF8, 0, string, real_size, wszData, converted_size );

      dll_log.Log ( L"[INI Parser] Converted UTF-8 INI File: '%s'",
                      wszName );

      wszData [converted_size] = L'\0';

      delete [] string;

      // No Byte-Order Marker
      alloc = wszData;
    }

    parse ();

    // TODO:  Should we keep data in unparsed format?
    delete [] alloc;
    wszData = nullptr;

    fflush (fINI);
    fclose (fINI);
  }
  else {
    //AD_MessageBox (L"Unable to Locate INI File", filename, MB_OK);
    delete [] wszName;
    wszName = nullptr;
    wszData = nullptr;
  }
}

iSK_INI::~iSK_INI (void)
{
  if (wszName != nullptr) {
    delete [] wszName;
    wszName = nullptr;
  }

  if (wszData != nullptr) {
    delete[] wszData;
    wszData = nullptr;
  }

  Release ();
}

auto wcrlen =
  [](wchar_t *_start, wchar_t *_end) ->
    size_t
    {
      size_t   _len = 0;

      wchar_t* _it  = _start;
      while (_it < _end) {
        _it = CharNextW (_it);
        ++_len;
      }

      return _len;
    };

iSK_INISection
Process_Section (wchar_t* name, wchar_t* start, wchar_t* end)
{
  iSK_INISection section (name);

  const wchar_t* penultimate = CharPrevW (start, end);
        wchar_t* key         = start;

  for (wchar_t* k = key; k < end; k = CharNextW (k)) {
    if (k < penultimate && *k == L'=') {

      wchar_t*    key_str = new    wchar_t    [k - key + 1];
      ZeroMemory (key_str, sizeof (wchar_t) * (k - key + 1));

      size_t   key_len =          wcrlen (key, k);
      wcsncpy (key_str, key, key_len);

      wchar_t* value = CharNextW (k);

      for (wchar_t* l = value; l <= end; l = CharNextW (l)) {
        if (l > penultimate || *l == L'\n') {
          key = CharNextW (l);
            k = key;

             wchar_t* val_str = new    wchar_t [   l - value + 1];
          ZeroMemory (val_str, sizeof (wchar_t) * (l - value + 1));

          size_t   val_len = wcrlen          (value, l);
          wcsncpy (val_str, value, val_len);

          //MessageBoxW (NULL, key_str, val_str, MB_OK);

          section.add_key_value (key_str, val_str);

          delete [] val_str;
          l = end;
        }
      }

      delete [] key_str;
    }
  }

  return section;
}

bool
Import_Section (iSK_INISection& section, wchar_t* start, wchar_t* end)
{
  const wchar_t* penultimate = CharPrevW (start, end);
        wchar_t* key         = start;

  for (wchar_t* k = key; k < end; k = CharNextW (k)) {
    if (k < penultimate && *k == L'=') {

      wchar_t*    key_str = new    wchar_t    [k - key + 1];
      ZeroMemory (key_str, sizeof (wchar_t) * (k - key + 1));

      size_t   key_len =          wcrlen (key, k);
      wcsncpy (key_str, key, key_len);

      wchar_t* value = CharNextW (k);

      for (wchar_t* l = value; l <= end; l = CharNextW (l)) {
        if (l > penultimate || *l == L'\n') {
          key = CharNextW (l);
            k = key;

             wchar_t* val_str = new    wchar_t [   l - value + 1];
          ZeroMemory (val_str, sizeof (wchar_t) * (l - value + 1));

          size_t   val_len = wcrlen          (value, l);
          wcsncpy (val_str, value, val_len);

          // Prefer to change an existing value
          if (section.contains_key (key_str)) {
            std::wstring& val = section.get_value (key_str);
            val = val_str;
          }

          // But create a new one if it doesn't already exist
          else {
            section.add_key_value (key_str, val_str);
          }

          delete [] val_str;
          l = end;
        }
      }

      delete [] key_str;
    }
  }

  return true;
}

void
__stdcall
iSK_INI::parse (void)
{
  if (wszData != nullptr) {
    int len = lstrlenW (wszData);

    // We don't want CrLf, just Lf
    bool strip_cr = false;

    wchar_t* wszStrip = &wszData [0];

    // Find if the file has any Cr's
    for (int i = 0; i < len; i++) {
      if (*wszStrip == L'\r') {
        strip_cr = true;
        break;
      }

      wszStrip = CharNextW (wszStrip);
    }

    wchar_t* wszDataEnd = &wszData [0];

    if (strip_cr) {
      wchar_t* wszDataNext = &wszData [0];

      // Remove all Cr's and then re-NUL terminate the truncated file
      int out = 0;

      for (int i = 0; i < len; i++) {
        if (*wszDataNext != L'\r') {
           ++out;
          *wszDataEnd = *wszDataNext;
           wszDataEnd = CharNextW (wszDataEnd);
        }

        wszDataNext = CharNextW (wszDataNext);
      }

      const wchar_t* wszNext =
        CharNextW (wszDataNext);

      memset ( wszDataEnd,
                 0x00,
                   (uintptr_t)wszNext - (uintptr_t)wszDataEnd );

      len = lstrlenW (wszData);
    }

    else {
      for (int i = 0; i < len; i++) {
        wszDataEnd = CharNextW (wszDataEnd);
      }
    }

    wchar_t* wszSecondToLast = CharPrevW (wszData, wszDataEnd);

    wchar_t* begin = nullptr;
    wchar_t* end   = nullptr;

    wchar_t* wszDataCur  = &wszData [0];
    wchar_t* wszDataNext = CharNextW (wszDataCur);

    for (wchar_t* i = wszDataCur; i < wszDataEnd; i = CharNextW (i))
    {
      if (*i == L'[' && (i == wszData || *CharPrevW (&wszData [0], i) == L'\n')) {
        begin = CharNextW (i);
      }

      if (*i == L']' && (i == wszSecondToLast || *CharNextW (i) == L'\n'))
        end = i;

      if (begin != nullptr && end != nullptr) {
           wchar_t* sec_name =    new wchar_t    [end - begin + 1];
        ZeroMemory (sec_name, sizeof (wchar_t) * (end - begin + 1));

        size_t   sec_len = wcrlen (begin, end);
        wcsncpy (sec_name,         begin, sec_len);

        //MessageBoxW (NULL, sec_name, L"Section", MB_OK);

        wchar_t* start  = CharNextW (CharNextW (end));
        wchar_t* finish = start;

        bool eof = false;
        for (wchar_t* j = start; j <= wszDataEnd; j = CharNextW (j)) {
          if (j == wszDataEnd) {
            finish = j;
            eof    = true;
            break;
          }

          wchar_t *wszPrev = nullptr;

          if (*j == L'[' && (*(wszPrev = CharPrevW (start, j)) == L'\n')) {
            finish = wszPrev;
            break;
          }
        }

        iSK_INISection section =
          Process_Section (sec_name, start, CharPrevW (start, finish));

        sections.insert (
          std::pair <std::wstring, iSK_INISection> (
            sec_name, section
          )
        );

        ordered_sections.push_back (sec_name);

        delete [] sec_name;

        if (eof)
          break;

        i = finish;

        end   = nullptr;
        begin = nullptr;
      }
    }
  }
}

void
__stdcall
iSK_INI::import (const wchar_t* import_data)
{
  wchar_t* wszImport = _wcsdup (import_data);

  if (wszImport != nullptr) {
    int len = lstrlenW (wszImport);

    // We don't want CrLf, just Lf
    bool strip_cr = false;

    wchar_t* wszStrip = &wszImport [0];

    // Find if the file has any Cr's
    for (int i = 0; i < len; i++) {
      if (*wszStrip == L'\r') {
        strip_cr = true;
        break;
      }

      wszStrip = CharNextW (wszStrip);
    }

    wchar_t* wszImportEnd = &wszImport [0];

    if (strip_cr) {
      wchar_t* wszImportNext = &wszImport [0];

      // Remove all Cr's and then re-NUL terminate the truncated file
      int out = 0;

      for (int i = 0; i < len; i++) {
        if (*wszImportNext != L'\r') {
           ++out;
          *wszImportEnd = *wszImportNext;
           wszImportEnd = CharNextW (wszImportEnd);
        }

        wszImportNext = CharNextW (wszImportNext);
      }

      const wchar_t* wszNext =
        CharNextW (wszImportNext);

      memset ( wszImportEnd,
                 0x00,
                   (uintptr_t)wszNext - (uintptr_t)wszImportEnd );

      len = lstrlenW (wszImport);
    }

    else {
      for (int i = 0; i < (len - 1); i++) {
        wszImportEnd = CharNextW (wszImportEnd);
      }
    }

    wchar_t* wszSecondToLast = CharPrevW (wszImport, wszImportEnd);

    wchar_t* begin = nullptr;
    wchar_t* end   = nullptr;

    wchar_t* wszImportCur  = &wszImport [0];
    wchar_t* wszImportNext = CharNextW (wszImportCur);

    for (wchar_t* i = wszImportCur; i < wszImportEnd; i = CharNextW (i))
    {
      if (*i == L'[' && (i == wszImport || *CharPrevW (&wszImport [0], i) == L'\n')) {
        begin = CharNextW (i);
      }

      if (*i == L']' && (i == wszSecondToLast || *CharNextW (i) == L'\n'))
        end = i;

      if (begin != nullptr && end != nullptr) {
           wchar_t* sec_name =    new wchar_t    [end - begin + 1];
        ZeroMemory (sec_name, sizeof (wchar_t) * (end - begin + 1));

        size_t   sec_len = wcrlen (begin, end);
        wcsncpy (sec_name,         begin, sec_len);

        //MessageBoxW (NULL, sec_name, L"Section", MB_OK);

        wchar_t* start  = CharNextW (CharNextW (end));
        wchar_t* finish = start;

        bool eof = false;
        for (wchar_t* j = start; j <= wszImportEnd; j = CharNextW (j)) {
          if (j == wszImportEnd) {
            finish = j;
            eof    = true;
            break;
          }

          wchar_t *wszPrev = nullptr;

          if (*j == L'[' && (*(wszPrev = CharPrevW (start, j)) == L'\n')) {
            finish = wszPrev;
            break;
          }
        }

        // Import if the section already exists
        if (contains_section (sec_name)) {
          iSK_INISection& section = get_section (sec_name);

          Import_Section (section, start, finish);
        }

        // Insert otherwise
        else {
          iSK_INISection section =
            Process_Section (sec_name, start, CharPrevW (start, finish));

          sections.insert (
            std::pair <std::wstring, iSK_INISection> (
              sec_name, section
            )
          );

          ordered_sections.push_back (sec_name);
        }
        delete [] sec_name;

        if (eof)
          break;

        i = finish;

        end   = nullptr;
        begin = nullptr;
      }
    }
  }

  delete [] wszImport;
}

std::wstring invalid = L"Invalid";

std::wstring&
__stdcall
iSK_INISection::get_value (const wchar_t* key)
{
  std::map <std::wstring, std::wstring>::iterator it_key = pairs.find (key);

  if (it_key != pairs.end ())
    return (*it_key).second;

  return invalid;
}

void
__stdcall
iSK_INISection::set_name (const wchar_t* name_)
{
  name = name_;
}

bool
__stdcall
iSK_INISection::contains_key (const wchar_t* key)
{
  for ( std::map <std::wstring, std::wstring>::iterator it = pairs.begin ();
          it != pairs.end ();
            it++ ) {
    if ((*it).first == std::wstring (key))
      return true;
  }

  return false;
}

void
__stdcall
iSK_INISection::add_key_value (const wchar_t* key, const wchar_t* value)
{
  pairs.insert (std::pair <std::wstring, std::wstring> (key, value));
  ordered_keys.push_back (key);
}

bool
__stdcall
iSK_INI::contains_section (const wchar_t* section)
{
  return sections.count (section) > 0;
}

iSK_INISection&
__stdcall
iSK_INI::get_section (const wchar_t* section)
{
  if (! sections.count (section))
    ordered_sections.push_back (section);

  iSK_INISection& ret = sections [section];

  ret.name = section;

  return ret;
}

#include "utility.h"

void
__stdcall
iSK_INI::write (const wchar_t* fname)
{
  SK_CreateDirectories (fname);

  FILE*   fOut = nullptr;
  errno_t ret  = 0;

  // Strip Read-Only
  /////////AD_SetNormalFileAttribs (fname);

  TRY_FILE_IO (_wfopen_s (&fOut, fname, L"wtc,ccs=UTF-16LE"), fname, ret);
  //fOut = _wfsopen (fname, L"wtc,ccs=UTF-16LE", _SH_DENYNO);

  if (ret != 0 || fOut == 0) {
    SK_MessageBox (L"ERROR: Cannot open INI file for writing. Is it read-only?", fname, MB_OK | MB_ICONSTOP);
    return;
  }

  std::vector <std::wstring>::iterator it  = ordered_sections.begin ();
  std::vector <std::wstring>::iterator end = ordered_sections.end   ();

  // Strip Empty Sections
  // --------------------
  //  *** These would cause blank lines to be appended to the end of the INI file
  //        if we did not do something about them here and now. ***
  //
  while (it != end) {
    iSK_INISection& section = get_section ((*it).c_str ());

    if (! section.ordered_keys.size ()) {
      remove_section (section.name.c_str ());

      it  = ordered_sections.begin ();
      end = ordered_sections.end   ();

      continue;
    }

    ++it;
  }

  it  = ordered_sections.begin ();
  end = ordered_sections.end   ();

  while (it != end) {
    iSK_INISection& section = get_section ((*it).c_str ());

    if (section.name.length () && section.ordered_keys.size ()) {
      fwprintf (fOut, L"[%s]\n", section.name.c_str ());

      std::vector <std::wstring>::iterator key_it  = section.ordered_keys.begin ();
      std::vector <std::wstring>::iterator key_end = section.ordered_keys.end   ();

      while (key_it != key_end) {
        std::wstring val = section.get_value ((*key_it).c_str ());
        fwprintf (fOut, L"%s=%s", key_it->c_str (), val.c_str ());
        ++key_it;

        // Append a newline for everything except the last key...
        if (key_it != key_end)
          fwprintf (fOut, L"\n");
      }

      // Append a newline for everything except the last line...
      if ((it + 1) != end)
        fwprintf (fOut, L"\n\n");
    }

    ++it;
  }

  fflush (fOut);
  fclose (fOut);

  // Make Read-Only
  ////SetFileAttributes (fname.c_str (), FILE_ATTRIBUTE_READONLY);
}


iSK_INI::_TSectionMap&
__stdcall
iSK_INI::get_sections (void)
{
  return sections;
}


HRESULT
__stdcall
iSK_INI::QueryInterface (THIS_ REFIID riid, void** ppvObj)
{
  if (IsEqualGUID (riid, IID_SK_INI)) {
    AddRef ();
    *ppvObj = this;
    return S_OK;
  }

  return E_NOTIMPL;
}

ULONG
__stdcall
iSK_INI::AddRef (THIS)
{
  return InterlockedIncrement (&refs);
}

ULONG
__stdcall
iSK_INI::Release (THIS)
{
  return InterlockedDecrement (&refs);
}

bool
__stdcall
iSK_INI::remove_section (const wchar_t* wszSection)
{
  for ( auto it  = ordered_sections.begin ();
             it != ordered_sections.end   ();
           ++it )
  {
    if (*it == wszSection) {
      ordered_sections.erase (it);
      sections.erase         (wszSection);

      return true;
    }
  }

  return false;
}

bool
__stdcall
iSK_INISection::remove_key (const wchar_t* wszKey)
{
  for ( auto it  = ordered_keys.begin ();
             it != ordered_keys.end   ();
           ++it )
  {
    if (*it == wszKey) {
      ordered_keys.erase (it);
      pairs.erase        (wszKey);

      return true;
    }
  }

  return false;
}


HRESULT
__stdcall
iSK_INISection::QueryInterface (THIS_ REFIID riid, void** ppvObj)
{
  if (IsEqualGUID (riid, IID_SK_INISection)) {
    AddRef ();
    *ppvObj = this;
    return S_OK;
  }

  return E_NOTIMPL;
}

ULONG
__stdcall
iSK_INISection::AddRef (THIS)
{
  return InterlockedIncrement (&refs);
}

ULONG
__stdcall
iSK_INISection::Release (THIS)
{
  return InterlockedDecrement (&refs);
}

iSK_INI*
__stdcall
SK_CreateINI (const wchar_t* const wszName)
{
  iSK_INI* pINI = new iSK_INI (wszName);

  return pINI;
}
#endif