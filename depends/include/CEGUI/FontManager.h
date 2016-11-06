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
#ifndef _CEGUIFontManager_h_
#define _CEGUIFontManager_h_

#include "CEGUI/Singleton.h"
#include "CEGUI/NamedXMLResourceManager.h"
#include "CEGUI/Font.h"
#include "CEGUI/Font_xmlHandler.h"
#include "CEGUI/IteratorBase.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4275)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
class FreeTypeFont;
class PixmapFont;

/*!
\brief
    Class providing a shared library of Font objects to the system.

    The FontManager is used to create, access, and destroy Font objects.  The
    idea is that the FontManager will function as a central repository for Font
    objects used within the GUI system, and that those Font objects can be
    accessed, via a unique name, by any interested party within the system.
*/
class CEGUIEXPORT FontManager :
        public Singleton<FontManager>,
        public NamedXMLResourceManager<Font, Font_xmlHandler>,
        public AllocatedObject<FontManager>
{
public:
    //! Constructor.
    FontManager();

    //! Destructor.
    ~FontManager();

    /*!
    \brief
        Creates a FreeType type font.

    \param font_name
        The name that the font will use within the CEGUI system.

    \param point_size
        Specifies the point size that the font is to be rendered at.

    \param anti_aliased
        Specifies whether the font should be rendered using anti aliasing.

    \param font_filename
        The filename of an font file that will be used as the source for
        glyph images for this font.

    \param resource_group
        The resource group identifier to use when loading the font file
        specified by \a font_filename.

    \param auto_scaled
        Specifies whether the font imagery should be automatically scaled to
        maintain the same physical size (which is calculated by using the
        native resolution setting).

    \param native_horz_res
        The horizontal native resolution value.  This is only significant when
        auto scaling is enabled.

    \param native_vert_res
        The vertical native resolution value.  This is only significant when
        auto scaling is enabled.

    \param action
        One of the XMLResourceExistsAction enumerated values indicating what
        action should be taken when a Font with the specified name
        already exists.

    \return
        Reference to the newly create Font object.
    */
    Font& createFreeTypeFont(const String& font_name, const float point_size,
                             const bool anti_aliased,
                             const String& font_filename,
                             const String& resource_group = "",
                             const AutoScaledMode auto_scaled = ASM_Disabled,
                             const Sizef& native_res = Sizef(640.0f, 480.0f),
                             XMLResourceExistsAction action = XREA_RETURN);

    /*!
    \brief
        Creates a Pixmap type font.

    \param font_name
        The name that the font will use within the CEGUI system.

    \param imageset_filename
        The filename of an imageset to load that will be used as the source for
        glyph images for this font.  If \a resource_group is the special value
        of "*", this parameter may instead refer to the name of an already
        loaded Imagset.

    \param resource_group
        The resource group identifier to use when loading the imageset file
        specified by \a imageset_filename.  If this group is set to the special
        value of "*", then \a imageset_filename instead will refer to the name
        of an existing Imageset.

    \param auto_scaled
        Specifies whether the font imagery should be automatically scaled to
        maintain the same physical size (which is calculated by using the
        native resolution setting).

    \param native_horz_res
        The horizontal native resolution value.  This is only significant when
        auto scaling is enabled.

    \param native_vert_res
        The vertical native resolution value.  This is only significant when
        auto scaling is enabled.

    \param action
        One of the XMLResourceExistsAction enumerated values indicating what
        action should be taken when a Font with the specified name
        already exists.

    \return
        Reference to the newly create Font object.
    */
    Font& createPixmapFont(const String& font_name,
                           const String& imageset_filename,
                           const String& resource_group = "",
                           const AutoScaledMode auto_scaled = ASM_Disabled,
                           const Sizef& native_res = Sizef(640.0f, 480.0f),
                           XMLResourceExistsAction action = XREA_RETURN);

    /*!
    \brief
        Notify the FontManager that display size may have changed.

    \param size
        Size object describing the display resolution
    */
    void notifyDisplaySizeChanged(const Sizef& size);

    /*!
    \brief
        Writes a full XML font file for the specified Font to the given
        OutStream.

    \param name
        String holding the name of the Font to be written to the stream.

    \param out_stream
        OutStream (std::ostream based) object where data is to be sent.
    */
    void writeFontToStream(const String& name, OutStream& out_stream) const;

    //! ConstBaseIterator type definition.
    typedef ConstMapIterator<ObjectRegistry> FontIterator;

    /*!
    \brief
        Return a FontManager::FontIterator object to iterate over the available
        Font objects.
    */
    FontIterator getIterator() const;

    // ensure we see overloads from template base class
    using NamedXMLResourceManager<Font, Font_xmlHandler>::createFromContainer;
    using NamedXMLResourceManager<Font, Font_xmlHandler>::createFromFile;
    using NamedXMLResourceManager<Font, Font_xmlHandler>::createFromString;
};

} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUIFontManager_h_
