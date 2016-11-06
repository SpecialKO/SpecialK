/***********************************************************************
    created:    Thu Jan 8 2009
    author:     Paul D Turner
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2010 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIGeometryBuffer_h_
#define _CEGUIGeometryBuffer_h_

#include "CEGUI/Base.h"
#include "CEGUI/Renderer.h"
#include "CEGUI/Rect.h"

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Abstract class defining the interface for objects that buffer geometry for
    later rendering.
*/
class CEGUIEXPORT GeometryBuffer :
    public AllocatedObject<GeometryBuffer>
{
public:
    //! Destructor
    virtual ~GeometryBuffer();

    /*!
    \brief
        Draw the geometry buffered within this GeometryBuffer object.
    */
    virtual void draw() const = 0;

    /*!
    \brief
        Set the translation to be applied to the geometry in the buffer when it
        is subsequently rendered.

    \param v
        Vector3 describing the three axis translation vector to be used.
    */
    virtual void setTranslation(const Vector3f& v) = 0;

    /*!
    \brief
        Set the rotations to be applied to the geometry in the buffer when it is
        subsequently rendered.

    \param r
        Quaternion describing the rotation to be used.
    */
    virtual void setRotation(const Quaternion& r) = 0;

    /*!
    \brief
        Set the pivot point to be used when applying the rotations.

    \param p
        Vector3 describing the location of the pivot point to be used when
        applying the rotation to the geometry.
    */
    virtual void setPivot(const Vector3f& p) = 0;

    /*!
    \brief
        Set the clipping region to be used when rendering this buffer.
    */
    virtual void setClippingRegion(const Rectf& region) = 0;

    /*!
    \brief
        Append a single vertex to the buffer.

    \param vertex
        Vertex object describing the vertex to be added to the GeometryBuffer.
    */
    virtual void appendVertex(const Vertex& vertex) = 0;

    /*!
    \brief
        Append a number of vertices from an array to the GeometryBuffer.

    \param vbuff
        Pointer to an array of Vertex objects that describe the vertices that
        are to be added to the GeometryBuffer.

    \param vertex_count
        The number of Vertex objects from the array \a vbuff that are to be
        added to the GeometryBuffer.
    */
    virtual void appendGeometry(const Vertex* const vbuff, uint vertex_count)=0;

    /*!
    \brief
        Set the active texture to be used with all subsequently added vertices.

    \param texture
        Pointer to a Texture object that shall be used for subsequently added
        vertices.  This may be 0, in which case texturing will be disabled for
        subsequently added vertices.
    */
    virtual void setActiveTexture(Texture* texture) = 0;

    /*!
    \brief
        Clear all buffered data and reset the GeometryBuffer to the default
        state.
    */
    virtual void reset() = 0;

    /*!
    \brief
        Return a pointer to the currently active Texture object.  This may
        return 0 if no texture is set.

    \return
        Pointer the Texture object that is currently active, or 0 if texturing
        is not being used.
    */
    virtual Texture* getActiveTexture() const = 0;

    /*!
    \brief
        Return the total number of vertices currently held by this
        GeometryBuffer object.

    \return
        The number of vertices that have been appended to this GeometryBuffer.
    */
    virtual uint getVertexCount() const = 0;

    /*!
    \brief
        Return the number of batches of geometry that this GeometryBuffer has
        split the vertices into.

    \note
        How batching is done will be largely implementation specific, although
        it would be reasonable to expect that you will have <em>at least</em>
        one batch of geometry per texture switch.

    \return
        The number of batches of geometry held by the GeometryBuffer.
    */
    virtual uint getBatchCount() const = 0;

    /*!
    \brief
        Set the RenderEffect to be used by this GeometryBuffer.

    \param effect
        Pointer to the RenderEffect to be used during renderng of the
        GeometryBuffer.  May be 0 to remove a previously added RenderEffect.

    \note
        When adding a RenderEffect, the GeometryBuffer <em>does not</em> take
        ownership of, nor make a copy of, the passed RenderEffect - this means
        you need to be careful not to delete the RenderEffect if it might still
        be in use!
    */
    virtual void setRenderEffect(RenderEffect* effect) = 0;

    /*!
    \brief
        Return the RenderEffect object that is assigned to this GeometryBuffer
        or 0 if none.
    */
    virtual RenderEffect* getRenderEffect() = 0;

    /*!
    \brief
        Set the blend mode option to use when rendering this GeometryBuffer.

    \note
        The blend mode setting is not a 'state' setting, but is used for \e all
        geometry added to the buffer regardless of when the blend mode is set.

    \param mode
        One of the BlendMode enumerated values indicating the blending mode to
        be used.
    */
    virtual void setBlendMode(const BlendMode mode);

    /*!
    \brief
        Return the blend mode that is set to be used for this GeometryBuffer.

    \return
        One of the BlendMode enumerated values indicating the blending mode
        that will be used when rendering all geometry added to this
        GeometryBuffer object.
    */
    virtual BlendMode getBlendMode() const;

    /*!
    \brief
        Set whether clipping will be active for subsequently added vertices.

    \param active
        - true if vertices added after this call should be clipped to the
          clipping region defined for this GeometryBuffer.
        - false if vertices added after this call should not be clipped
          (other than to the edges of rendering target.
    */
    virtual void setClippingActive(const bool active) = 0;

    /*
    \brief
        Return whether clipping will be used for the current batch
        of vertices being defined.

    \return
        - true if vertices subsequently added to the GeometryBuffer will
          be clipped to the clipping region defined for this GeometryBuffer.
        - false if vertices subsequently added will not be clippled (other than
          to the edges of the rendering target).
    */
    virtual bool isClippingActive() const = 0;

protected:
    //! Constructor.
    GeometryBuffer();

    //! The BlendMode to use when rendering this GeometryBuffer.
    BlendMode d_blendMode;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIGeometryBuffer_h_
