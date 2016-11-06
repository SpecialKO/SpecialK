/***********************************************************************
    created:    Tue Mar 3 2009
    author:     Paul D Turner (based on original code by Thomas Suter)
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
#ifndef _CEGUIIrrlichtTexture_h_
#define _CEGUIIrrlichtTexture_h_

#include "CEGUI/RendererModules/Irrlicht/RendererDef.h"
#include "../../Texture.h"
#include "CEGUI/RendererModules/Irrlicht/Renderer.h"
#include <string>

namespace irr
{
namespace video
{
class ITexture;
}
}

// Start of CEGUI namespace section
namespace CEGUI
{
class IrrlichtRenderer;

//! Implementation of the CEGUI::Texture class for the Irrlicht engine.
class IRR_GUIRENDERER_API IrrlichtTexture : public Texture
{
public:
    //! set the underlying Irrlicht texture represented by this Texture.
    void setIrrlichtTexture(irr::video::ITexture* tex);

    //! return a pointer to the Irrlicht texture represented by this Texture.
    irr::video::ITexture* getIrrlichtTexture() const;

    /*!
    \brief
        Sets what the texture should consider as the original data size.

    \note
        This also causes the texel scaling values to be updated.
    */
    void setOriginalDataSize(const Sizef& sz);

    //! return a std::string containing a unique name.
    static std::string getUniqueName();

    // Implement texture interface.
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
    // friends in order that Renderer module can create / destroy textures
    friend Texture& IrrlichtRenderer::createTexture(const String&);
    friend Texture& IrrlichtRenderer::createTexture(const String&, const String&,
                                                    const String&);
    friend Texture& IrrlichtRenderer::createTexture(const String&, const Sizef&);
    friend void IrrlichtRenderer::destroyTexture(Texture&);
    friend void IrrlichtRenderer::destroyTexture(const String&);

    //! Construct a basic texture
    IrrlichtTexture(IrrlichtRenderer& owner, irr::video::IVideoDriver& driver,
                    const String& name);
    //! Construct a texture from the specified file.
    IrrlichtTexture(IrrlichtRenderer& owner, irr::video::IVideoDriver& driver,
                    const String& name, const String& filename,
                    const String& resourceGroup);
    //! Construct a texture with the given size.
    IrrlichtTexture(IrrlichtRenderer& owner, irr::video::IVideoDriver& driver,
                    const String& name, const Sizef& size);
    //! destructor.
    ~IrrlichtTexture();

    //! create the underlying Irrlicht texture with the given size
    void createIrrlichtTexture(const Sizef& sz);

    //! release the underlying irrlicht texture currently used.
    void freeIrrlichtTexture();

    //! updates cached scale value used to map pixels to texture co-ords.
    void updateCachedScaleValues();

    //! Counter used to provide unique texture names.
    static uint32 d_textureNumber;
    //! Irrlicht video driver we're to use.
    irr::video::IVideoDriver& d_driver;
    //! ptr to underlying irrlicht texture.
    irr::video::ITexture* d_texture;
    //! Size of the texture.
    Sizef d_size;
    //! original pixel of size data loaded into texture
    Sizef d_dataSize;
    //! cached pixel to texel mapping scale values.
    Vector2f d_texelScaling;
    //! reference to the IrrlichtRenderer that created this texture
    IrrlichtRenderer& d_owner;
    //! name given when texture was created.
    const String d_name;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIIrrlichtTexture_h_
