/***********************************************************************
    created:    Sat Mar 7 2009
    author:     Paul D Turner (parts based on code by Rajko Stojadinovic)
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
#ifndef _CEGUIDirect3D10GeometryBuffer_h_
#define _CEGUIDirect3D10GeometryBuffer_h_

#include "../../GeometryBuffer.h"
#include "CEGUI/RendererModules/Direct3D10/Renderer.h"
#include "../../Rect.h"
#include "../../Quaternion.h"

// Unfortunately, MinGW-w64 doesn't have <d3dx10.h>
#ifdef __MINGW32__
    #include <d3d10.h>
    #include <d3dx9.h>
#else
    #include <d3dx10.h>
#endif

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
class Direct3D10Texture;

//! Implementation of CEGUI::GeometryBuffer for the Direct3D 10 API.
class D3D10_GUIRENDERER_API Direct3D10GeometryBuffer : public GeometryBuffer
{
public:
    //! Constructor
    Direct3D10GeometryBuffer(Direct3D10Renderer& owner);

    //! Destructor
    ~Direct3D10GeometryBuffer();

    //! return pointer to D3DXMATRIX used by this GeometryBuffer
    const D3DXMATRIX* getMatrix() const;

    // Implement GeometryBuffer interface.
    void draw() const;
    void setTranslation(const Vector3f& v);
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

protected:
    //! update cached matrix
    void updateMatrix() const;
    //! Synchronise data in the hardware buffer with what's been added
    void syncHardwareBuffer() const;
    //! allocate the hardware vertex buffer large enough for \a count vertices.
    void allocateVertexBuffer(const size_t count) const;
    //! cleanup the hardware vertex buffer.
    void cleanupVertexBuffer() const;

    //! internal Vertex structure used for Direct3D based geometry.
    struct D3DVertex
    {
        //! The transformed position for the vertex.
        FLOAT x, y, z;
        //! colour of the vertex.
        DWORD diffuse;
        //! texture coordinates.
        float tu, tv;
    };

    //! type to track info for per-texture sub batches of geometry
    struct BatchInfo
    {
        const ID3D10ShaderResourceView* texture;
        uint vertexCount;
        bool clip;
    };

    // Direct3D10Renderer object that created and owns this GeometryBuffer.
    Direct3D10Renderer& d_owner;
    //! The D3D Device
    ID3D10Device& d_device;
    //! last texture that was set as active
    Direct3D10Texture* d_activeTexture;
    //! hardware buffer where vertices will be drawn from.
    mutable ID3D10Buffer* d_vertexBuffer;
    //! Size of the currently allocated vertex buffer.
    mutable UINT d_bufferSize;
    //! whether the h/w buffer is in sync with the added geometry
    mutable bool d_bufferSynched;
    //! type of container that tracks BatchInfos.
    typedef std::vector<BatchInfo> BatchList;
    //! list of texture batches added to the geometry buffer
    BatchList d_batches;
    //! type of container used to queue the geometry
    typedef std::vector<D3DVertex> VertexList;
    //! container where added geometry is stored.
    VertexList d_vertices;
    //! rectangular clip region
    Rectf d_clipRect;
    //! whether clipping will be active for the current batch
    bool d_clippingActive;
    //! translation vector
    Vector3f d_translation;
    //! rotation Quaternion
    Quaternion d_rotation;
    //! pivot point for rotation
    Vector3f d_pivot;
    //! RenderEffect that will be used by the GeometryBuffer
    RenderEffect* d_effect;
    //! model matrix cache
    mutable D3DXMATRIX d_matrix;
    //! true when d_matrix is valid and up to date
    mutable bool d_matrixValid;
};


} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIDirect3D10GeometryBuffer_h_
