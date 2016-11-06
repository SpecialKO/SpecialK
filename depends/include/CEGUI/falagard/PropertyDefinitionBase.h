/***********************************************************************
    created:    Sat Jun 16 2012
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
#ifndef _CEGUIPropertyDefinitionBase_h_
#define _CEGUIPropertyDefinitionBase_h_

#include "CEGUI/String.h"

namespace CEGUI
{
class XMLSerializer;

/*!
\brief
    common base class used for types representing a new property to be
    available on all widgets that use the WidgetLook that the property
    definition is a part of.
*/
class CEGUIEXPORT PropertyDefinitionBase
{
public:
    PropertyDefinitionBase(const String& name, const String& help,
                           const String& initialValue,
                           bool redrawOnWrite, bool layoutOnWrite,
                           const String& fireEvent, const String& eventNamespace);

    virtual ~PropertyDefinitionBase();

    const String& getPropertyName() const;
    void setPropertyName(const String& name);

    const String& getInitialValue() const;
    void setInitialValue(const String& value);

    const String& getHelpString() const;
    void setHelpString(const String& help);

    bool isRedrawOnWrite() const;
    void setRedrawOnWrite(bool value);

    bool isLayoutOnWrite() const;
    void setLayoutOnWrite(bool value);

    const String& getEventFiredOnWrite() const;
    void setEventFiredOnWrite(const String& eventName);

    const String& getEventNamespace() const;
    void setEventNamespace(const String& eventNamespace);

    /*!
    \brief
        Writes an xml representation of the PropertyDefinitionBase based
        object to \a out_stream.

    \param xml_stream
        XMLSerializer where xml data should be output.
    */
    virtual void writeDefinitionXMLToStream(XMLSerializer& xml_stream) const;

    //! The PropertyDefinition's user string name suffix, which is appended to each #d_userStringName
    static const String UserStringNameSuffix;

protected:

    /*!
    \brief
        Write out the text of the XML element type.  Note that you should
        not write the opening '<' character, nor any other information such
        as attributes in this function.

    \param xml_stream
        XMLSerializer where xml data should be output.
    */
    virtual void writeDefinitionXMLElementType(XMLSerializer& xml_stream) const = 0;

    /*!
    \brief
        Write out any xml attributes added in a sub-class.  Note that you
        should not write the closing '/>' character sequence, nor any other
        information in this function.  You should always call the base class
        implementation of this function when overriding.

    \param xml_stream
        XMLSerializer where xml data should be output.
    */
    virtual void writeDefinitionXMLAttributes(XMLSerializer& xml_stream) const;

    String d_propertyName;
    String d_initialValue;
    String d_helpString;
    bool d_writeCausesRedraw;
    bool d_writeCausesLayout;
    String d_eventFiredOnWrite;
    String d_eventNamespace;
};

}

#endif

