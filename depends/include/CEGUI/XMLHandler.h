/***********************************************************************
    created:    12/3/2005
    author:     Paul D Turner
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
#ifndef _CEGUIXMLHandler_h_
#define _CEGUIXMLHandler_h_

#include "CEGUI/Base.h"
#include "CEGUI/String.h"

// Start of CEGUI namespace section
namespace CEGUI
{
    class CEGUIEXPORT XMLHandler
    {
    public:
        /*!
        \brief
            XMLHandler base class constructor.
         */
        XMLHandler(void);

        /*!
        \brief
            XMLHandler base class destructor.
         */
        virtual ~XMLHandler(void);
        
        /*!
        \brief
            Retrieves the schema file name to use with resources handled by this handler
         */
        virtual const String& getSchemaName() const;

        /*!
        \brief
            Retrieves the default resource group to be used when handling files
         */
        virtual const String& getDefaultResourceGroup() const = 0;

        /*!
        \brief
            Takes given RawDataContainer containing XML and handles it

        This is basically a convenience function used by NamedXMLResourceManager

        \internal
            No need for this to be virtual
         */
        void handleContainer(const RawDataContainer& source);

        /*!
        \brief
            Takes given file containing XML and handles it

        This is basically a convenience function used by NamedXMLResourceManager

        \internal
            No need for this to be virtual
         */
        void handleFile(const String& fileName, const String& resourceGroup);

        /*!
        \brief
            Takes given string containing XML source and handles it

        This is basically a convenience function used by NamedXMLResourceManager

        \internal
            No need for this to be virtual
         */
        void handleString(const String& source);

        /*!
        \brief
            Method called to notify the handler at the start of each XML element encountered.

        \param element
            String object holding the name of the element that is starting.

        \param attributes
            An XMLAttributes object holding the collection of attributes specified for the element.

        \return
            Nothing.
         */
        virtual void elementStart(const String& element, const XMLAttributes& attributes);
        
        /*!
        \brief
            Method called to notify the handler at the end of each XML element encountered.

        \param element
            String object holding the name of the element that is ending.

        \return
            Nothing.
         */
        virtual void elementEnd(const String& element);

	/*! 
	\brief
	    Method called to notify text node, several successiv text node are agregated. 
	    
	\param text 
	    String object holding the content of the text node.

	\return 
            Nothing.
	*/
	virtual void text(const String& text);
    };

} // End of  CEGUI namespace section


#endif  // end of guard _CEGUIXMLHandler_h_
