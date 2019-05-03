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

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef __SK__SPECIALK_H__
#define __SK__SPECIALK_H__

struct IUnknown;
#include <Unknwnbase.h>

#ifndef SK_STATIC_LIB
# ifdef SK_BUILD_DLL
#  define SK_PUBLIC_API _declspec (dllexport) __stdcall 
# else
#  define SK_PUBLIC_API _declspec (dllimport) __stdcall 
# endif
#else
# define  SK_PUBLIC_API __stdcall 
#endif

#ifdef __cplusplus
# define     SK_INCLUDE_START(Source) __pragma (once) extern "C" {
# define SK_INCLUDE_START_CPP(Source) __pragma (once)

# define     SK_INCLUDE_END(Source)   };
# define SK_INCLUDE_END_CPP(Source)
#else
# define     SK_INCLUDE_START(Source) __pragma (once)
# define SK_INCLUDE_START_CPP(Source) message ("C++ compiler required.")

# define     SK_INCLUDE_END(Source)   0;
# define SK_INCLUDE_END_CPP(Source)   message ("C++ compiler required.")
#endif


#if (! (defined (_MSC_VER) && (_MSC_VER >= 1020)))
# define     SK_INCLUDE_START(Source) message ("Newer compiler required.")
# define SK_INCLUDE_START_CPP(Source) message ("Newer compiler required.")
# define       SK_INCLUDE_END(Source) message ("Newer compiler required.")
# define   SK_INCLUDE_END_CPP(Source) message ("Newer compiler required.")
#endif

#define SK_GetVersionStr SK_GetVersionStrW
const wchar_t* __stdcall SK_GetVersionStrW (void);
const char*    __stdcall SK_GetVersionStrA (void);

#endif /* __SK__SPECIALK_H__ */