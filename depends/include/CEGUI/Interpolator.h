/***********************************************************************
    created:    7/8/2010
    author:     Martin Preisler

    purpose:    Defines the interface for the abstract Interpolator class
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
#ifndef _CEGUIInterpolator_h_
#define _CEGUIInterpolator_h_

#include "CEGUI/Base.h"
#include "CEGUI/String.h"

// Start of CEGUI namespace section
namespace CEGUI
{

/*!
\brief
    Defines a 'interpolator' class

    Interpolator allows you to interpolate between 2 properties.
    You can jut pass them as strings and Interpolator does everything for you.

    If you want to define your own interpolator, inherit this class and add it
    to AnimationManager via AnimationManager::addInterpolator to make it
    available for animations.

\see
    AnimationManager
*/
class CEGUIEXPORT Interpolator :
    public AllocatedObject<Interpolator>
{
public:
    //! destructor
    virtual ~Interpolator() {};

    //! returns type string of this interpolator
    virtual const String& getType() const = 0;

    // interpolate methods aren't const, because the interpolator could provide
    // some sort of caching mechanism if converting properties is too expensive
    // that it is worth it

    /** this is used when Affector is set to apply values in absolute mode
     * (application method == AM_Absolute)
     */
    virtual String interpolateAbsolute(const String& value1,
                                       const String& value2,
                                       float position) = 0;

    /** this is used when Affector is set to apply values in relative mode
     * (application method == AM_Relative)
     */
    virtual String interpolateRelative(const String& base,
                                       const String& value1,
                                       const String& value2,
                                       float position) = 0;

    /** this is used when Affector is set to apply values in relative multiply
     * mode (application method == AM_RelativeMultiply)
     */
    virtual String interpolateRelativeMultiply(const String& base,
            const String& value1,
            const String& value2,
            float position) = 0;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIInterpolator_h_

