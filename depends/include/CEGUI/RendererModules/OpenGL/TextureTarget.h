/***********************************************************************
    created:    Thu Oct 15 2009
    author:     Paul D Turner <paul@cegui.org.uk>
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
#ifndef _CEGUIOpenGLTextureTarget_h_
#define _CEGUIOpenGLTextureTarget_h_

#include "CEGUI/RendererModules/OpenGL/RenderTarget.h"
#include "../../TextureTarget.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4250)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    OpenGLTextureTarget - Common base class for all OpenGL render targets
    based on some form of RTT support.
*/
class OPENGL_GUIRENDERER_API OpenGLTextureTarget : public OpenGLRenderTarget<TextureTarget>
{
public:
    //! constructor.
    OpenGLTextureTarget(OpenGLRendererBase& owner);
    //! destructor
    virtual ~OpenGLTextureTarget();

    // implementation of RenderTarget interface
    bool isImageryCache() const;
    // implementation of parts of TextureTarget interface
    Texture& getTexture() const;
    bool isRenderingInverted() const;

    /*!
    \brief
        Grab the texture to a local buffer.

        This will destroy the OpenGL texture, and restoreTexture must be called
        before using it again.
    */
    virtual void grabTexture();

    /*!
    \brief
        Restore the texture from the locally buffered copy previously create by
        a call to grabTexture.
    */
    virtual void restoreTexture();

protected:
    //! helper to generate unique texture names
    static String generateTextureName();
    //! static data used for creating texture names
    static uint s_textureNumber;

    //! helper to create CEGUI::Texture d_CEGUITexture;
    void createCEGUITexture();

    //! Associated OpenGL texture ID
    GLuint d_texture;
    //! we use this to wrap d_texture so it can be used by the core CEGUI lib.
    OpenGLTexture* d_CEGUITexture;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif  // end of guard _CEGUIOpenGLTextureTarget_h_
