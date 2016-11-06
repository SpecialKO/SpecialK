/***********************************************************************
    created:    Tue Feb 28 2012
    author:     Paul D Turner <paul@cegui.org.uk>
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
#ifndef _CEGUIToggleButton_h_
#define _CEGUIToggleButton_h_

#include "CEGUI/widgets/ButtonBase.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

namespace CEGUI
{
//! Class providing logic buttons that can have their selected state toggled.
class CEGUIEXPORT ToggleButton : public ButtonBase
{
public:
    static const String EventNamespace;
    static const String WidgetTypeName;

    /** Event fired when then selected state of the ToggleButton changes.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the ToggleButton whose state has changed.
     */
    static const String EventSelectStateChanged;

    //!returns true if the toggle button is in the selected state.
    bool isSelected(void) const { return d_selected; }

    //! sets whether the toggle button is in the selected state.
    void setSelected(bool select);

    ToggleButton(const String& type, const String& name);

protected:
    virtual bool getPostClickSelectState() const;
    void addToggleButtonProperties();

    //! event triggered internally when toggle button select state changes.
    virtual void onSelectStateChange(WindowEventArgs& e);

    // base class overriddes
    void onMouseButtonUp(MouseEventArgs& e);

    bool d_selected;
};

}

#endif

