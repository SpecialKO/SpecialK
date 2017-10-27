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

#include <SpecialK/injection/address_cache.h>
#include <SpecialK/ini.h>
#include <SpecialK/utility.h>

SK_Inject_AddressCacheRegistry::SK_Inject_AddressCacheRegistry (void)
{
  auto injection_config =
    SK_GetDocumentsDir () + L"\\My Mods\\SpecialK\\Global\\injection.ini";

  address_ini_ =
    std::shared_ptr <iSK_INI> (
      SK_CreateINI (injection_config.c_str ())
    );
}

SK_Inject_AddressCacheRegistry::~SK_Inject_AddressCacheRegistry (void)
{
  std::wstring export_data = L"";

  for ( auto& it : address_ini_->get_sections () )
  {
    export_data += L"[" + it.first + L"]\n";

    for ( auto& it2 : it.second.ordered_keys )
    {
      export_data += it2 + L"=";

      export_data += ((iSK_INISection &)it.second).get_value (it2.c_str ());
      export_data += L"\n";
    }

    export_data += L"\n";

    address_ini_->remove_section (it.first.c_str ());
  }

  std::wstring fname =
    address_ini_->get_filename ();
    address_ini_.reset ();

  address_ini_ =
    std::shared_ptr <iSK_INI> (
      SK_CreateINI (fname.c_str ())
    );

  address_ini_->import (export_data.c_str ());
  address_ini_->write  (address_ini_->get_filename ());
}

auto SecName =
[] (auto mod_name, auto arch_type)
{
  switch (arch_type)
  {
    case SK_SYS_CPUArch::x64:
    {
      return std::wstring (mod_name) + std::wstring (L".x64");
    } break;

    case SK_SYS_CPUArch::x86:
    {
      return std::wstring (mod_name) + std::wstring (L".x86");
    } break;
  }

  return std::wstring (L"");
};

uintptr_t
SK_Inject_AddressCacheRegistry::getNamedAddress    (const wchar_t* wszModuleName, const char* szSymbolName, SK_SYS_CPUArch arch)
{
  iSK_INISection& ini_sec =
    address_ini_->get_section (SecName (wszModuleName, arch).c_str ());

  void* addr = 0;

  std::swscanf ( ini_sec.get_value (SK_UTF8ToWideChar (szSymbolName).c_str ()).c_str (),
                   L"%p",
                     &addr );

  return
    reinterpret_cast <uintptr_t> (addr);
}

void
SK_Inject_AddressCacheRegistry::storeNamedAddress  (const wchar_t* wszModuleName, const char* szSymbolName, uintptr_t addr, SK_SYS_CPUArch arch)
{
  iSK_INISection& ini_sec =
    address_ini_->get_section (SecName (wszModuleName, arch).c_str ());

  std::wstring wide_name (SK_UTF8ToWideChar (szSymbolName));

  if (ini_sec.contains_key (wide_name.c_str ()))
  {
    ini_sec.get_value (wide_name.c_str ()) =
      SK_FormatStringW (L"%p", addr);
  }

  else
  {
    ini_sec.add_key_value (wide_name.c_str (), SK_FormatStringW (L"%p", addr).c_str ());
  }
}

bool
SK_Inject_AddressCacheRegistry::removeNamedAddress (const wchar_t* wszModuleName, const char* szSymbolName, SK_SYS_CPUArch arch)
{
  iSK_INISection& ini_sec =
    address_ini_->get_section (SecName (wszModuleName, arch).c_str ());

  return ini_sec.remove_key (SK_UTF8ToWideChar (szSymbolName).c_str ());
}

SK_Inject_Address
SK_Inject_AddressCacheRegistry::getAddress         (const wchar_t* wszModuleName, const char* szSymbolName, SK_SYS_CPUArch arch)
{
  iSK_INISection& ini_sec =
    address_ini_->get_section (SecName (wszModuleName, arch).c_str ());

  void* addr = 0;

  std::swscanf ( ini_sec.get_value (SK_UTF8ToWideChar (szSymbolName).c_str ()).c_str (),
                   L"%p",
                     &addr );

  return
    SK_Inject_Address (             wszModuleName,
                 SK_UTF8ToWideChar (szSymbolName),
      reinterpret_cast <uintptr_t> (addr),
                                    arch          );
}

void
SK_Inject_AddressCacheRegistry::storeAddress       (SK_Inject_Address& addr)
{
  storeNamedAddress (
                       addr.module.c_str (),
    SK_WideCharToUTF8 (addr.name).c_str  (),
                       addr.address,
                       addr.arch             );
}

bool
SK_Inject_AddressCacheRegistry::removeAddress      (SK_Inject_Address& addr)
{
  return
    removeNamedAddress (
                         addr.module.c_str (),
      SK_WideCharToUTF8 (addr.name).c_str  (),
                         addr.arch             );
}

SK_Inject_AddressCacheRegistry *SK_Inject_AddressManager = nullptr;