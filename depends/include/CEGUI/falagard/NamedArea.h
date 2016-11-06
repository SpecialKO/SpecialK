/***********************************************************************
    created:    Sun Jun 26 2005
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
#ifndef _CEGUIFalNamedArea_h_
#define _CEGUIFalNamedArea_h_

#include "./Dimensions.h"

// Start of CEGUI namespace section
namespace CEGUI
{
    /*!
    \brief
        NamedArea defines an area for a component which may later be obtained
        and referenced by a name unique to the WidgetLook holding the NamedArea.
    */
    class CEGUIEXPORT NamedArea :
        public AllocatedObject<NamedArea>
    {
    public:
        NamedArea() {}
        NamedArea(const String& name);

        /*!
        \brief
            Return the name of this NamedArea.

        \return
            String object holding the name of this NamedArea.
        */
        const String& getName() const;

        /*!
        \brief
            set the name for this NamedArea.

        \param area
            String object holding the name of this NamedArea.

        \return
            Nothing.
        */
        void setName(const String& name);

        /*!
        \brief
            Return the ComponentArea of this NamedArea

        \return
            ComponentArea object describing the NamedArea's current target area.
        */
        const ComponentArea& getArea() const;

        /*!
        \brief
            Set the Area for this NamedArea.

        \param area
            ComponentArea object describing a new target area for the NamedArea..

        \return
            Nothing.
        */
        void setArea(const ComponentArea& area);

        /*!
        \brief
            Writes an xml representation of this NamedArea to \a out_stream.

        \param out_stream
            Stream where xml data should be output.

        \return
            Nothing.
        */
        void writeXMLToStream(XMLSerializer& xml_stream) const;

        //! perform any processing required due to the given font having changed.
        bool handleFontRenderSizeChange(Window& window, const Font* font) const;

    private:
        String d_name;
        ComponentArea d_area;
    };

} // End of  CEGUI namespace section


#endif  // end of guard _CEGUIFalNamedArea_h_
