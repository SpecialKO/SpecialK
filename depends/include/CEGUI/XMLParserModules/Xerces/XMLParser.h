/***********************************************************************
    created:    Sat Mar 12 2005
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
#ifndef _CEGUIXercesParser_h_
#define _CEGUIXercesParser_h_

#include "../../XMLParser.h"
#include "CEGUI/XMLParserModules/Xerces/XMLParserProperties.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

#if (defined( __WIN32__ ) || defined( _WIN32 )) && !defined(CEGUI_STATIC)
#   ifdef CEGUIXERCESPARSER_EXPORTS
#       define CEGUIXERCESPARSER_API __declspec(dllexport)
#   else
#       define CEGUIXERCESPARSER_API __declspec(dllimport)
#   endif
#else
#   define CEGUIXERCESPARSER_API
#endif


// Xerces-C includes
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/TransService.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>

// Start of CEGUI namespace section
namespace CEGUI
{
    class XercesHandler : public XERCES_CPP_NAMESPACE::DefaultHandler
    {
    public:
        XercesHandler(XMLHandler& handler);
        ~XercesHandler(void);

        // Implementation of methods in Xerces DefaultHandler.
        void startElement(const XMLCh* const uri, const XMLCh* const localname, const XMLCh* const qname, const XERCES_CPP_NAMESPACE::Attributes& attrs);
        void endElement(const XMLCh* const uri, const XMLCh* const localname, const XMLCh* const qname);
#if _XERCES_VERSION >= 30000
        void characters(const XMLCh* const chars, const XMLSize_t length);
#else /* _XERCES_VERSION >= 30000 */
        void characters (const XMLCh *const chars, const unsigned int length);
#endif /* _XERCES_VERSION >= 30000 */
        void warning (const XERCES_CPP_NAMESPACE::SAXParseException &exc);
        void error (const XERCES_CPP_NAMESPACE::SAXParseException &exc);
        void fatalError (const XERCES_CPP_NAMESPACE::SAXParseException &exc);

    protected:
        XMLHandler& d_handler;      //!< This is the 'real' CEGUI based handler which we interface via.
    };

    /*!
    \brief
        Implementation of XMLParser using Xerces-C++
     */
    class CEGUIXERCESPARSER_API XercesParser : public XMLParser
    {
    public:
        XercesParser(void);
        ~XercesParser(void);

        // Implementation of public abstract interface
        void parseXML(XMLHandler& handler, const RawDataContainer& source, const String& schemaName);

        // Internal methods
        /*!
        \brief
            Populate the CEGUI::XMLAttributes object with attribute data from the Xerces attributes block.
         */
        static void populateAttributesBlock(const XERCES_CPP_NAMESPACE::Attributes& src, XMLAttributes& dest);

        /*!
        \brief
            Return a CEGUI::String containing the Xerces XMLChar string data in \a xmlch_str.

        \param xmlch_str
            The string data.

        \param length
            The size of the string data. It can be computed using \code XMLString::stringLen(xmlch_str) \endcode

         */
        static String transcodeXmlCharToString(const XMLCh* const xmlch_str, unsigned int length);

        /*!
        \brief
            Sets the default resource group to be used when loading schema files.

        \param resourceGroup
            String describing the default resource group identifier to be used.

        \return
            Nothing.
        */
        static void setSchemaDefaultResourceGroup(const String& resourceGroup)
            { d_defaultSchemaResourceGroup = resourceGroup; }

        /*!
        \brief
            Returns the default resource group used when loading schema files.

        \return
            String describing the default resource group identifier..
        */
        static const String& getSchemaDefaultResourceGroup()
            { return d_defaultSchemaResourceGroup; }

    protected:
        static void initialiseSchema(XERCES_CPP_NAMESPACE::SAX2XMLReader* reader, const String& schemaName);
        static XERCES_CPP_NAMESPACE::SAX2XMLReader* createReader(XERCES_CPP_NAMESPACE::DefaultHandler& handler);
        static void doParse(XERCES_CPP_NAMESPACE::SAX2XMLReader* parser, const RawDataContainer& source);

        // Implementation of abstract interface.
        bool initialiseImpl(void);
        void cleanupImpl(void);

        //! holds the default resource group ID for loading schemas.
        static String d_defaultSchemaResourceGroup;
        //! Property for accessing the default schema resource group ID.
        static XercesParserProperties::SchemaDefaultResourceGroup
            s_schemaDefaultResourceGroupProperty;
    };

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif  // end of guard _CEGUIXercesParser_h_
