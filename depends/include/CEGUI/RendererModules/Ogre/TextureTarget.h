/***********************************************************************
    created:    Tue Feb 17 2009
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
#ifndef _CEGUIOgreTextureTarget_h_
#define _CEGUIOgreTextureTarget_h_

#include "../../TextureTarget.h"
#include "CEGUI/RendererModules/Ogre/RenderTarget.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4250)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
//! CEGUI::TextureTarget implementation for the Ogre engine.
class OGRE_GUIRENDERER_API OgreTextureTarget : public OgreRenderTarget<TextureTarget>
{
public:
    //! Constructor.
    OgreTextureTarget(OgreRenderer& owner, Ogre::RenderSystem& rs);
    //! Destructor.
    virtual ~OgreTextureTarget();

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
    //! This wraps d_texture so it can be used by the core CEGUI lib.
    OgreTexture* d_CEGUITexture;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIOgreTextureTarget_h_
