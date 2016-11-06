/***********************************************************************
    created:    Sat Jul 2 2005
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
#ifndef _FalFrameWindow_h_
#define _FalFrameWindow_h_

#include "CEGUI/WindowRendererSets/Core/Module.h"

// Start of CEGUI namespace section
namespace CEGUI
{
    /*!
    \brief
        FrameWindow class for the FalagardBase module.

        This class requires LookNFeel to be assigned.  The LookNFeel should provide the following:

        States:
            - ActiveWithTitleWithFrame
            - InactiveWithTitleWithFrame
            - DisabledWithTitleWithFrame
            - ActiveWithTitleNoFrame
            - InactiveWithTitleNoFrame
            - DisabledWithTitleNoFrame
            - ActiveNoTitleWithFrame
            - InactiveNoTitleWithFrame
            - DisabledNoTitleWithFrame
            - ActiveNoTitleNoFrame
            - InactiveNoTitleNoFrame
            - DisabledNoTitleNoFrame

        Named Areas:
            - ClientWithTitleWithFrame
            - ClientWithTitleNoFrame
            - ClientNoTitleWithFrame
            - ClientNoTitleNoFrame
    */
    class COREWRSET_API FalagardFrameWindow : public WindowRenderer
    {
    public:
        static const String TypeName;       //!< type name for this widget.

        /*!
        \brief
            Constructor
        */
        FalagardFrameWindow(const String& type);

        void render();
        Rectf getUnclippedInnerRect(void) const;
    };

} // End of  CEGUI namespace section


#endif  // end of guard _FalFrameWindow_h_
