/************************************************************************
    created:    Wed Mar 1 2006
    author:     Paul D Turner <paul@cegui.org.uk>
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
#ifndef _CEGUIRefCounted_h_
#define _CEGUIRefCounted_h_

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Simple, generic, reference counted pointer class.  This is primarily here
    for use by the Events system to track when to delete slot bindings.

\note
    Only safe to use with AllocatedObject<T> derived classes!
*/
template<typename T>
class RefCounted
{
public:
    /*!
    \brief
        Default constructor.
    */
    RefCounted() :
        d_object(0),
        d_count(0)
    {
    }

    /*!
    \brief
        Contruct a RefCounted object that wraps the pointer \a ob.
    */
    RefCounted(T* ob) :
        d_object(ob),
        // use system heap for this! no CEGUI_NEW_PT!
        d_count((ob != 0) ? new unsigned int(1) : 0)
    {
    }

    /*!
    \brief
        Copy constructor
    */
    RefCounted(const RefCounted<T>& other) :
        d_object(other.d_object),
        d_count(other.d_count)
    {
        if (d_count)
            addRef();
    }

    /*!
    \brief
        Destructor.  Only deletes the associated object if no further references
        exist.
    */
    ~RefCounted()
    {
        if (d_object)
            release();
    }

    /*!
    \brief
        Assignment operator.  Previously held object gets its reference count
        reduced, and is deleted if no further references exist.  The newly
        assigned object, taken from \a other, gets its count increased.
    */
    RefCounted<T>& operator=(const RefCounted<T>& other)
    {
        if (*this != other)
        {
            if (d_object)
                release();

            d_object = other.d_object;
            d_count = d_object ? other.d_count : 0;

            if (d_count)
                addRef();
        }

        return *this;
    }

    /*!
    \brief
        Return whether the two RefCounted ptrs are equal
        (point to the same object)
    */
    bool operator==(const RefCounted<T>& other) const
    {
        return d_object == other.d_object;
    }

    /*!
    \brief
        Return whether the two RefCounted ptrs are not equal
        (point to different objects)
    */
    bool operator!=(const RefCounted<T>& other) const
    {
        return d_object != other.d_object;
    }

    /*!
    \brief
        Return the object referred to by the wrapped pointer.
        (beware of null pointers when using this!)
    */
    const T& operator*() const
    {
        return *d_object;
    }

    T& operator*()
    {
        return *d_object;
    }

    /*!
    \brief
        Return the wrapped pointer.
    */
    const T* operator->() const
    {
        return d_object;
    }

    T* operator->()
    {
        return d_object;
    }

    /*!
    \brief
        Return whether the wrapped pointer is valid.  i.e. that it is not null.
    */
    bool isValid() const
    {
        return d_object != 0;
    }

private:
    /*!
    \brief
        Increases the reference count for the wrapped object.
    */
    void addRef()
    {
        ++*d_count;
    }

    /*!
    \brief
        Decreases the reference count for the wrapped object, and if the count
        has reached zero, the object and the shared counter are deleted.
    */
    void release()
    {
        if (!--*d_count)
        {
            // use CEGUI allocators for the object
            CEGUI_DELETE_AO d_object;

            // use system heap for this! no CEGUI_DELETE_PT!
            delete d_count;
            d_object = 0;
            d_count = 0;
        }
    }

    T* d_object;            //! pointer to the object.
    unsigned int* d_count;  //! pointer to the shared counter object.
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIRefCounted_h_
