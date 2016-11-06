/***********************************************************************
    created:    21/2/2004
    author:     Paul D Turner

    purpose:    Defines abstract base class for Window objects
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

#ifndef _CEGUIWindow_h_
#define _CEGUIWindow_h_

#include "CEGUI/Base.h"
#include "CEGUI/NamedElement.h"
#include "CEGUI/Vector.h"
#include "CEGUI/Quaternion.h"
#include "CEGUI/Rect.h"
#include "CEGUI/Size.h"
#include "CEGUI/EventSet.h"
#include "CEGUI/PropertySet.h"
#include "CEGUI/TplWindowProperty.h"
#include "CEGUI/System.h"
#include "CEGUI/GUIContext.h"
#include "CEGUI/InputEvent.h"
#include "CEGUI/UDim.h"
#include "CEGUI/WindowRenderer.h"
#include "CEGUI/TextUtils.h"
#include "CEGUI/BasicRenderedStringParser.h"
#include "CEGUI/DefaultRenderedStringParser.h"
#include <vector>
#include <set>

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif


namespace CEGUI
{

/*!
\brief
    Enumerated type used for specifying Window::update mode to be used.  Note
    that the setting specified will also have an effect on child window
    content; for WUM_NEVER and WUM_VISIBLE, if the parent's update function is
    not called, then no child window will have it's update function called
    either - even if it specifies WUM_ALWAYS as it's WindowUpdateMode.
*/
enum WindowUpdateMode
{
    //! Always call the Window::update function for this window.
    WUM_ALWAYS,
    //! Never call the Window::update function for this window.
    WUM_NEVER,
    //! Only call the Window::update function for this window if it is visible.
    WUM_VISIBLE
};

template<>
class PropertyHelper<WindowUpdateMode>
{
public:
    typedef WindowUpdateMode return_type;
    typedef return_type safe_method_return_type;
    typedef WindowUpdateMode pass_type;
    typedef String string_return_type;

    static const String& getDataTypeName()
    {
        static String type("WindowUpdateMode");

        return type;
    }

    static return_type fromString(const String& str)
    {
    
        if (str == "Always")
        {
            return WUM_ALWAYS;
        }
        else if (str == "Never")
        {
            return WUM_NEVER;
        }
        else
        {
            return WUM_VISIBLE;
        }
    }

    static string_return_type toString(pass_type val)
    {
        if (val == WUM_ALWAYS)
        {
            return "Always";
        }
        else if (val == WUM_NEVER)
        {
            return "Never";
        }
        else if (val == WUM_VISIBLE)
        {
            return "Visible";
        }
        else
        {
            assert(false && "Invalid Window Update Mode");
            return "Always";
        }
    }
};


/*!
\brief
    An abstract base class providing common functionality and specifying the
    required interface for derived classes.

    The Window base class is core UI object class that the the system knows
    about; for this reason, every other window, widget, or similar item within
    the system must be derived from Window.

    The base class provides the common functionality required by all UI objects,
    and specifies the minimal interface required to be implemented by derived
    classes.
*/
class CEGUIEXPORT Window :
    public NamedElement
{
public:
    /*************************************************************************
        Property name constants
    *************************************************************************/
    //! Name of property to access for the alpha value of the Window.
    static const String AlphaPropertyName;
    //! Name of property to access for the 'always on top' setting for the Window
    static const String AlwaysOnTopPropertyName;
    //! Name of property to access for the 'clipped by parent' setting for the Window
    static const String ClippedByParentPropertyName;
    //! Name of property to access for the 'destroyed by parent' setting for the Window
    static const String DestroyedByParentPropertyName;
    //! Name of property to access for the 'disabled state' setting for the Window
    static const String DisabledPropertyName;
    //! Name of property to access for the font for the Window.
    static const String FontPropertyName;
    //! Name of property to access for the ID value of the Window.
    static const String IDPropertyName;
    //! Name of property to access for the get/set the 'inherits alpha' setting for the Window.
    static const String InheritsAlphaPropertyName;
    //! Name of property to access for the the mouse cursor image for the Window.
    static const String MouseCursorImagePropertyName;
    //! Name of property to access for the the 'visible state' setting for the Window.
    static const String VisiblePropertyName;
    //! Name of property to access for the 'restore old capture' setting for the Window.
    static const String RestoreOldCapturePropertyName;
    //! Name of property to access for the text / caption for the Window.
    static const String TextPropertyName;
    //! Name of property to access for the 'z-order changing enabled' setting for the Window. 
    static const String ZOrderingEnabledPropertyName;
    //! Name of property to access for whether the window will receive double-click and triple-click events.
    static const String WantsMultiClickEventsPropertyName;
    //! Name of property to access for whether the window will receive autorepeat mouse button down events.
    static const String MouseAutoRepeatEnabledPropertyName;
    //! Name of property to access for the autorepeat delay.
    static const String AutoRepeatDelayPropertyName;
    //! Name of property to access for the autorepeat rate.
    static const String AutoRepeatRatePropertyName;
    //! Name of property to access for the whether captured inputs are passed to child windows.
    static const String DistributeCapturedInputsPropertyName;
    //! Name of property to access for the custom tooltip for the window. 
    static const String TooltipTypePropertyName;
    //! Name of property to access for the tooltip text for the window.
    static const String TooltipTextPropertyName;
    //! Name of property to access for the window inherits its parents tooltip text when it has none of its own.
    static const String InheritsTooltipTextPropertyName;
    //! Name of property to access for the window will come to the top of the Z-order when clicked.
    static const String RiseOnClickEnabledPropertyName;
    //! Name of property to access for the window ignores mouse events and pass them through to any windows behind it.
    static const String MousePassThroughEnabledPropertyName;
    //! Name of property to access for the Window will receive drag and drop related notifications.
    static const String DragDropTargetPropertyName;
    //! Name of property to access for the Window will automatically attempt to use a full imagery caching RenderingSurface (if supported by the renderer).
    static const String AutoRenderingSurfacePropertyName;
    //! Name of property to access for the text parsing setting for the Window.
    static const String TextParsingEnabledPropertyName;
    //! Name of property to access for the margin for the Window.
    static const String MarginPropertyName;
    //! Name of property to access for the window update mode setting.
    static const String UpdateModePropertyName;
    //! Name of property to access whether unhandled mouse inputs should be propagated back to the Window's parent. 
    static const String MouseInputPropagationEnabledPropertyName;
    //! Name of property to access whether the system considers this window to be an automatically created sub-component window.
    static const String AutoWindowPropertyName;

    /*************************************************************************
        Event name constants
    *************************************************************************/
    //! Namespace for global events
    static const String EventNamespace;

    // generated internally by Window
    /** Event fired as part of the time based update of the window.
     * Handlers are passed a const UpdateEventArgs reference.
     */
    static const String EventUpdated;
    
    /** Event fired when the text string for the Window has changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Window whose text was changed.
     */
    static const String EventTextChanged;
    /** Event fired when the Font object for the Window has been changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Window whose font was changed.
     */
    static const String EventFontChanged;
    /** Event fired when the Alpha blend value for the Window has changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Window whose alpha value was changed.
     */
    static const String EventAlphaChanged;
    /** Event fired when the client assigned ID for the Window has changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Window whose ID was changed.
     */
    static const String EventIDChanged;
    /** Event fired when the Window has been activated and has input focus.
     * Handlers are passed a const ActivationEventArgs reference with
     * WindowEventArgs::window set to the Window that is gaining activation and
     * ActivationEventArgs::otherWindow set to the Window that is losing
     * activation (may be 0).
     */
    static const String EventActivated;
    /** Event fired when the Window has been deactivated, losing input focus.
     * Handlers are passed a const ActivationEventArgs reference with
     * WindowEventArgs::window set to the Window that is losing activation and
     * ActivationEventArgs::otherWindow set to the Window that is gaining
     * activation (may be 0).
     */
    static const String EventDeactivated;
    /** Event fired when the Window is shown (made visible).
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Window that was shown.
     */
    static const String EventShown;
    /** Event fired when the Window is made hidden.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Window that was hidden.
     */
    static const String EventHidden;
    /** Event fired when the Window is enabled so interaction is possible.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Window that was enabled.
     */
    static const String EventEnabled;
    /** Event fired when the Window is disabled and interaction is no longer
     * possible.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Window that was disabled.
     */
    static const String EventDisabled;
    /** Event fired when the Window clipping mode is modified.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Window whose clipping mode was
     * changed.
     */
    static const String EventClippedByParentChanged;
    /** Event fired when the Window destruction mode is modified.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Window whose destruction mode was
     * changed.
     */
    static const String EventDestroyedByParentChanged;
    /** Event fired when the Window mode controlling inherited alpha is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Window whose alpha inheritence mode
     * was changed.
     */
    static const String EventInheritsAlphaChanged;
    /** Event fired when the always on top setting for the Window is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Window whose always on top setting
     * was changed.
     */
    static const String EventAlwaysOnTopChanged;
    /** Event fired when the Window gains capture of mouse inputs.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Window that has captured mouse inputs.
     */
    static const String EventInputCaptureGained;
    /** Event fired when the Window loses capture of mouse inputs.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to either:
     * - the Window that has lost capture of mouse inputs if that event was
     *   caused by the window itself releasing the capture.
     * - the Window that is @gaining capture of mouse inputs if that is the
     *   cause of the previous window with capture losing that capture.
     */
    static const String EventInputCaptureLost;
    /** Event fired when the Window has been invalidated.
     * When a window is invalidated its cached rendering geometry is cleared,
     * the rendering surface that recieves the window's output is invalidated
     * and the window's target GUIContext is marked as dirty; this causes all
     * objects involved in the display of the window to be refreshed the next
     * time that the GUIContext::draw function is called.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Window that has been invalidated.
     */
    static const String EventInvalidated;
    /** Event fired when rendering of the Window has started.  In this context
     * 'rendering' is the population of the GeometryBuffer with geometry for the
     * window, not the actual rendering of that GeometryBuffer content to the 
     * display.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Window whose rendering has started.
     */
    static const String EventRenderingStarted;
    /** Event fired when rendering of the Window has ended.  In this context
     * 'rendering' is the population of the GeometryBuffer with geometry for the
     * window, not the actual rendering of that GeometryBuffer content to the 
     * display.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Window whose rendering has ended.
     */
    static const String EventRenderingEnded;
    /** Event fired when destruction of the Window is about to begin.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Window that is about to be destroyed.
     */
    static const String EventDestructionStarted;
    /** Event fired when a DragContainer is dragged in to the window's area.
     * Handlers are passed a const DragDropEventArgs reference with
     * WindowEventArgs::window set to the window over which a DragContainer has
     * been dragged (the receiving window) and DragDropEventArgs::dragDropItem
     * set to the DragContainer that was dragged in to the receiving window's
     * area.
     */
    static const String EventDragDropItemEnters;
    /** Event fired when a DragContainer is dragged out of the window's area.
     * Handlers are passed a const DragDropEventArgs reference with
     * WindowEventArgs::window set to the window over which a DragContainer has
     * been dragged out of (the receiving window) and
     * DragDropEventArgs::dragDropItem set to the DragContainer that was dragged
     * out of the receiving window's area.
     */
    static const String EventDragDropItemLeaves;
    /** Event fired when a DragContainer is dropped within the window's area.
     * Handlers are passed a const DragDropEventArgs reference with
     * WindowEventArgs::window set to the window over which a DragContainer was
     * dropped (the receiving window) and DragDropEventArgs::dragDropItem set to
     * the DragContainer that was dropped within the receiving window's area.
     */
    static const String EventDragDropItemDropped;
    /** Event fired when a WindowRenderer object is attached to the window.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the window that had the WindowRenderer
     * attached to it.
     */
    static const String EventWindowRendererAttached;
    /** Event fired when a WindowRenderer object is detached from the window.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the window that had the WindowRenderer
     * detached from it.
     */
    static const String EventWindowRendererDetached;
    /** Event fired when the Window's setting controlling parsing of it's text
     * string is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Window whose text parsing setting was
     * changed.
     */
    static const String EventTextParsingChanged;
    /** Event fired when the Window's margin has changed (any of the four margins)
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Window whose margin was
     * changed.
     */
    static const String EventMarginChanged;

    // generated externally (inputs)
    /** Event fired when the mouse cursor has entered the Window's area.
     * Handlers are passed a const MouseEventArgs reference with all fields
     * valid.
     */
    static const String EventMouseEntersArea;
    /** Event fired when the mouse cursor has left the Window's area.
     * Handlers are passed a const MouseEventArgs reference with all fields
     * valid.
     */
    static const String EventMouseLeavesArea;
    /** Event fired when the mouse cursor enters the Window's area.
     * Handlers are passed a const MouseEventArgs reference with all fields
     * valid.
     *\note This event is fired if - and only if - the mouse cursor is actually
     * over some part of this Window's surface area, and will not fire for
     * example if the location of the mouse is over some child window (even
     * though the mouse is technically also within the area of this Window).
     * For an alternative version of this event see the
     * Window::EventMouseEntersArea event.
     */
    static const String EventMouseEntersSurface;
    /** Event fired when the mouse cursor is no longer over the Window's surface
     * area.
     * Handlers are passed a const MouseEventArgs reference with all fields
     * valid.
     *\note This event will fire whenever the mouse is no longer actually over
     * some part of this Window's surface area, for example if the mouse is
     * moved over some child window (even though technically the mouse has not
     * actually 'left' this Window's area).  For an alternative version of this
     * event see the Window::EventMouseLeavesArea event.
     */
    static const String EventMouseLeavesSurface;
    /** Event fired when the mouse cursor moves within the area of the Window.
     * Handlers are passed a const MouseEventArgs reference with all fields
     * valid.
     */
    static const String EventMouseMove;
    /** Event fired when the mouse wheel is scrolled when the mouse cursor is
     * within the Window's area.
     * Handlers are passed a const MouseEventArgs reference with all fields
     * valid.
     */
    static const String EventMouseWheel;
    /** Event fired when a mouse button is pressed down within the Window.
     * Handlers are passed a const MouseEventArgs reference with all fields
     * valid.
     */
    static const String EventMouseButtonDown;
    /** Event fired when a mouse button is released within the Window.
     * Handlers are passed a const MouseEventArgs reference with all fields
     * valid.
     */
    static const String EventMouseButtonUp;
    /** Event fired when a mouse button is clicked - that is, pressed down and
     * released within a specific time interval - while the mouse cursor is
     * within the Window's area.
     * Handlers are passed a const MouseEventArgs reference with all fields
     * valid.
     */
    static const String EventMouseClick;
    /** Event fired when a mouse button is double-clicked while the mouse cursor
     * is within the Window's area.
     * Handlers are passed a const MouseEventArgs reference with all fields
     * valid.
     */
    static const String EventMouseDoubleClick;
    /** Event fired when a mouse button is triple-clicked while the mouse cursor
     * is within the Window's area.
     * Handlers are passed a const MouseEventArgs reference with all fields
     * valid.
     */
    static const String EventMouseTripleClick;
    /** Event fired when a key on the keyboard was pressed down while the window
     * had input focus.
     * Handlers are passed a const KeyEventArgs reference with
     * WindowEventArgs::window set to the Window receiving the key press,
     * KeyEventArgs::scancode set to the Key::Scan value of the key that was
     * pressed, and KeyEventArgs::sysKeys set to the combination of ::SystemKey
     * values active when the key was pressed.
     */
    static const String EventKeyDown;
    /** Event fired when a key on the keyboard was released while the window
     * had input focus.
     * Handlers are passed a const KeyEventArgs reference with
     * WindowEventArgs::window set to the Window receiving the key release,
     * KeyEventArgs::scancode set to the Key::Scan value of the key that was
     * released, and KeyEventArgs::sysKeys set to the combination of ::SystemKey
     * values active when the key was released.
     */
    static const String EventKeyUp;
    /** Event fired when the Window receives a character key input event.
     * Handlers are passed a const KeyEventArgs reference with
     * WindowEventArgs::window set to the Window receiving the character input,
     * KeyEventArgs::codepoint set to the Unicode UTF32 / UCS-4 value for the
     * input, and KeyEventArgs::sysKeys set to the combination of ::SystemKey
     * values active when the character input was received.
     */
    static const String EventCharacterKey;

    /*************************************************************************
        Child Widget name suffix constants
    *************************************************************************/
    //! Widget name suffix for automatically created tooltip widgets.
    static const String TooltipNameSuffix;

    // XML element and attribute names that relate to Window.
	static const String WindowXMLElementName;
    static const String AutoWindowXMLElementName;
    static const String UserStringXMLElementName;
	static const String WindowTypeXMLAttributeName;
	static const String WindowNameXMLAttributeName;
    static const String AutoWindowNamePathXMLAttributeName;
    static const String UserStringNameXMLAttributeName;
    static const String UserStringValueXMLAttributeName;

    /*!
    \brief
        Constructor for Window base class

    \param type
        String object holding Window type (usually provided by WindowFactory).

    \param name
        String object holding unique name for the Window.
    */
    Window(const String& type, const String& name);

    /*!
    \brief
        Destructor for Window base class
    */
    virtual ~Window(void);

    /*!
    \brief
        return a String object holding the type name for this Window.

    \return
        String object holding the Window type.
    */
    const String& getType(void) const;

    /*!
    \brief
        returns whether or not this Window is set to be destroyed when its
        parent window is destroyed.

    \return
        - true if the Window will be destroyed when its parent is destroyed.
        - false if the Window will remain when its parent is destroyed.
    */
    bool    isDestroyedByParent(void) const    {return d_destroyedByParent;}

    /*!
    \brief
        returns whether or not this Window is an always on top Window.  Also
        known as a top-most window.

    \return
        - true if this Window is always drawn on top of other normal windows.
        - false if the Window has normal z-order behaviour.
    */
    bool isAlwaysOnTop(void) const  {return d_alwaysOnTop;}

    /*!
    \brief
        return whether the Window is currently disabled

    \note
        Only checks the state set for this window, and does not
        factor in inherited state from ancestor windows.

    \return
        - true if the window is disabled.
        - false if the window is enabled.
    */
    bool isDisabled() const;

    /*!
    \brief
        return whether the Window is currently disabled

    \note
        Not only checks the state set for this window, but also
        factors in inherited state from ancestor windows.

    \return
        - true if the window is disabled.
        - false if the window is enabled.
    */
    bool isEffectiveDisabled() const;

    /*!
    \brief
        return true if the Window is currently visible.

        When true is returned from this function does not mean that the window
        is not completely obscured by other windows, just that the window will
        be processed when rendering, and is not explicitly marked as hidden.

    \note
        Only checks the state set for this window, and does not
        factor in inherited state from ancestor windows.

    \return
        - true if the window is set as visible.
        - false if the window is set as hidden.
    */
    bool isVisible() const;

    /*!
    \brief
        return true if the Window is currently visible.

        When true is returned from this function does not mean that the window
        is not completely obscured by other windows, just that the window will
        be processed when rendering, and is not explicitly marked as hidden.

    \note
        Does check the state set for this window, but also
        factors in inherited state from ancestor windows.

    \return
        - true if the window will be drawn.
        - false if the window is hidden and therefore ignored when rendering.
    */
    bool isEffectiveVisible() const;

    /*!
    \brief
        return true if this is the active Window.  An active window is a window
        that may receive user inputs.

        Mouse events are always sent to the window containing the mouse cursor
        regardless of what this function reports (unless a window has captured
        inputs).  The active state mainly determines where send other, for
        example keyboard, inputs.

    \return
        - true if the window is active and may be sent inputs by the system.
        - false if the window is inactive and will not be sent inputs.
    */
    bool isActive(void) const;

    /*!
    \brief
        return true if this Window is clipped so that its rendering will not
        pass outside of its parent Window area.

    \return
        - true if the window will be clipped by its parent Window.
        - false if the windows rendering may pass outside its parents area
    */
    bool isClippedByParent(void) const {return d_clippedByParent;}

    /*!
    \brief
        return the ID code currently assigned to this Window by client code.

    \return
        uint value equal to the currently assigned ID code for this Window.
    */
    uint getID(void) const {return d_ID;}

    using NamedElement::isChild;
    /*!
    \brief
        returns whether at least one window with the given ID code is attached
        to this Window as a child.

    \note
        ID codes are client assigned and may or may not be unique, and as such,
        the return from this function will only have meaning to the client code.

    \param ID
        uint ID code to look for.

    \return
        - true if at least one child window was found with the ID code \a ID
        - false if no child window was found with the ID code \a ID.
    */
    bool isChild(uint ID) const;

    /*!
    \brief
        returns whether at least one window with the given ID code is attached
        to this Window or any of it's children as a child.

    \note
        ID codes are client assigned and may or may not be unique, and as such,
        the return from this function will only have meaning to the client code.

        WARNING! This function can be very expensive and should only be used
        when you have no other option available. If you decide to use it anyway,
        make sure the window hierarchy from the entry point is small.

    \param ID
        uint ID code to look for.

    \return
        - true if at least one child window was found with the ID code \a ID
        - false if no child window was found with the ID code \a ID.
    */
    bool isChildRecursive(uint ID) const;


    /*!
    \brief
        returns a pointer to the child window at the specified index. Idx is the
        index of the window in the child window list. It is based on the order
        in which the children were added and is stable.

    \param idx
        Index of the child window list position of the window that should be
        returned.This value is not bounds checked, client code should ensure that
        this is less than the value returned by getChildCount().

    \return
        Pointer to the child window currently attached at index position \a idx
    */
    inline Window* getChildAtIdx(size_t idx) const
    {
        return static_cast<Window*>(getChildElementAtIdx(idx));
    }
    
    /*!
    \brief
        return the attached child window that the given name path references.

        A name path is a string that describes a path down the window
        hierarchy using window names and the forward slash '/' as a separator.
        \par
        For example, if this window has a child attached to it named "Panel"
        which has its own children attached named "Okay" and "Cancel",
        you can access the window "Okay" from this window by using the
        name path "Panel/Okay".  To access "Panel", you would simply pass the
        name "Panel".

    \param name_path
        String object holding the name path of the child window to return.

    \return
        the Window object referenced by \a name_path.

    \exception UnknownObjectException
        thrown if \a name_path does not reference a Window attached to this
        Window.
    */
    inline Window* getChild(const String& name_path) const
    {
        return static_cast<Window*>(getChildElement(name_path));
    }

    /*!
    \brief
        return a pointer to the first attached child window with the specified
        name. Children are traversed recursively.

        Contrary to the non recursive version of this function, this one will
        not throw an exception, but return 0 in case no child was found.

    \note
        WARNING! This function can be very expensive and should only be used
        when you have no other option available. If you decide to use it anyway,
        make sure the window hierarchy from the entry point is small.

    \param name
        String object holding the name of the window to return a pointer to.

    \return
        Pointer to the (first) Window object attached to this window that has
        the name \a name.
        If no child is found with the name \a name, 0 is returned.
    */
    inline Window* getChildRecursive(const String& name) const
    {
        return static_cast<Window*>(getChildElementRecursive(name));
    }

    /*!
    \brief
        return a pointer to the first attached child window with the specified
        ID value.

        This function will throw an exception if no child object with the given
        ID is attached.  This decision was made (over returning NULL if no
        window was found) so that client code can assume that if the call
        returns it has a valid window pointer.  We provide the isChild()
        functions for checking if a given window is attached.

    \param ID
        uint value specifying the ID code of the window to return a pointer to.

    \return
        Pointer to the (first) Window object attached to this window that has
        the ID code \a ID.

    \exception UnknownObjectException
        thrown if no window with the ID code \a ID is attached to this Window.
    */
    Window* getChild(uint ID) const;

    /*!
    \brief
        return a pointer to the first attached child window with the specified
        ID value. Children are traversed recursively.

        Contrary to the non recursive version of this function, this one will
        not throw an exception, but return 0 in case no child was found.

    \note
        WARNING! This function can be very expensive and should only be used
        when you have no other option available. If you decide to use it anyway,
        make sure the window hierarchy from the entry point is small.

    \param ID
        uint value specifying the ID code of the window to return a pointer to.

    \return
        Pointer to the (first) Window object attached to this window that has
        the ID code \a ID.
        If no child is found with the ID code \a ID, 0 is returned.
    */
    Window* getChildRecursive(uint ID) const;

    /*!
    \brief
        return a pointer to the Window that currently has input focus starting
        with this Window.

    \return
        Pointer to the window that is active (has input focus) starting at this
        window.  The function will return 'this' if this Window is active
        and either no children are attached or if none of the attached children
        are active.  Returns NULL if this Window (and therefore all children)
        are not active.
    */
    Window* getActiveChild(void);
    const Window* getActiveChild(void) const;

    using NamedElement::isAncestor;
    /*!
    \brief
        return true if any Window with the given ID is some ancestor of this
        Window.

    \param ID
        uint value specifying the ID to look for.

    \return
        - true if an ancestor (parent, or parent of parent, etc) was found with
          the ID code \a ID.
        - false if no ancestor window has the ID code \a ID.
    */
    bool isAncestor(uint ID) const;

    /*!
    \brief
        return the active Font object for the Window.

    \param useDefault
        Specifies whether to return the default font if this Window has no
        preferred font set. This is typically set to true but whenever we
        want to know if a default font would be used, this will be set to
        false, and if the returned Font is a zero pointer we know that this
        means a default font would be used otherwise.

    \return
        Pointer to the Font being used by this Window.  If the window has no
        assigned font, and \a useDefault is true, then the default system font
        is returned.
    */
    /*!  \deprecated This function is deprecated, as the parameter will be removed in the next major version. Separate functions
         will be added with proper function names to replicate the functionality for useDefault=false.
    */
    const Font* getFont(bool useDefault = true) const;

    /*!
    \brief
        return the current text for the Window

    \return
        The String object that holds the current text for this Window.
    */
    const String& getText(void) const {return d_textLogical;}

    //! return text string with \e visual ordering of glyphs.
    const String& getTextVisual() const;

    /*!
    \brief
        return true if the Window inherits alpha from its parent(s).

    \return
        - true if the Window inherits alpha from its parent(s)
        - false if the alpha for this Window is independant from its parents.
    */
    bool inheritsAlpha(void) const  {return d_inheritsAlpha;}

    /*!
    \brief
        return the current alpha value set for this Window

    \note
        The alpha value set for any given window may or may not be the final
        alpha value that is used when rendering.  All window objects, by
        default, inherit alpha from thier parent window(s) - this will blend
        child windows, relatively, down the line of inheritance.  This behaviour
        can be overridden via the setInheritsAlpha() method.  To return the true
        alpha value that will be applied when rendering, use the
        getEffectiveAlpha() method.

    \return
        the currently set alpha value for this Window.  The value returned Will
        be between 0.0f and 1.0f.
    */
    float getAlpha(void) const  {return d_alpha;}

    /*!
    \brief
        return the effective alpha value that will be used when rendering this
        window, taking into account inheritance of parent window(s) alpha.

    \return
        the effective alpha that will be applied to this Window when rendering.
        The value returned Will be between 0.0f and 1.0f.
    */
    float getEffectiveAlpha(void) const;

    /*!
    \brief
        Return a Rect that describes the rendering clipping rect based upon the
        outer rect area of the window.

    \note
        The area returned by this function gives you the correct clipping rect
        for rendering within the Window's outer rect area.  The area described
        may or may not correspond to the final visual clipping actually seen on
        the display; this is intentional and neccessary due to the way that
        imagery is cached under some configurations.
    */
    const Rectf& getOuterRectClipper() const;

    /*!
    \brief
        Return a Rect that describes the rendering clipping rect based upon the
        inner rect area of the window.

    \note
        The area returned by this function gives you the correct clipping rect
        for rendering within the Window's inner rect area.  The area described
        may or may not correspond to the final visual clipping actually seen on
        the display; this is intentional and neccessary due to the way that
        imagery is cached under some configurations.
    */
    const Rectf& getInnerRectClipper() const;

    /*!
    \brief
        Return a Rect that describes the rendering clipping rect for the Window.

        This function can return the clipping rect for either the inner or outer
        area dependant upon the boolean values passed in.

    \note
        The areas returned by this function gives you the correct clipping rects
        for rendering within the Window's areas.  The area described may or may
        not correspond to the final visual clipping actually seen on the
        display; this is intentional and neccessary due to the way that imagery
        is cached under some configurations.

    \param non_client
        - true to return the non-client clipping area (based on outer rect).
        - false to return the client clipping area (based on inner rect).
    */
    const Rectf& getClipRect(const bool non_client = false) const;

    /*!
    \brief
        Return the Rect that descibes the clipped screen area that is used for
        determining whether this window has been hit by a certain point.

        The area returned by this function may also be useful for certain
        calculations that require the clipped Window area as seen on the display
        as opposed to what is used for rendering (since the actual rendering
        clipper rects should not to be used if reliable results are desired).
    */
    const Rectf& getHitTestRect() const;

    /*!
    \brief
        return the Window that currently has inputs captured.

    \return
        Pointer to the Window object that currently has inputs captured, or NULL
        if no Window has captured input.
    */
    Window* getCaptureWindow() const
        {return getGUIContext().getInputCaptureWindow();}

    /*!
    \brief
        return true if this Window has input captured.

    \return
        - true if this Window has captured inputs.
        - false if some other Window, or no Window, has captured inputs.
    */
    bool isCapturedByThis(void) const   {return getCaptureWindow() == this;}

    /*!
    \brief
        return true if an ancestor window has captured inputs.

    \return
        - true if input is captured by a Window that is some ancestor (parent,
          parent of parent, etc) of this Window.
        - false if no ancestor of this window has captured input.
    */
    bool isCapturedByAncestor(void) const
    {return isAncestor(getCaptureWindow());}

    /*!
    \brief
        return true if a child window has captured inputs.

    \return
        - true if input is captured by a Window that is a child of this Window.
        - false if no child of this window has not captured input.
    */
    bool isCapturedByChild(void) const  {return isChild(getCaptureWindow());}

    /*!
    \brief
        check if the given pixel position would hit this window.

    \param position
        Vector2 object describing the position to check.  The position
        describes a pixel offset from the top-left corner of the display.

    \param allow_disabled
        - true specifies that the window may be 'hit' if it is disabled.
        - false specifies that the window may only be hit if it is enabled.

    \return
        - true if \a position hits this Window.
        - false if \a position does not hit this window.
    */
    virtual bool isHit(const Vector2f& position,
                       const bool allow_disabled = false) const;

    /*!
    \brief
        return the child Window that is hit by the given pixel position

    \param position
        Vector2 object describing the position to check.  The position
        describes a pixel offset from the top-left corner of the display.

    \return
        Pointer to the child Window that was hit according to the location
        \a position, or 0 if no child of this window was hit.
    */
    Window* getChildAtPosition(const Vector2f& position) const;

    /*!
    \brief
        return the child Window that is 'hit' by the given position, and is
        allowed to handle mouse events.

    \param position
        Vector2 object describing the position to check.  The position
        describes a pixel offset from the top-left corner of the display.

    \param allow_disabled
        - true specifies that a disabled window may be returned as the target.
        - false specifies that only enabled windows may be returned.

    \return
        Pointer to the child Window that was hit according to the location
        \a position, or 0 if no child of this window was hit.
    */
    Window* getTargetChildAtPosition(const Vector2f& position, 
                                     const bool allow_disabled = false) const;

    /*!
    \brief
        return the parent of this Window.

    \return
        Pointer to the Window object that is the parent of this Window.
        This value can be NULL, in which case the Window is a GUI sheet / root.
    */
    inline Window* getParent() const
    {
        return static_cast<Window*>(getParentElement());
    }

    /*!
    \brief
        Return a pointer to the mouse cursor image to use when the mouse cursor
        is within this window's area.

    \param useDefault
        Sepcifies whether to return the default mouse cursor image if this
        window specifies no preferred mouse cursor image.

    \return
        Pointer to the mouse cursor image that will be used when the mouse
        enters this window's area.  May return NULL indicating no cursor will
        be drawn for this window.
    */
    const Image* getMouseCursor(bool useDefault = true) const;

    /*!
    \brief
        Return the user data set for this Window.

        Each Window can have some client assigned data attached to it, this data
        is not used by the GUI system in any way.  Interpretation of the data is
        entirely application specific.

    \return
        pointer to the user data that is currently set for this window.
    */
    void* getUserData(void) const   {return d_userData;}

    /*!
    \brief
        Return whether this window is set to restore old input capture when it
        loses input capture.

        This is only really useful for certain sub-components for widget
        writers.

    \return
        - true if the window will restore the previous capture window when it
          loses input capture.
        - false if the window will set the capture window to NULL when it loses
          input capture (this is the default behaviour).
    */
    bool restoresOldCapture(void) const     {return d_restoreOldCapture;}

    /*!
    \brief
        Return whether z-order changes are enabled or disabled for this Window.

    \note
        This is distinguished from the is/setRiseOnClickEnabled setting in that
        if rise on click is disabled it only affects the users ability to affect
        the z order of the Window by clicking the mouse; is still possible to
        programatically alter the Window z-order by calling the moveToFront,
        moveToBack, moveInFront and moveBehind member functions.  Whereas if z
        ordering is disabled those functions are also precluded from affecting
        the Window z position.

    \return
        - true if z-order changes are enabled for this window.
          moveToFront, moveToBack, moveInFront and moveBehind work normally.
        - false: z-order changes are disabled for this window.
          moveToFront, moveToBack, moveInFront and moveBehind are ignored.
    */
    bool isZOrderingEnabled(void) const;

    /*!
    \brief
        Return whether this window will receive multi-click events or multiple
        'down' events instead.

    \return
        - true if the Window will receive double-click and triple-click events.
        - false if the Window will receive multiple mouse button down events
          instead of double/triple click events.
    */
    bool wantsMultiClickEvents(void) const;

    /*!
    \brief
        Return whether mouse button down event autorepeat is enabled for this
        window.

    \return
        - true if autorepeat of mouse button down events is enabled for this
          window.
        - false if autorepeat of mouse button down events is not enabled for
          this window.
    */
    bool isMouseAutoRepeatEnabled(void) const;

    /*!
    \brief
        Return the current auto-repeat delay setting for this window.

    \return
        float value indicating the delay, in seconds, defore the first repeat
        mouse button down event will be triggered when autorepeat is enabled.
    */
    float getAutoRepeatDelay(void) const;

    /*!
    \brief
        Return the current auto-repeat rate setting for this window.

    \return
        float value indicating the rate, in seconds, at which repeat mouse
        button down events will be generated after the initial delay has
        expired.
    */
    float getAutoRepeatRate(void) const;

    /*!
    \brief
        Return whether the window wants inputs passed to its attached
        child windows when the window has inputs captured.

    \return
        - true if System should pass captured input events to child windows.
        - false if System should pass captured input events to this window only.
    */
    bool distributesCapturedInputs(void) const;

    /*!
    \brief
        Return whether this Window is using the system default Tooltip for its
        Tooltip window.

    \return
        - true if the Window will use the system default tooltip.
        - false if the window has a custom Tooltip object.
    */
    bool isUsingDefaultTooltip(void) const;

    /*!
    \brief
        Return a pointer to the Tooltip object used by this Window.  The value
        returned may point to the system default Tooltip, a custom Window
        specific Tooltip, or be NULL.

    \return
        Pointer to a Tooltip based object, or NULL.
    */
    Tooltip* getTooltip(void) const;

    /*!
    \brief
        Return the custom tooltip type.

    \return
        String object holding the current custom tooltip window type, or an
        empty string if no custom tooltip is set.
     */
    String getTooltipType(void) const;

    /*!
    \brief
        Return the current tooltip text set for this Window.

    \return
        String object holding the current tooltip text set for this window.
     */
    const String& getTooltipText(void) const;

    /*!
    \brief
        Return the current tooltip text set for this Window or the inherited one.
        If the tooltip text of this window is empty then the inherited value will
        be used instead.

    \return
        String object holding the current tooltip text of this window or the 
        tooltip text this window inherited.
     */
    const String& getTooltipTextIncludingInheritance(void) const;

    /*!
    \brief
        Return whether this window inherits Tooltip text from its parent when
        its own tooltip text is not set.

    \return
        - true if the window inherits tooltip text from its parent when its own
          text is not set.
        - false if the window does not inherit tooltip text from its parent
          (and shows no tooltip when no text is set).
     */
    bool inheritsTooltipText(void) const;

    /*!
    \brief
        Return whether this window will rise to the top of the z-order when
        clicked with the left mouse button.

    \note
        This is distinguished from the is/setZOrderingEnabled setting in that
        if rise on click is disabled it only affects the users ability to affect
        the z order of the Window by clicking the mouse; is still possible to
        programatically alter the Window z-order by calling the moveToFront,
        moveToBack, moveInFront and moveBehind member functions.  Whereas if z
        ordering is disabled those functions are also precluded from affecting
        the Window z position.

    \return
        - true if the window will come to the top of other windows when the left
          mouse button is pushed within its area.
        - false if the window does not change z-order position when the left
          mouse button is pushed within its area.
     */
    bool isRiseOnClickEnabled(void) const   { return d_riseOnClick; }

    /*!
    \brief
        Return the GeometryBuffer object for this Window.

    \return
        Reference to the GeometryBuffer object for this Window.
    */
    GeometryBuffer& getGeometryBuffer();

    /*!
    \brief
        Get the name of the LookNFeel assigned to this window.

    \return
        String object holding the name of the look assigned to this window.
        Returns the empty string if no look is assigned.
    */
    const String& getLookNFeel() const;

    /*!
    \brief
        Get whether or not this Window is the modal target.

    \return
        Returns true if this Window is the modal target, otherwise false.
    */
    bool getModalState(void) const
    {return(getGUIContext().getModalWindow() == this);}

    /*!
    \brief
        Returns a named user string.

    \param name
        String object holding the name of the string to be returned.

    \return
        String object holding the data stored for the requested user string.

    \exception UnknownObjectException
        thrown if a user string named \a name does not exist.
    */
    const String& getUserString(const String& name) const;

    /*!
    \brief
        Return whether a user string with the specified name exists.

    \param name
        String object holding the name of the string to be checked.

    \return
        - true if a user string named \a name exists.
        - false if no such user string exists.
    */
    bool isUserStringDefined(const String& name) const;

    /*!
    \brief
        Returns the active sibling window.

        This searches the immediate children of this window's parent, and
        returns a pointer to the active window.  The method will return this if
        we are the immediate child of our parent that is active.  If our parent
        is not active, or if no immediate child of our parent is active then 0
        is returned.  If this window has no parent, and this window is not
        active then 0 is returned, else this is returned.

    \return
        A pointer to the immediate child window attached to our parent that is
        currently active, or 0 if no immediate child of our parent is active.
    */
    Window* getActiveSibling();

    /*!
    \brief
        Returns whether this window should ignore mouse event and pass them
        through to and other windows behind it. In effect making the window
        transparent to the mouse.

    \return
        true if mouse pass through is enabled.
        false if mouse pass through is not enabled.
    */
    bool isMousePassThroughEnabled(void) const  {return d_mousePassThroughEnabled;}

    /*!
    \brief
        Returns whether this window is an auto window.

        An auto window is typically a Window object created automatically by
        CEGUI - for example to form part of a multi-element 'compound' widget.
    */
    bool isAutoWindow(void) const   {return d_autoWindow;}

    /*!
    \brief
        Returns whether this window is allowed to write XML.
    */
    bool isWritingXMLAllowed(void) const    {return d_allowWriteXML;}

    /*!
    \brief
        Returns whether this Window object will receive events generated by
        the drag and drop support in the system.

    \return
        - true if the Window is enabled as a drag and drop target.
        - false if the window is not enabled as a drag and drop target.
    */
    bool isDragDropTarget() const;

    /*!
    \brief
        Fill in the RenderingContext \a ctx with details of the RenderingSurface
        where this Window object should normally do it's rendering.
    */
    void getRenderingContext(RenderingContext& ctx) const;

    //! implementation of the default getRenderingContext logic.
    virtual void getRenderingContext_impl(RenderingContext& ctx) const;

    /*!
    \brief
        return the RenderingSurface currently set for this window.  May return
        0.
    */
    RenderingSurface* getRenderingSurface() const;

    /*!
    \brief
        return the RenderingSurface that will be used by this window as the
        target for rendering.
    */
    RenderingSurface& getTargetRenderingSurface() const;

    /*!
    \brief
        Returns whether \e automatic use of an imagery caching RenderingSurface
        (i.e. a RenderingWindow) is enabled for this window.  The reason we
        emphasise 'automatic' is because the client may manually set a
        RenderingSurface that does exactly the same job.

    \return
        - true if automatic use of a caching RenderingSurface is enabled.
        - false if automatic use of a caching RenderTarget is not enabled.
    */
    bool isUsingAutoRenderingSurface() const;

    /*!
    \brief
        Returns the window at the root of the hierarchy starting at this
        Window.  The root window is defined as the first window back up the
        hierarchy that has no parent window.

    \return
        A pointer to the root window of the hierarchy that this window is
        attached to.
    */
    const Window* getRootWindow() const;
    Window* getRootWindow();

    /*!
    \brief
        Initialises the Window based object ready for use.

    \note
        This must be called for every window created.  Normally this is handled
        automatically by the WindowManager.

    \return
        Nothing
    */
    virtual void initialiseComponents(void) {}

    /*!
    \brief
        Set whether or not this Window will automatically be destroyed when its
        parent Window is destroyed.

    \param setting
        - true to have the Window auto-destroyed when its parent is destroyed
          (default behaviour)
        - false to have the Window remain after its parent is destroyed.

    \return
        Nothing
    */
    void setDestroyedByParent(bool setting);

    /*!
    \brief
        Set whether this window is always on top, or not.

    \param setting
        - true to have the Window appear on top of all other non always on top
          windows
        - false to allow the window to be covered by other normal windows.

    \return
        Nothing
    */
    void setAlwaysOnTop(bool setting);

    /*!
    \brief
        Set whether this window is enabled or disabled.  A disabled window
        normally can not be interacted with, and may have different rendering.

    \param setting
        - true to enable the Window
        - false to disable the Window.

    \return
        Nothing
    */
    void setEnabled(bool setting);

    /*!
    \brief
        Set whether this window is enabled or disabled.  A disabled window
        normally can not be interacted with, and may have different rendering.

    \param setting
        - true to disable the Window
        - false to enable the Window.

    \return
        Nothing
    */
    void setDisabled(bool setting);

    /*!
    \brief
        enable the Window to allow interaction.

    \return
        Nothing
    */
    void enable(void)   {setEnabled(true);}

    /*!
    \brief
        disable the Window to prevent interaction.

    \return
        Nothing
    */
    void disable(void)  {setEnabled(false);}

    /*!
    \brief
        Set whether the Window is visible or hidden.

    \param setting
        - true to make the Window visible.
        - false to make the Window hidden.

    \note
        Hiding the active window will cause that window to become deactivated.
        Showing a window does not, however, automatically cause that window to
        become the active window (call Window::activate after making the window
        visible to activate it).

    \return
        Nothing
    */
    void setVisible(bool setting);

    /*!
    \brief
        show the Window.

    \note
        Showing a window does not automatically activate the window.  If you
        want the window to also become active you will need to call the
        Window::activate member also.

    \return
        Nothing
    */
    void show(void)     {setVisible(true);}

    /*!
    \brief
        hide the Window.
    \note
        If the window is the active window, it will become deactivated as a
        result of being hidden.

    \return
        Nothing
    */
    void hide(void)     {setVisible(false);}

    /*!
    \brief
        Activate the Window giving it input focus and bringing it to the top of
        all windows with the same always-on-top settig as this Window.

    \return
        Nothing
    */
    void activate(void);

    /*!
    \brief
        Deactivate the window.  No further inputs will be received by the window
        until it is re-activated either programmatically or by the user
        interacting with the gui.

    \return
        Nothing.
    */
    void deactivate(void);

    /*!
    \brief
        Set whether this Window will be clipped by its parent window(s).

    \param setting
        - true to have the Window clipped so that rendering is constrained to
          within the area of its parent(s).
        - false to have rendering constrained to the screen only.

    \return
        Nothing
    */
    void setClippedByParent(bool setting);

    /*!
    \brief
        Set the current ID for the Window.

    \param ID
        Client assigned ID code for this Window.  The GUI system assigns no
        meaning to any IDs, they are a device purely for client code usage.

    \return
        Nothing
    */
    void setID(uint ID);

    /*!
    \brief
        Set the current text string for the Window.

    \param text
        String object containing the text that is to be set as the Window text.

    \return
        Nothing
    */
    void setText(const String& text);

    /*!
    \brief
        Insert the text string \a text into the current text string for the
        Window object at the position specified by \a position.
     
    \param text
        String object holding the text that is to be inserted into the Window
        object's current text string.
     
    \param position
        The characted index position where the string \a text should be
        inserted.
    */
    void insertText(const String& text, const String::size_type position);

    /*!
    \brief
        Append the string \a text to the currect text string for the Window
        object.
     
    \param text
        String object holding the text that is to be appended to the Window
        object's current text string.
    */
    void appendText(const String& text);

    /*!
    \brief
        Set the font used by this Window.

    \param font
        Pointer to the Font object to be used by this Window.
        If \a font is NULL, the default font will be used.

    \return
        Nothing
    */
    void setFont(const Font* font);

    /*!
    \brief
        Set the font used by this Window.

    \param name
        String object holding the name of the Font object to be used by this
        Window.  If \a name == "", the default font will be used.

    \return
        Nothing

    \exception UnknownObjectException
        thrown if the specified Font is unknown within the system.
    */
    void setFont(const String& name);

    using NamedElement::removeChild;
    /*!
    \brief
        Remove the first child Window with the specified ID.  If there is more
        than one attached Window objects with the specified ID, only the fist
        one encountered will be removed.

    \param ID
        ID number assigned to the Window to be removed.  If no Window with ID
        code \a ID is attached, nothing happens.

    \return
        Nothing.
    */
    void removeChild(uint ID);

    /*!
    \brief
        Creates a child window attached to this window.
    
    \param type
        String that describes the type of Window to be created.  A valid
        WindowFactory for the specified type must be registered.

    \param name
        String that holds the name that is to be given to the new window.  If
        this string is empty, a name will be generated for the window.

    \return
        Pointer to the newly created child Window object.
    */
    Window* createChild(const String& type, const String& name = "");

    /*!
    \brief
        Destroys a child window of this window

    \param wnd
        The child window to destroy
    */
    void destroyChild(Window* wnd);

    /*!
    \brief
        Destroys a child window of this window

    \param name_path
        Name path that references the window to destroy
    */
    void destroyChild(const String& name_path);
    
    /*!
    \brief
        Move the Window to the top of the z order.

        - If the Window is a non always-on-top window it is moved the the top of
          all other non always-on-top sibling windows, and the process repeated
          for all ancestors.
        - If the Window is an always-on-top window it is moved to the of of all
          sibling Windows, and the process repeated for all ancestors.

    \return
        Nothing
    */
    void moveToFront();

    /*!
    \brief
        Move the Window to the bottom of the Z order.

        - If the window is non always-on-top the Window is sent to the very
          bottom of its sibling windows and the process repeated for all
          ancestors.
        - If the window is always-on-top, the Window is sent to the bottom of
          all sibling always-on-top windows and the process repeated for all
          ancestors.

    \return
        Nothing
    */
    void moveToBack();

    /*!
    \brief
        Move this window immediately above it's sibling \a window in the z order.

        No action will be taken under the following conditions:
        - \a window is 0.
        - \a window is not a sibling of this window.
        - \a window and this window have different AlwaysOnTop settings.
        - z ordering is disabled for this window.

    \param window
        The sibling window that this window will be moved in front of.
    */
    void moveInFront(const Window* const window);

    /*!
    \brief
        Move this window immediately behind it's sibling \a window in the z
        order.

        No action will be taken under the following conditions:
        - \a window is 0.
        - \a window is not a sibling of this window.
        - \a window and this window have different AlwaysOnTop settings.
        - z ordering is disabled for this window.

    \param window
        The sibling window that this window will be moved behind.
    */
    void moveBehind(const Window* const window);
    
    /*!
    \brief
        Return the (visual) z index of the window on it's parent.
        
        The z index is a number that indicates the order that windows will be
        drawn (but is not a 'z co-ordinate', as such).  Higher numbers are in
        front of lower numbers.

        The number returned will not be stable, and generally should be used to
        compare with the z index of sibling windows (and only sibling windows)
        to discover the current z ordering of those windows.
    */
    size_t getZIndex() const;

    /*!
    \brief
        Return whether /a this Window is in front of the given window.

    \note
        Here 'in front' just means that one window is drawn after the other, it
        is not meant to imply that the windows are overlapping nor that one
        window is obscured by the other.
    */
    bool isInFront(const Window& wnd) const;

    /*!
    \brief
        Return whether /a this Window is behind the given window.

    \note
        Here 'behind' just means that one window is drawn before the other, it
        is not meant to imply that the windows are overlapping nor that one
        window is obscured by the other.
    */
    bool isBehind(const Window& wnd) const;

    /*!
    \brief
        Captures input to this window

    \return
        - true if input was successfully captured to this window.
        - false if input could not be captured to this window
          (maybe because the window is not active).
    */
    bool captureInput(void);

    /*!
    \brief
        Releases input capture from this Window.  If this Window does not have
        inputs captured, nothing happens.

    \return
        Nothing
    */
    void releaseInput(void);

    /*!
    \brief
        Set whether this window will remember and restore the previous window
        that had inputs captured.

    \param setting
        - true: The window will remember and restore the previous capture
          window.  The CaptureLost event is not fired on the previous window
          when this window steals input capture.  When this window releases
          capture, the old capture window is silently restored.

        - false: Input capture works as normal, each window losing capture is
          signalled via CaptureLost, and upon the final release of capture, no
          previous setting is restored (this is the default behaviour).

    \return
        Nothing
    */
    void setRestoreOldCapture(bool setting);

    /*!
    \brief
        Set the current alpha value for this window.

    \note
        The alpha value set for any given window may or may not be the final
        alpha value that is used when rendering.  All window objects, by
        default, inherit alpha from thier parent window(s) - this will blend
        child windows, relatively, down the line of inheritance.  This behaviour
        can be overridden via the setInheritsAlpha() method.  To return the true
        alpha value that will be applied when rendering, use the
        getEffectiveAlpha() method.

    \param alpha
        The new alpha value for the window.
        Value should be between 0.0f and 1.0f.

    \return
        Nothing
    */
    void setAlpha(const float alpha);

    /*!
    \brief
        Sets whether this Window will inherit alpha from its parent windows.

    \param setting
        - true if the Window should use inherited alpha.
        - false if the Window should have an independant alpha value.

    \return
        Nothing
    */
    void setInheritsAlpha(bool setting);

    /*!
    \brief
        Invalidate this window causing at least this window to be redrawn during
        the next rendering pass.

    \return
        Nothing

    \deprecated
        This function is deprecated in favour of the version taking a boolean.
    */
    void invalidate(void);

    /*!
    \brief
        Invalidate this window and - dependant upon \a recursive - all child
        content, causing affected windows to be redrawn during the next
        rendering pass.

    \param recursive
        Boolean value indicating whether attached child content should also be
        invalidated.
        - true will cause all child content to be invalidated also.
        - false will just invalidate this single window.

    \return
        Nothing
    */
    void invalidate(const bool recursive);

    /*!
    \brief
        Set the mouse cursor image to be used when the mouse enters this window.

    \param image
        Pointer to the Image object to use as the mouse cursor image when the
        mouse enters the area for this Window.

    \return
        Nothing.
    */
    void setMouseCursor(const Image* image);

    /*!
    \brief
        Set the mouse cursor image to be used when the mouse enters this window.

    \param imageset
        String object that contains the name of the Imageset that contains the
        image to be used.

    \param name
        String object that contains the name of the Image to use.

    \return
        Nothing.

    \exception UnknownObjectException
        thrown if no Image named \a name exists.
    */
    void setMouseCursor(const String& name);

    /*!
    \brief
        Set the user data set for this Window.

        Each Window can have some client assigned data attached to it, this data
        is not used by the GUI system in any way.  Interpretation of the data is
        entirely application specific.

    \param user_data
        pointer to the user data that is to be set for this window.

    \return
        Nothing.
    */
    void setUserData(void* user_data)   {d_userData = user_data;}
    
    /*!
    \brief
        Set whether z-order changes are enabled or disabled for this Window.

    \note
        This is distinguished from the is/setRiseOnClickEnabled setting in that
        if rise on click is disabled it only affects the users ability to affect
        the z order of the Window by clicking the mouse; is still possible to
        programatically alter the Window z-order by calling the moveToFront,
        moveToBack, moveInFront and moveBehind member functions.  Whereas if z
        ordering is disabled those functions are also precluded from affecting
        the Window z position.

    \param setting
        - true if z-order changes are enabled for this window.
          moveToFront, moveToBack, moveInFront and moveBehind work normally.
        - false: z-order changes are disabled for this window.
          moveToFront, moveToBack, moveInFront and moveBehind are ignored.

    \return
        Nothing.
    */
    void    setZOrderingEnabled(bool setting);
    
    /*!
    \brief
        Set whether this window will receive multi-click events or multiple
        'down' events instead.

    \param setting
        - true if the Window will receive double-click and triple-click events.
        - false if the Window will receive multiple mouse button down events
          instead of double/triple click events.

    \return
        Nothing.
    */
    void setWantsMultiClickEvents(bool setting);

    /*!
    \brief
        Set whether mouse button down event autorepeat is enabled for this
        window.

    \param setting
        - true to enable autorepeat of mouse button down events.
        - false to disable autorepeat of mouse button down events.

    \return
        Nothing.
    */
    void setMouseAutoRepeatEnabled(bool setting);

    /*!
    \brief
        Set the current auto-repeat delay setting for this window.

    \param delay
        float value indicating the delay, in seconds, defore the first repeat
        mouse button down event should be triggered when autorepeat is enabled.

    \return
        Nothing.
    */
    void setAutoRepeatDelay(float delay);

    /*!
    \brief
        Set the current auto-repeat rate setting for this window.

    \param rate
        float value indicating the rate, in seconds, at which repeat mouse
        button down events should be generated after the initial delay has
        expired.

    \return
        Nothing.
    */
    void setAutoRepeatRate(float rate);

    /*!
    \brief
        Set whether the window wants inputs passed to its attached
        child windows when the window has inputs captured.

    \param setting
        - true if System should pass captured input events to child windows.
        - false if System should pass captured input events to this window only.
    */
    void setDistributesCapturedInputs(bool setting);

    /*!
    \brief
        Internal support method for drag & drop.  You do not normally call
        this directly from client code.  See the DragContainer class.
    */
    void notifyDragDropItemEnters(DragContainer* item);

    /*!
    \brief
        Internal support method for drag & drop.  You do not normally call
        this directly from client code.  See the DragContainer class.
    */
    void notifyDragDropItemLeaves(DragContainer* item);

    /*!
    \brief
        Internal support method for drag & drop.  You do not normally call
        this directly from client code.  See the DragContainer class.
    */
    void notifyDragDropItemDropped(DragContainer* item);

    /*!
    \brief
        Internal destroy method which actually just adds the window and any
        parent destructed child windows to the dead pool.

        This is virtual to allow for specialised cleanup which may be required
        in some advanced cases.  If you override this for the above reason, you
        MUST call this base class version.

    \note
        You never have to call this method yourself, use WindowManager to
        destroy your Window objects (which will call this for you).
    */
    virtual void destroy(void);

    /*!
    \brief
        Set the custom Tooltip object for this Window.  This value may be 0 to
        indicate that the Window should use the system default Tooltip object.

    \param tooltip
        Pointer to a valid Tooltip based object which should be used as the
        tooltip for this Window, or 0 to indicate that the Window should use the
        system default Tooltip object.  Note that when passing a pointer to a
        Tooltip object, ownership of the Tooltip does not pass to this Window
        object.

    \return
        Nothing.
    */
    void setTooltip(Tooltip* tooltip);

    /*!
    \brief
        Set the custom Tooltip to be used by this Window by specifying a Window
        type.

        The Window will internally attempt to create an instance of the
        specified window type (which must be derived from the base Tooltip
        class).  If the Tooltip creation fails, the error is logged and the
        Window will revert to using either the existing custom Tooltip or the
        system default Tooltip.

    \param tooltipType
        String object holding the name of the Tooltip based Window type which
        should be used as the Tooltip for this Window.

    \return
        Nothing.
    */
    void setTooltipType(const String& tooltipType);

    /*!
    \brief
        Set the tooltip text for this window.

    \param tip
        String object holding the text to be displayed in the tooltip for this
        Window.

    \return
        Nothing.
    */
    void setTooltipText(const String& tip);

    /*!
    \brief
        Set whether this window inherits Tooltip text from its parent when its
        own tooltip text is not set.

    \param setting
        - true if the window should inherit tooltip text from its parent when
          its own text is not set.
        - false if the window should not inherit tooltip text from its parent
          (and so show no tooltip when no text is set).

    \return
        Nothing.
     */
    void setInheritsTooltipText(bool setting);

    /*!
    \brief
        Set whether this window will rise to the top of the z-order when clicked
        with the left mouse button.

    \note
        This is distinguished from the is/setZOrderingEnabled setting in that
        if rise on click is disabled it only affects the users ability to affect
        the z order of the Window by clicking the mouse; is still possible to
        programatically alter the Window z-order by calling the moveToFront,
        moveToBack, moveInFront and moveBehind member functions.  Whereas if z
        ordering is disabled those functions are also precluded from affecting
        the Window z position.

    \param setting
        - true if the window should come to the top of other windows when the
          left mouse button is pushed within its area.
        - false if the window should not change z-order position when the left
          mouse button is pushed within its area.

    \return
        Nothing.
     */
    void setRiseOnClickEnabled(bool setting)    { d_riseOnClick = setting; }

    /*!
    \brief
        Set the LookNFeel that shoule be used for this window.

    \param look
        String object holding the name of the look to be assigned to the window.

    \exception UnknownObjectException
        thrown if the look'n'feel specified by \a look does not exist.

    \exception NullObjectException
        thrown if the Window does not have a WindowRenderer assigned to it.
        see Window::setWindowRenderer.

    \note
        This is really intended as an internal function. The way that client
        code is supposed to use skins is by defining a Falagard mapping (either
        in a scheme xml file or in code) and then create instances of that
        mapped type via WindowManager.  See
        WindowFactoryManager::addFalagardWindowMapping and \ref xml_scheme. 
        With that being said, it is possible for client code to use this
        function so long as you are aware of the implications of doing so:
        - Automatically created child windows (AutoWindows) will be deleted, and
          references or pointers you hold to these will become invalid.
        - Aside from those absolutely required, there is not guarantee that the
          newly assigned look will create the same set of child windows, nor
          that any created windows will be of any given type.
        - Any properties set on automatically created child windows after their
          creation will be lost - even if the new look creates a child of the
          same type with the same name.
    */
    virtual void setLookNFeel(const String& look);

    /*!
    \brief
        Set the modal state for this Window.

    \param state
        Boolean value defining if this Window should be the modal target.
        - true if this Window should be activated and set as the modal target.
        - false if the modal target should be cleared if this Window is
          currently the modal target.

    \return
        Nothing.
    */
    void setModalState(bool state);

    /*!
    \brief
        Layout child window content.

        Laying out of child content includes:
            - ensuring content specified in any assigned WidgetLook has its area
              rectangles sychronised.
            - assigned WindowRenderer given the opportunity to update child
              content areas as needed.
            - All content is then potentially updated via the onParentSized
              notification as required by changes in non-client and client area
              rectangles.

        The system may call this at various times (like when a window is resized
        for example), and it may be invoked directly where required.

    \param nonclient_sized_hint
        Hint that the non-client area rectangle has changed size.

    \param client_sized_hint
        Hint that the client area rectangle has changed size.

    \note
        The hint parameters are essentially a way to force onParentSized
        notifications for a given type (client / nonclient) of child window.
        Setting a hint to false does not mean a notification will not happen,
        instead it means that the function is to do its best to determine
        whether a given notification is required to be sent.
    */
    virtual void performChildWindowLayout(bool nonclient_sized_hint = false,
                                          bool client_sized_hint = false);

    /*!
    \brief
       Sets the value a named user string, creating it as required.

    \param name
        String object holding the name of the string to be returned.

    \param value
        String object holding the value to be assigned to the user string.

    \return
        Nothing.
    */
    void setUserString(const String& name, const String& value);

    /*!
    \brief
        Causes the Window object to render itself and all of it's attached
        children

    \return
        Nothing
    */
    void render();

    /*!
    \brief
        Cause window to update itself and any attached children.  Client code
        does not need to call this method; to ensure full, and proper updates,
        call the injectTimePulse methodname method provided by the System class.

    \note
        The update order is such that 'this' window is updated prior to any
        child windows, this is so that child windows that access the parent in
        their update code get the correct updated state.

    \param elapsed
        float value indicating the number of seconds passed since the last
        update.

    \return
        Nothing.
    */
    virtual void update(float elapsed);
    
    /*!
    \brief
        Asks the widget to perform a clipboard copy to the provided clipboard
        
    \param clipboard
        Target clipboard class
        
    \return
        true if the copy was successful and allowed, false otherwise
    */
    virtual bool performCopy(Clipboard& clipboard);
    
    /*!
    \brief
        Asks the widget to perform a clipboard cut to the provided clipboard
     
    \param clipboard
        Target clipboard class
     
    \return
        true if the cut was successful and allowed, false otherwise
    */
    virtual bool performCut(Clipboard& clipboard);
    
    /*!
    \brief
        Asks the widget to perform a clipboard paste from the provided clipboard
     
    \param clipboard
        Source clipboard class
     
    \return
        true if the paste was successful and allowed, false otherwise
     */
    virtual bool performPaste(Clipboard& clipboard);

    /*!
    \brief
        Writes an xml representation of this window object to \a out_stream.

    \param xml_stream
        Stream where xml data should be output.

    \return
        Nothing.
    */
    virtual void writeXMLToStream(XMLSerializer& xml_stream) const;

    /*!
    \brief
        Sets the internal 'initialising' flag to true.
        This can be use to optimize initialisation of some widgets, and is called
        automatically by the layout XML handler when it has created a window.
        That is just after the window has been created, but before any children or
        properties are read.
    */
    virtual void beginInitialisation(void)     {d_initialising = true;}

    /*!
    \brief
        Sets the internal 'initialising' flag to false.
        This is called automatically by the layout XML handler when it is done
        creating a window. That is after all properties and children have been
        loaded and just before the next sibling gets created.
    */
    virtual void endInitialisation(void)       {d_initialising = false;}

    /*!
    \brief
        Sets whether this window should ignore mouse events and pass them
        through to any windows behind it. In effect making the window
        transparent to the mouse.

    \param setting
        true if mouse pass through is enabled.
        false if mouse pass through is not enabled.
    */
    void setMousePassThroughEnabled(bool setting)   {d_mousePassThroughEnabled = setting;}

    /*!
    \brief
        Assign the WindowRenderer type to be used when rendering this window.

    \param name
        The factory name of the WindowRenderer to use.

    \note
        This is really intended as an internal function. The way that client
        code is supposed to use skins is by defining a Falagard mapping (either
        in a scheme xml file or in code) and then create instances of that
        mapped type via WindowManager.  See
        WindowFactoryManager::addFalagardWindowMapping and \ref xml_scheme. 
    */
    void setWindowRenderer(const String& name);

    /*!
    \brief
        Get the currently assigned WindowRenderer. (Look'N'Feel specification).

    \return
        A pointer to the assigned window renderer object.
        0 if no window renderer is assigned.
    */
    WindowRenderer* getWindowRenderer(void) const;

    /*!
    \brief
        Get the factory name of the currently assigned WindowRenderer.
        (Look'N'Feel specification).

    \return
        The factory name of the currently assigned WindowRenderer.
        If no WindowRenderer is assigned an empty string is returned.
    */
    const String& getWindowRendererName() const;

    /*!
    \brief
        Sets whether this window is allowed to write XML
    */
    void setWritingXMLAllowed(bool allow)   {d_allowWriteXML = allow;}

    /*!
    \brief
        Inform the window, and optionally all children, that screen area
        rectangles have changed.

    \param recursive
        - true to recursively call notifyScreenAreaChanged on attached child
          Window objects.
        - false to just process \e this Window.
    */
    virtual void notifyScreenAreaChanged(bool recursive = true);

    /*!
    \brief
        Changes the widget's falagard type, thus changing its look'n'feel and optionally its
        renderer in the process.

    \param type
        New look'n'feel of the widget

    \param type
        New renderer of the widget
    */
    void setFalagardType(const String& type, const String& rendererType = "");

    /*!
    \brief
        Specifies whether this Window object will receive events generated by
        the drag and drop support in the system.

    \param setting
        - true to enable the Window as a drag and drop target.
        - false to disable the Window as a drag and drop target.
    */
    void setDragDropTarget(bool setting);

    /*!
    \brief
        Set the RenderingSurface to be associated with this Window, or 0 if
        none is required.
    \par
        If this function is called, and the option for automatic use of an
        imagery caching RenderingSurface is enabled, any automatically created
        RenderingSurface will be released, and the affore mentioned option will
        be disabled.
    \par
        If after having set a custom RenderingSurface you then subsequently
        enable the automatic use of an imagery caching RenderingSurface by
        calling setUsingAutoRenderingSurface, the previously set
        RenderingSurface will be disassociated from the Window.  Note that the
        previous RenderingSurface is not destroyed or cleaned up at all - this
        is the job of whoever created that object initially.

    \param target
        Pointer to the RenderingSurface object to be associated with the window.
    */
    void setRenderingSurface(RenderingSurface* surface);

    /*!
    \brief
        Invalidate the chain of rendering surfaces from this window backwards to
        ensure they get properly redrawn - but doing the minimum amount of work
        possibe - next render.
    */
    void invalidateRenderingSurface();

    /*!
    \brief
        Sets whether \e automatic use of an imagery caching RenderingSurface
        (i.e. a RenderingWindow) is enabled for this window.  The reason we
        emphasise 'atutomatic' is because the client may manually set a
        RenderingSurface that does exactlythe same job.
    \par
        Note that this setting really only controls whether the Window
        automatically creates and manages the RenderingSurface, as opposed to
        the \e use of the RenderingSurface.  If a RenderingSurfaceis set for the
        Window it will be used regardless of this setting.
    \par
        Enabling this option will cause the Window to attempt to create a
        suitable RenderingSurface (which will actually be a RenderingWindow).
        If there is an existing RenderingSurface assocated with this Window, it
        will be removed as the Window's RenderingSurface
        <em>but not destroyed</em>; whoever created the RenderingSurface in the
        first place should take care of its destruction.
    \par
        Disabling this option will cause any automatically created
        RenderingSurface to be released.
    \par
        It is possible that the renderer in use may not support facilities for
        RenderingSurfaces that are suitable for full imagery caching.  If this
        is the case, then calling getRenderingSurface after enabling this option
        will return 0.  In these cases this option will still show as being
        'enabled', this is because Window \e settings should not be influenced
        by capabilities the renderer in use; for example, this enables correct
        XML layouts to be written from a Window on a system that does not
        support such RenderingSurfaces, so that the layout will function as
        preferred on systems that do.
    \par
        If this option is enabled, and the client subsequently assigns a
        different RenderingSurface to the Window, the existing automatically
        created RenderingSurface will be released and this setting will be
        disabled.

    \param setting
        - true to enable automatic use of an imagery caching RenderingSurface.
        - false to disable automatic use of an imagery caching RenderingSurface.
    */
    void setUsingAutoRenderingSurface(bool setting);

    //! Return the parsed RenderedString object for this window.
    const RenderedString& getRenderedString() const;
    //! Return a pointer to any custom RenderedStringParser set, or 0 if none.
    RenderedStringParser* getCustomRenderedStringParser() const;
    //! Set a custom RenderedStringParser, or 0 to remove an existing one.
    void setCustomRenderedStringParser(RenderedStringParser* parser);
    //! return the active RenderedStringParser to be used
    virtual RenderedStringParser& getRenderedStringParser() const;
    //! return whether text parsing is enabled for this window.
    bool isTextParsingEnabled() const;
    //! set whether text parsing is enabled for this window.
    void setTextParsingEnabled(const bool setting);

    //! set margin
    virtual void setMargin(const UBox& margin);
    //! retrieves currently set margin
    const UBox& getMargin() const;

    //! return Vector2 \a pos after being fully unprojected for this Window.
    Vector2f getUnprojectedPosition(const Vector2f& pos) const;

    //! return the pointer to the BidiVisualMapping for this window, if any.
    const BidiVisualMapping* getBidiVisualMapping() const
        {return d_bidiVisualMapping;}

    /*!
    \brief
        Adds the named property to the XML ban list for this window
        Essentially a property that is banned from XML will never end up being saved to it.
        This is very useful if 2 properties overlap (XPosition and Position for example).

        Please note that properties that are not writable (read-only properties) are
        implicitly/automatically banned from XML, no need to ban them manually.

    \param property_name 
        Name of the property you want to ban
    */
    void banPropertyFromXML(const String& property_name);

    /*!
    \brief
        Adds the named property to the XML ban list for this window and all of its child windows.
        Essentially a property that is banned from XML will never end up being saved to it.
        This is very useful if 2 properties overlap (XPosition and Position for example).

        Please note that properties that are not writable (read-only properties) are
        implicitly/automatically banned from XML, no need to ban them manually.

    \param property_name 
        Name of the property you want to ban
    */
    void banPropertyFromXMLRecursive(const String& property_name);

    /*!
    \brief
        Removes the named property from the XML ban list for this window.
        Essentially a property that is banned from XML will never end up being saved to it.

        Please note that properties that are not writable (read-only properties) are
        implicitly/automatically banned from XML.

    \param property_name 
        Name of the property you want to unban
    */
    void unbanPropertyFromXML(const String& property_name);

    /*!
    \brief
        Removes the named property from the XML ban list for this window and all of its child windows.
        Essentially a property that is banned from XML will never end up being saved to it.

        Please note that properties that are not writable (read-only properties) are
        implicitly/automatically banned from XML.

    \param property_name 
        Name of the property you want to unban
    */
    void unbanPropertyFromXMLRecursive(const String& property_name);
    

    /*!
    \brief
        Return whether the named property is banned from XML

    \note
        Read-only properties and properties that can't write to XML streams
        are implicitly banned. This method will return true for them.
    */
    bool isPropertyBannedFromXML(const String& property_name) const;

    //! Add the given property to the XML ban list for this window.
    void banPropertyFromXML(const Property* property);

    //! Remove the given property from the XML ban list for this window.
    void unbanPropertyFromXML(const Property* property);

    /*!
    \brief
        Return whether given property is banned from XML

    \note
        Read-only properties and properties that can't write to XML streams
        are implicitly banned. This method will return true for them.
    */
    bool isPropertyBannedFromXML(const Property* property) const;

    /*!
    \brief
        Set the window update mode.  This mode controls the behaviour of the
        Window::update member function such that updates are processed for
        this window (and therefore it's child content) according to the set
        mode.

    \note
        Disabling updates can have negative effects on the behaviour of CEGUI
        windows and widgets; updates should be disabled selectively and
        cautiously - if you are unsure of what you are doing, leave the mode
        set to WUM_ALWAYS.
    
    \param mode
        One of the WindowUpdateMode enumerated values indicating the mode to
        set for this Window.
    */
    void setUpdateMode(const WindowUpdateMode mode);

    /*!
    \brief
        Return the current window update mode that is set for this Window.
        This mode controls the behaviour of the Window::update member function
        such that updates are processed for this window (and therefore it's
        child content) according to the set mode.

    \note
        Disabling updates can have negative effects on the behaviour of CEGUI
        windows and widgets; updates should be disabled selectively and
        cautiously - if you are unsure of what you are doing, leave the mode
        set to WUM_ALWAYS.
    
    \return
        One of the WindowUpdateMode enumerated values indicating the current
        mode set for this Window.
    */
    WindowUpdateMode getUpdateMode() const;

    /*!
    \brief
        Set whether mouse input that is not directly handled by this Window
        (including it's event subscribers) should be propagated back to the
        Window's parent.

    \param enabled
        - true if unhandled mouse input should be propagated to the parent.
        - false if unhandled mouse input should not be propagated.
    */
    void setMouseInputPropagationEnabled(const bool enabled);

    /*!
    \brief
        Return whether mouse input that is not directly handled by this Window
        (including it's event subscribers) should be propagated back to the
        Window's parent.

    \return
        - true if unhandled mouse input will be propagated to the parent.
        - false if unhandled mouse input will not be propagated.
    */
    bool isMouseInputPropagationEnabled() const;

    /*!
    \brief
        Clones this Window and returns the result

    \param
        deepCopy if true, even children are copied

    \return
        the cloned Window
    */
    Window* clone(const bool deepCopy = true) const;

    //! copies this widget's properties to given target widget
    virtual void clonePropertiesTo(Window& target) const;
    //! copies this widget's child widgets to given target widget
    virtual void cloneChildWidgetsTo(Window& target) const;

    //! return the GUIContext this window is associated with.
    GUIContext& getGUIContext() const;
    //! function used internally.  Do not call this from client code.
    void setGUIContext(GUIContext* context);

    //! ensure that the window will be rendered to the correct target surface.
    void syncTargetSurface();

    /*!
    \brief
        Set whether this window is marked as an auto window.

        An auto window is typically a Window object created automatically by
        CEGUI - for example to form part of a multi-element 'compound' widget.
    */
    void setAutoWindow(bool is_auto);

    /*!
    \brief
        Return whether Window thinks mouse is currently within its area.

    \note
        If the mouse cursor has moved or Window's area has changed since the
        last time the GUIContext updated the window hit information, the value
        returned here may be inaccurate - this is not a bug, but is required
        to ensure correct handling of certain events.
    */
    bool isMouseContainedInArea() const;

    // overridden from Element
    const Sizef& getRootContainerSize() const;

protected:
    // friend classes for construction / initialisation purposes (for now)
    friend class System;
    friend class WindowManager;
    friend class GUIContext;

    /*************************************************************************
        Event trigger methods
    *************************************************************************/
    /*!
    \brief
        Handler called when the window's size changes.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onSized(ElementEventArgs& e);

    /*!
    \brief
        Handler called when the window's position changes.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onMoved(ElementEventArgs& e);
    
    virtual void onRotated(ElementEventArgs& e);

    /*!
    \brief
        Handler called when the window's text is changed.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onTextChanged(WindowEventArgs& e);

    /*!
    \brief
        Handler called when the window's font is changed.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onFontChanged(WindowEventArgs& e);

    /*!
    \brief
        Handler called when the window's alpha blend value is changed.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onAlphaChanged(WindowEventArgs& e);

    /*!
    \brief
        Handler called when the window's client assigned ID is changed.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onIDChanged(WindowEventArgs& e);

    /*!
    \brief
        Handler called when the window is shown (made visible).

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onShown(WindowEventArgs& e);

    /*!
    \brief
        Handler called when the window is hidden.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onHidden(WindowEventArgs& e);

    /*!
    \brief
        Handler called when the window is enabled.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onEnabled(WindowEventArgs& e);

    /*!
    \brief
        Handler called when the window is disabled.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onDisabled(WindowEventArgs& e);

    /*!
    \brief
        Handler called when the window's setting for being clipped by it's
        parent is changed.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onClippingChanged(WindowEventArgs& e);

    /*!
    \brief
        Handler called when the window's setting for being destroyed
        automatically be it's parent is changed.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onParentDestroyChanged(WindowEventArgs& e);

    /*!
    \brief
        Handler called when the window's setting for inheriting alpha-blending
        is changed.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onInheritsAlphaChanged(WindowEventArgs& e);

    /*!
    \brief
        Handler called when the window's always-on-top setting is changed.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onAlwaysOnTopChanged(WindowEventArgs& e);

    /*!
    \brief
        Handler called when this window gains capture of mouse inputs.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onCaptureGained(WindowEventArgs& e);

    /*!
    \brief
        Handler called when this window loses capture of mouse inputs.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onCaptureLost(WindowEventArgs& e);

    /*!
    \brief
        Handler called when this window gets invalidated.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onInvalidated(WindowEventArgs& e);

    /*!
    \brief
        Handler called when rendering for this window has started.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onRenderingStarted(WindowEventArgs& e);

    /*!
    \brief
        Handler called when rendering for this window has ended.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onRenderingEnded(WindowEventArgs& e);

    /*!
    \brief
        Handler called when the z-order position of this window has changed.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onZChanged(WindowEventArgs& e);

    /*!
    \brief
        Handler called when this window's destruction sequence has begun.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onDestructionStarted(WindowEventArgs& e);

    /*!
    \brief
        Handler called when this window has become the active window.

    \param e
        ActivationEventArgs class whose 'otherWindow' field is set to the window
        that previously was active, or NULL for none.
    */
    virtual void onActivated(ActivationEventArgs& e);

    /*!
    \brief
        Handler called when this window has lost input focus and has been
        deactivated.

    \param e
        ActivationEventArgs object whose 'otherWindow' field is set to the
        window that has now become active, or NULL for none.
    */
    virtual void onDeactivated(ActivationEventArgs& e);

    /*!
    \brief
        Handler called when this window's parent window has been resized.  If
        this window is the root / GUI Sheet window, this call will be made when
        the display size changes.

    \param e
        WindowEventArgs object whose 'window' pointer field is set the the
        window that caused the event; this is typically either this window's
        parent window, or NULL to indicate the screen size has changed.
    */
    virtual void onParentSized(ElementEventArgs& e);

    /*!
    \brief
        Handler called when a child window is added to this window.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that has been added.
    */
    virtual void onChildAdded(ElementEventArgs& e);

    /*!
    \brief
        Handler called when a child window is removed from this window.

    \param e
        WindowEventArgs object whose 'window' pointer field is set the window
        that has been removed.
    */
    virtual void onChildRemoved(ElementEventArgs& e);

    /*!
    \brief
        Handler called when the mouse cursor has entered this window's area.

    \param e
        MouseEventArgs object.  All fields are valid.
    */
    virtual void onMouseEntersArea(MouseEventArgs& e);

    /*!
    \brief
        Handler called when the mouse cursor has left this window's area.

    \param e
        MouseEventArgs object.  All fields are valid.
    */
    virtual void onMouseLeavesArea(MouseEventArgs& e);

    /*!
    \brief
        Handler called when the mouse cursor has entered this window's area and
        is actually over some part of this windows surface and not, for
        instance over a child window - even though technically in those cases
        the mouse is also within this Window's area, the handler will not be
        called.

    \param e
        MouseEventArgs object.  All fields are valid.

    \see
        Window::onMouseEntersArea
    */
    virtual void onMouseEnters(MouseEventArgs& e);

    /*!
    \brief
        Handler called when the mouse cursor is no longer over this window's
        surface area.  This will be called when the mouse is not over a part
        of this Window's actual surface - even though technically the mouse is
        still within the Window's area, for example if the mouse moves over a
        child window.

    \param e
        MouseEventArgs object.  All fields are valid.

    \see
        Window::onMouseLeavesArea
    */
    virtual void onMouseLeaves(MouseEventArgs& e);

    /*!
    \brief
        Handler called when the mouse cursor has been moved within this window's
        area.

    \param e
        MouseEventArgs object.  All fields are valid.
    */
    virtual void onMouseMove(MouseEventArgs& e);

    /*!
    \brief
        Handler called when the mouse wheel (z-axis) position changes within
        this window's area.

    \param e
        MouseEventArgs object.  All fields are valid.
    */
    virtual void onMouseWheel(MouseEventArgs& e);

    /*!
    \brief
        Handler called when a mouse button has been depressed within this
        window's area.

    \param e
        MouseEventArgs object.  All fields are valid.
    */
    virtual void onMouseButtonDown(MouseEventArgs& e);

    /*!
    \brief
        Handler called when a mouse button has been released within this
        window's area.

    \param e
        MouseEventArgs object.  All fields are valid.
    */
    virtual void onMouseButtonUp(MouseEventArgs& e);

    /*!
    \brief
        Handler called when a mouse button has been clicked (that is depressed
        and then released, within a specified time) within this window's area.

    \param e
        MouseEventArgs object.  All fields are valid.
    */
    virtual void onMouseClicked(MouseEventArgs& e);

    /*!
    \brief
        Handler called when a mouse button has been double-clicked within this
        window's area.

    \param e
        MouseEventArgs object.  All fields are valid.
    */
    virtual void onMouseDoubleClicked(MouseEventArgs& e);

    /*!
    \brief
        Handler called when a mouse button has been triple-clicked within this
        window's area.

    \param e
        MouseEventArgs object.  All fields are valid.
    */
    virtual void onMouseTripleClicked(MouseEventArgs& e);

    /*!
    \brief
        Handler called when a key as been depressed while this window has input
        focus.

    \param e
        KeyEventArgs object whose 'scancode' field is set to the Key::Scan value
        representing the key that was pressed, and whose 'sysKeys' field
        represents the combination of SystemKey that were active when the event
        was generated.
    */
    virtual void onKeyDown(KeyEventArgs& e);

    /*!
    \brief
        Handler called when a key as been released while this window has input
        focus.

    \param e
        KeyEventArgs object whose 'scancode' field is set to the Key::Scan value
        representing the key that was released, and whose 'sysKeys' field
        represents the combination of SystemKey that were active when the event
        was generated.  All other fields should be considered as 'junk'.
    */
    virtual void onKeyUp(KeyEventArgs& e);

    /*!
    \brief
        Handler called when a character-key has been pressed while this window
        has input focus.

    \param e
        KeyEventArgs object whose 'codepoint' field is set to the Unicode code
        point (encoded as utf32) for the character typed, and whose 'sysKeys'
        field represents the combination of SystemKey that were active when the
        event was generated.  All other fields should be considered as 'junk'.
    */
    virtual void onCharacter(KeyEventArgs& e);

    /*!
    \brief
        Handler called when a DragContainer is dragged over this window.

    \param e
        DragDropEventArgs object initialised as follows:
        - window field is normaly set to point to 'this' window.
        - dragDropItem is a pointer to a DragContainer window that triggered
          the event.
    */
    virtual void onDragDropItemEnters(DragDropEventArgs& e);

    /*!
    \brief
        Handler called when a DragContainer is dragged over this window.

    \param e
        DragDropEventArgs object initialised as follows:
        - window field is normaly set to point to 'this' window.
        - dragDropItem is a pointer to a DragContainer window that triggered
          the event.
    */
    virtual void onDragDropItemLeaves(DragDropEventArgs& e);

    /*!
    \brief
        Handler called when a DragContainer is dragged over this window.

    \param e
        DragDropEventArgs object initialised as follows:
        - window field is normaly set to point to 'this' window.
        - dragDropItem is a pointer to a DragContainer window that triggered
          the event.
    */
    virtual void onDragDropItemDropped(DragDropEventArgs& e);

    /*!
    \brief
        Handler called when a new window renderer object is attached.

    \param e
        WindowEventArgs object initialised as follows:
        - window field is set to point to the Window object that just got a new
          window renderer attached. (typically 'this').
    */
    virtual void onWindowRendererAttached(WindowEventArgs& e);

    /*!
    \brief
        Handler called when the currently attached window renderer object is detached.

    \param e
        WindowEventArgs object initialised as follows:
        - window field is set to point to the Window object that just got lost its
          window renderer. (typically 'this').
    */
    virtual void onWindowRendererDetached(WindowEventArgs& e);

    /*!
    \brief
        Handler called when the window's setting for whether text parsing is
        enabled is changed.

    \param e
        WindowEventArgs object whose 'window' pointer field is set to the window
        that triggered the event.  For this event the trigger window is always
        'this'.
    */
    virtual void onTextParsingChanged(WindowEventArgs& e);

    virtual void onMarginChanged(WindowEventArgs& e);

    /*************************************************************************
        Implementation Functions
    *************************************************************************/
    /*!
    \brief
        Perform actual update processing for this Window.

    \param elapsed
        float value indicating the number of seconds elapsed since the last
        update call.

    \return
        Nothing.
    */
    virtual void updateSelf(float elapsed);

    /*!
    \brief
        Perform the actual rendering for this Window.

    \param ctx
        RenderingContext holding the details of the RenderingSurface to be
        used for the Window rendering operations.

    \return
        Nothing
    */
    virtual void drawSelf(const RenderingContext& ctx);

    /*!
    \brief
        Perform drawing operations concerned with generating and buffering
        window geometry.

    \note
        This function is a sub-function of drawSelf; it is provided to make it
        easier to override drawSelf without needing to duplicate large sections
        of the code from the default implementation.
    */
    void bufferGeometry(const RenderingContext& ctx);

    /*!
    \brief
        Perform drawing operations concerned with positioning, clipping and
        queueing of window geometry to RenderingSurfaces.

    \note
        This function is a sub-function of drawSelf and is provided to make it
        easier to override drawSelf without needing to duplicate large sections
        of the code from the default implementation.
    */
    void queueGeometry(const RenderingContext& ctx);

    /*!
    \brief
        Update the rendering cache.

        Populates the Window's GeometryBuffer ready for rendering.
    */
    virtual void populateGeometryBuffer()  {}

    /*!
    \brief
        Set the parent window for this window object.

    \param parent
        Pointer to a Window object that is to be assigned as the parent to this
        Window.

    \return
        Nothing
    */
    virtual void setParent(Element* parent);

    /*!
    \brief
        Fires off a repeated mouse button down event for this window.
    */
    void generateAutoRepeatEvent(MouseButton button);

    /*!
    \brief
        Function used in checking if a WindowRenderer is valid for this window.

    \param renderer
    	Window renderer that will be checked (it can be null!)

    \return
        Returns true if the given WindowRenderer class name is valid for this window.
        False if not.
    */
    virtual bool validateWindowRenderer(const WindowRenderer* renderer) const;

    /*!
    \brief
        Returns whether a property is at it's default value.
        This function is different from Property::isDefatult as it takes the assigned look'n'feel
        (if the is one) into account.
    */
    bool isPropertyAtDefault(const Property* property) const;

    /*!
    \brief
        Recursively inform all children that the clipping has changed and screen rects
        needs to be recached.
    */
    void notifyClippingChanged(void);

    //! helper to create and setup the auto RenderingWindow surface
    void allocateRenderingWindow();

    //! helper to clean up the auto RenderingWindow surface
    void releaseRenderingWindow();

    //! Helper to intialise the needed clipping for geometry and render surface.
    void initialiseClippers(const RenderingContext& ctx);

    //! \copydoc Element::setArea_impl
    virtual void setArea_impl(const UVector2& pos, const USize& size, bool topLeftSizing = false, bool fireEvents = true);
    
    /*!
    \brief
        Cleanup child windows
    */
    virtual void cleanupChildren(void);

    /*!
    \copydoc Element::addChild_impl
    */
    virtual void addChild_impl(Element* element);

    /*!
    \copydoc Element::removeChild_impl
    */
    virtual void removeChild_impl(Element* element);

    /*!
    \brief
        Notify 'this' and all siblings of a ZOrder change event
    */
    virtual void onZChange_impl(void);

    /*!
    \brief
        Add standard CEGUI::Window properties.
    */
    void    addWindowProperties(void);

    /*!
    \brief
        Implements move to front behavior.

    \return
        Should return true if some action was taken, or false if there was
        nothing to be done.
    */
    virtual bool moveToFront_impl(bool wasClicked);

    /*!
    \brief
        Add the given window to the drawing list at an appropriate position for
        it's settings and the required direction.  Basically, when \a at_back
        is false, the window will appear in front of all other windows with the
        same 'always on top' setting.  When \a at_back is true, the window will
        appear behind all other windows wih the same 'always on top' setting.

    \param wnd
        Window object to be added to the drawing list.

    \param at_back
        Indicates whether the window should be placed at the back of other
        windows in the same group. If this is false, the window is placed in
        front of other windows in the group.

    \return
        Nothing.
    */
    void addWindowToDrawList(Window& wnd, bool at_back = false);

    /*!
    \brief
        Removes the window from the drawing list.  If the window is not attached
        to the drawing list then nothing happens.

    \param wnd
        Window object to be removed from the drawing list.

    \return
        Nothing.
    */
    void removeWindowFromDrawList(const Window& wnd);
    
    /*!
    \brief
        Return whether the window is at the top of the Z-Order.  This will
        correctly take into account 'Always on top' windows as needed.

    \return
        - true if the Window is at the top of the z-order in relation to sibling
          windows with the same 'always on top' setting.
        - false if the Window is not at the top of the z-order in relation to
          sibling windows with the same 'always on top' setting.
    */
    bool isTopOfZOrder() const;

    /*!
    \brief
        Update position and clip region on this Windows geometry / rendering
        surface.
    */
    void updateGeometryRenderSettings();

    //! transfer RenderingSurfaces to be owned by our target RenderingSurface.
    void transferChildSurfaces();

    //! helper function for calculating clipping rectangles.
    Rectf getParentElementClipIntersection(const Rectf& unclipped_area) const;

    //! helper function to invalidate window and optionally child windows.
    void invalidate_impl(const bool recursive);

    /*!
    \brief
        Helper function to return the ancestor Window of /a wnd that is attached
        as a child to a window that is also an ancestor of /a this.  Returns 0
        if /a wnd and /a this are not part of the same hierachy.
     */
    const Window* getWindowAttachedToCommonAncestor(const Window& wnd) const;

    virtual Rectf getUnclippedInnerRect_impl(bool skipAllPixelAlignment) const;
    //! Default implementation of function to return Window outer clipper area.
    virtual Rectf getOuterRectClipper_impl() const;
    //! Default implementation of function to return Window inner clipper area.
    virtual Rectf getInnerRectClipper_impl() const;
    //! Default implementation of function to return Window hit-test area.
    virtual Rectf getHitTestRect_impl() const;
    
    virtual int writePropertiesXML(XMLSerializer& xml_stream) const;
    virtual int writeChildWindowsXML(XMLSerializer& xml_stream) const;
    virtual bool writeAutoChildWindowXML(XMLSerializer& xml_stream) const;

    virtual void banPropertiesForAutoWindow();

    //! handler function for when font render size changes.
    virtual bool handleFontRenderSizeChange(const EventArgs& args);

    // mark the rect caches defined on Window invalid (does not affect Element)
    void markCachedWindowRectsInvalid();
    void layoutLookNFeelChildWidgets();

    Window* getChildAtPosition(const Vector2f& position,
                               bool (Window::*hittestfunc)(const Vector2f&, bool) const,
                               bool allow_disabled = false) const;

    bool isHitTargetWindow(const Vector2f& position, bool allow_disabled) const;

    /*************************************************************************
        Properties for Window base class
    *************************************************************************/

    /*!
    \brief
        Property to access/change the assigned window renderer object.

        \par Usage:
            - Name: WindowRenderer
            - Format: "[windowRendererName]"

        \par Where [windowRendererName] is the factory name of the window
             renderer type you wish to assign.
    */
    static
    class WindowRendererProperty : public TplWindowProperty<Window, String>
    {
    public:
        WindowRendererProperty();
        void writeXMLToStream(const PropertyReceiver* receiver,
                              XMLSerializer& xml_stream) const;
    } d_windowRendererProperty;

    /*!
    \brief
        Property to access/change the assigned look'n'feel.

        \par Usage:
            - Name: LookNFeel
            - Format: "[LookNFeelName]"

        \par Where [LookNFeelName] is the name of the look'n'feel you wish
             to assign.
    */
    static
    class LookNFeelProperty : public TplWindowProperty<Window, String>
    {
    public:
        LookNFeelProperty();
        void writeXMLToStream(const PropertyReceiver* receiver,
                              XMLSerializer& xml_stream) const;
    } d_lookNFeelProperty;

    /*************************************************************************
        Implementation Data
    *************************************************************************/
    //! definition of type used for the list of child windows to be drawn
    typedef std::vector<Window*
        CEGUI_VECTOR_ALLOC(Window*)> ChildDrawList;
    //! definition of type used for the UserString dictionary.
    typedef std::map<String, String, StringFastLessCompare
        CEGUI_MAP_ALLOC(String, String)> UserStringMap;
    //! definition of type used to track properties banned from writing XML.
    typedef std::set<String, StringFastLessCompare
        CEGUI_SET_ALLOC(String)> BannedXMLPropertySet;

    //! type of Window (also the name of the WindowFactory that created us)
    const String d_type;
    //! Type name of the window as defined in a Falagard mapping.
    String d_falagardType;
    //! true when this window is an auto-window
    bool d_autoWindow;

    //! true when this window is currently being initialised (creating children etc)
    bool d_initialising;
    //! true when this window is being destroyed.
    bool d_destructionStarted;
    //! true when Window is enabled
    bool d_enabled;
    //! is window visible (i.e. it will be rendered, but may still be obscured)
    bool d_visible;
    //! true when Window is the active Window (receiving inputs).
    bool d_active;

    //! Child window objects arranged in rendering order.
    ChildDrawList d_drawList;
    //! true when Window will be auto-destroyed by parent.
    bool d_destroyedByParent;

    //! true when Window will be clipped by parent Window area Rect.
    bool d_clippedByParent;

    //! Name of the Look assigned to this window (if any).
    String d_lookName;
    //! The WindowRenderer module that implements the Look'N'Feel specification
    WindowRenderer* d_windowRenderer;
    //! Object which acts as a cache of geometry drawn by this Window.
    GeometryBuffer* d_geometry;
    //! RenderingSurface owned by this window (may be 0)
    RenderingSurface* d_surface;
    //! true if window geometry cache needs to be regenerated.
    mutable bool d_needsRedraw;
    //! holds setting for automatic creation of of surface (RenderingWindow)
    bool d_autoRenderingWindow;

    //! Holds pointer to the Window objects current mouse cursor image.
    const Image* d_mouseCursor;

    //! Alpha transparency setting for the Window
    float d_alpha;
    //! true if the Window inherits alpha from the parent Window
    bool d_inheritsAlpha;

    //! The Window that previously had capture (used for restoreOldCapture mode)
    Window* d_oldCapture;
    //! Restore capture to the previous capture window when releasing capture.
    bool d_restoreOldCapture;
    //! Whether to distribute captured inputs to child windows.
    bool d_distCapturedInputs;

    //! Holds pointer to the Window objects current Font.
    const Font* d_font;
    //! Holds the text / label / caption for this Window.
    String d_textLogical;
    //! pointer to bidirection support object
    BidiVisualMapping* d_bidiVisualMapping;
    //! whether bidi visual mapping has been updated since last text change.
    mutable bool d_bidiDataValid;
    //! RenderedString representation of text string as ouput from a parser.
    mutable RenderedString d_renderedString;
    //! true if d_renderedString is valid, false if needs re-parse.
    mutable bool d_renderedStringValid;
    //! Shared instance of a parser to be used in most instances.
    static BasicRenderedStringParser d_basicStringParser;
    //! Shared instance of a parser to be used when rendering text verbatim.
    static DefaultRenderedStringParser d_defaultStringParser;
    //! Pointer to a custom (user assigned) RenderedStringParser object.
    RenderedStringParser* d_customStringParser;
    //! true if use of parser other than d_defaultStringParser is enabled
    bool d_textParsingEnabled;

	//! Margin, only used when the Window is inside LayoutContainer class
    UBox d_margin;

    //! User ID assigned to this Window
    uint d_ID;
    //! Holds pointer to some user assigned data.
    void* d_userData;
    //! Holds a collection of named user string values.
    UserStringMap d_userStrings;

    //! true if Window will be drawn on top of all other Windows
    bool d_alwaysOnTop;
    //! whether window should rise in the z order when left clicked.
    bool d_riseOnClick;
    //! true if the Window responds to z-order change requests.
    bool d_zOrderingEnabled;

    //! true if the Window wishes to hear about multi-click mouse events.
    bool d_wantsMultiClicks;
    //! whether (most) mouse events pass through this window
    bool d_mousePassThroughEnabled;
    //! whether pressed mouse button will auto-repeat the down event.
    bool d_autoRepeat;
    //! seconds before first repeat event is fired
    float d_repeatDelay;
    //! seconds between further repeats after delay has expired.
    float d_repeatRate;
    //! button we're tracking for auto-repeat purposes.
    MouseButton d_repeatButton;
    //! implements repeating - is true after delay has elapsed,
    bool d_repeating;
    //! implements repeating - tracks time elapsed.
    float d_repeatElapsed;

    //! true if window will receive drag and drop related notifications
    bool d_dragDropTarget;

    //! Text string used as tip for this window.
    String d_tooltipText;
    //! Possible custom Tooltip for this window.
    Tooltip* d_customTip;
    //! true if this Window created the custom Tooltip.
    bool d_weOwnTip;
    //! whether tooltip text may be inherited from parent.
    bool d_inheritsTipText;

    //! true if this window is allowed to write XML, false if not
    bool d_allowWriteXML;
    //! collection of properties not to be written to XML for this window.
    BannedXMLPropertySet d_bannedXMLProperties;

    //! outer area clipping rect in screen pixels
    mutable Rectf d_outerRectClipper;
    //! inner area clipping rect in screen pixels
    mutable Rectf d_innerRectClipper;
    //! area rect used for hit-testing against this window
    mutable Rectf d_hitTestRect;

    mutable bool d_outerRectClipperValid;
    mutable bool d_innerRectClipperValid;
    mutable bool d_hitTestRectValid;

    //! The mode to use for calling Window::update
    WindowUpdateMode d_updateMode;

    //! specifies whether mouse inputs should be propagated to parent(s)
    bool d_propagateMouseInputs;

    //! GUIContext.  Set when this window is used as a root window.
    GUIContext* d_guiContext;

    //! true when mouse is contained within this Window's area.
    bool d_containsMouse;

private:
    /*************************************************************************
        May not copy or assign Window objects
    *************************************************************************/
    Window(const Window&): NamedElement() {}
    Window& operator=(const Window&) {return *this;}

    //! Not intended for public use, only used as a "Font" property getter
    const Font* property_getFont() const;
    //! Not intended for public use, only used as a "MouseCursor" property getter
    const Image* property_getMouseCursor() const;

    //! connection for event listener for font render size changes.
    Event::ScopedConnection d_fontRenderSizeChangeConnection;
};

} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIWindow_h_

