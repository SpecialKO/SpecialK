/***********************************************************************
    created:    Sun Jul 19 2009
    author:     Paul D Turner <paul@cegui.org.uk>
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
#ifndef _CEGUIFont_xmlHandler_h_
#define _CEGUIFont_xmlHandler_h_

#include "CEGUI/XMLHandler.h"
#include "CEGUI/String.h"

// Start of CEGUI namespace section
namespace CEGUI
{
//! Handler class used to parse the Font XML files to create Font objects.
class CEGUIEXPORT Font_xmlHandler : public XMLHandler
{
public:
    //! Filename of the XML schema used for validating Font files.
    static const String FontSchemaName;
    //! Tag name for Font elements.
    static const String FontElement;
    //! Tag name for Mapping elements.
    static const String MappingElement;
    //! Attribute name that stores the specific font type.
    static const String FontTypeAttribute;
    //! Attribute name that stores the font name.
    static const String FontNameAttribute;
    //! Attribute name that stores the filename of the font source (font / imageset)
    static const String FontFilenameAttribute;
    //! Attribute name that stores the resource group of the font source.
    static const String FontResourceGroupAttribute;
    //! Attribute name that stores the auto-scaled setting.
    static const String FontAutoScaledAttribute;
    //! Attribute name that stores the horizontal native resolution.
    static const String FontNativeHorzResAttribute;
    //! Attribute name that stores the vertical native resolution.
    static const String FontNativeVertResAttribute;
    //! Attribute name that stores the line height that we'll report for this font.
    static const String FontLineSpacingAttribute;
    //! Attribute name that stores the font point size.
    static const String FontSizeAttribute;
    //! Attribute name that stores the font anti-aliasing setting.
    static const String FontAntiAliasedAttribute;
    //! Attribute name that stores the codepoint value for a mapping
    static const String MappingCodepointAttribute;
    //! Attribute name that stores the image name for a mapping
    static const String MappingImageAttribute;
    //! Attribute name that stores the horizontal advance value for a mapping.
    static const String MappingHorzAdvanceAttribute;
    //! Attribute specifying the datafile version.
    static const String FontVersionAttribute;
    //! Type name of FreeType fonts.
    static const String FontTypeFreeType;
    //! Type name of Pixmap fonts.
    static const String FontTypePixmap;

    //! Constructor.
    Font_xmlHandler();

    //! Destructor.
    ~Font_xmlHandler();

    //! Return string holding the name of the created Font.
    const String& getObjectName() const;

    //! Return reference to the created Font object.
    Font& getObject() const;

    // XMLHandler overrides
    const String& getSchemaName() const;
    const String& getDefaultResourceGroup() const;

    void elementStart(const String& element, const XMLAttributes& attributes);
    void elementEnd(const String& element);

private:
    //! handles the opening Font XML element.
    void elementFontStart(const XMLAttributes& attributes);
    //! handles the closing Font XML element.
    void elementFontEnd();
    //! handles the opening Mapping XML element.
    void elementMappingStart(const XMLAttributes& attributes);
    //! creates a FreeTypeFont
    void createFreeTypeFont(const XMLAttributes& attributes);
    //! creates a PixmapFont
    void createPixmapFont(const XMLAttributes& attributes);

    //! throw exception if file version is not supported.
    void validateFontFileVersion(const XMLAttributes& attrs);

    //! Font object that we are constructing.
    Font* d_font;
    //! inidcates whether client read the created object
    mutable bool d_objectRead;
};

} // End of  CEGUI namespace section

#endif
