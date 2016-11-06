/************************************************************************
    created:    03/23/2007
    author:     Lars 'Levia' Wesselius (Content Pane based on Tomas Lindquist Olsen's code)

    purpose:    Interface for the GroupBox widget
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2012 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIGroupBox_h_
#define _CEGUIGroupBox_h_

#include "../Base.h"
#include "../WindowFactory.h"
#include "./ClippedContainer.h"
#include <vector>

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

namespace CEGUI
{
/*!
\brief
    Base class for standard GroupBox widget.

\deprecated
    You should consider not using this class. It performs no useful function
    and can be replicated 100% accurately via an XML based WidgetLook
    definition.  If you are already using this and need to migrate, you can
    usually do so by changing the following:
    - in the looknfeel (WidgetLook) change the the \<Child\> element for
    "__auto_contentpane__" into a \<NamedArea\> with the name "inner_rect" (and
    remove anything other than the \<Area\> definition).
    - in the scheme file, change the targetType from "CEGUI/GroupBox" to
    "DefaultWindow".
*/
class CEGUIEXPORT GroupBox : public Window
{
public:
    //! Namespace for global events
    static const String EventNamespace;

    static const String WidgetTypeName;
    static const String ContentPaneName;


    GroupBox(const String& type, const String& name);


    /*!
    \brief
        Draws the GroupBox around a widget. The size and position of the
        GroupBox are overriden. Once the window that is drawn around resizes,
        you'll have to call the function again. FIXME
    */
    bool drawAroundWidget(const CEGUI::Window* wnd);
    bool drawAroundWidget(const String& name);

    /*!
    \brief
        Returns the content pane held by this GroupBox.

    \return
        Pointer to a Window instance.
    */
    Window * getContentPane() const;

protected:
    /*!
    \copydoc Window::addChild_impl
    */
    virtual void addChild_impl(Element* element);

    /*!
    \copydoc Window::removeChild_impl
    */
    virtual void removeChild_impl(Element* element);


};

}

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif

