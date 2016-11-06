/***********************************************************************
    created:    Mon Mar 6 2006
    author:     Paul D Turner <paul@cegui.org.uk> (based on Dalfy's code)
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
#ifndef _CEGUIExpatParser_h_
#define _CEGUIExpatParser_h_

#include "../../XMLParser.h"

#if (defined( __WIN32__ ) || defined( _WIN32 )) && !defined(CEGUI_STATIC)
#   ifdef CEGUIEXPATPARSER_EXPORTS
#       define CEGUIEXPATPARSER_API __declspec(dllexport)
#   else
#       define CEGUIEXPATPARSER_API __declspec(dllimport)
#   endif
#else
#   define CEGUIEXPATPARSER_API
#endif

// Start of CEGUI namespace section
namespace CEGUI
{

/*!
\brief
    Implementation of XMLParser using Expat
*/
class CEGUIEXPATPARSER_API ExpatParser : public XMLParser
{
public:
    ExpatParser(void);
    ~ExpatParser(void);

    // Implementation of public abstract interface
    void parseXML(XMLHandler& handler, const RawDataContainer& source, const String& schemaName);

protected:
    // Implementation of protected abstract interface.
    bool initialiseImpl(void);
    // Implementation of protected abstract interface.
    void cleanupImpl(void);
    // C++ class methods name are not valide C function pointer. static solve this
    static void startElement(void* data, const char* element, const char**attr); // Expat handlers
    static void endElement(void* data, const char* element); // Expat handlers
    static void characterData(void* data, const char* text, int len); // Expat handlers
};

} // End of  CEGUI namespace section

#endif // end of guard _CEGUIEXpatParser_h_
