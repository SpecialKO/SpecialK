/***********************************************************************
    created:    Fri Oct 07 2011
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
#ifndef _CEGUITplWindowFactoryRegisterer_h_
#define _CEGUITplWindowFactoryRegisterer_h_

#include "CEGUI/FactoryRegisterer.h"
#include "CEGUI/WindowFactoryManager.h"

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Template based implementation of FactoryRegisterer that allows easy
    registration of a factory for any Window type.
*/
template <typename T>
class TplWindowFactoryRegisterer : public FactoryRegisterer
{
public:
    //! Constructor.
    TplWindowFactoryRegisterer();

    void unregisterFactory() const;

protected:
    void doFactoryAdd() const;
    bool isAlreadyRegistered() const;
};


//----------------------------------------------------------------------------//
template <typename T>
TplWindowFactoryRegisterer<T>::TplWindowFactoryRegisterer() :
    FactoryRegisterer(T::WidgetTypeName)
{}

//----------------------------------------------------------------------------//
template <typename T>
void TplWindowFactoryRegisterer<T>::unregisterFactory() const
{
    WindowFactoryManager::getSingleton().removeFactory(d_type);
}

//----------------------------------------------------------------------------//
template <typename T>
void TplWindowFactoryRegisterer<T>::doFactoryAdd() const
{
    WindowFactoryManager::addWindowType<T>();
}

//----------------------------------------------------------------------------//
template <typename T>
bool TplWindowFactoryRegisterer<T>::isAlreadyRegistered() const
{
    return WindowFactoryManager::getSingleton().isFactoryPresent(d_type);
}

//----------------------------------------------------------------------------//

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUITplWindowFactoryRegisterer_h_

