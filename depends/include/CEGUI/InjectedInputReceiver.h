/***********************************************************************
    created:    Thu Feb 16 2012
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
#ifndef _CEGUIInjectedInputReceiver_h_
#define _CEGUIInjectedInputReceiver_h_

#include "CEGUI/InputEvent.h"

namespace CEGUI
{

//! Input injection interface to be inplemented by classes that take raw inputs
class CEGUIEXPORT InjectedInputReceiver
{
public:
    virtual ~InjectedInputReceiver() {}

    /*!
    \brief
        Function that injects a mouse movement event into the receiver.

    \param delta_x
        amount the mouse moved on the x axis.

    \param delta_y
        amount the mouse moved on the y axis.

    \return
        - true if the input was processed by the input receiver.
        - false if the input was not processed by the input receiver.
    */
    virtual bool injectMouseMove(float delta_x, float delta_y) = 0;

    /*!
    \brief
        Function that notifies that the mouse has left the host area that the
        receiver receives input for.

    \return
        - true if the event was handled.
        - false if the event was not handled.
    */
    virtual bool injectMouseLeaves() = 0;

    /*!
    \brief
        Function that injects a mouse button down event into the receiver.

    \param button
        One of the MouseButton values indicating which button was pressed.

    \return
        - true if the input was processed by the receiver.
        - false if the input was not processed by the receiver.
    */
    virtual bool injectMouseButtonDown(MouseButton button) = 0;

    /*!
    \brief
        Function that injects a mouse button up event into the receiver.

    \param button
        One of the MouseButton values indicating which button was released.

    \return
        - true if the input was processed by the receiver.
        - false if the input was not processed by the receiver.
    */
    virtual bool injectMouseButtonUp(MouseButton button) = 0;

    /*!
    \brief
        Function that injects a key down event into the receiver.

    \param key_code
        uint value indicating which key was pressed.

    \return
        - true if the input was processed by the receiver.
        - false if the input was not processed by the receiver.
    */
    virtual bool injectKeyDown(Key::Scan scan_code) = 0;

    /*!
    \brief
        Function that injects a key up event into the receiver.

    \param key_code
        Key::Scan value indicating which key was released.

    \return
        - true if the input was processed by the receiver.
        - false if the input was not processed by the receiver.
    */
    virtual bool injectKeyUp(Key::Scan scan_code) = 0;

    /*!
    \brief
        Function that injects a typed character event into the receiver.

    \param code_point
        Unicode or ASCII (depends on used String class) code point of the character that was typed.

    \return
        - true if the input was processed by the receiver.
        - false if the input was not processed by the receiver.
    */
    virtual bool injectChar(String::value_type code_point) = 0;

    /*!
    \brief
        Function that injects a mouse-wheel / scroll-wheel event into the receiver.

    \param delta
        float value representing the amount the wheel moved.

    \return
        - true if the input was processed by the receiver.
        - false if the input was not processed by the receiver.
    */
    virtual bool injectMouseWheelChange(float delta) = 0;

    /*!
    \brief
        Function that injects a new position for the mouse cursor.

    \param x_pos
        New absolute pixel position of the mouse cursor on the x axis.

    \param y_pos
        New absolute pixel position of the mouse cursoe in the y axis.

    \return
        - true if the generated mouse move event was handled.
        - false if the generated mouse move event was not handled.
    */
    virtual bool injectMousePosition(float x_pos, float y_pos) = 0;

    /*!
    \brief
        Function to inject time pulses into the receiver.

    \param timeElapsed
        float value indicating the amount of time passed, in seconds, since the last time this method was called.

    \return
        Currently, this method always returns true.
    */
    virtual bool injectTimePulse(float timeElapsed) = 0;

    /*!
    \brief
        Function to directly inject a mouse button click event.
        
        Here 'click' means a mouse button down event followed by a mouse
        button up event.

    \note
        Under normal, default settings, this event is automatically generated by
        the system from the regular up and down events you inject.  You may use
        this function directly, though you'll probably want to disable the
        automatic click event generation first by using the
        setMouseClickEventGenerationEnabled function - this setting controls the
        auto-generation of events and also determines the default 'handled'
        state of the injected click events according to the rules used for
        mouse up/down events.

    \param button
        One of the MouseButton enumerated values.
    
    \return
        - true if some window or handler reported that it handled the event.
        - false if nobody handled the event.
    */
    virtual bool injectMouseButtonClick(const MouseButton button) = 0;

    /*!
    \brief
        Function to directly inject a mouse button double-click event.
        
        Here 'double-click' means a single mouse button had the sequence down,
        up, down within a predefined period of time.

    \note
        Under normal, default settings, this event is automatically generated by
        the system from the regular up and down events you inject.  You may use
        this function directly, though you'll probably want to disable the
        automatic click event generation first by using the
        setMouseClickEventGenerationEnabled function - this setting controls the
        auto-generation of events and also determines the default 'handled'
        state of the injected click events according to the rules used for
        mouse up/down events.

    \param button
        One of the MouseButton enumerated values.
    
    \return
        - true if some window or handler reported that it handled the event.
        - false if nobody handled the event.
    */
    virtual bool injectMouseButtonDoubleClick(const MouseButton button) = 0;

    /*!
    \brief
        Function to directly inject a mouse button triple-click event.
        
        Here 'triple-click' means a single mouse button had the sequence down,
        up, down, up, down within a predefined period of time.

    \note
        Under normal, default settings, this event is automatically generated by
        the system from the regular up and down events you inject.  You may use
        this function directly, though you'll probably want to disable the
        automatic click event generation first by using the
        setMouseClickEventGenerationEnabled function - this setting controls the
        auto-generation of events and also determines the default 'handled'
        state of the injected click events according to the rules used for
        mouse up/down events.

    \param button
        One of the MouseButton enumerated values.
    
    \return
        - true if some window or handler reported that it handled the event.
        - false if nobody handled the event.
    */
    virtual bool injectMouseButtonTripleClick(const MouseButton button) = 0;
    
    /*!
    \brief
        Tells the receiver to perform a clipboard copy operation.
        
    \return
        - true if the copy was successful
        - false if the copy was denied
    */
    virtual bool injectCopyRequest() = 0;
    
    /*!
    \brief
        Tells the system to perform a clipboard cut operation.
     
    \return
        - true if the cut was successful
        - false if the cut was denied
    */
    virtual bool injectCutRequest() = 0;
    
    /*!
    \brief
        Tells the system to perform a clipboard paste operation.
     
    \return
        - true if the paste was successful
        - false if the paste was denied
    */
    virtual bool injectPasteRequest() = 0;
};

}

#endif

