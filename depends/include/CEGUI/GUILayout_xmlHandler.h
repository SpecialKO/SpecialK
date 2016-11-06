/***********************************************************************
	created:	5/7/2004
	author:		Paul D Turner
	
	purpose:	Interface to XML parser for GUILayout files
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
#ifndef _CEGUIGUILayout_xmlHandler_h_
#define _CEGUIGUILayout_xmlHandler_h_

#include "CEGUI/WindowManager.h"
#include "CEGUI/Window.h"
#include "CEGUI/XMLHandler.h"

#include <vector>

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
	Handler class used to parse the GUILayout XML files using SAX2
*/
class GUILayout_xmlHandler : public XMLHandler
{
	typedef WindowManager::PropertyCallback PropertyCallback;
public:
    static const String NativeVersion;                  //!< The only version that we will allow to load
    
	/*************************************************************************
		Construction & Destruction
	*************************************************************************/
	/*!
	\brief
		Constructor for GUILayout_xmlHandler objects
	*/
	GUILayout_xmlHandler(PropertyCallback* callback = 0, void* userdata = 0) :
	  d_root(0),
	  d_propertyCallback(callback),
	  d_userData(userdata)
	{}

	/*!
	\brief
		Destructor for GUILayout_xmlHandler objects
	*/
	virtual ~GUILayout_xmlHandler(void) {}

    virtual const String& getSchemaName() const;
    virtual const String& getDefaultResourceGroup() const;

	/*************************************************************************
		SAX2 Handler overrides
	*************************************************************************/ 
	/*!
	\brief
		document processing (only care about elements, schema validates format)
	*/
    virtual void elementStart(const String& element, const XMLAttributes& attributes);
    virtual void elementEnd(const String& element);
    virtual void text(const String& text);

	/*************************************************************************
		Functions used by our implementation
	*************************************************************************/
	/*!
	\brief
		Destroy all windows created so far.
	*/
	void	cleanupLoadedWindows(void);


	/*!
	\brief
		Return a pointer to the 'root' window created.
	*/
	Window*	getLayoutRootWindow(void) const;
    
	static const String GUILayoutElement;				//!< Tag name for GUILayout elements.
	static const String LayoutImportElement;			//!< Tag name for LayoutImport elements.
	static const String EventElement;					//!< Tag name for Event elements.
	static const String LayoutImportFilenameAttribute;//!< Attribute name that stores the file name of the layout to import.
    static const String LayoutImportResourceGroupAttribute; //!< Attribute name that stores the resource group identifier used when loading imported file.
	static const String EventNameAttribute;			//!< Attribute name that stores the event name to be subscribed.
	static const String EventFunctionAttribute;		//!< Attribute name that stores the name of the scripted function to be bound.
    static const String GUILayoutVersionAttribute;  //!< Attribute name that stores the xml file version.

private:
    /*!
    \brief
        Method that handles the opening GUILayout XML element.

    \note
        This method just checks the version attribute and there is no equivalent
        elementGUILayoutEnd method, because it would be just NOOP anyways
    */
    void elementGUILayoutStart(const XMLAttributes& attributes);

    /*!
    \brief
        Method that handles the opening Window XML element.
    */
    void elementWindowStart(const XMLAttributes& attributes);

    /*!
    \brief
        Method that handles the opening AutoWindow XML element.
    */
    void elementAutoWindowStart(const XMLAttributes& attributes);

    /*!
    \brief
        Method that handles the UserString XML element.
    */
    void elementUserStringStart(const XMLAttributes& attributes);

    /*!
    \brief
        Method that handles the Property XML element.
    */
    void elementPropertyStart(const XMLAttributes& attributes);

    /*!
    \brief
        Method that handles the LayoutImport XML element.
    */
    void elementLayoutImportStart(const XMLAttributes& attributes);

    /*!
    \brief
        Method that handles the Event XML element.
    */
    void elementEventStart(const XMLAttributes& attributes);

    /*!
    \brief
        Method that handles the closing Window XML element.
    */
    void elementWindowEnd();

    /*!
    \brief
        Method that handles the closing AutoWindow XML element.
    */
    void elementAutoWindowEnd();

    /*!
    \brief
        Method that handles the closing of a UserString XML element.
    */
    void elementUserStringEnd();

    /*!
    \brief
        Method that handles the closing of a property XML element.
    */
    void elementPropertyEnd();

    void operator=(const GUILayout_xmlHandler&) {}

    /*************************************************************************
		Implementation Data
	*************************************************************************/
    typedef std::pair<Window*, bool> WindowStackEntry; //!< Pair used as datatype for the window stack. second is false if the window is an autowindow.
	typedef std::vector<WindowStackEntry
        CEGUI_VECTOR_ALLOC(WindowStackEntry)> WindowStack;
	Window*	d_root;				//!< Will point to first window created.
	WindowStack	d_stack;		//!< Stack used to keep track of what we're doing to which window.
	PropertyCallback*	d_propertyCallback; //!< Callback for every property loaded
	void*				d_userData;			//!< User data for the property callback
    String d_stringItemName; //!< Use for long property or user string value
    String d_stringItemValue; //!< Use for long property or user string value
};


} // End of  CEGUI namespace section


#endif	// end of guard _CEGUIGUILayout_xmlHandler_h_
