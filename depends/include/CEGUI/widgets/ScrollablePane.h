/***********************************************************************
    created:    1/3/2005
    author:     Paul D Turner
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2015 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIScrollablePane_h_
#define _CEGUIScrollablePane_h_

#include "../Base.h"
#include "../Window.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
//! Base class for ScrollablePane window renderer objects.
class CEGUIEXPORT ScrollablePaneWindowRenderer : public WindowRenderer
{
public:
    //! Constructor
    ScrollablePaneWindowRenderer(const String& name);

    /*!
    \brief
        Return a Rect that described the pane's viewable area, relative
        to this Window, in pixels.

    \return
        Rect object describing the ScrollablePane's viewable area.
    */
    virtual Rectf getViewableArea(void) const = 0;
};

/*!
\brief
    Base class for the ScrollablePane widget.

    The ScrollablePane widget allows child windows to be attached which cover an
    area larger than the ScrollablePane itself and these child windows can be
    scrolled into view using the scrollbars of the scrollable pane.
*/
class CEGUIEXPORT ScrollablePane : public Window
{
public:
    //! Window factory name
    static const String WidgetTypeName;
    //! Namespace for global events
    static const String EventNamespace;
    /** Event fired when an area on the content pane has been updated.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ScrollablePane whose content pane
     * has been updated.
     */
    static const String EventContentPaneChanged;
    /** Event fired when the vertical scroll bar 'force' setting is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ScrollablePane whose vertical scroll
     * bar mode has been changed.
     */
    static const String EventVertScrollbarModeChanged;
    /** Event fired when the horizontal scroll bar 'force' setting is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ScrollablePane whose horizontal scroll
     * bar mode has been changed.
     */
    static const String EventHorzScrollbarModeChanged;
    /** Event fired when the auto size setting for the pane is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ScrollablePane whose auto size
     * setting has been changed.
     */
    static const String EventAutoSizeSettingChanged;
    /** Event fired when the pane gets scrolled.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ScrollablePane that has been scrolled.
     */
    static const String EventContentPaneScrolled;
    //! Widget name for the vertical scrollbar component.
    static const String VertScrollbarName;
    //! Widget name for the horizontal scrollbar component.
    static const String HorzScrollbarName;
    //! Widget name for the scrolled container component.
    static const String ScrolledContainerName;

    //! Constructor for the ScrollablePane base class.
    ScrollablePane(const String& type, const String& name);

    //! Destructor for the ScrollablePane base class.
    ~ScrollablePane(void);

    virtual int writeChildWindowsXML(XMLSerializer& xml_stream) const;

    /*!
    \brief
        Returns a pointer to the window holding the pane contents.

        The purpose of this is so that attached windows may be inspected,
        client code may not modify the returned window in any way.

    \return
        Pointer to the ScrolledContainer that is acting as the container for the
        scrollable pane content.  The returned window is const, client code
        should not modify the ScrolledContainer settings directly.
    */
    const ScrolledContainer* getContentPane(void) const;

    /*!
    \brief
        Return whether the vertical scroll bar is always shown.

    \return
        - true if the scroll bar will be shown even if it is not required.
        - false if the scroll bar will only be shown when it is required.
    */
    bool isVertScrollbarAlwaysShown(void) const;

    /*!
    \brief
        Set whether the vertical scroll bar should always be shown.

    \param setting
        - true if the vertical scroll bar should be shown even when it is not
          required.
        - false if the vertical scroll bar should only be shown when it is
          required.

    \return
        Nothing.
    */
    void setShowVertScrollbar(bool setting);

    /*!
    \brief
        Return whether the horizontal scroll bar is always shown.

    \return
        - true if the scroll bar will be shown even if it is not required.
        - false if the scroll bar will only be shown when it is required.
    */
    bool isHorzScrollbarAlwaysShown(void) const;

    /*!
    \brief
        Set whether the horizontal scroll bar should always be shown.

    \param setting
        - true if the horizontal scroll bar should be shown even when it is not
          required.
        - false if the horizontal scroll bar should only be shown when it is
          required.

    \return
        Nothing.
    */
    void setShowHorzScrollbar(bool setting);

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
        Return the current content pane area for the ScrollablePane.

    \return
        Rect object that details the current pixel extents of the content
        pane attached to this ScrollablePane.
    */
    const Rectf& getContentPaneArea(void) const;

    /*!
    \brief
        Set the current content pane area for the ScrollablePane.

    \note
        If the ScrollablePane is configured to auto-size the content pane
        this call will have no effect.

    \param area
        Rect object that details the pixel extents to use for the content
        pane attached to this ScrollablePane.

    \return
        Nothing.
    */
    void setContentPaneArea(const Rectf& area);

    /*!
    \brief
        Returns the horizontal scrollbar step size as a fraction of one
        complete view page.

    \return
        float value specifying the step size where 1.0f would be the width of
        the viewing area.
    */
    float getHorizontalStepSize(void) const;

    /*!
    \brief
        Sets the horizontal scrollbar step size as a fraction of one
        complete view page.

    \param step
        float value specifying the step size, where 1.0f would be the width of
        the viewing area.

    \return
        Nothing.
    */
    void setHorizontalStepSize(float step);

    /*!
    \brief
        Returns the horizontal scrollbar overlap size as a fraction of one
        complete view page.

    \return
        float value specifying the overlap size where 1.0f would be the width of
        the viewing area.
    */
    float getHorizontalOverlapSize(void) const;

    /*!
    \brief
        Sets the horizontal scrollbar overlap size as a fraction of one
        complete view page.

    \param overlap
        float value specifying the overlap size, where 1.0f would be the width
        of the viewing area.

    \return
        Nothing.
    */
    void setHorizontalOverlapSize(float overlap);

    /*!
    \brief
        Returns the horizontal scroll position as a fraction of the
        complete scrollable width.

    \return
        float value specifying the scroll position.
    */
    float getHorizontalScrollPosition(void) const;

    /*!
    \brief
        Sets the horizontal scroll position as a fraction of the
        complete scrollable width.

    \param position
        float value specifying the new scroll position.

    \return
        Nothing.
    */
    void setHorizontalScrollPosition(float position);

    /*!
    \brief
        Returns the vertical scrollbar step size as a fraction of one
        complete view page.

    \return
        float value specifying the step size where 1.0f would be the height of
        the viewing area.
    */
    float getVerticalStepSize(void) const;

    /*!
    \brief
        Sets the vertical scrollbar step size as a fraction of one
        complete view page.

    \param step
        float value specifying the step size, where 1.0f would be the height of
        the viewing area.

    \return
        Nothing.
    */
    void setVerticalStepSize(float step);

    /*!
    \brief
        Returns the vertical scrollbar overlap size as a fraction of one
        complete view page.

    \return
        float value specifying the overlap size where 1.0f would be the height
        of the viewing area.
    */
    float getVerticalOverlapSize(void) const;

    /*!
    \brief
        Sets the vertical scrollbar overlap size as a fraction of one
        complete view page.

    \param overlap
        float value specifying the overlap size, where 1.0f would be the height
        of the viewing area.

    \return
        Nothing.
    */
    void setVerticalOverlapSize(float overlap);

    /*!
    \brief
        Returns the vertical scroll position as a fraction of the
        complete scrollable height.

    \return
        float value specifying the scroll position.
    */
    float getVerticalScrollPosition(void) const;

    /*!
    \brief
        Sets the vertical scroll position as a fraction of the
        complete scrollable height.

    \param position
        float value specifying the new scroll position.

    \return
        Nothing.
    */
    void setVerticalScrollPosition(float position);

    /*!
    \brief
        Return a Rect that described the pane's viewable area, relative
        to this Window, in pixels.

    \return
        Rect object describing the ScrollablePane's viewable area.
    */
    Rectf getViewableArea(void) const;

    /*!
    \brief
        Return a pointer to the vertical scrollbar component widget for this
        ScrollablePane.

    \return
        Pointer to a Scrollbar object.

    \exception UnknownObjectException
        Thrown if the vertical Scrollbar component does not exist.
    */
    Scrollbar* getVertScrollbar() const;

    /*!
    \brief
        Return a pointer to the horizontal scrollbar component widget for
        this ScrollablePane.

    \return
        Pointer to a Scrollbar object.

    \exception UnknownObjectException
        Thrown if the horizontal Scrollbar component does not exist.
    */
    Scrollbar* getHorzScrollbar() const;

    // Overridden from Window
    void initialiseComponents(void);
    void destroy(void);

protected:
    /*!
    \brief
        display required integrated scroll bars according to current size of
        the ScrollablePane view area and the size of the attached
        ScrolledContainer.
    */
    void configureScrollbars(void);

    /*!
    \brief
        Return whether the vertical scrollbar is needed.

    \return
        - true if the scrollbar is either needed or forced via setting.
        - false if the scrollbar should not be shown.
    */
    bool isVertScrollbarNeeded(void) const;

    /*!
    \brief
        Return whether the horizontal scrollbar is needed.

    \return
        - true if the scrollbar is either needed or forced via setting.
        - false if the scrollbar should not be shown.
    */
    bool isHorzScrollbarNeeded(void) const;

    /*!
    \brief
        Update the content container position according to the current 
        state of the widget (like scrollbar positions, etc).
    */
    void updateContainerPosition(void);

    /*!
    \brief
        Return a pointer to the ScrolledContainer component widget for this
        ScrollablePane.

    \return
        Pointer to a ScrolledContainer object.

    \exception UnknownObjectException
        Thrown if the scrolled container component does not exist.
    */
    ScrolledContainer* getScrolledContainer() const;

    // validate window renderer
    virtual bool validateWindowRenderer(const WindowRenderer* renderer) const;

    /*************************************************************************
        Event triggers
    *************************************************************************/
    /*!
    \brief
        Event trigger method called when some pane content has changed size
        or location.

    \param e
        WindowEventArgs object.

    \return
        Nothing.
    */
    virtual void onContentPaneChanged(WindowEventArgs& e);

    /*!
    \brief
        Event trigger method called when the setting that controls whether the 
        vertical scrollbar is always shown or not, is changed.

    \param e
        WindowEventArgs object.

    \return
        Nothing.
    */
    virtual void onVertScrollbarModeChanged(WindowEventArgs& e);

    /*!
    \brief
        Event trigger method called when the setting that controls whether the 
        horizontal scrollbar is always shown or not, is changed.

    \param e
        WindowEventArgs object.

    \return
        Nothing.
    */
    virtual void onHorzScrollbarModeChanged(WindowEventArgs& e);

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

    /*!
    \brief
        Notification method called whenever the content pane is scrolled via
        changes in the scrollbar positions.

    \param e
        WindowEventArgs object.

    \return
        Nothing.
    */
    virtual void onContentPaneScrolled(WindowEventArgs& e);

    /*************************************************************************
        Event handler methods
    *************************************************************************/
    /*!
    \brief
        Handler method which gets subscribed to the scrollbar position change
        events.
    */
    bool handleScrollChange(const EventArgs& e);

    /*!
    \brief
        Handler method which gets subscribed to the ScrolledContainer content
        change events.
    */
    bool handleContentAreaChange(const EventArgs& e);

    /*!
    \brief
        Handler method which gets subscribed to the ScrolledContainer auto-size
        setting changes.
    */
    bool handleAutoSizePaneChanged(const EventArgs& e);

    // Overridden from Window
    void addChild_impl(Element* element);
    void removeChild_impl(Element* element);
    
    void onSized(ElementEventArgs& e);
    void onMouseWheel(MouseEventArgs& e);

    //! \copydoc Window::getChildByNamePath_impl
    NamedElement* getChildByNamePath_impl(const String& name_path) const;

    //! true if vertical scrollbar should always be displayed
    bool d_forceVertScroll;
    //! true if horizontal scrollbar should always be displayed
    bool d_forceHorzScroll;
    //! holds content area so we can track changes.
    Rectf d_contentRect;
    //! vertical scroll step fraction.
    float d_vertStep;
    //! vertical scroll overlap fraction.
    float d_vertOverlap;
    //! horizontal scroll step fraction.
    float d_horzStep;
    //! horizontal scroll overlap fraction.
    float d_horzOverlap;
    //! Event connection to content pane
    Event::Connection d_contentChangedConn;
    //! Event connection to content pane
    Event::Connection d_autoSizeChangedConn;

private:
    void addScrollablePaneProperties(void);
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIScrollablePane_h_
