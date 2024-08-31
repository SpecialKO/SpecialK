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


class SK_ScopedLocale {
public:
  SK_ScopedLocale (const wchar_t *wszLocale)
  {
    if (wszLocale != nullptr)
    {
      prev_locale_policy =
        _configthreadlocale (_ENABLE_PER_THREAD_LOCALE);

      if (prev_locale_policy != -1)
      {
        orig_locale =
        _wsetlocale (LC_ALL, wszLocale);
      }
    }
  }

  ~SK_ScopedLocale (void)
  {
    if (prev_locale_policy != -1)
    {
      if (! orig_locale.empty ())
        _wsetlocale (LC_ALL, orig_locale.c_str ());

      _configthreadlocale (prev_locale_policy);
    }
  }

private:
  std::wstring orig_locale        = L"";
  int          prev_locale_policy = -1;
};

std::wstring
ErrorMessage (errno_t        err,
              const char*    args,
              const wchar_t* ini_name,
              UINT           line_no,
              const char*    function_name,
              const char*    file_name)
{
  wchar_t wszFormattedError [1024] = {};

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
SK_LOG0 ( ( L"%ws", ErrorMessage (GetLastError (), #x, (y), __LINE__, __FUNCTION__, __FILE__).c_str () ), \
            L" SpecialK " ); }

BOOL
SK_File_GetModificationTime (const wchar_t* wszFile, FILETIME* pfModifyTime)
{
  if (! pfModifyTime)
    return FALSE;

  SK_AutoHandle hFile (
    CreateFileW ( wszFile, GENERIC_READ,
                        FILE_SHARE_READ, nullptr,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr )
  );

  if (hFile.m_h != 0)
  {
    FILETIME                   ftCreation,  ftAccess;

    return
      GetFileTime (hFile.m_h, &ftCreation, &ftAccess, pfModifyTime);
  }

  return FALSE;
}

bool
iSK_INI::reload (const wchar_t *fname)
{
  SK_ScopedLocale _locale (L"en_us.utf8");

  SK_TLS* pTLS =
    SK_TLS_Bottom ();
  
  if (pTLS == nullptr)
    return false; // Out of Memory

  if (fname == nullptr)
  {
    SK_ReleaseAssert (name.size  () > 0);
    if (              name.empty ()) return false; // Empty String -> Dummy INI

    fname = name.c_str ();
  }

  if (! PathFileExistsW (fname))
    return false;

  if (file_watch == nullptr)
  {
    wchar_t     wszDirName [MAX_PATH] = { };
    wcsncpy_s ( wszDirName, MAX_PATH,
                  fname,_TRUNCATE );

    if (PathRemoveFileSpecW (wszDirName))
    {
      file_watch =
        FindFirstChangeNotification (wszDirName, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
    }
  }

  // Avoid reloading when it would make no sense to do so
  if (! _wcsicmp (fname, name.c_str ()))
  {
    FILETIME                ftLastWrite =    file_stamp;
    if (SK_File_GetModificationTime (fname, &file_stamp) &&
          CompareFileTime (&ftLastWrite,    &file_stamp) <= 0)
    {
      flushed_ = file_stamp;

      if (! sections.empty ())
      {
        SK_LOG2 ( ( L"Skipping Reload on file '%ws' because it is unchanged", fname ),
                    L"ConfigLoad" );

        return true;
      }
    }

    flushed_ = file_stamp;

    SK_LOG2 ( ( L"Continuing reload on file '%ws' because old modification time = %llu, "
                L"new = %llu", fname, *(UINT64 *)&ftLastWrite, *(UINT64 *)&file_stamp ),
                    L"ConfigLoad" );
  }

  data.clear ();

  TRY_FILE_IO (_wfsopen (fname, L"rbS", _SH_DENYNO), fname, fINI);

  if (fINI != nullptr)
  {
    sections.clear         ();
    ordered_sections.clear ();

    auto size =
      sk::narrow_cast <long> (SK_File_GetSize (fname));

    // A 4 MiB INI file seems pretty dman unlikely...
    SK_ReleaseAssert (size >= 0 && size < (4L * 1024L * 1024L));

    data.resize (size + 3);

    SK_ReleaseAssert (data.size () > 0);

    if (data.size () == 0)
    {
      fclose (fINI);
      return false;
    }

    fread  (data.data (), size, 1, fINI);
    fclose (fINI);

    // First, consider Unicode
    // UTF16-LE  (All is well in the world)
    if (*data.data () == 0xFEFF)
    {
      bom_size = 1; // Skip the BOM

      encoding_ = INI_UTF16LE;
    }

    // UTF16-BE  (Somehow we are swapped)
    else if (*data.data () == 0xFFFE)
    {
      SK_LOG0 ( ( L"Encountered Byte-Swapped Unicode INI "
                  L"file ('%s'), attempting to recover...",
                                                   fname ),
                  L"INI Parser" );

      wchar_t* wszSwapMe = data.data ();

      for (int i = 0; i < size; i += 2)
      {
        unsigned short swapped =
          _byteswap_ushort (*wszSwapMe);

        (*wszSwapMe) = swapped;
        ++wszSwapMe;
      }

      bom_size = 1; // Skip the BOM

      encoding_ = INI_UTF16BE;
    }

    // Something else, if it's ANSI or UTF-8, let's hope Windows can figure
    //   out what to do...
    else
    {
      // Skip the silly UTF8 BOM if it is present
      bool utf8_bom = (reinterpret_cast <unsigned char *> (data.data ())) [0] == 0xEF &&
                      (reinterpret_cast <unsigned char *> (data.data ())) [1] == 0xBB &&
                      (reinterpret_cast <unsigned char *> (data.data ())) [2] == 0xBF;

      const uintptr_t offset =
           utf8_bom ? 3 : 0;

      const int       real_size =
        size - sk::narrow_cast <int> (offset);

      char*           start_addr =
      (reinterpret_cast <char *> (data.data ())) + offset;

      char*           string =
        pTLS->scratch_memory->ini.utf8_string.alloc (real_size + 3, true);

      SK_ReleaseAssert (string != nullptr);

      if (string == nullptr)
      {
        return false;
      }

      memcpy (string, start_addr, real_size);

      const UINT converted_size =
        std::max ( 0,
                     MultiByteToWideChar ( CP_UTF8, MB_PRECOMPOSED |
                                                    MB_ERR_INVALID_CHARS,
                                          string, real_size, nullptr, 0 )
                 );

      if (0 == converted_size && ((! utf8_bom) || real_size > 0))
      {
        SK_ReleaseAssert (GetLastError () != ERROR_NO_UNICODE_TRANSLATION);

        std::string utf8_fname =
          SK_StripUserNameFromPathA (
            SK_WideCharToUTF8 (fname).data ()
          );

        SK_LOG0 ( ( L"Could not convert UTF-8 / ANSI Encoded "
                    L".ini file ('%hs') to UTF-16.",
                                        utf8_fname.c_str () ),
                    L"INI Parser" );

        std::wstring backup =
          SK_FormatStringW (L"%ws.bak", fname);
        CopyFileW (                     fname,
                     backup.c_str (), TRUE
        );

        utf8_fname =
          SK_StripUserNameFromPathA (
            SK_WideCharToUTF8 (backup).data ()
          );

        SK_LOG0 ( ( L"Backing-up corrupted INI file to '%hs' and aborting!",
                     utf8_fname.c_str () ),
                    L"INI Parser" );

        encoding_ = INI_INVALID;

        return false;
      }

      if (data.size () < converted_size + 3)
          data.resize   (converted_size + 3);

      SK_ReleaseAssert (data.size () > 0);

      if (data.size () > 0)
      {
        MultiByteToWideChar ( CP_UTF8, MB_PRECOMPOSED |
                                 MB_ERR_INVALID_CHARS, string,  real_size,
                                             data.data (), converted_size );

        SK_ReleaseAssert (GetLastError () != ERROR_NO_UNICODE_TRANSLATION);

        //SK_LOG0 ( ( L"Converted UTF-8 INI File: '%s'",
                        //fname ), L"INI Parser" );
      }

      // No Byte-Order-Marker
      bom_size  = 0;
      encoding_ = utf8_bom ? INI_UTF8 : INI_UTF8NOBOM;
    }

    parse ();

    return true;
  }

  return false;
}

iSK_INI::iSK_INI (const wchar_t* filename)
{
  encoding_ = INI_UTF8;

  AddRef ();

  SK_ReleaseAssert (
          filename != nullptr);
  if (    filename == nullptr) return;
  if (   *filename == L'\0'  ) return; // Empty String -> Dummy INI

  name = filename;

  if (name.empty ())
  {
    return;
  }

  if (wcsstr (filename, L"Version") != nullptr)
    SK_CreateDirectories (filename);

  SK_StripTrailingSlashesW (name.data ());

  reload ();
}

iSK_INI::~iSK_INI (void)
{
  ULONG refs =
    Release ();

  SK_ReleaseAssert (refs == 0); // Memory leak?

  if (refs == 0)
  {
    name.clear ();
    data.clear ();
  }

  if (file_watch != nullptr)
  {
    FindCloseChangeNotification (
      std::exchange (file_watch, nullptr)
    );
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
      while   ( _it  != nullptr &&
                _it   < _end )
      {         _it   =
  SK_CharNextW (_it);
            ++_len;
           if ( _it != nullptr &&
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

  const wchar_t* penultimate = SK_CharPrevW (start, end);
        wchar_t* key         = start;

  for (wchar_t* k = key; k < end && k != nullptr; k = SK_CharNextW (k))
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
        SK_CharNextW (k);

      for (wchar_t* l = value; l <= end; l < end ? l = SK_CharNextW (l) : nullptr)
      {
        SK_ReleaseAssert (l != nullptr);

        if (l == nullptr) break;

        if (l > penultimate || *l == L'\n')
        {
          key = SK_CharNextW (l);
            k = key;

          if (l == end)
          {
            l = SK_CharNextW (l);
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


  const wchar_t* penultimate = SK_CharPrevW (start, end);
        wchar_t* key         = start;

  for (wchar_t* k = key; k < end && k != nullptr; k = SK_CharNextW (k))
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
        SK_CharNextW (k);

      for (wchar_t* l = value; l <= end; l < end ? l = SK_CharNextW (l) : nullptr)
      {
        SK_ReleaseAssert (l != nullptr);

        if (l == nullptr) break;

        if (l > penultimate || *l == L'\n')
        {
          key = SK_CharNextW (l);
            k = key;

          if (l == end)
          {
            l = SK_CharNextW (l);
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

__declspec(nothrow)
void
__stdcall
iSK_INI::parse (void)
{
  SK_ScopedLocale _locale (L"en_us.utf8");

  SK_ReleaseAssert (data.size () > 0);

  if (data.empty ())
    return;

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  // TODO: Re-write to fallback to heap allocated memory
  //         if TLS is not working.
  if (pTLS == nullptr)
    return; // Out of Memory

  if (sk::narrow_cast <size_t> (bom_size) >= data.size ())
    return; // Invalid Data

  if (data.at (0) == L'\0')
    return; // Empty String

  size_t len =
    wcsnlen_s (&data [bom_size], data.size () - bom_size);

  if (len == 0)
    return; // Empty String (except the BOM...?)

  wchar_t* pStart = &data [bom_size];
  wchar_t* pEnd   = pStart;

  // We don't want CrLf, just Lf
  bool strip_cr = false;

  // Find if the file has any Cr's
  for (size_t i = 0; i < len && (! strip_cr); i++)
  {
    strip_cr =
     ( *pEnd == L'\r' );

    pEnd =
      SK_CharNextW (pEnd);
  }

  pEnd = pStart;

  if (strip_cr)
  {
    wchar_t* pNext = pStart;
    wchar_t  wc    = *pNext;

    for (size_t i = 0; i < len; i++)
    {
      if (wc != L'\r')
      {
        *pEnd = wc;
         pEnd = SK_CharNextW (pEnd);
      } pNext = SK_CharNextW (pNext);

      wc = *pNext;
    }

    ZeroMemory (pEnd, (SK_CharNextW (pNext) - pEnd) *
                                     sizeof (*pEnd));

    len =
      wcsnlen_s (pStart, (pEnd - pStart) /
                        sizeof (*pStart));
  }

  else
  {
    pEnd =
      const_cast <wchar_t *> (
        SK_CharNextW (pEnd, len)
      );
  }

  wchar_t* wszSecondToLast =
    SK_CharPrevW (pStart, pEnd);

  wchar_t* begin           = nullptr;
  wchar_t* end             = nullptr;

  for ( wchar_t* i = pStart;
                 i < pEnd &&
                 i != nullptr; i = SK_CharNextW (i) )
  {
    if ( *i == L'[' &&
         (i == pStart || *SK_CharPrevW (pStart, i) == L'\n') )
    {
      begin =
        SK_CharNextW (i);
    }

    if (   *i == L']' &&
           (i == wszSecondToLast || *SK_CharNextW (i) == L'\n') )
    { end = i; }

    if ( begin != nullptr &&
         end   != nullptr && begin < end )
    {
      size_t     sec_len  = wcrlen (begin, end);
      auto*      sec_name =
                 pTLS->scratch_memory->ini.sec.alloc
                                       (   sec_len + 2, true);
      wcsncpy_s (sec_name,                 sec_len + 1,
                                           begin, _TRUNCATE);

      wchar_t* start  = SK_CharNextW (end, 2);
      wchar_t* finish = start;
      bool     eof    = false;

      if (start != nullptr)
      {
        for (wchar_t* j = start; j <= pEnd; j = SK_CharNextW (j))
        {
          if (j == nullptr)
            break;

          if (j == pEnd)
          {
            finish = j;
            eof    = true;
            break;
          }

          if ( wchar_t *wszPrev = nullptr;            *j    == L'[' &&
                      *(wszPrev = SK_CharPrevW (start, j)) == L'\n' )
          {
            finish = wszPrev;
            break;
          }
        }

        iSK_INISection
                section ( sec_name );
        Process_Section ( section,
                            start, finish,
                              &pTLS );

        ordered_sections.emplace_back (
               &sections.emplace      (section.name,
                                       section).first->second
                                      );
      }

      if (eof)
        break;

      i = finish;

      end   = nullptr;
      begin = nullptr;
    }
  }

  //if (crc32_ == 0x0)
  {
    std::wstring outbuf;
                 outbuf.reserve (16384);

    for ( auto& it : ordered_sections )
    {
      iSK_INISection& section = *it;

      section.parent = this;

      if ( (! section.name.        empty ()) &&
           (! section.ordered_keys.empty ()) )
      {
        outbuf.append (L"[");
        outbuf.append (section.name).append (L"]\n");

        for ( auto& key_it : section.ordered_keys )
        {
          const std::wstring& val =
            section.get_value (key_it);

          outbuf.append (key_it).append (L"=");
          outbuf.append (val).   append (L"\n");
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

__declspec(nothrow)
void
__stdcall
iSK_INI::import (const wchar_t* import_data)
{
  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (pTLS == nullptr)
    return; // Out of Memory

  if (import_data == nullptr)
    return; // Invalid Pointer

  if (*import_data == L'\0')
    return; // Empty String

  std::wstring      import_wstr (import_data);
  wchar_t* pStart = import_wstr.data ();
  wchar_t* pEnd   = pStart;

  if (pStart == nullptr)
    return; // Out of Memory

  size_t len =
    import_wstr.length ();

  // We don't want CrLf, just Lf
  bool strip_cr = false;

  // Find if the file has any Cr's
  for (size_t i = 0; i < len && (! strip_cr); i++)
  {
    strip_cr =
     ( *pEnd == L'\r' );

    pEnd =
      SK_CharNextW (pEnd);
  }

  pEnd = pStart;

  if (strip_cr)
  {
    wchar_t* pNext = pStart;
    wchar_t  wc    = *pNext;

    for (size_t i = 0; i < len; i++)
    {
      if (wc != L'\r')
      {
        *pEnd = wc;
         pEnd = SK_CharNextW (pEnd);
      } pNext = SK_CharNextW (pNext);

      wc = *pNext;
    }

    ZeroMemory (pEnd, (SK_CharNextW (pNext) - pEnd) *
                                     sizeof (*pEnd));

    len =
      wcsnlen_s (pStart, (pEnd - pStart) /
                        sizeof (*pStart));
  }

  else
  {
    pEnd =
      const_cast <wchar_t *> (
        SK_CharNextW (pEnd, len)
      );
  }

  wchar_t* wszSecondToLast =
    SK_CharPrevW (pStart, pEnd);

  wchar_t* begin           = nullptr;
  wchar_t* end             = nullptr;

  for ( wchar_t* i = pStart;
                 i < pEnd &&
                 i != nullptr; i = SK_CharNextW (i) )
  {
    if ( *i == L'[' &&
         (i == pStart || *SK_CharPrevW (pStart, i) == L'\n'))
    {
      begin =
        SK_CharNextW (i);
    }

    if (   *i == L']' &&
           (i == wszSecondToLast || *SK_CharNextW (i) == L'\n') )
    { end = i; }

    if ( begin != nullptr &&
         end   != nullptr && begin < end )
    {
      size_t   sec_len  = wcrlen (begin, end);
      auto*    sec_name =
               pTLS->scratch_memory->ini.sec.alloc
                                     (   sec_len + 2, true);
    wcsncpy_s (sec_name,                 sec_len + 1,
                                         begin, _TRUNCATE);

      //MessageBoxW (NULL, sec_name, L"Section", MB_OK);

      wchar_t* start  = SK_CharNextW (end, 2);
      wchar_t* finish = start;
      bool     eof    = false;

      for (wchar_t* j = start; j <= pEnd; j = SK_CharNextW (j))
      {
        if (j == nullptr)
          break;

        if (j == pEnd)
        {
          finish = j;
          eof    = true;
          break;
        }

        if ( wchar_t *wszPrev = nullptr;            *j    == L'[' &&
                    *(wszPrev = SK_CharPrevW (start, j)) == L'\n' )
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
                section ( sec_name );
        Process_Section ( section,
                            start, finish,
                              &pTLS ).parent = this;

        ordered_sections.emplace_back (
               &sections.emplace      (
                                        section.name,
                             std::move (section)
                                      ).first->second
                                      );
      }

      if (eof)
        break;

      i = finish;

      end   = nullptr;
      begin = nullptr;
    }
  }
}

__declspec(nothrow)
std::wstring&
__stdcall
iSK_INISection::get_value (const wchar_t* key)
{
  return
    get_value (std::wstring_view (key));
}

__declspec(nothrow)
std::wstring&
__stdcall
iSK_INISection::get_value (const std::wstring_view key)
{
  auto it_key =
    keys.find (key.data ());

  if (it_key != keys.cend ())
    return (*it_key).second;

  static
    std::wstring
         invalid = L"Invalid";
  return invalid;
}

__declspec(nothrow)
std::wstring&
__stdcall
iSK_INISection::get_value (const std::wstring& key)
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

__declspec(nothrow)
const std::wstring&
__stdcall
iSK_INISection::get_cvalue (const std::wstring& key) const
{
  const auto it_key =
    keys.find (key);

  if (it_key != keys.cend ())
    return (*it_key).second;

  static const
    std::wstring
         invalid = L"Invalid";
  return invalid;
}

__declspec(nothrow)
void
__stdcall
iSK_INISection::set_name (const wchar_t* name_)
{
  return
    set_name (std::wstring_view (name_));
}

__declspec(nothrow)
void
__stdcall
iSK_INISection::set_name (const std::wstring_view name_)
{
  if (parent != nullptr)
  {
    if (! name_._Equal (name))
    { parent->crc32_ = 0x0;

      SK_LOG2 ( ( L"Forced INI Flush: iSK_INISection::set_name (%ws)", name_.data () ),
                  L"ConfigMgmt" );
    }
  }

  name = name_;
}

__declspec(nothrow)
void
__stdcall
iSK_INISection::set_name (const std::wstring& name_)
{
  if (parent != nullptr)
  {
    if (! name_._Equal (name))
    { parent->crc32_ = 0x0;

      SK_LOG2 ( ( L"Forced INI Flush: iSK_INISection::set_name (%ws)", name_.c_str () ),
                  L"ConfigMgmt" );
    }
  }

  name = name_;
}

__declspec(nothrow)
bool
__stdcall
iSK_INISection::contains_key (const wchar_t* key) const
{
  return
    contains_key (std::wstring_view (key));
}

__declspec(nothrow)
bool
__stdcall
iSK_INISection::contains_key (const std::wstring_view key) const
{
  const auto _kvp =
    keys.find (key.data ());

  return
    (   _kvp != keys.cend  () &&
     (! _kvp->second.empty () ));
}

__declspec(nothrow)
const std::wstring*
__stdcall
iSK_INISection::contains_key (const std::wstring& key) const
{
  const auto _kvp =
    keys.find (key);

  if (       _kvp != keys.cend  () &&
          (! _kvp->second.empty () ))
    return  &_kvp->second;

  return nullptr;
}

__declspec(nothrow)
void
__stdcall
iSK_INISection::add_key_value (const wchar_t* key, const wchar_t* value)
{
  return
    add_key_value ( std::wstring_view (key),
                    std::wstring_view (value) );
}

__declspec(nothrow)
void
__stdcall
iSK_INISection::add_key_value (const std::wstring_view key, const std::wstring_view value)
{
  const auto add =
    keys.emplace (std::make_pair (key, value));

  if (add.second)
  {
    ordered_keys.emplace_back (key);

    if (parent != nullptr)
    {   parent->crc32_ = 0x0;

      SK_LOG2 ( ( L"Forced INI Flush: iSK_INISection::add_key_value (%ws, %ws)",
                                                 key.data (), value.data () ),
                  L"ConfigMgmt" );
    }
  }

  else
  {
    if (! value._Equal (add.first->second))
    {
      // Implicit Flush is not needed
      //
      ///if (parent != nullptr)
      ///    parent->crc32_ = 0x0;

      add.first->second =
        value;
    }
  }
}

__declspec(nothrow)
void
__stdcall
iSK_INISection::add_key_value (const std::wstring& key, const std::wstring& value)
{
  const auto add =
    keys.emplace (std::make_pair (key, value));

  if (add.second)
  {
    ordered_keys.emplace_back (key);

    if (parent != nullptr)
    {   parent->crc32_ = 0x0;

      SK_LOG2 ( ( L"Forced INI Flush: iSK_INISection::add_key_value (%ws, %ws)",
                                                 key.c_str (), value.c_str () ),
                  L"ConfigMgmt" );
    }
  }

  else
  {
    if (! value._Equal (add.first->second))
    {
      // Implicit Flush is not needed
      //
      ///if (parent != nullptr)
      ///    parent->crc32_ = 0x0;

      add.first->second =
        value;
    }
  }
}

__declspec(nothrow)
bool
__stdcall
iSK_INI::contains_section (const wchar_t* section)
{
  return
    contains_section (std::wstring_view (section));
}

__declspec(nothrow)
bool
__stdcall
iSK_INI::contains_section (const std::wstring_view section)
{
  return
    ( sections.find (section.data ()) !=
      sections.cend (               ) );
}

__declspec(nothrow)
iSK_INISection*
__stdcall
iSK_INI::contains_section (const std::wstring& section)
{
  const auto _sec =
    sections.find (section);

  if (_sec != sections.cend ())
    return &_sec->second;

  return nullptr;
}

__declspec(nothrow)
iSK_INISection&
__stdcall
iSK_INI::get_section (const wchar_t* section)
{
  return
    get_section (std::wstring_view (section));
}

__declspec(nothrow)
iSK_INISection&
__stdcall
iSK_INI::get_section (const std::wstring_view section)
{
  std::wstring wstr (section);

  bool try_emplace =
    ( sections.find (wstr) ==
      sections.cend (    ) );

  iSK_INISection& ret =
    sections [wstr];

  if (try_emplace)
  {
                                    ret.name = std::move (wstr);
    ordered_sections.emplace_back (&ret);;
  }

  return ret;
}

__declspec(nothrow)
iSK_INISection&
__stdcall
iSK_INI::get_section (const std::wstring& section)
{
  bool try_emplace =
    ( sections.find (section) ==
      sections.cend (       ) );

  iSK_INISection& ret =
    sections [section];

  if (try_emplace)
  {
                                    ret.name = section;
    ordered_sections.emplace_back (&ret);
  }

  return ret;
}


__declspec(nothrow)
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

    SK_ReleaseAssert (len <= 127);
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
    ordered_sections.emplace_back (&ret);
  }

  return ret;
}

__declspec(nothrow)
void
__stdcall
iSK_INI::write (const wchar_t* fname)
{
  SK_ScopedLocale _locale (L"en_us.utf8");

  if (ordered_sections.empty () && !allow_empty)
    return;

  if (fname == nullptr)
    fname = name.c_str ();

  // Do NOT overwrite default files
  if (StrStrIW (fname, LR"(\default_)"))
    return;

  std::wstring outbuf;
               outbuf.reserve (65536);

  for ( auto& it : ordered_sections )
  {
    iSK_INISection& section = *it;

    if ( (! section.name.empty         ()) &&
         (! section.ordered_keys.empty ()) )
    {
      // Special value to defer the removal of sections until
      //   INI write, rather than while the INI is in use.
      if (section.name._Equal (L"Invalid.Section.DoNotFlush"))
        continue;

      outbuf +=
        SK_FormatStringW (
          L"[%ws]\n", section.name.c_str ()
        );

      for ( auto& key_it : section.ordered_keys )
      {
        if (key_it.empty ())
          continue;

        const std::wstring& val =
          section.get_cvalue (key_it);

        outbuf +=
          val.empty ()
           ? SK_FormatStringW ( L"%ws=\n",    key_it.c_str () )
           : SK_FormatStringW ( L"%ws=%ws\n", key_it.c_str (),
                                                 val.c_str () );
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

  else if (outbuf.empty () && !allow_empty)
    return;


  uint32_t new_crc32 = 0;
  bool     orig_file =
    (0 == _wcsicmp (fname, get_filename ()));

  if (orig_file)
  {
    new_crc32 =
        crc32 ( 0x0, outbuf.data   (),
                     outbuf.length () * sizeof (wchar_t) );

    //dll_log->Log (L"%s => Old: %x, New: %x", fname, crc32_, new_crc32);

    if (new_crc32 == crc32_)
    {
      if (ReadAcquire (&__SK_DLL_Ending))
      {
        //if (WaitForSingleObject (   file_watch, 0) == WAIT_TIMEOUT)
        //  return;
        //
        //FindNextChangeNotification (file_watch);

        if (SK_File_GetModificationTime (fname, &file_stamp))
        {
          // Check if file was re-written externally, if not we can avoid flushing to disk.
          if (CompareFileTime (&flushed_, &file_stamp) == 0 || (! SK_GetFramesDrawn ()))
          {                                                 // Also avoid writing INI files when launchers exit
            SK_LOG2 ((L"Flush Skipped For Unmodified INI '%s' (Shutdown)", fname),
                     L"ConfigMgmt");
            return;
          }
        }
      }

      else
      {
        SK_LOG2 ( ( L"Flush Skipped For Unmodified INI '%s' (Normal)", fname ),
                    L"ConfigMgmt" );
        return;
      }
    }
  }

  SK_CreateDirectories (fname);

  if (StrStrIW (fname, LR"(\Global\)"))
    encoding_ = INI_UTF16LE;

  FILE* fOut = nullptr;

  switch (encoding_)
  {
    case INI_UTF8:
    case INI_UTF8NOBOM:
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

  // Erase the Byte-Order-Marker that Windows adds
  if (encoding_ == INI_UTF8NOBOM)
    fseek (fOut, 0L, SEEK_SET);

  fputws (outbuf.c_str (), fOut);
  fclose (fOut);

  if (orig_file)
  {
    crc32_ = new_crc32;

    if (SK_File_GetModificationTime (fname, &file_stamp))
    {
      flushed_ = file_stamp;
    }
  }
}


__declspec(nothrow)
iSK_INI::_TSectionMap&
__stdcall
iSK_INI::get_sections (void)
{
  return sections;
}


__declspec(nothrow)
HRESULT
__stdcall
iSK_INI::QueryInterface (THIS_ REFIID riid, void** ppvObj)
{
  if (ppvObj == nullptr)
    return E_POINTER;

  if (IsEqualGUID (riid, IID_SK_INI) != 0)
  {
    AddRef ();

    *ppvObj = this;

    return S_OK;
  }

  return E_NOTIMPL;
}

__declspec(nothrow)
ULONG
__stdcall
iSK_INI::AddRef (THIS)
{
  return InterlockedIncrement (&refs_);
}

__declspec(nothrow)
ULONG
__stdcall
iSK_INI::Release (THIS)
{
  return InterlockedDecrement (&refs_);
}

__declspec(nothrow)
bool
__stdcall
iSK_INI::remove_section (const wchar_t* wszSection)
{
  return
    remove_section (std::wstring_view (wszSection));
}

__declspec(nothrow)
bool
__stdcall
iSK_INI::remove_section (const std::wstring_view wszSection)
{
  const std::wstring wstr (wszSection.data ());

  return
    remove_section (wstr);
}

__declspec(nothrow)
bool
__stdcall
iSK_INI::remove_section (const std::wstring& wszSection)
{
  const auto it =
    sections.find (wszSection);

  if (it != sections.cend ())
  {
    sections.erase (it);

    const auto ordered_it =
      std::find ( ordered_sections.begin (),
                  ordered_sections.end   (), &(it->second) );

    if (ordered_it != ordered_sections.cend ())
                      ordered_sections.erase (ordered_it);

    return true;
  }

  return false;
}

__declspec(nothrow)
bool
__stdcall
iSK_INISection::remove_key (const wchar_t* wszKey)
{
  return
    remove_key (std::wstring_view (wszKey));
}

__declspec(nothrow)
bool
__stdcall
iSK_INISection::remove_key (const std::wstring_view wszKey)
{
  return
    remove_key (std::wstring (wszKey));
}

__declspec(nothrow)
bool
__stdcall
iSK_INISection::remove_key (const std::wstring& wszKey)
{
  auto it =
    keys.find (wszKey);

  if (it != keys.end ())
  {
    keys.erase (it);

    const auto ordered_it =
      std::find ( ordered_keys.begin (),
                  ordered_keys.end   (), wszKey );

    if (ordered_it != ordered_keys.cend ())
                      ordered_keys.erase (ordered_it);

    if (parent != nullptr)
    {   parent->crc32_ = 0x0;

        SK_LOG2 ( ( L"Forced INI Flush: iSK_INISection::remove_key (%ws)", wszKey.c_str () ),
                    L"ConfigMgmt" );
    }

    return true;
  }

  return false;
}

bool&
iSK_INI::get_allow_empty (void)
{
  return
    allow_empty;
}


__declspec(nothrow)
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

__declspec(nothrow)
ULONG
__stdcall
iSK_INISection::AddRef (THIS)
{
  return InterlockedIncrement (&refs);
}

__declspec(nothrow)
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


__declspec(nothrow)
const wchar_t*
iSK_INI::get_filename (void) const
{
  return
    name.c_str ();
}

__declspec(nothrow)
bool
iSK_INI::import_file (const wchar_t* fname)
{
  SK_ScopedLocale _locale (L"en_us.utf8");

  // Invalid filename
  if (fname == nullptr || *fname == L'\0')
    return false;

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  if (pTLS == nullptr)
    return false;

  wchar_t* wszImportData = nullptr;
  FILE*    fImportINI    = nullptr;

  TRY_FILE_IO (_wfsopen (fname, L"rbS", _SH_DENYNO),
                         fname, fImportINI);

  if (fImportINI != nullptr)
  {
    auto size =
      sk::narrow_cast <int> (
        SK_File_GetSize (fname)
      );

    wszImportData =
      pTLS->scratch_memory->ini.file_buffer.alloc (size + 3, true);

    if (wszImportData == nullptr)
    {
      fclose (fImportINI);
      return false;
    }

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
      SK_LOG0 ( ( L"Encountered Byte-Swapped Unicode INI "
                  L"file ('%s'), attempting to recover...",
                                                   fname ),
                  L"INI Parser" );

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
      bool utf8_bom =
        (reinterpret_cast <unsigned char *> (wszImportData)) [0] == 0xEF &&
        (reinterpret_cast <unsigned char *> (wszImportData)) [1] == 0xBB &&
        (reinterpret_cast <unsigned char *> (wszImportData)) [2] == 0xBF;

      const uintptr_t offset =
           utf8_bom ? 3 : 0;

      const int      real_size  =
        size - sk::narrow_cast <int> (offset);

      char*          start_addr =
      (reinterpret_cast <char *> (wszImportData)) + offset;

      char* string =
        pTLS->scratch_memory->ini.utf8_string.alloc (real_size + 2, true);

      if (string == nullptr)
      {
        return false; // Out of Memory
      }

      memcpy (string, start_addr, real_size);

      const int converted_size =
        MultiByteToWideChar ( CP_UTF8, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
                                               string, real_size, nullptr, 0 );

      if (0 == converted_size)
      {
        SK_ReleaseAssert (GetLastError () != ERROR_NO_UNICODE_TRANSLATION);

        if (real_size > 0)
        {
          SK_LOG0 ( ( L"Could not convert UTF-8 / ANSI "
                      L"Encoded .ini file ('%s') to UTF-16, aborting!",
                                                               fname ),
                      L"INI Parser" );
        }

        return false;
      }

      if (converted_size + 2 > size + 3)
      {
        wszImportData =
          pTLS->scratch_memory->ini.file_buffer.alloc (converted_size + 2, true);
      }

      SK_ReleaseAssert (
          wszImportData != nullptr);
      if (wszImportData != nullptr)
      {
        MultiByteToWideChar ( CP_UTF8, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
                                                           string, real_size,
                                               wszImportData, converted_size );

        SK_ReleaseAssert (GetLastError () != ERROR_NO_UNICODE_TRANSLATION);  

        //SK_LOG0 ( ( L"Converted UTF-8 INI File: '%s'",
                                      //wszImportName ), L"INI Parser" );
      }

      else
        return false;
    }

    // Don't ever recursively call this, wszImportData is thread-local
    import (wszImportData);

    return true;
  }

  return false;
}

__declspec(nothrow)
bool
iSK_INI::rename (const wchar_t* fname)
{
  if (  fname != nullptr &&
       *fname == L'\0' )
  {
    name = fname;

    crc32_     =   0  ;
    flushed_   = { 0 };
    file_stamp = { 0 };

    return true;
  }

  return false;
}

iSK_INI::CharacterEncoding
iSK_INI::get_encoding (void) const
{
  return encoding_;
}

bool
iSK_INI::set_encoding (CharacterEncoding encoding)
{
  if ( encoding >= INI_UTF8 &&
       encoding <= INI_UTF8NOBOM )
  {
    encoding_ = encoding;
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