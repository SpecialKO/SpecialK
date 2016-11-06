/***********************************************************************
    created:    Tue Mar 10 2009
    author:     Paul D Turner (parts based on code by Keith Mok)
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
#ifndef _CEGUIDirectFBRenderTarget_h_
#define _CEGUIDirectFBRenderTarget_h_

#include "../../RenderTarget.h"
#include "../../Rect.h"
#include <directfb.h>

// Start of CEGUI namespace section
namespace CEGUI
{
class DirectFBRenderer;

//! Implementation of CEGUI::RenderTarget for DirectFB
class DirectFBRenderTarget : public RenderTarget
{
public:
    //! Constructor.
    DirectFBRenderTarget(DirectFBRenderer& owner, IDirectFBSurface& target);

    // Implement RenderTarget interface
    void draw(const GeometryBuffer& buffer);
    void draw(const RenderQueue& queue);
    void setArea(const Rectf& area);
    const Rectf& getArea() const;
    bool isImageryCache() const;
    void activate();
    void deactivate();
    void unprojectPoint(const GeometryBuffer& buff,
                                const Vector2f& p_in, Vector2f& p_out) const;

protected:
    //! DirectFBRenderer that created and owns this RenderTarget.
    DirectFBRenderer& d_owner;
    //! IDirectFBSurface targetted by this RenderTarget.
    IDirectFBSurface& d_target;
    //! holds defined area for the RenderTarget
    Rectf d_area;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIDirectFBRenderTarget_h_
