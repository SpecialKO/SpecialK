/************************************************************************
    created:    Sat Oct 29 2005
    author:     Tomas Lindquist Olsen
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2009 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIScrolledItemListBase_h_
#define _CEGUIScrolledItemListBase_h_

#include "./ItemListBase.h"
#include "./Scrollbar.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif
// begin CEGUI namespace
namespace CEGUI
{

/*!
\brief
    ScrolledItemListBase window class
*/
class CEGUIEXPORT ScrolledItemListBase : public ItemListBase
{
public:
    static const String EventNamespace; //!< Namespace for global events

    /************************************************************************
        Constants
    *************************************************************************/
    static const String VertScrollbarName; //!< Name for vertical scrollbar component
    static const String HorzScrollbarName; //!< Name for horizontal scrollbar component
    static const String ContentPaneName;   //!< Name for the content pane component

    /** Event fired when the vertical scroll bar mode changes.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ScrolledItemListBase whose vertical
     * scroll bar mode has been changed.
     */
    static const String EventVertScrollbarModeChanged;
    /** Event fired when the horizontal scroll bar mode change.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ScrolledItemListBase whose horizontal
     * scroll bar mode has been changed.
     */
    static const String EventHorzScrollbarModeChanged;

    /************************************************************************
        Accessors
    *************************************************************************/
    /*!
    \brief
        Returns whether the vertical scrollbar is being forced visible. Despite content size.
    */
    bool isVertScrollbarAlwaysShown(void) const {return d_forceVScroll;}

    /*!
    \brief
        Returns whether the horizontal scrollbar is being forced visible. Despite content size.
    */
    bool isHorzScrollbarAlwaysShown(void) const {return d_forceHScroll;}

    /*!
    \brief
        Get the vertical scrollbar component attached to this window.
    */
    Scrollbar* getVertScrollbar() const;

    /*!
    \brief
        Get the horizontal scrollbar component attached to this window.
    */
    Scrollbar* getHorzScrollbar() const;

    /************************************************************************
        Manipulators
    *************************************************************************/
    /*!
    \brief
        Sets whether the vertical scrollbar should be forced visible. Despite content size.
    */
    void setShowVertScrollbar(bool mode);

    /*!
    \brief
        Sets whether the horizontal scrollbar should be forced visible. Despite content size.
    */
    void setShowHorzScrollbar(bool mode);

    /*!
    \brief
        Scroll the vertical list position if needed to ensure that the ItemEntry
        \a item is, if possible,  fully visible witin the ScrolledItemListBase
        viewable area.

    \param item
        const reference to an ItemEntry attached to this ScrolledItemListBase
        that should be made visible in the view area.

    \return
        Nothing.
    */
    void ensureItemIsVisibleVert(const ItemEntry& item);

    /*!
    \brief
        Scroll the horizontal list position if needed to ensure that the
        ItemEntry \a item is, if possible, fully visible witin the
        ScrolledItemListBase viewable area.

    \param item
        const reference to an ItemEntry attached to this ScrolledItemListBase
        that should be made visible in the view area.

    \return
        Nothing.
    */
    void ensureItemIsVisibleHorz(const ItemEntry& item);

    /************************************************************************
        Object Construction and Destruction
    *************************************************************************/
    /*!
    \brief
        Constructor for the ScrolledItemListBase base class constructor
    */
    ScrolledItemListBase(const String& type, const String& name);

    /*!
    \brief
        Destructor for the ScrolledItemListBase base class.
     */
    virtual ~ScrolledItemListBase(void);

    // overridden from ItemListBase
    virtual void initialiseComponents(void);

protected:
    /************************************************************************
        Implementation functions
    ************************************************************************/
    /*!
    \brief
        Configure scrollbars
    */
    void configureScrollbars(const Sizef& doc_size);

    /************************************************************************
        New event handlers
    ************************************************************************/
    virtual void onVertScrollbarModeChanged(WindowEventArgs& e);
    virtual void onHorzScrollbarModeChanged(WindowEventArgs& e);

    /************************************************************************
        Overridden event handlers
    ************************************************************************/
    virtual void onMouseWheel(MouseEventArgs& e);

    /************************************************************************
        Event subscribers
    ************************************************************************/
    bool handle_VScroll(const EventArgs& e);
    bool handle_HScroll(const EventArgs& e);

    /************************************************************************
        Implementation data
    ************************************************************************/
    bool d_forceVScroll;
    bool d_forceHScroll;

private:
    void addScrolledItemListBaseProperties(void);
};

} // end CEGUI namespace

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif // end of guard _CEGUIScrolledItemListBase_h_
