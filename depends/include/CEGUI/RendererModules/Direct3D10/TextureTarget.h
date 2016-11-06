/***********************************************************************
    created:    Sat Mar 7 2009
    author:     Paul D Turner (parts based on code by Rajko Stojadinovic)
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
#ifndef _CEGUIDirect3D10TextureTarget_h_
#define _CEGUIDirect3D10TextureTarget_h_

#include "CEGUI/RendererModules/Direct3D10/RenderTarget.h"
#include "../../TextureTarget.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4250)
#   pragma warning(disable : 4251)
#endif

// d3d forward refs
struct ID3D10Texture2D;
struct ID3D10RenderTargetView;
struct ID3D10DepthStencilView;

// Start of CEGUI namespace section
namespace CEGUI
{
class Direct3D10Texture;

//! Direct3D10TextureTarget - allows rendering to Direct3D 10 textures.
class D3D10_GUIRENDERER_API Direct3D10TextureTarget : public Direct3D10RenderTarget<TextureTarget>
{
public:
    Direct3D10TextureTarget(Direct3D10Renderer& owner);
    virtual ~Direct3D10TextureTarget();

    // overrides from Direct3D10RenderTarget
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

    //! Direct3D10 texture that's rendered to.
    ID3D10Texture2D* d_texture;
    //! render target view for d_texture
    ID3D10RenderTargetView* d_renderTargetView;
    //! we use this to wrap d_texture so it can be used by the core CEGUI lib.
    Direct3D10Texture* d_CEGUITexture;
    //! render target view that was bound before this target was activated
    ID3D10RenderTargetView* d_previousRenderTargetView;
    //! depth stencil view that was bound before this target was activated
    ID3D10DepthStencilView* d_previousDepthStencilView;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIDirect3D10TextureTarget_h_
