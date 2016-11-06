/***********************************************************************
	created:	13/4/2004
	author:		Paul D Turner
	
	purpose:	Interface to base class for FrameWindow
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
#ifndef _CEGUIFrameWindow_h_
#define _CEGUIFrameWindow_h_

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
	Abstract base class for a movable, sizable, window with a title-bar and a frame.
*/
class CEGUIEXPORT FrameWindow : public Window
{
public:
	static const String EventNamespace;				//!< Namespace for global events
    static const String WidgetTypeName;             //!< Window factory name

	/*************************************************************************
		Constants	
	*************************************************************************/
	// additional event names for this window
    /** Event fired when the rollup (shade) state of the window is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the FrameWindow whose rolled up state
     * has been changed.
     */
	static const String EventRollupToggled;
    /** Event fired when the close button for the window is clicked.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the FrameWindow whose close button was
     * clicked.
     */
	static const String EventCloseClicked;
    /** Event fired when drag-sizing of the window starts.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the FrameWindow that has started to be
     * drag sized.
     */
    static const String EventDragSizingStarted;
    /** Event fired when drag-sizing of the window ends.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the FrameWindow for which drag sizing has
     * ended.
     */
    static const String EventDragSizingEnded;

	// other bits
	static const float	DefaultSizingBorderSize;	//!< Default size for the sizing border (in pixels)

    /*************************************************************************
        Child Widget name constants
    *************************************************************************/
    static const String TitlebarName;      //!< Widget name for the titlebar component.
    static const String CloseButtonName;   //!< Widget name for the close button component.


	/*!
	\brief
		Enumeration that defines the set of possible locations for the mouse on a frame windows sizing border.
	*/
	enum SizingLocation {
		SizingNone,			//!< Position is not a sizing location.
		SizingTopLeft,		//!< Position will size from the top-left.
		SizingTopRight,		//!< Position will size from the top-right.
		SizingBottomLeft,	//!< Position will size from the bottom left.
		SizingBottomRight,	//!< Position will size from the bottom right.
		SizingTop,			//!< Position will size from the top.
		SizingLeft,			//!< Position will size from the left.
		SizingBottom,		//!< Position will size from the bottom.
		SizingRight         //!< Position will size from the right.
	};

	/*!
	\brief
		Initialises the Window based object ready for use.

	\note
		This must be called for every window created.  Normally this is handled automatically by the WindowFactory for each Window type.

	\return
		Nothing
	*/
	virtual void	initialiseComponents(void);
	
	
	/*!
	\brief
		Return whether this window is sizable.  Note that this requires that the window have an enabled frame and that sizing itself is enabled

	\return
		true if the window can be sized, false if the window can not be sized
	*/
	bool	isSizingEnabled(void) const					{return d_sizingEnabled && isFrameEnabled();}


	/*!
	\brief
		Return whether the frame for this window is enabled.

	\return
		true if the frame for this window is enabled, false if the frame for this window is disabled.
	*/
	bool	isFrameEnabled(void) const					{return d_frameEnabled;}


	/*!
	\brief
		Return whether the title bar for this window is enabled.

	\return
		true if the window has a title bar and it is enabled, false if the window has no title bar or if the title bar is disabled.
	*/	
	bool	isTitleBarEnabled(void) const;


	/*!
	\brief
		Return whether this close button for this window is enabled.

	\return
		true if the window has a close button and it is enabled, false if the window either has no close button or if the close button is disabled.
	*/
	bool	isCloseButtonEnabled(void) const;


	/*!
	\brief
		Return whether roll up (a.k.a shading) is enabled for this window.

	\return
		true if roll up is enabled, false if roll up is disabled.
	*/
	bool	isRollupEnabled(void) const					{return d_rollupEnabled;}

    /*!
    \brief
        Sets whether the window is currently rolled up (a.k.a shaded).

    \see
        Window::isRolledup
    */
    void    setRolledup(bool val);

	/*!
	\brief
		Return whether the window is currently rolled up (a.k.a shaded).

	\return
		true if the window is rolled up, false if the window is not rolled up.
	*/
	bool	isRolledup(void) const						{return d_rolledup;}


	/*!
	\brief
		Return the thickness of the sizing border.

	\return
		float value describing the thickness of the sizing border in screen pixels.
	*/
	float	getSizingBorderThickness(void) const		{return d_borderSize;}


	/*!
	\brief
		Enables or disables sizing for this window.

	\param setting
		set to true to enable sizing (also requires frame to be enabled), or false to disable sizing.

	\return
		nothing
	*/
	void	setSizingEnabled(bool setting);


	/*!
	\brief
		Enables or disables the frame for this window.

	\param setting
		set to true to enable the frame for this window, or false to disable the frame for this window.

	\return
		Nothing.
	*/
	void	setFrameEnabled(bool setting);


	/*!
	\brief
		Enables or disables the title bar for the frame window.

	\param setting
		set to true to enable the title bar (if one is attached), or false to disable the title bar.

	\return
		Nothing.
	*/
	void	setTitleBarEnabled(bool setting);


	/*!
	\brief
		Enables or disables the close button for the frame window.

	\param setting
		Set to true to enable the close button (if one is attached), or false to disable the close button.

	\return
		Nothing.
	*/
	void	setCloseButtonEnabled(bool setting);


	/*!
	\brief
		Enables or disables roll-up (shading) for this window.

	\param setting
		Set to true to enable roll-up for the frame window, or false to disable roll-up.

	\return
		Nothing.
	*/
	void	setRollupEnabled(bool setting);


	/*!
	\brief
		Toggles the state of the window between rolled-up (shaded) and normal sizes.  This requires roll-up to be enabled.

	\return
		Nothing
	*/
	void	toggleRollup(void);

	/*!
	\brief
		Set the size of the sizing border for this window.

	\param pixels
		float value specifying the thickness for the sizing border in screen pixels.

	\return
		Nothing.
	*/
	void	setSizingBorderThickness(float pixels)		{d_borderSize = pixels;}


	/*!
	\brief
		Move the window by the pixel offsets specified in \a offset.

		This is intended for internal system use - it is the method by which the title bar moves the frame window.

	\param offset
		Vector2 object containing the offsets to apply (offsets are in screen pixels).

	\return
		Nothing.
	*/
	void	offsetPixelPosition(const Vector2f& offset);


	/*!
	\brief
		Return whether this FrameWindow can be moved by dragging the title bar.

	\return
		true if the Window will move when the user drags the title bar, false if the window will not move.
	*/
	bool	isDragMovingEnabled(void) const		{return d_dragMovable;}


	/*!
	\brief
		Set whether this FrameWindow can be moved by dragging the title bar.

	\param setting
		true if the Window should move when the user drags the title bar, false if the window should not move.

	\return
		Nothing.
	*/
	void	setDragMovingEnabled(bool setting);


    /*!
    \brief
        Return a pointer to the currently set Image to be used for the north-south
        sizing mouse cursor.

    \return
        Pointer to an Image object, or 0 for none.
    */
    const Image* getNSSizingCursorImage() const;

    /*!
    \brief
        Return a pointer to the currently set Image to be used for the east-west
        sizing mouse cursor.

    \return
        Pointer to an Image object, or 0 for none.
    */
    const Image* getEWSizingCursorImage() const;

    /*!
    \brief
        Return a pointer to the currently set Image to be used for the northwest-southeast
        sizing mouse cursor.

    \return
        Pointer to an Image object, or 0 for none.
    */
    const Image* getNWSESizingCursorImage() const;

    /*!
    \brief
        Return a pointer to the currently set Image to be used for the northeast-southwest
        sizing mouse cursor.

    \return
        Pointer to an Image object, or 0 for none.
    */
    const Image* getNESWSizingCursorImage() const;

    /*!
    \brief
        Set the Image to be used for the north-south sizing mouse cursor.

    \param image
        Pointer to an Image object, or 0 for none.

    \return
        Nothing.
    */
    void setNSSizingCursorImage(const Image* image);

    /*!
    \brief
        Set the Image to be used for the east-west sizing mouse cursor.

    \param image
        Pointer to an Image object, or 0 for none.

    \return
        Nothing.
    */
    void setEWSizingCursorImage(const Image* image);

    /*!
    \brief
        Set the Image to be used for the northwest-southeast sizing mouse cursor.

    \param image
        Pointer to an Image object, or 0 for none.

    \return
        Nothing.
    */
    void setNWSESizingCursorImage(const Image* image);

    /*!
    \brief
        Set the Image to be used for the northeast-southwest sizing mouse cursor.

    \param image
        Pointer to an Image object, or 0 for none.

    \return
        Nothing.
    */
    void setNESWSizingCursorImage(const Image* image);

    /*!
    \brief
        Set the image to be used for the north-south sizing mouse cursor.

    \param name
        String holding the name of the Image to be used.

    \return
        Nothing.

    \exception UnknownObjectException thrown if either \a imageset or \a image refer to non-existant entities.
    */
    void setNSSizingCursorImage(const String& name);

    /*!
    \brief
        Set the image to be used for the east-west sizing mouse cursor.

    \param name
        String holding the name of the Image to be used.

    \return
        Nothing.

    \exception UnknownObjectException thrown if either \a imageset or \a image refer to non-existant entities.
    */
    void setEWSizingCursorImage(const String& name);

    /*!
    \brief
        Set the image to be used for the northwest-southeast sizing mouse cursor.

    \param name
        String holding the name of the Image to be used.

    \return
        Nothing.

    \exception UnknownObjectException thrown if either \a imageset or \a image refer to non-existant entities.
    */
    void setNWSESizingCursorImage(const String& name);

    /*!
    \brief
        Set the image to be used for the northeast-southwest sizing mouse cursor.

    \param name
        String holding the name of the Image to be used.

    \return
        Nothing.

    \exception UnknownObjectException thrown if either \a imageset or \a image refer to non-existant entities.
    */
    void setNESWSizingCursorImage(const String& name);

    // overridden from Window class
    bool isHit(const Vector2f& position, const bool /*allow_disabled*/) const
        { return Window::isHit(position) && !d_rolledup; }

    /*!
    \brief
        Return a pointer to the Titlebar component widget for this FrameWindow.

    \return
        Pointer to a Titlebar object.

    \exception UnknownObjectException
        Thrown if the Titlebar component does not exist.
    */
    Titlebar* getTitlebar() const;

    /*!
    \brief
        Return a pointer to the close button component widget for this
        FrameWindow.

    \return
        Pointer to a PushButton object.

    \exception UnknownObjectException
        Thrown if the close button component does not exist.
    */
    PushButton* getCloseButton() const;

	/*************************************************************************
		Construction / Destruction
	*************************************************************************/
	/*!
	\brief
		Constructor for FrameWindow objects.
	*/
	FrameWindow(const String& type, const String& name);

	/*!
	\brief
		Destructor for FramwWindow objects.
	*/
	virtual ~FrameWindow(void);


protected:
	/*************************************************************************
		Implementation Functions
	*************************************************************************/
	/*!
	\brief
		move the window's left edge by 'delta'.  The rest of the window does not move, thus this changes the size of the Window.

	\param delta
		float value that specifies the amount to move the window edge, and in which direction.  Positive values make window smaller.
	*/
	bool	moveLeftEdge(float delta, URect& out_area);


	/*!
	\brief
		move the window's right edge by 'delta'.  The rest of the window does not move, thus this changes the size of the Window.

	\param delta
		float value that specifies the amount to move the window edge, and in which direction.  Positive values make window larger.
	*/
	bool	moveRightEdge(float delta, URect& out_area);


	/*!
	\brief
		move the window's top edge by 'delta'.  The rest of the window does not move, thus this changes the size of the Window.

	\param delta
		float value that specifies the amount to move the window edge, and in which direction.  Positive values make window smaller.
	*/
	bool	moveTopEdge(float delta, URect& out_area);


	/*!
	\brief
		move the window's bottom edge by 'delta'.  The rest of the window does not move, thus this changes the size of the Window.

	\param delta
		float value that specifies the amount to move the window edge, and in which direction.  Positive values make window larger.
	*/
	bool	moveBottomEdge(float delta, URect& out_area);


	/*!
	\brief
		check local pixel co-ordinate point 'pt' and return one of the
		SizingLocation enumerated values depending where the point falls on
		the sizing border.

	\param pt
		Point object describing, in pixels, the window relative offset to check.

	\return
		One of the SizingLocation enumerated values that describe which part of
		the sizing border that \a pt corresponded to, if any.
	*/
	SizingLocation	getSizingBorderAtPoint(const Vector2f& pt) const;

 
	/*!
	\brief
		return true if given SizingLocation is on left edge.

	\param loc
		SizingLocation value to be checked.

	\return
		true if \a loc is on the left edge.  false if \a loc is not on the left edge.
	*/
	bool	isLeftSizingLocation(SizingLocation loc) const			{return ((loc == SizingLeft) || (loc == SizingTopLeft) || (loc == SizingBottomLeft));}


	/*!
	\brief
		return true if given SizingLocation is on right edge.

	\param loc
		SizingLocation value to be checked.

	\return
		true if \a loc is on the right edge.  false if \a loc is not on the right edge.
	*/
	bool	isRightSizingLocation(SizingLocation loc) const			{return ((loc == SizingRight) || (loc == SizingTopRight) || (loc == SizingBottomRight));}


	/*!
	\brief
		return true if given SizingLocation is on top edge.

	\param loc
		SizingLocation value to be checked.

	\return
		true if \a loc is on the top edge.  false if \a loc is not on the top edge.
	*/
	bool	isTopSizingLocation(SizingLocation loc) const			{return ((loc == SizingTop) || (loc == SizingTopLeft) || (loc == SizingTopRight));}


	/*!
	\brief
		return true if given SizingLocation is on bottom edge.

	\param loc
		SizingLocation value to be checked.

	\return
		true if \a loc is on the bottom edge.  false if \a loc is not on the bottom edge.
	*/
	bool	isBottomSizingLocation(SizingLocation loc) const		{return ((loc == SizingBottom) || (loc == SizingBottomLeft) || (loc == SizingBottomRight));}


	/*!
	\brief
		Method to respond to close button click events and fire our close event
	*/
	bool	closeClickHandler(const EventArgs& e);


	/*!
	\brief
		Set the appropriate mouse cursor for the given window-relative pixel point.
	*/
	void	setCursorForPoint(const Vector2f& pt) const;


	/*!
	\brief
		Return a Rect that describes, in window relative pixel co-ordinates, the outer edge of the sizing area for this window.
	*/
	virtual	Rectf	getSizingRect(void) const		{return Rectf(0, 0, d_pixelSize.d_width, d_pixelSize.d_height);}

	/*************************************************************************
		New events for Frame Windows
	*************************************************************************/
	/*!
	\brief
		Event generated internally whenever the roll-up / shade state of the window
		changes.
	*/
	virtual void	onRollupToggled(WindowEventArgs& e);


	/*!
	\brief
		Event generated internally whenever the close button is clicked.
	*/
	virtual void	onCloseClicked(WindowEventArgs& e);

    //! Handler called when drag-sizing of the FrameWindow starts.
    virtual void onDragSizingStarted(WindowEventArgs& e);

    //! Handler called when drag-sizing of the FrameWindow ends.
    virtual void onDragSizingEnded(WindowEventArgs& e);

	/*************************************************************************
		Overridden event handlers
	*************************************************************************/
	virtual void	onMouseMove(MouseEventArgs& e);
	virtual void	onMouseButtonDown(MouseEventArgs& e);
	virtual void	onMouseButtonUp(MouseEventArgs& e);
	virtual void	onCaptureLost(WindowEventArgs& e);
	virtual void    onTextChanged(WindowEventArgs& e);
	virtual void	onActivated(ActivationEventArgs& e);
	virtual void	onDeactivated(ActivationEventArgs& e);


	/*************************************************************************
		Implementation Data
	*************************************************************************/
	// frame data
	bool	d_frameEnabled;		//!< true if window frame should be drawn.

	// window roll-up data
	bool	d_rollupEnabled;	//!< true if roll-up of window is allowed.
	bool	d_rolledup;			//!< true if window is rolled up.

	// drag-sizing data
	bool	d_sizingEnabled;	//!< true if sizing is enabled for this window.
	bool	d_beingSized;		//!< true if window is being sized.
	float	d_borderSize;		//!< thickness of the sizing border around this window
	Vector2f d_dragPoint;		//!< point window is being dragged at.

	// images for cursor when on sizing border
	const Image*	d_nsSizingCursor;		//!< North/South sizing cursor image.
	const Image*	d_ewSizingCursor;		//!< East/West sizing cursor image.
	const Image*	d_nwseSizingCursor;		//!< North-West/South-East cursor image.
	const Image*	d_neswSizingCursor;		//!< North-East/South-West cursor image.

	bool	d_dragMovable;		//!< true if the window will move when dragged by the title bar.


private:
	/*************************************************************************
		Private methods
	*************************************************************************/
	void	addFrameWindowProperties(void);
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUIFrameWindow_h_

