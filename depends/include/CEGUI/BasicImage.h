/***********************************************************************
    created:    Wed Feb 16 2011
    author:     Paul D Turner <paul@cegui.org.uk>
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2011 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIBasicImage_h_
#define _CEGUIBasicImage_h_

#include "CEGUI/Image.h"
#include "CEGUI/String.h"
#include "CEGUI/Rect.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
class CEGUIEXPORT BasicImage : public Image
{
public:
    BasicImage(const String& name);
    BasicImage(const XMLAttributes& attributes);

    BasicImage(const String& name, Texture* texture,
               const Rectf& tex_area, const Vector2f& offset,
               const AutoScaledMode autoscaled, const Sizef& native_res);

    void setTexture(Texture* texture);
    void setArea(const Rectf& pixel_area);
    void setOffset(const Vector2f& pixel_offset);
    void setAutoScaled(const AutoScaledMode autoscaled);
    void setNativeResolution(const Sizef& native_res);

    // Implement CEGUI::Image interface
    const String& getName() const;
    const Sizef& getRenderedSize() const;
    const Vector2f& getRenderedOffset() const;
    void render(GeometryBuffer& buffer,
                const Rectf& dest_area,
                const Rectf* clip_area,
                const ColourRect& colours) const;

    //! Notifies the class that the display size of the renderer has changed so that
    // the window can adapt to the new display size accordingly
    void notifyDisplaySizeChanged(const Sizef& renderer_display_size);

protected:
    //! Updates the scaled size and offset values according to the new display size of the renderer 
    void updateScaledSizeAndOffset(const Sizef& renderer_display_size);
    //! Updates only the scaled size values according to the new display size of the renderer 
    void updateScaledSize(const Sizef& renderer_display_size);
    //! Updates only the scaled offset values according to the new display size of the renderer 
    void updateScaledOffset(const Sizef& renderer_display_size);

    //! name used when the BasicImage was created.
    String d_name;
    //! Texture used by this image.
    Texture* d_texture;
    //! Actual pixel size.
    Sizef d_pixelSize;
    //! Rect defining texture co-ords for this image.
    Rectf d_area;
    //! Defined pixel offset
    Vector2f d_pixelOffset;
    //! Whether image is auto-scaled or not and how.
    AutoScaledMode d_autoScaled;
    //! Native resolution used for autoscaling.
    Sizef d_nativeResolution;
    //! Size after having autoscaling applied.
    Sizef d_scaledSize;
    //! Offset after having autoscaling applied.
    Vector2f d_scaledOffset;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif  // end of guard _CEGUIBasicImage_h_

