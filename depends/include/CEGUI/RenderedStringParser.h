/***********************************************************************
    created:    28/05/2009
    author:     Paul Turner
 *************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2009 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIRenderedStringParser_h_
#define _CEGUIRenderedStringParser_h_

#include "CEGUI/RenderedString.h"
#include "CEGUI/String.h"

// Start of CEGUI namespace section
namespace CEGUI
{
//! Specifies interface for classes that parse text into RenderedString objects.
class CEGUIEXPORT RenderedStringParser :
    public AllocatedObject<RenderedStringParser>
{
public:
    /*!
    \brief
        parse a text string and return a RenderedString representation.

    \param input_string
        String object holding the text that is to be parsed.

    \param initial_font
        Pointer to the initial font to be used for text (0 for system default).

    \param initial_colours
        Pointer to the initial colours to be used (0 for default).

    \return
        RenderedString object holding the result of the parse operation.
    */
    virtual RenderedString parse(const String& input_string,
                                 const Font* initial_font,
                                 const ColourRect* initial_colours) = 0;
};

} // End of  CEGUI namespace section

#endif // end of guard _CEGUIRenderedStringParser_h_
