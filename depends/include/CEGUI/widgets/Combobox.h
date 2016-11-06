/***********************************************************************
	created:	13/4/2004
	author:		Paul D Turner
	
	purpose:	Interface to base class for Combobox widget
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
#ifndef _CEGUICombobox_h_
#define _CEGUICombobox_h_

#include "CEGUI/Base.h"
#include "CEGUI/Window.h"
#include "CEGUI/RegexMatcher.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif


// Start of CEGUI namespace section
namespace CEGUI
{

/*!
\brief
	Base class for the Combobox widget
*/
class CEGUIEXPORT Combobox : public Window
{
public:
    typedef RegexMatcher::MatchState MatchState;

	static const String EventNamespace;				//!< Namespace for global events
    static const String WidgetTypeName;             //!< Window factory name

	/*************************************************************************
		Constants
	*************************************************************************/
	// event names from edit box
    /** Event fired when the read-only mode for the edit box is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Combobox whose read only mode
     * has been changed.
     */
	static const String EventReadOnlyModeChanged;
    /** Event fired when the edix box validation string is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Combobox whose validation
     * string was changed.
     */
	static const String EventValidationStringChanged;
    /** Event fired when the maximum string length is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Combobox whose maximum edit box
     * string length has been changed.
     */
	static const String EventMaximumTextLengthChanged;
    /** Event fired when the validity of the Combobox text (as determined by a
     * RegexMatcher object) has changed.
     * Handlers are passed a const RegexMatchStateEventArgs reference with
     * WindowEventArgs::window set to the Combobox whose text validity has
     * changed and RegexMatchStateEventArgs::matchState set to the new match
     * validity. Handler return is significant, as follows:
     * - true indicates the new state - and therfore text - is to be accepted.
     * - false indicates the new state is not acceptable, and the previous text
     *   should remain in place. NB: This is only possible when the validity
     *   change is due to a change in the text, if the validity change is due to
     *   a change in the validation regular expression string, then returning
     *   false will have no effect.
     */
    static const String EventTextValidityChanged;
    /** Event fired when the edit box text insertion position is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Combobox whose caret position has
     * been changed.
     */
	static const String EventCaretMoved;
    /** Event fired when the current edit box text selection is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Combobox whose edit box text selection
     * has been changed.
     */
	static const String EventTextSelectionChanged;
    /** Event fired when the number of characters in the edit box has reached
     * the currently set maximum.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Combobox whose edit box has become
     * full.
     */
	static const String EventEditboxFull;
    /** Event fired when the user accepts the current edit box text by pressing
     * Return, Enter, or Tab.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Combobox whose edit box text has been
     * accepted / confirmed by the user.
     */
	static const String EventTextAccepted;

	// event names from list box
    /** Event fired when the contents of the list is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Combobox whose list content has
     * changed.
     */
	static const String EventListContentsChanged;
    /** Event fired when there is a change to the currently selected item in the
     * list.
     * @note This change in selection may be temporary (for example, when
     * hovering over an item in the combobox). See also the event
     * CEGUI::Combobox::EventListSelectionAccepted that is fired for a selection
     * that the user has 'confirmed' in some way.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Combobox whose currently selected list
     * item has changed.
     */
	static const String EventListSelectionChanged;
    /** Event fired when the sort mode setting of the list is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Combobox whose list sorting mode has
     * been changed.
     */
	static const String EventSortModeChanged;
    /** Event fired when the vertical scroll bar 'force' setting for the list is
     * changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Combobox whose vertical scroll bar
     * setting is changed.
     */
	static const String EventVertScrollbarModeChanged;
    /** Event fired when the horizontal scroll bar 'force' setting for the list
     * is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Combobox whose horizontal scroll bar
     * setting has been changed.
     */
	static const String EventHorzScrollbarModeChanged;

	// events we produce / generate ourselves
    /** Event fired when the drop-down list is displayed
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Combobox whose drop down list has
     * been displayed.
     */
	static const String EventDropListDisplayed;
    /** Event fired when the drop-down list is removed / hidden.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Combobox whose drop down list has
     * been hidden.
     */
	static const String EventDropListRemoved;
    /** Event fired when the user accepts a selection from the drop-down list
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Combobox in which the user has
     * confirmed a selection from the drop down list.
     */
	static const String EventListSelectionAccepted;

    /*************************************************************************
        Child Widget name constants
    *************************************************************************/
    static const String EditboxName;    //!< Widget name for the editbox component.
    static const String DropListName;   //!< Widget name for the drop list component.
    static const String ButtonName;     //!< Widget suffix for the button component.

    // override from Window class
    bool isHit(const Vector2f& position,
               const bool allow_disabled = false) const;

	/*!
	\brief
		returns the mode of operation for the combo box.

	\return
		- true if the user can show the list and select an item with a single mouse click.
		- false if the user must click to show the list and then click again to select an item.
	*/
	bool	getSingleClickEnabled(void) const;


	/*!
	\brief
		returns true if the drop down list is visible.

	\return
		true if the drop down list is visible, false otherwise.
	*/
	bool	isDropDownListVisible(void) const;


    /*!
    \brief
        Return a pointer to the Editbox component widget for this Combobox.

    \return
        Pointer to an Editbox object.

    \exception UnknownObjectException
        Thrown if the Editbox component does not exist.
    */
    Editbox* getEditbox() const;

    /*!
    \brief
        Return a pointer to the PushButton component widget for this Combobox.

    \return
        Pointer to a PushButton object.

    \exception UnknownObjectException
        Thrown if the PushButton component does not exist.
    */
    PushButton* getPushButton() const;

    /*!
    \brief
        Return a pointer to the ComboDropList component widget for this
        Combobox.

    \return
        Pointer to an ComboDropList object.

    \exception UnknownObjectException
        Thrown if the ComboDropList component does not exist.
    */
    ComboDropList* getDropList() const;

    //! return whether the drop-list will vertically auto size to content.
    bool getAutoSizeListHeightToContent() const;

    //! return whether the drop-list will horizontally auto size to content.
    bool getAutoSizeListWidthToContent() const;

	/*************************************************************************
		Editbox Accessors
	*************************************************************************/
	/*!
	\brief
		return true if the Editbox has input focus.

	\return
		true if the Editbox has keyboard input focus, false if the Editbox does not have keyboard input focus.
	*/
	bool	hasInputFocus(void) const;


	/*!
	\brief
		return true if the Editbox is read-only.

	\return
		true if the Editbox is read only and can't be edited by the user, false if the Editbox is not
		read only and may be edited by the user.
	*/
	bool	isReadOnly(void) const;


    /*!
    \brief
        return the validation MatchState for the current Combobox text, given
        the currently set validation string.

    \note
        Validation is performed by means of a regular expression.  If the text
        matches the regex, the text is said to have passed validation.  If the
        text does not match with the regex then the text fails validation.

    \return
        One of the MatchState enumerated values indicating the current match
        state.
    */
    MatchState getTextMatchState() const;


	/*!
	\brief
		return the currently set validation string

	\note
		Validation is performed by means of a regular expression.  If the text matches the regex, the text is said to have passed
		validation.  If the text does not match with the regex then the text fails validation.

	\return
		String object containing the current validation regex data
	*/
	const String&	getValidationString(void) const;


	/*!
	\brief
		return the current position of the caret.

	\return
		Index of the insert caret relative to the start of the text.
	*/
	size_t	getCaretIndex(void) const;


	/*!
	\brief
		return the current selection start point.

	\return
		Index of the selection start point relative to the start of the text.  If no selection is defined this function returns
		the position of the caret.
	*/
	size_t	getSelectionStartIndex(void) const;


	/*!
	\brief
		return the current selection end point.

	\return
		Index of the selection end point relative to the start of the text.  If no selection is defined this function returns
		the position of the caret.
	*/
	size_t	getSelectionEndIndex(void) const;

	
	/*!
	\brief
		return the length of the current selection (in code points / characters).

	\return
		Number of code points (or characters) contained within the currently defined selection.
	*/
	size_t	getSelectionLength(void) const;


	/*!
	\brief
		return the maximum text length set for this Editbox.

	\return
		The maximum number of code points (characters) that can be entered into this Editbox.

	\note
		Depending on the validation string set, the actual length of text that can be entered may be less than the value
		returned here (it will never be more).
	*/
	size_t	getMaxTextLength(void) const;


	/*************************************************************************
		List Accessors
	*************************************************************************/
	/*!
	\brief
		Return number of items attached to the list box

	\return
		the number of items currently attached to this list box.
	*/
	size_t	getItemCount(void) const;

	
	/*!
	\brief
		Return a pointer to the currently selected item.

	\return
		Pointer to a ListboxItem based object that is the selected item in the list.  will return NULL if
		no item is selected.
	*/
	ListboxItem*	getSelectedItem(void) const;


	/*!
	\brief
		Return the item at index position \a index.

	\param index
		Zero based index of the item to be returned.

	\return
		Pointer to the ListboxItem at index position \a index in the list box.

	\exception	InvalidRequestException	thrown if \a index is out of range.
	*/
	ListboxItem*	getListboxItemFromIndex(size_t index) const;


	/*!
	\brief
		Return the index of ListboxItem \a item

	\param item
		Pointer to a ListboxItem whos zero based index is to be returned.

	\return
		Zero based index indicating the position of ListboxItem \a item in the list box.

	\exception	InvalidRequestException	thrown if \a item is not attached to this list box.
	*/
	size_t	getItemIndex(const ListboxItem* item) const;


	/*!
	\brief
		return whether list sorting is enabled

	\return
		true if the list is sorted, false if the list is not sorted
	*/
	bool	isSortEnabled(void) const;


	/*!
	\brief
		return whether the string at index position \a index is selected

	\param index
		Zero based index of the item to be examined.

	\return
		true if the item at \a index is selected, false if the item at \a index is not selected.

	\exception	InvalidRequestException	thrown if \a index is out of range.
	*/
	bool	isItemSelected(size_t index) const;


	/*!
	\brief
		Search the list for an item with the specified text

	\param text
		String object containing the text to be searched for.

	\param start_item
		ListboxItem where the search is to begin, the search will not include \a item.  If \a item is
		NULL, the search will begin from the first item in the list.

	\return
		Pointer to the first ListboxItem in the list after \a item that has text matching \a text.  If
		no item matches the criteria NULL is returned.

	\exception	InvalidRequestException	thrown if \a item is not attached to this list box.
	*/
	ListboxItem*	findItemWithText(const String& text, const ListboxItem* start_item);


	/*!
	\brief
		Return whether the specified ListboxItem is in the List

	\return
		true if ListboxItem \a item is in the list, false if ListboxItem \a item is not in the list.
	*/
	bool	isListboxItemInList(const ListboxItem* item) const;


	/*!
	\brief
		Return whether the vertical scroll bar is always shown.

	\return
		- true if the scroll bar will always be shown even if it is not required.
		- false if the scroll bar will only be shown when it is required.
	*/
	bool	isVertScrollbarAlwaysShown(void) const;


	/*!
	\brief
		Return whether the horizontal scroll bar is always shown.

	\return
		- true if the scroll bar will always be shown even if it is not required.
		- false if the scroll bar will only be shown when it is required.
	*/
	bool	isHorzScrollbarAlwaysShown(void) const;


	/*************************************************************************
		Combobox Manipulators
	*************************************************************************/
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
		Show the drop-down list

	\return
		Nothing
	*/
	void	showDropList(void);


	/*!
	\brief
		Hide the drop-down list

	\return
		Nothing.
	*/
	void	hideDropList(void);


	/*!
	\brief
		Set the mode of operation for the combo box.

	\param setting
		- true if the user should be able to show the list and select an item with a single mouse click.
		- false if the user must click to show the list and then click again to select an item.

	\return
		Nothing.
	*/
	void	setSingleClickEnabled(bool setting);

    //! Select item in list matching editbox text, clear selection if none match
    void selectListItemWithEditboxText();

    /*!
    \brief
        Sets whether the Combobox drop-down list will automatically resize
        it's height according to the total height of items added to the list.
    */
    void setAutoSizeListHeightToContent(bool auto_size);

    /*!
    \brief
        Sets whether the Combobox drop-down list will automatically resize
        it's width according to the width of the items added to the list.
    */
    void setAutoSizeListWidthToContent(bool auto_size);

    //! update drop list size according to auto-size options.
    void updateAutoSizedDropList();

	/*************************************************************************
		Editbox Manipulators
	*************************************************************************/
	/*!
	\brief
		Specify whether the Editbox is read-only.

	\param setting
		true if the Editbox is read only and can't be edited by the user, false if the Editbox is not
		read only and may be edited by the user.

	\return
		Nothing.
	*/
	void	setReadOnly(bool setting);


	/*!
	\brief
		Set the text validation string.

	\note
		Validation is performed by means of a regular expression.  If the text matches the regex, the text is said to have passed
		validation.  If the text does not match with the regex then the text fails validation.

	\param validation_string
		String object containing the validation regex data to be used.

	\return
		Nothing.
	*/
	void	setValidationString(const String& validation_string);


	/*!
	\brief
		Set the current position of the caret.

	\param caret_pos
		New index for the insert caret relative to the start of the text.  If the value specified is greater than the
		number of characters in the Editbox, the caret is positioned at the end of the text.

	\return
		Nothing.
	*/
	void	setCaretIndex(size_t caret_pos);


	/*!
	\brief
		Define the current selection for the Editbox

	\param start_pos
		Index of the starting point for the selection.  If this value is greater than the number of characters in the Editbox, the
		selection start will be set to the end of the text.

	\param end_pos
		Index of the ending point for the selection.  If this value is greater than the number of characters in the Editbox, the
		selection start will be set to the end of the text.

	\return
		Nothing.
	*/
	void	setSelection(size_t start_pos, size_t end_pos);
	/*!
	\brief
		Define the current selection start for the Editbox

	\param start_pos
		Index of the starting point for the selection.  If this value is greater than the number of characters in the Editbox, the
		selection start will be set to the end of the text.

	\return
		Nothing.
	*/
	void	setSelectionStart(size_t start_pos);

	/*!
	\brief
		Define the current selection for the Editbox

	\param start_pos
		Length of the selection.

	\return
		Nothing.
	*/
	void	setSelectionLength(size_t length);

	/*!
	\brief
		set the maximum text length for this Editbox.

	\param max_len
		The maximum number of code points (characters) that can be entered into this Editbox.

	\note
		Depending on the validation string set, the actual length of text that can be entered may be less than the value
		set here (it will never be more).

	\return
		Nothing.
	*/
	void	setMaxTextLength(size_t max_len);


	/*!
	\brief
		Activate the edit box component of the Combobox.

	\return
		Nothing.
	*/
	void	activateEditbox(void);


	/*************************************************************************
		List Manipulators
	*************************************************************************/
	/*!
	\brief
		Remove all items from the list.

		Note that this will cause 'AutoDelete' items to be deleted.
	*/
	void	resetList(void);


	/*!
	\brief
		Add the given ListboxItem to the list.

	\param item
		Pointer to the ListboxItem to be added to the list.  Note that it is the passed object that is added to the
		list, a copy is not made.  If this parameter is NULL, nothing happens.

	\return
		Nothing.
	*/
	void	addItem(ListboxItem* item);


	/*!
	\brief
		Insert an item into the list box after a specified item already in the list.

		Note that if the list is sorted, the item may not end up in the requested position.

	\param item
		Pointer to the ListboxItem to be inserted.  Note that it is the passed object that is added to the
		list, a copy is not made.  If this parameter is NULL, nothing happens.

	\param position
		Pointer to a ListboxItem that \a item is to be inserted after.  If this parameter is NULL, the item is
		inserted at the start of the list.

	\return
		Nothing.
	*/
	void	insertItem(ListboxItem* item, const ListboxItem* position);


	/*!
	\brief
		Removes the given item from the list box.

	\param item
		Pointer to the ListboxItem that is to be removed.  If \a item is not attached to this list box then nothing
		will happen.

	\return
		Nothing.
	*/
	void	removeItem(const ListboxItem* item);


	/*!
	\brief
		Clear the selected state for all items.

	\return
		Nothing.
	*/
	void	clearAllSelections(void);


	/*!
	\brief
		Set whether the list should be sorted.

	\param setting
		true if the list should be sorted, false if the list should not be sorted.

	\return
		Nothing.
	*/
	void	setSortingEnabled(bool setting);

	
	/*!
	\brief
		Set whether the vertical scroll bar should always be shown.

	\param setting
		true if the vertical scroll bar should be shown even when it is not required.  false if the vertical
		scroll bar should only be shown when it is required.

	\return
		Nothing.
	*/
	void	setShowVertScrollbar(bool setting);


	/*!
	\brief
		Set whether the horizontal scroll bar should always be shown.

	\param setting
		true if the horizontal scroll bar should be shown even when it is not required.  false if the horizontal
		scroll bar should only be shown when it is required.

	\return
		Nothing.
	*/
	void	setShowHorzScrollbar(bool setting);


	/*!
	\brief
		Set the select state of an attached ListboxItem.

		This is the recommended way of selecting and deselecting items attached to a list box as it respects the
		multi-select mode setting.  It is possible to modify the setting on ListboxItems directly, but that approach
		does not respect the settings of the list box.

	\param item
		The ListboxItem to be affected.  This item must be attached to the list box.

	\param state
		true to select the item, false to de-select the item.

	\return
		Nothing.
	
	\exception	InvalidRequestException	thrown if \a item is not attached to this list box.
	*/
	void	setItemSelectState(ListboxItem* item, bool state);


	/*!
	\brief
		Set the select state of an attached ListboxItem.

		This is the recommended way of selecting and deselecting items attached to a list box as it respects the
		multi-select mode setting.  It is possible to modify the setting on ListboxItems directly, but that approach
		does not respect the settings of the list box.

	\param item_index
		The zero based index of the ListboxItem to be affected.  This must be a valid index (0 <= index < getItemCount())

	\param state
		true to select the item, false to de-select the item.

	\return
		Nothing.
	
	\exception	InvalidRequestException	thrown if \a item_index is out of range for the list box
	*/
	void	setItemSelectState(size_t item_index, bool state);


	/*!
	\brief
		Causes the list box to update it's internal state after changes have been made to one or more
		attached ListboxItem objects.

		Client code must call this whenever it has made any changes to ListboxItem objects already attached to the
		list box.  If you are just adding items, or removed items to update them prior to re-adding them, there is
		no need to call this method.

	\return
		Nothing.
	*/
	void	handleUpdatedListItemData(void);


	/*************************************************************************
		Construction and Destruction
	*************************************************************************/
	/*!
	\brief
		Constructor for Combobox base class
	*/
	Combobox(const String& type, const String& name);


	/*!
	\brief
		Destructor for Combobox base class
	*/
	virtual ~Combobox(void);


protected:
	/*************************************************************************
		Implementation Methods
	*************************************************************************/
	/*!
	\brief
		Handler function for button clicks.
	*/
	bool	button_PressHandler(const EventArgs& e);


	/*!
	\brief
		Handler for selections made in the drop-list
	*/
	bool	droplist_SelectionAcceptedHandler(const EventArgs& e);


	/*!
	\brief
		Handler for when drop-list hides itself
	*/
	bool	droplist_HiddenHandler(const EventArgs& e);


	/*!
	\brief
		Mouse button down handler attached to edit box
	*/
	bool	editbox_MouseDownHandler(const EventArgs& e);

    /*!
    \brief
        Update the Combobox text to reflect programmatically made changes to
        selected list item.
    */
    void itemSelectChangeTextUpdate(const ListboxItem* const item,
                                    bool new_state, bool old_state);

	/*************************************************************************
		Handlers to relay child widget events so they appear to come from us
	*************************************************************************/
	bool editbox_ReadOnlyChangedHandler(const EventArgs& e);
	bool editbox_ValidationStringChangedHandler(const EventArgs& e);
	bool editbox_MaximumTextLengthChangedHandler(const EventArgs& e);
	bool editbox_TextValidityChangedHandler(const EventArgs& e);
	bool editbox_CaretMovedHandler(const EventArgs& e);
	bool editbox_TextSelectionChangedHandler(const EventArgs& e);
	bool editbox_EditboxFullEventHandler(const EventArgs& e);
	bool editbox_TextAcceptedEventHandler(const EventArgs& e);
	bool editbox_TextChangedEventHandler(const EventArgs& e);
	bool listbox_ListContentsChangedHandler(const EventArgs& e);
	bool listbox_ListSelectionChangedHandler(const EventArgs& e);
	bool listbox_SortModeChangedHandler(const EventArgs& e);
	bool listbox_VertScrollModeChangedHandler(const EventArgs& e);
	bool listbox_HorzScrollModeChangedHandler(const EventArgs& e);

	
	/*************************************************************************
		New Events for Combobox
	*************************************************************************/
	/*!
	\brief
		Handler called internally when the read only state of the Combobox's Editbox has been changed.
	*/
	virtual	void	onReadOnlyChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called internally when the Combobox's Editbox validation string has been changed.
	*/
	virtual	void	onValidationStringChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called internally when the Combobox's Editbox maximum text length is changed.
	*/
	virtual	void	onMaximumTextLengthChanged(WindowEventArgs& e);


    /*!
    \brief
        Handler called when something has caused the validity state of the
        current text to change.
    */
    virtual void onTextValidityChanged(RegexMatchStateEventArgs& e);


	/*!
	\brief
		Handler called internally when the caret in the Comboxbox's Editbox moves.
	*/
	virtual	void	onCaretMoved(WindowEventArgs& e);


	/*!
	\brief
		Handler called internally when the selection within the Combobox's Editbox changes.
	*/
	virtual	void	onTextSelectionChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called internally when the maximum length is reached for text in the Combobox's Editbox.
	*/
	virtual	void	onEditboxFullEvent(WindowEventArgs& e);


	/*!
	\brief
		Handler called internally when the text in the Combobox's Editbox is accepted (by various means).
	*/
	virtual	void	onTextAcceptedEvent(WindowEventArgs& e);


	/*!
	\brief
		Handler called internally when the Combobox's Drop-down list contents are changed.
	*/
	virtual	void	onListContentsChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called internally when the selection within the Combobox's drop-down list changes
		(this is not the 'final' accepted selection, just the currently highlighted item).
	*/
	virtual	void	onListSelectionChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called  fired internally when the sort mode for the Combobox's drop-down list is changed.
	*/
	virtual	void	onSortModeChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called internally when the 'force' setting for the vertical scrollbar within the Combobox's
		drop-down list is changed.
	*/
	virtual	void	onVertScrollbarModeChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called internally when the 'force' setting for the horizontal scrollbar within the Combobox's
		drop-down list is changed.
	*/
	virtual	void	onHorzScrollbarModeChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called internally when the Combobox's drop-down list has been displayed.
	*/
	virtual	void	onDropListDisplayed(WindowEventArgs& e);


	/*!
	\brief
		Handler called internally when the Combobox's drop-down list has been hidden.
	*/
	virtual	void	onDroplistRemoved(WindowEventArgs& e);


	/*!
	\brief
		Handler called internally when the user has confirmed a selection within the Combobox's drop-down list.
	*/
	virtual	void	onListSelectionAccepted(WindowEventArgs& e);


	/*************************************************************************
		Overridden Event handlers
	*************************************************************************/
	virtual	void	onFontChanged(WindowEventArgs& e);
	virtual void	onTextChanged(WindowEventArgs& e);
	virtual void	onActivated(ActivationEventArgs& e);
    void onSized(ElementEventArgs& e);


	/*************************************************************************
		Implementation Data
	*************************************************************************/
	bool			d_singleClickOperation;		//!< true if user can show and select from list in a single click.
    bool d_autoSizeHeight;
    bool d_autoSizeWidth;

private:
	/*************************************************************************
		Private methods
	*************************************************************************/
	void	addComboboxProperties(void);
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUICombobox_h_
