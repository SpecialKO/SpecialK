/***********************************************************************
    created:    29/7/2010
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
#ifndef _CEGUILayoutContainer_h_
#define _CEGUILayoutContainer_h_

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
    An abstract base class providing common functionality and specifying the
    required interface for derived classes.

    Layout Container provide means for automatic positioning based on sizes of
    it's child Windows. This is useful for dynamic UIs.
*/
class CEGUIEXPORT LayoutContainer : public Window
{
public:
    /*************************************************************************
        Event name constants
    *************************************************************************/
    //! Namespace for global events
    static const String EventNamespace;

    /*!
    \brief
        Constructor for Window base class

    \param type
        String object holding Window type (usually provided by WindowFactory).

    \param name
        String object holding unique name for the Window.
    */
    LayoutContainer(const String& type, const String& name);

    /*!
    \brief
        Destructor for Window base class
    */
    virtual ~LayoutContainer(void);

    /*!
    \brief
        marks this layout container for relayouting before drawing
    */
    void markNeedsLayouting();

    /*!
    \brief
        returns true if this layout container will be relayouted before drawing
    */
    bool needsLayouting() const;

    /*!
    \brief
        (re)layouts all windows inside this layout container immediately
    */
    virtual void layout() = 0;

    /*!
    \brief
        (re)layouts all windows inside this layout container if it was marked
        necessary
    */
    virtual void layoutIfNecessary();

    /// @copydoc Window::update
    virtual void update(float elapsed);
    
    virtual const CachedRectf& getClientChildContentArea() const;

    virtual void notifyScreenAreaChanged(bool recursive);

protected:
    /// @copydoc Window::getUnclippedInnerRect_impl
    virtual Rectf getUnclippedInnerRect_impl(bool skipAllPixelAlignment) const;
    
    Rectf getClientChildContentArea_impl(bool skipAllPixelAlignment) const;

    size_t getIdxOfChild(Window* wnd) const;

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
        Handler called when child window changes margin(s)

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the
        window that triggered the event.  For this event the trigger window is
        the one that has had it's margin(s) changed.
    */
    virtual bool handleChildMarginChanged(const EventArgs& e);

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

    /*!
    \brief
        returns margin offset for given window
    */
    virtual UVector2 getOffsetForWindow(Window* window) const;

    /*!
    \brief
        returns bounding size for window, including margins
    */
    virtual UVector2 getBoundingSizeForWindow(Window* window) const;

    // overridden from parent class
    void onParentSized(ElementEventArgs& e);

    /*************************************************************************
        Implementation Data
    *************************************************************************/
    // if true, we will relayout before rendering of this window starts
    bool d_needsLayouting;

    typedef std::multimap<Window*, Event::Connection>  ConnectionTracker;
    //! Tracks event connections we make.
    ConnectionTracker d_eventConnections;
    
    CachedRectf d_clientChildContentArea;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUILayoutContainer_h_

