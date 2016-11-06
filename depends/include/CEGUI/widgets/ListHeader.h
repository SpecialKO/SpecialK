/***********************************************************************
	created:	13/4/2004
	author:		Paul D Turner
	
	purpose:	Interface to base class for ListHeader widget
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
#ifndef _CEGUIListHeader_h_
#define _CEGUIListHeader_h_

#include "../Base.h"
#include "../Window.h"
#include "./ListHeaderSegment.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif


// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
	EventArgs class used for segment move (sequence changed) events.
*/
class CEGUIEXPORT HeaderSequenceEventArgs : public WindowEventArgs
{
public:
	HeaderSequenceEventArgs(Window* wnd, uint old_idx, uint new_idx) : WindowEventArgs(wnd), d_oldIdx(old_idx), d_newIdx(new_idx) {};

	uint d_oldIdx;		//!< The original column index of the segment that has moved.
	uint d_newIdx;		//!< The new column index of the segment that has moved.
};

/*!
\brief
    Base class for the multi column list header window renderer.
*/
class CEGUIEXPORT ListHeaderWindowRenderer : public WindowRenderer
{
public:
    /*!
    \brief
        Constructor
    */
    ListHeaderWindowRenderer(const String& name);

    /*!
    \brief
        Create and return a pointer to a new ListHeaderSegment based object.

    \param name
        String object holding the name that should be given to the new Window.

    \return
        Pointer to an ListHeaderSegment based object of whatever type is appropriate for
        this ListHeader.
    */
    virtual ListHeaderSegment*  createNewSegment(const String& name) const  = 0;

    /*!
    \brief
        Cleanup and destroy the given ListHeaderSegment that was created via the
        createNewSegment method.

    \param segment
        Pointer to a ListHeaderSegment based object to be destroyed.

    \return
        Nothing.
    */
    virtual void    destroyListSegment(ListHeaderSegment* segment) const = 0;
};


/*!
\brief
	Base class for the multi column list header widget.
*/
class CEGUIEXPORT ListHeader : public Window
{
public:
	static const String EventNamespace;				//!< Namespace for global events
    static const String WidgetTypeName;             //!< Window factory name


	/*************************************************************************
		Constants
	*************************************************************************/
	// Event names
    /** Event fired when the current sort column of the header is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ListHeader whose sort column has
     * been changed.
     */
	static const String EventSortColumnChanged;
    /** Event fired when the sort direction of the header is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ListHeader whose sort direction had
     * been changed.
     */
	static const String EventSortDirectionChanged;
    /** Event fired when a segment of the header is sized by the user.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ListHeaderSegment that has been sized.
     */
	static const String EventSegmentSized;
    /** Event fired when a segment of the header is clicked by the user.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ListHeaderSegment that was clicked.
     */
	static const String EventSegmentClicked;
    /** Event fired when a segment splitter of the header is double-clicked.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ListHeaderSegment whose splitter area
     * was double-clicked.
     */
	static const String EventSplitterDoubleClicked;
    /** Event fired when the order of the segments in the header has changed.
     * Handlers are passed a const HeaderSequenceEventArgs reference with
     * WindowEventArgs::window set to the ListHeader whose segments have changed
     * sequence, HeaderSequenceEventArgs::d_oldIdx is the original index of the
     * segment that has moved, and HeaderSequenceEventArgs::d_newIdx is the new
     * index of the segment that has moved.
     */
	static const String EventSegmentSequenceChanged;
    /** Event fired when a segment is added to the header.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ListHeader that has had a new segment
     * added.
     */
	static const String EventSegmentAdded;
    /** Event fired when a segment is removed from the header.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ListHeader that has had a segment
     * removed.
     */
	static const String EventSegmentRemoved;
    /** Event fired when setting that controls user modification to sort
     * configuration is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ListHeader whose user sort control
     * setting has been changed.
     */
	static const String EventSortSettingChanged;
    /** Event fired when setting that controls user drag & drop of segments is
     * changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ListHeader whose drag & drop enabled
     * setting has changed.
     */
	static const String EventDragMoveSettingChanged;
    /** Event fired when setting that controls user sizing of segments is
     * changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ListHeader whose user sizing setting
     * has changed.
     */
	static const String EventDragSizeSettingChanged;
    /** Event fired when the rendering offset for the segments changes.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ListHeader whose segment rendering
     * offset has changed.
     */
	static const String EventSegmentRenderOffsetChanged;

	// values
	static const float	ScrollSpeed;				//!< Speed to scroll at when dragging outside header.
	static const float	MinimumSegmentPixelWidth;	//!< Miniumum width of a segment in pixels.

    /*************************************************************************
        Child Widget name suffix constants
    *************************************************************************/
    //! Widget name suffix for header segments.
    static const String SegmentNameSuffix;


	/*************************************************************************
		Accessor Methods
	*************************************************************************/
	/*!
	\brief
		Return the number of columns or segments attached to the header.

	\return
		uint value equal to the number of columns / segments currently in the header.
	*/
	uint	getColumnCount(void) const;

	
	/*!
	\brief
		Return the ListHeaderSegment object for the specified column

	\param column
		zero based column index of the ListHeaderSegment to be returned.

	\return
		ListHeaderSegment object at the requested index.

	\exception InvalidRequestException	thrown if column is out of range.
	*/
	ListHeaderSegment&	getSegmentFromColumn(uint column) const;


	/*!
	\brief
		Return the ListHeaderSegment object with the specified ID.

	\param id
		id code of the ListHeaderSegment to be returned.

	\return
		ListHeaderSegment object with the ID \a id.  If more than one segment has the same ID, only the first one will
		ever be returned.

	\exception	InvalidRequestException		thrown if no segment with the requested ID is attached.
	*/
	ListHeaderSegment&	getSegmentFromID(uint id) const;


	/*!
	\brief
		Return the ListHeaderSegment that is marked as being the 'sort key' segment.  There must be at least one segment
		to successfully call this method.

	\return
		ListHeaderSegment object which is the sort-key segment.

	\exception	InvalidRequestException		thrown if no segments are attached to the ListHeader.
	*/
	ListHeaderSegment&	getSortSegment(void) const;
	/*!
	\brief
		Return the ListHeaderSegment ID that is marked as being the 'sort key' segment.  There must be at least one segment
		to successfully call this method.

	\return
		uint which is the sort-key segment ID.

	\exception	InvalidRequestException		thrown if no segments are attached to the ListHeader.
	*/
	uint	getSortSegmentID(void) const;


	/*!
	\brief
		Return the zero based column index of the specified segment.

	\param segment
		ListHeaderSegment whos zero based index is to be returned.

	\return
		Zero based column index of the ListHeaderSegment \a segment.

	\exception	InvalidRequestException		thrown if \a segment is not attached to this ListHeader.
	*/
	uint	getColumnFromSegment(const ListHeaderSegment& segment) const;


	/*!
	\brief
		Return the zero based column index of the segment with the specified ID.

	\param id
		ID code of the segment whos column index is to be returned.

	\return
		Zero based column index of the first ListHeaderSegment whos ID matches \a id.

	\exception	InvalidRequestException		thrown if no attached segment has the requested ID.
	*/
	uint	getColumnFromID(uint id) const;


	/*!
	\brief
		Return the zero based index of the current sort column.  There must be at least one segment/column to successfully call this
		method.

	\return
		Zero based column index that is the current sort column.

	\exception	InvalidRequestException		thrown if there are no segments / columns in this ListHeader.
	*/
	uint	getSortColumn(void) const;


	/*!
	\brief
		Return the zero based column index of the segment with the specified text.

	\param text
		String object containing the text to be searched for.

	\return
		Zero based column index of the segment with the specified text.

	\exception InvalidRequestException	thrown if no attached segments have the requested text.
	*/
	uint	getColumnWithText(const String& text) const;


	/*!
	\brief
		Return the pixel offset to the given ListHeaderSegment.

	\param segment
		ListHeaderSegment object that the offset to is to be returned.

	\return
		The number of pixels up-to the begining of the ListHeaderSegment described by \a segment.

	\exception InvalidRequestException	thrown if \a segment is not attached to the ListHeader.
	*/
	float	getPixelOffsetToSegment(const ListHeaderSegment& segment) const;


	/*!
	\brief
		Return the pixel offset to the ListHeaderSegment at the given zero based column index.

	\param column
		Zero based column index of the ListHeaderSegment whos pixel offset it to be returned.

	\return
		The number of pixels up-to the begining of the ListHeaderSegment located at zero based column
		index \a column.

	\exception InvalidRequestException	thrown if \a column is out of range.
	*/
	float	getPixelOffsetToColumn(uint column) const;


	/*!
	\brief
		Return the total pixel width of all attached segments.

	\return
		Sum of the pixel widths of all attached ListHeaderSegment objects.
	*/
	float	getTotalSegmentsPixelExtent(void) const;


	/*!
	\brief
		Return the width of the specified column.

	\param column
		Zero based column index of the segment whose width is to be returned.

	\return
		UDim describing the width of the ListHeaderSegment at the zero based
        column index specified by \a column.

	\exception InvalidRequestException	thrown if \a column is out of range.
	*/
	UDim getColumnWidth(uint column) const;


	/*!
	\brief
		Return the currently set sort direction.

	\return
		One of the ListHeaderSegment::SortDirection enumerated values specifying the current sort direction.
	*/
	ListHeaderSegment::SortDirection	getSortDirection(void) const;


	/*!
	\brief
		Return whether user manipulation of the sort column & direction are enabled.

	\return
		true if the user may interactively modify the sort column and direction.  false if the user may not
		modify the sort column and direction (these can still be set programmatically).
	*/
	bool	isSortingEnabled(void) const;


	/*!
	\brief
		Return whether the user may size column segments.

	\return
		true if the user may interactively modify the width of column segments, false if they may not.
	*/
	bool	isColumnSizingEnabled(void) const;


	/*!
	\brief
		Return whether the user may modify the order of the segments.

	\return
		true if the user may interactively modify the order of the column segments, false if they may not.
	*/
	bool	isColumnDraggingEnabled(void) const;


	/*!
	\brief
		Return the current segment offset value.  This value is used to implement scrolling of the header segments within
		the ListHeader area.

	\return
		float value specifying the current segment offset value in whatever metrics system is active for the ListHeader.
	*/
	float	getSegmentOffset(void) const	{return d_segmentOffset;}


	/*************************************************************************
		Manipulator Methods
	*************************************************************************/
	/*!
	\brief
		Set whether user manipulation of the sort column and direction is enabled.

	\param setting
		- true to allow interactive user manipulation of the sort column and direction.
		- false to disallow interactive user manipulation of the sort column and direction.

	\return
		Nothing.
	*/
	void	setSortingEnabled(bool setting);


	/*!
	\brief
		Set the current sort direction.

	\param direction
		One of the ListHeaderSegment::SortDirection enumerated values indicating the sort direction to be used.

	\return
		Nothing.
	*/
	void	setSortDirection(ListHeaderSegment::SortDirection direction);


	/*!
	\brief
		Set the column segment to be used as the sort column.

	\param segment
		ListHeaderSegment object indicating the column to be sorted.

	\return
		Nothing.

	\exception InvalidRequestException	thrown if \a segment is not attached to this ListHeader.
	*/
	void	setSortSegment(const ListHeaderSegment& segment);


	/*!
	\brief
		Set the column to be used as the sort column.

	\param column
		Zero based column index indicating the column to be sorted.

	\return
		Nothing.

	\exception InvalidRequestException	thrown if \a column is out of range for this ListHeader.
	*/
	void	setSortColumn(uint column);


	/*!
	\brief
		Set the column to to be used for sorting via its ID code.

	\param id
		ID code of the column segment that is to be used as the sort column.

	\return
		Nothing.

	\exception InvalidRequestException	thrown if no segment with ID \a id is attached to the ListHeader.
	*/
	void	setSortColumnFromID(uint id);


	/*!
	\brief
		Set whether columns may be sized by the user.

	\param setting
		- true to indicate that the user may interactively size segments.
		- false to indicate that the user may not interactively size segments.

	\return
		Nothing.
	*/
	void	setColumnSizingEnabled(bool setting);


	/*!
	\brief
		Set whether columns may be reordered by the user via drag and drop.

	\param setting
		- true to indicate the user may change the order of the column segments via drag and drop.
		- false to indicate the user may not change the column segment order.

	\return
		Nothing.
	*/
	void	setColumnDraggingEnabled(bool setting);


	/*!
	\brief
		Add a new column segment to the end of the header.

	\param text
		String object holding the initial text for the new segment

	\param id
		Client specified ID code to be assigned to the new segment.

	\param width
		UDim describing the initial width of the new segment.

	\return
		Nothing.
	*/
	void	addColumn(const String& text, uint id, const UDim& width);


	/*!
	\brief
		Insert a new column segment at the specified position.

	\param text
		String object holding the initial text for the new segment

	\param id
		Client specified ID code to be assigned to the new segment.

    \param width
        UDim describing the initial width of the new segment.

	\param position
		Zero based column index indicating the desired position for the new column.  If this is greater than
		the current number of columns, the new segment is added to the end if the header.

	\return
		Nothing.
	*/
	void	insertColumn(const String& text, uint id, const UDim& width, uint position);


	/*!
	\brief
		Insert a new column segment at the specified position.

	\param text
		String object holding the initial text for the new segment

	\param id
		Client specified ID code to be assigned to the new segment.

    \param width
        UDim describing the initial width of the new segment.

	\param position
		ListHeaderSegment object indicating the insert position for the new segment.  The new segment will be
		inserted before the segment indicated by \a position.

	\return
		Nothing.

	\exception InvalidRequestException	thrown if ListHeaderSegment \a position is not attached to the ListHeader.
	*/
	void	insertColumn(const String& text, uint id, const UDim& width, const ListHeaderSegment& position);


	/*!
	\brief
		Removes a column segment from the ListHeader.

	\param column
		Zero based column index indicating the segment to be removed.

	\return
		Nothing.

	\exception InvalidRequestException	thrown if \a column is out of range.
	*/
	void	removeColumn(uint column);


	/*!
	\brief
		Remove the specified segment from the ListHeader.

	\param segment
		ListHeaderSegment object that is to be removed from the ListHeader.

	\return
		Nothing.

	\exception InvalidRequestException	thrown if \a segment is not attached to this ListHeader.
	*/
	void	removeSegment(const ListHeaderSegment& segment);


	/*!
	\brief
		Moves a column segment into a new position.

	\param column
		Zero based column index indicating the column segment to be moved.

	\param position
		Zero based column index indicating the new position for the segment.  If this is greater than the current number of segments,
		the segment is moved to the end of the header.

	\return
		Nothing.

	\exception InvalidRequestException thrown if \a column is out of range for this ListHeader.
	*/
	void	moveColumn(uint column, uint position);


	/*!
	\brief
		Move a column segment to a new position.

	\param column
		Zero based column index indicating the column segment to be moved.

	\param position
		ListHeaderSegment object indicating the new position for the segment.  The segment at \a column
		will be moved behind segment \a position (that is, segment \a column will appear to the right of
		segment \a position).

	\return
		Nothing.

	\exception InvalidRequestException thrown if \a column is out of range for this ListHeader, or if \a position
		is not attached to this ListHeader.
	*/
	void	moveColumn(uint column, const ListHeaderSegment& position);


	/*!
	\brief
		Moves a segment into a new position.

	\param segment
		ListHeaderSegment object that is to be moved.

	\param position
		Zero based column index indicating the new position for the segment.  If this is greater than the current number of segments,
		the segment is moved to the end of the header.

	\return
		Nothing.

	\exception InvalidRequestException thrown if \a segment is not attached to this ListHeader.
	*/
	void	moveSegment(const ListHeaderSegment& segment, uint position);


	/*!
	\brief
		Move a segment to a new position.

	\param segment
		ListHeaderSegment object that is to be moved.

	\param position
		ListHeaderSegment object indicating the new position for the segment.  The segment \a segment
		will be moved behind segment \a position (that is, segment \a segment will appear to the right of
		segment \a position).

	\return
		Nothing.

	\exception InvalidRequestException thrown if either \a segment or \a position are not attached to this ListHeader.
	*/
	void	moveSegment(const ListHeaderSegment& segment, const ListHeaderSegment& position);


	/*!
	\brief
		Set the current base segment offset.  (This implements scrolling of the header segments within
		the header area).

	\param offset
		New base offset for the first segment.  The segments will of offset to the left by the amount specified.
		\a offset should be specified using the active metrics system for the ListHeader.

	\return
		Nothing.
	*/
	void	setSegmentOffset(float offset);


	/*!
	\brief
		Set the width of the specified column.

	\param column
		Zero based column index of the segment whose width is to be set.

	\param width
		UDim value specifying the new width to set for the ListHeaderSegment at the zero based column
		index specified by \a column.

	\return
		Nothing

	\exception InvalidRequestException	thrown if \a column is out of range.
	*/
	void	setColumnWidth(uint column, const UDim& width);


	/*************************************************************************
		Construction and Destruction
	*************************************************************************/
	/*!
	\brief
		Constructor for the list header base class.
	*/
	ListHeader(const String& type, const String& name);


	/*!
	\brief
		Destructor for the list header base class.
	*/
	virtual ~ListHeader(void);


protected:
	/*************************************************************************
		Abstract Implementation Methods
	*************************************************************************/
	/*!
	\brief
		Create and return a pointer to a new ListHeaderSegment based object.

	\param name
		String object holding the name that should be given to the new Window.

	\return
		Pointer to an ListHeaderSegment based object of whatever type is appropriate for
		this ListHeader.
	*/
	//virtual ListHeaderSegment*	createNewSegment_impl(const String& name) const	= 0;


	/*!
	\brief
		Cleanup and destroy the given ListHeaderSegment that was created via the
		createNewSegment method.

	\param segment
		Pointer to a ListHeaderSegment based object to be destroyed.

	\return
		Nothing.
	*/
	//virtual void	destroyListSegment_impl(ListHeaderSegment* segment) const = 0;


	/*************************************************************************
		Implementation Methods
	*************************************************************************/
	/*!
	\brief
		Create initialise and return a ListHeaderSegment object, with all events subscribed and ready to use.
	*/
	ListHeaderSegment*	createInitialisedSegment(const String& text, uint id, const UDim& width);


	/*!
	\brief
		Layout the attached segments
	*/
	void	layoutSegments(void);

    /*!
    \brief
        Create and return a pointer to a new ListHeaderSegment based object.

    \param name
        String object holding the name that should be given to the new Window.

    \return
        Pointer to an ListHeaderSegment based object of whatever type is appropriate for
        this ListHeader.
    */
    ListHeaderSegment*  createNewSegment(const String& name) const;


    /*!
    \brief
        Cleanup and destroy the given ListHeaderSegment that was created via the
        createNewSegment method.

    \param segment
        Pointer to a ListHeaderSegment based object to be destroyed.

    \return
        Nothing.
    */
    void    destroyListSegment(ListHeaderSegment* segment) const;

    // validate window renderer
    virtual bool validateWindowRenderer(const WindowRenderer* renderer) const;

	/*************************************************************************
		New List header event handlers
	*************************************************************************/
	/*!
	\brief
		Handler called when the sort column is changed.
	*/
	virtual	void	onSortColumnChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called when the sort direction is changed.
	*/
	virtual	void	onSortDirectionChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called when a segment is sized by the user.  e.window points to the segment.
	*/
	virtual	void	onSegmentSized(WindowEventArgs& e);


	/*!
	\brief
		Handler called when a segment is clicked by the user.  e.window points to the segment.
	*/
	virtual	void	onSegmentClicked(WindowEventArgs& e);


	/*!
	\brief
		Handler called when a segment splitter / sizer is double-clicked.  e.window points to the segment.
	*/
	virtual	void	onSplitterDoubleClicked(WindowEventArgs& e);


	/*!
	\brief
		Handler called when the segment / column order changes.
	*/
	virtual	void	onSegmentSequenceChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called when a new segment is added to the header.
	*/
	virtual	void	onSegmentAdded(WindowEventArgs& e);


	/*!
	\brief
		Handler called when a segment is removed from the header.
	*/
	virtual	void	onSegmentRemoved(WindowEventArgs& e);


	/*!
	\brief
		Handler called then setting that controls the users ability to modify the search column & direction changes.
	*/
	virtual	void	onSortSettingChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called when the setting that controls the users ability to drag and drop segments changes.
	*/
	virtual	void	onDragMoveSettingChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called when the setting that controls the users ability to size segments changes.
	*/
	virtual	void	onDragSizeSettingChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called when the base rendering offset for the segments (scroll position) changes.
	*/
	virtual	void	onSegmentOffsetChanged(WindowEventArgs& e);

	/*************************************************************************
		handlers for events we subscribe to from segments
	*************************************************************************/
	bool	segmentSizedHandler(const EventArgs& e);
	bool	segmentMovedHandler(const EventArgs& e);
	bool	segmentClickedHandler(const EventArgs& e);
	bool	segmentDoubleClickHandler(const EventArgs& e);
	bool	segmentDragHandler(const EventArgs& e);


	/*************************************************************************
		Implementation Data
	*************************************************************************/
	typedef	std::vector<ListHeaderSegment*
        CEGUI_VECTOR_ALLOC(ListHeaderSegment*)> SegmentList;
	SegmentList	d_segments;			//!< Attached segment windows in header order.
	ListHeaderSegment*	d_sortSegment;	//!< Pointer to the segment that is currently set as the sork-key,
	bool	d_sizingEnabled;		//!< true if segments can be sized by the user.
	bool	d_sortingEnabled;		//!< true if the sort criteria modifications by user are enabled (no sorting is actuall done)
	bool	d_movingEnabled;		//!< true if drag & drop moving of columns / segments is enabled.
	uint	d_uniqueIDNumber;		//!< field used to create unique names.
	float	d_segmentOffset;		//!< Base offset used to layout the segments (allows scrolling within the window area)
	ListHeaderSegment::SortDirection	d_sortDir;		//!< Brief copy of the current sort direction.


private:
	/*************************************************************************
		Private methods
	*************************************************************************/
	void	addHeaderProperties(void);
};


} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUIListHeader_h_
