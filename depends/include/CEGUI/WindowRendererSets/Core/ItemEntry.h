/***********************************************************************
    created:    Thu Sep 22 2005
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
#ifndef _FalItemEntry_h_
#define _FalItemEntry_h_

#include "CEGUI/WindowRendererSets/Core/Module.h"
#include "CEGUI/widgets/ItemEntry.h"

// Start of CEGUI namespace section
namespace CEGUI
{
    /*!
    \brief
        ItemEntry class for the FalagardBase module.

        This class requires LookNFeel to be assigned.  The LookNFeel should provide the following:

        States:
            - Enabled           - basic rendering for enabled state.
            - Disabled          - basic rendering for disabled state.

        Optional states:
            - SelectedEnabled   - basic rendering for enabled and selected state.
            - SelectedDisabled  - basic rendering for disabled and selected state.

        You only need to provide the 'Selected' states if the item will be selectable.
        If if the item is selected (which implies that it is selectable) only the SelectedEnabled
        state will be rendered.

        Named areas:
            - ContentSize
    */
    class COREWRSET_API FalagardItemEntry : public ItemEntryWindowRenderer
    {
    public:
        static const String TypeName;       //!< type name for this widget.

        /*!
        \brief
            Constructor
        */
        FalagardItemEntry(const String& type);

        void render();
        Sizef getItemPixelSize() const;
    };

} // End of  CEGUI namespace section


#endif  // end of guard _FalItemEntry_h_
