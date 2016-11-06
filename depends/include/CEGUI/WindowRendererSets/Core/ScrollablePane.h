/***********************************************************************
    created:    Thu Jul 7 2005
    author:     Paul D Turner <paul@cegui.org.uk>
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
#ifndef _FalScrollablePane_h_
#define _FalScrollablePane_h_

#include "CEGUI/WindowRendererSets/Core/Module.h"
#include "CEGUI/widgets/ScrollablePane.h"

// Start of CEGUI namespace section
namespace CEGUI
{
    /*!
    \brief
        ScrollablePane class for the FalagardBase module.

        This class requires LookNFeel to be assigned.  The LookNFeel should provide the following:

        States:
            - Enabled
            - Disabled

        Named Areas:
            - ViewableArea         - area where content will be drawn when no scrollbars are visible.
            - ViewableAreaHScroll  - area where content will be drawn when the horizontal scrollbar is visible.
            - ViewableAreaVScroll  - area where content will be drawn when the vertical scrollbar is visible.
            - ViewableAreaHVScroll - area where content will be drawn when both scrollbars are visible.

        Child Widgets:
            Scrollbar based widget with name suffix "__auto_vscrollbar__"
            Scrollbar based widget with name suffix "__auto_hscrollbar__"

    */
    class COREWRSET_API FalagardScrollablePane : public ScrollablePaneWindowRenderer
    {
    public:
        static const String TypeName;     //! type name for this widget.

        /*!
        \brief
            Constructor
        */
        FalagardScrollablePane(const String& type);

        void render();
        Rectf getViewableArea(void) const;

        // overridden from WindowRenderer base class.
        Rectf getUnclippedInnerRect() const;

    protected:
        // overridden from WindowRenderer base class.
        void onLookNFeelAssigned();
        void onLookNFeelUnassigned();

        //! flag whether target window has looknfeel assigned yet.
        bool d_widgetLookAssigned;

    };

} // End of  CEGUI namespace section

#endif
