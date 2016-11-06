/***********************************************************************
created:    20th February 2010
author:     Lukas E Meindl

purpose:    Header of the class used to provide conversions between
the ColourPicker colour types
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
#ifndef _CEGUIColourPickerConversions_h_
#define _CEGUIColourPickerConversions_h_

#include "CEGUI/CommonDialogs/Module.h"
#include "CEGUI/CommonDialogs/ColourPicker/Types.h"
#include "CEGUI/Colour.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

namespace CEGUI
{

class CEGUI_COMMONDIALOGS_API ColourPickerConversions
{
public:
    //! Function for converting an RGB_Colour to a Lab_Colour
    static Lab_Colour toLab(RGB_Colour colour);

    //! Function for converting an RGB_Colour to an HSV_Colour
    static HSV_Colour toHSV(RGB_Colour colour);

    //! Function for converting a Lab_Colour to an RGB_Colour
    static RGB_Colour toRGB(const Lab_Colour& colour);

    //! Function for converting a Lab_Colour to an RGB_Colour
    static RGB_Colour toRGB(float L, float a, float b);

    //! Function for converting a CEGUI Colour to an RGB_Colour
    static RGB_Colour toRGB(const CEGUI::Colour& colour);

    //! Function for converting a HSV to an RGB_Colour
    static RGB_Colour toRGB(const HSV_Colour& colour);

    //! Conversion from RGB_Colour to CEGUI::Colour
    static CEGUI::Colour toCeguiColour(const RGB_Colour& colourRGB);

    //! Function for calculating X based on the RGB components
    static float toX(unsigned char R, unsigned char G, unsigned char B);

    //! Function for calculating Y based on the RGB components
    static float toY(unsigned char R, unsigned char G, unsigned char B);

    //! Function for calculating Z based on the RGB components
    static float toZ(unsigned char R, unsigned char G, unsigned char B);

    //! Function for calculating X based on the RGB components
    static float toX(float R, float G, float B);

    //! Function for calculating Y based on the RGB components
    static float toY(float R, float G, float B);

    //! Function for calculating Z based on the RGB components
    static float toZ(float R, float G, float B);

    //! Function for calculating L (of CIELab) based on the Y components
    static float toL(float Y);

    //! Function for calculating a (of CIELab) based on the X and Y components
    static float toA(float X, float Y);

    //! Function for calculating b (of CIELab) based on the Y and Z components
    static float toB(float Y, float Z);

    //! Linear interpolation helper function for RGB_Colour
    static RGB_Colour linearInterpolationRGB(float interPolBalance,
        const RGB_Colour& start,
        const RGB_Colour& end);

    //! Linear interpolation helper function for Alpha
    static unsigned char linearInterpolationAlpha(float interPolBalance,
        unsigned char startAlpha,
        unsigned char endAlpha);

    //! Linear interpolation helper function for Lab_Colour
    static Lab_Colour linearInterpolationLab(float interPolBalance,
        const Lab_Colour& start,
        const Lab_Colour& end);

    //! Linear interpolation helper function for HSV_Colour
    static HSV_Colour linearInterpolationHSV(float interPolBalance,
        const HSV_Colour& start,
        const HSV_Colour& end);

private:
    //! Clamp function for clamping interpolation values between 0.0 and 1.0
    static void clampInterpolationValue(float& interPolBalance);

    //! Helper function for the conversion from XYZ to CIELab
    static float normCalc(float div);

    //! Helper function for the conversion from XYZ to CIELab
    static float XNormCalc(float X);

    //! Helper function for the conversion from XYZ to CIELab
    static float YNormCalc(float Y);

    //! Helper function for the conversion from XYZ to CIELab
    static float ZNormCalc(float Z);

    //! Helper function for clamping values
    static void clamp(float& value, float min_val, float max_val);

    //! Normed reference values for Lab calculations in accordance with CIELab (D65)
    static const float Xn;
    static const float Yn;
    static const float Zn;

    //! Reference value needed in the Lab calculations to check for low
    // values of P/Pn (P/Pn < LAB_COMPARE_VALUE_CONST)
    static const float LAB_COMPARE_VALUE_CONST;
};

}

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif


#endif

