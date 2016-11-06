/***********************************************************************
    created:    Mon Jan 12 2009
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
#ifndef _CEGUIRenderingWindow_h_
#define _CEGUIRenderingWindow_h_

#include "CEGUI/RenderingSurface.h"
#include "CEGUI/Vector.h"
#include "CEGUI/Quaternion.h"
#include "CEGUI/Size.h"
#include "CEGUI/Rect.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    RenderingWindow is a RenderingSurface that can be "drawn back" onto another
    RenderingSurface and is primarily intended to be used as a kind of cache for
    rendered imagery.
*/
class CEGUIEXPORT RenderingWindow : public RenderingSurface
{
public:
    /*!
    \brief
        Constructor for RenderingWindow objects.

    \param target
        The TextureTarget based object that will be used as the target for
        content rendering done by the RenderingWindow.

    \param owner
        The RenderingSurface object that will be our initial owner.  This
        RenderingSurface is also the target where our cached imagery will be
        rendered back to.

    \note
        The TextureTarget \a target remains under it's original ownership, and
        the RenderingSurface \a owner actually owns \e this object.
    */
    RenderingWindow(TextureTarget& target, RenderingSurface& owner);

    //! Destructor for RenderingWindow objects.
    ~RenderingWindow();

    /*!
    \brief
        Set the clipping region that will be used when rendering the imagery
        for this RenderingWindow back onto the RenderingSurface that owns it.

    \note
        This is not the clipping region used when rendering the queued geometry
        \e onto the RenderingWindow, that still uses whatever regions are set
        on the queued GeometryBuffer objects.

    \param region
        Rect object describing a rectangular clipping region.

    \note
        The region should be described as absolute pixel locations relative to
        the screen or other root surface.  The region should \e not be described
        relative to the owner of the RenderingWindow.

    */
    void setClippingRegion(const Rectf& region);

    /*!
    \brief
        Set the two dimensional position of the RenderingWindow in pixels.  The
        origin is at the top-left corner.

    \param position
        Vector2 object describing the desired location of the RenderingWindow,
        in pixels.

    \note
        This position is an absolute pixel location relative to the screen or
        other root surface.  It is \e not relative to the owner of the
        RenderingWindow.
    */
    void setPosition(const Vector2f& position);

    /*!
    \brief
        Set the size of the RenderingWindow in pixels.

    \param size
        Size object that describes the desired size of the RenderingWindow, in
        pixels.
    */
    void setSize(const Sizef& size);

    /*!
    \brief
        Set the rotation quaternion to be used when rendering the RenderingWindow
        back onto it's owning RenderingSurface.

    \param rotation
        Quaternion object describing the rotation.
    */
    void setRotation(const Quaternion& rotation);

    /*!
    \brief
        Set the location of the pivot point around which the RenderingWindow
        will be rotated.

    \param pivot
        Vector3 describing the three dimensional point around which the
        RenderingWindow will be rotated.
    */
    void setPivot(const Vector3f& pivot);

    /*!
    \brief
        Return the current pixel position of the RenderingWindow.  The origin is
        at the top-left corner.

    \return
        Vector2 object describing the pixel position of the RenderingWindow.

    \note
        This position is an absolute pixel location relative to the screen or
        other root surface.  It is \e not relative to the owner of the
        RenderingWindow.
    */
    const Vector2f& getPosition() const;

    /*!
    \brief
        Return the current size of the RenderingWindow in pixels.

    \return
        Size object describing the current pixel size of the RenderingWindow.
    */
    const Sizef& getSize() const;

    /*!
    \brief
        Return the current rotation being applied to the RenderingWindow

    \return
        Quaternion object describing the rotation for the RenderingWindow.
    */
    const Quaternion& getRotation()const;

    /*!
    \brief
        Return the rotation pivot point location for the RenderingWindow.

    \return
        Vector3 object describing the current location of the pivot point used
        when rotating the RenderingWindow.
    */
    const Vector3f& getPivot() const;

    /*!
    \brief
        Return the TextureTarget object that is the target for content rendered
        to this RenderingWindows.  This is the same object passed into the
        constructor.

    \return
        The TextureTarget object that receives the rendered output resulting
        from geometry queued to this RenderingWindow.
    */
    const TextureTarget& getTextureTarget() const;
    TextureTarget& getTextureTarget();

    /*!
    \brief
        Peform time based updated for the RenderingWindow.

    \note
        Currently this really only has meaning for RenderingWindow objects that
        have RenderEffect objects set.  Though this may not always be the case.

    \param elapsed
        float value describing the number of seconds that have passed since the
        previous call to update.
    */
    void update(const float elapsed);

    /*!
    \brief
        Set the RenderEffect that should be used with the RenderingWindow.  This
        may be 0 to remove a previously set RenderEffect.

    \note
        Ownership of the RenderEffect does not change; the RenderingWindow will
        not delete a RenderEffect assigned to it when the RenderingWindow is
        destroyed.
    */
    void setRenderEffect(RenderEffect* effect);

    /*!
    \brief
        Return a pointer to the RenderEffect currently being used with the
        RenderingWindow.  A return value of 0 indicates that no RenderEffect
        is being used.

    \return
        Pointer to the RenderEffect used with this RenderingWindow, or 0 for
        none.
    */
    RenderEffect* getRenderEffect();

    /*!
    \brief
        generate geometry to be used when rendering back the RenderingWindow to
        it's owning RenderingSurface.

    \note
        In normal usage you should never have to call this directly.  There may
        be certain cases where it might be useful to call this when using the
        RenderEffect system.
    */
    void realiseGeometry();

    /*!
    \brief
        Mark the geometry used when rendering the RenderingWindow back to it's
        owning RenderingSurface as invalid so that it gets regenerated on the
        next rendering pass.

        This is separate from the main invalidate() function because in most
        cases invalidating the cached imagery will not require the potentially
        expensive regeneration of the geometry for the RenderingWindow itself.
    */
    void invalidateGeometry();

    /*!
    \brief
        Return the RenderingSurface that owns the RenderingWindow.  This is
        also the RenderingSurface that will be used when the RenderingWindow
        renders back it's cached imagery content.

    \return
        RenderingSurface object that owns, and is targetted by, the
        RenderingWindow.
    */
    const RenderingSurface& getOwner() const;
    RenderingSurface& getOwner();

    /*!
    \brief
        Fill in Vector2 object \a p_out with an unprojected version of the
        point described by Vector2 \a p_in.
    */
    void unprojectPoint(const Vector2f& p_in, Vector2f& p_out);

    // overrides from base
    void draw();
    void invalidate();
    bool isRenderingWindow() const;

protected:
    //! default generates geometry to draw window as a single quad.
    virtual void realiseGeometry_impl();

    //! set a new owner for this RenderingWindow object
    void setOwner(RenderingSurface& owner);
    // friend is so that RenderingSurface can call setOwner to xfer ownership.
    friend void RenderingSurface::transferRenderingWindow(RenderingWindow&);

    //! holds ref to renderer
    Renderer& d_renderer;
    //! TextureTarget to draw to. Like d_target in base, but avoiding downcasts.
    TextureTarget& d_textarget;
    //! RenderingSurface that owns this object, we render back to this object.
    RenderingSurface* d_owner;
    //! GeometryBuffer that holds geometry for drawing this window.
    GeometryBuffer* d_geometry;
    //! indicates whether data in GeometryBuffer is up-to-date
    bool d_geometryValid;
    //! Position of this RenderingWindow
    Vector2f d_position;
    //! Size of this RenderingWindow
    Sizef d_size;
    //! Rotation for this RenderingWindow
    Quaternion d_rotation;
    //! Pivot point used for the rotation.
    Vector3f d_pivot;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif  // end of guard _CEGUIRenderingWindow_h_
