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
#ifndef _CEGUIDirect3D9ViewportTarget_h_
#define _CEGUIDirect3D9ViewportTarget_h_

#include "CEGUI/RendererModules/Direct3D9/RenderTarget.h"
#include "../../Rect.h"

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Direct3D9 RenderTarget that represents a screen viewport.
*/
class DIRECT3D9_GUIRENDERER_API Direct3D9ViewportTarget : 
    public Direct3D9RenderTarget<>
{
public:
    /*!
    \brief
        Construct a default Direct3D9ViewportTarget that uses the current
        Direct3D9 viewport as it's initial area.
    */
    Direct3D9ViewportTarget(Direct3D9Renderer& owner);

    /*!
    \brief
        Construct a Direct3D9ViewportTarget that uses the specified Rect as it's
        initial area.

    \param area
        Rect object describing the initial viewport area that should be used for
        the RenderTarget.
    */
    Direct3D9ViewportTarget(Direct3D9Renderer& owner, const Rectf& area);

    // implementations of RenderTarget interface
    bool isImageryCache() const;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIDirect3D9ViewportTarget_h_
