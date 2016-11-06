/***********************************************************************
	created:	08/08/2004
	author:		Steve Streeting

	purpose:	Interface to base class for TabControl widget
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
#ifndef _CEGUITabControl_h_
#define _CEGUITabControl_h_

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

    // Forward declaration
    class TabButton;

/*!
\brief
    Base class for TabControl window renderer objects.
*/
class CEGUIEXPORT TabControlWindowRenderer : public WindowRenderer
{
public:
    /*!
    \brief
        Constructor
    */
    TabControlWindowRenderer(const String& name);

    /*!
    \brief
        create and return a pointer to a TabButton widget for use as a clickable tab header
    \param name
        Button name
    \return
        Pointer to a TabButton to be used for changing tabs.
    */
    virtual TabButton*  createTabButton(const String& name) const       = 0;
};

/*!
\brief
	Base class for standard Tab Control widget.
*/
class CEGUIEXPORT TabControl : public Window
{
public:
	static const String EventNamespace;				//!< Namespace for global events
    static const String WidgetTypeName;             //!< Window factory name

	enum TabPanePosition
	{
		Top,
		Bottom
	};

	/*************************************************************************
		Constants
	*************************************************************************/
	// event names
    /** Event fired when a different tab is selected.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the TabControl that has a newly
     * selected tab.
     */
	static const String EventSelectionChanged;

    /*************************************************************************
        Child Widget name constants
    *************************************************************************/
    static const String ContentPaneName;   //!< Widget name for the tab content pane component.
    static const String TabButtonName;     //!< Widget name for the tab button components.
    static const String TabButtonPaneName; //!< Widget name for the tab button pane component.
    static const String ButtonScrollLeft;  //!< Widget name for the scroll tabs to right pane component.
    static const String ButtonScrollRight; //!< Widget name for the scroll tabs to left pane component.


	/*************************************************************************
		Accessor Methods
	*************************************************************************/
	/*!
	\brief
		Return number of tabs

	\return
		the number of tabs currently present.
	*/
	size_t	getTabCount(void) const;

	/*!
	\brief
		Return the positioning of the tab pane.
	\return
		The positioning of the tab window within the tab control.
	*/
	TabPanePosition getTabPanePosition(void) const
	{ return d_tabPanePos; }

	/*!
	\brief
		Change the positioning of the tab button pane.
	\param pos
		The new positioning of the tab pane
	*/
	void	setTabPanePosition(TabPanePosition pos);

    /*!
    \brief
        Set the selected tab by the name of the root window within it.
		Also ensures that the tab is made visible (tab pane is scrolled if required).
    \exception	InvalidRequestException	thrown if there's no tab named \a name.
    */
    void    setSelectedTab(const String &name);

    /*!
    \brief
        Set the selected tab by the ID of the root window within it.
		Also ensures that the tab is made visible (tab pane is scrolled if required).
    \exception	InvalidRequestException	thrown if \a index is out of range.
    */
    void    setSelectedTab(uint ID);

    /*!
    \brief
        Set the selected tab by the index position in the tab control.
		Also ensures that the tab is made visible (tab pane is scrolled if required).
    \exception	InvalidRequestException	thrown if \a index is out of range.
    */
    void    setSelectedTabAtIndex(size_t index);

    /*!
    \brief
        Ensure that the tab by the name of the root window within it is visible.
    \exception	InvalidRequestException	thrown if there's no tab named \a name.
    */
    void    makeTabVisible(const String &name);

    /*!
    \brief
        Ensure that the tab by the ID of the root window within it is visible.
    \exception	InvalidRequestException	thrown if \a index is out of range.
    */
    void    makeTabVisible(uint ID);

    /*!
    \brief
        Ensure that the tab by the index position in the tab control is visible.
    \exception	InvalidRequestException	thrown if \a index is out of range.
    */
    void    makeTabVisibleAtIndex(size_t index);

    /*!
	\brief
		Return the Window which is the first child of the tab at index position \a index.

	\param index
		Zero based index of the item to be returned.

	\return
		Pointer to the Window at index position \a index in the tab control.

	\exception	InvalidRequestException	thrown if \a index is out of range.
	*/
    Window*	getTabContentsAtIndex(size_t index) const;

    /*!
    \brief
        Return the Window which is the tab content with the given name.

    \param name
        Name of the Window which was attached as a tab content.

    \return
        Pointer to the named Window in the tab control.

    \exception	InvalidRequestException	thrown if content is not found.
    */
    Window*	getTabContents(const String& name) const;

    /*!
    \brief
        Return the Window which is the tab content with the given ID.

    \param ID
        ID of the Window which was attached as a tab content.

    \return
        Pointer to the Window with the given ID in the tab control.

    \exception	InvalidRequestException	thrown if content is not found.
    */
    Window*	getTabContents(uint ID) const;

    /*!
	\brief
		Return whether the tab contents window is currently selected.

    \param wnd
        The tab contents window to query.

	\return
		true if the tab is currently selected, false otherwise.

	\exception	InvalidRequestException	thrown if \a wnd is not a valid tab contents window.
	*/
    bool	isTabContentsSelected(Window* wnd) const;

    /*!
	\brief
		Return the index of the currently selected tab.

	\return
		index of the currently selected tab.
	*/
    size_t	getSelectedTabIndex() const;

    /*!
    \brief
        Return the height of the tabs
    */
    const UDim& getTabHeight(void) const { return d_tabHeight; }

    /*!
    \brief
        Return the amount of padding to add either side of the text in the tab
    */
    const UDim& getTabTextPadding(void) const { return d_tabPadding; }


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
        Set the height of the tabs
    */
    void setTabHeight(const UDim& height);

    /*!
    \brief
        Set the amount of padding to add either side of the text in the tab
    */
    void setTabTextPadding(const UDim& padding);

    /*!
    \brief
        Add a new tab to the tab control.
    \par
        The new tab will be added with the same text as the window passed in.
    \param wnd
        The Window which will be placed in the content area of this new tab.
    */
    void addTab(Window* wnd);
    /*!
    \brief
        Remove the named tab from the tab control.
    \par
        The tab content will be destroyed.
    */
    void removeTab(const String& name);
    /*!
    \brief
    Remove the tab with the given ID from the tab control.
    \par
    The tab content will be destroyed.
    */
    void removeTab(uint ID);


	/*************************************************************************
		Construction and Destruction
	*************************************************************************/
	/*!
	\brief
		Constructor for TabControl base class.
	*/
	TabControl(const String& type, const String& name);


	/*!
	\brief
		Destructor for Listbox base class.
	*/
	virtual ~TabControl(void);


protected:

	/*************************************************************************
		Implementation Functions
	*************************************************************************/
    /*!
    \brief
        Perform the actual rendering for this Window.

    \param z
        float value specifying the base Z co-ordinate that should be used when rendering

    \return
        Nothing
    */
    virtual	void	drawSelf(const RenderingContext&) { /* do nothing; rendering handled by children */ }

    /*!
    \brief
        Add a TabButton for the specified child Window.
    */
    virtual void addButtonForTabContent(Window* wnd);
    /*!
    \brief
        Remove the TabButton for the specified child Window.
    */
    virtual void removeButtonForTabContent(Window* wnd);
    /*!
    \brief
        Return the TabButton associated with this Window.
    \exception	InvalidRequestException	thrown if content is not found.
    */
	TabButton* getButtonForTabContents(Window* wnd) const;
    /*!
    \brief
        Construct a button name to handle a window.
    */
    String makeButtonName(Window* wnd);

    /*!
    \brief
        Internal implementation of select tab.
    \param wnd
        Pointer to a Window which is the root of the tab content to select
    */
    virtual void selectTab_impl(Window* wnd);


    /*!
    \brief
        Internal implementation of make tab visible.
    \param wnd
        Pointer to a Window which is the root of the tab content to make visible
    */
    virtual void makeTabVisible_impl(Window* wnd);

    /*!
    \brief
        Return a pointer to the tab button pane (Window)for
        this TabControl.

    \return
        Pointer to a Window object.

    \exception UnknownObjectException
        Thrown if the component does not exist.
    */
    Window* getTabButtonPane() const;

    /*!
    \brief
        Return a pointer to the content component widget for
        this TabControl.

    \return
        Pointer to a Window object.

    \exception UnknownObjectException
        Thrown if the component does not exist.
    */
    Window* getTabPane() const;

    void performChildWindowLayout(bool nonclient_sized_hint = false,
                                  bool client_sized_hint = false);
    int writeChildWindowsXML(XMLSerializer& xml_stream) const;

    // validate window renderer
    virtual bool validateWindowRenderer(const WindowRenderer* renderer) const;

    /*!
    \brief
        create and return a pointer to a TabButton widget for use as a clickable tab header
    \param name
        Button name
    \return
        Pointer to a TabButton to be used for changing tabs.
    */
    TabButton*  createTabButton(const String& name) const;

    //! Implementation function to do main work of removing a tab.
    void removeTab_impl(Window* window);

	/*************************************************************************
		New event handlers
	*************************************************************************/

	/*!
	\brief
		Handler called internally when the currently selected item or items changes.
	*/
	virtual	void	onSelectionChanged(WindowEventArgs& e);

	/*!
	\brief
		Handler called when the window's font is changed.

	\param e
		WindowEventArgs object whose 'window' pointer field is set to the window that triggered the event.  For this
		event the trigger window is always 'this'.
	*/
	virtual void	onFontChanged(WindowEventArgs& e);

	/*************************************************************************
		Implementation Data
	*************************************************************************/
    UDim        d_tabHeight;        //!< The height of the tabs in pixels
    UDim        d_tabPadding;       //!< The padding of the tabs relative to parent
    typedef std::vector<TabButton*
        CEGUI_VECTOR_ALLOC(TabButton*)> TabButtonVector;
    TabButtonVector d_tabButtonVector;  //!< Sorting for tabs
    float       d_firstTabOffset;   //!< The offset in pixels of the first tab
    TabPanePosition d_tabPanePos;   //!< The position of the tab pane
    float       d_btGrabPos;        //!< The position on the button tab where user grabbed
    //! Container used to track event subscriptions to added tab windows.
    std::map<Window*, Event::ScopedConnection> d_eventConnections;
    /*************************************************************************
    Abstract Implementation Functions (must be provided by derived class)
    *************************************************************************/
    /*!
    \brief
        create and return a pointer to a TabButton widget for use as a clickable tab header
    \param name
        Button name
    \return
        Pointer to a TabButton to be used for changing tabs.
    */
    //virtual TabButton*	createTabButton_impl(const String& name) const		= 0;

    /*!
    \brief
        Calculate the correct position and size of a tab button, based on the
        index it is due to be placed at.
    \param index
        The index of the tab button
    */
    void calculateTabButtonSizePosition(size_t index);

protected:
    /*************************************************************************
		Private methods
	*************************************************************************/
	void	addTabControlProperties(void);

    void    addChild_impl(Element* element);
    void    removeChild_impl(Element* element);
    
    //! \copydoc Window::getChildByNamePath_impl
    virtual NamedElement* getChildByNamePath_impl(const String& name_path) const;

    /*************************************************************************
    Event handlers
    *************************************************************************/
    bool handleContentWindowTextChanged(const EventArgs& args);
    bool handleTabButtonClicked(const EventArgs& args);
    bool handleScrollPane(const EventArgs& e);
    bool handleDraggedPane(const EventArgs& e);
    bool handleWheeledPane(const EventArgs& e);
};

template<>
class PropertyHelper<TabControl::TabPanePosition>
{
public:
    typedef TabControl::TabPanePosition return_type;
    typedef return_type safe_method_return_type;
    typedef TabControl::TabPanePosition pass_type;
    typedef String string_return_type;

    static const String& getDataTypeName()
    {
        static String type("TabPanePosition");

        return type;
    }

    static return_type fromString(const String& str)
    {
        if (str == "Bottom")
        {
            return TabControl::Bottom;
        }
        else
        {
            return TabControl::Top;
        }
    }

    static string_return_type toString(pass_type val)
    {
        if (val == TabControl::Top)
        {
            return "Top";
        }
        else if (val == TabControl::Bottom)
        {
            return "Bottom";
        }
        else
        {
            assert(false && "Invalid Tab Pane Position");
            return "Top";
        }
    }
};
} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUITabControl_h_
