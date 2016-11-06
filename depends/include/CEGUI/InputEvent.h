/***********************************************************************
	created:	30/5/2004
	author:		Paul D Turner

	purpose:	Defines interface to input event classes
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2006 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIInputEvent_h_
#define _CEGUIInputEvent_h_

#include "CEGUI/Base.h"
#include "CEGUI/EventArgs.h"
#include "CEGUI/String.h"
#include "CEGUI/Vector.h"
#include "CEGUI/Size.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
	struct to give scope to scan code enumeration.
*/
struct CEGUIEXPORT Key
{
	// enumeration of scan codes.  Customised from table taken from Ogre, which took them from DirectInput...
	enum Scan
    {
        Unknown         =0x00,
        Escape          =0x01,
        One             =0x02,
        Two             =0x03,
        Three           =0x04,
        Four            =0x05,
        Five            =0x06,
        Six             =0x07,
        Seven           =0x08,
        Eight           =0x09,
        Nine            =0x0A,
        Zero            =0x0B,
        Minus           =0x0C,    /* - on main keyboard */
        Equals			=0x0D,
        Backspace		=0x0E,    /* backspace */
        Tab				=0x0F,
        Q               =0x10,
        W               =0x11,
        E               =0x12,
        R               =0x13,
        T               =0x14,
        Y               =0x15,
        U               =0x16,
        I               =0x17,
        O               =0x18,
        P               =0x19,
        LeftBracket     =0x1A,
        RightBracket    =0x1B,
        Return			=0x1C,    /* Enter on main keyboard */
        LeftControl		=0x1D,
        A               =0x1E,
        S               =0x1F,
        D               =0x20,
        F               =0x21,
        G               =0x22,
        H               =0x23,
        J               =0x24,
        K               =0x25,
        L               =0x26,
        Semicolon       =0x27,
        Apostrophe		=0x28,
        Grave           =0x29,    /* accent grave */
        LeftShift       =0x2A,
        Backslash       =0x2B,
        Z               =0x2C,
        X               =0x2D,
        C               =0x2E,
        V               =0x2F,
        B               =0x30,
        N               =0x31,
        M               =0x32,
        Comma           =0x33,
        Period          =0x34,    /* . on main keyboard */
        Slash           =0x35,    /* '/' on main keyboard */
        RightShift      =0x36,
        Multiply        =0x37,    /* * on numeric keypad */
        LeftAlt        =0x38,    /* left Alt */
        Space           =0x39,
        Capital         =0x3A,
        F1              =0x3B,
        F2              =0x3C,
        F3              =0x3D,
        F4              =0x3E,
        F5              =0x3F,
        F6              =0x40,
        F7              =0x41,
        F8              =0x42,
        F9              =0x43,
        F10             =0x44,
        NumLock         =0x45,
        ScrollLock      =0x46,    /* Scroll Lock */
        Numpad7         =0x47,
        Numpad8         =0x48,
        Numpad9         =0x49,
        Subtract        =0x4A,    /* - on numeric keypad */
        Numpad4         =0x4B,
        Numpad5         =0x4C,
        Numpad6         =0x4D,
        Add				=0x4E,    /* + on numeric keypad */
        Numpad1         =0x4F,
        Numpad2         =0x50,
        Numpad3         =0x51,
        Numpad0         =0x52,
        Decimal			=0x53,    /* . on numeric keypad */
        OEM_102         =0x56,    /* < > | on UK/Germany keyboards */
        F11             =0x57,
        F12             =0x58,
        F13             =0x64,    /*                     (NEC PC98) */
        F14             =0x65,    /*                     (NEC PC98) */
        F15             =0x66,    /*                     (NEC PC98) */
        Kana            =0x70,    /* (Japanese keyboard)            */
        ABNT_C1         =0x73,    /* / ? on Portugese (Brazilian) keyboards */
        Convert         =0x79,    /* (Japanese keyboard)            */
        NoConvert       =0x7B,    /* (Japanese keyboard)            */
        Yen             =0x7D,    /* (Japanese keyboard)            */
        ABNT_C2         =0x7E,    /* Numpad . on Portugese (Brazilian) keyboards */
        NumpadEquals    =0x8D,    /* = on numeric keypad (NEC PC98) */
        PrevTrack       =0x90,    /* Previous Track (KC_CIRCUMFLEX on Japanese keyboard) */
        At              =0x91,    /*                     (NEC PC98) */
        Colon           =0x92,    /*                     (NEC PC98) */
        Underline       =0x93,    /*                     (NEC PC98) */
        Kanji           =0x94,    /* (Japanese keyboard)            */
        Stop            =0x95,    /*                     (NEC PC98) */
        AX              =0x96,    /*                     (Japan AX) */
        Unlabeled       =0x97,    /*                        (J3100) */
        NextTrack       =0x99,    /* Next Track */
        NumpadEnter     =0x9C,    /* Enter on numeric keypad */
        RightControl    =0x9D,
        Mute            =0xA0,    /* Mute */
        Calculator      =0xA1,    /* Calculator */
        PlayPause       =0xA2,    /* Play / Pause */
        MediaStop       =0xA4,    /* Media Stop */
        VolumeDown      =0xAE,    /* Volume - */
        VolumeUp        =0xB0,    /* Volume + */
        WebHome         =0xB2,    /* Web home */
        NumpadComma     =0xB3,    /* , on numeric keypad (NEC PC98) */
        Divide          =0xB5,    /* / on numeric keypad */
        SysRq           =0xB7,
        RightAlt        =0xB8,    /* right Alt */
        Pause           =0xC5,    /* Pause */
        Home            =0xC7,    /* Home on arrow keypad */
        ArrowUp         =0xC8,    /* UpArrow on arrow keypad */
        PageUp          =0xC9,    /* PgUp on arrow keypad */
        ArrowLeft       =0xCB,    /* LeftArrow on arrow keypad */
        ArrowRight      =0xCD,    /* RightArrow on arrow keypad */
        End             =0xCF,    /* End on arrow keypad */
        ArrowDown       =0xD0,    /* DownArrow on arrow keypad */
        PageDown		=0xD1,    /* PgDn on arrow keypad */
        Insert          =0xD2,    /* Insert on arrow keypad */
        Delete          =0xD3,    /* Delete on arrow keypad */
        LeftWindows     =0xDB,    /* Left Windows key */
        RightWindows    =0xDC,    /* Right Windows key - Correct spelling :) */
        AppMenu         =0xDD,    /* AppMenu key */
        Power           =0xDE,    /* System Power */
        Sleep           =0xDF,    /* System Sleep */
        Wake			=0xE3,    /* System Wake */
        WebSearch		=0xE5,    /* Web Search */
        WebFavorites	=0xE6,    /* Web Favorites */
        WebRefresh		=0xE7,    /* Web Refresh */
        WebStop			=0xE8,    /* Web Stop */
        WebForward		=0xE9,    /* Web Forward */
        WebBack			=0xEA,    /* Web Back */
        MyComputer		=0xEB,    /* My Computer */
        Mail			=0xEC,    /* Mail */
        MediaSelect		=0xED     /* Media Select */
    };

};


/*!
\brief
    Enumeration of mouse buttons
*/
enum MouseButton
{
    //! The left mouse button.
    LeftButton,
    //! The right mouse button.
    RightButton,
    //! The middle mouse button.
    MiddleButton,
    //! The first 'extra' mouse button.
    X1Button,
    //! The second 'extra' mouse button.
    X2Button,
    //! Value that equals the number of mouse buttons supported by CEGUI.
    MouseButtonCount,
    //! Value set for no mouse button.  NB: This is not 0, do not assume!
    NoButton
};


/*!
\brief
	System key flag values
*/
enum SystemKey
{
	LeftMouse		= 0x0001,			//!< The left mouse button.
	RightMouse		= 0x0002,			//!< The right mouse button.
	Shift			= 0x0004,			//!< Either shift key.
	Control			= 0x0008,			//!< Either control key.
	MiddleMouse		= 0x0010,			//!< The middle mouse button.
	X1Mouse			= 0x0020,			//!< The first 'extra' mouse button
	X2Mouse			= 0x0040,			//!< The second 'extra' mouse button.
	Alt				= 0x0080,			//!< Either alt key.
    InvalidSysKey   = 0x8000
};


/*!
\brief
	EventArgs based class that is used for objects passed to handlers triggered for events
	concerning some Window object.
*/
class CEGUIEXPORT WindowEventArgs : public EventArgs
{
public:
	WindowEventArgs(Window* wnd) : window(wnd) {}

	Window*	window;		//!< pointer to a Window object of relevance to the event.
};

/*!
\brief
	WindowEventArgs class that is primarily used by lua scripts
*/
class CEGUIEXPORT UpdateEventArgs : public WindowEventArgs
{
public:
    UpdateEventArgs(Window* window, float tslf) :
        WindowEventArgs(window),
        d_timeSinceLastFrame(tslf)
    {}

	float d_timeSinceLastFrame; //!< Time since the last frame update
};


/*!
\brief
	EventArgs based class that is used for objects passed to input event handlers
	concerning mouse input.
*/
class CEGUIEXPORT MouseEventArgs : public WindowEventArgs
{
public:
	MouseEventArgs(Window* wnd) : WindowEventArgs(wnd) {}

	Vector2f	position;		//!< holds current mouse position.
	Vector2f	moveDelta;		//!< holds variation of mouse position from last mouse input
	MouseButton	button;			//!< one of the MouseButton enumerated values describing the mouse button causing the event (for button inputs only)
	uint		sysKeys;		//!< current state of the system keys and mouse buttons.
	float		wheelChange;	//!< Holds the amount the scroll wheel has changed.
	uint        clickCount;     //!< Holds number of mouse button down events currently counted in a multi-click sequence (for button inputs only).
};


/*!
\brief
	EventArgs based class that is used for objects passed to input event handlers
	concerning mouse cursor events.
*/
class CEGUIEXPORT MouseCursorEventArgs : public EventArgs
{
public:
	MouseCursorEventArgs(MouseCursor* cursor) : mouseCursor(cursor) {}

	MouseCursor* mouseCursor;	//!< pointer to a MouseCursor object of relevance to the event.
	const Image* image;			//!< pointer to an Image object of relevance to the event.
};


/*!
\brief
	EventArgs based class that is used for objects passed to input event handlers
	concerning keyboard input.
*/
class CEGUIEXPORT KeyEventArgs : public WindowEventArgs
{
public:
	KeyEventArgs(Window* wnd) : WindowEventArgs(wnd) {}

	String::value_type codepoint; //!< utf32 or char (depends on used String class) codepoint for the key (only used for Character inputs).
	Key::Scan          scancode;  //!< Scan code of key that caused event (only used for key up & down inputs.
	uint               sysKeys;   //!< current state of the system keys and mouse buttons.
};


/*!
\brief
	EventArgs based class that is used for Activated and Deactivated window events
*/
class CEGUIEXPORT ActivationEventArgs : public WindowEventArgs
{
public:
	ActivationEventArgs(Window* wnd) : WindowEventArgs(wnd) {}

	Window*	otherWindow;	//!< Pointer to the other window involved in the activation change.
};

/*!
\brief
	EventArgs based class used for certain drag/drop notifications
*/
class CEGUIEXPORT DragDropEventArgs : public WindowEventArgs
{
public:
	DragDropEventArgs(Window* wnd) : WindowEventArgs(wnd) {}
	DragContainer*	dragDropItem; //!< pointer to the DragContainer window being dragged / dropped.
};

/*!
\brief
    EventArgs based class that is used for notifications regarding the main
    display.
*/
class CEGUIEXPORT DisplayEventArgs : public EventArgs
{
public:
    DisplayEventArgs(const Sizef& sz):
        size(sz)
    {}

    //! current / new size of the display.
    Sizef size;
};

//! EventArgs based class that is used for notifications regarding resources.
class CEGUIEXPORT ResourceEventArgs : public EventArgs
{
public:
    ResourceEventArgs(const String& type, const String& name) :
       resourceType(type),
       resourceName(name)
    {}

    //! String identifying the resource type this notification is about.
    String resourceType;
    //! String identifying the name of the resource this notification is about.
    String resourceName;
};

//! EventArgs based class that is used for notifications regarding Font objects.
class CEGUIEXPORT FontEventArgs : public EventArgs
{
public:
    FontEventArgs(Font* font) :
       font(font)
    {}

    //! Pointer to the font object related to the event notification.
    Font* font;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUIInputEvent_h_
