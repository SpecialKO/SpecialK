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
#include <SpecialK/framerate.h>

class skLimitResetCmd : public SK_ICommand {
public:
  SK_ICommandResult execute (const char* szArgs) override;

  int getNumArgs         (void) noexcept override { return 0; }
  int getNumOptionalArgs (void) noexcept override { return 0; }
  int getNumRequiredArgs (void) noexcept override {
    return getNumArgs () - getNumOptionalArgs ();
  }

protected:
private:
};

SK_ICommandResult
skLimitResetCmd::execute (const char* szArgs)
{
  SK::Framerate::Limiter *pLimiter =
    SK::Framerate::GetLimiter ();

  if (pLimiter != nullptr)
      pLimiter->reset (true);

  return
    SK_ICommandResult ("Framerate Limiter Reset...", szArgs);
}
