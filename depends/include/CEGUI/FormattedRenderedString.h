/***********************************************************************
    created:    25/05/2009
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
#ifndef _CEGUIFormattedRenderedString_h_
#define _CEGUIFormattedRenderedString_h_

#include "CEGUI/RenderedString.h"

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Root of a class hierarchy that wrap RenderedString objects and render them
    with additional formatting.
*/
class CEGUIEXPORT FormattedRenderedString :
    public AllocatedObject<FormattedRenderedString>
{
public:
    //! Destructor.
    virtual ~FormattedRenderedString();

    virtual void format(const Window* ref_wnd, const Sizef& area_size) = 0;
    virtual void draw(const Window* ref_wnd, GeometryBuffer& buffer,
                      const Vector2f& position, const ColourRect* mod_colours,
                      const Rectf* clip_rect) const = 0;
    virtual size_t getFormattedLineCount() const = 0;
    virtual float getHorizontalExtent(const Window* ref_wnd) const = 0;
    virtual float getVerticalExtent(const Window* ref_wnd) const = 0;

    //! set the RenderedString.
    void setRenderedString(const RenderedString& string);

    const RenderedString& getRenderedString() const;

protected:
    //! Constructor.
    FormattedRenderedString(const RenderedString& string);

    //! RenderedString that we handle formatting for.
    const RenderedString* d_renderedString;
};

} // End of  CEGUI namespace section

#endif // end of guard _CEGUIFormattedRenderedString_h_
