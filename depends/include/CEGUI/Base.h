/***********************************************************************
	created:	20/2/2004
	author:		Paul D Turner

	purpose:	Base include used within the system
				This contains various lower level bits required
				by other parts of the system.  All other library
				headers will include this file.
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2006 Paul D Turner & The CEGUI Development Team
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this software and associated documentation files (the
 *   "Software"), to deal in the Software without restriction, including
 *   without limitation the rights to use, copy, modify, merge, publish,
 *   distribute, sublicense, and/or sell copies of the Software, and to
 *   permit persons to whom the Software is furnished to do so, subject to
 *   the following conditions:
 *
 *   The above copyright notice and this permission notice shall be
 *   included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *   OTHER DEALINGS IN THE SOFTWARE.
 ***************************************************************************/
#ifndef _CEGUIBase_h_
#define _CEGUIBase_h_

// bring in configuration options
#include "CEGUI/Config.h"

// add CEGUI version defines
#include "CEGUI/Version.h"

#include <cassert>
#include <algorithm>

/*************************************************************************
	Dynamic Library import / export control conditional
	(Define CEGUIBASE_EXPORTS to export symbols, else they are imported)
*************************************************************************/
#if (defined( __WIN32__ ) || defined( _WIN32 )) && !defined(CEGUI_STATIC)
#   ifdef CEGUIBASE_EXPORTS
#       define CEGUIEXPORT __declspec(dllexport)
#   else
#       define CEGUIEXPORT __declspec(dllimport)
#   endif
#       define CEGUIPRIVATE
#else
#       define CEGUIEXPORT
#       define CEGUIPRIVATE
#endif


// totally kill this warning (debug info truncated to 255 chars etc...) on <= VC6
#if defined(_MSC_VER) && (_MSC_VER <= 1200)
#   pragma warning(disable : 4786)
#endif


// Detect macros for min / max and undefine (with a warning where possible)
#if defined(max)
#   if defined(_MSC_VER)
#       pragma message("Macro definition of max detected - undefining")
#   elif defined (__GNUC__)
#       warning ("Macro definition of max detected - undefining")
#   endif
#   undef max
#endif
#if defined(min)
#   if defined(_MSC_VER)
#       pragma message("Macro definition of min detected - undefining")
#   elif defined (__GNUC__)
#       warning ("Macro definition of min detected - undefining")
#   endif
#   undef min
#endif


// include this to see if it defines _STLPORT_VERION
#	include <string>

// fix to undefine _STLP_DEBUG if STLport is not actually being used
// (resolves some unresolved externals concerning boost)
#if defined(_STLP_DEBUG) && defined(_MSC_VER) && (_MSC_VER >= 1200)
#   if !defined(_STLPORT_VERSION)
#       undef _STLP_DEBUG
#   endif
#endif

// The following defines macros used within CEGUI for std::min/std::max
// usage, and is done as a compatibility measure for VC6 with native STL.
#if defined(_MSC_VER) && (_MSC_VER <= 1200) && !defined(_STLPORT_VERSION)
#    define ceguimin	std::_cpp_min
#    define ceguimax	std::_cpp_max
#else
#    define ceguimin	std::min
#    define ceguimax	std::max
#endif

// CEGUI's Exception macros
// This provides a mechanism to override how exception handling is used.  Note
// that in general this facility _should not be used_.  Attempts to use this
// to disable exceptions to 'make things easier' are doomed to failure.  CEGUI
// becomes less robust without exceptions (because they are used internally by
// CEGUI).  In addition, overriding the exception mechanism will also cause
// memory leaks in various places.  This is your only warning about such things,
// if you decide to continue anyway you hereby waive any right to complain :-p
#ifndef CEGUI_TRY
#   define CEGUI_TRY try
#endif
#ifndef CEGUI_CATCH
#   define CEGUI_CATCH(e) catch (e)
#endif
#ifndef CEGUI_THROW
#   define CEGUI_THROW(e) throw e
#endif
#ifndef CEGUI_RETHROW
#   define CEGUI_RETHROW throw
#endif

// CEGUI_FUNCTION_NAME - CEGUI::String containing current function name
// in the best form we can get it
#if defined(_MSC_VER)
#   define  CEGUI_FUNCTION_NAME CEGUI::String(__FUNCSIG__)
#elif defined(__GNUC__)
#   define  CEGUI_FUNCTION_NAME CEGUI::String(__PRETTY_FUNCTION__)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#   define  CEGUI_FUNCTION_NAME CEGUI::String(__func__)
#else
#   define  CEGUI_FUNCTION_NAME CEGUI::String("[Function name unavailable]")
#endif

//! Prevent an "unused parameter/variable" warning.
#define CEGUI_UNUSED(var) (static_cast<void>(var))

/*************************************************************************
	Documentation for the CEGUI namespace itself
*************************************************************************/
/*!
\brief
	Main namespace for Crazy Eddie's GUI Library

	The CEGUI namespace contains all the classes and other items that comprise the core
	of Crazy Eddie's GUI system.
*/
namespace CEGUI
{

/*************************************************************************
	Simplification of some 'unsigned' types
*************************************************************************/
typedef	unsigned long	ulong;
typedef unsigned short	ushort;
typedef unsigned int	uint;
typedef unsigned char	uchar;

typedef long long    int64;
typedef int          int32;
typedef short        int16;
typedef signed char  int8;

typedef unsigned long long  uint64;
typedef unsigned int        uint32;
typedef unsigned short      uint16;
typedef unsigned char       uint8;


/*************************************************************************
	System wide constants
*************************************************************************/
static const float		DefaultNativeHorzRes	= 640.0f;		//!< Default native horizontal resolution (for fonts and imagesets)
static const float		DefaultNativeVertRes	= 480.0f;		//!< Default native vertical resolution (for fonts and imagesets)


/*************************************************************************
    Additional typedefs
*************************************************************************/
typedef std::ostream OutStream;     //!< Output stream class.
}  // end of CEGUI namespace section

// improve readability - http://www.parashift.com/c++-faq-lite/pointers-to-members.html#faq-33.6
#define CEGUI_CALL_MEMBER_FN(object, ptrToMember) ((object).*(ptrToMember)) 

/*************************************************************************
	Bring in forward references to all GUI base system classes
*************************************************************************/
#include "CEGUI/ForwardRefs.h"
#include "CEGUI/MemoryAllocation.h"

#endif	// end of guard _CEGUIBase_h_
