/***********************************************************************
    created:    Sun Feb 1 2009
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
#ifndef _CEGUIOpenGLApplePBTextureTarget_h_
#define _CEGUIOpenGLApplePBTextureTarget_h_

#include <OpenGL/OpenGL.h>
#include "CEGUI/RendererModules/OpenGL/TextureTarget.h"

// Start of CEGUI namespace section
namespace CEGUI
{
class OpenGLTexture;

/*!
\brief
    OpenGLApplePBTextureTarget - allows rendering to an OpenGL texture via the
    Apple pbuffer extension.
*/
class OpenGLApplePBTextureTarget : public OpenGLTextureTarget
{
public:
    OpenGLApplePBTextureTarget(OpenGLRendererBase& owner);
    virtual ~OpenGLApplePBTextureTarget();

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

    //! Perform basic initialisation of the texture we're going to use.
    void initialiseTexture();

    //! Switch rendering so it targets the pbuffer
    void enablePBuffer() const;

    //! Switch rendering to target what was active before the pbuffer was used.
    void disablePBuffer() const;

    //! The current pbuffer object used by this TextureTarget.
    CGLPBufferObj d_pbuffer;
    //! The GL context that's used by this TextureTarget.
    CGLContextObj d_context;
    //! virtual screen used by the "parent" context
    GLint d_screen;
    //! Context that was active before ours was activated.
    mutable CGLContextObj d_prevContext;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIOpenGLApplePBTextureTarget_h_
