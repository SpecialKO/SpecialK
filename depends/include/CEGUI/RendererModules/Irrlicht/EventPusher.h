/***********************************************************************
    created:    12/22/2004
    author:     Thomas Suter
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
#ifndef CCEGUIEVENTPUSHER_H_INCLUDED
#define CCEGUIEVENTPUSHER_H_INCLUDED

#include "../../CEGUI.h"
#include <irrlicht.h>

namespace CEGUI
{
using namespace irr;

class IrrlichtEventPusher
{
    gui::ICursorControl* d_cursorctrl;

public :
    IrrlichtEventPusher(irr::gui::ICursorControl* ctrl) :
        d_cursorctrl(ctrl)
    {
        initCodes();
    };

    virtual ~IrrlichtEventPusher(){};

    bool OnEvent(const SEvent& event)
    {
        switch (event.EventType)
        {
        case EET_KEY_INPUT_EVENT :
            if (event.KeyInput.PressedDown)
                return OnKeyDown(event.KeyInput.Key, event.KeyInput.Char, event.KeyInput.Control, event.KeyInput.Shift);
            else
                return OnKeyUp(event.KeyInput.Key, event.KeyInput.Char, event.KeyInput.Control, event.KeyInput.Shift);
            break;

        case EET_MOUSE_INPUT_EVENT :
            return OnMouse(event.MouseInput.X, event.MouseInput.Y, event.MouseInput.Wheel, event.MouseInput.Event);
            break;

        default:
            break;
        }

        return false;
    }

    bool OnKeyDown(EKEY_CODE key, wchar_t wch, bool /*ctrl*/, bool /*shift*/)
    {
        bool handled = false;
        CEGUI::GUIContext& cegui = CEGUI::System::getSingleton().getDefaultGUIContext();
        handled = cegui.injectKeyDown(getKeyCode(key));
        handled = cegui.injectChar(wch) || handled;
        return handled;
    }

    bool OnKeyUp(EKEY_CODE key, wchar_t /*wch*/, bool /*ctrl*/, bool /*shift*/)
    {
        bool handled = false;
        CEGUI::GUIContext& cegui = CEGUI::System::getSingleton().getDefaultGUIContext();
        handled = cegui.injectKeyUp(getKeyCode(key));
        return handled;
    }

    bool OnMouse(s32 x, s32 y, f32 w, EMOUSE_INPUT_EVENT e)
    {
        using namespace irr;
        bool handled = false;

        switch (e)
        {
            //! Left mouse button was pressed down.
        case EMIE_LMOUSE_PRESSED_DOWN:
            handled = CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(CEGUI::LeftButton);
            break;
            //! Right mouse button was pressed down.
        case EMIE_RMOUSE_PRESSED_DOWN:
            handled = CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(CEGUI::RightButton);
            break;
            //! Middle mouse button was pressed down.
        case EMIE_MMOUSE_PRESSED_DOWN:
            handled = CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(CEGUI::MiddleButton);
            break;
            //! Left mouse button was left up.
        case EMIE_LMOUSE_LEFT_UP:
            handled = CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(CEGUI::LeftButton);
            break;
            //! Right mouse button was left up.
        case EMIE_RMOUSE_LEFT_UP:
            handled = CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(CEGUI::RightButton);
            break;
            //! Middle mouse button was left up.
        case EMIE_MMOUSE_LEFT_UP:
            handled = CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(CEGUI::MiddleButton);
            break;
            //! The mouse cursor changed its position.
        case EMIE_MOUSE_MOVED:
            handled = CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition(
                        static_cast<float>(x), static_cast<float>(y));
            break;
            //! The mouse wheel was moved. Use Wheel value in event data to find out
            //! in what direction and how fast.
        case EMIE_MOUSE_WHEEL:
            handled = CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseWheelChange(w);
            break;
        default:
            break;
        }
        return handled;

    }

    /*! translate the irrlicht keycode to cegui keycode
    \param kc
    the irrlicht keycode

    \return
    the cegui keycode
    */
    CEGUI::Key::Scan getKeyCode(irr::EKEY_CODE kc) const
    {
        return irr2ceCODE[kc];
    }

protected:
    CEGUI::Key::Scan irr2ceCODE[irr::KEY_KEY_CODES_COUNT];

    void initCodes()
    {
        using namespace irr;
        memset(irr2ceCODE, Key::Unknown, KEY_KEY_CODES_COUNT);

        irr2ceCODE[KEY_LBUTTON   ] = Key::Unknown;  // Left mouse button
        irr2ceCODE[KEY_RBUTTON   ] = Key::Unknown;  // Right mouse button
        irr2ceCODE[KEY_CANCEL    ] = Key::Unknown;  // Control-break processing
        irr2ceCODE[KEY_MBUTTON   ] = Key::Unknown;  // Middle mouse button (three-button mouse)
        irr2ceCODE[KEY_XBUTTON1  ] = Key::Unknown;  // Windows 2000/XP: X1 mouse button
        irr2ceCODE[KEY_XBUTTON2  ] = Key::Unknown;  // Windows 2000/XP: X2 mouse button
        irr2ceCODE[KEY_BACK      ] = Key::Backspace; //0x08;  // BACKSPACE key
        irr2ceCODE[KEY_TAB       ] = Key::Tab; //0x09;  // TAB key
        irr2ceCODE[KEY_CLEAR     ] = Key::Unknown;  // CLEAR key
        irr2ceCODE[KEY_RETURN    ] = Key::Return; //0x0D;  // ENTER key
        irr2ceCODE[KEY_SHIFT     ] = Key::LeftShift;  // SHIFT key
        irr2ceCODE[KEY_CONTROL   ] = Key::LeftControl;   // CTRL key
        irr2ceCODE[KEY_MENU      ] = Key::LeftAlt;  // ALT key
        irr2ceCODE[KEY_PAUSE     ] = Key::Pause;   // PAUSE key
        irr2ceCODE[KEY_CAPITAL   ] = Key::Capital;  // CAPS LOCK key
        irr2ceCODE[KEY_KANA      ] = Key::Kana;  // IME Kana mode
        irr2ceCODE[KEY_HANGUEL   ] = Key::Unknown;  // IME Hanguel mode
        irr2ceCODE[KEY_HANGUL    ] = Key::Unknown;  // IME Hangul mode
        irr2ceCODE[KEY_JUNJA     ] = Key::Unknown;  // IME Junja mode
        irr2ceCODE[KEY_FINAL     ] = Key::Unknown;  // IME final mode
        irr2ceCODE[KEY_HANJA     ] = Key::Unknown;  // IME Hanja mode
        irr2ceCODE[KEY_KANJI     ] = Key::Unknown;  // IME Kanji mode
        irr2ceCODE[KEY_ESCAPE    ] = Key::Escape;  // ESC key
        irr2ceCODE[KEY_CONVERT   ] = Key::Convert;  // IME convert
        irr2ceCODE[KEY_NONCONVERT] = Key::NoConvert;  // IME nonconvert
        irr2ceCODE[KEY_ACCEPT    ] = Key::Unknown;  // IME accept
        irr2ceCODE[KEY_MODECHANGE] = Key::Unknown;  // IME mode change request
        irr2ceCODE[KEY_SPACE     ] = Key::Space;  // SPACEBAR
        irr2ceCODE[KEY_PRIOR     ] = Key::PageUp;  // PAGE UP key
        irr2ceCODE[KEY_NEXT      ] = Key::PageDown;  // PAGE DOWN key
        irr2ceCODE[KEY_END       ] = Key::End;  // END key
        irr2ceCODE[KEY_HOME      ] = Key::Home;  // HOME key
        irr2ceCODE[KEY_LEFT      ] = Key::ArrowLeft;  // LEFT ARROW key
        irr2ceCODE[KEY_UP        ] = Key::ArrowUp;  // UP ARROW key
        irr2ceCODE[KEY_RIGHT     ] = Key::ArrowRight;  // RIGHT ARROW key
        irr2ceCODE[KEY_DOWN      ] = Key::ArrowDown;  // DOWN ARROW key
        irr2ceCODE[KEY_SELECT    ] = Key::Unknown;  // SELECT key
        irr2ceCODE[KEY_PRINT     ] = Key::SysRq;  // PRINT key
        irr2ceCODE[KEY_EXECUT    ] = Key::Unknown;  // EXECUTE key
        irr2ceCODE[KEY_SNAPSHOT  ] = Key::Unknown;  // PRINT SCREEN key
        irr2ceCODE[KEY_INSERT    ] = Key::Insert;//0x2D;  // INS key
        irr2ceCODE[KEY_DELETE    ] = Key::Delete;//0x2E;  // DEL key
        irr2ceCODE[KEY_HELP      ] = Key::Unknown;  // HELP key
        irr2ceCODE[KEY_KEY_0     ] = Key::Zero;   // 0 key
        irr2ceCODE[KEY_KEY_1     ] = Key::One;    // 1 key
        irr2ceCODE[KEY_KEY_2     ] = Key::Two;    // 2 key
        irr2ceCODE[KEY_KEY_3     ] = Key::Three;  // 3 key
        irr2ceCODE[KEY_KEY_4     ] = Key::Four;   // 4 key
        irr2ceCODE[KEY_KEY_5     ] = Key::Five;   // 5 key
        irr2ceCODE[KEY_KEY_6     ] = Key::Six;    // 6 key
        irr2ceCODE[KEY_KEY_7     ] = Key::Seven;  // 7 key
        irr2ceCODE[KEY_KEY_8     ] = Key::Eight;  // 8 key
        irr2ceCODE[KEY_KEY_9     ] = Key::Nine;   // 9 key
        irr2ceCODE[KEY_KEY_A     ] = Key::A;  // A key
        irr2ceCODE[KEY_KEY_B     ] = Key::B;  // B key
        irr2ceCODE[KEY_KEY_C     ] = Key::C;  // C key
        irr2ceCODE[KEY_KEY_D     ] = Key::D;  // D key
        irr2ceCODE[KEY_KEY_E     ] = Key::E;  // E key
        irr2ceCODE[KEY_KEY_F     ] = Key::F;  // F key
        irr2ceCODE[KEY_KEY_G     ] = Key::G;  // G key
        irr2ceCODE[KEY_KEY_H     ] = Key::H;  // H key
        irr2ceCODE[KEY_KEY_I     ] = Key::I;  // I key
        irr2ceCODE[KEY_KEY_J     ] = Key::J;  // J key
        irr2ceCODE[KEY_KEY_K     ] = Key::K;  // K key
        irr2ceCODE[KEY_KEY_L     ] = Key::L;  // L key
        irr2ceCODE[KEY_KEY_M     ] = Key::M;  // M key
        irr2ceCODE[KEY_KEY_N     ] = Key::N;  // N key
        irr2ceCODE[KEY_KEY_O     ] = Key::O;  // O key
        irr2ceCODE[KEY_KEY_P     ] = Key::P;  // P key
        irr2ceCODE[KEY_KEY_Q     ] = Key::Q;  // Q key
        irr2ceCODE[KEY_KEY_R     ] = Key::R;  // R key
        irr2ceCODE[KEY_KEY_S     ] = Key::S;  // S key
        irr2ceCODE[KEY_KEY_T     ] = Key::T;  // T key
        irr2ceCODE[KEY_KEY_U     ] = Key::U;  // U key
        irr2ceCODE[KEY_KEY_V     ] = Key::V;  // V key
        irr2ceCODE[KEY_KEY_W     ] = Key::W;  // W key
        irr2ceCODE[KEY_KEY_X     ] = Key::X;  // X key
        irr2ceCODE[KEY_KEY_Y     ] = Key::Y;  // Y key
        irr2ceCODE[KEY_KEY_Z     ] = Key::Z;  // Z key
        irr2ceCODE[KEY_LWIN      ] = Key::LeftWindows;  // Left Windows key (Microsoft� Natural� keyboard)
        irr2ceCODE[KEY_RWIN      ] = Key::RightWindows;  // Right Windows key (Natural keyboard)
        irr2ceCODE[KEY_APPS      ] = Key::AppMenu;  //Applications key (Natural keyboard)
        irr2ceCODE[KEY_SLEEP     ] = Key::Sleep;  // Computer Sleep key
        irr2ceCODE[KEY_NUMPAD0   ] = Key::Numpad0;  // Numeric keypad 0 key
        irr2ceCODE[KEY_NUMPAD1   ] = Key::Numpad1;  // Numeric keypad 1 key
        irr2ceCODE[KEY_NUMPAD2   ] = Key::Numpad2;  // Numeric keypad 2 key
        irr2ceCODE[KEY_NUMPAD3   ] = Key::Numpad3;  // Numeric keypad 3 key
        irr2ceCODE[KEY_NUMPAD4   ] = Key::Numpad4;  // Numeric keypad 4 key
        irr2ceCODE[KEY_NUMPAD5   ] = Key::Numpad5;  // Numeric keypad 5 key
        irr2ceCODE[KEY_NUMPAD6   ] = Key::Numpad6;  // Numeric keypad 6 key
        irr2ceCODE[KEY_NUMPAD7   ] = Key::Numpad7;  // Numeric keypad 7 key
        irr2ceCODE[KEY_NUMPAD8   ] = Key::Numpad8;  // Numeric keypad 8 key
        irr2ceCODE[KEY_NUMPAD9   ] = Key::Numpad9;  // Numeric keypad 9 key
        irr2ceCODE[KEY_MULTIPLY  ] = Key::Multiply;  // Multiply key
        irr2ceCODE[KEY_ADD       ] = Key::Add;  // Add key
        irr2ceCODE[KEY_SEPARATOR ] = Key::Unknown;  // Separator key
        irr2ceCODE[KEY_SUBTRACT  ] = Key::Subtract;  // Subtract key
        irr2ceCODE[KEY_DECIMAL   ] = Key::Decimal;  // Decimal key
        irr2ceCODE[KEY_DIVIDE    ] = Key::Divide;  // Divide key
        irr2ceCODE[KEY_F1        ] = Key::F1;   // F1 key
        irr2ceCODE[KEY_F2        ] = Key::F2;   // F2 key
        irr2ceCODE[KEY_F3        ] = Key::F3;   // F3 key
        irr2ceCODE[KEY_F4        ] = Key::F4;   // F4 key
        irr2ceCODE[KEY_F5        ] = Key::F5;   // F5 key
        irr2ceCODE[KEY_F6        ] = Key::F6;   // F6 key
        irr2ceCODE[KEY_F7        ] = Key::F7;   // F7 key
        irr2ceCODE[KEY_F8        ] = Key::F8;   // F8 key
        irr2ceCODE[KEY_F9        ] = Key::F9;   // F9 key
        irr2ceCODE[KEY_F10       ] = Key::F10;  // F10 key
        irr2ceCODE[KEY_F11       ] = Key::F11;  // F11 key
        irr2ceCODE[KEY_F12       ] = Key::F12;  // F12 key
        irr2ceCODE[KEY_F13       ] = Key::F13;  // F13 key
        irr2ceCODE[KEY_F14       ] = Key::F14;  // F14 key
        irr2ceCODE[KEY_F15       ] = Key::F15;  // F15 key
        irr2ceCODE[KEY_F16       ] = Key::Unknown;  // F16 key
        irr2ceCODE[KEY_F17       ] = Key::Unknown;  // F17 key
        irr2ceCODE[KEY_F18       ] = Key::Unknown;  // F18 key
        irr2ceCODE[KEY_F19       ] = Key::Unknown;  // F19 key
        irr2ceCODE[KEY_F20       ] = Key::Unknown;  // F20 key
        irr2ceCODE[KEY_F21       ] = Key::Unknown;  // F21 key
        irr2ceCODE[KEY_F22       ] = Key::Unknown;  // F22 key
        irr2ceCODE[KEY_F23       ] = Key::Unknown;  // F23 key
        irr2ceCODE[KEY_F24       ] = Key::Unknown;  // F24 key
        irr2ceCODE[KEY_NUMLOCK   ] = Key::NumLock;  // NUM LOCK key
        irr2ceCODE[KEY_SCROLL    ] = Key::ScrollLock;  // SCROLL LOCK key
        irr2ceCODE[KEY_LSHIFT    ] = Key::LeftShift;  // Left SHIFT key
        irr2ceCODE[KEY_RSHIFT    ] = Key::RightShift;  // Right SHIFT key
        irr2ceCODE[KEY_LCONTROL  ] = Key::LeftControl;  // Left CONTROL key
        irr2ceCODE[KEY_RCONTROL  ] = Key::RightControl;  // Right CONTROL key
        irr2ceCODE[KEY_LMENU     ] = Key::LeftAlt;  // Left MENU key
        irr2ceCODE[KEY_RMENU     ] = Key::RightAlt;  // Right MENU key
        irr2ceCODE[KEY_COMMA     ] = Key::Comma;//0xBC;  // Comma Key  (;)
        irr2ceCODE[KEY_PLUS      ] = Key::Add;  // Plus Key   (+)
        irr2ceCODE[KEY_MINUS     ] = Key::Minus;  // Minus Key  (-)
        irr2ceCODE[KEY_PERIOD    ] = Key::Period;//0xBE;  // Period Key (.)
        irr2ceCODE[KEY_ATTN      ] = Key::Unknown;  // Attn key
        irr2ceCODE[KEY_CRSEL     ] = Key::Unknown;  // CrSel key
        irr2ceCODE[KEY_EXSEL     ] = Key::Unknown;  // ExSel key
        irr2ceCODE[KEY_EREOF     ] = Key::Unknown;  // Erase EOF key
        irr2ceCODE[KEY_PLAY      ] = Key::Unknown;  // Play key
        irr2ceCODE[KEY_ZOOM      ] = Key::Unknown;  // Zoom key
        irr2ceCODE[KEY_PA1       ] = Key::Unknown;  // PA1 key
        irr2ceCODE[KEY_OEM_CLEAR ] = Key::Unknown;  // Clear key
    }

};

}

#endif
