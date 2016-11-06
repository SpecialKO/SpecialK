/***********************************************************************
    created:    27/03/2009
    author:     Paul Turner
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
#ifndef _CEGUIXercesParserProperties_h_
#define _CEGUIXercesParserProperties_h_

#include "../../Property.h"

// Start of CEGUI namespace section
namespace CEGUI
{
//! Properties for the XercesParser XML parser module.
namespace XercesParserProperties
{    
/*!
\brief
    Property to access the resource group used to load .xsd schema files.
 
\par Usage:
    - Name: SchemaDefaultResourceGroup
    - Format: "[resource group name]".
 
 */
class SchemaDefaultResourceGroup : public Property
{
public:
    SchemaDefaultResourceGroup() : Property(
        "SchemaDefaultResourceGroup",
        "Property to get/set the resource group used when loading xsd schema "
        "files.  Value is a string describing the resource group name.",
        "")
    {}
    
    // implement property interface
    String get(const PropertyReceiver* receiver) const;
    void set(PropertyReceiver* receiver, const String& value);
    Property* clone() const;
};

} // End of XercesParserProperties namespace section
    
} // End of  CEGUI namespace section

#endif // end of guard _CEGUIXercesParserProperties_h_
