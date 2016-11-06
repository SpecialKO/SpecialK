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
#ifndef _CEGUIRenderedStringTextComponent_h_
#define _CEGUIRenderedStringTextComponent_h_

#include "CEGUI/RenderedStringComponent.h"
#include "CEGUI/ColourRect.h"
#include "CEGUI/String.h"

// Start of CEGUI namespace section
namespace CEGUI
{
//! String component that draws an image.
class CEGUIEXPORT RenderedStringTextComponent : public RenderedStringComponent
{
public:
    //! Constructor
    RenderedStringTextComponent();
    RenderedStringTextComponent(const String& text);
    RenderedStringTextComponent(const String& text, const String& font_name);
    RenderedStringTextComponent(const String& text, const Font* font);

    //! Set the text to be rendered by this component.
    void setText(const String& text);
    //! return the text that will be rendered by this component
    const String& getText() const;
    //! set the font to use when rendering the text.
    void setFont(const Font* font);
    //! set the font to use when rendering the text.
    void setFont(const String& font_name);
    //! return the font set to be used.  If 0 the default font will be used.
    const Font* getFont() const;
    //! Set the colour values used when rendering this component.
    void setColours(const ColourRect& cr);
    //! Set the colour values used when rendering this component.
    void setColours(const Colour& c);
    //! return the ColourRect object used when drawing this component.
    const ColourRect& getColours() const;

    // implementation of abstract base interface
    void draw(const Window* ref_wnd, GeometryBuffer& buffer,
              const Vector2f& position, const ColourRect* mod_colours,
              const Rectf* clip_rect, const float vertical_space,
              const float space_extra) const;
    Sizef getPixelSize(const Window* ref_wnd) const;
    bool canSplit() const;
    RenderedStringTextComponent* split(const Window* ref_wnd,
                                       float split_point, bool first_component);
    RenderedStringTextComponent* clone() const;
    size_t getSpaceCount() const;
    void setSelection(const Window* ref_wnd,
                      const float start, const float end);

protected:
    const Font* getEffectiveFont(const Window* window) const;

    static size_t getNextTokenLength(const String& text, size_t start_idx);

    //! pointer to the image drawn by the component.
    String d_text;
    //! Font to use for text rendering, 0 for system default.
    const Font* d_font;
    //! ColourRect object describing the colours to use when rendering.
    ColourRect d_colours;
    //! last set selection
    size_t d_selectionStart, d_selectionLength;
};
    
} // End of  CEGUI namespace section

#endif // end of guard _CEGUIRenderedStringTextComponent_h_
