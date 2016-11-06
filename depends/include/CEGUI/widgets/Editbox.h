/***********************************************************************
    created:    13/4/2004
    author:     Paul D Turner

    purpose:    Interface to base class for Editbox widget
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
#ifndef _CEGUIEditbox_h_
#define _CEGUIEditbox_h_

#include "CEGUI/Base.h"
#include "CEGUI/Window.h"
#include "CEGUI/RegexMatcher.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
//! Base class for the EditboxWindowRenderer class
class CEGUIEXPORT EditboxWindowRenderer : public WindowRenderer
{
public:
    //! Constructor
    EditboxWindowRenderer(const String& name);

    /*!
    \brief
        Return the text code point index that is rendered closest to screen
        position \a pt.

    \param pt
        Point object describing a position on the screen in pixels.

    \return
        Code point index into the text that is rendered closest to screen
        position \a pt.
    */
    virtual size_t getTextIndexFromPosition(const Vector2f& pt) const = 0;
};

//----------------------------------------------------------------------------//

//! Base class for an Editbox widget
class CEGUIEXPORT Editbox : public Window
{
public:
    typedef RegexMatcher::MatchState MatchState;

    //! Namespace for global events
    static const String EventNamespace;
    //! Window factory name
    static const String WidgetTypeName;
    /** Event fired when the read-only mode for the edit box is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Editbox whose read only setting
     * has been changed.
     */
    static const String EventReadOnlyModeChanged;
    /** Event fired when the masked rendering mode (password mode) is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Editbox that has been put into or
     * taken out of masked text (password) mode.
     */
    static const String EventMaskedRenderingModeChanged;
    /** Event fired whrn the code point (character) used for masked text is
     * changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Editbox whose text masking codepoint
     * has been changed.
     */
    static const String EventMaskCodePointChanged;
    /** Event fired when the validation string is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Editbox whose validation string has
     * been changed.
     */
    static const String EventValidationStringChanged;
    /** Event fired when the maximum allowable string length is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Editbox whose maximum string length
     * has been changed.
     */
    static const String EventMaximumTextLengthChanged;
    /** Event fired when the validity of the Exitbox text (as determined by a
     * RegexMatcher object) has changed.
     * Handlers are passed a const RegexMatchStateEventArgs reference with
     * WindowEventArgs::window set to the Editbox whose text validity has
     * changed and RegexMatchStateEventArgs::matchState set to the new match
     * validity. Handler return is significant, as follows:
     * - true indicates the new state - and therfore text - is to be accepted.
     * - false indicates the new state is not acceptable, and the previous text
     *   should remain in place. NB: This is only possible when the validity
     *   change is due to a change in the text, if the validity change is due to
     *   a change in the validation regular expression string, then returning
     *   false will have no effect.
     */
    static const String EventTextValidityChanged;
    /** Event fired when the text caret position / insertion point is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Editbox whose current insertion point
     * has changed.
     */
    static const String EventCaretMoved;
    /** Event fired when the current text selection is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Editbox whose current text selection
     * was changed.
     */
    static const String EventTextSelectionChanged;
    /** Event fired when the number of characters in the edit box reaches the
     * currently set maximum.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Editbox that has become full.
     */
    static const String EventEditboxFull;
    /** Event fired when the user accepts the current text by pressing Return,
     * Enter, or Tab.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the Editbox in which the user has accepted
     * the current text.
     */
    static const String EventTextAccepted;

    /*!
    \brief
        return true if the Editbox has input focus.

    \return
        - true if the Editbox has keyboard input focus.
        - false if the Editbox does not have keyboard input focus.
    */
    bool hasInputFocus(void) const;

    /*!
    \brief
        return true if the Editbox is read-only.

    \return
        true if the Editbox is read only and can't be edited by the user, false
        if the Editbox is not read only and may be edited by the user.
    */
    bool isReadOnly(void) const
        {return d_readOnly;}

    /*!
    \brief
        return true if the text for the Editbox will be rendered masked.

    \return
        true if the Editbox text will be rendered masked using the currently set
        mask code point, false if the Editbox text will be rendered as ordinary
        text.
    */
    bool isTextMasked(void) const
        {return d_maskText;}

    /*!
    \brief
        return the validation MatchState for the current Editbox text, given the
        currently set validation string.

    \note
        Validation is performed by means of a regular expression.  If the text
        matches the regex, the text is said to have passed validation.  If the
        text does not match with the regex then the text fails validation.
        The default RegexMatcher uses the pcre library to perform regular
        expression operations, details about the pattern syntax can be found
        on unix-like systems by way of <tt>man pcrepattern</tt> (or online at
        http://www.pcre.org/pcre.txt (scroll / search "PCREPATTERN(3)").
        Alternatively, see the perl regex documentation at
        http://perldoc.perl.org/perlre.html

    \return
        One of the MatchState enumerated values indicating the current match state.
    */
    MatchState getTextMatchState() const;

    /*!
    \brief
        return the currently set validation string

    \note
        Validation is performed by means of a regular expression.  If the text
        matches the regex, the text is said to have passed validation.  If the
        text does not match with the regex then the text fails validation.
        The default RegexMatcher uses the pcre library to perform regular
        expression operations, details about the pattern syntax can be found
        on unix-like systems by way of <tt>man pcrepattern</tt> (or online at
        http://www.pcre.org/pcre.txt (scroll / search "PCREPATTERN(3)").
        Alternatively, see the perl regex documentation at
        http://perldoc.perl.org/perlre.html

    \return
        String object containing the current validation regex data
    */
    const String& getValidationString(void) const
        {return d_validationString;}

    /*!
    \brief
        return the current position of the caret.

    \return
        Index of the insert caret relative to the start of the text.
    */
    size_t getCaretIndex(void) const;

    /*!
    \brief
        return the current selection start point.

    \return
        Index of the selection start point relative to the start of the text.
        If no selection is defined this function returns the position of the
        caret.
    */
    size_t getSelectionStartIndex(void) const;

    /*!
    \brief
        return the current selection end point.

    \return
        Index of the selection end point relative to the start of the text.  If
        no selection is defined this function returns the position of the caret.
    */
    size_t getSelectionEndIndex(void) const;

    /*!
    \brief
        return the length of the current selection (in code points /
        characters).

    \return
        Number of code points (or characters) contained within the currently
        defined selection.
    */
    size_t getSelectionLength(void) const;

    /*!
    \brief
        return the code point used when rendering masked text.

    \return
        utf32 or char (depends on used String class) code point value representing
        the Unicode code point that will be rendered instead of the Editbox text
        when rendering in masked mode.
    */
    String::value_type getMaskCodePoint(void) const
    { return d_maskCodePoint; }

    /*!
    \brief
        return the maximum text length set for this Editbox.

    \return
        The maximum number of code points (characters) that can be entered into
        this Editbox.

    \note
        Depending on the validation string set, the actual length of text that
        can be entered may be less than the value returned here
        (it will never be more).
    */
    size_t getMaxTextLength(void) const
        {return d_maxTextLen;}

    /*!
    \brief
        Specify whether the Editbox is read-only.

    \param setting
        true if the Editbox is read only and can't be edited by the user, false
        if the Editbox is not read only and may be edited by the user.

    \return
        Nothing.
    */
    void setReadOnly(bool setting);

    /*!
    \brief
        Specify whether the text for the Editbox will be rendered masked.

    \param setting
        - true if the Editbox text should be rendered masked using the currently
          set mask code point.
        - false if the Editbox text should be rendered as ordinary text.

    \return
        Nothing.
    */
    void setTextMasked(bool setting);

    /*!
    \brief
        Set the text validation string.

    \note
        Validation is performed by means of a regular expression.  If the text
        matches the regex, the text is said to have passed validation.  If the
        text does not match with the regex then the text fails validation.
        The default RegexMatcher uses the pcre library to perform regular
        expression operations, details about the pattern syntax can be found
        on unix-like systems by way of <tt>man pcrepattern</tt> (or online at
        http://www.pcre.org/pcre.txt (scroll / search "PCREPATTERN(3)").
        Alternatively, see the perl regex documentation at
        http://perldoc.perl.org/perlre.html

    \param validation_string
        String object containing the validation regex data to be used.

    \return
        Nothing.
    */
    void setValidationString(const String& validation_string);

    /*!
    \brief
        Set the current position of the caret.

    \param caret_pos
        New index for the insert caret relative to the start of the text.  If
        the value specified is greater than the number of characters in the
        Editbox, the caret is positioned at the end of the text.

    \return
        Nothing.
    */
    void setCaretIndex(size_t caret_pos);

    /*!
    \brief
        Define the current selection for the Editbox

    \param start_pos
        Index of the starting point for the selection.  If this value is greater
        than the number of characters in the Editbox, the selection start will
        be set to the end of the text.

    \param end_pos
        Index of the ending point for the selection.  If this value is greater
        than the number of characters in the Editbox, the selection end will be
        set to the end of the text.

    \return
        Nothing.
    */
    void setSelection(size_t start_pos, size_t end_pos);

    /*!
    \brief
        Define the current selection start for the Editbox

        \param start_pos
        Index of the starting point for the selection.  If this value is greater than the number of characters in the Editbox, the
        selection start will be set to the end of the text.

    \return
        Nothing.
    */
    void setSelectionStart(size_t start_pos);

    /*!
    \brief
        Define the current selection for the Editbox

    \param start_pos
        Length of the selection.

    \return
        Nothing.
    */
    void setSelectionLength(size_t length);

    /*!
    \brief
        set the code point used when rendering masked text.

    \param code_point
        utf32 or char (depends on used String class) code point value representing
        the vode point that should be rendered instead of the Editbox text when
        rendering in masked mode.

    \return
        Nothing.
    */
    void setMaskCodePoint(String::value_type code_point);

    /*!
    \brief
        set the maximum text length for this Editbox.

    \param max_len
        The maximum number of code points (characters) that can be entered into
        this Editbox.

    \note
        Depending on the validation string set, the actual length of text that
        can be entered may be less than the value set here
        (it will never be more).

    \return
        Nothing.
    */
    void setMaxTextLength(size_t max_len);

    /*!
    \brief
        Set the RegexMatcher based validator for this Editbox.

    \param matcher
        Pointer to an object that implements the RegexMatcher interface, or 0
        to restore a system supplied RegexMatcher (if support is available).

    \note
        If the previous RegexMatcher validator is one supplied via the system,
        it is deleted and replaced with the given RegexMatcher.  User supplied
        RegexMatcher objects will never be deleted by the system and you must
        ensure that the object is not deleted while the Editbox holds a pointer
        to it.  Once the Editbox is destroyed or the validator is set to
        something else it is the responsibility of client code to ensure any
        previous custom validator is deleted.
    */
    void setValidator(RegexMatcher* matcher);

    //! \copydoc Window::performCopy
    virtual bool performCopy(Clipboard& clipboard);

    //! \copydoc Window::performCut
    virtual bool performCut(Clipboard& clipboard);
    
    //! \copydoc Window::performPaste
    virtual bool performPaste(Clipboard& clipboard);
    
    //! Constructor for Editbox class.
    Editbox(const String& type, const String& name);

    //! Destructor for Editbox class.
    virtual ~Editbox(void);

protected:
    /*!
    \brief
        Return the text code point index that is rendered closest to screen
        position \a pt.

    \param pt
        Point object describing a position on the screen in pixels.

    \return
        Code point index into the text that is rendered closest to screen
        position \a pt.
    */
    size_t getTextIndexFromPosition(const Vector2f& pt) const;

    //! Clear the currently defined selection (just the region, not the text).
    void clearSelection(void);

    /*!
    \brief
        Erase the currently selected text.

    \param modify_text
        when true, the actual text will be modified.  When false, everything is
        done except erasing the characters.
    */
    void eraseSelectedText(bool modify_text = true);

    /*!
    \brief
        return the match state of the given string for the validation regular
        expression.
    */
    MatchState getStringMatchState(const String& str) const;

    /** Helper to update validator match state as needed for the given string
     * and event handler return codes.
     *
     * This effectively asks permission from event handlers to proceed with the
     * change, updates d_validatorMatchState and returns an appropriate bool.
     * The return value basically says whether or not to set the input string
     * as the current text for the Editbox.
     */
    bool handleValidityChangeForString(const String& str);

    //! Processing for backspace key
    void handleBackspace(void);

    //! Processing for Delete key
    void handleDelete(void);

    //! Processing to move caret one character left
    void handleCharLeft(uint sysKeys);

    //! Processing to move caret one word left
    void handleWordLeft(uint sysKeys);

    //! Processing to move caret one character right
    void handleCharRight(uint sysKeys);

    //! Processing to move caret one word right
    void handleWordRight(uint sysKeys);

    //! Processing to move caret to the start of the text.
    void handleHome(uint sysKeys);

    //! Processing to move caret to the end of the text
    void handleEnd(uint sysKeys);

    //! validate window renderer
    virtual bool validateWindowRenderer(const WindowRenderer* renderer) const;

    /*!
    \brief
        Handler called when the read only state of the Editbox has been changed.
    */
    virtual void onReadOnlyChanged(WindowEventArgs& e);

    /*!
    \brief
        Handler called when the masked rendering mode (password mode) has been
        changed.
    */
    virtual void onMaskedRenderingModeChanged(WindowEventArgs& e);

    /*!
    \brief
        Handler called when the code point to use for masked rendering has been
        changed.
    */
    virtual void onMaskCodePointChanged(WindowEventArgs& e);

    /*!
    \brief
        Event fired internally when the validation string is changed.
    */
    virtual void onValidationStringChanged(WindowEventArgs& e);

    /*!
    \brief
        Handler called when the maximum text length for the edit box is changed.
    */
    virtual void onMaximumTextLengthChanged(WindowEventArgs& e);

    /*!
    \brief
        Handler called when something has caused the validity state of the
        current text to change.
    */
    virtual void onTextValidityChanged(RegexMatchStateEventArgs& e);

    /*!
    \brief
        Handler called when the caret (insert point) position changes.
    */
    virtual void onCaretMoved(WindowEventArgs& e);

    /*!
    \brief
        Handler called when the current text selection changes.
    */
    virtual void onTextSelectionChanged(WindowEventArgs& e);

    /*!
    \brief
        Handler called when the edit box text has reached the set maximum
        length.
    */
    virtual void onEditboxFullEvent(WindowEventArgs& e);

    /*!
    \brief
        Handler called when the user accepts the edit box text by pressing
        Return, Enter, or Tab.
    */
    virtual void onTextAcceptedEvent(WindowEventArgs& e);

    // Overridden event handlers
    void onMouseButtonDown(MouseEventArgs& e);
    void onMouseButtonUp(MouseEventArgs& e);
    void onMouseDoubleClicked(MouseEventArgs& e);
    void onMouseTripleClicked(MouseEventArgs& e);
    void onMouseMove(MouseEventArgs& e);
    void onCaptureLost(WindowEventArgs& e);
    void onCharacter(KeyEventArgs& e);
    void onKeyDown(KeyEventArgs& e);
    void onTextChanged(WindowEventArgs& e);

    //! True if the editbox is in read-only mode
    bool d_readOnly;
    //! True if the editbox text should be rendered masked.
    bool d_maskText;
    //! Code point to use when rendering masked text.
    String::value_type d_maskCodePoint;
    //! Maximum number of characters for this Editbox.
    size_t d_maxTextLen;
    //! Position of the caret / insert-point.
    size_t d_caretPos;
    //! Start of selection area.
    size_t d_selectionStart;
    //! End of selection area.
    size_t d_selectionEnd;
    //! Copy of validation reg-ex string.
    String d_validationString;
    //! Pointer to class used for validation of text.
    RegexMatcher* d_validator;
    //! specifies whether validator was created by us, or supplied by user.
    bool d_weOwnValidator;
    //! true when a selection is being dragged.
    bool d_dragging;
    //! Selection index for drag selection anchor point.
    size_t d_dragAnchorIdx;
    //! Current match state of EditboxText
    MatchState d_validatorMatchState;
    //! Previous match state change response
    bool d_previousValidityChangeResponse;

private:

    void addEditboxProperties(void);
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIEditbox_h_
