/***********************************************************************
    created:    Sun Jan 11 2009
    author:     Paul D Turner
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
#ifndef _CEGUIOpenGLESFBOTextureTarget_h_
#define _CEGUIOpenGLESFBOTextureTarget_h_

#include "CEGUI/RendererModules/OpenGLES/RenderTarget.h"
#include "CEGUI/TextureTarget.h"
#include "CEGUI/Rect.h"
#include "CEGUI/RendererModules/OpenGLES/GLES.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4250)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
class OpenGLESTexture;

//! OpenGLESFBOTextureTarget - allows rendering to an OpenGLES texture via FBO.
class OPENGLES_GUIRENDERER_API OpenGLESFBOTextureTarget :
    public OpenGLESRenderTarget<TextureTarget>
{
public:
    OpenGLESFBOTextureTarget(OpenGLESRenderer& owner);
    virtual ~OpenGLESFBOTextureTarget();

    // overrides from OpenGLESRenderTarget
    void activate();
    void deactivate();
    // implementation of RenderTarget interface
    bool isImageryCache() const;
    // implementation of TextureTarget interface
    void clear();
    Texture& getTexture() const;
    void declareRenderSize(const Sizef& sz);
    bool isRenderingInverted() const;

	//! initialize FBO extension functions pointers
	static void initializedFBOExtension();

protected:
    //! default size of created texture objects
    static const float DEFAULT_SIZE;

    //! allocate and set up the texture used with the FBO.
    void initialiseRenderTexture();
    //! resize the texture
    void resizeRenderTexture();
    //! generate a texture name
    String generateTextureName();
	
	//! Saving fbo before activation
	GLint d_oldFbo;
    //! Frame buffer object.
    GLuint d_frameBuffer;
    //! Associated OpenGLES texture ID
    GLuint d_texture;
    //! we use this to wrap d_texture so it can be used by the core CEGUI lib.
    OpenGLESTexture* d_CEGUITexture;
    //! static member var used to generate unique texture names.
    static uint s_textureNumber;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIOpenGLESFBOTextureTarget_h_
