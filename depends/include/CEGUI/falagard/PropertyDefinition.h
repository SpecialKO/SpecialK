/***********************************************************************
    created:    Sun Jun 26 2005
    author:     Paul D Turner <paul@cegui.org.uk>
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
#ifndef _CEGUIFalPropertyDefinition_h_
#define _CEGUIFalPropertyDefinition_h_

#include "CEGUI/falagard/FalagardPropertyBase.h"
#include "CEGUI/falagard/XMLHandler.h"
#include "CEGUI/Logger.h"

namespace CEGUI
{
template <typename T>
class PropertyDefinition : public FalagardPropertyBase<T>
{
public:
    typedef typename TypedProperty<T>::Helper Helper;

    //------------------------------------------------------------------------//
    PropertyDefinition(const String& name, const String& initialValue,
                       const String& help, const String& origin,
                       bool redrawOnWrite, bool layoutOnWrite,
                       const String& fireEvent, const String& eventNamespace) :
        FalagardPropertyBase<T>(name, help, initialValue, origin,
                                redrawOnWrite, layoutOnWrite,
                                fireEvent, eventNamespace),
                                d_userStringName(name + PropertyDefinitionBase::UserStringNameSuffix)
    {
    }

    //------------------------------------------------------------------------//
    ~PropertyDefinition() {}

    //------------------------------------------------------------------------//
    void initialisePropertyReceiver(PropertyReceiver* receiver) const
    {
        setWindowUserString(static_cast<Window*>(receiver), FalagardPropertyBase<T>::d_initialValue);
    }

    //------------------------------------------------------------------------//
    Property* clone() const
    {
        return CEGUI_NEW_AO PropertyDefinition<T>(*this);
    }

protected:
    //------------------------------------------------------------------------//
    typename Helper::safe_method_return_type
    getNative_impl(const PropertyReceiver* receiver) const
    {
        const Window* const wnd = static_cast<const Window*>(receiver);

        // the try/catch is used instead of querying the existence of the user
        // string in order that for the 'usual' case - where the user string
        // exists - there is basically no additional overhead, and that any
        // overhead is incurred only for the initial creation of the user
        // string.
        // Maybe the only negative here is that an error gets logged, though
        // this can be treated as a 'soft' error.
        CEGUI_TRY
        {
            return Helper::fromString(wnd->getUserString(d_userStringName));
        }
        CEGUI_CATCH (UnknownObjectException&)
        {
            Logger::getSingleton().logEvent(
                "PropertyDefiniton::get: Defining new user string: " +
                d_userStringName);

            // HACK: FIXME: TODO: This const_cast is basically to allow the
            // above mentioned performance measure; the alternative would be
            // to just return d_default, and while technically correct, it
            // would be very slow.
            const_cast<Window*>(wnd)->
                setUserString(d_userStringName, FalagardPropertyBase<T>::d_initialValue);

            return Helper::fromString(FalagardPropertyBase<T>::d_initialValue);
        }
    }

    //------------------------------------------------------------------------//
    void setNative_impl(PropertyReceiver* receiver,typename Helper::pass_type value)
    {
        setWindowUserString(static_cast<Window*>(receiver), Helper::toString(value));
        FalagardPropertyBase<T>::setNative_impl(receiver, value);
    }

    //------------------------------------------------------------------------//
    void setWindowUserString(Window* window, const String& value) const
    {
        window->setUserString(d_userStringName, value);
    }

    //------------------------------------------------------------------------//
    void writeDefinitionXMLElementType(XMLSerializer& xml_stream) const
    {
        xml_stream.openTag(Falagard_xmlHandler::PropertyDefinitionElement);
        writeDefinitionXMLAdditionalAttributes(xml_stream);
    }
    //------------------------------------------------------------------------//
    void writeDefinitionXMLAdditionalAttributes(XMLSerializer& xml_stream) const
    {
        if(FalagardPropertyBase<T>::d_dataType.compare(Falagard_xmlHandler::GenericDataType) != 0)
            xml_stream.attribute(Falagard_xmlHandler::TypeAttribute, FalagardPropertyBase<T>::d_dataType);

        if (!FalagardPropertyBase<T>::d_helpString.empty() && FalagardPropertyBase<T>::d_helpString.compare(CEGUI::Falagard_xmlHandler::PropertyDefinitionHelpDefaultValue) != 0)
            xml_stream.attribute(Falagard_xmlHandler::HelpStringAttribute, FalagardPropertyBase<T>::d_helpString);
    }


    //------------------------------------------------------------------------//

    String d_userStringName;
};

}

#endif

