/***********************************************************************
    created:    Wed May 5 2010
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
#ifndef _CEGUIDirect3D10RenderTarget_h_
#define _CEGUIDirect3D10RenderTarget_h_

#include "../../RenderTarget.h"
#include "CEGUI/RendererModules/Direct3D11/Renderer.h"
#include "../../Rect.h"
#include <d3dx10.h>


#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
//! Implementation of an ntermediate RenderTarget for the Direct3D 10 API
template <typename T = RenderTarget>
class D3D11_GUIRENDERER_API Direct3D11RenderTarget : public T
{
public:
    //! Constructor
    Direct3D11RenderTarget(Direct3D11Renderer& owner);

    // implement parts of RenderTarget interface
    void draw(const GeometryBuffer& buffer);
    void draw(const RenderQueue& queue);
    void setArea(const Rectf& area);
    const Rectf& getArea() const;
    void activate();
    void deactivate();
    void unprojectPoint(const GeometryBuffer& buff,
                        const Vector2f& p_in,
                        Vector2f& p_out) const;

protected:
    //! helper that initialises the cached matrix
    void updateMatrix() const;
    //! helper to initialise the D3D10_VIEWPORT \a vp for this target.
    void setupViewport(D3D11_VIEWPORT& vp) const;

    //! Renderer that created and owns the render target.
    Direct3D11Renderer& d_owner;
    //! D3D10Device interface.
    IDevice11& d_device;
    //! holds defined area for the RenderTarget
    Rectf d_area;
    //! projection / view matrix cache
    mutable D3DXMATRIX d_matrix;
    //! true when d_matrix is valid and up to date
    mutable bool d_matrixValid;
    //! tracks viewing distance (this is set up at the same time as d_matrix)
    mutable float d_viewDistance;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIDirect3D10RenderTarget_h_
