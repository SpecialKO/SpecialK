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
#include <cinttypes>

class skMemCmd : public SK_ICommand {
public:
  SK_ICommandResult execute (const char* szArgs) override;

  int getNumArgs         (void) noexcept override { return 2; }
  int getNumOptionalArgs (void) noexcept override { return 1; }
  int getNumRequiredArgs (void) noexcept override {
    return getNumArgs () - getNumOptionalArgs ();
  }

protected:
private:
};

SK_ICommandResult
skMemCmd::execute (const char* szArgs)
{
  if (szArgs == nullptr)
    return SK_ICommandResult ("mem", szArgs);

  uintptr_t addr      =  0;
  char      type      =  0;
  char      val [256] = { };

  sscanf (szArgs, "%c %" SCNxPTR " %255s", &type, &addr, val);

  static uint8_t* base_addr = nullptr;

  if (base_addr == nullptr)
  {
    base_addr = reinterpret_cast <uint8_t *> (GetModuleHandle (nullptr));

    MEMORY_BASIC_INFORMATION basic_mem_info;
    VirtualQuery (base_addr, &basic_mem_info, sizeof basic_mem_info);

    base_addr = static_cast <uint8_t *> (basic_mem_info.BaseAddress);
  }

  addr += reinterpret_cast <uintptr_t> (base_addr);

  char result [512] = { };

  switch (type)
  {
    case 'b':
      if (strlen (val))
      {
        DWORD dwOld;

        VirtualProtect (reinterpret_cast <LPVOID> (addr), 1, PAGE_EXECUTE_READWRITE, &dwOld);
          char out;
          sscanf (val, "%cx", &out);
          *reinterpret_cast <uint8_t *> (addr) = out;
        VirtualProtect (reinterpret_cast <LPVOID> (addr), 1, dwOld, &dwOld);
      }

      sprintf (result, "%u", *reinterpret_cast <uint8_t *> (addr));

      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
    case 's':
      if (strlen (val))
      {
        DWORD dwOld;

        VirtualProtect (reinterpret_cast <LPVOID> (addr), 2, PAGE_EXECUTE_READWRITE, &dwOld);
          uint16_t out;
          sscanf (val, "%hx", &out);
          *reinterpret_cast <uint16_t *> (addr) = out;
        VirtualProtect (reinterpret_cast <LPVOID> (addr), 2, dwOld, &dwOld);
      }

      sprintf (result, "%u", *reinterpret_cast <uint16_t *> (addr));
      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
    case 'i':
      if (strlen (val))
      {
        DWORD dwOld;

        VirtualProtect (reinterpret_cast <LPVOID> (addr), 4, PAGE_EXECUTE_READWRITE, &dwOld);
          uint32_t out;
          sscanf (val, "%x", &out);
          *reinterpret_cast <uint32_t *> (addr) = out;
        VirtualProtect (reinterpret_cast <LPVOID> (addr), 4, dwOld, &dwOld);
      }

      sprintf (result, "%u", *reinterpret_cast <uint32_t *> (addr));
      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
    case 'l':
      if (strlen (val))
      {
        DWORD dwOld;

        VirtualProtect (reinterpret_cast <LPVOID> (addr), 8, PAGE_EXECUTE_READWRITE, &dwOld);
          uint64_t out;
          sscanf (val, "%llx", &out);
          *reinterpret_cast <uint64_t *> (addr) = out;
        VirtualProtect (reinterpret_cast <LPVOID> (addr), 8, dwOld, &dwOld);
      }

      sprintf (result, "%llu", *reinterpret_cast <uint64_t *> (addr));
      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
    case 'd':
      if (strlen (val))
      {
        DWORD dwOld;

        VirtualProtect (reinterpret_cast <LPVOID> (addr), 8, PAGE_EXECUTE_READWRITE, &dwOld);
          double out;
          sscanf (val, "%lf", &out);
          *reinterpret_cast <double *> (addr) = out;
        VirtualProtect (reinterpret_cast <LPVOID> (addr), 8, dwOld, &dwOld);
      }

      sprintf (result, "%f", *reinterpret_cast <double *> (addr));
      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
    case 'f':
      if (strlen (val))
      {
        DWORD dwOld;

        VirtualProtect (reinterpret_cast <LPVOID> (addr), 4, PAGE_EXECUTE_READWRITE, &dwOld);
          float out;
          sscanf (val, "%f", &out);
          *reinterpret_cast <float *> (addr) = out;
        VirtualProtect (reinterpret_cast <LPVOID> (addr), 4, dwOld, &dwOld);
      }

      sprintf (result, "%f", *reinterpret_cast <float *> (addr));
      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
    case 't':
      if (strlen (val))
      {
        DWORD dwOld;

        VirtualProtect (reinterpret_cast <LPVOID> (addr), 256, PAGE_EXECUTE_READWRITE, &dwOld);
          strcpy (reinterpret_cast <char *> (addr), val);
        VirtualProtect (reinterpret_cast <LPVOID> (addr), 256, dwOld, &dwOld);
      }
      sprintf (result, "%s", reinterpret_cast <char *> (addr));
      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;

    default:
      SK_ReleaseAssert (! L"Unknown Memory Type");
      break;
  }

  return SK_ICommandResult ("mem", szArgs);
}