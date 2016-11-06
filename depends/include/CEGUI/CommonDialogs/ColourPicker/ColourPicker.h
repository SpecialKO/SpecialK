/***********************************************************************
    created:    30/10/2010
    author:     Lukas E Meindl

    purpose:    Interface to base class for ColourPicker Widget
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
#ifndef _CEGUIColourPicker_h_
#define _CEGUIColourPicker_h_

#include "CEGUI/CommonDialogs/Module.h"
#include "CEGUI/CommonDialogs/ColourPicker/Controls.h"
#include "CEGUI/Window.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{

//!Base class for the ColourPicker widget
class CEGUI_COMMONDIALOGS_API ColourPicker : public Window
{
public:
    //! Constructor for ColourPicker class.
    ColourPicker(const String& type, const String& name);
    //! Destructor for ColourPicker class.
    ~ColourPicker(void);

    //! Namespace for global events
    static const String EventNamespace;
    //! Window factory name
    static const String WidgetTypeName;

    // generated internally by Window
    /** Event fired when the ColourPickerControls window is opened.
    * Handlers are passed a const WindowEventArgs reference with
    * WindowEventArgs::window set to the Window that triggered this event.
    */
    static const String EventOpenedPicker;
    // generated internally by Window
    /** Event fired when the ColourPickerControls window is closed.
    * Handlers are passed a const WindowEventArgs reference with
    * WindowEventArgs::window set to the Window that triggered this event.
    */
    static const String EventClosedPicker;

    // generated internally by Window
    /** Event fired when a new colour is set and accepted by the colour picker.
    * Handlers are passed a const WindowEventArgs reference with
    * WindowEventArgs::window set to the Window that triggered this event.
    */
    static const String EventAcceptedColour;

    /*!
    \brief
        Set the current colour of the colour picker manually and refresh the
        ColourPicker elements accordingly.

    \param setting
        newColour the selected Colour for the ColourPicker
    */
    void setColour(const Colour& newColour);

    /*!
    \brief
        Return the current colour of the colour picker.
    */
    Colour getColour();

    // overridden from Window base class
    void initialiseComponents(void);

protected:
    //! Widget name for the open button (colour rect) component.
    static const String ColourRectName;

    /*!\brief
        Initialises the properties for the creation of the ColourPickerControls
        window and decides if a new window of this type is necessary.

    \note
        This will be called once during the initialisation of the components.
    */
    void initialiseColourPickerControlsWindow();


    /*!\brief
        Creates the ColourPickerControls window.

    \param colourPickerControlsStyle
        The window type of the window that will be created.

    \note
        This will be called once during the initialisation of the components.
    */
    void createColourPickerControlsWindow(const String& colourPickerControlsStyle);

    /*!
    \brief
        Return a Window pointer to the ColourRect component widget for this
        ColourPicker.

    \return
        Pointer to a Window object.

    \exception UnknownObjectException
        Thrown if the colour rectangle component does not exist.
    */
    Window* getColourRect(void);

    bool colourRect_ColourRectClickedHandler(const EventArgs& e);
    virtual void onColourRectClicked(WindowEventArgs& e);

    static std::map<Window*, int> s_colourPickerWindows;

    bool d_shareColourPickerControlsWindow;

    ColourPickerControls* d_colourPickerControlsWindow;

    //! selected colour of the ColourPickerControls
    Colour d_selectedColour;
};

}

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif

