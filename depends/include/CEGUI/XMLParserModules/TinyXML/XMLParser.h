/***********************************************************************
    created:    Sun Mar 13 2005
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
#ifndef _CEGUITinyXMLParser_h_
#define _CEGUITinyXMLParser_h_

#include "../../XMLParser.h"

#if (defined( __WIN32__ ) || defined( _WIN32 )) && !defined(CEGUI_STATIC)
#   ifdef CEGUITINYXMLPARSER_EXPORTS
#       define CEGUITINYXMLPARSER_API __declspec(dllexport)
#   else
#       define CEGUITINYXMLPARSER_API __declspec(dllimport)
#   endif
#else
#   define CEGUITINYXMLPARSER_API
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
    /*!
    \brief
    Implementation of XMLParser using TinyXML
     */
    class CEGUITINYXMLPARSER_API TinyXMLParser : public XMLParser
    {
    public:
        TinyXMLParser(void);
        ~TinyXMLParser(void);

        // Implementation of public abstract interface
        void parseXML(XMLHandler& handler, const RawDataContainer& filename, const String& schemaName);

    protected:
        // Implementation of abstract interface.
        bool initialiseImpl(void);
        void cleanupImpl(void);
    };

} // End of  CEGUI namespace section


#endif  // end of guard _CEGUITinyXMLParser_h_
