/***********************************************************************
    created:    22/2/2011
    author:     Martin Preisler

    purpose:    Defines base (and default) layout cell class
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
#ifndef _CEGUILayoutCell_h_
#define _CEGUILayoutCell_h_

#include "../Window.h"

#include <map>

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{

/*!
\brief
    Represents a cell in a layout container

\par
    Layout cell's role is to encapsulate widgets inside a layout container.
    It also contains a "Margin" property to set contained widget's margin on
    all 4 edges.

    Unless you want to set the margin, you should never encounter this class.
    Everything is encapsulated automatically when adding widgets into layout
    containers. You will however see instances of this class inside XML layout
    files.
*/
class CEGUIEXPORT LayoutCell : public Window
{
public:
    /*************************************************************************
        Event name constants
    *************************************************************************/
    //! Namespace for events
    static const String EventNamespace;
    //! Window factory name
    static const String WidgetTypeName;

    /*!
    \brief
        Constructor for LayoutCell class

    \param type
        String object holding Window type (usually provided by WindowFactory).

    \param name
        String object holding unique name for the Window.
    */
    LayoutCell(const String& type, const String& name);

    /*!
    \brief
        Destructor
    */
    virtual ~LayoutCell(void);
    
    virtual const CachedRectf& getClientChildContentArea() const;

    virtual void notifyScreenAreaChanged(bool recursive);
    
protected:
    /// @copydoc Window::getUnclippedInnerRect_impl
    virtual Rectf getUnclippedInnerRect_impl(bool skipAllPixelAlignment) const;
    
    Rectf getClientChildContentArea_impl(bool skipAllPixelAlignment) const;

    /// @copydoc Window::addChild_impl
    virtual void addChild_impl(Element* element);
    /// @copydoc Window::removeChild_impl
    virtual void removeChild_impl(Element* element);

    /*************************************************************************
        Event trigger methods
    *************************************************************************/
    /*!
    \brief
        Handler called when child window gets sized

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the
        window that triggered the event.  For this event the trigger window is
        the one that was sized.
    */
    virtual bool handleChildSized(const EventArgs& e);

    /*!
    \brief
        Handler called when child window gets added

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the
        window that triggered the event.  For this event the trigger window is
        the one that was added.
    */
    virtual bool handleChildAdded(const EventArgs& e);

    /*!
    \brief
        Handler called when child window gets removed

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the
        window that triggered the event.  For this event the trigger window is
        the one that was removed.
    */
    virtual bool handleChildRemoved(const EventArgs& e);

    /*************************************************************************
        Implementation Data
    *************************************************************************/
    typedef std::multimap<Window*, Event::Connection>  ConnectionTracker;
    //! Tracks event connections we make.
    ConnectionTracker d_eventConnections;
    
    CachedRectf d_clientChildContentArea;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUILayoutCell_h_
