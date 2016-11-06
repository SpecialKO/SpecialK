/***********************************************************************
    created:    Tue May 31 2005
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
#ifndef _CEGUIUDim_h_
#define _CEGUIUDim_h_

#include "CEGUI/Base.h"
#include <ostream>

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

// some macros to aid in the creation of UDims
#define cegui_absdim(x)     CEGUI::UDim(0,(x))
#define cegui_reldim(x)     CEGUI::UDim((x),0)


// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Dimension that has both a relative 'scale' portion and and absolute 'offset' portion

\par
    The meaning of this class depends on where you use it, it is just a
    float tuple. We will only describe the usual usecase for CEGUI::Window
    position and size. CEGUI::Window gets that behavior from CEGUI::Element.

    UDims are used for both positioning and sizing (hence the name dimension).

    The scale component describes the amount of dimension we take from the
    parent CEGUI::Element. The offset component describes the amount of pixels
    and is not dependent on any element in the tree.

\par Example: 50% of parent's width
    \code{.cpp}
    UDim(0.5f, 0);
    \endcode

\par Example: 100px
    \code{.cpp}
    UDim(0, 100.0f);
    \endcode

\par Negative components
    You can use negative components in UDim and they are in fact very useful!
    \code{.cpp}
    UDim(1.0f, -100.0f); // 100px to the left from the right edge of parent widget
    \endcode

\par Floating point
    Both components are floats, that means that expressing fractions of pixels
    is allowed even though such construct goes against the definition of pixel.
    \code{.cpp}
    UDim(0.0f, 1.123456f); // perfectly valid UDim
    \endcode

    Keep in mind that if you use such dimensions they will likely get pixel
    aligned before rendering. You can disable such behavior via
    Element::setPixelAligned.

\see CEGUI::UVector2
\see CEGUI::USize
\see CEGUI::URect
*/
class CEGUIEXPORT UDim :
    public AllocatedObject<UDim>
{
public:
    inline UDim()
    {}
    
    inline UDim(float scale, float offset):
        d_scale(scale),
        d_offset(offset)
    {}
        
    inline UDim(const UDim& v):
        d_scale(v.d_scale),
        d_offset(v.d_offset)
    {}

    inline UDim operator+(const UDim& other) const
    {
        return UDim(d_scale + other.d_scale, d_offset + other.d_offset);
    }
    
    inline UDim operator-(const UDim& other) const
    {
        return UDim(d_scale - other.d_scale, d_offset - other.d_offset);
    }
    
    inline UDim operator*(const float val) const
    {
        return UDim(d_scale * val, d_offset * val);
    }
    
    inline friend UDim operator*(const float val, const UDim& u)
    {
        return UDim(val * u.d_scale, val * u.d_offset);
    }
    
    inline UDim operator*(const UDim& other) const
    {
        return UDim(d_scale * other.d_scale, d_offset * other.d_offset);
    }
    
    inline UDim operator/(const UDim& other) const
    {
        // division by zero sets component to zero.  Not technically correct
        // but probably better than exceptions and/or NaN values.
        return UDim(other.d_scale == 0.0f ? 0.0f : d_scale / other.d_scale,
                    other.d_offset == 0.0f ? 0.0f : d_offset / other.d_offset);
    }

    inline const UDim& operator+=(const UDim& other)
    {
        d_scale += other.d_scale;
        d_offset += other.d_offset;
        return *this;
    }
    
    inline const UDim& operator-=(const UDim& other)
    {
        d_scale -= other.d_scale;
        d_offset -= other.d_offset;
        return *this;
    }
    
    inline const UDim& operator*=(const UDim& other)
    {
        d_scale *= other.d_scale;
        d_offset *= other.d_offset;
        return *this;
    }
    
    inline const UDim& operator/=(const UDim& other)
    {
        // division by zero sets component to zero.  Not technically correct
        // but probably better than exceptions and/or NaN values.
        d_scale = (other.d_scale == 0.0f ? 0.0f : d_scale / other.d_scale);
        d_offset = (other.d_offset == 0.0f ? 0.0f : d_offset / other.d_offset);
        return *this;
    }

    inline bool operator==(const UDim& other) const
    {
        return d_scale == other.d_scale && d_offset == other.d_offset;
    }
    
    inline bool operator!=(const UDim& other) const
    {
        return !operator==(other);
    }
    
    /*!
    \brief allows writing the udim to std ostream
    */
    inline friend std::ostream& operator << (std::ostream& s, const UDim& v)
    {
        s << "CEGUI::UDim(" << v.d_scale << ", " << v.d_offset << ")";
        return s;
    }
    
    /*!
    \brief finger saving convenience method returning UDim(0, 0)
    */
    inline static UDim zero()
    {
        return UDim(0.0f, 0.0f);
    }

    /*!
    \brief finger saving convenience method returning UDim(1, 0)
    
    \note
        Allows quite neat 0.5 * UDim::relative() self documenting syntax
    */
    inline static UDim relative()
    {
        return UDim(1.0f, 0.0f);
    }
    
    /*!
    \brief finger saving convenience method returning UDim(0.01, 0)
    
    \note
        Allows quite neat 50 * UDim::percent() self documenting syntax
    */
    inline static UDim percent()
    {
        return UDim(0.01f, 0.0f);
    }
    
    /*!
    \brief finger saving convenience method returning UDim(0, 1)
    
    \note
        Allows quite neat 100 * UDim::px() self documenting syntax,
        you can combine it with UDim::relative() as well (using operator+)
    */
    inline static UDim px()
    {
        return UDim(0.0f, 1.0f);
    }
    
    float d_scale;
    float d_offset;
};

/*!
\brief
    Class encapsulating the 'Unified Box' - this is usually used for margin

\par
    top, left, right and bottom represent offsets on each edge

\note
    Name taken from W3 'box model'
*/
class CEGUIEXPORT UBox :
    public AllocatedObject<UBox>
{
public:
    UBox():
            d_top(),
            d_left(),
            d_bottom(),
            d_right()
    {}

    UBox(const UDim& margin):
            d_top(margin),
            d_left(margin),
            d_bottom(margin),
            d_right(margin)
    {}

    UBox(const UDim& top, const UDim& left, const UDim& bottom, const UDim& right):
            d_top(top),
            d_left(left),
            d_bottom(bottom),
            d_right(right)
    {}

    UBox(const UBox& b):
            d_top(b.d_top),
            d_left(b.d_left),
            d_bottom(b.d_bottom),
            d_right(b.d_right)
    {}

    /*************************************************************************
        Operators
    *************************************************************************/
    bool operator==(const UBox& rhs) const
    {
        return ((d_top == rhs.d_top) &&
                (d_left == rhs.d_left) &&
                (d_bottom == rhs.d_bottom) &&
                (d_right == rhs.d_right));
    }

    bool operator!=(const UBox& rhs) const
    {
        return !operator==(rhs);
    }

    UBox& operator=(const UBox& rhs)
    {
        d_top = rhs.d_top;
        d_left = rhs.d_left;
        d_bottom = rhs.d_bottom;
        d_right = rhs.d_right;

        return *this;
    }
    
    UBox operator*(const float val) const
    {
        return UBox(
                   d_top * val, d_left * val,
                   d_bottom * val, d_right * val);
    }

    UBox operator*(const UDim& dim) const
    {
        return UBox(
                   d_top * dim, d_left * dim,
                   d_bottom * dim, d_right * dim);
    }

    UBox operator+(const UBox& b) const
    {
        return UBox(
                   d_top + b.d_top, d_left + b.d_left,
                   d_bottom + b.d_bottom, d_right + b.d_right);
    }

    /*************************************************************************
        Data Fields
    *************************************************************************/
    UDim d_top;
    UDim d_left;
    UDim d_bottom;
    UDim d_right;
};

/*!
* \brief allows you to get UDim(0, 0) if you pass UDim or just 0 if you pass anything else
* 
* \todo Is this the right place where to put this?
*/
template<typename T>
inline T TypeSensitiveZero()
{
    return T(0);
}

template<>
inline UDim TypeSensitiveZero<UDim>()
{
    return UDim(0, 0);
}

/*!
* \brief allows you to get UDim::relative() if you pass UDim or just 1 if you pass anything else
* 
* \todo Is this the right place where to put this?
*/
template<typename T>
inline T TypeSensitiveOne()
{
    return T(1);
}

template<>
inline UDim TypeSensitiveOne<UDim>()
{
    return UDim::relative();
}

} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif  // end of guard _CEGUIUDim_h_

