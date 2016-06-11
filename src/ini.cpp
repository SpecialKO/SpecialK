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
dll_log.Log (L"[ SpecialK ] %ws", ErrorMessage ((z), #x, (y), __LINE__, __FUNCTION__, __FILE__)); }

sk::INI::File::File (wchar_t* filename)
{
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
      if (*wszData != L'[')
        ++wszData;

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

sk::INI::File::~File (void)
{
  if (wszName != nullptr) {
    delete [] wszName;
    wszName = nullptr;
  }

  if (wszData != nullptr) {
    delete[] wszData;
    wszData = nullptr;
  }
}

sk::INI::File::Section
Process_Section (wchar_t* buffer, wchar_t* name, int start, int end)
{
  sk::INI::File::Section section (name);

  int key = start;
  for (int k = key; k < end; k++) {
    if (k < end - 1 && buffer [k] == L'=') {
      wchar_t* key_str = new wchar_t [k - key + 1];
      wcsncpy (key_str, buffer + key, k - key);
      key_str [k - key] = L'\0';
      //if (! section.name.compare (L"IniVersion"))
      //MessageBoxW (NULL, key_str, L"Key", MB_OK);

      int value = k + 1;
      for (int l = value; l < end; l++) {
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
Import_Section (sk::INI::File::Section& section, wchar_t* buffer, int start, int end)
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
sk::INI::File::parse (void)
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

        Section section = Process_Section (wszData, sec_name, start, finish);

        sections.insert (std::pair <std::wstring, Section> (sec_name, section));
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
sk::INI::File::import (std::wstring import_data)
{
  wchar_t* wszImport = _wcsdup (import_data.c_str ());

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
        //MessageBoxW (NULL, sec_name, L"Section", MB_OK);

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
          Section& section = get_section (sec_name);

          Import_Section (section, wszImport, start, finish);
        }

        // Insert otherwise
        else {
          Section section = Process_Section (wszImport, sec_name, start, finish);

          sections.insert (std::pair <std::wstring, Section> (sec_name, section));
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
sk::INI::File::Section::get_value (std::wstring key)
{
  std::map <std::wstring, std::wstring>::iterator it_key = pairs.find (key);

  if (it_key != pairs.end ())
    return (*it_key).second;

  return invalid;
}

bool
sk::INI::File::Section::contains_key (std::wstring key)
{
  for ( std::map <std::wstring, std::wstring>::iterator it = pairs.begin ();
          it != pairs.end ();
            it++ ) {
    if ((*it).first == key)
      return true;
  }

  return false;
}

void
sk::INI::File::Section::add_key_value (std::wstring key, std::wstring value)
{
  pairs.insert (std::pair <std::wstring, std::wstring> (key, value));
  ordered_keys.push_back (key);
}

bool
sk::INI::File::contains_section (std::wstring section)
{
  return sections.count (section) > 0;
}

sk::INI::File::Section&
sk::INI::File::get_section (std::wstring section)
{
  if (! sections.count (section))
    ordered_sections.push_back (section);

  return sections [section];
}

void
sk::INI::File::write (std::wstring fname)
{
  FILE*   fOut;
  errno_t ret;

  // Strip Read-Only
  /////////AD_SetNormalFileAttribs (fname);

  TRY_FILE_IO (_wfopen_s (&fOut, fname.c_str (), L"w,ccs=UTF-16LE"), fname.c_str (), ret);

  if (ret != 0 || fOut == 0) {
    //AD_MessageBox (L"ERROR: Cannot open INI file for writing. Is it read-only?", fname.c_str (), MB_OK | MB_ICONSTOP);
    return;
  }

  std::vector <std::wstring>::iterator it  = ordered_sections.begin ();
  std::vector <std::wstring>::iterator end = ordered_sections.end   ();

  while (it != end) {
    Section& section = get_section (*it);
    if (section.name.length ()) {
      fwprintf (fOut, L"[%s]\n", section.name.c_str ());

      std::vector <std::wstring>::iterator key_it  = section.ordered_keys.begin ();
      std::vector <std::wstring>::iterator key_end = section.ordered_keys.end   ();

      while (key_it != key_end) {
        std::wstring val = section.get_value (*key_it);
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


const std::map <std::wstring, sk::INI::File::Section>&
sk::INI::File::get_sections (void)
{
  return sections;
}