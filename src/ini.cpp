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

#include <SpecialK/stdafx.h>


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
  SK_ConcealUserDirA (
    std::string (file_name).data () ),
                   function_name,
                     args,
                       ini_name,
                         _wcserror (err) );


  return wszFormattedError;
}

#define TRY_FILE_IO(x,y,z) { (z) = ##x; if ((z) == nullptr) \
dll_log->Log (L"[ SpecialK ] %ws", ErrorMessage (GetLastError (), #x, (y), __LINE__, __FUNCTION__, __FILE__).c_str ()); }

iSK_INI::iSK_INI (const wchar_t* filename)
{
  encoding_ = INI_UTF8;

  AddRef ();

  SK_ReleaseAssert (
          filename != nullptr)
  if (    filename == nullptr) return;

  // We skip a few bytes (Unicode BOM) in certain circumstances, so this is the
  //   actual pointer we need to free...
  data = nullptr;

  wszName =
    _wcsdup (filename);

  if (wszName == nullptr)
    return;

  if (wcsstr (filename, L"Version") != nullptr)
    SK_CreateDirectories (filename);

  SK_StripTrailingSlashesW (wszName);

  TRY_FILE_IO (_wfsopen (wszName, L"rbS", _SH_DENYNO), wszName, fINI);

  if (fINI != nullptr)
  {
    sections.clear ();

    auto size =
      gsl::narrow_cast <long> (SK_File_GetSize (wszName));

    // A 4 MiB INI file seems pretty dman unlikely...
    SK_ReleaseAssert (size >= 0 && size < (4L * 1024L * 1024L))

    wszData =
      new (std::nothrow) wchar_t [size + 3] { };

    SK_ReleaseAssert (wszData != nullptr)

    if (wszData == nullptr)
    {
      fclose (fINI);
      return;
    }

    data  = wszData;
    fread  (wszData, size, 1, fINI);
    fclose (fINI);

    // First, consider Unicode
    // UTF16-LE  (All is well in the world)
    if (*wszData == 0xFEFF)
    {
      ++wszData; // Skip the BOM

      encoding_ = INI_UTF16LE;
    }

    // UTF16-BE  (Somehow we are swapped)
    else if (*wszData == 0xFFFE)
    {
      dll_log->Log ( L"[INI Parser] Encountered Byte-Swapped Unicode INI "
                     L"file ('%s'), attempting to recover...",
                       wszName );

      wchar_t* wszSwapMe = wszData;

      for (int i = 0; i < size; i += 2)
      {
        unsigned short swapped =
          _byteswap_ushort (*wszSwapMe);

        (*wszSwapMe) = swapped;
        ++wszSwapMe;
      }

      ++wszData; // Skip the BOM

      encoding_ = INI_UTF16BE;
    }

    // Something else, if it's ANSI or UTF-8, let's hope Windows can figure
    //   out what to do...
    else
    {
      // Skip the silly UTF8 BOM if it is present
      bool utf8 = (reinterpret_cast <unsigned char *> (wszData)) [0] == 0xEF &&
                  (reinterpret_cast <unsigned char *> (wszData)) [1] == 0xBB &&
                  (reinterpret_cast <unsigned char *> (wszData)) [2] == 0xBF;

      const uintptr_t offset =
        utf8 ? 3 : 0;

      const int       real_size =
        size - gsl::narrow_cast <int> (offset);

      char*           start_addr =
      (reinterpret_cast <char *> (wszData)) + offset;

      auto*           string =
        new (std::nothrow) char [real_size + 3] { };

      SK_ReleaseAssert (string != nullptr)

      if (string != nullptr)
      {
        memcpy (string, start_addr, real_size);
      }

      delete [] wszData;
                wszData = nullptr;
                data    = nullptr;

      if (string == nullptr)
      {
        return;
      }

      unsigned int converted_size =
        std::max ( 0,
                     MultiByteToWideChar ( CP_UTF8, 0, string,
                                             real_size, nullptr, 0 )
                 );

      if (0 == converted_size)
      {
        dll_log->Log ( L"[INI Parser] Could not convert UTF-8 / ANSI Encoded "
                       L".ini file ('%s') to UTF-16, aborting!",
                         wszName );

        delete [] string;
                  string = nullptr;

        return;
      }

      wszData =
        new (std::nothrow) wchar_t [converted_size + 3] { };

      SK_ReleaseAssert (wszData != nullptr)

      if (wszData != nullptr)
      {
        data                  = wszData;
        MultiByteToWideChar ( CP_UTF8, 0, string, real_size,
                                wszData, converted_size );

        //dll_log->Log ( L"[INI Parser] Converted UTF-8 INI File: '%s'",
                        //wszName );
      }

      delete [] string;
                string = nullptr;

      // No Byte-Order Marker
      data      = wszData;

      encoding_ = INI_UTF8;
    }

    parse  ();
  }

  else
  {
    wszData = nullptr;
  }
}

iSK_INI::~iSK_INI (void)
{
  ULONG refs =
    Release ();

  SK_ReleaseAssert (refs == 0) // Memory leak?

  if (refs == 0)
  {
    if (wszName != nullptr)
    {
      free (wszName);
            wszName = nullptr;
    }

    delete [] data;
    data = nullptr;
  }
}

auto wcrlen =
  []( const wchar_t* _start,
      const wchar_t* _end ) ->
    size_t
    {
      size_t  _len    = 0;
      const
      wchar_t*  _it   = _start;
      while   ( _it   >    0 &&
                _it   < _end )
      {         _it   =
     CharNextW (_it);
            ++_len;
           if ( _it != 0 &&
               *_it == L'\0' ) break;
      }return _len;
    };

iSK_INISection&
Process_Section ( iSK_INISection  &kSection,
                         wchar_t  *start,
                         wchar_t  *end,
                         SK_TLS  **ppTLS      = nullptr )
{
  SK_TLS *pTLS = nullptr;

  if (   ppTLS != nullptr &&
       (*ppTLS == nullptr)   )
  {     *ppTLS  = SK_TLS_Bottom ();
          pTLS  = *ppTLS;           }

  if (pTLS == nullptr)
      pTLS = SK_TLS_Bottom ();

  // TODO: Re-write to fallback to heap allocated memory
  //         if TLS is not working.
  if (pTLS == nullptr)
  {
    static iSK_INISection fake;
    return fake;
  }

  auto& section = kSection;

  const wchar_t* penultimate = CharPrevW (start, end);
        wchar_t* key         = start;

  for (wchar_t* k = key; k < end && k != nullptr; k = CharNextW (k))
  {
    if (k < penultimate && *k == L'=')
    {

      size_t     key_len =        wcrlen (key, k);
        auto*    key_str =
                 pTLS->scratch_memory->ini.key.alloc
                                       (   key_len + 2, true);
      wcsncpy_s (key_str,                  key_len + 1,
                                           key, _TRUNCATE);

      wchar_t* value =
        CharNextW (k);

      for (wchar_t* l = value; l <= end; l < end ? l = CharNextW (l) : nullptr)
      {
        SK_ReleaseAssert (l != nullptr)

        if (l == nullptr) break;

        if (l > penultimate || *l == L'\n')
        {
          key = CharNextW (l);
            k = key;

          if (l == end)
          {
            l = CharNextW (l);
            k = end;
          }

          size_t     val_len =        wcrlen (value, l);
            auto*    val_str =
                     pTLS->scratch_memory->ini.val.alloc
                                           (   val_len + 2, true);
          wcsncpy_s (val_str,                  val_len + 1,
                                               value, _TRUNCATE);

          section.add_key_value (
                     key_str,
                     val_str    );

          l = end + 1;
        }
      }
    }
  }

  return section;
}

bool
Import_Section ( iSK_INISection  &section,
                        wchar_t  *start,
                        wchar_t  *end,
                         SK_TLS **ppTLS = nullptr )
{
  SK_TLS *pTLS = nullptr;

  if (   ppTLS != nullptr &&
       (*ppTLS == nullptr)   )
  {     *ppTLS  = SK_TLS_Bottom ();
          pTLS  = *ppTLS;           }

  if (pTLS == nullptr)
      pTLS = SK_TLS_Bottom ();

  // TODO: Re-write to fallback to heap allocated memory
  //         if TLS is not working.
  if (pTLS == nullptr)
  {
    return false;
  }


  const wchar_t* penultimate = CharPrevW (start, end);
        wchar_t* key         = start;

  for (wchar_t* k = key; k < end && k != nullptr; k = CharNextW (k))
  {
    if (k < penultimate && *k == L'=')
    {
      size_t     key_len =        wcrlen (key, k);
        auto*    key_str =
                 pTLS->scratch_memory->ini.key.alloc
                                       (   key_len + 2, true);
      wcsncpy_s (key_str,                  key_len + 1,
                                           key, _TRUNCATE);

      wchar_t* value =
        CharNextW (k);

      for (wchar_t* l = value; l <= end; l < end ? l = CharNextW (l) : nullptr)
      {
        SK_ReleaseAssert (l != nullptr);

        if (l == nullptr) break;

        if (l > penultimate || *l == L'\n')
        {
          key = CharNextW (l);
            k = key;

          if (l == end)
          {
            l = CharNextW (l);
            k = end;
          }

          size_t     val_len =        wcrlen (value, l);
            auto*    val_str =
                     pTLS->scratch_memory->ini.val.alloc
                                           (   val_len + 2, true);
          wcsncpy_s (val_str,                  val_len + 1,
                                               value, _TRUNCATE);

          // Prefer to change an existing value
          if (section.contains_key (key_str))
          {
            std::wstring& val =
              section.get_value (key_str);

            val = val_str;
          }

          // But create a new one if it doesn't already exist
          else {
            section.add_key_value (key_str, val_str);
          }

          l = end + 1;
        }
      }
    }
  }

  return true;
}

void
__stdcall
iSK_INI::parse (void)
{
  SK_ReleaseAssert (wszData != nullptr)
  SK_ReleaseAssert (data    != nullptr)

  // Should be a ByteOrderMarker offset, always
  //   > data (base alloc. addr)
  SK_ReleaseAssert (wszData >= data)

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  // TODO: Re-write to fallback to heap allocated memory
  //         if TLS is not working.
  if (pTLS == nullptr)
    return;

  if (wszData != nullptr)
  {
    size_t len =
      lstrlenW (wszData);

    SK_ReleaseAssert (len > 0)

    // We don't want CrLf, just Lf
    bool strip_cr = false;

    wchar_t* wszStrip =
      &wszData [0];

    // Find if the file has any Cr's
    for (size_t i = 0; i < len; i++)
    {
      if (*wszStrip == L'\r')
      {
        strip_cr = true;
        break;
      }

      wszStrip = CharNextW (wszStrip);
    }

    wchar_t* wszDataEnd =
      &wszData [0];

    if (strip_cr)
    {
      wchar_t* wszDataNext =
        &wszData [0];

      for (size_t i = 0; i < len; i++)
      {
        if (*wszDataNext != L'\r')
        {
          *wszDataEnd = *wszDataNext;
           wszDataEnd = CharNextW (wszDataEnd);
        }

        wszDataNext = CharNextW (wszDataNext);
      }

      const wchar_t* wszNext =
        CharNextW (wszDataNext);

      RtlSecureZeroMemory ( wszDataEnd,
                        reinterpret_cast <uintptr_t> (wszNext) -
                        reinterpret_cast <uintptr_t> (wszDataEnd) );

      len =
        lstrlenW (wszData);
    }

    else
    {
      for (size_t i = 0; i < len; i++)
      {
        wszDataEnd = CharNextW (wszDataEnd);
      }
    }

    wchar_t* wszSecondToLast =
      CharPrevW (wszData, wszDataEnd);

    wchar_t* begin           = nullptr;
    wchar_t* end             = nullptr;
    wchar_t* wszDataCur      = &wszData [0];

    for ( wchar_t* i = wszDataCur;
                   i < wszDataEnd &&
                   i != nullptr;     i = CharNextW (i) )
    {
      if ( *i == L'[' &&
           (i == wszData || *CharPrevW (&wszData [0], i) == L'\n') )
      {
        begin =
          CharNextW (i);
      }

      if (   *i == L']' &&
             (i == wszSecondToLast || *CharNextW (i) == L'\n') )
      { end = i; }

      if ( begin != nullptr  &&
           end   != nullptr  &&  begin < end )
      {
        size_t   sec_len =        wcrlen (begin, end);
        auto*    sec_name =
                 pTLS->scratch_memory->ini.sec.alloc
                                       (   sec_len + 2, true);
      wcsncpy_s (sec_name,                 sec_len + 1,
                                           begin, _TRUNCATE);

        wchar_t* start  = CharNextW (CharNextW (end));
        wchar_t* finish = start;

        bool eof = false;
        for (wchar_t* j = start; j <= wszDataEnd; j = CharNextW (j))
        {
          if (j == wszDataEnd)
          {
            finish = j;
            eof    = true;
            break;
          }

          wchar_t *wszPrev = nullptr;

          if (*j == L'[' && (*(wszPrev = CharPrevW (start, j)) == L'\n'))
          {
            finish = wszPrev;
            break;
          }
        }

        iSK_INISection
                section ( sec_name  );
        Process_Section ( section,
                            start, finish,
                              &pTLS );

        sections.emplace (
          std::make_pair (sec_name, section)
        );

        ordered_sections.emplace_back (sec_name);

        if (eof)
          break;

        i = finish;

        end   = nullptr;
        begin = nullptr;
      }
    }
  }



  if (crc32_ == 0x0)
  {
    std::wstring outbuf;
                 outbuf.reserve (16384);

    for ( auto& it : ordered_sections )
    {
      iSK_INISection& section =
        get_section (it.c_str ());

      section.parent = this;

      if ( (! section.name.empty         ()) &&
           (! section.ordered_keys.empty ()) )
      {
        outbuf.append (L"[");
        outbuf.append (section.name).append (L"]\n");

        for ( auto& key_it : section.ordered_keys )
        {
          const std::wstring& val =
            section.get_value (key_it.c_str ());

          outbuf.append (key_it).append (L"=");
          outbuf.append (val).append (L"\n");
        }

        outbuf.append (L"\n");
      }
    }

    if ((! outbuf.empty ()) &&
           outbuf.back  ()  == L'\n')
    {
      // Strip the unnecessary extra newline
      outbuf.resize (outbuf.size () - 1);
    }

    crc32_ =
      crc32 ( 0x0, outbuf.data   (),
                   outbuf.length () * sizeof (wchar_t) );
  }
}

void
__stdcall
iSK_INI::import (const wchar_t* import_data)
{
  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (pTLS == nullptr)
    return;

  wchar_t* wszImport =
    _wcsdup (import_data);

  if (wszImport != nullptr)
  {
    int len =
      lstrlenW (wszImport);

    // We don't want CrLf, just Lf
    bool strip_cr = false;

    wchar_t* wszStrip =
      &wszImport [0];

    // Find if the file has any Cr's
    for (int i = 0; i < len; i++)
    {
      if (*wszStrip == L'\r')
      {
        strip_cr = true;
        break;
      }

      wszStrip =
        CharNextW (wszStrip);
    }

    wchar_t* wszImportEnd =
      &wszImport [0];

    if (strip_cr)
    {
      wchar_t* wszImportNext =
        &wszImport [0];

      for (int i = 0; i < len; i++)
      {
        if (*wszImportNext != L'\r')
        {
          *wszImportEnd = *wszImportNext;
           wszImportEnd = CharNextW (wszImportEnd);
        }

        wszImportNext   = CharNextW (wszImportNext);
      }

      const wchar_t* wszNext =
        CharNextW (wszImportNext);

      RtlSecureZeroMemory ( wszImportEnd,
                        reinterpret_cast <uintptr_t> (wszNext) -
                        reinterpret_cast <uintptr_t> (wszImportEnd) );

      len =
        lstrlenW (wszImport);
    }

    else
    {
      for (int i = 0; i < (len - 1); i++)
      {
        wszImportEnd = CharNextW (wszImportEnd);
      }
    }

    wchar_t* wszSecondToLast =
      CharPrevW (wszImport, wszImportEnd);

    wchar_t* begin           = nullptr;
    wchar_t* end             = nullptr;

    wchar_t* wszImportCur    = &wszImport [0];

    for (wchar_t* i = wszImportCur; i < wszImportEnd && i != nullptr; i = CharNextW (i))
    {
      if (*i == L'[' && (i == wszImport || *CharPrevW (&wszImport [0], i) == L'\n'))
      {
        begin = CharNextW (i);
      }

      if (*i == L']' && (i == wszSecondToLast || *CharNextW (i) == L'\n'))
        end = i;

      if (begin != nullptr && end != nullptr)
      {
        size_t   sec_len =        wcrlen (begin, end);
        auto*    sec_name =
                 pTLS->scratch_memory->ini.sec.alloc
                                       (   sec_len + 2, true);
      wcsncpy_s (sec_name,                 sec_len + 1,
                                           begin, _TRUNCATE);

        //MessageBoxW (NULL, sec_name, L"Section", MB_OK);

        wchar_t* start  = CharNextW (CharNextW (end));
        wchar_t* finish = start;
        bool     eof    = false;

        for (wchar_t* j = start; j <= wszImportEnd; j = CharNextW (j))
        {
          if (j == wszImportEnd)
          {
            finish = j;
            eof    = true;
            break;
          }

          wchar_t *wszPrev = nullptr;

          if (*j == L'[' && (*(wszPrev = CharPrevW (start, j)) == L'\n'))
          {
            finish = wszPrev;
            break;
          }
        }

        // Import if the section already exists
        if (contains_section (sec_name))
        {
          iSK_INISection& section =
            get_section  (sec_name);

          section.parent = this;

          Import_Section (section, start, finish, &pTLS);
        }

        // Insert otherwise
        else
        {
          iSK_INISection
                  section ( sec_name  );
          Process_Section ( section,
                              start, finish,
                                &pTLS );

          section.parent = this;

          sections.emplace  (
            std::make_pair  (sec_name, section)
          );

          ordered_sections.emplace_back (sec_name);
        }

        if (eof)
          break;

        i = finish;

        end   = nullptr;
        begin = nullptr;
      }
    }
  }

  free (wszImport);
}

std::wstring&
__stdcall
iSK_INISection::get_value (const wchar_t* key)
{
  auto it_key =
    keys.find (key);

  if (it_key != keys.cend ())
    return (*it_key).second;

  static
    std::wstring
         invalid = L"Invalid";
  return invalid;
}

void
__stdcall
iSK_INISection::set_name (const wchar_t* name_)
{
  if (parent != nullptr)
  {
    if (name != name_)
      parent->crc32_ = 0x0;
  }

  name = name_;
}

bool
__stdcall
iSK_INISection::contains_key (const wchar_t* key)
{
  auto _kvp =
    keys.find (key);

  return
    (   _kvp != keys.cend  () &&
     (! _kvp->second.empty () ));
}

void
__stdcall
iSK_INISection::add_key_value (const wchar_t* key, const wchar_t* value)
{
  auto add =
    keys.emplace (std::make_pair (key, value));

  if (add.second)
  {
    ordered_keys.emplace_back (key);

    if (parent != nullptr)
        parent->crc32_ = 0x0;
  }

  else
  {
    std::wstring_view val_wstr (value);

    if (! add.first->second._Equal (val_wstr.data ()))
    {
      if (parent != nullptr)
      {
        parent->crc32_ = 0x0;
      }

      add.first->second =
        val_wstr;
    }
  }
}

bool
__stdcall
iSK_INI::contains_section (const wchar_t* section)
{
  return
    ( sections.find (section) !=
      sections.cend (       ) );
}

iSK_INISection&
__stdcall
iSK_INI::get_section (const wchar_t* section)
{
  auto sec =
    sections.find (section);

  bool try_emplace =
    ( sec == sections.cend () );

  iSK_INISection& ret =
    sections [section];

  if (try_emplace)
  {
                     ret.parent  =  this;
                     ret.set_name (section);
    ordered_sections.emplace_back (section);
  }

  return ret;
}


iSK_INISection&
__cdecl
iSK_INI::get_section_f ( _In_z_ _Printf_format_string_
                         wchar_t const* const    _Format,
                                                 ... )
{
  wchar_t wszFormatted [128] = { };

  va_list   _ArgList;
  va_start (_ArgList, _Format);
  {
    int len = 0;

    // ASSERT: Length <= 127 characters
    len += vswprintf (wszFormatted, _Format, _ArgList);

    SK_ReleaseAssert (len <= 127)
  }
  va_end   (_ArgList);

  bool try_emplace =
    ( sections.find (wszFormatted) ==
      sections.cend (            ) );

  iSK_INISection& ret =
    sections [wszFormatted];

  if (try_emplace)
  {
                        ret.name = wszFormatted;
    ordered_sections.emplace_back (wszFormatted);
  }

  return ret;
}

void
__stdcall
iSK_INI::write (const wchar_t* fname)
{
  std::wstring outbuf;
               outbuf.reserve (16384);

  for ( auto& it : ordered_sections )
  {
    iSK_INISection& section =
      get_section (it.c_str ());

    if ( (! section.name.empty         ()) &&
         (! section.ordered_keys.empty ()) )
    {
      outbuf += L"[";
      outbuf += section.name + L"]\n";

      for ( auto& key_it : section.ordered_keys )
      {
        const std::wstring& val =
          section.get_value (key_it.c_str ());

        outbuf += key_it + L"=";
        outbuf += val    + L"\n";
      }

      outbuf += L"\n";
    }
  }

  if ( (! outbuf.empty ()) &&
          outbuf.back  ()  == L'\n' )
  {
    // Strip the unnecessary extra newline
    outbuf.resize (outbuf.size () - 1);
  }


  uint32_t new_crc32 =
    crc32 ( 0x0, outbuf.data   (),
                 outbuf.length () * sizeof (wchar_t) );

//dll_log->Log (L"%s => Old: %x, New: %x", get_filename ( ), crc32c_, new_crc32c);

  if (new_crc32 == crc32_)
    return;


  SK_CreateDirectories (fname);

  FILE* fOut = nullptr;

  switch (encoding_)
  {
    case INI_UTF8:
      TRY_FILE_IO (_wfsopen (fname, L"wc,ccs=UTF-8",    _SH_DENYNO), fname, fOut);
      break;

    // Cannot preserve this, consider adding a byte-swap on file close
    case INI_UTF16BE:
      TRY_FILE_IO (_wfsopen (fname, L"wc,ccs=UTF-16LE", _SH_DENYNO), fname, fOut);
      break;

    default:
    case INI_UTF16LE:
      TRY_FILE_IO (_wfsopen (fname, L"wc,ccs=UTF-16LE", _SH_DENYNO), fname, fOut);
      break;
  }

  if (fOut == nullptr)
  {
    //SK_MessageBox (L"ERROR: Cannot open INI file for writing. Is it "
    //               L"read-only?", fname, MB_OK | MB_ICONSTOP);
    return;
  }

  fputws (outbuf.c_str (), fOut);
  fclose (fOut);

  crc32_ = new_crc32;
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
  if (IsEqualGUID (riid, IID_SK_INI) != 0)
  {
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
  return InterlockedIncrement (&refs_);
}

ULONG
__stdcall
iSK_INI::Release (THIS)
{
  return InterlockedDecrement (&refs_);
}

bool
__stdcall
iSK_INI::remove_section (const wchar_t* wszSection)
{
  std::wstring_view section_w (wszSection);

  auto it =
    std::find ( ordered_sections.begin (),
                ordered_sections.end   (),
                  section_w );

  if (it != ordered_sections.end ())
  {
    ordered_sections.erase (it);
    sections.erase         (section_w.data ());

    return true;
  }

  return false;
}

bool
__stdcall
iSK_INISection::remove_key (const wchar_t* wszKey)
{
  std::wstring_view key_w (wszKey);

  auto it =
    std::find ( ordered_keys.begin (),
                ordered_keys.end   (),
                  key_w );

  if (it != ordered_keys.end ())
  {
    ordered_keys.erase (it);
    keys.erase         (key_w.data ());

    if (parent != nullptr)
        parent->crc32_ = 0x0;

    return true;
  }

  return false;
}


HRESULT
__stdcall
iSK_INISection::QueryInterface (THIS_ REFIID riid, void** ppvObj)
{
  if (IsEqualGUID (riid, IID_SK_INISection) != 0)
  {
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
  ULONG ret = InterlockedDecrement (&refs);

  if (ret == 0)
  {
    // Add to ToFree list in future
    //delete this;
  }

  return ret;
}


const wchar_t*
iSK_INI::get_filename (void) const
{
  return wszName;
}

bool
iSK_INI::import_file (const wchar_t* fname)
{
  size_t len =
    wcslen (fname);

  if (len == 0)
    return false;

  // We skip a few bytes (Unicode BOM) in certain circumstances, so this is
  //   the actual pointer we need to free...
  CHeapPtr <wchar_t> wszImportName (
    new (std::nothrow) wchar_t [len + 2] { }
  );

  if (wszImportName == nullptr)
    return false;

  wchar_t  *wszImportData = nullptr;
  wchar_t  *alloc         = wszImportData;
  FILE*     fImportINI    = nullptr;

  wcsncpy_s (wszImportName, len + 1,
                         fname, _TRUNCATE);

  TRY_FILE_IO (_wfsopen (fname, L"rbS", _SH_DENYNO),
                         fname, fImportINI);

  if (fImportINI != nullptr)
  {
    auto size =
      gsl::narrow_cast <long> (
        SK_File_GetSize (fname)
      );

        wszImportData  = new (std::nothrow) wchar_t [size + 3] { };
    if (wszImportData == nullptr)
    {
      fclose (fImportINI);
      return false;
    }

    alloc = wszImportData;
    fread  (wszImportData, size, 1, fImportINI);

    fflush (fImportINI);
    fclose (fImportINI);
            fImportINI = nullptr;

    // First, consider Unicode
    // UTF16-LE  (All is well in the world)
    if (*wszImportData == 0xFEFF)
    {
      ++wszImportData; // Skip the BOM
    }

    // UTF16-BE  (Somehow we are swapped)
    else if (*wszImportData == 0xFFFE)
    {
      dll_log->Log ( L"[INI Parser] Encountered Byte-Swapped Unicode INI "
                     L"file ('%s'), attempting to recover...",
                       wszImportName.m_pData );

      wchar_t* wszSwapMe = wszImportData;

      for (int i = 0; i < size; i += 2)
      {
        unsigned short swapped =
          _byteswap_ushort (*wszSwapMe);

        (*wszSwapMe) = swapped;

        ++wszSwapMe;
      }

      ++wszImportData; // Skip the BOM
    }

    // Something else, if it's ANSI or UTF-8, let's hope Windows can figure
    //   out what to do...
    else
    {
      // Skip the silly UTF8 BOM if it is present
      bool utf8 =
        (reinterpret_cast <unsigned char *> (wszImportData)) [0] == 0xEF &&
        (reinterpret_cast <unsigned char *> (wszImportData)) [1] == 0xBB &&
        (reinterpret_cast <unsigned char *> (wszImportData)) [2] == 0xBF;

      const uintptr_t offset =
        utf8 ? 3 : 0;

      const int       real_size  =
        size - gsl::narrow_cast <int> (offset);

      char*           start_addr =
      (reinterpret_cast <char *> (wszImportData)) + offset;

      auto*           string =
        new (std::nothrow) char [real_size + 2] { };

      if (string != nullptr)
      {
        memcpy (string, start_addr, real_size);
      }

      if (        wszImportData != nullptr)
      {
        delete [] wszImportData;
                  wszImportData = nullptr;
                          alloc = nullptr;
      }

      if (string == nullptr)
      {
        return false;
      }

      unsigned int converted_size =
        MultiByteToWideChar ( CP_UTF8, 0, string, real_size,
                              nullptr, 0 );

      if (0 == converted_size)
      {
        if (real_size > 0)
        {
          dll_log->Log ( L"[INI Parser] Could not convert UTF-8 / ANSI "
                         L"Encoded .ini file ('%s') to UTF-16, aborting!",
                           wszImportName.m_pData );
        }

        if (        string != nullptr) {
          delete [] string;
                    string = nullptr;  }

        return false;
      }

      wszImportData =
        new (std::nothrow) wchar_t [converted_size + 2] { };

      SK_ReleaseAssert (
          wszImportData != nullptr)
      if (wszImportData != nullptr)
      {
        if (string != nullptr)
        {
          MultiByteToWideChar ( CP_UTF8, 0, string, real_size,
                               wszImportData, converted_size );

          //dll_log->Log ( L"[INI Parser] Converted UTF-8 INI File: '%s'",
          //wszImportName );

          delete [] string;
                    string = nullptr;
        }

        // No Byte-Order Marker
        alloc = wszImportData;
      }

      else
        return false;
    }

    import (wszImportData);

    if (alloc != nullptr)
    {
      delete [] alloc;
                alloc = nullptr;
    }

    return true;
  }

  return false;
}

bool
iSK_INI::rename (const wchar_t* fname)
{
  if (  fname  != nullptr &&
       *fname == L'\0' )
  {
    free (wszName);

    wszName =
      _wcsdup (fname);

    return true;
  }

  return false;
}

iSK_INI*
__stdcall
SK_CreateINI (const wchar_t* const wszName)
{
  auto  *pINI =
    new (std::nothrow) iSK_INI (wszName);

  assert
        (pINI != nullptr);

  return pINI;
}