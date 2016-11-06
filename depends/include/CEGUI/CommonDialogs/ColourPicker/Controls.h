/***********************************************************************
    created:    20th February 2010
    author:     Lukas E Meindl

    purpose:    Interface to base class for ColourPickerControls Widget
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
#ifndef _CEGUIColourPickerControls_h_
#define _CEGUIColourPickerControls_h_

#include "CEGUI/CommonDialogs/Module.h"
#include "CEGUI/Window.h"
#include "CEGUI/CommonDialogs/ColourPicker/Types.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
class ColourPicker;

//! Base class for the ColourPickerControls widget
class CEGUI_COMMONDIALOGS_API ColourPickerControls : public Window
{
public:
    enum SliderMode
    {
        SliderMode_Lab_L = 1,
        SliderMode_Lab_A = 1 << 1,
        SliderMode_Lab_B = 1 << 2,
        SliderMode_HSV_H = 1 << 3,
        SliderMode_HSV_S = 1 << 4,
        SliderMode_HSV_V = 1 << 5
    };

    //! Constructor for ColourPickerControls class.
    ColourPickerControls(const String& type, const String& name);

    //! Destructor for ColourPickerControls class.
    ~ColourPickerControls();

    //!< Namespace for global events
    static const String EventNamespace;
    //! Window type name
    static const String WidgetTypeName;

    /** Event fired when the button is clicked.
    * Handlers are passed a const WindowEventArgs reference with
    * WindowEventArgs::window set to the PushButton that was clicked.
    */
    static const String EventColourAccepted;
    static const String EventClosed;

    RGB_Colour getSelectedColourRGB();

    /*!
    \brief
        Set the current colour of the colour picker manually and refresh the
        ColourPickerControls elements accordingly.

    \param setting
        newColour the selected Colour for the ColourPickerControls
    */
    void setColours(const Colour& newColour);

    void setPreviousColour(const Colour& previousColour);

    void refreshAllElements();

    /*!
    \brief
        Set the local d_callingColourPicker pointer to the colour picker window.

    \param setting
        colourPicker the ColourPicker window.
    */
    void setCallingColourPicker(ColourPicker* colourPicker);

    // overridden from window
    void initialiseComponents();

protected:
    //! Widget name for the cancel button component.
    static const String CancelButtonName;
    //! Widget name for the accept button component.
    static const String AcceptButtonName;
    //! Widget name for the static text label of the hexadecimal colour value EditBox.
    static const String HexaDecimalDescriptionName;
    //! Widget name for the colour value EditBox.
    static const String HexaDecimalEditBoxName;
    static const String TitleBarName;
    static const String ColourPickerStaticImageName;
    static const String ColourPickerImageSliderName;
    static const String ColourPickerAlphaSliderName;
    static const String NewColourDescriptionName;
    static const String OldColourDescriptionName;
    static const String NewColourRectName;
    static const String OldColourRectName;
    static const String ColourEditBoxRDescriptionName;
    static const String ColourEditBoxGDescriptionName;
    static const String ColourEditBoxBDescriptionName;
    static const String ColourEditBoxRName;
    static const String ColourEditBoxGName;
    static const String ColourEditBoxBName;
    static const String HSVRadioButtonHName;
    static const String HSVRadioButtonSName;
    static const String HSVRadioButtonVName;
    static const String HSVEditBoxHName;
    static const String HSVEditBoxSName;
    static const String HSVEditBoxVName;
    static const String LabRadioButtonLName;
    static const String LabRadioButtonAName;
    static const String LabRadioButtonBName;
    static const String LabEditBoxLName;
    static const String LabEditBoxAName;
    static const String LabEditBoxBName;
    static const String AlphaEditBoxName;
    static const String ColourPickerCursorName;
    //! Name of the Picking Texture Image in the ImageSet
    static const String ColourPickerControlsPickingTextureImageName;
    //! Name of the Slider Texture Image in the ImageSet
    static const String ColourPickerControlsColourSliderTextureImageName;
    //! Name of the Slider Texture Image in the ImageSet
    static const String ColourPickerControlsAlphaSliderTextureImageName;
    //! Name of the colour defining ColourRectProperty of ColourRect windows
    static const String ColourRectPropertyName;

    static const float LAB_L_MIN;
    static const float LAB_L_MAX;
    static const float LAB_L_DIFF;
    static const float LAB_A_MIN;
    static const float LAB_A_MAX;
    static const float LAB_A_DIFF;
    static const float LAB_B_MIN;
    static const float LAB_B_MAX;
    static const float LAB_B_DIFF;

    void initColourPickerControlsImageSet();
    void deinitColourPickerControlsTexture();
    void refreshColourPickerControlsTextures();

    void refreshColourSliderImage();
    void refreshColourPickingImage();
    void refreshAlphaSliderImage();

    void reloadColourPickerControlsTexture();

    Lab_Colour getColourSliderPositionColourLAB(float value);
    Lab_Colour getColourPickingPositionColourLAB(float xAbs, float yAbs);

    HSV_Colour getColourSliderPositionColourHSV(float value);
    HSV_Colour getColourPickingPositionColourHSV(float xAbs, float yAbs);

    RGB_Colour getAlphaSliderPositionColour(int x, int y);

    Vector2f getColourPickingColourPosition();
    void getColourPickingColourPositionHSV(float& x, float& y);

    // Handlers to relay child widget events so they appear to come from us
    bool handleCancelButtonClicked(const EventArgs& e);
    bool handleAcceptButtonClicked(const EventArgs& e);
    bool handleEditboxDeactivated(const EventArgs& args);
    bool handleRadioButtonModeSelection(const EventArgs& args);

    bool handleHexadecimalEditboxTextChanged(const EventArgs& args);
    bool handleRGBEditboxTextChanged(const EventArgs& args);
    bool handleLABEditboxTextChanged(const EventArgs& args);
    bool handleHSVEditboxTextChanged(const EventArgs& args);

    bool handleAlphaEditboxTextChanged(const EventArgs& args);

    bool handleColourPickerStaticImageMouseLeaves(const EventArgs& args);
    bool handleColourPickerStaticImageMouseButtonUp(const EventArgs& args);
    bool handleColourPickerStaticImageMouseButtonDown(const EventArgs& args);
    bool handleColourPickerStaticImageMouseMove(const EventArgs& args);

    virtual void onCancelButtonClicked(WindowEventArgs& e);
    virtual void onAcceptButtonClicked(WindowEventArgs& e);

    void onColourCursorPositionChanged();
    void onColourSliderChanged();

    void refreshColourPickerCursorPosition(const MouseEventArgs& mouseEventArgs);
    void refreshAlpha();

    void refreshOnlyColourSliderImage();
    bool handleColourPickerSliderValueChanged(const EventArgs& args);
    bool handleAlphaSliderValueChanged(const EventArgs& args);

    void refreshEditboxesAndColourRects();

    void refreshColourRects();
    void positionColourPickerCursorAbsolute(float x, float y);
    void positionColourPickerCursorRelative(float x, float y);
    void setColours(const Lab_Colour& newColourLAB);
    void setColours(const RGB_Colour& newColourRGB);
    void setColours(const HSV_Colour& newColourHSV);

    void setColourAlpha(float alphaValue);

    void refreshColourPickerCursorPosition();
    void refreshColourSliderPosition();

    void initColourPicker();
    PushButton* getCancelButton();
    PushButton* getAcceptButton();
    Window* getHexadecimalDescription();
    Editbox* getHexadecimalEditbox();
    Titlebar* getTitleBar();
    Slider* getColourPickerImageSlider();
    Slider* getColourPickerAlphaSlider();
    Window* getColourPickerStaticImage();
    Window* getNewColourDescription();
    Window* getOldColourDescription();
    Window* getNewColourRect();
    Window* getOldColourRect();
    Window* getColourEditBoxRDescription();
    Window* getColourEditBoxGDescription();
    Window* getColourEditBoxBDescription();
    Editbox* getColourEditBoxR();
    Editbox* getColourEditBoxG();
    Editbox* getColourEditBoxB();
    RadioButton* getHSVRadioButtonH();
    RadioButton* getHSVRadioButtonS();
    RadioButton* getHSVRadioButtonV();
    Editbox* getHSVEditBoxH();
    Editbox* getHSVEditBoxS();
    Editbox* getHSVEditBoxV();
    RadioButton* getLabRadioButtonL();
    RadioButton* getLabRadioButtonA();
    RadioButton* getLabRadioButtonB();
    Editbox* getLabEditBoxL();
    Editbox* getLabEditBoxA();
    Editbox* getLabEditBoxB();
    Editbox* getAlphaEditBox();
    Window* getColourPickerCursorStaticImage();

    ColourPicker* d_callingColourPicker;
    Window* d_colourPickerCursor;

    SliderMode d_sliderMode;

    //! selected colour of the ColourPickerControls as CEGUI colour
    Colour d_selectedColour;
    /** selected colour of the ColourPickerControls as RGB_Colour
     * (Each colour component as unsigned char 0 - 255)
     */
    RGB_Colour d_selectedColourRGB;
    /** selected colour of the ColourPickerControls as Lab_Colour
     * (Each component as float)
     */
    Lab_Colour d_selectedColourLAB;
    /** selected colour of the ColourPickerControls as HSV_Colour
     * (Each component as float)
     */
    HSV_Colour d_selectedColourHSV;

    //! Previously selected colour of the ColourPickerControls
    Colour d_previouslySelectedColour;

    TextureTarget* d_colourPickerControlsTextureTarget;

    int d_colourPickerImageOffset;
    int d_colourPickerPickingImageHeight;
    int d_colourPickerPickingImageWidth;
    int d_colourPickerColourSliderImageWidth;
    int d_colourPickerColourSliderImageHeight;
    int d_colourPickerAlphaSliderImageWidth;
    int d_colourPickerAlphaSliderImageHeight;

    int d_colourPickerControlsTextureSize;

    bool d_draggingColourPickerCursor;

    RGB_Colour* d_colourPickingTexture;

    bool d_ignoreEvents;
    RegexMatcher& d_regexMatcher;
};

}

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif


#endif

