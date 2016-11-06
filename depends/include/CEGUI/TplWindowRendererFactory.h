/***********************************************************************
    created:    Thu Mar 19 2009
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
#ifndef _CEGUITplWindowRendererFactory_h_
#define _CEGUITplWindowRendererFactory_h_

#include "CEGUI/WindowRenderer.h"

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Template based WindowRendererFactory that can be used to automatically
    generate a WindowRendererFactory given a WindowRenderer based class.

    The advantage of this over the previous macro based methods is that there is
    no longer any need to have any supporting code or structure in order to add
    new WindowRenderer types to the system, rather you can just do:
    \code
    CEGUI::WindowRendererManager::addFactory<TplWindowRendererFactory<MyWindowRenderer> >();
    \endcode

\tparam T
    Specifies the WindowRenderer based class that the factory will create and
    destroy instances of.
*/
template <typename T>
class TplWindowRendererFactory : public WindowRendererFactory
{
public:
    //! Default constructor.
    TplWindowRendererFactory();

    // Implement WindowRendererFactory interface
    WindowRenderer* create();
    void destroy(WindowRenderer* wr);
};

//----------------------------------------------------------------------------//
template <typename T>
TplWindowRendererFactory<T>::TplWindowRendererFactory() :
    WindowRendererFactory(T::TypeName)
{
}

//----------------------------------------------------------------------------//
template <typename T>
WindowRenderer* TplWindowRendererFactory<T>::create()
{
    return CEGUI_NEW_AO T(T::TypeName);
}

//----------------------------------------------------------------------------//
template <typename T>
void TplWindowRendererFactory<T>::destroy(WindowRenderer* wr)
{
    CEGUI_DELETE_AO wr;
}

//----------------------------------------------------------------------------//

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUITplWindowRendererFactory_h_
