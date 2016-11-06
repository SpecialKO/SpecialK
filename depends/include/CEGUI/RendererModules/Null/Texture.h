/***********************************************************************
    created:    Fri Jan 15 2010
    author:     Eugene Marcotte
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
#ifndef _CEGUINullTexture_h_
#define _CEGUINullTexture_h_

#include "../../Texture.h"
#include "CEGUI/RendererModules/Null/Renderer.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
//! Implementation of the CEGUI::Texture class for no particular engine.
class NULL_GUIRENDERER_API NullTexture : public Texture
{
public:
    // implement CEGUI::Texture interface
    const String& getName() const;
    const Sizef& getSize() const;
    const Sizef& getOriginalDataSize() const;
    const Vector2f& getTexelScaling() const;
    void loadFromFile(const String& filename, const String& resourceGroup);
    void loadFromMemory(const void* buffer, const Sizef& buffer_size,
                                PixelFormat pixel_format);
    void blitFromMemory(const void* sourceData, const Rectf& area);
    void blitToMemory(void* targetData);
    bool isPixelFormatSupported(const PixelFormat fmt) const;

protected:
    // we all need a little help from out friends ;)
    friend Texture& NullRenderer::createTexture(const String&);
    friend Texture& NullRenderer::createTexture(const String&, const String&, const String&);
    friend Texture& NullRenderer::createTexture(const String&, const Sizef&);
    friend void NullRenderer::destroyTexture(Texture&);
    friend void NullRenderer::destroyTexture(const String&);

    //! standard constructor
    NullTexture(const String& name);
    //! construct texture via an image file.
    NullTexture(const String& name, const String& filename,
                const String& resourceGroup);
    //! construct texture with a specified initial size.
    NullTexture(const String& name, const Sizef& sz);

    //! destructor.
    virtual ~NullTexture();
    //! updates cached scale value used to map pixels to texture co-ords.
    void updateCachedScaleValues();

    //! Counter used to provide unique texture names.
    static uint32 d_textureNumber;
    //! Size of the texture.
    Sizef d_size;
    //! original pixel of size data loaded into texture
    Sizef d_dataSize;
    //! cached pixel to texel mapping scale values.
    Vector2f d_texelScaling;
    //! Name this texture was created with.
    const String d_name;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUINullTexture_h_
