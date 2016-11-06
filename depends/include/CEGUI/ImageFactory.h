/***********************************************************************
    created:    Sat Jun 11 2011
    author:     Paul D Turner <paul@cegui.org.uk>
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2011 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIImageFactory_h_
#define _CEGUIImageFactory_h_

#include "CEGUI/Image.h"

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Interface for factory objects that create instances of classes
    derived from the Image class.

\note
    This interface is intended for internal use only.
*/
class ImageFactory : public
    AllocatedObject<ImageFactory>
{
public:
    //! base class virtual destructor.
    virtual ~ImageFactory() {}

    //! Create an instance of the Image subclass that this factory creates.
    virtual Image& create(const String& name) = 0;

    /** Create an instance of the Image subclass that this factory creates
     * using the given XMLAttributes object.
     */
    virtual Image& create(const XMLAttributes& attributes) = 0;

    //! Destroy an instance of the Image subclass that this factory creates.
    virtual void destroy(Image& image) = 0;
};

//! Templatised ImageFactory subclass used internally by the system.
template <typename T>
class TplImageFactory : public ImageFactory
{
public:
    // Implement ImageFactory interface
    Image& create(const String& name);
    Image& create(const XMLAttributes& attributes);
    void destroy(Image& image);
};

//---------------------------------------------------------------------------//
template <typename T>
Image& TplImageFactory<T>::create(const String& name)
{
    return *CEGUI_NEW_AO T(name);
}

//---------------------------------------------------------------------------//
template <typename T>
Image& TplImageFactory<T>::create(const XMLAttributes& attributes)
{
    return *CEGUI_NEW_AO T(attributes);
}

//---------------------------------------------------------------------------//
template <typename T>
void TplImageFactory<T>::destroy(Image& image)
{
    CEGUI_DELETE_AO &image;
}

//---------------------------------------------------------------------------//

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIImageFactory_h_

