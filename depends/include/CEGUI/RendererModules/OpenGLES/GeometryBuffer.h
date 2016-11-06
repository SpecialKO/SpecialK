/***********************************************************************
    created:    Thu Jan 8 2009
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
#ifndef _CEGUIOpenGLESGeometryBuffer_h_
#define _CEGUIOpenGLESGeometryBuffer_h_

#include "CEGUI/GeometryBuffer.h"
#include "CEGUI/RendererModules/OpenGLES/Renderer.h"
#include "CEGUI/Rect.h"
#include "CEGUI/Quaternion.h"

#include <utility>
#include <vector>

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
class OpenGLESTexture;

/*!
\brief
    OpenGLES based implementation of the GeometryBuffer interface.
*/
class OPENGLES_GUIRENDERER_API OpenGLESGeometryBuffer : public GeometryBuffer
{
public:
    //! Constructor
    OpenGLESGeometryBuffer();

    // implementation of abstract members from GeometryBuffer
    void draw() const;
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
	const float* getMatrix() const;

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

    //! last texture that was set as active
    OpenGLESTexture* d_activeTexture;
    //! type to track info for per-texture sub batches of geometry
    typedef std::pair<uint, uint> BatchInfo;
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
    //! model matrix cache
    mutable float d_matrix[16];
    //! true when d_matrix is valid and up to date
    mutable bool d_matrixValid;
};


} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIOpenGLESGeometryBuffer_h_
