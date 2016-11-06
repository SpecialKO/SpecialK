/***********************************************************************
	created:	28/10/2010
	author:		Martin Preisler (inspired by Ogre3D)

	purpose:	Wraps CEGUI Allocators to std::allocator class
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
#ifndef _CEGUIMemorySTLWrapper_h_
#define _CEGUIMemorySTLWrapper_h_

#ifndef _CEGUIMemoryAllocation_h_
#   error Dont include this directly! Include CEGUIBase.h instead.
#endif

namespace CEGUI
{
 
#ifdef CEGUI_CUSTOM_ALLOCATORS

template<typename T>
struct STLAllocatorWrapperBase
{
	typedef T value_type;
};
// getting rid of const trick taken from Ogre :-)
template<typename T>
struct STLAllocatorWrapperBase<const T>
{
	typedef T value_type;
};

template <typename T, typename Allocator>
class STLAllocatorWrapper : public STLAllocatorWrapperBase<T>
{
public:
    typedef STLAllocatorWrapperBase<T> Base;
    typedef typename Base::value_type value_type;

	typedef value_type* pointer;
	typedef const value_type* const_pointer;
	typedef value_type& reference;
	typedef const value_type& const_reference;

	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;

	template<typename U>
	struct rebind
	{
		typedef STLAllocatorWrapper<U, Allocator> other;
	};

	inline explicit STLAllocatorWrapper()
	{}

	inline STLAllocatorWrapper(const STLAllocatorWrapper&)
	{}

	template <typename U, typename P>
	inline STLAllocatorWrapper(const STLAllocatorWrapper<U, P>&)
	{}

    inline pointer address(reference x) const
	{
		return &x;
	}

	inline const_pointer address(const_reference x) const
	{
		return &x;
	}

	inline size_type max_size() const throw()
	{
		return Allocator::getMaxAllocationSize();
	}

	inline pointer allocate(size_type count, typename std::allocator<void>::const_pointer ptr = 0)
	{
        (void)ptr;
		return static_cast<pointer>(Allocator::allocateBytes(count * sizeof(T)));
	}

	inline void deallocate(pointer ptr, size_type /*size*/ )
	{
		Allocator::deallocateBytes(ptr);
	}

	inline void construct(pointer p, const T& val)
	{
		new(static_cast<void*>(p)) T(val);
	}

	inline void destroy(pointer p)
	{
		p->~T();
	}
};

template<typename T, typename T2, typename P>
inline bool operator==(const STLAllocatorWrapper<T, P>&, const STLAllocatorWrapper<T2, P>&)
{
	// same allocator, return true
	return true;
}

template<typename T, typename P, typename OtherAllocator>
inline bool operator==(const STLAllocatorWrapper<T, P>&, const OtherAllocator&)
{
    // if the template abose doesn't get matched, return false (allocators differ)
	return false;
}

template<typename T, typename T2, typename P>
inline bool operator!=(const STLAllocatorWrapper<T, P>&, const STLAllocatorWrapper<T2,P>&)
{
	// same allocator, return false (they are not different)
	return false;
}

template<typename T, typename P, typename OtherAllocator>
inline bool operator!=(const STLAllocatorWrapper<T, P>&, const OtherAllocator&)
{
    // the above didn't get matched, that means the allocators differ...
	return true;
}

// STL allocator helper macros
#define CEGUI_VECTOR_ALLOC(T) , ::CEGUI::STLAllocatorWrapper<T, ::CEGUI::AllocatorConfig< ::CEGUI::STLAllocator >::Allocator>
#define CEGUI_SET_ALLOC(T) , ::CEGUI::STLAllocatorWrapper<T, ::CEGUI::AllocatorConfig< ::CEGUI::STLAllocator >::Allocator>
#define CEGUI_MAP_ALLOC(K, V) , ::CEGUI::STLAllocatorWrapper<std::pair<K, V>, ::CEGUI::AllocatorConfig< ::CEGUI::STLAllocator >::Allocator>
#define CEGUI_MULTIMAP_ALLOC(K, V) , ::CEGUI::STLAllocatorWrapper<std::pair<K, V>, ::CEGUI::AllocatorConfig< ::CEGUI::STLAllocator >::Allocator>

#else

// STL allocator helper macros
#define CEGUI_VECTOR_ALLOC(T)
#define CEGUI_SET_ALLOC(T)
#define CEGUI_MAP_ALLOC(K, V)
#define CEGUI_MULTIMAP_ALLOC(K, V)

#endif

}

#endif	// end of guard _CEGUIMemorySTLWrapper_h_
