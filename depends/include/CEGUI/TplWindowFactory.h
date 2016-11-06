/***********************************************************************
    created:    Thu Mar 19 2009
    author:     Paul D Turner
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
#ifndef _CEGUITplWindowFactory_h_
#define _CEGUITplWindowFactory_h_

#include "CEGUI/WindowFactory.h"

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Template based WindowFactory that can be used to automatically generate a
    WindowFactory given a Window based class.

    The advantage of this over the previous macro based methods is that there is
    no longer any need to have any supporting code or structure in order to add
    new Window types to the system, rather you can just do:
    \code
    CEGUI::WindowFactoryManager::addFactory<TplWindowFactory<MyWidget> >();
    \endcode

\tparam T
    Specifies the Window based class that the factory will create and
    destroy instances of.
*/
template <typename T>
class TplWindowFactory : public WindowFactory
{
public:
    //! Default constructor.
    TplWindowFactory();

    // Implement WindowFactory interface
    Window* createWindow(const String& name);
    void destroyWindow(Window* window);
};

//----------------------------------------------------------------------------//
template <typename T>
TplWindowFactory<T>::TplWindowFactory() :
    WindowFactory(T::WidgetTypeName)
{
}

//----------------------------------------------------------------------------//
template <typename T>
Window* TplWindowFactory<T>::createWindow(const String& name)
{
    return CEGUI_NEW_AO T(d_type, name);
}

//----------------------------------------------------------------------------//
template <typename T>
void TplWindowFactory<T>::destroyWindow(Window* window)
{
    CEGUI_DELETE_AO window;
}

//----------------------------------------------------------------------------//

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUITplWindowFactory_h_
