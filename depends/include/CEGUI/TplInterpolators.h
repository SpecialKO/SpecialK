/***********************************************************************
    created:    23/11/2010
    author:     Martin Preisler

    purpose:    Provides templated finger saving interpolators
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
#ifndef _CEGUITplInterpolators_h_
#define _CEGUITplInterpolators_h_

#include "CEGUI/Base.h"
#include "CEGUI/Interpolator.h"
#include "CEGUI/PropertyHelper.h"

// Start of CEGUI namespace section
namespace CEGUI
{

class CEGUIEXPORT TplInterpolatorBase : public Interpolator
{
public:
    TplInterpolatorBase(const String& type):
        d_type(type)
    {}
    
    //! destructor
    virtual ~TplInterpolatorBase() {}
    
    //! \copydoc Interpolator::getType
    virtual const String& getType() const
    {
        return d_type;
    }
    
private:
    const String d_type;
};

/*!
 \brief Generic linear interpolator class
 
 This class works on a simple formula: result = val1 * (1.0f - position) + val2 * (position);
 You can only use it on types that have operator*(float) and operator+(T) overloaded!
 */
template<typename T>
class TplLinearInterpolator : public TplInterpolatorBase
{
public:
    typedef PropertyHelper<T> Helper;
    
    TplLinearInterpolator(const String& type):
        TplInterpolatorBase(type)
    {}
    
    //! destructor
    virtual ~TplLinearInterpolator() {}
    
    //! \copydoc Interpolator::interpolateAbsolute
    virtual String interpolateAbsolute(const String& value1,
                                       const String& value2,
                                       float position)
    {
        typename Helper::return_type val1 = Helper::fromString(value1);
        typename Helper::return_type val2 = Helper::fromString(value2);

        const T result = static_cast<const T>(val1 * (1.0f - position) + val2 * (position));

        return Helper::toString(result);
    }
    
    //! \copydoc Interpolator::interpolateRelative
    virtual String interpolateRelative(const String& base,
                                       const String& value1,
                                       const String& value2,
                                       float position)
    {
        typename Helper::return_type bas = Helper::fromString(base);
        typename Helper::return_type val1 = Helper::fromString(value1);
        typename Helper::return_type val2 = Helper::fromString(value2);

        const T result = static_cast<const T>(bas + (val1 * (1.0f - position) + val2 * (position)));

        return Helper::toString(result);
    }
    
    //! \copydoc Interpolator::interpolateRelativeMultiply
    virtual String interpolateRelativeMultiply(const String& base,
                                               const String& value1,
                                               const String& value2,
                                               float position)
    {
        typename Helper::return_type bas = Helper::fromString(base);
        typename PropertyHelper<float>::return_type val1 = PropertyHelper<float>::fromString(value1);
        typename PropertyHelper<float>::return_type val2 = PropertyHelper<float>::fromString(value2);

        const float mul = val1 * (1.0f - position) + val2 * (position);

        const T result = static_cast<const T>(bas * mul);

        return Helper::toString(result);
    }
};

/*!
 \brief Generic discrete interpolator class
 
 This class returns the value the position is closest to.
 You can only use it on any types (they must have a PropertyHelper of course).
 No requirements on operators 
 */
template<typename T>
class TplDiscreteInterpolator : public TplInterpolatorBase
{
public:
    typedef PropertyHelper<T> Helper;
    
    TplDiscreteInterpolator(const String& type):
        TplInterpolatorBase(type)
    {}
    
    //! destructor
    virtual ~TplDiscreteInterpolator() {}
    
    //! \copydoc Interpolator::interpolateAbsolute
    virtual String interpolateAbsolute(const String& value1,
                                       const String& value2,
                                       float position)
    {
        typename Helper::return_type val1 = Helper::fromString(value1);
        typename Helper::return_type val2 = Helper::fromString(value2);

        typename Helper::return_type result = position < 0.5 ? val1 : val2;;

        return Helper::toString(result);
    }
    
    //! \copydoc Interpolator::interpolateRelative
    virtual String interpolateRelative(const String& /*base*/,
                                       const String& value1,
                                       const String& value2,
                                       float position)
    {
        //typename Helper::return_type bas = Helper::fromString(base);
        typename Helper::return_type val1 = Helper::fromString(value1);
        typename Helper::return_type val2 = Helper::fromString(value2);
        
        typename Helper::return_type result = position < 0.5 ? val1 : val2;
        
        return Helper::toString(result);
    }
    
    //! \copydoc Interpolator::interpolateRelativeMultiply
    virtual String interpolateRelativeMultiply(const String& base,
                                               const String& /*value1*/,
                                               const String& /*value2*/,
                                               float /*position*/)
    {
        typename Helper::return_type bas = Helper::fromString(base);
        /*const float val1 = PropertyHelper<float>::fromString(value1);
        const float val2 = PropertyHelper<float>::fromString(value2);

        const float mul = val1 * (1.0f - position) + val2 * (position);*/

        // there is nothing we can do, we have no idea what operators T has overloaded
        return Helper::toString(bas);
    }
};

/*!
 \brief Generic discrete relative interpolator class
 
 This class returns the value the position is closest to. It is different to discrete
 interpolator in interpolateRelative. It adds the resulting value to the base value.
 
 You can use this on types that have operator+(T) overloaded
 */
template<typename T>
class TplDiscreteRelativeInterpolator : public TplDiscreteInterpolator<T>
{
public:
    typedef PropertyHelper<T> Helper;
    
    TplDiscreteRelativeInterpolator(const String& type):
        TplDiscreteInterpolator<T>(type)
    {}
    
    //! destructor
    virtual ~TplDiscreteRelativeInterpolator() {}
    
    //! \copydoc Interpolator::interpolateRelative
    virtual String interpolateRelative(const String& base,
                                       const String& value1,
                                       const String& value2,
                                       float position)
    {
        typename Helper::return_type bas = Helper::fromString(base);
        typename Helper::return_type val1 = Helper::fromString(value1);
        typename Helper::return_type val2 = Helper::fromString(value2);
        
        typename Helper::return_type result = bas + (position < 0.5 ? val1 : val2);
        
        return Helper::toString(result);
    }
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUITplInterpolators_h_
