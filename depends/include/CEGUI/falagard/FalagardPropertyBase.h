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
#ifndef _CEGUIFalagardPropertyBase_h_
#define _CEGUIFalagardPropertyBase_h_

#include "CEGUI/TypedProperty.h"
#include "CEGUI/falagard/PropertyDefinitionBase.h"

namespace CEGUI
{
template <typename T>
class FalagardPropertyBase : public PropertyDefinitionBase, public TypedProperty<T>
{
public:
    //------------------------------------------------------------------------//
    typedef typename TypedProperty<T>::Helper Helper;

    //------------------------------------------------------------------------//
    FalagardPropertyBase(const String& name, const String& help,
                         const String& initialValue, const String& origin,
                         bool redrawOnWrite, bool layoutOnWrite,
                         const String& fireEvent, const String& eventNamespace) :
        PropertyDefinitionBase(name, help, initialValue,
                               redrawOnWrite, layoutOnWrite,
                               fireEvent, eventNamespace),
        TypedProperty<T>(name, help, origin,
                         Helper::fromString(initialValue))
    {}

    //------------------------------------------------------------------------//
    ~FalagardPropertyBase() {}

protected:
    //------------------------------------------------------------------------//
    void setNative_impl(PropertyReceiver* receiver,
                        typename Helper::pass_type /*value*/)
    {
        if (d_writeCausesLayout)
            static_cast<Window*>(receiver)->performChildWindowLayout();

        if (d_writeCausesRedraw)
            static_cast<Window*>(receiver)->invalidate();

        if (!d_eventFiredOnWrite.empty())
        {
            WindowEventArgs args(static_cast<Window*>(receiver));
            args.window->fireEvent(d_eventFiredOnWrite, args,
                                   d_eventNamespace);
        }
    }

    //------------------------------------------------------------------------//
};

}

#endif

