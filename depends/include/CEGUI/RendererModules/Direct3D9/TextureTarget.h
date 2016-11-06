/***********************************************************************
    created:    Fri Feb 13 2009
    author:     Paul D Turner
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
#ifndef _CEGUIDirect3D9TextureTarget_h_
#define _CEGUIDirect3D9TextureTarget_h_

#include "CEGUI/RendererModules/Direct3D9/RenderTarget.h"
#include "../../TextureTarget.h"
#include <d3d9.h>

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4250)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
class Direct3D9Texture;

//! Direct3D9TextureTarget - allows rendering to an Direct3D9 texture via .
class DIRECT3D9_GUIRENDERER_API Direct3D9TextureTarget : public Direct3D9RenderTarget<TextureTarget>
{
public:
    Direct3D9TextureTarget(Direct3D9Renderer& owner);
    virtual ~Direct3D9TextureTarget();

    //! auto called via the Renderer prior to Reset on the Direct3DDevice9.
    void preD3DReset();

    //! auto called via the Renderer after Reset on the Direct3DDevice9.
    void postD3DReset();

    // overrides from Direct3D9RenderTarget
    void activate();
    void deactivate();
    // implementation of RenderTarget interface
    bool isImageryCache() const;
    // implementation of TextureTarget interface
    void clear();
    Texture& getTexture() const;
    void declareRenderSize(const Sizef& sz);
    bool isRenderingInverted() const;

protected:
    //! default size of created texture objects
    static const float DEFAULT_SIZE;
    //! static data used for creating texture names
    static uint s_textureNumber;
    //! helper to generate unique texture names
    static String generateTextureName();

    //! allocate and set up the texture used for rendering.
    void initialiseRenderTexture();
    //! clean up the texture used for rendering.
    void cleanupRenderTexture();
    //! resize the texture
    void resizeRenderTexture();

    //! switch to the texture surface & depth buffer
    void enableRenderTexture();
    //! switch back to previous surface
    void disableRenderTexture();

    //! Direct3D9 texture that's rendered to.
    LPDIRECT3DTEXTURE9 d_texture;
    //! Direct3D9 surface for the texture
    LPDIRECT3DSURFACE9 d_surface;
    //! we use this to wrap d_texture so it can be used by the core CEGUI lib.
    Direct3D9Texture* d_CEGUITexture;
    //! colour surface that was in use before this target was activated.
    LPDIRECT3DSURFACE9 d_prevColourSurface;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIDirect3D9TextureTarget_h_
