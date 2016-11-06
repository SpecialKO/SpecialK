/***********************************************************************
	created:	22/2/2004
	author:		Paul D Turner

	purpose:	Singleton Base Class
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
/*************************************************************************

	The code in this file is taken from article 1.3 in the the book:
	Game Programming Gems from Charles River Media

*************************************************************************/
#ifndef _CEGUISingleton_h_
#define _CEGUISingleton_h_

#include "CEGUI/Base.h"
#include <cassert>

// Start of CEGUI namespace section
namespace CEGUI
{
/* Copyright (C) Scott Bilas, 2000.
 * All rights reserved worldwide.
 *
 * This software is provided "as is" without express or implied
 * warranties. You may freely copy and compile this source into
 * applications you distribute provided that the copyright text
 * below is included in the resulting source code, for example:
 * "Portions Copyright (C) Scott Bilas, 2000"
 */


template <typename T> class CEGUIEXPORT Singleton
{
protected:
// TODO: Come up with something better than this!
// TODO:
// TODO: This super-nasty piece of nastiness was put in for continued
// TODO: compatability with MSVC++ and MinGW - the latter apparently
// TODO: needs this.
    static
#ifdef __MINGW32__
    CEGUIEXPORT
#endif
    T* ms_Singleton;

public:
    Singleton( void )
    {
        assert( !ms_Singleton );
        ms_Singleton = static_cast<T*>(this);
    }
   ~Singleton( void )
        {  assert( ms_Singleton );  ms_Singleton = 0;  }
    static T& getSingleton    ( void )
        {  assert( ms_Singleton );  return ( *ms_Singleton );  }
    static T* getSingletonPtr ( void )
        {  return ( ms_Singleton );  }

    static T& getDllSingleton    ( void );
    static T* getDllSingletonPtr ( void );

private:
    Singleton& operator= (const Singleton&) { return *this; }
    Singleton            (const Singleton&) {               }
};

} // End of  CEGUI namespace section



#endif	// end of guard _CEGUISingleton_h_
