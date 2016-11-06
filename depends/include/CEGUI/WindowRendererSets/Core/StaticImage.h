/***********************************************************************
    created:    Tue Jul 5 2005
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
#ifndef _FalStaticImage_h_
#define _FalStaticImage_h_

#include "CEGUI/Image.h"
#include "CEGUI/WindowRendererSets/Core/Module.h"
#include "CEGUI/WindowRendererSets/Core/Static.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
    /*!
    \brief
        StaticImage class for the FalagardBase module.

        This class requires LookNFeel to be assigned.  The LookNFeel should provide the following:

        States:
            - Enabled                     - basic rendering for enabled state.
            - Disabled                    - basic rendering for disabled state.
            - EnabledFrame                - frame rendering for enabled state
            - DisabledFrame               - frame rendering for disabled state.
            - WithFrameEnabledBackground  - backdrop rendering for enabled state with frame enabled.
            - WithFrameDisabledBackground - backdrop rendering for disabled state with frame enabled.
            - NoFrameEnabledBackground    - backdrop rendering for enabled state with frame disabled.
            - NoFrameDisabledBackground   - backdrop rendering for disabled state with frame disabled.
            - WithFrameImage              - image rendering when frame is enabled
            - NoFrameImage                - image rendering when frame is disabled (defaults to WithFrameImage if not present)
    */
    class COREWRSET_API FalagardStaticImage : public FalagardStatic
    {
    public:
        static const String TypeName;       //!< type name for this widget.

        /*!
        \brief
            Constructor
        */
        FalagardStaticImage(const String& type);

        /*!
        \brief
            Set the image for this FalagardStaticImage widget
        */
        void setImage(const Image* img);

        /*!
        \brief
            Get the image for this FalagardStaticImage widget
        */
        const Image* getImage(void) const   {return d_image;}

        void render();

    protected:
        // implementation data
        const Image* d_image;
    };

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif  // end of guard _FalStaticImage_h_
