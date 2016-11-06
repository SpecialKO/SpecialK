/***********************************************************************
    created:    13/4/2004
    author:     Paul D Turner
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2012 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIScrollbar_h_
#define _CEGUIScrollbar_h_

#include "../Base.h"
#include "../Window.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
//! Base class for Scrollbar window renderer objects.
class CEGUIEXPORT ScrollbarWindowRenderer : public WindowRenderer
{
public:
    ScrollbarWindowRenderer(const String& name);

    /*!
    \brief
        update the size and location of the thumb to properly represent the
        current state of the scroll bar
    */
    virtual void updateThumb(void) = 0;

    /*!
    \brief
        return value that best represents current scroll bar position given the
        current location of the thumb.

    \return
        float value that, given the thumb widget position, best represents the
        current position for the scroll bar.
    */
    virtual float getValueFromThumb(void) const = 0;

    /*!
    \brief
        Given window location \a pt, return a value indicating what change
        should be made to the scroll bar.

    \param pt
        Point object describing a pixel position in window space.

    \return
        - -1 to indicate scroll bar position should be moved to a lower value.
        -  0 to indicate scroll bar position should not be changed.
        - +1 to indicate scroll bar position should be moved to a higher value.
    */
    virtual float getAdjustDirectionFromPoint(const Vector2f& pt) const  = 0;
};

/*!
\brief
    Base scroll bar class.

    This base class for scroll bars does not have any idea of direction - a
    derived class would add whatever meaning is appropriate according to what
    that derived class represents to the user.
*/
class CEGUIEXPORT Scrollbar : public Window
{
public:
    //! Window factory name
    static const String WidgetTypeName;

    //! Namespace for global events
    static const String EventNamespace;
    /** Event fired when the scroll bar position value changes.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Scrollbar whose position value had
     * changed.
     */
    static const String EventScrollPositionChanged;
    /** Event fired when the user begins dragging the scrollbar thumb.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Scrollbar whose thumb is being
     * dragged.
     */
    static const String EventThumbTrackStarted;
    /** Event fired when the user releases the scrollbar thumb.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Scrollbar whose thumb has been
     * released.
     */
    static const String EventThumbTrackEnded;
    /** Event fired when the scroll bar configuration data is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Scrollbar whose configuration
     * has been changed.
     */
    static const String EventScrollConfigChanged;

    //! Widget name for the thumb component.
    static const String ThumbName;
    //! Widget name for the increase button component.
    static const String IncreaseButtonName;
    //! Widget name for the decrease button component.
    static const String DecreaseButtonName;

    /*!
    \brief
        Return the size of the document or data.

        The document size should be thought of as the total size of the data
        that is being scrolled through (the number of lines in a text file for
        example).

    \note
        The returned value has no meaning within the Gui system, it is left up
        to the application to assign appropriate values for the application
        specific use of the scroll bar.

    \return
        float value specifying the currently set document size.
    */
    float getDocumentSize(void) const
    {
        return d_documentSize;
    }

    /*!
    \brief
        Set the size of the document or data.

        The document size should be thought of as the total size of the data
        that is being scrolled through (the number of lines in a text file for
        example).

    \note
        The value set has no meaning within the Gui system, it is left up to
        the application to assign appropriate values for the application
        specific use of the scroll bar.

    \param document_size
        float value specifying the document size.
    */
    void setDocumentSize(float document_size);

    /*!
    \brief
        Return the page size for this scroll bar.

        The page size is typically the amount of data that can be displayed at
        one time.  This value is also used when calculating the amount the
        position will change when you click either side of the scroll bar
        thumb, the amount the position changes will is (pageSize - overlapSize).

    \note
        The returned value has no meaning within the Gui system, it is left up
        to the application to assign appropriate values for the application
        specific use of the scroll bar.

    \return
        float value specifying the currently set page size.
    */
    float getPageSize(void) const
    {
        return d_pageSize;
    }

    /*!
    \brief
        Set the page size for this scroll bar.

        The page size is typically the amount of data that can be displayed at
        one time.  This value is also used when calculating the amount the
        position will change when you click either side of the scroll bar
        thumb, the amount the position changes will is (pageSize - overlapSize).

    \note
        The value set has no meaning within the Gui system, it is left up to the
        application to assign appropriate values for the application specific
        use of the scroll bar.

    \param page_size
        float value specifying the page size.
    */
    void setPageSize(float page_size);

    /*!
    \brief
        Return the step size for this scroll bar.

        The step size is typically a single unit of data that can be displayed,
        this is the amount the position will change when you click either of
        the arrow buttons on the scroll bar.  (this could be 1 for a single
        line of text, for example).

    \note
        The returned value has no meaning within the Gui system, it is left up
        to the application to assign appropriate values for the application
        specific use of the scroll bar.

    \return
        float value specifying the currently set step size.
    */
    float getStepSize(void) const
    {
        return d_stepSize;
    }

    /*!
    \brief
        Set the step size for this scroll bar.

        The step size is typically a single unit of data that can be displayed,
        this is the amount the position will change when you click either of the
        arrow buttons on the scroll bar.  (this could be 1 for a single line of
        text, for example).

    \note
        The value set has no meaning within the Gui system, it is left up to the
        application to assign appropriate values for the application specific
        use of the scroll bar.

    \param step_size
        float value specifying the step size.
    */
    void setStepSize(float step_size);

    /*!
    \brief
        Return the overlap size for this scroll bar.

        The overlap size is the amount of data from the end of a 'page' that
        will remain visible when the position is moved by a page.  This is
        usually used so that the user keeps some context of where they were
        within the document's data when jumping a page at a time.

    \note
        The returned value has no meaning within the Gui system, it is left up
        to the application to assign appropriate values for the application
        specific use of the scroll bar.

    \return
        float value specifying the currently set overlap size.
    */
    float getOverlapSize(void) const
    {
        return d_overlapSize;
    }

    /*!
    \brief
        Set the overlap size for this scroll bar.

        The overlap size is the amount of data from the end of a 'page' that
        will remain visible when the position is moved by a page.  This is
        usually used so that the user keeps some context of where they were
        within the document's data when jumping a page at a time.

    \note
        The value set has no meaning within the Gui system, it is left up to the
        application to assign appropriate values for the application specific
        use of the scroll bar.

    \param overlap_size
        float value specifying the overlap size.
    */
    void setOverlapSize(float overlap_size);

    /*!
    \brief
        Return the current position of scroll bar within the document.

        The range of the scroll bar is from 0 to the size of the document minus
        the size of a page (0 <= position <= (documentSize - pageSize)).

    \note
        The returned value has no meaning within the Gui system, it is left up
        to the application to assign appropriate values for the application
        specific use of the scroll bar.

    \return
        float value specifying the current position of the scroll bar within its
        document.
    */
    float getScrollPosition(void) const
    {
        return d_position;
    }

    /*!
    \brief
        Set the current position of scroll bar within the document.

        The range of the scroll bar is from 0 to the size of the document minus
        the size of a page (0 <= position <= (documentSize - pageSize)), any
        attempt to set the position outside this range will be adjusted so that
        it falls within the legal range.

    \param position
        float value specifying the position of the scroll bar within its
        document.
    */
    void setScrollPosition(float position);

    //! return the current scroll position as a value in the interval [0, 1]
    float getUnitIntervalScrollPosition() const;
    //! set the current scroll position as a value in the interval [0, 1]
    void setUnitIntervalScrollPosition(float position);

    /*!
    \brief
        Return a pointer to the 'increase' PushButtoncomponent widget for this
        Scrollbar.

    \return
        Pointer to a PushButton object.

    \exception UnknownObjectException
        Thrown if the increase PushButton component does not exist.
    */
    PushButton* getIncreaseButton() const;

    /*!
    \brief
        Return a pointer to the 'decrease' PushButton component widget for this
        Scrollbar.

    \return
        Pointer to a PushButton object.

    \exception UnknownObjectException
        Thrown if the 'decrease' PushButton component does not exist.
    */
    PushButton* getDecreaseButton() const;

    /*!
    \brief
        Return a pointer to the Thumb component widget for this Scrollbar.

    \return
        Pointer to a Thumb object.

    \exception UnknownObjectException
        Thrown if the Thumb component does not exist.
    */
    Thumb* getThumb() const;

    /*!
    \brief
        Sets multiple scrollbar configuration parameters simultaneously.

        This function is provided in order to be able to minimise the number
        of internal state updates that occur when modifying the scrollbar
        parameters.

    \param document_size
        Pointer to a float value holding the new value to be used for the
        scroll bar document size.  If this is 0 the document size is left
        unchanged.

    \param page_size
        Pointer to a float value holding the new value to be used for the scroll
        bar page size.  If this is 0 the page size is left unchanged.

    \param step_size
        Pointer to a float value holding the new value to be used for the scroll
        bar step size.  If this is 0 the step size is left unchanged.

    \param overlap_size
        Pointer to a float value holding the new value to be used for the scroll
        bar overlap size.  If this is 0 then overlap size is left unchanged.

    \param position
        Pointer to a float value holding the new value to be used for the scroll
        bar current scroll position.  If this is 0 then the current position is
        left unchanged.

    \note
        Even if \a position is 0, the scrollbar position may still change
        depending on how the other changes affect the scrollbar.
    */
    void setConfig(const float* const document_size,
                   const float* const page_size,
                   const float* const step_size,
                   const float* const overlap_size,
                   const float* const position);

    /*!
    \brief
        Enable or disable the 'end lock' mode for the scrollbar.

        When enabled and the current position of the scrollbar is at the end of
        it's travel, the end lock mode of the scrollbar will automatically
        update the position when the document and/or page size change in order
        that the scroll position will remain at the end of it's travel.  This
        can be used to implement auto-scrolling in certain other widget types.

    \param enabled
        - true to indicate that end lock mode should be enabled.
        - false to indicate that end lock mode should be disabled.
    */
    void setEndLockEnabled(const bool enabled);

    /*!
    \brief
        Returns whether the 'end lock'mode for the scrollbar is enabled.

        When enabled, and the current position of the scrollbar is at the end of
        it's travel, the end lock mode of the scrollbar will automatically
        update the scrollbar position when the document and/or page size change
        in order that the scroll position will remain at the end of it's travel.
        This can be used to implement auto-scrolling in certain other widget
        types.

    \return
        - true to indicate that the end lock mode is enabled.
        - false to indicate that the end lock mode is disabled.
    */
    bool isEndLockEnabled() const;

    //! move scroll position forwards by the current step size
    void scrollForwardsByStep();
    //! move scroll position backwards by the current step size
    void scrollBackwardsByStep();

    //! move scroll position forwards by a page (uses appropriate overlap)
    void scrollForwardsByPage();
    //! move scroll position backwards by a page (uses appropriate overlap)
    void scrollBackwardsByPage();

    Scrollbar(const String& type, const String& name);
    ~Scrollbar(void);

    // overrides
    void initialiseComponents(void);

protected:
    /*!
    \brief
        update the size and location of the thumb to properly represent the
        current state of the scroll bar
    */
    void updateThumb(void);

    /*!
    \brief
        return value that best represents current scroll bar position given the
        current location of the thumb.

    \return
        float value that, given the thumb widget position, best represents the
        current position for the scroll bar.
    */
    float getValueFromThumb(void) const;

    /*!
    \brief
        Given window location \a pt, return a value indicating what change
        should be made to the scroll bar.

    \param pt
        Point object describing a pixel position in window space.

    \return
        - -1 to indicate scroll bar position should be moved to a lower value.
        -  0 to indicate scroll bar position should not be changed.
        - +1 to indicate scroll bar position should be moved to a higher value.
    */
    float getAdjustDirectionFromPoint(const Vector2f& pt) const;

    /** implementation func that updates scroll position value, returns true if
     * value was changed.  NB: Fires no events and does no other updates.
     */
    bool setScrollPosition_impl(const float position);

    //! return whether the current scroll position is at the end of the range.
    bool isAtEnd() const;

    //! return the max allowable scroll position value
    float getMaxScrollPosition() const;

    //! handler function for when thumb moves.
    bool handleThumbMoved(const EventArgs& e);

    //! handler function for when the increase button is clicked.
    bool handleIncreaseClicked(const EventArgs& e);

    //! handler function for when the decrease button is clicked.
    bool handleDecreaseClicked(const EventArgs& e);

    //! handler function for when thumb tracking begins
    bool handleThumbTrackStarted(const EventArgs& e);

    //! handler function for when thumb tracking begins
    bool handleThumbTrackEnded(const EventArgs& e);

    //! validate window renderer
    virtual bool validateWindowRenderer(const WindowRenderer* renderer) const;

    // New event handlers for slider widget
    //! Handler triggered when the scroll position changes
    virtual void onScrollPositionChanged(WindowEventArgs& e);

    //! Handler triggered when the user begins to drag the scroll bar thumb.
    virtual void onThumbTrackStarted(WindowEventArgs& e);

    //! Handler triggered when the scroll bar thumb is released
    virtual void onThumbTrackEnded(WindowEventArgs& e);

    //! Handler triggered when the scroll bar data configuration changes
    virtual void onScrollConfigChanged(WindowEventArgs& e);

    // Overridden event handlers
    virtual void onMouseButtonDown(MouseEventArgs& e);
    virtual void onMouseWheel(MouseEventArgs& e);

    // base class overrides
    void banPropertiesForAutoWindow();

    // Implementation Data
    //! The size of the document / data being scrolled thorugh.
    float d_documentSize;
    //! The size of a single 'page' of data.
    float d_pageSize;
    //! Step size used for increase / decrease button clicks.
    float d_stepSize;
    //! Amount of overlap when jumping by a page.
    float d_overlapSize;
    //! Current scroll position.
    float d_position;
    //! whether 'end lock' mode is enabled.
    bool d_endLockPosition;

private:
    //! Adds scrollbar specific properties.
    void addScrollbarProperties(void);
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIScrollbar_h_
