/***********************************************************************
	created:	14/10/2010
	author:		Martin Preisler (inspired by Ogre3D)

	purpose:	Allows custom memory allocators to be used within CEGUI
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
#ifndef _CEGUIMemoryAllocation_h_
#define _CEGUIMemoryAllocation_h_

#ifndef _CEGUIBase_h_
#   error Dont include this directly! Include CEGUIBase.h instead.
#endif

#define CEGUI_SET_DEFAULT_ALLOCATOR(A)\
template<typename T>\
struct AllocatorConfig\
{\
    typedef A Allocator;\
};

#define CEGUI_SET_ALLOCATOR(Class, A)\
template<>\
struct AllocatorConfig<Class>\
{\
    typedef A Allocator;\
};

#ifdef CEGUI_CUSTOM_ALLOCATORS

namespace CEGUI
{

// stub classes uses for allocator configuration
class STLAllocator {};
class BufferAllocator {};

// borrowed from Ogre, used to construct arrays
template<typename T>
T* constructN(T* basePtr, size_t count)
{
	for (size_t i = 0; i < count; ++i)
	{
		new ((void*)(basePtr+i)) T();
	}
	return basePtr;
}

// ogre doesn't do this template but I added it because it works even for types without
// destructors where I was getting syntax errors with just the macro
template<typename T>
void destructN(T* basePtr, size_t count)
{
    // iterate in reverse for consistency with delete []
	for (size_t i = count - 1; i-- > 0;)
	{
		basePtr[i].~T();
    }
}

} // CEGUI namespace

#ifndef CEGUI_CUSTOM_ALLOCATORS_DEBUG
#   define CEGUI_NEW_AO new
#   define CEGUI_DELETE_AO delete
// for primitive types, types not inherited from AllocatedObject
#   define CEGUI_NEW_PT(T, A) new (::CEGUI::AllocatorConfig<A>::Allocator::allocateBytes(sizeof(T))) T
#   define CEGUI_NEW_ARRAY_PT(T, count, A) ::CEGUI::constructN(static_cast<T*>(::CEGUI::AllocatorConfig<A>::Allocator::allocateBytes(sizeof(T)*(count))), count)
#   define CEGUI_DELETE_PT(ptr, T, A) do{if(ptr){(ptr)->~T(); ::CEGUI::AllocatorConfig<A>::Allocator::deallocateBytes((void*)ptr);}}while(0)
#   define CEGUI_DELETE_ARRAY_PT(ptr, T, count, A) do{if(ptr){ ::CEGUI::destructN(static_cast<T*>(ptr), count); ::CEGUI::AllocatorConfig<A>::Allocator::deallocateBytes((void*)ptr);}}while(0)
#else
#   define CEGUI_NEW_AO new(__FILE__, __LINE__, __FUNCTION__)
#   define CEGUI_DELETE_AO delete
// for primitive types, types not inherited from AllocatedObject
#   define CEGUI_NEW_PT(T, A) new (::CEGUI::AllocatorConfig<A>::Allocator::allocateBytes(sizeof(T), __FILE__, __LINE__, __FUNCTION__)) T
#   define CEGUI_NEW_ARRAY_PT(T, count, A) ::CEGUI::constructN(static_cast<T*>(::CEGUI::AllocatorConfig<A>::Allocator::allocateBytes(sizeof(T)*(count), __FILE__, __LINE__, __FUNCTION__)), count)
#   define CEGUI_DELETE_PT(ptr, T, A) do{if(ptr){(ptr)->~T(); ::CEGUI::AllocatorConfig<A>::Allocator::deallocateBytes((void*)ptr);}}while(0)
#   define CEGUI_DELETE_ARRAY_PT(ptr, T, count, A) do{if(ptr){for (size_t b = count; b-- > 0;){ (ptr)[b].~T();} ::CEGUI::AllocatorConfig<A>::Allocator::deallocateBytes((void*)ptr);}}while(0)
#endif

#ifndef CEGUI_CUSTOM_ALLOCATORS_INCLUDE
#   define CEGUI_CUSTOM_ALLOCATORS_INCLUDE "CEGUI/MemoryStdAllocator.h"
#endif

// all the wrappers have been declared, now we include the chosen memory allocator file
#include CEGUI_CUSTOM_ALLOCATORS_INCLUDE

#else

// dummy macros
#define CEGUI_NEW_AO new
#define CEGUI_DELETE_AO delete
// for primitive types, types not inherited from AllocatedObject
#define CEGUI_NEW_PT(T, Allocator) new T
#define CEGUI_NEW_ARRAY_PT(T, count, Allocator) new T[count]
#define CEGUI_DELETE_PT(ptr, T, Allocator) delete ptr
#define CEGUI_DELETE_ARRAY_PT(ptr, T, count, Allocator) delete [] ptr

#endif

#include "CEGUI/MemoryAllocatedObject.h"
#include "CEGUI/MemorySTLWrapper.h"

#endif	// end of guard _CEGUIMemoryAllocation_h_
