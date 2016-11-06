/***********************************************************************
    created:    23/11/2010
    author:     Martin Preisler

    purpose:    Finger saving template property
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2010 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUITplProperty_h_
#define _CEGUITplProperty_h_

#include "CEGUI/TypedProperty.h"

// Start of CEGUI namespace section
namespace CEGUI
{

template<class C, typename T>
class TplProperty : public TypedProperty<T>
{
public:
    typedef PropertyHelper<T> Helper;
    
    typedef void (C::*Setter)(typename Helper::pass_type);

    /*!
    Template madness to sort out getters returning ref T, const ref T or just T.
    */
    struct GetterFunctor
    {
        template<typename DT> struct EnsurePlain { typedef DT result; };
        template<typename DT> struct EnsurePlain<DT&> { typedef DT result; };
        template<typename DT> struct EnsurePlain<const DT&> { typedef DT result; };

        template<typename DT> struct EnsureConstRef { typedef const DT& result; };
        template<typename DT> struct EnsureConstRef<DT&> { typedef const DT& result; };
        template<typename DT> struct EnsureConstRef<const DT&> { typedef const DT& result; };

        template<typename DT> struct EnsureRef { typedef DT& result; };
        template<typename DT> struct EnsureRef<DT&> { typedef DT& result; };
        template<typename DT> struct EnsureRef<const DT&> { typedef DT& result; };

        typedef typename EnsurePlain<typename Helper::safe_method_return_type>::result (C::*PlainGetter)() const;
        typedef typename EnsureConstRef<typename Helper::safe_method_return_type>::result (C::*ConstRefGetter)() const;
        typedef typename EnsureRef<typename Helper::safe_method_return_type>::result (C::*RefGetter)() const;

        GetterFunctor(PlainGetter getter):
            d_plainGetter(getter)
            //d_constRefGetter(0), no need to initialise these, we will never use them
            //d_refGetter(0)
        {}

        GetterFunctor(ConstRefGetter getter):
            d_plainGetter(0),
            d_constRefGetter(getter)
            //d_refGetter(0) // no need to initialise this, we will never use it
        {}

        GetterFunctor(RefGetter getter):
            d_plainGetter(0),
            d_constRefGetter(0),
            d_refGetter(getter)
        {}
        // to set 0 as func
        GetterFunctor(int /*val*/):
            d_plainGetter(0),
            d_constRefGetter(0),
            d_refGetter(0)
        {}
        operator bool(void) const
        {
            return d_plainGetter || d_constRefGetter || d_refGetter;
        }
        typename Helper::safe_method_return_type operator()(const C* instance) const
        {
            // FIXME: Ideally we want this to be done during compilation, not runtime

            if (d_plainGetter)
                return CEGUI_CALL_MEMBER_FN(*instance, d_plainGetter)();
            if (d_constRefGetter)
                return CEGUI_CALL_MEMBER_FN(*instance, d_constRefGetter)();
            if (d_refGetter)
                return CEGUI_CALL_MEMBER_FN(*instance, d_refGetter)();

            assert(false);
            // just to get rid of the warning
            return CEGUI_CALL_MEMBER_FN(*instance, d_plainGetter)();
        }

        PlainGetter d_plainGetter;
        ConstRefGetter d_constRefGetter;
        RefGetter d_refGetter;
    };

    TplProperty(const String& name, const String& help, const String& origin, Setter setter, GetterFunctor getter, typename Helper::pass_type defaultValue = T(), bool writesXML = true):
        TypedProperty<T>(name, help, origin, defaultValue, writesXML),
        
        d_setter(setter),
        d_getter(getter)
    {}
    
    virtual ~TplProperty()
    {}

    //! \copydoc Property::isReadable
    virtual bool isReadable() const
    {
        return d_getter;
    }
    //! \copydoc Property::isWritable
    virtual bool isWritable() const
    {
        return d_setter;
    }

protected:
    Setter d_setter;
    GetterFunctor d_getter;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUITplProperty_h_

