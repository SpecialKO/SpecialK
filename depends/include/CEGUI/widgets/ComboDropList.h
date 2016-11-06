/***********************************************************************
	created:	13/6/2004
	author:		Paul D Turner

	purpose:	Interface for the Combobox Drop-List widget base class
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
#ifndef _CEGUIComboDropList_h_
#define _CEGUIComboDropList_h_

#include "./Listbox.h"


#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif


// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
	Base class for the combo box drop down list.  This is a specialisation of the Listbox class.
*/
class CEGUIEXPORT ComboDropList : public Listbox
{
public:
	static const String EventNamespace;				//!< Namespace for global events
    static const String WidgetTypeName;             //!< Window factory name


	/*************************************************************************
		Constants
	*************************************************************************/
	// Event names
    /** Event fired when the user confirms the selection by clicking the mouse.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ComboDropList whose selection has been
     * confirmed by the user.
     */
	static const String EventListSelectionAccepted;


	/*!
	\brief
		Initialise the Window based object ready for use.

	\note
		This must be called for every window created.  Normally this is handled automatically by the WindowFactory for each Window type.

	\return
		Nothing
	*/
	virtual void	initialiseComponents(void);


	/*!
	\brief
		Set whether the drop-list is 'armed' for selection.

	\note
		This setting is not exclusively under client control; the ComboDropList will auto-arm in
		response to certain left mouse button events.  This is also dependant upon the autoArm
		setting of the ComboDropList.

	\param setting
		- true to arm the box; items will be highlighted and the next left button up event
		will cause dismissal and possible item selection.

		- false to disarm the box; items will not be highlighted or selected until the box is armed.

	\return
		Nothing.
	*/
	void	setArmed(bool setting)		{ d_armed = setting; }


	/*!
	\brief
		Return the 'armed' state of the ComboDropList.

	\return
		- true if the box is armed; items will be highlighted and the next left button up event
		will cause dismissal and possible item selection.

		- false if the box is not armed; items will not be highlighted or selected until the box is armed.
	*/
	bool	isArmed(void) const		{ return d_armed; }


	/*!
	\brief
		Set the mode of operation for the ComboDropList.

	\param setting
		- true if the ComboDropList auto-arms when the mouse enters the box.
		- false if the user must click to arm the box.

	\return
		Nothing.
	*/
	void	setAutoArmEnabled(bool setting)		{ d_autoArm = setting; }


	/*!
	\brief
		returns the mode of operation for the drop-list

	\return
		- true if the ComboDropList auto-arms when the mouse enters the box.
		- false if the user must click to arm the box.
	*/
	bool	isAutoArmEnabled(void) const		{ return d_autoArm; }

    //! resize the widget such that the content is shown without scrollbars.
    void resizeToContent(bool fit_width, bool fit_height);

	/*************************************************************************
		Constructor & Destructor
	*************************************************************************/
	/*!
	\brief
		Constructor for ComboDropList base class
	*/
	ComboDropList(const String& type, const String& name);


	/*!
	\brief
		Destructor for ComboDropList base class
	*/
	virtual ~ComboDropList(void);


protected:
	/*************************************************************************
		New event handlers
	*************************************************************************/
	/*!
	\brief
		Handler for when list selection is confirmed.
	*/
	void	onListSelectionAccepted(WindowEventArgs& e);


	/*************************************************************************
		Overridden Event handling
	*************************************************************************/
	virtual void	onMouseMove(MouseEventArgs& e);
	virtual void	onMouseButtonDown(MouseEventArgs& e);
	virtual void	onMouseButtonUp(MouseEventArgs& e);
	virtual void	onCaptureLost(WindowEventArgs& e);
	virtual void	onActivated(ActivationEventArgs& e);
    virtual void    onListContentsChanged(WindowEventArgs& e);
    virtual void    onSelectionChanged(WindowEventArgs& e);

	/*************************************************************************
		Implementation Data
	*************************************************************************/
	bool	d_autoArm;		//!< true if the box auto-arms when the mouse enters it.
	bool	d_armed;		//!< true when item selection has been armed.
    ListboxItem* d_lastClickSelected; //!< Item last accepted by user.
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUIComboDropList_h_
