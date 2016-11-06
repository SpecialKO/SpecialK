/***********************************************************************
    created:    Mon Jul 4 2005
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
#ifndef _FalListbox_h_
#define _FalListbox_h_

#include "CEGUI/WindowRendererSets/Core/Module.h"
#include "CEGUI/widgets/Listbox.h"

// Start of CEGUI namespace section
namespace CEGUI
{
    /*!
    \brief
        Listbox class for the FalagardBase module.

        This class requires LookNFeel to be assigned.  The LookNFeel should provide the following:

        States:
            - Enabled
            - Disabled

        Named Areas:
            - ItemRenderingArea
            - ItemRenderingAreaHScroll
            - ItemRenderingAreaVScroll
            - ItemRenderingAreaHVScroll
            - - OR -
            - ItemRenderArea
            - ItemRenderAreaHScroll
            - ItemRenderAreaVScroll
            - ItemRenderAreaHVScroll

        Child Widgets:
            Scrollbar based widget with name suffix "__auto_vscrollbar__"
            Scrollbar based widget with name suffix "__auto_hscrollbar__"
    */
    class COREWRSET_API FalagardListbox : public ListboxWindowRenderer
    {
    public:
        static const String TypeName;       //!< type name for this widget.

        /*!
        \brief
            Constructor
        */
        FalagardListbox(const String& type);

        /*!
        \brief
            Perform caching of the widget control frame and other 'static' areas.  This
            method should not render the actual items.  Note that the items are typically
            rendered to layer 3, other layers can be used for rendering imagery behind and
            infront of the items.

        \return
            Nothing.
        */
        void cacheListboxBaseImagery();

        // overriden from ListboxWindowRenderer
        void render();

        Rectf getListRenderArea(void) const;

        void resizeListToContent(bool fit_width,
                                 bool fit_height) const;

        bool handleFontRenderSizeChange(const Font* const font);

    protected:
        Rectf getItemRenderingArea(bool hscroll, bool vscroll) const;
    };
} // End of  CEGUI namespace section


#endif  // end of guard _FalListbox_h_
