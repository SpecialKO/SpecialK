/***********************************************************************
    created:    Tue Apr 30 2013
    authors:    Paul D Turner <paul@cegui.org.uk>
                Lukas E Meindl
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2013 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIGeometryBufferBase_h_
#define _CEGUIGeometryBufferBase_h_

#include "../../GeometryBuffer.h"
#include "CEGUI/RendererModules/OpenGL/RendererBase.h"
#include "../../Rect.h"
#include "../../Quaternion.h"

#include <utility>
#include <vector>

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

namespace CEGUI
{
class OpenGLTexture;

/*!
\brief
    OpenGL based implementation of the GeometryBuffer interface.
*/
class OPENGL_GUIRENDERER_API OpenGLGeometryBufferBase : public GeometryBuffer
{
public:
    //! Constructor
    OpenGLGeometryBufferBase(OpenGLRendererBase& owner);
    virtual ~OpenGLGeometryBufferBase();

    // implementation of abstract members from GeometryBuffer
    void setTranslation(const Vector3f& t);
    void setRotation(const Quaternion& r);
    void setPivot(const Vector3f& p);
    void setClippingRegion(const Rectf& region);
    void appendVertex(const Vertex& vertex);
    void appendGeometry(const Vertex* const vbuff, uint vertex_count);
    void setActiveTexture(Texture* texture);
    void reset();
    Texture* getActiveTexture() const;
    uint getVertexCount() const;
    uint getBatchCount() const;
    void setRenderEffect(RenderEffect* effect);
    RenderEffect* getRenderEffect();
    void setClippingActive(const bool active);
    bool isClippingActive() const;

    //! return the GL modelview matrix used for this buffer.
    const mat4Pimpl* getMatrix() const;

protected:
    //! perform batch management operations prior to adding new geometry.
    void performBatchManagement();

    //! update cached matrix
    void updateMatrix() const;

    //! internal Vertex structure used for GL based geometry.
    struct GLVertex
    {
        float tex[2];
        float colour[4];
        float position[3];
    };

    //! type to track info for per-texture sub batches of geometry
    struct BatchInfo
    {
        uint texture;
        uint vertexCount;
        bool clip;
    };

    //! OpenGLRendererBase that owns the GeometryBuffer.
    OpenGLRendererBase* d_owner;
    //! last texture that was set as active
    OpenGLTexture* d_activeTexture;
    //! type of container that tracks BatchInfos.
    typedef std::vector<BatchInfo> BatchList;
    //! list of texture batches added to the geometry buffer
    BatchList d_batches;
    //! type of container used to queue the geometry
    typedef std::vector<GLVertex> VertexList;
    //! container where added geometry is stored.
    VertexList d_vertices;
    //! rectangular clip region
    Rectf d_clipRect;
    //! whether clipping will be active for the current batch
    bool d_clippingActive;
    //! translation vector
    Vector3f d_translation;
    //! rotation quaternion
    Quaternion d_rotation;
    //! pivot point for rotation
    Vector3f d_pivot;
    //! RenderEffect that will be used by the GeometryBuffer
    RenderEffect* d_effect;
    //! model matrix cache - we use double because gluUnproject takes double
    mutable mat4Pimpl*              d_matrix;
    //! true when d_matrix is valid and up to date
    mutable bool                    d_matrixValid;
};

}

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif

