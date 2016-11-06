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
#ifndef _CEGUIRadioButton_h_
#define _CEGUIRadioButton_h_

#include "CEGUI/widgets/ToggleButton.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

namespace CEGUI
{
//! Base class to provide the logic for Radio Button widgets.
class CEGUIEXPORT RadioButton : public ToggleButton
{
public:
    static const String WidgetTypeName;

    /*!
    \brief
        set the groupID for this radio button

    \param group
        ulong value specifying the radio button group that this widget
        belongs to.
    */
    void setGroupID(ulong group);

    /*!
    \brief
        return the groupID assigned to this radio button

    \return
        ulong value that identifies the Radio Button group this widget
        belongs to.
    */
    ulong getGroupID() const    { return d_groupID; }

    /*!
    \brief
        Return a pointer to the RadioButton object within the same group as this
        RadioButton, that is currently selected.

    \return
        Pointer to the RadioButton object that is the RadioButton within the
        same group as this RadioButton, and is attached to the same parent
        window as this RadioButton, that is currently selected. Returns 0 if no
        button within the group is selected, or if 'this' is not attached to a
        parent window.
    */
    RadioButton* getSelectedButtonInGroup() const;

    RadioButton(const String& type, const String& name);

protected:
    void deselectSiblingRadioButtonsInGroup() const;
    void addRadioButtonProperties();

    // overridden from ToggleButton
    bool getPostClickSelectState() const;
    void onSelectStateChange(WindowEventArgs& e);

    ulong d_groupID;
};

}

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif

