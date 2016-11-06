/************************************************************************
    created:    Tue Feb 28 2006
    authors:    Paul D Turner <paul@cegui.org.uk>
                Martin Preisler <martin@preisler.me>
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2014 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIFunctorCopySlot_h_
#define _CEGUIFunctorCopySlot_h_

#include "CEGUI/SlotFunctorBase.h"

// Start of CEGUI namespace section
namespace CEGUI
{

namespace FunctorCopySlot_detail
{
    typedef char Yes;
    typedef int No;

    template<typename T>
    struct HasBoolEventArgsConstOp
    {
        template<typename U, bool (U::*)(const EventArgs&) const> struct SFINAE {};
        template<typename U> static Yes Test(SFINAE<U, &U::operator()>*);
        template<typename U> static No Test(...);
        static const bool Value = sizeof(Test<T>(0)) == sizeof(Yes);
    };

    template<typename T>
    struct HasBoolEventArgsOp
    {
        template<typename U, bool (U::*)(const EventArgs&)> struct SFINAE {};
        template<typename U> static Yes Test(SFINAE<U, &U::operator()>*);
        template<typename U> static No Test(...);
        static const bool Value = sizeof(Test<T>(0)) == sizeof(Yes);
    };

    template<typename T>
    struct HasVoidEventArgsConstOp
    {
        template<typename U, void (U::*)(const EventArgs&) const> struct SFINAE {};
        template<typename U> static Yes Test(SFINAE<U, &U::operator()>*);
        template<typename U> static No Test(...);
        static const bool Value = sizeof(Test<T>(0)) == sizeof(Yes);
    };

    template<typename T>
    struct HasVoidEventArgsOp
    {
        template<typename U, void (U::*)(const EventArgs&)> struct SFINAE {};
        template<typename U> static Yes Test(SFINAE<U, &U::operator()>*);
        template<typename U> static No Test(...);
        static const bool Value = sizeof(Test<T>(0)) == sizeof(Yes);
    };

    template<typename T>
    struct HasBoolNoArgsConstOp
    {
        template<typename U, bool (U::*)() const> struct SFINAE {};
        template<typename U> static Yes Test(SFINAE<U, &U::operator()>*);
        template<typename U> static No Test(...);
        static const bool Value = sizeof(Test<T>(0)) == sizeof(Yes);
    };

    template<typename T>
    struct HasBoolNoArgsOp
    {
        template<typename U, bool (U::*)()> struct SFINAE {};
        template<typename U> static Yes Test(SFINAE<U, &U::operator()>*);
        template<typename U> static No Test(...);
        static const bool Value = sizeof(Test<T>(0)) == sizeof(Yes);
    };

    template<typename T>
    struct HasVoidNoArgsConstOp
    {
        template<typename U, void (U::*)() const> struct SFINAE {};
        template<typename U> static Yes Test(SFINAE<U, &U::operator()>*);
        template<typename U> static No Test(...);
        static const bool Value = sizeof(Test<T>(0)) == sizeof(Yes);
    };

    // The following 3 templates use the same idea as boost::disable_if
    template <bool B, class T = void>
    struct DisableIfC
    {
        typedef T Type;
    };

    template <class T>
    struct DisableIfC<true, T>
    {};

    template <class Cond, class T = void>
    struct DisableIf : public DisableIfC<Cond::Value, T>
    {};


    // The following is just overload trickery to accommodate 8 allowed
    // operator() signatures:
    //
    // 1) bool (const EventArgs&) const = BoolEventArgsConstOp
    // 2) bool (const EventArgs&) = BoolEventArgsOp
    // 3) void (const EventArgs&) const = VoidEventArgsConstOp
    // 4) void (const EventArgs&) = VoidEventArgsOp
    // 5) bool () const = BoolNoArgsConstOp
    // 6) bool () = BoolNoArgsOp
    // 7) void () const = VoidNoArgsConstOp
    // 8) void () = VoidNoArgsOp
    //
    // The calls are priorised as outlined above, if call 1) is possible, no
    // other variants are tried, ...

    template<typename T>
    inline bool call(const T& functor, bool(T::*member_fn)(const EventArgs&) const, const EventArgs& args,
        void* = 0,
        void* = 0,
        void* = 0,
        void* = 0,
        void* = 0,
        void* = 0,
        void* = 0
    )
    {
        return CEGUI_CALL_MEMBER_FN(functor, member_fn)(args);
    }

    template<typename T>
    inline bool call(T& functor, bool(T::*member_fn)(const EventArgs&), const EventArgs& args,
        typename DisableIf<HasBoolEventArgsConstOp<T> >::Type* = 0,
        void* = 0,
        void* = 0,
        void* = 0,
        void* = 0,
        void* = 0,
        void* = 0
    )
    {
        return CEGUI_CALL_MEMBER_FN(functor, member_fn)(args);
    }

    template<typename T>
    inline bool call(const T& functor, void(T::*member_fn)(const EventArgs&) const, const EventArgs& args,
        typename DisableIf<HasBoolEventArgsConstOp<T> >::Type* = 0,
        typename DisableIf<HasBoolEventArgsOp<T> >::Type* = 0,
        void* = 0,
        void* = 0,
        void* = 0,
        void* = 0,
        void* = 0
    )
    {
        CEGUI_CALL_MEMBER_FN(functor, member_fn)(args);
        return true;
    }

    template<typename T>
    inline bool call(T& functor, void(T::*member_fn)(const EventArgs&), const EventArgs& args,
        typename DisableIf<HasBoolEventArgsConstOp<T> >::Type* = 0,
        typename DisableIf<HasBoolEventArgsOp<T> >::Type* = 0,
        typename DisableIf<HasVoidEventArgsConstOp<T> >::Type* = 0,
        void* = 0,
        void* = 0,
        void* = 0,
        void* = 0
    )
    {
        CEGUI_CALL_MEMBER_FN(functor, member_fn)(args);
        return true;
    }

    template<typename T>
    inline bool call(const T& functor, bool(T::*member_fn)() const, const EventArgs& /*args*/,
        typename DisableIf<HasBoolEventArgsConstOp<T> >::Type* = 0,
        typename DisableIf<HasBoolEventArgsOp<T> >::Type* = 0,
        typename DisableIf<HasVoidEventArgsConstOp<T> >::Type* = 0,
        typename DisableIf<HasVoidEventArgsOp<T> >::Type* = 0,
        void* = 0,
        void* = 0,
        void* = 0
    )
    {
        return CEGUI_CALL_MEMBER_FN(functor, member_fn)();
    }

    template<typename T>
    inline bool call(T& functor, bool(T::*member_fn)(), const EventArgs& /*args*/,
        typename DisableIf<HasBoolEventArgsConstOp<T> >::Type* = 0,
        typename DisableIf<HasBoolEventArgsOp<T> >::Type* = 0,
        typename DisableIf<HasVoidEventArgsConstOp<T> >::Type* = 0,
        typename DisableIf<HasVoidEventArgsOp<T> >::Type* = 0,
        typename DisableIf<HasBoolNoArgsConstOp<T> >::Type* = 0,
        void* = 0,
        void* = 0
    )
    {
        return CEGUI_CALL_MEMBER_FN(functor, member_fn)();
    }

    template<typename T>
    inline bool call(const T& functor, void(T::*member_fn)() const, const EventArgs& /*args*/,
        typename DisableIf<HasBoolEventArgsConstOp<T> >::Type* = 0,
        typename DisableIf<HasBoolEventArgsOp<T> >::Type* = 0,
        typename DisableIf<HasVoidEventArgsConstOp<T> >::Type* = 0,
        typename DisableIf<HasVoidEventArgsOp<T> >::Type* = 0,
        typename DisableIf<HasBoolNoArgsConstOp<T> >::Type* = 0,
        typename DisableIf<HasBoolNoArgsOp<T> >::Type* = 0,
        void* = 0
    )
    {
        CEGUI_CALL_MEMBER_FN(functor, member_fn)();
        return true;
    }

    template<typename T>
    inline bool call(T& functor, void(T::*member_fn)(), const EventArgs& /*args*/,
        typename DisableIf<HasBoolEventArgsConstOp<T> >::Type* = 0,
        typename DisableIf<HasBoolEventArgsOp<T> >::Type* = 0,
        typename DisableIf<HasVoidEventArgsConstOp<T> >::Type* = 0,
        typename DisableIf<HasVoidEventArgsOp<T> >::Type* = 0,
        typename DisableIf<HasBoolNoArgsConstOp<T> >::Type* = 0,
        typename DisableIf<HasBoolNoArgsOp<T> >::Type* = 0,
        typename DisableIf<HasVoidNoArgsConstOp<T> >::Type* = 0
    )
    {
        CEGUI_CALL_MEMBER_FN(functor, member_fn)();
        return true;
    }
}

/*!
\brief
    Slot template class that creates a functor that calls back via a copy of a
    functor object.
*/
template<typename T>
class FunctorCopySlot : public SlotFunctorBase
{
public:
    FunctorCopySlot(const T& functor):
        d_functor(functor)
    {}

    virtual bool operator()(const EventArgs& args)
    {
        return FunctorCopySlot_detail::call<T>(d_functor, &T::operator(), args);
    }

private:
    T d_functor;
};

}

#endif  // end of guard _CEGUIFunctorCopySlot_h_
