/***********************************************************************
    created:    01/8/2010
    author:     Martin Preisler

    purpose:    Interface to a vertical layout container
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2010 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIGridLayoutContainer_h_
#define _CEGUIGridLayoutContainer_h_

#include "./LayoutContainer.h"
#include "../WindowFactory.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    A Layout Container window layouting it's children into a grid
*/
class CEGUIEXPORT GridLayoutContainer : public LayoutContainer
{
public:
    /**
     * enumerates auto positioning methods for the grid - these allow you to
     * fill the grid without specifying gridX and gridY positions for each
     * addChild.
     */
    enum AutoPositioning
    {
        //! no auto positioning!
        AP_Disabled,
        /**
         * Left to right positioning:
         * - 1 2 3
         * - 4 5 6
         */
        AP_LeftToRight,
        /**
         * Top to bottom positioning
         * - 1 3 5
         * - 2 4 6
         */
        AP_TopToBottom
    };

    /*************************************************************************
        Constants
    *************************************************************************/
    //! The unique typename of this widget
    static const String WidgetTypeName;

    /*************************************************************************
        Child Widget name suffix constants
    *************************************************************************/
    //! Widget name for dummies.
    static const String DummyName;

    /*************************************************************************
        Event name constants
    *************************************************************************/
    //! Namespace for global events
    static const String EventNamespace;

    //! fired when child windows get rearranged
    static const String EventChildOrderChanged;

    /*************************************************************************
        Construction and Destruction
    *************************************************************************/
    /*!
    \brief
        Constructor for GUISheet windows.
    */
    GridLayoutContainer(const String& type, const String& name);

    /*!
    \brief
        Destructor for GUISheet windows.
    */
    virtual ~GridLayoutContainer(void);

    /*!
    \brief
        Sets grid's dimensions.
    */
    void setGridDimensions(size_t width, size_t height);
    /*!
    \brief
        Sets grid's dimensions.
    */
    void setGrid(const Sizef &size);

    /*!
    \brief
        Retrieves grid width, the amount of cells in one row
    */
    size_t getGridWidth() const;

    /*!
    \brief
        Retrieves grid height, the amount of rows in the grid
    */
    size_t getGridHeight() const;

    /*!
    \brief
        Retrieves grid width, the amount of cells in one row
    */
    Sizef getGrid() const;


    /*!
    \brief
        Sets new auto positioning method.

    \par
        The newly set auto positioning sequence will start over!
        Use setAutoPositioningIdx to set it's starting point
    */
    void setAutoPositioning(AutoPositioning positioning);

    /*!
    \brief
        Retrieves current auto positioning method.
    */
    AutoPositioning getAutoPositioning() const;

    /*!
    \brief
        Sets the next auto positioning "sequence position", this will be used
        next time when addChild is called.
    */
    void setNextAutoPositioningIdx(size_t idx);

    /*!
    \brief
        Retrieves auto positioning "sequence position", this will be used next
        time when addChild is called.
    */
    size_t getNextAutoPositioningIdx() const;

    /*!
    \brief
        Skips given number of cells in the auto positioning sequence
    */
    void autoPositioningSkipCells(size_t cells);

    /*!
    \brief
        Add the specified Window to specified grid position as a child of
        this Grid Layout Container.  If the Window \a window is already
        attached to a Window, it is detached before being added to this Window.

    \par
        If something is already in given grid cell, it gets removed!

    \par
        This disabled auto positioning from further usage! You need to call
        setAutoPositioning(..) to set it back to your desired value and use
        setAutoPositioningIdx(..) to set it's starting point back

    \see
        Window::addChild
    */
    void addChildToPosition(Window* window, size_t gridX, size_t gridY);

    /*!
    \brief
        Retrieves child window that is currently at given grid position
    */
    Window* getChildAtPosition(size_t gridX, size_t gridY);

    /*!
    \brief
        Removes the child window that is currently at given grid position

    \see
        Window::removeChild
    */
    void removeChildFromPosition(size_t gridX, size_t gridY);

    /*!
    \brief
        Swaps positions of 2 windows given by their index

    \par
        For advanced users only!
    */
    virtual void swapChildPositions(size_t wnd1, size_t wnd2);

    /*!
    \brief
        Swaps positions of 2 windows given by grid positions
    */
    void swapChildPositions(size_t gridX1, size_t gridY1,
                                  size_t gridX2, size_t gridY2);

    /*!
    \brief
        Swaps positions of given windows
    */
    void swapChildren(Window* wnd1, Window* wnd2);

    /*!
    \brief
        Swaps positions of given windows
    */
    void swapChildren(Window* wnd1, const String& wnd2);

    /*!
    \brief
        Swaps positions of given windows
    */
    void swapChildren(const String& wnd1, Window* wnd2);

    /*!
    \brief
        Moves given child window to given grid position
    */
    void moveChildToPosition(Window* wnd, size_t gridX, size_t gridY);

    /*!
    \brief
        Moves named child window to given grid position
    */
    void moveChildToPosition(const String& wnd,
                                   size_t gridX, size_t gridY);

    //! @copydoc LayoutContainer::layout
    virtual void layout();

protected:
    /*!
    \brief
        Handler called when children of this window gets rearranged in any way

    \param e
        WindowEventArgs object whose 'window' field is set this layout
        container.
    */
    virtual void onChildOrderChanged(WindowEventArgs& e);

    //! converts from grid cell position to idx
    size_t mapFromGridToIdx(size_t gridX, size_t gridY,
                            size_t gridWidth, size_t gridHeight) const;
    //! converts from idx to grid cell position
    void mapFromIdxToGrid(size_t idx, size_t& gridX, size_t& gridY,
                          size_t gridWidth, size_t gridHeight) const;

    /** calculates grid cell offset
     * (relative to position of this layout container)
     */
    UVector2 getGridCellOffset(const std::vector<UDim>& colSizes,
                               const std::vector<UDim>& rowSizes,
                               size_t gridX, size_t gridY) const;
    //! calculates total grid size
    USize getGridSize(const std::vector<UDim>& colSizes,
                         const std::vector<UDim>& rowSizes) const;

    //! translates auto positioning index to absolute grid index
    size_t translateAPToGridIdx(size_t APIdx) const;

    //! stores grid width - amount of columns
    size_t d_gridWidth;
    //! stores grid height - amount of rows
    size_t d_gridHeight;

    //! stores currently used auto positioning method
    AutoPositioning d_autoPositioning;
    /** stores next auto positioning index (will be used for next
     * added window if d_autoPositioning != AP_Disabled)
     */
    size_t d_nextAutoPositioningIdx;

    /** stores next used grid X position
     * (only used if d_autoPositioning == AP_Disabled)
     */
    size_t d_nextGridX;
    /** stores next used grid Y position
     * (only used if d_autoPositioning == AP_Disabled)
     */
    size_t d_nextGridY;

    /** stores next used dummy suffix index
     * (used to generate unique dummy names)
     */
    size_t d_nextDummyIdx;

    //! creates a dummy window
    Window* createDummy();
    //! checks whether given window is a dummy
    bool isDummy(Window* wnd) const;

    /// @copydoc Window::addChild_impl
    virtual void addChild_impl(Element* element);
    /// @copydoc Window::removeChild_impl
    virtual void removeChild_impl(Element* element);

private:
    void addGridLayoutContainerProperties(void);
};

template<>
class PropertyHelper<GridLayoutContainer::AutoPositioning>
{
public:
    typedef GridLayoutContainer::AutoPositioning return_type;
    typedef return_type safe_method_return_type;
    typedef GridLayoutContainer::AutoPositioning pass_type;
    typedef String string_return_type;

    static const String& getDataTypeName()
    {
        static String type("AutoPositioning");

        return type;
    }

    static return_type fromString(const String& str)
    {
        if (str == "Disabled")
        {
            return GridLayoutContainer::AP_Disabled;
        }
        else if (str == "Top to Bottom")
        {
            return GridLayoutContainer::AP_TopToBottom;
        }
        else
        {
            return GridLayoutContainer::AP_LeftToRight;
        }
    }

    static string_return_type toString(pass_type val)
    {
        if (val == GridLayoutContainer::AP_LeftToRight)
        {
            return "Left to Right";
        }
        else if (val == GridLayoutContainer::AP_Disabled)
        {
            return "Disabled";
        }
        else if (val == GridLayoutContainer::AP_TopToBottom)
        {
            return "Top to Bottom";
        }
        else
        {
            assert(false && "Invalid Auto Positioning");
            return "Left to Right";
        }
    }
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIGridLayoutContainer_h_
