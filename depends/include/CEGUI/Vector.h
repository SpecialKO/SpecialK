/***********************************************************************
	created:	13/2/2011
	author:		Martin Preisler (reworked from code by Paul D Turner)
	
	purpose:	Defines interfaces for Vector classes
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
#ifndef _CEGUIVector_h_
#define _CEGUIVector_h_

#include "CEGUI/UDim.h"
#include <typeinfo>
#include <ostream>

// Start of CEGUI namespace section
namespace CEGUI
{

/*!
\brief
    Class used as a two dimensional vector (aka a Point)

\par
    This class is templated now, this allows us to use it as a Vector2 of floats,
    ints or even UDims without replicating the code all over the place.

\par
    For a simple Vector2 of floats (what was called Vector2 before), use Vector2f
    as the T template parameter defaults to float to save fingertips.
*/
template<typename T>
class Vector2:
    public AllocatedObject<Vector2<T> >
{
public:
    typedef T value_type;

    inline Vector2()
    {}
    
    inline Vector2(const T x, const T y):
        d_x(x),
        d_y(y)
    {}

    inline Vector2(const Vector2& v):
        d_x(v.d_x),
        d_y(v.d_y)
    {}

    inline Vector2& operator*=(const Vector2& vec)
    {
        d_x *= vec.d_x;
        d_y *= vec.d_y;

        return *this;
    }

    inline Vector2& operator/=(const Vector2& vec)
    {
        d_x /= vec.d_x;
        d_y /= vec.d_y;

        return *this;
    }

    inline Vector2& operator+=(const Vector2& vec)
    {
        d_x += vec.d_x;
        d_y += vec.d_y;

        return *this;
    }

    inline Vector2& operator-=(const Vector2& vec)
    {
        d_x -= vec.d_x;
        d_y -= vec.d_y;

        return *this;
    }

    inline Vector2 operator+(const Vector2& vec) const
    {
        return Vector2(d_x + vec.d_x, d_y + vec.d_y);
    }

    inline Vector2 operator-(const Vector2& vec) const
    {
        return Vector2(d_x - vec.d_x, d_y - vec.d_y);
    }

    inline Vector2 operator*(const Vector2& vec) const
    {
        return Vector2(d_x * vec.d_x, d_y * vec.d_y);
    }

    inline Vector2 operator/(const Vector2& vec) const
    {
        return Vector2(d_x / vec.d_x, d_y / vec.d_y);
    }

    inline Vector2 operator*(const T c) const
    {
        return Vector2(d_x * c, d_y * c);
    }

    inline Vector2& operator*=(const T c)
    {
        d_x *= c;
        d_y *= c;

        return *this;
    }

    inline Vector2 operator/(const T c) const
    {
        return Vector2(d_x / c, d_y / c);
    }

    inline bool operator==(const Vector2& vec) const
    {
        return ((d_x == vec.d_x) && (d_y == vec.d_y));
    }

    inline bool operator!=(const Vector2& vec) const
    {
        return !(operator==(vec));
    }
    
    /*!
    \brief allows writing the vector2 to std ostream
    */
    inline friend std::ostream& operator << (std::ostream& s, const Vector2& v)
    {
        s << "CEGUI::Vector2<" << typeid(T).name() << ">(" << v.d_x << ", " << v.d_y << ")";
        return s;
    }

    //! \brief finger saving alias for Vector2(0, 0)
    inline static Vector2 zero()
    {
        return Vector2(TypeSensitiveZero<T>(), TypeSensitiveZero<T>());
    }

    //! \brief finger saving alias for Vector2(1, 1)
    inline static Vector2 one()
    {
        return Vector2(TypeSensitiveOne<T>(), TypeSensitiveOne<T>());
    }
    
    //! \brief finger saving alias for Vector2(1, 0)
    inline static Vector2 one_x()
    {
        return Vector2(TypeSensitiveOne<T>(), TypeSensitiveZero<T>());
    }
    
    //! \brief finger saving alias for Vector2(0, 1)
    inline static Vector2 one_y()
    {
        return Vector2(TypeSensitiveZero<T>(), TypeSensitiveOne<T>());
    }

    T d_x;
    T d_y;
};

// the main reason for this is to keep C++ API in sync with other languages
typedef Vector2<float> Vector2f;

// we need to allow UVector2 to be multiplied by floats, this is the most elegant way to do that
inline Vector2<UDim> operator * (const Vector2<UDim>& v, const float c)
{
    return Vector2<UDim>(v.d_x * c, v.d_y * c);
}

typedef Vector2<UDim> UVector2;

/*!
\brief
    Class used as a three dimensional vector

\par
    This class is templated now, this allows us to use it as a Vector3 of floats,
    ints or even UDims without replicating the code all over the place.

\par
    For a simple Vector3 of floats (what was called Vector3 before), use Vector3f
    as the T template parameter defaults to float to save fingertips.
*/
template<typename T>
class Vector3:
    public AllocatedObject<Vector3<T> >
{
public:
    typedef T value_type;

    inline Vector3()
    {}

    inline Vector3(const T x, const T y, const T z):
        d_x(x),
        d_y(y),
        d_z(z)
    {}

    inline explicit Vector3(const Vector2<T>& v, const T z):
        d_x(v.d_x),
        d_y(v.d_y),
        d_z(z)
    {}

    inline Vector3(const Vector3& v):
        d_x(v.d_x),
        d_y(v.d_y),
        d_z(v.d_z)
    {}

    inline bool operator==(const Vector3& vec) const
    {
        return ((d_x == vec.d_x) && (d_y == vec.d_y) && (d_z == vec.d_z));
    }

    inline bool operator!=(const Vector3& vec) const
    {
        return !(operator==(vec));
    }

    inline Vector3 operator*(const T c) const
    {
        return Vector3(d_x * c, d_y * c, d_z * c);
    }

    inline Vector3 operator+(const Vector3& v) const
    {
        return Vector3(d_x + v.d_x, d_y + v.d_y, d_z + v.d_z);
    }

    inline Vector3 operator-(const Vector3& v) const
    {
        return Vector3(d_x - v.d_x, d_y - v.d_y, d_z - v.d_z);
    }
    
    /*!
    \brief allows writing the vector3 to std ostream
    */
    inline friend std::ostream& operator << (std::ostream& s, const Vector3& v)
    {
        s << "CEGUI::Vector3<" << typeid(T).name() << ">(" << v.d_x << ", " << v.d_y << ", " << v.d_z << ")";
        return s;
    }

    //! \brief finger saving alias for Vector3(0, 0, 0)
    inline static Vector3 zero()
    {
        return Vector3(TypeSensitiveZero<T>(), TypeSensitiveZero<T>(), TypeSensitiveZero<T>());
    }

    //! \brief finger saving alias for Vector3(1, 1, 1)
    inline static Vector3 one()
    {
        return Vector3(TypeSensitiveOne<T>(), TypeSensitiveOne<T>(), TypeSensitiveOne<T>());
    }
    
    //! \brief finger saving alias for Vector3(1, 0, 0)
    inline static Vector3 one_x()
    {
        return Vector3(TypeSensitiveOne<T>(), TypeSensitiveZero<T>(), TypeSensitiveZero<T>());
    }
    
    //! \brief finger saving alias for Vector3(0, 1, 0)
    inline static Vector3 one_y()
    {
        return Vector3(TypeSensitiveZero<T>(), TypeSensitiveOne<T>(), TypeSensitiveZero<T>());
    }

    //! \brief finger saving alias for Vector3(0, 0, 1)
    inline static Vector3 one_z()
    {
        return Vector3(TypeSensitiveZero<T>(), TypeSensitiveZero<T>(), TypeSensitiveOne<T>());
    }
    
    T d_x;
    T d_y;
    T d_z;
};

// the main reason for this is to keep C++ API in sync with other languages
typedef Vector3<float> Vector3f;

} // End of  CEGUI namespace section

#endif	// end of guard _CEGUIVector_h_
