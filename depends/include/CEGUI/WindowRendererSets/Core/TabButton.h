/***********************************************************************
    created:    Fri Jul 8 2005
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
#ifndef _FalTabButton_h_
#define _FalTabButton_h_

#include "CEGUI/WindowRendererSets/Core/Module.h"

// Start of CEGUI namespace section
namespace CEGUI
{
    /*!
    \brief
        TabButton class for the FalagardBase module.

        This class requires LookNFeel to be assigned.  The LookNFeel should provide the following:

        States (missing states will default to 'Normal'):
            - Normal    - Rendering for when the tab button is neither selected nor has the mouse hovering over it.
            - Hover     - Rendering for then the tab button has the mouse hovering over it.
            - Selected  - Rendering for when the tab button is the button for the selected tab.
            - Disabled  - Rendering for when the tab button is disabled.
    */
    class COREWRSET_API FalagardTabButton : public WindowRenderer
    {
    public:
        static const String TypeName;     //! type name for this widget.

        /*!
        \brief
            Constructor
        */
        FalagardTabButton(const String& type);

        void render();
    };

} // End of  CEGUI namespace section


#endif  // end of guard _FalTabButton_h_
