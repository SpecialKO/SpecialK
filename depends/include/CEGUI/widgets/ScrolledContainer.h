/***********************************************************************
    created:    1/3/2005
    author:     Paul D Turner
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2009 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIScrolledContainer_h_
#define _CEGUIScrolledContainer_h_

#include "../Window.h"
#include "../WindowFactory.h"
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
    Helper container window class which is used in the implementation of the
    ScrollablePane widget class.
*/
class CEGUIEXPORT ScrolledContainer : public Window
{
public:
    //! Type name for ScrolledContainer.
    static const String WidgetTypeName;
    //! Namespace for global events
    static const String EventNamespace;
    /** Event fired whenever some child changes.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ScrolledContainer for which a child
     * window has changed.
     */
    static const String EventContentChanged;
    /** Event fired when the autosize setting changes.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ScrolledContainer whose auto size
     * setting has been changed.
     */
    static const String EventAutoSizeSettingChanged;

    //! Constructor for ScrolledContainer objects.
    ScrolledContainer(const String& type, const String& name);

    //! Destructor for ScrolledContainer objects.
    ~ScrolledContainer(void);

    /*!
    \brief
        Return whether the content pane is auto sized.

    \return
        - true to indicate the content pane will automatically resize itself.
        - false to indicate the content pane will not automatically resize
          itself.
    */
    bool isContentPaneAutoSized(void) const;

    /*!
    \brief
        Set whether the content pane should be auto-sized.

    \param setting
        - true to indicate the content pane should automatically resize itself.
        - false to indicate the content pane should not automatically resize
          itself.

    \return 
        Nothing.
    */
    void setContentPaneAutoSized(bool setting);

    /*!
    \brief
        Return the current content pane area for the ScrolledContainer.

    \return
        Rect object that details the current pixel extents of the content
        pane represented by this ScrolledContainer.
    */
    const Rectf& getContentArea(void) const;

    /*!
    \brief
        Set the current content pane area for the ScrolledContainer.

    \note
        If the ScrolledContainer is configured to auto-size the content pane
        this call will have no effect.

    \param area
        Rect object that details the pixel extents to use for the content
        pane represented by this ScrolledContainer.

    \return
        Nothing.
    */
    void setContentArea(const Rectf& area);

    /*!
    \brief
        Return the current extents of the attached content.

    \return
        Rect object that describes the pixel extents of the attached
        child windows.  This is effectively the smallest bounding box
        that could contain all the attached windows.
    */
    Rectf getChildExtentsArea(void) const;
    
    virtual const CachedRectf& getClientChildContentArea() const;
    virtual const CachedRectf& getNonClientChildContentArea() const;

    virtual void notifyScreenAreaChanged(bool recursive);
    
protected:
    // Overridden from Window.
    virtual Rectf getUnclippedInnerRect_impl(bool skipAllPixelAlignment) const;
    
    Rectf getClientChildContentArea_impl(bool skipAllPixelAlignment) const;
    
    /*!
    \brief
        Notification method called whenever the content size may have changed.

    \param e
        WindowEventArgs object.

    \return
        Nothing.
    */
    virtual void onContentChanged(WindowEventArgs& e);

    /*!
    \brief
        Notification method called whenever the setting that controls whether
        the content pane is automatically sized is changed.

    \param e
        WindowEventArgs object.

    \return
        Nothing.
    */
    virtual void onAutoSizeSettingChanged(WindowEventArgs& e);

    //! handles notifications about child windows being moved.
    bool handleChildSized(const EventArgs& e);
    //! handles notifications about child windows being sized.
    bool handleChildMoved(const EventArgs& e);

    // overridden from Window.
    void drawSelf(const RenderingContext&) {};
    Rectf getInnerRectClipper_impl() const;

    void setArea_impl(const UVector2& pos, const USize& size,
                      bool topLeftSizing = false, bool fireEvents = true);
    Rectf getHitTestRect_impl() const;
    void onChildAdded(ElementEventArgs& e);
    void onChildRemoved(ElementEventArgs& e);
    void onParentSized(ElementEventArgs& e);

    //! type definition for collection used to track event connections.
    typedef std::multimap<Window*, Event::Connection>  ConnectionTracker;
    //! Tracks event connections we make.
    ConnectionTracker d_eventConnections;
    //! Holds extents of the content pane.
    Rectf d_contentArea;
    //! true if the pane auto-sizes itself.
    bool d_autosizePane;
    
    CachedRectf d_clientChildContentArea;

private:
    void addScrolledContainerProperties(void);
};

} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif	// end of guard _CEGUIScrolledContainer_h_
