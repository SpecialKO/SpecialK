/************************************************************************
    created:    Tue Feb 28 2006
    author:     Paul D Turner <paul@cegui.org.uk>
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
#ifndef _CEGUIMemberFunctionSlot_h_
#define _CEGUIMemberFunctionSlot_h_

#include "CEGUI/SlotFunctorBase.h"

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Slot template class that creates a functor that calls back via a class
    member function.
*/
template<typename T>
class MemberFunctionSlot : public SlotFunctorBase
{
public:
    //! Member function slot type.
    typedef bool(T::*MemberFunctionType)(const EventArgs&);

    MemberFunctionSlot(MemberFunctionType func, T* obj) :
        d_function(func),
        d_object(obj)
    {}

    virtual bool operator()(const EventArgs& args)
    {
        return (d_object->*d_function)(args);
    }

private:
    MemberFunctionType d_function;
    T* d_object;
};

/*!
\brief
    Slot template class that creates a functor that calls back via a class
    member function. This variant doesn't require a handler that returns bool.

\note
    This functor always returns true to CEGUI, which means the event was
    handled.
*/
template<typename T>
class MemberFunctionSlotVoid : public SlotFunctorBase
{
public:
    //! Member function slot type.
    typedef void(T::*MemberFunctionType)(const EventArgs&);

    MemberFunctionSlotVoid(MemberFunctionType func, T* obj) :
        d_function(func),
        d_object(obj)
    {}

    virtual bool operator()(const EventArgs& args)
    {
        (d_object->*d_function)(args);

        return true;
    }

private:
    MemberFunctionType d_function;
    T* d_object;
};

/*!
\brief
    Slot template class that creates a functor that calls back via a class
    member function. This variant ignores passed EventArgs.
*/
template<typename T>
class MemberFunctionSlotNoArgs : public SlotFunctorBase
{
public:
    //! Member function slot type.
    typedef bool(T::*MemberFunctionType)();

    MemberFunctionSlotNoArgs(MemberFunctionType func, T* obj) :
        d_function(func),
        d_object(obj)
    {}

    virtual bool operator()(const EventArgs& /*args*/)
    {
        return (d_object->*d_function)();
    }

private:
    MemberFunctionType d_function;
    T* d_object;
};

/*!
\brief
    Slot template class that creates a functor that calls back via a class
    member function. This variant ignores passed EventArgs and the handler
    doesn't have to return a bool.

\note
    This functor always returns true to CEGUI, which means the event was
    handled.
*/
template<typename T>
class MemberFunctionSlotVoidNoArgs : public SlotFunctorBase
{
public:
    //! Member function slot type.
    typedef void(T::*MemberFunctionType)();

    MemberFunctionSlotVoidNoArgs(MemberFunctionType func, T* obj) :
        d_function(func),
        d_object(obj)
    {}

    virtual bool operator()(const EventArgs& /*args*/)
    {
        (d_object->*d_function)();

        return true;
    }

private:
    MemberFunctionType d_function;
    T* d_object;
};

}

#endif  // end of guard _CEGUIMemberFunctionSlot_h_
