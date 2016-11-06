/***********************************************************************
	created:	13/4/2004
	author:		Paul D Turner

	purpose:	Interface to base class for Listbox widget
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
#ifndef _CEGUIListbox_h_
#define _CEGUIListbox_h_

#include "../Base.h"
#include "../Window.h"
#include <vector>


#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif


// Start of CEGUI namespace section
namespace CEGUI
{

/*!
\brief
    Base class for Listbox window renderer.
*/
class CEGUIEXPORT ListboxWindowRenderer : public WindowRenderer
{
public:
    /*!
    \brief
        Constructor
    */
    ListboxWindowRenderer(const String& name);

    /*!
    \brief
        Return a Rect object describing, in un-clipped pixels, the window
        relative area that is to be used for rendering list items.
    */
    virtual Rectf getListRenderArea(void) const = 0;

    /*!
     * Resize the Listbox the renderer is attached to such that it's
     * content can be displayed without needing scrollbars if there is
     * enough space, otherwise make the Listbox as large as possible
     * (without moving it).
     */
    virtual void resizeListToContent(bool fit_width,
                                     bool fit_height) const = 0;
};

/*!
\brief
	Base class for standard Listbox widget.
*/
class CEGUIEXPORT Listbox : public Window
{
public:
	static const String EventNamespace;				//!< Namespace for global events
    static const String WidgetTypeName;             //!< Window factory name

	/*************************************************************************
		Constants
	*************************************************************************/
	// event names
    /** Event fired when the contents of the list is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Listbox whose content is changed.
     */
	static const String EventListContentsChanged;
    /** Event fired when there is a change to the currently selected item(s)
     * within the list.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Listbox that has had a change in the
     * selected items.
     */
	static const String EventSelectionChanged;
    /** Event fired when the sort mode setting changes for the Listbox.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Listbox whose sort mode has been
     * changed.
     */
	static const String EventSortModeChanged;
    /** Event fired when the multi-select mode setting changes for the Listbox.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Listbox whose multi-select mode has
     * been changed.
     */
	static const String EventMultiselectModeChanged;
    /** Event fired when the mode setting that forces the display of the
     * vertical scroll bar for the Listbox is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Listbox whose vertical
     * scrollbar mode has been changed.
     */
	static const String EventVertScrollbarModeChanged;
    /** Event fired when the mode setting that forces the display of the
     * horizontal scroll bar for the Listbox is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Listbox whose horizontal
     * scrollbar mode has been changed.
     */
	static const String EventHorzScrollbarModeChanged;

    /*************************************************************************
        Child Widget name constants
    *************************************************************************/
    static const String VertScrollbarName;   //!< Widget name for the vertical scrollbar component.
    static const String HorzScrollbarName;   //!< Widget name for the horizontal scrollbar component.

	/*************************************************************************
		Accessor Methods
	*************************************************************************/
	/*!
	\brief
		Return number of items attached to the list box

	\return
		the number of items currently attached to this list box.
	*/
	size_t	getItemCount(void) const		{return d_listItems.size();}


	/*!
	\brief
		Return the number of selected items in the list box.

	\return
		Total number of attached items that are in the selected state.
	*/
	size_t	getSelectedCount(void) const;


	/*!
	\brief
		Return a pointer to the first selected item.

	\return
		Pointer to a ListboxItem based object that is the first selected item in the list.  will return NULL if
		no item is selected.
	*/
	ListboxItem*	getFirstSelectedItem(void) const;


	/*!
	\brief
		Return a pointer to the next selected item after item \a start_item

	\param start_item
		Pointer to the ListboxItem where the search for the next selected item is to begin.  If this
		parameter is NULL, the search will begin with the first item in the list box.

	\return
		Pointer to a ListboxItem based object that is the next selected item in the list after
		the item specified by \a start_item.  Will return NULL if no further items were selected.

	\exception	InvalidRequestException	thrown if \a start_item is not attached to this list box.
	*/
	ListboxItem*	getNextSelected(const ListboxItem* start_item) const;


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
	bool	isSortEnabled(void) const		{return d_sorted;}

	/*!
	\brief
		return whether multi-select is enabled

	\return
		true if multi-select is enabled, false if multi-select is not enabled.
	*/
	bool	isMultiselectEnabled(void) const	{return d_multiselect;}

	bool	isItemTooltipsEnabled(void) const	{return d_itemTooltips;}

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
		Manipulator Methods
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
		Insert an item into the list box before a specified item already in the
        list.

		Note that if the list is sorted, the item may not end up in the
        requested position.

	\param item
		Pointer to the ListboxItem to be inserted.  Note that it is the passed
        object that is added to the list, a copy is not made.  If this parameter
        is NULL, nothing happens.

	\param position
		Pointer to a ListboxItem that \a item is to be inserted before.  If this
        parameter is NULL, the item is inserted at the start of the list.

	\return
		Nothing.

	\exception InvalidRequestException	thrown if no ListboxItem \a position is
        attached to this list box.
	*/
	void	insertItem(ListboxItem* item, const ListboxItem* position);


	/*!
	\brief
		Removes the given item from the list box.  If the item is has the auto delete state set, the item will be deleted.

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
		Set whether the list should allow multiple selections or just a single selection

	\param  setting
		true if the widget should allow multiple items to be selected, false if the widget should only allow
		a single selection.

	\return
		Nothing.
	*/
	void	setMultiselectEnabled(bool setting);


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

	void	setItemTooltipsEnabled(bool setting);
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
	void	handleUpdatedItemData(void);


	/*!
	\brief
		Ensure the item at the specified index is visible within the list box.

	\param item_index
		Zero based index of the item to be made visible in the list box.  If this value is out of range, the
		list is always scrolled to the bottom.

	\return
		Nothing.
	*/
	void	ensureItemIsVisible(size_t item_index);


	/*!
	\brief
		Ensure the item at the specified index is visible within the list box.

	\param item
		Pointer to the ListboxItem to be made visible in the list box.

	\return
		Nothing.

	\exception	InvalidRequestException	thrown if \a item is not attached to this list box.
	*/
	void	ensureItemIsVisible(const ListboxItem* item);


    /*!
    \brief
        Return a Rect object describing, in un-clipped pixels, the window relative area
        that is to be used for rendering list items.

    \return
        Rect object describing the area of the Window to be used for rendering
        list box items.
    */
    virtual Rectf getListRenderArea(void) const;


    /*!
    \brief
        Return a pointer to the vertical scrollbar component widget for this
        Listbox.

    \return
        Pointer to a Scrollbar object.

    \exception UnknownObjectException
        Thrown if the vertical Scrollbar component does not exist.
    */
    Scrollbar* getVertScrollbar() const;

    /*!
    \brief
        Return a pointer to the horizontal scrollbar component widget for this
        Listbox.

    \return
        Pointer to a Scrollbar object.

    \exception UnknownObjectException
        Thrown if the horizontal Scrollbar component does not exist.
    */
    Scrollbar* getHorzScrollbar() const;


    /*!
    \brief
        Return the sum of all item heights
    */
    float   getTotalItemsHeight(void) const;


    /*!
    \brief
        Return the width of the widest item
    */
    float   getWidestItemWidth(void) const;


	/*!
	\brief
		Return a pointer to the ListboxItem attached to this Listbox at the
        given screen pixel co-ordinate.

	\return
		Pointer to the ListboxItem attached to this Listbox that is at screen
        position \a pt, or 0 if no ListboxItem attached to this Listbox is at
        that position.
	*/
    ListboxItem* getItemAtPoint(const Vector2f& pt) const;


	/*************************************************************************
		Construction and Destruction
	*************************************************************************/
	/*!
	\brief
		Constructor for Listbox base class.
	*/
	Listbox(const String& type, const String& name);


	/*!
	\brief
		Destructor for Listbox base class.
	*/
	virtual ~Listbox(void);


protected:
	/*************************************************************************
		Abstract Implementation Functions (must be provided by derived class)
	*************************************************************************/
	/*!
	\brief
		Return a Rect object describing, in un-clipped pixels, the window relative area
		that is to be used for rendering list items.

	\return
		Rect object describing the area of the Window to be used for rendering
		list box items.
	*/
	//virtual	Rect	getListRenderArea_impl(void) const = 0;


	/*************************************************************************
		Implementation Functions
	*************************************************************************/
	/*!
	\brief
		display required integrated scroll bars according to current state of the list box and update their values.
	*/
	void	configureScrollbars(void);

	/*!
	\brief
		select all strings between positions \a start and \a end.  (inclusive)
		including \a end.
	*/
	void	selectRange(size_t start, size_t end);


	/*!
	\brief
		Clear the selected state for all items (implementation)

	\return
		true if some selections were cleared, false nothing was changed.
	*/
	bool	clearAllSelections_impl(void);


	/*!
	\brief
		Remove all items from the list.

	\note
		Note that this will cause 'AutoDelete' items to be deleted.

	\return
		- true if the list contents were changed.
		- false if the list contents were not changed (list already empty).
	*/
	bool	resetList_impl(void);

	/*!
	\brief
	   Internal handler that is triggered when the user interacts with the scrollbars.
    */
    bool handle_scrollChange(const EventArgs& args);


    // validate window renderer
    virtual bool validateWindowRenderer(const WindowRenderer* renderer) const;

    /*!
    \brief
        Causes the internal list to be (re)sorted.
    */
    void resortList();

	/*************************************************************************
		New event handlers
	*************************************************************************/
	/*!
	\brief
		Handler called internally when the list contents are changed
	*/
	virtual	void	onListContentsChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called internally when the currently selected item or items changes.
	*/
	virtual	void	onSelectionChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called internally when the sort mode setting changes.
	*/
	virtual	void	onSortModeChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called internally when the multi-select mode setting changes.
	*/
	virtual	void	onMultiselectModeChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called internally when the forced display of the vertical scroll bar setting changes.
	*/
	virtual	void	onVertScrollbarModeChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called internally when the forced display of the horizontal scroll bar setting changes.
	*/
	virtual	void	onHorzScrollbarModeChanged(WindowEventArgs& e);


	/*************************************************************************
		Overridden Event handlers
	*************************************************************************/
	virtual void	onSized(ElementEventArgs& e);
	virtual void	onMouseButtonDown(MouseEventArgs& e);
	virtual	void	onMouseWheel(MouseEventArgs& e);
	virtual void	onMouseMove(MouseEventArgs& e);


	/*************************************************************************
		Implementation Data
	*************************************************************************/
	typedef	std::vector<ListboxItem*
        CEGUI_VECTOR_ALLOC(ListboxItem*)> LBItemList;
	bool	d_sorted;				//!< true if list is sorted
	bool	d_multiselect;			//!< true if multi-select is enabled
	bool	d_forceVertScroll;		//!< true if vertical scrollbar should always be displayed
	bool	d_forceHorzScroll;		//!< true if horizontal scrollbar should always be displayed
	bool	d_itemTooltips;			//!< true if each item should have an individual tooltip
	LBItemList	d_listItems;		//!< list of items in the list box.
	ListboxItem*	d_lastSelected;	//!< holds pointer to the last selected item (used in range selections)

    friend class ListboxWindowRenderer;

private:

	/*************************************************************************
		Private methods
	*************************************************************************/
	void	addListboxProperties(void);
};


/*!
\brief
	Helper function used in sorting to compare two list box item text strings
	via the ListboxItem pointers and return if \a a is less than \a b.
*/
bool lbi_less(const ListboxItem* a, const ListboxItem* b);


/*!
\brief
	Helper function used in sorting to compare two list box item text strings
	via the ListboxItem pointers and return if \a a is greater than \a b.
*/
bool lbi_greater(const ListboxItem* a, const ListboxItem* b);

} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUIListbox_h_
