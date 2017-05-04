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
#ifndef __SK__Update__Version_H__
#define __SK__Update__Version_H__

extern bool
__stdcall
SK_FetchVersionInfo1 (const wchar_t* wszProduct = L"SpecialK", bool force = false);

extern bool
__stdcall
SK_FetchVersionInfo (const wchar_t* wszProduct = L"SpecialK");

#include <string>

struct SK_VersionInfo_V1
{
  std::wstring branch;
  std::wstring package;
  int          build;
};

typedef SK_VersionInfo_V1 SK_VersionInfo;

SK_VersionInfo SK_Version_GetLatestInfo_V1 (const wchar_t* wszProduct);
SK_VersionInfo SK_Version_GetLocalInfo_V1  (const wchar_t* wszProduct);

#define SK_Version_GetLatestInfo SK_Version_GetLatestInfo_V1
#define SK_Version_GetLocalInfo  SK_Version_GetLocalInfo_V1

std::vector <std::string>
SK_Version_GetAvailableBranches (const wchar_t* wszProduct);

bool
SK_Version_SwitchBranches       (const wchar_t* wszProduct, const char* szBranch);

std::wstring SK_Version_GetLastCheckTime_WStr (void);

#endif /* __SK__Update_Version_H__ */