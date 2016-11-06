/************************************************************************
    created:    Tue Sep 27 2005
    author:     Tomas Lindquist Olsen
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
#ifndef _CEGUIItemListbox_h_
#define _CEGUIItemListbox_h_

#include "./ScrolledItemListBase.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

// begin CEGUI namespace
namespace CEGUI
{

/*!
\brief
    ItemListbox window class
*/
class CEGUIEXPORT ItemListbox : public ScrolledItemListBase
{
public:
    static const String EventNamespace; //!< Namespace for global events
    static const String WidgetTypeName; //!< Window factory name

    /************************************************************************
        Constants
    *************************************************************************/
    /** Event fired when the list selection changes.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ItemListbox whose current selection
     * has been changed.
     */
    static const String EventSelectionChanged;
    /** Event fired when the multiselect mode of the list box is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ItemListbox whose multiselect mode
     * has been changed.
     */
    static const String EventMultiSelectModeChanged;

    /************************************************************************
        Accessors
    *************************************************************************/
    /*!
    \brief
        Returns the number of selected items in this ItemListbox.
    */
    size_t getSelectedCount(void) const;

    /*!
    \brief
        Returns a pointer to the last selected item.

    \return
        A pointer to the last selected item, 0 is none.
    */
    ItemEntry* getLastSelectedItem(void) const      {return d_lastSelected;}

    /*!
    \brief
        Returns a pointer to the first selected item

    \param start_index
        The index where the search should begin. If omitted the search will
        begin with the first item.

    \return
        A pointer to the first selected item in the listbox.
        If no item is selected the return value is 0.
        If \a start_index is out of bounds the return value is 0.

    \note
        If multiselect is disabled then this does the equivalent of calling
        getLastSelectedItem.
        If multiselect is enabled it will search the array starting at \a start_index
    */
    ItemEntry* getFirstSelectedItem(size_t start_index=0) const;

    /*!
    \brief
        Returns a pointer to the next seleced item relative to a previous call to
        getFirstSelectedItem or getNextSelectedItem.

    \return
        A pointer to the next seleced item. If there are no further selected items
        the return value is 0.
        If multiselect is disabled the return value is 0.

    \note
        This member function will take on from where the last call to
        getFirstSelectedItem or getNextSelectedItem returned. So be sure to start with a
        call to getFirstSelectedItem.

        This member function should be preferred over getNextSelectedItemAfter as it will
        perform better, especially on large lists.
    */
    ItemEntry* getNextSelectedItem(void) const;

    /*!
    \brief
        Returns a pointer to the next selected item after the item 'start_item' given.

    \note
        This member function will search the array from the beginning and will be slow
        for large lists, it will not advance the internal counter used by
        getFirstSelectedItem and getNextSelectedItem either.
    */
    ItemEntry* getNextSelectedItemAfter(const ItemEntry* start_item) const;

    /*!
    \brief
        Returns 'true' if multiple selections are allowed. 'false' if not.
    */
    bool isMultiSelectEnabled(void) const   {return d_multiSelect;}

    /*!
    \brief
        Returns 'true' if the item at the given index is selectable and currently selected.
    */
    bool isItemSelected(size_t index) const;

    /************************************************************************
        Manipulators
    *************************************************************************/
    /*!
    \brief
        Set whether or not multiple selections should be allowed.
    */
    void setMultiSelectEnabled(bool state);

    /*!
    \brief
        Clears all selections.
    */
    void clearAllSelections(void);

    /*!
    \brief
        Select a range of items.

    \param a
        Start item. (inclusive)

    \param z
        End item. (inclusive)
    */
    void selectRange(size_t a, size_t z);

    /*!
    \brief
        Select all items.
        Does nothing if multiselect is disabled.
    */
    void selectAllItems(void);

    /************************************************************************
        Object Construction and Destruction
    *************************************************************************/
    /*!
    \brief
        Constructor for the ItemListbox base class constructor.
    */
    ItemListbox(const String& type, const String& name);

    /*!
    \brief
        Destructor for the ItemListbox base class.
     */
    virtual ~ItemListbox(void) {}

    /************************************************************************
        Implementation functions
    ************************************************************************/
    /*!
    \brief
        Setup size and position for the item widgets attached to this ItemListbox
    */
    virtual void layoutItemWidgets();

    /*!
    \brief
        Returns the Size in unclipped pixels of the content attached to this ItemListbox.
    */
    virtual Sizef getContentSize() const;

    /*!
    \brief
        Notify this ItemListbox that the given ListItem was just clicked.
        Internal function - not to be used from client code.
    */
    virtual void notifyItemClicked(ItemEntry* li);

    /*!
    \brief
        Notify this ItemListbox that the given ListItem just changed selection state.
        Internal function - not to be used from client code.
    */
    virtual void notifyItemSelectState(ItemEntry* li, bool state);

protected:
    /************************************************************************
        Protected implementation functions
    ************************************************************************/
    /*!
    \brief
        Returns a pointer to the first selected item starting the search
        from \a start_index

    \param start_index
        The index where the search should begin (inclusive)

    \return
        A pointer to the first selected item in the listbox found
        If no item is selected the return value is 0
        If \a start_index is out of bounds the return value is 0

    \note
        This function advances the internal counter and is made for
        getFirstSelectedItem and getNextSelectedItem
    */
    ItemEntry* findSelectedItem(size_t start_index) const;

    // overridden from ItemListBase
    bool handle_PaneChildRemoved(const EventArgs& e);

    /************************************************************************
        New event handlers
    ************************************************************************/
    virtual void onSelectionChanged(WindowEventArgs& e);
    virtual void onMultiSelectModeChanged(WindowEventArgs& e);

    /************************************************************************
        Overridden event handlers
    ************************************************************************/
    virtual void onKeyDown(KeyEventArgs& e);

    /************************************************************************
        Implementation data
    ************************************************************************/
    bool d_multiSelect; //! Controls whether multiple items can be selected simultaneously
    ItemEntry* d_lastSelected; //! The last item that was selected
    mutable size_t d_nextSelectionIndex; //! The index of the last item that was returned with the getFirst/NextSelection members

private:
    void addItemListboxProperties(void);
};

} // end CEGUI namespace

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif // end of guard _CEGUIItemListbox_h_
