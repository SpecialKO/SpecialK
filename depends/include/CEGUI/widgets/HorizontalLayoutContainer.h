/***********************************************************************
    created:    29/7/2010
    author:     Martin Preisler

    purpose:    Interface to a Horizontal layout container
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
#ifndef _CEGUIHorizontalLayoutContainer_h_
#define _CEGUIHorizontalLayoutContainer_h_

#include "./SequentialLayoutContainer.h"
#include "../WindowFactory.h"

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    A Layout Container window layouting it's children Horizontally
*/
class CEGUIEXPORT HorizontalLayoutContainer : public SequentialLayoutContainer
{
public:
    /*************************************************************************
        Constants
    *************************************************************************/
    //! The unique typename of this widget
    static const String WidgetTypeName;

    /*************************************************************************
        Construction and Destruction
    *************************************************************************/
    /*!
    \brief
        Constructor for GUISheet windows.
    */
    HorizontalLayoutContainer(const String& type, const String& name);

    /*!
    \brief
        Destructor for GUISheet windows.
    */
    virtual ~HorizontalLayoutContainer(void);

    /// @copydoc LayoutContainer::layout
    virtual void layout();
};

} // End of  CEGUI namespace section


#endif  // end of guard _CEGUIHorizontalLayoutContainer_h_

