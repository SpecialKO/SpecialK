/***********************************************************************
	created:	18/10/2010
	author:		Martin Preisler

	purpose:	Implements an allocator that delegates to Ogre
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2010 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIMemoryOgreAllocator_h_
#define _CEGUIMemoryOgreAllocator_h_

#include <OgreMemoryAllocatorConfig.h>

namespace CEGUI
{

// I deliberately kept the interface exactly the same,
// so this will do just fine...

CEGUI_SET_DEFAULT_ALLOCATOR(::Ogre::GeneralAllocPolicy)
CEGUI_SET_ALLOCATOR(System, ::Ogre::GeneralAllocPolicy)

}

#endif	// end of guard _CEGUIMemoryOgreAllocator_h_
