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
#ifndef _CEGUIXMLParser_h_
#define _CEGUIXMLParser_h_

#include "CEGUI/Base.h"
#include "CEGUI/String.h"
#include "CEGUI/PropertySet.h"

// Start of CEGUI namespace section
namespace CEGUI
{
    /*!
    \brief
        This is an abstract class that is used by CEGUI to interface with XML parser libraries.
     */
    class CEGUIEXPORT XMLParser :
        public PropertySet,
        public AllocatedObject<XMLParser>
    {
    public:
        /*!
        \brief
            XMLParser base class constructor.
         */
        XMLParser(void);

        /*!
        \brief
            XMLParser base class destructor.
         */
        virtual ~XMLParser(void);

        /*!
        \brief
            Initialises the XMLParser module ready for use.

            Note that this calls the protected abstract method 'initialiseImpl', which should
            be provided in your implementation to perform any required initialisation.

        \return
            - true if the module initialised successfully.
            - false if the module initialisation failed.
         */
        bool initialise(void);

        /*!
        \brief
            Cleans up the XMLParser module after use.

            Note that this calls the protected abstract method 'cleanupImpl', which should
            be provided in your implementation to perform any required cleanup.

        \return
            Nothing.
         */
        void cleanup(void);

        /*!
        \brief
            abstract method which initiates parsing of an XML.

        \param handler
            XMLHandler based object which will process the XML elements.

        \param source
            RawDataContainer containing the data to parse

        \param schemaName
            String object holding the name of the XML schema file to use for validating the XML.
            Note that whether this is used or not is dependant upon the XMLParser in use.

        \return
            Nothing.
         */
        virtual void parseXML(XMLHandler& handler, const RawDataContainer& source, const String& schemaName) = 0;

        /*!
        \brief
            convenience method which initiates parsing of an XML file.

        \param handler
            XMLHandler based object which will process the XML elements.

        \param filename
            String object holding the filename of the XML file to be parsed.

        \param schemaName
            String object holding the name of the XML schema file to use for validating the XML.
            Note that whether this is used or not is dependant upon the XMLParser in use.

        \param resourceGroup
            String object holding the resource group identifier which will be passed to the
            ResourceProvider when loading the XML and schema files.

        \return
            Nothing.
         */
        virtual void parseXMLFile(XMLHandler& handler, const String& filename, const String& schemaName, const String& resourceGroup);

        /*!
        \brief
            convenience method which initiates parsing of an XML source from string.

        \param handler
            XMLHandler based object which will process the XML elements.

        \param source
            The XML source passed as a String

        \param schemaName
            String object holding the name of the XML schema file to use for validating the XML.
            Note that whether this is used or not is dependant upon the XMLParser in use.

        \return
            Nothing.
         */
        virtual void parseXMLString(XMLHandler& handler, const String& source, const String& schemaName);

        /*!
        \brief
            Return identification string for the XML parser module.  If the internal id string has not been
            set by the XML parser module creator, a generic string of "Unknown XML parser" will be returned.

        \return
            String object holding a string that identifies the XML parser in use.
        */
        const String& getIdentifierString() const;

    protected:
        /*!
        \brief
            abstract method which initialises the XMLParser ready for use.

        \return
            - true if the module initialised successfully.
            - false if the module initialisation failed.
         */
        virtual bool initialiseImpl(void) = 0;

        /*!
        \brief
            abstract method which cleans up the XMLParser after use.

        \return
            Nothing.
         */

        virtual void cleanupImpl(void) = 0;

        // data fields
        String d_identifierString;                 //!< String that holds some id information about the module.

    private:
        bool d_initialised;     //!< true if the parser module has been initialised,
    };

} // End of  CEGUI namespace section


#endif  // end of guard _CEGUIXMLParser_h_
