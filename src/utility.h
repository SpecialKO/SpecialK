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

#ifndef __SK__UTILITY_H__
#define __SK__UTILITY_H__

#include <string>

std::wstring  SK_GetDocumentsDir      (void);
bool          SK_GetUserProfileDir    (wchar_t* buf, uint32_t* pdwLen);
bool          SK_IsTrue               (const wchar_t* string);
int           SK_MessageBox           (std::wstring caption,
                                        std::wstring title,
                                        uint32_t     flags);

void          SK_SetNormalFileAttribs (std::wstring file);

#endif /* __SK__UTILITY_H__ */