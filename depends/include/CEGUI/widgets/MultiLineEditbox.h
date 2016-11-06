/***********************************************************************
	created:	30/6/2004
	author:		Paul D Turner
	
	purpose:	Interface to the Multi-lien edit box base class.
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
#ifndef _CEGUIMultiLineEditbox_h_
#define _CEGUIMultiLineEditbox_h_

#include "../Base.h"
#include "../Window.h"
#include "../Font.h"

#include <vector>

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif


// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Base class for multi-line edit box window renderer objects.
*/
class CEGUIEXPORT MultiLineEditboxWindowRenderer : public WindowRenderer
{
public:
    /*!
    \brief
        Constructor
    */
    MultiLineEditboxWindowRenderer(const String& name);

    /*!
    \brief
        Return a Rect object describing, in un-clipped pixels, the window relative area
        that the text should be rendered in to.

    \return
        Rect object describing the area of the Window to be used for rendering text.
    */
    virtual Rectf getTextRenderArea(void) const = 0;

protected:
    // base class overrides
    void onLookNFeelAssigned();
};

/*!
\brief
	Base class for the multi-line edit box widget.
*/
class CEGUIEXPORT MultiLineEditbox : public Window
{
public:
	static const String EventNamespace;				//!< Namespace for global events
    static const String WidgetTypeName;             //!< Window factory name

	/*************************************************************************
		Constants
	*************************************************************************/
	// event names
    /** Event fired when the read-only mode for the edit box has been changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the MultiLineEditbox whose read-only mode
     * was changed.
     */
	static const String EventReadOnlyModeChanged;
    /** Event fired when the word wrap mode of the edit box has been changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the MultiLineEditbox whose word wrap
     * mode was changed.
     */
	static const String EventWordWrapModeChanged;
    /** Event fired when the maximum allowable string length for the edit box
     * has been changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the MultiLineEditbox whose maximum string
     * length was changed.
     */
	static const String EventMaximumTextLengthChanged;
    /** Event fired when the text caret / current insertion position is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the MultiLineEditbox whose caret position
     * has changed.
     */
	static const String EventCaretMoved;
    /** Event fired when the current text selection for the edit box is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the MultiLineEditbox whose text selection
     * was changed.
     */
	static const String EventTextSelectionChanged;
    /** Event fired when the number of characters in the edit box reaches the
     * current maximum length.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the MultiLineEditbox whose text length
     * has reached the set maximum allowable length for the edit box.
     */
	static const String EventEditboxFull;
    /** Event fired when the mode setting that forces the display of the
     * vertical scroll bar for the edit box is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the MultiLineEditbox whose vertical
     * scrollbar mode has been changed.
     */
	static const String EventVertScrollbarModeChanged;
    /** Event fired when the mode setting that forces the display of the
     * horizontal scroll bar for the edit box is changed.
     * Handlers are passed a const WindowEventArgs reference with
     * WindowEventArgs::window set to the MultiLineEditbox whose horizontal
     * scrollbar mode has been changed.
     */
	static const String EventHorzScrollbarModeChanged;

    /*************************************************************************
        Child Widget name constants
    *************************************************************************/
    static const String VertScrollbarName;   //!< Widget name for the vertical scrollbar component.
    static const String HorzScrollbarName;   //!< Widget name for the horizontal scrollbar component.

    /*************************************************************************
        Implementation struct
    *************************************************************************/
    /*!
    \brief
        struct used to store information about a formatted line within the
        paragraph.
    */
    struct LineInfo
    {
        size_t  d_startIdx;     //!< Starting index for this line.
        size_t  d_length;       //!< Code point length of this line.
        float   d_extent;       //!< Rendered extent of this line.
    };
    typedef std::vector<LineInfo
        CEGUI_VECTOR_ALLOC(LineInfo)>   LineList;   //!< Type for collection of LineInfos.

	/*************************************************************************
		Accessor Functions
	*************************************************************************/
	/*!
	\brief
		return true if the edit box has input focus.

	\return
		- true if the edit box has keyboard input focus.
		- false if the edit box does not have keyboard input focus.
	*/
	bool	hasInputFocus(void) const;


	/*!
	\brief
		return true if the edit box is read-only.

	\return
		- true if the edit box is read only and can't be edited by the user.
		- false if the edit box is not read only and may be edited by the user.
	*/
	bool	isReadOnly(void) const		{return d_readOnly;}


	/*!
	\brief
		return the current position of the caret.

	\return
		Index of the insert caret relative to the start of the text.
	*/
	size_t	getCaretIndex(void) const		{return d_caretPos;}


	/*!
	\brief
		return the current selection start point.

	\return
		Index of the selection start point relative to the start of the text.  If no selection is defined this function returns
		the position of the caret.
	*/
	size_t	getSelectionStartIndex(void) const;


	/*!
	\brief
		return the current selection end point.

	\return
		Index of the selection end point relative to the start of the text.  If no selection is defined this function returns
		the position of the caret.
	*/
	size_t	getSelectionEndIndex(void) const;

	
	/*!
	\brief
		return the length of the current selection (in code points / characters).

	\return
		Number of code points (or characters) contained within the currently defined selection.
	*/
	size_t	getSelectionLength(void) const;


	/*!
	\brief
		return the maximum text length set for this edit box.

	\return
		The maximum number of code points (characters) that can be entered into this edit box.
	*/
	size_t	getMaxTextLength(void) const		{return d_maxTextLen;}


	/*!
	\brief
		Return whether the text in the edit box will be word-wrapped.

	\return
		- true if the text will be word-wrapped at the edges of the widget frame.
		- false if text will not be word-wrapped (a scroll bar will be used to access long text lines).
	*/
	bool	isWordWrapped(void) const;


    /*!
    \brief
        Return a pointer to the vertical scrollbar component widget for this
        MultiLineEditbox.

    \return
        Pointer to a Scrollbar object.

    \exception UnknownObjectException
        Thrown if the vertical Scrollbar component does not exist.
    */
    Scrollbar* getVertScrollbar() const;

    /*!
	\brief
		Return whether the vertical scroll bar is always shown.

	\return
		- true if the scroll bar will always be shown even if it is not required.
		- false if the scroll bar will only be shown when it is required.
	*/
	bool	isVertScrollbarAlwaysShown(void) const;

    /*!
    \brief
        Return a pointer to the horizontal scrollbar component widget for this
        MultiLineEditbox.

    \return
        Pointer to a Scrollbar object.

    \exception UnknownObjectException
        Thrown if the horizontal Scrollbar component does not exist.
    */
    Scrollbar* getHorzScrollbar() const;


    /*!
    \brief
        Return a Rect object describing, in un-clipped pixels, the window relative area
        that the text should be rendered in to.

    \return
        Rect object describing the area of the Window to be used for rendering text.
    */
    Rectf getTextRenderArea(void) const;

    // get d_lines
    const LineList& getFormattedLines(void) const   {return d_lines;}

    /*!
    \brief
        Return the line number a given index falls on with the current formatting.  Will return last line
        if index is out of range.
    */
    size_t  getLineNumberFromIndex(size_t index) const;

	/*************************************************************************
		Manipulators
	*************************************************************************/
	/*!
	\brief
		Initialise the Window based object ready for use.

	\note
		This must be called for every window created.  Normally this is handled automatically by the WindowFactory for each Window type.

	\return
		Nothing
	*/
	virtual void	initialiseComponents(void);


	/*!
	\brief
		Specify whether the edit box is read-only.

	\param setting
		- true if the edit box is read only and can't be edited by the user.
		- false if the edit box is not read only and may be edited by the user.

	\return
		Nothing.
	*/
	void	setReadOnly(bool setting);


	/*!
	\brief
		Set the current position of the caret.

	\param caret_pos
		New index for the insert caret relative to the start of the text.  If the value specified is greater than the
		number of characters in the edit box, the caret is positioned at the end of the text.

	\return
		Nothing.
	*/
	void	setCaretIndex(size_t caret_pos);


	/*!
	\brief
		Define the current selection for the edit box

	\param start_pos
		Index of the starting point for the selection.  If this value is greater than the number of characters in the edit box, the
		selection start will be set to the end of the text.

	\param end_pos
		Index of the ending point for the selection.  If this value is greater than the number of characters in the edit box, the
		selection start will be set to the end of the text.

	\return
		Nothing.
	*/
	void	setSelection(size_t start_pos, size_t end_pos);
	

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
		set the maximum text length for this edit box.

	\param max_len
		The maximum number of code points (characters) that can be entered into this Editbox.

	\return
		Nothing.
	*/
	void	setMaxTextLength(size_t max_len);


	/*!
	\brief
		Scroll the view so that the current caret position is visible.
	*/
	void	ensureCaretIsVisible(void);


	/*!
	\brief
		Set whether the text will be word wrapped or not.

	\param setting
		- true if the text should word-wrap at the edges of the text box.
		- false if the text should not wrap, but a scroll bar should be used.

	\return
		Nothing.
	*/
	void	setWordWrapping(bool setting);

    /*!
	\brief
		Set whether the vertical scroll bar should always be shown.

	\param setting
		true if the vertical scroll bar should be shown even when it is not required.  false if the vertical
		scroll bar should only be shown when it is required.

	\return
		Nothing.
	*/
	void	setShowVertScrollbar(bool setting);

    // selection brush image property support
    void setSelectionBrushImage(const Image* image);
    const Image* getSelectionBrushImage() const;
    
    //! \copydoc Window::performCopy
    virtual bool performCopy(Clipboard& clipboard);
    
    //! \copydoc Window::performCut
    virtual bool performCut(Clipboard& clipboard);
    
    //! \copydoc Window::performPaste
    virtual bool performPaste(Clipboard& clipboard);

    /*!
    \brief
        Format the text into lines as dictated by the formatting options.

    \param update_scrollbars 
        - true if scrollbar configuration should be performed.
        - false if scrollbar configuration should not be performed.
    */
    void formatText(const bool update_scrollbars);

	/*************************************************************************
		Construction and Destruction
	*************************************************************************/
	/*!
	\brief
		Constructor for the MultiLineEditbox base class.
	*/
	MultiLineEditbox(const String& type, const String& name);


	/*!
	\brief
		Destructor for the MultiLineEditbox base class.
	*/
	virtual ~MultiLineEditbox(void);


protected:
	/*************************************************************************
		Implementation Methods (abstract)
	*************************************************************************/
	/*!
	\brief
		Return a Rect object describing, in un-clipped pixels, the window relative area
		that the text should be rendered in to.

	\return
		Rect object describing the area of the Window to be used for rendering text.
	*/
	//virtual	Rect	getTextRenderArea_impl(void) const		= 0;


	/*************************************************************************
		Implementation Methods
	*************************************************************************/
	/*!
	\brief
		Format the text into lines as needed by the current formatting options.
    \deprecated
        This is deprecated in favour of the version taking a boolean.
	*/
	void	formatText(void);

	/*!
	\brief
		Return the length of the next token in String \a text starting at index \a start_idx.

	\note
		Any single whitespace character is one token, any group of other characters is a token.
	
	\return
		The code point length of the token.
	*/
	size_t	getNextTokenLength(const String& text, size_t start_idx) const;


    /*!
	\brief
		display required integrated scroll bars according to current state of the edit box and update their values.
	*/
	void	configureScrollbars(void);


	/*!
	\brief
		Return the text code point index that is rendered closest to screen position \a pt.

	\param pt
		Point object describing a position on the screen in pixels.

	\return
		Code point index into the text that is rendered closest to screen position \a pt.
	*/
	size_t	getTextIndexFromPosition(const Vector2f& pt) const;


	/*!
	\brief
		Clear the current selection setting
	*/
	void	clearSelection(void);


	/*!
	\brief
		Erase the currently selected text.

	\param modify_text
		when true, the actual text will be modified.  When false, everything is done except erasing the characters.
	*/
	void	eraseSelectedText(bool modify_text = true);


	/*!
	\brief
		Processing for backspace key
	*/
	void	handleBackspace(void);


	/*!
	\brief
		Processing for Delete key
	*/
	void	handleDelete(void);


	/*!
	\brief
		Processing to move caret one character left
	*/
	void	handleCharLeft(uint sysKeys);


	/*!
	\brief
		Processing to move caret one word left
	*/
	void	handleWordLeft(uint sysKeys);


	/*!
	\brief
		Processing to move caret one character right
	*/
	void	handleCharRight(uint sysKeys);


	/*!
	\brief
		Processing to move caret one word right
	*/
	void	handleWordRight(uint sysKeys);


	/*!
	\brief
		Processing to move caret to the start of the text.
	*/
	void	handleDocHome(uint sysKeys);


	/*!
	\brief
		Processing to move caret to the end of the text
	*/
	void	handleDocEnd(uint sysKeys);


	/*!
	\brief
		Processing to move caret to the start of the current line.
	*/
	void	handleLineHome(uint sysKeys);


	/*!
	\brief
		Processing to move caret to the end of the current line
	*/
	void	handleLineEnd(uint sysKeys);


	/*!
	\brief
		Processing to move caret up a line.
	*/
	void	handleLineUp(uint sysKeys);


	/*!
	\brief
		Processing to move caret down a line.
	*/
	void	handleLineDown(uint sysKeys);


	/*!
	\brief
		Processing to insert a new line / paragraph.
	*/
	void	handleNewLine(uint sysKeys);


    /*!
    \brief
        Processing to move caret one page up
    */
    void    handlePageUp(uint sysKeys);


    /*!
    \brief
        Processing to move caret one page down
    */
    void    handlePageDown(uint sysKeys);

	/*!
	\brief
	   Internal handler that is triggered when the user interacts with the scrollbars.
    */
    bool handle_scrollChange(const EventArgs& args);

    // handler triggered when vertical scrollbar is shown or hidden
    bool handle_vertScrollbarVisibilityChanged(const EventArgs&);

    // validate window renderer
    virtual bool validateWindowRenderer(const WindowRenderer* renderer) const;

	/*************************************************************************
		New event handlers
	*************************************************************************/
	/*!
	\brief
		Handler called when the read-only state of the edit box changes
	*/
	void	onReadOnlyChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called when the word wrap mode for the the edit box changes
	*/
	void	onWordWrapModeChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called when the maximum text length for the edit box changes
	*/
	void	onMaximumTextLengthChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called when the caret moves.
	*/
	void	onCaretMoved(WindowEventArgs& e);


	/*!
	\brief
		Handler called when the text selection changes
	*/
	void	onTextSelectionChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called when the edit box is full
	*/
	void	onEditboxFullEvent(WindowEventArgs& e);


	/*!
	\brief
		Handler called when the 'always show' setting for the vertical scroll bar changes.
	*/
	void	onVertScrollbarModeChanged(WindowEventArgs& e);


	/*!
	\brief
		Handler called when 'always show' setting for the horizontal scroll bar changes.
	*/
	void	onHorzScrollbarModeChanged(WindowEventArgs& e);


	/*************************************************************************
		Overridden event handlers
	*************************************************************************/
	virtual	void	onMouseButtonDown(MouseEventArgs& e);
	virtual void	onMouseButtonUp(MouseEventArgs& e);
	virtual	void	onMouseDoubleClicked(MouseEventArgs& e);
	virtual	void	onMouseTripleClicked(MouseEventArgs& e);
	virtual void	onMouseMove(MouseEventArgs& e);
	virtual void	onCaptureLost(WindowEventArgs& e);
	virtual void	onCharacter(KeyEventArgs& e);
	virtual void	onKeyDown(KeyEventArgs& e);
	virtual void	onTextChanged(WindowEventArgs& e);
	virtual void	onSized(ElementEventArgs& e);
	virtual	void	onMouseWheel(MouseEventArgs& e);
    virtual void onFontChanged(WindowEventArgs& e);


	/*************************************************************************
		Implementation data
	*************************************************************************/
	bool	d_readOnly;			//!< true if the edit box is in read-only mode
	size_t	d_maxTextLen;		//!< Maximum number of characters for this Editbox.
	size_t	d_caretPos;			//!< Position of the caret / insert-point.
	size_t	d_selectionStart;	//!< Start of selection area.
	size_t	d_selectionEnd;		//!< End of selection area.
	bool	d_dragging;			//!< true when a selection is being dragged.
	size_t	d_dragAnchorIdx;	//!< Selection index for drag selection anchor point.

	static String d_lineBreakChars;	//!< Holds what we consider to be line break characters.
	bool		d_wordWrap;			//!< true when formatting uses word-wrapping.
	LineList	d_lines;			//!< Holds the lines for the current formatting.
	float		d_widestExtent;		//!< Holds the extent of the widest line as calculated in the last formatting pass.

	// component widget settings
	bool	d_forceVertScroll;		//!< true if vertical scrollbar should always be displayed
	bool	d_forceHorzScroll;		//!< true if horizontal scrollbar should always be displayed

	// images
	const Image*	d_selectionBrush;	//!< Image to use as the selection brush (should be set by derived class).


private:
	/*************************************************************************
		Private methods
	*************************************************************************/
	void	addMultiLineEditboxProperties(void);
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUIMultiLineEditbox_h_
