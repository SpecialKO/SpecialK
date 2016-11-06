/***********************************************************************
    created:    Sat Dec 26 2009
    author:     Paul D Turner <paul@cegui.org.uk>
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2009 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIRenderEffectFactory_h_
#define _CEGUIRenderEffectFactory_h_

#include "CEGUI/RenderEffect.h"

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Interface for factory objects that create RenderEffect instances.
    Currently this interface is intended for internal use only.
*/
class RenderEffectFactory : public
    AllocatedObject<RenderEffectFactory>
{
public:
    //! base class virtual destructor.
    virtual ~RenderEffectFactory() {}

    //! Create an instance of the RenderEffect that this factory creates.
    virtual RenderEffect& create(Window* window) = 0;

    //! Destroy an instance of the RenderEffect that this factory creates.
    virtual void destroy(RenderEffect& effect) = 0;
};

//! Templatised RenderEffectFactory subclass used internally by the system.
template <typename T>
class TplRenderEffectFactory : public RenderEffectFactory
{
public:
    // Implement RenderEffectFactory interface
    RenderEffect& create(Window* window);
    void destroy(RenderEffect& effect);
};

//---------------------------------------------------------------------------//
template <typename T>
RenderEffect& TplRenderEffectFactory<T>::create(Window* window)
{
    return *CEGUI_NEW_AO T(window);
}

//---------------------------------------------------------------------------//
template <typename T>
void TplRenderEffectFactory<T>::destroy(RenderEffect& effect)
{
    CEGUI_DELETE_AO &effect;
}

//---------------------------------------------------------------------------//

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIRenderEffectFactory_h_

