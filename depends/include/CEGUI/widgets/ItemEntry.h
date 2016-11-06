/***********************************************************************
	created:	31/3/2005
	author:		Tomas Lindquist Olsen (based on code by Paul D Turner)
	
	purpose:	Interface to base class for ItemEntry widget
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
#ifndef _CEGUIItemEntry_h_
#define _CEGUIItemEntry_h_

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
class CEGUIEXPORT ItemEntryWindowRenderer : public WindowRenderer
{
public:
    /*!
    \brief
        Constructor
    */
    ItemEntryWindowRenderer(const String& name);

    /*!
    \brief
        Return the "optimal" size for the item

    \return
        Size describing the size in pixel that this ItemEntry's content requires
        for non-clipped rendering
    */
    virtual Sizef getItemPixelSize(void) const = 0;
};

/*!
\brief
	Base class for item type widgets.

\todo
    Fire events on selection / deselection.
    (Maybe selectable mode changed as well?)
*/
class CEGUIEXPORT ItemEntry : public Window
{
public:
    /*************************************************************************
        Constants
    *************************************************************************/
    static const String WidgetTypeName;             //!< Window factory name
    /** Event fired when the item's selection state changes.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ItemEntry whose selection state has
     * changed.
     */
    static const String EventSelectionChanged;

	/*************************************************************************
		Accessors
	*************************************************************************/
	/*!
	\brief
		Return the "optimal" size for the item
	
	\return
		Size describing the size in pixel that this ItemEntry's content requires
		for non-clipped rendering
	*/
	Sizef getItemPixelSize(void) const;

    /*!
    \brief
        Returns a pointer to the owner ItemListBase.
        0 if there is none.
    */
    ItemListBase* getOwnerList(void) const  {return d_ownerList;}

    /*!
    \brief
        Returns whether this item is selected or not.
    */
    bool isSelected(void) const             {return d_selected;}

    /*!
    \brief
        Returns whether this item is selectable or not.
    */
    bool isSelectable(void) const           {return d_selectable;}

    /*************************************************************************
		Set methods
	*************************************************************************/
    /*!
    \brief
        Sets the selection state of this item (on/off).
        If this item is not selectable this function does nothing.

    \param setting
        'true' to select the item.
        'false' to deselect the item.
    */
    void setSelected(bool setting)  {setSelected_impl(setting, true);}
    
    /*!
    \brief
        Selects the item.
    */
    void select(void)               {setSelected_impl(true, true);}

    /*!
    \brief
        Deselects the item.
    */
    void deselect(void)             {setSelected_impl(false, true);}

    /*!
    \brief
        Set the selection state for this ListItem.
        Internal version. Should NOT be used by client code.
    */
    void setSelected_impl(bool state, bool notify);

    /*!
    \brief
        Sets whether this item will be selectable.

    \param setting
        'true' to allow this item to be selected.
        'false' to disallow this item from ever being selected.

    \note
        If the item is currently selectable and selected, calling this
        function with \a setting as 'false' will first deselect the item
        and then disable selectability.
    */
    void setSelectable(bool setting);

	/*************************************************************************
		Construction and Destruction
	*************************************************************************/
	/*!
	\brief
		Constructor for ItemEntry objects
	*/
	ItemEntry(const String& type, const String& name);

	/*!
	\brief
		Destructor for ItemEntry objects
	*/
	virtual ~ItemEntry(void) {}

protected:
    /*************************************************************************
        Abstract Implementation Functions
    *************************************************************************/
    /*!
    \brief
        Return the "optimal" size for the item

    \return
        Size describing the size in pixel that this ItemEntry's content requires
        for non-clipped rendering
    */
    //virtual Size getItemPixelSize_impl(void) const = 0;

	/*************************************************************************
		Implementation Functions
	*************************************************************************/
    // validate window renderer
    virtual bool validateWindowRenderer(const WindowRenderer* renderer) const;

    /*************************************************************************
        New Event Handlers
    *************************************************************************/
    /*!
    \brief
        Handles selection state changes.
    */
    virtual void onSelectionChanged(WindowEventArgs& e);

    /*************************************************************************
		Overridden Event Handlers
	*************************************************************************/
	virtual void onMouseClicked(MouseEventArgs& e);

    /*************************************************************************
        Implementation Data
    *************************************************************************/
    
    //!< pointer to the owner ItemListBase. 0 if there is none.
    ItemListBase* d_ownerList;
    
    //!< 'true' when the item is in the selected state, 'false' if not.
    bool d_selected;

    //!< 'true' when the item is selectable.
    bool d_selectable;
    
    // make the ItemListBase a friend
    friend class ItemListBase;

private:
    
    void addItemEntryProperties(void);
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUIItemEntry_h_
