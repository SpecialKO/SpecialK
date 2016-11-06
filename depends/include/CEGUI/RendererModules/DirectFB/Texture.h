/***********************************************************************
    created:    Tue Mar 10 2009
    author:     Paul D Turner (parts based on code by Keith Mok)
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
#ifndef _CEGUIDirectFBTexture_h_
#define _CEGUIDirectFBTexture_h_

#include "../../Texture.h"
#include "CEGUI/RendererModules/DirectFB/Renderer.h"

// Start of CEGUI namespace section
namespace CEGUI
{

//! Implementation of CEGUI::Texture interface using DirectFB.
class DirectFBTexture : public Texture
{
public:
    //! Return a pointer to the IDirectFBSurface this texture represents.
    IDirectFBSurface* getDirectFBSurface() const;

    // implement interface for Texture
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
    // friends to allow renderer to construct and destroy texture objects
    friend Texture& DirectFBRenderer::createTexture(const String&);
    friend Texture& DirectFBRenderer::createTexture(const String&, const String&, const String&);
    friend Texture& DirectFBRenderer::createTexture(const String&, const Sizef&);
    friend void DirectFBRenderer::destroyTexture(const String&);

    //! Basic constructor.
    DirectFBTexture(IDirectFB& directfb, const String& name);
    //! Construct texture from file.
    DirectFBTexture(IDirectFB& directfb, const String& name,
                    const String& filename, const String& resourceGroup);
    //! Construct texture with given size.
    DirectFBTexture(IDirectFB& directfb, const String& name, const Sizef& size);
    //! Destructor.
    ~DirectFBTexture();

    //! clean up the internal texture.
    void cleanupDirectFBTexture();
    //! updates cached scale value used to map pixels to texture co-ords.
    void updateCachedScaleValues();

    //! DirectFB interface we were given when constructed.
    IDirectFB& d_directfb;
    //! surface representing the texture.
    IDirectFBSurface* d_texture;
    //! Size of the texture.
    Sizef d_size;
    //! original pixel of size data loaded into texture
    Sizef d_dataSize;
    //! cached pixel to texel mapping scale values.
    Vector2f d_texelScaling;
    //! The name given for this texture.
    const String d_name;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIDirectFBTexture_h_
