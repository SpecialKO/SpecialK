/***********************************************************************
    created:    Tue Mar 3 2009
    author:     Paul D Turner (parts based on original code by Thomas Suter)
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
#ifndef _CEGUIIrrlichtTextureTarget_h_
#define _CEGUIIrrlichtTextureTarget_h_

#include "CEGUI/RendererModules/Irrlicht/RendererDef.h"
#include "../../TextureTarget.h"
#include "CEGUI/RendererModules/Irrlicht/RenderTarget.h"
#include "../../String.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4250)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
class IrrlichtTexture;

//! CEGUI::TextureTarget implementation for the Irrlicht engine.
class IRR_GUIRENDERER_API IrrlichtTextureTarget : public IrrlichtRenderTarget<TextureTarget>
{
public:
    //! Constructor.
    IrrlichtTextureTarget(IrrlichtRenderer& owner,
                          irr::video::IVideoDriver& driver);
    //! Destructor.
    virtual ~IrrlichtTextureTarget();

    // overrides from IrrlichtRenderTarget
    void activate();
    void deactivate();
    // implementation of RenderTarget interface
    bool isImageryCache() const;
    // implement CEGUI::TextureTarget interface.
    void clear();
    Texture& getTexture() const;
    void declareRenderSize(const Sizef& sz);
    bool isRenderingInverted() const;

protected:
    //! default / initial size for the underlying texture.
    static const float DEFAULT_SIZE;
    //! static data used for creating texture names
    static uint s_textureNumber;
    //! helper to generate unique texture names
    static String generateTextureName();

    //! cleans up the current render target texture used by this object.
    void cleanupTargetTexture();

    //! The irrlicht render target texture we'll be drawing to
    irr::video::ITexture* d_texture;
    //! This wraps d_texture so it can be used by the core CEGUI lib.
    IrrlichtTexture* d_CEGUITexture;
};

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIIrrlichtTextureTarget_h_
