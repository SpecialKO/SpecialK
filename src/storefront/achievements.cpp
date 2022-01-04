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
#include <SpecialK/storefront/achievements.h>
#include <SpecialK/resource.h>

void SK_AchievementManager::loadSound (const wchar_t *wszUnlockSound)
{
  if (! wszUnlockSound) // Try again, stupid
    return;

  bool xbox = false,
       psn  = false,
       dt   = false;

  wchar_t wszFileName [MAX_PATH + 2] = { };

  extern iSK_INI* steam_ini;

  if (*wszUnlockSound == L'\0' && steam_ini != nullptr)
  {
    // If the config file is empty, establish defaults and then write it.
    if (steam_ini->get_sections ().empty ())
    {
      steam_ini->import ( L"[Steam.Achievements]\n"
                          L"SoundFile=psn\n"
                          L"PlaySound=true\n"
                          L"TakeScreenshot=false\n"
                          L"AnimatePopup=true\n"
                          L"NotifyCorner=0\n" );

      steam_ini->write ();
    }

    if (steam_ini->contains_section (L"Steam.Achievements"))
    {
      iSK_INISection& sec =
        steam_ini->get_section (L"Steam.Achievements");

      if (sec.contains_key (L"SoundFile"))
      {
        wcsncpy_s ( wszFileName,   MAX_PATH,
                    sec.get_value (L"SoundFile").c_str (),
                                   _TRUNCATE );
      }
    }
  }

  else
  {
    wcsncpy_s ( wszFileName,    MAX_PATH,
                wszUnlockSound, _TRUNCATE );
  }

  if ((!      _wcsnicmp (wszFileName, L"psn", MAX_PATH)))
    psn  = true;
  else if  (! _wcsnicmp (wszFileName, L"xbox", MAX_PATH))
    xbox = true;
  else if ((! _wcsnicmp (wszFileName, L"dream_theater", MAX_PATH)))
    dt   = true;

  FILE *fWAV = nullptr;

  if ( (!  psn) &&
       (! xbox) &&
       (!   dt) && (fWAV = _wfopen (wszFileName, L"rb")) != nullptr )
  {
    SK_ConcealUserDir (wszFileName);

    steam_log->LogEx ( true,
                        L"  >> Loading Achievement Unlock Sound: '%s'...",
                          wszFileName );

                fseek (fWAV, 0, SEEK_END);
    long size = ftell (fWAV);
               rewind (fWAV);

    unlock_sound =
      static_cast <uint8_t *> (
        malloc (size)
      );

    if (unlock_sound != nullptr)
      fread  (unlock_sound, size, 1, fWAV);

    fclose (fWAV);

    steam_log->LogEx (false, L" %d bytes\n", size);

    default_loaded = false;
  }

  else
  {
    // Default to PSN if not specified
    if ((! psn) && (! xbox) && (! dt))
      psn = true;

    steam_log->Log ( L"  * Loading Built-In Achievement Unlock Sound: '%s'",
                       wszFileName );

    HRSRC default_sound = nullptr;

    if (psn)
      default_sound =
      FindResource (SK_GetDLL (), MAKEINTRESOURCE (IDR_TROPHY),        L"WAVE");
    else if (xbox)
      default_sound =
      FindResource (SK_GetDLL (), MAKEINTRESOURCE (IDR_XBOX),          L"WAVE");
    else
      default_sound =
      FindResource (SK_GetDLL (), MAKEINTRESOURCE (IDR_DREAM_THEATER), L"WAVE");

    if (default_sound != nullptr)
    {
      HGLOBAL sound_ref     =
        LoadResource (SK_GetDLL (), default_sound);

      if (sound_ref != nullptr)
      {
        unlock_sound        =
          static_cast <uint8_t *> (LockResource (sound_ref));

        default_loaded = true;
      }
    }
  }
};