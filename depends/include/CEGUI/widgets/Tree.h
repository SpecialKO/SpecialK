/***********************************************************************
    created:	5-13-07
    author:		Jonathan Welch (Based on Code by David Durant)
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
#ifndef _CEGUITree_h_
#define _CEGUITree_h_

#include "../Base.h"
#include "../Window.h"
#include "../WindowManager.h"
#include "./TreeItem.h"
#include "./Scrollbar.h"
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
    EventArgs based class that is used for objects passed to input event
    handlers concerning Tree events.
 */
class CEGUIEXPORT TreeEventArgs : public WindowEventArgs
{
public:
    TreeEventArgs(Window* wnd) : WindowEventArgs(wnd)
        { treeItem = 0; }

    TreeItem *treeItem;
};


/*!
 \brief
    Base class for standard Tree widget.

 \deprecated
    The CEGUI::Tree, CEGUI::TreeItem and any other associated classes are
    deprecated and thier use should be minimised - preferably eliminated -
    where possible.  It is extremely unfortunate that this widget was ever added
    to CEGUI since its design and implementation are poor and do not meet 
    established standards for the CEGUI project.
    \par
    While no alternative currently exists, a superior, replacement tree widget
    will be provided prior to the final removal of the current implementation.
 */
class CEGUIEXPORT Tree : public Window
{
    friend class TreeItem;

    typedef	std::vector<TreeItem*
        CEGUI_VECTOR_ALLOC(TreeItem*)> LBItemList;
    
public:
    //! Namespace for global events
    static const String EventNamespace;
    static const String WidgetTypeName;
    
    /*************************************************************************
        Constants
     *************************************************************************/
    // event names
    /** Event fired when the content of the tree is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Tree whose content has changed.
     */
    static const String EventListContentsChanged;
    /** Event fired when there is a change to the currently selected item(s).
     * Handlers are passed a const TreeEventArgs reference with
     * WindowEventArgs::window set to the Tree whose item selection has changed,
     * and TreeEventArgs::treeItem is set to the (last) item to be selected, or
     * 0 if none.
     */
    static const String EventSelectionChanged;
    /** Event fired when the sort mode setting for the Tree is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Tree whose sort mode has been
     * changed.
     */
    static const String EventSortModeChanged;
    /** Event fired when the multi-select mode setting for the Tree changes.
     * Handlers are passed a const TreeEventArgs reference with
     * WindowEventArgs::window set to the Tree whose setting has changed.
     * TreeEventArgs::treeItem is always set to 0.
     */
    static const String EventMultiselectModeChanged;
    /** Event fired when the mode setting that forces the display of the
     * vertical scroll bar for the tree is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Tree whose vertical scrollbar mode has
     * been changed.
     */
    static const String EventVertScrollbarModeChanged;
    /** Event fired when the mode setting that forces the display of the
     * horizontal scroll bar for the tree is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Tree whose horizontal scrollbar mode
     * has been changed.
     */
    static const String EventHorzScrollbarModeChanged;
    /** Event fired when a branch of the tree is opened by the user.
     * Handlers are passed a const TreeEventArgs reference with
     * WindowEventArgs::window set to the Tree containing the branch that has
     * been opened and TreeEventArgs::treeItem set to the TreeItem at the head
     * of the opened branch.
     */
    static const String EventBranchOpened;
    /** Event fired when a branch of the tree is closed by the user.
     * Handlers are passed a const TreeEventArgs reference with
     * WindowEventArgs::window set to the Tree containing the branch that has
     * been closed and TreeEventArgs::treeItem set to the TreeItem at the head
     * of the closed branch.
     */
    static const String EventBranchClosed;

    //Render the actual tree
    void doTreeRender()
        { populateGeometryBuffer(); }

    //UpdateScrollbars
    void doScrollbars()
        { configureScrollbars(); }

    /*************************************************************************
        Accessor Methods
     *************************************************************************/
    /*!
     \brief
        Return number of items attached to the tree
     
     \return
        the number of items currently attached to this tree.
     */
    size_t getItemCount(void) const
        { return d_listItems.size(); }

    /*!
     \brief
        Return the number of selected items in the tree.
     
     \return
        Total number of attached items that are in the selected state.
     */
    size_t getSelectedCount(void) const;

    /*!
     \brief
        Return a pointer to the first selected item.
     
     \return
        Pointer to a TreeItem based object that is the first selected item in
        the tree.  will return 0 if no item is selected.
     */
    TreeItem* getFirstSelectedItem(void) const;
        
    /*!
     \brief
        Return a pointer to the first selected item.
     
     \return
        Pointer to a TreeItem based object that is the last item selected by the
        user, not necessarily the last selected in the tree.  Will return 0 if
        no item is selected.
     */
    TreeItem* getLastSelectedItem(void) const
        { return d_lastSelected; }

    /*!
     \brief
        Return a pointer to the next selected item after item \a start_item
     
     \param start_item
         Pointer to the TreeItem where the search for the next selected item is
         to begin.  If this parameter is 0, the search will begin with the first
         item in the tree.
     
     \return
         Pointer to a TreeItem based object that is the next selected item in
         the tree after the item specified by \a start_item.  Will return 0 if
         no further items were selected.
     
     \exception	InvalidRequestException	thrown if \a start_item is not attached
        to this tree.
     */
    TreeItem* getNextSelected(const TreeItem* start_item) const;
    
    TreeItem* getNextSelectedItemFromList(const LBItemList &itemList,
                                          const TreeItem* start_item,
                                          bool& foundStartItem) const;

    /*!
     \brief
        return whether tree sorting is enabled
     
     \return
        - true if the tree is sorted
        - false if the tree is not sorted
     */
    bool isSortEnabled(void) const
        { return d_sorted; }
    
    void setItemRenderArea(Rectf& r)
        { d_itemArea = r; }
    
    Scrollbar* getVertScrollbar()
        { return d_vertScrollbar; }

    Scrollbar* getHorzScrollbar()
        { return d_horzScrollbar; }

    /*!
     \brief
        return whether multi-select is enabled
     
     \return
        true if multi-select is enabled, false if multi-select is not enabled.
     */
    bool isMultiselectEnabled(void) const
        { return d_multiselect; }
    
    bool isItemTooltipsEnabled(void) const
        { return d_itemTooltips; }

    /*!
     \brief
        Search the tree for an item with the specified text
     
     \param text
        String object containing the text to be searched for.
     
     \param start_item
        TreeItem where the search is to begin, the search will not include \a
        item.  If \a item is 0, the search will begin from the first item in
        the tree.
     
     \return
        Pointer to the first TreeItem in the tree after \a item that has text
        matching \a text.  If no item matches the criteria, 0 is returned.
     
     \exception	InvalidRequestException	thrown if \a item is not attached to
        this tree.
     */
    TreeItem* findFirstItemWithText(const String& text);

    TreeItem* findNextItemWithText(const String& text,
                                   const TreeItem* start_item);

    TreeItem* findItemWithTextFromList(const LBItemList &itemList,
                                       const String& text,
                                       const TreeItem* start_item,
                                       bool foundStartItem);

    /*!
     \brief
        Search the tree for an item with the specified text
     
     \param text
        String object containing the text to be searched for.
     
     \param start_item
        TreeItem where the search is to begin, the search will not include
        \a item.  If \a item is 0, the search will begin from the first item in
        the tree.
     
     \return
        Pointer to the first TreeItem in the tree after \a item that has text
        matching \a text.  If no item matches the criteria 0 is returned.
     
     \exception	InvalidRequestException	thrown if \a item is not attached to
        this tree.
     */
    TreeItem* findFirstItemWithID(uint searchID);
    TreeItem* findNextItemWithID(uint searchID, const TreeItem* start_item);
    TreeItem* findItemWithIDFromList(const LBItemList &itemList, uint searchID,
                                     const TreeItem* start_item,
                                     bool foundStartItem);

    /*!
     \brief
        Return whether the specified TreeItem is in the tree
     
     \return
        - true if TreeItem \a item is in the tree
        - false if TreeItem \a item is not in the tree.
     */
    bool isTreeItemInList(const TreeItem* item) const;

    /*!
     \brief
        Return whether the vertical scroll bar is always shown.
     
     \return
         - true if the scroll bar will always be shown even if it is not required.
         - false if the scroll bar will only be shown when it is required.
     */
    bool isVertScrollbarAlwaysShown(void) const;

    /*!
     \brief
        Return whether the horizontal scroll bar is always shown.
     
     \return
         - true if the scroll bar will always be shown even if it is not required.
         - false if the scroll bar will only be shown when it is required.
     */
    bool isHorzScrollbarAlwaysShown(void) const;

    /*************************************************************************
        Manipulator Methods
     *************************************************************************/
    /*!
     \brief
        Initialise the Window based object ready for use.
     
     \note
        This must be called for every window created.  Normally this is handled
        automatically by the WindowFactory for each Window type.

     \return
        Nothing
     */
    virtual void initialise(void);

    /*!
     \brief
        Remove all items from the tree.
     
        Note that this will cause 'AutoDelete' items to be deleted.
     */
    void resetList(void);

    /*!
     \brief
        Add the given TreeItem to the tree.
     
     \param item
         Pointer to the TreeItem to be added to the tree.  Note that it is the
         passed object that is added to the tree, a copy is not made.  If this
         parameter is NULL, nothing happens.
     
     \return
        Nothing.
     */
    void addItem(TreeItem* item);

    /*!
     \brief
        Insert an item into the tree after a specified item already in the
        tree.
     
        Note that if the tree is sorted, the item may not end up in the
        requested position.
     
     \param item
         Pointer to the TreeItem to be inserted.  Note that it is the passed
         object that is added to the tree, a copy is not made.  If this
         parameter is 0, nothing happens.
     
     \param position
         Pointer to a TreeItem that \a item is to be inserted after.  If this
         parameter is 0, the item is inserted at the start of the tree.
     
     \return
        Nothing.
     
     \exception InvalidRequestException	thrown if no TreeItem \a position is
        attached to this tree.
     */
    void insertItem(TreeItem* item, const TreeItem* position);

    /*!
     \brief
        Removes the given item from the tree.  If the item is has the auto
        delete state set, the item will be deleted.
     
     \param item
        Pointer to the TreeItem that is to be removed.  If \a item is not
        attached to this tree then nothing will happen.
     
     \return
        Nothing.
     */
    void removeItem(const TreeItem* item);

    /*!
     \brief
        Clear the selected state for all items.
     
     \return
        Nothing.
     */
    void clearAllSelections(void);
    bool clearAllSelectionsFromList(const LBItemList &itemList);

    /*!
     \brief
        Set whether the tree should be sorted.
     
     \param setting
        - true if the tree should be sorted
        - false if the tree should not be sorted.
     
     \return
        Nothing.
     */
    void setSortingEnabled(bool setting);

    /*!
     \brief
        Set whether the tree should allow multiple selections or just a single
        selection.
     
     \param  setting
         - true if the widget should allow multiple items to be selected
         - false if the widget should only allow a single selection.

     \return
        Nothing.
     */
    void setMultiselectEnabled(bool setting);

    /*!
     \brief
        Set whether the vertical scroll bar should always be shown.
     
     \param setting
         - true if the vertical scroll bar should be shown even when it is not
           required.
         - false if the vertical scroll bar should only be shown when it is
           required.

     \return
        Nothing.
     */
    void setShowVertScrollbar(bool setting);

    /*!
     \brief
        Set whether the horizontal scroll bar should always be shown.
     
     \param setting
         - true if the horizontal scroll bar should be shown even when it is not
           required.
         - false if the horizontal scroll bar should only be shown when it is
           required.

     \return
        Nothing.
     */
    void setShowHorzScrollbar(bool setting);
    
    void setItemTooltipsEnabled(bool setting);

    /*!
     \brief
        Set the select state of an attached TreeItem.
     
        This is the recommended way of selecting and deselecting items attached
        to a tree as it respects the multi-select mode setting.  It is
        possible to modify the setting on TreeItems directly, but that approach
        does not respect the settings of the tree.
     
     \param item
        The TreeItem to be affected.
        This item must be attached to the tree.
     
     \param state
        - true to select the item.
        - false to de-select the item.
     
     \return
        Nothing.
     
     \exception	InvalidRequestException	thrown if \a item is not attached to
        this tree.
     */
    void setItemSelectState(TreeItem* item, bool state);

    /*!
     \brief
        Set the select state of an attached TreeItem.
     
        This is the recommended way of selecting and deselecting items attached
        to a tree as it respects the multi-select mode setting.  It is
        possible to modify the setting on TreeItems directly, but that approach
        does not respect the settings of the tree.
     
     \param item_index
        The zero based index of the TreeItem to be affected.
        This must be a valid index (0 <= index < getItemCount())

     \param state
        - true to select the item.
        - false to de-select the item.
     
     \return
        Nothing.
     
     \exception	InvalidRequestException	thrown if \a item_index is out of range
        for the tree
     */
    void setItemSelectState(size_t item_index, bool state);
    
    /*!
     \brief
        Set the LookNFeel that shoule be used for this window.
     
     \param look
        String object holding the name of the look to be assigned to the window.
     
     \return
        Nothing.
     
     \exception UnknownObjectException
        thrown if the look'n'feel specified by \a look does not exist.
     
     \note
        Once a look'n'feel has been assigned it is locked - as in cannot be
        changed.
     */
    virtual void setLookNFeel(const String& look);
    
    /*!
     \brief
        Causes the tree to update it's internal state after changes have
        been made to one or more attached TreeItem objects.
     
        Client code must call this whenever it has made any changes to TreeItem
        objects already attached to the tree.  If you are just adding items,
        or removed items to update them prior to re-adding them, there is no
        need to call this method.
     
     \return
        Nothing.
     */
    void handleUpdatedItemData(void);

    /*!
     \brief
        Ensure the item at the specified index is visible within the tree.
     
     \param item
        Pointer to the TreeItem to be made visible in the tree.
     
     \return
        Nothing.
     
     \exception	InvalidRequestException	thrown if \a item is not attached to
        this tree.
     */
    void ensureItemIsVisible(const TreeItem* item);
    
    
    /*************************************************************************
     Construction and Destruction
     *************************************************************************/
    /*!
     \brief
        Constructor for Tree base class.
     */
    Tree(const String& type, const String& name);

    /*!
     \brief
        Destructor for Tree base class.
     */
    virtual ~Tree(void);

protected:
    /*************************************************************************
        Abstract Implementation Functions (must be provided by derived class)
     *************************************************************************/
    /*!
     \brief
        Return a Rect object describing, in un-clipped pixels, the window
        relative area that is to be used for rendering tree items.
     
     \return
        Rect object describing the area of the Window to be used for rendering
        tree items.
     */
    virtual	Rectf getTreeRenderArea(void) const
        { return d_itemArea; }
    
    /*!
     \brief
        create and return a pointer to a Scrollbar widget for use as vertical
        scroll bar.
     
     \param name
        String holding the name to be given to the created widget component.
     
     \return
        Pointer to a Scrollbar to be used for scrolling the tree vertically.
     */
    virtual Scrollbar* createVertScrollbar(const String& name) const
        { return static_cast<Scrollbar*>(getChild(name)); }

    /*!
     \brief
        create and return a pointer to a Scrollbar widget for use as horizontal
        scroll bar.
     
     \param name
        String holding the name to be given to the created widget component.
     
     \return
        Pointer to a Scrollbar to be used for scrolling the tree horizontally.
     */
    virtual Scrollbar* createHorzScrollbar(const String& name) const
        { return static_cast<Scrollbar*>(getChild(name)); }

    /*!
     \brief
         Perform caching of the widget control frame and other 'static' areas.
         This method should not render the actual items.  Note that the items
         are typically rendered to layer 3, other layers can be used for
         rendering imagery behind and infront of the items.
     
     \return
        Nothing.
     */
    virtual	void cacheTreeBaseImagery()
    {}
    
    /*************************************************************************
        Implementation Functions
     *************************************************************************/
    /*!
     \brief
        Checks if a tree item is visible (searches sub-items)
    */
    bool containsOpenItemRecursive(const LBItemList& itemList, TreeItem* item);

    /*!
     \brief
        Add tree specific events
     */
    void addTreeEvents(void);
    
    
    /*!
     \brief
        display required integrated scroll bars according to current state of
        the tree and update their values.
     */
    void configureScrollbars(void);
    
    /*!
     \brief
        select all strings between positions \a start and \a end.  (inclusive)
        including \a end.
     */
    void selectRange(size_t start, size_t end);
    
    /*!
     \brief
        Return the sum of all item heights
     */
    float getTotalItemsHeight(void) const;
    void getTotalItemsInListHeight(const LBItemList &itemList,
                                    float *heightSum) const;

    /*!
     \brief
        Return the width of the widest item
     */
    float getWidestItemWidth(void) const;
    void getWidestItemWidthInList(const LBItemList &itemList, int itemDepth,
                                  float *widest) const;
    
    /*!
     \brief
        Clear the selected state for all items (implementation)
     
     \return
        - true if treeItem was found in the search.
        - false if it was not.
     */
    bool getHeightToItemInList(const LBItemList &itemList,
                               const TreeItem *treeItem,
                               int itemDepth,
                               float *height) const;

    /*!
     \brief
        Clear the selected state for all items (implementation)
     
     \return
        - true if some selections were cleared
        - false nothing was changed.
     */
    bool clearAllSelections_impl(void);

    /*!
     \brief
        Return the TreeItem under the given window local pixel co-ordinate.
     
     \return
         TreeItem that is under window pixel co-ordinate \a pt, or 0 if no
         item is under that position.
     */
    TreeItem* getItemAtPoint(const Vector2f& pt) const;
    TreeItem* getItemFromListAtPoint(const LBItemList &itemList, float *bottomY,
                                     const Vector2f& pt) const;

    /*!
     \brief
        Remove all items from the tree.
     
     \note
        Note that this will cause 'AutoDelete' items to be deleted.
     
     \return
         - true if the tree contents were changed.
         - false if the tree contents were not changed (tree already empty).
     */
    bool resetList_impl(void);
    
    /*!
     \brief
        Internal handler that is triggered when the user interacts with the
        scrollbars.
     */
    bool handle_scrollChange(const EventArgs& args);
    
    // overridden from Window base class.
    virtual void populateGeometryBuffer();
    bool handleFontRenderSizeChange(const EventArgs& args);
 
     void drawItemList(LBItemList& itemList, Rectf& itemsArea, float widest,
                       Vector2f& itemPos, GeometryBuffer& geometry, float alpha);
    
    /*************************************************************************
        New event handlers
     *************************************************************************/
    /*!
     \brief
        Handler called internally when the tree contents are changed
     */
    virtual	void onListContentsChanged(WindowEventArgs& e);

    /*!
     \brief
        Handler called internally when the currently selected item or items
        changes.
     */
    virtual	void onSelectionChanged(TreeEventArgs& e);

    /*!
     \brief
        Handler called internally when the sort mode setting changes.
     */
    virtual	void onSortModeChanged(WindowEventArgs& e);

    /*!
     \brief
        Handler called internally when the multi-select mode setting changes.
     */
    virtual	void onMultiselectModeChanged(WindowEventArgs& e);

    /*!
     \brief
        Handler called internally when the forced display of the vertical scroll
        bar setting changes.
     */
    virtual	void onVertScrollbarModeChanged(WindowEventArgs& e);

    /*!
     \brief
        Handler called internally when the forced display of the horizontal
        scroll bar setting changes.
     */
    virtual	void onHorzScrollbarModeChanged(WindowEventArgs& e);
    
    /*!
     \brief
        Handler called internally when the user opens a branch of the tree.
     */
    virtual	void onBranchOpened(TreeEventArgs& e);
    
    /*!
     \brief
        Handler called internally when the user closes a branch of the tree.
     */
    virtual	void onBranchClosed(TreeEventArgs& e);
    
    /*************************************************************************
        Overridden Event handlers
     *************************************************************************/
    virtual void onSized(ElementEventArgs& e);
    virtual void onMouseButtonDown(MouseEventArgs& e);
    virtual	void onMouseWheel(MouseEventArgs& e);
    virtual void onMouseMove(MouseEventArgs& e);

    /*************************************************************************
        Implementation Data
     *************************************************************************/
    //! true if tree is sorted
    bool d_sorted;
    //! true if multi-select is enabled
    bool d_multiselect;
    //! true if vertical scrollbar should always be displayed
    bool d_forceVertScroll;
    //! true if horizontal scrollbar should always be displayed
    bool d_forceHorzScroll;
    //! true if each item should have an individual tooltip
    bool d_itemTooltips;
    //! vertical scroll-bar widget
    Scrollbar* d_vertScrollbar;
    //! horizontal scroll-bar widget
    Scrollbar* d_horzScrollbar;
    //! list of items in the tree.
    LBItemList d_listItems;
    //! holds pointer to the last selected item (used in range selections)
    TreeItem* d_lastSelected;
    const ImagerySection* d_openButtonImagery;
    const ImagerySection* d_closeButtonImagery;

private:
    /*************************************************************************
        Private methods
     *************************************************************************/
    void addTreeProperties(void);
    Rectf d_itemArea;
};

/*!
 \brief
    Helper function used in sorting to compare two tree item text strings
    via the TreeItem pointers and return if \a a is less than \a b.
 */
bool lbi_less(const TreeItem* a, const TreeItem* b);


/*!
 \brief
    Helper function used in sorting to compare two tree item text strings
    via the TreeItem pointers and return if \a a is greater than \a b.
 */
bool lbi_greater(const TreeItem* a, const TreeItem* b);

} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUITree_h_
