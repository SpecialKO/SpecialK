/***********************************************************************
    created:    Sun Jun 19 2005
    author:     Paul D Turner <paul@cegui.org.uk>
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2012 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIFalTextComponent_h_
#define _CEGUIFalTextComponent_h_

#include "./ComponentBase.h"
#include "../RenderedString.h"
#include "../RefCounted.h"
#include "../FormattedRenderedString.h"
#include "CEGUI/falagard/FormattingSetting.h"

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
    /*!
    \brief
        Class that encapsulates information for a text component.
    */
    class CEGUIEXPORT TextComponent : public FalagardComponentBase
    {
    public:
        TextComponent();
        ~TextComponent();
        TextComponent(const TextComponent& obj);
        TextComponent& operator=(const TextComponent& other);

        /*!
        \brief
            Return the text set for this TextComponent.

        \note
            This returns the text string set directly to the TextComponent,
            which may or may not be the actual string that will be used -
            since the actual string may be sourced from a property or the main
            text string from a window that the TextComponent is rendered to.
            To get the actual string, call the getEffectiveText function
            instead.
        */
        const String& getText() const;

        /*
        \brief
            Return a copy of the actual text string that will be used when
            rendering this TextComponent.
        */
        String getEffectiveText(const Window& wnd) const;

        /*!
        \brief
            return text string with \e visual ordering of glyphs.

        \note
            This returns the visual text derived from the string set directly to
            the TextComponent, which may or may not be the actual string that
            will be used - since the actual string may be sourced from a
            property or the main text string from a window that the
            TextComponent is rendered to. To get the actual visual string, call
            the getEffectiveVisualText function instead.
        */
        const String& getTextVisual() const;

        /*
        \brief
            Return a copy of the actual text - with visual ordering - that
            will be used when rendering this TextComponent.
        */
        String getEffectiveVisualText(const Window& wnd) const;

        /*!
        \brief
            Set the text string for this TextComponent.

        \note
            Setting this string may not set the actual string that will be used
            when rendering the TextComponent.  The acutal string used will
            depend upon whether a text source property is set and whether this
            string is set to the empty string or not.

        \param text
            String containing text to set on the TextComponent.
        */
        void setText(const String& text);

        /*!
        \brief
            Return the name of the font set to be used when rendering this
            TextComponent.

        \note
            This returns the name of the font set directly to the TextComponent,
            which may or may not be the actual font that will be used -
            since the actual font may be sourced from a property or the main
            font setting on a window that the TextComponent is rendered to, or
            the default font. To get the actual font name that will be used,
            call the getEffectiveFont function instead.

        \return
            String object containing the name of a font
        */
        const String& getFont() const;

        /*
        \brief
            Return a copy of the name of the font that will actually be used
            when rendering this TextComponent.
        */
        String getEffectiveFont(const Window& wnd) const;

        /*!
        \brief
            Set the name of a font to be used when rendering this TextComponent.

        \note
            Setting this may not set the actual font that will be used
            when rendering the TextComponent.  The acutal font used will
            depend upon whether a font source property is set and whether the
            font name set here is set to the empty string or not.

        \param font
            String containing name of a font
        */
        void setFont(const String& font);

        /*!
        \brief
            Return the current vertical formatting setting for this TextComponent.

        \return
            One of the VerticalTextFormatting enumerated values.
        */
        VerticalTextFormatting getVerticalFormatting(const Window& wnd) const;

        /*!
        \brief
            Directly returns the vertical formatting which was set for the ImageryComponent.

        \return
            The VerticalTextFormatting enum.
        */
        VerticalTextFormatting getVerticalFormattingFromComponent() const;

        /*!
        \brief
            Set the vertical formatting setting for this TextComponent.

        \param fmt
            One of the VerticalTextFormatting enumerated values.

        \return
            Nothing.
        */
        void setVerticalFormatting(VerticalTextFormatting fmt);

        /*!
        \brief
            Return the current horizontal formatting setting for this TextComponent.

        \return
            One of the HorizontalTextFormatting enumerated values.
        */
        HorizontalTextFormatting getHorizontalFormatting(const Window& wnd) const;

        /*!
        \brief
            Directly returns the horizontal formatting which was set for the ImageryComponent.

        \return
            The HorizontalTextFormatting enum.
        */
        HorizontalTextFormatting getHorizontalFormattingFromComponent() const;

        /*!
        \brief
            Set the horizontal formatting setting for this TextComponent.

        \param fmt
            One of the HorizontalTextFormatting enumerated values.

        \return
            Nothing.
        */
        void setHorizontalFormatting(HorizontalTextFormatting fmt);

        /*!
        \brief
            Returns the name of the property that will be used to obtain the horizontal
            formatting to use for this ImageryComponent or an empty string if none is set.

        \return
            A String containing the name of the property
        */
        const String& getHorizontalFormattingPropertySource() const;

        /*!
        \brief
            Set the name of a property that will be used to obtain the horizontal
            formatting to use for this ImageryComponent.
        */
        void setHorizontalFormattingPropertySource(const String& property_name);

        /*!
        \brief
            Returns the name of the property that will be used to obtain the vertical
            formatting to use for this ImageryComponent or an empty string if none is set.

        \return
            A String containing the name of the property
        */
        const String& getVerticalFormattingPropertySource() const;

        /*!
        \brief
            Set the name of a property that will be used to obtain the vertical
            formatting to use for this ImageryComponent.
        */
        void setVerticalFormattingPropertySource(const String& property_name);

        /*!
        \brief
            Writes an xml representation of this TextComponent to \a out_stream.

        \param xml_stream
            Stream where xml data should be output.


        \return
            Nothing.
        */
        void writeXMLToStream(XMLSerializer& xml_stream) const;

        /*!
        \brief
            Return whether this TextComponent fetches it's text string via a property on the target window.

        \return
            - true if the text string comes via a Propery.
            - false if the text string is defined explicitly, or will come from the target window.
        */
        bool isTextFetchedFromProperty() const;

        /*!
        \brief
            Return the name of the property that will be used to determine the text string to render
            for this TextComponent.

        \return
            String object holding the name of a Propery.
        */
        const String& getTextPropertySource() const;

        /*!
        \brief
            Set the name of the property that will be used to determine the text string to render
            for this TextComponent.

        \param property
            String object holding the name of a Propery.  The property can contain any text string to render.

        \return
            Nothing.
        */
        void setTextPropertySource(const String& property);
        
        /*!
        \brief
            Return whether this TextComponent fetches it's font via a property on the target window.

        \return
            - true if the font comes via a Propery.
            - false if the font is defined explicitly, or will come from the target window.
        */
        bool isFontFetchedFromProperty() const;

        /*!
        \brief
            Return the name of the property that will be used to determine the font to use for rendering
            the text string for this TextComponent.

        \return
            String object holding the name of a Propery.
        */
        const String& getFontPropertySource() const;

        /*!
        \brief
            Set the name of the property that will be used to determine the font to use for rendering
            the text string of this TextComponent.

        \param property
            String object holding the name of a Propery.  The property should access a valid font name.

        \return
            Nothing.
        */
        void setFontPropertySource(const String& property);

        //! return the horizontal pixel extent of the formatted rendered string.
        float getHorizontalTextExtent(const Window& window) const;

        //! return the vertical pixel extent of the formatted rendered string.
        float getVerticalTextExtent(const Window& window) const;
    
        // overridden from ComponentBase.
        bool handleFontRenderSizeChange(Window& window, const Font* font) const;


        //! Update string formatting.
        void updateFormatting(const Window& srcWindow) const;

        /*!
        \brief
            Update string formatting.
        
        \param size
            The pixel size of the component.
        */
        void updateFormatting(const Window& srcWindow, const Sizef& size) const;

    protected:
        // implemets abstract from base
        void render_impl(Window& srcWindow, Rectf& destRect, const CEGUI::ColourRect* modColours, const Rectf* clipper, bool clipToDisplay) const;
        //! helper to set up an appropriate FormattedRenderedString
        void setupStringFormatter(const Window& window,
                                  const RenderedString& rendered_string) const;
        //! helper to get the font object to use
        const Font* getFontObject(const Window& window) const;

    private:
        //! text rendered by this component.
        String d_textLogical;
        //! pointer to bidirection support object
        BidiVisualMapping* d_bidiVisualMapping;
        //! whether bidi visual mapping has been updated since last text change.
        mutable bool d_bidiDataValid;
        //! RenderedString used when not using the one from the target Window.
        mutable RenderedString d_renderedString;
        //! FormattedRenderedString object that applies formatting to the string
        mutable RefCounted<FormattedRenderedString> d_formattedRenderedString;
        //! Tracks last used horizontal formatting (in order to detect changes)
        mutable HorizontalTextFormatting d_lastHorzFormatting;

        String               d_font;            //!< name of font to use.
        //! Vertical formatting to be applied when rendering the image component.
        FormattingSetting<VerticalTextFormatting> d_vertFormatting;
        //! Horizontal formatting to be applied when rendering the image component.
        FormattingSetting<HorizontalTextFormatting> d_horzFormatting;
        String  d_textPropertyName;             //!< Name of the property to access to obtain the text string to render.
        String  d_fontPropertyName;             //!< Name of the property to access to obtain the font to use for rendering.
    };

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

#endif  // end of guard _CEGUIFalTextComponent_h_
