/***********************************************************************
    created:    Thu Jul 7 2005
    author:     Paul D Turner <paul@cegui.org.uk>
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
#ifndef _FalMultiLineEditbox_h_
#define _FalMultiLineEditbox_h_

#include "CEGUI/WindowRendererSets/Core/Module.h"
#include "CEGUI/widgets/MultiLineEditbox.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    MultiLineEditbox class for the FalagardBase module.

    This class requires LookNFeel to be assigned.  The LookNFeel should provide the following:

    States:
        - Enabled    - Rendering for when the editbox is in enabled and is in read-write mode.
        - ReadOnly  - Rendering for when the editbox is in enabled and is in read-only mode.
        - Disabled  - Rendering for when the editbox is disabled.

    NamedAreas:
        TextArea         - area where text, selection, and caret imagery will appear.
        TextAreaHScroll  - TextArea when only horizontal scrollbar is visible.
        TextAreaVScroll  - TextArea when only vertical scrollbar is visible.
        TextAreaHVScroll - TextArea when both horizontal and vertical scrollbar is visible.

    PropertyDefinitions (optional, defaults will be black):
        - NormalTextColour        - property that accesses a colour value to be used to render normal unselected text.
        - SelectedTextColour      - property that accesses a colour value to be used to render selected text.
        - ActiveSelectionColour   - property that accesses a colour value to be used to render active selection highlight.
        - InactiveSelectionColour - property that accesses a colour value to be used to render inactive selection highlight.

    Imagery Sections:
        - Caret

    Child Widgets:
        Scrollbar based widget with name suffix "__auto_vscrollbar__"
        Scrollbar based widget with name suffix "__auto_hscrollbar__"

*/
class COREWRSET_API FalagardMultiLineEditbox : public MultiLineEditboxWindowRenderer
{
public:
    static const String TypeName;     //! type name for this widget.

    //! Name of the optional property to access for the unselected text colour.
    static const String UnselectedTextColourPropertyName;
    //! Name of the optional property to access for the selected text colour.
    static const String SelectedTextColourPropertyName;
    //! Name of the optional property to access to obtain active selection rendering colour.
    static const String ActiveSelectionColourPropertyName;
    //! Name of the optional property to access to obtain inactive selection rendering colour.
    static const String InactiveSelectionColourPropertyName;
    //! The default timeout (in seconds) used when blinking the caret.
    static const float DefaultCaretBlinkTimeout;

    /*!
    \brief
        Constructor
    */
    FalagardMultiLineEditbox(const String& type);

    // overridden from base classes.
    Rectf getTextRenderArea(void) const;
    void render();
    void update(float elapsed);

    //! return whether the blinking caret is enabled.
    bool isCaretBlinkEnabled() const;
    //! return the caret blink timeout period (only used if blink is enabled).
    float getCaretBlinkTimeout() const;
    //! set whether the blinking caret is enabled.
    void setCaretBlinkEnabled(bool enable);
    //! set the caret blink timeout period (only used if blink is enabled).
    void setCaretBlinkTimeout(float seconds);

    // overridden from base class
    bool handleFontRenderSizeChange(const Font* const font);

protected:
    /*!
    \brief
        Perform rendering of the widget control frame and other 'static' areas.  This
        method should not render the actual text.  Note that the text will be rendered
        to layer 4 and the selection brush to layer 3, other layers can be used for
        rendering imagery behind and infront of the text & selection..

    \return
        Nothing.
    */
    void cacheEditboxBaseImagery();

    /*!
    \brief
        Render the caret.

    \return
        Nothing
    */
    void cacheCaretImagery(const Rectf& textArea);

    /*!
    \brief
        Render text lines.
    */
    void cacheTextLines(const Rectf& dest_area);

    /*!
    \brief
        Set the given ColourRect to the colour to be used for rendering Editbox
        text oustside of the selected region.
    */
    void setColourRectToUnselectedTextColour(ColourRect& colour_rect) const;

    /*!
    \brief
        Set the given ColourRect to the colour to be used for rendering Editbox
        text falling within the selected region.
    */
    void setColourRectToSelectedTextColour(ColourRect& colour_rect) const;

    /*!
    \brief
        Set the given ColouRect to the colours to be used for rendering the
        selection highlight when the editbox is active.
    */
    void setColourRectToActiveSelectionColour(ColourRect& colour_rect) const;

    /*!
    \brief
        set the given ColourRect to the colours to be used for rendering the
        selection highlight when the editbox is inactive.
    */
    void setColourRectToInactiveSelectionColour(ColourRect& colour_rect) const;

    /*!
    \brief
        Set the given ColourRect to the colour(s) fetched from the named
        property if it exists, else the default colour of black.

    \param propertyName
        String object holding the name of the property to be accessed if it
        exists.

    \param colour_rect
        Reference to a ColourRect that will be set.
    */
    void setColourRectToOptionalPropertyColour(const String& propertyName,
                                               ColourRect& colour_rect) const;

    //! true if the caret imagery should blink.
    bool d_blinkCaret;
    //! time-out in seconds used for blinking the caret.
    float d_caretBlinkTimeout;
    //! current time elapsed since last caret blink state change.
    float d_caretBlinkElapsed;
    //! true if caret should be shown.
    bool d_showCaret;
};

} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif  // end of guard _FalMultiLineEditbox_h_
