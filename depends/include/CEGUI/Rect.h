/***********************************************************************
	created:	14/2/2011
	author:		Martin Preisler (reworked from code by Paul D Turner)
	
	purpose:	Defines 'Rect' class
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
#ifndef _CEGUIRect_h_
#define _CEGUIRect_h_

#include "CEGUI/Vector.h"
#include "CEGUI/Size.h"

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
	Class encapsulating operations on a Rectangle
*/
template<typename T>
class Rect:
    public AllocatedObject<Rect<T> >
{
public:
    typedef T value_type;

	inline Rect()
    {}

	inline Rect(const T& left, const T& top, const T& right, const T& bottom):
        d_min(left, top),
        d_max(right, bottom)
    {}

    inline Rect(const Vector2<T>& min, const Vector2<T>& max):
        d_min(min),
        d_max(max)
    {}

    inline Rect(const Vector2<T>& pos, const Size<T>& size):
        d_min(pos),
        d_max(pos + Vector2<T>(size.d_width, size.d_height))
    {}

    inline Rect(const Rect& r):
        d_min(r.d_min),
        d_max(r.d_max)
    {}

    inline Rect& operator=(const Rect& rhs)
    {
        d_min = rhs.d_min;
        d_max = rhs.d_max;

        return *this;
    }

    inline void left(const T& v)
    {
        d_min.d_x = v;
    }

    inline const T& left() const
    {
        return d_min.d_x;
    }

    inline void top(const T& v)
    {
        d_min.d_y = v;
    }

    inline const T& top() const
    {
        return d_min.d_y;
    }

    inline void right(const T& v)
    {
        d_max.d_x = v;
    }

    inline const T& right() const
    {
        return d_max.d_x;
    }

    inline void bottom(const T& v)
    {
        d_max.d_y = v;
    }

    inline const T& bottom() const
    {
        return d_max.d_y;
    }

	/*!
	\brief
		set the position of the Rect (leaves size in tact)
	*/
	void setPosition(const Vector2<T>& min)
    {
        const Size<T> size = getSize();
        d_min = min;
        setSize(size);
    }

    /*!
	\brief
		Return top-left position of Rect as a Vector2<T>
	*/
	const Vector2<T>& getPosition() const
    {
        return d_min;
    }

    void setSize(const Size<T>& size)
    {
        d_max = d_min + Vector2<T>(size.d_width, size.d_height);
    }

    /*!
	\brief
		return the size of the Rect area
	*/
	inline Size<T> getSize() const
    {
        return Size<T>(getWidth(), getHeight());
    }

    void setWidth(const T& w)
    {
        d_max.d_x = d_min.d_x + w;
    }

	/*!
	\brief
		return width of Rect area
	*/
	inline T getWidth() const
    {
        return d_max.d_x - d_min.d_x;
    }

    void setHeight(const T& h)
    {
        d_max.d_y = d_min.d_y + h;
    }

	/*!
	\brief
		return height of Rect area
	*/
	inline T getHeight() const
    {
        return d_max.d_y - d_min.d_y;
    }

	/*!
	\brief
		return a Rect that is the intersection of 'this' Rect with the Rect 'rect'

	\note
		It can be assumed that if d_left == d_right, or d_top == d_bottom, or getWidth() == 0, or getHeight() == 0, then
		'this' rect was totally outside 'rect'.
	*/
	inline Rect getIntersection(const Rect& rect) const
    {
        if ((d_max.d_x > rect.d_min.d_x) &&
		    (d_min.d_x < rect.d_max.d_x) &&
		    (d_max.d_y > rect.d_min.d_y) &&
		    (d_min.d_y < rect.d_max.d_y))
	    {
		    Rect ret;

		    // fill in ret with the intersection
		    ret.d_min.d_x = (d_min.d_x > rect.d_min.d_x) ? d_min.d_x : rect.d_min.d_x;
		    ret.d_max.d_x = (d_max.d_x < rect.d_max.d_x) ? d_max.d_x : rect.d_max.d_x;
		    ret.d_min.d_y = (d_min.d_y > rect.d_min.d_y) ? d_min.d_y : rect.d_min.d_y;
		    ret.d_max.d_y = (d_max.d_y < rect.d_max.d_y) ? d_max.d_y : rect.d_max.d_y;

		    return ret;
	    }
	    else
	    {
		    return Rect(0.0f, 0.0f, 0.0f, 0.0f);
	    }
    }

	/*!
	\brief
		Applies an offset the Rect object

	\param pt
		Vector2 object containing the offsets to be applied to the Rect.

	\return
		this Rect after the offset is performed
	*/
	inline void offset(const Vector2<T>& v)
    {
        d_min += v;
        d_max += v;
    }

	/*!
	\brief
		Return true if the given Vector2 falls within this Rect

	\param pt
		Vector2 object describing the position to test.

	\return
		true if position \a pt is within this Rect's area, else false
	*/
	inline bool isPointInRect(const Vector2<T>& v) const
    {
		if ((d_min.d_x >  v.d_x) ||
		    (d_max.d_x <= v.d_x) ||
		    (d_min.d_y >  v.d_y) ||
		    (d_max.d_y <= v.d_y))
	    {
		    return false;
	    }

	    return true;
    }


	/*!
	\brief
		check the size of the Rect object and if it is bigger than \a sz, resize it so it isn't.

	\param sz
		Size object that describes the maximum dimensions that this Rect should be limited to.

	\return
		'this' Rect object after the constrain operation
	*/
	void constrainSizeMax(const Size<T>& size)
    {
        if (getWidth() > size.d_width)
	    {
		    setWidth(size.d_width);
	    }

	    if (getHeight() > size.d_height)
	    {
		    setHeight(size.d_height);
	    }
    }


	/*!
	\brief
		check the size of the Rect object and if it is smaller than \a sz, resize it so it isn't.

	\param sz
		Size object that describes the minimum dimensions that this Rect should be limited to.

	\return
		'this' Rect object after the constrain operation
	*/
	void constrainSizeMin(const Size<T>& size)
    {
        if (getWidth() < size.d_width)
	    {
		    setWidth(size.d_width);
	    }

	    if (getHeight() < size.d_height)
	    {
		    setHeight(size.d_height);
	    }
    }


	/*!
	\brief
		check the size of the Rect object and if it is bigger than \a max_sz or smaller than \a min_sz, resize it so it isn't.

	\param max_sz
		Size object that describes the maximum dimensions that this Rect should be limited to.

	\param min_sz
		Size object that describes the minimum dimensions that this Rect should be limited to.

	\return
		'this' Rect object after the constrain operation
	*/
	void constrainSize(const Size<T>& max_sz, const Size<T>& min_sz)
    {
        Size<T> curr_sz(getSize());

	    if (curr_sz.d_width > max_sz.d_width)
	    {
		    setWidth(max_sz.d_width);
	    }
	    else if (curr_sz.d_width < min_sz.d_width)
	    {
		    setWidth(min_sz.d_width);
	    }

	    if (curr_sz.d_height > max_sz.d_height)
	    {
		    setHeight(max_sz.d_height);
	    }
	    else if (curr_sz.d_height < min_sz.d_height)
	    {
		    setHeight(min_sz.d_height);
	    }
    }


	/*************************************************************************
		Operators
	*************************************************************************/
	inline bool operator==(const Rect& rhs) const
	{
		return ((d_min == rhs.d_min) && (d_max == rhs.d_max));
	}

	inline bool operator!=(const Rect& rhs) const
    {
        return !operator==(rhs);
    }

    inline Rect operator*(T scalar) const
    {
        return Rect(d_min * scalar, d_max * scalar);
    }

    const Rect& operator*=(T scalar)
    {
        d_min *= scalar;
        d_max *= scalar;
        return *this;
    }

	Rect operator+(const Rect& r) const
    {
        return Rect(d_min + r.d_min, d_max + r.d_max);
    }
    
    inline friend std::ostream& operator << (std::ostream& s, const Rect& v)
    {
        s << "CEGUI::Rect<" << typeid(T).name() << ">(" << v.d_min.d_x << ", " << v.d_min.d_y << ", " << v.d_max.d_x << ", " << v.d_max.d_y << ")";
        return s;
    }
    
    //! \brief finger saving alias for zero sized, zero positioned rect
    inline static Rect zero()
    {
        return Rect(Vector2<T>::zero(), Size<T>::zero());
    }
    
    /*************************************************************************
        Data Fields
    *************************************************************************/
    Vector2<T> d_min;
    Vector2<T> d_max;

    // d_min.d_x is former d_left
    // d_min.d_y is former d_top
    // d_max.d_x is former d_right
    // d_max.d_y is former d_bottom
};

// the main reason for this is to keep C++ API in sync with other languages
typedef Rect<float> Rectf;

// we need to allow URect to be multiplied by floats, this is the most elegant way to do that
inline Rect<UDim> operator * (const Rect<UDim>& v, const float c)
{
    return Rect<UDim>(v.d_min * c, v.d_max * c);
}

typedef Rect<UDim> URect;

} // End of  CEGUI namespace section


#endif	// end of guard _CEGUIRect_h_

