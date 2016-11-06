/***********************************************************************
	created:	2/1/2011
	author:		Martin Preisler
	
	purpose:	Defines interface for the Quaternion class
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
#ifndef _CEGUIQuaternion_h_
#define _CEGUIQuaternion_h_

#include "CEGUI/Base.h"
#include "CEGUI/Interpolator.h"
#include "CEGUI/Vector.h"
#include <cmath>

// Start of CEGUI namespace section
namespace CEGUI
{

/*!
\brief
    Class to represent rotation, avoids Gimbal lock

Most people are afraid of Quaternions, you don't have to fully understand them.
In CEGUI, you can just think of quaternions as magic opaque boxes that hold rotation
data. No need to understand how they work and why. You obviously have to
understand what degrees and radians are, I won't go into that here.

How to convert "human readable" rotation data to Quaternions:
1) Euler angles, 3 floating point values
    x - rotation around X axis (anticlockwise)
    y - rotation around Y axis (anticlockwise)
    z - rotation around Z axis (anticlockwise)
    
    The actual rotation is performed in z, y, x order. Keep that in mind!

    For these, just use eulerAnglesDegrees or eulerAnglesRadians static methods.

2) Rotation around axis, 1 Vector3 and one degree/radian angle
    the vector represents the axis (it's length doesn't matter at all),
    rotation is then performed by rotating given angle anticlockwise
    around that axis.

    For these, use axisAngleDegrees or axisAngleRadians static methods.
*/
class CEGUIEXPORT Quaternion :
    public AllocatedObject<Quaternion>
{
public:
    //! verbatim constructor
    inline Quaternion(float w = 1.0f, float x = 0.0f, float y = 0.0f, float z = 0.0f):
        d_w(w),
        d_x(x),
        d_y(y),
        d_z(z)
	{}

    //! copy constructor
    inline Quaternion(const Quaternion& v):
        d_w(v.d_w),
        d_x(v.d_x),
        d_y(v.d_y),
        d_z(v.d_z)
    {}

    //! assignment operator
    inline Quaternion& operator = (const Quaternion& v)
	{
        d_w = v.d_w;
        d_x = v.d_x;
        d_y = v.d_y;
        d_z = v.d_z;
        
        return *this;
	}

    /*!
    \brief constructs a quaternion from euler angles in radians

    \param x
        Anticlockwise rotation around X axis
    \param y
        Anticlockwise rotation around Y axis
    \param z
        Anticlockwise rotation around Z axis

    The rotation is performed around Z first, then Y and then X!
    */
    static Quaternion eulerAnglesRadians(float x, float y, float z);

    /*!
    \brief constructs a quaternion from euler angles in degrees

    \param x
        Anticlockwise rotation around X axis
    \param y
        Anticlockwise rotation around Y axis
    \param z
        Anticlockwise rotation around Z axis

    The rotation is performed around Z first, then Y and then X!
    */
    static Quaternion eulerAnglesDegrees(float x, float y, float z);

    /*!
    \brief constructs a quaternion from axis and angle around it in radians

    \param axis
        vector describing the axis of rotation
    \param rotation
        Anticlockwise rotation around given axis
    */
    static Quaternion axisAngleRadians(const Vector3f& axis, float rotation);

    /*!
    \brief constructs a quaternion from axis and angle around it in degrees

    \param axis
        vector describing the axis of rotation
    \param rotation
        Anticlockwise rotation around given axis
    */
    static Quaternion axisAngleDegrees(const Vector3f& axis, float rotation);

    //! equality operator
    inline bool operator == (const Quaternion& v) const
	{
		return (d_w == v.d_w) && (d_x == v.d_x) && (d_y == v.d_y) && (d_z == v.d_z);
	}

    //! inequality operator
    inline bool operator != (const Quaternion& v) const
	{
		return (d_w != v.d_w) || (d_x != v.d_x) || (d_y != v.d_y) || (d_z != v.d_z);
	}

    //! negation operator
    inline Quaternion operator - () const
    {
        return Quaternion(-d_w, -d_x, -d_y, -d_z);
    }

    //! scalar multiplication operator
    inline Quaternion operator * (float v) const
    {
        return Quaternion(d_w * v, d_x * v, d_y * v, d_z * v);
    }

    //! scalar multiplication operator
    inline friend Quaternion operator * (float v, const Quaternion& q)
    {
        return Quaternion(v * q.d_w, v * q.d_x, v * q.d_y, v * q.d_z);
    }

    //! quaternion dot product
    inline float dot(const Quaternion& v) const
    {
        return d_w * v.d_w + d_x * v.d_x + d_y * v.d_y + d_z * v.d_z;
    }

    //! addition operator
    inline Quaternion operator + (const Quaternion& v) const
    {
        return Quaternion(d_w + v.d_w, d_x + v.d_x, d_y + v.d_y, d_z + v.d_z);
    }

    /*!
    \brief quaternion multiplication (not commutative!)

    Lets say we have quaternion A describing a rotation and another quaternion B.
    If we write C = A * B, C is actually describing a rotation we would get if we
    rotated Identity by A and then rotated the result by B
    */
    inline Quaternion operator * (const Quaternion& v) const
    {
        return Quaternion(
            d_w * v.d_w - d_x * v.d_x - d_y * v.d_y - d_z * v.d_z,
            d_w * v.d_x + d_x * v.d_w + d_y * v.d_z - d_z * v.d_y,
            d_w * v.d_y + d_y * v.d_w + d_z * v.d_x - d_x * v.d_z,
            d_w * v.d_z + d_z * v.d_w + d_x * v.d_y - d_y * v.d_x
        );
    }

    /*!
    \brief computers and returns the length of this quaternion
    */
    inline float length() const
    {
        return sqrtf((d_w * d_w) + (d_x * d_x) + (d_y * d_y) + (d_z * d_z));
    }

    /*!
    \brief normalises this quaternion and returns it's length (since it has to be computed anyways)
    */
    inline float normalise()
    {
        const float len = length();
        const float factor = 1.0f / len;
        *this = *this * factor;

        return len;
    }

    /*!
    \brief spherical linear interpolation

    \param left
        Left keyframe Quaternion

    \param right
        Right keyframe Quaternion

    \param position
        Number from range <0.0, 1.0), the closer this is to 1.0, the closer the interpolation is to the "right" quaternion

    \param shortestPath
        If true, the interpolation is guaranteed to go through the shortest path
    */
    static Quaternion slerp(const Quaternion& left, const Quaternion& right, float position, bool shortestPath = false);

    //! Quaternion(0, 0, 0, 0)
    static const Quaternion ZERO;
    //! Quaternion(1, 0, 0, 0)
    static const Quaternion IDENTITY;
    
    /*!
    \brief allows writing the quaternion to std ostream
    */
    inline friend std::ostream& operator << (std::ostream& s, const Quaternion& v)
    {
        s << "CEGUI::Quaternion(" << v.d_w << ", " << v.d_x << ", " << v.d_y << ", " << v.d_z << ")";
        return s;
    }

    //! imaginary part
    float d_w;
    //! x component of the vector part
    float d_x;
    //! y component of the vector part
    float d_y;
    //! z component of the vector part
    float d_z;
};

/*!
 \brief Special interpolator class for Quaternion
 
 Quaternions can't be interpolated as floats and/or vectors, we have to use
 "Spherical linear interpolator" instead.
 */
class QuaternionSlerpInterpolator : public Interpolator
{
public:
    typedef PropertyHelper<Quaternion> Helper;
    
    //! destructor
    virtual ~QuaternionSlerpInterpolator() {}

    //! \copydoc Interpolator::getType
    virtual const String& getType() const;
    
    //! \copydoc Interpolator::interpolateAbsolute
    virtual String interpolateAbsolute(const String& value1,
                                       const String& value2,
                                       float position);
    
    //! \copydoc Interpolator::interpolateRelative
    virtual String interpolateRelative(const String& base,
                                       const String& value1,
                                       const String& value2,
                                       float position);
    
    //! \copydoc Interpolator::interpolateRelativeMultiply
    virtual String interpolateRelativeMultiply(const String& base,
                                               const String& value1,
                                               const String& value2,
                                               float position);
};

} // End of  CEGUI namespace section

#endif	// end of guard _CEGUIQuaternion_h_
