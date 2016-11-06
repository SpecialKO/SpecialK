/***********************************************************************
    created:    Fri Feb 17 2012
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
#ifndef _CEGUISystemKeys_h_
#define _CEGUISystemKeys_h_

#include "CEGUI/InputEvent.h"

namespace CEGUI
{
class CEGUIEXPORT SystemKeys
{
public:
    enum SystemKey
    {
        None          = 0x0000,
        LeftMouse     = 0x0001,
        RightMouse    = 0x0002,
        Shift         = 0x0004,
        Control       = 0x0008,
        MiddleMouse   = 0x0010,
        X1Mouse       = 0x0020,
        X2Mouse       = 0x0040,
        Alt           = 0x0080
    };

    SystemKeys();

    void reset();

    uint get() const;
    bool isPressed(SystemKey key) const;

    //! notify that the given key was presed
    void keyPressed(Key::Scan key);
    //! notify that the given key was released.
    void keyReleased(Key::Scan key);
    //! notify that the given mouse button was pressed.
    void mouseButtonPressed(MouseButton button);
    //! notify that the given mouse button was released.
    void mouseButtonReleased(MouseButton button);

    static SystemKey mouseButtonToSystemKey(MouseButton button);
    static SystemKey keyCodeToSystemKey(Key::Scan key);

private:
    void updatePressedStateForKey(Key::Scan key, bool state);
    void updateSystemKeyState(SystemKey syskey);

    uint d_current;

    bool d_leftShift;
    bool d_rightShift;
    bool d_leftCtrl;
    bool d_rightCtrl;
    bool d_leftAlt;
    bool d_rightAlt;
};

}

#endif

