/***********************************************************************
	created:	15/6/2004
	author:		Paul D Turner
	
	purpose:	Interface to list header segment class.
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
#ifndef _CEGUIListHeaderSegment_h_
#define _CEGUIListHeaderSegment_h_

#include "../Base.h"
#include "../Window.h"


#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif


// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
	Base class for list header segment window
*/
class CEGUIEXPORT ListHeaderSegment : public Window
{
public:
	static const String EventNamespace;				//!< Namespace for global events
    static const String WidgetTypeName;             //!< Window factory name


	/*************************************************************************
		Constants
	*************************************************************************/
	// Event names
    /** Event fired when the segment is clicked.
     * Hanlders are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ListHeaderSegment that was clicked.
     */
	static const String EventSegmentClicked;
    /** Event fired when the sizer/splitter is double-clicked.
     * Hanlders are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ListHeaderSegment whose
     * sizer / splitter area was double-clicked.
     */
	static const String EventSplitterDoubleClicked;
    /** Event fired when the user drag-sizable setting is changed.
     * Hanlders are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ListHeaderSegment whose user sizable
     * setting has been changed.
     */
	static const String EventSizingSettingChanged;
    /** Event fired when the sort direction value is changed.
     * Hanlders are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ListHeaderSegment whose sort direction
     * has been changed.
     */
	static const String EventSortDirectionChanged;
    /** Event fired when the user drag-movable setting is changed.
     * Hanlders are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ListHeaderSegment whose user
     * drag-movable setting has been changed.
     */
	static const String EventMovableSettingChanged;
    /** Event fired when the segment has started to be dragged.
     * Hanlders are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ListHeaderSegment that has started to
     * be dragged.
     */
	static const String EventSegmentDragStart;
    /** Event fired when segment dragging has stopped (via mouse release).
     * Hanlders are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ListHeaderSegment that is no longer
     * being dragged.
     */
	static const String EventSegmentDragStop;
    /** Event fired when the segment drag position has changed.
     * Hanlders are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ListHeaderSegment whose position has
     * changed due to being dragged.
     */
	static const String EventSegmentDragPositionChanged;
    /** Event fired when the segment is sized by the user.
     * Hanlders are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ListHeaderSegment that has been
     * resized by the user dragging.
     */
	static const String EventSegmentSized;
    /** Event fired when the clickable setting for the segment is changed.
     * Hanlders are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ListHeaderSegment whose setting that
     * controls whether the segment is clickable has been changed.
     */
	static const String EventClickableSettingChanged;

	// Defaults
	static const float	DefaultSizingArea;		//!< Default size of the sizing area.
	static const float	SegmentMoveThreshold;	//!< Amount the mouse must be dragged before drag-moving is initiated.


	/*************************************************************************
		Enumerated types
	*************************************************************************/
	/*!
	\brief
		Enumeration of possible values for sorting direction used with ListHeaderSegment classes
	*/
	enum SortDirection
	{
		None,		//!< Items under this segment should not be sorted.
		Ascending,	//!< Items under this segment should be sorted in ascending order.
		Descending	//!< Items under this segment should be sorted in descending order.
	};


	/*************************************************************************
		Accessor Methods
	*************************************************************************/
	/*!
	\brief
		Return whether this segment can be sized.

	\return
		true if the segment can be horizontally sized, false if the segment can not be horizontally sized.
	*/
	bool	isSizingEnabled(void) const			{return d_sizingEnabled;}


	/*!
	\brief
		Return the current sort direction set for this segment.

		Note that this has no impact on the way the segment functions (aside from the possibility
		of varied rendering).  This is intended as a 'helper setting' to classes that make use of
		the ListHeaderSegment objects.

	\return
		One of the SortDirection enumerated values indicating the current sort direction set for this
		segment.
	*/
	SortDirection	getSortDirection(void) const	{return d_sortDir;}


	/*!
	\brief
		Return whether drag moving is enabled for this segment.

	\return
		true if the segment can be drag moved, false if the segment can not be drag moved.
	*/
	bool	isDragMovingEnabled(void) const		{return d_movingEnabled;}


	/*!
	\brief
		Return the current drag move position offset (in pixels relative to the top-left corner of the segment).

	\return
		Point object describing the drag move offset position.
	*/
	const Vector2f& getDragMoveOffset(void) const	{return d_dragPosition;}


	/*!
	\brief
		Return whether the segment is clickable.

	\return
		true if the segment can be clicked, false of the segment can not be clicked (so no highlighting or events will happen).
	*/
	bool	isClickable(void) const		{return d_allowClicks;}


    /*!
    \brief
        Return whether the segment is currently in its hovering state.
    */
    bool    isSegmentHovering(void) const  {return d_segmentHover;}


    /*!
    \brief
        Return whether the segment is currently in its pushed state.
    */
    bool    isSegmentPushed(void) const  {return d_segmentPushed;}


    /*!
    \brief
        Return whether the splitter is currently in its hovering state.
    */
    bool    isSplitterHovering(void) const  {return d_splitterHover;}


    /*!
    \brief
        Return whether the segment is currently being drag-moved.
    */
    bool    isBeingDragMoved(void) const  {return d_dragMoving;}


    /*!
    \brief
        Return whether the segment is currently being drag-moved.
    */
    bool    isBeingDragSized(void) const  {return d_dragSizing;}


    const Image* getSizingCursorImage() const;
    const Image* getMovingCursorImage() const;


	/*************************************************************************
		Manipulator Methods
	*************************************************************************/
	/*!
	\brief
		Set whether this segment can be sized.

	\param setting
		true if the segment may be horizontally sized, false if the segment may not be horizontally sized.

	\return
		Nothing.
	*/
	void	setSizingEnabled(bool setting);


	/*!
	\brief
		Set the current sort direction set for this segment.

		Note that this has no impact on the way the segment functions (aside from the possibility
		of varied rendering).  This is intended as a 'helper setting' to classes that make use of
		the ListHeaderSegment objects.

	\param sort_dir
		One of the SortDirection enumerated values indicating the current sort direction set for this
		segment.

	\return
		Nothing
	*/
	void	setSortDirection(SortDirection sort_dir);


	/*!
	\brief
		Set whether drag moving is allowed for this segment.

	\param setting
		true if the segment may be drag moved, false if the segment may not be drag moved.

	\return
		Nothing.
	*/
	void	setDragMovingEnabled(bool setting);


	/*!
	\brief
		Set whether the segment is clickable.

	\param setting
		true if the segment may be clicked, false of the segment may not be clicked (so no highlighting or events will happen).

	\return
		Nothing.
	*/
	void setClickable(bool setting);


    void setSizingCursorImage(const Image* image);
    void setSizingCursorImage(const String& name);
    void setMovingCursorImage(const Image* image);
    void setMovingCursorImage(const String& name);


	/*************************************************************************
		Construction & Destruction
	*************************************************************************/
	/*!
	\brief
		Constructor for list header segment base class
	*/
	ListHeaderSegment(const String& type, const String& name);


	/*!
	\brief
		Destructor for list header segment base class.
	*/
	virtual ~ListHeaderSegment(void);


protected:
	/*************************************************************************
		Implementation Methods
	*************************************************************************/
	/*!
	\brief
		Update state for drag sizing.

	\param local_mouse
		Mouse position as a pixel offset from the top-left corner of this window.

	\return
		Nothing.
	*/
	void	doDragSizing(const Vector2f& local_mouse);


	/*!
	\brief
		Update state for drag moving.

	\param local_mouse
		Mouse position as a pixel offset from the top-left corner of this window.

	\return
		Nothing.
	*/
	void	doDragMoving(const Vector2f& local_mouse);


	/*!
	\brief
		Initialise the required states to put the widget into drag-moving mode.
	*/
	void	initDragMoving(void);


	/*!
	\brief
		Initialise the required states when we are hovering over the sizing area.
	*/
	void	initSizingHoverState(void);


	/*!
	\brief
		Initialise the required states when we are hovering over the main segment area.
	*/
	void	initSegmentHoverState(void);


	/*!
	\brief
		Return whether the required minimum movement threshold before initiating drag-moving
		has been exceeded.

	\param local_mouse
		Mouse position as a pixel offset from the top-left corner of this window.

	\return
		true if the threshold has been exceeded and drag-moving should be initiated, or false
		if the threshold has not been exceeded.
	*/		
	bool	isDragMoveThresholdExceeded(const Vector2f& local_mouse);

	/*************************************************************************
		New Event Handlers
	*************************************************************************/
	/*!
	\brief
		Handler called when segment is clicked.
	*/
	virtual void	onSegmentClicked(WindowEventArgs& e);


	/*!
	\brief
		Handler called when the sizer/splitter is double-clicked.
	*/
	virtual void	onSplitterDoubleClicked(WindowEventArgs& e);


	/*!
	\brief
		Handler called when sizing setting changes.
	*/
	virtual void	onSizingSettingChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called when the sort direction value changes.
	*/
	virtual void	onSortDirectionChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called when the drag-movable setting is changed.
	*/
	virtual void	onMovableSettingChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called when the user starts dragging the segment.
	*/
	virtual void	onSegmentDragStart(WindowEventArgs& e);


	/*!
	\brief
		Handler called when the user stops dragging the segment (releases mouse button)
	*/
	virtual void	onSegmentDragStop(WindowEventArgs& e);


	/*!
	\brief
		Handler called when the drag position changes.
	*/
	virtual void	onSegmentDragPositionChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called when the segment is sized.
	*/
	virtual void	onSegmentSized(WindowEventArgs& e);


	/*!
	\brief
		Handler called when the clickable setting for the segment changes
	*/
	virtual void	onClickableSettingChanged(WindowEventArgs& e);


	/*************************************************************************
		Overridden Event Handlers
	*************************************************************************/
	virtual void	onMouseMove(MouseEventArgs& e);
	virtual void	onMouseButtonDown(MouseEventArgs& e);
	virtual void	onMouseButtonUp(MouseEventArgs& e);
	virtual void	onMouseDoubleClicked(MouseEventArgs& e);
	virtual void	onMouseLeaves(MouseEventArgs& e);
	virtual void	onCaptureLost(WindowEventArgs& e);


	/*************************************************************************
		Implementation Data
	*************************************************************************/
	const Image*	d_sizingMouseCursor;	//!< Image to use for mouse when sizing (typically set by derived class).
	const Image*	d_movingMouseCursor;	//!< Image to use for mouse when moving (typically set by derived class).

	float	d_splitterSize;		//!< pixel width of the sizing area.
	bool	d_splitterHover;	//!< True if the mouse is over the splitter

	bool	d_dragSizing;		//!< true when we are being sized.
	Vector2f d_dragPoint;		//!< point we are being dragged at when sizing or moving.

	SortDirection	d_sortDir;	//!< Direction for sorting (used for deciding what icon to display).

	bool	d_segmentHover;		//!< true when the mouse is within the segment area (and not in sizing area).
	bool	d_segmentPushed;	//!< true when the left mouse button has been pressed within the confines of the segment.
	bool	d_sizingEnabled;	//!< true when sizing is enabled for this segment.
	bool	d_movingEnabled;	//!< True when drag-moving is enabled for this segment;
	bool	d_dragMoving;		//!< true when segment is being drag moved.
	Vector2f d_dragPosition;		//!< position of dragged segment.
	bool	d_allowClicks;		//!< true if the segment can be clicked.

private:
	/*************************************************************************
		Private methods
	*************************************************************************/
	void	addHeaderSegmentProperties(void);
};


template<>
class PropertyHelper<ListHeaderSegment::SortDirection>
{
public:
    typedef ListHeaderSegment::SortDirection return_type;
    typedef return_type safe_method_return_type;
    typedef ListHeaderSegment::SortDirection pass_type;
    typedef String string_return_type;

    static const String& getDataTypeName()
    {
        static String type("SortDirection");

        return type;
    }

    static return_type fromString(const String& str)
    {
        if (str == "Ascending")
        {
            return ListHeaderSegment::Ascending;
        }
        else if (str == "Descending")
        {
            return ListHeaderSegment::Descending;
        }
        else
        {
            return ListHeaderSegment::None;
        }
    }

    static string_return_type toString(pass_type val)
    {
        if (val == ListHeaderSegment::None)
        {
            return "None";
        }
        else if (val == ListHeaderSegment::Ascending)
        {
            return "Ascending";
        }
        else if (val == ListHeaderSegment::Descending)
        {
            return "Descending";
        }
        else
        {
            assert(false && "Invalid Sort Direction");
            return "Ascending";
        }
    }
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUIListHeaderSegment_h_
