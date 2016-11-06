/***********************************************************************
    created:    20th February 2010
    author:     Lukas E Meindl

    purpose:    Header of the ColourPicker colour type classes
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
#ifndef CEGUI_COLOUR_PICKER_TYPES_H
#define CEGUI_COLOUR_PICKER_TYPES_H

#include "CEGUI/CommonDialogs/Module.h"
#include "CEGUI/Window.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif


namespace CEGUI
{

/*!
\brief
    Enum defining the ColourPicker Slider mode

\deprecated
    This enum is deprecated and has been marked for removal.
*/
enum ColourPickerSliderMode
{
    ColourPickerSliderMode_L,
    ColourPickerSliderMode_A,
    ColourPickerSliderMode_B
};

class CEGUI_COMMONDIALOGS_API Lab_Colour;
class CEGUI_COMMONDIALOGS_API RGB_Colour;
class CEGUI_COMMONDIALOGS_API HSV_Colour;

//! Class representing an RGB colour using unsigned chars
class CEGUI_COMMONDIALOGS_API RGB_Colour :
    public AllocatedObject<RGB_Colour>
{
public:
    RGB_Colour(unsigned char red, unsigned char green, unsigned char blue) :
        r(red), g(green), b(blue)
    {}

    RGB_Colour() :
        r(0), g(0), b(0)
    {}

    RGB_Colour(const Lab_Colour& colour);
    RGB_Colour(const HSV_Colour& colour);
    RGB_Colour(const CEGUI::Colour& colour);

    unsigned char   r;
    unsigned char   g;
    unsigned char   b;

    RGB_Colour operator*(const float& number) const;
    RGB_Colour operator+(const RGB_Colour& colour) const;
};

//! Class representing a Colour according to the L*a*b* standard
class CEGUI_COMMONDIALOGS_API Lab_Colour :
    public AllocatedObject<Lab_Colour>
{
public:
    Lab_Colour(float LValue, float aValue, float bValue) :
        L(LValue), a(aValue), b(bValue)
    {}

    Lab_Colour() :
        L(0.0f), a(0.0f), b(0.0f)
    {}

    Lab_Colour(const RGB_Colour& colour);
    Lab_Colour(const HSV_Colour& colour);
    Lab_Colour(const CEGUI::Colour& colour);


    float L;
    float a;
    float b;
};

//! Class representing an HSV (hue, saturation and value) colour using floats.
class CEGUI_COMMONDIALOGS_API HSV_Colour :
    public AllocatedObject<HSV_Colour>
{
public:
    HSV_Colour(float HValue, float SValue, float VValue) :
        H(HValue), S(SValue), V(VValue)
    {}

    HSV_Colour() :
        H(0.0f), S(0.0f), V(0.0f)
    {}

    HSV_Colour(const RGB_Colour& colour);
    HSV_Colour(const Lab_Colour& colour);
    HSV_Colour(const CEGUI::Colour& colour);

    float H;
    float S;
    float V;
};

}

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif

