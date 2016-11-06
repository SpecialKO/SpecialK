/***********************************************************************
    created:    Sat Jan 31 2009
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
#ifndef _CEGUIOpenGLGLXPBTextureTarget_h_
#define _CEGUIOpenGLGLXPBTextureTarget_h_

#include <GL/glxew.h>

#include "CEGUI/RendererModules/OpenGL/TextureTarget.h"

// Start of CEGUI namespace section
namespace CEGUI
{
class OpenGLTexture;

/*!
\brief
    OpenGLGLXPBTextureTarget - allows rendering to an OpenGL texture via the
    pbuffer provided in GLX 1.3 and above.
*/
class OPENGL_GUIRENDERER_API OpenGLGLXPBTextureTarget : public OpenGLTextureTarget
{
public:
    OpenGLGLXPBTextureTarget(OpenGLRendererBase& owner);
    virtual ~OpenGLGLXPBTextureTarget();

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

    //! Switch rendering so it targets the pbuffer
    void enablePBuffer() const;

    //! Switch rendering to target what was active before the pbuffer was used.
    void disablePBuffer() const;

    //! Perform basic initialisation of the texture we're going to use.
    void initialiseTexture();

    //! Selects an appropriate FB config to use and stores in d_fbconfig.
    void selectFBConfig();

    //!Creates the context to use with the pbuffer.
    void createContext();

    //! X server display.
    Display* d_dpy;
    //! GLX config used in creating pbuffer
    GLXFBConfig d_fbconfig;
    //! GLX context
    GLXContext d_context;
    //! The GLX pbuffer we're using.
    GLXPbuffer d_pbuffer;
    //! stores previous X display when switching to pbuffer
    mutable Display* d_prevDisplay;
    //! stores previous GLX drawable when switching to pbuffer
    mutable GLXDrawable d_prevDrawable;
    //! stores previous GLX context when switching to pbuffer
    mutable GLXContext d_prevContext;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIOpenGLGLXPBTextureTarget_h_
