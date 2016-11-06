/***********************************************************************
	created:	14/3/2004
	author:		Paul D Turner
	
	purpose:	Defines interface for Size class
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
#ifndef _CEGUISize_h_
#define _CEGUISize_h_

#include "CEGUI/UDim.h"
#include "CEGUI/Vector.h"
#include <typeinfo>
#include <ostream>

// Start of CEGUI namespace section
namespace CEGUI
{

/*!
\brief
    How aspect ratio should be maintained
*/
enum AspectMode
{
    //! Ignores the target aspect (default)
    AM_IGNORE,
    /*!
    Satisfies the aspect ratio by shrinking the size as little
    as possible to fit inside it
    */
    AM_SHRINK,
    /*!
    Satisfies the aspect ratio by expanding the widget as little
    as possible outside it
    */
    AM_EXPAND
};

/*!
\brief
	Class that holds the size (width & height) of something.
*/
template<typename T>
class Size:
    public AllocatedObject<Size<T> >
{
public:
    typedef T value_type;

    inline Size()
    {}

    inline Size(const T width, const T height):
        d_width(width),
        d_height(height)
    {}

    inline Size(const Size& v):
        d_width(v.d_width),
        d_height(v.d_height)
    {}

    inline bool operator==(const Size& other) const
    {
        return d_width == other.d_width && d_height == other.d_height;
    }

    inline bool operator!=(const Size& other) const
    {
        return !operator==(other);
    }

    inline Size operator*(const T c) const
    {
        return Size(d_width * c, d_height * c);
    }

    inline Size operator*(const Size& s) const
    {
        return Size(d_width * s.d_width, d_height * s.d_height);
    }

    inline Size operator*(const Vector2f& vec) const
    {
        return Size(d_width * vec.d_x, d_height * vec.d_y);
    }

    inline Size operator+(const Size& s) const
    {
        return Size(d_width + s.d_width, d_height + s.d_height);
    }

    inline Size operator-(const Size& s) const
    {
        return Size(d_width - s.d_width, d_height - s.d_height);
    }

    inline void clamp(const Size& min, const Size& max)
    {
        assert(min.d_width <= max.d_width);
        assert(min.d_height <= max.d_height);
        
        if (d_width < min.d_width)
            d_width = min.d_width;
        else if (d_width > max.d_width)
            d_width = max.d_width;
        
        if (d_height < min.d_height)
            d_height = min.d_height;
        else if (d_height > max.d_height)
            d_height = max.d_height;
    }

    inline void scaleToAspect(AspectMode mode, T ratio)
    {
        if (mode == AM_IGNORE)
            return;

        if(d_width <= 0 && d_height <= 0)
            return;

        assert(ratio > 0);

        const T expectedWidth = d_height * ratio;
        const bool keepHeight = (mode == AM_SHRINK) ?
                expectedWidth <= d_width : expectedWidth >= d_width;

        if (keepHeight)
        {
            d_width = expectedWidth;
        }
        else
        {
            d_height = d_width / ratio;
        }
    }
    
    /*!
    \brief allows writing the size to std ostream
    */
    inline friend std::ostream& operator << (std::ostream& s, const Size& v)
    {
        s << "CEGUI::Size<" << typeid(T).name() << ">(" << v.d_width << ", " << v.d_height << ")";
        return s;
    }

    //! \brief finger saving alias for Size(side, side)
    inline static Size square(const T side)
    {
        return Size(side, side);
    }

    //! \brief finger saving alias for Size(0, 0)
    inline static Size zero()
    {
        return square(TypeSensitiveZero<T>());
    }
    
    //! \brief finger saving alias for Size(1, 1)
    inline static Size one()
    {
        return square(TypeSensitiveOne<T>());
    }
    
    //! \brief finger saving alias for Size(1, 0)
    inline static Size one_width()
    {
        return Size(TypeSensitiveOne<T>(), TypeSensitiveZero<T>());
    }
    
    //! \brief finger saving alias for Size(0, 1)
    inline static Size one_height()
    {
        return Size(TypeSensitiveOne<T>(), TypeSensitiveZero<T>());
    }

    T d_width;
    T d_height;
};

// the main reason for this is to keep C++ API in sync with other languages
typedef Size<float> Sizef;
typedef Size<UDim> USize;

inline USize operator*(const USize& i, float x)
{
    return i * UDim(x,x);
}

} // End of  CEGUI namespace section

#endif	// end of guard _CEGUISize_h_
