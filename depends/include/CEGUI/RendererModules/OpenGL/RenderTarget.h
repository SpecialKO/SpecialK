/***********************************************************************
    created:    Wed, 8th Feb 2012
    author:     Lukas E Meindl (based on code by Paul D Turner)
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2012 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIOpenGLRenderTarget_h_
#define _CEGUIOpenGLRenderTarget_h_

#include "CEGUI/RendererModules/OpenGL/RendererBase.h"
#include "../../RenderTarget.h"
#include "../../Rect.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

namespace CEGUI
{
/*!
\brief
    Intermediate OpenGL implementation of a RenderTarget.
*/
template <typename T = RenderTarget>
class OPENGL_GUIRENDERER_API OpenGLRenderTarget : public T
{
public:
    //! Constructor
    OpenGLRenderTarget(OpenGLRendererBase& owner);
    virtual ~OpenGLRenderTarget();

    // implement parts of RenderTarget interface
    void draw(const GeometryBuffer& buffer);
    void draw(const RenderQueue& queue);
    void setArea(const Rectf& area);
    const Rectf& getArea() const;
    void activate();
    void deactivate();
    void unprojectPoint(const GeometryBuffer& buff,
                        const Vector2f& p_in, Vector2f& p_out) const;

protected:
    //! helper that initialises the cached matrix
    virtual void updateMatrix() const;

    //! OpenGLRendererBase that created this object
    OpenGLRendererBase& d_owner;
    //! holds defined area for the RenderTarget
    Rectf d_area;
    //! tangent of the y FOV half-angle; used to calculate viewing distance.
    static const double d_yfov_tan;
    //! saved copy of projection matrix
    mutable mat4Pimpl* d_matrix;
    //! true if saved matrix is up to date
    mutable bool d_matrixValid;
    //! tracks viewing distance (this is set up at the same time as d_matrix)
    mutable double d_viewDistance;
};

}

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif

