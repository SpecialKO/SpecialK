/***********************************************************************
	created:	13/4/2004
	author:		Paul D Turner
	
	purpose:	Interface to base class for Slider widget
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
#ifndef _CEGUISlider_h_
#define _CEGUISlider_h_

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
    Base class for ItemEntry window renderer objects.
*/
class CEGUIEXPORT SliderWindowRenderer : public WindowRenderer
{
public:
    /*!
    \brief
        Constructor
    */
    SliderWindowRenderer(const String& name);

    /*!
    \brief
        update the size and location of the thumb to properly represent the current state of the slider
    */
    virtual void    updateThumb(void)   = 0;

    /*!
    \brief
        return value that best represents current slider value given the current location of the thumb.

    \return
        float value that, given the thumb widget position, best represents the current value for the slider.
    */
    virtual float   getValueFromThumb(void) const   = 0;

    /*!
    \brief
        Given window location \a pt, return a value indicating what change should be 
        made to the slider.

    \param pt
        Point object describing a pixel position in window space.

    \return
        - -1 to indicate slider should be moved to a lower setting.
        -  0 to indicate slider should not be moved.
        - +1 to indicate slider should be moved to a higher setting.
    */
    virtual float   getAdjustDirectionFromPoint(const Vector2f& pt) const  = 0;
};


/*!
\brief
	Base class for Slider widgets.

	The slider widget has a default range of 0.0f - 1.0f.  This enables use of the slider value to scale
	any value needed by way of a simple multiplication.
*/
class CEGUIEXPORT Slider : public Window
{
public:
	static const String EventNamespace;				//!< Namespace for global events
    static const String WidgetTypeName;             //!< Window factory name

	/*************************************************************************
		Event name constants
	*************************************************************************/
    /** Event fired when the slider value changes.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Slider whose value has changed.
     */
	static const String EventValueChanged;
    /** Event fired when the user begins dragging the thumb.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Slider whose thumb has started to
     * be dragged.
     */
	static const String EventThumbTrackStarted;
    /** Event fired when the user releases the thumb.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Slider whose thumb has been released.
     */
	static const String EventThumbTrackEnded;

    /*************************************************************************
        Child Widget name suffix constants
    *************************************************************************/
    static const String ThumbName;            //!< Widget name for the thumb component.

	/*************************************************************************
		Accessors
	*************************************************************************/
	/*!
	\brief
		return the current slider value.

	\return
		float value equal to the sliders current value.
	*/
	float	getCurrentValue(void) const			{return d_value;}


	/*!
	\brief
		return the maximum value set for this widget

	\return
		float value equal to the currently set maximum value for this slider.
	*/
	float	getMaxValue(void) const				{return d_maxValue;}


	/*!
	\brief
		return the current click step setting for the slider.

		The click step size is the amount the slider value will be adjusted when the widget
		is clicked wither side of the slider thumb.

	\return
		float value representing the current click step setting.
	*/
	float	getClickStep(void) const		{return d_step;}


    /*!
    \brief
        Return a pointer to the Thumb component widget for this Slider.

    \return
        Pointer to a Thumb object.

    \exception UnknownObjectException
        Thrown if the Thumb component does not exist.
    */
    Thumb* getThumb() const;


	/*************************************************************************
		Manipulators
	*************************************************************************/
	/*!
	\brief
		Initialises the Window based object ready for use.

	\note
		This must be called for every window created.  Normally this is handled automatically by the WindowFactory for each Window type.

	\return
		Nothing
	*/
	virtual	void	initialiseComponents(void);


	/*!
	\brief
		set the maximum value for the slider.  Note that the minimum value is fixed at 0.

	\param maxVal
		float value specifying the maximum value for this slider widget.

	\return
		Nothing.
	*/
	void	setMaxValue(float maxVal);


	/*!
	\brief
		set the current slider value.

	\param value
		float value specifying the new value for this slider widget.

	\return
		Nothing.
	*/
	void	setCurrentValue(float value);


	/*!
	\brief
		set the current click step setting for the slider.

		The click step size is the amount the slider value will be adjusted when the widget
		is clicked wither side of the slider thumb.

	\param step
		float value representing the click step setting to use.

	\return
		Nothing.
	*/
	void	setClickStep(float step)		{d_step = step;}


	/*************************************************************************
		Construction / Destruction
	*************************************************************************/
	/*!
	\brief
		Slider base class constructor
	*/
	Slider(const String& type, const String& name);


	/*!
	\brief
		Slider base class destructor
	*/
	virtual ~Slider(void);


protected:
	/*************************************************************************
		Implementation Functions
	*************************************************************************/
	/*!
	\brief
		update the size and location of the thumb to properly represent the current state of the slider
	*/
	virtual void	updateThumb(void);


	/*!
	\brief
		return value that best represents current slider value given the current location of the thumb.

	\return
		float value that, given the thumb widget position, best represents the current value for the slider.
	*/
	virtual float	getValueFromThumb(void) const;


	/*!
	\brief
		Given window location \a pt, return a value indicating what change should be 
		made to the slider.

	\param pt
		Point object describing a pixel position in window space.

	\return
		- -1 to indicate slider should be moved to a lower setting.
		-  0 to indicate slider should not be moved.
		- +1 to indicate slider should be moved to a higher setting.
	*/
	virtual float	getAdjustDirectionFromPoint(const Vector2f& pt) const;


    /*!
    \brief
        update the size and location of the thumb to properly represent the current state of the slider
    */
    //virtual void    updateThumb_impl(void)   = 0;


    /*!
    \brief
        return value that best represents current slider value given the current location of the thumb.

    \return
        float value that, given the thumb widget position, best represents the current value for the slider.
    */
    //virtual float   getValueFromThumb_impl(void) const   = 0;


    /*!
    \brief
        Given window location \a pt, return a value indicating what change should be 
        made to the slider.

    \param pt
        Point object describing a pixel position in window space.

    \return
        - -1 to indicate slider should be moved to a lower setting.
        -  0 to indicate slider should not be moved.
        - +1 to indicate slider should be moved to a higher setting.
    */
    //virtual float   getAdjustDirectionFromPoint_impl(const Point& pt) const  = 0;

	/*!
	\brief
		handler function for when thumb moves.
	*/
	bool	handleThumbMoved(const EventArgs& e);


	/*!
	\brief
		handler function for when thumb tracking begins
	*/
	bool	handleThumbTrackStarted(const EventArgs& e);


	/*!
	\brief
		handler function for when thumb tracking begins
	*/
	bool	handleThumbTrackEnded(const EventArgs& e);

    // validate window renderer
	virtual bool validateWindowRenderer(const WindowRenderer* renderer) const;


	/*************************************************************************
		New event handlers for slider widget
	*************************************************************************/
	/*!
	\brief
		Handler triggered when the slider value changes
	*/
	virtual void	onValueChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler triggered when the user begins to drag the slider thumb. 
	*/
	virtual void	onThumbTrackStarted(WindowEventArgs& e);


	/*!
	\brief
		Handler triggered when the slider thumb is released
	*/
	virtual void	onThumbTrackEnded(WindowEventArgs& e);


	/*************************************************************************
		Overridden event handlers
	*************************************************************************/
	virtual void	onMouseButtonDown(MouseEventArgs& e);
	virtual	void	onMouseWheel(MouseEventArgs& e);


	/*************************************************************************
		Implementation Data
	*************************************************************************/
	float	d_value;		//!< current slider value
	float	d_maxValue;		//!< slider maximum value (minimum is fixed at 0)
	float	d_step;			//!< amount to adjust slider by when clicked (and not dragged).

private:

	/*************************************************************************
		Private methods
	*************************************************************************/
	void	addSliderProperties(void);
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUISlider_h_
