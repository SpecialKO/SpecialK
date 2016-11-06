/***********************************************************************
    created:    Fri Jun 15 2012
    author:     Paul D Turner <paul@cegui.org.uk>
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2012 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIFormattingSetting_h_
#define _CEGUIFormattingSetting_h_

#include "CEGUI/Window.h"
#include "CEGUI/falagard/XMLEnumHelper.h"

namespace CEGUI
{

template<typename T>
class FormattingSetting
{
public:
    //------------------------------------------------------------------------//
    FormattingSetting() :
        d_value(FalagardXMLHelper<T>::fromString(""))
    {}

    //------------------------------------------------------------------------//
    FormattingSetting(const String& property_name) :
        d_value(FalagardXMLHelper<T>::fromString("")),
        d_propertySource(property_name)
    {}

    //------------------------------------------------------------------------//
    FormattingSetting(T val) :
        d_value(val)
    {}

    //------------------------------------------------------------------------//
    T get(const Window& wnd) const
    {
        if (d_propertySource.empty())
            return d_value;

        return FalagardXMLHelper<T>::fromString(
            wnd.getProperty(d_propertySource));
    }

    //------------------------------------------------------------------------//
    void set(T val)
    {
        d_value = val;
        d_propertySource.clear();
    }

    //------------------------------------------------------------------------//
    T getValue() const
    {
        return d_value;
    }

    //------------------------------------------------------------------------//
    const String& getPropertySource() const
    {
        return d_propertySource;
    }

    //------------------------------------------------------------------------//
    void setPropertySource(const String& property_name)
    {
        d_propertySource = property_name;
    }

    //------------------------------------------------------------------------//
    bool isFetchedFromProperty() const
    {
        return !d_propertySource.empty();
    }

    //------------------------------------------------------------------------//
    void writeXMLToStream(XMLSerializer& xml_stream) const
    {
        writeXMLTagToStream(xml_stream);
        writeXMLAttributesToStream(xml_stream);
        xml_stream.closeTag();
    }

    //------------------------------------------------------------------------//
    virtual void writeXMLTagToStream(XMLSerializer& /*xml_stream*/) const;

    //------------------------------------------------------------------------//
    virtual void writeXMLAttributesToStream(XMLSerializer& /*xml_stream*/) const;

    //------------------------------------------------------------------------//
    bool operator==(const FormattingSetting<T>& rhs) const
    {
        return d_value == rhs.d_value &&
               d_propertySource == rhs.d_propertySource;
    }

    //------------------------------------------------------------------------//
    bool operator!=(const FormattingSetting<T>& rhs) const
    {
        return !operator==(rhs);
    }

protected:
    T d_value;
    String d_propertySource;
};

template<> void CEGUIEXPORT FormattingSetting<VerticalFormatting>::writeXMLTagToStream(
                                            XMLSerializer& xml_stream) const;
template<> void CEGUIEXPORT FormattingSetting<VerticalFormatting>::writeXMLAttributesToStream(
                                            XMLSerializer& xml_stream) const;
template<> void CEGUIEXPORT FormattingSetting<HorizontalFormatting>::writeXMLTagToStream(
                                            XMLSerializer& xml_stream) const;
template<> void CEGUIEXPORT FormattingSetting<HorizontalFormatting>::writeXMLAttributesToStream(
                                            XMLSerializer& xml_stream) const;
template<> void CEGUIEXPORT FormattingSetting<VerticalTextFormatting>::writeXMLTagToStream(
                                            XMLSerializer& xml_stream) const;
template<> void CEGUIEXPORT FormattingSetting<VerticalTextFormatting>::writeXMLAttributesToStream(
                                            XMLSerializer& xml_stream) const;
template<> void CEGUIEXPORT FormattingSetting<HorizontalTextFormatting>::writeXMLTagToStream(
                                            XMLSerializer& xml_stream) const;
template<> void CEGUIEXPORT FormattingSetting<HorizontalTextFormatting>::writeXMLAttributesToStream(
                                            XMLSerializer& xml_stream) const;
}

#endif

