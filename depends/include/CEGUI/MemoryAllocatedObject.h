/***********************************************************************
	created:	28/10/2010
	author:		Martin Preisler (inspired by Ogre3D)

	purpose:	Overrides new an delete operators and uses given Allocator
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
#ifndef _CEGUIMemoryAllocatedObject_h_
#define _CEGUIMemoryAllocatedObject_h_

#ifndef _CEGUIMemoryAllocation_h_
#   error Dont include this directly! Include CEGUIBase.h instead.
#endif

namespace CEGUI
{
 
#ifdef CEGUI_CUSTOM_ALLOCATORS

/*!
\brief
    Defines a 'allocated object' class

    This is used to allocate CEGUI classes via custom allocators. It's
    basically a wrapper that calls given Allocator when new/delete is called.
    This is managed via overloading of new and delete operators.
*/
template <typename Class>
class AllocatedObject
{
public:
    typedef typename AllocatorConfig<Class>::Allocator Allocator;

	inline explicit AllocatedObject()
	{}

#ifndef CEGUI_CUSTOM_ALLOCATORS_DEBUG
	inline void* operator new(size_t size)
	{
		return Allocator::allocateBytes(size);
	}
#else
	inline void* operator new(size_t size, const char* file, int line, const char* func)
	{
		return Allocator::allocateBytes(size, file, line, func);
	}
#endif

	inline void operator delete(void* ptr)
	{
		Allocator::deallocateBytes(ptr);
	}

#ifndef CEGUI_CUSTOM_ALLOCATORS_DEBUG
	inline void* operator new[] (size_t size)
	{
		return Allocator::allocateBytes(size);
	}
#else
	inline void* operator new[] (size_t size, const char* file, int line, const char* func)
	{
		return Allocator::allocateBytes(size, file, line, func);
	}
#endif

	inline void operator delete[] (void* ptr)
	{
		Allocator::deallocateBytes(ptr);
	}

    // todo: does debug variant even make sense with placement new?
	inline void* operator new(size_t size, void* ptr)
	{
		(void) size;
		return ptr;
	}

    inline void operator delete(void* ptr, void*)
	{
		Allocator::deallocateBytes(ptr);
	}
};

#else

// allocated object is just a stub template class if custom memory allocators aren't used
template<typename Allocator>
class AllocatedObject
{
public:
    inline explicit AllocatedObject()
	{}
};

#endif

}

#endif	// end of guard _CEGUIMemoryAllocatedObject_h_
