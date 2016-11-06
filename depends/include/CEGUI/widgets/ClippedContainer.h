/***********************************************************************
	created:	Sun Jun 11 2006
	author:		Tomas Lindquist Olsen
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
#ifndef _CEGUIClippedContainer_h_
#define _CEGUIClippedContainer_h_

#include "../Window.h"
#include "../WindowFactory.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Helper container window that has configurable clipping.
    Used by the ItemListbox widget.

\deprecated
    This class is deprecated and is scheduled for removal.  The function this
    class used to provide was broken when the inner-rect (aka client area)
    support got fixed.  The good news is that fixing inner-rect support
    effectively negated the need for this class anyway - clipping areas can
    now be established in the looknfeel and extracted via the WindowRenderer.
*/
class CEGUIEXPORT ClippedContainer : public Window
{
public:
    /*************************************************************************
        Constants
    *************************************************************************/
    static const String WidgetTypeName;     //!< Type name for ClippedContainer.
    static const String EventNamespace;     //!< Namespace for global events

    /*************************************************************************
    	Object construction and destruction
    *************************************************************************/
    /*!
    \brief
        Constructor for ClippedContainer objects.
    */
    ClippedContainer(const String& type, const String& name);

    /*!
    \brief
        Destructor for ClippedContainer objects.
    */
    ~ClippedContainer(void);

    /*************************************************************************
    	Public interface methods
    *************************************************************************/
    /*!
    \brief
        Return the current clipping rectangle.

    \return
        Rect object describing the clipping area in pixel that will be applied during rendering.
    */
    const Rectf& getClipArea(void) const;

    /*!
    \brief
        Returns the reference window used for converting the clipper rect to screen space.
    */
    Window* getClipperWindow(void) const;

    /*!
    \brief
        Set the custom clipper area in pixels.
    */
    void setClipArea(const Rectf& r);

    /*!
    \brief
        Set the clipper reference window.

    \param w
        The window to be used a base for converting the custom clipper rect to
        screen space. NULL if the clipper rect is relative to the screen.
    */
    void setClipperWindow(Window* w);

protected:
    /*************************************************************************
    	Overridden from Window.
    *************************************************************************/
    virtual Rectf getUnclippedInnerRect_impl(bool skipAllPixelAlignment) const;
    virtual void drawSelf(const RenderingContext&) {}

    /*************************************************************************
    	Data fields
    *************************************************************************/
    //! the pixel rect to be used for clipping relative to either a window or the screen.
    Rectf d_clipArea;
    //! the base window which the clipping rect is relative to.
    Window* d_clipperWindow;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif	// end of guard _CEGUIClippedContainer_h_
