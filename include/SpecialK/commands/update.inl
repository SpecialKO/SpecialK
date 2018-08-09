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

#pragma once

#include <SpecialK/command.h>

#include <SpecialK/update/version.h>
#include <SpecialK/update/network.h>

class skUpdateCmd : public SK_ICommand {
public:
  virtual SK_ICommandResult execute (const char* szArgs)  override;

  virtual int getNumArgs         (void)  override { return 1; }
  virtual int getNumOptionalArgs (void)  override { return 1; }
  virtual int getNumRequiredArgs (void)  override {
    return getNumArgs () - getNumOptionalArgs ();
  }

protected:
private:
};

SK_ICommandResult
skUpdateCmd::execute (const char* szArgs) 
{
  if (! strlen (szArgs))
  {
    SK_FetchVersionInfo1 (L"SpecialK", true);
    SK_UpdateSoftware1   (L"SpecialK", true);
  }

  else
  {
    wchar_t wszProduct [128] = { };

    mbtowc (wszProduct, szArgs, strlen (szArgs));

    SK_FetchVersionInfo1 (wszProduct, true);
    SK_UpdateSoftware1   (wszProduct, true);
  }

  return SK_ICommandResult ("Manual update initiated...", szArgs);
}