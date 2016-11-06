/***********************************************************************
    created:    Sun Feb 1 2009
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
#ifndef _CEGUIOpenGLWGLPBTextureTarget_h_
#define _CEGUIOpenGLWGLPBTextureTarget_h_

//#include <windows.h>
#include "CEGUI/RendererModules/OpenGL/GL.h"
#include <GL/wglew.h>

#include "CEGUI/RendererModules/OpenGL/TextureTarget.h"
#include "../../Rect.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4250)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
class OpenGLTexture;

/*!
\brief
    OpenGLWGLPBTextureTarget - allows rendering to an OpenGL texture via the
    pbuffer WGL extension.
*/
class OPENGL_GUIRENDERER_API OpenGLWGLPBTextureTarget : public OpenGLTextureTarget
{
public:
    OpenGLWGLPBTextureTarget(OpenGLRendererBase& owner);
    virtual ~OpenGLWGLPBTextureTarget();

    // overrides from OpenGLRenderTarget
    void activate();
    void deactivate();
    // implementation of TextureTarget interface
    void clear();
    void declareRenderSize(const Sizef& sz);
    // specialise functions from OpenGLTextureTarget
    void grabTexture();
    void restoreTexture();

protected:
    //! default size of created texture objects
    static const float DEFAULT_SIZE;

    //! Initialise the PBuffer with the needed size
    void initialisePBuffer();

    //! Cleans up the pbuffer resources.
    void releasePBuffer();

    //! Switch rendering so it targets the pbuffer
    void enablePBuffer() const;

    //! Switch rendering to target what was active before the pbuffer was used.
    void disablePBuffer() const;

    //! Perform basic initialisation of the texture we're going to use.
    void initialiseTexture();

    //! Holds the pixel format we use when creating the pbuffer.
    int d_pixfmt;
    //! Handle to the pbuffer itself.
    HPBUFFERARB d_pbuffer;
    //! Handle to the rendering context for the pbuffer.
    HGLRC d_context;
    //! Handle to the Windows device context for the pbuffer.
    HDC d_hdc;
    //! Handle to the rendering context in use when we switched to the pbuffer.
    mutable HGLRC d_prevContext;
    //! Handle to the device context in use when we switched to the pbuffer.
    mutable HDC d_prevDC;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIOpenGLWGLPBTextureTarget_h_
