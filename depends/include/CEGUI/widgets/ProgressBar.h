/***********************************************************************
	created:	13/4/2004
	author:		Paul D Turner
	
	purpose:	Interface to base class for ProgressBar widget
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
#ifndef _CEGUIProgressBar_h_
#define _CEGUIProgressBar_h_

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
	Base class for progress bars.
*/
class CEGUIEXPORT ProgressBar : public Window
{
public:
	static const String EventNamespace;				//!< Namespace for global events
    static const String WidgetTypeName;             //!< Window factory name

	/*************************************************************************
		Event name constants
	*************************************************************************/
    /** Event fired whenever the progress value is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ProgressBar whose value has been
     * changed.
     */
	static const String EventProgressChanged;
    /** Event fired when the progress bar's value reaches 100%.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ProgressBar whose progress value
     * has reached 100%.
     */
	static const String EventProgressDone;


	/************************************************************************
		Accessor Functions
	************************************************************************/
	/*!
	\brief
		return the current progress value
	*/
	float	getProgress(void) const		{return d_progress;}

	/*!
	\brief
		return the current step size
	*/
	float	getStepSize(void) const			{return d_step;}


	/*************************************************************************
		Manipulator Functions
	*************************************************************************/
	/*!
	\brief
		set the current progress.

	\param progress
		The level of progress to set.  If this value is >1.0f (100%) progress will be limited to 1.0f.

	\return
		Nothing.
	*/
	void	setProgress(float progress);


	/*!
	\brief
		set the size of the 'step' in percentage points (default is 0.01f or 1%).

	\param step
		Amount to increase the progress by each time the step method is called.

	\return
		Nothing.
	*/
	void	setStepSize(float step_val)		{d_step = step_val;}


	/*!
	\brief
		cause the progress to step

		The amount the progress bar will step can be changed by calling the setStepSize method.  The
		default step size is 0.01f which is equal to 1%.
	
	\return
		Nothing.
	*/
	void	step(void)			{setProgress(d_progress + d_step);}


	/*!
	\brief
		Modify the progress level by a specified delta.

	\param delta
		amount to adjust the progress by.  Whatever this value is, the progress of the bar will be kept
		within the range: 0.0f <= progress <= 1.0f.

	\return
		Nothing.
	*/
	void	adjustProgress(float delta)		{setProgress(d_progress + delta);}


	/*************************************************************************
		Construction / Destruction
	*************************************************************************/
	/*!
	\brief
		Constructor for ProgressBar class
	*/
	ProgressBar(const String& type, const String& name);


	/*!
	\brief
		Destructor for ProgressBar
	*/
	virtual ~ProgressBar(void);


protected:
	/*************************************************************************
		Event handlers for progress bar specific events
	*************************************************************************/
	/*!
	\brief
		event triggered when progress changes
	*/
	virtual	void	onProgressChanged(WindowEventArgs& e);


	/*!
	\brief
		event triggered when progress reaches 100%
	*/
	virtual void	onProgressDone(WindowEventArgs& e);


	/*************************************************************************
		Implementation Data
	*************************************************************************/
	float	d_progress;		//!< current progress (from 0.0f to 1.0f)
	float	d_step;			//!< amount to 'step' progress by on a call to step()


private:
	/*************************************************************************
		Private methods
	*************************************************************************/
	void	addProgressBarProperties(void);
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUIProgressBar_h_
