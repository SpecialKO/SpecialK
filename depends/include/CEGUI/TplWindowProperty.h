/***********************************************************************
    created:    Sun Oct 23 2011
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
#ifndef _CEGUITplWindowProperty_h_
#define _CEGUITplWindowProperty_h_

#include "CEGUI/TplProperty.h"

// Start of CEGUI namespace section
namespace CEGUI
{

template<class C, typename T>
class TplWindowProperty : public TplProperty<C, T>
{
public:
    TplWindowProperty(const String& name, const String& help,
                      const String& origin,
                      typename TplProperty<C, T>::Setter setter,
                      typename TplProperty<C, T>::GetterFunctor getter,
                      typename TplProperty<C, T>::Helper::pass_type defaultValue = T(),
                      bool writesXML = true) :
        TplProperty<C, T>(name, help, origin, setter, getter,
                          defaultValue, writesXML)
    {}

    virtual Property* clone() const
    {
        return CEGUI_NEW_AO TplWindowProperty<C, T>(*this);
    }

protected:
    //! \copydoc TypedProperty::setNative_impl
    void setNative_impl(PropertyReceiver* receiver,
                        typename TplProperty<C, T>::Helper::pass_type value)
    {
        C* instance = static_cast<C*>(receiver);
        CEGUI_CALL_MEMBER_FN(*instance, this->d_setter)(value);
    }

    //! \copydoc TypedProperty::getNative_impl
    typename TplProperty<C, T>::Helper::safe_method_return_type
    getNative_impl(const PropertyReceiver* receiver) const
    {
        const C* instance = static_cast<const C*>(receiver);
        return this->d_getter(instance);
    }
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUITplWindowProperty_h_

