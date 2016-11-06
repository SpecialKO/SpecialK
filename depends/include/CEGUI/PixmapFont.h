/***********************************************************************
    created:    14/6/2006
    author:     Andrew Zabolotny
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
#ifndef _CEGUIPixmapFont_h_
#define _CEGUIPixmapFont_h_

#include "CEGUI/Font.h"

// Start of CEGUI namespace section
namespace CEGUI
{

/*!
\brief
    Implementation of the Font class interface using static Imageset's.

    To create such a font you must create a Imageset with all the glyphs,
    and then define individual glyphs via defineMapping.
*/
class PixmapFont : public Font
{
public:
    /*!
    \brief
        Constructor for Pixmap type fonts.

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
    */
    PixmapFont(const String& font_name, const String& imageset_filename,
               const String& resource_group = "",
               const AutoScaledMode auto_scaled = ASM_Disabled,
               const Sizef& native_res = Sizef(640.0f, 480.0f));

    //! Destructor.
    ~PixmapFont();

    void defineMapping(const utf32 codepoint, const String& image_name, 
                       const float horz_advance);
    void defineMapping(const String& value);
    //! Return the image name prefix that the font is using for it's glyphs.
    const String& getImageNamePrefix() const;

    /*!
    \brief
        Set image name prefix font should use for it's glyphs.

        This will potentially cause existing images to be destroyed (if they
        were created specifically by, and for, this Font).  Images using the new
        name prefix must already exist within the system.

    \param name_prefix
        Name prefix used by an existing set of images to be used as the glyph
        source for this Font.
    */
    void setImageNamePrefix(const String& name_prefix);

protected:
    //! Initialize the imageset.
    void reinit();
    //! Register all properties of this class.
    void addPixmapFontProperties();

    // override of functions in Font base class.
    void updateFont ();
    void writeXMLToStream_impl (XMLSerializer& xml_stream) const;

    //! The Image name prefix used for the glyphs
    String d_imageNamePrefix;
    //! Current X scaling for glyph images
    float d_origHorzScaling;
    //! true if we own the imageset
    bool d_imagesetOwner;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIPixmapFont_h_
