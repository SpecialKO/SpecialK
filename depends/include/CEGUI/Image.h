/***********************************************************************
    created:    Wed Feb 16 2011
    author:     Paul D Turner <paul@cegui.org.uk>
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
#ifndef _CEGUIImage_h_
#define _CEGUIImage_h_

#include "CEGUI/ChainedXMLHandler.h"
#include "CEGUI/String.h"
#include "CEGUI/ColourRect.h"
#include "CEGUI/Rect.h"

// Start of CEGUI namespace section
namespace CEGUI
{

enum AutoScaledMode
{
    //! No auto scaling takes place
    ASM_Disabled,
    /*!
    Objects are auto scaled depending on their original height and current
    resolution height. Horizontal scaling is computed to keep aspect ratio.
    */
    ASM_Vertical,
    /*!
    Objects are auto scaled depending on their original width and current
    resolution width. Horizontal scaling is computed to keep aspect ratio.
    */
    ASM_Horizontal,
    /*!
    Objects are auto scaled by the smaller scaling factor of the two.
    */
    ASM_Min,
    /*!
    Objects are auto scaled by the larger scaling factor of the two.
    */
    ASM_Max,
    /*!
    Objects are auto scaled depending on their original size and current
    resolution. Both dimensions are scaled accordingly.

    This was the only auto scaling behavior available in 0.7 and it might
    break aspect ratios!
    */
    ASM_Both
};


template<>
class PropertyHelper<AutoScaledMode>
{
public:
    typedef AutoScaledMode return_type;
    typedef return_type safe_method_return_type;
    typedef AutoScaledMode pass_type;
    typedef String string_return_type;

    static const String& getDataTypeName()
    {
        static String type("AutoScaledMode");

        return type;
    }

    static return_type fromString(const String& str)
    {
        if (str == "vertical")
        {
            return ASM_Vertical;
        }
        else if (str == "horizontal")
        {
            return ASM_Horizontal;
        }
        else if (str == "min")
        {
            return ASM_Min;
        }
        else if (str == "max")
        {
            return ASM_Max;
        }
        else if (str == "true" || str == "True")
        {
            return ASM_Both;
        }
        else
        {
            return ASM_Disabled;
        }
    }

    static string_return_type toString(pass_type val)
    {
        if (val == ASM_Disabled)
        {
            return "false";
        }
        else if (val == ASM_Vertical)
        {
            return "vertical";
        }
        else if (val == ASM_Horizontal)
        {
            return "horizontal";
        }
        else if (val == ASM_Min)
        {
            return "min";
        }
        else if (val == ASM_Max)
        {
            return "max";
        }
        else if (val == ASM_Both)
        {
            return "true";
        }
        else
        {
            assert(false && "Invalid auto scaled mode");
            return "false";
        }
    }
};

/*!
\brief
    Interface for Image.

    In CEGUI, an Image is some object that can render itself into a given
    GeometryBuffer object.  This may be something as simple as a basic textured
    quad, or something more complex.
*/
class CEGUIEXPORT Image :
    public AllocatedObject<Image>,
    public ChainedXMLHandler
{
public:
    virtual ~Image();

    virtual const String& getName() const = 0;

    virtual const Sizef& getRenderedSize() const = 0;
    virtual const Vector2f& getRenderedOffset() const = 0;

    virtual void render(GeometryBuffer& buffer,
                        const Rectf& dest_area,
                        const Rectf* clip_area,
                        const ColourRect& colours) const = 0;

    virtual void notifyDisplaySizeChanged(const Sizef& size) = 0;

    // Standard Image::render overloads
    void render(GeometryBuffer& buffer,
                const Vector2f& position,
                const Rectf* clip_area = 0) const
    {
        const ColourRect colours(0XFFFFFFFF);
        render(buffer, Rectf(position, getRenderedSize()), clip_area, colours);
    }

    void render(GeometryBuffer& buffer,
                const Vector2f& position,
                const Rectf* clip_area,
                const ColourRect& colours) const
    {
        render(buffer, Rectf(position, getRenderedSize()), clip_area, colours);
    }

    void render(GeometryBuffer& buffer,
                const Vector2f& position,
                const Sizef& size,
                const Rectf* clip_area = 0) const
    {
        const ColourRect colours(0XFFFFFFFF);
        render(buffer, Rectf(position, size), clip_area, colours);
    }

    void render(GeometryBuffer& buffer,
                const Vector2f& position,
                const Sizef& size,
                const Rectf* clip_area,
                const ColourRect& colours) const
    {
        render(buffer, Rectf(position, size), clip_area, colours);
    }

    /*!
    \brief
        Helper able to compute scaling factors for auto scaling

    \note
        This is mostly for internal use, unless you know what you are doing,
        please don't touch this method!
     */
    static void computeScalingFactors(AutoScaledMode mode,
                                      const Sizef& display_size,
                                      const Sizef& native_display_size,
                                      float& x_scale,
                                      float& y_scale);

protected:
    // implement chained xml handler abstract interface
    void elementStartLocal(const String& element,
                           const XMLAttributes& attributes);
    void elementEndLocal(const String& element);
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIImage_h_

