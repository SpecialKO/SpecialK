/***********************************************************************
    created:    21/2/2004
    author:     Paul D Turner <paul@cegui.org.uk>
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2011 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIFont_h_
#define _CEGUIFont_h_

#include "CEGUI/Base.h"
#include "CEGUI/PropertySet.h"
#include "CEGUI/EventSet.h"
#include "CEGUI/String.h"
#include "CEGUI/XMLSerializer.h"
#include "CEGUI/FontGlyph.h"

#include <map>

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Class that encapsulates a typeface.

    A Font object is created for each unique typeface required.
    The Font class provides methods for loading typefaces from various sources,
    and then for outputting text via the Renderer object.

    This class is not specific to any font renderer, it just provides the
    basic interfaces needed to manage fonts.
*/
class CEGUIEXPORT Font :
    public PropertySet,
    public EventSet,
    public AllocatedObject<Font>
{
public:
    //! Colour value used whenever a colour is not specified.
    static const argb_t DefaultColour;

    //! Event namespace for font events
    static const String EventNamespace;
    /** Event fired when the font internal state has changed such that the
     * rendered size of they glyphs is different.
     * Handlers are passed a const FontEventArgs reference with
     * FontEventArgs::font set to the Font whose rendered size has changed.
     */
    static const String EventRenderSizeChanged;

    //! Destructor.
    virtual ~Font();

    //! Return the string holding the font name.
    const String& getName() const;

    //! Return the type of the font.
    const String& getTypeName() const;

    //! Return the filename of the used font.
    const String& getFileName() const;
    
    /*!
    \brief
        Return whether this Font can draw the specified code-point

    \param cp
        utf32 code point that is the subject of the query.

    \return
        true if the font contains a mapping for code point \a cp,
        false if it does not contain a mapping for \a cp.
    */
    bool isCodepointAvailable(utf32 cp) const
    { return (d_cp_map.find(cp) != d_cp_map.end()); }

    /*!
    \brief
        Draw text into a specified area of the display.

    \param buffer
        GeometryBuffer object where the geometry for the text be queued.

    \param text
        String object containing the text to be drawn.

    \param position
        Reference to a Vector2 object describing the location at which the text
        is to be drawn.

    \param clip_rect
        Rect object describing the clipping area for the drawing.
        No drawing will occur outside this Rect.

    \param colours
        ColourRect object describing the colours to be applied when drawing the
        text.  NB: The colours specified in here are applied to each glyph,
        rather than the text as a whole.

    \param space_extra
        Number of additional pixels of spacing to be added to space characters.

    \param x_scale
        Scaling factor to be applied to each glyph's x axis, where 1.0f is
        considered to be 'normal'.

    \param y_scale
        Scaling factor to be applied to each glyph's y axis, where 1.0f is
        considered to be 'normal'.

    \return
        The x co-ord where subsequent text should be rendered to ensure correct
        positioning (which is not possible to determine accurately by using the
        extent measurement functions).
    */
    float drawText(GeometryBuffer& buffer, const String& text,
                   const Vector2f& position, const Rectf* clip_rect,
                   const ColourRect& colours, const float space_extra = 0.0f,
                   const float x_scale = 1.0f, const float y_scale = 1.0f) const;

    /*!
    \brief
        Set the native resolution for this Font

    \param size
        Size object describing the new native screen resolution for this Font.
    */
    void setNativeResolution(const Sizef& size);

    /*!
    \brief
        Return the native display size for this Font.  This is only relevant if
        the Font is being auto-scaled.

    \return
        Size object describing the native display size for this Font.
    */
    const Sizef& getNativeResolution() const;

    /*!
    \brief
        Enable or disable auto-scaling for this Font.

    \param auto_scaled
        AutoScaledMode describing how this font should be auto scaled

    \see AutoScaledMode
    */
    void setAutoScaled(const AutoScaledMode auto_scaled);

    /*!
    \brief
        Checks whether this font is being auto-scaled and how.

    \return
        AutoScaledMode describing how this font should be auto scaled
    */
    AutoScaledMode getAutoScaled() const;

    /*!
    \brief
        Notify the Font that the display size may have changed.

    \param size
        Size object describing the display resolution
    */
    virtual void notifyDisplaySizeChanged(const Sizef& size);

    /*!
    \brief
        Return the pixel line spacing value for.

    \param y_scale
        Scaling factor to be applied to the line spacing, where 1.0f
        is considered to be 'normal'.

    \return
        Number of pixels between vertical base lines, i.e. The minimum
        pixel space between two lines of text.
    */
    float getLineSpacing(float y_scale = 1.0f) const
    { return d_height * y_scale; }

    /*!
    \brief
        return the exact pixel height of the font.

    \param y_scale
        Scaling factor to be applied to the height, where 1.0f
        is considered to be 'normal'.

    \return
        float value describing the pixel height of the font without
        any additional padding.
    */
    float getFontHeight(float y_scale = 1.0f) const
    { return (d_ascender - d_descender) * y_scale; }

    /*!
    \brief
        Return the number of pixels from the top of the highest glyph
        to the baseline

    \param y_scale
        Scaling factor to be applied to the baseline distance, where 1.0f
        is considered to be 'normal'.

    \return
        pixel spacing from top of front glyphs to baseline
    */
    float getBaseline(float y_scale = 1.0f) const
    { return d_ascender * y_scale; }

    /*!
    \brief
        Return the pixel width of the specified text if rendered with
        this Font.

    \param text
        String object containing the text to return the rendered pixel
        width for.

    \param x_scale
        Scaling factor to be applied to each glyph's x axis when
        measuring the extent, where 1.0f is considered to be 'normal'.

    \return
        Number of pixels that \a text will occupy when rendered with
        this Font.

    \note
        The difference between the advance and the extent of a text string is
        important for numerous reasons. Picture some scenario where a glyph
        has a swash which extends way beyond the subsequent glyph - the text
        extent of those two glyphs is to the end of the swash on the first glyph
        whereas the advance of those two glyphs is to the start of a theoretical
        third glyph - still beneath the swash of the first glyph.
        The difference can basically be summarised as follows:
        - the extent is the total rendered width of all glyphs in the string.
        - the advance is the width to the point where the next character would
          have been drawn.

    \see getTextAdvance
    */
    float getTextExtent(const String& text, float x_scale = 1.0f) const;

    /*!
    \brief
        Return pixel advance of the specified text when rendered with this Font.

    \param text
        String object containing the text to return the pixel advance for.

    \param x_scale
        Scaling factor to be applied to each glyph's x axis when
        measuring the advance, where 1.0f is considered to be 'normal'.

    \return
        pixel advance of \a text when rendered with this Font.

    \note
        The difference between the advance and the extent of a text string is
        important for numerous reasons. Picture some scenario where a glyph
        has a swash which extends way beyond the subsequent glyph - the text
        extent of those two glyphs is to the end of the swash on the first glyph
        whereas the advance of those two glyphs is to the start of a theoretical
        third glyph - still beneath the swash of the first glyph.
        The difference can basically be summarised as follows:
        - the extent is the total rendered width of all glyphs in the string.
        - the advance is the width to the point where the next character would
          have been drawn.

    \see getTextExtent
    */
    float getTextAdvance(const String& text, float x_scale = 1.0f) const;

    /*!
    \brief
        Return the index of the closest text character in String \a text
        that corresponds to pixel location \a pixel if the text were rendered.

    \param text
        String object containing the text.

    \param pixel
        Specifies the (horizontal) pixel offset to return the character
        index for.

    \param x_scale
        Scaling factor to be applied to each glyph's x axis when measuring
        the text extent, where 1.0f is considered to be 'normal'.

    \return
        Returns a character index into String \a text for the character that
        would be rendered closest to horizontal pixel offset \a pixel if the
        text were to be rendered via this Font.  Range of the return is from
        0 to text.length(), so may actually return an index past the end of
        the string, which indicates \a pixel was beyond the last character.
    */
    size_t getCharAtPixel(const String& text, float pixel,
                          float x_scale = 1.0f) const
    { return getCharAtPixel(text, 0, pixel, x_scale); }

    /*!
    \brief
        Return the index of the closest text character in String \a text,
        starting at character index \a start_char, that corresponds
        to pixel location \a pixel if the text were to be rendered.

    \param text
        String object containing the text.

    \param start_char
        index of the first character to consider.  This is the lowest
        value that will be returned from the call.

    \param pixel
        Specifies the (horizontal) pixel offset to return the character
        index for.

    \param x_scale
        Scaling factor to be applied to each glyph's x axis when measuring
        the text extent, where 1.0f is considered to be 'normal'.

    \return
        Returns a character index into String \a text for the character that
        would be rendered closest to horizontal pixel offset \a pixel if the
        text were to be rendered via this Font.  Range of the return is from
        0 to text.length(), so may actually return an index past the end of
        the string, which indicates \a pixel was beyond the last character.
    */
    size_t getCharAtPixel(const String& text, size_t start_char, float pixel,
                          float x_scale = 1.0f) const;

    /*!
    \brief
        Sets the default resource group to be used when loading font data

    \param resourceGroup
        String describing the default resource group identifier to be used.

    \return
        Nothing.
    */
    static void setDefaultResourceGroup(const String& resourceGroup)
    { d_defaultResourceGroup = resourceGroup; }

    /*!
    \brief
        Returns the default resource group currently set for Fonts.

    \return
        String describing the default resource group identifier that will be
        used when loading font data.
    */
    static const String& getDefaultResourceGroup()
    { return d_defaultResourceGroup; }

    /*!
    \brief
        Writes an xml representation of this Font to \a out_stream.

    \param xml_stream
        Stream where xml data should be output.

    \return
        Nothing.
    */
    void writeXMLToStream(XMLSerializer& xml_stream) const;

    /*!
    \brief
        Return a pointer to the glyphDat struct for the given codepoint,
        or 0 if the codepoint does not have a glyph defined.

    \param codepoint
        utf32 codepoint to return the glyphDat structure for.

    \return
        Pointer to the glyphDat struct for \a codepoint, or 0 if no glyph
        is defined for \a codepoint.
    */
    const FontGlyph* getGlyphData(utf32 codepoint) const;

protected:
    //! Constructor.
    Font(const String& name, const String& type_name, const String& filename,
         const String& resource_group, const AutoScaledMode auto_scaled,
         const Sizef& native_res);

    /*!
    \brief
        This function prepares a certain range of glyphs to be ready for
        displaying. This means that after returning from this function
        glyphs from d_cp_map[start_codepoint] to d_cp_map[end_codepoint]
        should have their d_image member set. If there is an error
        during rasterisation of some glyph, it's okay to leave the
        d_image field set to NULL, in which case such glyphs will
        be skipped from display.
    \param start_codepoint
        The lowest codepoint that should be rasterised
    \param end_codepoint
        The highest codepoint that should be rasterised
    */
    virtual void rasterise(utf32 start_codepoint, utf32 end_codepoint) const;

    //! Update the font as needed, according to the current parameters.
    virtual void updateFont() = 0;

    //! implementaion version of writeXMLToStream.
    virtual void writeXMLToStream_impl(XMLSerializer& xml_stream) const = 0;

    //! Register all properties of this class.
    void addFontProperties();

    //! event trigger function for when the font rendering size changes.
    virtual void onRenderSizeChanged(FontEventArgs& args);

    /*!
    \brief
        Set the maximal glyph index. This reserves the respective
        number of bits in the d_glyphPageLoaded array.
    */
    void setMaxCodepoint(utf32 codepoint);

    //! finds FontGlyph in map and returns it, or 0 if none.
    virtual const FontGlyph* findFontGlyph(const utf32 codepoint) const;

    //! Name of this font.
    String d_name;
    //! Type name string for this font (not used internally)
    String d_type;
    //! Name of the file used to create this font (font file or imagset)
    String d_filename;
    //! Name of the font file's resource group.
    String d_resourceGroup;
    //! Holds default resource group for font loading.
    static String d_defaultResourceGroup;

    //! maximal font ascender (pixels above the baseline)
    float d_ascender;
    //! maximal font descender (negative pixels below the baseline)
    float d_descender;
    //! (ascender - descender) + linegap
    float d_height;

    //! which mode should we use for auto-scaling
    AutoScaledMode d_autoScaled;
    //! native resolution for this Font.
    Sizef d_nativeResolution;
    //! current horizontal scaling factor.
    float d_horzScaling;
    //! current vertical scaling factor.
    float d_vertScaling;

    //! Maximal codepoint for font glyphs
    utf32 d_maxCodepoint;

    /*!
    \brief
        This bitmap holds information about loaded 'pages' of glyphs.
        A glyph page is a set of 256 codepoints, starting at 256-multiples.
        For example, the 1st glyph page is 0-255, fourth is 1024-1279 etc.
        When a specific glyph is required for painting, the corresponding
        bit is checked to see if the respective page has been rasterised.
        If not, the rasterise() method is invoked, which prepares the
        glyphs from the respective glyph page for being painted.

        This array is big enough to hold at least max_codepoint bits.
        If this member is NULL, all glyphs are considered pre-rasterised.
    */
    uint* d_glyphPageLoaded;

    //! Definition of CodepointMap type.
    typedef std::map<utf32, FontGlyph, std::less<utf32>
        CEGUI_MAP_ALLOC(utf32, FontGlyph)> CodepointMap;
    //! Contains mappings from code points to Image objects
    mutable CodepointMap d_cp_map;
};



} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif


#endif  // end of guard _CEGUIFont_h_
