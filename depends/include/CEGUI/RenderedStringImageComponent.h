/***********************************************************************
    created:    24/05/2009
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
#ifndef _CEGUIRenderedStringImageComponent_h_
#define _CEGUIRenderedStringImageComponent_h_

#include "CEGUI/RenderedStringComponent.h"
#include "CEGUI/ColourRect.h"
#include "CEGUI/String.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
//! String component that draws an image.
class CEGUIEXPORT RenderedStringImageComponent : public RenderedStringComponent
{
public:
    //! Constructor
    RenderedStringImageComponent();
    RenderedStringImageComponent(const String& name);
    RenderedStringImageComponent(const Image* image);

    //! Set the image to be drawn by this component.
    void setImage(const String& name);
    //! Set the image to be drawn by this component.
    void setImage(const Image* image);
    //! return the current set image that will be drawn by this component
    const Image* getImage() const;
    //! Set the colour values used when rendering this component.
    void setColours(const ColourRect& cr);
    //! Set the colour values used when rendering this component.
    void setColours(const Colour& c);
    //! return the ColourRect object used when drawing this component.
    const ColourRect& getColours() const;
    //! set the size for rendering the image (0s mean 'normal' size)
    void setSize(const Sizef& sz);
    //! return the size for rendering the image (0s mean 'normal' size)
    const Sizef& getSize() const;

    // implementation of abstract base interface
    void draw(const Window* ref_wnd, GeometryBuffer& buffer,
              const Vector2f& position, const ColourRect* mod_colours,
              const Rectf* clip_rect, const float vertical_space,
              const float space_extra) const;
    Sizef getPixelSize(const Window* ref_wnd) const;
    bool canSplit() const;
    RenderedStringImageComponent* split(const Window* ref_wnd,
                                        float split_point,
                                        bool first_component);
    RenderedStringImageComponent* clone() const;
    size_t getSpaceCount() const;
    void setSelection(const Window* ref_wnd,
                      const float start, const float end);
protected:
    //! pointer to the image drawn by the component.
    const Image* d_image;
    //! ColourRect object describing the colours to use when rendering.
    ColourRect d_colours;
    //! target size to render the image at (0s mean natural size)
    Sizef d_size;
    // whether the image is marked as selected.
    bool d_selected;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif // end of guard _CEGUIRenderedStringImageComponent_h_
