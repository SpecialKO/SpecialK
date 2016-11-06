/***********************************************************************
    created:    Sun Sep 18 2005
    author:     Paul D Turner <paul@cegui.org.uk>
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2006 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUICoordConverter_h_
#define _CEGUICoordConverter_h_

#include "CEGUI/UDim.h"
#include "CEGUI/Vector.h"
#include "CEGUI/Size.h"
#include "CEGUI/Rect.h"

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Utility class that helps in converting various types of co-ordinate between
    absolute screen positions and positions offset from the top-left corner of
    a given Window object.
*/
class CEGUIEXPORT CoordConverter
{
public:
    /*!
    \brief
        Static method used to return a float value rounded to the nearest integer.

        This method is used throughout the library to ensure that elements are
        kept at integer pixel positions on the display if user wishes so.

    \param x
        Expression to be rounded to nearest whole number

    \return
        \a x after having been rounded
        
    \see Node::setPixelAligned
    */
    inline static float alignToPixels(float x)
    {
        return (float)(int)(( x ) + (( x ) > 0.0f ? 0.5f : -0.5f));
    }
    
    /*!
    \brief
        converts given UDim to absolute value
    */
    inline static float asAbsolute(const UDim& u, float base, bool pixelAlign = true)
    {
        return pixelAlign ? alignToPixels(base * u.d_scale + u.d_offset) : base * u.d_scale + u.d_offset;
    }

    /*!
    \brief
        converts given UDim to relative value
    */
    inline static float asRelative(const UDim& u, float base)
    {
        return (base != 0.0f) ? u.d_offset / base + u.d_scale : 0.0f;
    }

    /*!
    \brief
        converts given Vector2<UDim> to absolute Vector2f
    */
    inline static Vector2f asAbsolute(const Vector2<UDim>& v, const Sizef& base, bool pixelAlign = true)
    {
        return Vector2f(asAbsolute(v.d_x, base.d_width, pixelAlign), asAbsolute(v.d_y, base.d_height, pixelAlign));
    }

    /*!
    \brief
        converts given Vector2<UDim> to relative Vector2f
    */
    inline static Vector2f asRelative(const Vector2<UDim>& v, const Sizef& base)
    {
        return Vector2f(asRelative(v.d_x, base.d_width), asRelative(v.d_y, base.d_height));
    }

	/*!
    \brief
        converts given Size<UDim> to absolute Sizef
    */
    inline static Sizef asAbsolute(const Size<UDim>& v, const Sizef& base, bool pixelAlign = true)
    {
        return Sizef(asAbsolute(v.d_width, base.d_width, pixelAlign), asAbsolute(v.d_height, base.d_height, pixelAlign));
    }

    /*!
    \brief
        converts given Size<UDim> to relative Sizef
    */
    inline static Sizef asRelative(const Size<UDim>& v, const Sizef& base)
    {
        return Sizef(asRelative(v.d_width, base.d_width), asRelative(v.d_height, base.d_height));
    }

    inline static Rectf asAbsolute(const URect& r, const Sizef& base, bool pixelAlign = true)
    {
        return Rectf(
                   asAbsolute(r.d_min.d_x, base.d_width,  pixelAlign),
                   asAbsolute(r.d_min.d_y, base.d_height, pixelAlign),
                   asAbsolute(r.d_max.d_x, base.d_width,  pixelAlign),
                   asAbsolute(r.d_max.d_y, base.d_height, pixelAlign)
               );
    }

    inline static Rectf asRelative(const URect& r, const Sizef& base)
    {
        return Rectf(
                   asRelative(r.d_min.d_x, base.d_width),
                   asRelative(r.d_min.d_y, base.d_height),
                   asRelative(r.d_max.d_x, base.d_width),
                   asRelative(r.d_max.d_y, base.d_height)
               );
    }

    /*!
    \brief
        Convert a screen relative UDim co-ordinate value to a window co-ordinate
        value, specified in pixels.

    \param window
        Window object to use as a target for the conversion.

    \param x
        UDim x co-ordinate value to be converted

    \return
        float value describing a window co-ordinate value that is equivalent to
        screen UDim co-ordinate \a x.
    */
    static float screenToWindowX(const Window& window, const UDim& x);

    /*!
    \brief
        Convert a screen pixel co-ordinate value to a window co-ordinate
        value, specified in pixels.

    \param window
        Window object to use as a target for the conversion.

    \param x
        float x co-ordinate value to be converted.

    \return
        float value describing a window co-ordinate value that is equivalent to
        screen co-ordinate \a x.
    */
    static float screenToWindowX(const Window& window, const float x);

    /*!
    \brief
        Convert a screen relative UDim co-ordinate value to a window co-ordinate
        value, specified in pixels.

    \param window
        Window object to use as a target for the conversion.

    \param y
        UDim y co-ordinate value to be converted

    \return
        float value describing a window co-ordinate value that is equivalent to
        screen UDim co-ordinate \a y.
    */
    static float screenToWindowY(const Window& window, const UDim& y);

    /*!
    \brief
        Convert a screen pixel co-ordinate value to a window co-ordinate
        value, specified in pixels.

    \param window
        Window object to use as a target for the conversion.

    \param y
        UDim y co-ordinate value to be converted.

    \return
        float value describing a window co-ordinate value that is equivalent to
        screen co-ordinate \a y.
    */
    static float screenToWindowY(const Window& window, const float y);

    /*!
    \brief
        Convert a screen relative UVector2 point to a window co-ordinate point,
        specified in pixels.

    \param window
        Window object to use as a target for the conversion.

    \param vec
        UVector2 object describing the point to be converted

    \return
        Vector2 object describing a window co-ordinate point that is equivalent
        to screen based UVector2 point \a vec.
    */
    static Vector2f screenToWindow(const Window& window, const UVector2& vec);

    /*!
    \brief
        Convert a screen Vector2 pixel point to a window co-ordinate point,
        specified in pixels.

    \param window
        Window object to use as a target for the conversion.

    \param vec
        Vector2 object describing the point to be converted.

    \return
        Vector2 object describing a window co-ordinate point that is equivalent
        to screen based Vector2 point \a vec.
    */
    static Vector2f screenToWindow(const Window& window, const Vector2f& vec);

    /*!
    \brief
        Convert a URect screen area to a window area, specified in pixels.

    \param window
        Window object to use as a target for the conversion.

    \param rect
        URect object describing the area to be converted

    \return
        Rect object describing a window area that is equivalent to URect screen
        area \a rect.
    */
    static Rectf screenToWindow(const Window& window, const URect& rect);

    /*!
    \brief
        Convert a Rect screen pixel area to a window area, specified in pixels.

    \param window
        Window object to use as a target for the conversion.

    \param rect
        Rect object describing the area to be converted.

    \return
        Rect object describing a window area that is equivalent to Rect screen
        area \a rect.
    */
    static Rectf screenToWindow(const Window& window, const Rectf& rect);

private:
    //! disallows construction of this class
    CoordConverter();

    /*!
    \brief
        Return the base X co-ordinate of the given Window object.

    \param window
        Window object to return base position for.

    \return
        float value indicating the base on-screen pixel location of the window
        on the x axis (i.e. The screen co-ord of the window's left edge).
    */
    static float getBaseXValue(const Window& window);

    /*!
    \brief
        Return the base Y co-ordinate of the given Window object.

    \param window
        Window object to return base position for.

    \return
        float value indicating the base on-screen pixel location of the window
        on the y axis (i.e. The screen co-ord of the window's top edge).
    */
    static float getBaseYValue(const Window& window);

    /*!
    \brief
        Return the base position of the given Window object.

    \param window
        Window object to return base position for.

    \return
        Vector2 value indicating the base on-screen pixel location of the window
        object. (i.e. The screen co-ord of the window's top-left corner).
    */
    static Vector2f getBaseValue(const Window& window);
};

} // End of  CEGUI namespace section


#endif  // end of guard _CEGUICoordConverter_h_
