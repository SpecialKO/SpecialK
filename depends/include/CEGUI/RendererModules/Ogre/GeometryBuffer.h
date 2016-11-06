/***********************************************************************
    created:    Tue Feb 17 2009
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
#ifndef _CEGUIOgreGeometryBuffer_h_
#define _CEGUIOgreGeometryBuffer_h_

#include "CEGUI/GeometryBuffer.h"
#include "CEGUI/RendererModules/Ogre/Renderer.h"
#include "CEGUI/Rect.h"
#include "CEGUI/Quaternion.h"

#include <OgreMatrix4.h>
#include <OgreColourValue.h>
#include <OgreRenderOperation.h>
#include <OgreTexture.h>

#include <utility>
#include <vector>

#ifdef CEGUI_USE_OGRE_HLMS
#include <OgreRenderable.h>
#endif

// Ogre forward refs
namespace Ogre
{
class RenderSystem;
}

// Start of CEGUI namespace section
namespace CEGUI
{
//! Implementation of CEGUI::GeometryBuffer for the Ogre engine
class OGRE_GUIRENDERER_API OgreGeometryBuffer : public GeometryBuffer
{
public:
    //! Constructor
    OgreGeometryBuffer(OgreRenderer& owner, Ogre::RenderSystem& rs);
    //! Destructor
    virtual ~OgreGeometryBuffer();

    //! return the transformation matrix used for this buffer.
    const Ogre::Matrix4& getMatrix() const;

    // implement CEGUI::GeometryBuffer interface.
    virtual void draw() const;
    virtual void setTranslation(const Vector3f& v);
    virtual void setRotation(const Quaternion& r);
    virtual void setPivot(const Vector3f& p);
    virtual void setClippingRegion(const Rectf& region);
    virtual void appendVertex(const Vertex& vertex);
    virtual void appendGeometry(const Vertex* const vbuff, uint vertex_count);
    virtual void setActiveTexture(Texture* texture);
    virtual void reset();
    virtual Texture* getActiveTexture() const;
    virtual uint getVertexCount() const;
    virtual uint getBatchCount() const;
    virtual void setRenderEffect(RenderEffect* effect);
    virtual RenderEffect* getRenderEffect();
    void setClippingActive(const bool active);
    bool isClippingActive() const;

protected:
    //! convert CEGUI::colour into something Ogre can use
    Ogre::RGBA colourToOgre(const Colour& col) const;
    //! update cached matrix
    void updateMatrix() const;
    //! Synchronise data in the hardware buffer with what's been added
    void syncHardwareBuffer() const;
    //! set up texture related states
    void initialiseTextureStates() const;

    //! vertex structure used internally and also by Ogre.
    struct OgreVertex
    {
        float x, y, z;
        Ogre::RGBA diffuse;
        float u, v;
    };

    //! type to track info for per-texture sub batches of geometry
    struct BatchInfo
    {
        Ogre::TexturePtr texture;
        uint vertexCount;
        bool clip;
    };

    //! Renderer object that owns this GeometryBuffer
    OgreRenderer& d_owner;
    //! Ogre render system we're to use.
    Ogre::RenderSystem& d_renderSystem;
    //! Texture that is set as active
    OgreTexture* d_activeTexture;
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
    //! offset to be applied to all geometry
    Vector2f d_texelOffset;
    //! model matrix cache
    mutable Ogre::Matrix4 d_matrix;
    //! true when d_matrix is valid and up to date
    mutable bool d_matrixValid;
#ifdef CEGUI_USE_OGRE_HLMS
    //! Render operation for this buffer.
    mutable Ogre::v1::RenderOperation d_renderOp;
    //! H/W buffer where the vertices are rendered from.
    mutable Ogre::v1::HardwareVertexBufferSharedPtr d_hwBuffer;
#else
    //! Render operation for this buffer.
    mutable Ogre::RenderOperation d_renderOp;
    //! H/W buffer where the vertices are rendered from.
    mutable Ogre::HardwareVertexBufferSharedPtr d_hwBuffer;
#endif
    //! whether the h/w buffer is in sync with the added geometry
    mutable bool d_sync;
    //! type of container that tracks BatchInfos.
    typedef std::vector<BatchInfo> BatchList;
    //! list of texture batches added to the geometry buffer
    BatchList d_batches;
    //! type of container used to queue the geometry
    typedef std::vector<OgreVertex> VertexList;
    //! container where added geometry is stored.
    VertexList d_vertices;
};


} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIOgreGeometryBuffer_h_
