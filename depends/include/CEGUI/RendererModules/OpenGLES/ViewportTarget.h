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
#ifndef _CEGUIOpenGLESViewportTarget_h_
#define _CEGUIOpenGLESViewportTarget_h_

#include "CEGUI/RendererModules/OpenGLES/RenderTarget.h"
#include "CEGUI/Rect.h"

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    OpenGLES implementation of a RenderTarget that represents am on-scren
    viewport.
*/
class OPENGLES_GUIRENDERER_API OpenGLESViewportTarget : public OpenGLESRenderTarget<>
{
public:
    /*!
    \brief
        Construct a default OpenGLESViewportTarget that uses the currently
        defined OpenGLES viewport as it's initial area.
    */
    OpenGLESViewportTarget(OpenGLESRenderer& owner);

    /*!
    \brief
        Construct a OpenGLESViewportTarget that uses the specified Rect as it's
        initial area.

    \param area
        Rect object describing the initial viewport area that should be used for
        the RenderTarget.
    */
    OpenGLESViewportTarget(OpenGLESRenderer& owner, const Rectf& area);

    // implementations of RenderTarget interface
    bool isImageryCache() const;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIOpenGLESViewportTarget_h_
