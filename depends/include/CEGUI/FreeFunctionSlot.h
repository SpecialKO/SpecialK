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
#ifndef _CEGUIFreeFunctionSlot_h_
#define _CEGUIFreeFunctionSlot_h_

#include "CEGUI/SlotFunctorBase.h"

// Start of CEGUI namespace section
namespace CEGUI
{

/*!
\brief
    Slot functor class that calls back via a free function pointer.
*/
class FreeFunctionSlot : public SlotFunctorBase
{
public:
    //! Slot function type.
    typedef bool (SlotFunction)(const EventArgs&);

    FreeFunctionSlot(SlotFunction* func) :
        d_function(func)
    {}

    virtual bool operator()(const EventArgs& args)
    {
        return d_function(args);
    }

private:
    SlotFunction* d_function;
};

/*!
\brief
    Slot functor class that calls back via a free function pointer.
    This variant doesn't require a handler that returns bool.

\note
    This functor always returns true to CEGUI, which means the event was
    handled.
*/
class FreeFunctionSlotVoid : public SlotFunctorBase
{
public:
    //! Slot function type.
    typedef void (SlotFunction)(const EventArgs&);

    FreeFunctionSlotVoid(SlotFunction* func) :
        d_function(func)
    {}

    virtual bool operator()(const EventArgs& args)
    {
        d_function(args);

        return true;
    }

private:
    SlotFunction* d_function;
};

/*!
\brief
    Slot functor class that calls back via a free function pointer.
    This variant ignores passed EventArgs.
*/
class FreeFunctionSlotNoArgs : public SlotFunctorBase
{
public:
    //! Slot function type.
    typedef bool (SlotFunction)();

    FreeFunctionSlotNoArgs(SlotFunction* func) :
        d_function(func)
    {}

    virtual bool operator()(const EventArgs& /*args*/)
    {
        return d_function();
    }

private:
    SlotFunction* d_function;
};

/*!
\brief
    Slot functor class that calls back via a free function pointer.
    This variant ignores passed EventArgs and the handler
    doesn't have to return a bool.

\note
    This functor always returns true to CEGUI, which means the event was
    handled.
*/
class FreeFunctionSlotVoidNoArgs : public SlotFunctorBase
{
public:
    //! Slot function type.
    typedef void (SlotFunction)();

    FreeFunctionSlotVoidNoArgs(SlotFunction* func) :
        d_function(func)
    {}

    virtual bool operator()(const EventArgs&)
    {
        d_function();

        return true;
    }

private:
    SlotFunction* d_function;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIFreeFunctionSlot_h_
