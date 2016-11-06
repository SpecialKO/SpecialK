/***********************************************************************
    created:    Mon Jul 18 2005
    author:     Paul D Turner <paul@cegui.org.uk>
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2012 Paul D Turner & The CEGUI Development Team
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this software and associated documentation files (the
 *   "Software, to deal in the Software without restriction, including
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
#ifndef _CEGUIFalXMLEnumHelper_h_
#define _CEGUIFalXMLEnumHelper_h_

#include "../String.h"
#include "../Window.h"
#include "CEGUI/falagard/Enums.h"

namespace CEGUI
{
template<typename T>
class FalagardXMLHelper;

template<>
class CEGUIEXPORT FalagardXMLHelper<VerticalFormatting>
{
public:
    typedef VerticalFormatting return_type;
    typedef VerticalFormatting pass_type;

    static String toString(pass_type val)
    {
        switch (val)
        {
        case VF_BOTTOM_ALIGNED:
            return BottomAligned;

        case VF_CENTRE_ALIGNED:
            return CentreAligned;

        case VF_TILED:
            return Tiled;

        case VF_STRETCHED:
            return Stretched;

        default:
            return TopAligned;
        }
    }

    static return_type fromString(const String& str)
    {
        if (str == CentreAligned)
            return VF_CENTRE_ALIGNED;

        else if (str == BottomAligned)
            return VF_BOTTOM_ALIGNED;

        else if (str == Tiled)
            return VF_TILED;

        else if (str == Stretched)
            return VF_STRETCHED;

        else
            return VF_TOP_ALIGNED;
    }

    //! Definitions of the possible values represented as Strings
    static const CEGUI::String TopAligned;
    static const CEGUI::String CentreAligned;
    static const CEGUI::String BottomAligned;
    static const CEGUI::String Stretched;
    static const CEGUI::String Tiled;
};

template<>
class CEGUIEXPORT FalagardXMLHelper<HorizontalFormatting>
{
public:
    typedef HorizontalFormatting return_type;
    typedef HorizontalFormatting pass_type;

    static String toString(pass_type val)
    {
        switch (val)
        {
        case HF_RIGHT_ALIGNED:
            return RightAligned;

        case HF_CENTRE_ALIGNED:
            return CentreAligned;

        case HF_TILED:
            return Tiled;

        case HF_STRETCHED:
            return Stretched;

        default:
            return LeftAligned;
        }
    }

    static return_type fromString(const String& str)
    {
        if (str == CentreAligned)
            return HF_CENTRE_ALIGNED;

        else if (str == RightAligned)
            return HF_RIGHT_ALIGNED;

        else if (str == Tiled)
            return HF_TILED;

        else if (str == Stretched)
            return HF_STRETCHED;

        else
            return HF_LEFT_ALIGNED;
    }

    //! Definitions of the possible values represented as Strings
    static const CEGUI::String LeftAligned;
    static const CEGUI::String CentreAligned;
    static const CEGUI::String RightAligned;
    static const CEGUI::String Stretched;
    static const CEGUI::String Tiled;
};

template<>
class CEGUIEXPORT FalagardXMLHelper<VerticalAlignment>
{
public:
    typedef VerticalAlignment return_type;
    typedef VerticalAlignment pass_type;

    static String toString(pass_type val)
    {
        switch (val)
        {
        case VA_BOTTOM:
            return BottomAligned;

        case VA_CENTRE:
            return CentreAligned;

        default:
            return TopAligned;
        }
    }

    static return_type fromString(const String& str)
    {
        if (str == CentreAligned)
            return VA_CENTRE;

        else if (str == BottomAligned)
            return VA_BOTTOM;

        else
            return VA_TOP;
    }

    //! Definitions of the possible values represented as Strings
    static const CEGUI::String TopAligned;
    static const CEGUI::String CentreAligned;
    static const CEGUI::String BottomAligned;
};

template<>
class CEGUIEXPORT FalagardXMLHelper<HorizontalAlignment>
{
public:
    typedef HorizontalAlignment return_type;
    typedef HorizontalAlignment pass_type;

    static String toString(pass_type val)
    {
        switch (val)
        {
        case HA_RIGHT:
            return RightAligned;

        case HA_CENTRE:
            return CentreAligned;

        default:
            return LeftAligned;
        }
    }

    static return_type fromString(const String& str)
    {
        if (str == CentreAligned)
            return HA_CENTRE;

        else if (str == RightAligned)
            return HA_RIGHT;

        else
            return HA_LEFT;
    }

    //! Definitions of the possible values represented as Strings
    static const CEGUI::String LeftAligned;
    static const CEGUI::String CentreAligned;
    static const CEGUI::String RightAligned;
};

template<>
class CEGUIEXPORT FalagardXMLHelper<DimensionType>
{
public:
    typedef DimensionType return_type;
    typedef DimensionType pass_type;

    static String toString(pass_type val)
    {
        switch (val)
        {
        case DT_LEFT_EDGE:
            return LeftEdge;

        case DT_X_POSITION:
            return XPosition;

        case DT_TOP_EDGE:
            return TopEdge;

        case DT_Y_POSITION:
            return YPosition;

        case DT_RIGHT_EDGE:
            return RightEdge;

        case DT_BOTTOM_EDGE:
            return BottomEdge;

        case DT_WIDTH:
            return Width;

        case DT_HEIGHT:
            return Height;

        case DT_X_OFFSET:
            return XOffset;

        case DT_Y_OFFSET:
            return YOffset;

        default:
            return Invalid;
        }
    }

    static return_type fromString(const String& str)
    {
        if (str == LeftEdge)
            return DT_LEFT_EDGE;

        else if (str == XPosition)
            return DT_X_POSITION;

        else if (str == TopEdge)
            return DT_TOP_EDGE;

        else if (str == YPosition)
            return DT_Y_POSITION;

        else if (str == RightEdge)
            return DT_RIGHT_EDGE;

        else if (str == BottomEdge)
            return DT_BOTTOM_EDGE;

        else if (str == Width)
            return DT_WIDTH;

        else if (str == Height)
            return DT_HEIGHT;

        else if (str == XOffset)
            return DT_X_OFFSET;

        else if (str == YOffset)
            return DT_Y_OFFSET;

        else
            return DT_INVALID;
    }

    //! Definitions of the possible values represented as Strings
    static const CEGUI::String LeftEdge;
    static const CEGUI::String RightEdge;
    static const CEGUI::String BottomEdge;
    static const CEGUI::String TopEdge;
    static const CEGUI::String XPosition;
    static const CEGUI::String YPosition;
    static const CEGUI::String Width;
    static const CEGUI::String Height;
    static const CEGUI::String XOffset;
    static const CEGUI::String YOffset;
    static const CEGUI::String Invalid;
};

template<>
class CEGUIEXPORT FalagardXMLHelper<VerticalTextFormatting>
{
public:
    typedef VerticalTextFormatting return_type;
    typedef VerticalTextFormatting pass_type;

    static String toString(pass_type val)
    {
        switch (val)
        {
        case VTF_BOTTOM_ALIGNED:
            return BottomAligned;

        case VTF_CENTRE_ALIGNED:
            return CentreAligned;

        default:
            return TopAligned;
        }
    }

    static return_type fromString(const String& str)
    {
        if (str == CentreAligned)
            return VTF_CENTRE_ALIGNED;

        else if (str == BottomAligned)
            return VTF_BOTTOM_ALIGNED;

        else
            return VTF_TOP_ALIGNED;
    }

    //! Definitions of the possible values represented as Strings
    static const CEGUI::String TopAligned;
    static const CEGUI::String CentreAligned;
    static const CEGUI::String BottomAligned;
};

template<>
class CEGUIEXPORT FalagardXMLHelper<HorizontalTextFormatting>
{
public:
    typedef HorizontalTextFormatting return_type;
    typedef HorizontalTextFormatting pass_type;

    static String toString(pass_type val)
    {
        switch (val)
        {
        case HTF_RIGHT_ALIGNED:
            return RightAligned;

        case HTF_CENTRE_ALIGNED:
            return CentreAligned;

        case HTF_JUSTIFIED:
            return Justified;

        case HTF_WORDWRAP_LEFT_ALIGNED:
            return WordWrapLeftAligned;

        case HTF_WORDWRAP_RIGHT_ALIGNED:
            return WordWrapRightAligned;

        case HTF_WORDWRAP_CENTRE_ALIGNED:
            return WordWrapCentreAligned;

        case HTF_WORDWRAP_JUSTIFIED:
            return WordWrapJustified;

        default:
            return LeftAligned;
        }
    }

    static return_type fromString(const String& str)
    {
        if (str == CentreAligned)
            return HTF_CENTRE_ALIGNED;

        else if (str == RightAligned)
            return HTF_RIGHT_ALIGNED;

        else if (str == Justified)
            return HTF_JUSTIFIED;

        else if (str == WordWrapLeftAligned)
            return HTF_WORDWRAP_LEFT_ALIGNED;

        else if (str == WordWrapCentreAligned)
            return HTF_WORDWRAP_CENTRE_ALIGNED;

        else if (str == WordWrapRightAligned)
            return HTF_WORDWRAP_RIGHT_ALIGNED;

        else if (str == WordWrapJustified)
            return HTF_WORDWRAP_JUSTIFIED;

        else
            return HTF_LEFT_ALIGNED;
    }

    //! Definitions of the possible values represented as Strings
    static const CEGUI::String LeftAligned;
    static const CEGUI::String CentreAligned;
    static const CEGUI::String RightAligned;
    static const CEGUI::String Justified;
    static const CEGUI::String WordWrapLeftAligned;
    static const CEGUI::String WordWrapRightAligned;
    static const CEGUI::String WordWrapCentreAligned;
    static const CEGUI::String WordWrapJustified;
};

template<>
class CEGUIEXPORT FalagardXMLHelper<FontMetricType>
{
public:
    typedef FontMetricType return_type;
    typedef FontMetricType pass_type;

    static String toString(pass_type val)
    {
        switch (val)
        {
        case FMT_BASELINE:
            return Baseline;

        case FMT_HORZ_EXTENT:
            return HorzExtent;

        default:
            return LineSpacing;
        }
    }

    static return_type fromString(const String& str)
    {
        if (str == LineSpacing)
            return FMT_LINE_SPACING;

        else if (str == Baseline)
            return FMT_BASELINE;

        else
            return FMT_HORZ_EXTENT;
    }

    static const CEGUI::String LineSpacing;
    static const CEGUI::String HorzExtent;
    static const CEGUI::String Baseline;
};

template<>
class CEGUIEXPORT FalagardXMLHelper<DimensionOperator>
{
public:
    typedef DimensionOperator return_type;
    typedef DimensionOperator pass_type;

    static String toString(pass_type val)
    {
        switch (val)
        {
        case DOP_ADD:
            return Add;

        case DOP_SUBTRACT:
            return Subtract;

        case DOP_MULTIPLY:
            return Multiply;

        case DOP_DIVIDE:
            return Divide;

        default:
            return Noop;
        }
    }

    static return_type fromString(const String& str)
    {
        if (str == Add)
            return DOP_ADD;

        else if (str == Subtract)
            return DOP_SUBTRACT;

        else if (str == Multiply)
            return DOP_MULTIPLY;

        else if (str == Divide)
            return DOP_DIVIDE;

        else
            return DOP_NOOP;
    }

    static const CEGUI::String Add;
    static const CEGUI::String Subtract;
    static const CEGUI::String Multiply;
    static const CEGUI::String Divide;
    static const CEGUI::String Noop;
};

template<>
class CEGUIEXPORT FalagardXMLHelper<FrameImageComponent>
{
public:
    typedef FrameImageComponent return_type;
    typedef FrameImageComponent pass_type;

    static String toString(pass_type val)
    {
        switch (val)
        {
        case FIC_TOP_LEFT_CORNER:
            return TopLeftCorner;

        case FIC_TOP_RIGHT_CORNER:
            return TopRightCorner;

        case FIC_BOTTOM_LEFT_CORNER:
            return BottomLeftCorner;

        case FIC_BOTTOM_RIGHT_CORNER:
            return BottomRightCorner;

        case FIC_LEFT_EDGE:
            return LeftEdge;

        case FIC_RIGHT_EDGE:
            return RightEdge;

        case FIC_TOP_EDGE:
            return TopEdge;

        case FIC_BOTTOM_EDGE:
            return BottomEdge;

        default:
            return Background;
        }
    }

    static return_type fromString(const String& str)
    {
        if (str == TopLeftCorner)
            return FIC_TOP_LEFT_CORNER;

        if (str == TopRightCorner)
            return FIC_TOP_RIGHT_CORNER;

        if (str == BottomLeftCorner)
            return FIC_BOTTOM_LEFT_CORNER;

        if (str == BottomRightCorner)
            return FIC_BOTTOM_RIGHT_CORNER;

        if (str == LeftEdge)
            return FIC_LEFT_EDGE;

        if (str == RightEdge)
            return FIC_RIGHT_EDGE;

        if (str == TopEdge)
            return FIC_TOP_EDGE;

        if (str == BottomEdge)
            return FIC_BOTTOM_EDGE;

        else
            return FIC_BACKGROUND;
    }

    static const CEGUI::String TopLeftCorner;
    static const CEGUI::String TopRightCorner;
    static const CEGUI::String BottomLeftCorner;
    static const CEGUI::String BottomRightCorner;
    static const CEGUI::String TopEdge;
    static const CEGUI::String LeftEdge;
    static const CEGUI::String RightEdge;
    static const CEGUI::String BottomEdge;
    static const CEGUI::String Background;
};

template<>
class CEGUIEXPORT FalagardXMLHelper<ChildEventAction>
{
public:
    typedef ChildEventAction return_type;
    typedef ChildEventAction pass_type;

    static String toString(pass_type val)
    {
        switch (val)
        {
        case CEA_REDRAW:
            return Redraw;

        case CEA_LAYOUT:
            return Layout;

        default:
            CEGUI_THROW(InvalidRequestException(
                "Invalid enumeration value given."));
        }
    }

    static return_type fromString(const String& str)
    {
        if (str == Redraw)
            return CEA_REDRAW;

        if (str == Layout)
            return CEA_LAYOUT;

        CEGUI_THROW(InvalidRequestException(
            "'" + str +
            "' does not represent a ChildEventAction enumerated value."));
    }

    static const CEGUI::String Redraw;
    static const CEGUI::String Layout;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// PropertyHelper specialisations
//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template<>
class PropertyHelper<VerticalFormatting>
{
public:
    typedef VerticalFormatting return_type;
    typedef return_type safe_method_return_type;
    typedef VerticalFormatting pass_type;
    typedef String string_return_type;

    static const String& getDataTypeName()
    {
        static String type("VerticalFormatting");

        return type;
    }

    static return_type fromString(const String& str)
    {
        return FalagardXMLHelper<return_type>::fromString(str);
    }

    static string_return_type toString(pass_type val)
    {
        return FalagardXMLHelper<return_type>::toString(val);
    }
};

template<>
class PropertyHelper<HorizontalFormatting>
{
public:
    typedef HorizontalFormatting return_type;
    typedef return_type safe_method_return_type;
    typedef HorizontalFormatting pass_type;
    typedef String string_return_type;

    static const String& getDataTypeName()
    {
        static String type("HorizontalFormatting");

        return type;
    }

    static return_type fromString(const String& str)
    {
        return FalagardXMLHelper<return_type>::fromString(str);
    }

    static string_return_type toString(pass_type val)
    {
        return FalagardXMLHelper<return_type>::toString(val);
    }
};

template<>
class PropertyHelper<VerticalTextFormatting>
{
public:
    typedef VerticalTextFormatting return_type;
    typedef return_type safe_method_return_type;
    typedef VerticalTextFormatting pass_type;
    typedef String string_return_type;

    static const String& getDataTypeName()
    {
        static String type("VerticalTextFormatting");

        return type;
    }

    static return_type fromString(const String& str)
    {
        return FalagardXMLHelper<return_type>::fromString(str);
    }

    static string_return_type toString(pass_type val)
    {
        return FalagardXMLHelper<return_type>::toString(val);
    }
};

template<>
class PropertyHelper<HorizontalTextFormatting>
{
public:
    typedef HorizontalTextFormatting return_type;
    typedef return_type safe_method_return_type;
    typedef HorizontalTextFormatting pass_type;
    typedef String string_return_type;

    static const String& getDataTypeName()
    {
        static String type("HorizontalTextFormatting");

        return type;
    }

    static return_type fromString(const String& str)
    {
        return FalagardXMLHelper<return_type>::fromString(str);
    }

    static string_return_type toString(pass_type val)
    {
        return FalagardXMLHelper<return_type>::toString(val);
    }
};

}


#endif
