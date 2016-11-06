/***********************************************************************
    created:    Mon Jul 20 2009
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
#ifndef _CEGUIScheme_xmlHandler_h_
#define _CEGUIScheme_xmlHandler_h_

#include "CEGUI/XMLHandler.h"
#include "CEGUI/String.h"

// Start of CEGUI namespace section
namespace CEGUI
{
//! Handler class used to parse the Scheme XML files using SAX2
class CEGUIEXPORT Scheme_xmlHandler : public XMLHandler
{
public:
    //! Constructor.
    Scheme_xmlHandler();

    //! Destructor.
    ~Scheme_xmlHandler();

    //! Return string holding the name of the created Imageset.
    const String& getObjectName() const;

    //! Return reference to the created Scheme object.
    Scheme& getObject() const;

    // XMLHandler overrides
    const String& getSchemaName() const;
    const String& getDefaultResourceGroup() const;
    void elementStart(const String& element, const XMLAttributes& attributes);
    void elementEnd(const String& element);

private:
    //! Filename of the XML schema used for validating GUIScheme files.
    static const String GUISchemeSchemaName;
    //! Root GUIScheme element.
    static const String GUISchemeElement;
    //! Element specifying an Imageset.
    static const String ImagesetElement;
    //! Element specifying an Imageset to be created directly via an image file.
    static const String ImagesetFromImageElement;
    //! Element specifying a Font.
    static const String FontElement;
    //! Element specifying a module and set of WindowFactory elements.
    static const String WindowSetElement;
    //! Element specifying a WindowFactory type.
    static const String WindowFactoryElement;
    //! Element specifying a WindowFactory type alias.
    static const String WindowAliasElement;
    //! Element specifying a Falagard window mapping.
    static const String FalagardMappingElement;
    //! Element specifying a LookNFeel.
    static const String LookNFeelElement;
    //! Attribute specifying the name of some object.
    static const String NameAttribute;
    //! Attribute specifying the name of some file.
    static const String FilenameAttribute;
    //! Attribute specifying an alias name.
    static const String AliasAttribute;
    //! Attribute specifying target for an alias.
    static const String TargetAttribute;
    //! Attribute specifying resource group for some loadable resource.
    static const String ResourceGroupAttribute;
    //! Attribute specifying the type of a window being created via a mapping.
    static const String WindowTypeAttribute;
    //! Attribute specifying the base type of a falagard mapped window type.
    static const String TargetTypeAttribute;
    //! Attribute specifying the name of a LookNFeel for a falagard mapping.
    static const String LookNFeelAttribute;
    //! Element specifying a module and set of WindowRendererFactory elements.
    static const String WindowRendererSetElement;
    //! Element specifying a WindowRendererFactory type.
    static const String WindowRendererFactoryElement;
    //! Attribute specifying the type name of a window renderer.
    static const String WindowRendererAttribute;
    //! Attribute specifying the name of a registered RenderEffect.
    static const String RenderEffectAttribute;
    //! Attribute specifying the datafile version.
    static const String SchemeVersionAttribute;

    //! Function that handles the opening GUIScheme XML element.
    void elementGUISchemeStart(const XMLAttributes& attributes);
    //! Function that handles the Imageset XML element.
    void elementImagesetStart(const XMLAttributes& attributes);
    //! Function that handles the ImagesetFromImage XML element.
    void elementImagesetFromImageStart(const XMLAttributes& attributes);
    //! Function that handles the Font XML element.
    void elementFontStart(const XMLAttributes& attributes);
    //! Function that handles the WindowSet XML element.
    void elementWindowSetStart(const XMLAttributes& attributes);
    //! Function that handles the WindowFactory XML element.
    void elementWindowFactoryStart(const XMLAttributes& attributes);
    //! Function that handles the WindowRendererSet XML element.
    void elementWindowRendererSetStart(const XMLAttributes& attributes);
    //! Function that handles the WindowRendererFactory XML element.
    void elementWindowRendererFactoryStart(const XMLAttributes& attributes);
    //! Function that handles the WindowAlias XML element.
    void elementWindowAliasStart(const XMLAttributes& attributes);
    //! Function that handles the FalagardMapping XML element.
    void elementFalagardMappingStart(const XMLAttributes& attributes);
    //! Function that handles the LookNFeel XML element.
    void elementLookNFeelStart(const XMLAttributes& attributes);
    //! Function that handles the closing GUIScheme XML element.
    void elementGUISchemeEnd();

    //! throw exception if file version is not supported.
    void validateSchemeFileVersion(const XMLAttributes& attrs);

    //! Scheme object that we are constructing
    Scheme* d_scheme;
    //! inidcates whether client read the created object
    mutable bool d_objectRead;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIScheme_xmlHandler_h_
