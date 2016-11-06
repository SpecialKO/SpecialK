/***********************************************************************
    created:    02/8/2010
    author:     Martin Preisler

    purpose:    Defines abstract base class for layout containers
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
#ifndef _CEGUISequentialLayoutContainer_h_
#define _CEGUISequentialLayoutContainer_h_

#include "./LayoutContainer.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{

/*!
\brief
    An abstract base class providing common functionality and specifying the
    required interface for derived classes.

    Sequential Layout Container provide means for automatic positioning of
    windows in sequence
*/
class CEGUIEXPORT SequentialLayoutContainer : public LayoutContainer
{
public:
    /*************************************************************************
        Event name constants
    *************************************************************************/
    //! Namespace for global events
    static const String EventNamespace;

    //! fired when child windows get rearranged
    static const String EventChildOrderChanged;

    /*!
    \brief
        Constructor for Window base class

    \param type
        String object holding Window type (usually provided by WindowFactory).

    \param name
        String object holding unique name for the Window.
    */
    SequentialLayoutContainer(const String& type, const String& name);

    /*!
    \brief
        Destructor for Window base class
    */
    virtual ~SequentialLayoutContainer(void);

    /*!
    \brief
        Gets the position of given child window
    */
    size_t getPositionOfChild(Window* wnd) const;

    /*!
    \brief
        Gets the position of given child window
    */
    size_t getPositionOfChild(const String& wnd) const;

    /*!
    \brief
        Gets the child window that currently is at given position
    */
    Window* getChildAtPosition(size_t position) const;

    /*!
    \brief
        Swaps windows at given positions
    */
    virtual void swapChildPositions(size_t wnd1, size_t wnd2);

    /*!
    \brief
        Swaps positions of given windows
    */
    void swapChildren(Window* wnd1, Window* wnd2);

    /*!
    \brief
        Swaps positions of given windows
    */
    void swapChildren(const String& wnd1, Window* wnd2);

    /*!
    \brief
        Swaps positions of given windows
    */
    void swapChildren(Window* wnd1, const String& wnd2);

    /*!
    \brief
        Swaps positions of given windows
    */
    void swapChildren(const String& wnd1, const String& wnd2);

    /*!
    \brief
        Moves a window that is alrady child of thi layout container
        to given position (if the window is currently in a position
        that is smaller than given position, given position is
        automatically decremented
    */
    virtual void moveChildToPosition(Window* wnd, size_t position);

    /*!
    \brief
        Moves a window that is alrady child of thi layout container
        to given position (if the window is currently in a position
        that is smaller than given position, given position is
        automatically decremented
    */
    void moveChildToPosition(const String& wnd, size_t position);

    /*!
    \brief
        Moves a window forward or backward, depending on delta
        (-1 moves it backward one step, 1 moves it forward one step)

    \param delta
        The amount of steps the window will be moved
        (old position + delta = new position)
    */
    void moveChild(Window* window, int delta = 1);

    /*!
    \brief
        Adds a window to given position
    */
    void addChildToPosition(Window* window, size_t position);

    /*!
    \brief
        Removes a window from given position
    */
    void removeChildFromPosition(size_t position);

protected:
    /*!
    \brief
        Handler called when children of this window gets rearranged in any way

    \param e
        WindowEventArgs object whose 'window' field is set this layout
        container.
    */
    virtual void onChildOrderChanged(WindowEventArgs& e);
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUISequentialLayoutContainer_h_

