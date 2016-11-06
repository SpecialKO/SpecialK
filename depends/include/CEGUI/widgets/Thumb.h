/***********************************************************************
	created:	25/4/2004
	author:		Paul D Turner
	
	purpose:	Interface for a 'Thumb' widget.  Intended to be used as
				part of other widgets such as scrollers and sliders.
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
#ifndef _CEGUIThumb_h_
#define _CEGUIThumb_h_

#include "./PushButton.h"
#include <utility>


#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#	pragma warning(disable : 4996)
#endif


// Start of CEGUI namespace section
namespace CEGUI
{

/*!
\brief
	Base class for Thumb widget.
	
	The thumb widget is used to compose other widgets (like sliders and scroll bars).  You would
	not normally need to use this widget directly unless you are making a new widget of some type.
*/
class CEGUIEXPORT Thumb : public PushButton
{
public:
	static const String EventNamespace;				//!< Namespace for global events
    static const String WidgetTypeName;             //!< Window factory name

	/*************************************************************************
		Event name constants
	*************************************************************************/
	// generated internally by Window
    /** Event fired when the position of the thumb widget has changed (this
     * event is only fired when hot tracking is enabled).
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Thumb whose position has changed.
     */
	static const String EventThumbPositionChanged;
    /** Event fired when the user begins dragging the thumb.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Thumb that has started to be dragged
     * by the user.
     */
	static const String EventThumbTrackStarted;
    /** Event fired when the user releases the thumb.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Thumb that has been released.
     */
	static const String EventThumbTrackEnded;


	/*************************************************************************
		Accessor Functions
	*************************************************************************/ 
	/*!
	\brief
		return whether hot-tracking is enabled or not.

	\return
		true if hot-tracking is enabled.  false if hot-tracking is disabled.
	*/
	bool	isHotTracked(void) const			{return d_hotTrack;}

	/*!
	\brief
		return whether the thumb is movable on the vertical axis.

	\return
		true if the thumb is movable along the vertical axis.
		false if the thumb is fixed on the vertical axis.
	*/
	bool	isVertFree(void) const				{return d_vertFree;}

	/*!
	\brief
		return whether the thumb is movable on the horizontal axis.

	\return
		true if the thumb is movable along the horizontal axis.
		false if the thumb is fixed on the horizontal axis.
	*/
	bool	isHorzFree(void) const				{return d_horzFree;}


	/*!
	\brief
		Return a std::pair that describes the current range set for the vertical movement.

	\return
		a std::pair describing the current vertical range.  The first element is the minimum value,
		the second element is the maximum value.
	*/
	std::pair<float, float>	getVertRange(void) const;


	/*!
	\brief
		Return a std::pair that describes the current range set for the horizontal movement.

	\return
		a std::pair describing the current horizontal range.  The first element is the minimum value,
		the second element is the maximum value.
	*/
	std::pair<float, float>	getHorzRange(void) const;


	/*************************************************************************
		Manipulator Functions
	*************************************************************************/
	/*!
	\brief
		set whether the thumb uses hot-tracking.

	\param setting
		true to enable hot-tracking.  false to disable hot-tracking.

	\return
		Nothing.
	*/
	void	setHotTracked(bool setting)				{d_hotTrack = setting;}


	/*!
	\brief
		set whether thumb is movable on the vertical axis.

	\param setting
		true to allow movement of thumb along the vertical axis.  false to fix thumb on the vertical axis.

	\return
		nothing.
	*/
	void	setVertFree(bool setting)						{d_vertFree = setting;}


	/*!
	\brief
		set whether thumb is movable on the horizontal axis.

	\param setting
		true to allow movement of thumb along the horizontal axis.  false to fix thumb on the horizontal axis.

	\return
		nothing.
	*/
	void	setHorzFree(bool setting)						{d_horzFree = setting;}
	

	/*!
	\brief
		set the movement range of the thumb for the vertical axis.

		The values specified here are relative to the parent window for the thumb, and are specified in whichever
		metrics mode is active for the widget.

	\param min
		the minimum setting for the thumb on the vertical axis.

	\param max
		the maximum setting for the thumb on the vertical axis.

	\return
		Nothing.
	*/
	void	setVertRange(float min, float max);

	/*!
	\brief
		set the movement range of the thumb for the vertical axis.

		The values specified here are relative to the parent window for the thumb, and are specified in whichever
		metrics mode is active for the widget.

	\param range
		the setting for the thumb on the vertical axis.

	\return
		Nothing.
	*/
	void	setVertRange(const std::pair<float, float> &range);


	/*!
	\brief
		set the movement range of the thumb for the horizontal axis.

		The values specified here are relative to the parent window for the thumb, and are specified in whichever
		metrics mode is active for the widget.

	\param min
		the minimum setting for the thumb on the horizontal axis.

	\param max
		the maximum setting for the thumb on the horizontal axis.

	\return
		Nothing.
	*/
	void	setHorzRange(float min, float max);
	/*!
	\brief
		set the movement range of the thumb for the horizontal axis.

		The values specified here are relative to the parent window for the thumb, and are specified in whichever
		metrics mode is active for the widget.

	\param range
		the setting for the thumb on the horizontal axis.

	\return
		Nothing.
	*/
	void	setHorzRange(const std::pair<float, float> &range);


	/*************************************************************************
		Construction / Destruction
	*************************************************************************/
	/*!
	\brief
		Constructor for Thumb objects
	*/
	Thumb(const String& type, const String& name);


	/*!
	\brief
		Destructor for Thumb objects
	*/
	virtual ~Thumb(void);


protected:
    // overridden from base class
    void banPropertiesForAutoWindow();

	/*************************************************************************
		New Thumb Events
	*************************************************************************/
	/*!
	\brief
		event triggered internally when the position of the thumb
	*/
	virtual void	onThumbPositionChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler triggered when the user begins to drag the thumb. 
	*/
	virtual void	onThumbTrackStarted(WindowEventArgs& e);


	/*!
	\brief
		Handler triggered when the thumb is released
	*/
	virtual void	onThumbTrackEnded(WindowEventArgs& e);



	/*************************************************************************
		Overridden event handling routines
	*************************************************************************/
	virtual void	onMouseMove(MouseEventArgs& e);
	virtual void	onMouseButtonDown(MouseEventArgs& e);
	virtual void	onCaptureLost(WindowEventArgs& e);


	/*************************************************************************
		Implementation Data
	*************************************************************************/
	// general settings
	bool	d_hotTrack;					//!< true if events are to be sent real-time, else just when thumb is released
	bool	d_vertFree;					//!< true if thumb is movable vertically
	bool	d_horzFree;					//!< true if thumb is movable horizontally

	// operational limits
	float	d_vertMin, d_vertMax;		//!< vertical range
	float	d_horzMin, d_horzMax;		//!< horizontal range

	// internal state
	bool	d_beingDragged;				//!< true if thumb is being dragged
	Vector2f d_dragPoint;				//!< point where we are being dragged at.


private:
	/*************************************************************************
		Private methods
	*************************************************************************/
	void	addThumbProperties(void);
};

/*
TODO: This is horrible, PropertyHelper for std::pair<float, float> would be fine but enforcing min: %f max: %f is just horrible
*/
template<>
class PropertyHelper<std::pair<float,float> >
{
public:
    typedef std::pair<float,float> return_type;
    typedef return_type safe_method_return_type;
    typedef const std::pair<float,float>& pass_type;
    typedef String string_return_type;

    static const String& getDataTypeName()
    {
        static String type("std::pair<float,float>");

        return type;
    }

    static return_type fromString(const String& str)
    {
        float rangeMin = 0, rangeMax = 0;
        sscanf(str.c_str(), " min:%f max:%f", &rangeMin, &rangeMax);
        return std::pair<float,float>(rangeMin,rangeMax);
    }

    static string_return_type toString(pass_type val)
    {
        char buff[64];
        sprintf(buff, "min:%f max:%f", val.first, val.second);
        return buff;
    }
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUIThumb_h_
