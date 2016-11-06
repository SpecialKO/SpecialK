/***********************************************************************
    created:    Sat Sep 17 2005
    author:     Tomas Lindquist Olsen (based on code by Paul D Turner)
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
#ifndef _FalStatic_h_
#define _FalStatic_h_

#include "CEGUI/WindowRendererSets/Core/Module.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
    /*!
    \brief
        Static class for the FalagardBase module.

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
    */
    class COREWRSET_API FalagardStatic : public WindowRenderer
    {
    public:
        static const String TypeName;     //! type name for this widget.

        /*!
        \brief
            Constructor
        */
        FalagardStatic(const String& type);

        /*!
        \brief
            Return whether the frame for this static widget is enabled or disabled.

        \return
            true if the frame is enabled and will be rendered.  false is the frame is disabled and will not be rendered.
        */
        bool    isFrameEnabled(void) const        {return d_frameEnabled;}

        /*!
        \brief
            Return whether the background for this static widget is enabled to disabled.

        \return
            true if the background is enabled and will be rendered.  false if the background is disabled and will not be rendered.
        */
        bool    isBackgroundEnabled(void) const        {return d_backgroundEnabled;}

        /*!
        \brief
            Enable or disable rendering of the frame for this static widget.

        \param setting
            true to enable rendering of a frame.  false to disable rendering of a frame.
        */
        void    setFrameEnabled(bool setting);

        /*!
        \brief
            Enable or disable rendering of the background for this static widget.

        \param setting
            true to enable rendering of the background.  false to disable rendering of the background.
        */
        void    setBackgroundEnabled(bool setting);

        virtual void render();

    protected:
        // implementation data
        bool d_frameEnabled;        //!< True when the frame is enabled.
        bool d_backgroundEnabled;   //!< true when the background is enabled.
    };

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif  // end of guard _FalStatic_h_
