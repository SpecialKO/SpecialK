/************************************************************************
    created:    Tue Feb 28 2006
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
#ifndef _CEGUIFunctorReferenceSlot_h_
#define _CEGUIFunctorReferenceSlot_h_

#include "CEGUI/SlotFunctorBase.h"

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Slot template class that creates a functor that calls back via a functor
    object reference.
*/
template<typename T>
class FunctorReferenceSlot : public SlotFunctorBase
{
public:
    FunctorReferenceSlot(T& functor) :
        d_functor(functor)
    {}

    virtual bool operator()(const EventArgs& args)
    {
        return d_functor(args);
    }

private:
    T& d_functor;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIFunctorReferenceSlot_h_
