/***********************************************************************
    created:    27/08/2011
    author:     Hans Mackowiak

    purpose:    
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
#ifndef _TplWindowRendererProperty_h_
#define _TplWindowRendererProperty_h_

#include "CEGUI/TplProperty.h"

// Start of CEGUI namespace section
namespace CEGUI
{
 
template<class C, typename T>
class TplWindowRendererProperty : public TplProperty<C, T>
{
public:
    TplWindowRendererProperty(
        const String& name, const String& help, const String& origin,
        typename TplProperty<C, T>::Setter setter,
        typename TplProperty<C, T>::GetterFunctor getter,
        typename TplProperty<C, T>::Helper::pass_type defaultValue = T(),
        bool writesXML = true) :

        TplProperty<C, T>(name, help, origin,
                          setter, getter,
                          defaultValue, writesXML)
    {}

    virtual Property* clone() const
    {
        return CEGUI_NEW_AO TplWindowRendererProperty<C, T>(*this);
    }

protected:
    //! \copydoc TypedProperty::setNative_impl
    void setNative_impl(PropertyReceiver* receiver,
                        typename TplProperty<C, T>::Helper::pass_type value)
    {
        C* instance = static_cast<C*>(
            static_cast<const Window*>(receiver)->getWindowRenderer());

        CEGUI_CALL_MEMBER_FN(*instance, this->d_setter)(value);
    }

    //! \copydoc TypedProperty::getNative_impl
    typename TplProperty<C, T>::Helper::safe_method_return_type
    getNative_impl(const PropertyReceiver* receiver) const
    {
        const C* instance = static_cast<const C*>(
            static_cast<const Window*>(receiver)->getWindowRenderer());

        return this->d_getter(instance);
    }
};

/*!
Example of usage inside addStandardProperties or similar method.
{
    CEGUI_DEFINE_WINDOW_RENDERER_PROPERTY(Window, float, "Alpha",
        "Property to get/set the alpha value of the Window. Value is floating point number.",
        &Window::setAlpha, &Window::getAlpha, 1.0f)
};

*/
#define CEGUI_DEFINE_WINDOW_RENDERER_PROPERTY(class_type, property_native_type, name, help, setter, getter, default_value)\
{\
    static ::CEGUI::TplWindowRendererProperty<class_type, property_native_type> sProperty(\
            name, help, TypeName, setter, getter, default_value);\
    \
    this->registerProperty(&sProperty);\
}


/*!
Same as CEGUI_DEFINE_WINDOW_RENDERER_PROPERTY but writeXML is set to false

Example of usage inside addStandardProperties or similar method.
{
    CEGUI_DEFINE_WINDOW_RENDERER_PROPERTY_NO_XML(Window, float, "Alpha",
        "Property to get/set the alpha value of the Window. Value is floating point number.",
        &Window::setAlpha, &Window::getAlpha, 1.0f)
};

*/
#define CEGUI_DEFINE_WINDOW_RENDERER_PROPERTY_NO_XML(class_type, property_native_type, name, help, setter, getter, default_value)\
{\
    static ::CEGUI::TplWindowRendererProperty<class_type, property_native_type> sProperty(\
            name, help, TypeName, setter, getter, default_value, false);\
    \
    this->registerProperty(&sProperty,true);\
}
} // End of  CEGUI namespace section

#endif  // end of guard _TplWindowRendererProperty_h_
