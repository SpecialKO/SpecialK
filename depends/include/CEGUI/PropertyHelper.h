/***********************************************************************
	created:	21/11/2010
	author:		Martin Preisler (reworked from code by Paul D Turner)
	
	purpose:	Interface to the PropertyHelper class
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
#ifndef _CEGUIPropertyHelper_h_
#define _CEGUIPropertyHelper_h_

#include "CEGUI/String.h"
#include "CEGUI/Size.h"
#include "CEGUI/Vector.h"
#include "CEGUI/Quaternion.h"
#include "CEGUI/Colour.h"
#include "CEGUI/ColourRect.h"
#include "CEGUI/UDim.h"
#include "CEGUI/Rect.h"


#include <cstdio>

#include <sstream>

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4996)
#endif

#ifdef _MSC_VER
    #define snprintf _snprintf
#endif

#ifdef __MINGW32__

    #if __USE_MINGW_ANSI_STDIO != 1
        #warning  __USE_MINGW_ANSI_STDIO must be set to 1 for sscanf and snprintf to work with 64bit integers
    #endif

    #pragma GCC diagnostic push

    /* Due to a bug in MinGW-w64, a false warning is sometimes issued when using
       "%llu" format with the "printf"/"scanf" family of functions. */
    #pragma GCC diagnostic ignored "-Wformat"
    #pragma GCC diagnostic ignored "-Wformat-extra-args"

#endif

namespace CEGUI
{

/*!
\brief
	Helper class used to convert various data types to and from the format expected in Property strings

\par
    Usage:

    float value = PropertyHelper<float>::fromString("0.1");
    String value = PropertyHelper<float>::toString(0.1);
*/
template<typename T>
class PropertyHelper;

// this redirects PropertyHelper<const T> to PropertyHelper<T>
template<typename T>
class PropertyHelper<const T>
{
public:
    typedef typename PropertyHelper<T>::return_type return_type;
    typedef typename PropertyHelper<T>::safe_method_return_type safe_method_return_type;
    typedef typename PropertyHelper<T>::pass_type pass_type;
    typedef typename PropertyHelper<T>::string_return_type string_return_type;

    static inline const String& getDataTypeName()
    {
        return PropertyHelper<T>::getDataTypeName();
    }

    static inline return_type fromString(const String& str)
    {
        return PropertyHelper<T>::fromString(str);
    }

    static inline String toString(pass_type val)
    {
        return PropertyHelper<T>::toString(val);
    }
};

// this redirects PropertyHelper<const T&> to PropertyHelper<T>
template<typename T>
class PropertyHelper<const T&>
{
public:
    typedef typename PropertyHelper<T>::return_type return_type;
    typedef typename PropertyHelper<T>::safe_method_return_type safe_method_return_type;
    typedef typename PropertyHelper<T>::pass_type pass_type;
    typedef typename PropertyHelper<T>::string_return_type string_return_type;

    static inline const String& getDataTypeName()
    {
        return PropertyHelper<T>::getDataTypeName();
    }

    static inline return_type fromString(const String& str)
    {
        return PropertyHelper<T>::fromString(str);
    }

    static inline String toString(pass_type val)
    {
        return PropertyHelper<T>::toString(val);
    }
};

// this redirects PropertyHelper<const T*> to PropertyHelper<T*>
template<typename T>
class PropertyHelper<const T*>
{
public:
    typedef typename PropertyHelper<T*>::return_type return_type;
    typedef typename PropertyHelper<T*>::safe_method_return_type safe_method_return_type;
    typedef typename PropertyHelper<T*>::pass_type pass_type;
    typedef typename PropertyHelper<T*>::string_return_type string_return_type;

    static inline const String& getDataTypeName()
    {
        return PropertyHelper<T>::getDataTypeName();
    }

    static inline return_type fromString(const String& str)
    {
        return PropertyHelper<T*>::fromString(str);
    }

    static inline String toString(pass_type val)
    {
        return PropertyHelper<T*>::toString(val);
    }
};

template<>
class PropertyHelper<String>
{
public:
    typedef const String& return_type;
    typedef String safe_method_return_type;
    typedef const String& pass_type;
    typedef const String& string_return_type;

    static const String& getDataTypeName()
    {
        static String type("String");

        return type;
    }

    static inline return_type fromString(const String& str)
    {
        return str;
    }

    static inline string_return_type toString(pass_type val)
    {
        return val;
    }
};

template<>
class PropertyHelper<float>
{
public:
    typedef float return_type;
    typedef return_type safe_method_return_type;
    typedef const float pass_type;
    typedef String string_return_type;
    
    static const String& getDataTypeName()
    {
        static String type("float");

        return type;
    }

    static inline return_type fromString(const String& str)
    {
        float val = 0;
        sscanf(str.c_str(), " %g", &val);
        
        return val;
    }

    static inline string_return_type toString(pass_type val)
    {
        char buff[64];
        snprintf(buff, sizeof(buff), "%g", val);

        return String(buff);
    }
};
template<>
class PropertyHelper<double>
{
public:
    typedef double return_type;
    typedef return_type safe_method_return_type;
    typedef const double pass_type;
    typedef String string_return_type;
    
    static const String& getDataTypeName()
    {
        static String type("double");

        return type;
    }

    static inline return_type fromString(const String& str)
    {
        double val = 0;
        sscanf(str.c_str(), " %lg", &val);
        
        return val;
    }

    static inline string_return_type toString(pass_type val)
    {
        char buff[64];
        snprintf(buff, sizeof(buff), "%g", val);

        return String(buff);
    }
};

template<>
class PropertyHelper<int>
{
public:
    typedef int return_type;
    typedef return_type safe_method_return_type;
    typedef const int pass_type;
    typedef String string_return_type;
    
    static const String& getDataTypeName()
    {
        static String type("int");

        return type;
    }

    static inline return_type fromString(const String& str)
    {
        int val = 0;
        sscanf(str.c_str(), " %d", &val);

        return val;
    }

    static inline string_return_type toString(pass_type val)
    {
        char buff[64];
        snprintf(buff, sizeof(buff), "%d", val);

        return String(buff);
    }
};

template<>
class PropertyHelper<uint>
{
public:
    typedef uint return_type;
    typedef return_type safe_method_return_type;
    typedef const uint pass_type;
    typedef String string_return_type;
    
    static const String& getDataTypeName()
    {
        static String type("uint");

        return type;
    }

    static return_type fromString(const String& str)
    {
        uint val = 0;
        sscanf(str.c_str(), " %u", &val);

        return val;
    }

    static string_return_type toString(pass_type val)
    {
        char buff[64];
        snprintf(buff, sizeof(buff), "%u", val);

        return String(buff);
    }
};

template<>
class PropertyHelper<uint64>
{
public:
    typedef uint64 return_type;
    typedef return_type safe_method_return_type;
    typedef const uint64 pass_type;
    typedef String string_return_type;
    
    static const String& getDataTypeName()
    {
        static String type("uint64");

        return type;
    }

    static return_type fromString(const String& str)
    {
        uint64 val = 0;
        sscanf(str.c_str(), " %llu", &val);

        return val;
    }

    static string_return_type toString(pass_type val)
    {
        char buff[64];
        snprintf(buff, sizeof(buff), "%llu", val);

        return String(buff);
    }
};

#if CEGUI_STRING_CLASS != CEGUI_STRING_CLASS_UNICODE

template<>
class PropertyHelper<String::value_type>
{
public:
    typedef String::value_type return_type;
    typedef return_type safe_method_return_type;
    typedef const String::value_type pass_type;
    typedef String string_return_type;
    
    static const String& getDataTypeName()
    {
        static String type("char");

        return type;
    }

    static return_type fromString(const String& str)
    {
        return str[0];
    }

    static string_return_type toString(pass_type val)
    {
        return String("") + val;
    }
};

#endif

template<>
class PropertyHelper<unsigned long>
{
public:
    typedef unsigned long return_type;
    typedef return_type safe_method_return_type;
    typedef const unsigned long pass_type;
    typedef String string_return_type;
    
    static const String& getDataTypeName()
    {
        static String type("unsigned long");

        return type;
    }

    static return_type fromString(const String& str)
    {
        unsigned long val = 0;
        sscanf(str.c_str(), " %lu", &val);

        return val;
    }

    static string_return_type toString(pass_type val)
    {
        char buff[64];
        snprintf(buff, sizeof(buff), "%lu", val);

        return String(buff);
    }
};

template<> 
class CEGUIEXPORT PropertyHelper<bool>
{
public:
    typedef bool return_type;
    typedef return_type safe_method_return_type;
    typedef const bool pass_type;
    typedef const String& string_return_type;
    
    static const String& getDataTypeName()
    {
        static String type("bool");

        return type;
    }

    static return_type fromString(const String& str)
    {
        return (str == True || str == "True");
    }

    static string_return_type toString(pass_type val)
    {
        return val ? True : False;
    }

    //! Definitions of the possible values represented as Strings
    static const CEGUI::String True;
    static const CEGUI::String False;
};



template<> 
class CEGUIEXPORT PropertyHelper<AspectMode>
{
public:
    typedef AspectMode return_type;
    typedef return_type safe_method_return_type;
    typedef AspectMode pass_type;
    typedef String string_return_type;

    static const String& getDataTypeName()
    {
        static String type("AspectMode");

        return type;
    }

    static return_type fromString(const String& str)
    {
        if (str == Shrink)
        {
            return AM_SHRINK;
        }
        else if (str == Expand)
        {
            return AM_EXPAND;
        }
        else
        {
            return AM_IGNORE;
        }
    }

    static string_return_type toString(pass_type val)
    {
        if (val == AM_IGNORE)
        {
            return Ignore;
        }
        else if (val == AM_SHRINK)
        {
            return Shrink;
        }
        else if (val == AM_EXPAND)
        {
            return Expand;
        }
        else
        {
            assert(false && "Invalid aspect mode");
            return Ignore;
        }
    }

    //! Definitions of the possible values represented as Strings
    static const CEGUI::String Shrink;
    static const CEGUI::String Expand;
    static const CEGUI::String Ignore;
};

template<>
class PropertyHelper<Sizef >
{
public:
    typedef Sizef return_type;
    typedef return_type safe_method_return_type;
    typedef const Sizef& pass_type;
    typedef String string_return_type;

    static const String& getDataTypeName()
    {
        static String type("Sizef");

        return type;
    }

    static return_type fromString(const String& str)
    {
        Sizef val(0, 0);
        sscanf(str.c_str(), " w:%g h:%g", &val.d_width, &val.d_height);

        return val;
    }

    static string_return_type toString(pass_type val)
    {
        char buff[128];
        snprintf(buff, sizeof(buff), "w:%g h:%g", val.d_width, val.d_height);

        return String(buff);
    }
};

template<>
class PropertyHelper<Vector2f >
{
public:
    typedef Vector2f return_type;
    typedef return_type safe_method_return_type;
    typedef const Vector2f& pass_type;
    typedef String string_return_type;

    static const String& getDataTypeName()
    {
        static String type("Vector2f");

        return type;
    }

    static return_type fromString(const String& str)
    {
        Vector2f val(0, 0) ;
        sscanf(str.c_str(), " x:%g y:%g", &val.d_x, &val.d_y);

        return val;
    }

    static string_return_type toString(pass_type val)
    {
        char buff[128];
        snprintf(buff, sizeof(buff), "x:%g y:%g", val.d_x, val.d_y);

        return String(buff);
    }
};

template<>
class PropertyHelper<Vector3f >
{
public:
    typedef Vector3f return_type;
    typedef return_type safe_method_return_type;
    typedef const Vector3f& pass_type;
    typedef String string_return_type;
    
    static const String& getDataTypeName()
    {
        static String type("Vector3f");

        return type;
    }

    static return_type fromString(const String& str)
    {
        Vector3f val(0, 0, 0);
        sscanf(str.c_str(), " x:%g y:%g z:%g", &val.d_x, &val.d_y, &val.d_z);

        return val;
    }

    static string_return_type toString(pass_type val)
    {
        char buff[128];
        snprintf(buff, sizeof(buff), "x:%g y:%g z:%g", val.d_x, val.d_y, val.d_z);

        return String(buff);
    }
};

template<>
class PropertyHelper<Quaternion>
{
public:
    typedef Quaternion return_type;
    typedef return_type safe_method_return_type;
    typedef const Quaternion& pass_type;
    typedef String string_return_type;
    
    static const String& getDataTypeName()
    {
        static String type("Quaternion");

        return type;
    }

    static return_type fromString(const String& str)
    {
        if (strchr(str.c_str(), 'w') || strchr(str.c_str(), 'W'))
        {
            Quaternion val(1, 0, 0, 0);
            sscanf(str.c_str(), " w:%g x:%g y:%g z:%g", &val.d_w, &val.d_x, &val.d_y, &val.d_z);

            return val;
        }
        else
        {
            float x, y, z;
            sscanf(str.c_str(), " x:%g y:%g z:%g", &x, &y, &z);
            return Quaternion::eulerAnglesDegrees(x, y, z);
        }
    }

    static string_return_type toString(pass_type val)
    {
        char buff[128];
        snprintf(buff, sizeof(buff), "w:%g x:%g y:%g z:%g", val.d_w, val.d_x, val.d_y, val.d_z);

        return String(buff);
    }
};

template<>
class PropertyHelper<Rectf >
{
public:
    typedef Rectf return_type;
    typedef return_type safe_method_return_type;
    typedef const Rectf& pass_type;
    typedef String string_return_type;
    
    static const String& getDataTypeName()
    {
        static String type("Rectf");

        return type;
    }

    static return_type fromString(const String& str)
    {
        Rectf val(0, 0, 0, 0);
        sscanf(str.c_str(), " l:%g t:%g r:%g b:%g", &val.d_min.d_x, &val.d_min.d_y, &val.d_max.d_x, &val.d_max.d_y);

        return val;
    }

    static string_return_type toString(pass_type val)
    {
        char buff[256];
        snprintf(buff, sizeof(buff), "l:%g t:%g r:%g b:%g",
                 val.d_min.d_x, val.d_min.d_y, val.d_max.d_x, val.d_max.d_y);

        return String(buff);
    }
};

template<>
class CEGUIEXPORT PropertyHelper<Image*>
{
public:
    typedef const Image* return_type;
    typedef return_type safe_method_return_type;
    typedef const Image* const pass_type;
    typedef String string_return_type;
    
    static const String& getDataTypeName()
    {
        static String type("Image");

        return type;
    }

    static return_type fromString(const String& str);

    static string_return_type toString(pass_type val);
};

template<>
class PropertyHelper<Colour>
{
public:
    typedef Colour return_type;
    typedef return_type safe_method_return_type;
    typedef const Colour& pass_type;
    typedef String string_return_type;
    
    static const String& getDataTypeName()
    {
        static String type("Colour");

        return type;
    }

    static return_type fromString(const String& str)
    {
        argb_t val = 0xFF000000;
        sscanf(str.c_str(), " %8X", &val);

        return Colour(val);
    }

    static string_return_type toString(pass_type val)
    {
        char buff[16];
        sprintf(buff, "%.8X", val.getARGB());

        return String(buff);
    }
};

template<>
class PropertyHelper<ColourRect>
{
public:
    typedef ColourRect return_type;
    typedef return_type safe_method_return_type;
    typedef const ColourRect& pass_type;
    typedef String string_return_type;
    
    static const String& getDataTypeName()
    {
        static String type("ColourRect");

        return type;
    }

    static return_type fromString(const String& str)
    {
        if (str.length() == 8)
        {
            argb_t all = 0xFF000000;
            sscanf(str.c_str(), "%8X", &all);
            return ColourRect(all);
        }

        argb_t topLeft = 0xFF000000, topRight = 0xFF000000, bottomLeft = 0xFF000000, bottomRight = 0xFF000000;
        sscanf(str.c_str(), "tl:%8X tr:%8X bl:%8X br:%8X", &topLeft, &topRight, &bottomLeft, &bottomRight);

        return ColourRect(topLeft, topRight, bottomLeft, bottomRight);
    }

    static string_return_type toString(pass_type val)
    {
        char buff[64];
        sprintf(buff, "tl:%.8X tr:%.8X bl:%.8X br:%.8X", val.d_top_left.getARGB(), val.d_top_right.getARGB(), val.d_bottom_left.getARGB(), val.d_bottom_right.getARGB());

        return String(buff);
    }
};

template<>
class PropertyHelper<UDim>
{
public:
    typedef UDim return_type;
    typedef return_type safe_method_return_type;
    typedef const UDim& pass_type;
    typedef String string_return_type;
    
    static const String& getDataTypeName()
    {
        static String type("UDim");

        return type;
    }

    static return_type fromString(const String& str)
    {
        UDim ud;
        sscanf(str.c_str(), " { %g , %g }", &ud.d_scale, &ud.d_offset);

        return ud;
    }

    static string_return_type toString(pass_type val)
    {
        char buff[128];
        snprintf(buff, sizeof(buff), "{%g,%g}", val.d_scale, val.d_offset);

        return String(buff);
    }
};

template<>
class PropertyHelper<UVector2>
{
public:
    typedef UVector2 return_type;
    typedef return_type safe_method_return_type;
    typedef const UVector2& pass_type;
    typedef String string_return_type;
    
    static const String& getDataTypeName()
    {
        static String type("UVector2");

        return type;
    }

    static return_type fromString(const String& str)
    {
        UVector2 uv;
        sscanf(str.c_str(), " { { %g , %g } , { %g , %g } }",
               &uv.d_x.d_scale, &uv.d_x.d_offset,
               &uv.d_y.d_scale, &uv.d_y.d_offset);

        return uv;
    }

    static string_return_type toString(pass_type val)
    {
        char buff[256];
        snprintf(buff, sizeof(buff), "{{%g,%g},{%g,%g}}",
                 val.d_x.d_scale, val.d_x.d_offset, val.d_y.d_scale, val.d_y.d_offset);

        return String(buff);
    }
};

template<>
class PropertyHelper<USize>
{
public:
    typedef USize return_type;
    typedef return_type safe_method_return_type;
    typedef const USize& pass_type;
    typedef String string_return_type;
    
    static const String& getDataTypeName()
    {
        static String type("USize");

        return type;
    }

    static return_type fromString(const String& str)
    {
        USize uv;
        sscanf(str.c_str(), " { { %g , %g } , { %g , %g } }",
               &uv.d_width.d_scale, &uv.d_width.d_offset,
               &uv.d_height.d_scale, &uv.d_height.d_offset);

        return uv;
    }

    static string_return_type toString(pass_type val)
    {
        char buff[256];
        snprintf(buff, sizeof(buff), "{{%g,%g},{%g,%g}}",
                 val.d_width.d_scale, val.d_width.d_offset, val.d_height.d_scale, val.d_height.d_offset);

        return String(buff);
    }
};

template<>
class PropertyHelper<URect>
{
public:
    typedef URect return_type;
    typedef return_type safe_method_return_type;
    typedef const URect& pass_type;
    typedef String string_return_type;
    
    static const String& getDataTypeName()
    {
        static String type("URect");

        return type;
    }

    static return_type fromString(const String& str)
    {
        URect ur;
        sscanf(
            str.c_str(),
            " { { %g , %g } , { %g , %g } , { %g , %g } , { %g , %g } }",
            &ur.d_min.d_x.d_scale, &ur.d_min.d_x.d_offset,
            &ur.d_min.d_y.d_scale, &ur.d_min.d_y.d_offset,
            &ur.d_max.d_x.d_scale, &ur.d_max.d_x.d_offset,
            &ur.d_max.d_y.d_scale, &ur.d_max.d_y.d_offset
        );

        return ur;
    }

    static string_return_type toString(pass_type val)
    {
        char buff[512];
        snprintf(buff, sizeof(buff), "{{%g,%g},{%g,%g},{%g,%g},{%g,%g}}",
                 val.d_min.d_x.d_scale, val.d_min.d_x.d_offset,
                 val.d_min.d_y.d_scale, val.d_min.d_y.d_offset,
                 val.d_max.d_x.d_scale, val.d_max.d_x.d_offset,
                 val.d_max.d_y.d_scale, val.d_max.d_y.d_offset);

        return String(buff);
    }
};

template<>
class PropertyHelper<UBox>
{
public:
    typedef UBox return_type;
    typedef return_type safe_method_return_type;
    typedef const UBox& pass_type;
    typedef String string_return_type;
    
    static const String& getDataTypeName()
    {
        static String type("UBox");

        return type;
    }

    static return_type fromString(const String& str)
    {
        UBox ret;
        sscanf(
            str.c_str(),
            " { top: { %g , %g } , left: { %g , %g } , bottom: { %g , %g } , right: { %g , %g } }",
            &ret.d_top.d_scale, &ret.d_top.d_offset,
            &ret.d_left.d_scale, &ret.d_left.d_offset,
            &ret.d_bottom.d_scale, &ret.d_bottom.d_offset,
            &ret.d_right.d_scale, &ret.d_right.d_offset
        );

        return ret;
    }

    static string_return_type toString(pass_type val)
    {
        char buff[512];
        snprintf(buff, sizeof(buff), "{top:{%g,%g},left:{%g,%g},bottom:{%g,%g},right:{%g,%g}}",
                 val.d_top.d_scale, val.d_top.d_offset,
                 val.d_left.d_scale, val.d_left.d_offset,
                 val.d_bottom.d_scale, val.d_bottom.d_offset,
                 val.d_right.d_scale, val.d_right.d_offset);

        return String(buff);
    }
};


template<>
class CEGUIEXPORT PropertyHelper<Font*>
{
public:
    typedef const Font* return_type;
    typedef return_type safe_method_return_type;
    typedef const Font* const pass_type;
    typedef String string_return_type;
    
    static const String& getDataTypeName()
    {
        static String type("Font");

        return type;
    }

    static return_type fromString(const String& str);
    static string_return_type toString(pass_type val);
};

}

#ifdef __MINGW32__
    #pragma GCC diagnostic pop
#endif

#if defined(_MSC_VER)
    #pragma warning(pop)
#endif

#endif
