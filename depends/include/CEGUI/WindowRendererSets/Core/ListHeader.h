/***********************************************************************
    created:    Wed Jul 6 2005
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
#ifndef _FalListHeader_h_
#define _FalListHeader_h_

#include "CEGUI/WindowRendererSets/Core/Module.h"
#include "CEGUI/widgets/ListHeader.h"
#include "CEGUI/falagard/WidgetLookFeel.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
    /*!
    \brief
        ListHeader class for the FalagardBase module.

        This class requires LookNFeel to be assigned.  The LookNFeel should provide the following:

        Property Initialisers:
            SegmentWidgetType   - type of widget to create for segments.

        Imagery States:
            - Enabled           - basic rendering for enabled state.
            - Disabled          - basic rendering for disabled state.
    */
    class COREWRSET_API FalagardListHeader : public ListHeaderWindowRenderer
    {
    public:
        static const String TypeName;       //!< type name for this widget.

        /*!
        \brief
            Constructor
        */
        FalagardListHeader(const String& type);

        const String& getSegmentWidgetType() const;
        void setSegmentWidgetType(const String& type);

        // overridden from ListHeaderWindowRenderer base class.
        void render();
        ListHeaderSegment* createNewSegment(const String& name) const;
        void destroyListSegment(ListHeaderSegment* segment) const;

protected:

        // data fields
        String  d_segmentWidgetType;
    };

} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif  // end of guard _FalListHeader_h_
