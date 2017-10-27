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

#ifndef __SK__ADDRESS_CACHE_H__
#define __SK__ADDRESS_CACHE_H__

#include <SpecialK/core.h>

#include <string>
#include <memory>

struct iSK_INI;

enum class SK_SYS_CPUArch
{
  x86     = 0x1,
  x64     = 0x2,
  ARM     = 0x4,
  UNKNOWN = 0x0
};

#ifdef _WIN64
const SK_SYS_CPUArch SK_DEFAULT_CPU (SK_SYS_CPUArch::x64);
#else
const SK_SYS_CPUArch SK_DEFAULT_CPU (SK_SYS_CPUArch::x86);
#endif

class SK_Inject_Address
{
public:
  SK_Inject_Address ( std::wstring   parent_mod,
                      std::wstring   symbol,
                      uintptr_t      addr,
                      SK_SYS_CPUArch code_arch = SK_DEFAULT_CPU )
  {
     module  = parent_mod;
     name    = symbol;
     address = addr;
     arch    = code_arch;
  }

  operator void* (void) { return reinterpret_cast <void *>(address); }

//protected:
//private:
  std::wstring   module;
  std::wstring   name;
  SK_SYS_CPUArch arch;
  uintptr_t      address;
};

class SK_Inject_AddressCacheRegistry
{
public:
   SK_Inject_AddressCacheRegistry (void);
  ~SK_Inject_AddressCacheRegistry (void);

  uintptr_t         getNamedAddress    (const wchar_t* wszModuleName, const char* szSymbolName,                 SK_SYS_CPUArch = SK_DEFAULT_CPU);
  void              storeNamedAddress  (const wchar_t* wszModuleName, const char* szSymbolName, uintptr_t addr, SK_SYS_CPUArch = SK_DEFAULT_CPU);
  bool              removeNamedAddress (const wchar_t* wszModuleName, const char* szSymbolName,                 SK_SYS_CPUArch = SK_DEFAULT_CPU);

  SK_Inject_Address getAddress         (const wchar_t* wszModuleName, const char* szSymbolName,                 SK_SYS_CPUArch = SK_DEFAULT_CPU);
  void              storeAddress       (SK_Inject_Address& addr);
  bool              removeAddress      (SK_Inject_Address& addr);

protected:
private:
  std::shared_ptr <iSK_INI> address_ini_;
} extern *SK_Inject_AddressManager;

#endif /* __SK__ADDRESS_CACHE_H__ */
