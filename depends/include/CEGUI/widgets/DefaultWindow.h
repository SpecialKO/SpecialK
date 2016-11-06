/***********************************************************************
    created:    5/6/2004
    author:     Paul D Turner

    purpose:    Interface to a default window
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
#ifndef _CEGUIDefaultWindow_h_
#define _CEGUIDefaultWindow_h_

#include "../Window.h"
#include "../WindowFactory.h"


// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Window class intended to be used as a simple, generic Window.

    This class does no rendering and so appears totally transparent.  This window defaults
    to position 0.0f, 0.0f with a size of 1.0f x 1.0f.

    /par
    This Window has been used as the root GUI-sheet (root window) but it's usage has been extended
    beyond that. That's why it's name has been changed to "DefaultWindow" for 0.8.
*/
class CEGUIEXPORT DefaultWindow : public Window
{
public:
    /*************************************************************************
        Constants
    *************************************************************************/
    // type name for this widget
    static const String WidgetTypeName;             //!< The unique typename of this widget


    /*************************************************************************
        Construction and Destruction
    *************************************************************************/
    /*!
    \brief
        Constructor for DefaultWindows.
    */
    DefaultWindow(const String& type, const String& name);


    /*!
    \brief
        Destructor for DefaultWindows.
    */
    virtual ~DefaultWindow(void) {}


protected:
    //! helper to update mouse input handled state
    void updateMouseEventHandled(MouseEventArgs& e) const;

    // overridden functions from Window base class
    bool moveToFront_impl(bool wasClicked);

    // override the mouse event handlers
    void onMouseMove(MouseEventArgs& e);
    void onMouseWheel(MouseEventArgs& e);
    void onMouseButtonDown(MouseEventArgs& e);
    void onMouseButtonUp(MouseEventArgs& e);
    void onMouseClicked(MouseEventArgs& e);
    void onMouseDoubleClicked(MouseEventArgs& e);
    void onMouseTripleClicked(MouseEventArgs& e);
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIDefaultWindow_h_
