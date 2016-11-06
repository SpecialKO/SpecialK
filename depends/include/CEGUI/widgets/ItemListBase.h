/***********************************************************************
	created:	31/3/2005
	author:		Tomas Lindquist Olsen (based on original Listbox code by Paul D Turner)
	
	purpose:	Interface to base class for ItemListBase widgets
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
#ifndef _CEGUIItemListBase_h_
#define _CEGUIItemListBase_h_

#include "../Base.h"
#include "../Window.h"
#include "./ItemEntry.h"

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
    Base class for ItemListBase window renderer.
*/
class CEGUIEXPORT ItemListBaseWindowRenderer : public WindowRenderer
{
public:
    /*!
    \brief
        Constructor
    */
    ItemListBaseWindowRenderer(const String& name);

    /*!
    \brief
        Return a Rect object describing, in un-clipped pixels, the window relative area
        that is to be used for rendering list items.

    \return
        Rect object describing the window relative area of the that is to be used for rendering
        the items.
    */
    virtual Rectf getItemRenderArea(void) const = 0;
};

/*!
\brief
	Base class for item list widgets.
*/
class CEGUIEXPORT ItemListBase : public Window
{
public:
	static const String EventNamespace;				//!< Namespace for global events

    /*!
    \brief
        Sort modes for ItemListBase
    */
    enum SortMode
    {
        Ascending,
        Descending,
        UserSort
    };

    //!< Sorting callback type
    typedef bool (*SortCallback)(const ItemEntry* a, const ItemEntry* b);

	/*************************************************************************
		Constants
	*************************************************************************/
	// event names
    /** Event fired when the contents of the list is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ItemListBase whose contents
     * has changed.
     */
	static const String EventListContentsChanged;
    /** Event fired when the sort enabled state of the list is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ItemListBase whose sort enabled mode
     * has been changed.
     */
    static const String EventSortEnabledChanged;
    /** Event fired when the sort mode of the list is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ItemListBase whose sorting mode
     * has been changed.
     */
    static const String EventSortModeChanged;

	/*************************************************************************
		Accessor Methods
	*************************************************************************/
	/*!
	\brief
		Return number of items attached to the list

	\return
		the number of items currently attached to this list.
	*/
	size_t	getItemCount(void) const		{return d_listItems.size();}


	/*!
	\brief
		Return the item at index position \a index.

	\param index
		Zero based index of the item to be returned.

	\return
		Pointer to the ItemEntry at index position \a index in the list.

	\exception	InvalidRequestException	thrown if \a index is out of range.
	*/
	ItemEntry*	getItemFromIndex(size_t index) const;


	/*!
	\brief
		Return the index of ItemEntry \a item

	\param item
		Pointer to a ItemEntry whos zero based index is to be returned.

	\return
		Zero based index indicating the position of ItemEntry \a item in the list.

	\exception	InvalidRequestException	thrown if \a item is not attached to this list.
	*/
	size_t	getItemIndex(const ItemEntry* item) const;


	/*!
	\brief
		Search the list for an item with the specified text

	\param text
		String object containing the text to be searched for.

	\param start_item
		ItemEntry where the search is to begin, the search will not include \a item.  If \a item is
		NULL, the search will begin from the first item in the list.

	\return
		Pointer to the first ItemEntry in the list after \a item that has text matching \a text.  If
		no item matches the criteria NULL is returned.

	\exception	InvalidRequestException	thrown if \a item is not attached to this list box.
	*/
	ItemEntry*	findItemWithText(const String& text, const ItemEntry* start_item);


	/*!
	\brief
		Return whether the specified ItemEntry is in the List

	\return
		true if ItemEntry \a item is in the list, false if ItemEntry \a item is not in the list.
	*/
	bool	isItemInList(const ItemEntry* item) const;


	/*!
	\brief
		Return whether this window is automatically resized to fit its content.

	\return
		true if automatic resizing is enabled, false if it is disabled.
	*/
	bool isAutoResizeEnabled() const		{return d_autoResize;}


    /*!
    \brief
        Returns 'true' if the list is sorted
    */
    bool isSortEnabled(void) const          {return d_sortEnabled;}


    /*!
    \brief
        Get sort mode.
    */
    SortMode getSortMode(void) const        {return d_sortMode;}


    /*!
    \brief
        Get user sorting callback.
    */
    SortCallback getSortCallback(void) const {return d_sortCallback;}

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
    virtual void initialiseComponents(void);


	/*!
	\brief
		Remove all items from the list.

		Note that this will cause items, which does not have the 'DestroyedByParent' property set to 'false', to be deleted.
	*/
	void	resetList(void);


	/*!
	\brief
		Add the given ItemEntry to the list.

	\param item
		Pointer to the ItemEntry to be added to the list.  Note that it is the passed object that is added to the
		list, a copy is not made.  If this parameter is NULL, nothing happens.

	\return
		Nothing.
	*/
	void	addItem(ItemEntry* item);


	/*!
	\brief
		Insert an item into the list before a specified item already in the list.

		Note that if the list is sorted, the item may not end up in the
        requested position.

	\param item
		Pointer to the ItemEntry to be inserted.  Note that it is the passed
        object that is added to the list, a copy is not made.  If this parameter
        is NULL, nothing happens.

	\param position
		Pointer to a ItemEntry that \a item is to be inserted before.  If this
        parameter is NULL, the item is inserted at the start of the list.

	\return
		Nothing.
	*/
	void	insertItem(ItemEntry* item, const ItemEntry* position);


	/*!
	\brief
		Removes the given item from the list.  If the item is has the 'DestroyedByParent' property set to 'true', the item will be deleted.

	\param item
		Pointer to the ItemEntry that is to be removed.  If \a item is not attached to this list then nothing
		will happen.

	\return
		Nothing.
	*/
	void	removeItem(ItemEntry* item);


    /*!
	\brief
		Causes the list to update it's internal state after changes have been made to one or more
		attached ItemEntry objects.

		It should not be necessary to call this from client code, as the ItemEntries themselves call it if their parent is an ItemListBase.

    \param resort
        'true' to redo the list sorting as well.
        'false' to only do layout and perhaps auto resize.
        (defaults to 'false')

	\return
		Nothing.
	*/
	void	handleUpdatedItemData(bool resort=false);


	/*!
	\brief
		Set whether or not this ItemListBase widget should automatically resize to fit its content.

	\param setting
		Boolean value that if true enables automatic resizing, if false disables automatic resizing.

	\return
		Nothing.
	*/
	void setAutoResizeEnabled(bool setting);


	/*!
	\brief
	Resize the ItemListBase to exactly fit the content that is attached to it.
	Return a Rect object describing, in un-clipped pixels, the window relative area
	that is to be used for rendering items.

	\return
	Nothing
	*/
	virtual	void	sizeToContent(void)		{sizeToContent_impl();}


    /*!
    \brief
        Triggers a ListContentsChanged event.
        These are not fired during initialisation for optimization purposes.
    */
    virtual void endInitialisation(void);


    //! \copydoc Window::performChildWindowLayout(bool ,bool)
    void performChildWindowLayout(bool nonclient_sized_hint = false,
                                  bool client_sized_hint = false);


    /*!
    \brief
        Return a Rect object describing, in un-clipped pixels, the window relative area
        that is to be used for rendering list items.

    \return
        Rect object describing the window relative area of the that is to be used for rendering
        the items.
    */
    Rectf getItemRenderArea(void) const;

    /*!
    \brief
        Returns a pointer to the window that all items are directed too.
        
    \return
        A pointer to the content pane window, or 'this' if children are added
        directly to this window.
    */
    Window* getContentPane(void) const  {return d_pane;}

    /*!
    \brief
        Notify this ItemListBase that the given item was just clicked.
        Internal function - NOT to be used from client code.
    */
    virtual void notifyItemClicked(ItemEntry*) {}

    /*!
    \brief
        Notify this ItemListBase that the given item just changed selection state.
        Internal function - NOT to be used from client code.
    */
    virtual void notifyItemSelectState(ItemEntry*, bool) {}

    /*!
    \brief
        Set whether the list should be sorted (by text).
    */
    void setSortEnabled(bool setting);

    /*!
    \brief
        Set mode to be used when sorting the list.
    \param mode
        SortMode enum.
    */
    void setSortMode(SortMode mode);

    /*!
    \brief
        Set a user callback as sorting function

    \param mode
        SortCallback
    */
    void setSortCallback(SortCallback cb);

    /*!
    \brief
        Sort the list.

    \param relayout
        True if the item layout should be redone after the sorting.
        False to only sort the internal list. Nothing more.

        This parameter defaults to true and should generally not be
        used in client code.
    */
    void sortList(bool relayout=true);

	/*************************************************************************
		Construction and Destruction
	*************************************************************************/
	/*!
	\brief
		Constructor for ItemListBase base class.
	*/
	ItemListBase(const String& type, const String& name);


	/*!
	\brief
		Destructor for ItemListBase base class.
	*/
	virtual ~ItemListBase(void);


protected:
	/*************************************************************************
		Abstract Implementation Functions (must be provided by derived class)
	*************************************************************************/
	/*!
	\brief
		Resize the ItemListBase to exactly fit the content that is attached to it.
		Return a Rect object describing, in un-clipped pixels, the window relative area
		that is to be used for rendering items.

	\return
		Nothing
	*/
	virtual void	sizeToContent_impl(void);


	/*!
	\brief
		Returns the Size in unclipped pixels of the content attached to this ItemListBase that is attached to it.

	\return
		Size object describing in unclipped pixels the size of the content ItemEntries attached to this menu.
	*/
	virtual Sizef getContentSize() const = 0;


	/*!
	\brief
		Return a Rect object describing, in un-clipped pixels, the window relative area
		that is to be used for rendering list items.

	\return
		Rect object describing the window relative area of the that is to be used for rendering
		the items.
	*/
	//virtual	Rect	getItemRenderArea_impl(void) const		= 0;


	/*!
	\brief
		Setup size and position for the item widgets attached to this ItemListBase

	\return
		Nothing.
	*/
	virtual void	layoutItemWidgets()	= 0;


	/*************************************************************************
		Implementation Functions
	*************************************************************************/
	/*!
	\brief
		Remove all items from the list.

	\note
		Note that this will cause items with the 'DestroyedByParent' property set to 'true', to be deleted.

	\return
		- true if the list contents were changed.
		- false if the list contents were not changed (list already empty).
	*/
	bool	resetList_impl(void);

    // validate window renderer
    virtual bool validateWindowRenderer(const WindowRenderer* renderer) const;

    /*!
    \brief
        Returns the SortCallback that's really going to be used for the sorting operation.
    */
    SortCallback getRealSortCallback(void) const;

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
        Handler called internally when sorting gets enabled.
    */
    virtual void onSortEnabledChanged(WindowEventArgs& e);

    /*!
    \brief
        Handler called internally when the sorting mode is changed.
    */
    virtual void onSortModeChanged(WindowEventArgs& e);

	/*************************************************************************
		Overridden Event handlers
	*************************************************************************/
    virtual void onParentSized(ElementEventArgs& e);
	//virtual void    onChildRemoved(WindowEventArgs& e);
    //virtual void    onDestructionStarted(WindowEventArgs& e);


    /*!
    \brief
        Handler to manage items being removed from the content pane.
        If there is one!

    \note
        If you override this, you should call this base class version to
        ensure correct behaviour is maintained.
    */
    virtual bool handle_PaneChildRemoved(const EventArgs& e);

	/*************************************************************************
		Implementation Data
	*************************************************************************/
	typedef	std::vector<ItemEntry*
        CEGUI_VECTOR_ALLOC(ItemEntry*)> ItemEntryList;
	ItemEntryList	d_listItems;		//!< list of items in the list.

    //!< True if this ItemListBase widget should automatically resize to fit its content. False if not.
	bool d_autoResize;

    //!< Pointer to the content pane (for items), 0 if we're not using one
    Window* d_pane;

    //!< True if this ItemListBase is sorted. False if not.
    bool d_sortEnabled;
    //!< The current sorting mode applied if sorting is enabled.
    SortMode d_sortMode;
    //!< The user sort callback or 0 if none.
    SortCallback d_sortCallback;
    //!< True if the list needs to be resorted.
    bool d_resort;

private:
	/*************************************************************************
		Private methods
	*************************************************************************/
	void	addItemListBaseProperties(void);


	/*!
	\copydoc Element::addChild_impl
	*/
	virtual void addChild_impl(Element* element);
};


template<>
class PropertyHelper<ItemListBase::SortMode>
{
public:
    typedef ItemListBase::SortMode return_type;
    typedef return_type safe_method_return_type;
    typedef ItemListBase::SortMode pass_type;
    typedef String string_return_type;

    static const String& getDataTypeName()
    {
        static String type("SortMode");

        return type;
    }

    static return_type fromString(const String& str)
    {
        if (str == "Ascending")
        {
            return ItemListBase::Ascending;
        }
        else if (str == "Descending")
        {
            return ItemListBase::Descending;
        }
        else
        {
            return ItemListBase::UserSort;
        }
    }

    static string_return_type toString(pass_type val)
    {
        if (val == ItemListBase::UserSort)
        {
            return "UserSort";
        }
        else if (val == ItemListBase::Ascending)
        {
            return "Ascending";
        }
        else if (val == ItemListBase::Descending)
        {
            return "Descending";
        }
        else
        {
            assert(false && "Invalid sort mode");
            return "Ascending";
        }
    }
};


} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUIItemListBase_h_
