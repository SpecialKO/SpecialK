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
#ifndef _FalProgressBar_h_
#define _FalProgressBar_h_

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
        ProgressBar class for the FalagardBase module.

        This class requires LookNFeel to be assigned.  The LookNFeel should provide the following:

        States:
            - Enabled
            - Disabled
            - EnabledProgress
            - DisabledProgress

        Named Areas:
            - ProgressArea

        Property initialiser definitions:
            - VerticalProgress - boolean property.
              Determines whether the progress widget is horizontal or vertical.
              Default is horizontal.  Optional.

            - ReversedProgress - boolean property.
              Determines whether the progress grows in the opposite direction to
              what is considered 'usual'.  Set to "true" to have progress grow
              towards the left or bottom of the progress area.  Optional.
    */
    class COREWRSET_API FalagardProgressBar : public WindowRenderer
    {
    public:
        static const String TypeName;     //! type name for this widget.

        /*!
        \brief
            Constructor
        */
        FalagardProgressBar(const String& type);

        bool isVertical() const;
        bool isReversed() const;

        void setVertical(bool setting);
        void setReversed(bool setting);

        void render();

    protected:
        // settings to make this class universal.
        bool d_vertical;    //!< True if progress bar operates on the vertical plane.
        bool d_reversed;    //!< True if progress grows in the opposite direction to usual (i.e. to the left / downwards).
    };

} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif  // end of guard _FalProgressBar_h_
